/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004-2010 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 30131 $ $Date: 2014-12-01 14:45:03 +0800 (Mon, 01 Dec 2014) $
 *
 */

/*
 * This file handles commands sent to the driver which are pending
 * in the down-queue
 */

extern const char D2_ComponentReleaseVersion_g729ab[];
extern const char D2_ComponentReleaseVersion_ecsr[];
extern const char D2_ComponentReleaseVersion_comm[];
extern const char D2_ComponentReleaseVersion_dcrm[];
extern const char D2_ComponentReleaseVersion_nfe[];
extern const char D2_ComponentReleaseVersion_aec[];
extern const char D2_ComponentReleaseVersion_bnd[];
extern const char D2_ComponentReleaseVersion_plc[];
extern const char D2_ComponentReleaseVersion_jb[];
extern const char D2_ComponentReleaseVersion_dtmf[];
extern const char D2_ComponentReleaseVersion_fmtd[];
extern const char D2_ComponentReleaseVersion_g726[];
extern const char D2_ComponentReleaseVersion_g723a[];
extern const char D2_ComponentReleaseVersion_fr38[];
extern const char D2_ComponentReleaseVersion_ilbc[];
extern const char D2_ComponentReleaseVersion_g722[];
extern const char D2_ComponentReleaseVersion_g722p1[];
extern const char D2_ComponentReleaseVersion_silk[];
extern const char D2_ComponentReleaseVersion_gamrnb[];
extern const char D2_ComponentReleaseVersion_gamrwb[];
extern const char D2_ComponentReleaseVersion_lms[];
extern const char D2_ComponentReleaseVersion_genf[];
extern const char D2_ComponentReleaseVersion_nse[];
extern const char D2_ComponentReleaseVersion_tone[];
extern const char D2_ComponentReleaseVersion_fsks[];
extern const char D2_ComponentReleaseVersion_fskr[];
extern const char D2_ComponentReleaseVersion_fmdp[];
extern const char D2_ComponentReleaseVersion_v2xr[];
extern const char D2_ComponentReleaseVersion_v2xt[];
extern const char D2_ComponentReleaseVersion_v17r[];
extern const char D2_ComponentReleaseVersion_v17t[];
extern const char D2_ComponentReleaseVersion_v21r[];
extern const char D2_ComponentReleaseVersion_v21t[];
extern const char D2_ComponentReleaseVersion_uds[];

#include "vtspr.h"
#include "_vtspr_private.h"
#include "voice_net.h"

void _VTSPR_printCommand(
    _VTSP_CmdMsgCode code) 
{
    switch (code) {
        case _VTSP_CMD_STREAM_START:
            OSAL_logMsg("command _VTSP_CMD_STREAM_START\n"); 
            break;
        case _VTSP_CMD_STREAM_MODIFY:
            OSAL_logMsg("command _VTSP_CMD_STREAM_MODIFY\n"); 
            break;
        case _VTSP_CMD_STREAM_MODIFY_DIR:
            OSAL_logMsg("command _VTSP_CMD_STREAM_MODIFY_DIR\n"); 
            break;
        case _VTSP_CMD_STREAM_MODIFY_ENCODER:
            OSAL_logMsg("command _VTSP_CMD_STREAM_MODIFY_ENCODER\n"); 
            break;
        case _VTSP_CMD_STREAM_MODIFY_CONFMASK:
            OSAL_logMsg("command _VTSP_CMD_STREAM_MODIFY_CONFMASK\n"); 
            break;
        case _VTSP_CMD_STREAM_QUERY:
//            OSAL_logMsg("command _VTSP_CMD_STREAM_QUERY\n"); 
            break;
        case _VTSP_CMD_STREAM_END:
            OSAL_logMsg("command _VTSP_CMD_STREAM_END\n"); 
            break;
        case _VTSP_CMD_INFC_CONTROL_IO:
            OSAL_logMsg("command _VTSP_CMD_INFC_CONTROL_IO\n"); 
            break;
        default:
            OSAL_logMsg("command VTSP code=%d\n", code);
    }
}

/*
 * ======== _VTSPR_activeInfcStreamFilterStart ========
 *
 * Limits the number of active streams/interfaces upon stream startup.
 *
 */
VTSP_Return _VTSPR_activeInfcStreamFilterStart(
    VTSPR_DSP *dsp_ptr, 
    vint streamId,
    vint infc,
    VTSPR_Queues *q_ptr)
{
    VTSPR_StreamObj *stream_ptr;
    VTSPR_ChanObj   *chan_ptr;
    
    
    stream_ptr = _VTSPR_streamIdToStreamPtr(dsp_ptr, infc, streamId);
    chan_ptr   = _VTSPR_infcToChanPtr(dsp_ptr, infc);

    /* Check if current interface is already active */
    if (!chan_ptr->curActive) {
    
        /* Is another active interface permittied? */
        if (dsp_ptr->curActiveInfcs < VTSP_MAX_NUM_ACTIVE_INFCS) {
        
            /* Is another active stream permitted? */
            if(dsp_ptr->curActiveStreams < VTSP_MAX_NUM_ACTIVE_STREAMS) {
                dsp_ptr->curActiveStreams++;
                dsp_ptr->curActiveInfcs++;
                stream_ptr->curActive = 1;
                chan_ptr->curActive = 1;
                       
                /* Send stream unavailable event */
                if(dsp_ptr->curActiveStreams >= VTSP_MAX_NUM_ACTIVE_STREAMS) {
                    q_ptr->eventMsg.infc = VTSP_INFC_GLOBAL;
                    q_ptr->eventMsg.code = VTSP_EVENT_MSG_CODE_RESOURCE;
                    q_ptr->eventMsg.msg.resource.reason = VTSP_EVENT_REC_STREAM_UNAVAILABLE;
                    q_ptr->eventMsg.msg.resource.streamId = streamId;
                    VTSPR_sendEvent(q_ptr, &q_ptr->eventMsg, VTSP_INFC_GLOBAL);
                }
            }
            
            /* Block Stream */
            else {
                /* Send stream unavailable event */                
                q_ptr->eventMsg.infc = VTSP_INFC_GLOBAL;
                q_ptr->eventMsg.code = VTSP_EVENT_MSG_CODE_RESOURCE;
                q_ptr->eventMsg.msg.resource.reason = VTSP_EVENT_REC_STREAM_UNAVAILABLE;
                q_ptr->eventMsg.msg.resource.streamId = streamId;
                VTSPR_sendEvent(q_ptr, &q_ptr->eventMsg, VTSP_INFC_GLOBAL);
                _VTSP_TRACE("stream blocked on infc", infc);
                return (VTSP_E_RESOURCE);
            }
            
            /* Send interface unavailable event */
            if (dsp_ptr->curActiveInfcs >= VTSP_MAX_NUM_ACTIVE_INFCS) {                
                q_ptr->eventMsg.infc = VTSP_INFC_GLOBAL;
                q_ptr->eventMsg.code = VTSP_EVENT_MSG_CODE_RESOURCE;
                q_ptr->eventMsg.msg.resource.reason = VTSP_EVENT_REC_INFC_UNAVAILABLE;
                q_ptr->eventMsg.msg.resource.streamId = streamId; 
                VTSPR_sendEvent(q_ptr, &q_ptr->eventMsg, VTSP_INFC_GLOBAL);
            }
        }
        
        /* Block Interface */
        else {            
            q_ptr->eventMsg.infc = VTSP_INFC_GLOBAL;
            q_ptr->eventMsg.code = VTSP_EVENT_MSG_CODE_RESOURCE;
            q_ptr->eventMsg.msg.resource.reason = VTSP_EVENT_REC_INFC_UNAVAILABLE;
            q_ptr->eventMsg.msg.resource.streamId = streamId;
            VTSPR_sendEvent(q_ptr, &q_ptr->eventMsg, VTSP_INFC_GLOBAL);
            _VTSP_TRACE("infc blocked", infc);
            return (VTSP_E_RESOURCE);
        }
    }
    
    /* Interface is already active. Is another active stream permitted> */
    else {
        if(dsp_ptr->curActiveStreams < VTSP_MAX_NUM_ACTIVE_STREAMS) {                    
            dsp_ptr->curActiveStreams++;
            stream_ptr->curActive = 1;
 
            /* Send stream unavailable event */
            if (dsp_ptr->curActiveStreams >= VTSP_MAX_NUM_ACTIVE_STREAMS) {
                q_ptr->eventMsg.infc = VTSP_INFC_GLOBAL;
                q_ptr->eventMsg.code = VTSP_EVENT_MSG_CODE_RESOURCE;
                q_ptr->eventMsg.msg.resource.reason = VTSP_EVENT_REC_STREAM_UNAVAILABLE;
                q_ptr->eventMsg.msg.resource.streamId = streamId;
                VTSPR_sendEvent(q_ptr, &q_ptr->eventMsg, VTSP_INFC_GLOBAL);
            }
        }
        
        /* Block Stream */
        else {
            /* Send stream unavailable event */
            q_ptr->eventMsg.infc = VTSP_INFC_GLOBAL;
            q_ptr->eventMsg.code = VTSP_EVENT_MSG_CODE_RESOURCE;
            q_ptr->eventMsg.msg.resource.reason = VTSP_EVENT_REC_STREAM_UNAVAILABLE;
            q_ptr->eventMsg.msg.resource.streamId = streamId;
            VTSPR_sendEvent(q_ptr, &q_ptr->eventMsg, VTSP_INFC_GLOBAL);
            _VTSP_TRACE("stream blocked on infc", infc);
            return (VTSP_E_RESOURCE);
        }
    }
    
    return (VTSP_OK);
}

/*
 * ======== _VTSPR_activeInfcStreamFilterStart ========
 *
 * Updates the number of active streams/interfaces upon stream end.
 *
 */
void _VTSPR_activeInfcStreamFilterEnd(
    VTSPR_DSP *dsp_ptr, 
    vint streamId,
    vint infc,
    VTSPR_Queues *q_ptr)
{
    VTSPR_StreamObj *stream_ptr;
    VTSPR_ChanObj   *chan_ptr;
    vint             activeStreamOnInfc;
    vint             ctr;
    
    stream_ptr = _VTSPR_streamIdToStreamPtr(dsp_ptr, infc, streamId);
    chan_ptr   = _VTSPR_infcToChanPtr(dsp_ptr, infc);    
    
    /* Deactivate stream, if active */
    if (stream_ptr->curActive) {
        stream_ptr->curActive = 0;
        dsp_ptr->curActiveStreams--;
        
        /* Send stream available event */
        q_ptr->eventMsg.infc = VTSP_INFC_GLOBAL;
        q_ptr->eventMsg.code = VTSP_EVENT_MSG_CODE_RESOURCE;
        q_ptr->eventMsg.msg.resource.reason = VTSP_EVENT_REC_STREAM_AVAILABLE;
        q_ptr->eventMsg.msg.resource.streamId = streamId;
        VTSPR_sendEvent(q_ptr, &q_ptr->eventMsg, VTSP_INFC_GLOBAL);        
    }
    
    /* Check for any more active streams on current interface */
    activeStreamOnInfc = 0;
    for (ctr = 0; ctr < _VTSP_STREAM_PER_INFC; ctr++) {
        stream_ptr = _VTSPR_streamIdToStreamPtr(dsp_ptr, infc, ctr);
        
        if (stream_ptr->curActive) {
            activeStreamOnInfc = 1;
        }
    }
    
    /* If no active streams on intervace, set active to 0 */
    if (!activeStreamOnInfc) {
        chan_ptr = _VTSPR_infcToChanPtr(dsp_ptr, infc);
        
        if (chan_ptr->curActive) {
            chan_ptr->curActive = 0;
            dsp_ptr->curActiveInfcs--;
            
            /* Send interface available event */
            q_ptr->eventMsg.infc = VTSP_INFC_GLOBAL;
            q_ptr->eventMsg.code = VTSP_EVENT_MSG_CODE_RESOURCE;
            q_ptr->eventMsg.msg.resource.reason = VTSP_EVENT_REC_INFC_AVAILABLE;
            q_ptr->eventMsg.msg.resource.streamId = streamId;
            VTSPR_sendEvent(q_ptr, &q_ptr->eventMsg, VTSP_INFC_GLOBAL);
        }
    }

    return;
}

/*
 * ======== _VTSPR_updateStream ========
 *
 * Update internal stream params based on VTSP command.
 *
 *
 * XXX Note this function may handle changes in stream direction,
 * currently, it does not.
 *
 */
void _VTSPR_updateStream(
        VTSPR_DSP       *dsp_ptr,
        vint             infc,
        VTSPR_StreamObj *stream_ptr,
        VTSP_Stream     *newParam_ptr)
{
    /*
     * Copy stream params.
     */
    OSAL_memCpy(&stream_ptr->streamParam, newParam_ptr,
            sizeof(VTSP_Stream));

#ifdef VTSP_ENABLE_DTMFR
    if ((0 != (stream_ptr->drEncodeObj.status & DR_IN_DIGIT))) {
        /*
         * DR is active - change prev coder, set current encoder back to DTMF
         */
        stream_ptr->drEncodeObj.prevCoder = stream_ptr->streamParam.encoder;
        stream_ptr->streamParam.encoder = VTSP_CODER_DTMF;
    }
    else {
        /* Configure DR sample rate */
        if (0 != (stream_ptr->streamParam.extension &
                VTSP_MASK_EXT_DTMFR_16K)) {
            stream_ptr->drEncodeObj.sampleRate = DR_SAMPLE_RATE_16K;
            stream_ptr->drDecodeObj_ptr->sampleRate = DR_SAMPLE_RATE_16K;
        }
        else {
            stream_ptr->drEncodeObj.sampleRate = DR_SAMPLE_RATE_8K;
            stream_ptr->drDecodeObj_ptr->sampleRate = DR_SAMPLE_RATE_8K;
        }
    }
#endif

    /*
     * Turn off local tone when direction has recv component (recvonly or
     * sendrecv).  For streams in sendonly or inactive direction, TONE is OK to
     * continue.
     */
    if ((VTSP_STREAM_DIR_SENDONLY == stream_ptr->streamParam.dir) ||
            (VTSP_STREAM_DIR_INACTIVE == stream_ptr->streamParam.dir)) {
        /*
         * Tone is OK to continue, if it is active.
         */
    }
    else {
        /*
         * Turn off Tone.
         * This protects against the following cases:
         *  - tone is being played.
         *  - stream is started or modified, especially in high Mhz case (i.e.
         *  conference)
         *  - Mhz may be exceeded.
         *  -- This may occur when application programmer does not turn off
         *  tone, then starts/modifies stream, thus Mhz indication is higher
         *  than it should be, because tone is mistakenly active.
         * or,
         *  - tone is being played
         *  - voice recv is active
         *  - tone overwrites voice accidentally, or voice overwrites tone
         *  accidentally
         */
        _VTSPR_algStateChan(dsp_ptr, infc, VTSPR_ALG_CHAN_TONE, 0);
    }

    /*
     * Check if new encode Type is T38
     */
    if ((VTSP_CODER_T38 == stream_ptr->streamParam.encoder)
            && (VTSP_CODER_T38 != stream_ptr->lastEncoder)) {
#ifdef VTSP_ENABLE_T38
        /*
         * Switch from voice encoder -> T38.
         * Turn off voice algorithms
         * Turn on T38
         */
        _VTSPR_algStateChan(dsp_ptr, infc,
#ifdef VTSP_ENABLE_CIDS
                VTSPR_ALG_CHAN_CIDS    |
                VTSPR_ALG_CHAN_CIDCWS  |
#endif
                VTSPR_ALG_CHAN_DCRM_PEER |
                VTSPR_ALG_CHAN_NFE_INFC | VTSPR_ALG_CHAN_ECSR     |
                VTSPR_ALG_CHAN_NLP      | VTSPR_ALG_CHAN_DTMF     |
                VTSPR_ALG_CHAN_FMTD     | VTSPR_ALG_CHAN_TONE     |
                VTSPR_ALG_CHAN_DR_DEC   | VTSPR_ALG_CHAN_NFE_PEER |
                VTSPR_ALG_CHAN_FAX      | VTSPR_ALG_CHAN_AEC,
                VTSPR_ALG_CHAN_T38);

        /*
         * Turn off voice stream algorithms
         * Turn on T38
         */
        _VTSPR_algStateStream(dsp_ptr, infc,
                stream_ptr->streamParam.streamId,
                VTSPR_ALG_STREAM_JB |
                VTSPR_ALG_STREAM_PLC,
                VTSPR_ALG_STREAM_T38);

        /*
         * Force direction to bidirectional.
         */
        stream_ptr->streamParam.dir = VTSP_STREAM_DIR_SENDRECV;
#else
        /* XXX Should not get here */
        /* _VTSP_TRACE(__FILE__, __LINE__); */
#endif
    }
    else if ((VTSP_CODER_T38 != stream_ptr->streamParam.encoder)
            && (VTSP_CODER_T38 == stream_ptr->lastEncoder)) {
#ifdef VTSP_ENABLE_T38
        /*
         * Switch from T38 -> voice encoder.
         * Set all the algorithms for voice.
         */
        _VTSPR_algStateChan(dsp_ptr, infc,
                VTSPR_ALG_CHAN_T38      | VTSPR_ALG_CHAN_FAX,
                VTSPR_ALG_CHAN_DCRM_PEER | VTSPR_ALG_CHAN_AEC |
                VTSPR_ALG_CHAN_NFE_INFC | VTSPR_ALG_CHAN_ECSR |
                VTSPR_ALG_CHAN_NLP      | VTSPR_ALG_CHAN_DTMF |
                VTSPR_ALG_CHAN_FMTD     | VTSPR_ALG_CHAN_NFE_PEER);
        _VTSPR_algStateStream(dsp_ptr, infc,
                stream_ptr->streamParam.streamId,
                VTSPR_ALG_STREAM_T38,
#ifndef VTSP_ENABLE_MP_LITE
                VTSPR_ALG_STREAM_JB     | VTSPR_ALG_STREAM_PLC);
#else
                VTSPR_ALG_STREAM_JB);
#endif /* VTSP_ENABLE_MP_LITE */
#else
        /* XXX Should not get here */
        /* _VTSP_TRACE(__FILE__, __LINE__); */
#endif
    }
    else {
        /*
         * Voice encoder.
         * Turn on stream algorithms.
         * This will init where necessary.
         */
        _VTSPR_algStateStream(dsp_ptr, infc,
                stream_ptr->streamParam.streamId,
                0,
#ifndef VTSP_ENABLE_MP_LITE
                VTSPR_ALG_STREAM_PLC | VTSPR_ALG_STREAM_JB);
#else
                VTSPR_ALG_STREAM_JB);
#endif
    }

    /*
     * Update other vars
     */
    stream_ptr->cnPktTime = 10;

    return;
}


/*
 * ======== VTSPR_cidDataUnpack ========
 */
VTSP_Return VTSPR_cidDataUnpack(
    VTSP_CIDDataFields   field,
    uint8               *string_ptr,
    VTSP_CIDData        *cid_ptr)
{
    _VTSP_CIDData   *obj_ptr;
    uvint            len;
    uvint            tlen;
    uint8           *src_ptr;
    uint8           *srcEnd_ptr;
    uvint            mtype;
    vint             type;

    if (NULL == string_ptr) { 
        return (VTSP_E_ARG);
    }
    if (NULL == cid_ptr) { 
        return (VTSP_E_ARG);
    }

    obj_ptr    = (_VTSP_CIDData *)cid_ptr;
    len        = 0;
    src_ptr    = &obj_ptr->data[0];
    mtype      = *src_ptr++;
    tlen       = *src_ptr++;
    srcEnd_ptr = src_ptr + tlen;

    if (tlen < 3) {
        return (VTSP_E_NO_MSG);
    }

    switch (field) {
        case VTSP_CIDDATA_FIELD_NAME:
            if (0x80 != mtype) {
                return (VTSP_E_NO_MSG);
            }
            field = VTSP_CIDDATA_FIELD_NAME;
            break;
            
        case VTSP_CIDDATA_FIELD_NUMBER:
            if (0x80 != mtype) {
                return (VTSP_E_NO_MSG);
            }
            field = VTSP_CIDDATA_FIELD_NUMBER;
            break;
            
        case VTSP_CIDDATA_FIELD_DATE_TIME:
            if (0x80 != mtype) {
                return (VTSP_E_NO_MSG);
            }
            field = VTSP_CIDDATA_FIELD_DATE_TIME;
            break;

        case VTSP_CIDDATA_FIELD_VI:
            if (0x82 != mtype) {
                return (VTSP_E_NO_MSG);
            }
            field = VTSP_CIDDATA_FIELD_VI;
            break;
            
        case VTSP_CIDDATA_FIELD_RAW:
            OSAL_memCpy(string_ptr, src_ptr, tlen);
            return (VTSP_OK);

        default:
            /* No such field is handled */
            return (VTSP_E_RESOURCE);
    }

    type = *src_ptr++;
    len = *src_ptr++;
    while (field != type) {
        if (src_ptr >= (srcEnd_ptr - 2)) {
            return (VTSP_E_NO_MSG);
        }
        src_ptr += len;
        type = *src_ptr++;
        len = *src_ptr++;
    }
    OSAL_memCpy(string_ptr, src_ptr, len);
    string_ptr[len] = 0;
    
    return (VTSP_OK);
}

/*
 * ======== _VTSPR_setChanMute() ========
 *
 * set / unset input / ouput mute
 */
OSAL_INLINE void _VTSPR_setChanMute(
    VTSPR_DSP *dsp_ptr,
    uvint      chan,
    vint       muteMask,
    uvint      muteOn)
{
    if (1 == muteOn) {
        _VTSPR_algStateChan(dsp_ptr, chan, 0, muteMask);
    }
    else {
        _VTSPR_algStateChan(dsp_ptr, chan, muteMask, 0);
    }
}

/*
 * ======== _VTSPR_runDnCmd() ========
 *
 * Get cmd from driver downstream queue
 * and run it
 */
void _VTSPR_runDnCmd(
    VTSPR_Obj     *vtspr_ptr,
    VTSPR_Queues  *q_ptr,
    VTSPR_DSP     *dsp_ptr,
    _VTSP_CmdMsg  *cmd_ptr)
{
    vint                 infc;              /* must be signed */
    vint                 tId;
    int32                enable;
    VTSPR_StreamObj     *stream_ptr;
    _VTSPR_FlowObj      *flow_ptr;
    _VTSPR_RtcpObject   *rtcp_ptr;
    _VTSPR_RtpObject    *rtp_ptr;
    int32                time;
    uint32               repeat;
    _VTSP_RingTemplate  *ring_ptr;
#ifndef VTSP_ENABLE_MP_LITE
    TONE_Params         *toneParam_ptr;
    VTSPR_ToneSeq       *toneSeq_ptr;    
#endif
#ifdef VTSP_ENABLE_TONE_QUAD
    GENF_Params         *toneQuadParam_ptr;
    uvint                index;
#endif
#ifdef VTSP_ENABLE_UTD
    UTD_Tonedef         *utdToneParam_ptr;
    UTD_SITTonedef      *utdSitParam_ptr;
    _VTSPR_UtdObj       *utd_ptr;
    vint                 type;
#endif
#ifdef VTSP_ENABLE_CIDS
    _VTSPR_CidsObj      *cids_ptr;
#endif
    VTSPR_ChanObj       *chan_ptr;
    VTSPR_FxsInfcObj    *fxs_ptr;
    VTSPR_FxoInfcObj    *fxo_ptr;
    TIC_Obj             *tic_ptr;
    TIC_ControlCode      ticCode;
#ifdef VTSP_ENABLE_FMTD    
    FMTD_Obj            *fmtd_ptr; 
    vint                 ctr;
#endif
    uvint                chan;
    JB_Params           *jbParams_ptr;
#ifdef VTSP_ENABLE_T38
    FR38_Params         *fr38Params_ptr;
#endif
    vint                 coder;
    int32                gain;
    vint                 dir;
    vint                 mask;
    vint                 errval;
    vint                 streamId;
#ifdef VTSP_ENABLE_AEC
extern AEC_Params       _VTSPR_aecHandsetParams;
extern AEC_Params       _VTSPR_aecHandsFreeParams;
#endif
#ifdef VTSP_ENABLE_DTMF
    DTMF_Params         *dtmfParams_ptr;
#endif
#ifdef VTSP_ENABLE_RTP_REDUNDANT
    uint8                parameter;
#endif
    OSAL_NetAddress      localRtcpAddr;
    OSAL_NetAddress      remoteRtcpAddr;

#ifdef VTSPR_ENABLE_AUTOSTART
    _VTSP_CmdMsg _VTSPR_toneTestCmd;

    if (1 == vtspr_ptr->autoStartCmd)
        autoStartCmd = 0;
        cmd_ptr = &_VTSPR_toneTestCmd;
        cmd_ptr->code = _VTSP_CMD_TONE_LOCAL;
        cmd_ptr->infc = 0;
        /* 9 = _VTSPR_toneTest2 */
        /* 5 = _VTSPR_toneVoipRingback */
        cmd_ptr->msg.arg.arg0 = /* tId */ 9;
        cmd_ptr->msg.arg.arg1 = /* repeat */ VTSP_TONE_NMAX;
        cmd_ptr->msg.arg.arg2 = /* time */ VTSP_TONE_TMAX;
        _VTSP_TRACE(__FILE__, __LINE__);
#endif

    infc = cmd_ptr->infc;

    if ((infc >= _VTSP_INFC_NUM) && (infc != VTSP_INFC_GLOBAL)) {
        _VTSP_TRACE(__FILE__, __LINE__);
        _VTSP_TRACE("infc does not exist", infc);
        return;
    }
    /* Print the command name */
    _VTSPR_printCommand(cmd_ptr->code);    

    switch (cmd_ptr->code) {
        case _VTSP_CMD_CID_GET_DATA:                /* XXX */
        case _VTSP_CMD_NO_OP:
        case _VTSP_CMD_TONE_LOCAL_DIGIT:
        default:
            _VTSP_TRACE(__FILE__, __LINE__);
            _VTSP_TRACE("unhandled cmd code", cmd_ptr->code);
            break;
        case _VTSP_CMD_INFC_CONTROL_GAIN:
            chan_ptr   = _VTSPR_infcToChanPtr(dsp_ptr, infc);
            /*
             * The gain must range between -20 dB and +20 dB, in 0.5 dB steps.
             * Thus, arg0 and arg1 range from -40 to +40.
             *
             * The COMM_db2lin() function creates a linear value between
             * 205 and 20480; coresponds to -20 dB to +20 dB,
             * with 2048 as 0 dB.
             *
             * Scale this value for Q15 math.
             */
            gain = (int32)cmd_ptr->msg.arg.arg0;
            if (0 == gain) {
                /* Zero is special case, _VTSPR_audioGain() is not called */
                chan_ptr->gainAudioOut = 0;
            }
            else {
                gain = gain + 40;       /* Transmit Gain */
                if (80 < gain) {
                    gain = 80;
                }
                else if (0 > gain) {
                    gain = 0;
                }
                gain = COMM_db2lin(gain);
                gain = (32767 * gain) >> 11;
                chan_ptr->gainAudioOut = gain;
            }

            gain = (int32)cmd_ptr->msg.arg.arg1;
            if (0 == gain) {
                /* Zero is special case, _VTSPR_audioGain() is not called */
                chan_ptr->gainAudioIn = 0;
            }
            else {
                gain = gain + 40;       /* Receive Gain */
                if (80 < gain) {
                    gain = 80;
                }
                else if (0 > gain) {
                    gain = 0;
                }
                gain = COMM_db2lin(gain);
                gain = (32767 * gain) >> 11;
                chan_ptr->gainAudioIn = gain;
            }
            break;
        case _VTSP_CMD_TRACE:
            _VTSPR_printInfo();
            break;

#ifndef VTSP_ENABLE_MP_LITE
        case _VTSP_CMD_TONE_LOCAL:
            /*
             * Near end tone generation
             */
            chan_ptr     = _VTSPR_infcToChanPtr(dsp_ptr, infc);
            tId           = cmd_ptr->msg.arg.arg0;
            repeat        = cmd_ptr->msg.arg.arg1;
            time          = cmd_ptr->msg.arg.arg2;
            toneParam_ptr = dsp_ptr->toneParams_ptr[tId];
            toneSeq_ptr = &chan_ptr->toneSeq;

            if ((NULL != toneParam_ptr) && 
                    (VTSP_OK == _VTSPR_updateToneSeq(dsp_ptr, toneSeq_ptr,
                    NULL, tId, time,
                    0, /* ignored; seq numToneIds */
                    0, /* ignored; seq control */
                    repeat, VTSPR_TONE_DUAL))) { 
                toneParam_ptr->repeat = repeat;
                _VTSPR_algStateChan(dsp_ptr, infc, 0, VTSPR_ALG_CHAN_TONE);
            }
            else {
                /* 
                 * Stop local tone(s) on infc. Off-mask clears algorithm.
                 */
                _VTSPR_algStateChan(dsp_ptr, infc, VTSPR_ALG_CHAN_TONE, 0); 
            }
            break;
        case _VTSP_CMD_TONE_LOCAL_SEQUENCE:
            /*
             * Near tone generation
             */
            chan_ptr    = _VTSPR_infcToChanPtr(dsp_ptr, infc);
            toneSeq_ptr = &chan_ptr->toneSeq;
            if (VTSP_OK == _VTSPR_updateToneSeq(dsp_ptr,
                        toneSeq_ptr,
                        cmd_ptr->msg.toneSequence.toneIds,
                        0, /* ignored; tId */
                        cmd_ptr->msg.toneSequence.maxTime,
                        cmd_ptr->msg.toneSequence.numToneIds,
                        cmd_ptr->msg.toneSequence.control,
                        cmd_ptr->msg.toneSequence.repeat,
                        VTSPR_TONE_DUAL)) { 
                _VTSPR_algStateChan(dsp_ptr, infc, 
                        0, VTSPR_ALG_CHAN_TONE); 
            }
            else {
                /* 
                 * Stop local tone(s) on infc. Off-mask clears algorithm.
                 */
                _VTSPR_algStateChan(dsp_ptr, infc, VTSPR_ALG_CHAN_TONE, 0); 
            }
            break;
#endif
        case _VTSP_CMD_TONE_QUAD_LOCAL_SEQUENCE:
#ifdef VTSP_ENABLE_TONE_QUAD            
            /*
             * Near genf generation
             */
            chan_ptr    = _VTSPR_infcToChanPtr(dsp_ptr, infc);
            toneSeq_ptr = &chan_ptr->toneQuadSeq;
            if (VTSP_OK == _VTSPR_updateToneSeq(dsp_ptr,
                        toneSeq_ptr,
                        cmd_ptr->msg.toneSequence.toneIds,
                        0, /* ignored; tId */
                        cmd_ptr->msg.toneSequence.maxTime,
                        cmd_ptr->msg.toneSequence.numToneIds,
                        cmd_ptr->msg.toneSequence.control,
                        cmd_ptr->msg.toneSequence.repeat,
                        VTSPR_TONE_QUAD)) { 
                _VTSPR_algStateChan(dsp_ptr, infc, 
                            0, VTSPR_ALG_CHAN_TONE_QUAD);
            }
            else {
                /* 
                 * Stop local tone(s) on chan. Off-mask clears algorithm.
                 */
                _VTSPR_algStateChan(dsp_ptr, infc, 
                        VTSPR_ALG_CHAN_TONE_QUAD, 0); 
            }
#endif
            break;

        case _VTSP_CMD_TONE_QUAD_LOCAL:
            /*
             * Near end GENF generation
             */
#ifdef VTSP_ENABLE_TONE_QUAD
            chan_ptr      = _VTSPR_infcToChanPtr(dsp_ptr, infc);
            tId           = cmd_ptr->msg.arg.arg0;
            repeat        = cmd_ptr->msg.arg.arg1;
            time          = cmd_ptr->msg.arg.arg2;
            toneQuadParam_ptr = dsp_ptr->toneQuadParams_ptr[tId];

            /*
             * Gen tone. Init GENF and set On-mask to call GENF_tone().
             */
            toneSeq_ptr = &chan_ptr->toneQuadSeq;
            if ((NULL != toneQuadParam_ptr) && 
                    (VTSP_OK == _VTSPR_updateToneSeq(dsp_ptr, toneSeq_ptr,
                    NULL, tId, time,
                    0, /* ignored; seq numToneIds */
                    0, /* ignored; seq control */
                    repeat,
                    VTSPR_TONE_QUAD))) { 
#warning XXX check repeat arg, if 0 == infinite, in tone vs genf
                toneQuadParam_ptr->sRepeat = repeat;
                _VTSPR_algStateChan(dsp_ptr, infc, 
                        0, VTSPR_ALG_CHAN_TONE_QUAD);
            }
            else {
                /* 
                 * Stop local tone(s) on chan. Off-mask clears algorithm.
                 */
                _VTSPR_algStateChan(dsp_ptr, infc, 
                        VTSPR_ALG_CHAN_TONE_QUAD, 0); 
            }
#endif
            break;    
#ifndef VTSP_ENABLE_MP_LITE
        case _VTSP_CMD_STREAM_TONE:
            /*
             * Remote peer tone generation
             */
            streamId      = cmd_ptr->msg.arg.arg3;
            stream_ptr    = _VTSPR_streamIdToStreamPtr(dsp_ptr, infc, streamId);
            tId           = cmd_ptr->msg.arg.arg0;
            repeat        = cmd_ptr->msg.arg.arg1;
            time          = cmd_ptr->msg.arg.arg2;
            toneParam_ptr = dsp_ptr->toneParams_ptr[tId];
            toneSeq_ptr   = &stream_ptr->toneSeq;
            if ((NULL != toneParam_ptr) && 
                    (VTSP_OK == _VTSPR_updateToneSeq(dsp_ptr, toneSeq_ptr,
                    NULL, tId, time,
                    0, /* ignored; seq numToneIds */
                    0, /* ignored; seq control */
                    repeat,
                    VTSPR_TONE_DUAL))) { 
                toneParam_ptr->repeat = repeat;
                _VTSPR_algStateStream(dsp_ptr, infc, streamId, 
                        0, VTSPR_ALG_STREAM_TONE);
            }
            else {
                /* 
                 * Stop stream tone(s) on chan. Off-mask clears algorithm.
                 */
                _VTSPR_algStateStream(dsp_ptr, infc, streamId,
                        VTSPR_ALG_STREAM_TONE, 0); 
            }
            break;
#endif
        case _VTSP_CMD_STREAM_TONE_QUAD:
            /*
             * Remote peer GENF generation
             */
#ifdef VTSP_ENABLE_TONE_QUAD
            streamId      = cmd_ptr->msg.arg.arg3;
            stream_ptr   = _VTSPR_streamIdToStreamPtr(dsp_ptr, infc, streamId);
            tId           = cmd_ptr->msg.arg.arg0;
            repeat        = cmd_ptr->msg.arg.arg1;
            time          = cmd_ptr->msg.arg.arg2;
            toneQuadParam_ptr = dsp_ptr->toneQuadParams_ptr[tId];
            toneSeq_ptr   = &stream_ptr->toneQuadSeq;

            /*
             * Gen tone. Init GENF and set On-mask to call GENF_tone().
             */
            toneSeq_ptr = &stream_ptr->toneQuadSeq;
            if ((NULL != toneQuadParam_ptr) && 
                    (VTSP_OK == _VTSPR_updateToneSeq(dsp_ptr, toneSeq_ptr,
                    NULL, tId, time,
                    0, /* ignored; seq numToneIds */
                    0, /* ignored; seq control */
                    repeat,
                    VTSPR_TONE_QUAD))) { 
#warning XXX check repeat arg, if 0 == infinite, in tone vs genf
                toneQuadParam_ptr->sRepeat = repeat;
                _VTSPR_algStateStream(dsp_ptr, infc, streamId,
                        0, VTSPR_ALG_STREAM_TONE_QUAD); 
            }
            else {
                /* 
                 * Stop stream tone. Off-mask clears algorithm.
                 */
                _VTSPR_algStateStream(dsp_ptr, infc, streamId,
                        VTSPR_ALG_STREAM_TONE_QUAD, 0); 
            }
#endif
            break;    
#ifndef VTSP_ENABLE_MP_LITE
        case _VTSP_CMD_STREAM_TONE_SEQUENCE:
            /*
             * STREAM tone runs the stream tone object using the dsp tone
             * template configuration, with the dst_ptr set to the stream
             * buffer.
             *
             * Stream tone may be active simultaneously with the channel
             * tone.  Stream tone may be simultaneously active on all
             * streams.
             */
            streamId = cmd_ptr->msg.toneSequence.streamId;
            stream_ptr = _VTSPR_streamIdToStreamPtr(dsp_ptr, infc, streamId);
            toneSeq_ptr = &stream_ptr->toneSeq;
            if (VTSP_OK == _VTSPR_updateToneSeq(dsp_ptr,
                    toneSeq_ptr,
                    cmd_ptr->msg.toneSequence.toneIds,
                    0, /* ignored; tId */
                    cmd_ptr->msg.toneSequence.maxTime,
                    cmd_ptr->msg.toneSequence.numToneIds,
                    cmd_ptr->msg.toneSequence.control,
                    cmd_ptr->msg.toneSequence.repeat,
                    VTSPR_TONE_DUAL)) { 
                _VTSPR_algStateStream(dsp_ptr, infc, streamId, 
                        0, VTSPR_ALG_STREAM_TONE);
            }
            else {
                /* 
                 * Stop stream tone(s) on chan. Off-mask clears algorithm.
                 */
                _VTSPR_algStateStream(dsp_ptr, infc, streamId,
                        VTSPR_ALG_STREAM_TONE, 0); 
            }
            break;
#endif
        case _VTSP_CMD_STREAM_TONE_QUAD_SEQUENCE:
#ifdef VTSP_ENABLE_TONE_QUAD            
            /*
             * Stream tone generation
             */
            streamId    = cmd_ptr->msg.toneSequence.streamId;
            stream_ptr  = _VTSPR_streamIdToStreamPtr(dsp_ptr, infc, streamId);
            toneSeq_ptr = &stream_ptr->toneQuadSeq;
#warning XXX need to check stream direction here
            if (VTSP_OK == _VTSPR_updateToneSeq(dsp_ptr,
                    toneSeq_ptr,
                    cmd_ptr->msg.toneSequence.toneIds,
                    0, /* ignored; tId */
                    cmd_ptr->msg.toneSequence.maxTime,
                    cmd_ptr->msg.toneSequence.numToneIds,
                    cmd_ptr->msg.toneSequence.control,
                    cmd_ptr->msg.toneSequence.repeat,
                    VTSPR_TONE_QUAD)) { 
                _VTSPR_algStateStream(dsp_ptr, infc, streamId,
                        0, VTSPR_ALG_STREAM_TONE_QUAD); 
            }
            else {
                /* 
                 * Stop stream tone. Off-mask clears algorithm.
                 */
                _VTSPR_algStateStream(dsp_ptr, infc, streamId,
                        VTSPR_ALG_STREAM_TONE_QUAD, 0); 
            }
#endif
            break;
        case _VTSP_CMD_DETECT:
            enable      = cmd_ptr->msg.arg.arg0;

#ifdef VTSP_ENABLE_CIDR
            if (0 != (VTSP_DETECT_CALLERID_ONHOOK & enable)) {
                /*
                 * Enable CIDR ONHOOK
                 * Note, CIDR logic requires DTMF so enable it.
                 */
                _VTSPR_algStateChan(dsp_ptr, infc,
                        0,
                        VTSPR_ALG_CHAN_FSKR_ONH);
                enable |= VTSP_DETECT_DTMF;
            }
            else if (0 != (VTSP_DETECT_CALLERID_OFFHOOK & enable)) {
                /*
                 * Enable CIDR OFFHOOK
                 * Note, CIDR logic requires DTMF so enable it.
                 * VTSPR_ALG_CHAN_FSKR_OFH is turned on by _vtspr_audio.c after
                 * CAS tone is detected.
                 */
                _VTSPR_algStateChan(dsp_ptr, infc,
                        0,
                        VTSPR_ALG_CHAN_CAS);
                enable |= VTSP_DETECT_DTMF;
            }
            else {
                /*
                 * Disable CIDR
                 * DTMF might be disabled below, depending on enable
                 */
                _VTSPR_algStateChan(dsp_ptr, infc,
                        (VTSPR_ALG_CHAN_FSKR_ONH | VTSPR_ALG_CHAN_FSKR_OFH |
                         VTSPR_ALG_CHAN_CAS),
                        0);
            }
#endif

            if (0 != (VTSP_DETECT_DTMF & enable)) {
                /*
                 * Start DTMF detector.
                 */
                _VTSPR_algStateChan(dsp_ptr, infc, 0,
                        VTSPR_ALG_CHAN_DTMF);
            }
            else {
                /*
                 * Stop.
                 */
                _VTSPR_algStateChan(dsp_ptr, infc, VTSPR_ALG_CHAN_DTMF, 0);
            }
#ifdef VTSP_ENABLE_UTD
            if (0 != (VTSP_DETECT_UTD & enable)) {
                /*
                 * Start UTD detector.
                 */
                _VTSPR_algStateChan(dsp_ptr, infc, 0, VTSPR_ALG_CHAN_UTD);
            }
            else {
                /*
                 * Stop.
                 */
                _VTSPR_algStateChan(dsp_ptr, infc, VTSPR_ALG_CHAN_UTD, 0);
            }
#endif

            if (0 != (VTSP_DETECT_FMTD & enable)) {
                /*
                 * Start FMTD detector.
                 */
                _VTSPR_algStateChan(dsp_ptr, infc, 0, VTSPR_ALG_CHAN_FMTD);
            }
            else {
                /*
                 * Stop.
                 * Turn off Fax mode on channel.
                 */
                _VTSPR_algStateChan(dsp_ptr, infc, VTSPR_ALG_CHAN_FMTD |
                        VTSPR_ALG_CHAN_FAX, 0);
            }

            break;

        case _VTSP_CMD_RING:
            if (VTSPR_INFC_TYPE_FXS == _VTSPR_infcToType(infc)) {
                /* RING FXS interface */
                tic_ptr  = _VTSPR_infcToTicPtr(dsp_ptr, infc);
                fxs_ptr  = _VTSPR_infcToInfcPtr(dsp_ptr, infc);
                repeat   = cmd_ptr->msg.arg.arg1;
                if (0 == repeat) {
                    /*
                     * Stop ring
                     * Event will be generated elsewhere, from change in tic
                     * state.
                     *
                     * XXX CID Complete event will not be sent.  This is OK.
                     */
                    _VTSPR_algStateChan(dsp_ptr, infc, VTSPR_ALG_CHAN_CIDS, 0);
                    TIC_control(tic_ptr, TIC_CONTROL_STOP_RING, 0);
                    TIC_control(tic_ptr, TIC_CONTROL_POLARITY_FWD, 0);
                    break;
                }

                /*
                 * Must check if phone is released prior to attempting ring
                 */
                if (TIC_IMMEDIATE_OFFHOOK == TIC_getStatus(tic_ptr,
                        TIC_GET_STATUS_HOOK_IMMEDIATE)) {
                    break;
                }

                /*
                 * Start Ring
                 */
                tId = (uint16)cmd_ptr->msg.arg.arg0;
                time = (cmd_ptr->msg.arg.arg2 / 2);
                ring_ptr = dsp_ptr->ringTemplate_ptr[tId];
                fxs_ptr->ringTemplate_ptr = ring_ptr;
                if (NULL == ring_ptr) {
                    /* No valid ring template */
                    _VTSP_TRACE(__FILE__, __LINE__);
                    break;
                }

                /*
                 * Only ring if a valid cadence was set
                 * and if the line is currently onhook
                 */
                ring_ptr->ticParam.repeat = repeat;
                fxs_ptr->ringTime = time; /* ms */
                fxs_ptr->ringCountMax = repeat;
                fxs_ptr->ringNum = -1;
#ifndef VTSP_ENABLE_CIDS
                fxs_ptr->ringEvent = VTSP_EVENT_ACTIVE;
                TIC_setRingTable(tic_ptr, &ring_ptr->ticParam);
#else /* VTSP_ENABLE_CIDS */
                chan_ptr = _VTSPR_infcToChanPtr(dsp_ptr, infc);
                cids_ptr = chan_ptr->cids_ptr;
                if ((cmd_ptr->msg.arg.arg3 > 0) && (chan_ptr->cidsData.len != 0)) {
                    /*
                     * Ring with Caller-ID Send
                     */

                    /*
                     * Set duration of ring signal time used in CIDS from ring
                     * template information.
                     * This allows FSKS to be inserted during the proper cadence
                     * within the ring.
                     */
                    cids_ptr->cidType = VTSP_EVENT_TYPE_CID_ONHOOK;
                    cids_ptr->cidsParam_ptr->signalTime = 10 *
                            ring_ptr->ticParam.make1;
                    if (ring_ptr->cidsBreakNum > 1) {
                        cids_ptr->cidsParam_ptr->signalTime += 10 *
                                (ring_ptr->ticParam.break1 +
                                ring_ptr->ticParam.make2);
                    }
                    if (ring_ptr->cidsBreakNum > 2) {
                        cids_ptr->cidsParam_ptr->signalTime += 10 *
                                (ring_ptr->ticParam.break2 +
                                 ring_ptr->ticParam.make3);
                    }

                    cids_ptr->cidsParam_ptr->msgSz =
                            chan_ptr->cidsData.len;
                    _VTSPR_algStateChan(dsp_ptr, infc,
                            0, VTSPR_ALG_CHAN_CIDS);
                }
                else {
                    /*
                     * Ring without CIDS
                     */
                    _VTSPR_algStateChan(dsp_ptr, infc,
                            VTSPR_ALG_CHAN_CIDS, 0);
                    fxs_ptr->ringEvent = VTSP_EVENT_ACTIVE;
                    TIC_setRingTable(tic_ptr, &ring_ptr->ticParam);
                }
#endif /* VTSP_ENABLE_CIDS */
            }
            else if (VTSPR_INFC_TYPE_AUDIO == _VTSPR_infcToType(infc)) {
                tic_ptr  = _VTSPR_infcToTicPtr(dsp_ptr, infc);
                chan_ptr = _VTSPR_infcToChanPtr(dsp_ptr, infc);
                repeat   = cmd_ptr->msg.arg.arg1;
                if (0 == repeat) {
                    /*
                     * Stop ring
                     * Event will be generated elsewhere, from change in tic
                     * state.
                     *
                     * XXX CID Complete event will not be sent.  This is OK.
                     */
                    _VTSPR_algStateChan(dsp_ptr, infc, VTSPR_ALG_CHAN_CIDS, 0);
                    TIC_control(tic_ptr, TIC_CONTROL_STOP_RING, 0);
                }
                else {
#ifdef VTSP_ENABLE_HANDSET_CIDS
                    if (chan_ptr->cidsData.len > 0) {
                        tic_ptr->audio.cidLen = chan_ptr->cidsData.len;
                        OSAL_memCpy(tic_ptr->audio.cidData, 
                                chan_ptr->cidsData.data,
                                chan_ptr->cidsData.len);  
                    }
#endif
                    /* RING AUDIO interface */
                    TIC_setRingTable(tic_ptr,
                            (TIC_RingParams *)(&(cmd_ptr->msg.arg.arg1)));
                }
            }
            else {
                /* Ring for FXO is not supported */
                _VTSP_TRACE(__FILE__, __LINE__);
            }
            break;

#ifdef VTSP_ENABLE_CIDS
        case _VTSP_CMD_CID_TX:
            /* Initiate CID data transmission if CID is in DATA mode */
            chan_ptr = _VTSPR_infcToChanPtr(dsp_ptr, infc);
            if (VTSPR_INFC_TYPE_FXS == _VTSPR_infcToType(infc)) {
            cids_ptr = chan_ptr->cids_ptr;
            mask = _VTSPR_getAlgStateChan(chan_ptr, infc);
            /* If CID is running, set control word */
            if (0 != (mask & VTSPR_ALG_CHAN_CIDS)) { 
                cids_ptr->cidsObj.ctrl |= CIDS_CTRL_TX; 
            }
            else if (0 != (mask & VTSPR_ALG_CHAN_CIDCWS)) { 
                cids_ptr->cidcwsObj.ctrl |= CIDCWS_CTRL_TX; 
            }
            }
            break;
#endif
        case _VTSP_CMD_CID_OFFHOOK:
            /*
             * XXX During either DTMF Relay encoding or decoding "in digit"
             * state, transmission of CIDCWS must override the DTMF Relay.  So
             * this command should end any currently active DTMF relay digits
             * somehow.
             */


#ifdef VTSP_ENABLE_CIDS
            chan_ptr = _VTSPR_infcToChanPtr(dsp_ptr, infc);
            if (VTSPR_INFC_TYPE_FXS == _VTSPR_infcToType(infc)) {
                cids_ptr = chan_ptr->cids_ptr;
                if (chan_ptr->cidsData.len != 0) {
                    /*
                     * Type 2 Caller-ID Send
                     */
                    cids_ptr->cidType = VTSP_EVENT_TYPE_CID_OFFHOOK;
                    cids_ptr->cidcwsParam_ptr->msgSz = chan_ptr->cidsData.len;
                    _VTSPR_algStateChan(dsp_ptr, infc, 0, 
                            VTSPR_ALG_CHAN_CIDCWS);
                }
            }
#endif /* VTSP_ENABLE_CIDS */
#ifdef VTSP_ENABLE_HANDSET_CIDS
            if (VTSPR_INFC_TYPE_AUDIO == _VTSPR_infcToType(infc)) {
                if (chan_ptr->cidsData.len > 0) {
                    tic_ptr->audio.cidLen = chan_ptr->cidsData.len;
                    OSAL_memCpy(tic_ptr->audio.cidData,
                            chan_ptr->cidsData.data,
                            chan_ptr->cidsData.len);
                }
                TIC_control(tic_ptr, TIC_CONTROL_HANDSET_CIDS, 0);
            }
#endif
            break;
        case _VTSP_CMD_INFC_LINE_STATUS:
            /*
             * Reset hook state for interface, which
             * generates a 'change' event during event processing
             */
            if (VTSPR_INFC_TYPE_FXS == _VTSPR_infcToType(infc)) {
                fxs_ptr  = _VTSPR_infcToInfcPtr(dsp_ptr, infc);
                fxs_ptr->hookEvent = -1;
            }
            else if (VTSPR_INFC_TYPE_FXO == _VTSPR_infcToType(infc)) {
                fxo_ptr  = _VTSPR_infcToInfcPtr(dsp_ptr, infc);
                fxo_ptr->hookEvent = -1;
            }
            break;

        case _VTSP_CMD_INFC_CONTROL_HOOK:
            tic_ptr = _VTSPR_infcToTicPtr(dsp_ptr, infc);
            ticCode = _VTSPR_infcLineToTicCode(
                    (VTSP_ControlInfcLine)cmd_ptr->msg.control.code);
            TIC_control(tic_ptr, ticCode, 0);   
            break;         

        case _VTSP_CMD_INFC_CONTROL_IO:
            chan = _VTSPR_infcToChan(infc);
            switch(cmd_ptr->msg.control.code) {
                case VTSP_CONTROL_INFC_IO_MUTE_INPUT_SW:
                    _VTSPR_setChanMute(dsp_ptr, chan, VTSPR_ALG_CHAN_MUTE_IN,
                            cmd_ptr->msg.control.arg);
                    break;
                case VTSP_CONTROL_INFC_IO_MUTE_OUTPUT_SW:
                    _VTSPR_setChanMute(dsp_ptr, chan, VTSPR_ALG_CHAN_MUTE_OUT,
                            cmd_ptr->msg.control.arg);
                    break;
                default:
                    tic_ptr = _VTSPR_infcToTicPtr(dsp_ptr, infc);
                    ticCode = _VTSPR_infcIOToTicCode(
                            (VTSP_ControlInfcIO)cmd_ptr->msg.control.code);
                    TIC_control(tic_ptr, ticCode, cmd_ptr->msg.control.arg);
                    break;
            };
            break;
 
        case _VTSP_CMD_CONFIG:
            switch (cmd_ptr->msg.config.templCode) {
                case VTSP_TEMPL_CODE_JB:
                    streamId = cmd_ptr->msg.config.u.data[1];
                    stream_ptr = _VTSPR_streamIdToStreamPtr(dsp_ptr, infc,
                            streamId);
                    mask = 0;
                    jbParams_ptr = &(stream_ptr->jbParams);
                    switch (cmd_ptr->msg.config.u.data[0]) { 
                        case VTSP_TEMPL_CONTROL_JB_VOICE:
                            jbParams_ptr->eFreeze = JB_ADAPT_RUN;
                            jbParams_ptr->eDtRly = JB_DTRLY_ENABLE;
                            jbParams_ptr->maxLevel = 
                                    cmd_ptr->msg.config.u.data[2];
                            jbParams_ptr->initLevel = 
                                    cmd_ptr->msg.config.u.data[3];
                            break;
                        case VTSP_TEMPL_CONTROL_JB_FIXED:
                            jbParams_ptr->eFreeze = JB_FIXED;
                            jbParams_ptr->eDtRly = JB_DTRLY_DISABLE;
                            jbParams_ptr->maxLevel = 
                                    cmd_ptr->msg.config.u.data[2];
                            jbParams_ptr->initLevel = 
                                    cmd_ptr->msg.config.u.data[3];
                            break;
                        case VTSP_TEMPL_CONTROL_JB_FREEZE:
                            jbParams_ptr->eFreeze = JB_FREEZE_SIZE;
                            break;
                    }
                    /* Reinit by setting JB as on mask */
                    mask = stream_ptr->algStreamState & VTSPR_ALG_STREAM_JB;
                    if (mask > 0) { 
                        /* Only turn on JB if already on */
                        _VTSPR_algStateStream(dsp_ptr, infc, streamId, 0,
                                VTSPR_ALG_STREAM_JB);
                    }
                    break;

                case VTSP_TEMPL_CODE_FR38:
#ifdef VTSP_ENABLE_T38
                    fr38Params_ptr = dsp_ptr->fr38Params_ptr;
                    switch (cmd_ptr->msg.config.u.data[0]) {
                        case VTSP_TEMPL_CONTROL_FR38_RATE_MAX:
                            fr38Params_ptr->pMAXRATE =
                                    cmd_ptr->msg.config.u.data[1];
                            break;
                        case VTSP_TEMPL_CONTROL_FR38_JITTER_MAX:
                            fr38Params_ptr->pMAX_JITTER = 
                                    cmd_ptr->msg.config.u.data[1];
                            break;
                        case VTSP_TEMPL_CONTROL_FR38_ECC:
                            fr38Params_ptr->pECC_Type =
                                    cmd_ptr->msg.config.u.data[1];
                            fr38Params_ptr->pECC_Signal =
                                    cmd_ptr->msg.config.u.data[2];
                            fr38Params_ptr->pECC_Data =
                                    cmd_ptr->msg.config.u.data[3];
                            break;
                        case VTSP_TEMPL_CONTROL_FR38_RATE_MGMT:
                            fr38Params_ptr->pRate_Management =
                                    cmd_ptr->msg.config.u.data[1];
                            break;
                        case VTSP_TEMPL_CONTROL_FR38_ECM_MODE:
                            fr38Params_ptr->pECM_Enabled =
                                    cmd_ptr->msg.config.u.data[1];
                            break;
                        case VTSP_TEMPL_CONTROL_FR38_CED_LEN:
                            dsp_ptr->fr38Global_ptr->pCEDLength =
                                    cmd_ptr->msg.config.u.data[1];
                            break;
                    }
#endif
                    break;

                case VTSP_TEMPL_CODE_RTCP:
                    streamId = cmd_ptr->msg.config.u.data[1];
                    rtcp_ptr = _VTSPR_streamIdToRtcpPtr(vtspr_ptr->net_ptr,
                            infc, streamId);
                    _VTSPR_rtcpSetControl(rtcp_ptr, cmd_ptr->msg.config.u.data[0],
                            cmd_ptr->msg.config.u.data[2]);
                    break;
                case VTSP_TEMPL_CODE_RTP:
                    streamId = cmd_ptr->msg.config.u.data[1];
                    rtp_ptr = _VTSPR_streamIdToRtpPtr(vtspr_ptr->net_ptr,
                            infc, streamId);
                    stream_ptr = _VTSPR_streamIdToStreamPtr(dsp_ptr, infc,
                            streamId);
                    switch (cmd_ptr->msg.config.u.data[0]) { 
                        case VTSP_TEMPL_CONTROL_RTP_TOS:
                            rtp_ptr->tos = cmd_ptr->msg.config.u.data[2];
                            if (OSAL_FAIL == VOICE_NET_SET_SOCKET_OPTIONS(&rtp_ptr->socket,
                                    OSAL_NET_IP_TOS, rtp_ptr->tos)) {
                                _VTSP_TRACE(__FILE__, __LINE__);
                            }
                            break;
                        case VTSP_TEMPL_CONTROL_RTP_TS_INIT:
                            rtp_ptr->tsRandom = cmd_ptr->msg.config.u.data[2];
                            break;
                        case VTSP_TEMPL_CONTROL_RTP_SN_INIT:
                            rtp_ptr->seqRandom = cmd_ptr->msg.config.u.data[2];
                            break;
#ifdef VTSP_ENABLE_RTP_REDUNDANT                            
                        /* RFC2198 configuration */
                        case VTSP_TEMPL_CONTROL_RTP_REDUNDANT:
                            /* Get the redundant Level, 0 means disable */
                            rtp_ptr->recvRtpObj.rdnCache_ptr->level =
                            rtp_ptr->sendRtpObj.rdnCache_ptr->level =
                                    cmd_ptr->msg.config.u.data[2];
                            /* Get the value of Delayed Hop */
                            parameter = cmd_ptr->msg.config.u.data[3];
                            if (parameter < 1) {
                                /* Hop must be at least 1 if 2198 enabled */
                                parameter = 
                                        rtp_ptr->sendRtpObj.rdnCache_ptr->level;
                            }
                            rtp_ptr->sendRtpObj.rdnCache_ptr->hop = parameter;
                            rtp_ptr->recvRtpObj.rdnCache_ptr->hop = parameter;
                            break;
#endif
                    }
                    break;
                case VTSP_TEMPL_CODE_FMTD:
#ifdef VTSP_ENABLE_FMTD                    
                    switch (cmd_ptr->msg.config.u.data[0]) { 
                        case VTSP_TEMPL_CONTROL_FMTD_GBL_PWR_MIN_INFC:
                            dsp_ptr->fmtdGlobalPowerMinInfc = 
                                    (vint)cmd_ptr->msg.config.u.data[1];
                            /*
                             * BUG 2432 FIX
                             * For all channels, check if FMTD is running
                             * If so turn FMTD off/on to re-init. 
                             */
                            for (ctr = 0; ctr < _VTSP_INFC_NUM; ctr++) {
                                chan = _VTSPR_infcToChan(ctr);
                                chan_ptr = _VTSPR_infcToChanPtr(dsp_ptr, infc);
                                if (0 != (chan_ptr->algChannelState &
                                        VTSPR_ALG_CHAN_FMTD)) {
                                    _VTSPR_algStateChan(dsp_ptr, chan, 
                                            VTSPR_ALG_CHAN_FMTD, 0);
                                    _VTSPR_algStateChan(dsp_ptr, chan, 
                                            0, VTSPR_ALG_CHAN_FMTD);
                                }
                            }
                            break;
                        case VTSP_TEMPL_CONTROL_FMTD_GBL_PWR_MIN_PEER:
                            dsp_ptr->fmtdGlobalPowerMinPeer = 
                                    (vint)cmd_ptr->msg.config.u.data[1];
                            /* BUG 2432 FIX - see comment above */
                            for (ctr = 0; ctr < _VTSP_INFC_NUM; ctr++) {
                                chan = _VTSPR_infcToChan(ctr);
                                chan_ptr = _VTSPR_infcToChanPtr(dsp_ptr, infc);
                                if (0 != (chan_ptr->algChannelState &
                                        VTSPR_ALG_CHAN_FMTD)) {
                                    _VTSPR_algStateChan(dsp_ptr, chan, 
                                            VTSPR_ALG_CHAN_FMTD, 0);
                                    _VTSPR_algStateChan(dsp_ptr, chan, 
                                            0, VTSPR_ALG_CHAN_FMTD);
                                }
                            }
                            break;
                        case VTSP_TEMPL_CONTROL_FMTD_DETECT_MASK:
                            chan        = _VTSPR_infcToChan(infc);
                            chan_ptr = _VTSPR_infcToChanPtr(dsp_ptr, infc);
                            enable      = cmd_ptr->msg.config.u.data[1];
                            fmtd_ptr    = &chan_ptr->fmtd_ptr->fmtdInfcObj;
                            fmtd_ptr->control = enable;
                            fmtd_ptr    = &chan_ptr->fmtd_ptr->fmtdPeerObj;
                            fmtd_ptr->control = enable;
                            /* 
                             * After setting control, turn FMTD off then on to
                             * initialize the algorithm
                             */
                            _VTSPR_algStateChan(dsp_ptr, chan,
                                    VTSPR_ALG_CHAN_FMTD, 0);
                            _VTSPR_algStateChan(dsp_ptr, chan, 0,
                                    VTSPR_ALG_CHAN_FMTD);
                            break;
                        default:
                            _VTSP_TRACE(__FILE__, __LINE__);
                            break;
                    }
#endif /* VTSP_ENABLE_FMTD */
                    break;
                case VTSP_TEMPL_CODE_CID:
#ifdef VTSP_ENABLE_CIDS
                    chan_ptr = _VTSPR_infcToChanPtr(dsp_ptr, infc);
                    switch (cmd_ptr->msg.config.u.data[0]) {
                        case VTSP_TEMPL_CONTROL_CID_FORMAT:
                            _VTSPR_cidSetCountryCode(chan_ptr, infc,
                                    cmd_ptr->msg.config.u.data[1]);
                            break;
                        case VTSP_TEMPL_CONTROL_CID_MDID_TIMEOUT:
                            cids_ptr = chan_ptr->cids_ptr;                    
                            cids_ptr->cidsParam_ptr->onTimeout
                                = (cmd_ptr->msg.config.u.data[1]
                                        - CIDS_LOCALS_ENDTIME_JP);
                            break;
                        case VTSP_TEMPL_CONTROL_CID_FSK_PWR:
                            cids_ptr = chan_ptr->cids_ptr;
                            cids_ptr->cidsParam_ptr->markPwr = 
                                    cmd_ptr->msg.config.u.data[1];
                            cids_ptr->cidsParam_ptr->spacePwr = 
                                    cmd_ptr->msg.config.u.data[1];

                            cids_ptr->cidcwsParam_ptr->markPwr = 
                                    cmd_ptr->msg.config.u.data[1];
                            cids_ptr->cidcwsParam_ptr->spacePwr = 
                                    cmd_ptr->msg.config.u.data[1];
                            break;
                        case VTSP_TEMPL_CONTROL_CID_SAS_PWR:
                            cids_ptr = chan_ptr->cids_ptr;
                            cids_ptr->cidcwsParam_ptr->sasPwr = 
                                    cmd_ptr->msg.config.u.data[1];
                            break;
                        case VTSP_TEMPL_CONTROL_CID_CAS_PWR:
                            cids_ptr = chan_ptr->cids_ptr;
                            cids_ptr->cidcwsParam_ptr->casPwr = 
                                    cmd_ptr->msg.config.u.data[1];
                            break;
                    }
#endif
                    break;

                case VTSP_TEMPL_CODE_EC:
#ifdef VTSP_ENABLE_ESCR
                    switch (cmd_ptr->msg.config.u.data[0]) {
                        case VTSP_TEMPL_CONTROL_EC_NORMAL:
                            /* 
                             * Normal mode
                             * Turn off FMTD mode, turn off freeze.
                             * Turn EC on, if infc is seized 
                             */
                            _VTSPR_algStateChan(dsp_ptr, infc, 
                                    VTSPR_ALG_CHAN_ECSR_FMTD | 
                                    VTSPR_ALG_CHAN_ECSR_FRZ, 0);
                            tic_ptr = _VTSPR_infcToTicPtr(dsp_ptr, infc);
                            if (TIC_SEIZE == TIC_getStatus(tic_ptr,
                                    TIC_GET_STATUS_HOOK)) { 
                                _VTSPR_algStateChan(dsp_ptr, infc, 
                                        0, VTSPR_ALG_CHAN_ECSR); 
                            }
                            break;
                        case VTSP_TEMPL_CONTROL_EC_BYPASS:
                            /* Debug mode. Turn off EC */
                            _VTSPR_algStateChan(dsp_ptr, infc, 
                                    VTSPR_ALG_CHAN_ECSR, 0); 
                            break;
                        case VTSP_TEMPL_CONTROL_EC_FREEZE:
                            /*
                             * Debug mode. Turn on EC, enable Freeze.
                             * Note FRZ is 'sticky' until EC is turned OFF 
                             * or ON.
                             */
                            _VTSPR_algStateChan(dsp_ptr, infc, 0,
                                    VTSPR_ALG_CHAN_ECSR | 
                                    VTSPR_ALG_CHAN_ECSR_FRZ); 
                            break;
                        case VTSP_TEMPL_CONTROL_EC_FMTD:
                            /*
                             * Enable EC-FMTD.  Freeze may still
                             * be active.
                             * Turn EC on, if infc is seized 
                             */
#ifdef VTSP_ENABLE_FMTD
                            _VTSPR_algStateChan(dsp_ptr, infc, 0,
                                    VTSPR_ALG_CHAN_ECSR_FMTD); 
                            tic_ptr  = _VTSPR_infcToTicPtr(dsp_ptr, infc);
                            if (TIC_SEIZE == TIC_getStatus(tic_ptr,
                                    TIC_GET_STATUS_HOOK)) { 
                                _VTSPR_algStateChan(dsp_ptr, infc, 0,
                                        VTSPR_ALG_CHAN_ECSR); 
                            }
                            /* 
                             * Process .silenceTime word.
                             */
                            chan_ptr = _VTSPR_infcToChanPtr(dsp_ptr, infc);
                            chan_ptr->fmtd_ptr->fmtdSilenceTimeMax = 
                                    cmd_ptr->msg.config.u.data[1];
                            dsp_ptr->faxSilRxDB = cmd_ptr->msg.config.u.data[2];
                            dsp_ptr->faxSilTxDB = cmd_ptr->msg.config.u.data[3];
#endif
                            break;
                    }
#endif
                    break;

                case VTSP_TEMPL_CODE_AEC:
#ifdef VTSP_ENABLE_AEC
                    chan_ptr = _VTSPR_infcToChanPtr(dsp_ptr, infc);
                    switch (cmd_ptr->msg.config.u.data[0]) {
                        case VTSP_TEMPL_CONTROL_AEC_NORMAL:
                            /* Normal mode, AEC active */ 
                            chan_ptr->aec_ptr->aecNearObj.control = 0; 
                            break;
                        case VTSP_TEMPL_CONTROL_AEC_BYPASS:
                            /* Bypass mode. Turn off AEC */
                            chan_ptr->aec_ptr->aecNearObj.control =
                                    AEC_CONTROL_AEC_BYPASS;
                            break;
                        case VTSP_TEMPL_CONTROL_AEC_BYPASS_AGC:
                            /* Bypass AGC */
                            chan_ptr->aec_ptr->aecNearObj.control |=
                                   (AEC_CONTROL_AGC_ROUT_BYPASS | 
                                    AEC_CONTROL_AGC_SOUT_BYPASS);
                            break;
                        case VTSP_TEMPL_CONTROL_AEC_HANDSET_MODE:
                            /* Re-init AEC with handset mode parameters */
                            dsp_ptr->aecNearParams_ptr = 
                                    &_VTSPR_aecHandsetParams;
                            /* AEC re-init */
                            AEC_init(&(chan_ptr->aec_ptr->aecNearObj),
                                    dsp_ptr->globals_ptr,
                                    dsp_ptr->aecNearParams_ptr,
                                    AEC_INIT_WARM);
                            break;
                        case VTSP_TEMPL_CONTROL_AEC_HANDSFREE_MODE:
                            /* Re-init AEC with speaker phone mode parameters */
                            dsp_ptr->aecNearParams_ptr = 
                                    &_VTSPR_aecHandsFreeParams;
                            /* AEC re-init */
                            AEC_init(&(chan_ptr->aec_ptr->aecNearObj),
                                    dsp_ptr->globals_ptr,
                                    dsp_ptr->aecNearParams_ptr,
                                    AEC_INIT_WARM);
                            break;
                        case VTSP_TEMPL_CONTROL_AEC_TAIL_LENGTH:
                            /* Re-init AEC with new tail length */
//                            dsp_ptr->aecNearParams_ptr->pECHO_TAIL_LENGTH =
//                               cmd_ptr->msg.config.u.data[1];
                            /* AEC re-init */
                            AEC_init(&(chan_ptr->aec_ptr->aecNearObj),
                                    dsp_ptr->globals_ptr,
                                    dsp_ptr->aecNearParams_ptr,
                                    AEC_INIT_WARM); 
                            break;
                        case VTSP_TEMPL_CONTROL_AEC_FREEZE:
                            chan_ptr->aec_ptr->aecNearObj.control |=
                                    AEC_CONTROL_AEC_FREEZE;
                            break;
                        case VTSP_TEMPL_CONTROL_AEC_COMFORT_NOISE:
                            chan_ptr->aec_ptr->aecNearObj.control |=
                                    AEC_CONTROL_AEC_COMFORT_NOISE;
                            break;
                        case VTSP_TEMPL_CONTROL_AEC_HALF_DUPLEX:
                            chan_ptr->aec_ptr->aecNearObj.control |=
                                    AEC_CONTROL_AEC_HALF_DUPLEX;
                            /* Clean bypass and freeze bit */
                            chan_ptr->aec_ptr->aecNearObj.control &=
                                    ~(AEC_CONTROL_AEC_BYPASS |
                                    AEC_CONTROL_AEC_FREEZE);
                            break;
                    }
#endif
                    break;

                case VTSP_TEMPL_CODE_CN:
                    chan_ptr = _VTSPR_infcToChanPtr(dsp_ptr, infc);
                    switch (cmd_ptr->msg.config.u.data[0]) {
                        case VTSP_TEMPL_CONTROL_CN_POWER_ATTEN:
                            /* Set CN Power Attenuation */
                            dsp_ptr->cnPwrAttenDb = cmd_ptr->msg.config.u.data[1];
                            break;
                        default:
                            _VTSP_TRACE(__FILE__, __LINE__);
                    }
                    break;

                case VTSP_TEMPL_CODE_TIC:
                    tic_ptr = _VTSPR_infcToTicPtr(dsp_ptr, infc);
                    switch (cmd_ptr->msg.config.u.data[0]) {
                        case VTSP_TEMPL_CONTROL_TIC_SET_FLASH_TIME:
                            TIC_control(tic_ptr, TIC_CONTROL_FLASH_TIME_MIN,
                                    cmd_ptr->msg.config.u.data[1]);
                            TIC_control(tic_ptr, TIC_CONTROL_FLASH_TIME_MAX,
                                    cmd_ptr->msg.config.u.data[2]);
                            break;
                        case VTSP_TEMPL_CONTROL_TIC_SET_RELEASE_TIME:
                            TIC_control(tic_ptr, TIC_CONTROL_RELEASE_TIME_MIN,
                                    cmd_ptr->msg.config.u.data[1]);
                            break;
                        case VTSP_TEMPL_CONTROL_TIC_SET_PDD_PARAMS:
                            TIC_control(tic_ptr, TIC_CONTROL_PDD_MAKE_MIN,
                                    cmd_ptr->msg.config.u.data[1]);
                            TIC_control(tic_ptr, TIC_CONTROL_PDD_MAKE_MAX,
                                    cmd_ptr->msg.config.u.data[2]);
                            TIC_control(tic_ptr, TIC_CONTROL_PDD_BREAK_MIN,
                                    cmd_ptr->msg.config.u.data[3]);
                            TIC_control(tic_ptr, TIC_CONTROL_PDD_BREAK_MAX,
                                    cmd_ptr->msg.config.u.data[4]);
                            TIC_control(tic_ptr, TIC_CONTROL_PDD_INTERDIGIT_MIN,
                                    cmd_ptr->msg.config.u.data[5]);
                            break;
                    }
                    break;

                case VTSP_TEMPL_CODE_RING:
                    tId = cmd_ptr->msg.config.u.data[0];
                    ring_ptr = dsp_ptr->ringTemplate_ptr[tId];
                    if (NULL == ring_ptr)  {
                        break;
                    }
                    ring_ptr->ticParam.numCads = cmd_ptr->msg.config.u.data[1];
                    ring_ptr->ticParam.make1   = cmd_ptr->msg.config.u.data[2];
                    ring_ptr->ticParam.break1  = cmd_ptr->msg.config.u.data[3];
                    ring_ptr->ticParam.make2   = cmd_ptr->msg.config.u.data[4];
                    ring_ptr->ticParam.break2  = cmd_ptr->msg.config.u.data[5];
                    ring_ptr->ticParam.make3   = cmd_ptr->msg.config.u.data[6];
                    ring_ptr->ticParam.break3  = cmd_ptr->msg.config.u.data[7];
                    ring_ptr->cidsBreakNum     = cmd_ptr->msg.config.u.data[8];
                    break;
#ifndef VTSP_ENABLE_MP_LITE
                case VTSP_TEMPL_CODE_TONE:
                    tId = cmd_ptr->msg.config.u.data[0];
                    toneParam_ptr = dsp_ptr->toneParams_ptr[tId];
                    if (NULL == toneParam_ptr) {
                        break;
                    }
                    toneParam_ptr->tone1    = cmd_ptr->msg.config.u.data[1];
                    toneParam_ptr->tone2    = cmd_ptr->msg.config.u.data[2];
                    toneParam_ptr->t1Pw     = cmd_ptr->msg.config.u.data[3];
                    toneParam_ptr->t2Pw     = cmd_ptr->msg.config.u.data[4];
                    toneParam_ptr->numCads  = cmd_ptr->msg.config.u.data[5];
                    toneParam_ptr->make1    = cmd_ptr->msg.config.u.data[6];
                    toneParam_ptr->break1   = cmd_ptr->msg.config.u.data[7];
                    toneParam_ptr->repeat1  = cmd_ptr->msg.config.u.data[8];
                    toneParam_ptr->make2    = cmd_ptr->msg.config.u.data[9];
                    toneParam_ptr->break2   = cmd_ptr->msg.config.u.data[10];
                    toneParam_ptr->repeat2  = cmd_ptr->msg.config.u.data[11];
                    toneParam_ptr->make3    = cmd_ptr->msg.config.u.data[12];
                    toneParam_ptr->break3   = cmd_ptr->msg.config.u.data[13];
                    toneParam_ptr->repeat3  = cmd_ptr->msg.config.u.data[14];
                    if (0 == toneParam_ptr->tone2) {
                        toneParam_ptr->ctrlWord = TONE_MONO;
                    }
                    else {
                        toneParam_ptr->ctrlWord = TONE_DUAL;
                    }
                    break;
#endif
#ifdef VTSP_ENABLE_UTD
                case VTSP_TEMPL_CODE_UTD:
                    switch (cmd_ptr->msg.config.u.data[0]) {
                        case VTSP_TEMPL_CONTROL_UTD_MASK:
                            chan_ptr = _VTSPR_infcToChanPtr(dsp_ptr, infc);
                            /*
                             * Modifying UTD mask while UTD is active has
                             * unknown effects.  So disable UTD on infc first,
                             * if it is running.
                             */
                            mask = _VTSPR_getAlgStateChan(chan_ptr, infc) &
                                    VTSPR_ALG_CHAN_UTD;
                            if (mask) {
                                /* UTD is on.  Turn off UTD. */
                                _VTSPR_algStateChan(dsp_ptr, infc,
                                        VTSPR_ALG_CHAN_UTD, 0);
                            }
                            /* Modify the detection mask */
                            utd_ptr = chan_ptr->utd_ptr;
                            utd_ptr->utdObj.detectionMask =
                                    (uint16) cmd_ptr->msg.config.u.data[1];
                            if (mask) {
                                /* UTD was on.  Turn on UTD again. */
                                _VTSPR_algStateChan(dsp_ptr, infc, 0,
                                        VTSPR_ALG_CHAN_UTD);
                            }
                            break;
                        case VTSP_TEMPL_CONTROL_UTD_TONE:
                            tId = cmd_ptr->msg.config.u.data[1];
                            if (tId > dsp_ptr->utdParamNum) {
                                /*
                                 * Application error.
                                 * Only allow to add new tId sequentially.
                                 * Required for UTD translated params list.
                                 */
                                _VTSP_TRACE(__FILE__, __LINE__);
                                break;
                            }
                            else if (tId == dsp_ptr->utdParamNum) {
                                /* New tId, increment number on list */
                                dsp_ptr->utdParamNum++;
                            }
                            utdToneParam_ptr = dsp_ptr->utdParams_ptr[tId];
                            if (NULL == utdToneParam_ptr) {
                                break;
                            }
                            /*
                             * Running UTD translate while UTD is active has
                             * unknown effects.  So disable UTD on all infcs
                             * first.  This loop uses infc variable so it will
                             * not be valid afterwards.
                             */
                            _VTSPR_FOR_ALL_FXO(infc) {
                                /* Stop UTD */
                                _VTSPR_algStateChan(dsp_ptr, infc,
                                        VTSPR_ALG_CHAN_UTD, 0);
                            }
                            type = cmd_ptr->msg.config.u.data[2];
                            if (VTSP_TEMPL_UTD_TONE_TYPE_SIT == type) {
                                utdSitParam_ptr = dsp_ptr->utdParams_ptr[tId];
                                utdSitParam_ptr->code = type;
                                /* sit.control */
                                utdSitParam_ptr->type =
                                        cmd_ptr->msg.config.u.data[3]; 
                                /* UTD return value is the template Id */
                                utdSitParam_ptr->retVal = tId;
                                utdSitParam_ptr->freq1 =
                                        cmd_ptr->msg.config.u.data[4];
                                utdSitParam_ptr->freqDev1 =
                                        cmd_ptr->msg.config.u.data[5];
                                utdSitParam_ptr->freq2 =
                                        cmd_ptr->msg.config.u.data[6];
                                utdSitParam_ptr->freqDev2 =
                                        cmd_ptr->msg.config.u.data[7];
                                utdSitParam_ptr->freq3 =
                                        cmd_ptr->msg.config.u.data[8];
                                utdSitParam_ptr->freqDev3 =
                                        cmd_ptr->msg.config.u.data[9];
                                utdSitParam_ptr->freq4 =
                                        cmd_ptr->msg.config.u.data[10];
                                utdSitParam_ptr->freqDev4 =
                                        cmd_ptr->msg.config.u.data[11];
                                utdSitParam_ptr->freq5 =
                                        cmd_ptr->msg.config.u.data[12];
                                utdSitParam_ptr->freqDev5 =
                                        cmd_ptr->msg.config.u.data[13];
                                utdSitParam_ptr->shortMin =
                                        cmd_ptr->msg.config.u.data[14];
                                utdSitParam_ptr->shortMax =
                                        cmd_ptr->msg.config.u.data[15];
                                utdSitParam_ptr->longMin =
                                        cmd_ptr->msg.config.u.data[16];
                                utdSitParam_ptr->longMax =
                                        cmd_ptr->msg.config.u.data[17];
                                utdSitParam_ptr->power =
                                        cmd_ptr->msg.config.u.data[18];
                            }
                            else {
                                /* Assume dual type */
                                utdToneParam_ptr->code = type;
                                utdToneParam_ptr->type =
                                        cmd_ptr->msg.config.u.data[3]; /* dual.control */
                                /* UTD return value is the template Id */
                                utdToneParam_ptr->retVal = tId;
                                utdToneParam_ptr->numCads =
                                        cmd_ptr->msg.config.u.data[4];
                                utdToneParam_ptr->freq1 =
                                        cmd_ptr->msg.config.u.data[5];
                                utdToneParam_ptr->freqDev1 =
                                        cmd_ptr->msg.config.u.data[6];
                                utdToneParam_ptr->freq2 =
                                        cmd_ptr->msg.config.u.data[7];
                                utdToneParam_ptr->freqDev2 =
                                        cmd_ptr->msg.config.u.data[8];
                                utdToneParam_ptr->minMake1 =
                                        cmd_ptr->msg.config.u.data[9];
                                utdToneParam_ptr->maxMake1 =
                                        cmd_ptr->msg.config.u.data[10];
                                utdToneParam_ptr->minBreak1 =
                                        cmd_ptr->msg.config.u.data[11];
                                utdToneParam_ptr->maxBreak1 =
                                        cmd_ptr->msg.config.u.data[12];
                                utdToneParam_ptr->minMake2 =
                                        cmd_ptr->msg.config.u.data[13];
                                utdToneParam_ptr->maxMake2 =
                                        cmd_ptr->msg.config.u.data[14];
                                utdToneParam_ptr->minBreak2 =
                                        cmd_ptr->msg.config.u.data[15];
                                utdToneParam_ptr->maxBreak2 =
                                        cmd_ptr->msg.config.u.data[16];
                                utdToneParam_ptr->minMake3 =
                                        cmd_ptr->msg.config.u.data[17];
                                utdToneParam_ptr->maxMake3 =
                                        cmd_ptr->msg.config.u.data[18];
                                utdToneParam_ptr->minBreak3 =
                                        cmd_ptr->msg.config.u.data[19];
                                utdToneParam_ptr->maxBreak3 =
                                        cmd_ptr->msg.config.u.data[20];
                                utdToneParam_ptr->power =
                                        cmd_ptr->msg.config.u.data[21];
                            }
                            /* Translate the new tone */
                            _VTSPR_utdTranslate(dsp_ptr);
                            break;
                        default:
                            _VTSP_TRACE(__FILE__, __LINE__); /* XXX */
                            break;
                    }
                    break;
#endif

                case VTSP_TEMPL_CODE_TONE_QUAD:
#ifdef VTSP_ENABLE_TONE_QUAD
                    index = 0;
                    tId = cmd_ptr->msg.config.u.data[index++];
                    toneQuadParam_ptr = dsp_ptr->toneQuadParams_ptr[tId];
                    if(NULL != toneQuadParam_ptr) {
                        toneQuadParam_ptr->ctrlWord = 
                                cmd_ptr->msg.config.u.data[index++];
                        if ((GENF_MONO == toneQuadParam_ptr->ctrlWord) ||
                                (GENF_DUAL == toneQuadParam_ptr->ctrlWord) ||
                                (GENF_TRI == toneQuadParam_ptr->ctrlWord) ||
                                (GENF_QUAD == toneQuadParam_ptr->ctrlWord)) { 
                            /* 4 tone */
                            toneQuadParam_ptr->tone.quad.tone1 =
                                cmd_ptr->msg.config.u.data[index++];
                            toneQuadParam_ptr->tone.quad.tone2 =
                                cmd_ptr->msg.config.u.data[index++];
                            toneQuadParam_ptr->tone.quad.tone3 =
                                cmd_ptr->msg.config.u.data[index++];
                            toneQuadParam_ptr->tone.quad.tone4 =
                                cmd_ptr->msg.config.u.data[index++];
                            toneQuadParam_ptr->tone.quad.t1Pw =
                                cmd_ptr->msg.config.u.data[index++];
                            toneQuadParam_ptr->tone.quad.t2Pw =
                                cmd_ptr->msg.config.u.data[index++];
                            toneQuadParam_ptr->tone.quad.t3Pw =
                                cmd_ptr->msg.config.u.data[index++];
                            toneQuadParam_ptr->tone.quad.t4Pw =
                                cmd_ptr->msg.config.u.data[index++];
                        }
                        else {
                            /* modulated */
                            toneQuadParam_ptr->tone.mod.carrier =
                                cmd_ptr->msg.config.u.data[index++];
                            toneQuadParam_ptr->tone.mod.signal =
                                cmd_ptr->msg.config.u.data[index++];
                            toneQuadParam_ptr->tone.mod.power =
                                cmd_ptr->msg.config.u.data[index++];
                            toneQuadParam_ptr->tone.mod.mIndex =
                                cmd_ptr->msg.config.u.data[index++];
                        }
                        index = 10;
                        toneQuadParam_ptr->numCads = 
                                cmd_ptr->msg.config.u.data[index++];
                        toneQuadParam_ptr->tMake1 = 
                                cmd_ptr->msg.config.u.data[index++];
                        toneQuadParam_ptr->tBreak1 = 
                                cmd_ptr->msg.config.u.data[index++];
                        toneQuadParam_ptr->tRepeat1 = 
                                cmd_ptr->msg.config.u.data[index++];
                        toneQuadParam_ptr->tMake2 = 
                                cmd_ptr->msg.config.u.data[index++];
                        toneQuadParam_ptr->tBreak2 =     
                                cmd_ptr->msg.config.u.data[index++];
                        toneQuadParam_ptr->tRepeat2 = 
                                cmd_ptr->msg.config.u.data[index++];
                        toneQuadParam_ptr->tMake3 = 
                                cmd_ptr->msg.config.u.data[index++];
                        toneQuadParam_ptr->tBreak3 = 
                                cmd_ptr->msg.config.u.data[index++];
                        toneQuadParam_ptr->tRepeat3 = 
                                cmd_ptr->msg.config.u.data[index++];
                        toneQuadParam_ptr->sRepeat = 1;
                        toneQuadParam_ptr->deltaF = 
                                cmd_ptr->msg.config.u.data[index++];
                        toneQuadParam_ptr->decay = 
                                cmd_ptr->msg.config.u.data[index++];
                    }
                    else {
                        _VTSP_TRACE(__FILE__, __LINE__);
                        _VTSP_TRACE("tQuadId", tId);
                    }
#endif
                    break;
#ifdef VTSP_ENABLE_DTMF
                case VTSP_TEMPL_CODE_DTMF:

                    dtmfParams_ptr = dsp_ptr->dtmfParams_ptr;
                    switch (cmd_ptr->msg.config.u.data[0]) {
                        case VTSP_TEMPL_CONTROL_DTMF_POWER:
                            dtmfParams_ptr->pDTMFPWR = 
                                    cmd_ptr->msg.config.u.data[1];
                            break;
                        case VTSP_TEMPL_CONTROL_DTMF_DURATION:
                            dtmfParams_ptr->tDTMFDUR = 
                                    cmd_ptr->msg.config.u.data[1];
                            break;
                        case VTSP_TEMPL_CONTROL_DTMF_SILENCE:
                            dtmfParams_ptr->tDTMFSIL = 
                                    cmd_ptr->msg.config.u.data[1];
                            break;
                        case VTSP_TEMPL_CONTROL_DTMF_HI_TWIST:
                            dtmfParams_ptr->pDTMFHITWIST = 
                                    cmd_ptr->msg.config.u.data[1];
                            break;
                        case VTSP_TEMPL_CONTROL_DTMF_LO_TWIST:
                            dtmfParams_ptr->pDTMFLOTWIST = 
                                    cmd_ptr->msg.config.u.data[1];
                            break;
                        case VTSP_TEMPL_CONTROL_DTMF_FRE_DEV:
                            dtmfParams_ptr->fDTMFFDEV = 
                                    cmd_ptr->msg.config.u.data[1];
                            break;
                        case VTSP_TEMPL_CONTROL_DTMF_ERR_FRAMES:
                            dtmfParams_ptr->tDTMFEFRAMES = 
                                    cmd_ptr->msg.config.u.data[1];
                            break;                    
                    }
                    break;
#endif
                case VTSP_TEMPL_CODE_DEBUG:
                    switch (cmd_ptr->msg.config.u.data[0]) {
                        case VTSP_TEMPL_CONTROL_DEBUG_PCMDUMP:
#ifdef VTSP_ENABLE_NETLOG
                            if (1 == cmd_ptr->msg.config.u.data[1]) {
                                vtspr_ptr->netlog_ptr->enable |=
                                    (1 << infc);
                            }
                            else {
                                vtspr_ptr->netlog_ptr->enable &=
                                    (0 << infc);
                            }
                            vtspr_ptr->netlog_ptr->remoteIP =
                                     cmd_ptr->msg.config.u.data32[1];
#endif
                            break;
                        case VTSP_TEMPL_CONTROL_DEBUG_TRACE:
                            /* Do trace stuff */
                            break;
                    }
                    break;

                default:
                    _VTSP_TRACE(__FILE__, __LINE__);
                    /* XXX */
                    break;
            }
            break;
        case _VTSP_CMD_QUERY:
            /* XXX */
            _VTSP_TRACE(__FILE__, __LINE__);
            break;
        case _VTSP_CMD_START:
            vtspr_ptr->task10ms.taskEnable &= ~(VTSPR_TASK_WAIT);
            vtspr_ptr->task10ms.taskEnable |= VTSPR_TASK_RUN;
            dsp_ptr->heartbeat = VTSP_HEARTBEAT_EVENT_COUNT - 1;
            dsp_ptr->heartbeat1s = 100 - 1;

            /*
             * Start FXS.
             * From TIC_init(), FXS is in POWER_DOWN + RELAY FXS<->FXO.
             */
            _VTSPR_FOR_ALL_FXS(infc) {
                tic_ptr = _VTSPR_infcToTicPtr(dsp_ptr, infc);
                /* Set line state active */
                TIC_control(tic_ptr, TIC_CONTROL_LINE_STATE, TIC_STATE_ACTIVE);
            }
            /*
             * Start FXO.
             * From TIC_init(), FXO is in RELEASE + POWER_DOWN.
             */
            _VTSPR_FOR_ALL_FXO(infc) {
                tic_ptr = _VTSPR_infcToTicPtr(dsp_ptr, infc);
                TIC_control(tic_ptr, TIC_CONTROL_ONHOOK, 0);
            }
            break;
        case _VTSP_CMD_GET_VERSION:
            _VTSP_TRACE(__FILE__, __LINE__);
            /* Version Labels have EOL at the end of string */
            OSAL_logMsg("%s", D2_Release_VTSP_RT);
#ifdef VTSP_ENABLE_ECSR           
           OSAL_logMsg("%s", D2_ComponentReleaseVersion_ecsr);
           OSAL_logMsg("%s", D2_ComponentReleaseVersion_lms);
#endif
           OSAL_logMsg("%s", D2_ComponentReleaseVersion_comm);
#ifndef VTSP_ENABLE_MP_LITE
           OSAL_logMsg("%s", D2_ComponentReleaseVersion_dcrm);
#endif
#ifdef VTSP_ENABLE_TONE_QUAD
           OSAL_logMsg("%s", D2_ComponentReleaseVersion_genf);
#endif
#ifdef VTSP_ENABLE_FMTD
           OSAL_logMsg("%s", D2_ComponentReleaseVersion_fmtd);
#endif
#ifdef VTSP_ENABLE_DTMF
           OSAL_logMsg("%s", D2_ComponentReleaseVersion_dtmf);
#endif
#ifdef VTSP_ENABLE_NFE
           OSAL_logMsg("%s", D2_ComponentReleaseVersion_nfe);
#else
#ifndef VTSP_ENABLE_MP_LITE
           OSAL_logMsg("%s", D2_ComponentReleaseVersion_bnd);
#endif
#endif
#ifdef VTSP_ENABLE_AEC
           OSAL_logMsg("%s", D2_ComponentReleaseVersion_aec);
#endif
#ifndef VTSP_ENABLE_MP_LITE
           OSAL_logMsg("%s", D2_ComponentReleaseVersion_nse);
           OSAL_logMsg("%s", D2_ComponentReleaseVersion_plc);
           OSAL_logMsg("%s", D2_ComponentReleaseVersion_tone);
#endif
           OSAL_logMsg("%s", D2_ComponentReleaseVersion_jb);
#ifdef VTSP_ENABLE_CIDS
           OSAL_logMsg("%s", D2_ComponentReleaseVersion_fsks);
#endif            
#ifdef VTSP_ENABLE_CIDR
           OSAL_logMsg("%s", D2_ComponentReleaseVersion_fskr);
#endif  
#if defined(VTSP_ENABLE_G726) && !defined(VTSP_ENABLE_G726_ACCELERATOR)
           OSAL_logMsg("%s", D2_ComponentReleaseVersion_g726);
#endif
#if defined(VTSP_ENABLE_G729) && !defined(VTSP_ENABLE_G729_ACCELERATOR)
           OSAL_logMsg("%s", D2_ComponentReleaseVersion_g729ab);
#endif
#ifdef VTSP_ENABLE_ILBC
           OSAL_logMsg("%s", D2_ComponentReleaseVersion_ilbc);
#endif
#ifdef VTSP_ENABLE_G723
           OSAL_logMsg("%s", D2_ComponentReleaseVersion_g723a);
#endif
#ifdef VTSP_ENABLE_G722
           OSAL_logMsg("%s", D2_ComponentReleaseVersion_g722);
#endif
#ifdef VTSP_ENABLE_G722P1
           OSAL_logMsg("%s", D2_ComponentReleaseVersion_g722p1);
#endif
#ifdef VTSP_ENABLE_SILK
           OSAL_logMsg("%s", D2_ComponentReleaseVersion_silk);
#endif
#ifdef VTSP_ENABLE_GAMRNB
           OSAL_logMsg("%s", D2_ComponentReleaseVersion_gamrnb);
#endif
#ifdef VTSP_ENABLE_GAMRWB
           OSAL_logMsg("%s", D2_ComponentReleaseVersion_gamrwb);
#endif
#ifdef VTSPENABLE_T38
           OSAL_logMsg("%s", D2_ComponentReleaseVersion_fr38v3);
#endif
#ifdef VTSP_ENABLE_STREAM_16K
#ifndef VTSP_ENABLE_MP_LITE
           OSAL_logMsg("%s", D2_ComponentReleaseVersion_uds);
#endif
#endif                
            break;
        case _VTSP_CMD_GET_INFO:
            /* XXX */
            _VTSP_TRACE(__FILE__, __LINE__);
            break;
        case _VTSP_CMD_SHUTDOWN:
            /*
             * Powerdown FXS
             */
            _VTSPR_FOR_ALL_FX(infc) {
                tic_ptr = _VTSPR_infcToTicPtr(dsp_ptr, infc);
                TIC_control(tic_ptr, TIC_CONTROL_LINE_STATE,
                        TIC_STATE_POWER_DOWN);
            }
            /*
             * Set task loop to exit
             */
            VTSPR_shutdown();
            break;
        case _VTSP_CMD_STREAM_MODIFY:
            streamId = cmd_ptr->msg.stream.streamId;
            stream_ptr = _VTSPR_streamIdToStreamPtr(dsp_ptr, infc, streamId);

            if (_VTSP_STREAM_DIR_ENDED == stream_ptr->streamParam.dir) {
                /* Stream must be started in order to modify */
                break;
            }

            /*
             * If the stream direction changes from non-receive to receive, the
             * jitter buffer needs to reinitialize.
             */
            if ((VTSP_STREAM_DIR_INACTIVE == stream_ptr->streamParam.dir) ||
                    (VTSP_STREAM_DIR_SENDONLY == stream_ptr->streamParam.dir)) {
                _VTSPR_algStateStream(dsp_ptr, infc,
                        cmd_ptr->msg.stream.streamId, 0, VTSPR_ALG_STREAM_JB);
            }

#ifdef VTSP_ENABLE_DTMFR
            /*
             * If MODIFY turns send off, need to make sure to turn off
             * active DTMF relay encoding on this stream.
             */
            if ((VTSP_STREAM_DIR_INACTIVE == cmd_ptr->msg.stream.dir) ||
                    (VTSP_STREAM_DIR_RECVONLY == cmd_ptr->msg.stream.dir)) {
                stream_ptr->drEncodeObj.status &= ~(DR_IN_DIGIT);
            }

            /*
             * If MODIFY turns recv off, need to make sure to turn off
             * active DTMF relay decoding on this stream by setting the
             * decoder type to the previous coder used before receiving DTMF
             * relay packets.
             */
            if (((VTSP_STREAM_DIR_INACTIVE == cmd_ptr->msg.stream.dir) ||
                    (VTSP_STREAM_DIR_SENDONLY == cmd_ptr->msg.stream.dir)) &&
                    (VTSP_CODER_DTMF == stream_ptr->decoderType)) {
                stream_ptr->decoderType = stream_ptr->drEncodeObj.prevCoder;
                stream_ptr->drDecodeObj_ptr->status &= ~(DR_IN_DIGIT);
            }
#endif
            /*
             * Update internal stream data
             */
            _VTSPR_updateStream(dsp_ptr, infc,
                    stream_ptr, &cmd_ptr->msg.stream);

            /*
             * Update the ports used by RTP and RTCP.
             */
            rtp_ptr = _VTSPR_streamIdToRtpPtr(vtspr_ptr->net_ptr, infc,
                    streamId);
            _VTSPR_rtpOpen(rtp_ptr, stream_ptr->streamParam.dir,
                    stream_ptr->streamParam.remoteAddr,
                    stream_ptr->streamParam.localAddr,
                    stream_ptr->streamParam.rdnDynType,
                    stream_ptr->streamParam.srtpSecurityType,
                    stream_ptr->streamParam.srtpSendKey,
                    stream_ptr->streamParam.srtpRecvKey);

            rtcp_ptr = _VTSPR_streamIdToRtcpPtr(vtspr_ptr->net_ptr, infc,
                    streamId);

            OSAL_memCpy(&localRtcpAddr, &stream_ptr->streamParam.localAddr,
                        sizeof(localRtcpAddr));
            localRtcpAddr.port = stream_ptr->streamParam.localControlPort;
            OSAL_memCpy(&remoteRtcpAddr, &stream_ptr->streamParam.remoteAddr,
                        sizeof(remoteRtcpAddr));
            remoteRtcpAddr.port = stream_ptr->streamParam.remoteControlPort;
            _VTSPR_rtcpOpen(q_ptr, rtcp_ptr, remoteRtcpAddr, localRtcpAddr);
            break;

        case _VTSP_CMD_STREAM_START:
            streamId = cmd_ptr->msg.stream.streamId;
            stream_ptr = _VTSPR_streamIdToStreamPtr(dsp_ptr, infc, streamId);

            /* Check active stream/interface limit */
            if (VTSP_OK != _VTSPR_activeInfcStreamFilterStart(dsp_ptr, streamId, infc, q_ptr)) {
                break;
            }
            
            /*
             * Set coder type to unavailable to force initialization when the
             * stream is started.
             */
            stream_ptr->decoderType = VTSP_CODER_UNAVAIL;
            stream_ptr->lastEncoder = VTSP_CODER_UNAVAIL;
            stream_ptr->marker = 0;

            /*
             * Clear the stream count stats 
             */
            stream_ptr->count.encodeBytes = 0;
            stream_ptr->count.decodeBytes = 0;
            stream_ptr->count.encodePkt = 0;
            stream_ptr->count.decodePkt = 0;
            stream_ptr->count.encodeCN = 0;
            stream_ptr->count.decodeCN = 0;
            stream_ptr->count.runNSE = 0;
            stream_ptr->count.runPLC = 0;

            /*
             * Update internal stream data
             */
            _VTSPR_updateStream(dsp_ptr, infc,
                    stream_ptr, &cmd_ptr->msg.stream);

            /*
             * Initialize the stream for sending RTP packets over network
             * sockets.
             */
            rtp_ptr = _VTSPR_streamIdToRtpPtr(vtspr_ptr->net_ptr, infc,
                    streamId);
            errval = _VTSPR_rtpOpen(rtp_ptr, stream_ptr->streamParam.dir,
                    stream_ptr->streamParam.remoteAddr,
                    stream_ptr->streamParam.localAddr,
                    stream_ptr->streamParam.rdnDynType,
                    stream_ptr->streamParam.srtpSecurityType,
                    stream_ptr->streamParam.srtpSendKey,
                    stream_ptr->streamParam.srtpRecvKey);

            if (_VTSPR_RTP_ERROR == errval) {
                _VTSP_TRACE(__FILE__, __LINE__);
                _VTSP_TRACE("RTP Error infc", infc);
                _VTSP_TRACE("RTP Error remoteDataPort",
                        OSAL_netNtohs(stream_ptr->streamParam.remoteAddr.port));
            }
            /*
             * Initialize the socket for send and receiving RTCP packets.
             */
            rtcp_ptr = _VTSPR_streamIdToRtcpPtr(vtspr_ptr->net_ptr, infc,
                    streamId);
            OSAL_memCpy(&localRtcpAddr, &stream_ptr->streamParam.localAddr,
                        sizeof(localRtcpAddr));
            localRtcpAddr.port = stream_ptr->streamParam.localControlPort;
            OSAL_memCpy(&remoteRtcpAddr, &stream_ptr->streamParam.remoteAddr,
                        sizeof(remoteRtcpAddr));
            remoteRtcpAddr.port = stream_ptr->streamParam.remoteControlPort;
            _VTSPR_rtcpOpen(q_ptr, rtcp_ptr, remoteRtcpAddr, localRtcpAddr);

            break;
        case _VTSP_CMD_STREAM_END:
            
            streamId = cmd_ptr->msg.stream.streamId;
            stream_ptr = _VTSPR_streamIdToStreamPtr(dsp_ptr, infc, streamId);
 
            /*
             * XXX the flowId is assumed to be indentical with the
             * streamId. This may change in the future.
             */
            flow_ptr = _VTSPR_streamIdToFlowPtr(dsp_ptr, infc, streamId);
            
            /* Set stream direction to End. */
            stream_ptr->streamParam.dir = _VTSP_STREAM_DIR_ENDED;

            /*
             * Set coder type to unavailable to force initialization when the
             * stream is started.
             */
            stream_ptr->decoderType = VTSP_CODER_UNAVAIL;
            stream_ptr->lastEncoder = VTSP_CODER_UNAVAIL;

            /*
             * Turn algs off.
             */
            if (VTSP_CODER_T38 == stream_ptr->streamParam.encoder) {
                /* Turn off T38 and FAX on the channel */
                _VTSPR_algStateChan(dsp_ptr, infc,
                                    VTSPR_ALG_CHAN_T38 | VTSPR_ALG_CHAN_FAX, 0);
            }
            /* Turn off all stream algs */
            _VTSPR_algStateStream(dsp_ptr, infc,
                    cmd_ptr->msg.stream.streamId,
                    VTSPR_ALG_STREAM_JB |
#ifndef VTSP_ENABLE_MP_LITE
                    VTSPR_ALG_STREAM_PLC |
#endif
                    VTSPR_ALG_STREAM_T38,
                    0);
            /* 
             * Turn off DR encode if digit in progress.  This will not
             * send END pkts.
             */
#if defined(VTSP_ENABLE_DTMFR) && !defined(VTSP_ENABLE_MP_LITE)
            DR_encodeInit(&stream_ptr->drEncodeObj);
#endif

            rtp_ptr = _VTSPR_streamIdToRtpPtr(vtspr_ptr->net_ptr, infc,
                    streamId);
            _VTSPR_rtpClose(rtp_ptr);
            _VTSPR_rtcpClose(q_ptr, vtspr_ptr->net_ptr, infc,
                    cmd_ptr->msg.stream.streamId);

            /*
             * In the case where the stream is associated with a flow, a closing
             * of a stream associated with a peer flow closes aborts that flow.
             */
            if (_VTSPR_FLOW_STATE_IDLE != flow_ptr->playState) {
                /*
                 * If a play peer flow is active clean up and close the flow.
                 */
                if (0 != (VTSP_FLOW_DIR_PEER_PLAY & flow_ptr->flowDir)) {
                    _VTSPR_flowAbortFlush(dsp_ptr, flow_ptr);
                    flow_ptr->playState = _VTSPR_FLOW_STATE_IDLE;
                    /*
                     * Send an event to the application that the
                     * flow was aborted.
                     */
                    q_ptr->eventMsg.code = VTSP_EVENT_MSG_CODE_FLOW;
                    q_ptr->eventMsg.infc = flow_ptr->infc;
                    q_ptr->eventMsg.tick = dsp_ptr->tick1ms;
                    q_ptr->eventMsg.msg.flow.reason = VTSP_EVENT_HALTED;
                    q_ptr->eventMsg.msg.flow.flowId = flow_ptr->flowId;
                    q_ptr->eventMsg.msg.flow.key = flow_ptr->key;

                    VTSPR_sendEvent(q_ptr, &q_ptr->eventMsg, flow_ptr->infc);
                }
            }
            if (_VTSPR_FLOW_STATE_IDLE == flow_ptr->recState) {
            /* XXX need anything here XXX */
            }
            /* Restore JB Parameters to stream defaults for next stream */
            OSAL_memCpy(&stream_ptr->jbParams, dsp_ptr->jbDefParams_ptr, 
                    sizeof(stream_ptr->jbParams));                    
            
            /* Update active stream/interface counts */
            _VTSPR_activeInfcStreamFilterEnd(dsp_ptr, streamId, infc, q_ptr);

            break;
        case _VTSP_CMD_STREAM_MODIFY_DIR:
            streamId = cmd_ptr->msg.arg.arg0;
            stream_ptr = _VTSPR_streamIdToStreamPtr(dsp_ptr, infc, streamId);

            /*
             * If the stream direction changes from non-receive to receive, the
             * jitter buffer needs to reinitialize. Based on old state of dir.
             */
            if ((VTSP_STREAM_DIR_INACTIVE == stream_ptr->streamParam.dir) ||
                    (VTSP_STREAM_DIR_SENDONLY == stream_ptr->streamParam.dir)) {
                _VTSPR_algStateStream(dsp_ptr, infc,
                        cmd_ptr->msg.stream.streamId, 0, VTSPR_ALG_STREAM_JB);
            }

            /*
             * Update direction by copying the current stream to the msg, then
             * updating the direction only and calling the main update
             * function.  This allows all changes to be made.
             */
            dir = cmd_ptr->msg.arg.arg1;
            OSAL_memCpy(&cmd_ptr->msg.stream, &stream_ptr->streamParam,
                    sizeof(VTSP_Stream));
            cmd_ptr->msg.stream.dir = (VTSP_StreamDir)dir;
            _VTSPR_updateStream(dsp_ptr, infc,
                    stream_ptr, &cmd_ptr->msg.stream);

#ifdef VTSP_ENABLE_DTMFR
            /*
             * If MODIFY_DIR turns send off, need to make sure to turn off
             * active DTMF relay encoding on this stream.
             */
            if ((VTSP_STREAM_DIR_INACTIVE == cmd_ptr->msg.arg.arg1) ||
                    (VTSP_STREAM_DIR_SENDONLY == cmd_ptr->msg.arg.arg1)) {
                stream_ptr->drEncodeObj.status &= ~(DR_IN_DIGIT);
            }

            /*
             * If MODIFY_DIR turns recv off, need to make sure to turn off
             * active DTMF relay decoding on this stream by setting the
             * decoder type to the previous coder used before receiving DTMF
             * relay packets.
             */
            if ((VTSP_STREAM_DIR_INACTIVE == cmd_ptr->msg.arg.arg1) ||
                    (VTSP_STREAM_DIR_RECVONLY == cmd_ptr->msg.arg.arg1)) {
                stream_ptr->decoderType = stream_ptr->drEncodeObj.prevCoder;
                stream_ptr->drDecodeObj_ptr->status &= ~(DR_IN_DIGIT);
            }
#endif
            /*
             * Tell RTP that direction has changed.
             */
            rtp_ptr = _VTSPR_streamIdToRtpPtr(vtspr_ptr->net_ptr, infc,
                    streamId);
            _VTSPR_rtpDir(rtp_ptr, cmd_ptr->msg.arg.arg1);
            break;
        case _VTSP_CMD_STREAM_MODIFY_ENCODER:
            /*
             * XXX warning - only change encoder if outside DR in-digit
             * state.  If inside DR in-digit state, change prevCoder
             * instead.
             */
            streamId = cmd_ptr->msg.arg.arg0;
            stream_ptr = _VTSPR_streamIdToStreamPtr(dsp_ptr, infc, streamId);
            coder = cmd_ptr->msg.arg.arg1;
#ifdef VTSP_ENABLE_DTMFR
            if ((0 != (stream_ptr->drEncodeObj.status & DR_IN_DIGIT))) {
                /*
                 * DR is active - do not change coder, only change
                 * prev coder
                 */
                stream_ptr->drEncodeObj.prevCoder = coder;
            }
            else {
                /*
                 * DR not active - nothing special needs to be done
                 */
                stream_ptr->streamParam.encoder = coder;
            }
#else
            /* No DTMF Relay */
            stream_ptr->streamParam.encoder = coder;
#endif
            break;
#ifdef VTSP_ENABLE_DTMFR
        case _VTSP_CMD_STREAM_RFC4733_PEER:   
            /*
             * Start DTMF relay encoding
             * implemented in Bug 2564
             */
            streamId = cmd_ptr->msg.rfc4733.data[0];
            stream_ptr = _VTSPR_streamIdToStreamPtr(dsp_ptr, infc, streamId);
            chan_ptr = _VTSPR_infcToChanPtr(dsp_ptr, infc);

            /*
             * Check the tone duration
             */
            if (0 != cmd_ptr->msg.rfc4733.duration) {
                /*
                 * Start new DTMF relay
                 * Do not call DR_encodeInit() so that END packets are generated
                 * for any current IN_DIGIT.
                 */
                chan_ptr->drEventObj.event |= DR_EVENT_LE;
                /* Check the playing time by telephone-event sample rate */
                if (0 != (stream_ptr->streamParam.extension &
                        VTSP_MASK_EXT_DTMFR_16K)) {
                    chan_ptr->drEventObj.newPlayTime = (uint32)
                            ((160 * cmd_ptr->msg.rfc4733.duration) / 10);
                }
                else {
                    chan_ptr->drEventObj.newPlayTime = (uint32)
                            ((80 * cmd_ptr->msg.rfc4733.duration) / 10);
                }

                chan_ptr->drEventObj.newPower = 
                        (vint)cmd_ptr->msg.rfc4733.data[1];
                stream_ptr->drEncodeObj.redundancy =
                        (uvint)cmd_ptr->msg.rfc4733.data[2];

                if (VTSP_RFC4733_TYPE_EVENT == cmd_ptr->msg.rfc4733.type) {
                    /* Telephony Event */
                    chan_ptr->drEventObj.relayMode = DR_RELAY_MODE_EVENT;
                    chan_ptr->drEventObj.newDigit = 
                            (uvint)cmd_ptr->msg.rfc4733.data[3];
                }
                else {
                    /* Telephone Tone Params */
                    chan_ptr->drEventObj.relayMode = DR_RELAY_MODE_TONE;
                    stream_ptr->drEncodeObj.modulation =
                        (uvint)cmd_ptr->msg.rfc4733.data[3];
                    stream_ptr->drEncodeObj.freq1 = 
                            (uvint)cmd_ptr->msg.rfc4733.data[4];
                    stream_ptr->drEncodeObj.freq2 = 
                            (uvint)cmd_ptr->msg.rfc4733.data[5];
                    stream_ptr->drEncodeObj.freq3 = 
                            (uvint)cmd_ptr->msg.rfc4733.data[6];
                    stream_ptr->drEncodeObj.freq4 = 
                            (uvint)cmd_ptr->msg.rfc4733.data[7];
                    stream_ptr->drEncodeObj.divMod3 = 
                            (uvint)cmd_ptr->msg.rfc4733.data[8];
                }
            }
            else {
                /*
                 * Stop any currently active DTMF relay
                 */
                chan_ptr->drEventObj.stop = DR_STOP_SET;
            }
            break;
#endif
        case _VTSP_CMD_STREAM_MODIFY_CONFMASK:
            streamId = cmd_ptr->msg.arg.arg0;
            stream_ptr = _VTSPR_streamIdToStreamPtr(dsp_ptr, infc, streamId);
            stream_ptr->streamParam.confMask = cmd_ptr->msg.arg.arg1;
           break;
        case _VTSP_CMD_STREAM_QUERY:
            /* 
             * Generate event msgs for application RTCP pkts.
             */
            _VTSPR_genEventStreamQuery(vtspr_ptr, q_ptr, dsp_ptr, 
                    infc, cmd_ptr->msg.arg.arg0);
            break;
        case _VTSP_CMD_FLOW_OPEN:
            /*
             * Open the flow.
             *
             * The flow takes control of the stream's encoder and decoder.
             * There is no need to save the state of the stream. When the flow
             * is terminated, it uses the current values in the stream object
             * to restore functionality. Using this method, the stream can be
             * modified without affecting the flow.
             */
            streamId   = cmd_ptr->msg.flowOpen.streamId;
            flow_ptr   = _VTSPR_streamIdToFlowPtr(dsp_ptr, infc, streamId);

            flow_ptr->infc     = cmd_ptr->infc;
            flow_ptr->flowId   = cmd_ptr->msg.flowOpen.flowId;
            flow_ptr->streamId = cmd_ptr->msg.flowOpen.streamId;
            flow_ptr->flowDir  = cmd_ptr->msg.flowOpen.flowDir;
            flow_ptr->key      = cmd_ptr->msg.flowOpen.key;

            if ((0 != (VTSP_FLOW_DIR_LOCAL_PLAY & flow_ptr->flowDir)) ||
                    (0 != (VTSP_FLOW_DIR_PEER_PLAY & flow_ptr->flowDir))) {
                if (((vint)flow_ptr->playPayloadIndex != -1) ||
                        (flow_ptr->playMsg.key == flow_ptr->key)) {
                    stream_ptr = _VTSPR_streamIdToStreamPtr(dsp_ptr, infc,
                            streamId);
                    _VTSPR_flowOpenFlush(flow_ptr, stream_ptr);
                }
                /*
                 * Set coder to an illegal type to force initialization.
                 */
                flow_ptr->playControl = 0;
                flow_ptr->playLastCoder = VTSP_CODER_UNAVAIL;
                flow_ptr->playDuration = 0;
                flow_ptr->playState = _VTSPR_FLOW_STATE_ACTIVE;
            }
            if ((0 != (VTSP_FLOW_DIR_LOCAL_RECORD & flow_ptr->flowDir)) ||
                    (0 != (VTSP_FLOW_DIR_PEER_RECORD == flow_ptr->flowDir))) {
                /*
                 * Set coder to an illegal type to force initialization.
                 */
                flow_ptr->recCoder = cmd_ptr->msg.flowOpen.coder;
                flow_ptr->recLastCoder = VTSP_CODER_UNAVAIL;
                flow_ptr->recPayloadIndex = 0;
                flow_ptr->recControl = 0;
                flow_ptr->recDuration = 0;
                flow_ptr->recState = _VTSPR_FLOW_STATE_ACTIVE;
            }
            break;
        case _VTSP_CMD_FLOW_CLOSE:
            /*
             * Close the flow if OPEN.
             */
            streamId   = cmd_ptr->msg.flowOpen.streamId;
            flow_ptr   = _VTSPR_streamIdToFlowPtr(dsp_ptr, infc, streamId);

            if (flow_ptr->playState ==
                    _VTSPR_FLOW_STATE_ACTIVE) {
                flow_ptr->playState = _VTSPR_FLOW_STATE_CLOSING;
            }
            if (flow_ptr->recState ==
                    _VTSPR_FLOW_STATE_ACTIVE) {
                stream_ptr = _VTSPR_streamIdToStreamPtr(dsp_ptr, flow_ptr->infc,
                        flow_ptr->streamId);
                _VTSPR_flowAbortRec(vtspr_ptr, q_ptr, flow_ptr, stream_ptr);
                flow_ptr->recState = _VTSPR_FLOW_STATE_IDLE;
            }
            break;
        case _VTSP_CMD_FLOW_ABORT:
            /*
             * Abort the flow. Only close it if it is ACTIVE or CLOSE. Aborting
             * while in the close state forces an immediate close versus waiting
             * for all data to play.
             */
            streamId   = cmd_ptr->msg.flowOpen.streamId;
            flow_ptr   = _VTSPR_streamIdToFlowPtr(dsp_ptr, infc, streamId);

            if ((_VTSPR_FLOW_STATE_ACTIVE == flow_ptr->playState) ||
                    (_VTSPR_FLOW_STATE_CLOSING == flow_ptr->playState)) {
                /*
                 * If a play flow is active clean up and close the flow.
                 */
                if ((0 != (VTSP_FLOW_DIR_LOCAL_PLAY & flow_ptr->flowDir)) ||
                        (0 != (VTSP_FLOW_DIR_PEER_PLAY & flow_ptr->flowDir))) {
                    _VTSPR_flowAbortFlush(dsp_ptr, flow_ptr);
                }
            }
            if ((_VTSPR_FLOW_STATE_ACTIVE == flow_ptr->recState) ||
                    (_VTSPR_FLOW_STATE_CLOSING == flow_ptr->recState)) {
                /*
                 * If a record flow is active clean up and close the flow.
                 */
                if ((0 != (VTSP_FLOW_DIR_LOCAL_RECORD & flow_ptr->flowDir)) ||
                       (0 != (VTSP_FLOW_DIR_PEER_RECORD & flow_ptr->flowDir))) {
                    stream_ptr = _VTSPR_streamIdToStreamPtr(dsp_ptr,
                            flow_ptr->infc, flow_ptr->streamId);
                    _VTSPR_flowAbortRec(vtspr_ptr, q_ptr, flow_ptr, stream_ptr);
                }
            }

            /*
             * If either stream was not IDLE, send an event to the application
             * that the flow was aborted.
             */
            if ((_VTSPR_FLOW_STATE_IDLE != flow_ptr->recState) ||
                    (_VTSPR_FLOW_STATE_IDLE != flow_ptr->playState)) {
                q_ptr->eventMsg.code = VTSP_EVENT_MSG_CODE_FLOW;
                q_ptr->eventMsg.infc = flow_ptr->infc;
                q_ptr->eventMsg.tick = dsp_ptr->tick1ms;
                q_ptr->eventMsg.msg.flow.reason = VTSP_EVENT_HALTED;
                q_ptr->eventMsg.msg.flow.flowId = flow_ptr->flowId;
                q_ptr->eventMsg.msg.flow.key = flow_ptr->key;

                VTSPR_sendEvent(q_ptr, &q_ptr->eventMsg, flow_ptr->infc);
            }

            flow_ptr->recState = _VTSPR_FLOW_STATE_IDLE;
            flow_ptr->playState = _VTSPR_FLOW_STATE_IDLE;
            break;
        case _VTSP_CMD_RTCP_CNAME:
            /*
             * Update the CNAME structure with the new string.
             */
            _VTSPR_rtcpSetCname(vtspr_ptr->net_ptr, infc, (const char *)
                    (cmd_ptr->msg.cname.cname));
            break;
    }
}

