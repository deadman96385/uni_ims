/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL AND
 * PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2006-2007 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * +D2Tech+ Rev: 2115  Date: 2007-08-01 15:15:20 -0700 (Wed, 01 Aug 2007) 
 * +D2Tech+ Release Version: alg_core3-ilbc
 *
 *
 * The following were measured for the iLBC coder. Cycle counts were recorded
 * from the start to the end of the function for each block of data. Interrupts
 * were disabled during the measurement period. The cycle counts were normalized
 * to cycles per second.
 *
 * ==================== MHz on m4kc1 ==========================================
 *          ENCODER     DECODER   FULL DUPLEX
 *  20ms     39.2        14.2       53.4 MHz
 *  30ms     45.5        13.9       59.4 MHz
 *
 * ==================== MHz on a9201 ==========================================
 *          ENCODER     DECODER   FULL DUPLEX
 *  20ms     62.74       20.23      82.97 MHz
 *  30ms     75.23       20.07      97.30 MHz
 *
 *
 *
 */
/******************************************************************

    iLBC Speech Coder ANSI-C Source Code

    iLBCInterface.h

    Copyright (c) 2004,
    Confidential
    All use is subject to license agreement with Global IP Sound

    Global IP Sound Inc.
    All rights reserved.

******************************************************************/

#ifndef _ILBC_H
#define _ILBC_H

/*
 * Define the fixpoint numeric formats
 */

#include <d2types.h>


/*
 * The following are public constansts.
 */
#define ILBC_BLOCK_LENGTH_20MS        (160)
#define ILBC_BLOCK_LENGTH_30MS        (240)
#define ILBC_MODE_20MS                (20)
#define ILBC_MODE_30MS                (30)

/*
 * The following constants are used by the algorithms and should not be changed.
 */
#define _ILBC_LPC_FILTERORDER         (10)
#define _ILBC_LPC_LOOPBACK            (60)
#define _ILBC_BLOCKL_MAX              (ILBC_BLOCK_LENGTH_30MS)
#define _ILBC_NSUB_MAX                (6)
#define _ILBC_SUBL                    (40)
#define _ILBC_ENH_BLOCKL              (80)
#define _ILBC_ENH_BUFL_FILTEROVERHEAD (3)
#define _ILBC_ENH_NBLOCKS_TOT         (8)
#define _ILBC_ENH_BUFL                (_ILBC_ENH_NBLOCKS_TOT * _ILBC_ENH_BLOCKL)

/* 
 * Solution to support multiple instances 
 * Customer has to cast instance to proper type
 */

/* type definition encoder instance */
typedef struct {

    /* flag for frame size mode */
    int16 mode;

    /* basic parameters for different frame sizes */
    int16 blockl;
    int16 nsub;
    int16 nasub;
    int16 no_of_bytes, no_of_words;
    int16 lpc_n;
    int16 state_short_len;

    /* analysis filter state */
    int16 anaMem[_ILBC_LPC_FILTERORDER];

    /* Fix-point old lsf parameters for interpolation */
    int16 lsfold[_ILBC_LPC_FILTERORDER];
    int16 lsfdeqold[_ILBC_LPC_FILTERORDER];

    /* signal buffer for LP analysis */
    int16 lpc_buffer[_ILBC_LPC_LOOPBACK + _ILBC_BLOCKL_MAX];

    /* state of input HP filter */
    int16 hpimemx[2];
    int16 hpimemy[4];

#ifdef __ILBC_WITH_SCRATCHMEM
    /* Scratch mem */
    int16 *ScratchMem;
#endif

} ILBC_EncObj;

/* type definition decoder instance */
typedef struct {

    /* flag for frame size mode */
    int16 mode;

    /* basic parameters for different frame sizes */
    int16 blockl;
    int16 nsub;
    int16 nasub;
    int16 no_of_bytes, no_of_words;
    int16 lpc_n;
    int16 state_short_len;
    
    /* synthesis filter state */
    int16 syntMem[_ILBC_LPC_FILTERORDER];

    /* old LSF for interpolation */
    int16 lsfdeqold[_ILBC_LPC_FILTERORDER];

    /* pitch lag estimated in enhancer and used in PLC */
    vint last_lag;

    /* PLC state information */
    vint consPLICount, prev_enh_pl;
    int16 perSquare;

    int16 prevScale, prevPLI;
    int16 prevLag, prevLpc[_ILBC_LPC_FILTERORDER+1];
    int16 prevResidual[_ILBC_NSUB_MAX*_ILBC_SUBL];
    int16 seed;

    /* previous synthesis filter parameters */

    int16 old_syntdenum[(_ILBC_LPC_FILTERORDER + 1)*_ILBC_NSUB_MAX];

    /* state of output HP filter */
    int16 hpimemx[2];
    int16 hpimemy[4];

    /* enhancer state information */
    vint use_enhancer;
    int16 enh_buf[_ILBC_ENH_BUFL+_ILBC_ENH_BUFL_FILTEROVERHEAD];
    int16 enh_period[_ILBC_ENH_NBLOCKS_TOT];

#ifdef __ILBC_WITH_SCRATCHMEM
    /* Scratch mem */
    int16 *ScratchMem;
#endif

} ILBC_DecObj;

/*
 * Comfort noise constants
 */

#define ILBC_GIPS_SPEECH    1
#define ILBC_GIPS_CNG        2



/****************************************************************************
 * ILBC_encoderinit(...)
 *
 * This function initializes a iLBC instance
 *
 * Input:
 *        - iLBCenc_inst        : iLBC instance, i.e. the user that should receive 
 *                              be initialized
 *        - mode                : Frame length in ms (20 or 30)
 *
 * Return value                :  0 - Ok
 *                              -1 - Error
 */

int16 ILBC_encoderinit(ILBC_EncObj *enc_ptr, int16 mode);

/****************************************************************************
 * ILBC_encode(...)
 *
 * This function encodes one iLBC frame. Input speech length has be 240 
 * samples.
 *
 * Input:
 *        - iLBCenc_inst        : iLBC instance, i.e. the user that should encode
 *                              a package
 *      - speechIn            : Input speech vector
 *      - len                : Samples in speechIn (240 or 480)
 *
 * Output:
 *        - encoded            : The encoded data vector
 *
 * Return value                : >0 - Length (in bytes) of coded data
 *                              -1 - Error
 */

int16 ILBC_encode(ILBC_EncObj *enc_ptr, 
                                   int16 *speechIn, 
                                   int16 len, 
                                   int16 *encoded);

/****************************************************************************
 * ILBC_decoderinit(...)
 *
 * This function initializes a iLBC instance
 *
 * Input:
 *        - ILBC_DecObj    : iLBC instance, i.e. the user that should receive 
 *                              be initialized
 *        - frameLen            : Frame length in ms (20 or 30)
 *
 * Return value                :  0 - Ok
 *                              -1 - Error
 */

vint ILBC_decoderinit(ILBC_DecObj *dec_ptr, int16 frameLen);

/****************************************************************************
 * ILBC_decode(...)
 *
 * This function decodes a packet with iLBC frame(s). Output speech length 
 * will be a multiple of 240 samples (240*frames/packet).
 *
 * Input:
 *        - iLBCdec_inst        : iLBC instance, i.e. the user that should decode
 *                              a packet
 *      - encoded            : Encoded iLBC frame(s)
 *      - len                : Bytes in encoded vector
 *
 * Output:
 *        - decoded            : The decoded vector
 *      - speechType        : 1 normal, 2 CNG
 *
 * Return value                : >0 - Samples in decoded vector
 *                              -1 - Error
 */

int16 ILBC_decode(ILBC_DecObj *dec_ptr, 
                                   int16 *encoded, 
                                   int16 len, 
                                   int16 *decoded,
                                   int16 *speechType);

/****************************************************************************
 * ILBC_decodePLC(...)
 *
 * This function conducts PLC for iLBC frame(s). Output speech length 
 * will be a multiple of 160/240 samples.
 *
 * Input:
 *        - iLBCdec_inst        : iLBC instance, i.e. the user that should perform
 *                              a PLC
 *      - noOfLostFrames    : Number of PLC frames to produce
 *
 * Output:
 *        - decoded            : The "decoded" vector
 *
 * Return value                : >0 - Samples in decoded PLC vector
 *                              -1 - Error
 */

int16 ILBC_decodePLC(ILBC_DecObj *dec_ptr, 
                                     int16 *decoded, 
                                     int16 noOfLostFrames);

/****************************************************************************
 * ILBC_NetEqPLC(...)
 *
 * This function updates the decoder when a packet loss has occured, but it
 * does not produce any PLC data. Function can be used if another PLC method 
 * is used (i.e NetEq).
 *
 * Input:
 *        - iLBCdec_inst        : iLBC instance that should be updated
 *      - noOfLostFrames    : Number of lost frames
 *
 * Output:
 *        - decoded            : The "decoded" vector (nothing in this case)
 *
 * Return value                : >0 - Samples in decoded PLC vector
 *                              -1 - Error
 */

int16 ILBC_NetEqPLC(ILBC_DecObj *dec_ptr, 
                                  int16 *decoded, 
                                  int16 noOfLostFrames);


/****************************************************************************
 * NETEQFIX_GIPS_version(...)
 *
 * This function returns the version number of iLBC
 *
 * Output:
 *        - version        : Version number of iLBC (maximum 20 char)                  
 */

void ILBC_version(char *version);


#endif

