/******************************************************************************
 ** File Name:    h264dec_global.c                                            *
 ** Author:       Xiaowei.Luo                                                 *
 ** DATE:         03/29/2010                                                  *
 ** Copyright:    2010 Spreatrum, Incoporated. All Rights Reserved.           *
 ** Description:                                                              *
 *****************************************************************************/
/******************************************************************************
 **                   Edit    History                                         *
 **---------------------------------------------------------------------------* 
 ** DATE          NAME            DESCRIPTION                                 * 
 ** 03/29/2010    Xiaowei.Luo     Create.                                     *
 *****************************************************************************/
/*----------------------------------------------------------------------------*
**                        Dependencies                                        *
**---------------------------------------------------------------------------*/
#include "sc8800g_video_header.h"
/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    extern   "C" 
    {
#endif

DEC_IMAGE_PARAMS_T		*g_image_ptr;
DEC_SLICE_T				*g_curr_slice_ptr;
DEC_OLD_SLICE_PARAMS_T	*g_old_slice_ptr;
DEC_MB_CACHE_T			*g_mb_cache_ptr;
DEC_DECODED_PICTURE_BUFFER_T	*g_dpb_ptr;

DEC_STORABLE_PICTURE_T	*g_dec_picture_ptr;
DEC_STORABLE_PICTURE_T	*g_list0[MAX_REF_FRAME_NUMBER+1];
DEC_DEC_REF_PIC_MARKING_T	g_dec_ref_pic_marking_buffer[DEC_REF_PIC_MARKING_COMMAND_NUM];
DEC_STORABLE_PICTURE_T	*g_no_reference_picture_ptr;

DEC_SPS_T	*g_sps_ptr;
DEC_PPS_T	*g_pps_ptr;
DEC_SPS_T	*g_active_sps_ptr;
DEC_PPS_T	*g_active_pps_ptr;
DEC_SPS_T	*g_sps_array_ptr;
DEC_PPS_T	*g_pps_array_ptr;
DEC_NALU_T	*g_nalu_ptr;

int8	*g_MbToSliceGroupMap;
int32	g_old_pps_id;
int32	g_list_size;
int32	g_dec_ref_pic_marking_buffer_size;
int32	g_ready_to_decode_slice;
int32	g_is_avc1_es;
int32	g_searching_IDR_pic;
int32	g_pre_mb_is_intra4;	
int32	g_nFrame_dec_h264;
int32 g_firstBsm_init_h264;/*BSM init*/
int32	g_stream_offset;

uint8 g_lengthSizeMinusOne;

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End 
