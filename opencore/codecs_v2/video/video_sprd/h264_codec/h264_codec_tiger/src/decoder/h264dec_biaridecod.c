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
#include "tiger_video_header.h"
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

/* Range table for  LPS */ 
const uint8 rLPS_table_64x4[64][4]=
{
  { 128, 176, 208, 240},
  { 128, 167, 197, 227},
  { 128, 158, 187, 216},
  { 123, 150, 178, 205},
  { 116, 142, 169, 195},
  { 111, 135, 160, 185},
  { 105, 128, 152, 175},
  { 100, 122, 144, 166},
  {  95, 116, 137, 158},
  {  90, 110, 130, 150},
  {  85, 104, 123, 142},
  {  81,  99, 117, 135},
  {  77,  94, 111, 128},
  {  73,  89, 105, 122},
  {  69,  85, 100, 116},
  {  66,  80,  95, 110},
  {  62,  76,  90, 104},
  {  59,  72,  86,  99},
  {  56,  69,  81,  94},
  {  53,  65,  77,  89},
  {  51,  62,  73,  85},
  {  48,  59,  69,  80},
  {  46,  56,  66,  76},
  {  43,  53,  63,  72},
  {  41,  50,  59,  69},
  {  39,  48,  56,  65},
  {  37,  45,  54,  62},
  {  35,  43,  51,  59},
  {  33,  41,  48,  56},
  {  32,  39,  46,  53},
  {  30,  37,  43,  50},
  {  29,  35,  41,  48},
  {  27,  33,  39,  45},
  {  26,  31,  37,  43},
  {  24,  30,  35,  41},
  {  23,  28,  33,  39},
  {  22,  27,  32,  37},
  {  21,  26,  30,  35},
  {  20,  24,  29,  33},
  {  19,  23,  27,  31},
  {  18,  22,  26,  30},
  {  17,  21,  25,  28},
  {  16,  20,  23,  27},
  {  15,  19,  22,  25},
  {  14,  18,  21,  24},
  {  14,  17,  20,  23},
  {  13,  16,  19,  22},
  {  12,  15,  18,  21},
  {  12,  14,  17,  20},
  {  11,  14,  16,  19},
  {  11,  13,  15,  18},
  {  10,  12,  15,  17},
  {  10,  12,  14,  16},
  {   9,  11,  13,  15},
  {   9,  11,  12,  14},
  {   8,  10,  12,  14},
  {   8,   9,  11,  13},
  {   7,   9,  11,  12},
  {   7,   9,  10,  12},
  {   7,   8,  10,  11},
  {   6,   8,   9,  11},
  {   6,   7,   9,  10},
  {   6,   7,   8,   9},
  {   2,   2,   2,   2}
};

const unsigned short AC_next_state_MPS_64[64] =    
{
  1,2,3,4,5,6,7,8,9,10,
  11,12,13,14,15,16,17,18,19,20,
  21,22,23,24,25,26,27,28,29,30,
  31,32,33,34,35,36,37,38,39,40,
  41,42,43,44,45,46,47,48,49,50,
  51,52,53,54,55,56,57,58,59,60,
  61,62,62,63
};


const unsigned short AC_next_state_LPS_64[64] =    
{
  0, 0, 1, 2, 2, 4, 4, 5, 6, 7,
  8, 9, 9,11,11,12,13,13,15,15, 
  16,16,18,18,19,19,21,21,22,22,
  23,24,24,25,26,26,27,27,28,29,
  29,30,30,30,31,32,32,33,33,33,
  34,34,35,35,35,36,36,36,37,37, 
  37,38,38,63
};

const uint8 renorm_table_32[32]={6,5,4,4,3,3,3,3,2,2,2,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1};

uint32 arideco_start_decoding(DEC_IMAGE_PARAMS_T *img_ptr)
{
	DEC_BS_T * stream = img_ptr->bitstrm_ptr;
	DecodingEnvironment* dep = &img_ptr->de_cabac;
	uint32 nStuffedBits;
	uint32 nDecTotalBits = stream->bitcnt;//VSP_READ_REG(VSP_BSM_REG_BASE+BSM_TOTAL_BITS_OFF, "read BSMR_TOTAL_BITS reg");
	uint32 bitsLeft = 32 - (nDecTotalBits % 32);
	nStuffedBits = (bitsLeft%8);
	
	if (nStuffedBits)
	{
		READ_FLC(stream, nStuffedBits);	//flush
	}
			 
	dep->Drange = HALF-2;
	dep->Dvalue = READ_FLC(stream, (B_BITS-1));

	return (nStuffedBits+(B_BITS-1));
}

/*!
 ************************************************************************
 * \brief
 *    Initializes a given context with some pre-defined probability state
 ************************************************************************
 */
void biari_init_context (DEC_IMAGE_PARAMS_T *img_ptr, BiContextType *ctx, const int* ini)
{
  int pstate;

  pstate = ((ini[0]*img_ptr->qp)>>4) + ini[1];
  pstate = mmin (mmax ( 1, pstate), 126);

#if 0
  if ( pstate >= 64 )
  {
    ctx->state  = pstate - 64;
    ctx->MPS    = 1;
  }
  else
  {
    ctx->state  = 63 - pstate;
    ctx->MPS    = 0;
  }
 #else
	 if( pstate <= 63 )
               *ctx = 2 * ( 63 - pstate ) + 0;
           else
               *ctx = 2 * ( pstate - 64 ) + 1;
 #endif
}

uint32 biari_decode_symbol (DEC_IMAGE_PARAMS_T *img_ptr, BiContextType *bi_ctx_ptr)
{
	DEC_BS_T * stream = img_ptr->bitstrm_ptr;
	DecodingEnvironment* dep = &img_ptr->de_cabac;
	uint32 bit = *bi_ctx_ptr & 0x1;//bi_ctx_ptr->MPS;
	uint32 value = dep->Dvalue;
	uint32 range = dep->Drange;
//	uint32 rLPS = rLPS_table_64x4[bi_ctx_ptr->state][(range>>6)&0x3];
	int32 state = *bi_ctx_ptr>>1;
	uint32 rLPS = rLPS_table_64x4[state][(range>>6)&0x3];

	range -= rLPS;

	if (value < range) /*MPS*/
	{
		state = AC_next_state_MPS_64[state];	//next state
	}else	/*LPS*/
	{
		value -= range;
		range = rLPS;
		bit = !bit;
		if(!state)	//switch meaing of MPS if necessary
		{
			*bi_ctx_ptr ^= 0x01;
		}
		state = AC_next_state_LPS_64[state]; // next state 

	}
		*bi_ctx_ptr = (*bi_ctx_ptr&0x1)|(state<<1);

	while (range < QUARTER)
	{
		/* Double range */
		range <<= 1;

		/* Shift in next bit and add to value */
		value = (value << 1) | (READ_BITS1(stream));
	}

	dep->Drange = range;
	dep->Dvalue = value;

	return (bit);
}

uint32 biari_decode_final (DEC_IMAGE_PARAMS_T *img_ptr)
{	
	DEC_BS_T * stream = img_ptr->bitstrm_ptr;
	DecodingEnvironment* dep = &img_ptr->de_cabac;
	uint32 value = dep->Dvalue;
	uint32 range = dep->Drange - 2;

	if (value >= range)
	{
		return 1;
	}else
	{
		while (range < QUARTER)
		{
			/* Double range */
			range <<= 1;

			/* Shift in next bit and add to value */
			value = (value << 1) | (READ_BITS1(stream));
		}

		dep->Drange = range;
		dep->Dvalue = value;

		return 0;
	}
}

uint32 unary_bin_max_decode (DEC_IMAGE_PARAMS_T *img_ptr, BiContextType *bi_ctx_ptr, int32 ctx_offset, uint32 max_symbol)
{
	DecodingEnvironment* dep = &img_ptr->de_cabac;
	uint32 l;
	uint32 symbol;
	BiContextType *ictx;

	symbol = biari_decode_symbol(img_ptr, bi_ctx_ptr);
	if (symbol == 0)
	{
		return 0;
	}else
	{
		if (max_symbol == 1)
		{
			return symbol;
		}
		symbol = 0;
		ictx = bi_ctx_ptr+ctx_offset;
		do
		{
			l = biari_decode_symbol(img_ptr, ictx);
			symbol++;
		}while( (l!= 0) && (symbol < (max_symbol-1)));

		if ((l!= 0) && (symbol == (max_symbol-1)))
		{
			symbol++;
		}

		return symbol;
	}
}

uint32 unary_bin_decode (DEC_IMAGE_PARAMS_T *img_ptr, BiContextType *bi_ctx_ptr, int32 ctx_offset)
{
	DecodingEnvironment* dep = &img_ptr->de_cabac;
	uint32 l;
	uint32 symbol;
	BiContextType *ictx;

	symbol = biari_decode_symbol(img_ptr, bi_ctx_ptr);
	if (symbol == 0)
	{
		return 0;
	}else
	{
		symbol = 0;
		ictx = bi_ctx_ptr+ctx_offset;
		do
		{
			l = biari_decode_symbol(img_ptr, ictx);
			symbol++;
		}while( l!= 0);

		return symbol;
	}
}

/*!
 ************************************************************************
 * \brief
 *    biari_decode_symbol_eq_prob():
 * \return
 *    the decoded symbol
 ************************************************************************
 */
uint32 biari_decode_symbol_eq_prob(DEC_IMAGE_PARAMS_T *img_ptr)
{
	DEC_BS_T * stream = img_ptr->bitstrm_ptr;
	DecodingEnvironment* dep = &img_ptr->de_cabac;
	uint32 bit = 0;
    uint32 value  = (dep->Dvalue<<1);

	value |= (READ_BITS1(stream));

	if (value >= dep->Drange)
	{
		bit = 1;
		value -= dep->Drange;
	}

	dep->Dvalue = value;

	return (bit);
}

/*!
 ************************************************************************
 * \brief
 *    Exp Golomb binarization and decoding of a symbol
 *    with prob. of 0.5
 ************************************************************************
 */
uint32 exp_golomb_decode_eq_prob(DEC_IMAGE_PARAMS_T *img_ptr, int32 k)
{
	uint32 l;
	int32 symbol = 0;
	int32 binary_symbol = 0;
	DecodingEnvironment* dep = &img_ptr->de_cabac;

	do {
		l = biari_decode_symbol_eq_prob(img_ptr);

		if (l == 1)
		{
			symbol += (1 << k);
			k++;
		}
	} while(l != 0);

	while (k--)	//next binary part
	{
		if (biari_decode_symbol_eq_prob(img_ptr) == 1)
		{
			binary_symbol |= (1 << k);
		}
	}

	return (uint32)(symbol+binary_symbol);
}

/*!
 ************************************************************************
 * \brief
 *    Exp-Golomb decoding for Motion Vectors
 ***********************************************************************
 */
uint32 unary_exp_golomb_mv_decode (DEC_IMAGE_PARAMS_T *img_ptr, BiContextType *bi_ctx_ptr, uint32 max_bin)
{
	uint32 l, k;
	uint32 bin = 1;
	uint32 symbol;
	uint32 exp_start = 8;
	BiContextType *ictx = bi_ctx_ptr;

	symbol = biari_decode_symbol(img_ptr, ictx);

	if (symbol == 0)
	{
		return 0;
	}else
	{
		symbol = 0;
		k = 1;

		ictx++;
		do {
			l = biari_decode_symbol(img_ptr, ictx);
			if ((++bin) == 2) ictx++;
			if (bin == max_bin)	ictx++;
			symbol++;
			k++;
		} while((l!=0) && (k != exp_start));

		if (l != 0)
		{
			symbol += exp_golomb_decode_eq_prob (img_ptr, 3) + 1;
		}

		return symbol;
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
