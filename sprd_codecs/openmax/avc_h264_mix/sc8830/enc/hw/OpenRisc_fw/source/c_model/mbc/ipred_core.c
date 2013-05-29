#include "sci_types.h"
#include "video_common.h"
#include "common_global.h"
#include "ipred_global.h"
#include "mbc_global.h"

/*blkIdxInMB order
___ ___ ___ ___
| 0 | 1 | 2 | 3 |
|___|___|___|___|
| 4 | 5 | 6 | 7 |
|___|___|___|___|
| 8 | 9 | 10| 11|
|___|___|___|___|
| 12| 13| 14| 15|
|___|___|___|___| 

 */


void get_nei_pixels (int32 blkIdxInMB,
					 uint8 *pPred, 
					 uint8 *pTopLeftPix,
					 uint8 *pLeftPix,
					 uint8 left[], 
					 uint8 top[])
{
	uint8 *pPixTop, *pPixLeft;
	
	if (blkIdxInMB == 0)
	{
		top[0] = pTopLeftPix[0];
		top[1] = pTopLeftPix[1];
		top[2] = pTopLeftPix[2];
		top[3] = pTopLeftPix[3];
		top[4] = pTopLeftPix[4];

		left[0] = top[0];
		left[1] = pLeftPix[0];
		left[2] = pLeftPix[1];
		left[3] = pLeftPix[2];
		left[4] = pLeftPix[3];
	}else if (blkIdxInMB < 4) //blk 1, 2, 3
	{
		pPixTop = pTopLeftPix + blkIdxInMB * 4;

		top[0] = pPixTop[0];
		top[1] = pPixTop[1];
		top[2] = pPixTop[2];
		top[3] = pPixTop[3];
		top[4] = pPixTop[4];

		left[0] = top[0];
		pPixLeft = pPred - 1;
		left[1] = pPixLeft[0*MBC_Y_SIZE];
		left[2] = pPixLeft[1*MBC_Y_SIZE];
		left[3] = pPixLeft[2*MBC_Y_SIZE];
		left[4] = pPixLeft[3*MBC_Y_SIZE];
	}else
	{
		pPixLeft = pLeftPix + blkIdxInMB - 1;
		left[0] = pPixLeft[0];
		left[1] = pPixLeft[1];
		left[2] = pPixLeft[2];
		left[3] = pPixLeft[3];
		left[4] = pPixLeft[4];

		top[0] = left[0];
		pPixTop = pPred - MBC_Y_SIZE;
		top[1] = pPixTop[0];
		top[2] = pPixTop[1];
		top[3] = pPixTop[2];
		top[4] = pPixTop[3];
	}

	return;
}

//4x4 luma intra prediction

//mode 0
void intra_pred_luma4x4_VERT_PRED (int32 blkIdxInMB, 
								  uint8 *pPred, 
								  uint8 *pTopLeftPix,
								  uint8 *pLeftPix
								  )
{
	uint32 a;
	uint8 *pPix;
	uint32 *pIntPix;

	if (blkIdxInMB >= 4)
	{
		pPix = pPred - MBC_Y_SIZE;
	}else
	{
		int32 avail = (g_mbc_reg_ptr->MBC_CMD0 >> 25) & 0x01; //mbAvailB

		if (avail == 0)
		{
			PRINTF ("VERT_PRED4x4 luma not allowed!\n");
			return;
		}

		pPix = pTopLeftPix + blkIdxInMB * 4 + 1;
	}

if(g_trace_enable_flag&TRACE_ENABLE_IPRED)	
{
	FPRINTF(g_ipred_log_fp, "A = %d, B = %d, C = %d, D = %d\n", pPix[0], pPix[1], pPix[2], pPix[3]);
}
	a = *((uint32 *)pPix);

	pIntPix = (uint32 *)pPred;

	pIntPix[0] = a; pIntPix += 6;
	pIntPix[0] = a; pIntPix += 6; //next line
	pIntPix[0] = a; pIntPix += 6; //next line
	pIntPix[0] = a; //next line

	return;
}

//mode 1
void intra_pred_luma4x4_HOR_PRED (int32 blkIdxInMB, 
								  uint8 *pPred, 
								  uint8 *pTopLeftPix,
								  uint8 *pLeftPix
								  )
{
	int32 i;
	uint8 *pPix, *pPixBlk;
	uint8 pixVal[4];

	if ((blkIdxInMB % 4) != 0) //not the first column block
	{
		pPix = pPred - 1;

		pixVal[0] = pPix[MBC_Y_SIZE * 0]; 
		pixVal[1] = pPix[MBC_Y_SIZE * 1]; //next line
		pixVal[2] = pPix[MBC_Y_SIZE * 2]; //next line
		pixVal[3] = pPix[MBC_Y_SIZE * 3]; //next line
	}else //first column block
	{
		int32 avail = (g_mbc_reg_ptr->MBC_CMD0 >> 26) & 0x01; //mbAvailA

		if (avail == 0)
		{
			PRINTF ("HOR_PRED4x4 luma not allowed!\n");
			return;
		}

		pPix = pLeftPix + blkIdxInMB;

		pixVal[0] = pPix[0];
		pixVal[1] = pPix[1]; //next line
		pixVal[2] = pPix[2]; //next line
		pixVal[3] = pPix[3]; //next line
	}
if(g_trace_enable_flag&TRACE_ENABLE_IPRED)	
{
	FPRINTF(g_ipred_log_fp, "I = %d, J = %d, K = %d, L = %d\n", pixVal[0], pixVal[1], pixVal[2], pixVal[3]);
}
	pPixBlk = pPred;

	for (i = 0; i < 4; i++) //four line
	{
		pPixBlk[0] = pixVal[i]; //pixel 0
		pPixBlk[1] = pixVal[i]; //pixel 1
		pPixBlk[2] = pixVal[i]; //pixel 2
		pPixBlk[3] = pixVal[i]; //pixel 3

		//next line
		pPixBlk += MBC_Y_SIZE;
	}

	return;
}

//mode 2
void intra_pred_luma4x4_DC_PRED (int32 blkIdxInMB, 
								  uint8 *pPred, 
								  uint8 *pTopLeftPix,
								  uint8 *pLeftPix
								  )
{
	int32 i;
	int32 avail;
	int32 pred_sum = 0;
	uint8 *pPix;
	uint8 *pPixBlk;
	int32 blk_avail_num = 0;

	//left block
	if ((blkIdxInMB % 4) != 0) //not the first column block
	{
		blk_avail_num ++;

		pPix = pPred - 1;

		pred_sum += pPix[0 * MBC_Y_SIZE];
		pred_sum += pPix[1 * MBC_Y_SIZE]; //next line
		pred_sum += pPix[2 * MBC_Y_SIZE]; //next line
		pred_sum += pPix[3 * MBC_Y_SIZE]; //next line

		if(g_trace_enable_flag&TRACE_ENABLE_IPRED)	
		{
		FPRINTF(g_ipred_log_fp, "I = %d, J = %d, K = %d, L = %d\n", 
			pPix[0 * MBC_Y_SIZE], pPix[1 * MBC_Y_SIZE], pPix[2 * MBC_Y_SIZE], pPix[3 * MBC_Y_SIZE]);
		}
	}else
	{
		avail = (g_mbc_reg_ptr->MBC_CMD0 >> 26) & 0x01; //mbAvailA

		if (avail)
		{
			blk_avail_num++;

			pPix = pLeftPix + blkIdxInMB;

			pred_sum += pPix[0]; 
			pred_sum += pPix[1]; //next line
			pred_sum += pPix[2]; //next line
			pred_sum += pPix[3]; //next line
			if(g_trace_enable_flag&TRACE_ENABLE_IPRED)	
			{
				FPRINTF(g_ipred_log_fp, "I = %d, J = %d, K = %d, L = %d\n", pPix[0], pPix[1], pPix[2], pPix[3]);
			}
		}else
		{
			if(g_trace_enable_flag&TRACE_ENABLE_IPRED)	
			{
				FPRINTF(g_ipred_log_fp, "I = X, J = X, K = X, L = X\n");
			}
		}
	}

	//top pixel
	if (blkIdxInMB >= 4)
	{
		blk_avail_num++;

		pPix = pPred - MBC_Y_SIZE;

		pred_sum += pPix[0]; 
		pred_sum += pPix[1]; //next line
		pred_sum += pPix[2]; //next line
		pred_sum += pPix[3]; //next line

		if(g_trace_enable_flag&TRACE_ENABLE_IPRED)	
		{
			FPRINTF(g_ipred_log_fp, "A = %d, B = %d, C = %d, D = %d\n", pPix[0], pPix[1], pPix[2], pPix[3]);
		}
	}else
	{
		avail = (g_mbc_reg_ptr->MBC_CMD0 >> 25) & 0x01; //mbAvailB

		if (avail)
		{
			blk_avail_num++;

			pPix = pTopLeftPix + blkIdxInMB * 4 + 1;

			pred_sum += pPix[0]; 
			pred_sum += pPix[1]; //next line
			pred_sum += pPix[2]; //next line
			pred_sum += pPix[3]; //next line

			if(g_trace_enable_flag&TRACE_ENABLE_IPRED)	
			{
				FPRINTF(g_ipred_log_fp, "A = %d, B = %d, C = %d, D = %d\n", pPix[0], pPix[1], pPix[2], pPix[3]);
			}
		}else
		{
			if(g_trace_enable_flag&TRACE_ENABLE_IPRED)	
			{
				FPRINTF(g_ipred_log_fp, "A = X, B = X, C = X, D = X\n");
			}
		}
	}

	if (blk_avail_num == 2)
	{
		pred_sum = (pred_sum + 4) >> 3;
	}else if (blk_avail_num == 0)
	{
		pred_sum = 128;
	}else
	{
		pred_sum = (pred_sum + 2) >> 2;
	}

	pPixBlk = pPred;
	for (i = 0; i < 4; i++) //four line
	{
		pPixBlk[0] = pred_sum; //pixel 0
		pPixBlk[1] = pred_sum; //pixel 1
		pPixBlk[2] = pred_sum; //pixel 2
		pPixBlk[3] = pred_sum; //pixel 3

		//next line
		pPixBlk += MBC_Y_SIZE;
	}

	return;
}

int32 s_blkC_avai_tbl[16] = 
{
    1, 1, 1, 1,
	1, 0, 1, 0,
	1, 1, 1, 0, 
	1, 0, 1, 0, 
};

//mode 3
void intra_pred_luma4x4_DIAG_DOWN_LEFT_PRED (int32 blkIdxInMB, 
											  uint8 *pPred, 
											  uint8 *pTopLeftPix,
											  uint8 *pLeftPix
											  )
{
	int32 blkC_avail;
	uint8 *pPix;
	uint8 *pPixBlk;
	uint8 t0, t1, t2, t3, t4, t5, t6, t7;
	
	if (blkIdxInMB >= 4) //not the first row block
	{
		pPix = pPred - MBC_Y_SIZE;

		blkC_avail = s_blkC_avai_tbl[blkIdxInMB];  //NOTE!!
	}else
	{
		if (blkIdxInMB < 3)
		{
			int32 top_avail = (g_mbc_reg_ptr->MBC_CMD0 >> 25) & 0x01; //mbAvailB

			if (top_avail == 0)
			{
				PRINTF ("DIAG_DOWN_LEFT_PRED4x4 luma not allowed!\n");
				return;
			}
			
			blkC_avail = top_avail;
		}else
		{
			int32 top_right_avail = (g_mbc_reg_ptr->MBC_CMD0 >> 24) & 0x01; //mbAvailC

			blkC_avail = top_right_avail;
		}

		pPix = pTopLeftPix + blkIdxInMB * 4 + 1;
	}

	t0 = pPix[0];
	t1 = pPix[1];
	t2 = pPix[2];
	t3 = pPix[3];

	if (blkC_avail)
	{
		t4 = pPix[4];
		t5 = pPix[5];
		t6 = pPix[6];
		t7 = pPix[7];
	}else
	{
		t4 = t3;
		t5 = t3;
		t6 = t3;
		t7 = t3;
	}

	if(g_trace_enable_flag&TRACE_ENABLE_IPRED)	
	{
		FPRINTF(g_ipred_log_fp, "A = %d, B = %d, C = %d, D = %d, E = %d, F = %d, G = %d, H = %d\n", t0, t1, t2, t3, t4, t5, t6, t7);
	}

	pPixBlk = pPred;

	pPixBlk[0]/*a*/= (t0 + t1 *2 + t2 + 2) >> 2; 
	pPixBlk[1]/*b*/= pPixBlk[1 * MBC_Y_SIZE]/*e*/ = (t1 + t2 * 2 + t3 + 2) >> 2;
	pPixBlk[2]/*c*/ = pPixBlk[1*MBC_Y_SIZE+1]/*f*/ = pPixBlk[2*MBC_Y_SIZE+0]/*i*/ = (t2 + t3 * 2 + t4 + 2) >> 2;
	pPixBlk[3]/*d*/ = pPixBlk[1*MBC_Y_SIZE+2]/*g*/ = pPixBlk[2*MBC_Y_SIZE+1]/*j*/ = pPixBlk[3*MBC_Y_SIZE+0]/*m*/ = (t3 + t4 * 2 + t5 + 2) >> 2;
	pPixBlk[1*MBC_Y_SIZE+3]/*h*/ = pPixBlk[2*MBC_Y_SIZE+2]/*k*/ = pPixBlk[3*MBC_Y_SIZE+1]/*n*/ = (t4 + t5 * 2 + t6 + 2) >> 2;
	pPixBlk[2*MBC_Y_SIZE+3]/*l*/ = pPixBlk[3*MBC_Y_SIZE+2]/*o*/ = (t5 + t6 * 2 + t7 + 2) >> 2;
	pPixBlk[3*MBC_Y_SIZE+3]/*p*/ = (t6 + t7 * 3 + 2) >> 2;

	return;
}

//mode 4
void intra_pred_luma4x4_DIAG_DOWN_RIGHT_PRED (int32 blkIdxInMB, 
											  uint8 *pPred, 
											  uint8 *pTopLeftPix,
											  uint8 *pLeftPix
											  )
{
	uint8 *pPix, *pPixBlk;
	uint8 tn1, t0, t1, t2, t3;
	uint8 ln1, l0, l1, l2, l3;

	if ((blkIdxInMB < 4) /*the first row block*/ ||
		((blkIdxInMB & 0x3) == 0)) /*the first column block*/
	{
		uint8 left[5], top[5];

		/*compute left and top respectively*/
		get_nei_pixels (blkIdxInMB, pPred, pTopLeftPix, pLeftPix, left, top);

		tn1 = top[0];
		t0  = top[1];
		t1  = top[2];
		t2  = top[3];
		t3  = top[4];

		ln1 = left[0];
		l0  = left[1];
		l1  = left[2];
		l2  = left[3];
		l3  = left[4];
	}else
	{
		pPix = pPred - MBC_Y_SIZE - 1;

		tn1 = pPix[0];
		t0  = pPix[1];
		t1  = pPix[2];
		t2  = pPix[3];
		t3  = pPix[4];

		ln1 = tn1;
		l0  = pPix[1*MBC_Y_SIZE];
		l1  = pPix[2*MBC_Y_SIZE];
		l2  = pPix[3*MBC_Y_SIZE];
		l3  = pPix[4*MBC_Y_SIZE];
	}

	if(g_trace_enable_flag&TRACE_ENABLE_IPRED)	
	{
		FPRINTF(g_ipred_log_fp, "M = %d\n", tn1);
		FPRINTF(g_ipred_log_fp, "A = %d, B = %d, C = %d, D = %d\n", t0, t1, t2, t3);
		FPRINTF(g_ipred_log_fp, "I = %d, J = %d, K = %d, L = %d\n", l0, l1, l2, l3);
	}

	pPixBlk = pPred;

	/*x coordinate greater than y*/
	pPixBlk[1]/*b*/ = pPixBlk[2+1*MBC_Y_SIZE]/*g*/ = pPixBlk[3+2*MBC_Y_SIZE]/*l*/ = (tn1 + t0 * 2 + t1 + 2)>>2;
	pPixBlk[2]/*c*/ = pPixBlk[3+1*MBC_Y_SIZE]/*h*/ = (t0 + t1 * 2 + t2 + 2)>>2;
	pPixBlk[3]/*d*/ = (t1 + t2 * 2 + t3 + 2) >> 2;

	/*x coordinate equal y*/
	pPixBlk[0]/*a*/ = pPixBlk[1+1*MBC_Y_SIZE]/*f*/ = pPixBlk[2+2*MBC_Y_SIZE]/*k*/ = pPixBlk[3+3*MBC_Y_SIZE]/*p*/ = (t0 + tn1 * 2 + l0 + 2)>>2;

	/*x < y*/
	pPixBlk[1*MBC_Y_SIZE]/*e*/ = pPixBlk[1+2*MBC_Y_SIZE]/*j*/ = pPixBlk[2+3*MBC_Y_SIZE]/*o*/ = (ln1 + l0 * 2 + l1 + 2)>>2;
	pPixBlk[2*MBC_Y_SIZE]/*i*/ = pPixBlk[1+3*MBC_Y_SIZE]/*n*/ = (l0 + l1 * 2 + l2 + 2)>>2;
	pPixBlk[3*MBC_Y_SIZE]/*m*/ = (l1 + l2 * 2 + l3 + 2) >> 2;

	return;
}

//mode 5
void intra_pred_luma4x4_VERT_RIGHT_PRED (int32 blkIdxInMB, 
											  uint8 *pPred, 
											  uint8 *pTopLeftPix,
											  uint8 *pLeftPix
											  )
{
	uint8 *pPix, *pPixBlk;
	uint8 tn1, t0, t1, t2, t3;
	uint8 l0, l1, l2;

	if ((blkIdxInMB < 4) /*the first row block*/ ||
		((blkIdxInMB & 0x3) == 0)) /*the first column block*/
	{
		uint8 left[5], top[5];

		/*compute left and top respectively*/
		get_nei_pixels (blkIdxInMB, pPred, pTopLeftPix, pLeftPix, left, top);

		tn1 = top[0];
		t0  = top[1];
		t1  = top[2];
		t2  = top[3];
		t3  = top[4];

		l0  = left[1];
		l1  = left[2];
		l2  = left[3];
	}else
	{
		pPix = pPred - MBC_Y_SIZE - 1;

		tn1 = pPix[0];
		t0  = pPix[1];
		t1  = pPix[2];
		t2  = pPix[3];
		t3  = pPix[4];

		l0  = pPix[1*MBC_Y_SIZE];
		l1  = pPix[2*MBC_Y_SIZE];
		l2  = pPix[3*MBC_Y_SIZE];
	}

if(g_trace_enable_flag&TRACE_ENABLE_IPRED)	
{
	FPRINTF(g_ipred_log_fp, "M = %d\n", tn1);
	FPRINTF(g_ipred_log_fp, "A = %d, B = %d, C = %d, D = %d\n", t0, t1, t2, t3);
	FPRINTF(g_ipred_log_fp, "I = %d, J = %d, K = %d\n", l0, l1, l2);
}
	pPixBlk = pPred;

	/*(2*x - y) equal 0, 2, 4, 6*/
	pPixBlk[0]/*a*/ = pPixBlk[1+2*MBC_Y_SIZE]/*j*/ = (tn1 + t0 + 1)/2;
	pPixBlk[1]/*b*/ = pPixBlk[2+2*MBC_Y_SIZE]/*k*/ = (t0 + t1 + 1)/2;
	pPixBlk[2]/*c*/ = pPixBlk[3+2*MBC_Y_SIZE]/*l*/ = (t1 + t2 + 1)/2;
	pPixBlk[3]/*d*/ = (t2 + t3 + 1)/2;

	/*(2*x - y) equal 1, 3, 5*/
	pPixBlk[1+MBC_Y_SIZE]/*f*/ = pPixBlk[2+3*MBC_Y_SIZE]/*o*/ = (tn1 + t0 *2 + t1 + 2)/4;
	pPixBlk[2+MBC_Y_SIZE]/*g*/ = pPixBlk[3+3*MBC_Y_SIZE]/*p*/ = (t0 + t1 * 2 + t2 + 2)/4;
	pPixBlk[3+MBC_Y_SIZE]/*h*/ = (t1 + t2 *2 + t3 + 2)/4;
	
	/*(2*x - y) equal -1*/
	pPixBlk[MBC_Y_SIZE]/*e*/ = pPixBlk[1+3*MBC_Y_SIZE]/*n*/ = (l0 + tn1 * 2 + t0 + 2)/4;

	/*(2*x - y) equal -2*/
	pPixBlk[2*MBC_Y_SIZE]/*i*/ = (tn1 + l0 * 2 + l1 + 2)/4;

	/*(2*x - y) equal -3*/
	pPixBlk[3*MBC_Y_SIZE]/*m*/ = (l0 + l1 * 2 + l2 + 2)/4;
}

//mode 6
void intra_pred_luma4x4_HOR_DOWN_PRED (int32 blkIdxInMB, 
									  uint8 *pPred, 
									  uint8 *pTopLeftPix,
									  uint8 *pLeftPix
									  )
{
	uint8 *pPix, *pPixBlk;
	uint8 tn1, t0, t1, t2;
	uint8 ln1, l0, l1, l2, l3;

	if ((blkIdxInMB < 4) /*the first row block*/ ||
		((blkIdxInMB & 0x3) == 0)) /*the first column block*/
	{
		uint8 left[5], top[5];

		/*compute left and top respectively*/
		get_nei_pixels (blkIdxInMB, pPred, pTopLeftPix, pLeftPix, left, top);

		tn1 = top[0];
		t0  = top[1];
		t1  = top[2];
		t2  = top[3];

		ln1 = left[0];
		l0  = left[1];
		l1  = left[2];
		l2  = left[3];
		l3  = left[4];
	}else
	{
		pPix = pPred - MBC_Y_SIZE - 1;

		tn1 = pPix[0];
		t0  = pPix[1];
		t1  = pPix[2];
		t2  = pPix[3];

		ln1 = tn1;
		l0  = pPix[1*MBC_Y_SIZE];
		l1  = pPix[2*MBC_Y_SIZE];
		l2  = pPix[3*MBC_Y_SIZE];
		l3  = pPix[4*MBC_Y_SIZE];
	}

	if(g_trace_enable_flag&TRACE_ENABLE_IPRED)	
	{
		FPRINTF(g_ipred_log_fp, "M = %d\n", tn1);
		FPRINTF(g_ipred_log_fp, "A = %d, B = %d, C = %d\n", t0, t1, t2);
		FPRINTF(g_ipred_log_fp, "I = %d, J = %d, K = %d, L = %d\n", l0, l1, l2, l3);
	}

	pPixBlk = pPred;

	/*(2*y - x) equal 0, 2, 4, 6*/
	pPixBlk[0]/*a*/ = pPixBlk[2+MBC_Y_SIZE]/*g*/ = (ln1 + l0 + 1)/2;
	pPixBlk[0+1*MBC_Y_SIZE]/*e*/ = pPixBlk[2+2*MBC_Y_SIZE]/*k*/ = (l0 + l1 + 1)/2;
	pPixBlk[0+2*MBC_Y_SIZE]/*i*/ = pPixBlk[2+3*MBC_Y_SIZE]/*o*/ = (l1 + l2 + 1)/2;
	pPixBlk[0+3*MBC_Y_SIZE]/*m*/ = (l2 + l3 + 1)/2;

	/*(2*y - x) equal 1, 3, 5*/
	pPixBlk[1+1*MBC_Y_SIZE]/*f*/ = pPixBlk[3+2*MBC_Y_SIZE]/*l*/ = (ln1 + l0 * 2 + l1 + 2)/4;
	pPixBlk[1+2*MBC_Y_SIZE]/*j*/ = pPixBlk[3+3*MBC_Y_SIZE]/*p*/ = (l0 + l1 * 2 + l2 + 2)/4;
	pPixBlk[1+3*MBC_Y_SIZE]/*n*/ = (l1 + l2 * 2 + l3 + 2)/4;

	/*(2*y - x) equal -1*/
	pPixBlk[1]/*b*/ = pPixBlk[3+1*MBC_Y_SIZE]/*h*/ = (l0 + ln1 * 2 + t0 + 2)/4;
	
	/*(2*y - x) equal -2*/
	pPixBlk[2]/*c*/ = (ln1 + t0 * 2 + t1 + 2)/4;

	/*(2*y - x) equal -3*/
	pPixBlk[3]/*d*/ = (t0 + t1 * 2 + t2 + 2)/4;

}

//mode 7
void intra_pred_luma4x4_VERT_LEFT_PRED (int32 blkIdxInMB, 
									  uint8 *pPred, 
									  uint8 *pTopLeftPix,
									  uint8 *pLeftPix
									  )
{
	int32 blkC_avail;
	uint8 *pPix, *pPixBlk;
	uint8 t0, t1, t2, t3, t4, t5, t6;

	if (blkIdxInMB >= 4) //not the first row block
	{
		pPix = pPred - MBC_Y_SIZE;

		blkC_avail = s_blkC_avai_tbl[blkIdxInMB];  //NOTE!!
	}else
	{
		if (blkIdxInMB < 3)
		{
			int32 top_avail = (g_mbc_reg_ptr->MBC_CMD0 >> 25) & 0x01; //mbAvailB

			if (top_avail == 0)
			{
				PRINTF ("VERT_LEFT_PRED4x4 luma not allowed!\n");
				return;
			}
			
			blkC_avail = top_avail;
		}else
		{
			int32 top_right_avail = (g_mbc_reg_ptr->MBC_CMD0 >> 24) & 0x01; //mbAvailC

			blkC_avail = top_right_avail;
		}

		pPix = pTopLeftPix + blkIdxInMB * 4 + 1;
	}

	pPixBlk = pPred;

	t0 = pPix[0];
	t1 = pPix[1];
	t2 = pPix[2];
	t3 = pPix[3];
	if (!blkC_avail)
	{
		t4 = t3;
		t5 = t3;
		t6 = t3;
	}else
	{
		t4 = pPix[4];
		t5 = pPix[5];
		t6 = pPix[6];
	}

	if(g_trace_enable_flag&TRACE_ENABLE_IPRED)	
	{
		FPRINTF(g_ipred_log_fp, "A = %d, B = %d, C = %d, D = %d, E = %d, F = %d, G = %d\n", t0, t1, t2, t3, t4, t5, t6);
	}

	/*y = 0 or 2*/
	pPixBlk[0+0*MBC_Y_SIZE]/*a*/ = (t0 + t1 + 1)/2;
	pPixBlk[1+0*MBC_Y_SIZE]/*b*/ = pPixBlk[0+2*MBC_Y_SIZE]/*i*/ = (t1 + t2 + 1)/2;
	pPixBlk[2+0*MBC_Y_SIZE]/*c*/ = pPixBlk[1+2*MBC_Y_SIZE]/*j*/ = (t2 + t3 + 1)/2;
	pPixBlk[3+0*MBC_Y_SIZE]/*d*/ = pPixBlk[2+2*MBC_Y_SIZE]/*k*/ = (t3 + t4 + 1)/2;
	pPixBlk[3+2*MBC_Y_SIZE]/*l*/ = (t4 + t5 + 1)/2;

	/*y = 1 or 3*/
	pPixBlk[0+1*MBC_Y_SIZE]/*e*/ = (t0 + t1 * 2 + t2 + 2)/4;
	pPixBlk[1+1*MBC_Y_SIZE]/*f*/ = pPixBlk[0+3*MBC_Y_SIZE]/*m*/ = (t1 + t2 * 2 + t3 + 2)/4;
	pPixBlk[2+1*MBC_Y_SIZE]/*g*/ = pPixBlk[1+3*MBC_Y_SIZE]/*n*/ = (t2 + t3 * 2 + t4 + 2)/4;
	pPixBlk[3+1*MBC_Y_SIZE]/*h*/ = pPixBlk[2+3*MBC_Y_SIZE]/*o*/ = (t3 + t4 * 2 + t5 + 2)/4;
	pPixBlk[3+3*MBC_Y_SIZE]/*p*/ = (t4 + t5 * 2 + t6 + 2)/4;	
}

//mode 8
void intra_pred_luma4x4_HOR_UP_PRED (int32 blkIdxInMB, 
									  uint8 *pPred, 
									  uint8 *pTopLeftPix,
									  uint8 *pLeftPix
									  )
{
	uint8 *pPix, *pPixBlk;
	uint8 l0, l1, l2, l3;

	//left block
	if ((blkIdxInMB % 4) != 0) //not the first column block
	{
		pPix = pPred - 1;

		l0 = pPix[0 * MBC_Y_SIZE];
		l1 = pPix[1 * MBC_Y_SIZE]; //next line
		l2 = pPix[2 * MBC_Y_SIZE]; //next line
		l3 = pPix[3 * MBC_Y_SIZE]; //next line
	}else
	{
		int32 avail = (g_mbc_reg_ptr->MBC_CMD0 >> 26) & 0x01; //mbAvailA

		if (avail)
		{
			pPix = pLeftPix + blkIdxInMB;

			l0 = pPix[0]; 
			l1 = pPix[1]; //next line
			l2 = pPix[2]; //next line
			l3 = pPix[3]; //next line
		}else
		{
			PRINTF ("luma4x4_HOR_UP_PRED is not allowed!\n");
			return;
		}
	}

	if(g_trace_enable_flag&TRACE_ENABLE_IPRED)	
	{
		FPRINTF(g_ipred_log_fp, "I = %d, J = %d, K = %d, L = %d\n", l0, l1, l2, l3);
	}
	pPixBlk = pPred;

	/*(x+2*y) = 0, 2, 4*/
	pPixBlk[0+0*MBC_Y_SIZE]/*a*/ = (l0 + l1 + 1)/2;
	pPixBlk[2+0*MBC_Y_SIZE]/*c*/ = pPixBlk[0+1*MBC_Y_SIZE]/*e*/ = (l1 + l2 + 1)/2;
	pPixBlk[2+1*MBC_Y_SIZE]/*g*/ = pPixBlk[0+2*MBC_Y_SIZE]/*i*/ = (l2 + l3 + 1)/2;
	
	/*(x+2*y) = 1, 3*/
	pPixBlk[1+0*MBC_Y_SIZE]/*b*/ = (l0 + l1 * 2 + l2 + 2)/4;
	pPixBlk[3+0*MBC_Y_SIZE]/*d*/ = pPixBlk[1+1*MBC_Y_SIZE]/*f*/ = (l1 + l2 * 2 + l3 + 2)/4;

	/*(x+2*y) = 5*/
	pPixBlk[3+1*MBC_Y_SIZE]/*h*/ = pPixBlk[1+2*MBC_Y_SIZE]/*j*/ = (l2 + l3 * 3 + 2)/4;

	/*(x+2*y) > 5*/
	pPixBlk[2+2*MBC_Y_SIZE]/*k*/ = l3;
	pPixBlk[3+2*MBC_Y_SIZE]/*l*/ = l3;
	pPixBlk[0+3*MBC_Y_SIZE]/*m*/ = l3;
	pPixBlk[1+3*MBC_Y_SIZE]/*n*/ = l3;
	pPixBlk[2+3*MBC_Y_SIZE]/*o*/ = l3;
	pPixBlk[3+3*MBC_Y_SIZE]/*p*/ = l3;
}

//16x16 luma intra prediction

//mode 0
void intra_pred_luma16x16_VERT_PRED(uint8 *pPred, 
									uint8 *pTopLeftPix,
									uint8 *pLeftPix)
{
	int32 i, j;
	uint8 *pPix;
	uint8 *pTopRefPix = pTopLeftPix + 1;
	int32 avail = (g_mbc_reg_ptr->MBC_CMD0 >> 25) & 0x01; //mbAvailB

	if (!avail)
	{
		PRINTF ("intra_pred_luma16x16_VERT_PRED is not allowed!\n");
		return;
	}

if(g_trace_enable_flag&TRACE_ENABLE_IPRED)	
{
	FPRINTF(g_ipred_log_fp, "the top 16 pixels:");
	for(i = 0; i< 16; i++)
	{
		FPRINTF(g_ipred_log_fp, "%d  ", pTopRefPix[i]);
	}
	FPRINTF(g_ipred_log_fp, "\n");
}
	pPix = pPred;
	for (i = 0; i < 16; i++) //16 row
	{
		for (j = 0; j < 16; j++) //16 pixel per row
		{
			pPix[j] = pTopRefPix[j];
		}
		pPix += MBC_Y_SIZE;
	}

	return;
}

//mode 1
void intra_pred_luma16x16_HOR_PRED(uint8 *pPred, 
									uint8 *pTopLeftPix,
									uint8 *pLeftPix)
{
	int32 i, j;
	uint8 *pPix;
// 	uint8 *pLeftRefPix = pLeftPix;
	int32 avail = (g_mbc_reg_ptr->MBC_CMD0 >> 26) & 0x01; //mbAvailA

	if (!avail)
	{
		PRINTF ("intra_pred_luma16x16_HOR_PRED is not allowed!\n");
		return;
	}
if(g_trace_enable_flag&TRACE_ENABLE_IPRED)	
{
	FPRINTF(g_ipred_log_fp, "the left 16 pixels:");
	for(i = 0; i< 16; i++)
	{
		FPRINTF(g_ipred_log_fp, "%d  ", pLeftPix[i]);
	}
	FPRINTF(g_ipred_log_fp, "\n");
}
	pPix = pPred;
	for (i = 0; i < 16; i++) //16 row
	{
		uint8 left_pix = pLeftPix[i];

		for (j = 0; j < 16; j++) //16 pixels per row
		{
			pPix[j] = left_pix;
		}
		pPix += MBC_Y_SIZE;
	}

	return;
}

//mode 2
void intra_pred_luma16x16_DC_PRED(uint8 *pPred, 
								  uint8 *pTopLeftPix,
								  uint8 *pLeftPix)
{
	int32 i, j;
	uint8 *pPix;
	int32 pred_sum = 0;
	int32 avail;
	int32 avail_num = 0;

	//left 
if(g_trace_enable_flag&TRACE_ENABLE_IPRED)	
{
	FPRINTF(g_ipred_log_fp, "the left 16 pixels:");
}	
	avail = (g_mbc_reg_ptr->MBC_CMD0 >> 26) & 0x01; //mbAvailA

	if (avail)
	{
		avail_num++;
		pPix = pLeftPix;

		for (i = 0; i < 16; i++)
		{
			pred_sum += pPix[i];
			if(g_trace_enable_flag&TRACE_ENABLE_IPRED)	
			{
				FPRINTF(g_ipred_log_fp, "%d  ", pPix[i]);
			}
		}	
		if(g_trace_enable_flag&TRACE_ENABLE_IPRED)	
		{
			FPRINTF(g_ipred_log_fp, "\n");
		}
	}else
	{
		if(g_trace_enable_flag&TRACE_ENABLE_IPRED)	
		{	
			FPRINTF(g_ipred_log_fp, "X X X X X X X X X X X X X X X X\n");
		}
	}

	//top
if(g_trace_enable_flag&TRACE_ENABLE_IPRED)	
{
	FPRINTF(g_ipred_log_fp, "the top 16 pixels:");
}
	avail = (g_mbc_reg_ptr->MBC_CMD0 >> 25) & 0x01; //mbAvailB

	if (avail)
	{
		avail_num++;
		pPix = pTopLeftPix + 1;

		for (i = 0; i < 16; i++)
		{
			pred_sum += pPix[i];
			if(g_trace_enable_flag&TRACE_ENABLE_IPRED)	
			{
				FPRINTF(g_ipred_log_fp, "%d  ", pPix[i]);
			}
		}
		if(g_trace_enable_flag&TRACE_ENABLE_IPRED)	
		{
			FPRINTF(g_ipred_log_fp, "\n");
		}
	}else
	{
		if(g_trace_enable_flag&TRACE_ENABLE_IPRED)	
		{
			FPRINTF(g_ipred_log_fp, "X X X X X X X X X X X X X X X X\n");
		}
	}

	if (avail_num == 2)
	{
		pred_sum = (pred_sum + 16)/32;
	}else if (avail_num == 1)
	{
		pred_sum = (pred_sum + 8)/16;
	}else
	{
		pred_sum = 128;
	}

	pPix = pPred;
	for (i = 0; i < 16; i++) //16 row
	{
		for (j = 0; j < 16; j++)//16 pixels per row
		{
			pPix[j] = pred_sum; 
		}
		pPix += MBC_Y_SIZE;
	}

	return;
}

//mode 3
void intra_pred_luma16x16_PLANE_PRED(uint8 *pPred, 
								  uint8 *pTopLeftPix,
								  uint8 *pLeftPix)
{
	int32 x, y;
	uint8 *pPix;
	int32 a, b, c;
	int32 H, V;
	uint8 *pRef;
	int32 left_avail, top_avail, tl_avail;

	left_avail = (g_mbc_reg_ptr->MBC_CMD0 >> 26) & 0x01; //mbAvailA
	top_avail  = (g_mbc_reg_ptr->MBC_CMD0 >> 25) & 0x01; //mbAvailB
	tl_avail   = (g_mbc_reg_ptr->MBC_CMD0 >> 27) & 0x01; //mbAvailD

	if (!left_avail || !top_avail || !tl_avail)
	{
		PRINTF ("intra_pred_luma16x16_PLANE_PRED is not allowed!\n");
		return;
	}

	//compute H parameter
	H = 0;
	pRef = pTopLeftPix + 1;
	for (x = 0; x < 8; x++)
	{
		H += ((x+1) * (pRef[8+x] - pRef[6-x]));
	}
if(g_trace_enable_flag&TRACE_ENABLE_IPRED)	
{
	FPRINTF(g_ipred_log_fp, "the top 16 pixels:");
	for (x = 0; x < 16; x++)
	{
		FPRINTF(g_ipred_log_fp, "%d  ", pRef[x]);
	}
	FPRINTF(g_ipred_log_fp, "\n");
}
	//compute V parameter
	V = 0;
	pRef = pLeftPix;
	for (y = 0; y < 7; y++)
	{
		V += ((y+1) * (pRef[8+y] - pRef[6-y]));
	}
	//for y = 7
	V += (7+1) * (pLeftPix[15] - pTopLeftPix[0]);
if(g_trace_enable_flag&TRACE_ENABLE_IPRED)	
{
	FPRINTF(g_ipred_log_fp, "the left 16 pixels:");
	for (y = 0; y < 16; y++)
	{
		FPRINTF(g_ipred_log_fp, "%d  ", pRef[y]);
	}
	FPRINTF(g_ipred_log_fp, "\n");
}
	//compute a,b, c parameter
	a = (pLeftPix[15] + pTopLeftPix[16]) * 16;
	b = (5*H + 32)>>6;
	c = (5*V + 32)>>6;

	//compute each positon's pixel prediction value
	pPix = pPred;
	for (y = 0; y < 16; y++)
	{
		for (x = 0; x < 16; x++)
		{
			pPix[x] = IClip(0, 255, (a + b * (x-7) + c*(y-7)+16)/32);
		}
		pPix += MBC_Y_SIZE;
	}

	return;
}

//chroma intra prediction

//chorma mode 0
void intra_pred_chroma8x8_DC_PRED(uint8 *pPred, 
							uint8 *pTopLeftPix,
							uint8 *pLeftPix)
{
	int32 left_avail, top_avail;
	uint8 *pPix, *pPixBlk;
	int32 top4_left_sum = 0;
	int32 top4_right_sum = 0;
	int32 left4_top_sum = 0;
	int32 left4_bot_sum = 0;
	int32 pred_sum;
	int32 i;

	//top
if(g_trace_enable_flag&TRACE_ENABLE_IPRED)	
{
	FPRINTF(g_ipred_log_fp, "the top 8 pixels:");
}
	top_avail = (g_mbc_reg_ptr->MBC_CMD0 >> 25) & 0x01; //mbAvailB
	if(top_avail)
	{
		pPix = pTopLeftPix + 1;

		top4_left_sum += pPix[0];
		top4_left_sum += pPix[1];
		top4_left_sum += pPix[2];
		top4_left_sum += pPix[3];

		top4_right_sum += pPix[4];
		top4_right_sum += pPix[5];
		top4_right_sum += pPix[6];
		top4_right_sum += pPix[7];

		if(g_trace_enable_flag&TRACE_ENABLE_IPRED)	
		{
			for (i = 0; i < 8; i++)
			{
				FPRINTF(g_ipred_log_fp, "%d  ", pPix[i]);
			}
			FPRINTF(g_ipred_log_fp, "\n");	
		}
	}else
	{
		if(g_trace_enable_flag&TRACE_ENABLE_IPRED)	
		{
			FPRINTF(g_ipred_log_fp, "X X X X X X X X\n");
		}
	}

	//left
if(g_trace_enable_flag&TRACE_ENABLE_IPRED)	
{
	FPRINTF(g_ipred_log_fp, "the left 8 pixels:");
}
	left_avail = (g_mbc_reg_ptr->MBC_CMD0 >> 26) & 0x01; //mbAvailA
	if (left_avail)
	{
		pPix = pLeftPix;

		left4_top_sum += pPix[0];
		left4_top_sum += pPix[1];
		left4_top_sum += pPix[2];
		left4_top_sum += pPix[3];

		left4_bot_sum += pPix[4];
		left4_bot_sum += pPix[5];
		left4_bot_sum += pPix[6];
		left4_bot_sum += pPix[7];

		if(g_trace_enable_flag&TRACE_ENABLE_IPRED)	
		{
			for (i = 0; i < 8; i++)
			{
				FPRINTF(g_ipred_log_fp, "%d  ", pPix[i]);
			}
			FPRINTF(g_ipred_log_fp, "\n");	
		}
	}else
	{
		if(g_trace_enable_flag&TRACE_ENABLE_IPRED)	
		{
			FPRINTF(g_ipred_log_fp, "X X X X X X X X\n");
		}
	}

//the first 4x4 block
	pred_sum = 0;

	if (left_avail && top_avail)
	{
		pred_sum += top4_left_sum;
		pred_sum += left4_top_sum;

		pred_sum = (pred_sum + 4)/8;
	}else if (!left_avail && !top_avail)
	{
		pred_sum = 128;
	}else
	{
		if (left_avail)
		{
			pred_sum = left4_top_sum;
		}else //top_avail
		{
			pred_sum = top4_left_sum;
		}
		
		pred_sum = (pred_sum + 2)/4;
	}

	pPixBlk = pPred;
	
	for (i = 0; i < 4; i++)
	{
		pPixBlk[0] = pred_sum;
		pPixBlk[1] = pred_sum;
		pPixBlk[2] = pred_sum;
		pPixBlk[3] = pred_sum;

		pPixBlk += MBC_C_SIZE; //next line
	}

	//the 2th 4x4 block
	pred_sum = 0;
	if (top_avail)
	{
		pred_sum = top4_right_sum;
		
		pred_sum = (pred_sum + 2)/4;
	}else if (left_avail)
	{
		pred_sum = left4_top_sum;

		pred_sum = (pred_sum + 2)/4;
	}else
	{
		pred_sum = 128;
	}

	pPixBlk = pPred +4;	
	for (i = 0; i < 4; i++)
	{
		pPixBlk[0] = pred_sum;
		pPixBlk[1] = pred_sum;
		pPixBlk[2] = pred_sum;
		pPixBlk[3] = pred_sum;

		pPixBlk += MBC_C_SIZE; //next line
	}

//the 3th 4x4 block
	pred_sum = 0;
	if(left_avail)
	{
		pred_sum = (left4_bot_sum + 2)/4;
	}else if (top_avail)
	{
		pred_sum = (top4_left_sum + 2)/4;
	}else
	{
		pred_sum = 128;
	}

	pPixBlk = pPred +4*MBC_C_SIZE;	
	for (i = 0; i < 4; i++)
	{
		pPixBlk[0] = pred_sum;
		pPixBlk[1] = pred_sum;
		pPixBlk[2] = pred_sum;
		pPixBlk[3] = pred_sum;

		pPixBlk += MBC_C_SIZE; //next line
	}

//the 4th 4x4 block
	pred_sum = 0;

	if (top_avail)
	{
		pred_sum += top4_right_sum;
	}

	if (left_avail)
	{
		pred_sum += left4_bot_sum;
	}

	if (top_avail && left_avail)
	{
		pred_sum = (pred_sum + 4)/8;
	}else if (!top_avail && !left_avail)
	{
		pred_sum = 128;
	}else
	{
		pred_sum = (pred_sum + 2)/4;
	}

	pPixBlk = pPred +4*MBC_C_SIZE+4;	
	for (i = 0; i < 4; i++)
	{
		pPixBlk[0] = pred_sum;
		pPixBlk[1] = pred_sum;
		pPixBlk[2] = pred_sum;
		pPixBlk[3] = pred_sum;

		pPixBlk += MBC_C_SIZE; //next line
	}

	return;
}

//mode 1
void intra_pred_chroma8x8_HOR_PRED(uint8 *pPred, 
									uint8 *pTopLeftPix,
									uint8 *pLeftPix)
{
	int32 i,j;
	uint8 *pPix;
// 	uint8 *pLeftRefPix = pLeftPix;
	int32 avail = (g_mbc_reg_ptr->MBC_CMD0 >> 26) & 0x01; //mbAvailA

	if (!avail)
	{
		PRINTF ("intra_pred_chroma_HOR_PRED is not allowed!\n");
		return;
	}
if(g_trace_enable_flag&TRACE_ENABLE_IPRED)	
{
	FPRINTF(g_ipred_log_fp, "the left 8 pixels:");
	for(i = 0; i< 8; i++)
	{
		FPRINTF(g_ipred_log_fp, "%d  ", pLeftPix[i]);
	}
	FPRINTF(g_ipred_log_fp, "\n");
}
	pPix = pPred;
	for (i = 0; i < 8; i++) //8 row
	{
		uint8 left_pix = pLeftPix[i];

		for (j = 0; j < 8; j++) //8 pixels per row
		{
			pPix[j] = left_pix;
		}
		pPix += MBC_C_SIZE;
	}

	return;
}

//mode 2
void intra_pred_chroma8x8_VERT_PRED(uint8 *pPred, 
								uint8 *pTopLeftPix,
								uint8 *pLeftPix)
{
	int32 i, j;
	uint8 *pPix;
	uint8 *pTopRefPix = pTopLeftPix + 1;
	int32 avail = (g_mbc_reg_ptr->MBC_CMD0 >> 25) & 0x01; //mbAvailB

	if (!avail)
	{
		PRINTF ("intra_pred_chroma_VERT_PRED is not allowed!\n");
		return;
	}
if(g_trace_enable_flag&TRACE_ENABLE_IPRED)	
{
	FPRINTF(g_ipred_log_fp, "the top 8 pixels:");
	for(i = 0; i< 8; i++)
	{
		FPRINTF(g_ipred_log_fp, "%d  ", pTopRefPix[i]);
	}
}
	pPix = pPred;
	for (i = 0; i < 8; i++) //8 row
	{
		for (j = 0; j < 8; j++) //8 pixels per row
		{
			pPix[j] = pTopRefPix[j];
		}
		pPix += MBC_C_SIZE;
	}

	return;
}

//mode 3
void intra_pred_chroma8x8_PLANE_PRED(uint8 *pPred, 
								  uint8 *pTopLeftPix,
								  uint8 *pLeftPix)
{
	int32 x, y;
	int32 a, b, c;
	int32 H, V;
	uint8 *pRef;
	int32 left_avail, top_avail, tl_avail;

	left_avail = (g_mbc_reg_ptr->MBC_CMD0 >> 26) & 0x01; //mbAvailA
	top_avail  = (g_mbc_reg_ptr->MBC_CMD0 >> 25) & 0x01; //mbAvailB
	tl_avail   = (g_mbc_reg_ptr->MBC_CMD0 >> 27) & 0x01; //mbAvailD

	if (!left_avail || !top_avail || !tl_avail)
	{
		PRINTF ("intra_pred_chroma8x8_PLANE_PRED is not allowed!\n");
		return;
	}

	//compute H parameter
	H = 0;
	pRef = pTopLeftPix + 1;
	for (x = 0; x < 4; x++)
	{
		H += ((x+1) * (pRef[4+x] - pRef[2-x]));
	}
if(g_trace_enable_flag&TRACE_ENABLE_IPRED)	
{
	FPRINTF(g_ipred_log_fp, "the top 8 pixels:");
	for (x = 0; x < 8; x++)
	{
		FPRINTF(g_ipred_log_fp, "%d  ", pRef[x]);
	}
	FPRINTF(g_ipred_log_fp, "\n");
}
	//compute V parameter
	V = 0;
	pRef = pLeftPix;
	for (y = 0; y < 3; y++)
	{
		V += ((y+1) * (pRef[4+y] - pRef[2-y]));
	}
	//for y = 3
	V += (3+1) * (pLeftPix[7] - pTopLeftPix[0]);
if(g_trace_enable_flag&TRACE_ENABLE_IPRED)	
{
	FPRINTF(g_ipred_log_fp, "the left 8 pixels:");
	for (y = 0; y < 8; y++)
	{
		FPRINTF(g_ipred_log_fp, "%d  ", pRef[y]);
	}
	FPRINTF(g_ipred_log_fp, "\n");
}
	//compute a,b, c parameter
	a = (pLeftPix[7] + pTopLeftPix[8]) * 16;
	b = (17*H + 16)>>5;
	c = (17*V + 16)>>5;

	//compute each positon's pixel prediction value
	for (y = 0; y < 8; y++)
	{
		for (x = 0; x < 8; x++)
		{
			pPred[y*MBC_C_SIZE + x] = IClip(0, 255, (a + b * (x-3) + c*(y-3)+16)>>5);
		}
	}

	return;
}