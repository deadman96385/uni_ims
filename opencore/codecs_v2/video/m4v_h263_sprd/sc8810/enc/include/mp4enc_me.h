/******************************************************************************
 ** File Name:      mp4_vsp_mea.h                                             *
 ** Author:         Xiaowei Luo                                               *
 ** DATE:           12/14/2006                                                *
 ** Copyright:      2006 Spreatrum, Incoporated. All Rights Reserved.         *
 ** Description:    This file defines the basic vsp mea module interfaces     *
 ** 				of mp4 codec.		      								  *
 *****************************************************************************/
/******************************************************************************
 **                   Edit    History                                         *
 **---------------------------------------------------------------------------* 
 ** DATE          NAME            DESCRIPTION                                 * 
 ** 12/14/2006    Xiaowei Luo     Create.                                     *
 *****************************************************************************/
#ifndef _MP4_MEA_H_
#define _MP4_MEA_H_

/*----------------------------------------------------------------------------*
**                        Dependencies                                        *
**---------------------------------------------------------------------------*/
#include "video_common.h"
#include "mp4_basic.h"
#include "mp4enc_mode.h"
#include "mp4enc_mode.h"
/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    extern   "C" 
    {
#endif

typedef struct 
{
	int start_X;
	int start_Y;
	int fetch_XWidth;
	int fetch_YHeigth;

	int leftPixX;
	int leftPixY;
} MEA_FETCH_REF;

#define MB_NB					256  //250

#define MB_SAD_THRESHOLD		10 //0 
#define MAX_MV_X				23
#define MAX_MV_Y				23
#define MAX_MV_X_H263			15
#define MAX_MV_Y_H263			15
#define MAX_SAD					0x7FFFFFFF
#define MAX_SEARCH_CYCLE		9 //7

#define PREFILTER_EN			
#define PRED_EN
#define _4MV_ENABLE				//8x8 search enable or not
#define INTRA_EN				//intra mb mode enable or not

////////////////////////////////////////////////////////////////////////////////////////
//don't modified the following macro define
#define Y_LEN (HEIGHT*WIDTH)
#define C_LEN (Y_LEN>>2)
#define HALF_HEIGHT (HEIGHT>>1)
#define HALF_WIDTH (WIDTH>>1)
#define QUANT_HEIGHT (HEIGHT>>2)
#define MB_NUM	(((WIDTH+15)/16)*((HEIGHT+15)/16))

/**----------------------------------------------------------------------------*
**                           Function Prototype                               **
**----------------------------------------------------------------------------*/
PUBLIC void Mp4Enc_JudgeMBMode(ENC_MB_MODE_T * pmbmd);
PUBLIC void Mp4Enc_init_MEA(ENC_VOP_MODE_T *vop_mode_ptr);
PUBLIC void Mp4Enc_Init_MEA_Fetch(ENC_VOP_MODE_T *vop_mode_ptr);
PUBLIC void Mp4Enc_Update_MEA_Fetch_Y(ENC_VOP_MODE_T * vop_mode_ptr);
PUBLIC void Mp4Enc_Update_MEA_Fetch_X (ENC_VOP_MODE_T * vop_mode_ptr);
PUBLIC void Mp4Enc_MVprediction(ENC_VOP_MODE_T *vop_mode_ptr, ENC_MB_MODE_T *mb_mode_ptr, uint32 mb_pos_x); 

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End 
#endif  //_MP4_MEA_H_
