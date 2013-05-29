/******************************************************************************
 ** File Name:    vlc_mpeg4_rlc.c    										  *
 ** Author:       Xiaowei Luo                                                 *
 ** DATE:         12/14/2006                                                  *
 ** Copyright:    2006 Spreatrum, Incoporated. All Rights Reserved.           *
 ** Description:  vlc coding for one event, intra/inter coefficient and       *
 **	              last/non-last coeff for h.263 and mpeg4                     *
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

/**----------------------------------------------------------------------------*
**                           Function Prototype                               **
**----------------------------------------------------------------------------*/

char LmaxTabIntra [40] =
{
	27, 10, 5, 4, 3, 3, 3, 3,    //last = 0
		2,  2, 1, 1, 1, 1, 1, 
		0, //stuffing to 16
		
		8, 3, 2, 2, 2, 2, 2,		//last = 1
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
		0, 0, 0   //stuffing to 24
};

char RmaxTabIntra [40] =
{
	0, 14, 9, 7, 3, 2, 1, 1, 1, 1, 1,     //last = 0
		0, 0, 0, 0, 0, 
		0, 0, 0, 0, 0, 
		0, 0, 0, 0, 0, 0, 0,     //to 28   
		
		
		0, 20, 6, 1, 0, 0, 0, 0, 0,      //last = 1
		0, 0, 0      //stuffing
};

char LmaxTabInter [72] = 
{
	12, 6, 4, 3, 3, 3, 3, 2, 2, 2, 2,		//last = 0
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 
		0,     //stuffing to 28
		
		3, 2, 1, 1, 1, 1, 1, 1, 			//last = 1
		1, 1, 1, 1, 1, 1, 1, 1,
		1, 1, 1, 1, 1, 1, 1, 1,
		1, 1, 1, 1, 1, 1, 1, 1,
		1, 1, 1, 1, 1, 1, 1, 1, 1, 
		0, 0, 0,    //stuffing to 44
};

char RmaxTabInter [20] = 
{
	0, 26, 10, 6, 2, 1, 1, 0, 0, 0, 0, 0, 0,	//last = 0
		0, 0, 0, //stuffing to 16
		
		0, 40, 1, 0,							//last = 1
		
};

void IntraHuffEnc (
				   int		run, 
				   int		level, 
				   int		last, 
				   int	*   offset_ptr
				   )
{
	int base_addr;
	int is_run = 0;

	if (last == 0)
	{
		if (run == 0)
		{
			base_addr = VLC_INTRA_OFFSET_L0R0;
			is_run = 1;
		}
		else if (level == 1)
		{
			base_addr = VLC_INTRA_OFFSET_L0L1;
		}
		else if (level == 2)
		{
			base_addr = VLC_INTRA_OFFSET_L0L2;				
		}
		else if (level == 3)
		{
			base_addr = VLC_INTRA_OFFSET_L0L3;
		}
		else if (run == 1)
		{
			base_addr = VLC_INTRA_OFFSET_L0R1;
			is_run = 1;
		}
		else if(run == 2)
		{
			base_addr = VLC_INTRA_OFFSET_L0R2;
			is_run = 1;
		}
		else if (run == 3)
		{
			base_addr = VLC_INTRA_OFFSET_L0R3;
			is_run = 1;
		}
	}
	else
	{	
		if (run == 0)
		{ 
			base_addr = VLC_INTRA_OFFSET_L1R0;
			is_run = 1;
		}
		else if (level == 1)
		{
			base_addr = VLC_INTRA_OFFSET_L1L1;
		}
		else if (level == 2)
		{
			base_addr = VLC_INTRA_OFFSET_L1L2;
		}
		else if (level == 3)
		{
			base_addr = VLC_INTRA_OFFSET_L1L3;
		}
	}

	*offset_ptr = base_addr + (is_run ? level : run);
}

void InterHuffEnc (
				   int		run, 
				   int		level, 
				   int		last, 
				   int	*   offset_ptr
				   )
{
	int base_addr;
	int is_run = 0;

	base_addr = VLC_INTER_OFFSET_L0R0;
	
	if (last == 0)
	{
		if (run == 0)
		{
			base_addr = VLC_INTER_OFFSET_L0R0;
			is_run = 1;
		}	
		else if(level == 1)
		{
			base_addr = VLC_INTER_OFFSET_L0L1;
		}
		else if (run == 1)
		{
			base_addr = VLC_INTER_OFFSET_L0R1;
			is_run = 1;
		}
		else if (level == 2)
		{
			base_addr = VLC_INTER_OFFSET_L0L2;
		}
		else if (level == 3)
		{
			base_addr = VLC_INTER_OFFSET_L0L3;
		}
		else if (run == 2)
		{
			base_addr = VLC_INTER_OFFSET_L0R2;
			is_run = 1;
		}
	}
	else
	{		
		if (level == 1)
		{
			base_addr = VLC_INTER_OFFSET_L1L1;
		}
		else if(run == 0)
		{
			base_addr = VLC_INTER_OFFSET_L1R0;
			is_run = 1;
		}		
		else if (run == 1)
		{
			base_addr = VLC_INTER_OFFSET_L1R1;
			is_run = 1;
		}
	}

	*offset_ptr = base_addr + (is_run ? level : run);
}

int coeff_num = 0;

void mpeg4_vlc (
				int		is_short_header, 
				int		is_intra, 
				int		run, 
				int16	level, 
				int		last
				)
{
	int		lmax;
	int		rmax;
	int		sign;
	int		code_word;
	int		code_len;

	int		vlc_tbuf_addr;
	uint32  vlc_tbuf_rdata;

	uint32	vlc_bsm_wdata;
	int		vlc_bsm_length;

	uint16	val;

	int escape_mode;		//0: normal, 1: escape mode 1,  2: escape mode 2,  3: escape mode 3 (fix length coding)


	is_intra = is_short_header ? 0 : is_intra;

	if (coeff_num == 2834)
		printf ("");

	escape_mode = 0;

	if (level > 0)
	{
		sign = 0;		
	}
	else
	{
		sign = 1;
		level = -level;
	}

	/*judge whether escape, and get the escape mode if be escape*/
	{
		int		l_esc_mode;
		int		r_esc_mode;
		int		mpeg4_esc_mode;
		
		/*L escape mode*/
		l_esc_mode = 0;

		if (is_intra)
		{
			if (run <= (last ? 20 : 14))
			{
				lmax = LmaxTabIntra [run+last*16];	
				
				if (level > lmax)
				{				
					if (level <= (lmax*2))
						l_esc_mode = 1;
					else
						l_esc_mode = 3;
				}		
			}
			else
			{
				l_esc_mode = 3;
			}
		}
		else
		{
			if (run <= (last ? 40 : 26))
			{
				lmax = LmaxTabInter [run+last*28];	
				
				if (level > lmax)
				{
					if (level <= (lmax*2))
						l_esc_mode = 1;
					else
						l_esc_mode = 3;
				}
			}
			else
			{
				l_esc_mode = 3;
			}
		}	
		
		
		/*R escape mode*/
		r_esc_mode = 0;
		if (is_intra)
		{
			if (level <= (last ? 8 : 27))
			{
				rmax = RmaxTabIntra [level+last*28];
				
				if (run > rmax)
				{
					if (run <= rmax*2+1)
						r_esc_mode = 2;
					else
						r_esc_mode = 3;
				}		
			}
			else
			{
				r_esc_mode = 3;
			}
		}
		else
		{
			if (level <= (last ? 3 : 12))
			{
				rmax = RmaxTabInter [level+last*16];
				
				if (run > rmax)
				{
					if (run <= rmax*2+1)
						r_esc_mode = 2;
					else
						r_esc_mode = 3;
				}			
			}	
			else
			{
				r_esc_mode = 3;
			}			
		}	
		
		mpeg4_esc_mode = (l_esc_mode == 0) ? 0 :
						 (l_esc_mode == 1) ? 1 :
						 (r_esc_mode == 2) ? 2 :
						 3;

		if (is_short_header)
			escape_mode = (mpeg4_esc_mode > 0) ? 3 : 0;
		else
			escape_mode = mpeg4_esc_mode;
	}

	FPRINTF (g_fp_trace_vlc, "bEscape: %d, escapeMode: %d, ", escape_mode > 0, escape_mode);

	/*if be escape mode 3*/
	if (escape_mode == 3)
	{
		/*fix length coding*/
		if (sign)
		{
			level = (level^0xfff)+1;
		}
		
		if (!is_short_header)
		{
			vlc_bsm_wdata	= (0xf << 21) | (last << 20) | (run << 14) | (1 << 13) | (level << 1) | 1;
			vlc_bsm_length	= 30;
		}
		else
		{
			vlc_bsm_wdata = (3 << 15) | (last << 14) | (run << 8) | (level&0xff);
			vlc_bsm_length = 22;
		}
	}
	else
	{
		if (escape_mode == 1)
		{
			level = level - lmax;
		}
		
		if (escape_mode == 2)
		{
			run = run - (rmax+1);
		}

		/*huffman encoding, get codeword and codelength*/
		vlc_tbuf_addr = 0;
		if (is_intra)
			IntraHuffEnc (run, level, last, &vlc_tbuf_addr);
		else
			InterHuffEnc (run, level, last, &vlc_tbuf_addr);

		vlc_tbuf_rdata = vsp_huff_dcac_tab [vlc_tbuf_addr];

		val = (uint16)(is_intra ? (vlc_tbuf_rdata >> 16) : (vlc_tbuf_rdata & 0xffff));

		//note codeword is left align in huffman table
		code_len  = val & 0xf;
		code_word = val >> (16 - code_len);   //right alignment

		if (escape_mode == 1)
		{			
			vlc_bsm_length = code_len + 8;
			vlc_bsm_wdata = (6 << code_len) | code_word;
		}
		else if (escape_mode == 2)
		{
			vlc_bsm_wdata = (0xe << code_len) | code_word;
			vlc_bsm_length = code_len + 9;
		}
		else
		{
			vlc_bsm_length = code_len;
			vlc_bsm_wdata  = code_word;
		}

//		PrintVlcEvent (escape_mode, lmax, rmax, vlc_tbuf_addr);

		vlc_bsm_wdata = ((vlc_bsm_wdata >> 1) << 1) | sign;
	}

	PrintVlcEvent (escape_mode, lmax, rmax, vlc_tbuf_addr);

	vlc_bsm_length = vlc_bsm_length & 0x1f;
	
	write_nbits (vlc_bsm_wdata, vlc_bsm_length, 1);
	if(or1200_print)
	{
		OR1200_READ_REG_POLL(BSM_CTRL_REG_BASE_ADDR + 0x20, V_BIT_0, 0x00000001, "ORSC: Polling BSM_RDY");
		OR1200_WRITE_REG(BSM_CTRL_REG_BASE_ADDR + 0x08, (vlc_bsm_length&0x3f) << 24, "ORSC: BSM_OPERATE: Set OPT_BITS");
		OR1200_WRITE_REG(BSM_CTRL_REG_BASE_ADDR + 0x0C, vlc_bsm_wdata, "ORSC: BSM_WDATA");
	}
	FPRINTF (g_fp_trace_vlc, "codeWord: %0x, length: %d\n", vlc_bsm_wdata, vlc_bsm_length);

	coeff_num++;
}

int vlc_GetDivInverse (int dc_scaler)
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

void mpeg4_dc_enc (
				   int16	dc_diff_q, 
				   int		blk_cnt, 
				   int *    vlc_bsm_wdata_ptr, 
				   int *	vlc_bsm_length_ptr
				   )
{
	uint16	dc_diff_abs;
	int		dc_size;
	int		dc_size_huf_len;
	int		dc_size_huf_code;
	uint16	level;
	uint16	val;
	
	dc_diff_abs = (dc_diff_q < 0) ? -dc_diff_q : dc_diff_q;
	
	/*decide size of dc coefficient*/
	if ((dc_diff_abs>>8)&1)
	{
		dc_size = 9;
	}
	else if ((dc_diff_abs>>7)&1)
	{
		dc_size = 8;
	}
	else if ((dc_diff_abs>>6)&1)
	{
		dc_size = 7;
	}
	else if ((dc_diff_abs>>5)&1)
	{
		dc_size = 6;
	}
	else if ((dc_diff_abs>>4)&1)
	{
		dc_size = 5;
	}
	else if ((dc_diff_abs>>3)&1)
	{
		dc_size = 4;
	}
	else if ((dc_diff_abs>>2)&1)
	{
		dc_size = 3;
	}
	else if ((dc_diff_abs>>1)&1)
	{
		dc_size = 2;
	}
	else if ((dc_diff_abs>>0)&1)
	{
		dc_size = 1;
	}
	else
	{
		dc_size = 0;
	}
	
	/*huffman encoding dc_size*/
	if (blk_cnt < 4)
	{
		if (dc_size < 4)
		{
			if (dc_size == 0)
			{
				dc_size_huf_len = 3;
				dc_size_huf_code = 3;
			}
			else if (dc_size == 1)
			{
				dc_size_huf_len = 2;
				dc_size_huf_code = 3; 
			}
			else if (dc_size == 2)
			{
				dc_size_huf_len = 2;
				dc_size_huf_code = 2;
			}
			else 
			{
				dc_size_huf_len = 3;
				dc_size_huf_code = 2;
			}
		}
		else
		{
			dc_size_huf_len  = dc_size - 1;
			dc_size_huf_code = 1;
		}
	}
	else
	{
		if (dc_size < 2)
		{
			if (dc_size == 0)
			{
				dc_size_huf_len = 2;
				dc_size_huf_code = 3;
			}
			else if (dc_size == 1)
			{
				dc_size_huf_len = 2;
				dc_size_huf_code = 2;
			}
		}
		else
		{
			dc_size_huf_len = dc_size;
			dc_size_huf_code = 1;
		}
	}
	
	/*two cycle to output */
	write_nbits (dc_size_huf_code, dc_size_huf_len, 1);	
	if(or1200_print)
	{
		OR1200_READ_REG_POLL(BSM_CTRL_REG_BASE_ADDR + 0x20, V_BIT_0, 0x00000001, "ORSC: Polling BSM_RDY");
		OR1200_WRITE_REG(BSM_CTRL_REG_BASE_ADDR + 0x08, (dc_size_huf_len&0x3f) << 24, "ORSC: BSM_OPERATE: Set OPT_BITS");
		OR1200_WRITE_REG(BSM_CTRL_REG_BASE_ADDR + 0x0C, dc_size_huf_code, "ORSC: BSM_WDATA");
	}
	FPRINTF (g_fp_trace_vlc, "dc_diff: %d, dc_huff_code: %0x, dc_huff_len: %d\n", dc_diff_q, dc_size_huf_code, dc_size_huf_len);

	PrintDCEnc (dc_size_huf_code, dc_size_huf_len);

	level = (dc_diff_q > 0) ? dc_diff_q : (~dc_diff_abs);
	
	if (dc_size > 0)
	{
		val = (level & ((1<<dc_size) - 1));

		if (dc_size > 8)
		{
			dc_size += 1;
			val = (val << 1) | 1;
		}

		write_nbits ((level & ((1<<dc_size) - 1)), dc_size, 1);
		if(or1200_print)
		{
			OR1200_READ_REG_POLL(BSM_CTRL_REG_BASE_ADDR + 0x20, V_BIT_0, 0x00000001, "ORSC: Polling BSM_RDY");
			OR1200_WRITE_REG(BSM_CTRL_REG_BASE_ADDR + 0x08, (dc_size&0x3f) << 24, "ORSC: BSM_OPERATE: Set OPT_BITS");
			OR1200_WRITE_REG(BSM_CTRL_REG_BASE_ADDR + 0x0C, (level & ((1<<dc_size) - 1)), "ORSC: BSM_WDATA");
		}
		FPRINTF (g_fp_trace_vlc, "dc_fixLen_code: %0x, dc_size: %d\n", 
			(level & ((1<<dc_size) - 1)), dc_size);
	}
}

extern int coeff_num;

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End 




















































