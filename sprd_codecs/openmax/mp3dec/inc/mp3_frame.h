/********************************************************************************
**  File Name: 	mp3_frame.h						                                *
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


# ifndef LIBMP3_FRAME_H
# define LIBMP3_FRAME_H

# include "mp3_fixed.h"
# include "mp3_stream.h"


#define  MP3_LAYER_I    1			/* Layer I */
#define  MP3_LAYER_II   2			/* Layer II */
#define  MP3_LAYER_III  3			/* Layer III */
#define  MP3_MODE_SINGLE_CHANNEL 0		/* single channel */
#define  MP3_MODE_DUAL_CHANNEL	1		/* dual channel */

#define  MP3_MODE_JOINT_STEREO	        2		/* joint (MS/intensity) stereo */
#define  MP3_MODE_STEREO	                3		/* normal LR stereo */
#define  MP3_EMPHASIS_NONE	           0		/* no emphasis */
#define  MP3_EMPHASIS_50_15_US	   1		/* 50/15 microseconds emphasis */
#define  MP3_EMPHASIS_CCITT_J_17  3		/* CCITT J.17 emphasis */
#define  MP3_EMPHASIS_RESERVED   2		/* unknown emphasis */


typedef struct _Mp3Header {
  int32 layer;			/* audio layer (1, 2, or 3) */
  int32 mode;			/* channel mode (see above) */
  int32 mode_extension;			/* additional mode info */
  int32 emphasis;		/* de-emphasis to use (see above) */

  uint32 bitrate;		/* stream bitrate (bps) */
  uint32 samplerate;		/* sampling frequency (Hz) */

  uint16 crc_check;		/* frame CRC accumulator */
  uint16 crc_target;		/* final target CRC checksum */

  uint32 flags;				/* flags (see below) */
  uint32 private_bits;			/* private bits (see below) */

  uint32 sfreqi;

  uint16 frame_len;
  uint16 next_md_begin;
  uint16 header_len;
  uint16 si_len;
  uint16 num_ch;
  uint16 num_gr;
}MP3_HEADER_T;

typedef struct _Mp3Frame {
  MP3_HEADER_T header;		/* MPEG audio header */
  int32 options;				/* decoding options (from stream) */
  
  mp3_fixed_t sbsample[2][36][32];	/* synthesis subband filter samples */
  mp3_fixed_t overlap[2][32][9/*18*/];
  
  int32 sblimit_prev[2];
  mp3_fixed_t *xr[2];
  uint32 allocation[2][32];
  uint32 scfsi[2][32];
  uint32 scalefactor[2][32][3];
}MP3_FRAME_T;

# define MP3_NCHANNELS(header)		((header)->mode ? 2 : 1)
# define MP3_NSBSAMPLES(header)  \
  ((header)->layer == MP3_LAYER_I ? 12 :  \
   (((header)->layer == MP3_LAYER_III &&  \
     ((header)->flags & MP3_FLAG_LSF_EXT)) ? 18 : 36))

#define  MP3_FLAG_NPRIVATE_III	  0x0007	/* number of Layer III private bits */
#define  MP3_FLAG_INCOMPLETE	  0x0008	/* header but not data is decoded */
#define  MP3_FLAG_PROTECTION	  0x0010	/* frame has CRC protection */

#define  MP3_FLAG_COPYRIGHT	  0x0020	/* frame is copyright */
#define  MP3_FLAG_ORIGINAL	          0x0040	/* frame is original (else copy) */
#define  MP3_FLAG_PADDING	          0x0080	/* frame has additional slot */
#define  MP3_FLAG_I_STEREO	          0x0100	/* uses intensity joint stereo */

#define  MP3_FLAG_MS_STEREO	  0x0200	/* uses middle/side joint stereo */
#define  MP3_FLAG_FREEFORMAT	  0x0400	/* uses free format bitrate */
#define  MP3_FLAG_LSF_EXT	          0x1000	/* lower sampling freq. extension */

#define  MP3_FLAG_MC_EXT	          0x2000	/* multichannel audio extension */
#define  MP3_FLAG_MPEG_2_5_EXT	  0x4000	/* MPEG 2.5 (unofficial) extension */
#define  MP3_PRIVATE_HEADER	 0x0100	/* header private bit */
#define  MP3_PRIVATE_III	                 0x001f	/* Layer III private bits (up to 5) */

#define	MP3_OPTION_IGNORECRC              0x0001	/* ignore CRC errors */
#define     MP3_OPTION_HALFSAMPLERATE  0x0002	/* generate PCM at 1/2 sample rate */

# if 0  /* not yet implemented */
		MP3_OPTION_LEFTCHANNEL    = 0x0010,	/* decode left channel only */
		MP3_OPTION_RIGHTCHANNEL   = 0x0020,	/* decode right channel only */
		MP3_OPTION_SINGLECHANNEL  = 0x0030	/* combine channels */
# endif

void MP3_DEC_FrameInit(MP3_FRAME_T *);
int32 MP3_DEC_FrameDecode(uint8 const *bufin_ptr, MP3_FRAME_T *frame, MP3_STREAM_T *stream);


# endif





















