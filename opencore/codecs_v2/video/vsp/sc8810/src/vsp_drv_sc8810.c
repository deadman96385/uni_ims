/******************************************************************************
 ** File Name:      vsp_drv_sc8810.c                                                 *
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
#include "vsp_drv_sc8810.h"
#if !defined(_VSP_)
#include "common_global.h"
#include "bsm_global.h"
#endif //_CMODEL_

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>

/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef __cplusplus
extern "C"
{
#endif 

uint32 s_vsp_Vaddr_base = 0;
int32 s_vsp_fd = -1;

/*************************************/
/* functions needed for video engine */
/*************************************/
PUBLIC int32 VSP_OPEN_Dev (void)
{
	if (-1 == s_vsp_fd)
	{
	  	if((s_vsp_fd = open(SPRD_VSP_DRIVER,O_RDWR))<0)
	    	{
			return -1;
	    	}else
	    	{
	        	s_vsp_Vaddr_base = (uint32)mmap(NULL,SPRD_VSP_MAP_SIZE,PROT_READ|PROT_WRITE,MAP_SHARED,s_vsp_fd,0);
				s_vsp_Vaddr_base -= VSP_DCAM_BASE;
		}
	}
    		
    	SCI_TRACE_LOW("vsp addr %x\n",s_vsp_Vaddr_base);	

	return 0;
}

PUBLIC void VSP_CLOSE_Dev(void)
{
	if(s_vsp_fd>=0)
	{
		munmap(s_vsp_fd,SPRD_VSP_MAP_SIZE);	
		close(s_vsp_fd);	
	}
}

PUBLIC void VSP_START_CQM (void)
{	
	ioctl(s_vsp_fd,VSP_START,NULL);
}

PUBLIC int VSP_ACQUIRE_Dev(void)
{	
	int ret ;

	ret =  ioctl(s_vsp_fd,VSP_ACQUAIRE,NULL);
	if(ret)
	{
		SCI_TRACE_LOW("avcdec VSP hardware timeout try again %d\n",ret);	
		ret =  ioctl(s_vsp_fd,VSP_ACQUAIRE,NULL);
		if(ret)
		{
			SCI_TRACE_LOW("avcdec VSP hardware timeout give up %d\n",ret);
		 	return 1;
		}		 
	}	
	
	ioctl(s_vsp_fd,VSP_ENABLE,NULL);
	ioctl(s_vsp_fd,VSP_RESET,NULL);

	return 0;
}

PUBLIC void VSP_RELEASE_Dev(void)
{
	ioctl(s_vsp_fd,VSP_DISABLE,NULL);
	ioctl(s_vsp_fd,VSP_RELEASE,NULL);
}

uint32 *g_cmd_data_ptr;
uint32 *g_cmd_info_ptr;
uint32 *g_cmd_data_base;
uint32 *g_cmd_info_base;

PUBLIC  int32 vsp_read_reg_poll_normal(uint32 reg_addr, uint32 msk,uint32 exp_value, int32 time, char *pstring)
{
	while ((*(volatile uint32*)(reg_addr+s_vsp_Vaddr_base) & msk) != exp_value)
	{
		if (time -- < 0)
		{
			return 1;
		}
	}

	return 0;
}

PUBLIC  int32 READ_REG_MBC_ST0_REG(uint32 addr, uint32 msk,uint32 exp_value, char *pstring)
{
	int32 mbc_st0 = VSP_READ_REG(VSP_MBC_REG_BASE+MBC_ST0_OFF, "MBC_ST0: read regist");

	exp_value = mbc_st0 & msk;
	
	return exp_value;	
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

//configure the huffman table
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

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef __cplusplus
}
#endif 
/**---------------------------------------------------------------------------*/
// End 
