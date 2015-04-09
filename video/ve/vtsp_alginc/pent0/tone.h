/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2003-2007 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * +D2Tech+ Rev: 23  Date: 2007-03-27 17:46:18 -0700 (Tue, 27 Mar 2007) 
 * +D2Tech+ Release Version: trunk-tone
 */

#ifndef _TONE_H_
#define _TONE_H_

#include <comm.h>

/* 
 * Valid TONE_generate() return values which describe
 * the different states of the tone generator.
 */
#define TONE_DONE           (0)             /* Tone generation complete */
#define TONE_STATE_DONE     (TONE_DONE)
#define TONE_STATE_MAK1     (1)
#define TONE_STATE_BRK1     (2)
#define TONE_STATE_MAK2     (4)
#define TONE_STATE_BRK2     (8)
#define TONE_STATE_MAK3     (16)
#define TONE_STATE_BRK3     (32)

/*
 *  Example Control Words for typical tone sequences
 */
#define TONE_SIL                (0x0000) /* generate silence */
#define TONE_MONO               (0x0001) /* single tone */
#define TONE_DUAL               (0x0002) /* dual tone */
#define TONE_SAMPLERATE_8K      (0x0000) /* 8kHz  Sampling rate */
#define TONE_SAMPLERATE_16K     (0x1000) /* 16kHz Sampling rate */


/*
 * Local Parameters Structure
 */
typedef struct {
    vint  tone1;    /* Frequency of tone1 to generate */
    vint  t1Pw;     /* Power in 0.5db units for tone1 */
    vint  tone2;    /* Frequency of tone2 to generate */
    vint  t2Pw;     /* Power in 0.5db units for tone2 */
    vint  ctrlWord; /* Control Word */
    vint  numCads;  /* Number of on/off pairs */
    vint  make1;    /* 1st on  time in 1 ms units */
    vint  break1;   /* 1st off time in 1 ms units */
    vint  repeat1;  /* 1st cadence cycles */ 
    vint  make2;    /* 2nd on  time in 1 ms units */
    vint  break2;   /* 2nd off time in 1 ms units */
    vint  repeat2;  /* 2nd cadence cycles */ 
    vint  make3;    /* 3rd on  time in 1 ms units */
    vint  break3;   /* 3rd off time in 1 ms units */
    vint  repeat3;  /* 3rd cadence cycles */ 
    vint  repeat;   /* Number of cycles of tones */
} TONE_Params;

/*
 * Internal Object:
 */
typedef struct {
    char internal_ary[0x58];
} _TONE_Internal;

/*
 * Tone Object
 */
typedef struct {
    _TONE_Internal  internal; /* Internal object */
    vint           *dst_ptr;  /* Pointer to user-supplied buffer for the tone */
} TONE_Obj;

/*
 * Function prototypes
 */
void TONE_init(
    TONE_Obj      *toneObj_ptr,  /* Pointer to TONE object struct */
    GLOBAL_Params *globals_ptr,  /* Pointer to global parameters struct */
    TONE_Params   *locals_ptr);  /* Pointer to TONE local parameters struct */

vint TONE_generate(
    TONE_Obj      *toneObj_ptr); /* Pointer to TONE object struct */

#endif
