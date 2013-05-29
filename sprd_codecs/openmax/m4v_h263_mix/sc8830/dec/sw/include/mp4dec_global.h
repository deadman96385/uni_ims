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
#include "mp4_basic.h"
#include "mp4dec_mode.h"
//#include "mpeg4dec.h"
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
//extern MP4_MC_16x16 g_mp4dec_mc_16x16 [4];
//extern MP4_MC_8x8   g_mp4dec_mc_8x8 [4];
extern const uint8 g_ClipTab[1024];
extern const int32 g_MvRound4[4];
extern const int32 g_MvRound16[16];

//extern int32 g_firstBsm_init;/*BSM init*/
//extern DEC_VOP_MODE_T *g_dec_vop_mode_ptr;
//extern DEC_FRM_BFR g_FrmYUVBfr[DEC_YUV_BUFFER_NUM];
//extern DEC_FRM_BFR g_DispFrmYUVBfr[DISP_YUV_BUFFER_NUM];
//extern BOOLEAN g_dec_is_first_frame;
//extern BOOLEAN g_dec_is_stop_decode_vol;
//extern BOOLEAN g_dec_is_changed_format;
//extern VOP_PRED_TYPE_E g_dec_pre_vop_format;
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
//extern H263_PLUS_HEAD_INFO_T *g_h263_plus_head_info_ptr;
//extern uint32 g_nFrame_dec;
//extern uint32 g_dispFrmNum;
//extern GetIntraBlkTCoef_sw_func g_Mp4Dec_GetIntraBlkTCoef_sw;
//extern GetInterBlkTCoef_sw_func g_Mp4Dec_GetInterBlkTCoef_sw;
//extern GetIntraBlkTCoef_hw_func g_Mp4Dec_GetIntraBlkTCoef_hw;
//extern GetInterBlkTCoef_hw_func g_Mp4Dec_GetInterBlkTCoef_hw;
//extern DecIVOP_func g_Mp4Dec_IVOP;
//extern DecPVOP_func g_Mp4Dec_PVOP;
//extern DecBVOP_func g_Mp4Dec_BVOP;
//extern Dec_frm_level_sync_func g_Mp4Dec_Frm_Level_Sync;

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

//extern uint32 *g_cmd_data_ptr;
//extern uint32 *g_cmd_info_ptr;
//extern uint32 *g_cmd_data_base;
//extern uint32 *g_cmd_info_base;

//DEC_VOP_MODE_T *Mp4Dec_GetVopmode(void);
//void Mp4Dec_SetVopmode(DEC_VOP_MODE_T *vop_mode_ptr);
const int8 *Mp4Dec_GetDqTable(void);
//H263_PLUS_HEAD_INFO_T *Mp4Dec_GetH263PlusHeadInfo(void);
//void Mp4Dec_SetH263PlusHeadInfo(H263_PLUS_HEAD_INFO_T *h263_plus_head_info_ptr);
//int32 **Mp4Dec_GetDcStore(void);
//void Mp4Dec_SetDcStore(int32 **dc_store_pptr);

#if _CMODEL_
//extern int32 g_stream_offset;
#endif //_CMODEL_

#if _DEBUG_
void foo(void);
#endif //_DEBUG_

#if defined(SIM_IN_WIN32)
//extern double  g_psnr[3];
//extern uint8 * g_src_frame_ptr; 
#endif

//#ifdef _VSP_LINUX_
//extern DEC_FRM_BFR g_rec_buf;
//extern FunctionType_BufCB VSP_bindCb;
//extern FunctionType_BufCB VSP_unbindCb;
//extern void *g_user_data;
//#endif

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End 
#endif  //_MP4DEC_GLOBAL_H_
