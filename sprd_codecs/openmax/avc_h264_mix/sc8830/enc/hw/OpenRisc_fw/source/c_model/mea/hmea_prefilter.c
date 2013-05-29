/*pre_filter.c*/
#include "video_common.h"
#include "mmcodec.h"
#include "hmea_mode.h"
#include "hmea_global.h"

/*high frequence coefficient modification*/
int HfCoeffMdf (int val, int thresh)
{
	int val_mdf;
	int abs_val;
	int sign;

	abs_val = (val >= 0) ? val : -val;
	sign    = (val >= 0) ? 0 : 1;

	if (abs_val <= thresh)
	{
		abs_val = 0;
	}
	else
	{
		abs_val = abs_val - thresh;
	}

	val_mdf = (sign == 0) ? abs_val : -abs_val;

	return val_mdf;
}

uint8 PreFilter (
				 uint8	x0, 
				 uint8	x1, 
				 uint8	x2, 
				 uint8	x3, 
				 int	thresh
				 )
{
	int		y0;
	int		y1;
	int		y2;
	int		y3;

	int		y1_m;
	int		y2_m;
	int		y3_m;

	int		x0_m;

	/*forward wavelet transform*/
	y0 = (x0 + x1 + x2 + x3);
	y1 = (x0 - x1 + x2 - x3);
	y2 = (x0 + x1 - x2 - x3);
	y3 = (x0 - x1 - x2 + x3);

	/*coeffient modification in high frequence*/
	y1_m = HfCoeffMdf (y1, thresh);
	y2_m = HfCoeffMdf (y2, thresh);
	y3_m = HfCoeffMdf (y3, thresh);

	/*inverse wavelet transform*/
	x0_m = (y0 + y1_m + y2_m + y3_m + 2) / 4;

	if (x0_m < 0)
		x0_m = 0;

	if (x0_m > 255)
		x0_m = 255;

	if (!g_mea_fetch_ptr->pre_filter_en)
	{
		x0_m = x0;   //only for debug
	}

	return  x0_m;
}
