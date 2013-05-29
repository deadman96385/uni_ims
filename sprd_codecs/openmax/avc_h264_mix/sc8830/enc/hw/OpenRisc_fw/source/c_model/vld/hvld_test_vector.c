/*hvld_test_vector.c*/

#include <stdio.h>
#include "hvld_test_vector.h"
#include "hvld_global.h"
#include "buffer_global.h"

FILE	*	g_hvld_cmd_fp;
FILE	*	g_coeff_token_fp;
FILE	*	g_coeff_fp;
FILE	*	g_total_zero_fp;
FILE	*	g_run_before_fp;
FILE	*	g_nnz_mb_fp;
FILE	*	g_bitstream_fp;
FILE	*	g_dct_coeff_nzf;

extern FILE * g_fp_vld_tv;

int			g_cmd_cnt = 0;

void HvldTestVecInit ()
{
if(g_vector_enable_flag&VECTOR_ENABLE_VLD)
{
	g_hvld_cmd_fp = fopen ("..\\..\\test_vectors\\vld\\hvld_tv_cmd.dat", "w");
	assert (g_hvld_cmd_fp != NULL);
	
	g_coeff_token_fp = fopen ("..\\..\\test_vectors\\vld\\coeff_token.txt", "w");
	assert (g_coeff_token_fp != NULL);

	g_coeff_fp = fopen ("..\\..\\test_vectors\\vld\\coeff.txt", "w");
	assert (g_coeff_fp != NULL);

	g_total_zero_fp = fopen ("..\\..\\test_vectors\\vld\\total_zero.txt", "w");
	assert (g_total_zero_fp != NULL);

	g_run_before_fp = fopen ("..\\..\\test_vectors\\vld\\run_before.txt", "w");
	assert (g_run_before_fp != NULL);

	g_nnz_mb_fp = fopen ("..\\..\\test_vectors\\vld\\nnz_mb.txt", "w");
	assert (g_nnz_mb_fp != NULL);

	g_bitstream_fp = fopen ("..\\..\\test_vectors\\vld\\bitstream.txt", "w");
	assert (g_bitstream_fp != NULL);

	g_dct_coeff_nzf = fopen ("..\\..\\test_vectors\\vld\\coeff_nzf.txt", "w");
	assert (g_dct_coeff_nzf != NULL);
}
}

void FwCmdInc (int number)
{
	if (g_cmd_cnt == 99)
		printf ("");

	g_cmd_cnt += number;
}

void BsmInitCmd ()
{
if(g_vector_enable_flag&VECTOR_ENABLE_VLD)
{
	static int s_firstBsm_init = 0;

	if(!s_firstBsm_init)
	{	
		FPRINTF (g_hvld_cmd_fp, "1, 20210408, %08x\n", (1<<2) | (1<<1));	//BSM_CFG2: clear bsm and clear counter

		FwCmdInc (1);
	}
	s_firstBsm_init = FALSE;

	FPRINTF (g_hvld_cmd_fp, "1, 20210404, %08x\n", (0>>2));	//configure the bitstream address

	FPRINTF (g_hvld_cmd_fp, "1, 20210400, %08x\n", ((1<<31) | 0xfffff));	//BSM_CFG0: init bsm register: buffer 0 ready and the max buffer size	

	FPRINTF (g_hvld_cmd_fp, "2, 20210418, 0, %08x, %08x\n", (1<<31), (1<<31));		//BSM_DEBUG: polling bsm status

	FwCmdInc (3);
}
}

void BsmShowBitsCmd (int nbits, uint32 ret)
{
if(g_vector_enable_flag&VECTOR_ENABLE_VLD)
{
	FPRINTF (g_hvld_cmd_fp, "1, 20210408, %08x\n", (nbits << 24) | (0<<0));

	FwCmdInc (1);

	FPRINTF (g_hvld_cmd_fp, "0, 20210410, %08x\n", ret);

	FwCmdInc (1);
}
}


void BsmHfullPolling ()
{
if(g_vector_enable_flag&VECTOR_ENABLE_VLD)
{
	FPRINTF (g_hvld_cmd_fp, "2, 20210418, 0, %08x, %08x\n", (1<<3), (1<<3));

	FwCmdInc (1);
}
}


void BsmFlushBitsCmd (int nbits)
{
if(g_vector_enable_flag&VECTOR_ENABLE_VLD)
{	
	BsmHfullPolling ();

	FPRINTF (g_hvld_cmd_fp, "1, 20210408, %08x\n", (nbits << 24) | (1<<0));

	FwCmdInc (1);
}
}

void PrintfCoeffToken (
					   int		nc, 
					   uint32	bsm_token_data, 
					   int		trailing_one, 
					   int		total_coeff,
					   int		code_len
					   )
{
if(g_vector_enable_flag&VECTOR_ENABLE_VLD)
{
	if (nc == -1)
		nc = 31;

	FPRINTF (g_coeff_token_fp, "%08x, %08x, %08x, %08x, %08x\n", 
		nc, bsm_token_data, trailing_one, total_coeff, code_len);
}
}

void PrintfCoeff (int32 level)
{
if(g_vector_enable_flag&VECTOR_ENABLE_VLD)
{
	static int level_cnt = 0;

	FPRINTF (g_coeff_fp, "%08x\n", level);

	level_cnt++;
}
}

void PrintfTotalZero (
					  int		blk_type, 
					  uint32	bsm_run_data, 
					  int		total_coeff, 
					  int		total_zeros, 
					  int		code_len
					  )
{
if(g_vector_enable_flag&VECTOR_ENABLE_VLD)
{
	FPRINTF (g_total_zero_fp, "%08x, %08x, %08x, %08x, %08x\n",
		(blk_type == 2), bsm_run_data, total_coeff, total_zeros, code_len);
}
}

void PrintfRunBefore (
					  uint32	bsm_run_data, 
					  int		zeros_left, 
					  int		run_before, 
					  int		code_len
					  )
{
if(g_vector_enable_flag&VECTOR_ENABLE_VLD)
{
//	FPRINTF (g_run_before_fp, "%08x, %08x, %08x, %08x\n", 
//		bsm_run_data, zeros_left, run_before, code_len);

	FPRINTF (g_run_before_fp, "%08x\n", run_before);
}
}

void PrintfDCTBuf ()
{
if(g_vector_enable_flag&VECTOR_ENABLE_VLD)
{
	int		i;
	int32	val;

	for (i = 0; i < 256; i++)
	{
		val = vsp_dct_io_0[i];

		FPRINTF (g_fp_vld_tv, "%08x\n", val);
	}
}
}

