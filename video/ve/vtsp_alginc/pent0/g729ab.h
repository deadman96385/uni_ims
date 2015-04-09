/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2006-2007 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * +D2Tech+ Rev: 2193  Date: 2007-08-24 11:14:40 -0700 (Fri, 24 Aug 2007) 
 * +D2Tech+ Release Version: alg_core3-g729ab_arm
 *
 *
 *
 *  ==================== MHz on m24ke and m34K ==============================
 *          ENCODER         DECODER     FULL DUPLEX
 *   10ms   20.2            6.0         26.2 MHz
 *
 *  ==================== MHz on m4kc1 =======================================
 *          ENCODER         DECODER     FULL DUPLEX
 *   10ms   31.9            8.2         40.1 MHz
 *
 *  ==================== MHz on a9201 =======================================
 *          ENCODER         DECODER     FULL DUPLEX
 *   10ms   39.37           10.0        49.37 MHz
 *
 *  ==================== MHz on TI TNETV1020 ================================
 *          ENCODER         DECODER     FULL DUPLEX
 *   10ms   xx.9            xx.2         xx.1 MHz
 *
 *

 */

#ifndef __G729AB_H__
#define __G729AB_H__

#include "d2types.h"

#define     G729AB_L_FRAME      (80)      /* Frame size. */
#define     G729AB_PRM_SIZE     (11)      /* Sizeof vector of analysis params */
#define     G729AB_M            (10)      /* Order of LP filter.          */
#define     G729AB_L_INTERPOL   (10+1)    /* Len of filter for interpolation*/
#define     G729AB_L_TOTAL      (240)     /* Total size of speech buffer. */
#define     G729AB_MA_NP        (4)       /* MA prediction order for LSP */
#define     G729AB_MP1          (G729AB_M+1) /* Order of LP filter + 1    */
#define     G729AB_NB_SUMACF    (3)
#define     G729AB_NB_CURACF    (2)
#define     G729AB_NB_GAIN      (2)
#define     G729AB_SIZ_SUMACF   (G729AB_NB_SUMACF * G729AB_MP1)
#define     G729AB_SIZ_ACF      (G729AB_NB_CURACF * G729AB_MP1)
#define     G729AB_PIT_MAX      (143)     /* Maximum pitch lag.           */
#define     G729AB_SERIAL_SIZE  (80+2)    /* bfi+ number of speech bits   */


typedef struct {
    /* These 6 elements are copied to the stack, so their order can't change */
    int32    y1_hi;
    int32    y2_hi;
    int32    y1_lo;
    int32    y2_lo;
    int32    x0;
    int32    x1;
} PRE_PROC_Str;

typedef struct {
    int16   Prev_Min;
    int16   Min;         /* must come after Prev_Min */
    int16   Next_Min;
    int16   MeanE;
    int16   MeanSE;
    int16   MeanSLE;
    int16   MeanSZC;
    int16   prev_energy;
    vint    count_sil;
    vint    count_update;
    vint    count_ext;
    vint    flag;
    vint    v_flag;
    vint    less_count;
    int16   MeanLSF[G729AB_M];
    int16   Min_buffer[16];
} VAD_Str;

typedef struct {
    vint     fr_cur;                        /* for _G729AB_Cod_cng */
    vint     nb_ener;                       /* for _G729AB_Cod_cng */
    vint     flag_chang;                    /* for _G729AB_Cod_cng */
    vint     count_fr0;                     /* for _G729AB_Cod_cng */
    vint     sh_RCoeff;                     /* for _G729AB_Cod_cng */
    vint     cur_gain;                      /* for _G729AB_Cod_cng */
    vint     sid_gain;                      /* for _G729AB_Cod_cng */
    vint     prev_energy;                   /* Must follow flag_chang */
    int16    sh_Acf[G729AB_NB_CURACF];      /* [2] for _G729AB_Cod_cng*/
    int16    sh_sumAcf[G729AB_NB_SUMACF];   /* [3] Must follow sh_Acf[] */
    int16    sh_ener[G729AB_NB_GAIN];       /* [2] Must follow sh_sumAcf[] */
    int16    ener[G729AB_NB_GAIN];          /* [2]  Must follow sh_ener[] */
    int16    lspSid_q[G729AB_M];            /* for _G729AB_Cod_cng*/
    int16    pastCoeff[G729AB_MP1];         /* for _G729AB_Cod_cng*/
    int16    RCoeff[G729AB_MP1];            /* for _G729AB_Cod_cng*/
    int16    Acf[G729AB_SIZ_ACF];           /* [22] for _G729AB_Cod_cng*/
    int16    sumAcf[G729AB_SIZ_SUMACF];     /* [33] for _G729AB_Cod_cng*/
} COD_CNG_Str;   /* 104 words */

/*
 *  ======== G729AB ENCODER OBJECT ========
 */
typedef struct {
    vint            vad_enable;          /* 1 enables silence detection */
    /* These 5 elements are copied to the stack, so their order can't change */
    vint            sharp;               /* init to SHARPMIN */
    vint            pastVad;             /* init to 1 */
    vint            ppastVad;            /* init to 1 */
    vint            frm_count;           /* wraps to 256 */
    vint            seed;                /* init to INIT_SEED */
    int16           lsp_old[G729AB_M];   /* LSP */
    PRE_PROC_Str    pre_proc_str;
    /* Note: needs alignment */
    int32           L_exc_err[4];        /* array of 4 32-bit values */
    vint            past_qua_en[4];      /* for _G729AB_Qua_gain */
    int16           lsp_old_q[G729AB_M]; /* quantized LSP */
    int16           old_rc[2];           /* for _G729AB_Levinson */
    int16           old_A[G729AB_MP1];   /* for _G729AB_Levinson (must follow old_rc) */
    int16           mem_w0[G729AB_M];
    int16           mem_w[G729AB_M];
    int16           freq_prev[G729AB_MA_NP][G729AB_M];
    int16           old_wsp[G729AB_PIT_MAX+G729AB_L_FRAME];
    int16           old_exc[G729AB_PIT_MAX+G729AB_L_INTERPOL+G729AB_L_FRAME];
    int16           old_speech[G729AB_L_TOTAL-G729AB_L_FRAME+G729AB_L_FRAME];
    VAD_Str         vad_str;
    COD_CNG_Str     cod_cng_str;
} G729AB_EncObj;

typedef struct {
    vint    y1_lo;   /* The order of the 6 Post_Proc */
    vint    y2_lo;   /* variables is significant */
    vint    y1_hi;
    vint    y2_hi;
    vint    x0;
    vint    x1;
} POST_PROC_Str;

typedef struct {
    int16    mem_pre;
    int16    past_gain;
    int16    mem_synBuf[G729AB_M];
    int16    mem_syn_pst[G729AB_M];
    int16    res2_buf[G729AB_PIT_MAX+G729AB_L_FRAME];
} POST_FILT_Str;

/* total of 421+5=426 words for decoder object */
/*
 *  ======== G729AB DECODER OBJECT ========
 */
/* total of 421 words for internal decoder object */
typedef struct {
    /*
     * The first 3 elements are set by the calling routine
     */
    vint            nBytes;     /* 0, 2, or 10 bytes */
    vint            bfi;        /* 0=valid frame, 1=bad frame */
    vint            bad_lsf;    /* 0=valid */
    /* These 10 elements are copied to the stack, so their order can't change */
    vint            old_T0;     /* init to 60 */
    vint            sharp;      /* init to SHARPMIN */
    vint            gain_pitch;
    vint            gain_code;
    int16           prev_ma;    /* for _G729AB_D_lsp */
    vint            seed_fer;   /* init to 21845 */
    vint            seed;       /* init to INIT_SEED for _G729AB_Random() */
    vint            past_ftyp;  /* init to 1 */
    vint            sh_sid_sav; /* init to 1 */
    vint            sid_sav;
    /* These 6 elements are copied to the stack, so their order can't change */
    POST_PROC_Str   post_proc_str;
    int16           mem_syn[G729AB_M];
    vint            past_qua_en[4];
    int16           lsp_old[G729AB_M];        /* must follow past_qua_en[] */
    int16           prev_lsp[G729AB_M];       /* must follow lsp_old[] */
    int16           freq_prev[G729AB_MA_NP][G729AB_M];
    /* 3 elements for _G729AB_Dec_cng() (order is significant!) */
    vint            cur_gain;
    vint            sid_gain;            /* for _G729AB_Dec_cng() must folow cur_gain*/
    int16           lspSid[G729AB_M];    /* for _G729AB_Dec_cng() must folow sid_gain*/
    int16           old_exc[G729AB_PIT_MAX+G729AB_L_INTERPOL+G729AB_L_FRAME];
    /* 5 elements for _G729AB_Post_Filter() (order is significant!) */
    POST_FILT_Str   post_filt_str;
} G729AB_DecObj;

/* Function prototypes */
void G729AB_encode_init(
        G729AB_EncObj *encObj_ptr);

void G729AB_decode_init(
        G729AB_DecObj *decObj_ptr);

vint G729AB_encode(
        G729AB_EncObj *encObj_ptr,
        vint           speechIn[],
        int16          packedWords[]);

void G729AB_decode(
        G729AB_DecObj *decObj_ptr,
        int16          parameters[],
        vint           speechOut[],
        vint           nBytes,
        vint           bfi);


#endif
