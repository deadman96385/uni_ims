/*hdbk_test_vector.h*/

#ifndef _HDBK_TEST_VECTOR_H_
#define _HDBK_TEST_VECTOR_H_


extern FILE	*	g_hdbk_cmd_fp;
extern FILE	*	g_mb_bf_filter_fp;		
extern FILE	*	g_mb_af_filter_fp;		
extern FILE	*	g_filter_upt_fp_fp;	

void HdbkTestVecInit ();

void PrintMBBfFilter ();

void PrintMBAfFilter (int mb_x);

void PrintfFilterUpt (
					  int		mb_cnt,
					  int		blk_id,
					  int		line_id,
					  int		upt_flag,
					  uint32	p_upt,
					  uint32	q_upt			
					  );


#endif