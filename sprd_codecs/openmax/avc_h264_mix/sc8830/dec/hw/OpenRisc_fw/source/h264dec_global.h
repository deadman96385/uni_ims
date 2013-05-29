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

extern DEC_STORABLE_PICTURE_T g_rec_buf;
extern DEC_SPS_T	*g_sps_ptr;
extern DEC_PPS_T	*g_pps_ptr;
extern DEC_SPS_T	*g_active_sps_ptr;
extern DEC_PPS_T	*g_active_pps_ptr;
extern DEC_SPS_T	*g_sps_array_ptr;
extern DEC_PPS_T	*g_pps_array_ptr;

extern uint32 direct_mb_info_addr[MAX_REF_FRAME_NUMBER+1];

#if _MVC_
#define MAXSPS  32
extern subset_seq_parameter_set_rbsp_t *g_active_subset_sps;
  //int svc_extension_flag;
extern subset_seq_parameter_set_rbsp_t g_SubsetSeqParSet[MAXSPS];
extern int8 last_pic_width_in_mbs_minus1;
extern int8 last_pic_height_in_map_units_minus1;
extern int8 last_max_dec_frame_buffering;
extern uint8 last_profile_idc;
#endif

extern DEC_IMAGE_PARAMS_T		*g_image_ptr;
extern DEC_SLICE_T				*g_curr_slice_ptr;
extern DEC_OLD_SLICE_PARAMS_T	*g_old_slice_ptr;
extern DEC_MB_CACHE_T			*g_mb_cache_ptr;
#if (!_MVC_)
extern DEC_DECODED_PICTURE_BUFFER_T	*g_dpb_ptr;
#else
extern DEC_DECODED_PICTURE_BUFFER_T	 *g_dpb_layer[2];
#endif

extern DEC_STORABLE_PICTURE_T	*g_dec_picture_ptr;
extern DEC_STORABLE_PICTURE_T	*g_list0[MAX_REF_FRAME_NUMBER+MAX_REF_FRAME_NUMBER];//weihu//+1
extern DEC_STORABLE_PICTURE_T	*g_list1[MAX_REF_FRAME_NUMBER+1];
extern DEC_DEC_REF_PIC_MARKING_T	g_dec_ref_pic_marking_buffer[DEC_REF_PIC_MARKING_COMMAND_NUM];
extern DEC_STORABLE_PICTURE_T	*g_no_reference_picture_ptr;
extern DEC_NALU_T	*g_nalu_ptr;
extern int8	*g_MbToSliceGroupMap;

extern int32	g_old_pps_id;
extern int32	g_list_size[2];
extern int32	g_dec_ref_pic_marking_buffer_size;
extern int32	g_feed_flag_h264;
extern int32	g_ready_to_decode_slice;
extern int32	g_searching_IDR_pic;
extern int32	g_pre_mb_is_intra4;	//????

extern int32	g_nFrame_dec_h264;

extern unsigned char *display_array_Y[16];
extern unsigned char *display_array_UV[16];
extern int32	display_array_len;
extern int32 g_dispFrmNum;

extern int32	g_firstBsm_init_h264;/*BSM init*/

extern int8		g_ICBP_TBL[];
extern uint8	g_QP_SCALER_CR_TBL[];
extern uint8	g_cbp_intra_tbl [48];
extern uint8	g_cbp_inter_tbl [48];
extern uint8	g_cbp_intra_tbl_400 [16];
extern uint8	g_cbp_inter_tbl_400 [16];
extern uint8	g_qpPerRem_tbl [52][2] ;
extern uint8 g_blk_order_map_tbl[16+2 * 4];
extern uint32 g_huff_tab_token [69];

extern int32 g_wp_weight[2][MAX_REF_FRAME_NUMBER][3];  // weight in [list][index][component] order
extern int32 g_wp_offset[2][MAX_REF_FRAME_NUMBER][3];  // offset in [list][index][component] order
extern int32 g_wbp_weight[2][MAX_REF_FRAME_NUMBER][MAX_REF_FRAME_NUMBER][3]; //weight in [list][fw_index][bw_index][component] order
extern int32 g_list1_map_list0[MAX_REF_FRAME_NUMBER];
extern int8  g_list0_map_addr[32];//weihu
extern int8  g_list1_map_addr[16];//weihu

//#if _CMODEL_
extern volatile int32	g_stream_offset;
extern volatile int32    g_slice_datalen;//weihu
extern int8	load_vld_table_en;
//#endif

extern char weightscale4x4[6][4][4];//6 6*2+2=14*16=224B
extern char weightscale8x8[2][8][8];//2 2*2+2=6*64=384B

extern char weightscale4x4_intra_default[16];
extern char weightscale4x4_inter_default[16];
extern char weightscale8x8_intra_default[64];
extern char weightscale8x8_inter_default[64];

extern uint8	g_lengthSizeMinusOne;

extern nal_startcode_follows_func nal_startcode_follows;
extern readRefFrame_func readRefFrame;
extern readMVD_xy_func readMVD_xy;
extern direct_mv_func direct_mv;
extern pred_skip_bslice_func pred_skip_bslice;
extern MC8x8_direct_func MC8x8_direct;

//cabac
extern int32 last_dquant;

extern const int32 INIT_MB_TYPE_I[1][3][11][2];
extern const int32 INIT_MB_TYPE_P[3][3][11][2];
extern const int32 INIT_B8_TYPE_I[1][2][9][2];
extern const int32 INIT_B8_TYPE_P[3][2][9][2];
extern const int32 INIT_MV_RES_I[1][2][10][2];
extern const int32 INIT_MV_RES_P[3][2][10][2];
extern const int32 INIT_REF_NO_I[1][2][6][2];
extern const int32 INIT_REF_NO_P[3][2][6][2];
extern const int32 INIT_DELTA_QP_I[1][1][4][2];
extern const int32 INIT_DELTA_QP_P[3][1][4][2];
extern const int32 INIT_MB_AFF_I[1][1][4][2];
extern const int32 INIT_MB_AFF_P[3][1][4][2];
extern const int32 INIT_TRANSFORM_SIZE_I[1][1][3][2];
extern const int32 INIT_TRANSFORM_SIZE_P[3][1][3][2];
extern const int32 INIT_IPR_I[1][1][2][2];
extern const int32 INIT_IPR_P[3][1][2][2];
extern const int32 INIT_CIPR_I[1][1][4][2];
extern const int32 INIT_CIPR_P[3][1][4][2];
extern const int32 INIT_CBP_I[1][3][4][2];
extern const int32 INIT_CBP_P[3][3][4][2];
extern const int32 INIT_BCBP_I[1][8][4][2];
extern const int32 INIT_BCBP_P[3][8][4][2];
extern const int32 INIT_MAP_I[1][8][15][2];
extern const int32 INIT_MAP_P[3][8][15][2];
extern const int32 INIT_LAST_I[1][8][15][2];
extern const int32 INIT_LAST_P[3][8][15][2];
extern const int32 INIT_ONE_I[1][8][5][2];
extern const int32 INIT_ONE_P[3][8][5][2];
extern const int32 INIT_ABS_I[1][8][5][2];
extern const int32 INIT_ABS_P[3][8][5][2];
extern const int32 INIT_FLD_MAP_I[1][8][15][2];
extern const int32 INIT_FLD_MAP_P[3][8][15][2];
extern const int32 INIT_FLD_LAST_I[1][8][15][2];
extern const int32 INIT_FLD_LAST_P[3][8][15][2];

extern uint8	g_block_order_map_tbl[16+2 * 4];

extern uint16 g_incVlc_tbl[7];
extern int8 g_total_zero_len_tbl [16];
extern int8 g_run_before_len_tbl [16];

/*tailing one and total coefficient vlc table*/
/*0<=nC<2*/
extern const uint16 g_vlc0_0_tbl [8];
extern const uint16 g_vlc0_1_tbl [32];
extern const uint16 g_vlc0_2_tbl [32];
extern const uint16 g_vlc0_3_tbl [128];

/*2<=nC<4*/
extern const uint16 g_vlc1_0_tbl [64];
extern const uint16 g_vlc1_1_tbl [32];
extern const uint16 g_vlc1_2_tbl [32];
extern const uint16 g_vlc1_3_tbl [32];

/*4<=nC<8*/
extern const uint16 g_vlc2_0_tbl [32];
extern const uint16 g_vlc2_1_tbl [32];
extern const uint16 g_vlc2_2_tbl [32];
extern const uint16 g_vlc2_3_tbl [16];

/*8<=nC*/
extern const uint16 g_vlc3_0_tbl [64];

/*chroma dc coeff_token table*/
extern const uint16 g_coeffToken_chroma_0_tbl [8];
extern const uint16 g_coeffToken_chroma_1_tbl [32];



/*total zero coefficient table, for 4x4 DCT*/
extern const uint8 g_totZero1_0_tbl [32];
extern const uint8 g_totZero1_1_tbl [32];
extern const uint8 g_totZero2_0_tbl [16];
extern const uint8 g_totZero2_1_tbl [8];
extern const uint8 g_totZero3_0_tbl [16];
extern const uint8 g_totZero3_1_tbl [8];
extern const uint8 g_totZero4_0_tbl [32];
extern const uint8 g_totZero5_0_tbl [32];
extern const uint8 g_totZero6_0_tbl [16];
extern const uint8 g_totZero6_1_tbl [4];
extern const uint8 g_totZero7_0_tbl [16];
extern const uint8 g_totZero7_1_tbl [4];
extern const uint8 g_totZero8_0_tbl [16];
extern const uint8 g_totZero8_1_tbl [4];
extern const uint8 g_totZero9_0_tbl [16];
extern const uint8 g_totZero9_1_tbl [4];
extern const uint8 g_totZero10_0_tbl [8];
extern const uint8 g_totZero10_1_tbl [4];
extern const uint8 g_totZero11_0_tbl [16];
extern const uint8 g_totZero12_0_tbl [16];
extern const uint8 g_totZero13_0_tbl [8];
extern const uint8 g_totZero14_0_tbl [4];
extern const uint8 g_totZero15_0_tbl [2];

extern const uint8 g_totZero_Chroma_DC1_tbl [8];
extern const uint8 g_totZero_Chroma_DC2_tbl [4];
extern const uint8 g_totZero_Chroma_DC3_tbl [2];

/*run before table*/
extern const uint8 g_run_zeroLeft1_tbl [2];
extern const uint8 g_run_zeroLeft2_tbl [4];
extern const uint8 g_run_zeroLeft3_tbl [4];
extern const uint8 g_run_zeroLeft4_tbl [8];
extern const uint8 g_run_zeroLeft5_tbl [8];
extern const uint8 g_run_zeroLeft6_tbl [8];
extern const uint8 g_run_zeroLeftGt6_tbl [8];

extern const uint8 *g_totZero_Chroma_DC [3];
extern const uint8 * g_run_zeroLeft [6];

extern const int32 g_msk[33];

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End 
#endif  //_H264DEC_GLOBAL_H_
