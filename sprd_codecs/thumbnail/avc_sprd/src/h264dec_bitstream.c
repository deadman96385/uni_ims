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

uint32 H264Dec_ByteConsumed(H264DecContext *img_ptr)
{
    DEC_BS_T *bs_ptr = img_ptr->bitstrm_ptr;
    uint32 nDecTotalBits = bs_ptr->bitcnt;
    
    return (nDecTotalBits+7)/8;
}

void H264Dec_InitBitstream(DEC_BS_T * stream, void *pOneFrameBitstream, int32 length)
{
    stream->bitcnt = 0;
    stream->rdptr = (uint32 *)pOneFrameBitstream;
    stream->bitsLeft   = 32;
    stream->bitcnt_before_vld = stream->bitcnt;
}

PUBLIC void REVERSE_BITS(DEC_BS_T *stream, uint32 nbits)
{
    stream->bitcnt -= nbits;
    stream->bitsLeft += nbits;

    if (stream->bitsLeft > 0x20)
    {
        stream->bitsLeft -= 0x20;
        stream->rdptr--;
    }
}

#ifdef WIN32
PUBLIC uint32 READ_BITS(DEC_BS_T *stream, uint32 nbits)
{
    uint32 temp;

    temp = BITSTREAMSHOWBITS(stream, nbits);
    BITSTREAMFLUSHBITS(stream, nbits);

    return temp;
}

PUBLIC uint32 READ_BITS1(DEC_BS_T *stream)
{
    uint32 val;

    val = ((*stream->rdptr >> (stream->bitsLeft - 1)) & 0x1);
    stream->bitcnt++;
    stream->bitsLeft--;

    if (!stream->bitsLeft)
    {
        stream->bitsLeft = 32;
        stream->rdptr++;
    }

    return val;
}

PUBLIC uint32 READ_UE_V (DEC_BS_T * stream)
{
    int32 info;
    uint32 ret;
    uint32 tmp;
    uint32 leading_zero = 0;

    //tmp = BitstreamShowBits (stream, 32);
    tmp = (uint32)(BITSTREAMSHOWBITS (stream, 32));

    /*find the leading zero number*/
#ifndef _ARM_CLZ_OPT_
    if (!tmp)
    {
        stream->error_flag |= ER_BSM_ID;
        return 0;
    }
    while ( (tmp & (1 << (31 - leading_zero))) == 0 )
        leading_zero++;
#else
#if defined(__GNUC__)
    __asm__("clz %0, %1":"=&r"(leading_zero):"r"(tmp):"cc");
#else
    __asm {
        clz leading_zero, tmp
    }
#endif
#endif

    //must! because BITSTRM may be error and can't find the leading zero, xw@20100527
    if (leading_zero > 16)
    {
        stream->error_flag |= ER_BSM_ID;
        //g_image_ptr->return_pos |= (1<<0);
        return 0;
    }

    BITSTREAMFLUSHBITS(stream, leading_zero * 2 + 1);

    info = (tmp >> (32 - 2 * leading_zero -1)) & g_msk [leading_zero];
    ret = (1 << leading_zero) + info - 1;

    return ret;
}

PUBLIC int32 READ_SE_V (DEC_BS_T * stream)
{
    int32 ret;
    int32 info;
    int32 tmp;
    uint32 leading_zero = 0;

    //tmp = BitstreamShowBits (stream, 32);
    tmp = BITSTREAMSHOWBITS(stream, 32);

    /*find the leading zero number*/
#ifndef _ARM_CLZ_OPT_
    if (!tmp)
    {
        stream->error_flag |= ER_BSM_ID;
        return 0;
    }
    while ( (tmp & (1 << (31 - leading_zero))) == 0 )
        leading_zero++;
#else
#if defined(__GNUC__)
    __asm__("clz %0, %1":"=&r"(leading_zero):"r"(tmp):"cc");
#else
    __asm {
        clz leading_zero, tmp
    }
#endif
#endif

    //must! because BITSTRM may be error and can't find the leading zero, xw@20100527
    if (leading_zero > 16)
    {
        stream->error_flag |= ER_BSM_ID;
        //g_image_ptr->return_pos |= (1<<1);
        return 0;
    }

    BITSTREAMFLUSHBITS (stream, leading_zero * 2 + 1);

    info = (tmp >> (32 - 2 * leading_zero -1)) & g_msk [leading_zero];

    tmp = (1 << leading_zero) + info - 1;
    ret = (tmp + 1) / 2;

    if ( (tmp & 1) == 0 )
        ret = -ret;

    return ret;
}
#endif

PUBLIC uint32 H264Dec_Long_UEV (DEC_BS_T * stream)
{
    uint32 tmp;
    int32 leading_zero = 0;

    tmp = BITSTREAMSHOWBITS (stream, 16);

    if (tmp == 0)
    {
        READ_FLC (stream, 16);
        leading_zero = 16;

        do {
            tmp = READ_BITS1 (stream);
            leading_zero++;
        } while(!tmp);

        leading_zero--;
        tmp = READ_FLC (stream, leading_zero);

        return tmp;
    } else
    {
        return READ_UE_V (stream);
    }
}

PUBLIC int32 H264Dec_Long_SEV (DEC_BS_T * stream)
{
    uint32 tmp;
    int32 leading_zero = 0;

    tmp = BITSTREAMSHOWBITS (stream, 16);

    if (tmp == 0)
    {
        READ_FLC (stream, 16);
        leading_zero = 16;

        do {
            tmp = READ_BITS1 (stream);
            leading_zero++;
        } while(!tmp);

        leading_zero--;
        tmp = READ_FLC (stream, leading_zero);

        return tmp;
    } else
    {
        return READ_SE_V (stream);
    }
}

#if 0
LOCAL void H264Dec_byte_align (void)
{
    int32 left_bits;
    int32 flush_bits;
    DEC_BS_T * stream = g_image_ptr->bitstrm_ptr;
    uint32 nDecTotalBits = stream->bitcnt;

    left_bits = 8 - (nDecTotalBits&0x7);

    flush_bits = (left_bits == 8)?8: left_bits;

    READ_FLC(stream, flush_bits);

    return;
}

PUBLIC void H264Dec_flush_left_byte (void)
{
    int32 i;
    int32 dec_len;
    int32 left_bytes;
    DEC_BS_T * stream = g_image_ptr->bitstrm_ptr;
    uint32 nDecTotalBits = stream->bitcnt;

    H264Dec_byte_align ();

    dec_len = nDecTotalBits>>3;

    left_bytes = g_nalu_ptr->len - dec_len;

    for (i = 0; i < left_bytes; i++)
    {
        READ_FLC(stream, 8);
    }

    return;
}
#endif
/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
}
#endif
/**---------------------------------------------------------------------------*/
// End
