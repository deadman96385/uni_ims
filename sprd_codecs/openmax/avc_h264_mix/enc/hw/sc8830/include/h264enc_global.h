/******************************************************************************
 ** File Name:      h264enc_global.h                                           *
 ** Author:         Xiaowei Luo                                               *
 ** DATE:           06/18/2013                                                *
 ** Copyright:      2013 Spreatrum, Incoporated. All Rights Reserved.         *
 ** Description:                                                                                                   *
 *****************************************************************************/
/******************************************************************************
 **                   Edit    History                                         *
 **---------------------------------------------------------------------------*
 ** DATE          NAME            DESCRIPTION                                 *
 ** 01/23/2007    Xiaowei Luo     Create.                                     *
 *****************************************************************************/
#ifndef _H264ENC_GLOBAL_H_
#define _H264ENC_GLOBAL_H_

#include "sci_types.h"

#ifdef   __cplusplus
extern   "C"
{
#endif
extern uint32 g_vlc_hw_tbl [406*2*3];
extern uint32 g_skipBlock_QP_table[52];

//cabac
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

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
}
#endif
/**---------------------------------------------------------------------------*/

#endif //_H264ENC_GLOBAL_H_