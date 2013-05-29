/******************************************************************************
 ** File Name:	  mca_trace.c                                                *
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
#include "sc6800x_video_header.h"
/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    extern   "C" 
    {
#endif

int g_mca_tv_num = 0;

PUBLIC void Mp4Dec_Trace_MCA_result_one_macroblock(uint8 *pMcaBfr)
{
	if(g_vector_enable_flag&VECTOR_ENABLE_MCA)
	{
		int32 mb_x = g_glb_reg_ptr->VSP_CTRL0 & 0xff; //
		int32 mb_y = ((g_glb_reg_ptr->VSP_CTRL0>>8) & 0xff);
		int ii, jj;
		uint8 *pLine;

 	// 	FPRINTF (g_fp_mca_tv, "frame: %d, mb_y: %d, mb_x: %d\n", g_nFrame_dec, mb_y, mb_x);

		for (ii = 0; ii < MB_SIZE; ii++)
		{
			pLine = &pMcaBfr[ii *MB_SIZE];
			for(jj = 0; jj < 4; jj++)
			{
				FomatPrint4Pix(pLine, g_fp_mca_tv);
				pLine += 4;	
				g_mca_tv_num++;
			}			
		}

		pMcaBfr += MB_SIZE*MB_SIZE;
		for (ii = 0; ii < BLOCK_SIZE; ii++)
		{
			pLine = &pMcaBfr[ii *BLOCK_SIZE];
			for(jj = 0; jj < 2; jj++)
			{
				FomatPrint4Pix(pLine, g_fp_mca_tv);
				pLine += 4;
				g_mca_tv_num++;
			}			
		}

		pMcaBfr += BLOCK_SIZE*BLOCK_SIZE;
		for (ii = 0; ii < BLOCK_SIZE; ii++)
		{
			pLine = &pMcaBfr[ii *BLOCK_SIZE];
			for(jj = 0; jj < 2; jj++)
			{
				FomatPrint4Pix(pLine, g_fp_mca_tv);
				pLine += 4;
				g_mca_tv_num++;
			}			
		}
	}
}

#if defined(MPEG4_DEC)
PUBLIC void Mp4Dec_Trace_Interpolation_result_one_macroblock_forDebug(FILE * pfIntp, uint8 *pMcaBfr)
{
if(g_trace_enable_flag&TRACE_ENABLE_MCA)
{
	int32 mb_x = g_glb_reg_ptr->VSP_CTRL0 & 0xff; //
	int32 mb_y = ((g_glb_reg_ptr->VSP_CTRL0>>8) & 0xff);
	uint8 * pix_ptr;
	int i, j;

 	FPRINTF (g_fp_trace_mca, "frame: %d, mb_y: %d, mb_x: %d\n", g_nFrame_dec, mb_y, mb_x);

	//Y
	FPRINTF (g_fp_trace_mca, "Y MB\n");
	pix_ptr = pMcaBfr;
	for (i = 0; i < MB_SIZE; i++)
	{
		for (j = 0; j < MB_SIZE; j++)
		{
			FPRINTF (g_fp_trace_mca, "%3d, ", pix_ptr[i*MB_SIZE + j]);
		}
		FPRINTF (g_fp_trace_mca, "\n");
	}

	//U
	FPRINTF (g_fp_trace_mca, "\nU MB\n");
	pix_ptr += MB_SIZE*MB_SIZE;
	for (i = 0; i < BLOCK_SIZE; i++)
	{
		for (j = 0; j < BLOCK_SIZE; j++)
		{
			FPRINTF (g_fp_trace_mca, "%3d, ", pix_ptr[i*BLOCK_SIZE + j]);
		}
		FPRINTF (g_fp_trace_mca, "\n");
	}

	FPRINTF (g_fp_trace_mca, "\nV MB\n");
	pix_ptr += BLOCK_SIZE*BLOCK_SIZE;
	for (i = 0; i < BLOCK_SIZE; i++)
	{
		for (j = 0; j < BLOCK_SIZE; j++)
		{
			FPRINTF (g_fp_trace_mca, "%3d, ", pix_ptr[i*BLOCK_SIZE + j]);
		}
		FPRINTF (g_fp_trace_mca, "\n");
	}

	FPRINTF (g_fp_trace_mca, "\n\n");
}
}
 #endif //

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End 
