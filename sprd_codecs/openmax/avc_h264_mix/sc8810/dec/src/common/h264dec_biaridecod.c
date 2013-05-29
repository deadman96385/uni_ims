/******************************************************************************
 ** File Name:      h264dec_biaridecod.c                                      *
 ** Author:         Xiaowei.Luo                                               *
 ** DATE:           03/29/2010                                                *
 ** Copyright:      2010 Spreadtrum, Incoporated. All Rights Reserved.        *
 ** Description:    interface of transplant                                   *
 ** Note:           None                                                      *
 ******************************************************************************

 ******************************************************************************
 **                        Edit History                                       *
 ** ------------------------------------------------------------------------- *
 ** DATE           NAME             DESCRIPTION                               *
 ** 03/29/2010     Xiaowei.Luo      Create.                                   *
 ******************************************************************************/
#include "sc8810_video_header.h"
/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    extern   "C" 
    {
#endif

#define B_BITS    10  // Number of bits to represent the whole coding interval
#define HALF      (1 << (B_BITS-1))
#define QUARTER   (1 << (B_BITS-2))

uint32 ff_init_cabac_decoder(DEC_IMAGE_PARAMS_T *img_ptr)
{
	CABACContext *c = &(img_ptr->cabac);
	DEC_BS_T * stream = img_ptr->bitstrm_ptr;
	uint32 nStuffedBits;
	uint32 nDecTotalBits = stream->bitcnt;//VSP_READ_REG(VSP_BSM_REG_BASE+BSM_TOTAL_BITS_OFF, "read BSMR_TOTAL_BITS reg");
	uint32 bitsLeft = 32 - (nDecTotalBits % 32);
	nStuffedBits = (bitsLeft%8);
	
	if (nStuffedBits)
	{
		READ_FLC(stream, nStuffedBits);	//flush
	}
	
    	c->low =  READ_FLC(stream, (B_BITS-1));
    	c->range= 0x1FE;

	return (nStuffedBits+(B_BITS-1));
}

uint32 get_cabac (CABACContext *c, uint8 * const state)
{
    int32 s = *state;
    int32 RangeLPS = ff_h264_lps_range[2*(c->range&0xc0)+s];
    int32 bit, lps_mask;

    c->range -= RangeLPS;
    lps_mask = (c->low < c->range) ? 0 : -1;//((c->range<<(CABAC_BITS+1)) - c->low)>>31;

    c->low -= (c->range & lps_mask);
    c->range += (RangeLPS - c->range) & lps_mask;

    s ^= lps_mask;
    *state = (ff_h264_mlps_state+128)[s];
    bit = s&1;

    while (c->range < QUARTER)
    {
        uint32 val;
        DEC_BS_T * stream = g_image_ptr->bitstrm_ptr;

    	val = ((*stream->rdptr >> (stream->bitsLeft - 1)) & 0x1);
    	stream->bitcnt++;
    	stream->bitsLeft--;

    	if (!stream->bitsLeft)
    	{
    		stream->bitsLeft = 32;
    		stream->rdptr++;
    	}

    	/* Shift in next bit and add to value */
    	c->low = ((c->low << 1) | val);

    	/* Double range */
    	c->range <<= 1;
    }
	
    return (bit);
}

int32 get_cabac_bypass(CABACContext *c)
{
    DEC_BS_T * stream = g_image_ptr->bitstrm_ptr;
    uint32 bit = 0;

    c->low = (c->low << 1) | (READ_BITS1(stream));

    if (c->low >= c->range)
    {
		bit = 1;
		c->low -= c->range;
    }

    return (bit);
}

int32 get_cabac_bypass_sign(CABACContext *c, int val)
{
    DEC_BS_T * stream = g_image_ptr->bitstrm_ptr;

    c->low = (c->low << 1) | (READ_BITS1(stream));

    if ( c->low >= c->range)
    {
        c->low -= c->range;
    }else
    {
		val = -val;
    }

    return (val);
}

int32 get_cabac_terminate (DEC_IMAGE_PARAMS_T *img_ptr)
{
    CABACContext *c = &img_ptr->cabac;

    c->range -= 2;
    if (c->low >= c->range)
    {
		return 1;
    }else
    {
		while (c->range < QUARTER)
		{
			DEC_BS_T * stream = img_ptr->bitstrm_ptr;

			/* Double range */
			c->range <<= 1;

			/* Shift in next bit and add to value */
			c->low = (c->low << 1) | (READ_BITS1(stream));
		}

		return 0;
    }
}

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End 
