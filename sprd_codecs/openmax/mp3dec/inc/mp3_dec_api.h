
/********************************************************************************
**  File Name: 	imdct.c											                *
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


#ifndef _MP3_API_H
#define	_MP3_API_H

#include "t_types.h"

#ifdef   __cplusplus
extern   "C" 
{
#endif

	
#define	MP3_ARM_DEC_ERROR_NONE			  0x0000	/* no error */
#define	MP3_ARM_DEC_ERROR_DECODING		  0x0001	
#define	MP3_ARM_DEC_ERROR_INPUT_PARAM          0x0002	
#define	MP3_ARM_DEC_ERROR_INPUT_BUFPTR	  0x0003
#define	MP3_ARM_DEC_ERROR_OUTPUT_BUFPTR	  0x0004
#define	MP3_ARM_DEC_ERROR_FRAMELEN		  0x0005
#define	MP3_ARM_DEC_ERROR_NEXT_BEGIN	          0x0006
#define	MP3_ARM_DEC_ERROR_BITRATE		          0x0007
	


typedef struct _FrameDec  //Input param
{
	uint8 * frame_buf_ptr;  // one audio frame buffer start address
	uint16 frame_len;   // one audio frame length
	uint16 next_begin;
	uint16 bitrate;
}FRAME_DEC_T;

typedef struct _OutputFrame  //Output param
{
	uint16* pcm_data_l_ptr;  // left channel pcm data
	uint16* pcm_data_r_ptr;	  // right channel pcm data
	uint16 pcm_bytes;  // frame sample counts
	uint16 channel_num; // channel number	
}OUTPUT_FRAME_T;
	
int32 MP3_ARM_DEC_Construct(void **h_decoder_ptr);
int32 MP3_ARM_DEC_Deconstruct(void const **h_decoder_ptr);
void MP3_ARM_DEC_InitDecoder(void *);
void MP3_ARM_DEC_DecodeFrame( 
							 void *,
						FRAME_DEC_T *frame_dec_buf_ptr,  // [Input]
						 OUTPUT_FRAME_T *output_frame_ptr, // [Output]
						 uint32 *decode_result	// [Output]
						 );

#ifdef   __cplusplus
}
#endif

#endif  // _MP3_API_H

