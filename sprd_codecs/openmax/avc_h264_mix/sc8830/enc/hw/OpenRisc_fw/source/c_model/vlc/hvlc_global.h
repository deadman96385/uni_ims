/******************************************************************************
 ** File Name:      hvlc_global.h	                                          *
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
#ifndef _HVLC_GLOBAL_H_
#define _HVLC_GLOBAL_H_

/*----------------------------------------------------------------------------*
**                        Dependencies                                        *
**---------------------------------------------------------------------------*/
#include "video_common.h"
#include "hvlc_mode.h"
#include "hvlc_tv.h"
/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    extern   "C" 
    {
#endif
	
extern vlc_t x264_coeff_token[5][17*4];
extern vlc_t x264_level_prefix[16];
extern vlc_t x264_total_zeros[15][16];
extern vlc_t x264_total_zeros_dc[3][4];
extern vlc_t x264_run_before[7][15];

void block_residual_write_cavlc( ENC_IMAGE_PARAMS_T *img_ptr, ENC_MB_MODE_T *mb_info_ptr, int32 i_idx, int16 *coef, int32 i_count );
void macroblock_luma_write_cavlc( ENC_IMAGE_PARAMS_T *img_ptr, ENC_MB_MODE_T *mb_info_ptr, int32 i8start, int32 i8end );


#endif //_HVLC_GLOBAL_H_