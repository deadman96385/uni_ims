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
#include "tiger_video_header.h"
/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    extern   "C" 
    {
#endif

DEC_SPS_T	*g_sps_ptr;
DEC_PPS_T	*g_pps_ptr;
DEC_SPS_T	*g_active_sps_ptr;
DEC_PPS_T	*g_active_pps_ptr;
DEC_SPS_T	*g_sps_array_ptr;
DEC_PPS_T	*g_pps_array_ptr;

DEC_IMAGE_PARAMS_T		*g_image_ptr;
DEC_SLICE_T				*g_curr_slice_ptr;
DEC_OLD_SLICE_PARAMS_T	*g_old_slice_ptr;
DEC_MB_CACHE_T			*g_mb_cache_ptr;
DEC_DECODED_PICTURE_BUFFER_T	*g_dpb_ptr;

DEC_STORABLE_PICTURE_T	*g_dec_picture_ptr;
DEC_STORABLE_PICTURE_T	*g_list[2][MAX_REF_FRAME_NUMBER+1];
DEC_DEC_REF_PIC_MARKING_T	g_dec_ref_pic_marking_buffer[DEC_REF_PIC_MARKING_COMMAND_NUM];
DEC_STORABLE_PICTURE_T	*g_no_reference_picture_ptr;

DEC_NALU_T	*g_nalu_ptr;

int32 g_wp_weight[2][MAX_REF_FRAME_NUMBER][3];  // weight in [list][index][component] order
int32 g_wp_offset[2][MAX_REF_FRAME_NUMBER][3];  // offset in [list][index][component] order
int32 g_wbp_weight[2][MAX_REF_FRAME_NUMBER][MAX_REF_FRAME_NUMBER][3]; //weight in [list][fw_index][bw_index][component] order
int32 g_list1_map_list0[MAX_REF_FRAME_NUMBER];

int8	*g_MbToSliceGroupMap;
int32	g_old_pps_id;
int32	g_list_size[2];
int32	g_dec_ref_pic_marking_buffer_size;
int32	g_ready_to_decode_slice;
int32	g_is_avc1_es;
int32	g_searching_IDR_pic;
int32	g_nFrame_dec_h264;
int32 g_firstBsm_init_h264;/*BSM init*/
int32	g_stream_offset;

uint8 g_lengthSizeMinusOne;

int32 last_dquant;

const uint8 *g_totZero_Chroma_DC [3];
const uint8 * g_run_zeroLeft [6];

nal_startcode_follows_func nal_startcode_follows;
readRefFrame_func readRefFrame;
readMVD_xy_func readMVD_xy;
direct_mv_func direct_mv;
pred_skip_bslice_func pred_skip_bslice;
MC8x8_direct_func MC8x8_direct;
decode_mb_coeff_func sw_vld_mb;
read_mb_type_func read_mb_type_ISlice;
read_mb_type_func read_mb_type_PBSlice;
readB8_typeInfo_func read_b8mode;
BS_and_Para_interMB_func BS_and_Para_interMB_hor;
BS_and_Para_interMB_func BS_and_Para_interMB_ver;
readMB_typeInfo_CABAC_func readMB_typeInfo_CABAC;

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End 
