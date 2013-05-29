/******************************************************************************
 ** File Name:    mbc_top.c		                                              *
 ** Author:       Xiaowei Luo                                                 *
 ** DATE:         12/14/2006                                                  *
 ** Copyright:    2006 Spreatrum, Incoporated. All Rights Reserved.           *
 ** Description:                                                              *
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
#include "tv_mea.h"
/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    extern   "C" 
    {
#endif

DOWN_SAMPLE		g_downsample[3];
int32 mbc_valid_bfr_id;

int32 g_blk_rec_ord_tbl [16] = 
{
	0,  1,  4, 5, 
	2,  3,  6, 7, 
	8,  9, 12, 13,
	10, 11, 14, 15
};


void init_mbc_module(void)
{
	g_mbc_reg_ptr->MBC_ST0 = 0; 
	mbc_valid_bfr_id = 0;
	Init_downsample_fun();


	init_ipred();
}

void PutRec2Frm ()
{
#if defined(H264_DEC)
	int32 i, j;
	int32 mb_x = ((g_glb_reg_ptr->VSP_CTRL0>>0) & 0x7f);//weihu //0x3f
	int32 mb_y = ((g_glb_reg_ptr->VSP_CTRL0>>8) & 0x7f);
	int32 mb_num_x	= (g_glb_reg_ptr->VSP_CFG1 >> 0) & 0xff;
	int32 frame_width = mb_num_x*MB_SIZE;
	int32 mb_luma_offset = (mb_y * MB_SIZE) * frame_width + mb_x * MB_SIZE;
	int32 mb_chroma_offset = (mb_y * BLOCK_SIZE) * frame_width + mb_x * MB_SIZE;
	uint8 *rec_mb_y, *rec_mb_u, *rec_mb_v;
	int32 frm_width = frame_width;
	uint8* rec_frame_y_ptr = g_dec_picture_ptr->imgY;
	uint8* rec_frame_uv_ptr = g_dec_picture_ptr->imgU; //uv interleaved
	uint8* mbc_bfr_ptr = (uint8 *)vsp_mbc_out_bfr;
	uint8* mbc_ptr;

	rec_mb_y = rec_frame_y_ptr + mb_luma_offset;
	rec_mb_u = rec_frame_uv_ptr + mb_chroma_offset;
	rec_mb_v = rec_frame_uv_ptr + mb_chroma_offset+1;

	//y
	mbc_ptr = mbc_bfr_ptr + 26*4;
	for (i = 0; i < MB_SIZE; i++)
	{
		for (j = 0; j < MB_SIZE; j++)
		{
			rec_mb_y[i*frm_width +j] = mbc_ptr[i*MBC_Y_SIZE+j];
		}		
	}

	//u
	mbc_ptr = mbc_bfr_ptr + MBC_U_OFFSET+ 18*4;
	for (i = 0; i < MB_CHROMA_SIZE; i++)
	{
		for (j = 0; j < MB_CHROMA_SIZE; j++)
		{
			rec_mb_u[i*frm_width +2*j] = mbc_ptr[i*MBC_C_SIZE+j];
		}		
	}

	//v
	mbc_ptr = mbc_bfr_ptr + MBC_V_OFFSET+ 18*4;
	for (i = 0; i < MB_CHROMA_SIZE; i++)
	{
		for (j = 0; j < MB_CHROMA_SIZE; j++)
		{
			rec_mb_v[i*frm_width +2*j] = mbc_ptr[i*MBC_C_SIZE+j];
		}		
	}
#endif
	return;
}

void update_top_line_bfr ()
{ 
#if defined(H264_DEC)||defined(H264_ENC)
	int32 mb_x = ((g_glb_reg_ptr->VSP_CTRL0>>0) & 0x7f);
	int32 mb_num_x	= (g_glb_reg_ptr->VSP_CFG1 >> 0) & 0xff;
	uint8 *topBorder_luma, *topBorder_chroma[2];
	uint8* mbc_bfr_ptr = (uint8 *)vsp_mbc_out_bfr;
	uint8 *pred;
	int32 i;

#if defined(H264_DEC)
	topBorder_luma = g_image_ptr->ipred_top_line_buffer + mb_x * MB_SIZE;
	topBorder_chroma[0] = g_image_ptr->ipred_top_line_buffer + mb_num_x * MB_SIZE + mb_x * MB_CHROMA_SIZE;
	topBorder_chroma[1] = g_image_ptr->ipred_top_line_buffer + mb_num_x * (MB_SIZE+MB_CHROMA_SIZE) + mb_x * MB_CHROMA_SIZE;
#else //ENC
	topBorder_luma = g_enc_image_ptr->ipred_top_line_buffer + mb_x * MB_SIZE;
	topBorder_chroma[0] = g_enc_image_ptr->ipred_top_line_buffer + mb_num_x * MB_SIZE + mb_x * MB_CHROMA_SIZE;
	topBorder_chroma[1] = g_enc_image_ptr->ipred_top_line_buffer + mb_num_x * (MB_SIZE+MB_CHROMA_SIZE) + mb_x * MB_CHROMA_SIZE;
#endif

	//update top left pixel
	g_top_left[0] = topBorder_luma[MB_SIZE-1];
	g_top_left[1] = topBorder_chroma[0][MB_CHROMA_SIZE-1];
	g_top_left[2] = topBorder_chroma[1][MB_CHROMA_SIZE-1];
				
	//update ipred_top_line_bfr 
	pred = mbc_bfr_ptr + MBC_U_OFFSET - MB_SIZE;
	for (i = 0; i < MB_SIZE; i++) //luma
	{
		topBorder_luma[i] = pred[i];
	}
				
	pred = mbc_bfr_ptr + MBC_V_OFFSET - MB_CHROMA_SIZE;		
	for (i = 0; i < MB_CHROMA_SIZE; i++) //u
	{
		topBorder_chroma[0][i] = pred[i];
	}
				
	pred = mbc_bfr_ptr + MBC_V_OFFSET + 16*12/*12*12*/ - MB_CHROMA_SIZE;
	for (i = 0; i < MB_CHROMA_SIZE; i++) //v
	{
		topBorder_chroma[1][i] = pred[i];
	}
				
	//update ipred left_bfr
	pred = mbc_bfr_ptr + MBC_Y_SIZE*5 - 1;
	for (i = 0; i < MB_SIZE; i++) //luma
	{
		g_luma_left[i] = pred[i*MBC_Y_SIZE];
	}
				
	pred = mbc_bfr_ptr + MBC_U_OFFSET + MBC_C_SIZE * 5 - 1;
	for (i = 0; i < MB_CHROMA_SIZE; i++) //u
	{
		g_chroma_left[0][i] = pred[i*MBC_C_SIZE];
	}
				
	pred = mbc_bfr_ptr + MBC_V_OFFSET + MBC_C_SIZE * 5 - 1;
	for (i = 0; i < MB_CHROMA_SIZE; i++) //v
	{
		g_chroma_left[1][i] = pred[i*MBC_C_SIZE];
	}	
#endif
	return;
}

/***************************************************
description:
1: add reference and residual
2. out reconstructed MB to frame buffer
****************************************************/
void mbc_module(void)
{
	int standard;
	int is_encoder;

	/*register file*/
	standard		= (g_glb_reg_ptr->VSP_CFG0 >> 8) & 0x7;
	is_encoder = (g_glb_reg_ptr->VSP_CFG0 >> 14) & 0x1;

	if(VSP_JPEG != standard)
	{
		uint32 *tmp_ptr;
		int32 eob_cfg_flag = (g_mbc_reg_ptr->MBC_ST0 >> 6) & 0x01;

		if(!eob_cfg_flag)
		{
			if (standard == VSP_H264)
			{
				int32 mb_type = ((g_mbc_reg_ptr->MBC_CMD0>>30)&0x1);

			#if defined(H264_DEC)
				if (mb_type == 0) //intra mb
				{
					ipred_module();
				}else //inter mb
				{
					h264_add_ref_and_residual();
				}
			#else
				h264_add_ref_and_residual();
			#endif

				update_top_line_bfr ();

				//put to rec frame.
			//	PutRec2Frm ();
			}else
			{
				mp4_add_ref_and_residual();
			}			
		}

#ifdef TV_OUT
#ifdef ONE_FRAME
		if(g_nFrame_enc>FRAME_X)
#endif
		{
			uint32 mb_type = ((g_mbc_reg_ptr->MBC_CMD0>>30)&0x1);
			uint32 intra_44 = ((g_mbc_reg_ptr->MBC_CMD0>>28)&0x3);
			intra_44 = ((mb_type==0)&&(intra_44==1))?1:0;
			PrintfRecPreFilter(intra_44);
		}
#endif
#if !defined(MPEG4_LIB)
		printf_mbc_mb(g_fp_mbc_tv);
#endif
		/*switch two dbk pointer*/
		tmp_ptr        = vsp_mbc_out_bfr;
		vsp_mbc_out_bfr = vsp_dbk_out_bfr;
		vsp_dbk_out_bfr = tmp_ptr;
	}else
	{
		int32 input_mcu_format = ((g_glb_reg_ptr->VSP_CFG1 >> 24) & 0x07);
		int32 scale_down_factor = ((g_mbc_reg_ptr->MBC_CFG >> 12) & 0x03);

		switch(input_mcu_format)
		{
		case JPEG_FW_YUV444:
			format_convertion_444to422();
			downsample_org444(scale_down_factor);
			break;
		case JPEG_FW_YUV411:
			format_convertion_411to422();
			downsample_org411(scale_down_factor);
			break;
		case JPEG_FW_YUV411_R:
			format_convertion_411Rto420();
			downsample_org411R(scale_down_factor);
			break;
		case JPEG_FW_YUV422_R:
			format_convertion_422Rto420();
			downsample_org422R(scale_down_factor);
			break;
		case JPEG_FW_YUV422:
			format_convertion_422to422();
			downsample_org422(scale_down_factor);
			break;
		case JPEG_FW_YUV420:
			format_convertion_420to420();
			downsample_org420(scale_down_factor);
			break;
		case JPEG_FW_YUV400:
			format_convertion_400to400();
			downsample_org400(scale_down_factor);
			break;
		}
	}

	mbc_valid_bfr_id = 1 - mbc_valid_bfr_id;

	g_mbc_reg_ptr->MBC_ST0 = 0x30;
	g_mbc_reg_ptr->MBC_ST0 &= (0xffffffff - (mbc_valid_bfr_id << 23));

	if ((standard == JPEG) || 
		(((standard == MPEG4) || (standard == ITU_H263)|| (standard == H264)) && (is_encoder)) )
	{
 		dbk_module();
	}
}

PUBLIC int32 READ_REG_MBC_ST0_REG(uint32 addr, uint32 mask,uint32 exp_value, char *pstring)
{
#if _FW_TEST_VECTOR_
	int32 value = 0x0;
	int32 mbc_st0 = g_mbc_reg_ptr->MBC_ST0;

	exp_value = mbc_st0 & mask;
	
	FPRINTF(g_fw_tv_cmd,"%x, %x, %x, %08x, %08x \t  //%s\n",4,addr,value,mask,exp_value, pstring);
	g_tv_cmd_num++;

	
#endif// _FW_TEST_VECTOR_
	return exp_value;	
}
/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End 
