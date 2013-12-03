/*************************************************************************
** File Name:      sbr_hfgen.c                                           *
** Author:         Reed zhang                                            *
** Date:           17/01/2006                                            *
** Copyright:      2001 Spreatrum, Incoporated. All Rights Reserved.     *
** Description:    High Frequency generation                             *
**                        Edit History                                   *
** ----------------------------------------------------------------------*
** DATE           NAME             DESCRIPTION                           *
** 17/01/2006     Reed zhang       Create.                               *                                          
** 18/01/2006     Reed zhang       modify and rewrite function           *
                                   auto_correlation fixed-point code     *
** 19/01/2006     Reed zhang       testing and checking auto_correlation *
**                                 fixed-point code                      *
** 10/04/2006     Reed zhang       add sbr_ptr-high quality mode             *
**************************************************************************/
#include "aac_common.h"
#include "aac_structs.h"
#include "AAC_sbr_common.h"
#ifdef AAC_SBR_DEC
#include "AAC_sbr_hfgen.h"
/* Real_AAC */
#define AAC_LP_FBITS_COEF_BITS	29	/* Q29 for data range of (-4, 4) */
#define AAC_LP_MAG16			 (16 * (1 << (32 - (2*(32-AAC_LP_FBITS_COEF_BITS)))))		/* i.e. 16 in Q26 format */
#define AAC_LP_RELAX_COEF		 0x7ffff79c	/* 1.0 / (1.0 + 1e-6), Q31 */
#define NUM_TIME_SLOTS			16
#define SAMPLES_PER_SLOT		2	/* RATE in spec */
#define NUM_SAMPLE_RATES_SBR	9	/* downsampled (single-rate) mode unsupported, so only use Fs_sbr >= 16 kHz */
#define FBITS_IN_QMFA	14
#define FBITS_LOST_QMFA	(1 + 2 + 3 + 2 + 1)	/* 1 from cTab, 2 in premul, 3 in FFT, 2 in postmul, 1 for implicit scaling by 2.0 */
#define AAC_DEC_BITS_OUT_QMFA	(FBITS_IN_QMFA - FBITS_LOST_QMFA)
#define Q28_2	0x20000000	/* Q28: 2.0 */
#define Q28_15	0x30000000	/* Q28: 1.5 */
#define NUM_ITER_IRN		5



extern const uint8 g_goalSbTab[];

/* static function declarations */
static void AAC_SbrLPGenCalcPredictionFilterCoefs(int32 *XBuf_ptr,
                                  int32 *a0re_ptr);

static int32 AAC_SBR_GenCovCalc(int32 *Xbuf_ptr,
                                                                  int32 *p01reN_ptr, 
                                                                  int32 *p01imN_ptr, 
                                                                  int32 *p12reN_ptr, 
                                                                  int32 *p12imN_ptr, 
                                                                  int32 *p11reN_ptr, 
                                                                  int32 *p22reN_ptr);

static int32 AAC_SBR_GenCovCalc1(int32 *XBuf_ptr, 
                                                                    int32 *p02reN_ptr, 
                                                                    int32 *p02imN_ptr);
static __inline int32 AAC_SBR_FASTABS(int32 x);                                                                    
int32 AAC_InvRNormalized(int32 r);                                                                    

static void AAC_CalcChirpFactors(AAC_SBR_INFO_T *sbr_ptr,
							   uint8 ch);
static void AAC_PatchConstruction(AAC_SBR_INFO_T *sbr_ptr);
//////////////////////////////////////////////////////////////////////////

/* ARM paltform  */
static __inline int32 AAC_DEC_CLZ(int32 x)
{
	int32 j = 0;
/*@jgdu	
	__asm{
		clz j, x
	}
*/	
        __asm__("clz %0,%1":"=&r"(j):"r"(x):"cc");
	//j = 32 - j;
	return j;
}
extern void AAC_SBRGenConvCalculate(int32 *XBuf, int32 *accBuf);
extern void AAC_SBRGenConvCalculate1(int32 *XBuf, int32 *accBuf);
#endif

extern void  AAC_SBR_HfGenerateAsm(AAC_SBR_INFO_T *sbr_ptr,
    	                                                aac_complex              *Xlow,
    	                                                uint8                             ch,
    	                                                int32                           *alpha_ptr);
void AAC_HfGeneration(AAC_SBR_INFO_T *sbr_ptr,                           // sbr_ptr relative info. for High frequency generation
                                        aac_complex    Xlow[MAX_NTSRHFG][64],    // sbr_ptr LOW frequency data, fixed-point:  S21.0                   
                                        uint8 ch,
                                        int32 *shared_buffer_ptr)
{
    // GLOBAL VARIANT aac_complex
    int32 *alpha_0_r = shared_buffer_ptr + 128;  // data format:   alpha_0_r,  alpha_0_i, alpha_1_r,  alpha_1_i
    uint8 noPatches;
    int16 i,  conv_skip = 0;        
    uint8 kx     = sbr_ptr->kx;
    int16 start_p = 0;
	
    /* calc the bwArray data and the data fixed-point: S3.12 */
    /* reed modify at 2006-01-17 */
    AAC_CalcChirpFactors(sbr_ptr, ch);
    /* reed modify at 2006-01-17 */
    if ((ch == 0) && (sbr_ptr->Reset))
        AAC_PatchConstruction(sbr_ptr);	
    noPatches = sbr_ptr->noPatches;
    /* calc the parameter alpha0 and alpha1*/
    start_p = sbr_ptr->patchStartSubband[0];
    if (sbr_ptr->bwArray[ch][0] || sbr_ptr->bwArray[ch][1] || sbr_ptr->bwArray[ch][2] || sbr_ptr->bwArray[ch][3] || sbr_ptr->bwArray[ch][4])
    {
        conv_skip = 1;
    }
    for (i = 1; i < noPatches; i++)
    {
        if (start_p > sbr_ptr->patchStartSubband[i])
       {
            start_p = sbr_ptr->patchStartSubband[i];
        }
    }
    if (conv_skip)
    {
       alpha_0_r += 4*start_p;
       for (i = start_p; i < kx; i++)
       {
           AAC_SbrLPGenCalcPredictionFilterCoefs(Xlow[0][i], alpha_0_r);
           alpha_0_r +=4;// S3.29
       }
       alpha_0_r -= 4*kx;
    }
#if 0//def GEN_TEST_DATA
    if ((0 == ch) && (g_frm_counter > TEST_FRAME))
    {
        FILE *fp = fopen("gen_appha", "wb");			
        for (i = start_p; i < kx; i++)
        {
            fprintf(fp, "%5d, %5d, %5d, %5d, \n", alpha_0_r[4*i+0], alpha_0_r[4*i+1], alpha_0_r[4*i+2], alpha_0_r[4*i+3]);
        }
        fclose(fp);
    }
#endif       
    /* actual HF generation */
    AAC_SBR_HfGenerateAsm(sbr_ptr, Xlow[2], ch, alpha_0_r);   

}
static void AAC_SbrLPGenCalcPredictionFilterCoefs(int32 *Xbuf_ptr, 
                                                                                   int32 *a0_re_ptr)
{
    int32 sign, bit1, bit2, nd, d, dInv, tre, tim;
    int32 c01_re, c01_im, c02_re, c02_im, c12re, c12_im, c11_re, c22_re;
    /* calculate covariance coefficients */
    bit1 = AAC_SBR_GenCovCalc(Xbuf_ptr, 
                                                      &c01_re, 
                                                      &c01_im, 
                                                      &c12re, 
                                                      &c12_im, 
                                                      &c11_re, 
                                                      &c22_re);
    bit2 = AAC_SBR_GenCovCalc1(Xbuf_ptr, 
                                                        &c02_re, 
                                                        &c02_im);
		
    /* normalize everything to larger power of 2 scalefactor, call it bit1 */
    if (bit1 < bit2)
    {
        nd = AAC_DEC_MIN(bit2 - bit1, 31);
        c01_re >>= nd;	
        c01_im >>= nd;
        c12re >>= nd;	
        c12_im >>= nd;
        c11_re >>= nd;	
        c22_re >>= nd;
        bit1 = bit2;
    }
    else if (bit1 > bit2) 
    {
        nd = AAC_DEC_MIN(bit1 - bit2, 31);
        c02_re >>= nd;	
        c02_im >>= nd;
    }
    /* calculate determinant of covariance matrix (at least 1 GB in pXX) */
    d = AAC_DEC_MultiplyShiftR32(c12re, c12re) + AAC_DEC_MultiplyShiftR32(c12_im, c12_im);
    d = AAC_DEC_MultiplyShiftR32(d, AAC_LP_RELAX_COEF) << 1;
    d = AAC_DEC_MultiplyShiftR32(c11_re, c22_re) - d;
    //ASSERT(d >= 0);	/* should never be < 0 */
    sign = 0;
    a0_re_ptr[0] = 0;
    a0_re_ptr[1] = 0;
    a0_re_ptr[2] = 0;
    a0_re_ptr[3] = 0;
    //*a0_im_ptr  = 0;
    //*a1_re_ptr  = 0;
    //*a1_im_ptr  = 0;
    if (d > 0) 
    {
        /* input =   Q31  d    = Q(-2*bit1 - 32 + nd) = Q31 * 2^(31 + 2*bit1 + 32 - nd)
         * inverse = Q29  dInv = Q29 * 2^(-31 - 2*bit1 - 32 + nd) = Q(29 + 31 + 2*bit1 + 32 - nd)
         *
         * numerator has same Q format as d, since it's sum of normalized squares
         * so num * inverse = Q(-2*bit1 - 32) * Q(29 + 31 + 2*bit1 + 32 - nd)
         *                  = Q(29 + 31 - nd), drop low 32 in AAC_DEC_MultiplyShiftR32
         *                  = Q(29 + 31 - 32 - nd) = Q(28 - nd)
         */
        nd = AAC_DEC_CLZ(d) - 1;
        d <<= nd;
        dInv = AAC_InvRNormalized(d);
        /* 1 GB in pXX */
        tre = AAC_DEC_MultiplyShiftR32(c01_re, c12re) - AAC_DEC_MultiplyShiftR32(c01_im, c12_im) - AAC_DEC_MultiplyShiftR32(c02_re, c11_re);
        tre = AAC_DEC_MultiplyShiftR32(tre, dInv);
        tim = AAC_DEC_MultiplyShiftR32(c01_re, c12_im) + AAC_DEC_MultiplyShiftR32(c01_im, c12re) - AAC_DEC_MultiplyShiftR32(c02_im, c11_re);
        tim = AAC_DEC_MultiplyShiftR32(tim, dInv);
        /* if d is extremely small, just set coefs to 0 (would have poor precision anyway) */
        if (nd > 28 || (AAC_SBR_FASTABS(tre) >> (28 - nd)) >= 4 || (AAC_SBR_FASTABS(tim) >> (28 - nd)) >= 4) {
            sign = 1;
        } else {
            a0_re_ptr[2]/**a1_re_ptr*/ = tre << (AAC_LP_FBITS_COEF_BITS - 28 + nd);	/* i.e. convert Q(28 - nd) to Q(29) */
            a0_re_ptr[3]/**a1_im_ptr*/ = tim << (AAC_LP_FBITS_COEF_BITS - 28 + nd);
        }
    }
    if (c11_re) {
        /* input =   Q31  c11_re = Q(-bit1 + nd) = Q31 * 2^(31 + bit1 - nd)
         * inverse = Q29  dInv  = Q29 * 2^(-31 - bit1 + nd) = Q(29 + 31 + bit1 - nd)
         *
         * numerator is Q(-bit1 - 3)
         * so num * inverse = Q(-bit1 - 3) * Q(29 + 31 + bit1 - nd)
         *                  = Q(29 + 31 - 3 - nd), drop low 32 in AAC_DEC_MultiplyShiftR32
         *                  = Q(29 + 31 - 3 - 32 - nd) = Q(25 - nd)
         */
        nd = AAC_DEC_CLZ(c11_re) - 1;	/* assume positive */
        c11_re <<= nd;
        dInv = AAC_InvRNormalized(c11_re);
        /* a1_re_ptr, a1_im_ptr = Q29, so scaled by (bit1 + 3) */
        tre = (c01_re >> 3) + AAC_DEC_MultiplyShiftR32(c12re, a0_re_ptr[2]) + AAC_DEC_MultiplyShiftR32(c12_im, a0_re_ptr[3]);
        tre = -AAC_DEC_MultiplyShiftR32(tre, dInv);
        tim = (c01_im >> 3) - AAC_DEC_MultiplyShiftR32(c12_im, a0_re_ptr[2]) + AAC_DEC_MultiplyShiftR32(c12re, a0_re_ptr[3]);
        tim = -AAC_DEC_MultiplyShiftR32(tim, dInv);
        if (nd > 25 || (AAC_SBR_FASTABS(tre) >> (25 - nd)) >= 4 || (AAC_SBR_FASTABS(tim) >> (25 - nd)) >= 4) {
            sign = 1;
        } else 
        {
            a0_re_ptr[0]/**a0_re_ptr*/ = tre << (AAC_LP_FBITS_COEF_BITS - 25 + nd);	/* i.e. convert Q(25 - nd) to Q(29) */
            a0_re_ptr[1]/**a0_im_ptr*/ = tim << (AAC_LP_FBITS_COEF_BITS - 25 + nd);
        } 
    } 
    /* if magnitude of a0 or a1 >= 4 then a0 = a1 = 0 
     *  a0_re_ptr < 4, a0_im_ptr < 4, a1_re_ptr < 4, a1_im_ptr < 4	 
     */
    if (sign || AAC_DEC_MultiplyShiftR32(a0_re_ptr[0], a0_re_ptr[0]) + AAC_DEC_MultiplyShiftR32(a0_re_ptr[1], a0_re_ptr[1]) >= AAC_LP_MAG16 || AAC_DEC_MultiplyShiftR32(a0_re_ptr[2], a0_re_ptr[2]) + AAC_DEC_MultiplyShiftR32(a0_re_ptr[3], a0_re_ptr[3]) >= AAC_LP_MAG16)
    {
        a0_re_ptr[0] = 0;
        a0_re_ptr[1] = 0;
        a0_re_ptr[2] = 0;
        a0_re_ptr[3] = 0;
    }
}

static __inline int32 AAC_SBR_FASTABS(int32 x)
{
	int32 sign;
	sign = x >> (sizeof(int32) * 8 - 1);
	x ^= sign;
	x -= sign;
	return x;
}
int32 AAC_InvRNormalized(int32 r)
{
	int32 i, xn, t;
	/* r =   [0.5, 1.0) 
	* 1/r = (1.0, 2.0] 
	*   so use 1.5 as initial guess 
	*/
	xn = Q28_15;
	/* xn = xn*(2.0 - r*xn) */
	for (i = NUM_ITER_IRN; i != 0; i--) {
		t = AAC_DEC_MultiplyShiftR32(r, xn);			/* Q31*Q29 = Q28 */
		t = Q28_2 - t;					/* Q28 */
		xn = AAC_DEC_MultiplyShiftR32(xn, t) << 4;	/* Q29*Q28 << 4 = Q29 */
	}
	return xn;
}
/**************************************************************************************
 * function name:    AAC_SBR_GenCovCalc1
 *
 * function description: calculate covariance matrix parametre: p02
 *
 * Inputs:      buffer of low-freq samples, starting at time index = 0, 
 *                freq index = patch subband
 *
 * Outputs:     complex covariance element c02_re, c02_im
 *              format = integer (Q0) * 2^N, with scalefactor N >= 0
 *
 * Return:      scalefactor N
 **************************************************************************************/
static int32 AAC_SBR_GenCovCalc1(int32 *XBuf_ptr, 
                                                                    int32 *p02reN_ptr, 
                                                                    int32 *p02imN_ptr)
{
	int32 tmp_var[4];
	AAC_DEC_U64 c02_re, c02_im;
	int32 n, z, s, low_sft, high_sft, b_msk;

	AAC_SBRGenConvCalculate1(XBuf_ptr, tmp_var);
	c02_re.part.low32 = tmp_var[0];
	c02_re.part.high32 = tmp_var[1];
	c02_im.part.low32 = tmp_var[2];
	c02_im.part.high32 = tmp_var[3];

	b_msk  = ((c02_re.part.high32) ^ (c02_re.part.high32 >> 31)) | ((c02_im.part.high32) ^ (c02_im.part.high32 >> 31));
	if (b_msk == 0) {
		s = c02_re.part.high32 >> 31; 
		b_msk  = ((int32)(c02_re.part.low32 ^ s)) - s;/*lint !e737 */
		s = c02_im.part.high32 >> 31; 
		b_msk |=((int32) (c02_im.part.low32 ^ s)) - s;/*lint !e737 */
		z = 32 + AAC_DEC_CLZ(b_msk);
	} else {
		b_msk  = AAC_SBR_FASTABS(c02_re.part.high32) | AAC_SBR_FASTABS(c02_im.part.high32);
		z = AAC_DEC_CLZ(b_msk);
	}
	n = 64 - z;	/* number of non-zero bits in bottom of 64-bit word */
	if (n <= 30) {
		low_sft = (30 - n);
		*p02reN_ptr = c02_re.part.low32 << low_sft;	
		*p02imN_ptr = c02_im.part.low32 << low_sft;
		return -(low_sft + 2*AAC_DEC_BITS_OUT_QMFA);
	} 
	else if (n < 62) {
		low_sft = (n - 30);
		high_sft = 32 - low_sft;
		*p02reN_ptr = (c02_re.part.high32 << high_sft) | (c02_re.part.low32 >> low_sft);
		*p02imN_ptr = (c02_im.part.high32 << high_sft) | (c02_im.part.low32 >> low_sft);
		return (low_sft - 2*AAC_DEC_BITS_OUT_QMFA);
	} else 
	{
		high_sft = n - (32 + 30);
		*p02reN_ptr = c02_re.part.high32 >> high_sft;	
		*p02imN_ptr = c02_im.part.high32 >> high_sft;
		return (32 - 2*AAC_DEC_BITS_OUT_QMFA - high_sft);
	}			
}


/**************************************************************************************
 * function:    AAC_SBR_GenCovCalc
 *
 * description: calculating the covariance matrix.
 *
 * inputs:      Xbuf_ptr
 *
 * outputs:    p01re_ptr, 
 *                 p01im_ptr, 
 *                 p12re_ptr,
 *                 p12im_ptr,
 *                 p11re_ptr,
 *                 p22re_ptr
 *
 * return:      scale-factor N
 **************************************************************************************/
static int32 AAC_SBR_GenCovCalc(int32 *Xbuf_ptr,
                                                                  int32 *p01reN_ptr, 
                                                                  int32 *p01imN_ptr, 
                                                                  int32 *p12reN_ptr, 
                                                                  int32 *p12imN_ptr, 
                                                                  int32 *p11reN_ptr, 
                                                                  int32 *p22reN_ptr)
{
	int32 med_var[12];
	int32 k, z, s;
	int32 low_sft, high_sft, b_msk;
	AAC_DEC_U64 c01re, c01im, c12re, c12im, c11re, c22re;
	AAC_SBRGenConvCalculate(Xbuf_ptr, 
		                                 med_var);
	c01re.part.low32 = med_var[0];	
	c01re.part.high32 = med_var[1];
	c01im.part.low32 = med_var[2];	
	c01im.part.high32 = med_var[3];
	c11re.part.low32 = med_var[4];	
	c11re.part.high32 = med_var[5];
	c12re.part.low32 = med_var[6];	
	c12re.part.high32 = med_var[7];
	c12im.part.low32 = med_var[8];	
	c12im.part.high32 = med_var[9];
	c22re.part.low32 = med_var[10];	
	c22re.part.high32 = med_var[11];

	b_msk  = ((c01re.part.high32) ^ (c01re.part.high32 >> 31)) | ((c01im.part.high32) ^ (c01im.part.high32 >> 31));
	b_msk |= ((c12re.part.high32) ^ (c12re.part.high32 >> 31)) | ((c12im.part.high32) ^ (c12im.part.high32 >> 31));
	b_msk |= ((c11re.part.high32) ^ (c11re.part.high32 >> 31)) | ((c22re.part.high32) ^ (c22re.part.high32 >> 31));
    if (b_msk == 0) 
    {
		s = c01re.part.high32 >> 31; 
		b_msk  = ((int32)(c01re.part.low32 ^ s)) - s; /*lint !e737 */
		s = c01im.part.high32 >> 31; 
		b_msk |= ((int32)(c01im.part.low32 ^ s)) - s;/*lint !e737 */
		s = c12re.part.high32 >> 31; 
		b_msk |= ((int32)(c12re.part.low32 ^ s)) - s;/*lint !e737 */
		s = c12im.part.high32 >> 31; 
		b_msk |= ((int32)(c12im.part.low32 ^ s)) - s;/*lint !e737 */
		s = c11re.part.high32 >> 31; 
		b_msk |= ((int32)(c11re.part.low32 ^ s)) - s;/*lint !e737 */
		s = c22re.part.high32 >> 31; 
		b_msk |= ((int32)(c22re.part.low32 ^ s)) - s;/*lint !e737 */
		z = 32 + AAC_DEC_CLZ(b_msk);
	} else {
		b_msk  = AAC_SBR_FASTABS(c01re.part.high32) | AAC_SBR_FASTABS(c01im.part.high32);
		b_msk |= AAC_SBR_FASTABS(c12re.part.high32) | AAC_SBR_FASTABS(c12im.part.high32);
		b_msk |= AAC_SBR_FASTABS(c11re.part.high32) | AAC_SBR_FASTABS(c22re.part.high32);
		z = AAC_DEC_CLZ(b_msk);
	}
	k = 64 - z;	/* count of non-zero bits in bottom of 64-bit data */
    if (k <= 30) 
    {
		low_sft = (30 - k);
		*p01reN_ptr = c01re.part.low32 << low_sft;	
		*p01imN_ptr = c01im.part.low32 << low_sft;
		*p12reN_ptr = c12re.part.low32 << low_sft;	
		*p12imN_ptr = c12im.part.low32 << low_sft;
		*p11reN_ptr = c11re.part.low32 << low_sft;	
		*p22reN_ptr = c22re.part.low32 << low_sft;
		return -(low_sft + 2*AAC_DEC_BITS_OUT_QMFA);
    } else if (k < 62) 
    {
		low_sft = (k - 30);
		high_sft = 32 - low_sft;
		*p01reN_ptr = (c01re.part.high32 << high_sft) | (c01re.part.low32 >> low_sft);
		*p01imN_ptr = (c01im.part.high32 << high_sft) | (c01im.part.low32 >> low_sft);
		*p12reN_ptr = (c12re.part.high32 << high_sft) | (c12re.part.low32 >> low_sft);
		*p12imN_ptr = (c12im.part.high32 << high_sft) | (c12im.part.low32 >> low_sft);
		*p11reN_ptr = (c11re.part.high32 << high_sft) | (c11re.part.low32 >> low_sft);
		*p22reN_ptr = (c22re.part.high32 << high_sft) | (c22re.part.low32 >> low_sft);
		return (low_sft - 2*AAC_DEC_BITS_OUT_QMFA);
    } else 
    {
		high_sft = k - (62);
		*p01reN_ptr = c01re.part.high32 >> high_sft;	
		*p01imN_ptr = c01im.part.high32 >> high_sft;
		*p12reN_ptr = c12re.part.high32 >> high_sft;	
		*p12imN_ptr = c12im.part.high32 >> high_sft;
		*p11reN_ptr = c11re.part.high32 >> high_sft;	
		*p22reN_ptr = c22re.part.high32 >> high_sft;
		return (32 - 2*AAC_DEC_BITS_OUT_QMFA - high_sft);
	}
}
static int32 AAC_MapNewBw(uint8 invf_mode, 
					    uint8 invf_mode_prev)
{
    switch (invf_mode)
    {
    case 1: /* LOW */
        if (invf_mode_prev == 0) /* NONE */
            return 1288490189 ;   //S3.31 COEF_CONST(0.6);
        else
            return 1610612736 ;   //S4.28 COEF_CONST(0.75);
    case 2: /* MID */
        return 1932735283 ;   //S4.28 COEF_CONST(0.9);
    case 3: /* HIGH */
        return 2104533975 ;   //S4.28 COEF_CONST(0.98);
    default: /* NONE */
        if (invf_mode_prev == 1) /* LOW */
            return 1288490189 ;   //S4.28 COEF_CONST(0.6);
        else
            return 0 ;
    }
}

/* calc the parameter bwArray */
static void AAC_CalcChirpFactors(AAC_SBR_INFO_T *sbr_ptr, uint8 ch)
{
    uint8 i;

	uint8 N_Q = sbr_ptr->N_Q;
    for (i = 0; i < N_Q; i++)
    {
		int32 tbwArray;  // S0.12
        tbwArray = AAC_MapNewBw(sbr_ptr->bs_invf_mode[ch][i], sbr_ptr->bs_invf_mode_prev[ch][i]);

        if (tbwArray < sbr_ptr->bwArray[ch][i])
		{
            tbwArray = ((tbwArray >> 2) * 3) + (sbr_ptr->bwArray[ch][i] >> 2);				
		}
        else
		{		
			tbwArray = AAC_DEC_MultiplyShiftR32(tbwArray, 0x74000000) + AAC_DEC_MultiplyShiftR32(0x0c000000, sbr_ptr->bwArray[ch][i] );	/* new is larger: 0.90625*new + 0.09375*old */
			tbwArray <<= 1;
		}
		if (tbwArray < 33554432)
            tbwArray = 0;

        if (tbwArray >= 2139095040)
            tbwArray = 2139095040;
		
		sbr_ptr->bwArray[ch][i]           = tbwArray;
        sbr_ptr->bs_invf_mode_prev[ch][i] = sbr_ptr->bs_invf_mode[ch][i];
    }
}

/* calc the parameter patchNoSubbands*/ 
static void AAC_PatchConstruction(AAC_SBR_INFO_T *sbr_ptr)
{
    uint8 i, k;
    uint8 odd, sb;
    uint8 k0     = sbr_ptr->k0;
    uint8 kx     = sbr_ptr->kx;    
    /* (uint8)(2.048e6/sbr_ptr->sample_rate + 0.5); */
    uint8 goalSb = g_goalSbTab[AAC_GetSrIndex(sbr_ptr->sample_rate)];
    sbr_ptr->noPatches = 0;
    if (goalSb < (sbr_ptr->kx + sbr_ptr->M))
    {
        for (i = 0, k = 0; sbr_ptr->f_master[i] < goalSb; i++)
            k = (uint8) (i+1);
    } else 
	{
        k = sbr_ptr->N_master;
    }

    do
    {
        uint8 j = (uint8)(k + 1);
        do
        {
            j--;

            sb = sbr_ptr->f_master[j];
            odd = (uint8) ((sb - 2 + sbr_ptr->k0) % 2);
        } while (sb > (sbr_ptr->k0 - 1 + k0 - odd));

        sbr_ptr->patchNoSubbands[sbr_ptr->noPatches] = (uint8) (AAC_DEC_MAX(sb - kx, 0));
        sbr_ptr->patchStartSubband[sbr_ptr->noPatches] = (uint8)( sbr_ptr->k0 - odd -
        sbr_ptr->patchNoSubbands[sbr_ptr->noPatches]);
		
        if (sbr_ptr->patchNoSubbands[sbr_ptr->noPatches] > 0)
        {
            kx = sb;
            k0 = sb;
            sbr_ptr->noPatches++;
        } else {
            k0 = sbr_ptr->kx;
        }
		
        if (sbr_ptr->f_master[k] - sb < 3)
            k = sbr_ptr->N_master;
    } while (sb != (sbr_ptr->kx + sbr_ptr->M));
	
    if ((sbr_ptr->patchNoSubbands[sbr_ptr->noPatches-1] < 3) && (sbr_ptr->noPatches > 1))
    {
        sbr_ptr->noPatches--;
    }
	
    sbr_ptr->noPatches = (uint8) (AAC_DEC_MIN(sbr_ptr->noPatches, 5));
}



