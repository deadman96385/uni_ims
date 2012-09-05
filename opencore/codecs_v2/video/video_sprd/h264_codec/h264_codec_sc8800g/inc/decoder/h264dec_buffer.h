/******************************************************************************
** File Name:      h264dec_buffer.h                                            *
** Author:         Xiaowei Luo                                               *
** DATE:           12/06/2007                                                *
** Copyright:      2007 Spreatrum, Incoporated. All Rights Reserved.         *
** Description:    common define for video codec.	     			          *
*****************************************************************************/
/******************************************************************************
**                   Edit    History                                         *
**---------------------------------------------------------------------------* 
** DATE          NAME            DESCRIPTION                                 * 
** 11/20/2007    Xiaowei Luo     Create.                                     *
*****************************************************************************/
#ifndef _H264DEC_BUFFER_H_
#define _H264DEC_BUFFER_H_

/*----------------------------------------------------------------------------*
**                        Dependencies                                        *
**---------------------------------------------------------------------------*/
#include "video_common.h"
/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
extern   "C" 
{
#endif

typedef struct storable_picture
{
	int32	poc;
	int32	frame_poc;
	int32	frame_num;
	int32	pic_num;
	int32	long_term_pic_num;
	int32	long_term_frame_idx;
	
	int8	is_long_term;
	int8	used_for_reference;
	int8	chroma_vector_adjustment;
	int8	coded_frame;

	int8	slice_type;
	int8	idr_flag;
	int8	no_output_of_prior_pics_flag;
	int8	adaptive_ref_pic_buffering_flag;

	int32	non_existing;

	DEC_DEC_REF_PIC_MARKING_T *dec_ref_pic_marking_buffer; //<! stores the memory management control operations

	uint8	*imgY;		//should be 64 word alignment
	uint8	*imgU;
	uint8	*imgV;

	uint32	imgYAddr;	//frame address which are configured to VSP,  imgYAddr = ((uint32)imgY >> 8), 64 word aligment
	uint32	imgUAddr;	//imgUAddr = ((uint32)imgU>>8)
	uint32	imgVAddr;	//imgVAddr = ((uint32)imgV>>8)

	int32	mc_ref_pic_num;
#ifdef _VSP_LINUX_					
	void *pBufferHeader;
#endif		

}DEC_STORABLE_PICTURE_T;

typedef struct frame_store_tag
{
	int8	is_reference;
	int8	is_long_term;
	int8	is_short_term;
	int8	disp_status;

	int32	pic_num;
	int32	frame_num;
	int32	frame_num_wrap;
	int32	long_term_frame_idx;
	int32	poc;

	DEC_STORABLE_PICTURE_T *frame;
}DEC_FRAME_STORE_T;


//decoded picture buffer
typedef struct decoded_picture_buffer
{
	DEC_FRAME_STORE_T	**fs;
	int32				size;
	int32				used_size;

	//ref buffer
	DEC_FRAME_STORE_T	**fs_ref;
	int32				ref_frames_in_buffer;

	DEC_FRAME_STORE_T	**fs_ltref;
	int32				ltref_frames_in_buffer;

	int32				max_long_term_pic_idx;
	int32				num_ref_frames;
}DEC_DECODED_PICTURE_BUFFER_T;



PUBLIC void H264Dec_init_img_buffer (DEC_IMAGE_PARAMS_T *img_ptr);
PUBLIC void	H264Dec_init_dpb (DEC_IMAGE_PARAMS_T *img_ptr);
PUBLIC void H264Dec_store_picture_in_dpb (DEC_IMAGE_PARAMS_T *img_ptr, DEC_STORABLE_PICTURE_T *picture_ptr);
PUBLIC void H264Dec_reorder_list (void);
PUBLIC void H264Dec_init_list (DEC_IMAGE_PARAMS_T *img_ptr, int32 curr_slice_type);
	
/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End 
#endif  //_H264DEC_BUFFER_H_