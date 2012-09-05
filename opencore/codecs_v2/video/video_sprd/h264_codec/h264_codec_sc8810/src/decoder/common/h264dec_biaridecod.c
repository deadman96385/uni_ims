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

#define CABAC_BITS 16
#define CABAC_MASK ((1<<CABAC_BITS)-1)

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
	
    	c->low =  ((READ_FLC(stream, 24))<<2) + 2;
    	c->range= 0x1FE;

	return (nStuffedBits+24);
}

uint32 get_cabac (CABACContext *c, uint8 * const state)
{
	int32 s = *state;
	int32 RangeLPS = ff_h264_lps_range[2*(c->range&0xc0)+s];
	int32 bit, lps_mask;

	c->range -= RangeLPS;
	lps_mask = ((c->range<<(CABAC_BITS+1)) - c->low)>>31;

	c->low -= (c->range<<(CABAC_BITS+1)) & lps_mask;
	c->range += (RangeLPS - c->range) & lps_mask;

	s ^= lps_mask;
	*state = (ff_h264_mlps_state+128)[s];
	bit = s&1;

	lps_mask = ff_h264_norm_shift[c->range];
	c->range <<= lps_mask;
	c->low <<= lps_mask;
	if (!(c->low & CABAC_MASK))
	{
		//	refill2(c);
		int32	i, x;

		x = c->low ^ (c->low - 1);
		i = 7 - ff_h264_norm_shift[x>>(CABAC_BITS-1)];

		x =-CABAC_MASK;
		x += (READ_BITS(g_image_ptr->bitstrm_ptr, 16)<<1);

		c->low += (x<<i);
	}

	return bit;	
}

int32 get_cabac_bypass(CABACContext *c)
{
	int range;
	
	c->low += c->low;

	if(!(c->low & CABAC_MASK))
    	{
		//refill(c);
		c->low += (READ_BITS(g_image_ptr->bitstrm_ptr, 16))<<1;
		c->low -= CABAC_MASK;		
    	}

	range= c->range<<(CABAC_BITS+1);
    	if(c->low < range)
	{
       	return 0;
	}else
	{
		c->low -= range;
        	return 1;
    	}
}

int32 get_cabac_bypass_sign(CABACContext *c, int val)
{
	int range, mask;

	c->low += c->low;

	if(!(c->low & CABAC_MASK))
    	{
		//refill(c);
		c->low += (READ_BITS(g_image_ptr->bitstrm_ptr, 16))<<1;
		c->low -= CABAC_MASK;		
    	}

	range= c->range<<(CABAC_BITS+1);
	c->low -= range;
	mask= c->low >> 31;
	range &= mask;
	c->low += range;
    	return (val^mask)-mask;
}

int32 get_cabac_terminate (DEC_IMAGE_PARAMS_T *img_ptr)
{
	CABACContext *c = &img_ptr->cabac;
	c->range -= 2;
	if (c->low < c->range<<(CABAC_BITS+1))
	{
		int32 shift = (uint32)(c->range - 0x100) >> 31;
		c->range <<= shift;
		c->low <<= shift;

		if (!(c->low & CABAC_MASK))
		{
		//refill
			c->low += (READ_BITS(g_image_ptr->bitstrm_ptr, 16))<<1;
			c->low -= CABAC_MASK;			
		}
		
		return 0;
	}else
	{
		return 1;
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
