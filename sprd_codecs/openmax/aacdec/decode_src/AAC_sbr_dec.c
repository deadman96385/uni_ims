/*************************************************************************
** File Name:      sbr_dec.c                                             *
** Author:         Reed zhang                                            *
** Date:           12/01/2006                                            *
** Copyright:      2001 Spreatrum, Incoporated. All Rights Reserved.     *
** Description:    realize sbr_ptr signal analyzing                          *
**                                                                       *
**                        Edit History                                   *
** ----------------------------------------------------------------------*
** DATE           NAME             DESCRIPTION                           *
** 12/01/2006     Reed zhang       Create.                               *
** 19/01/2006     Reed zhang       finish the function AAC_HfGeneration     *
**                                 fixed-point code implementation and   *
**                                 testing it simply                     *
**************************************************************************/
#include "aac_common.h"
#include "aac_structs.h"
#include "AAC_sbr_dec.h"
#include "AAC_sbr_common.h"


static uint8 AAC_SbrSavePrevData(AAC_SBR_INFO_T *sbr_ptr, 
								  uint8  ch);

/* static function declarations */
void sbrDecodeInit(uint16 framelength, 
				   uint8  id_aac,
                   uint32 sample_rate,
				   AAC_SBR_INFO_T *sbr_ptr
                        )
{
    /* save id of the parent element */	
    sbr_ptr->id_aac                = id_aac;
    sbr_ptr->sample_rate        = sample_rate;
    sbr_ptr->bs_freq_scale      = 2;
    sbr_ptr->bs_alter_scale      = 1;
    sbr_ptr->bs_noise_bands     = 2;
    sbr_ptr->bs_limiter_bands   = 2;
    sbr_ptr->bs_limiter_gains     = 2;
    sbr_ptr->bs_interpol_freq    = 1;
    sbr_ptr->bs_smoothing_mode  = 1;
    sbr_ptr->bs_start_freq             = 5;
    sbr_ptr->bs_amp_res              = 1;
    sbr_ptr->bs_samplerate_mode = 1;
    sbr_ptr->prevEnvIsShort[0]     = -1;
    sbr_ptr->prevEnvIsShort[1]     = -1;
    sbr_ptr->header_count            = 0;
    sbr_ptr->bs_samplerate_mode = 1;
    /* force sbr_ptr reset */
    sbr_ptr->bs_start_freq_prev    = 0;
    sbr_ptr->numTimeSlotsRate    = AAC_RATE * AAC_NO_TIME_SLOTS;
    sbr_ptr->numTimeSlots           = AAC_NO_TIME_SLOTS;
    sbr_ptr->M_prev                    = 0;	

}

static uint8 AAC_SbrSavePrevData(AAC_SBR_INFO_T *sbr_ptr, 
								  uint8  ch)
{
    uint8 i;

    /* save data for next frame */
    sbr_ptr->kx_prev = sbr_ptr->kx;
	sbr_ptr->M_prev = sbr_ptr->M;
    sbr_ptr->L_E_prev[ch] = sbr_ptr->L_E[ch];
    /* sbr_ptr->L_E[ch] can become 0 on files with bit errors */
    if (sbr_ptr->L_E[ch] == 0)
        return 19;
    sbr_ptr->f_prev[ch] = sbr_ptr->f[ch][sbr_ptr->L_E[ch] - 1];
    for (i = 0; i < MAX_M; i++)
    {
        sbr_ptr->E_prev[ch][i] = sbr_ptr->E[ch][sbr_ptr->L_E[ch] - 1][i];
        sbr_ptr->Q_prev[ch][i] = sbr_ptr->Q[ch][sbr_ptr->L_Q[ch] - 1][i];
    }

    for (i = 0; i < MAX_M; i++)
    {
        sbr_ptr->bs_add_harmonic_prev[ch][i] = sbr_ptr->bs_add_harmonic[ch][i];
    }
    sbr_ptr->bs_add_harmonic_flag_prev[ch] = sbr_ptr->bs_add_harmonic_flag[ch];

    if (sbr_ptr->l_A[ch] == sbr_ptr->L_E[ch])
        sbr_ptr->prevEnvIsShort[ch] = 0;
    else
        sbr_ptr->prevEnvIsShort[ch] = -1;

    return 0;
}

static void SbrProcessChannel(  AAC_SBR_INFO_T	*sbr_ptr, 
								int32		*channel_buf,   // channel_ptr data buffer S30.2
                                uint8		ch, 
								uint8		dont_process,
								void   *aac_dec_mem_ptr)
{ 
    AAC_PLUS_DATA_STRUC_T *aac_dec_struc_ptr = (AAC_PLUS_DATA_STRUC_T *) aac_dec_mem_ptr;
    int16 k;
    int32 *tmp_shared_buffer_ptr = aac_dec_struc_ptr->g_shared_buffer;
    k = sbr_ptr->kx;
    if (dont_process)
    {
        k = 32;
    }
    /* subband analysis */	
    /* the output data sbr_ptr->Xsbr[ch] fixed-point is S32.0*/
    AAC_SbrQmfAnalysis( sbr_ptr, 
                                             sbr_ptr->qmfa[ch],
                                             channel_buf,        // decoded data from aac engine  fix-point: S17.2
                                             sbr_ptr->Xsbr[ch],  // the fixed-point: S21.0, caculate the low frequency information :sbr_ptr->Xcodec[ch]
                                              (uint8)k,
                                              tmp_shared_buffer_ptr);
    for (k = 8; k < 40; k++)
    {
        AAC_DEC_MEMSET(sbr_ptr->Xsbr[ch][k] + 32, 0, 32 * sizeof(aac_complex));
    }					
					

	
    /* hf generation using patching, insert high frequencies here */
    if (!dont_process)
    {
#if 0//def GEN_TEST_DATA
        if ((0 == ch) && (g_frm_counter > TEST_FRAME))
        {
            int16 i,j;
            FILE * fp = fopen("ARM_OPT_before_hf_gen.txt", "wb");
            for (j = 0; j < 40; j++)
            {
                fprintf(fp, "frm: %d, SLOT number: %d,\n", g_frm_counter, j);
                for (i = 0; i < 32; i ++)
                {
                    fprintf(fp, "%12d,  %12d, %12d,  %12d,\n", sbr_ptr->Xsbr[ch][j][i][0], sbr_ptr->Xsbr[ch][j][i][1], 
                        sbr_ptr->Xsbr[ch][j][i+32][0], sbr_ptr->Xsbr[ch][j][i+32][1]);
                }
            }
            fclose(fp);
        }
#endif
    
         AAC_HfGeneration(sbr_ptr, 
                                           sbr_ptr->Xsbr[ch],   // Low  frequency data: fixed-point: S21.0					  
                                           ch,
                                           tmp_shared_buffer_ptr);	

         if (sbr_ptr->Reset)
         {
             AAC_LimiterFrequencyTable(sbr_ptr, tmp_shared_buffer_ptr);
         }
         /* hf adjustment */
#if 0//def GEN_TEST_DATA
        if ((0 == ch) && (g_frm_counter > TEST_FRAME))
        {
            int16 i,j;
            FILE * fp = fopen("ARM_OPT_hf_gen.txt", "wb");
            for (j = 0; j < 40; j++)
            {
                fprintf(fp, "frm: %d, SLOT number: %d,\n", g_frm_counter, j);
                for (i = 0; i < 32; i ++)
                {
                    fprintf(fp, "%12d,  %12d, %12d,  %12d,\n", sbr_ptr->Xsbr[ch][j][i][0], sbr_ptr->Xsbr[ch][j][i][1], 
                        sbr_ptr->Xsbr[ch][j][i+32][0], sbr_ptr->Xsbr[ch][j][i+32][1]);
                }
            }
            fclose(fp);
        }
#endif

             AAC_HfAdjustment(sbr_ptr,
					 sbr_ptr->Xsbr[ch], // High frequency data: fixed-point: S21.0
					 ch,
					 tmp_shared_buffer_ptr
					 );
#if 0//def GEN_TEST_DATA
        if ((0 == ch) && (g_frm_counter > TEST_FRAME))
        {
            int16 i,j;
            FILE * fp = fopen("ARM_OPT_hf_ADJ.txt", "wb");
            for (j = 0; j < 40; j++)
            {
                fprintf(fp, "frm: %d, SLOT number: %d,\n", g_frm_counter, j);
                for (i = 0; i < 32; i ++)
                {
                    fprintf(fp, "%12d,  %12d, %12d,  %12d,\n", sbr_ptr->Xsbr[ch][j][i][0], sbr_ptr->Xsbr[ch][j][i][1], 
                        sbr_ptr->Xsbr[ch][j][i+32][0], sbr_ptr->Xsbr[ch][j][i+32][1]);
                }
            }
            fclose(fp);
        }
#endif

        }
}

int16 sbrDecodeCoupleFrame(AAC_SBR_INFO_T      *sbr_ptr,                 // sbr_ptr relative information
							 int32        *left_chan,           // left channel_ptr data : S17.2
							 int32        *right_chan,          // right channel_ptr data: S17.2                                
							 void           *aac_dec_mem_ptr
							 )
{
    AAC_PLUS_DATA_STRUC_T *aac_dec_struc_ptr = (AAC_PLUS_DATA_STRUC_T *) aac_dec_mem_ptr;
    uint8 dont_process = 0;
    int16 ret = 0, i;
    int32 *tmp_shared_buffer_ptr = aac_dec_struc_ptr->g_shared_buffer;
    if (sbr_ptr == NULL)
        return 20;
    /* case can occur due to bit errors */
    /* delete this error handling */
    //if (sbr_ptr->id_aac != AAC_ID_CPE)
    //    return 21;
    if (sbr_ptr->ret || (sbr_ptr->header_count == 0))
    {

        /* don't process just upsample */
        dont_process = 1;
        /* Re-activate reset for next frame */
        if (sbr_ptr->ret && sbr_ptr->Reset)
            sbr_ptr->bs_start_freq_prev = 0;
    }
    /*Left channel_ptr processing*/
#if 0//def GEN_TEST_DATA
    if (g_frm_counter >= TEST_FRAME)
    {
        FILE *fp; 
        int k;	        
        fp = fopen("VC_pcm_out_BFsys.txt", "wb");
        for (k = 0; k < 40; k++)
        {	   
            int i;
            for (i = 0; i < 64; i++)
            {
                fprintf(fp, "frm: %2d, k: %2d, l: %2d, real: %10d,  image: %10d\n", g_frm_counter, k, i, sbr_ptr->Xsbr[0][k][i][0], sbr_ptr->Xsbr[0][k][i][1]);
            }            
        }		
        fclose(fp);
        
    }
#endif
    SbrProcessChannel(sbr_ptr,      // sbr_ptr relative information   
					  left_chan,// left channel_ptr data from AAC core , S17.2
					  0, 
					  dont_process,
					  aac_dec_mem_ptr);

	/* subband synthesis */

#if 0//def GEN_TEST_DATA
    if (g_frm_counter >= TEST_FRAME)
    {
        FILE *fp; 
        int k;			
        
        fp = fopen("VC_pcm_out_sys.txt", "wb");
        for (k = 0; k < 40; k++)
        {	   
            int i;
            for (i = 0; i < 64; i++)
            {
                fprintf(fp, "frm: %2d, k: %2d, l: %2d, real: %10d,  image: %10d\n", g_frm_counter, k, i, sbr_ptr->Xsbr[0][k][i][0], sbr_ptr->Xsbr[0][k][i][1]);
            }
            
        }		
        fclose(fp);
        
    }
#endif
    AAC_SbrQmfSynthesis(32, 
		sbr_ptr->qmfs[0],
		(sbr_ptr->Xsbr[0] + 2),      // S21.0
		aac_dec_struc_ptr->pcm_out_l_ptr,   // S16
		tmp_shared_buffer_ptr
		);	
	
	/*Left channel_ptr processing*/
    SbrProcessChannel(  sbr_ptr, 
						right_chan,
						1, 
						dont_process,
						aac_dec_mem_ptr);    
	
    /* subband synthesis */
    AAC_SbrQmfSynthesis( 32, 
                                           sbr_ptr->qmfs[1],
                                            (sbr_ptr->Xsbr[1] + 2), 
                                           (aac_dec_struc_ptr->pcm_out_r_ptr),
                                           tmp_shared_buffer_ptr);

    for (i = 0; i < AAC_T_HFGEN; i++)
    {       
        AAC_DEC_MEMMOVE(sbr_ptr->Xsbr[0][i],   sbr_ptr->Xsbr[0][i+32], 64 * sizeof(aac_complex));
        AAC_DEC_MEMMOVE(sbr_ptr->Xsbr[1][i],   sbr_ptr->Xsbr[1][i+32], 64 * sizeof(aac_complex));
    }
#if 0//def TEST_DATA
    {
        FILE   *ps_decorrelation_ptr;
        int16  l;
        //if (ps_decorrelation_ptr == NULL)
        {
            ps_decorrelation_ptr = fopen("syn_shift_bandC.txt", "a+");
        }        
        for(l = 0; l < AAC_T_HFGEN; l++)
        {
            int i;
            for (i = 0; i < 64; i++)
            {
                int32 re0, im0, re1, im1;
                re0 = sbr_ptr->Xsbr[0][l][i][0];
                im0 = sbr_ptr->Xsbr[0][l][i][1];
                
                re1 = sbr_ptr->Xsbr[0][l][i][0];
                im1 = sbr_ptr->Xsbr[0][l][i][1];
                fprintf(ps_decorrelation_ptr, "frame: %4d, Line: %10d,  %8d,  %8d,  %8d,  %8d\n", g_frm_counter, i, re0, im0, re1, im1);
            }            
        }
        fclose(ps_decorrelation_ptr);
        ps_decorrelation_ptr = NULL;
    }
    
#endif	
    if (sbr_ptr->bs_header_flag)
        sbr_ptr->just_seeked = 0;
    if (sbr_ptr->header_count != 0 && sbr_ptr->ret == 0)
    {
        ret = AAC_SbrSavePrevData(sbr_ptr, 0);
        if (ret) 
            return ret;
        ret = AAC_SbrSavePrevData(sbr_ptr, 1);
        if (ret)
            return ret;
    }
    sbr_ptr->frame++;
    return 0;
}

uint8 sbrDecodeSingleFrame(AAC_SBR_INFO_T      *sbr_ptr,          // the relative info. on sbr_ptr decoder
							 int32        *channel_ptr,      // the current frame dataafter AAC decoder
                             const uint8 just_seeked,
							 void           *aac_dec_mem_ptr)
{	
    AAC_PLUS_DATA_STRUC_T *aac_dec_struc_ptr = (AAC_PLUS_DATA_STRUC_T *) aac_dec_mem_ptr;
    uint8   dont_process = 0;
    uint8   ret          = 0, i;
    int32 *tmp_shared_buffer_ptr = aac_dec_struc_ptr->g_shared_buffer;
    if (sbr_ptr == NULL)
        return 20;
     /* case can occur due to bit errors */
     if (sbr_ptr->id_aac != AAC_ID_SCE && sbr_ptr->id_aac != AAC_ID_LFE)
        return 21;
    if (sbr_ptr->ret || (sbr_ptr->header_count == 0))
    {
        /* don't process just upsample */
        dont_process = 1;
        /* Re-activate reset for next frame */
        if (sbr_ptr->ret && sbr_ptr->Reset)
            sbr_ptr->bs_start_freq_prev = 0;
    }
    if (just_seeked)
    {
        sbr_ptr->just_seeked = 1;
    } else {
        sbr_ptr->just_seeked = 0;
    }
    SbrProcessChannel(  sbr_ptr,
                                     channel_ptr, 
                                     0, 
                                     dont_process,
                                     aac_dec_mem_ptr);


#if 0
	{
		FILE *fp = fopen("..\\SBR_HF.txt", "wb");
		int i;
		for (i = 0; i < 40; i++)
		{
			int j;
			fprintf(fp, "index: %d\n", i);
			for(j = 0; j < 64; j++)
			{
				fprintf(fp, "%5d,   %5d,  \n", sbr_ptr->Xsbr[0][2 + i][j][0], sbr_ptr->Xsbr[0][2 + i][j][1]);
			}

		}
		fclose(fp);
	}
#endif

    /* subband synthesis */
    AAC_SbrQmfSynthesis(32, 
                                          sbr_ptr->qmfs[0], 
                                          (sbr_ptr->Xsbr[0] + 2), 
                                          aac_dec_struc_ptr->pcm_out_l_ptr,
                                          tmp_shared_buffer_ptr);
    for (i = 0; i < AAC_T_HFGEN; i++)
    {       
        AAC_DEC_MEMMOVE(sbr_ptr->Xsbr[0][i],   sbr_ptr->Xsbr  [0][i+32], 64 * sizeof(aac_complex));
    }
    if (sbr_ptr->bs_header_flag)
        sbr_ptr->just_seeked = 0;

    if (sbr_ptr->header_count != 0 && sbr_ptr->ret == 0)
    {
        ret = AAC_SbrSavePrevData(sbr_ptr, 0);
        if (ret) return ret;
    }
    sbr_ptr->frame++;
    return 0;
}

#ifdef AAC_PS_DEC
uint8 sbrDecodeSingleFramePS(AAC_SBR_INFO_T      *sbr_ptr, 
							   int32       *left_channel_ptr,    //  S16.2
							   void          *aac_dec_mem_ptr
                               )
{
    uint8 dont_process = 0;
    uint8 ret = 0;    
    uint8 l;	
    /* case can occur due to bit errors */
    if (sbr_ptr->id_aac != AAC_ID_SCE && sbr_ptr->id_aac != AAC_ID_LFE)
        return 21;
	
    if (sbr_ptr->ret || (sbr_ptr->header_count == 0))
    {
        /* don't process just upsample */
        dont_process = 1;		
        /* Re-activate reset for next frame */
        if (sbr_ptr->ret && sbr_ptr->Reset)
            sbr_ptr->bs_start_freq_prev = 255;
    }	

    SbrProcessChannel(sbr_ptr, 
                                       left_channel_ptr, //  S16.2
				    0, 
				    dont_process,
				    aac_dec_mem_ptr);	
#if 0
	{
		FILE *fp = fopen("..\\SBR_HF.txt", "wb");
		int i;
		for (i = 0; i < 40; i++)
		{
			int j;
			fprintf(fp, "index: %d\n", i);
			for(j = 0; j < 64; j++)
			{
				fprintf(fp, "%5d,   %5d,  \n", sbr_ptr->Xsbr[0][2 + i][j][0], sbr_ptr->Xsbr[0][2 + i][j][1]);
			}
		}
		fclose(fp);
	}
#endif
    sbr_ptr->ps->usb = sbr_ptr->f_table_res[0][sbr_ptr->N_low];
    /* perform parametric stereo */
    PsDecode((sbr_ptr->ps), 
		     (sbr_ptr->Xsbr[0] + AAC_T_HFADJ),   // S17.0
                      aac_dec_mem_ptr);	
    for(l = 0; l < AAC_T_HFGEN; l++)
    {
        AAC_DEC_MEMCPY(sbr_ptr->Xsbr[0][l], sbr_ptr->Xsbr[0][l+32], 64 * sizeof(aac_complex));
    }

#if 0//def TEST_DATA
    {
        FILE   *ps_decorrelation_ptr;
        //if (ps_decorrelation_ptr == NULL)
        {
            ps_decorrelation_ptr = fopen("syn_shift_band.txt", "a+");
        }        
        for(l = 0; l < AAC_T_HFGEN; l++)
        {
            int i;
            for (i = 0; i < 64; i++)
            {
                int32 re0, im0;
                re0 = sbr_ptr->Xsbr[0][l][i][0];
                im0 = sbr_ptr->Xsbr[0][l][i][1];
                fprintf(ps_decorrelation_ptr, "frame: %4d, Line: %10d,  %8d,  %8d\n", g_frm_counter, i, re0, im0);
            }            
        }
        fclose(ps_decorrelation_ptr);
        ps_decorrelation_ptr = NULL;
    }
    
#endif	
    if (sbr_ptr->bs_header_flag)
        sbr_ptr->just_seeked = 0;
    if (sbr_ptr->header_count != 0 && sbr_ptr->ret == 0)
    {
        ret = AAC_SbrSavePrevData(sbr_ptr, 0);
        if (ret) return ret;
    }	
    sbr_ptr->frame++;
    return 0;
}
#endif
