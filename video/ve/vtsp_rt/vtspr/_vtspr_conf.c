/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004-2010 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 30131 $ $Date: 2014-12-01 14:45:03 +0800 (Mon, 01 Dec 2014) $
 *
 */
#include "vtspr.h"
#include "_vtspr_private.h"

#ifndef VTSP_ENABLE_MP_LITE
/*
 * ======== _VTSPR_audioConfToLocal() ========
 *
 * Process conferencing to local end
 *
 */
void _VTSPR_audioConfToLocal(
    VTSPR_Obj    *vtspr_ptr,
    VTSPR_DSP    *dsp_ptr)
{
    VTSPR_ChanObj   *chan_ptr;
    VTSPR_ChanObj   *peerChan_ptr;
    VTSPR_StreamObj *stream_ptr;
    _VTSPR_FlowObj  *flow_ptr;
    VTSPR_ToneSeq   *tone_ptr;
    vint             streamId;
    vint             peerChan;
    vint             infc;       /* must be signed */
    vint             toneCopyFlag;
    /*
     * Do conferencing to the local interface here, as audio on all interfaces
     * and streams have been decoded.
     *
     * Decoded audio on each stream is in decOut_ptr for that stream.
     *
     * This code will do the following:
     *
     * - For each stream, sum all active peer streams, and
     * put the sum in a dsp_ptr->confLocal (a stack buffer) for encoding to
     * that stream.
     *
     */
    _VTSPR_FOR_ALL_INFC(infc) {

        chan_ptr   = _VTSPR_infcToChanPtr(dsp_ptr, infc);

        /*
         * If T38 is active, skip conference sum towards this interface.
         */
        if (0 != (VTSPR_ALG_CHAN_T38 & chan_ptr->algChannelState)) {
#ifdef VTSP_ENABLE_T38
            continue;
#endif
        }

        /*
         * If CIDCWS is active, skip conference sum towards this interface.
         */
        if (0 != (VTSPR_ALG_CHAN_CIDCWS & chan_ptr->algChannelState)) {
            continue;
        }

        if ((0 != (VTSPR_ALG_CHAN_TONE & chan_ptr->algChannelState)) ||
            (0 != (VTSPR_ALG_CHAN_TONE_QUAD & chan_ptr->algChannelState))) {
            /* 
             * If the toneControl is set to MIX, then the TONE_generate()
             * output is used during the make times, and not used during
             * the break times.
             *
             * Thus, do not copy the TONE_generate output, and run the audio
             * conferencing for the following cases (when in MIX mode):
             *      TONE is in the BREAK state
             *      TONE is DONE, and previously in the BREAK state.
             *
             * This achieves mixing of tone with voice during the break
             * times.
             */
            toneCopyFlag = 1;
            if (0 != (VTSPR_ALG_CHAN_TONE & chan_ptr->algChannelState)) { 
                tone_ptr = &chan_ptr->toneSeq;
                if ((VTSP_TONE_BREAK_MIX == tone_ptr->toneControl) &&
                        (VTSP_EVENT_TRAILING == tone_ptr->toneEdge)) { 
                        /* no copy, no write audioOut_ary buffer */
                    toneCopyFlag = 0; 
                }
            }
#ifdef VTSP_ENABLE_TONE_QUAD
            else if (0 != (VTSPR_ALG_CHAN_TONE_QUAD & chan_ptr->algChannelState)) {
                tone_ptr = &chan_ptr->toneQuadSeq;
                if ((VTSP_TONE_BREAK_MIX == tone_ptr->toneControl) &&
                        (VTSP_EVENT_TRAILING == tone_ptr->toneEdge)) {
                        /* no copy, no write audioOut_ary buffer */
                    toneCopyFlag = 0; 
                }
            }
#endif
            if ( 1 == toneCopyFlag ) {
                COMM_copy(chan_ptr->audioOut_ary,   /* dst_ptr */
                        chan_ptr->toneOut_ary,      /* src_ptr */
                        chan_ptr->numSamples10ms);  /* length */
                continue;
            }
        }

        /*
         * Zero temp dest buffer
         */
        COMM_fill(dsp_ptr->confLocal_ary, 0, VTSPR_NSAMPLES_STREAM);

        for (streamId = _VTSP_STREAM_PER_INFC - 1; streamId >= 0; streamId--) {
            stream_ptr = _VTSPR_streamIdToStreamPtr(dsp_ptr, infc, streamId);

            flow_ptr = _VTSPR_streamIdToFlowPtr(dsp_ptr, infc, streamId);

            if (_VTSPR_FLOW_STATE_IDLE != flow_ptr->playState) {
                /*
                 * If a flow is active on the stream, only sum in the contents
                 * of the buffer is the flow is LOCAL_PLAY. If not, the flow is
                 * using both the streams encoder and decoder. The flow is
                 * intended for the peer, not the local interface.
                 */
                if (0 != (VTSP_FLOW_DIR_LOCAL_PLAY & flow_ptr->flowDir)) {
                    /*
                     * Add flow data into conference buffer
                     */
                    COMM_sum(dsp_ptr->confLocal_ary, stream_ptr->streamDec_ary,
                            VTSPR_NSAMPLES_STREAM);
                }
            }
            else if (VTSP_STREAM_PEER_NETWORK != stream_ptr->streamParam.peer) {
                /*
                 * Local conference member.
                 * Add the audio data from peerInfc into this infc,
                 * depending on the direction setting.
                 * Skip this if the local conference interface is doing CIDCW.
                 */
                peerChan     = _VTSPR_infcToChan(stream_ptr->streamParam.peer);
                peerChan_ptr = &dsp_ptr->chan[peerChan];

                if (((VTSP_STREAM_DIR_RECVONLY ==
                        stream_ptr->streamParam.dir) ||
                        (VTSP_STREAM_DIR_SENDRECV ==
                        stream_ptr->streamParam.dir)) &&
                        (0 == (peerChan_ptr->algChannelState &
                        VTSPR_ALG_CHAN_CIDCWS))) {
                    COMM_sum(dsp_ptr->confLocal_ary,
                            peerChan_ptr->audioToPeer_ary,
                            VTSPR_NSAMPLES_STREAM);
                }
            }
            else {
                /*
                 * Network conference member.
                 * Add the audio data from that stream's decode array,
                 * depending on the direction setting.
                 */
                if ((VTSP_STREAM_DIR_RECVONLY ==
                        stream_ptr->streamParam.dir) ||
                        (VTSP_STREAM_DIR_SENDRECV ==
                        stream_ptr->streamParam.dir)) {
                    COMM_sum(dsp_ptr->confLocal_ary, stream_ptr->streamDec_ary,
                            VTSPR_NSAMPLES_STREAM);
                }
            }
        }

        /*
         * Place the conference sum towards this interface in the buffer
         * to be used by the audio driver and to be copied to Rin for
         * the next echo canceller processing loop.
         */
#ifdef VTSP_ENABLE_STREAM_16K
        if (VTSPR_NSAMPLES_10MS_8K == chan_ptr->numSamples10ms) {
            _VTSPR_downSample(&chan_ptr->udsAudioDown,
                   chan_ptr->audioOut_ary, dsp_ptr->confLocal_ary);            
        }
        else {
            COMM_copy(chan_ptr->audioOut_ary, dsp_ptr->confLocal_ary,
                chan_ptr->numSamples10ms);
        }
#else
        COMM_copy(chan_ptr->audioOut_ary, dsp_ptr->confLocal_ary,
                chan_ptr->numSamples10ms);
#endif
    }
}
#endif /* VTSP_ENABLE_MP_LITE. */

/*
 * ======== _VTSPR_audioConfToPeer() ========
 *
 * Process conferencing audio to peer end
 * and call encode
 *
 */
void _VTSPR_audioConfToPeer(
    VTSPR_Obj    *vtspr_ptr,
    VTSPR_Queues *q_ptr,
    VTSPR_DSP    *dsp_ptr)
{
    VTSPR_ChanObj    *chan_ptr;
    VTSPR_ChanObj    *peerChan_ptr;
    VTSPR_StreamObj  *stream_ptr;
    VTSPR_StreamObj  *peerStream_ptr;
    _VTSPR_FlowObj   *flow_ptr;
    _VTSPR_RtpObject *rtp_ptr;
#ifdef VTSP_ENABLE_DTMF    
    _VTSPR_DtmfObj   *dtmf_ptr;
#endif
    vint              streamId;
    uint32            silenceMask;
    vint              peerChan;
    vint              infc;
    uint32            confMask;
    vint              peerId;
    vint              coder;
    vint              encoder;
    VTSP_BlockHeader  blockHeader;
    /*
     *
     *
     * Do conferencing to the peer interface here, as audio on all interfaces
     * and streams have been decoded.
     *
     * - For each stream, sum and all active peer streams, and
     * put the sum in a stream_ptr->confPeer (a stack buffer) for encoding to
     * that stream.
     *
     * - Mute audio when necessary just prior to conferencing or encode
     *
     */
    _VTSPR_FOR_ALL_INFC(infc) {
        chan_ptr   = _VTSPR_infcToChanPtr(dsp_ptr, infc);

        /*
         * Compute conference sums towards each stream on this interface.
         */
        for (streamId = _VTSP_STREAM_PER_INFC - 1; streamId >= 0; streamId--) {
            stream_ptr = _VTSPR_streamIdToStreamPtr(dsp_ptr, infc, streamId);
            rtp_ptr    = _VTSPR_streamIdToRtpPtr(vtspr_ptr->net_ptr, infc,
                    streamId);
            /* XXX Use the one to one mapping between streams and flows here */
            flow_ptr = _VTSPR_streamIdToFlowPtr(dsp_ptr, infc, streamId);

            /*
             * If the stream is not sending or a flow has acquired the encoder,
             * increment the timestamp.
             */
            if (((VTSP_STREAM_DIR_SENDONLY != stream_ptr->streamParam.dir) &&
                    (VTSP_STREAM_DIR_SENDRECV != stream_ptr->streamParam.dir))
                    || ((_VTSPR_FLOW_STATE_IDLE != flow_ptr->recState) &&
                    ((VTSP_FLOW_DIR_LOCAL_RECORD & flow_ptr->flowDir) != 0))
                    || ((_VTSPR_FLOW_STATE_IDLE != flow_ptr->recState) &&
                    ((VTSP_FLOW_DIR_PEER_RECORD & flow_ptr->flowDir) != 0))) {
                stream_ptr->marker = 1;
            }
            /*
             * If the flow is set to record from the local interface, no data is
             * sent to the peer interface because the flow has co-opted the
             * encoder for this stream.
             */
            if ((_VTSPR_FLOW_STATE_IDLE != flow_ptr->recState) &&
                    ((VTSP_FLOW_DIR_LOCAL_RECORD & flow_ptr->flowDir) != 0)) {
                /*
                 * Copy the local interface data to the conference buffer.
                 */
                 COMM_copy(stream_ptr->confPeer_ary, chan_ptr->audioToPeer_ary,
                     VTSPR_NSAMPLES_STREAM);

                 /*
                 * Send the encoded data to the application. If the function
                 * returns 0, the record session is delayed until the current
                 * stream coder finishes processing its data block.
                 */
                if (0 != _VTSPR_flowRecord(dsp_ptr, q_ptr, flow_ptr,
                            stream_ptr)) {
                    continue;
                }
            }
            if ((_VTSPR_FLOW_STATE_IDLE != flow_ptr->recState) &&
                    ((VTSP_FLOW_DIR_PEER_RECORD & flow_ptr->flowDir) != 0)) {
                /*
                 * Copy the local interface data to the conference buffer.
                 */
                 COMM_copy(stream_ptr->confPeer_ary, stream_ptr->streamDec_ary,
                     VTSPR_NSAMPLES_STREAM);

                /*
                 * Send the encoded data to the application. If the function
                 * returns 0, the record session is delayed until the current
                 * stream coder finishes processing its data block.
                 */
                if (0 != _VTSPR_flowRecord(dsp_ptr, q_ptr, flow_ptr,
                            stream_ptr)) {
                    continue;
                }
            }

            if ((VTSP_STREAM_DIR_SENDONLY !=
                    stream_ptr->streamParam.dir) &&
                    (VTSP_STREAM_DIR_SENDRECV !=
                    stream_ptr->streamParam.dir)) {
                /*
                 * If SEND is off, skip conf sum.
                 * No encode, no block is sent to net.
                 *
                 * XXX Should not need to zero buffer here, but do it anyway
                 */
                COMM_fill(stream_ptr->confPeer_ary, 0, VTSPR_NSAMPLES_STREAM);
                continue;
            }

            if ((_VTSPR_FLOW_STATE_IDLE != flow_ptr->playState) &&
                    ((VTSP_FLOW_DIR_PEER_PLAY & flow_ptr->flowDir))  != 0) {
                /*
                 * Decode data from the application and recode it to send to the
                 * stream. If the coder type for the stream and flow are the
                 * same skip the recode step.
                 */
                encoder = stream_ptr->streamParam.encoder;
                coder   = flow_ptr->playMsg.coder;
                if (coder != encoder) {
                    /*
                     * If encoder is configured for CN, and the coder is the
                     * right type. Pass the CN payload to the next stage.
                     */
                    if ((VTSP_CODER_CN == coder) &&
                            (((VTSP_CODER_G711U == encoder) &&
                             (0 != (stream_ptr->streamParam.silenceComp &
                              VTSP_MASK_CODER_G711U))) ||
                             ((VTSP_CODER_G711A == encoder) &&
                             (0 != (stream_ptr->streamParam.silenceComp &
                              VTSP_MASK_CODER_G711A))) ||
#ifdef VTSP_ENABLE_16K_MU
                             ((VTSP_CODER_16K_MU == encoder) &&
                             (0 != (stream_ptr->streamParam.silenceComp &
                              VTSP_MASK_CODER_16K_MU))) ||
#endif /* end VTSP_ENABLE_16K_MU */
                             ((VTSP_CODER_G726_32K == encoder) &&
                             (0 != (stream_ptr->streamParam.silenceComp &
                              VTSP_MASK_CODER_G726_32K))))) {
                        /*
                         * CN is a legal type with G.711 or G.726, and silence
                         * compression is enabled.  Generate header information
                         * for VTSPR->RTP interface. Sets the extension field
                         * and timestamp field.
                         */
                        _VTSPR_setBlockHdr(stream_ptr, &blockHeader, coder);

                        /*
                         * Fill in other fields of BlockHeader here
                         */
                        blockHeader.dynamicCoder = _VTSPR_localToDynamicEncoder(
                                &stream_ptr->streamParam, coder);
                        blockHeader.localCoder = coder;
                        blockHeader.infc = infc;
                        blockHeader.streamId = stream_ptr->streamParam.streamId;

                        /*
                         * Call _VTSPR_rtpSend() to transmit data to network.
                         * The block data is in network byte order.
                         * framesM1 is not used for CN packets
                         */
                        _VTSPR_rtpSend(vtspr_ptr, rtp_ptr, &blockHeader,
                                    (uint8 *)flow_ptr->playMsg.payload,
                                    flow_ptr->playSize, 0);
                        continue;
                    }

                    /*
                     * The decoded flow is sent to the encoder.
                     */
                    COMM_copy(stream_ptr->confPeer_ary,
                              stream_ptr->streamDec_ary,
                              VTSPR_NSAMPLES_STREAM);
                    /*
                     * XXX Disable Silence Compress on the stream because the
                     * noisefloor for the flow was never calculated. This
                     * routine uses the noise floor from the local interface to
                     * determine whether CN should be sent.
                     */
                    silenceMask = stream_ptr->streamParam.silenceComp;
                    stream_ptr->streamParam.silenceComp &=
                            (~(VTSP_MASK_CODER_G711U
                               | VTSP_MASK_CODER_G711A
#ifdef VTSP_ENABLE_16K_MU
                               | VTSP_MASK_CODER_16K_MU
#endif /* end VTSP_ENABLE_16K_MU */
                               | VTSP_MASK_CODER_G726_32K)
                            );
                    _VTSPR_audioStreamEncode(vtspr_ptr, dsp_ptr, infc,
                            stream_ptr);
                    /*
                     * Restore the mask.
                     */
                    stream_ptr->streamParam.silenceComp = silenceMask;

                    continue;

                }
                else {
                    /*
                     * The flow coder matches the stream encoder. Generate
                     * header information for VTSPR->RTP interface.
                     * Sets the extension field and timestamp field.
                     */
                    _VTSPR_setBlockHdr(stream_ptr, &blockHeader, coder);

                    /*
                     * Fill in other fields of BlockHeader here
                     */
                    blockHeader.dynamicCoder = _VTSPR_localToDynamicEncoder(
                            &stream_ptr->streamParam, coder);
                    blockHeader.localCoder   = coder;
                    blockHeader.infc         = infc;
                    blockHeader.streamId     = stream_ptr->streamParam.streamId;

                    /*
                     * Call _VTSPR_rtpSend() to transmit data to network.
                     * The block data is in network byte order.
                     */
                    _VTSPR_rtpSend(vtspr_ptr, rtp_ptr, &blockHeader,
                                (uint8 *) &(flow_ptr->playMsg.payload
                                [flow_ptr->playCurrentOffset]),
                                flow_ptr->playSize, 0);
                    continue;
                }
            }
            else {
                /*
                 * Copy the interface audio to the temp conf dest buffer.
                 * Do this only if we are not doing CIDCW on this interface.
                 */
                if (0 == (chan_ptr->algChannelState & VTSPR_ALG_CHAN_CIDCWS)) {
                    COMM_copy(stream_ptr->confPeer_ary,
                            chan_ptr->audioToPeer_ary,
                            VTSPR_NSAMPLES_STREAM);
                }

                /*
                 * Conference sum.
                 * Add audio from any conferenced streams.
                 * confMask is a bit mask, with bit N set to 1 if the stream
                 * with streamID=N should be added to this stream.
                 *
                 * Check this mask, and add the streams to this stream's input
                 * array as required.
                 */

                confMask = stream_ptr->streamParam.confMask;
                peerId = 0;

                /*
                 * Do for each stream peer to be conferenced in
                 */
                while (confMask != 0) {
                    /*
                     * Determine the stream ID for the next peer by determining
                     * the placement of the lowest nonzero bit in confMask.
                     */
                    while (0 == (confMask & 0x1)) {
                        confMask = confMask >> 1;
                        peerId++;
                    }
                    confMask = confMask >> 1;

                    if (peerId == streamId) {
                        /*
                         * Skip conferencing self
                         */
                        continue;
                    }

                    peerStream_ptr = _VTSPR_streamIdToStreamPtr(dsp_ptr,
                                                            infc, peerId);

                    /*
                     * Check if we are receiving from the peer stream.
                     * If not receiving from that stream, then don't conference
                     * it in.
                     */
                    if ((VTSP_STREAM_DIR_SENDRECV !=
                            peerStream_ptr->streamParam.dir) &&
                            (VTSP_STREAM_DIR_RECVONLY !=
                            peerStream_ptr->streamParam.dir)) {
                        continue;
                    }

                    if (VTSP_STREAM_PEER_NETWORK ==
                            peerStream_ptr->streamParam.peer) {
                        /*
                         * Common case.
                         * Network peer.  Add the audio data from
                         * peerStream if configured to receive from that peer.
                         */
                        /* dot: streamDec->confPeer; */
                        if ((VTSP_STREAM_DIR_RECVONLY ==
                                peerStream_ptr->streamParam.dir) ||
                                (VTSP_STREAM_DIR_SENDRECV ==
                                peerStream_ptr->streamParam.dir)) {
                            COMM_sum(stream_ptr->confPeer_ary,
                                    peerStream_ptr->streamDec_ary,
                                    VTSPR_NSAMPLES_STREAM);
                        }
                    }
                    else {
                        if (infc != peerStream_ptr->streamParam.peer) {
                            /*
                             * If not a network peer and not self,
                             * this is another local peer. Add the audio data
                             * from peerInfc into this stream.
                             */
                            peerChan = _VTSPR_infcToChan(
                                    peerStream_ptr->streamParam.peer);
                            peerChan_ptr = &dsp_ptr->chan[peerChan];
                            COMM_sum(stream_ptr->confPeer_ary,
                                          peerChan_ptr->audioToPeer_ary,
                                          VTSPR_NSAMPLES_STREAM);
                        }
                    }
                }
            }
            /*
             * Encode the result
             */
            /*
            * Check for DTMF being enabled on a channel
            */
            if ( (VTSPR_INFC_TYPE_AUDIO != _VTSPR_infcToType(infc)) &&
                (0 != (chan_ptr->algChannelState & VTSPR_ALG_CHAN_DTMF))) {
#ifdef VTSP_ENABLE_DTMF                
                /*
                 * If DTMF relay is enabled on stream then set a bit mask to
                 * reflect status of DTMF relay.
                 * dtmfRemove : 00 -> Do not run DTMF remove
                 *              01 -> Run DTMF remove
                 *              10 -> Run DTMF remove
                 *              11 -> Run DTMF remove
                 */
                dtmf_ptr = chan_ptr->dtmf_ptr;
                if (VTSP_CODER_DTMF != stream_ptr->streamParam.encoder) {
                    if ((0 != (stream_ptr->streamParam.dtmfRelay &
                            (1 << stream_ptr->streamParam.encoder)))) {
                            dtmf_ptr->dtmfRemove |= (1 << streamId);
                    }
                    else {
                        /* Clear the dtmfRemove */
                        dtmf_ptr->dtmfRemove &= ~(1 << streamId);
                    }
                }
                else {
                    /* DTMF relay enabled, so enable Dtmf remove */
                    dtmf_ptr->dtmfRemove |= (1 << streamId);
                }
#endif
            }
            _VTSPR_audioStreamEncode(vtspr_ptr, dsp_ptr, infc, stream_ptr);
            /*
             * _VTSPR_audioStreamEncode() encode linear data to RTP packet, then send to network,
             * Place below line after _VTSPR_audioStreamEncode() can make sure the first RTP packet
             * use the initial "rtp_ptr->rtpTime" value which is initialized by _vtpsr_rtp_open.c
             *
             * Increment the rtpTime every 10 ms after _VTSPR_audioStreamEncode()
             */
            rtp_ptr->rtpTime +=
                    _VTSPR_getIncrTime(stream_ptr->streamParam.encoder,
                    rtp_ptr->lastLocVCoder);            
        } /* end for(streamId) */
    }     /* end FOR_ALL_INFC  */
}

