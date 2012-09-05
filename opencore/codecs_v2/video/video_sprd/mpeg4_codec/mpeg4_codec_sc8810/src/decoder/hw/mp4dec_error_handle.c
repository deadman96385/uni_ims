/******************************************************************************
 ** File Name:    mp4dec_error_handle.c                                       *
 ** Author:       Xiaowei Luo                                                 *
 ** DATE:         11/24/2009                                                  *
 ** Copyright:    2009 Spreatrum, Incoporated. All Rights Reserved.           *
 ** Description:                                                              *
 *****************************************************************************/
/******************************************************************************
 **                   Edit    History                                         *
 **---------------------------------------------------------------------------* 
 ** DATE          NAME            DESCRIPTION                                 * 
 ** 11/24/2009    Xiaowei Luo     Create.                                     *
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

//now, don't define "DO_MC_CHECK" due to ARM cache issue.20100525
//#define DO_MC_CHECK

LOCAL int32 GetHorGrad (uint8 *curr_blk_pix, int32 frame_width, int32 blk_size, int32 is_uv)
{
	int32 i;
	int32 mad = 0;
	int32 left_pos = (is_uv ? (-2) : (-1));

	for (i = 0; i < blk_size; i++)
	{
		mad += ABS(curr_blk_pix[0] - curr_blk_pix[left_pos]);
		
		curr_blk_pix += frame_width;
	}

	return mad;
}

LOCAL int32 GetVertGrad (uint8 *curr_blk_pix, int32 frame_width, int32 blk_size, int32 is_uv)
{
	int32 i;
	uint8 *top_pix = curr_blk_pix - frame_width;
	int32 mad = 0;
	int32 step = (is_uv ? (2) : (1));

	for (i = 0; i < blk_size; i += step)
	{
		mad += ABS(curr_blk_pix[i] - top_pix[i]);
	}

	return mad;
}

//only for uv_interleaved format, xwluo@20100927
LOCAL int32 GetAvgGrad (DEC_VOP_MODE_T *vop_mode_ptr, int32 nei_avail[], int32 avail_num, int32 component_num)
{	
	int32 frame_width = vop_mode_ptr->FrameWidth;
	int32 curr_blk_addr = 0;
	uint8 *blk_ptr = PNULL;
	int32 blk_width = MB_SIZE;
	int32 shift = 4;
	int32 avg_grad = 0;
	int32 is_uv = 0;

	SCI_ASSERT (component_num < 3);

	if (component_num == 0) //luma
	{
		curr_blk_addr = vop_mode_ptr->mb_y * MB_SIZE * frame_width + vop_mode_ptr->mb_x * MB_SIZE;
		blk_ptr = vop_mode_ptr->pCurRecFrame->pDecFrame->imgY + curr_blk_addr;
		blk_width = MB_SIZE;
		shift = 4;
	}else if (component_num == 1) //chroma U
	{
		curr_blk_addr = vop_mode_ptr->mb_y * BLOCK_SIZE * frame_width + vop_mode_ptr->mb_x * BLOCK_SIZE*2;
		blk_ptr = vop_mode_ptr->pCurRecFrame->pDecFrame->imgU + curr_blk_addr;
		blk_width = BLOCK_SIZE;
		shift = 3;
		is_uv = 1;
	}else //chroma V
	{
		curr_blk_addr = vop_mode_ptr->mb_y * BLOCK_SIZE * frame_width + vop_mode_ptr->mb_x * BLOCK_SIZE*2+1;
		blk_ptr = vop_mode_ptr->pCurRecFrame->pDecFrame->imgU + curr_blk_addr;
		blk_width = BLOCK_SIZE;
		shift = 3;
		is_uv = 1;
	}

	if (nei_avail[0]) //left
	{
		avg_grad += GetHorGrad(blk_ptr, frame_width, blk_width, is_uv);
	}

	if (nei_avail[1]) //right
	{
		avg_grad += GetHorGrad(blk_ptr + MB_SIZE, frame_width, blk_width, is_uv);
	}

	if (nei_avail[2]) //top
	{
		avg_grad += GetVertGrad(blk_ptr, frame_width, MB_SIZE, is_uv);
	}

	if (nei_avail[3]) //bottom
	{
		avg_grad += GetVertGrad(blk_ptr + blk_width*frame_width, frame_width, MB_SIZE, is_uv);
	}

	if (avail_num)
	{
		avg_grad /= avail_num;
		avg_grad >>= shift;
	}

	return avg_grad;
}

#define THRES_MOTION(nei_mb_mode_ptr, mv_amp, intra_num, inter4v_num)\
{\
	if (nei_mb_mode_ptr->dctMd < INTER)\
	{\
		intra_num++;\
	}else\
	{\
		mv_amp += ABS (nei_mb_mode_ptr->mv[0].x);\
		mv_amp += ABS (nei_mb_mode_ptr->mv[0].y);\
		if (INTER4V == nei_mb_mode_ptr->dctMd)\
		{\
			inter4v_num++;\
		}\
	}\
}
	
LOCAL int32 GetThresMotion (DEC_VOP_MODE_T *vop_mode_ptr, DEC_MB_MODE_T *mb_mode_ptr, int32 nei_avail[])
{
	int32 intra_num = 0, inter4v_num = 0;
	int32 mv_amp = 0;
	int32 thres_mv;
	int32 thres_motion;

	if (vop_mode_ptr->VopPredType != IVOP)
	{
		if (nei_avail[0]) //left
		{
			THRES_MOTION ((mb_mode_ptr - 1), mv_amp, intra_num, inter4v_num);
		}

//		if (nei_avail[1]) //right
//		{
//			THRES_MOTION ((mb_mode_ptr + 1), mv_amp, intra_num, inter4v_num);
//		}

		if (nei_avail[2]) //top
		{
			THRES_MOTION ((mb_mode_ptr - vop_mode_ptr->MBNumX), mv_amp, intra_num, inter4v_num);
		}

//		if (nei_avail[3]) //bottom
//		{
//			THRES_MOTION ((mb_mode_ptr + vop_mode_ptr->MBNumX), mv_amp, intra_num, inter4v_num);
//		}

		thres_mv = (mv_amp > 10) ? 10 : mv_amp;

		if (intra_num)
		{
			thres_motion = 25;
		}else if (inter4v_num)
		{
			thres_motion = 10 + thres_mv;
		}else
		{
			thres_motion = thres_mv;
		}
	}else
	{
		thres_motion = 0;
	}
	
	return thres_motion;	
}

LOCAL BOOLEAN DetectErrBMA (DEC_VOP_MODE_T *vop_mode_ptr, DEC_MB_MODE_T *mb_mode_ptr)
{
	int32 mb_x, mb_y;
	int32 avg_grad = 0;
	int32 nei_avail[4], avail_num = 0;
	int32 thres_qp = 0, thres_motion = 0, threshold = 0;
	int32 is_error = FALSE;
// 	int32 frame_width = vop_mode_ptr->FrameWidth;
// 	int32 curr_mb_addr = 0;
	int32 curr_mb_num = vop_mode_ptr->mbnumDec;
// 	uint8 *mb_y_ptr = PNULL;

	mb_x = vop_mode_ptr->mb_x;
	mb_y = vop_mode_ptr->mb_y;

	//judge neighbor's availability
	//left
	nei_avail[0] = ((mb_x > 0) && (vop_mode_ptr->mbdec_stat_ptr[curr_mb_num -1] >= DECODED_IN_ERR_PKT));
	if (nei_avail[0])
	{
		avail_num++;
	}

	//right
	nei_avail[1] = ((mb_x < (vop_mode_ptr->MBNumX - 1)) && 
		(vop_mode_ptr->mbdec_stat_ptr[curr_mb_num+1] >= DECODED_IN_ERR_PKT));
	if (nei_avail[1])
	{
		avail_num++;
	}

	//top
	nei_avail[2] = ((mb_y > 0) && (vop_mode_ptr->mbdec_stat_ptr[curr_mb_num - vop_mode_ptr->MBNumX] >= DECODED_IN_ERR_PKT));
	if (nei_avail[2])
	{
		avail_num++;
	}

	//bottom
	nei_avail[3] = ((mb_y < (vop_mode_ptr->MBNumY-1)) 
		&& (vop_mode_ptr->mbdec_stat_ptr[curr_mb_num + vop_mode_ptr->MBNumX]) >= DECODED_IN_ERR_PKT);
	if (nei_avail[3])
	{
		avail_num++;
	}

	//get average gradient, luma
	avg_grad = GetAvgGrad(vop_mode_ptr, nei_avail, avail_num, 0);

	//get motion threshold
	if (PVOP == vop_mode_ptr->pre_vop_type)
	{
		thres_motion = GetThresMotion (vop_mode_ptr, mb_mode_ptr, nei_avail);
	}

	//get qp threshold
	thres_qp = (vop_mode_ptr->StepSize/* << 1*/);

	threshold = thres_qp + thres_motion;

	//compare
	if (avg_grad > threshold)
	{
		is_error = TRUE;
	}else{
		threshold >>= 1;

		//get average gradient, chroma U
		avg_grad = GetAvgGrad(vop_mode_ptr, nei_avail, avail_num, 1);

		if (avg_grad > threshold)
		{
			is_error = TRUE;
		}else
		{
			//get average gradient, chroma V
			avg_grad = GetAvgGrad(vop_mode_ptr, nei_avail, avail_num, 2);

			if (avg_grad > threshold)
			{
				is_error = TRUE;
			}
		}

	}
		
	return is_error;
}

#if 0

LOCAL int32 s_sx[3][3] = {{-1, 0, 1}, {-2, 0, 2}, {-1, 0, 1}};
LOCAL int32 s_sy[3][3] = {{1, 2, 1}, {0, 0, 0}, {-1, -2, -1}};

LOCAL void sobel_operation (uint8 *pix, int32 frame_width, int32 grad[], int32 dir[], int32 idx, int32 mb_x, int32 mb_y)
{
	int32 gx = 0, gy = 0;
	int32 i, j;
	int32 thres_grad = 100;
	int32 x, y;
	int32 max_x = frame_width - 1;

	for ( i = -1 ; i < 2; i++)
	{
		for (j = -1; j < 2; j++)
		{
			y = i; 
			x = mb_x * MB_SIZE + idx + j;

			if (x > max_x)
			{
				x = max_x;
			}

			gx += s_sx[i+1][j+1] * pix[y*frame_width+x];
			gy += s_sy[i+1][j+1] * pix[y*frame_width+x];
		}
	}

	grad[idx] = ABS(gx) + ABS(gy);

	if ((grad[idx] > thres_grad) && (gy))
	{
		if (gx*gy > 0)
		{
			dir[idx] = ((-16*gx)/gy);
		}else
		{
			dir[idx] = 32 + ((16*gx)/gy);
		}
	}

	return;
}
#endif

#if 0
LOCAL void sobel_grad (DEC_VOP_MODE_T *vop_mode_ptr, int32 is_top)
{
	int32 i;
	uint8 *pix;
	int32 frame_width = vop_mode_ptr->FrameWidth;
	int32 mb_x = vop_mode_ptr->mb_x;
	int32 mb_y = vop_mode_ptr->mb_y;
	int32 *grad, *dir;
	int32 avg_grad = 0, var_grad = 0;
	uint8 *curr_line = vop_mode_ptr->pCurRecFrame->pDecFrame->imgY + mb_y * MB_SIZE * frame_width;
	
	if (is_top)
	{
		grad = vop_mode_ptr->top_grad;
		dir  = vop_mode_ptr->top_dir;

		pix = curr_line - 2 * frame_width;
	}else
	{
		grad = vop_mode_ptr->bot_grad;
		dir  = vop_mode_ptr->bot_dir;

		pix = (curr_line + (MB_SIZE + 1) * frame_width);
	}

	//sobel gradient
	for (i = 0; i < MB_SIZE; i++)
	{
		grad[i] = 0;
		sobel_operation (pix, frame_width, grad, dir, i, mb_x, mb_y);
	}

	//avg_grad
	for (i = 0; i < MB_SIZE; i++)
	{
		avg_grad += grad[i];
	}
	avg_grad >>= 4;

	//var_grad
	for (i = 0; i < MB_SIZE; i++)
	{
		var_grad += ((avg_grad - grad[i]) * (avg_grad - grad[i]));	
	}
	var_grad >>= 4;

	if (is_top)
	{
		vop_mode_ptr->top_avg_grad = avg_grad;
		vop_mode_ptr->top_var_grad = var_grad;
	}else
	{
		vop_mode_ptr->bot_avg_grad = avg_grad;
		vop_mode_ptr->bot_var_grad = var_grad;
	}
}
#endif

#if 0
LOCAL int32 nei_mb_type (int32 grad[], int32 dir[])
{
	int32 i;
	int32 type; //0: smooth, 1: much texture, 2: edge
	int32 edge_cand[16];
	int32 valid_num = 0;

	for (i = 0; i < 16; i++)
	{
		edge_cand[i] = 0;

		if (ABS(dir[i]) < 48)
		{
			if (grad[i] > 100)
			{
				edge_cand[i] = TRUE;
			}
		}
	}

	if (edge_cand[0] && edge_cand[1])
	{
		if (ABS(dir[0] -dir[1]) > 16) //different direction
		{
			edge_cand[0] = FALSE;
		}
	}

	for (i = 0; i < 15; i++)
	{
		if (edge_cand[i] && edge_cand[i+1])
		{
			if ((ABS(dir[i] - dir[i+1]) <= 16) || (ABS(dir[i-1] - dir[i]) <= 16))
			{
				edge_cand[i] = TRUE;
			}else
			{
				edge_cand[i] = FALSE;
			}
		}
	}

	if (edge_cand[14] && edge_cand[15])
	{
		if (ABS(dir[14] -dir[15]) > 16) //different direction
		{
			edge_cand[14] = FALSE;
		}
	}

	for (i = 0; i < 15; i++)
	{
		if (edge_cand[i] && edge_cand[i+1])
		{
			valid_num++;
		}
	}

	if (valid_num)
	{
		type = 2;
	}else
	{
		type = 0;
	}

	return type;
}
#endif

#define MAX_GRAD(D, DIR, GRAD)\
{	int32 i;\
	D = GRAD[0]; DIR = 0;\
	for (i = 0; i < 16; i++)\
	{\
		if (GRAD[i] > D)\
		{\
			D = GRAD[i]; DIR = i;\
		}\
	}\
	GRAD[DIR] = 0;\
}

#if 0
LOCAL int32 mb_edge_dir (int32 mb_x, int32 mb_y, int32 grad[], int32 dir[])
{
// 	int32 pix_dir = 0;
	int32 edge_dir = 0;
	int32 D1, D2, D3;  //D1: first max, D2: second max, D3, third max
	int32 dir1, dir2, dir3;
	int32 thres_grad = 100;

 	MAX_GRAD (D1, dir1, grad);
 	MAX_GRAD (D2, dir2, grad);
 	MAX_GRAD (D3, dir3, grad);

	if (D1 > thres_grad)
	{
		if (ABS(dir1 - dir2) < 2)
		{
			edge_dir = (dir1 * D2 + dir2 * D1)/(D1+D2);
		}else
		{
			edge_dir = dir1;
		}

		return dir[edge_dir];
	}else
	{
		return 0;
	}
}
#endif

LOCAL int32 GetHorMAD_BMA (uint8 *left_pix_ptr, uint8 *ref_frame_y_ptr, int32 y_ref, int32 x_ref, int32 frame_width, int32 frame_height, int32 blk_size)
{
	int32 i;
	int32 dx, dy;
	int32 x_ref0, x_ref1;
	int32 y_ref0, y_ref1;
	int32 y_cor;
	uint8 pix_rec, pix_ref;
	int32 mad_bma = 0;

	dx = x_ref & 1;
	dy = y_ref & 1;

	x_ref >>= 1;
	y_ref >>= 1;

	//clip x cordination, two parameter for half pixel in horizontal direction
	x_ref0 = IClip(0, frame_width-1, x_ref);
	x_ref1 = IClip(0, frame_width-1, x_ref+1);

	for (i = 0; i < blk_size; i++)
	{
		y_cor = y_ref + i;

		y_ref0 = IClip(0, frame_height-1, y_cor);
		
		if (dy == 0)
		{
			if (dx == 0) //
			{
				pix_ref = ref_frame_y_ptr[y_ref0*frame_width+x_ref0];
			}else
			{
				pix_ref = ((ref_frame_y_ptr[y_ref0*frame_width+x_ref0] + 
					        ref_frame_y_ptr[y_ref0*frame_width+x_ref1] + 1) >> 1);
			}
		}else
		{
			y_ref1 = IClip(0, frame_height-1, y_cor+1);

			if (dx == 0) //
			{
				pix_ref = ((ref_frame_y_ptr[y_ref0*frame_width+x_ref0] + 
							ref_frame_y_ptr[y_ref1*frame_width+x_ref0] + 1) >>1);
			}else
			{
				pix_ref = ((ref_frame_y_ptr[y_ref0*frame_width+x_ref0] + 
							ref_frame_y_ptr[y_ref0*frame_width+x_ref1] + 
							ref_frame_y_ptr[y_ref1*frame_width+x_ref0] +
							ref_frame_y_ptr[y_ref1*frame_width+x_ref1] + 2) >> 2);
			}
		}

		pix_rec = left_pix_ptr[0];

		mad_bma += ABS(pix_rec - pix_ref);

		left_pix_ptr += frame_width;
	}

	return mad_bma;
}

LOCAL int32 GetVertMAD_BMA (uint8 *curr_pix_ptr, uint8 *ref_frame_y_ptr, int32 y_ref, int32 x_ref, int32 frame_width, int32 frame_height, int32 blk_size)
{
	int32 i;
	int32 dx, dy;
	int32 x_cor;
	int32 x_ref0, x_ref1;
	int32 y_ref0, y_ref1;
	uint8 pix_rec, pix_ref;
	int32 mad_bma = 0;
// 	int32 x_max = frame_width - 1;

	dx = x_ref & 1;
	dy = y_ref & 1;

	x_ref >>= 1;
	y_ref >>= 1;

	//clip y cordination, two parameter for half pixel in vertical direction
	y_ref0 = IClip(0, frame_height -1, y_ref);
	y_ref1 = IClip(0, frame_height -1, y_ref+1);

	for (i = 0; i < blk_size; i++)
	{
		x_cor = x_ref + i;

		x_ref0 = IClip(0, frame_width-1, x_cor);

		if (dx == 0)
		{
			if (dy == 0)
			{
				pix_ref = ref_frame_y_ptr[y_ref0*frame_width + x_ref0];
			}else
			{
				pix_ref = ((ref_frame_y_ptr[y_ref0*frame_width + x_ref0] + 
						    ref_frame_y_ptr[y_ref1*frame_width + x_ref0] + 1) >> 1);
			}
		}else
		{
			x_ref1 = IClip(0, frame_width-1, x_cor + 1);

			if (dy == 0)
			{
				pix_ref = ((ref_frame_y_ptr[y_ref0*frame_width + x_ref0] + 
						    ref_frame_y_ptr[y_ref0*frame_width + x_ref1] + 1) >> 1);
			}else
			{
				pix_ref = ((ref_frame_y_ptr[y_ref0*frame_width+x_ref0] + 
							ref_frame_y_ptr[y_ref0*frame_width+x_ref1] + 
							ref_frame_y_ptr[y_ref1*frame_width+x_ref0] +
							ref_frame_y_ptr[y_ref1*frame_width+x_ref1] + 2) >> 2);
			}
		}
		
		pix_rec = curr_pix_ptr[i];

		mad_bma += ABS(pix_rec - pix_ref);
	}

	return mad_bma;
}


LOCAL int32 GetMAD_OBMA (DEC_VOP_MODE_T *vop_mode_ptr, MOTION_VECTOR_T *mv_ptr, int32 nei_avail, int32 blk_size, int32 blk_idx)
{
	int32 mb_x = vop_mode_ptr->mb_x;
	int32 mb_y = vop_mode_ptr->mb_y;
	int32 mv_x = mv_ptr->x;	//use integer pixels
	int32 mv_y = mv_ptr->y;
	int32 mad_bma = 0;
	int32 x_ref, y_ref;
	int32 blk_x_offet, blk_y_offset;
	int32 curr_blk_addr;
	uint8 *curr_blk_ptr;
	int32 frame_width = vop_mode_ptr->FrameWidth;
	int32 frame_height = vop_mode_ptr->FrameHeight;
	uint8 *ref_frame_y_ptr  = vop_mode_ptr->pBckRefFrame->pDecFrame->imgY;
	uint8 *curr_frame_y_ptr = vop_mode_ptr->pCurRecFrame->pDecFrame->imgY;

	blk_x_offet  = (blk_idx & 1 ) * BLOCK_SIZE;
	blk_y_offset = (blk_idx >> 1) * BLOCK_SIZE;

	x_ref = mb_x * MB_SIZE * 2 + blk_x_offet * 2 + mv_x;  //integer resolution
	y_ref = mb_y * MB_SIZE * 2 + blk_y_offset * 2 + mv_y;

	curr_blk_addr = (mb_y * MB_SIZE + blk_y_offset) * frame_width + mb_x * MB_SIZE + blk_x_offet;
	curr_blk_ptr = curr_frame_y_ptr + curr_blk_addr;
	
	//left mad (obma)
	if (nei_avail & 1)
	{
		uint8 *left_pix_ptr = curr_blk_ptr - 1;

		mad_bma += GetHorMAD_BMA (left_pix_ptr, ref_frame_y_ptr, y_ref, x_ref - 2, frame_width, frame_height, blk_size);
	}

	//top mad (obma)
	if (nei_avail & 2)
	{
		uint8 *top_pix_ptr = curr_blk_ptr - frame_width;

		mad_bma += GetVertMAD_BMA (top_pix_ptr, ref_frame_y_ptr, y_ref - 2, x_ref, frame_width, frame_height, blk_size);
	}

	//bottom mad (obma)
	if (nei_avail & 8)
	{
		uint8 *bot_pix_ptr = curr_blk_ptr + frame_width * blk_size;

		mad_bma += GetVertMAD_BMA (bot_pix_ptr, ref_frame_y_ptr, y_ref + (blk_size<<1), x_ref, frame_width, frame_height, blk_size);
	}

	return mad_bma;
}

#if defined(DO_MC_CHECK)
LOCAL BOOLEAN MC_ConcealMB_Check (DEC_VOP_MODE_T *vop_mode_ptr, DEC_MB_MODE_T *co_mb_mode_ptr)
{
	int32 mb_x, mb_y;
	int32 mad = 0, thres = 512, gain = 1;
	int32 top_avail = FALSE, bot_avail = FALSE;
	int32 nei_avail = 0;

	mb_x = vop_mode_ptr->mb_x;
	mb_y = vop_mode_ptr->mb_y;

	top_avail = (mb_y > 0);
	bot_avail = (mb_y < (vop_mode_ptr->MBNumY - 1));
	nei_avail = (bot_avail << 3) | (top_avail << 1);

	if (top_avail)
	{
		sobel_grad (vop_mode_ptr, 1);
	}

	if (bot_avail)
	{
		sobel_grad (vop_mode_ptr, 0);
	}

	gain = ((vop_mode_ptr->top_var_grad + vop_mode_ptr->bot_avg_grad)/1000);

	if (INTER4V == co_mb_mode_ptr->dctMd)
	{
		int32 blk_idx;

		for (blk_idx = 0; blk_idx < 4; blk_idx++)
		{
			mad += GetMAD_OBMA (vop_mode_ptr, &(co_mb_mode_ptr->mv[blk_idx]), nei_avail, BLOCK_SIZE, blk_idx);
		}
	}else
	{
		mad += GetMAD_OBMA (vop_mode_ptr, &(co_mb_mode_ptr->mv[0]), nei_avail, MB_SIZE, 0);
	}

	thres = (gain) ? (thres * gain) : (thres);

	if (mad > thres)
	{
		//check left neighbor's mb has strong edge or not
		if (mb_x)
		{
			int32 i;
			int32 grad[16], dir[16];
			int32 left_mb_type = 0;
			int32 frame_width = vop_mode_ptr->FrameWidth;
			uint8 *pix = vop_mode_ptr->pCurRecFrame->pDecFrame->imgY + mb_y * MB_SIZE * frame_width;
			
			grad[0] = grad[15] = 0;
			dir[0] = dir[15] = 0;

			for (i = 0; i < 15; i++)
			{
				grad[i] = 0;
				sobel_operation(pix, frame_width, grad, dir, 14, mb_x-1, mb_y);

				grad[i] = grad[14];
				dir[i]  = dir[14];

				pix += frame_width;
			}

			left_mb_type = nei_mb_type (grad, dir);

			if (left_mb_type == 2) //strong edge
			{
				return TRUE;
			}else
			{
				return FALSE;
			}
		}else
		{
			return FALSE;
		}
	}

	return TRUE;
}

//directional spatial interpolation
LOCAL void DI_SpatialConceal_Y (DEC_VOP_MODE_T *vop_mode_ptr, DEC_MB_MODE_T *mb_mode_ptr, int32 dir_top, int32 dir_bot)
{
	int32 i, j;
	uint8 pred_t[256], pred_b[256], *pred;
	int32 x1, x2;
	int32 mb_x = vop_mode_ptr->mb_x;
	int32 mb_y = vop_mode_ptr->mb_y;
	int32 frame_width = vop_mode_ptr->FrameWidth;
	uint8 *curr_pix_line = vop_mode_ptr->pCurRecFrame->pDecFrame->imgY + mb_y * MB_SIZE * frame_width;
	uint8 *top_pix_line = curr_pix_line - frame_width;
	uint8 *bot_pix_line = curr_pix_line + MB_SIZE * frame_width; 
	uint8 *curr_blk_ptr = curr_pix_line + MB_SIZE * mb_x;
	uint8 top, bot, tmp, cnt = 0;

	if (mb_y == 0)
	{
		top_pix_line = bot_pix_line + dir_bot;
	}else if (mb_y == (vop_mode_ptr->MBNumY - 1))
	{
		bot_pix_line = top_pix_line;
	}

	if (vop_mode_ptr->mbdec_stat_ptr[(mb_y+1)*vop_mode_ptr->MBNumX+mb_x] == NOT_DECODED)
	{
		bot_pix_line = top_pix_line;
	}

	SCI_MEMSET(pred_t, 0, 256);
	SCI_MEMSET(pred_b, 0, 256);

	pred = pred_t;
	for (j = 0; j < 16; j++)
	{
		x1 = (dir_top*j/16) + MB_SIZE*mb_x;
		x2 = x1 - dir_top;

		x1 = IClip(0, frame_width-1, x1);
		x2 = IClip(0, frame_width-1, x2);

		for (i = 0; i < 16; i++)
		{
			*pred++ = IClip(0, 255, ((top_pix_line[x1]*(16-j) + bot_pix_line[x2]*j)>>4));

			x1++; x2++;

			x1 = IClip(0, frame_width-1, x1);
			x2 = IClip(0, frame_width-1, x2);
		}
	}

	pred = pred_b;
	for (j = 0; j < 16; j++)
	{
		x1 = (dir_bot*j/16) + MB_SIZE*mb_x;
		x2 = x1 - dir_bot;

		x1 = IClip(0, frame_width-1, x1);
		x2 = IClip(0, frame_width-1, x2);

		for (i = 0; i < 16; i++)
		{
			*pred++ = IClip(0, 255, ((top_pix_line[x1]*(16-j) + bot_pix_line[x2]*j)>>4));

			x1++; x2++;

			x1 = IClip(0, frame_width-1, x1);
			x2 = IClip(0, frame_width-1, x2);
		}
	}

	for (i = 0; i < MB_SIZE; i++)
	{
		for (j = 0; j < MB_SIZE; j++)
		{
			top = pred_t[cnt];
			bot = pred_b[cnt++];

			if (top && bot)
			{
				tmp = ((top + bot) >> 1);
			}else if (bot)
			{
				tmp = bot;
			}else if (top)
			{
				tmp = top;
			}else
			{
				tmp = 0;
			}

			curr_blk_ptr[j] = tmp;
		}

		curr_blk_ptr += frame_width;
	}
}

LOCAL void DI_SpatialConceal_UV (DEC_VOP_MODE_T *vop_mode_ptr, DEC_MB_MODE_T *mb_mode_ptr, int32 dir_top, int32 dir_bot)
{
	int32 i, j;
	uint8 u_pred_t[64], u_pred_b[64], *u_pred;
	uint8 v_pred_t[64], v_pred_b[64], *v_pred;
	int32 x1, x2;
	int32 mb_x = vop_mode_ptr->mb_x;
	int32 mb_y = vop_mode_ptr->mb_y;
	int32 frame_width_c = (vop_mode_ptr->FrameWidth>>1);
	uint8 *u_curr_pix_line = vop_mode_ptr->pCurRecFrame->pDecFrame->imgU + mb_y*BLOCK_SIZE*frame_width_c;
	uint8 *v_curr_pix_line = vop_mode_ptr->pCurRecFrame->pDecFrame->imgV + mb_y*BLOCK_SIZE*frame_width_c;
	uint8 *u_top_pix_line = u_curr_pix_line - frame_width_c;
	uint8 *v_top_pix_line = v_curr_pix_line - frame_width_c;
	uint8 *u_bot_pix_line = u_curr_pix_line + BLOCK_SIZE*frame_width_c;
	uint8 *v_bot_pix_line = v_curr_pix_line + BLOCK_SIZE*frame_width_c;
	uint8 *u_curr_blk_ptr = u_curr_pix_line + mb_x * BLOCK_SIZE;
	uint8 *v_curr_blk_ptr = v_curr_pix_line + mb_x * BLOCK_SIZE;
	uint8 top, bot, tmp, cnt = 0;

	if (mb_y == 0)
	{
		u_top_pix_line = u_bot_pix_line;
		v_top_pix_line = v_bot_pix_line;
	}else if (mb_y == (vop_mode_ptr->MBNumY -1))
	{
		u_bot_pix_line = u_top_pix_line;
		v_bot_pix_line = v_top_pix_line;
	}

	if (vop_mode_ptr->mbdec_stat_ptr[(mb_y+1)*vop_mode_ptr->MBNumX+mb_x] == NOT_DECODED)
	{
		u_bot_pix_line = u_top_pix_line;
		v_bot_pix_line = v_top_pix_line;
	}

	SCI_MEMSET(u_pred_t, 0, 64);
	SCI_MEMSET(v_pred_t, 0, 64);
	SCI_MEMSET(u_pred_b, 0, 64);
	SCI_MEMSET(v_pred_b, 0, 64);
	
	u_pred = u_pred_t;
	v_pred = v_pred_t;
	for (j = 0; j < BLOCK_SIZE; j++)
	{
		x1 = (-dir_top*j/BLOCK_SIZE) + BLOCK_SIZE*mb_x;
		x2 = x1 + dir_top;

		x1 = IClip(0, frame_width_c-1, x1);
		x2 = IClip(0, frame_width_c-1, x2);

		for (i = 0; i < BLOCK_SIZE; i++)
		{
			*u_pred++ = IClip(0, 255, ((u_top_pix_line[x1]*(BLOCK_SIZE-j) + u_bot_pix_line[x2]*j)>>3));
			*v_pred++ = IClip(0, 255, ((v_top_pix_line[x1]*(BLOCK_SIZE-j) + v_bot_pix_line[x2]*j)>>3));

			x1++; x2++;
			
			x1 = IClip(0, frame_width_c-1, x1);
			x2 = IClip(0, frame_width_c-1, x2);
		}
	}

	u_pred = u_pred_b;
	v_pred = v_pred_b;
	for (j = 0; j < BLOCK_SIZE; j++)
	{
		x1 = (dir_bot*j/BLOCK_SIZE) + BLOCK_SIZE*mb_x;
		x2 = x1 - dir_bot;

		x1 = IClip(0, frame_width_c-1, x1);
		x2 = IClip(0, frame_width_c-1, x2);

		for (i = 0; i < BLOCK_SIZE; i++)
		{
			*u_pred++ = IClip(0, 255, ((u_top_pix_line[x1]*(BLOCK_SIZE-j) + u_bot_pix_line[x2]*j)>>3));
			*v_pred++ = IClip(0, 255, ((v_top_pix_line[x1]*(BLOCK_SIZE-j) + v_bot_pix_line[x2]*j)>>3));

			x1++; x2++;
			
			x1 = IClip(0, frame_width_c-1, x1);
			x2 = IClip(0, frame_width_c-1, x2);
		}
	}

	for (i = 0; i < BLOCK_SIZE; i++)
	{
		for (j = 0; j < BLOCK_SIZE; j++)
		{
			//u
			top = u_pred_t[cnt];
			bot = u_pred_b[cnt];

			if (top && bot)
			{
				tmp = ((top + bot) >> 1);
			}else if (bot)
			{
				tmp = bot;
			}else if (top)
			{
				tmp = top;
			}else
			{
				tmp = 0;
			}

			u_curr_blk_ptr[j] = tmp;

			//v
			top = v_pred_t[cnt];
			bot = v_pred_b[cnt++];

			if (top && bot)
			{
				tmp = ((top + bot) >> 1);
			}else if (bot)
			{
				tmp = bot;
			}else if (top)
			{
				tmp = top;
			}else
			{
				tmp = 0;
			}

			v_curr_blk_ptr[j] = tmp;
		}

		u_curr_blk_ptr += frame_width_c;
		v_curr_blk_ptr += frame_width_c;
	}	
}

LOCAL void SpatialConceal (DEC_VOP_MODE_T *vop_mode_ptr, DEC_MB_MODE_T *mb_mode_ptr)
{
 	int32 mb_x = vop_mode_ptr->mb_x;
	int32 mb_y = vop_mode_ptr->mb_y;
	int32 dir_top = 0, dir_bot = 0;
	int32 type_top = 0, type_bot = 0;

	if (mb_y)
	{
		type_top = nei_mb_type (vop_mode_ptr->top_grad, vop_mode_ptr->top_dir);
	}

	if (mb_y < (vop_mode_ptr->MBNumY -1))
	{
		type_bot = nei_mb_type (vop_mode_ptr->bot_grad, vop_mode_ptr->bot_dir); 
	}

	if (type_top < 2)
	{
		dir_top = 0;
	}else
	{
		dir_top = mb_edge_dir (mb_x, mb_y, vop_mode_ptr->top_grad, vop_mode_ptr->top_dir);
	}

	if (type_bot < 2)
	{
		dir_bot = 0;
	}else
	{
		dir_bot = mb_edge_dir (mb_x, mb_y, vop_mode_ptr->bot_grad, vop_mode_ptr->bot_dir);
	}

	DI_SpatialConceal_Y (vop_mode_ptr, mb_mode_ptr, dir_top, dir_bot);
	DI_SpatialConceal_UV (vop_mode_ptr, mb_mode_ptr, dir_top/2, dir_bot/2);
}
#endif

static /*inline*/ uint32 endian_conv(uint32 in)
{
    uint32 out;

	out =  (in << 24);
	out |= (in << 8) & 0x00ff0000;
	out |= (in >> 8) & 0x0000ff00;
	out |= (in >> 24);
      
    return out;
}

void MCABfr2RecFrm(DEC_VOP_MODE_T *vop_mode_ptr)
{
    int32 cmd, i;
	uint32 mb_x = vop_mode_ptr->mb_x;
	uint32 mb_y = vop_mode_ptr->mb_y;	
	uint32 frm_width = vop_mode_ptr->FrameWidth;
	uint32 mb_addr = (mb_y * frm_width + mb_x)<<4;
	uint32 *pMb_y = (uint32 *)(vop_mode_ptr->pCurRecFrame->pDecFrame->imgY + mb_addr);
#ifdef _VSP_LINUX_
       extern uint32 g_vsp_Vaddr_base;
	uint32 *pMcaBfr = (uint32*)(0x20c05000-VSP_DCAM_BASE+g_vsp_Vaddr_base);
#else
	uint32 *pMcaBfr = (uint32*)0x20c05000;
#endif
	
#if _CMODEL_
	pMcaBfr = (uint32*)vsp_fw_mca_out_bfr;
#endif

	VSP_READ_REG_POLL(VSP_MCA_REG_BASE+MCA_DEBUG_OFF, V_BIT_2|V_BIT_1|V_BIT_0, 0, TIME_OUT_CLK, "MCA_DEBUG: polling mca done status");

	cmd = (1<<4);		
	VSP_WRITE_REG(VSP_DCAM_REG_BASE+DCAM_CFG_OFF, cmd, "DCAM_CFG: allow software to access the mca bfr");

	VSP_READ_REG_POLL(VSP_DCAM_REG_BASE+DCAM_CFG_OFF, V_BIT_7, V_BIT_7, TIME_OUT_CLK, "DCAM_CFG: polling dcam clock status");

	//y
	for (i = 0; i < 16; i++)
	{
	#if !defined(CHIP_ENDIAN_LITTLE) //big endian
		*pMb_y++ = endian_conv(*pMcaBfr++);
		*pMb_y++ = endian_conv(*pMcaBfr++);
		*pMb_y++ = endian_conv(*pMcaBfr++);
		*pMb_y++ = endian_conv(*pMcaBfr++);
	#else
	    	*pMb_y++ = *pMcaBfr++;
	    	*pMb_y++ = *pMcaBfr++;
	    	*pMb_y++ = *pMcaBfr++;
	    	*pMb_y++ = *pMcaBfr++;
	#endif	
		pMb_y += ((frm_width>>2)-4);
	}

	if (vop_mode_ptr->uv_interleaved) //two plane
	{
		uint8 u_bfr[64], v_bfr[64];
		uint32 *pMb_uv = PNULL;
		uint8 *pu, *pv;
		uint32 *pIntU, *pIntV;
		uint8 u0, u1, v0, v1;
		int32 j;

		//copy mca bfr, because it only be accessed by word
		pIntU = (uint32 *)u_bfr;
		for (i = 0; i < BLOCK_SIZE; i++)
		{
			*pIntU++ = *pMcaBfr++;
			*pIntU++ = *pMcaBfr++;
		}
		pIntV = (uint32 *)v_bfr;
		for (i = 0; i < BLOCK_SIZE; i++)
		{
			*pIntV++ = *pMcaBfr++;
			*pIntV++ = *pMcaBfr++;
		}

		//uv-interleaved
		pMb_uv = (uint32 *)(vop_mode_ptr->pCurRecFrame->pDecFrame->imgU + mb_y * BLOCK_SIZE *frm_width + mb_x*MB_SIZE);
		pu = u_bfr; pv = v_bfr;
		for (i = 0; i < BLOCK_SIZE; i++)
		{
			for (j = 0; j < 4; j++)
			{
				u0 =*pu++; u1 = *pu++;
				v0 =*pv++; v1 = *pv++;

			#if !defined(CHIP_ENDIAN_LITTLE) //big endian
				*pMb_uv++ = (u0<<24)|(v0<<16)|(u1<<8)|(v1);
			#else
				*pMb_uv++ = (v1<<24)|(u1<<16)|(v0<<8)|(u0);
			#endif			
			}
			pMb_uv += ((frm_width>>2)-4);
		}
	}else //three plane
	{
		uint32 *pMb_u = PNULL, *pMb_v = PNULL;

		//u
		mb_addr = (mb_y * (frm_width>>1) + mb_x)<<3;
		pMb_u = (uint32 *)(vop_mode_ptr->pCurRecFrame->pDecFrame->imgU + mb_addr);
		for (i = 0; i < 8; i++)
		{
		#if !defined(CHIP_ENDIAN_LITTLE) //big endian
			*pMb_u++ =  endian_conv(*pMcaBfr++);
			*pMb_u++ =  endian_conv(*pMcaBfr++);	
		#else
		    	*pMb_u++ = *pMcaBfr++;
	    		*pMb_u++ = *pMcaBfr++;
		#endif	
			pMb_u += ((frm_width>>3)-2);
		}
		
		//v
		pMb_v = (uint32 *)(vop_mode_ptr->pCurRecFrame->pDecFrame->imgV + mb_addr);
		for (i = 0; i < 8; i++)
		{
		#if !defined(CHIP_ENDIAN_LITTLE) //big endian
			*pMb_v++ =  endian_conv(*pMcaBfr++);
			*pMb_v++ =  endian_conv(*pMcaBfr++);
		#else
			*pMb_v++ = *pMcaBfr++;
	    		*pMb_v++ = *pMcaBfr++;
		#endif	
			pMb_v += ((frm_width>>3)-2);
		}
	}
	
	cmd = (0<<4) | (1<<3);		//allow hardware to access the vsp buffer
	VSP_WRITE_REG(VSP_DCAM_REG_BASE+DCAM_CFG_OFF, cmd, "DCAM_CFG: allow hardware to access the mca bfr");

    VSP_READ_REG_POLL (VSP_DCAM_REG_BASE+DCAM_CFG_OFF, 0, 0, TIME_OUT_CLK, "DCAM_CFG: polling dcam clock status");
}

PUBLIC void Mp4Dec_EC_IVOP (DEC_VOP_MODE_T *vop_mode_ptr)
{
	int32 mb_x, mb_y;
	int32 curr_mb_dec_status = NOT_DECODED;
	int32 curr_mb_num = 0;
	int32 is_error = FALSE;
	int32 prev_pvop_valid = (PVOP == vop_mode_ptr->pre_vop_type) ? TRUE : FALSE;
// 	int32 mc_check_result = FALSE;
	int32 total_mb_num_y = vop_mode_ptr->MBNumY;
	int32 total_mb_num_x = vop_mode_ptr->MBNumX;
	DEC_MB_MODE_T *mb_mode_ptr = vop_mode_ptr->pMbMode;
	DEC_MB_MODE_T *co_mb_mode_ptr = vop_mode_ptr->pMbMode_prev;
	
	for (mb_y = 0; mb_y < total_mb_num_y; mb_y++)
	{
		vop_mode_ptr->mb_y = mb_y;

		for (mb_x = 0; mb_x < total_mb_num_x; mb_x++)
		{
			vop_mode_ptr->mb_x = mb_x;
			vop_mode_ptr->mbnumDec = curr_mb_num = mb_y * total_mb_num_x + mb_x;
			curr_mb_dec_status = vop_mode_ptr->mbdec_stat_ptr[curr_mb_num];

			if (curr_mb_dec_status < DECODED_NOT_IN_ERR_PKT) //not coded or coded but in error packet
			{
				mb_mode_ptr->bIntra = FALSE;
				mb_mode_ptr->CBP	= 0;

				if (NOT_DECODED == curr_mb_dec_status)
				{
					is_error = TRUE;
				}else
				{
					is_error = DetectErrBMA (vop_mode_ptr, mb_mode_ptr);

					if (is_error)
					{
						vop_mode_ptr->err_MB_num++;
					}
				}

				if (is_error) //then do error concealment
				{
				#if defined(DO_MC_CHECK)
					mc_check_result = FALSE;
				#endif

					if (prev_pvop_valid) //try temporal interpolation
					{
						co_mb_mode_ptr = vop_mode_ptr->pMbMode_prev + curr_mb_num;

						Mp4Dec_VspMBInit (mb_x, mb_y);

						vop_mode_ptr->VopPredType = PVOP; //here, for use MCA hardware module
						Mp4Dec_StartMcaOneDir (vop_mode_ptr, vop_mode_ptr->mb_cache_ptr, co_mb_mode_ptr->mv);
                        if(VSP_READ_REG_POLL(VSP_MCA_REG_BASE+MCA_DEBUG_OFF, V_BIT_2|V_BIT_1|V_BIT_0, 0, TIME_OUT_CLK, "MCA_DEBUG: polling mca done status"))
                        {
               				PRINTF("mca time out!\n");
                            vop_mode_ptr->VopPredType = IVOP;  //here, set it's original value
            				return;
						}
                        
						MCABfr2RecFrm(vop_mode_ptr);

						vop_mode_ptr->VopPredType = IVOP;  //here, set it's original value

						//check match or not
					#if defined(DO_MC_CHECK)
						mc_check_result = MC_ConcealMB_Check (vop_mode_ptr, co_mb_mode_ptr);
					#endif	
					}

				#if defined(DO_MC_CHECK)
					if (!mc_check_result)
					{
						SpatialConceal (vop_mode_ptr, mb_mode_ptr);
					}
				#endif
					
					vop_mode_ptr->mbdec_stat_ptr[curr_mb_num] = ERR_CONCEALED;
				}else
				{
					vop_mode_ptr->mbdec_stat_ptr[curr_mb_num] = DECODED_OK;
				}				
			}else
			{
				vop_mode_ptr->mbdec_stat_ptr[curr_mb_num] = DECODED_OK;
			}

			mb_mode_ptr++;
		}
	}

// 	if(bUseRecFrmAsDispFrm)
	{
		int32 size = vop_mode_ptr->FrameWidth*vop_mode_ptr->FrameHeight;

		memcpy(vop_mode_ptr->pCurDispFrame->pDecFrame->imgY, vop_mode_ptr->pCurRecFrame->pDecFrame->imgY, size);

		if (vop_mode_ptr->uv_interleaved)
		{
			memcpy(vop_mode_ptr->pCurDispFrame->pDecFrame->imgU, vop_mode_ptr->pCurRecFrame->pDecFrame->imgU, size/2);
		}else
		{
			memcpy(vop_mode_ptr->pCurDispFrame->pDecFrame->imgU, vop_mode_ptr->pCurRecFrame->pDecFrame->imgU, size/4);
			memcpy(vop_mode_ptr->pCurDispFrame->pDecFrame->imgV, vop_mode_ptr->pCurRecFrame->pDecFrame->imgV, size/4);
		}
	}
}

/*****************************************************************************
  neighbor	MB
		|  B1  |  C2
  ------|------|------
	    |  cur | 
	A0	|  MB  |		
  ------|------|------
        |   D3 |

  neighbor MB: left, top, top right and bottom MB
******************************************************************************/
LOCAL int32 GetNeiAvail (DEC_VOP_MODE_T *vop_mode_ptr, DEC_MB_MODE_T *mb_mode_ptr, int32 *nei_intra_num, int32 *nei_avail)
{
	int32 mb_x = vop_mode_ptr->mb_x;
	int32 mb_y = vop_mode_ptr->mb_y;
	int32 mb_num_x = vop_mode_ptr->MBNumX;
	int32 curr_mb_num = mb_y * mb_num_x + mb_x;
	int32 avail_num = 0;

	if (mb_y)
	{
		DEC_MB_MODE_T *top_mb_mode_ptr = mb_mode_ptr - mb_num_x;
		int32 top_avail = (vop_mode_ptr->mbdec_stat_ptr[curr_mb_num - mb_num_x] != NOT_DECODED);
		avail_num += top_avail;
		*nei_avail |= (top_avail << 1);

		if ((top_avail) && (top_mb_mode_ptr->bIntra))
		{
			*nei_intra_num++;
		}

		if (mb_x < mb_num_x -1)
		{
// 			DEC_MB_MODE_T *tr_mb_mode_ptr = top_mb_mode_ptr + 1; //top right
			int32 tr_avail = (vop_mode_ptr->mbdec_stat_ptr[curr_mb_num - mb_num_x + 1] != NOT_DECODED);
			avail_num += tr_avail;
			*nei_avail |= (tr_avail << 2);
		}
	}

	if (mb_x > 0)
	{
// 		DEC_MB_MODE_T *left_mb_mode_ptr = mb_mode_ptr - 1; //left
		int32 left_avail = (vop_mode_ptr->mbdec_stat_ptr[curr_mb_num - 1] != NOT_DECODED);

		avail_num += left_avail;
		*nei_avail |= (left_avail << 0);
	}

	if (mb_y < (vop_mode_ptr->MBNumY -1))
	{
		DEC_MB_MODE_T *bot_mb_mode_ptr = mb_mode_ptr - mb_num_x; //bottom
		int32 bot_avail = (vop_mode_ptr->mbdec_stat_ptr[curr_mb_num + mb_num_x] != NOT_DECODED);

		avail_num += bot_avail;
		*nei_avail |= (bot_avail << 3);

		if ((bot_avail) && (bot_mb_mode_ptr->bIntra))
		{
			*nei_intra_num++;
		}
	}

	return avail_num;
}

//only for uv-interleaved format
LOCAL void BI_SpatialConceal(DEC_VOP_MODE_T *vop_mode_ptr, DEC_MB_MODE_T *mb_mode_ptr, int32 nei_avail)
{
	int32 mb_x = vop_mode_ptr->mb_x;
	int32 mb_y = vop_mode_ptr->mb_y;
	int32 top_avail = ((nei_avail>>1) & 0x1);
	int32 bot_avail = ((nei_avail>>1) & 0x3);
	int32 frame_width = vop_mode_ptr->FrameWidth;
	uint8 *curr_pix_ptr_y = vop_mode_ptr->pCurRecFrame->pDecFrame->imgY + (mb_y * MB_SIZE) * frame_width + (mb_x * MB_SIZE);
	uint8 *curr_pix_ptr_uv = vop_mode_ptr->pCurRecFrame->pDecFrame->imgU + (mb_y * BLOCK_SIZE) * frame_width + (mb_x * MB_SIZE);
	uint8 *top_pix_ptr_y = curr_pix_ptr_y - frame_width;
	uint8 *top_pix_ptr_uv = curr_pix_ptr_uv - frame_width;
	uint8 *bot_pix_ptr_y = curr_pix_ptr_y + MB_SIZE * frame_width;
	uint8 *bot_pix_ptr_uv = curr_pix_ptr_uv + BLOCK_SIZE * frame_width;
	int32 i, j;

	if (!top_avail)
	{
		top_pix_ptr_y = bot_pix_ptr_y;
		top_pix_ptr_uv = bot_pix_ptr_uv;
	}

	if (!bot_avail)
	{
		bot_pix_ptr_y = top_pix_ptr_y;
		bot_pix_ptr_uv = top_pix_ptr_uv;
	}

	//y
	for (i = 0; i < MB_SIZE; i++)
	{
		uint8 top_y, bot_y;

		for (j = 0; j < MB_SIZE; j++)
		{
			top_y = top_pix_ptr_y[j];
			bot_y = bot_pix_ptr_y[j];

			curr_pix_ptr_y[j] = ((MB_SIZE - i) * top_y + i * bot_y) >> 4;
		}

		curr_pix_ptr_y += frame_width;
	}

	//u and v
	for (i = 0; i < BLOCK_SIZE; i++)
	{
		uint8 top_uv, bot_uv;

		for (j = 0; j < MB_SIZE; j++)
		{
			top_uv = top_pix_ptr_uv[j];
			bot_uv = bot_pix_ptr_uv[j];

			curr_pix_ptr_uv[j] = ((BLOCK_SIZE - i) * top_uv + i * bot_uv) >> 3;
		}

		curr_pix_ptr_uv += frame_width;
	}
}

LOCAL void TemporalConceal (DEC_VOP_MODE_T *vop_mode_ptr, DEC_MB_MODE_T *curr_mb_mode_ptr, int32 nei_avail)
{
	MOTION_VECTOR_T zero_mv = {0, 0};
	MOTION_VECTOR_T mv_cand = zero_mv;
	int32 min_mad_obma = GetMAD_OBMA (vop_mode_ptr, &zero_mv, nei_avail, MB_SIZE, 0);
	int32 left_avail = nei_avail & 0x1;
	int32 top_avail  = ((nei_avail>>1) & 0x1);
	int32 tr_avail   = ((nei_avail>>2) & 0x1);
	int32 bot_avail  = ((nei_avail>>3) & 0x1);
	DEC_MB_MODE_T *left_mb_mode_ptr = curr_mb_mode_ptr -1;
	DEC_MB_MODE_T *top_mb_mode_ptr = curr_mb_mode_ptr - vop_mode_ptr->MBNumX;
	DEC_MB_MODE_T *tr_mb_mode_ptr = top_mb_mode_ptr + 1;
	DEC_MB_MODE_T *bot_mb_mode_ptr = curr_mb_mode_ptr + vop_mode_ptr->MBNumX;
	DEC_MB_MODE_T *co_mb_mode_ptr = vop_mode_ptr->pMbMode_prev+vop_mode_ptr->mb_y * vop_mode_ptr->MBNumX+vop_mode_ptr->mb_x;
	int32 mad_obma = 0;

	//left mad
	if (left_avail && !(left_mb_mode_ptr->bIntra))
	{
		mad_obma = GetMAD_OBMA (vop_mode_ptr, &left_mb_mode_ptr->mv[1], nei_avail, MB_SIZE, 0);

		if (mad_obma < min_mad_obma)
		{
			min_mad_obma = mad_obma;
			mv_cand = left_mb_mode_ptr->mv[1];
		}
	}

	//top mad
	if (top_avail && !top_mb_mode_ptr->bIntra)
	{
		mad_obma = GetMAD_OBMA (vop_mode_ptr, &top_mb_mode_ptr->mv[2], nei_avail, MB_SIZE, 0);

		if (mad_obma < min_mad_obma)
		{
			min_mad_obma = mad_obma;
			mv_cand = top_mb_mode_ptr->mv[2];
		}
	}

	//bot mad
	if (bot_avail && !bot_mb_mode_ptr->bIntra)
	{
		mad_obma = GetMAD_OBMA (vop_mode_ptr, &bot_mb_mode_ptr->mv[0], nei_avail, MB_SIZE, 0);

		if (mad_obma < min_mad_obma)
		{
			min_mad_obma = mad_obma;
			mv_cand = bot_mb_mode_ptr->mv[0];
		}
	}

	//top right mad
	if (tr_avail && !tr_mb_mode_ptr->bIntra)
	{
		mad_obma = GetMAD_OBMA (vop_mode_ptr, &tr_mb_mode_ptr->mv[2], nei_avail, MB_SIZE, 0);

		if (mad_obma < min_mad_obma)
		{
			min_mad_obma = mad_obma;
			mv_cand = tr_mb_mode_ptr->mv[2];
		}
	}

	if (!co_mb_mode_ptr->bIntra)
	{
		mad_obma = GetMAD_OBMA (vop_mode_ptr, &co_mb_mode_ptr->mv[0], nei_avail, MB_SIZE, 0);

		if (mad_obma < min_mad_obma)
		{
			min_mad_obma = mad_obma;
			mv_cand = co_mb_mode_ptr->mv[0];
		}
	}

	Mp4Dec_StartMcaOneDir (vop_mode_ptr, vop_mode_ptr->mb_cache_ptr, &mv_cand);

	return;
}

PUBLIC void Mp4Dec_EC_PVOP (DEC_VOP_MODE_T *vop_mode_ptr)
{
	int32 mb_x, mb_y;
	int32 curr_mb_dec_status = NOT_DECODED; 
	int32 curr_mb_num = 0;
	int32 is_error = FALSE;
	int32 total_mb_num_x = vop_mode_ptr->MBNumX;
	int32 need_load_data = TRUE;
	int32 bUseRecFrmAsDispFrm = FALSE;
	DEC_MB_MODE_T *mb_mode_ptr = vop_mode_ptr->pMbMode;

	vop_mode_ptr->mb_cache_ptr->mca_type = MCA_BACKWARD;

	for (mb_y = 0; mb_y < vop_mode_ptr->MBNumY; mb_y++)
	{
		vop_mode_ptr->mb_y = mb_y;

		for (mb_x = 0; mb_x < vop_mode_ptr->MBNumX; mb_x++)
		{
			int32 nei_intra_num = 0;
			int32 nei_avail = 0;
			int32 avail_num = 0;

			vop_mode_ptr->mb_x = mb_x;
			vop_mode_ptr->mbnumDec = curr_mb_num = mb_y * total_mb_num_x + mb_x;
			curr_mb_dec_status = vop_mode_ptr->mbdec_stat_ptr[curr_mb_num];

			if (curr_mb_dec_status < DECODED_NOT_IN_ERR_PKT) //not coded or coded but in error packet
			{
				mb_mode_ptr->bIntra = FALSE;
				mb_mode_ptr->CBP	= 0;

				if(mb_y < (vop_mode_ptr->MBNumY-1))
				{
					bUseRecFrmAsDispFrm = TRUE;
				}

				if (NOT_DECODED == curr_mb_dec_status)
				{
					is_error = TRUE;
				}else
				{
					is_error = DetectErrBMA (vop_mode_ptr, mb_mode_ptr);

					if (is_error)
					{
						vop_mode_ptr->err_MB_num++;
					}
				}

				if (is_error)
				{
					if ((mb_y == (vop_mode_ptr->MBNumY-1)) && mb_x && mb_y && need_load_data)
					{
					// 	LoadLeftData (vop_mode_ptr);
						need_load_data = FALSE;
					}

					avail_num = GetNeiAvail (vop_mode_ptr, mb_mode_ptr, &nei_intra_num, &nei_avail);

					Mp4Dec_VspMBInit (mb_x, mb_y);

					if (nei_intra_num > 1)
					{
						BI_SpatialConceal (vop_mode_ptr, mb_mode_ptr, nei_avail);
					}else
					{
						if(vop_mode_ptr->pBckRefFrame->pDecFrame != NULL)
						{
							TemporalConceal (vop_mode_ptr, mb_mode_ptr, nei_avail);
							if(VSP_READ_REG_POLL(VSP_MCA_REG_BASE+MCA_DEBUG_OFF, V_BIT_2|V_BIT_1|V_BIT_0, 0, TIME_OUT_CLK, "MCA_DEBUG: polling mca done status"))
                        	{
               					PRINTF("mca time out!\n");
            					return;
							}

							MCABfr2RecFrm(vop_mode_ptr);
						}
					}

					vop_mode_ptr->mbdec_stat_ptr[curr_mb_num] = ERR_CONCEALED;
				}else
				{
					vop_mode_ptr->mbdec_stat_ptr[curr_mb_num] = DECODED_OK;
				}
			}else
			{
				vop_mode_ptr->mbdec_stat_ptr[curr_mb_num] = DECODED_OK;
			}
			mb_mode_ptr++;
		}		
	}

	if(bUseRecFrmAsDispFrm)
	{
		int32 size = vop_mode_ptr->FrameWidth*vop_mode_ptr->FrameHeight;

		memcpy(vop_mode_ptr->pCurDispFrame->pDecFrame->imgY, vop_mode_ptr->pCurRecFrame->pDecFrame->imgY, size);

		if (vop_mode_ptr->uv_interleaved)
		{
			memcpy(vop_mode_ptr->pCurDispFrame->pDecFrame->imgU, vop_mode_ptr->pCurRecFrame->pDecFrame->imgU, size/2);
		}else
		{
			memcpy(vop_mode_ptr->pCurDispFrame->pDecFrame->imgU, vop_mode_ptr->pCurRecFrame->pDecFrame->imgU, size/4);
			memcpy(vop_mode_ptr->pCurDispFrame->pDecFrame->imgV, vop_mode_ptr->pCurRecFrame->pDecFrame->imgV, size/4);
		}
	}
}

//update error information when resync point is founded
PUBLIC void Mp4Dec_UpdateErrInfo (DEC_VOP_MODE_T *vop_mode_ptr)
{
	int32 i;
	int32 bitstream_pos;
	ERR_POS_T *err_pos_ptr_tmp;

	bitstream_pos = (vop_mode_ptr->bitstrm_ptr->bitcnt + 7) / 8;
	err_pos_ptr_tmp = vop_mode_ptr->err_pos_ptr;

	for (i = 0; i < vop_mode_ptr->err_left; i++)
	{
		if (bitstream_pos <= err_pos_ptr_tmp->end_pos)
		{
			break;
		}else
		{
			err_pos_ptr_tmp++;
		}
	}

	vop_mode_ptr->err_left	  -= i;
	vop_mode_ptr->err_pos_ptr += i;
}

//MB from dbk indicated to synchronization are not decoded, use zero-mv inter MBs to fill them.
//use MCA and DBK hardware module.
PUBLIC MMDecRet Mp4Dec_EC_FillMBGap (DEC_VOP_MODE_T *vop_mode_ptr)
{
	int32 ref_frm_valid = (vop_mode_ptr->pBckRefFrame->pDecFrame != PNULL) ? TRUE : FALSE;
	
	if (ref_frm_valid)
	{
		int32 error_mb;
		DEC_MB_MODE_T *mb_mode_ptr,*co_mb_mode_ptr = vop_mode_ptr->pMbMode_prev;
		int32 total_mb_num = vop_mode_ptr->MBNum;
		int32 pos_x, pos_y;
	#if 0	//disabled in CQM mode
		uint32 mb_pos = VSP_READ_REG(VSP_DBK_REG_BASE+DBK_SID_CFG_OFF, "readout current mb index in dbk module");
		
		pos_x = (int32)(mb_pos&0xffff)+1;
		pos_y = (int32)((mb_pos>>16)&0xffff);
	#else
		pos_x = vop_mode_ptr->mb_x;
		pos_y = vop_mode_ptr->mb_y;		
	#endif
	
		if (pos_x == vop_mode_ptr->MBNumX)
		{
			pos_x = 0;
			pos_y++;
		}

		error_mb = pos_y * vop_mode_ptr->MBNumX + pos_x;
		vop_mode_ptr->mbdec_stat_ptr[error_mb-1] = NOT_DECODED;//previous MB maybe error, set "NOT DECODED" for EC later
		for(; (error_mb < vop_mode_ptr->mbnumDec) && (error_mb < total_mb_num); error_mb++)
		{
			mb_mode_ptr = vop_mode_ptr->pMbMode + error_mb;
			mb_mode_ptr->bIntra = FALSE; //let this mb not available in mb pred
			mb_mode_ptr->CBP	= 0;										
			
		#if 0	//disabled in CQM mode
			co_mb_mode_ptr = vop_mode_ptr->pMbMode_prev + error_mb;
			Mp4Dec_VspMBInit (pos_x, pos_y);
			Mp4Dec_MBC_DBK_Cmd (vop_mode_ptr, mb_mode_ptr);
			Mp4Dec_StartMcaOneDir (vop_mode_ptr, vop_mode_ptr->mb_cache_ptr, co_mb_mode_ptr->mv);
			Mp4Dec_CheckMBCStatus (vop_mode_ptr);
			if (vop_mode_ptr->error_flag)
			{
				PRINTF ("MBC time out error!\n");
				return MMDEC_HW_ERROR;
			}

			//set mb status
			vop_mode_ptr->mbdec_stat_ptr[error_mb] = ERR_CONCEALED;
		#endif	
			//next mb
			pos_x++;
			if (pos_x == vop_mode_ptr->MBNumX)
			{
				pos_x = 0;
				pos_y++;
			}
		}
	}else
	{
		return MMDEC_ERROR;
	}

	return MMDEC_OK;
}
/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End 
