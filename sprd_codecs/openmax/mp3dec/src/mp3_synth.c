/********************************************************************************
** File Name:      mp3_synth.c                                           *
**  Author:		Tan Li      									                *
**  Date:		17/01/2011                                                      *
**  Copyright:	2011 Spreadtrum, Incorporated. All Rights Reserved.		        *
**  Description:                                  						        *
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
# include "mp3_fixed.h"
# include "mp3_frame.h"
# include "mp3_synth.h"


#define GETSAMPLE16(s, d)		\
s = (short)(d);


/*
* NAME:	synth->init()
* DESCRIPTION:	initialize synth struct
*/
void MP3_DEC_SynthInit(MP3_SYNTH_T *synth_ptr)
{
	MP3_DEC_SynthMute(synth_ptr);
	
	synth_ptr->phase = 0;
	
	synth_ptr->samplerate = 0;
	synth_ptr->channels   = 0;
	synth_ptr->length     = 0;
}

/*
* NAME:	synth->mute()
* DESCRIPTION:	zero all polyphase filterbank values, resetting synthesis
*/
void MP3_DEC_SynthMute(MP3_SYNTH_T *synth)
{
	unsigned int ch, s, v;
	
	for (ch = 0; ch < 2; ++ch) {
		for (s = 0; s < 16; ++s) {
			for (v = 0; v < 16; ++v) {
				synth->filter[ch][0][0][s][v] = synth->filter[ch][0][1][s][v] =
					synth->filter[ch][1][0][s][v] = synth->filter[ch][1][1][s][v] = 0;
			}
		}
	}
}

/*
* An optional optimization called here the Subband Synthesis Optimization
* (SSO) improves the performance of subband synthesis at the expense of
* accuracy.
*
* The idea is to simplify 32x32->64-bit multiplication to 32x32->32 such
* that extra scaling and rounding are not necessary. This often allows the
* compiler to use faster 32-bit multiply-accumulate instructions instead of
* explicit 64-bit multiply, shift, and add instructions.
*
* SSO works like this: a full 32x32->64-bit multiply of two mp3_fixed_t
* values requires the result to be right-shifted 28 bits to be properly
* scaled to the same fixed-point format. Right shifts can be applied at any
* time to either operand or to the result, so the optimization involves
* careful placement of these shifts to minimize the loss of accuracy.
*
* First, a 14-bit shift is applied with rounding at compile-time to the D[]
* table of coefficients for the subband synthesis window. This only loses 2
* bits of accuracy because the lower 12 bits are always zero. A second
* 12-bit shift occurs after the DCT calculation. This loses 12 bits of
* accuracy. Finally, a third 2-bit shift occurs just before the sample is
* saved in the PCM buffer. 14 + 12 + 2 == 28 bits.
*/

/* FPM_DEFAULT without OPT_SSO will actually lose accuracy and performance */
#include "mp3_synth.h"

# if defined(FPM_DEFAULT) && !defined(OPT_SSO)
#  define OPT_SSO
# endif


/*
* NAME:	dct32()
* DESCRIPTION:	perform fast in[32]->out[32] DCT
*/
#define OPT_DCTO

# if !defined(ASO_DCT32)
#ifndef __ASM_OPT__


#else


#endif



static 
mp3_fixed_t const costab1[17]=
{
0x7fd8878e,  /*1*/  /*0*/
0x647e645f,  /*2*/  /*1*/
0x7f62368f,  /*3*/  /*2*/
//0,			 /*4*/  /*-*/
0x7d3b4b20,  /*5*/  /*3*/
//0,           /*6*/  /*-*/
0x7b15676c,  /*7*/  /*4*/
0x63e345e4,  /*8*/  /*5*/
0x78547c68,  /*9*/  /*6*/
//0,           /*10*/  /*-*/
0x74fa4d9f,  /*11*/  /*7*/
0x4a5061ff,  /*12*/  /*8*/
0x7109563e,  /*13*/  /*9*/
//0,           /*14*/  /*-*/
0x6c8361f8,  /*15*/  /*10*/
//0,			 /*16*/  /*-*/
//0,			 /*17*/  /*-*/
0x6d7444cf,  /*18*/  /*11*/
0x61c671cf,  /*19*/  /*12*/
0x78ad5b94,  /*20*/  /*13*/
//0,           /*21*/  /*-*/
0x738854db,  /*22*/  /*14*/
//0,           /*23*/
//0,           /*24*/
//0,           /*25*/
//0,           /*26*/
//0,           /*27*/
//0,           /*28*/
0x7b5e57d7,  /*29*/  /*15*/
//0,           /*30*/
0x6a0a       /*31*/  /*16*/
};

#define costab1_0    0x7fd8878e 
#define costab1_1    0x647e645f 
#define costab1_2    0x7f62368f 
#define costab1_3    0x7d3b4b20 
#define costab1_4    0x7b15676c 
#define costab1_5    0x63e345e4 
#define costab1_6    0x78547c68 
#define costab1_7    0x74fa4d9f 
#define costab1_8    0x4a5061ff 
#define costab1_9    0x7109563e 
#define costab1_10    0x6c8361f8
#define costab1_11    0x6d7444cf
#define costab1_12    0x61c671cf
#define costab1_13    0x78ad5b94
#define costab1_14    0x738854db
#define costab1_15    0x7b5e57d7
#define costab1_16    0x6a0a    




#endif






/* third SSO shift and/or D[] optimization preshift */
#define SYNTH_REORG

# if defined(OPT_SSO)
#  if MP3_F_FRACBITS != 28
#   error "MP3_F_FRACBITS must be 28 to use OPT_SSO"
#  endif



#  define MLZ(hi, lo)		((void) (hi), (mp3_fixed_t) (lo))

#  define PRESHIFT(x)		((MP3_F(x) + (1L << 13)) >> 14)

# elif defined(__ASM_OPT__)

#ifndef  SYNTH_REORG





#   define PRESHIFT(x)	(MP3_F(x)<<2)//((MP3_F(x)+(1L<<13))>>14)
#else





#   define PRESHIFT(x)	(MP3_F(x)+(1L<<11) >> 12)
#endif


#else






#   undef  MP3_F_SCALEBITS
#   define MP3_F_SCALEBITS	(MP3_F_FRACBITS - 12)
#   define PRESHIFT(x)		(MP3_F(x)+(1L<<11) >> 12)

# endif


#ifdef SYNTH_REORG
#define PRESHIFT0(x)		(MP3_F(x)-0x10000000+(1L<<11) >> 12)//(MP3_F(x)-0x8000000+(1L<<11) >> 12)
#define PRESHIFT1(x)		(MP3_F(x)-0x10000000+(1L<<11) >> 12)
#endif

#ifndef SYNTH_REORG
static
mp3_fixed_t const D[17][32] = {
# include "D.dat"
};
#else
static
mp3_fixed_t const D[16][8] = {
# include "D_reorg.dat"
};

#endif

/*
* NAME:	mp3_synth_frame()
* DESCRIPTION:	perform full frequency PCM synthesis
*/
# if defined(ASO_SYNTHFILTER)
void MP3_DEC_SynthFilter(mp3_fixed_t const (*sbsample)[36][32],int32 ns,short *pcm1,mp3_fixed_t (*filter)[2][2][16][16],  unsigned int phase);
# else
void MP3_DEC_SynthFilter(mp3_fixed_t const (*sbsample)[36][32],int32 ns,short *pcm1,mp3_fixed_t (*filter)[2][2][16][16],  unsigned int phase)
{
	int32 s;
	for (s = 0; s < ns; ++s) 
	{
		mp3_fixed_t *fdct32_lo;
		register mp3_fixed_t (*fe)[16], (*fx)[16];
		register int t0, t1;
		
		fdct32_lo = &(*filter)[0][phase & 1][0][0];		
		t0 = phase>>1;
		fe = &fdct32_lo[t0];
# if defined(ASO_DCT32)	
		t1 =(((phase - 1)>>1) & 0x7);						
		fx = &(*filter)[0][~phase & 1][0][t1];			
		dct32((*sbsample)[s], fe+47);	
		synth_full(fe, fx, &pcm1/*, nch*/);	
#else
		dct32((*sbsample)[s], /*t0*/0, &fdct32_lo[t0], &fdct32_lo[t0]+512);			
		t1 =(((phase - 1)>>1) & 0x7);						
		fx = &(*filter)[0][~phase & 1][0][t1];	
		synth_full(fe, fx, &pcm1/*, nch*/);	
#endif		
		phase = (phase + 1) % 16;
	}
}
#endif


void MP3_DEC_SynthFrame(MP3_SYNTH_T *synth, MP3_FRAME_T *frame)					
{
	int32 ch;	
	int32 ns = MP3_NSBSAMPLES(&frame->header);	
	int32 nch= MP3_NCHANNELS(&frame->header);	
	unsigned int phase = synth->phase;
	synth->phase = (synth->phase + (uint32)ns) &0x0f;
	for (ch = 0; ch < nch; ++ch)
	{
		short *pcm1;
		register mp3_fixed_t (*filter)[2][2][16][16];
		register mp3_fixed_t const (*sbsample)[36][32];

		sbsample = (const int32 (*)[36][32])&frame->sbsample[ch][0][0];
		filter   = &synth->filter[ch];
		//phase    = synth->phase;
#ifndef INTERLEAVE_PCM
		pcm1     =  (ch==0)?(short *)synth->left_data_out : (short *)synth->right_data_out;			
#else	
		pcm1     =  (ch==0)?(short *)synth->left_data_out : (short *)synth->left_data_out+1;			
#endif

	MP3_DEC_SynthFilter(sbsample, ns, pcm1, filter, phase);

  }

  synth->channels   = (uint16)nch;//MP3_NCHANNELS(&frame->header);
  synth->length     = (uint16)(ns<<5);//MP3_NSBSAMPLES(&frame->header)*32;	
  synth->samplerate = frame->header.samplerate;
     
}
























