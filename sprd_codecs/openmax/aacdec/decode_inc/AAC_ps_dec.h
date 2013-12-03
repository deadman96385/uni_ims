/*************************************************************************
** File Name:      ps_dec.h                                              *
** Author:         Reed zhang                                            *
** Date:           18/04/2006                                            *
** Copyright:      2001 Spreatrum, Incoporated. All Rights Reserved.     *
** Description:    realize SBR signal analyzing                          *               
**                                                                       *
**                        Edit History                                   *
** ----------------------------------------------------------------------*
** DATE           NAME             DESCRIPTION                           *
** 18/04/2006     Reed zhang       Create.                               *
**************************************************************************/
#ifndef __PS_DEC_H__
#define __PS_DEC_H__

#ifdef __cplusplus
extern "C" {
#endif
#include "aac_bits.h"

#define AAC_PS_EXTENSION_ID_PS 2
#define AAC_PS_MAX_PS_ENVELOPES 5
#define AAC_PS_NO_ALLPASS_LINKS 3
#ifdef _AAC_PS_BASE_LINE_
typedef int16  ps_sh_complex;
#define PS_RE(A) A
#else
typedef int16  ps_sh_complex[2];
#define PS_RE(A) A[0]
#define PS_IM(A) A[1]
#endif
/* type definitions */
typedef struct
{
    uint8 frame_len;
    uint8 resolution20[3];

    aac_complex   buffer[3][44];
} AAC_HYB_INFO_T;
	
typedef struct
{
    /* for asm code */
    uint8    saved_delay;                                                //  pos 0
    uint8    next_temp_delay_ser[AAC_PS_NO_ALLPASS_LINKS];                                    //  pos 1
    int16    usb;                                                            //  pos 4
    int16    delay_D ;                                                   //  pos  6
    int32  *AAC_ps_Q_fract_allpass_subQmf20_ptr;  // pos  8
    int32  *Phi_Fract_SubQmf_ptr;                              // pos  12
    int32  *DecaySlope_filt_ptr;                                    // pos  16
    int32  *AAC_ps_phi_fract_qmf_ptr;                       // pos  20
    uint16 *map_group2bk;                                           // pos  24
    uint16 *group_border;                                             // pos  28        
    uint32 P_PeakDecayNrg[20];                                 // pos   32
    uint32 P_prev[20];                                                  // pos   112
    uint32 P_SmoothPeakDecayDiffNrg_prev[20];       // pos   192
    aac_complex delay_SubQmf[2][12];                                                                 /* pos:   272,   size:  192,      2 samples delay max (SubQmf is always allpass filtered) */
    aac_complex delay_SubQmf_ser[AAC_PS_NO_ALLPASS_LINKS][5][16]; /* pos:  464,   size:  1920,      5 samples delay max (table 8.34) */
    aac_complex delay_Qmf_nr_allpass_bandsLT22[2][23];                                    /*pos: 2384,  size: 368,  14 samples delay max, 64 QMF channels */
    aac_complex delay_Qmf_ser[AAC_PS_NO_ALLPASS_LINKS][5][32];         /*pos:2752,  size: 3840,  5 samples delay max (table 8.34), 32 QMF channels */
    aac_complex delay_Qmf_nr_allpass_bandsGT22[14][12];   /* pos:  6592,     size: 1344   14 samples delay max, 12 QMF channels */
    aac_complex delay_Qmf_nr_allpass_bandsGE35[29];         /* pos:  7936,     29 samples delay max, 41QMF channels */
    /* bitstream parameters */
    uint8 decay_cutoff;     
    uint8 enable_iid;
    uint8 enable_icc;
    uint8 enable_ext;
    uint8 iid_mode;
    uint8 icc_mode;
    uint8 nr_iid_par;
    uint8 nr_ipdopd_par;
    uint8 nr_icc_par;
    uint8 frame_class;
    uint8 num_env;
    /* a header has been read */
    uint8  header_read;
    
    uint8 border_position[AAC_PS_MAX_PS_ENVELOPES+1];
    uint8 iid_dt[AAC_PS_MAX_PS_ENVELOPES];
    uint8 icc_dt[AAC_PS_MAX_PS_ENVELOPES];
    
    /* indices */
    int8  iid_index_prev[34];
    int8  icc_index_prev[34];    
    int8  iid_index[AAC_PS_MAX_PS_ENVELOPES][34];
    int8  icc_index[AAC_PS_MAX_PS_ENVELOPES][34]; 
   
    /* hybrid filterbank parameters */
    AAC_HYB_INFO_T hyb;
    
    /**/
    uint8 num_groups;
    uint8 num_hybrid_groups;
    uint8 nr_par_bands;
    uint8 nr_allpass_bands;
    /* filter delay handling */
    /* mixing and phase */
    int32 h11_prev[50];
    int32 h12_prev[50];
    int32 h21_prev[50];
    int32 h22_prev[50];     
    int16 ps_init_sign;
    uint8 phase_hist;
    uint8  use34hybrid_bands;
    /* ps data was correctly read */
    uint8  ps_data_available;
    uint8 enable_ipdopd;
    uint8 ipd_mode;   
    uint8 res;
    
    
} AAC_PS_INFO_T;

/* ps_syntax.c */
uint16 AAC_PsData(AAC_PS_INFO_T *ps_ptr,
				 AAC_BIT_FIFO_FORMAT_T *ld_ptr, 
				 uint8 *header_ptr);
/* ps_dec.c */
void AAC_PsInit(uint8 sr_index,
			 AAC_PS_INFO_T *ps_ptr);
/* for ps decoder */
uint8 PsDecode( AAC_PS_INFO_T *ps_ptr, 
				  aac_complex X_left[38][64], 
				  void   *aac_dec_mem_ptr
				  );

	
#ifdef __cplusplus
}
#endif
#endif

