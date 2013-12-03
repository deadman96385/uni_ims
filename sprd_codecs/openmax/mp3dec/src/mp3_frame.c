/*************************************************************************
** File Name:      mp3_frame.c                                           *
** Date:           05/11/2010                                            *
** Copyright:      2010 Spreatrum, Incoporated. All Rights Reserved.     *
** Description:    This file is used to perform mp3 frame decoding       *
**                 
**                        Edit History                                   *
** ----------------------------------------------------------------------*
**************************************************************************/

# ifdef HAVE_CONFIG_H
#  include "mp3_config.h"
# endif

# include <stdlib.h>

# include "mp3_bit.h"
# include "mp3_stream.h"
# include "mp3_frame.h"
# include "mp3_layer12.h"
# include "mp3_layer3.h"


static uint32 const bitrate_table[5][15] = {
  /* MPEG-1 */
  { 0,  32000,  64000,  96000, 128000, 160000, 192000, 224000,  /* Layer I   */
       256000, 288000, 320000, 352000, 384000, 416000, 448000 },
  { 0,  32000,  48000,  56000,  64000,  80000,  96000, 112000,  /* Layer II  */
       128000, 160000, 192000, 224000, 256000, 320000, 384000 },
  { 0,  32000,  40000,  48000,  56000,  64000,  80000,  96000,  /* Layer III */
       112000, 128000, 160000, 192000, 224000, 256000, 320000 },

  /* MPEG-2 LSF */
  { 0,  32000,  48000,  56000,  64000,  80000,  96000, 112000,  /* Layer I   */
       128000, 144000, 160000, 176000, 192000, 224000, 256000 },
  { 0,   8000,  16000,  24000,  32000,  40000,  48000,  56000,  /* Layers    */
        64000,  80000,  96000, 112000, 128000, 144000, 160000 } /* II & III  */
};

static uint32 const samplerate_table[3] = { 44100, 48000, 32000 };

static int32 (*const decoder_table[3])(MP3_STREAM_T *, MP3_FRAME_T *) = {
  MP3_DEC_LayerI,
  MP3_DEC_LayerII,
  MP3_DEC_LayerIII
};

/*****************************************************************/
// 	Description : header structure initialization
//	Global resource dependence :
//  Author: 
//	Note:
/*****************************************************************/
static void MP3_DEC_HeaderInit(MP3_HEADER_T *header)
{
  header->layer          = 0;
  header->mode           = 0;
  header->mode_extension = 0;
  header->emphasis       = 0;

  header->bitrate        = 0;
  header->samplerate     = 0;

  header->crc_check      = 0;
  header->crc_target     = 0;

  header->flags          = 0;
  header->private_bits   = 0;
}



/*****************************************************************/
// 	Description : mute all subband values
//	Global resource dependence :
//  Author: 
//	Note:
/*****************************************************************/
static void MP3_DEC_FrameMute(MP3_FRAME_T *frame)
{
  unsigned int s, sb;

  for (s = 0; s < 36; ++s) {
    for (sb = 0; sb < 32; ++sb) {
      frame->sbsample[0][s][sb] =
      frame->sbsample[1][s][sb] = 0;
    }
  }

 {
    for (s = 0; s < 9/*18*/; ++s) {
      for (sb = 0; sb < 32; ++sb) {
	frame->overlap[0][sb][s] =
	frame->overlap[1][sb][s] = 0;
      }
    }
  }
}

/*****************************************************************/
// 	Description : frame structure init
//	Global resource dependence :
//  Author: 
//	Note:
/*****************************************************************/
void MP3_DEC_FrameInit(MP3_FRAME_T *frame_ptr)
{
  MP3_DEC_HeaderInit(&frame_ptr->header);

  frame_ptr->options = 0;
  //frame->overlap = 0;
  frame_ptr->sblimit_prev[0] = 0;
  frame_ptr->sblimit_prev[1] = 0;
  MP3_DEC_FrameMute(frame_ptr);
}

/*****************************************************************/
// 	Description : header parsing
//	Global resource dependence :
//  Author: 
//	Note:
/*****************************************************************/
static int16 MP3_DEC_DecodeHeader(MP3_HEADER_T *header_ptr, uint8 *in_buf)
{
	uint16 index1;	
	uint16 sfreq;
	uint8  tmp_stream0 = in_buf[1];	
	
	/* header_ptr() */
	if (!(in_buf[0] == 0xff && (in_buf[1] & 0xe0) == 0xe0))
	{
		return -1;
	}
	
	/* layer */
	header_ptr->layer = 4 - ((tmp_stream0>>1)&0x3) ;
	if(header_ptr->layer == 4) /*lint !e650*/
		return -1;
	
	/* mode */
	header_ptr->mode = 3 - ((in_buf[3]>>6)&0x3);
	header_ptr->num_ch = (header_ptr->mode)?2:1;
	
	/* mode_extension */
	header_ptr->mode_extension = ((in_buf[3]>>4)&0x3);	  
	
	/* sampling_frequency */
	index1 = ((in_buf[2]>>2)&0x3);
	if(index1 == 3)
		return -1;
	header_ptr->samplerate = samplerate_table[index1];
	
	header_ptr->flags       = 0;
	header_ptr->header_len  = 4; 
	
	
	/* bitrate_index */	  
	if (!((in_buf[2]>>4)&0xf))	  
		header_ptr->flags    |= MP3_FLAG_FREEFORMAT;	  
	
	/* protection_bit */
	if ((tmp_stream0&0x1)  == 0) 
	{
		MP3_DEC_BIT_POOL_T ptr;
		header_ptr->flags    |= MP3_FLAG_PROTECTION;
		header_ptr->header_len = 6;
		header_ptr->crc_target = ((uint16)in_buf[4]<<8)|(in_buf[5]);//tmp_stream2;
		
		//mp3_bit_init(&ptr, &in_buf[1],0);	 
		ptr.byte_ptr = &in_buf[2];
		ptr.cache = 0;
		ptr.left = 8;
		header_ptr->crc_check = MP3_DEC_BIT_CRCCheck(ptr, 16, 0xffff);
	}
	

	if (((tmp_stream0>>3)&0x1)  == 0) 
	{
		
		header_ptr->flags |= MP3_FLAG_LSF_EXT;	 
		header_ptr->num_gr = 1;		
		header_ptr->si_len = (header_ptr->num_ch == 1)?9:17;
		header_ptr->samplerate >>= 1;
		
		if (((tmp_stream0>>4)&0x1) == 0)
		{
			header_ptr->flags |= MP3_FLAG_MPEG_2_5_EXT;
			header_ptr->samplerate >>= 1;
		}
	}
	else
	{
		header_ptr->num_gr = 2;
		header_ptr->si_len = (header_ptr->num_ch == 1)?17:32;	  	
	}
	
	
	{
		sfreq = header_ptr->samplerate;
		if (header_ptr->flags & MP3_FLAG_MPEG_2_5_EXT)
			sfreq *= 2;
		
		header_ptr->sfreqi = (uint16)(((sfreq >>  7) & 0x000f) +
			((sfreq >> 15) & 0x0001) - 8);
		
		if (header_ptr->flags & MP3_FLAG_MPEG_2_5_EXT)
			header_ptr->sfreqi += 3;		
	}
		
	return 0;
}


int32 MP3_DEC_FrameDecode(uint8 const *bufin_ptr, MP3_FRAME_T *frame, MP3_STREAM_T *stream)
{
	MP3_HEADER_T *header = &frame->header;
	uint16 length;
	uint8 *in_buf = (uint8 *)bufin_ptr;
	frame->options = stream->options;
	
	/* header() */
	/* error_check() */
	if (MP3_DEC_DecodeHeader(header, (uint8 *)bufin_ptr) == -1)
		goto fail;
	
	length =  header->frame_len- header->header_len;
	if (length < header->si_len)
	{
		stream->error = MP3_ERROR_BADFRAMELEN;
		stream->md_len = 0;
		goto fail;
	}
	
	in_buf += header->header_len;
	stream->sync = 1;  
	stream->this_frame = in_buf;
	stream->next_frame = in_buf + length;//stream->bufend;	
	stream->ptr.byte_ptr = in_buf;
	stream->ptr.cache = 0;
	stream->ptr.left = 8;

	if (decoder_table[frame->header.layer - 1](stream, frame) == -1) 
	{		
		goto fail;
	}
	
	return 0;
	
fail:
	return -1;
}

