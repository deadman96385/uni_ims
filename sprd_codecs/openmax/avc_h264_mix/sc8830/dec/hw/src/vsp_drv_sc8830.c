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

PUBLIC int32 VSP_CFG_FREQ(VSPObject *vo, uint32 frame_size)
{
    if(frame_size > 1280 *720)
    {
        vo->vsp_freq_div = 0;
    } else if(frame_size > 720*576)
    {
        vo->vsp_freq_div = 1;
    } else if(frame_size > 320*240)
    {
        vo->vsp_freq_div = 2;
    } else
    {
        vo->vsp_freq_div = 3;
    }

    return 0;
}

PUBLIC int32 VSP_OPEN_Dev (VSPObject *vo)
{
    int ret =0;

    if (-1 == vo->s_vsp_fd)
    {
        SCI_TRACE_LOW("open SPRD_VSP_DRIVER ");
        if((vo->s_vsp_fd = open(SPRD_VSP_DRIVER,O_RDWR)) < 0)
        {
            SCI_TRACE_LOW("open SPRD_VSP_DRIVER ERR");
            return -1;
        } else
        {
            vo->s_vsp_Vaddr_base = (uint_32or64)mmap(NULL,SPRD_VSP_MAP_SIZE,PROT_READ|PROT_WRITE,MAP_SHARED, vo->s_vsp_fd,0);
            vo->s_vsp_Vaddr_base -= VSP_REG_BASE_ADDR;
        }

        ioctl(vo->s_vsp_fd, VSP_CAPABILITY, &(vo->vsp_capability));
    }

    SCI_TRACE_LOW("%s, %d, vsp addr %lx, vsp_capability: %d\n",__FUNCTION__, __LINE__, vo->s_vsp_Vaddr_base, vo->vsp_capability);

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
        int ret = ioctl(vo->s_vsp_fd, VSP_CONFIG_FREQ, vsp_clk_ptr);
        if (ret < 0)
        {
            SCI_TRACE_LOW ("%s, VSP_CONFIG_FREQ failed", __FUNCTION__);
            return -1;
        }
    } else
    {
        SCI_TRACE_LOW ("%s, error", __FUNCTION__);
        return -1;
    }

    return 0;
}

#define MAX_POLL_CNT    10
PUBLIC int32 VSP_POLL_COMPLETE(VSPObject *vo)
{
    if(vo->s_vsp_fd > 0)
    {
        int32 ret;
        int32 cnt = 0;

        do
        {
            ioctl(vo->s_vsp_fd, VSP_COMPLETE, &ret);
            cnt++;
        } while ((ret & V_BIT_30) &&  (cnt < MAX_POLL_CNT));
        if(!(V_BIT_1 == ret || V_BIT_2 == ret ))
        {
            SCI_TRACE_LOW("%s, %d, int_ret: %0x", __FUNCTION__, __LINE__, ret);
        }

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
#if 0
        SCI_TRACE_LOW("%s: VSP_ACQUAIRE failed,  try again %d\n",__FUNCTION__, ret);
        ret =  ioctl(vo->s_vsp_fd, VSP_ACQUAIRE, NULL);
        if(ret < 0)
        {
            SCI_TRACE_LOW("%s: VSP_ACQUAIRE failed, give up %d\n",__FUNCTION__, ret);
            return -1;
        }
#else
        SCI_TRACE_LOW("%s: VSP_ACQUAIRE failed,  %d\n",__FUNCTION__, ret);
        return -1;
#endif
    }

    //shark:  [0]:"clk_256m", [1]:"clk_192m",   [2]:"clk_128m", [3]:"clk_76m8"
    //dolphin [0]:"clk_192m", [1]:"clk_153m6", [2]:"clk_128m", [3]:"clk_76m8"
    if (VSP_CONFIG_DEV_FREQ(vo,&(vo->vsp_freq_div)))
    {
        return -1;
    }

    ret = ioctl(vo->s_vsp_fd, VSP_ENABLE, NULL);
    if (ret < 0)
    {
        SCI_TRACE_LOW("%s: VSP_ENABLE failed %d\n",__FUNCTION__, ret);
        return -1;
    }

    ret = ioctl(vo->s_vsp_fd, VSP_RESET, NULL);
    if (ret < 0)
    {
        SCI_TRACE_LOW("%s: VSP_RESET failed %d\n",__FUNCTION__, ret);
        return -1;
    }

    SCI_TRACE_LOW("%s, %d", __FUNCTION__, __LINE__);

    return 0;
}

PUBLIC int32 VSP_RELEASE_Dev(VSPObject *vo)
{
    int ret = 0;

    if(vo->s_vsp_fd > 0)
    {
        if (vo->error_flag)
        {
            VSP_WRITE_REG(GLB_REG_BASE_ADDR + AXIM_PAUSE_OFF, 0x1, "AXIM_PAUSE: stop DATA TRANS to AXIM");
            usleep(1);
        }

        if (VSP_READ_REG_POLL(GLB_REG_BASE_ADDR + AXIM_STS_OFF, V_BIT_1, 0x0, TIME_OUT_CLK_FRAME, "Polling AXIM_STS: not Axim_wch_busy")) //check all data has written to DDR
        {
            SCI_TRACE_LOW("%s, %d, Axim_wch_busy", __FUNCTION__, __LINE__);
        }

        if (VSP_READ_REG_POLL(GLB_REG_BASE_ADDR + AXIM_STS_OFF, V_BIT_2, 0x0, TIME_OUT_CLK_FRAME, "Polling AXIM_STS: not Axim_rch_busy"))
        {
            SCI_TRACE_LOW("%s, %d, Axim_wch_busy", __FUNCTION__, __LINE__);
        }

        ret = ioctl(vo->s_vsp_fd, VSP_RESET, NULL);
        if (ret < 0)
        {
            SCI_TRACE_LOW("%s: VSP_RESET failed %d\n",__FUNCTION__, ret);
            ret = -1;
            goto RELEASE_END;
        }

        ret = ioctl(vo->s_vsp_fd, VSP_DISABLE, NULL);
        if(ret < 0)
        {
            SCI_TRACE_LOW("%s: VSP_DISABLE failed, %d\n",__FUNCTION__, ret);
            ret = -1;
            goto RELEASE_END;
        }

        ret = ioctl(vo->s_vsp_fd, VSP_RELEASE, NULL);
        if(ret < 0)
        {
            SCI_TRACE_LOW("%s: VSP_RELEASE failed, %d\n",__FUNCTION__, ret);
            ret = -1;
            goto RELEASE_END;
        }
    } else
    {
        SCI_TRACE_LOW("%s: failed :fd <  0", __FUNCTION__);
        ret = -1;
    }

RELEASE_END:

    if (vo->error_flag || (ret < 0))
    {
        usleep(20*1000);
    }

    SCI_TRACE_LOW("%s, %d, ret: %d", __FUNCTION__, __LINE__, ret);

    return ret;
}

PUBLIC int32 ARM_VSP_RST (VSPObject *vo)
{
    if(VSP_ACQUIRE_Dev(vo) < 0)
    {
        return -1;
    }

    VSP_WRITE_REG(AHB_CTRL_BASE_ADDR + ARM_ACCESS_CTRL_OFF, 0,"RAM_ACC_by arm");
    if (VSP_READ_REG_POLL(AHB_CTRL_BASE_ADDR + ARM_ACCESS_STATUS_OFF, (V_BIT_1 | V_BIT_0), 0x00000000, TIME_OUT_CLK, "ARM_ACCESS_STATUS_OFF"))
    {
        return -1;
    }

    VSP_WRITE_REG(AHB_CTRL_BASE_ADDR + ARM_INT_MASK_OFF, 0,"arm int mask set");	// Disable Openrisc interrrupt . TBD.
    VSP_WRITE_REG(GLB_REG_BASE_ADDR + AXIM_ENDIAN_OFF, 0x30828,"axim endian set"); // VSP and OR endian.

    return 0;
}

PUBLIC  int32 vsp_read_reg_poll(VSPObject *vo, uint32 reg_addr, uint32 msk_data,uint32 exp_value, uint32 time, char *pstring)
{
    uint32 tmp, vsp_time_out_cnt = 0;

    tmp=(*((volatile uint32*)(reg_addr+((VSPObject *)vo)->s_vsp_Vaddr_base)))&msk_data;
    while((tmp != exp_value) && (vsp_time_out_cnt < time))
    {
        tmp=(*((volatile uint32*)(reg_addr+((VSPObject *)vo)->s_vsp_Vaddr_base)))&msk_data;
        vsp_time_out_cnt++;
    }

    if (vsp_time_out_cnt >= time)
    {
        uint32 mm_eb_reg;

        ioctl(vo->s_vsp_fd, VSP_HW_INFO, &mm_eb_reg);
        vo->error_flag |= ER_HW_ID;
        SCI_TRACE_LOW ("vsp_time_out_cnt %d, MM_CLK_REG (0x402e0000): 0x%0x",vsp_time_out_cnt, mm_eb_reg);
        return 1;
    }

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
