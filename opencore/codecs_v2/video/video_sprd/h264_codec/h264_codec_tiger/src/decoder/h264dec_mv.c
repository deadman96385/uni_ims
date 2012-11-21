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
#include "tiger_video_header.h"
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

int32 s_list, s_dir;
int8 *s_ref_idx_ptr_array[2], *s_ref_idx_ptr;	
int32 *s_ref_mv_ptr_array[2], *s_ref_mv_ptr;
int32 *s_mvd_ptr;
int32 s_mvd_xy;
uint32 s_row;

//for MCA CONFIG
uint32 s_ref_blk_end;
uint32 s_ref_bir_blk;
uint32 s_ref_cmd_type;
uint32 s_ref_frame_id[2]; //0: fw, 1: bw
uint32 s_ref_blk_id;
uint32 s_ref_blk_size;
uint32 s_mc_blk_info;

#define WRITE_MC_BLK_INFO	\
{\
	uint32 mc_blk_info = (s_ref_blk_end << 27) | (s_ref_bir_blk << 26) | (s_ref_cmd_type << 24) | ((s_ref_frame_id[1]&0xf) << 20)|\
		((s_ref_frame_id[0]&0xf) << 16) | (s_ref_blk_id << 8) | (s_ref_blk_size & 0xff);\
	VSP_WRITE_REG_CQM(VSP_MCA_REG_BASE+MCA_BLK_CBUF_OFF, mc_blk_info, "configure MCA command buffer, block info");\
}

/**----------------------------------------------------------------------------*
**                           Function Prototype                               **
**----------------------------------------------------------------------------*/
PUBLIC int32 H264Dec_get_te (DEC_IMAGE_PARAMS_T *img_ptr, DEC_MB_CACHE_T *mb_cache_ptr, int32 blk_id, int32 list)
{
	int32 ref_frm_id;
	DEC_BS_T *stream = img_ptr->bitstrm_ptr;
	int32 num_ref_idx_active = img_ptr->ref_count[list];

	if (num_ref_idx_active == 2)
	{
		ref_frm_id = READ_BITS1(stream);
		ref_frm_id = 1 - ref_frm_id;
	}else
	{
		ref_frm_id = READ_UE_V(stream);
	}

	return ref_frm_id;
}

PUBLIC void readMVD_xy_cabac(DEC_IMAGE_PARAMS_T *img_ptr, DEC_MB_CACHE_T *mb_cache_ptr, int32 sub_blk_id)
{
	s_mvd_xy = readMVD_CABAC (img_ptr, mb_cache_ptr, sub_blk_id, s_list); 
}

PUBLIC void readMVD_xy_cavlc(DEC_IMAGE_PARAMS_T *img_ptr, DEC_MB_CACHE_T *mb_cache_ptr, int32 sub_blk_id)
{
	int32 mvd_x, mvd_y;
	DEC_BS_T *stream = img_ptr->bitstrm_ptr;

	mvd_x = READ_SE_V(stream); 
	mvd_y = READ_SE_V(stream); 

#if defined(H264_BIG_ENDIAN)
	s_mvd_xy  = (mvd_x << 16) | (mvd_y & 0xffff);
#else
	s_mvd_xy  = (mvd_y << 16) | (mvd_x & 0xffff);
#endif
}

#define IS_DIR(pdir)	(((pdir) == 2) || (s_list == (pdir)))

LOCAL void H264Dec_read_motionAndRefId_PMB16x16 (DEC_IMAGE_PARAMS_T *img_ptr, DEC_MB_INFO_T *mb_info_ptr, DEC_MB_CACHE_T *mb_cache_ptr)
{
	DEC_STORABLE_PICTURE_T	**listX;
	int32 ref_frm_id;
	int32 pdir = mb_cache_ptr->b8pdir[0];

	for (s_list = 0; s_list < img_ptr->list_count; s_list++)
	{
		if(IS_DIR(pdir))
		{
			listX = g_list[s_list];
			
			if (mb_cache_ptr->read_ref_id_flag[s_list])
			{
				ref_frm_id = readRefFrame (img_ptr, mb_cache_ptr, 0, s_list);
			}else
			{
				 ref_frm_id = 0;
			}

			if (ref_frm_id) // != 0
			{
				int32 tmp = ref_frm_id * 0x01010101;
				s_ref_idx_ptr = mb_cache_ptr->ref_idx_cache[s_list];
				((int32 *)s_ref_idx_ptr)[4] = ((int32 *)s_ref_idx_ptr)[7] = 
				((int32 *)s_ref_idx_ptr)[10] = ((int32 *)s_ref_idx_ptr)[13] = tmp;
			}
		}else
		{
			s_ref_idx_ptr = mb_cache_ptr->ref_idx_cache[s_list];
			((int32 *)s_ref_idx_ptr)[4] = ((int32 *)s_ref_idx_ptr)[7] = 
			((int32 *)s_ref_idx_ptr)[10] = ((int32 *)s_ref_idx_ptr)[13] = LIST_NOT_USED;
		}
	}

	for (s_list = 0; s_list < img_ptr->list_count; s_list++)
	{
		if(IS_DIR(pdir))
		{
			readMVD_xy(img_ptr, mb_cache_ptr, CTX_CACHE_WIDTH_PLUS4);

			if (s_mvd_xy)
			{
				s_mvd_ptr = (int32 *)(mb_cache_ptr->mvd_cache[s_list]);

				s_mvd_ptr[16] = s_mvd_ptr[17] = s_mvd_ptr[18] = s_mvd_ptr[19] =
				s_mvd_ptr[28] = s_mvd_ptr[29] = s_mvd_ptr[30] = s_mvd_ptr[31] =
				s_mvd_ptr[40] = s_mvd_ptr[41] = s_mvd_ptr[42] = s_mvd_ptr[43] =
				s_mvd_ptr[52] = s_mvd_ptr[53] = s_mvd_ptr[54] = s_mvd_ptr[55] = s_mvd_xy;
			}			
		}
	}

	return;
}

LOCAL void H264Dec_read_motionAndRefId_PMB16x8 (DEC_IMAGE_PARAMS_T *img_ptr, DEC_MB_INFO_T *mb_info_ptr, DEC_MB_CACHE_T *mb_cache_ptr)
{
	DEC_STORABLE_PICTURE_T	**listX;
	int32 ref_frm_id;
	uint32 b16x8;
	int32 pdir;
	
	for (s_list = 0; s_list < img_ptr->list_count; s_list++)
	{
		for (b16x8= 0; b16x8 < 2; b16x8++)
		{
			pdir = mb_cache_ptr->b8pdir[2*b16x8];
		
			if(IS_DIR(pdir))
			{
				listX = g_list[s_list];
				
				if (mb_cache_ptr->read_ref_id_flag[s_list])
				{
					ref_frm_id = readRefFrame (img_ptr, mb_cache_ptr, 2*b16x8, s_list);
				}else
				{
					 ref_frm_id = 0;
				}
				
				if (ref_frm_id) // != 0
				{
					int32 tmp = ref_frm_id * 0x01010101;
					s_ref_idx_ptr = mb_cache_ptr->ref_idx_cache[s_list] + CTX_CACHE_WIDTH_PLUS4+ CTX_CACHE_WIDTH_X2 *b16x8;
					((int32 *)s_ref_idx_ptr)[0] = tmp;	s_ref_idx_ptr += CTX_CACHE_WIDTH;
					((int32 *)s_ref_idx_ptr)[0] = tmp;			
				}
			}else
			{
				int32 tmp = LIST_NOT_USED;
				s_ref_idx_ptr = mb_cache_ptr->ref_idx_cache[s_list] + CTX_CACHE_WIDTH_PLUS4+ CTX_CACHE_WIDTH_X2 *b16x8;
				((int32 *)s_ref_idx_ptr)[0] = tmp;	s_ref_idx_ptr += CTX_CACHE_WIDTH;
				((int32 *)s_ref_idx_ptr)[0] = tmp;	
			}
		}
	}

	for (s_list = 0; s_list < img_ptr->list_count; s_list++)
	{
		for (b16x8 = 0; b16x8 < 2; b16x8++)
		{
			pdir = mb_cache_ptr->b8pdir[2*b16x8]; 
		
			if(IS_DIR(pdir))
			{
				readMVD_xy(img_ptr, mb_cache_ptr, CTX_CACHE_WIDTH_PLUS4+b16x8*CTX_CACHE_WIDTH_X2);

				if (s_mvd_xy)
				{
					s_mvd_ptr = (int32 *)(mb_cache_ptr->mvd_cache[s_list]) + CTX_CACHE_WIDTH_PLUS4+ CTX_CACHE_WIDTH_X2 *b16x8;
					s_mvd_ptr[0] = s_mvd_ptr[1] = s_mvd_ptr[2] = s_mvd_ptr[3] = s_mvd_xy;	s_mvd_ptr += CTX_CACHE_WIDTH;
					s_mvd_ptr[0] = s_mvd_ptr[1] = s_mvd_ptr[2] = s_mvd_ptr[3] = s_mvd_xy;
				}			
			}
		}
	}

	return;
}

LOCAL void H264Dec_read_motionAndRefId_PMB8x16 (DEC_IMAGE_PARAMS_T *img_ptr, DEC_MB_INFO_T *mb_info_ptr, DEC_MB_CACHE_T *mb_cache_ptr)
{
	DEC_STORABLE_PICTURE_T	**listX;
	int32 ref_frm_id;
	uint32 b8x16;
	int32 pdir;

	for (s_list = 0; s_list < img_ptr->list_count; s_list++)
	{
		for (b8x16 = 0; b8x16 < 2; b8x16++)
		{
			pdir = mb_cache_ptr->b8pdir[b8x16]; 
		
			if(IS_DIR(pdir))
			{
				listX = g_list[s_list];
				
				if (mb_cache_ptr->read_ref_id_flag[s_list])
				{
					ref_frm_id = readRefFrame (img_ptr, mb_cache_ptr, b8x16, s_list);
				}else
				{
					 ref_frm_id = 0;
				}

				if (ref_frm_id) // != 0
				{
					int32 tmp = ref_frm_id * 0x0101;
					s_ref_idx_ptr = mb_cache_ptr->ref_idx_cache[s_list] + CTX_CACHE_WIDTH_PLUS4+ 2*b8x16;
					((int16 *)s_ref_idx_ptr)[0] = tmp;	s_ref_idx_ptr += CTX_CACHE_WIDTH;
					((int16 *)s_ref_idx_ptr)[0] = tmp;	s_ref_idx_ptr += CTX_CACHE_WIDTH;
					((int16 *)s_ref_idx_ptr)[0] = tmp;	s_ref_idx_ptr += CTX_CACHE_WIDTH;
					((int16 *)s_ref_idx_ptr)[0] = tmp;			
				}
			}else
			{
				int32 tmp = LIST_NOT_USED; 
				s_ref_idx_ptr = mb_cache_ptr->ref_idx_cache[s_list] + CTX_CACHE_WIDTH_PLUS4+ 2*b8x16;
				((int16 *)s_ref_idx_ptr)[0] = tmp;	s_ref_idx_ptr += CTX_CACHE_WIDTH;
				((int16 *)s_ref_idx_ptr)[0] = tmp;	s_ref_idx_ptr += CTX_CACHE_WIDTH;
				((int16 *)s_ref_idx_ptr)[0] = tmp;	s_ref_idx_ptr += CTX_CACHE_WIDTH;
				((int16 *)s_ref_idx_ptr)[0] = tmp;	
			}
		}
	}

	for (s_list = 0; s_list < img_ptr->list_count; s_list++)
	{
		for (b8x16 = 0; b8x16 < 2; b8x16++)
		{
			pdir = mb_cache_ptr->b8pdir[b8x16]; 
		
			if(IS_DIR(pdir))
			{
				readMVD_xy(img_ptr, mb_cache_ptr, CTX_CACHE_WIDTH_PLUS4+b8x16*2);

				if (s_mvd_xy)
				{
					s_mvd_ptr = (int32 *)(mb_cache_ptr->mvd_cache[s_list]) + CTX_CACHE_WIDTH_PLUS4+ 2 *b8x16;
					s_mvd_ptr[0] = s_mvd_ptr[1] = s_mvd_xy; s_mvd_ptr += CTX_CACHE_WIDTH;
					s_mvd_ptr[0] = s_mvd_ptr[1] = s_mvd_xy;	s_mvd_ptr += CTX_CACHE_WIDTH;
					s_mvd_ptr[0] = s_mvd_ptr[1] = s_mvd_xy;	s_mvd_ptr += CTX_CACHE_WIDTH;
					s_mvd_ptr[0] = s_mvd_ptr[1] = s_mvd_xy;	
				}			
			}
		}
	}

	return;
}

LOCAL void H264Dec_read_motionAndRefId_P8x8 (DEC_IMAGE_PARAMS_T *img_ptr, DEC_MB_INFO_T *mb_info_ptr, DEC_MB_CACHE_T *mb_cache_ptr)
{
	DEC_STORABLE_PICTURE_T	**listX;
	int32 ref_frm_id;
	uint32 b8, cache_offset;
	int32 pdir;

	for (s_list = 0; s_list < img_ptr->list_count; s_list++)
	{
		for (b8 = 0; b8 < 4; b8++)
		{
			if (mb_cache_ptr->b8mode[b8] == 0)	continue;	//direct 8x8

			pdir = mb_cache_ptr->b8pdir[b8];
			if(IS_DIR(pdir))
			{
				listX = g_list[s_list];
				
				if (mb_cache_ptr->read_ref_id_flag[s_list])
				{
					ref_frm_id =  readRefFrame (img_ptr, mb_cache_ptr, b8, s_list);
				}else
				{
					 ref_frm_id = 0;
				}

				if (ref_frm_id) // != 0
				{
					int32 tmp = ref_frm_id * 0x0101;
					s_ref_idx_ptr = mb_cache_ptr->ref_idx_cache[s_list] + CTX_CACHE_WIDTH_PLUS4+ CTX_CACHE_WIDTH_X2*(b8>>1)+2*(b8&1);
					((int16 *)s_ref_idx_ptr)[0] = tmp;	s_ref_idx_ptr += CTX_CACHE_WIDTH;
					((int16 *)s_ref_idx_ptr)[0] = tmp;			
				}
			}else
			{
				int32 tmp = LIST_NOT_USED;
				s_ref_idx_ptr = mb_cache_ptr->ref_idx_cache[s_list] + CTX_CACHE_WIDTH_PLUS4+ CTX_CACHE_WIDTH_X2*(b8>>1)+2*(b8&1);
				((int16 *)s_ref_idx_ptr)[0] = tmp;	s_ref_idx_ptr += CTX_CACHE_WIDTH;
				((int16 *)s_ref_idx_ptr)[0] = tmp;	
			}
		}
	}

	for (s_list = 0; s_list < img_ptr->list_count; s_list++)
	{
		for (b8 = 0; b8 < 4; b8++)
		{
			if (mb_cache_ptr->b8mode[b8] == 0)	continue;
			
			pdir = mb_cache_ptr->b8pdir[b8];
			if(IS_DIR(pdir))
			{
				int32 blk_offset = CTX_CACHE_WIDTH_X2*(b8>>1)+2*(b8&1);
				int32 sub_blk_id = CTX_CACHE_WIDTH_PLUS4+blk_offset;///{7, 9, 19, 21};

				cache_offset = CTX_CACHE_WIDTH_PLUS4 + blk_offset;
				s_mvd_ptr = (int32 *)(mb_cache_ptr->mvd_cache[s_list]) + cache_offset;

				switch(mb_cache_ptr->b8mode[b8])
				{
				case PMB8X8_BLOCK8X8:
					readMVD_xy(img_ptr, mb_cache_ptr, sub_blk_id);
					s_mvd_ptr[0] = s_mvd_ptr[1] = s_mvd_xy;	s_mvd_ptr += CTX_CACHE_WIDTH;
					s_mvd_ptr[0] = s_mvd_ptr[1] = s_mvd_xy;	
					
					break;
				case PMB8X8_BLOCK8X4:
					readMVD_xy(img_ptr, mb_cache_ptr, sub_blk_id+0*CTX_CACHE_WIDTH);
					s_mvd_ptr[0] = s_mvd_ptr[1] = s_mvd_xy;	s_mvd_ptr += CTX_CACHE_WIDTH;

					readMVD_xy(img_ptr, mb_cache_ptr, sub_blk_id+1*CTX_CACHE_WIDTH);
					s_mvd_ptr[0] = s_mvd_ptr[1] = s_mvd_xy;	
					
					break;
				case PMB8X8_BLOCK4X8:
					readMVD_xy(img_ptr, mb_cache_ptr, sub_blk_id+0);
					s_mvd_ptr[0] = s_mvd_ptr[CTX_CACHE_WIDTH] = s_mvd_xy;	s_mvd_ptr ++;

					readMVD_xy(img_ptr, mb_cache_ptr, sub_blk_id+1);
					s_mvd_ptr[0] = s_mvd_ptr[CTX_CACHE_WIDTH] = s_mvd_xy;		
						
					break;
				case PMB8X8_BLOCK4X4:

					///0
					readMVD_xy(img_ptr, mb_cache_ptr, sub_blk_id);
					s_mvd_ptr[0] = s_mvd_xy;

					///1
					readMVD_xy(img_ptr, mb_cache_ptr, sub_blk_id+1);
					s_mvd_ptr[1] = s_mvd_xy;
					
					sub_blk_id += CTX_CACHE_WIDTH;
					s_mvd_ptr += CTX_CACHE_WIDTH;

					///2
					readMVD_xy(img_ptr, mb_cache_ptr, sub_blk_id);
					s_mvd_ptr[0] = s_mvd_xy;

					///3
					readMVD_xy(img_ptr, mb_cache_ptr, sub_blk_id+1);
					s_mvd_ptr[1] = s_mvd_xy;
					
					break;
				default:
					break;
				}
			}
		}
	}

	return;
}

PUBLIC void H264Dec_read_motionAndRefId (DEC_IMAGE_PARAMS_T *img_ptr, DEC_MB_INFO_T *mb_info_ptr, DEC_MB_CACHE_T *mb_cache_ptr)
{
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
	int32 blk_pos = (-CTX_CACHE_WIDTH + blk_width);
	ref_idx_c = ref_idx_ptr[blk_pos];

	if (ref_idx_c == PART_NOT_AVAIL)
	{
		ref_idx_c = ref_idx_ptr[-(CTX_CACHE_WIDTH+1)];
		mv_c_x = (ref_mv_ptr-(CTX_CACHE_WIDTH+1)*2)[0];
		mv_c_y = (ref_mv_ptr-(CTX_CACHE_WIDTH+1)*2)[1];
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
	ref_idx_b = ref_idx_ptr[-CTX_CACHE_WIDTH];
	mv_b_x = (ref_mv_ptr-CTX_CACHE_WIDTH*2)[0];
	mv_b_y = (ref_mv_ptr-CTX_CACHE_WIDTH*2)[1];

	match_cnt = ((ref_idx == ref_idx_a) ? 1 : 0) + ((ref_idx == ref_idx_b) ? 1 : 0) + ((ref_idx == ref_idx_c) ? 1 : 0);

	if (match_cnt == 0)
	{
		if (!((ref_idx_b == PART_NOT_AVAIL) && (ref_idx_c == PART_NOT_AVAIL) && (ref_idx_a != PART_NOT_AVAIL)))
		{
			int32 tmp;

			tmp = MEDIAN(mv_a_x, mv_b_x, mv_c_x);
			pred_mv = MEDIAN(mv_a_y, mv_b_y, mv_c_y);
			pred_mv = ((pred_mv<<16)|tmp&0xffff);
		}else
		{
			pred_mv = ((mv_a_y << 16) | (mv_a_x &0xffff));
		}
	}else if (match_cnt > 1)
	{
		int32 tmp;

		tmp = MEDIAN(mv_a_x, mv_b_x, mv_c_x);
		pred_mv = MEDIAN(mv_a_y, mv_b_y, mv_c_y);
		pred_mv = ((pred_mv<<16)|tmp&0xffff);
	}else
	{
		if (ref_idx == ref_idx_a)
		{
			pred_mv = ((mv_a_y << 16) | (mv_a_x &0xffff));
		}else if (ref_idx == ref_idx_b)
		{
			pred_mv = ((mv_b_y << 16) | (mv_b_x &0xffff));
		}else // (ref_idx == ref_idx_c)
		{
			pred_mv = ((mv_c_y << 16) | (mv_c_x &0xffff));
		}
	}
	return pred_mv;
}

LOCAL int32 H264Dec_pred16x8_motion (DEC_MB_CACHE_T *mb_cache_ptr, int32 blkIdx, int32 list)
{
	int32 blk_pos;
	int32 ref_id;
	int32 pred_mv;
	int8 *ref_idx_ptr;	
	int16 *ref_mv_ptr;

	ref_mv_ptr = mb_cache_ptr->mv_cache[list];
	ref_idx_ptr = mb_cache_ptr->ref_idx_cache[list];
	ref_id = ref_idx_ptr[blkIdx];

	if (blkIdx == CTX_CACHE_WIDTH_PLUS4) //the top 16x8 partition
	{
		blk_pos = blkIdx - CTX_CACHE_WIDTH;

		if (ref_id == ref_idx_ptr[blk_pos])
		{
			pred_mv = ((int32 *)ref_mv_ptr)[blk_pos];
			return pred_mv;
		}
	}else
	{
		blk_pos = blkIdx - 1;

		if (ref_id == ref_idx_ptr[blk_pos])
		{
			pred_mv = ((int32 *)ref_mv_ptr)[blk_pos];
			return pred_mv;
		}
	}

	pred_mv = H264Dec_pred_motion (ref_idx_ptr + blkIdx,  ref_mv_ptr + 2*blkIdx, 4);

	return pred_mv;
}

LOCAL int32 H264Dec_pred8x16_motion (DEC_MB_CACHE_T *mb_cache_ptr, int32 blkIdx, int32 list)
{
	int32 blk_pos;
	int32 ref_id;
	int32 pred_mv;
	int8 *ref_idx_ptr;	
	int16 *ref_mv_ptr;

	ref_mv_ptr = mb_cache_ptr->mv_cache[list];
	ref_idx_ptr = mb_cache_ptr->ref_idx_cache[list];
	ref_id = ref_idx_ptr[blkIdx];

	if (blkIdx == CTX_CACHE_WIDTH_PLUS4) //the left 8x16 partition
	{
		blk_pos = blkIdx - 1;

		if (ref_id == ref_idx_ptr[blk_pos])
		{
			pred_mv = ((int32 *)ref_mv_ptr)[blk_pos];
			return pred_mv;
		}
	}else
	{
		//diagonal block
		int32 ref_idx_c;

		blk_pos = blkIdx - CTX_CACHE_WIDTH + 2;
		ref_idx_c = ref_idx_ptr[blk_pos];

		if (ref_idx_c == PART_NOT_AVAIL)
		{
			blk_pos = blkIdx - (CTX_CACHE_WIDTH+1);
			ref_idx_c = ref_idx_ptr[blk_pos];
		}

		if (ref_id == ref_idx_c)
		{
			pred_mv = ((int32 *)ref_mv_ptr)[blk_pos];
			return pred_mv;
		}
	}

	pred_mv = H264Dec_pred_motion (ref_idx_ptr + blkIdx, ref_mv_ptr + 2*blkIdx, 2);

	return pred_mv;
}

//weighted prediction
int32 b8map[16] = 
{
	0, 0, 1, 1,
		0, 0, 1, 1, 
		2, 2, 3, 3,
		2, 2, 3, 3,
};

void H264Dec_Config_WP_info(DEC_IMAGE_PARAMS_T *img_ptr, DEC_MB_INFO_T *mb_info_ptr, DEC_MB_CACHE_T *mb_cache_ptr)
{
	uint32 bir_cmd = 0;
	uint32 wp_cmd[16];
	uint32 row, j;
	int32 blk4x4_id;
	uint32 w128_flag;
	int32 is_same_para = 0x7; //y, u, v 
	
	int32 b8;
  	int32 comp;

	VSP_WRITE_REG_CQM(VSP_MBC_REG_BASE+MBC_H264_WPBUF_SW_OFF, 0, "write 0: SW can access this buffer");
	VSP_WRITE_CMD_INFO((VSP_MBC << CQM_SHIFT_BIT) | (1<<24) |MBC_H264_WPBUF_SW_WOFF);

	for (comp = 0; comp < 3; comp++)
	{
		int8 *ref_idx_ptr0 = mb_cache_ptr->ref_idx_cache[0] + CTX_CACHE_WIDTH_PLUS4;
		int8 *ref_idx_ptr1 = mb_cache_ptr->ref_idx_cache[1] + CTX_CACHE_WIDTH_PLUS4;
		
		w128_flag = 0;
		blk4x4_id = 0;
		
		for (row = 0; row < 4; row++)
		{
			int32 mc_ref_idx_fw, mc_ref_idx_bw;
			
			for (j = 0; j < 4; j++)
			{
				int32 offset0, offset1 = 0;
				int32 weight0, weight1 = 0;
				
				b8 = b8map[blk4x4_id];
				mc_ref_idx_fw = ref_idx_ptr0[j]; 
				mc_ref_idx_bw = ref_idx_ptr1[j];
				
				if (mb_cache_ptr->mbc_b8pdir[b8])//bi-dir
				{
					offset0 = g_wp_offset[0][mc_ref_idx_fw][comp];
					offset1 = g_wp_offset[1][mc_ref_idx_bw][comp];
					weight0 = g_wbp_weight[0][mc_ref_idx_fw][mc_ref_idx_bw][comp];
					weight1 = g_wbp_weight[1][mc_ref_idx_fw][mc_ref_idx_bw][comp];

					if (weight0 == 128)
					{
						w128_flag |= (1<<blk4x4_id);
					}

					if (weight1 == 128)
					{
						w128_flag |= (1<<(blk4x4_id+16));
					}

				#if _DEBUG_
					if (weight0 == 128 || weight1 == 128)
					{
						foo2();
					}
				#endif
					wp_cmd[blk4x4_id] = ((offset1&0xff)<<24) | ((offset0&0xff)<<16) | ((weight1&0xff)<<8) | (weight0&0xff);//lsw for debug 8 bits is not enough for weight since it can be 128 
				}else //one-dir
				{
					if (mc_ref_idx_fw >= 0)//list 0
					{
						offset0 = g_wp_offset[0][mc_ref_idx_fw][comp];
						weight0 = g_wp_weight[0][mc_ref_idx_fw][comp];										
					}else //list 1
					{
						offset0 = g_wp_offset[1][mc_ref_idx_bw][comp];
						weight0 = g_wp_weight[1][mc_ref_idx_bw][comp];							
					}

					if (weight0 == 128)
					{
						w128_flag |= (1<<blk4x4_id);
					}
							
					wp_cmd[blk4x4_id] = ((offset1&0xff)<<24) | ((offset0&0xff)<<16) | ((weight1&0xff)<<8) | (weight0&0xff);
				}

				blk4x4_id++;
			}
			
			ref_idx_ptr0 += CTX_CACHE_WIDTH;
			ref_idx_ptr1 += CTX_CACHE_WIDTH;	
		}	
		
		for (blk4x4_id = 0; blk4x4_id < 15; blk4x4_id++)
		{
			if (wp_cmd[blk4x4_id] != wp_cmd[blk4x4_id+1])
			{
				is_same_para ^= (1<<comp);
				break;
			}
		}
		
		if (is_same_para & (1<<comp))
		{
			VSP_WRITE_REG_CQM(VSP_MBC_REG_BASE+MBC_HY_WP_BLK_OFF+0*4+comp*4*16, wp_cmd[0], "configure MBC wp info");
			VSP_WRITE_CMD_INFO( (VSP_MBC << CQM_SHIFT_BIT) | (2<<24) | ((MBC_HY_weight128_WOFF+comp)<<8) |(MBC_HY_WP_BLK0_WOFF + comp*16));
		}else
		{
			uint32 cmd_info_off = MBC_HY_WP_BLK0_WOFF+comp*16;
				
			for (blk4x4_id = 0; blk4x4_id < 16; blk4x4_id++)
			{
				VSP_WRITE_REG_CQM(VSP_MBC_REG_BASE+MBC_HY_WP_BLK_OFF+blk4x4_id*4+comp*4*16, wp_cmd[blk4x4_id], "configure MBC wp info");
			}
			
			VSP_WRITE_CMD_INFO( (VSP_MBC << CQM_SHIFT_BIT) | (17<<24) | ((cmd_info_off+2)<<16) | ((cmd_info_off+1)<<8) | cmd_info_off);
			VSP_WRITE_CMD_INFO( ((cmd_info_off+6)<<24) | ((cmd_info_off+5)<<16) | ((cmd_info_off+4)<<8) | (cmd_info_off+3));
			VSP_WRITE_CMD_INFO( ((cmd_info_off+10)<<24) | ((cmd_info_off+9)<<16) | ((cmd_info_off+8)<<8) | (cmd_info_off+7));
			VSP_WRITE_CMD_INFO( ((cmd_info_off+14)<<24) | ((cmd_info_off+13)<<16) | ((cmd_info_off+12)<<8) | (cmd_info_off+11));
			VSP_WRITE_CMD_INFO( ((MBC_HY_weight128_WOFF+comp)<<8) | (cmd_info_off+15));
		}
		
		VSP_WRITE_REG_CQM(VSP_MBC_REG_BASE+MBC_HY_weight128_OFF+comp*4, w128_flag, "configure MBC weight128 flag, Y");	
	}

	VSP_WRITE_REG_CQM(VSP_MBC_REG_BASE+MBC_H264_WPBUF_SW_OFF, 1, "write 1: HW can access this buffer");
	VSP_WRITE_CMD_INFO((VSP_MBC << CQM_SHIFT_BIT) | (1<<24) |MBC_H264_WPBUF_SW_WOFF);
	
	img_ptr->mbc_cfg_cmd &= 0xffff;
	img_ptr->mbc_cfg_cmd |= (is_same_para<<16);

}

uint32 s_b8_offset[4] = { 0, 2, CTX_CACHE_WIDTH_X2, CTX_CACHE_WIDTH_X2+2};

//for one 8x8 block in one MB
void H264Dec_Config_8x8MC_info(DEC_IMAGE_PARAMS_T *img_ptr, DEC_MB_CACHE_T *mb_cache_ptr, int32 b8, int32 b8pdir)
{
	uint32 cache_offset;

	s_ref_blk_size = MC_BLKSIZE_8x8;
	s_ref_blk_id = b8;
	s_ref_blk_end = (b8 == 3) ? 1 : 0; 
	
	cache_offset = CTX_CACHE_WIDTH_PLUS4 + s_b8_offset[b8];

	if (img_ptr->direct_spatial_mv_pred_flag == 0)
	{
		s_ref_idx_ptr = mb_cache_ptr->ref_idx_cache[0] + cache_offset;
		s_ref_frame_id[0] = s_ref_idx_ptr[0];

		s_ref_idx_ptr = mb_cache_ptr->ref_idx_cache[1] + cache_offset;
		s_ref_frame_id[1] = g_list1_map_list0[s_ref_idx_ptr[0]];
 	}
		
	WRITE_MC_BLK_INFO;
		
	if (b8pdir != 2)
	{
		s_list = b8pdir;
		s_ref_mv_ptr = (int32 *)(mb_cache_ptr->mv_cache[s_list]) + cache_offset;
		VSP_WRITE_REG_CQM(VSP_MCA_REG_BASE+MCA_MV_CBUF_OFF, s_ref_mv_ptr[0], "configure MCA command buffer, block mv info");
		VSP_WRITE_CMD_INFO((VSP_MCA << CQM_SHIFT_BIT) | (2<<24) | (MCA_MV_CBUF_WOFF<<8) | MCA_BLK_CBUF_WOFF);
	}else
	{
		s_ref_mv_ptr = (int32 *)(mb_cache_ptr->mv_cache[0]) + cache_offset;
		VSP_WRITE_REG_CQM(VSP_MCA_REG_BASE+MCA_MV_CBUF_OFF, s_ref_mv_ptr[0], "configure MCA command buffer, block mv info");

		s_ref_mv_ptr = (int32 *)(mb_cache_ptr->mv_cache[1]) + cache_offset;	
		VSP_WRITE_REG_CQM(VSP_MCA_REG_BASE+MCA_MV_CBUF_OFF, s_ref_mv_ptr[0], "configure MCA command buffer, block mv info");
		
		VSP_WRITE_CMD_INFO((VSP_MCA << CQM_SHIFT_BIT) | (3<<24) | (MCA_MV_CBUF_WOFF<<16) | (MCA_MV_CBUF_WOFF<<8) | MCA_BLK_CBUF_WOFF);
		
		mb_cache_ptr->mbc_b8pdir[b8] = 1;
	}

	return;
}

uint32 s_b4_offset[4] = {0, 1, CTX_CACHE_WIDTH, CTX_CACHE_WIDTH+1};

//for four 4x4 subblock in one 8x8 block
void H264Dec_Config_4x4MC_info(DEC_IMAGE_PARAMS_T *img_ptr, DEC_MB_CACHE_T *mb_cache_ptr, int32 b8, int32 b8pdir)
{
	uint32 b4;
	uint32 b8_cache_offset;
	uint32 b4_cache_offset;

	mb_cache_ptr->mbc_b8pdir[b8] = ((b8pdir == 2) ? 1 : 0);

	s_ref_blk_size = MC_BLKSIZE_4x4;
	s_ref_blk_end = 0;
	s_ref_blk_id = b8*4;

	b8_cache_offset = CTX_CACHE_WIDTH_PLUS4 + s_b8_offset[b8];
	for(b4 = 0; b4 < 4; b4++)
	{
		b4_cache_offset = b8_cache_offset + s_b4_offset[b4];
			
		if (s_ref_blk_id == 15)
		{
			s_ref_blk_end = 1;
		}

		if (img_ptr->direct_spatial_mv_pred_flag == 0)
		{
			s_ref_idx_ptr = mb_cache_ptr->ref_idx_cache[0] + b4_cache_offset;
			s_ref_frame_id[0] = s_ref_idx_ptr[0];

			s_ref_idx_ptr = mb_cache_ptr->ref_idx_cache[1] + b4_cache_offset;
			s_ref_frame_id[1] = g_list1_map_list0[s_ref_idx_ptr[0]];
 		}
			
		WRITE_MC_BLK_INFO;
			
		if (b8pdir != 2)
		{
			s_list = b8pdir;//lsw for debug mb_info_ptr->b8pdir[b8];
			s_ref_mv_ptr = (int32 *)(mb_cache_ptr->mv_cache[s_list]) + b4_cache_offset;
			VSP_WRITE_REG_CQM(VSP_MCA_REG_BASE+MCA_MV_CBUF_OFF, s_ref_mv_ptr[0], "configure MCA command buffer, block mv info");
			VSP_WRITE_CMD_INFO((VSP_MCA << CQM_SHIFT_BIT) | (2<<24) | (MCA_MV_CBUF_WOFF<<8) | MCA_BLK_CBUF_WOFF);
		}else
		{
			s_ref_mv_ptr = (int32 *)(mb_cache_ptr->mv_cache[0]) + b4_cache_offset;
			VSP_WRITE_REG_CQM(VSP_MCA_REG_BASE+MCA_MV_CBUF_OFF, s_ref_mv_ptr[0], "configure MCA command buffer, block mv info");
	
			s_ref_mv_ptr = (int32 *)(mb_cache_ptr->mv_cache[1]) + b4_cache_offset;
			VSP_WRITE_REG_CQM(VSP_MCA_REG_BASE+MCA_MV_CBUF_OFF, s_ref_mv_ptr[0], "configure MCA command buffer, block mv info");
			
			VSP_WRITE_CMD_INFO((VSP_MCA << CQM_SHIFT_BIT) | (3<<24) | (MCA_MV_CBUF_WOFF<<16) | (MCA_MV_CBUF_WOFF<<8) | MCA_BLK_CBUF_WOFF);				
		}

		s_ref_blk_id++; //NEXT sub-block
	}
}

//not reference
void H264Dec_FillNoRefList_8x8(DEC_MB_INFO_T *mb_info_ptr, uint32 b8, int32 b8pdir)
{
	if (g_image_ptr->type != B_SLICE) return;
		
	if (b8pdir != 2)
	{
		uint32 cache_offset = CTX_CACHE_WIDTH_PLUS4 + CTX_CACHE_WIDTH_X2*(b8>>1)+2*(b8&1);
		
		s_list = 1 - b8pdir;
		s_ref_idx_ptr = g_mb_cache_ptr->ref_idx_cache[s_list]+cache_offset;

		((int16 *)s_ref_idx_ptr)[0] = LIST_NOT_USED;	s_ref_idx_ptr += CTX_CACHE_WIDTH;
		((int16 *)s_ref_idx_ptr)[0] = LIST_NOT_USED;
	}
	return;	
}

PUBLIC int8 H264Dec_mv_prediction_skipped_bslice_spatial (DEC_IMAGE_PARAMS_T *img_ptr, DEC_MB_INFO_T *mb_info_ptr, DEC_MB_CACHE_T *mb_cache_ptr)
{
	int32 mv_xy;
	int32 mv_cmd[2];
	int32 use_8x8mc;
	int32 use_4x4mc[4] = {0,0,0,0};
	int32 b8;
	uint32 row;
	int32 b8pdir = 0;
	int32 fw_rFrame,bw_rFrame;
	
	s_ref_blk_end = TRUE;
	s_ref_cmd_type = 0;
	s_ref_frame_id[0] = 0;
	s_ref_frame_id[1] = 0;
	s_ref_blk_id = 0;
	s_ref_blk_size = MC_BLKSIZE_16x16;

	s_dir = 0;

	fw_rFrame = mb_cache_ptr->fw_rFrame;
	bw_rFrame = mb_cache_ptr->bw_rFrame;

	use_8x8mc = 0;
	if (fw_rFrame >= 0)
	{
		mv_cmd[s_dir] = mb_cache_ptr->fw_mv_xy;
		mv_xy  = mb_cache_ptr->fw_mv_xy;

		//set mv cache
		if (mv_xy)	//curr mv context has been set to value 0 in fill_mb func.
		{			
			uint32 col;
			int32 condition = (fw_rFrame || (g_list[1][0]->is_long_term)) ? 0 : 1;
			int8 *moving_block = mb_cache_ptr->moving_block;
			
			s_ref_mv_ptr = (int32 *)(mb_cache_ptr->mv_cache[0]) + CTX_CACHE_WIDTH_PLUS4;
			for (row = 0; row < 4; row++)
			{
				for (col = 0; col < 4; col++)
				{
					b8 = ((row>>1)<<1)+(col>>1);
					if  (condition  && (!moving_block[col]))
					{
			       		use_8x8mc = 1;
						use_4x4mc[b8] = 1;
				    }else
				    {
		      			s_ref_mv_ptr[col] = mv_xy;
				    }
				}

				moving_block += 4;
				s_ref_mv_ptr += CTX_CACHE_WIDTH;
			}
		}
		
		b8pdir = 0;
		s_dir++;
	}

	if (bw_rFrame >= 0)
	{
		mv_cmd[s_dir] = mb_cache_ptr->bw_mv_xy;
		mv_xy  = mb_cache_ptr->bw_mv_xy;

		//set mv cache
		if (mv_xy)	//curr mv context has been set to value 0 in fill_mb func.
		{			
			uint32 col;
			int32 condition = (bw_rFrame || (g_list[1][0]->is_long_term)) ? 0 : 1;
			int8 *moving_block = mb_cache_ptr->moving_block;
				
			s_ref_mv_ptr = (int32 *)(mb_cache_ptr->mv_cache[1])+ CTX_CACHE_WIDTH_PLUS4;
			for (row = 0; row < 4; row++)
			{
				for (col = 0; col < 4; col++)
				{
					b8 = ((row>>1)<<1)+(col>>1);
					if  (condition  && (!moving_block[col]))
					{
			       		use_8x8mc = 1;
						use_4x4mc[b8] = 1;
				    }else
				    {
		      			s_ref_mv_ptr[col] = mv_xy;
				    }
				}

				moving_block += 4;
				s_ref_mv_ptr += CTX_CACHE_WIDTH;
			}
		}

		b8pdir = 1;
		s_dir++;
	}else
	{
		// fw < 0 and bw < 0
		if (fw_rFrame < 0)
		{		
			s_dir = 2;
			fw_rFrame = bw_rFrame = 0;
			mv_cmd[0] = mv_cmd[1] = 0;
		}		
	}

	//save ref_idx to mb_cache.
	if (fw_rFrame >= 0)
	{
		uint32 ref_u32 = fw_rFrame * 0x01010101;
		
		s_ref_frame_id[0] = fw_rFrame;
		
		s_ref_idx_ptr = mb_cache_ptr->ref_idx_cache[0];
		((uint32 *)s_ref_idx_ptr)[4] =  
		((uint32 *)s_ref_idx_ptr)[7] =  
		((uint32 *)s_ref_idx_ptr)[10] =  
		((uint32 *)s_ref_idx_ptr)[13] = ref_u32; 
	}

	if (bw_rFrame >= 0)
	{
		uint32 ref_u32 = bw_rFrame * 0x01010101;
		
		s_ref_frame_id[1] = g_list1_map_list0[bw_rFrame];
		
		s_ref_idx_ptr = mb_cache_ptr->ref_idx_cache[1];
		((uint32 *)s_ref_idx_ptr)[4] =  
		((uint32 *)s_ref_idx_ptr)[7] =  
		((uint32 *)s_ref_idx_ptr)[10] =  
		((uint32 *)s_ref_idx_ptr)[13] = ref_u32; 		
	}
	s_ref_bir_blk = (s_dir == 2) ? 1 : 0;

	if (s_ref_bir_blk)
	{
		b8pdir = 2;
	}
	((int32 *)mb_cache_ptr->b8pdir)[0] = b8pdir * 0x01010101;

	//only one direction, and ref in list1
	if (b8pdir == 1)
	{
		s_ref_frame_id[0] = s_ref_frame_id[1];
	}
	
	//configure mv command
	if (!use_8x8mc)
	{
		WRITE_MC_BLK_INFO;
		VSP_WRITE_REG_CQM(VSP_MCA_REG_BASE+MCA_MV_CBUF_OFF, mv_cmd[0], "configure MCA command buffer, block mv info");
		if(s_ref_bir_blk)
		{
			VSP_WRITE_REG_CQM(VSP_MCA_REG_BASE+MCA_MV_CBUF_OFF, mv_cmd[1], "configure MCA command buffer, block mv info");
			VSP_WRITE_CMD_INFO((VSP_MCA << CQM_SHIFT_BIT) | (3<<24) | (MCA_MV_CBUF_WOFF<<16) | (MCA_MV_CBUF_WOFF<<8) | MCA_BLK_CBUF_WOFF);

			((int32 *)mb_cache_ptr->mbc_b8pdir)[0] = 0x01010101;
		}else
		{
			VSP_WRITE_CMD_INFO((VSP_MCA << CQM_SHIFT_BIT) | (2<<24) | (MCA_MV_CBUF_WOFF<<8) | MCA_BLK_CBUF_WOFF);
		}
	}else
	{
		for (b8 = 0; b8 < 4; b8++)
		{
			if (use_4x4mc[b8])
			{
				H264Dec_Config_4x4MC_info(img_ptr, mb_cache_ptr, b8, b8pdir);
			}else
			{		
				H264Dec_Config_8x8MC_info(img_ptr, mb_cache_ptr, b8, b8pdir);	
			}
		}
	}

	return b8pdir;
}

PUBLIC int8 H264Dec_mv_prediction_skipped_bslice_temporal (DEC_IMAGE_PARAMS_T *img_ptr, DEC_MB_INFO_T *mb_info_ptr, DEC_MB_CACHE_T *mb_cache_ptr)
{
	int32 mv_cmd[2];
	int32 use_8x8mc;
	int32 b8;
	int32 b8pdir;
	int32 offset;
	int8  *ref_idx_ptr0, *ref_idx_ptr1;
	int32 *ref_mv_ptr0, *ref_mv_ptr1;

	//save mv and ref_idx to mb_cache
	for (s_list = 0; s_list < 2; s_list++)
	{
		int32  *ref_idx_src_ptr = (int32 *)(mb_cache_ptr->ref_idx_cache_direct[s_list]);
		int32  *ref_idx_dst_ptr = (int32 *)(mb_cache_ptr->ref_idx_cache[s_list]);
		int32  *ref_mv_src_ptr = (int32 *)(mb_cache_ptr->mv_cache_direct[s_list]);
		int32  *ref_mv_dst_ptr = (int32 *)(mb_cache_ptr->mv_cache[s_list]) + CTX_CACHE_WIDTH_PLUS4;
		int32 row;

		ref_idx_dst_ptr[4] = ref_idx_src_ptr[0];
		ref_idx_dst_ptr[7] = ref_idx_src_ptr[1];
		ref_idx_dst_ptr[10] = ref_idx_src_ptr[2];
		ref_idx_dst_ptr[13] = ref_idx_src_ptr[3];

		for (row = 4 ; row > 0; row--)
		{			
			ref_mv_dst_ptr[0] =  ref_mv_src_ptr[0];
			ref_mv_dst_ptr[1] =  ref_mv_src_ptr[1];
			ref_mv_dst_ptr[2] =  ref_mv_src_ptr[2];
			ref_mv_dst_ptr[3] =  ref_mv_src_ptr[3];

			ref_mv_src_ptr += 4;
			ref_mv_dst_ptr += CTX_CACHE_WIDTH;
		}
	}

	s_ref_blk_end = TRUE;
	s_ref_bir_blk = 1;
	s_ref_cmd_type = 0;
	s_ref_frame_id[0] = 0;
	s_ref_frame_id[1] = 0;
	s_ref_blk_id = 0;
	s_ref_blk_size = MC_BLKSIZE_16x16;

	ref_idx_ptr0 = mb_cache_ptr->ref_idx_cache_direct[0];
	ref_idx_ptr1 = mb_cache_ptr->ref_idx_cache_direct[1];
	ref_mv_ptr0 = (int32*)(mb_cache_ptr->mv_cache_direct[0]);
	ref_mv_ptr1 = (int32*)(mb_cache_ptr->mv_cache_direct[1]);

	use_8x8mc = 0;
	for (b8=1 ; b8 < 4; b8++)
	{
		offset = ((b8>>1)<<3)+((b8&1)<<1);
		
		if(ref_idx_ptr0[0]!=ref_idx_ptr0[offset]
			|| ref_idx_ptr1[0]!=ref_idx_ptr1[offset]
			|| ref_mv_ptr0[0]!= ref_mv_ptr0[offset]
			|| ref_mv_ptr1[0]!= ref_mv_ptr1[offset])
		{
			use_8x8mc = 1;
			break;
		}
	}

	s_dir = 2;
	b8pdir = 2;
	((int32 *)mb_cache_ptr->b8pdir)[0] = b8pdir * 0x01010101;

	//configure mv command
	mv_cmd[0] = mv_cmd[1] = 0;
	if (!use_8x8mc)
	{
		s_ref_frame_id[0] = ref_idx_ptr0[0];
		mv_cmd[0] = ((int32 *)mb_cache_ptr->mv_cache_direct[0])[0];
		
		s_ref_frame_id[1] = g_list1_map_list0[ref_idx_ptr1[0]];
		mv_cmd[1] = ((int32 *)mb_cache_ptr->mv_cache_direct[1])[0];	

		WRITE_MC_BLK_INFO;
		VSP_WRITE_REG_CQM(VSP_MCA_REG_BASE+MCA_MV_CBUF_OFF, mv_cmd[0], "configure MCA command buffer, block mv info");
		VSP_WRITE_REG_CQM(VSP_MCA_REG_BASE+MCA_MV_CBUF_OFF, mv_cmd[1], "configure MCA command buffer, block mv info");
		VSP_WRITE_CMD_INFO((VSP_MCA << CQM_SHIFT_BIT) | (3<<24) | (MCA_MV_CBUF_WOFF<<16) | (MCA_MV_CBUF_WOFF<<8) | MCA_BLK_CBUF_WOFF);
		
		((int32 *)mb_cache_ptr->mbc_b8pdir)[0] = 0x01010101;
	}else
	{
		for (b8 = 0; b8 < 4; b8++)
		{
			int32 b4,offset_b4;
			int32 use_4x4mc = 0;

			if (g_active_sps_ptr->direct_8x8_inference_flag == 0)
			{
				offset = ((b8>>1)<<2)+((b8&1)<<1);
				for (b4=1 ; b4 < 4; b4++)
				{
					offset_b4 = ((b4>>1)<<2)+(b4&1);

					if(ref_idx_ptr0[offset+offset_b4]!=ref_idx_ptr0[offset]
						|| ref_idx_ptr1[offset+offset_b4]!=ref_idx_ptr1[offset]
						|| ref_mv_ptr0[offset+offset_b4]!= ref_mv_ptr0[offset]
						|| ref_mv_ptr1[offset+offset_b4]!= ref_mv_ptr1[offset])
					{
						use_4x4mc = 1;
						break;
					}
				}			
			}

			if (use_4x4mc)
			{
				H264Dec_Config_4x4MC_info(img_ptr, mb_cache_ptr, b8, b8pdir);
			}else
			{		
				H264Dec_Config_8x8MC_info(img_ptr, mb_cache_ptr, b8, b8pdir);	
			}
		}
	}

	return b8pdir;
}

//pred_mv = mv_y<<16 | mv_x
LOCAL void H264Dec_mv_prediction_skipped_pslice (DEC_IMAGE_PARAMS_T *img_ptr, DEC_MB_INFO_T *mb_info_ptr, DEC_MB_CACHE_T *mb_cache_ptr)
{
	int32 pred_mv;
	int32 mv_xy;
	int32 mv_cmd;
	int32 top_ref_idx, left_ref_idx;
	
	s_ref_blk_end = TRUE;
	s_ref_bir_blk = 0;
	s_ref_cmd_type = 0;
	s_ref_frame_id[0] = 0;
	s_ref_frame_id[1] = 0;
	s_ref_blk_id = 0;
	s_ref_blk_size = MC_BLKSIZE_16x16;

	s_ref_idx_ptr = mb_cache_ptr->ref_idx_cache[0] + CTX_CACHE_WIDTH_PLUS4;
	s_ref_mv_ptr = (int32 *)(mb_cache_ptr->mv_cache[0]) + CTX_CACHE_WIDTH_PLUS4;
	top_ref_idx = s_ref_idx_ptr[-CTX_CACHE_WIDTH];
	left_ref_idx = s_ref_idx_ptr[-1];
	s_ref_frame_id[0] = s_ref_idx_ptr[0];

	if ((top_ref_idx == PART_NOT_AVAIL) || (left_ref_idx == PART_NOT_AVAIL) ||
		((top_ref_idx == 0) && (s_ref_mv_ptr[-CTX_CACHE_WIDTH] == 0)) ||
		((left_ref_idx == 0) && (s_ref_mv_ptr[-1] == 0)))
	{
		mv_cmd = 0;
		mv_xy = 0;
	}else
	{
		pred_mv = H264Dec_pred_motion(s_ref_idx_ptr, s_ref_mv_ptr, 4);
			
		mv_cmd = pred_mv;
	#if defined(H264_BIG_ENDIAN)
		mv_x = ((pred_mv << 16) >> 16);
		mv_y = (pred_mv >> 16);
		mv_xy  = (mv_x << 16) | (mv_y & 0xffff);
	#else
		mv_xy  = pred_mv;
	#endif
	}

	//set mv cache
	if (mv_xy)	//curr mv context has been set to value 0 in fill_mb func.
	{
		s_ref_mv_ptr[0] = s_ref_mv_ptr[1] = s_ref_mv_ptr[2] = s_ref_mv_ptr[3] = mv_xy; s_ref_mv_ptr += CTX_CACHE_WIDTH;
		s_ref_mv_ptr[0] = s_ref_mv_ptr[1] = s_ref_mv_ptr[2] = s_ref_mv_ptr[3] = mv_xy; s_ref_mv_ptr += CTX_CACHE_WIDTH;
		s_ref_mv_ptr[0] = s_ref_mv_ptr[1] = s_ref_mv_ptr[2] = s_ref_mv_ptr[3] = mv_xy; s_ref_mv_ptr += CTX_CACHE_WIDTH;
		s_ref_mv_ptr[0] = s_ref_mv_ptr[1] = s_ref_mv_ptr[2] = s_ref_mv_ptr[3] = mv_xy;  
	}
	
	//configure mv command
	WRITE_MC_BLK_INFO;
	VSP_WRITE_REG_CQM(VSP_MCA_REG_BASE+MCA_MV_CBUF_OFF, mv_cmd, "configure MCA command buffer, block mv info");
	VSP_WRITE_CMD_INFO((VSP_MCA << CQM_SHIFT_BIT) | (2<<24) | (MCA_MV_CBUF_WOFF<<8) | MCA_BLK_CBUF_WOFF);

	return;
}

LOCAL void H264Dec_mv_prediction_PMB16x16 (DEC_IMAGE_PARAMS_T *img_ptr, DEC_MB_INFO_T *mb_info_ptr, DEC_MB_CACHE_T *mb_cache_ptr)
{
	int32 pred_mv_x, pred_mv_y, pred_mv_xy;
	int32 mvd_x, mvd_y;
	int32 mv_x, mv_y;
	int32 mv_xy;
	int32 mv_cmd[2];
	uint32 row;
	uint32 cache_offset;
	int32 pdir = mb_cache_ptr->b8pdir[0];

	s_ref_blk_end = TRUE;
	s_ref_bir_blk = (mb_cache_ptr->b8pdir[0] == 2) ? 1 : 0;
	s_ref_cmd_type = 0;
	s_ref_frame_id[0] = 0;
	s_ref_frame_id[1] = 0;
	s_ref_blk_id = 0;
	s_ref_blk_size = MC_BLKSIZE_16x16;
	s_dir = 0;
	do
	{
		s_list = (pdir == 2) ? s_dir : pdir;
		cache_offset = CTX_CACHE_WIDTH_PLUS4;
		s_ref_idx_ptr = mb_cache_ptr->ref_idx_cache[s_list] + cache_offset;
		s_ref_mv_ptr = (int32 *)(mb_cache_ptr->mv_cache[s_list]) + cache_offset;
		s_mvd_ptr = (int32 *)(mb_cache_ptr->mvd_cache[s_list]) + cache_offset;

		s_ref_frame_id[s_dir] = ((s_list == 0) ?  s_ref_idx_ptr[0] : g_list1_map_list0[s_ref_idx_ptr[0]]);

		pred_mv_xy = H264Dec_pred_motion (s_ref_idx_ptr, (int16 *)s_ref_mv_ptr, 4);
		pred_mv_x = ((pred_mv_xy << 16) >> 16);
		pred_mv_y = (pred_mv_xy >> 16);

		s_mvd_xy = s_mvd_ptr[0];
		mvd_x = ((s_mvd_xy << 16) >> 16);
		mvd_y = (s_mvd_xy >> 16);

		mv_x = mvd_x + pred_mv_x;
		mv_y = mvd_y + pred_mv_y;

		mv_cmd[s_dir] = (mv_y << 16) | (mv_x & 0xffff);
	#if defined(H264_BIG_ENDIAN)
		mv_xy  = (mv_x << 16) | (mv_y & 0xffff);
	#else
		mv_xy  = mv_cmd[s_dir];
	#endif

		//set mv cache
		for (row = 4; row > 0; row--)
		{
			s_ref_mv_ptr[0] = s_ref_mv_ptr[1] =
			s_ref_mv_ptr[2] = s_ref_mv_ptr[3] = mv_xy;

			s_ref_mv_ptr += CTX_CACHE_WIDTH;
		}

		s_dir++;
	}while (s_dir < pdir);
	//configure mv command
	WRITE_MC_BLK_INFO;
	VSP_WRITE_REG_CQM(VSP_MCA_REG_BASE+MCA_MV_CBUF_OFF, mv_cmd[0], "configure MCA command buffer, block mv info");
	if (s_ref_bir_blk) ///*bi-dir*/
	{
		VSP_WRITE_REG_CQM(VSP_MCA_REG_BASE+MCA_MV_CBUF_OFF, mv_cmd[1], "configure MCA command buffer, block mv info");	
	    VSP_WRITE_CMD_INFO((VSP_MCA << CQM_SHIFT_BIT) | (3<<24) | (MCA_MV_CBUF_WOFF<<16) | (MCA_MV_CBUF_WOFF<<8) | MCA_BLK_CBUF_WOFF);

		((int32 *)mb_cache_ptr->mbc_b8pdir)[0] = 0x01010101;
	}else
	{
    	VSP_WRITE_CMD_INFO((VSP_MCA << CQM_SHIFT_BIT) | (2<<24) | (MCA_MV_CBUF_WOFF<<8) | MCA_BLK_CBUF_WOFF);
	}
	
	return;	
}

LOCAL void H264Dec_mv_prediction_PMB16x8 (DEC_IMAGE_PARAMS_T *img_ptr, DEC_MB_INFO_T *mb_info_ptr, DEC_MB_CACHE_T *mb_cache_ptr)
{
	int32 pred_mv_x, pred_mv_y, pred_mv_xy;
	int32 mvd_x, mvd_y;
	int32 mv_x, mv_y;
	int32 mv_xy;
	int32 mv_cmd[2];
	uint32 row, b16x8;
	uint32 cache_offset;
	int32 pdir;

	s_ref_blk_end = FALSE;
	s_ref_bir_blk = FALSE;
	s_ref_cmd_type = 0;
	s_ref_frame_id[0] = 0;
	s_ref_frame_id[1] = 0;
	s_ref_blk_id = 0;
	s_ref_blk_size = MC_BLKSIZE_8x8;

	for (b16x8 = 0; b16x8 < 2; b16x8++)
	{
		pdir = mb_cache_ptr->b8pdir[2*b16x8];
		s_ref_bir_blk = (pdir == 2) ? 1 : 0;

		s_dir = 0;
		do 
		{
			s_list = (pdir == 2) ? s_dir : pdir;
			cache_offset = CTX_CACHE_WIDTH_PLUS4 + CTX_CACHE_WIDTH_X2 *b16x8;
			s_ref_idx_ptr = mb_cache_ptr->ref_idx_cache[s_list] + cache_offset;
			s_ref_mv_ptr = (int32 *)(mb_cache_ptr->mv_cache[s_list]) + cache_offset;
			s_mvd_ptr = (int32 *)(mb_cache_ptr->mvd_cache[s_list]) + cache_offset;

			s_ref_frame_id[s_dir] = ((s_list == 0) ?  s_ref_idx_ptr[0] : g_list1_map_list0[s_ref_idx_ptr[0]]);

			pred_mv_xy = H264Dec_pred16x8_motion (mb_cache_ptr, cache_offset, s_list);
			pred_mv_x = ((pred_mv_xy << 16) >> 16);
			pred_mv_y = (pred_mv_xy >> 16);

			s_mvd_xy = s_mvd_ptr[0];
			mvd_x = ((s_mvd_xy << 16) >> 16);
			mvd_y = (s_mvd_xy >> 16);

			mv_x = mvd_x + pred_mv_x;
			mv_y = mvd_y + pred_mv_y;

			mv_cmd[s_dir] = (mv_y << 16) | (mv_x & 0xffff);
		#if defined(H264_BIG_ENDIAN)
			mv_xy  = (mv_x << 16) | (mv_y & 0xffff);
		#else
			mv_xy  = mv_cmd[s_dir];
		#endif

			//set mv cache
			for (row = 2; row > 0; row--)
			{
				s_ref_mv_ptr[0] = s_ref_mv_ptr[1] = 
				s_ref_mv_ptr[2] = s_ref_mv_ptr[3] = mv_xy;

				s_ref_mv_ptr += CTX_CACHE_WIDTH;
			}

			s_dir++;
		}while(s_dir < pdir);
	
		//configure mv command
		s_ref_blk_id = 2*b16x8;

		WRITE_MC_BLK_INFO;
		VSP_WRITE_REG_CQM(VSP_MCA_REG_BASE+MCA_MV_CBUF_OFF, mv_cmd[0], "configure MCA command buffer, block mv info");
		if (s_ref_bir_blk)
		{
			VSP_WRITE_REG_CQM(VSP_MCA_REG_BASE+MCA_MV_CBUF_OFF, mv_cmd[1], "configure MCA command buffer, block mv info");
			mb_cache_ptr->mbc_b8pdir[s_ref_blk_id] = 1;

			VSP_WRITE_CMD_INFO((VSP_MCA << CQM_SHIFT_BIT) | (3<<24) | (MCA_MV_CBUF_WOFF<<16) | (MCA_MV_CBUF_WOFF<<8) | MCA_BLK_CBUF_WOFF);
		}else
		{
			VSP_WRITE_CMD_INFO((VSP_MCA << CQM_SHIFT_BIT) | (2<<24) | (MCA_MV_CBUF_WOFF<<8) | MCA_BLK_CBUF_WOFF);
		}
		
		if (b16x8 == 1)
		{
			s_ref_blk_end = TRUE;
		}

		s_ref_blk_id += 1;	
		WRITE_MC_BLK_INFO;
		VSP_WRITE_REG_CQM(VSP_MCA_REG_BASE+MCA_MV_CBUF_OFF, mv_cmd[0], "configure MCA command buffer, block mv info");
		if (s_ref_bir_blk)
		{
			VSP_WRITE_REG_CQM(VSP_MCA_REG_BASE+MCA_MV_CBUF_OFF, mv_cmd[1], "configure MCA command buffer, block mv info");
			VSP_WRITE_CMD_INFO((VSP_MCA << CQM_SHIFT_BIT) | (3<<24) | (MCA_MV_CBUF_WOFF<<16) | (MCA_MV_CBUF_WOFF<<8) | MCA_BLK_CBUF_WOFF);

			mb_cache_ptr->mbc_b8pdir[s_ref_blk_id] = 1;
		}else
		{
			VSP_WRITE_CMD_INFO((VSP_MCA << CQM_SHIFT_BIT) | (2<<24) | (MCA_MV_CBUF_WOFF<<8) | MCA_BLK_CBUF_WOFF);
		}
	}

	return;	
}

LOCAL void H264Dec_mv_prediction_PMB8x16 (DEC_IMAGE_PARAMS_T *img_ptr, DEC_MB_INFO_T *mb_info_ptr, DEC_MB_CACHE_T *mb_cache_ptr)
{
	int32 pred_mv_x, pred_mv_y, pred_mv_xy;
	int32 mvd_x, mvd_y;
	int32 mv_x, mv_y;
	int32 mv_xy;
	int32 mv_cmd[2];
	uint32 row, b8x16;
	uint32 cache_offset;
	int32 pdir = mb_cache_ptr->b8pdir[0];

	s_ref_blk_end = FALSE;
	s_ref_cmd_type = 0;
	s_ref_frame_id[0] = 0;
	s_ref_frame_id[1] = 0;
	s_ref_blk_id = 0;
	s_ref_blk_size = MC_BLKSIZE_8x8;

	for (b8x16 = 0; b8x16 < 2; b8x16++)
	{
		pdir = mb_cache_ptr->b8pdir[b8x16];
		s_ref_bir_blk = (pdir == 2) ? 1 : 0;

		s_dir = 0;
		do
		{
			s_list = (pdir == 2) ? s_dir : pdir;
			cache_offset = CTX_CACHE_WIDTH_PLUS4 + 2*b8x16;
			s_ref_idx_ptr = mb_cache_ptr->ref_idx_cache[s_list] + cache_offset;
			s_ref_mv_ptr = (int32 *)(mb_cache_ptr->mv_cache[s_list]) + cache_offset;
			s_mvd_ptr = (int32 *)(mb_cache_ptr->mvd_cache[s_list]) + cache_offset;

			s_ref_frame_id[s_dir] = ((s_list == 0) ?  s_ref_idx_ptr[0] : g_list1_map_list0[s_ref_idx_ptr[0]]);

			pred_mv_xy = H264Dec_pred8x16_motion (mb_cache_ptr, cache_offset, s_list);
			pred_mv_x = ((pred_mv_xy << 16) >> 16);
			pred_mv_y = (pred_mv_xy >> 16);

			s_mvd_xy = s_mvd_ptr[0];
			mvd_x = ((s_mvd_xy << 16) >> 16);
			mvd_y = (s_mvd_xy >> 16);

			mv_x = mvd_x + pred_mv_x;
			mv_y = mvd_y + pred_mv_y;

			mv_cmd[s_dir] = (mv_y << 16) | (mv_x & 0xffff);
		#if defined(H264_BIG_ENDIAN)
			mv_xy  = (mv_x << 16) | (mv_y & 0xffff);
		#else
			mv_xy  = mv_cmd[s_dir];
		#endif

			//set mv cache
			s_ref_mv_ptr = (int32 *)(mb_cache_ptr->mv_cache[s_list]) + cache_offset;
			for (row = 0; row < 4; row++)
			{
				s_ref_mv_ptr[0] = s_ref_mv_ptr[1] = mv_xy;
				s_ref_mv_ptr += CTX_CACHE_WIDTH;
			}

			s_dir++;
		}while(s_dir < pdir);

		//configure mv command
		s_ref_blk_id = b8x16;

		WRITE_MC_BLK_INFO;
		VSP_WRITE_REG_CQM(VSP_MCA_REG_BASE+MCA_MV_CBUF_OFF, mv_cmd[0], "configure MCA command buffer, block mv info");
		if (s_ref_bir_blk)
		{
			VSP_WRITE_REG_CQM(VSP_MCA_REG_BASE+MCA_MV_CBUF_OFF, mv_cmd[1], "configure MCA command buffer, block mv info");
			VSP_WRITE_CMD_INFO((VSP_MCA << CQM_SHIFT_BIT) | (3<<24) | (MCA_MV_CBUF_WOFF<<16) | (MCA_MV_CBUF_WOFF<<8) | MCA_BLK_CBUF_WOFF);

			mb_cache_ptr->mbc_b8pdir[s_ref_blk_id] = 1;
		}else
		{
			VSP_WRITE_CMD_INFO((VSP_MCA << CQM_SHIFT_BIT) | (2<<24) | (MCA_MV_CBUF_WOFF<<8) | MCA_BLK_CBUF_WOFF);
		}
		
		if (b8x16 == 1)
		{
			s_ref_blk_end = TRUE;
		}

		s_ref_blk_id += 2;	
		WRITE_MC_BLK_INFO;
		VSP_WRITE_REG_CQM(VSP_MCA_REG_BASE+MCA_MV_CBUF_OFF, mv_cmd[0], "configure MCA command buffer, block mv info");
		if (s_ref_bir_blk)
		{
			VSP_WRITE_REG_CQM(VSP_MCA_REG_BASE+MCA_MV_CBUF_OFF, mv_cmd[1], "configure MCA command buffer, block mv info");
			VSP_WRITE_CMD_INFO((VSP_MCA << CQM_SHIFT_BIT) | (3<<24) | (MCA_MV_CBUF_WOFF<<16) | (MCA_MV_CBUF_WOFF<<8) | MCA_BLK_CBUF_WOFF);

			mb_cache_ptr->mbc_b8pdir[s_ref_blk_id] = 1;
		}else
		{
			VSP_WRITE_CMD_INFO((VSP_MCA << CQM_SHIFT_BIT) | (2<<24) | (MCA_MV_CBUF_WOFF<<8) | MCA_BLK_CBUF_WOFF);
		}
	}

	return;	
}
PUBLIC int32 H264Dec_mv_prediction_P8x8_8x8 (DEC_MB_INFO_T *mb_info_ptr, DEC_MB_CACHE_T *mb_cache_ptr, int32 cache_offset, int32 b8)
{
	int32 pred_mv_x, pred_mv_y, pred_mv_xy;
	int32 mvd_x, mvd_y;
	int32 mv_x, mv_y;
	int32 mv_xy;
	int32 mv_cmd[2];
	int32 pdir = mb_cache_ptr->b8pdir[b8];

	s_ref_blk_end = FALSE;
	s_ref_cmd_type = 0;
	s_ref_frame_id[0] = 0;
	s_ref_frame_id[1] = 0;
	s_ref_blk_id = b8;
	s_ref_blk_size = MC_BLKSIZE_8x8;

	s_dir = 0;
	do
	{
		s_ref_bir_blk = (pdir == 2) ? 1 : 0;

		s_list = (pdir == 2) ? s_dir : pdir;
		s_ref_idx_ptr = mb_cache_ptr->ref_idx_cache[s_list]+cache_offset;
		s_ref_mv_ptr = (int32*)(mb_cache_ptr->mv_cache[s_list]) + cache_offset;
		s_mvd_ptr =(int32*)(mb_cache_ptr->mvd_cache[s_list]) + cache_offset;

		s_ref_frame_id[s_dir] = ((s_list == 0) ?  s_ref_idx_ptr[0] : g_list1_map_list0[s_ref_idx_ptr[0]]);

		pred_mv_xy = H264Dec_pred_motion (s_ref_idx_ptr, (int16 *)s_ref_mv_ptr, 2);
		pred_mv_x = ((pred_mv_xy << 16) >> 16);
		pred_mv_y = (pred_mv_xy >> 16);

		s_mvd_xy = s_mvd_ptr[0];
		mvd_x = ((s_mvd_xy << 16) >> 16);
		mvd_y = (s_mvd_xy >> 16);

		mv_x = mvd_x + pred_mv_x;
		mv_y = mvd_y + pred_mv_y;

		mv_cmd[s_dir] = (mv_y << 16) | (mv_x & 0xffff);
	#if defined(H264_BIG_ENDIAN)
		mv_xy  = (mv_x << 16) | (mv_y & 0xffff);
	#else
		mv_xy  = mv_cmd[s_dir];
	#endif

		//set mv cache
		s_ref_mv_ptr[0] = s_ref_mv_ptr[1] = mv_xy; s_ref_mv_ptr += CTX_CACHE_WIDTH;
		s_ref_mv_ptr[0] = s_ref_mv_ptr[1] = mv_xy; 

		s_dir++;
	}while(s_dir < pdir);

	//configure mv command
	if (b8 == 3)
	{
		s_ref_blk_end = TRUE;
	}
	
	s_ref_bir_blk = (pdir == 2) ? 1 : 0;

	WRITE_MC_BLK_INFO;
	VSP_WRITE_REG_CQM(VSP_MCA_REG_BASE+MCA_MV_CBUF_OFF, mv_cmd[0], "configure MCA command buffer, block mv info");
	if (s_ref_bir_blk)
	{
		VSP_WRITE_REG_CQM(VSP_MCA_REG_BASE+MCA_MV_CBUF_OFF, mv_cmd[1], "configure MCA command buffer, block mv info");
		VSP_WRITE_CMD_INFO((VSP_MCA << CQM_SHIFT_BIT) | (3<<24) | (MCA_MV_CBUF_WOFF<<16) | (MCA_MV_CBUF_WOFF<<8) | MCA_BLK_CBUF_WOFF);

		mb_cache_ptr->mbc_b8pdir[s_ref_blk_id] = 1;
	}else
	{
		VSP_WRITE_CMD_INFO((VSP_MCA << CQM_SHIFT_BIT) | (2<<24) |(MCA_MV_CBUF_WOFF<<8) |MCA_BLK_CBUF_WOFF);
	}
	return pdir;
}

PUBLIC int32 H264Dec_mv_prediction_P8x8_8x4 (DEC_MB_INFO_T *mb_info_ptr, DEC_MB_CACHE_T *mb_cache_ptr, int32 cache_offset, int32 b8)
{
	int32 pred_mv_x, pred_mv_y, pred_mv_xy;
	int32 mvd_x, mvd_y;
	int32 mv_x, mv_y;
	int32 mv_xy;
	int32 mv_cmd[2];
	uint32 b8x4;
	uint32 offset;
	int32 pdir = mb_cache_ptr->b8pdir[b8];

	s_ref_blk_end = FALSE;
	s_ref_bir_blk = (pdir == 2) ? 1 : 0;
	s_ref_cmd_type = 0;
	s_ref_frame_id[0] = 0;
	s_ref_frame_id[1] = 0;
	s_ref_blk_id = 0;
	s_ref_blk_size = MC_BLKSIZE_4x4;

	if (pdir == 2)
	{
		mb_cache_ptr->mbc_b8pdir[b8] = 1;
	}

	for (b8x4 = 0; b8x4 < 2; b8x4++)
	{	
		offset = cache_offset + b8x4 * CTX_CACHE_WIDTH;

		s_dir = 0;
		do
		{
			s_list = (pdir == 2) ? s_dir : pdir;
			s_ref_idx_ptr = mb_cache_ptr->ref_idx_cache[s_list]+offset;
			s_ref_mv_ptr = (int32 *)(mb_cache_ptr->mv_cache[s_list]) + offset;
			s_mvd_ptr = (int32 *)(mb_cache_ptr->mvd_cache[s_list]) + offset;

			s_ref_frame_id[s_dir] = ((s_list == 0) ?  s_ref_idx_ptr[0] : g_list1_map_list0[s_ref_idx_ptr[0]]);

			pred_mv_xy = H264Dec_pred_motion (s_ref_idx_ptr, (int16 *)s_ref_mv_ptr, 2);
			pred_mv_x = ((pred_mv_xy << 16) >> 16);
			pred_mv_y = (pred_mv_xy >> 16);

			s_mvd_xy = s_mvd_ptr[0];
			mvd_x = ((s_mvd_xy << 16) >> 16);
			mvd_y = (s_mvd_xy >> 16);

			mv_x = mvd_x + pred_mv_x;
			mv_y = mvd_y + pred_mv_y;

			mv_cmd[s_dir] = (mv_y << 16) | (mv_x & 0xffff);
		#if defined(H264_BIG_ENDIAN)
			mv_xy  = (mv_x << 16) | (mv_y & 0xffff);
		#else
			mv_xy  = mv_cmd[s_dir];
		#endif

			s_ref_mv_ptr[0] = s_ref_mv_ptr[1] = mv_xy;

			s_dir++;
		}while(s_dir < pdir);

		//configure mv command
		s_ref_blk_id = b8*4+b8x4*2;
		WRITE_MC_BLK_INFO;
		VSP_WRITE_REG_CQM(VSP_MCA_REG_BASE+MCA_MV_CBUF_OFF, mv_cmd[0], "configure MCA command buffer, block mv info");
		if (s_ref_bir_blk)
		{
			VSP_WRITE_REG_CQM(VSP_MCA_REG_BASE+MCA_MV_CBUF_OFF, mv_cmd[1], "configure MCA command buffer, block mv info");
    		VSP_WRITE_CMD_INFO((VSP_MCA << CQM_SHIFT_BIT) | (3<<24) | (MCA_MV_CBUF_WOFF<<16) | (MCA_MV_CBUF_WOFF<<8) | MCA_BLK_CBUF_WOFF);
		}else
		{
			VSP_WRITE_CMD_INFO((VSP_MCA << CQM_SHIFT_BIT) | (2<<24) |(MCA_MV_CBUF_WOFF<<8) |MCA_BLK_CBUF_WOFF);
		}
		
		if ((b8 == 3) && (b8x4 == 1))
		{
			s_ref_blk_end = TRUE;
		}

		s_ref_blk_id += 1;	
		WRITE_MC_BLK_INFO;
		VSP_WRITE_REG_CQM(VSP_MCA_REG_BASE+MCA_MV_CBUF_OFF, mv_cmd[0], "configure MCA command buffer, block mv info");
		if (s_ref_bir_blk)
		{
			VSP_WRITE_REG_CQM(VSP_MCA_REG_BASE+MCA_MV_CBUF_OFF, mv_cmd[1], "configure MCA command buffer, block mv info");
    		VSP_WRITE_CMD_INFO((VSP_MCA << CQM_SHIFT_BIT) | (3<<24) | (MCA_MV_CBUF_WOFF<<16) | (MCA_MV_CBUF_WOFF<<8) | MCA_BLK_CBUF_WOFF);
		}else
		{
			VSP_WRITE_CMD_INFO((VSP_MCA << CQM_SHIFT_BIT) | (2<<24) |(MCA_MV_CBUF_WOFF<<8) |MCA_BLK_CBUF_WOFF);
		}
	}

	return pdir;
}

PUBLIC int32 H264Dec_mv_prediction_P8x8_4x8 (DEC_MB_INFO_T *mb_info_ptr, DEC_MB_CACHE_T *mb_cache_ptr, int32 cache_offset, int32 b8)
{
	int32 pred_mv_x, pred_mv_y, pred_mv_xy;
	int32 mvd_x, mvd_y;
	int32 mv_x, mv_y;
	int32 mv_xy;
	int32 mv_cmd[2];
	uint32 b4x8;
	uint32 offset;
	int32 pdir = mb_cache_ptr->b8pdir[b8];

	s_ref_blk_end = FALSE;
	s_ref_bir_blk = (pdir == 2) ? 1 : 0;
	s_ref_cmd_type = 0;
	s_ref_frame_id[0] = 0;
	s_ref_frame_id[1] = 0;
	s_ref_blk_id = 0;
	s_ref_blk_size = MC_BLKSIZE_4x4;

	if (pdir == 2)
	{
		mb_cache_ptr->mbc_b8pdir[b8] = 1;
	}
	
	for (b4x8 = 0; b4x8 < 2; b4x8++)
	{
		offset = cache_offset + b4x8;

		s_dir = 0;
		do
		{
			s_list = (pdir == 2) ? s_dir : pdir;
			s_ref_idx_ptr = mb_cache_ptr->ref_idx_cache[s_list]+offset;
			s_ref_mv_ptr = (int32 *)(mb_cache_ptr->mv_cache[s_list]) + offset;
			s_mvd_ptr = (int32 *)(mb_cache_ptr->mvd_cache[s_list]) + offset;

			s_ref_frame_id[s_dir] = ((s_list == 0) ?  s_ref_idx_ptr[0] : g_list1_map_list0[s_ref_idx_ptr[0]]);

			pred_mv_xy = H264Dec_pred_motion (s_ref_idx_ptr, (int16 *)s_ref_mv_ptr, 1);
			pred_mv_x = ((pred_mv_xy << 16) >> 16);
			pred_mv_y = (pred_mv_xy >> 16);

			s_mvd_xy = s_mvd_ptr[0];
			mvd_x = ((s_mvd_xy << 16) >> 16);
			mvd_y = (s_mvd_xy >> 16);

			mv_x = mvd_x + pred_mv_x;
			mv_y = mvd_y + pred_mv_y;

			mv_cmd[s_dir] = (mv_y << 16) | (mv_x & 0xffff);
		#if defined(H264_BIG_ENDIAN)
			mv_xy  = (mv_x << 16) | (mv_y & 0xffff);
		#else
			mv_xy  = mv_cmd[s_dir];
		#endif

			s_ref_mv_ptr[0] = s_ref_mv_ptr[CTX_CACHE_WIDTH] = mv_xy;

			s_dir++;
		}while(s_dir < pdir);

		//configure mv command
		s_ref_blk_id = b8*4+b4x8;
		WRITE_MC_BLK_INFO;
		VSP_WRITE_REG_CQM(VSP_MCA_REG_BASE+MCA_MV_CBUF_OFF, mv_cmd[0], "configure MCA command buffer, block mv info");
		if (s_ref_bir_blk)
		{
			VSP_WRITE_REG_CQM(VSP_MCA_REG_BASE+MCA_MV_CBUF_OFF, mv_cmd[1], "configure MCA command buffer, block mv info");
			VSP_WRITE_CMD_INFO((VSP_MCA << CQM_SHIFT_BIT) | (3<<24) | (MCA_MV_CBUF_WOFF<<16) | (MCA_MV_CBUF_WOFF<<8) | MCA_BLK_CBUF_WOFF);

			mb_cache_ptr->mbc_b8pdir[b8] = 1;
		}else
		{
			VSP_WRITE_CMD_INFO((VSP_MCA << CQM_SHIFT_BIT) | (2<<24) |(MCA_MV_CBUF_WOFF<<8) |MCA_BLK_CBUF_WOFF);
		}
		
		if ((b8 == 3) && (b4x8 == 1))
		{
			s_ref_blk_end = TRUE;
		}else
		{
			s_ref_blk_end = FALSE;
		}

		s_ref_blk_id += 2;	
		WRITE_MC_BLK_INFO;;
		VSP_WRITE_REG_CQM(VSP_MCA_REG_BASE+MCA_MV_CBUF_OFF, mv_cmd[0], "configure MCA command buffer, block mv info");
		if (s_ref_bir_blk)
		{
			VSP_WRITE_REG_CQM(VSP_MCA_REG_BASE+MCA_MV_CBUF_OFF, mv_cmd[1], "configure MCA command buffer, block mv info");
			VSP_WRITE_CMD_INFO((VSP_MCA << CQM_SHIFT_BIT) | (3<<24) | (MCA_MV_CBUF_WOFF<<16) | (MCA_MV_CBUF_WOFF<<8) | MCA_BLK_CBUF_WOFF);
		}else
		{
			VSP_WRITE_CMD_INFO((VSP_MCA << CQM_SHIFT_BIT) | (2<<24) |(MCA_MV_CBUF_WOFF<<8) |MCA_BLK_CBUF_WOFF);
		}
	}

	return pdir;
}

PUBLIC int32 H264Dec_mv_prediction_P8x8_4x4 (DEC_MB_INFO_T *mb_info_ptr, DEC_MB_CACHE_T *mb_cache_ptr, int32 cache_offset, int32 b8)
{
	int32 pred_mv_x, pred_mv_y, pred_mv_xy;
	int32 mvd_x, mvd_y;
	int32 mv_x, mv_y;
	int32 mv_xy;
	int32 mv_cmd[2];
	uint32 b4;
	uint32 offset;
	int32 pdir = mb_cache_ptr->b8pdir[b8];

	s_ref_blk_end = FALSE;
	s_ref_bir_blk = (pdir == 2) ? 1 : 0;
	s_ref_cmd_type = 0;
	s_ref_frame_id[0] = 0;
	s_ref_frame_id[1] = 0;
	s_ref_blk_id = 0;
	s_ref_blk_size = MC_BLKSIZE_4x4;

	if (pdir == 2)
	{
		mb_cache_ptr->mbc_b8pdir[b8] = 1;
	}

	for (b4 = 0; b4 < 4; b4++)
	{
		offset = cache_offset + (b4>>1) * CTX_CACHE_WIDTH + (b4&1);

		s_dir = 0;
		do
		{
			s_list = (pdir == 2) ? s_dir : pdir;
			s_ref_idx_ptr = mb_cache_ptr->ref_idx_cache[s_list]+offset;
			s_ref_mv_ptr = (int32 *)(mb_cache_ptr->mv_cache[s_list]) + offset;
			s_mvd_ptr = (int32 *)(mb_cache_ptr->mvd_cache[s_list]) + offset;

			s_ref_frame_id[s_dir] = ((s_list == 0) ?  s_ref_idx_ptr[0] : g_list1_map_list0[s_ref_idx_ptr[0]]);

			pred_mv_xy = H264Dec_pred_motion (s_ref_idx_ptr, (int16 *)s_ref_mv_ptr, 1);
			pred_mv_x = ((pred_mv_xy << 16) >> 16);
			pred_mv_y = (pred_mv_xy >> 16);

			s_mvd_xy = s_mvd_ptr[0];
			mvd_x = ((s_mvd_xy << 16) >> 16);
			mvd_y = (s_mvd_xy >> 16);

			mv_x = mvd_x + pred_mv_x;
			mv_y = mvd_y + pred_mv_y;

			mv_cmd[s_dir] = (mv_y << 16) | (mv_x & 0xffff);
		#if defined(H264_BIG_ENDIAN)
			mv_xy  = (mv_x << 16) | (mv_y & 0xffff);
		#else
			mv_xy  = mv_cmd[s_dir];
		#endif
			s_ref_mv_ptr[0] = mv_xy;
		
			s_dir++;
		}while(s_dir < pdir);
	
		//configure mv command
		s_ref_blk_id = b8*4+b4;

		if ((b8 == 3) && (b4 == 3))
		{
			s_ref_blk_end = TRUE;
		}

		WRITE_MC_BLK_INFO;
		VSP_WRITE_REG_CQM(VSP_MCA_REG_BASE+MCA_MV_CBUF_OFF, mv_cmd[0], "configure MCA command buffer, block mv info");
		if (s_ref_bir_blk)
		{
			VSP_WRITE_REG_CQM(VSP_MCA_REG_BASE+MCA_MV_CBUF_OFF, mv_cmd[1], "configure MCA command buffer, block mv info");
    		VSP_WRITE_CMD_INFO((VSP_MCA << CQM_SHIFT_BIT) | (3<<24) | (MCA_MV_CBUF_WOFF<<16) | (MCA_MV_CBUF_WOFF<<8) | MCA_BLK_CBUF_WOFF);
		}else
		{
			VSP_WRITE_CMD_INFO((VSP_MCA << CQM_SHIFT_BIT) | (2<<24) |(MCA_MV_CBUF_WOFF<<8) |MCA_BLK_CBUF_WOFF);
		}
	}

	return pdir;
}

PUBLIC int8 H264Dec_MC8x8_direct_spatial(DEC_MB_INFO_T *mb_info_ptr, DEC_MB_CACHE_T *mb_cache_ptr, uint8 b8, int32 cache_offset)
{
	int32 fw_rFrame,bw_rFrame;
	int32 mv_xy;
	uint32 row;
	uint32 use_4x4mc = 0;
	int32 mv_cmd[2];
	uint32 b8pdir = 0;

	fw_rFrame = mb_cache_ptr->fw_rFrame;
	bw_rFrame = mb_cache_ptr->bw_rFrame;

	s_dir = 0;	
	
	if (fw_rFrame >= 0)
	{
		mv_cmd[s_dir] = mb_cache_ptr->fw_mv_xy;
	#if defined(H264_BIG_ENDIAN)
		mv_x = (mb_cache_ptr->fw_mv_xy >> 16);
		mv_y = ((mb_cache_ptr->fw_mv_xy << 16) >> 16);
		mv_xy  = (mv_x << 16) | (mv_y & 0xffff);
	#else
		mv_xy  = mb_cache_ptr->fw_mv_xy;
	#endif

		//set mv cache
		if (mv_xy)	//curr mv context has been set to value 0 in fill_mb func.
		{			
			uint32 col;
			int32 condition = (fw_rFrame || (g_list[1][0]->is_long_term)) ? 0 : 1;
			int8 *moving_block = mb_cache_ptr->moving_block + (( (b8>>1)*2)*4 + (b8&0x1)*2);
			
			s_ref_mv_ptr = (int32 *)(mb_cache_ptr->mv_cache[0]) + cache_offset;
			for (row = 0; row < 2; row++)
			{
				for (col = 0; col < 2; col++)
				{
					if  (condition && (!moving_block[col]) )
				    {
		       			use_4x4mc = 1;
			       	}else
			       	{
	      				s_ref_mv_ptr[col] = mv_xy;
			       	}
				}

				moving_block += 4;
				s_ref_mv_ptr += CTX_CACHE_WIDTH;
			}
		}
			
		b8pdir = 0;
		s_dir++;
	}

	if (bw_rFrame >= 0)
	{	
		mv_cmd[s_dir] = mb_cache_ptr->bw_mv_xy;
	#if defined(H264_BIG_ENDIAN)
		mv_x = (mb_cache_ptr->bw_mv_xy >> 16);
		mv_y = ((mb_cache_ptr->bw_mv_xy << 16) >> 16);
		mv_xy  = (mv_x << 16) | (mv_y & 0xffff);
	#else
		mv_xy  = mb_cache_ptr->bw_mv_xy;
	#endif

		//set mv cache
		if (mv_xy)	//curr mv context has been set to value 0 in fill_mb func.
		{			
			uint32 col;
			int32 condition = (bw_rFrame || (g_list[1][0]->is_long_term)) ? 0 : 1;
			int8 *moving_block = mb_cache_ptr->moving_block + (( (b8>>1)*2)*4 + (b8&0x1)*2);
			
			s_ref_mv_ptr = (int32 *)(mb_cache_ptr->mv_cache[1]) + cache_offset;
			for (row = 0; row < 2; row++)
			{
				for (col = 0; col < 2; col++)
				{
					if  (condition && (!moving_block[col]))	
				    {
		       			use_4x4mc = 1;
			       	}else
			       	{
	      				s_ref_mv_ptr[col] = mv_xy;
			       	}
				}

				moving_block += 4;
				s_ref_mv_ptr += CTX_CACHE_WIDTH;
			}
		}
			
		b8pdir = 1;
		s_dir++;
	}else
	{
		// fw < 0 and bw < 0
		if (fw_rFrame < 0)
		{		
			fw_rFrame =  
			bw_rFrame = 0;
			s_dir = 2;
			mv_cmd[0] = 
			mv_cmd[1] = 0;
		}	
	}

	//save ref_idx to mb_cache.
	if (fw_rFrame >= 0)
	{
		s_ref_frame_id[0] = fw_rFrame;
		
		s_ref_idx_ptr = mb_cache_ptr->ref_idx_cache[0]+cache_offset;	
		s_ref_idx_ptr[0] = s_ref_idx_ptr[1] = fw_rFrame;	s_ref_idx_ptr += CTX_CACHE_WIDTH;
		s_ref_idx_ptr[0] = s_ref_idx_ptr[1] = fw_rFrame;
	}

	if (bw_rFrame >= 0)
	{
		s_ref_frame_id[1] = g_list1_map_list0[bw_rFrame];
		
		s_ref_idx_ptr = mb_cache_ptr->ref_idx_cache[1]+cache_offset;
		s_ref_idx_ptr[0] = s_ref_idx_ptr[1] = bw_rFrame;	s_ref_idx_ptr += CTX_CACHE_WIDTH;
		s_ref_idx_ptr[0] = s_ref_idx_ptr[1] = bw_rFrame;
	}
	s_ref_bir_blk = (s_dir == 2) ? 1 : 0;
	
	if (s_ref_bir_blk)
	{
		b8pdir = 2;
	}
	mb_cache_ptr->b8pdir[b8] = b8pdir;

	//only one direction, and ref in list1//lsw for debug
	if (b8pdir == 1)
	{
		s_ref_frame_id[0] = s_ref_frame_id[1];
	}
		
	s_ref_blk_id = b8;

	if (!use_4x4mc)
	{
		s_ref_blk_size = MC_BLKSIZE_8x8;
		s_ref_blk_end = (b8 == 3) ? 1 : 0; 
		WRITE_MC_BLK_INFO;
		VSP_WRITE_REG_CQM(VSP_MCA_REG_BASE+MCA_MV_CBUF_OFF, mv_cmd[0], "configure MCA command buffer, block mv info");
		if (s_ref_bir_blk)
		{
			VSP_WRITE_REG_CQM(VSP_MCA_REG_BASE+MCA_MV_CBUF_OFF, mv_cmd[1], "configure MCA command buffer, block mv info");
			VSP_WRITE_CMD_INFO((VSP_MCA << CQM_SHIFT_BIT) | (3<<24) | (MCA_MV_CBUF_WOFF<<16) | (MCA_MV_CBUF_WOFF<<8) | MCA_BLK_CBUF_WOFF);

			mb_cache_ptr->mbc_b8pdir[b8] = 1;
		}else
		{
			VSP_WRITE_CMD_INFO((VSP_MCA << CQM_SHIFT_BIT) | (2<<24) | (MCA_MV_CBUF_WOFF<<8) | MCA_BLK_CBUF_WOFF);
		}
	}else
	{
		H264Dec_Config_4x4MC_info(g_image_ptr, mb_cache_ptr, b8, s_dir);
	}
	
	return b8pdir;
}

PUBLIC int8 H264Dec_MC8x8_direct_temporal(DEC_MB_INFO_T *mb_info_ptr, DEC_MB_CACHE_T *mb_cache_ptr, uint8 b8, int32 cache_offset)
{
	uint32 use_4x4mc = 0;
	int32 mv_cmd[2];
	uint32 b8pdir = 0;
	int32 j,offset_b4;
	int32 src_offset = (b8>>1)*8 + (b8&1)*2;
	int8  *ref_idx_ptr0, *ref_idx_ptr1;
	int32 *ref_mv_ptr0, *ref_mv_ptr1;

	//save mv and ref_idx to mb_cache
	for (s_list = 0; s_list < 2; s_list++)
	{
		int8  *ref_idx_src_ptr = mb_cache_ptr->ref_idx_cache_direct[s_list]+src_offset;
		int8  *ref_idx_dst_ptr = mb_cache_ptr->ref_idx_cache[s_list]+cache_offset;
		int32  *ref_mv_src_ptr = (int32 *)(mb_cache_ptr->mv_cache_direct[s_list])+src_offset;
		int32  *ref_mv_dst_ptr = (int32 *)(mb_cache_ptr->mv_cache[s_list])+cache_offset;
		int32 row;

		for (row = 0 ; row < 2; row++)
		{
			((int16 *)ref_idx_dst_ptr)[0] = ((int16 *)ref_idx_src_ptr)[0];
			
			ref_mv_dst_ptr[0] = ref_mv_src_ptr[0];
			ref_mv_dst_ptr[1] = ref_mv_src_ptr[1];

			ref_idx_src_ptr += 4; ref_idx_dst_ptr += CTX_CACHE_WIDTH;
			ref_mv_src_ptr += 4;  ref_mv_dst_ptr += CTX_CACHE_WIDTH;
		}
	}

	ref_idx_ptr0 = mb_cache_ptr->ref_idx_cache_direct[0] + src_offset;
	ref_idx_ptr1 = mb_cache_ptr->ref_idx_cache_direct[1] + src_offset;
	ref_mv_ptr0 = (int32*)(mb_cache_ptr->mv_cache_direct[0]) + src_offset;
	ref_mv_ptr1 = (int32*)(mb_cache_ptr->mv_cache_direct[1]) + src_offset;

	for (j=1 ; j < 4; j++)
	{
		offset_b4 = ((j>>1)<<2)+(j&1);
		
		if(ref_idx_ptr0[offset_b4]!=ref_idx_ptr0[0]
			|| ref_idx_ptr1[offset_b4]!=ref_idx_ptr1[0]
			|| ref_mv_ptr0[offset_b4]!= ref_mv_ptr0[0]
			|| ref_mv_ptr1[offset_b4]!= ref_mv_ptr1[0])
		{
			use_4x4mc = 1;
			break;
		}
	}

	s_dir = 2;
	b8pdir = 2;
	mb_cache_ptr->b8pdir[b8] = b8pdir;
	
	s_ref_bir_blk = 1;		

	if (!use_4x4mc)
	{
		s_ref_blk_size = MC_BLKSIZE_8x8;
		s_ref_blk_end = (b8 == 3) ? 1 : 0; 
		s_ref_blk_id = b8;
		
		s_ref_frame_id[0] = ref_idx_ptr0[0];
		mv_cmd[0] = ((int32 *)mb_cache_ptr->mv_cache_direct[0])[src_offset];
		
		s_ref_frame_id[1] = g_list1_map_list0[ref_idx_ptr1[0]];
		mv_cmd[1] = ((int32 *)mb_cache_ptr->mv_cache_direct[1])[src_offset];
		
		WRITE_MC_BLK_INFO;		
		VSP_WRITE_REG_CQM(VSP_MCA_REG_BASE+MCA_MV_CBUF_OFF, mv_cmd[0], "configure MCA command buffer, block mv info");		
		VSP_WRITE_REG_CQM(VSP_MCA_REG_BASE+MCA_MV_CBUF_OFF, mv_cmd[1], "configure MCA command buffer, block mv info");
		VSP_WRITE_CMD_INFO((VSP_MCA << CQM_SHIFT_BIT) | (3<<24) | (MCA_MV_CBUF_WOFF<<16) | (MCA_MV_CBUF_WOFF<<8) | MCA_BLK_CBUF_WOFF);

		mb_cache_ptr->mbc_b8pdir[b8] = 1;
	}else
	{
		H264Dec_Config_4x4MC_info(g_image_ptr, mb_cache_ptr, b8, s_dir);
	}
	
	return b8pdir;
}

LOCAL void H264Dec_mv_prediction_P8x8 (DEC_IMAGE_PARAMS_T *img_ptr, DEC_MB_INFO_T *mb_info_ptr, DEC_MB_CACHE_T *mb_cache_ptr)
{
	uint32 i;
	int8  *ref_idx_ptr[2];
	int32 b8mode;
	int32 ref_idx_b8_1[2], ref_idx_b8_3[2];
	int32 cache_offset;
	int8  b8pdir;

	for (i = 0; i < 2; i++)
	{
		ref_idx_ptr[i] = mb_cache_ptr->ref_idx_cache[i] + CTX_CACHE_WIDTH_PLUS4;

		ref_idx_b8_1[i] = ref_idx_ptr[i][2];	ref_idx_ptr[i][2] = PART_NOT_AVAIL;
		ref_idx_b8_3[i] = ref_idx_ptr[i][CTX_CACHE_WIDTH_X2+2];	ref_idx_ptr[i][CTX_CACHE_WIDTH_X2+2] = PART_NOT_AVAIL;
	}

	//8x8 0
	cache_offset = CTX_CACHE_WIDTH_PLUS4;
	b8mode = mb_cache_ptr->b8mode[0];
	if (b8mode != 0)
	{
		b8pdir = (*img_ptr->b8_mv_pred_func[b8mode-4])(mb_info_ptr, mb_cache_ptr, cache_offset, 0);
	}else
	{
		b8pdir = MC8x8_direct(mb_info_ptr, mb_cache_ptr, 0,  cache_offset);
	}
	
	if (!s_ref_bir_blk && (img_ptr->type == B_SLICE))
	{
		H264Dec_FillNoRefList_8x8(mb_info_ptr, 0, b8pdir);
	}

	//8x8 1
	cache_offset = CTX_CACHE_WIDTH_PLUS4+2;
	ref_idx_ptr[0][2] = ref_idx_b8_1[0];
	ref_idx_ptr[1][2] = ref_idx_b8_1[1];
	b8mode = mb_cache_ptr->b8mode[1];
	if (b8mode != 0)
	{
		b8pdir = (*img_ptr->b8_mv_pred_func[b8mode-4])(mb_info_ptr, mb_cache_ptr, cache_offset, 1);
	}else
	{
		b8pdir = MC8x8_direct(mb_info_ptr, mb_cache_ptr, 1,  cache_offset);
	}
	
	if (!s_ref_bir_blk && (img_ptr->type == B_SLICE))
	{
		H264Dec_FillNoRefList_8x8(mb_info_ptr, 1, b8pdir);
	}
	
	//8x8 2
	cache_offset = CTX_CACHE_WIDTH_PLUS4 + CTX_CACHE_WIDTH_X2;
	b8mode = mb_cache_ptr->b8mode[2];
	if (b8mode != 0)
	{		
		b8pdir = (*img_ptr->b8_mv_pred_func[b8mode-4])(mb_info_ptr, mb_cache_ptr, cache_offset, 2);
	}else
	{
		b8pdir = MC8x8_direct(mb_info_ptr, mb_cache_ptr, 2,  cache_offset);
	}
	
	if (!s_ref_bir_blk && (img_ptr->type == B_SLICE))
	{
		H264Dec_FillNoRefList_8x8(mb_info_ptr, 2, b8pdir);
	}

	//8x8 3
	cache_offset = CTX_CACHE_WIDTH_PLUS4 + CTX_CACHE_WIDTH_X2 + 2;
	ref_idx_ptr[0] = mb_cache_ptr->ref_idx_cache[0] + cache_offset;
	ref_idx_ptr[1] = mb_cache_ptr->ref_idx_cache[1] + cache_offset;
	ref_idx_ptr[0][0] = ref_idx_b8_3[0];
	ref_idx_ptr[1][0] = ref_idx_b8_3[1];
	b8mode = mb_cache_ptr->b8mode[3];
	if (b8mode != 0)
	{
		b8pdir = (*img_ptr->b8_mv_pred_func[b8mode-4])(mb_info_ptr, mb_cache_ptr, cache_offset, 3);
	}else
	{
		b8pdir = MC8x8_direct(mb_info_ptr, mb_cache_ptr, 3,  cache_offset);
	}
	
	if (!s_ref_bir_blk && (img_ptr->type == B_SLICE))
	{
		H264Dec_FillNoRefList_8x8(mb_info_ptr, 3, b8pdir);
	}

	return;
}

int mvscale[MAX_REF_FRAME_NUMBER];

#define JUDGE_MOVING_BLOCK	\
{\
	int32 condition0, condition1;\
\
	*ref_idx_cache_direct0 = fs->ref_idx_ptr[0][offset];\
	*ref_idx_cache_direct1 = fs->ref_idx_ptr[1][offset];\
	*ref_pic_id_cache_direct0++ = fs->ref_pic_id_ptr[0][offset];\
	*ref_pic_id_cache_direct1++ = fs->ref_pic_id_ptr[1][offset];\
	((int32 *)ref_mv_cache_direct0)[0]  = ((int32 *)(fs->mv_ptr[0]))[offset];\
	((int32 *)ref_mv_cache_direct1)[0]  = ((int32 *)(fs->mv_ptr[1]))[offset];\
\
	condition0 = (fs->is_long_term				|| \
				((ref_idx_cache_direct0[0] != 0)			|| \
				((ABS(ref_mv_cache_direct0[0])>>1) != 0) || \
				((ABS(ref_mv_cache_direct0[1])>>1) != 0))); \
\
	if (condition0)\
	{\
		condition1 = ((ref_idx_cache_direct0[0] != -1)		|| \
					(ref_idx_cache_direct1[0] != 0)			|| \
					((ABS(ref_mv_cache_direct1[0])>>1) != 0) || \
					((ABS(ref_mv_cache_direct1[1])>>1) != 0));\
		mb_cache_ptr->moving_block[blk4x4Idx] = condition1;\
	}else\
	{\
		mb_cache_ptr->moving_block[blk4x4Idx] = 0;\
	}\
\
	ref_idx_cache_direct0++;\
	ref_idx_cache_direct1++;\
	ref_mv_cache_direct0 += 2;\
	ref_mv_cache_direct1 += 2;\
}

PUBLIC void H264Dec_direct_mv_spatial(DEC_IMAGE_PARAMS_T *img_ptr, DEC_MB_INFO_T *mb_info_ptr, DEC_MB_CACHE_T *mb_cache_ptr)
{
	int fw_rFrameL, fw_rFrameU, fw_rFrameUL, fw_rFrameUR;
	int bw_rFrameL, bw_rFrameU, bw_rFrameUL, bw_rFrameUR;
	int32 fw_rFrame,bw_rFrame;
	int32 i, offset ,blk4x4Idx;

	DEC_STORABLE_PICTURE_T	*fs = g_list[1][0];
	int8  *ref_idx_cache_direct0 = mb_cache_ptr->ref_idx_cache_direct[0];
	int8  *ref_idx_cache_direct1 = mb_cache_ptr->ref_idx_cache_direct[1];
	int32 *ref_pic_id_cache_direct0 = mb_cache_ptr->ref_pic_id_cache_direct[0];
	int32 *ref_pic_id_cache_direct1 = mb_cache_ptr->ref_pic_id_cache_direct[1];
	int16 *ref_mv_cache_direct0 = mb_cache_ptr->mv_cache_direct[0];
	int16 *ref_mv_cache_direct1 = mb_cache_ptr->mv_cache_direct[1];

	for (blk4x4Idx = 0; blk4x4Idx < 16; blk4x4Idx++)
	{
		offset = img_ptr->corner_map[blk4x4Idx];
		JUDGE_MOVING_BLOCK
	}

	for (i = 0; i < 2; i++)
	{
		s_ref_mv_ptr_array[i] = (int32 *)(mb_cache_ptr->mv_cache[i]) + CTX_CACHE_WIDTH_PLUS4;
		s_ref_idx_ptr_array[i] = mb_cache_ptr->ref_idx_cache[i] + CTX_CACHE_WIDTH_PLUS4;
	}

	fw_rFrameL = s_ref_idx_ptr_array[0][-1];
	fw_rFrameU = s_ref_idx_ptr_array[0][-CTX_CACHE_WIDTH];
	fw_rFrameUL = s_ref_idx_ptr_array[0][-CTX_CACHE_WIDTH-1];
	fw_rFrameUR = mb_cache_ptr->mb_avail_c ? (s_ref_idx_ptr_array[0][-CTX_CACHE_WIDTH+4]) : fw_rFrameUL;
		    
	bw_rFrameL = s_ref_idx_ptr_array[1][-1];
	bw_rFrameU = s_ref_idx_ptr_array[1][-CTX_CACHE_WIDTH];
	bw_rFrameUL = s_ref_idx_ptr_array[1][-CTX_CACHE_WIDTH-1];
	bw_rFrameUR = mb_cache_ptr->mb_avail_c ? (s_ref_idx_ptr_array[1][-CTX_CACHE_WIDTH+4]) : bw_rFrameUL;

	fw_rFrame = (fw_rFrameL >= 0 && fw_rFrameU >= 0) ? mmin(fw_rFrameL,fw_rFrameU): mmax(fw_rFrameL,fw_rFrameU);
	fw_rFrame = (fw_rFrame >= 0 && fw_rFrameUR >= 0) ? mmin(fw_rFrame,fw_rFrameUR): mmax(fw_rFrame,fw_rFrameUR);
	mb_cache_ptr->fw_rFrame = fw_rFrame;
	  
	bw_rFrame = (bw_rFrameL >= 0 && bw_rFrameU >= 0) ? mmin(bw_rFrameL,bw_rFrameU): mmax(bw_rFrameL,bw_rFrameU);
	bw_rFrame = (bw_rFrame >= 0 && bw_rFrameUR >= 0) ? mmin(bw_rFrame,bw_rFrameUR): mmax(bw_rFrame,bw_rFrameUR);
	mb_cache_ptr->bw_rFrame = bw_rFrame;

	mb_cache_ptr->direct_pdir = 0;
	if (fw_rFrame >= 0)
	{
		int32 tmp;
		s_ref_idx_ptr = s_ref_idx_ptr_array[0];
		s_ref_mv_ptr = s_ref_mv_ptr_array[0];
		tmp = s_ref_idx_ptr[0];
		s_ref_idx_ptr[0] = fw_rFrame;
	    mb_cache_ptr->fw_mv_xy = H264Dec_pred_motion (s_ref_idx_ptr, (int16 *)s_ref_mv_ptr, 4);
	    s_ref_idx_ptr[0] = tmp;
		mb_cache_ptr->direct_pdir++;
	}

	if (bw_rFrame >= 0)
	{
		int32 tmp;
		s_ref_idx_ptr = s_ref_idx_ptr_array[1];
		s_ref_mv_ptr = s_ref_mv_ptr_array[1];
		tmp = s_ref_idx_ptr[0];
		s_ref_idx_ptr[0] = bw_rFrame;
	    mb_cache_ptr->bw_mv_xy = H264Dec_pred_motion (s_ref_idx_ptr, (int16 *)s_ref_mv_ptr, 4);
	    s_ref_idx_ptr[0] = tmp;
		mb_cache_ptr->direct_pdir++;
	}
}

PUBLIC void H264Dec_direct_mv_temporal(DEC_IMAGE_PARAMS_T *img_ptr, DEC_MB_INFO_T *mb_info_ptr, DEC_MB_CACHE_T *mb_cache_ptr)
{
	DEC_STORABLE_PICTURE_T	*fs = g_list[1][0];
	int32 iref_max = mmin(img_ptr->ref_count[0], g_list_size[0]);
	int32 i, offset, blk4x4Idx;
	int32 blk4x4_id;
	int8  *ref_idx_cache_direct0 = mb_cache_ptr->ref_idx_cache_direct[0];
	int8  *ref_idx_cache_direct1 = mb_cache_ptr->ref_idx_cache_direct[1];
	int32 *ref_pic_id_cache_direct0 = mb_cache_ptr->ref_pic_id_cache_direct[0];
	int32 *ref_pic_id_cache_direct1 = mb_cache_ptr->ref_pic_id_cache_direct[1];
	int16 *ref_mv_cache_direct0 = mb_cache_ptr->mv_cache_direct[0];
	int16 *ref_mv_cache_direct1 = mb_cache_ptr->mv_cache_direct[1];

	for (blk4x4Idx = 0; blk4x4Idx < 16; blk4x4Idx++)
	{
		offset = img_ptr->corner_map[blk4x4Idx];

		*ref_idx_cache_direct0++ = fs->ref_idx_ptr[0][offset];
		*ref_idx_cache_direct1++ = fs->ref_idx_ptr[1][offset];
		*ref_pic_id_cache_direct0++ = fs->ref_pic_id_ptr[0][offset];
		*ref_pic_id_cache_direct1++ = fs->ref_pic_id_ptr[1][offset];
		((int32 *)ref_mv_cache_direct0)[0]  = ((int32 *)(fs->mv_ptr[0]))[offset];
		((int32 *)ref_mv_cache_direct1)[0]  = ((int32 *)(fs->mv_ptr[1]))[offset];	

		ref_mv_cache_direct0 += 2;
		ref_mv_cache_direct1 += 2;
	}	
	
	for (i = 0; i < iref_max; i++)
	{
		int32 prescale, iTRb, iTRp;

		iTRb = Clip3( -128, 127, (g_dec_picture_ptr->poc - g_list[0][i]->poc ));
		iTRp = Clip3( -128, 127,  (g_list[1][0]->poc - g_list[0][i]->poc));

		if (iTRp!=0)
		{
			prescale = ( 16384 + ABS( iTRp / 2 ) ) / iTRp;
			mvscale[i] = Clip3( -1024, 1023, ( iTRb * prescale + 32 ) >> 6 ) ;
		}else
		{
		  	mvscale[i] = 9999;
		}
	}

	//////////////lsw for direct mode (temporal direct mode)
	ref_idx_cache_direct0 = mb_cache_ptr->ref_idx_cache_direct[0];
	ref_idx_cache_direct1 = mb_cache_ptr->ref_idx_cache_direct[1];
	ref_pic_id_cache_direct0 = mb_cache_ptr->ref_pic_id_cache_direct[0];
	ref_pic_id_cache_direct1 = mb_cache_ptr->ref_pic_id_cache_direct[1];
	ref_mv_cache_direct0 = mb_cache_ptr->mv_cache_direct[0];
	ref_mv_cache_direct1 = mb_cache_ptr->mv_cache_direct[1];

	for(blk4x4_id = 0; blk4x4_id < 16; blk4x4_id++)
	{
		int32 b8 = b8map[blk4x4_id] ;//((j>>1)<<1) + (i>>1);
		int32 refList = (ref_idx_cache_direct0[0]== -1 ? 1 : 0);
			
		if (mb_cache_ptr->b8mode[b8] == 0)
		{
			int32 col_ref_idx =  (refList == 1) ? ref_idx_cache_direct1[0] : ref_idx_cache_direct0[0];
				
			if (col_ref_idx == -1)
			{
				mb_cache_ptr->fw_rFrame = 0;
				mb_cache_ptr->fw_mv_xy = 0;
				mb_cache_ptr->bw_mv_xy = 0;
				((int32 *)ref_mv_cache_direct0)[0] = 0;
				((int32 *)ref_mv_cache_direct1)[0] = 0;					
			}else
			{
				int mapped_idx=0;
				int32 ref_pic_id = ((refList == 0) ? ref_pic_id_cache_direct0[0] : ref_pic_id_cache_direct1[0]);
				int iref_max = mmin(img_ptr->ref_count[0], g_list_size[0]);
				int iref;

				for (iref = 0;iref < iref_max; iref++)
				{
					// If the current MB is a frame MB and the colocated is from a field picture,
					// then the co_located_ref_id may have been generated from the wrong value of
					// frame_poc if it references it's complementary field, so test both POC values
					if((g_list[0][iref]->poc * 2) == ref_pic_id)
					{
						mapped_idx=iref;
						break;
					}
					else //! invalid index. Default to zero even though this case should not happen
						mapped_idx = INVALID_REF_ID;
				}

				if (INVALID_REF_ID == mapped_idx)
				{
					PRINTF("temporal direct error\ncolocated block has ref that is unavailable\n");
				//	exit(-1);
				}

				if (mvscale[mapped_idx] == 9999 || g_list[0][mapped_idx]->is_long_term)
				{
					if (refList == 0)
					{
						mb_cache_ptr->fw_mv_xy = ((int32 *)ref_mv_cache_direct0)[0];
					}else
					{
						mb_cache_ptr->fw_mv_xy = ((int32 *)ref_mv_cache_direct1)[0];
						((int32 *)ref_mv_cache_direct0)[0] = ((int32 *)ref_mv_cache_direct1)[0];
					}
					mb_cache_ptr->bw_mv_xy = 0;
					((int32 *)ref_mv_cache_direct1)[0] = 0;
				}else
				{
					int32 tmp1, tmp2;

					int16 *ref_mv_ptr = ((refList == 0) ? ref_mv_cache_direct0 : ref_mv_cache_direct1);

					tmp1 = (mvscale[mapped_idx] * ref_mv_ptr[0] + 128) >> 8;
					tmp2 = (mvscale[mapped_idx] * ref_mv_ptr[1] + 128) >> 8;						
					mb_cache_ptr->fw_mv_xy = ((tmp2<<16) | (tmp1 & 0xffff));
					tmp1 -= ref_mv_ptr[0];
					tmp2 -= ref_mv_ptr[1];
					mb_cache_ptr->bw_mv_xy = ((tmp2<<16) |(tmp1 & 0xffff));
					((int32 *)ref_mv_cache_direct0)[0] = mb_cache_ptr->fw_mv_xy;
					((int32 *)ref_mv_cache_direct1)[0] = mb_cache_ptr->bw_mv_xy;							
				}

				mb_cache_ptr->fw_rFrame = mapped_idx;
			}

			mb_cache_ptr->bw_rFrame = 0;		
			ref_idx_cache_direct0[0] = mb_cache_ptr->fw_rFrame;
			ref_idx_cache_direct1[0] = mb_cache_ptr->bw_rFrame;
		}
			
		ref_idx_cache_direct0 ++;
		ref_idx_cache_direct1 ++;
		ref_pic_id_cache_direct0 ++;
		ref_pic_id_cache_direct1 ++;
		ref_mv_cache_direct0 += 2;
		ref_mv_cache_direct1 += 2;
	}

	mb_cache_ptr->direct_pdir = 2;
}

PUBLIC void H264Dec_mv_prediction (DEC_IMAGE_PARAMS_T *img_ptr, DEC_MB_INFO_T *mb_info_ptr, DEC_MB_CACHE_T *mb_cache_ptr)
{  
	if (mb_cache_ptr->is_skipped || mb_info_ptr->mb_type == 0)
	{
		int8 b8pdir;
		if ( img_ptr->type == B_SLICE)
		{
			b8pdir = pred_skip_bslice (img_ptr, mb_info_ptr, mb_cache_ptr);

			if (b8pdir != 2)
			{
				H264Dec_FillNoRefList_8x8(mb_info_ptr, 0, b8pdir);
				H264Dec_FillNoRefList_8x8(mb_info_ptr, 1, b8pdir);
				H264Dec_FillNoRefList_8x8(mb_info_ptr, 2, b8pdir);
				H264Dec_FillNoRefList_8x8(mb_info_ptr, 3, b8pdir);
			}
		}else //pslice
		{
			H264Dec_mv_prediction_skipped_pslice (img_ptr, mb_info_ptr, mb_cache_ptr);
			b8pdir = 0;
		}
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

	if (img_ptr->apply_weights)
	{
		H264Dec_Config_WP_info(img_ptr, mb_info_ptr, mb_cache_ptr);
	}
	VSP_WRITE_REG_CQM(VSP_MBC_REG_BASE+MBC_CFG_OFF, img_ptr->mbc_cfg_cmd, "MBC weighted prediction ON/OFF and parameter");

	{
		uint32 cmd;

		cmd = (mb_cache_ptr->mbc_b8pdir[3]<<3) | (mb_cache_ptr->mbc_b8pdir[2]<<2) |
			(mb_cache_ptr->mbc_b8pdir[1]<<1) | (mb_cache_ptr->mbc_b8pdir[0]<<0);
		VSP_WRITE_REG_CQM(VSP_MBC_REG_BASE+MBC_H264_BIR_INFO_OFF, cmd, "MBC_H264_RND_CTRL: config h.264 mbc b8 bi- or not");
    	VSP_WRITE_CMD_INFO((VSP_MBC << CQM_SHIFT_BIT) | (2<<24) | (MBC_H264_BIR_INFO_WOFF<<8) | MBC_CFG_WOFF);
	}
	
	VSP_WRITE_REG_CQM(VSP_MCA_REG_BASE+MCA_CFG_OFF, 1, "MCA_CFG: start MCA");
	VSP_WRITE_CMD_INFO((VSP_MCA << CQM_SHIFT_BIT) | (1<<24) |MCA_CFG_WOFF);

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
