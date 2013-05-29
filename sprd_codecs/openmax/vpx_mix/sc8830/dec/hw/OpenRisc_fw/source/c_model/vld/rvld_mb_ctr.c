/*rvld_mb_ctr.c*/
#include "rvld_mode.h"
//include "rvdec_basic.h"
#include "rvld_global.h"
#include "common_global.h"
#include "buffer_global.h"
// #include "iqt_global.h"

/************************************************************************************
max register organization in huffman table
word0: bit31~bit24	bit23~bit17		bit16~bit11	  bit10~bit6	bit5~bit2	bit1~bit0
		max_len6	  max_len5		 max_len4	   max_len3		 max_len2	 max_len1 

word1:  bit29~bit19	 bit18~bit9	    bit8~bit0
	     max_len9	  max_len8		 max_len7 

word2:  bit24~bit12	    bit11~bit0
		  max_len11		 max_len10 

word3:  bit30~bit16	    bit15~bit0
		max_len13		 max_len14 

word4:  bit30~bit14	    bit13~bit0
		max_len15		 max_len12 
************************************************************************************/

void LoadMaxRegArray (
					  int	mb_type, 
					  int	dsc_type, 
					  int	blk_type,
					  int	cx,
					  int	dsc2x2_type,
					  int	is_intra_lev
					  )
{
	int		is_intra;
	int		is_intra16;
	int		is_inter16;
	int		cbp_type;

	int		is_chroma;
	int		svlc_mbtype;
	int		is_dc_y;
	
	int		is_8x8dsc = 0;
	int		max_reg_base; 
	uint32	tbuf_rdata;

	MAX_REG_ARR_T *	max_reg_arr_ptr;

	is_intra	= (mb_type == INTRA16X16) || (mb_type == INTRA4X4);
	is_intra16	= (mb_type == INTRA16X16);
	is_inter16	= (mb_type == INTER16X16);

	is_chroma   = (blk_type == CHROMA_AC_BLK) ? 1 : 0;

	is_dc_y		= (blk_type == LUMA_DC_BLK)   ? 1 : 0;
	
	cbp_type	= (is_intra16 || is_inter16) ? 2 : (is_intra ? 1 : 0);

	svlc_mbtype	= is_dc_y ? 3 :
				  (is_intra16 || is_inter16) ? 2 : 
				  is_intra ? 1 : 0;

	if (dsc_type == DSC_CBP)
	{
		max_reg_base =	(cbp_type == 2) ? INTRA_CBP1_MAX_REG :
						(cbp_type == 1) ? INTRA_CBP0_MAX_REG : INTER_CBP_MAX_REG;

		max_reg_arr_ptr = &g_rvld_mode_ptr->c84_reg_arr;
	}
	else if (dsc_type == DSC_8X8)
	{
		is_8x8dsc = 1;
		if (cbp_type == 2)
		{
			max_reg_base =	(cx == 0) ? INTRA_8X8DSC10_MAX_REG_BASE :
							(cx == 1) ? INTRA_8X8DSC11_MAX_REG_BASE :
							(cx == 2) ? INTRA_8X8DSC12_MAX_REG_BASE : INTRA_8X8DSC13_MAX_REG_BASE;				
		}
		else if (cbp_type == 1)
		{
			max_reg_base =	(cx == 0) ? INTRA_8X8DSC00_MAX_REG_BASE :
							(cx == 1) ? INTRA_8X8DSC01_MAX_REG_BASE :
							(cx == 2) ? INTRA_8X8DSC02_MAX_REG_BASE : INTRA_8X8DSC03_MAX_REG_BASE;
		}
		else
		{
			max_reg_base =	(cx == 0) ? INTER_8X8DSC0_MAX_REG_BASE :
							(cx == 1) ? INTER_8X8DSC1_MAX_REG_BASE :
							(cx == 2) ? INTER_8X8DSC2_MAX_REG_BASE : INTER_8X8DSC3_MAX_REG_BASE;
		}

		max_reg_arr_ptr = &g_rvld_mode_ptr->c84_reg_arr;
	}
	else if (dsc_type == DSC_4X4)
	{
		if (!is_chroma)
		{
			max_reg_base =	(svlc_mbtype == 3)	? INTRA_L4X4DSC2_MAX_REG_BASE :
							(svlc_mbtype == 2)	? INTRA_L4X4DSC1_MAX_REG_BASE :
							(svlc_mbtype == 1)	? INTRA_L4X4DSC0_MAX_REG_BASE : INTER_L4X4DSC_MAX_REG_BASE;
		}
		else
		{
			max_reg_base = is_intra ? INTRA_C4X4DSC_MAX_REG_BASE : INTER_C4X4DSC_MAX_REG_BASE;
		}

		max_reg_arr_ptr = &g_rvld_mode_ptr->c84_reg_arr;
	}
	else if (dsc_type == DSC_2X2)
	{
		if (!is_chroma)
		{
			if (svlc_mbtype != 0)
			{
				max_reg_base = (dsc2x2_type == 0) ? INTRA_L2X2DSC0_MAX_REG_BASE : INTRA_L2X2DSC1_MAX_REG_BASE;
			}
			else
			{
				max_reg_base = (dsc2x2_type == 0) ? INTER_L2X2DSC0_MAX_REG_BASE : INTER_L2X2DSC1_MAX_REG_BASE;
			}
		}
		else
		{
			if (svlc_mbtype != 0)
			{
				max_reg_base = (dsc2x2_type == 0) ? INTRA_C2X2DSC0_MAX_REG_BASE : INTRA_C2X2DSC1_MAX_REG_BASE;
			}
			else
			{
				max_reg_base = (dsc2x2_type == 0) ? INTER_C2X2DSC0_MAX_REG_BASE : INTER_C2X2DSC1_MAX_REG_BASE;
			}			
		}

		max_reg_arr_ptr = (dsc2x2_type == 0) ? &g_rvld_mode_ptr->dsc2x2_reg_arr0 : &g_rvld_mode_ptr->dsc2x2_reg_arr1;
	}
	else if (dsc_type == DSC_LEV)
	{
		max_reg_base = is_intra_lev ? INTRA_LEVDSC_MAX_REG_BASE : INTER_LEVDSC_MAX_REG_BASE;
		max_reg_arr_ptr = is_intra_lev ? &g_rvld_mode_ptr->intra_lev_reg_arr : &g_rvld_mode_ptr->inter_lev_reg_arr;
	}
	
	tbuf_rdata = g_rvld_huff_tab[max_reg_base];
	
	max_reg_arr_ptr->max_reg_len1 = (tbuf_rdata >> 0) & 0x3;		//2
	max_reg_arr_ptr->max_reg_len2 = (tbuf_rdata >> 2) & 0x7;		//3
	max_reg_arr_ptr->max_reg_len3 = (tbuf_rdata >> 5) & 0xf;		//4
	max_reg_arr_ptr->max_reg_len4 = (tbuf_rdata >> 9) & 0x1f;		//5
	max_reg_arr_ptr->max_reg_len5 = (tbuf_rdata >> 14) & 0x3f;		//6
	max_reg_arr_ptr->max_reg_len6 = (tbuf_rdata >> 20) & 0x7f;		//7
	
	tbuf_rdata = g_rvld_huff_tab[max_reg_base + 1];
	max_reg_arr_ptr->max_reg_len7 = (tbuf_rdata >> 0) & 0xff;		//8
	max_reg_arr_ptr->max_reg_len8 = (tbuf_rdata >> 8) & 0x1ff;		//9
	max_reg_arr_ptr->max_reg_len9 = (tbuf_rdata >> 17) & 0x3ff;		//10
	
	if (!is_8x8dsc)
	{
		tbuf_rdata = g_rvld_huff_tab[max_reg_base + 2];					
		max_reg_arr_ptr->max_reg_len10 = (tbuf_rdata >> 0) & 0x7ff;		//11
		max_reg_arr_ptr->max_reg_len11 = (tbuf_rdata >> 11) & 0xfff;	//12
		
		tbuf_rdata = g_rvld_huff_tab[max_reg_base + 3];
		max_reg_arr_ptr->max_reg_len12 = (tbuf_rdata >> 0) & 0x1fff;	//13
		max_reg_arr_ptr->max_reg_len13 = (tbuf_rdata >> 13) & 0x3fff;	//14
		
		tbuf_rdata = g_rvld_huff_tab[max_reg_base + 4];
		max_reg_arr_ptr->max_reg_len14 = (tbuf_rdata >> 0) & 0x7fff;	//15
		max_reg_arr_ptr->max_reg_len15 = (tbuf_rdata >> 15) & 0xffff;	//16
	}
	else
	{
		max_reg_arr_ptr->max_reg_len10 = 0;
		max_reg_arr_ptr->max_reg_len11 = 0;
		max_reg_arr_ptr->max_reg_len12 = 0;
		max_reg_arr_ptr->max_reg_len13 = 0;
		max_reg_arr_ptr->max_reg_len14 = 0;
		max_reg_arr_ptr->max_reg_len15 = 0;
	}

}

int g_vld_mb_cnt = 0;

void rvld_mb_ctr ()
{
	int		mb_type;
	int		is_intra;
	int		is_intra16;
	int		is_inter16;
	int		cbp_type;
	uint32	cbp;
	int		blk4x4_id;
	int		blk_store_idx;
	uint32	cbp_tmp0;
	uint32	cbp_tmp1;
	uint32	cbp_tmp2;

	if(g_vld_mb_cnt == 396)
	{
		printf ("");

	//	exit(-1);
	}
	
	g_vld_mb_cnt++;
	
	memset (vsp_dct_io_0, 0, 256*sizeof(uint32));

	mb_type		= g_rvld_reg_ptr->mb_info & 3;
	
	is_intra	= (mb_type == INTRA16X16) || (mb_type == INTRA4X4);
	is_intra16	= (mb_type == INTRA16X16);
	is_inter16	= (mb_type == INTER16X16);

	cbp_type	= (is_intra16 || is_inter16) ? 2 : (is_intra ? 1 : 0);

	cbp = RvldDecodeCBP (mb_type, cbp_type);
//	mb_cache_ptr->cbp = cbp;
	g_rvld_reg_ptr->cbp = cbp;
	cbp_tmp0 = cbp & 0xffffc3c3;
	cbp_tmp1 = cbp & 0x3030;
	cbp_tmp2 = cbp & 0xc0c;
	cbp = cbp_tmp0 | (cbp_tmp1 >> 2) | (cbp_tmp2 << 2);
	FPRINTF (g_rvld_trace_fp, "cbp: %08x\n", cbp);

// 	g_dct_reg_ptr->iict_cfg0 = cbp;

	if ((cbp == 0) && !(is_intra16 || is_inter16))
	{
		PrintfDCTBuf ();
		return;
	}	

	LoadMaxRegArray (mb_type, DSC_LEV, 0, 0, 0, 1);
	LoadMaxRegArray (mb_type, DSC_LEV, 0, 0, 0, 0);	

	if (is_intra16 || is_inter16 || is_intra)
	{
		LoadMaxRegArray (mb_type, DSC_2X2, LUMA_DC_BLK, 0, 0, 0);
		LoadMaxRegArray (mb_type, DSC_2X2, LUMA_DC_BLK, 0, 1, 0);
	}
	else if (cbp & 0xffff)
	{		
		LoadMaxRegArray (mb_type, DSC_2X2, LUMA_AC_BLK, 0, 0, 0);
		LoadMaxRegArray (mb_type, DSC_2X2, LUMA_AC_BLK, 0, 1, 0);
	}


	if (is_intra16 || is_inter16)
	{
		FPRINTF (g_rvld_trace_fp, "luma DC: \n");
		
		LoadMaxRegArray (mb_type, DSC_4X4, LUMA_DC_BLK, 0, 0, 0);
		
		RvldDecodeBlk4x4 (mb_type, LUMA_DC_BLK, 0);
	}

	/*decoder AC_Y*/
	FPRINTF (g_rvld_trace_fp, "luma AC: \n");

	if (cbp & 0xffff)   //for trace comparision
	{
		LoadMaxRegArray (mb_type, DSC_4X4, LUMA_AC_BLK, 0, 0, 0);

		for (blk4x4_id = 0; blk4x4_id < 16; blk4x4_id++)
		{
			if (blk4x4_id == 13)
				printf ("");
			
			/*change blk order from raster scan to block8x8 mode;*/
			//blk4x4_id = (blk4x4_id & 0x9) | ((blk4x4_id & 2) << 1) | ((blk4x4_id & 4) >> 1);
			blk_store_idx = (blk4x4_id & 0x9) | ((blk4x4_id & 2) << 1) | ((blk4x4_id & 4) >> 1);
			if (cbp & (1 << /*blk4x4_id*/blk_store_idx))
			{
				FPRINTF (g_rvld_trace_fp, "blk4x4_id: %d\n", blk_store_idx/*blk4x4_id*/);
				//RvldDecodeBlk4x4 (mb_type, LUMA_AC_BLK, blk_store_idx);
				RvldDecodeBlk4x4 (mb_type, LUMA_AC_BLK, blk_store_idx/*blk4x4_id*/);
			}
		}
	}
	
	FPRINTF (g_rvld_trace_fp, "chroma: \n");
	/**/  //for trace comparision
//	if ((cbp >> 16) == 0)
//	{
//		iqt_config();
//		return;
//	}

	LoadMaxRegArray (mb_type, DSC_4X4, CHROMA_AC_BLK, 0, 0, 0);
	
	LoadMaxRegArray (mb_type, DSC_2X2, CHROMA_AC_BLK, 0, 0, 0);
	LoadMaxRegArray (mb_type, DSC_2X2, CHROMA_AC_BLK, 0, 1, 0);

	/*chroma vld*/
	cbp = cbp >> 16;
	for (blk4x4_id = 0; blk4x4_id < 8; blk4x4_id++)
	{
		if (blk4x4_id == 1)
			printf ("");
		
		if (cbp & (1 << blk4x4_id))
		{
			FPRINTF (g_rvld_trace_fp, "blk4x4_id: %d\n", blk4x4_id);
			RvldDecodeBlk4x4 (mb_type, CHROMA_AC_BLK, blk4x4_id);
		}
	}	
//for iqt test
//		iqt_config();

	PrintfDCTBuf ();

}

