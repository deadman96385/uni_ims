/*hdbk_test_vector.c*/
#include "sc8810_video_header.h"

FILE	*	g_hdbk_cmd_fp;
FILE	*	g_mb_bf_filter_fp;			//only left part and current MB
FILE	*	g_mb_af_filter_fp;			//including top and left and current MB
FILE	*	g_filter_upt_fp_fp;			//filtered pixel, which pixels updtaed, block number and line_id


void HdbkTestVecInit ()
{
if(g_vector_enable_flag&VECTOR_ENABLE_DBK)	
{
/*	g_hdbk_cmd_fp = fopen ("..\\..\\test_vectors\\dbk\\hdbk_tv_cmd.dat", "w");
	assert (g_hdbk_cmd_fp != NULL);

	g_mb_bf_filter_fp = fopen ("..\\..\\test_vectors\\dbk\\mb_bf_filter.txt", "w");
	assert (g_mb_bf_filter_fp != NULL);

	g_mb_af_filter_fp = fopen ("..\\..\\test_vectors\\dbk\\mb_af_filter.txt", "w");
	assert (g_mb_af_filter_fp != NULL);

	g_filter_upt_fp_fp = fopen ("..\\..\\test_vectors\\dbk\\filter_upt.txt", "w");
	assert (g_filter_upt_fp_fp != NULL);*///weihu
}
}

/**********************************************************************
   these pixels should be filled into dbk_out_buf before starting filter
   only include left and current MB, top pixels is in dbk_line_buf
***********************************************************************/
void PrintMBBfFilter ()
{
	int		i;
	uint32	*	mb_ptr;	

	mb_ptr = vsp_dbk_out_bfr;

	for (i = 0; i < 172; i++)
	{
		fprintf (g_mb_bf_filter_fp, "%08x\n", *mb_ptr++);		
	}
}


/**********************************************************************
   these pixels should be checked after one mb filtered:
   including top, left and current MB pixels
***********************************************************************/
void PrintMBAfFilter (int mb_x)
{
if(g_vector_enable_flag&VECTOR_ENABLE_DBK)	
{
	int			i;
	uint32	*	mb_ptr;
	uint32		val;

	/*Y in dbk_out_buf*/
	mb_ptr = vsp_dbk_out_bfr;
	for (i = 0; i < 16*5; i++)
	{
		fprintf (g_mb_af_filter_fp, "%08x\n", *mb_ptr++);
	}


	/*Y in line_buf*/
	mb_ptr = g_dbk_line_buf + mb_x*4;
	for (i = 0; i < 4; i++)
	{
		if (mb_x == 0)
			val = 0;
		else 
			val = mb_ptr[-1];

		fprintf (g_mb_af_filter_fp, "%08x\n", val);
		fprintf (g_mb_af_filter_fp, "%08x\n", mb_ptr[0]);
		fprintf (g_mb_af_filter_fp, "%08x\n", mb_ptr[1]);
		fprintf (g_mb_af_filter_fp, "%08x\n", mb_ptr[2]);
		fprintf (g_mb_af_filter_fp, "%08x\n", mb_ptr[3]);

		mb_ptr += LINE_WIDTH_Y;		
	}

	/*U in dbk out buf*/
	mb_ptr = vsp_dbk_out_bfr + U_OFFSET_BUF_C;
	for (i = 0; i < 8*3; i++)
	{
		fprintf (g_mb_af_filter_fp, "%08x\n", *mb_ptr++);
	}

	/*U in line buffer*/
	mb_ptr = g_dbk_line_buf + U_OFFSET_LINE_BUF + mb_x*2;
	for (i = 0; i < 4; i++)
	{
		if (mb_x == 0)
			val = 0;
		else 
			val = mb_ptr[-1];

		fprintf (g_mb_af_filter_fp, "%08x\n", val);
		fprintf (g_mb_af_filter_fp, "%08x\n", mb_ptr[0]);
		fprintf (g_mb_af_filter_fp, "%08x\n", mb_ptr[1]);

		mb_ptr += LINE_WIDTH_C;		
	}

	/*V in dbk out buf*/
	mb_ptr = vsp_dbk_out_bfr + V_OFFSET_BUF_C;
	for (i = 0; i < 8*3; i++)
	{
		fprintf (g_mb_af_filter_fp, "%08x\n", *mb_ptr++);
	}

	/*U in line buffer*/
	mb_ptr = g_dbk_line_buf + V_OFFSET_LINE_BUF + mb_x*2;
	for (i = 0; i < 4; i++)
	{
		if (mb_x == 0)
			val = 0;
		else 
			val = mb_ptr[-1];

		fprintf (g_mb_af_filter_fp, "%08x\n", val);
		fprintf (g_mb_af_filter_fp, "%08x\n", mb_ptr[0]);
		fprintf (g_mb_af_filter_fp, "%08x\n", mb_ptr[1]);

		mb_ptr += LINE_WIDTH_C;		
	}
}
}

void PrintfFilterUpt (
					  int		mb_cnt,
					  int		blk_id,
					  int		line_id,
					  int		upt_flag,
					  uint32	p_upt,
					  uint32	q_upt			
					  )
{
if(g_vector_enable_flag&VECTOR_ENABLE_DBK)	
{
	fprintf (g_filter_upt_fp_fp, "%08x, %2d, %d, %02x, %08x, %08x\n", mb_cnt, blk_id, line_id, upt_flag, p_upt, q_upt);
}
}

