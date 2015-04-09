/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2006-2009 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 28012 $ $Date: 2014-08-08 17:36:29 +0800 (Fri, 08 Aug 2014) $
 *
 * Author: C M Garrido
 */

#include <vtspr.h>
#include <_vtspr_private.h>

#if defined(VTSP_ENABLE_G726_ACCELERATOR) || \
        defined(VTSP_ENABLE_G729_ACCELERATOR)
/* Include if DSP ACCELERATOR is used */
#include <dsp.h>
#endif

/*
 * ======== _VTSPR_multiDecodeTaskInit() ========
 * Create tasks to handle multi-frame decoding of 10ms, 20ms and 30ms coder
 * types seperately. These tasks are required because the MHz used by the 
 * decoder needs to be distributed across 10, 20 or 30ms.
 */
vint _VTSPR_multiDecodeTaskInit(VTSPR_Obj *vtspr_ptr)
{
#if defined(VTSP_ENABLE_ILBC) || defined(VTSP_ENABLE_G722P1) || \
        defined(VTSP_ENABLE_SILK) || defined(VTSP_ENABLE_GAMRNB) || \
        defined(VTSP_ENABLE_GAMRWB) || defined(VTSP_ENABLE_G723) || \
        defined(VTSP_ENABLE_G729_ACCELERATOR) || \
        defined(VTSP_ENABLE_G726_ACCELERATOR)
    OSAL_TaskArg       taskArg;
#endif
#if defined(VTSP_ENABLE_G729_ACCELERATOR) || \
        defined(VTSP_ENABLE_G726_ACCELERATOR)
    VTSPR_TaskContext *multiDecTask10ms_ptr;
#endif
#if defined(VTSP_ENABLE_ILBC) || defined(VTSP_ENABLE_G722P1) || \
        defined(VTSP_ENABLE_GAMRNB) || defined(VTSP_ENABLE_SILK) || \
        defined(VTSP_ENABLE_GAMRWB)
    VTSPR_TaskContext *multiDecTask20ms_ptr;
#endif
#if defined(VTSP_ENABLE_ILBC) || defined(VTSP_ENABLE_G723)
    VTSPR_TaskContext *multiDecTask30ms_ptr;
#endif

    /*
     * Create 10ms Task
     */
#if (defined(VTSP_ENABLE_G729_ACCELERATOR) || \
        defined(VTSP_ENABLE_G726_ACCELERATOR))
    multiDecTask10ms_ptr = &(vtspr_ptr->multiDecTask10ms);
    taskArg = multiDecTask10ms_ptr; /* or &myVar */
    multiDecTask10ms_ptr->taskEnable   = VTSPR_TASK_RUN;
    multiDecTask10ms_ptr->taskPriority = VTSPR_MULTI_DECODE10_TASK_PRIORITY;
    multiDecTask10ms_ptr->stackSize    = VTSPR_MULTI_DECODE10_TASK_STACK_SZ;
    
    multiDecTask10ms_ptr->taskId = OSAL_taskCreate("d2_multDecode10",
            multiDecTask10ms_ptr->taskPriority, multiDecTask10ms_ptr->stackSize,
            (void *)_VTSPR_multiDecode10Task, taskArg);
    if (NULL == multiDecTask10ms_ptr->taskId) {
        _VTSP_TRACE(__FILE__, __LINE__);
        return (-1);
    }
#endif

    /*
     * Create 20ms Task
     */
#if defined(VTSP_ENABLE_ILBC) || defined(VTSP_ENABLE_G722P1) || \
        defined(VTSP_ENABLE_GAMRNB) || defined(VTSP_ENABLE_SILK) || \
        defined(VTSP_ENABLE_GAMRWB)
    multiDecTask20ms_ptr = &(vtspr_ptr->multiDecTask20ms);
    taskArg = multiDecTask20ms_ptr; /* or &myVar */
    multiDecTask20ms_ptr->taskEnable   = VTSPR_TASK_RUN;
    multiDecTask20ms_ptr->taskPriority = VTSPR_MULTI_DECODE20_TASK_PRIORITY;
    multiDecTask20ms_ptr->stackSize    = VTSPR_MULTI_DECODE20_TASK_STACK_SZ;
    
    multiDecTask20ms_ptr->taskId = OSAL_taskCreate("d2_multDecode20",
            multiDecTask20ms_ptr->taskPriority, multiDecTask20ms_ptr->stackSize,
            (void *)_VTSPR_multiDecode20Task, taskArg);
    if (NULL == multiDecTask20ms_ptr->taskId) {
        _VTSP_TRACE(__FILE__, __LINE__);
        return (-1);
    }
#endif

    /*
     * Create 30ms Task
     */
#if defined(VTSP_ENABLE_ILBC) || defined(VTSP_ENABLE_G723)
    multiDecTask30ms_ptr = &(vtspr_ptr->multiDecTask30ms);
    taskArg = multiDecTask30ms_ptr; /* or &myVar */
    multiDecTask30ms_ptr->taskEnable   = VTSPR_TASK_RUN;
    multiDecTask30ms_ptr->taskPriority = VTSPR_MULTI_DECODE30_TASK_PRIORITY;
    multiDecTask30ms_ptr->stackSize    = VTSPR_MULTI_DECODE30_TASK_STACK_SZ;
    
    multiDecTask30ms_ptr->taskId = OSAL_taskCreate("d2_multDecode30",
            multiDecTask30ms_ptr->taskPriority, multiDecTask30ms_ptr->stackSize,
            (void *)_VTSPR_multiDecode30Task, taskArg);
    if (NULL == multiDecTask30ms_ptr->taskId) {
        _VTSP_TRACE(__FILE__, __LINE__);
        return (-1);
    }
#endif
    return (0);
}

/*
 * ======== _VTSPR_multiDecodeInit() ========
 * Create Multi-frame Decode task queues for 10, 20 and 30ms cases.
 */
vint _VTSPR_multiDecodeInit(
    VTSPR_Obj      *vtspr_ptr,
    VTSPR_Queues   *q_ptr,
    VTSPR_DSP      *dsp_ptr) 
{
    VTSPR_TaskContext *multiDecTask10ms_ptr;
    VTSPR_TaskContext *multiDecTask20ms_ptr;
    VTSPR_TaskContext *multiDecTask30ms_ptr;
    VTSPR_StreamObj   *stream_ptr;
    vint               infc;        /* must be signed */
    vint               streamId;
    vint               index;
#if defined(VTSP_ENABLE_G729_ACCELERATOR) || \
        defined(VTSP_ENABLE_G726_ACCELERATOR) || \
        defined(VTSP_ENABLE_ILBC) || defined(VTSP_ENABLE_G722P1) || \
        defined(VTSP_ENABLE_ILBC) || defined(VTSP_ENABLE_G723) || \
        defined(VTSP_ENABLE_GAMRNB) || defined(VTSP_ENABLE_SILK) || \
        defined(VTSP_ENABLE_GAMRWB)
    char               qname[16];
#endif

    /* 
     * Set Multi-Frame Decode task context pointers to be self contained
     */
    multiDecTask10ms_ptr          = &(vtspr_ptr->multiDecTask10ms);
    multiDecTask10ms_ptr->q_ptr   = q_ptr;
    multiDecTask10ms_ptr->dsp_ptr = dsp_ptr;

    multiDecTask20ms_ptr          = &(vtspr_ptr->multiDecTask20ms);
    multiDecTask20ms_ptr->q_ptr   = q_ptr;
    multiDecTask20ms_ptr->dsp_ptr = dsp_ptr;

    multiDecTask30ms_ptr          = &(vtspr_ptr->multiDecTask30ms);
    multiDecTask30ms_ptr->q_ptr   = q_ptr;
    multiDecTask30ms_ptr->dsp_ptr = dsp_ptr;

    /*
     * Create shared packet queue for 10, 20, and 20ms tasks, respectively
     */
#if defined(VTSP_ENABLE_G729_ACCELERATOR) || \
        defined(VTSP_ENABLE_G726_ACCELERATOR)
    if (NULL == (q_ptr->data10Pkt = OSAL_msgQCreate(
                                    "vtsp-m10pktq",
                                    OSAL_MODULE_AUDIO_VE, OSAL_MODULE_AUDIO_VE,
                                    OSAL_DATA_STRUCT__VTSPR_MultiPktMsg,
                                    VTSPR_Q_MULTI_DECODE_PKT_NUM_MSG,
                                    VTSPR_Q_MULTI_DECODE_PKT_MSG_SIZE, 0)))
    {
        _VTSP_TRACE(__FILE__,__LINE__);
        return (-1);
    }
#endif
#if defined(VTSP_ENABLE_ILBC) || defined(VTSP_ENABLE_G722P1) || \
         defined(VTSP_ENABLE_GAMRNB) || defined(VTSP_ENABLE_SILK) || \
         defined(VTSP_ENABLE_GAMRWB)
    if (NULL == (q_ptr->data20Pkt = OSAL_msgQCreate(
                                    "vtsp-m20pktq",
                                    OSAL_MODULE_AUDIO_VE, OSAL_MODULE_AUDIO_VE,
                                    OSAL_DATA_STRUCT__VTSPR_MultiPktMsg,
                                    VTSPR_Q_MULTI_DECODE_PKT_NUM_MSG,
                                    VTSPR_Q_MULTI_DECODE_PKT_MSG_SIZE, 0)))
    {
        _VTSP_TRACE(__FILE__,__LINE__);
        return (-1);
    }
#endif
#if defined(VTSP_ENABLE_ILBC) || defined(VTSP_ENABLE_G723)
    if (NULL == (q_ptr->data30Pkt = OSAL_msgQCreate(
                                    "vtsp-m30pktq",
                                    OSAL_MODULE_AUDIO_VE, OSAL_MODULE_AUDIO_VE,
                                    OSAL_DATA_STRUCT__VTSPR_MultiPktMsg,
                                    VTSPR_Q_MULTI_DECODE_PKT_NUM_MSG,
                                    VTSPR_Q_MULTI_DECODE_PKT_MSG_SIZE, 0)))
    {
        _VTSP_TRACE(__FILE__,__LINE__);
        return (-1);
    }
#endif
    /*
     * Set Multi-frame Decode queue for each object.
     * Initialize all stream object to point to a NULL iLBC decode object. If
     * the decode object is non-NULL, that stream may decode iLBC.
     */
    for (index = 0; index < _VTSP_STREAM_NUM; index++) {
#if defined(VTSP_ENABLE_G729_ACCELERATOR) || \
        defined(VTSP_ENABLE_G726_ACCELERATOR)
        OSAL_snprintf(qname, sizeof(qname), "vtsp-m10dec%dq", index);
        if (NULL == (q_ptr->data10Dec[index] =
                OSAL_msgQCreate(qname,
                OSAL_MODULE_AUDIO_VE, OSAL_MODULE_AUDIO_VE,
                OSAL_DATA_STRUCT__VTSPR_MultiDecodedMsg,
                VTSPR_Q_MULTI_DECODE_DEC_NUM_MSG,
                VTSPR_Q_MULTI_DECODE_DEC_MSG_SIZE, 0)))
        {
            _VTSP_TRACE(__FILE__,__LINE__);
            return (-1);
        }
#endif
#if defined(VTSP_ENABLE_ILBC) || defined(VTSP_ENABLE_G722P1) || \
         defined(VTSP_ENABLE_GAMRNB) || defined(VTSP_ENABLE_SILK) || \
         defined(VTSP_ENABLE_GAMRWB)
        OSAL_snprintf(qname, sizeof(qname), "vtsp-m20dec%dq", index);
        if (NULL == (q_ptr->data20Dec[index] =
                OSAL_msgQCreate(qname,
                OSAL_MODULE_AUDIO_VE, OSAL_MODULE_AUDIO_VE,
                OSAL_DATA_STRUCT__VTSPR_MultiDecodedMsg,
                VTSPR_Q_MULTI_DECODE_DEC_NUM_MSG,
                VTSPR_Q_MULTI_DECODE_DEC_MSG_SIZE, 0)))
        {
            _VTSP_TRACE(__FILE__,__LINE__);
            return (-1);
        }
#endif
#if defined(VTSP_ENABLE_ILBC) || defined(VTSP_ENABLE_G723)
        OSAL_snprintf(qname, sizeof(qname), "vtsp-m30dec%dq", index);
        if (NULL == (q_ptr->data30Dec[index] =
                OSAL_msgQCreate(qname,
                OSAL_MODULE_AUDIO_VE, OSAL_MODULE_AUDIO_VE,
                OSAL_DATA_STRUCT__VTSPR_MultiDecodedMsg,
                VTSPR_Q_MULTI_DECODE_DEC_NUM_MSG,
                VTSPR_Q_MULTI_DECODE_DEC_MSG_SIZE, 0)))
        {
            _VTSP_TRACE(__FILE__,__LINE__);
            return (-1);
        }
#endif
    }

    _VTSPR_FOR_ALL_INFC_PP(infc) {
        for (streamId = 0; streamId < _VTSP_STREAM_PER_INFC; streamId++) {
           stream_ptr = _VTSPR_streamIdToStreamPtr(dsp_ptr, infc, streamId);
           stream_ptr->pktData10 = q_ptr->data10Pkt;
           stream_ptr->pktData20 = q_ptr->data20Pkt;
           stream_ptr->pktData30 = q_ptr->data30Pkt;
           stream_ptr->decData10 = _VTSPR_streamIdToMultiDecQId(q_ptr, infc,
                   streamId, _VTSPR_MULTI_CODER_PKT_RATE_10);
           stream_ptr->decData20 = _VTSPR_streamIdToMultiDecQId(q_ptr, infc,
                   streamId, _VTSPR_MULTI_CODER_PKT_RATE_20);
           stream_ptr->decData30 = _VTSPR_streamIdToMultiDecQId(q_ptr, infc,
                   streamId, _VTSPR_MULTI_CODER_PKT_RATE_30);
           stream_ptr->multiDecObj.decOffset = 0;
        }
    }
    OSAL_taskDelay(1000);   /* XXX guarantee serial console is idle */
    
    /*
     * Create multi-frame decoded task semaphores to synchronize shutdown of
     * 10ms, 20ms and 30ms tasks, repsectively.
     */
#if (defined(VTSP_ENABLE_G729_ACCELERATOR) || \
        defined(VTSP_ENABLE_G726_ACCELERATOR))
    if (NULL == (multiDecTask10ms_ptr->finishSemId
                = OSAL_semCountCreate(0))) { 
        _VTSP_TRACE(__FILE__,__LINE__);
        return (-1);
    }
#endif
#if defined(VTSP_ENABLE_ILBC) || defined(VTSP_ENABLE_G722P1) || \
        defined(VTSP_ENABLE_GAMRNB) || defined(VTSP_ENABLE_SILK) || \
        defined(VTSP_ENABLE_GAMRWB)
    if (NULL == (multiDecTask20ms_ptr->finishSemId
                = OSAL_semCountCreate(0))) { 
        _VTSP_TRACE(__FILE__,__LINE__);
        return (-1);
    }
#endif
#if defined(VTSP_ENABLE_ILBC) || defined(VTSP_ENABLE_G723)
    if (NULL == (multiDecTask30ms_ptr->finishSemId
                = OSAL_semCountCreate(0))) { 
        _VTSP_TRACE(__FILE__,__LINE__);
        return (-1);
    }
#endif
    return(0);
}

/*
 * ======== _VTSPR_multiEncodeTaskInit() ========
 * Create tasks to handle multi-frame encoding of 10ms, 20ms and 30ms coder
 * types seperately. These tasks are required because the MHz used by the 
 * encoder needs to be distributed across 10, 20 or 30ms.
 */
vint _VTSPR_multiEncodeTaskInit(VTSPR_Obj *vtspr_ptr)
{
#if defined(VTSP_ENABLE_G729_ACCELERATOR) || \
        defined(VTSP_ENABLE_G726_ACCELERATOR) || \
        defined(VTSP_ENABLE_SILK) || defined(VTSP_ENABLE_G722P1) || \
        defined(VTSP_ENABLE_GAMRNB) || defined(VTSP_ENABLE_GAMRWB) || \
        defined(VTSP_ENABLE_ILBC) || defined(VTSP_ENABLE_G723)
    OSAL_TaskArg taskArg;
#endif
#if defined(VTSP_ENABLE_G729_ACCELERATOR) || \
        defined(VTSP_ENABLE_G726_ACCELERATOR)
    VTSPR_TaskContext *multiEncTask10ms_ptr;
#endif
#if defined(VTSP_ENABLE_ILBC) || defined(VTSP_ENABLE_G722P1) || \
        defined(VTSP_ENABLE_GAMRNB) || defined(VTSP_ENABLE_GAMRWB) || \
        defined(VTSP_ENABLE_SILK)
    VTSPR_TaskContext *multiEncTask20ms_ptr;
#endif
#if defined(VTSP_ENABLE_ILBC) || defined(VTSP_ENABLE_G723)
    VTSPR_TaskContext *multiEncTask30ms_ptr;
#endif

    /*
     * Create 10ms Task  
     */
#if defined(VTSP_ENABLE_G729_ACCELERATOR) || \
        defined(VTSP_ENABLE_G726_ACCELERATOR)
    multiEncTask10ms_ptr = &(vtspr_ptr->multiEncTask10ms);
    taskArg = multiEncTask10ms_ptr; /* or &myVar */
    multiEncTask10ms_ptr->taskEnable   = VTSPR_TASK_RUN;
    multiEncTask10ms_ptr->taskPriority = VTSPR_MULTI_ENCODE10_TASK_PRIORITY;
    multiEncTask10ms_ptr->stackSize    = VTSPR_MULTI_ENCODE10_TASK_STACK_SZ;
    
    multiEncTask10ms_ptr->taskId = OSAL_taskCreate("d2_multEncode10",
            multiEncTask10ms_ptr->taskPriority, multiEncTask10ms_ptr->stackSize,
            (void *)_VTSPR_multiEncode10Task, taskArg);
    if (NULL == multiEncTask10ms_ptr->taskId) {
        _VTSP_TRACE(__FILE__, __LINE__);
        return (-1);
    }
#endif

    /*
     * Create 20ms Task  
     */
#if defined(VTSP_ENABLE_ILBC) || defined(VTSP_ENABLE_G722P1) || \
        defined(VTSP_ENABLE_GAMRNB) || defined(VTSP_ENABLE_GAMRWB) || \
        defined(VTSP_ENABLE_SILK)
    multiEncTask20ms_ptr = &(vtspr_ptr->multiEncTask20ms);
    taskArg = multiEncTask20ms_ptr; /* or &myVar */
    multiEncTask20ms_ptr->taskEnable   = VTSPR_TASK_RUN;
    multiEncTask20ms_ptr->taskPriority = VTSPR_MULTI_ENCODE20_TASK_PRIORITY;
    multiEncTask20ms_ptr->stackSize    = VTSPR_MULTI_ENCODE20_TASK_STACK_SZ;
    
    multiEncTask20ms_ptr->taskId = OSAL_taskCreate("d2_multEncode20",
            multiEncTask20ms_ptr->taskPriority, multiEncTask20ms_ptr->stackSize,
            (void *)_VTSPR_multiEncode20Task, taskArg);
    if (NULL == multiEncTask20ms_ptr->taskId) {
        _VTSP_TRACE(__FILE__, __LINE__);
        return (-1);
    }
#endif
    
    /*
     * Create 30ms Task  
     */
#if defined(VTSP_ENABLE_ILBC) || defined(VTSP_ENABLE_G723)
    multiEncTask30ms_ptr = &(vtspr_ptr->multiEncTask30ms);
    taskArg = multiEncTask30ms_ptr; /* or &myVar */
    multiEncTask30ms_ptr->taskEnable   = VTSPR_TASK_RUN;
    multiEncTask30ms_ptr->taskPriority = VTSPR_MULTI_ENCODE30_TASK_PRIORITY;
    multiEncTask30ms_ptr->stackSize    = VTSPR_MULTI_ENCODE30_TASK_STACK_SZ;
    
    multiEncTask30ms_ptr->taskId = OSAL_taskCreate("d2_multEncode30",
            multiEncTask30ms_ptr->taskPriority, multiEncTask30ms_ptr->stackSize,
            (void *)_VTSPR_multiEncode30Task, taskArg);
    if (NULL == multiEncTask30ms_ptr->taskId) {
        _VTSP_TRACE(__FILE__, __LINE__);
        return (-1);
    }
#endif
    return (0);
}

/*
 * ======== _VTSPR_multiEncodeInit() ========
 */
vint _VTSPR_multiEncodeInit(
    VTSPR_Obj      *vtspr_ptr,
    VTSPR_Queues   *q_ptr,
    VTSPR_DSP      *dsp_ptr) 
{
    VTSPR_TaskContext *multiEncTask10ms_ptr;
    VTSPR_TaskContext *multiEncTask20ms_ptr;
    VTSPR_TaskContext *multiEncTask30ms_ptr;
    VTSPR_StreamObj   *stream_ptr;
    vint               infc;        /* must be signed */
    vint               streamId;
    vint               index;
    char               qname[16];

    /* 
     * Set Multi-Frame Encode task context pointers to be self contained
     */
    multiEncTask10ms_ptr          = &(vtspr_ptr->multiEncTask10ms);
    multiEncTask10ms_ptr->q_ptr   = q_ptr;
    multiEncTask10ms_ptr->dsp_ptr = dsp_ptr;

    multiEncTask20ms_ptr          = &(vtspr_ptr->multiEncTask20ms);
    multiEncTask20ms_ptr->q_ptr   = q_ptr;
    multiEncTask20ms_ptr->dsp_ptr = dsp_ptr;

    multiEncTask30ms_ptr          = &(vtspr_ptr->multiEncTask30ms);
    multiEncTask30ms_ptr->q_ptr   = q_ptr;
    multiEncTask30ms_ptr->dsp_ptr = dsp_ptr;

    /*
     * Create Multi-frame Encode task queues for 10, 20 and 30ms respectively.
     * The raw queue is shared, but one Enc queue for each encoder object
     * is created.
     */
#if defined(VTSP_ENABLE_G729_ACCELERATOR) || \
        defined(VTSP_ENABLE_G726_ACCELERATOR)
    if (NULL == (q_ptr->data10Raw = OSAL_msgQCreate(
                                    "vtsp-m10rawq",
                                    OSAL_MODULE_AUDIO_VE, OSAL_MODULE_AUDIO_VE,
                                    OSAL_DATA_STRUCT__VTSPR_MultiRawMsg,
                                    VTSPR_Q_MULTI_ENCODE_RAW_NUM_MSG,
                                    VTSPR_Q_MULTI_ENCODE_RAW_MSG_SIZE, 0)))
    {
        _VTSP_TRACE(__FILE__,__LINE__);
        return (-1);
    }
#endif
#if defined(VTSP_ENABLE_ILBC) || defined(VTSP_ENABLE_G722P1) || \
        defined(VTSP_ENABLE_GAMRNB) || defined(VTSP_ENABLE_SILK) || \
        defined(VTSP_ENABLE_GAMRWB)
    if (NULL == (q_ptr->data20Raw = OSAL_msgQCreate(
                                    "vtsp-m20rawq",
                                    OSAL_MODULE_AUDIO_VE, OSAL_MODULE_AUDIO_VE,
                                    OSAL_DATA_STRUCT__VTSPR_MultiRawMsg,
                                    VTSPR_Q_MULTI_ENCODE_RAW_NUM_MSG,
                                    VTSPR_Q_MULTI_ENCODE_RAW_MSG_SIZE, 0)))
    {
        _VTSP_TRACE(__FILE__,__LINE__);
        return (-1);
    }
#endif
#if defined(VTSP_ENABLE_ILBC) || defined(VTSP_ENABLE_G723)
    if (NULL == (q_ptr->data30Raw = OSAL_msgQCreate(
                                    "vtsp-m30rawq",
                                    OSAL_MODULE_AUDIO_VE, OSAL_MODULE_AUDIO_VE,
                                    OSAL_DATA_STRUCT__VTSPR_MultiRawMsg,
                                    VTSPR_Q_MULTI_ENCODE_RAW_NUM_MSG,
                                    VTSPR_Q_MULTI_ENCODE_RAW_MSG_SIZE, 0)))
    {
        _VTSP_TRACE(__FILE__,__LINE__);
        return (-1);
    }
#endif
    /*
     * Set Multi-frame Encode queue in object.
     * Initialize all stream object to point to a NULL iLBC encode object. If
     * the encode object is non-NULL, that stream may encode iLBC.
     */
    for (index = 0; index < _VTSP_STREAM_NUM; index++) {
        OSAL_snprintf(qname, sizeof(qname), "vtsp-m10enc%dq", index);
#if defined(VTSP_ENABLE_G729_ACCELERATOR) || \
    defined(VTSP_ENABLE_G726_ACCELERATOR)
        if (NULL == (q_ptr->data10Enc[index] =
                OSAL_msgQCreate(qname,
                OSAL_MODULE_AUDIO_VE, OSAL_MODULE_AUDIO_VE,
                OSAL_DATA_STRUCT__VTSPR_MultiCodedMsg,
                VTSPR_Q_MULTI_ENCODE_ENC_NUM_MSG,
                VTSPR_Q_MULTI_ENCODE_ENC_MSG_SIZE, 0)))
        {
            _VTSP_TRACE(__FILE__,__LINE__);
            return (-1);
        }
#endif
        OSAL_snprintf(qname, sizeof(qname), "vtsp-m20enc%dq", index);
#if defined(VTSP_ENABLE_ILBC) || defined(VTSP_ENABLE_G722P1) || \
    defined(VTSP_ENABLE_GAMRNB) || defined(VTSP_ENABLE_GAMRWB) || \
    defined(VTSP_ENABLE_SILK)
        if (NULL == (q_ptr->data20Enc[index] =
                OSAL_msgQCreate(qname,
                OSAL_MODULE_AUDIO_VE, OSAL_MODULE_AUDIO_VE,
                OSAL_DATA_STRUCT__VTSPR_MultiCodedMsg,
                VTSPR_Q_MULTI_ENCODE_ENC_NUM_MSG,
                VTSPR_Q_MULTI_ENCODE_ENC_MSG_SIZE, 0)))
        {
            _VTSP_TRACE(__FILE__,__LINE__);
            return (-1);
        }
#endif
        OSAL_snprintf(qname, sizeof(qname), "vtsp-m30enc%dq", index);
#if defined(VTSP_ENABLE_ILBC) || defined(VTSP_ENABLE_G723)
        if (NULL == (q_ptr->data30Enc[index] =
                OSAL_msgQCreate(qname,
                OSAL_MODULE_AUDIO_VE, OSAL_MODULE_AUDIO_VE,
                OSAL_DATA_STRUCT__VTSPR_MultiCodedMsg,
                VTSPR_Q_MULTI_ENCODE_ENC_NUM_MSG,
                VTSPR_Q_MULTI_ENCODE_ENC_MSG_SIZE, 0)))
        {
            _VTSP_TRACE(__FILE__,__LINE__);
            return (-1);
        }
#endif
    }

    _VTSPR_FOR_ALL_INFC_PP(infc) {
        for (streamId = 0; streamId < _VTSP_STREAM_PER_INFC; streamId++) {
           stream_ptr = _VTSPR_streamIdToStreamPtr(dsp_ptr, infc, streamId);
           stream_ptr->rawData10 = q_ptr->data10Raw;
           stream_ptr->rawData20 = q_ptr->data20Raw;
           stream_ptr->rawData30 = q_ptr->data30Raw;
           stream_ptr->encData10 = _VTSPR_streamIdToMultiEncQId(q_ptr, infc,
                   streamId, _VTSPR_MULTI_CODER_PKT_RATE_10);
           stream_ptr->encData20 = _VTSPR_streamIdToMultiEncQId(q_ptr, infc,
                   streamId, _VTSPR_MULTI_CODER_PKT_RATE_20);
           stream_ptr->encData30 = _VTSPR_streamIdToMultiEncQId(q_ptr, infc,
                   streamId, _VTSPR_MULTI_CODER_PKT_RATE_30);
           stream_ptr->multiEncObj.encOffset = 0;
        }
    }
    OSAL_taskDelay(1000);   /* XXX guarantee serial console is idle */
    
    /*
     * Create multi-frame encoded task semaphores to synchronize shutdown of
     * 10ms, 20ms and 30ms tasks, repsectively.
     */
#if (defined(VTSP_ENABLE_G729_ACCELERATOR) || \
        defined(VTSP_ENABLE_G726_ACCELERATOR))
    if (NULL == (multiEncTask10ms_ptr->finishSemId
                = OSAL_semCountCreate(0))) { 
        _VTSP_TRACE(__FILE__,__LINE__);
        return (-1);
    }
#endif
#if defined(VTSP_ENABLE_ILBC) || defined(VTSP_ENABLE_G722P1) || defined(VTSP_ENABLE_GAMRNB) || defined(VTSP_ENABLE_GAMRWB) || defined(VTSP_ENABLE_SILK)
    if (NULL == (multiEncTask20ms_ptr->finishSemId
                = OSAL_semCountCreate(0))) { 
        _VTSP_TRACE(__FILE__,__LINE__);
        return (-1);
    }
#endif
#if defined(VTSP_ENABLE_ILBC) || defined(VTSP_ENABLE_G723)
    if (NULL == (multiEncTask30ms_ptr->finishSemId
                = OSAL_semCountCreate(0))) { 
        _VTSP_TRACE(__FILE__,__LINE__);
        return (-1);
    }
#endif 
    return(0);
}

/*
 * ======== _VTSPR_multiCoderInit() ========
 * Initialize multi-coder module
 */
void _VTSPR_multiCoderInit(
    VTSPR_StreamObj *stream_ptr,
    vint             encDec,
    vint             coderType)
{
    _VTSPR_MultiCodedMsg    encMsg;
    _VTSPR_MultiDecodedMsg  decMsg;
    _VTSPR_MultiEncObj   *multiEncObj_ptr;
    _VTSPR_MultiDecObj   *multiDecObj_ptr;
    vint                  extension;

    switch (encDec) {
        case VTSPR_DECODER:
            multiDecObj_ptr = &(stream_ptr->multiDecObj);
            /*
             * Drain the queue of any unprocessed payloads.
             */
            while (0 < OSAL_msgQRecv(multiDecObj_ptr->decData,
                    (char *)&decMsg, VTSPR_Q_MULTI_DECODE_DEC_MSG_SIZE,
                    OSAL_NO_WAIT, NULL)) {
            }
            /*
             * Now initialize the local decode object.
             * Assume init is steady state, plc not active
             */
            OSAL_memSet(&(multiDecObj_ptr->pktMsg), 0,
                        VTSPR_Q_MULTI_DECODE_PKT_MSG_SIZE);
            OSAL_memSet(multiDecObj_ptr->recvSpeech_ary, 0, 
                    VTSPR_MULTI_NSAMPLE_MAX * sizeof(vint));
            OSAL_memSet(multiDecObj_ptr->playSpeech_ary, 0, 
                    VTSPR_MULTI_NSAMPLE_MAX * sizeof(vint));
            multiDecObj_ptr->decOffset        = 0;
            multiDecObj_ptr->pktMsg.initFlag  = 1;
            multiDecObj_ptr->pktMsg.infc      = 0;
            multiDecObj_ptr->pktMsg.streamId  = 0;
            multiDecObj_ptr->plcActive        = 0;
            multiDecObj_ptr->lastFrameSize    = 0;
            multiDecObj_ptr->pktMsg.decType   = coderType;

            /* Set extention for multi-bitrate codecs */
            extension = stream_ptr->streamParam.extension;
            multiDecObj_ptr->pktMsg.extension = extension;
            
            switch (coderType) {
#ifdef VTSP_ENABLE_ILBC
                case VTSP_CODER_ILBC_20MS:
                    multiDecObj_ptr->pktMsg.payloadSize = VTSP_BLOCK_ILBC_20MS_SZ;
                    multiDecObj_ptr->decMsgSz         = ILBC_BLOCK_LENGTH_20MS;
                    multiDecObj_ptr->coderSamples10ms = VTSPR_NSAMPLES_10MS_8K;
                    multiDecObj_ptr->pktData          = stream_ptr->pktData20;
                    multiDecObj_ptr->decData          = stream_ptr->decData20;
                    break;
                case VTSP_CODER_ILBC_30MS:
                    multiDecObj_ptr->pktMsg.payloadSize = VTSP_BLOCK_ILBC_30MS_SZ;
                    multiDecObj_ptr->decMsgSz         = ILBC_BLOCK_LENGTH_30MS;
                    multiDecObj_ptr->coderSamples10ms = VTSPR_NSAMPLES_10MS_8K;
                    multiDecObj_ptr->pktData          = stream_ptr->pktData30;
                    multiDecObj_ptr->decData          = stream_ptr->decData30;
                    break;
#endif
#ifdef VTSP_ENABLE_G723
                case VTSP_CODER_G723_30MS:
                    multiDecObj_ptr->decMsgSz = G723A_FRAME;
                    if (0 != (extension & VTSP_MASK_EXT_G723_53)) {
                        multiDecObj_ptr->pktMsg.payloadSize
                           = VTSP_BLOCK_G723_30MS_53_SZ;
                    }
                    else {
                        multiDecObj_ptr->pktMsg.payloadSize
                            = VTSP_BLOCK_G723_30MS_63_SZ;
                    }
                    multiDecObj_ptr->coderSamples10ms = VTSPR_NSAMPLES_10MS_8K;
                    multiDecObj_ptr->pktData          = stream_ptr->pktData30;
                    multiDecObj_ptr->decData          = stream_ptr->decData30;
                    break;
#endif
#ifdef VTSP_ENABLE_G722P1
                case VTSP_CODER_G722P1_20MS:
                    multiDecObj_ptr->decMsgSz = G722P1_SPEECH_BUFSZ_16KHZ;
                    if (0 != (extension & VTSP_MASK_EXT_G722P1_32)) {
                        multiDecObj_ptr->pktMsg.payloadSize =
                               VTSP_BLOCK_G722P1_32KBPS_20MS_SZ;
                    }
                    else {
                        multiDecObj_ptr->pktMsg.payloadSize =
                                VTSP_BLOCK_G722P1_24KBPS_20MS_SZ;
                    }
                    multiDecObj_ptr->coderSamples10ms
                        = VTSPR_NSAMPLES_10MS_16K;
                    multiDecObj_ptr->pktData        = stream_ptr->pktData20;
                    multiDecObj_ptr->decData        = stream_ptr->decData20;
                    /* XXX multiDecObj_ptr->g722p1decObj.scratch_ptr =
                            (void *) multiDecObj_ptr->g722p1Scratch_ptr; */
                    break;
#endif 
#ifdef VTSP_ENABLE_GAMRNB
                case VTSP_CODER_GAMRNB_20MS_OA:
                    multiDecObj_ptr->decMsgSz         = 2 * VTSPR_NSAMPLES_10MS_8K;
                    multiDecObj_ptr->coderSamples10ms = VTSPR_NSAMPLES_10MS_8K;
                    multiDecObj_ptr->pktData          = stream_ptr->pktData20;
                    multiDecObj_ptr->decData          = stream_ptr->decData20;
                    GAMRNB_decodeInit(&(multiDecObj_ptr->gamrnbDecObj),
                            GAMRNB_SPEECH_FORMAT_LINEAR,
                            GAMRNB_DATA_FORMAT_PACKED_OA,
                            GAMRNB_BIT_MASK_13BIT);
                    break;
                case VTSP_CODER_GAMRNB_20MS_BE:
                    multiDecObj_ptr->decMsgSz         = 2 * VTSPR_NSAMPLES_10MS_8K;
                    multiDecObj_ptr->coderSamples10ms = VTSPR_NSAMPLES_10MS_8K;
                    multiDecObj_ptr->pktData          = stream_ptr->pktData20;
                    multiDecObj_ptr->decData          = stream_ptr->decData20;
                    GAMRNB_decodeInit(&(multiDecObj_ptr->gamrnbDecObj),
                            GAMRNB_SPEECH_FORMAT_LINEAR,
                            GAMRNB_DATA_FORMAT_PACKED_BE,
                            GAMRNB_BIT_MASK_13BIT);
                    break;
#endif
#ifdef VTSP_ENABLE_GAMRWB
                case VTSP_CODER_GAMRWB_20MS_OA:
                    multiDecObj_ptr->decMsgSz         = 2 * VTSPR_NSAMPLES_10MS_16K;
                    multiDecObj_ptr->coderSamples10ms = VTSPR_NSAMPLES_10MS_16K;
                    multiDecObj_ptr->pktData          = stream_ptr->pktData20;
                    multiDecObj_ptr->decData          = stream_ptr->decData20;
                    GAMRWB_decoderInit(&(multiDecObj_ptr->gamrwbDecObj),
                            GAMRWB_DATA_FORMAT_PACKED_IF1);
                    break;
                case VTSP_CODER_GAMRWB_20MS_BE:
                    multiDecObj_ptr->decMsgSz         = 2 * VTSPR_NSAMPLES_10MS_16K;
                    multiDecObj_ptr->coderSamples10ms = VTSPR_NSAMPLES_10MS_16K;
                    multiDecObj_ptr->pktData          = stream_ptr->pktData20;
                    multiDecObj_ptr->decData          = stream_ptr->decData20;
                    GAMRWB_decoderInit(&(multiDecObj_ptr->gamrwbDecObj),
                            GAMRWB_DATA_FORMAT_PACKED_IF2);
                    break;
#endif
#ifdef VTSP_ENABLE_SILK
                case VTSP_CODER_SILK_20MS_8K:
                                _VTSP_TRACE(__FUNCTION__,__LINE__);
                    multiDecObj_ptr->pktMsg.payloadSize = 
                            VTSP_BLOCK_SILK_20MS_SZ;
                    multiDecObj_ptr->decMsgSz         
                            = SILK_SPEECH_BUFSZ_20MS_8K;
                    multiDecObj_ptr->coderSamples10ms = VTSPR_NSAMPLES_10MS_8K;
                    multiDecObj_ptr->pktData          = stream_ptr->pktData20;
                    multiDecObj_ptr->decData          = stream_ptr->decData20;
                    break;
                case VTSP_CODER_SILK_20MS_16K:
                    _VTSP_TRACE(__FUNCTION__,__LINE__);
                    multiDecObj_ptr->pktMsg.payloadSize = 
                            VTSP_BLOCK_SILK_20MS_SZ;
                    multiDecObj_ptr->decMsgSz         
                            = SILK_SPEECH_BUFSZ_20MS_16K;
                    multiDecObj_ptr->coderSamples10ms = VTSPR_NSAMPLES_10MS_16K;
                    multiDecObj_ptr->pktData          = stream_ptr->pktData20;
                    multiDecObj_ptr->decData          = stream_ptr->decData20;
                    break;
/* XXX               case VTSP_CODER_SILK_20MS:
                    multiDecObj_ptr->pktMsg.payloadSize = 
                            VTSP_BLOCK_SILK_20MS_SZ;
                    multiDecObj_ptr->decMsgSz         
                            = SILK_SPEECH_BUFSZ_20MS_24K;
                    multiDecObj_ptr->coderSamples10ms = VTSPR_NSAMPLES_10MS_24K;
                    multiDecObj_ptr->pktData          = stream_ptr->pktData20;
                    multiDecObj_ptr->decData          = stream_ptr->decData20;
                    break; */
#endif
#ifdef VTSP_ENABLE_G729_ACCELERATOR
                case VTSP_CODER_G729:
                    multiDecObj_ptr->decMsgSz         = COMM_10MS_LEN;
                    multiDecObj_ptr->coderSamples10ms = VTSPR_NSAMPLES_10MS_8K;
                    multiDecObj_ptr->pktData          = stream_ptr->pktData10;
                    multiDecObj_ptr->decData          = stream_ptr->decData10;
                    break;
#endif
#ifdef VTSP_ENABLE_G726_ACCELERATOR
                case VTSP_CODER_G726_32K:
                    multiDecObj_ptr->decMsgSz         = COMM_10MS_LEN;
                    multiDecObj_ptr->coderSamples10ms = VTSPR_NSAMPLES_10MS_8K;
                    multiDecObj_ptr->pktData          = stream_ptr->pktData10;
                    multiDecObj_ptr->decData          = stream_ptr->decData10;
                    break;
#endif
                default:;
                    _VTSP_TRACE(__FUNCTION__,__LINE__);
                    /* should not get here */
            }
            break;
        case VTSPR_ENCODER:
            multiEncObj_ptr = &(stream_ptr->multiEncObj);
            /*
             * Drain the queue of any unprocessed payloads.
             */
            while (0 < OSAL_msgQRecv(multiEncObj_ptr->encData, (char *)&encMsg,
                    VTSPR_Q_MULTI_ENCODE_ENC_MSG_SIZE, OSAL_NO_WAIT, NULL)) {
            }

            /*
             * Now initialize the local encode object.
             */
            OSAL_memSet(&(multiEncObj_ptr->rawMsg), 0,
                        VTSPR_Q_MULTI_ENCODE_RAW_MSG_SIZE);
            multiEncObj_ptr->encOffset       = 0;
            multiEncObj_ptr->rawMsg.initFlag = 1;
            multiEncObj_ptr->rawMsg.infc     = 0;
            multiEncObj_ptr->rawMsg.streamId = 0;
            multiEncObj_ptr->rawMsg.encType  = coderType;

            /* Set extention for multi-bitrate codecs */
            extension = stream_ptr->streamParam.extension;
            multiEncObj_ptr->rawMsg.extension = extension;
            
            switch (coderType) {
#ifdef VTSP_ENABLE_ILBC
                case VTSP_CODER_ILBC_20MS:
                    multiEncObj_ptr->rawMsg.msgSize   = ILBC_BLOCK_LENGTH_20MS;
                    multiEncObj_ptr->encMsgSz         = VTSP_BLOCK_ILBC_20MS_SZ;
                    multiEncObj_ptr->coderSamples10ms = VTSPR_NSAMPLES_10MS_8K;
                    multiEncObj_ptr->rawData          = stream_ptr->rawData20;
                    multiEncObj_ptr->encData          = stream_ptr->encData20;
                    break;
                case VTSP_CODER_ILBC_30MS:
                    multiEncObj_ptr->rawMsg.msgSize   = ILBC_BLOCK_LENGTH_30MS;
                    multiEncObj_ptr->encMsgSz         = VTSP_BLOCK_ILBC_30MS_SZ;
                    multiEncObj_ptr->coderSamples10ms = VTSPR_NSAMPLES_10MS_8K;
                    multiEncObj_ptr->rawData          = stream_ptr->rawData30;
                    multiEncObj_ptr->encData          = stream_ptr->encData30;
                    break;
#endif
#ifdef VTSP_ENABLE_G723
                case VTSP_CODER_G723_30MS:
                    multiEncObj_ptr->rawMsg.msgSize   = G723A_FRAME;
                    if (0 != (extension & VTSP_MASK_EXT_G723_53)) {
                        multiEncObj_ptr->encMsgSz
                           = VTSP_BLOCK_G723_30MS_53_SZ;
                    }
                    else {
                        multiEncObj_ptr->encMsgSz
                            = VTSP_BLOCK_G723_30MS_63_SZ;
                    }
                    multiEncObj_ptr->coderSamples10ms = VTSPR_NSAMPLES_10MS_8K;
                    multiEncObj_ptr->rawData          = stream_ptr->rawData30;
                    multiEncObj_ptr->encData          = stream_ptr->encData30;
                    break;
#endif
#ifdef VTSP_ENABLE_G722P1
                case VTSP_CODER_G722P1_20MS:
                    multiEncObj_ptr->rawMsg.msgSize = G722P1_SPEECH_BUFSZ_16KHZ;
                    if (0 != (extension & VTSP_MASK_EXT_G722P1_32)) {
                        multiEncObj_ptr->encMsgSz
                           = VTSP_BLOCK_G722P1_32KBPS_20MS_SZ;
                    }
                    else {
                        multiEncObj_ptr->encMsgSz
                            = VTSP_BLOCK_G722P1_24KBPS_20MS_SZ;
                    }
                    multiEncObj_ptr->coderSamples10ms
                        = VTSPR_NSAMPLES_10MS_16K;
                    multiEncObj_ptr->rawData        = stream_ptr->rawData20;
                    multiEncObj_ptr->encData        = stream_ptr->encData20;
                    break;
#endif
#ifdef VTSP_ENABLE_GAMRNB
                case VTSP_CODER_GAMRNB_20MS_OA:
                    /* Octet-Align format. */
                    multiEncObj_ptr->rawMsg.msgSize   =
                            (2 * VTSPR_NSAMPLES_10MS_16K);
                    /* Always set to maximum encoded size. */
                    multiEncObj_ptr->encMsgSz =
                                VTSP_BLOCK_GAMRNB_20MS_OA_MR122_SZ; 

                    multiEncObj_ptr->coderSamples10ms = VTSPR_NSAMPLES_10MS_8K;
                    multiEncObj_ptr->rawData          = stream_ptr->rawData20;
                    multiEncObj_ptr->encData          = stream_ptr->encData20;
                    break;
                case VTSP_CODER_GAMRNB_20MS_BE:
                    /* Bandwidth-Efficient format. */
                    multiEncObj_ptr->rawMsg.msgSize   =
                            (2 * VTSPR_NSAMPLES_10MS_16K);
                    /* Always set to maximum encoded size. */
                    multiEncObj_ptr->encMsgSz =
                                VTSP_BLOCK_GAMRNB_20MS_BE_MR122_SZ; 

                    multiEncObj_ptr->coderSamples10ms = VTSPR_NSAMPLES_10MS_8K;
                    multiEncObj_ptr->rawData          = stream_ptr->rawData20;
                    multiEncObj_ptr->encData          = stream_ptr->encData20;
                    break;
#endif
#ifdef VTSP_ENABLE_GAMRWB
                case VTSP_CODER_GAMRWB_20MS_OA:
                    /* Octet-Align format. */
                    multiEncObj_ptr->rawMsg.msgSize   =
                            (2 * VTSPR_NSAMPLES_10MS_16K);
                    /* Always set to maximum encoded size. */
                    multiEncObj_ptr->encMsgSz =
                            VTSP_BLOCK_GAMRWB_20MS_OA_MR2385_SZ;

                    multiEncObj_ptr->coderSamples10ms = VTSPR_NSAMPLES_10MS_16K;
                    multiEncObj_ptr->rawData          = stream_ptr->rawData20;
                    multiEncObj_ptr->encData          = stream_ptr->encData20;
                    break;
                case VTSP_CODER_GAMRWB_20MS_BE:
                    /* Bandwidth-Efficient format. */
                    multiEncObj_ptr->rawMsg.msgSize   =
                            (2 * VTSPR_NSAMPLES_10MS_16K);
                    /* Always set to maximum encoded size. */
                    multiEncObj_ptr->encMsgSz =
                            VTSP_BLOCK_GAMRWB_20MS_BE_MR2385_SZ;
                    multiEncObj_ptr->coderSamples10ms = VTSPR_NSAMPLES_10MS_16K;
                    multiEncObj_ptr->rawData          = stream_ptr->rawData20;
                    multiEncObj_ptr->encData          = stream_ptr->encData20;
                    break;
#endif
#ifdef VTSP_ENABLE_SILK
                _VTSP_TRACE(__FUNCTION__,__LINE__);
                case VTSP_CODER_SILK_20MS_8K:
                    _VTSP_TRACE(__FUNCTION__,__LINE__);
                    multiEncObj_ptr->rawMsg.msgSize   
                            = SILK_SPEECH_BUFSZ_20MS_8K;
                    multiEncObj_ptr->encMsgSz         = VTSP_BLOCK_SILK_20MS_SZ;
                    multiEncObj_ptr->coderSamples10ms = VTSPR_NSAMPLES_10MS_8K;
                    multiEncObj_ptr->rawData          = stream_ptr->rawData20;
                    multiEncObj_ptr->encData          = stream_ptr->encData20;
                    break;

                case VTSP_CODER_SILK_20MS_16K:
                    _VTSP_TRACE(__FUNCTION__,__LINE__);
                    multiEncObj_ptr->rawMsg.msgSize   
                            = SILK_SPEECH_BUFSZ_20MS_16K;
                    multiEncObj_ptr->encMsgSz         = VTSP_BLOCK_SILK_20MS_SZ;
                    multiEncObj_ptr->coderSamples10ms = VTSPR_NSAMPLES_10MS_16K;
                    multiEncObj_ptr->rawData          = stream_ptr->rawData20;
                    multiEncObj_ptr->encData          = stream_ptr->encData20;
                    break;
/* XXX SILK 24K not supported yet XXX
                case VTSP_CODER_SILK_20MS_24K:
                    multiEncObj_ptr->rawMsg.msgSize   
                            = SILK_SPEECH_BUFSZ_20MS_16K;
                    multiEncObj_ptr->encMsgSz         = VTSP_BLOCK_SILK_20MS_SZ;
                    multiEncObj_ptr->coderSamples10ms = VTSPR_NSAMPLES_10MS_16K;
                    multiEncObj_ptr->rawData          = stream_ptr->rawData20;
                    multiEncObj_ptr->encData          = stream_ptr->encData20;
                    break;
*/            
#endif
#ifdef VTSP_ENABLE_G729_ACCELERATOR
                case VTSP_CODER_G729:
                    multiEncObj_ptr->rawMsg.msgSize   = COMM_10MS_LEN;
                    multiEncObj_ptr->encMsgSz         = VTSP_BLOCK_G729_10MS_SZ;
                    multiEncObj_ptr->coderSamples10ms = VTSPR_NSAMPLES_10MS_8K;
                    multiEncObj_ptr->rawData          = stream_ptr->rawData10;
                    multiEncObj_ptr->encData          = stream_ptr->encData10;
                    break;
#endif 
#ifdef VTSP_ENABLE_G726_ACCELERATOR
                case VTSP_CODER_G726_32K:
                    multiEncObj_ptr->rawMsg.msgSize   = COMM_10MS_LEN;
                    multiEncObj_ptr->encMsgSz     
                            = VTSP_BLOCK_G726_32K_10MS_SZ;
                    multiEncObj_ptr->coderSamples10ms = VTSPR_NSAMPLES_10MS_8K;
                    multiEncObj_ptr->rawData          = stream_ptr->rawData10;
                    multiEncObj_ptr->encData          = stream_ptr->encData10;
                    break;
#endif 
                default:;
                    _VTSP_TRACE(__FUNCTION__,__LINE__);
                    /* should not get here */
            }
            break;
        default:;
            /* Should not get here */                    
    }
}

