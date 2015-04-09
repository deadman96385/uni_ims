/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004-2007 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * +D2Tech+ Rev: 4709  Date: 2014-06-20 10:33:22 -0700 (Fri, 20 Jun 2014) 
 * +D2Tech+ Release Version: d2fs1-jb_6_6
 */

#ifndef _JB_H_
#define _JB_H_

#include <comm.h>

//#define JB_DEBUG
//#define DEBUG_OFFLINE

#ifdef JB_DEBUG
#ifdef DEBUG_OFFLINE
#define JB_log HST_printf // for ALG_CORE
#else
#define JB_log OSAL_logMsg // for VTSPR
#endif
#else
#define JB_log
#endif

/*
 * Function return codes
 */
#define JB_OK           (1)
#define JB_ERR_MEM      (0)

/*
 * Sampling frequency for voice.
 */
#define JB_TS_INC_10MS  (80)
#define JB_TS_INC_WB    (JB_TS_INC_10MS << 1)

/*
 * Minimum and maximum packet sizes this module can handle. In samples.
 */
#define JB_PKTSZ_DEF    (80)
/* 
 * JB can handle a Max of 240ms of speech.
 * for 240ms of G711 speech, we need 24 * 80 = 1920 bytes.
 */
#define JB_PKTSZ_MAX    (1920)
#define JB_PKTSZ_MIN    (80)

/*
 * Maximum level to which the buffer can fill up in samples.
 */
#define JB_MAXLEVEL_DEF (1600) /* 0.2 seconds */
#define JB_MAXLEVEL_MAX (5600) /* 0.7 seconds for fax */
#define JB_MAXLEVEL_MIN (240)

/*
 * Leak rate and accumulate rate in packets.
 */
#define JB_LEAKRATE_DEF (500)    /* was 8 */
#define JB_LEAKRATE_MAX (2000)
#define JB_LEAKRATE_MIN (3)
#define JB_ACCMRATE_DEF (500)    /* was 8 */
#define JB_ACCMRATE_MAX (2000)
#define JB_ACCMRATE_MIN (2)

/*
 * Enveloping factor in Q.15. It is the JB size decay factor in pkts/32786.
 * The higher the value, the faster the decay rate.
 */
#define JB_DECAY_RATE_MIN  (16)   /* time = 2000 10 ms packets ~= 20. seconds */
#define JB_DECAY_RATE_MAX  (1638) /* 1/e time = 20 packets = 0.20 seconds */
#define JB_DECAY_RATE_DEF  (16)   /* decay time ~= 20 seconds (was 2 sec) */

/*
 * Macro to find size of Jitter Buffer's memory block
 */
#define JB_Q_ELEMENTS(maxLevel) (((maxLevel) / JB_PKTSZ_MIN) + 1)
#define JB_MEM_SIZE(maxLevel) ((JB_Q_ELEMENTS(maxLevel)) * (sizeof(JB_Pkt)))


/*
 * Flags for indicating that statistics in
 * SS Statistics Summary, arg1, are valid
 */
#define JB_STATS_JIT_OK    (0x400)
#define JB_STATS_DUP_OK    (0x200)
#define JB_STATS_LOST_OK   (0x100)

/*
 * Thresholds for Speech vs. NonSpeech
 * These two thresholds correspond to -45dBm and -42dBm, respectively.
 * They are valid for both mulaw and alaw because the _JB_mulaw2[]
 * and _JB_alaw2[] tables are scaled differently, by a factor of 4.
 */
#define JB_LOW_ENG  (2573)
#define JB_HIGH_ENG (5146)

/*
 * Define frame length each coder has, in 10 ms frames
 */
#define JB_G711_SZ      (1)
#define JB_G726_SZ      (1)
#define JB_G729_SZ      (1)
#define JB_G723_SZ      (3)
#define JB_ILBC20_SZ    (2)
#define JB_ILBC30_SZ    (3)
#define JB_GAMRNB_SZ    (2)
#define JB_GAMRWB_SZ    (2)
#define JB_SILK_SZ      (2)

#define JB_16K_MU_SZ    (1)
#define JB_G722_0_SZ    (1)
#define JB_G722_1_SZ    (2)
#define JB_G711P1_SZ    (1)
/*
 * Define octets in one frame of a coder type.
 * The coders that work on sample basis have frame size set to JB_PKTSZ_MIN.
 */
#define JB_G711_VOICE_SZ           (JB_PKTSZ_MIN)      /* G.711 uLaw/aLaw */
#define JB_G726_32_VOICE_SZ        (JB_PKTSZ_MIN >> 1) /* G.726 @ 32 kbps */
#define JB_G729_VOICE_SZ           (10)     /* G.729 Voice */
#define JB_G729_SID_SZ             (2)      /* G.729 SID */
#define JB_DTRLY_SZ                (4)      /* DTMF relay pkt sz */
#define JB_G723_SID_SZ             (4)      /* G.723 SID */
#define JB_G723_63_VOICE_SZ        (24)     /* G.723 @ 6.3 kbps */
#define JB_G723_53_VOICE_SZ        (20)     /* G.723 @ 5.3 kbps */
#define JB_ILBC20_VOICE_SZ         (38)     /* iLBC 20 ms */
#define JB_ILBC30_VOICE_SZ         (50)     /* iLBC 30 ms */
#define JB_16K_MU_VOICE_SZ         (160)    /* bytes in 10ms Mulaw @16K samps */
#define JB_G722_0_VOICE_SZ         (80)     /* see G722.h */
#define JB_G722_1_24_VOICE_SZ      (60)     /* see G722P1.h; only 1 frame/pkt */
#define JB_G722_1_32_VOICE_SZ      (80)     /* see G722P1.h */
/* AMR Octet-Align format */
#define JB_GAMRNB_OA_MRDTX_SID_SZ     (7)    /* GSM-AMR SID */
#define JB_GAMRNB_OA_MRDTX_NODATA_SZ  (2)    /* GSM-AMR NO DATA */
#define JB_GAMRNB_OA_MR475_VOICE_SZ   (14)   /* GSM-AMR @ 4.95 kbps */
#define JB_GAMRNB_OA_MR515_VOICE_SZ   (15)   /* GSM-AMR @ 5.15 kbps */
#define JB_GAMRNB_OA_MR59_VOICE_SZ    (17)   /* GSM-AMR @ 5.9  kbps */
#define JB_GAMRNB_OA_MR67_VOICE_SZ    (19)   /* GSM-AMR @ 6.7  kbps */
#define JB_GAMRNB_OA_MR74_VOICE_SZ    (21)   /* GSM-AMR @ 7.4  kbps */
#define JB_GAMRNB_OA_MR795_VOICE_SZ   (22)   /* GSM-AMR @ 7.95 kbps */
#define JB_GAMRNB_OA_MR102_VOICE_SZ   (28)   /* GSM-AMR @ 10.2 kbps */
#define JB_GAMRNB_OA_MR122_VOICE_SZ   (33)   /* GSM-AMR @ 12.2 kbps */
/* AMR Bandwidth-Efficient */
#define JB_GAMRNB_BE_MRDTX_SID_SZ     (7)    /* GSM-AMR SID */
#define JB_GAMRNB_BE_MRDTX_NODATA_SZ  (2)    /* GSM-AMR NO DATA */
#define JB_GAMRNB_BE_MR475_VOICE_SZ   (14)   /* GSM-AMR @ 4.95 kbps */
#define JB_GAMRNB_BE_MR515_VOICE_SZ   (15)   /* GSM-AMR @ 5.15 kbps */
#define JB_GAMRNB_BE_MR59_VOICE_SZ    (16)   /* GSM-AMR @ 5.9  kbps */
#define JB_GAMRNB_BE_MR67_VOICE_SZ    (18)   /* GSM-AMR @ 6.7  kbps */
#define JB_GAMRNB_BE_MR74_VOICE_SZ    (20)   /* GSM-AMR @ 7.4  kbps */
#define JB_GAMRNB_BE_MR795_VOICE_SZ   (22)   /* GSM-AMR @ 7.95 kbps */
#define JB_GAMRNB_BE_MR102_VOICE_SZ   (27)   /* GSM-AMR @ 10.2 kbps */
#define JB_GAMRNB_BE_MR122_VOICE_SZ   (32)   /* GSM-AMR @ 12.2 kbps */
/* AMR-WB Octet-Align format */
#define JB_GAMRWB_OA_MRDTX_SID_SZ     (7)    /* GSM-AMR-WB SID */
#define JB_GAMRWB_OA_MRDTX_NODATA_SZ  (2)    /* GSM-AMR NO DATA */
#define JB_GAMRWB_OA_MR660_VOICE_SZ   (19)   /* GSM-AMR-WB @ 6.60  kbps */
#define JB_GAMRWB_OA_MR885_VOICE_SZ   (25)   /* GSM-AMR-WB @ 8.85  kbps */
#define JB_GAMRWB_OA_MR1265_VOICE_SZ  (34)   /* GSM-AMR-WB @ 12.65 kbps */
#define JB_GAMRWB_OA_MR1425_VOICE_SZ  (38)   /* GSM-AMR-WB @ 14.25 kbps */
#define JB_GAMRWB_OA_MR1585_VOICE_SZ  (42)   /* GSM-AMR-WB @ 15.85 kbps */
#define JB_GAMRWB_OA_MR1825_VOICE_SZ  (48)   /* GSM-AMR-WB @ 18.25 kbps */
#define JB_GAMRWB_OA_MR1985_VOICE_SZ  (52)   /* GSM-AMR-WB @ 19.85 kbps */
#define JB_GAMRWB_OA_MR2305_VOICE_SZ  (60)   /* GSM-AMR-WB @ 22.05 kbps */
#define JB_GAMRWB_OA_MR2385_VOICE_SZ  (62)   /* GSM-AMR-WB @ 23.85 kbps */
/* AMR-WB Bandwidth-Efficient */
#define JB_GAMRWB_BE_MRDTX_SID_SZ     (7)    /* GSM-AMR-WB SID */
#define JB_GAMRWB_BE_MRDTX_NODATA_SZ  (2)    /* GSM-AMR-WB NO DATA */
#define JB_GAMRWB_BE_MR660_VOICE_SZ   (18)   /* GSM-AMR-WB @ 6.60  kbps */
#define JB_GAMRWB_BE_MR885_VOICE_SZ   (24)   /* GSM-AMR-WB @ 8.85  kbps */
#define JB_GAMRWB_BE_MR1265_VOICE_SZ  (33)   /* GSM-AMR-WB @ 12.65 kbps */
#define JB_GAMRWB_BE_MR1425_VOICE_SZ  (37)   /* GSM-AMR-WB @ 14.25 kbps */
#define JB_GAMRWB_BE_MR1585_VOICE_SZ  (41)   /* GSM-AMR-WB @ 15.85 kbps */
#define JB_GAMRWB_BE_MR1825_VOICE_SZ  (47)   /* GSM-AMR-WB @ 18.25 kbps */
#define JB_GAMRWB_BE_MR1985_VOICE_SZ  (51)   /* GSM-AMR-WB @ 19.85 kbps */
#define JB_GAMRWB_BE_MR2305_VOICE_SZ  (59)   /* GSM-AMR-WB @ 22.05 kbps */
#define JB_GAMRWB_BE_MR2385_VOICE_SZ  (61)   /* GSM-AMR-WB @ 23.85 kbps */

#define JB_G711P1_MODE_R3_VOICE_SZ (120)    /* G711.1 R3 @ 96 kbps */
#define JB_G711P1_MODE_R2_VOICE_SZ (100)    /* G711.1 R2(a/b) @ 80 kbps */
#define JB_G711P1_MODE_R1_VOICE_SZ (80)     /* G711.1 R1 @ 64 kbps */
#define JB_TONERLY_SZ              (12)     /* Tone relay pkt sz */
#define JB_SILK_VOICE_SZ_MAX       (250)    /* Max SILK packet sz */
#define JB_SILK_DTX_SZ             (0)      /* SILK DTX (SID)packet sz XXX??*/

/*
 * Define bit masks for flags member of the JB_Pkt structure
 * Bug 2947
 */
#define JB_PKT_FLAGS_MASK_WB_INC    (0x0001)    /* increment 160 every 10ms */
/* Define MASK for DTMFR sampling rate. */
#define JB_PKT_FLAGS_MASK_DTMFR_8K  (0x0002)
#define JB_PKT_FLAGS_MASK_DTMFR_16K (0x0004)

typedef enum {
  JB_FALSE = 0,
  JB_TRUE  = 1
} JB_Bool;

/*
 * Jitter buffer size adaptation or freeze.
 */
typedef enum {
  JB_ADAPT_RUN   = 0,          /* normal adaptive mode for speech */
  JB_FREEZE_SIZE = 1,          /* jbSize will not change */
  JB_FIXED       = 2           /* stop all leaks and accums, for G711 fax */
} JB_Adap;

/*
 * Values for 'reason' in JB_getStats(), according to IETF rtp RFCs
 */
typedef enum {
  JB_GETSTATS_REASON_RR,       /* Receiver Report */
  JB_GETSTATS_REASON_SR,       /* Sender Report */
  JB_GETSTATS_REASON_SS,       /* Statistics Summary */
  JB_GETSTATS_REASON_MR,       /* Metrics Report */
  JB_GETSTATS_REASON_CS,       /* Cumulative Summary */
  JB_GETSTATS_REASON_XR        /* eXtended Report NOT SUPPORTED */
} JB_Reason;

/*
 * DTMF relay support at init.
 */
#define JB_DTRLY_DISABLE  JB_FALSE
#define JB_DTRLY_ENABLE   JB_TRUE
#define JB_PKT_INVALID    JB_FALSE  /* Invalid location */
#define JB_PKT_VALID      JB_TRUE   /* Valid packet of specified type */
    
/*
 * Define known payload types.
 * Typical numbers from the RFC.
 */
typedef enum {
    JB_PT_G711U     = 0,    /* G.711 uLaw */
    JB_PT_G726_32   = 2,    /* G.726 @ 32 Kbps */
    JB_PT_G723      = 4,    /* G.723.1 */
    JB_PT_G711A     = 8,    /* G.711 aLaw */
    JB_PT_G722_0    = 9,    /* G.722 */
    JB_PT_CNSE      = 13,   /* Comfort noise */
    JB_PT_G729      = 18,   /* G.729AB */
    JB_PT_G711P1A   = 96,   /* G.711.1 aLaw */
    JB_PT_G711P1U   = 97,   /* G.711.1 uLaw */
    JB_PT_ILBC      = 98,   /* iLBC, dynamic type */
    JB_PT_16K_MU    = 99,   /* D2 only for 16K sampling Mu-law */
    JB_PT_TONERLY   = 100,  /* Tone relay, dynamic type RFC4733 */
    JB_PT_DTRLY     = 101,  /* DTMF relay, dynamic type RFC4733 */
    JB_PT_G722_1    = 103,  /* G.722.1 */
    JB_PT_GAMRWB_OA = 104,  /* GAMRWB octet-align */
    JB_PT_GAMRWB_BE = 105,  /* GAMRWB bandwidth-efficient */
    JB_PT_GAMRNB_OA = 106,  /* GAMRNB octet-align */
    JB_PT_GAMRNB_BE = 107,  /* GAMRNB bandwidth-efficient */
    JB_PT_SILK_8K   = 108,  /* SILK 8K  */
    JB_PT_SILK_16K  = 109, /* SILK 16K */
    JB_PT_SILK_24K  = 110, /* SILK 24K */
} JB_Payload;

/*
 * This is the DTMF/Tone relay packet to be placed in the payload of the JB_Pkt.
 */
typedef struct {
    int32   tsPast;        /* timestamp @ start, not for user */
    int32   tsFutr;        /* timestamp @ end, not for user */
    int32   end;           /* Is this the end of playing DTMF tone ? */
    int16   modFreq;       /* Modulation Frequency */
    int16   freq1;         /* frequency 1 */
    int16   freq2;         /* frequency 2 */
    int16   freq3;         /* frequency 3 */
    int16   freq4;         /* frequency 4 */
    JB_Bool divMod3;       /* divide mod freq by 3 */
    int8    pwr;           /* Power of digit in dBm */
    int8    evt;           /* Event in ASCII. */
    JB_Bool eventType;     /* 1: event type (i.e. DTMF), 0: tone type */  
} JB_DtmfPkt;

/*
 * JB_MsgRtp is the type of data structure filled in by JB_getStats()
 */
typedef struct {
    JB_Reason reason; /* input to JB_getStats: */
    uvint  ssrc;      /* not used; only for compatibility with VTSP_EventMsg */
    uvint  streamId;  /* not used; only for compatibility with VTSP_EventMsg */
                      /* some of the outputs below may be left unchanged */
    uint32 arg1;      /* output; meaning depends on input value 'reason' */
    uint32 arg2;      /* output; meaning depends on input value 'reason' */
    uint32 arg3;      /* output; meaning depends on input value 'reason' */
    uint32 arg4;      /* output; meaning depends on input value 'reason' */
    uint32 arg5;      /* output; meaning depends on input value 'reason' */
    uint32 arg6;      /* output; meaning depends on input value 'reason' */
    uint32 arg7;      /* output; meaning depends on input value 'reason' */
    uint32 arg8;      /* output; meaning depends on input value 'reason' */
    uint32 arg9;      /* output; not currently used */
} JB_MsgRtp;   /* name chosen for compatibility with VTSP_EventMsg */

/*
 * Packet: 1st 6 members and payload[] prepared from RTP.
 *       : Other members used internally.
 * Note that for wideband packets, payload[], ts, and pSizeSamps
     all are in units of samples, which is 16K/s for WB and 8K/s for NB.
 * In contrast, units of atime do not vary between WB and NB.
 */
typedef struct {
    uint32     ts;                    /* Time Stamp, increments by samples */
    uint32     tsOrig;                /* Original Time Stamp, increments by samples */
    JB_Bool    valid;                 /* Valid flag */
    uint32     atime;                 /* Packet arrival time, local clock */
    JB_Payload type;                  /* Packet type */
    uint16     seqn;                  /* Sequence number */
    uint16     pSize;                 /* Packet size in octets */

    uint16     location;              /* Location of valid data in payload */
    uint16     pSizeSamps;            /* Packet size in samples, 160 or 80 */
    uint16     flags;
#ifdef OSAL_WINCE
    __declspec(align(4))    uint8       payload[JB_PKTSZ_MAX];
#else
    uint8      payload[JB_PKTSZ_MAX] __attribute__((aligned(4))); /* octets */
#endif
} JB_Pkt;

/*
 * Init params of jitter buffer
 */
typedef struct {
    uint16  leakRate;  /* Leak rate in packets */
    uint16  accmRate;  /* Accumulate rate in packets */
    uint16  initLevel; /* Level to which the buffer will initially be filled */
    uint16  maxLevel;  /* Max jitter buffer size */
    uint16  decayRt;   /* High envelop decay rate */
    JB_Bool eDtRly;    /* Enable or disable DTMF relay */
    JB_Adap eFreeze;   /* adaptation of JB size, and leak/accm */
} JB_Params;

/*
 * JB_Stat for diagnositics
 * Written, but never read, by JB_getPkt() and JB_putPkt.
 * Read by JB_getStats(), reset to zero by JB_resetStats().
 * Read by event processing outside the JB module.
 * Bug 3435 modifications to this structure.
 */
typedef struct {
    uint32  plcs;          /* # PLCs to be done */
    uint32  drops;         /* # packets dropped */
    uint32  leaks;         /* # packets deleted during LEAK state */
    uint32  skips;         /* # packets skipped while waiting in a gap */
    uint32  accms;         /* # packets added to fill the buffer */
    uint32  dups;          /* # packets that arrive with duplicate seqn */
    uint32  resets;        /* # times JB has been re-initialized */
    int32   npjit;         /* number of packets analyzed for jitter */
    uint32  sumjit;        /* sum of jitter analyzed (t.s. units) */
    uint32  maxjit;        /* maximum jitter (t.s. units) */
    uint32  minjit;        /* minimum jitter (t.s. units), positive only */
    uint32  n1var;         /* sum of [jitter squared] - npjit*meanjit^2 */
    uint32  seqnOldest;    /* oldest sequence number since JB_resetStats() */
    uint32  seqnNewest;    /* newest sequence number since JB_resetStats() */
    uint32  putPktCnt;     /* calls to JB_putPkt() since JB_resetStats() */
    uint32  getPktCnt;     /* calls to JB_putPkt() since JB_resetStats() */
} JB_Stat;

/*
 * JB Object.
 */
typedef struct {
    /* 
     * jitter buffer indices
     */
    int32   maxLocation;   /* Maximum location */
    int32   dLocation;     /* Current DSP packet location */
    int32   state;         /* State of JB */
    int32   level;         /* Current level of JB, in 1/8000 seconds */
    int32   eTs;           /* Expected timestamp, in 1/8000 seconds */
    int32   pht;           /* packet hold time, in 1/8000 seconds */
    int32   dt;            /* Interarrival jitter */

    int32   leakCount;     /* Counter for leak */
    int32   accmCount;     /* Counter for accumulate */
    int32   ploss;         /* To detect a sequence of lost packets */

    int32   hiEnv;         /* Jitter high envelope */
    int32   maxm;          /* Jitter high */ 
    int32   jbSize;        /* Adapted JB size */
    /* 
     * jitter buffer times and sequence numbers
     */
    int32   timeM1;        /* Last arrival time */
    int32   timeLast;      /* Time from last packet */
    int32   timeOvfl;      /* Time overflow count */
    int32   tsM1;          /* Last time stamp, in 1/8000 seconds */
    int32   tsLast;        /* Last timestamp, in 1/8000 seconds */
    int32   oldTs;         /* Last RTP packet timestamp */
    int32   tsOvfl;        /* Timestamp overflow  */

    int32   seqnMax;       /* Max seq. number arrived yet */
    int32   seqnLast;      /* Sequence number from last packet */
    int32   oldSeqn;       /* Last RTP packet seqn */
    int32   seqnOvfl;      /* Sequence number overflow count */
    /*
     * Special logic for G722 rtp timestamp interop problem, Bug 3125
     */
    uint16      g722_seqnLast;
    uint32      g722_tsLast;
    JB_Payload  g722_typeLast;
    uint16      g722_wbIncFlag;

    /* 
     * long-term history for post-mortem
     */
    JB_Stat stats;         /* container for diagnostics */
    uint32  pkt_all_recd;  /* packets received since start of transmission */
    uint32  pkt_all_put;   /* packets placed since start of transmission */
    /* 
     * recent jitter buffer history
     */
    int32   szSamp;        /* Previous packet size, samples (was pSizeM1) */
    JB_Payload lastType;   /* Type of the last packet received */
    int32   lastPktTs;     /* timestamp (in samples) of last packet rec'd */
    int32   lastTsInc;     /* timestamp incr of prev voice packet rec'd */
    int32   lastPSize;     /* Packet size of the last packet received */
    JB_Bool lastSid;       /* Was the last packet rec'd an SID packet? */
    JB_Bool lastDtmf;      /* Was the last packet rec'd a DTMF relay packet? */
    JB_Bool lastDrawnSid;  /* Was the last packet drawn an SID packet? */
    JB_Bool lastDrawnDtmf; /* Was the last packet drawn a DTRLY packet? */
    int32   dtmfEvt;       /* used with lastDrawnDtmf */
    int32   dtmfPwr;       /* used with lastDrawnDtmf */
    int32   toneModFreq;   /* Used with lastDrawnDtmf */
    int32   toneDivMod3;   /* Used with lastDrawnDtmf */
    int32   toneFreq1;     /* Used with lastDrawnDtmf */
    int32   toneFreq2;     /* Used with lastDrawnDtmf */
    int32   toneFreq3;     /* Used with lastDrawnDtmf */
    int32   toneFreq4;     /* Used with lastDrawnDtmf */
    JB_Bool dspDraw;       /* is DSP allowed to draw packet? */
    JB_Bool wasNonSpeech;  /* user setable, to indicate silence signal */
    int32   multiDraw;     /* number of frames left in ILBC or G723 pkts */
    /* 
     * jitter buffer params, mostly from JB_init()
     */
    int32   shiftFac;      /* Shift factor for increasing precision */
    int32   decayRt;       /* decay rate for envelope calculation */
    int32   initLevel;     /* Level the buffer is initially filled to */
    int32   maxLevel;      /* Max size allowed for JB */
    int32   pktMax;        /* Max packets allowed */
    int32   maxJit;        /* Maximum jitter allowed */
    int32   leakRate;      /* Leak rate in packets */
    int32   accmRate;      /* Accumulate rate in packets */
    JB_Adap freeze;        /* Freeze jitter adaptation */
    JB_Bool eDtRly;        /* Enable (JB_TRUE) or disable DTMF relay */
    JB_Pkt *pktStart_ptr;  /* Pointer to first packet in buffer */
    int32   firstTime;     /* Local arrival time of 1st packet */
    int32   firstSeqn;     /* Seq. num of 1st packet */
    int32   firstSqStat;   /* Seq. num of 1st packet, but not reset */
    int32   firstTs;       /* Time stamp of 1st packet */
    JB_Bool firstPkt;      /* Indicates the first packet isn't processed yet */
} JB_Obj;

/*
 * Function prototypes.
 */

/*
 * Init the JB. Must be called prior to every new call or connection.
 */
int JB_init(
    JB_Obj      *obj_ptr,      /* JB object pointer */
    JB_Params   *params_ptr);  /* Parameters pointer */

/*
 * Place a packet into the jitter buffer.
 */
void JB_putPkt(
    JB_Obj    *obj_ptr,      /* JB object pointer */
    JB_Pkt    *pkt_ptr);     /* Packet pointer (input) */

/*
 * Extract a packet from the jitter buffer.
 * pkt_ptr must point to an empty JB_Pkt type structure.
 * pkt_ptr will be populated by the module with new packet parameters.
 */
void JB_getPkt(
    JB_Obj    *obj_ptr,      /* JB object pointer */
    JB_Pkt    *pkt_ptr);     /* Packet pointer (output) */

/*
 * Extract a packet from the jitter buffer.
 * pkt_ptr must point to an empty JB_Pkt type structure.
 * pkt_ptr will be populated by the module with new packet parameters.
 */
vint JB_getStats(
    JB_Obj    *obj_ptr,      /* pointer to JB object to get stats from */
    JB_MsgRtp *msgRtp_ptr);  /* pointer to I/O struct */

/*
 * Reset statistics counters needed for JB_getStats(), JB_GETSTATS_REASON_SS
 */
void JB_resetStats(
    JB_Obj    *obj_ptr);     /* pointer to JB object to have stats reset */

#endif /* _JB_H_ */

