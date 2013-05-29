/******************************************************************************
 ** File Name:    dct_asic.c												  *
 ** Author:       Xiaowei Luo                                                 *
 ** DATE:         12/14/2006                                                  *
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
#include "sc8810_video_header.h"
/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    extern   "C" 
    {
#endif
#define MULTI_VAR_BIT  15
#define MEDIAN_VAR_BIT 19
#define DCT_BOTTOM     -(1 << MEDIAN_VAR_BIT)
#define DCT_TOP        (1 << MEDIAN_VAR_BIT) - 1
#define SKIP           2147483648-1
#define DCT_BOTTOM20   -(1 << (MEDIAN_VAR_BIT+1))
#define DCT_TOP20      (1 << (MEDIAN_VAR_BIT+1)) - 1
#define DCT_CLIP(val, min, max) ((val>max) ? (val = max):((val < min) ? (val = min) : (val)))

/**----------------------------------------------------------------------------*
**                           Function Prototype                               **
**----------------------------------------------------------------------------*/
/**---------------------------------------------------------------------------*
 **                         STATIC DEFINITION                                  *
 **---------------------------------------------------------------------------*/
int32 g_med_buf[64];
/************************************************************************/
/* dct_idct table                                                       */
/************************************************************************/
const uint16 g_dct_matrix[] = /* the fixed-point is Q1.13 */
{
    	8192,	8192,	9786,	9786,	9846,	8133,	8133,	9846,
		8192,	8192,	9786,	9786,	9846,	8133,	8133,	9846,
		9786,	9786,	11689,	11689,	11761,	9715,	9715,	11761,
		9786,	9786,	11689,	11689,	11761,	9715,	9715,	11761,
		9846,	9846,	11761,	11761,	11833,	9775,	9775,	11833,
		8133,	8133,	9715,	9715,	9775,	8075,	8075,	9775,
		8133,	8133,	9715,	9715,	9775,	8075,	8075,	9775,
		9846,	9846,	11761,	11761,	11833,	9775,	9775,	11833,
/*
	8192,	8192,	9786,	9786,	9846,	8133,	8133,	9846,
	8192,	8192,	9786,	9786,	9846,	8133,	8133,	9846,
	9786,	9786,	11689,	11689,	11761,	9715,	9715,	11761,
	9786,	9786,	11689,	11689,	11761,	9715,	9715,	11761,
	9846,	9846,	11761,	11761,	11833,	9775,	9775,	11833,
	8133,	8133,	9715,	9715,	9775,	8075,	8075,	9775,
	8133,	8133,	9715,	9715,	9775,	8075,	8075,	9775,
	9846,	9846,	11761,	11761,	11833,	9775,	9775,	11833,		*/
};

const int8 dct_fun_H0[] = {0, 1, 1, 0,  0, 0, 1,  1};
const int8 dct_fun_H1[] = {1, 1, 0, 0,  0, 0, 0, -1};
const int8 dct_fun_H2[] = {1, 0, 0, 0,  1, 1, 0,  0};
const int8 dct_fun_H3[] = {1, 1, 0, 0,  0, 1, 0,  0};
const int8 dct_fun_H4[] = {0, 1, 0, 0,  0, 1, 1,  0};
const int8 dct_fun_H5[] = {1, 0, 0, 0,  0, 0, 1,  1};
const int8 dct_fun_H6[] = {0, 1, 0, 0, -1, 0, 1,  0};
const int8 dct_fun_H7[] = {0, 0, 1, 0,  0, 1, 1,  0};

/************************************************************************/
/* idct_idct table                                                       */
/************************************************************************/
const uint16 IDCT_matrix[] = /* the fixed-point is Q1.13 */
{
	8192,	8192,	4893,	4893,	4923,	8133,	8133,	4923,
    8192,	8192,	4893,	4893,	4923,	8133,	8133,	4923,
    4893,	4893,	2922,	2922,	2940,	4858,	4858,	2940,
	4893,	4893,	2922,	2922,	2940,	4858,	4858,	2940,
	4923,	4923,	2940,	2940,	2958,	4887,	4887,	2958,
	8133,	8133,	4858,	4858,	4887,	8075,	8075,	4887,
	8133,	8133,	4858,	4858,	4887,	8075,	8075,	4887,
	4923,	4923,	2940,	2940,	2958,	4887,	4887,	2958,
};

const uint8 posMap [8] = 
{
	0, 4, 2, 6, 1, 5, 3, 7 
};
/**----------------------------------------------------------------------------*
**                           Function Prototype                               **
**----------------------------------------------------------------------------*/
/*****************************************************************************
 **	Name : 			AsicMultiAdd
 ** Description:	AsicMultiAdd function.
 ** Author:			Xiaowei Luo
 **	Note:
 *****************************************************************************/
LOCAL int32 AsicMultiAdd( int32  *t0, 
					  int32  *t1,
					  const int8   *c0,
					  const int8   *c1,
					  uint8   sign // 0: t0 * c0 - t1 * c1;
									 // 1: t0 * c0 + t1 * c1;
					  )
{
	int32 rel = 0;
	uint8 i;
	int32 sum0[8] = {0}, sum1[8] = {0};
	int32 suma = 0, sumb = 0;
	int32 d0 = *t0;
	int32 d1 = *t1;

	for (i = 0; i < 8; i++)
	{
		int32 tmp;
		tmp     = (d0) * (1<<(7 - i)) * c0[i] - ((1 - c0[i]) / 2);
		if (tmp)
			tmp     = tmp & 0xFFFFFFF0;
		sum0[i] = tmp;

		if (SKIP != d1)
		{
			if (sign)
			{
				tmp     = (d1) * (1<<(7 - i)) * c1[i] - ((1 - c1[i]) / 2);
				if (tmp)
					tmp     = tmp & 0xFFFFFFF0;
				sum1[i] = tmp;
			}else 
			{
				tmp     = -(d1) * (1<<(7 - i)) * c1[i] - ((1 + c1[i]) / 2);
				if (tmp)
					tmp     = tmp & 0xFFFFFFF0;
				sum1[i] = tmp;
			}	
		}			
	}

	for (i = 0; i < 8; i++)
	{
		suma += sum0[i];
		sumb += sum1[i];
	}
	if (sign)
	{
		rel = suma + sumb; //S8.19
	}else
	{
		rel = suma + sumb + 16; //S8.19
	}

	return rel;
}

/*****************************************************************************
 **	Name : 			idct_1D_8
 ** Description:	idct_1D_8 function.
 ** Author:			Xiaowei Luo
 **	Note:			do 1D 8 point DCT   
					the input data fixed-point is: S12.8 or S11.9
					and the output data fixed-point is: S8.12 or S9.11
 *****************************************************************************/
void idct_1D_8(int32 *point_ptr, uint32 in_data_bit)
{
	uint8 i;
	int32 d0, d1;
	int32 r0, r1;
	// step 1
	d0 = point_ptr[4];
	d1 = point_ptr[7];
	r0 = AsicMultiAdd(&d0, &d1, dct_fun_H7, dct_fun_H1, 0)*2;
	r1 = AsicMultiAdd(&d1, &d0, dct_fun_H7, dct_fun_H1, 1)*2;
	r0 = (r0 + 128) >> 8;
	r1 = (r1 + 128) >> 8;
	
	DCT_CLIP(r0, DCT_BOTTOM, (DCT_TOP+1));
	DCT_CLIP(r1, DCT_BOTTOM, (DCT_TOP+1));
	point_ptr[4] = r0;
	point_ptr[7] = r1;
	
	d0 = point_ptr[5];
	d1 = point_ptr[6];
	r0 = AsicMultiAdd(&d0, &d1, dct_fun_H3, dct_fun_H5, 0);
	r1 = AsicMultiAdd(&d1, &d0, dct_fun_H3, dct_fun_H5, 1);
	r0 = (r0 + 128) >> 8;
	r1 = (r1 + 128) >> 8;
	DCT_CLIP(r0, DCT_BOTTOM, (DCT_TOP+1));
	DCT_CLIP(r1, DCT_BOTTOM, (DCT_TOP+1));
	point_ptr[5] = r0;
	point_ptr[6] = r1;

	// step 2
	d0 = point_ptr[0];
	d1 = point_ptr[1];
	r0 = (d0 + d1);
	r1 = (d0 - d1);
	DCT_CLIP(r0, DCT_BOTTOM, (DCT_TOP+1));
	DCT_CLIP(r1, DCT_BOTTOM, (DCT_TOP+1));
	point_ptr[0] = r0;
	point_ptr[1] = r1;

	d0 = point_ptr[2];
	d1 = point_ptr[3];
	r0 = AsicMultiAdd(&d0, &d1, dct_fun_H6, dct_fun_H2, 0)*4;
	r1 = AsicMultiAdd(&d1, &d0, dct_fun_H6, dct_fun_H2, 1)*4;
	r0 = (r0 + 128) >> 8;
	r1 = (r1 + 128) >> 8;
	DCT_CLIP(r0, DCT_BOTTOM, (DCT_TOP+1));
	DCT_CLIP(r1, DCT_BOTTOM, (DCT_TOP+1));
	point_ptr[2] = r0;
	point_ptr[3] = r1;

	d0 = point_ptr[4];
	d1 = point_ptr[5];
	r0 = (d0 + d1);
	r1 = (d0 - d1);
	DCT_CLIP(r0, DCT_BOTTOM, DCT_TOP);
	DCT_CLIP(r1, DCT_BOTTOM, DCT_TOP);
	point_ptr[4] = r0;
	point_ptr[5] = r1;

	d0 = point_ptr[6];
	d1 = point_ptr[7];
	r0 = (-d0 + d1);
	r1 = ( d0 + d1);
	DCT_CLIP(r0, DCT_BOTTOM, DCT_TOP);
	DCT_CLIP(r1, DCT_BOTTOM, DCT_TOP);
	point_ptr[6] = r0;
	point_ptr[7] = r1;

	// step 3
	for (i = 0; i < 2; i++)
	{
		d0 = point_ptr[i];
		d1 = point_ptr[3-i];
		r0 = (d0 + d1);
		r1 = (d0 - d1);
		DCT_CLIP(r0, DCT_BOTTOM, DCT_TOP);
		DCT_CLIP(r1, DCT_BOTTOM, DCT_TOP);
		point_ptr[i]   = r0;
		point_ptr[3-i] = r1;
	}

	d0 = point_ptr[4];
	d1 = SKIP;
	r0 = AsicMultiAdd(&d0, &d1, dct_fun_H0, dct_fun_H0, 1) * 4;
	d0 = point_ptr[7];
	r1 = AsicMultiAdd(&d0, &d1, dct_fun_H0, dct_fun_H0, 0) * 4;
	r0 = (r0 + 128) >> 8;
	r1 = (r1 + 128) >> 8;
	DCT_CLIP(r0, DCT_BOTTOM, (DCT_TOP+1));
	DCT_CLIP(r1, DCT_BOTTOM, (DCT_TOP+1));
	point_ptr[4] = r0;
	point_ptr[7] = r1;
	
	d0 = point_ptr[5];
	d1 = point_ptr[6];
	r0 = AsicMultiAdd(&d1, &d0, dct_fun_H4, dct_fun_H4, 0) * 4;
	r1 = AsicMultiAdd(&d1, &d0, dct_fun_H4, dct_fun_H4, 1) * 4;
	r0 = (r0 + 128) >> 8;
	r1 = (r1 + 128) >> 8;	
	DCT_CLIP(r0, DCT_BOTTOM, (DCT_TOP+1));
	DCT_CLIP(r1, DCT_BOTTOM, (DCT_TOP+1));
	point_ptr[5] = r0;
	point_ptr[6] = r1;

    // step 4
	for (i = 0; i < 4; i++)
	{		
		d0 = point_ptr[i];
		d1 = point_ptr[7-i];
		r0 = (d0 + d1);
		r1 = (d0 - d1);
		DCT_CLIP(r0, DCT_BOTTOM, DCT_TOP);
		DCT_CLIP(r1, DCT_BOTTOM, DCT_TOP);
		point_ptr[i]   = r0;  
		point_ptr[7-i] = r1;
	}
}

/*****************************************************************************
 **	Name : 			dct_1D_8
 ** Description:	dct_1D_8 function.
 ** Author:			Xiaowei Luo
 **	Note:			do 1D 8 point DCT   
					the input data fixed-point is: S8.0 or S9.0
					and the output data fixed-point is: S8.12 or S9.11
 *****************************************************************************/
void dct_1D_8(int32 *point_ptr, uint32 bit_width)
{
	uint8 i;
	int32 d0, d1;
	int32 r0, r1;
	// step 1, fixed-point S8.12  //or S9.11
	for (i = 0; i < 4; i++)
	{		
		d0 = point_ptr[i];
		d1 = point_ptr[7-i];
        if (MEDIAN_VAR_BIT == bit_width)
		{
			r0 = (d0 + d1 + 1) >> 1;
			r1 = (d0 - d1 + 1) >> 1;
		}else
		{
			r0 = (d0 + d1 + 2) >> 2;
			r1 = (d0 - d1 + 2) >> 2;
		}
		
		DCT_CLIP(r0, DCT_BOTTOM, DCT_TOP);
		DCT_CLIP(r1, DCT_BOTTOM, DCT_TOP);
		point_ptr[i]   = r0;  
		point_ptr[7-i] = r1;
	}

	// step 2, fixed-point S8.12 //or s9.11 ?s10.11
	for (i = 0; i < 2; i++)
	{
		d0 = point_ptr[i];
		d1 = point_ptr[3-i];
		r0 = (d0 + d1 + 1) >> 1;
		r1 = (d0 - d1 + 1) >> 1;
		DCT_CLIP(r0, DCT_BOTTOM, DCT_TOP);
		DCT_CLIP(r1, DCT_BOTTOM, DCT_TOP);
		point_ptr[i]   = r0;
		point_ptr[3-i] = r1;
	}
	d0 = point_ptr[4];
	d1 = SKIP;
	r0 = AsicMultiAdd(&d0, &d1, dct_fun_H0, dct_fun_H0, 0);
	d0 = point_ptr[7];
	r1 = AsicMultiAdd(&d0, &d1, dct_fun_H0, dct_fun_H0, 1);
	r0 = (r0 + 128) >> 8;
	r1 = (r1 + 128) >> 8;
	DCT_CLIP(r0, DCT_BOTTOM, (DCT_TOP+1));
	DCT_CLIP(r1, DCT_BOTTOM, (DCT_TOP+1));
	point_ptr[4] = r0;
	point_ptr[7] = r1;

	d0 = point_ptr[5];
	d1 = point_ptr[6];
	r0 = AsicMultiAdd(&d1, &d0, dct_fun_H4, dct_fun_H4, 0);
	r1 = AsicMultiAdd(&d1, &d0, dct_fun_H4, dct_fun_H4, 1);
	r0 = (r0 + 128) >> 8;
	r1 = (r1 + 128) >> 8;
	
	DCT_CLIP(r0, DCT_BOTTOM, (DCT_TOP+1));
	DCT_CLIP(r1, DCT_BOTTOM, (DCT_TOP+1));
	point_ptr[5] = r0;
	point_ptr[6] = r1;

	// step 3, fixed-point S8.12 //or s9.11 
	d0 = point_ptr[4];
	d1 = point_ptr[5];
	r0 = d0 + d1;
	r1 = d0 - d1;
	DCT_CLIP(r0, DCT_BOTTOM, DCT_TOP);
	DCT_CLIP(r1, DCT_BOTTOM, DCT_TOP);
	point_ptr[4] = r0;
	point_ptr[5] = r1;

	d0 = point_ptr[6];
	d1 = point_ptr[7];
	r0 = -d0 + d1;
	r1 =  d0 + d1;
	DCT_CLIP(r0, DCT_BOTTOM, DCT_TOP);
	DCT_CLIP(r1, DCT_BOTTOM, DCT_TOP);
	point_ptr[6] = r0;
	point_ptr[7] = r1;

	// step 4, fixed-point S8.13 //or s9.12 
	d0 = point_ptr[0];
	d1 = point_ptr[1];
	r0 = (d0 + d1);
	r1 = (d0 - d1);
	DCT_CLIP(r0, DCT_BOTTOM20, DCT_TOP20);
	DCT_CLIP(r1, DCT_BOTTOM20, DCT_TOP20);
	point_ptr[0] = r0;
	point_ptr[1] = r1;

	d0 = point_ptr[2];
	d1 = point_ptr[3];
	r0 = AsicMultiAdd(&d0, &d1, dct_fun_H6, dct_fun_H2, 1);
	r1 = AsicMultiAdd(&d1, &d0, dct_fun_H6, dct_fun_H2, 0);
	r0 = (r0) >> 7;
	r1 = (r1) >> 7;
	DCT_CLIP(r0, DCT_BOTTOM20, DCT_TOP20);
	DCT_CLIP(r1, DCT_BOTTOM20, DCT_TOP20);
	point_ptr[2] = r0;
	point_ptr[3] = r1;

	d0 = point_ptr[4];
	d1 = point_ptr[7];
	r0 = AsicMultiAdd(&d0, &d1, dct_fun_H7, dct_fun_H1, 1);
	r1 = AsicMultiAdd(&d1, &d0, dct_fun_H7, dct_fun_H1, 0);
	r0 = (r0) >> 7;
	r1 = (r1) >> 7;

	DCT_CLIP(r0, DCT_BOTTOM20, DCT_TOP20);
	DCT_CLIP(r1, DCT_BOTTOM20, DCT_TOP20);
	point_ptr[4] = r0;
	point_ptr[7] = r1;

	d0 = point_ptr[5];
	d1 = point_ptr[6];
	r0 = AsicMultiAdd(&d0, &d1, dct_fun_H3, dct_fun_H5, 1);
	r1 = AsicMultiAdd(&d1, &d0, dct_fun_H3, dct_fun_H5, 0);
	r0 = (r0) >> 7;
	r1 = (r1) >> 7;
	DCT_CLIP(r0, DCT_BOTTOM20, DCT_TOP20);
	DCT_CLIP(r1, DCT_BOTTOM20, DCT_TOP20);
	point_ptr[5] = r0;
	point_ptr[6] = r1;
}

/*****************************************************************************
 **	Name : 			tranpose_blk
 ** Description:	tranpose_blk function.
 ** Author:			Xiaowei Luo
 **	Note:
 *****************************************************************************/
LOCAL void tranpose_blk(int16 *pBlk)
{
	int32 i, j;
	int16 pBlkTmp[64];

	for (j = 0; j < 8; j++)
	{
		for (i = 0; i < 8; i++)
		{
			pBlkTmp [i * 8 + j] = pBlk [j * 8 + i];
		}
	}

	for (j = 0; j < 8; j++)
	{
		for (i = 0; i < 8; i++)
		{
			pBlk [j * 8 + i] = pBlkTmp [j * 8 +i];
		}
	}
}

/*****************************************************************************
 **	Name : 			exchange_pos
 ** Description:	exchange_pos function.
 ** Author:			Xiaowei Luo
 **	Note:
 *****************************************************************************/
LOCAL void exchange_pos(int16 *pBlk)
{
	int32 i, j;
	int16 BlkTmp [64];
	int16 * pSrcLine, * pDstLine;

	pSrcLine = pBlk;
	for (j = 0; j < 8; j++)
	{
		pDstLine = BlkTmp + posMap [j] * 8;
		for (i = 0; i < 8; i++)
		{
			pDstLine [i] = pSrcLine [posMap[i]];
		}

		pSrcLine += 8;
	}

	for (i = 0; i < 64; i++)
	{
		pBlk [i] = BlkTmp [i];
	}
}


/*****************************************************************************
 **	Name : 			mpeg4_dct
 ** Description:	mpeg4_dct function.
 ** Author:			Xiaowei Luo
 **	Note:			dct_data:  input data and the fixed-point: S8.0 or S9.0
					idct_data: output data and the fixed-point: S11.0 or S12.0
 *****************************************************************************/
void mpeg4_dct(void *dct_data,      // input data
			   int16 *idct_data,    // output data
			   int32 buf_width,     // data buffer width
			   int32 data_bit_width // input data bit width
			   )
{
	int32 med_buf[8][8];//med_buf[8];//weihu
	uint8 ix, iy;
	int32 *tmp_buf  = g_med_buf;
	int16 *in_buf16 = (int16*)dct_data;
	int8  *in_buf8  = (int8*)dct_data;
	int32 t0;
	/* line direction dct */
	for (iy = 0; iy < 8; iy++)
	{
		/* get data */		
		for (ix = 0; ix < 8; ix ++)
		{
			if (8 == data_bit_width)
			{
				tmp_buf[ix] = (*(in_buf8 + ix)) << (20 - data_bit_width);
			}else
			{
				tmp_buf[ix] = (*(in_buf16 + ix)) << (20 - data_bit_width);
			}			
		}
		/* 8 point dct */
		dct_1D_8(tmp_buf, MEDIAN_VAR_BIT);// the outdata is S21//weihu
		if (8 == data_bit_width)
		{
			in_buf8  += buf_width;
		}else
		{
			in_buf16  += buf_width;
		}		
		tmp_buf += 8;
	}

	/* transform as S20 */
	tmp_buf = g_med_buf;
	for(ix = 0; ix < 64; ix++)
	{
		int32 t;
		t   = tmp_buf[ix];
		t  |= 0x1;
		tmp_buf[ix] = t;
	} //bit[0] equal to 1, not need save
	
	//transpose buffer
	tmp_buf = g_med_buf;
	for (ix = 0; ix < 8; ix++)
	{
		/* get data */		
		for (iy = 0; iy < 8; iy ++)
		{
			med_buf[ix][iy] = *(tmp_buf + iy * 8);
			
		}
		tmp_buf ++;

    }

    /* column direction dct */
	for (ix = 0; ix < 8; ix++)
	{
		/* 8 point dct */
		
		dct_1D_8(&med_buf[ix][0], (MEDIAN_VAR_BIT+1));  // the outdata is S21
		
	}
	
	tmp_buf = g_med_buf;
	for (ix = 0; ix < 8; ix++)
	{
			
		for (iy = 0; iy < 8; iy ++)
		{
			*tmp_buf++ = med_buf[ix][iy];
		
		}
		
	}
	/* transform as S16 */
	tmp_buf = g_med_buf;
	for(ix = 0; ix < 64; ix++)
	{
		int32 t;
		t   = tmp_buf[ix] >> 5;
		t  |= 0x1;
		tmp_buf[ix] = t;
	}

	/* multify the DCT matrix and the output data fixed point S20 */
	for(ix = 0; ix < 64; ix++)
	{
		int32 tt;
		tt          = tmp_buf[ix] * g_dct_matrix[ix];//s16*u14==s29
		tmp_buf[ix] = tt >> 9;//s20
	}
	/*copy 20bits*/
//	memcpy ((uint8 *)(g_DCTCoeff_20Bit[g_iBlk]), (uint8 *)tmp_buf, 64*4);
	
	/* to S11 or S12 */
	iy = (uint8)(20 - 3 - data_bit_width);
	t0 = 1 << (iy - 1);
	for(ix = 0; ix < 64; ix++)
	{
		idct_data[ix] = (int16) ((tmp_buf[ix] + t0) >> iy);
	}
}

/*****************************************************************************
 **	Name : 			mpeg4_idct
 ** Description:	mpeg4_idct function.
 ** Author:			Xiaowei Luo
 **	Note:			dct_data:  input data and the fixed-point: S11.0 or S12.0
					idct_data: output data and the fixed-point: S8.0 or S9.0
 *****************************************************************************/
 void clip_block (int16* pBlk)
 {
	 int i;
	 int16 coeff;
	 
	 for (i = 0; i < 64; i++)
	 {
		 coeff = pBlk[i];
		 
		 if (coeff > 255)
			 coeff = 255;
		 else if (coeff < -256)
			 coeff = -256;
		 
		 pBlk[i] = coeff;
	 }
 }
 

void mpeg4_idct(int16 *idct_data,  // input data
				int16 *dct_data,    // output data
				uint32 buf_width,     // data buffer width
				uint32 data_bit_width // input data bit width
				)  	
{
	int32 med_buf[8][8];//med_buf[8];//weihu
	uint8 ix, iy;
	int32 *tmp_buf = g_med_buf;
	int16 *in_buf  = idct_data;
	int32 t0;
	/* line direction idct */	

	for (iy = 0; iy < 8; iy++)
	{
		/* get data */		
		for (ix = 0; ix < 8; ix ++)
		{
			t0 = (*(in_buf + ix));
			t0 = (t0 * IDCT_matrix[ix + iy * 8]) >> (data_bit_width - 7);//s12*u14>>5=s20
			tmp_buf[ix] = t0 | 0x1;
		}
		/* 8 point dct */
		idct_1D_8(tmp_buf, MEDIAN_VAR_BIT);	// the outdata is S20	
		in_buf  += buf_width;
		tmp_buf += 8;
	}
	
	//tanspose buffer//weihu
	tmp_buf = g_med_buf;
	for (ix = 0; ix < 8; ix++)
	{
		/* get data */		
		for (iy = 0; iy < 8; iy ++)
		{
			med_buf[ix][iy] = *(tmp_buf + iy * 8);
			
		}
		tmp_buf++;
	}//weihu
	
	/* column direction dct */
	for (ix = 0; ix < 8; ix++)//weihu
	{//weihu
	
		/* 8 point dct */
		//idct_1D_8(med_buf, (MEDIAN_VAR_BIT+1));  // the outdata is S20//weihu
		idct_1D_8(&med_buf[ix][0], (MEDIAN_VAR_BIT+1));  // the outdata is S20//weihu

    }//weihu

	tmp_buf = g_med_buf;
	for (ix = 0; ix < 8; ix++)//weihu
	{//weihu
	    for (iy = 0; iy < 8; iy ++)
		{
			//*(tmp_buf + iy * 8) = med_buf[iy];
            //*tmp_buf++ = med_buf[ix][iy];//weihu
			*tmp_buf++ = med_buf[iy][ix];//transpose
		}
		//tmp_buf ++;//weihu
	}

	tmp_buf = g_med_buf;
	/* to S11 or S12 */ //S9//weihu
	iy = (uint8)(20 + 3 - data_bit_width);//15
	t0 = 1 << (iy - 1);
	for(ix = 0; ix < 64; ix++)
	{
		dct_data[ix] = (int16) ((tmp_buf[ix] + t0) >> iy);
	}
	clip_block (dct_data);
}

/*****************************************************************************
 **	Name : 			Mp4Enc_Asic_Dct_Intra
 ** Description:	DCT for intra macroblock.
 ** Author:			Xiaowei Luo
 **	Note:			
 *****************************************************************************/
/*void Mp4Enc_Asic_Dct_Intra(int8 *pSrc, int16 *pDst,int32 DCScaler)
{
	mpeg4_dct ((void *)pSrc, pDst, 8, 9);
// 	pDst[0] += 1024;
	exchange_pos (pDst);
}*/

/*****************************************************************************
 **	Name : 			JpegEnc_Asic_Dct_Intra
 ** Description:	DCT for intra macroblock.
 ** Author:			Xiaowei Luo
 **	Note:			
 *****************************************************************************/
/*void JpegEnc_Asic_Dct_Intra(int8 *pSrc, int16 *pDst,int32 DCScaler)
{
	mpeg4_dct ((void *)pSrc, pDst, 8, 8);
	pDst[0] += 1024;
//	exchange_pos (pDst);
}*/

/*****************************************************************************
 **	Name : 			Mp4Enc_Asic_Dct_Inter
 ** Description:	DCT for inter macroblock.
 ** Author:			Xiaowei Luo
 **	Note:			
 *****************************************************************************/
//void Mp4Enc_Asic_Dct_Inter(int16 *pSrc, int16 *pDst)
/*void Mp4Enc_Asic_Dct(int16 *pSrc, int16 *pDst)
{
	mpeg4_dct((void *)pSrc, pDst, 8, 9);
	//exchange_pos(pDst);//weihu
}*/


/*****************************************************************************
 **	Name : 			Mp4_Asic_idct1
 ** Description:	IDCT function.
 ** Author:			Xiaowei Luo
 **	Note:			asic idct when input data is already in dct io buffer.
 *****************************************************************************/
/*void Mp4_Asic_idct1(int16 *pBlk)
{
//	tranpose_blk(pBlk);  
//	exchange_pos(pBlk);
   
	printf_IDCTCoeff_block(g_fp_idct_tv,pBlk);//weihu

	mpeg4_idct((int16 *)pBlk, pBlk, BLOCK_SIZE, 12);
//	clip_block (pBlk);
//	tranpose_blk(pBlk);  
}*/

/*****************************************************************************
 **	Name : 			Mp4_Asic_idct2
 ** Description:	IDCT function.
 ** Author:			Xiaowei Luo
 **	Note:			asic idct when input data is from extern buffer( such as decoder idct 
					and mpeg-4 encoder entra mb's idct
 *****************************************************************************/
void Mp4_Asic_idct2(int16 *pBlk)
{
	exchange_pos(pBlk);
	mpeg4_idct((int16 *)pBlk, pBlk, BLOCK_SIZE, 12);
}

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End 






















