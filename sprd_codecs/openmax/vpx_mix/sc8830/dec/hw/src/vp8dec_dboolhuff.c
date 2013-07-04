/******************************************************************************
 ** File Name:    vp8dec_dboolhuff.c                                             *
 ** Author:       Xiaowei.Luo                                                 *
 ** DATE:         07/04/2013                                                  *
 ** Copyright:    2013 Spreatrum, Incoporated. All Rights Reserved.           *
 ** Description:                                                              *
 *****************************************************************************/
/******************************************************************************
 **                   Edit    History                                         *
 **---------------------------------------------------------------------------*
 ** DATE          NAME            DESCRIPTION                                 *
 ** 07/04/2013    Xiaowei.Luo     Create.                                     *
 *****************************************************************************/
/*----------------------------------------------------------------------------*
**                        Dependencies                                        *
**---------------------------------------------------------------------------*/
#include "vp8dec_video_header.h"
/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
extern   "C"
{
#endif

const unsigned int vp8dx_bitreader_norm[256] =
{
    0, 7, 6, 6, 5, 5, 5, 5, 4, 4, 4, 4, 4, 4, 4, 4, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};


int vp8dx_decode_bool(VPXDecObject *vo, BOOL_DECODER *br, int probability)
{
    /*
     * Until optimized versions of this function are available, we
     * keep the implementation in the header to allow inlining.
     *
     *return DBOOLHUFF_INVOKE(br->rtcd, debool)(br, probability);
     */
    unsigned int bit = 0;
    unsigned int split;
    unsigned int bigsplit;
    unsigned int range = br->range;
    unsigned int value = br->value;

    split = 1 + (((range - 1) * probability) >> 8);
    bigsplit = (split << 8);

    range = split;

    if (value >= bigsplit)
    {
        range = br->range - split;
        value = value - bigsplit;
        bit = 1;
    }

    /*if(range>=0x80)
    {
        br->value = value;
        br->range = range;
        return bit
    }*/

    {
        int count = br->count;
        /*register */unsigned int shift = vp8dx_bitreader_norm[range];
        range <<= shift;
        value <<= shift;
        count -= shift;

        if (count <= 0)
        {
            value |= (BitstreamReadBits(vo, 8) << (-count));
            count += 8 ;
        }

        br->count = count;
    }
    br->value = value;
    br->range = range;
    return bit;
}

int vp8_decode_value(VPXDecObject *vo, BOOL_DECODER *br, int bits)
{
    /*
     * Until optimized versions of this function are available, we
     * keep the implementation in the header to allow inlining.
     *
     *return DBOOLHUFF_INVOKE(br->rtcd, devalue)(br, bits);
     */
    int z = 0;
    int bit;

    for (bit = bits - 1; bit >= 0; bit--)
    {
        z |= (vp8dx_decode_bool(vo, br, 0x80) << bit);
    }

    return z;
}

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
}
#endif
/**---------------------------------------------------------------------------*/
// End

