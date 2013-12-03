/*************************************************************************
** File Name:      filtbank.c                                            *
** Author:         Reed zhang                                            *
** Date:           12/08/2006                                            *
** Copyright:      2001 Spreatrum, Incoporated. All Rights Reserved.     *
** Description:    
**                        Edit History                                   *
** ----------------------------------------------------------------------*
** DATE           NAME             DESCRIPTION                           *
** 12/08/2006     Reed zhang       Create and assembly implement AAC     *
                                   IMCT and window filter.               *
**************************************************************************/
#include "aac_common.h"
#include "aac_structs.h"

extern  void short_block_only_window(AAC_DATA_PARA_STRUC_T * data_ptr); 

extern void AAC_DEC_Imdct128Asm(int32 *in_buf,
						 int32 *tmp_buf);
extern void asm_imdct1024_pre(int32 * ptr1, int32 *ptr2, int32 *ptr3, int16 N);              
extern void AAC_DEC_OnlyLongBlockWindowLCLtpAsm(AAC_DATA_PARA_STRUC_T *ptr, int16 *pcm_out_ptr);     
extern void AAC_DEC_LongStartBlockWindowLCLtpAsm(AAC_DATA_PARA_STRUC_T *ptr, int16 *pcm_out_ptr);
extern void AAC_DEC_LongBlockStopWindowLC_LtpAsm(AAC_DATA_PARA_STRUC_T *ptr, int16 *pcm_out_ptr); 
extern void AAC_DEC_ShortBlockOnlyWindowLCLtpAsm(AAC_DATA_PARA_STRUC_T *ptr, int16 *pcm_out_ptr);
extern int32 AAC_DEC_ARM_OnlyLongBlockProcessingAsm(
                                              int32   *in_data_ptr,
                                              int32   *overlap_ptr,
                                              int16   *pcm_out_ptr,  
                                              int32   *table_addr_ptr              
                                              );
extern int32 AAC_DEC_SBR_ARM_OnlyLongBlockProcessingAsm(
                                              int32   *in_data_ptr,
                                              int32   *overlap_ptr,
                                              int32   *pcm_out_ptr,  
                                              int32   *table_addr_ptr              
                                              );
extern int32 AAC_DEC_ARM_StartLongBlockProcessingAsm(
                                              int32   *in_data_ptr,
                                              int32   *overlap_ptr,
                                              int16   *pcm_out_ptr,  
                                              int32   *table_addr_ptr              
                                              );
extern int32 AAC_DEC_SBR_ARM_StartLongBlockProcessingAsm(
                                              int32   *in_data_ptr,
                                              int32   *overlap_ptr,
                                              int32   *pcm_out_ptr,  
                                              int32   *table_addr_ptr              
                                              );
extern int32 AAC_DEC_ARM_StopLongBlockProcessingAsm(
                                              int32   *in_data_ptr,
                                              int32   *overlap_ptr,
                                              int16   *pcm_out_ptr,  
                                              int32   *table_addr_ptr              
                                              );
extern int32 AAC_DEC_SBR_ARM_StopLongBlockProcessingAsm(
                                              int32   *in_data_ptr,
                                              int32   *overlap_ptr,
                                              int32   *pcm_out_ptr,  
                                              int32   *table_addr_ptr              
                                              );
                          
extern const int32 imdct_sin_cos_tbl_16_64[];                               		                                        
extern int32 AAC_DEC_SIN_win[1024];	
extern int32 AAC_DEC_KBD_win[1024];                        
const extern int32 sine_short_128[];
const extern int32 kbd_short_128[];
extern void AAC_DEC_Imdct128PostReorderAsm(int32 *in_data_ptr,
                                           int32 *out_data_ptr,
                                           int32 *table_ptr);

static void AAC_DEC_IMDCT1024(int32 *in_data_ptr, 
                       int32 *out_data_ptr);
                       
void AAC_IfilterBank(  uint8  window_sequence, 
				     uint8  window_shape,
                                         uint8  window_shape_prev, 
	   			     int32 *freq_in,   //the data before IMDCT                  
				     int32 *overlap,    //the saving data for next frame imdct 
				     int16 *tmp_sample_buffer,
				     int16  aac_format_sign,  // 5, sbr , 2 aac_lc, 4, AAC_LTP
				     void  *aac_dec_mem_ptr,
				     int16 *ltp_sample_rec_ptr)
{
        AAC_DATA_PARA_STRUC_T  data_strc = {0};
        AAC_PLUS_DATA_STRUC_T *aac_dec_struc_ptr = (AAC_PLUS_DATA_STRUC_T *) aac_dec_mem_ptr;
        const int32 *fb_long_window[2];
        const int32 *fb_short_window[2];
        const int32 *window_long       = NULL;
        const int32 *window_long_prev  = NULL;
        const int32 *window_short      = NULL;
        const int32 *window_short_prev = NULL;	
        int32       *ptr1, *ptr2, i;
        int32       *shared_buffer_ptr = aac_dec_struc_ptr->g_shared_buffer;
        int32       addr[2];	
        fb_long_window[0]  = AAC_DEC_SIN_win;
        fb_long_window[1]  = AAC_DEC_KBD_win;
        fb_short_window[0] = sine_short_128;
        fb_short_window[1] = kbd_short_128;

        /* new implementation, the algorithm can meet the full compliance test */
        window_long             = fb_long_window[window_shape];
        window_long_prev   = fb_long_window[window_shape_prev];
        window_short           = fb_short_window[window_shape];
        window_short_prev = fb_short_window[window_shape_prev];	
        
        ptr1 = freq_in;
        ptr2 = shared_buffer_ptr;
        //window_sequence = LONG_START_SEQUENCE;
        switch (window_sequence)
        {
        case AAC_ONLY_LONG_SEQUENCE:		
                /* 1024 point IDCT processing */		
                AAC_DEC_IMDCT1024(freq_in, shared_buffer_ptr);  // the outpu data size: 1024
                addr[0] = (int32) window_long_prev;
                addr[1] = (int32) window_long;
        
                /* output the PCM data or SBR data */
                if (AAC_LC == aac_format_sign || AAC_LTP == aac_format_sign)
               {
                        /* output the AAC_LC/LTP PCM data */
                        AAC_DEC_ARM_OnlyLongBlockProcessingAsm(shared_buffer_ptr, overlap, tmp_sample_buffer, addr);
               }else
               {
                        /* for SBR */
                        AAC_DEC_SBR_ARM_OnlyLongBlockProcessingAsm(shared_buffer_ptr, overlap, freq_in, addr);
               }	    
				
                break;
        case AAC_LONG_START_SEQUENCE:		
		/* 1024 point IDCT processing */		
                  AAC_DEC_IMDCT1024(freq_in, shared_buffer_ptr);  // the outpu data size: 1024
                  addr[0] = (int32)  window_long_prev;
                  addr[1] = (int32) window_short;
        
                  if (AAC_LC == aac_format_sign || AAC_LTP == aac_format_sign)
                  {
                          /* output the AAC_LC PCM data */
                          AAC_DEC_ARM_StartLongBlockProcessingAsm(shared_buffer_ptr, overlap, tmp_sample_buffer, addr);
                  }else
                  {
                           /* output the PCM data or SBR data */
                           AAC_DEC_SBR_ARM_StartLongBlockProcessingAsm(shared_buffer_ptr, overlap, freq_in, addr);
                  }
		break;		                                
        case AAC_EIGHT_SHORT_SEQUENCE:		
            /* short(128 point) block IMDCT processing */				
            for (i = 0; i < 8; i++)
            {			
                 asm_imdct1024_pre( ptr1,                    // intput data
                                                   ptr2,                    // output data
                                                   (int32 *) imdct_sin_cos_tbl_16_64,
                                                   64);		                           
                          
                 AAC_DEC_Imdct128Asm(ptr2,                   // intput data
                                                            ptr1);                   // output data
	    	    
                 AAC_DEC_Imdct128PostReorderAsm(ptr1,                    // intput data
                                                                               ptr2,                    // output data
                                                                              (int32 *) (imdct_sin_cos_tbl_16_64));                 	       
                 ptr1 += 128;	    
                 ptr2 += 256;	    
             }   
             ptr1 = shared_buffer_ptr;		
             /* short block window filter processing */ 
             data_strc.overlap_data_ptr   = overlap;
             data_strc.pcm_filter_ptr     = (int32*) window_short_prev;
             data_strc.overlap_filter_ptr = (int32*)  window_short;
             data_strc.input_data1_ptr    = (int32*) ptr1;
             data_strc.tmp_buf_ptr        = (int32*) freq_in;
             if (AAC_LC == aac_format_sign || AAC_LTP == aac_format_sign)
             {
                  /* output the AAC_LC PCM data */
                 AAC_DEC_ShortBlockOnlyWindowLCLtpAsm(&data_strc, tmp_sample_buffer);
             }else
            {
                 /* output the PCM data or SBR data */
                 short_block_only_window(&data_strc);
            }
            break;
        case AAC_LONG_STOP_SEQUENCE:		
            /* 1024 point IDCT processing */		
            AAC_DEC_IMDCT1024(freq_in, shared_buffer_ptr);  // the outpu data size: 1024
            addr[0] = (int32) window_short_prev;
            addr[1] = (int32) window_long;		
            if (AAC_LC == aac_format_sign || AAC_LTP == aac_format_sign)
            {
                /* output the AAC_LC PCM data */
                AAC_DEC_ARM_StopLongBlockProcessingAsm(shared_buffer_ptr, overlap, tmp_sample_buffer, addr);
            }else
            {
                /* output the PCM data or SBR data */
                AAC_DEC_SBR_ARM_StopLongBlockProcessingAsm(shared_buffer_ptr, overlap, freq_in, addr);
            }
				
            break;
        default:
            break;       		
        }
}

/* improved IMDCT algorithm 
   2010-12-20
*/
/* do 1024 point IMDCT
   The fast implementaion algorithm is done base on the IFFT algorithm
 */
 
 
 
extern int32 AAC_DEC_ARM_LongImdctPreAsm(
                                         int32   *in_out_ptr,
                                         int32   *table_ptr
                                        );     
extern int32 AAC_DEC_ARM_LongIfftStep1Asm(
                                          int32   *in_out_ptr,
                                          int32   *table_ptr
                                         );
extern int32 AAC_DEC_ARM_LongImdctPostAsm(
                                           int32   *in_out_ptr,
                                           int32   *table_ptr
                                          );      
extern int32 AAC_DEC_ARM_LongIfftStep2Asm(
                                   int32   *in_out_ptr,
                                   int32   *table_ptr
                                   );   
                                   
extern int32 AAC_DEC_ARM_LongIfftStep3Asm(
                                              int32   *in_out_ptr,
                                              int32   *table_ptr
                                          );
extern int32 AAC_DEC_ARM_LongIfftStep4Asm(
                                          int32   *in_out_ptr
                                          );
extern void AAC_LTP_DEC_IFFTStep5Asm(int32 *in_data_ptr,
                              int32 *out_data_ptr);   
                                                                        
extern int32 AAC_DEC_Imdct_prepost_table[512];
extern int32 AAC_DEC_IFFT_step1[63];
extern int32 AAC_DEC_IFFT_step2[248];
extern int32 AAC_DEC_IFFT_step3[84];                                          
static void AAC_DEC_IMDCT1024(int32 *in_data_ptr, 
                       int32 *out_data_ptr)
{
    /* pre-processing */
    AAC_DEC_ARM_LongImdctPreAsm(in_data_ptr, AAC_DEC_Imdct_prepost_table);
    
#if 0//def FFT_test //def PRINT_TEST_DATA// print test data
	{
		FILE *fp; 
		int k;		
		int32 *ptr2 = (int32 *)  in_data_ptr;
		
		fp = fopen("fft_steppre.txt", "wb");
		for (k = 0; k < 1024; k++)
		{		
			int32 tt = ptr2[k];				
			fprintf(fp, "%d\n", tt);
		}		
		fclose(fp);
		
	}
#endif  
    
    /* step 1*/
    AAC_DEC_ARM_LongIfftStep1Asm(in_data_ptr, AAC_DEC_IFFT_step1);
    
#if 0//def FFT_test //def PRINT_TEST_DATA// print test data
	{
		FILE *fp; 
		int k;		
		int32 *ptr2 = (int32 *)  in_data_ptr;
		
		fp = fopen("fft_step1out.txt", "wb");
		for (k = 0; k < 1024; k++)
		{		
			int32 tt = ptr2[k];				
			fprintf(fp, "%d\n", tt);
		}		
		fclose(fp);
		
	}
#endif      
    
    
    /* step 2*/
    AAC_DEC_ARM_LongIfftStep2Asm(in_data_ptr, AAC_DEC_IFFT_step2);
    
#if 0    
    {
        FILE *fp;
        int16 k, i;
        fp = fopen("fft_step2out.txt", "wb");
        //for (k = 0; k < 4; k++)
	    {
	    	for (i = 0; i < 128; i ++)
	        {
	        	fprintf(fp, "%14d, %14d, %14d, %14d,\n", in_data_ptr[128*0+i], in_data_ptr[128*1+i], in_data_ptr[128*2+i], in_data_ptr[128*3+i]);	        
	    	}
	    	
	    	for (i = 0; i < 128; i ++)
	        {        	
		        fprintf(fp, "%14d, %14d, %14d, %14d,\n", in_data_ptr[512+128*0+i], in_data_ptr[512+128*1+i], in_data_ptr[512+128*2+i], in_data_ptr[512+128*3+i]);
	    	}
	    }    
	    fclose(fp);
        
    }
#endif    
/**/    
    
    /* step 3*/
    AAC_DEC_ARM_LongIfftStep3Asm(in_data_ptr, AAC_DEC_IFFT_step3);
    
    /* step 4*/
    AAC_DEC_ARM_LongIfftStep4Asm(in_data_ptr);
    
    /* step 5 */
    AAC_LTP_DEC_IFFTStep5Asm(in_data_ptr, out_data_ptr);
#if 0//def FFT_test //def PRINT_TEST_DATA// print test data
	{
		FILE *fp; 
		int k;		
		int32 *ptr2 = (int32 *) out_data_ptr;
		
		fp = fopen("fft_out.txt", "wb");
		for (k = 0; k < 1024; k++)
		{		
			int32 tt = out_data_ptr[k];				
			fprintf(fp, "%d\n", tt);
		}		
		fclose(fp);
		
	}
#endif      
    
    
    
    /* post processing */
    AAC_DEC_ARM_LongImdctPostAsm(out_data_ptr, AAC_DEC_Imdct_prepost_table);
}
