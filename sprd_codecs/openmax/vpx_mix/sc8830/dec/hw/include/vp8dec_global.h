/******************************************************************************
** File Name:      vp8dec_global.h                                           *
** Author:         Xiaowei Luo                                               *
** DATE:           07/04/2013                                                *
** Copyright:      2013 Spreatrum, Incoporated. All Rights Reserved.         *
** Description:    common define for video codec.	     			          *
*****************************************************************************/
/******************************************************************************
**                   Edit    History                                         *
**---------------------------------------------------------------------------*
** DATE          NAME            DESCRIPTION                                 *
** 11/20/2007    Xiaowei Luo     Create.                                     *
*****************************************************************************/
#ifndef _VP8DEC_GLOBAL_H_
#define _VP8DEC_GLOBAL_H_
/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
extern   "C"
{
#endif

#include "vp8dec_mode.h"

extern const int dc_qlookup[QINDEX_RANGE];
extern const int ac_qlookup[QINDEX_RANGE];

extern const int vp8_mb_feature_data_bits[MB_LVL_MAX];
extern const vp8_prob vp8_coef_update_probs [BLOCK_TYPES] [COEF_BANDS] [PREV_COEF_CONTEXTS] [vp8_coef_tokens-1];
extern const vp8_prob default_coef_probs [BLOCK_TYPES][COEF_BANDS][PREV_COEF_CONTEXTS][ENTROPY_NODES];
extern const vp8_prob vp8_ymode_prob[VP8_YMODES-1];
extern const vp8_prob vp8_kf_ymode_prob[VP8_YMODES-1];
extern const vp8_prob vp8_uv_mode_prob[VP8_UV_MODES-1];
extern const vp8_prob vp8_kf_uv_mode_prob[VP8_UV_MODES-1];
extern const vp8_prob vp8_bmode_prob[VP8_BINTRAMODES-1];
extern const vp8_prob vp8_kf_bmode_prob[VP8_BINTRAMODES] [VP8_BINTRAMODES] [VP8_BINTRAMODES-1];
extern const vp8_prob vp8_sub_mv_ref_prob2 [SUBMVREF_COUNT][VP8_SUBMVREFS-1];
extern const vp8_prob vp8_mbsplit_probs [VP8_NUMMBSPLITS-1];
extern const int vp8_mode_contexts[6][4];
extern const MV_CONTEXT vp8_mv_update_probs[2], vp8_default_mv_context[2];
extern const vp8_prob vp8_mbsplit_probs [VP8_NUMMBSPLITS-1];
extern const vp8_prob vp8_sub_mv_ref_prob2 [SUBMVREF_COUNT][VP8_SUBMVREFS-1];

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
}
#endif
/**---------------------------------------------------------------------------*/
// End
#endif //VP8DEC_GLOBAL_H

