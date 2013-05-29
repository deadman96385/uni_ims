/******************************************************************************
 ** File Name:      vsp_drv.c                                                 *
 ** Author:         Xiaowei Luo                                               *
 ** DATE:           06/11/2009                                                *
 ** Copyright:      2009 Spreatrum, Incoporated. All Rights Reserved.         *
 ** Description:    VSP Driver												  *
 *****************************************************************************/
/******************************************************************************
 **                   Edit    History                                         *
 **---------------------------------------------------------------------------* 
 ** DATE          NAME            DESCRIPTION                                 * 
 ** 06/11/2009    Xiaowei Luo     Create.                                     *
 *****************************************************************************/

/*----------------------------------------------------------------------------*
**                        Dependencies                                        *
**---------------------------------------------------------------------------*/
#include "sci_types.h"
#include "video_common.h"
#include "vsp_drv_sc8800g.h"
#if !defined(_VSP_)
#include "common_global.h"
#include "bsm_global.h"
#endif //_CMODEL_

/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef __cplusplus
extern "C"
{
#endif 

#define VSP_96MHz	0x00
#define VSP_64MHz	0x01
#define VSP_48MHz	0x10
#define VSP_26MHz	0x11

extern FILE	*	g_hvld_cmd_fp;

/************************************************************************/
/* Reset HW                                                             */
/************************************************************************/
PUBLIC void  VSP_Reset(void)
{
	uint32 cmd = 0;
// 	cmd = (1<<1);

//	cmd |= VSP_READ_REG(DCAM_CLOCK_EN, "DCAM_CLOCK_EN: Read the dcam clock");
	WRITE_REG(DCAM_CLOCK_EN, (0x18002000), "DCAM_CLOCK_EN: enable dcam clock");
	
	//dont set DCAM_CLK_FREQUENCE on FPGA board. @xiaowei.luo,20090225
	//VSP_WRITE_REG (DCAM_CLK_FREQUENCE, 0x2003, "configure dcam clock to 80 MHz");
	
	/*reset dcam and vsp*/
	cmd = READ_REG(VSP_RESET_ADDR, "VSP_RESET_ADDR: Read the vsp reset");
	WRITE_REG(VSP_RESET_ADDR, cmd | (/*(1<<2) |*/ (1<<15)), "VSP_RESET_ADDR: only reset vsp, don't reset dcam");
	WRITE_REG(VSP_RESET_ADDR, cmd | (/*(0<<2) |*/ (0<<15)), "VSP_RESET_ADDR: only reset vsp, don't reset dcam");
}

/*only generate firmware command*/
PUBLIC void flush_unalign_bytes(int32 nbytes)
{
	int i = 0;
	uint32 cmd = 0;
	
	cmd = (8<<24) | 1;
	
	for (i = 0; i < nbytes; i++)
	{
		VSP_READ_REG_POLL(VSP_BSM_REG_BASE+BSM_DEBUG_OFF,3, 1, 1, "polling bsm fifo fifo depth >= 8 words for gob header");	
		VSP_WRITE_REG(VSP_BSM_REG_BASE+BSM_CFG2_OFF, cmd, "BSM_CFG2: flush one byte");	

		VSP_WRITE_CMD_INFO((VSP_BSM<<MID_SHIFT_BIT) | (2<<24) | (BSM_CFG2_WOFF<<8) | ((1<<7) |BSM_DEBUG_WOFF));
	}
}

/**
configure the huffman table
**/
PUBLIC void configure_huff_tab(uint32 *pHuff_tab, int32 n)
{
	int i = 0;
	uint32 val = 0;
	
	open_vsp_iram();

	for(i = 0; i < n; i++)
	{
		val = *pHuff_tab++;
		
		WRITE_REG(HUFFMAN_TBL_ADDR+i*4, val, "HUFFMAN_TBL_ADDR: configure vlc table");
	}

	close_vsp_iram();
}

uint32 g_cmd_done_init;

/*COMMAND DONE INTERRUPT PROCESS*/
PUBLIC void CMD_DONE_INT_PROC(void)
{
	g_cmd_done_init = 1;
}

/*TIMEOUT INTERRUPT PROCESS*/
PUBLIC void TIMEOUT_INT_PROC(void)
{

	
}
/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef __cplusplus
}
#endif 
/**---------------------------------------------------------------------------*/
// End 
