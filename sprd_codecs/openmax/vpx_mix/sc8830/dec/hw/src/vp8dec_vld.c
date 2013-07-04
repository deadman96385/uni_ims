/******************************************************************************
 ** File Name:    vp8_entropy.c                                             *
 ** Author:       Xiaowei.Luo                                                 *
 ** DATE:         07/04/2013                                                  *
 ** Copyright:    2013 Spreatrum, Incoporated. All Rights Reserved.           *
 ** Description:                                                              *
 *****************************************************************************/
/******************************************************************************
 **                   Edit    History                                         *
 **---------------------------------------------------------------------------*
 ** DATE          NAME            DESCRIPTION                                 *
 ** 07/04/2013    Xiaowei.Luo     Create.                                     *
 *****************************************************************************/
/*----------------------------------------------------------------------------*
**                        Dependencies                                        *
**---------------------------------------------------------------------------*/
#include "vp8dec_video_header.h"
/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
extern   "C"
{
#endif

void vp8_default_coef_probs(VP8_COMMON *pc)
{
    vpx_memcpy(pc->fc.coef_probs, default_coef_probs, sizeof(default_coef_probs));
}

void vp8_init_mbmode_probs(VP8_COMMON *x)
{
    vpx_memcpy(x->fc.ymode_prob, vp8_ymode_prob, sizeof(vp8_ymode_prob));
    vpx_memcpy(x->kf_ymode_prob, vp8_kf_ymode_prob, sizeof(vp8_kf_ymode_prob));
    vpx_memcpy(x->fc.uv_mode_prob, vp8_uv_mode_prob, sizeof(vp8_uv_mode_prob));
    vpx_memcpy(x->kf_uv_mode_prob, vp8_kf_uv_mode_prob, sizeof(vp8_kf_uv_mode_prob));
    //vpx_memcpy(x->fc.sub_mv_ref_prob, sub_mv_ref_prob, sizeof(sub_mv_ref_prob));
}

void vp8_default_bmode_probs(vp8_prob p [VP8_BINTRAMODES-1])
{
    vpx_memcpy(p, vp8_bmode_prob, sizeof(vp8_bmode_prob));
}

void vp8_kf_default_bmode_probs(vp8_prob p [VP8_BINTRAMODES] [VP8_BINTRAMODES] [VP8_BINTRAMODES-1])
{
    vpx_memcpy(p, vp8_kf_bmode_prob, sizeof(vp8_kf_bmode_prob));
}

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
}
#endif
/**---------------------------------------------------------------------------*/
// End

