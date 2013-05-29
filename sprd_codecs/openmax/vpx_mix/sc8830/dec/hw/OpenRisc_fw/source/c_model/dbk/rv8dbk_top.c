/*dbk_top.c*/
#include <stdio.h>
#include "rdbk_global.h"
//#include "rvdec_mode.h"
#include "rdbk_mode.h"
#include "rv_dbk_trace.h"
#include "common_global.h"
//#include "rvdec_global.h"
#include "sc8810_video_header.h"

//now, dbk c-model only support two-plane.
#if defined (REAL_DEC)
void dbk_module_rv8 (/*RV_DECODER_T * pDecoder, MB_MODE_T * pmbmd, uint8 ** pMBYUV, uint8 **pFrm*/)
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

	uint32		uv_data;
	uint32	*	src_ptr;
	uint32	*	dst_ptr;

	int			addr;
	
	int			uv;
	uint32		four_pix;
	uint32		four_pix_u;
	uint32		four_pix_v;
	int			addr_offset;
	uint8		pix_u0, pix_u1, pix_u2, pix_u3;
	uint8		pix_v0, pix_v1, pix_v2, pix_v3;

	uint32	*	frame_y_ptr = NULL;

	int32		mb_x;
	int32		mb_y;
	int32		mb_num_x	= ((g_glb_reg_ptr->VSP_CFG1) & 0xff);
	int32		mb_num_y	= ((g_glb_reg_ptr->VSP_CFG1 >> 12) & 0xff);
	uint8	*	pFrm[3];

	
		mb_x = (g_glb_reg_ptr->VSP_CTRL0) & 0x7f;//weihu for 1080p//3f
		mb_y = (g_glb_reg_ptr->VSP_CTRL0 >> 8) & 0x7f;//weihu for 1080p//3f

		pFrm[0] =  g_rv_decoder_ptr->pCurRecFrame->pDecFrame->imgY;
		pFrm[1] =  g_rv_decoder_ptr->pCurRecFrame->pDecFrame->imgU;
		pFrm[2] =  g_rv_decoder_ptr->pCurRecFrame->pDecFrame->imgV;

		fst_row  = (mb_y == 0) ? 1 : 0;
		last_row = (mb_y == mb_num_y - 1) ? 1 : 0;
		fst_col  = (mb_x == 0) ? 1 : 0;
		last_col = (mb_x == mb_num_x - 1) ? 1 : 0;

		pic_width = mb_num_x*(MB_SIZE>>2);
		FPRINTF(g_fp_dbk_mb_tv,"mbx = %d, mby = %d\n",mb_x,mb_y);
		/************************************************************************/
		/* Copy current MB to Obuf. 
		   NOTE: THIS should be done in MBC module. Here is only for simulation.
		   */
		/************************************************************************/
		//copy y 
#if 0		
		addr = 26;
		for (i=0; i< 16; i++)
		{
#if 0//RDBK_IN_PIPELINE
			frm_pix_ptr = ((uint32 *)pMBYUV[0]) + 4*i;
#else
			frm_pix_ptr = ((uint32 *)pMBYUV[0]) + pic_width*i;
#endif
			for (j=0;j<4;j++)
			{
				vsp_rdbk_obuf[addr] = * frm_pix_ptr;
			
				FPRINTF(fp_rdbk_trace_file,"%08x\n",vsp_rdbk_obuf[addr]);
				addr ++;
				frm_pix_ptr ++;
			}
			addr +=2;
		}
		FPRINTF(fp_rdbk_trace_file,"output:\n");
		//copy u
		addr = U_OFFSET_OBUF_C + 18;
		for (i=0; i< 8; i++)
		{
#if 0//RDBK_IN_PIPELINE
			frm_pix_ptr = ((uint32 *)pMBYUV[1]) + 2*i;
#else
			frm_pix_ptr = ((uint32 *)pMBYUV[1]) + (pic_width>>1)*i;
#endif
			for (j=0;j<2;j++)
			{
				vsp_rdbk_obuf[addr] = * frm_pix_ptr;
				addr ++;
				frm_pix_ptr ++;
			}
			addr +=2;
		}

		//copy v
		addr = V_OFFSET_OBUF_C + 18;
		for (i=0; i< 8; i++)
		{
#if 0//RDBK_IN_PIPELINE
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
			addr +=2;
		}
#endif
		/***********************************************************************
					start filter_core_ctr to filter one MB 
		************************************************************************/

//		PrintMBBfFilter ();

		rv8dbk_core_ctr (mb_x,mb_num_x);

//		PrintMBAfFilter (mb_x);

		/***********************************************************************
							send out filtered pixels
		************************************************************************/
		/*send out Y component*/

		pic_y_offset	= mb_y*MB_SIZE*pic_width + mb_x*(MB_SIZE>>2);

		start_row		= fst_row  ? 4 : 0;
		end_row			= last_row ? 20 : 16;

		start_col		= fst_col  ? 2 : 1;
		end_col			= last_col ? 6 : 5;


		frm_pix_ptr		= ((uint32 *)pFrm[0]) + pic_y_offset - 2;


		frm_pix_ptr		= fst_row ? frm_pix_ptr : frm_pix_ptr - pic_width*4;	

		for (j = start_row; j < end_row; j++)
		{
			for (i = start_col; i < end_col; i++)
			{
				int		addr;			
				
				if (j < 16)
				{
					addr = j * 6 + i;
					frm_pix_ptr[i] = vsp_rdbk_obuf[addr];
				}
				else
				{
					/*note the last row of one MB is in g_dbk_line_buf other than in mb_dbk_buf*/
					addr = mb_x * 4 - 2 + (j & 0x3) * Y_LINE_BUF_WIDTH + i;
					frm_pix_ptr[i] = vsp_rdbk_lbuf[addr];				
				}	
				//fprintf(fp_rdbk_trace_file,"%08x\n",frm_pix_ptr[i]);
				FPRINTF(g_fp_dbk_mb_tv,"%08x\n",frm_pix_ptr[i]);
			}

			frm_pix_ptr += pic_width;
		}

		//send out uv 
		start_row		= fst_row  ? 4 : 0;
		end_row			= last_row ? 12 : 8;
		start_col		= fst_col  ? 2 : 1;
		end_col			= last_col ? 4 : 3;

	if (/*uv_interleaved*/1)
	{
		
	frm_pix_ptr = ((uint32 *)pFrm[1]) +mb_y*UV_MB_SIZE*(pic_width)+ mb_x * (MB_SIZE >>2)-4;
	frm_pix_ptr	= fst_row ? frm_pix_ptr : frm_pix_ptr - (pic_width)*4;
		for (j = start_row; j < end_row; j++)
		{
			for (i = start_col; i < end_col; i++)
			{
				if (j < 8)
				{
					addr_offset	= j * 4 + i;	
					four_pix_u	= *(vsp_rdbk_obuf + U_OFFSET_OBUF_C + addr_offset);	
					four_pix_v	= *(vsp_rdbk_obuf + V_OFFSET_OBUF_C + addr_offset);	
				}
				else
				{
					/*note the last row of one MB is in g_dbk_line_buf other than in dbk_out_buf*/
					addr_offset = mb_x* 2 - 2 + (j & 0x3)*UV_LINE_BUF_WIDTH + i;
					
					four_pix_u = *(vsp_rdbk_lbuf + U_OFFSET_LINE_BUF + addr_offset);
					four_pix_v = *(vsp_rdbk_lbuf + V_OFFSET_LINE_BUF + addr_offset);
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


		/*send U component*/
		pic_c_offset	= mb_y*MB_CHROMA_SIZE*(pic_width>>1) + mb_x*(MB_CHROMA_SIZE>>2);
		
		frm_pix_ptr	= ((uint32 *)pFrm[1]) + pic_c_offset - 2;

		
		frm_pix_ptr	= fst_row ? frm_pix_ptr : frm_pix_ptr - (pic_width>>1)*4;
		
		for (j = start_row; j < end_row; j++)
		{		
			for (i = start_col; i < end_col; i++)
			{
				
				if (j < 8)
				{
					addr_offset	= j * 4 + i;	
					four_pix_u	= *(vsp_rdbk_obuf + U_OFFSET_OBUF_C + addr_offset);	
				}
				else
				{
					/*note the last row of one MB is in g_dbk_line_buf other than in dbk_out_buf*/
					addr_offset = mb_x* 2 - 2 + (j & 0x3)*UV_LINE_BUF_WIDTH + i;
					
					four_pix_u = *(vsp_rdbk_lbuf + U_OFFSET_LINE_BUF + addr_offset); 
				}
				frm_pix_ptr[i] = four_pix_u;
			//	fprintf(fp_rdbk_trace_file,"%08x\n",frm_pix_ptr[i]);
			}
			
			frm_pix_ptr += (pic_width>>1);
		}

		/*send V component*/
		frm_pix_ptr	= ((uint32 *)pFrm[2]) + pic_c_offset - 2;

		
		frm_pix_ptr	= fst_row ? frm_pix_ptr : frm_pix_ptr - (pic_width>>1)*4;
		
		for (j = start_row; j < end_row; j++)
		{		
			for (i = start_col; i < end_col; i++)
			{
				
				if (j < 8)
				{
					addr_offset	= j * 4 + i;	
					four_pix_v	= *(vsp_rdbk_obuf + V_OFFSET_OBUF_C + addr_offset);				
				}
				else
				{
					/*note the last row of one MB is in g_dbk_line_buf other than in dbk_out_buf*/
					addr_offset = mb_x* 2 - 2 + (j & 0x3)*UV_LINE_BUF_WIDTH + i;
					
					four_pix_v = *(vsp_rdbk_lbuf + V_OFFSET_LINE_BUF + addr_offset);
				}
				frm_pix_ptr[i] = four_pix_v;

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
				if (j < 8)
				{
					addr_offset	= j * 4 + i;	
					four_pix_u	= *(vsp_rdbk_obuf + U_OFFSET_OBUF_C + addr_offset);	
					four_pix_v	= *(vsp_rdbk_obuf + V_OFFSET_OBUF_C + addr_offset);	
				}
				else
				{
					/*note the last row of one MB is in g_dbk_line_buf other than in dbk_out_buf*/
					addr_offset = mb_x* 2 - 2 + (j & 0x3)*UV_LINE_BUF_WIDTH + i;
					
					four_pix_u = *(vsp_rdbk_lbuf + U_OFFSET_LINE_BUF + addr_offset);
					four_pix_v = *(vsp_rdbk_lbuf + V_OFFSET_LINE_BUF + addr_offset);
				}
				four_pix = (uv == 0) ? four_pix_u : four_pix_v;
				FPRINTF(g_fp_dbk_mb_tv,"%08x\n",four_pix);
			}

		}	
		
	}
		/************************************************************************
		1. polling MBC done, 
		2. copy pixels in vsp_dbk_out_bfr to vsp_mbc_out_bfr
		for next MB's post-filter
		3. switch buffer
		*************************************************************************/
		/*copy Y component*/
		src_ptr = vsp_rdbk_obuf + 5;
		dst_ptr = vsp_mbc_out_bfr + 1;
		for (i = 0; i < 20; i++)
		{
			uint32	src_data;
			if (i <16)
			{
				src_data = src_ptr[0];
			}
			else
			{
				src_data = *(vsp_rdbk_lbuf + mb_x * 4 + (i & 3) * Y_LINE_BUF_WIDTH + 3);
			}

			dst_ptr[0] = src_data;

			src_ptr += 6;
			dst_ptr += 6;
		}

		/*copy U component*/
		src_ptr = vsp_rdbk_obuf + U_OFFSET_OBUF_C + 3;
		dst_ptr = vsp_mbc_out_bfr + U_OFFSET_OBUF_C + 1;
		for (i = 0; i < 12; i++)
		{
			uint32	src_data;
			if (i < 8)
			{
				src_data = src_ptr[0];
			}
			else
			{
				src_data = *(vsp_rdbk_lbuf + U_OFFSET_LINE_BUF + mb_x*2 + (i & 3) * UV_LINE_BUF_WIDTH + 1);
			}
			
			dst_ptr[0] = src_data;
			
			src_ptr += 4;
			dst_ptr += 4;
		}

		/*copy V component*/
		src_ptr = vsp_rdbk_obuf + V_OFFSET_OBUF_C + 3;
		dst_ptr = vsp_mbc_out_bfr + V_OFFSET_OBUF_C + 1;
		for (i = 0; i < 12; i++)
		{
			uint32	src_data;
			if (i < 8)
			{
				src_data = src_ptr[0];
			}
			else
			{
				src_data = *(vsp_rdbk_lbuf + V_OFFSET_LINE_BUF	 + mb_x*2 + (i & 3) * UV_LINE_BUF_WIDTH + 1);
			}
			
			dst_ptr[0] = src_data;
			
			src_ptr += 4;
			dst_ptr += 4;
		}
}
#endif
