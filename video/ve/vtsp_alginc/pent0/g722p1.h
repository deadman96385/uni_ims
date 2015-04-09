/***********************************************************************
************************************************************************
**
**   ITU-T 7/14kHz Audio Coder Candidate (G.722.1 Annex C) Source Code
**
**   © 2004 Polycom, Inc.
**
**   All rights reserved.
**
************************************************************************
***********************************************************************/
/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2007 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * +D2Tech+ Rev: 2047  Date: 2007-07-24 12:04:58 -0700 (Tue, 24 Jul 2007) 
 * +D2Tech+ Release Version: alg_core3-g722p1
 */

#ifndef _G722P1_H_
#define _G722P1_H_

#include <d2types.h>

#define _G722P1_D2MOD       /* Runs Optimized Code */

/*
 * The following constants define sample rates and bit rates for G722P1
 * encoder and decoder initialization functions.
 */
#define G722P1_SAMPLE_RATE_16KHZ            (16000)
#define G722P1_SAMPLE_RATE_32KHZ            (32000) /* DISABLED */
#define G722P1_BIT_RATE_24KBPS              (24000)
#define G722P1_BIT_RATE_32KBPS              (32000) 
#define G722P1_BIT_RATE_48KBPS              (48000) /* DISABLED */

/* Speech buffers sizes are (Sample Rate / 50) for 20 ms processing */
#define G722P1_SPEECH_BUFSZ_16KHZ           (320) 
#define G722P1_SPEECH_BUFSZ_32KHZ           (640)   /* DISABLED */

/* Packet Buffer sizes are ((Bit Rate / 50) >> 3) for 20 ms processing. */
#define G722P1_PACKET_BUFSZ_24KBPS          (60)
#define G722P1_PACKET_BUFSZ_32KBPS          (80)
#define G722P1_PACKET_BUFSZ_48KBPS          (120)   /* DISABLED */

#define G722P1_ERRORFLAG_PACKET_OK          (0)
#define G722P1_ERRORFLAG_PACKET_LOSS        (1)

#define G722P1_SCRATCH_NBYTES               (4 * G722P1_SPEECH_BUFSZ_32KHZ * 2)


/*
 * Object definitions
 */
typedef struct {
    int16       code_bit_count;      /* bit count of the current word */
    int16       current_word;        /* current word in the bitstream */
    int16      *code_word_ptr;       /* pointer to the bitstream */
    int16       number_of_bits_left; /* no. of bits left in the current word */
    int16       next_bit;            /* next bit in the current word */
} _G722P1_Bit_Obj;

typedef struct {
    int16       seed0;
    int16       seed1;
    int16       seed2;
    int16       seed3;
} _G722P1_Rand_Obj;

typedef struct {
    void               *scratch_ptr;
    int16               history[G722P1_SPEECH_BUFSZ_32KHZ];
    int16               mltCoefs[G722P1_SPEECH_BUFSZ_32KHZ];
    int16               numAvailableBits;
    int16               numRegions;
    int16               speechBufSz;
} G722P1_EncObj;

typedef struct {
    void               *scratch_ptr;
    _G722P1_Bit_Obj     bitObj;
    _G722P1_Rand_Obj    randObj;
    int16               decoderMltCoefs[G722P1_SPEECH_BUFSZ_32KHZ];
    int16               oldDecoderMltCoefs[G722P1_SPEECH_BUFSZ_32KHZ];
    int16               oldSamples[G722P1_SPEECH_BUFSZ_16KHZ];
    int16               oldMagShift;
    int16               numAvailableBits;
    int16               numRegions;
    int16               speechBufSz;
} G722P1_DecObj;

/*
 * Function Prototypes
 */
OSAL_Status G722P1_encodeInit(
        G722P1_EncObj      *encObj_ptr,
        vint                bitRate,
        vint                sampleRate);

OSAL_Status G722P1_decodeInit(
        G722P1_DecObj      *decObj_ptr,
        vint                bitRate,
        vint                sampleRate);

vint G722P1_encode(
        G722P1_EncObj      *encObj_ptr,
        vint               *speechIn_ptr,
        uint8              *packetOut_ptr);

void G722P1_decode(
        G722P1_DecObj      *decObj_ptr,
        uint8              *packetIn_ptr,
        vint               *speechOut_ptr,
        vint                packetBufSz,
        vint                errorFlag);


#endif  /* _G722P1_H_ */
