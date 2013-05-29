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
#define Little_endian

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
	fprintf(g_ipred_log_fp, "A = %d, B = %d, C = %d, D = %d\n", pPix[0], pPix[1], pPix[2], pPix[3]);
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
	fprintf(g_ipred_log_fp, "I = %d, J = %d, K = %d, L = %d\n", pixVal[0], pixVal[1], pixVal[2], pixVal[3]);
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
		fprintf(g_ipred_log_fp, "I = %d, J = %d, K = %d, L = %d\n", 
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
				fprintf(g_ipred_log_fp, "I = %d, J = %d, K = %d, L = %d\n", pPix[0], pPix[1], pPix[2], pPix[3]);
			}
		}else
		{
			if(g_trace_enable_flag&TRACE_ENABLE_IPRED)	
			{
				fprintf(g_ipred_log_fp, "I = X, J = X, K = X, L = X\n");
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
			fprintf(g_ipred_log_fp, "A = %d, B = %d, C = %d, D = %d\n", pPix[0], pPix[1], pPix[2], pPix[3]);
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
				fprintf(g_ipred_log_fp, "A = %d, B = %d, C = %d, D = %d\n", pPix[0], pPix[1], pPix[2], pPix[3]);
			}
		}else
		{
			if(g_trace_enable_flag&TRACE_ENABLE_IPRED)	
			{
				fprintf(g_ipred_log_fp, "A = X, B = X, C = X, D = X\n");
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
		fprintf(g_ipred_log_fp, "A = %d, B = %d, C = %d, D = %d, E = %d, F = %d, G = %d, H = %d\n", t0, t1, t2, t3, t4, t5, t6, t7);
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
		fprintf(g_ipred_log_fp, "M = %d\n", tn1);
		fprintf(g_ipred_log_fp, "A = %d, B = %d, C = %d, D = %d\n", t0, t1, t2, t3);
		fprintf(g_ipred_log_fp, "I = %d, J = %d, K = %d, L = %d\n", l0, l1, l2, l3);
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
	fprintf(g_ipred_log_fp, "M = %d\n", tn1);
	fprintf(g_ipred_log_fp, "A = %d, B = %d, C = %d, D = %d\n", t0, t1, t2, t3);
	fprintf(g_ipred_log_fp, "I = %d, J = %d, K = %d\n", l0, l1, l2);
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
		fprintf(g_ipred_log_fp, "M = %d\n", tn1);
		fprintf(g_ipred_log_fp, "A = %d, B = %d, C = %d\n", t0, t1, t2);
		fprintf(g_ipred_log_fp, "I = %d, J = %d, K = %d, L = %d\n", l0, l1, l2, l3);
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
		fprintf(g_ipred_log_fp, "A = %d, B = %d, C = %d, D = %d, E = %d, F = %d, G = %d\n", t0, t1, t2, t3, t4, t5, t6);
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
		fprintf(g_ipred_log_fp, "I = %d, J = %d, K = %d, L = %d\n", l0, l1, l2, l3);
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


//8x8 luma intra prediction //weihu

void get_8x8_nei_pixels (//int32 blkIdxInMB,
						 int avail_left,
						 int avail_top,
						 int avail_topleft,
					 //uint8 *pPred, 
					 uint8 *pTopLeftPix,
					 uint8 *pLeftPix,
					 uint8 left[], 
					 uint8 top[])
{
	//uint8 *pPixTop, *pPixLeft;
	int i;
	
	if (avail_top)
	{
		if(avail_topleft)
             top[1]=(pTopLeftPix[0]+2*pTopLeftPix[1]+pTopLeftPix[2]+2)>>2;
		else
			 top[1]=(3*pTopLeftPix[1]+pTopLeftPix[2]+2)>>2;
        top[16]=(pTopLeftPix[15]+3*pTopLeftPix[16]+2)>>2;
		for(i=2;i<16;i++)
			top[i]=(pTopLeftPix[i-1]+2*pTopLeftPix[i]+pTopLeftPix[i+1]+2)>>2;

	}
	
	if(avail_topleft)
	{
         if(avail_top&&avail_left)
			 left[0] = top[0]=(pLeftPix[0]+2*pTopLeftPix[0]+pTopLeftPix[1]+2)>>2;
		 else if(avail_top)
			 left[0] = top[0]=(3*pTopLeftPix[0]+pTopLeftPix[1]+2)>>2;
		 else if(avail_left)
			 left[0] = top[0]=(pLeftPix[0]+3*pTopLeftPix[0]+2)>>2;
		 else 
			 left[0] = top[0]=pTopLeftPix[0];
	}
	
	if (avail_left)
	{
		if(avail_topleft)
			left[1]=(pTopLeftPix[0]+2*pLeftPix[0]+pLeftPix[1]+2)>>2;
		else
			left[1]=(3*pLeftPix[0]+pLeftPix[1]+2)>>2;
        left[8]=(pLeftPix[7]+3*pLeftPix[8]+2)>>2;
		for(i=2;i<8;i++)
			left[i]=(pLeftPix[i-1]+2*pLeftPix[i]+pLeftPix[i+1]+2)>>2;
		
	}	

	return;
}


//mode 0
void intra_pred_luma8x8_VERT_PRED (int32 blkIdxInMB, 
								  uint8 *pPred, 
								  uint8 *pTopLeftPix,
								  uint8 *pLeftPix
								  )
{

	int i;
	uint8 *pPixBlk;

	uint8 left[9], top[17];
	int avail_left,avail_top,avail_topleft;
	if (blkIdxInMB<2) 
	      avail_top = (g_mbc_reg_ptr->MBC_CMD0 >> 25) & 0x01; //mbAvailB
	else
		  avail_top =1;
	if((blkIdxInMB&0x1)==0)
	      avail_left = (g_mbc_reg_ptr->MBC_CMD0 >> 26) & 0x01; //mbAvailA
	else
		  avail_left = 1;
	if(blkIdxInMB==0)
          avail_topleft = (g_mbc_reg_ptr->MBC_CMD0 >> 27) & 0x01; //mbAvailD
	else if(blkIdxInMB==1)
          avail_topleft = (g_mbc_reg_ptr->MBC_CMD0 >> 25) & 0x01; //mbAvailB
	else if(blkIdxInMB==2)
		  avail_topleft = (g_mbc_reg_ptr->MBC_CMD0 >> 26) & 0x01; //mbAvailA
	else
          avail_topleft =1;
	/*compute left and top respectively*/
	get_8x8_nei_pixels (avail_left,avail_top,avail_topleft, pTopLeftPix, pLeftPix, left, top);

    pPixBlk = pPred;

	if(g_trace_enable_flag&TRACE_ENABLE_IPRED)	
	{
//		fprintf(g_ipred_log_fp, "A = %d, B = %d, C = %d, D = %d\n", pPix[0], pPix[1], pPix[2], pPix[3]);
	}
	
	for (i = 0; i < 8; i++) //8 line
	{
		pPixBlk[0] = top[1]; //pixel 0
		pPixBlk[1] = top[2]; //pixel 1
		pPixBlk[2] = top[3]; //pixel 2
		pPixBlk[3] = top[4]; //pixel 3
		pPixBlk[4] = top[5]; //pixel 4
		pPixBlk[5] = top[6]; //pixel 5
		pPixBlk[6] = top[7]; //pixel 6
		pPixBlk[7] = top[8]; //pixel 7
		
		//next line
		pPixBlk += MBC_Y_SIZE;
	}
	return;
}

//mode 1
void intra_pred_luma8x8_HOR_PRED (int32 blkIdxInMB, 
								  uint8 *pPred, 
								  uint8 *pTopLeftPix,
								  uint8 *pLeftPix
								  )
{
	int32 i;
	uint8 *pPixBlk;

	uint8 left[9], top[17];
	int avail_left,avail_top,avail_topleft;
	
	if (blkIdxInMB<2) 
		avail_top = (g_mbc_reg_ptr->MBC_CMD0 >> 25) & 0x01; //mbAvailB
	else
		avail_top =1;
	if((blkIdxInMB&0x1)==0)
		avail_left = (g_mbc_reg_ptr->MBC_CMD0 >> 26) & 0x01; //mbAvailA
	else
		avail_left = 1;
	if(blkIdxInMB==0)
		avail_topleft = (g_mbc_reg_ptr->MBC_CMD0 >> 27) & 0x01; //mbAvailD
	else if(blkIdxInMB==1)
		avail_topleft = (g_mbc_reg_ptr->MBC_CMD0 >> 25) & 0x01; //mbAvailB
	else if(blkIdxInMB==2)
		avail_topleft = (g_mbc_reg_ptr->MBC_CMD0 >> 26) & 0x01; //mbAvailA
	else
		avail_topleft =1;
	/*compute left and top respectively*/
	get_8x8_nei_pixels (avail_left,avail_top,avail_topleft, pTopLeftPix, pLeftPix, left, top);
	
    pPixBlk = pPred;

	if(g_trace_enable_flag&TRACE_ENABLE_IPRED)	
	{
//		fprintf(g_ipred_log_fp, "I = %d, J = %d, K = %d, L = %d\n", pixVal[0], pixVal[1], pixVal[2], pixVal[3]);
	}

    for (i = 1; i < 9; i++) //8 line
	{
		pPixBlk[0] = left[i]; //pixel 0
		pPixBlk[1] = left[i]; //pixel 1
		pPixBlk[2] = left[i]; //pixel 2
		pPixBlk[3] = left[i]; //pixel 3
		pPixBlk[4] = left[i]; //pixel 4
		pPixBlk[5] = left[i]; //pixel 5
		pPixBlk[6] = left[i]; //pixel 6
		pPixBlk[7] = left[i]; //pixel 7
		
		//next line
		pPixBlk += MBC_Y_SIZE;
	}
	return;
}

//mode 2
void intra_pred_luma8x8_DC_PRED (int32 blkIdxInMB, 
								  uint8 *pPred, 
								  uint8 *pTopLeftPix,
								  uint8 *pLeftPix
								  )
{
	int32 i;
//	int32 avail;
	int32 pred_sum = 0;
	uint8 *pPixBlk;
	int32 blk_avail_num = 0;

    uint8 left[9], top[17];
	int avail_left,avail_top,avail_topleft;
	
	if (blkIdxInMB<2) 
		avail_top = (g_mbc_reg_ptr->MBC_CMD0 >> 25) & 0x01; //mbAvailB
	else
		avail_top =1;
	if((blkIdxInMB&0x1)==0)
		avail_left = (g_mbc_reg_ptr->MBC_CMD0 >> 26) & 0x01; //mbAvailA
	else
		avail_left = 1;
	if(blkIdxInMB==0)
		avail_topleft = (g_mbc_reg_ptr->MBC_CMD0 >> 27) & 0x01; //mbAvailD
	else if(blkIdxInMB==1)
		avail_topleft = (g_mbc_reg_ptr->MBC_CMD0 >> 25) & 0x01; //mbAvailB
	else if(blkIdxInMB==2)
		avail_topleft = (g_mbc_reg_ptr->MBC_CMD0 >> 26) & 0x01; //mbAvailA
	else
		avail_topleft =1;
	/*compute left and top respectively*/
	get_8x8_nei_pixels (avail_left,avail_top,avail_topleft, pTopLeftPix, pLeftPix, left, top);
	
    pPixBlk = pPred;

	//left block
    if (avail_left)
	{
		blk_avail_num++;
		
		for(i=1;i<9;i++)
		    pred_sum += left[i]; //next line
	}

	//top block
    if (avail_left)
	{
		blk_avail_num++;
		
		for(i=1;i<9;i++)
			pred_sum += top[i]; //next line
	}

    if (blk_avail_num == 2)
	{
		pred_sum = (pred_sum + 8) >> 4;
	}else if (blk_avail_num == 0)
	{
		pred_sum = 128;
	}else
	{
		pred_sum = (pred_sum + 4) >> 3;
	}

	if(g_trace_enable_flag&TRACE_ENABLE_IPRED)	
	{
//		fprintf(g_ipred_log_fp, "A = %d, B = %d, C = %d, D = %d\n", pPix[0], pPix[1], pPix[2], pPix[3]);
	}
  
	for (i = 0; i < 8; i++) //8 line
	{
		pPixBlk[0] = pred_sum; //pixel 0
		pPixBlk[1] = pred_sum; //pixel 1
		pPixBlk[2] = pred_sum; //pixel 2
		pPixBlk[3] = pred_sum; //pixel 3
		pPixBlk[4] = pred_sum; //pixel 4
		pPixBlk[5] = pred_sum; //pixel 5
		pPixBlk[6] = pred_sum; //pixel 6
		pPixBlk[7] = pred_sum; //pixel 7
		
		//next line
		pPixBlk += MBC_Y_SIZE;
	}


	return;
}
/*
int32 s_blkC_avai_tbl[16] = 
{
    1, 1, 1, 1,
	1, 0, 1, 0,
	1, 1, 1, 0, 
	1, 0, 1, 0, 
};*/

//mode 3
void intra_pred_luma8x8_DIAG_DOWN_LEFT_PRED (int32 blkIdxInMB, 
											  uint8 *pPred, 
											  uint8 *pTopLeftPix,
											  uint8 *pLeftPix
											  )
{
	//int32 blkC_avail;
	int i,j;
//	uint8 *pPix;
	uint8 *pPixBlk;
	//uint8 t0, t1, t2, t3, t4, t5, t6, t7;

	uint8 left[9], top[17];
	int avail_left,avail_top,avail_topleft;
	
	if (blkIdxInMB<2) 
		avail_top = (g_mbc_reg_ptr->MBC_CMD0 >> 25) & 0x01; //mbAvailB
	else
		avail_top =1;
	if((blkIdxInMB&0x1)==0)
		avail_left = (g_mbc_reg_ptr->MBC_CMD0 >> 26) & 0x01; //mbAvailA
	else
		avail_left = 1;
	if(blkIdxInMB==0)
		avail_topleft = (g_mbc_reg_ptr->MBC_CMD0 >> 27) & 0x01; //mbAvailD
	else if(blkIdxInMB==1)
		avail_topleft = (g_mbc_reg_ptr->MBC_CMD0 >> 25) & 0x01; //mbAvailB
	else if(blkIdxInMB==2)
		avail_topleft = (g_mbc_reg_ptr->MBC_CMD0 >> 26) & 0x01; //mbAvailA
	else
		avail_topleft =1;
	/*compute left and top respectively*/
	get_8x8_nei_pixels (avail_left,avail_top,avail_topleft, pTopLeftPix, pLeftPix, left, top);
	
    pPixBlk = pPred;	

	for (i = 0; i < 8; i++) //8 line
	{
		for (j = 0; j < 8; i++) //8 column
		{
            if((i==7)&&(j==7))
                 pPixBlk[i*MBC_Y_SIZE+j] = (top[15] + top[16] * 3 + 2) >> 2;
			else
				 pPixBlk[i*MBC_Y_SIZE+j] = (top[i+j+1] + top[i+j+2] * 2 + top[i+j+3] + 2) >> 2;
		}
	}

	return;
}

//mode 4
void intra_pred_luma8x8_DIAG_DOWN_RIGHT_PRED (int32 blkIdxInMB, 
											  uint8 *pPred, 
											  uint8 *pTopLeftPix,
											  uint8 *pLeftPix
											  )
{
	uint8 *pPixBlk;
	//uint8 tn1, t0, t1, t2, t3;
	//uint8 ln1, l0, l1, l2, l3;

	int i,j;
	uint8 left[9], top[17];
	int avail_left,avail_top,avail_topleft;
	
	if (blkIdxInMB<2) 
		avail_top = (g_mbc_reg_ptr->MBC_CMD0 >> 25) & 0x01; //mbAvailB
	else
		avail_top =1;
	if((blkIdxInMB&0x1)==0)
		avail_left = (g_mbc_reg_ptr->MBC_CMD0 >> 26) & 0x01; //mbAvailA
	else
		avail_left = 1;
	if(blkIdxInMB==0)
		avail_topleft = (g_mbc_reg_ptr->MBC_CMD0 >> 27) & 0x01; //mbAvailD
	else if(blkIdxInMB==1)
		avail_topleft = (g_mbc_reg_ptr->MBC_CMD0 >> 25) & 0x01; //mbAvailB
	else if(blkIdxInMB==2)
		avail_topleft = (g_mbc_reg_ptr->MBC_CMD0 >> 26) & 0x01; //mbAvailA
	else
		avail_topleft =1;
	/*compute left and top respectively*/
	get_8x8_nei_pixels (avail_left,avail_top,avail_topleft, pTopLeftPix, pLeftPix, left, top);
	
    pPixBlk = pPred;	

    for (i = 0; i < 8; i++) //8 line
	{
		for (j = 0; j < 8; i++) //8 column
		{
            if(j>i)
				pPixBlk[i*MBC_Y_SIZE+j] = (top[j-i-1] + top[j-i] * 2 + top[j-i+1] + 2) >> 2;
			else if(j<i)
				pPixBlk[i*MBC_Y_SIZE+j] = (left[i-j-1] + left[i-j] * 2 + left[i-j+1] + 2) >> 2;
			else
				pPixBlk[i*MBC_Y_SIZE+j] = (left[1] + left[0] * 2 + top[1] + 2) >> 2;
		}
	}

	return;
}

//mode 5
void intra_pred_luma8x8_VERT_RIGHT_PRED (int32 blkIdxInMB, 
											  uint8 *pPred, 
											  uint8 *pTopLeftPix,
											  uint8 *pLeftPix
											  )
{
	uint8 *pPixBlk;
	//uint8 tn1, t0, t1, t2, t3;
	//uint8 l0, l1, l2;

    int i,j;//y,x
	uint8 left[9], top[17];
	int avail_left,avail_top,avail_topleft;
	int zvr,zvrd2;
	
	if (blkIdxInMB<2) 
		avail_top = (g_mbc_reg_ptr->MBC_CMD0 >> 25) & 0x01; //mbAvailB
	else
		avail_top =1;
	if((blkIdxInMB&0x1)==0)
		avail_left = (g_mbc_reg_ptr->MBC_CMD0 >> 26) & 0x01; //mbAvailA
	else
		avail_left = 1;
	if(blkIdxInMB==0)
		avail_topleft = (g_mbc_reg_ptr->MBC_CMD0 >> 27) & 0x01; //mbAvailD
	else if(blkIdxInMB==1)
		avail_topleft = (g_mbc_reg_ptr->MBC_CMD0 >> 25) & 0x01; //mbAvailB
	else if(blkIdxInMB==2)
		avail_topleft = (g_mbc_reg_ptr->MBC_CMD0 >> 26) & 0x01; //mbAvailA
	else
		avail_topleft =1;
	/*compute left and top respectively*/
	get_8x8_nei_pixels (avail_left,avail_top,avail_topleft, pTopLeftPix, pLeftPix, left, top);
	
    pPixBlk = pPred;
	
		
    for (i = 0; i < 8; i++) //8 line
	{
		for (j = 0; j < 8; i++) //8 column
		{
			zvr=2*j-i;
			zvrd2=j-(i>>1);
            if(zvr==-1)
				pPixBlk[i*MBC_Y_SIZE+j] = (left[1] + left[0] * 2 + top[1] + 2) >> 2;
			else if(zvr<0)//-2 to -7
				pPixBlk[i*MBC_Y_SIZE+j] = (left[-zvr] + left[-zvr-1] * 2 + left[-zvr-2] + 2) >> 2;
			else if(zvr&0x1)//1,3,5,7,9,11,13
				pPixBlk[i*MBC_Y_SIZE+j] = (top[zvrd2-1] + top[zvrd2] * 2 + top[zvrd2+1] + 2) >> 2;
			else//0,2,4,6,8,10,12,14
				pPixBlk[i*MBC_Y_SIZE+j] = (top[zvrd2]  + top[zvrd2+1] + 1) >> 1;
		}
	}


if(g_trace_enable_flag&TRACE_ENABLE_IPRED)	
{
	//fprintf(g_ipred_log_fp, "M = %d\n", tn1);
	//fprintf(g_ipred_log_fp, "A = %d, B = %d, C = %d, D = %d\n", t0, t1, t2, t3);
	//fprintf(g_ipred_log_fp, "I = %d, J = %d, K = %d\n", l0, l1, l2);
}

    return;
}

//mode 6
void intra_pred_luma8x8_HOR_DOWN_PRED (int32 blkIdxInMB, 
									  uint8 *pPred, 
									  uint8 *pTopLeftPix,
									  uint8 *pLeftPix
									  )
{
	uint8 *pPixBlk;
	//uint8 tn1, t0, t1, t2;
	//uint8 ln1, l0, l1, l2, l3;

	int i,j;//y,x
	uint8 left[9], top[17];
	int avail_left,avail_top,avail_topleft;
	int zhd,zhdd2;
	
	if (blkIdxInMB<2) 
		avail_top = (g_mbc_reg_ptr->MBC_CMD0 >> 25) & 0x01; //mbAvailB
	else
		avail_top =1;
	if((blkIdxInMB&0x1)==0)
		avail_left = (g_mbc_reg_ptr->MBC_CMD0 >> 26) & 0x01; //mbAvailA
	else
		avail_left = 1;
	if(blkIdxInMB==0)
		avail_topleft = (g_mbc_reg_ptr->MBC_CMD0 >> 27) & 0x01; //mbAvailD
	else if(blkIdxInMB==1)
		avail_topleft = (g_mbc_reg_ptr->MBC_CMD0 >> 25) & 0x01; //mbAvailB
	else if(blkIdxInMB==2)
		avail_topleft = (g_mbc_reg_ptr->MBC_CMD0 >> 26) & 0x01; //mbAvailA
	else
		avail_topleft =1;
	/*compute left and top respectively*/
	get_8x8_nei_pixels (avail_left,avail_top,avail_topleft, pTopLeftPix, pLeftPix, left, top);
	
    pPixBlk = pPred;
	
	
    for (i = 0; i < 8; i++) //8 line
	{
		for (j = 0; j < 8; i++) //8 column
		{
			zhd=2*i-j;
			zhdd2=i-(j>>1);
            if(zhd==-1)
				pPixBlk[i*MBC_Y_SIZE+j] = (left[1] + left[0] * 2 + top[1] + 2) >> 2;
			else if(zhd<0)//-2 to -7
				pPixBlk[i*MBC_Y_SIZE+j] = (top[-zhd] + top[-zhd-1] * 2 + top[-zhd-2] + 2) >> 2;
			else if(zhd&0x1)//1,3,5,7,9,11,13
				pPixBlk[i*MBC_Y_SIZE+j] = (left[zhdd2-1] + left[zhdd2] * 2 + left[zhdd2+1] + 2) >> 2;
			else//0,2,4,6,8,10,12,14
				pPixBlk[i*MBC_Y_SIZE+j] = (left[zhdd2]  + left[zhdd2+1] + 1) >> 1;
		}
	}


	if(g_trace_enable_flag&TRACE_ENABLE_IPRED)	
	{
		//fprintf(g_ipred_log_fp, "M = %d\n", tn1);
		//fprintf(g_ipred_log_fp, "A = %d, B = %d, C = %d\n", t0, t1, t2);
		//fprintf(g_ipred_log_fp, "I = %d, J = %d, K = %d, L = %d\n", l0, l1, l2, l3);
	}
	return;

}

//mode 7
void intra_pred_luma8x8_VERT_LEFT_PRED (int32 blkIdxInMB, 
									  uint8 *pPred, 
									  uint8 *pTopLeftPix,
									  uint8 *pLeftPix
									  )
{
	//int32 blkC_avail;
	uint8 *pPixBlk;
	//uint8 t0, t1, t2, t3, t4, t5, t6;

	int i,j;//y,x
	uint8 left[9], top[17];
	int avail_left,avail_top,avail_topleft;
	int zvld2;
	
	if (blkIdxInMB<2) 
		avail_top = (g_mbc_reg_ptr->MBC_CMD0 >> 25) & 0x01; //mbAvailB
	else
		avail_top =1;
	if((blkIdxInMB&0x1)==0)
		avail_left = (g_mbc_reg_ptr->MBC_CMD0 >> 26) & 0x01; //mbAvailA
	else
		avail_left = 1;
	if(blkIdxInMB==0)
		avail_topleft = (g_mbc_reg_ptr->MBC_CMD0 >> 27) & 0x01; //mbAvailD
	else if(blkIdxInMB==1)
		avail_topleft = (g_mbc_reg_ptr->MBC_CMD0 >> 25) & 0x01; //mbAvailB
	else if(blkIdxInMB==2)
		avail_topleft = (g_mbc_reg_ptr->MBC_CMD0 >> 26) & 0x01; //mbAvailA
	else
		avail_topleft =1;
	/*compute left and top respectively*/
	get_8x8_nei_pixels (avail_left,avail_top,avail_topleft, pTopLeftPix, pLeftPix, left, top);
	
    pPixBlk = pPred;
	
	
    for (i = 0; i < 8; i++) //8 line
	{
		for (j = 0; j < 8; i++) //8 column
		{
			zvld2=j+(i>>1);
            if(i&0x1)//1,3,5,7
				pPixBlk[i*MBC_Y_SIZE+j] = (top[zvld2+1] + top[zvld2+2] * 2 + top[zvld2+3] + 2) >> 2;
			else//0,2,4,6
				pPixBlk[i*MBC_Y_SIZE+j] = (top[zvld2+1]  + top[zvld2+2] + 1) >> 1;
		}
	}


	if(g_trace_enable_flag&TRACE_ENABLE_IPRED)	
	{
	//	fprintf(g_ipred_log_fp, "A = %d, B = %d, C = %d, D = %d, E = %d, F = %d, G = %d\n", t0, t1, t2, t3, t4, t5, t6);
	}

    return;	
}

//mode 8
void intra_pred_luma8x8_HOR_UP_PRED (int32 blkIdxInMB, 
									  uint8 *pPred, 
									  uint8 *pTopLeftPix,
									  uint8 *pLeftPix
									  )
{
	uint8 *pPixBlk;
	//uint8 l0, l1, l2, l3;

	int i,j;//y,x
	uint8 left[9], top[17];
	int avail_left,avail_top,avail_topleft;
	int zhu,zhud2;
	
	if (blkIdxInMB<2) 
		avail_top = (g_mbc_reg_ptr->MBC_CMD0 >> 25) & 0x01; //mbAvailB
	else
		avail_top =1;
	if((blkIdxInMB&0x1)==0)
		avail_left = (g_mbc_reg_ptr->MBC_CMD0 >> 26) & 0x01; //mbAvailA
	else
		avail_left = 1;
	if(blkIdxInMB==0)
		avail_topleft = (g_mbc_reg_ptr->MBC_CMD0 >> 27) & 0x01; //mbAvailD
	else if(blkIdxInMB==1)
		avail_topleft = (g_mbc_reg_ptr->MBC_CMD0 >> 25) & 0x01; //mbAvailB
	else if(blkIdxInMB==2)
		avail_topleft = (g_mbc_reg_ptr->MBC_CMD0 >> 26) & 0x01; //mbAvailA
	else
		avail_topleft =1;
	/*compute left and top respectively*/
	get_8x8_nei_pixels (avail_left,avail_top,avail_topleft, pTopLeftPix, pLeftPix, left, top);
	
    pPixBlk = pPred;
	
	
    for (i = 0; i < 8; i++) //8 line
	{
		for (j = 0; j < 8; i++) //8 column
		{
			zhu=2*i+j;
			zhud2=i+(j>>1);
            if(zhu==13)
				pPixBlk[i*MBC_Y_SIZE+j] = (left[7] + left[8] * 3 + 2) >> 2;
			else if(zhu>13)
				pPixBlk[i*MBC_Y_SIZE+j] = left[8];
			else if(zhu&0x1)//1,3,5,7,9,11
				pPixBlk[i*MBC_Y_SIZE+j] = (left[zhud2+1] + left[zhud2+2] * 2 + left[zhud2+3] + 2) >> 2;
			else//0,2,4,6,8,10,12
				pPixBlk[i*MBC_Y_SIZE+j] = (left[zhud2+1]  + left[zhud2+2] + 1) >> 1;
		}
	}
	


	if(g_trace_enable_flag&TRACE_ENABLE_IPRED)	
	{
	//	fprintf(g_ipred_log_fp, "I = %d, J = %d, K = %d, L = %d\n", l0, l1, l2, l3);
	}

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
	fprintf(g_ipred_log_fp, "the top 16 pixels:");
	for(i = 0; i< 16; i++)
	{
		fprintf(g_ipred_log_fp, "%d  ", pTopRefPix[i]);
	}
	fprintf(g_ipred_log_fp, "\n");
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
	fprintf(g_ipred_log_fp, "the left 16 pixels:");
	for(i = 0; i< 16; i++)
	{
		fprintf(g_ipred_log_fp, "%d  ", pLeftPix[i]);
	}
	fprintf(g_ipred_log_fp, "\n");
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
	fprintf(g_ipred_log_fp, "the left 16 pixels:");
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
				fprintf(g_ipred_log_fp, "%d  ", pPix[i]);
			}
		}	
		if(g_trace_enable_flag&TRACE_ENABLE_IPRED)	
		{
			fprintf(g_ipred_log_fp, "\n");
		}
	}else
	{
		if(g_trace_enable_flag&TRACE_ENABLE_IPRED)	
		{	
			fprintf(g_ipred_log_fp, "X X X X X X X X X X X X X X X X\n");
		}
	}

	//top
if(g_trace_enable_flag&TRACE_ENABLE_IPRED)	
{
	fprintf(g_ipred_log_fp, "the top 16 pixels:");
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
				fprintf(g_ipred_log_fp, "%d  ", pPix[i]);
			}
		}
		if(g_trace_enable_flag&TRACE_ENABLE_IPRED)	
		{
			fprintf(g_ipred_log_fp, "\n");
		}
	}else
	{
		if(g_trace_enable_flag&TRACE_ENABLE_IPRED)	
		{
			fprintf(g_ipred_log_fp, "X X X X X X X X X X X X X X X X\n");
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
	fprintf(g_ipred_log_fp, "the top 16 pixels:");
	for (x = 0; x < 16; x++)
	{
		fprintf(g_ipred_log_fp, "%d  ", pRef[x]);
	}
	fprintf(g_ipred_log_fp, "\n");
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
	fprintf(g_ipred_log_fp, "the left 16 pixels:");
	for (y = 0; y < 16; y++)
	{
		fprintf(g_ipred_log_fp, "%d  ", pRef[y]);
	}
	fprintf(g_ipred_log_fp, "\n");
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
	fprintf(g_ipred_log_fp, "the top 8 pixels:");
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
				fprintf(g_ipred_log_fp, "%d  ", pPix[i]);
			}
			fprintf(g_ipred_log_fp, "\n");	
		}
	}else
	{
		if(g_trace_enable_flag&TRACE_ENABLE_IPRED)	
		{
			fprintf(g_ipred_log_fp, "X X X X X X X X\n");
		}
	}

	//left
if(g_trace_enable_flag&TRACE_ENABLE_IPRED)	
{
	fprintf(g_ipred_log_fp, "the left 8 pixels:");
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
				fprintf(g_ipred_log_fp, "%d  ", pPix[i]);
			}
			fprintf(g_ipred_log_fp, "\n");	
		}
	}else
	{
		if(g_trace_enable_flag&TRACE_ENABLE_IPRED)	
		{
			fprintf(g_ipred_log_fp, "X X X X X X X X\n");
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
	fprintf(g_ipred_log_fp, "the left 8 pixels:");
	for(i = 0; i< 8; i++)
	{
		fprintf(g_ipred_log_fp, "%d  ", pLeftPix[i]);
	}
	fprintf(g_ipred_log_fp, "\n");
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
	fprintf(g_ipred_log_fp, "the top 8 pixels:");
	for(i = 0; i< 8; i++)
	{
		fprintf(g_ipred_log_fp, "%d  ", pTopRefPix[i]);
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
	fprintf(g_ipred_log_fp, "the top 8 pixels:");
	for (x = 0; x < 8; x++)
	{
		fprintf(g_ipred_log_fp, "%d  ", pRef[x]);
	}
	fprintf(g_ipred_log_fp, "\n");
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
	fprintf(g_ipred_log_fp, "the left 8 pixels:");
	for (y = 0; y < 8; y++)
	{
		fprintf(g_ipred_log_fp, "%d  ", pRef[y]);
	}
	fprintf(g_ipred_log_fp, "\n");
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

/************************************************************************/
/* Real Ipred functions                                                 */
/************************************************************************/

uint8 t0, t1, t2, t3, t4, t5, t6, t7;
uint8 l0, l1, l2, l3, l4, l5, l6, l7;
uint8 x;


void get_luma_nei_pixels(uint8 *pPred, uint8 *pTopPix, uint8 *pLeftPix, uint8 uTopLeftPix, int iBlkIdxInMB)
{
	uint8 *pLeft, *pTop; 
	int	mbAvailA = (g_mbc_reg_ptr->MBC_CMD0 >> 26) & 0x01;
	int	mbAvailB = (g_mbc_reg_ptr->MBC_CMD0 >> 25) & 0x01;
	int	mbAvailC = (g_mbc_reg_ptr->MBC_CMD0 >> 24) & 0x01;
	int	mbAvailD = (g_mbc_reg_ptr->MBC_CMD0 >> 27) & 0x01;
	int bTop = (!mbAvailB)&&(iBlkIdxInMB < 4);
	int bLeft = (!mbAvailA)&&((iBlkIdxInMB == 0) ||(iBlkIdxInMB == 4)||(iBlkIdxInMB == 8)||(iBlkIdxInMB == 12));

	if(iBlkIdxInMB < 4)
	{
		pTop = pTopPix + 4 * iBlkIdxInMB;
	}else
	{
		pTop = pPred - MBC_Y_SIZE;
	}

	if((iBlkIdxInMB == 0) ||(iBlkIdxInMB == 4) ||(iBlkIdxInMB == 8)||(iBlkIdxInMB == 12))
	{
		pLeft = pLeftPix + iBlkIdxInMB;
	}else
	{
		pLeft = pPred - 1;
	}

	if(bTop)
	{
		x = 128;

		if(bLeft)
		{
			l0 = t0 = 128;
			l1 = t1 = 128;
			l2 = t2 = 128;
			l3 = t3 = 128;
		}else
		{
			if((iBlkIdxInMB == 0) || (iBlkIdxInMB == 4) ||(iBlkIdxInMB == 8)  || (iBlkIdxInMB == 12))
			{
				l0 = t0 = pLeft[0];		l1 = t1 = pLeft[1];		l2 = t2 = pLeft[2];		l3 = t3 = pLeft[3];
			}else
			{
				l0 = t0 = pLeft[0];		l1 = t1 = pLeft[MBC_Y_SIZE];	l2 = t2 = pLeft[2 * MBC_Y_SIZE];	l3 = t3 = pLeft[3 * MBC_Y_SIZE];
			}
		}
	}else if(bLeft)
	{
		x = 128;
		l0 = t0 = pTop[0];
		l1 = t1 = pTop[1];
		l2 = t2 = pTop[2];
		l3 = t3 = pTop[3];
	}else
	{
		if(iBlkIdxInMB == 0)
		{
			x = uTopLeftPix;
		}else if(iBlkIdxInMB < 4)
		{
			x = pTop[-1];
		}else if((iBlkIdxInMB == 4) ||(iBlkIdxInMB == 8)  || (iBlkIdxInMB == 12))
		{
			x = pLeft[-1];
		}else
		{
			x = pLeft[-MBC_Y_SIZE];
		}

		if((iBlkIdxInMB == 0) || (iBlkIdxInMB == 4) ||(iBlkIdxInMB == 8)  || (iBlkIdxInMB == 12))
		{
			l0 = pLeft[0];		l1 = pLeft[1];		l2 = pLeft[2];		l3 = pLeft[3];
		}else
		{
			l0 = pLeft[0];		l1 = pLeft[MBC_Y_SIZE];	l2 = pLeft[2 * MBC_Y_SIZE];	l3 = pLeft[3 * MBC_Y_SIZE];
		}
		
		t0 = pTop[0];		t1 = pTop[1];		t2 = pTop[2];		t3 = pTop[3];

		if((iBlkIdxInMB == 7) || (iBlkIdxInMB == 11) ||(iBlkIdxInMB == 15) || ((iBlkIdxInMB == 3) && (!mbAvailC)))
		{
			t4 = t5 = t6 = t7 = t3;
		}else
		{
			t4 = pTop[4];		t5 = pTop[5];		t6 = pTop[6];		t7 = pTop[7];
		}

		if((iBlkIdxInMB != 0) && (iBlkIdxInMB != 4) && (iBlkIdxInMB != 8))
		{
			l4 = l5 = l6 = l7 = l3;
		}else
		{
			l4 = pLeft[4];		l5 = pLeft[5];		l6 = pLeft[6];		l7 = pLeft[7];
		}		
	}
}

void get_chroma_nei_pixels(uint8 *pPred, uint8 *pTopPix, uint8 *pLeftPix, uint8 uTopLeftPix,  int iBlkIdxInMB)
{
	uint8 *pLeft, *pTop; 
	int	mbAvailA = (g_mbc_reg_ptr->MBC_CMD0 >> 26) & 0x01;
	int	mbAvailB = (g_mbc_reg_ptr->MBC_CMD0 >> 25) & 0x01;
	int	mbAvailC = (g_mbc_reg_ptr->MBC_CMD0 >> 24) & 0x01;
	int	mbAvailD = (g_mbc_reg_ptr->MBC_CMD0 >> 27) & 0x01;
	int bTop = (!mbAvailB)&&(iBlkIdxInMB < 2);
	int bLeft = (!mbAvailA)&&((iBlkIdxInMB == 0) ||(iBlkIdxInMB == 2));

	if(iBlkIdxInMB < 2)
	{
		pTop = pTopPix + 4 * iBlkIdxInMB;
	}else
	{
		pTop = pPred - MBC_C_SIZE;
	}

	if((iBlkIdxInMB == 0) ||(iBlkIdxInMB == 2))
	{
		pLeft = pLeftPix + 2 * iBlkIdxInMB;
	}else
	{
		pLeft = pPred - 1;
	}

	if(bTop)
	{
		x = 128;

		if(bLeft)
		{
			l0 = t0 = 128;
			l1 = t1 = 128;
			l2 = t2 = 128;
			l3 = t3 = 128;
		}else
		{
			if((iBlkIdxInMB == 0)  ||(iBlkIdxInMB == 2))
			{
				l0 = t0 = pLeft[0];		l1 = t1 = pLeft[1];		l2 = t2 = pLeft[2];		l3 = t3 = pLeft[3];
			}else
			{
				l0 = t0 = pLeft[0];		l1 = t1 = pLeft[MBC_C_SIZE];		
				l2 = t2 = pLeft[2 * MBC_C_SIZE];		l3 = t3 = pLeft[3 * MBC_C_SIZE];
			}
		}
	}else if(bLeft)
	{
		x = 128;
		l0 = t0 = pTop[0];
		l1 = t1 = pTop[1];
		l2 = t2 = pTop[2];
		l3 = t3 = pTop[3];
	}else
	{
		if(iBlkIdxInMB == 0)
		{
			x = uTopLeftPix;
		}else if(iBlkIdxInMB == 1)
		{
			x = pTop[-1];
		}else if(iBlkIdxInMB == 2)
		{
			x = pLeft[-1];
		}else
		{
			x = pLeft[-MBC_C_SIZE];
		}	
		
		if((iBlkIdxInMB == 0)  ||(iBlkIdxInMB == 2))
		{
			l0 = pLeft[0];		l1 = pLeft[1];		l2 = pLeft[2];		l3 = pLeft[3];
		}else
		{
			l0 = pLeft[0];		l1 = pLeft[MBC_C_SIZE];		l2 = pLeft[2 * MBC_C_SIZE];		l3 = pLeft[3 * MBC_C_SIZE];
		}
		
		t0 = pTop[0];		t1 = pTop[1];		t2 = pTop[2];		t3 = pTop[3];

		if((iBlkIdxInMB == 3)  || ((iBlkIdxInMB == 1) && (!mbAvailC)))
		{
			t4 = t5 = t6 = t7 = t3;
		}else
		{
			t4 = pTop[4];		t5 = pTop[5];		t6 = pTop[6];		t7 = pTop[7];
		}

		if(iBlkIdxInMB != 0)
		{
			l4 = l5 = l6 = l7 = l3;
		}else
		{
			l4 = pLeft[4];		l5 = pLeft[5];		l6 = pLeft[6];		l7 = pLeft[7];
		}		
	}
}

//mode 0
void rv_intra_pred_4x4_DC_PRED(uint8 *pPred, /*MB_CACHE_T *pMbCache,*/ uint32 uBlkWidth)
{
	int32 sum = 0;
	uint32 *pIntPix;
	uint32 a;
	int		uBlkWidth_word,i;
	sum += l0; sum += l1; sum += l2, sum += l3;
	sum += t0; sum += t1; sum += t2; sum += t3;
	
	sum = (sum + 4)/8;

	a = ((sum << 24) |(sum << 16) |(sum << 8) |sum);

	pIntPix = (uint32 *)pPred;
	uBlkWidth_word = uBlkWidth >>2;
	for ( i = 0 ;i <4; i++)
	{
		pIntPix[i * uBlkWidth_word] = a;
	}
}

//mode 1
void rv_intra_pred_4x4_VERT_PRED(uint8 *pPred, /*MB_CACHE_T *pMbCache,*/ uint32 uBlkWidth)
{
	uint32 a;
	uint32 *pIntPix;
	int		uBlkWidth_word,i;
	int	mbAvailA = (g_mbc_reg_ptr->MBC_CMD0 >> 26) & 0x01;
	int	mbAvailB = (g_mbc_reg_ptr->MBC_CMD0 >> 25) & 0x01;
	int	mbAvailC = (g_mbc_reg_ptr->MBC_CMD0 >> 24) & 0x01;
	int	mbAvailD = (g_mbc_reg_ptr->MBC_CMD0 >> 27) & 0x01;

	if(!mbAvailB)
	{
//		IPRED_PRINTF("VERT_PRED4x4 luma not allowed!\n");
//		exit(-1);
	}
#ifdef Little_endian	
	a = (t3 << 24)|(t2 << 16)|(t1 << 8)|(t0); 
#else
	a = (t0 << 24)|(t1 << 16)|(t2 << 8)|(t3); 
#endif

	pIntPix = (uint32 *)pPred;
	
// 	pIntPix[0] = a;				*(pIntPix + 2*uOffset) = a;	
// 	*(pIntPix + 4*uOffset) = a;	*(pIntPix + 6*uOffset) = a;	
	uBlkWidth_word = uBlkWidth >>2;
	for ( i = 0 ;i <4; i++)
	{
		pIntPix[i * uBlkWidth_word] = a;
	}
}

//mode 2
void rv_intra_pred_4x4_HOR_PRED(uint8 *pPred, /*MB_CACHE_T *pMbCache,*/ uint32 uBlkWidth)
{
	uint8 *pp;
	int	mbAvailA = (g_mbc_reg_ptr->MBC_CMD0 >> 26) & 0x01;
	int	mbAvailB = (g_mbc_reg_ptr->MBC_CMD0 >> 25) & 0x01;
	int	mbAvailC = (g_mbc_reg_ptr->MBC_CMD0 >> 24) & 0x01;
	int	mbAvailD = (g_mbc_reg_ptr->MBC_CMD0 >> 27) & 0x01;
	if(!mbAvailA)
	{
	//	IPRED_PRINTF("HOR_PRED4x4 luma not allowed!\n");
	//	exit(-1);
	}

	pp = pPred;

	pp[0] = pp[1] = pp[2] = pp[3] = l0;		pp += uBlkWidth;
	pp[0] = pp[1] = pp[2] = pp[3] = l1; 	pp += uBlkWidth;
	pp[0] = pp[1] = pp[2] = pp[3] = l2; 	pp += uBlkWidth;
	pp[0] = pp[1] = pp[2] = pp[3] = l3;
}

//mode 3
void rv_intra_pred_4x4_DIAG_DOWN_RIGHT_PRED(uint8 *pPred, /*MB_CACHE_T *pMbCache,*/ uint32 uBlkWidth)
{
	int	mbAvailA = (g_mbc_reg_ptr->MBC_CMD0 >> 26) & 0x01;
	int	mbAvailB = (g_mbc_reg_ptr->MBC_CMD0 >> 25) & 0x01;
	int	mbAvailC = (g_mbc_reg_ptr->MBC_CMD0 >> 24) & 0x01;
	int	mbAvailD = (g_mbc_reg_ptr->MBC_CMD0 >> 27) & 0x01;
	if(!mbAvailA || !mbAvailB)
	{
	//	IPRED_PRINTF("DIAG_DOWN_RIGHT_PRED4x4 luma not allowed!\n");
	//	exit(-1);
	}
	
	/*x coordinate greater than y*/
	pPred[1] = pPred[2 + uBlkWidth] = pPred[3 + 2 * uBlkWidth] = (x + 2 * t0 + t1 + 2)/4;
	pPred[2] = pPred[3 + uBlkWidth] = (t0 + 2 * t1 + t2 + 2)/4;
	pPred[3] = (t1 + 2 * t2 + t3 + 2)/4;

	/*x coordinate equal y*/
	pPred[0] = pPred[1 + uBlkWidth] = pPred[2 + uBlkWidth * 2] = pPred[3 + uBlkWidth * 3] = (t0 + 2 *x + l0 + 2)/4;

	/*x<y*/
	pPred[uBlkWidth] = pPred[1 + 2 * uBlkWidth] = pPred[2 + 3 * uBlkWidth] = (l1 + 2 * l0 + x + 2)/4;
	pPred[2 * uBlkWidth] = pPred[1 + 3 * uBlkWidth] = (l2 + 2 * l1 + l0 + 2)/4;
	pPred[3 * uBlkWidth] = (l3 + 2 * l2 + l1 + 2)/4;	
}

//mode 4
void rv_intra_pred_4x4_DIAG_DOWN_LEFT_PRED(uint8 *pPred, /*MB_CACHE_T *pMbCache,*/ uint32 uBlkWidth)
{
	int	mbAvailA = (g_mbc_reg_ptr->MBC_CMD0 >> 26) & 0x01;
	int	mbAvailB = (g_mbc_reg_ptr->MBC_CMD0 >> 25) & 0x01;
	int	mbAvailC = (g_mbc_reg_ptr->MBC_CMD0 >> 24) & 0x01;
	int	mbAvailD = (g_mbc_reg_ptr->MBC_CMD0 >> 27) & 0x01;
	if(!mbAvailA || !mbAvailB)
	{
//		IPRED_PRINTF("DIAG_DOWN_LEFT_PRED4x4 luma not allowed!\n");
//		exit(-1);
	}
	
	pPred[0]  = (t0 + t2 + l0 + l2 + ((t1 + l1) << 1) + 4) >> 3;
	pPred[1]  = pPred[uBlkWidth]  = (t1 + t3 + l1 + l3 + ((t2 + l2) << 1) + 4) >> 3;
	pPred[2]  = pPred[uBlkWidth + 1]  = pPred[2 * uBlkWidth]  = (t2 + t4 + l2 + l4 + ((t3 + l3) << 1) + 4) >> 3;
	pPred[3]  = pPred[uBlkWidth + 2]  = pPred[2 * uBlkWidth + 1]  = pPred[3 * uBlkWidth] = (t3 + t5 + l3 + l5 + ((t4 + l4) << 1) + 4) >> 3;
	pPred[uBlkWidth + 3]  = pPred[2 * uBlkWidth + 2] = pPred[3*uBlkWidth + 1] = (t4 + t6 + l4 + l6 + ((t5 + l5) << 1) + 4) >> 3;
	pPred[2 * uBlkWidth + 3] = pPred[3 * uBlkWidth + 2] = (t5 + t7 + l5 + l7 + ((t6 + l6) << 1) + 4) >> 3;
	pPred[3 * uBlkWidth + 3] = (t6 + l6 + t7 + l7 + 2) >> 2;
}

//mode 5
void rv_intra_pred_4x4_VERT_RIGHT_PRED(uint8 *pPred, /*MB_CACHE_T *pMbCache,*/ uint32 uBlkWidth)
{
	int	mbAvailA = (g_mbc_reg_ptr->MBC_CMD0 >> 26) & 0x01;
	int	mbAvailB = (g_mbc_reg_ptr->MBC_CMD0 >> 25) & 0x01;
	int	mbAvailC = (g_mbc_reg_ptr->MBC_CMD0 >> 24) & 0x01;
	int	mbAvailD = (g_mbc_reg_ptr->MBC_CMD0 >> 27) & 0x01;
	if(!mbAvailA || !mbAvailB)
	{
//		IPRED_PRINTF("VERT_RIGHT_PRED4x4 luma not allowed!\n");
//		exit(-1);
	}

	pPred[0]  = pPred[2 * uBlkWidth + 1]  = (x + t0 + 1) >> 1;
	pPred[1]  = pPred[2 * uBlkWidth + 2] = (t0 + t1 + 1) >> 1;
	pPred[2]  = pPred[2 * uBlkWidth + 3] = (t1 + t2 + 1) >> 1;
	pPred[3]  =            (t2 + t3 + 1) >> 1;
	pPred[uBlkWidth]  = pPred[3 * uBlkWidth + 1] = (l0 + (x<<1) + t0 + 2) >> 2;
	pPred[uBlkWidth + 1]  = pPred[3 * uBlkWidth + 2] = (x + (t0<<1) + t1 + 2) >> 2;
	pPred[uBlkWidth + 2]  = pPred[3 * uBlkWidth + 3] = (t0 + (t1<<1) + t2 + 2) >> 2;
	pPred[uBlkWidth + 3]  =            (t1 + (t2<<1) + t3 + 2) >> 2;
	pPred[2 *uBlkWidth]  =            (x + (l0<<1) + l1 + 2) >> 2;
	pPred[3 * uBlkWidth] =            (l0 + (l1<<1) + l2 + 2) >> 2;
}

//mode 6
void rv_intra_pred_4x4_VERT_LEFT_PRED(uint8 *pPred, /*MB_CACHE_T *pMbCache,*/ uint32 uBlkWidth)
{
	int	mbAvailA = (g_mbc_reg_ptr->MBC_CMD0 >> 26) & 0x01;
	int	mbAvailB = (g_mbc_reg_ptr->MBC_CMD0 >> 25) & 0x01;
	int	mbAvailC = (g_mbc_reg_ptr->MBC_CMD0 >> 24) & 0x01;
	int	mbAvailD = (g_mbc_reg_ptr->MBC_CMD0 >> 27) & 0x01;
	if(!mbAvailA || !mbAvailB)
	{
//		IPRED_PRINTF("VERT_LEFT_PRED4x4 luma not allowed!\n");
//		exit(-1);
	}
	
	/*y = 0 or 2*/
    pPred[0] =  (((t0 + t1 + l2) << 1) + l1 + l3 + 4) >> 3;
    pPred[1] = pPred[2 *uBlkWidth] = (t1 + t2 + 1) >> 1;
   	pPred[2] = pPred[1+2 * uBlkWidth] = (t2 + t3 + 1) >> 1;
    pPred[3] = pPred[2+2 * uBlkWidth] = (t3 + t4 + 1) >> 1;
    pPred[3+2 * uBlkWidth] = (t4 + t5 + 1) >> 1;

	/*y = 1 or 3*/
	pPred[uBlkWidth] = (((t1 + l3) << 1) + t0 + t2 + l2 + l4 + 4) >> 3;
	pPred[1+uBlkWidth] = pPred[3 *uBlkWidth] = (t1 + (t2 << 1) + t3 + 2) >> 2;
	pPred[2+uBlkWidth] = pPred[1+3 * uBlkWidth] = (t2 + (t3 << 1) + t4 + 2) >> 2;
	pPred[3+uBlkWidth] = pPred[2+3 * uBlkWidth] = (t3 + (t4 << 1) + t5 + 2) >> 2;
	pPred[3+3 * uBlkWidth] = (t4 + (t5 << 1) + t6 + 2) >> 2;
}

//mode 7
void rv_intra_pred_4x4_HOR_UP_PRED(uint8 *pPred, /*MB_CACHE_T *pMbCache,*/ uint32 uBlkWidth)
{
	int	mbAvailA = (g_mbc_reg_ptr->MBC_CMD0 >> 26) & 0x01;
	int	mbAvailB = (g_mbc_reg_ptr->MBC_CMD0 >> 25) & 0x01;
	int	mbAvailC = (g_mbc_reg_ptr->MBC_CMD0 >> 24) & 0x01;
	int	mbAvailD = (g_mbc_reg_ptr->MBC_CMD0 >> 27) & 0x01;
	if(!mbAvailA || !mbAvailB)
	{
//		IPRED_PRINTF("HOR_UP_PRED4x4 luma not allowed!\n");
//		exit(-1);
	}

	/* (x + 2y) = 0, 2, 4*/
	pPred[0] = (((t2 + l0 + l1) << 1) + t1 + t3 + 4) >> 3;
	pPred[2] = pPred[uBlkWidth] = (((t4 + l1 + l2) << 1) + t3 + t5 + 4) >> 3;
	pPred[2+uBlkWidth] = pPred[2 *uBlkWidth] = (((t6 + l2 + l3) << 1) + t5 + t7 + 4) >> 3;

    /* (x + 2y) = 1, 3*/
   	pPred[1] = (((t3 + l1) << 1) + t2 + t4 + l0 + l2 + 4) >> 3;
    pPred[3] = pPred[1+uBlkWidth] = (((t5 + l2) << 1) + t4 + t6 + l1 + l3 + 4) >> 3;

    /* (x + 2y) = 5*/
    pPred[3+uBlkWidth] = pPred[1+2 * uBlkWidth] = (((t7 + l3) << 1) + t6 + t7 + l2 + l3 + 4) >> 3;

    /* (x + 2y) > 5*/
    pPred[2+2 * uBlkWidth] = pPred[3 *uBlkWidth] = (t6 + t7 + l3 + l4 + 2) >> 2;;
    pPred[3+2 * uBlkWidth] = pPred[1+3 * uBlkWidth] = (l3 + (l4 << 1) + l5 + 2) >> 2;
    pPred[2+3 * uBlkWidth] = (l4 + l5 + 1) >> 1;
    pPred[3+3 * uBlkWidth] =  (l4 + (l5 << 1) + l6 + 2) >> 2;
}

//mode 8
void rv_intra_pred_4x4_HOR_DOWN_PRED(uint8 *pPred, /*MB_CACHE_T *pMbCache,*/ uint32 uBlkWidth)
{
	/* (2 * y - x) equal 0, 2, 4, 6 */
    pPred[0] = pPred[2+uBlkWidth] = (x + l0 + 1) >> 1;
    pPred[uBlkWidth] = pPred[2+2 * uBlkWidth] = (l0 + l1 + 1) >> 1;
    pPred[2 *uBlkWidth] = pPred[2+3 * uBlkWidth] = (l1 + l2 + 1) >> 1;
    pPred[3 *uBlkWidth] = (l2 + l3 + 1) >> 1;

    /* (2 * y - x) equal 1, 3, 5 */
    pPred[1+uBlkWidth] = pPred[3+2 * uBlkWidth] = (x + (l0 << 1) + l1 + 2) >> 2;
    pPred[1+2 * uBlkWidth] = pPred[3+3 * uBlkWidth] = (l0 + (l1 << 1) + l2 + 2) >> 2;
    pPred[1+3 * uBlkWidth] = (l1 + (l2 << 1) + l3 + 2) >> 2;

    /* (2 * y - x) equal -1 */
    pPred[1] = pPred[3+uBlkWidth] = (l0 + (x << 1) + t0 + 2) >> 2;

    /* (2 * y - x) equal -2, -3 */
    pPred[2] = (x + (t0 << 1) + t1 + 2) >> 2;
    pPred[3] = (t0 + (t1 << 1) + t2 + 2) >> 2;
}

void rv_intra_pred_Luma16x16_DC_PRED(/*MB_CACHE_T *pMbCache,*/ uint8 *pTopPix, uint8 *pLeftPix, uint8 uTopLeftPix, uint8 *pPred)
{
	int32 i;
	int32 sum = 0;
	int	mbAvailA = (g_mbc_reg_ptr->MBC_CMD0 >> 26) & 0x01;
	int	mbAvailB = (g_mbc_reg_ptr->MBC_CMD0 >> 25) & 0x01;
	int	mbAvailC = (g_mbc_reg_ptr->MBC_CMD0 >> 24) & 0x01;
	int	mbAvailD = (g_mbc_reg_ptr->MBC_CMD0 >> 27) & 0x01;

	if(!mbAvailA && !mbAvailB)
	{
		sum = 128;
	}else if(!mbAvailB && mbAvailA)
	{
		for(i = 0; i < 16; i++)
		{
			sum += pLeftPix[i];
		}
		sum = (sum + 8)>>4;
	}else if(mbAvailB && (!mbAvailA))
	{
		for(i = 0; i < 16; i++)
		{
			sum += pTopPix[i];
		}
		sum = (sum + 8)>>4;
	}else
	{
		for(i = 0; i < 16; i++)
		{
			sum += pLeftPix[i];
			sum += pTopPix[i];
		}
		sum = (sum + 16)>>5;
	}

	//memset(pPred, sum, 256 * sizeof(uint8));
	for (i= 0; i<16 ; i++)
	{
		memset(pPred+ i*24,sum,16 * sizeof(uint8));
	}

	FPRINTF(g_fp_trace_ipred, "the top 16 pixels:");
	for(i = 0; i< 16; i++)
	{
		FPRINTF(g_fp_trace_ipred, "%d  ", pTopPix[i]);
	}

	FPRINTF(g_fp_trace_ipred, "\nthe left 16 pixels:");
	for(i = 0; i< 16; i++)
	{
		FPRINTF(g_fp_trace_ipred, "%d  ", pLeftPix[i]);
	}
	FPRINTF(g_fp_trace_ipred, "\n");
}

void rv_intra_pred_Luma16x16_VERT_PRED(/*MB_CACHE_T *pMbCache,*/ uint8 *pTopPix, uint8 *pLeftPix, uint8 uTopLeftPix, uint8 *pPred)
{
	uint32 i;
	int	mbAvailA = (g_mbc_reg_ptr->MBC_CMD0 >> 26) & 0x01;
	int	mbAvailB = (g_mbc_reg_ptr->MBC_CMD0 >> 25) & 0x01;
	int	mbAvailC = (g_mbc_reg_ptr->MBC_CMD0 >> 24) & 0x01;
	int	mbAvailD = (g_mbc_reg_ptr->MBC_CMD0 >> 27) & 0x01;

	if(!mbAvailB)
	{
		IPRED_PRINTF("Luma16x16_VERT_PRED not allowed!\n");
		exit(-1);
	}

	for(i = 0; i < 16; i++)
	{
		memcpy(pPred + i * 24, pTopPix, 16 * sizeof(uint8));
	}

	FPRINTF(g_fp_trace_ipred, "the top 16 pixels:");
	for(i = 0; i< 16; i++)
	{
		FPRINTF(g_fp_trace_ipred, "%d  ", pTopPix[i]);
	}

	FPRINTF(g_fp_trace_ipred, "\nthe left 16 pixels: No Used");
	FPRINTF(g_fp_trace_ipred, "\n");
}

void rv_intra_pred_Luma16x16_HORZ_PRED(/*MB_CACHE_T *pMbCache,*/ uint8 *pTopPix, uint8 *pLeftPix, uint8 uTopLeftPix, uint8 *pPred)
{
	uint32 i;
	int	mbAvailA = (g_mbc_reg_ptr->MBC_CMD0 >> 26) & 0x01;
	int	mbAvailB = (g_mbc_reg_ptr->MBC_CMD0 >> 25) & 0x01;
	int	mbAvailC = (g_mbc_reg_ptr->MBC_CMD0 >> 24) & 0x01;
	int	mbAvailD = (g_mbc_reg_ptr->MBC_CMD0 >> 27) & 0x01;

	if(!mbAvailA)
	{
		IPRED_PRINTF("Luma16x16_HORZ_PRED not allowed!\n");
		exit(-1);
	}

	for(i = 0; i < 16; i++)
	{
		memset(pPred + i * 24, pLeftPix[i], 16 * sizeof(uint8));
	}
	FPRINTF(g_fp_trace_ipred, "the top 16 pixels: No Used");

	FPRINTF(g_fp_trace_ipred, "\nthe left 16 pixels:\n ");
	for(i = 0; i< 16; i++)
	{
		FPRINTF(g_fp_trace_ipred, "%d  ", pLeftPix[i]);
	}
	FPRINTF(g_fp_trace_ipred, "\n");

}

void rv_intra_pred_Luma16x16_PLANAR_PRED(/*MB_CACHE_T *pMbCache,*/ uint8 *pTopPix, uint8 *pLeftPix, uint8 uTopLeftPix, uint8 *pPred)
{
	int i,j;
	int32 iH = 0, iV = 0;
	uint8 *tp, *lp;
	int32 a, b, c;
	int32 temp;
	int	mbAvailA = (g_mbc_reg_ptr->MBC_CMD0 >> 26) & 0x01;
	int	mbAvailB = (g_mbc_reg_ptr->MBC_CMD0 >> 25) & 0x01;
	int	mbAvailC = (g_mbc_reg_ptr->MBC_CMD0 >> 24) & 0x01;
	int	mbAvailD = (g_mbc_reg_ptr->MBC_CMD0 >> 27) & 0x01;
	
	if(!(mbAvailA && mbAvailB))
	{
		IPRED_PRINTF("Luma16x16_HORZ_PRED not allowed!\n");
		exit(-1);
	}

	tp = pTopPix - 1;
	lp = pLeftPix -1;

	FPRINTF(g_fp_trace_ipred, "the top 16 pixels:");
	for(i = 0; i< 16; i++)
	{
		FPRINTF(g_fp_trace_ipred, "%d  ", pTopPix[i]);
	}

	FPRINTF(g_fp_trace_ipred, "\nthe left 16 pixels:");
	for(i = 0; i< 16; i++)
	{
		FPRINTF(g_fp_trace_ipred, "%d  ", pLeftPix[i]);
	}
	FPRINTF(g_fp_trace_ipred, "\nx = %d\n", uTopLeftPix);

	for(i = 1; i < 8; i++)
	{
		iH += i * (tp[8+i] - tp[8-i]);
		iV += i * (lp[8+i] - lp[8-i]);
	}

	iH += 8*(tp[16] - uTopLeftPix);
	iV += 8*(lp[16] - uTopLeftPix);
	FPRINTF(g_fp_trace_ipred, "H = %d, V = %d\n", iH, iV);

	a = (tp[16]+lp[16])*16;
	b = (iH + (iH>>2))>>4;
	c = (iV + (iV>>2))>>4;

	FPRINTF(g_fp_trace_ipred, "a = %d, b = %d, c = %d\n", a, b, c);
	
	for (j=0; j<16; j++)
	{
		for (i=0; i<16; i++)
		{
			temp= (a+(i-7)*b+(j-7)*c+16)>>5;
			temp= (temp>255)?255:temp;
			temp= (temp<0)?0:temp;
			pPred[i + j*24]=(uint8) temp;
		}
	}
}

void rv_intra_pred_CHROMA8x8_DC_PRED(/*MB_CACHE_T *pMbCache, */uint8 *pTopPix, uint8 *pLeftPix, uint8 uTopLeftPix, uint8 *pPred)
{
	int32 i;
	int32 sum = 0;
	int	mbAvailA = (g_mbc_reg_ptr->MBC_CMD0 >> 26) & 0x01;
	int	mbAvailB = (g_mbc_reg_ptr->MBC_CMD0 >> 25) & 0x01;
	int	mbAvailC = (g_mbc_reg_ptr->MBC_CMD0 >> 24) & 0x01;
	int	mbAvailD = (g_mbc_reg_ptr->MBC_CMD0 >> 27) & 0x01;

	if(!mbAvailA && !mbAvailB)
	{
		sum = 128;
	}else if(!mbAvailB && mbAvailA)
	{
		for(i = 0; i < 8; i++)
		{
			sum += pLeftPix[i];
		}
		sum = (sum + 4)>>3;
	}else if(mbAvailB && (!mbAvailA))
	{
		for(i = 0; i < 8; i++)
		{
			sum += pTopPix[i];
		}
		sum = (sum + 4)>>3;
	}else
	{
		for(i = 0; i < 8; i++)
		{
			sum += pLeftPix[i];
			sum += pTopPix[i];
		}
		sum = (sum + 8)>>4;
	}

	//memset(pPred, sum, 64 * sizeof(uint8));
	for (i = 0; i< 8 ; i ++)
	{
		memset(pPred + i*16, sum, 8 * sizeof(uint8));
	}

	FPRINTF(g_fp_trace_ipred, "the top 8 pixels:");
	for(i = 0; i< 8; i++)
	{
		FPRINTF(g_fp_trace_ipred, "%d  ", pTopPix[i]);
	}

	FPRINTF(g_fp_trace_ipred, "\nthe left 8 pixels:");
	for(i = 0; i< 8; i++)
	{
		FPRINTF(g_fp_trace_ipred, "%d  ", pLeftPix[i]);
	}
	FPRINTF(g_fp_trace_ipred, "\n");

}

void rv_intra_pred_CHROMA8x8_VERT_PRED(/*MB_CACHE_T *pMbCache,*/ uint8 *pTopPix, uint8 *pLeftPix, uint8 uTopLeftPix, uint8 *pPred)
{
	uint32 i;
	int	mbAvailA = (g_mbc_reg_ptr->MBC_CMD0 >> 26) & 0x01;
	int	mbAvailB = (g_mbc_reg_ptr->MBC_CMD0 >> 25) & 0x01;
	int	mbAvailC = (g_mbc_reg_ptr->MBC_CMD0 >> 24) & 0x01;
	int	mbAvailD = (g_mbc_reg_ptr->MBC_CMD0 >> 27) & 0x01;

	if(!mbAvailB)
	{
		IPRED_PRINTF("CHROMA8x8_VERT_PRED not allowed!\n");
		exit(-1);
	}

	for(i = 0; i < 8; i++)
	{
		memcpy(pPred + i * 16, pTopPix, 8 * sizeof(uint8));
	}

	FPRINTF(g_fp_trace_ipred, "the top 8 pixels:");
	for(i = 0; i< 8; i++)
	{
		FPRINTF(g_fp_trace_ipred, "%d  ", pTopPix[i]);
	}

	FPRINTF(g_fp_trace_ipred, "\nthe left 8 pixels: No Used");
	FPRINTF(g_fp_trace_ipred, "\n");

}

void rv_intra_pred_CHROMA8x8_HORZ_PRED(/*MB_CACHE_T *pMbCache,*/ uint8 *pTopPix, uint8 *pLeftPix, uint8 uTopLeftPix, uint8 *pPred)
{
	uint32 i;
	int	mbAvailA = (g_mbc_reg_ptr->MBC_CMD0 >> 26) & 0x01;
	int	mbAvailB = (g_mbc_reg_ptr->MBC_CMD0 >> 25) & 0x01;
	int	mbAvailC = (g_mbc_reg_ptr->MBC_CMD0 >> 24) & 0x01;
	int	mbAvailD = (g_mbc_reg_ptr->MBC_CMD0 >> 27) & 0x01;
	if(!mbAvailA)
	{
		IPRED_PRINTF("CHROMA8x8_HORZ_PRED not allowed!\n");
		exit(-1);
	}

	for(i = 0; i < 8; i++)
	{
		memset(pPred + i * 16, pLeftPix[i], 8 * sizeof(uint8));
	}

	FPRINTF(g_fp_trace_ipred, "the top 8 pixels: No Used");

	FPRINTF(g_fp_trace_ipred, "\nthe left 8 pixels:\n");
	for(i = 0; i< 8; i++)
	{
		FPRINTF(g_fp_trace_ipred, "%d  ", pLeftPix[i]);
	}
	FPRINTF(g_fp_trace_ipred, "\n");
}

