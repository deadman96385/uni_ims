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
#include "h264dec_bitstream.h"
#include "h264dec_mc.h"
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
extern DEC_STORABLE_PICTURE_T	*g_list[2][MAX_REF_FRAME_NUMBER+1];
extern DEC_DEC_REF_PIC_MARKING_T	g_dec_ref_pic_marking_buffer[DEC_REF_PIC_MARKING_COMMAND_NUM];
extern DEC_STORABLE_PICTURE_T	*g_no_reference_picture_ptr;
extern DEC_NALU_T	*g_nalu_ptr;
extern int8	*g_MbToSliceGroupMap;

extern int32	g_old_pps_id;
extern int32	g_list_size[2];
extern int32	g_dec_ref_pic_marking_buffer_size;
extern int32	g_feed_flag_h264;
extern int32	g_ready_to_decode_slice;
extern int32	g_is_avc1_es;
extern int32	g_searching_IDR_pic;

extern int32	g_nFrame_dec_h264;

extern int32	g_firstBsm_init_h264;/*BSM init*/

extern uint32 *g_cmd_data_ptr;
extern uint32 *g_cmd_info_ptr;
extern uint32 *g_cmd_data_base;
extern uint32 *g_cmd_info_base;

extern const int8 g_ICBP_TBL[];
extern const uint8	g_QP_SCALER_CR_TBL[];
extern const uint8	g_cbp_intra_tbl [48];
extern const uint8	g_cbp_inter_tbl [48];
extern const uint16	g_qpPerRem_tbl [52] ;
extern const uint8 g_blk_order_map_tbl[16+2 * 4];
extern const uint32 g_huff_tab_token [69];

extern int16 g_wp_weight[2][MAX_REF_FRAME_NUMBER][3];  // weight in [list][index][component] order
extern int16 g_wp_offset[2][MAX_REF_FRAME_NUMBER][3];  // offset in [list][index][component] order
extern int16 g_wbp_weight[2][MAX_REF_FRAME_NUMBER][MAX_REF_FRAME_NUMBER][3]; //weight in [list][fw_index][bw_index][component] order
extern int32 g_list1_map_list0[MAX_REF_FRAME_NUMBER];

//#if _CMODEL_
extern int32	g_stream_offset;
//#endif

extern uint8	g_lengthSizeMinusOne;
extern nal_startcode_follows_func nal_startcode_follows;
extern readRefFrame_func readRefFrame;
extern decode_mvd_xy_func decode_mvd_xy;
extern direct_mv_func direct_mv;
extern pred_skip_bslice_func pred_skip_bslice;
extern MC8x8_direct_func MC8x8_direct;
extern decode_mb_coeff_func sw_vld_mb;
extern read_mb_type_func read_mb_type;
extern readB8_typeInfo_func read_b8mode;
extern BS_and_Para_interMB_func BS_and_Para_interMB_hor;
extern BS_and_Para_interMB_func BS_and_Para_interMB_ver;
extern readMB_typeInfo_CABAC_func readMB_typeInfo_CABAC;

/************************************************************
					internal table 
  ************************************************************/

//cabac
extern int32 last_dquant;

//extern uint8 g_ClipTab_264 [CLIP_TAB_SIZE];
extern const uint8 * g_rgiClipTab;
extern const int8 g_blkC_avaiable_tbl[16];
extern int32 g_refPosx,g_refPosy;
extern const uint8 g_blkScanOdrToCacheOdrMap [16];
extern const uint8 ALPHA_TABLE[52*3];
extern const uint8  BETA_TABLE[52*3];
extern const uint8 CLIP_TAB[52*3][5];
extern const uint8 *g_totZero_Chroma_DC [3];
extern const uint8 * g_run_zeroLeft [6];
extern const uint8 g_b8_offset[4];
extern const uint8 g_b4_offset[4];
extern const uint8 g_b8map[16];
extern const uint32 g_msk[33];

extern const int8 g_inverse_zigzag_tbl[16];
extern const int8 g_inverse_zigzag_cabac_I16_ac_tbl[15];
extern const int8 g_inverse_8x8_zigzag_tbl[64];
extern const int8 g_inverse_8x8_zigzag_tbl_cavlc[64];
extern const uint8 g_clip_tbl[1024+256];
extern const uint8 * g_rgiClipTab; 
extern const uint8 g_blKIndex [4*4];
extern const uint8 g_mbcache_addr_map_tbl [16+4];
extern const uint8 g_dec_order_to_scan_order_map_tbl [16];
extern const uint8 weightscale4x4_intra_default[16];
extern const uint8 weightscale4x4_inter_default[16];
extern const uint8 weightscale8x8_intra_default[64];
extern const uint8 weightscale8x8_inter_default[64];

extern MC4xN_LUMA g_MC4xN_luma[16]; 
extern MC8xN_LUMA g_MC8xN_luma[16];
extern MC16xN_LUMA g_MC16xN_luma[16];
extern Intra4x4Pred g_intraPred4x4[9];
extern Intra8x8Pred g_intraPred8x8[9];
extern Intra16x16Pred g_intraPred16x16[4];
extern IntraChromPred g_intraChromaPred[4];

extern MC_chroma8xN g_MC_chroma8xN;
extern MC_chroma4xN g_MC_chroma4xN;
extern MC_chroma2xN g_MC_chroma2xN;


extern int16  *g_halfPixTemp;// [24*16]; //wxh

extern const uint8 dequant4_coeff_init[6][3];
extern const uint8 dequant8_coeff_init_scan[16];
extern const uint8 dequant8_coeff_init[6][6];

//mv
extern int32 s_list;
extern int32 s_dir;
extern int8 *s_ref_idx_ptr_array[2];
extern int8 *s_ref_idx_ptr;	
extern int32 *s_ref_mv_ptr_array[2];
extern int32 *s_ref_mv_ptr;
extern int32 *s_mvd_ptr;
extern int32 s_mvd_xy;
extern uint32 s_row;

#ifdef _VSP_LINUX_
#include "h264dec.h"
extern DEC_STORABLE_PICTURE_T g_rec_buf;
extern FunctionType_BufCB VSP_bindCb;
extern FunctionType_BufCB VSP_unbindCb;
extern void *g_user_data;
extern FunctionType_SPS VSP_spsCb;
extern FunctionType_FlushCacheCB VSP_fluchCacheCb ;
#endif

//cavlc
extern const int8 chroma_dc_coeff_token_vlc_tbl[256][2];
extern const int16 coeff_token_vlc_tbl_0[520][2];
extern const int16 coeff_token_vlc_tbl_1[332][2];
extern const int16 coeff_token_vlc_tbl_2[280][2];
extern const int16 coeff_token_vlc_tbl_3[256][2];
extern const int8 chroma_dc_total_zeros_vlc_tbl_0[8][2];
extern const int8 chroma_dc_total_zeros_vlc_tbl_1[8][2];
extern const int8 chroma_dc_total_zeros_vlc_tbl_2[8][2];
extern const int8 total_zeros_vlc_tbl_0[512][2];
extern const int8 total_zeros_vlc_tbl_1[512][2];
extern const int8 total_zeros_vlc_tbl_2[512][2];
extern const int8 total_zeros_vlc_tbl_3[512][2];
extern const int8 total_zeros_vlc_tbl_4[512][2];
extern const int8 total_zeros_vlc_tbl_5[512][2];
extern const int8 total_zeros_vlc_tbl_6[512][2];
extern const int8 total_zeros_vlc_tbl_7[512][2];
extern const int8 total_zeros_vlc_tbl_8[512][2];
extern const int8 total_zeros_vlc_tbl_9[512][2];
extern const int8 total_zeros_vlc_tbl_10[512][2];
extern const int8 total_zeros_vlc_tbl_11[512][2];
extern const int8 total_zeros_vlc_tbl_12[512][2];
extern const int8 total_zeros_vlc_tbl_13[512][2];
extern const int8 total_zeros_vlc_tbl_14[512][2];
extern const int8 run_vlc_tbl_0[8][2];
extern const int8 run_vlc_tbl_1[8][2];
extern const int8 run_vlc_tbl_2[8][2];
extern const int8 run_vlc_tbl_3[8][2];
extern const int8 run_vlc_tbl_4[8][2];
extern const int8 run_vlc_tbl_5[8][2];
extern const int8 run7_vlc_tbl[96][2];

//cabac
extern const int8 cabac_context_init_I[460][2];
extern const int8 cabac_context_init_PB[3][460][2];
extern const uint8 ff_h264_mlps_state[4*64];
extern const uint8 ff_h264_lps_range[4*2*64];  ///< rangeTabLPS
extern const uint8 ff_h264_mps_state[2*64];     ///< transIdxMPS

extern const int32 significant_coeff_flag_offset[6];
extern const int32 last_coeff_flag_offset[6];
extern const int32 coeff_abs_level_m1_offset[6];
extern const uint8 significant_coeff_flag_offset_8x8[63];
extern const uint8 last_coeff_flag_offset_8x8[63];
extern const  uint8 coeff_abs_level1_ctx[8];
extern const uint8 coeff_abs_levelgt1_ctx[8];
extern const uint8 coeff_abs_level_transition[2][8]; 

extern int32 g_need_back_last_word;
extern int32 g_back_last_word;
/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End 
#endif  //_H264DEC_GLOBAL_H_
