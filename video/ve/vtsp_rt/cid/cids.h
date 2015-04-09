/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 12522 $ $Date: 2010-07-14 06:52:44 +0800 (Wed, 14 Jul 2010) $
 */

#include "osal_types.h"
#include "comm.h"
#include "fsks.h"
#include "tone.h"

#ifndef _CIDS_H_
#define _CIDS_H_

/*
 * The block size for the module
 */
#define CIDS_BLOCK_SIZE   (COMM_10MS_LEN)

/*
 * Data transmission type: FSK send or DTMF send
 */
#define CIDS_DATA_FSK       (1)
#define CIDS_DATA_DTMF      (2)

/*
 * CIDS_run() return state.
 */

/*
 * All done.
 */
#define CIDS_DONE         (0x0000)

/*
 * Answer: CIDS is waiting for answer (user offhook).
 */
#define CIDS_WAITING      (0x0400)

/*
 *
 * Status flag bits:
 * These can also be called line state flags.
 * 
 */

/*
 * Idle state status flag
 */
#define CIDS_STAT_IDLE    (0x0000)

/*
 * Ringing status flag:
 * When this flag is set, user must ring the phone. When this flag is not set,
 * user must stop ringing the phone.
 */
#define CIDS_STAT_RING    (0x0001)

/*
 * Transmission status flag:
 * When this flag is set, user must put phone in xmit state and play the data
 * in CIDS_Obj.dst_ptr[] to line. When this flag is not set, user must not put
 * the data on line and can take line to any state.
 */
#define CIDS_STAT_XMIT    (0x0002)

/*
 * Battery reversal status flag:
 * When this flag is set, user must toggle battery polarity.
 * This flag must be checked after every call to CIDS_run().
 */
#define CIDS_STAT_BATTREV (0x0004)

/*
 * Hook state status flags:
 * If the line is in onhook or offhook state.
 */
#define CIDS_STAT_OFFHOOK       (0x0008)

/*
 * Short rings status only for Japanese caller id.
 */
#define CIDS_STAT_RING_SHORT    (0x0010)

/*
 * Control bits to shut off seize and release in Japanese CID state .
 */
#define CIDS_STAT_SEIZE_OFF     (0x0040)
#define CIDS_STAT_REL_OFF       (0x0080)
#define CIDS_STAT_ABORT         (0x0100)

/*
 * Status flag transitioned to new status
 */
#define CIDS_STAT_CHANGED (0x8000)

/*
 * Control word values:
 * Must be set before every call to run function.
 */
#define CIDS_CTRL_IDLE    (0x0000)
#define CIDS_CTRL_TIMEOUT (0x0001)
#define CIDS_CTRL_ONHOOK  (0x0008)
#define CIDS_CTRL_OFFHOOK (0x0010)
#define CIDS_CTRL_TX      (0x0020)


/*
 * 
 * Typical, minimum, and maximum local parameters values:
 * 
 */

/*
 * Pre-ONHT signal type (STAS, RPAS(ring), DTAS etc.):
 * User must set this flag to indicate signal type pre-ONHT.
 * RPAS follows Bellcore. STAS/DTAS follows NTT.
 */
#define CIDS_SIG_NONE       (0x0000)
#define CIDS_SIG_RPAS       (0x0001)
#define CIDS_SIG_STAS       (0x0002)
#define CIDS_SIG_DTAS       (0x0003)
#define CIDS_SIG_SHORT_RING (0x0004)

/*
 * CIDS checksum/CRC type in local params:
 */
#define CIDS_CHECKSUM     (0)
#define CIDS_CRC          (1)

/*
 * Various message types. Used for message assembly.
 * Message format (MMDF):
 *  7 6 5 4 3 2 1 0   -> bit#
 *  _______________
 * | Message Type  |  -> message 0 start
 * | Message Len   | 
 * | Param Type    |  -> parameter 0 start
 * | Param Len     |
 * | Params ...    |
 * | Param Type    |  -> parameter 1 start
 * | ...           |
 * | Message Type  |  -> message 1 start
 * | Message Len   | 
 * | Param Type    |  -> parameter 0 start (of message 1)
 * | ...           |
 * | Checksum      |
 *  ---------------
 */
#define CIDS_MSG_TYPE_CLIP           (0x80)

/*
 * Various CLIP message type parameter types.
 * Used for message assembly.
 */
#define CIDS_CLIP_PARAM_CALL_TYPE    (0x11)
#define CIDS_CLIP_PARAM_TIME_DATE    (0x01)
#define CIDS_CLIP_PARAM_CALLING_DN   (0x02)
#define CIDS_CLIP_PARAM_CALLED_DN    (0x03)
#define CIDS_CLIP_PARAM_RFA_DN       (0x04)
#define CIDS_CLIP_PARAM_CALLER_NAME  (0x07)
#define CIDS_CLIP_PARAM_RFA_NAME     (0x10)
#define CIDS_CLIP_PARAM_NETWORK_STAT (0x13)

/*
 * These local parameter defaults for e.g. USA, Canada, Australia, V.23.
 *
 * -1 for OFFTIMEOUT or ONTIMEOUT means timeout/hook-change is not used
 */
#define CIDS_LOCALS_BATTREV_DEF      (0)
#define CIDS_LOCALS_STARTTIME_DEF    (0)
#define CIDS_LOCALS_SIGNALTIME_DEF   (200)      /* in blocktimes */
#define CIDS_LOCALS_PRESEZTIME_DEF   (500)
#define CIDS_LOCALS_SEZLEN_DEF       (300)
#define CIDS_LOCALS_MARKLEN_DEF      (180)
#define CIDS_LOCALS_ENDTIME_DEF      (50)       /* in blocktimes */
#define CIDS_LOCALS_OFFTIMEOUT_DEF   (-1)
#define CIDS_LOCALS_ONTIMEOUT_DEF    (-1)
#define CIDS_LOCALS_ONSILTIME_DEF    (0)
#define CIDS_LOCALS_SPACEFREQ_DEF    (2200)
#define CIDS_LOCALS_MARKFREQ_DEF     (1200)
#define CIDS_LOCALS_MARKPWR_DEF      (-27)
#define CIDS_LOCALS_SPACEPWR_DEF     (-27)
#define CIDS_LOCALS_TONE1FREQ_DEF    (2130)
#define CIDS_LOCALS_TONE2FREQ_DEF    (2750)
#define CIDS_LOCALS_TONE1PWR_DEF     (-30)
#define CIDS_LOCALS_TONE2PWR_DEF     (-30)
#define CIDS_LOCALS_CTYPE_DEF        (CIDS_CHECKSUM)
#define CIDS_LOCALS_SIGNALTYPE_DEF   (CIDS_SIG_RPAS)
#define CIDS_LOCALS_RINGTIME_DEF     (0) 

/*
 * These local parameter defaults for e.g. UK. Note, variations.
 * DTMF indicates UK tuning required for DTMF caller ID.
 */
#define CIDS_LOCALS_BATTREV_UK          (1)
#define CIDS_LOCALS_STARTTIME_UK        (100)
#define CIDS_LOCALS_SIGNALTIME_UK       (100)
#define CIDS_LOCALS_PRESEZTIME_UK       (100)
#define CIDS_LOCALS_SEZLEN_UK           (300)
#define CIDS_LOCALS_MARKLEN_UK          (55)
#define CIDS_LOCALS_ENDTIME_UK          (500)
#define CIDS_LOCALS_ENDTIME_UK_DTMF     (3000)
#define CIDS_LOCALS_OFFTIMEOUT_UK       (-1)
#define CIDS_LOCALS_ONTIMEOUT_UK        (-1)
#define CIDS_LOCALS_ONSILTIME_UK        (0)
#define CIDS_LOCALS_SPACEFREQ_UK        (2100)
#define CIDS_LOCALS_MARKFREQ_UK         (1300)
#define CIDS_LOCALS_MARKPWR_UK          (-40)
#define CIDS_LOCALS_SPACEPWR_UK         (-40)
#define CIDS_LOCALS_TONE1FREQ_UK        (2110)
#define CIDS_LOCALS_TONE2FREQ_UK        (2745)
#define CIDS_LOCALS_TONE1PWR_UK         (-60)
#define CIDS_LOCALS_TONE2PWR_UK         (-46)
#define CIDS_LOCALS_CTYPE_UK            (CIDS_CHECKSUM)
#define CIDS_LOCALS_SIGNALTYPE_UK       (CIDS_SIG_DTAS)
#define CIDS_LOCALS_SIGNALTYPE_UK_DTMF  (CIDS_SIG_RPAS)
#define CIDS_LOCALS_RINGTIME_UK         (0) 

/*
 * These local parameter defaults for e.g. Japan. Note, variations.
 */
#define CIDS_LOCALS_BATTREV_JP       (1)
#define CIDS_LOCALS_STARTTIME_JP     (150)   /* 150 ms */
#define CIDS_LOCALS_SIGNALTIME_JP    (5850)  /* NTT Spec is 6000ms = 5850ms(SIGNALTIME) + 150ms(STARTTIME) */
#define CIDS_LOCALS_PRESEZTIME_JP    (150)   /* 150 ms */
#define CIDS_LOCALS_SEZLEN_JP        (0)
#define CIDS_LOCALS_MARKLEN_JP       (100)
#define CIDS_LOCALS_ENDTIME_JP       (2000)  /* 2 seconds after FSK is sent */
#define CIDS_LOCALS_OFFTIMEOUT_JP    (150)   
#define CIDS_LOCALS_ONTIMEOUT_JP     (5000)  /* NTT Spec is 7000ms = 5000ms(ONTIMEOUT + 2000ms(ENDTIME) */
#define CIDS_LOCALS_ONSILTIME_JP     (500)   /* 500 ms */
#define CIDS_LOCALS_SPACEFREQ_JP     (2100)
#define CIDS_LOCALS_MARKFREQ_JP      (1300)
#define CIDS_LOCALS_MARKPWR_JP       (-40)
#define CIDS_LOCALS_SPACEPWR_JP      (-40)
#define CIDS_LOCALS_TONE1FREQ_JP     (0)
#define CIDS_LOCALS_TONE2FREQ_JP     (0)
#define CIDS_LOCALS_TONE1PWR_JP      (0)
#define CIDS_LOCALS_TONE2PWR_JP      (0)
#define CIDS_LOCALS_CTYPE_JP         (CIDS_CRC)
#define CIDS_LOCALS_SIGNALTYPE_JP    (CIDS_SIG_SHORT_RING)
#define CIDS_LOCALS_RINGTIME_JP      (1000) 

/* 
 * CIDS state machine Extension flags 
 */
#define CIDS_LOCALS_EXT_DATA_ONLY    (0x0001)

/*
 * Local parameters struct:
 *
 * Timings: (CID Type1 in General)
 * 
 * battrev (if requested)
 * |
 * v ___________/\/\/\/\/\/\/\____________-_-_-_-_-~~~~~~~~xxxxxxxxx_________
 *  ^           ^            ^            ^        ^       ^       ^         ^
 *  | startTime | signalTime | preSezTime | tSeize | tMark | tData | endTime |
 */
typedef struct {
    FSKS_Obj *fsksObj_ptr;  /* Allocate and pass this pointer */
    TONE_Obj *toneObj_ptr;  /* Allocate and pass this pointer */
    uint8    *msg_ptr;      /* Message string, max = 256 bytes */
    uvint     msgSz;        /* Message size, max = 256 */   
    vint      dataType;     /* CIDS_DATA_FSK or CIDS_DATA_DTMF */
    uvint     battRev;      /* Set if battery reversal is required */
    uvint     startTime;    /* Silence before signal(ring, TAS) */
    uvint     signalTime;   /* Signal (special ring, TAS) */
    uvint     preSezTime;   /* Silence before seize, ONHT */
    uvint     sezLen;       /* (FSKS) Seize length in bits, ONHT */
    uvint     markLen;      /* (FSKS) Mark length in bits, ONHT */
    uvint     endTime;      /* Silence before starting normal ring */
    vint      offTimeout;   /* Timeout - for OFHOOK, -1 is no JP CID */
    vint      onTimeout;    /* Timeout - for ONHOOK, -1 is no JP CID */
    vint      onSilTime;    /* Last silence before ring, for JP CID */
    uvint     spaceFreq;    /* (FSKS) Space frequency */
    uvint     markFreq;     /* (FSKS) Mark frequency */
    vint      spacePwr;     /* (FSKS) Power during space in dBm, 0.5 dB steps */
    vint      markPwr;      /* (FSKS) Power during mark in dBm, 0.5 dB steps */
    uvint     tone1Freq;    /* Frequency of tone signal 1 */
    uvint     tone2Freq;    /* Frequency of tone signal 2 */
    vint      tone1Pwr;     /* Power of tone signal 1, if applicable */
    vint      tone2Pwr;     /* Power of tone signal 2, if applicable */
    vint      cType;        /* checksum or CRC */
    vint      signalType;   /* Signal type, ring or TAS */
    vint      ringTime;
    vint      extension;    /* Special Flags */
} CIDS_Params;

#ifndef EXPORT

/*
 * CIDS internal states:
 */
#define _CIDS_STATE_DONE        (CIDS_DONE)
#define _CIDS_STATE_INIT        (0x0001)
#define _CIDS_STATE_SILSTART    (0x0002)
#define _CIDS_STATE_SIGNAL      (0x0004)
#define _CIDS_STATE_SILPRESEIZE (0x0008)
#define _CIDS_STATE_START       (0x0040)
#define _CIDS_STATE_SILEND      (0x0080)
#define _CIDS_STATE_OFFHOOK     (0x0100)
#define _CIDS_STATE_ONHOOK      (0x0200)
#define _CIDS_STATE_WAITING     (CIDS_WAITING)
#define _CIDS_STATE_SILONHOOK   (0x0800)

/*
 * CIDS internal object:
 */
typedef struct {
    vint           state;
    vint           dataType;
    vint           sigType;
    vint           battRev;
    vint           callTime;
    vint           tSilStart;
    vint           tSignal;
    vint           tSilPreSez;
    vint           tSilEnd;
    vint           tOffHook;
    vint           tOnHook;
    vint           tOnSil;
    vint           offReq;
    vint           onReq;
    vint           ringTime;
    vint           pMode;
    vint           extension;
    FSKS_Obj      *fsksObj_ptr;
    TONE_Obj      *toneObj_ptr;
    GLOBAL_Params *globals_ptr;
    uint8         *msg_ptr;
    vint           dtmfSendIndex;
    vint           dtmfSendLen;
    vint           dtmfSendReady;
    TONE_Params    toneParams;
} _CIDS_Internal;

#endif

/*
 * CIDS object:
 */
typedef struct {
    _CIDS_Internal  internal;
    uvint           stat;      /* Status word */
    uvint           ctrl;      /* Control word */
    vint           *dst_ptr;   /* Destination data pointer */
} CIDS_Obj;


/*
 * Function prototypes:
 */
void CIDS_init(
    CIDS_Obj      *cidsObj_ptr,
    GLOBAL_Params *globals_ptr,
    CIDS_Params   *cidsParams_ptr);

int CIDS_run(
    CIDS_Obj *cidsObj_ptr);

int CIDS_processMessageJcid(
    uint8 *msg_ptr,
    vint   msgLen);

#endif
