#include "rdbk_global.h"
#include "rdbk_mode.h"
#include "common_global.h"

void ComputeBlkPos_rv9 (
					  int	blk_id,
					  int   last_row,
					  int	last_col,
					  int * blk_id_x_ptr, 
					  int * blk_id_y_ptr, 
					  int * blk_comp_ptr
					  )
{
	int		blk_id_x;
	int		blk_id_y;
	int		blk_componet;


	if (last_col && last_row)
	{
		if (blk_id <30)
		{// Y first phase
			blk_componet = 0;
			if (blk_id < 6)
			{
				blk_id_x = blk_id;
				blk_id_y = 0;
			}
			else if (blk_id < 12)
			{
				blk_id_x = blk_id - 6;
				blk_id_y = 1;
			}
			else if (blk_id < 18)
			{
				blk_id_x = blk_id - 12;
				blk_id_y = 2;
			}
			else if (blk_id <24)
			{
				blk_id_x = blk_id - 18;
				blk_id_y = 3;
			}
			else
			{
				blk_id_x = blk_id - 24;
				blk_id_y = 4;
			}
		}
		else if (blk_id < 42)
		{// U first phase

			blk_componet = 1;

			if (blk_id < 34)
			{
				blk_id_x = blk_id - 30;
				blk_id_y = 0;
			}
			else if (blk_id < 38)
			{
				blk_id_x = blk_id - 34;
				blk_id_y = 1;
			}
			else
			{
				blk_id_x = blk_id - 38;
				blk_id_y = 2;
			}
		}
		else if (blk_id < 54)
		{// V first phase

			blk_componet = 2;


			if (blk_id < 46)
			{
				blk_id_x = blk_id - 42;
				blk_id_y = 0;
			}
			else if (blk_id < 50)
			{
				blk_id_x = blk_id - 46;
				blk_id_y = 1;
			}
			else
			{
				blk_id_x = blk_id - 50;
				blk_id_y = 2;
			}
		}
		else if (blk_id < 60)
		{// Y second phase
			blk_componet = 0;

			blk_id_x = blk_id - 54;
			blk_id_y = 0;
		}
		else if (blk_id < 64)
		{// U second phase
			blk_componet = 1;

			blk_id_x = blk_id - 60;
			blk_id_y = 0;
		}
		else 
		{// V second phase
			blk_componet = 2;

			blk_id_x = blk_id - 64;
			blk_id_y = 0;
		}
	}
	else if (last_col && !last_row)
	{
		if (blk_id <24)
		{// Y first phase
			blk_componet = 0;
			if (blk_id < 6)
			{
				blk_id_x = blk_id;
				blk_id_y = 0;
			}
			else if (blk_id < 12)
			{
				blk_id_x = blk_id - 6;
				blk_id_y = 1;
			}
			else if (blk_id < 18)
			{
				blk_id_x = blk_id - 12;
				blk_id_y = 2;
			}
			else if (blk_id <24)
			{
				blk_id_x = blk_id - 18;
				blk_id_y = 3;
			}
		}
		else if (blk_id < 32)
		{// U first phase

			blk_componet = 1;

			if (blk_id < 28)
			{
				blk_id_x = blk_id - 24;
				blk_id_y = 0;
			}
			else if (blk_id < 32)
			{
				blk_id_x = blk_id - 28;
				blk_id_y = 1;
			}
		}
		else if (blk_id < 40)
		{// V first phase

			blk_componet = 2;


			if (blk_id < 36)
			{
				blk_id_x = blk_id - 32;
				blk_id_y = 0;
			}
			else if (blk_id < 40)
			{
				blk_id_x = blk_id - 36;
				blk_id_y = 1;
			}
		}
		else if (blk_id < 46)
		{// Y second phase
			blk_componet = 0;

			blk_id_x = blk_id - 40;
			blk_id_y = 0;
		}
		else if (blk_id < 50)
		{// U second phase
			blk_componet = 1;

			blk_id_x = blk_id - 46;
			blk_id_y = 0;
		}
		else 
		{// V second phase
			blk_componet = 2;

			blk_id_x = blk_id - 50;
			blk_id_y = 0;
		}		
	}
	else if (!last_col && last_row)
	{
		if (blk_id <26)
		{// Y first phase
			blk_componet = 0;
			if (blk_id < 6)
			{
				blk_id_x = blk_id;
				blk_id_y = 0;
			}
			else if (blk_id < 11)
			{
				blk_id_x = blk_id - 6;
				blk_id_y = 1;
			}
			else if (blk_id < 16)
			{
				blk_id_x = blk_id - 11;
				blk_id_y = 2;
			}
			else if (blk_id <21)
			{
				blk_id_x = blk_id - 16;
				blk_id_y = 3;
			}
			else
			{
				blk_id_x = blk_id - 21;
				blk_id_y = 4;
			}
		}
		else if (blk_id < 36)
		{// U first phase

			blk_componet = 1;

			if (blk_id < 30)
			{
				blk_id_x = blk_id - 26;
				blk_id_y = 0;
			}
			else if (blk_id < 33)
			{
				blk_id_x = blk_id - 30;
				blk_id_y = 1;
			}
			else
			{
				blk_id_x = blk_id - 33;
				blk_id_y = 2;
			}
		}
		else if (blk_id < 46)
		{// V first phase

			blk_componet = 2;


			if (blk_id < 40)
			{
				blk_id_x = blk_id - 36;
				blk_id_y = 0;
			}
			else if (blk_id < 43)
			{
				blk_id_x = blk_id - 40;
				blk_id_y = 1;
			}
			else
			{
				blk_id_x = blk_id - 43;
				blk_id_y = 2;
			}
		}
		else if (blk_id < 51)
		{// Y second phase
			blk_componet = 0;

			blk_id_x = blk_id - 46;
			blk_id_y = 0;
		}
		else if (blk_id < 54)
		{// U second phase
			blk_componet = 1;

			blk_id_x = blk_id - 51;
			blk_id_y = 0;
		}
		else 
		{// V second phase
			blk_componet = 2;

			blk_id_x = blk_id - 54;
			blk_id_y = 0;
		}
	}
	else if (!last_col && !last_row)
	{
		if (blk_id <21)
		{// Y first phase
			blk_componet = 0;
			if (blk_id < 6)
			{
				blk_id_x = blk_id;
				blk_id_y = 0;
			}
			else if (blk_id < 11)
			{
				blk_id_x = blk_id - 6;
				blk_id_y = 1;
			}
			else if (blk_id < 16)
			{
				blk_id_x = blk_id - 11;
				blk_id_y = 2;
			}
			else if (blk_id <21)
			{
				blk_id_x = blk_id - 16;
				blk_id_y = 3;
			}
		}
		else if (blk_id < 28)
		{// U first phase

			blk_componet = 1;

			if (blk_id < 25)
			{
				blk_id_x = blk_id - 21;
				blk_id_y = 0;
			}
			else if (blk_id < 28)
			{
				blk_id_x = blk_id - 25;
				blk_id_y = 1;
			}
		}
		else if (blk_id < 35)
		{// V first phase

			blk_componet = 2;


			if (blk_id < 32)
			{
				blk_id_x = blk_id - 28;
				blk_id_y = 0;
			}
			else if (blk_id < 35)
			{
				blk_id_x = blk_id - 32;
				blk_id_y = 1;
			}

		}
		else if (blk_id < 40)
		{// Y second phase
			blk_componet = 0;

			blk_id_x = blk_id - 35;
			blk_id_y = 0;
		}
		else if (blk_id < 43)
		{// U second phase
			blk_componet = 1;

			blk_id_x = blk_id - 40;
			blk_id_y = 0;
		}
		else 
		{// V second phase
			blk_componet = 2;

			blk_id_x = blk_id - 43;
			blk_id_y = 0;
		}
	}

	*blk_id_x_ptr = blk_id_x;
	*blk_id_y_ptr = blk_id_y;
	*blk_comp_ptr = blk_componet;
}

void FilterPixWriteBuf_rv9 (
						int		line_id,
						int		mb_x,
						uint8	pix_write[4], 
						int		is_upper_blk,
						int		second_phase,
						int		last_col,
						int		blk_wr_x, 
						int		blk_wr_y, 
						int		blk_wr_comp
						)
{
	int		addr;
	uint32	dbk_buf_wdata;

	int		last_blk;
	int		b_Obuf_notLbuf;

	int		Lbuf_unavailable;

	int		Obuf_wen = 0;
	int		Lbuf_wen = 0;
	int		Tbuf_wen = 0;

	dbk_buf_wdata = (pix_write[3] << 24) | (pix_write[2] << 16) | (pix_write[1] << 8) | pix_write[0];
	
	last_blk = (blk_wr_comp == BLK_COMPONET_Y) ? 5: 3;

	b_Obuf_notLbuf = !is_upper_blk && !second_phase && !last_col && blk_wr_y == 1 && (blk_wr_x == last_blk );
	
	Lbuf_unavailable = mb_x == 0 && blk_wr_x < 2;

	if (blk_wr_comp == BLK_COMPONET_Y)
	{
		// Y
		if (blk_wr_y == 0)
		{
			//Top blk line. Wirte to Tbuf all the time.
			Tbuf_wen = 1;
			addr = Y_OFFSET_TBUF_C + line_id * 6 + blk_wr_x;
		}
		else if (is_upper_blk)
		{
			//upper reg. write to Obuf.
			Obuf_wen = 1;
			addr = Y_OFFSET_OBUF_C + (blk_wr_y * 4 + line_id) * 6 + blk_wr_x;
		}
		else
		{	// lower reg.
			if (second_phase || b_Obuf_notLbuf)
			{
				//write back to Obuf in second phase.
				Obuf_wen = 1;
				addr = Y_OFFSET_OBUF_C + (blk_wr_y * 4 + line_id) * 6 + blk_wr_x;
			}
			else
			{
				//write back to LBuf in first phase
				Lbuf_wen = 1;
				addr = Y_OFFSET_LINE_BUF + line_id * Y_LINE_BUF_WIDTH + mb_x*4 - 2 + blk_wr_x;
			}
		}
		 

	}
	else if (blk_wr_comp == BLK_COMPONET_U)
	{
		// U
		if (blk_wr_y == 0)
		{
			//Top blk line. Wirte to Tbuf all the time.
			Tbuf_wen = 1;
			addr = U_OFFSET_TBUF_C + line_id * 4 + blk_wr_x;
		}
		else if (is_upper_blk)
		{
			//upper reg. write to Obuf.
			Obuf_wen = 1;
			addr = U_OFFSET_OBUF_C + (blk_wr_y * 4 + line_id) * 4 + blk_wr_x;
		}
		else
		{	// lower reg.
			if (second_phase || b_Obuf_notLbuf)
			{
				//write back to Obuf in second phase.
				Obuf_wen = 1;
				addr = U_OFFSET_OBUF_C + (blk_wr_y * 4 + line_id) * 4 + blk_wr_x;
			}
			else
			{
				//write back to LBuf in first phase
				Lbuf_wen = 1;
				addr = U_OFFSET_LINE_BUF + line_id * UV_LINE_BUF_WIDTH + mb_x*2 - 2 + blk_wr_x;
			}
		}
		 
	}
	else if (blk_wr_comp == BLK_COMPONET_V)
	{
		// V
		if (blk_wr_y == 0)
		{
			//Top blk line. Wirte to Tbuf all the time.
			Tbuf_wen = 1;
			addr = V_OFFSET_TBUF_C + line_id * 4 + blk_wr_x;
		}
		else if (is_upper_blk)
		{
			//upper reg. write to Obuf.
			Obuf_wen = 1;
			addr = V_OFFSET_OBUF_C + (blk_wr_y * 4 + line_id) * 4 + blk_wr_x;
		}
		else
		{	// lower reg.
			if (second_phase || b_Obuf_notLbuf)
			{
				//write back to Obuf in second phase.
				Obuf_wen = 1;
				addr = V_OFFSET_OBUF_C + (blk_wr_y * 4 + line_id) * 4 + blk_wr_x;
			}
			else
			{
				//write back to LBuf in first phase
				Lbuf_wen = 1;
				addr = V_OFFSET_LINE_BUF + line_id * UV_LINE_BUF_WIDTH + mb_x*2 - 2 + blk_wr_x;
			}
		}
		 
	}

		
	if (Tbuf_wen)
	{
		vsp_rdbk_tbuf[addr] = dbk_buf_wdata;
	}

	if (Obuf_wen)
	{
		vsp_rdbk_obuf[addr] = dbk_buf_wdata;
	}
	
	if (Lbuf_wen && ! Lbuf_unavailable)
	{
		vsp_rdbk_lbuf[addr] = dbk_buf_wdata;
	}
	 	

}

void ReadFourPixels_rv9 (
					 int	line_id, 
					 int	mb_x,
					 uint8	pix_read[4], 
				     int	is_upper_blk,
					 int	second_phase,
					 int	blk_rd_x, 
					 int	blk_rd_y, 
					 int	blk_rd_comp
					 )
{
	int		addr;
	uint32	read_data;

	int		Obuf_ren = 0;
	int		Tbuf_ren = 0;
	int		Lbuf_ren = 0;
	
	if (blk_rd_comp == BLK_COMPONET_Y)
	{
		if (is_upper_blk)
		{
			if (!second_phase)
			{
				Lbuf_ren = 1;
				addr = Y_OFFSET_LINE_BUF + (line_id )* Y_LINE_BUF_WIDTH + mb_x*4 - 2 + blk_rd_x;
			}
			else
			{
				Tbuf_ren = 1;
				addr = Y_OFFSET_TBUF_C + (line_id ) * 6 + blk_rd_x;
			}
		}
		else
		{
			Obuf_ren = 1;
			addr = Y_OFFSET_OBUF_C + (blk_rd_y * 4 + (line_id )) * 6 + blk_rd_x;
		}
	}
	else if (blk_rd_comp == BLK_COMPONET_U)
	{
		if (is_upper_blk)
		{
			if (!second_phase)
			{
				Lbuf_ren = 1;
				addr = U_OFFSET_LINE_BUF + (line_id )* UV_LINE_BUF_WIDTH + mb_x*2 - 2 + blk_rd_x;
			}
			else
			{
				Tbuf_ren = 1;
				addr = U_OFFSET_TBUF_C + (line_id ) * 4 + blk_rd_x;
			}
		}
		else
		{
			Obuf_ren = 1;
			addr = U_OFFSET_OBUF_C + (blk_rd_y * 4 + (line_id )) * 4 + blk_rd_x;
		}
	}
	else if (blk_rd_comp == BLK_COMPONET_V)
	{
		if (is_upper_blk)
		{
			if (!second_phase)
			{
				Lbuf_ren = 1;
				addr = V_OFFSET_LINE_BUF + (line_id )* UV_LINE_BUF_WIDTH + mb_x*2 - 2 + blk_rd_x;
			}
			else
			{
				Tbuf_ren = 1;
				addr = V_OFFSET_TBUF_C + (line_id ) * 4 + blk_rd_x;
			}
		}
		else
		{
			Obuf_ren = 1;
			addr = V_OFFSET_OBUF_C + (blk_rd_y * 4 + (line_id )) * 4 + blk_rd_x;
		}
	}

	if (Tbuf_ren)
	{
		read_data = vsp_rdbk_tbuf[addr];
	}

	if (Obuf_ren)
	{
		read_data = vsp_rdbk_obuf[addr];
	}
	
	if (Lbuf_ren)
	{
		read_data = vsp_rdbk_lbuf[addr];
	}


	pix_read[3] = (read_data >> 24) & 0xff;
	pix_read[2] = (read_data >> 16) & 0xff;
	pix_read[1] = (read_data >> 8 ) & 0xff;
	pix_read[0] = (read_data >> 0 ) & 0xff;
}

void rdbk_core_ctr_rv9()
{

	int		mb_x;

	int		four_cycles;

	int		blkid;
	int		blkid_wr;
	int		blkid_filt;
	int		blkid_para;

	int		blk_rd_x;
	int		blk_rd_y;
	int		blk_wr_x;
	int		blk_wr_y;
	int		blk_rd_comp;
	int		blk_wr_comp;

	int		blk_rd_y1;
	int		blk_wr_y1;
	
	int		filt_blk_comp;
	int		filt_blk_x;
	int		filt_blk_y;

	int		para_blk_comp;
	int		para_blk_x;
	int		para_blk_y;

	
	int		line_id;
	uint8	pix_write0[4];
	uint8	pix_write1[4];
	uint8	pix_read0[4];
	uint8	pix_read1[4];


	int		edge_id;
	int		proc_edge[2];//0: no filter, just read and write. 1-4 filter edge 1-4

	uint8	pix_p[4]; // left of above
	uint8	pix_q[4]; // right of below
	/*pix_p[3] pix_p[2] pix_p[1] pix_p[0] | pix_q[0] pix_q[1] pix_q[2] */

	uint8 left_arr[12];
	uint8 right_arr[12];
	/*left_arr[12] = L32 L31 L30 L22 L21 L20 L12 L11 L10 L02 L01 L00
	  right_arr[12]= R32 R31 R30 R22 R21 R20 R12 R11 R10 R02 R01 R00
	which organized as 
	L02 L01 L00 | R00 R01 R02
	L12 L11 L10 | R10 R11 R12
	L22 L21 L20 | R20 R21 R22
	L32 L31 L30 | R30 R31 R32*/


	int		bStrong;
	int		edge_clip_left[2];
	int		edge_clip_right[2];
	int		alpha;
	int		beta;
	int		beta2;

	int		Al,Ar;
	int		filter_type;


	int		blk_read_en;
	int		blk_write_en;
	int		blk_filt_en;

	int		total_blks;
	int		fst_phase_blks;

	int		second_phase;
	int		second_phase_wr;
	int		second_phase_filt;
	int		second_phase_para;
	
	int		last_row;
	int		last_col;
#if RDBK_OWN_REG
	last_col= (g_dbk_reg_ptr->rdbk_cfg6 >>24) & 0x1;
	last_row= (g_dbk_reg_ptr->rdbk_cfg6>>22) & 0x1;

 	mb_x	= (g_glb_reg_ptr->VSP_CTRL0) & 0x3f;
#else 

	last_row= (g_dbk_reg_ptr->RDBK_PARS >>1) & 0x1;

	last_col= (g_dbk_reg_ptr->RDBK_PARS >>3) & 0x1;

	mb_x	= (g_dbk_reg_ptr->HDBK_MB_INFO >>24) & 0x3f;
#endif


	total_blks = (last_col && last_row) ? 67 : last_col? 53 : last_row ? 56 : 45;
	fst_phase_blks = (last_col && last_row) ? 53 : last_col? 39 : last_row ? 45 : 34;
	
	proc_edge[0] = proc_edge[1] = 0;

	for (blkid = 0; blkid <= (total_blks + 3); blkid ++)
	{
		
		if (blkid == 37)
		{
		//	printf("");
		}
		blkid_wr = blkid-3;
		blkid_filt = blkid-1;
		blkid_para = blkid;

		//Read block position		
		ComputeBlkPos_rv9 (blkid, last_row,	last_col, &blk_rd_x,&blk_rd_y,&blk_rd_comp);
		blk_read_en		= (blkid <=total_blks);

		//Write block position
		ComputeBlkPos_rv9(blkid_wr, last_row, last_col,&blk_wr_x, &blk_wr_y, &blk_wr_comp);
		blk_write_en	= (blkid_wr >=0 && blkid_wr<=total_blks);

		//Filter block position		
		ComputeBlkPos_rv9(blkid_filt, last_row,	last_col, &filt_blk_x, &filt_blk_y, &filt_blk_comp);
		blk_filt_en		= (blkid_filt>=0 && blkid_filt<=total_blks);


// 		//Parameter block position
// 		ComputeBlkPos(blkid_para, last_row, last_col, &para_blk_x, &para_blk_y, &para_blk_comp);
		para_blk_x = blk_rd_x;
		para_blk_y = blk_rd_y;
		para_blk_comp = blk_rd_comp;
		

		//whether process(read, write, filter, para) is second phase
		second_phase = (blkid > fst_phase_blks) ? 1: 0;
		second_phase_wr = (blkid_wr > fst_phase_blks) ? 1: 0;
		second_phase_filt = (blkid_filt > fst_phase_blks) ? 1: 0;
		second_phase_para = (blkid_para > fst_phase_blks) ? 1: 0;
		

		//Lower block y position of read.
		blk_rd_y1 = (blk_rd_comp == BLK_COMPONET_Y && blk_rd_y==4) ? 4:
		(blk_rd_comp != BLK_COMPONET_Y && blk_rd_y==2) ? 2: (blk_rd_y + 1) ;

		//Lower block y position of write.
		blk_wr_y1 = (blk_wr_comp == BLK_COMPONET_Y && blk_wr_y==4) ? 4:
		(blk_wr_comp != BLK_COMPONET_Y && blk_wr_y==2) ? 2: (blk_wr_y + 1) ;
		
		//If second phase, filter and para are done to lower blk
		para_blk_y = second_phase_para ? (para_blk_y + 1) : para_blk_y;
		filt_blk_y = second_phase_filt ? (filt_blk_y + 1) : filt_blk_y;


#if RDBK_TRACE_ON
		if(blk_filt_en && (proc_edge[0] || proc_edge[1]))
		{
			RDBK_TRACE(fp_rdbk_filter_para_trace,"blk_x = %d blk_y = %d blk_comp = %d\n",filt_blk_x, filt_blk_y, filt_blk_comp);
			RDBK_TRACE(fp_rdbk_filter_data_trace,"blk_x = %d blk_y = %d blk_comp = %d\n",filt_blk_x, filt_blk_y, filt_blk_comp);
		}
	
#endif


		for (four_cycles = 0 ; four_cycles <2; four_cycles ++)
		{
			edge_id = proc_edge[four_cycles];
			bStrong = (edge_id > 2) ? 1 : 0;

			for (line_id = 0; line_id <4; line_id ++)
			{
				// Each Block requires 8  cycles. First 4 write buffer, last 4 read buffer.

				if (four_cycles == 0 && blk_write_en)
				{
					GetEightPixWrite_rv9(line_id,pix_write0,pix_write1);
			
					// write upper register(reg_blk 4)
					FilterPixWriteBuf_rv9 ( line_id,
										mb_x,
										pix_write0, 
										1,//upper reg
										second_phase_wr,
										last_col,
										blk_wr_x, 
										blk_wr_y, 
										blk_wr_comp
								);
					// write lower register(reg_blk 5)
					FilterPixWriteBuf_rv9 ( line_id,
										mb_x,
										pix_write1, 
										0,//lower reg
										second_phase_wr,
										last_col,
										blk_wr_x, 
										blk_wr_y1, 
										blk_wr_comp
								);

				}


				if (four_cycles == 1 && blk_read_en)
				{
					ReadFourPixels_rv9 (line_id, 
									mb_x,
									pix_read0, 
						     		1, //upper reg
									second_phase,
									blk_rd_x, 
									blk_rd_y, 
									blk_rd_comp
							 );

					ReadFourPixels_rv9 (line_id, 
									mb_x,
									pix_read1, 
						     		0, //lower reg
									second_phase,
									blk_rd_x, 
									blk_rd_y1, 
									blk_rd_comp
							 );

					ReadPixUptArr_rv9 (line_id, pix_read0, pix_read1);
				}

				if (blk_filt_en && edge_id>0)
				{
					// Filter type decision logic : 3:strong, 2:normal, 1:weak or 0:none
					if (line_id == 0)
					{
						GetLeftRightArr_rv9(edge_id, second_phase_filt, left_arr,right_arr);
	
						CalFilterType_rv9(left_arr,right_arr, bStrong,beta,beta2,&Al,&Ar,&filter_type);
#if RDBK_TRACE_ON
	// Print filter_para.txt
	RDBK_TRACE(fp_rdbk_filter_para_trace,"edge_id = %d\n",edge_id);
	RDBK_TRACE(fp_rdbk_filter_para_trace,"second_phase_filt = %d\n",second_phase_filt);
	RDBK_TRACE(fp_rdbk_filter_para_trace,"bStrong = %d\n",bStrong);
	RDBK_TRACE(fp_rdbk_filter_para_trace,"beta = %d\n",beta);
	RDBK_TRACE(fp_rdbk_filter_para_trace,"beta2 = %d\n",beta2);

	RDBK_TRACE(fp_rdbk_filter_para_trace,"Al = %d\n",Al);
	RDBK_TRACE(fp_rdbk_filter_para_trace,"Ar = %d\n",Ar);
	RDBK_TRACE(fp_rdbk_filter_para_trace,"filter_type = %d\n",filter_type);

	// Print filter_data.txt
	RDBK_TRACE(fp_rdbk_filter_data_trace,"edge_id = %d\n",edge_id);
#endif
					}
					// Filter current blk
					Get8PixFiltering_rv9(second_phase_filt,edge_id,line_id,pix_p,pix_q);	
					
					RDBKFilter(	pix_p, pix_q, 
							edge_clip_left[four_cycles], edge_clip_right[four_cycles],					
							alpha, beta, 
							Al, Ar,					
							filter_type,			
							(filt_blk_comp == BLK_COMPONET_Y),
							edge_id,  line_id,second_phase_filt);
				}



			}
		}

		// Calculate parameter for next block 
		CalBlkPara_RDBK_rv9(para_blk_x, para_blk_y, para_blk_comp, second_phase_para,
					proc_edge, edge_clip_left, edge_clip_right,
					 &alpha, &beta, &beta2);


#if RDBK_TRACE_ON
	//Print blk_para.txt 
	RDBK_TRACE(fp_rdbk_blk_para_trace,"blk_x = %d blk_y = %d blk_comp = %d second_phase = %d\n",para_blk_x, para_blk_y, para_blk_comp, second_phase_para);
	RDBK_TRACE(fp_rdbk_blk_para_trace,"proc_edge[0] = %d\n",proc_edge[0]);
	RDBK_TRACE(fp_rdbk_blk_para_trace,"proc_edge[1] = %d\n",proc_edge[1]);
	RDBK_TRACE(fp_rdbk_blk_para_trace,"edge_clip_right[0] = %d\n",edge_clip_right[0]);
	RDBK_TRACE(fp_rdbk_blk_para_trace,"edge_clip_left[0] = %d\n",edge_clip_left[0]);
	RDBK_TRACE(fp_rdbk_blk_para_trace,"edge_clip_right[1] = %d\n",edge_clip_right[1]);
	RDBK_TRACE(fp_rdbk_blk_para_trace,"edge_clip_left[1] = %d\n",edge_clip_left[1]);
	RDBK_TRACE(fp_rdbk_blk_para_trace,"alpha = %d\n",alpha );
	RDBK_TRACE(fp_rdbk_blk_para_trace,"beta = %d\n",beta);
	RDBK_TRACE(fp_rdbk_blk_para_trace,"beta2 = %d\n",beta2);
#endif


		//Update Pixel Array Pipeline
		UpdatePixArrPipe_rv9();

	}

}