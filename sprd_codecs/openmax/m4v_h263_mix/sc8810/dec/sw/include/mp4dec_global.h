/******************************************************************************
 ** File Name:      mp4dec_global.h                                           *
 ** Author:         Xiaowei Luo                                               *
 ** DATE:           01/23/2007                                                *
 ** Copyright:      2006 Spreatrum, Incoporated. All Rights Reserved.         *
 ** Description:    This file defines the basic interfaces of mp4 codec       *
 *****************************************************************************/
/******************************************************************************
 **                   Edit    History                                         *
 **---------------------------------------------------------------------------*
 ** DATE          NAME            DESCRIPTION                                 *
 ** 01/23/2007    Xiaowei Luo     Create.                                     *
 *****************************************************************************/
#ifndef _MP4DEC_GLOBAL_H_
#define _MP4DEC_GLOBAL_H_
/*----------------------------------------------------------------------------*
**                        Dependencies                                        *
**---------------------------------------------------------------------------*/
#include "mp4dec_basic.h"
#include "mp4dec_mode.h"
#include "mpeg4dec.h"
#include "mp4dec_mc.h"

#if defined(SIM_IN_WIN)
#include <stdio.h>
#endif //SIM_IN_WIN

/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
extern   "C"
{
#endif

extern const uint8 g_dc_scaler_table_y[32];
extern const uint8 g_dc_scaler_table_c[32];
extern const uint8 g_ClipTab[1024];
extern const int32 g_MvRound4[4];
extern const int32 g_MvRound16[16];

extern const int8  g_dec_dq_tab[4];
extern const MCBPC_TABLE_CODE_LEN_T g_dec_mcbpc_tab_intra[];
extern const MCBPC_TABLE_CODE_LEN_T g_dec_mcbpc_tab[];
extern const CBPY_TABLE_CODE_LEN_T g_dec_cbpy_tab[];
extern const MV_TABLE_CODE_LEN_T g_dec_tmn_mv_tab0[];
extern const MV_TABLE_CODE_LEN_T g_dec_tmn_mv_tab1[];
extern const MV_TABLE_CODE_LEN_T g_dec_tmn_mv_tab2[];
extern const uint8 g_intra_max_level[128];
extern const uint8 g_inter_max_level[128];
extern const uint8 g_intra_max_run [44];
extern const uint8 g_inter_max_run[20];
extern const VLC_TABLE_CODE_LEN_T g_dec_dct3d_tab3[];
extern const VLC_TABLE_CODE_LEN_T g_dec_dct3d_tab4[];
extern const VLC_TABLE_CODE_LEN_T g_dec_dct3d_tab5[];
extern const VLC_TABLE_CODE_LEN_T g_dec_dct3d_tab0[];
extern const VLC_TABLE_CODE_LEN_T g_dec_dct3d_tab1[];
extern const VLC_TABLE_CODE_LEN_T g_dec_dct3d_tab2[];
extern const uint32 g_mp4_dec_huff_tbl[152];
extern const uint32 g_rvlc_huff_tab[146];

//common
extern const uint8 ASIC_DCT_Matrix[64];
extern const uint8 g_horizontal_scan[BLOCK_SQUARE_SIZE];
extern const uint8 g_vertical_scan[BLOCK_SQUARE_SIZE];
extern const uint8 g_standard_zigzag_normal[BLOCK_SQUARE_SIZE];
extern const uint8 g_standard_zigzag_trans[BLOCK_SQUARE_SIZE];
extern const uint8 g_default_intra_qmatrix_normal[BLOCK_SQUARE_SIZE];
extern const uint8 g_default_intra_qmatrix_trans[BLOCK_SQUARE_SIZE];
extern const uint8 g_default_inter_qmatrix_normal[BLOCK_SQUARE_SIZE];
extern const uint8 g_default_inter_qmatrix_trans[BLOCK_SQUARE_SIZE];

//for data partition
//extern int32 **g_dec_dc_store;
#ifdef _MP4CODEC_DATA_PARTITION_
extern const RVLC_TABLE_CODE_LEN_T g_dec_rvlc_dct3d_tab_intra[];
extern const RVLC_TABLE_CODE_LEN_T g_dec_rvlc_dct3d_tab_inter[];

//for dec rvlc
extern const uint16 g_dec_rvlc_code_tab_intra[];
extern const uint16 g_dec_rvlc_index_tab_intra[];
extern const uint16 g_dec_rvlc_code_tab_inter[];
extern const uint16 g_dec_rvlc_index_tab_inter[];
#endif  //DATA_PARTITION

const int8 *Mp4Dec_GetDqTable(void);

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
}
#endif
/**---------------------------------------------------------------------------*/
// End
#endif  //_MP4DEC_GLOBAL_H_
