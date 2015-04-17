/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2006-2010 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 12522 $ $Date: 2010-07-13 18:52:44 -0400 (Tue, 13 Jul 2010) $
 *
 */
#include "_vc_private.h"

/*
 * ======== _VC_setBlockHdr() ========
 *
 * Set block header information for RTP for packing and sending out blocks
 * generated by the video coders.
 */
void _VC_setBlockHdr(
    _VC_StreamObj      *stream_ptr,
    VTSP_BlockHeader   *hdr_ptr,
    uvint               encType)
{

    hdr_ptr->extension  &= ~(VTSP_MASK_EXT_SEND);

    switch (encType) {
        case VTSP_CODER_VIDEO_H263:
            hdr_ptr->extension |= VTSP_MASK_EXT_SEND;
            if (1 == stream_ptr->enc.marker) {
                hdr_ptr->extension |= VTSP_MASK_EXT_MARKER;
                stream_ptr->enc.marker = 0;
            }
            return;
        case VTSP_CODER_VIDEO_H264:
            hdr_ptr->extension |= VTSP_MASK_EXT_SEND;
            if (1 == stream_ptr->enc.marker) {
                hdr_ptr->extension |= VTSP_MASK_EXT_MARKER;
                stream_ptr->enc.marker = 0;
            }
            return;

        case VTSP_CODER_UNAVAIL:
            /* Fall-through */
        default:
            /*
             * May get here if application sets improper encoder.
             * Don't send anything.
             */
            break;
    }
}

/*
 * ======== _VC_setHdrExtension() ========
 *
 * Set RTP Header Extension for last packet in key (I-frame) frame.
 */
void _VC_setHdrExtension(
    _VC_RtpObject      *rtp_ptr,
    VTSP_BlockHeader   *hdr_ptr,
    uint8               rcsRtpExtnPayload,
    vint                extmapId)
{

    uint8 *buf_ptr;
    vint key = 0;

    /* Check if RTP is Marked. This indicates if its the last frame. */
    if (0 != (hdr_ptr->extension & VTSP_MASK_EXT_MARKER)) {
        /* Check if its key frame. */
        buf_ptr = rtp_ptr->sendRtpObj.pkt.payload;
        if (NALU_IDR == H264_READ_NALU(buf_ptr[0])) {
            key = 1;
        }
        else if (NALU_FU_A == H264_READ_NALU(buf_ptr[0])) {
            if (NALU_IDR == H264_READ_NALU(buf_ptr[1])) {
                key = 1;
            }
        }

        /* If last packed of key frame. Add Hdr Extension. */
        if (key == 1) {
            hdr_ptr->extension |= VTSP_MASK_EXT_RTP_EXTN;
            /* Add 8 bytes of hdr extn to payload. */
            OSAL_memMove((char *)buf_ptr + 8, (char *)buf_ptr, H264_ENC_MAX_RTP_SIZE);
            buf_ptr[0] = 0xBE;         // fixed hdr part 1
            buf_ptr[1] = 0xDE;         // fixed hdr part 2
            buf_ptr[2] = 0x00;         // length part 1
            buf_ptr[3] = 0x01;         // length part 2
            buf_ptr[4] = (0xF0) & (extmapId << 4); //Four bits Id and Len = 0
            buf_ptr[5] = rcsRtpExtnPayload;     // payload 1 byte
            buf_ptr[6] = 0x00;         // 0 pad
            buf_ptr[7] = 0x00;         // 0 pad
        }
    }
}

/*
 * ======== _VC_videoStreamSendEncodedData() ========
 *
 * Sends the H264 Encoded Data
 *
 */
vint _VC_videoStreamSendEncodedData(
    _VC_Obj *vc_ptr,
    uint8   *data_ptr,
    vint     length,
    uint64   tsMs,
    uint8    rcsRtpExtnPayload)
{
    VTSP_BlockHeader  blockHeader;
    _VC_Dsp          *dsp_ptr;
    _VC_NetObj       *net_ptr;
    _VC_RtpObject    *rtp_ptr;
    vint              encType;
    Video_Packet      pkt;
    _VC_StreamObj    *stream_ptr;
    vint              infc;
    vint              streamId;
    uint64            previousTsMs;
    OSAL_SelectTimeval timeval;

    dsp_ptr = vc_ptr->dsp_ptr;
    for (infc = VTSP_INFC_VIDEO; infc == VTSP_INFC_VIDEO; infc++) {
        /*
         * Do for all streams on infc.
         */
        for (streamId = _VTSP_STREAM_PER_INFC - 1; streamId >= 0; streamId--) {
            stream_ptr = _VC_streamIdToStreamPtr(dsp_ptr, streamId);

            /*
             * For the stream, sum all active peer streams, and
             * put the sum in a dsp_ptr->confBuffer (a stack buffer) 
             * for encoding to that stream.
             */

            /*
             * Check for SEND stream direction.
             */
            if ((VTSP_STREAM_DIR_SENDONLY != stream_ptr->streamParam.dir) &&
                    (VTSP_STREAM_DIR_SENDRECV != stream_ptr->streamParam.dir)) {
                /*
                 * Not sending; skip encode
                 *
                 * Should never get here
                 */
                DBG("");
                continue;
            }

            encType = stream_ptr->streamParam.encoder;
            net_ptr = vc_ptr->net_ptr;

            if ((stream_ptr->streamParam.encodeType[encType] 
                    != VTSP_CODER_UNAVAIL)) {
                /*
                 * Get Data
                 */
                rtp_ptr = _VC_streamIdToRtpPtr(net_ptr, stream_ptr->streamParam.streamId);

                pkt.buf_ptr = rtp_ptr->sendRtpObj.pkt.payload;

                DBG("");

                /* Calculate the Video RTP time stamp. */
                previousTsMs = rtp_ptr->tsMs;
                DBG("previousTsMs:%llu", previousTsMs);
                if (FRAME_SPS_PPS_SIZE == length) {
                    /*
                     * For the first frame, Use the randomly initialized rtpTime.
                     * Also record the OSAL Time so that we can calculate the time elapsed
                     * when sending RTCP SR. This is approximately the sending time.
                     * NOTE: For improved accuracy, rtp_ptr->firstRtpTimeMs value should be adjusted by calculating
                     * CameraFrameTime-to-Send path latency and subtracting it.
                     */
                    OSAL_selectGetTime(&timeval);
                    rtp_ptr->info.firstRtpTimeMs = (timeval.sec * 1000) + (timeval.usec / 1000);
                }
                else {
                    /*
                     * For subsequent frame, calculate the Video RTP sample increment as:
                     * Video RTP Timestamp Increment = 90 KHz * (Time duration between frames)
                     * -- If the Time duration between frames is in milliseconds, simply multiply by 90
                     */
                    rtp_ptr->rtpTime += ((tsMs - previousTsMs) * _VC_VIDEO_CLOCK_RATE_IN_KHZ);
                }
                /* Update the local Video RTP Time stamp in Milliseconds. */
                rtp_ptr->tsMs = tsMs;
                DBG("currentTsMs:%llu", tsMs);
                while (_VC_videoGetCodedData(stream_ptr, &pkt, data_ptr, length, tsMs, encType)) {
                    /*
                     * Fill in BlockHeader here
                     */
                    stream_ptr->enc.marker = pkt.mark;
                    blockHeader.extension = 0;
                    _VC_setBlockHdr(stream_ptr, &blockHeader, encType);
                    /*Set Header Extension if we have a valid extmap id. */
                    if (stream_ptr->streamParam.extmapId > 0) {
                        _VC_setHdrExtension(rtp_ptr, &blockHeader, rcsRtpExtnPayload,
                                stream_ptr->streamParam.extmapId);
                    }
                    blockHeader.dynamicCoder = _VC_localToDynamicEncoder(
                            &stream_ptr->streamParam, encType);
                    blockHeader.localCoder = encType;
                    blockHeader.infc         = infc;
                    blockHeader.streamId     = stream_ptr->streamParam.streamId;
                    rtp_ptr->payloadOffset = pkt.sz;
                    if (VTSP_MASK_EXT_RTP_EXTN == (blockHeader.extension & VTSP_MASK_EXT_RTP_EXTN)) {
                        rtp_ptr->payloadOffset += 8;
                    } 

                    /*
                     * Call _VC_rtpSend().
                     *
                     * Call _VC_rtpSend() to transmit data to network.
                     * The block data is in network byte order.
                     */
                    _VC_rtpSend(vc_ptr, rtp_ptr, &blockHeader, NULL, 0, 0);
                }
            }
        }
    }
    return (_VC_OK);
}

/*
 * ======== _VC_videoStreamSendFir() ========
 *
 * Sends a FIR RTCP Feedback packet
 *
 */
vint _VC_videoStreamSendFir(
    _VC_Obj    *vc_ptr)
{
    _VC_RtcpCmdMsg    message;
    _VC_StreamObj     *stream_ptr;
    int16              streamId;

    /*
     * Do for all streams on infc.
     */
    for (streamId = _VTSP_STREAM_PER_INFC - 1; streamId >= 0; streamId--) {
        stream_ptr = _VC_streamIdToStreamPtr(vc_ptr->dsp_ptr, streamId);

        if (stream_ptr->rtcpEnable) {
            message.cmd  = _VC_RTCP_CMD_SEND_RTCP_FB;
            message.streamId     = streamId;
            message.feedbackMask = VTSP_MASK_RTCP_FB_FIR;
            _VC_sendRtcpCommand(vc_ptr->q_ptr, &message);
        }
    }
    return (_VC_OK);
}

/*
 * ======== _VC_populateFlags() ========
 *
 * Identify the nal type of frames in the data buffer.
 */
void _VC_populateFlags(
    uint8 *data,
    vint   pSize,
    vint  *flags_ptr)
{
    uint8   nalu;
    int     offset = 0;

    /* There could be some frames in one data buffer. For example, 
     * it could contains SPS and PPS in one data buffer.
     * But should only contain one frame in buffer for 
     * I-Frame / P-Frame / B-Frame.
     */
    while(offset < pSize) {
        /* For H.264 raw format, every NALU should begin with 00 00 00 01 */
        if ((*(uint8 *) (data + offset + 0) == 0x00) &&
            (*(uint8 *) (data + offset + 1) == 0x00) &&
            (*(uint8 *) (data + offset + 2) == 0x00) &&
            (*(uint8 *) (data + offset + 3) == 0x01)) {

            /* Set Flags. */
            nalu = (*(uint8 *) (data + offset + 4)) & 0x1F;
            DBG("nal:%d\n", nalu);

            if (NALU_SPS == nalu || NALU_PPS == nalu) {
                /* If we are processing nalu 7 or nalu 8, there may remain some frames in this data,
                 * we should keep looking for nalu.
                 */

                /* SPS/PPS - append flag BUFFER_FLAG_CODEC_CONFIG. */
                *flags_ptr |= BUFFER_FLAG_CODEC_CONFIG;
                DBG("");
            }
            else if (NALU_IDR == nalu) {
                /* If we are processing I-Frame, there should be no any other frames in this data.
                 * Break the loop after I-Frame. The loop will only run once.
                 */
                /* I Frame - append flag  BUFFER_FLAG_SYNC_FRAME . */
                *flags_ptr |= BUFFER_FLAG_SYNC_FRAME;
                DBG("");
                break;
            }
            else {
                /* For any other kind of frame, there should be no other frames in this buffer.
                 * Break the loop. The loop will only run once.
                 */
                break;
            }
                offset += 4;
        }
        else {
            offset ++;
        }
    }
}

/*
 * ======== _VC_videoStreamGetDataToDecode() ========
 *
 * gets the H264 Encoded Data from jitter buffer
 */
vint _VC_videoStreamGetDataToDecode(
    _VC_Obj *vc_ptr,
    uint8  **data_ptr,
    vint    *length_ptr,
    uint64  *tsMs_ptr,
    vint    *flags_ptr,
    uint8   *rcsRtpExtnPayload_ptr)
{

    _VC_Queues         *q_ptr;
    _VC_Dsp            *dsp_ptr;
    _VC_StreamObj      *stream_ptr;
    JBV_RtcpInfo       *rtcpInfo_ptr;
    uint32              streamMask;
    vint                streamId;
    vint                pSize;
    uint8              *data;
    vint                ret;
    OSAL_SelectTimeval  tv;

    q_ptr = vc_ptr->q_ptr;
    dsp_ptr = vc_ptr->dsp_ptr;

    /* Set defaults. */
    *length_ptr = 0;
    *flags_ptr = 0;
    *tsMs_ptr = 0;
    ret = _VC_ERROR;

    /* Do for all streams. */
    for (streamId = _VTSP_STREAM_PER_INFC - 1; streamId >= 0; streamId--) {
        stream_ptr = _VC_streamIdToStreamPtr(dsp_ptr, streamId);
        streamMask = stream_ptr->algStreamState;

        if ((VTSP_STREAM_DIR_RECVONLY != stream_ptr->streamParam.dir)
                && (VTSP_STREAM_DIR_SENDRECV != stream_ptr->streamParam.dir)) {
            /*
             * Skip stream if not receiving.
             */
            continue;
        }
        if (0 != (_VC_ALG_STREAM_JB & streamMask)) {
            OSAL_selectGetTime(&tv);
            JBV_getPkt(&stream_ptr->dec.jbObj, &dsp_ptr->jbGetPkt, (JBV_Timeval *)&tv);
            DBG("");

            /*
             * After adding JB, move the source pointer from RTP.
             */
            if ((0 == dsp_ptr->jbGetPkt.valid) || (0 == dsp_ptr->jbGetPkt.pSize)) {
                continue;
            }
            DBG("");
            pSize = dsp_ptr->jbGetPkt.pSize;
        }
        else {
            DBG("");
            continue;
        }

        /*
         * Decode far end data and place in proper stream array.
         * Get a pointer decOut_ptr for writing data into audio
         */
        if (pSize > 0) {
            if (dsp_ptr->jbGetPkt.seqn % 50 == 0) {
                /* Notify video RTP timestamp once for every 50 packets. */
                DBG("VC notify video RTP timestamp:%u\n", dsp_ptr->jbGetPkt.tsOrig);
                _VC_LipSync_rtpTs(streamId, q_ptr, &dsp_ptr->jbGetPkt);
            }

            data = OSAL_memAlloc(pSize, 0);
            OSAL_memCpy(data, dsp_ptr->jbGetPkt.data_ptr, pSize);

            if (NALU_SEI == H264_READ_NALU(*data)) {
                DBG("");
                return (_VC_ERROR);
            }

            /* Get RCS RTP Extn payload. */
            *rcsRtpExtnPayload_ptr = dsp_ptr->jbGetPkt.rcsRtpExtnPayload;
            DBG("payload:%d\n", *rcsRtpExtnPayload_ptr);

            _VC_populateFlags(data, pSize, flags_ptr);
            if ((*flags_ptr)  & BUFFER_FLAG_SYNC_FRAME) {
                rtcpInfo_ptr = &stream_ptr->dec.jbObj.rtcpInfo;
                /* Atomic write is allowed */
                rtcpInfo_ptr->keyFrameRead = OSAL_TRUE;
            }
            /* Assign the data pointer. */
            *data_ptr = data;
            /* Set Length. */
            *length_ptr = pSize;

            *tsMs_ptr = dsp_ptr->jbGetPkt.tsOrig;

            DBG("");
            ret = (_VC_OK);
        }
    }
    return (ret);
}
