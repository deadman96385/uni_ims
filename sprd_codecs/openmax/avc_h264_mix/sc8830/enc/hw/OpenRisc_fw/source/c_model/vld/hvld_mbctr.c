/*hvld_mbctr.c*/
#include "video_common.h"
#include "hvld_mode.h"
#include "hvld_global.h"
#include "hvld_test_vector.h"
#include "buffer_global.h"
#include "common_global.h"

int		g_mb_vld_cnt = 0;

void ComputeCBPIqtMbc (int mb_type, int cbp)
{
	uint32	cbp_iqt  = 0;
	uint32	cbp_mbc  = 0;
	uint32	cbp_luma_iqt = 0;
	uint32	cbp_luma_mbc = 0;
	uint32	cbp_uv;

	if (mb_type == H264_IPCM)
	{
		cbp_iqt = 0;
		cbp_mbc = 0xffffff;
	}
	else 
	{
		cbp_luma_iqt = ((((g_hvld_reg_ptr->nnz_blk_3 >>  0) & 0x1f) != 0) << 15) |
					   ((((g_hvld_reg_ptr->nnz_blk_3 >>  8) & 0x1f) != 0) << 14) |
					   ((((g_hvld_reg_ptr->nnz_blk_3 >> 16) & 0x1f) != 0) << 13) |
					   ((((g_hvld_reg_ptr->nnz_blk_3 >> 24) & 0x1f) != 0) << 12) |
					   ((((g_hvld_reg_ptr->nnz_blk_2 >>  0) & 0x1f) != 0) << 11) |
					   ((((g_hvld_reg_ptr->nnz_blk_2 >>  8) & 0x1f) != 0) << 10) |
					   ((((g_hvld_reg_ptr->nnz_blk_2 >> 16) & 0x1f) != 0) <<  9) |
					   ((((g_hvld_reg_ptr->nnz_blk_2 >> 24) & 0x1f) != 0) <<  8) |
					   ((((g_hvld_reg_ptr->nnz_blk_1 >>  0) & 0x1f) != 0) <<  7) |
					   ((((g_hvld_reg_ptr->nnz_blk_1 >>  8) & 0x1f) != 0) <<  6) |
					   ((((g_hvld_reg_ptr->nnz_blk_1 >> 16) & 0x1f) != 0) <<  5) |
					   ((((g_hvld_reg_ptr->nnz_blk_1 >> 24) & 0x1f) != 0) <<  4) |
					   ((((g_hvld_reg_ptr->nnz_blk_0 >>  0) & 0x1f) != 0) <<  3) |
					   ((((g_hvld_reg_ptr->nnz_blk_0 >>  8) & 0x1f) != 0) <<  2) |
					   ((((g_hvld_reg_ptr->nnz_blk_0 >> 16) & 0x1f) != 0) <<  1) |
					   ((((g_hvld_reg_ptr->nnz_blk_0 >> 24) & 0x1f) != 0) <<  0);

		cbp_luma_mbc =  (cbp_luma_iqt & 0xc3c3) | 
						(((cbp_luma_iqt >>  2) & 0x3) <<  4) | 
						(((cbp_luma_iqt >>  4) & 0x3) <<  2) | 
						(((cbp_luma_iqt >> 10) & 0x3) << 12) |
						(((cbp_luma_iqt >> 12) & 0x3) << 10);

		cbp_uv = g_hvld_reg_ptr->cbp_uv;

		cbp_iqt = (cbp_uv << 16) | cbp_luma_iqt;

		if (cbp > 15)
			cbp_iqt |= (1<<24);
		
		if (cbp > 31)
			cbp_iqt |= (1<<25);

		cbp_mbc = (cbp_uv << 16) | cbp_luma_mbc;
		
		if (mb_type == H264_IMB16X16)
		{
			cbp_mbc |= 0xffff;
		}
	}

	g_hvld_reg_ptr->cbp_iqt = cbp_iqt;
	g_hvld_reg_ptr->cbp_mbc = cbp_mbc;
}

void H264VldMBCtr (
				   /*input*/
				   /*output*/
				   )
{
#if defined(H264_DEC)
	#include "h264dec_global.h"

	int		cbp_luma;
	int		cbp_chroma;
	int		blk_id;
	int		cbp_8x8;		//current 8x8 block has coeff or not
	int		cbp_left;		//cbp for not decoded block8x8 
	int		error;
	int		start_pos;
	int		max_coeff_num;

	int		mb_type;
	int		cbp;
	int		lmb_avail;
	int		tmb_avail;

	mb_type		= g_hvld_reg_ptr->mb_info & 3;
	cbp			= (g_hvld_reg_ptr->mb_info >> 8) & 0x3f;
	lmb_avail	= (g_hvld_reg_ptr->mb_info >> 16) & 1;
	tmb_avail	= (g_hvld_reg_ptr->mb_info >> 17) & 1;

	if(g_trace_enable_flag&TRACE_ENABLE_VLD)	
	{
		FPRINTF (g_hvld_trace_fp, "frame: %d, mb_y: %d, mb_x: %d\n", g_nFrame_dec_h264, (g_glb_reg_ptr->VSP_CTRL0 >> 8) & 0xff, (g_glb_reg_ptr->VSP_CTRL0 >> 0) & 0xff);
	}

	if (mb_type != H264_IPCM)
	{
		if(g_trace_enable_flag&TRACE_ENABLE_VLD)	
		{
			FPRINTF (g_hvld_trace_fp, "mb type: %d, cbp: %d, left_avai: %d, top_avail: %d\n", 
				mb_type, cbp, lmb_avail, tmb_avail);
			
			FPRINTF (g_hvld_trace_fp, "top_nnz_y: %08x, left_nnz_y: %08x, tl_nnz_cb: %08x, tl_nnz_cr: %08x\n",
				g_hvld_reg_ptr->top_nnz_y, g_hvld_reg_ptr->left_nnz_y, g_hvld_reg_ptr->tl_nnz_cb, g_hvld_reg_ptr->tl_nnz_cr);
		}
	}

//	if ((g_nFrame_dec == 0) && (g_mb_x == 5) && (g_mb_y == 0))
//		printf ("");

	if (g_mb_vld_cnt == 5)
		printf ("");
	
	g_mb_vld_cnt++;	

	//clear dct/io buffer only for verification
	memset (vsp_dct_io_0, 0, 256*sizeof(uint32));
	
	if 	(mb_type == H264_IPCM)
	{
		H264_IpcmDec ();
		PrintfDCTBuf ();

		goto VLD_END;
	}

	if (mb_type == H264_IMB16X16)
	{
		/*decoding 4x4 luma DC*/
		if(g_trace_enable_flag&TRACE_ENABLE_VLD)	
		{
			FPRINTF (g_hvld_trace_fp, "DC of IMB16x16\n");
		}

		max_coeff_num = 16;

		H264VldBlk (LUMA_DC, 0, 0, max_coeff_num, lmb_avail, tmb_avail, &error);

		if (error)
		{
			/*set done and error flag*/
			g_hvld_reg_ptr->hvld_ctr = (1 << 1) | (1 << 2);
			goto VLD_END;
		}
	}

	/*reset nnz register array of current MB at the start vld signal*/
	NnzRegInit ();

	if (cbp == 0)
	{
		PrintfDCTBuf ();	

		ComputeCBPIqtMbc (mb_type, cbp);
		
		goto VLD_END;
	}

	cbp_luma	= cbp & 0xf;
	cbp_chroma	= cbp >> 4;
	
	if (cbp_luma)
	{
		if(g_trace_enable_flag&TRACE_ENABLE_VLD)	
		{
			FPRINTF (g_hvld_trace_fp, "AC of LUMA\n");
		}
	}

	if (cbp_luma > 0)
	{
		/*decoding luma AC coefficient*/
		blk_id = 0;

		while (1)
		{
			cbp_8x8 = (cbp_luma >> (blk_id>>2)) & 1;

			if (cbp_8x8)
			{
				if (blk_id == 8)
					printf ("");

				/*decoding one 4x4 block*/
				start_pos = (mb_type == H264_IMB16X16) ? 1 : 0;
				max_coeff_num  = (mb_type == H264_IMB16X16) ? 15 : 16;

				if(g_trace_enable_flag&TRACE_ENABLE_VLD)	
				{
					FPRINTF (g_hvld_trace_fp, "blk_id: %d\n", blk_id);
				}

				H264VldBlk (LUMA_AC, blk_id, start_pos, max_coeff_num, lmb_avail, tmb_avail, &error);

				if (error)
				{
					/*set done and error flag*/
					g_hvld_reg_ptr->hvld_ctr = (1 << 1) | (1 << 2);
					goto VLD_END;
				}

				blk_id += 1;
			}
			else
			{
				/*current 8x8 block is skipped*/
				blk_id += 4;
			}

			cbp_left = cbp_luma >> (blk_id >> 2);
			if ((blk_id == 16) | (cbp_left == 0))
				break;
		}

	}

	if (cbp_chroma == 0)
	{
		PrintfDCTBuf ();	
		
		goto VLD_END;
	}

	/*decoding chroma DC*/
	if(g_trace_enable_flag&TRACE_ENABLE_VLD)	
	{
		FPRINTF (g_hvld_trace_fp, "DC of CHROMA\n");
	}
	
	for (blk_id = 0; blk_id < 2; blk_id++)
	{
		/*decoding one 2x2 chroma DC block*/
		if(g_trace_enable_flag&TRACE_ENABLE_VLD)	
		{
			FPRINTF (g_hvld_trace_fp, "blk_id: %d\n", blk_id);
		}
		max_coeff_num = 4;

		H264VldBlk (CHROMA_DC, blk_id, 0, max_coeff_num, lmb_avail, tmb_avail, &error);

		if (error)
		{
			/*set done and error flag*/
			g_hvld_reg_ptr->hvld_ctr = (1 << 1) | (1 << 2);
			goto VLD_END;
		}
	}

	/*decoding chroma AC*/
	if (cbp_chroma & 2)
	{
		if(g_trace_enable_flag&TRACE_ENABLE_VLD)	
		{
			FPRINTF (g_hvld_trace_fp, "AC of CHROMA\n");
		}

		for (blk_id = 0; blk_id < 8; blk_id++)
		{
			/*decoding one 4x4 chroma AC block*/
			if (blk_id == 7)
				printf ("");

			if(g_trace_enable_flag&TRACE_ENABLE_VLD)	
			{
				FPRINTF (g_hvld_trace_fp, "blk_id: %d\n", blk_id);
			}

			start_pos = 1;
			max_coeff_num = 15;

			H264VldBlk (CHROMA_AC, blk_id, start_pos, max_coeff_num, lmb_avail, tmb_avail, &error);

			if (error)
			{
				/*set done and error flag*/
				g_hvld_reg_ptr->hvld_ctr = (1 << 1) | (1 << 2);
				goto VLD_END;
			}
		}
	}

	PrintfDCTBuf ();

VLD_END:
	ComputeCBPIqtMbc (mb_type, cbp);


#endif //H264_DEC
	return;
}