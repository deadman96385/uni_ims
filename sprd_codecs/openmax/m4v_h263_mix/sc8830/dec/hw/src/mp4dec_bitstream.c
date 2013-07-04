/******************************************************************************
 ** File Name:    mp4dec_bitstream.c                                          *
 ** Author:       Xiaowei Luo                                                 *
 ** DATE:         11/25/2008                                                  *
 ** Copyright:    2006 Spreatrum, Incoporated. All Rights Reserved.           *
 ** Description:                                                              *
 *****************************************************************************/
/******************************************************************************
 **                   Edit    History                                         *
 **---------------------------------------------------------------------------*
 ** DATE          NAME            DESCRIPTION                                 *
 ** 12/14/2006    Xiaowei Luo     Create.                                     *
 *****************************************************************************/
/*----------------------------------------------------------------------------*
**                        Dependencies                                        *
**---------------------------------------------------------------------------*/
#include "mp4dec_video_header.h"
/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
extern   "C"
{
#endif

PUBLIC uint32 show_bits(Mp4DecObject *vo, uint32 nbits)
{
    uint32 val;

    VSP_READ_REG_POLL(BSM_CTRL_REG_BASE_ADDR+BSM_RDY_OFF, 0x00000001,0x00000001,TIME_OUT_CLK, "BSM_rdy");
    VSP_WRITE_REG(BSM_CTRL_REG_BASE_ADDR+BSM_OP_OFF, (nbits<<24),"BSM_rd n bits");
    val =VSP_READ_REG(BSM_CTRL_REG_BASE_ADDR+BSM_RDATA_OFF,"BSM_rd dara");

    return val;
}

PUBLIC void flush_bits(Mp4DecObject *vo, uint32 nbits)
{
    VSP_READ_REG_POLL(BSM_CTRL_REG_BASE_ADDR+BSM_RDY_OFF, 0x00000001,0x00000001, TIME_OUT_CLK, "BSM_rdy");
    VSP_WRITE_REG(BSM_CTRL_REG_BASE_ADDR+BSM_OP_OFF, (nbits<<24|0x01),"Flush n bits");

}

/*****************************************************************************
 **	Name : 			Mp4Dec_ReadBits
 ** Description:	Read out nbits data from bitstream.
 ** Author:			Xiaowei Luo
 **	Note:
 *****************************************************************************/
PUBLIC uint32 read_bits(Mp4DecObject *vo, uint32 nbits)
{
    uint32 val;


    VSP_READ_REG_POLL(BSM_CTRL_REG_BASE_ADDR+BSM_RDY_OFF, 0x00000001,0x00000001, TIME_OUT_CLK, "BSM_rdy");
    VSP_WRITE_REG(BSM_CTRL_REG_BASE_ADDR+BSM_OP_OFF, (nbits<<24),"BSM_rd n bits");
    val =VSP_READ_REG(BSM_CTRL_REG_BASE_ADDR+BSM_RDATA_OFF,"BSM_rd dara");
    VSP_WRITE_REG(BSM_CTRL_REG_BASE_ADDR+BSM_OP_OFF, (nbits<<24|0x01),"Flush n bits");

    return val;
}

/*****************************************************************************
 **	Name : 			Mp4Dec_ByteAlign_Mp4
 ** Description:	Byte align function when decode mp4 bitstream.
 ** Author:			Xiaowei Luo
 **	Note:
 *****************************************************************************/
uint32 Mp4Dec_ByteAlign_Mp4(Mp4DecObject *vo)
{
    uint32 n_stuffed, bitsLeft;
    uint32 nDecTotalBits = VSP_READ_REG(BSM_CTRL_REG_BASE_ADDR+TOTAL_BITS_OFF, "read BSMR_TOTAL_BITS reg");
    bitsLeft = 32 - (nDecTotalBits % 32);
    n_stuffed = bitsLeft % 8;

    if(n_stuffed == 0)
    {
        n_stuffed = 8;
    }

    Mp4Dec_FlushBits(n_stuffed);

    return n_stuffed;
}

/*****************************************************************************
 **	Name : 			Mp4Dec_ByteAlign_Startcode
 ** Description:	Byte align the startcode.
 ** Author:			Xiaowei Luo
 **	Note:
 *****************************************************************************/
uint32 Mp4Dec_ByteAlign_Startcode(Mp4DecObject *vo)
{

    uint32 nStuffedBits;
    uint32 nByteBits = 8;
    uint32 bitsLeft;
    uint32 nDecTotalBits = VSP_READ_REG(BSM_CTRL_REG_BASE_ADDR+TOTAL_BITS_OFF, "read BSMR_TOTAL_BITS reg");
    bitsLeft = 32 - (nDecTotalBits % 32);
    nStuffedBits = (bitsLeft%nByteBits);

    Mp4Dec_FlushBits(nStuffedBits);

    return nStuffedBits;

}

/*****************************************************************************
 **	Name : 			Mp4Dec_ShowBitsByteAlign
 ** Description:	Mp4Dec_ShowBitsByteAlign function.
 ** Author:			Xiaowei Luo
 **	Note:
 *****************************************************************************/
uint32 Mp4Dec_ShowBitsByteAlign(Mp4DecObject *vo, int32 nbits)
{

    int32 nAlignBitsLeft;
    int32 nStuffBit;
    uint32 tmpVar;
    uint32 tmpLen;
    uint32 nDecTotalBits = VSP_READ_REG(BSM_CTRL_REG_BASE_ADDR+TOTAL_BITS_OFF, "read BSMR_TOTAL_BITS reg");
    uint32 bitsLeft = 32 - (nDecTotalBits % 32);

    nStuffBit = bitsLeft & 0x7;

    /*left byte align bits in bufa*/
    nAlignBitsLeft = (bitsLeft >> 3) << 3;

    tmpLen = bitsLeft - (uint32)nAlignBitsLeft;

    if(nStuffBit == 0)
    {
        nAlignBitsLeft -= 8;
        tmpLen += 8;
    }

    tmpLen += (uint32)nbits;

    if(tmpLen > 0x20)
    {
        /*uint32 tmpVar1 = */Mp4Dec_ShowBits(tmpLen - 0x20);
        tmpVar = Mp4Dec_ShowBits(0x20);

//		PRINTF("Warning! show bit length is large than 32!");
    } else
    {
        tmpVar = Mp4Dec_ShowBits(tmpLen);
    }

    return (tmpVar & ((1 << nbits) - 1));

}

/*****************************************************************************
 **	Name : 			Mp4Dec_ShowBitsByteAlign_H263
 ** Description:	Mp4Dec_ShowBitsByteAlign_H263 function.
 ** Author:			Xiaowei Luo
 **	Note:
 *****************************************************************************/
PUBLIC uint32 Mp4Dec_ShowBitsByteAlign_H263(Mp4DecObject *vo, int32 nbits)
{
    int32 nStuffBit;
    uint32 tmpVar;
    uint32 nShowBits;
    uint32 nDecTotalBits = VSP_READ_REG(BSM_CTRL_REG_BASE_ADDR+TOTAL_BITS_OFF, "read BSMR_TOTAL_BITS reg");

    nStuffBit = 8 -  (nDecTotalBits&7);   //bitsLeft & 0x7;

    if(nStuffBit == 8)
    {
        nStuffBit = 0;
    }

    nShowBits = nbits + nStuffBit;

    tmpVar = Mp4Dec_ShowBits(nShowBits);

    return (tmpVar & ((1 << nbits) - 1));
}
/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
}
#endif
/**---------------------------------------------------------------------------*/
// End
