/******************************************************************************
 ** File Name:    hdvsp.c
 *
 ** Author:       Simon.Wang                                                  *
 ** DATE:         03/07/2013                                                  *
 ** Copyright:    2010 Spreatrum, Incoporated. All Rights Reserved.           *
 ** Description:                                                              *
 *****************************************************************************/
/******************************************************************************
 **                   Edit    History                                         *
 **---------------------------------------------------------------------------* 
 ** DATE          NAME            DESCRIPTION                                 * 
 ** 03/07/2010    Simon.Wang      Create.                                     *
 *****************************************************************************/
/*----------------------------------------------------------------------------*
**                        Dependencies                                        *
**---------------------------------------------------------------------------*/
#include "mmcodec.h"
#include "vsp_drv_sc8830.h"
#include "vsp_or_data.h"

/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    extern   "C" 
    {
#endif

int32 s_vsp_fd = -1;

#ifndef _FPGA_TEST_
uint32 s_vsp_Vaddr_base = 0;

PUBLIC int32 VSP_OPEN_Dev ()
{
 	int ret =0;
	
	if (-1 == s_vsp_fd)
	{
		SCI_TRACE_LOW("open SPRD_VSP_DRIVER ");	
	  	if((s_vsp_fd = open(SPRD_VSP_DRIVER,O_RDWR))<0)
	    	{
	    		SCI_TRACE_LOW("open SPRD_VSP_DRIVER ERR");	
			return -1;
			
	    	}else
	    	{
	        	s_vsp_Vaddr_base = (uint32)mmap(NULL,SPRD_VSP_MAP_SIZE,PROT_READ|PROT_WRITE,MAP_SHARED,s_vsp_fd,0);
		}
	}

    	SCI_TRACE_LOW("vsp addr %x\n",s_vsp_Vaddr_base);	

	return 0;
}

PUBLIC void VSP_CLOSE_Dev()
{

	int ret = 0;

	if(s_vsp_fd > 0)
	{	
		munmap(s_vsp_fd,SPRD_VSP_MAP_SIZE);	
		close(s_vsp_fd);	
	}
}

PUBLIC void VSP_RESET_Dev(void)
{
	ioctl(s_vsp_fd,VSP_RESET,NULL);
}

PUBLIC void VSP_START_Dev(void)
{
	ioctl(s_vsp_fd,VSP_START,NULL);
}

PUBLIC int VSP_ACQUIRE_Dev(void)
{	
	int ret ;

	if(s_vsp_fd <  0)
	{
		SCI_TRACE_LOW("VSP_ACQUIRE_Dev failed :fd <  0");	
	}
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
	
	return 0;
}

PUBLIC void VSP_RELEASE_Dev(void)
{
	if(s_vsp_fd > 0)
	{
		ioctl(s_vsp_fd,VSP_RELEASE,NULL);
	}
}
#else
uint32 s_vsp_Vaddr_base = VSP_REG_BASE_ADDR;
#endif 

uint32 * OR_addr_ptr  = NULL;
/*************************************/
/* functions needed for video engine */
/*************************************/
PUBLIC int32 OR_VSP_RST ()
{
	int i;
#ifndef _FPGA_TEST_
	// Acqurire VSP device. 
	if(VSP_ACQUIRE_Dev()<0)
	{
		return MMDEC_HW_ERROR;
	}		

#endif

	//Boot ram, share ram, global regiser are accessed by ARM.
	VSP_WRITE_REG(AHB_CTRL_BASE_ADDR+ARM_ACCESS_CTRL_OFF, 0,"RAM_ACC_by arm");
	VSP_READ_REG_POLL(AHB_CTRL_BASE_ADDR+ARM_ACCESS_STATUS_OFF, 0x00000003, 0x00000000, "ARM_ACCESS_STATUS_OFF");
	
	// Load bootcode to boot ram.
	for(i=0;i<BOOT_CODE_SIZE;i++)
		VSP_WRITE_REG(OR_BOOTROM_BASE_ADDR+i*4, bootcode[i],"boot code");

#ifndef _FPGA_TEST_
	VSP_RESET_Dev();
#else
	VSP_WRITE_REG(0x60d01004, (1<<11)|(1<<4),"openrisc rst");	
	VSP_WRITE_REG(0x60d02004, (1<<4),"openrisc rst");	//Reset VSP 
#endif

	// Clean Share Ram.
	for(i=0; i<(SHARE_RAM_SIZE/4);i++)
		VSP_WRITE_REG(SHARE_RAM_BASE_ADDR + i*4, 0, "Clean Share Ram");

	//Boot ram, share ram, global regiser are accessed by ARM.
	VSP_WRITE_REG(AHB_CTRL_BASE_ADDR+ARM_ACCESS_CTRL_OFF, 0,"RAM_ACC_by arm");
	VSP_READ_REG_POLL(AHB_CTRL_BASE_ADDR+ARM_ACCESS_STATUS_OFF, 0x00000003, 0x00000000, "ARM_ACCESS_STATUS_OFF");

	//Config Openrisc running space.
    	VSP_WRITE_REG(AHB_CTRL_BASE_ADDR+WB_ADDR_SET0_OFF, (((uint32)OR_addr_ptr)>>3),"Iwb_addr_base");
	VSP_WRITE_REG(AHB_CTRL_BASE_ADDR+WB_ADDR_SET1_OFF, (((uint32)OR_addr_ptr)>>3),"Dwb_addr_base");

	
    	VSP_WRITE_REG(AHB_CTRL_BASE_ADDR+ARM_INT_MASK_OFF, 0,"arm int mask set");	// Disable Openrisc interrrupt . TBD.	    	
	VSP_WRITE_REG(GLB_REG_BASE_ADDR+AXIM_ENDIAN_OFF, 0x30828,"axim endian set"); // VSP and OR endian.

	//config share ram
	VSP_WRITE_REG(SHARE_RAM_BASE_ADDR+0x54, ((uint32)OR_addr_ptr),"or_addr_offset");//OPENRISC text+heap+stack
	VSP_WRITE_REG(SHARE_RAM_BASE_ADDR+0xfc, 0x12345678,"boot code jump to main enable");

	return MMDEC_OK;

}

PUBLIC int32 OR_VSP_START ()
{
	int32 ret;
	// Start OR clk to boot. 		
	VSP_WRITE_REG(AHB_CTRL_BASE_ADDR+ARM_ACCESS_CTRL_OFF, 7,"RAM_ACC_by arm");
	VSP_READ_REG_POLL(AHB_CTRL_BASE_ADDR+ARM_ACCESS_STATUS_OFF, 0x00000003, 0x00000003, "ARM_ACCESS_STATUS_OFF");
	

	VSP_WRITE_REG(AHB_CTRL_BASE_ADDR+MCU_CTRL_SET_OFF, 1,"VSP clk gate en");//3//weihu chip need
        VSP_START_Dev();			
			
	VSP_READ_REG_POLL(AHB_CTRL_BASE_ADDR+ARM_INT_RAW_OFF, 0x00000001,0x00000001,"ARM INT MCU_done");
	VSP_WRITE_REG(AHB_CTRL_BASE_ADDR+ARM_INT_CLR_OFF, 7,"clr arm int");
	VSP_WRITE_REG(AHB_CTRL_BASE_ADDR+MCU_CTRL_SET_OFF, 0,"VSP clk gate");//3

	VSP_WRITE_REG(AHB_CTRL_BASE_ADDR+ARM_ACCESS_CTRL_OFF, 0,"RAM_ACC_by arm");
	VSP_READ_REG_POLL(AHB_CTRL_BASE_ADDR+ARM_ACCESS_STATUS_OFF, 0x00000003, 0x00000000, "ARM_ACCESS_STATUS_OFF");
	VSP_WRITE_REG(GLB_REG_BASE_ADDR+AXIM_PAUSE_OFF, 3,"AXIM_PAUSE");
	VSP_READ_REG_POLL(GLB_REG_BASE_ADDR+AXIM_STS_OFF, 0x00000001,0x00000000,"AXIM_STS");

	// Get output of OR function
	ret = VSP_READ_REG(SHARE_RAM_BASE_ADDR+0x5c," Get output of OR function");

	return ret;
}
	
/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End 
