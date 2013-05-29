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

//#define CHIP_ENDIAN_LITTLE

#if defined(_VSP_)
	#ifdef CHIP_ENDIAN_LITTLE
	#else
		#define H264_BIG_ENDIAN
	#endif
#endif

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
	uint32 mc_blk_info;\
	if (s_ref_frame_id[0] >= (uint32)g_list_size[0] || s_ref_frame_id[1] >= (uint32)g_list_size[0])\
	{\
		g_image_ptr->error_flag |= ER_REF_FRM_ID;\
		g_image_ptr->return_pos1 |= (1<<16);\
		return -1;\
	}else\
	{\
		mc_blk_info= (s_ref_blk_end << 27) | (s_ref_bir_blk << 26) | (s_ref_cmd_type << 24) | ((s_ref_frame_id[1]&0xf) << 20)|\
			((s_ref_frame_id[0]&0xf) << 16) | (s_ref_blk_id << 8) | (s_ref_blk_size & 0xff);\
		VSP_WRITE_REG_CQM(VSP_MCA_REG_BASE+MCA_BLK_CBUF_OFF, mc_blk_info, "configure MCA command buffer, block info");\
	}\
}

/**----------------------------------------------------------------------------*
**                           Function Prototype                               **
**----------------------------------------------------------------------------*/
#define IS_DIR(pdir)	(((pdir) == 2) || (s_list == (pdir)))

//weighted prediction
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
				
				b8 = g_b8map[blk4x4_id];
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
				}
				wp_cmd[blk4x4_id] = ((offset1&0xff)<<24) | ((offset0&0xff)<<16) | ((weight1&0xff)<<8) | (weight0&0xff);//lsw for debug 8 bits is not enough for weight since it can be 128 
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

//for one 8x8 block in one MB
int32 H264Dec_Config_8x8MC_info_hw(DEC_IMAGE_PARAMS_T *img_ptr, DEC_MB_CACHE_T *mb_cache_ptr, int32 b8, int32 b8pdir)
{
	uint32 cache_offset;

	s_ref_blk_size = MC_BLKSIZE_8x8;
	s_ref_blk_id = b8;
	s_ref_blk_end = (b8 == 3) ? 1 : 0; 
	
	cache_offset = CTX_CACHE_WIDTH_PLUS4 + g_b8_offset[b8];

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

	return 0;
}

//for four 4x4 subblock in one 8x8 block
int32 H264Dec_Config_4x4MC_info_hw(DEC_IMAGE_PARAMS_T *img_ptr, DEC_MB_CACHE_T *mb_cache_ptr, int32 b8, int32 b8pdir)
{
	uint32 b4;
	uint32 b8_cache_offset;
	uint32 b4_cache_offset;

	mb_cache_ptr->mbc_b8pdir[b8] = ((b8pdir == 2) ? 1 : 0);

	s_ref_blk_size = MC_BLKSIZE_4x4;
	s_ref_blk_end = 0;
	s_ref_blk_id = b8*4;

	b8_cache_offset = CTX_CACHE_WIDTH_PLUS4 + g_b8_offset[b8];
	for(b4 = 0; b4 < 4; b4++)
	{
		b4_cache_offset = b8_cache_offset + g_b4_offset[b4];
			
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

	return 0;
}

PUBLIC int8 H264Dec_mv_prediction_skipped_bslice_spatial_hw (DEC_IMAGE_PARAMS_T *img_ptr, DEC_MB_INFO_T *mb_info_ptr, DEC_MB_CACHE_T *mb_cache_ptr)
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
				H264Dec_Config_4x4MC_info_hw(img_ptr, mb_cache_ptr, b8, b8pdir);
			}else
			{		
				H264Dec_Config_8x8MC_info_hw(img_ptr, mb_cache_ptr, b8, b8pdir);	
			}
		}
	}

	return b8pdir;
}

PUBLIC int8 H264Dec_mv_prediction_skipped_bslice_temporal_hw (DEC_IMAGE_PARAMS_T *img_ptr, DEC_MB_INFO_T *mb_info_ptr, DEC_MB_CACHE_T *mb_cache_ptr)
{
	int32 mv_cmd[2];
	int32 use_8x8mc;
	int32 b4, offset_b4;
	int32 use_4x4mc[4] = {0, 0, 0, 0};
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

//	use_8x8mc = 0;
	for (b8=0 ; b8 < 4; b8++)
	{
		offset = ((b8>>1)<<3)+((b8&1)<<1);

		if (g_active_sps_ptr->direct_8x8_inference_flag == 0)
		{
			for (b4 = 1; b4 < 4; b4++)
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
			if (use_4x4mc[b8])
			{
				H264Dec_Config_4x4MC_info_hw(img_ptr, mb_cache_ptr, b8, b8pdir);
			}else
			{		
				H264Dec_Config_8x8MC_info_hw(img_ptr, mb_cache_ptr, b8, b8pdir);	
			}
		}
	}

	return b8pdir;
}

//pred_mv = mv_y<<16 | mv_x
LOCAL int32 H264Dec_mv_prediction_skipped_pslice_hw (DEC_IMAGE_PARAMS_T *img_ptr, DEC_MB_INFO_T *mb_info_ptr, DEC_MB_CACHE_T *mb_cache_ptr)
{
	int32 pred_mv;
	int32 mv_xy;
	int32 mv_cmd;
	int32 top_ref_idx, left_ref_idx;
#ifdef _NEON_OPT_
	int32x4_t v32x4;
#endif
	
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
		pred_mv = H264Dec_pred_motion(s_ref_idx_ptr, (int16 *)s_ref_mv_ptr, 4);
			
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
	
	//configure mv command
	WRITE_MC_BLK_INFO;
	VSP_WRITE_REG_CQM(VSP_MCA_REG_BASE+MCA_MV_CBUF_OFF, mv_cmd, "configure MCA command buffer, block mv info");
	VSP_WRITE_CMD_INFO((VSP_MCA << CQM_SHIFT_BIT) | (2<<24) | (MCA_MV_CBUF_WOFF<<8) | MCA_BLK_CBUF_WOFF);

	return 0;
}

LOCAL int32 H264Dec_mv_prediction_PMB16x16_hw (DEC_IMAGE_PARAMS_T *img_ptr, DEC_MB_INFO_T *mb_info_ptr, DEC_MB_CACHE_T *mb_cache_ptr)
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
	#if _H264_PROTECT_ & _LEVEL_HIGH_	
		if (s_list >=2)
		{
			img_ptr->error_flag |= ER_REF_FRM_ID;
			img_ptr->return_pos2 |= (1<<15);
			return -1;
		}
	#endif	
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
	
	return 0;	
}

LOCAL int32 H264Dec_mv_prediction_PMB16x8_hw (DEC_IMAGE_PARAMS_T *img_ptr, DEC_MB_INFO_T *mb_info_ptr, DEC_MB_CACHE_T *mb_cache_ptr)
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
		#if _H264_PROTECT_ & _LEVEL_HIGH_	
			if (s_list >=2)
			{
				img_ptr->error_flag |= ER_REF_FRM_ID;
				img_ptr->return_pos1 |= (1<<6);
				return -1;
			}
		#endif	
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

	return 0;	
}

LOCAL int32 H264Dec_mv_prediction_PMB8x16_hw (DEC_IMAGE_PARAMS_T *img_ptr, DEC_MB_INFO_T *mb_info_ptr, DEC_MB_CACHE_T *mb_cache_ptr)
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
		#if _H264_PROTECT_ & _LEVEL_HIGH_	
			if (s_list >=2)
			{
				img_ptr->error_flag |= ER_REF_FRM_ID;
				img_ptr->return_pos1 |= (1<<7);
				return -1;
			}
		#endif	
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

	return 0;	
}
PUBLIC int32 H264Dec_mv_prediction_P8x8_8x8_hw (DEC_MB_INFO_T *mb_info_ptr, DEC_MB_CACHE_T *mb_cache_ptr, int32 cache_offset, int32 b8)
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
	#if _H264_PROTECT_ & _LEVEL_HIGH_	
		if (s_list >=2)
		{
			g_image_ptr->error_flag |= ER_REF_FRM_ID;
			g_image_ptr->return_pos1 |= (1<<8);
			return -1;
		}
	#endif	
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

PUBLIC int32 H264Dec_mv_prediction_P8x8_8x4_hw (DEC_MB_INFO_T *mb_info_ptr, DEC_MB_CACHE_T *mb_cache_ptr, int32 cache_offset, int32 b8)
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
		#if _H264_PROTECT_ & _LEVEL_HIGH_	
			if (s_list >=2)
			{
				g_image_ptr->error_flag |= ER_REF_FRM_ID;
				g_image_ptr->return_pos1 |= (1<<9);
				return -1;
			}
		#endif	
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

PUBLIC int32 H264Dec_mv_prediction_P8x8_4x8_hw (DEC_MB_INFO_T *mb_info_ptr, DEC_MB_CACHE_T *mb_cache_ptr, int32 cache_offset, int32 b8)
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
		#if _H264_PROTECT_ & _LEVEL_HIGH_	
			if (s_list >=2)
			{
				g_image_ptr->error_flag |= ER_REF_FRM_ID;
				g_image_ptr->return_pos1 |= (1<<10);
				return -1;
			}
		#endif	
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

PUBLIC int32 H264Dec_mv_prediction_P8x8_4x4_hw (DEC_MB_INFO_T *mb_info_ptr, DEC_MB_CACHE_T *mb_cache_ptr, int32 cache_offset, int32 b8)
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
		#if _H264_PROTECT_ & _LEVEL_HIGH_	
			if (s_list >=2)
			{
				g_image_ptr->error_flag |= ER_REF_FRM_ID;
				g_image_ptr->return_pos1 |= (1<<11);
				return -1;
			}
		#endif	
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

PUBLIC int32 H264Dec_MC8x8_direct_spatial_hw(DEC_MB_INFO_T *mb_info_ptr, DEC_MB_CACHE_T *mb_cache_ptr, int32 cache_offset, int32 b8)
{
	int32 fw_rFrame,bw_rFrame;
	int32 mv_xy;
	uint32 row;
	uint32 use_4x4mc = 0;
	int32 mv_cmd[2];
	int32 b8pdir = 0;

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
		H264Dec_Config_4x4MC_info_hw(g_image_ptr, mb_cache_ptr, b8, s_dir);
	}
	
	return b8pdir;
}

PUBLIC int32 H264Dec_MC8x8_direct_temporal_hw(DEC_MB_INFO_T *mb_info_ptr, DEC_MB_CACHE_T *mb_cache_ptr, int32 cache_offset, int32 b8)
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
		H264Dec_Config_4x4MC_info_hw(g_image_ptr, mb_cache_ptr, b8, s_dir);
	}
	
	return b8pdir;
}

PUBLIC void H264Dec_mv_prediction_hw (DEC_IMAGE_PARAMS_T *img_ptr, DEC_MB_INFO_T *mb_info_ptr, DEC_MB_CACHE_T *mb_cache_ptr)
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
			H264Dec_mv_prediction_skipped_pslice_hw (img_ptr, mb_info_ptr, mb_cache_ptr);
			b8pdir = 0;
		}
	}else
	{
		switch(mb_info_ptr->mb_type)
		{
		case PMB16x16:
			H264Dec_mv_prediction_PMB16x16_hw (img_ptr, mb_info_ptr, mb_cache_ptr);
			break;
		case PMB16x8:
			H264Dec_mv_prediction_PMB16x8_hw (img_ptr, mb_info_ptr, mb_cache_ptr);
			break;
		case PMB8x16:
			H264Dec_mv_prediction_PMB8x16_hw (img_ptr, mb_info_ptr, mb_cache_ptr);
			break;
		case P8x8:
			H264Dec_mv_prediction_P8x8 (img_ptr, mb_info_ptr, mb_cache_ptr);
			break;
		default:
			break;
		}
	}

	if (img_ptr->error_flag)
	{
	#if _CMODEL_
		MCA_CMD_Num = 0;
	#endif
		return;
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
