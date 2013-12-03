/********************************************************************************
**  File Name: 	mp3_layer3.c            						                *
**  Author:		Tan Li      									                *
**  Date:		17/01/2011                                                      *
**  Copyright:	2011 Spreadtrum, Incorporated. All Rights Reserved.		        *
**  Description:  layer3 decoding process       						        *
*********************************************************************************
*********************************************************************************
**  Edit History											                    *
**------------------------------------------------------------------------------*
**  DATE			NAME			DESCRIPTION				                    *
**  17/01/2011		Tan li    		Create. 				                    *
*********************************************************************************/

# include <string.h>

# ifdef HAVE_ASSERT_H
#  include <assert.h>
# endif

# ifdef HAVE_LIMITS_H
#  include <limits.h>
# else
#  define CHAR_BIT  8
# endif

# include "mp3_fixed.h"
# include "mp3_bit.h"
# include "mp3_stream.h"
# include "mp3_frame.h"
# include "mp3_huffman.h"
# include "mp3_layer3.h"
# include "mp3_synth.h"


#ifdef __ASM_OPT__
extern mp3_fixed_t MP3_DecMulLongAsm(mp3_fixed_t x0, mp3_fixed_t y0);
#define mp3_f_mul_stereo MP3_DecMulLongAsm 
//#else
#endif

#define  MP3_F_WIN(x)  (((x)+(1<<12))>>13)
#define  MP3_F_WIN_1(x)  (((x)+(1<<11))>>12)

/* --- Layer III ----------------------------------------------------------- */

#define	count1table_select     0x01
#define	scalefac_scale            0x02
#define	preflag	                 0x04
#define	mixed_block_flag      0x08
#define	I_STEREO         0x1
#define	MS_STEREO    0x2


typedef struct _SideInfo {
    uint32 main_data_begin;
    uint32 private_bits;
    
    uint32 scfsi[2];
    
    struct granule {
        struct channel {
            /* from side info */
            uint8 region_count[3];			
            uint8 table_select[3];
            uint16 big_values;
            uint16 flags;
            
            uint16 part2_3_length;
            uint16 global_gain;
            uint16 scalefac_compress;			
            
            uint8 block_type;			
            uint8 subblock_gain[3];			
            /* from main_data */
            uint8 scalefac[39];	/* scalefac_l and/or scalefac_s */
            
        } ch[2];
    } gr[2];
}SIDE_INFO_T;

/*
* scalefactor bit lengths
* derived from section 2.4.2.7 of ISO/IEC 11172-3
*/
static
struct {
	unsigned char slen1;
	unsigned char slen2;
} const sflen_table[16] = {
	{ 0, 0 }, { 0, 1 }, { 0, 2 }, { 0, 3 },
	{ 3, 0 }, { 1, 1 }, { 1, 2 }, { 1, 3 },
	{ 2, 1 }, { 2, 2 }, { 2, 3 }, { 3, 1 },
	{ 3, 2 }, { 3, 3 }, { 4, 2 }, { 4, 3 }
};

/*
* number of LSF scalefactor band values
* derived from section 2.4.3.2 of ISO/IEC 13818-3
*/
static
unsigned char const nsfb_table[6][3][4] = {
	{ {  6,  5,  5, 5 },
    {  9,  9,  9, 9 },
    {  6,  9,  9, 9 } },
	
	{ {  6,  5,  7, 3 },
    {  9,  9, 12, 6 },
    {  6,  9, 12, 6 } },
	
	{ { 11, 10,  0, 0 },
    { 18, 18,  0, 0 },
    { 15, 18,  0, 0 } },
	
	{ {  7,  7,  7, 0 },
    { 12, 12, 12, 0 },
    {  6, 15, 12, 0 } },
	
	{ {  6,  6,  6, 3 },
    { 12,  9,  9, 6 },
    {  6, 12,  9, 6 } },
	
	{ {  8,  8,  5, 0 },
    { 15, 12,  9, 0 },
    {  6, 18,  9, 0 } }
};

/*
* MPEG-1 scalefactor band widths
* derived from Table B.8 of ISO/IEC 11172-3
*/
static
unsigned char const sfb_48000_long[] = {
	4,  4,  4,  4,  4,  4,  6,  6,  6,   8,  10,
		12, 16, 18, 22, 28, 34, 40, 46, 54,  54, 192
};

static
unsigned char const sfb_44100_long[] = {
	4,  4,  4,  4,  4,  4,  6,  6,  8,   8,  10,
		12, 16, 20, 24, 28, 34, 42, 50, 54,  76, 158
};

static
unsigned char const sfb_32000_long[] = {
	4,  4,  4,  4,  4,  4,  6,  6,  8,  10,  12,
		16, 20, 24, 30, 38, 46, 56, 68, 84, 102,  26
};

static
unsigned char const sfb_48000_short[] = {
	4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  6,
		6,  6,  6,  6,  6, 10, 10, 10, 12, 12, 12, 14, 14,
		14, 16, 16, 16, 20, 20, 20, 26, 26, 26, 66, 66, 66
};

static
unsigned char const sfb_44100_short[] = {
	4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  6,
		6,  6,  8,  8,  8, 10, 10, 10, 12, 12, 12, 14, 14,
		14, 18, 18, 18, 22, 22, 22, 30, 30, 30, 56, 56, 56
};

static
unsigned char const sfb_32000_short[] = {
	4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  6,
		6,  6,  8,  8,  8, 12, 12, 12, 16, 16, 16, 20, 20,
		20, 26, 26, 26, 34, 34, 34, 42, 42, 42, 12, 12, 12
};

static
unsigned char const sfb_48000_mixed[] = {
	/* long */   4,  4,  4,  4,  4,  4,  6,  6,
	/* short */  4,  4,  4,  6,  6,  6,  6,  6,  6, 10,
	10, 10, 12, 12, 12, 14, 14, 14, 16, 16,
		16, 20, 20, 20, 26, 26, 26, 66, 66, 66
};

static
unsigned char const sfb_44100_mixed[] = {
	/* long */   4,  4,  4,  4,  4,  4,  6,  6,
	/* short */  4,  4,  4,  6,  6,  6,  8,  8,  8, 10,
	10, 10, 12, 12, 12, 14, 14, 14, 18, 18,
		18, 22, 22, 22, 30, 30, 30, 56, 56, 56
};

static
unsigned char const sfb_32000_mixed[] = {
	/* long */   4,  4,  4,  4,  4,  4,  6,  6,
	/* short */  4,  4,  4,  6,  6,  6,  8,  8,  8, 12,
	12, 12, 16, 16, 16, 20, 20, 20, 26, 26,
		26, 34, 34, 34, 42, 42, 42, 12, 12, 12
};

/*
* MPEG-2 scalefactor band widths
* derived from Table B.2 of ISO/IEC 13818-3
*/
static
unsigned char const sfb_24000_long[] = {
	6,  6,  6,  6,  6,  6,  8, 10, 12,  14,  16,
		18, 22, 26, 32, 38, 46, 54, 62, 70,  76,  36
};

static
unsigned char const sfb_22050_long[] = {
	6,  6,  6,  6,  6,  6,  8, 10, 12,  14,  16,
		20, 24, 28, 32, 38, 46, 52, 60, 68,  58,  54
};

# define sfb_16000_long  sfb_22050_long

static
unsigned char const sfb_24000_short[] = {
	4,  4,  4,  4,  4,  4,  4,  4,  4,  6,  6,  6,  8,
		8,  8, 10, 10, 10, 12, 12, 12, 14, 14, 14, 18, 18,
		18, 24, 24, 24, 32, 32, 32, 44, 44, 44, 12, 12, 12
};

static
unsigned char const sfb_22050_short[] = {
	4,  4,  4,  4,  4,  4,  4,  4,  4,  6,  6,  6,  6,
		6,  6,  8,  8,  8, 10, 10, 10, 14, 14, 14, 18, 18,
		18, 26, 26, 26, 32, 32, 32, 42, 42, 42, 18, 18, 18
};

static
unsigned char const sfb_16000_short[] = {
	4,  4,  4,  4,  4,  4,  4,  4,  4,  6,  6,  6,  8,
		8,  8, 10, 10, 10, 12, 12, 12, 14, 14, 14, 18, 18,
		18, 24, 24, 24, 30, 30, 30, 40, 40, 40, 18, 18, 18
};

static
unsigned char const sfb_24000_mixed[] = {
	/* long */   6,  6,  6,  6,  6,  6,
	/* short */  6,  6,  6,  8,  8,  8, 10, 10, 10, 12,
	12, 12, 14, 14, 14, 18, 18, 18, 24, 24,
		24, 32, 32, 32, 44, 44, 44, 12, 12, 12
};

static
unsigned char const sfb_22050_mixed[] = {
	/* long */   6,  6,  6,  6,  6,  6,
	/* short */  6,  6,  6,  6,  6,  6,  8,  8,  8, 10,
	10, 10, 14, 14, 14, 18, 18, 18, 26, 26,
		26, 32, 32, 32, 42, 42, 42, 18, 18, 18
};

static
unsigned char const sfb_16000_mixed[] = {
	/* long */   6,  6,  6,  6,  6,  6,
	/* short */  6,  6,  6,  8,  8,  8, 10, 10, 10, 12,
	12, 12, 14, 14, 14, 18, 18, 18, 24, 24,
		24, 30, 30, 30, 40, 40, 40, 18, 18, 18
};

/*
* MPEG 2.5 scalefactor band widths
* derived from public sources
*/
# define sfb_12000_long  sfb_16000_long
# define sfb_11025_long  sfb_12000_long

static
unsigned char const sfb_8000_long[] = {
	12, 12, 12, 12, 12, 12, 16, 20, 24,  28,  32,
		40, 48, 56, 64, 76, 90,  2,  2,  2,   2,   2
};

# define sfb_12000_short  sfb_16000_short
# define sfb_11025_short  sfb_12000_short

static
unsigned char const sfb_8000_short[] = {
	8,  8,  8,  8,  8,  8,  8,  8,  8, 12, 12, 12, 16,
		16, 16, 20, 20, 20, 24, 24, 24, 28, 28, 28, 36, 36,
		36,  2,  2,  2,  2,  2,  2,  2,  2,  2, 26, 26, 26
};

# define sfb_12000_mixed  sfb_16000_mixed
# define sfb_11025_mixed  sfb_12000_mixed

/* the 8000 Hz short block scalefactor bands do not break after
the first 36 frequency lines, so this is probably wrong */
static
unsigned char const sfb_8000_mixed[] = {
	/* long */  12, 12, 12,
	/* short */  4,  4,  4,  8,  8,  8, 12, 12, 12, 16, 16, 16,
	20, 20, 20, 24, 24, 24, 28, 28, 28, 36, 36, 36,
		2,  2,  2,  2,  2,  2,  2,  2,  2, 26, 26, 26
};

static
struct {
	unsigned char const *l;
	unsigned char const *s;
	unsigned char const *m;
} const sfbwidth_table[9] = {
	{ sfb_48000_long, sfb_48000_short, sfb_48000_mixed },
	{ sfb_44100_long, sfb_44100_short, sfb_44100_mixed },
	{ sfb_32000_long, sfb_32000_short, sfb_32000_mixed },
	{ sfb_24000_long, sfb_24000_short, sfb_24000_mixed },
	{ sfb_22050_long, sfb_22050_short, sfb_22050_mixed },
	{ sfb_16000_long, sfb_16000_short, sfb_16000_mixed },
	{ sfb_12000_long, sfb_12000_short, sfb_12000_mixed },
	{ sfb_11025_long, sfb_11025_short, sfb_11025_mixed },
	{  sfb_8000_long,  sfb_8000_short,  sfb_8000_mixed }
};

/*
* scalefactor band preemphasis (used only when preflag is set)
* derived from Table B.6 of ISO/IEC 11172-3
*/
static
unsigned char const pretab[22] = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 3, 3, 3, 2, 0
};



/*
* fractional powers of two
* used for requantization and joint stereo decoding
*
* root_table[3 + x] = 2^(x/4)
*/
static
mp3_fixed_t const root_table[7] = {
	MP3_F(0x09837f05) /* 2^(-3/4) == 0.59460355750136 */,
		MP3_F(0x0b504f33) /* 2^(-2/4) == 0.70710678118655 */,
		MP3_F(0x0d744fcd) /* 2^(-1/4) == 0.84089641525371 */,
		MP3_F(0x10000000) /* 2^( 0/4) == 1.00000000000000 */,
		MP3_F(0x1306fe0a) /* 2^(+1/4) == 1.18920711500272 */,
		MP3_F(0x16a09e66) /* 2^(+2/4) == 1.41421356237310 */,
		MP3_F(0x1ae89f99) /* 2^(+3/4) == 1.68179283050743 */
};

                            
/*lint -save -e649 -e572 -e778 */                            
                            
#define  MP3_F_ALIAS_cs(x)   ((((x)+(1<<11))>>12)<<16)
#define  MP3_F_ALIAS_ca_0(x)   ((((x)+(1<<11))>>12)&0xffff)
#define  MP3_F_ALIAS_ca(x)   ((((x)+(1<<11))>>12)&0xffff)
/*static*/
mp3_fixed_t const cs_ca[8] = {
    (MP3_F_ALIAS_cs(0x0db84a81-0x10000000))|(MP3_F_ALIAS_ca_0((0x083b5fe7-0x10000000))),
		(MP3_F_ALIAS_cs(0x0e1b9d7f-0x10000000))|(MP3_F_ALIAS_ca(0x078c36d2)),
		(MP3_F_ALIAS_cs(0x0f31adcf-0x10000000))|(MP3_F_ALIAS_ca(0x05039814)),
		(MP3_F_ALIAS_cs(0x0fbba815-0x10000000))|(MP3_F_ALIAS_ca(0x02e91dd1)),
		(MP3_F_ALIAS_cs(0x0feda417-0x10000000))|(MP3_F_ALIAS_ca(0x0183603a)),
		(MP3_F_ALIAS_cs(0x0ffc8fc8-0x10000000))|(MP3_F_ALIAS_ca(0x00a7cb87)),
		(MP3_F_ALIAS_cs(0x0fff964c-0x10000000))|(MP3_F_ALIAS_ca(0x003a2847)),
		(MP3_F_ALIAS_cs(0x0ffff8d3-0x10000000))|(MP3_F_ALIAS_ca(0x000f27b4))//0x7fff00f2
};

/*lint -restore */

# if !defined(ASO_IMDCT)
/*
* windowing coefficients for long blocks
* derived from section 2.4.3.4.10.3 of ISO/IEC 11172-3
*
* window_l[i] = sin((PI / 36) * (i + 1/2))
*/

#ifndef __ASM_OPT__
/*static*/
short const window_l[18] = {
	MP3_F_WIN(0x00b2aa3e) /* 0.043619387 0*/, MP3_F_WIN(0x0216a2a2) /* 0.130526192 1*/,
		MP3_F_WIN(0x03768962) /* 0.216439614 2*/, MP3_F_WIN(0x04cfb0e2) /* 0.300705800 3*/,
		MP3_F_WIN(0x061f78aa) /* 0.382683432 4*/, MP3_F_WIN(0x07635284) /* 0.461748613 5*/,
		MP3_F_WIN(0x0898c779) /* 0.537299608 6*/, MP3_F_WIN(0x09bd7ca0) /* 0.608761429 7*/,
		MP3_F_WIN(0x0acf37ad) /* 0.675590208 8*/, MP3_F_WIN(0x0bcbe352) /* 0.737277337 9*/,
		MP3_F_WIN(0x0cb19346) /* 0.793353340 10*/, MP3_F_WIN(0x0d7e8807) /* 0.843391446 11*/,
		
		MP3_F_WIN(0x0e313245) /* 0.887010833 12*/, MP3_F_WIN(0x0ec835e8) /* 0.923879533 13*/,
		MP3_F_WIN(0x0f426cb5) /* 0.953716951 14*/, MP3_F_WIN(0x0f9ee890) /* 0.976296007 15*/,
		MP3_F_WIN(0x0fdcf549) /* 0.991444861 16*/, MP3_F_WIN(0x0ffc19fd) /* 0.999048222 17*/
};
#else
/*static*/
mp3_fixed_t const window_l[9] = {
	((MP3_F_WIN(0x00b2aa3e)<<16)|((MP3_F_WIN(0x0ffc19fd))&0xffff)),
	((MP3_F_WIN(0x0216a2a2)<<16)|((MP3_F_WIN(0x0fdcf549))&0xffff)),
	((MP3_F_WIN(0x03768962)<<16)|((MP3_F_WIN(0x0f9ee890))&0xffff)),
	((MP3_F_WIN(0x04cfb0e2)<<16)|((MP3_F_WIN(0x0f426cb5))&0xffff)),
	((MP3_F_WIN(0x061f78aa)<<16)|((MP3_F_WIN(0x0ec835e8))&0xffff)),
	((MP3_F_WIN(0x07635284)<<16)|((MP3_F_WIN(0x0e313245))&0xffff)),
	((MP3_F_WIN(0x0898c779)<<16)|((MP3_F_WIN(0x0d7e8807))&0xffff)),
	((MP3_F_WIN(0x09bd7ca0)<<16)|((MP3_F_WIN(0x0cb19346))&0xffff)),
	((MP3_F_WIN(0x0acf37ad)<<16)|((MP3_F_WIN(0x0bcbe352))&0xffff))
};
#endif

# endif  /* ASO_IMDCT */

/*
* windowing coefficients for short blocks
* derived from section 2.4.3.4.10.3 of ISO/IEC 11172-3
*
* window_s[i] = sin((PI / 12) * (i + 1/2))
*/
static
short const window_s[6] = {
	MP3_F_WIN(0x0216a2a2) /* 0.130526192 */, MP3_F_WIN(0x061f78aa) /* 0.382683432 */,
	MP3_F_WIN(0x09bd7ca0) /* 0.608761429 */, MP3_F_WIN(0x0cb19346) /* 0.793353340 */,
	MP3_F_WIN(0x0ec835e8) /* 0.923879533 */, MP3_F_WIN(0x0fdcf549) /* 0.991444861 */
};

/*
* coefficients for intensity stereo processing
* derived from section 2.4.3.4.9.3 of ISO/IEC 11172-3
*
* is_ratio[i] = tan(i * (PI / 12))
* is_table[i] = is_ratio[i] / (1 + is_ratio[i])
*/
static
mp3_fixed_t const is_table[7] = {
	MP3_F(0x00000000) /* 0.000000000 */,
		MP3_F(0x0361962f) /* 0.211324865 */,
		MP3_F(0x05db3d74) /* 0.366025404 */,
		MP3_F(0x08000000) /* 0.500000000 */,
		MP3_F(0x0a24c28c) /* 0.633974596 */,
		MP3_F(0x0c9e69d1) /* 0.788675135 */,
		MP3_F(0x10000000) /* 1.000000000 */
};

/*
* coefficients for LSF intensity stereo processing
* derived from section 2.4.3.2 of ISO/IEC 13818-3
*
* is_lsf_table[0][i] = (1 / sqrt(sqrt(2)))^(i + 1)
* is_lsf_table[1][i] = (1 /      sqrt(2)) ^(i + 1)
*/
static
mp3_fixed_t const is_lsf_table[2][15] = {
	{
		MP3_F(0x0d744fcd) /* 0.840896415 */,
			MP3_F(0x0b504f33) /* 0.707106781 */,
			MP3_F(0x09837f05) /* 0.594603558 */,
			MP3_F(0x08000000) /* 0.500000000 */,
			MP3_F(0x06ba27e6) /* 0.420448208 */,
			MP3_F(0x05a8279a) /* 0.353553391 */,
			MP3_F(0x04c1bf83) /* 0.297301779 */,
			MP3_F(0x04000000) /* 0.250000000 */,
			MP3_F(0x035d13f3) /* 0.210224104 */,
			MP3_F(0x02d413cd) /* 0.176776695 */,
			MP3_F(0x0260dfc1) /* 0.148650889 */,
			MP3_F(0x02000000) /* 0.125000000 */,
			MP3_F(0x01ae89fa) /* 0.105112052 */,
			MP3_F(0x016a09e6) /* 0.088388348 */,
			MP3_F(0x01306fe1) /* 0.074325445 */
	}, {
		MP3_F(0x0b504f33) /* 0.707106781 */,
			MP3_F(0x08000000) /* 0.500000000 */,
			MP3_F(0x05a8279a) /* 0.353553391 */,
			MP3_F(0x04000000) /* 0.250000000 */,
			MP3_F(0x02d413cd) /* 0.176776695 */,
			MP3_F(0x02000000) /* 0.125000000 */,
			MP3_F(0x016a09e6) /* 0.088388348 */,
			MP3_F(0x01000000) /* 0.062500000 */,
			MP3_F(0x00b504f3) /* 0.044194174 */,
			MP3_F(0x00800000) /* 0.031250000 */,
			MP3_F(0x005a827a) /* 0.022097087 */,
			MP3_F(0x00400000) /* 0.015625000 */,
			MP3_F(0x002d413d) /* 0.011048543 */,
			MP3_F(0x00200000) /* 0.007812500 */,
			MP3_F(0x0016a09e) /* 0.005524272 */
		}
};


/*
* FUNCTION NAME:	   MP3_DEC_SideInfoParse
* FUNCTION DESCRIPTOR: decode frame side information from the bitstream
*/

/*static*/ int32   MP3_DEC_SideInfoParse(MP3_DEC_BIT_POOL_T *ptr, 
                                     uint32 nch,
									 int32 lsf, 
                                     SIDE_INFO_T *si,
									 uint32 *data_bitlen,
									 uint32 *priv_bitlen)
{
	uint32 ngr, gr, ch;
	int32  result = MP3_ERROR_NONE;
	
	*data_bitlen = 0;
	*priv_bitlen = lsf ? ((nch == 1) ? 1 : 2) : ((nch == 1) ? 5 : 3);
	
	si->main_data_begin = MP3_DEC_8BitRead(ptr, lsf ? 8 : 9);
	si->private_bits    = MP3_DEC_8BitRead(ptr, *priv_bitlen);
	
	ngr = 1;
	if (!lsf)
    {
		ngr = 2;		
		for (ch = 0; ch < nch; ++ch)
			si->scfsi[ch] = MP3_DEC_8BitRead(ptr, 4);
	}
	
	for (gr = 0; gr < ngr; ++gr) 
    {
		struct granule *granule = &si->gr[gr];		
		for (ch = 0; ch < nch; ++ch) 
        {
            int32 cw;
			struct channel *channel = &granule->ch[ch];
			
            cw = MP3_DEC_8BitRead(ptr, 29);
            channel->part2_3_length    = (uint16) (cw >> 17);
            channel->big_values        = (uint16)((cw >> 8) & 0x1FF);
            channel->global_gain       = cw & 0xFF;//MP3_DEC_8BitRead(ptr, 8);
			channel->scalefac_compress = (uint16) (MP3_DEC_8BitRead(ptr, lsf ? 9 : 4));
			
			*data_bitlen += channel->part2_3_length;
			
			if (channel->big_values > 288 && result == 0)
					result = MP3_ERROR_BADBIGVALUES;
			
			channel->flags = 0;
			
			/* window_switching_flag */
			if (MP3_DEC_8BitRead(ptr, 1)) 
            {
               // int32 cw;
				channel->block_type = (uint8) (MP3_DEC_8BitRead(ptr, 2));
				
				if (channel->block_type == 0 && result == 0)
					result = MP3_ERROR_BADBLOCKTYPE;
				
				if (!lsf && channel->block_type == 2 && si->scfsi[ch] && result == 0)
					result = MP3_ERROR_BADSCFSI;
				
				channel->region_count[0] = 7;
				channel->region_count[1] = 36;
				channel->region_count[2] = 36;
				if (MP3_DEC_8BitRead(ptr, 1))
					channel->flags |= MP3_MIXED_BLOCK_FLAG;
				else if (channel->block_type == 2)
					channel->region_count[0] = 8;
				
                cw = MP3_DEC_8BitRead(ptr, 19);
                channel->table_select[0]  = (cw >> 14) & 0x1f;
				if (((channel->table_select[0]==4)||(channel->table_select[0] == 14))&&(result == 0))
				{
					result = MP3_ERROR_BADHUFFTABLE;
				}
                channel->table_select[1]  = (cw >> 9 ) & 0x1f;
				if (((channel->table_select[1]==4)||(channel->table_select[1] == 14))&&(result == 0))
				{
					result = MP3_ERROR_BADHUFFTABLE;
				}

                channel->subblock_gain[0] = (cw >> 6 ) & 0x7 ;
                channel->subblock_gain[1] = (cw >> 3 ) & 0x7 ;
                channel->subblock_gain[2] = (cw ) & 0x7 ;
				
			}
			else 
            {
               // int32 cw;
				channel->block_type = 0;
                cw = MP3_DEC_8BitRead(ptr, 22);
                channel->table_select[0] = (cw >> 17) & 0x1F;
				if (((channel->table_select[0]==4)||(channel->table_select[0] == 14))&&(result == 0))
				{
					result = MP3_ERROR_BADHUFFTABLE;
				}
                channel->table_select[1] = (cw >> 12) & 0x1F;
				if (((channel->table_select[1]==4)||(channel->table_select[1] == 14))&&(result == 0))
				{
					result = MP3_ERROR_BADHUFFTABLE;
				}
                channel->table_select[2] = (cw >>  7) & 0x1F;
				if (((channel->table_select[2]==4)||(channel->table_select[2] == 14))&&(result == 0))
				{
					result = MP3_ERROR_BADHUFFTABLE;
				}
                channel->region_count[0] = (cw >>3) & 0xF;
                channel->region_count[1] = (cw) & 0x7;				
                channel->region_count[2] = 36;
			}			
			
			channel->flags |= MP3_DEC_8BitRead(ptr, lsf ? 2 : 3);
		}
	}
	
	return result;
}

/*
* NAME:	MP3_DEC_scalefactors_lsf()
* DESCRIPTION:	decode channel scalefactors for LSF from a bitstream
*/
static
unsigned int MP3_DEC_ScalefactorsLsf(MP3_DEC_BIT_POOL_T *ptr,
								  struct channel *channel,
								  struct channel *gr1ch, int mode_extension)
{
	MP3_DEC_BIT_POOL_T start;
	unsigned int scalefac_compress, index, slen[4], part, n, i;
	unsigned char const *nsfb;
	
	start = *ptr;
	
	scalefac_compress = channel->scalefac_compress;
	index = (channel->block_type == 2) ?
		((channel->flags & mixed_block_flag) ? 2 : 1) : 0;
	
	if (!((mode_extension & I_STEREO) && gr1ch)) {
		if (scalefac_compress < 400) {
			slen[0] = (scalefac_compress >> 4) / 5;
			slen[1] = (scalefac_compress >> 4) % 5;
			slen[2] = (scalefac_compress % 16) >> 2;
			slen[3] =  scalefac_compress %  4;
			
			nsfb = nsfb_table[0][index];
		}
		else if (scalefac_compress < 500) {
			scalefac_compress -= 400;
			
			slen[0] = (scalefac_compress >> 2) / 5;
			slen[1] = (scalefac_compress >> 2) % 5;
			slen[2] =  scalefac_compress %  4;
			slen[3] = 0;
			
			nsfb = nsfb_table[1][index];
		}
		else {
			scalefac_compress -= 500;
			
			slen[0] = scalefac_compress / 3;
			slen[1] = scalefac_compress % 3;
			slen[2] = 0;
			slen[3] = 0;
			
			channel->flags |= preflag;
			
			nsfb = nsfb_table[2][index];
		}
		
		n = 0;
		for (part = 0; part < 4; ++part) {
			for (i = 0; i < nsfb[part]; ++i)
				channel->scalefac[n++] = (uint8)MP3_DEC_8BitRead(ptr, slen[part]);
		}
		
		while (n < 39)
			channel->scalefac[n++] = 0;
	}
	else {  /* (mode_extension & I_STEREO) && gr1ch (i.e. ch == 1) */
		scalefac_compress >>= 1;
		
		if (scalefac_compress < 180) {
			slen[0] =  scalefac_compress / 36;
			slen[1] = (scalefac_compress % 36) / 6;
			slen[2] = (scalefac_compress % 36) % 6;
			slen[3] = 0;
			
			nsfb = nsfb_table[3][index];
		}
		else if (scalefac_compress < 244) {
			scalefac_compress -= 180;
			
			slen[0] = (scalefac_compress % 64) >> 4;
			slen[1] = (scalefac_compress % 16) >> 2;
			slen[2] =  scalefac_compress %  4;
			slen[3] = 0;
			
			nsfb = nsfb_table[4][index];
		}
		else {
			scalefac_compress -= 244;
			
			slen[0] = scalefac_compress / 3;
			slen[1] = scalefac_compress % 3;
			slen[2] = 0;
			slen[3] = 0;
			
			nsfb = nsfb_table[5][index];
		}
		
		n = 0;
		for (part = 0; part < 4; ++part) {
			unsigned int max, is_pos;
			
			max = (1 << slen[part]) - 1;
			
			for (i = 0; i < nsfb[part]; ++i) {
				is_pos = MP3_DEC_8BitRead(ptr, slen[part]);
				
				channel->scalefac[n] = is_pos;
				gr1ch->scalefac[n++] = (is_pos == max);
			}
		}
		
		while (n < 39) {
			channel->scalefac[n] = 0;
			gr1ch->scalefac[n++] = 0;  /* apparently not illegal */
		}
	}
	
	return MP3_DEC_CalcBitLen(&start, ptr);
}


/*
* NAME:	MP3_DEC_scalefactors()
* DESCRIPTION:	decode channel scalefactors of one granule from a bitstream
*/
static
unsigned int MP3_DEC_Scalefactors(MP3_DEC_BIT_POOL_T *ptr, struct channel *channel,
							  struct channel const *gr0ch, unsigned int scfsi)
{
	MP3_DEC_BIT_POOL_T start;
	unsigned int slen1, slen2, sfbi;
	
	start = *ptr;
	
	slen1 = sflen_table[channel->scalefac_compress].slen1;
	slen2 = sflen_table[channel->scalefac_compress].slen2;
	
	if (channel->block_type == 2) {
		unsigned int nsfb;
		
		sfbi = 0;
		
		nsfb = (channel->flags & mixed_block_flag) ? 8 + 3 * 3 : 6 * 3;
		while (nsfb--)
			channel->scalefac[sfbi++] = (uint8)MP3_DEC_8BitRead(ptr, slen1);
		
		nsfb = 6 * 3;
		while (nsfb--)
			channel->scalefac[sfbi++] = (uint8)MP3_DEC_8BitRead(ptr, slen2);
		
		nsfb = 1 * 3;
		while (nsfb--)
			channel->scalefac[sfbi++] = 0;
	}
	else {  /* channel->block_type != 2 */
		if (scfsi & 0x8) {
			for (sfbi = 0; sfbi < 6; ++sfbi)
				channel->scalefac[sfbi] = gr0ch->scalefac[sfbi];
		}
		else {
			for (sfbi = 0; sfbi < 6; ++sfbi)
				channel->scalefac[sfbi] = (uint8)MP3_DEC_8BitRead(ptr, slen1);
		}
		
		if (scfsi & 0x4) {
			for (sfbi = 6; sfbi < 11; ++sfbi)
				channel->scalefac[sfbi] = gr0ch->scalefac[sfbi];
		}
		else {
			for (sfbi = 6; sfbi < 11; ++sfbi)
				channel->scalefac[sfbi] = (uint8)MP3_DEC_8BitRead(ptr, slen1);
		}
		
		if (scfsi & 0x2) {
			for (sfbi = 11; sfbi < 16; ++sfbi)
				channel->scalefac[sfbi] = gr0ch->scalefac[sfbi];
		}
		else {
			for (sfbi = 11; sfbi < 16; ++sfbi)
				channel->scalefac[sfbi] = (uint8)MP3_DEC_8BitRead(ptr, slen2);
		}
		
		if (scfsi & 0x1) {
			for (sfbi = 16; sfbi < 21; ++sfbi)
				channel->scalefac[sfbi] = gr0ch->scalefac[sfbi];
		}
		else {
			for (sfbi = 16; sfbi < 21; ++sfbi)
				channel->scalefac[sfbi] = (uint8)MP3_DEC_8BitRead(ptr, slen2);
		}
		
		channel->scalefac[21] = 0;
	}
	
	return MP3_DEC_CalcBitLen(&start, ptr);
}


/*
* The Layer III formula for requantization and scaling is defined by
* section 2.4.3.4.7.1 of ISO/IEC 11172-3, as follows:
*
*   long blocks:
*   xr[i] = sign(is[i]) * abs(is[i])^(4/3) *
*           2^((1/4) * (global_gain - 210)) *
*           2^-(scalefac_multiplier *
*               (scalefac_l[sfb] + preflag * pretab[sfb]))
*
*   short blocks:
*   xr[i] = sign(is[i]) * abs(is[i])^(4/3) *
*           2^((1/4) * (global_gain - 210 - 8 * subblock_gain[w])) *
*           2^-(scalefac_multiplier * scalefac_s[sfb][w])
*
*   where:
*   scalefac_multiplier = (scalefac_scale + 1) / 2
*
* The routines MP3_DEC_exponents() and MP3_DEC_requantize() facilitate this
* calculation.
*/

/*
* NAME:	MP3_DEC_exponents()
* DESCRIPTION:	calculate scalefactor exponents
*/
static
void MP3_DEC_Exponents(struct channel const *channel,
				   unsigned char const *sfbwidth, signed int exponents[39])
{
	signed int gain;
	unsigned int scalefac_multiplier, sfbi;
	
	gain = (signed int) channel->global_gain - 210;
	scalefac_multiplier = (channel->flags & scalefac_scale) ? 2 : 1;
	
	if (channel->block_type == 2) {
		unsigned int l;
		signed int gain0, gain1, gain2;
		
		sfbi = l = 0;
		
		if (channel->flags & mixed_block_flag) {
			unsigned int premask;
			
			premask = (channel->flags & preflag) ? ~0 : 0;
			
			/* long block subbands 0-1 */
			
			while (l < 36) {
				exponents[sfbi] = gain -
					(signed int) ((channel->scalefac[sfbi] + (pretab[sfbi] & premask)) <<
					scalefac_multiplier);
				
				l += sfbwidth[sfbi++];
			}
		}
		
		/* this is probably wrong for 8000 Hz short/mixed blocks */
		
		gain0 = gain - 8 * (signed int) channel->subblock_gain[0];
		gain1 = gain - 8 * (signed int) channel->subblock_gain[1];
		gain2 = gain - 8 * (signed int) channel->subblock_gain[2];
		
		while (l < 576) {
			exponents[sfbi + 0] = gain0 -
				(signed int) (channel->scalefac[sfbi + 0] << scalefac_multiplier);
			exponents[sfbi + 1] = gain1 -
				(signed int) (channel->scalefac[sfbi + 1] << scalefac_multiplier);
			exponents[sfbi + 2] = gain2 -
				(signed int) (channel->scalefac[sfbi + 2] << scalefac_multiplier);
			
			l    += 3 * sfbwidth[sfbi];
			sfbi += 3;
		}
	}
	else {  /* channel->block_type != 2 */
		if (channel->flags & preflag) {
			for (sfbi = 0; sfbi < 22; ++sfbi) {
				exponents[sfbi] = gain -
					(signed int) ((channel->scalefac[sfbi] + pretab[sfbi]) <<
					scalefac_multiplier);
			}
		}
		else {
			for (sfbi = 0; sfbi < 22; ++sfbi) {
				exponents[sfbi] = gain -
					(signed int) (channel->scalefac[sfbi] << scalefac_multiplier);
			}
		}
	}
}

/*
* NAME:	MP3_DEC_requantize()
* DESCRIPTION:	requantize one (positive) value
*/

	
	
	
	
	
	
	

/* we must take care that sz >= bits and sz < sizeof(long) lest bits == 0 */
# define MASK(cache, sz, bits)	\
(((cache) >> ((sz) - (bits))) & ((1 << (bits)) - 1))
# define MASK1BIT(cache, sz)  \
((cache) & (1 << ((sz) - 1)))

/*
* NAME:	MP3_DEC_huffdecode()
* DESCRIPTION:	decode Huffman code words of one channel of one granule
*/
# if defined(ASO_HUFFMAN)
extern int32 MP3_DEC_HuffmanParsingAsm(MP3_DEC_BIT_POOL_T *peek_ptr, 
									   int32             *tst_data_ptr,
									   int32             *xr_ptr);
#endif


static void MP3_DEC_CalcScaleFac(struct channel const *channel,
                          uint8 const *sfbwidth_ptr,
                          int32 *exp_ptr)
{
    int32  gain;
    uint32 scalefac_multiplier, sfbi;
    
    gain                = (int32) channel->global_gain - 210;
    scalefac_multiplier = (channel->flags & MP3_DEC_SCLAE_FACTOR) ? 2 : 1;
    
    if (channel->block_type == 2) {
        uint32 l;
        int32 gain0, gain1, gain2;		
        sfbi = l = 0;
        
        if (channel->flags & MP3_MIXED_BLOCK_FLAG)
        {
            uint32 premask;			
            premask = (channel->flags & MP3_DEC_PRE_FLAG) ? ~0 : 0;			
            /* long block subbands 0-1 */			
            while (l < 36)
            {
                exp_ptr[sfbi] = gain -
                    (int32) ((channel->scalefac[sfbi] + (MP3_DEC_pre_tab[sfbi] & premask)) <<
                    scalefac_multiplier);
                
                l += sfbwidth_ptr[sfbi++];
            }
        }
        
        /* this is probably wrong for 8000 Hz short/mixed blocks */
        
        gain0 = gain - 8 * (int32) channel->subblock_gain[0];
        gain1 = gain - 8 * (int32) channel->subblock_gain[1];
        gain2 = gain - 8 * (int32) channel->subblock_gain[2];
        
        while (l < 576) 
        {
            exp_ptr[sfbi + 0] = gain0 - (int32) (channel->scalefac[sfbi + 0] << scalefac_multiplier);
            exp_ptr[sfbi + 1] = gain1 - (int32) (channel->scalefac[sfbi + 1] << scalefac_multiplier);
            exp_ptr[sfbi + 2] = gain2 - (int32) (channel->scalefac[sfbi + 2] << scalefac_multiplier);
            
            l    += 3 * sfbwidth_ptr[sfbi];
            sfbi += 3;
        }
    }
    else 
    {  /* channel->block_type != 2 */
        if (channel->flags & MP3_DEC_PRE_FLAG) 
        {
            for (sfbi = 0; sfbi < 22; ++sfbi) 
            {
                exp_ptr[sfbi] = gain -(int32) ((channel->scalefac[sfbi] + MP3_DEC_pre_tab[sfbi]) << scalefac_multiplier);
            }
        }
        else 
        {
            for (sfbi = 0; sfbi < 22; ++sfbi) 
            {
                exp_ptr[sfbi] = gain - (int32) (channel->scalefac[sfbi] << scalefac_multiplier);
            }
        }
    }
}


/*
* FUNCTOIN NAME:	MP3_DEC_HuffIQDec
* FUNCTOIN DESCRIPTION:	decode Huffman code words and re-quantization processing of one channel of one granule
*/
int32 MP3_DEC_HuffIQDec(MP3_DEC_BIT_POOL_T *bit_pool_ptr, 
                        int32        *xr_in_ptr,
						struct channel     *channel_ptr,
						uint8 const        *sfbwidth_ptr,
						uint32              part2_length,
                        uint32              *region0_pos_ptr)
{
    int32 exponents_data[39], exp_val;
    MP3_DEC_BIT_POOL_T peek;
    int32 bits_left, cachesz;    
# if !defined(ASO_HUFFMAN)      
    int32 const *expptr;
    int32 *xrptr;
    int32 const *sfboundary_ptr;
# endif
    uint32 bitcache;
    int32 test_data[32] = {0};
    int32 addr;
    int32 rel = MP3_ERROR_NONE;
    peek = *bit_pool_ptr;

    bits_left = (int32) channel_ptr->part2_3_length - (int32) part2_length;
    if (bits_left < 0)
    {
        return MP3_ERROR_BADPART3LEN;    
    }
    MP3_DEC_CalcScaleFac(channel_ptr, sfbwidth_ptr, exponents_data);    
    MP3_DEC_BitSkip(bit_pool_ptr, bits_left);
    //////////////////////////////////////////////////////////////////////////
    cachesz    = peek.left;
    bitcache   = MP3_DEC_8BitRead(&peek, peek.left);
    bits_left -= cachesz;
    addr = (int32)peek.byte_ptr;
    addr &= 0x3;
    if (addr)
    {
        addr = (4 - addr) * 8;
        cachesz += addr;
        bitcache   = (bitcache << addr) | MP3_DEC_8BitRead(&peek, addr);
        bits_left -= addr;        
    }
    peek.word_ptr = (uint32*) peek.byte_ptr;
    peek.left     = 32;
    peek.cache    = 0;    
    //////////////////////////////////////////////////////////////////////////    
     
    test_data[0] = bits_left;
    test_data[1] = bitcache;
    test_data[2] = cachesz;
    test_data[3] = (int32)sfbwidth_ptr;
    test_data[4] = (int32)exponents_data;
    test_data[5] = (int32)channel_ptr;

    test_data[6] = channel_ptr->big_values;    
    test_data[8] = 0;
    /**/
    exp_val = MP3_DEC_HuffmanParsingAsm(&peek, 
        test_data,
        xr_in_ptr
        );
    region0_pos_ptr[0] = exp_val & 0xFFFF;
    rel = exp_val >> 16;
        

        
            

                    
                    
            
            
            
                    
                    
                    
                
                    
                    
                    
                
                    

  
      
      
          

          
              
          
          
      
  

  return rel;
}
# undef MASK
# undef MASK1BIT


/*
* NAME:	MP3_DEC_stereo()
* DESCRIPTION:	perform joint stereo processing on a granule
*/

static
int32  MP3_DEC_Stereo(mp3_fixed_t *xr[2]/*[576]*/,
						  struct granule const *granule,
						  MP3_HEADER_T *header,
						  unsigned char const *sfbwidth)
{
	short modes[39];
	unsigned int sfbi, l, n, i;
	
	if (granule->ch[0].block_type !=
		granule->ch[1].block_type ||
		(granule->ch[0].flags & mixed_block_flag) !=
		(granule->ch[1].flags & mixed_block_flag))
		return MP3_ERROR_BADSTEREO;
	
	for (i = 0; i < 39; ++i)
		modes[i] = header->mode_extension;
	
	/* intensity stereo */
	
	if (header->mode_extension & I_STEREO) {
		struct channel const *right_ch = &granule->ch[1];
		mp3_fixed_t const *right_xr = xr[1];
		unsigned int is_pos;
		
		header->flags |= MP3_FLAG_I_STEREO;
		
		/* first determine which scalefactor bands are to be processed */
		
		if (right_ch->block_type == 2) {
			unsigned int lower, start, max, bound[3], w;
			
			lower = start = max = bound[0] = bound[1] = bound[2] = 0;
			
			sfbi = l = 0;
			
			if (right_ch->flags & mixed_block_flag) {
				while (l < 36) {
					n = sfbwidth[sfbi++];
					
					for (i = 0; i < n; ++i) {
						if (right_xr[i]) {
							lower = sfbi;
							break;
						}
					}
					
					right_xr += n;
					l += n;
				}
				
				start = sfbi;
			}
			
			w = 0;
			while (l < 576) {
				n = sfbwidth[sfbi++];
				
				for (i = 0; i < n; ++i) {
					if (right_xr[i]) {
						max = bound[w] = sfbi;
						break;
					}
				}
				
				right_xr += n;
				l += n;
				w = (w + 1) % 3;
			}
			
			if (max)
				lower = start;
			
			/* long blocks */
			
			for (i = 0; i < lower; ++i)
				modes[i] = header->mode_extension & ~I_STEREO;
			
			/* short blocks */
			
			w = 0;
			for (i = start; i < max; ++i) {
				if (i < bound[w])
					modes[i] = header->mode_extension & ~I_STEREO;
				
				w = (w + 1) % 3;
			}
		}
		else {  /* right_ch->block_type != 2 */
			unsigned int bound;
			
			bound = 0;
			for (sfbi = l = 0; l < 576; l += n) {
				n = sfbwidth[sfbi++];
				
				for (i = 0; i < n; ++i) {
					if (right_xr[i]) {
						bound = sfbi;
						break;
					}
				}
				
				right_xr += n;
			}
			
			for (i = 0; i < bound; ++i)
				modes[i] = header->mode_extension & ~I_STEREO;
		}
		
		/* now do the actual processing */
		
		if (header->flags & MP3_FLAG_LSF_EXT) {
			unsigned char const *illegal_pos = granule[1].ch[1].scalefac;
			mp3_fixed_t const *lsf_scale;
			
			/* intensity_scale */
			lsf_scale = is_lsf_table[right_ch->scalefac_compress & 0x1];
			
			for (sfbi = l = 0; l < 576; ++sfbi, l += n) {
				n = sfbwidth[sfbi];
				
				if (!(modes[sfbi] & I_STEREO))
					continue;
				
				if (illegal_pos[sfbi]) {
					modes[sfbi] &= ~I_STEREO;
					continue;
				}
				
				is_pos = right_ch->scalefac[sfbi];
				
				for (i = 0; i < n; ++i) {
					register mp3_fixed_t left;
					
					left = xr[0][l + i];
					
					if (is_pos == 0)
						xr[1][l + i] = left;
					else {
						register mp3_fixed_t opposite;
						
						opposite = mp3_f_mul_stereo(left, lsf_scale[(is_pos - 1) / 2]);
						
						if (is_pos & 1) {
							xr[0][l + i] = opposite;
							xr[1][l + i] = left;
						}
						else
							xr[1][l + i] = opposite;
					}
				}
			}
		}
		else {  /* !(header->flags & MP3_FLAG_LSF_EXT) */
			for (sfbi = l = 0; l < 576; ++sfbi, l += n) {
				n = sfbwidth[sfbi];
				
				if (!(modes[sfbi] & I_STEREO))
					continue;
				
				is_pos = right_ch->scalefac[sfbi];
				
				if (is_pos >= 7) {  /* illegal intensity position */
					modes[sfbi] &= ~I_STEREO;
					continue;
				}
				
				for (i = 0; i < n; ++i) {
					register mp3_fixed_t left;
					
					left = xr[0][l + i];
					
					xr[0][l + i] = mp3_f_mul_stereo(left, is_table[    is_pos]);
					xr[1][l + i] = mp3_f_mul_stereo(left, is_table[6 - is_pos]);
				}
			}
		}
  }
  
  /* middle/side stereo */  
  if (header->mode_extension & MS_STEREO) {
	  register mp3_fixed_t invsqrt2;
	  
	  header->flags |= MP3_FLAG_MS_STEREO;
	  
	  invsqrt2 = root_table[3 + -2];
	  
	  for (sfbi = l = 0; l < 576; ++sfbi, l += n) {
		  n = sfbwidth[sfbi];
		  
		  if (modes[sfbi] != MS_STEREO)
			  continue;
		  
		  for (i = 0; i < n; ++i) {
			  register mp3_fixed_t m, s;
			  
			  m = xr[0][l + i];
			  s = xr[1][l + i];
			  
			  xr[0][l + i] = mp3_f_mul_stereo(m + s, invsqrt2);  /* l = (m + s) / sqrt(2) */
			  xr[1][l + i] = mp3_f_mul_stereo(m - s, invsqrt2);  /* r = (m - s) / sqrt(2) */
		  }
	  }
  }
  
  return MP3_ERROR_NONE;
}

/*
* NAME:	MP3_DEC_AliasReduce()
* DESCRIPTION:	perform frequency line alias reduction
*/

	

		
			

			
		
				
					




		



					


/*
* NAME:	MP3_DEC_overlap()
* DESCRIPTION:	perform overlap-add of windowed IMDCT outputs
*/
void MP3_DEC_ImdctWinOverlap(mp3_fixed_t *X, mp3_fixed_t overlap[32][/*18*/9],
							   mp3_fixed_t sample[18][32], unsigned int sblimit);
void MP3_DEC_AliasImdctWinOverlap(mp3_fixed_t *x, mp3_fixed_t *Overlap/*[32][9]*/,
									 mp3_fixed_t sample[18][32], unsigned int sblimit);
void MP3_DEC_ImdctWinOverlap1(mp3_fixed_t *x, mp3_fixed_t sample[18][32],
							  unsigned int sb, unsigned int sblimit, mp3_fixed_t *Overlap);




	

		
		
		
		
		

		


		
		
		
		



		
		

		
			

				









		

		
		
		
		

		

		

		

		
		
		
		
		

		

		
		
		



	

		
		
		
		
		

		


		
		
		
		



		
		

		
			

				









		

		
		
		
		

		

		

		

		
		
		
		
		

		

		
		




	 
	

			
		
		
			
			
		
		
			
			
		


			
		
			

		
		

			

	
		
		

		


		
			
			

		
		
			
			



		
		







		






		

		
		

		

		

		



		
		


	

		
		
		
		
		


		

		

		
		


	


/*lint -save -e649*/
#define window_s_0_5  ((MP3_F_WIN_1(0x0216a2a2)<<16)|((MP3_F_WIN_1(0x0fdcf549-0x10000000))&0xffff)) 
#define window_s_1_4  ((MP3_F_WIN_1(0x061f78aa)<<16)|((MP3_F_WIN_1(0x0ec835e8-0x10000000))&0xffff)) 
#define window_s_2_3  ((MP3_F_WIN_1(0x09bd7ca0-0x10000000)<<16)|((MP3_F_WIN_1(0x0cb19346-0x10000000))&0xffff)) 
#define rc_rd ((int32)((((int32)-26201)<<16)|(((int32)-21720)&0xffff)))
#define ra_rb ((int32)((((int32)-19955)<<16)|(((int32)-25080)&0xffff)))

void MP3_DEC_ImdctsWinOverlap(mp3_fixed_t  *X, mp3_fixed_t overlap[9],
						   mp3_fixed_t sample[18][32], unsigned int sb, unsigned int sblimit,
						   unsigned char const *sfbwidth)
{
	mp3_fixed_t t0, t1, t2, t3, t4;
	uint32 f, sfbw;
	uint32 lin = 0;  	
	sfbw = f = *sfbwidth;

	for (/*sb = 0*/; sb < sblimit; ++sb) 
	{		
		register int32 r0, r1, r2, r3, r4, r5, r6, r7;
		mp3_fixed_t X_tmp[18];		
		{
			unsigned int l;
			for(l=0; l < 6; l+=2)
			{				
				X_tmp[l]    = X[lin];				
				X_tmp[l+1]  = X[lin+1];
				X_tmp[l+6]  = X[lin + sfbw]; 	
				X_tmp[l+7]  = X[lin + sfbw + 1]; 
				X_tmp[l+12] = X[lin + 2*sfbw]; 	
				X_tmp[l+13] = X[lin + 2*sfbw + 1]; /*lint !e661*/
				lin+=2;		
				f-=2;
				if (f==0)
				{
					sfbwidth += 3;
					lin += 2*sfbw;
					sfbw = f = *sfbwidth;					
				}				
			}
		}
		
		t0 = overlap[7];
		t3 = overlap[6];
		t4 = overlap[8];				
		//////////////////////////////////////////////////////////////////////////		
		r0=X_tmp[1];		
		r1=X_tmp[4];
		r2=X_tmp[5];		
		
		r3=X_tmp[0];
		r4=X_tmp[2];		
		r5=X_tmp[3];
		r4= r4-r2;		
		
		r2=r4+r2*2;		
		r3=r5-r3;		
		
		r5=r3-r5*2;		
		r6=r0-r2;		
		r0=r2+r0*2;		
		r7=r1+r3;		
		r1=r1*2-r3;
		r2=mp3_f_mul_fixed16_t_inline (r4, rc_rd)/2 + r4 - mp3_f_mul_fixed16_inline(r5, rc_rd);	
		r3=mp3_f_mul_fixed16_t_inline (r5, rc_rd)/2 + mp3_f_mla_fixed16_inline(r4, rc_rd, r5);//+r5;	 		
		r4=(mp3_f_mla_fixed16_inline(r0, ra_rb, r1) + mp3_f_mul_fixed16_t_inline (r1, ra_rb)/4 )/2;	
		r5=(mp3_f_mul_fixed16_inline(r1, ra_rb) - mp3_f_mul_fixed16_t_inline (r0, ra_rb)/4 - r0)/2;		
		r0= mp3_f_mul_fixed16_inline(r7, ra_rb) - mp3_f_mul_fixed16_t_inline (r6, ra_rb)/4 - r6;			
		//yptr[1] = r0;			
		t1 =  mp3_f_mla_fixed16_inline(t0, window_s_1_4, t0);//+ t0;
		t2 =  mp3_f_mul_fixed16_t_inline (t0, window_s_1_4);
		/*sample[6+1][sb]*/t0 = mp3_f_mla_fixed16_t_inline(r0 , window_s_1_4, t1);//+t1;
		sample[7][sb] = (sb&1)?-t0:t0;
		sample[11-1][sb] = -mp3_f_mla_fixed16_inline(r0 , window_s_1_4, r0)+t2;	
		/*yptr[4]*/t0 = mp3_f_mul_fixed16_t_inline (r7, ra_rb)/4 + mp3_f_mla_fixed16_inline(r6, ra_rb, r7);//+r7;	
		
		r5 =r5-r3;		
		r3=r5+r3*2;			
		t1 =  mp3_f_mla_fixed16_inline(t3, window_s_0_5, t3);//+t3;
		t2 =  mp3_f_mul_fixed16_t_inline (t3, window_s_0_5);
		sample[6+0][sb] = mp3_f_mla_fixed16_t_inline(r5, window_s_0_5, t1);//+t1;
		/*sample[11-0][sb]*/t3 = -mp3_f_mla_fixed16_inline(r5, window_s_0_5, r5)+t2;	
		sample[11][sb] = (sb&1)?-t3:t3;
		/*yptr[5]*/t3=r3;	
		
		r4=r4+r2;		
		r2=r2*2-r4;			
		t1 =  mp3_f_mla_fixed16_inline(t4, window_s_2_3, t4);// + t4;
		t2 =  mp3_f_mla_fixed16_t_inline(t4, window_s_2_3, t4);// + t4;
		sample[6+2][sb] = mp3_f_mla_fixed16_t_inline(r4, window_s_2_3, r4)+t1;
		/*sample[11-2][sb] */t4= -mp3_f_mla_fixed16_inline(r4, window_s_2_3, r4)+t2;	
		sample[9][sb] = (sb&1)?-t4:t4;
		/*yptr[3]*/t4=r2;	
		
		//////////////////////////////////////////////////////////////////////////
		
		r0=X_tmp[1+6];		
		r1=X_tmp[4+6];
		r2=X_tmp[5+6];		
		
		r3=X_tmp[0+6];
		r4=X_tmp[2+6];		
		r5=X_tmp[3+6];
		r4= r4-r2;		
		
		r2=r4+r2*2;		
		r3=r5-r3;		
		
		r5=r3-r5*2;		
		r6=r0-r2;		
		
		r0=r2+r0*2;		
		r7=r1+r3;		
		r1=r1*2-r3;
		r2=mp3_f_mul_fixed16_t_inline (r4, rc_rd)/2 + r4 - mp3_f_mul_fixed16_inline(r5, rc_rd);	
		r3=mp3_f_mul_fixed16_t_inline (r5, rc_rd)/2 + mp3_f_mla_fixed16_inline(r4, rc_rd, r5);	 		
		r4=(mp3_f_mla_fixed16_inline(r0, ra_rb, r1) + mp3_f_mul_fixed16_t_inline (r1, ra_rb)/4)/2;	
		r5=(mp3_f_mul_fixed16_inline(r1, ra_rb) - mp3_f_mul_fixed16_t_inline (r0, ra_rb)/4 - r0)/2;
		r0= mp3_f_mul_fixed16_inline(r7, ra_rb) - mp3_f_mul_fixed16_t_inline (r6, ra_rb)/4 - r6;					
		t1 = mp3_f_mla_fixed16_inline(t0, window_s_1_4, t0);//+t0;
		t2 = mp3_f_mul_fixed16_t_inline (t0, window_s_1_4);
		/*yptr[1-3+6]sample[13][sb]*/ t0= mp3_f_mla_fixed16_t_inline(r0, window_s_1_4, t1);//+t1;
		sample[13][sb] = (sb&1)?-t0:t0;
		/*	yptr[1+6]*/sample[16][sb] = -mp3_f_mla_fixed16_inline(r0, window_s_1_4,r0)+t2;
		/*yptr[4+6]*/t0 = mp3_f_mul_fixed16_t_inline (r7, ra_rb)/4 + mp3_f_mla_fixed16_inline(r6, ra_rb, r7);			
		
		
		r5 =r5-r3;		
		r3=r5+r3*2;		
		t1 = mp3_f_mla_fixed16_inline(t4, window_s_0_5,t4);//+t4;
		t2 = mp3_f_mul_fixed16_t_inline (t4, window_s_0_5);
		/*yptr[0-3+6]*/sample[12][sb] = mp3_f_mla_fixed16_t_inline(r5, window_s_0_5,t1);
		/*yptr[0+6]sample[17][sb]*/t4 = -mp3_f_mla_fixed16_inline(r5, window_s_0_5,r5)+t2;			
		sample[17][sb] = (sb&1)?-t4:t4;
		/*yptr[5+6]*/t4=r3;	//	yptr[0+6]=r5;				
		
		r4=r4+r2;		
		r2=r2*2-r4;		
		t1 = mp3_f_mla_fixed16_inline(t3, window_s_2_3, t3);
		t2 = mp3_f_mla_fixed16_t_inline(t3, window_s_2_3, t3);
		/*yptr[2-3+6]*/sample[14][sb] = mp3_f_mla_fixed16_t_inline(r4, window_s_2_3,r4)+t1;
		/*yptr[2+6]sample[15][sb]*/t3 = -mp3_f_mla_fixed16_inline(r4, window_s_2_3,r4)+t2;			
		sample[15][sb] = (sb&1)?-t3:t3;
		/*yptr[3+6]*/t3=r2;		//yptr[2+6]=r4;				
		
		
		//////////////////////////////////////////////////////////////////////////		
		r0=X_tmp[1+12];		
		r1=X_tmp[4+12];
		r2=X_tmp[5+12];		
		
		r3=X_tmp[0+12];
		r4=X_tmp[2+12];		
		r5=X_tmp[3+12];
		r4= r4-r2;		
		
		r2=r4+r2*2;		
		r3=r5-r3;		
		
		r5=r3-r5*2;		
		r6=r0-r2;		
		
		
		r0=r2+r0*2;		
		r7=r1+r3;		
		r1=r1*2-r3;
		r2=mp3_f_mul_fixed16_t_inline (r4, rc_rd)/2 + r4 - mp3_f_mul_fixed16_inline(r5, rc_rd);	
		r3=mp3_f_mul_fixed16_t_inline (r5, rc_rd)/2 + mp3_f_mla_fixed16_inline(r4, rc_rd, r5);	 		
		r4=(mp3_f_mla_fixed16_inline(r0, ra_rb, r1) + mp3_f_mul_fixed16_t_inline (r1, ra_rb)/4)/2;	
		r5=(mp3_f_mul_fixed16_inline(r1, ra_rb) - mp3_f_mul_fixed16_t_inline (r0, ra_rb)/4 - r0)/2;		
		/*yptr[4+12]*/overlap[ 7] = mp3_f_mul_fixed16_t_inline (r7, ra_rb)/4 + mp3_f_mla_fixed16_inline(r6, ra_rb, r7);	
		r0= mp3_f_mul_fixed16_inline(r7, ra_rb) - mp3_f_mul_fixed16_t_inline (r6, ra_rb)/4 - r6;					
		
		sample[4][sb] = overlap[4];
		sample[1][sb] = (sb&1)?-overlap[1]:overlap[1];	
		t1 = mp3_f_mla_fixed16_inline(t0, window_s_1_4,t0);
		t2 = mp3_f_mul_fixed16_t_inline (t0, window_s_1_4);
		/*yptr[1-3+12]*/overlap[ 1] = mp3_f_mla_fixed16_t_inline(r0, window_s_1_4,t1);
		/*yptr[1+12]*/ overlap[ 4]= -mp3_f_mla_fixed16_inline(r0, window_s_1_4,r0)+t2;			
				
		r5 =r5-r3;		
		r3=r5+r3*2;
		/*yptr[5+12]*/overlap[ 8]=r3;				
		
		sample[0][sb] = overlap[0];
		sample[5][sb] = (sb&1)?-overlap[5]:overlap[5];	
		t1 = mp3_f_mla_fixed16_inline(t3, window_s_0_5, t3);
		t2 = mp3_f_mul_fixed16_t_inline (t3, window_s_0_5);
		/*yptr[0-3+12]*/overlap[ 0] = mp3_f_mla_fixed16_t_inline(r5, window_s_0_5,t1);
		/*yptr[0+12]*/overlap[ 5] = -mp3_f_mla_fixed16_inline(r5, window_s_0_5,r5)+t2;
		
		r4=r4+r2;		
		r2=r2*2-r4;
		/*yptr[3+12]*/overlap[ 6]=r2;
		
		sample[2][sb] = overlap[2];
		sample[3][sb] = (sb&1)?-overlap[3]:overlap[3];	
		t1 = mp3_f_mla_fixed16_inline(t4, window_s_2_3, t4);
		t2 = mp3_f_mla_fixed16_t_inline(t4, window_s_2_3, t4);
		/*	yptr[2-3+12]*/overlap[2] = mp3_f_mla_fixed16_t_inline(r4, window_s_2_3, r4) +t1;
		/*	yptr[2+12] */overlap[ 3]= -mp3_f_mla_fixed16_inline(r4, window_s_2_3, r4) +t2;			
				
		overlap += 9;		
	}
}
/*lint -restore */
/*
* NAME:	MP3_DEC_overlap_z()
* DESCRIPTION:	perform "overlap-add" of zero IMDCT outputs
*/
#ifndef WIN32
extern void MP3_DEC_OverlapZ0Asm(mp3_fixed_t *overlap_ptr,
                                                              mp3_fixed_t *sample_ptr, unsigned int sb);
#define MP3_DEC_OverlapZ0              MP3_DEC_OverlapZ0Asm                                                 
#else
#endif

static void MP3_DEC_OverlapZ1(mp3_fixed_t overlap[/*18*/9],
				   mp3_fixed_t sample[18][32], unsigned int sb)
{
	unsigned int i;
	mp3_fixed_t t0, t3, z;
	t3 = sb;
	z = 0;

	{	
		for (i =  0; i <  6; i+=2) 	
		{
			sample[i][sb] = overlap[i];	
			sample[i+1][sb] = (sb&1)?-overlap[i+1]:overlap[i+1];	
		}

		for(i = 0; i < 3; i++)
		{
			t0 = overlap[6+i]; /*X[2]*/
			sample[6+i][sb] =  mp3_f_mul_fixed16_inline(t0, window_s[5-i]) << 1;
			sample[11-i][sb] =  mp3_f_mul_fixed16_inline(t0, window_s[i]) << 1;
		}

		if (sb&1)
		{
			sample[7][sb] = -sample[7][sb];
			sample[9][sb] = -sample[9][sb];
			sample[11][sb] = -sample[11][sb];
		}
		
	
		for (i =  12; i <  18; ++i) 
		{
			sample[i][sb] = 0;
		}

		for (i = 0; i < 9; i ++)	overlap[   i]	  = 0;
	}
}

/*
* NAME:	MP3_DEC_decode()
* DESCRIPTION:	decode frame main_data
*/
#include <stdio.h>
extern FILE *fp_dbg;
static
int32  MP3_DEC_Decode(MP3_DEC_BIT_POOL_T *ptr, MP3_FRAME_T *frame,
						  SIDE_INFO_T *si, uint32 nch)
{
	MP3_HEADER_T *header = &frame->header;
	unsigned int sfreqi, ngr, gr;
	
	sfreqi = header->sfreqi;
	/* scalefactors, Huffman decoding, requantization */
	ngr = (header->flags & MP3_FLAG_LSF_EXT) ? 1 : 2;
	for (gr = 0; gr < ngr; ++gr) 
	{
		struct granule *granule = &si->gr[gr];
	unsigned char const *sfbwidth[2] = {NULL};
	//	mp3_fixed_t xr[2][576];
		mp3_fixed_t *xr[2];
		uint32 ch, xrlimit[2] = {0};
		int32 error;		

		xr[0] = frame->xr[0]; //&frame->sbsample[1][18][0];
		xr[1] = frame->xr[1]; //(gr == 0)?&frame->sbsample[0][18][0]:&frame->sbsample[1][0][0];


		for (ch = 0; ch < nch; ++ch) 
		{
			struct channel *channel = &granule->ch[ch];
			unsigned int part2_length;
			
			sfbwidth[ch] = sfbwidth_table[sfreqi].l;
			if (channel->block_type == 2) {
				sfbwidth[ch] = (channel->flags & mixed_block_flag) ?
					sfbwidth_table[sfreqi].m : sfbwidth_table[sfreqi].s;
			}
			
			if (header->flags & MP3_FLAG_LSF_EXT) {
				part2_length = MP3_DEC_ScalefactorsLsf(ptr, channel,
					ch == 0 ? 0 : &si->gr[1].ch[1],
					header->mode_extension);
			}
			else {
				part2_length = MP3_DEC_Scalefactors(ptr, channel, &si->gr[0].ch[ch],
					gr == 0 ? 0 : si->scfsi[ch]);
			}
			
			error = MP3_DEC_HuffIQDec(ptr, xr[ch], channel, sfbwidth[ch], part2_length, &xrlimit[ch]);
			if (error)
				return error;
		}

#if 0//def __DEBUG__
#if 1
#ifndef __DSPPLATFORM__
		{
			int32 i;	
			
			for(i = 0; i < 576; i++)
			{				
				uint16 d0;
				d0 = (uint16)(xr[1][i]>>16);
				fprintf(fp_dbg,"0x%04x\n",(uint16)d0);				
				d0 = (uint16)(xr[1][i]&0x0000ffff);
				fprintf(fp_dbg,"0x%04x\n",(uint16)d0);		
			}		  
		}
#else
		{		  
			int32 i;
			for (i = 0; i < 576; i++)
			{				
				uint16 d0;
				d0 = (uint16)(xr[0][i]>>16);			  
				*(uint16 *)(DEBUGPORT) = d0;	
				d0 = (uint16)(xr[0][i]&0x0000ffff);
				*(uint16 *)(DEBUGPORT) = d0;					
			}
		}
#endif
#else
#ifndef __DSPPLATFORM__
		{
			int32 i,j;		
			for(i = 0; i < 18; i++)
			{			
				for(j = 0; j <32; j++)
				{
					uint16 d0;
					d0 = (uint16)(frame->sbsample[1][i][j]>>16);
					fprintf(fp_dbg,"0x%04x\n",(uint16)d0);				
					d0 = (uint16)(frame->sbsample[1][i][j]&0x0000ffff);
					fprintf(fp_dbg,"0x%04x\n",(uint16)d0);	
				}			
			}		  
		}
#else
		{		  
			int32 i,j;		
			for(i = 0; i < 18; i++)
			{			
				for(j = 0; j <32; j++)
				{
					uint16 d0;
					d0 = (uint16)(frame->sbsample[0][i*32+j]>>16);
					*(uint16 *)(DEBUGPORT) = d0;	
					d0 = (uint16)(frame->sbsample[0][i*32+j]&0x0000ffff);
					*(uint16 *)(DEBUGPORT) = d0;					
				}
			}
		}
#endif
#endif
#endif
		
		/* joint stereo processing */
		if (header->mode == MP3_MODE_JOINT_STEREO && header->mode_extension) 
		{
			xrlimit[0] = (xrlimit[1] = ((xrlimit[0] > xrlimit[1])?xrlimit[0]:xrlimit[1]));
			error = MP3_DEC_Stereo(xr, granule, header, sfbwidth[0]);
			if (error)
				return error;
		}
		
		/* reordering, alias reduction, IMDCT, overlap-add, frequency inversion */		
		for (ch = 0; ch < nch; ++ch) 
		{
                        int16 ch_bak;
			struct channel const *channel = &granule->ch[ch];
			mp3_fixed_t (*sample)[32] = &frame->sbsample[ch][18 * gr];
			unsigned int sb, l, i, sblimit, sblimit_prev;
			unsigned int line_limit = 576;
			
			{
				sblimit = 32 - (576 - xrlimit[ch]) / 18;
				line_limit = sblimit*18;
				if (((line_limit - xrlimit[ch]) <= 8)&&(sblimit != 32))
				{
					sblimit++;
					line_limit += 18;
				}		
				
				sblimit_prev = frame->sblimit_prev[ch];
				frame->sblimit_prev[ch] = sblimit;
			}
				
			
			switch(channel->block_type)
			{
			case 0:
			case 1:
				sb = sblimit;
				l = line_limit;
				MP3_DEC_AliasImdctWinOverlap(&xr[ch][l-18], &frame->overlap[ch][sb-1][0], sample, sb-1);				
				if (sblimit < sblimit_prev)
				{
				        mp3_fixed_t *over_lap_ptr;
#ifdef TEST_DATA
					FILE *fp = fopen("E:\\AAC_test_data\\mp3_test\\arm_test.dat", "a+");
					fprintf(fp, "frame number: %d, start_sb: %d\n", g_fr_cnt, sblimit);
#endif
				        over_lap_ptr = frame->overlap[ch][sb];
					for (; sb < sblimit_prev; ++sb)										
					{
						MP3_DEC_OverlapZ0(over_lap_ptr, sample, sb);
#ifdef TEST_DATA
                                                if (1 == ch)
        						fprintf(fp, "sb: %4d, %8d, %8d, %8d, %8d, %8d, %8d, %8d, %8d, %8d\n", sb,over_lap_ptr[0], over_lap_ptr[1], over_lap_ptr[2], over_lap_ptr[3], over_lap_ptr[4], over_lap_ptr[5], over_lap_ptr[6], over_lap_ptr[7], over_lap_ptr[8]);
#endif
						over_lap_ptr += 9;
					}
#ifdef TEST_DATA
					fclose(fp);
#endif
				}
				break;
			case 2:
				{
					sb = 0;
					l = 0;
					if (channel->flags & mixed_block_flag)
					{
						//MP3_DEC_AliasReduce(xr[ch], 36);	
						//MP3_DEC_ImdctWinOverlap(&xr[ch][0], frame->overlap[ch], sample, 2);					
						MP3_DEC_AliasImdctWinOverlap(&xr[ch][18], &frame->overlap[ch][1][0], sample, 1);				
						sb = 2;
						l = 0;
						while (l < 36)
						{
							l += *sfbwidth[ch]++;					
						}
						l = 36;
					}
					
					MP3_DEC_ImdctsWinOverlap(&xr[ch][l], frame->overlap[ch][sb], sample, sb, sblimit, sfbwidth[ch]);
					if (1 == ch)
					{
						int16 sb;
#ifdef TEST_DATA
						FILE *fp = fopen("E:\\AAC_test_data\\mp3_test\\ARM_2_over_lap.dat", "a+");
						fprintf(fp, "frame number: %d,\n", g_fr_cnt);
#endif
						for (sb = 0; sb < 32; ++sb)		
						{
#ifdef TEST_DATA
							fprintf(fp, "sb: %4d, right: %12d, %12d, %12d, %12d, %12d, %12d, %12d, %12d, %12d\n", sb, 								
								frame->overlap[1][sb][0], frame->overlap[1][sb][1], frame->overlap[1][sb][2], frame->overlap[1][sb][3],
								frame->overlap[1][sb][4], frame->overlap[1][sb][5], frame->overlap[1][sb][6], frame->overlap[1][sb][7], frame->overlap[1][sb][8]);
#endif
						}
#ifdef TEST_DATA
						fclose(fp);
#endif
					}
					
				
					sb = sblimit;
					if (sblimit < sblimit_prev)
					{					
						for (/*sb = sblimit*/; sb < sblimit_prev; ++sb)										
							MP3_DEC_OverlapZ1(frame->overlap[ch][sb], sample, sb);
					}
				}
				break;
				
			case 3:				
			        //ch_bak = ch;
				MP3_DEC_AliasReduce(xr[ch], /*576*/line_limit);									
				
				sb = 0;
				l = 0;
				if (channel->flags & mixed_block_flag)
				{					
					MP3_DEC_ImdctWinOverlap(&xr[ch][0], frame->overlap[ch], sample, 2);
					l = 36;
					sb = 2;
				}

				
				MP3_DEC_ImdctWinOverlap1(&xr[ch][l], sample, sb, sblimit, &frame->overlap[ch][sb][0]);
			        //ch = ch_bak;
			

				sb = sblimit;
				if (sblimit < sblimit_prev)
				{					
					for (/*sb = sblimit*/; sb < sblimit_prev; ++sb)										
						MP3_DEC_OverlapZ1(frame->overlap[ch][sb], sample, sb);
				}
				break;
			default:
				break;
			}

			/* remaining (zero) subbands */			
			for (/*sb = sblimit*/; sb < 32; ++sb)/*lint !e644*/
			{
				for (i =  0; i <  9; ++i) 
				{
					sample[2*i][sb] = 0;
					sample[2*i+1][sb] = 0;
					frame->overlap[ch][sb][   i]	  = 0;
				}					
			}
		}
  }
  if (0)
  {
	  int16 sb, ch;
#ifdef TEST_DATA
	  FILE *fp = fopen("E:\\AAC_test_data\\mp3_test\\ARM_over_lap.dat", "wb");
	  fprintf(fp, "frame number: %d,\n", g_fr_cnt);
#endif
	  ch = 1;
	  for (sb = 0; sb < 32; ++sb)		
	  {
#ifdef TEST_DATA
		  fprintf(fp, "sb: %4d, left: %12d, %12d, %12d, %12d, %12d, %12d, %12d, %12d, %12d,   right: %12d, %12d, %12d, %12d, %12d, %12d, %12d, %12d, %12d\n", sb, 
			  frame->overlap[0][sb][0], frame->overlap[0][sb][1], frame->overlap[0][sb][2], frame->overlap[0][sb][3],
			  frame->overlap[0][sb][4], frame->overlap[0][sb][5], frame->overlap[0][sb][6], frame->overlap[0][sb][7], frame->overlap[0][sb][8],
			  frame->overlap[1][sb][0], frame->overlap[1][sb][1], frame->overlap[1][sb][2], frame->overlap[1][sb][3],
			  frame->overlap[1][sb][4], frame->overlap[1][sb][5], frame->overlap[1][sb][6], frame->overlap[1][sb][7], frame->overlap[1][sb][8]);
#endif
	  }
#ifdef TEST_DATA
	  fclose(fp);
#endif


  }

  return MP3_ERROR_NONE;
}


/*
* NAME:	layer->III()
* DESCRIPTION:	decode a single Layer III frame
*/
int32 MP3_DEC_LayerIII(MP3_STREAM_T *stream, MP3_FRAME_T *frame)
{
	MP3_HEADER_T *header = &frame->header;
	unsigned int nch, priv_bitlen, next_md_begin = 0;
	unsigned int si_len, data_bitlen, md_len;
	unsigned int frame_space, frame_used, frame_free;
	MP3_DEC_BIT_POOL_T ptr;
	SIDE_INFO_T si;
	int32 error;
	int result = 0;	

	nch = header->num_ch;
	si_len = header->si_len;
	
	/* check CRC word */
	if (header->flags & MP3_FLAG_PROTECTION) {
		header->crc_check =
			MP3_DEC_BIT_CRCCheck(stream->ptr, si_len * CHAR_BIT, header->crc_check);
		
		if (header->crc_check != header->crc_target &&
			!(frame->options & MP3_OPTION_IGNORECRC)) {
			stream->error = MP3_ERROR_BADCRC;
			result = -1;
		}
	}
	
	/* decode frame side information */
	error = MP3_DEC_SideInfoParse(&stream->ptr, nch, header->flags & MP3_FLAG_LSF_EXT,
		&si, (uint32 *)&data_bitlen, (uint32 *)&priv_bitlen);
	if (error && result == 0) {
		stream->error = error;
		result = -1;
	}
	
	header->flags        |= priv_bitlen;
	header->private_bits |= si.private_bits;
	
	next_md_begin = header->next_md_begin;
	
	/* find main_data of this frame */
	frame_space = stream->next_frame - MP3_DEC_BitNextByte(&stream->ptr);
	if (next_md_begin > si.main_data_begin + frame_space)
		next_md_begin = 0;
	
	md_len = si.main_data_begin + frame_space - next_md_begin;
	
	frame_used = 0;
	if (si.main_data_begin == 0) {
		ptr = stream->ptr;
		stream->md_len = 0;
		
		frame_used = md_len;
	}
	else {
		if (si.main_data_begin > stream->md_len) {
			if (result == 0) {
				stream->error = MP3_ERROR_BADDATAPTR;
				result = -1;
			}
		}
		else {
			MP3_DEC_BitPoolInit(&ptr,
				stream->main_data + stream->md_len - si.main_data_begin);
			
			if (md_len > si.main_data_begin) {
				if(stream->md_len + md_len -
					si.main_data_begin > MP3_BUFFER_MDLEN)
				{
					stream->error = MP3_ERROR_BADDATAPTR;
					result = -1;
					return result;
				}
				
				memcpy(stream->main_data + stream->md_len,
					MP3_DEC_BitNextByte(&stream->ptr),
					frame_used = md_len - si.main_data_begin);
				stream->md_len += frame_used;
			}
		}
	}
	frame_free = frame_space - frame_used;
	
	
	/* decode main_data */
	if (result == 0) {
		error = MP3_DEC_Decode(&ptr, frame, &si, nch);
		if (error) 
		{		
			stream->error = error;
			result = -1;
		}
		
		/* designate ancillary bits */		
		stream->anc_ptr    = ptr;
		stream->anc_bitlen = md_len * CHAR_BIT - data_bitlen;
		
	}


	
	/* preload main_data buffer with up to 511 bytes for next frame(s) */
	if (frame_free >= next_md_begin) {
		memcpy(stream->main_data,
			stream->next_frame - next_md_begin, next_md_begin);
		stream->md_len = next_md_begin;
	}
	else {
		if (md_len < si.main_data_begin) {
			unsigned int extra;
			
			extra = si.main_data_begin - md_len;
			if (extra + frame_free > next_md_begin)
				extra = next_md_begin - frame_free;
			
			if (extra < stream->md_len) {
				memmove(stream->main_data,
					stream->main_data + stream->md_len - extra, extra);
				stream->md_len = extra;
			}
		}
		else
			stream->md_len = 0;
		
		memcpy(stream->main_data + stream->md_len,
			stream->next_frame - frame_free, frame_free);
		stream->md_len += frame_free;
	}
	
	
	return result;
}













































