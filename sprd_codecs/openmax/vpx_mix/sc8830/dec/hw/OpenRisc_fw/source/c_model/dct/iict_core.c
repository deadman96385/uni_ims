/*iqt_core.c*/
#include <memory.h>
#include "sci_types.h"
#include "video_common.h"
#include "iict_global.h"
#include "common_global.h"
#include "buffer_global.h"
#include "hvld_mode.h"
#include "rvld_mode.h"

// #include "iqt_local.h"


uint8 g_blKIndex[16] = 
{
	 0,   1,  4,  5 , 
     2,   3,  6,  7 , 
     8,   9, 12,  13, 
     10, 11, 14,  15, 
};

// LOCAL int32 g_postrans [4] = {0, 2, 1, 3};
int32 s_postrans[16] =
	{	0, 2, 1, 3, 
		8, 10, 9,11,
		4, 6, 5, 7,
		12, 14,13, 15
	};
int16 s_quantMatrix[6][16] = 
{
	{160, 208, 160, 208, 208, 256, 208, 256,	160, 208, 160, 208,	208,256, 208, 256, },
	{176, 224, 176, 224, 224, 288, 224, 288,	176, 224, 176, 224,	224, 288, 224, 288,},
	{208, 256, 208, 256, 256, 320, 256, 320,	208, 256, 208, 256,	256, 320, 256, 320,},
	{224, 288, 224, 288, 288, 368, 288, 368,	224, 288, 224, 288,	288, 368, 288, 368,},
	{256, 320, 256, 320, 320, 400, 320, 400,	256, 320, 256, 320,	320, 400, 320, 400,},
	{288, 368, 288, 368, 368, 464, 368, 464,	288, 368, 288, 368,	368, 464, 368, 464,},
};


int16 inverse_quant_rv_h264 (int16 coeff, int32 asic_index, int32 is_luma,int32 is_DC_itrans)
{
	int32 qp_per;
	int32 qp_rem;
	int16 *quant_ptr = NULL;
	int32 normal_idx;
	int32 mb_type = (g_dct_reg_ptr->iict_cfg0 >> 28) & 0x3;
	int32 isH264 = 0;
// 	int32 qp_rv;
	int32 scale_coeff;
	int32 qp_const;
	int32 right_shift;
	int32 shift_value;
	int	  standard	  = (g_glb_reg_ptr->VSP_CFG0 >> 8) & 0xf;

	isH264 = (standard == VSP_H264) ? 1 : 0;
	normal_idx = s_postrans[asic_index];/*g_postrans [asic_index&3] * 4 + g_postrans [(asic_index>>2)];*/

	if (isH264)
	{
		if (is_luma)
		{
			qp_per = (g_dct_reg_ptr->iict_cfg1>>12)&0xf;
			qp_rem = (g_dct_reg_ptr->iict_cfg1>>4)&0xf;
		}else
		{
			qp_per = (g_dct_reg_ptr->iict_cfg1>>8)&0xf;
			qp_rem = (g_dct_reg_ptr->iict_cfg1>>0)&0xf;
		}
	} 
	else
	{//Real 8/9
		if (is_luma)
		{
			if ((normal_idx == 0 || normal_idx == 1 || normal_idx ==4 || standard==VSP_RV8) && is_DC_itrans)
			{
				scale_coeff = (g_dct_reg_ptr->ridct_cfg0 >>0) & 0xfff;
			} 
			else
			{
				scale_coeff = (g_dct_reg_ptr->ridct_cfg0 >>16) & 0xfff;
			}
		} 
		else
		{
			if (normal_idx == 0)
			{
				scale_coeff = (g_dct_reg_ptr->ridct_cfg1 >>0) & 0xfff;
			} 
			else
			{
				scale_coeff = (g_dct_reg_ptr->ridct_cfg1 >>16) & 0xfff;
			}
		}

	}


	if (isH264)
	{
		quant_ptr = s_quantMatrix[qp_rem];
		scale_coeff = quant_ptr[normal_idx];
	} 



	if (isH264)
	{
		if (normal_idx == 0 && is_luma && mb_type ==0 )
		{
			qp_const = (qp_per<6)? (1<<(5-qp_per)): 0;
		} 
		else if(normal_idx == 0 && is_luma == 0)
		{
			qp_const = 0;
		}
		else
		{
			qp_const = (qp_per<4)? (1<<(3-qp_per)):0;
		}	
	} 
	else
	{
		qp_const = 8;
	}


	if (isH264)
	{
		if (normal_idx == 0 && is_luma && mb_type == 0)
		{
			shift_value = (qp_per<6)? (6 - qp_per): (qp_per - 6);
		} 
		else if(normal_idx == 0 && is_luma == 0)
		{
			shift_value = (qp_per<5)? (5 - qp_per): (qp_per - 5);
		}
		else
		{
			shift_value = (qp_per<4)? (4 - qp_per): (qp_per - 4);
		}
	} 
	else
	{
		shift_value = 4;
	}


	if (isH264)
	{
		if (normal_idx == 0 && is_luma && mb_type == 0)
		{
			right_shift = (qp_per<6)? 1: 0;
		}
		else if (normal_idx == 0 && is_luma == 0)
		{
			right_shift = (qp_per<5)? 1: 0;
		} 
		else  
		{
			right_shift = (qp_per<4)? 1: 0;
		}
	} 
	else
	{
		right_shift = 1;
	}


	if (right_shift)
	{
		coeff =(coeff * scale_coeff + qp_const)>>shift_value;
	} 
	else
	{
		coeff =(coeff * scale_coeff + qp_const)<<shift_value;
	}

	
	return coeff;
}

void Iict(int32 is_DC_itrans, int32 blk4x4Idx, int32 is_luma, int32 second_phase, int32 is_h264, int32 cbp26,  int32 need_y_hadama)
{
	int32 dbuf_base ; 
	int32 i;
	int32 data0, data1;
	int32 sum0, sub0;
	int32 idct_sum0, idct_sub0;
	int32 rv_sum0, rv_sub0;
	int32 rv_tmp_sum0, rv_tmp_sub0;
	int32 rv_idct_sum0, rv_idct_sub0;
	int32 sum0_reg, sub0_reg;
	int32 sum1_reg, sub1_reg;
	int32 xr0, xr1, xr2, xr3;
	int32 idct_xr0, idct_xr1;
	int32 rv_idct_xr0, rv_idct_xr1;
	int32 xr_out0, xr_out1;
	int32 dbuf_rdata, dbuf_wdata, dbuf_addr;
	int32 trans0_rdata, trans0_wdata, trans0_addr;
	int32 trans1_radta, trans1_wdata, trans1_addr;
	int32 flt_data0, flt_data1;
	int32 iict_flt_data0, iict_flt_data1;
	int32 rv_flt_data0, rv_flt_data1;
	int32 rv_iict_flt_data0, rv_iict_flt_data1;
	int32 is_uv_hadama, is_y_hadama, is_iict;
	int32 nz_flag0, nz_flag1;
	int16 pre_iq_data0, pre_iq_data1;
	int32 after_iq_data0, after_iq_data1;
	int32 select_dc;
	int16 blk_dc_value;
	int32 dbuf_rd, dbuf_wr;
	int32 trans_rd, trans_wr;
	int32 dout_cnt = 0;
	int32 trans_wdata_inv, trans_rdata_inv;
	int32 data_reg0, data_reg1, data_reg2, data_reg3, data_reg4, data_reg5, data_reg6, data_reg7;
	int32 dbuf_wen;
	int32 rd_nxt_nzf, rd_nxt_dc, rd_nzf;
	int32 nz_flag_in, iict_dc_in;
	int32 select_odd_dc, set_first_dc;
	int32 blk_id,nxt_blk_id;
	int32 uv_reg0, uv_reg1;


	is_uv_hadama = (is_h264 && !is_luma && is_DC_itrans)? 1: 0; 
	is_y_hadama = (is_luma && is_DC_itrans)? 1: 0;
	is_iict = (!is_DC_itrans)? 1: 0;


	
	if (is_uv_hadama)
	{
		dbuf_base = COEFF_CHROMA_DC_BASE;
	}
	else if (is_y_hadama)
	{
		dbuf_base = COEFF_LUMA_DC_BASE;
	}
	else
	{
		dbuf_base = COEFF_LUMA_AC_BASE + 8 * blk4x4Idx;
	}


	for (i=0; i<= (rd_cnt_max ); i++)
	{
		//IO buf control logic
		dbuf_rd = (i<=fetch_max) && ((!second_phase)||(is_uv_hadama && second_phase));
		dbuf_wr =  (i<=(rd_cnt_max)) && (((!is_uv_hadama) && second_phase && (i>=1)) || (is_uv_hadama && (i>=3)));
		
		// Trans buf control logic
		trans_rd = (i<=fetch_max) && second_phase && (!is_uv_hadama);
		trans_wr = (i>=1) && (/*i<=rd_cnt_max*/dout_cnt<=7) && (!second_phase) && (!is_uv_hadama);

		// Read data
		if (dbuf_rd)
		{
			//Read from iobuf
			dbuf_addr = dbuf_base + i + (second_phase <<1);
			dbuf_rdata = vsp_dct_io_0[dbuf_addr] ;
		}		
		else if(trans_rd)
		{
			//Read from trans0 and trans1
			switch(i)
			{
			case 0: trans0_addr = 0;	trans1_addr = 2; break;
			case 1: trans0_addr = 4;	trans1_addr = 6; break;
			case 2: trans0_addr = 2;	trans1_addr = 0; break;
			case 3: trans0_addr = 6;	trans1_addr = 4; break;
			case 4: trans0_addr = 1;	trans1_addr = 3; break;
			case 5: trans0_addr = 5;	trans1_addr = 7; break;
			case 6: trans0_addr = 3;	trans1_addr = 1; break;
			case 7: trans0_addr = 7;	trans1_addr = 5; break;
			default: trans0_addr = 0;	trans1_addr = 0;
			}

			trans_rdata_inv = (i>>1) & 0x1;	
			trans0_rdata = (trans_rdata_inv)? g_vsp_dct_trans1[trans1_addr] : g_vsp_dct_trans0[trans0_addr];
			trans1_radta = (trans_rdata_inv)? g_vsp_dct_trans0[trans0_addr] : g_vsp_dct_trans1[trans1_addr];
			
		}


		//if first phase end or uv_hadama, read next nzf and dc. Address control logic.
		rd_nxt_nzf = rd_nxt_nzf_ena ||((is_uv_hadama && (i == 2))||((is_y_hadama || is_iict) && (i ==8) && (!second_phase)));
		rd_nzf = rd_nxt_nzf || rd_u_dc_nzf || rd_y_dc_nzf || rd_y_ac_nzf;
		rd_nxt_dc = rd_nxt_dc_ena || (is_iict && (i == 9) && (!second_phase));

		if (rd_nzf )
		{
			if (rd_u_dc_nzf)
			{
				dbuf_addr = NZFLAG_CHROMA_DC_BASE;
			}
			else if (rd_y_dc_nzf)
			{
				dbuf_addr = NZFLAG_LUMA_DC_BASE;
			}
			else if (rd_y_ac_nzf)
			{
				dbuf_addr = NZFLAG_LUMA_AC_BASE;
			}
			else if (is_uv_hadama)
			{
				if (second_phase)
				{
					if (need_y_hadama)
					{
						dbuf_addr = NZFLAG_LUMA_DC_BASE;
					} 
					else
					{
						dbuf_addr = NZFLAG_LUMA_AC_BASE;
					}
				} 
				else
				{
					dbuf_addr = NZFLAG_CR_DC_BASE;
				}
			}
			else if (is_y_hadama)
			{
				dbuf_addr = NZFLAG_LUMA_AC_BASE;
			}
			else if (is_iict)
			{
				dbuf_addr = NZFLAG_LUMA_AC_BASE + blk4x4Idx + 1; // Read next iict blk nzf
			}
			nz_flag_in = (vsp_dct_io_0[dbuf_addr]) & 0xffff;

		}


		blk_id = (blk4x4Idx < 16)? (g_blKIndex[blk4x4Idx]) : blk4x4Idx; // Trans order for luma blk
		if (is_h264)
		{
			nxt_blk_id = ((blk4x4Idx+1) < 16)? (g_blKIndex[(blk4x4Idx+1)]) : (blk4x4Idx+1);
		} 
		else
		{ //NOTE!Blk order in real is still 0,1,2,4...  To be fixed!
			nxt_blk_id = ((blk4x4Idx+1) < 16)? (g_blKIndex[(blk4x4Idx+1)]) : (blk4x4Idx+1);
			//nxt_blk_id = (blk4x4Idx+1); //original
		  //NOTE!Blk order in real is still 0,1,2,4...  To be fixed!
		}

		select_odd_dc = nxt_blk_id & 0x1;
		if (rd_nxt_dc)
		{	
			// Read next dc only when is_iict 
			dbuf_addr = COEFF_LUMA_DC_BASE + (nxt_blk_id>>1);
			iict_dc_in = (select_odd_dc)? (vsp_dct_io_0[dbuf_addr] >>16) : (vsp_dct_io_0[dbuf_addr] & 0xffff);
		}

		// if is_y_hamdama, the dc of next first luma ac blk is set
		set_first_dc = dbuf_wr && (dout_cnt == 0) && is_y_hadama && second_phase;



		select_dc = (is_iict && ((need_y_hadama && blk4x4Idx<16) || (blk4x4Idx>=16 && is_h264)) && (!second_phase) && (i==0))? 1: 0;

		// Nz_flag computation
		if ( (!(cbp26 &(1<<blk4x4Idx))) && is_iict)
		{
			if (i == 0)
			{
				nz_flag0 = (select_dc)? 1: (nz_flag & 0x1);
				nz_flag1 = 0;
			} 
			else
			{
				nz_flag0 = 0;
				nz_flag1 = 0;
			}
		}
		else
		{
			if (i == 0)
			{
				nz_flag0 = (select_dc)? 1: (nz_flag & 0x1);
				nz_flag1 = (nz_flag>>1) & 0x1;
			} 
			else
			{
				nz_flag0 = (nz_flag>>(2*i)) & 0x1;
				nz_flag1 = (nz_flag>>(2*i+1)) & 0x1;
			}
		}



		//Inverse quant

		
		blk_dc_value = select_dc? dc_value: (dbuf_rdata & 0xffff);
		pre_iq_data0 = nz_flag0? blk_dc_value: 0;
		pre_iq_data1 = nz_flag1? ((dbuf_rdata>>16) & 0xffff): 0;
		after_iq_data0 = (int16)inverse_quant_rv_h264((int16)pre_iq_data0,(i<<1),is_luma, is_DC_itrans) & 0xffff;
		after_iq_data1 = (int16)inverse_quant_rv_h264((int16)pre_iq_data1,((i<<1)+1),is_luma, is_DC_itrans) & 0xffff; 
		// expand the sign bit
		after_iq_data0 = ((after_iq_data0>>15) & 1)?(0xffff0000 | after_iq_data0) : after_iq_data0;
		after_iq_data1 = ((after_iq_data1>>15) & 1)?(0xffff0000 | after_iq_data1) : after_iq_data1;

		//data0,1 = dbuf_rdata or trans0,1_rdata, or iq out data
		if(trans_rd)
		{	//Read from transbuf0,1 
			data0 = trans0_rdata;
			data1 = trans1_radta;
		}
		else if (dbuf_rd)
		{
			if (is_uv_hadama || (is_y_hadama && is_h264))
			{
				//Read data from iobuf without iq for H264 UV_HADAMA and H264_YHADAMA
				data0 = (nz_flag0)? (dbuf_rdata & 0xffff): 0;
				data1 = (nz_flag1)? ((dbuf_rdata>>16) & 0xffff): 0;
			}
			else if (!is_h264 && need_y_hadama && !is_y_hadama && is_luma && select_dc)
			{	//Real luma ac without dc. dc DONOT need iq
				data0 = pre_iq_data0;
				data1 = after_iq_data1;
			}
			else
			{	// H264 ac iict and real luma dc and real luma ac with dc. After iq.
				data0 = after_iq_data0;
				data1 = after_iq_data1;
			}
		}
		


		
		//ITrans 1-D computation
		sum0 = data0 + data1;
		sub0 = data0 - data1;
		idct_sum0 = data0 + (data1>>1);
		idct_sub0 = (data0>>1) - data1;
		rv_sum0 = (sum0<<4) - (sum0<<1) - sum0;//13*sum0;
		rv_sub0 = (sub0<<4) - (sub0<<1) - sub0;//13*sub0;
		rv_tmp_sum0 = (data0 <<1) + data1;
		rv_tmp_sub0 = data0 - (data1 <<1);
		rv_idct_sum0 = (rv_tmp_sum0<<3) + sub0;//17*data0 + 7data1;
		rv_idct_sub0 = (rv_tmp_sub0<<3) - sum0;//7*data0 - 17*data1;


		xr0 = sum0_reg + sum0;
		xr1 = sub0_reg + sub0;
		xr2 = sub0_reg - sub1_reg;
		xr3 = sum0_reg - sum1_reg;

		idct_xr0 = sum0_reg + idct_sum0;
		idct_xr1 = sub0_reg + idct_sub0;

		rv_idct_xr0 = sum0_reg + rv_idct_sum0;
		rv_idct_xr1 = sub0_reg + rv_idct_sub0;
		
		//Output data from second cycle
		if (i%2 == 1)
		{
			if (is_uv_hadama)
			{
				xr_out0 = xr0;
				xr_out1 = xr1;
			}
			else if (is_y_hadama && is_h264)
			{
				xr_out0 = xr0;
				xr_out1 = xr1;
			}
			else if (is_h264)
			{
				xr_out0 = idct_xr0;
				xr_out1 = idct_xr1;
			}
			else
			{
				xr_out0 = rv_idct_xr0;
				xr_out1 = rv_idct_xr1;
			}
			
		}
		else if (i>0 && i%2==0)
		{
			if (is_uv_hadama)
			{
				xr_out0 = xr3;
				xr_out1 = xr2;
			}
			else if (is_y_hadama && is_h264)
			{
				xr_out0 = xr2;
				xr_out1 = xr3;
			}
			else if (is_h264)
			{
				xr_out0 = xr2;
				xr_out1 = xr3;
			}
			else
			{
				xr_out0 = xr2;
				xr_out1 = xr3;
			}
		}


		//Filter out logic
		iict_flt_data0 = (xr_out0 + 32) >>6;
		iict_flt_data1 = (xr_out1 + 32) >>6;

		rv_flt_data0 = ((xr_out0<<5) + (xr_out0<<4)) >>15;
		rv_flt_data1 = ((xr_out1<<5) + (xr_out1<<4)) >>15;		

		rv_iict_flt_data0 = (xr_out0 + 512) >>10;
		rv_iict_flt_data1 = (xr_out1 + 512) >>10;

		if (is_h264)
		{
			if (is_iict)
			{
				if (second_phase) 
				{
					flt_data0 = iict_flt_data0;
					flt_data1 = iict_flt_data1;
				} 
				else
				{
					flt_data0 = xr_out0;
					flt_data1 = xr_out1;
				}
			} 
			else
			{
				flt_data0 = xr_out0;
				flt_data1 = xr_out1;
			}
		} 
		else
		{
			if (is_iict)
			{
				if (second_phase) 
				{
					flt_data0 = rv_iict_flt_data0;
					flt_data1 = rv_iict_flt_data1;
				} 
				else
				{
					flt_data0 = xr_out0;
					flt_data1 = xr_out1;
				}
			} 
			else
			{
				if (second_phase)
				{
					flt_data0 = rv_flt_data0;
					flt_data1 = rv_flt_data1;
				} 
				else
				{
					flt_data0 = xr_out0;
					flt_data1 = xr_out1;
				}
			}			
		}



		//Update tmp registers
		if (i%2 == 0)
		{
			if (is_uv_hadama)
			{
				sum0_reg = sum0;
				sub0_reg = sub0;
			}
			else if (is_y_hadama && is_h264)
			{
				sum0_reg = sum0;
				sub0_reg = sub0;
			}
			else if (is_h264)
			{
				sum0_reg = sum0;
				sub0_reg = sub0;
			}
			else
			{
				sum0_reg = rv_sum0;
				sub0_reg = rv_sub0;
			}
			
		}
		else if (i%2==1)
		{
			if (is_uv_hadama)
			{
				sum1_reg = sum0;
				sub1_reg = sub0;
			}
			else if (is_y_hadama && is_h264)
			{
				sum1_reg = sum0;
				sub1_reg = sub0;
			}
			else if (is_h264)
			{
				sum1_reg = idct_sum0;
				sub1_reg = idct_sub0;
			}
			else
			{
				sum1_reg = rv_idct_sum0;
				sub1_reg = rv_idct_sub0;
			}
		}



		// register uv_hadama output
		if (is_uv_hadama)
		{
			if (i == 1)
			{
				uv_reg0 = (flt_data1 <<16) | (flt_data0 & 0xffff);
			}
			else if (i == 2)
			{
				uv_reg1 = (flt_data1 <<16) | (flt_data0 & 0xffff);
			}
		}

		if (trans_wr)
		{
			// write trans0 ,1
			trans0_addr = dout_cnt;
			trans1_addr = dout_cnt;
			trans_wdata_inv = (dout_cnt>>1) & 0x1;	
			trans0_wdata = (trans_wdata_inv)? flt_data1 : flt_data0;
			trans1_wdata = (trans_wdata_inv)? flt_data0 : flt_data1;
			g_vsp_dct_trans0[trans0_addr] = trans0_wdata;
			g_vsp_dct_trans1[trans1_addr] = trans1_wdata;
		} 
		else if (dbuf_wr)
		{
			// write iobuf
			// Store flt_data1 to data_reg0~7
			switch(dout_cnt)
			{
			case 0: data_reg0 = flt_data1 & 0xffff; break;
			case 1: data_reg1 = flt_data1 & 0xffff; break;
			case 2: data_reg2 = flt_data1 & 0xffff; break;
			case 3: data_reg3 = flt_data1 & 0xffff; break;
			case 4: data_reg4 = flt_data1 & 0xffff; break;
			case 5: data_reg5 = flt_data1 & 0xffff; break;
			case 6: data_reg6 = flt_data1 & 0xffff; break;
			case 7: data_reg7 = flt_data1 & 0xffff; break;
			}
			
			if (is_uv_hadama)
			{
				if(i == 3)
					dbuf_wdata = uv_reg0;//(flt_data1 <<16) | (flt_data0 & 0xffff);
				else if (i == 4)
					dbuf_wdata = uv_reg1;
				dbuf_wen = 0;
				dbuf_addr = dbuf_base + dout_cnt + (second_phase<<1);
			} 
			else
			{
				switch(dout_cnt)
				{
				case 0: 
					dbuf_wdata = (flt_data0 & 0xffff); 
					dbuf_addr = dbuf_base + 0; 
					dbuf_wen = 2;
					break;
				case 1: 
					dbuf_wdata = (flt_data0 & 0xffff);
					dbuf_addr = dbuf_base + 4; 
					dbuf_wen = 2;
					break;
				case 2: 
					dbuf_wdata = (flt_data0 <<16); 
					dbuf_addr = dbuf_base + 0; 
					dbuf_wen = 1;
					break;
				case 3: 
					dbuf_wdata = (flt_data0 <<16); 
					dbuf_addr = dbuf_base + 4; 
					dbuf_wen = 1;
					break;
				case 4: 
					dbuf_wdata = (flt_data0 & 0xffff); 
					dbuf_addr = dbuf_base + 1; 
					dbuf_wen = 2;
					break;
				case 5: 
					dbuf_wdata = (flt_data0 & 0xffff); 
					dbuf_addr = dbuf_base + 5; 
					dbuf_wen = 2;
					break;
				case 6: 
					dbuf_wdata = (flt_data0 <<16); 
					dbuf_addr = dbuf_base + 1; 
					dbuf_wen = 1;
					break;
				case 7: 
					dbuf_wdata = (flt_data0 <<16); 
					dbuf_addr = dbuf_base + 5; 
					dbuf_wen = 1;
					break;
				case 8: 
					dbuf_wdata = (data_reg2 <<16) | data_reg0; 
					dbuf_addr = dbuf_base + 2; 
					dbuf_wen = 0;
					break;
				case 9: 
					dbuf_wdata = (data_reg6 <<16) | data_reg4; 
					dbuf_addr = dbuf_base + 3; 
					dbuf_wen = 0;
					break;
				case 10: 
					dbuf_wdata = (data_reg3 <<16) | data_reg1; 
					dbuf_addr = dbuf_base + 6; 
					dbuf_wen = 0;
					break;
				case 11: 
					dbuf_wdata = (data_reg7 <<16) | data_reg5; 
					dbuf_addr = dbuf_base + 7; 
					dbuf_wen = 0;
					break;
				}
			}

		}

		if (dbuf_wr)
		{
			if (dbuf_wen == 0)
			{
				vsp_dct_io_0[dbuf_addr] = dbuf_wdata;
			}
			else if (dbuf_wen == 1)
			{
				vsp_dct_io_0[dbuf_addr] = (vsp_dct_io_0[dbuf_addr] & 0xffff) | dbuf_wdata;
			}
			else if (dbuf_wen == 2)
			{
				vsp_dct_io_0[dbuf_addr] = (vsp_dct_io_0[dbuf_addr] & 0xffff0000) | dbuf_wdata;
			}
		}


		// updata dc_value and nz_flag

		if (set_first_dc)
		{
			dc_value = flt_data0 & 0xffff;
		}
		else if (rd_nxt_dc)
		{
			dc_value = iict_dc_in;
		}


		if (rd_nzf)
		{
			nz_flag = ((blk4x4Idx >= 16) && is_h264)? ((cbp26 & (1<<25))? nz_flag_in : 0): nz_flag_in;
		}

		 //update outdata counter
		if (dbuf_wr || trans_wr)
		{
			dout_cnt = dout_cnt + 1;
		} 
		else
		{
			dout_cnt = 0;
		}
	}
		return;
}