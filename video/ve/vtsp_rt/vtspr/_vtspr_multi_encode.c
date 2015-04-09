/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2006-2008 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 28422 $ $Date: 2014-08-22 11:55:05 +0800 (Fri, 22 Aug 2014) $
 *
 * Author: C M Garrido
 */

#include "vtspr.h"
#include "_vtspr_private.h"

/*
 * =========== _VTSPR_multiEncode() ===============
 *
 * This function is a generic wrapper for calling encoder routine of multi-frame
 * speech coders such as iLBC and G723.  New coders which use multi-tasking
 * should be added to the switch statement below.
 */

vint _VTSPR_multiEncode(
    vint            *src_ptr,
    vint            *dst_ptr,
    VTSPR_StreamObj *stream_ptr,
    vint             encType,
    vint             infc)
{
    _VTSPR_MultiEncObj *multiEncObj_ptr = &(stream_ptr->multiEncObj);
    VTSP_Stream        *streamParam_ptr = &(stream_ptr->streamParam);
    vint                msgReady = 0;
    vint                bytesToSend = 0;

    switch(encType) {
#if defined(VTSP_ENABLE_G729) || defined(VTSP_ENABLE_G726) || \
    defined(VTSP_ENABLE_G711P1)
        /*
         * 10ms Coders
         */
#ifdef VTSP_ENABLE_G729
        case VTSP_CODER_G729:
#endif    
#ifdef VTSP_ENABLE_G726
        case VTSP_CODER_G726_32K:
#endif 
#ifdef VTSP_ENABLE_G711P1
        case VTSP_CODER_G711P1U:
        case VTSP_CODER_G711P1A:
#endif    
            msgReady = _VTSPR_multiEncodeBuffer10(src_ptr, dst_ptr,
                    multiEncObj_ptr, stream_ptr);
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
            msgReady = _VTSPR_multiEncodeBuffer20(src_ptr, dst_ptr,
                                                multiEncObj_ptr, stream_ptr);
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
            msgReady = _VTSPR_multiEncodeBuffer30(src_ptr, dst_ptr,
                                        multiEncObj_ptr, stream_ptr, encType);
            break;
#endif

        default:;
            /* shouldn't get here */
    }

    /* send message if ready */
    if (msgReady) {
        _VTSPR_multiEncodeMsgToTask(multiEncObj_ptr, streamParam_ptr,
                                        encType, infc);
    }

    /*
     * Check the queue for encoded packets, which are ready. 
     */
    bytesToSend = _VTSPR_multiEncodeMsgFromTask(multiEncObj_ptr, dst_ptr);

    return (bytesToSend);
}

/*
 * =========== _VTSPR_multiEncodeBuffer30() ===============
 *
 * This function places 10ms of data in a buffer until the buffer holds
 * 30ms of data
 * 
 * Output: 1, if buffer holds 30ms of data
 *         0, else
 */

vint _VTSPR_multiEncodeBuffer30(
    vint               *src_ptr,
    vint               *dst_ptr,
    _VTSPR_MultiEncObj *multiEncObj_ptr,
    VTSPR_StreamObj    *stream_ptr,  
    vint                encType)
{
    vint                 msgReady = 0;
    vint                *payload_ptr;
    vint                 encOffset;
    vint                 coderSamples10ms;
    

    encOffset   = multiEncObj_ptr->encOffset;
    payload_ptr = multiEncObj_ptr->rawMsg.enc_ary;
    coderSamples10ms = multiEncObj_ptr->coderSamples10ms;

    if (0 == encOffset) {
        /*
         * Store first 10ms of speech to be encoded.
         */
        OSAL_memCpy(payload_ptr, src_ptr,
                coderSamples10ms * sizeof(vint));
        multiEncObj_ptr->encOffset = coderSamples10ms;
    }
    else if (coderSamples10ms == encOffset) {
        /*
         * Store second 10ms of speech to be encoded.
         */
        OSAL_memCpy(payload_ptr + encOffset, src_ptr,
                coderSamples10ms * sizeof(vint));
        multiEncObj_ptr->encOffset += coderSamples10ms;
    }
    else {
        /*
         * Store final 10ms of speech to form 30ms of speech.
         */
        OSAL_memCpy(payload_ptr + encOffset, src_ptr,
                coderSamples10ms * sizeof(vint));
        multiEncObj_ptr->encOffset = 0;

        /* Set msgReady */
        msgReady = 1;
    }

    return (msgReady);
}


/*
 * =========== _VTSPR_multiEncodeBuffer20() ===============
 *
 * This function places 10ms of data in a buffer until the buffer holds
 * 20ms of data
 * 
 * Output: 1, if buffer holds 30ms of data
 *         0, else
 */

vint _VTSPR_multiEncodeBuffer20(
    vint               *src_ptr,
    vint               *dst_ptr,
    _VTSPR_MultiEncObj *multiEncObj_ptr,
    VTSPR_StreamObj    *stream_ptr)  
{
    vint                 msgReady = 0;
    vint                *payload_ptr;
    vint                 encOffset;
    vint                 coderSamples10ms;

    encOffset   = multiEncObj_ptr->encOffset;
    payload_ptr = multiEncObj_ptr->rawMsg.enc_ary;
    coderSamples10ms = multiEncObj_ptr->coderSamples10ms;

    if (0 == encOffset) {
//        OSAL_logMsg("%s:%d, encOffset=%d, coderSamples10ms=%d\n", __FUNCTION__, __LINE__, encOffset, coderSamples10ms);
        /*
         * Store first 10ms of speech to be encoded.
         */
        OSAL_memCpy(payload_ptr, src_ptr,
                coderSamples10ms * sizeof(vint));
        multiEncObj_ptr->encOffset = coderSamples10ms;
    }
    else {
//        OSAL_logMsg("%s:%d, encOffset=%d\n", __FUNCTION__, __LINE__, encOffset);
        /*
         * Store final 10ms of speech to form 20ms of speech.
         */
        OSAL_memCpy(payload_ptr + encOffset, src_ptr,
                coderSamples10ms * sizeof(vint));
        multiEncObj_ptr->encOffset = 0;

        /* Set msgReady */
        msgReady = 1;
    }

    return (msgReady);
}

/*
 * =========== _VTSPR_multiEncodeBuffer10() ===============
 *
 * This function places 10ms of data in a buffer 
 */

vint _VTSPR_multiEncodeBuffer10(
    vint               *src_ptr,
    vint               *dst_ptr,
    _VTSPR_MultiEncObj *multiEncObj_ptr,
    VTSPR_StreamObj    *stream_ptr)  
{
    vint                 msgReady = 0;
    vint                *payload_ptr;
    vint                 coderSamples10ms;

    payload_ptr = multiEncObj_ptr->rawMsg.enc_ary;
    coderSamples10ms = multiEncObj_ptr->coderSamples10ms;

    /*
     * Store 10ms of speech to be encoded.
     */
    OSAL_memCpy(payload_ptr, src_ptr,
            coderSamples10ms * sizeof(vint));

    /* Set msgReady */
    msgReady = 1;

    return (msgReady);
}

/* 
 * ======== _VTSPR_multiEncodeMsgToTask() ========
 */
void _VTSPR_multiEncodeMsgToTask(
    _VTSPR_MultiEncObj *multiEncObj_ptr,
    VTSP_Stream        *streamParam,
    vint                encType,
    vint                infc)
{
    multiEncObj_ptr->rawMsg.cmd         = _VTSPR_MULTI_CODER_CMD_RUN;
    multiEncObj_ptr->rawMsg.encType     = encType;
    multiEncObj_ptr->rawMsg.infc        = infc;
    multiEncObj_ptr->rawMsg.streamId    = streamParam->streamId;
    multiEncObj_ptr->rawMsg.silenceComp = streamParam->silenceComp;
    multiEncObj_ptr->rawMsg.extension   = streamParam->extension;

    /*
     * Send data to task for encoding.
     */
    if (OSAL_SUCCESS != OSAL_msgQSend(multiEncObj_ptr->rawData,
        (char *)(&(multiEncObj_ptr->rawMsg)),
        VTSPR_Q_MULTI_ENCODE_RAW_MSG_SIZE, OSAL_NO_WAIT, NULL)) {
                _VTSP_TRACE(__FILE__, __LINE__);
    }
    /* If init flag was set, clear it */
    if (1 == multiEncObj_ptr->rawMsg.initFlag) {
        multiEncObj_ptr->rawMsg.initFlag = 0;
    }
}

/* 
 * ======== _VTSPR_multiEncodeMsgFromTask() ========
 */
vint _VTSPR_multiEncodeMsgFromTask(
    _VTSPR_MultiEncObj *multiEncObj_ptr,
    vint               *dst_ptr)
{
    _VTSPR_MultiCodedMsg msg;
    vint                 encMsgSz;

    encMsgSz = multiEncObj_ptr->encMsgSz;

    while (OSAL_msgQRecv(multiEncObj_ptr->encData, (char *)&msg,
                VTSPR_Q_MULTI_ENCODE_ENC_MSG_SIZE, OSAL_NO_WAIT, NULL) > 0) {
        if (msg.msgSize <= encMsgSz) {
            OSAL_memCpy(dst_ptr, &(msg.coded_ary), msg.msgSize);
            return (msg.msgSize);
        }
        else {
            /* Encoded msg too large; drop it; continue, to flush */
            OSAL_logMsg("%s:%d msgSizez=%d, encType=%d\n", __FILE__,
                    __LINE__, msg.msgSize,
                    multiEncObj_ptr->rawMsg.encType);
            continue;
        } 
    }
    
    return (0);
}
