/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2008 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * +D2Tech+ Rev: 4585  Date: 2013-12-17 23:47:02 -0800 (Tue, 17 Dec 2013) 
 * +D2Tech+ Release Version: PENT0_A_1_1-gamrnb_1_6
 */

#ifndef __GAMRNB_H__
#define __GAMRNB_H__

#include <d2types.h>


typedef enum {
    GAMRNB_MR475 = 0,
    GAMRNB_MR515,            
    GAMRNB_MR59,
    GAMRNB_MR67,
    GAMRNB_MR74,
    GAMRNB_MR795,
    GAMRNB_MR102,
    GAMRNB_MR122,            
    GAMRNB_MRDTX,
} GAMRNB_RateMode;


typedef enum {
    GAMRNB_TX_SPEECH_GOOD = 0,
    GAMRNB_TX_SID_FIRST,
    GAMRNB_TX_SID_UPDATE,
    GAMRNB_TX_NO_DATA,
} GAMRNB_TxStatus;

typedef enum {
    GAMRNB_DTX_VAD_DISABLE = 0,
    GAMRNB_DTX_VAD_ENABLE
} GAMRNB_DtxEnable;

typedef enum {
    GAMRNB_VAD_TYPE_VAD1_ENS = 0,
    GAMRNB_VAD_TYPE_VAD2_MOTOROLA
} GAMRNB_VadType;

typedef enum {
    GAMRNB_SPEECH_FORMAT_LINEAR = 0,
    GAMRNB_SPEECH_FORMAT_ULAW,
    GAMRNB_SPEECH_FORMAT_ALAW
} GAMRNB_SpeechFormat;

typedef enum {    
    GAMRNB_ERRORFLAG_PACKET_OK = 0,
    GAMRNB_ERRORFLAG_PACKET_LOSS
} GAMRNB_ErrorFlag;

typedef enum {    
    GAMRNB_DATA_FORMAT_SERIAL = 0,
    GAMRNB_DATA_FORMAT_PACKED_OA, /* Octet-Align */
    GAMRNB_DATA_FORMAT_PACKED_BE  /* Bandwidth-efficient */
} GAMRNB_DataFormat;

typedef enum {    
    GAMRNB_BIT_MASK_NONE = 0,
    GAMRNB_BIT_MASK_13BIT
} GAMRNB_BitMaskMode;

/*
 * Internal Object:
 */
typedef struct {
    char internal_ary[0x14a4];
} _GAMRNB_Internal;

typedef struct {
    _GAMRNB_Internal internal;
} GAMRNB_EncObj;

typedef struct {
    _GAMRNB_Internal internal;
} GAMRNB_DecObj;

/* 
 * Function prototypes
 */
void GAMRNB_encodeInit(
    GAMRNB_EncObj       *encObj_ptr,
    GAMRNB_VadType       vadType,
    GAMRNB_SpeechFormat  speechFormat,
    GAMRNB_DataFormat    dataFormat,
    GAMRNB_BitMaskMode   bitMaskMode);

void GAMRNB_decodeInit(
    GAMRNB_DecObj       *decObj_ptr,
    GAMRNB_SpeechFormat  speechFormat,
    GAMRNB_DataFormat    dataFormat,
    GAMRNB_BitMaskMode   bitMaskMode);

vint GAMRNB_encode(
    GAMRNB_EncObj   *encObj_ptr,
    vint            *speechIn_ptr,
    uint8           *packetOut_ptr,
    GAMRNB_RateMode  encRateMode,
    GAMRNB_DtxEnable dtxEnable,
    GAMRNB_TxStatus *txStatus_ptr);

void GAMRNB_decode(
    GAMRNB_DecObj    *decObj_ptr,
    uint8            *packetIn_ptr,
    vint             *speechOut_ptr,
    GAMRNB_ErrorFlag  errorFlag);

#endif
