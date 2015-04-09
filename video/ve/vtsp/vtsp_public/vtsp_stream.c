/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004-2008 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 23756 $ $Date: 2013-12-26 12:01:04 +0800 (Thu, 26 Dec 2013) $
 *
 */

#include "osal.h"
#include "vtsp.h"
#include "../vtsp_private/_vtsp_private.h"

/*
 * ======== VTSP_streamStart ========
 */
VTSP_Return VTSP_streamStart(
    uvint            infc,
    VTSP_Stream     *stream_ptr)
{
    VTSP_Return      e;
    _VTSP_CmdMsg     cmd;

    _VTSP_VERIFY_INIT;
    _VTSP_VERIFY_STARTED;

    if (VTSP_OK != (e = _VTSP_isInfcValid(infc))) { 
        _VTSP_TRACE(__FILE__, __LINE__);
        return (e);
    }

    if (NULL == stream_ptr) { 
        _VTSP_TRACE(__FILE__, __LINE__);
        return (VTSP_E_STREAM_DATA);
    }

    /* check if streamId is in valid range
     */
    if (VTSP_OK != (e = _VTSP_isStreamIdValid(infc, stream_ptr->streamId))) { 
        _VTSP_TRACE(__FILE__, __LINE__);
        return (e);
    }

    /* check if dir is valid
     */
    if (VTSP_OK != (e = _VTSP_isStreamDirValid(infc, stream_ptr->streamId, 
                    stream_ptr->dir))) { 
        _VTSP_TRACE(__FILE__, __LINE__);
        return (e);
    }

    if (VTSP_OK != (e = _VTSP_isStreamPeerValid(infc, stream_ptr))) { 
        _VTSP_TRACE(__FILE__, __LINE__);
        return (e);
    }

    if (VTSP_OK != (e = _VTSP_isStreamConfMaskValid(infc, 
                    stream_ptr->streamId, stream_ptr->confMask))) { 
        _VTSP_TRACE(__FILE__, __LINE__);
        return (e);
    }

    /* update to ipc */
    cmd.code = _VTSP_CMD_STREAM_START;
    cmd.infc = infc;
    OSAL_memCpy(&cmd.msg.stream, stream_ptr, sizeof(VTSP_Stream));

    _VTSP_putCmd(infc, &cmd, 0);
    /* set stream started in internal object */
    _VTSP_object_ptr->infc[infc].streamIds |= (1 << stream_ptr->streamId);
    return (VTSP_OK);
}

/*
 * ======== VTSP_streamEnd ========
 */
VTSP_Return VTSP_streamEnd(
    uvint           infc,
    uvint           streamId)
{
    VTSP_Return      e;
    _VTSP_CmdMsg     cmd;

    _VTSP_VERIFY_INIT;
    _VTSP_VERIFY_STARTED;
    if (VTSP_OK != (e = _VTSP_isInfcValid(infc))) { 
        _VTSP_TRACE(__FILE__, __LINE__);
        return (e);
    }

    /* check if streamId is in valid range
     */
    if (VTSP_OK != (e = _VTSP_isStreamIdValid(infc, streamId))) { 
        _VTSP_TRACE(__FILE__, __LINE__);
        return (e);
    }

    /* Set internal state */
    _VTSP_object_ptr->infc[infc].streamIds &= ~(1 << streamId);

    /* Send cmd to deactivate stream
     */
    cmd.code = _VTSP_CMD_STREAM_END;
    cmd.infc = infc;
    cmd.msg.arg.arg0 = streamId;

    _VTSP_putCmd(infc, &cmd, 0);
    return (VTSP_OK);
}

/*
 * ======== VTSP_streamModify ========
 */
VTSP_Return VTSP_streamModify(
    uvint            infc,
    VTSP_Stream     *stream_ptr)
{

    VTSP_Return      e;
    _VTSP_CmdMsg     cmd;

    _VTSP_VERIFY_INIT;
    _VTSP_VERIFY_STARTED;
    if (VTSP_OK != (e = _VTSP_isInfcValid(infc))) { 
        _VTSP_TRACE(__FILE__, __LINE__);
        return (e);
    }
    if (NULL == stream_ptr) {
        _VTSP_TRACE(__FILE__, __LINE__);
        return (VTSP_E_STREAM_DATA);
    }

    /* check if streamId is in valid range
     */
    if (VTSP_OK != (e = _VTSP_isStreamIdValid(infc, stream_ptr->streamId))) { 
        _VTSP_TRACE(__FILE__, __LINE__);
        return (e);
    }

    /* check if dir is valid
     */
    if (VTSP_OK != (e = _VTSP_isStreamDirValid(infc, stream_ptr->streamId, 
                    stream_ptr->dir))) { 
        _VTSP_TRACE(__FILE__, __LINE__);
        return (e);
    }

    /* update to ipc */
    cmd.code = _VTSP_CMD_STREAM_MODIFY;
    cmd.infc = infc;
    OSAL_memCpy(&cmd.msg.stream, stream_ptr, sizeof(VTSP_Stream));

    _VTSP_putCmd(infc, &cmd, 0);
    /* set stream started in internal object */
    _VTSP_object_ptr->infc[infc].streamIds |= (1 << stream_ptr->streamId);

    return (VTSP_OK);
}


/*
 * ======== VTSP_streamModifyDir ========
 */
VTSP_Return VTSP_streamModifyDir(
    uvint           infc,
    uvint           streamId,
    VTSP_StreamDir  dir)
{

    VTSP_Return      e;
    _VTSP_CmdMsg     cmd;


    _VTSP_VERIFY_INIT;
    _VTSP_VERIFY_STARTED;
    if (VTSP_OK != (e = _VTSP_isInfcValid(infc))) { 
        _VTSP_TRACE(__FILE__, __LINE__);
        return (e);
    }

    /* check if streamId is in valid range
     */
    if (VTSP_OK != (e = _VTSP_isStreamIdValid(infc, streamId))) { 
        _VTSP_TRACE(__FILE__, __LINE__);
        return (e);
    }

    /* check if dir is valid
     */
    if (VTSP_OK != (e = _VTSP_isStreamDirValid(infc, streamId, dir))) { 
        _VTSP_TRACE(__FILE__, __LINE__);
        return (e);
    }


    /* Send cmd to modify stream dir 
     */
    cmd.code = _VTSP_CMD_STREAM_MODIFY_DIR;
    cmd.infc = infc;
    cmd.msg.arg.arg0 = streamId;
    cmd.msg.arg.arg1 = dir;

    _VTSP_putCmd(infc, &cmd, 0);
    return (VTSP_OK);
}


/*
 * ======== VTSP_streamModifyEncoder ========
 *
 */
VTSP_Return VTSP_streamModifyEncoder(
    uvint            infc,
    uvint            streamId,
    uvint            encoder)
{

    VTSP_Return      e;
    _VTSP_CmdMsg     cmd;

    _VTSP_VERIFY_INIT;
    _VTSP_VERIFY_STARTED;

    if (VTSP_OK != (e = _VTSP_isInfcValid(infc))) { 
        _VTSP_TRACE(__FILE__, __LINE__);
        return (e);
    }

    /* check if streamId is in valid range
     */
    if (VTSP_OK != (e = _VTSP_isStreamIdValid(infc, streamId))) { 
        _VTSP_TRACE(__FILE__, __LINE__);
        return (e);
    }

    /* Send cmd to modify stream coder
     */
    cmd.code = _VTSP_CMD_STREAM_MODIFY_ENCODER;
    cmd.infc = infc;
    cmd.msg.arg.arg0 = streamId;
    cmd.msg.arg.arg1 = encoder;

    _VTSP_putCmd(infc, &cmd, 0);

    return (VTSP_OK);
}

/*
 * ======== VTSP_streamSendRfc4733 ========
 */
VTSP_Return VTSP_streamSendRfc4733(
    uvint             infc,
    uvint             streamId,
    VTSP_Rfc4733Type  type,
    void             *data_ptr)
{
    VTSP_Rfc4733Event *event_ptr;
    VTSP_Rfc4733Tone  *tone_ptr;
    VTSP_Return        e;
    _VTSP_CmdMsg       cmd;

   _VTSP_VERIFY_INIT;
    _VTSP_VERIFY_STARTED;

    if (VTSP_OK != (e = _VTSP_isInfcValid(infc))) { 
        _VTSP_TRACE(__FILE__, __LINE__);
        return (e);
    }

    /* check if streamId is in valid range
     */
    if (VTSP_OK != (e = _VTSP_isStreamIdValid(infc, streamId))) { 
        _VTSP_TRACE(__FILE__, __LINE__);
        return (e);
    }
    
    cmd.code                = _VTSP_CMD_STREAM_RFC4733_PEER;
    cmd.infc                = infc;
    cmd.msg.rfc4733.type    = type;
    cmd.msg.rfc4733.data[0] = streamId;

    switch (type) {
        case VTSP_RFC4733_TYPE_EVENT:
            event_ptr = (VTSP_Rfc4733Event*)data_ptr;

            /* Boundary checks */
            if (16 < event_ptr->eventNum) { 
                return (VTSP_E_DIGIT);
            }
            cmd.msg.rfc4733.duration = event_ptr->duration;
            cmd.msg.rfc4733.data[1] = event_ptr->volume;
            /*
             * End Packets can be set to 1, 2, or 3 with 3 being the default
             */
            if ((0 == event_ptr->endPackets) || (3 < event_ptr->endPackets)) {
                cmd.msg.rfc4733.data[2] = 3;
            }
            else {
                cmd.msg.rfc4733.data[2] = event_ptr->endPackets;
            }

            cmd.msg.rfc4733.data[3] = event_ptr->eventNum;
            break;
                
        case VTSP_RFC4733_TYPE_TONE:
            tone_ptr = (VTSP_Rfc4733Tone *)data_ptr;
            cmd.msg.rfc4733.duration = tone_ptr->duration;
            cmd.msg.rfc4733.data[1] = tone_ptr->volume;
            /*
             * End Packets can be set to 1, 2, or 3 with 3 being the default
             */
            if ((0 == tone_ptr->endPackets) || (3 < tone_ptr->endPackets)) {
                cmd.msg.rfc4733.data[2] = 3;
            }
            else {
                cmd.msg.rfc4733.data[2] = tone_ptr->endPackets;
            }
            cmd.msg.rfc4733.data[3] = tone_ptr->modFreq;
            cmd.msg.rfc4733.data[4] = tone_ptr->freq1;
            cmd.msg.rfc4733.data[5] = tone_ptr->freq2;
            cmd.msg.rfc4733.data[6] = tone_ptr->freq3;
            cmd.msg.rfc4733.data[7] = tone_ptr->freq4;
            cmd.msg.rfc4733.data[8] = tone_ptr->divMod3;

            break;

        default:
            _VTSP_TRACE(__FILE__, __LINE__);
            return (VTSP_E_ARG);
    }
    /* 
     * Send cmd to generate RFC4733 Tone or Event
     */
    _VTSP_putCmd(infc, &cmd, 0);
    return (VTSP_OK);
}

/*
 * ======== VTSP_streamModifyConf ========
 */

VTSP_Return VTSP_streamModifyConf(
    uvint           infc,
    uvint           streamId,
    uint32          confMask)
{

    VTSP_Return      e;
    _VTSP_CmdMsg     cmd;

    _VTSP_VERIFY_INIT;
    _VTSP_VERIFY_STARTED;

    if (VTSP_OK != (e = _VTSP_isInfcValid(infc))) { 
        _VTSP_TRACE(__FILE__, __LINE__);
        return (e);
    }

    /* check if streamId is in valid range
     */
    if (VTSP_OK != (e = _VTSP_isStreamIdValid(infc, streamId))) { 
        _VTSP_TRACE(__FILE__, __LINE__);
        return (e);
    }


    /* check if mask is valid
     */
    if (VTSP_OK != (e = _VTSP_isStreamConfMaskValid(infc, 
                    streamId, confMask))) { 
        _VTSP_TRACE(__FILE__, __LINE__);
        return (e);
    }

    /* Send cmd to modify stream confMask
     */
    cmd.code = _VTSP_CMD_STREAM_MODIFY_CONFMASK;
    cmd.infc = infc;
    cmd.msg.arg.arg0 = streamId;
    cmd.msg.arg.arg1 = confMask;

    _VTSP_putCmd(infc, &cmd, 0);

    return (VTSP_OK);
}


/*
 * ======== VTSP_streamQuery ========
 */
VTSP_Return VTSP_streamQuery(
    uvint           infc,
    uvint           streamId)
{

    VTSP_Return      e;
    _VTSP_CmdMsg     cmd;


    _VTSP_VERIFY_INIT;
    _VTSP_VERIFY_STARTED;
    if (VTSP_OK != (e = _VTSP_isInfcValid(infc))) { 
        _VTSP_TRACE(__FILE__, __LINE__);
        return (e);
    }

    /* check if streamId is in valid range
     */
    if (VTSP_OK != (e = _VTSP_isStreamIdValid(infc, streamId))) { 
        _VTSP_TRACE(__FILE__, __LINE__);
        return (e);
    }

    /* Send cmd as stream query
     */
    cmd.code = _VTSP_CMD_STREAM_QUERY;
    cmd.infc = infc;
    cmd.msg.arg.arg0 = streamId;

    _VTSP_putCmd(infc, &cmd, 0);
    return (VTSP_OK);
}

/*
 * ======== VTSP_streamTone ========
 */
VTSP_Return VTSP_streamTone(
    uvint           infc,
    uvint           streamId,
    uvint           templateTone,
    uvint           repeat,
    uint32          maxTime)
{
    VTSP_Return  e;
    _VTSP_CmdMsg cmd;

    _VTSP_VERIFY_INIT;
    _VTSP_VERIFY_STARTED;
    if (VTSP_OK != (e = _VTSP_isInfcValid(infc))) { 
        return (e);
    }
    if (VTSP_OK != (e = _VTSP_isToneTemplIdValid(infc, templateTone))) { 
        return (e);
    }
    /* check if streamId is in valid range
     */
    if (VTSP_OK != (e = _VTSP_isStreamIdValid(infc, streamId))) { 
        _VTSP_TRACE(__FILE__, __LINE__);
        return (e);
    }

    /* Bounds check */
    if (repeat > VTSP_TONE_NMAX) { 
        repeat = VTSP_TONE_NMAX;
    }
    if (maxTime > VTSP_TONE_TMAX) { 
        maxTime = VTSP_TONE_TMAX;
    }

    cmd.code = _VTSP_CMD_STREAM_TONE;
    cmd.infc = infc;
    cmd.msg.arg.arg0 = templateTone;
    cmd.msg.arg.arg1 = repeat;
    cmd.msg.arg.arg2 = maxTime;
    cmd.msg.arg.arg3 = streamId;

    _VTSP_putCmd(infc, &cmd, 0);
    return (VTSP_OK);
}

/*
 * ======== VTSP_streamToneQuad ========
 */
VTSP_Return VTSP_streamToneQuad(
    uvint           infc,
    uvint           streamId,
    uvint           templateQuad,
    uvint           repeat,
    uint32          maxTime)
{
#ifdef VTSP_ENABLE_TONE_QUAD
    VTSP_Return  e;

    _VTSP_CmdMsg cmd;

    _VTSP_VERIFY_INIT;
    _VTSP_VERIFY_STARTED;
    if (VTSP_OK != (e = _VTSP_isInfcValid(infc))) { 
        return (e);
    }
    if (VTSP_OK != (e = _VTSP_isToneTemplIdValid(infc, templateQuad))) { 
        return (e);
    }
    /* check if streamId is in valid range
     */
    if (VTSP_OK != (e = _VTSP_isStreamIdValid(infc, streamId))) { 
        _VTSP_TRACE(__FILE__, __LINE__);
        return (e);
    }

    /* Bounds check */
    if (repeat > VTSP_TONE_NMAX) { 
        repeat = VTSP_TONE_NMAX;
    }
    if (maxTime > VTSP_TONE_TMAX) { 
        maxTime = VTSP_TONE_TMAX;
    }

    cmd.code = _VTSP_CMD_STREAM_TONE_QUAD;
    cmd.infc = infc;
    cmd.msg.arg.arg0 = templateQuad;
    cmd.msg.arg.arg1 = repeat;
    cmd.msg.arg.arg2 = maxTime;
    cmd.msg.arg.arg3 = streamId;

    _VTSP_putCmd(infc, &cmd, 0);
    return (VTSP_OK);
#else
    return (VTSP_E_CONFIG);
#endif
}

/*
 * ======== VTSP_streamToneSequence ========
 */
VTSP_Return VTSP_streamToneSequence(
    uvint   infc,
    uvint   streamId,
    uvint  *toneId_ptr,
    uvint   numToneIds,
    uint32  control,
    uint32  repeat)
{
    VTSP_Return     e;
    _VTSP_CmdMsg    cmd;
    vint            index;

    _VTSP_VERIFY_INIT;
    _VTSP_VERIFY_STARTED;
    if (VTSP_OK != (e = _VTSP_isInfcValid(infc))) { 
        return (e);
    }
    /* check if streamId is in valid range
     */
    if (VTSP_OK != (e = _VTSP_isStreamIdValid(infc, streamId))) { 
        _VTSP_TRACE(__FILE__, __LINE__);
        return (e);
    }
    if (NULL == toneId_ptr) { 
        return (VTSP_E_ARG);
    }
    if (numToneIds < 1) { 
        return (VTSP_E_ARG);
    }
    if (numToneIds > _VTSP_NUM_TONE_SEQUENCE_MAX) {
        return (VTSP_E_ARG);
    }
    for (index = (numToneIds - 1); index >= 0; index--) {
        if (VTSP_OK != 
                (e = _VTSP_isToneTemplIdValid(infc, toneId_ptr[index]))) { 
            return (e);
        }
    }

    if (repeat > VTSP_TONE_NMAX) { 
        repeat = VTSP_TONE_NMAX;
    }

    cmd.code = _VTSP_CMD_STREAM_TONE_SEQUENCE;
    cmd.infc = infc;
    cmd.msg.toneSequence.streamId = streamId;
    cmd.msg.toneSequence.numToneIds = numToneIds;
    cmd.msg.toneSequence.control = control;
    cmd.msg.toneSequence.repeat = repeat;
    cmd.msg.toneSequence.maxTime = VTSP_TONE_TMAX;
    for (index = (numToneIds - 1); index >= 0; index--) {
        cmd.msg.toneSequence.toneIds[index] = 
            toneId_ptr[index];
    }

    _VTSP_putCmd(infc, &cmd, 0);
    return (VTSP_OK);
}

/*
 * ======== VTSP_streamToneQuadSequence ========
 */
VTSP_Return VTSP_streamToneQuadSequence(
    uvint   infc,
    uvint   streamId,
    uvint  *toneId_ptr,
    uvint   numToneIds,
    uint32  control,
    uint32  repeat)
{
    VTSP_Return     e;
    _VTSP_CmdMsg    cmd;
    vint            index;

    _VTSP_VERIFY_INIT;
    _VTSP_VERIFY_STARTED;
    if (VTSP_OK != (e = _VTSP_isInfcValid(infc))) { 
        return (e);
    }
    /* check if streamId is in valid range
     */
    if (VTSP_OK != (e = _VTSP_isStreamIdValid(infc, streamId))) { 
        _VTSP_TRACE(__FILE__, __LINE__);
        return (e);
    }
    if (NULL == toneId_ptr) { 
        return (VTSP_E_ARG);
    }
    if (numToneIds < 1) { 
        return (VTSP_E_ARG);
    }
    if (numToneIds > _VTSP_NUM_TONE_SEQUENCE_MAX) {
        return (VTSP_E_ARG);
    }
    for (index = (numToneIds - 1); index >= 0; index--) {
        if (VTSP_OK != 
                (e = _VTSP_isToneTemplIdValid(infc, toneId_ptr[index]))) { 
            return (e);
        }
    }

    if (repeat > VTSP_TONE_NMAX) { 
        repeat = VTSP_TONE_NMAX;
    }

    cmd.code = _VTSP_CMD_STREAM_TONE_QUAD_SEQUENCE;
    cmd.infc = infc;
    cmd.msg.toneSequence.streamId = streamId;
    cmd.msg.toneSequence.numToneIds = numToneIds;
    cmd.msg.toneSequence.control = control;
    cmd.msg.toneSequence.repeat = repeat;
    cmd.msg.toneSequence.maxTime = VTSP_TONE_TMAX;

    for (index = (numToneIds - 1); index >= 0; index--) {
        cmd.msg.toneSequence.toneIds[index] = 
            toneId_ptr[index];
    }

    _VTSP_putCmd(infc, &cmd, 0);
    return (VTSP_OK);
}
/*
 * ======== VTSP_streamToneStop ========
 */
VTSP_Return VTSP_streamToneStop(
    uvint             infc,
    uvint             streamId)
{
    return (VTSP_streamTone(infc, streamId, 0, 0, 0));
}

/*
 * ======== VTSP_streamToneQuadStop ========
 */
VTSP_Return VTSP_streamToneQuadStop(
    uvint             infc,
    uvint             streamId)
{
    return (VTSP_streamToneQuad(infc, streamId, 0, 0, 0));
}

