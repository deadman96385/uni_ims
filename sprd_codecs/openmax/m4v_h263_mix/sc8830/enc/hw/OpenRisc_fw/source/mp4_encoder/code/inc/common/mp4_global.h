/******************************************************************************
 ** File Name:      mp4_global.h                                              *
 ** Author:         Xiaowei Luo                                               *
 ** DATE:           10/13/2009                                                *
 ** Copyright:      2006 Spreatrum, Incoporated. All Rights Reserved.         *
 ** Description:    This file defines the global interfaces of mp4 codec      *
 *****************************************************************************/
/******************************************************************************
 **                   Edit    History                                         *
 **---------------------------------------------------------------------------* 
 ** DATE          NAME            DESCRIPTION                                 * 
 ** 01/23/2007    Xiaowei Luo     Create.                                     *
 *****************************************************************************/
#ifndef _MP4_GLOBAL_H_
#define _MP4_GLOBAL_H_
/*----------------------------------------------------------------------------*
**                        Dependencies                                        *
**---------------------------------------------------------------------------*/
#include "mp4_basic.h"

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

char weightscale4x4[6][4][4];//6 6*2+2=14*16=224B
char weightscale8x8[2][8][8];//2 2*2+2=6*64=384B

//#define OUTPUT_TEST_VECTOR

//#define WITHOUT_THRES
#if !(_CMODEL_)
#define PrintfVLCOut
#endif

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End 
#endif  //_MP4_GLOBAL_H_
