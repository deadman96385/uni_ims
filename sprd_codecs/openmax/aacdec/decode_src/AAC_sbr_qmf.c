/*************************************************************************
** File Name:      sbr_qmf.c                                             *
** Author:         Reed zhang                                            *
** Date:           12/01/2006                                            *
** Copyright:      2001 Spreatrum, Incoporated. All Rights Reserved.     *
** Description:    realize SBR QMF model signal analysis                 *               
**                                                                       *
**                        Edit History                                   *
** ----------------------------------------------------------------------*
** DATE           NAME             DESCRIPTION                           *
** 12/01/2006     Reed zhang       Create.                               *

** 13/01/2006     Reed zhang       finish sbr_qmf_analysis_32 function   *
**                                 fixed-point implementation            *
**************************************************************************/
#include "aac_common.h"
#include "aac_structs.h"
#include "AAC_sbr_qmf.h"

//#define TEST_DATA
/************************************************************************/
/* the function description:                                            */
/* function: is used to split the time domain signal output from the    */
/*           core decoder into 32 subband signals.                      */
/* interface:                                                           */
/* sbr_ptr  : the relative information for QMF analysis                     */
/* qmfa_ptr : output data                                                   */
/* input: the input channel data                                        */
/* X    : the output data for HF generation model                       */
/************************************************************************/
// the assembly code function interface
extern void asm_DCT_IV(int32 *buffer);
extern void asm_sbr_synthesis_filter(uint16 *tmp_out,  // output data
                                                        int32  *t_x1      // input data
                                                         );     // table
extern void asm_sbr_synthesis_dct_post(int32 *t_x1,									   
									   int32 *in_real1);						  			 
extern void asm_sbr_synthesis_dct_pre(int32 *in_real1,
									  int32 *t_x1);		
extern void asm_sbr_analysis_filter(int32 *out_data,
									int32 *intput,
									int32 *t_x);				  			 
extern void asm_sbr_analysis_post(int32 *t_x,
								  int32 *in_real,
								  uint8  kx);		
//#define TEST_ANA_DATA
//#define SYN_TEST_DATA
void AAC_SbrQmfAnalysis(
AAC_SBR_INFO_T	   *sbr_ptr, 
int32                              *qmfa_ptr, 
int32                              *input,            // fix-point: S17.2
aac_complex                X[MAX_NTSRHFG][64],// fix-point: S21.0
uint8                              kx,
int32                             *tmp_buf_ptr)
{
    uint16  l;		
    int32  *t_x = qmfa_ptr;
    int32  *t_x1;	
    int32   *in_real = tmp_buf_ptr;
    int32  *tmp_buf = tmp_buf_ptr + 128;
    /* qmf subsample No l */
    uint8 numTimeSlotsRate = sbr_ptr->numTimeSlotsRate;
    t_x1 = tmp_buf + 1024;
    AAC_DEC_MEMCPY(t_x1, t_x, 320 * sizeof(int32));    
    for (l = 0; l < numTimeSlotsRate; l++)
    {
        t_x1 -= 32;		
        t_x = t_x1; 
        /* assembly code */		
        t_x = t_x1+32;
        asm_sbr_analysis_filter(in_real,
                                            input,
                                            t_x);
#ifdef TEST_ANA_DATA
		{
			FILE *fp;
			int i;
			
			fp = fopen("..\\asm_sbr_analysis_filter.txt", "wb");
			
			
			for (i = 0; i < 64; i++)
			{
			        fprintf(fp, "%5d, %5d\n", in_real[i], t_x[i-32]);
			}
			fclose(fp);
		}
#endif	

        input += 32;
        asm_DCT_IV(in_real);		
#ifdef TEST_ANA_DATA       
		{
			FILE *fp;
			int i;			
			fp = fopen("..\\AAC_FftDifFxed.txt", "wb");	
			for (i = 0; i < 32; i++)
			{
				fprintf(fp, "%5d, %5d, \n", in_real[i], in_real[i+32]);
			}
			fclose(fp);
		}
		kx = 32-l;
#endif	
        t_x  = X[l + AAC_T_HFGEN][0];
        asm_sbr_analysis_post(    t_x,
                                                in_real,
                                                kx);
#ifdef TEST_ANA_DATA
		{
			FILE *fp;
			int i;
			
			fp = fopen("..\\ADS_qmf.txt", "wb");
			
			
			for (i = 0; i < 64; i+=2)
			{
				fprintf(fp, "kx: %3d,  %5d, %5d, \n", kx, t_x[i], t_x[i+1]);
			}
			fclose(fp);
		}
#endif	

    }
    AAC_DEC_MEMCPY(qmfa_ptr, tmp_buf, 320 * sizeof(int32));
}



/* high quality mode */
void AAC_SbrQmfSynthesis(int16             numTimeSlotsRate, 
                                             int32           *qmfs, 
                                             aac_complex X[MAX_NTSRHFG][64],  // S22.0
                                             uint16           *output,              // S16
                                             int32             *tmp_buf_ptr
                                             )
{	
        int32  *t_x1;
        uint16  l;	
        int32  in_real1[128];   
        int32  *tmp_v;
        uint16     *tmp_out = output;	
        if (32 == numTimeSlotsRate)
        {
                tmp_v = tmp_buf_ptr + 4096;	
                AAC_DEC_MEMCPY(tmp_v, qmfs, 1280 * sizeof(int32));
        }else
        {
                tmp_v = tmp_buf_ptr;
        }
        ////////////////////////////////////////////////
        for (l = 0; l < numTimeSlotsRate; l++)
        {
                /* shift buffer v */
                tmp_v -= 128;		
                t_x1 = X[l][0];
                /* assembly code */

                asm_sbr_synthesis_dct_pre(in_real1,
                                                          t_x1
                                                           );
#ifdef SYN_TEST_DATA   
        if (g_frm_counter > TEST_FRAME)
        {
            FILE *fp;
            int i;
            
            fp = fopen("..\\ADS_qmf_syn.txt", "wb");
            
            
            for (i = 0; i < 128; i++)
            {
                fprintf(fp, "%5d, \n", in_real1[i]);
            }
            fclose(fp);
        }
#endif
		
                asm_DCT_IV(in_real1);
                asm_DCT_IV(in_real1+64);
                t_x1 = tmp_v;
#ifdef SYN_TEST_DATA   
        if (g_frm_counter > TEST_FRAME)
        {
            FILE *fp;
            int i;
            
            fp = fopen("..\\AAC_FftDifFxed.txt", "wb");
            
            
            for (i = 0; i < 128; i++)
            {
                fprintf(fp, "%5d, \n", in_real1[i]);
            }
            fclose(fp);
        }
#endif
                
                
                asm_sbr_synthesis_dct_post(t_x1,								  
                                                             in_real1);
#ifdef SYN_TEST_DATA   
        if (g_frm_counter > TEST_FRAME)
        {
            FILE *fp;
            int i;
            
            fp = fopen("..\\asm_sbr_synthesis_dct_post.txt", "wb");
            
            
            for (i = 0; i < 1280; i++)
            {
                fprintf(fp, "%5d, \n", t_x1[i]);
            }
            fclose(fp);
        }
#endif
                asm_sbr_synthesis_filter(tmp_out,
                                                      t_x1);
#ifdef SYN_TEST_DATA   
        if (g_frm_counter > TEST_FRAME)
        {
            FILE *fp;
            int i;
            
            fp = fopen("..\\asm_sbr_synthesis_pcm.txt", "wb");
            
            
            for (i = 0; i < 64; i++)
            {
                fprintf(fp, "%5d, \n", tmp_out[i]);
            }
            fclose(fp);
        }
#endif
                tmp_out += 64;
        }
        if (32 == numTimeSlotsRate)
        {
                AAC_DEC_MEMCPY(qmfs, tmp_v, 1280 * sizeof(int32));
        }
}

