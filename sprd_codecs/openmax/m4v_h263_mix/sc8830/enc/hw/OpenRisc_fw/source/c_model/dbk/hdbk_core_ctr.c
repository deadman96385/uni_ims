/*dbk_core_ctr.c*/
#ifdef VP8_DEC
#include "vp8dbk_global.h"
#else
#include "sc8810_video_header.h"
#endif


void ComputeBlkPos (
					  int	blk_id, 
					  int * blk_id_x_ptr, 
					  int * blk_id_y_ptr, 
					  int * blk_comp_ptr
					  )
{
	int		blk_id_x;
	int		blk_id_y;
	int		blk_componet;

	if (blk_id < 20)
	{
		blk_componet = 0;

		if (blk_id < 5)
		{
			blk_id_x = blk_id;
			blk_id_y = 0;
		}
		else if (blk_id < 10)
		{
			blk_id_x = blk_id - 5;
			blk_id_y = 1;
		}
		else if (blk_id < 15)
		{
			blk_id_x = blk_id - 10;
			blk_id_y = 2;
		}
		else
		{
			blk_id_x = blk_id - 15;
			blk_id_y = 3;
		}
	}
	else if (blk_id < 26)
	{
		blk_componet = 1;

		if (blk_id < 23)
		{
			blk_id_x = blk_id - 20;
			blk_id_y = 0;
		}
		else
		{
			blk_id_x = blk_id - 23;
			blk_id_y = 1;
		}
	}
	else
	{
		blk_componet = 2;

		if (blk_id < 29)
		{
			blk_id_x = blk_id - 26;
			blk_id_y = 0;
		}
		else
		{
			blk_id_x = blk_id - 29;
			blk_id_y = 1;
		}
	}

	*blk_id_x_ptr = blk_id_x;
	*blk_id_y_ptr = blk_id_y;
	*blk_comp_ptr = blk_componet;
}

void FilterPixWriteBuf (
						int		mb_x,
						int		line_id, 
						uint8	pix_write[4], 
						int		blk_wr_x, 
						int		blk_wr_y, 
						int		blk_wr_comp
						)
{
	int		addr;
	uint32	dbk_buf_wdata;

	dbk_buf_wdata = (pix_write[3] << 24) | (pix_write[2] << 16) | (pix_write[1] << 8) | pix_write[0];

	if (line_id >= 4)
	{
		/*write g_dbk_line_buffer*/
		if (blk_wr_comp == BLK_COMPONET_Y)
		{
			addr = (line_id & 3) * LINE_WIDTH_Y + mb_x*4 - 2 + (blk_wr_x + 1);
		}
		else if (blk_wr_comp == BLK_COMPONET_U)
		{
			addr = U_OFFSET_LINE_BUF + (line_id & 3) * LINE_WIDTH_C + mb_x * 2 - 2 + (blk_wr_x + 1);
		}
		else
		{
			addr = V_OFFSET_LINE_BUF + (line_id & 3) * LINE_WIDTH_C + mb_x * 2 - 2 + (blk_wr_x + 1);
		}

		g_dbk_line_buf[addr] = dbk_buf_wdata;
	}
	else
	{
		/*write mb_dbk_buf*/
		if (blk_wr_comp == BLK_COMPONET_Y)
		{
			addr = (blk_wr_y * 4 + line_id) * 6 + (blk_wr_x + 1);
		}
		else if (blk_wr_comp == BLK_COMPONET_U)
		{
			addr = U_OFFSET_BUF_C + (blk_wr_y * 4 + line_id) * 4 + (blk_wr_x + 1);
		}
		else
		{
			addr = V_OFFSET_BUF_C + (blk_wr_y * 4 + line_id) * 4 + (blk_wr_x + 1);
		}

		vsp_dbk_out_bfr[addr] = dbk_buf_wdata;
	}
}

void ReadFourPixels (
					 int	mb_x,
					 int	line_id, 
					 uint8	pix_read[4], 
					 int	blk_rd_x, 
					 int	blk_rd_y, 
					 int	blk_rd_comp
					 )
{
	int		addr;
	uint32	read_data;
	

	if (line_id >= 4)
	{
		/*the bottom four line are read from g_dbk_out_buf*/
		if (blk_rd_comp == BLK_COMPONET_Y)
		{
			addr = (blk_rd_y * 4 + line_id) * 6 + (blk_rd_x + 1);
		}
		else if (blk_rd_comp == BLK_COMPONET_U)
		{
			addr = U_OFFSET_BUF_C + (blk_rd_y * 4 + line_id) * 4 + (blk_rd_x + 1);
		}
		else
		{
			addr = V_OFFSET_BUF_C + (blk_rd_y * 4 + line_id) * 4 + (blk_rd_x + 1);
		}

		read_data = vsp_dbk_out_bfr[addr];
	}
	else
	{
		/*the top four line are read from g_dbk_line_buf*/
		if (blk_rd_comp == BLK_COMPONET_Y)
		{
			addr = (line_id & 3) * LINE_WIDTH_Y + mb_x*4 - 2 + (blk_rd_x + 1);
		}
		else if (blk_rd_comp == BLK_COMPONET_U)
		{
			addr = U_OFFSET_LINE_BUF + (line_id & 3) * LINE_WIDTH_C + mb_x * 2 - 2 + (blk_rd_x + 1);
		}
		else
		{
			addr = V_OFFSET_LINE_BUF + (line_id & 3) * LINE_WIDTH_C + mb_x * 2 - 2 + (blk_rd_x + 1);
		}

		read_data = g_dbk_line_buf[addr];
	}

	pix_read[3] = (read_data >> 24) & 0xff;
	pix_read[2] = (read_data >> 16) & 0xff;
	pix_read[1] = (read_data >> 8 ) & 0xff;
	pix_read[0] = (read_data >> 0 ) & 0xff;
}

void hdbk_core_ctr (int		mb_x)
{
	int		blk_id;
	int		line_id;
	int		blk_rd_x;
	int		blk_rd_y;
	int		blk_wr_x;
	int		blk_wr_y;
	int		blk_rd_comp;
	int		blk_wr_comp;
	uint8	pix_write[4];
	uint8	pix_read[4];
//	uint8	pix_filter[8];
	uint8	pix_p[4];
	uint8	pix_q[4];
	int		is_hor_filter;
	int		filt_blk_comp;
	int		filt_blk_x;
	int		filt_blk_y;
	int		filt_blk_id;

#if defined(H264_DEC)
	int		Bs;
	int		Qp;
	int		alpha;
	int		beta;
	int		clip_par;
#elif defined(VP8_DEC)
	int		edge_limit;
	int		interior_diff_limit;
	int		hevthr;
	int		filter_type;
#endif

	int		write_lt_blk;     //not write left_top block


	for (blk_id = 0; blk_id < 34; blk_id++)
	{
		ComputeBlkPos (blk_id,   &blk_rd_x, &blk_rd_y, &blk_rd_comp);
		ComputeBlkPos (blk_id-2, &blk_wr_x, &blk_wr_y, &blk_wr_comp);
#ifndef VP8_DEC
		g_blk_id = blk_id;
#endif	

		for (line_id = 7; line_id >= 0; line_id--)
		{
			if ((blk_id == 23) && (line_id == 7))
				printf ("");

			/**********************************************************
			write filtered pixel into buf:
			1. (bottom 4 line in pixel array) horizontal filtered pixels into dbk_h_buf
			2. (top 4 line in pixl array) horizontal and vertical filtered pixels into mb_dbk_buf
			**********************************************************/
			GetFourPixWrite (line_id, pix_write);
			
			//for first block, there is no data need to be written to buffer
			write_lt_blk = (blk_wr_x == 0) && (blk_wr_y == 0) && (line_id < 4);

		//	if (blk_wr_x >= 0)
			if ((!write_lt_blk) & ((blk_wr_x > 0) | ((blk_wr_x == 0) && (mb_x > 0))))
				FilterPixWriteBuf (mb_x, line_id, pix_write, blk_wr_x, blk_wr_y, blk_wr_comp);

			/*********************************************************
			read pixels into pixel array
			1. read original pixels from mb_dbk_buf;
			2. read horizontal filtered pixels from dbk_h_buf
			**********************************************************/
			ReadFourPixels (mb_x, line_id, pix_read, blk_rd_x, blk_rd_y, blk_rd_comp);

			PixBlkUptWrite  (line_id, pix_read[0], pix_read[1], pix_read[2], pix_read[3]);


			/*********************************************************
			horizontal and verical filtering
			1. the first four line_id is for horizontal filtering
			2. the second four line_id is for vertical filtering
			**********************************************************/
			Get8PixFiltering (line_id, pix_p, pix_q);

			is_hor_filter = (line_id >= 4) ? 1 : 0;
			filt_blk_id   = is_hor_filter ? blk_id : blk_id - 1;
			ComputeBlkPos (filt_blk_id, &filt_blk_x, &filt_blk_y, &filt_blk_comp);						
			
			#if defined(H264_DEC)
			GetFilterPara (
							/*input*/
							is_hor_filter, 
							filt_blk_comp, 
							filt_blk_y, 
							filt_blk_x,
							line_id,
				
							/*output*/
							&Bs,
							&Qp,
							&alpha,
							&beta,
							&clip_par
						);

			DbkFilter	(
							line_id,
							pix_p, 
							pix_q, 
							(filt_blk_comp == BLK_COMPONET_Y),
							Bs, 
							Qp, 
							alpha, 
							beta, 
							clip_par
						);
			#elif defined(VP8_DEC)
			GetFilterPara_VP8 (
					/*input*/
						is_hor_filter, 
						filt_blk_comp, 
						filt_blk_y, 
						filt_blk_x,
	
					/*output*/
						 & edge_limit,
						 & interior_diff_limit,
						 & hevthr,
						 & filter_type
					);

			DbkFilter_VP8(
						line_id,
						pix_p,
						pix_q,
	
						edge_limit,
						interior_diff_limit,
						hevthr,
						filter_type);
			#endif


		}

		/***************************************************************
			exchange pixels in pixel array to decrease mutiplex logic
		****************************************************************/
		PixBlkExchange ();		
	}

}