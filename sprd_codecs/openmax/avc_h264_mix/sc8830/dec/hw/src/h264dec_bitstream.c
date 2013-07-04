/******************************************************************************
 ** File Name:    h264dec_bitstream.c                                         *
 ** Author:       Xiaowei Luo                                                 *
 ** DATE:         01/23/2007                                                  *
 ** Copyright:    2006 Spreatrum, Incoporated. All Rights Reserved.           *
 ** Description:                                                              *
 *****************************************************************************/
/******************************************************************************
 **                   Edit    History                                         *
 **---------------------------------------------------------------------------*
 ** DATE          NAME            DESCRIPTION                                 *
 ** 01/23/2007    Xiaowei Luo     Create.                                     *
 *****************************************************************************/
/*----------------------------------------------------------------------------*
**                        Dependencies                                        *
**---------------------------------------------------------------------------*/
#include "h264dec_video_header.h"
/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
extern   "C"
{
#endif

PUBLIC uint32 show_bits (H264DecObject *vo, int32 nbits)
{
    uint32 temp;

    VSP_READ_REG_POLL(BSM_CTRL_REG_BASE_ADDR+BSM_RDY_OFF, 0x00000001,0x00000001,TIME_OUT_CLK, "BSM_rdy");
    VSP_WRITE_REG(BSM_CTRL_REG_BASE_ADDR+BSM_OP_OFF, (nbits<<24),"BSM_rd n bits");
    temp = VSP_READ_REG(BSM_CTRL_REG_BASE_ADDR+BSM_RDATA_OFF,"BSM_rd dara");

    return temp;
}

PUBLIC uint32 read_bits (H264DecObject *vo, int32 nbits)
{
    uint32 temp;

    VSP_READ_REG_POLL(BSM_CTRL_REG_BASE_ADDR+BSM_RDY_OFF, 0x00000001,0x00000001,TIME_OUT_CLK, "BSM_rdy");
    VSP_WRITE_REG(BSM_CTRL_REG_BASE_ADDR+BSM_OP_OFF, (nbits<<24),"BSM_rd n bits");
    temp = VSP_READ_REG(BSM_CTRL_REG_BASE_ADDR+BSM_RDATA_OFF,"BSM_rd dara");
    VSP_WRITE_REG(BSM_CTRL_REG_BASE_ADDR+BSM_OP_OFF, (nbits<<24)|0x1,"BSM_flush n bits");

    return temp;
}

PUBLIC uint32 ue_v (H264DecObject *vo)
{
    uint32 ret;
    uint32 tmp;
    VSP_READ_REG_POLL(BSM_CTRL_REG_BASE_ADDR+BSM_RDY_OFF, 0x00000001,0x00000001,TIME_OUT_CLK, "BSM_rdy");
    VSP_WRITE_REG(BSM_CTRL_REG_BASE_ADDR+USEV_RD_OFF, 1,"BSM_rd_UE cmd");
    tmp = VSP_READ_REG(BSM_CTRL_REG_BASE_ADDR+USEV_RD_OFF,"BSM_rd_UE check error");
    if(tmp&4)
    {
        vo->error_flag = TRUE;
        return 0;
    }
    ret = VSP_READ_REG(BSM_CTRL_REG_BASE_ADDR+USEV_RDATA_OFF,"BSM_rd_UE dara");

    return ret;
}

PUBLIC int32 se_v (H264DecObject *vo)
{
    int32 ret;
    int32 tmp;

    VSP_READ_REG_POLL(BSM_CTRL_REG_BASE_ADDR+BSM_RDY_OFF, 0x00000001,0x00000001,TIME_OUT_CLK, "BSM_rdy");
    VSP_WRITE_REG(BSM_CTRL_REG_BASE_ADDR+USEV_RD_OFF, 2,"BSM_rd_SE cmd");
    tmp = VSP_READ_REG(BSM_CTRL_REG_BASE_ADDR+USEV_RD_OFF,"BSM_rd_SE check error");
    if(tmp&4)
    {
        vo->error_flag = TRUE;
        return 0;
    }
    ret = (int32)VSP_READ_REG(BSM_CTRL_REG_BASE_ADDR+USEV_RDATA_OFF,"BSM_rd_SE dara");

    return ret;
}

PUBLIC int32 long_ue_v (H264DecObject *vo)
{
    uint32 tmp;
    int32 leading_zero = 0;
    int32 ret;

    tmp = SHOW_FLC (16);
    if (tmp == 0)
    {
        READ_FLC (16);
        leading_zero = 16;

        do {
            tmp = READ_FLC (1);
            leading_zero++;

            if (leading_zero > 32)//weihu ?//>=16
            {
                vo->error_flag = TRUE;
                return 0;
            }
        } while(!tmp);

        leading_zero--;
        tmp = READ_FLC (leading_zero);

        ret = (1 << leading_zero) + tmp - 1;

        return ret;
    } else
    {
        return UE_V ();
    }
}

PUBLIC int32 long_se_v (H264DecObject *vo)
{
    uint32 tmp;
    int32 leading_zero = 0;
    int32 ret;

    tmp = SHOW_FLC (16);

    if (tmp == 0)
    {
        READ_FLC (16);
        leading_zero = 16;

        do {
            tmp = READ_FLC (1);
            leading_zero++;

            if (leading_zero > 32)//weihu ?//>=16
            {
                vo->error_flag = TRUE;
                return 0;
            }
        } while(!tmp);

        leading_zero--;
        tmp = READ_FLC (leading_zero);
        tmp = (1 << leading_zero) + tmp - 1;
        ret = (tmp + 1) / 2;

        if ( (tmp & 1) == 0 )
            ret = -ret;

        return ret;
    } else
    {
        return SE_V ();
    }
}

PUBLIC void H264Dec_flush_left_byte (H264DecObject *vo)
{
    int32 i;
    int32 dec_len;
    int32 left_bytes;
    uint32 nDecTotalBits;// = stream->bitcnt;

    VSP_READ_REG_POLL(BSM_CTRL_REG_BASE_ADDR+BSM_RDY_OFF, 0x00000001,0x00000001,TIME_OUT_CLK, "BSM_rdy");
    VSP_WRITE_REG(BSM_CTRL_REG_BASE_ADDR+BSM_OP_OFF, 8,"BSM byte align");
    VSP_READ_REG_POLL(BSM_CTRL_REG_BASE_ADDR+BSM_RDY_OFF, 0x00000001,0x00000001,TIME_OUT_CLK, "BSM_rdy");
    nDecTotalBits =VSP_READ_REG(BSM_CTRL_REG_BASE_ADDR+TOTAL_BITS_OFF,"BSM flushed bits cnt");
    //bytealign
    dec_len = nDecTotalBits>>3;
    left_bytes = vo->g_nalu_ptr->len - dec_len;

    for (i = 0; i < left_bytes; i++)
    {
        READ_FLC(8);
    }

    return;
}

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
}
#endif
/**---------------------------------------------------------------------------*/
// End

