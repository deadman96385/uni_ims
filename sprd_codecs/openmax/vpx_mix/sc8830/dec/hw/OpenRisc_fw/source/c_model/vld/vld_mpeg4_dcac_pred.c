/******************************************************************************
 ** File Name:    vld_mpeg4_dcac_pred.c 									  *
 ** Author:       Xiaowei Luo                                                 *
 ** DATE:         11/19/2008                                                  *
 ** Copyright:    2006 Spreatrum, Incoporated. All Rights Reserved.           *
 ** Description:   c model of bsmr module in mpeg4 decoder                    *
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

#if defined(MPEG4_DEC)
extern DEC_VOP_MODE_T *g_dec_vop_mode_ptr;
#endif

#define VLD_HORIZONTAL		1
#define VLD_VERTICAL		2

/************************************************************
function:
   compute dc_scaler
***********************************************************/
int GetDCScaler (int qp_pred, int blk_cnt)
{
	int dc_scaler;

	if (blk_cnt < 4)
	{
		if (qp_pred <= 4)
			dc_scaler = 8;
		else if (qp_pred <= 8)
			dc_scaler = qp_pred << 1;
		else if (qp_pred <= 24)
			dc_scaler = qp_pred + 8;
		else
			dc_scaler = qp_pred*2 - 16;
	}
	else
	{
		if (qp_pred <= 4)
			dc_scaler = 8;
		else if (qp_pred <= 24)
			dc_scaler = (qp_pred + 13) / 2;
		else 
			dc_scaler = qp_pred - 6;
	}

	return dc_scaler;
}


/***********************************************************
function:
   compute div_inverse
uint16 inv_scaler [49] = 
{
	0x0000, 0x0000, 0x0000, 0xaaab, 0x8001, 0x6667, 0x5556, 0x4925, 
	0x4001, 0x38e4, 0x3334, 0x2e8c, 0x2aab, 0x2763, 0x2493, 0x2223, 
	0x2001, 0x1e1f, 0x1c72, 0x1af3, 0x199a, 0x1862, 0x1746, 0x1643, 
	0x1556, 0x147b, 0x13b2, 0x12f7, 0x124a, 0x11a8, 0x1112, 0x1085, 
	0x1001, 0x0f84, 0x0f10, 0x0ea1, 0x0e39, 0x0dd7, 0x0d7a, 0x0d21, 
	0x0ccd, 0x0c7d, 0x0c31, 0x0be9, 0x0ba3, 0x0b61, 0x0b22, 0x0ae5, 
	0x0aab, 
}

Q_SHIFT = 17

************************************************************/
int vld_GetDivInverse (int dc_scaler)
{
	int div_inverse;
	
	switch (dc_scaler)
	{
	case 3:
		div_inverse = 0xaaab;
		break;		
	case 4:
		div_inverse = 0x8001;
		break;
	case 5:
		div_inverse = 0x6667;
		break;
	case 6:
		div_inverse = 0x5556;
		break;
	case 7:
		div_inverse = 0x4925;
		break;
	case 8:
		div_inverse = 0x4001;
		break;
	case 9:
		div_inverse = 0x38e4;
		break;
	case 10:
		div_inverse = 0x3334;
		break;
	case 11:
		div_inverse = 0x2e8c;
		break;
	case 12:
		div_inverse = 0x2aab;
		break;	
	case 13:
		div_inverse = 0x2763;
		break;
	case 14:
		div_inverse = 0x2493;
		break;
	case 15:
		div_inverse = 0x2223;
		break;	
	case 16:
		div_inverse = 0x2001;
		break;	
	case 17:
		div_inverse = 0x1e1f;
		break;
	case 18:
		div_inverse = 0x1c72;
		break;
	case 19:
		div_inverse = 0x1af3;
		break;		
	case 20:
		div_inverse = 0x199a;
		break;		
	case 21:
		div_inverse = 0x1862;
		break;
	case 22:
		div_inverse = 0x1746;
		break;
	case 23:
		div_inverse = 0x1643;
		break;
	case 24:
		div_inverse = 0x1556;
		break;
	case 25:
		div_inverse = 0x147b;
		break;
	case 26:
		div_inverse = 0x13b2;
		break;
	case 27:
		div_inverse = 0x12f7;
		break;
	case 28:
		div_inverse = 0x124a;
		break;
	case 29:
		div_inverse = 0x11a8;
		break;
	case 30:
		div_inverse = 0x1112;
		break;
	case 31:
		div_inverse = 0x1085;
		break;
	case 32:
		div_inverse = 0x1001;
		break;
	case 33:
		div_inverse = 0x0f84;
		break;
	case 34:
		div_inverse = 0x0f10;
		break;
	case 35:
		div_inverse = 0x0ea1;
		break;
	case 36:
		div_inverse = 0x0e39;
		break;
	case 37:
		div_inverse = 0x0dd7;
		break;
	case 38:
		div_inverse = 0x0d7a;
		break;
	case 39:
		div_inverse = 0x0d21;
		break;
	case 40:
		div_inverse = 0x0ccd;
		break;
	case 41:
		div_inverse = 0x0c7d;
		break;
	case 42:
		div_inverse = 0x0c31;
		break;
	case 43:
		div_inverse = 0x0be9;
		break;
	case 44:
		div_inverse = 0x0ba3;
		break;
	case 45:
		div_inverse = 0x0b61;
		break;
	case 46:
		div_inverse = 0x0b22;
		break;
	case 47:
		div_inverse = 0x0ae5;
		break;
	case 48:
		div_inverse = 0x0aab;
		break;
	default:
		div_inverse = 0;
		break;
	}

	return div_inverse;	
}

/*****************************************************************************
judge the coefficient is zero or not
input:
nz_flag_acdc: none zero flag for acdc
direct: process direction
i: coefficient from 0~7 

return:
the coefficient is zero or not, 1: none zero

  nz_flag_acdc: 14~0
  |56|48|40|32|24|16|8|7|6|5|4|3|2|1|0|  : in dct/io buffer order
   14                                0
******************************************************************************/
int GegNZFlag (int direct, int nz_flag_acdc, int is_fst_row_dctio, int i)
{
	int nz_flag;

	if (is_fst_row_dctio)   //transposed, mapping to 0~7
	{
		switch (i)
		{
		case 0:
			nz_flag = (direct == VLD_HORIZONTAL) ? ((nz_flag_acdc>>0) & 1) : 1;
			break;
		case 1:
			nz_flag = (nz_flag_acdc>>1) & 1;
			break;
		case 2:
			nz_flag = (nz_flag_acdc>>2) & 1;
			break;
		case 3:
			nz_flag = (nz_flag_acdc>>3) & 1;
			break;
		case 4:
			nz_flag = (nz_flag_acdc>>4) & 1;
			break;
		case 5:
			nz_flag = (nz_flag_acdc>>5) & 1;
			break;
		case 6:
			nz_flag = (nz_flag_acdc>>6) & 1;
			break;
		case 7:
			nz_flag = (nz_flag_acdc>>7) & 1;
			break;
		default:
			nz_flag = 0;
			break;
		}
	}
	else  
	{
		switch (i)
		{
		case 0:
			nz_flag = (direct == VLD_HORIZONTAL) ? ((nz_flag_acdc>>0) & 1) : 1;
			break;
		case 1:
			nz_flag = (nz_flag_acdc>>8) & 1;
			break;
		case 2:
			nz_flag = (nz_flag_acdc>>9) & 1;
			break;
		case 3:
			nz_flag = (nz_flag_acdc>>10) & 1;
			break;
		case 4:
			nz_flag = (nz_flag_acdc>>11) & 1;
			break;
		case 5:
			nz_flag = (nz_flag_acdc>>12) & 1;
			break;
		case 6:
			nz_flag = (nz_flag_acdc>>13) & 1;
			break;
		case 7:
			nz_flag = (nz_flag_acdc>>14) & 1;
			break;
		default:
			nz_flag = 0;
			break;
		}
	}

	return nz_flag;
}


/**************************************************************************************
function:
get one dct coeff diff 
byte_ena: 1: second coeff in one word, 0: first coeff in one word
**************************************************************************************/
int16 GetDctCoeff (int nz_flag, int is_dc_coded_as_ac, int is_dc, int delta_dc, int address, int direct)
{
	int coeff_diff;
	int16*  dct_io_ptr = (int16 *)vsp_dct_io_1;

	if (is_dc && !is_dc_coded_as_ac && (direct == VLD_HORIZONTAL))
	{
		coeff_diff = delta_dc;
	}
	else
	{
		if (nz_flag == 0)
			coeff_diff = 0;
		else
			coeff_diff = dct_io_ptr [address];
	}

	return coeff_diff;
}


int GetPredEna (
				int i_coeff,					//the index of coeff in 1th row or 1th column
				int ac_pred_ena,				//ac prediction enable or not, configured by firmware
				int direct,						//process direction, horizontal or vertical
				int pred_direct,				//dc/ac prediction 
				int blk_cnt,					//block number
				int left_avail,					//left MB is available or not
				int top_avail					//top MB is available or not
				)
{
	int pred_ena;
	
	if (i_coeff == 0)
	{
		pred_ena = (direct == VLD_HORIZONTAL) ? 1 : 0;
	}
	else
	{
		if (!ac_pred_ena)
		{
			pred_ena = 0;
		}
		else
		{
			if (direct != pred_direct)
			{
				pred_ena = 0;
			}
			else
			{
				if (pred_direct == VLD_HORIZONTAL)
				{
					if ((blk_cnt == 1) || (blk_cnt == 3))
						pred_ena = 1;
					else
						pred_ena = left_avail;
				}
				else
				{
					if ((blk_cnt == 2) || (blk_cnt == 3))
					{
						pred_ena = 1;
					}
					else
					{
						pred_ena = top_avail;
					}
				}
			}
		}
	}

	return pred_ena;
}

/**************************************************************************************************

dc_dire_pred:
need 4 dc register:

actual address in DCT/IO buffer, and considering rotation
nz_flag_acdc: 15 bits, get from vld one block
| 56 | 48 | 40 | 32 | 24 | 16 | 8 | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
  14   13	12	 11	  10	9	8							    0bit

**************************************************************************************************/
#define DEFAULT_VALUE	1024


void mpeg4dec_acdc_pred (
						 int standard,
						 int is_rvlc,
						 int mb_type,
						 int cbp,
						 int rotation_ena
						 )
{	
	//register file
	int			is_dc_coded_as_ac;	
	int			top_qp;
	int			top_avail;
	int			left_qp;
	int			left_avail;
	int			qp;
	int			mb_x;
	int			ac_pred_ena;
	int			is_data_partition;

	//output
	int			vld_error = 0;

	//bsm interface
	uint32		bsm_out;	
	int			flush_bits;	

	//vld interface
	int			start_position;
	int			blk_cnt;
	int			zigzag_mode;
	int			nz_flag_acdc;			//to store none-zero flag of first row and first column, 15 bits register

//	uint16		top_left_dc;
	int			top_coeff_offset;
	int			left_coeff_offset;

	//four dc register
	uint16		tl_dc_reg;				//store top left dc
	uint16		t_dc_reg;				//store top dc
	uint16		l_dc_reg;				//store left dc
	uint16		l1_dc_reg;				//dc register for L1


	uint32*		acdc_coeff_top_ptr;		//acdc_coeff address in external memory	
	uint32*		acdc_coeff_cur_ptr;		//acdc_coeff address in on-chip memory


	int			grad_ver;
	int			grad_hor;
	int			pred_direct;
	int			huff_tab_out_val;

	int			dc_size;
	int			dc_abs;
	int			dc_max;
	int			is_positive;

	int			delta_dc;

	int			dc_value;
	int			qp_pred;

	
	int		nz_flag;

	int transpose_blkId;


	/*register file*/
	mb_x					= (rotation_ena)?((g_glb_reg_ptr->VSP_CTRL0>>8) & 0x7f):(g_glb_reg_ptr->VSP_CTRL0 & 0x7f);//weihu //0x3f
//	acdc_coeff_top_ptr		= (uint32 *)(g_ahbm_reg_ptr->AHBM_FRM_ADDR_7<<8);
	acdc_coeff_top_ptr      = PNULL;
#if defined(MPEG4_DEC)
	acdc_coeff_top_ptr		= (uint32 *)(g_dec_vop_mode_ptr->pTopCoeff);
#endif
	ac_pred_ena				= (g_vld_reg_ptr->MPEG4_CFG1 >>  0) & 1;
	is_dc_coded_as_ac		= (g_vld_reg_ptr->MPEG4_CFG1 >>  1) & 1;
	is_data_partition		= (g_vld_reg_ptr->MPEG4_CFG1 >>  2) & 1;
	top_avail				= (g_vld_reg_ptr->MPEG4_CFG1 >> 21) & 1;
	left_avail				= (g_vld_reg_ptr->MPEG4_CFG1 >> 13) & 1;
	top_qp					= (g_vld_reg_ptr->MPEG4_CFG1 >> 16) & 0x1f;
	left_qp					= (g_vld_reg_ptr->MPEG4_CFG1 >>  8) & 0x1f;
	qp						= (g_vld_reg_ptr->MPEG4_CFG1 >> 24) & 0x1f;

	if (ac_pred_ena)
		printf ("");

	if (is_dc_coded_as_ac)
		printf ("");



	acdc_coeff_cur_ptr		= vsp_huff_dcac_tab + 160 + 16;

	//fetch top DC/AC coefficient into vsp buffer;	
	acdc_coeff_top_ptr += mb_x * 16;
	if (standard == VSP_MPEG4) 
	{
		VLD_FPRINTF(g_mpeg4dec_vld_trace_fp, "is_dc_coded_as_ac: %d\n", is_dc_coded_as_ac);	

		if (top_avail)
		{
			memcpy (acdc_coeff_cur_ptr, acdc_coeff_top_ptr, 16*sizeof(uint32));
		}
	}

	for (blk_cnt = 0; blk_cnt < 6; blk_cnt++)
	{
		VLD_FPRINTF (g_mpeg4dec_vld_trace_fp, "blk_cnt: %d\n", blk_cnt);
		VLD_FPRINTF (g_mpegdec_vld_no_acdc_fp, "blk_cnt: %d\n", blk_cnt);
		VLD_FPRINTF (g_pfRunLevel_mpeg4dec, "block: %d\n", blk_cnt);

		transpose_blkId = blk_cnt;
		if (rotation_ena)
		{
			if (blk_cnt == 1)
				transpose_blkId = 2;
			else if (blk_cnt == 2)
				transpose_blkId = 1;		
		}

#if defined(MPEG4_DEC)
// 		if ((blk_cnt == 2) && (mb_x == 10) && (g_dec_vop_mode_ptr->mb_y == 8) && (g_nFrame_dec == 7))
// 			printf ("");
#endif

		/*dc direction prediction if mpeg4*/
		if (standard == VSP_MPEG4)
		{			
			if (blk_cnt == 0)
			{
				tl_dc_reg = (uint16)g_vld_reg_ptr->MPEG4_TL_DC_Y;
			}
			else if (blk_cnt == 4)
			{
				tl_dc_reg = (uint16)(g_vld_reg_ptr->MPEG4_TL_DC_UV & 0xffff);
			}
			else if (blk_cnt == 5)
			{
				tl_dc_reg = (uint16)((g_vld_reg_ptr->MPEG4_TL_DC_UV >> 16) & 0xffff);
			}

			if ((blk_cnt == 1) || (blk_cnt == 3))
				tl_dc_reg = t_dc_reg;
			else if (blk_cnt == 2)
				tl_dc_reg = l1_dc_reg;	

			switch (blk_cnt)
			{
			case 0:
				left_coeff_offset = 0;
				top_coeff_offset = 16;
				break;
			case 1:
				left_coeff_offset = 0;
				top_coeff_offset = 16+4;
				break;
			case 2:
				left_coeff_offset = 4;
				top_coeff_offset = 16;
				break;
			case 3:
				left_coeff_offset = 4;
				top_coeff_offset = 16+4;
				break;
			case 4:
				left_coeff_offset = 8;
				top_coeff_offset = 16+8;
				break;
			default:
				left_coeff_offset = 12;
				top_coeff_offset = 16+12;
				break;
			}

			huff_tab_out_val = vsp_huff_dcac_tab[160+left_coeff_offset] & 0xffff;
			l_dc_reg = (((blk_cnt == 1) || (blk_cnt == 3)) || left_avail) ? huff_tab_out_val : DEFAULT_VALUE;

			/*update dc_register*/
			if (blk_cnt == 0)
				l1_dc_reg = l_dc_reg;

			huff_tab_out_val = vsp_huff_dcac_tab[160+top_coeff_offset] & 0xffff;
			t_dc_reg = (((blk_cnt == 2) || (blk_cnt == 3)) || top_avail) ? huff_tab_out_val : DEFAULT_VALUE;

			/*decide the direction*/
			grad_ver = (tl_dc_reg >= l_dc_reg) ? (tl_dc_reg - l_dc_reg) : (l_dc_reg - tl_dc_reg);
			grad_hor = (tl_dc_reg >= t_dc_reg) ? (tl_dc_reg - t_dc_reg) : (t_dc_reg - tl_dc_reg);
			pred_direct = (grad_hor > grad_ver) ? VLD_VERTICAL : VLD_HORIZONTAL;

			if (pred_direct == VLD_HORIZONTAL)
				VLD_FPRINTF (g_mpeg4dec_vld_trace_fp, "pred_direct: horizontal\n");
			else
				VLD_FPRINTF (g_mpeg4dec_vld_trace_fp, "pred_direct: vertical\n");

			if (blk_cnt == 0x3)
				PRINTF ("");
	
		}
		
		/*dc decoding*/
		flush_bits = 0;
		bsm_out = show_nbits (32);
	
		if (standard == VSP_MPEG4)
		{			
			int left_3_bits;
			
			if (!is_dc_coded_as_ac)
			{	
				/*decoding variable length code: dc_size*/
				if (!is_data_partition)
				{
					left_3_bits = bsm_out >> 29;
					
					if (left_3_bits > 0)
					{
						if (blk_cnt < 4)
						{
							if (left_3_bits == 3)
							{
								flush_bits = 3;
								dc_size = 0;
							}
							else if ((left_3_bits>>1) == 3)
							{
								flush_bits = 2;
								dc_size = 1;
							}
							else if ((left_3_bits>>1) == 2)
							{
								flush_bits = 2;
								dc_size = 2;
							}
							else if (left_3_bits == 2)
							{
								flush_bits = 3;
								dc_size = 3;
							}
							else 
							{
								flush_bits = 3;
								dc_size = 4;
							}
						}
						else
						{
							if ((left_3_bits >> 1) == 3)
							{
								flush_bits = 2;
								dc_size = 0;
							}
							else if ((left_3_bits >> 1) == 2)
							{
								flush_bits = 2;
								dc_size = 1;
							}
							else if ((left_3_bits >> 1) == 1)
							{
								flush_bits = 2;
								dc_size = 2;
							}
							else
							{
								flush_bits = 3;
								dc_size = 3;
							}
						}					
					}	
					else 
					{
						if (((bsm_out >> 28) & 1) == 1)
						{
							flush_bits = 4;
						}
						else if (((bsm_out >> 27) & 1) == 1)
						{
							flush_bits = 5;
						}
						else if (((bsm_out >> 26) & 1) == 1)
						{
							flush_bits = 6;
						}
						else if (((bsm_out >> 25) & 1) == 1)
						{
							flush_bits = 7;
						}
						else if (((bsm_out >> 24) & 1) == 1)
						{
							flush_bits = 8;
						}
						else if (((bsm_out >> 23) & 1) == 1)
						{
							flush_bits = 9;
						}
						else if (((bsm_out >> 22) & 1) == 1)
						{
							flush_bits = 10;
						}
						else if (((bsm_out >> 21) & 1) == 1)
						{
							flush_bits = 11;
						}
						else if (((bsm_out >> 20) & 1) == 1)
						{
							flush_bits = 12;
						}
						else
						{
							flush_bits = 0;
							vld_error = 1;						
						}
						
						dc_size = (blk_cnt < 4) ? flush_bits+1 : flush_bits;			
					}
					
					flush_nbits (flush_bits);
					
					
					/*decode delta_dc according to dc_size*/
					bsm_out = show_nbits (32);
					
					is_positive = bsm_out >> 31;
					
					switch (dc_size)
					{
					case 0:
						dc_abs = 0;
						dc_max = 0;
						break;
					case 1:
						dc_abs = bsm_out >> 31;
						dc_max = 1;
						break;
					case 2:
						dc_abs = bsm_out >> 30;
						dc_max = 3;
						break;
					case 3:
						dc_abs = bsm_out >> 29;
						dc_max = 7;
						break;
					case 4:
						dc_abs = bsm_out >> 28;
						dc_max = 0xf;
						break;
					case 5:
						dc_abs = bsm_out >> 27;
						dc_max = 0x1f;
						break;
					case 6:
						dc_abs = bsm_out >> 26;
						dc_max = 0x3f;
						break;
					case 7:
						dc_abs = bsm_out >> 25;
						dc_max = 0x7f;
						break;
					case 8:
						dc_abs = bsm_out >> 24;
						dc_max = 0xff;
						break;
					case 9:
						dc_abs = bsm_out >> 23;
						dc_max = 0x1ff;
						break;
					default:
						vld_error = 1;
						break;
					}	
					
					delta_dc = is_positive ? dc_abs : (dc_abs - dc_max);
					
					flush_bits = (dc_size > 8) ? (dc_size+1) : dc_size;
					
					flush_nbits (flush_bits);
				
					VLD_FPRINTF (g_pfRunLevel_mpeg4dec, "dc_size: %d, dc_diff: %d\n", dc_size, delta_dc);
					VLD_FPRINTF (g_mpeg4dec_vld_trace_fp, "dc_size: %d, dc_diff: %d\n", dc_size, delta_dc);
				}
				else   //data_partition
				{
					switch(blk_cnt)
					{
					case 0:
						delta_dc = g_vld_reg_ptr->MPEG4_DC_Y10 & 0xffff;
						break;
					case 1:
						delta_dc = (g_vld_reg_ptr->MPEG4_DC_Y10 >> 16) & 0xffff;
						break;
					case 2:
						delta_dc = g_vld_reg_ptr->MPEG4_DC_Y32 & 0xffff;
						break;
					case 3:
						delta_dc = (g_vld_reg_ptr->MPEG4_DC_Y32 >> 16) & 0xffff;
						break;
					case 4:
						delta_dc = g_vld_reg_ptr->MPEG4_DC_UV & 0xffff;
						break;
					case 5:
						delta_dc = (g_vld_reg_ptr->MPEG4_DC_UV >> 16) & 0xffff;
						break;
					default:
						break;
					}

					VLD_FPRINTF (g_mpeg4dec_vld_trace_fp, "data_partition: %d\n", delta_dc);
					VLD_FPRINTF (g_pfRunLevel_mpeg4dec, "dc_diff: %d\n", (int16)delta_dc);
				}
			}			
		}
		else
		{
			//h.263 dc decoding
			bsm_out = show_nbits (32);

			dc_value = bsm_out >> 24;

			vld_error = ((dc_value == 128) || (dc_value == 0)) ? 1 : 0;

			dc_value = (dc_value == 255) ? 128 : dc_value;

			VLD_FPRINTF (g_mpeg4dec_vld_trace_fp, "dc_coeff: %d\n", dc_value);
			VLD_FPRINTF (g_pfRunLevel_mpeg4dec, "dc_coeff: %d\n", dc_value);

			flush_nbits (8);
		}

		if(vld_error) //added by xiaowei, 20090724
		{
			g_vld_reg_ptr->VLD_CTL = V_BIT_30;
			return;
		}

		/*get zigzag_mode*/
		zigzag_mode = ((standard == VSP_MPEG4) && (ac_pred_ena)) ? ((pred_direct == VLD_HORIZONTAL) ? 2 : 1) : 0;	
		nz_flag_acdc = 0;
		
		start_position = (standard == ITU_H263) ? 1
						: ((!is_dc_coded_as_ac) ? 1 : 0);
		
		if ((1<<(5-blk_cnt)) & cbp)
		{
			nz_flag_acdc = mpeg4dec_vld (
								standard,
								is_rvlc,
								mb_type,
								0,							//cbp not used for intra
								start_position,
								zigzag_mode,
								blk_cnt,
								rotation_ena			
							);
		}
		/******************************************************************************************
		tbuf organization:
		1. 0x00~0x15f, used to store vld huffman table;
		2. 0xa0~0xaf, are used to store first column coeff; 
		3. 0xb0~0xbf, used to store first row coeff;
		******************************************************************************************/

		/*dc_ac_precdiction*/
		if (standard == VSP_MPEG4)
		{
			int i_coeff;
			int direct;				///1: HORIZONTAL   2: VERTICAL
			
			// dc/ac inverse prediction
			/*****************************************************************************************
			NOTE:
			dct_coeff in dct/io buffer is in asic order,

			process direction:
			Horizontal: first column coeff in normal order, if !rotation, it's first row in dct/io buffer,
						if rotation, it's first column in dct/io buffer; and coeff_pred and pred result are
						read from or written to left coeff buffer in tbuf, start address is 0xa0;

            Vertical: first row coeff in normal order, if !rotation, it's first column in dct/io buffer,
					   if rotation, it's first row in dct/io buffer; and thsese coeffs are stored into top 
					   coeff buffer in tbuf, start address is 0xb0	
				
			in left and top coeff, order is asic order [0,2,4,6,1,5,3,7], it will not affect the prediction result
			******************************************************************************************/
			
			for (direct = VLD_HORIZONTAL; direct <= VLD_VERTICAL; direct++)
			{
				int16	coeff_pred0;								//coeff_pred in [15~0]
				int16	coeff_pred1;								//coeff_pred in [31~15]
				int16	coeff_pred;									//wire 16 bits
				int32	two_coeff_pred;								//wire 32 bits
				int32	blk_offset;
				int16*  dct_io_ptr = (int16 *)vsp_dct_io_1;			//half word address, and allow half word access
				int     is_fst_row_dctio;								//coeff is in first row in dct/io buffer in asic order

				for (i_coeff = 0; i_coeff < 8; i_coeff++)
				{	
					int16	coeff_diff;									//12 bits register
					int		address_dctio;
					int		address_acdc;
					int		scaler;
					int16	coeff_pred_abs;
					int16   coeff_pred_q_abs;
					int16	coeff;
					int		pred_ena;									//current coefficient pred enable or not
					int		read_acdc_buffer_flag;						//flag to read out pred AC for adding	

					if ((direct == 1) && (i_coeff == 2))
						printf ("");
					
					/*read coeff from dct/io or delta_dc*/
					is_fst_row_dctio    = !rotation_ena ? (direct == VLD_HORIZONTAL) : (direct == VLD_VERTICAL); //(direct == HORIZONTAL) ? 1 : 0; 
					address_dctio	= transpose_blkId*64 + (is_fst_row_dctio ? i_coeff : i_coeff*8);  //  dct/io buffer transposed
					nz_flag			 = GegNZFlag (direct, nz_flag_acdc, is_fst_row_dctio, i_coeff);
					coeff_diff		 = GetDctCoeff (nz_flag, is_dc_coded_as_ac, (i_coeff == 0), delta_dc, address_dctio, direct);
					
					/*read pred from huff_acdc table, if the direction equal to pred_direction, and ac_pred enable*/					
					if (blk_cnt >= 4)
						blk_offset = (blk_cnt - 2) * 4;
					else
					{
						if (direct == VLD_HORIZONTAL)				//transposed
							blk_offset = (blk_cnt >> 1) * 4;
						else
							blk_offset = (blk_cnt &  1) * 4;
					}

					address_acdc = 160 + blk_offset + (i_coeff>>1) + ((direct == VLD_HORIZONTAL) ? 0 : 16);
					
					read_acdc_buffer_flag = ((i_coeff & 1) == 1) ? 0
						: (!ac_pred_ena) ? 0
						: (direct != pred_direct) ? 0 : 1;

					if (read_acdc_buffer_flag)
					{
						two_coeff_pred	= vsp_huff_dcac_tab[address_acdc];
						coeff_pred0		= two_coeff_pred & 0xffff;
						coeff_pred1		= (two_coeff_pred >> 16) & 0xffff;
					}
					
					/*get scaler*/
					scaler = (i_coeff == 0) ? GetDCScaler (qp, blk_cnt) : qp;	
				
					if (pred_direct == VLD_VERTICAL)
					{
						qp_pred = ((blk_cnt == 2) || (blk_cnt == 3)) ? qp : top_qp;
					}
					else
					{
						qp_pred = ((blk_cnt == 1) || (blk_cnt == 3)) ? qp : left_qp;
					}				

					pred_ena  = GetPredEna (i_coeff, ac_pred_ena, direct, pred_direct, blk_cnt, left_avail, top_avail);

					if (i_coeff == 0)
					{
						coeff_pred		= pred_ena ? ((pred_direct == VLD_HORIZONTAL) ? l_dc_reg : t_dc_reg) : 0;
					}
					else
					{
						coeff_pred		= pred_ena ? (((i_coeff & 1) == 1)   ? coeff_pred1 : coeff_pred0) : 0;
					}
	

					coeff_pred_abs		= (coeff_pred > 0) ? coeff_pred  : -coeff_pred;

					if (i_coeff != 0)
						coeff_pred_abs = coeff_pred_abs * qp_pred;
					
					coeff_pred_q_abs	= (scaler == 0) ? 0
						: (scaler == 1) ? coeff_pred_abs 
						: (scaler == 2) ? ((coeff_pred_abs + 1) >> 1)
						: (coeff_pred_abs + (scaler>>1)) * vld_GetDivInverse (scaler) >> 17;
			

					coeff = coeff_diff + ((coeff_pred > 0) ? coeff_pred_q_abs : -coeff_pred_q_abs);
				
					//store back to dct/io
					if (i_coeff == 0)
					{
						if (direct == VLD_HORIZONTAL)
							dct_io_ptr [address_dctio] = (coeff > 2047) ? 2047 : ((coeff<0) ? 0 : coeff);
					}
					else
					{
						if (direct == pred_direct)
							dct_io_ptr [address_dctio] = (coeff > 2047) ? 2047 : ((coeff < -2048) ? -2048 : coeff);
					}
					
					if ((i_coeff&1) == 0)
					{
						if ((blk_cnt >2) && (direct == 1) && (i_coeff == 0))
							printf ("");

						if (i_coeff == 0)
							coeff_pred0 = coeff * scaler;	
						else
							coeff_pred0 = coeff;

						if ((direct == 1) && (i_coeff == 0))
						{
							if (blk_cnt == 3)
							{
								g_vld_reg_ptr->MPEG4_TL_DC_Y = coeff_pred0;
							}
							else if (blk_cnt == 4)
							{
								g_vld_reg_ptr->MPEG4_TL_DC_UV = (g_vld_reg_ptr->MPEG4_TL_DC_UV & 0xffff0000) | (coeff_pred0 & 0xffff);
							}
							else if (blk_cnt == 5)
							{
								g_vld_reg_ptr->MPEG4_TL_DC_UV = (g_vld_reg_ptr->MPEG4_TL_DC_UV & 0x0000ffff) | ((coeff_pred0 & 0xffff) << 16);
							}
						}
					}
					else
					{
						coeff_pred1 = coeff;					
					}
					
					//store back to huff_acdc buffer
					if ((i_coeff & 1) == 1)						
					{
						vsp_huff_dcac_tab[address_acdc] = (coeff_pred1 << 16) | (coeff_pred0 & 0xffff);	
					}
				}
			}

			if(ac_pred_ena)
			{
				/*none-zero coefficient flag update*/
				if (((pred_direct == VLD_HORIZONTAL)&&(!rotation_ena)) || ((pred_direct == VLD_VERTICAL)&&(rotation_ena)))
				{
					nz_flag = vsp_dct_io_1 [192+transpose_blkId*4];
					nz_flag = nz_flag | 0xff;
					vsp_dct_io_1 [192+transpose_blkId*4] = nz_flag; 
				}else //(((pred_direct == VERTICAL)&&(!rotation_ena)) || ((pred_direct == HORIZONTAL)&&(rotation_ena)))
				{
					nz_flag = vsp_dct_io_1 [192+transpose_blkId*4];
					nz_flag = nz_flag | 0x0101;
					vsp_dct_io_1 [192+transpose_blkId*4] = nz_flag; 

					nz_flag = vsp_dct_io_1 [192+transpose_blkId*4+1];
					nz_flag = nz_flag | 0x0101;
					vsp_dct_io_1 [192+transpose_blkId*4+1] = nz_flag; 

					nz_flag = vsp_dct_io_1 [192+transpose_blkId*4+2];
					nz_flag = nz_flag | 0x0101;
					vsp_dct_io_1 [192+transpose_blkId*4+2] = nz_flag; 

					nz_flag = vsp_dct_io_1 [192+transpose_blkId*4+3];
					nz_flag = nz_flag | 0x0101;
					vsp_dct_io_1 [192+transpose_blkId*4+3] = nz_flag;
				}				
			}else
			{
				nz_flag = vsp_dct_io_1 [192+transpose_blkId*4];
				nz_flag = nz_flag | 1;
				vsp_dct_io_1 [192+transpose_blkId*4] = nz_flag; //set DC none-zero
			}
		}	
		else 
		{
			((int16 *)vsp_dct_io_1 ) [transpose_blkId*64] = dc_value;

			nz_flag = vsp_dct_io_1 [192+transpose_blkId*4];
			nz_flag = nz_flag | 1;
			vsp_dct_io_1 [192+transpose_blkId*4] = nz_flag; 			
		}

		FprintfOneBlock ((int16 *)(vsp_dct_io_1 + transpose_blkId*32), g_mpeg4dec_vld_trace_fp);
	}
	
	//output current MB's hor DC/AC coefficient into top DC/AC pred buffer
	if (standard == VSP_MPEG4)
	{
		memcpy (acdc_coeff_top_ptr, acdc_coeff_cur_ptr, 16*sizeof(uint32));
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

