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
#include "h264dec_video_header.h"
/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
extern   "C"
{
#endif

//sw
const uint8 * g_rgiClipTab;

/*function pointer array*/
MC4xN_LUMA g_MC4xN_luma[16];
MC8xN_LUMA g_MC8xN_luma[16];
MC16xN_LUMA g_MC16xN_luma[16];

Intra4x4Pred g_intraPred4x4[9];
Intra8x8Pred g_intraPred8x8[9];
Intra16x16Pred g_intraPred16x16[4];
IntraChromPred g_intraChromaPred[4];

MC_chroma8xN g_MC_chroma8xN;
MC_chroma4xN g_MC_chroma4xN;
MC_chroma2xN g_MC_chroma2xN;
/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
}
#endif
/**---------------------------------------------------------------------------*/
// End
