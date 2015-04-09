/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004-2007 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 23655 $ $Date: 2013-12-20 14:04:57 +0800 (Fri, 20 Dec 2013) $
 *
 */

#include "osal.h"

#include "vtsp.h"
#include "_vtsp_private.h"

/*
 * --------
 *  This private layer contains very little error checking.
 *  All args & objs must be valid prior to calling these functions.
 * --------
 */

/*
 * All private globals are allocated below
 * --------
 */

_VTSP_Object *_VTSP_object_ptr = NULL;

/*
 * ======== _VTSP_setObj() ========
 */
VTSP_Return _VTSP_setObj(void)
{
    VTSP_QueryData *config_ptr;

    if (NULL == _VTSP_object_ptr) {
        return (VTSP_E_INIT);
    }

    config_ptr = &_VTSP_object_ptr->config;

    config_ptr->encoder.types = 
              VTSP_MASK_CODER_G711U
            | VTSP_MASK_CODER_G711A
#if defined(VTSP_ENABLE_G726)
            | VTSP_MASK_CODER_G726_32K
#endif
#if defined(VTSP_ENABLE_G729)
            | VTSP_MASK_CODER_G729
#endif
#if defined(VTSP_ENABLE_ILBC)
            | VTSP_MASK_CODER_ILBC_20MS
            | VTSP_MASK_CODER_ILBC_30MS
#endif
#if defined(VTSP_ENABLE_G723)
            | VTSP_MASK_CODER_G723_30MS
#endif
#if defined(VTSP_ENABLE_DTMFR)
            | VTSP_MASK_CODER_DTMF
            | VTSP_MASK_CODER_TONE
#endif
#if defined(VTSP_ENABLE_G722)
            | VTSP_MASK_CODER_G722
#endif
#if defined(VTSP_ENABLE_G722P1)
            | VTSP_MASK_CODER_G722P1_20MS
#endif
#if defined(VTSP_ENABLE_GAMRNB) || defined(VTSP_ENABLE_GAMRNB_ACCELERATOR)
            | VTSP_MASK_CODER_GAMRNB_20MS_OA
            | VTSP_MASK_CODER_GAMRNB_20MS_BE
#endif
#if defined(VTSP_ENABLE_GAMRWB) || defined(VTSP_ENABLE_GAMRWB_ACCELERATOR)
            | VTSP_MASK_CODER_GAMRWB_20MS_OA
            | VTSP_MASK_CODER_GAMRWB_20MS_BE
#endif
#if defined(VTSP_ENABLE_G711P1)
            | VTSP_MASK_CODER_G711P1U
            | VTSP_MASK_CODER_G711P1A
#endif
#if defined(VTSP_ENABLE_SILK)
            | VTSP_MASK_CODER_SILK_20MS_8K
            | VTSP_MASK_CODER_SILK_20MS_16K
            | VTSP_MASK_CODER_SILK_20MS_24K
#endif
#if defined(VTSP_ENABLE_T38)
            | VTSP_MASK_CODER_T38
#endif
            | VTSP_MASK_CODER_CN;

    config_ptr->encoder.silenceComp =
              VTSP_MASK_CODER_G711U
            | VTSP_MASK_CODER_G711A
#if defined(VTSP_ENABLE_G726)
            | VTSP_MASK_CODER_G726_32K
#endif
#if defined(VTSP_ENABLE_G729)
            | VTSP_MASK_CODER_G729
#endif
#if defined(VTSP_ENABLE_T38)
            | VTSP_MASK_CODER_T38
#endif
            ;

    config_ptr->encoder.dtmfRelay =
              VTSP_MASK_CODER_G711U
            | VTSP_MASK_CODER_G711A
#if defined(VTSP_ENABLE_G726)
            | VTSP_MASK_CODER_G726_32K
#endif
#if defined(VTSP_ENABLE_G729)
            | VTSP_MASK_CODER_G729
#endif
#if defined(VTSP_ENABLE_ILBC)
            | VTSP_MASK_CODER_ILBC_20MS
            | VTSP_MASK_CODER_ILBC_30MS
#endif
#if defined(VTSP_ENABLE_G723)
            | VTSP_MASK_CODER_G723_30MS
#endif
#if defined(VTSP_ENABLE_G722)
            | VTSP_MASK_CODER_G722
#endif
#if defined(VTSP_ENABLE_G722P1)
            | VTSP_MASK_CODER_G722P1_20MS
#endif        
#if defined(VTSP_ENABLE_GAMRNB) || defined(VTSP_ENABLE_GAMRNB_ACCELERATOR)
            | VTSP_MASK_CODER_GAMRNB_20MS_OA
            | VTSP_MASK_CODER_GAMRNB_20MS_BE
#endif
#if defined(VTSP_ENABLE_GAMRWB) || defined(VTSP_ENABLE_GAMRWB_ACCELERATOR)
            | VTSP_MASK_CODER_GAMRWB_20MS_OA
            | VTSP_MASK_CODER_GAMRWB_20MS_BE
#endif
#if defined(VTSP_ENABLE_SILK)
            | VTSP_MASK_CODER_SILK_20MS_8K
            | VTSP_MASK_CODER_SILK_20MS_16K
            | VTSP_MASK_CODER_SILK_20MS_24K
#endif
#if defined(VTSP_ENABLE_G711P1)
            | VTSP_MASK_CODER_G711P1U
            | VTSP_MASK_CODER_G711P1A
#endif
            ;

    config_ptr->encoder.pktRate = VTSP_MASK_CODER_RATE_SET1_10MS;

    config_ptr->encoder.bitRate = 0
#if defined(VTSP_ENABLE_G726)        
            | VTSP_MASK_CODER_BITRATE_G726_32
#endif
#if defined(VTSP_ENABLE_G723_30MS)
            | VTSP_MASK_CODER_BITRATE_G723_30MS_53
            | VTSP_MASK_CODER_BITRATE_G723_30MS_63
#endif
#if defined(VTSP_ENABLE_G722P1_20MS)
            | VTSP_MASK_CODER_BITRATE_G722P1_20MS_24
            | VTSP_MASK_CODER_BITRATE_G722P1_20MS_32
#endif
#if defined(VTSP_ENABLE_GAMRNB) || defined(VTSP_ENABLE_GAMRNB_ACCELERATOR)
            | VTSP_MASK_CODER_BITRATE_GAMRNB_20MS_475
            | VTSP_MASK_CODER_BITRATE_GAMRNB_20MS_515
            | VTSP_MASK_CODER_BITRATE_GAMRNB_20MS_59
            | VTSP_MASK_CODER_BITRATE_GAMRNB_20MS_67
            | VTSP_MASK_CODER_BITRATE_GAMRNB_20MS_74
            | VTSP_MASK_CODER_BITRATE_GAMRNB_20MS_795
            | VTSP_MASK_CODER_BITRATE_GAMRNB_20MS_102
            | VTSP_MASK_CODER_BITRATE_GAMRNB_20MS_122
#endif
#if defined(VTSP_ENABLE_GAMRWB) || defined(VTSP_ENABLE_GAMRWB_ACCELERATOR)
            | VTSP_MASK_CODER_BITRATE_GAMRWB_20MS_660
            | VTSP_MASK_CODER_BITRATE_GAMRWB_20MS_885
            | VTSP_MASK_CODER_BITRATE_GAMRWB_20MS_1265
            | VTSP_MASK_CODER_BITRATE_GAMRWB_20MS_1425
            | VTSP_MASK_CODER_BITRATE_GAMRWB_20MS_1585
            | VTSP_MASK_CODER_BITRATE_GAMRWB_20MS_1825
            | VTSP_MASK_CODER_BITRATE_GAMRWB_20MS_1985
            | VTSP_MASK_CODER_BITRATE_GAMRWB_20MS_2305
            | VTSP_MASK_CODER_BITRATE_GAMRWB_20MS_2385
#endif
#if defined(VTSP_ENABLE_G711P1)
            | VTSP_MASK_CODER_BITRATE_G711P1_R3_96
            | VTSP_MASK_CODER_BITRATE_G711P1_R2_80
            | VTSP_MASK_CODER_BITRATE_G711P1_R1_64
#endif
            ;

    config_ptr->decoder.types =
              VTSP_MASK_CODER_G711U
            | VTSP_MASK_CODER_G711A
#if defined(VTSP_ENABLE_G726)
            | VTSP_MASK_CODER_G726_32K
#endif
#if defined(VTSP_ENABLE_G729)
            | VTSP_MASK_CODER_G729
#endif
#if defined(VTSP_ENABLE_ILBC)
            | VTSP_MASK_CODER_ILBC_20MS
            | VTSP_MASK_CODER_ILBC_30MS
#endif
#if defined(VTSP_ENABLE_G723)
            | VTSP_MASK_CODER_G723_30MS
#endif
#if defined(VTSP_ENABLE_DTMFR)
            | VTSP_MASK_CODER_DTMF
            | VTSP_MASK_CODER_TONE
#endif
#if defined(VTSP_ENABLE_T38)
            | VTSP_MASK_CODER_T38
#endif
#if defined(VTSP_ENABLE_G722)
            | VTSP_MASK_CODER_G722
#endif
#if defined(VTSP_ENABLE_G722P1)
            | VTSP_MASK_CODER_G722P1_20MS
#endif
#if defined(VTSP_ENABLE_GAMRNB) || defined(VTSP_ENABLE_GAMRNB_ACCELERATOR)
            | VTSP_MASK_CODER_GAMRNB_20MS_OA
            | VTSP_MASK_CODER_GAMRNB_20MS_BE
#endif
#if defined(VTSP_ENABLE_GAMRWB) || defined(VTSP_ENABLE_GAMRWB_ACCELERATOR)
            | VTSP_MASK_CODER_GAMRWB_20MS_OA
            | VTSP_MASK_CODER_GAMRWB_20MS_BE
#endif
#if defined(VTSP_ENABLE_G711P1)
            | VTSP_MASK_CODER_G711P1U
            | VTSP_MASK_CODER_G711P1A
#endif
#if defined(VTSP_ENABLE_SILK)
            | VTSP_MASK_CODER_SILK_20MS_8K
            | VTSP_MASK_CODER_SILK_20MS_16K
            | VTSP_MASK_CODER_SILK_20MS_24K
#endif
#if defined(VTSP_ENABLE_T38)
            | VTSP_MASK_CODER_T38
#endif
            | VTSP_MASK_CODER_CN;

    config_ptr->decoder.silenceComp =
              VTSP_MASK_CODER_G711U
            | VTSP_MASK_CODER_G711A
#if defined(VTSP_ENABLE_G726)
            | VTSP_MASK_CODER_G726_32K
#endif
#if defined(VTSP_ENABLE_G729)
            | VTSP_MASK_CODER_G729
#endif
            ;

    config_ptr->decoder.dtmfRelay =
              VTSP_MASK_CODER_G711U
            | VTSP_MASK_CODER_G711A
#if defined(VTSP_ENABLE_G726)
            | VTSP_MASK_CODER_G726_32K
#endif
#if defined(VTSP_ENABLE_G729)
            | VTSP_MASK_CODER_G729
#endif
#if defined(VTSP_ENABLE_ILBC)
            | VTSP_MASK_CODER_ILBC_20MS
            | VTSP_MASK_CODER_ILBC_30MS
#endif
#if defined(VTSP_ENABLE_G723)
            | VTSP_MASK_CODER_G723_30MS
#endif
#if defined(VTSP_ENABLE_G722)
            | VTSP_MASK_CODER_G722
#endif
#if defined(VTSP_ENABLE_G722P1)
            | VTSP_MASK_CODER_G722P1_20MS
#endif
#if defined(VTSP_ENABLE_GAMRNB) || defined(VTSP_ENABLE_GAMRNB_ACCELERATOR)
            | VTSP_MASK_CODER_GAMRNB_20MS_OA
            | VTSP_MASK_CODER_GAMRNB_20MS_BE
#endif
#if defined(VTSP_ENABLE_GAMRWB) || defined(VTSP_ENABLE_GAMRWB_ACCELERATOR)
            | VTSP_MASK_CODER_GAMRWB_20MS_OA
            | VTSP_MASK_CODER_GAMRWB_20MS_BE
#endif
#if defined(VTSP_ENABLE_SILK)
            | VTSP_MASK_CODER_SILK_20MS_8K
            | VTSP_MASK_CODER_SILK_20MS_16K
            | VTSP_MASK_CODER_SILK_20MS_24K
#endif
#if defined(VTSP_ENABLE_G711P1)
            | VTSP_MASK_CODER_G711P1U
            | VTSP_MASK_CODER_G711P1A
#endif
            ;

    config_ptr->decoder.rate = VTSP_MASK_CODER_RATE_SET1_10MS;

    config_ptr->stream.numStreams = _VTSP_STREAM_NUM;
    config_ptr->stream.numPerInfc = _VTSP_STREAM_PER_INFC;

#ifdef VTSP_ENABLE_SRTP
    config_ptr->stream.srtpEnabled = 1;
#else
    config_ptr->stream.srtpEnabled = 0;
#endif
    
    config_ptr->detect.types =
            VTSP_DETECT_DTMF | 
            VTSP_DETECT_FMTD;

#if defined(VTSP_ENABLE_T38)
    config_ptr->faxModem.coder = VTSP_MASK_CODER_T38;
#else
    config_ptr->faxModem.coder = VTSP_MASK_CODER_G711U;     /* No T.38 */
#endif

    config_ptr->tone.numTemplateIds = _VTSP_NUM_TONE_TEMPL;
    config_ptr->tone.maxSequenceLen = _VTSP_NUM_TONE_SEQUENCE_MAX;
    config_ptr->ring.numTemplateIds = _VTSP_NUM_RING_TEMPL;

#ifdef VTSP_ENABLE_FXO
    config_ptr->hw.numFxo = _VTSP_INFC_FXO_NUM;
#else
    config_ptr->hw.numFxo = 0;
#endif
    config_ptr->hw.numInfc        = _VTSP_INFC_NUM;
    config_ptr->hw.numFxs         = _VTSP_INFC_FXS_NUM;
    config_ptr->hw.fxsFirst       = _VTSP_INFC_FXS_FIRST;
    config_ptr->hw.fxsLast        = _VTSP_INFC_FXS_LAST;
    config_ptr->hw.fxoFirst       = _VTSP_INFC_FXO_FIRST;
    config_ptr->hw.fxoLast        = _VTSP_INFC_FXO_LAST;
    config_ptr->hw.fxoFlashTime   = _VTSP_INFC_FXO_FLASH_MS;
    
    config_ptr->hw.numAudio       = _VTSP_INFC_AUDIO_NUM;
    config_ptr->hw.audioFirst     = _VTSP_INFC_AUDIO_FIRST;
    config_ptr->hw.audioLast      = _VTSP_INFC_AUDIO_LAST;
    config_ptr->flow.payloadMaxSz = _VTSP_Q_FLOW_PAYLOAD_SZ;
    config_ptr->flow.keyMask      = _VTSP_Q_FLOW_PUT_MASK;
    return (VTSP_OK);
}

#if 0
/*
 * ======== _VTSP_msgSizePrint() ========
 */
VTSP_Return _VTSP_msgSizePrint(void)
{
    /*
     * Useful code for printing sizes of the command messages
     */
    OSAL_logMsg("%s:%d size of _VTSP_CmdMsg is: %d bytes\n", 
            (int)__FILE__, __LINE__, sizeof(_VTSP_CmdMsg), 0);
    OSAL_logMsg("%s:%d size of _VTSP_CmdMsgArg is: %d bytes\n", 
            (int)__FILE__, __LINE__, sizeof(_VTSP_CmdMsgArg), 0);
    OSAL_logMsg("%s:%d size of VTSP_Stream is: %d bytes\n", 
            (int)__FILE__, __LINE__, sizeof(VTSP_Stream), 0);
    OSAL_logMsg("%s:%d size of _VTSP_CmdMsgConfig is: %d bytes\n", 
            (int)__FILE__, __LINE__, sizeof(_VTSP_CmdMsgConfig), 0);
    OSAL_logMsg("%s:%d size of _VTSP_CmdMsgControl is: %d bytes\n", 
            (int)__FILE__, __LINE__, sizeof(_VTSP_CmdMsgControl), 0);
    OSAL_logMsg("%s:%d size of _VTSP_CmdMsgToneSequence is: %d bytes\n", 
            (int)__FILE__, __LINE__, sizeof(_VTSP_CmdMsgToneSequence), 0);
    OSAL_logMsg("%s:%d size of _VTSP_CmdMsgFlowOpen is: %d bytes\n", 
            (int)__FILE__, __LINE__, sizeof(_VTSP_CmdMsgFlowOpen), 0);
    OSAL_logMsg("%s:%d size of _VTSP_CmdMsgFlow is: %d bytes\n", 
            (int)__FILE__, __LINE__, sizeof(_VTSP_CmdMsgFlow), 0);
    OSAL_logMsg("%s:%d size of _VTSP_CmdMsgRtcpCname is: %d bytes\n", 
            (int)__FILE__, __LINE__, sizeof(_VTSP_CmdMsgRtcpCname), 0);

    return(VTSP_OK);
}
#endif
