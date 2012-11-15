/******************************************************************************
 ** File Name:      vsp_drv_sc8810.h                                         *
 ** Author:         Xiaowei Luo                                               *
 ** DATE:           01/07/2010                                                *
 ** Copyright:      2010 Spreatrum, Incoporated. All Rights Reserved.         *
 ** Description:    														  *
 *****************************************************************************/
/******************************************************************************
 **                   Edit    History                                         *
 **---------------------------------------------------------------------------* 
 ** DATE          NAME            DESCRIPTION                                 * 
 ** 06/11/2009    Xiaowei Luo     Create.                                     *
 *****************************************************************************/
#ifndef _VSP_DRV_SC8810_H_
#define _VSP_DRV_SC8810_H_
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
#include "vsp_vld.h"
#include "vsp_global.h"
#include "vsp_ahbm.h"
#include "vsp_bsm.h"
#include "vsp_dcam.h"
#include "vsp_vlc.h"
#include "vsp_mca.h"
#include "vsp_mea.h"
#include "vsp_dbk.h"
#include "vsp_mbc.h"
#include "vsp_dct.h"
#include "vsp_global_define.h"

#include <linux/ioctl.h>

/*----------------------------------------------------------------------------*
**                            Macro Definitions                               *
**---------------------------------------------------------------------------*/

#define SPRD_VSP_DRIVER "/dev/sprd_vsp"

#ifdef _VSP_LINUX_
#define LOG_TAG "VSP"
#include <utils/Log.h>
#define  SCI_TRACE_LOW   ALOGI
#define SCI_MEMSET  memset
#define SCI_MEMCPY	memcpy
#define SCI_ASSERT(...) 
#endif 

/*76k vsp address space size*/
#define SPRD_VSP_MAP_SIZE 0x13000

#define SPRD_VSP_IOCTL_MAGIC 	'm'
#define VSP_CONFIG_FREQ 		_IOW(SPRD_VSP_IOCTL_MAGIC, 1, unsigned int)
#define VSP_GET_FREQ    			_IOR(SPRD_VSP_IOCTL_MAGIC, 2, unsigned int)
#define VSP_ENABLE      			_IO(SPRD_VSP_IOCTL_MAGIC, 3)
#define VSP_DISABLE     			_IO(SPRD_VSP_IOCTL_MAGIC, 4)
#define VSP_ACQUAIRE    			_IO(SPRD_VSP_IOCTL_MAGIC, 5)
#define VSP_RELEASE     			_IO(SPRD_VSP_IOCTL_MAGIC, 6)
#define VSP_START       			_IO(SPRD_VSP_IOCTL_MAGIC, 7)
#define VSP_RESET       			_IO(SPRD_VSP_IOCTL_MAGIC, 8)

#define TIME_OUT_CLK			0x1fffff //0x1ffff

#define IS_TIME_OUT				1
#define NOT_TIME_OUT			0

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

/*
	Bit define, for video
*/
#define V_BIT_0               0x00000001
#define V_BIT_1               0x00000002
#define V_BIT_2               0x00000004
#define V_BIT_3               0x00000008
#define V_BIT_4               0x00000010
#define V_BIT_5               0x00000020
#define V_BIT_6               0x00000040
#define V_BIT_7               0x00000080
#define V_BIT_8               0x00000100
#define V_BIT_9               0x00000200
#define V_BIT_10              0x00000400
#define V_BIT_11              0x00000800
#define V_BIT_12              0x00001000
#define V_BIT_13              0x00002000
#define V_BIT_14              0x00004000
#define V_BIT_15              0x00008000
#define V_BIT_16              0x00010000
#define V_BIT_17              0x00020000
#define V_BIT_18              0x00040000
#define V_BIT_19              0x00080000
#define V_BIT_20              0x00100000
#define V_BIT_21              0x00200000
#define V_BIT_22              0x00400000
#define V_BIT_23              0x00800000
#define V_BIT_24              0x01000000
#define V_BIT_25              0x02000000
#define V_BIT_26              0x04000000
#define V_BIT_27              0x08000000
#define V_BIT_28              0x10000000
#define V_BIT_29              0x20000000
#define V_BIT_30              0x40000000 
#define V_BIT_31              0x80000000

#ifdef _VSP_LINUX_
void  VSP_SetVirtualBaseAddr(uint32 vsp_Vaddr_base);
#endif

int32 VSP_OPEN_Dev (void);
void VSP_CLOSE_Dev(void);
void VSP_START_CQM(void);
int32 VSP_ACQUIRE_Dev(void);
void VSP_RELEASE_Dev(void);
void configure_huff_tab (uint32 *pHuff_tab, int32 n);
void flush_unalign_bytes (int32 nbytes);

extern uint32 s_vsp_Vaddr_base;

#if !defined(_VSP_)
//PUBLIC void vsp_write_register_normal(uint32 reg_addr, int32 value, char *pstring);
//PUBLIC uint32 vsp_read_register_normal (uint32 reg_addr, int8 *pString);
//PUBLIC int32 vsp_read_reg_poll_normal(uint32 addr, uint32 msk,uint32 exp_value, uint32 time, char *pstring);

//PUBLIC void vsp_write_register_cqm(uint32 reg_addr, int32 value, char *pstring);
//PUBLIC uint32 vsp_read_register_cqm (uint32 reg_addr, int8 *pString);
//PUBLIC void vsp_read_reg_poll_cqm(uint32 reg_addr,uint32 shift, uint32 msk_data,uint32 msked_data, char *pstring);

//PUBLIC void vsp_write_cmd_info(uint32 cmd_info);
#else
PUBLIC  int32 vsp_read_reg_poll_normal(uint32 reg_addr, uint32 msk,uint32 exp_value, uint32 time, char *pstring);

//normal mode
#define VSP_WRITE_REG(reg_addr, value, pstring) *(volatile uint32 *)(reg_addr-VSP_DCAM_BASE+s_vsp_Vaddr_base)  = (value) //vsp_write_register_normal(reg_addr, value, pstring)
#define VSP_READ_REG(reg_addr, pstring)		(*(volatile uint32 *)(reg_addr-VSP_DCAM_BASE+s_vsp_Vaddr_base)) //vsp_read_register_normal(reg_addr, pstring)
#define VSP_READ_REG_POLL(reg_addr, msk, exp_value, time, pstring) vsp_read_reg_poll_normal(reg_addr, msk, exp_value, time, pstring)

//command queue mode
#define VSP_WRITE_REG_CQM(reg_addr, value, pstring) * g_cmd_data_ptr ++ = (value) //vsp_write_register_cqm(reg_addr, value, pstring)
#define VSP_READ_REG_POLL_CQM(reg_addr,shift, msk_data, msked_data, pstring) \
		* g_cmd_data_ptr ++ = ((shift<<24) | ((msk_data&0xfff)<<12) | (msked_data&0xfff)) //vsp_read_reg_poll_cqm(reg_addr,shift, msk_data, msked_data, pstring)
#define VSP_WRITE_CMD_INFO(cmd_info)	* g_cmd_info_ptr++ = cmd_info //vsp_write_cmd_info(cmd_info);
#define WRITE_VLD_CABAC_BFR(addr, value, pstring) *(uint32 *)(addr)  = (value) //write_vld_cabac_bfr(addr, value, pstring)

PUBLIC  int32 READ_REG_MBC_ST0_REG(uint32 addr, uint32 msk,uint32 exp_value, char *pstring);
#endif //!defined(_VSP_)

//allow software to access the vsp buffer
PUBLIC void open_vsp_iram (void);

//allow hardware to access the vsp buffer
PUBLIC void close_vsp_iram (void);

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef __cplusplus
}
#endif 
/**---------------------------------------------------------------------------*/
// End 
#endif //_VSP_DRV_SC8810_H_
