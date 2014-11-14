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

const uint32 g_msk[33] =
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

uint32 Mp4Dec_ByteConsumed(DEC_VOP_MODE_T *vop_mode_ptr)
{
    DEC_BS_T *bs_ptr = vop_mode_ptr->bitstrm_ptr;
    uint32 nDecTotalBits = bs_ptr->bitcnt;

    return (nDecTotalBits+7)/8;
}

PUBLIC void Mp4Dec_InitBitstream(DEC_BS_T *bitstream_ptr, void *pOneFrameBitstream, int32 length)
{
    uint_32or64	offset = 0;
    uint8	*pTmp;

    offset = ((uint_32or64)pOneFrameBitstream)&0x03;
    length = ((length>>2)<<2)+8;
    pTmp = (uint8 *)((uint_32or64)pOneFrameBitstream - offset);

    bitstream_ptr->OneframeStreamLen = length;
    bitstream_ptr->stream_len_left = length;
    bitstream_ptr->pOneFrameBitstream = pTmp;

    bitstream_ptr->bitsLeft = 32 - 8*offset;
    bitstream_ptr->bitcnt = 8*offset;
    bitstream_ptr->bitcnt_before_vld = bitstream_ptr->bitcnt;
#if defined(CHIP_ENDIAN_LITTLE)
    bitstream_ptr->rdptr = bitstream_ptr->rdbfr;
#else//BIG_ENDIAN
    bitstream_ptr->rdptr = bitstream_ptr->rdbfr + 1;
#endif

    if(BITSTREAM_BFR_SIZE * sizeof (uint32) >= length)
    {
        memset(bitstream_ptr->rdptr, 0, BITSTREAM_BFR_SIZE * sizeof(uint32));

        memcpy(bitstream_ptr->rdptr, bitstream_ptr->pOneFrameBitstream, length/*+4*/);
        bitstream_ptr->OneframeStreamLen = 0;
    } else
    {
        memcpy (bitstream_ptr->rdptr, bitstream_ptr->pOneFrameBitstream, BITSTREAM_BFR_SIZE * sizeof (uint32));
        bitstream_ptr->pOneFrameBitstream += BITSTREAM_BFR_SIZE * sizeof (uint32);
        bitstream_ptr->OneframeStreamLen -= BITSTREAM_BFR_SIZE * sizeof (uint32);
        bitstream_ptr->stream_len_left -= BITSTREAM_BFR_SIZE * sizeof (uint32);
    }

#if defined(CHIP_ENDIAN_LITTLE)
    {
        uint8 * pCharTmp;

        pCharTmp = (uint8 *)bitstream_ptr->rdptr;

        ((uint8 *)(&bitstream_ptr->bufa)) [0] = pCharTmp[3];
        ((uint8 *)(&bitstream_ptr->bufa)) [1] = pCharTmp[2];
        ((uint8 *)(&bitstream_ptr->bufa)) [2] = pCharTmp[1];
        ((uint8 *)(&bitstream_ptr->bufa)) [3] = pCharTmp[0];

        ((uint8 *)(&bitstream_ptr->bufb)) [0] = pCharTmp[7];
        ((uint8 *)(&bitstream_ptr->bufb)) [1] = pCharTmp[6];
        ((uint8 *)(&bitstream_ptr->bufb)) [2] = pCharTmp[5];
        ((uint8 *)(&bitstream_ptr->bufb)) [3] = pCharTmp[4];

        bitstream_ptr->rdptr += 2;
    }
#endif
}

PUBLIC uint32 Mp4Dec_Show32Bits(DEC_BS_T *bs_ptr)
{
    uint32 bitsLeft = bs_ptr->bitsLeft;
    uint32 ret;
    uint32 nbits = 32;

#if defined(CHIP_ENDIAN_LITTLE)
    if ( bitsLeft < 32)
    {
        int nBitsInBfrb = 32 - bitsLeft;
        ret = ((bs_ptr->bufa << nBitsInBfrb) | (bs_ptr->bufb >> bitsLeft)) ;

    }
    else//bitsLeft = 32
    {
        ret = bs_ptr->bufa;
    }
#else
    if ( bitsLeft < 32)
    {
        nbits = 32 - bitsLeft;
        return (((*bs_ptr->rdptr) & g_msk [bitsLeft]) << nbits) | ((*(bs_ptr->rdptr+1)) >>bitsLeft);
    }
    else//bitsLeft = 32
    {
        return  *bs_ptr->rdptr;
    }
#endif


    return ret;
}

PUBLIC uint32 Mp4Dec_ShowBits(DEC_BS_T *bs_ptr, uint32 nbits)
{
    uint32 bitsLeft = bs_ptr->bitsLeft;
    uint32 ret;

#if defined(CHIP_ENDIAN_LITTLE)
    if (nbits <= bitsLeft)
    {
        ret = ( bs_ptr->bufa >> (bitsLeft - nbits) ) & g_msk [nbits];
    }
    else
    {
        int nBitsInBfrb = nbits - bitsLeft;

        ret = ((bs_ptr->bufa << nBitsInBfrb) | (bs_ptr->bufb >> (32 - nBitsInBfrb))) & g_msk [nbits];
    }
#else
    if (nbits <= bitsLeft)
    {
        return  ((*bs_ptr->rdptr) >> (bitsLeft - nbits)) & g_msk [nbits];
    }
    else
    {
        nbits = nbits - bitsLeft;
        return (((*bs_ptr->rdptr) & g_msk [bitsLeft]) << nbits) | ((*(bs_ptr->rdptr+1)) >> (32 - nbits));
    }
#endif


    return ret;
}

PUBLIC void Mp4Dec_FlushBits(DEC_BS_T *bs_ptr, uint32 nbits)
{
    bs_ptr->bitcnt += nbits;
    if (nbits < bs_ptr->bitsLeft)
    {
        bs_ptr->bitsLeft -= nbits;
    }
    else
    {
        bs_ptr->bitsLeft += 32 - nbits;

#if defined(CHIP_ENDIAN_LITTLE)
        bs_ptr->bufa = bs_ptr->bufb;
#else
        bs_ptr->rdptr++;
#endif

        if (bs_ptr->rdptr >=  bs_ptr->rdbfr + BITSTREAM_BFR_SIZE)
        {
            uint32 * ptr;

#if defined(CHIP_ENDIAN_LITTLE)
            ptr = bs_ptr->rdbfr;
#else
            bs_ptr->rdbfr[0] = *bs_ptr->rdptr;
            ptr = bs_ptr->rdbfr + 1;
#endif

            if (BITSTREAM_BFR_SIZE * sizeof (int) > bs_ptr->stream_len_left)
            {
                if (bs_ptr->stream_len_left != 0)
                {
                    memcpy (ptr, bs_ptr->pOneFrameBitstream, bs_ptr->stream_len_left);
                    bs_ptr->stream_len_left = 0;
                }
            }
            else
            {

                memcpy (ptr, bs_ptr->pOneFrameBitstream, BITSTREAM_BFR_SIZE * sizeof (int));
                bs_ptr->pOneFrameBitstream += BITSTREAM_BFR_SIZE * sizeof (int);
                bs_ptr->stream_len_left -= BITSTREAM_BFR_SIZE * sizeof (int);
            }

            bs_ptr->rdptr = bs_ptr->rdbfr;
        }
#if defined(CHIP_ENDIAN_LITTLE)
        /*little endian*/
        {
            uint8 *pCharTmp;

            pCharTmp = (uint8 *)bs_ptr->rdptr;

            ((uint8 *)&bs_ptr->bufb)[0] = pCharTmp[3];
            ((uint8 *)&bs_ptr->bufb)[1] = pCharTmp[2];
            ((uint8 *)&bs_ptr->bufb)[2] = pCharTmp[1];
            ((uint8 *)&bs_ptr->bufb)[3] = pCharTmp[0];
        }
        bs_ptr->rdptr++;
#endif
    }
}

/*****************************************************************************
 **	Name : 			Mp4Dec_ReadBits
 ** Description:	Read out nbits data from bitstream.
 ** Author:			Xiaowei Luo
 **	Note:
 *****************************************************************************/
PUBLIC uint32 Mp4Dec_ReadBits(DEC_BS_T *bitstream_ptr, uint32 nbits)
{
    uint32 val;

    val = Mp4Dec_ShowBits(bitstream_ptr, nbits);
    Mp4Dec_FlushBits(bitstream_ptr, nbits);

    return val;
}

/*****************************************************************************
 **	Name : 			Mp4Dec_ByteAlign_Mp4
 ** Description:	Byte align function when decode mp4 bitstream.
 ** Author:			Xiaowei Luo
 **	Note:
 *****************************************************************************/
uint32 Mp4Dec_ByteAlign_Mp4(DEC_BS_T *bitstream_ptr)
{
    uint32 n_stuffed, v, should_bits;

    n_stuffed = bitstream_ptr->bitsLeft &0x07;

    if(n_stuffed == 0)
    {
        n_stuffed = 8;
    }

    v = Mp4Dec_ShowBits(bitstream_ptr, n_stuffed);
    should_bits = (1<<(n_stuffed-1)) -1;

    if(v!=should_bits)
    {
        //fprintf( stderr, "stuffing bits not correct\n" );
        //exit(-1);
        PRINTF("stuffing bits not correct\n");
        return 0;
    }

    Mp4Dec_FlushBits(bitstream_ptr, n_stuffed);

    return n_stuffed;

}

/*****************************************************************************
 **	Name : 			Mp4Dec_ByteAlign_Startcode
 ** Description:	Byte align the startcode.
 ** Author:			Xiaowei Luo
 **	Note:
 *****************************************************************************/
uint32 Mp4Dec_ByteAlign_Startcode(DEC_BS_T *bitstream_ptr)
{
    uint8 nStuffedBits;
    uint8 nByteBits = 8;

    nStuffedBits = (uint8)((bitstream_ptr->bitsLeft)%nByteBits);
    bitstream_ptr->bitcnt += nStuffedBits;
    bitstream_ptr->bitsLeft -= nStuffedBits;

    return nStuffedBits;
}

/*****************************************************************************
 **	Name : 			Mp4Dec_ShowBitsByteAlign
 ** Description:	Mp4Dec_ShowBitsByteAlign function.
 ** Author:			Xiaowei Luo
 **	Note:
 *****************************************************************************/
uint32 Mp4Dec_ShowBitsByteAlign(DEC_BS_T *bitstream_ptr, int32 nbits)
{
    int32 nAlignBitsLeft;
    int32 nStuffBit;
    uint32 tmpVar;

    nStuffBit = bitstream_ptr->bitsLeft & 0x7;

    /*left byte align bits in bufa*/
    nAlignBitsLeft = (bitstream_ptr->bitsLeft >> 3) << 3;

    if(nStuffBit == 0)
    {
        nAlignBitsLeft -= 8;
    }

#if defined(CHIP_ENDIAN_LITTLE)
    if (nbits <= nAlignBitsLeft)
    {
        tmpVar = (bitstream_ptr->bufa >> (nAlignBitsLeft - nbits)) & g_msk [nbits];
    } else
    {
        nbits -= nAlignBitsLeft;
        tmpVar = ((bitstream_ptr->bufa & g_msk[nAlignBitsLeft]) << nbits) | (bitstream_ptr->bufb >> (32 - nbits));
    }
#else//BIG_ENDIAN
    if (nbits <= nAlignBitsLeft)
    {
        tmpVar = ((*bitstream_ptr->rdptr) >> (nAlignBitsLeft - nbits)) & g_msk [nbits];
    } else
    {
        nbits -= nAlignBitsLeft;
        tmpVar = ((*bitstream_ptr->rdptr & g_msk[nAlignBitsLeft]) << nbits) | (*(bitstream_ptr->rdptr+1) >> (32 - nbits));
    }
#endif
    return tmpVar;

}

/*****************************************************************************
 **	Name : 			Mp4Dec_ShowBitsByteAlign_H263
 ** Description:	Mp4Dec_ShowBitsByteAlign_H263 function.
 ** Author:			Xiaowei Luo
 **	Note:
 *****************************************************************************/
PUBLIC uint32 Mp4Dec_ShowBitsByteAlign_H263(DEC_BS_T *bitstream_ptr, int32 nbits)
{
    int32 nStuffBit;
    uint32 tmpVar;
    uint32 nShowBits;

    nStuffBit = 8 -  (bitstream_ptr->bitcnt&7);   //bitsLeft & 0x7;

    if(nStuffBit == 8)
    {
        nStuffBit = 0;
    }

    nShowBits = nbits + nStuffBit;

    tmpVar = Mp4Dec_ShowBits(bitstream_ptr, nShowBits);

    return (tmpVar & g_msk[nbits]);
}

PUBLIC MMDecRet Mp4Dec_VerifyBitstrm(Mp4DecObject *vo,uint8 *pStream, int32 strmLen)
{
    uint8 *pStreamEnd = pStream + (strmLen-4);
    uint8 *tempPos = pStream;
    MMDecRet ret = MMDEC_OK;
    DEC_VOP_MODE_T *vop_mode_ptr = vo->vop_mode_ptr;


    if(MPEG4 != vop_mode_ptr->video_std)
    {
        uint32 first_32bits = (pStream[0]<<24) | (pStream[1]<<16) | (pStream[2]<<8) | (pStream[3]);
        if((first_32bits>>11) == 0x10)
        {
            if((first_32bits>>10) == 0x20)
                vop_mode_ptr->video_std = ITU_H263;
            else
                vop_mode_ptr->video_std = FLV_V1;
        }
    }


    if (vop_mode_ptr->video_std != FLV_V1) //for MPEG4 and ITU_H263 bitstrm
    {
        while (tempPos < pStreamEnd)
        {
            if (tempPos[0] == 0x00 && tempPos[1] == 0x00)
            {
                if (tempPos[2] == 0x01 && tempPos[3] == 0xB6) /* MPEG4 VOP start code */
                {
                    vop_mode_ptr->video_std = MPEG4;
                    return ret;
                }
                else if ((tempPos[2] & 0xFC) == 0x80 && (tempPos[3] & 0x03)==0x02) /* H.263 PSC*/
                {
                    SCI_TRACE_LOW("Mp4Dec_VerifyBitstrm: it is ITU-H.263 format:\n");
                    vop_mode_ptr->video_std = ITU_H263;
                    vop_mode_ptr->bDataPartitioning = FALSE; //MUST!, xweiluo@2012.03.01
                    vop_mode_ptr->bReversibleVlc = FALSE;
                    return ret;
                }
            }
            tempPos++;
        }

        if (tempPos == pStreamEnd)
        {
            ret = MMDEC_STREAM_ERROR;
        }
    }

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
