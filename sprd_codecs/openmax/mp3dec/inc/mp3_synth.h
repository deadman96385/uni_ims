/********************************************************************************
** File Name:      mp3_synth.h                                           *
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


# ifndef LIBMAD_SYNTH_H
# define LIBMAD_SYNTH_H

# include "mp3_fixed.h"
# include "mp3_frame.h"

#define SYNTH_FILTER16


typedef struct _Mp3Synth 
{
#ifndef SYNTH_FILTER16
  mp3_fixed_t filter[2][2][2][16][8];	/* polyphase filterbank outputs */
  					/* [ch][eo][peo][s][v] */
#else
  mp3_fixed_t filter[2][2][2][16][16];	/* polyphase filterbank outputs */
  /* [ch][eo][peo][s][v] */
#endif
  
  // writing data to the buffer outside
  int16* 	left_data_out;
  int16* 	right_data_out;

  uint32 phase;			/* current processing phase */

  //struct mp3_pcm pcm;			/* PCM output */
  uint32 samplerate;		/* sampling frequency (Hz) */
  uint16 channels;		/* number of channels */
  uint16 length;		/* number of samples per channel */

  // player options
  uint32 options;
}MP3_SYNTH_T;

/* single channel PCM selector */
#define  MAD_PCM_CHANNEL_SINGLE   0

/* dual channel PCM selector */
#define  MAD_PCM_CHANNEL_DUAL_1  0
#define  MAD_PCM_CHANNEL_DUAL_2  1

/* stereo PCM selector */
#define  MAD_PCM_CHANNEL_STEREO_LEFT     0
#define  MAD_PCM_CHANNEL_STEREO_RIGHT  1

void MP3_DEC_SynthInit(MP3_SYNTH_T *);

# define MP3_DEC_SynthFinish(synth)  /* nothing */

void MP3_DEC_SynthMute(MP3_SYNTH_T *);

void MP3_DEC_SynthFrame(MP3_SYNTH_T *synth, MP3_FRAME_T *frame);
				

# endif
