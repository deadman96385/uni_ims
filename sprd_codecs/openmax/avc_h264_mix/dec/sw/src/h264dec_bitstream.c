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

const uint32 s_msk[33] =
{
    0x00000000, 0x00000001, 0x00000003, 0x00000007,
    0x0000000f, 0x0000001f, 0x0000003f, 0x0000007f,
    0x000000ff, 0x000001ff, 0x000003ff, 0x000007ff,
    0x00000fff, 0x00001fff, 0x00003fff, 0x00007fff,
    0x0000ffff, 0x0001ffff, 0x0003ffff, 0x0007ffff,
    0x000fffff, 0x001fffff, 0x003fffff, 0x007fffff,
    0x00ffffff, 0x01ffffff, 0x03ffffff, 0x07ffffff,
    0x0fffffff, 0x1fffffff, 0x3fffffff, 0x7fffffff,
    0xffffffff
};

PUBLIC uint32 H264Dec_ByteConsumed(DEC_BS_T *bs_ptr)
{
    uint32 nDecTotalBits = bs_ptr->bitsCnt - bs_ptr->bitsAligned;

    return (nDecTotalBits+7)/8;
}

PUBLIC void H264Dec_InitBitstream(DEC_BS_T *bs_ptr, void *nalu_strm_ptr, int32 nal_strm_len)
{
    int32	offset = 0;
    uint8	*pTmp;

    offset = ((int32)nalu_strm_ptr)&0x03;
    nal_strm_len = ((nal_strm_len>>2)<<2)+8;
    pTmp = (uint8 *)((uint32)nalu_strm_ptr - offset);

    bs_ptr->stream_len = nal_strm_len;
    bs_ptr->stream_len_left = nal_strm_len;
    bs_ptr->p_stream = pTmp;

    bs_ptr->bitsLeft = 32 - 8*offset;
    bs_ptr->bitsCnt = 8*offset;
    bs_ptr->bitsAligned = bs_ptr->bitsCnt;
    bs_ptr->rdptr = bs_ptr->rdbfr;

    if(BITSTREAM_BFR_SIZE * sizeof (uint32) >= nal_strm_len) {
        memset(bs_ptr->rdptr, 0, BITSTREAM_BFR_SIZE * sizeof(uint32));
        memcpy(bs_ptr->rdptr, bs_ptr->p_stream, nal_strm_len/*+4*/);
        bs_ptr->stream_len_left = 0;
    } else {
        memcpy (bs_ptr->rdptr, bs_ptr->p_stream, BITSTREAM_BFR_SIZE * sizeof (uint32));
        bs_ptr->p_stream += BITSTREAM_BFR_SIZE * sizeof (uint32);
        bs_ptr->stream_len_left -= BITSTREAM_BFR_SIZE * sizeof (uint32);
    }

#if defined(CHIP_ENDIAN_LITTLE)
    {
        uint8 * pCharTmp;

        pCharTmp = (uint8 *)bs_ptr->rdptr;

        ((uint8 *)(&bs_ptr->bufa)) [0] = pCharTmp[3];
        ((uint8 *)(&bs_ptr->bufa)) [1] = pCharTmp[2];
        ((uint8 *)(&bs_ptr->bufa)) [2] = pCharTmp[1];
        ((uint8 *)(&bs_ptr->bufa)) [3] = pCharTmp[0];

        ((uint8 *)(&bs_ptr->bufb)) [0] = pCharTmp[7];
        ((uint8 *)(&bs_ptr->bufb)) [1] = pCharTmp[6];
        ((uint8 *)(&bs_ptr->bufb)) [2] = pCharTmp[5];
        ((uint8 *)(&bs_ptr->bufb)) [3] = pCharTmp[4];

        bs_ptr->rdptr += 2;
    }
#endif
}

PUBLIC uint32 show_bits(DEC_BS_T *bs_ptr, uint32 nbits)
{
    uint32 bitsLeft = bs_ptr->bitsLeft;
    uint32 ret;

    if (nbits <= bitsLeft) {
        ret = ( bs_ptr->bufa >> (bitsLeft - nbits) ) & s_msk [nbits];
    } else {
        int nBitsInBfrb = nbits - bitsLeft;
        ret = ((bs_ptr->bufa << nBitsInBfrb) | (bs_ptr->bufb >> (32 - nBitsInBfrb))) & s_msk [nbits];
    }

    return ret;
}

void flush_bits(DEC_BS_T *bs_ptr, uint32 nbits)
{
    bs_ptr->bitsCnt += nbits;
    if (nbits < bs_ptr->bitsLeft) {
        bs_ptr->bitsLeft -= nbits;
    } else {
        bs_ptr->bitsLeft += 32 - nbits;
        bs_ptr->bufa = bs_ptr->bufb;

        if (bs_ptr->rdptr >=  bs_ptr->rdbfr + BITSTREAM_BFR_SIZE) {
            uint32 * ptr;

            ptr = bs_ptr->rdbfr;

            if (BITSTREAM_BFR_SIZE * sizeof (int) > bs_ptr->stream_len_left) {
                if (bs_ptr->stream_len_left != 0) {
                    memcpy (ptr, bs_ptr->p_stream, bs_ptr->stream_len_left);
                    bs_ptr->stream_len_left = 0;
                }
            } else {
                memcpy (ptr, bs_ptr->p_stream, BITSTREAM_BFR_SIZE * sizeof (int));
                bs_ptr->p_stream += BITSTREAM_BFR_SIZE * sizeof (int);
                bs_ptr->stream_len_left -= BITSTREAM_BFR_SIZE * sizeof (int);
            }

            bs_ptr->rdptr = bs_ptr->rdbfr;
        }

#if defined(CHIP_ENDIAN_LITTLE)
        {
            uint8 *pCharTmp;

            pCharTmp = (uint8 *)bs_ptr->rdptr;

            ((uint8 *)&bs_ptr->bufb)[0] = pCharTmp[3];
            ((uint8 *)&bs_ptr->bufb)[1] = pCharTmp[2];
            ((uint8 *)&bs_ptr->bufb)[2] = pCharTmp[1];
            ((uint8 *)&bs_ptr->bufb)[3] = pCharTmp[0];
            bs_ptr->rdptr++;
        }
#endif
    }
}

/*****************************************************************************
 **	Name : 			H265Dec_ReadBits
 ** Description:	Read out nbits data from bitstream.
 ** Author:			Xiaowei Luo
 **	Note:
 *****************************************************************************/
PUBLIC uint32 read_bits(DEC_BS_T *bs_ptr, uint32 nbits)
{
    uint32 val;

    val = show_bits(bs_ptr, nbits);
    flush_bits(bs_ptr, nbits);

    return val;
}

PUBLIC uint32 ue_v (DEC_BS_T *bs_ptr)
{
    uint32 ret;
    uint32 tmp;

    tmp = show_bits (bs_ptr, 16);
    if (tmp == 0) {
        ret = long_ue_v(bs_ptr);
    } else {
        int32 info;
        uint32 leading_zero = 0;

        tmp = show_bits (bs_ptr, 32);

        /*find the leading zero number*/
#ifndef _ARM_CLZ_OPT_
        while ((tmp & (1 << (31 - leading_zero))) == 0) {
            leading_zero++;
        }
#else
#if defined(__GNUC__)
        __asm__("clz %0, %1":"=&r"(leading_zero):"r"(tmp):"cc");
#else
        __asm {
            clz leading_zero, tmp
        }
#endif
#endif

        flush_bits(bs_ptr, leading_zero * 2 + 1);

        info = (tmp >> (32 - 2 * leading_zero -1)) & s_msk [leading_zero];
        ret = (1 << leading_zero) + info - 1;
    }

    return ret;
}

PUBLIC int32 se_v (DEC_BS_T *bs_ptr)
{
    int32 ret;
    int32 tmp;

    tmp = show_bits (bs_ptr, 16);
    if (tmp == 0) {
        ret = long_se_v(bs_ptr);
    } else {
        int32 info;
        uint32 leading_zero = 0;

        tmp = show_bits(bs_ptr, 32);

        /*find the leading zero number*/
#ifndef _ARM_CLZ_OPT_
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

        flush_bits (bs_ptr, leading_zero * 2 + 1);

        info = (tmp >> (32 - 2 * leading_zero -1)) & s_msk [leading_zero];

        tmp = (1 << leading_zero) + info - 1;
        ret = (tmp + 1) / 2;
        if ( (tmp & 1) == 0 ) {
            ret = -ret;
        }
    }

    return ret;
}

PUBLIC int32 long_ue_v (DEC_BS_T *bs_ptr)
{
    uint32 tmp;
    int32 leading_zero = 0;
    int32 ret;

    tmp = show_bits (bs_ptr, 16);
    if (tmp == 0) {
        read_bits (bs_ptr, 16);
        leading_zero = 16;

        do {
            tmp = read_bits (bs_ptr, 1);
            leading_zero++;
        } while(!tmp);

        leading_zero--;
        tmp = read_bits (bs_ptr, leading_zero);

        ret = (1 << leading_zero) + tmp - 1;

        return ret;
    } else {
        return ue_v (bs_ptr);
    }
}

PUBLIC int32 long_se_v (DEC_BS_T *bs_ptr)
{
    uint32 tmp;
    int32 leading_zero = 0;
    int32 ret;

    tmp = show_bits (bs_ptr, 16);
    if (tmp == 0) {
        read_bits (bs_ptr, 16);
        leading_zero = 16;

        do {
            tmp = read_bits (bs_ptr, 1);
            leading_zero++;
        } while(!tmp);

        leading_zero--;
        tmp = read_bits (bs_ptr, leading_zero);
        tmp = (1 << leading_zero) + tmp - 1;
        ret = (tmp + 1) / 2;

        if ( (tmp & 1) == 0 ) {
            ret = -ret;
        }

        return ret;
    } else {
        return se_v (bs_ptr);
    }
}

PUBLIC void revserse_bits (DEC_BS_T *bs_ptr, uint32 nbits)
{
    bs_ptr->bitsCnt -= nbits;
    bs_ptr->bitsLeft += nbits;

    if (bs_ptr->bitsLeft > 32) {
        bs_ptr->bitsLeft -= 32;
        bs_ptr->bufb = bs_ptr->bufa;

#if defined(CHIP_ENDIAN_LITTLE)
        {
            uint8 *pCharTmp = (uint8 *)(bs_ptr->rdptr - 3);
            ((uint8 *)&bs_ptr->bufa)[0] = pCharTmp[3];
            ((uint8 *)&bs_ptr->bufa)[1] = pCharTmp[2];
            ((uint8 *)&bs_ptr->bufa)[2] = pCharTmp[1];
            ((uint8 *)&bs_ptr->bufa)[3] = pCharTmp[0];
            bs_ptr->rdptr--;
        }
#endif
    }
}

/*****************************************************************************
 **	Name : 			H264Dec_ByteAlign
 ** Description:	Byte align function when decode mp4 bitstream.
 ** Author:			Xiaowei Luo
 **	Note:
 *****************************************************************************/
void H264Dec_ByteAlign(DEC_BS_T *bs_ptr)
{
    uint32 n_stuffed, v;

    v = read_bits(bs_ptr, 1);
    SCI_ASSERT(v == 1);

    n_stuffed = bs_ptr->bitsLeft &0x07;
    if (n_stuffed) {
        v = read_bits(bs_ptr, n_stuffed);
        SCI_ASSERT(v == 0);
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
