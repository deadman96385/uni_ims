/******************************************************************************
 ** File Name:    h264dec_isqt.c                                              *
 ** Author:       Xiaowei.Luo                                                 *
 ** DATE:         03/29/2010                                                  *
 ** Copyright:    2010 Spreatrum, Incoporated. All Rights Reserved.           *
 ** Description:                                                              *
 *****************************************************************************/
/******************************************************************************
 **                   Edit    History                                         *
 **---------------------------------------------------------------------------* 
 ** DATE          NAME            DESCRIPTION                                 * 
 ** 03/29/2010    Xiaowei.Luo     Create.                                     *
 *****************************************************************************/
#include "sc8825_video_header.h"

/*isqt*/
#define DQ_BITS         6
#define DQ_ROUND        (1<<(DQ_BITS-1))

/*description:
	1. inverse quantization
	2: put to MB's DC position
*/
#ifdef WIN32	
void itrans_lumaDC (int16 * DCCoeff, int16 * pCoeffIq, int32 qp)
{
	int i;
	int16 blk_tmp [16];
	int32 m0, m1, m2, m3;
	int32 n0, n1, n2, n3;
	int16 * p_blk, *p_blkIn;
	const uint8 *blkIndex = g_blKIndex;
	int32 quantizer = g_image_ptr->dequant4_buffer[0][qp][0];	

	p_blk = blk_tmp;
	p_blkIn = DCCoeff;

	/*horizontal inverse transform*/
	for (i = 4; i > 0; i--)
	{
		m0 = *p_blkIn++;
		m1 = *p_blkIn++;
		m2 = *p_blkIn++;
		m3 = *p_blkIn++;

		n0 = m0 + m2;
		n1 = m0 - m2;
		n2 = m1 - m3;
		n3 = m1 + m3;

		*p_blk++ = n0 + n3;
		*p_blk++ = n1 + n2;
		*p_blk++ = n1 - n2;
		*p_blk++ = n0 - n3;		
	}

	/*vertical inverse transform*/
	p_blkIn = blk_tmp + 3;
	for (i = 3; i >= 0; i--)
	{
		p_blk = DCCoeff + i;

		m0 = p_blkIn[0];
		m1 = p_blkIn[4];
		m2 = p_blkIn[8];
		m3 = p_blkIn[12];
		
		n0 = m0 + m2;
		n1 = m0 - m2;
		n2 = m1 - m3;
		n3 = m1 + m3;
		
		pCoeffIq[blkIndex [i+0]*16] = ((n0 + n3) * quantizer + 128) >> 8;	
		pCoeffIq[blkIndex [i+4]*16] = ((n1 + n2) * quantizer + 128) >> 8;	
		pCoeffIq[blkIndex [i+8]*16] = ((n1 - n2) * quantizer + 128) >> 8;	
		pCoeffIq[blkIndex [i+12]*16] = ((n0 - n3) * quantizer + 128) >> 8;	
		
		p_blkIn--;
	}
}
#endif

/*
description: inverse transform the 4x4 block
			 add the coefficient and the prediction
			 saturate the addition and put currMB->pred_Y;
	
*/
#ifdef WIN32
void itrans_4x4 (int16 *coff, uint8 *pred, int32 width_p, uint8 *rec, int32 width_r)
{	
	int32 i;
	int32 m0, m1, m2, m3;
	int32 n0, n1, n2, n3;
	int16 *src = coff;
	uint8 *ref, *dst;

	/*horizontal inverse transform*/
	for (i = 4; i > 0; i--)
	{
		m0 = src[0]; 
		m1 = src[1];
		m2 = src[2]; 
		m3 = src[3]; 

		n0 = m0 + m2;
		n1 = m0 - m2;
		n2 = (m1 >> 1) - m3;
		n3 = m1 + (m3 >> 1);

		*src++ = n0 + n3;
		*src++ = n1 + n2;
		*src++ = n1 - n2;
		*src++ = n0 - n3;
	}

	/*vertical inverse transform*/
	src = coff;
	ref = pred;
	dst = rec;

	for (i = 4; i > 0; i--)
	{
		m0 = src [0 * 4];
		m1 = src [1 * 4];
		m2 = src [2 * 4];
		m3 = src [3 * 4];	
		
		n0 = m0 + m2;
		n1 = m0 - m2;
		n2 = (m1 >> 1) - m3;
		n3 = m1 + (m3 >> 1);

		/*get the prediction value*/
		m0 = ref [0 * width_p];
		m1 = ref [1 * width_p];
		m2 = ref [2 * width_p];
		m3 = ref [3 * width_p];

		m0 = IClip(0, 255, ((n0 + n3 + (m0 << 6 ) + 32) >> 6));
		m1 = IClip(0, 255, ((n1 + n2 + (m1 << DQ_BITS ) + DQ_ROUND) >> DQ_BITS));
		m2 = IClip(0, 255, ((n1 - n2 + (m2 << DQ_BITS ) + DQ_ROUND) >> DQ_BITS));
		m3 = IClip(0, 255, ((n0 - n3 + (m3 << DQ_BITS ) + DQ_ROUND) >> DQ_BITS));

		dst [0 * width_r] = m0; 
		dst [1 * width_r] = m1;
		dst [2 * width_r] = m2;
		dst [3 * width_r] = m3;	
		
#if 0 //for debug
		ref [0 * width_p] = m0;
		ref [1 * width_p] = m1;
		ref [2 * width_p] = m2;
		ref [3 * width_p] = m3;
#endif

		/*point to next column*/
		src += 1;
		ref += 1;
		dst += 1;
	}
}
#endif

#ifdef WIN32
void itrans_8x8 (int16 *coff, uint8 *pred, int32 width_p, uint8 *rec, int32 width_r)
{
	int i;
	int32 d0, d1, d2, d3, d4, d5, d6, d7;
	int32 e0, e1, e2, e3, e4, e5, e6, e7;
	int32 f0, f1, f2, f3, f4, f5, f6, f7;
	int16 * src = coff;
	uint8 *ref, *dst;
	int16 block_tmp[64], *tmp;

	tmp = block_tmp;
	for(i = 8; i > 0; i--)
	{
		d0 = src[0*8];
		d1 = src[1*8];
		d2 = src[2*8];
		d3 = src[3*8];
		d4 = src[4*8];
		d5 = src[5*8];
		d6 = src[6*8];
		d7 = src[7*8];
		
		e0 = d0 + d4;
		e2 = d0 - d4;
		e1 = -d3 + d5 - d7 -(d7>>1);
		e3 = d1 + d7 -d3 - (d3>>1);
		e4 = (d2>>1) - d6;
		e6 = d2 + (d6>>1);
		e5 = -d1 + d7 + d5 + (d5>>1);
		e7 = d3 + d5 + d1 + (d1>>1);
	
		f0 = e0 + e6;
		f6 = e0 - e6;
		f2 = e2 + e4;
		f4 = e2 - e4;
		f1 = e1 + (e7>>2);
		f7 = e7 -(e1>>2);
		f3 = e3 + (e5>>2);
		f5 = (e3>>2) - e5;

		tmp[0] = f0 + f7;
		tmp[7] = f0 - f7;
		tmp[1] = f2 + f5;
		tmp[6] = f2 - f5;
		tmp[2] = f4 + f3;
		tmp[5] = f4 - f3;
		tmp[3] = f6 + f1;
		tmp[4] = f6 - f1;

		src++;
		tmp += 8;
	}

	src = block_tmp;
	ref = pred;
	dst = rec;
	for(i = 8; i > 0; i--)
	{
		
		d0 = src[0 * 8];
		d1 = src[1 * 8];
		d2 = src[2 * 8];
		d3 = src[3 * 8];
		d4 = src[4 * 8];
		d5 = src[5 * 8];
		d6 = src[6 * 8];
		d7 = src[7 * 8];
		
		e0 = d0 + d4;
		e2 = d0 - d4;
		e1 = -d3 + d5 - d7 -(d7>>1);
		e3 = d1 + d7 -d3 - (d3>>1);
		e4 = (d2>>1) - d6;
		e6 = d2 + (d6>>1);
		e5 = -d1 + d7 + d5 + (d5>>1);
		e7 = d3 + d5 + d1 + (d1>>1);

		f0 = e0 + e6;
		f6 = e0 - e6;
		f2 = e2 + e4;
		f4 = e2 - e4;
		f1 = e1 + (e7>>2);
		f7 = e7 -(e1>>2);
		f3 = e3 + (e5>>2);
		f5 = (e3>>2) - e5;

		/*get the prediction value*/
		d0 = ref [0 * width_p];
		d1 = ref [1 * width_p];
		d2 = ref [2 * width_p];
		d3 = ref [3 * width_p];
		d4 = ref [4 * width_p];
		d5 = ref [5 * width_p];
		d6 = ref [6 * width_p];
		d7 = ref [7 * width_p];

		d0 = IClip(0, 255, ((f0 + f7 + (d0 << DQ_BITS) + DQ_ROUND) >> DQ_BITS));
		d1 = IClip(0, 255, ((f2 + f5 + (d1 << DQ_BITS) + DQ_ROUND) >> DQ_BITS));
		d2 = IClip(0, 255, ((f4 + f3 + (d2 << DQ_BITS) + DQ_ROUND) >> DQ_BITS));
		d3 = IClip(0, 255, ((f6 + f1 + (d3 << DQ_BITS) + DQ_ROUND) >> DQ_BITS));
		d4 = IClip(0, 255, ((f6 - f1 + (d4 << DQ_BITS) + DQ_ROUND) >> DQ_BITS));
		d5 = IClip(0, 255, ((f4 - f3 + (d5 << DQ_BITS) + DQ_ROUND) >> DQ_BITS));
		d6 = IClip(0, 255, ((f2 - f5 + (d6 << DQ_BITS) + DQ_ROUND) >> DQ_BITS));
		d7 = IClip(0, 255, ((f0 - f7 + (d7 << DQ_BITS) + DQ_ROUND) >> DQ_BITS));

		dst [0 * width_r] = d0;
		dst [1 * width_r] = d1;
		dst [2 * width_r] = d2;
		dst [3 * width_r] = d3;
		dst [4 * width_r] = d4;
		dst [5 * width_r] = d5;
		dst [6 * width_r] = d6;
		dst [7 * width_r] = d7;

#if 0 //for debug
		ref [0 * width_p] = d0;
		ref [1 * width_p] = d1;
		ref [2 * width_p] = d2;
		ref [3 * width_p] = d3;
		ref [4 * width_p] = d4;
		ref [5 * width_p] = d5;
		ref [6 * width_p] = d6;
		ref [7 * width_p] = d7;
#endif
		/*point to next column*/
		src += 1;
		ref += 1;
		dst += 1;
	}
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

