/*hvld_test_vector.h*/
#ifndef _HVLD_TEST_VECTOR_H_
#define _HVLD_TEST_VECTOR_H_

#include <stdio.h>
#include "video_common.h"

extern FILE	*	g_hvld_cmd_fp;
extern FILE	*	g_coeff_token_fp;
extern FILE	*	g_coeff_fp;
extern FILE	*	g_total_zero_fp;
extern FILE	*	g_run_before_fp;
extern FILE	*	g_huff_tab_fp;
extern FILE	*	g_nnz_mb_fp;
extern FILE	*	g_dct_coeff_nzf;

void HvldTestVecInit ();

void BsmInitCmd ();

void BsmShowBitsCmd (int nbits, uint32 ret);

void BsmFlushBitsCmd (int nBits);

void PrintfCoeffToken (
					   int		nc, 
					   uint32	bsm_token_data, 
					   int		trailing_one, 
					   int		total_coeff,
					   int		code_len
					   );

void PrintfCoeff (int32 level);

void PrintfTotalZero (
					  int		blk_type, 
					  uint32	bsm_run_data, 
					  int		total_coeff, 
					  int		total_zeros, 
					  int		code_len
					  );

void PrintfRunBefore (
					  uint32	bsm_run_data, 
					  int		zeros_left, 
					  int		run_before, 
					  int		code_len
					  );

void PrintfDCTBuf ();

#endif