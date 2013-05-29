#include <stdio.h>
#include "rdbk_global.h"
//#include "rvdec_mode.h"
#include "rdbk_mode.h"
#include "rv_dbk_trace.h"
#include "common_global.h"
//#include "rvdec_global.h"
#include "sc8810_video_header.h"


int	g_nFrame = 0;

int	g_nMb = 0;

#if defined (REAL_DEC)
void rdbk_module_rv9()
{


	int16	i, j;
	uint32	val;
	int32	mb_posx;
	int32	mb_posy;

	int		fst_row;
	int		last_row;
	int		fst_col;
	int		last_col;

	int		start_row;
	int		end_row	;
	int		start_col;
	int		end_col	;

	int		addr,obuf_addr,lbuf_addr;
	int		pic_width;
	uint32	*frm_pix_ptr;
	uint8	*pFrm[3];
	uint32  four_pix_u;
	uint32  four_pix_v;
	uint32	four_pix;
	uint8	pix_u0, pix_u1, pix_u2, pix_u3;
 	uint8	pix_v0, pix_v1, pix_v2, pix_v3;
	int		uv;
	int32 uv_interleaved	= ((g_ahbm_reg_ptr->AHBM_BURST_GAP >> 8) & 0x01);

	//return;

	pFrm[0] =  g_rv_decoder_ptr->pCurRecFrame->pDecFrame->imgY;
	pFrm[1] =  g_rv_decoder_ptr->pCurRecFrame->pDecFrame->imgU;
	pFrm[2] =  g_rv_decoder_ptr->pCurRecFrame->pDecFrame->imgV;
	mb_posx = (g_glb_reg_ptr->VSP_CTRL0) & 0x7f;////weihu for 1080p//3f
	mb_posy = (g_glb_reg_ptr->VSP_CTRL0 >> 8) & 0x7f;////weihu for 1080p//3f

	fst_row	= (g_dbk_reg_ptr->rdbk_cfg6 >>21) & 0x1;
	last_row= (g_dbk_reg_ptr->rdbk_cfg6 >>22) & 0x1;
	fst_col = (g_dbk_reg_ptr->rdbk_cfg6 >>23) & 0x1;
	last_col= (g_dbk_reg_ptr->rdbk_cfg6 >>24) & 0x1;

	pic_width = ((g_glb_reg_ptr->VSP_CFG1) & 0xff)*(MB_SIZE>>2);
	//pic_width =  (pDecoder->uMbNumX)*(MB_SIZE>>2);
	//pic_width	= (pDecoder->uFrameExtendWidth) >>2;


	/************************************************************************/
	/* READ CURRENT MB from file to RDBK_OBUF                               */
	/************************************************************************/
	// Read Y 
#if 0 
	addr = 26;//6*4+2
	for (i=0;i<16;i++)
	{
#if RDBK_IN_PIPELINE
		frm_pix_ptr = ((uint32 *)pMBYUV[0]) + 4*i;
#else
		frm_pix_ptr = ((uint32 *)pMBYUV[0]) + pic_width*i;
#endif
		for (j=0;j<4;j++)
		{
		 	vsp_rdbk_obuf[addr] = * frm_pix_ptr;
			fprintf(fp_rdbk_trace_file,"%08x\n",vsp_rdbk_obuf[addr]);
			frm_pix_ptr ++;
			addr ++;
		}
		addr += 2;
	}
	// Read U
	addr = 138;//120+4*4+2
	for (i=0;i<8;i++)
	{
#if RDBK_IN_PIPELINE
		frm_pix_ptr = ((uint32 *)pMBYUV[1]) + 2*i;
#else
		frm_pix_ptr = ((uint32 *)pMBYUV[1]) + (pic_width>>1)*i;
#endif
		for (j=0;j<2;j++)
		{
			vsp_rdbk_obuf[addr] = * frm_pix_ptr;
			//fprintf(fp_rdbk_trace_file,"%08x\n",vsp_rdbk_obuf[addr]);
			addr ++;
			frm_pix_ptr ++;
		}
		addr += 2;
	}	


	// Read V
	addr = 186;//168+4*4+2
	for (i=0;i<8;i++)
	{
#if RDBK_IN_PIPELINE
		frm_pix_ptr = ((uint32 *)pMBYUV[2]) + 2*i;
#else
		frm_pix_ptr = ((uint32 *)pMBYUV[2]) + (pic_width>>1)*i;
#endif
		
		for (j=0;j<2;j++)
		{
			vsp_rdbk_obuf[addr] = * frm_pix_ptr;
			addr ++;
			frm_pix_ptr ++;
		}
		addr += 2;
	}	


#endif //end of read mb to Obuffer
	
#if RDBK_TRACE_ON
//Printf input_data.txt and config.txt
	Print_input_data(g_nMb);
	Print_config(g_nMb);
#endif

// 	/************************************************************************/
// 	/* read linebuf to T0 T1 T2 T3 of outbuf                                 */
// 	/************************************************************************/
// 
// 	//y
// #if 0
// 	for (j = 0; j<4; j++)
// 	{
// 		for (i=2;i<6;i++)
// 		{
// 			addr = mb_posx * 4 + j  * Y_LINE_BUF_WIDTH + i - 2;
// 			obuf_addr = j*6 + i;
//  	 		vsp_rdbk_obuf[obuf_addr]= vsp_rdbk_lbuf[addr] ;
// 		}
// 	}
// 
// 
// 	// u
// 	for (j = 0; j<4; j++)
// 	{
// 		for (i=2;i<4;i++)
// 		{
// 			addr = mb_posx * 2 + j  * UV_LINE_BUF_WIDTH + i + U_OFFSET_LINE_BUF -2;
// 			obuf_addr = j*4 + i + U_OFFSET_OBUF_C;
//  	 		vsp_rdbk_obuf[obuf_addr]= vsp_rdbk_lbuf[addr];
// 		}
// 	}
// 
// 	// v
// 	for (j = 0; j<4; j++)
// 	{
// 		for (i=2;i<4;i++)
// 		{
// 			addr = mb_posx * 2 + j  * UV_LINE_BUF_WIDTH + i + V_OFFSET_LINE_BUF -2;
// 			obuf_addr = j*4 + i + V_OFFSET_OBUF_C;
//  	 		vsp_rdbk_obuf[obuf_addr]= vsp_rdbk_lbuf[addr] ;
// 		}
// 	}
// #endif

	/************************************************************************/
	/* RDBK core ctrl                                                       */
	/************************************************************************/
  	if (mb_posx == 0 && mb_posy == 12 && g_nFrame == 61)
  	{
	//	printf("");
  	}
	rdbk_core_ctr_rv9();
#if RDBK_TRACE_ON
//Printf output_data.txt
	if (1)
	{
	Print_output_data(g_nMb,last_row,last_col);
	}
#endif

	/************************************************************************/
	/* write out filtered context to frame		                            */
	/************************************************************************/
	start_row		= fst_row  ? 4 : 0;
	end_row			= last_row ? 20 : 16;

	start_col		= fst_col  ? 2 : 0;
	end_col			= last_col ? 6 : 4;	
	
	//y
	frm_pix_ptr = ((uint32 *)pFrm[0]) +mb_posy*MB_SIZE*pic_width+ mb_posx * (MB_SIZE >>2)-2;
	frm_pix_ptr	= fst_row ? frm_pix_ptr : frm_pix_ptr - pic_width*4;	

		for (j = start_row; j < end_row; j++)
		{
			for (i = start_col; i < end_col; i++)
			{
				if (j<4)
				{
					//Output TL0 TL1 T0 T1 from Tbuf
					addr = j * 6 + i;
					frm_pix_ptr[i] = vsp_rdbk_tbuf[addr];
				}
				else if (j<16)
				{
					//Output L0 L1 L2 L3 L4 L5 0 1 4 5 8 9 from Obuf
					addr = j * 6 + i;
					frm_pix_ptr[i] = vsp_rdbk_obuf[addr];
				}
				else
				{
					//Frame bottom. Last blk row is also output from Obuf
					addr = j * 6 + i;
					frm_pix_ptr[i] = vsp_rdbk_obuf[addr];
				}
				
				FPRINTF(g_fp_dbk_mb_tv,"%08x\n",frm_pix_ptr[i]);
			}

			frm_pix_ptr += pic_width;
		}	

	//u v
	start_row		= fst_row  ? 4 : 0;
	end_row			= last_row ? 12 : 8;

	start_col		= fst_col  ? 2 : 0;
	end_col			= last_col ? 4 : 2;	


	if (/*uv_interleaved*/1)
	{
		
	frm_pix_ptr = ((uint32 *)pFrm[1]) +mb_posy*UV_MB_SIZE*(pic_width)+ mb_posx * (MB_SIZE >>2)-4;
	frm_pix_ptr	= fst_row ? frm_pix_ptr : frm_pix_ptr - (pic_width)*4;
		for (j = start_row; j < end_row; j++)
		{
			for (i = start_col; i < end_col; i++)
			{
				if (j<4)
				{
					//Output TL0 TL1 from Tbuf
					addr = j * 4 + i + U_OFFSET_TBUF_C;
					four_pix_u = vsp_rdbk_tbuf[addr];
					addr = j * 4 + i + V_OFFSET_TBUF_C;
					four_pix_v = vsp_rdbk_tbuf[addr];
				}
				else if (j<8)
				{
					//Output L0 L1 from Obuf
					addr = j * 4 + i + U_OFFSET_OBUF_C;
					four_pix_u = vsp_rdbk_obuf[addr];
					addr = j * 4 + i + V_OFFSET_OBUF_C;
					four_pix_v = vsp_rdbk_obuf[addr];
				}
				else
				{
					//Frame bottom. Last blk row is output from OBuf
					addr = j * 4 + i + U_OFFSET_OBUF_C;
					four_pix_u = vsp_rdbk_obuf[addr];
					addr = j * 4 + i + V_OFFSET_OBUF_C;
					four_pix_v = vsp_rdbk_obuf[addr];
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

			}

			frm_pix_ptr += (pic_width);
		}	
		
	} 
	else
	{

	frm_pix_ptr = ((uint32 *)pFrm[1]) +mb_posy*UV_MB_SIZE*(pic_width>>1)+ mb_posx * (UV_MB_SIZE >>2)-2;
	frm_pix_ptr	= fst_row ? frm_pix_ptr : frm_pix_ptr - (pic_width>>1)*4;
		for (j = start_row; j < end_row; j++)
		{
			for (i = start_col; i < end_col; i++)
			{
				if (j<4)
				{
					//Output TL0 TL1 from Tbuf
					addr = j * 4 + i + U_OFFSET_TBUF_C;
					frm_pix_ptr[i] = vsp_rdbk_tbuf[addr];
				}
				else if (j<8)
				{
					//Output L0 L1 from Obuf
					addr = j * 4 + i + U_OFFSET_OBUF_C;
					frm_pix_ptr[i] = vsp_rdbk_obuf[addr];
				}
				else
				{
					//Frame bottom. Last blk row is output from OBuf
					addr = j * 4 + i + U_OFFSET_OBUF_C;
					frm_pix_ptr[i] = vsp_rdbk_obuf[addr];
				}			
			}

			frm_pix_ptr += (pic_width>>1);
		}	
		
	frm_pix_ptr = ((uint32 *)pFrm[2]) +mb_posy*UV_MB_SIZE*(pic_width>>1)+ mb_posx * (UV_MB_SIZE >>2)-2;
	frm_pix_ptr	= fst_row ? frm_pix_ptr : frm_pix_ptr - (pic_width>>1)*4;
		for (j = start_row; j < end_row; j++)
		{
			for (i = start_col; i < end_col; i++)
			{
				if (j<4)
				{
					//Output TL0 TL1 from Tbuf
					addr = j * 4 + i + V_OFFSET_TBUF_C;
					frm_pix_ptr[i] = vsp_rdbk_tbuf[addr];
				}
				else if (j<8)
				{
					//Output L0 L1 from Obuf
					addr = j * 4 + i + V_OFFSET_OBUF_C;
					frm_pix_ptr[i] = vsp_rdbk_obuf[addr];
				}
				else
				{
					//Frame bottom. Last blk row is output from LBuf
					addr = j * 4 + i + V_OFFSET_OBUF_C;
					frm_pix_ptr[i] = vsp_rdbk_obuf[addr];
				}			
			}

			frm_pix_ptr += (pic_width>>1);
		}	

	}

	/************************************************************************/
	/* ONLY FOR RTL VERFICATION UV											*/
	/************************************************************************/
	for (uv = 0 ; uv < 2; uv++)
	{
		for (j = start_row; j < end_row; j++)
		{
			for (i = start_col; i < end_col; i++)
			{
				if (j<4)
				{
					//Output TL0 TL1 from Tbuf
					addr = j * 4 + i + U_OFFSET_TBUF_C;
					four_pix_u = vsp_rdbk_tbuf[addr];
					addr = j * 4 + i + V_OFFSET_TBUF_C;
					four_pix_v = vsp_rdbk_tbuf[addr];
				}
				else if (j<8)
				{
					//Output L0 L1 from Obuf
					addr = j * 4 + i + U_OFFSET_OBUF_C;
					four_pix_u = vsp_rdbk_obuf[addr];
					addr = j * 4 + i + V_OFFSET_OBUF_C;
					four_pix_v = vsp_rdbk_obuf[addr];
				}
				else
				{
					//Frame bottom. Last blk row is output from OBuf
					addr = j * 4 + i + U_OFFSET_OBUF_C;
					four_pix_u = vsp_rdbk_obuf[addr];
					addr = j * 4 + i + V_OFFSET_OBUF_C;
					four_pix_v = vsp_rdbk_obuf[addr];
				}	
				four_pix = (uv == 0) ? four_pix_u : four_pix_v;
				FPRINTF(g_fp_dbk_mb_tv,"%08x\n",four_pix);
			}

		}	
		
	}


// 	if (last_row && last_col && g_nFrame == 1) 
// 	{
// 		//end of frame, write out frame buf
// 		for (j = 0 ; j< MB_SIZE * RDBK_FRM_MBNUM_Y; j++)
// 		{
// 	 		fwrite (g_rdbk_cmodel_frame[0]+j*pic_width, 4, pic_width, fp_rdbk_af_frame);
// 		}
// 		for (j = 0 ; j< MB_SIZE * RDBK_FRM_MBNUM_Y/2; j++)
// 		{
// 	 		fwrite (g_rdbk_cmodel_frame[1]+j*(pic_width/2), 4, pic_width/2, fp_rdbk_af_frame);
// 		}
// 		for (j = 0 ; j< MB_SIZE * RDBK_FRM_MBNUM_Y/2; j++)
// 		{
// 	 		fwrite (g_rdbk_cmodel_frame[2]+j*(pic_width/2), 4, pic_width/2, fp_rdbk_af_frame);
// 		}
// 
// 	}

// 	if (last_row && last_col) 
// 	{	
// 		PrintMBTrace(g_nFrame);
// 		g_nFrame ++ ;
// 	}
	/************************************************************************/
	/* write blk 14 in linebuf to outbuf. Note while in bottom edge, blk 14 is in Obuf
	/* write TL0 TL1 from Tbuf to Lbuf																		*/
	/* write 2 3 6 7 10 11 15 from obuf to obuf 
	/************************************************************************/
	// copy blk 14
	//y 
	for (j = 0; j<4; j++)
	{
		if (last_row)
		{
			addr = (j + 16) * 6 + 4;
			obuf_addr = 96/*6*4*4*/ + 6 * j;
			vsp_mbc_out_bfr[obuf_addr] = vsp_rdbk_obuf[addr];
		} 
		else
		{
			addr = mb_posx * 4 + j  * Y_LINE_BUF_WIDTH - 2 + 4;
			obuf_addr = 96/*6*4*4*/ + 6 * j;
			vsp_mbc_out_bfr[obuf_addr] = vsp_rdbk_lbuf[addr];
		}

	}
	//uv
	for (j = 0; j<4; j++)
	{
		if (last_row)
		{
			addr = (j + 8) * 4 + 2 + U_OFFSET_OBUF_C;
			obuf_addr  = 32/*4*4*2*/ + U_OFFSET_OBUF_C + 4 * j;
			vsp_mbc_out_bfr[obuf_addr] = vsp_rdbk_obuf[addr];
		} 
		else
		{
			addr = mb_posx * 2 + j  * UV_LINE_BUF_WIDTH - 2 + 2 + U_OFFSET_LINE_BUF;
			obuf_addr = 32/*4*4*2*/ + U_OFFSET_OBUF_C + 4 * j;
			vsp_mbc_out_bfr[obuf_addr] = vsp_rdbk_lbuf[addr];
		}

	}
	for (j = 0; j<4; j++)
	{
		if (last_row)
		{
			addr = (j + 8) * 4 + 2 + V_OFFSET_OBUF_C;
			obuf_addr = 32/*4*4*2*/ + V_OFFSET_OBUF_C + 4 * j;
			vsp_mbc_out_bfr[obuf_addr] = vsp_rdbk_obuf[addr];
		} 
		else
		{
			addr = mb_posx * 2 + j  * UV_LINE_BUF_WIDTH - 2 + 2 + V_OFFSET_LINE_BUF;
			obuf_addr = 32/*4*4*2*/ + V_OFFSET_OBUF_C + 4 * j;
			vsp_mbc_out_bfr[obuf_addr] = vsp_rdbk_lbuf[addr];
		}

	}

	//write TL0 TL1 from Tbuf to Lbuf
	//y

	if (!last_col)
	{
		for (j = 0; j< 4; j++)
		{
			for (i = 0; i< 2 ;i ++)
			{
				addr = j * 6 + (4 + i);
				lbuf_addr = mb_posx * 4 + j  * Y_LINE_BUF_WIDTH - 2 + (4 +i);
				vsp_rdbk_lbuf[lbuf_addr] = vsp_rdbk_tbuf[addr];
			}
		}
		//uv
		for (j=0 ; j<4 ;j++)
		{
			for (i = 0; i<2 ;i++)
			{
				addr = j * 4 + (2 + i) + U_OFFSET_TBUF_C;
				lbuf_addr = mb_posx * 2 + j  * UV_LINE_BUF_WIDTH - 2 + (2 +i) + U_OFFSET_LINE_BUF;
				vsp_rdbk_lbuf[lbuf_addr] = vsp_rdbk_tbuf[addr];
			}
		}

		for (j=0 ; j<4 ;j++)
		{
			for (i = 0; i<2 ;i++)
			{
				addr = j * 4 + (2 + i) + V_OFFSET_TBUF_C;
				lbuf_addr = mb_posx * 2 + j  * UV_LINE_BUF_WIDTH - 2 + (2 + i) + V_OFFSET_LINE_BUF;
				vsp_rdbk_lbuf[lbuf_addr] = vsp_rdbk_tbuf[addr];
			}
		}
	}

		
// 	for (j = 0; j< 4; j++)
// 	{
// 		for (i = 0; i<2 ;i ++)
// 		{
// 			addr = j * 6 + (4 + i);
// 			obuf_addr = j * 6 + i;
// 			vsp_rdbk_obuf[obuf_addr] = vsp_rdbk_tbuf[addr];
// 		}
// 	}
// 	//uv
// 	for (j = 0; j< 4; j++)
// 	{
// 		for (i = 0; i<2 ;i ++)
// 		{
// 			addr = j * 4 + (2 + i) + U_OFFSET_TBUF_C;
// 			obuf_addr = j * 4 + i + U_OFFSET_OBUF_C;
// 			vsp_rdbk_obuf[obuf_addr] = vsp_rdbk_tbuf[addr];
// 		}
// 	}
// 	for (j = 0; j< 4; j++)
// 	{
// 		for (i = 0; i<2 ;i ++)
// 		{
// 			addr = j * 4 + (2 + i) + V_OFFSET_TBUF_C;
// 			obuf_addr = j * 4 + i + V_OFFSET_OBUF_C;
// 			vsp_rdbk_obuf[obuf_addr] = vsp_rdbk_tbuf[addr];
// 		}
// 	}

	//write 2 3 6 7 10 11 15 from obuf to obuf 
	//y
	for (j = 4; j < 20; j++)
	{
		for (i = 0; i < 2; i++)
		{
			if (j < 16 || i>0) // not blk 14
			{
				addr = 6 * j + (4+i);
				obuf_addr = 6 * j + i;
				vsp_mbc_out_bfr[obuf_addr] = vsp_rdbk_obuf[addr];
			}
		}
	}

	//uv
	for (j = 4; j < 12; j++)
	{
		for (i = 0 ; i< 2; i++)
		{
			if (j<8 || i>0)
			{
				addr = 4 * j + (2+i) + U_OFFSET_OBUF_C;
				obuf_addr = 4 * j + i + U_OFFSET_OBUF_C;
				vsp_mbc_out_bfr[obuf_addr] = vsp_rdbk_obuf[addr];
			}
		}
	}

	for (j = 4; j < 12; j++)
	{
		for (i = 0 ; i< 2; i++)
		{
			if (j<8 || i>0)
			{
				addr = 4 * j + (2+i) + V_OFFSET_OBUF_C;
				obuf_addr = 4 * j + i + V_OFFSET_OBUF_C;
				vsp_mbc_out_bfr[obuf_addr] = vsp_rdbk_obuf[addr];
			}
		}
	}

	g_nMb ++ ;


}

#endif