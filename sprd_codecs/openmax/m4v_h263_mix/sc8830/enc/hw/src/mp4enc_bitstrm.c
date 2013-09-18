/******************************************************************************
 ** File Name:    mp4enc_bitstrm.c                                            *
 ** Author:       Xiaowei Luo                                                 *
 ** DATE:         09/14/2013                                                  *
 ** Copyright:    2013 Spreatrum, Incoporated. All Rights Reserved.           *
 ** Description:                                                              *
 *****************************************************************************/
/******************************************************************************
 **                   Edit    History                                         *
 **---------------------------------------------------------------------------*
 ** DATE          NAME            DESCRIPTION                                 *
 ** 09/14/2013    Xiaowei Luo     Create.                                     *
 *****************************************************************************/
/*----------------------------------------------------------------------------*
**                        Dependencies                                        *
**---------------------------------------------------------------------------*/
#include "mp4enc_video_header.h"

/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
extern   "C"
{
#endif

/*****************************************************************************
 **	Name : 			Mp4Enc_OutputBits
 ** Description:	output bitsteam
 ** Author:			Xiaowei Luo
 **	Note:
 *****************************************************************************/
uint32 Mp4Enc_OutputBits(Mp4EncObject*vo, uint32 val, uint32 nbits)
{
    VSP_READ_REG_POLL(BSM_CTRL_REG_BASE_ADDR + BSM_RDY_OFF, V_BIT_0, V_BIT_0, TIME_OUT_CLK, "Polling BSM_RDY");
    VSP_WRITE_REG(BSM_CTRL_REG_BASE_ADDR + BSM_OP_OFF, (nbits&0x3f) << 24, "BSM_OPERATE: Set OPT_BITS");
    VSP_WRITE_REG(BSM_CTRL_REG_BASE_ADDR + BSM_WDATA_OFF, val, "BSM_WDATA");

    return nbits;
}

/*****************************************************************************
 **	Name : 			Mp4Enc_ByteAlign
 ** Description:	Byte Align
 ** Author:			Xiaowei Luo
 **	Note:           now, only support mpeg4, @2007.06.19
 *****************************************************************************/
uint32 Mp4Enc_ByteAlign(Mp4EncObject*vo, BOOLEAN is_short_header)
{
    uint32 stuffing_bits;
    uint32 bitsLeft;
    uint32 total_bits;
    uint32 NumBits = 0;

    total_bits = VSP_READ_REG(BSM_CTRL_REG_BASE_ADDR + TOTAL_BITS_OFF,"read total bits");
    bitsLeft = 32 - (total_bits % 32);
    stuffing_bits = bitsLeft & 0x7;

    if(stuffing_bits != 0)
    {
        if(is_short_header)  //h.263,lxw,@0615
        {
            NumBits += Mp4Enc_OutputBits(vo, 0, stuffing_bits);
        } else//mpeg4,old code
        {
            NumBits += Mp4Enc_OutputBits(vo, (1 << (stuffing_bits - 1)) - 1, stuffing_bits);
        }
    } else
    {
        if(!is_short_header)
        {
            stuffing_bits = 8;
            NumBits += Mp4Enc_OutputBits(vo, 0x7f, stuffing_bits);
        }
    }
    return NumBits;
}

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
}
#endif
/**---------------------------------------------------------------------------*/
// End
