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

#define AHB_BASE_ADDR		0x20900000
#define AHB_SHARERAM_OFF	AHB_BASE_ADDR+0x200
#define AHB_VSP_GLB_OFF		AHB_BASE_ADDR+0x1000
#define AHB_BSM_CTRL_OFF	AHB_BASE_ADDR+0x8000

#ifdef ORSC_SIM
	#define ORSC_BASE_ADDR		0x00500000
#else
	#define ORSC_BASE_ADDR		0x80000000
#endif
#define ORSC_SHARERAM_OFF	ORSC_BASE_ADDR+0x200
#define ORSC_VSP_GLB_OFF	ORSC_BASE_ADDR+0x1000
#define ORSC_PPA_SINFO_OFF	ORSC_BASE_ADDR+0x1200		// ListX_POC[X][31:0], MCA weighted & offset table
#define ORSC_IQW_TBL_OFF	ORSC_BASE_ADDR+0x1400
#define ORSC_FMADD_TBL_OFF	ORSC_BASE_ADDR+0x1800
#define ORSC_VLC0_TBL_OFF	ORSC_BASE_ADDR+0x2000
#define ORSC_VLC1_TBL_OFF	ORSC_BASE_ADDR+0x3000
#define ORSC_PPA_PARAM_OFF	ORSC_BASE_ADDR+0x7400
#define ORSC_BSM_CTRL_OFF	ORSC_BASE_ADDR+0x8000

#define	VSP_REG_BASE_ADDR 0x80000000
#define	SHARE_RAM_BASE_ADDR (VSP_REG_BASE_ADDR+0x200)
#define	GLB_REG_BASE_ADDR (VSP_REG_BASE_ADDR+0x1000)
#define	PPA_SLICE_INFO_BASE_ADDR (VSP_REG_BASE_ADDR+0x1200)
#define	DCT_IQW_TABLE_BASE_ADDR (VSP_REG_BASE_ADDR+0x1400)
#define	FRAME_ADDR_TABLE_BASE_ADDR (VSP_REG_BASE_ADDR+0x1800)
#define	CABAC_CONTEXT_BASE_ADDR (VSP_REG_BASE_ADDR+0x1a00)
#define	VLC_TABLE0_BASE_ADDR (VSP_REG_BASE_ADDR+0x2000)
#define	VLC_TABLE1_BASE_ADDR (VSP_REG_BASE_ADDR+0x3000)
#define	BSM_CTRL_REG_BASE_ADDR (VSP_REG_BASE_ADDR+0x8000)


//glb reg
#define VSP_INT_SYS_OFF 0x0
#define VSP_INT_MASK_OFF 0x04
#define VSP_INT_CLR_OFF 0x08
#define VSP_INT_RAW_OFF 0x0c
#define VSP_MODE_OFF 0x20
#define IMG_SIZE_OFF 0x24
#define RAM_ACC_SEL_OFF 0x28
#define VSP_INT_GEN_OFF 0x2c
#define VSP_START_OFF 0x30
#define VSP_SIZE_SET_OFF 0x34
#define VSP_CFG0_OFF 0x3c
#define VSP_CFG1_OFF 0x40
#define VSP_CFG2_OFF 0x44
#define VSP_CFG3_OFF 0x48
#define VSP_CFG4_OFF 0x4c
#define VSP_CFG5_OFF 0x50
#define VSP_CFG6_OFF 0x54
#define VSP_CFG7_OFF 0x58


#ifdef ORSC_SIM
	#define VLC_TABLE_BFR_ADDR	0x0600000
#else
	#define VLC_TABLE_BFR_ADDR	0x4500000
#endif
#define VLC_TABLE_BFR_SIZE	0x4000		// 16K

#ifdef SIM_IN_WIN
	#define OR1200_WRITE_REG(reg_addr, value, pstring) FPRINTF_ORSC(g_vsp_glb_reg_fp,"1_%08x_%08x \t\t//%s\n", reg_addr, value, pstring)
	#define OR1200_READ_REG(reg_addr, pstring) FPRINTF_ORSC(g_vsp_glb_reg_fp,"0_%08x_00000000 \t\t\t//%s\n", reg_addr, pstring)
	#define OR1200_READ_REG_POLL(reg_addr, msk_data, msked_data, pstring) FPRINTF_ORSC(g_vsp_glb_reg_fp,"2_%08x_%08x_%08x \t//%s\n", reg_addr, msk_data, msked_data, pstring)
#else
	#define OR1200_WRITE_REG(reg_addr, value, pstring) (*((volatile uint32*)(reg_addr)))=(value)
	#define OR1200_READ_REG(reg_addr, pstring) (*((volatile uint32*)(reg_addr)))
#if ORSC_SIM
	#define OR1200_READ_REG_POLL(reg_addr, msk_data, msked_data, pstring)
#else
	#define OR1200_READ_REG_POLL(reg_addr, msk_data, msked_data, pstring) \
	{\
		uint32 tmp;\
		tmp=(*((volatile uint32*)(reg_addr)))&(msk_data);\
		while(tmp != (msked_data))\
		{\
		tmp=(*((volatile uint32*)(reg_addr)))&(msk_data);\
		}\
	}
#endif
#endif

void SharedRAM_Init();
void ORSC_Init();
void BSM_Init();

#endif