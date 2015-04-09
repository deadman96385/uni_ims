/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL AND
 * PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2006-2009 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 26340 $ $Date: 2014-05-20 11:22:57 +0800 (Tue, 20 May 2014) $
 *
 * Author: CM Garrido
 */        

#include <vtspr.h>
#include <_vtspr_private.h>

#if defined(VTSP_ENABLE_G726_ACCELERATOR) || \
        defined(VTSP_ENABLE_G729_ACCELERATOR)
/* Include if DSP ACCELERATOR is used */
#include <dsp.h>
#endif

#ifdef VTSP_ENABLE_ILBC
static void _VTSPR_formatVintTo16(int16*, vint*, vint);
#endif

/*
 * ======== _VTSPR_multiEncode30Task() ========
 * The multiEncode task processes all instances of speech encoders who operate
 * on multiple of 10ms frames. The task waits on
 * the single rawData queue, encodes the data, and then sends the encoded data b
 * ack to the framework using * the encData queue (selected by the key).
 */
vint _VTSPR_multiEncode30Task(
    OSAL_TaskArg taskArg)
{
    VTSPR_TaskContext    *task_ptr;
    VTSPR_Queues         *q_ptr;
    vint                  msgSize;
    _VTSPR_MultiRawMsg   *recvMsg_ptr = NULL;
    _VTSPR_MultiCodedMsg *sendMsg_ptr = NULL;
    uvint                 encType;
    vint                  infc;       /* must be signed */
    vint                  streamId;
    OSAL_MsgQId           encQId;
#if defined(VTSP_ENABLE_G723) || defined(VTSP_ENABLE_ILBC)
   _VTSPR_MultiEncObj    *multiEncObj_ptr;
    vint                  silenceComp;
    vint                  extension;
    uint8                *dst_ptr = NULL;
    vint                 *payload_ptr;
    VTSPR_DSP            *dsp_ptr;
#endif
#ifdef VTSP_ENABLE_G723
    G723A_EncObj         *g723encObj_ptr;
#endif  
#ifdef VTSP_ENABLE_ILBC
    ILBC_EncObj          *iLBCencObj_ptr;
#endif 

    /*
     * Create generic message structures and objects.
     */
    if (NULL == (recvMsg_ptr =
            (_VTSPR_MultiRawMsg *)
                    (OSAL_memAlloc(VTSPR_Q_MULTI_ENCODE_RAW_MSG_SIZE, 0)))) {
        _VTSP_TRACE(__FILE__, __LINE__);
        return (-1);
    }
    if (NULL == (sendMsg_ptr =
            (_VTSPR_MultiCodedMsg *)
                    (OSAL_memAlloc(VTSPR_Q_MULTI_ENCODE_ENC_MSG_SIZE, 0)))) {
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
    payload_ptr = recvMsg_ptr->enc_ary;
    dst_ptr     = sendMsg_ptr->coded_ary;
#endif

    while (0 != (VTSPR_TASK_RUN & task_ptr->taskEnable)) {
        /*
         * Wait for data from VTSPR task.
         */
        if (0 >= (msgSize = OSAL_msgQRecv(q_ptr->data30Raw, (char *)recvMsg_ptr,
                VTSPR_Q_MULTI_ENCODE_RAW_MSG_SIZE, OSAL_WAIT_FOREVER, NULL))) {
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

        encType     = recvMsg_ptr->encType;
        infc        = recvMsg_ptr->infc;
        streamId    = recvMsg_ptr->streamId;
#if defined(VTSP_ENABLE_G723) || defined(VTSP_ENABLE_ILBC)
        extension   = recvMsg_ptr->extension;
        silenceComp = recvMsg_ptr->silenceComp;
        /* get multi-enc object for this stream */
        multiEncObj_ptr = _VTSPR_streamIdToMultiEncPtr(dsp_ptr, infc, streamId);
#endif
        

        switch (encType) {
#ifdef VTSP_ENABLE_G723
            case VTSP_CODER_G723_30MS:
                g723encObj_ptr = &(multiEncObj_ptr->g723encObj);
                if (1 == recvMsg_ptr->initFlag) {
                    G723A_encodeInit(g723encObj_ptr);
                }
                /*
                 * Run the G723 encoder
                 * First, convert Q13 speech to Q15 speech, 
                 * needed by the encoder.
                 */
                COMM_shiftLeft(payload_ptr, payload_ptr, 2,
                                (3 * VTSPR_NSAMPLES_10MS_8K));

                g723encObj_ptr->src_ptr = payload_ptr;
                g723encObj_ptr->dst_ptr = dst_ptr;
                if (0 != (extension & VTSP_MASK_EXT_G723_53)) {
                    g723encObj_ptr->wrkRate = G723A_WRKRATE_53;
                }
                else {
                    g723encObj_ptr->wrkRate = G723A_WRKRATE_63;
                }
                if (0 != (silenceComp & VTSP_MASK_CODER_G723_30MS)) {
                    g723encObj_ptr->useVx = G723A_USEVX_ENABLE;
                }
                else { 
                    g723encObj_ptr->useVx = G723A_USEVX_DISABLE;
                }
                g723encObj_ptr->useHp = G723A_USEHP_ENABLE;
                msgSize = G723A_encode(g723encObj_ptr);
                if (VTSP_BLOCK_G723_SID_SZ > msgSize) {
                    /* 
                     * G723A returns:
                     * 1 for untransmitted frame
                     * 4 for silence frame
                     * 20 for 5.3 kb speech
                     * 24 for 6.3 kb speech
                     */
                    msgSize = 0;
                }
                break;
#endif
#ifdef VTSP_ENABLE_ILBC
            case VTSP_CODER_ILBC_30MS:
                iLBCencObj_ptr = &(multiEncObj_ptr->iLBCencObj);
                if (1 == recvMsg_ptr->initFlag) {
                    /* Initialize encoder */
                    ILBC_encoderinit(iLBCencObj_ptr, ILBC_MODE_30MS);
                }
                _VTSPR_formatVintTo16((int16 *)payload_ptr,
                        payload_ptr, ILBC_BLOCK_LENGTH_30MS);

                msgSize = ILBC_encode(iLBCencObj_ptr, (int16 *)payload_ptr,
                        ILBC_BLOCK_LENGTH_30MS, (int16 *)dst_ptr);
                break;
#endif
            default:;
            /* XXX should we get here? XXX */
        }    
        sendMsg_ptr->msgSize = msgSize;

        /*
         * Send the data back to the application.
         */
        encQId = _VTSPR_streamIdToMultiEncQId(q_ptr, infc, streamId,
                _VTSPR_MULTI_CODER_PKT_RATE_30);
        if (OSAL_SUCCESS != OSAL_msgQSend(encQId, (char *)sendMsg_ptr,
                VTSPR_Q_MULTI_ENCODE_ENC_MSG_SIZE, OSAL_WAIT_FOREVER,
                NULL))
        {
            _VTSP_TRACE(__FILE__, __LINE__);
        }
    }
    /*
     * Make the Multi-frame Encoded task as finished.
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

    return(0);
}

/*
 * ======== _VTSPR_multiEncodeTask20() ========
 * The multiEncode task processes all instances of speech encoders who operate
 * on multiple of 10ms frames. The task waits on
 * the single rawData queue, encodes the data, and then sends the encoded data b
 * ack to the framework using * the encData queue (selected by the key).
 */
vint _VTSPR_multiEncode20Task(
    OSAL_TaskArg taskArg)
{
    VTSPR_TaskContext    *task_ptr;
    VTSPR_Queues         *q_ptr;
    vint                  msgSize;
    _VTSPR_MultiRawMsg   *recvMsg_ptr;
    _VTSPR_MultiCodedMsg *sendMsg_ptr;
    uvint                 encType;
    vint                  infc;       /* must be signed */
    vint                  streamId;
#if defined(VTSP_ENABLE_GAMRNB) || defined(VTSP_ENABLE_GAMRWB) || \
        defined(VTSP_ENABLE_G722P1)
    vint                  extension;
#endif
    OSAL_MsgQId           encQId;
#if defined(VTSP_ENABLE_GAMRNB) || defined(VTSP_ENABLE_GAMRWB) || \
        defined(VTSP_ENABLE_SILK) || defined(VTSP_ENABLE_ILBC) || \
        defined(VTSP_ENABLE_G722P1)
    VTSPR_DSP            *dsp_ptr;
    uint8                *dst_ptr = NULL;
    vint                 *payload_ptr;
    _VTSPR_MultiEncObj   *multiEncObj_ptr;
#ifdef VTSP_ENABLE_BENCHMARK
    VTSPR_StreamObj      *stream_ptr;
#endif
#endif

#ifdef VTSP_ENABLE_ILBC
    ILBC_EncObj          *iLBCencObj_ptr;
#endif
#ifdef VTSP_ENABLE_SILK
    SILK_EncObj          *silkEncObj_ptr;
    SILK_SampleRate       sampleRate;
    vint                  blockSize;
#endif 
#ifdef VTSP_ENABLE_G722P1
    G722P1_EncObj        *g722p1encObj_ptr;
    uint8                *g722p1Scratch_ptr;
#endif  
#if defined(VTSP_ENABLE_G722P1) || defined(VTSP_ENABLE_GAMRNB) || \
        defined(VTSP_ENABLE_SILK)
    vint                  bitRate;
#endif
#ifdef VTSP_ENABLE_GAMRNB
    GAMRNB_EncObj        *gamrnbEncObj_ptr;
    GAMRNB_TxStatus       txType;
#endif
#ifdef VTSP_ENABLE_GAMRWB
    GAMRWB_EncObj        *gamrwbEncObj_ptr;
    GAMRWB_TxStatus       txTypeWb;
    GAMRWB_RateMode       bitRateWb;
#endif
#if defined(VTSP_ENABLE_GAMRNB) ||  defined(VTSP_ENABLE_GAMRWB)
    vint                  vadEnable;
    vint                  dataFormat;
    //vint                  idx;
#endif
    /*
     * Create generic message structures and objects.
     */
    if (NULL == (recvMsg_ptr =
            (_VTSPR_MultiRawMsg *)
                    (OSAL_memAlloc(VTSPR_Q_MULTI_ENCODE_RAW_MSG_SIZE, 0)))) {
        _VTSP_TRACE(__FILE__, __LINE__);
        return (-1);
    }
    if (NULL == (sendMsg_ptr =
            (_VTSPR_MultiCodedMsg *)
                    (OSAL_memAlloc(VTSPR_Q_MULTI_ENCODE_ENC_MSG_SIZE, 0)))) {
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
    payload_ptr = recvMsg_ptr->enc_ary;
    dst_ptr     = sendMsg_ptr->coded_ary;
#endif
#ifdef VTSP_ENABLE_G722P1
    g722p1Scratch_ptr = dsp_ptr->g722p1EncScratch_ary;
#endif

 while (0 != (VTSPR_TASK_RUN & task_ptr->taskEnable)) {
        /*
         * Wait for data from VTSPR task.
         */
        if (0 >= (msgSize = OSAL_msgQRecv(q_ptr->data20Raw, (char *)recvMsg_ptr,
                VTSPR_Q_MULTI_ENCODE_RAW_MSG_SIZE, OSAL_WAIT_FOREVER, NULL))) {
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

        encType   = recvMsg_ptr->encType;
        infc      = recvMsg_ptr->infc;
        streamId  = recvMsg_ptr->streamId;
#if defined(VTSP_ENABLE_GAMRNB) || defined(VTSP_ENABLE_GAMRWB) || \
        defined(VTSP_ENABLE_G722P1)
        extension = recvMsg_ptr->extension;
#endif

#if defined(VTSP_ENABLE_GAMRNB) || defined(VTSP_ENABLE_GAMRWB) || \
        defined(VTSP_ENABLE_SILK) || defined(VTSP_ENABLE_ILBC) || \
        defined(VTSP_ENABLE_G722P1)
        /* get multi-enc object for this stream */
        multiEncObj_ptr = _VTSPR_streamIdToMultiEncPtr(dsp_ptr, infc, streamId);
#ifdef VTSP_ENABLE_BENCHMARK
        stream_ptr = _VTSPR_streamIdToStreamPtr(dsp_ptr, infc, streamId);
#endif
#endif

        switch (encType) {
#ifdef VTSP_ENABLE_ILBC
            case VTSP_CODER_ILBC_20MS:
                iLBCencObj_ptr = &(multiEncObj_ptr->iLBCencObj);
                if (1 == recvMsg_ptr->initFlag) {
                    /* Initialize encoder */
                    ILBC_encoderinit(iLBCencObj_ptr, ILBC_MODE_20MS);
                }
                _VTSPR_formatVintTo16((int16 *)payload_ptr,
                        payload_ptr, ILBC_BLOCK_LENGTH_20MS);
                msgSize = ILBC_encode(iLBCencObj_ptr, (int16 *)payload_ptr,
                        ILBC_BLOCK_LENGTH_20MS, (int16 *)dst_ptr);
                break;
#endif
#ifdef VTSP_ENABLE_SILK
            case VTSP_CODER_SILK_20MS_8K:
            case VTSP_CODER_SILK_20MS_16K:
            case VTSP_CODER_SILK_20MS_24K:
                if (VTSP_CODER_SILK_20MS_8K == encType) {
                    sampleRate = SILK_SAMPLE_RATE_8000;
                    bitRate    = SILK_ENCODER_BITRATE_20MS_8K;
                    blockSize  = SILK_SPEECH_BUFSZ_20MS_8K;
                }
                else if (VTSP_CODER_SILK_20MS_16K == encType) {
                    sampleRate = SILK_SAMPLE_RATE_16000;
                    bitRate    = SILK_ENCODER_BITRATE_20MS_16K;
                    blockSize  = SILK_SPEECH_BUFSZ_20MS_16K;
                }
                else {
                    sampleRate = SILK_SAMPLE_RATE_24000;
                    bitRate    = SILK_ENCODER_BITRATE_20MS_24K;
                    blockSize  = SILK_SPEECH_BUFSZ_20MS_24K;
                    _VTSP_TRACE(__FILE__, __LINE__);
                    break;
                }
                silkEncObj_ptr = &(multiEncObj_ptr->silkEncObj);
                if (1 == recvMsg_ptr->initFlag) {
                    /* Initialize encoder */
                    SILK_encodeInit(silkEncObj_ptr, sampleRate, 
                            SILK_DTX_VAD_DISABLE, SILK_FEC_ENABLE, 
                            SILK_COMPLEXITY_MODE_2, SILK_PACKET_SIZE_MS_20, 
                            bitRate, 0);
                }
                COMM_shiftLeft(dst_ptr, dst_ptr, 2, blockSize);
                _VTSPR_formatVintTo16((int16 *)payload_ptr, payload_ptr, 
                        blockSize);
                msgSize = SILK_encode(silkEncObj_ptr, (int16 *)payload_ptr,
                        dst_ptr);
                break;
#endif
#ifdef VTSP_ENABLE_G722P1
            case VTSP_CODER_G722P1_20MS:
                g722p1encObj_ptr = &(multiEncObj_ptr->g722p1encObj);
                g722p1encObj_ptr->scratch_ptr = (void *)g722p1Scratch_ptr;
                if (1 == recvMsg_ptr->initFlag) {
                    if (0 != (extension & VTSP_MASK_EXT_G722P1_32)) {
                        bitRate = G722P1_BIT_RATE_32KBPS;
                    }
                    else {
                        bitRate = G722P1_BIT_RATE_24KBPS;
                    }
                    G722P1_encodeInit(g722p1encObj_ptr, bitRate,
                            G722P1_SAMPLE_RATE_16KHZ);
                }
                /* Convert Q13 speech to Q15 speech, needed by the codec */
                COMM_shiftLeft(payload_ptr, payload_ptr, 2,
                        (2 * VTSPR_NSAMPLES_10MS_16K));
                msgSize = G722P1_encode(g722p1encObj_ptr, payload_ptr,
                                dst_ptr);
                break;
#endif
#ifdef VTSP_ENABLE_GAMRNB
            case VTSP_CODER_GAMRNB_20MS_OA:
            case VTSP_CODER_GAMRNB_20MS_BE:
                gamrnbEncObj_ptr = &(multiEncObj_ptr->gamrnbEncObj);
                if (1 == recvMsg_ptr->initFlag) {
                    if (VTSP_CODER_GAMRNB_20MS_OA == encType) {
                        dataFormat = GAMRNB_DATA_FORMAT_PACKED_OA;
                    }
                    else {
                        dataFormat = GAMRNB_DATA_FORMAT_PACKED_BE;
                    }
                    GAMRNB_encodeInit(gamrnbEncObj_ptr,
                            GAMRNB_VAD_TYPE_VAD1_ENS, 
                            GAMRNB_SPEECH_FORMAT_LINEAR,
                            dataFormat,
                            GAMRNB_BIT_MASK_13BIT);
                }
                /* Convert Q13 speech to Q15 speech, needed by the codec */
                COMM_shiftLeft(payload_ptr, payload_ptr, 2,
                        (2 * VTSPR_NSAMPLES_10MS_8K));

                if (0 != (extension & VTSP_MASK_EXT_GAMRNB_20MS_475)) {
                    bitRate = GAMRNB_MR475;
                }
                else if (0 != (extension & VTSP_MASK_EXT_GAMRNB_20MS_515)) {
                    bitRate = GAMRNB_MR515;
                }
                else if (0 != (extension & VTSP_MASK_EXT_GAMRNB_20MS_59)) {
                    bitRate = GAMRNB_MR59;
                }
                else if (0 != (extension & VTSP_MASK_EXT_GAMRNB_20MS_67)) {
                    bitRate = GAMRNB_MR67;
                }
                else if (0 != (extension & VTSP_MASK_EXT_GAMRNB_20MS_74)) {
                    bitRate = GAMRNB_MR74;
                }
                else if (0 != (extension & VTSP_MASK_EXT_GAMRNB_20MS_795)) {
                    bitRate = GAMRNB_MR795;
                }
                else if (0 != (extension & VTSP_MASK_EXT_GAMRNB_20MS_102)) {
                    bitRate = GAMRNB_MR102;
                }
                else {
                    bitRate = GAMRNB_MR122; /* user 12.2 as default rate */
                }

                /* Determine VAD on/off*/
                if (0 != (recvMsg_ptr->silenceComp &
                        (VTSP_MASK_CODER_GAMRNB_20MS_OA |
                        VTSP_MASK_CODER_GAMRNB_20MS_BE))) {
                    vadEnable = GAMRNB_DTX_VAD_ENABLE;
                }
                else {
                    vadEnable = GAMRNB_DTX_VAD_DISABLE;
                }

                msgSize = GAMRNB_encode(gamrnbEncObj_ptr, payload_ptr, 
                        dst_ptr, bitRate, vadEnable, &txType);

                break;
#endif
#ifdef VTSP_ENABLE_GAMRWB
            case VTSP_CODER_GAMRWB_20MS_OA:
            case VTSP_CODER_GAMRWB_20MS_BE:
                gamrwbEncObj_ptr = &(multiEncObj_ptr->gamrwbEncObj);
                if (1 == recvMsg_ptr->initFlag) {
                    if (VTSP_CODER_GAMRWB_20MS_OA == encType) {
                        dataFormat = GAMRWB_DATA_FORMAT_PACKED_IF1;
                    }
                    else {
                        dataFormat = GAMRWB_DATA_FORMAT_PACKED_IF2;
                    }
                    GAMRWB_encoderInit(gamrwbEncObj_ptr, dataFormat);
                }
                /* Convert Q13 speech to Q15 speech, needed by the codec */
                COMM_shiftLeft(payload_ptr, payload_ptr, 2,
                        (2 * VTSPR_NSAMPLES_10MS_16K));

                if (0 != (extension & VTSP_MASK_EXT_GAMRWB_20MS_660)) {
                    bitRateWb = GAMRWB_MR660;
                }
                else if (0 != (extension & VTSP_MASK_EXT_GAMRWB_20MS_885)) {
                    bitRateWb = GAMRWB_MR885;
                }
                else if (0 != (extension & VTSP_MASK_EXT_GAMRWB_20MS_1265)) {
                    bitRateWb = GAMRWB_MR1265;
                }
                else if (0 != (extension & VTSP_MASK_EXT_GAMRWB_20MS_1425)) {
                    bitRateWb = GAMRWB_MR1425;
                }
                else if (0 != (extension & VTSP_MASK_EXT_GAMRWB_20MS_1585)) {
                    bitRateWb = GAMRWB_MR1585;
                }
                else if (0 != (extension & VTSP_MASK_EXT_GAMRWB_20MS_1825)) {
                    bitRateWb = GAMRWB_MR1825;
                }
                else if (0 != (extension & VTSP_MASK_EXT_GAMRWB_20MS_1985)) {
                    bitRateWb = GAMRWB_MR1985;
                }
                else if (0 != (extension & VTSP_MASK_EXT_GAMRWB_20MS_2305)) {
                    bitRateWb = GAMRWB_MR2305;
                }
                else {
                    bitRateWb = GAMRWB_MR2385; /* user 23.85 as default rate */
                }

                /* Determine VAD on/off*/
                if (0 != (recvMsg_ptr->silenceComp &
                        (VTSP_MASK_CODER_GAMRWB_20MS_OA |
                        VTSP_MASK_CODER_GAMRWB_20MS_BE))) {
                    vadEnable = GAMRWB_DTX_VAD_ENABLE;
                }
                else {
                    vadEnable = GAMRWB_DTX_VAD_DISABLE;
                }
#ifdef VTSP_ENABLE_BENCHMARK
                if (0 == stream_ptr->streamParam.streamId) {
                    _VTSPR_benchmarkStart(&VTSPR_obj,
                            _VTSPR_BENCHMARK_GAMRWB_ENCODE_ID0, 0);
                 }
                 else {
                     _VTSPR_benchmarkStart(&VTSPR_obj,
                            _VTSPR_BENCHMARK_GAMRWB_ENCODE_ID1, 0);
                 }
#endif
                msgSize = GAMRWB_encode(gamrwbEncObj_ptr, payload_ptr, 
                        dst_ptr, &bitRateWb, vadEnable, &txTypeWb);
#ifdef VTSP_ENABLE_BENCHMARK

                if (0 == stream_ptr->streamParam.streamId) {
                    _VTSPR_benchmarkStop(&VTSPR_obj,
                            _VTSPR_BENCHMARK_GAMRWB_ENCODE_ID0, 0);
                }
                else {
                    _VTSPR_benchmarkStop(&VTSPR_obj,
                            _VTSPR_BENCHMARK_GAMRWB_ENCODE_ID1, 0);
                }
#endif
                break;
#endif
            default:;
            /* XXX should we get here? XXX */
        }    
        sendMsg_ptr->msgSize = msgSize;

        /* Send the data back to the application */
        encQId = _VTSPR_streamIdToMultiEncQId(q_ptr, infc, streamId,
                _VTSPR_MULTI_CODER_PKT_RATE_20);
        if (OSAL_SUCCESS != OSAL_msgQSend(encQId, (char *)sendMsg_ptr,
                VTSPR_Q_MULTI_ENCODE_ENC_MSG_SIZE, OSAL_WAIT_FOREVER,
                NULL)) {
            _VTSP_TRACE(__FILE__, __LINE__);
        }
    }
    /*
     * Make the Multi-frame Encoded task as finished.
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
 * ======== _VTSPR_multiEncode10Task() ========
 * The multiEncode task processes all instances of speech encoders who operate
 * on multiple of 10ms frames. The task waits on
 * the single rawData queue, encodes the data, and then sends the encoded data b
 * ack to the framework using * the encData queue (selected by the key).
 */
vint _VTSPR_multiEncode10Task(
    OSAL_TaskArg taskArg)
{
    VTSPR_TaskContext    *task_ptr;
    VTSPR_Queues         *q_ptr;
    vint                  msgSize;
    _VTSPR_MultiRawMsg   *recvMsg_ptr;
    _VTSPR_MultiCodedMsg *sendMsg_ptr;
    uvint                 encType;
    vint                  infc;       /* must be signed */
    vint                  streamId;
    OSAL_MsgQId           encQId;
#if defined(VTSP_ENABLE_G729_ACCELERATOR) || \
   defined(VTSP_ENABLE_G726_ACCELERATOR)
    uint32                dspInitParam;
    DSP_Instance          retInstance;
    VTSPR_StreamObj      *stream_ptr;
    _VTSPR_MultiEncObj   *multiEncObj_ptr; 
    vint                  extension;
    uint8                *dst_ptr = NULL;
    vint                 *payload_ptr;
    VTSPR_DSP            *dsp_ptr;
#endif

    /*
     * Create generic message structures and objects.
     */
    if (NULL == (recvMsg_ptr =
            (_VTSPR_MultiRawMsg *)
                    (OSAL_memAlloc(VTSPR_Q_MULTI_ENCODE_RAW_MSG_SIZE, 0)))) {
        _VTSP_TRACE(__FILE__, __LINE__);
        return (-1);
    }
    if (NULL == (sendMsg_ptr =
            (_VTSPR_MultiCodedMsg *)
                    (OSAL_memAlloc(VTSPR_Q_MULTI_ENCODE_ENC_MSG_SIZE, 0)))) {
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
    payload_ptr = recvMsg_ptr->enc_ary;
    dst_ptr     = sendMsg_ptr->coded_ary;
#endif

    while (0 != (VTSPR_TASK_RUN & task_ptr->taskEnable)) {
        /*
         * Wait for data from VTSPR task.
         */
        if (0 >= (msgSize = OSAL_msgQRecv(q_ptr->data10Raw, (char *)recvMsg_ptr,
                VTSPR_Q_MULTI_ENCODE_RAW_MSG_SIZE, OSAL_WAIT_FOREVER, NULL))) {
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

        encType   = recvMsg_ptr->encType;
        infc      = recvMsg_ptr->infc;
        streamId  = recvMsg_ptr->streamId;
#if defined(VTSP_ENABLE_G729_ACCELERATOR) || \
   defined(VTSP_ENABLE_G726_ACCELERATOR)
        extension = recvMsg_ptr->extension;

        /* get multi-enc object for this stream */
        multiEncObj_ptr = _VTSPR_streamIdToMultiEncPtr(dsp_ptr, infc, streamId);
        stream_ptr = _VTSPR_streamIdToStreamPtr(dsp_ptr, infc, streamId);
#endif

        switch (encType) {
#ifdef VTSP_ENABLE_G729_ACCELERATOR 
            case VTSP_CODER_G729:
                dspInitParam = 0;
                /* Determine whether or not VAD/CNG is enabled */
                if (0 != (stream_ptr->streamParam.silenceComp &
                        VTSP_MASK_CODER_G729)) {
                    dspInitParam |= DSP_VAD_ENABLE;
                }

                /*
                 * Do not re-init the DSP instance
                 * - alex
                 */
                if (1 == recvMsg_ptr->initFlag) {
                    /* Initialize encoder, get instance identifier */
                    retInstance = DSP_encodeInit(
                            multiEncObj_ptr->encG729Instance,
                            DSP_CODER_TYPE_G729,
                            dspInitParam);
                    if ((DSP_Instance)NULL == retInstance) {
                        /* Error! */
                        OSAL_logMsg("%s:%d DSP_encodeInit G729 failed\n",
                                (int)__FILE__, __LINE__, 0, 0);
                    }
                    else if (retInstance != multiEncObj_ptr->encG729Instance) {
                        multiEncObj_ptr->encG729Instance = retInstance;
                    }
                }

                /*
                 * Run G.729AB
                 */
                msgSize = DSP_encode(
                        multiEncObj_ptr->encG729Instance,    
                        DSP_CODER_TYPE_G729,
                        (uint8 *)dst_ptr,
                        (vint *)payload_ptr);

                break;
#endif
#ifdef VTSP_ENABLE_G726_ACCELERATOR 
            case VTSP_CODER_G726_32K:
                dspInitParam = 0;
                if (1 == recvMsg_ptr->initFlag) {
                    dspInitParam |= DSP_G726_BITRATE_32KBPS;
                    /* Initialize encoder, get instance identifier */
                    retInstance = DSP_encodeInit(
                            multiEncObj_ptr->encG726Instance,
                            DSP_CODER_TYPE_G726, dspInitParam);
                    if ((DSP_Instance)NULL == retInstance) {
                        /* Error! */
                        OSAL_logMsg("%s:%d DSP_encodeInit G726 failed\n",
                                (int)__FILE__, __LINE__, 0, 0);
                    }
                    else if (retInstance != multiEncObj_ptr->encG726Instance) {
                        multiEncObj_ptr->encG726Instance = retInstance;
                    }
                    stream_ptr->cnPower = 0;
                }

                /*
                 * Run G.726
                 */
                msgSize = DSP_encode(
                        multiEncObj_ptr->encG726Instance,
                        DSP_CODER_TYPE_G726,
                        (uint8 *)dst_ptr,
                        (vint *)payload_ptr);
                break;
#endif
#ifdef VTSP_ENABLE_G711P1_ACCELERATOR 
            case VTSP_CODER_G711P1U:
            case VTSP_CODER_G711P1A:
#error XXX G711P1 ACCELERATOR NOT FINISHED

                break;
#endif
            default:;
            /* XXX should we get here? XXX */
        }    
        sendMsg_ptr->msgSize = msgSize;

        /* Send the data back to the application */
        encQId = _VTSPR_streamIdToMultiEncQId(q_ptr, infc, streamId,
                _VTSPR_MULTI_CODER_PKT_RATE_10);
        if (OSAL_SUCCESS != OSAL_msgQSend(encQId, (char *)sendMsg_ptr,
                VTSPR_Q_MULTI_ENCODE_ENC_MSG_SIZE, OSAL_WAIT_FOREVER,
                NULL)) {
            _VTSP_TRACE(__FILE__, __LINE__);
        }
    }
    /*
     * Make the Multi-frame Encoded task as finished.
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
 * ======== _VTSPR_formatVintTo16() ========
 *
 * This function packs the voice data. It takes 32 bit voice samples and packs
 * them into 16 bit voice samples. This is required by the current
 * implementation of ILBC.
 */
static void _VTSPR_formatVintTo16(
    int16  *dst_ptr,
    vint   *src_ptr,
    vint    length)
{
    register int16  *dstx_ptr = dst_ptr;
    register vint   *srcx_ptr = (vint *)src_ptr;

    while (length--) {
        *dstx_ptr++ = ((int16)(*srcx_ptr++) << 2);
    }
}
#endif
