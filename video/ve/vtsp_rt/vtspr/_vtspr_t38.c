/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004-2010 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 28394 $ $Date: 2014-08-21 10:24:05 +0800 (Thu, 21 Aug 2014) $
 *
 */
#include "vtspr.h"
#include "_vtspr_private.h"

#ifdef VTSP_ENABLE_T38

/*
 * ======== _VTSPR_processT38() ========
 *
 * Process t.38 encode/decode
 * Run:
 * FR38
 */
vint _VTSPR_processT38(
    VTSPR_Obj    *vtspr_ptr,
    VTSPR_DSP    *dsp_ptr)
{
#ifdef VTSP_ENABLE_T38
    VTSPR_ChanObj    *chan_ptr;
    VTSPR_StreamObj  *stream_ptr;
    _VTSPR_RtpObject *rtp_ptr;
    _VTSPR_FR38Obj   *fr38_ptr;
    vint              infc;
    uint32            mask;
    vint              retVal;
    uint32            streamMask;
    vint              streamId;
    int16             faxIn_ary[VTSPR_NSAMPLES_10MS_8K];
    int16             faxOut_ary[VTSPR_NSAMPLES_10MS_8K];
    vint              samp;
    vint              e;
    vint              pktSz;
    vint              pktWordSz;
    uint16           *tmp_ptr;

    e = VTSP_OK;

    _VTSPR_FOR_ALL_FX(infc) { /* Do for all physical infcs. */

        e = VTSP_OK;

        chan_ptr = _VTSPR_infcToChanPtr(dsp_ptr, infc);
        mask    = chan_ptr->algChannelState;
        if (0 != (VTSPR_ALG_CHAN_T38 & mask)) {
            fr38_ptr = chan_ptr->fr38_ptr;
            for (streamId = _VTSP_STREAM_PER_INFC - 1; streamId >= 0;
                    streamId--) {
                stream_ptr = _VTSPR_streamIdToStreamPtr(dsp_ptr, infc,
                        streamId);
                streamMask = stream_ptr->algStreamState;
                if (0 != (VTSPR_ALG_STREAM_T38 & streamMask)) {
                    /*
                     * Since currently FR38 takes in 16-bit we have to convert
                     * audioInary that is a vint to faxIn_ary that is int16.
                     */
                    stream_ptr->fr38Obj_ptr->src_ptr = (uint16 *)faxIn_ary;
                    stream_ptr->fr38Obj_ptr->dst_ptr = (uint16 *)faxOut_ary;
    
                    for (samp = 0; samp < VTSPR_NSAMPLES_10MS_8K; samp++) {
                        faxIn_ary[samp] = (int16)chan_ptr->audioIn_ary[samp];
                    }
                        
                    retVal = FR38_run(stream_ptr->fr38Obj_ptr, 0);

                    /* Send T38 event to application */
                    switch(retVal) {
                        case FR38_DISCONNECT:
                            fr38_ptr->fr38Event = VTSP_EVENT_T38_DISCON;
                            break;
                        case FR38_NOT_FAX:
                            fr38_ptr->fr38Event = VTSP_EVENT_T38_NOT_FAX;
                            break;
                    }

                    for (samp = 0; samp < VTSPR_NSAMPLES_10MS_8K; samp++) {
                        chan_ptr->audioOut_ary[samp] = (vint)(stream_ptr->fr38Obj_ptr->dst_ptr[samp]);
                    }
                    
                    pktSz =
                      stream_ptr->fr38Obj_ptr->fax_packetOut_ptr->numBits >> 3; 

                    rtp_ptr = _VTSPR_streamIdToRtpPtr(vtspr_ptr->net_ptr,
                            infc, streamId);

                    if (FR38_PACKET_READY == retVal) {
                        if (pktSz > FR38_MAX_T38_PACKET || pktSz <= 0) { 
                            /* _VTSP_TRACE(__FILE__, __LINE__); */
                            /* _VTSP_TRACE("not sending t38, pktSz", pktSz); */
                        }
                        else { 
                            tmp_ptr = (uint16*)stream_ptr->fr38Obj_ptr->fax_packetOut_ptr->data;
                            for (pktWordSz = (pktSz+1) >> 1; pktWordSz > 0; pktWordSz--) {
                                *tmp_ptr = OSAL_netHtons(*tmp_ptr);
                                tmp_ptr++;
                            }
                            retVal = _VTSPR_netSendto(rtp_ptr->socket,
                                 stream_ptr->fr38Obj_ptr->fax_packetOut_ptr->data, 
                                 pktSz, rtp_ptr->remoteAddr);
                            if (retVal != pktSz) {
                                _VTSP_TRACE(__FILE__, __LINE__);
                                _VTSP_TRACE(__FILE__, retVal);
                                e = _VTSPR_RTP_ERROR;
                            }
                        }
                    }
                }
            }
        }
    }
    return (e);
#else
    return (VTSP_OK);
#endif
}


/*
 * ======== _VTSPR_processT38EventLog() ========
 *
 * Process t.38 status every 10ms
 * Run:
 */
void _VTSPR_processT38EventLog(
        vint          infc,
        VTSPR_Obj    *vtspr_ptr,
        VTSPR_Queues *q_ptr,
        VTSPR_DSP    *dsp_ptr)
{
    vint              index;
    vint              streamId;
    uint32            streamMask;
    VTSPR_StreamObj  *stream_ptr;
    FR38_Obj         *fr38Obj_ptr;

    for (streamId = _VTSP_STREAM_PER_INFC - 1; streamId >= 0; streamId--) { 
        stream_ptr = _VTSPR_streamIdToStreamPtr(dsp_ptr, infc, streamId);
  
        streamMask = stream_ptr->algStreamState;
        if (0 != (VTSPR_ALG_STREAM_T38 & streamMask)) {
            continue;
        }
        fr38Obj_ptr = stream_ptr->fr38Obj_ptr;
        q_ptr->eventMsg.code = VTSP_EVENT_MSG_CODE_T38;
        q_ptr->eventMsg.infc = infc;
        q_ptr->eventMsg.tick = dsp_ptr->tick1ms;
        q_ptr->eventMsg.msg.t38.reason    = VTSP_EVENT_T38_EVT;
        q_ptr->eventMsg.msg.t38.streamId  = streamId;
        q_ptr->eventMsg.msg.t38.state     = 
                fr38Obj_ptr->status_ptr->frs_state;
        q_ptr->eventMsg.msg.t38.pages     = 
                fr38Obj_ptr->status_ptr->frs_pages;
        q_ptr->eventMsg.msg.t38.trainDown = 
                fr38Obj_ptr->status_ptr->frs_train_down;
        for (index = 0; 
                index < fr38Obj_ptr->status_ptr->frs_event_cnt
                && index < FR38_MAX_EVENTS; 
                index++) {
            switch (fr38Obj_ptr->status_ptr->frs_events[index]) {
                case FR38_EVT_CRC_ERROR:
                    q_ptr->eventMsg.msg.t38.evt1 |= 
                            VTSP_T38_EVT1_CRC_E;
                    break;
                case FR38_EVT_UNRECOVERED_PACKET:
                    q_ptr->eventMsg.msg.t38.evt1 |= 
                            VTSP_T38_EVT1_UNRECOV_PKT;
                    break;
                case FR38_EVT_JITTER_BUFFER_OVERFLOW:
                    q_ptr->eventMsg.msg.t38.evt1 |= 
                            VTSP_T38_EVT1_JB_OFLOW;
                    break;
                case FR38_EVT_MDM_OVERFLOW:
                    q_ptr->eventMsg.msg.t38.evt1 |= 
                            VTSP_T38_EVT1_MDM_OFLOW;
                    break;
                case FR38_INVALID_UDP_FRAME:
                    q_ptr->eventMsg.msg.t38.evt1 |= 
                            VTSP_T38_EVT1_INV_UDP_FRAME;
                    break;
                case FR38_CORRUPT_RED_FRAME:
                    q_ptr->eventMsg.msg.t38.evt1 |= 
                            VTSP_T38_EVT1_CRPT_RED_FRAME;
                    break;
                case FR38_CORRUPT_FEC_FRAME:
                    q_ptr->eventMsg.msg.t38.evt1 |= 
                            VTSP_T38_EVT1_CRPT_FEC_FRAME;
                    break;
                case FR38_CORRUPT_PRIMARY_FRAME:
                    q_ptr->eventMsg.msg.t38.evt1 |= 
                            VTSP_T38_EVT1_CRPT_PRI_FRAME;
                    break;
                case FR38_UDPTL_FRAMING_ERROR:
                    q_ptr->eventMsg.msg.t38.evt1 |= 
                            VTSP_T38_EVT1_UDPTL_FRM_E;
                    break;
                /* Evt2 codes */
                case FR38_EVT_EPT_TIMER_EXPIRED:
                    q_ptr->eventMsg.msg.t38.evt2 |= 
                            VTSP_T38_EVT2_EPT_TE;
                    break;
                case FR38_EVT_V21_CD_TMR:
                    q_ptr->eventMsg.msg.t38.evt2 |= 
                            VTSP_T38_EVT2_V21_CD_T;
                    break;
                case FR38_EVT_T30_PREAM_TMR:
                    q_ptr->eventMsg.msg.t38.evt2 |= 
                            VTSP_T38_EVT2_T30_PREAM_T;
                    break;
                case FR38_EVT_INACTIVITY_TMR:
                    q_ptr->eventMsg.msg.t38.evt2 |= 
                            VTSP_T38_EVT2_INACT_T;
                    break;
                /* XXX do not report the following cases, for now */
                case FR38_EVT_T1_TIMER_EXPIRED:
                case FR38_EVT_T3_TIMER_EXPIRED:
                default:
                    break;
            }
        }
        if (0 != (q_ptr->eventMsg.msg.t38.evt1) || 
                (0 != q_ptr->eventMsg.msg.t38.evt2)) {
            VTSPR_sendEvent(q_ptr, &q_ptr->eventMsg, infc);
        }
    }
}

#endif /* VTSP_ENABLE_T38 */

