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
#include "sc8825_video_header.h"
/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    extern   "C" 
    {
#endif

#define PITCH	CONTEXT_CACHE_WIDTH

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
#define IS_DIR(pdir)	(((pdir) == 2) || (s_list == (pdir)))

//weighted prediction
typedef struct WP_info_tag
{
	uint8 *b8map_ptr;
	uint8 *clip_data_ptr;
	int8 *b8pdir_ptr;
	int16 *g_wp_offset_ptr;
	int16 *g_wbp_weight_ptr;
	int16 *g_wp_weight_ptr;
}DEC_WP_MEM_INFO_T;

LOCAL void H264Dec_Config_WP_info_sw(DEC_IMAGE_PARAMS_T *img_ptr, DEC_MB_INFO_T *mb_info_ptr, DEC_MB_CACHE_T *mb_cache_ptr)
{
#ifdef WIN32
	uint32 row, col;
	int32 blk4x4_id;	
	int32 b8;
  	int32 comp;
	int32 pdir;
	int32 offset[2];
	int32 weight[2];
	int32 mc_ref_idx[2];
	int8 *ref_idx_ptr0;
	int8 *ref_idx_ptr1;
	int32 pitch;
	int32 shift;
	int32 blk_size;
	int32 pix_offset;
	uint8 * src_ptr[2];
	int i, j;
	int32 tmp, fw, bw;

	blk4x4_id = 0;

	for (comp = 0; comp < 3; comp++)
	{
		blk4x4_id = 0;
		ref_idx_ptr0 = mb_cache_ptr->ref_idx_cache[0] + CTX_CACHE_WIDTH_PLUS4;
		ref_idx_ptr1 = mb_cache_ptr->ref_idx_cache[1] + CTX_CACHE_WIDTH_PLUS4;
		pitch = (comp == 0) ? MB_SIZE : MB_CHROMA_SIZE;
		shift = (comp == 0) ? img_ptr->luma_log2_weight_denom : img_ptr->chroma_log2_weight_denom;
		blk_size = (comp == 0) ? 4 : 2;
		
		for (row = 0; row < 4; row++)
		{
			for (col = 0; col < 4; col++)
			{
				b8 = g_b8map[blk4x4_id];
				pdir = mb_cache_ptr->b8pdir[b8];
				mc_ref_idx[0] = ref_idx_ptr0[col]; 
				mc_ref_idx[1] = ref_idx_ptr1[col];

				if (0 == comp) //y
				{
					pix_offset = 64 * row + 4 * col;
					src_ptr[0] = mb_cache_ptr->pred_cache[0].pred_Y + pix_offset;
					src_ptr[1] = mb_cache_ptr->pred_cache[1].pred_Y + pix_offset;
				}else //uv
				{
					pix_offset = 16 * row + 2 * col;
					src_ptr[0] = mb_cache_ptr->pred_cache[0].pred_UV[comp -1] + pix_offset;
					src_ptr[1] = mb_cache_ptr->pred_cache[1].pred_UV[comp -1] + pix_offset;
				}
				
				if (2 == pdir) //bi-dir
				{
					offset[0] = g_wp_offset[0][mc_ref_idx[0]][comp];
					offset[1] = g_wp_offset[1][mc_ref_idx[1]][comp];
					weight[0] = g_wbp_weight[0][mc_ref_idx[0]][mc_ref_idx[1]][comp];
					weight[1] = g_wbp_weight[1][mc_ref_idx[0]][mc_ref_idx[1]][comp];

					for (i = 0; i < blk_size; i++)
					{
						for (j = 0; j < blk_size; j++)
						{
							fw = src_ptr[0][j];
							bw = src_ptr[1][j];

							tmp = ((fw*weight[0] + bw*weight[1] + (1<<shift)) >>(shift+1)) + ((offset[0] + offset[1] +1 )>>1);
							src_ptr[0][j] = IClip(0, 255, tmp);
						}
						src_ptr[0] += pitch;
						src_ptr[1] += pitch;
					}
				}else //list 0 or list 1
				{
					offset[pdir] = g_wp_offset[pdir][mc_ref_idx[pdir]][comp];
					weight[pdir] = g_wp_weight[pdir][mc_ref_idx[pdir]][comp];

					for (i = 0; i < blk_size; i++)
					{
						for (j = 0; j < blk_size; j++)
						{
							tmp = src_ptr[pdir][j];
							if (shift >= 1)
							{
								tmp = ((tmp * weight[pdir] + (1<<(shift-1))) >> shift) + offset[pdir];
							}else
							{
								tmp = tmp * weight[pdir] + offset[pdir];
							}
							src_ptr[0][j] = IClip(0, 255, tmp);
						}
						src_ptr[0] += pitch;
						src_ptr[1] += pitch;
					}
				}
				
				blk4x4_id++;
			}
			
			ref_idx_ptr0 += CTX_CACHE_WIDTH;
			ref_idx_ptr1 += CTX_CACHE_WIDTH;	
		}
	}
#else
	int32 shift;
	int8 *ref_idx_ptr0;
	uint8 *src_ptr0;
	DEC_WP_MEM_INFO_T mem_t;

	shift = img_ptr->luma_log2_weight_denom;
	ref_idx_ptr0 = mb_cache_ptr->ref_idx_cache[0] + CTX_CACHE_WIDTH_PLUS4;
	mem_t.b8map_ptr = (uint8 *)g_b8map;
//	mem_t.clip_data_ptr = g_clip_ptr 
	mem_t.b8pdir_ptr = mb_cache_ptr->b8pdir;
	mem_t.g_wp_offset_ptr = (int16 *)g_wp_offset;
	mem_t.g_wbp_weight_ptr = (int16 *)g_wbp_weight;
	mem_t.g_wp_weight_ptr = (int16 *)g_wp_weight;

	//y component
	src_ptr0 = mb_cache_ptr->pred_cache[0].pred_Y;
	H264Dec_wp_y(ref_idx_ptr0, src_ptr0, shift, &mem_t);

	//u component
	shift = img_ptr->chroma_log2_weight_denom;
	src_ptr0 = mb_cache_ptr->pred_cache[0].pred_UV[0];
	mem_t.g_wp_offset_ptr += 1;
	mem_t.g_wbp_weight_ptr += 1;
	mem_t.g_wp_weight_ptr += 1;
	H264Dec_wp_c(ref_idx_ptr0, src_ptr0, shift, &mem_t);

	//v component
	src_ptr0 = mb_cache_ptr->pred_cache[0].pred_UV[1];
	mem_t.g_wp_offset_ptr += 1;
	mem_t.g_wbp_weight_ptr += 1;
	mem_t.g_wp_weight_ptr += 1;
	H264Dec_wp_c(ref_idx_ptr0, src_ptr0, shift, &mem_t);
#endif
}

//for one 8x8 block in one MB
void H264Dec_Config_8x8MC_info_sw(DEC_IMAGE_PARAMS_T *img_ptr, DEC_MB_CACHE_T *mb_cache_ptr, int32 b8, int32 b8pdir)
{
	uint32 b8_cache_offset;
	int32 mv_x, mv_y;
	int32 mv_xy;
	uint8 **pRefFrame;
	DEC_STORABLE_PICTURE_T ** list;
	
	b8_cache_offset = CTX_CACHE_WIDTH_PLUS4 + g_b8_offset[b8];
	
	s_dir = 0;
	do
	{
		s_list = (b8pdir == 2) ? s_dir : b8pdir;
		s_ref_idx_ptr = mb_cache_ptr->ref_idx_cache[s_list]+b8_cache_offset;
		s_ref_mv_ptr = (int32*)(mb_cache_ptr->mv_cache[s_list]) + b8_cache_offset;
		
		mv_xy = s_ref_mv_ptr[0];
		mv_x = ((mv_xy << 16) >> 16);
		mv_y = (mv_xy >> 16);

		list = g_list[s_list];
		pRefFrame = list[s_ref_idx_ptr[0]]->imgYUV;
#if defined(CTS_PROTECT)
		if ((pRefFrame == NULL) || (pRefFrame[0] == NULL) ||(pRefFrame[1] == NULL) )
		{
			img_ptr->error_flag |= ER_REF_FRM_ID;
			img_ptr->return_pos2 |= (1<<15);
			return;
		}
#endif
		H264Dec_mc_8x8 (img_ptr, mb_cache_ptr->pred_cache[s_list].pred_Y, pRefFrame, mv_x, mv_y, b8);
					
		s_dir++;
	}while(s_dir < b8pdir);

	return;
}

//for four 4x4 subblock in one 8x8 block
void H264Dec_Config_4x4MC_info_sw(DEC_IMAGE_PARAMS_T *img_ptr, DEC_MB_CACHE_T *mb_cache_ptr, int32 b8, int32 b8pdir)
{
	uint32 b4;
	uint32 b8_cache_offset;
	uint32 b4_cache_offset;
	int32 mv_x, mv_y;
	int32 mv_xy;
	uint8 ** pRefFrame;
	DEC_STORABLE_PICTURE_T ** list;

	b8_cache_offset = CTX_CACHE_WIDTH_PLUS4 + g_b8_offset[b8];
	for(b4 = 0; b4 < 4; b4++)
	{
		b4_cache_offset = b8_cache_offset + g_b4_offset[b4];
		
		s_dir = 0;
		
		do
		{
			s_list = (b8pdir == 2) ? s_dir : b8pdir;
			s_ref_idx_ptr = mb_cache_ptr->ref_idx_cache[s_list]+b4_cache_offset;
			s_ref_mv_ptr = (int32*)(mb_cache_ptr->mv_cache[s_list]) + b4_cache_offset;
		
			mv_xy = s_ref_mv_ptr[0];
			mv_x = ((mv_xy << 16) >> 16);
			mv_y = (mv_xy >> 16);

			list = g_list[s_list];
			pRefFrame = list[s_ref_idx_ptr[0]]->imgYUV;
#if defined(CTS_PROTECT)
			if ((pRefFrame == NULL) || (pRefFrame[0] == NULL) ||(pRefFrame[1] == NULL) )
			{
				img_ptr->error_flag |= ER_REF_FRM_ID;
				img_ptr->return_pos2 |= (1<<16);
				return;
			}
#endif

			H264Dec_mc_4x4 (img_ptr, mb_cache_ptr->pred_cache[s_list].pred_Y, pRefFrame, mv_x, mv_y, b8, b4);

			s_dir++;
		}while(s_dir < b8pdir);	
	}
			
	return;
}

PUBLIC int8 H264Dec_mv_prediction_skipped_bslice_spatial_sw (DEC_IMAGE_PARAMS_T *img_ptr, DEC_MB_INFO_T *mb_info_ptr, DEC_MB_CACHE_T *mb_cache_ptr)
{
	int32 mv_xy;
	int32 use_8x8mc;
	int32 use_4x4mc[4] = {0,0,0,0};
	int32 b8;
	uint32 row;
	int32 b8pdir = 0;
	int32 fw_rFrame,bw_rFrame;

	uint32 cache_offset;
	int32 mv_x, mv_y;
	uint8 ** pRefFrame;
	DEC_STORABLE_PICTURE_T ** list;
	
	s_dir = 0;

	fw_rFrame = mb_cache_ptr->fw_rFrame;
	bw_rFrame = mb_cache_ptr->bw_rFrame;

	use_8x8mc = 0;
	if (fw_rFrame >= 0)
	{
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
		}		
	}

	//save ref_idx to mb_cache.
	if (fw_rFrame >= 0)
	{
		uint32 ref_u32 = fw_rFrame * 0x01010101;
		
		s_ref_idx_ptr = mb_cache_ptr->ref_idx_cache[0];
		((uint32 *)s_ref_idx_ptr)[4] =  
		((uint32 *)s_ref_idx_ptr)[7] =  
		((uint32 *)s_ref_idx_ptr)[10] =  
		((uint32 *)s_ref_idx_ptr)[13] = ref_u32; 
	}

	if (bw_rFrame >= 0)
	{
		uint32 ref_u32 = bw_rFrame * 0x01010101;
				
		s_ref_idx_ptr = mb_cache_ptr->ref_idx_cache[1];
		((uint32 *)s_ref_idx_ptr)[4] =  
		((uint32 *)s_ref_idx_ptr)[7] =  
		((uint32 *)s_ref_idx_ptr)[10] =  
		((uint32 *)s_ref_idx_ptr)[13] = ref_u32; 		
	}
	
	b8pdir = (s_dir == 2) ? 2 : b8pdir;
	((int32 *)mb_cache_ptr->b8pdir)[0] = b8pdir * 0x01010101;
	
	//configure mv command
	if (!use_8x8mc)
	{
		s_dir = 0;
		do
		{
			s_list = (b8pdir == 2) ? s_dir : b8pdir;
			cache_offset = CTX_CACHE_WIDTH_PLUS4;
			s_ref_idx_ptr = mb_cache_ptr->ref_idx_cache[s_list]+cache_offset;
			s_ref_mv_ptr = (int32*)(mb_cache_ptr->mv_cache[s_list]) + cache_offset;
			mv_xy = s_ref_mv_ptr[0];
			
			list = g_list[s_list];
			pRefFrame = list[s_ref_idx_ptr[0]]->imgYUV;
#if defined(CTS_PROTECT)
			if ((pRefFrame == NULL) || (pRefFrame[0] == NULL) ||(pRefFrame[1] == NULL) )
			{
				img_ptr->error_flag |= ER_REF_FRM_ID;
				img_ptr->return_pos2 |= (1<<17);
				return 0;
			}
#endif					
			mv_x = ((mv_xy << 16) >> 16);
			mv_y = (mv_xy >> 16);
			H264Dec_mc_16x16 (img_ptr, mb_cache_ptr->pred_cache[s_list].pred_Y, pRefFrame, mv_x, mv_y);
						
			s_dir++;
		}while(s_dir < b8pdir);	
	}else
	{
		for (b8 = 0; b8 < 4; b8++)
		{
			if (use_4x4mc[b8])
			{
				H264Dec_Config_4x4MC_info_sw(img_ptr, mb_cache_ptr, b8, b8pdir);
			}else
			{		
				H264Dec_Config_8x8MC_info_sw(img_ptr, mb_cache_ptr, b8, b8pdir);	
			}
		}
	}

	return b8pdir;
}

PUBLIC int8 H264Dec_mv_prediction_skipped_bslice_temporal_sw (DEC_IMAGE_PARAMS_T *img_ptr, DEC_MB_INFO_T *mb_info_ptr, DEC_MB_CACHE_T *mb_cache_ptr)
{
	int32 use_8x8mc;
	int32 use_4x4mc[4] = {0, 0, 0, 0};
	int32 b4, offset_b4;
	int32 b8;
	int32 b8pdir;
	int32 offset;
	int8  *ref_idx_ptr0, *ref_idx_ptr1;
	int32 *ref_mv_ptr0, *ref_mv_ptr1;

	int32 mv_xy;
	uint32 cache_offset;
	int32 mv_x, mv_y;
	uint8 ** pRefFrame;
	DEC_STORABLE_PICTURE_T ** list;


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

	ref_idx_ptr0 = mb_cache_ptr->ref_idx_cache_direct[0];
	ref_idx_ptr1 = mb_cache_ptr->ref_idx_cache_direct[1];
	ref_mv_ptr0 = (int32*)(mb_cache_ptr->mv_cache_direct[0]);
	ref_mv_ptr1 = (int32*)(mb_cache_ptr->mv_cache_direct[1]);

	for (b8=0; b8 < 4; b8++)
	{
		offset = ((b8>>1)<<3)+((b8&1)<<1);

		if (g_active_sps_ptr->direct_8x8_inference_flag == 0)
		{
			for (b4=1 ; b4 < 4; b4++)
			{
				offset_b4 = ((b4>>1)<<2)+(b4&1);

				if(ref_idx_ptr0[offset+offset_b4]!=ref_idx_ptr0[offset]
					|| ref_idx_ptr1[offset+offset_b4]!=ref_idx_ptr1[offset]
					|| ref_mv_ptr0[offset+offset_b4]!= ref_mv_ptr0[offset]
					|| ref_mv_ptr1[offset+offset_b4]!= ref_mv_ptr1[offset])
				{
					use_4x4mc[b8] = 1;
					use_8x8mc = 1;
					break;
				}
			}			
		}
		
		if (use_4x4mc[b8] == 0 && b8 != 0)
		{
			if(ref_idx_ptr0[0]!=ref_idx_ptr0[offset]
				|| ref_idx_ptr1[0]!=ref_idx_ptr1[offset]
				|| ref_mv_ptr0[0]!= ref_mv_ptr0[offset]
				|| ref_mv_ptr1[0]!= ref_mv_ptr1[offset])
			{
				use_8x8mc = 1;
			}
		}
	}

	s_dir = 2;
	b8pdir = 2;
	((int32 *)mb_cache_ptr->b8pdir)[0] = b8pdir * 0x01010101;

	//configure mv command
	if (!use_8x8mc)
	{
		s_dir = 0;
		do
		{
			s_list = (b8pdir == 2) ? s_dir : b8pdir;
			cache_offset = CTX_CACHE_WIDTH_PLUS4;
			s_ref_idx_ptr = mb_cache_ptr->ref_idx_cache[s_list]+cache_offset;
			s_ref_mv_ptr = (int32*)(mb_cache_ptr->mv_cache[s_list]) + cache_offset;
			mv_xy = s_ref_mv_ptr[0];
			
			mv_x = ((mv_xy << 16) >> 16);
			mv_y = (mv_xy >> 16);
		
			list = g_list[s_list];
			pRefFrame = list[s_ref_idx_ptr[0]]->imgYUV;
#if defined(CTS_PROTECT)
		if ((pRefFrame == NULL) || (pRefFrame[0] == NULL) ||(pRefFrame[1] == NULL) )
		{
			img_ptr->error_flag |= ER_REF_FRM_ID;
			img_ptr->return_pos2 |= (1<<18);
			return 0;
		}
#endif			
			H264Dec_mc_16x16 (img_ptr, mb_cache_ptr->pred_cache[s_list].pred_Y, pRefFrame, mv_x, mv_y);
						
			s_dir++;
		}while(s_dir < b8pdir);		
	}else
	{
		for (b8 = 0; b8 < 4; b8++)
		{		
			if (use_4x4mc[b8])
			{
				H264Dec_Config_4x4MC_info_sw(img_ptr, mb_cache_ptr, b8, b8pdir);
			}else
			{		
				H264Dec_Config_8x8MC_info_sw(img_ptr, mb_cache_ptr, b8, b8pdir);	
			}
		}
	}

	return b8pdir;
}

//pred_mv = mv_y<<16 | mv_x
LOCAL void H264Dec_mv_prediction_skipped_pslice_sw (DEC_IMAGE_PARAMS_T *img_ptr, DEC_MB_INFO_T *mb_info_ptr, DEC_MB_CACHE_T *mb_cache_ptr)
{
	int32 pred_mv;
	int32 mv_xy;
	int32 mv_x, mv_y;
	int32 top_ref_idx, left_ref_idx;
	uint8 ** pRefFrame;
	DEC_STORABLE_PICTURE_T ** list = g_list[0];
#ifdef _NEON_OPT_
	int32x4_t v32x4;
#endif
	
	s_ref_idx_ptr = mb_cache_ptr->ref_idx_cache[0] + CTX_CACHE_WIDTH_PLUS4;
	s_ref_mv_ptr = (int32 *)(mb_cache_ptr->mv_cache[0]) + CTX_CACHE_WIDTH_PLUS4;
	top_ref_idx = s_ref_idx_ptr[-CTX_CACHE_WIDTH];
	left_ref_idx = s_ref_idx_ptr[-1];

	if ((top_ref_idx == PART_NOT_AVAIL) || (left_ref_idx == PART_NOT_AVAIL) ||
		((top_ref_idx == 0) && (s_ref_mv_ptr[-CTX_CACHE_WIDTH] == 0)) ||
		((left_ref_idx == 0) && (s_ref_mv_ptr[-1] == 0)))
	{
		mv_xy = 0;
	}else
	{
		pred_mv = H264Dec_pred_motion(s_ref_idx_ptr, (int16 *)s_ref_mv_ptr, 4);
			
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
#ifndef _NEON_OPT_	
		s_ref_mv_ptr[0] = s_ref_mv_ptr[1] = s_ref_mv_ptr[2] = s_ref_mv_ptr[3] = mv_xy; s_ref_mv_ptr += CTX_CACHE_WIDTH;
		s_ref_mv_ptr[0] = s_ref_mv_ptr[1] = s_ref_mv_ptr[2] = s_ref_mv_ptr[3] = mv_xy; s_ref_mv_ptr += CTX_CACHE_WIDTH;
		s_ref_mv_ptr[0] = s_ref_mv_ptr[1] = s_ref_mv_ptr[2] = s_ref_mv_ptr[3] = mv_xy; s_ref_mv_ptr += CTX_CACHE_WIDTH;
		s_ref_mv_ptr[0] = s_ref_mv_ptr[1] = s_ref_mv_ptr[2] = s_ref_mv_ptr[3] = mv_xy;  
#else
		v32x4 = vmovq_n_s32 (mv_xy);
		vst1q_s32 (s_ref_mv_ptr+0, v32x4);
		vst1q_s32 (s_ref_mv_ptr+CTX_CACHE_WIDTH_X1, v32x4);
		vst1q_s32 (s_ref_mv_ptr+CTX_CACHE_WIDTH_X2, v32x4);
		vst1q_s32 (s_ref_mv_ptr+CTX_CACHE_WIDTH_X3, v32x4);
#endif
	}
	
	mv_x = ((mv_xy << 16) >> 16);
	mv_y = (mv_xy >> 16);
	
	pRefFrame = list[0]->imgYUV;
#if defined(CTS_PROTECT)
	if ((pRefFrame == NULL) || (pRefFrame[0] == NULL) ||(pRefFrame[1] == NULL) )
	{
		img_ptr->error_flag |= ER_REF_FRM_ID;
		img_ptr->return_pos2 |= (1<<19);
		return;
	}
#endif
	
	H264Dec_mc_16x16 (img_ptr, mb_cache_ptr->pred_cache[0].pred_Y, pRefFrame, mv_x, mv_y);

	return;
}

LOCAL void H264Dec_mv_prediction_PMB16x16_sw (DEC_IMAGE_PARAMS_T *img_ptr, DEC_MB_INFO_T *mb_info_ptr, DEC_MB_CACHE_T *mb_cache_ptr)
{
	int32 pred_mv_x, pred_mv_y, pred_mv_xy;
	int32 mvd_x, mvd_y;
	int32 mv_x, mv_y;
	int32 mv_xy;
	uint32 row;
	uint32 cache_offset;
	int32 pdir = mb_cache_ptr->b8pdir[0];
	uint8 ** pRefFrame;
	DEC_STORABLE_PICTURE_T ** list;
#ifdef _NEON_OPT_
	int32x4_t v32x4;
#endif

	s_dir = 0;
	do
	{
		s_list = (pdir == 2) ? s_dir : pdir;
		cache_offset = CTX_CACHE_WIDTH_PLUS4;
		s_ref_idx_ptr = mb_cache_ptr->ref_idx_cache[s_list] + cache_offset;
		s_ref_mv_ptr = (int32 *)(mb_cache_ptr->mv_cache[s_list]) + cache_offset;
		s_mvd_ptr = (int32 *)(mb_cache_ptr->mvd_cache[s_list]) + cache_offset;

		pred_mv_xy = H264Dec_pred_motion (s_ref_idx_ptr, (int16 *)s_ref_mv_ptr, 4);
		pred_mv_x = ((pred_mv_xy << 16) >> 16);
		pred_mv_y = (pred_mv_xy >> 16);

		s_mvd_xy = s_mvd_ptr[0];
		mvd_x = ((s_mvd_xy << 16) >> 16);
		mvd_y = (s_mvd_xy >> 16);

		mv_x = mvd_x + pred_mv_x;
		mv_y = mvd_y + pred_mv_y;

	#if defined(H264_BIG_ENDIAN)
		mv_xy  = (mv_x << 16) | (mv_y & 0xffff);
	#else
		mv_xy  = (mv_y << 16) | (mv_x & 0xffff);
	#endif

		//set mv cache
	#ifndef _NEON_OPT_	
		for (row = 4; row > 0; row--)
		{
			s_ref_mv_ptr[0] = s_ref_mv_ptr[1] =
			s_ref_mv_ptr[2] = s_ref_mv_ptr[3] = mv_xy;

			s_ref_mv_ptr += CTX_CACHE_WIDTH;
		}
	#else	
		v32x4 = vmovq_n_s32 (mv_xy);
		vst1q_s32 (s_ref_mv_ptr+0, v32x4);
		vst1q_s32 (s_ref_mv_ptr+CTX_CACHE_WIDTH_X1, v32x4);
		vst1q_s32 (s_ref_mv_ptr+CTX_CACHE_WIDTH_X2, v32x4);
		vst1q_s32 (s_ref_mv_ptr+CTX_CACHE_WIDTH_X3, v32x4);
	#endif

		/*motion compensation*/
		list = g_list[s_list];
		pRefFrame = list[s_ref_idx_ptr[0]]->imgYUV;
#if defined(CTS_PROTECT)
		if ((pRefFrame == NULL) || (pRefFrame[0] == NULL) ||(pRefFrame[1] == NULL) )
		{
			img_ptr->error_flag |= ER_REF_FRM_ID;
			img_ptr->return_pos2 |= (1<<20);
			return;
		}
#endif
		
		H264Dec_mc_16x16 (img_ptr, mb_cache_ptr->pred_cache[s_list].pred_Y, pRefFrame, mv_x, mv_y);

		s_dir++;
	}while (s_dir < pdir);
	
	return;	
}

LOCAL void H264Dec_mv_prediction_PMB16x8_sw (DEC_IMAGE_PARAMS_T *img_ptr, DEC_MB_INFO_T *mb_info_ptr, DEC_MB_CACHE_T *mb_cache_ptr)
{
	int32 pred_mv_x, pred_mv_y, pred_mv_xy;
	int32 mvd_x, mvd_y;
	int32 mv_x, mv_y;
	int32 mv_xy;
	uint32 row, b16x8;
	uint32 cache_offset;
	int32 pdir;
	uint8 **pRefFrame;
	DEC_STORABLE_PICTURE_T ** list;
#ifdef _NEON_OPT_
	int32x4_t v32x4;
#endif

	for (b16x8 = 0; b16x8 < 2; b16x8++)
	{
		pdir = mb_cache_ptr->b8pdir[2*b16x8];

		s_dir = 0;
		do 
		{
			s_list = (pdir == 2) ? s_dir : pdir;
			cache_offset = CTX_CACHE_WIDTH_PLUS4 + CTX_CACHE_WIDTH_X2 *b16x8;
			s_ref_idx_ptr = mb_cache_ptr->ref_idx_cache[s_list] + cache_offset;
			s_ref_mv_ptr = (int32 *)(mb_cache_ptr->mv_cache[s_list]) + cache_offset;
			s_mvd_ptr = (int32 *)(mb_cache_ptr->mvd_cache[s_list]) + cache_offset;

			pred_mv_xy = H264Dec_pred16x8_motion (mb_cache_ptr, cache_offset, s_list);
			pred_mv_x = ((pred_mv_xy << 16) >> 16);
			pred_mv_y = (pred_mv_xy >> 16);

			s_mvd_xy = s_mvd_ptr[0];
			mvd_x = ((s_mvd_xy << 16) >> 16);
			mvd_y = (s_mvd_xy >> 16);

			mv_x = mvd_x + pred_mv_x;
			mv_y = mvd_y + pred_mv_y;

		#if defined(H264_BIG_ENDIAN)
			mv_xy  = (mv_x << 16) | (mv_y & 0xffff);
		#else
			mv_xy  = (mv_y << 16) | (mv_x & 0xffff);
		#endif

			//set mv cache
		#ifndef _NEON_OPT_
			for (row = 2; row > 0; row--)
			{
				s_ref_mv_ptr[0] = s_ref_mv_ptr[1] = 
				s_ref_mv_ptr[2] = s_ref_mv_ptr[3] = mv_xy;

				s_ref_mv_ptr += CTX_CACHE_WIDTH;
			}		
		#else
			v32x4 = vmovq_n_s32 (mv_xy);
			vst1q_s32 (s_ref_mv_ptr+0, v32x4);
			vst1q_s32 (s_ref_mv_ptr+CTX_CACHE_WIDTH_X1, v32x4);
		#endif
		
	//		mv_x = ((mv_xy << 16) >> 16);
	//		mv_y = (mv_xy >> 16);

			list = g_list[s_list];
			pRefFrame = list[s_ref_idx_ptr[0]]->imgYUV;
#if defined(CTS_PROTECT)
		if ((pRefFrame == NULL) || (pRefFrame[0] == NULL) ||(pRefFrame[1] == NULL) )
		{
			img_ptr->error_flag |= ER_REF_FRM_ID;
			img_ptr->return_pos2 |= (1<<21);
			return;
		}
#endif
			
			H264Dec_mc_16x8 (img_ptr, mb_cache_ptr->pred_cache[s_list].pred_Y, pRefFrame, mv_x, mv_y, b16x8);

			s_dir++;
		}while(s_dir < pdir);
	}
	
	return;	
}

LOCAL void H264Dec_mv_prediction_PMB8x16_sw (DEC_IMAGE_PARAMS_T *img_ptr, DEC_MB_INFO_T *mb_info_ptr, DEC_MB_CACHE_T *mb_cache_ptr)
{
	int32 pred_mv_x, pred_mv_y, pred_mv_xy;
	int32 mvd_x, mvd_y;
	int32 mv_x, mv_y;
	int32 mv_xy;
	uint32 row, b8x16;
	uint32 cache_offset;
	int32 pdir = mb_cache_ptr->b8pdir[0];
	uint8 **pRefFrame;
	DEC_STORABLE_PICTURE_T ** list;
#ifdef _NEON_OPT_
	int32x2_t v32x2;
#endif	

	for (b8x16 = 0; b8x16 < 2; b8x16++)
	{
		pdir = mb_cache_ptr->b8pdir[b8x16];

		s_dir = 0;
		do
		{
			s_list = (pdir == 2) ? s_dir : pdir;
			cache_offset = CTX_CACHE_WIDTH_PLUS4 + 2*b8x16;
			s_ref_idx_ptr = mb_cache_ptr->ref_idx_cache[s_list] + cache_offset;
			s_ref_mv_ptr = (int32 *)(mb_cache_ptr->mv_cache[s_list]) + cache_offset;
			s_mvd_ptr = (int32 *)(mb_cache_ptr->mvd_cache[s_list]) + cache_offset;

			pred_mv_xy = H264Dec_pred8x16_motion (mb_cache_ptr, cache_offset, s_list);
			pred_mv_x = ((pred_mv_xy << 16) >> 16);
			pred_mv_y = (pred_mv_xy >> 16);

			s_mvd_xy = s_mvd_ptr[0];
			mvd_x = ((s_mvd_xy << 16) >> 16);
			mvd_y = (s_mvd_xy >> 16);

			mv_x = mvd_x + pred_mv_x;
			mv_y = mvd_y + pred_mv_y;

		#if defined(H264_BIG_ENDIAN)
			mv_xy  = (mv_x << 16) | (mv_y & 0xffff);
		#else
			mv_xy  = (mv_y << 16) | (mv_x & 0xffff);
		#endif

			//set mv cache
			s_ref_mv_ptr = (int32 *)(mb_cache_ptr->mv_cache[s_list]) + cache_offset;	
		#ifndef _NEON_OPT_
			for (row = 0; row < 4; row++)
			{
				s_ref_mv_ptr[0] = s_ref_mv_ptr[1] = mv_xy;
				s_ref_mv_ptr += CTX_CACHE_WIDTH;
			}
		#else
			v32x2 = vmov_n_s32 (mv_xy);
			vst1_s32 (s_ref_mv_ptr+0, v32x2);
			vst1_s32 (s_ref_mv_ptr+CTX_CACHE_WIDTH_X1, v32x2);
			vst1_s32 (s_ref_mv_ptr+CTX_CACHE_WIDTH_X2, v32x2);
			vst1_s32 (s_ref_mv_ptr+CTX_CACHE_WIDTH_X3, v32x2);
		#endif
		
	//		mv_x = ((mv_xy << 16) >> 16);
	//		mv_y = (mv_xy >> 16);				
			
			list = g_list[s_list];
			pRefFrame = list[s_ref_idx_ptr[0]]->imgYUV;
#if defined(CTS_PROTECT)
		if ((pRefFrame == NULL) || (pRefFrame[0] == NULL) ||(pRefFrame[1] == NULL) )
		{
			img_ptr->error_flag |= ER_REF_FRM_ID;
			img_ptr->return_pos2 |= (1<<22);
			return;
		}
#endif
			
			H264Dec_mc_8x16 (img_ptr, mb_cache_ptr->pred_cache[s_list].pred_Y, pRefFrame, mv_x, mv_y, b8x16);

			s_dir++;
		}while(s_dir < pdir);
	}

	return;	
}

PUBLIC int32 H264Dec_mv_prediction_P8x8_8x8_sw (DEC_MB_INFO_T *mb_info_ptr, DEC_MB_CACHE_T *mb_cache_ptr, int32 cache_offset, int32 b8)
{
	int32 pred_mv_x, pred_mv_y, pred_mv_xy;
	int32 mvd_x, mvd_y;
	int32 mv_x, mv_y;
	int32 mv_xy;
	int32 pdir = mb_cache_ptr->b8pdir[b8];
	DEC_IMAGE_PARAMS_T *img_ptr = g_image_ptr;
	uint8 **pRefFrame;
	DEC_STORABLE_PICTURE_T ** list;
#ifdef _NEON_OPT_
	int32x2_t v32x2;
#endif

	s_dir = 0;
	do
	{
		s_list = (pdir == 2) ? s_dir : pdir;
		s_ref_idx_ptr = mb_cache_ptr->ref_idx_cache[s_list]+cache_offset;
		s_ref_mv_ptr = (int32*)(mb_cache_ptr->mv_cache[s_list]) + cache_offset;
		s_mvd_ptr =(int32*)(mb_cache_ptr->mvd_cache[s_list]) + cache_offset;

		pred_mv_xy = H264Dec_pred_motion (s_ref_idx_ptr, (int16 *)s_ref_mv_ptr, 2);
		pred_mv_x = ((pred_mv_xy << 16) >> 16);
		pred_mv_y = (pred_mv_xy >> 16);

		s_mvd_xy = s_mvd_ptr[0];
		mvd_x = ((s_mvd_xy << 16) >> 16);
		mvd_y = (s_mvd_xy >> 16);

		mv_x = mvd_x + pred_mv_x;
		mv_y = mvd_y + pred_mv_y;

	#if defined(H264_BIG_ENDIAN)
		mv_xy  = (mv_x << 16) | (mv_y & 0xffff);
	#else
		mv_xy  = (mv_y << 16) | (mv_x & 0xffff);
	#endif

		//set mv cache
	#ifndef _NEON_OPT_	
		s_ref_mv_ptr[0] = s_ref_mv_ptr[1] = mv_xy; s_ref_mv_ptr += CTX_CACHE_WIDTH;
		s_ref_mv_ptr[0] = s_ref_mv_ptr[1] = mv_xy; 
	#else
		v32x2 = vmov_n_s32 (mv_xy);
		vst1_s32 (s_ref_mv_ptr+0, v32x2);
		vst1_s32 (s_ref_mv_ptr+CTX_CACHE_WIDTH_X1, v32x2);
	#endif

		list = g_list[s_list];
		pRefFrame = list[s_ref_idx_ptr[0]]->imgYUV;
#if defined(CTS_PROTECT)
		if ((pRefFrame == NULL) || (pRefFrame[0] == NULL) ||(pRefFrame[1] == NULL) )
		{
			img_ptr->error_flag |= ER_REF_FRM_ID;
			img_ptr->return_pos2 |= (1<<23);
			return 0;
		}
#endif
		
		H264Dec_mc_8x8 (img_ptr, mb_cache_ptr->pred_cache[s_list].pred_Y, pRefFrame, mv_x, mv_y, b8);

		s_dir++;
	}while(s_dir < pdir);

	return pdir;
}

PUBLIC int32 H264Dec_mv_prediction_P8x8_8x4_sw (DEC_MB_INFO_T *mb_info_ptr, DEC_MB_CACHE_T *mb_cache_ptr, int32 cache_offset, int32 b8)
{
	int32 pred_mv_x, pred_mv_y, pred_mv_xy;
	int32 mvd_x, mvd_y;
	int32 mv_x, mv_y;
	int32 mv_xy;
	uint32 b8x4;
	uint32 offset;
	int32 pdir = mb_cache_ptr->b8pdir[b8];
	DEC_IMAGE_PARAMS_T *img_ptr = g_image_ptr;
	uint8 **pRefFrame;
	DEC_STORABLE_PICTURE_T ** list;
#ifdef _NEON_OPT_
		int32x2_t v32x2;
#endif

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

			pred_mv_xy = H264Dec_pred_motion (s_ref_idx_ptr, (int16 *)s_ref_mv_ptr, 2);
			pred_mv_x = ((pred_mv_xy << 16) >> 16);
			pred_mv_y = (pred_mv_xy >> 16);

			s_mvd_xy = s_mvd_ptr[0];
			mvd_x = ((s_mvd_xy << 16) >> 16);
			mvd_y = (s_mvd_xy >> 16);

			mv_x = mvd_x + pred_mv_x;
			mv_y = mvd_y + pred_mv_y;

		#if defined(H264_BIG_ENDIAN)
			mv_xy  = (mv_x << 16) | (mv_y & 0xffff);
		#else
			mv_xy  = (mv_y << 16) | (mv_x & 0xffff);
		#endif

		#ifndef _NEON_OPT_	
			s_ref_mv_ptr[0] = s_ref_mv_ptr[1] = mv_xy;
		#else
			v32x2 = vmov_n_s32 (mv_xy);
			vst1_s32 (s_ref_mv_ptr+0, v32x2);
		#endif	

			list = g_list[s_list];
			pRefFrame = list[s_ref_idx_ptr[0]]->imgYUV;
#if defined(CTS_PROTECT)
			if ((pRefFrame == NULL) || (pRefFrame[0] == NULL) ||(pRefFrame[1] == NULL) )
			{
				img_ptr->error_flag |= ER_REF_FRM_ID;
				img_ptr->return_pos2 |= (1<<24);
				return 0;
			}
#endif
			
			H264Dec_mc_8x4 (img_ptr, mb_cache_ptr->pred_cache[s_list].pred_Y, pRefFrame, mv_x, mv_y, b8, b8x4);

			s_dir++;
		}while(s_dir < pdir);
	}

	return pdir;
}

PUBLIC int32 H264Dec_mv_prediction_P8x8_4x8_sw (DEC_MB_INFO_T *mb_info_ptr, DEC_MB_CACHE_T *mb_cache_ptr, int32 cache_offset, int32 b8)
{
	int32 pred_mv_x, pred_mv_y, pred_mv_xy;
	int32 mvd_x, mvd_y;
	int32 mv_x, mv_y;
	int32 mv_xy;
	uint32 b4x8;
	uint32 offset;
	int32 pdir = mb_cache_ptr->b8pdir[b8];
	DEC_IMAGE_PARAMS_T *img_ptr = g_image_ptr;
	uint8 **pRefFrame;
	DEC_STORABLE_PICTURE_T ** list;
	
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

			pred_mv_xy = H264Dec_pred_motion (s_ref_idx_ptr, (int16 *)s_ref_mv_ptr, 1);
			pred_mv_x = ((pred_mv_xy << 16) >> 16);
			pred_mv_y = (pred_mv_xy >> 16);

			s_mvd_xy = s_mvd_ptr[0];
			mvd_x = ((s_mvd_xy << 16) >> 16);
			mvd_y = (s_mvd_xy >> 16);

			mv_x = mvd_x + pred_mv_x;
			mv_y = mvd_y + pred_mv_y;

		#if defined(H264_BIG_ENDIAN)
			mv_xy  = (mv_x << 16) | (mv_y & 0xffff);
		#else
			mv_xy  = (mv_y << 16) | (mv_x & 0xffff);
		#endif

			s_ref_mv_ptr[0] = s_ref_mv_ptr[CTX_CACHE_WIDTH] = mv_xy;

			list = g_list[s_list];
			pRefFrame = list[s_ref_idx_ptr[0]]->imgYUV;
#if defined(CTS_PROTECT)
			if ((pRefFrame == NULL) || (pRefFrame[0] == NULL) ||(pRefFrame[1] == NULL) )
			{
				img_ptr->error_flag |= ER_REF_FRM_ID;
				img_ptr->return_pos2 |= (1<<25);
				return 0;
			}
#endif			
			H264Dec_mc_4x8 (img_ptr, mb_cache_ptr->pred_cache[s_list].pred_Y, pRefFrame, mv_x, mv_y, b8, b4x8);	

			s_dir++;
		}while(s_dir < pdir);
	}

	return pdir;
}

PUBLIC int32 H264Dec_mv_prediction_P8x8_4x4_sw (DEC_MB_INFO_T *mb_info_ptr, DEC_MB_CACHE_T *mb_cache_ptr, int32 cache_offset, int32 b8)
{
	int32 pred_mv_x, pred_mv_y, pred_mv_xy;
	int32 mvd_x, mvd_y;
	int32 mv_x, mv_y;
	int32 mv_xy;
	uint32 b4;
	uint32 offset;
	int32 pdir = mb_cache_ptr->b8pdir[b8];
	DEC_IMAGE_PARAMS_T *img_ptr = g_image_ptr;
	uint8 **pRefFrame;
	DEC_STORABLE_PICTURE_T ** list;

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

			pred_mv_xy = H264Dec_pred_motion (s_ref_idx_ptr, (int16 *)s_ref_mv_ptr, 1);
			pred_mv_x = ((pred_mv_xy << 16) >> 16);
			pred_mv_y = (pred_mv_xy >> 16);

			s_mvd_xy = s_mvd_ptr[0];
			mvd_x = ((s_mvd_xy << 16) >> 16);
			mvd_y = (s_mvd_xy >> 16);

			mv_x = mvd_x + pred_mv_x;
			mv_y = mvd_y + pred_mv_y;

		#if defined(H264_BIG_ENDIAN)
			mv_xy  = (mv_x << 16) | (mv_y & 0xffff);
		#else
			mv_xy  =  (mv_y << 16) | (mv_x & 0xffff);
		#endif
			s_ref_mv_ptr[0] = mv_xy;

			list = g_list[s_list];
			pRefFrame = list[s_ref_idx_ptr[0]]->imgYUV;
#if defined(CTS_PROTECT)
			if ((pRefFrame == NULL) || (pRefFrame[0] == NULL) ||(pRefFrame[1] == NULL) )
			{
				img_ptr->error_flag |= ER_REF_FRM_ID;
				img_ptr->return_pos2 |= (1<<26);
				return 0;
			}
#endif			
			H264Dec_mc_4x4 (img_ptr, mb_cache_ptr->pred_cache[s_list].pred_Y, pRefFrame, mv_x, mv_y, b8, b4);
	
			s_dir++;
		}while(s_dir < pdir);
	
		//configure mv command

		if ((b8 == 3) && (b4 == 3))
		{
		}
	}

	return pdir;
}

PUBLIC int32 H264Dec_MC8x8_direct_spatial_sw(DEC_MB_INFO_T *mb_info_ptr, DEC_MB_CACHE_T *mb_cache_ptr, int32 cache_offset, int32 b8)
{
	int32 fw_rFrame,bw_rFrame;
	int32 mv_xy;
	uint32 row;
	uint32 use_4x4mc = 0;
	int32 b8pdir = 0;

	fw_rFrame = mb_cache_ptr->fw_rFrame;
	bw_rFrame = mb_cache_ptr->bw_rFrame;

	s_dir = 0;	
	
	if (fw_rFrame >= 0)
	{
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
			fw_rFrame =  bw_rFrame = 0;
			s_dir = 2;
		}	
	}

	//save ref_idx to mb_cache.
	if (fw_rFrame >= 0)
	{		
		s_ref_idx_ptr = mb_cache_ptr->ref_idx_cache[0]+cache_offset;	
		s_ref_idx_ptr[0] = s_ref_idx_ptr[1] = fw_rFrame;	s_ref_idx_ptr += CTX_CACHE_WIDTH;
		s_ref_idx_ptr[0] = s_ref_idx_ptr[1] = fw_rFrame;
	}

	if (bw_rFrame >= 0)
	{		
		s_ref_idx_ptr = mb_cache_ptr->ref_idx_cache[1]+cache_offset;
		s_ref_idx_ptr[0] = s_ref_idx_ptr[1] = bw_rFrame;	s_ref_idx_ptr += CTX_CACHE_WIDTH;
		s_ref_idx_ptr[0] = s_ref_idx_ptr[1] = bw_rFrame;
	}
	
	b8pdir = (s_dir == 2) ? 2 : b8pdir;
	mb_cache_ptr->b8pdir[b8] = b8pdir;
		
	if (!use_4x4mc)
	{
		H264Dec_Config_8x8MC_info_sw (g_image_ptr, mb_cache_ptr, b8, b8pdir);
	}else
	{
		H264Dec_Config_4x4MC_info_sw(g_image_ptr, mb_cache_ptr, b8, b8pdir);
	}
	
	return b8pdir;
}

PUBLIC int32 H264Dec_MC8x8_direct_temporal_sw(DEC_MB_INFO_T *mb_info_ptr, DEC_MB_CACHE_T *mb_cache_ptr, int32 cache_offset, int32 b8)
{
	uint32 use_4x4mc = 0;
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

	b8pdir = 2;
	mb_cache_ptr->b8pdir[b8] = b8pdir;
	
	if (!use_4x4mc)
	{
		H264Dec_Config_8x8MC_info_sw(g_image_ptr, mb_cache_ptr, b8, b8pdir);
	}else
	{
		H264Dec_Config_4x4MC_info_sw(g_image_ptr, mb_cache_ptr, b8, b8pdir);
	}
	
	return b8pdir;
}

LOCAL void H264_MC_GetAverage (DEC_MB_CACHE_T *mb_cache_ptr, int32 b8)
{
	int i;
	uint8 *pPred0, *pPred1;
	uint8 *fw_mca_bfr = mb_cache_ptr->pred_cache[0].pred_Y;
	uint8 *bw_mca_bfr = mb_cache_ptr->pred_cache[1].pred_Y;

	//for 8x8 block mc
	int32 b8_luma_offset[4] = {0, 8, 8*16, 8*16+8};
	int32 b8_chroma_offset[4] = {0, 4, 4*8, 4*8+4};
#ifndef _NEON_OPT_
	int32 j;
#else
	uint8x8_t v8x8_0, v8x8_1;
#endif

	//for Y
	pPred0 = fw_mca_bfr + b8_luma_offset[b8];
	pPred1 = bw_mca_bfr + b8_luma_offset[b8];
	for (i = 8; i > 0; i--)	//y dir
	{
	#ifndef _NEON_OPT_
		for (j = 0; j < 8; j++)
		{
			pPred0[j] = (pPred0[j] + pPred1[j] + 1)/2;	
		}	
	#else
		v8x8_0 = vld1_u8(pPred0);
		v8x8_1 = vld1_u8(pPred1);
		
		v8x8_0 = vrhadd_u8(v8x8_0, v8x8_1);
		vst1_u8(pPred0, v8x8_0);		
	#endif
		pPred0 += MB_SIZE;
		pPred1 += MB_SIZE;		
	}

	//for U
	fw_mca_bfr = mb_cache_ptr->pred_cache[0].pred_UV[0];
	bw_mca_bfr = mb_cache_ptr->pred_cache[1].pred_UV[0];
	
	pPred0 = fw_mca_bfr + b8_chroma_offset[b8];
	pPred1 = bw_mca_bfr + b8_chroma_offset[b8];
	for (i = 0; i < 4; i++)
	{	
	#ifndef _NEON_OPT_
		for (j = 0; j < 4; j++)
		{
			pPred0[j] = (pPred0[j] + pPred1[j] + 1)/2;	
		}	
	#else
		v8x8_0 = vld1_u8(pPred0);
		v8x8_1 = vld1_u8(pPred1);
		
		v8x8_0 = vrhadd_u8(v8x8_0, v8x8_1);
		pPred0[0] = vget_lane_u8(v8x8_0, 0);
		pPred0[1] = vget_lane_u8(v8x8_0, 1);
		pPred0[2] = vget_lane_u8(v8x8_0, 2);
		pPred0[3] = vget_lane_u8(v8x8_0, 3);
	#endif
		pPred0 += MB_CHROMA_SIZE;
		pPred1 += MB_CHROMA_SIZE;		
	}

	//for V
	fw_mca_bfr = mb_cache_ptr->pred_cache[0].pred_UV[1];
	bw_mca_bfr = mb_cache_ptr->pred_cache[1].pred_UV[1];
	
	pPred0 = fw_mca_bfr + b8_chroma_offset[b8];
	pPred1 = bw_mca_bfr + b8_chroma_offset[b8];
	for (i = 4; i > 0; i--)
	{
	#ifndef _NEON_OPT_
		for (j = 0; j < 4; j++)
		{
			pPred0[j] = (pPred0[j] + pPred1[j] + 1)/2;	
		}	
	#else
		v8x8_0 = vld1_u8(pPred0);
		v8x8_1 = vld1_u8(pPred1);
		
		v8x8_0 = vrhadd_u8(v8x8_0, v8x8_1);
		pPred0[0] = vget_lane_u8(v8x8_0, 0);
		pPred0[1] = vget_lane_u8(v8x8_0, 1);
		pPred0[2] = vget_lane_u8(v8x8_0, 2);
		pPred0[3] = vget_lane_u8(v8x8_0, 3);
	#endif
		pPred0 += MB_CHROMA_SIZE;
		pPred1 += MB_CHROMA_SIZE;	
	}
}

LOCAL void H264_MC_Copy (DEC_MB_CACHE_T *mb_cache_ptr, int32 b8)
{
	int i;
	uint8 *pPred0, *pPred1;
	uint8 *fw_mca_bfr = mb_cache_ptr->pred_cache[0].pred_Y;
	uint8 *bw_mca_bfr = mb_cache_ptr->pred_cache[1].pred_Y;

	//for 8x8 block mc
	int32 b8_luma_offset[4] = {0, 8, 8*16, 8*16+8};
	int32 b8_chroma_offset[4] = {0, 4, 4*8, 4*8+4};

	//for Y
	pPred0 = fw_mca_bfr + b8_luma_offset[b8];
	pPred1 = bw_mca_bfr + b8_luma_offset[b8];
	for (i = 0; i < 8; i++)
	{
		*pPred0++ = *pPred1++;
		*pPred0++ = *pPred1++;
		*pPred0++ = *pPred1++;
		*pPred0++ = *pPred1++;
		*pPred0++ = *pPred1++;
		*pPred0++ = *pPred1++;
		*pPred0++ = *pPred1++;
		*pPred0++ = *pPred1++;

		pPred0 += 8;
		pPred1 += 8;		
	}

	//for U
	fw_mca_bfr = mb_cache_ptr->pred_cache[0].pred_UV[0];
	bw_mca_bfr = mb_cache_ptr->pred_cache[1].pred_UV[0];
	
	pPred0 = fw_mca_bfr + b8_chroma_offset[b8];
	pPred1 = bw_mca_bfr + b8_chroma_offset[b8];
	for (i = 0; i < 4; i++)
	{
		*pPred0++ = *pPred1++;
		*pPred0++ = *pPred1++;
		*pPred0++ = *pPred1++;
		*pPred0++ = *pPred1++;

		pPred0 += 4;
		pPred1 += 4;		
	}

	//for V
	fw_mca_bfr = mb_cache_ptr->pred_cache[0].pred_UV[1];
	bw_mca_bfr = mb_cache_ptr->pred_cache[1].pred_UV[1];
	
	pPred0 = fw_mca_bfr + b8_chroma_offset[b8];
	pPred1 = bw_mca_bfr + b8_chroma_offset[b8];
	for (i = 0; i < 4; i++)
	{
		*pPred0++ = *pPred1++;
		*pPred0++ = *pPred1++;
		*pPred0++ = *pPred1++;
		*pPred0++ = *pPred1++;

		pPred0 += 4;
		pPred1 += 4;		
	}	
}

PUBLIC void H264Dec_mv_prediction_sw (DEC_IMAGE_PARAMS_T *img_ptr, DEC_MB_INFO_T *mb_info_ptr, DEC_MB_CACHE_T *mb_cache_ptr)
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
			H264Dec_mv_prediction_skipped_pslice_sw (img_ptr, mb_info_ptr, mb_cache_ptr);
			b8pdir = 0;
		}
	}else
	{
		switch(mb_info_ptr->mb_type)
		{
		case PMB16x16:
			H264Dec_mv_prediction_PMB16x16_sw (img_ptr, mb_info_ptr, mb_cache_ptr);
			break;
		case PMB16x8:
			H264Dec_mv_prediction_PMB16x8_sw (img_ptr, mb_info_ptr, mb_cache_ptr);
			break;
		case PMB8x16:
			H264Dec_mv_prediction_PMB8x16_sw (img_ptr, mb_info_ptr, mb_cache_ptr);
			break;
		case P8x8:
			H264Dec_mv_prediction_P8x8 (img_ptr, mb_info_ptr, mb_cache_ptr);
			break;
		default:
			break;
		}
	}
#if defined(CTS_PROTECT)
	if(img_ptr->error_flag)
	{
		return;
	}
#endif

	if (img_ptr->apply_weights)
	{
		H264Dec_Config_WP_info_sw(img_ptr, mb_info_ptr, mb_cache_ptr);
	}else if (img_ptr->type == B_SLICE) //B_SLICE
	{
		int b8;
		int pdir;
		for (b8 = 0; b8 < 4; b8++)
		{
			pdir = mb_cache_ptr->b8pdir[b8];
			if (2 == pdir) //calculate average for bi-prd block
			{
				H264_MC_GetAverage(mb_cache_ptr, b8);
			}else if (1 == pdir) //copy pred pixel from pred_cache[1] to pred_cache[0]
			{
				H264_MC_Copy(mb_cache_ptr, b8);
			}
		}
	}
	
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
