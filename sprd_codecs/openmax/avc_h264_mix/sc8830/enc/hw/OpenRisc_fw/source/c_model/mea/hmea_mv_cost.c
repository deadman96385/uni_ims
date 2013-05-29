/*mv_cost.c*/
#include "video_common.h"
#include "mmcodec.h"
#include "hmea_mode.h"
#include "hmea_global.h"

//#define LAMBDA_ACCURACY_BITS 16
//#define  WEIGHTED_COST(factor,bits)   (((factor) * (bits)) >> LAMBDA_ACCURACY_BITS)
//#define  MV_COST_SMP(f,cx,cy,px,py)  (WEIGHTED_COST(f,bs_size_se(cx - px) + bs_size_se(cy - py)))

int32	i_size0_254[255] = 
{
	1, 3, 3, 5, 5, 5, 5, 7, 7, 7, 7, 7, 7, 7, 7,
		9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
		11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11,
		11, 11, 11, 11, 11, 11, 11, 11, 11, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13,
		13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 
		13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 
		13, 13, 13, 13, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
		15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
		15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
		15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
		15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
		15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15
};


/**********************************************************************
note: all the mv is in quarter pixel resolution
**********************************************************************/
int32 bs_size_ue (uint32 val)
{	
	if (val < 255)
	{
		return i_size0_254[val];
	}
	else
	{
		int32 i_size = 0;
		
		val++;
		
		if (val >= 0x10000)
		{
			i_size += 32;
			val = (val >> 16) - 1;
		}
		
		if (val >= 0x100)
		{
			i_size += 16;
			val = (val >> 8) - 1;
		}
		
		return i_size0_254[val] + i_size;
	}
}

int32 bs_size_se (int32 val)
{
	return bs_size_ue (val <= 0 ? -val*2 : val*2-1);
}

int BitsMvdMpeg4 (int mvd)
{
	int f_code;
	int n_bits;
	int abs_mvd;
	int nor_mvd;

	int search_range = g_mode_dcs_ptr->sch_range_x;

	if (mvd == 0)
	{
		n_bits = 1;
	}
	else
	{
		f_code = (search_range <= 16) ? 1 : 
				 (search_range <= 32) ? 2 : 3;

		abs_mvd = ABS(mvd);

		if (abs_mvd <= 32)
		{
			nor_mvd = abs_mvd;
		}
		else if(abs_mvd <= 64)
		{
			nor_mvd = abs_mvd >> 1;
		}
		else
		{
			nor_mvd = abs_mvd >> 2;
		}

		n_bits = (nor_mvd == 1) ? 3 :
				 (nor_mvd == 2) ? 4 :
				 (nor_mvd == 3) ? 5 :
				 (nor_mvd == 4) ? 7 :
				 (nor_mvd <  8) ? 8 :
				 (nor_mvd < 11) ? 10 :
				 (nor_mvd < 25) ? 11 :
				 (nor_mvd < 31) ? 12 : 13;

		n_bits += f_code - 1;
	}

	return n_bits;
}

int CostMVD (int mv_x, int mv_y, int mvp_x, int mvp_y)
{
	int cost;
	int mvdx;
	int mvdy;
	int is_h264;
	int bits_mvdx;
	int bits_mvdy;
	int lambda = g_mode_dcs_ptr->lambda;

	is_h264 = (g_standard == VSP_H264) ? 1 : 0;

	mvdx = mv_x - mvp_x;
	mvdy = mv_y - mvp_y;

	bits_mvdx = is_h264 ? bs_size_se(mvdx) : BitsMvdMpeg4 (mvdx>>1);
	bits_mvdy = is_h264 ? bs_size_se(mvdy) : BitsMvdMpeg4 (mvdy>>1);
	
//	cost = lambda * bs_size_se(mv_x - mvp_x) + lambda * bs_size_se(mv_y - mvp_y);
	cost = lambda * bits_mvdx + lambda * bits_mvdy;
	
	return cost;
}