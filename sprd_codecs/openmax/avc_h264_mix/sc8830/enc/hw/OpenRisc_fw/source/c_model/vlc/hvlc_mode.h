/******************************************************************************
 ** File Name:      hvlc_mode.h	                                          *
 ** Author:         Xiaowei Luo                                               *
 ** DATE:           11/20/2007                                                *
 ** Copyright:      2007 Spreatrum, Incoporated. All Rights Reserved.         *
 ** Description:    VSP bsm Driver for video codec.	  						  *
 *****************************************************************************/
/******************************************************************************
 **                   Edit    History                                         *
 **---------------------------------------------------------------------------* 
 ** DATE          NAME            DESCRIPTION                                 * 
 ** 11/20/2007    Xiaowei Luo     Create.                                     *
 *****************************************************************************/
#ifndef _HVLC_MODE_H_
#define _HVLC_MODE_H_

/*----------------------------------------------------------------------------*
**                        Dependencies                                        *
**---------------------------------------------------------------------------*/
#include "video_common.h"
/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    extern   "C" 
    {
#endif
	
#define BLOCK_INDEX_CHROMA_DC   (-1)
#define BLOCK_INDEX_LUMA_DC     (-2)

typedef struct
{
    int32 i_bits;
    int32 i_size;
} vlc_t;

#endif //_HVLC_MODE_H_