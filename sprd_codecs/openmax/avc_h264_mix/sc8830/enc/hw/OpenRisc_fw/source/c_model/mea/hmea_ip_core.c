
#include "video_common.h"
#include "mmcodec.h"
#include "hmea_mode.h"
#include "hmea_global.h"
#include "h264enc_mode.h"
#include "h264enc_global.h"
#include "common_global.h"
#include "buffer_global.h"

void me_ipred_luma16x16_DC_PRED (uint8 *pPred, 
								 uint8 *pTopLeftPix,
								 uint8 *pLeftPix )
{
	int32 i, j;
	uint8 *pPix;
	int32 pred_sum = 0;
	int32 avail;
	int32 avail_num = 0;

	//left
	avail = g_mode_dcs_ptr->left_avail;
	if (avail)
	{
		avail_num++;
		pPix = pLeftPix;

		for (i = 0; i < 16; i++)
		{
			pred_sum += pPix[i];
		}
	}

	//top
	avail  = g_mode_dcs_ptr->top_avail;
	if (avail)
	{
		avail_num++;
		pPix = pTopLeftPix + 1;

		for (i = 0; i < 16; i++)
		{
			pred_sum += pPix[i];
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
		pPix += 16;
	}
}

void me_ipred_luma16x16_HOR_PRED (uint8 *pPred, 
								 uint8 *pTopLeftPix,
								 uint8 *pLeftPix )
{
	int32 i, j;
	uint8 *pPix;
	uint8 *pLeftRefPix = pLeftPix;
	int32 avail = g_mode_dcs_ptr->left_avail; //mbAvailA

	if (!avail)
	{
		PRINTF ("me_ipred_luma16x16_HOR_PRED is not allowed!\n");
		return;
	}

	pPix = pPred;
	for (i = 0; i < 16; i++) //16 row
	{
		uint8 left_pix = pLeftPix[i];

		for (j = 0; j < 16; j++) //16 pixels per row
		{
			pPix[j] = left_pix;
		}
		pPix += 16;
	}

	return;
}

void me_ipred_luma16x16_VERT_PRED (uint8 *pPred, 
								 uint8 *pTopLeftPix,
								 uint8 *pLeftPix )
{
	int32 i, j;
	uint8 *pPix;
	uint8 *pTopRefPix = pTopLeftPix + 1;
	int32 avail  = g_mode_dcs_ptr->top_avail; //mbAvailB

	if (!avail)
	{
		PRINTF ("me_ipred_luma16x16_VERT_PRED is not allowed!\n");
		return;
	}

	pPix = pPred;
	for (i = 0; i < 16; i++) //16 row
	{
		for (j = 0; j < 16; j++) //16 pixel per row
		{
			pPix[j] = pTopRefPix[j];
		}
		pPix += 16;
	}

	return;
}

//4x4 luma intra prediction

//mode 0
void me_ipred_luma4x4_VERT_PRED ( uint8 *pPred, 
								  uint8 *pTopPix,
								  uint8 *pLeftPix
								  )
{
	uint32 a;
	uint32 *pIntPix;

	a = *((uint32 *)pTopPix);

	pIntPix = (uint32 *)pPred;

	pIntPix[0] = a; pIntPix += 4;
	pIntPix[0] = a; pIntPix += 4; //next line
	pIntPix[0] = a; pIntPix += 4; //next line
	pIntPix[0] = a; //next line

	return;
}

//mode 1
void me_ipred_luma4x4_HOR_PRED (  uint8 *pPred, 
								  uint8 *pTopPix,
								  uint8 *pLeftPix
								  )
{
	int32 i;
	uint8 *pPixBlk;

	pPixBlk = pPred;

	for (i = 0; i < 4; i++) //four line
	{
		pPixBlk[0] = pLeftPix[i]; //pixel 0
		pPixBlk[1] = pLeftPix[i]; //pixel 1
		pPixBlk[2] = pLeftPix[i]; //pixel 2
		pPixBlk[3] = pLeftPix[i]; //pixel 3

		//next line
		pPixBlk += 16;
	}

	return;
}

//mode 2
void me_ipred_luma4x4_DC_PRED (   uint8 blkIdxInMB,
							      uint8 *pPred, 
								  uint8 *pTopPix,
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

		pred_sum += pLeftPix[0];
		pred_sum += pLeftPix[1]; //next line
		pred_sum += pLeftPix[2]; //next line
		pred_sum += pLeftPix[3]; //next line
	}else
	{
		avail = g_mode_dcs_ptr->left_avail; //mbAvailA

		if (avail)
		{
			blk_avail_num++;

			pPix = pLeftPix + blkIdxInMB;

			pred_sum += pLeftPix[0]; 
			pred_sum += pLeftPix[1]; //next line
			pred_sum += pLeftPix[2]; //next line
			pred_sum += pLeftPix[3]; //next line
		}
	}

	//top pixel
	if (blkIdxInMB >= 4)
	{
		blk_avail_num++;

		pred_sum += pTopPix[0]; 
		pred_sum += pTopPix[1]; //next line
		pred_sum += pTopPix[2]; //next line
		pred_sum += pTopPix[3]; //next line
	}else
	{
		avail = g_mode_dcs_ptr->top_avail; //mbAvailB

		if (avail)
		{
			blk_avail_num++;

			pred_sum += pTopPix[0]; 
			pred_sum += pTopPix[1]; //next line
			pred_sum += pTopPix[2]; //next line
			pred_sum += pTopPix[3]; //next line
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
		pPixBlk += 16;
	}

	return;
}

//chroma intra prediction

//chorma mode 0
void me_ipred_chroma8x8_DC_PRED(uint8 *pPred, 
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
	top_avail = g_mode_dcs_ptr->top_avail; //mbAvailB
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
	}

	//left
	left_avail = g_mode_dcs_ptr->left_avail; //mbAvailA
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

		pPixBlk += 8; //next line
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

		pPixBlk += 8; //next line
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

	pPixBlk = pPred +4*8;	
	for (i = 0; i < 4; i++)
	{
		pPixBlk[0] = pred_sum;
		pPixBlk[1] = pred_sum;
		pPixBlk[2] = pred_sum;
		pPixBlk[3] = pred_sum;

		pPixBlk += 8; //next line
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

	pPixBlk = pPred +4*8+4;	
	for (i = 0; i < 4; i++)
	{
		pPixBlk[0] = pred_sum;
		pPixBlk[1] = pred_sum;
		pPixBlk[2] = pred_sum;
		pPixBlk[3] = pred_sum;

		pPixBlk += 8; //next line
	}

	return;
}

//mode 1
void me_ipred_chroma8x8_HOR_PRED(uint8 *pPred, 
									uint8 *pTopLeftPix,
									uint8 *pLeftPix)
{
	int32 i,j;
	uint8 *pPix;
	uint8 *pLeftRefPix = pLeftPix;
	int32 avail = g_mode_dcs_ptr->left_avail; //mbAvailA

	if (!avail)
	{
		PRINTF ("intra_pred_chroma_HOR_PRED is not allowed!\n");
		return;
	}

	pPix = pPred;
	for (i = 0; i < 8; i++) //8 row
	{
		uint8 left_pix = pLeftPix[i];

		for (j = 0; j < 8; j++) //8 pixels per row
		{
			pPix[j] = left_pix;
		}
		pPix += 8;
	}

	return;
}

//mode 2
void me_ipred_chroma8x8_VERT_PRED(uint8 *pPred, 
								uint8 *pTopLeftPix,
								uint8 *pLeftPix)
{
	int32 i, j;
	uint8 *pPix;
	uint8 *pTopRefPix = pTopLeftPix + 1;
	int32 avail = g_mode_dcs_ptr->top_avail; //mbAvailB

	if (!avail)
	{
		PRINTF ("intra_pred_chroma_VERT_PRED is not allowed!\n");
		return;
	}

	pPix = pPred;
	for (i = 0; i < 8; i++) //8 row
	{
		for (j = 0; j < 8; j++) //8 pixels per row
		{
			pPix[j] = pTopRefPix[j];
		}
		pPix += 8;
	}

	return;
}

void IntraPred_get_ref_mb (MODE_DECISION_T * mode_dcs_ptr)
{ 		
	uint8 *topBorder_luma, *topBorder_chroma[2];
	int32 mb_num_x = (g_glb_reg_ptr->VSP_CFG1 & 0x1ff);
	int32 mb_x = mode_dcs_ptr->mb_x;
	int32 ipredmode_chroma = mode_dcs_ptr->intra_mode_c;
	uint8 *pred;
	
	topBorder_luma = g_enc_image_ptr->ipred_top_line_buffer + mb_x * MB_SIZE;
	topBorder_chroma[0] = g_enc_image_ptr->ipred_top_line_buffer + mb_num_x * MB_SIZE + mb_x * MB_CHROMA_SIZE;
	topBorder_chroma[1] = g_enc_image_ptr->ipred_top_line_buffer + mb_num_x * (MB_SIZE+MB_CHROMA_SIZE) + mb_x * MB_CHROMA_SIZE;
	
	//load current mb's bordery data
	memcpy(g_luma_top+1, topBorder_luma, 16+4);
	g_luma_top[0] = g_top_left[0];
	
	if (mb_x == 0)
	{
		memset (g_luma_left, 0, 16);
		memset (g_chroma_left[0], 0, 8);
		memset (g_chroma_left[1], 0, 8);
	}
	
	//luma
	if (mode_dcs_ptr->mb_type == INTRA_MB_16X16)
	{
		int32 i16mode = (mode_dcs_ptr->intra16_mode_y);
		
		pred = (uint8 *)vsp_mea_out_bfr;
		
		switch(i16mode) 
		{
		case INTRA_L_VER:
			me_ipred_luma16x16_VERT_PRED(pred, g_luma_top, g_luma_left);
			break;
		case INTRA_L_HOR:
			me_ipred_luma16x16_HOR_PRED(pred, g_luma_top, g_luma_left);
			break;
		case INTRA_L_DC:
			me_ipred_luma16x16_DC_PRED(pred, g_luma_top, g_luma_left);
			break;
		default:
			break;
		}
	}
	else //4x4 must call dct and idct module, copy reconstructed pixels of neighbor block
	{
		
	}
	
	//chroma
	//u
	memcpy (g_chroma_top[0]+1, topBorder_chroma[0], 8);
	*(g_chroma_top[0]) =  g_top_left[1];
	
	pred = (uint8 *)vsp_mea_out_bfr + 16*16;
	switch(ipredmode_chroma) 
	{
	case INTRA_C_VER:
		me_ipred_chroma8x8_VERT_PRED (pred, g_chroma_top[0], g_chroma_left[0]);
		break;
	case INTRA_C_HOR:
		me_ipred_chroma8x8_HOR_PRED (pred, g_chroma_top[0], g_chroma_left[0]);
		break;
	case INTRA_C_DC:
		me_ipred_chroma8x8_DC_PRED (pred, g_chroma_top[0], g_chroma_left[0]);
		break;
	default:
		break;
	}
	
	//v
	memcpy (g_chroma_top[1]+1, topBorder_chroma[1], 8);
	*(g_chroma_top[1]) =  g_top_left[2];
	
	pred = (uint8 *)vsp_mea_out_bfr + 16*16+8*8;
	switch(ipredmode_chroma) 
	{
	case INTRA_C_VER:
		me_ipred_chroma8x8_VERT_PRED (pred, g_chroma_top[1], g_chroma_left[1]);
		break;
	case INTRA_C_HOR:
		me_ipred_chroma8x8_HOR_PRED (pred, g_chroma_top[1], g_chroma_left[1]);
		break;
	case INTRA_C_DC:
		me_ipred_chroma8x8_DC_PRED (pred, g_chroma_top[1], g_chroma_left[1]);
		break;
	default:
		break;
	}
}

void IntraPred_get_ref_blk (uint8 blkIdx)
{
	//将预测值输出到mea_out_bfr中的固定位置，利用mea_src_bfr中的值求出残差存到dct_io_bfr_0（确实是按行扫描来存的，而不是4x4块扫描来存的）中的位置

	uint8 *topBorder_luma;
	int32 mb_num_x = (g_glb_reg_ptr->VSP_CFG1 & 0x1ff);
	int32 mb_x = g_mode_dcs_ptr->mb_x;
	uint8 *pred;
	uint8 blk_x, blk_y;
	uint8 *luma_top; 
	uint8 luma_left[4];
	uint8 *line_ptr;

	blk_x = blkIdx%4;
	blk_y = (blkIdx>>2);

	//first derive the MB neighboring pixels
	
	topBorder_luma = g_enc_image_ptr->ipred_top_line_buffer + mb_x * MB_SIZE;
	
	//load current mb's boundary data
	memcpy(g_luma_top+1, topBorder_luma, 16+4);
	g_luma_top[0] = g_top_left[0];
	
	if (mb_x == 0)
	{
		memset (g_luma_left, 0, 16);
	}

	//derive the neighboring 4x4 block pixels

	if (blk_x == 0)
	{
		line_ptr = g_luma_left + 4*blk_y;
		luma_left[0] = line_ptr[0];
		luma_left[1] = line_ptr[1];
		luma_left[2] = line_ptr[2];
		luma_left[3] = line_ptr[3];
	}
	else
	{
		line_ptr = (uint8 *)mea_out_buf0 + 4*blk_x + 64*blk_y - 1;//不要用mbc buffer了，用mea out buffer这里的mbc buffer比以前解码器中的mbc out buffer要小一些，是5x5而不是5x6的
		luma_left[0] = line_ptr[0];
		luma_left[1] = line_ptr[1*MB_SIZE];
		luma_left[2] = line_ptr[2*MB_SIZE];
		luma_left[3] = line_ptr[3*MB_SIZE];
	}

	if (blk_y == 0)
	{
		luma_top = g_luma_top + 1 + 4*blk_x;
	}
	else
	{
		luma_top = (uint8 *)mea_out_buf0 + 4*blk_x + 64*blk_y - 16;
	}
	
	//4x4 intra luma prediction
	{
		int32 i4mode = (g_mode_dcs_ptr->intra4x4_pred_mode[blkIdx]);
		
		pred = (uint8 *)vsp_mea_out_bfr + 4*blk_x + 64*blk_y;
		
		switch(i4mode) 
		{
		case INTRA_L_VER:
			me_ipred_luma4x4_VERT_PRED(pred, luma_top, luma_left);
			break;
		case INTRA_L_HOR:
			me_ipred_luma4x4_HOR_PRED(pred, luma_top, luma_left);
			break;
		case INTRA_L_DC:
			me_ipred_luma4x4_DC_PRED(blkIdx, pred, luma_top, luma_left);
			break;
		default:
			break;
		}
	}	

	/////////////////////GET THE RESIDUE/////////////////////
	{
		uint32 i,j;
		
		uint8 * pSrc = (uint8 *)mea_src_buf0 + 4*blk_x + 64*blk_y;
		uint8 * pRef = (uint8 *)vsp_mea_out_bfr + 4*blk_x + 64*blk_y;
		int16 * pDst;
		
		int16 * dct_io_bfr = (int16*)vsp_dct_io_0 + 4*blk_x + 64*blk_y;;
		
		pDst = dct_io_bfr;
		
		for(j=0; j<4; j++)
		{
			for (i=0; i<4; i++)
			{
				pDst[i+j*16] = (int16)pSrc[i+j*16] - (int16)pRef[i+j*16];
			}
		}
	}
}
