/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004-2010 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 12522 $ $Date: 2010-07-14 06:52:44 +0800 (Wed, 14 Jul 2010) $
 */


#ifndef _CIDCWS_H_
#define _CIDCWS_H_

#include "osal_types.h"
#include "comm.h"
#include "fsks.h"
#include "tone.h"

#ifdef VTSP_ENABLE_TONE_QUAD
#include "genf.h"
#endif

/* Public state.
 */
#define CIDCWS_DONE         (0x0000)

/*
 * The block size for the module
 */
#define CIDCWS_BLOCK_SIZE   (COMM_10MS_LEN)

/*
 * CIDCWS checksum/CRC type in local params:
 */
#define CIDCWS_CHECKSUM     (0x0001)  /* BC, BT, ETSI */
#define CIDCWS_CRC          (0x0002)  /* NTT */

/*
 * Signal types:
 */
#define CIDCWS_SIG_CAS      (0x0001)  /* BC, BT, ETSI */
#define CIDCWS_SIG_CAT      (0x0002)  /* NTT */

/*
 * ACK types
 */
#define CIDCWS_ACK_DTMF_D   (0x000F)  /* BC, BT, ETSI, digit D */
#define CIDCWS_ACK_NONE     (0x0000)  /* NTT */

/*
 * Commonly used frequencies (information):
 */

#define CIDCWS_CAS_FREQ1    (2130)    /* CAS */
#define CIDCWS_CAS_FREQ2    (2750)

#define CIDCWS_CAT_FREQ1    (1633)    /* CAT */
#define CIDCWS_CAT_FREQ2    (852)
#define CIDCWS_CAT_FREQ3    (1633)
#define CIDCWS_CAT_FREQ4    (941)

/*
 * Define local parameters for various countries:
 */

/*
 * Defaults for ETSI.
 */
#define CIDCWS_LOCALS_STARTTIME_ETSI    (40)
#define CIDCWS_LOCALS_SASTIME_ETSI      (0)
#define CIDCWS_LOCALS_INTRTIME_ETSI     (0)
#define CIDCWS_LOCALS_CASTIME_ETSI      (80)
#define CIDCWS_LOCALS_ACKTIMEOUT_ETSI   (160)
#define CIDCWS_LOCALS_MINACKTIME_ETSI   (40) /* Set tDTMFDUR in DTMF Params */
#define CIDCWS_LOCALS_PREXMITTIME_ETSI  (80)
#define CIDCWS_LOCALS_SEZLEN_ETSI       (0)
#define CIDCWS_LOCALS_MARKLEN_ETSI      (100)
#define CIDCWS_LOCALS_ENDTIME_ETSI      (80)
#define CIDCWS_LOCALS_MARKFREQ_ETSI     (1300)
#define CIDCWS_LOCALS_SPACEFREQ_ETSI    (2100)
#define CIDCWS_LOCALS_SASFREQ_ETSI      (440)
#define CIDCWS_LOCALS_MARKPWR_ETSI      (-27)
#define CIDCWS_LOCALS_SPACEPWR_ETSI     (-27)
#define CIDCWS_LOCALS_SASPWR_ETSI       (-24)
#define CIDCWS_LOCALS_CASPWR_ETSI       (-24)
#define CIDCWS_LOCALS_CTYPE_ETSI        (CIDCWS_CHECKSUM)
#define CIDCWS_LOCALS_ACKTYPE_ETSI      (CIDCWS_ACK_DTMF_D)
#define CIDCWS_LOCALS_SIGNALTYPE_ETSI   (CIDCWS_SIG_CAS)

/*
 * Defaults for Bellcore.
 */
#define CIDCWS_LOCALS_STARTTIME_BC      (60)
#define CIDCWS_LOCALS_SASTIME_BC        (300)
#define CIDCWS_LOCALS_INTRTIME_BC       (30)
#define CIDCWS_LOCALS_CASTIME_BC        (80)
/* ACKTIMEOUT includes length of DTMF digit (until trailing edge detection) 
 * Assume length of DTMF digit is nominally 80 ms 
 */
#define CIDCWS_LOCALS_ACKTIMEOUT_BC     (80+160 * 2)
#define CIDCWS_LOCALS_MINACKTIME_BC     (40) /* Set tDTMFDUR in DTMF Params */
/* Note for PREXMITTIME, FSKS includes a 50ms internal silence period
 * prior to transmission */
#define CIDCWS_LOCALS_PREXMITTIME_BC    (0) 
#define CIDCWS_LOCALS_SEZLEN_BC         (0)
#define CIDCWS_LOCALS_MARKLEN_BC        (80) 
#define CIDCWS_LOCALS_ENDTIME_BC        (300)
#define CIDCWS_LOCALS_MARKFREQ_BC       (1200)
#define CIDCWS_LOCALS_SPACEFREQ_BC      (2200)
#define CIDCWS_LOCALS_SASFREQ_BC        (440)
#define CIDCWS_LOCALS_MARKPWR_BC        (-27)
#define CIDCWS_LOCALS_SPACEPWR_BC       (-27)
#define CIDCWS_LOCALS_SASPWR_BC         (-24)
#define CIDCWS_LOCALS_CASPWR_BC         (-24)
#define CIDCWS_LOCALS_CTYPE_BC          (CIDCWS_CHECKSUM)
#define CIDCWS_LOCALS_ACKTYPE_BC        (CIDCWS_ACK_DTMF_D)
#define CIDCWS_LOCALS_SIGNALTYPE_BC     (CIDCWS_SIG_CAS)

/*
 * Defaults for BT.
 */
#define CIDCWS_LOCALS_STARTTIME_BT      (60)
#define CIDCWS_LOCALS_SASTIME_BT        (0)
#define CIDCWS_LOCALS_INTRTIME_BT       (0)
#define CIDCWS_LOCALS_CASTIME_BT        (80)
#define CIDCWS_LOCALS_ACKTIMEOUT_BT     (160)
#define CIDCWS_LOCALS_MINACKTIME_BT     (70) /* Set tDTMFDUR in DTMF Params */
#define CIDCWS_LOCALS_PREXMITTIME_BT    (50)
#define CIDCWS_LOCALS_SEZLEN_BT         (0)
#define CIDCWS_LOCALS_MARKLEN_BT        (80)
#define CIDCWS_LOCALS_ENDTIME_BT        (80)
#define CIDCWS_LOCALS_MARKFREQ_BT       (1300)
#define CIDCWS_LOCALS_SPACEFREQ_BT      (2100)
#define CIDCWS_LOCALS_SASFREQ_BT        (440)
#define CIDCWS_LOCALS_MARKPWR_BT        (-27)
#define CIDCWS_LOCALS_SPACEPWR_BT       (-27)
#define CIDCWS_LOCALS_SASPWR_BT         (-24)
#define CIDCWS_LOCALS_CASPWR_BT         (-24)
#define CIDCWS_LOCALS_CTYPE_BT          (CIDCWS_CHECKSUM)
#define CIDCWS_LOCALS_ACKTYPE_BT        (CIDCWS_ACK_DTMF_D)
#define CIDCWS_LOCALS_SIGNALTYPE_BT     (CIDCWS_SIG_CAS)

/*
 * Defaults for NTT.
 */
#define CIDCWS_LOCALS_STARTTIME_NTT     (0)
#define CIDCWS_LOCALS_SASTIME_NTT       (1200)
#define CIDCWS_LOCALS_INTRTIME_NTT      (0)
#define CIDCWS_LOCALS_CASTIME_NTT       (70)
#define CIDCWS_LOCALS_CASBREAKTIME_NTT  (70)
#define CIDCWS_LOCALS_ACKTIMEOUT_NTT    (0)
#define CIDCWS_LOCALS_MINACKTIME_NTT    (0)
#define CIDCWS_LOCALS_PREXMITTIME_NTT   (370)
#define CIDCWS_LOCALS_SEZLEN_NTT        (0)
#define CIDCWS_LOCALS_MARKLEN_NTT       (80)
#define CIDCWS_LOCALS_ENDTIME_NTT       (100)
#define CIDCWS_LOCALS_MARKFREQ_NTT      (1300)
#define CIDCWS_LOCALS_SPACEFREQ_NTT     (2100)
#define CIDCWS_LOCALS_SASFREQ_NTT       (400)
#define CIDCWS_LOCALS_MARKPWR_NTT       (-27)
#define CIDCWS_LOCALS_SPACEPWR_NTT      (-27)
#define CIDCWS_LOCALS_SASPWR_NTT        (-24)
#define CIDCWS_LOCALS_CASPWR_NTT        (-24)
#define CIDCWS_LOCALS_CTYPE_NTT         (CIDCWS_CRC)
#define CIDCWS_LOCALS_ACKTYPE_NTT       (CIDCWS_ACK_NONE)
#define CIDCWS_LOCALS_SIGNALTYPE_NTT    (CIDCWS_SIG_CAT)
/* 
 * CIDCWS state machine Extension flags 
 * CIDCWS_LOCALS_EXT_DATA_ONLY: Bypass normal state machine, run only FSKS
 *      for data transmission
 */
#define CIDCWS_LOCALS_EXT_DATA_ONLY    (0x0001)

/*
 * Control flags
 * CIDCWS_CTRL_TX: Allow application to time the sending of FSK, when 
 *      in DATA_ONLY mode
 *
 */
#define CIDCWS_CTRL_TX                 (0x0001)
#define CIDCWS_CTRL_TONE_INIT          (0x0002)
#define CIDCWS_CTRL_TONE_TRAILING      (0x0004)

/*
 * Local parameters struct:
 */
typedef struct {
    FSKS_Obj *fsksObj_ptr;  /* Allocate and pass this pointer */
    TONE_Obj *toneObj_ptr;  /* Allocate and pass this pointer */
#ifdef VTSP_ENABLE_TONE_QUAD
    GENF_Obj *genfObj_ptr;  /* Allocate and pass this pointer */
#endif
    uint8    *msg_ptr;      /* Message string, max = 256 bytes */
    uvint     msgSz;        /* Message size, max = 256 */   
    vint      startTime;    /* Silence before signal SAS/IIT */
    vint      sasTime;      /* SAS/IIT time */
    vint      intrTime;     /* Silence between CAS/SAS */
    vint      casTime;      /* CAS/CAT/DTAS time */
    vint      ackTimeout;   /* ACK timeout */
    vint      preXmitTime;  /* Silence after ACK and before xmit */
    uvint     sezLen;       /* Seize length in bits, ONHT */
    uvint     markLen;      /* Mark length in bits, ONHT */
    vint      endTime;      /* Silence before starting normal ring */
    uvint     spaceFreq;    /* Space frequency */
    uvint     markFreq;     /* Mark frequency */
    vint      sasFreq;      /* SAS/IIT frequency */
    vint      spacePwr;     /* Power during space in dBm, 0.5 dB steps */
    vint      markPwr;      /* Power during mark in dBm, 0.5 dB steps */
    vint      casPwr;       /* Power of CAS signal in dBm, 0.5 dB steps */
    vint      sasPwr;       /* Power of SAS signal in dBm, 0.5 dB step */
    uvint     ackType;      /* ACK type */
    uvint     cType;        /* checksum or CRC */
    uvint     signalType;   /* Signal type, CAS, DTAS, CAT etc. */
    vint      extension;    /* Special Flags */
} CIDCWS_Params;

#ifndef EXPORT

/*
 * CIDCWS internal states:
 */
#define _CIDCWS_STATE_DONE        (CIDCWS_DONE)
#define _CIDCWS_STATE_SILSTART    (0x0001)
#define _CIDCWS_STATE_SAS         (0x0002)
#define _CIDCWS_STATE_SILINTR     (0x0004)
#define _CIDCWS_STATE_CAS         (0x0008)
#define _CIDCWS_STATE_ACK         (0x0010)
#define _CIDCWS_STATE_SILPREXMIT  (0x0020)
#define _CIDCWS_STATE_START       (0x0040)
#define _CIDCWS_STATE_SILEND      (0x0080)
#define _CIDCWS_STATE_SAS_IIT     (0x0100)

/*
 * CIDCWS internal object:
 */
typedef struct {
    uvint     state;
    uvint     sigType;
    uvint     callTime;
    vint      tSilStart;
    vint      tSas;
    vint      tSilIntr;
    vint      tCasMake;
    vint      tCasBreak;
    vint      tAck;
    vint      tSilPreXmit;
    vint      tSilEnd;
    vint      ackType;
    uvint     toneInit;
    vint      casPwr;
    uvint     p0db;
    vint      extension;
    vint      pMode;
    FSKS_Obj *fsksObj_ptr;
    TONE_Obj *toneObj_ptr;
#ifdef VTSP_ENABLE_TONE_QUAD
    GENF_Obj *genfObj_ptr;
#endif
} _CIDCWS_Internal;

#endif

/*
 * CIDCWS object:
 */
typedef struct {
    _CIDCWS_Internal  internal;
    vint              dtmfDigit; /* Digit field from dtmf detector: 'D'=15 */
    uvint             ctrl;      /* Control word */
    vint             *dst_ptr;   /* Destination data pointer */
} CIDCWS_Obj;


/*
 * Function prototypes:
 */
void CIDCWS_init(
    CIDCWS_Obj    *cidsObj_ptr,
    GLOBAL_Params *globals_ptr,
    CIDCWS_Params *cidsParams_ptr);

int CIDCWS_run(
    CIDCWS_Obj *cidsObj_ptr);

int CIDCWS_processMessageJcid(
    uint8 *msg_ptr,
    vint   msgLen);

#endif
