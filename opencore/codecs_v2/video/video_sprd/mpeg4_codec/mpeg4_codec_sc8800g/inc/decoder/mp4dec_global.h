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
#include "mpeg4dec.h"

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

extern uint8 *stream_buf_ptr;
extern uint32 read_byteshh;

/*BSM init*/
extern int32 g_firstBsm_init;
extern DEC_VOP_MODE_T *g_dec_vop_mode_ptr;
extern DEC_FRM_BFR g_FrmYUVBfr[DEC_YUV_BUFFER_NUM];
extern DEC_FRM_BFR g_DispFrmYUVBfr[DISP_YUV_BUFFER_NUM];
extern BOOLEAN g_dec_is_first_frame;
extern BOOLEAN g_dec_is_stop_decode_vol;
extern BOOLEAN g_dec_is_changed_format;
extern VOP_PRED_TYPE_E g_dec_pre_vop_format;
extern const int8  g_dec_dq_tab[4];
extern const MCBPC_TABLE_CODE_LEN_T g_dec_mcbpc_tab_intra[];
extern const MCBPC_TABLE_CODE_LEN_T g_dec_mcbpc_tab[];
extern const CBPY_TABLE_CODE_LEN_T g_dec_cbpy_tab[];
extern const MV_TABLE_CODE_LEN_T g_dec_tmn_mv_tab0[];
extern const MV_TABLE_CODE_LEN_T g_dec_tmn_mv_tab1[];
extern const MV_TABLE_CODE_LEN_T g_dec_tmn_mv_tab2[];
extern const VLC_TABLE_CODE_LEN_T g_dec_dct3d_tab0[];
extern const VLC_TABLE_CODE_LEN_T g_dec_dct3d_tab1[];
extern const VLC_TABLE_CODE_LEN_T g_dec_dct3d_tab2[];
extern uint32 g_mp4_dec_huff_tbl[152];
extern uint32 g_rvlc_huff_tab[146];
extern uint32 g_nFrame_dec;

//common
extern const uint8 ASIC_DCT_Matrix[64];
extern const uint8 g_standard_zigzag[BLOCK_SQUARE_SIZE];
extern const uint8 g_default_intra_qmatrix[BLOCK_SQUARE_SIZE];
extern const uint8 g_default_inter_qmatrix[BLOCK_SQUARE_SIZE];
//for data partition
extern int32 **g_dec_dc_store;

DEC_VOP_MODE_T *Mp4Dec_GetVopmode(void);
void Mp4Dec_SetVopmode(DEC_VOP_MODE_T *vop_mode_ptr);
H263_PLUS_HEAD_INFO_T *Mp4Dec_GetH263PlusHeadInfo(void);
void Mp4Dec_SetH263PlusHeadInfo(H263_PLUS_HEAD_INFO_T *h263_plus_head_info_ptr);
int32 **Mp4Dec_GetDcStore(void);
void Mp4Dec_SetDcStore(int32 **dec_dc_store_pptr);
const int8 *Mp4Dec_GetDqTable(void);

#if _CMODEL_
extern int32 g_stream_offset;
#endif //_CMODEL_
extern uint32 g_dispFrmNum;

#if _DEBUG_
void foo(void);
#endif //_DEBUG_

#if defined(SIM_IN_WIN32)
extern double  g_psnr[3];
extern uint8 * g_src_frame_ptr; 
#endif

#ifdef _VSP_LINUX_
extern DEC_FRM_BFR g_rec_buf;
extern FunctionType_BufCB VSP_bindCb;
extern FunctionType_BufCB VSP_unbindCb;
extern void *g_user_data;
#endif
/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End 
#endif  //_MP4DEC_GLOBAL_H_
