/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004-2008 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 30131 $ $Date: 2014-12-01 14:45:03 +0800 (Mon, 01 Dec 2014) $
 */

#include "osal.h"
#ifndef VTSP_ENABLE_MP_LITE
#include "tone.h"
#endif
#ifdef VTSP_ENABLE_TONE_QUAD
#include "genf.h"
#endif
#include "jb.h"

#ifndef _DR_H_
#define _DR_H_

#define DR_BLOCKSIZE_8K         (80)                /* fixed blocksize of 8K */
#define DR_BLOCKSIZE_16K        (160)               /* fixed blocksize of 16K */

#define DR_EVENT_LE             (0x0001)            /* leading edge event */
#define DR_EVENT_TE             (0x0002)            /* trailing edge event */

#define DR_STOP_CLEAR           (0x0000)
#define DR_STOP_SET             (0x0001)

#define DR_IN_DIGIT             (0x0100)            /* status = digit active */
#define DR_DIGIT_END            (0x0200)            /* status = digit ended */

#define DR_SAMPLE_RATE_8K       (0)
#define DR_SAMPLE_RATE_16K      (1)
/*
 * Misc constants
 */
#define DR_START_DUR_8K         (80)                /* in 8000 Hz units */
#define DR_START_DUR_16K        (160)               /* in 16000 Hz units */
#define DR_DECODE_DIGIT_DB_MAX  (-5)                /* Max dB for tone gen */
#define DR_MAX_PLAYTIME_8K      (8000 * 80 / 10)    /* 8 seconds */
#define DR_MAX_PLAYTIME_16K     (4000 * 160 / 10)   /* 4 seconds */

#define DR_RELAY_MODE_EVENT     (0)
#define DR_RELAY_MODE_TONE      (1) 

#define DR_NUM_BYTES_EVENT      (4)
#define DR_NUM_BYTES_TONE       (12)

/* 
 * DR_EventObj
 * This is the dtmf relay event structure placed in the channel object
 */
typedef struct {
    /* DR transitions controlled by event and stop */
    uvint        event;             /* event mask */
    uvint        stop;              /* Event signal to stop encoding */

    /* The remaining element control only leading edge events */
    uvint        newDigit;           /* Leading Edge digit, 0 - 15 */
    /* 
     * digit power in (-dB), ex: 36 for -36 dB 
     * posPower must be masked with 0x3f to limit range.
     */
    vint         newPower;    
    uint32       newPlayTime;       /* playTime for the new leDigit */
    /* Use Event or Tone relay mode */
    uvint        relayMode;
} DR_EventObj;

/* 
 * DR_EncodeObj
 * This is the dtmf relay encoder object placed in the stream object
 */
typedef struct { 
    /* Public - R/W by caller */
    uvint        status;
    vint         end;
    uvint        prevCoder;
    uvint        redundancy;
    uvint        detectLeTime;      /* used to adjust start timestamp */
    uvint        detectTeTime;      /* used to adjust duration */

    /* Internal - to DR state */
    uvint        newEvent;
    uvint        lastEvent;
    uint32       dur;
    uint32       playTime;
    uint32       totalTime;
    vint         power;
    /* Telephone-event type paramater */
    uvint        digit;
    /* Telphone-tone type parameters */
    uvint        modulation;
    uvint        freq1;
    uvint        freq2;
    uvint        freq3;
    uvint        freq4;
    uvint        divMod3;

    /* Use Event or Tone relay mode */
    uvint        relayMode;

    /* Sample rate */
    uvint        sampleRate;
} DR_EncodeObj;

/* 
 * DR_DecodeObj
 *
 * This is the dtmf relay decoder object placed in the stream object
 */
typedef struct {
    /* Public - R/W by caller */
    uvint          control;
    vint           flash;
    vint           flashDone;
    uvint          status;
    uvint          power;

#ifndef VTSP_ENABLE_MP_LITE
    TONE_Obj      *toneObj_ptr;
    TONE_Params   *toneParam_ptr;
#endif
#ifdef VTSP_ENABLE_TONE_QUAD
    GENF_Obj      *toneQuadObj_ptr;
    GENF_Params   *toneQuadParam_ptr;
#endif
    GLOBAL_Params *globals_ptr;
    uvint          edge;

    /* Internal - to DR state */
    uvint        end;
    uvint        digit;

    /* Use Event or Tone relay mode */
    uvint        relayMode;

    /* Sample rate */
    uvint        sampleRate;
} DR_DecodeObj;

/* Public functions
 */
vint DR_encode(
    DR_EncodeObj *encObj_ptr,
    DR_EventObj  *encEvent_ptr,
    void              *dst_ptr);
vint DR_decode(
    DR_DecodeObj *decObj_ptr,
    void              *src_ptr,
    void              *dst_ptr,
    uvint              pSize);
void DR_decodeInit(
    DR_DecodeObj *decObj_ptr);
void DR_encodeInit(
    DR_EncodeObj *encObj_ptr);
void DR_increaseEnd(
    DR_EncodeObj *encObj_ptr);

#endif

