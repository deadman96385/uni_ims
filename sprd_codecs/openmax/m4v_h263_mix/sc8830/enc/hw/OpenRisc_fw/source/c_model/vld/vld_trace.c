/******************************************************************************
 ** File Name:    vld_trace.c	    										  *
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
FILE * g_mpeg4dec_vld_trace_fp;  //asic order, intra is after inverse ac_dc prediction
FILE * g_mpegdec_vld_no_acdc_fp; //normal order, intra is before ac_dc prediction
FILE * g_pfRunLevel_mpeg4dec;

void Mp4Dec_VldTraceInit ()
{
if(g_trace_enable_flag&VECTOR_ENABLE_VLD)
{
	char test_vect_file_path[200] = "..\\..\\trace\\";//"D:/SC8801H/code/Firmware_RTL_verification_Mp4Dec/mp4_decoder/simulation/trace/";
	char module_tv_file_path[200];

	strcpy(module_tv_file_path, test_vect_file_path);
	g_mpeg4dec_vld_trace_fp = fopen(strcat(module_tv_file_path, "vld_infor.txt"), "w");
	assert (g_mpeg4dec_vld_trace_fp != NULL);

	strcpy(module_tv_file_path, test_vect_file_path);
	g_mpegdec_vld_no_acdc_fp = fopen (strcat(module_tv_file_path, "vld_infor_no_acdc.txt"), "w");
	assert (g_mpegdec_vld_no_acdc_fp != NULL);

	strcpy(module_tv_file_path, test_vect_file_path);
	g_pfRunLevel_mpeg4dec = fopen (strcat(module_tv_file_path, "vld_run_leve.txt"), "w");
	assert (g_pfRunLevel_mpeg4dec != NULL);
}
}


void FprintfOneBlock (int16 * dct_coef_blk_ptr, FILE * file_ptr)
{
if(g_trace_enable_flag&VECTOR_ENABLE_VLD)
{
	int i, j;
	fprintf (file_ptr, "\n");

	for (i = 0; i < 8; i++)
	{
		for (j = 0; j < 8; j++)
		{
			fprintf (file_ptr, "0x%08x, ", dct_coef_blk_ptr [i*8 + j]);
		}

		fprintf (file_ptr, "\n");
	}

	fprintf (file_ptr, "\n");
}
}


/*printf codeword in binary mode*/
void printf_codeWordInfo (int codeWord, int codeLen, FILE * pfHuffVal)
{
if(g_trace_enable_flag&VECTOR_ENABLE_VLD)
{
	int i;
	
	fprintf (pfHuffVal, "codeWord: ");
	for (i = codeLen - 1; i >= 0; i--)
	{
		if (codeWord & (1<<i))
		{
			fprintf (pfHuffVal, "1");
		}
		else
		{
			fprintf (pfHuffVal, "0");
		}
	}
	
	fprintf (pfHuffVal, "  codeLen: %d  ", codeLen);
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