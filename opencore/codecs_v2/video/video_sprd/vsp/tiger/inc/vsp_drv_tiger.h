/******************************************************************************
 ** File Name:      vsp_drv_tiger.h                                         *
 ** Author:         Xiaowei Luo                                               *
 ** DATE:           01/07/2010                                                *
 ** Copyright:      2010 Spreatrum, Incoporated. All Rights Reserved.         *
 ** Description:    														  *
 *****************************************************************************/
/******************************************************************************
 **                   Edit    History                                        **
 **---------------------------------------------------------------------------* 
 ** DATE          NAME            DESCRIPTION                                 * 
 ** 06/11/2009    Xiaowei Luo     Create.   
 ** 06/26/2012   Leon Li             modify
 *****************************************************************************/
#ifndef _VSP_DRV_TIGER_H_
#define _VSP_DRV_TIGER_H_
/*----------------------------------------------------------------------------*
**                        Dependencies                                        *
**---------------------------------------------------------------------------*/
   
/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef __cplusplus
extern "C"
{
#endif 
#include "sci_types.h"
#include "vsp_dcam.h"
#include "vsp_axim.h"
#include "vsp_vld.h"
#include "vsp_global.h"
#include "vsp_bsm.h"
#include "vsp_vlc.h"
#include "vsp_mca.h"
#include "vsp_mea.h"
#include "vsp_dbk.h"
#include "vsp_mbc.h"
#include "vsp_dct.h"
#include "vsp_glb_ctrl.h"
#include "vsp_global_define.h"

#include <linux/ioctl.h>

/*----------------------------------------------------------------------------*
**                            Macro Definitions                               *
**---------------------------------------------------------------------------*/

#define SPRD_VSP_DRIVER "/dev/sprd_vsp"

#ifdef _VSP_LINUX_
#define LOG_TAG "VSP"
#include <utils/Log.h>
#define  SCI_TRACE_LOW   ALOGD
#define SCI_MEMSET  memset
#define SCI_MEMCPY	memcpy
#define SCI_ASSERT(...) 
#endif 
/*----------------------------------------------------------------------------*
**                            Macro Definitions                               *
**---------------------------------------------------------------------------*/

#define TIME_OUT_CLK			0x7fffff

#define IS_TIME_OUT				1
#define NOT_TIME_OUT			0

#define MID_SHIFT_BIT			29 //28	//shift bit of module id
#define CQM_SHIFT_BIT			29 //28	//shift bit of module id
extern uint32 *g_cmd_data_ptr;
extern uint32 *g_cmd_info_ptr;

typedef enum{
	VSP_GLB = 0,
	VSP_BSM,
	VSP_VLD,
	//VSP_VLC,
	VSP_RAM10,
	VSP_DCT,
	VSP_MCA,
	VSP_MBC,
	VSP_DBK
	}VSP_MODULE_ID;

/*standard*/
typedef enum {
		VSP_ITU_H263 = 0, 
		VSP_MPEG4,  
		VSP_JPEG,
		VSP_FLV_V1,
		VSP_H264,
		VSP_RV8,
		VSP_RV9
		}VIDEO_STD_E;

#define VSP_INTRA		0
#define VSP_INTER		1

#define VSP_STD_ZIGZAG		0
#define VSP_HOR_ZIGZAG		1
#define VSP_VER_ZIGZAG		2

#ifdef _VSP_LINUX_
typedef int (*FunctionType_ResetVSP)(int fd);
PUBLIC void  VSP_reg_reset_callback(FunctionType_ResetVSP p_cb,int fd);
void  VSP_SetVirtualBaseAddr(uint32 vsp_Vaddr_base);
#endif

PUBLIC void VSP_Reset (void);
PUBLIC void configure_huff_tab (uint32 *pHuff_tab, int32 n);
PUBLIC void flush_unalign_bytes (int32 nbytes);
PUBLIC void CMD_DONE_INT_PROC(void);
PUBLIC void TIMEOUT_INT_PROC(void);

extern volatile uint32 g_cmd_done_init;
extern uint32 g_vsp_Vaddr_base;

#if !defined(_VSP_)
PUBLIC void write_register(uint32 reg_addr, int32 value, char *pstring);
PUBLIC uint32 read_register (uint32 reg_addr, int8 *pString);
PUBLIC int32 read_reg_poll(uint32 addr, uint32 msk,uint32 exp_value, uint32 time, char *pstring);

PUBLIC void vsp_write_register(uint32 reg_addr, int32 value, char *pstring);
PUBLIC uint32 vsp_read_register (uint32 reg_addr, int8 *pString);
PUBLIC void vsp_read_reg_poll(uint32 reg_addr,uint32 shift, uint32 msk_data,uint32 msked_data, char *pstring);

PUBLIC void vsp_write_cmd_info(uint32 cmd_info);
#else

/*PUBLIC inline void write_vld_cabac_bfr(uint32 addr, int32 value, char *pstring)	
{
	*(uint32 *)(addr)  = (value);
}

PUBLIC inline void write_register(uint32 reg_addr, int32 value, char *pstring)	
{
	*(volatile uint32 *)(reg_addr)  = (value);
}

PUBLIC inline uint32 read_register(uint32 reg_addr, char *pstring)	
{
	return (*(volatile uint32 *)(reg_addr));
}

PUBLIC inline int32 read_reg_poll(uint32 reg_addr, uint32 msk,uint32 exp_value, uint32 time, char *pstring)
{
	uint32 vsp_time_out_cnt = 0;
	
	while ((*(volatile uint32*)reg_addr & msk) != exp_value)
	{
		if (vsp_time_out_cnt > time)
		{
			return 1;
		}
		vsp_time_out_cnt++;
	}

	return 0;
}
*/
/*
PUBLIC inline void vsp_write_register(uint32 reg_addr, int32 value, char *pstring)	
{	
	* g_cmd_data_ptr ++ = (value);
}

PUBLIC inline uint32 vsp_read_register (uint32 reg_addr, int8 *pString)
{
	return (*(volatile uint32 *)(reg_addr));
}

PUBLIC inline void vsp_read_reg_poll(uint32 reg_addr, uint32 shift, uint32 msk_data,uint32 msked_data, char *pstring)
{

	uint32 value =  (shift<<24) | ((msk_data&0xfff)<<12) | (msked_data&0xfff);
	* g_cmd_data_ptr ++ = (value);
}

PUBLIC inline void vsp_write_cmd_info(uint32 cmd_info)
{
	* g_cmd_info_ptr++ = cmd_info;
}
*/
PUBLIC inline int32 READ_REG_MBC_ST0_REG(uint32 addr, uint32 msk,uint32 exp_value, char *pstring);

#endif //!defined(_VSP_)

//normal mode
#define WRITE_VLD_CABAC_BFR(addr, value, pstring) (*(uint32 *)(addr)  = (value))

#define VSP_WRITE_REG(reg_addr, value, pstring) (*(volatile uint32 *)(reg_addr-VSP_DCAM_BASE+g_vsp_Vaddr_base)  = (value))
#define VSP_READ_REG(reg_addr, pstring)		((*(volatile uint32 *)(reg_addr-VSP_DCAM_BASE+g_vsp_Vaddr_base)))

#define VSP_READ_REG_POLL(reg_addr, msk, exp_value, time, pstring) \
read_reg_poll(reg_addr, msk, exp_value, time, pstring) 
//command queue mode
#define VSP_WRITE_REG_CQM(reg_addr, value, pstring) (* g_cmd_data_ptr ++ = (value))
#define VSP_READ_REG_CQM(reg_addr, pstring)		((*(volatile uint32 *)(reg_addr)))
#define VSP_READ_REG_POLL_CQM(reg_addr,shift, msk_data, msked_data, pstring) (* g_cmd_data_ptr ++ = ((shift<<24) | ((msk_data&0xfff)<<12) | (msked_data&0xfff)))
#define VSP_WRITE_CMD_INFO(cmd_info)	(* g_cmd_info_ptr++ = cmd_info)


void VSP_Write_CMD_by_BSM (uint32 cmd_info_base_phy, uint32 cmd_data_base_phy);
//allow software to access the vsp buffer
 void open_vsp_iram (void);

//allow hardware to access the vsp buffer
 void close_vsp_iram (void);

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef __cplusplus
}
#endif 
/**---------------------------------------------------------------------------*/
// End 
#endif //_VSP_DRV_SC8810P_H_
