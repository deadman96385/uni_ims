/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2006-2010 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 28422 $ $Date: 2014-08-22 11:55:05 +0800 (Fri, 22 Aug 2014) $
 *
 * Author: C M Garrido
 */

#include "vtspr.h"
#include "_vtspr_private.h"

#ifdef VTSP_ENABLE_ILBC
static void _VTSPR_ILBC_format16ToVint(vint*, int16*, vint);
#endif

/*
 * =========== _VTSPR_multiDecode() ===============
 *
 * This function is a generic wrapper for calling decoder routine of multi-frame
 * speech coders such as iLBC and G723.  New coders which use multi-tasking
 * should be added to the switch statement below.
 */

vint _VTSPR_multiDecode(
    vint            *src_ptr,
    vint            *dst_ptr,
    vint             payloadSize,
    VTSPR_StreamObj *stream_ptr,
    vint             decType,
    vint             infc)
{
    _VTSPR_MultiDecObj *multiDecObj_ptr = &(stream_ptr->multiDecObj);
    VTSP_Stream        *streamParam_ptr = &(stream_ptr->streamParam);
    vint               *recvSpeech_ptr;
    vint               *playSpeech_ptr;
    vint                decOffset;

    recvSpeech_ptr = multiDecObj_ptr->recvSpeech_ary;
    playSpeech_ptr = multiDecObj_ptr->playSpeech_ary;
    decOffset      = multiDecObj_ptr->decOffset;

    if ((0 == payloadSize) && ((VTSP_CODER_G726_32K == decType) ||
                (VTSP_CODER_G722 == decType))) {
        /* Turn on plcFlag, coders use external PLC module */
        return (1);
    }

    /*
     * If decOffset is 0, the payload is decoded producing multiple frames of
     * speech. If payload is available but decOffset is not 0, then
     * frames of PLC speech is produced. The speech is played to dst_ptr 10ms
     * at a time, in a staggered fashion.
     */
    if ((NULL != src_ptr) && (0 != decOffset)) {
        /*
         * Jitter buffer has produced a packet that is not on a mutli-frame
         * boundry. Resynch the decOffset logic, by setting decOffset to
         * zero.
         *
         * !!! Important Note !!!
         * Note, this case is quite normal between untransmitted frames, and 
         * between silence suppression periods.
         * However, this case is a JB error when plc is not active,
         * because JB has produced a multi-frame speech packet which is not on a
         * multi-frame boundary.
         */
        decOffset = 0;
    }

    if (0 == decOffset) {
        /*
         * Send packets to task for decoding only when Jitter Buffer has a
         * packet ready.
         */
        _VTSPR_multiDecodeMsgToTask(src_ptr, multiDecObj_ptr, streamParam_ptr,
                payloadSize, decType, infc);
        /* Reset decOffset to 0 */
        multiDecObj_ptr->decOffset = 0;
    }

    /*
     * Check the queue for decoded packets received from task. 
     */
    _VTSPR_multiDecodeMsgFromTask(multiDecObj_ptr,
            recvSpeech_ptr);
   
    if (0 == decOffset) {
        /* 
         * Copy recv buffer to play buffer once the play buffer has been played
         */
        OSAL_memCpy(playSpeech_ptr, recvSpeech_ptr,
                (VTSPR_MULTI_NSAMPLE_MAX * sizeof(vint)));
    }

    /* 
     * Fill the audio buffer with 10ms of decoded speech
     */
    switch(decType) {
        /*
         * 10ms Coders
         */
#if defined(VTSP_ENABLE_G729_ACCELERATOR) || \
    defined(VTSP_ENABLE_G726_ACCELERATOR) || \
    defined(VTSP_ENABLE_G711P1_ACCELERATOR)
#ifdef VTSP_ENABLE_G729_ACCELERATOR
        case VTSP_CODER_G729:
#endif    
#ifdef VTSP_ENABLE_G726_ACCELERATOR
        case VTSP_CODER_G726_32K:
#endif
#ifdef VTSP_ENABLE_G711P1_ACCELERATOR
        case VTSP_CODER_G711P1U:
        case VTSP_CODER_G711P1A:
#endif
            _VTSPR_multiDecodeBuffer10(dst_ptr, 
                    playSpeech_ptr, multiDecObj_ptr);
            break;
#endif

        /*
         * 20ms Coders
         */
#if defined(VTSP_ENABLE_ILBC) || defined(VTSP_ENABLE_G722P1) || \
    defined(VTSP_ENABLE_GAMRNB) || defined(VTSP_ENABLE_GAMRWB) || \
    defined(VTSP_ENABLE_SILK)
#ifdef VTSP_ENABLE_ILBC 
        case VTSP_CODER_ILBC_20MS:
#endif
#ifdef VTSP_ENABLE_G722P1
        case VTSP_CODER_G722P1_20MS:
#endif   
#ifdef VTSP_ENABLE_GAMRNB
        case VTSP_CODER_GAMRNB_20MS_OA:
        case VTSP_CODER_GAMRNB_20MS_BE:
#endif
#ifdef VTSP_ENABLE_GAMRWB
        case VTSP_CODER_GAMRWB_20MS_OA:
        case VTSP_CODER_GAMRWB_20MS_BE:
#endif
#ifdef VTSP_ENABLE_SILK
        case VTSP_CODER_SILK_20MS_8K:
        case VTSP_CODER_SILK_20MS_16K:
        case VTSP_CODER_SILK_20MS_24K:
#endif
            _VTSPR_multiDecodeBuffer20(dst_ptr, 
                    playSpeech_ptr, multiDecObj_ptr);
            break;
#endif

        /*
         * 30ms Coders
         */
#if defined(VTSP_ENABLE_ILBC) || defined(VTSP_ENABLE_G723)
#ifdef VTSP_ENABLE_ILBC
        case VTSP_CODER_ILBC_30MS:
#endif
#ifdef VTSP_ENABLE_G723
        case VTSP_CODER_G723_30MS:
#endif
            _VTSPR_multiDecodeBuffer30(dst_ptr, 
                    playSpeech_ptr, multiDecObj_ptr);
            break;
#endif

        default:;
            /* shouldn't get here */
    }

    /* Return plcActive to indicate if plc is needed. */
    return (multiDecObj_ptr->plcActive);
}

/*
 * =========== _VTSPR_multiDecodeBuffer30() ===============
 *
 * This function plays 10ms of data in the decoded speech buffer 
 */
void _VTSPR_multiDecodeBuffer30(
    vint               *dst_ptr,
    vint               *decodedSpeech_ptr,
    _VTSPR_MultiDecObj *multiDecObj_ptr)
{
    vint    decOffset = multiDecObj_ptr->decOffset;
    vint    coderSamples10ms = multiDecObj_ptr->coderSamples10ms;

    /*
     * Play 10ms of speech
     */
    OSAL_memCpy(dst_ptr, 
            (decodedSpeech_ptr + decOffset),
            (coderSamples10ms * sizeof(vint)));
    
    if ((2 * coderSamples10ms) == decOffset) {
        /* Whole buffer has played, reset decOffset */
        multiDecObj_ptr->decOffset = 0;
    }
    else {
        /* Increment offset to play next 10MS of speech next time */
        multiDecObj_ptr->decOffset += coderSamples10ms;
    }
}

/*
 * =========== _VTSPR_multiDecodeBuffer20() ===============
 *
 * This function plays 10ms of data in the decoded speech buffer 
 */
void _VTSPR_multiDecodeBuffer20(
    vint               *dst_ptr,
    vint               *decodedSpeech_ptr,
    _VTSPR_MultiDecObj *multiDecObj_ptr)
{
    vint    decOffset = multiDecObj_ptr->decOffset;
    vint    coderSamples10ms = multiDecObj_ptr->coderSamples10ms;

    /*
     * Play 10ms of speech
     */
    OSAL_memCpy(dst_ptr, 
            (decodedSpeech_ptr + decOffset),
            (coderSamples10ms * sizeof(vint)));
    
    if (0 == decOffset) {
        /* Increment offset to play next 10MS of speech next time */
        multiDecObj_ptr->decOffset += coderSamples10ms;
    }
    else if (coderSamples10ms == decOffset) {
        /* Whole buffer has played, reset decOffset */
        multiDecObj_ptr->decOffset = 0;
    }
}

/*
 * =========== _VTSPR_multiDecodeBuffer10() ===============
 *
 * This function places 10ms of data in a buffer 
 */
void _VTSPR_multiDecodeBuffer10(
    vint               *dst_ptr,
    vint               *decodedSpeech_ptr,
    _VTSPR_MultiDecObj *multiDecObj_ptr)
{
    vint    coderSamples10ms = multiDecObj_ptr->coderSamples10ms;

    /*
     * Play 10ms of speech
     */
    OSAL_memCpy(dst_ptr, decodedSpeech_ptr,
            (coderSamples10ms * sizeof(vint)));
}

/* 
 * ======== _VTSPR_multiDecodeMsgToTask() ========
 */
void _VTSPR_multiDecodeMsgToTask(
    vint               *src_ptr,    
    _VTSPR_MultiDecObj *multiDecObj_ptr,
    VTSP_Stream        *streamParam_ptr,
    vint                payloadSize,
    vint                decType,
    vint                infc)
{
    /*
     * Copy packet into the message
     */
    OSAL_memCpy(multiDecObj_ptr->pktMsg.pkt_ary, src_ptr, payloadSize);

    multiDecObj_ptr->pktMsg.cmd         = _VTSPR_MULTI_CODER_CMD_RUN;
    multiDecObj_ptr->pktMsg.decType     = decType;
    multiDecObj_ptr->pktMsg.infc        = infc;
    multiDecObj_ptr->pktMsg.streamId    = streamParam_ptr->streamId;
    multiDecObj_ptr->pktMsg.extension   = streamParam_ptr->extension;
    multiDecObj_ptr->pktMsg.payloadSize = payloadSize;

    /*
     * Send data to task for decoding.
     */
    if (OSAL_SUCCESS != OSAL_msgQSend(multiDecObj_ptr->pktData,
            (char *)(&(multiDecObj_ptr->pktMsg)),
            VTSPR_Q_MULTI_DECODE_PKT_MSG_SIZE, OSAL_NO_WAIT, NULL)) {
        _VTSP_TRACE(__FILE__, __LINE__);
    }
    /* 
     * If init flag was set, clear it
     */
    if (1 == multiDecObj_ptr->pktMsg.initFlag) {
        multiDecObj_ptr->pktMsg.initFlag = 0;
    }
}

/* 
 * ======== _VTSPR_multiDecodeMsgFromTask() ========
 */
vint _VTSPR_multiDecodeMsgFromTask(
    _VTSPR_MultiDecObj *multiDecObj_ptr,
    vint               *dst_ptr)
{
    _VTSPR_MultiDecodedMsg msg;
    vint                 decMsgSz;

    decMsgSz = multiDecObj_ptr->decMsgSz;

    while (0 < OSAL_msgQRecv(multiDecObj_ptr->decData, (char *)&msg,
                VTSPR_Q_MULTI_DECODE_DEC_MSG_SIZE, OSAL_NO_WAIT, NULL)) {
        if (msg.msgSize <= decMsgSz) {
            OSAL_memCpy(dst_ptr, &(msg.decoded_ary),
                    (msg.msgSize * sizeof(vint)));
            return (msg.msgSize);
        }
        else {
            /* Decoded msg too large; drop it; continue, to flush */
            OSAL_logMsg("%s:%d msgSizez=%d, decType=%d, decMsgSz=%d\n",
                    __FILE__, __LINE__, msg.msgSize,
                    multiDecObj_ptr->pktMsg.decType, decMsgSz);
            continue;
        } 
    }
    
    return (0);
}
