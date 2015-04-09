/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2005 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 29023 $ $Date: 2014-09-25 17:28:52 +0800 (Thu, 25 Sep 2014) $
 */

#ifndef _DSPEXT_H_
#define _DSPEXT_H_

#define DSPEXT_FRAME_MAX_SZ                 (64)

typedef enum {
    DSPEXT_AMRNB_FRAME_TYPE_475         = 0,
    DSPEXT_AMRNB_FRAME_TYPE_515,
    DSPEXT_AMRNB_FRAME_TYPE_590,
    DSPEXT_AMRNB_FRAME_TYPE_670,
    DSPEXT_AMRNB_FRAME_TYPE_740,
    DSPEXT_AMRNB_FRAME_TYPE_795,
    DSPEXT_AMRNB_FRAME_TYPE_1020,
    DSPEXT_AMRNB_FRAME_TYPE_1220,
    DSPEXT_AMRNB_FRAME_TYPE_AMR_SID,
    DSPEXT_AMRNB_FRAME_TYPE_GSM_EFR_SID,
    DSPEXT_AMRNB_FRAME_TYPE_TDMA_EFR_SID,
    DSPEXT_AMRNB_FRAME_TYPE_PDC_EFR_SID,
    DSPEXT_AMRNB_FRAME_TYPE_SPEECH_LOST = 14,
    DSPEXT_AMRNB_FRAME_TYPE_NO_DATA     = 15,
} DSPExt_AMRNBFrameType;

typedef enum {
    DSPEXT_AMRWB_FRAME_TYPE_660         = 0,
    DSPEXT_AMRWB_FRAME_TYPE_885,
    DSPEXT_AMRWB_FRAME_TYPE_1265,
    DSPEXT_AMRWB_FRAME_TYPE_1425,
    DSPEXT_AMRWB_FRAME_TYPE_1585,
    DSPEXT_AMRWB_FRAME_TYPE_1825,
    DSPEXT_AMRWB_FRAME_TYPE_1985,
    DSPEXT_AMRWB_FRAME_TYPE_2305,
    DSPEXT_AMRWB_FRAME_TYPE_2385,
    DSPEXT_AMRWB_FRAME_TYPE_SID         = 9,
    DSPEXT_AMRWB_FRAME_TYPE_SPEECH_LOST = 14,
    DSPEXT_AMRWB_FRAME_TYPE_NO_DATA     = 15,
} DSPExt_AMRWBFrameType;

typedef enum {
    DSPEXT_ERR,
    DSPEXT_OK,
} DSPExt_Return;

typedef enum {
    DSPEXT_CODER_TYPE_UNKNOWN  = 0,
    DSPEXT_CODER_TYPE_AMRNB_OA,
    DSPEXT_CODER_TYPE_AMRNB_BE,
    DSPEXT_CODER_TYPE_AMRWB_OA,
    DSPEXT_CODER_TYPE_AMRWB_BE,
} DSPExt_CoderType;

typedef enum {
    DSPEXT_RX_TYPE_SPEECH_GOOD = 0,
    DSPEXT_RX_TYPE_SPEECH_BAD  = 3,
    DSPEXT_RX_TYPE_SID_FIRST   = 4,
    DSPEXT_RX_TYPE_UPDATA      = 5,
    DSPEXT_RX_TYPE_SID_BAD     = 6,
    DSPEXT_RX_TYPE_NO_DATA     = 7,
    DSPEXT_RX_TYPE_SPEECH_LOST = 8,
} DSPExt_RxType;

typedef enum {
    DSPEXT_TX_TYPE_SPEECH_GOOD     = 0, 
    DSPEXT_TX_TYPE_SID_FIRST       = 1,
    DSPEXT_TX_TYPE_SID_UPDATA      = 2,
    DSPEXT_TX_TYPE_NO_DATA         = 3,
    DSPEXT_TX_TYPE_SPEECH_DEGRADED = 4,
    DSPEXT_TX_TYPE_SPEECH_BAD      = 5,
    DSPEXT_TX_TYPE_SID_BAD         = 6,
    DSPEXT_TX_TYPE_ONSET           = 7,
    DSPEXT_TX_TYPE_N_FRAMETYPES    = 8,
    DSPEXT_TX_TYPE_SPEECH_LOST     = 9,
} DSPExt_TxType;

typedef struct  {
    uint8               frameXmit_arry[DSPEXT_FRAME_MAX_SZ]; // to DSP
    uint8               frameRecv_arry[DSPEXT_FRAME_MAX_SZ]; // from DSP

    OSAL_SemId          frameSem;
    vint                app10msflag;
    OSAL_Boolean        isRead;
    OSAL_SemId          readMutex;

    DSPExt_CoderType    currCoderType;
    OSAL_Boolean        isCodecStart; /* 0: not started, 1: started */
    vint                vadEnable;    /* 0: no VAD, 1: VAD */
    vint                rateMode;     /* AMR-WB is 0-8 */
    OSAL_Boolean        isSid;
} DspExt_Obj;

/*
 * Software API in host processor
 */
DSPExt_Return DSPExt_init(
    void);

void DSPExt_shutdown(
    void);

DSPExt_Return DSPExt_codecStart(
    DSPExt_CoderType    coderType,
    int                 rateMode,
    int                 vadEnable);

DSPExt_Return DSPExt_codecStop(
    void);

int DSPExt_Encode(
    DSPExt_CoderType    coderType,
    uint8              *dst_ptr);

void DSPExt_Decode(
    DSPExt_CoderType    coderType,
    uint8              *src_ptr,
    uvint               pSize);

#endif // _DSPEXT_H_
