/******************************************************************************
 ** File Name:    mca_top.c                                                   *
 ** Author:       Xiaowei Luo                                                 *
 ** DATE:         11/19/2008                                                  *
 ** Copyright:    2006 Spreatrum, Incoporated. All Rights Reserved.           *
 ** Description:   c model of bsmr module in mpeg4 decoder                    *
 *****************************************************************************/
/******************************************************************************
 **                   Edit    History                                         *
 **---------------------------------------------------------------------------* 
 ** DATE          NAME            DESCRIPTION                                 * 
 ** 12/14/2006    Xiaowei Luo     Create.                                     *
 *****************************************************************************/
/*----------------------------------------------------------------------------*
**                        Dependencies                                        *
**---------------------------------------------------------------------------*/
#include "sc6800x_video_header.h"
/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    extern   "C" 
    {
#endif

MC_16x16 g_dec_mc_16x16[4];
MC_8x8   g_dec_mc_8x8[4];
MC_8x8   g_dec_mc_8x8_me[4];

int32 MCA_CMD_Num;
int32 MCA_CMD_Buf[33];

PUBLIC void init_mca(void)
{
	/*init motion compensation function*/
	g_dec_mc_16x16[0] = Mp4_mc_xyfull_16x16;
	g_dec_mc_16x16[1] = Mp4_mc_xfullyhalf_16x16;
	g_dec_mc_16x16[2] = Mp4_mc_xhalfyfull_16x16;
	g_dec_mc_16x16[3] = Mp4_mc_xyhalf_16x16;

	g_dec_mc_8x8[0] = Mp4_mc_xyfull_8x8;
	g_dec_mc_8x8[1] = Mp4_mc_xfullyhalf_8x8;
	g_dec_mc_8x8[2] = Mp4_mc_xhalfyfull_8x8;
	g_dec_mc_8x8[3] = Mp4_mc_xyhalf_8x8;

	g_dec_mc_8x8_me[0] = Mp4_mc_xyfull_8x8_me;
	g_dec_mc_8x8_me[1] = Mp4_mc_xfullyhalf_8x8_me;
	g_dec_mc_8x8_me[2] = Mp4_mc_xhalfyfull_8x8_me;
	g_dec_mc_8x8_me[3] = Mp4_mc_xyhalf_8x8_me;
}

int ref_blk_id;
int ref_blk_size;
int ref_blk_end;
int ref_bir_blk;
int ref_cmd_type;
int ref_bw_frame_id;
int ref_fw_frame_id;
int16 mv_x;
int16 mv_y;
int16 mv_x_bck;
int16 mv_y_bck;
int cmd_idx;

void parse_mca_cmd_pair()
{
	uint32 blk_info = (uint32)MCA_CMD_Buf[cmd_idx++];
	int32 mv_info;

//blk info
	ref_blk_end = (blk_info>>27)&0x01;
	ref_bir_blk = (blk_info>>26)&0x01;
	ref_cmd_type = (blk_info>>24)&0x03;
	ref_bw_frame_id = (blk_info>>20)&0x0f;
	ref_fw_frame_id = (blk_info>>16)&0x0f;
	ref_blk_id = (blk_info>>8)&0x0f;
	ref_blk_size = (blk_info>>0)&0x03;

	mv_info = MCA_CMD_Buf[cmd_idx++];
//mv info
	mv_x = (int16)((mv_info<<18)>>18);
	mv_y = (int16)((mv_info<<2)>>18);	

	if(ref_bir_blk) //bi-dir
	{
		mv_info = MCA_CMD_Buf[cmd_idx++];
		mv_x_bck = (int16)((mv_info<<18)>>18);
		mv_y_bck = (int16)((mv_info<<2)>>18);	
	}
}

void mpeg4_h263_flv_motion_compensation (int32 mb_x, int32 mb_y)
{
#if defined(MPEG4_DEC)
	int32 dx, dy;
	int32 start_x, start_y;
	uint8 *pRefFrmY, *pRefFrmUV;
	uint8 *pPred;
	uint8 *fw_mca_bfr = (uint8 *)vsp_fw_mca_out_bfr;
	uint8 *bw_mca_bfr = (uint8 *)vsp_bw_mca_out_bfr;
	int32 rounding_control = (g_mca_reg_ptr->MCA_CFG>>1)&0x01;
	int32 frame_width = ((g_glb_reg_ptr->VSP_CFG1)&0xFF)*MB_SIZE;
	int32 frame_height = ((g_glb_reg_ptr->VSP_CFG1>>12)&0xFF)*MB_SIZE;

	if(ref_cmd_type == 1) //y
	{
		if (ref_fw_frame_id == 1)		//choose backward reference frame
		{
			pRefFrmY = g_dec_vop_mode_ptr->pBckRefFrame->pDecFrame->imgY;
		}else
		{
			pRefFrmY = g_dec_vop_mode_ptr->pFrdRefFrame->pDecFrame->imgY;
		}

		if(ref_blk_size == MCA_BLOCK16X16) ///1mv
		{
			if(ref_blk_id != 0)
			{
				assert(ref_blk_id == 0);
			}

			start_x = mb_x * MB_SIZE + (mv_x >> 1);
			start_y = mb_y * MB_SIZE + (mv_y >> 1);

			dx = mv_x & 1;
			dy = mv_y & 1;

			g_dec_mc_16x16[(dx<<1) | dy](pRefFrmY, fw_mca_bfr, rounding_control, frame_width, frame_height, start_x, start_y);				
		}else ///4mv
		{
			uint8 xoffset = (ref_blk_id & 1) * 8;
			uint8 yoffset = (ref_blk_id >> 1) * 8;

			start_x = mb_x * MB_SIZE + xoffset + (mv_x >> 1);
			start_y = mb_y * MB_SIZE + yoffset + (mv_y >> 1);
								
			pPred = fw_mca_bfr + yoffset * MB_SIZE + xoffset;
		
			dx = mv_x & 1;
			dy = mv_y & 1;
			g_dec_mc_8x8[(dx<<1) | dy](pRefFrmY, pPred, rounding_control, frame_width, frame_height, MB_SIZE, start_x, start_y);
		}			
	}else if(ref_cmd_type == 2)//uv
	{
		if (ref_fw_frame_id == 1)		//choose backward reference frame
		{
			pRefFrmUV = g_dec_vop_mode_ptr->pBckRefFrame->pDecFrame->imgU; 
		}else
		{
			pRefFrmUV = g_dec_vop_mode_ptr->pFrdRefFrame->pDecFrame->imgU; 
		}
				
		start_x = mb_x * BLOCK_SIZE + (mv_x>>1);
		start_y = mb_y * BLOCK_SIZE + (mv_y>>1);

		dx = mv_x & 1;
		dy = mv_y & 1;

		g_dec_mc_8x8[(dx<<1) | dy](pRefFrmUV, fw_mca_bfr+MB_SIZE*MB_SIZE, rounding_control, frame_width/2, frame_height/2, BLOCK_SIZE, start_x, start_y);
	}
			
	if(ref_bir_blk) //bi-dir
	{	
		if(ref_cmd_type == 1) //y
		{
			pRefFrmY = g_dec_vop_mode_ptr->pBckRefFrame->pDecFrame->imgY;

			if(ref_blk_size == MCA_BLOCK16X16) ///1mv
			{
				assert(ref_blk_id == 0);

				start_x = mb_x * MB_SIZE + (mv_x_bck >> 1);
				start_y = mb_y * MB_SIZE + (mv_y_bck >> 1);

				dx = mv_x_bck & 1;
				dy = mv_y_bck & 1;

				g_dec_mc_16x16[(dx<<1) | dy](pRefFrmY, bw_mca_bfr, rounding_control, frame_width, frame_height, start_x, start_y);				
			}else ///4mv
			{
				uint8 xoffset = (ref_blk_id & 1) * 8;
				uint8 yoffset = (ref_blk_id >> 1) * 8;

				start_x = mb_x * MB_SIZE + xoffset + (mv_x_bck >> 1);
				start_y = mb_y * MB_SIZE + yoffset + (mv_y_bck >> 1);
									
				pPred = bw_mca_bfr + yoffset * MB_SIZE + xoffset;
			
				dx = mv_x_bck & 1;
				dy = mv_y_bck & 1;
				g_dec_mc_8x8[(dx<<1) | dy](pRefFrmY, pPred, rounding_control, frame_width, frame_height, MB_SIZE, start_x, start_y);
			}			
		}else if(ref_cmd_type == 2)//uv
		{
			pRefFrmUV = g_dec_vop_mode_ptr->pBckRefFrame->pDecFrame->imgU; 
					
			start_x = mb_x * BLOCK_SIZE + (mv_x_bck>>1);
			start_y = mb_y * BLOCK_SIZE + (mv_y_bck>>1);

			dx = mv_x_bck & 1;
			dy = mv_y_bck & 1;
				
			g_dec_mc_8x8[(dx<<1) | dy](pRefFrmUV, bw_mca_bfr+MB_SIZE*MB_SIZE, rounding_control, frame_width/2, frame_height/2, BLOCK_SIZE, start_x, start_y);
		}
	}
#endif

	return;
}

void h264_motion_compensation (int32 mb_x, int32 mb_y)
{
#if defined(H264_DEC)
	int32 start_pos_x, start_pos_y;
	int32 blk_size;
	uint8 *rec_blk_y_ptr, *rec_blk_u_ptr, *rec_blk_v_ptr;
	uint8 *fw_mca_bfr = (uint8 *)vsp_fw_mca_out_bfr;

	if (ref_cmd_type != 0)
	{
		printf("configure error!\n");
	}

	start_pos_x = mb_x*MB_SIZE*4+mv_x;
	start_pos_y = mb_y*MB_SIZE*4+mv_y;
	if(ref_blk_size == MCA_BLOCK16X16)
	{	
		blk_size = MB_SIZE;
		rec_blk_y_ptr = fw_mca_bfr;

		h264_mc_luma (start_pos_x, start_pos_y, blk_size, rec_blk_y_ptr);

		blk_size = MB_CHROMA_SIZE;
		rec_blk_u_ptr = fw_mca_bfr+MCA_U_OFFSET;
		rec_blk_v_ptr = fw_mca_bfr+MCA_V_OFFSET;
		h264_mc_chroma (start_pos_x, start_pos_y, blk_size, rec_blk_u_ptr, rec_blk_v_ptr);
	}else if(ref_blk_size == MCA_BLOCK8X8)
	{
		int32 i, j;
		int32 b8_luma_offset[4] = {0, 8, 8*16, 8*16+8};
		int32 b8_chroma_offset[4] = {0, 4, 4*8, 4*8+4};
		
		i = ref_blk_id%2;
		j = ref_blk_id/2;

		start_pos_x += (i*8*4);
		start_pos_y += (j*8*4);

		blk_size = MB_SIZE/2;
		rec_blk_y_ptr = fw_mca_bfr + b8_luma_offset[ref_blk_id];
		h264_mc_luma (start_pos_x, start_pos_y, blk_size, rec_blk_y_ptr);

		blk_size = MB_CHROMA_SIZE/2;
		rec_blk_u_ptr = fw_mca_bfr+MCA_U_OFFSET+b8_chroma_offset[ref_blk_id];
		rec_blk_v_ptr = fw_mca_bfr+MCA_V_OFFSET+b8_chroma_offset[ref_blk_id];
		h264_mc_chroma (start_pos_x, start_pos_y, blk_size, rec_blk_u_ptr, rec_blk_v_ptr);
	}else if(ref_blk_size == MCA_BLOCK4X4)
	{
		int32 i, j;
		//0, 1,    4,  5
		//2, 3,    6,  7
		//8, 9,   12, 13
		//10, 11, 14, 15
		int32 sub4x4_luma_offset[16] = 
			{
				    0,        4,     4*16,    4*16+4,
				    8,       12,   4*16+8,   4*16+12,
				 8*16,   8*16+4,    12*16,   12*16+4,
				8*16+8, 8*16+12,  12*16+8,  12*16+12
			};
		int32 sub4x4_chroma_offset[16] = 
			{
				0,		2,		2*8,	2*8+2,
				4,		6,		2*8+4,	2*8+6,
				4*8,	4*8+2,	6*8,	6*8+2,
				4*8+4,	4*8+6,	6*8+4,	6*8+6

			};
		int32 i_idx[16] = {0, 1, 0, 1, 2, 3, 2, 3, 0, 1, 0, 1, 2, 3, 2, 3};
		int32 j_idx[16] = {0, 0, 1, 1, 0, 0, 1, 1, 2, 2, 3, 3, 2, 2, 3, 3};
		
		i = i_idx[ref_blk_id];
		j = j_idx[ref_blk_id];

		start_pos_x += (i*4*4);
		start_pos_y += (j*4*4);

		blk_size = MB_SIZE/4;
		rec_blk_y_ptr = fw_mca_bfr + sub4x4_luma_offset[ref_blk_id];
		h264_mc_luma (start_pos_x, start_pos_y, blk_size, rec_blk_y_ptr);

		blk_size = MB_CHROMA_SIZE/4;
		rec_blk_u_ptr = fw_mca_bfr+MCA_U_OFFSET+sub4x4_chroma_offset[ref_blk_id];
		rec_blk_v_ptr = fw_mca_bfr+MCA_V_OFFSET+sub4x4_chroma_offset[ref_blk_id];
		h264_mc_chroma (start_pos_x, start_pos_y, blk_size, rec_blk_u_ptr, rec_blk_v_ptr);		
	}
#endif

	return;
}

PUBLIC void mca_module(void)
{
	int32 mb_x; //should read out from global registor.
	int32 mb_y;
	int32 rounding_control;
	int32 frame_width, frame_height;
	uint8 *fw_mca_bfr = (uint8 *)vsp_fw_mca_out_bfr;
	uint8 *bw_mca_bfr = (uint8 *)vsp_bw_mca_out_bfr;
	int32 standard	  = (g_glb_reg_ptr->VSP_CFG0 >> 8) & 0x7;
	
	mb_x = g_glb_reg_ptr->VSP_CTRL0 & 0xff; //
	mb_y = ((g_glb_reg_ptr->VSP_CTRL0>>8) & 0xff);
	
	rounding_control = (g_mca_reg_ptr->MCA_CFG>>1)&0x01;
	frame_width = ((g_glb_reg_ptr->VSP_CFG1)&0xFF)*MB_SIZE;
	frame_height = ((g_glb_reg_ptr->VSP_CFG1>>12)&0xFF)*MB_SIZE;

//initial
	ref_blk_id = 0;
	ref_blk_size = 0;
	ref_blk_end = 0;
	ref_bir_blk = 0;
	ref_cmd_type = 0;
	ref_fw_frame_id = 0;
	mv_x = 0;
	mv_y = 0;
	
	cmd_idx = 0;

	while(!ref_blk_end)
	{	
		parse_mca_cmd_pair();

		if ((standard == VSP_MPEG4) || (standard == VSP_ITU_H263) || (standard == VSP_FLV_V1))
		{
			mpeg4_h263_flv_motion_compensation (mb_x, mb_y);
		}else if (standard == VSP_H264)
		{
			h264_motion_compensation (mb_x, mb_y);
		}else
		{
			printf("config error!\n");
		}
	}

	Mp4Dec_Trace_MCA_result_one_macroblock(fw_mca_bfr);
#if (!defined(MPEG4_LIB)) && defined(MPEG4_DEC)
	Mp4Dec_Trace_Interpolation_result_one_macroblock_forDebug (g_fp_trace_mca, fw_mca_bfr);
#endif //!defined(MPEG4_LIB)

	if(ref_bir_blk) //bi-dir
	{
		Mp4Dec_Trace_MCA_result_one_macroblock(bw_mca_bfr);
#if (!defined(MPEG4_LIB)) && defined(MPEG4_DEC)
		Mp4Dec_Trace_Interpolation_result_one_macroblock_forDebug(g_fp_trace_mca, bw_mca_bfr);
#endif //!defined(MPEG4_LIB)

		Mp4Dec_MC_GetAverage();
	}

	MCA_CMD_Num = 0;
}
/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End 

