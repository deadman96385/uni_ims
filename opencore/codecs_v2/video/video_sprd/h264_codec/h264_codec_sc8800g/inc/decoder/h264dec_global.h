/******************************************************************************
** File Name:      h264dec_global.h                                           *
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
#ifndef _H264DEC_GLOBAL_H_
#define _H264DEC_GLOBAL_H_

/*----------------------------------------------------------------------------*
**                        Dependencies                                        *
**---------------------------------------------------------------------------*/
//#include "sci_types.h"
#include "h264dec_mode.h"
#include "h264dec_buffer.h"
/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
extern   "C" 
{
#endif

extern DEC_SPS_T	*g_sps_ptr;
extern DEC_PPS_T	*g_pps_ptr;
extern DEC_SPS_T	*g_active_sps_ptr;
extern DEC_PPS_T	*g_active_pps_ptr;
extern DEC_SPS_T	*g_sps_array_ptr;
extern DEC_PPS_T	*g_pps_array_ptr;

extern DEC_IMAGE_PARAMS_T		*g_image_ptr;
extern DEC_SLICE_T				*g_curr_slice_ptr;
extern DEC_OLD_SLICE_PARAMS_T	*g_old_slice_ptr;
extern DEC_MB_CACHE_T			*g_mb_cache_ptr;
extern DEC_DECODED_PICTURE_BUFFER_T	*g_dpb_ptr;

extern DEC_STORABLE_PICTURE_T	*g_dec_picture_ptr;
extern DEC_STORABLE_PICTURE_T	*g_list0[MAX_REF_FRAME_NUMBER+1];
extern DEC_DEC_REF_PIC_MARKING_T	g_dec_ref_pic_marking_buffer[DEC_REF_PIC_MARKING_COMMAND_NUM];
extern DEC_STORABLE_PICTURE_T	*g_no_reference_picture_ptr;
extern DEC_NALU_T	*g_nalu_ptr;
extern int8	*g_MbToSliceGroupMap;

extern int32	g_old_pps_id;
extern int32	g_list_size;
extern int32	g_dec_ref_pic_marking_buffer_size;
extern int32	g_feed_flag_h264;
extern int32	g_ready_to_decode_slice;
extern int32	g_is_avc1_es;
extern int32	g_error;
extern int32	g_searching_IDR_pic;
extern int32	g_pre_mb_is_intra4;	//????

extern int32	g_nFrame_dec_h264;

extern int32	g_firstBsm_init_h264;/*BSM init*/

extern int8		g_ICBP_TBL[];
extern uint8	g_QP_SCALER_CR_TBL[];
extern uint8	g_cbp_intra_tbl [48];
extern uint8	g_cbp_inter_tbl [48];
extern uint8	g_qpPerRem_tbl [52][2] ;
extern uint8 g_blk_order_map_tbl[16+2 * 4];
extern uint32 g_huff_tab_token [69];

//#if _CMODEL_
extern int32	g_stream_offset;
//#endif

extern uint8	g_lengthSizeMinusOne;

#ifdef _VSP_LINUX_
#include "h264dec.h"
extern DEC_STORABLE_PICTURE_T g_rec_buf;
extern FunctionType_BufCB VSP_bindCb;
extern FunctionType_BufCB VSP_unbindCb;
extern void *g_user_data;
extern FunctionType_SPS VSP_spsCb;
#endif

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End 
#endif  //_H264DEC_GLOBAL_H_
