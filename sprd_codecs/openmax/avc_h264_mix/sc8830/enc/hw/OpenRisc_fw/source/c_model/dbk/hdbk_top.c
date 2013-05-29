/*dbk_top.c*/
#include "video_common.h"
#include "hdbk_mode.h"
#include "hdbk_global.h"

//now, dbk c-model only support two-plane.
void dbk_module ()
{
	int			i;
	int			j;
	uint32	*	mb_pix_ptr;
	uint8	*	mb_pix_u_ptr;
	uint8	*	mb_pix_v_ptr;
	uint32	*	frm_pix_ptr;
	int			pic_width;		//unit is word, 
	
	int			pic_y_offset;
	int			pic_c_offset;

	int			fst_row;
	int			last_row;
	int			fst_col;
	int			last_col;

	int			start_row;
	int			end_row;
	int			start_col;
	int			end_col;
	int			vid_standard;
	int			dbk_eb;
	int			h264_fmo_mode;
	int			is_encoder;

	uint32		uv_data;
	uint32	*	src_ptr;
	uint32	*	dst_ptr;
	
//	int			uv;
//	uint32		four_pix;
	uint32		four_pix_u;
	uint32		four_pix_v;
	int			addr_offset;
	uint8		pix_u0, pix_u1, pix_u2, pix_u3;
	uint8		pix_v0, pix_v1, pix_v2, pix_v3;

	uint32	*	frame_y_ptr = NULL;

	int32		mb_x;
	int32		mb_y;
	int32		mb_num_x	= (g_glb_reg_ptr->VSP_CFG1 >> 0 ) & 0xff;
	int32		mb_num_y	= (g_glb_reg_ptr->VSP_CFG1 >> 12) & 0xff;

	int32		post_filter_en = (g_dbk_reg_ptr->DBK_CFG >> 2) & 0x01; 

#if defined(JPEG_DEC)
	mb_x		= (g_dbk_reg_ptr->DBK_SID_CFG >> 0) & 0x3ff;
	mb_y		= (g_dbk_reg_ptr->DBK_SID_CFG >> 16) & 0x3ff;
#else
	mb_x		= (g_glb_reg_ptr->VSP_CTRL0 >> 0) & 0x7f;
	mb_y		= (g_glb_reg_ptr->VSP_CTRL0 >> 8) & 0x7f;
#endif

	vid_standard	= (g_glb_reg_ptr->VSP_CFG0 >> 8) & 0xf;
	h264_fmo_mode   = (g_dbk_reg_ptr->DBK_CFG >> 3) & 0x1;
	is_encoder = (g_glb_reg_ptr->VSP_CFG0 >> 14) & 0x1;

	if(VSP_JPEG != vid_standard)
	{
	#if defined(MPEG4_DEC)||defined(H264_DEC)||defined(MPEG4_ENC)||defined(H264_ENC)
		dbk_eb			= (g_glb_reg_ptr->VSP_CFG0 >> 12) & 0x1;
	
		if(g_trace_enable_flag&TRACE_ENABLE_DBK)
		{
		// 	FPRINTF (g_dbk_trace_fp, "\nframe: %d, mb_x: %d, mb_y: %d\n", g_nFrame_dec_h264, mb_x, mb_y);
		}
		
		DBK_TraceMBInfor (
						   g_dbk_reg_ptr->HDBK_MB_INFO&0x3f,
						   (g_dbk_reg_ptr->HDBK_MB_INFO>>8)&0x3f,
						   (g_dbk_reg_ptr->HDBK_MB_INFO>>16)&0x3f,
						   (g_dbk_reg_ptr->HDBK_PARS>>16)&0x1f,
						   (g_dbk_reg_ptr->HDBK_PARS>>8)&0x1f,
						   (g_dbk_reg_ptr->HDBK_PARS>>0)&0x1f
						   );
		
		DBK_TraceMBBs (g_dbk_reg_ptr->HDBK_BS_H0, g_dbk_reg_ptr->HDBK_BS_H1, g_dbk_reg_ptr->HDBK_BS_V0, g_dbk_reg_ptr->HDBK_BS_V1);

	// 	if ((g_nFrame_dec == 1) && (mb_x == 13) && (mb_y == 8))
	//	{
	//		printf ("");
	//	}

		/************************************************************************
				if mpeg4, out reconstructed MB to external memory 
				 as reference frame
		************************************************************************/
		fst_row  = (mb_y == 0) ? 1 : 0;
		last_row = (mb_y == mb_num_y - 1) ? 1 : 0;
		fst_col  = (mb_x == 0) ? 1 : 0;
		last_col = (mb_x == mb_num_x - 1) ? 1 : 0;

		pic_width = mb_num_x*(MB_SIZE>>2);

		//mpeg4 output reconstructed frame
		if ((vid_standard == ITU_H263) || 
			(vid_standard == MPEG4)    || 
			(vid_standard == FLV_H263) || 
			(h264_fmo_mode == TRUE)	   ||
			((vid_standard == H264)&&(is_encoder) )  
		   )
		{
		#if defined(MPEG4_DEC)
			g_frame_y_ptr = (uint32 *)(g_dec_vop_mode_ptr->pCurRecFrame->pDecFrame->imgY);
			g_frame_c_ptr = (uint32 *)(g_dec_vop_mode_ptr->pCurRecFrame->pDecFrame->imgU);
		#elif defined(MPEG4_ENC)
			g_frame_y_ptr = (uint32 *)(g_enc_vop_mode_ptr->pYUVRecFrame->imgY);
			g_frame_c_ptr = (uint32 *)(g_enc_vop_mode_ptr->pYUVRecFrame->imgU);		
		#elif defined(H264_DEC)	
			g_frame_y_ptr = ((uint32 *)g_dec_picture_ptr->imgY);
			g_frame_c_ptr = ((uint32 *)g_dec_picture_ptr->imgU);
		#elif defined(H264_ENC)
			g_frame_y_ptr = ((uint32 *)g_enc_image_ptr->pYUVRecFrame->imgY);
			g_frame_c_ptr = ((uint32 *)g_enc_image_ptr->pYUVRecFrame->imgUV);
		#endif

			pic_y_offset  = mb_y*MB_SIZE*pic_width + mb_x*(MB_SIZE>>2);
			pic_c_offset  = mb_y*MB_CHROMA_SIZE*pic_width + mb_x*(MB_SIZE>>2);		
			
			/*copy Y component*/
			frm_pix_ptr  = g_frame_y_ptr + pic_y_offset;
			mb_pix_ptr	 = vsp_dbk_out_bfr + 26;
			
			for (j = 0; j < MB_SIZE; j++)
			{
				for (i = 0; i < 4; i++)
				{
					frm_pix_ptr[i] = mb_pix_ptr[i];
					
					if(g_vector_enable_flag&VECTOR_ENABLE_DBK)
					{
						FPRINTF (g_fp_dbk_mb_tv, "%08x\n", frm_pix_ptr[i]); 
					}
				}
				
				mb_pix_ptr  += 6;
				frm_pix_ptr += pic_width;
			}
			
			/*copy UV component*/
			frm_pix_ptr  = g_frame_c_ptr + pic_c_offset;
			mb_pix_u_ptr = (uint8 *)(vsp_dbk_out_bfr + U_OFFSET_BUF_C+18);
			mb_pix_v_ptr = (uint8 *)(vsp_dbk_out_bfr + V_OFFSET_BUF_C+18);
			
#if 1 //two plane
			for (j = 0; j < MB_CHROMA_SIZE; j++)
			{
				for (i = 0; i < 4; i++)
				{
					uv_data = (mb_pix_u_ptr[2*i+0] << 0) | (mb_pix_v_ptr[2*i+0] << 8) | (mb_pix_u_ptr[2*i+1] << 16) | (mb_pix_v_ptr[2*i+1] << 24);
					
					frm_pix_ptr[i] = uv_data;
				}
				
				mb_pix_u_ptr += 16;
				mb_pix_v_ptr += 16;
				
				frm_pix_ptr += pic_width;
			}
#else
			{
				uint32 c_frm_width = mb_num_x*8;
				uint8 *frm_pix_u_ptr = g_enc_image_ptr->pYUVRecFrame->imgU + mb_x*8+mb_y*8*c_frm_width;
				uint8 *frm_pix_v_ptr = g_enc_image_ptr->pYUVRecFrame->imgV + mb_x*8+mb_y*8*c_frm_width;

				for (j = 0; j < MB_CHROMA_SIZE; j++)
				{
					for (i = 0; i < 8; i++)
					{			
						frm_pix_u_ptr[i] = mb_pix_u_ptr[i];
						frm_pix_v_ptr[i] = mb_pix_v_ptr[i];
					}
					
					mb_pix_u_ptr += 16;
					mb_pix_v_ptr += 16;
					
					frm_pix_u_ptr += c_frm_width;
					frm_pix_v_ptr += c_frm_width;
				}
			}
#endif
			
			//only for printf U and V DBK out data for verification
			/*for (uv = 0 ; uv < 2; uv++)
			{
				for (j = 0; j < 8; j++)
				{		
					for (i = 0; i < 2; i++)
					{	
						addr_offset	= 18 + j * 4 + i;	
						four_pix_u	= *(vsp_dbk_out_bfr + U_OFFSET_BUF_C + addr_offset);
						four_pix_v	= *(vsp_dbk_out_bfr + V_OFFSET_BUF_C + addr_offset);				
						
						
						four_pix = (uv == 0) ? four_pix_u : four_pix_v;
						
						if(g_vector_enable_flag&VECTOR_ENABLE_DBK)
						{
							FPRINTF (g_fp_dbk_mb_tv, "%08x\n", four_pix);
						}
					}
				}
			}*/
		}

		if(g_trace_enable_flag&TRACE_ENABLE_DBK)
		{
			//FPRINTF (g_dbk_trace_fp, "\npixels before deblocking: \n");
		}
//		DBK_PrintOneMB (mb_y, mb_x);
		g_dbk_reg_ptr->DBK_SID_CFG = (mb_y<<16) | mb_x;			

		/***********************************************************************
					start filter_core_ctr to filter one MB 
		************************************************************************/
		if (((vid_standard == H264) && (h264_fmo_mode == TRUE)) || (!post_filter_en))
			return;

#if defined(H264_ENC)
//		return;
#endif

//		PrintMBBfFilter ();

		hdbk_core_ctr (mb_x);

//		PrintMBAfFilter (mb_x);

		/***********************************************************************
							send out filtered pixels
		************************************************************************/
		/*send out Y component*/
		if(g_trace_enable_flag&TRACE_ENABLE_DBK)
		{
			//FPRINTF (g_dbk_trace_fp, "\npixels after deblocking: \n");
		}
		//DBK_PrintFilterOneMB (mb_y, mb_x);
		pic_y_offset	= mb_y*MB_SIZE*pic_width + mb_x*(MB_SIZE>>2);

		start_row		= fst_row  ? 4 : 0;
		end_row			= last_row ? 20 : 16;

		start_col		= fst_col  ? 2 : 1;
		end_col			= last_col ? 6 : 5;

	#if defined(H264_DEC)
		frm_pix_ptr		= ((uint32 *)g_dec_picture_ptr->imgY) + pic_y_offset - 2;
	#elif defined(MPEG4_DEC)
		frm_pix_ptr		= ((uint32 *)g_dec_vop_mode_ptr->pCurDispFrame->pDecFrame->imgY) + pic_y_offset - 2;
	#elif defined(H264_ENC)
		frm_pix_ptr		= ((uint32 *)g_enc_image_ptr->pYUVRecFrame->imgY) + pic_y_offset - 2;
	#elif defined(VP8_DEC)
		frm_pix_ptr		= ((uint32 *)y) - 2;
	#endif

		frm_pix_ptr		= fst_row ? frm_pix_ptr : frm_pix_ptr - pic_width*4;	

		for (j = start_row; j < end_row; j++)
		{
			for (i = start_col; i < end_col; i++)
			{
				int		addr,addr1;	//weihu		
				
				if (j < 16)
				{
					addr = j * 6 + i;
					frm_pix_ptr[i] = vsp_dbk_out_bfr[addr];
				}
				else
				{
					/*note the last row of one MB is in g_dbk_line_buf other than in mb_dbk_buf*/
					addr = mb_x * 4 - 2 + (j & 0x3) * LINE_WIDTH_Y + i;
					frm_pix_ptr[i] = g_dbk_line_buf[addr];	
					addr1 = j * 6 + i;//weihu
					vsp_dbk_out_bfr[addr1]=g_dbk_line_buf[addr];//weihu
				}			

// 				if(g_vector_enable_flag&VECTOR_ENABLE_IPRED)	
// 				{
// 					fprintf (g_fp_dbk_mb_tv, "%08x\n", frm_pix_ptr[i]);
// 				}
			}

			frm_pix_ptr += pic_width;
		}

		/*send UV component*/
		pic_c_offset	= mb_y*MB_CHROMA_SIZE*pic_width + mb_x*(MB_SIZE>>2);
		
		start_row		= fst_row  ? 4 : 0;
		end_row			= last_row ? 12 : 8;
		start_col		= fst_col  ? 2 : 1;
		end_col			= last_col ? 4 : 3;

	#if defined(H264_DEC)
		frm_pix_ptr	= ((uint32 *)g_dec_picture_ptr->imgU) + pic_c_offset - 4;
	#elif defined(MPEG4_DEC)
		frm_pix_ptr	= ((uint32 *)g_dec_vop_mode_ptr->pCurDispFrame->pDecFrame->imgU) + pic_c_offset - 4;
	#elif defined(H264_ENC)
		frm_pix_ptr	= ((uint32 *)g_enc_image_ptr->pYUVRecFrame->imgUV) + pic_c_offset - 4;
	#elif defined(VP8_DEC)
		frm_pix_ptr = ((uint32 *)u) - 2;
	#endif
		
		frm_pix_ptr	= fst_row ? frm_pix_ptr : frm_pix_ptr - pic_width*4;
		
		for (j = start_row; j < end_row; j++)
		{		
			for (i = start_col; i < end_col; i++)
			{
				int		addr1;	//weihu	
				if (j < 8)
				{
					addr_offset	= j * 4 + i;	
					four_pix_u	= *(vsp_dbk_out_bfr + U_OFFSET_BUF_C + addr_offset);
					four_pix_v	= *(vsp_dbk_out_bfr + V_OFFSET_BUF_C + addr_offset);				
				}
				else
				{
					/*note the last row of one MB is in g_dbk_line_buf other than in dbk_out_buf*/
					addr_offset = mb_x* 2 - 2 + (j & 0x3)*LINE_WIDTH_C + i;
					
					four_pix_u = *(g_dbk_line_buf + U_OFFSET_LINE_BUF + addr_offset); 
					four_pix_v = *(g_dbk_line_buf + V_OFFSET_LINE_BUF + addr_offset);
					addr1 = j * 4 + i;//weihu
					vsp_dbk_out_bfr[U_OFFSET_BUF_C+addr1]=*(g_dbk_line_buf + U_OFFSET_LINE_BUF + addr_offset);//weihu
					vsp_dbk_out_bfr[V_OFFSET_BUF_C+addr1]=*(g_dbk_line_buf + V_OFFSET_LINE_BUF + addr_offset);//weihu
				}

				pix_u0 = (four_pix_u >> 0)  & 0xff;
				pix_u1 = (four_pix_u >> 8)  & 0xff;
				pix_u2 = (four_pix_u >> 16) & 0xff;
				pix_u3 = (four_pix_u >> 24) & 0xff;

				pix_v0 = (four_pix_v >> 0)  & 0xff;
				pix_v1 = (four_pix_v >> 8)  & 0xff;
				pix_v2 = (four_pix_v >> 16) & 0xff;
				pix_v3 = (four_pix_v >> 24) & 0xff;

				frm_pix_ptr[2*i+0] = (pix_u0 << 0) | (pix_v0 << 8) | (pix_u1 << 16) | (pix_v1 << 24);
				frm_pix_ptr[2*i+1] = (pix_u2 << 0) | (pix_v2 << 8) | (pix_u3 << 16) | (pix_v3 << 24);			
				
			//	FPRINTF (g_fp_dbk_mb_tv, "%08x\n", frm_pix_ptr[2*i+0]);
			//	FPRINTF (g_fp_dbk_mb_tv, "%08x\n", frm_pix_ptr[2*i+1]);
			}
			
			frm_pix_ptr += pic_width;
		}
	
 	//only for printf U and V DBK out data for verification
 	/*for (uv = 0 ; uv < 2; uv++)
 	{
 		for (j = start_row; j < end_row; j++)
 		{		
 			for (i = start_col; i < end_col; i++)
 			{				
 				if (j < 8)
 				{
 					addr_offset	= j * 4 + i;	
 					four_pix_u	= *(vsp_dbk_out_bfr + U_OFFSET_BUF_C + addr_offset);
 					four_pix_v	= *(vsp_dbk_out_bfr + V_OFFSET_BUF_C + addr_offset);				
 				}
 				else
 				{
 					//note the last row of one MB is in g_dbk_line_buf other than in dbk_out_buf
 					addr_offset = mb_x* 2 - 2 + (j & 0x3)*LINE_WIDTH_C + i;
 					
 					four_pix_u = *(g_dbk_line_buf + U_OFFSET_LINE_BUF + addr_offset); 
 					four_pix_v = *(g_dbk_line_buf + V_OFFSET_LINE_BUF + addr_offset);
 				}
 				
 				four_pix = (uv == 0) ? four_pix_u : four_pix_v;
				
 //				if(g_vector_enable_flag&VECTOR_ENABLE_IPRED)	
 				{
// 					fprintf (g_fp_dbk_mb_tv, "%08x\n", four_pix);
 				}
 			}
 		}
 	}*/

#ifdef TV_OUT
#ifdef ONE_FRAME
		if(g_nFrame_enc==FRAME_X)
#endif
		Print_DBK_Out();	// before the preparation of next MB
#endif

		/************************************************************************
		1. polling MBC done, 
		2. copy pixels in vsp_dbk_out_bfr to vsp_mbc_out_bfr
		for next MB's post-filter
		3. switch buffer
		*************************************************************************/
		/*copy Y component*/
		src_ptr = vsp_dbk_out_bfr + 5;
#ifdef VP8_DEC	
        dst_ptr = vsp_dbk_out_bfr + 1;
#else			
		dst_ptr = vsp_mbc_out_bfr + 1;
#endif		
		for (i = 0; i < 20; i++)
		{
			uint32	src_data;
			if (i <16)
			{
				src_data = src_ptr[0];
			}
			else
			{
				src_data = *(g_dbk_line_buf + mb_x * 4 + (i & 3) * LINE_WIDTH_Y + 3);
			}

			dst_ptr[0] = src_data;

			src_ptr += 6;
			dst_ptr += 6;
		}

		/*copy U component*/
		src_ptr = vsp_dbk_out_bfr + U_OFFSET_BUF_C + 3;
#ifdef VP8_DEC
        dst_ptr = vsp_dbk_out_bfr + U_OFFSET_BUF_C + 1;
#else			
		dst_ptr = vsp_mbc_out_bfr + U_OFFSET_BUF_C + 1;
#endif		
		for (i = 0; i < 12; i++)
		{
			uint32	src_data;
			if (i < 8)
			{
				src_data = src_ptr[0];
			}
			else
			{
				src_data = *(g_dbk_line_buf + U_OFFSET_LINE_BUF + mb_x*2 + (i & 3) * LINE_WIDTH_C + 1);
			}
			
			dst_ptr[0] = src_data;
			
			src_ptr += 4;
			dst_ptr += 4;
		}

		/*copy V component*/
		src_ptr = vsp_dbk_out_bfr + V_OFFSET_BUF_C + 3;
#ifdef VP8_DEC
#else		
		dst_ptr = vsp_mbc_out_bfr + V_OFFSET_BUF_C + 1;
#endif
		for (i = 0; i < 12; i++)
		{
			uint32	src_data;
			if (i < 8)
			{
				src_data = src_ptr[0];
			}
			else
			{
				src_data = *(g_dbk_line_buf + V_OFFSET_LINE_BUF	 + mb_x*2 + (i & 3) * LINE_WIDTH_C + 1);
			}
			
			dst_ptr[0] = src_data;
			
			src_ptr += 4;
			dst_ptr += 4;
		}
#ifdef VP8_DEC
#else
		g_mb_cnt++;
#endif		
	#endif
	}else //jpeg
 	{
// 		int32 input_mcu_format	= ((g_glb_reg_ptr->VSP_CFG1 >> 24) & 0x07);
// 		int32 scale_down_factor = ((g_mbc_reg_ptr->MBC_CFG >> 12) & 0x03);
// 		int32 out_width_y		= ((g_dcam_reg_ptr->DCAM_SRC_SZIE >> 0) & 0xFFF);
// 		uint8 *frm_y = PNULL;
// 		uint8 *frm_u = PNULL;
// 		uint8 *frm_v = PNULL;
// 
// #if defined(JPEG_DEC)
// 		JPEG_CODEC_T *JpegCodec = Get_JPEGDecCodec();
// 		frm_y = JpegCodec->dbk_bfr0_valid ? JpegCodec->YUV_Info_0.y_data_ptr : JpegCodec->YUV_Info_1.y_data_ptr;
// 		frm_u = JpegCodec->dbk_bfr0_valid ? JpegCodec->YUV_Info_0.u_data_ptr : JpegCodec->YUV_Info_1.u_data_ptr;
// 		frm_v = JpegCodec->dbk_bfr0_valid ? JpegCodec->YUV_Info_0.v_data_ptr : JpegCodec->YUV_Info_1.v_data_ptr;
// 
// 		if(g_int_yuv_buf0_full == TRUE) 
// 		{
// 			//assert(g_int_yuv_buf1_full == FALSE);
// 			frm_y = JpegCodec->YUV_Info_1.y_data_ptr;
// 			frm_u = JpegCodec->YUV_Info_1.u_data_ptr;
// 			frm_v = JpegCodec->YUV_Info_1.v_data_ptr;
// 		}
// #endif
// 
// 		switch(input_mcu_format)
// 		{
// 		case JPEG_FW_YUV444:
// 			out_org444(frm_y, frm_u, frm_v, out_width_y, mb_x, mb_y, scale_down_factor);
// 			break;
// 		case JPEG_FW_YUV411:
// 			out_org411(frm_y, frm_u, frm_v, out_width_y, mb_x, mb_y, scale_down_factor);
// 			break;
// 		case JPEG_FW_YUV411_R:
// 			out_org411R(frm_y, frm_u, frm_v, out_width_y, mb_x, mb_y, scale_down_factor);
// 			break;
// 		case JPEG_FW_YUV422_R:
// 			out_org422R(frm_y, frm_u, frm_v, out_width_y, mb_x, mb_y, scale_down_factor);
// 			break;
// 		case JPEG_FW_YUV422:
// 			out_org422(frm_y, frm_u, frm_v, out_width_y, mb_x, mb_y, scale_down_factor);
// 			break;
// 		case JPEG_FW_YUV420:
// 			out_org420(frm_y, frm_u, frm_v, out_width_y, mb_x, mb_y, scale_down_factor);
// 			break;
// 		case JPEG_FW_YUV400:
// 			out_org400(frm_y, frm_u, frm_v, out_width_y, mb_x, mb_y, scale_down_factor);
// 			break;
// 		}
 	}
}


#if 1
void dbk_ppa_module(int *slice_info)
{
	int i, j, blk4x4StrIdx, cmd;
	// MB para 
	int cur_mb_x = slice_info[0];
	int cur_mb_y = slice_info[1];
	char mb_avail_a = slice_info[2];
	char mb_avail_b = slice_info[3];
	char mb_avail_c = slice_info[4];
	char mb_avail_d = slice_info[5];

	//dbk
	char skip_deblock = (slice_info[6]==1);
	char bs_h[16]={0};//3b
	char bs_v[16]={0};//3b
	int  filter_left_edge;
	int  filter_top_edge;

	int  ref0_p,ref0_q;
	int  mv0_p[2],mv0_q[2];
	int  condition0;

	filter_left_edge = (cur_mb_x > 0)&&(mb_avail_a||(slice_info[6]!=2))&&!skip_deblock;
	filter_top_edge = (cur_mb_y > 0)&&(mb_avail_b||(slice_info[6]!=2))&&!skip_deblock;
	//filter_internal_edge=!skip_deblock;
	for(i=0; i<4; i++)//horizontal line
	{
		for(j=0; j<4; j++)//part
		{
			blk4x4StrIdx = 13 + i + j*5;
			ref0_p = slice_info[blk4x4StrIdx-1];
			ref0_q = slice_info[blk4x4StrIdx];
			blk4x4StrIdx = 44 + 2*i + j*10;
			mv0_p[0] = slice_info[blk4x4StrIdx-2];
			mv0_p[1] = slice_info[blk4x4StrIdx-1];
			mv0_q[0] = slice_info[blk4x4StrIdx];
			mv0_q[1] = slice_info[blk4x4StrIdx+1];
			condition0 = (ref0_p!=ref0_q)||(abs(mv0_p[0]-mv0_q[0])>=4)||(abs(mv0_p[1]-mv0_q[1])>=4);

			blk4x4StrIdx = 88 + i + j*5;
			if ((i==0) && (!filter_left_edge))
				bs_h[4*i+j]=0;
			else if ((i==0) && (slice_info[107] || slice_info[108]))	// is_intra || left_mb_intra
				bs_h[4*i+j]=4;
			else if ((i!=0) && (slice_info[107]))	// is_intra
				bs_h[4*i+j]=3;
			else if (slice_info[blk4x4StrIdx] || slice_info[blk4x4StrIdx-1])	// cbp
				bs_h[4*i+j]=2;
			else
				bs_h[4*i+j] = condition0;
		}//for(j=0;j<4;j++)//part
	}

	for(i=0; i<4; i++)//vertical line
	{
		for(j=0; j<4; j++)//part
		{
			blk4x4StrIdx = 13 + j + i*5;
			ref0_p = slice_info[blk4x4StrIdx-5];
			ref0_q = slice_info[blk4x4StrIdx];
			blk4x4StrIdx = 44 + 2*j + i*10;
			mv0_p[0] = slice_info[blk4x4StrIdx-10];
			mv0_p[1] = slice_info[blk4x4StrIdx-9];
			mv0_q[0] = slice_info[blk4x4StrIdx];
			mv0_q[1] = slice_info[blk4x4StrIdx+1];
			condition0 = (ref0_p!=ref0_q)||(abs(mv0_p[0]-mv0_q[0])>=4)||(abs(mv0_p[1]-mv0_q[1])>=4);
			
			blk4x4StrIdx = 88 + j + i*5;
			if ((i==0) && (!filter_top_edge))
				bs_v[4*i+j]=0;
			else if ((i==0) && (slice_info[107] || slice_info[109]))		// is_intra || top_mb_intra
				bs_v[4*i+j]=4;
			else if ((i!=0) && (slice_info[107]))	// is_intra
				bs_v[4*i+j]=3;
			else if (slice_info[blk4x4StrIdx] || slice_info[blk4x4StrIdx-5])	// cbp
				bs_v[4*i+j]=2;
			else
				bs_v[4*i+j] = condition0;
		}//for(j=0;j<4;j++)//part
	}
	
	cmd = ( ((cur_mb_x&0x7f) << 25) | ((slice_info[112]&0x3f) << 16) | ((slice_info[111]&0x3f) << 8) | ((slice_info[110]&0x3f) << 0) );
	VSP_WRITE_REG (VSP_DBK_REG_BASE+HDBK_MB_INFO_OFF, cmd, "configure mb information");
	cmd = ((0 & 0x1f) << 21) | ((slice_info[113] & 0x1f) << 16) | ((slice_info[114] & 0x1f) << 8) | ((slice_info[115] & 0x1f) << 0);
	VSP_WRITE_REG (VSP_DBK_REG_BASE+HDBK_PARS_OFF, cmd, "configure dbk parameter");
	cmd = (bs_h[7]<<28)|(bs_h[6]<<24)|(bs_h[5]<<20)|(bs_h[4]<<16)|(bs_h[3]<<12)|(bs_h[2]<<8)|(bs_h[1]<<4)|bs_h[0]; //32b
	VSP_WRITE_REG(VSP_DBK_REG_BASE+HDBK_BS_H0_OFF, cmd, "configure bs h0"); 
	cmd = (bs_h[15]<<28)|(bs_h[14]<<24)|(bs_h[13]<<20)|(bs_h[12]<<16)|(bs_h[11]<<12)|(bs_h[10]<<8)|(bs_h[9]<<4)|bs_h[8]; //32b
	VSP_WRITE_REG(VSP_DBK_REG_BASE+HDBK_BS_H1_OFF, cmd, "configure bs h1");
	cmd = (bs_v[7]<<28)|(bs_v[6]<<24)|(bs_v[5]<<20)|(bs_v[4]<<16)|(bs_v[3]<<12)|(bs_v[2]<<8)|(bs_v[1]<<4)|bs_v[0]; //32b
	VSP_WRITE_REG(VSP_DBK_REG_BASE+HDBK_BS_V0_OFF, cmd, "configure bs v0");
	cmd = (bs_v[15]<<28)|(bs_v[14]<<24)|(bs_v[13]<<20)|(bs_v[12]<<16)|(bs_v[11]<<12)|(bs_v[10]<<8)|(bs_v[9]<<4)|bs_v[8]; //32b
	VSP_WRITE_REG(VSP_DBK_REG_BASE+HDBK_BS_V1_OFF, cmd, "configure bs v1");
}
#endif