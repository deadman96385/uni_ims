#include <stdio.h>
#include <stdlib.h>
#include "sci_types.h"
#include "common_global.h"

void Trace_after_intra_prediction_luma16x6(uint8 *pred_y)
{
if(g_trace_enable_flag&TRACE_ENABLE_IPRED)	
{
	int i,j;

	for(i = 0; i < 16; i++)
	{
		for(j = 0; j < 16; j++)
		{
			FPRINTF(g_ipred_log_fp, "%3d, ", pred_y[i*16+j]);
		}
		FPRINTF(g_ipred_log_fp, "\n");
	}
	FPRINTF(g_ipred_log_fp, "\n");
}
}

void Trace_after_intra_prediction_chroma8x8 (uint8 *pred_uv)
{
if(g_trace_enable_flag&TRACE_ENABLE_IPRED)	
{
	int i,j;

	for(i = 0; i < 8; i++)
	{
		for(j = 0; j < 8; j++)
		{
			FPRINTF(g_ipred_log_fp, "%3d, ", pred_uv[i*8+j]);
		}
		FPRINTF(g_ipred_log_fp, "\n");
	}
	FPRINTF(g_ipred_log_fp, "\n");
}
}

void Trace_after_intra_prediction_Blk4x4(uint8 *pred, uint32 width)
{
if(g_trace_enable_flag&TRACE_ENABLE_IPRED)	
{
	int i,j;

	for(i = 0; i < 4; i++)
	{
		for(j = 0; j < 4; j++)
		{
			FPRINTF(g_ipred_log_fp, "%3d, ", pred[i*width+j]);
		}
		FPRINTF(g_ipred_log_fp, "\n");
	}
	FPRINTF(g_ipred_log_fp, "\n");
}
}

void trace_ipred(uint8 *tmp_pred_Y, uint8 *tmp_pred_U, uint8 *tmp_pred_V)
{
if(g_trace_enable_flag&TRACE_ENABLE_IPRED)	
{
	int ii, jj;
	FPRINTF(g_fp_trace_ipred, "Curr mb Pred value\n");
	FPRINTF(g_fp_trace_ipred, "+----------------------------------------------------------------------------------------\n");
	for (ii = 0; ii < MB_SIZE; ii++)
	{
		FPRINTF(g_fp_trace_ipred, "| ");
		for (jj = 0; jj < MB_SIZE; jj++)
		{
			FPRINTF(g_fp_trace_ipred, "%03d, ", tmp_pred_Y[ii *MB_SIZE + jj]);
			if ((jj + 1) % 4 == 0)
			{
				if ((jj + 1) % 8 == 0)
				{
					FPRINTF(g_fp_trace_ipred, "+ ");
				}
				else
				{
					FPRINTF(g_fp_trace_ipred, "| ");
				}
			}
		}
		FPRINTF(g_fp_trace_ipred, "\n");
		if ((ii + 1) % 4 == 0)
		{
			if ((jj + 1) % 8 == 0)
			{
				FPRINTF(g_fp_trace_ipred, "---------------------------------------------------------------------------------------\n");
			}
			else
			{
				FPRINTF(g_fp_trace_ipred, "+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
			}
		}
	}
	FPRINTF(g_fp_trace_ipred, "\n");
	
	for (ii = 0; ii < BLOCK_SIZE; ii++)
	{
		FPRINTF(g_fp_trace_ipred, "| ");
		for (jj = 0; jj < BLOCK_SIZE; jj++)
		{
			FPRINTF(g_fp_trace_ipred, "%03d, ", tmp_pred_U[ii *BLOCK_SIZE + jj]);
		}
		FPRINTF(g_fp_trace_ipred, "\t|\t");
		for (jj = 0; jj < BLOCK_SIZE; jj++)
		{
			FPRINTF(g_fp_trace_ipred, "%03d, ", tmp_pred_V[ii *BLOCK_SIZE + jj]);
		}
		FPRINTF(g_fp_trace_ipred, "\n");
	}
	FPRINTF(g_fp_trace_ipred, "\n");
}
}