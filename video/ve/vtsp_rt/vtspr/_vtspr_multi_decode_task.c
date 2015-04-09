/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2006-2009 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 30103 $ $Date: 2014-11-27 17:00:34 +0800 (Thu, 27 Nov 2014) $
 *
 */        

#include <vtspr.h>
#include <_vtspr_private.h>

#if defined(VTSP_ENABLE_G726_ACCELERATOR) || \
        defined(VTSP_ENABLE_G729_ACCELERATOR)
/* Include if DSP ACCELERATOR is used */
#include <dsp.h>
#endif

#ifdef VTSP_ENABLE_ILBC
static void _VTSPR_format16ToVint(vint *, int16 *, vint);
#endif

/*
 * Enable the following definition in order to replace the GAMRWB decoder output
 * with the tone specified in the toneBuffer array
 */
//#define VTSP_SWAP_GAMRWB_OUTPUT_WITH_TONE


#ifdef VTSP_SWAP_GAMRWB_OUTPUT_WITH_TONE
/*
 * toneBuffer is a table containing a 1000 Hz tone at nominal power:
 * 16,000 samples/sec * 20 ms = 320 total samples.
 */
vint toneBuffer[] = {
    0,  1003,  1854,  2422,  2621,  2422,  1854,  1003,
    0, -1003, -1854, -2422, -2621, -2422, -1854, -1003,
    0,  1003,  1854,  2422,  2621,  2422,  1854,  1003,
    0, -1003, -1854, -2422, -2621, -2422, -1854, -1003,
    0,  1003,  1854,  2422,  2621,  2422,  1854,  1003,
    0, -1003, -1854, -2422, -2621, -2422, -1854, -1003,
    0,  1003,  1854,  2422,  2621,  2422,  1854,  1003,
    0, -1003, -1854, -2422, -2621, -2422, -1854, -1003,
    0,  1003,  1854,  2422,  2621,  2422,  1854,  1003,
    0, -1003, -1854, -2422, -2621, -2422, -1854, -1003,
    0,  1003,  1854,  2422,  2621,  2422,  1854,  1003,
    0, -1003, -1854, -2422, -2621, -2422, -1854, -1003,
    0,  1003,  1854,  2422,  2621,  2422,  1854,  1003,
    0, -1003, -1854, -2422, -2621, -2422, -1854, -1003,
    0,  1003,  1854,  2422,  2621,  2422,  1854,  1003,
    0, -1003, -1854, -2422, -2621, -2422, -1854, -1003,
    0,  1003,  1854,  2422,  2621,  2422,  1854,  1003,
    0, -1003, -1854, -2422, -2621, -2422, -1854, -1003,
    0,  1003,  1854,  2422,  2621,  2422,  1854,  1003,
    0, -1003, -1854, -2422, -2621, -2422, -1854, -1003,
    0,  1003,  1854,  2422,  2621,  2422,  1854,  1003,
    0, -1003, -1854, -2422, -2621, -2422, -1854, -1003,
    0,  1003,  1854,  2422,  2621,  2422,  1854,  1003,
    0, -1003, -1854, -2422, -2621, -2422, -1854, -1003,
    0,  1003,  1854,  2422,  2621,  2422,  1854,  1003,
    0, -1003, -1854, -2422, -2621, -2422, -1854, -1003,
    0,  1003,  1854,  2422,  2621,  2422,  1854,  1003,
    0, -1003, -1854, -2422, -2621, -2422, -1854, -1003,
    0,  1003,  1854,  2422,  2621,  2422,  1854,  1003,
    0, -1003, -1854, -2422, -2621, -2422, -1854, -1003,
    0,  1003,  1854,  2422,  2621,  2422,  1854,  1003,
    0, -1003, -1854, -2422, -2621, -2422, -1854, -1003,
    0,  1003,  1854,  2422,  2621,  2422,  1854,  1003,
    0, -1003, -1854, -2422, -2621, -2422, -1854, -1003,
    0,  1003,  1854,  2422,  2621,  2422,  1854,  1003,
    0, -1003, -1854, -2422, -2621, -2422, -1854, -1003,
    0,  1003,  1854,  2422,  2621,  2422,  1854,  1003,
    0, -1003, -1854, -2422, -2621, -2422, -1854, -1003,
    0,  1003,  1854,  2422,  2621,  2422,  1854,  1003,
    0, -1003, -1854, -2422, -2621, -2422, -1854, -1003
};
#endif

/*
 * ======== _VTSPR_multiDecode30Task() ========
 * The multiDecode task processes all instances of speech decoders who operate
 * on multiple of 10ms frames. The task waits on the single pktData queue,
 * decodes the data, and then sends the decoded data back to the framework
 * using the decData queue (selected by the key).
 */
vint _VTSPR_multiDecode30Task(
    OSAL_TaskArg taskArg)
{
    VTSPR_TaskContext    *task_ptr;
    VTSPR_Queues         *q_ptr;
    vint                  payloadSize;
    vint                  msgSize = 0;
    _VTSPR_MultiPktMsg   *recvMsg_ptr = NULL;
    _VTSPR_MultiDecodedMsg *sendMsg_ptr = NULL;
    uvint                 decType;
    vint                  infc;       /* must be signed */
    vint                  streamId;
    OSAL_MsgQId           decQId;
#if defined(VTSP_ENABLE_G723) || defined(VTSP_ENABLE_ILBC)
    _VTSPR_MultiDecObj   *multiDecObj_ptr;
    uint8                *payload_ptr;
    VTSPR_DSP            *dsp_ptr;
    vint                 *dst_ptr = NULL;
#endif
#ifdef VTSP_ENABLE_G723
    G723A_DecObj         *g723decObj_ptr;
    char                  noPacket[VTSP_BLOCK_MAX_ENC_BYTES_SZ];
#endif  
#ifdef VTSP_ENABLE_ILBC
    ILBC_DecObj          *iLBCdecObj_ptr;
    int16                 speechType;
#endif 

    /*
     * Create generic message structures and objects.
     */
    if (NULL == (recvMsg_ptr =
            (_VTSPR_MultiPktMsg *)
                    (OSAL_memAlloc(VTSPR_Q_MULTI_DECODE_PKT_MSG_SIZE, 0)))) {
        _VTSP_TRACE(__FILE__, __LINE__);
        return (-1);
    }
    if (NULL == (sendMsg_ptr =
            (_VTSPR_MultiDecodedMsg *)
                    (OSAL_memAlloc(VTSPR_Q_MULTI_DECODE_DEC_MSG_SIZE, 0)))) {
        _VTSP_TRACE(__FILE__, __LINE__);
        return (-1);
    }

    /*
     * Assign pointers to the queues, and send/receive messages.
     */
    task_ptr    = (VTSPR_TaskContext *) taskArg;
    q_ptr       = task_ptr->q_ptr;
#if defined(VTSP_ENABLE_G723) || defined(VTSP_ENABLE_ILBC)
    dsp_ptr     = task_ptr->dsp_ptr;
    payload_ptr = recvMsg_ptr->pkt_ary;
    dst_ptr     = sendMsg_ptr->decoded_ary;
#endif

    while (0 != (VTSPR_TASK_RUN & task_ptr->taskEnable)) {
        /*
         * Wait for data from VTSPR task.
         */
        payloadSize = OSAL_msgQRecv(q_ptr->data30Pkt,
                (char *)recvMsg_ptr,
                VTSPR_Q_MULTI_DECODE_PKT_MSG_SIZE, OSAL_WAIT_FOREVER, NULL);
        if (0 >= payloadSize) {
            _VTSP_TRACE(__FILE__, __LINE__);
        }
        
        if (_VTSPR_MULTI_CODER_CMD_SHUTDOWN == recvMsg_ptr->cmd) {
            _VTSP_TRACE(__FILE__, __LINE__);            
            break; /* if receive shutdown signal break while loop */
        }
        else if (_VTSPR_MULTI_CODER_CMD_RUN != recvMsg_ptr->cmd) {
            OSAL_logMsg("%s:%d FATAL ERROR cmd!=_VTSPR_MULTI_CODER_CMD_RUN", 
                    __FUNCTION__, __LINE__);
            continue;
        }

        decType       = recvMsg_ptr->decType;
        infc          = recvMsg_ptr->infc;
        streamId      = recvMsg_ptr->streamId;
        
#if defined(VTSP_ENABLE_G723) || defined(VTSP_ENABLE_ILBC)
        /* get multi-dec object for this stream */
        multiDecObj_ptr = _VTSPR_streamIdToMultiDecPtr(dsp_ptr, infc, streamId);
#endif

        switch (decType) {
#ifdef VTSP_ENABLE_G723
            case VTSP_CODER_G723_30MS:
                g723decObj_ptr = &(multiDecObj_ptr->g723decObj);
                if (1 == recvMsg_ptr->initFlag) {
                    G723A_decodeInit(g723decObj_ptr);
                }
                if (payloadSize != 0) {
                    /*
                     * Run decoder with valid packet to produce 30 ms of speech
                     */
                    g723decObj_ptr->src_ptr = (char *)payload_ptr;
                    g723decObj_ptr->dst_ptr = dst_ptr;
                    g723decObj_ptr->usePf = G723A_USEPF_ENABLE;
                    g723decObj_ptr->crc = G723A_CRC_NORMAL;
                    G723A_decode(g723decObj_ptr);
                    multiDecObj_ptr->plcActive = 0;
                    multiDecObj_ptr->lastFrameSize = payloadSize;
                }
                else {
                    /*
                     * Run decoder with packet loss, or untransmitted frame:
                     * - packet loss if last frame was size > 4 (voiced frame)
                     * - else, current frame is untransmitted, 0x3
                     *
                     * The decoder always produces 30ms of speech.
                     */
                    noPacket[0] = 0x3;
                    g723decObj_ptr->src_ptr = noPacket;
                    g723decObj_ptr->dst_ptr = dsp_ptr;
                    g723decObj_ptr->usePf = G723A_USEPF_ENABLE;
                    g723decObj_ptr->crc = G723A_CRC_PACKETLOSS;
                    if (VTSP_BLOCK_G723_SID_SZ < multiDecObj_ptr->lastFrameSize)
                    {
                        multiDecObj_ptr->plcActive = 1;
                    }
                    else {
                        multiDecObj_ptr->plcActive = 0;
                    }
                    G723A_decode(g723decObj_ptr);
                }
                /*
                 * Convert Q15 speech to Q13 speech, needed by the framework
                 */
                COMM_shiftRight(g723decObj_ptr->dst_ptr, 
                        g723decObj_ptr->dst_ptr, 2,
                        (3 * VTSPR_NSAMPLES_10MS_8K));
                msgSize = G723A_FRAME;
                break;
#endif
#ifdef VTSP_ENABLE_ILBC
            case VTSP_CODER_ILBC_30MS:
                iLBCdecObj_ptr = &(multiDecObj_ptr->iLBCdecObj);
                if (1 == recvMsg_ptr->initFlag) {
                    /* Initialize decoder */
                    ILBC_decoderinit(iLBCdecObj_ptr, ILBC_MODE_30MS);
                }
                if (payloadSize != 0) {
                    /*
                     * Run decoder. 
                     */
                    ILBC_decode(iLBCdecObj_ptr, (int16 *)payload_ptr,
                            VTSP_BLOCK_ILBC_30MS_SZ, (int16 *)dst_ptr,
                            &speechType);
                    multiDecObj_ptr->plcActive = 0;
                }
                else {
                    /*
                     * Packet loss
                     */
                    ILBC_decodePLC(iLBCdecObj_ptr, (int16 *)(dst_ptr), 1);
                    multiDecObj_ptr->plcActive = 1;
                }
                /*
                 * Since both decode methods produce int16 vs vint samples.
                 * Unpack the iLBC data into vint payloads.
                 */
                _VTSPR_format16ToVint(dst_ptr, (int16 *)dst_ptr,
                        ILBC_BLOCK_LENGTH_30MS);
                msgSize = ILBC_BLOCK_LENGTH_30MS;
                break;
#endif
            default:;
            /* XXX should we get here? XXX */
        }    
        sendMsg_ptr->msgSize = msgSize;

        /*
         * Send the data back to the application.
         */
        decQId = _VTSPR_streamIdToMultiDecQId(q_ptr, infc, streamId,
                _VTSPR_MULTI_CODER_PKT_RATE_30);
        if (OSAL_SUCCESS != OSAL_msgQSend(decQId, (char *)sendMsg_ptr,
                    VTSPR_Q_MULTI_DECODE_DEC_MSG_SIZE, OSAL_WAIT_FOREVER, NULL))
        {
            _VTSP_TRACE(__FILE__, __LINE__);
        }
    }
    /*
     * Mark the Multi-frame Decoded task as finished.
     */
    task_ptr->taskEnable = VTSPR_TASK_FINISHED;

    /*
     * Free allocated memory.
     */
    if (NULL != recvMsg_ptr) {
        if (OSAL_SUCCESS != OSAL_memFree(recvMsg_ptr, 0)) {
            _VTSP_TRACE(__FILE__, __LINE__);
        }
    }
    if (NULL != sendMsg_ptr) {
        if (OSAL_SUCCESS != OSAL_memFree(sendMsg_ptr, 0)) {
            _VTSP_TRACE(__FILE__, __LINE__);
        }
    }

    /*
     * Signal that ILBC has completed its cleanup.
     */
    OSAL_semGive(task_ptr->finishSemId);

    return (0);
}

/*
 * ======== _VTSPR_multiDecode20Task() ========
 * The multiDecode task processes all instances of speech decoders who operate
 * on multiple of 10ms frames. The task waits on the single pktData queue, 
 * decodes the data, and then sends the decoded data back to the framework using
 * the decData queue (selected by the key).
 */
vint _VTSPR_multiDecode20Task(
    OSAL_TaskArg taskArg)
{
    VTSPR_TaskContext    *task_ptr;
    VTSPR_Queues         *q_ptr;
    vint                  msgSize = 0;
    _VTSPR_MultiPktMsg   *recvMsg_ptr;
    _VTSPR_MultiDecodedMsg *sendMsg_ptr;
    uvint                 decType;
    vint                  infc;       /* must be signed */
    vint                  streamId;
    OSAL_MsgQId           decQId;
#ifdef VTSP_ENABLE_ILBC
    ILBC_DecObj          *iLBCdecObj_ptr;
    int16                 speechType;
#endif
#ifdef VTSP_ENABLE_SILK
    SILK_DecObj          *silkDecObj_ptr;
#endif 
#ifdef VTSP_ENABLE_G722P1
    G722P1_DecObj        *g722p1decObj_ptr;
    vint                  bitRate;
    uint8                *g722p1Scratch_ptr;
#endif
#ifdef VTSP_ENABLE_GAMRNB
    GAMRNB_DecObj        *gamrnbDecObj_ptr;
#endif
#ifdef VTSP_ENABLE_GAMRWB
    GAMRWB_DecObj        *gamrwbDecObj_ptr;
#endif
#if defined(VTSP_ENABLE_GAMRNB) || defined(VTSP_ENABLE_GAMRWB) || \
        defined(VTSP_ENABLE_SILK) || defined(VTSP_ENABLE_ILBC) || \
        defined(VTSP_ENABLE_G722P1)
    VTSPR_DSP            *dsp_ptr;
    _VTSPR_MultiDecObj   *multiDecObj_ptr;
    char                  noPacket[VTSP_BLOCK_MAX_ENC_BYTES_SZ];
    vint                  errorFlag;
    vint                 *dst_ptr = NULL;
    vint                  payloadSize;
    uint8                *payload_ptr;
    vint                  dataFormat;
    //char                  sidDecoded[VTSPR_MULTI_NSAMPLE_MAX];
#endif
#ifdef VTSP_ENABLE_BENCHMARK
    VTSPR_StreamObj      *stream_ptr;
#endif

    /*
     * Create generic message structures and objects.
     */
    if (NULL == (recvMsg_ptr =
            (_VTSPR_MultiPktMsg *)
                    (OSAL_memAlloc(VTSPR_Q_MULTI_DECODE_PKT_MSG_SIZE, 0)))) {
        _VTSP_TRACE(__FILE__, __LINE__);
        return (-1);
    }
    if (NULL == (sendMsg_ptr =
            (_VTSPR_MultiDecodedMsg *)
                    (OSAL_memAlloc(VTSPR_Q_MULTI_DECODE_DEC_MSG_SIZE, 0)))) {
        _VTSP_TRACE(__FILE__, __LINE__);
        return (-1);
    }

    /*
     * Assign pointers to the queues, and send/receive messages.
     */
    task_ptr    = (VTSPR_TaskContext *) taskArg;
    q_ptr       = task_ptr->q_ptr;
#if defined(VTSP_ENABLE_GAMRNB) || defined(VTSP_ENABLE_GAMRWB) || \
        defined(VTSP_ENABLE_SILK) || defined(VTSP_ENABLE_ILBC) || \
        defined(VTSP_ENABLE_G722P1)
    dsp_ptr     = task_ptr->dsp_ptr;
    payload_ptr = recvMsg_ptr->pkt_ary;
    dst_ptr     = sendMsg_ptr->decoded_ary;
#endif

    while (0 != (VTSPR_TASK_RUN & task_ptr->taskEnable)) {
        /*
         * Wait for data from VTSPR task.
         */
        if (0 >= OSAL_msgQRecv(q_ptr->data20Pkt, 
                (char *)recvMsg_ptr, 
                VTSPR_Q_MULTI_DECODE_PKT_MSG_SIZE, OSAL_WAIT_FOREVER, NULL)) {
            _VTSP_TRACE(__FILE__, __LINE__);
        }

        if (_VTSPR_MULTI_CODER_CMD_SHUTDOWN == recvMsg_ptr->cmd) {
            _VTSP_TRACE(__FILE__, __LINE__);            
            break; /* if receive shutdown signal break while loop */
        }
        else if (_VTSPR_MULTI_CODER_CMD_RUN != recvMsg_ptr->cmd) {
            OSAL_logMsg("%s:%d FATAL ERROR cmd!=_VTSPR_MULTI_CODER_CMD_RUN", 
                    __FUNCTION__, __LINE__);
            continue;
        }

        decType   = recvMsg_ptr->decType;
        infc      = recvMsg_ptr->infc;
        streamId  = recvMsg_ptr->streamId;

#if defined(VTSP_ENABLE_GAMRNB) || defined(VTSP_ENABLE_GAMRWB) || \
        defined(VTSP_ENABLE_SILK) || defined(VTSP_ENABLE_ILBC) || \
        defined(VTSP_ENABLE_G722P1)
        payloadSize     = recvMsg_ptr->payloadSize;
        /* get multi-dec object for this stream */
        multiDecObj_ptr = _VTSPR_streamIdToMultiDecPtr(dsp_ptr, infc, streamId);
#ifdef VTSP_ENABLE_BENCHMARK
        stream_ptr = _VTSPR_streamIdToStreamPtr(dsp_ptr, infc, streamId);
#endif
#endif

        switch (decType) {
#ifdef VTSP_ENABLE_ILBC
            case VTSP_CODER_ILBC_20MS:
                iLBCdecObj_ptr = &(multiDecObj_ptr->iLBCdecObj);
                if (1 == recvMsg_ptr->initFlag) {
                    /* Initialize decoder */
                    ILBC_decoderinit(iLBCdecObj_ptr, ILBC_MODE_20MS);
                }
                payload_ptr = multiDecObj_ptr->pktMsg.pkt_ary;
                if (payloadSize != 0) {
                    /*
                     * Run decoder. 
                     */
                    ILBC_decode(iLBCdecObj_ptr, (int16 *)payload_ptr,
                            VTSP_BLOCK_ILBC_20MS_SZ,
                            (int16 *)dst_ptr, &speechType);
                    multiDecObj_ptr->plcActive = 0;
                }
                else {
                    /*
                     * Packet loss
                     */
                    ILBC_decodePLC(iLBCdecObj_ptr, (int16 *)(dst_ptr), 1);
                    multiDecObj_ptr->plcActive = 1;
                }
                /*
                 * Since both decode methods produce int16 vs vint samples.
                 * Unpack the iLBC data into vint payloads.
                 */
                _VTSPR_format16ToVint(dst_ptr, (int16 *)dst_ptr,
                        ILBC_BLOCK_LENGTH_20MS);
                msgSize = ILBC_BLOCK_LENGTH_20MS;
                break;
#endif
#ifdef VTSP_ENABLE_G722P1
            case VTSP_CODER_G722P1_20MS:
                g722p1Scratch_ptr = dsp_ptr->g722p1DecScratch_ary;
                g722p1decObj_ptr = &(multiDecObj_ptr->g722p1decObj);
                g722p1decObj_ptr->scratch_ptr = (void *)g722p1Scratch_ptr;
                if (payloadSize != 0) {
                    /* Check if bitrate has changed */
                    if (payloadSize != multiDecObj_ptr->lastFrameSize) {
                        /* re-init to new rate */
                        if (VTSP_BLOCK_G722P1_24KBPS_20MS_SZ == payloadSize) {
                            bitRate = G722P1_BIT_RATE_24KBPS;
                        }
                        else {
                            bitRate = G722P1_BIT_RATE_32KBPS;
                        }
                        G722P1_decodeInit(g722p1decObj_ptr,
                                bitRate, G722P1_SAMPLE_RATE_16KHZ);
                    }
                    multiDecObj_ptr->plcActive = G722P1_ERRORFLAG_PACKET_OK;
                    multiDecObj_ptr->lastFrameSize = payloadSize;

                    G722P1_decode(g722p1decObj_ptr,
                            (uint8 *)payload_ptr, dst_ptr,
                            payloadSize, G722P1_ERRORFLAG_PACKET_OK);
                }
                else {
                    multiDecObj_ptr->plcActive = G722P1_ERRORFLAG_PACKET_LOSS;
                    /* XXX need to test code for handling packet loss XXX */
                    if (NULL == payload_ptr) {
                        payload_ptr = (uint8 *)noPacket;
                    }
                    G722P1_decode(g722p1decObj_ptr,
                            (uint8 *)payload_ptr, dst_ptr,
                            payloadSize, G722P1_ERRORFLAG_PACKET_LOSS);
                }

                /*
                 * Convert Q15 speech to Q13 speech, needed by the framework
                 */
                COMM_shiftRight(dst_ptr, dst_ptr, 2,
                        (2 * VTSPR_NSAMPLES_10MS_16K));
                msgSize = G722P1_SPEECH_BUFSZ_16KHZ;
                break;
#endif
#ifdef VTSP_ENABLE_GAMRNB
            case VTSP_CODER_GAMRNB_20MS_OA:
            case VTSP_CODER_GAMRNB_20MS_BE:
                gamrnbDecObj_ptr = &(multiDecObj_ptr->gamrnbDecObj);
                if (1 == recvMsg_ptr->initFlag) {
                    if (VTSP_CODER_GAMRNB_20MS_OA == decType) {
                        dataFormat = GAMRNB_DATA_FORMAT_PACKED_OA;
                    }
                    else {
                        dataFormat = GAMRNB_DATA_FORMAT_PACKED_BE;
                    }
                    GAMRNB_decodeInit(gamrnbDecObj_ptr,
                            GAMRNB_SPEECH_FORMAT_LINEAR,
                            dataFormat,
                            GAMRNB_BIT_MASK_13BIT);
                }

                if (payloadSize != 0) {
                    /*
                     * Run decoder with valid packet to produce 20 ms of speech.
                     */
                    multiDecObj_ptr->plcActive = GAMRNB_ERRORFLAG_PACKET_OK;
                    multiDecObj_ptr->lastFrameSize = payloadSize;
                    GAMRNB_decode(gamrnbDecObj_ptr, (uint8 *)payload_ptr,
                            dst_ptr, GAMRNB_ERRORFLAG_PACKET_OK);
                }
                else {
                    /*
                     * Run decoder with packet loss, or untransmitted frame:
                     * - packet loss if last frame was size > 4 (voiced frame)
                     * - else, current frame is untransmitted, 0x3
                     *
                     */
                    noPacket[0] = 0xFF; /* this byte is inconsequential */
                    noPacket[1] = 0xFF; /* this is the byte the decoder will look at */
                    if (NULL == payload_ptr) {
                        payload_ptr = (uint8*)noPacket;
                    }
                    /* If last fram not DTX (SID), then we have loss */
                    if (VTSP_BLOCK_GAMRNB_20MS_OA_MRDTX_SZ <
                            multiDecObj_ptr->lastFrameSize) {
                        multiDecObj_ptr->plcActive = 1;
                        errorFlag = GAMRNB_ERRORFLAG_PACKET_LOSS;
                    }
                    else {
                        multiDecObj_ptr->plcActive = 0;
                        errorFlag = GAMRNB_ERRORFLAG_PACKET_OK;
                    }
                    GAMRNB_decode(gamrnbDecObj_ptr, (uint8 *)payload_ptr,
                            dst_ptr, errorFlag);
                }

                /*
                 * Convert Q15 speech to Q13 speech, needed by the framework
                 */
                COMM_shiftRight(dst_ptr, dst_ptr, 2, 
                        2 * VTSPR_NSAMPLES_10MS_8K);
                msgSize = 2 * VTSPR_NSAMPLES_10MS_8K;
                break;
#endif
#ifdef VTSP_ENABLE_GAMRWB
            case VTSP_CODER_GAMRWB_20MS_OA:
            case VTSP_CODER_GAMRWB_20MS_BE:
                gamrwbDecObj_ptr = &(multiDecObj_ptr->gamrwbDecObj);
                if (1 == recvMsg_ptr->initFlag) {
                    if (VTSP_CODER_GAMRWB_20MS_OA == decType) {
                        dataFormat = GAMRWB_DATA_FORMAT_PACKED_IF1;
                    }
                    else {
                         dataFormat = GAMRWB_DATA_FORMAT_PACKED_IF2;
                    }
                    GAMRWB_decoderInit(gamrwbDecObj_ptr, 
                            dataFormat);
                }
                if (payloadSize != 0) {
                    /*
                     * Run decoder with valid packet to produce 
                     * 20 ms of speech.
                     */
                    multiDecObj_ptr->plcActive = GAMRWB_ERRORFLAG_PACKET_OK;
                    multiDecObj_ptr->lastFrameSize = payloadSize;

                    /*
                     * XXX Note that 'adaptive' codec rates are not supported.
                     * It should be handled in GAMRNB eventually.
                     *
                     * Ignore payload header in first byte since 'adaptive'
                     * rates are not supported.
                     */
#ifdef VTSP_ENABLE_BENCHMARK
                    if (0 == stream_ptr->streamParam.streamId) {
                        _VTSPR_benchmarkStart(&VTSPR_obj,
                                _VTSPR_BENCHMARK_GAMRWB_DECODE_ID0, 0);
                    }
                    else {
                        _VTSPR_benchmarkStart(&VTSPR_obj,
                                _VTSPR_BENCHMARK_GAMRWB_DECODE_ID1, 0);
                    }
#endif

                    GAMRWB_decode(gamrwbDecObj_ptr, (uint8 *)payload_ptr,
                            dst_ptr, GAMRWB_ERRORFLAG_PACKET_OK);

                    /* save the sid decoded buffer. */
                    if (VTSP_BLOCK_GAMRWB_20MS_OA_MRDTX_SZ == payloadSize) {
                        OSAL_memCpy(multiDecObj_ptr->sidDecoded, dst_ptr,
                                2 * VTSPR_NSAMPLES_10MS_16K);
                    }
#ifdef VTSP_ENABLE_BENCHMARK
                    if (0 == stream_ptr->streamParam.streamId) {
                        _VTSPR_benchmarkStop(&VTSPR_obj,
                                _VTSPR_BENCHMARK_GAMRWB_DECODE_ID0, 0);
                    }
                    else {
                        _VTSPR_benchmarkStop(&VTSPR_obj,
                                _VTSPR_BENCHMARK_GAMRWB_DECODE_ID1, 0);
                    }
#endif
                }
                else {
                    /* For bug 14424, if gets silence packets, 
                     * GAMRWB would replay decoded silence PCM to 
                     * replace noData.
                     * This can resolve the explosion sounds problem
                     * and bi-bi-bi sounds.
                     */

                    if (VTSP_BLOCK_GAMRWB_20MS_OA_MRDTX_SZ <
                            multiDecObj_ptr->lastFrameSize) {
                        multiDecObj_ptr->plcActive = 1;
                        msgSize = 0;
                        break;
                    }
                    else {
                        /*
                         * copy the decoded SID buffer as dst_ptr.
                         */
                        OSAL_memCpy(dst_ptr, multiDecObj_ptr->sidDecoded,
                                2 * VTSPR_NSAMPLES_10MS_16K);
                        multiDecObj_ptr->plcActive = 0;
                    }
                }

                /*
                 * Convert Q15 speech to Q13 speech, needed by the framework
                 */
                COMM_shiftRight(dst_ptr, dst_ptr, 2, 
                        2 * VTSPR_NSAMPLES_10MS_16K);
                msgSize = 2 * VTSPR_NSAMPLES_10MS_16K;

#ifdef VTSP_SWAP_GAMRWB_OUTPUT_WITH_TONE
                COMM_copy(dst_ptr, toneBuffer, 2 * VTSPR_NSAMPLES_10MS_16K);
#endif
                break;
#endif
#ifdef VTSP_ENABLE_SILK
            case VTSP_CODER_SILK_20MS_8K:
            case VTSP_CODER_SILK_20MS_16K:
            case VTSP_CODER_SILK_20MS_24K:
                silkDecObj_ptr = &(multiDecObj_ptr->silkDecObj);
                if (1 == recvMsg_ptr->initFlag) {
                    /* Initialize decoder */
                    if (VTSP_CODER_SILK_20MS_8K == decType) {
                        SILK_decodeInit(silkDecObj_ptr, SILK_SAMPLE_RATE_8000);
                    }
                    else if (VTSP_CODER_SILK_20MS_16K == decType) {
                        SILK_decodeInit(silkDecObj_ptr, SILK_SAMPLE_RATE_16000);
                    }
                    else {
//                      SILK_decodeInit(silkDecObj_ptr, SILK_SAMPLE_RATE_24000);
                        _VTSP_TRACE(__FILE__, __LINE__);
                        break;
                    }
                }
                if (payloadSize != 0) {
                    multiDecObj_ptr->plcActive = SILK_ERRORFLAG_PACKET_OK;
                    multiDecObj_ptr->lastFrameSize = payloadSize;
                }
                else {
                    multiDecObj_ptr->plcActive = SILK_ERRORFLAG_PACKET_LOSS;
                    /* XXX need to test code for handling packet loss XXX */
                    if (NULL == payload_ptr) {
                        payload_ptr = (uint8 *)noPacket;
                    }
                }
                msgSize = SILK_decode(silkDecObj_ptr, payload_ptr, 
                        (int16 *)dst_ptr, payloadSize, 
                        multiDecObj_ptr->plcActive);

                _VTSPR_format16ToVint(dst_ptr, (int16 *)dst_ptr, msgSize);
                /*
                 * Convert Q15 speech to Q13 speech, needed by the framework
                 */
                COMM_shiftRight(dst_ptr, dst_ptr, 2, msgSize);
                break;
#endif
            default:;
                _VTSP_TRACE(__FILE__, __LINE__);
        }    
        sendMsg_ptr->msgSize = msgSize;

        /* Send the data back to the application */
        decQId = _VTSPR_streamIdToMultiDecQId(q_ptr, infc, streamId,
                _VTSPR_MULTI_CODER_PKT_RATE_20);
        if (OSAL_SUCCESS != OSAL_msgQSend(decQId, (char *)sendMsg_ptr,
                    VTSPR_Q_MULTI_DECODE_DEC_MSG_SIZE, OSAL_WAIT_FOREVER, NULL))
        {
            _VTSP_TRACE(__FILE__, __LINE__);
        }
    }
    /*
     * Make the Multi-frame Decoded task as finished.
     */
    task_ptr->taskEnable = VTSPR_TASK_FINISHED;

    /*
     * Free allocated memory.
     */
    if (NULL != recvMsg_ptr) {
        if (OSAL_SUCCESS != OSAL_memFree(recvMsg_ptr, 0)) {
            _VTSP_TRACE(__FILE__, __LINE__);
        }
    }
    if (NULL != sendMsg_ptr) {
        if (OSAL_SUCCESS != OSAL_memFree(sendMsg_ptr, 0)) {
            _VTSP_TRACE(__FILE__, __LINE__);
        }
    }
    /*
     * Signal that 20ms task has completed its cleanup.
     */
    OSAL_semGive(task_ptr->finishSemId);

    return (0);
}

/*
 * ======== _VTSPR_multiDecode10Task() ========
 * The multiDecode task processes all instances of speech decoders who operate
 * on 10ms task frames. The task waits on the single pktData queue, decodes the
 * data, and then sends the decoded data back to the framework using the 
 * decData queue (selected by the key).
 */
vint _VTSPR_multiDecode10Task(
    OSAL_TaskArg taskArg)
{
    VTSPR_TaskContext      *task_ptr;
    VTSPR_Queues           *q_ptr;
    _VTSPR_MultiPktMsg     *recvMsg_ptr;
    _VTSPR_MultiDecodedMsg *sendMsg_ptr;
    uvint                   decType;
    vint                    infc;       /* must be signed */
    vint                    streamId;
    OSAL_MsgQId             decQId;
    uvint                   speechBufSize = 0;
#if defined(VTSP_ENABLE_G729_ACCELERATOR) || \
    defined(VTSP_ENABLE_G726_ACCELERATOR)
    uint32                  dspInitParam;
    VTSPR_DSP              *dsp_ptr;
    vint                    payloadSize;
    uint8                  *payload_ptr;
    vint                   *dst_ptr;
    _VTSPR_MultiDecObj     *multiDecObj_ptr; 
    VTSPR_StreamObj        *stream_ptr;
    DSP_Instance            retInstance;
#endif

    /*
     * Create generic message structures and objects.
     */
    if (NULL == (recvMsg_ptr =
            (_VTSPR_MultiPktMsg *)
                    (OSAL_memAlloc(VTSPR_Q_MULTI_DECODE_PKT_MSG_SIZE, 0)))) {
        _VTSP_TRACE(__FILE__, __LINE__);
        return (-1);
    }
    if (NULL == (sendMsg_ptr =
            (_VTSPR_MultiDecodedMsg *)
                    (OSAL_memAlloc(VTSPR_Q_MULTI_DECODE_DEC_MSG_SIZE, 0)))) {
        _VTSP_TRACE(__FILE__, __LINE__);
        return (-1);
    }

    /*
     * Assign pointers to the queues, and send/receive messages.
     */
    task_ptr    = (VTSPR_TaskContext *) taskArg;
    q_ptr       = task_ptr->q_ptr;
#if defined(VTSP_ENABLE_G729_ACCELERATOR) || \
    defined(VTSP_ENABLE_G726_ACCELERATOR) 
    dsp_ptr     = task_ptr->dsp_ptr;
    payload_ptr = recvMsg_ptr->pkt_ary;
    dst_ptr     = sendMsg_ptr->decoded_ary;
#endif
   
    while (0 != (VTSPR_TASK_RUN & task_ptr->taskEnable)) {
        /*
         * Wait for data from VTSPR task.
         */
        if (0 > OSAL_msgQRecv(q_ptr->data10Pkt, (char *)recvMsg_ptr,
                VTSPR_Q_MULTI_DECODE_PKT_MSG_SIZE, OSAL_WAIT_FOREVER, NULL)) {
            _VTSP_TRACE(__FILE__, __LINE__);
        }

        if (_VTSPR_MULTI_CODER_CMD_SHUTDOWN == recvMsg_ptr->cmd) {
            _VTSP_TRACE(__FILE__, __LINE__);            
            break; /* if receive shutdown signal break while loop */
        }
        else if (_VTSPR_MULTI_CODER_CMD_RUN != recvMsg_ptr->cmd) {
            OSAL_logMsg("%s:%d FATAL ERROR cmd!=_VTSPR_MULTI_CODER_CMD_RUN", 
                    __FUNCTION__, __LINE__);
            continue;
        }

        decType   = recvMsg_ptr->decType;
        infc      = recvMsg_ptr->infc;
        streamId  = recvMsg_ptr->streamId;

#if defined(VTSP_ENABLE_G729_ACCELERATOR) || \
    defined(VTSP_ENABLE_G726_ACCELERATOR) 
        payloadSize = recvMsg_ptr->payloadSize;
        /* get multi-dec object for this stream */
        multiDecObj_ptr = _VTSPR_streamIdToMultiDecPtr(dsp_ptr,
                infc, streamId);
#endif
#ifdef VTSP_ENABLE_G726_ACCELERATOR 
        stream_ptr = _VTSPR_streamIdToStreamPtr(dsp_ptr, infc, streamId);
#endif

        switch (decType) {
#ifdef VTSP_ENABLE_G729_ACCELERATOR 
            case VTSP_CODER_G729:
                /*
                 * Do not re-init the DSP instance
                 * - alex
                 */
                if (1 == recvMsg_ptr->initFlag) {
                    dspInitParam = DSP_VAD_ENABLE;
                    /* 
                     * Initialize decoder with VAD always enabled.
                     * get instance identifier
                     */
                    retInstance = DSP_decodeInit(
                            multiDecObj_ptr->decG729Instance,
                            DSP_CODER_TYPE_G729,
                            dspInitParam);
                    if ((DSP_Instance)NULL == retInstance) {
                        /* Error! */
                        _VTSP_TRACE(__FILE__, __LINE__);
                    }
                    else if (retInstance != multiDecObj_ptr->decG729Instance) {
                        multiDecObj_ptr->decG729Instance = retInstance;
                    }
                   
                }

                /*
                 * Run G.729AB
                 */
                speechBufSize = DSP_decode(multiDecObj_ptr->decG729Instance,
                        DSP_CODER_TYPE_G729, dst_ptr,
                        (uint8 *)payload_ptr, payloadSize);
                break;
#endif
#ifdef VTSP_ENABLE_G726_ACCELERATOR 
            case VTSP_CODER_G726_32K:
                if (1 == recvMsg_ptr->initFlag) {
                    dspInitParam = DSP_G726_BITRATE_32KBPS;
                    /* Initialize decoder, get instance identifier */
                    retInstance = DSP_decodeInit(
                            multiDecObj_ptr->decG726Instance,
                            DSP_CODER_TYPE_G726,
                            dspInitParam);
                    if ((DSP_Instance)NULL == retInstance) {
                        /* Error! */
                        _VTSP_TRACE(__FILE__, __LINE__);
                    }
                    else if (retInstance != multiDecObj_ptr->decG726Instance) {
                        multiDecObj_ptr->decG726Instance = retInstance;
                    }
                    stream_ptr->cnPower = 0;
                }

                /*
                 * Run G.726
                 */
                if (0 != payloadSize) {
                    speechBufSize = DSP_decode(
                            multiDecObj_ptr->decG726Instance,
                            DSP_CODER_TYPE_G726,
                            dst_ptr,
                            (uint8 *)payload_ptr,
                            payloadSize);
                }
                break;
#endif
#ifdef VTSP_ENABLE_G711P1_ACCELERATOR
            case VTSP_CODER_G711P1U:
            case VTSP_CODER_G711P1A:
#error XXX G711P1 ACCELERATOR NOT YET IMPLEMENTED
                break;
#endif
            default:;
            /* XXX should we get here? XXX */
        }   
        sendMsg_ptr->msgSize = (vint)speechBufSize;

        /* Send the data back to the application */
        decQId = _VTSPR_streamIdToMultiDecQId(q_ptr, infc, streamId,
                _VTSPR_MULTI_CODER_PKT_RATE_10);
        if (OSAL_SUCCESS != OSAL_msgQSend(decQId, (char *)sendMsg_ptr,
                VTSPR_Q_MULTI_DECODE_DEC_MSG_SIZE, OSAL_WAIT_FOREVER, NULL))
        {
            _VTSP_TRACE(__FILE__, __LINE__);
        }
    }
    /*
     * Make the Multi-frame Decoded task as finished.
     */
    task_ptr->taskEnable = VTSPR_TASK_FINISHED;

    /*
     * Free allocated memory.
     */
    if (NULL != recvMsg_ptr) {
        if (OSAL_SUCCESS != OSAL_memFree(recvMsg_ptr, 0)) {
            _VTSP_TRACE(__FILE__, __LINE__);
        }
    }
    if (NULL != sendMsg_ptr) {
        if (OSAL_SUCCESS != OSAL_memFree(sendMsg_ptr, 0)) {
            _VTSP_TRACE(__FILE__, __LINE__);
        }
    }
    /*
     * Signal that ILBC has completed its cleanup.
     */
    OSAL_semGive(task_ptr->finishSemId);

    return (0);
}

#ifdef VTSP_ENABLE_ILBC
/*
 * ======== _VTSPR_format16ToVint() ========
 *
 * The current implementation of iLBC produces voice samples in arrays of 16-bit
 * words. VTSPR uses arrays of voice samples in 32 bit words. This function
 * converts iLBC voice data into VTSPR voice arrays.
 */
static void _VTSPR_format16ToVint(
    vint  *dst_ptr,
    int16 *src_ptr,
    vint   length)
{
    register vint  *dstx_ptr;
    register int16 *srcx_ptr;

    /*
     * Work through the array in reverse order. This allows the src and dst
     * arrays to be the same.
     */
    srcx_ptr = src_ptr + length - 1;
    dstx_ptr = dst_ptr + length - 1;

    while (length--) {
        *dstx_ptr--  = ((vint)(*srcx_ptr--) >> 2);
    }
}
#endif
