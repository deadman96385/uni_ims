/******************************************************************************
 ** File Name:    h264enc_bitstrm.c                                           *
 ** Author:       Xiaowei Luo                                                 *
 ** DATE:         06/18/2013                                                  *
 ** Copyright:    2013 Spreatrum, Incoporated. All Rights Reserved.           *
 ** Description:                                                              *
 *****************************************************************************/
/******************************************************************************
 **                   Edit    History                                         *
 **---------------------------------------------------------------------------*
 ** DATE          NAME            DESCRIPTION                                 *
 ** 06/18/2013    Xiaowei Luo     Create.                                     *
 *****************************************************************************/
/*----------------------------------------------------------------------------*
**                        Dependencies                                        *
**---------------------------------------------------------------------------*/
#include "h264enc_video_header.h"

/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
extern   "C"
{
#endif

int32 i_size0_255[256] =
{
    1, 1, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
    6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8
};

PUBLIC int32 write_ue_v(H264EncObject *vo, uint32 val)
{
    if (val == 0)
    {
        VSP_READ_REG_POLL(BSM_CTRL_REG_BASE_ADDR + BSM_RDY_OFF, V_BIT_0, V_BIT_0, TIME_OUT_CLK, "Polling BSM_RDY");
        VSP_WRITE_REG(BSM_CTRL_REG_BASE_ADDR + BSM_OP_OFF, (1&0x3f) << 24, "BSM_OPERATE: Set OPT_BITS");
        VSP_WRITE_REG(BSM_CTRL_REG_BASE_ADDR + BSM_WDATA_OFF, 1, "BSM_WDATA");
    } else
    {
        int32 i_size = 0;
        uint32 tmp = ++val;

        if (tmp >= V_BIT_16)
        {
            i_size += 16;
            tmp >>= 16;
        }

        if (tmp >= V_BIT_8)
        {
            i_size += 8;
            tmp >>= 8;
        }

        i_size += i_size0_255[tmp];

        VSP_READ_REG_POLL(BSM_CTRL_REG_BASE_ADDR + BSM_RDY_OFF, V_BIT_0, V_BIT_0, TIME_OUT_CLK, "Polling BSM_RDY");
        VSP_WRITE_REG(BSM_CTRL_REG_BASE_ADDR + BSM_OP_OFF, ((2*i_size-1)&0x3f) << 24, "BSM_OPERATE: Set OPT_BITS");
        VSP_WRITE_REG(BSM_CTRL_REG_BASE_ADDR + BSM_WDATA_OFF, val, "BSM_WDATA");
    }

    return 0;
}

/*****************************************************************************
 **	Name : 			Mp4Enc_OutputBits
 ** Description:	output bitsteam
 ** Author:			Xiaowei Luo
 **	Note:
 *****************************************************************************/
uint32 H264Enc_OutputBits(H264EncObject *vo, uint32 val, uint32 nbits)
{
    VSP_READ_REG_POLL(BSM_CTRL_REG_BASE_ADDR + BSM_RDY_OFF, V_BIT_0, V_BIT_0, TIME_OUT_CLK, "Polling BSM_RDY");
    VSP_WRITE_REG(BSM_CTRL_REG_BASE_ADDR + BSM_OP_OFF, (nbits&0x3f) << 24, "BSM_OPERATE: Set OPT_BITS");
    VSP_WRITE_REG(BSM_CTRL_REG_BASE_ADDR + BSM_WDATA_OFF, val, "BSM_WDATA");

    return nbits;
}

/*****************************************************************************
 **	Name : 			H264Enc_ByteAlign
 ** Description:	Byte Align
 ** Author:			Xiaowei Luo
 **	Note:           now, only support mpeg4, @2007.06.19
 *****************************************************************************/
uint32 H264Enc_ByteAlign(H264EncObject *vo, int32 is_align1)
{
    uint32 stuffing_bits;
    uint32 bitsLeft;
    uint32 total_bits;
    uint32 NumBits = 0;

    total_bits = VSP_READ_REG(BSM_CTRL_REG_BASE_ADDR + TOTAL_BITS_OFF,"TOTAL_BITS");
    bitsLeft = 32 - (total_bits % 32);
    stuffing_bits = bitsLeft & 0x7;

    if(stuffing_bits != 0)
    {
        if(is_align1)  //h.263,lxw,@0615
        {
            NumBits += H264Enc_OutputBits(vo, (1 << (stuffing_bits - 1)) - 1, stuffing_bits);
        } else//mpeg4,old code
        {
            NumBits += H264Enc_OutputBits(vo, 0, stuffing_bits);
        }
    }

    return NumBits;
}

void H264Enc_rbsp_trailing (H264EncObject *vo)
{
    H264Enc_OutputBits (vo, 1, 1);
    H264Enc_ByteAlign(vo, 0);
}

/* long nal start code (we always use long ones)*/
void H264Enc_write_nal_start_code (H264EncObject *vo)
{
    //write "0x00000001"
    //H264Enc_OutputBits (0x0000, 16);
    //H264Enc_OutputBits (0x0001, 16);
    H264Enc_OutputBits (vo, 0xffff, 16);// derek 2012-12-05
    H264Enc_OutputBits (vo, 0xffff, 16);// replaced with customized code, will change back when frame finished
}

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
}
#endif
/**---------------------------------------------------------------------------*/
// End
