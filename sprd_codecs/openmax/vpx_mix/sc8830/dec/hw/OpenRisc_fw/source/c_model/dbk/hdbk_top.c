/*dbk_top.c*/
#ifdef VP8_DEC
#include "vp8dbk_global.h"
#include "vp8_test_vectors.h"
#else
#include "sc8810_video_header.h"
#endif


//now, dbk c-model only support two-plane.
#ifdef VP8_DEC
void dbk_module(unsigned char *y, unsigned char *u, unsigned char *v,
             int ystride, int uv_stride)
#else
void dbk_module()
#endif
{
	int			i;
	int			j;
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

	uint32	*	src_ptr;
	uint32	*	dst_ptr;
	
//	int			uv;
//	uint32		four_pix;
	uint32		four_pix_u;
	uint32		four_pix_v;
	int			addr_offset;
	//uint8		pix_u0, pix_u1, pix_u2, pix_u3;
	//uint8		pix_v0, pix_v1, pix_v2, pix_v3;

	uint32	*	frame_y_ptr = NULL;

	int32		mb_x;
	int32		mb_y;
	int32		mb_num_x	= (g_glb_reg_ptr->VSP_CFG1 >> 0 ) & 0xff;
	int32		mb_num_y	= (g_glb_reg_ptr->VSP_CFG1 >> 12) & 0xff;

	int32		post_filter_en = (g_dbk_reg_ptr->DBK_CFG >> 2) & 0x01; 
	
	int32		addr;	

#if defined(JPEG_DEC)
	mb_x		= (g_dbk_reg_ptr->DBK_SID_CFG >> 0) & 0x3ff;
	mb_y		= (g_dbk_reg_ptr->DBK_SID_CFG >> 16) & 0x3ff;
#else
	mb_x		= (g_glb_reg_ptr->VSP_CTRL0 >> 0) & 0x7f;
	mb_y		= (g_glb_reg_ptr->VSP_CTRL0 >> 8) & 0x7f;
#endif

	vid_standard	= (g_glb_reg_ptr->VSP_CFG0 >> 8) & 0xf;
	h264_fmo_mode   = (g_dbk_reg_ptr->DBK_CFG >> 3) & 0x1;

	if(VSP_JPEG != vid_standard)
	{
	#if defined(MPEG4_DEC)||defined(H264_DEC)||defined(MPEG4_ENC)||defined(VP8_DEC)
		dbk_eb			= (g_glb_reg_ptr->VSP_CFG0 >> 12) & 0x1;	// DBK_EB, active high

	
//		if(g_trace_enable_flag&TRACE_ENABLE_DBK)
//		{
//		 	fprintf (g_dbk_trace_fp, "\nframe: %d, mb_x: %d, mb_y: %d\n", g_nFrame_dec_h264, mb_x, mb_y);
//		}
		
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

#ifdef VP8_DEC
		pic_width = ystride >>2;


		/************************************************************************/
		/* Copy current MB to Obuf. 
		   NOTE: THIS should be done in MBC module. Here is only for simulation.
		   */
		/************************************************************************/
		//copy y 
#if 1		
		addr = 26;
		for (i=0; i< 16; i++)
		{
#if 0//RDBK_IN_PIPELINE
			frm_pix_ptr = ((uint32 *)pMBYUV[0]) + 4*i;
#else
			frm_pix_ptr = ((uint32 *)y + pic_width * i);
#endif
			for (j=0;j<4;j++)
			{
				vsp_dbk_out_bfr[addr] = * frm_pix_ptr;
			
			//	FPRINTF(fp_rdbk_trace_file,"%08x\n",vsp_rdbk_obuf[addr]);
				addr ++;
				frm_pix_ptr ++;
			}
			addr +=2;
		}
	//	FPRINTF(fp_rdbk_trace_file,"output:\n");
		//copy u
		addr = U_OFFSET_BUF_C + 18;
		for (i=0; i< 8; i++)
		{
#if 0//RDBK_IN_PIPELINE
			frm_pix_ptr = ((uint32 *)pMBYUV[1]) + 2*i;
#else
			frm_pix_ptr = ((uint32 *)u) + (pic_width>>1) * i;
#endif
			for (j=0;j<2;j++)
			{
				vsp_dbk_out_bfr[addr] = * frm_pix_ptr;
				addr ++;
				frm_pix_ptr ++;
			}
			addr +=2;
		}

		//copy v
		addr = V_OFFSET_BUF_C + 18;
		for (i=0; i< 8; i++)
		{
#if 0//RDBK_IN_PIPELINE
			frm_pix_ptr = ((uint32 *)pMBYUV[2]) + 2*i;
#else
			frm_pix_ptr = ((uint32 *)v) + (pic_width>>1) * i;
#endif
			for (j=0;j<2;j++)
			{
				vsp_dbk_out_bfr[addr] = * frm_pix_ptr;
				addr ++;
				frm_pix_ptr ++;
			}
			addr +=2;
		}
#endif

#else	// !VP8_DEC
		pic_width = mb_num_x*(MB_SIZE>>2);
		
			if ((vid_standard == ITU_H263) || (vid_standard == VSP_MPEG4) || (vid_standard == FLV_H263) || (h264_fmo_mode == TRUE))  //mpeg4 output reconstructed frame
		{
		#if defined(MPEG4_DEC)
			g_frame_y_ptr = (uint32 *)(g_dec_vop_mode_ptr->pCurRecFrame->pDecFrame->imgY);
			g_frame_c_ptr = (uint32 *)(g_dec_vop_mode_ptr->pCurRecFrame->pDecFrame->imgU);
		#elif defined(MPEG4_ENC)
			g_frame_y_ptr = (uint32 *)(g_enc_vop_mode_ptr->pYUVRecFrame->imgY);
			g_frame_c_ptr = (uint32 *)(g_enc_vop_mode_ptr->pYUVRecFrame->imgU);		
		#elif defined(H264_DEC)	
			g_frame_y_ptr		= ((uint32 *)g_dec_picture_ptr->imgY);
			g_frame_c_ptr		= ((uint32 *)g_dec_picture_ptr->imgU);
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
						fprintf (g_fp_dbk_mb_tv, "%08x\n", frm_pix_ptr[i]); 
					}
				}
				
				mb_pix_ptr  += 6;
				frm_pix_ptr += pic_width;
			}
			
			/*copy UV component*/
			frm_pix_ptr  = g_frame_c_ptr + pic_c_offset;
			mb_pix_u_ptr = (uint8 *)(vsp_dbk_out_bfr + U_OFFSET_BUF_C+18);
			mb_pix_v_ptr = (uint8 *)(vsp_dbk_out_bfr + V_OFFSET_BUF_C+18);
			
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
			
			//only for printf U and V DBK out data for verification
			for (uv = 0 ; uv < 2; uv++)
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
							fprintf (g_fp_dbk_mb_tv, "%08x\n", four_pix);
						}
					}
				}
			}
		}

		//if(g_trace_enable_flag&TRACE_ENABLE_DBK)
		//{
		//	fprintf (g_dbk_trace_fp, "\npixels before deblocking: \n");
		//}
		//DBK_PrintOneMB (mb_y, mb_x);
		g_dbk_reg_ptr->DBK_SID_CFG = (mb_y<<16) | mb_x;
#endif	// VP8_DEC		


		/***********************************************************************
					start filter_core_ctr to filter one MB 
		************************************************************************/
#ifdef VP8_DEC
#else		
		if (((vid_standard == VSP_H264) && (h264_fmo_mode == TRUE)) || (!post_filter_en))
			return;
#endif

//		PrintMBBfFilter ();

		hdbk_core_ctr (mb_x);

//		PrintMBAfFilter (mb_x);

		/***********************************************************************
							send out filtered pixels
		************************************************************************/
		/*send out Y component*/
        //if(g_trace_enable_flag&TRACE_ENABLE_DBK)
		//{
		//	fprintf (g_dbk_trace_fp, "\npixels after deblocking: \n");
		//}
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
	#elif defined(VP8_DEC)
		frm_pix_ptr = ((uint32 *)u) - 2;
	#endif

#ifdef VP8_DEC	
	//Not UV interleaved
	{


		/*send U component*/
		pic_c_offset	= mb_y*MB_CHROMA_SIZE*(pic_width>>1) + mb_x*(MB_CHROMA_SIZE>>2);
		
//		frm_pix_ptr	= ((uint32 *)pFrm[1]) + pic_c_offset - 2;

		
		frm_pix_ptr	= fst_row ? frm_pix_ptr : frm_pix_ptr - (pic_width>>1)*4;
		
		for (j = start_row; j < end_row; j++)
		{		
			for (i = start_col; i < end_col; i++)
			{
				int		addr1;	//weihu		
				if (j < 8)
				{
					addr_offset	= j * 4 + i;	
					four_pix_u	= *(vsp_dbk_out_bfr + U_OFFSET_BUF_C + addr_offset);	
				}
				else
				{
					/*note the last row of one MB is in g_dbk_line_buf other than in dbk_out_buf*/
					addr_offset = mb_x* 2 - 2 + (j & 0x3)*LINE_WIDTH_C + i;
					
					four_pix_u = *(g_dbk_line_buf + U_OFFSET_LINE_BUF + addr_offset); 
					addr1 = U_OFFSET_BUF_C + j * 4 + i;//weihu
					vsp_dbk_out_bfr[addr1]=*(g_dbk_line_buf + U_OFFSET_LINE_BUF + addr_offset);//weihu
				}
				frm_pix_ptr[i] = four_pix_u;
			//	fprintf(fp_rdbk_trace_file,"%08x\n",frm_pix_ptr[i]);
			}
			
			frm_pix_ptr += (pic_width>>1);
		}

		/*send V component*/

		//frm_pix_ptr	= ((uint32 *)pFrm[2]) + pic_c_offset - 2;
		frm_pix_ptr = ((uint32 *)v) - 2;
		
		frm_pix_ptr	= fst_row ? frm_pix_ptr : frm_pix_ptr - (pic_width>>1)*4;
		
		for (j = start_row; j < end_row; j++)
		{		
			for (i = start_col; i < end_col; i++)
			{
				int		addr1;	//weihu	
				if (j < 8)
				{
					addr_offset	= j * 4 + i;	
					four_pix_v	= *(vsp_dbk_out_bfr + V_OFFSET_BUF_C + addr_offset);				
				}
				else
				{
					/*note the last row of one MB is in g_dbk_line_buf other than in dbk_out_buf*/
					addr_offset = mb_x* 2 - 2 + (j & 0x3)*LINE_WIDTH_C + i;
					
					four_pix_v = *(g_dbk_line_buf + V_OFFSET_LINE_BUF + addr_offset);
					addr1 =V_OFFSET_BUF_C+ j * 4 + i;//weihu
					vsp_dbk_out_bfr[addr1]=*(g_dbk_line_buf + V_OFFSET_LINE_BUF + addr_offset);//weihu
				}
				frm_pix_ptr[i] = four_pix_v;

			}
			
			frm_pix_ptr += (pic_width>>1);
		}	
	
	}
#else
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
				
			//	fprintf (g_fp_dbk_mb_tv, "%08x\n", frm_pix_ptr[2*i+0]);
			//	fprintf (g_fp_dbk_mb_tv, "%08x\n", frm_pix_ptr[2*i+1]);
			}
			
			frm_pix_ptr += pic_width;
		}
#endif	
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
		if(g_nFrame_dec==FRAME_X)
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



void dbk_module_ppa (
					 char  dbk_out_buf[864],//216*4
					 char  mbc_out_buf[384],
					 int slice_info[40],
					 int dbk_para_buf[8], //8*32b
					 char decoder_format,//3b
					 char picwidthinMB,//7b
					 char picheightinMB//7b					 
					 )//weihu
{
       int i,j,cmd;
       char qp_top,qp_left,qp_y,cur_mb_x,cur_mb_y;//32b
	   char u_qp_offset,v_qp_offset,LFAlphaC0Offset,LFBetaOffset;
	   uint32	*	src_ptr;
	   uint32	*	dst_ptr;

	   u_qp_offset=slice_info[6];
	   v_qp_offset=slice_info[7];
	   LFAlphaC0Offset=slice_info[36];
	   LFBetaOffset=slice_info[37]; 
	   	   
	   qp_top=(dbk_para_buf[0]>>26)&0x3f;
	   qp_left=(dbk_para_buf[0]>>20)&0x3f;
	   qp_y=(dbk_para_buf[0]>>14)&0x3f;
	   cur_mb_x=(dbk_para_buf[0]>>7)&0x7f;
	   cur_mb_y=dbk_para_buf[0]&0x7f;
	   
	   cmd = ((cur_mb_x << 24) | (qp_top << 16) | (qp_left << 8) | (qp_y << 0));
	   VSP_WRITE_REG (VSP_DBK_REG_BASE+HDBK_MB_INFO_OFF, cmd, "configure mb information");
	     // FPRINTF (pFmoFile, "HDBK_MB_INFO: %x \n", cmd);//weihu
	   cmd = ((u_qp_offset & 0x1f) << 16) | ((LFAlphaC0Offset & 0x1f) << 8) | ((LFBetaOffset & 0x1f) << 0);
	   VSP_WRITE_REG (VSP_DBK_REG_BASE+HDBK_PARS_OFF, cmd, "configure dbk parameter");
         // FPRINTF (pFmoFile, "HDBK_PARS: %x \n", cmd);//weihu
	   VSP_WRITE_REG(VSP_DBK_REG_BASE+HDBK_BS_H0_OFF, dbk_para_buf[1], "configure bs h0"); 
	   VSP_WRITE_REG(VSP_DBK_REG_BASE+HDBK_BS_H1_OFF, dbk_para_buf[2], "configure bs h1");
	   VSP_WRITE_REG(VSP_DBK_REG_BASE+HDBK_BS_V0_OFF, dbk_para_buf[3], "configure bs v0");
	   VSP_WRITE_REG(VSP_DBK_REG_BASE+HDBK_BS_V1_OFF, dbk_para_buf[4], "configure bs v1");
	     // FPRINTF (pFmoFile, "HDBK_BS_H0: %x \n", dbk_para_buf[1]);//weihu
	     // FPRINTF (pFmoFile, "HDBK_BS_H1: %x \n", dbk_para_buf[2]);//weihu
	     // FPRINTF (pFmoFile, "HDBK_BS_V0: %x \n", dbk_para_buf[3]);//weihu
	     // FPRINTF (pFmoFile, "HDBK_BS_V1: %x \n", dbk_para_buf[4]);//weihu

	   /*copy Y component*/
	   src_ptr = (uint32*)(&(mbc_out_buf[0]));
	   dst_ptr = vsp_dbk_out_bfr + 2+6*4;
	   for (i = 0; i < 16; i++)
	   {
		   for (j = 0; j < 4; j++)  
		      dst_ptr[j] = src_ptr[j];
		   
		   src_ptr += 4;
		   dst_ptr += 6;
	   }
	   
	   /*copy U component*/
	   src_ptr = (uint32*)(&(mbc_out_buf[256]));
	   dst_ptr = vsp_dbk_out_bfr + U_OFFSET_BUF_C + 2+4*4;
	   for (i = 0; i < 8; i++)
	   {
		 	   
		   for (j = 0; j < 2; j++)  
			   dst_ptr[j] = src_ptr[j];
		   
		   src_ptr += 2;
		   dst_ptr += 4;
	   }
	   
	   /*copy V component*/
	   src_ptr = (uint32*)(&(mbc_out_buf[320]));
	   dst_ptr = vsp_dbk_out_bfr + V_OFFSET_BUF_C + 2+4*4;
	   for (i = 0; i < 8; i++)
	   {
		   
		   for (j = 0; j < 2; j++)  
			   dst_ptr[j] = src_ptr[j];
		   
		   src_ptr += 2;
		   dst_ptr += 4;
	   }
#ifdef VP8_DEC
#else
	   dbk_module();
#endif
	   src_ptr = vsp_dbk_out_bfr;
	   dst_ptr = (uint32*)(&(dbk_out_buf[0]));
	   for (i = 0; i < 216; i++)
	   {
             dst_ptr[i] = src_ptr[i];
	   }

}