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

LOCAL int32 VSP_set_ddr_freq(const char* freq_in_khz)
{
    const char* const set_freq = "/sys/devices/platform/scxx30-dmcfreq.0/devfreq/scxx30-dmcfreq.0/ondemand/set_freq";
    FILE* fp = fopen(set_freq, "wb");

    if (fp != NULL) {
        fprintf(fp, "%s", freq_in_khz);
        SCI_TRACE_LOW("set ddr freq to %skhz", freq_in_khz);
        fclose(fp);
        return 0;
    } else {
        SCI_TRACE_LOW("Failed to open %s", set_freq);
        return -1;
    }
}

LOCAL int32 VSP_clean_freq(VSPObject *vo)
{
    while(vo->ddr_bandwidth_req_cnt > 0)
    {
        if (VSP_set_ddr_freq("0") < 0)
        {
            SCI_TRACE_LOW("%s, %d", __FUNCTION__, __LINE__);
            return -1;
        }
        vo->ddr_bandwidth_req_cnt --;
    }

    return 0;
}

PUBLIC int32 VSP_CFG_FREQ(VSPObject *vo, uint32 frame_size)
{
    char* ddr_freq;

    if(frame_size > 1280 *720)
    {
        ddr_freq = "500000";
        vo->vsp_freq_div = 0;
    } else if(frame_size > 720*576)
    {
        ddr_freq = "400000";
        vo->vsp_freq_div = 1;
    } else if(frame_size > 320*240)
    {
        ddr_freq = "300000";
        vo->vsp_freq_div = 2;
    } else
    {
        ddr_freq = "200000";
        vo->vsp_freq_div = 3;
    }

    if (VSP_clean_freq(vo) < 0)
    {
        return -1;
    }

    if (VSP_set_ddr_freq(ddr_freq) < 0)
    {
        return -1;
    }
    vo->ddr_bandwidth_req_cnt ++;

    return 0;
}

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

PUBLIC int32 VSP_CLOSE_Dev(VSPObject *vo)
{
    if(vo->s_vsp_fd > 0)
    {
        if (munmap(vo->s_vsp_Vaddr_base + VSP_REG_BASE_ADDR, SPRD_VSP_MAP_SIZE))
        {
            SCI_TRACE_LOW("%s, %d, %d", __FUNCTION__, __LINE__, errno);
            return -1;
        }

        close(vo->s_vsp_fd);
        if (VSP_clean_freq(vo) < 0)
        {
            return -1;
        }
        return 0;
    } else
    {
        SCI_TRACE_LOW ("%s, error", __FUNCTION__);
        return -1;
    }
}

PUBLIC int32 VSP_GET_DEV_FREQ(VSPObject *vo, int32*  vsp_clk_ptr)
{
    if(vo->s_vsp_fd > 0)
    {
        ioctl(vo->s_vsp_fd, VSP_GET_FREQ, vsp_clk_ptr);
        return 0;
    } else
    {
        SCI_TRACE_LOW ("%s, error", __FUNCTION__);
        return -1;
    }
}

PUBLIC int32 VSP_CONFIG_DEV_FREQ(VSPObject *vo,int32*  vsp_clk_ptr)
{
    if(vo->s_vsp_fd > 0)
    {
        ioctl(vo->s_vsp_fd, VSP_CONFIG_FREQ, vsp_clk_ptr);
        return 0;
    } else
    {
        SCI_TRACE_LOW ("%s, error", __FUNCTION__);
        return -1;
    }
}

PUBLIC int32 VSP_POLL_COMPLETE(VSPObject *vo)
{
    if(vo->s_vsp_fd > 0)
    {
        int ret ;

        ioctl(vo->s_vsp_fd, VSP_COMPLETE, &ret);

        SCI_TRACE_LOW("%s, %d, int_ret: %0x", __FUNCTION__, __LINE__, ret);

        return ret;
    } else
    {
        SCI_TRACE_LOW ("%s, error", __FUNCTION__);
        return -1;
    }
}

PUBLIC int32 VSP_ACQUIRE_Dev(VSPObject *vo)
{
    int32 ret ;

    if(vo->s_vsp_fd <  0)
    {
        SCI_TRACE_LOW("%s: failed :fd <  0", __FUNCTION__);
        return -1;
    }

    ret = ioctl(vo->s_vsp_fd, VSP_ACQUAIRE, NULL);
    if(ret)
    {
        SCI_TRACE_LOW("%s: VSP hardware timeout try again %d\n",__FUNCTION__, ret);
        ret =  ioctl(vo->s_vsp_fd, VSP_ACQUAIRE, NULL);
        if(ret)
        {
            SCI_TRACE_LOW("%s: VSP hardware timeout give up %d\n",__FUNCTION__, ret);
            return -1;
        }
    }

    ioctl(vo->s_vsp_fd, VSP_ENABLE, NULL);
    ioctl(vo->s_vsp_fd, VSP_RESET, NULL);

    SCI_TRACE_LOW("%s, %d", __FUNCTION__, __LINE__);

    return 0;
}

PUBLIC int32 VSP_RELEASE_Dev(VSPObject *vo)
{
    if(vo->s_vsp_fd > 0)
    {
        ioctl(vo->s_vsp_fd, VSP_DISABLE, NULL);
        ioctl(vo->s_vsp_fd, VSP_RELEASE, NULL);

        SCI_TRACE_LOW("%s, %d", __FUNCTION__, __LINE__);

        return 0;
    } else
    {
        SCI_TRACE_LOW("%s: failed :fd <  0", __FUNCTION__);
        return -1;
    }
}

PUBLIC int32 ARM_VSP_RST (VSPObject *vo)
{
    if(VSP_ACQUIRE_Dev(vo) < 0)
    {
        return -1;
    }

    {
        // 0:{256000000,"clk_256m"},
        // 1:{192000000,"clk_192p6m"},
        // 2:{128000000,"clk_128m"},
        // 3:{76800000,"clk_76m8"}

        if (VSP_CONFIG_DEV_FREQ(vo,&(vo->vsp_freq_div)))
        {
            return -1;
        }
    }
    VSP_WRITE_REG(AHB_CTRL_BASE_ADDR + ARM_ACCESS_CTRL_OFF, 0,"RAM_ACC_by arm");
    VSP_READ_REG_POLL(AHB_CTRL_BASE_ADDR + ARM_ACCESS_STATUS_OFF, (V_BIT_1 | V_BIT_0), 0x00000000, TIME_OUT_CLK, "ARM_ACCESS_STATUS_OFF");

    VSP_WRITE_REG(AHB_CTRL_BASE_ADDR + ARM_INT_MASK_OFF, 0,"arm int mask set");	// Disable Openrisc interrrupt . TBD.
    VSP_WRITE_REG(GLB_REG_BASE_ADDR + AXIM_ENDIAN_OFF, 0x30828,"axim endian set"); // VSP and OR endian.

    return 0;
}

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
}
#endif
/**---------------------------------------------------------------------------*/
// End
