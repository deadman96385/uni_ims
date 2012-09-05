/******************************************************************************
 ** File Name:    h264dec_deblock.c                                           *
 ** Author:       Xiaowei Luo                                                 *
 ** DATE:         01/23/2007                                                  *
 ** Copyright:    2006 Spreatrum, Incoporated. All Rights Reserved.           *
 ** Description:                                                              *
 *****************************************************************************/
/******************************************************************************
 **                   Edit    History                                         *
 **---------------------------------------------------------------------------* 
 ** DATE          NAME            DESCRIPTION                                 * 
 ** 01/23/2007    Xiaowei Luo     Create.                                     *
 *****************************************************************************/
/*----------------------------------------------------------------------------*
**                        Dependencies                                        *
**---------------------------------------------------------------------------*/
#include "sc8800g_video_header.h"
/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    extern   "C" 
    {
#endif

LOCAL void H264Dec_BS_Para_intraMB (DEC_IMAGE_PARAMS_T *img_ptr, DEC_MB_INFO_T *mb_info_ptr, DEC_MB_CACHE_T *mb_cache_ptr)
{
	int32 leftEdgeFilterFlag = mb_cache_ptr->left_edge_filter_flag;
	int32 topEdgeFilterFlag = mb_cache_ptr->top_edge_filter_flag;

	if (leftEdgeFilterFlag)
	{
		mb_cache_ptr->BS[0] = 0x04;
		mb_cache_ptr->BS[1] = 0x04;	
		mb_cache_ptr->BS[2] = 0x04;
		mb_cache_ptr->BS[3] = 0x04;
	}else
	{
		mb_cache_ptr->BS[0] = 0x00;
		mb_cache_ptr->BS[1] = 0x00;	
		mb_cache_ptr->BS[2] = 0x00;
		mb_cache_ptr->BS[3] = 0x00;	
	}

	mb_cache_ptr->BS[0] |= 0x03030300;
	mb_cache_ptr->BS[1] |= 0x03030300;
	mb_cache_ptr->BS[2] |= 0x03030300;
	mb_cache_ptr->BS[3] |= 0x03030300;

	if (topEdgeFilterFlag)
	{
		mb_cache_ptr->BS[4] = 0x04;
		mb_cache_ptr->BS[5] = 0x04;
		mb_cache_ptr->BS[6] = 0x04;
		mb_cache_ptr->BS[7] = 0x04;
	}else
	{
		mb_cache_ptr->BS[4] = 0x00;
		mb_cache_ptr->BS[5] = 0x00;
		mb_cache_ptr->BS[6] = 0x00;
		mb_cache_ptr->BS[7] = 0x00;
	}

	mb_cache_ptr->BS[4] |= 0x03030300;
	mb_cache_ptr->BS[5] |= 0x03030300;
	mb_cache_ptr->BS[6] |= 0x03030300;
	mb_cache_ptr->BS[7] |= 0x03030300;

	return;
}

LOCAL void H264Dec_BS_Para_inerMB_hor (DEC_IMAGE_PARAMS_T *img_ptr, DEC_MB_INFO_T *mb_info_ptr, DEC_MB_CACHE_T *mb_cache_ptr)
{
	int8 *ref_idx_ptr = mb_cache_ptr->ref_pic_cache + CONTEXT_CACHE_WIDTH + 1;
	int16 *ref_mv_ptr = mb_info_ptr->mv_cache + (CONTEXT_CACHE_WIDTH+1)*2;
	int32 blkP_mv_x, blkP_mv_y;
	int32 blkQ_mv_x, blkQ_mv_y;
	int32 left_ref_idx, curr_ref_idx;
	int32 leftEdgeFilterFlag = mb_cache_ptr->left_edge_filter_flag;

	if (leftEdgeFilterFlag)
	{
		DEC_MB_INFO_T *left_mb_info_ptr = mb_info_ptr - 1;

		if (left_mb_info_ptr->is_intra)
		{
			mb_cache_ptr->BS[0] = 0x04;
			mb_cache_ptr->BS[1] = 0x04;
			mb_cache_ptr->BS[2] = 0x04;
			mb_cache_ptr->BS[3] = 0x04;

			ref_mv_ptr += (1*2);
		}else
		{
			int16 *blkP_ref_mv_ptr;
			int8  *blkP_ref_idx_ptr;
			int32 edgePart;

			blkP_ref_idx_ptr = ref_idx_ptr - 1;

			if (mb_cache_ptr->mb_avail_a)
			{
				blkP_ref_mv_ptr = ref_mv_ptr - 2*1;
			}else
			{
				blkP_ref_mv_ptr = left_mb_info_ptr->mv_cache+ (CONTEXT_CACHE_WIDTH+4)*2;
			}

			for (edgePart = 0; edgePart < 4; edgePart++)
			{
				blkP_mv_x = blkP_ref_mv_ptr[0];
				blkP_mv_y = blkP_ref_mv_ptr[1];

				blkQ_mv_x = ref_mv_ptr[0];
				blkQ_mv_y = ref_mv_ptr[1];

				left_ref_idx = blkP_ref_idx_ptr[0];
				curr_ref_idx = ref_idx_ptr[0];

				blkP_ref_mv_ptr += 2*CONTEXT_CACHE_WIDTH;
				blkP_ref_idx_ptr += CONTEXT_CACHE_WIDTH;
				ref_mv_ptr += 2*CONTEXT_CACHE_WIDTH;
				ref_idx_ptr += CONTEXT_CACHE_WIDTH;

				mb_cache_ptr->BS[edgePart] = ((ABS(blkP_mv_x - blkQ_mv_x) >= 4) || (ABS(blkP_mv_y - blkQ_mv_y) >=4)) || (left_ref_idx != curr_ref_idx);
			}

			ref_mv_ptr += (-4*CONTEXT_CACHE_WIDTH+1)*2;
			ref_idx_ptr += (-4*CONTEXT_CACHE_WIDTH);
		}
	}else
	{
		mb_cache_ptr->BS[0] = 0x00;
		mb_cache_ptr->BS[1] = 0x00;
		mb_cache_ptr->BS[2] = 0x00;
		mb_cache_ptr->BS[3] = 0x00;

		ref_mv_ptr += (1*2);
	}

	//the other 3 block edge in current MB
	if (!mb_info_ptr->is_skipped)
	{
		int32 edge;

		for (edge = 1; edge < 4; edge++)
		{
			int32 tmp_bs;

			blkP_mv_x = ref_mv_ptr[-2];
			blkP_mv_y = ref_mv_ptr[-1];
			blkQ_mv_x = ref_mv_ptr[0];
			blkQ_mv_y = ref_mv_ptr[1];
			ref_mv_ptr += 2*CONTEXT_CACHE_WIDTH;

			tmp_bs = ((ABS(blkP_mv_x - blkQ_mv_x) >= 4) || (ABS(blkP_mv_y - blkQ_mv_y) >= 4));
			mb_cache_ptr->BS[0] |= (tmp_bs << (8*edge));

			blkP_mv_x = ref_mv_ptr[-2];
			blkP_mv_y = ref_mv_ptr[-1];
			blkQ_mv_x = ref_mv_ptr[0];
			blkQ_mv_y = ref_mv_ptr[1];
			ref_mv_ptr += 2*CONTEXT_CACHE_WIDTH;

			tmp_bs = ((ABS(blkP_mv_x - blkQ_mv_x) >= 4) || (ABS(blkP_mv_y - blkQ_mv_y) >= 4));
			mb_cache_ptr->BS[1] |= (tmp_bs << (8*edge));

			blkP_mv_x = ref_mv_ptr[-2];
			blkP_mv_y = ref_mv_ptr[-1];
			blkQ_mv_x = ref_mv_ptr[0];
			blkQ_mv_y = ref_mv_ptr[1];
			ref_mv_ptr += 2*CONTEXT_CACHE_WIDTH;

			tmp_bs = ((ABS(blkP_mv_x - blkQ_mv_x) >= 4) || (ABS(blkP_mv_y - blkQ_mv_y) >= 4));
			mb_cache_ptr->BS[2] |= (tmp_bs << (8*edge));

			blkP_mv_x = ref_mv_ptr[-2];
			blkP_mv_y = ref_mv_ptr[-1];
			blkQ_mv_x = ref_mv_ptr[0];
			blkQ_mv_y = ref_mv_ptr[1];
			ref_mv_ptr += 2*(-3*CONTEXT_CACHE_WIDTH+1);

			tmp_bs = ((ABS(blkP_mv_x - blkQ_mv_x) >= 4) || (ABS(blkP_mv_y - blkQ_mv_y) >= 4));
			mb_cache_ptr->BS[3] |= (tmp_bs << (8*edge));

			left_ref_idx = ref_idx_ptr[edge-1];
			curr_ref_idx = ref_idx_ptr[edge];
			if (left_ref_idx != curr_ref_idx)
			{
				mb_cache_ptr->BS[0] |= (0x1<<(8*edge));
			}

			left_ref_idx = ref_idx_ptr[edge + CONTEXT_CACHE_WIDTH - 1];
			curr_ref_idx = ref_idx_ptr[edge + CONTEXT_CACHE_WIDTH];
			if (left_ref_idx != curr_ref_idx)
			{
				mb_cache_ptr->BS[1] |= (0x01<<(8*edge));
			}

			left_ref_idx = ref_idx_ptr[edge + 2*CONTEXT_CACHE_WIDTH - 1];
			curr_ref_idx = ref_idx_ptr[edge + 2*CONTEXT_CACHE_WIDTH];
			if (left_ref_idx != curr_ref_idx)
			{
				mb_cache_ptr->BS[2] |= (0x01<<(8*edge));
			}

			left_ref_idx = ref_idx_ptr[edge + 3*CONTEXT_CACHE_WIDTH - 1];
			curr_ref_idx = ref_idx_ptr[edge + 3*CONTEXT_CACHE_WIDTH];
			if (left_ref_idx != curr_ref_idx)
			{
				mb_cache_ptr->BS[3] |= (0x01<<(8*edge));
			}
		}
	}else
	{
		mb_cache_ptr->BS[0] &= 0xff;
		mb_cache_ptr->BS[1] &= 0xff;
		mb_cache_ptr->BS[2] &= 0xff;
		mb_cache_ptr->BS[3] &= 0xff;
	}

	return;
}

LOCAL void H264Dec_BS_Para_inerMB_ver (DEC_IMAGE_PARAMS_T *img_ptr, DEC_MB_INFO_T *mb_info_ptr, DEC_MB_CACHE_T *mb_cache_ptr)
{
	int8 *ref_idx_ptr = mb_cache_ptr->ref_pic_cache + CONTEXT_CACHE_WIDTH + 1;
	int16 *ref_mv_ptr = mb_info_ptr->mv_cache + (CONTEXT_CACHE_WIDTH+1)*2;
	int32 blkP_mv_x, blkP_mv_y;
	int32 blkQ_mv_x, blkQ_mv_y;
	int32 topEdgeFilterFlag = mb_cache_ptr->top_edge_filter_flag;

	if (topEdgeFilterFlag)
	{
		DEC_MB_INFO_T *top_mb_info_ptr = mb_info_ptr - img_ptr->frame_width_in_mbs;

		if (top_mb_info_ptr->is_intra)
		{
			mb_cache_ptr->BS[4] = 0x04;
			mb_cache_ptr->BS[5] = 0x04;
			mb_cache_ptr->BS[6] = 0x04;
			mb_cache_ptr->BS[7] = 0x04;

			ref_mv_ptr += (2*CONTEXT_CACHE_WIDTH);
			ref_idx_ptr += CONTEXT_CACHE_WIDTH;
		}else
		{
			int16 *blkP_ref_mv_ptr;
			int8  *blkP_ref_idx_ptr;
			int32 tmp_bs;

			if (!mb_cache_ptr->mb_avail_b)
			{
				blkP_ref_mv_ptr = top_mb_info_ptr->mv_cache+(4*CONTEXT_CACHE_WIDTH+1)*2;
			}else
			{
				blkP_ref_mv_ptr = ref_mv_ptr - (CONTEXT_CACHE_WIDTH*2);
 			}

			//bs4
			blkP_mv_x = blkP_ref_mv_ptr[0];
			blkP_mv_y = blkP_ref_mv_ptr[1];

			blkQ_mv_x = ref_mv_ptr[0];
			blkQ_mv_y = ref_mv_ptr[1];

			tmp_bs = ((ABS(blkP_mv_x - blkQ_mv_x) >= 4) || (ABS(blkP_mv_y - blkQ_mv_y) >=4));
			mb_cache_ptr->BS[4] = tmp_bs;

			//bs5
			blkP_ref_mv_ptr += 2;
			ref_mv_ptr += 2;
			blkP_mv_x = blkP_ref_mv_ptr[0];
			blkP_mv_y = blkP_ref_mv_ptr[1];

			blkQ_mv_x = ref_mv_ptr[0];
			blkQ_mv_y = ref_mv_ptr[1];

			tmp_bs = ((ABS(blkP_mv_x - blkQ_mv_x) >= 4) || (ABS(blkP_mv_y - blkQ_mv_y) >=4));
			mb_cache_ptr->BS[5] = tmp_bs;

			//bs6
			blkP_ref_mv_ptr += 2;
			ref_mv_ptr += 2;
			blkP_mv_x = blkP_ref_mv_ptr[0];
			blkP_mv_y = blkP_ref_mv_ptr[1];

			blkQ_mv_x = ref_mv_ptr[0];
			blkQ_mv_y = ref_mv_ptr[1];

			tmp_bs = ((ABS(blkP_mv_x - blkQ_mv_x) >= 4) || (ABS(blkP_mv_y - blkQ_mv_y) >=4));
			mb_cache_ptr->BS[6] = tmp_bs;

			//bs7
			blkP_ref_mv_ptr += 2;
			ref_mv_ptr += 2;
			blkP_mv_x = blkP_ref_mv_ptr[0];
			blkP_mv_y = blkP_ref_mv_ptr[1];

			blkQ_mv_x = ref_mv_ptr[0];
			blkQ_mv_y = ref_mv_ptr[1];

			tmp_bs = ((ABS(blkP_mv_x - blkQ_mv_x) >= 4) || (ABS(blkP_mv_y - blkQ_mv_y) >=4));
			mb_cache_ptr->BS[7] = tmp_bs;

			ref_mv_ptr += 2*(CONTEXT_CACHE_WIDTH-3);

			//update according to ref_idx
			blkP_ref_idx_ptr = ref_idx_ptr - CONTEXT_CACHE_WIDTH; 
			
			if (blkP_ref_idx_ptr[0] != ref_idx_ptr[0])
			{
				mb_cache_ptr->BS[4] |= 0x01;
			}

			if (blkP_ref_idx_ptr[1] != ref_idx_ptr[1])
			{
				mb_cache_ptr->BS[5] |= 0x01;
			}

			if (blkP_ref_idx_ptr[2] != ref_idx_ptr[2])
			{
				mb_cache_ptr->BS[6] |= 0x01;
			}

			if (blkP_ref_idx_ptr[3] != ref_idx_ptr[3])
			{
				mb_cache_ptr->BS[7] |= 0x01;
			}

			ref_idx_ptr += CONTEXT_CACHE_WIDTH;
		}
	}else
	{
		mb_cache_ptr->BS[4] = 0x00;
		mb_cache_ptr->BS[5] = 0x00;
		mb_cache_ptr->BS[6] = 0x00;
		mb_cache_ptr->BS[7] = 0x00;

		ref_mv_ptr += (CONTEXT_CACHE_WIDTH*2);
		ref_idx_ptr += (CONTEXT_CACHE_WIDTH);
	}

	//the other 3 block edge in current MB
	if (!mb_info_ptr->is_skipped)
	{
		int32 edge;

		for (edge = 1; edge < 4; edge++)
		{
			int32 tmp_bs;

			blkP_mv_x = ref_mv_ptr[-2*CONTEXT_CACHE_WIDTH];
			blkP_mv_y = ref_mv_ptr[-2*CONTEXT_CACHE_WIDTH+1];
			blkQ_mv_x = ref_mv_ptr[0];
			blkQ_mv_y = ref_mv_ptr[1];
			ref_mv_ptr += 2;

			tmp_bs = ((ABS(blkP_mv_x - blkQ_mv_x) >= 4) || (ABS(blkP_mv_y - blkQ_mv_y) >= 4));
			mb_cache_ptr->BS[4] |= (tmp_bs << (8*edge));

			blkP_mv_x = ref_mv_ptr[-2*CONTEXT_CACHE_WIDTH];
			blkP_mv_y = ref_mv_ptr[-2*CONTEXT_CACHE_WIDTH+1];
			blkQ_mv_x = ref_mv_ptr[0];
			blkQ_mv_y = ref_mv_ptr[1];
			ref_mv_ptr += 2;

			tmp_bs = ((ABS(blkP_mv_x - blkQ_mv_x) >= 4) || (ABS(blkP_mv_y - blkQ_mv_y) >= 4));
			mb_cache_ptr->BS[5] |= (tmp_bs << (8*edge));

			blkP_mv_x = ref_mv_ptr[-2*CONTEXT_CACHE_WIDTH];
			blkP_mv_y = ref_mv_ptr[-2*CONTEXT_CACHE_WIDTH+1];
			blkQ_mv_x = ref_mv_ptr[0];
			blkQ_mv_y = ref_mv_ptr[1];
			ref_mv_ptr += 2;

			tmp_bs = ((ABS(blkP_mv_x - blkQ_mv_x) >= 4) || (ABS(blkP_mv_y - blkQ_mv_y) >= 4));
			mb_cache_ptr->BS[6] |= (tmp_bs << (8*edge));

			blkP_mv_x = ref_mv_ptr[-2*CONTEXT_CACHE_WIDTH];
			blkP_mv_y = ref_mv_ptr[-2*CONTEXT_CACHE_WIDTH+1];
			blkQ_mv_x = ref_mv_ptr[0];
			blkQ_mv_y = ref_mv_ptr[1];
			ref_mv_ptr += 2*(CONTEXT_CACHE_WIDTH-3);

			tmp_bs = ((ABS(blkP_mv_x - blkQ_mv_x) >= 4) || (ABS(blkP_mv_y - blkQ_mv_y) >= 4));
			mb_cache_ptr->BS[7] |= (tmp_bs << (8*edge));

			if (ref_idx_ptr[0-CONTEXT_CACHE_WIDTH] != ref_idx_ptr[0])
			{
				mb_cache_ptr->BS[4] |= (0x1<<(8*edge));
			}

			if (ref_idx_ptr[1-CONTEXT_CACHE_WIDTH] != ref_idx_ptr[1])
			{
				mb_cache_ptr->BS[5] |= (0x01<<(8*edge));
			}

			if (ref_idx_ptr[2-CONTEXT_CACHE_WIDTH] != ref_idx_ptr[2])
			{
				mb_cache_ptr->BS[6] |= (0x01<<(8*edge));
			}

			if (ref_idx_ptr[3-CONTEXT_CACHE_WIDTH] != ref_idx_ptr[3])
			{
				mb_cache_ptr->BS[7] |= (0x01<<(8*edge));
			}

			ref_idx_ptr += CONTEXT_CACHE_WIDTH;
		}
	}else
	{
		mb_cache_ptr->BS[4] &= 0xff;
		mb_cache_ptr->BS[5] &= 0xff;
		mb_cache_ptr->BS[6] &= 0xff;
		mb_cache_ptr->BS[7] &= 0xff;
	}

	return;
}

LOCAL void H264Dec_InterMB_BS_nnz_hor (DEC_IMAGE_PARAMS_T *img_ptr, DEC_MB_INFO_T *mb_info_ptr, DEC_MB_CACHE_T *mb_cache_ptr)
{
	int32 edge;
	int32 leftEdgeFilterFlag = mb_cache_ptr->left_edge_filter_flag;
	int8 *nnz_ref_ptr = mb_info_ptr->nnz_ref + CONTEXT_CACHE_WIDTH + 1;
	uint32 left_nnz, curr_nnz;
	uint32 tmpBs;

	if (leftEdgeFilterFlag)
	{
		DEC_MB_INFO_T *left_mb_info_ptr = mb_info_ptr-1;

		if (!left_mb_info_ptr->is_intra)
		{
			int8 *left_mb_nnz_ref_ptr = left_mb_info_ptr->nnz_ref + CONTEXT_CACHE_WIDTH + 4;

			left_nnz = left_mb_nnz_ref_ptr[0];
			curr_nnz = nnz_ref_ptr[0];
			nnz_ref_ptr += CONTEXT_CACHE_WIDTH;
			left_mb_nnz_ref_ptr += CONTEXT_CACHE_WIDTH;
			if (left_nnz || curr_nnz)
			{
				tmpBs = mb_cache_ptr->BS[0];
				tmpBs = ((tmpBs>>8)<<8);
				mb_cache_ptr->BS[0] = (tmpBs | 0x02);
			}

			left_nnz = left_mb_nnz_ref_ptr[0];
			curr_nnz = nnz_ref_ptr[0];
			nnz_ref_ptr += CONTEXT_CACHE_WIDTH;
			left_mb_nnz_ref_ptr += CONTEXT_CACHE_WIDTH;
			if (left_nnz || curr_nnz)
			{
				tmpBs = mb_cache_ptr->BS[1];
				tmpBs = ((tmpBs>>8)<<8);
				mb_cache_ptr->BS[1] = (tmpBs | 0x02);
			}

			left_nnz = left_mb_nnz_ref_ptr[0];
			curr_nnz = nnz_ref_ptr[0];
			nnz_ref_ptr += CONTEXT_CACHE_WIDTH;
			left_mb_nnz_ref_ptr += CONTEXT_CACHE_WIDTH;
			if (left_nnz || curr_nnz)
			{
				tmpBs = mb_cache_ptr->BS[2];
				tmpBs = ((tmpBs>>8)<<8);
				mb_cache_ptr->BS[2] = (tmpBs | 0x02);
			}

			left_nnz = left_mb_nnz_ref_ptr[0];
			curr_nnz = nnz_ref_ptr[0];
			if (left_nnz || curr_nnz)
			{
				tmpBs = mb_cache_ptr->BS[3];
				tmpBs = ((tmpBs>>8)<<8);
				mb_cache_ptr->BS[3] = (tmpBs | 0x02);
			}

			nnz_ref_ptr -= (3*CONTEXT_CACHE_WIDTH);			
		}
	}

	//the other 3 block edge in current MB
	if (!mb_info_ptr->is_skipped)
	{
		for (edge = 1; edge < 4; edge++)
		{
			left_nnz = nnz_ref_ptr[edge-1];
			curr_nnz = nnz_ref_ptr[edge];
			tmpBs = (0xff << (8*edge));
			tmpBs = (~tmpBs);

			if (left_nnz || curr_nnz)
			{
				mb_cache_ptr->BS[0] &= tmpBs;
				mb_cache_ptr->BS[0] |= (0x02 << (8*edge));
			}

			left_nnz = nnz_ref_ptr[edge + CONTEXT_CACHE_WIDTH -1];
			curr_nnz = nnz_ref_ptr[edge + CONTEXT_CACHE_WIDTH];
			if (left_nnz || curr_nnz)
			{
				mb_cache_ptr->BS[1] &= tmpBs;
				mb_cache_ptr->BS[1] |= (0x02 << (8*edge));
			}

			left_nnz = nnz_ref_ptr[edge + 2*CONTEXT_CACHE_WIDTH -1];
			curr_nnz = nnz_ref_ptr[edge + 2*CONTEXT_CACHE_WIDTH];
			if (left_nnz || curr_nnz)
			{
				mb_cache_ptr->BS[2] &= tmpBs;
				mb_cache_ptr->BS[2] |= (0x02 << (8*edge));
			}

			left_nnz = nnz_ref_ptr[edge + 3*CONTEXT_CACHE_WIDTH -1];
			curr_nnz = nnz_ref_ptr[edge + 3*CONTEXT_CACHE_WIDTH];
			if (left_nnz || curr_nnz)
			{
				mb_cache_ptr->BS[3] &= tmpBs;
				mb_cache_ptr->BS[3] |= (0x02 << (8*edge));
			}
		}
	}

	return;
}

LOCAL void H264Dec_InterMB_BS_nnz_ver (DEC_IMAGE_PARAMS_T *img_ptr, DEC_MB_INFO_T *mb_info_ptr, DEC_MB_CACHE_T *mb_cache_ptr)
{
	int32 edge, edgePart;
	int32 topEdgeFilterFlag = mb_cache_ptr->top_edge_filter_flag;
	int8 *nnz_ref_ptr = mb_info_ptr->nnz_ref + CONTEXT_CACHE_WIDTH + 1;
	uint32 tmpBs;

	if (topEdgeFilterFlag)
	{
		DEC_MB_INFO_T *top_mb_info_ptr = mb_info_ptr - img_ptr->frame_width_in_mbs;

		if (!top_mb_info_ptr->is_intra)
		{
			int8 *top_nnz_ref_ptr = top_mb_info_ptr->nnz_ref+4*CONTEXT_CACHE_WIDTH+1;

			for (edgePart = 0; edgePart < 4; edgePart++)
			{
				if (top_nnz_ref_ptr[edgePart] | nnz_ref_ptr[edgePart])
				{
					tmpBs = mb_cache_ptr->BS[4+edgePart];
					tmpBs = ((tmpBs>>8)<<8);

					mb_cache_ptr->BS[4+edgePart] = (tmpBs | 0x02);
				}
			}
		}
	}
	nnz_ref_ptr += CONTEXT_CACHE_WIDTH;

	//deblocking the other 3 edge
	if (!mb_info_ptr->is_skipped)
	{
		for (edge = 1; edge < 4; edge++)
		{
			tmpBs = (0xff << (8*edge));
			tmpBs = (~tmpBs);

			if (nnz_ref_ptr[0-CONTEXT_CACHE_WIDTH] | nnz_ref_ptr[0])
			{
				mb_cache_ptr->BS[4] &= tmpBs;
				mb_cache_ptr->BS[4] |= (0x02<<(8*edge));			
			}

			if (nnz_ref_ptr[1-CONTEXT_CACHE_WIDTH] | nnz_ref_ptr[1])
			{
				mb_cache_ptr->BS[5] &= tmpBs;
				mb_cache_ptr->BS[5] |= (0x02<<(8*edge));			
			}

			if (nnz_ref_ptr[2-CONTEXT_CACHE_WIDTH] | nnz_ref_ptr[2])
			{
				mb_cache_ptr->BS[6] &= tmpBs;
				mb_cache_ptr->BS[6] |= (0x02<<(8*edge));			
			}

			if (nnz_ref_ptr[3-CONTEXT_CACHE_WIDTH] | nnz_ref_ptr[3])
			{
				mb_cache_ptr->BS[7] &= tmpBs;
				mb_cache_ptr->BS[7] |= (0x02<<(8*edge));			
			}
			nnz_ref_ptr += CONTEXT_CACHE_WIDTH;
		}
	}

	return;
}

LOCAL void H264Dec_BS_Para_interMB (DEC_IMAGE_PARAMS_T *img_ptr, DEC_MB_INFO_T *mb_info_ptr, DEC_MB_CACHE_T *mb_cache_ptr)
{
	int32 i;
	int32 position[4] = {7, 9, 19, 21};
	DEC_MB_INFO_T *left_mb_info_ptr = mb_info_ptr - 1;
	DEC_MB_INFO_T *top_mb_info_ptr = mb_info_ptr - img_ptr->frame_width_in_mbs;

	if (mb_cache_ptr->left_edge_filter_flag)
	{
		mb_cache_ptr->ref_pic_cache[CONTEXT_CACHE_WIDTH] = left_mb_info_ptr->ref_pic[1];
		mb_cache_ptr->ref_pic_cache[2*CONTEXT_CACHE_WIDTH] = left_mb_info_ptr->ref_pic[1];
		mb_cache_ptr->ref_pic_cache[3*CONTEXT_CACHE_WIDTH] = left_mb_info_ptr->ref_pic[3];
		mb_cache_ptr->ref_pic_cache[4*CONTEXT_CACHE_WIDTH] = left_mb_info_ptr->ref_pic[3];
	}

	if (mb_cache_ptr->top_edge_filter_flag)
	{
		mb_cache_ptr->ref_pic_cache[1] = top_mb_info_ptr->ref_pic[2];
		mb_cache_ptr->ref_pic_cache[2] = top_mb_info_ptr->ref_pic[2];
		mb_cache_ptr->ref_pic_cache[3] = top_mb_info_ptr->ref_pic[3];
		mb_cache_ptr->ref_pic_cache[4] = top_mb_info_ptr->ref_pic[3];
	}

	for (i = 0; i < 4; i++)
	{
		int32 idx = position[i];
		int8 ref_pic_num = mb_info_ptr->ref_pic[i];

		mb_cache_ptr->ref_pic_cache[idx] = ref_pic_num;
		mb_cache_ptr->ref_pic_cache[idx+1] = ref_pic_num;
		mb_cache_ptr->ref_pic_cache[idx+CONTEXT_CACHE_WIDTH] = ref_pic_num;
		mb_cache_ptr->ref_pic_cache[idx+CONTEXT_CACHE_WIDTH+1] = ref_pic_num;
	}

	H264Dec_BS_Para_inerMB_hor (img_ptr, mb_info_ptr, mb_cache_ptr);
	H264Dec_BS_Para_inerMB_ver (img_ptr, mb_info_ptr, mb_cache_ptr);

	return;
}

LOCAL void H264Dec_Config_DBK (DEC_IMAGE_PARAMS_T *img_ptr, DEC_MB_INFO_T *mb_info_ptr, DEC_MB_CACHE_T *mb_cache_ptr)
{
	int32 mb_x = img_ptr->mb_x;
	int32 mb_y = img_ptr->mb_y;
	int32 top_mb_qp = 0;
	int32 left_mb_qp = 0;
	uint32 bs[32];
	uint32 cmd;
	
	if (mb_y > 0)
	{
		top_mb_qp = (mb_info_ptr - img_ptr->frame_width_in_mbs)->qp;
	}

	if (mb_x > 0)
	{
		left_mb_qp = (mb_info_ptr - 1)->qp;
	}

	//polling dbk ready
	READ_REG_POLL(VSP_DBK_REG_BASE+HDBK_CFG_FINISH_OFF, V_BIT_0, 0, TIME_OUT_CLK,"dbk: polling dbk cfg finish flag = 0");

	cmd = ((mb_x << 24) | (top_mb_qp << 16) | (left_mb_qp << 8) | (mb_info_ptr->qp << 0));
	VSP_WRITE_REG (VSP_DBK_REG_BASE+HDBK_MB_INFO_OFF, cmd, "configure mb information");

	cmd = (((img_ptr->chroma_qp_offset & 0x1f) << 16) | ((mb_info_ptr->LFAlphaC0Offset & 0x1f) << 8) | ((mb_info_ptr->LFBetaOffset & 0x1f) << 0));
	VSP_WRITE_REG (VSP_DBK_REG_BASE+HDBK_PARS_OFF, cmd, "configure dbk parameter");
	
#if 0
	mb_cache_ptr->BS[0] = 0;
	mb_cache_ptr->BS[1] = 0;
	mb_cache_ptr->BS[2] = 0;
	mb_cache_ptr->BS[3] = 0;
	mb_cache_ptr->BS[4] = 0;
	mb_cache_ptr->BS[5] = 0;
	mb_cache_ptr->BS[6] = 0;
	mb_cache_ptr->BS[7] = 0;							
#endif	

	/*vertical edge BS*/
	bs[0] = (mb_cache_ptr->BS[0] >> 0)  & 0xff;
	bs[4] = (mb_cache_ptr->BS[0] >> 8)  & 0xff;
	bs[8] = (mb_cache_ptr->BS[0] >> 16) & 0xff;
	bs[12]= (mb_cache_ptr->BS[0] >> 24) & 0xff;
	
	bs[1] = (mb_cache_ptr->BS[1] >> 0)  & 0xff;
	bs[5] = (mb_cache_ptr->BS[1] >> 8)  & 0xff;
	bs[9] = (mb_cache_ptr->BS[1] >> 16) & 0xff;
	bs[13]= (mb_cache_ptr->BS[1] >> 24) & 0xff;

	bs[2] = (mb_cache_ptr->BS[2] >> 0)  & 0xff;
	bs[6] = (mb_cache_ptr->BS[2] >> 8)  & 0xff;
	bs[10]= (mb_cache_ptr->BS[2] >> 16) & 0xff;
	bs[14]= (mb_cache_ptr->BS[2] >> 24) & 0xff;
	
	bs[3] = (mb_cache_ptr->BS[3] >> 0)  & 0xff;
	bs[7] = (mb_cache_ptr->BS[3] >> 8)  & 0xff;
	bs[11]= (mb_cache_ptr->BS[3] >> 16) & 0xff;
	bs[15]= (mb_cache_ptr->BS[3] >> 24) & 0xff;

	cmd = (bs[0] << 0)  | (bs[1] << 4)  | (bs[2] << 8)  | (bs[3] << 12) |
		  (bs[4] << 16) | (bs[5] << 20) | (bs[6] << 24) | (bs[7] << 28);
	VSP_WRITE_REG(VSP_DBK_REG_BASE+HDBK_BS_H0_OFF, cmd, "configure bs h0");

	cmd = (bs[8] << 0)   | (bs[9] << 4)   | (bs[10] << 8)  | (bs[11] << 12) |
		  (bs[12] << 16) | (bs[13] << 20) | (bs[14] << 24) | (bs[15] << 28);
	VSP_WRITE_REG(VSP_DBK_REG_BASE+HDBK_BS_H1_OFF, cmd, "configure bs h1");

	/*horizontal edge BS*/
	bs[16] = (mb_cache_ptr->BS[4] >> 0)  & 0xff;
	bs[20] = (mb_cache_ptr->BS[4] >> 8)  & 0xff;
	bs[24] = (mb_cache_ptr->BS[4] >> 16) & 0xff;
	bs[28] = (mb_cache_ptr->BS[4] >> 24) & 0xff;
	
	bs[17] = (mb_cache_ptr->BS[5] >> 0)  & 0xff;
	bs[21] = (mb_cache_ptr->BS[5] >> 8)  & 0xff;
	bs[25] = (mb_cache_ptr->BS[5] >> 16) & 0xff;
	bs[29] = (mb_cache_ptr->BS[5] >> 24) & 0xff;

	bs[18] = (mb_cache_ptr->BS[6] >> 0)  & 0xff;
	bs[22] = (mb_cache_ptr->BS[6] >> 8)  & 0xff;
	bs[26] = (mb_cache_ptr->BS[6] >> 16) & 0xff;
	bs[30] = (mb_cache_ptr->BS[6] >> 24) & 0xff;

	bs[19] = (mb_cache_ptr->BS[7] >> 0)  & 0xff;
	bs[23] = (mb_cache_ptr->BS[7] >> 8)  & 0xff;
	bs[27] = (mb_cache_ptr->BS[7] >> 16) & 0xff;
	bs[31] = (mb_cache_ptr->BS[7] >> 24) & 0xff;

	cmd = (bs[16] << 0)  | (bs[17] << 4)  | (bs[18] << 8)  | (bs[19] << 12) |
		  (bs[20] << 16) | (bs[21] << 20) | (bs[22] << 24) | (bs[23] << 28);
	VSP_WRITE_REG(VSP_DBK_REG_BASE+HDBK_BS_V0_OFF, cmd, "configure bs v0");

	cmd = (bs[24] << 0)  | (bs[25] << 4)  | (bs[26] << 8)  | (bs[27] << 12) |
		  (bs[28] << 16) | (bs[29] << 20) | (bs[30] << 24) | (bs[31] << 28);
	VSP_WRITE_REG(VSP_DBK_REG_BASE+HDBK_BS_V1_OFF, cmd, "configure bs v1");

	VSP_WRITE_REG(VSP_DBK_REG_BASE+HDBK_CFG_FINISH_OFF, 1, "config finished, start dbk");

	return;
}

PUBLIC void H264Dec_BS_and_Para (DEC_IMAGE_PARAMS_T *img_ptr, DEC_MB_INFO_T *mb_info_ptr, DEC_MB_CACHE_T *mb_cache_ptr)
{
	if (mb_info_ptr->LFDisableIdc == 2)
	{
		mb_cache_ptr->top_edge_filter_flag = mb_cache_ptr->mb_avail_b;
		mb_cache_ptr->left_edge_filter_flag = mb_cache_ptr->mb_avail_a;
	}else
	{
		mb_cache_ptr->top_edge_filter_flag = ((img_ptr->mb_y > 0) ? 1 : 0);
		mb_cache_ptr->left_edge_filter_flag = ((img_ptr->mb_x > 0) ? 1 : 0);
	}

	if (mb_info_ptr->LFDisableIdc == 1) //not filtered
	{
		mb_cache_ptr->BS[0] = mb_cache_ptr->BS[1] = mb_cache_ptr->BS[2] = mb_cache_ptr->BS[3] = 0x0;
		mb_cache_ptr->BS[4] = mb_cache_ptr->BS[5] = mb_cache_ptr->BS[6] = mb_cache_ptr->BS[7] = 0x0;
	}else
	{
		if (mb_info_ptr->is_intra)
		{
			H264Dec_BS_Para_intraMB (img_ptr, mb_info_ptr, mb_cache_ptr);
		}else
		{
			H264Dec_BS_Para_interMB (img_ptr, mb_info_ptr, mb_cache_ptr);

			H264Dec_InterMB_BS_nnz_hor (img_ptr, mb_info_ptr, mb_cache_ptr);
			H264Dec_InterMB_BS_nnz_ver (img_ptr, mb_info_ptr, mb_cache_ptr);
		}
	}

	H264Dec_Config_DBK (img_ptr, mb_info_ptr, mb_cache_ptr);

	return;
}

//for FMO mode
PUBLIC void H264Dec_deblock_one_frame (DEC_IMAGE_PARAMS_T *img_ptr)
{
	int32 mb_x, mb_y;
	DEC_MB_CACHE_T *mb_cache_ptr = g_mb_cache_ptr;
	DEC_MB_INFO_T *curr_mb_info_ptr;

#if _CMODEL_
	uint8 *tmp_imgY = g_list0[0]->imgY;
	uint8 *tmp_imgU = g_list0[0]->imgU;
	g_list0[0]->imgY = g_dec_picture_ptr->imgY;
	g_list0[0]->imgU = g_dec_picture_ptr->imgU;
#endif
	
	//config current rec frame as mca ref frame
	open_vsp_iram();
	VSP_WRITE_REG(VSP_MEMO10_ADDR+ 0*4, g_dec_picture_ptr->imgYAddr, "configure recontructed picture");
	VSP_WRITE_REG(VSP_MEMO10_ADDR+ 4*4, g_dec_picture_ptr->imgYAddr, "configure reference picture");
	close_vsp_iram();

	//clear fmo flag
	VSP_WRITE_REG(VSP_DBK_REG_BASE+DBK_CFG_OFF, 6, "configure dbk free run mode and enable filter");

	for (mb_y = 0; mb_y < img_ptr->frame_height_in_mbs; mb_y++)
	{
		for (mb_x = 0; mb_x < img_ptr->frame_width_in_mbs; mb_x++)
		{
			//config mca using zero mv
			int32 ref_blk_end = TRUE;
			int32 ref_bir_blk = FALSE;
			int32 ref_cmd_type = 0;
			int32 ref_bw_frame_id = 0;
			int32 ref_fw_frame_id = 0;//use the first reference frame
			int32 ref_blk_id = 0;
			int32 ref_blk_size = MC_BLKSIZE_16x16;
			int32 mc_blk_info;

			if ((mb_x == 5) && (mb_y == 0) && (g_nFrame_dec_h264 == 1))
			{
				PRINTF("");
			}

			img_ptr->curr_mb_nr = mb_y*img_ptr->frame_width_in_mbs+mb_x;
			img_ptr->mb_x = mb_x;
			img_ptr->mb_y = mb_y;

			curr_mb_info_ptr = img_ptr->mb_info + img_ptr->curr_mb_nr;

			H264Dec_get_mb_avail(img_ptr, curr_mb_info_ptr, mb_cache_ptr);
			
			VSP_WRITE_REG (VSP_GLB_REG_BASE+GLB_CTRL0_OFF, (mb_y << 8) | (mb_x << 0), "vsp global reg: configure current MB position");
			
			mc_blk_info = (ref_blk_end << 27) | (ref_bir_blk << 26) | (ref_cmd_type << 24) | (ref_bw_frame_id << 20)|
				(ref_fw_frame_id << 16) | (ref_blk_id << 8) | (ref_blk_size & 0xff);
			VSP_WRITE_REG(VSP_MCA_REG_BASE+MCA_BLK_CBUF_OFF, mc_blk_info, "configure MCA command buffer, block info");
			VSP_WRITE_REG(VSP_MCA_REG_BASE+MCA_MV_CBUF_OFF, 0, "configure MCA command buffer, block mv info");

			VSP_WRITE_REG(VSP_MCA_REG_BASE+MCA_CFG_OFF, 1, "MCA_CFG: start MCA");

		#if _CMODEL_
			mca_module();
		#endif

			//mbc
			VSP_WRITE_REG(VSP_MBC_REG_BASE+MBC_CMD0_OFF, (MBC_INTER_MB<<30) |	(0 & 0xffffff), "configure mb information, cbp = 0.");
						
			//dbk
			H264Dec_BS_and_Para (img_ptr, curr_mb_info_ptr, mb_cache_ptr);

			H264Dec_mb_level_sync (img_ptr);
		}
	}

#if _CMODEL_
	g_list0[0]->imgY = tmp_imgY;
	g_list0[0]->imgU = tmp_imgU;
#endif

	return;
}
/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End 