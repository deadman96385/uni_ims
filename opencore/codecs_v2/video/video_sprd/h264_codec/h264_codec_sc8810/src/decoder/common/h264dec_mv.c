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
#include "sc8810_video_header.h"
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

PUBLIC int32 decode_cavlc_mb_mvd(DEC_IMAGE_PARAMS_T *img_ptr, DEC_MB_CACHE_T *mb_cache_ptr, int32 sub_blk_id, int32 list)
{
	int32 mvd_x, mvd_y, mvd_xy;
	DEC_BS_T *stream = img_ptr->bitstrm_ptr;

	mvd_x = READ_SE_V(stream); 
	mvd_y = READ_SE_V(stream); 

#if defined(H264_BIG_ENDIAN)
	mvd_xy  = (mvd_x << 16) | (mvd_y & 0xffff);
#else
	mvd_xy  = (mvd_y << 16) | (mvd_x & 0xffff);
#endif
	return mvd_xy;
}

#define IS_DIR(pdir)	(((pdir) == 2) || (s_list == (pdir)))

LOCAL void H264Dec_read_motionAndRefId_PMB16x16 (DEC_IMAGE_PARAMS_T *img_ptr, DEC_MB_INFO_T *mb_info_ptr, DEC_MB_CACHE_T *mb_cache_ptr)
{
	DEC_STORABLE_PICTURE_T	**listX;
	int32 ref_frm_id_s32;
	int32 pdir = mb_cache_ptr->b8pdir[0];
#ifdef _NEON_OPT_
	int32x4_t v32x4;
#endif	
	
	for (s_list = 0; s_list < img_ptr->list_count; s_list++)
	{
		if(IS_DIR(pdir))
		{
			listX = g_list[s_list];
			
			if (mb_cache_ptr->read_ref_id_flag[s_list])
			{
				int32 ref_frm_id = readRefFrame (img_ptr, mb_cache_ptr, 0, s_list);

			#if _H264_PROTECT_ & _LEVEL_HIGH_	
				if (ref_frm_id >= (MAX_REF_FRAME_NUMBER+1))
			    	{
			    		img_ptr->error_flag |= ER_REF_FRM_ID;
			        	img_ptr->return_pos |= (1<<27);
					return;
			    	}
			#endif	
				ref_frm_id_s32 = ref_frm_id * 0x01010101;
			}else
			{
				 ref_frm_id_s32 = 0;
			}
		}else
		{
 			ref_frm_id_s32 = LIST_NOT_USED;
		}

		if (ref_frm_id_s32) // != 0
		{
			s_ref_idx_ptr = mb_cache_ptr->ref_idx_cache[s_list];
			((int32 *)s_ref_idx_ptr)[4] = ((int32 *)s_ref_idx_ptr)[7] = 
			((int32 *)s_ref_idx_ptr)[10] = ((int32 *)s_ref_idx_ptr)[13] = ref_frm_id_s32;
		}
	}

	for (s_list = 0; s_list < img_ptr->list_count; s_list++)
	{
		if(IS_DIR(pdir))
		{
			s_mvd_xy = decode_mvd_xy(img_ptr, mb_cache_ptr, CTX_CACHE_WIDTH_PLUS4, s_list);

			if (s_mvd_xy)
			{
				s_mvd_ptr = (int32 *)(mb_cache_ptr->mvd_cache[s_list]);

#ifndef _NEON_OPT_
				s_mvd_ptr[16] = s_mvd_ptr[17] = s_mvd_ptr[18] = s_mvd_ptr[19] =
				s_mvd_ptr[28] = s_mvd_ptr[29] = s_mvd_ptr[30] = s_mvd_ptr[31] =
				s_mvd_ptr[40] = s_mvd_ptr[41] = s_mvd_ptr[42] = s_mvd_ptr[43] =
				s_mvd_ptr[52] = s_mvd_ptr[53] = s_mvd_ptr[54] = s_mvd_ptr[55] = s_mvd_xy;
#else
				v32x4 = vmovq_n_s32 (s_mvd_xy);
				vst1q_s32 (s_mvd_ptr+16, v32x4);
				vst1q_s32 (s_mvd_ptr+28, v32x4);
				vst1q_s32 (s_mvd_ptr+40, v32x4);
				vst1q_s32 (s_mvd_ptr+52, v32x4);
#endif
			}			
		}
	}

	return;
}

LOCAL void H264Dec_read_motionAndRefId_PMB16x8 (DEC_IMAGE_PARAMS_T *img_ptr, DEC_MB_INFO_T *mb_info_ptr, DEC_MB_CACHE_T *mb_cache_ptr)
{
	DEC_STORABLE_PICTURE_T	**listX;
	int32 ref_frm_id_s32;
	uint32 b16x8;
	int32 pdir;
#ifdef _NEON_OPT_
	int32x4_t v32x4;
#endif	
	
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
					int32 ref_frm_id = readRefFrame (img_ptr, mb_cache_ptr, 2*b16x8, s_list);

				#if _H264_PROTECT_ & _LEVEL_HIGH_	
					if (ref_frm_id >= (MAX_REF_FRAME_NUMBER+1))
				    	{
				    		img_ptr->error_flag |= ER_REF_FRM_ID;
				        	img_ptr->return_pos |= (1<<28);
				                
				    		return;
				    	}
				#endif	
					ref_frm_id_s32 = ref_frm_id * 0x01010101;
				}else
				{
					 ref_frm_id_s32 = 0;
				}				
			}else
			{
				ref_frm_id_s32 = LIST_NOT_USED;
			}

			if (ref_frm_id_s32) // != 0
			{
				s_ref_idx_ptr = mb_cache_ptr->ref_idx_cache[s_list] + CTX_CACHE_WIDTH_PLUS4+ CTX_CACHE_WIDTH_X2 *b16x8;
				((int32 *)s_ref_idx_ptr)[0] = ref_frm_id_s32;	s_ref_idx_ptr += CTX_CACHE_WIDTH;
				((int32 *)s_ref_idx_ptr)[0] = ref_frm_id_s32;			
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
				s_mvd_xy = decode_mvd_xy(img_ptr, mb_cache_ptr, CTX_CACHE_WIDTH_PLUS4+b16x8*CTX_CACHE_WIDTH_X2, s_list);

				if (s_mvd_xy)
				{
					s_mvd_ptr = (int32 *)(mb_cache_ptr->mvd_cache[s_list]) + CTX_CACHE_WIDTH_PLUS4+ CTX_CACHE_WIDTH_X2 *b16x8;
				#ifndef _NEON_OPT_	
					s_mvd_ptr[0] = s_mvd_ptr[1] = s_mvd_ptr[2] = s_mvd_ptr[3] = s_mvd_xy;	s_mvd_ptr += CTX_CACHE_WIDTH;
					s_mvd_ptr[0] = s_mvd_ptr[1] = s_mvd_ptr[2] = s_mvd_ptr[3] = s_mvd_xy;
				#else
					v32x4 = vmovq_n_s32 (s_mvd_xy);
					vst1q_s32 (s_mvd_ptr+0, v32x4);
					vst1q_s32 (s_mvd_ptr+CTX_CACHE_WIDTH, v32x4);
				#endif
				}			
			}
		}
	}

	return;
}

LOCAL void H264Dec_read_motionAndRefId_PMB8x16 (DEC_IMAGE_PARAMS_T *img_ptr, DEC_MB_INFO_T *mb_info_ptr, DEC_MB_CACHE_T *mb_cache_ptr)
{
	DEC_STORABLE_PICTURE_T	**listX;
	int32 ref_frm_id_s32;
	uint32 b8x16;
	int32 pdir;
#ifdef _NEON_OPT_
	int32x2_t v32x2;
#endif	

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
					int32 ref_frm_id = readRefFrame (img_ptr, mb_cache_ptr, b8x16, s_list);
					
				#if _H264_PROTECT_ & _LEVEL_HIGH_	
					if (ref_frm_id >= (MAX_REF_FRAME_NUMBER+1))
			       		{
			        		img_ptr->error_flag |= ER_REF_FRM_ID;
			            		img_ptr->return_pos |= (1<<(29+b8x16));
			                
			    			return;
			        	}
				#endif	
					ref_frm_id_s32 = ref_frm_id * 0x0101;
				}else
				{
					 ref_frm_id_s32 = 0;
				}
			}else
			{
				ref_frm_id_s32 = LIST_NOT_USED; 
			}

			if (ref_frm_id_s32) // != 0
			{
				s_ref_idx_ptr = mb_cache_ptr->ref_idx_cache[s_list] + CTX_CACHE_WIDTH_PLUS4+ 2*b8x16;
				((int16 *)s_ref_idx_ptr)[0] = ref_frm_id_s32;	s_ref_idx_ptr += CTX_CACHE_WIDTH;
				((int16 *)s_ref_idx_ptr)[0] = ref_frm_id_s32;	s_ref_idx_ptr += CTX_CACHE_WIDTH;
				((int16 *)s_ref_idx_ptr)[0] = ref_frm_id_s32;	s_ref_idx_ptr += CTX_CACHE_WIDTH;
				((int16 *)s_ref_idx_ptr)[0] = ref_frm_id_s32;			
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
				s_mvd_xy = decode_mvd_xy(img_ptr, mb_cache_ptr, CTX_CACHE_WIDTH_PLUS4+b8x16*2, s_list);

				if (s_mvd_xy)
				{
					s_mvd_ptr = (int32 *)(mb_cache_ptr->mvd_cache[s_list]) + CTX_CACHE_WIDTH_PLUS4+ 2 *b8x16;
				#ifndef _NEON_OPT_	
					s_mvd_ptr[0] = s_mvd_ptr[1] = s_mvd_xy; s_mvd_ptr += CTX_CACHE_WIDTH;
					s_mvd_ptr[0] = s_mvd_ptr[1] = s_mvd_xy;	s_mvd_ptr += CTX_CACHE_WIDTH;
					s_mvd_ptr[0] = s_mvd_ptr[1] = s_mvd_xy;	s_mvd_ptr += CTX_CACHE_WIDTH;
					s_mvd_ptr[0] = s_mvd_ptr[1] = s_mvd_xy;	
				#else
					v32x2 = vmov_n_s32 (s_mvd_xy);
					vst1_s32 (s_mvd_ptr+0, v32x2 );
					vst1_s32 (s_mvd_ptr+CTX_CACHE_WIDTH_X1, v32x2);
					vst1_s32 (s_mvd_ptr+CTX_CACHE_WIDTH_X2, v32x2);
					vst1_s32 (s_mvd_ptr+CTX_CACHE_WIDTH_X3, v32x2);
				#endif
				}			
			}
		}
	}

	return;
}

LOCAL void H264Dec_read_motionAndRefId_P8x8 (DEC_IMAGE_PARAMS_T *img_ptr, DEC_MB_INFO_T *mb_info_ptr, DEC_MB_CACHE_T *mb_cache_ptr)
{
	DEC_STORABLE_PICTURE_T	**listX;
	int32 ref_frm_id_s32;
	uint32 b8, cache_offset;
	int32 pdir;
#ifdef _NEON_OPT_
	int32x2_t v32x2;
#endif	

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
					int32 ref_frm_id =  readRefFrame (img_ptr, mb_cache_ptr, b8, s_list);

				#if _H264_PROTECT_ & _LEVEL_HIGH_	
					 if (ref_frm_id >= (MAX_REF_FRAME_NUMBER+1))
        			 	{
            		 			img_ptr->error_flag |= ER_REF_FRM_ID;
            					img_ptr->return_pos1 |= (1<<(2+b8));

						return;
        			 	}
				#endif	 
					ref_frm_id_s32 = ref_frm_id * 0x0101;
				}else
				{
					 ref_frm_id_s32 = 0;
				}				
			}else
			{
				ref_frm_id_s32 = LIST_NOT_USED;
			}

			if (ref_frm_id_s32) // != 0
			{
				s_ref_idx_ptr = mb_cache_ptr->ref_idx_cache[s_list] + CTX_CACHE_WIDTH_PLUS4+ CTX_CACHE_WIDTH_X2*(b8>>1)+2*(b8&1);
				((int16 *)s_ref_idx_ptr)[0] = ref_frm_id_s32;	s_ref_idx_ptr += CTX_CACHE_WIDTH;
				((int16 *)s_ref_idx_ptr)[0] = ref_frm_id_s32;			
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
					s_mvd_xy = decode_mvd_xy(img_ptr, mb_cache_ptr, sub_blk_id, s_list);
			#ifndef _NEON_OPT_	
					s_mvd_ptr[0] = s_mvd_ptr[1] = s_mvd_xy;	s_mvd_ptr += CTX_CACHE_WIDTH;
					s_mvd_ptr[0] = s_mvd_ptr[1] = s_mvd_xy;	
			#else
					v32x2 = vmov_n_s32 (s_mvd_xy);
					vst1_s32 (s_mvd_ptr+0, v32x2 );
					vst1_s32 (s_mvd_ptr+CTX_CACHE_WIDTH, v32x2 );
			#endif
					
					break;
				case PMB8X8_BLOCK8X4:
					s_mvd_xy = decode_mvd_xy(img_ptr, mb_cache_ptr, sub_blk_id+0*CTX_CACHE_WIDTH, s_list);
			#ifndef _NEON_OPT_		
					s_mvd_ptr[0] = s_mvd_ptr[1] = s_mvd_xy;	s_mvd_ptr += CTX_CACHE_WIDTH;
			#else
					v32x2 = vmov_n_s32 (s_mvd_xy);
					vst1_s32 (s_mvd_ptr+0, v32x2 );
			#endif

					s_mvd_xy = decode_mvd_xy(img_ptr, mb_cache_ptr, sub_blk_id+1*CTX_CACHE_WIDTH, s_list);
			#ifndef _NEON_OPT_	
					s_mvd_ptr[0] = s_mvd_ptr[1] = s_mvd_xy;	
			#else
					v32x2 = vmov_n_s32 (s_mvd_xy);
					vst1_s32 (s_mvd_ptr+CTX_CACHE_WIDTH, v32x2 );
			#endif
					
					break;
				case PMB8X8_BLOCK4X8:
					s_mvd_xy = decode_mvd_xy(img_ptr, mb_cache_ptr, sub_blk_id+0, s_list);
			#if 1//ndef _NEON_OPT_		
					s_mvd_ptr[0] = s_mvd_ptr[CTX_CACHE_WIDTH] = s_mvd_xy;	s_mvd_ptr ++;
			#else
					v32x2 = vset_lane_s32(s_mvd_xy, v32x2, 0);
			#endif
			
					s_mvd_xy = decode_mvd_xy(img_ptr, mb_cache_ptr, sub_blk_id+1, s_list);
			#if 1//ndef _NEON_OPT_				
					s_mvd_ptr[0] = s_mvd_ptr[CTX_CACHE_WIDTH] = s_mvd_xy;		
			#else
					v32x2 = vset_lane_s32(s_mvd_xy, v32x2, 1);
					vst1_s32 (s_mvd_ptr, v32x2 );
					vst1_s32 (s_mvd_ptr + CTX_CACHE_WIDTH, v32x2 );
			#endif
						
					break;
				case PMB8X8_BLOCK4X4:

					///0
					s_mvd_xy = decode_mvd_xy(img_ptr, mb_cache_ptr, sub_blk_id, s_list);
					s_mvd_ptr[0] = s_mvd_xy;

					///1
					s_mvd_xy = decode_mvd_xy(img_ptr, mb_cache_ptr, sub_blk_id+1, s_list);
					s_mvd_ptr[1] = s_mvd_xy;
					
					sub_blk_id += CTX_CACHE_WIDTH;
					s_mvd_ptr += CTX_CACHE_WIDTH;

					///2
					s_mvd_xy = decode_mvd_xy(img_ptr, mb_cache_ptr, sub_blk_id, s_list);
					s_mvd_ptr[0] = s_mvd_xy;

					///3
					s_mvd_xy = decode_mvd_xy(img_ptr, mb_cache_ptr, sub_blk_id+1, s_list);
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

PUBLIC int32 H264Dec_pred_motion (int8 *ref_idx_ptr, int16 *ref_mv_ptr, int32 blk_width)
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

PUBLIC int32 H264Dec_pred16x8_motion (DEC_MB_CACHE_T *mb_cache_ptr, int32 blkIdx, int32 list)
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

PUBLIC int32 H264Dec_pred8x16_motion (DEC_MB_CACHE_T *mb_cache_ptr, int32 blkIdx, int32 list)
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

//not reference
PUBLIC void H264Dec_FillNoRefList_8x8(DEC_MB_INFO_T *mb_info_ptr, uint32 b8, int32 b8pdir)
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

PUBLIC void H264Dec_mv_prediction_P8x8 (DEC_IMAGE_PARAMS_T *img_ptr, DEC_MB_INFO_T *mb_info_ptr, DEC_MB_CACHE_T *mb_cache_ptr)
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
#if 0	
	if (b8mode != 0)
	{
		b8pdir = (*img_ptr->b8_mv_pred_func[b8mode-4])(mb_info_ptr, mb_cache_ptr, cache_offset, 0);
	}else
	{
		b8pdir = MC8x8_direct(mb_info_ptr, mb_cache_ptr, 0,  cache_offset);
	}
#else
	b8pdir = (*img_ptr->b8_mv_pred_func[b8mode])(mb_info_ptr, mb_cache_ptr, cache_offset, 0);
#endif
	
	if ((img_ptr->type == B_SLICE) && (b8pdir != 2))
	{
		H264Dec_FillNoRefList_8x8(mb_info_ptr, 0, b8pdir);
	}

	//8x8 1
	cache_offset = CTX_CACHE_WIDTH_PLUS4+2;
	ref_idx_ptr[0][2] = ref_idx_b8_1[0];
	ref_idx_ptr[1][2] = ref_idx_b8_1[1];
	b8mode = mb_cache_ptr->b8mode[1];
#if 0	
	if (b8mode != 0)
	{
		b8pdir = (*img_ptr->b8_mv_pred_func[b8mode-4])(mb_info_ptr, mb_cache_ptr, cache_offset, 1);
	}else
	{
		b8pdir = MC8x8_direct(mb_info_ptr, mb_cache_ptr, 1,  cache_offset);
	}
#else
	b8pdir = (*img_ptr->b8_mv_pred_func[b8mode])(mb_info_ptr, mb_cache_ptr, cache_offset, 1);
#endif
	
	if ((img_ptr->type == B_SLICE) && (b8pdir != 2))
	{
		H264Dec_FillNoRefList_8x8(mb_info_ptr, 1, b8pdir);
	}
	
	//8x8 2
	cache_offset = CTX_CACHE_WIDTH_PLUS4 + CTX_CACHE_WIDTH_X2;
	b8mode = mb_cache_ptr->b8mode[2];
#if 0	
	if (b8mode != 0)
	{		
		b8pdir = (*img_ptr->b8_mv_pred_func[b8mode-4])(mb_info_ptr, mb_cache_ptr, cache_offset, 2);
	}else
	{
		b8pdir = MC8x8_direct(mb_info_ptr, mb_cache_ptr, 2,  cache_offset);
	}
#else
	b8pdir = (*img_ptr->b8_mv_pred_func[b8mode])(mb_info_ptr, mb_cache_ptr, cache_offset, 2);
#endif
	
	if ((img_ptr->type == B_SLICE) && (b8pdir != 2))
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
#if 0	
	if (b8mode != 0)
	{
		b8pdir = (*img_ptr->b8_mv_pred_func[b8mode-4])(mb_info_ptr, mb_cache_ptr, cache_offset, 3);
	}else
	{
		b8pdir = MC8x8_direct(mb_info_ptr, mb_cache_ptr, 3,  cache_offset);
	}
#else
	b8pdir = (*img_ptr->b8_mv_pred_func[b8mode])(mb_info_ptr, mb_cache_ptr, cache_offset, 3);
#endif
	
	if ((img_ptr->type == B_SLICE) && (b8pdir != 2))
	{
		H264Dec_FillNoRefList_8x8(mb_info_ptr, 3, b8pdir);
	}

	return;
}

//int mvscale[MAX_REF_FRAME_NUMBER];

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
	int32 /*i,*/ offset, blk4x4Idx;
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
	
#if 0	//moved to header
	for (i = 0; i < iref_max; i++)
	{
		int32 prescale, iTRb, iTRp;

		iTRb = Clip3( -128, 127, (g_dec_picture_ptr->poc - g_list[0][i]->poc ));
		iTRp = Clip3( -128, 127,  (g_list[1][0]->poc - g_list[0][i]->poc));

		if (iTRp!=0)
		{
			prescale = ( 16384 + ABS( iTRp / 2 ) ) / iTRp;
			img_ptr->dist_scale_factor[i] = Clip3( -1024, 1023, ( iTRb * prescale + 32 ) >> 6 ) ;
		}else
		{
		  	img_ptr->dist_scale_factor[i] = 9999;
		}
	}
#endif

	//////////////lsw for direct mode (temporal direct mode)
	ref_idx_cache_direct0 = mb_cache_ptr->ref_idx_cache_direct[0];
	ref_idx_cache_direct1 = mb_cache_ptr->ref_idx_cache_direct[1];
	ref_pic_id_cache_direct0 = mb_cache_ptr->ref_pic_id_cache_direct[0];
	ref_pic_id_cache_direct1 = mb_cache_ptr->ref_pic_id_cache_direct[1];
	ref_mv_cache_direct0 = mb_cache_ptr->mv_cache_direct[0];
	ref_mv_cache_direct1 = mb_cache_ptr->mv_cache_direct[1];

	for(blk4x4_id = 0; blk4x4_id < 16; blk4x4_id++)
	{
		int32 b8 = g_b8map[blk4x4_id] ;//((j>>1)<<1) + (i>>1);
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

				if (img_ptr->dist_scale_factor[mapped_idx] == 9999 || g_list[0][mapped_idx]->is_long_term)
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

					tmp1 = (img_ptr->dist_scale_factor[mapped_idx] * ref_mv_ptr[0] + 128) >> 8;
					tmp2 = (img_ptr->dist_scale_factor[mapped_idx] * ref_mv_ptr[1] + 128) >> 8;						
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
