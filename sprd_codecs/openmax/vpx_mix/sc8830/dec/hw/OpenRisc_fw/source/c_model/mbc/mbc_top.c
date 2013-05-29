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
#include "sc8810_video_header.h"
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

//store current mb's top and left refernce pixel; 
uint8 luma_top[17+8], luma_left[16];//weihu
uint8 chroma_top[2][9+4], chroma_left[2][8+4];
uint8 top_left[3];

void init_mbc_module(void)
{
	g_mbc_reg_ptr->MBC_ST0 = 0; 
	mbc_valid_bfr_id = 0;
	Init_downsample_fun();

	//h264 ipred
	memset (luma_left, 0, 16);
	memset (chroma_left[0], 0, 8);
	memset (chroma_left[1], 0, 8);
	memset (top_left, 0, 3);

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
#if defined(H264_DEC) || defined(REAL_DEC)

	int32 mb_x = ((g_glb_reg_ptr->VSP_CTRL0>>0) & 0x7f);//weihu //0x3f
	int32 mb_num_x	= (g_glb_reg_ptr->VSP_CFG1 >> 0) & 0xff;
	uint8 *topBorder_luma, *topBorder_chroma[2];
	uint8* mbc_bfr_ptr = (uint8 *)vsp_mbc_out_bfr;
	uint8 *pred;
	int32 i;

#if defined(H264_DEC)
	topBorder_luma = g_image_ptr->ipred_top_line_buffer + mb_x * MB_SIZE;
	topBorder_chroma[0] = g_image_ptr->ipred_top_line_buffer + mb_num_x * MB_SIZE + mb_x * MB_CHROMA_SIZE;
	topBorder_chroma[1] = g_image_ptr->ipred_top_line_buffer + mb_num_x * (MB_SIZE+MB_CHROMA_SIZE) + mb_x * MB_CHROMA_SIZE;
#else
	topBorder_luma = g_rv_decoder_ptr->ipred_top_line_buffer + mb_x * MB_SIZE;
	topBorder_chroma[0] = g_rv_decoder_ptr->ipred_top_line_buffer + mb_num_x * MB_SIZE + mb_x * MB_CHROMA_SIZE;
	topBorder_chroma[1] = g_rv_decoder_ptr->ipred_top_line_buffer + mb_num_x * (MB_SIZE+MB_CHROMA_SIZE) + mb_x * MB_CHROMA_SIZE;
#endif

	//update top left pixel
	top_left[0] = topBorder_luma[MB_SIZE-1];
	top_left[1] = topBorder_chroma[0][MB_CHROMA_SIZE-1];
	top_left[2] = topBorder_chroma[1][MB_CHROMA_SIZE-1];
				
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
		luma_left[i] = pred[i*MBC_Y_SIZE];
	}
				
	pred = mbc_bfr_ptr + MBC_U_OFFSET + MBC_C_SIZE * 5 - 1;
	for (i = 0; i < MB_CHROMA_SIZE; i++) //u
	{
		chroma_left[0][i] = pred[i*MBC_C_SIZE];
	}
				
	pred = mbc_bfr_ptr + MBC_V_OFFSET + MBC_C_SIZE * 5 - 1;
	for (i = 0; i < MB_CHROMA_SIZE; i++) //v
	{
		chroma_left[1][i] = pred[i*MBC_C_SIZE];
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
	standard		= (g_glb_reg_ptr->VSP_CFG0 >> 8) & 0xf;
	is_encoder = (g_glb_reg_ptr->VSP_CFG0 >> 14) & 0x1;

	if(VSP_JPEG != standard)
	{
		uint32 *tmp_ptr;
		int32 eob_cfg_flag = (g_mbc_reg_ptr->MBC_ST0 >> 6) & 0x01;

		if(!eob_cfg_flag)
		{
			if (standard == VSP_H264 || standard == VSP_RV9 || standard == VSP_RV8)
			{
				int32 mb_type = ((g_mbc_reg_ptr->MBC_CMD0>>30)&0x1);

				if (mb_type == 0) //intra mb
				{
					ipred_module();
				}else //inter mb
				{
					h264_add_ref_and_residual();
				}

				update_top_line_bfr ();

				//put to rec frame.
			//	PutRec2Frm ();
			}else
			{
				mp4_add_ref_and_residual();
			}			
		}
		
// 		put_recMB_to_frame();
#if !defined(MPEG4_LIB)
		printf_mbc_mb(g_fp_mbc_tv);
#endif
		/*switch two dbk pointer*/
#if defined(REAL_DEC)
		tmp_ptr        = vsp_mbc_out_bfr;
		vsp_mbc_out_bfr = vsp_rdbk_obuf ;
		vsp_rdbk_obuf = tmp_ptr;
#else
		tmp_ptr        = vsp_mbc_out_bfr;
		vsp_mbc_out_bfr = vsp_dbk_out_bfr;
		vsp_dbk_out_bfr = tmp_ptr;
#endif
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
#ifndef VP8_DEC 
	if ((standard == VSP_JPEG) || 
		(((standard == VSP_MPEG4) || (standard == ITU_H263)) && (is_encoder)) )
	{

 		dbk_module();
	}
#endif
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

void mbc_module_ppa (char  mbc_out_buf[384],
					 char  mca_out_buf[384],
					 short dct_out_buf[384], 
					 int slice_info[40],
					 int mbc_para_buf[4], 
					 char decoder_format,//3b
					 char picwidthinMB,//7b
					 char picheightinMB//7b					 
					 )//weihu
{
	int i,j;
    char intra_chroma_pred_mode,Ipred_size,Ipred_avail_d,Ipred_avail_c,Ipred_avail_b,Ipred_avail_a,skip_idct,is_IPCM,is_intra,cur_mb_x, cur_mb_y;//25b
	int cbp; 
	char intra16x16_luma_pred_mode;
	char i4_luma_pred_mode[16];
    //char i8_luma_pred_mode[4];
	uint32	*	src_ptr;
	uint32	*	dst_ptr;
	

	int mb_mode;
	int mbc_cbp;
	int MBC_CMD0,MBC_CMD1,MBC_CMD2,MBC_CMD3;

	//MB_para in
	intra_chroma_pred_mode=(mbc_para_buf[0]>>23)&0x3;
	Ipred_size=(mbc_para_buf[0]>>21)&0x3;	
	Ipred_avail_d=(mbc_para_buf[0]>>20)&0x1;
	Ipred_avail_c=(mbc_para_buf[0]>>19)&0x1;
	Ipred_avail_b=(mbc_para_buf[0]>>18)&0x1;
	Ipred_avail_a=(mbc_para_buf[0]>>17)&0x1;
    skip_idct=(mbc_para_buf[0]>>16)&0x1;
	is_IPCM=(mbc_para_buf[0]>>15)&0x1;
	is_intra=(mbc_para_buf[0]>>14)&0x1;
	cur_mb_x=(mbc_para_buf[0]>>7)&0x3f;
	cur_mb_y=(mbc_para_buf[0])&0x3f;
	cbp=(mbc_para_buf[1])&0xffffff;;//24b
    if(Ipred_size==I_16X16)
	{
	   intra16x16_luma_pred_mode=(mbc_para_buf[2])&0x3;//2b
	}
	else if(Ipred_size==I_4X4)
	{
		for(i=0;i<8;i++)
		{
			i4_luma_pred_mode[i]=(mbc_para_buf[2]>>(i*4))&0xf;//4b
			i4_luma_pred_mode[i+8]=(mbc_para_buf[3]>>(i*4))&0xf;//4b
		}
		
	}
	/*else if(Ipred_size==I_8X8)
	{
		for(i=0;i<2;i++)
		{
			i8_luma_pred_mode[i]=(mbc_para_buf[2]>>(i*16))&0xf;//4b
			i8_luma_pred_mode[i+2]=(mbc_para_buf[3]>>(i*16))&0xf;//4b
		}
	}*/
	
	//reg set
	if(!is_intra)
        mb_mode=0;
	else if (is_IPCM) 
		mb_mode=2;
	else if (skip_idct) 
		mb_mode=3;
	else 
		mb_mode=Ipred_size;//!=8x8
	mbc_cbp=(cbp&0xffc3c3)|((cbp&0x3030)>>2)|((cbp&0x0c0c)<<2);

	MBC_CMD0=((!is_intra)<<30)|(mb_mode<<28)|((Ipred_avail_d&&is_intra)<<27)|((Ipred_avail_a&&is_intra)<<26)|((Ipred_avail_b&&is_intra)<<25)|((Ipred_avail_c&&is_intra)<<24)|mbc_cbp;
    if (Ipred_size==I_16X16)
	{
    	MBC_CMD1=intra16x16_luma_pred_mode;
        MBC_CMD2=intra16x16_luma_pred_mode;
	}
	else
	{
        MBC_CMD1=(i4_luma_pred_mode[0]<<28)|(i4_luma_pred_mode[1]<<24)|(i4_luma_pred_mode[4]<<20)|(i4_luma_pred_mode[5]<<16)|(i4_luma_pred_mode[2]<<12)|(i4_luma_pred_mode[3]<<8)|(i4_luma_pred_mode[6]<<4)|(i4_luma_pred_mode[7]);
		MBC_CMD2=(i4_luma_pred_mode[8]<<28)|(i4_luma_pred_mode[9]<<24)|(i4_luma_pred_mode[12]<<20)|(i4_luma_pred_mode[13]<<16)|(i4_luma_pred_mode[10]<<12)|(i4_luma_pred_mode[11]<<8)|(i4_luma_pred_mode[14]<<4)|(i4_luma_pred_mode[15]);	
	}
	
	MBC_CMD3=intra_chroma_pred_mode;

    g_mbc_reg_ptr->MBC_CMD0=MBC_CMD0;
	g_mbc_reg_ptr->MBC_CMD1=MBC_CMD1;
    g_mbc_reg_ptr->MBC_CMD2=MBC_CMD2;
	g_mbc_reg_ptr->MBC_CMD3=MBC_CMD3;

	//FPRINTF (pFmoFile, "frame: %d, mb_y: %d, mb_x: %d \n", g_nFrame_dec_h264, cur_mb_y, cur_mb_x);//weihu
	//FPRINTF (pFmoFile, "CMD0: %x \n", MBC_CMD0);//weihu
	//if (is_intra) {
		//FPRINTF (pFmoFile, "CMD0: %x \n", MBC_CMD0);//weihu
		//FPRINTF (pFmoFile, "CMD1: %x \n", MBC_CMD1);//weihu
		//FPRINTF (pFmoFile, "CMD2: %x \n", MBC_CMD2);//weihu
		//FPRINTF (pFmoFile, "CMD3: %x \n", MBC_CMD3);//weihu
	//}
	src_ptr = (uint32*)(&(mca_out_buf[0]));
	dst_ptr = vsp_fw_mca_out_bfr;
	   for (i = 0; i < 96; i++)
	   {
		   dst_ptr[i] = src_ptr[i];
	   }
	
	 src_ptr = (uint32*)(&(dct_out_buf[0]));
	 dst_ptr = vsp_dct_io_0;
	   for (i = 0; i < 192; i++)
	   {
		   dst_ptr[i] = src_ptr[i];
	   }

	mbc_module();

	/*copy Y component*/
	   src_ptr = vsp_dbk_out_bfr + 2+6*4;//switch mbc dbk buf, vsp_mbc_out_bfr + 2+6*4;
	   dst_ptr = (uint32*)(&(mbc_out_buf[0]));
	   for (i = 0; i < 16; i++)
	   {
		   for (j = 0; j < 4; j++)  
			   dst_ptr[j] = src_ptr[j];
		   
		   src_ptr += 6;
		   dst_ptr += 4;
	   }
	   
	   /*copy U component*/
	   src_ptr = vsp_dbk_out_bfr + U_OFFSET_BUF_C + 2+4*4;
	   dst_ptr = (uint32*)(&(mbc_out_buf[256]));
	   for (i = 0; i < 8; i++)
	   {
		   
		   for (j = 0; j < 2; j++)  
			   dst_ptr[j] = src_ptr[j];
		   
		   src_ptr += 4;
		   dst_ptr += 2;
	   }
	   
	   /*copy V component*/
	   src_ptr = vsp_dbk_out_bfr + V_OFFSET_BUF_C + 2+4*4;
	   dst_ptr = (uint32*)(&(mbc_out_buf[320]));
	   for (i = 0; i < 8; i++)
	   {
		   
		   for (j = 0; j < 2; j++)  
			   dst_ptr[j] = src_ptr[j];
		   
		   src_ptr += 4;
		   dst_ptr += 2;
	   }
	

}
/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End 





















