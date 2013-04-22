/******************************************************************************
 ** File Name:      hdvsp.h                                         *
 ** Author:         william wei                                               *
 ** DATE:           09/29/2012                                                *
 ** Copyright:      2012 Spreatrum, Incoporated. All Rights Reserved.         *
 ** Description:    														  *
 *****************************************************************************/
/******************************************************************************
 **                   Edit    History                                         *
 **---------------------------------------------------------------------------* 
 ** DATE          NAME            DESCRIPTION                                 * 
 ** 09/29/2012    william wei     Create.                                     *
 *****************************************************************************/
#ifndef _VSP_DRV_SC8830_H_
#define _VSP_DRV_SC8830_H_


//#define _FPGA_TEST_ 

/*----------------------------------------------------------------------------*
**                        Dependencies                                        *
**---------------------------------------------------------------------------*/
#ifndef _FPGA_TEST_
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/ioctl.h>
#endif
#include "mmcodec.h"

/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef __cplusplus
extern "C"
{
#endif 

/*----------------------------------------------------------------------------*
**                            Macro Definitions                               *
**---------------------------------------------------------------------------*/

/* ------------------------------------------------------------------------
** Constants
** ------------------------------------------------------------------------ */
#define SPRD_VSP_MAP_SIZE 0xA000            // 40kbyte
#define SPRD_VSP_DRIVER "/dev/sprd_vsp"

#define SPRD_VSP_IOCTL_MAGIC 	'm'
#define VSP_CONFIG_FREQ 		_IOW(SPRD_VSP_IOCTL_MAGIC, 1, unsigned int)
#define VSP_GET_FREQ    			_IOR(SPRD_VSP_IOCTL_MAGIC, 2, unsigned int)
#define VSP_ENABLE      			_IO(SPRD_VSP_IOCTL_MAGIC, 3)
#define VSP_DISABLE     			_IO(SPRD_VSP_IOCTL_MAGIC, 4)
#define VSP_ACQUAIRE    			_IO(SPRD_VSP_IOCTL_MAGIC, 5)
#define VSP_RELEASE     			_IO(SPRD_VSP_IOCTL_MAGIC, 6)
#define VSP_COMPLETE       			_IO(SPRD_VSP_IOCTL_MAGIC, 7)
#define VSP_RESET       			_IO(SPRD_VSP_IOCTL_MAGIC, 8)
#define VSP_START       			_IO(SPRD_VSP_IOCTL_MAGIC, 9)
#define VSP_ACQUAIRE_MEA_DONE _IO(SPRD_VSP_IOCTL_MAGIC, 10)
#define VSP_ACQUAIRE_MP4ENC_DONE  			_IO(SPRD_VSP_IOCTL_MAGIC, 11)

/* -----------------------------------------------------------------------
** Standard Types
** ----------------------------------------------------------------------- */
#define STREAM_ID_H263				0
#define STREAM_ID_MPEG4				1
#define STREAM_ID_VP8				2
#define STREAM_ID_FLVH263			3
#define STREAM_ID_H264				4
#define STREAM_ID_REAL8				5
#define STREAM_ID_REAL9				6
#define	SHARE_RAM_SIZE 0x100

#define ENC 1
#define DEC 0
/* -----------------------------------------------------------------------
** Control Register Address on ARM
** ----------------------------------------------------------------------- */
#define	VSP_REG_BASE_ADDR 0x60900000//AHB

#define	AHB_CTRL_BASE_ADDR VSP_REG_BASE_ADDR+0x0
#define	SHARE_RAM_BASE_ADDR VSP_REG_BASE_ADDR+0x200
#define	OR_BOOTROM_BASE_ADDR VSP_REG_BASE_ADDR+0x400
#define	GLB_REG_BASE_ADDR VSP_REG_BASE_ADDR+0x1000
#define	BSM_CTRL_REG_BASE_ADDR VSP_REG_BASE_ADDR+0x8000

//ahb ctrl
#define ARM_ACCESS_CTRL_OFF 0x0
#define ARM_ACCESS_STATUS_OFF 0x04
#define MCU_CTRL_SET_OFF 0x08
#define ARM_INT_STS_OFF 0x10        //from OPENRISC
#define ARM_INT_MASK_OFF 0x14
#define ARM_INT_CLR_OFF 0x18
#define ARM_INT_RAW_OFF 0x1C
#define WB_ADDR_SET0_OFF 0x20
#define WB_ADDR_SET1_OFF 0x24

//glb reg
#define VSP_INT_SYS_OFF 0x0             //from VSP
#define VSP_INT_MASK_OFF 0x04
#define VSP_INT_CLR_OFF 0x08
#define VSP_INT_RAW_OFF 0x08
#define AXIM_ENDIAN_OFF 0x10
#define AXIM_PAUSE_OFF 0x18
#define AXIM_STS_OFF 0x1c
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
#define BSM0_FRM_ADDR_OFF 0x60
//#define BSM1_FRM_ADDR_OFF 0x64
#define VSP_ADDRIDX0_OFF 0x68
#define VSP_ADDRIDX1_OFF 0x6c
#define VSP_ADDRIDX2_OFF 0x70
#define VSP_ADDRIDX3_OFF 0x74
#define VSP_ADDRIDX4_OFF 0x78
#define VSP_ADDRIDX5_OFF 0x7c
#define MCU_START_OFF 0x80//ro
#define CLR_START_OFF 0x84//wo
#define MCU_SLEEP_OFF 0x88//wo

/* -----------------------------------------------------------------------
** Structs
** ----------------------------------------------------------------------- */

/* -----------------------------------------------------------------------
** Global
** ----------------------------------------------------------------------- */
extern uint32 * OR_addr_ptr;

extern uint32 s_vsp_Vaddr_base;
extern int32 s_vsp_fd ;

#define VSP_WRITE_REG(reg_addr, value, pstring) (*(volatile uint32 *)(reg_addr-VSP_REG_BASE_ADDR +s_vsp_Vaddr_base)  = (value))   
#define VSP_READ_REG(reg_addr, pstring)	((*(volatile uint32 *)(reg_addr-VSP_REG_BASE_ADDR+s_vsp_Vaddr_base)))
#define VSP_READ_REG_POLL(reg_addr, msk_data, msked_data, pstring) \
{\
	uint32 tmp;\
	tmp=(*((volatile uint32*)(reg_addr-VSP_REG_BASE_ADDR+s_vsp_Vaddr_base)))&msk_data;\
    while(tmp != msked_data)\
{\
	tmp=(*((volatile uint32*)(reg_addr)))&msk_data;\
}\
}
/**---------------------------------------------------------------------------
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef __cplusplus
}
#endif 
/**---------------------------------------------------------------------------*/
// End 
#endif  //_VSP_DRV_SC8830_H_

