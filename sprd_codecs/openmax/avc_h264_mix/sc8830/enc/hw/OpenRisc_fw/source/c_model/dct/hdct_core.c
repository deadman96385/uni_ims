
#include "sc6800x_video_header.h"

void trans4x4(int16 *src_ptr, int32 src_width, int16 *dst_ptr, int32 dst_width)
{
	int32 i;
	int16 tmp[4][4];	// [x][y]
	int16 *src = src_ptr;
	int16 *dst = dst_ptr;

	for (i = 0; i < 4; i++)
	{
		int32 s03 = src[0] + src[3];
		int32 s12 = src[1] + src[2];
		int32 d03 = src[0] - src[3];
		int32 d12 = src[1] - src[2];

		tmp[0][i] = s03 + s12;
		tmp[1][i] = 2*d03 + d12;
		tmp[2][i] = s03 - s12;
		tmp[3][i] = d03 - 2*d12;

		src += src_width;
	}

	for (i = 0; i < 4; i++)
	{
		int32 s03 = tmp[i][0] + tmp[i][3];
		int32 s12 = tmp[i][1] + tmp[i][2];
		int32 d03 = tmp[i][0] - tmp[i][3];
		int32 d12 = tmp[i][1] - tmp[i][2];

		dst[i*4+0] = s03 + s12;
		dst[i*4+1] = 2*d03 + d12;
		dst[i*4+2] = s03 - s12;
		dst[i*4+3] = d03 - 2*d12;
	}
	// dst[0~15] scan is 
	// 0 4 8  12 
	// 1 5 9  13
	// 2 6 10 14
	// 3 7 11 15
}

int32 quant4_mf[6][16] = 
{
	{ 13107, 8066, 13107, 8066, 8066, 5243, 8066, 5243, 13107, 8066, 13107, 8066, 8066, 5243, 8066, 5243},
	{ 11916, 7490, 11916, 7490, 7490, 4660, 7490, 4660, 11916, 7490, 11916, 7490, 7490, 4660, 7490, 4660},
	{ 10082, 6554, 10082, 6554, 6554, 4194, 6554, 4194, 10082, 6554, 10082, 6554, 6554, 4194, 6554, 4194},
	{ 9362,  5825, 9362, 5825, 5825, 3647, 5825, 3647, 9362, 5825, 9362, 5825, 5825, 3647, 5825, 3647	 },
	{ 8192,  5243, 8192, 5243, 5243, 3355, 5243, 3355, 8192, 5243, 8192, 5243, 5243, 3355, 5243, 3355	 },
	{ 7282,  4559, 7282, 4559, 4559, 2893, 4559, 2893, 7282, 4559, 7282, 4559, 4559, 2893, 4559, 2893	 },
};

void quant4x4 (int16 *dct4x4, int32 qp_per, int32 qp_rem, int32 b_intra)
{
	int32 qbits = 15 + qp_per;
	int32 f = g_qpPerF_tbl[qp_per] >> (b_intra ? 0 : 1);
	//int32 f = (1<<qbits) /(b_intra ? 3 : 6);
	int32 i;
	int32 *quant = quant4_mf[qp_rem];
	int16 coef;

	for (i = 0; i < 16; i++)
	{
		coef = dct4x4[i];
		if (coef > 0)
		{
			dct4x4[i] = (f + coef * quant[i]) >> qbits;
		}else
		{
			dct4x4[i] = -((f - coef * quant[i])>> qbits);
		}
	}
}

void zigzag4x4(int16 *src4x4, int16 *dst4x4)
{
	//because dct4x4 has reverseed in dct process 
	dst4x4[0] = src4x4[1*4+0];
	dst4x4[1] = src4x4[0*4+1];
	dst4x4[2] = src4x4[0*4+2];
	dst4x4[3] = src4x4[1*4+1];
	dst4x4[4] = src4x4[2*4+0];
	dst4x4[5] = src4x4[3*4+0];
	dst4x4[6] = src4x4[2*4+1];
	dst4x4[7] = src4x4[1*4+2];
	dst4x4[8] = src4x4[0*4+3];
	dst4x4[9] = src4x4[1*4+3];
	dst4x4[10] = src4x4[2*4+2];
	dst4x4[11] = src4x4[3*4+1];
	dst4x4[12] = src4x4[3*4+2];
	dst4x4[13] = src4x4[2*4+3];
	dst4x4[14] = src4x4[3*4+3];
}

int32 dequant4_mf[6][16] = 
{
	{ 160, 208, 160, 208, 208, 256, 208, 256, 160, 208, 160, 208, 208, 256, 208, 256},
	{ 176, 224, 176, 224, 224, 288, 224, 288, 176, 224, 176, 224, 224, 288, 224, 288},
	{ 208, 256, 208, 256, 256, 320, 256, 320, 208, 256, 208, 256, 256, 320, 256, 320},
	{ 224, 288, 224, 288, 288, 368, 288, 368, 224, 288, 224, 288, 288, 368, 288, 368},
	{ 256, 320, 256, 320, 320, 400, 320, 400, 256, 320, 256, 320, 320, 400, 320, 400},
	{ 288, 368, 288, 368, 368, 464, 368, 464, 288, 368, 288, 368, 368, 464, 368, 464}
};

void dequant4x4(int16 dct[16], int32 qp_per, int32 qp_rem)
{
	int32 qbits = qp_per-4;
	int32 y;

	if (qbits >= 0)
	{
		for (y = 0; y < 4; y++)
		{
			dct[y*4+0] = (dct[y*4+0] * dequant4_mf[qp_rem][y*4+0])<<qbits;
			dct[y*4+1] = (dct[y*4+1] * dequant4_mf[qp_rem][y*4+1])<<qbits;
			dct[y*4+2] = (dct[y*4+2] * dequant4_mf[qp_rem][y*4+2])<<qbits;
			dct[y*4+3] = (dct[y*4+3] * dequant4_mf[qp_rem][y*4+3])<<qbits;
		}
	}else
	{
		int32 f = 1 << (-qbits-1);
		
		for (y = 0; y < 4; y++)
		{
			dct[y*4+0] = (dct[y*4+0] * dequant4_mf[qp_rem][y*4+0]+f)>>(-qbits);
			dct[y*4+1] = (dct[y*4+1] * dequant4_mf[qp_rem][y*4+1]+f)>>(-qbits);
			dct[y*4+2] = (dct[y*4+2] * dequant4_mf[qp_rem][y*4+2]+f)>>(-qbits);
			dct[y*4+3] = (dct[y*4+3] * dequant4_mf[qp_rem][y*4+3]+f)>>(-qbits);
		}
	}
}

void dct4x4dc(int16 dc[16])
{
	int16 tmp[4][4]; // [x][y]
	int32 s01, s23;
	int32 d01, d23;
	int32 i;

	for (i = 0; i < 4; i++)
	{
		s01 = dc[i*4+0] + dc[i*4+1];
		d01 = dc[i*4+0] - dc[i*4+1];
		s23 = dc[i*4+2] + dc[i*4+3];
		d23 = dc[i*4+2] - dc[i*4+3];

		tmp[0][i] = s01 + s23;
		tmp[1][i] = s01 - s23;
		tmp[2][i] = d01 - d23;
		tmp[3][i] = d01 + d23;
	}

	for (i = 0; i < 4; i++)
	{
		s01 = tmp[i][0] + tmp[i][1];
		d01 = tmp[i][0] - tmp[i][1];
		s23 = tmp[i][2] + tmp[i][3];
		d23 = tmp[i][2] - tmp[i][3];

		dc[i*4+0] = (s01 + s23 + 1)>>1;
		dc[i*4+1] = (s01 - s23 + 1)>>1;
		dc[i*4+2] = (d01 - d23 + 1)>>1;
		dc[i*4+3] = (d01 + d23 + 1)>>1;
/*
		dc[i*4+0] = (s01 + s23)>>1;
		dc[i*4+1] = (s01 - s23)>>1;
		dc[i*4+2] = (d01 - d23)>>1;
		dc[i*4+3] = (d01 + d23)>>1;
*/
	}
	// dst[0~15] scan is 
	// 0 4 8  12 
	// 1 5 9  13
	// 2 6 10 14
	// 3 7 11 15
}

void quant4x4dc(int16 dc[16], int32 qp_per, int32 qp_rem)
{
	int32 qbits = 16 + qp_per;
	int32 mf = quant4_mf[qp_rem][0];
	int32 f = (g_qpPerF_tbl[qp_per] << 1);	// 16-15=1
	//int32 f = (1<<qbits)/3;
	int32 i;

	for (i = 0; i < 16; i++)
	{
		if (dc[i] > 0)
		{
			dc[i] = (f + dc[i]*mf) >> qbits;
		}else
		{
			dc[i] = -((f-dc[i]*mf)>>qbits);
		}
	}
}

void zigzag4x4_full(int16 *src4x4, int16 *dst4x4)
{
	//because dct4x4 has reverseed in dct process 
	dst4x4[0] = src4x4[0*4+0];
	dst4x4[1] = src4x4[1*4+0];
	dst4x4[2] = src4x4[0*4+1];
	dst4x4[3] = src4x4[0*4+2];
	dst4x4[4] = src4x4[1*4+1];
	dst4x4[5] = src4x4[2*4+0];
	dst4x4[6] = src4x4[3*4+0];
	dst4x4[7] = src4x4[2*4+1];
	dst4x4[8] = src4x4[1*4+2];
	dst4x4[9] = src4x4[0*4+3];
	dst4x4[10] = src4x4[1*4+3];
	dst4x4[11] = src4x4[2*4+2];
	dst4x4[12] = src4x4[3*4+1];
	dst4x4[13] = src4x4[3*4+2];
	dst4x4[14] = src4x4[2*4+3];
	dst4x4[15] = src4x4[3*4+3];
}

void idct4x4dc(int16 dc[16])
{
	int16 tmp[4][4];
	int32 s01, s23;
	int32 d01, d23;
	int32 i;

	for (i = 0; i < 4; i++)
	{
		s01 = dc[i*4+0] + dc[i*4+1];
		d01 = dc[i*4+0] - dc[i*4+1];
		s23 = dc[i*4+2] + dc[i*4+3];
		d23 = dc[i*4+2] - dc[i*4+3];

		tmp[0][i] = s01 + s23;
		tmp[1][i] = s01 - s23;
		tmp[2][i] = d01 - d23;
		tmp[3][i] = d01 + d23;
	}

	for (i = 0; i < 4; i++)
	{
		s01 = tmp[i][0] + tmp[i][1];
		d01 = tmp[i][0] - tmp[i][1];
		s23 = tmp[i][2] + tmp[i][3];
		d23 = tmp[i][2] - tmp[i][3];

		dc[i*4+0] = s01 + s23;
		dc[i*4+1] = s01 - s23;
		dc[i*4+2] = d01 - d23;
		dc[i*4+3] = d01 + d23;
	}
}

void dequant4x4dc(int16 dc[16], int32 qp_per, int32 qp_rem)
{
	int32 qbits = qp_per - 6;
	int32 y;

	if (qbits >= 0)
	{
		int32 dmf = dequant4_mf[qp_rem][0] << qbits;

		for (y = 0; y < 4; y++)
		{
			dc[y*4+0] *= dmf;
			dc[y*4+1] *= dmf;
			dc[y*4+2] *= dmf;
			dc[y*4+3] *= dmf;
		}
	}else
	{
		int32 dmf = dequant4_mf[qp_rem][0];
		int32 f = 1<< (-qbits-1);

		for (y = 0; y < 4; y++)
		{
			dc[y*4+0] = (dc[y*4+0]*dmf + f) >> (-qbits);
			dc[y*4+1] = (dc[y*4+1]*dmf + f) >> (-qbits);
			dc[y*4+2] = (dc[y*4+2]*dmf + f) >> (-qbits);
			dc[y*4+3] = (dc[y*4+3]*dmf + f) >> (-qbits);
		}
	}
}

void idct4x4(int16 *src_ptr, int32 src_width, int16 *dst_ptr, int32 dst_width)
{
	int16 tmp[4][4];
	int32 i;
	int16 *src = src_ptr; //4x4
	int16 *dst = dst_ptr;

	for (i = 0; i < 4; i++)
	{
		int32 s02 = src[0]				  + src[2*src_width];
		int32 d02 = src[0]				  - src[2*src_width];
		int32 s13 = src[1*src_width]	  + (src[3*src_width]>>1);
		int32 d13 = (src[1*src_width]>>1) - src[3*src_width];

		tmp[i][0] = s02 + s13;
		tmp[i][1] = d02 + d13;
		tmp[i][2] = d02 - d13;
		tmp[i][3] = s02 - s13;

		src++;
	}

	for (i = 0; i < 4; i++)
	{
		int32 s02 = tmp[0][i] + tmp[2][i];
		int32 d02 = tmp[0][i] - tmp[2][i];
		int32 s13 = tmp[1][i] + (tmp[3][i]>>1);
		int32 d13 = (tmp[1][i]>>1) - tmp[3][i];

		dst[0]           = (s02 + s13 + 32) >> 6;
		dst[1*dst_width] = (d02 + d13 + 32) >> 6;
		dst[2*dst_width] = (d02 - d13 + 32) >> 6;
		dst[3*dst_width] = (s02 - s13 + 32) >> 6;

		dst++;
	}
}

uint8 get_nnz4x4(int16 *vlc_coef_ptr, int32 start_pos)
{
	int32 i;
	uint32 nnz = 0;

	for (i = start_pos; i < 16; i++)
	{
		if (vlc_coef_ptr[i])
		{
			nnz++;
		}
	}

	return nnz;
}

void dct2x2dc(int16 dc[4])
{
	int32 tmp[2][2];

	tmp[0][0] = dc[0] + dc[1];
	tmp[1][0] = dc[0] - dc[1];
	tmp[0][1] = dc[2] + dc[3];
	tmp[1][1] = dc[2] - dc[3];

	dc[0] = tmp[0][0] + tmp[0][1];
	dc[2] = tmp[1][0] + tmp[1][1];
	dc[1] = tmp[0][0] - tmp[0][1];
	dc[3] = tmp[1][0] - tmp[1][1];
}

void quant2x2dc(int16 dc[4], int32 qp_per, int32 qp_rem, int32 b_intra)
{
	int32 qbits = 16 + qp_per;
	int32 mf = quant4_mf[qp_rem][0];
	int32 f = g_qpPerF_tbl[qp_per] << (b_intra ? 1 : 0);
	//int32 f = (1<<qbits)/(b_intra ? 3 : 6);
	int32 i;

	for (i = 0; i < 4; i++)
	{
		if (dc[i] > 0)
		{
			dc[i] = (f + dc[i]*mf) >> qbits;
		}else
		{
			dc[i] = -((f-dc[i]*mf)>>qbits);
		}
	}
}

void zigzag2x2_dc(int16 *src2x2, int16 *dst2x2)
{
	dst2x2[0] = src2x2[0];
	dst2x2[1] = src2x2[1*2+0];
	dst2x2[2] = src2x2[0*2+1];
	dst2x2[3] = src2x2[1*2+1];
}

void dequant2x2dc(int16 dc[4], int32 qp_per, int32 qp_rem)
{
	int32 qbits = qp_per - 5;
	
	if (qbits >= 0)
	{
		int32 dmf = dequant4_mf[qp_rem][0] << qbits;

		dc[0] *= dmf;
		dc[1] *= dmf;
		dc[2] *= dmf;
		dc[3] *= dmf;
	}else
	{
		int32 dmf = dequant4_mf[qp_rem][0];
		// chroma DC is truncated, not rounded
		dc[0] = (dc[0]*dmf) >> (-qbits);
		dc[1] = (dc[1]*dmf) >> (-qbits);
		dc[2] = (dc[2]*dmf) >> (-qbits);
		dc[3] = (dc[3]*dmf) >> (-qbits);
	}
}