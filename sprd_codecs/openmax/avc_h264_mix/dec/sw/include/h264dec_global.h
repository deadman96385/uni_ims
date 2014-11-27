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

extern const int8 g_ICBP_TBL[];
extern const uint8	g_QP_SCALER_CR_TBL[];
extern const uint8	g_cbp_intra_tbl [48];
extern const uint8	g_cbp_inter_tbl [48];
extern const uint16	g_qpPerRem_tbl [52] ;
extern const uint8 g_blk_order_map_tbl[16+2 * 4];

/************************************************************
					internal table
  ************************************************************/
extern const uint8 * g_rgiClipTab;
extern const int8 g_blkC_avaiable_tbl[16];
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

extern const uint8 g_inverse_zigzag_tbl[16];
extern const uint8 g_inverse_zigzag_cabac_I16_ac_tbl[15];
extern const uint8 g_inverse_8x8_zigzag_tbl[64];
extern const uint8 g_inverse_8x8_zigzag_tbl_cavlc[64];
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

extern const uint8 dequant4_coeff_init[6][3];
extern const uint8 dequant8_coeff_init_scan[16];
extern const uint8 dequant8_coeff_init[6][6];

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
extern const uint8 ff_h264_norm_shift[512];
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

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
}
#endif
/**---------------------------------------------------------------------------*/
// End
#endif  //_H264DEC_GLOBAL_H_
