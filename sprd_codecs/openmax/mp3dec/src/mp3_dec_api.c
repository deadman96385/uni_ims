/********************************************************************************
**  File Name: 	mp3_dec_api.c									                *
**  Author:		Tan Li      									                *
**  Date:		17/01/2011                                                      *
**  Copyright:	2011 Spreadtrum, Incorporated. All Rights Reserved.		        *
**  Description:  api function implemenation       						        *
*********************************************************************************
*********************************************************************************
**  Edit History											                    *
**------------------------------------------------------------------------------*
**  DATE			NAME			DESCRIPTION				                    *
**  17/01/2011		Tan li    		Create. 				                    *
*********************************************************************************/


// mp3_decoder.c
#include <stdio.h>
#ifdef WIN32
#include <memory.h>
#endif
#include "mp3_bit.h"
#include "mp3_decoder.h"
#include "mp3_dec_api.h"

//static MP3_DECODER_T	g_decoder;
//static mp3_fixed_t Xr_buf[2][576];

int32 MP3_ARM_DEC_Construct(void **h_decoder_ptr)
{
	MP3_DECODER_T *h_decoder;
	h_decoder = (MP3_DECODER_T *)malloc(sizeof(MP3_DECODER_T));
	memset(h_decoder, 0, sizeof(MP3_DECODER_T));
	*h_decoder_ptr = h_decoder;
	return 0;
}

int32 MP3_ARM_DEC_Deconstruct(void const **h_decoder_ptr)
{
	free(*h_decoder_ptr);
	*h_decoder_ptr = NULL;	
	return 0;
}


/*****************************************************************/
// 	Description : Init decoder structure
//	Global resource dependence : g_decoder
//   Author: Tan. Li
//	Note:
/*****************************************************************/
void MP3_ARM_DEC_InitDecoder(void *decoder_ptr)
{
	MP3_DECODER_T *h_decoder = (MP3_DECODER_T *)decoder_ptr;
    MP3_DEC_StreamInit(&h_decoder->stream);
    MP3_DEC_FrameInit(&h_decoder->frame);
    MP3_DEC_SynthInit(&h_decoder->synth);    
	h_decoder->stream.options = 1;
	return;
}

/*****************************************************************/
// 	Description : frame decoder process
//	Global resource dependence : g_decoder
//   Author: Tan. Li
//	Note:
/*****************************************************************/

void MP3_ARM_DEC_DecodeFrame(
							 void *decoder_ptr,
							 FRAME_DEC_T *frame_dec_buf_ptr,  // [Input]
							 OUTPUT_FRAME_T *output_frame_ptr, // [Output]
							 uint32 *decode_result	// [Output]
						 )
{
	MP3_DECODER_T *g_decoder = (MP3_DECODER_T *)decoder_ptr;
	
	if(PNULL == decode_result)
	{
		goto fail;
	}

	/*input param protection*/
	if ((PNULL==decoder_ptr)||(PNULL == frame_dec_buf_ptr)||(PNULL == output_frame_ptr))
	{
		*decode_result = MP3_ARM_DEC_ERROR_INPUT_PARAM;
		goto fail;
	}
	
	if ((PNULL == frame_dec_buf_ptr->frame_buf_ptr))
	{
		*decode_result = MP3_ARM_DEC_ERROR_INPUT_BUFPTR;
		goto fail;
	}
	
	if((PNULL == output_frame_ptr->pcm_data_l_ptr)||(PNULL == output_frame_ptr->pcm_data_r_ptr))
	{
		*decode_result = MP3_ARM_DEC_ERROR_OUTPUT_BUFPTR;
		goto fail;
		
	}
	
	/* demux interface protection*/
	if ((frame_dec_buf_ptr->frame_len <= 4)||(frame_dec_buf_ptr->frame_len > 1440))
	{
		*decode_result = MP3_ARM_DEC_ERROR_FRAMELEN;
		goto fail;
	}

	if (frame_dec_buf_ptr->next_begin > 511)
	{
		*decode_result = MP3_ARM_DEC_ERROR_NEXT_BEGIN;
		goto fail;
	}
	
	if (frame_dec_buf_ptr->bitrate > 448)
	{
		*decode_result = MP3_ARM_DEC_ERROR_BITRATE;
		goto fail;
	}

	g_decoder->frame.header.bitrate = frame_dec_buf_ptr->bitrate*1000;
	g_decoder->frame.header.frame_len = frame_dec_buf_ptr->frame_len;
	g_decoder->frame.header.next_md_begin = frame_dec_buf_ptr->next_begin;	
	output_frame_ptr->pcm_bytes = 0;
	g_decoder->synth.left_data_out=(int16 *)output_frame_ptr->pcm_data_l_ptr;
	g_decoder->synth.right_data_out=(int16 *)output_frame_ptr->pcm_data_r_ptr;
	g_decoder->frame.xr[0]= (mp3_fixed_t *)output_frame_ptr->pcm_data_l_ptr;
	g_decoder->frame.xr[1]= (mp3_fixed_t *)output_frame_ptr->pcm_data_r_ptr;

		
	if (MP3_DEC_FrameDecode(frame_dec_buf_ptr->frame_buf_ptr, &g_decoder->frame, &g_decoder->stream) == 0)
	{		
		*decode_result = MP3_ARM_DEC_ERROR_NONE;
		goto synth;
	}
	else 
	{
		*decode_result = MP3_ARM_DEC_ERROR_DECODING;
		goto fail;
	}
	
synth:	
	MP3_DEC_SynthFrame(&g_decoder->synth, &g_decoder->frame);	

	if (g_decoder->synth.channels == 1)
	{
		memcpy(g_decoder->synth.right_data_out, g_decoder->synth.left_data_out, g_decoder->synth.length*2);
	}	

	output_frame_ptr->channel_num = g_decoder->synth.channels;
	output_frame_ptr->pcm_bytes = g_decoder->synth.length;
	
fail:		
	return;
}