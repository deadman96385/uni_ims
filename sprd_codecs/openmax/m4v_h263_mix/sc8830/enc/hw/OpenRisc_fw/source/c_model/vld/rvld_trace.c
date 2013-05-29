/*vld_trace.c*/
#include <stdio.h>
#include "rvld_mode.h"
#include "rvld_global.h"
#include "buffer_global.h"

FILE * g_rvld_trace_fp;
FILE * g_cbp_val_fp;
FILE * g_cache_sta_fp;
FILE * g_huff_dec_pf;

#if defined(REAL_DEC)
//FILE * g_cbp_val_fp;
//FILE * g_huff_dec_pf;
FILE * g_mb_coeff_fp;

FILE * g_huff_tab_fp;
FILE * g_huff_sdram_fp;
FILE * g_bitstream_fp;

//FILE * g_cache_sta_fp;

FILE * g_acc_stat_fp;

FILE * g_rvld_tv_cmd_fp;
// int	   g_cmd_cnt = 0;


/*generate test vector*/

void TestVecInit ()
{
	g_huff_sdram_fp	= fopen ("..\\..\\test_vectors\\vld\\huff_tab_sdram.txt", "w");
	g_huff_tab_fp	= fopen ("..\\..\\test_vectors\\vld\\huff_tab.txt", "w");

	g_cbp_val_fp	= fopen ("..\\..\\test_vectors\\vld\\cbp.txt", "w");
	g_huff_dec_pf	= fopen ("..\\..\\test_vectors\\vld\\huff_code.txt", "w");

	g_mb_coeff_fp	= fopen ("..\\..\\test_vectors\\vld\\mb_coeff.txt", "w");

	g_rvld_tv_cmd_fp= fopen ("..\\..\\test_vectors\\vld\\rvld_tv_cmd.dat", "w");

	g_acc_stat_fp	= fopen ("..\\..\\test_vectors\\vld\\dsc4_distribute.txt", "w");

	g_cache_sta_fp  = fopen ("..\\..\\test_vectors\\vld\\cache_sta.txt", "w");
}

void PrintfMBCoeff ()
{
	int			i;
	uint32	*	dbuf_ptr;

	dbuf_ptr = vsp_dct_io_0;

	for (i = 0; i < 256; i++)
	{
		FPRINTF (g_mb_coeff_fp, "%08x\n", *dbuf_ptr++);
	}
}

void PrintfHuffTabSdram (int qp)
{
	int			i;
	uint32	*	tab_ptr;

	int			intra_qp_idx;
	int			inter_qp_idx;

	intra_qp_idx = g_intra_qp_to_idx[qp];

	tab_ptr = g_rvld_intra_code[intra_qp_idx];

	for (i = 0; i < MAX_CBP+432*4; i++)
	{
		FPRINTF (g_huff_sdram_fp, "%08x\n", tab_ptr[i]);
	}

	inter_qp_idx = g_inter_qp_to_idx[qp];

	tab_ptr = g_rvld_inter_code[inter_qp_idx];

	for (i = 0; i < MAX_CBP/2 + 432*2; i++)
	{
		FPRINTF (g_huff_sdram_fp, "%08x\n", tab_ptr[i]);
	}

	/*
	for (qp_idx = 0; qp_idx < MAX_INTRA_QP_REGIONS; qp_idx++)
	{
		tab_ptr = g_rvld_intra_code[qp_idx];

		for (i = 0; i < INTRA_CODE_SIZE; i++)
		{
			fprintf (g_huff_sdram_fp, "%08x\n", tab_ptr[i]);
		}

		tab_ptr = g_rvld_intra_max_base[qp_idx];

		for (i = 0; i < INTRA_MAX_BASE_SIZE; i++)
		{
			fprintf (g_huff_sdram_fp, "%08x\n", tab_ptr[i]);
		}		
	}

	for (qp_idx = 0; qp_idx < MAX_INTER_QP_REGIONS; qp_idx++)
	{
		tab_ptr = g_rvld_inter_code[qp_idx];
		
		for (i = 0; i < INTER_CODE_SIZE; i++)
		{
			fprintf (g_huff_sdram_fp, "%08x\n", tab_ptr[i]);
		}
		
		tab_ptr = g_rvld_inter_max_base[qp_idx];
		
		for (i = 0; i < INTER_MAX_BASE_SIZE; i++)
		{
			fprintf (g_huff_sdram_fp, "%08x\n", tab_ptr[i]);
		}		
	}
	*/
}

void rvld_PrintfHuffTab ()
{
	int i;

	for (i = 0; i < HUFF_TAB_SIZE+CACHE_TAG_SIZE; i++)
	{
		FPRINTF (g_huff_tab_fp, "%08x\n", g_rvld_huff_tab[i]);
	}
}

void PrintfCmd (int cmd_type, int addr, int val, int mask)
{
	if ((cmd_type == 0) || (cmd_type == 1))
	{
		FPRINTF(g_rvld_tv_cmd_fp, "%x,%08x,%08x\n", cmd_type, addr, val);
	}
	else
	{
		FPRINTF(g_rvld_tv_cmd_fp, "%x,%08x,%08x,%08x,%08x\n", cmd_type, addr, 0, mask, val);		
	}

// 	g_cmd_cnt++;

//	if (g_cmd_cnt == 0x2b)
//		printf ("");
}

#endif
