/********************************************************************************
** File Name:      mp3_layer3.h                                          *
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

# ifndef LIBMAD_LAYER3_H
# define LIBMAD_LAYER3_H

# include "mp3_stream.h"
# include "mp3_frame.h"
# include "mp3_synth.h"

int32 MP3_DEC_LayerIII(MP3_STREAM_T *, MP3_FRAME_T *);

# endif
