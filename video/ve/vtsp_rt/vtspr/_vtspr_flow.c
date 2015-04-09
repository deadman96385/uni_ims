/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL AND
 * PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2006-2009 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 30131 $ $Date: 2014-12-01 14:45:03 +0800 (Mon, 01 Dec 2014) $
 *
 */        
#include "vtspr.h"
#include "_vtspr_private.h"

/*
 * ======== _VTSPR_flowOpenFlush() ========
 *
 * This routine flushes the play queue when a flow is opened. It flushes all
 * messages whose keys to not match that of the current open flow.
 */
void _VTSPR_flowOpenFlush(
    _VTSPR_FlowObj  *flow_ptr,
    VTSPR_StreamObj *stream_ptr)
{
    vint payloadIndex;
    /*
     * Flush Queue. This loop ends if either the message is from the newly
     * opened flow or the queue is empty. In either case, there are only
     * a finite number of packets to process.
     */
    payloadIndex = 0;
    while (OSAL_msgQRecv(flow_ptr->flowFromAppQ,
            (char *)&(flow_ptr->playMsg), sizeof(_VTSP_FlowMsg),
            OSAL_NO_WAIT, NULL) > 0) {
        if (flow_ptr->key == flow_ptr->playMsg.key) {
            /*
             * The key number changed. This is the new payload for a flow
             * that will be opened soon. Ideally, this payload would be
             * "unreceived". Since this is not implemented, the next best thing
             * would be to prevent the first read from the queue after an open
             * and use this read instead. This can be done by setting the
             * payloadIndex to a non-zero value illegal value.
             */
            payloadIndex = -1;
            break;
        }
    }
    flow_ptr->playPayloadIndex = payloadIndex;
}

/*
 * ======== _VTSPR_flowAbortFlush() ========
 *
 * When a flow is aborted, this routine is called to flush the fromApp queue. If
 * the abort is send from the application, flush the queue until the abort
 * message is read. Otherwise, flush all messages with the key of the aborted
 * flow. This can happen is the abort was caused by a DTMF event.
 */
void _VTSPR_flowAbortFlush(
    VTSPR_DSP       *dsp_ptr,
    _VTSPR_FlowObj  *flow_ptr)
{
    vint             payloadIndex;
    VTSPR_StreamObj *stream_ptr;

    stream_ptr = _VTSPR_streamIdToStreamPtr(dsp_ptr, flow_ptr->infc,
                                            flow_ptr->streamId);

    /*
     * Flush Queue. This loop ends if either a special abort packet is
     * found, or the queue is empty. In either case, there are only a finite
     * number of packets to process.
     */
    payloadIndex = 0;
    while (OSAL_msgQRecv(flow_ptr->flowFromAppQ,
            (char *)&(flow_ptr->playMsg), sizeof(_VTSP_FlowMsg),
            OSAL_NO_WAIT, NULL) > 0) {
        if (VTSP_CODER_ABORT == flow_ptr->playMsg.coder) {
            break;
        }
        else if (flow_ptr->key != flow_ptr->playMsg.key) {
            /*
             * The key number changed. This is the new payload for a flow
             * that will be opened soon. Ideally, this payload would be
             * "unreceived". Since this is not implemented, the next best thing
             * would be to prevent the first read from the queue after an open
             * and use this read instead. This can be done by setting the
             * payloadIndex to a non-zero value illegal value.
             */
            payloadIndex = -1;
            break;
        }
    }

    /*
     * If the stream coder was not CN and it was different from the flow
     * decoder, initialize it.
     */
    if ((VTSP_STREAM_DIR_RECVONLY == stream_ptr->streamParam.dir) ||
            (VTSP_STREAM_DIR_SENDRECV == stream_ptr->streamParam.dir)) {
        _VTSPR_initCoder(dsp_ptr, stream_ptr, VTSPR_DECODER,
                stream_ptr->decoderType);
    }
    /*
     * If the play was directed to the PEER interface, the encoder does not need
     * to be re-initialized because the encoder was never changed during the
     * flow. Only the stream would have changed the encoder.
     */

    /*
     * Reinitialize PLC if stream is active.
     */
    if (stream_ptr->streamParam.dir != _VTSP_STREAM_DIR_ENDED) {
        _VTSPR_algStateStream(dsp_ptr, flow_ptr->infc,
                flow_ptr->streamId, VTSPR_ALG_STREAM_PLC, 0);
        _VTSPR_algStateStream(dsp_ptr, flow_ptr->infc,
                flow_ptr->streamId, 0, VTSPR_ALG_STREAM_PLC);
    }
        
    /*
     * Re-initialize the flow object.
     */
    flow_ptr->playPayloadIndex = payloadIndex;
    flow_ptr->playMsg.coder = stream_ptr->decoderType;
    flow_ptr->playLastCoder = VTSP_CODER_UNAVAIL;
}

/*
 * ======== _VTSPR_flowAbortRec() ========
 */
void _VTSPR_flowAbortRec(
    VTSPR_Obj       *vtspr_ptr,
    VTSPR_Queues    *q_ptr,
    _VTSPR_FlowObj  *flow_ptr,
    VTSPR_StreamObj *stream_ptr)
{
    vint osalStatus;

    /*
     * Send the last payload.
     */
    flow_ptr->recMsg.coder = flow_ptr->recCoder;
    flow_ptr->recMsg.blockSize = flow_ptr->recPayloadIndex;
    flow_ptr->recMsg.key = flow_ptr->key;
    flow_ptr->recMsg.control = 0;
    flow_ptr->recMsg.duration = flow_ptr->recDuration;

    osalStatus = OSAL_msgQSend(flow_ptr->flowToAppQ,
            (char *)&(flow_ptr->recMsg), sizeof(_VTSP_FlowMsg),
            VTSP_TIMEOUT_NO_WAIT, NULL);

    /*
     * If the last packet failed to send. Send a flow resource error.
     */
    if (OSAL_SUCCESS != osalStatus) {
        q_ptr->eventMsg.code = VTSP_EVENT_MSG_CODE_FLOW;
        q_ptr->eventMsg.infc = flow_ptr->infc;
        q_ptr->eventMsg.tick = vtspr_ptr->dsp_ptr->tick1ms;
        q_ptr->eventMsg.msg.flow.reason = VTSP_EVENT_FLOW_RESOURCE;
        q_ptr->eventMsg.msg.flow.flowId = flow_ptr->flowId;
        VTSPR_sendEvent(q_ptr, &q_ptr->eventMsg, flow_ptr->infc);
    }

    /*
     * Re-initialize the stream encoder if the stream is sending.
     */
    if ((VTSP_STREAM_DIR_SENDONLY == stream_ptr->streamParam.dir) ||
            (VTSP_STREAM_DIR_SENDRECV == stream_ptr->streamParam.dir)) {
        stream_ptr->lastEncoder = VTSP_CODER_UNAVAIL;
    }
}
   
/*
 * ======== _VTSPR_flowPlayNext() ========
 *
 * This function is used to get the next encode voice buffer from the flow.
 */
vint _VTSPR_flowPlayNext(
    VTSPR_DSP        *dsp_ptr,
    VTSPR_Queues     *q_ptr,
    _VTSPR_FlowObj   *flow_ptr,
    VTSPR_StreamObj  *stream_ptr,
    void            **data_ptr)
{
    vint size;
    vint payloadIndex;
    vint coder;

    payloadIndex = flow_ptr->playPayloadIndex;
   
    /*
     * If the payloadIndex is 0, a new message needs to be read from the queue.
     */
    if (0 == payloadIndex) {
        if ((size = OSAL_msgQRecv(flow_ptr->flowFromAppQ,
                (char *)&(flow_ptr->playMsg), sizeof(_VTSP_FlowMsg),
                OSAL_NO_WAIT, NULL)) < 0) {
            /*
             * A invalid message was read from the application. Find out the
             * reason.
             */
            size = 0;
        }

        /*
         * If the flow is closing, it finishes either with an empty queue or the
         * start of a new flow.
         */
        if (_VTSPR_FLOW_STATE_CLOSING == flow_ptr->playState) {
            if ((0 == size) || (flow_ptr->key != flow_ptr->playMsg.key)) {
                /*
                 * If a new flow is available, this is probably an error
                 * because the new flow would force an open after the close.
                 * If it some how did not, save the new packet for the next
                 * open.
                 */
                if (0 != size) {
                    payloadIndex = -1;
                }
                /*
                 * Force initialization of the stream's decoder.
                 */
                flow_ptr->playMsg.coder = stream_ptr->decoderType;
                flow_ptr->playLastCoder = VTSP_CODER_UNAVAIL;
                flow_ptr->playState = _VTSPR_FLOW_STATE_IDLE;
                flow_ptr->playPayloadIndex = payloadIndex;
                flow_ptr->playMsg.payload[0] = 80;
                *data_ptr = &(flow_ptr->playMsg.payload[0]);
                flow_ptr->playSize = VTSP_BLOCK_G711_CN_SZ;

                /*
                 * If the play was directed to the PEER interface, the encoder
                 * needs to be re-initialized.
                 */
                if (0 != (VTSP_FLOW_DIR_PEER_PLAY & flow_ptr->flowDir)) {
                    stream_ptr->lastEncoder = VTSP_CODER_UNAVAIL;
                }
                /*
                 * Reinitialize PLC if stream is active.
                 */
                if (stream_ptr->streamParam.dir != _VTSP_STREAM_DIR_ENDED) {
                    _VTSPR_algStateStream(dsp_ptr, flow_ptr->infc,
                            flow_ptr->streamId, VTSPR_ALG_STREAM_PLC, 0);
                    _VTSPR_algStateStream(dsp_ptr, flow_ptr->infc,
                            flow_ptr->streamId, 0, VTSPR_ALG_STREAM_PLC);
                }
                /*
                 * Send event to application that flow has completed.
                 */
                q_ptr->eventMsg.code = VTSP_EVENT_MSG_CODE_FLOW;
                q_ptr->eventMsg.infc = flow_ptr->infc;
                q_ptr->eventMsg.tick = dsp_ptr->tick1ms;
                q_ptr->eventMsg.msg.flow.reason = VTSP_EVENT_COMPLETE;
                q_ptr->eventMsg.msg.flow.flowId = flow_ptr->flowId;
                q_ptr->eventMsg.msg.flow.key = flow_ptr->key;
                VTSPR_sendEvent(q_ptr, &q_ptr->eventMsg, flow_ptr->infc);
                return (VTSP_CODER_CN);
            }
        }
        else if (0 == size) {
            /*
             * There was not a packet available, play CN except for G.729. For
             * iLBC use it builtin packloss compensation.
             */
            switch (flow_ptr->playLastCoder) {
                case VTSP_CODER_G729:
#ifdef VTSP_ENABLE_G722
                case VTSP_CODER_G722:
#endif       
#ifdef VTSP_ENABLE_G722P1
                case VTSP_CODER_G722P1_20MS:
#endif                
#ifdef VTSP_ENABLE_ILBC
                case VTSP_CODER_ILBC_20MS:
                case VTSP_CODER_ILBC_30MS:
#endif
#ifdef VTSP_ENABLE_G723
                case VTSP_CODER_G723_30MS:
#endif
                    /*
                     * Use coder's own PLC mechanism.
                     */
                    flow_ptr->playSize = 0;
                    return (flow_ptr->playLastCoder);
                default:
                    flow_ptr->playPayloadIndex = 0;
                    flow_ptr->playMsg.payload[0] = 80;
                    *data_ptr = &(flow_ptr->playMsg.payload[0]);
                    flow_ptr->playSize = VTSP_BLOCK_G711_CN_SZ;
                    return (VTSP_CODER_CN);
            }
        }
    } else if (-1 == payloadIndex) {
        payloadIndex = 0;
    }

    /*
     * At this point, we are ACTIVE, there is no need to check the change in
     * flows (keys) because the flows will be naturally concatonated.
     */

    coder = flow_ptr->playMsg.coder;
    flow_ptr->playControl = flow_ptr->playMsg.control;
    if (flow_ptr->playLastCoder != coder) {
        /*
         * The speech coder changed, reinitialize.
         */
        if (VTSP_CODER_CN != coder) {
            _VTSPR_initCoder(dsp_ptr, stream_ptr, VTSPR_DECODER, coder);
        }
        flow_ptr->playLastCoder = coder;
    }

    /*
     * Now get the next block of data.
     *
     * Don't fill the data_ptr if the flow is aborted or closed.
     */
    flow_ptr->playSize = 0;
    if (_VTSPR_FLOW_STATE_IDLE == flow_ptr->playState) {
        /*
         * If the flow has been idled, play comfort noise. And reinitialize the
         * speech coder on that stream.
         */
        flow_ptr->playPayloadIndex = payloadIndex;
        /*
         * Initialize the decoder when closing.
         */
        if (VTSP_CODER_CN != coder) {
            _VTSPR_initCoder(dsp_ptr, stream_ptr, VTSPR_DECODER, coder);
        }
        /*
         * If the stream coder was not CN and it was different from the flow
         * decoder, initialize it.
         */
        if ((VTSP_STREAM_DIR_RECVONLY == stream_ptr->streamParam.dir) ||
                (VTSP_STREAM_DIR_SENDRECV == stream_ptr->streamParam.dir)) {
                _VTSPR_initCoder(dsp_ptr, stream_ptr, VTSPR_DECODER,
                        stream_ptr->decoderType);
            }
        return (VTSP_CODER_CN);
    }
    switch (coder) {
        case VTSP_CODER_G711U:
        case VTSP_CODER_G711A:
            /*
             * Get the next VTSP_BLOCK_G711_10MS_SZ bytes and put them in the
             * decode data buffer.
             */
            *data_ptr = &(flow_ptr->playMsg.payload[payloadIndex]);
            flow_ptr->playCurrentOffset = payloadIndex;
            flow_ptr->playSize = VTSP_BLOCK_G711_10MS_SZ;
            payloadIndex += VTSP_BLOCK_G711_10MS_SZ;
            if (payloadIndex >= flow_ptr->playMsg.blockSize) {
                payloadIndex = 0;
            }
            break;
#ifdef VTSP_ENABLE_16K_MU
        case VTSP_CODER_16K_MU:
            /*
             * Get the next VTSP_BLOCK_16K_MU_SZ bytes and put them in the
             * decode data buffer.
             */
            *data_ptr = &(flow_ptr->playMsg.payload[payloadIndex]);
            flow_ptr->playCurrentOffset = payloadIndex;
            flow_ptr->playSize = VTSP_BLOCK_16K_MU_SZ;
            payloadIndex += VTSP_BLOCK_16K_MU_SZ;
            if (payloadIndex >= flow_ptr->playMsg.blockSize) {
                payloadIndex = 0;
            }
            break;
#endif /* end VTSP_ENABLE_16K_MU */
        case VTSP_CODER_CN:
            /*
             * The first time that a CN is read from the queue, payloadIndex is
             * 0. Send the CN block to the decoder. If the payloadIndex is not
             * 0, increment the duration counter until the CN period has
             * expired.
             */
            if (0 == payloadIndex) {
                *data_ptr = &(flow_ptr->playMsg.payload[0]);
                flow_ptr->playCurrentOffset = 0;
                flow_ptr->playSize = VTSP_BLOCK_G711_CN_SZ;
                flow_ptr->playDuration = 1;
                if (1 == flow_ptr->playMsg.duration) {
                    /*
                     * If silence duration has been exceeded for next
                     * interval, set payloadIndex to 0 to force a read from
                     * the queue for the next interval.
                     */
                    payloadIndex = 0;
                }
                else {
                    payloadIndex = VTSP_BLOCK_G711_CN_SZ;
                }
            }
            else {
                flow_ptr->playSize = 0;
                flow_ptr->playDuration += 1;
                if (flow_ptr->playDuration >= flow_ptr->playMsg.duration) {
                    /*
                     * If silence duration has been exceeded for next
                     * interval, set payloadIndex to 0 to force a read from
                     * the queue for the next interval.
                     */
                    payloadIndex = 0;
                }
                else {
                    payloadIndex = VTSP_BLOCK_G711_CN_SZ;
                }
            }
            break;
#ifdef VTSP_ENABLE_G729
        case VTSP_CODER_G729:
            /*
             * Get the next VTSP_BLOCK_G711_10MS_SZ bytes and put them in the
             * decode data buffer.
             */
            if (VTSP_BLOCK_G729_SID_SZ == flow_ptr->playMsg.blockSize) {
                /*
                 * This is a SID block. The block is passed to the coder the
                 * first time. After that nothing is passed until the duration
                 * expires.
                 */
                if (0 == payloadIndex) {
                    *data_ptr = &(flow_ptr->playMsg.payload[0]);
                    flow_ptr->playCurrentOffset = 0;
                    flow_ptr->playSize = VTSP_BLOCK_G729_SID_SZ;
                    flow_ptr->playDuration = 1;
                    if (1 == flow_ptr->playMsg.duration) {
                        /*
                         * If silence duration has been exceeded for next
                         * interval, set payloadIndex to 0 to force a read from
                         * the queue for the next interval.
                         */
                        payloadIndex = 0;
                    }
                    else {
                        payloadIndex = VTSP_BLOCK_G729_SID_SZ;
                    }
                }
                else {
                    flow_ptr->playSize = 0;
                    flow_ptr->playDuration += 1;
                    if (flow_ptr->playDuration >= flow_ptr->playMsg.duration) {
                        /*
                         * If silence duration has been exceeded for next
                         * interval, set payloadIndex to 0 to force a read from
                         * the queue for the next interval.
                         */
                        payloadIndex = 0;
                    }
                    else {
                        payloadIndex = VTSP_BLOCK_G729_SID_SZ;
                    }
                }
            }
            else {
                /*
                 * For non-SID blocks, copy a 10ms block of decode data to the
                 * decoded data buffer.
                 */
                *data_ptr = &(flow_ptr->playMsg.payload[payloadIndex]);
                flow_ptr->playCurrentOffset = payloadIndex;
                flow_ptr->playSize = VTSP_BLOCK_G729_10MS_SZ;
                payloadIndex += VTSP_BLOCK_G729_10MS_SZ;
                if (payloadIndex >= flow_ptr->playMsg.blockSize) {
                    payloadIndex = 0;
                }
            }
            break;
#endif
#ifdef VTSP_ENABLE_G722
        case VTSP_CODER_G722:
            /*
             * Get the next VTSP_BLOCK_G722_10MS_SZ bytes and put them in the
             * decode data buffer.
             */
            *data_ptr = &(flow_ptr->playMsg.payload[payloadIndex]);
            flow_ptr->playCurrentOffset = payloadIndex;
            flow_ptr->playSize = VTSP_BLOCK_G722_SZ;
            payloadIndex += VTSP_BLOCK_G722_SZ;
            if (payloadIndex >= flow_ptr->playMsg.blockSize) {
                payloadIndex = 0;
            }
            break;
#endif            
#ifdef VTSP_ENABLE_G726
        case VTSP_CODER_G726_32K:
            /*
             * Get the next VTSP_BLOCK_G726_32K_10MS_SZ bytes and put them in the
             * decode data buffer.
             */
            *data_ptr = &(flow_ptr->playMsg.payload[payloadIndex]);
            flow_ptr->playCurrentOffset = payloadIndex;
            flow_ptr->playSize = VTSP_BLOCK_G726_32K_10MS_SZ;
            payloadIndex += VTSP_BLOCK_G726_32K_10MS_SZ;
            if (payloadIndex >= flow_ptr->playMsg.blockSize) {
                payloadIndex = 0;
            }
            break;
#endif
#ifdef VTSP_ENABLE_ILBC
        case VTSP_CODER_ILBC_20MS:
            /*
             * Get the next VTSP_BLOCK_ILBC_20MS_SZ bytes and put them in the
             * decode data buffer. Only get data when the decoder needs it.
             */
            if (0 == stream_ptr->multiDecObj.decOffset) {
                *data_ptr = &(flow_ptr->playMsg.payload[payloadIndex]);
                flow_ptr->playCurrentOffset = payloadIndex;

                payloadIndex += VTSP_BLOCK_ILBC_20MS_SZ;
            }
            else {
                if (payloadIndex >= flow_ptr->playMsg.blockSize) {
                    payloadIndex = 0;
                }
            }
            flow_ptr->playSize = VTSP_BLOCK_ILBC_20MS_SZ;
            break;

        case VTSP_CODER_ILBC_30MS:
            /*
             * Get the next VTSP_BLOCK_ILBC_30MS_SZ bytes and put them in the
             * decode data buffer. Only get data when the decoder needs it.
             */
            if (0 == stream_ptr->multiDecObj.decOffset) {
                /*
                 * In the first 10ms phase, get the frame that will be decoded.
                 */
                *data_ptr = &(flow_ptr->playMsg.payload[payloadIndex]);
                flow_ptr->playCurrentOffset = payloadIndex;
                payloadIndex += VTSP_BLOCK_ILBC_30MS_SZ;
            }
            else if (80 == stream_ptr->multiDecObj.decOffset) {
                /*
                 * In the second 10ms phase, there is nothing to do.
                 */
            }
            else {
                /*
                 * In the final phase, check to see if a new block needs to be
                 * read from the queue. If it does, reset payloadIndex.
                 */
                if (payloadIndex >= flow_ptr->playMsg.blockSize) {
                    payloadIndex = 0;
                }
            }
            flow_ptr->playSize = VTSP_BLOCK_ILBC_30MS_SZ;
            break;
#endif
#ifdef VTSP_ENABLE_G723
        case VTSP_CODER_G723_30MS:
            /*
             * Get the next VTSP_BLOCK_G723_30MS_SZ bytes and put them in the
             * decode data buffer. Only get data when the decoder needs it.
             */
            if (0 == stream_ptr->multiDecObj.decOffset) {
                /*
                 * In the first 10ms phase, get the frame that will be decoded.
                 */
                *data_ptr = &(flow_ptr->playMsg.payload[payloadIndex]);
                flow_ptr->playCurrentOffset = payloadIndex;
                payloadIndex += VTSP_BLOCK_G723_30MS_63_SZ; /* XXX wrong? */
            }
            else if (80 == stream_ptr->multiDecObj.decOffset) {
                /*
                 * In the second 10ms phase, there is nothing to do.
                 */
            }
            else {
                /*
                 * In the final phase, check to see if a new block needs to be
                 * read from the queue. If it does, reset payloadIndex.
                 */
                if (payloadIndex >= flow_ptr->playMsg.blockSize) {
                    payloadIndex = 0;
                }
            }
            flow_ptr->playSize = VTSP_BLOCK_G723_30MS_63_SZ;  /* XXX wrong? */
            break;
#endif
#ifdef VTSP_ENABLE_G722P1
        case VTSP_CODER_G722P1_20MS:
            /*
             * Get the next VTSP_BLOCK_G722P1_20MS_SZ bytes and put them in the
             * decode data buffer. Only get data when the decoder needs it.
             */
            if (0 == stream_ptr->multiDecObj.decOffset) {
                /*
                 * In the first 10ms phase, get the frame that will be decoded.
                 */
                *data_ptr = &(flow_ptr->playMsg.payload[payloadIndex]);
                flow_ptr->playCurrentOffset = payloadIndex;
                payloadIndex += VTSP_BLOCK_G722P1_24KBPS_20MS_SZ;
            }
            else {
                /*
                 * In the final phase, check to see if a new block needs to be
                 * read from the queue. If it does, reset payloadIndex.
                 */
                if (payloadIndex >= flow_ptr->playMsg.blockSize) {
                    payloadIndex = 0;
                }
            }
            flow_ptr->playSize = VTSP_BLOCK_G722P1_24KBPS_20MS_SZ;
            break;
#endif               
    }
    /*
     * Update the payload index in the flow object.
     */
    flow_ptr->playPayloadIndex = payloadIndex;

    return (coder);
}

/*
 * ======== _VTSPR_flowRecord() ========
 *
 * This routine is called with the data that is to be passed to the application.
 */
vint _VTSPR_flowRecord(
    VTSPR_DSP       *dsp_ptr,
    VTSPR_Queues    *q_ptr,
    _VTSPR_FlowObj  *flow_ptr,
    VTSPR_StreamObj *stream_ptr)
{
    vint *src_ptr = NULL; /* 80 or 160 vints; depends on flow_ptr->recCoder */
    void *dst_ptr;
    vint  payloadReady;
    vint  payloadIndex;
    vint  duration;

    if (flow_ptr->recLastCoder != flow_ptr->recCoder) {
        /*
         * Initialize encoder.
         */
        _VTSPR_initCoder(dsp_ptr, stream_ptr, VTSPR_ENCODER,
                flow_ptr->recCoder);
        flow_ptr->recLastCoder = flow_ptr->recCoder;
    }
    payloadReady = 0;
    payloadIndex = flow_ptr->recPayloadIndex;
    duration = flow_ptr->recDuration + 1;

#ifdef VTSP_ENABLE_STREAM_16K
    if (_VTSPR_isCoderWb(flow_ptr->recCoder)) {
        src_ptr = stream_ptr->confPeer_ary;
    }
    else {
#ifndef VTSP_ENABLE_MP_LITE
            _VTSPR_downSample(&stream_ptr->udsStreamDown,
                   stream_ptr->streamEncIn_ary, stream_ptr->confPeer_ary);
        src_ptr = stream_ptr->streamEncIn_ary;
#endif
    }
#else /* not VTSP_ENABLE_STREAM_16K */
    src_ptr = stream_ptr->confPeer_ary;
#endif /* end VTSP_ENABLE_STREAM_16K */

    dst_ptr = &(flow_ptr->recMsg.payload[payloadIndex]);

    switch (flow_ptr->recCoder) {
    case VTSP_CODER_G711U:
        
        /*
         * Perform SID encode here if SID is enabled.
         * Set cnPkt if a CN packet is generated.
         */
        COMM_lin2mu(src_ptr, src_ptr, VTSPR_NSAMPLES_10MS_8K);
        COMM_pack32(dst_ptr, src_ptr, VTSPR_NSAMPLES_10MS_8K);
        payloadIndex += VTSP_BLOCK_G711_10MS_SZ;

        /*
         * If the next coded voice block will not fit in the message. Send the
         * current message and reset the index.
         */
        if ((payloadIndex + VTSP_BLOCK_G711_10MS_SZ) >
                _VTSP_Q_FLOW_PAYLOAD_SZ) {
            payloadReady = 1;
        }
        break;                

#ifdef VTSP_ENABLE_16K_MU
    case VTSP_CODER_16K_MU:
        /*
         * Convert to Mu-law
         */
        COMM_lin2mu(src_ptr, src_ptr, VTSPR_NSAMPLES_10MS_16K);
        COMM_pack32(dst_ptr, src_ptr, VTSPR_NSAMPLES_10MS_16K);
        payloadIndex += VTSP_BLOCK_16K_MU_SZ;

        /*
         * If the next coded voice block will not fit in the message, send the
         * current message and reset the index.
         */
        if ((payloadIndex + VTSP_BLOCK_16K_MU_SZ) >
                _VTSP_Q_FLOW_PAYLOAD_SZ) {
            payloadReady = 1;
        }
        break;                
#endif /* end VTSP_ENABLE_16K_MU */
    case VTSP_CODER_G711A:
        
        /*
         * Convert to alaw levels.
         */
        COMM_shiftRight(src_ptr, src_ptr, 1, VTSPR_NSAMPLES_10MS_8K);

        COMM_lin2a(src_ptr, src_ptr, VTSPR_NSAMPLES_10MS_8K);
        COMM_pack32(dst_ptr, src_ptr, VTSPR_NSAMPLES_10MS_8K);
        payloadIndex += VTSP_BLOCK_G711_10MS_SZ;

        /*
         * If the next coded voice block will not fit in the message. Send the
         * current message and reset the index.
         */
        if ((payloadIndex + VTSP_BLOCK_G711_10MS_SZ) >
                _VTSP_Q_FLOW_PAYLOAD_SZ) {
            payloadReady = 1;
        }
        break;                
            
#ifdef VTSP_ENABLE_G729
    case VTSP_CODER_G729:
#ifdef VTSP_ENABLE_G729_ACCELERATOR
#warning XXX No Flow for multi-frame coders
#else
        /*
         * Convert to Q.15 from Q.13.
         */
        COMM_shiftLeft(src_ptr, src_ptr, 2, VTSPR_NSAMPLES_10MS_8K);
        /*
         * Run G.729A
         */
        /*
         * XXX - Silence compression is not managed as part of a record flow.
         */
            stream_ptr->g729EncObj.vad_enable = 0;

        payloadIndex += G729AB_encode(&stream_ptr->g729EncObj, src_ptr,
                (int16 *)dst_ptr);
#endif // VTSP_ENABLE_G729_ACCELERATOR
        /*
         * If the next coded voice block will not fit in the message. Send the
         * current message and reset the index.
         */
        if ((payloadIndex + VTSP_BLOCK_G729_10MS_SZ) >
                _VTSP_Q_FLOW_PAYLOAD_SZ) {
            payloadReady = 1;
        }
        break;
#endif
#ifdef VTSP_ENABLE_G722
    case VTSP_CODER_G722:
        /*
         * Run G.722
         */

        payloadIndex += g722_encode(&stream_ptr->g722EncObj, src_ptr,
                (uint8 *)dst_ptr);
        /*
         * If the next coded voice block will not fit in the message. Send the
         * current message and reset the index.
         */
        if ((payloadIndex + VTSP_BLOCK_G722_SZ) >
                _VTSP_Q_FLOW_PAYLOAD_SZ) {
            payloadReady = 1;
        }
        break;
#endif            
#ifdef VTSP_ENABLE_G726
    case VTSP_CODER_G726_32K:
#ifdef VTSP_ENABLE_G726_ACCELERATOR
#warning XXX No Flow for multi-frame coders
#else
        /*
         * Run G.726
         */
        stream_ptr->g726EncObj.src_ptr = src_ptr;
        stream_ptr->g726EncObj.dst_ptr = dst_ptr;
        G726_encode(&stream_ptr->g726EncObj);
        payloadIndex += VTSP_BLOCK_G726_32K_10MS_SZ;
#endif
        /*
         * If the next coded voice block will not fit in the message. Send the
         * current message and reset the index.
         */
        if ((payloadIndex + VTSP_BLOCK_G726_32K_10MS_SZ) >
                _VTSP_Q_FLOW_PAYLOAD_SZ) {
            payloadReady = 1;
        }
        break;
#endif
#ifdef VTSP_ENABLE_ILBC
    case VTSP_CODER_ILBC_20MS:
        /*
         * Run iLBC encoder for 20 ms blocks. 
         */
#warning XXX No Flow for multi-frame coders        
        /*
         * If the next coded voice block will not fit in the message. Send the
         * current message and reset the index.
         */
        if ((payloadIndex + VTSP_BLOCK_ILBC_20MS_SZ) >
                _VTSP_Q_FLOW_PAYLOAD_SZ) {
            payloadReady = 1;
        }
        break;
    case VTSP_CODER_ILBC_30MS:
        /*
         * Run iLBC encoder for 30 ms blocks. 
         */
#warning XXX No Flow for multi-frame coders        
        /*
         * If the next coded voice block will not fit in the message. Send the
         * current message and reset the index.
         */
        if ((payloadIndex + VTSP_BLOCK_ILBC_30MS_SZ) >
                _VTSP_Q_FLOW_PAYLOAD_SZ) {
            payloadReady = 1;
        }
        break;
#endif
#ifdef VTSP_ENABLE_G723
    case VTSP_CODER_G723_30MS:
        /*
         * Run iLBC encoder for 30 ms blocks. 
         */
#warning XXX No Flow for multi-frame coders        
        /*
         * If the next coded voice block will not fit in the message. Send the
         * current message and reset the index.
         */
        if ((payloadIndex + VTSP_BLOCK_G723_30MS_63_SZ) >
                _VTSP_Q_FLOW_PAYLOAD_SZ) {
            payloadReady = 1;
        }
        break;
#endif
    }

    /*
     * If the payload ready flag has been set, send the data to the application.
     */
    if (0 != payloadReady) {
        flow_ptr->recMsg.coder = flow_ptr->recCoder;
        flow_ptr->recMsg.blockSize = payloadIndex;
        flow_ptr->recMsg.key = flow_ptr->key;
        flow_ptr->recMsg.control = 0;
        flow_ptr->recMsg.duration = duration;

        OSAL_msgQSend(flow_ptr->flowToAppQ,
                (char *)&(flow_ptr->recMsg), sizeof(_VTSP_FlowMsg),
                VTSP_TIMEOUT_NO_WAIT, NULL);

        payloadIndex = 0;
        duration = 0;
    }
    flow_ptr->recPayloadIndex = payloadIndex;
    flow_ptr->recDuration = duration;
    return (1);
}
