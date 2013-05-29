/******************************************************************************
 ** File Name:      vsp_drv_sc8800g.h                                         *
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
#ifndef _VSP_DRV_SC8800G_H_
#define _VSP_DRV_SC8800G_H_
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
#ifdef SIM_IN_WIN
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
#include "vsp_ip_syntax.h"
#endif
#include "vsp_global_define.h"

/*----------------------------------------------------------------------------*
**                            Macro Definitions                               *
**---------------------------------------------------------------------------*/

#ifdef SIM_IN_WIN
#define TIME_OUT_CLK			0xffff

#define IS_TIME_OUT				1
#define NOT_TIME_OUT			0

/*standard*/
typedef enum {
		VSP_ITU_H263 = 0, 
		VSP_MPEG4,  
		VSP_JPEG,
		VSP_FLV_V1,
		VSP_H264
		}VIDEO_STD_E;

#define VSP_INTRA		0
#define VSP_INTER		1

#define VSP_STD_ZIGZAG		0
#define VSP_HOR_ZIGZAG		1
#define VSP_VER_ZIGZAG		2

PUBLIC void VSP_Reset (void);
PUBLIC void configure_huff_tab (uint32 *pHuff_tab, int32 n);
PUBLIC void flush_unalign_bytes (int32 nbytes);
PUBLIC void open_vsp_iram (void);
PUBLIC void close_vsp_iram (void);


#if !defined(_VSP_)
PUBLIC void vsp_write_register(uint32 reg_addr, int32 value/*, char *pstring*/);
PUBLIC uint32 vsp_read_register (uint32 reg_addr/*, int8 *pString*/);
PUBLIC int32 vsp_read_reg_poll(uint32 addr, uint32 msk,uint32 exp_value, uint32 time/*, char *pstring*/);
#else
PUBLIC __inline void vsp_write_register(uint32 reg_addr, int32 value/*, char *pstring*/)	
{
	*(volatile uint32 *)(reg_addr)  = (value);
}

PUBLIC __inline uint32 vsp_read_register(uint32 reg_addr/*, char *pstring*/)	
{
	return (*(volatile uint32 *)(reg_addr));
}

PUBLIC __inline int32 vsp_read_reg_poll(uint32 reg_addr, uint32 msk,uint32 exp_value, uint32 time/*, char *pstring*/)
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

PUBLIC __inline int32 READ_REG_MBC_ST0_REG(uint32 addr, uint32 msk,uint32 exp_value, char *pstring)
{
	int32 mbc_st0 = VSP_READ_REG(VSP_MBC_REG_BASE+MBC_ST0_OFF, "MBC_ST0: read regist");

	exp_value = mbc_st0 & msk;
	
	return exp_value;	
}
#endif //!defined(_VSP_)

#define VSP_WRITE_REG(reg_addr, value, pstring) vsp_write_register(reg_addr, value)
#define VSP_READ_REG(reg_addr, pstring)		vsp_read_register(reg_addr)
#define READ_REG_POLL(reg_addr, msk, exp_value, time, pstring) vsp_read_reg_poll(reg_addr, msk, exp_value, time)

#endif //SIM_IN_WIN
/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef __cplusplus
}
#endif 
/**---------------------------------------------------------------------------*/
// End 
#endif //_VSP_DRV_SC8800G_H_
