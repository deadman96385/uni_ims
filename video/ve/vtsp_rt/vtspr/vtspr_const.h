/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004-2010 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 30256 $ $Date: 2014-12-08 17:30:17 +0800 (Mon, 08 Dec 2014) $
 */

#ifndef _VTSPR_CONST_H_
#define _VTSPR_CONST_H_

#include <osal.h>

/* 
 * Hardware interface
 */
#define VTSPR_INFC_TYPE_FXS   (1)      /* fxs interface type enumeration */
#define VTSPR_INFC_TYPE_FXO   (2)      /* fxo interface type enumeration */
#define VTSPR_INFC_TYPE_AUDIO (3)      /* handset audio infc type enumeration */

/* 
 * Real time task control
 *
 * WAITING : waiting for application to 'start' voice 
 * RUN : running normally
 * STOP : application control to 'stop' voice
 * FINISHED : task has exited
 */
#define VTSPR_TASK_WAIT            (0x02)
#define VTSPR_TASK_RUN             (0x04)
#define VTSPR_TASK_STOP            (0x08)
#define VTSPR_TASK_FINISHED        (0x10)

#define VTSPR_TASK_PRIORITY        (OSAL_TASK_PRIO_VTSPR)
#define VTSPR_TASK_STACK_SZ        (OSAL_STACK_SZ_LARGE)       

#define VTSPR_MULTI_ENCODE10_TASK_STACK_SZ    (OSAL_STACK_SZ_LARGE)
#define VTSPR_MULTI_ENCODE10_TASK_PRIORITY    (OSAL_TASK_PRIO_ENC10)
#define VTSPR_MULTI_ENCODE20_TASK_STACK_SZ    (OSAL_STACK_SZ_LARGE) 
#define VTSPR_MULTI_ENCODE20_TASK_PRIORITY    (OSAL_TASK_PRIO_ENC20)
#define VTSPR_MULTI_ENCODE30_TASK_STACK_SZ    (OSAL_STACK_SZ_LARGE)
#define VTSPR_MULTI_ENCODE30_TASK_PRIORITY    (OSAL_TASK_PRIO_ENC30)

#define VTSPR_MULTI_DECODE10_TASK_STACK_SZ    (OSAL_STACK_SZ_LARGE) 
#define VTSPR_MULTI_DECODE10_TASK_PRIORITY    (OSAL_TASK_PRIO_DEC10)
#define VTSPR_MULTI_DECODE20_TASK_STACK_SZ    (OSAL_STACK_SZ_LARGE)
#define VTSPR_MULTI_DECODE20_TASK_PRIORITY    (OSAL_TASK_PRIO_DEC20)
#define VTSPR_MULTI_DECODE30_TASK_STACK_SZ    (OSAL_STACK_SZ_LARGE)
#define VTSPR_MULTI_DECODE30_TASK_PRIORITY    (OSAL_TASK_PRIO_DEC30) 

/*
 * The following constants are associated with the 
 * MULTI_ENCODE and MUTLI_DECODE tasks.
 */
#define VTSPR_Q_MULTI_ENCODE_RAW_NUM_MSG    (4)
#define VTSPR_Q_MULTI_ENCODE_RAW_MSG_SIZE   (sizeof(_VTSPR_MultiRawMsg))
#define VTSPR_Q_MULTI_ENCODE_ENC_NUM_MSG    (4*_VTSP_STREAM_NUM)
#define VTSPR_Q_MULTI_ENCODE_ENC_MSG_SIZE   (sizeof(_VTSPR_MultiCodedMsg))
#define VTSPR_Q_MULTI_DECODE_PKT_NUM_MSG    (4)
#define VTSPR_Q_MULTI_DECODE_PKT_MSG_SIZE   (sizeof(_VTSPR_MultiPktMsg))
#define VTSPR_Q_MULTI_DECODE_DEC_NUM_MSG    (4*_VTSP_STREAM_NUM)
#define VTSPR_Q_MULTI_DECODE_DEC_MSG_SIZE   (sizeof(_VTSPR_MultiDecodedMsg))

/*
 * Macros for looping through interfaces dynamically.
 */
#define _VTSPR_FOR_ALL_FXS(i) \
    for((i)=_VTSP_INFC_FXS_LAST; (i)>=_VTSP_INFC_FXS_FIRST; (i)--)
#define _VTSPR_FOR_ALL_FXO(i) \
    for((i)=_VTSP_INFC_FXO_LAST; (i)>=_VTSP_INFC_FXO_FIRST; (i)--)
#define _VTSPR_FOR_ALL_FX(i) \
    for((i)=_VTSP_INFC_FXO_LAST; (i)>=_VTSP_INFC_FXS_FIRST; (i)--)
#define _VTSPR_FOR_ALL_AUDIO(i) \
    for((i)=_VTSP_INFC_AUDIO_LAST; (i)>=_VTSP_INFC_AUDIO_FIRST; (i)--)
#define _VTSPR_FOR_ALL_INFC(i) \
    for((i)=_VTSP_INFC_AUDIO_LAST; (i)>=_VTSP_INFC_FXS_FIRST; (i)--)
#define _VTSPR_FOR_ALL_INFC_PP(i) \
    for((i)=_VTSP_INFC_FXS_FIRST; (i)<=_VTSP_INFC_AUDIO_LAST; (i)++)

/*
 * The following constants define buffer sizes for stream processing.
 */
#ifdef VTSP_ENABLE_STREAM_16K
#define VTSPR_NSAMPLES_STREAM   (160)
#endif
#ifdef VTSP_ENABLE_STREAM_8K
#define VTSPR_NSAMPLES_STREAM   (80)
#endif

#define VTSPR_SAMPLING_RATE_16KHZ   (16)
#define VTSPR_SAMPLING_RATE_8KHZ    (80)

/*
 * The following constants define buffer sizes for encoding, decoding, and 
 * physical interface processing.
 * Also, used for audio processing of specific block sizes, which
 * can never be changed.
 */
#define VTSPR_NSAMPLES_10MS_48K         (480)
#define VTSPR_NSAMPLES_10MS_32K         (320)
#define VTSPR_NSAMPLES_10MS_24K         (240)
#define VTSPR_NSAMPLES_10MS_16K         (160)
#define VTSPR_NSAMPLES_10MS_8K          (80)

#ifndef VTSPR_NSAMPLES_10MS_MAX
#define VTSPR_NSAMPLES_10MS_MAX (VTSPR_NSAMPLES_10MS_16K)
#endif

#define VTSPR_MULTI_NSAMPLE_MAX         (3 * VTSPR_NSAMPLES_10MS_MAX)

#define VTSPR_MAX_CODED_BYTES   (160)   /* XXX check this */

#ifdef VTSP_ENABLE_ECSR
#define VTSPR_EC_TAILLENGTH  (EC_TAIL_LENGTH_16MS) /* EC tail length */
#define VTSPR_EC_FULLLEN     (EC_RIN_FULL_128MS)    /* EC full length */
#define VTSPR_EC_DLYLEN      (EC_DLY_BUF_128MS)     /* EC delay length */
#endif

#define VTSPR_FSKR_BUFLEN    (256)                 /* Max FSKR data length */

/*
 * Tones are specified at the end of the user programmable tone templates.
 * These tones are for use by the VTSPR framework only, and can not be
 * programmed by the user.
 */
#define VTSPR_TONE_LIST_SZ   (_VTSP_NUM_TONE_TEMPL + 1)
#define VTSPR_TONE_ID_CIDACK (VTSPR_TONE_LIST_SZ - 1)

/* Blocktimes until silence determines end of fax call */
#define VTSPR_FAX_TIMEOUT    (150)
/* VTSPR_FAX_SIL_DB = -250 = -25dB */
#define VTSPR_FAX_SIL_DB     (-350)  /* silence threshold, in -0.1 dB units */

/*
 * Bug 2073
 * FTMD voice component currently detects tones down to -33 dB
 * Do not change this constant.
 */
#define VTSPR_FMTD_POWER_MIN_DETECT_DEF     (-33)


#if defined(VTSP_ENABLE_AEC) || defined(VTSP_ENABLE_ECSR)
/*
 * !!!!!!!!!!!!!!!!!!!!!!  
 * !!! Important Note !!!
 * !!!!!!!!!!!!!!!!!!!!!!
 * The following constant defines how many buffers are required for time
 * allignment of Rin and Rout with Sin, because of delay in the VHW driver, from
 * tx_ptr to rx_ptr. Examples are tablulated below.
 * 
 * Measured delay between tx_ptr and rx_ptr     VTSPR_NALLIGNMENT_BUFS
 * 10 ms                                        (2)
 * 20 ms                                        (3)
 * 30 ms                                        (4)
 *
 * A unique setting is required for each PLATFORM, as set below.
 */
#if !defined(VTSPR_NALLIGNMENT_BUFS)
#error "Please define VTSPR_NALLIGNMENT_BUFS in make/system-$(PLATFORM)"
#endif

#endif /* defined(VTSP_ENABLE_AEC) || defined(VTSP_ENABLE_ECSR) */

/*
 * --------
 * The following VTSPR_ALG masks are used in the state variables:
 *
 * algChannelState
 * algStreamState
 *
 * --------
 */

/* 
 * Channel alg processing masks (32 bit)
 *
 * Design Note; Although DTMFR is processed as a stream decoder type, channel
 * state information is required since CIDS, CIDCWS, DTMFR, and tone generation
 * functions all share the same toneObj. The added DTMFR channel state
 * information helps resource manage the toneObj, with the following precedence:
 * 1) CIDS and CIDCWS
 * 2) DTMFR
 * 3) tone generation
 */
#define VTSPR_ALG_CHAN_ECSR      (0x00000001)
#define VTSPR_ALG_CHAN_ECSR_FRZ  (0x00000002)   /* freeze coeffs */
#define VTSPR_ALG_CHAN_ECSR_FMTD (0x00000004)   /* bypass on phase rev */
#define VTSPR_ALG_CHAN_NFE_INFC  (0x00000008)
#define VTSPR_ALG_CHAN_NLP       (0x00000010)
#define VTSPR_ALG_CHAN_MUTE_OUT  (0x00000020)   /* mute audio to interface */
#define VTSPR_ALG_CHAN_MUTE_IN   (0x00000040)   /* mute audio from interface */
#define VTSPR_ALG_CHAN_TONE      (0x00000080)   /* dual tone */
#define VTSPR_ALG_CHAN_DTMF      (0x00000100)   /* DTMF */
#define VTSPR_ALG_CHAN_FMTD      (0x00000200)
#define VTSPR_ALG_CHAN_CIDS      (0x00000400)   /* CID-Send processing (FXS) */
#define VTSPR_ALG_CHAN_CIDCWS    (0x00000800)   /* CID-Send processing (FXS) */
#define VTSPR_ALG_CHAN_CIDR      (0x00001000)   /* CID-Recv processing (FXO) */
#define VTSPR_ALG_CHAN_DR_DEC    (0x00002000)   /* DTMF Relay Decode */
#define VTSPR_ALG_CHAN_TESTTONE  (0x00004000)   /* Manual 1Khz tone */
#define VTSPR_ALG_CHAN_LOOPBACK  (0x00008000)   /* linear data loopback */
#define VTSPR_ALG_CHAN_NFE_PEER  (0x00010000)
#define VTSPR_ALG_CHAN_DCRM_PEER (0x00020000)
#define VTSPR_ALG_CHAN_T38       (0x00040000)
#define VTSPR_ALG_CHAN_FAX       (0x00080000)   /* FMTD detect 64k clear chan */
#define VTSPR_ALG_CHAN_UTD       (0x00100000)   /* UTD */
#define VTSPR_ALG_CHAN_CAS       (0x00200000)
#define VTSPR_ALG_CHAN_FSKR_ONH  (0x00400000)   /* FSKR ONHOOK */
#define VTSPR_ALG_CHAN_FSKR_OFH  (0x00800000)   /* FSKR OFFHOOK */
#define VTSPR_ALG_CHAN_AEC       (0x01000000)   /* split AEC from ECSR */
#define VTSPR_ALG_CHAN_TONE_QUAD (0x02000000)   /* GENF quad/modulated tone */

/* 
 * Stream alg masks (32-bit)
 */
#define VTSPR_ALG_STREAM_MUTE_RECV (0x000001)   /* from net mute */
#define VTSPR_ALG_STREAM_MUTE_SEND (0x000002)   /* to net mute */
#define VTSPR_ALG_STREAM_TONE      (0x000004)   /* stream tone */
#define VTSPR_ALG_STREAM_TONE_QUAD (0x000008)
#define VTSPR_ALG_STREAM_SID_D     (0x000010)
#define VTSPR_ALG_STREAM_SID_E     (0x000020)
#define VTSPR_ALG_STREAM_DTMFRELAY (0x000040)
#define VTSPR_ALG_STREAM_PLC       (0x000200)
#define VTSPR_ALG_STREAM_JB        (0x000800)  /* jitter buffer */
#define VTSPR_ALG_STREAM_LOOPBACK  (0x008000)  /* send = recv */
#define VTSPR_ALG_STREAM_T38       (0x010000)  /* T38 active on stream */

/*
 * Define Comfort noise threshold
 */
#define VTSPR_CN_THRESHOLD         (-30)     /* dBm */
#define VTSPR_CN_HYSTERESIS        (1)       /* dBm */
#if defined(PROVIDER_LGUPLUS)
#define VTSPR_CN_PWR_ATTEN_DEF     (-35)       /* dBm */
#else
#define VTSPR_CN_PWR_ATTEN_DEF     (0)       /* dBm */
#endif

/*
 * Audio processing defs
 */
#define VTSPR_CHANNELS      (_VTSP_INFC_NUM)  /* # audio channels */

/*
 * Debug Masks
 */
#define VTSPR_DB_AUDIOCAP_IN       (0x0001)        /* audio capture fromphone */
#define VTSPR_DB_AUDIOCAP_OUT      (0x0002)        /* audio capture tophone */


/*
 * VTSPR internal TONE control values
 */
#define VTSPR_TONE_DUAL             (0x0001)
#define VTSPR_TONE_QUAD             (0x0002)

/*
 * RTP Constants.
 */
#define _VTSPR_RTP_OK          (1)
#define _VTSPR_RTP_ERROR       (-1)
#define _VTSPR_RTP_NOTREADY    (-1)
#define _VTSPR_RTP_NOT_BOUND   (0)
#define _VTSPR_RTP_BOUND       (1)
#define _VTSPR_RTP_READY       (1)

#define _VTSPR_RTP_NOT_DTMF_PKT   (0xFFFFFFFF)
#define _VTSPR_RTP_MIN_SEQUENTIAL (2)
#define _VTSPR_RTP_MAX_MISORDER   (100)
#define _VTSPR_RTP_MAX_DROPOUT    (300)
#define _VTSPR_RTP_SEQ_MOD        (1 << 16)

/*
 * RTCP Constants
 */
#define _VTSPR_RTCP_TOS_VALUE     (0xB8)

/*
 * Set socket priority (TOS bits) to 10111000b = 0xB8 per bug 908.
 */
#define _VTSPR_RTP_IP_TOS  (0xB8)

/*
 * Encoder decoder.
 */
#define VTSPR_ENCODER (1)
#define VTSPR_DECODER (0)

/*
 * Flow States
 */
#define _VTSPR_FLOW_STATE_IDLE    (0)
#define _VTSPR_FLOW_STATE_ACTIVE  (1)
#define _VTSPR_FLOW_STATE_CLOSING (2)

/*
 * Size of the jitter buffer used by VTSPR is defined here
 */
#define _VTSPR_JB_MAXLEVEL         (JB_MAXLEVEL_MAX)
#define _VTSPR_JB_PKTSZ            (JB_PKTSZ_MIN)
#define _VTSPR_SEQN_DISP_MAX       (_VTSPR_JB_MAXLEVEL / _VTSPR_JB_PKTSZ)

/*
 * Size of the T38 Jitter Buffer allocated for 200ms jitter
 */
#define _VTSPR_FR38_JB_SZ   (FR38_JITTER_SPACE_20MS * (200 /* FR38_DEF_JITTER */ / 20))

#ifdef VTSP_ENABLE_T38
/*
 * Input/Output level for Fr38v3 for 0 dbm.
 */
#define _VTSPR_FR38V3_P0DB          (FR38_P0DB_DEF)
/*
 * Default value of FR38v3 CED length setting
 */
#define _VTSPR_FR38V3_CED_LENGTH    (FR38_CED_LENGTH_DEF)
/*
 * Size of FR38v3 modem context
 */
#define _VTSPR_FR38V3_MDM_SIZE      (FR38_MDM_SIZE_DEF)
#endif

/*
 * BENCHMARK Constants
 */
typedef enum {
    _VTSPR_BENCHMARK_VTSPR,
    _VTSPR_BENCHMARK_G729_ENCODE_ID0,
    _VTSPR_BENCHMARK_G729_DECODE_ID0,   
    _VTSPR_BENCHMARK_G729_ENCODE_ID1,   
    _VTSPR_BENCHMARK_G729_DECODE_ID1,
    _VTSPR_BENCHMARK_G711P1_ENCODE_ID0,
    _VTSPR_BENCHMARK_G711P1_DECODE_ID0,   
    _VTSPR_BENCHMARK_G711P1_ENCODE_ID1,   
    _VTSPR_BENCHMARK_G711P1_DECODE_ID1,
    _VTSPR_BENCHMARK_ILBC_ENCODE_ID0,
    _VTSPR_BENCHMARK_ILBC_DECODE_ID0,   
    _VTSPR_BENCHMARK_ILBC_ENCODE_ID1,   
    _VTSPR_BENCHMARK_ILBC_DECODE_ID1,  
    _VTSPR_BENCHMARK_G722P1_ENCODE_ID0,
    _VTSPR_BENCHMARK_G722P1_DECODE_ID0,   
    _VTSPR_BENCHMARK_G722P1_ENCODE_ID1,   
    _VTSPR_BENCHMARK_G722P1_DECODE_ID1,
    _VTSPR_BENCHMARK_GAMRNB_ENCODE_ID0,
    _VTSPR_BENCHMARK_GAMRNB_DECODE_ID0,   
    _VTSPR_BENCHMARK_GAMRNB_ENCODE_ID1,   
    _VTSPR_BENCHMARK_GAMRNB_DECODE_ID1,
    _VTSPR_BENCHMARK_GAMRWB_ENCODE_ID0,
    _VTSPR_BENCHMARK_GAMRWB_DECODE_ID0,
    _VTSPR_BENCHMARK_GAMRWB_ENCODE_ID1,
    _VTSPR_BENCHMARK_GAMRWB_DECODE_ID1,
    _VTSPR_BENCHMARK_SILK_ENCODE_ID0,
    _VTSPR_BENCHMARK_SILK_DECODE_ID0,
    _VTSPR_BENCHMARK_SILK_ENCODE_ID1,
    _VTSPR_BENCHMARK_SILK_DECODE_ID1,    
    _VTSPR_BENCHMARK_G723_ENCODE_ID0,
    _VTSPR_BENCHMARK_G723_DECODE_ID0,   
    _VTSPR_BENCHMARK_G723_ENCODE_ID1,   
    _VTSPR_BENCHMARK_G723_DECODE_ID1,    
    _VTSPR_BENCHMARK_G722_ENCODE_ID0,
    _VTSPR_BENCHMARK_G722_DECODE_ID0,   
    _VTSPR_BENCHMARK_G722_ENCODE_ID1,   
    _VTSPR_BENCHMARK_G722_DECODE_ID1,    
    _VTSPR_BENCHMARK_G726_ENCODE_ID0,
    _VTSPR_BENCHMARK_G726_DECODE_ID0,   
    _VTSPR_BENCHMARK_G726_ENCODE_ID1,   
    _VTSPR_BENCHMARK_G726_DECODE_ID1,
    _VTSPR_BENCHMARK_AEC_COMPUTE_ROUT,
    _VTSPR_BENCHMARK_AEC_COMPUTE_SOUT,
    _VTSPR_BENCHMARK_ECSR,
    _VTSPR_BENCHMARK_FMTD,               
    _VTSPR_BENCHMARK_DTMF,               
    _VTSPR_BENCHMARK_T38,                
    _VTSPR_BENCHMARK_TIC,                
    _VTSPR_BENCHMARK_RTP_XMIT,
    _VTSPR_BENCHMARK_RTP_RECV,           
    _VTSPR_BENCHMARK_NFE,           
    _VTSPR_BENCHMARK_DCRM_NEAR,
    _VTSPR_BENCHMARK_DCRM_PEER,
    _VTSPR_BENCHMARK_JB_GET,           
    _VTSPR_BENCHMARK_JB_PUT,
    _VTSPR_BENCHMARK_CIDS,
    _VTSPR_BENCHMARK_EVENTS,      
    _VTSPR_BENCHMARK_CMD,
    _VTSPR_BENCHMARK_TOTAL,
    _VTSPR_BENCHMARK_NUM                
} _VTSPR_BenchmarkType;

/* 
 * Multi-coder Task Constants
 */
#define _VTSPR_MULTI_CODER_CMD_SHUTDOWN  (256)
#define _VTSPR_MULTI_CODER_CMD_RUN       (1)
#define _VTSPR_MULTI_CODER_PKT_RATE_10   (10)
#define _VTSPR_MULTI_CODER_PKT_RATE_20   (20)
#define _VTSPR_MULTI_CODER_PKT_RATE_30   (30)

#endif
