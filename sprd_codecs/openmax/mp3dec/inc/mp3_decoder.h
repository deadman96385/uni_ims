/*************************************************************************
** File Name:      mp3_decoder.h                                         *
** Date:           05/11/2010                                            *
** Copyright:      2010 Spreatrum, Incoporated. All Rights Reserved.     *
** Description:    This file is used to define decoder structure         *
**                 
**                        Edit History                                   *
** ----------------------------------------------------------------------*
**************************************************************************/

#ifndef _DECODER_H
#define _DECODER_H

#include "mp3_stream.h"
#include "mp3_frame.h"
#include "mp3_synth.h"

typedef struct _Mp3Decoder {
	MP3_STREAM_T 	stream;
    MP3_FRAME_T		frame;
    MP3_SYNTH_T 	synth;
} MP3_DECODER_T;

#endif