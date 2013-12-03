/*************************************************************************
** File Name:      aac_ltp.c                                             *
** Author:         Reed zhang                                            *
** Date:           09/11/2010                                            *
** Copyright:      2001 Spreatrum, Incoporated. All Rights Reserved.     *
** Description:                                                          *
**      This file is used to do LTP mode.                                *
**                        Edit History                                   *
** ----------------------------------------------------------------------*
** DATE           NAME             DESCRIPTION                           *
** 09/11/2010     Reed zhang       Create.                               *
**************************************************************************/
#include "aac_ltp.h"
#include "AAC_sbr_common.h"
#include "aac_tns.h"

const int16 AAC_DEC_LTP_code_book[8] // S2.14
=
{
        9352 , 
        11413, 
        13320, 
        14931, 
        16137, 
        17496, 
        19572, 
        22438  
        
};


int16   AAC_LTP_DEC_MDCT(int32 *in_ptr,
                         int32 *out_ptr);
                         
int16 AAC_DEC_FilterBankLtp(int32 *in_buf_ptr,
                            AAC_ICS_STREAM_T  *ics_ptr,
                            int16  win_shape_prev,
                            int32 *tmp_buf_ptr);
extern int16 AAC_LTP_DEC_SynthesisAsm(int32 *spec_ptr,
                                     int32 *ltp_est_ptr,
                                     uint16 *swb_offset_ptr,
                                     AAC_DEC_LTP_INFO_T *ltp_info_ptr);  
void AAC_LTP_RecSampleAsm(int32 *overlap_ptr,
                          int16 *last0_pcm_ptr,
                          int16 *last1_pcm_ptr,
                          int16 *rec_out_ptr);                                                                 
//////////////////////////////////////////////////////////////////////////
//#define LTP_TEST
int16 AAC_DEC_LtpModel(int32              *spec_ptr,
                       AAC_ICS_STREAM_T   *ics_ptr,
                       int16               window_shape_prev,
                       int32              *pred_sample_ptr,
                       int32              *tmp_buf_ptr,
                       
                       
                       int16              *last_first_frm_pcm_ptr,
                       int32              *last_first_frm_overlap_ptr
                       )
{
    int16 rel = 0;
    AAC_DEC_LTP_INFO_T *ltp_info_ptr = &ics_ptr->ltp_data_s;
    int16  *ptr = (int16 *) tmp_buf_ptr;
    
    if ((!ltp_info_ptr->data_present) || AAC_EIGHT_SHORT_SEQUENCE == ics_ptr->window_sequence)
    {
        return 0;
    }    
    ptr[0] = ltp_info_ptr->lag;
    AAC_LTP_RecSampleAsm(last_first_frm_overlap_ptr,
                         last_first_frm_pcm_ptr,
                         ltp_info_ptr->last_pcm_data,
                         ptr);
    
    
#ifdef LTP_TEST    
        {
            FILE *fp = fopen("AAC_DEC_LTPSynthesis_step1.txt", "wb");
            int i;
            for (i = 0; i < 2048; i++)
            {
                fprintf(fp, "%8d\n", ptr[i]);
            }
            fclose(fp);
            fp = NULL;
        }    
#endif
    
    
    
    /* step 1, ltp sample reconstruction, prediction and filterbank processing */
    rel = AAC_DEC_FilterBankLtp(pred_sample_ptr,
                                ics_ptr,
                                window_shape_prev,
                                tmp_buf_ptr);
    if (rel)
    {
        return rel;
    }
    /* step 2, ltp model TNS analysis filterbank processing */
    rel = AAC_DEC_TNSFrmAnalysisFilter(pred_sample_ptr,
                                 ltp_info_ptr,
                                 ics_ptr);
                                 
#ifdef LTP_TEST    
    {
        FILE *fp = fopen("AAC_DEC_LTPSynthesis0.txt", "wb");
        int i;
        for (i = 0; i < 1024; i++)
        {
            fprintf(fp, "%8d\n", pred_sample_ptr[i]);
        }
        fclose(fp);
    }    
#endif
                                 
                                 
    if (rel)
    {
        return rel;
    }
    /* step 3, ltp prediction data synthesis */
    rel = AAC_LTP_DEC_SynthesisAsm(
                                   spec_ptr,
                                   pred_sample_ptr,
                                   ics_ptr->swb_offset,
                                   ltp_info_ptr);
#ifdef LTP_TEST    
    {
        FILE *fp = fopen("AAC_DEC_LTPSynthesis.txt", "wb");
        int i;
        for (i = 0; i < 1024; i++)
        {
            fprintf(fp, "%8d\n", spec_ptr[i]>>14);
        }
        fclose(fp);
    }    
#endif
    
    return rel;
}

const extern int32 sine_long_1024[];
const extern int32 kbd_long_1024[];                          
const extern int32 sine_short_128[];
const extern int32 kbd_short_128[];


extern int32 AAC_LTP_ARMFilterBankLongAsm(
                                            int16   *in_rec_ptr,
                                            int32   *out_est_ptr,
                                            int16   *win_pre_ptr,
                                            int16   *win_cur_ptr);
extern int32 AAC_LTP_ARMFilterBankLongStartAsm(
                                               int16   *in_rec_ptr,
                                               int32   *out_est_ptr,
                                               int16   *win_pre_ptr,
                                               int16   *win_cur_ptr);     
extern int32 AAC_LTP_ARMFilterBankLongStopAsm(
                                              int16   *in_rec_ptr,
                                              int32   *out_est_ptr,
                                              int16   *win_pre_ptr,
                                              int16   *win_cur_ptr);                                                                                      
int16 AAC_DEC_FilterBankLtp(int32 *in_buf_ptr,
                            AAC_ICS_STREAM_T  *ics_ptr,
                            int16  win_shape_prev,
                            int32 *tmp_buf_ptr)
{
    int32 *fb_long_win_ptr[2];
    int32 *fb_short_win_ptr[2]; 
    int16 *rec_ptr;
    
#ifdef LTP_TEST    
    {
        FILE *fp = fopen("in.txt", "wb");
        int i;
        for (i = 0; i < 4096; i++)
        {
            fprintf(fp, "%8d\n", ics_ptr->ltp_data_s.ltp_pred_sample[i]);
        }
        fclose(fp);
    }    
#endif
    
    fb_long_win_ptr[0]  = (int32 *) sine_long_1024;
    fb_long_win_ptr[1]  = (int32 *) kbd_long_1024;
    fb_short_win_ptr[0] = (int32 *) sine_short_128;
    fb_short_win_ptr[1] = (int32 *) kbd_short_128;
    
    rec_ptr = (int16 *)tmp_buf_ptr;//ics_ptr->ltp_data_s.ltp_pred_sample + 2048 - ics_ptr->ltp_data_s.lag;
    
#ifdef LTP_TEST 
	/**/
    ics_ptr->ltp_data_s.lag = 449;
    ics_ptr->window_sequence = AAC_LONG_STOP_SEQUENCE;
#endif
    
    if (ics_ptr->ltp_data_s.lag < 1024)
       in_buf_ptr[0] = AAC_DEC_LTP_code_book[ics_ptr->ltp_data_s.coef] | (ics_ptr->ltp_data_s.lag << 16);
    else
       in_buf_ptr[0] = AAC_DEC_LTP_code_book[ics_ptr->ltp_data_s.coef] | (1024 << 16);
    switch(ics_ptr->window_sequence)
    {
    case AAC_ONLY_LONG_SEQUENCE:
        /* only long sequence */        
        AAC_LTP_ARMFilterBankLongAsm((int16*)rec_ptr,
                                    (int32*) in_buf_ptr,
                                    (int16*) fb_long_win_ptr[win_shape_prev],
                                    (int16*) fb_long_win_ptr[ics_ptr->window_shape]);
#ifdef LTP_TEST    
        {
            FILE *fp = fopen("win.txt", "wb");
            int i;
            for (i = 0; i < 2048; i++)
            {
                fprintf(fp, "%8d\n", in_buf_ptr[i]);
            }
            fclose(fp);
        }    
#endif          
                                           
        
        AAC_LTP_DEC_MDCT(in_buf_ptr, tmp_buf_ptr);
#ifdef LTP_TEST    
        {
            FILE *fp = fopen("MDCT.txt", "wb");
            int i;
            for (i = 0; i < 1024; i++)
            {
                fprintf(fp, "%8d\n", in_buf_ptr[i]);
            }
            fclose(fp);
        }    
#endif          
        
        break;
    case AAC_LONG_START_SEQUENCE:	
        /* long start sequence */
        
        AAC_LTP_ARMFilterBankLongStartAsm((int16*)rec_ptr,
                                          (int32*)in_buf_ptr,
                                          (int16*)fb_long_win_ptr[win_shape_prev],
                                          (int16*)fb_short_win_ptr[ics_ptr->window_shape]);
#ifdef LTP_TEST    
        {
            FILE *fp = fopen("win.txt", "wb");
            int i;
            for (i = 0; i < 2048; i++)
            {
                fprintf(fp, "%8d\n", in_buf_ptr[i]);
            }
            fclose(fp);
        }    
#endif          
        
        AAC_LTP_DEC_MDCT(in_buf_ptr, tmp_buf_ptr);
        
        break;
    case AAC_LONG_STOP_SEQUENCE:
        /* long stop sequence */
        AAC_LTP_ARMFilterBankLongStopAsm((int16*)rec_ptr,
                                         in_buf_ptr,
                                         (int16*)fb_short_win_ptr[win_shape_prev],
                                         (int16*)fb_long_win_ptr[ics_ptr->window_shape]);
        
        
#ifdef LTP_TEST    
        {
            FILE *fp = fopen("win.txt", "wb");
            int i;
            for (i = 0; i < 2048; i++)
            {
                fprintf(fp, "%8d\n", in_buf_ptr[i]);
            }
            fclose(fp);
        }    
#endif              
        AAC_LTP_DEC_MDCT(in_buf_ptr, tmp_buf_ptr);
        
        break;
    default:
        return 0x1000;     
    }   
    return 0;
}                            





//////////////////////////////////////////////////////////
/* MDCT model */
extern void AAC_LTP_DEC_FFTPreProcessAsm(int32 *in_data_ptr,
                                         int32 *out_data_ptr,
                                         int16 *table_ptr);
extern void AAC_LTP_DEC_FFTStep1Asm(int32 *in_data_ptr,
                                    int16 *table_ptr);
extern void AAC_LTP_DEC_FFTStep2Asm(int32 *in_data_ptr,
                                    int16 *table_ptr);
extern void AAC_LTP_DEC_FFTStep3Asm(int32 *in_data_ptr,
                                    int16 *table_ptr);
extern void AAC_LTP_DEC_FFTStep4Asm(int32 *in_data_ptr);
extern void AAC_LTP_DEC_FFTStep5Asm(int32 *in_data_ptr,
                                    int32 *out_data_ptr);  
extern void AAC_LTP_DEC_FFTPostProcessAsm(int32 *in_data_ptr,
                                          int16 *table_ptr);                                                                      

/* data*/
extern const int32 AAC_DEC_FFT_PrePost_Table[512];
extern const int32 AAC_DEC_FFT_Step1_Table[64];
extern const int32 AAC_DEC_FFT_Step2_Table[32*3];
extern const int32 AAC_DEC_FFT_Step3_Table[16*3];

int16   AAC_LTP_DEC_MDCT(int32 *in_ptr,
                         int32 *out_ptr)
{    
    
    AAC_LTP_DEC_FFTPreProcessAsm(in_ptr, out_ptr, (int16*) AAC_DEC_FFT_PrePost_Table);   
    
    
    AAC_LTP_DEC_FFTStep1Asm(out_ptr, (int16*)AAC_DEC_FFT_Step1_Table);
    /*
    //for (k = 0; k < 4; k++)
    {
    	for (i = 0; i < 1024; i ++)
        {
        	//fprintf(fp, "%d, \n", out_ptr[128*k + i]);
	        //fprintf(fp, "%d, \n", out_ptr[512 + 128*k + i]);
    	}
    }
    
    //fclose(fp);
    */    
    
    AAC_LTP_DEC_FFTStep2Asm(out_ptr, (int16*)AAC_DEC_FFT_Step2_Table);
/*    
    for (k = 0; k < 4; k++)
    {
    	for (i = 0; i < 128; i ++)
        {
        	fprintf(fp, "%d, \n", out_ptr[128*k+i]);	        
    	}
    	
    	for (i = 0; i < 128; i ++)
        {        	
	        fprintf(fp, "%d, \n", out_ptr[512+128*k+i]);
    	}
    }    
    fclose(fp);
*/    
    
    AAC_LTP_DEC_FFTStep3Asm(out_ptr, (int16*)AAC_DEC_FFT_Step3_Table);
    
    
    /*for (k = 0; k < 4; k++)
    {
    	for (i = 0; i < 32; i ++)
        {
            int32 t0, t1, t2, t3;
            t0 =  out_ptr[128*k+32*0+i];
            t1 =  out_ptr[128*k+32*1+i];
            t2 =  out_ptr[128*k+32*2+i];
            t3 =  out_ptr[128*k+32*3+i];
            
        	fprintf(fp, "%12d, %12d, %12d, %12d, \n", t0, t1, t2, t3);	        
    	}
    	
    	for (i = 0; i < 32; i ++)
        {        	
	        int32 t0, t1, t2, t3;
            t0 =  out_ptr[512+128*k+32*0+i];
            t1 =  out_ptr[512+128*k+32*1+i];
            t2 =  out_ptr[512+128*k+32*2+i];
            t3 =  out_ptr[512+128*k+32*3+i];
            
        	fprintf(fp, "%12d, %12d, %12d, %12d, \n", t0, t1, t2, t3);	    
    	}
    }    
    fclose(fp);*/
    
    
    AAC_LTP_DEC_FFTStep4Asm(out_ptr);
    /*{
    
        int m;
    	for (i = 0; i < 4; i ++)
        {
            int32 t0, t1, t2, t3;
            for (k = 0; k < 4; k++)
            {
	            for (m=0; m < 8; m++)
    	        {
        	        t0 =  out_ptr[128*k+32*i+8*0+m];
            	    t1 =  out_ptr[128*k+32*i+8*1+m];
                	t2 =  out_ptr[128*k+32*i+8*2+m];
	                t3 =  out_ptr[128*k+32*i+8*3+m];            
    	            fprintf(fp, "%12d, %12d, %12d, %12d, \n", t0, t1, t2, t3);
        	    }

	            for (m=0; m < 8; m++)
	            {
	                t0 =  out_ptr[512+128*k+32*i+8*0+m];
	                t1 =  out_ptr[512+128*k+32*i+8*1+m];
	                t2 =  out_ptr[512+128*k+32*i+8*2+m];
	                t3 =  out_ptr[512+128*k+32*i+8*3+m];            
	                fprintf(fp, "%12d, %12d, %12d, %12d, \n", t0, t1, t2, t3);	    
	            }
            }        	
    	}
    }    
    fclose(fp);*/
        
    AAC_LTP_DEC_FFTStep5Asm(out_ptr, in_ptr);
    
    /*fp = fopen("out_data5.dat", "wb");
    for (i = 0; i < 256; i ++)
    {
        int32 t0, t1, t2, t3;
        t0 =  in_ptr[0  +i];
        t1 =  in_ptr[256+i];
        t2 =  in_ptr[512+i];
        t3 =  in_ptr[768+i];
        fprintf(fp, "%12d, %12d, %12d, %12d, \n", t0, t1, t2, t3);
    }
    fclose(fp);*/
    
    AAC_LTP_DEC_FFTPostProcessAsm(in_ptr, (int16*) AAC_DEC_FFT_PrePost_Table);    
    return 0;
}


/* AAC-LTP stream parsing model */
/* LTP mpde side information stream parsing */
int16 AAC_DEC_LTPSideInfoParsing(AAC_ICS_STREAM_T      *ics_ptr,
                                 AAC_BIT_FIFO_FORMAT_T *ld_ptr)
{
    AAC_DEC_LTP_INFO_T    *ltp_info_ptr = &ics_ptr->ltp_data_s;
    int16 cw;
    cw = (int16) (AAC_GetBits(ld_ptr, 14));
    ltp_info_ptr->lag  = cw >> 3;  //AAC_GetBits(ld_ptr, 11);  // parsing the lag length
    ltp_info_ptr->coef = cw & 0x7; //AAC_GetBits(ld_ptr, 3);  // parsing the lag length
    
    if (ics_ptr->window_sequence != AAC_EIGHT_SHORT_SEQUENCE)
    {        
        /* only for long block including long block, long start block and long stop block */
        ltp_info_ptr->last_band = ics_ptr->max_sfb;
        if (ltp_info_ptr->last_band > AAC_DEC_LTP_MAX_SFB)
        {
            ltp_info_ptr->last_band = AAC_DEC_LTP_MAX_SFB;
        }
        for (cw = 0; cw < ltp_info_ptr->last_band; cw++)
        {
            ltp_info_ptr->long_used[cw] = AAC_Get1Bit(ld_ptr);
        }
    }
    
    return 0;
}