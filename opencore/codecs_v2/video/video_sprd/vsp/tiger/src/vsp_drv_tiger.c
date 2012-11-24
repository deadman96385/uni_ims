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
 ** 06/26/2012   Leon Li             Modify.                                                                                       *
 *****************************************************************************/

/*----------------------------------------------------------------------------*
**                        Dependencies                                        *
**---------------------------------------------------------------------------*/
#include "sci_types.h"
#include "video_common.h"
#include "vsp_drv_tiger.h"
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

//extern FILE	*	g_hvld_cmd_fp;
extern uint32 * g_cmd_info_ptr;	
uint32 *g_vsp_cmd_data_base;
uint32 *g_vsp_cmd_info_base;


#ifdef _VSP_LINUX_

uint32 g_vsp_Vaddr_base = 0;
int g_vsp_dev_fd = 0;
FunctionType_ResetVSP ResetVSP_cb=NULL;
PUBLIC void  VSP_SetVirtualBaseAddr(uint32 vsp_Vaddr_base)
{	
	g_vsp_Vaddr_base = vsp_Vaddr_base;
}
PUBLIC void  VSP_reg_reset_callback(FunctionType_ResetVSP cb,int fd)
{
	ResetVSP_cb = cb;
	g_vsp_dev_fd = fd;
}

#endif

uint32 *g_cmd_data_ptr;
uint32 *g_cmd_info_ptr;
uint32 *g_cmd_data_base;
uint32 *g_cmd_info_base;
uint32 *g_vsp_cmd_data_base;
uint32 *g_vsp_cmd_info_base;
/************************************************************************/
/* Reset HW                                                             */
/************************************************************************/
PUBLIC void  VSP_Reset(void)
{
//SCI_TRACE_LOW("VSP_Reset");
#ifdef _VSP_LINUX_
	if(ResetVSP_cb)
		(*ResetVSP_cb)(g_vsp_dev_fd);
#else
	uint32 cmd = 0;

	cmd = VSP_READ_REG(AHB_CTRL2,"AHB_CTRL2:Read the AHB_CTRL2 CLOCK");
	cmd |= 0xfe0;
	VSP_WRITE_REG(AHB_CTRL2,cmd,"AHB_CTRL2:enable MMMTX_CLK_EN");

	cmd = VSP_READ_REG(PLL_SRC,"PLL_SRC:Read the PLL_SRC CLOCK");
	cmd &=~(0xc);//192M
	VSP_WRITE_REG(PLL_SRC,cmd,"PLL_SRC:set vsp clock");

	cmd = VSP_READ_REG(DCAM_CLOCK_EN,"DCAM_CLOCK_EN:Read the DCAM_CLOCK_EN ");
	cmd = 0xFFFFFFFF;
	VSP_WRITE_REG(DCAM_CLOCK_EN,cmd,"DCAM_CLOCK_EN:enable DCAM_CLOCK_EN");

	VSP_WRITE_REG(0x20900218,cmd,"REMAP ,to make interrup enble");

	if(1)
	{
        	cmd = VSP_READ_REG(VSP_AXIM_REG_BASE+AXIM_GAP_ENDIAN_OFF, "AXIM_GAP_ENDIAN_OFF: readout AXIM_GAP_ENDIAN_OFF");
         	cmd  |= ((1<<26) |(1<<25));
		VSP_WRITE_REG (VSP_AXIM_REG_BASE+AXIM_GAP_ENDIAN_OFF, cmd, "'STOP AXIM");
	
       		if(VSP_READ_REG_POLL(VSP_AXIM_REG_BASE+AXIM_STS_OFF,(1<<0),0,TIME_OUT_CLK,"POLL AXIM_IDLE"))
		{
			cmd =cmd ;
		}
	}
	
	/*reset dcam and vsp*/
	cmd = VSP_READ_REG(VSP_RESET_ADDR, "VSP_RESET_ADDR: Read the vsp reset");
	VSP_WRITE_REG(VSP_RESET_ADDR, cmd | (/*(1<<2) |*/ (1<<15)), "VSP_RESET_ADDR: only reset vsp, don't reset dcam");
	VSP_WRITE_REG(VSP_RESET_ADDR, cmd | (/*(0<<2) |*/ (0<<15)), "VSP_RESET_ADDR: only reset vsp, don't reset dcam");
#if 0
#if defined(_VSP_)
	cmd = VSP_READ_REG(0x8b000070, "");
	cmd &= ~0xc;
	cmd |= (VSP_64MHz<<2);
	VSP_WRITE_REG(0x8b000070, cmd, "vsp: 96MHz");
#endif
#endif



#endif
	//for little endian system
//#if 0//defined(CHIP_ENDIAN_LITTLE)
#if defined(_VSP_)
	VSP_WRITE_REG(VSP_AHBM_REG_BASE+AHBM_ENDAIN_SEL_OFFSET, 0x5, "ENDAIN_SEL: 0x5 for little endian system");
#endif

}

/*only generate firmware command*/
PUBLIC void flush_unalign_bytes(int32 nbytes)
{
	int i = 0;
	uint32 cmd = 0;
	
	cmd = (8<<24) | 1;
	
	for (i = 0; i < nbytes; i++)
	{
		VSP_READ_REG_POLL_CQM(VSP_BSM_REG_BASE+BSM_DEBUG_OFF,3, 1, 1, "polling bsm fifo fifo depth >= 8 words for gob header");	
		VSP_WRITE_REG_CQM(VSP_BSM_REG_BASE+BSM_CFG2_OFF, cmd, "BSM_CFG2: flush one byte");	

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
		
		VSP_WRITE_REG(HUFFMAN_TBL_ADDR+i*4, val, "HUFFMAN_TBL_ADDR: configure vlc table");
	}

	close_vsp_iram();
}

volatile uint32 g_cmd_done_init;

/*COMMAND DONE INTERRUPT PROCESS*/
PUBLIC void CMD_DONE_INT_PROC(void)
{
	g_cmd_done_init = 1;
}

/*TIMEOUT INTERRUPT PROCESS*/
PUBLIC void TIMEOUT_INT_PROC(void)
{
	VSP_Reset();
}

PUBLIC inline int32 read_reg_poll(uint32 reg_addr, uint32 msk,uint32 exp_value, uint32 time, char *pstring)
{
	uint32 vsp_time_out_cnt = 0;
	
	while ((*(volatile uint32*)(reg_addr-VSP_DCAM_BASE+g_vsp_Vaddr_base) & msk) != exp_value)
	{
		if (vsp_time_out_cnt > time)
		{
			SCI_TRACE_LOW ("vsp_time_out_cnt %d!\n",vsp_time_out_cnt);
			return 1;
		}
		vsp_time_out_cnt++;
	}

	return 0;
}

 void open_vsp_iram (void)
{
	uint32 cmd;
//SCI_TRACE_LOW ("open_vsp_iram");
	cmd = VSP_READ_REG(VSP_DCAM_REG_BASE+DCAM_CFG_OFF, "DCAM_CFG: readout DCAM CFG");	
	cmd = cmd | ((1<<4) | (1<<3));		
	VSP_WRITE_REG(VSP_DCAM_REG_BASE+DCAM_CFG_OFF, cmd, "DCAM_CFG: configure DCAM register");
	
	VSP_READ_REG_POLL(VSP_DCAM_REG_BASE+DCAM_CFG_OFF, 1<<7, 1<<7, TIME_OUT_CLK, "DCAM_CFG: polling dcam clock status");
}

//allow hardware to access the vsp buffer
 void close_vsp_iram (void)
{
	uint32 cmd;
//SCI_TRACE_LOW ("close_vsp_iram");	
	cmd = VSP_READ_REG(VSP_DCAM_REG_BASE+DCAM_CFG_OFF, "DCAM_CFG: readout DCAM CFG");
	//cmd = (0<<4) | (1<<3);	
	cmd = (cmd & ~0x10) | (1 << 3);
	VSP_WRITE_REG(VSP_DCAM_REG_BASE+DCAM_CFG_OFF, cmd, "DCAM_CFG: configure DCAM register");
	
	VSP_READ_REG_POLL (VSP_DCAM_REG_BASE+DCAM_CFG_OFF, 0, 0, TIME_OUT_CLK, "DCAM_CFG: polling dcam clock status");
}


PUBLIC inline int32 READ_REG_MBC_ST0_REG(uint32 addr, uint32 msk,uint32 exp_value, char *pstring)
{
	int32 mbc_st0 = VSP_READ_REG(VSP_MBC_REG_BASE+MBC_ST0_OFF, "MBC_ST0: read regist");

	exp_value = mbc_st0 & msk;
	
	return exp_value;	
}


LOCAL void write_cmd (uint32 *addr_src, uint32 addr_phy, uint32 length)
{
	uint32 i;
//SCI_TRACE_LOW ("IN %s,%d!\n",__FUNCTION__,__LINE__);
//SCI_TRACE_LOW (" addr_src %x,%x,%d!\n",addr_src,addr_phy,length);
	VSP_WRITE_REG(VSP_BSM_REG_BASE+BSM_CFG0_OFF, ((0<<31)|0x3ffff), "configure bsm buffer size");
	VSP_WRITE_REG(VSP_BSM_REG_BASE+BSM_CFG1_OFF, (0<<31), "configure bsm buffer offset");
	VSP_WRITE_REG(VSP_BSM_REG_BASE+BSM_CFG2_OFF, V_BIT_2, "clear bsmw bit counter");

	open_vsp_iram();
	VSP_WRITE_REG(VSP_MEMO10_ADDR+8, (addr_phy>>2), "AHBM_FRM_ADDR_6: encoded bistream buffer0 addressr");

	close_vsp_iram();

	/*configure write data length*/
	VSP_WRITE_REG(VSP_BSM_REG_BASE+BSM_CFG2_OFF, 32<<24, "conifgure bsmw write data length");
//SCI_TRACE_LOW ("%s,%d!\n",__FUNCTION__,__LINE__);
	for (i = 0; i < length; i += 4) //length is byte unit
	{
		uint32 data = *addr_src++;
		VSP_READ_REG_POLL (VSP_BSM_REG_BASE+BSM_READY_OFF, 1, 1,TIME_OUT_CLK, "polling bsm rfifo ready");
		/*configure the write data*/
		VSP_WRITE_REG(VSP_BSM_REG_BASE+BSM_WDATA_OFF, data, "configure the value to be written to bitstream");
	}

	/*clear bsm-fifo, polling inactive status reg*/
	VSP_WRITE_REG(VSP_BSM_REG_BASE+BSM_CFG2_OFF, V_BIT_1, "clear bsm-fifo");
//SCI_TRACE_LOW ("OUT %s,%d!\n",__FUNCTION__,__LINE__);
	return;
}

PUBLIC void VSP_Write_CMD_by_BSM (uint32 cmd_info_base_phy, uint32 cmd_data_base_phy)
{
	uint32 cmd, length;
//	SCI_TRACE_LOW ("in VSP_Write_CMD_by_BSM!\n");
	/*init dcam command*/
	VSP_WRITE_REG(VSP_DCAM_REG_BASE+DCAM_CFG_OFF, (0<<4)|(1<<3), "configure DCAM register, switch buffer to hardware");
//SCI_TRACE_LOW ("%s,%d!\n",__FUNCTION__,__LINE__);
	/*init vsp command*/
	//cmd = V_BIT_17|V_BIT_14|V_BIT_7|V_BIT_4;
	cmd = V_BIT_17|V_BIT_14|V_BIT_7|V_BIT_4;
	cmd &= ~(V_BIT_17);

//	cmd = (1<<17) |(0<<16) |(0<<15) | (0<<14) |(1<<12) | (1<<7) | (1<<6) | (1<<3) | (1<<2) | (1<<1) | (1<<0);
	VSP_WRITE_REG(VSP_GLB_REG_BASE+GLB_CFG0_OFF, cmd, "little endian, bsmw enable, arbi enable");
//SCI_TRACE_LOW ("%s,%d!\n",__FUNCTION__,__LINE__);
	length = (uint32)g_cmd_info_ptr - (uint32)g_cmd_info_base;
	write_cmd (g_cmd_info_base, cmd_info_base_phy, length);
//SCI_TRACE_LOW ("%s,%d!\n",__FUNCTION__,__LINE__);
	length = (uint32)g_cmd_data_ptr - (uint32)g_cmd_data_base;
	write_cmd (g_cmd_data_base, cmd_data_base_phy, length);
//SCI_TRACE_LOW ("OUT %s,%d!\n",__FUNCTION__,__LINE__);
	return;
}/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef __cplusplus
}
#endif 
/**---------------------------------------------------------------------------*/
// End 
