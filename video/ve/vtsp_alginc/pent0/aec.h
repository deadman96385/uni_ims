/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES CONFIDENTIAL AND
 * PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2006-2007 D2 TECHNOLOGIES ALL RIGHTS RESERVED"
 *
 * +D2Tech+ Rev: 350  Date: 2010-04-29 15:47:38 -0700 (Thu, 29 Apr 2010) 
 * +D2Tech+ Release Version: trunk-aec
 *
 */

#ifndef _AEC_H_
#define _AEC_H_

#include <d2types.h>
#include <bnd.h>
#include <nse.h>

/* 
 * Initialization modes for AEC_init()
 */
#define AEC_INIT_COLD                (0)
#define AEC_INIT_WARM                (1)

/* 
 * Constant definitions for AEC_Obj.state
 */
#define AEC_STATE_IDLE               (0)
#define AEC_STATE_NEAR_ACTIVE        (1)
#define AEC_STATE_FAR_ACTIVE         (2)
#define AEC_STATE_DBL_TALK           (3)

/* 
 * Constant definitions for AEC_Obj.nearActive
 */
#define AEC_NEAR_ACTIVE_IDLE         (0)
#define AEC_NEAR_ACTIVE_SPEECH       (1)

/* 
 * Constant definitions for bits in AEC_Obj.control
 */
#define AEC_CONTROL_AGC_ROUT_BYPASS   (0x0001) /* if 1, bypass Rout gain */
#define AEC_CONTROL_AGC_SOUT_BYPASS   (0x0002) /* if 1, bypass Sout gain */
#define AEC_CONTROL_AEC_BYPASS        (0x0004) /* if 1, bypass echo canceller */
#define AEC_CONTROL_AEC_FREEZE        (0x0008) /* if 1, freeze AEC adaptation */
#define AEC_CONTROL_AEC_COMFORT_NOISE (0x0010) /* if 1, mix CN on Sout */
#define AEC_CONTROL_AEC_HALF_DUPLEX   (0x0020) /* if 1, half-duplex mode */

enum AEC_SAMPLE_RATE {
    AEC_SAMPLE_RATE_8K  = 0,
    AEC_SAMPLE_RATE_16K = 1
};

/*
 * Macros for specifying buffer sizes
 *    'len' is the length available for the echo tail in milliseconds,
 *         from 8 ms to AEC_ECHO_TAIL_MAX, in steps of 8 ms.
 *    'rate' is either of two values, 
 *         AEC_SAMPLE_RATE_8K (0)     or      AEC_SAMPLE_RATE_16K (1)
 */
#define AEC_NSAMPLES(rate)           ((rate)==AEC_SAMPLE_RATE_8K?80:160)

#define AEC_ECHO_TAIL_MAX            (128)   /* max filter length, ms */

#define AEC_LIMIT_ECHO_TAIL(len) \
     (((len)>AEC_ECHO_TAIL_MAX)?AEC_ECHO_TAIL_MAX:(((len)<8)?8:(len)))

#define AEC_FILTER_BUFSZ(len,rate) \
     (AEC_LIMIT_ECHO_TAIL(len) * (rate << 1) * 8)

#define AEC_HIST_BUFSZ(len,rate)    \
     (AEC_NSAMPLES(rate) + (2*AEC_FILTER_BUFSZ(len,rate)))

#define AEC_NSAMPLES_MAX           (AEC_NSAMPLES(AEC_SAMPLE_RATE_16K))
#define AEC_FILTER_BUFSZ_MAX       \
      (AEC_FILTER_BUFSZ(AEC_ECHO_TAIL_MAX,AEC_SAMPLE_RATE_16K))
#define AEC_HIST_BUFSZ_MAX         \
      (AEC_HIST_BUFSZ(AEC_ECHO_TAIL_MAX,AEC_SAMPLE_RATE_16K))


/*
 * Constants for default parameters
 */
#define AEC_pECHO_TAIL_LENGTH_DEF    (8)            /* 8ms echo tail */
#define AEC_pNEAR_DELAY_DEF          (16)           /* samples */
#define AEC_pNEAR_ERL_DEF            (0)            /* dB */
#define AEC_pDTD_THRESHOLD_DEF       (27000)        /* 0.80 Q15 */
#define AEC_pROUT_AGC_MAX_DBL_DEF    (-6)           /* dB */
#define AEC_pSOUT_AGC_MAX_DBL_DEF    (-6)           /* dB */
#define AEC_pSOUT_AGC_MAX_FAR_DEF    (-18)          /* dB */
#define AEC_pSIDETONE_GAIN_DEF       (-24)          /* dB */
#define AEC_pSIDETONE_GAIN_DISABLE   (-50)          /* use no sidetone */

#define AEC_pNOISE_DEF               (BND_pNOISE_THRESH_DEF)
#define AEC_tSUSTAIN_DEF             (BND_tSUSTAIN_DEF)
#define AEC_tHOLDOVER_DEF            (BND_tHOLDOVER_DEF)

typedef struct {
    uvint pSAMPLE_RATE;      /* user-specified sample rate; 0=>8K or 1=>16K */
    uvint pECHO_TAIL_LENGTH; /* user-specified echo tail; 8 ms up to 32 ms */
    uvint pNEAR_END_DELAY;   /* minimum acoustic delay, in milliseconds */
    uvint pNEAR_END_ERL;     /* in dB, XXX not used yet */
    vint  pDTD_THRESHOLD;    /* Q15 decision point for double talk */
    vint  pROUT_AGC_MAX_DBL; /* in dB, max AGC on Rout during double-talk */
    vint  pSOUT_AGC_MAX_DBL; /* in dB, max AGC on Sout during double-talk */
    vint  pSOUT_AGC_MAX_FAR; /* in dB, max AGC on Sout during Far-end  */
    vint  pSIDETONE_GAIN;    /* in dB, user specified sidetone gain */
    /* for BND params, to be set for bnd_sin */
    vint  pSIN_NOISE_THRESH;
    uvint tSIN_SUSTAIN;
    uvint tSIN_HOLDOVER;
    /* for BND params, to be set for bnd_rin */
    vint  pRIN_NOISE_THRESH;
    uvint tRIN_SUSTAIN;
    uvint tRIN_HOLDOVER;
} AEC_Params;

/*
 * Internal Object:
 */
typedef struct {
    char internal_ary[0x824c];
} _AEC_Internal;

typedef struct {
    vint         *rin_ptr;
    vint         *rout_ptr;
    vint         *sin_ptr;
    vint         *sout_ptr;
    vint         *filter_ptr;
    vint         *rinHist_ptr;
    uvint         control;
    uvint         nearActive;
    uvint         state;
    vint          noiseFloor;
    _AEC_Internal internal;
} AEC_Obj;
 
void AEC_init(
    AEC_Obj       *aec_ptr,
    GLOBAL_Params *global_ptr,
    AEC_Params    *params_ptr,
    vint           initMode);

void AEC_computeSout(
    AEC_Obj *aec_ptr);

void AEC_computeRout(
    AEC_Obj *aec_ptr);


#endif /* _AEC_H_ */
