/******************************************************************************
** File Name:      h264enc_reg.h                                              *
** Author:         Derek Yu		                                              *
** DATE:           07/09/2012                                                 *
** Copyright:      2007 Spreatrum, Incoporated. All Rights Reserved.          *
** Description:    rate control for video codec.	     			          *
*****************************************************************************/
/******************************************************************************
**                   Edit    History                                         *
**---------------------------------------------------------------------------* 
** DATE          NAME            DESCRIPTION                                 * 
** 07/09/2012    Derek Yu	     Create.                                     *
*****************************************************************************/
#ifndef _H264ENC_REG_H_
#define _H264ENC_REG_H_

#include "video_common.h"




#define ORSC_BASE_ADDR		0x60900000

#define ORSC_SHARERAM_OFF	ORSC_BASE_ADDR+0x200
#define ORSC_VSP_GLB_OFF	ORSC_BASE_ADDR+0x1000
#define ORSC_PPA_SINFO_OFF	ORSC_BASE_ADDR+0x1200		// ListX_POC[X][31:0], MCA weighted & offset table
#define ORSC_IQW_TBL_OFF	ORSC_BASE_ADDR+0x1400
#define ORSC_FMADD_TBL_OFF	ORSC_BASE_ADDR+0x1800
#define ORSC_VLC0_TBL_OFF	ORSC_BASE_ADDR+0x2000
#define ORSC_VLC1_TBL_OFF	ORSC_BASE_ADDR+0x3000
#define ORSC_PPA_PARAM_OFF	ORSC_BASE_ADDR+0x7400
#define ORSC_BSM_CTRL_OFF	ORSC_BASE_ADDR+0x8000


void ORSC_Init();
void BSM_Init();

#endif