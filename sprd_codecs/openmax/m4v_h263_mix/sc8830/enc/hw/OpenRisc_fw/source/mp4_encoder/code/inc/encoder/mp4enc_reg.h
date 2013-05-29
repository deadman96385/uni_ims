/******************************************************************************
** File Name:      mp4enc_reg.h                                               *
** Author:         James Chen	                                              *
** DATE:           12/10/2012                                                 *
** Copyright:      2007 Spreatrum, Incoporated. All Rights Reserved.          *
** Description:    rate control for video codec.	     			          *
*****************************************************************************/
/******************************************************************************
**                   Edit    History                                         *
**---------------------------------------------------------------------------* 
** DATE          NAME            DESCRIPTION                                 * 
** 12/10/2012    James Chen	     Create.                                     *
*****************************************************************************/
#ifndef _MP4ENC_REG_H_
#define _MP4ENC_REG_H_

#include "video_common.h"

#define AHB_BASE_ADDR		0x80000000
#define AHB_SHARERAM_OFF	AHB_BASE_ADDR+0x200
#define AHB_VSP_GLB_OFF		AHB_BASE_ADDR+0x1000
#define AHB_BSM_CTRL_OFF	AHB_BASE_ADDR+0x8000

/*****************openrisc mem map***********/
#define VSP_REG_BASE_ADDR		0x80000000


#define	VSP_IMEM_BASE_ADDR 0x00000000//openrisc D wishbone//weihu//
//#define	DDR_MEM_BASE_ADDR 0//0x80000000//openrisc D wishbone//weihu//

#define	SHARE_RAM_BASE_ADDR			VSP_REG_BASE_ADDR+0x200
#define	GLB_REG_BASE_ADDR			VSP_REG_BASE_ADDR+0x1000
#define	PPA_SLICE_INFO_BASE_ADDR	VSP_REG_BASE_ADDR+0x1200
#define	DCT_IQW_TABLE_BASE_ADDR		VSP_REG_BASE_ADDR+0x1400
#define	FRAME_ADDR_TABLE_BASE_ADDR	VSP_REG_BASE_ADDR+0x1800
#define	VLC_TABLE0_BASE_ADDR		VSP_REG_BASE_ADDR+0x2000
#define	VLC_TABLE1_BASE_ADDR		VSP_REG_BASE_ADDR+0x3000
#define	ORSC_PPA_PARAM_OFF			VSP_REG_BASE_ADDR+0x7400
#define	BSM_CTRL_REG_BASE_ADDR		VSP_REG_BASE_ADDR+0x8000
#ifdef ORSC_SIM
	#define VLC_TABLE_BFR_ADDR	0x0600000
#else
	#define VLC_TABLE_BFR_ADDR	0x4500000
#endif
#define VLC_TABLE_BFR_SIZE	0x4000		// 16K

#ifdef SIM_IN_WIN
	#define OR1200_WRITE_REG(reg_addr, value, pstring) { if(g_vector_enable_flag&VECTOR_ENABLE_FW) FPRINTF_ORSC(g_fp_global_tv,"1_%08x_%08x   //%s\n",reg_addr,value,pstring);}//weihu 
	#define OR1200_READ_REG(reg_addr, pstring)	{if(g_vector_enable_flag&VECTOR_ENABLE_FW) FPRINTF_ORSC(g_fp_global_tv,"0_%08x_0x00000000   //%s\n",reg_addr,pstring);}
	#define OR1200_READ_REG_POLL(reg_addr, msk_data, msked_data, pstring) {if(g_vector_enable_flag&VECTOR_ENABLE_FW) FPRINTF_ORSC(g_fp_global_tv,"2_%08x_%08x_%08x   //%s\n",reg_addr,msk_data,msked_data,pstring);}
#else
	#define OR1200_WRITE_REG(reg_addr, value, pstring) *((volatile uint32*)(reg_addr))=(value)
	#define OR1200_READ_REG(reg_addr, pstring) *((volatile uint32*)(reg_addr))
#if ORSC_SIM
	#define OR1200_READ_REG_POLL(reg_addr, msk_data, msked_data, pstring)
#else
	#define OR1200_READ_REG_POLL(reg_addr, msk_data, msked_data, pstring) \
	{\
		int tmp;\
		tmp=(*((volatile uint32*)(reg_addr)))&msk_data;\
		while(tmp != msked_data)\
		{\
		tmp=(*((volatile uint32*)(reg_addr)))&msk_data;\
		}\
	}
#endif
#endif

#endif
