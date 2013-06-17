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

	int8	is_output;
	int8	slice_type;
	int8	idr_flag;
	int8	no_output_of_prior_pics_flag;

	int32	non_existing;

	int8	adaptive_ref_pic_buffering_flag;
#if _MVC_
	int8    view_id;
	int8    inter_view_flag;
	int8    anchor_pic_flag;
#endif

	DEC_DEC_REF_PIC_MARKING_T *dec_ref_pic_marking_buffer; //<! stores the memory management control operations

	uint8	*imgY;		//should be 64 word alignment
	uint8	*imgU;
	uint8	*imgV;
    int32   *direct_mb_info;
	uint32	imgYAddr;	//frame address which are configured to VSP,  imgYAddr = ((uint32)imgY >> 8), 64 word aligment
	uint32	imgUAddr;	//imgUAddr = ((uint32)imgU>>8)
	uint32	imgVAddr;	//imgVAddr = ((uint32)imgV>>8)
	uint32  direct_mb_info_Addr;

	
	int32   DPB_addr_index;//weihu

	void *pBufferHeader;
        int32       mPicId;  // Which output picture is for which input buffer?

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

#if _MVC_
	int8       view_id;
	int8       inter_view_flag[2];
	int8       anchor_pic_flag[2];
    int8       layer_id;
#endif
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
#if _MVC_
	DEC_FRAME_STORE_T	**fs_ilref; // inter-layer reference (for multi-layered codecs)
	int8				last_output_view_id;
	int8				init_done;
	int8                resv;
	int8                resv1;
#endif
}DEC_DECODED_PICTURE_BUFFER_T;

PUBLIC void H264Dec_init_img_buffer (DEC_IMAGE_PARAMS_T *img_ptr);
PUBLIC void	H264Dec_init_dpb (DEC_IMAGE_PARAMS_T *img_ptr, DEC_DECODED_PICTURE_BUFFER_T *dpb_ptr, int type);
PUBLIC void H264Dec_store_picture_in_dpb (DEC_IMAGE_PARAMS_T *img_ptr, DEC_STORABLE_PICTURE_T *picture_ptr, DEC_DECODED_PICTURE_BUFFER_T *dpb_ptr);
PUBLIC void H264Dec_reorder_list (void);
PUBLIC void H264Dec_reorder_list_mvc (void);
PUBLIC void H264Dec_init_list (DEC_IMAGE_PARAMS_T *img_ptr, int32 curr_slice_type);
PUBLIC void H264Dec_flush_dpb ();
	
/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End 
#endif  //_H264DEC_BUFFER_H_
