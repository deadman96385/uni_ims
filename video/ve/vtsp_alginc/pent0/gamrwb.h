/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2009 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * +D2Tech+ Rev: 4558  Date: 2013-11-21 11:48:56 -0800 (Thu, 21 Nov 2013) 
 * +D2Tech+ Release Version: PENT0_A_1_1-gamrwb_1_9
 */

#ifndef __GAMRWB_H__
#define __GAMRWB_H__

#include <d2types.h>

#define GAMRWB_L_FRAME  (320)                  /* Window size in LP analysis */

typedef enum {
    GAMRWB_MR660 = 0,
    GAMRWB_MR885,
    GAMRWB_MR1265,
    GAMRWB_MR1425,
    GAMRWB_MR1585,
    GAMRWB_MR1825,
    GAMRWB_MR1985,
    GAMRWB_MR2305,
    GAMRWB_MR2385,
    GAMRWB_MRDTX,
    GAMRWB_LOST_FRAME = 14,
    GAMRWB_MRNODATA = 15,
} GAMRWB_RateMode;

//#ifndef EXPORT
typedef enum {
    GAMRWB_RX_SPEECH_GOOD = 0,
    GAMRWB_RX_SPEECH_PROBABLY_DEGRADED,
    GAMRWB_RX_SPEECH_LOST,
    GAMRWB_RX_SPEECH_BAD,
    GAMRWB_RX_SID_FIRST,
    GAMRWB_RX_SID_UPDATE,
    GAMRWB_RX_SID_BAD,
    GAMRWB_RX_NO_DATA,
} GAMRWB_RXFrameType;
//#endif /* EXPORT */

typedef enum {
    GAMRWB_TX_SPEECH = 0,
    GAMRWB_TX_SID_FIRST,
    GAMRWB_TX_SID_UPDATE,
    GAMRWB_TX_NO_DATA,
} GAMRWB_TxStatus;

typedef enum {
    GAMRWB_DTX_VAD_DISABLE = 0,
    GAMRWB_DTX_VAD_ENABLE
} GAMRWB_DtxEnable;

typedef enum {
    GAMRWB_ERRORFLAG_PACKET_OK = 0,
    GAMRWB_ERRORFLAG_PACKET_LOSS
} GAMRWB_ErrorFlag;

typedef enum {
    GAMRWB_DATA_FORMAT_SERIAL = 0,
    GAMRWB_DATA_FORMAT_PACKED_IF1,
    GAMRWB_DATA_FORMAT_PACKED_IF2
} GAMRWB_DataFormat;

/*
 * Internal Object:
 */
typedef struct {
    char internal_ary[0x2508];
} _GAMRWB_Internal;

typedef struct {
    _GAMRWB_Internal internal;
} GAMRWB_EncObj;

typedef struct {
    _GAMRWB_Internal internal;
} GAMRWB_DecObj;

/*
 * Function Prototypes
 */
void GAMRWB_encoderInit(
    GAMRWB_EncObj       *encObj_ptr,
    GAMRWB_DataFormat    dataFormat);

vint GAMRWB_encode(
    GAMRWB_EncObj       *encObj_ptr,
    vint                *speechIn_ptr,
    uint8                  *packetOut_ptr,
    GAMRWB_RateMode     *rateMode_ptr,
    GAMRWB_DtxEnable     dtxEnable,
    GAMRWB_TxStatus     *txStatus_ptr);

void GAMRWB_decoderInit(
    GAMRWB_DecObj       *decObj_ptr,
    GAMRWB_DataFormat    dataFormat);

void GAMRWB_decode(
    GAMRWB_DecObj      *decObj_ptr,
    uint8              *packetIn_ptr,
    vint               *speechOut_ptr,
    GAMRWB_ErrorFlag    errorFlag);

#endif



