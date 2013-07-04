/******************************************************************************
 ** File Name:    vsp_drv_sc8830.c
 *
 ** Author:       Xiaowei Luo                                                  *
 ** DATE:         06/20/2013                                                  *
 ** Copyright:    2013 Spreatrum, Incoporated. All Rights Reserved.           *
 ** Description:                                                              *
 *****************************************************************************/
/******************************************************************************
 **                   Edit    History                                         *
 **---------------------------------------------------------------------------*
 ** DATE          NAME            DESCRIPTION                                 *
 ** 06/20/2013    Xiaowei Luo      Create.                                     *
 *****************************************************************************/
/*----------------------------------------------------------------------------*
**                        Dependencies                                        *
**---------------------------------------------------------------------------*/
#include "mmcodec.h"
#include "vsp_drv_sc8830.h"

/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
extern   "C"
{
#endif

PUBLIC int32 VSP_OPEN_Dev (VSPObject *vo)
{
    int ret =0;

    if (-1 == vo->s_vsp_fd)
    {
        SCI_TRACE_LOW("open SPRD_VSP_DRIVER ");
        if((vo->s_vsp_fd = open(SPRD_VSP_DRIVER,O_RDWR))<0)
        {
            SCI_TRACE_LOW("open SPRD_VSP_DRIVER ERR");
            return -1;

        } else
        {
            vo->s_vsp_Vaddr_base = (uint32)mmap(NULL,SPRD_VSP_MAP_SIZE,PROT_READ|PROT_WRITE,MAP_SHARED, vo->s_vsp_fd,0);
            vo->s_vsp_Vaddr_base -= VSP_REG_BASE_ADDR;
        }
    }

    SCI_TRACE_LOW("%s, %d, vsp addr %x\n",__FUNCTION__, __LINE__, vo->s_vsp_Vaddr_base);

    return 0;
}

PUBLIC void VSP_CLOSE_Dev(VSPObject *vo)
{
    int ret = 0;

    if(vo->s_vsp_fd > 0)
    {
        munmap(vo->s_vsp_fd,SPRD_VSP_MAP_SIZE);
        close(vo->s_vsp_fd);
    }
}

PUBLIC int32 VSP_POLL_COMPLETE(VSPObject *vo)
{
    int ret ;

    ioctl(vo->s_vsp_fd,VSP_COMPLETE,&ret);

    SCI_TRACE_LOW("%s, %d, tmp1: %0x", __FUNCTION__, __LINE__, ret);

    return ret;
}

PUBLIC int VSP_ACQUIRE_Dev(VSPObject *vo)
{
    int ret ;

    if(vo->s_vsp_fd <  0)
    {
        SCI_TRACE_LOW("VSP_ACQUIRE_Dev failed :fd <  0");
    }

    ret =  ioctl(vo->s_vsp_fd,VSP_ACQUAIRE,NULL);
    if(ret)
    {
        SCI_TRACE_LOW("avcdec VSP hardware timeout try again %d\n",ret);
        ret =  ioctl(vo->s_vsp_fd,VSP_ACQUAIRE,NULL);
        if(ret)
        {
            SCI_TRACE_LOW("avcdec VSP hardware timeout give up %d\n",ret);
            return 1;
        }
    }

    ioctl(vo->s_vsp_fd,VSP_ENABLE,NULL);
    ioctl(vo->s_vsp_fd,VSP_RESET,NULL);

    SCI_TRACE_LOW("%s, %d", __FUNCTION__, __LINE__);

    return 0;
}

PUBLIC void VSP_RELEASE_Dev(VSPObject *vo)
{
    if(vo->s_vsp_fd > 0)
    {
        ioctl(vo->s_vsp_fd,VSP_DISABLE,NULL);
        ioctl(vo->s_vsp_fd,VSP_RELEASE,NULL);

        SCI_TRACE_LOW("%s, %d", __FUNCTION__, __LINE__);
    }
}

PUBLIC int32 ARM_VSP_RST (VSPObject *vo)
{
    if(VSP_ACQUIRE_Dev(vo)<0)
    {
        return MMDEC_HW_ERROR;
    }

    VSP_WRITE_REG(AHB_CTRL_BASE_ADDR+ARM_ACCESS_CTRL_OFF, 0,"RAM_ACC_by arm");
    VSP_READ_REG_POLL(AHB_CTRL_BASE_ADDR+ARM_ACCESS_STATUS_OFF, 0x00000003, 0x00000000, TIME_OUT_CLK, "ARM_ACCESS_STATUS_OFF");

    VSP_WRITE_REG(AHB_CTRL_BASE_ADDR+ARM_INT_MASK_OFF, 0,"arm int mask set");	// Disable Openrisc interrrupt . TBD.
    VSP_WRITE_REG(GLB_REG_BASE_ADDR+AXIM_ENDIAN_OFF, 0x30828,"axim endian set"); // VSP and OR endian.

    return MMDEC_OK;
}

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
}
#endif
/**---------------------------------------------------------------------------*/
// End
