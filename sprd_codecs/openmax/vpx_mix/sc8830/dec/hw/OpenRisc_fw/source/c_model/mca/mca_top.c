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
#include "sc8810_video_header.h"
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

RvDec_MC g_luma_mc[4][4];
RvDec_MC g_chroma_mc[4][4];
int32 MCA_CMD_Num;
#ifdef VP8_DEC
int32 MCA_CMD_Buf[56200];//[16200];
#else
int32 MCA_CMD_Buf[49];//[16200];
#endif
PUBLIC void init_mca(void)
{
#if defined(REAL_DEC)
	if(g_rv_decoder_ptr->bIsRV8)
	{
		g_luma_mc[0][0] = RvDec_MC_Luma_H00V00_R8;
		g_luma_mc[1][0] = RvDec_MC_Luma_H01V00_R8;
		g_luma_mc[2][0] = RvDec_MC_Luma_H02V00_R8;
		g_luma_mc[3][0] = NULL;
		
		g_luma_mc[0][1] = RvDec_MC_Luma_H00V01_R8;
		g_luma_mc[1][1] = RvDec_MC_Luma_H01V01_R8;
		g_luma_mc[2][1] = RvDec_MC_Luma_H02V01_R8;
		g_luma_mc[3][1] = PNULL;
			
		g_luma_mc[0][2] = RvDec_MC_Luma_H00V02_R8;
		g_luma_mc[1][2] = RvDec_MC_Luma_H01V02_R8;
		g_luma_mc[2][2] = RvDec_MC_Luma_H02V02_R8;
		g_luma_mc[3][2] = PNULL;

		g_luma_mc[0][3] = PNULL;
		g_luma_mc[1][3] = PNULL;
		g_luma_mc[2][3] = PNULL;
		g_luma_mc[3][3] = PNULL;


		g_chroma_mc[0][0] = RvDec_MC_Chroma_H00V00_R8;
		g_chroma_mc[1][0] = RvDec_MC_Chroma_H01V00_R8;
		g_chroma_mc[2][0] = RvDec_MC_Chroma_H02V00_R8;
		g_chroma_mc[3][0] = PNULL;
			
		g_chroma_mc[0][1] = RvDec_MC_Chroma_H00V01_R8;
		g_chroma_mc[1][1] = RvDec_MC_Chroma_H01V01_R8;
		g_chroma_mc[2][1] = RvDec_MC_Chroma_H02V01_R8;
		g_chroma_mc[3][1] = PNULL;
			
		g_chroma_mc[0][2] = RvDec_MC_Chroma_H00V02_R8;
		g_chroma_mc[1][2] = RvDec_MC_Chroma_H01V02_R8;
		g_chroma_mc[2][2] = RvDec_MC_Chroma_H02V02_R8;
		g_chroma_mc[3][2] = PNULL;

		g_chroma_mc[0][3] = PNULL;
		g_chroma_mc[1][3] = PNULL;
		g_chroma_mc[2][3] = PNULL;
		g_chroma_mc[3][3] = PNULL;
	}else
	{
		g_luma_mc[0][0] = RvDec_MC_Luma_H00V00_R9;
		g_luma_mc[1][0] = RvDec_MC_Luma_H01V00_R9;
		g_luma_mc[2][0] = RvDec_MC_Luma_H02V00_R9;
		g_luma_mc[3][0] = RvDec_MC_Luma_H03V00_R9;
		
		g_luma_mc[0][1] = RvDec_MC_Luma_H00V01_R9;
		g_luma_mc[1][1] = RvDec_MC_Luma_H01V01_R9;
		g_luma_mc[2][1] = RvDec_MC_Luma_H02V01_R9;
		g_luma_mc[3][1] = RvDec_MC_Luma_H03V01_R9;
			
		g_luma_mc[0][2] = RvDec_MC_Luma_H00V02_R9;
		g_luma_mc[1][2] = RvDec_MC_Luma_H01V02_R9;
		g_luma_mc[2][2] = RvDec_MC_Luma_H02V02_R9;
		g_luma_mc[3][2] = RvDec_MC_Luma_H03V02_R9;

		g_luma_mc[0][3] = RvDec_MC_Luma_H00V03_R9;
		g_luma_mc[1][3] = RvDec_MC_Luma_H01V03_R9;
		g_luma_mc[2][3] = RvDec_MC_Luma_H02V03_R9;
		g_luma_mc[3][3] = RvDec_MC_Luma_H03V03_R9;


		g_chroma_mc[0][0] = RvDec_MC_Chroma_H00V00_R9;
		g_chroma_mc[1][0] = RvDec_MC_Chroma_H01V00_R9;
		g_chroma_mc[2][0] = RvDec_MC_Chroma_H02V00_R9;
		g_chroma_mc[3][0] = RvDec_MC_Chroma_H03V00_R9;
			
		g_chroma_mc[0][1] = RvDec_MC_Chroma_H00V01_R9;
		g_chroma_mc[1][1] = RvDec_MC_Chroma_H01V01_R9;
		g_chroma_mc[2][1] = RvDec_MC_Chroma_H02V01_R9;
		g_chroma_mc[3][1] = RvDec_MC_Chroma_H03V01_R9;
			
		g_chroma_mc[0][2] = RvDec_MC_Chroma_H00V02_R9;
		g_chroma_mc[1][2] = RvDec_MC_Chroma_H01V02_R9;
		g_chroma_mc[2][2] = RvDec_MC_Chroma_H02V02_R9;
		g_chroma_mc[3][2] = RvDec_MC_Chroma_H03V02_R9;

		g_chroma_mc[0][3] = RvDec_MC_Chroma_H00V03_R9;
		g_chroma_mc[1][3] = RvDec_MC_Chroma_H01V03_R9;
		g_chroma_mc[2][3] = RvDec_MC_Chroma_H02V03_R9;
		g_chroma_mc[3][3] = RvDec_MC_Chroma_H02V02_R9;
	}	
#else
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
#ifdef VP8_DEC	
	cmd_idx = 0;
#endif	
#endif
}




#ifndef VP8_DEC
int ref_y_end;
#else
uint8 int_type;
#endif
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

int skip_flag;

void init_mca_cmd()
{
	cmd_idx = 0;
	skip_flag = 0;
}

void parse_mca_cmd_pair()
{
	uint32 blk_info = (uint32)MCA_CMD_Buf[cmd_idx++];
	int32 mv_info;

//blk info
#ifdef VP8_DEC
int_type = (uint8)((blk_info>>28)&0x01);			//0: six tap. 1: bilinear filter
#else
    ref_y_end = (blk_info>>28)&0x1;
#endif
	ref_blk_end = (blk_info>>27)&0x01;
	ref_bir_blk = (blk_info>>26)&0x01;
	ref_cmd_type = (blk_info>>24)&0x03;
	ref_bw_frame_id = (blk_info>>20)&0x0f;
	ref_fw_frame_id = (blk_info>>16)&0x0f;
	ref_blk_id = (blk_info>>8)&0x0f;
	ref_blk_size = (blk_info>>0)&0x03;
	

	if(ref_blk_id == 12)
		printf("");
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

	//for 4x4 block mc
	///0, 1,    4,  5
	///2, 3,    6,  7
	///8, 9,   12, 13
	///10, 11, 14, 15
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

	//mca order
	// 0, 1, 4, 5,
	// 2, 3, 6, 7,
	// 8, 9, 12, 13
	//10, 11, 14, 15

	//mbc order
	//0, 1, 2, 3
	//4, 5, 6, 7
	//8, 9, 10, 11
	//12, 13, 14, 15
	int32 sub4x4_order_mca2mbc[16] = 
		{
			0, 1, 4, 5,
			2, 3, 6, 7,
			8, 9, 12, 13,
			10, 11, 14, 15
		};

void h264_wp_block_oneDir (int32 luma_blk_size, int32 blk4x4_id)
{
	int32 h264_Y_SHIFT = (g_mbc_reg_ptr->MBC_CFG >> 4) & 0x7;
	int32 h264_UV_SHIFT = (g_mbc_reg_ptr->MBC_CFG >> 1) & 0x7;
	int32 row, j;
	uint8 *fw_mca_bfr = (uint8 *)vsp_fw_mca_out_bfr;
	uint8 *p;
	int32 tmp;
	int32 component;
	int32 blk_size;

	for (component = 0; component < 3; component++)
	{
		int32 pitch = (component == 0) ? MB_SIZE : MB_CHROMA_SIZE;
		int32 shift = (component == 0) ? h264_Y_SHIFT : h264_UV_SHIFT;	
		int32 mbc_blk4x4_id = sub4x4_order_mca2mbc[blk4x4_id];
		int32 wp_cmd = g_mbc_reg_ptr->HY_WP_BLK[component*16 + mbc_blk4x4_id];
		int32 offset0 = ((wp_cmd<<8) >> 24);
		int32 weight0 = (wp_cmd << 24 ) >> 24;
		uint32 w128_flag = g_mbc_reg_ptr->H264_weight128[component];
		uint32 is_same_param = g_mbc_reg_ptr->MBC_CFG & (1<<(component+16));

		if (is_same_param)
		{
			wp_cmd = g_mbc_reg_ptr->HY_WP_BLK[component*16 + 0];
			offset0 = ((wp_cmd<<8) >> 24);
			weight0 = (wp_cmd << 24 ) >> 24;	
		}

		if (weight0 == -128 && (w128_flag & (1<<mbc_blk4x4_id)))
		{
			weight0 = 128;
		}
		
		if (component == 0) //y
		{
			p = fw_mca_bfr + sub4x4_luma_offset[blk4x4_id];
			blk_size = luma_blk_size;
		}else if (component == 1) //u
		{
			p = fw_mca_bfr + MCA_U_OFFSET + sub4x4_chroma_offset[blk4x4_id];
			blk_size /= 2;
		}else	//v
		{
			p = fw_mca_bfr + MCA_V_OFFSET + sub4x4_chroma_offset[blk4x4_id];
		}

		for (row = 0; row < blk_size; row++)
		{
			for (j = 0; j < blk_size; j++)
			{
				tmp = p[j];
				if (shift >= 1)
				{
					tmp = ((p[j] * weight0 + (1<<(shift-1))) >> shift) + offset0;
				}else
				{
					tmp = tmp * weight0 + offset0;
				}
				p[j] = IClip(0, 255, tmp);
			}
			p += pitch;
		}
		
	}	
}

void h264_wp_block_twoDir (int32 luma_blk_size, int32 blk4x4_id)
{
	int32 h264_Y_SHIFT = (g_mbc_reg_ptr->MBC_CFG >> 4) & 0x7;
	int32 h264_UV_SHIFT = (g_mbc_reg_ptr->MBC_CFG >> 1) & 0x7;
	int32 row, j;
	uint8 *fw_mca_bfr = (uint8 *)vsp_fw_mca_out_bfr;
	uint8 *bw_mca_bfr = (uint8 *)vsp_bw_mca_out_bfr;
	uint8 *p_fwd, *p_bck;
	int32 tmp, fw, bw;
	int32 component;
	int32 blk_size;

	for (component = 0; component < 3; component++)
	{
		int32 pitch = (component == 0) ? MB_SIZE : MB_CHROMA_SIZE;
		int32 shift = (component == 0) ? h264_Y_SHIFT : h264_UV_SHIFT;	
		int32 mbc_blk4x4_id = sub4x4_order_mca2mbc[blk4x4_id];
		int32 wp_cmd = g_mbc_reg_ptr->HY_WP_BLK[component*16 + mbc_blk4x4_id];
		int32 offset0 = (wp_cmd << 8) >> 24;
		int32 weight0 = (wp_cmd << 24 ) >> 24;
		int32 offset1 = (wp_cmd >> 24);
		int32 weight1 = (wp_cmd << 16 ) >> 24;
		uint32 w128_flag = g_mbc_reg_ptr->H264_weight128[component];
		uint32 is_same_param = g_mbc_reg_ptr->MBC_CFG & (1<<(component+16));

		if (is_same_param)
		{
			wp_cmd = g_mbc_reg_ptr->HY_WP_BLK[component*16 + 0];
			offset0 = ((wp_cmd<<8) >> 24);
			weight0 = (wp_cmd << 24 ) >> 24;	
			offset1 = (wp_cmd >> 24);
			weight1 = (wp_cmd << 16 ) >> 24;
		}

		if (weight0 == -128 && (w128_flag & (1<<mbc_blk4x4_id)))
		{
			weight0 = 128;
		}

		if (weight1 == -128 && (w128_flag & (1<<(mbc_blk4x4_id+16))))
		{
			weight1 = 128;
		}
		
		if (component == 0) //y
		{
			p_fwd = fw_mca_bfr + sub4x4_luma_offset[blk4x4_id];
			p_bck = bw_mca_bfr + sub4x4_luma_offset[blk4x4_id];
			blk_size = luma_blk_size;
		}else if (component == 1) //u
		{
			p_fwd = fw_mca_bfr + MCA_U_OFFSET + sub4x4_chroma_offset[blk4x4_id];
			p_bck = bw_mca_bfr + MCA_U_OFFSET + sub4x4_chroma_offset[blk4x4_id];
			blk_size /= 2;
		}else	//v
		{
			p_fwd = fw_mca_bfr + MCA_V_OFFSET + sub4x4_chroma_offset[blk4x4_id];
			p_bck = bw_mca_bfr + MCA_V_OFFSET + sub4x4_chroma_offset[blk4x4_id];
		}

		for (row = 0; row < blk_size; row++)
		{
			for (j = 0; j < blk_size; j++)
			{
				fw = p_fwd[j];
				bw = p_bck[j];

				tmp = ((fw*weight0 + bw*weight1 + (1<<shift)) >> (shift+1)) + ((offset0 + offset1+1)>>1);
				p_fwd[j] = IClip(0, 255, tmp);
			}
			p_fwd += pitch;
			p_bck += pitch;
		}		
	}
}

	//block id order for 4x4 block wp
	///0,   1,    4,  5
	///2,   3,    6,  7
	///8,   9,   12, 13
	///10, 11,   14, 15

void h264_weighted_prediction ()
{
	uint8 *fw_mca_bfr = (uint8 *)vsp_fw_mca_out_bfr;
	uint8 *bw_mca_bfr = (uint8 *)vsp_bw_mca_out_bfr;
	
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
		
		if(ref_blk_size == MCA_BLOCK16X16)
		{
			if (!ref_bir_blk)
			{
				h264_wp_block_oneDir (16, 0);
			}else
			{
				h264_wp_block_twoDir (16, 0);
			}	
		}else if(ref_blk_size == MCA_BLOCK8X8)
		{
			int32 b8_4x4blk[4] = {0, 4, 8, 12};

			if (!ref_bir_blk)
			{
				h264_wp_block_oneDir (8, b8_4x4blk[ref_blk_id]);
			}else
			{
				h264_wp_block_twoDir (8, b8_4x4blk[ref_blk_id]);
			}			
		}else if(ref_blk_size == MCA_BLOCK4X4)
		{
			if (!ref_bir_blk)
			{
				h264_wp_block_oneDir (4, ref_blk_id);
			}else
			{
				h264_wp_block_twoDir (4, ref_blk_id);
			}		
		}		
	}
}

void h264_motion_compensation (int32 mb_x, int32 mb_y)
{
#if defined(H264_DEC)
	int32 start_pos_x, start_pos_y;
	int32 blk_size;
	uint8 *rec_blk_y_ptr, *rec_blk_u_ptr, *rec_blk_v_ptr;
	uint8 *fw_mca_bfr = (uint8 *)vsp_fw_mca_out_bfr;
	uint8 *bw_mca_bfr = (uint8 *)vsp_bw_mca_out_bfr;
	int32 i, j;

	//for 8x8 block mc
	int32 b8_luma_offset[4] = {0, 8, 8*16, 8*16+8};
	int32 b8_chroma_offset[4] = {0, 4, 4*8, 4*8+4};

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

		h264_mc_luma (start_pos_x, start_pos_y, blk_size, rec_blk_y_ptr, 0);

		blk_size = MB_CHROMA_SIZE;
		rec_blk_u_ptr = fw_mca_bfr+MCA_U_OFFSET;
		rec_blk_v_ptr = fw_mca_bfr+MCA_V_OFFSET;
		h264_mc_chroma (start_pos_x, start_pos_y, blk_size, rec_blk_u_ptr, rec_blk_v_ptr, 0);


	}else if(ref_blk_size == MCA_BLOCK8X8)
	{
		i = ref_blk_id%2;
		j = ref_blk_id/2;

		start_pos_x += (i*8*4);
		start_pos_y += (j*8*4);

		blk_size = MB_SIZE/2;
		rec_blk_y_ptr = fw_mca_bfr + b8_luma_offset[ref_blk_id];
		h264_mc_luma (start_pos_x, start_pos_y, blk_size, rec_blk_y_ptr, 0);

		blk_size = MB_CHROMA_SIZE/2;
		rec_blk_u_ptr = fw_mca_bfr+MCA_U_OFFSET+b8_chroma_offset[ref_blk_id];
		rec_blk_v_ptr = fw_mca_bfr+MCA_V_OFFSET+b8_chroma_offset[ref_blk_id];
		h264_mc_chroma (start_pos_x, start_pos_y, blk_size, rec_blk_u_ptr, rec_blk_v_ptr, 0);
	}else if(ref_blk_size == MCA_BLOCK4X4)
	{
		int32 i, j;
		
		i = i_idx[ref_blk_id];
		j = j_idx[ref_blk_id];

		start_pos_x += (i*4*4);
		start_pos_y += (j*4*4);

		blk_size = MB_SIZE/4;
		rec_blk_y_ptr = fw_mca_bfr + sub4x4_luma_offset[ref_blk_id];
		h264_mc_luma (start_pos_x, start_pos_y, blk_size, rec_blk_y_ptr, 0);

		blk_size = MB_CHROMA_SIZE/4;
		rec_blk_u_ptr = fw_mca_bfr+MCA_U_OFFSET+sub4x4_chroma_offset[ref_blk_id];
		rec_blk_v_ptr = fw_mca_bfr+MCA_V_OFFSET+sub4x4_chroma_offset[ref_blk_id];
		h264_mc_chroma (start_pos_x, start_pos_y, blk_size, rec_blk_u_ptr, rec_blk_v_ptr, 0);		
	}

	if(ref_bir_blk) //bi-dir
	{
		start_pos_x = mb_x*MB_SIZE*4+mv_x_bck;
		start_pos_y = mb_y*MB_SIZE*4+mv_y_bck;
		if(ref_blk_size == MCA_BLOCK16X16)
		{	
			blk_size = MB_SIZE;
			rec_blk_y_ptr = bw_mca_bfr;

			h264_mc_luma (start_pos_x, start_pos_y, blk_size, rec_blk_y_ptr, 1);

			blk_size = MB_CHROMA_SIZE;
			rec_blk_u_ptr = bw_mca_bfr+MCA_U_OFFSET;
			rec_blk_v_ptr = bw_mca_bfr+MCA_V_OFFSET;
			h264_mc_chroma (start_pos_x, start_pos_y, blk_size, rec_blk_u_ptr, rec_blk_v_ptr, 1);
		}else if(ref_blk_size == MCA_BLOCK8X8)
		{
			i = ref_blk_id%2;
			j = ref_blk_id/2;

			start_pos_x += (i*8*4);
			start_pos_y += (j*8*4);

			blk_size = MB_SIZE/2;
			rec_blk_y_ptr = bw_mca_bfr + b8_luma_offset[ref_blk_id];
			h264_mc_luma (start_pos_x, start_pos_y, blk_size, rec_blk_y_ptr, 1);

			blk_size = MB_CHROMA_SIZE/2;
			rec_blk_u_ptr = bw_mca_bfr+MCA_U_OFFSET+b8_chroma_offset[ref_blk_id];
			rec_blk_v_ptr = bw_mca_bfr+MCA_V_OFFSET+b8_chroma_offset[ref_blk_id];
			h264_mc_chroma (start_pos_x, start_pos_y, blk_size, rec_blk_u_ptr, rec_blk_v_ptr, 1);
		}else if(ref_blk_size == MCA_BLOCK4X4)
		{
			int32 i, j;
			
			i = i_idx[ref_blk_id];
			j = j_idx[ref_blk_id];

			start_pos_x += (i*4*4);
			start_pos_y += (j*4*4);

			blk_size = MB_SIZE/4;
			rec_blk_y_ptr = bw_mca_bfr + sub4x4_luma_offset[ref_blk_id];
			h264_mc_luma (start_pos_x, start_pos_y, blk_size, rec_blk_y_ptr, 1);

			blk_size = MB_CHROMA_SIZE/4;
			rec_blk_u_ptr = bw_mca_bfr+MCA_U_OFFSET+sub4x4_chroma_offset[ref_blk_id];
			rec_blk_v_ptr = bw_mca_bfr+MCA_V_OFFSET+sub4x4_chroma_offset[ref_blk_id];
			h264_mc_chroma (start_pos_x, start_pos_y, blk_size, rec_blk_u_ptr, rec_blk_v_ptr, 1);		
		}
	}
#endif

	return;
}

//MCA top, parsing mca command and start filter

void rv9_motion_compensation (int32 mb_x, int32 mb_y)
{
#if defined(REAL_DEC)
	int32 dx, dy;
	int32 start_x, start_y,start_x_c,start_y_c;
	uint8 *pRefFrmY;
	uint8 *pRefFrmU;
	uint8 *pRefFrmV;
	uint8 *pPred;
	uint8 uSub4x4BlkIdx;
	int32 xoffset, yoffset;
	uint8 uMcFactor = 4;
	uint8 *fw_mca_bfr = (uint8 *)vsp_fw_mca_out_bfr;
	uint8 *bw_mca_bfr = (uint8 *)vsp_bw_mca_out_bfr;
	int32 rounding_control = (g_mca_reg_ptr->MCA_CFG>>1)&0x01;
	int32 frame_width = ((g_glb_reg_ptr->VSP_CFG1)&0xFF)*MB_SIZE;
	int32 frame_height = ((g_glb_reg_ptr->VSP_CFG1>>12)&0xFF)*MB_SIZE;
	int32 frame_height_y = frame_height;
	int32 frame_height_c =  frame_height >>1;

	if(ref_cmd_type == 0) //y and uv shared cmd
	{
		/*if (ref_fw_frame_id == 0 && g_rv_decoder_ptr->uPicCodeType != INTERPIC)*/		//choose backward reference frame
		if(ref_fw_frame_id == 1)
		{
			pRefFrmY = g_rv_decoder_ptr->pBckRefFrame->pDecFrame->imgY;
			pRefFrmU = g_rv_decoder_ptr->pBckRefFrame->pDecFrame->imgU;
			pRefFrmV = pRefFrmU + 1;
		}
		else
		{
			pRefFrmY = g_rv_decoder_ptr->pFrdRefFrame->pDecFrame->imgY;
			pRefFrmU = g_rv_decoder_ptr->pFrdRefFrame->pDecFrame->imgU;
			pRefFrmV = pRefFrmU + 1;		
		}


		if(ref_blk_size == MCA_BLOCK16X16) ///1mv
		{
			if(ref_blk_id != 0)
			{
				assert(ref_blk_id == 0);
			}
			//y 
			start_x = mb_x * MB_SIZE  + (mv_x >>2) ;
			start_y = mb_y * MB_SIZE  + (mv_y >>2) ;

			dx = mv_x & 3;
			dy = mv_y & 3;

			frame_height = frame_height_y;

			pPred = fw_mca_bfr;

			g_luma_mc[dx][dy](pRefFrmY, pPred, frame_width, frame_height, start_x, start_y, MB_SIZE); 

			//u v

			start_x_c = mb_x * MB_CHROMA_SIZE  + ((mv_x /2) >> 2);
			start_y_c = mb_y * MB_CHROMA_SIZE  + ((mv_y /2) >> 2);
			
			dx = (mv_x /2) & 3;
			dy = (mv_y /2) & 3;
			
			frame_height = frame_height_c;
			
			for (uSub4x4BlkIdx = 0; uSub4x4BlkIdx < 4; uSub4x4BlkIdx ++)
			{				
				xoffset = (uSub4x4BlkIdx & 1) * 4;
				yoffset = (uSub4x4BlkIdx >> 1) * 4;

				start_x = start_x_c + xoffset;
				start_y = start_y_c + yoffset;
				// 4x4 blk of u
				pPred = fw_mca_bfr + MB_SIZE * MB_SIZE + yoffset * MB_CHROMA_SIZE + xoffset;
				g_chroma_mc[dx][dy](pRefFrmU, pPred, frame_width, frame_height, start_x, start_y, MB_CHROMA_SIZE);
				
				// 4x4 blk of v
				pPred = fw_mca_bfr + MB_SIZE * MB_SIZE + MB_CHROMA_SIZE * MB_CHROMA_SIZE + yoffset * MB_CHROMA_SIZE + xoffset;
				g_chroma_mc[dx][dy](pRefFrmV, pPred, frame_width, frame_height, start_x, start_y, MB_CHROMA_SIZE);
						
			}
		}else ///4mv
		{
			//y 
			xoffset = (ref_blk_id & 1) * 8;
			yoffset = (ref_blk_id >> 1) * 8;

			start_x = mb_x * MB_SIZE  + xoffset + (mv_x >> 2);
			start_y = mb_y * MB_SIZE  + yoffset + (mv_y >> 2);					
					
			dx = mv_x & 3;
			dy = mv_y & 3;

			pPred = fw_mca_bfr + yoffset * MB_SIZE + xoffset;

			frame_height = frame_height_y;

			g_luma_mc[dx][dy](pRefFrmY, pPred, frame_width, frame_height, start_x, start_y, BLOCK_SIZE); 

			// u v
			xoffset = xoffset >> 1;
			yoffset = yoffset >> 1;

			start_x = mb_x * MB_CHROMA_SIZE  + xoffset + ((mv_x /2) >> 2);
			start_y = mb_y * MB_CHROMA_SIZE  + yoffset + ((mv_y /2) >> 2);

			dx = (mv_x /2) & 3;
			dy = (mv_y /2) & 3;
			
			frame_height = frame_height_c;

			// 4x4 blk of u
			pPred = fw_mca_bfr + MB_SIZE * MB_SIZE + yoffset * MB_CHROMA_SIZE + xoffset;
			g_chroma_mc[dx][dy](pRefFrmU, pPred, frame_width, frame_height, start_x, start_y, MB_CHROMA_SIZE);
				
			// 4x4 blk of v
			pPred = fw_mca_bfr + MB_SIZE * MB_SIZE + MB_CHROMA_SIZE * MB_CHROMA_SIZE + yoffset * MB_CHROMA_SIZE + xoffset;
			g_chroma_mc[dx][dy](pRefFrmV, pPred, frame_width, frame_height, start_x, start_y, MB_CHROMA_SIZE);
						
		}			
	}
	else if(ref_cmd_type == 1) // y
	{
		/*if (ref_fw_frame_id == 0 && g_rv_decoder_ptr->uPicCodeType != INTERPIC)*/		//choose backward reference frame
		if(ref_fw_frame_id == 1)
		{
			pRefFrmY = g_rv_decoder_ptr->pBckRefFrame->pDecFrame->imgY;
		}else
		{	
			pRefFrmY = g_rv_decoder_ptr->pFrdRefFrame->pDecFrame->imgY;
		}


		if(ref_blk_size == MCA_BLOCK16X16) ///1mv
		{
			if(ref_blk_id != 0)
			{
				assert(ref_blk_id == 0);
			}
			//y 
			start_x = mb_x * MB_SIZE  + (mv_x >>2) ;
			start_y = mb_y * MB_SIZE  + (mv_y >>2) ;

			dx = mv_x & 3;
			dy = mv_y & 3;

			pPred = fw_mca_bfr;

			frame_height = frame_height_y;

			g_luma_mc[dx][dy](pRefFrmY, pPred, frame_width, frame_height, start_x, start_y, MB_SIZE); 
		}else ///4mv
		{
			//y 
			xoffset = (ref_blk_id & 1) * 8;
			yoffset = (ref_blk_id >> 1) * 8;

			start_x = mb_x * MB_SIZE  + xoffset + (mv_x >> 2);
			start_y = mb_y * MB_SIZE  + yoffset + (mv_y >> 2);					
					
			dx = mv_x & 3;
			dy = mv_y & 3;

			pPred = fw_mca_bfr + yoffset * MB_SIZE + xoffset;

			frame_height = frame_height_y;

			g_luma_mc[dx][dy](pRefFrmY, pPred, frame_width, frame_height, start_x, start_y, BLOCK_SIZE); 

		}
	}
	else if (ref_cmd_type == 2) // UV
	{
		/*if (ref_fw_frame_id == 0 && g_rv_decoder_ptr->uPicCodeType != INTERPIC)*/		//choose backward reference frame
		if(ref_fw_frame_id == 1)
		{
			pRefFrmU = g_rv_decoder_ptr->pBckRefFrame->pDecFrame->imgU;
			pRefFrmV = pRefFrmU + 1;	
		}else
		{
			pRefFrmU = g_rv_decoder_ptr->pFrdRefFrame->pDecFrame->imgU;
			pRefFrmV = pRefFrmU + 1;			
		}


		if(ref_blk_size == MCA_BLOCK8X8) ///1mv
		{
			if(ref_blk_id != 0)
			{
				assert(ref_blk_id == 0);
			}
			//u v

			start_x_c = mb_x * MB_CHROMA_SIZE  + ((mv_x ) >> 2);
			start_y_c = mb_y * MB_CHROMA_SIZE  + ((mv_y ) >> 2);
			
			dx = (mv_x ) & 3;
			dy = (mv_y ) & 3;
			
			frame_height = frame_height_c;
			
			for (uSub4x4BlkIdx = 0; uSub4x4BlkIdx < 4; uSub4x4BlkIdx ++)
			{				
				xoffset = (uSub4x4BlkIdx & 1) * 4;
				yoffset = (uSub4x4BlkIdx >> 1) * 4;

				start_x = start_x_c + xoffset;
				start_y = start_y_c + yoffset;
				// 4x4 blk of u
				pPred = fw_mca_bfr + MB_SIZE * MB_SIZE + yoffset * MB_CHROMA_SIZE + xoffset;
				g_chroma_mc[dx][dy](pRefFrmU, pPred, frame_width, frame_height, start_x, start_y, MB_CHROMA_SIZE);
				
				// 4x4 blk of v
				pPred = fw_mca_bfr + MB_SIZE * MB_SIZE + MB_CHROMA_SIZE * MB_CHROMA_SIZE + yoffset * MB_CHROMA_SIZE + xoffset;
				g_chroma_mc[dx][dy](pRefFrmV, pPred, frame_width, frame_height, start_x, start_y, MB_CHROMA_SIZE);
						
			}
		}else ///4mv
		{
			// u v
			xoffset = (ref_blk_id & 1) * 4;
			yoffset = (ref_blk_id >> 1) * 4;

			start_x = mb_x * MB_CHROMA_SIZE  + xoffset + ((mv_x ) >> 2);
			start_y = mb_y * MB_CHROMA_SIZE  + yoffset + ((mv_y ) >> 2);

			dx = (mv_x ) & 3;
			dy = (mv_y ) & 3;
			
			frame_height = frame_height_c;

			// 4x4 blk of u
			pPred = fw_mca_bfr + MB_SIZE * MB_SIZE + yoffset * MB_CHROMA_SIZE + xoffset;
			g_chroma_mc[dx][dy](pRefFrmU, pPred, frame_width, frame_height, start_x, start_y, MB_CHROMA_SIZE);
				
			// 4x4 blk of v
			pPred = fw_mca_bfr + MB_SIZE * MB_SIZE + MB_CHROMA_SIZE * MB_CHROMA_SIZE + yoffset * MB_CHROMA_SIZE + xoffset;
			g_chroma_mc[dx][dy](pRefFrmV, pPred, frame_width, frame_height, start_x, start_y, MB_CHROMA_SIZE);
		}
	}
		

	
	if(ref_bir_blk) //bi-dir
	{	
		if(ref_cmd_type == 0) //y and uv shared cmd
		{
			{
				pRefFrmY = g_rv_decoder_ptr->pBckRefFrame->pDecFrame->imgY;
				pRefFrmU = g_rv_decoder_ptr->pBckRefFrame->pDecFrame->imgU;
				pRefFrmV = pRefFrmU + 1;
			}

			if(ref_blk_size == MCA_BLOCK16X16) ///1mv
			{
				if(ref_blk_id != 0)
				{
					assert(ref_blk_id == 0);
				}
				//y 
				start_x = mb_x * MB_SIZE  + (mv_x_bck >>2) ;
				start_y = mb_y * MB_SIZE  + (mv_y_bck >>2) ;

				dx = mv_x_bck & 3;
				dy = mv_y_bck & 3;

				pPred = bw_mca_bfr;

				frame_height = frame_height_y;

				g_luma_mc[dx][dy](pRefFrmY, pPred, frame_width, frame_height, start_x, start_y, MB_SIZE); 

				//u v

				start_x_c = mb_x * MB_CHROMA_SIZE  + ((mv_x_bck /2) >> 2);
				start_y_c = mb_y * MB_CHROMA_SIZE  + ((mv_y_bck /2) >> 2);
				
				dx = (mv_x_bck /2) & 3;
				dy = (mv_y_bck /2) & 3;
				
				frame_height = frame_height_c;
				
				for (uSub4x4BlkIdx = 0; uSub4x4BlkIdx < 4; uSub4x4BlkIdx ++)
				{				
					xoffset = (uSub4x4BlkIdx & 1) * 4;
					yoffset = (uSub4x4BlkIdx >> 1) * 4;

					start_x = start_x_c + xoffset;
					start_y = start_y_c + yoffset;
					// 4x4 blk of u
					pPred = bw_mca_bfr + MB_SIZE * MB_SIZE + yoffset * MB_CHROMA_SIZE + xoffset;
					g_chroma_mc[dx][dy](pRefFrmU, pPred, frame_width, frame_height, start_x, start_y, MB_CHROMA_SIZE);
					
					// 4x4 blk of v
					pPred = bw_mca_bfr + MB_SIZE * MB_SIZE + MB_CHROMA_SIZE * MB_CHROMA_SIZE + yoffset * MB_CHROMA_SIZE + xoffset;
					g_chroma_mc[dx][dy](pRefFrmV, pPred, frame_width, frame_height, start_x, start_y, MB_CHROMA_SIZE);
							
				}
			}else ///4mv
			{
				//y 
				xoffset = (ref_blk_id & 1) * 8;
				yoffset = (ref_blk_id >> 1) * 8;

				start_x = mb_x * MB_SIZE  + xoffset + (mv_x_bck >> 2);
				start_y = mb_y * MB_SIZE  + yoffset + (mv_y_bck >> 2);					
						
				dx = mv_x_bck & 3;
				dy = mv_y_bck & 3;

				pPred = bw_mca_bfr + yoffset * MB_SIZE + xoffset;

				frame_height = frame_height_y;

				g_luma_mc[dx][dy](pRefFrmY, pPred, frame_width, frame_height, start_x, start_y, BLOCK_SIZE); 

				// u v
				xoffset = xoffset >> 1;
				yoffset = yoffset >> 1;

				start_x = mb_x * MB_CHROMA_SIZE  + xoffset + ((mv_x_bck /2) >> 2);
				start_y = mb_y * MB_CHROMA_SIZE  + yoffset + ((mv_y_bck /2) >> 2);

				dx = (mv_x_bck /2) & 3;
				dy = (mv_y_bck /2) & 3;
				
				frame_height = frame_height_c;

				// 4x4 blk of u
				pPred = bw_mca_bfr + MB_SIZE * MB_SIZE + yoffset * MB_CHROMA_SIZE + xoffset;
				g_chroma_mc[dx][dy](pRefFrmU, pPred, frame_width, frame_height, start_x, start_y, MB_CHROMA_SIZE);
					
				// 4x4 blk of v
				pPred = bw_mca_bfr + MB_SIZE * MB_SIZE + MB_CHROMA_SIZE * MB_CHROMA_SIZE + yoffset * MB_CHROMA_SIZE + xoffset;
				g_chroma_mc[dx][dy](pRefFrmV, pPred, frame_width, frame_height, start_x, start_y, MB_CHROMA_SIZE);
							
			}			
		}
		else if(ref_cmd_type == 1) // y
		{

			{
				pRefFrmY = g_rv_decoder_ptr->pBckRefFrame->pDecFrame->imgY;
			}

			if(ref_blk_size == MCA_BLOCK16X16) ///1mv
			{
				if(ref_blk_id != 0)
				{
					assert(ref_blk_id == 0);
				}
				//y 
				start_x = mb_x * MB_SIZE  + (mv_x_bck >>2) ;
				start_y = mb_y * MB_SIZE  + (mv_y_bck >>2) ;

				dx = mv_x_bck & 3;
				dy = mv_y_bck & 3;

				pPred = bw_mca_bfr;

				frame_height = frame_height_y;

				g_luma_mc[dx][dy](pRefFrmY, pPred, frame_width, frame_height, start_x, start_y, MB_SIZE); 
			}else ///4mv
			{
				//y 
				xoffset = (ref_blk_id & 1) * 8;
				yoffset = (ref_blk_id >> 1) * 8;

				start_x = mb_x * MB_SIZE  + xoffset + (mv_x_bck >> 2);
				start_y = mb_y * MB_SIZE  + yoffset + (mv_y_bck >> 2);					
						
				dx = mv_x_bck & 3;
				dy = mv_y_bck & 3;

				pPred = bw_mca_bfr + yoffset * MB_SIZE + xoffset;

				frame_height = frame_height_y;

				g_luma_mc[dx][dy](pRefFrmY, pPred, frame_width, frame_height, start_x, start_y, BLOCK_SIZE); 

			}
		}
		else if (ref_cmd_type == 2) // UV
		{
			{
				pRefFrmU = g_rv_decoder_ptr->pBckRefFrame->pDecFrame->imgU;
				pRefFrmV = pRefFrmU + 1;	
			}

			if(ref_blk_size == MCA_BLOCK8X8) ///1mv
			{
				if(ref_blk_id != 0)
				{
					assert(ref_blk_id == 0);
				}
				//u v

				start_x_c = mb_x * MB_CHROMA_SIZE  + ((mv_x_bck ) >> 2);
				start_y_c = mb_y * MB_CHROMA_SIZE  + ((mv_y_bck ) >> 2);
				
				dx = (mv_x_bck ) & 3;
				dy = (mv_y_bck ) & 3;
				
				frame_height = frame_height_c;
				
				for (uSub4x4BlkIdx = 0; uSub4x4BlkIdx < 4; uSub4x4BlkIdx ++)
				{				
					xoffset = (uSub4x4BlkIdx & 1) * 4;
					yoffset = (uSub4x4BlkIdx >> 1) * 4;

					start_x = start_x_c + xoffset;
					start_y = start_y_c + yoffset;
					// 4x4 blk of u
					pPred = bw_mca_bfr + MB_SIZE * MB_SIZE + yoffset * MB_CHROMA_SIZE + xoffset;
					g_chroma_mc[dx][dy](pRefFrmU, pPred, frame_width, frame_height, start_x, start_y, MB_CHROMA_SIZE);
					
					// 4x4 blk of v
					pPred = bw_mca_bfr + MB_SIZE * MB_SIZE + MB_CHROMA_SIZE * MB_CHROMA_SIZE + yoffset * MB_CHROMA_SIZE + xoffset;
					g_chroma_mc[dx][dy](pRefFrmV, pPred, frame_width, frame_height, start_x, start_y, MB_CHROMA_SIZE);
							
				}
			}else ///4mv
			{
				// u v
				xoffset = (ref_blk_id & 1) * 4;
				yoffset = (ref_blk_id >> 1) * 4;

				start_x = mb_x * MB_CHROMA_SIZE  + xoffset + ((mv_x_bck ) >> 2);
				start_y = mb_y * MB_CHROMA_SIZE  + yoffset + ((mv_y_bck ) >> 2);

				dx = (mv_x_bck ) & 3;
				dy = (mv_y_bck ) & 3;
				
				frame_height = frame_height_c;

				// 4x4 blk of u
				pPred = bw_mca_bfr + MB_SIZE * MB_SIZE + yoffset * MB_CHROMA_SIZE + xoffset;
				g_chroma_mc[dx][dy](pRefFrmU, pPred, frame_width, frame_height, start_x, start_y, MB_CHROMA_SIZE);
					
				// 4x4 blk of v
				pPred = bw_mca_bfr + MB_SIZE * MB_SIZE + MB_CHROMA_SIZE * MB_CHROMA_SIZE + yoffset * MB_CHROMA_SIZE + xoffset;
				g_chroma_mc[dx][dy](pRefFrmV, pPred, frame_width, frame_height, start_x, start_y, MB_CHROMA_SIZE);
			}
		}
	}
#endif
	return;
 
}



#ifdef VP8_DEC //weihu
PUBLIC void mca_module(MACROBLOCKD *xd)
#else
PUBLIC void mca_module()
#endif
{
	int32 mb_x; //should read out from global registor.
	int32 mb_y;
	int32 rounding_control;
	int32 frame_width, frame_height;
	//int32 int_type;
	uint8 *fw_mca_bfr = (uint8 *)vsp_fw_mca_out_bfr;
	uint8 *bw_mca_bfr = (uint8 *)vsp_bw_mca_out_bfr;
#ifdef VP8_DEC //weihu
	int32 standard	  = VSP_VP8;
#else
	int32 standard	  =(g_glb_reg_ptr->VSP_CFG0 >> 8) & 0xf;
#endif
	
	mb_x = g_glb_reg_ptr->VSP_CTRL0 & 0x7f;
	mb_y = ((g_glb_reg_ptr->VSP_CTRL0>>8) & 0x7f);
	
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
#ifdef VP8_DEC
#else
cmd_idx = 0;//weihu
#endif
	//if(mb_x == 6 && mb_y ==0 && g_nFrame_dec == 8)
	//	printf("");

	while(!ref_blk_end)
	{	
		parse_mca_cmd_pair();

		if ((standard == VSP_MPEG4) || (standard == ITU_H263) || (standard == FLV_H263))
		{
			mpeg4_h263_flv_motion_compensation (mb_x, mb_y);
		}else if (standard == VSP_H264)
		{
			h264_motion_compensation (mb_x, mb_y);
		}else if (standard == VSP_RV9 || standard == VSP_RV8)
		{
			rv9_motion_compensation (mb_x, mb_y);
		} 
		else if (standard == VSP_VP8)
		{
#ifdef VP8_DEC
			vp8_motion_compensation(mb_x, mb_y,xd);
#endif
		}else
		{
			printf("config error!\n");
		}
	}

	Mp4Dec_Trace_MCA_result_one_macroblock(fw_mca_bfr);
#ifdef VP8_DEC
	{
		if(!skip_flag)
		{		
			FILE *ref_fp;
			FILE *cmodel_fp;
			char a,b;
			int i;
			if((ref_fp = fopen("..\\..\\test_vectors\\ref_mca_out.log","r"))== NULL)
				exit(-1);
			if((cmodel_fp = fopen("..\\..\\test_vectors\\Cmodel_MCA_out.log","r"))== NULL)
				exit(-1);
			for(i = 0; i< 256 +128+96; i++)
			{
				a = getc(ref_fp);
				b = getc(cmodel_fp);
				if(a != b)
				{
					printf("Not MATCH!");
					exit(-1); //weihu
				}
			}
			fclose(ref_fp);
			fclose(cmodel_fp);
		}
	}
#endif

#if (!defined(MPEG4_LIB)) && defined(MPEG4_DEC)
	Mp4Dec_Trace_Interpolation_result_one_macroblock_forDebug (g_fp_trace_mca, fw_mca_bfr);

#endif //!defined(MPEG4_LIB)

#if defined(H264_DEC)
	ref_bir_blk = g_mbc_reg_ptr->H264_BIR_INFO & 0xf;
#endif

	if(ref_bir_blk) //bi-dir
	{
		Mp4Dec_Trace_MCA_result_one_macroblock(bw_mca_bfr);
#if (!defined(MPEG4_LIB)) && (defined(MPEG4_DEC)||defined(H264_DEC))
		Mp4Dec_Trace_Interpolation_result_one_macroblock_forDebug(g_fp_trace_mca, bw_mca_bfr);
#endif //!defined(MPEG4_LIB)

		if (standard == VSP_H264)
		{
			int32 h264_wp_ena = g_mbc_reg_ptr->MBC_CFG & 0x1;

			if (!h264_wp_ena)
			{
				uint32 b8;
				uint32 bi_dir;

				for (b8 = 0; b8 < 4; b8++)
				{
					bi_dir = ((ref_bir_blk>>b8)&0x1); 
					if (bi_dir)
					{
						h264_MC_GetAverage(b8);
					}
				}
			}else
			{
				h264_weighted_prediction();
			}
		}
		else
		{
			Mp4Dec_MC_GetAverage();
		}

	}
	else if(standard == VSP_H264)
	{
		int32 h264_wp_ena = g_mbc_reg_ptr->MBC_CFG & 0x1;

		if (h264_wp_ena)
		{
			h264_weighted_prediction();
		}
	}

	MCA_CMD_Num = 0;
}

#if defined(VP8_DEC)//weihu
void vp8_motion_compensation(int32 mb_x, int32 mb_y,MACROBLOCKD *xd)
{

	int32 start_pos_x;
	int32 start_pos_y;
	int32 blk_size;
	int16 mv_x_c,mv_y_c;
	uint8 *rec_blk_y_ptr, *rec_blk_u_ptr, *rec_blk_v_ptr;
	uint8 *fw_mca_bfr = (uint8 *)vsp_fw_mca_out_bfr;

	if(ref_cmd_type == 3)
	{
		start_pos_x = mb_x*MB_SIZE*MC_PRECISION_1_8+mv_x;
		start_pos_y = mb_y*MB_SIZE*MC_PRECISION_1_8+mv_y;
		if(ref_blk_size == MCA_BLOCK16X16)
		{	
			blk_size = MB_SIZE;
			rec_blk_y_ptr = fw_mca_bfr;

			vp8_mc (start_pos_x, start_pos_y, blk_size, rec_blk_y_ptr,int_type,xd);

			blk_size = MB_CHROMA_SIZE;
			rec_blk_u_ptr = fw_mca_bfr+MCA_U_OFFSET;
			rec_blk_v_ptr = fw_mca_bfr+MCA_V_OFFSET;
			mv_x_c = mv_x;
			mv_y_c = mv_y;
			 if (mv_y < 0)
		            mv_y_c -= 1;
		        else
		            mv_y_c += 1;

		        if (mv_x < 0)
		            mv_x_c -= 1;
		        else
		            mv_x_c += 1;

		        mv_x_c /= 2;
		        mv_y_c /= 2;

			start_pos_x = mb_x * MB_CHROMA_SIZE * MC_PRECISION_1_8 + mv_x_c;
			start_pos_y = mb_y * MB_CHROMA_SIZE* MC_PRECISION_1_8 + mv_y_c;
			
			vp8_mc_u (start_pos_x, start_pos_y, blk_size, rec_blk_u_ptr,int_type,xd);	
			vp8_mc_v (start_pos_x, start_pos_y, blk_size, rec_blk_v_ptr,int_type,xd);
		}else if(ref_blk_size == MCA_BLOCK8X8)
		{
			int32 i, j;
			int32 b8_luma_offset[4] = {0, 8, 8*16, 8*16+8};
			int32 b8_chroma_offset[4] = {0, 4, 4*8, 4*8+4};

			i = ref_blk_id%2;
			j = ref_blk_id/2;

			start_pos_x += (i*BLOCK_SIZE*MC_PRECISION_1_8);
			start_pos_y += (j*BLOCK_SIZE*MC_PRECISION_1_8);

			blk_size = MB_SIZE/2;
			rec_blk_y_ptr = fw_mca_bfr + b8_luma_offset[ref_blk_id];
			vp8_mc (start_pos_x, start_pos_y, blk_size, rec_blk_y_ptr,int_type,xd);

			blk_size = MB_CHROMA_SIZE/2;
			mv_x_c = mv_x;
			mv_y_c = mv_y;
			 if (mv_y < 0)
		            mv_y_c -= 1;
		        else
		            mv_y_c += 1;

		        if (mv_x < 0)
		            mv_x_c -= 1;
		        else
		            mv_x_c += 1;

		        mv_x_c /= 2;
		        mv_y_c /= 2;

			start_pos_x = mb_x * MB_CHROMA_SIZE * MC_PRECISION_1_8 + mv_x_c;
			start_pos_y = mb_y * MB_CHROMA_SIZE * MC_PRECISION_1_8 + mv_y_c;

			start_pos_x += (i*CHROMA_BLOCK*MC_PRECISION_1_8);
			start_pos_y += (j*CHROMA_BLOCK*MC_PRECISION_1_8);

			rec_blk_u_ptr = fw_mca_bfr+MCA_U_OFFSET+b8_chroma_offset[ref_blk_id];
			rec_blk_v_ptr = fw_mca_bfr+MCA_V_OFFSET+b8_chroma_offset[ref_blk_id];
			vp8_mc_u (start_pos_x, start_pos_y, blk_size, rec_blk_u_ptr,int_type,xd);	
			vp8_mc_v (start_pos_x, start_pos_y, blk_size, rec_blk_v_ptr,int_type,xd);	
		}else if(ref_blk_size == MCA_BLOCK4X4)
		{
			int32 i, j;
			///0, 1,    4,  5			//mca out buffer is configed using different setting in different block mode
			///2, 3,    6,  7
			///8, 9,   12, 13
			///10, 11, 14, 15
/*			int32 sub4x4_luma_offset[16] = 
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
*/
			int32 sub4x4_luma_offset[16] =
			{
				0,		4,		8,		12,
				16*4,	16*4 +4,	16*4 + 8,16*4 +12,
				16*8,	16*8 +4, 16*8 +8,	16*8 +12,
				16*12,	16 *12+4,16*12+8,16*12+12
			};
			int32 b8_chroma_offset[4] = {0, 4, 4*8, 4*8+4};
			
			int32 i_idx[16] = {0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3};
			int32 j_idx[16] = {0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3};

			i = i_idx[ref_blk_id];
			j = j_idx[ref_blk_id];

			start_pos_x += (i*CHROMA_BLOCK*MC_PRECISION_1_8);
			start_pos_y += (j*CHROMA_BLOCK*MC_PRECISION_1_8);

			blk_size = MB_SIZE/4;
			rec_blk_y_ptr = fw_mca_bfr + sub4x4_luma_offset[ref_blk_id];
			vp8_mc (start_pos_x, start_pos_y, blk_size, rec_blk_y_ptr,int_type,xd);

			if(!ref_blk_id%4)//chroma mv referenced from xd structure for convenient reason, may be calculated in MCA model later JIN 20120109
				{
				start_pos_x = mb_x*MB_CHROMA_SIZE*MC_PRECISION_1_8+xd->block[16+ref_blk_id/4].bmi.mv.as_mv.col;
				start_pos_y = mb_y*MB_CHROMA_SIZE*MC_PRECISION_1_8+xd->block[16+ref_blk_id/4].bmi.mv.as_mv.row;
				
				start_pos_x += (i*CHROMA_BLOCK*MC_PRECISION_1_8);
				start_pos_y += (j*CHROMA_BLOCK*MC_PRECISION_1_8);

				blk_size = MB_CHROMA_SIZE/2;
				rec_blk_u_ptr = fw_mca_bfr+MCA_U_OFFSET+b8_chroma_offset[ref_blk_id/4];
				rec_blk_v_ptr = fw_mca_bfr+MCA_V_OFFSET+b8_chroma_offset[ref_blk_id/4];
				vp8_mc_u (start_pos_x, start_pos_y, blk_size, rec_blk_u_ptr,int_type,xd);	
				vp8_mc_v (start_pos_x, start_pos_y, blk_size, rec_blk_v_ptr,int_type,xd);	
				}
		}
	}

}
#endif

int32 b4order[16]=
{
	0, 1, 4, 5,
		2, 3, 6, 7, 
		8, 9, 12, 13,
		10, 11, 14, 15,
};

#ifdef H264_MCA
void mca_module_ppa (char  mca_out_buf[384],
					 int slice_info[40],
					 int mca_para_buf[38], //38*26b
					 char decoder_format,//3b
					 char picwidthinMB,//7b
					 char picheightinMB//7b					 
					 )//weihu
{
	int i,j,blk4,blk8;
	char mb_type_mca,is_intra,cur_mb_x,cur_mb_y;//17b
	char predflag[4][2];//[blk][reflist]//1b
	int w[4][2];//weight[blk][reflist]//9b
    char sub_mb_type_mca[4];//2b
	int mv_x[2][16],mv_y[2][16];//14b/12b
	char ref_idx[2][4];//5b  //2*16

	int apply_weights;
	int weighted_pred_flag,weighted_bipred_idc,slice_type,luma_log2_weight_denom,chroma_log2_weight_denom; 

	int blk_end,bir_blk,ref_id[2], blk_id,blk_size;
	uint32 MCA_BLK_CBUF_reg;
	uint32 mv_reg0,mv_reg1;
	uint32	*	src_ptr;
	uint32	*	dst_ptr;


	uint32 bir_cmd = 0;
	uint32 wp_cmd[16];
	int32 blk4x4_id;
	uint32 w128_flag;
	int32 is_same_para = 0x7; //y, u, v 
	int32 is_inter16x16=1;
	int32 is_inter8x8[4];
	int32 b8;
	int32 comp;
	uint32 cmd_info;
	int mbc_cfg_cmd;

    slice_type= slice_info[0];
	weighted_pred_flag=slice_info[32];
	weighted_bipred_idc=slice_info[9];
	luma_log2_weight_denom=slice_info[33]; 
	chroma_log2_weight_denom=slice_info[34];

	mb_type_mca=(mca_para_buf[0]>>15)&0x3;
    is_intra=(mca_para_buf[0]>>14)&0x1;
    cur_mb_x=(mca_para_buf[0]>>7)&0x7f;
    cur_mb_y=mca_para_buf[0]&0x7f;

	FPRINTF (pFmoFile, "frame: %d, mb_y: %d, mb_x: %d \n", g_nFrame_dec_h264, cur_mb_y, cur_mb_x);//weihu
	for(i=0;i<4;i++)
	{
		predflag[i][0]=(mca_para_buf[1]>>(8+i))&0x1;
		predflag[i][1]=(mca_para_buf[1]>>(12+i))&0x1;
		sub_mb_type_mca[i]=(mca_para_buf[1]>>(2*i))&0x3;
	}
	for(i=0;i<2;i++)
	{
		ref_idx[i][0]=predflag[0][i] ? mca_para_buf[2+18*i]&0xf : -1 ;
        ref_idx[i][1]=predflag[1][i] ? (mca_para_buf[2+18*i]>>4)&0xf : -1;
		ref_idx[i][2]=predflag[2][i] ? mca_para_buf[3+18*i]&0xf : -1;
        ref_idx[i][3]=predflag[3][i] ? (mca_para_buf[3+18*i]>>4)&0xf : -1;
		w[0][i]=(mca_para_buf[2+18*i]>>8)&0x1ff;
		w[1][i]=(mca_para_buf[2+18*i]>>17)&0x1ff;
		w[2][i]=(mca_para_buf[3+18*i]>>8)&0x1ff;
		w[3][i]=(mca_para_buf[3+18*i]>>17)&0x1ff;
	}
		 
    for(i=0;i<16;i++)
	{
        mv_x[0][i]=(mca_para_buf[4+i]>>25)&0x1 ? ((mca_para_buf[4+i]>>12)&0x3fff)|0xffffc000: (mca_para_buf[4+i]>>12)&0x3fff;
		mv_y[0][i]=(mca_para_buf[4+i]>>11)&0x1 ? ((mca_para_buf[4+i])&0xfff)|0xfffff000 :(mca_para_buf[4+i])&0xfff;
		mv_x[1][i]=(mca_para_buf[22+i]>>25)&0x1 ? ((mca_para_buf[22+i]>>12)&0x3fff)|0xffffc000: (mca_para_buf[22+i]>>12)&0x3fff;
		mv_y[1][i]=(mca_para_buf[22+i]>>11)&0x1 ? ((mca_para_buf[22+i])&0xfff)|0xfffff000 : (mca_para_buf[22+i])&0xfff;
	}

    is_inter16x16=!is_intra;
	for(i=0;i<3;i++)
	{
		if((ref_idx[0][i]!=ref_idx[0][i+1])||(ref_idx[1][i]!=ref_idx[1][i+1]))
			is_inter16x16=0;
	}
	if(is_inter16x16)
	{
		for(i=0;i<15;i++)
		{
			if((mv_x[0][i]!=mv_x[0][i+1])||(mv_y[0][i]!=mv_y[0][i+1])||(mv_x[1][i]!=mv_x[1][i+1])||(mv_y[1][i]!=mv_y[1][i+1]))
				is_inter16x16=0;
			
		}
	}
    if(is_inter16x16&&(slice_info[3]==0))
          mb_type_mca=0;

    for(j=0;j<4;j++)
	{
	    is_inter8x8[j]=!is_intra;
		for(i=4*j+0;i<3+j*4;i++)
		{
			if((mv_x[0][i]!=mv_x[0][i+1])||(mv_y[0][i]!=mv_y[0][i+1])||(mv_x[1][i]!=mv_x[1][i+1])||(mv_y[1][i]!=mv_y[1][i+1]))
				is_inter8x8[j]=0;
			
		}
		if(is_inter8x8[j]&&(slice_info[3]==1))
		     sub_mb_type_mca[j]=0;
	}
	
	blk_end=0;
	if (is_intra) 
	{
	}
	else 
	{ 
		while(!blk_end)
		{
			if (mb_type_mca==0)//16x16 
			{
				blk_end=1;
				blk_size=MC_BLKSIZE_16x16;
				blk_id=0;
				bir_blk=0;
			
							
				if (predflag[0][0]) 
				{
					ref_id[0]=ref_idx[0][blk_id];
					mv_reg0 = (mv_y[0][0] << 16) | (mv_x[0][0] & 0xffff);
					if(predflag[0][1])
					{
						mv_reg1 = (mv_y[1][0] << 16) | (mv_x[1][0] & 0xffff);
						bir_blk=1;
						ref_id[1]=g_list1_map_list0[ref_idx[1][blk_id]];
					}
					else
						ref_id[1]=0;
				}
				else if (predflag[0][1]) 
				{
					mv_reg0 = (mv_y[1][0] << 16) | (mv_x[1][0] & 0xffff);
					ref_id[0]=g_list1_map_list0[ref_idx[1][blk_id]];
					ref_id[1]=0;
				}
				

				MCA_BLK_CBUF_reg = (blk_end << 27) | (bir_blk << 26) | ((ref_id[1]&0xf) << 20)|	((ref_id[0]&0xf) << 16) | (blk_id << 8) | (blk_size & 0xff);
				VSP_WRITE_REG(VSP_MCA_REG_BASE+MCA_BLK_CBUF_OFF, MCA_BLK_CBUF_reg, "configure MCA command buffer, block info");
				VSP_WRITE_REG(VSP_MCA_REG_BASE+MCA_MV_CBUF_OFF, mv_reg0, "configure MCA command buffer, block mv info");	
			//	FPRINTF (pFmoFile, "MCA_BLK_CBUF: %x %d %d\n", MCA_BLK_CBUF_reg, blk_id,blk_size);//weihu
			//	FPRINTF (pFmoFile, "MCA_MV_CBUF0: %x \n", mv_reg0);//weihu
				if(bir_blk)
				{
			//	    FPRINTF (pFmoFile, "MCA_MV_CBUF1: %x \n", mv_reg1);//weihu
					VSP_WRITE_REG(VSP_MCA_REG_BASE+MCA_MV_CBUF_OFF, mv_reg1, "configure MCA command buffer, block mv info");	
				}
			}
			else if (mb_type_mca!=3)//16x8 or 8X16
			{
				blk_size=MC_BLKSIZE_8x8;
		
				
				for(i=0;i<4;i++)
				{
					bir_blk=0;
					if(mb_type_mca==1)//16x8
					    blk_id=i;
					else
						blk_id=((i&0x1)<<1)+(i>>1);
					blk8=blk_id;
					blk4=4*blk_id;
					if (blk_id==3)
						blk_end=1;
					
				
					if (predflag[blk8][0]) 
					{
						ref_id[0]=ref_idx[0][blk8];
						mv_reg0 = (mv_y[0][blk4] << 16) | (mv_x[0][blk4] & 0xffff);
						if(predflag[blk8][1])
						{
							mv_reg1 = (mv_y[1][blk4] << 16) | (mv_x[1][blk4] & 0xffff);
							bir_blk=1;
							ref_id[1]=g_list1_map_list0[ref_idx[1][blk8]];
						}
						else
							ref_id[1]=0;
					}
					else if (predflag[blk8][1]) 
					{
						mv_reg0 = (mv_y[1][blk4] << 16) | (mv_x[1][blk4] & 0xffff);
						ref_id[0]=g_list1_map_list0[ref_idx[1][blk8]];
						ref_id[1]=0;
					}
                  
					MCA_BLK_CBUF_reg = (blk_end << 27) | (bir_blk << 26) | ((ref_id[1]&0xf) << 20)|	((ref_id[0]&0xf) << 16) | (blk_id << 8) | (blk_size & 0xff);
					VSP_WRITE_REG(VSP_MCA_REG_BASE+MCA_BLK_CBUF_OFF, MCA_BLK_CBUF_reg, "configure MCA command buffer, block info");
					VSP_WRITE_REG(VSP_MCA_REG_BASE+MCA_MV_CBUF_OFF, mv_reg0, "configure MCA command buffer, block mv info");	
			//		FPRINTF (pFmoFile, "MCA_BLK_CBUF: %x %d %d\n", MCA_BLK_CBUF_reg, blk_id,blk_size);//weihu
			//		FPRINTF (pFmoFile, "MCA_MV_CBUF0: %x \n", mv_reg0);//weihu
					if(bir_blk)
					{
			//			FPRINTF (pFmoFile, "MCA_MV_CBUF1: %x \n", mv_reg1);//weihu
						VSP_WRITE_REG(VSP_MCA_REG_BASE+MCA_MV_CBUF_OFF, mv_reg1, "configure MCA command buffer, block mv info");	
					}
				}

			}
			else //if (mb_type_mca==3) 
			{
				for(i=0;i<4;i++)//8x8
				{
					if(sub_mb_type_mca[i]==0)
					{
						blk_size=MC_BLKSIZE_8x8;
                        bir_blk=0;
						blk_id=i;
						blk8=i;
						blk4=4*i;
						if (i==3)
							blk_end=1;
						
					
						if (predflag[blk8][0]) 
						{
							ref_id[0]=ref_idx[0][blk8];
							mv_reg0 = (mv_y[0][blk4] << 16) | (mv_x[0][blk4] & 0xffff);
							if(predflag[blk8][1])
							{
								mv_reg1 = (mv_y[1][blk4] << 16) | (mv_x[1][blk4] & 0xffff);
								bir_blk=1;
								ref_id[1]=g_list1_map_list0[ref_idx[1][blk8]];
							}
							else
								ref_id[1]=0;
						}
						else if (predflag[blk8][1]) 
						{
							mv_reg0 = (mv_y[1][blk4] << 16) | (mv_x[1][blk4] & 0xffff);
							ref_id[0]=g_list1_map_list0[ref_idx[1][blk8]];
							ref_id[1]=0;
						}
						
						MCA_BLK_CBUF_reg = (blk_end << 27) | (bir_blk << 26) | ((ref_id[1]&0xf) << 20)|	((ref_id[0]&0xf) << 16) | (blk_id << 8) | (blk_size & 0xff);
						VSP_WRITE_REG(VSP_MCA_REG_BASE+MCA_BLK_CBUF_OFF, MCA_BLK_CBUF_reg, "configure MCA command buffer, block info");
						VSP_WRITE_REG(VSP_MCA_REG_BASE+MCA_MV_CBUF_OFF, mv_reg0, "configure MCA command buffer, block mv info");	
				//		FPRINTF (pFmoFile, "MCA_BLK_CBUF: %x %d %d\n", MCA_BLK_CBUF_reg, blk_id,blk_size);//weihu
				//		FPRINTF (pFmoFile, "MCA_MV_CBUF0: %x \n", mv_reg0);//weihu
						if(bir_blk)
						{
				//			FPRINTF (pFmoFile, "MCA_MV_CBUF1: %x \n", mv_reg1);//weihu
							VSP_WRITE_REG(VSP_MCA_REG_BASE+MCA_MV_CBUF_OFF, mv_reg1, "configure MCA command buffer, block mv info");	
						}


					}
					else //if(sub_mb_type_mca==1)//8x4 4x8 4x4
					{
						bir_blk=0;
						blk8=i;
						
					
						for(j=0;j<4;j++)//4x4
						{
							blk_size=MC_BLKSIZE_4x4;
							if (sub_mb_type_mca[i]==2)
							   blk4=4*i+(j>>1)+((j&0x1)<<1);
							else
                               blk4=4*i+j;
							blk_id=blk4;
							if ((i==3)&&(j==3))
								blk_end=1;
							if (predflag[blk8][0]) 
							{
								ref_id[0]=ref_idx[0][blk8];
								mv_reg0 = (mv_y[0][blk4] << 16) | (mv_x[0][blk4] & 0xffff);
								if(predflag[blk8][1])
								{
									mv_reg1 = (mv_y[1][blk4] << 16) | (mv_x[1][blk4] & 0xffff);
									bir_blk=1;
									ref_id[1]=g_list1_map_list0[ref_idx[1][blk8]];
								}
								else
									ref_id[1]=0;
							}
							else if (predflag[blk8][1]) 
							{
								mv_reg0 = (mv_y[1][blk4] << 16) | (mv_x[1][blk4] & 0xffff);
								ref_id[0]=g_list1_map_list0[ref_idx[1][blk8]];
								ref_id[1]=0;
							}
							
							MCA_BLK_CBUF_reg = (blk_end << 27) | (bir_blk << 26) | ((ref_id[1]&0xf) << 20)|	((ref_id[0]&0xf) << 16) | (blk_id << 8) | (blk_size & 0xff);
							VSP_WRITE_REG(VSP_MCA_REG_BASE+MCA_BLK_CBUF_OFF, MCA_BLK_CBUF_reg, "configure MCA command buffer, block info");
							VSP_WRITE_REG(VSP_MCA_REG_BASE+MCA_MV_CBUF_OFF, mv_reg0, "configure MCA command buffer, block mv info");	
				//			FPRINTF (pFmoFile, "MCA_BLK_CBUF: %x %d %d\n", MCA_BLK_CBUF_reg, blk_id,blk_size);//weihu
				//			FPRINTF (pFmoFile, "MCA_MV_CBUF0: %x \n", mv_reg0);//weihu
							if(bir_blk)
							{
				//				FPRINTF (pFmoFile, "MCA_MV_CBUF1: %x \n", mv_reg1);//weihu
								VSP_WRITE_REG(VSP_MCA_REG_BASE+MCA_MV_CBUF_OFF, mv_reg1, "configure MCA command buffer, block mv info");	
							}
						}

					}
				

				}//for(i=0;i<4;i++)//8x8


			}//if (mb_type_mca==3) 

		}//while(!blk_end)



		apply_weights = (weighted_pred_flag && (slice_type == P_SLICE ))|| ((weighted_bipred_idc > 0 ) && (slice_type == B_SLICE));
 
		if(apply_weights)
		{
			//	VSP_WRITE_REG(VSP_MBC_REG_BASE+MBC_H264_WPBUF_SW_OFF, 0, "write 0: SW can access this buffer");

				for (comp = 0; comp < 3; comp++)
				{
								
					w128_flag = 0;
					blk4x4_id = 0;
					
					for (b8 = 0; b8 < 4; b8++)
					{
						int32 mc_ref_idx_fw, mc_ref_idx_bw;
						mc_ref_idx_fw = ref_idx[0][b8]; 
						mc_ref_idx_bw = ref_idx[1][b8]; 
						
						for (j = 0; j < 4; j++)
						{
							int32 offset0, offset1 = 0;
							int32 weight0, weight1 = 0;
							
							blk4x4_id=b4order[b8*4+j];
										
							if (predflag[b8][1]&&predflag[b8][0])//bi-dir
							{
								if ((weighted_bipred_idc==2) &&(slice_type == B_SLICE ))
								{
									offset0 = 0;
									offset1 = 0;
									weight0 = w[b8][0];
									weight1 = w[b8][1];
								}
								else
								{
									offset0 = g_wp_offset[0][mc_ref_idx_fw][comp];
									offset1 = g_wp_offset[1][mc_ref_idx_bw][comp];
									weight0 = g_wbp_weight[0][mc_ref_idx_fw][mc_ref_idx_bw][comp];
									weight1 = g_wbp_weight[1][mc_ref_idx_fw][mc_ref_idx_bw][comp];
								}
								

								if (weight0 == 128)
								{
									w128_flag |= (1<<blk4x4_id);
								}

								if (weight1 == 128)
								{
									w128_flag |= (1<<(blk4x4_id+16));
								}

						
								wp_cmd[blk4x4_id] = ((offset1&0xff)<<24) | ((offset0&0xff)<<16) | ((weight1&0xff)<<8) | (weight0&0xff);//lsw for debug 8 bits is not enough for weight since it can be 128 
							}else //one-dir
							{
								if ((weighted_bipred_idc==2) &&(slice_type == B_SLICE ))
								{
									offset0 = 0;
									offset1 = 0;
									weight0 = w[b8][0];
									weight1 = w[b8][1];
								}
								else
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
								}

								if (weight0 == 128)
								{
									w128_flag |= (1<<blk4x4_id);
								}
										
								wp_cmd[blk4x4_id] = ((offset1&0xff)<<24) | ((offset0&0xff)<<16) | ((weight1&0xff)<<8) | (weight0&0xff);
							}

							
						}
						
					
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
		//				FPRINTF (pFmoFile, "MBC_HY_WP_BLK: %x \n", wp_cmd[0]);//weihu
						VSP_WRITE_REG(VSP_MBC_REG_BASE+MBC_HY_WP_BLK_OFF+0*4+comp*4*16, wp_cmd[0], "configure MBC wp info");

					}else
					{
							
						for (blk4x4_id = 0; blk4x4_id < 16; blk4x4_id++)
						{
		//					FPRINTF (pFmoFile, "MBC_HY_WP_BLK %d : %x \n", blk4x4_id,wp_cmd[blk4x4_id]);//weihu
							VSP_WRITE_REG(VSP_MBC_REG_BASE+MBC_HY_WP_BLK_OFF+blk4x4_id*4+comp*4*16, wp_cmd[blk4x4_id], "configure MBC wp info");
						}
						
					
					}
					
		//			FPRINTF (pFmoFile, "MBC_HY_weight128: %x \n", w128_flag);//weihu
					VSP_WRITE_REG(VSP_MBC_REG_BASE+MBC_HY_weight128_OFF+comp*4, w128_flag, "configure MBC weight128 flag, Y");
					
				}

			//	VSP_WRITE_REG(VSP_MBC_REG_BASE+MBC_H264_WPBUF_SW_OFF, 1, "write 1: HW can access this buffer");
		}//if(apply_weights)

		
		if (apply_weights)
		{
		  mbc_cfg_cmd = (luma_log2_weight_denom<<4) | (chroma_log2_weight_denom<<1) | 1;
		  mbc_cfg_cmd &= 0xffff;
		  mbc_cfg_cmd |= (is_same_para<<16);
		}else
		{
		  mbc_cfg_cmd = 0;
		}
		
     //   FPRINTF (pFmoFile, "MBC_CFG: %x \n", mbc_cfg_cmd);//weihu
	      VSP_WRITE_REG(VSP_MBC_REG_BASE+MBC_CFG_OFF, mbc_cfg_cmd, "MBC weighted prediction ON/OFF and parameter");
		
		{
			uint32 cmd;
			
			cmd = ((predflag[3][1]&&predflag[3][0])<<3) | ((predflag[2][1]&&predflag[2][0])<<2) |((predflag[1][1]&&predflag[1][0])<<1) | ((predflag[0][1]&&predflag[0][0])<<0);
	       VSP_WRITE_REG(VSP_MBC_REG_BASE+MBC_H264_BIR_INFO_OFF, cmd, "MBC_H264_RND_CTRL: config h.264 mbc b8 bi- or not");
	//	   FPRINTF (pFmoFile, "MBC_H264_BIR_INFO: %x \n", cmd);//weihu	
		}
	
		mca_module();

		src_ptr = vsp_fw_mca_out_bfr;
		dst_ptr = &(mca_out_buf[0]);
		for (i = 0; i < 96; i++)
		{
			dst_ptr[i] = src_ptr[i];
		}
	}//if(is_intra)
	
	
   
   

}
#endif
/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End 

