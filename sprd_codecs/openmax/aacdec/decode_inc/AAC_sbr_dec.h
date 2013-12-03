/*************************************************************************
** File Name:      sbr_dec.h                                             *
** Author:         Reed zhang                                            *
** Date:           12/01/2006                                            *
** Copyright:      2001 Spreatrum, Incoporated. All Rights Reserved.     *
** Description:    realize sbr_ptr dequatization algorithm                   *               
**                                                                       *
**                        Edit History                                   *
** ----------------------------------------------------------------------*
** DATE           NAME             DESCRIPTION                           *
** 12/01/2006     Reed zhang       Create.                               *
**************************************************************************/

#ifndef __SBR_DEC_H__
#define __SBR_DEC_H__

#ifdef __cplusplus
extern "C" {
#endif

#ifdef AAC_PS_DEC
#include "AAC_ps_dec.h"
#endif
/* MAX_NTSRHFG: maximum of number_time_slots * rate + HFGen. 16*2+8 */
#define MAX_NTSRHFG 40
typedef struct
{
    uint8   kx;                                               //  0
    uint8   M;                                               //  1
    uint8   k0;                                               // 2
    uint8   noPatches;                                   // 3
    uint8   psi_is_prev[2];                             // 4
    int8    l_A[2];                                         // 6
    int8    l_A_prev[2];                                //  8
    int8    prevEnvIsShort[2];                       //  10
    int16    GQ_ringbuf_index[2];                 //  12
    uint16  index_noise_prev[2];                  //  16
    uint8   t_E[2][6];                                   //   20   
    
    int32    G_temp_prev[2][5][49];             //  32+1960
    int32    Q_temp_prev[2][5][49];             //  32+1960 + 1960
    
    // sbr gen 
    int32   bwArray[2][5];                            // S0.12    
    uint8   patchStartSubband[8];    
    uint8   patchNoSubbands[8];
    uint8   table_map_k_to_g[64];
    
    uint8   L_E[2];        
    uint8   rate;
    uint8   just_seeked;    
    
    uint32  sample_rate;
    
    uint8   ret;
    uint8   amp_res[2];    
    uint8   N_master;    

    uint8   N_L[4];
    uint8   n[2];
    uint8   N_high;
    uint8   N_low;

    uint8   f_master[64];
    uint8   f_table_res[2][64];
    uint8   f_table_noise[64];
    uint8   f_table_lim[4][64];
    uint8   f_group[5][64];
    uint8   N_G[5];    
    uint8   N_Q;
    uint8   abs_bord_lead[2];
    uint8   abs_bord_trail[2];
    uint8   n_rel_lead[2];
    uint8   n_rel_trail[2];    
    uint8   L_E_prev[2]
    ;
    uint8   L_Q[2];    
    uint8   t_Q[2][3];
    
    uint8   f[2][6];
    
    
    int16   E_prev[2][64];	
    int16   E[2][5][64];	
    int16   Q[2][2][64];
    uint32  (*E_curr)[64];   // S16.0
    int16   Q_prev[2][64];  
    uint8   bs_invf_mode[2][5];
    uint8   bs_invf_mode_prev[2][5];
    
    uint8   bs_add_harmonic[2][64];
    uint8   bs_add_harmonic_prev[2][64];
    uint8   f_prev[2];    
    uint8   bs_start_freq_prev;
    uint8   bs_stop_freq_prev;
    
    uint8   bs_xover_band_prev;
    uint8   bs_freq_scale_prev;
    uint8   bs_alter_scale_prev;
    uint8   bs_noise_bands_prev;

    uint8   id_aac;
    int8    kx_prev;
    uint8   M_prev;
    uint8   Reset;
    uint32  frame;
    uint32  header_count;
    
    
    
    int32   *qmfa[2];
    int32   *qmfs[2];
    aac_complex     Xsbr[2][MAX_NTSRHFG][64];     // the high frequency information  
     /* to get it compiling */
    /* we'll see during the coding of all the tools, whether
       these are all used or not.*/    
    uint16  bs_sbr_crc_bits;
    uint8   bs_header_flag;
    uint8   numTimeSlotsRate;
    
    uint8   numTimeSlots;  
    uint8   bs_protocol_version;
    uint8	  bs_amp_res;
    uint8   bs_start_freq;
    
    uint8   bs_stop_freq;
    uint8   bs_xover_band;
    uint8   bs_freq_scale;
    uint8   bs_alter_scale;
    
    uint8   bs_noise_bands;
    uint8   bs_limiter_bands;
    uint8   bs_limiter_gains;
    uint8   bs_interpol_freq;
    
    uint8   bs_smoothing_mode;
    uint8   bs_samplerate_mode;
    uint8   bs_add_harmonic_flag[2];
    
    uint8   bs_add_harmonic_flag_prev[2];
    uint8   bs_extended_data;
    uint8   bs_extension_id;
    
    uint8   bs_extension_data;
    uint8   bs_coupling;
    uint8   bs_frame_class[2];
    
    uint8   bs_rel_bord[2][9];
    uint8   bs_rel_bord_0[2][9];
    uint8   bs_rel_bord_1[2][9];
    uint8   bs_pointer[2];
    
    uint8   bs_abs_bord_0[2];
    uint8   bs_abs_bord_1[2];
    
    uint8   bs_num_rel_0[2];
    uint8   bs_num_rel_1[2];
    
    uint8   bs_df_env[2][9];
    uint8   bs_df_noise[2][3];
    
 #if (defined(AAC_PS_DEC))
    uint8 ps_used;
    uint8 res[3];
#endif   

	
#ifdef AAC_PS_DEC
    AAC_PS_INFO_T *ps;
#endif

} AAC_SBR_INFO_T;








   






void sbrDecodeInit(uint16 framelength, 
				   uint8  id_aac,
				   uint32 sample_rate,
				   AAC_SBR_INFO_T *sbr_ptr
				   );

/************************************************************************/
//double channel aac audio sbr_ptr decoder
/************************************************************************/
int16 sbrDecodeCoupleFrame(AAC_SBR_INFO_T *sbr_ptr,                // relative abr information for decoder
							 int32 *left_chan_ptr,            // aac left channel data
							 int32 *right_chan_ptr,            // aac right channel data
							 void    *aac_dec_mem_pre                              
							 );
/************************************************************************/
//single channel aac audio sbr_ptr decoder
/************************************************************************/
uint8 sbrDecodeSingleFrame(AAC_SBR_INFO_T *sbr_ptr,                // relative abr information for decoder
							 int32 *channel_ptr,              // aac audio data
                             const uint8 just_seeked,
							 void           *aac_dec_mem_ptr);



#if (defined(AAC_PS_DEC) || defined(DRM_PS))
uint8 sbrDecodeSingleFramePS(AAC_SBR_INFO_T      *sbr_ptr, 
							   int32       *left_channel_ptr,
							   void          *aac_dec_mem_ptr);
#endif


#ifdef __cplusplus
}
#endif
#endif

