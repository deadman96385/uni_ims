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
#include "sc8810_video_header.h"
/*#include "sci_types.h"
#include "video_common.h"*/
#include "vsp_drv_sc8810.h"
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

#ifdef _VSP_LINUX_

uint32 g_vsp_Vaddr_base = 0;
int g_vsp_dev_fd = 0;
FunctionType_ResetVSP ResetVSP_cb=NULL;
functionType_Start_CQM StartCQM = NULL;
FunctionType_Acquaire_VSP AcquaireVSP_cb = NULL;

PUBLIC void  VSP_SetVirtualBaseAddr(uint32 vsp_Vaddr_base)
{	
	g_vsp_Vaddr_base = vsp_Vaddr_base;
}
PUBLIC void  VSP_reg_reset_callback(FunctionType_ResetVSP cb,int fd)
{
	ResetVSP_cb = cb;
	g_vsp_dev_fd = fd;
}

PUBLIC void VSP_reg_start_cqm_callback(functionType_Start_CQM cb)
{
	StartCQM = cb;
}

PUBLIC void VSP_reg_acquaire_callback(FunctionType_Acquaire_VSP cb)
{
	AcquaireVSP_cb = cb;
}

#endif

uint32 *g_cmd_data_ptr;
uint32 *g_cmd_info_ptr;
uint32 *g_cmd_data_base;
uint32 *g_cmd_info_base;
uint32 *g_vsp_cmd_data_base;
uint32 *g_vsp_cmd_info_base;

PUBLIC  void write_vld_cabac_bfr(uint32 addr, int32 value, char *pstring)	
{
	*(uint32 *)(addr)  = (value);
}

PUBLIC  void vsp_write_register_normal(uint32 reg_addr, int32 value, char *pstring)	
{
#ifdef _VSP_LINUX_
       extern uint32 g_vsp_Vaddr_base;
	*(volatile uint32 *)(reg_addr-VSP_DCAM_BASE+g_vsp_Vaddr_base)  = (value);
#else
	*(volatile uint32 *)(reg_addr)  = (value);
#endif
}

PUBLIC  uint32 vsp_read_register_normal(uint32 reg_addr, char *pstring)	
{
#ifdef _VSP_LINUX_
       extern uint32 g_vsp_Vaddr_base;
	return (*(volatile uint32 *)(reg_addr-VSP_DCAM_BASE+g_vsp_Vaddr_base));
#else
	return (*(volatile uint32 *)(reg_addr));
#endif
}

PUBLIC  int32 vsp_read_reg_poll_normal(uint32 reg_addr, uint32 msk,uint32 exp_value, uint32 time, char *pstring)
{
	extern uint32 g_vsp_Vaddr_base;
	uint32 vsp_time_out_cnt = 0;
	
	while ((*(volatile uint32*)(reg_addr-VSP_DCAM_BASE+g_vsp_Vaddr_base) & msk) != exp_value)
	{
		if (vsp_time_out_cnt > time)
		{
			return 1;
		}
		vsp_time_out_cnt++;
	}

	return 0;
}

#if 0
PUBLIC  void vsp_write_register_cqm(uint32 reg_addr, int32 value, char *pstring)	
{	
	* g_cmd_data_ptr ++ = (value);
}

//same as read_register, xweiluo@20110719
PUBLIC  uint32 vsp_read_register_cqm (uint32 reg_addr, int8 *pString)
{
	return (*(volatile uint32 *)(reg_addr));
}

PUBLIC  void vsp_read_reg_poll_cqm(uint32 reg_addr, uint32 shift, uint32 msk_data,uint32 msked_data, char *pstring)
{

	uint32 value =  (shift<<24) | ((msk_data&0xfff)<<12) | (msked_data&0xfff);
	* g_cmd_data_ptr ++ = (value);
}

PUBLIC  void vsp_write_cmd_info(uint32 cmd_info)
{
	* g_cmd_info_ptr++ = cmd_info;
}
#endif

PUBLIC  int32 READ_REG_MBC_ST0_REG(uint32 addr, uint32 msk,uint32 exp_value, char *pstring)
{
	int32 mbc_st0 = VSP_READ_REG(VSP_MBC_REG_BASE+MBC_ST0_OFF, "MBC_ST0: read regist");

	exp_value = mbc_st0 & msk;
	
	return exp_value;	
}


/************************************************************************/
/* Reset HW                                                             */
/************************************************************************/
PUBLIC void  VSP_Reset(void)
{
#ifdef _VSP_LINUX_
	if(ResetVSP_cb)
		(*ResetVSP_cb)(g_vsp_dev_fd);
#else
	uint32 cmd = 0;

	VSP_WRITE_REG(DCAM_CLOCK_EN, (0x18002000), "DCAM_CLOCK_EN: enable dcam clock");
	
	//dont set DCAM_CLK_FREQUENCE on FPGA board. @xiaowei.luo,20090225
	//VSP_WRITE_REG (DCAM_CLK_FREQUENCE, 0x2003, "configure dcam clock to 80 MHz");
	
	/*reset dcam and vsp*/
	cmd = VSP_READ_REG(VSP_RESET_ADDR, "VSP_RESET_ADDR: Read the vsp reset");
	VSP_WRITE_REG(VSP_RESET_ADDR, cmd | (/*(1<<2) |*/ (1<<15)), "VSP_RESET_ADDR: only reset vsp, don't reset dcam");
	VSP_WRITE_REG(VSP_RESET_ADDR, cmd | (/*(0<<2) |*/ (0<<15)), "VSP_RESET_ADDR: only reset vsp, don't reset dcam");
#endif			
	//for little endian system
#if defined(_VSP_)
	VSP_WRITE_REG(VSP_AHBM_REG_BASE+AHBM_ENDAIN_SEL_OFFSET, 0x5, "ENDAIN_SEL: 0x5 for little endian system");

{
//	uint32 cmd = VSP_READ_REG(VSP_AHBM_REG_BASE+AHBM_ENDAIN_SEL_OFFSET, "");
//	SCI_TRACE_LOW("VSP_Reset: endian: %d.\n ", cmd);
}	
#endif

}

PUBLIC void VSP_START_CQM(void)
{	
#ifdef _VSP_LINUX_
	if(StartCQM)
		(*StartCQM)(g_vsp_dev_fd);
#endif
}

PUBLIC void VSP_ACQUAIRE_Dev(void)
{	
#ifdef _VSP_LINUX_
	if(AcquaireVSP_cb)
		(*AcquaireVSP_cb)(g_vsp_dev_fd);
#endif
}


/*only generate firmware command*/
PUBLIC void flush_unalign_bytes(int32 nbytes)
{
	int i = 0;
	uint32 cmd = 0;

//	SCI_TRACE_LOW("flush_unalign_bytes: nbytes: %d.\n ",nbytes);
	
	cmd = (8<<24) | 1;
	
	for (i = 0; i < nbytes; i++)
	{
		VSP_READ_REG_POLL_CQM(VSP_BSM_REG_BASE+BSM_DEBUG_OFF,3, 1, 1, "polling bsm fifo fifo depth >= 8 words for gob header");	
		VSP_WRITE_REG_CQM(VSP_BSM_REG_BASE+BSM_CFG2_OFF, cmd, "BSM_CFG2: flush one byte");	

		VSP_WRITE_CMD_INFO((VSP_BSM<<CQM_SHIFT_BIT) | (2<<24) | (BSM_CFG2_WOFF<<8) | ((1<<7) |BSM_DEBUG_WOFF));
	}
}

//allow software to access the vsp buffer
PUBLIC void open_vsp_iram (void)
{
	uint32 cmd;

	cmd = VSP_READ_REG(VSP_DCAM_REG_BASE+DCAM_CFG_OFF, "DCAM_CFG: readout DCAM CFG");	
	cmd = cmd | ((1<<4) | (1<<3));		
	VSP_WRITE_REG(VSP_DCAM_REG_BASE+DCAM_CFG_OFF, cmd, "DCAM_CFG: configure DCAM register");

	VSP_READ_REG_POLL(VSP_DCAM_REG_BASE+DCAM_CFG_OFF, 1<<7, 1<<7, TIME_OUT_CLK, "DCAM_CFG: polling dcam clock status");
}

//allow hardware to access the vsp buffer
PUBLIC void close_vsp_iram (void)
{
	uint32 cmd;

	cmd = VSP_READ_REG(VSP_DCAM_REG_BASE+DCAM_CFG_OFF, "DCAM_CFG: readout DCAM CFG");

	cmd = (cmd & ~0x10) | (1 << 3);
	VSP_WRITE_REG(VSP_DCAM_REG_BASE+DCAM_CFG_OFF, cmd, "DCAM_CFG: configure DCAM register");
	VSP_READ_REG_POLL (VSP_DCAM_REG_BASE+DCAM_CFG_OFF, 0, 0, TIME_OUT_CLK, "DCAM_CFG: polling dcam clock status");
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

	//SCI_TRACE_LOW("configure_huff_tab 1\n");
}

/**
stop vsp
**/
PUBLIC void Vsp_Stop()
{
	uint32 cmd = 0;
	
	/*reset dcam and vsp*/
	cmd = VSP_READ_REG(VSP_RESET_ADDR, "VSP_RESET_ADDR: Read the vsp reset");
	VSP_WRITE_REG(VSP_RESET_ADDR, cmd | (/*(1<<2) |*/ (1<<15)), "VSP_RESET_ADDR: only reset vsp, don't reset dcam");
	VSP_WRITE_REG(VSP_RESET_ADDR, cmd | (/*(0<<2) |*/ (0<<15)), "VSP_RESET_ADDR: only reset vsp, don't reset dcam");
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

LOCAL void write_cmd (uint32 *addr_src, uint32 addr_phy, uint32 length)
{
	uint32 i;

	VSP_WRITE_REG(VSP_BSM_REG_BASE+BSM_CFG0_OFF, ((0<<31)|0x3ffff), "configure bsm buffer size");
	VSP_WRITE_REG(VSP_BSM_REG_BASE+BSM_CFG1_OFF, (0<<31), "configure bsm buffer offset");
	VSP_WRITE_REG(VSP_BSM_REG_BASE+BSM_CFG2_OFF, V_BIT_2, "clear bsmw bit counter");

	open_vsp_iram();
	VSP_WRITE_REG(VSP_MEMO10_ADDR+8, (addr_phy>>8), "AHBM_FRM_ADDR_6: encoded bistream buffer0 addressr");
	close_vsp_iram();

	/*configure write data length*/
	VSP_WRITE_REG(VSP_BSM_REG_BASE+BSM_CFG2_OFF, 32<<24, "conifgure bsmw write data length");
	for (i = 0; i < length; i += 4) //length is byte unit
	{
		uint32 data = *addr_src++;

		VSP_READ_REG_POLL (VSP_BSM_REG_BASE+BSM_READY_OFF, 1, 1,TIME_OUT_CLK, "polling bsm rfifo ready");

		/*configure the write data*/
		VSP_WRITE_REG(VSP_BSM_REG_BASE+BSM_WDATA_OFF, data, "configure the value to be written to bitstream");
	}

	/*clear bsm-fifo, polling inactive status reg*/
	VSP_WRITE_REG(VSP_BSM_REG_BASE+BSM_CFG2_OFF, V_BIT_1, "clear bsm-fifo");

	return;
}

PUBLIC void VSP_Write_CMD_by_BSM (uint32 cmd_info_base_phy, uint32 cmd_data_base_phy)
{
	uint32 cmd, length;

	/*init dcam command*/
	VSP_WRITE_REG(VSP_DCAM_REG_BASE+DCAM_CFG_OFF, (0<<4)|(1<<3), "configure DCAM register, switch buffer to hardware");

	/*init vsp command*/
	cmd = V_BIT_17|V_BIT_14|V_BIT_7|V_BIT_4;
	VSP_WRITE_REG(VSP_GLB_REG_BASE+GLB_CFG0_OFF, cmd, "little endian, bsmw enable, arbi enable");

	length = (uint32)g_cmd_info_ptr - (uint32)g_cmd_info_base;
	write_cmd (g_cmd_info_base, cmd_info_base_phy, length);

	length = (uint32)g_cmd_data_ptr - (uint32)g_cmd_data_base;
	write_cmd (g_cmd_data_base, cmd_data_base_phy, length);

	return;
}

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef __cplusplus
}
#endif 
/**---------------------------------------------------------------------------*/
// End 
