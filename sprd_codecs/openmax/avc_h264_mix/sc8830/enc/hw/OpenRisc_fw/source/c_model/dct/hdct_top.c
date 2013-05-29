
#include "sc6800x_video_header.h"
#include "hmea_mode.h"

int MBDecimateScore (int16 * blk_ptr, int i_max)
{	
	int score = 0;
	int idx	= i_max - 1;
	int ds_table[16] = {3,2,2,1,1,1,0,0,0,0,0,0,0,0,0,0 };
	
	while( idx >= 0 && blk_ptr[idx] == 0 )
		idx--;
	
	while( idx >= 0 )
	{
		int i_run;
		
		if( abs( blk_ptr[idx--] ) > 1 )
			return 9;
		
		i_run = 0;
		while( idx >= 0 && blk_ptr[idx] == 0 )
		{
			idx--;
			i_run++;
		}

		score += ds_table[i_run];
	}
	
	return score;	
}

int MBSkipDecimateScore (int16 * blk_ptr, int i_max)
{	
	int score = 0;
	int idx	= i_max - 1;
	int ds_table[16] = {3,2,2,1,1,1,0,0,0,0,0,0,0,0,0,0 };
	
	while( idx >= 0 && blk_ptr[idx] == 0 )
		idx--;
	
	while( idx >= 0 )
	{
		int i_run;
		
		if( abs( blk_ptr[idx--] ) > 1 )
			return 9;
		
		i_run = 0;
		while( idx >= 0 && blk_ptr[idx] == 0 )
		{
			idx--;
			i_run++;
		}

		score += ds_table[i_run];
	}
	
	return score;	
}

#ifdef TV_OUT
extern short dct_out_buf[432];
extern short idct_in_buf[432];
#endif
void hdct_module()
{
	int32 mb_type = (g_dct_reg_ptr->hdct_cfg0 >> 28) & 0x3;
	int32 Inter_mode_en = (g_dct_reg_ptr->hdct_cfg0 >> 24 ) & 0x01;
	int32 is_I4x4 = (g_dct_reg_ptr->hdct_cfg0) & 0x01;
	int32 qp_per, qp_rem;// = mb_info->qp;
	int32 idx;
	int32 blk8_idx;
	int32 blk4x4Idx;
	int	  blk8_score;
	int   mb_score;
	int	  b_decimate = g_decimate_en && ((mb_type >= 3) ? 1 : 0); //only decimate for inter Mb
	int32 F_trans_en = (g_dct_reg_ptr->DCT_CONFIG >> 0) & 0x01;
	//jzy
#ifdef TV_OUT
	int i,j;
#endif
	
	//0, 1,    4,  5
	//2, 3,    6,  7
	//8, 9,   12, 13
	//10, 11, 14, 15
	int32 sub8x8_luma_offset[4] = 
	{
		0,        8,     8*16,    8*16+8,			
	};
	int32 sub4x4_luma_offset[16] = 
	{
		0,        4,     4*16,    4*16+4,
			8,       12,   4*16+8,   4*16+12,
			8*16,   8*16+4,    12*16,   12*16+4,
			8*16+8, 8*16+12,  12*16+8,  12*16+12
	};
	int32 sub4x4_chroma_offset[4] = 
	{
		0,		4,		
			4*8,	4*8+4,	
	};
	
	const uint8 block_idx_x[16] =
	{
		0, 1, 0, 1, 2, 3, 2, 3, 0, 1, 0, 1, 2, 3, 2, 3
	};
	
	const uint8 block_idx_y[16] =
	{
		0, 0, 1, 1, 0, 0, 1, 1, 2, 2, 3, 3, 2, 2, 3, 3
	};
	
	if(F_trans_en) //dct
	{
		int16 *dct_src_ptr;
		int16 *dct_dst_ptr;
		int16 *dct_vlc_ptr;
		uint32 tmp_vlc_bfr[DCT_IO_BFR_SIZE];
		int8	cbp_luma = 0;
		int8	cbp_chroma = 0;
		int8 blk_nnz[24] = {0};
		
		qp_per = (g_dct_reg_ptr->iict_cfg1>>12)&0xf;
		qp_rem = (g_dct_reg_ptr->iict_cfg1>>4)&0xf;

		if(mb_type == 1)	// skip mb
		{
			memset(vsp_dct_io_1, 0, sizeof(uint32) * DCT_IO_BFR_SIZE);
			g_dct_reg_ptr->nnz_blk_0 = 0;
			g_dct_reg_ptr->nnz_blk_1 = 0;
			g_dct_reg_ptr->nnz_blk_2 = 0;
			g_dct_reg_ptr->nnz_blk_3 = 0;
			g_dct_reg_ptr->nnz_blk_4 = 0;
			g_dct_reg_ptr->nnz_blk_5 = 0;			
			g_dct_reg_ptr->h264_out_cbp = 0;
			return;
		}		
		else if (mb_type == 0)//i16x16 mode
		{
			int16 *dc_ptr = (int16 *)(vsp_dct_io_1 + HDCT_COEFF_LUMA_DC_BASE);
			int16 *luma_16x16_dc = (int16 *)(tmp_vlc_bfr + HDCT_COEFF_LUMA_DC_BASE);
			int32 dc_pos;
			
			//after zigzag, the coef are stored in dct_buf_0
			for (blk4x4Idx = 0; blk4x4Idx < 16; blk4x4Idx++)
			{
				dct_src_ptr = (int16 *)vsp_dct_io_0 + sub4x4_luma_offset[blk4x4Idx];
				dct_dst_ptr = (int16 *)vsp_dct_io_1 + blk4x4Idx*16;
				dct_vlc_ptr = (int16 *)tmp_vlc_bfr + blk4x4Idx*16;
				
				trans4x4 (dct_src_ptr, 16, dct_dst_ptr, 16);
				
				//copy dc coeff
				dc_pos = block_idx_y[blk4x4Idx]*4+block_idx_x[blk4x4Idx];
				dc_ptr[dc_pos] = dct_dst_ptr[0];
				
				//quant
				quant4x4(dct_dst_ptr, qp_per, qp_rem, 1);
				
#ifdef TV_OUT
				for(j=0; j<4; j++)
					for(i=0; i<4; i++)
						dct_out_buf[sub4x4_luma_offset[blk4x4Idx]+j+16*i]= dct_dst_ptr[i+4*j];
#endif
	
				//scan
				zigzag4x4(dct_dst_ptr, dct_vlc_ptr+1); //for ac coeff
				
				blk_nnz[blk4x4Idx] = get_nnz4x4(dct_dst_ptr, 1);
				
				if (blk_nnz[blk4x4Idx] > 0)
				{
					cbp_luma = 0xf;
				}
			}
			
			//dc
			dct4x4dc(dc_ptr);
			quant4x4dc (dc_ptr, qp_per, qp_rem);
#ifdef TV_OUT
			for(j=0; j<4; j++)
				for(i=0; i<4; i++)
					dct_out_buf[384+j+4*i]= dc_ptr[i+4*j];//24*16
#endif
			zigzag4x4_full(dc_ptr, luma_16x16_dc);
		}
		else if(!is_I4x4)//for intra4x4 and inter mode
		{
			mb_score = 0;
			
			for (blk8_idx = 0; blk8_idx < 4; blk8_idx++)
			{
				blk8_score = 0;

				for (idx = 0; idx < 4; idx++)
				{
					blk4x4Idx = blk8_idx*4 + idx;
					dct_src_ptr = (int16 *)vsp_dct_io_0 + sub4x4_luma_offset[blk4x4Idx];
					dct_dst_ptr = (int16 *)vsp_dct_io_1 + blk4x4Idx*16;
					dct_vlc_ptr = (int16 *)tmp_vlc_bfr + blk4x4Idx*16;
					
					trans4x4 (dct_src_ptr, 16, dct_dst_ptr, 16);
					
					//quant/scan
					quant4x4(dct_dst_ptr, qp_per, qp_rem, !Inter_mode_en);

#ifdef TV_OUT
					for(j=0; j<4; j++)
						for(i=0; i<4; i++)
							dct_out_buf[sub4x4_luma_offset[blk4x4Idx]+j+16*i]= dct_dst_ptr[i+4*j];
#endif
					zigzag4x4_full(dct_dst_ptr, dct_vlc_ptr);

					if (b_decimate)
					{
						blk8_score += MBSkipDecimateScore (dct_vlc_ptr, 16);
					}
					
					blk_nnz[blk4x4Idx] = get_nnz4x4(dct_dst_ptr, 0);
					
					if (blk_nnz[blk4x4Idx] > 0)
					{
						cbp_luma |= (1<<(blk4x4Idx/4));
					}
				}				
				
				if ((blk8_score < LUMA_BLK8_COEFF_COST) && b_decimate)
				{
					blk_nnz[blk8_idx*4 + 0] = 0;
					blk_nnz[blk8_idx*4 + 1] = 0;
					blk_nnz[blk8_idx*4 + 2] = 0;
					blk_nnz[blk8_idx*4 + 3] = 0;

			//		blk8_score = 0;

					cbp_luma = cbp_luma & ~(1<<blk8_idx);
					
					dct_dst_ptr = (int16 *)vsp_dct_io_1 + blk8_idx*4*16;
					memset (dct_dst_ptr, 0, sizeof(int16)*4*16);
#ifdef TV_OUT
					for(j=0; j<8; j++)
						for(i=0; i<8; i++)
							dct_out_buf[sub8x8_luma_offset[blk8_idx]+i+16*j]=0;
#endif
				}				

				mb_score += blk8_score;
			}

			if ((mb_score < LUMA_MB_COEFF_COST) && b_decimate)
			{
				cbp_luma = 0;
				memset (blk_nnz, 0, sizeof(uint8)*16);
				memset (vsp_dct_io_1, 0, 16*16*sizeof(int16));				
#ifdef TV_OUT
				for(i=0; i<16*16; i++)
					dct_out_buf[i]=0;
#endif
			}
		}
		
		//chroma
		qp_per = (g_dct_reg_ptr->iict_cfg1>>8)&0xf;
		qp_rem = (g_dct_reg_ptr->iict_cfg1>>0)&0xf;
		{
			int16 *dc_ptr = (int16 *)(vsp_dct_io_1 + HDCT_COEFF_CHROMA_DC_BASE);
			int16 *chroma_2x2_dc = (int16 *)(tmp_vlc_bfr + HDCT_COEFF_CHROMA_DC_BASE);
			int32 dc_pos;
			int	  cbp_cb = 0;
			int	  cbp_cr = 0;

			blk8_score = 0;
			
			//u
			for (blk4x4Idx = 0; blk4x4Idx < 4; blk4x4Idx++)
			{
				dct_src_ptr = (int16 *)vsp_dct_io_0 + 16*16+sub4x4_chroma_offset[blk4x4Idx];
				dct_dst_ptr = (int16 *)vsp_dct_io_1 + 16*(16+blk4x4Idx);
				dct_vlc_ptr = (int16 *)tmp_vlc_bfr +  16*(16+blk4x4Idx);
				
				trans4x4 (dct_src_ptr, 8, dct_dst_ptr, 16);
				
				//copy dc coeff
				dc_pos = blk4x4Idx;//block_idx_y[blk4x4Idx]*2+block_idx_x[blk4x4Idx];
				dc_ptr[dc_pos] = dct_dst_ptr[0];
				
				//quant
				quant4x4(dct_dst_ptr, qp_per, qp_rem, !Inter_mode_en);

#ifdef TV_OUT
				for(i=0; i<4; i++)
					for(j=0; j<4; j++)
						dct_out_buf[16*16+sub4x4_chroma_offset[blk4x4Idx]+j+8*i]= dct_dst_ptr[i+4*j];
#endif
				//scan
				zigzag4x4(dct_dst_ptr, dct_vlc_ptr+1); //for ac coeff

				if (b_decimate)
				{
					blk8_score += MBSkipDecimateScore (dct_vlc_ptr+1, 15);
				}

				blk_nnz[16+blk4x4Idx] = get_nnz4x4(dct_dst_ptr, 1);
				cbp_cb = (blk_nnz[16+blk4x4Idx] != 0) ? 0x1 : cbp_cb;	
			}
			
			if (b_decimate && (blk8_score < CHROMA_COEFF_COST))
			{
				blk_nnz[16 + 0] = 0;
				blk_nnz[16 + 1] = 0;
				blk_nnz[16 + 2] = 0;
				blk_nnz[16 + 3] = 0;
				cbp_cb			= 0;
				
				dct_dst_ptr = (int16 *)vsp_dct_io_1 + 16*16;
				memset (dct_dst_ptr, 0, sizeof(int16)*4*16);
#ifdef TV_OUT
				for(j=0; j<64; j++)
					dct_out_buf[16*16+j]= 0;
#endif
				dct_vlc_ptr = (int16 *)tmp_vlc_bfr + 16*16;
				memset (dct_vlc_ptr, 0, sizeof(int16)*4*16);
			}
			
			//dc
			dct2x2dc(dc_ptr);
			quant2x2dc(dc_ptr, qp_per, qp_rem, !Inter_mode_en);
#ifdef TV_OUT
			for(i=0; i<2; i++)
				for(j=0; j<2; j++)
					dct_out_buf[400+j+2*i]= dc_ptr[i+2*j];
#endif
			zigzag2x2_dc(dc_ptr, chroma_2x2_dc);
			
			//v
			dc_ptr = (int16 *)(vsp_dct_io_1 + HDCT_COEFF_CHROMA_DC_BASE+2);
			chroma_2x2_dc = (int16 *)(tmp_vlc_bfr + HDCT_COEFF_CHROMA_DC_BASE+2);
			
			blk8_score = 0;
			for (blk4x4Idx = 0; blk4x4Idx < 4; blk4x4Idx++)
			{
				dct_src_ptr = (int16 *)vsp_dct_io_0 + 16*20+ sub4x4_chroma_offset[blk4x4Idx];
				dct_dst_ptr = (int16 *)vsp_dct_io_1 + 16*(20+blk4x4Idx);
				dct_vlc_ptr = (int16 *)tmp_vlc_bfr + 16*(20+ blk4x4Idx);
				
				trans4x4 (dct_src_ptr, 8, dct_dst_ptr, 16);
				
				//copy dc coeff
				dc_pos = block_idx_y[blk4x4Idx]*2+block_idx_x[blk4x4Idx];
				dc_ptr[dc_pos] = dct_dst_ptr[0];
				
				//quant
				quant4x4(dct_dst_ptr, qp_per, qp_rem, !Inter_mode_en);
#ifdef TV_OUT
				for(i=0; i<4; i++)
					for(j=0; j<4; j++)
						dct_out_buf[16*20+sub4x4_chroma_offset[blk4x4Idx]+j+8*i]= dct_dst_ptr[i+4*j];
#endif
				//scan
				zigzag4x4(dct_dst_ptr, dct_vlc_ptr+1); //for ac coeff

				if (b_decimate)
				{
					blk8_score += MBSkipDecimateScore (dct_vlc_ptr+1, 15);
				}
				
				blk_nnz[20+blk4x4Idx] = get_nnz4x4(dct_dst_ptr, 1);			
				cbp_cr = (blk_nnz[20+blk4x4Idx]!= 0) ? 0x1 : cbp_cr;			
			}

			if (b_decimate && (blk8_score < CHROMA_COEFF_COST))
			{
				blk_nnz[20 + 0] = 0;
				blk_nnz[20 + 1] = 0;
				blk_nnz[20 + 2] = 0;
				blk_nnz[20 + 3] = 0;
				cbp_cr			= 0;
				
				dct_dst_ptr = (int16 *)vsp_dct_io_1 + 20*16;
				memset (dct_dst_ptr, 0, sizeof(int16)*4*16);
#ifdef TV_OUT
				for(j=0; j<64; j++)
					dct_out_buf[16*20+j]= 0;
#endif
				dct_vlc_ptr = (int16 *)tmp_vlc_bfr + 20*16;
				memset (dct_vlc_ptr, 0, sizeof(int16)*4*16);
			}

			cbp_chroma = (cbp_cb | cbp_cr) ? 0x2 : 0;

			//dc
			dct2x2dc(dc_ptr);
			quant2x2dc(dc_ptr, qp_per, qp_rem, !Inter_mode_en);
#ifdef TV_OUT
			for(i=0; i<2; i++)
				for(j=0; j<2; j++)
					dct_out_buf[404+j+2*i]= dc_ptr[i+2*j];
#endif
			zigzag2x2_dc(dc_ptr, chroma_2x2_dc);
			
			if (cbp_chroma == 0)
			{
				int32 i;
				int16 *chroma_dc = (int16 *)(tmp_vlc_bfr + HDCT_COEFF_CHROMA_DC_BASE);
				
				for (i = 0; i < 8; i++)
				{
					if (chroma_dc[i])
					{
						cbp_chroma = 1;
						break;
					}
				}	
			}
		}
		
		memcpy (vsp_dct_io_0, tmp_vlc_bfr, DCT_IO_BFR_SIZE*sizeof(uint32));
		g_dct_reg_ptr->nnz_blk_0 = (blk_nnz[0]<<24)  | (blk_nnz[1]<<16)  | (blk_nnz[2]<<8)  | blk_nnz[3];
		g_dct_reg_ptr->nnz_blk_1 = (blk_nnz[4]<<24)  | (blk_nnz[5]<<16)  | (blk_nnz[6]<<8)  | blk_nnz[7];
		g_dct_reg_ptr->nnz_blk_2 = (blk_nnz[8]<<24)  | (blk_nnz[9]<<16)  | (blk_nnz[10]<<8) | blk_nnz[11];
		g_dct_reg_ptr->nnz_blk_3 = (blk_nnz[12]<<24) | (blk_nnz[13]<<16) | (blk_nnz[14]<<8) | blk_nnz[15];
		g_dct_reg_ptr->nnz_blk_4 = (blk_nnz[16]<<24) | (blk_nnz[17]<<16) | (blk_nnz[18]<<8) | blk_nnz[19];
		g_dct_reg_ptr->nnz_blk_5 = (blk_nnz[20]<<24) | (blk_nnz[21]<<16) | (blk_nnz[22]<<8) | blk_nnz[23];
		
		g_dct_reg_ptr->h264_out_cbp = (cbp_chroma<<4) | cbp_luma;
	}
	else
	{
		int16 *idct_src_ptr;
		int16 *idct_dst_ptr;
		
		//luma
		qp_per = (g_dct_reg_ptr->iict_cfg1>>12)&0xf;
		qp_rem = (g_dct_reg_ptr->iict_cfg1>>4)&0xf;
		
		if (mb_type == 1) // skip mb
		{
			return;
		}
		if (mb_type == 0)//i16x16 mode
		{
			int16 *dc_ptr = (int16 *)(vsp_dct_io_1 + HDCT_COEFF_LUMA_DC_BASE);
			int32 dc_pos;

#ifdef TV_OUT
			for(i=0; i<16; i++)
				idct_in_buf[384+i]= dc_ptr[i];//24*16
#endif
			idct4x4dc(dc_ptr); //hadamard
			dequant4x4dc(dc_ptr, qp_per, qp_rem);
			
			for (blk4x4Idx = 0; blk4x4Idx < 16; blk4x4Idx++)
			{
				idct_src_ptr = (int16 *)vsp_dct_io_1 + blk4x4Idx*16;
				idct_dst_ptr = (int16 *)vsp_dct_io_0 + blk4x4Idx*16;

#ifdef TV_OUT
				for(j=0; j<4; j++)
					for(i=0; i<4; i++)
						idct_in_buf[sub4x4_luma_offset[blk4x4Idx]+i+16*j]= idct_src_ptr[i+4*j];
#endif
				dequant4x4(idct_src_ptr, qp_per, qp_rem);
				
				dc_pos = block_idx_y[blk4x4Idx]*4+block_idx_x[blk4x4Idx];
				idct_src_ptr[0] = dc_ptr[dc_pos];
				idct4x4(idct_src_ptr, 4, idct_dst_ptr, 4);
			}
		}
		else// if  (mb_type == I_4x4)
		{
			for (blk4x4Idx = 0; blk4x4Idx < 16; blk4x4Idx++)
			{
				idct_src_ptr = (int16 *)vsp_dct_io_1 + blk4x4Idx*16;
				idct_dst_ptr = (int16 *)vsp_dct_io_0 + blk4x4Idx*16;
				if(!is_I4x4)
				{
#ifdef TV_OUT
					for(j=0; j<4; j++)
						for(i=0; i<4; i++)
							idct_in_buf[sub4x4_luma_offset[blk4x4Idx]+i+16*j]= idct_src_ptr[i+4*j];
#endif
					dequant4x4(idct_src_ptr, qp_per, qp_rem);
				}
				idct4x4(idct_src_ptr, 4, idct_dst_ptr, 4);
			}
		}
		
		//chroma
		qp_per = (g_dct_reg_ptr->iict_cfg1>>8)&0xf;
		qp_rem = (g_dct_reg_ptr->iict_cfg1>>0)&0xf;
		{
			//u
			int16 *dc_ptr = (int16 *)(vsp_dct_io_1 + HDCT_COEFF_CHROMA_DC_BASE);
			int32 dc_pos;

#ifdef TV_OUT
			for(i=0; i<4; i++)
				idct_in_buf[400+i]= dc_ptr[i];
#endif
			dct2x2dc(dc_ptr); //idct
			dequant2x2dc(dc_ptr, qp_per, qp_rem);
			for (blk4x4Idx = 0; blk4x4Idx < 4; blk4x4Idx++)
			{
				idct_src_ptr = (int16 *)vsp_dct_io_1 + 16*(16+blk4x4Idx);
				idct_dst_ptr = (int16 *)vsp_dct_io_0 + 16*(16+blk4x4Idx);

#ifdef TV_OUT
				for(j=0; j<4; j++)
					for(i=0; i<4; i++)
						idct_in_buf[16*16+sub4x4_chroma_offset[blk4x4Idx]+i+8*j]= idct_src_ptr[i+4*j];
#endif
				dequant4x4(idct_src_ptr, qp_per, qp_rem);
				
				dc_pos = blk4x4Idx;//block_idx_y[blk4x4Idx]*4+block_idx_x[blk4x4Idx];
				idct_src_ptr[0] = dc_ptr[dc_pos];
				idct4x4(idct_src_ptr, 4, idct_dst_ptr, 4);
			}
			
			//v
			dc_ptr = (int16 *)(vsp_dct_io_1 + HDCT_COEFF_CHROMA_DC_BASE+2);
#ifdef TV_OUT
			for(i=0; i<4; i++)
				idct_in_buf[404+i]= dc_ptr[i];
#endif
			dct2x2dc(dc_ptr); //idct
			dequant2x2dc(dc_ptr, qp_per, qp_rem);
			for (blk4x4Idx = 0; blk4x4Idx < 4; blk4x4Idx++)
			{
				idct_src_ptr = (int16 *)vsp_dct_io_1 + 16*(20+blk4x4Idx);
				idct_dst_ptr = (int16 *)vsp_dct_io_0 + 16*(20+blk4x4Idx);

#ifdef TV_OUT
				for(j=0; j<4; j++)
					for(i=0; i<4; i++)
						idct_in_buf[16*20+sub4x4_chroma_offset[blk4x4Idx]+i+8*j]= idct_src_ptr[i+4*j];
#endif
				dequant4x4(idct_src_ptr, qp_per, qp_rem);
				
				dc_pos = blk4x4Idx;//block_idx_y[blk4x4Idx]*4+block_idx_x[blk4x4Idx];
				idct_src_ptr[0] = dc_ptr[dc_pos];
				idct4x4(idct_src_ptr, 4, idct_dst_ptr, 4);
			}		
		}
		
	}
	
}