/******************************************************************************
 ** File Name:    h264dec_mv.c			                                      *
 ** Author:       Xiaowei Luo                                                 *
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
/*----------------------------------------------------------------------------*
**                        Dependencies                                        *
**---------------------------------------------------------------------------*/
#include "sci_types.h"
#include "sc8800g_video_header.h"
/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    extern   "C" 
    {
#endif

//#define CHIP_ENDIAN_LITTLE

#if defined(_VSP_)
	#ifdef CHIP_ENDIAN_LITTLE
	#else
		#define H264_BIG_ENDIAN
	#endif
#endif

/**----------------------------------------------------------------------------*
**                           Function Prototype                               **
**----------------------------------------------------------------------------*/

LOCAL int32 H264Dec_get_te (int32 num_ref_idx_active)
{
	int32 ref_frm_id;

	if (num_ref_idx_active == 2)
	{
		ref_frm_id = READ_FLC(1);
		ref_frm_id = 1 - ref_frm_id;
	}else
	{
		ref_frm_id = READ_UE_V();
	}

	return ref_frm_id;
}

LOCAL void H264Dec_read_motionAndRefId_PMB16x16 (DEC_IMAGE_PARAMS_T *img_ptr, DEC_MB_INFO_T *mb_info_ptr, DEC_MB_CACHE_T *mb_cache_ptr)
{
	int16 *ref_mv_ptr = mb_info_ptr->mv_cache + (CONTEXT_CACHE_WIDTH+1)*2;
	int8  *ref_idx_ptr = mb_info_ptr->ref_idx_cache + (CONTEXT_CACHE_WIDTH+1);
	int32 all_zero_ref = mb_cache_ptr->all_zero_ref;
	int32 ref_frm_id = 0;
	int32 num_ref_idx_active = img_ptr->num_ref_idx_l0_active;

	if ((num_ref_idx_active > 1) && (!all_zero_ref))
	{
		int32 ref_pic_num;
		int32 i;

		ref_frm_id = H264Dec_get_te (num_ref_idx_active);

        if (ref_frm_id >= (MAX_REF_FRAME_NUMBER+1))
        {
            img_ptr->error_flag |= ER_REF_FRM_ID;
            img_ptr->return_pos2 |= (1<<8);
            H264Dec_get_HW_status(img_ptr);
			return;
        }

		ref_pic_num = g_list0[ref_frm_id]->mc_ref_pic_num;
		((int32 *)(mb_info_ptr->ref_pic))[0] = (ref_pic_num<<24)|(ref_pic_num<<16)|(ref_pic_num<<8)|(ref_pic_num<<0);

		for (i = 0; i < 4; i++)
		{
			ref_idx_ptr[0] = ref_frm_id;
			ref_idx_ptr[1] = ref_frm_id;
			ref_idx_ptr[2] = ref_frm_id;
			ref_idx_ptr[3] = ref_frm_id;
			ref_idx_ptr += CONTEXT_CACHE_WIDTH;
		}
	}

	ref_mv_ptr[0] = READ_SE_V(); //mvd_x
	ref_mv_ptr[1] = READ_SE_V(); //mvd_y
	
	return;
}

LOCAL void H264Dec_read_motionAndRefId_PMB16x8 (DEC_IMAGE_PARAMS_T *img_ptr, DEC_MB_INFO_T *mb_info_ptr, DEC_MB_CACHE_T *mb_cache_ptr)
{
	int16 *ref_mv_ptr = mb_info_ptr->mv_cache + (CONTEXT_CACHE_WIDTH+1)*2;
	int8  *ref_idx_ptr = mb_info_ptr->ref_idx_cache + (CONTEXT_CACHE_WIDTH+1);
	int32 all_zero_ref = mb_cache_ptr->all_zero_ref;
	int32 ref_frm_id = 0;
	int32 num_ref_idx_active = img_ptr->num_ref_idx_l0_active;
//	int32 mv_num_cnt = 0;

	//read two reference frame index
	if ((num_ref_idx_active > 1) && (!all_zero_ref))
	{
		int32 i;

		for (i = 0; i < 2; i++)
		{
			int32 ref_pic_num;
			int32 j;

			ref_frm_id = H264Dec_get_te (num_ref_idx_active);

            if (ref_frm_id >= (MAX_REF_FRAME_NUMBER+1))
            {
                img_ptr->error_flag |= ER_REF_FRM_ID;
                img_ptr->return_pos2 |= (1<<9);
                H264Dec_get_HW_status(img_ptr);
    			return;
            }
            
			ref_pic_num = g_list0[ref_frm_id]->mc_ref_pic_num;
			mb_info_ptr->ref_pic[2*i]	= ref_pic_num;
			mb_info_ptr->ref_pic[2*i+1] = ref_pic_num;

			for (j = 0; j < 2; j++)
			{
				ref_idx_ptr[0] = ref_frm_id;
				ref_idx_ptr[1] = ref_frm_id;
				ref_idx_ptr[2] = ref_frm_id;
				ref_idx_ptr[3] = ref_frm_id;

				ref_idx_ptr += CONTEXT_CACHE_WIDTH;
			}
		}
	}

	ref_mv_ptr[0] = READ_SE_V(); //mvd_x;
	ref_mv_ptr[1] = READ_SE_V(); //mvd_y;
	ref_mv_ptr[2*CONTEXT_CACHE_WIDTH*2]		= READ_SE_V();//mvd_x;
	ref_mv_ptr[2*CONTEXT_CACHE_WIDTH*2+1]	= READ_SE_V();//mvd_y;

	return;
}

LOCAL void H264Dec_read_motionAndRefId_PMB8x16 (DEC_IMAGE_PARAMS_T *img_ptr, DEC_MB_INFO_T *mb_info_ptr, DEC_MB_CACHE_T *mb_cache_ptr)
{
	int16 *ref_mv_ptr = mb_info_ptr->mv_cache + (CONTEXT_CACHE_WIDTH+1)*2;
	int8  *ref_idx_ptr = mb_info_ptr->ref_idx_cache + (CONTEXT_CACHE_WIDTH+1);
	int32 all_zero_ref = mb_cache_ptr->all_zero_ref;
	int32 ref_frm_id = 0;
	int32 num_ref_idx_active = img_ptr->num_ref_idx_l0_active;
//	int32 mv_num_cnt = 0;

	//read two reference frame index
	if ((num_ref_idx_active > 1) && (!all_zero_ref))
	{
		int32 i;

		for (i = 0; i < 2; i++)
		{
			int32 ref_pic_num;
			int32 j;
			int8 *ref_idx_tmp_ptr = ref_idx_ptr + i*2;

			ref_frm_id = H264Dec_get_te (num_ref_idx_active);

            if (ref_frm_id >= (MAX_REF_FRAME_NUMBER+1))
            {
                img_ptr->error_flag |= ER_REF_FRM_ID;
                img_ptr->return_pos2 |= (1<<(10+i));
                H264Dec_get_HW_status(img_ptr);
    			return;
            }

			ref_pic_num = g_list0[ref_frm_id]->mc_ref_pic_num;
			mb_info_ptr->ref_pic[i]		= ref_pic_num;
			mb_info_ptr->ref_pic[i+2]	= ref_pic_num;

			for (j = 0; j < 4; j++)
			{
				ref_idx_tmp_ptr[0] = ref_frm_id;
				ref_idx_tmp_ptr[1] = ref_frm_id;

				ref_idx_tmp_ptr += CONTEXT_CACHE_WIDTH;
			}
		}
	}

	ref_mv_ptr[0]		= READ_SE_V(); //mvd_x;
	ref_mv_ptr[1]		= READ_SE_V(); //mvd_y;
	ref_mv_ptr[2*2]		= READ_SE_V();//mvd_x;
	ref_mv_ptr[2*2+1]	= READ_SE_V();//mvd_y;

	return;
}

LOCAL void H264Dec_read_motionAndRefId_P8x8 (DEC_IMAGE_PARAMS_T *img_ptr, DEC_MB_INFO_T *mb_info_ptr, DEC_MB_CACHE_T *mb_cache_ptr)
{
	int16 *ref_mv_ptr = mb_info_ptr->mv_cache + (CONTEXT_CACHE_WIDTH+1)*2;
	int8  *ref_idx_ptr = mb_info_ptr->ref_idx_cache + (CONTEXT_CACHE_WIDTH+1);
	int32 all_zero_ref = mb_cache_ptr->all_zero_ref;
//	int32 ref_frm_id = 0;
	int32 num_ref_idx_active = img_ptr->num_ref_idx_l0_active;
	int32 blk8x8Idx = 0;

	//read two reference frame index
	if ((num_ref_idx_active > 1) && (!all_zero_ref))
	{
		int32 ref_frm_id0, ref_frm_id1;
		int32 ref_pic_num;

		ref_frm_id0 = H264Dec_get_te (num_ref_idx_active);
        if (ref_frm_id0 >= (MAX_REF_FRAME_NUMBER+1))
        {
            img_ptr->error_flag |= ER_REF_FRM_ID;
            img_ptr->return_pos2 |= (1<<12);
            H264Dec_get_HW_status(img_ptr);
			return;
        }
		ref_pic_num = g_list0[ref_frm_id0]->mc_ref_pic_num;
		mb_info_ptr->ref_pic[0]		= ref_pic_num;

		ref_frm_id1 = H264Dec_get_te (num_ref_idx_active);
        if (ref_frm_id1 >= (MAX_REF_FRAME_NUMBER+1))
        {
            img_ptr->error_flag |= ER_REF_FRM_ID;
            img_ptr->return_pos2 |= (1<<13);
            H264Dec_get_HW_status(img_ptr);
			return;
        }
		ref_pic_num = g_list0[ref_frm_id1]->mc_ref_pic_num;
		mb_info_ptr->ref_pic[1]		= ref_pic_num;

		ref_idx_ptr[0] = ref_frm_id0; ref_idx_ptr[1] = ref_frm_id0;
		ref_idx_ptr[2] = ref_frm_id1; ref_idx_ptr[3] = ref_frm_id1;

		ref_idx_ptr[0+CONTEXT_CACHE_WIDTH] = ref_frm_id0;
		ref_idx_ptr[1+CONTEXT_CACHE_WIDTH] = ref_frm_id0;
		ref_idx_ptr[2+CONTEXT_CACHE_WIDTH] = ref_frm_id1;
		ref_idx_ptr[3+CONTEXT_CACHE_WIDTH] = ref_frm_id1;

		ref_frm_id0 = H264Dec_get_te (num_ref_idx_active);
        if (ref_frm_id0 >= (MAX_REF_FRAME_NUMBER+1))
        {
            img_ptr->error_flag |= ER_REF_FRM_ID;
            img_ptr->return_pos2 |= (1<<14);
            H264Dec_get_HW_status(img_ptr);
			return;
        }
		ref_pic_num = g_list0[ref_frm_id0]->mc_ref_pic_num;
		mb_info_ptr->ref_pic[2]		= ref_pic_num;

		ref_frm_id1 = H264Dec_get_te (num_ref_idx_active);
        if (ref_frm_id1 >= (MAX_REF_FRAME_NUMBER+1))
        {
            img_ptr->error_flag |= ER_REF_FRM_ID;
            img_ptr->return_pos2 |= (1<<15);
            H264Dec_get_HW_status(img_ptr);
			return;
        }
		ref_pic_num = g_list0[ref_frm_id1]->mc_ref_pic_num;
		mb_info_ptr->ref_pic[3]		= ref_pic_num;

		ref_idx_ptr[0+2*CONTEXT_CACHE_WIDTH] = ref_frm_id0;
		ref_idx_ptr[1+2*CONTEXT_CACHE_WIDTH] = ref_frm_id0;
		ref_idx_ptr[2+2*CONTEXT_CACHE_WIDTH] = ref_frm_id1;
		ref_idx_ptr[3+2*CONTEXT_CACHE_WIDTH] = ref_frm_id1;

		ref_idx_ptr[0+3*CONTEXT_CACHE_WIDTH] = ref_frm_id0;
		ref_idx_ptr[1+3*CONTEXT_CACHE_WIDTH] = ref_frm_id0;
		ref_idx_ptr[2+3*CONTEXT_CACHE_WIDTH] = ref_frm_id1;
		ref_idx_ptr[3+3*CONTEXT_CACHE_WIDTH] = ref_frm_id1;		
	}

	for (blk8x8Idx = 0; blk8x8Idx < 4; blk8x8Idx++)
	{
		int32 blk_offset = 2*CONTEXT_CACHE_WIDTH*2*(blk8x8Idx>>1)+2*2*(blk8x8Idx&1);
		int16 *mv_tmp_ptr = ref_mv_ptr+blk_offset;
		
		switch(mb_cache_ptr->b8mode[blk8x8Idx])
		{
		case PMB8X8_BLOCK8X8:
			mv_tmp_ptr[0] = READ_SE_V(); //mvd_x;
			mv_tmp_ptr[1] = READ_SE_V(); //mvd_y;
			break;
		case PMB8X8_BLOCK8X4:
			mv_tmp_ptr[0] = READ_SE_V(); //mvd_x;
			mv_tmp_ptr[1] = READ_SE_V(); //mvd_y;
			mv_tmp_ptr[CONTEXT_CACHE_WIDTH*2]	 = READ_SE_V(); //mvd_x;
			mv_tmp_ptr[CONTEXT_CACHE_WIDTH*2+1] = READ_SE_V(); //mvd_y;
			break;
		case PMB8X8_BLOCK4X8:
			mv_tmp_ptr[0] = READ_SE_V(); //mvd_x;
			mv_tmp_ptr[1] = READ_SE_V(); //mvd_y;
			mv_tmp_ptr[1*2]	 = READ_SE_V(); //mvd_x;
			mv_tmp_ptr[1*2+1] = READ_SE_V(); //mvd_y;
			break;
		case PMB8X8_BLOCK4X4:
			mv_tmp_ptr[0] = READ_SE_V(); //mvd_x;
			mv_tmp_ptr[1] = READ_SE_V(); //mvd_y;
			mv_tmp_ptr[1*2]	  = READ_SE_V(); //mvd_x;
			mv_tmp_ptr[1*2+1] = READ_SE_V(); //mvd_y;
			mv_tmp_ptr[CONTEXT_CACHE_WIDTH*2]	 = READ_SE_V(); //mvd_x;
			mv_tmp_ptr[CONTEXT_CACHE_WIDTH*2+1] = READ_SE_V(); //mvd_y;
			mv_tmp_ptr[(CONTEXT_CACHE_WIDTH+1)*2]	 = READ_SE_V(); //mvd_x;
			mv_tmp_ptr[(CONTEXT_CACHE_WIDTH+1)*2+1] = READ_SE_V(); //mvd_y;
			break;
		default:
			break;
		}
	}

	return;
}

PUBLIC void H264Dec_read_motionAndRefId (DEC_IMAGE_PARAMS_T *img_ptr, DEC_MB_INFO_T *mb_info_ptr, DEC_MB_CACHE_T *mb_cache_ptr)
{
	READ_REG_POLL (VSP_BSM_REG_BASE+BSM_DEBUG_OFF, V_BIT_3, V_BIT_3, TIME_OUT_CLK,
		"polling bsm fifo depth >= 8 words for h264 header");
	
	switch (mb_info_ptr->mb_type)
	{
	case PMB16x16:
		H264Dec_read_motionAndRefId_PMB16x16 (img_ptr, mb_info_ptr, mb_cache_ptr);
		break;
	case PMB16x8:
		H264Dec_read_motionAndRefId_PMB16x8 (img_ptr, mb_info_ptr, mb_cache_ptr);
		break;
	case PMB8x16:
		H264Dec_read_motionAndRefId_PMB8x16 (img_ptr, mb_info_ptr, mb_cache_ptr);
		break;
	case P8x8:
		H264Dec_read_motionAndRefId_P8x8 (img_ptr, mb_info_ptr, mb_cache_ptr);
		break;
	default:
		break;
	}
	
	return;
}

LOCAL int32 H264Dec_pred_motion (int8 *ref_idx_ptr, int16 *ref_mv_ptr, int32 blk_width)
{
	int32 ref_idx_a, ref_idx_b, ref_idx_c;
	int32 mv_a_x, mv_b_x, mv_c_x;
	int32 mv_a_y, mv_b_y, mv_c_y;
	int32 pred_mv;
	int32 ref_idx = ref_idx_ptr[0];
	int32 match_cnt;

	//fetch diagonal mv and reference frame index
	int32 blk_pos = (-CONTEXT_CACHE_WIDTH + blk_width);
	ref_idx_c = ref_idx_ptr[blk_pos];

	if (ref_idx_c == PART_NOT_AVAIL)
	{
		ref_idx_c = ref_idx_ptr[-(CONTEXT_CACHE_WIDTH+1)];
		mv_c_x = (ref_mv_ptr-(CONTEXT_CACHE_WIDTH+1)*2)[0];
		mv_c_y = (ref_mv_ptr-(CONTEXT_CACHE_WIDTH+1)*2)[1];
	}else
	{
		mv_c_x = (ref_mv_ptr+blk_pos*2)[0];
		mv_c_y = (ref_mv_ptr+blk_pos*2)[1];
	}

	//left block ref and mv
	ref_idx_a = ref_idx_ptr[-1];
	mv_a_x = (ref_mv_ptr-1*2)[0];
	mv_a_y = (ref_mv_ptr-1*2)[1];

	//top block ref and mv
	ref_idx_b = ref_idx_ptr[-CONTEXT_CACHE_WIDTH];
	mv_b_x = (ref_mv_ptr-CONTEXT_CACHE_WIDTH*2)[0];
	mv_b_y = (ref_mv_ptr-CONTEXT_CACHE_WIDTH*2)[1];

	match_cnt = ((ref_idx == ref_idx_a) ? 1 : 0) + ((ref_idx == ref_idx_b) ? 1 : 0) + ((ref_idx == ref_idx_c) ? 1 : 0);

	if (match_cnt == 0)
	{
		if (!((ref_idx_b == PART_NOT_AVAIL) && (ref_idx_c == PART_NOT_AVAIL) && (ref_idx_a != PART_NOT_AVAIL)))
		{
			int32 tmp;

			pred_mv = MEDIAN(mv_a_x, mv_b_x, mv_c_x);
			tmp = MEDIAN(mv_a_y, mv_b_y, mv_c_y);
			pred_mv = ((pred_mv<<16)|tmp&0xffff);
		}else
		{
			pred_mv = ((mv_a_x << 16) | (mv_a_y &0xffff));
		}
	}else if (match_cnt > 1)
	{
		int32 tmp;

		pred_mv = MEDIAN(mv_a_x, mv_b_x, mv_c_x);
		tmp = MEDIAN(mv_a_y, mv_b_y, mv_c_y);
		pred_mv = ((pred_mv<<16)|tmp&0xffff);
	}else
	{
		if (ref_idx == ref_idx_a)
		{
			pred_mv = ((mv_a_x << 16) | (mv_a_y &0xffff));
		}else if (ref_idx == ref_idx_b)
		{
			pred_mv = ((mv_b_x << 16) | (mv_b_y &0xffff));
		}else // (ref_idx == ref_idx_c)
		{
			pred_mv = ((mv_c_x << 16) | (mv_c_y &0xffff));
		}
	}

	return pred_mv;
}

LOCAL int32 H264Dec_pred16x8_motion (DEC_MB_INFO_T *mb_info_ptr, int32 blkIdx)
{
	int32 blk_pos;
	int16 *ref_mv_ptr = mb_info_ptr->mv_cache;
	int8  *ref_idx_ptr = mb_info_ptr->ref_idx_cache;
	int32 ref_id = ref_idx_ptr[blkIdx];
	int32 pred_mv_x, pred_mv_y, pred_mv;

	if (blkIdx == (CONTEXT_CACHE_WIDTH+1)) //the top 16x8 partition
	{
		blk_pos = blkIdx - CONTEXT_CACHE_WIDTH;

		if (ref_id == ref_idx_ptr[blk_pos])
		{
			pred_mv_x = ref_mv_ptr[blk_pos*2];
			pred_mv_y = ref_mv_ptr[blk_pos*2+1];
			pred_mv = (pred_mv_x << 16)|(pred_mv_y&0xffff);

			return pred_mv;
		}
	}else
	{
		blk_pos = blkIdx - 1;

		if (ref_id == ref_idx_ptr[blk_pos])
		{
			pred_mv_x = ref_mv_ptr[blk_pos*2];
			pred_mv_y = ref_mv_ptr[blk_pos*2+1];
			pred_mv = (pred_mv_x << 16)|(pred_mv_y&0xffff);

			return pred_mv;
		}
	}

	pred_mv = H264Dec_pred_motion (ref_idx_ptr+blkIdx, ref_mv_ptr+2*blkIdx, 4);

	return pred_mv;
}

LOCAL int32 H264Dec_pred8x16_motion (DEC_MB_INFO_T *mb_info_ptr, int32 blkIdx)
{
	int32 blk_pos;
	int16 *ref_mv_ptr = mb_info_ptr->mv_cache;
	int8  *ref_idx_ptr = mb_info_ptr->ref_idx_cache;
	int32 ref_id = ref_idx_ptr[blkIdx];
	int32 pred_mv_x, pred_mv_y, pred_mv;

	if (blkIdx == (CONTEXT_CACHE_WIDTH+1)) //the left 8x16 partition
	{
		blk_pos = blkIdx - 1;

		if (ref_id == ref_idx_ptr[blk_pos])
		{
			pred_mv_x = ref_mv_ptr[blk_pos*2];
			pred_mv_y = ref_mv_ptr[blk_pos*2+1];
			pred_mv = (pred_mv_x << 16)|(pred_mv_y&0xffff);

			return pred_mv;
		}
	}else
	{
		//diagonal block
		int32 ref_idx_c;

		blk_pos = blkIdx - CONTEXT_CACHE_WIDTH + 2;
		ref_idx_c = ref_idx_ptr[blk_pos];

		if (ref_idx_c == PART_NOT_AVAIL)
		{
			blk_pos = blkIdx - (CONTEXT_CACHE_WIDTH+1);
			ref_idx_c = ref_idx_ptr[blk_pos];
		}

		if (ref_id == ref_idx_c)
		{
			pred_mv_x = ref_mv_ptr[blk_pos*2];
			pred_mv_y = ref_mv_ptr[blk_pos*2+1];
			pred_mv = (pred_mv_x << 16)|(pred_mv_y&0xffff);

			return pred_mv;
		}
	}

	pred_mv = H264Dec_pred_motion (ref_idx_ptr+blkIdx, ref_mv_ptr+2*blkIdx, 2);

	return pred_mv;
}

//pred_mv = mv_x<<16 | mv_y
LOCAL void H264Dec_mv_prediction_skipped (DEC_IMAGE_PARAMS_T *img_ptr, DEC_MB_INFO_T *mb_info_ptr, DEC_MB_CACHE_T *mb_cache_ptr)
{
	int8  *ref_idx_ptr = mb_info_ptr->ref_idx_cache;
	int16 *ref_mv_ptr = mb_info_ptr->mv_cache;
	int32 *int_ref_mv_ptr;
	int32 msk = 0xffff;
	int32 top_ref_idx = ref_idx_ptr[1];
	int32 left_ref_idx = ref_idx_ptr[CONTEXT_CACHE_WIDTH];
	int32 i;
	int32 pred_mv;
	int32 mv_x, mv_y;
	int32 mv_xy, mv_cmd;

	if ((top_ref_idx == PART_NOT_AVAIL) || (left_ref_idx == PART_NOT_AVAIL) ||
		((top_ref_idx == 0) && (((uint32 *)ref_mv_ptr)[1] == 0)) ||
		((left_ref_idx == 0) && (((uint32 *)ref_mv_ptr)[CONTEXT_CACHE_WIDTH] == 0)))
	{
		mv_x = 0;
		mv_y = 0;
	}else
	{
		pred_mv = H264Dec_pred_motion(ref_idx_ptr+CONTEXT_CACHE_WIDTH+1, ref_mv_ptr+(CONTEXT_CACHE_WIDTH+1)*2,4);
		mv_x = (pred_mv >> 16);
		mv_y = ((pred_mv << 16) >> 16);
	}

	mv_cmd = (mv_y << 16) | (mv_x & msk);
#if defined(H264_BIG_ENDIAN)
	mv_xy  = (mv_x << 16) | (mv_y & msk);
#else
	mv_xy  = (mv_y << 16) | (mv_x & msk);
#endif

	//set mv cache
	int_ref_mv_ptr = (int32 *)(ref_mv_ptr+(CONTEXT_CACHE_WIDTH+1)*2);
	for (i = 0; i < 4; i++)
	{
		int_ref_mv_ptr[0] = mv_xy;
		int_ref_mv_ptr[1] = mv_xy;
		int_ref_mv_ptr[2] = mv_xy;
		int_ref_mv_ptr[3] = mv_xy;

		int_ref_mv_ptr += CONTEXT_CACHE_WIDTH;
	}

	//configure mv command
	{
		int32 ref_blk_end = TRUE;
		int32 ref_bir_blk = FALSE;
		int32 ref_cmd_type = 0;
		int32 ref_bw_frame_id = 0;
		int32 ref_fw_frame_id = mb_info_ptr->ref_idx_cache[1*CONTEXT_CACHE_WIDTH+1];//mb_info_ptr->ref_pic[0];
		int32 ref_blk_id = 0;
		int32 ref_blk_size = MC_BLKSIZE_16x16;
		int32 mc_blk_info;
		
		mc_blk_info = (ref_blk_end << 27) | (ref_bir_blk << 26) | (ref_cmd_type << 24) | (ref_bw_frame_id << 20)|
			(ref_fw_frame_id << 16) | (ref_blk_id << 8) | (ref_blk_size & 0xff);
		VSP_WRITE_REG(VSP_MCA_REG_BASE+MCA_BLK_CBUF_OFF, mc_blk_info, "configure MCA command buffer, block info");
		VSP_WRITE_REG(VSP_MCA_REG_BASE+MCA_MV_CBUF_OFF, mv_cmd, "configure MCA command buffer, block mv info");
	}

	return;
}

LOCAL void H264Dec_mv_prediction_PMB16x16 (DEC_IMAGE_PARAMS_T *img_ptr, DEC_MB_INFO_T *mb_info_ptr, DEC_MB_CACHE_T *mb_cache_ptr)
{
	int8  *ref_idx_ptr = mb_info_ptr->ref_idx_cache+(CONTEXT_CACHE_WIDTH+1);
	int16 *ref_mv_ptr = mb_info_ptr->mv_cache+(CONTEXT_CACHE_WIDTH+1)*2;
	int32 msk = 0xffff;
//	int32 top_ref_idx = ref_idx_ptr[1];
//	int32 left_ref_idx = ref_idx_ptr[CONTEXT_CACHE_WIDTH];
	int32 i;
	int32 mvd_x, mvd_y;
	int32 mv_x, mv_y, mv_xy;
	int32 pred_mv_x, pred_mv_y;
	int32 pred_mv;
	int32 mv_cmd;

	mvd_x = ref_mv_ptr[0];
	mvd_y = ref_mv_ptr[1];
	pred_mv = H264Dec_pred_motion (ref_idx_ptr, ref_mv_ptr, 4);
	pred_mv_x = (pred_mv >> 16);
	pred_mv_y = ((pred_mv << 16) >> 16);

	mv_x = mvd_x + pred_mv_x;
	mv_y = mvd_y + pred_mv_y;

	mv_cmd = (mv_y << 16) | (mv_x & msk);
#if defined(H264_BIG_ENDIAN)
	mv_xy  = (mv_x << 16) | (mv_y & msk);
#else
	mv_xy  = (mv_y << 16) | (mv_x & msk);
#endif

	//set mv cache
	for (i = 0; i < 4; i++)
	{
		((int32 *)ref_mv_ptr)[0] = mv_xy;
		((int32 *)ref_mv_ptr)[1] = mv_xy;
		((int32 *)ref_mv_ptr)[2] = mv_xy;
		((int32 *)ref_mv_ptr)[3] = mv_xy;

		ref_mv_ptr += (CONTEXT_CACHE_WIDTH*2);
	}

	//configure mv command
	{
		int32 ref_blk_end = TRUE;
		int32 ref_bir_blk = FALSE;
		int32 ref_cmd_type = 0;
		int32 ref_bw_frame_id = 0;
		int32 ref_fw_frame_id = ref_idx_ptr[0];//mb_info_ptr->ref_pic[0];
		int32 ref_blk_id = 0;
		int32 ref_blk_size = MC_BLKSIZE_16x16;
		int32 mc_blk_info;
		
		mc_blk_info = (ref_blk_end << 27) | (ref_bir_blk << 26) | (ref_cmd_type << 24) | (ref_bw_frame_id << 20)|
			(ref_fw_frame_id << 16) | (ref_blk_id << 8) | (ref_blk_size & 0xff);
		VSP_WRITE_REG(VSP_MCA_REG_BASE+MCA_BLK_CBUF_OFF, mc_blk_info, "configure MCA command buffer, block info");
		VSP_WRITE_REG(VSP_MCA_REG_BASE+MCA_MV_CBUF_OFF, mv_cmd, "configure MCA command buffer, block mv info");
	}

	return;	
}

LOCAL void H264Dec_mv_prediction_PMB16x8 (DEC_IMAGE_PARAMS_T *img_ptr, DEC_MB_INFO_T *mb_info_ptr, DEC_MB_CACHE_T *mb_cache_ptr)
{
	int8  *ref_idx_ptr = mb_info_ptr->ref_idx_cache+(CONTEXT_CACHE_WIDTH+1);
	int16 *ref_mv_ptr = mb_info_ptr->mv_cache+(CONTEXT_CACHE_WIDTH+1)*2;
	int32 msk = 0xffff;
//	int32 top_ref_idx = ref_idx_ptr[1];
//	int32 left_ref_idx = ref_idx_ptr[CONTEXT_CACHE_WIDTH];
	int32 i, j;
	int32 mvd_x = 0;
	int32 mvd_y = 0;
	int32 mv_x, mv_y;
	int32 mv_xy;
	int32 pred_mv_x, pred_mv_y;
	int32 pred_mv;
	int32 mv_cmd;

	for (j = 0; j < 2; j++)
	{
		mvd_y = (ref_mv_ptr[1]);
		mvd_x = (ref_mv_ptr[0]);
		pred_mv = H264Dec_pred16x8_motion (mb_info_ptr, CONTEXT_CACHE_WIDTH+1+j*2*CONTEXT_CACHE_WIDTH);
		pred_mv_x = (pred_mv >> 16);
		pred_mv_y = ((pred_mv << 16) >> 16);

		mv_x = mvd_x + pred_mv_x;
		mv_y = mvd_y + pred_mv_y;

		mv_cmd = (mv_y << 16) | (mv_x & msk);
	#if defined(H264_BIG_ENDIAN)
		mv_xy  = (mv_x << 16) | (mv_y & msk);
	#else
		mv_xy  = (mv_y << 16) | (mv_x & msk);
	#endif

		//set mv cache
		for (i = 0; i < 2; i++)
		{
			((int32 *)ref_mv_ptr)[0] = mv_xy;
			((int32 *)ref_mv_ptr)[1] = mv_xy;
			((int32 *)ref_mv_ptr)[2] = mv_xy;
			((int32 *)ref_mv_ptr)[3] = mv_xy;

			ref_mv_ptr += (CONTEXT_CACHE_WIDTH*2);
		}

		//configure mv command
		{
			int32 ref_blk_end = FALSE;
			int32 ref_bir_blk = FALSE;
			int32 ref_cmd_type = 0;
			int32 ref_bw_frame_id = 0;
			int32 ref_fw_frame_id = ref_idx_ptr[0];//mb_info_ptr->ref_pic[2*j];
			int32 ref_blk_id = 2*j;
			int32 ref_blk_size = MC_BLKSIZE_8x8;
			int32 mc_blk_info;
			
			mc_blk_info = (ref_blk_end << 27) | (ref_bir_blk << 26) | (ref_cmd_type << 24) | (ref_bw_frame_id << 20)|
				(ref_fw_frame_id << 16) | (ref_blk_id << 8) | (ref_blk_size & 0xff);
			VSP_WRITE_REG(VSP_MCA_REG_BASE+MCA_BLK_CBUF_OFF, mc_blk_info, "configure MCA command buffer, block info");
			VSP_WRITE_REG(VSP_MCA_REG_BASE+MCA_MV_CBUF_OFF, mv_cmd, "configure MCA command buffer, block mv info");

			if (j == 1)
			{
				ref_blk_end = TRUE;
			}
			ref_blk_id = (2*j+1);
			ref_fw_frame_id = ref_idx_ptr[2];
			ref_idx_ptr += 2*CONTEXT_CACHE_WIDTH;

			mc_blk_info = (ref_blk_end << 27) | (ref_bir_blk << 26) | (ref_cmd_type << 24) | (ref_bw_frame_id << 20)|
				(ref_fw_frame_id << 16) | (ref_blk_id << 8) | (ref_blk_size & 0xff);
			VSP_WRITE_REG(VSP_MCA_REG_BASE+MCA_BLK_CBUF_OFF, mc_blk_info, "configure MCA command buffer, block info");
			VSP_WRITE_REG(VSP_MCA_REG_BASE+MCA_MV_CBUF_OFF, mv_cmd, "configure MCA command buffer, block mv info");
		}	
	}

	return;	
}

LOCAL void H264Dec_mv_prediction_PMB8x16 (DEC_IMAGE_PARAMS_T *img_ptr, DEC_MB_INFO_T *mb_info_ptr, DEC_MB_CACHE_T *mb_cache_ptr)
{
	int8  *ref_idx_ptr = mb_info_ptr->ref_idx_cache+(CONTEXT_CACHE_WIDTH+1);
	int16 *ref_mv_ptr = mb_info_ptr->mv_cache+(CONTEXT_CACHE_WIDTH+1)*2;
	int32 msk = 0xffff;
//	int32 top_ref_idx = ref_idx_ptr[1];
//	int32 left_ref_idx = ref_idx_ptr[CONTEXT_CACHE_WIDTH];
	int32 i, j;
	int32 mvd_x, mvd_y;
	int32 mv_x, mv_y, mv_xy;
	int32 pred_mv_x, pred_mv_y;
	int32 pred_mv;
	int32 mv_cmd;

	for (j = 0; j < 2; j++)
	{
		int16 *mv_tmp_ptr = ref_mv_ptr + j*2*2;
		mvd_x = mv_tmp_ptr[0];
		mvd_y = mv_tmp_ptr[1];
		pred_mv = H264Dec_pred8x16_motion (mb_info_ptr, CONTEXT_CACHE_WIDTH+1+j*2);
		pred_mv_x = (pred_mv >> 16);
		pred_mv_y = ((pred_mv << 16) >> 16);

		mv_x = mvd_x + pred_mv_x;
		mv_y = mvd_y + pred_mv_y;

	#if defined(H264_BIG_ENDIAN)
		mv_xy  = (mv_x << 16) | (mv_y & msk);
		mv_cmd = (mv_y << 16) | (mv_x & msk);
	#else
		mv_xy  = (mv_y << 16) | (mv_x & msk);
		mv_cmd = mv_xy;
 	#endif

		//set mv cache
		for (i = 0; i < 4; i++)
		{
			((int32 *)mv_tmp_ptr)[0] = mv_xy;
			((int32 *)mv_tmp_ptr)[1] = mv_xy;

			mv_tmp_ptr += (CONTEXT_CACHE_WIDTH*2);
		}

		//configure mv command
		{
			int32 ref_blk_end = FALSE;
			int32 ref_bir_blk = FALSE;
			int32 ref_cmd_type = 0;
			int32 ref_bw_frame_id = 0;
			int32 ref_fw_frame_id = ref_idx_ptr[0];//mb_info_ptr->ref_pic[j];
			int32 ref_blk_id = j;
			int32 ref_blk_size = MC_BLKSIZE_8x8;
			int32 mc_blk_info;
			
			mc_blk_info = (ref_blk_end << 27) | (ref_bir_blk << 26) | (ref_cmd_type << 24) | (ref_bw_frame_id << 20)|
				(ref_fw_frame_id << 16) | (ref_blk_id << 8) | (ref_blk_size & 0xff);
			VSP_WRITE_REG(VSP_MCA_REG_BASE+MCA_BLK_CBUF_OFF, mc_blk_info, "configure MCA command buffer, block info");
			VSP_WRITE_REG(VSP_MCA_REG_BASE+MCA_MV_CBUF_OFF, mv_cmd, "configure MCA command buffer, block mv info");

			if (j == 1)
			{
				ref_blk_end = TRUE;
			}
			ref_blk_id = (j+2);
			ref_fw_frame_id = ref_idx_ptr[2*CONTEXT_CACHE_WIDTH];
			ref_idx_ptr+=2;

			mc_blk_info = (ref_blk_end << 27) | (ref_bir_blk << 26) | (ref_cmd_type << 24) | (ref_bw_frame_id << 20)|
				(ref_fw_frame_id << 16) | (ref_blk_id << 8) | (ref_blk_size & 0xff);
			VSP_WRITE_REG(VSP_MCA_REG_BASE+MCA_BLK_CBUF_OFF, mc_blk_info, "configure MCA command buffer, block info");
			VSP_WRITE_REG(VSP_MCA_REG_BASE+MCA_MV_CBUF_OFF, mv_cmd, "configure MCA command buffer, block mv info");
		}	
	}

	return;	
}
PUBLIC void H264Dec_mv_prediction_P8x8_8x8 (DEC_MB_INFO_T *mb_info_ptr, int8 *ref_idx_ptr, int16 *ref_mv_ptr, int32 b8)
{
//	int32 b8_offset_y = (b8 & 2);
//	int32 b8_offset_x = (b8 << 1)&2;
	int32 mvd_x, mvd_y;
	int32 pred_mv_x, pred_mv_y, pred_mv;
	int32 mv_x, mv_y;
	int32 mv_xy, mv_cmd;

	mvd_x = ref_mv_ptr[0];
	mvd_y = ref_mv_ptr[1];
	pred_mv = H264Dec_pred_motion (ref_idx_ptr, ref_mv_ptr, 2);

	pred_mv_x = pred_mv >> 16;
	pred_mv_y = (pred_mv << 16)>>16;

	mv_x = mvd_x + pred_mv_x;
	mv_y = mvd_y + pred_mv_y;

	mv_cmd = (mv_y<<16)|(mv_x&0xffff);
#if defined(H264_BIG_ENDIAN)
	mv_xy = (mv_x<<16)|(mv_y&0xffff);
#else
	mv_xy = (mv_y<<16)|(mv_x&0xffff);
#endif

	((int32 *)(ref_mv_ptr))[0] = mv_xy;
	((int32 *)(ref_mv_ptr))[1] = mv_xy;
	((int32 *)(ref_mv_ptr))[CONTEXT_CACHE_WIDTH] = mv_xy;
	((int32 *)(ref_mv_ptr))[CONTEXT_CACHE_WIDTH+1] = mv_xy;

	//configure mv command
	{
		int32 ref_blk_end = FALSE;
		int32 ref_bir_blk = FALSE;
		int32 ref_cmd_type = 0;
		int32 ref_bw_frame_id = 0;
		int32 ref_fw_frame_id = ref_idx_ptr[0];//mb_info_ptr->ref_pic[b8];
		int32 ref_blk_id = b8;
		int32 ref_blk_size = MC_BLKSIZE_8x8;
		int32 mc_blk_info;

		if (b8 == 3)
		{
			ref_blk_end = TRUE;
		}else
		{
			ref_blk_end = FALSE;
		}

		ref_bir_blk = FALSE;
		ref_cmd_type = 0;
		ref_bw_frame_id = 0;
		ref_fw_frame_id = ref_idx_ptr[0]; //(mb_info_ptr->ref_pic[b8]);
		ref_blk_id = b8;
		ref_blk_size = MC_BLKSIZE_8x8;

		mc_blk_info = (ref_blk_end << 27) | (ref_bir_blk << 26) | (ref_cmd_type << 24) | (ref_bw_frame_id << 20)|
				(ref_fw_frame_id << 16) | (ref_blk_id << 8) | (ref_blk_size & 0xff);
		VSP_WRITE_REG(VSP_MCA_REG_BASE+MCA_BLK_CBUF_OFF, mc_blk_info, "configure MCA command buffer, block info");
		VSP_WRITE_REG(VSP_MCA_REG_BASE+MCA_MV_CBUF_OFF, mv_cmd, "configure MCA command buffer, block mv info");
	}
	
	return;
}

PUBLIC void H264Dec_mv_prediction_P8x8_8x4 (DEC_MB_INFO_T *mb_info_ptr, int8 *ref_idx_ptr, int16 *ref_mv_ptr, int32 b8)
{
//	int32 b8_offset_y = (b8 & 2);
//	int32 b8_offset_x = (b8 << 1)&2;
	int32 mvd_x, mvd_y;
	int32 pred_mv_x, pred_mv_y, pred_mv;
	int32 mv_x, mv_y;
	int32 mv_xy, mv_cmd;
	int32 i;

	for (i = 0; i < 2; i++)
	{
		mvd_x = ref_mv_ptr[0];
		mvd_y = ref_mv_ptr[1];
		pred_mv = H264Dec_pred_motion (ref_idx_ptr, ref_mv_ptr, 2);
		pred_mv_x = pred_mv >> 16;
		pred_mv_y = (pred_mv << 16)>>16;

		mv_x = mvd_x + pred_mv_x;
		mv_y = mvd_y + pred_mv_y;

		mv_cmd = (mv_y<<16)|(mv_x&0xffff);
	#if defined(H264_BIG_ENDIAN)
		mv_xy = (mv_x<<16)|(mv_y&0xffff);
	#else
		mv_xy = (mv_y<<16)|(mv_x&0xffff);
 	#endif

		((int32 *)(ref_mv_ptr))[0] = mv_xy;
		((int32 *)(ref_mv_ptr))[1] = mv_xy;

		ref_mv_ptr  += CONTEXT_CACHE_WIDTH*2;

		//configure mv command
		{
			int32 ref_blk_end = FALSE;
			int32 ref_bir_blk = FALSE;
			int32 ref_cmd_type = 0;
			int32 ref_bw_frame_id = 0;
			int32 ref_fw_frame_id = ref_idx_ptr[0];//mb_info_ptr->ref_pic[b8];
			int32 ref_blk_id = b8*4+i*2;
			int32 ref_blk_size = MC_BLKSIZE_4x4;
			int32 mc_blk_info;

			mc_blk_info = (ref_blk_end << 27) | (ref_bir_blk << 26) | (ref_cmd_type << 24) | (ref_bw_frame_id << 20)|
					(ref_fw_frame_id << 16) | (ref_blk_id << 8) | (ref_blk_size & 0xff);
			VSP_WRITE_REG(VSP_MCA_REG_BASE+MCA_BLK_CBUF_OFF, mc_blk_info, "configure MCA command buffer, block info");
			VSP_WRITE_REG(VSP_MCA_REG_BASE+MCA_MV_CBUF_OFF, mv_cmd, "configure MCA command buffer, block mv info");

			if ((b8 == 3) && (i == 1))
			{
				ref_blk_end = TRUE;
			}else
			{
				ref_blk_end = FALSE;
			}
			ref_blk_id = b8*4+i*2 + 1;
			ref_fw_frame_id = ref_idx_ptr[1];
			ref_idx_ptr += CONTEXT_CACHE_WIDTH;

			mc_blk_info = (ref_blk_end << 27) | (ref_bir_blk << 26) | (ref_cmd_type << 24) | (ref_bw_frame_id << 20)|
					(ref_fw_frame_id << 16) | (ref_blk_id << 8) | (ref_blk_size & 0xff);
			VSP_WRITE_REG(VSP_MCA_REG_BASE+MCA_BLK_CBUF_OFF, mc_blk_info, "configure MCA command buffer, block info");
			VSP_WRITE_REG(VSP_MCA_REG_BASE+MCA_MV_CBUF_OFF, mv_cmd, "configure MCA command buffer, block mv info");
		}
	}
	
	return;
}

PUBLIC void H264Dec_mv_prediction_P8x8_4x8 (DEC_MB_INFO_T *mb_info_ptr, int8 *ref_idx_ptr, int16 *ref_mv_ptr, int32 b8)
{
//	int32 b8_offset_y = (b8 & 2);
//	int32 b8_offset_x = (b8 << 1)&2;
	int32 mvd_x, mvd_y;
	int32 pred_mv_x, pred_mv_y, pred_mv;
	int32 mv_x, mv_y;
	int32 mv_xy, mv_cmd;
	int32 i;

	for (i = 0; i < 2; i++)
	{
		mvd_x = ref_mv_ptr[0];
		mvd_y = ref_mv_ptr[1];
		pred_mv = H264Dec_pred_motion (ref_idx_ptr, ref_mv_ptr, 1);
		pred_mv_x = pred_mv >> 16;
		pred_mv_y = (pred_mv << 16)>>16;

		mv_x = mvd_x + pred_mv_x;
		mv_y = mvd_y + pred_mv_y;

		mv_cmd = (mv_y<<16)|(mv_x&0xffff);
	#if defined(H264_BIG_ENDIAN)
		mv_xy = (mv_x<<16)|(mv_y&0xffff);
	#else
		mv_xy = (mv_y<<16)|(mv_x&0xffff);
 	#endif

		((int32 *)(ref_mv_ptr))[0] = mv_xy;
		((int32 *)(ref_mv_ptr))[CONTEXT_CACHE_WIDTH] = mv_xy;

		ref_mv_ptr  += (1*2);

		//configure mv command
		{
			int32 ref_blk_end = FALSE;
			int32 ref_bir_blk = FALSE;
			int32 ref_cmd_type = 0;
			int32 ref_bw_frame_id = 0;
			int32 ref_fw_frame_id = ref_idx_ptr[0];//mb_info_ptr->ref_pic[b8];
			int32 ref_blk_id = b8*4+i;
			int32 ref_blk_size = MC_BLKSIZE_4x4;
			int32 mc_blk_info;

			mc_blk_info = (ref_blk_end << 27) | (ref_bir_blk << 26) | (ref_cmd_type << 24) | (ref_bw_frame_id << 20)|
					(ref_fw_frame_id << 16) | (ref_blk_id << 8) | (ref_blk_size & 0xff);
			VSP_WRITE_REG(VSP_MCA_REG_BASE+MCA_BLK_CBUF_OFF, mc_blk_info, "configure MCA command buffer, block info");
			VSP_WRITE_REG(VSP_MCA_REG_BASE+MCA_MV_CBUF_OFF, mv_cmd, "configure MCA command buffer, block mv info");

			if ((b8 == 3) && (i == 1))
			{
				ref_blk_end = TRUE;
			}else
			{
				ref_blk_end = FALSE;
			}
			ref_blk_id = b8*4+i+2;
			ref_fw_frame_id = ref_idx_ptr[CONTEXT_CACHE_WIDTH];
			ref_idx_ptr+=1;

			mc_blk_info = (ref_blk_end << 27) | (ref_bir_blk << 26) | (ref_cmd_type << 24) | (ref_bw_frame_id << 20)|
					(ref_fw_frame_id << 16) | (ref_blk_id << 8) | (ref_blk_size & 0xff);
			VSP_WRITE_REG(VSP_MCA_REG_BASE+MCA_BLK_CBUF_OFF, mc_blk_info, "configure MCA command buffer, block info");
			VSP_WRITE_REG(VSP_MCA_REG_BASE+MCA_MV_CBUF_OFF, mv_cmd, "configure MCA command buffer, block mv info");
		}
	}
	
	return;
}

PUBLIC void H264Dec_mv_prediction_P8x8_4x4 (DEC_MB_INFO_T *mb_info_ptr, int8 *ref_idx_ptr, int16 *ref_mv_ptr, int32 b8)
{
//	int32 b8_offset_y = (b8 & 2);
//	int32 b8_offset_x = (b8 << 1)&2;
	int32 mvd_x, mvd_y;
	int32 pred_mv_x, pred_mv_y, pred_mv;
	int32 mv_x, mv_y;
	int32 mv_xy, mv_cmd;
	int32 ref_blk_end = FALSE;
	int32 ref_bir_blk = FALSE;
	int32 ref_cmd_type = 0;
	int32 ref_bw_frame_id = 0;
	int32 ref_fw_frame_id = mb_info_ptr->ref_pic[b8];
	int32 ref_blk_id;
	int32 ref_blk_size = MC_BLKSIZE_4x4;
	int32 mc_blk_info;
	
	//4x4 0
	mvd_x = ref_mv_ptr[0];
	mvd_y = ref_mv_ptr[1];
	pred_mv = H264Dec_pred_motion (ref_idx_ptr, ref_mv_ptr, 1);
	pred_mv_x = pred_mv >> 16;
	pred_mv_y = (pred_mv << 16)>>16;

	mv_x = mvd_x + pred_mv_x;
	mv_y = mvd_y + pred_mv_y;

	mv_cmd = (mv_y<<16)|(mv_x&0xffff);
#if defined(H264_BIG_ENDIAN)
	mv_xy = (mv_x<<16)|(mv_y&0xffff);
#else
	mv_xy = (mv_y<<16)|(mv_x&0xffff);
#endif
	((int32 *)(ref_mv_ptr))[0] = mv_xy;

	//blk0 info
	ref_blk_end = FALSE;
	ref_bir_blk = FALSE;
	ref_cmd_type = 0;
	ref_bw_frame_id = 0;
	ref_blk_id = b8*4+0;
	ref_fw_frame_id = ref_idx_ptr[0];//mb_info_ptr->ref_pic[b8];
	ref_blk_size = MC_BLKSIZE_4x4;

	mc_blk_info = (ref_blk_end << 27) | (ref_bir_blk << 26) | (ref_cmd_type << 24) | (ref_bw_frame_id << 20)|
		(ref_fw_frame_id << 16) | (ref_blk_id << 8) | (ref_blk_size & 0xff);
	VSP_WRITE_REG(VSP_MCA_REG_BASE+MCA_BLK_CBUF_OFF, mc_blk_info, "configure MCA command buffer, block info");
	VSP_WRITE_REG(VSP_MCA_REG_BASE+MCA_MV_CBUF_OFF, mv_cmd, "configure MCA command buffer, block mv info");

//4x4 1
	mvd_x = ref_mv_ptr[2];
	mvd_y = ref_mv_ptr[3];
	pred_mv = H264Dec_pred_motion (ref_idx_ptr+1, ref_mv_ptr+2, 1);
	pred_mv_x = pred_mv >> 16;
	pred_mv_y = (pred_mv << 16)>>16;

	mv_x = mvd_x + pred_mv_x;
	mv_y = mvd_y + pred_mv_y;

	mv_cmd = (mv_y<<16)|(mv_x&0xffff);
#if defined(H264_BIG_ENDIAN)
	mv_xy = (mv_x<<16)|(mv_y&0xffff);
#else
	mv_xy = (mv_y<<16)|(mv_x&0xffff);
#endif
	((int32 *)(ref_mv_ptr))[1] = mv_xy;

	//blk1 info
	ref_blk_id = b8*4+1;
	ref_fw_frame_id = ref_idx_ptr[1];

	mc_blk_info = (ref_blk_end << 27) | (ref_bir_blk << 26) | (ref_cmd_type << 24) | (ref_bw_frame_id << 20)|
		(ref_fw_frame_id << 16) | (ref_blk_id << 8) | (ref_blk_size & 0xff);
	VSP_WRITE_REG(VSP_MCA_REG_BASE+MCA_BLK_CBUF_OFF, mc_blk_info, "configure MCA command buffer, block info");
	VSP_WRITE_REG(VSP_MCA_REG_BASE+MCA_MV_CBUF_OFF, mv_cmd, "configure MCA command buffer, block mv info");

//4x4 2
	mvd_x = ref_mv_ptr[2*CONTEXT_CACHE_WIDTH];
	mvd_y = ref_mv_ptr[2*CONTEXT_CACHE_WIDTH+1];
	pred_mv = H264Dec_pred_motion (ref_idx_ptr+CONTEXT_CACHE_WIDTH, ref_mv_ptr+CONTEXT_CACHE_WIDTH*2, 1);
	pred_mv_x = pred_mv >> 16;
	pred_mv_y = (pred_mv << 16)>>16;

	mv_x = mvd_x + pred_mv_x;
	mv_y = mvd_y + pred_mv_y;

	mv_cmd = (mv_y<<16)|(mv_x&0xffff);
#if defined(H264_BIG_ENDIAN)
	mv_xy = (mv_x<<16)|(mv_y&0xffff);
#else
	mv_xy = (mv_y<<16)|(mv_x&0xffff);
#endif
	((int32 *)(ref_mv_ptr))[CONTEXT_CACHE_WIDTH] = mv_xy;

	//blk0 info
	ref_blk_id = b8*4+2;
	ref_fw_frame_id = ref_idx_ptr[1*CONTEXT_CACHE_WIDTH];

	mc_blk_info = (ref_blk_end << 27) | (ref_bir_blk << 26) | (ref_cmd_type << 24) | (ref_bw_frame_id << 20)|
		(ref_fw_frame_id << 16) | (ref_blk_id << 8) | (ref_blk_size & 0xff);
	VSP_WRITE_REG(VSP_MCA_REG_BASE+MCA_BLK_CBUF_OFF, mc_blk_info, "configure MCA command buffer, block info");
	VSP_WRITE_REG(VSP_MCA_REG_BASE+MCA_MV_CBUF_OFF, mv_cmd, "configure MCA command buffer, block mv info");

//4x4 3
	mvd_x = ref_mv_ptr[2*(CONTEXT_CACHE_WIDTH+1)];
	mvd_y = ref_mv_ptr[2*(CONTEXT_CACHE_WIDTH+1)+1];
	pred_mv = H264Dec_pred_motion (ref_idx_ptr+CONTEXT_CACHE_WIDTH+1, ref_mv_ptr+(CONTEXT_CACHE_WIDTH+1)*2, 1);
	pred_mv_x = pred_mv >> 16;
	pred_mv_y = (pred_mv << 16)>>16;

	mv_x = mvd_x + pred_mv_x;
	mv_y = mvd_y + pred_mv_y;

	mv_cmd = (mv_y<<16)|(mv_x&0xffff);
#if defined(H264_BIG_ENDIAN)
	mv_xy = (mv_x<<16)|(mv_y&0xffff);
#else
	mv_xy = (mv_y<<16)|(mv_x&0xffff);
#endif
	((int32 *)(ref_mv_ptr))[CONTEXT_CACHE_WIDTH+1] = mv_xy;

	//blk0 info
	if (b8 == 3)
	{
		ref_blk_end = TRUE;
	}
	ref_blk_id = b8*4+3;
	ref_fw_frame_id = ref_idx_ptr[1*CONTEXT_CACHE_WIDTH+1];

	mc_blk_info = (ref_blk_end << 27) | (ref_bir_blk << 26) | (ref_cmd_type << 24) | (ref_bw_frame_id << 20)|
		(ref_fw_frame_id << 16) | (ref_blk_id << 8) | (ref_blk_size & 0xff);
	VSP_WRITE_REG(VSP_MCA_REG_BASE+MCA_BLK_CBUF_OFF, mc_blk_info, "configure MCA command buffer, block info");
	VSP_WRITE_REG(VSP_MCA_REG_BASE+MCA_MV_CBUF_OFF, mv_cmd, "configure MCA command buffer, block mv info");

	return;
}

LOCAL void H264Dec_mv_prediction_P8x8 (DEC_IMAGE_PARAMS_T *img_ptr, DEC_MB_INFO_T *mb_info_ptr, DEC_MB_CACHE_T *mb_cache_ptr)
{
	int16 *ref_mv_ptr = mb_info_ptr->mv_cache + (CONTEXT_CACHE_WIDTH+1)*2;
	int8  *ref_idx_ptr = mb_info_ptr->ref_idx_cache + (CONTEXT_CACHE_WIDTH+1);
	int32 b8mode;
	int32 ref_idx_b8_1, ref_idx_b8_3;

	ref_idx_b8_1 = ref_idx_ptr[2];
	ref_idx_ptr[2] = PART_NOT_AVAIL;

	ref_idx_b8_3 = ref_idx_ptr[2*CONTEXT_CACHE_WIDTH+2];
	ref_idx_ptr[2*CONTEXT_CACHE_WIDTH+2] = PART_NOT_AVAIL;

	//8x8 0
	b8mode = mb_cache_ptr->b8mode[0];
	(*mb_cache_ptr->b8_mv_pred_func[b8mode-4])(mb_info_ptr, ref_idx_ptr, ref_mv_ptr, 0);

	//8x8 1
	ref_idx_ptr[2] = ref_idx_b8_1;
	b8mode = mb_cache_ptr->b8mode[1];
	(*mb_cache_ptr->b8_mv_pred_func[b8mode-4])(mb_info_ptr, ref_idx_ptr+2, ref_mv_ptr+2*2, 1);

	//8x8 2
	b8mode = mb_cache_ptr->b8mode[2];
	(*mb_cache_ptr->b8_mv_pred_func[b8mode-4])(mb_info_ptr, ref_idx_ptr+2*CONTEXT_CACHE_WIDTH, 
		ref_mv_ptr+2*CONTEXT_CACHE_WIDTH*2, 2);

	//8x8 3
	b8mode = mb_cache_ptr->b8mode[3];
	ref_idx_ptr[2*CONTEXT_CACHE_WIDTH+2] = ref_idx_b8_3;
	(*mb_cache_ptr->b8_mv_pred_func[b8mode-4])(mb_info_ptr, ref_idx_ptr+2*CONTEXT_CACHE_WIDTH+2, 
		ref_mv_ptr+(2*CONTEXT_CACHE_WIDTH+2)*2, 3);

	return;
}

PUBLIC void H264Dec_mv_prediction (DEC_IMAGE_PARAMS_T *img_ptr, DEC_MB_INFO_T *mb_info_ptr, DEC_MB_CACHE_T *mb_cache_ptr)
{
	if (mb_info_ptr->is_skipped)
	{
		H264Dec_mv_prediction_skipped (img_ptr, mb_info_ptr, mb_cache_ptr);

	}else
	{
		switch(mb_info_ptr->mb_type)
		{
		case PMB16x16:
			H264Dec_mv_prediction_PMB16x16 (img_ptr, mb_info_ptr, mb_cache_ptr);
			break;
		case PMB16x8:
			H264Dec_mv_prediction_PMB16x8 (img_ptr, mb_info_ptr, mb_cache_ptr);
			break;
		case PMB8x16:
			H264Dec_mv_prediction_PMB8x16 (img_ptr, mb_info_ptr, mb_cache_ptr);
			break;
		case P8x8:
			H264Dec_mv_prediction_P8x8 (img_ptr, mb_info_ptr, mb_cache_ptr);
			break;
		default:
			break;
		}
	}

	VSP_WRITE_REG(VSP_MCA_REG_BASE+MCA_CFG_OFF, 1, "MCA_CFG: start MCA");

#if _CMODEL_
	mca_module();
#endif
	
	return;
}

#if defined(_VSP_)
	#if defined(H264_BIG_ENDIAN)
		#undef H264_BIG_ENDIAN
	#endif
#endif

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End 