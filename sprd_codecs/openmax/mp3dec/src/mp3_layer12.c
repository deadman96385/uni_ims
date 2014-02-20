/********************************************************************************
**  File Name: 	mp3_layer12.c           						                *
**  Author:		Tan Li      									                *
**  Date:		17/01/2011                                                      *
**  Copyright:	2011 Spreadtrum, Incorporated. All Rights Reserved.		        *
**  Description:  layer12 decoding function        						        *
*********************************************************************************
*********************************************************************************
**  Edit History											                    *
**------------------------------------------------------------------------------*
**  DATE			NAME			DESCRIPTION				                    *
**  17/01/2011		Tan li    		Create. 				                    *
*********************************************************************************/

# ifdef HAVE_CONFIG_H
#  include "mp3_config.h"
# endif

//# include "mp3_global.h"

# ifdef HAVE_LIMITS_H
#  include <limits.h>
# else
#  define CHAR_BIT  8
# endif

# include "mp3_fixed.h"
# include "mp3_bit.h"
# include "mp3_stream.h"
# include "mp3_frame.h"
# include "mp3_layer12.h"


/*
 * scalefactor table
 * used in both Layer I and Layer II decoding
 */
static
mp3_fixed_t const sf_table[64] = {
# include "sf_table.dat"
};

/* --- Layer I ------------------------------------------------------------- */

/* linear scaling table */
static
mp3_fixed_t const linear_table[14] = {
  MP3_F(0x15555555),  /* 2^2  / (2^2  - 1) == 1.33333333333333 */
  MP3_F(0x12492492),  /* 2^3  / (2^3  - 1) == 1.14285714285714 */
  MP3_F(0x11111111),  /* 2^4  / (2^4  - 1) == 1.06666666666667 */
  MP3_F(0x10842108),  /* 2^5  / (2^5  - 1) == 1.03225806451613 */
  MP3_F(0x10410410),  /* 2^6  / (2^6  - 1) == 1.01587301587302 */
  MP3_F(0x10204081),  /* 2^7  / (2^7  - 1) == 1.00787401574803 */
  MP3_F(0x10101010),  /* 2^8  / (2^8  - 1) == 1.00392156862745 */
  MP3_F(0x10080402),  /* 2^9  / (2^9  - 1) == 1.00195694716243 */
  MP3_F(0x10040100),  /* 2^10 / (2^10 - 1) == 1.00097751710655 */
  MP3_F(0x10020040),  /* 2^11 / (2^11 - 1) == 1.00048851978505 */
  MP3_F(0x10010010),  /* 2^12 / (2^12 - 1) == 1.00024420024420 */
  MP3_F(0x10008004),  /* 2^13 / (2^13 - 1) == 1.00012208521548 */
  MP3_F(0x10004001),  /* 2^14 / (2^14 - 1) == 1.00006103888177 */
  MP3_F(0x10002000)   /* 2^15 / (2^15 - 1) == 1.00003051850948 */
};

/*
 * NAME:	I_sample()
 * DESCRIPTION:	decode one requantized Layer I sample from a bitstream
 */
static
mp3_fixed_t I_sample(MP3_DEC_BIT_POOL_T *ptr, unsigned int nb)
{
  mp3_fixed_t sample;

  sample = MP3_DEC_8BitRead(ptr, nb);

  /* invert most significant bit, extend sign, then scale to fixed format */

  sample ^= 1 << (nb - 1);
  sample |= -(sample & (1 << (nb - 1)));

  sample <<= MP3_F_FRACBITS - (nb - 1);

  /* requantize the sample */

  /* s'' = (2^nb / (2^nb - 1)) * (s''' + 2^(-nb + 1)) */

  sample += MP3_F_ONE >> (nb - 1);

  return mp3_f_mul(sample, linear_table[nb - 2]);

  /* s' = factor * s'' */
  /* (to be performed by caller) */
}

/*
 * NAME:	layer->I()
 * DESCRIPTION:	decode a single Layer I frame
 */
int32 MP3_DEC_LayerI(MP3_STREAM_T *stream, MP3_FRAME_T *frame)
{
  MP3_HEADER_T *header = &frame->header;
  uint32 nch, bound, ch, s, sb, nb;
  uint32 allocation[2][32] = {0}, scalefactor[2][32]={0};

  nch = header->num_ch;//MP3_NCHANNELS(header);

  bound = 32;
  if (header->mode == MP3_MODE_JOINT_STEREO) {
    header->flags |= MP3_FLAG_I_STEREO;
    bound = 4 + header->mode_extension * 4;
  }

  /* check CRC word */

  if (header->flags & MP3_FLAG_PROTECTION) {
    header->crc_check =
      MP3_DEC_BIT_CRCCheck(stream->ptr, 4 * (bound * nch + (32 - bound)),
		  header->crc_check);

    if (header->crc_check != header->crc_target &&
	!(frame->options & MP3_OPTION_IGNORECRC)) {
      stream->error = MP3_ERROR_BADCRC;
      return -1;
    }
  }

  /* decode bit allocations */

  for (sb = 0; sb < bound; ++sb) {
    for (ch = 0; ch < nch; ++ch) {
      nb = MP3_DEC_8BitRead(&stream->ptr, 4);

      if (nb == 15) {
	stream->error = MP3_ERROR_BADBITALLOC;
	return -1;
      }

      allocation[ch][sb] = nb ? nb + 1 : 0;
    }
  }

  for (sb = bound; sb < 32; ++sb) {
    nb = MP3_DEC_8BitRead(&stream->ptr, 4);

    if (nb == 15) {
      stream->error = MP3_ERROR_BADBITALLOC;
      return -1;
    }

    allocation[0][sb] =
    allocation[1][sb] = (nb ? nb + 1 : 0);
  }

  /* decode scalefactors */

  for (sb = 0; sb < 32; ++sb) {
    for (ch = 0; ch < nch; ++ch) {
      if (allocation[ch][sb]) {
	scalefactor[ch][sb] = MP3_DEC_8BitRead(&stream->ptr, 6);

# if defined(OPT_STRICT)
	/*
	 * Scalefactor index 63 does not appear in Table B.1 of
	 * ISO/IEC 11172-3. Nonetheless, other implementations accept it,
	 * so we only reject it if OPT_STRICT is defined.
	 */
	if (scalefactor[ch][sb] == 63) {
	  stream->error = MP3_ERROR_BADSCALEFACTOR;
	  return -1;
	}
# endif
      }
    }
  }

  /* decode samples */

  for (s = 0; s < 12; ++s) {
    for (sb = 0; sb < bound; ++sb) {
      for (ch = 0; ch < nch; ++ch) {
	nb = allocation[ch][sb];
	frame->sbsample[ch][s][sb] = nb ?
	  mp3_f_mul(I_sample(&stream->ptr, nb),
		    sf_table[scalefactor[ch][sb]]) : 0;
      }
    }

    for (sb = bound; sb < 32; ++sb) 
    {
    	nb = allocation[0][sb];
      if (nb) {
	mp3_fixed_t sample;

	sample = I_sample(&stream->ptr, nb);

	for (ch = 0; ch < nch; ++ch) {
	  frame->sbsample[ch][s][sb] =
	    mp3_f_mul(sample, sf_table[scalefactor[ch][sb]]);
	}
      }
      else {
	for (ch = 0; ch < nch; ++ch)
	  frame->sbsample[ch][s][sb] = 0;
      }
    }
  }

  return 0;
}

/* --- Layer II ------------------------------------------------------------ */

/* possible quantization per subband table */
static
struct {
  unsigned int sblimit;
  unsigned char const offsets[30];
} const sbquant_table[5] = {
  /* ISO/IEC 11172-3 Table B.2a */
  { 27, { 7, 7, 7, 6, 6, 6, 6, 6, 6, 6, 6, 3, 3, 3, 3, 3,	/* 0 */
	  3, 3, 3, 3, 3, 3, 3, 0, 0, 0, 0 } }, /*lint !e785*/
  /* ISO/IEC 11172-3 Table B.2b */
  { 30, { 7, 7, 7, 6, 6, 6, 6, 6, 6, 6, 6, 3, 3, 3, 3, 3,	/* 1 */
	  3, 3, 3, 3, 3, 3, 3, 0, 0, 0, 0, 0, 0, 0 } }, /*lint !e785*/
  /* ISO/IEC 11172-3 Table B.2c */
  {  8, { 5, 5, 2, 2, 2, 2, 2, 2 } },				/* 2 */ /*lint !e785*/
  /* ISO/IEC 11172-3 Table B.2d */
  { 12, { 5, 5, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2 } },		/* 3 */ /*lint !e785*/
  /* ISO/IEC 13818-3 Table B.1 */
  { 30, { 4, 4, 4, 4, 2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1,	/* 4 */
	  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 } } /*lint !e785*/
};

/* bit allocation table */
static
struct {
  unsigned short nbal;
  unsigned short offset;
} const bitalloc_table[8] = {
  { 2, 0 },  /* 0 */
  { 2, 3 },  /* 1 */
  { 3, 3 },  /* 2 */
  { 3, 1 },  /* 3 */
  { 4, 2 },  /* 4 */
  { 4, 3 },  /* 5 */
  { 4, 4 },  /* 6 */
  { 4, 5 }   /* 7 */
};

/* offsets into quantization class table */
static
unsigned char const offset_table[6][15] = {
  { 0, 1, 16                                             },  /* 0 */ /*lint !e785*/
  { 0, 1,  2, 3, 4, 5, 16                                },  /* 1 */ /*lint !e785*/
  { 0, 1,  2, 3, 4, 5,  6, 7,  8,  9, 10, 11, 12, 13, 14 },  /* 2 */
  { 0, 1,  3, 4, 5, 6,  7, 8,  9, 10, 11, 12, 13, 14, 15 },  /* 3 */
  { 0, 1,  2, 3, 4, 5,  6, 7,  8,  9, 10, 11, 12, 13, 16 },  /* 4 */
  { 0, 2,  4, 5, 6, 7,  8, 9, 10, 11, 12, 13, 14, 15, 16 }   /* 5 */
};

/* quantization class table */
static
struct quantclass {
  unsigned short nlevels;
  unsigned char group;
  unsigned char bits;
  mp3_fixed_t C;
  mp3_fixed_t D;
} const qc_table[17] = {
# include "qc_table.dat"
};

/*
 * NAME:	II_samples()
 * DESCRIPTION:	decode three requantized Layer II samples from a bitstream
 */
static
void II_samples(MP3_DEC_BIT_POOL_T *ptr,
		struct quantclass const *quantclass,
		mp3_fixed_t output[3])
{
  unsigned int nb, s, sample[3];

	nb = quantclass->group;
  if (nb) {
    unsigned int c, nlevels;

    /* degrouping */
    c = MP3_DEC_8BitRead(ptr, quantclass->bits);
    nlevels = quantclass->nlevels;

    for (s = 0; s < 3; ++s) {
      sample[s] = c % nlevels;
      c /= nlevels;
    }
  }
  else {
    nb = quantclass->bits;

    for (s = 0; s < 3; ++s)
      sample[s] = MP3_DEC_8BitRead(ptr, nb);
  }

  for (s = 0; s < 3; ++s) {
    mp3_fixed_t requantized;

    /* invert most significant bit, extend sign, then scale to fixed format */

    requantized  = sample[s] ^ (1 << (nb - 1));
    requantized |= -(requantized & (1 << (nb - 1)));

    requantized <<= MP3_F_FRACBITS - (nb - 1);

    /* requantize the sample */

    /* s'' = C * (s''' + D) */

    output[s] = mp3_f_mul(requantized + quantclass->D, quantclass->C); /*lint !e776*/

    /* s' = factor * s'' */
    /* (to be performed by caller) */
  }
}

/*
 * NAME:	layer->II()
 * DESCRIPTION:	decode a single Layer II frame
 */
int32 MP3_DEC_LayerII(MP3_STREAM_T *stream, MP3_FRAME_T *frame)
{
  MP3_HEADER_T *header = &frame->header;
  MP3_DEC_BIT_POOL_T start;
  unsigned int index, sblimit, nbal, nch, bound, gr, ch, s, sb;
  unsigned char const *offsets;
    uint32 *allocation_ptr, *scfsi_ptr, *scalefactor_ptr;
  mp3_fixed_t samples[3];
    uint32 bitrate_per_channel;
    allocation_ptr = (uint32*) (frame->allocation);
    memset(allocation_ptr, 0, 32*2*sizeof(uint32));    
    scfsi_ptr = (uint32*) (frame->scfsi);
    memset(scfsi_ptr, 0, 32*2*sizeof(uint32));    
    scalefactor_ptr = (uint32*) (frame->scalefactor);
    memset(scalefactor_ptr, 0, 32*2*3*sizeof(uint32));    

  nch = header->num_ch;//MP3_NCHANNELS(header);

  if (header->flags & MP3_FLAG_LSF_EXT)
    index = 4;
  else if (header->flags & MP3_FLAG_FREEFORMAT)
    goto freeformat;
    else 
    {

    bitrate_per_channel = header->bitrate;
        if (nch == 2) 
        {
      bitrate_per_channel /= 2;

# if defined(OPT_STRICT)
      /*
       * ISO/IEC 11172-3 allows only single channel mode for 32, 48, 56, and
       * 80 kbps bitrates in Layer II, but some encoders ignore this
       * restriction. We enforce it if OPT_STRICT is defined.
       */
      if (bitrate_per_channel <= 28000 || bitrate_per_channel == 40000) {
	stream->error = MP3_ERROR_BADMODE;
	return -1;
      }
# endif
    }
    else
    {  /* nch == 1 */
           if (bitrate_per_channel > 192000) 
           {
	/*
	 * ISO/IEC 11172-3 does not allow single channel mode for 224, 256,
	 * 320, or 384 kbps bitrates in Layer II.
	 */
	stream->error = MP3_ERROR_BADMODE;
	return -1;
      }
    }

    if (bitrate_per_channel <= 48000)
      index = (header->samplerate == 32000) ? 3 : 2;
    else if (bitrate_per_channel <= 80000)
      index = 0;
        else 
        {
    freeformat:
      index = (header->samplerate == 48000) ? 0 : 1;
    }
  }

  sblimit = sbquant_table[index].sblimit;
  offsets = sbquant_table[index].offsets;

  bound = 32;
    if (header->mode == MP3_MODE_JOINT_STEREO) 
    {
    header->flags |= MP3_FLAG_I_STEREO;
    bound = 4 + header->mode_extension * 4;
  }

  if (bound > sblimit)
    bound = sblimit;

  start = stream->ptr;

  /* decode bit allocations */

    for (sb = 0; sb < bound; ++sb) 
    {
    nbal = bitalloc_table[offsets[sb]].nbal;

    for (ch = 0; ch < nch; ++ch)
        {
            allocation_ptr[ch*32+sb] = MP3_DEC_8BitRead(&stream->ptr, nbal);
        }
  }

    for (sb = bound; sb < sblimit; ++sb) 
    {
    nbal = bitalloc_table[offsets[sb]].nbal;

        allocation_ptr/*[0]*/[sb] =  allocation_ptr[32+sb] = MP3_DEC_8BitRead(&stream->ptr, nbal);
  }

  /* decode scalefactor selection info */

    for (sb = 0; sb < sblimit; ++sb) 
    {
        for (ch = 0; ch < nch; ++ch) 
        {
            if (allocation_ptr[ch*32+sb])
                scfsi_ptr[ch*32+sb] = MP3_DEC_8BitRead(&stream->ptr, 2);
    }
  }

  /* check CRC word */

    if (header->flags & MP3_FLAG_PROTECTION) 
    {
        header->crc_check = MP3_DEC_BIT_CRCCheck(start, MP3_DEC_CalcBitLen(&start, &stream->ptr), header->crc_check);
        if (header->crc_check != header->crc_target && !(frame->options & MP3_OPTION_IGNORECRC)) 

        {
      stream->error = MP3_ERROR_BADCRC;
      return -1;
    }
  }

  /* decode scalefactors */

    for (sb = 0; sb < sblimit; ++sb) 
    {        
        for (ch = 0; ch < nch; ++ch) 
        {

            uint32 *t_ptr = scalefactor_ptr + ch*96 + sb*3;
            if (allocation_ptr[ch*32+sb]) 
            {
                t_ptr[0] = MP3_DEC_8BitRead(&stream->ptr, 6);
                switch (scfsi_ptr[ch*32+sb]) 
                {
	case 2:
                        t_ptr[2] = t_ptr[1] = t_ptr[0];
	  break;

	case 0:
                        t_ptr[1] = MP3_DEC_8BitRead(&stream->ptr, 6);
	  /* fall through */

	case 1: /*lint !e825*/
	case 3:
                        t_ptr[2] = MP3_DEC_8BitRead(&stream->ptr, 6);
	default: /*lint !e825 !e616*/
		break;	 
	}

                if (scfsi_ptr[ch*32+sb] & 1)
                    t_ptr[1] = t_ptr[scfsi_ptr[ch*32+sb] - 1];

# if defined(OPT_STRICT)
	/*
	 * Scalefactor index 63 does not appear in Table B.1 of
	 * ISO/IEC 11172-3. Nonetheless, other implementations accept it,
	 * so we only reject it if OPT_STRICT is defined.
	 */
	if (scalefactor_ptr[ch][sb][0] == 63 ||
	    scalefactor_ptr[ch][sb][1] == 63 ||
	    scalefactor_ptr[ch][sb][2] == 63) {
	  stream->error = MP3_ERROR_BADSCALEFACTOR;
	  return -1;
	}
# endif
      }
    }
  }

  /* decode samples */

    for (gr = 0; gr < 12; ++gr) 
    {
        for (sb = 0; sb < bound; ++sb) 
        {
            for (ch = 0; ch < nch; ++ch) 
            {
                uint32 *t_ptr = scalefactor_ptr + ch*96 + sb*3;
                index = allocation_ptr[ch*32+sb];
                if (index) 
                {
	  index = offset_table[bitalloc_table[offsets[sb]].offset][index - 1];

	  II_samples(&stream->ptr, &qc_table[index], samples);

                    for (s = 0; s < 3; ++s) 
                    {
                        frame->sbsample[ch][3 * gr + s][sb] = mp3_f_mul(samples[s], sf_table[t_ptr[gr / 4]]);
	  }
	}
                else 
                {
	  for (s = 0; s < 3; ++s)
	    frame->sbsample[ch][3 * gr + s][sb] = 0;
	}
      }
    }

        for (sb = bound; sb < sblimit; ++sb) 
        {
            index = allocation_ptr[sb];
            if (index) 
            {
	index = offset_table[bitalloc_table[offsets[sb]].offset][index - 1];

	II_samples(&stream->ptr, &qc_table[index], samples);

                 for (ch = 0; ch < nch; ++ch) 
                 {
                     uint32 *t_ptr = scalefactor_ptr + ch*96 + sb*3;
                     for (s = 0; s < 3; ++s) 
                    {
                        frame->sbsample[ch][3 * gr + s][sb] = mp3_f_mul(samples[s], sf_table[t_ptr[gr / 4]]);
	  }
	}
      }
            else 
            {
                for (ch = 0; ch < nch; ++ch) 
                {
	  for (s = 0; s < 3; ++s)
                    {
	    frame->sbsample[ch][3 * gr + s][sb] = 0;
	}
      }
    }

        }
        for (ch = 0; ch < nch; ++ch) 
        {
            for (s = 0; s < 3; ++s) 
            {
	for (sb = sblimit; sb < 32; ++sb)
                {
	  frame->sbsample[ch][3 * gr + s][sb] = 0;
      }
    }
  }
    }

  return 0;
}
