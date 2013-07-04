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

/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
extern   "C"
{
#endif



extern uint32 g_mp4_dec_huff_tbl[152];
extern uint32 g_rvlc_huff_tab[146];

//common
extern const uint8 g_standard_zigzag[BLOCK_SQUARE_SIZE];
extern const uint8 g_default_intra_qmatrix[BLOCK_SQUARE_SIZE];
extern const uint8 g_default_inter_qmatrix[BLOCK_SQUARE_SIZE];
//for data partition

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
}
#endif
/**---------------------------------------------------------------------------*/
// End
#endif  //_MP4DEC_GLOBAL_H_
