/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004-2010 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 28422 $ $Date: 2014-08-22 11:55:05 +0800 (Fri, 22 Aug 2014) $
 *
 */
#include "vtspr.h"
#include "_vtspr_private.h"

/* 
 * vTSPR structure, private to VTSPR
 */
VTSPR_Obj VTSPR_obj;
 
/* 
 * ======== VTSPR_printInfo() ========
 *
 */
vint _VTSPR_printInfo(void)
{
    VTSPR_Obj        *vtspr_ptr;
    VTSPR_Queues     *q_ptr;
    VTSPR_DSP        *dsp_ptr;
    vint              infc;
    vint              streamId;
    VTSPR_StreamObj  *stream_ptr;
    VTSPR_ChanObj    *chan_ptr;

    vtspr_ptr = &VTSPR_obj;
    q_ptr = vtspr_ptr->q_ptr;
    dsp_ptr = vtspr_ptr->dsp_ptr;

    OSAL_logMsg("========\n");
    OSAL_logMsg("VTSPR_Obj %p  taskEnable 0x%0x taskId %p\n",
            vtspr_ptr, vtspr_ptr->task10ms.taskEnable,
            vtspr_ptr->task10ms.taskId);
    OSAL_logMsg("VTSPR eventGlobalQ %p"
            " rtcpMsgQ %p\n"
            " rtcpEventQ %p\n",
            q_ptr->eventGlobalQ, q_ptr->rtcpMsg, 
            q_ptr->rtcpEvent);
    OSAL_logMsg("========\n");
    OSAL_logMsg(" VTSPR_DSP %p\n",
            dsp_ptr);

    _VTSPR_FOR_ALL_INFC(infc) {
        chan_ptr = _VTSPR_infcToChanPtr(dsp_ptr, infc);
        OSAL_logMsg("VTSPR FX Infc %d infcType %d\n", 
                infc, _VTSPR_infcToType(infc));
        OSAL_logMsg("    EventQ %p  CIDQ %p\n",
                q_ptr->eventInfcQ[infc],
                q_ptr->cidQ[infc]);
        OSAL_logMsg("    chanState 0x%0x\n", 
                chan_ptr->algChannelState);
        for (streamId = _VTSP_STREAM_PER_INFC - 1; streamId >= 0; streamId--) { 
            stream_ptr = _VTSPR_streamIdToStreamPtr(dsp_ptr, infc, streamId);
            OSAL_logMsg("  StreamId %d  streamState 0x%0x  streamObj %p\n",
                    streamId, stream_ptr->algStreamState, 
                    stream_ptr);
            OSAL_logMsg("       dir %d  peer %d "
                    "extension 0x%0x confMask 0x%0x\n",
                    stream_ptr->streamParam.dir,
                    stream_ptr->streamParam.peer, 
                    stream_ptr->streamParam.extension, 
                    stream_ptr->streamParam.confMask);
            if (0 != stream_ptr->streamParam.dir) {
                OSAL_logMsg("       remote: ip %x rtpPort %d rtcpPort %d\n",
                        stream_ptr->streamParam.remoteAddr.ipv4,
                        stream_ptr->streamParam.remoteAddr.port,
                        stream_ptr->streamParam.remoteControlPort);
                OSAL_logMsg("       local: ip %x rtpPort %d rtcpPort %d\n",
                        stream_ptr->streamParam.localAddr.ipv4,
                        stream_ptr->streamParam.localAddr.port,
                        stream_ptr->streamParam.localControlPort);
            }
            OSAL_logMsg("       encoder %d  decoder %d "
                    "silenceComp 0x%x dtmfRelay 0x%x\n",
                    stream_ptr->streamParam.encoder,
                    stream_ptr->decoderType, 
                    stream_ptr->streamParam.silenceComp, 
                    stream_ptr->streamParam.dtmfRelay);
            OSAL_logMsg("       countDecode 0x%x countEncode 0x%x\n" 
                        "       countNSE 0x%x countCN 0x%x\n",
                    stream_ptr->countDecode,
                    stream_ptr->countEncode, 
                    stream_ptr->countNSE, 
                    stream_ptr->countCN);
        }
    }

    OSAL_logMsg("\nVTSPR CPU use (x 100): Avg %d   High %d\n",
            dsp_ptr->measureAvg, dsp_ptr->measureHi);

    return (0xc0ffee);
}

/*
 * ======== VTSPR_init() ========
 *
 * Soft-dsp initialization.
 *
 * Returns:  
 *  zero    = successful init
 *  nonzero = bit mask of error code
 *
 */
OSAL_TaskId VTSPR_init(
    void)
{
    VTSPR_Obj    *vtspr_ptr;

    /* Init the device private data object 
     */
    vtspr_ptr = &VTSPR_obj;
    OSAL_memSet(vtspr_ptr, 0, sizeof(VTSPR_obj)); 

    /*
     * Initialize for waiting, Start the task.
     */
    vtspr_ptr->task10ms.taskEnable = VTSPR_TASK_WAIT;
#if defined(OSAL_LKM) || defined (OSAL_KERNEL_EMULATION)
    /* 
     * LKM uses compile-time priority and fixed stack size.
     * XXX future enhancement could allow 'insmod' command line
     * to specify these as user initialization variables.
     */
     vtspr_ptr->task10ms.taskPriority = VTSPR_TASK_PRIORITY;
     vtspr_ptr->task10ms.stackSize = VTSPR_TASK_STACK_SZ;
#else
    /* 
     * RTOS allows application to configure priority and 
     * additional stack size.
     */
    vtspr_ptr->task10ms.taskPriority = _VTSP_object_ptr->taskPriority;
    vtspr_ptr->task10ms.stackSize
        = VTSPR_TASK_STACK_SZ + _VTSP_object_ptr->taskAddStackSz;
#endif
    vtspr_ptr->task10ms.taskId = OSAL_taskCreate("vtspr",
            (OSAL_TaskPrio)vtspr_ptr->task10ms.taskPriority,
            vtspr_ptr->task10ms.stackSize,
            (OSAL_TaskPtr)VTSPR_postInit, NULL);

    return (vtspr_ptr->task10ms.taskId);
}

/*
 * ======== VTSPR_postInit() ========
 *
 * Soft-dsp initialization.
 *
 * Returns:  
 *  zero    = successful init
 *  nonzero = bit mask of error code
 *
 */
vint VTSPR_postInit(
    void)
{
    VTSPR_Obj    *vtspr_ptr;
    VTSPR_Queues *q_ptr;
    VTSPR_DSP    *dsp_ptr;
    TIC_Obj      *tic_ptr;
    _VTSPR_RtpObject  *rtp_ptr;
    _VTSPR_RtcpObject *rtcp_ptr;
    vint          infcType;
    vint          chan;
    vint          result;
    vint          infc;                   /* must be signed */
#if defined(VTSP_ENABLE_PLAY) || defined(VTSP_ENABLE_RECORD)
    vint          flowIndex;
    vint          flowNum;
#endif
    vint          index;
    vint          ret_val;
    vint          streamId;
    char          qname[16];

    vtspr_ptr = &VTSPR_obj;

    /*
     * Alloc & Init Queue Obj Structure
     */
    if (NULL == (vtspr_ptr->q_ptr = OSAL_memCalloc(1, 
                    sizeof(VTSPR_Queues), 0))) { 
        _VTSP_TRACE(__FILE__,__LINE__);
        return (VTSP_E_INIT);
    }
    q_ptr = vtspr_ptr->q_ptr;

    /* 
     * Alloc & Init voice processing context
     */
    if (NULL == (vtspr_ptr->dsp_ptr = OSAL_memCalloc(1, 
                    sizeof(VTSPR_DSP), 0))) { 
        OSAL_logMsg("%s:%d fatal malloc error, size=%d\n", 
                __FILE__, __LINE__, sizeof(VTSPR_DSP));

        return (VTSP_E_INIT);
    }

    dsp_ptr = vtspr_ptr->dsp_ptr;

    for (index = 0; index < _VTSP_STREAM_NUM; index++) {
        if (NULL == (dsp_ptr->streamObj_ptr[index]
                        = (VTSPR_StreamObj *) OSAL_memCalloc(1,
                                sizeof(VTSPR_StreamObj), 0))) { 
            _VTSP_TRACE(__FILE__,__LINE__);
            return (VTSP_E_INIT);
        }
        if (NULL == (dsp_ptr->jbBuffer_ptr[index]
                        = (JB_Pkt *) OSAL_memCalloc(1,
                                 JB_MEM_SIZE(_VTSPR_JB_MAXLEVEL), 0))) { 
            _VTSP_TRACE(__FILE__,__LINE__);
            return (VTSP_E_INIT);
        }
        dsp_ptr->streamObj_ptr[index]->jbObj.pktStart_ptr
            = dsp_ptr->jbBuffer_ptr[index];
#ifdef VTSP_ENABLE_G722P1
        dsp_ptr->streamObj_ptr[index]->multiDecObj.g722p1Scratch_ptr
            = dsp_ptr->g722p1DecScratch_ary;
#endif
    }

    /*
     * Alloc & Init block exchange context
     */
    if (NULL == (vtspr_ptr->net_ptr = OSAL_memCalloc(1, 
                    sizeof(VTSPR_NetObj), 0))) { 
        _VTSP_TRACE(__FILE__,__LINE__);
        return (VTSP_E_INIT);
    }

#ifdef VTSP_ENABLE_NETLOG
    vtspr_ptr->netlog_ptr = OSAL_memCalloc(1, sizeof(VTSPR_Netlog), 0);
    _VTSPR_netlogInit(vtspr_ptr);
#endif

    /*
     * Set Q context
     * --------
     * This Q context is created by VTSP layer.
     *
     * In protected mode operating systems, the OSAL KERNUSER argument will
     * connect the kernel-side queue to the user-side queue using the ID.
     *
     * In RTOS, the KERNUSER argument is ignored. The queues are assigned the
     * same qId/memory space using the ID as a lookup.
     */
    if (NULL == (q_ptr->cmdQ = OSAL_msgQCreate(
                "vtsp-cmdq",
                OSAL_MODULE_AUDIO_VE, OSAL_MODULE_AUDIO_VE,
                OSAL_DATA_STRUCT__VTSP_CmdMsg,
                _VTSP_Q_CMD_NUM_MSG,
                _VTSP_Q_CMD_MSG_SZ,
                0))) { 
        _VTSP_TRACE(__FILE__,__LINE__);
        goto critical_error;
    }
    if (NULL == (q_ptr->eventGlobalQ = 
                OSAL_msgQCreate(
                "vtsp-globalq",
                OSAL_MODULE_AUDIO_VE, OSAL_MODULE_AUDIO_VE,
                OSAL_DATA_STRUCT_VTSP_EventMsg,
                _VTSP_Q_GLOBAL_NUM_MSG,
                _VTSP_Q_GLOBAL_MSG_SZ,
                0))) { 
        _VTSP_TRACE(__FILE__,__LINE__);
        goto critical_error;
    }

    _VTSPR_FOR_ALL_INFC(infc) {
        OSAL_snprintf(qname, sizeof(qname), "vtsp-infc%dq", infc);
        if (NULL == (q_ptr->eventInfcQ[infc] =
                    OSAL_msgQCreate(
                    qname,
                    OSAL_MODULE_AUDIO_VE, OSAL_MODULE_AUDIO_VE,
                    OSAL_DATA_STRUCT_VTSP_EventMsg,
                    _VTSP_Q_EVENT_NUM_MSG,
                    _VTSP_Q_EVENT_MSG_SZ,
                    0))) { 
            _VTSP_TRACE(__FILE__,__LINE__);
            goto critical_error;
        }
    }

    _VTSPR_FOR_ALL_INFC(infc) {
        /* 
         * cidQ exists for all interface types
         * For FXS, Q direction is VAPP -> vTSP -> VTSP_RT -> infc
         * For AUDIO, Q direction is VAPP -> vTSP -> VTSP_RT -> infc
         * For FXO, Q direction is VAPP <- vTSP <- VTSP_RT <- infc
         */
        OSAL_snprintf(qname, sizeof(qname), "vtsp-cid%dq", infc);
        if (NULL == (q_ptr->cidQ[infc] = OSAL_msgQCreate(
                    qname,
                    OSAL_MODULE_AUDIO_VE, OSAL_MODULE_AUDIO_VE,
                    OSAL_DATA_STRUCT__VTSP_CIDData,
                    1,
                    sizeof(_VTSP_CIDData),
                    0))) { 
            _VTSP_TRACE(__FILE__,__LINE__);
            goto critical_error;
        }
    }

    /*
     * Associate the VTSP flow queue with VTSPR flow queues.
     */
#if defined(VTSP_ENABLE_PLAY) || defined(VTSP_ENABLE_RECORD)
    flowIndex = 0;
    _VTSPR_FOR_ALL_INFC_PP(infc) {
        for (flowNum = 0; flowNum < _VTSP_FLOW_PER_INFC; flowNum++) {
#ifdef VTSP_ENABLE_PLAY
            OSAL_snprintf(qname, sizeof(qname), "vtsp-flowp%dq", flowIndex);
            if (NULL == (dsp_ptr->flowObj[flowIndex].flowFromAppQ =
                    OSAL_msgQCreate(
                    qname,
                    OSAL_MODULE_AUDIO_VE, OSAL_MODULE_AUDIO_VE,
                    OSAL_DATA_STRUCT__VTSP_FlowMsg,
                    _VTSP_Q_FLOW_PUT_NUM_MSG,
                    _VTSP_Q_FLOW_PUT_DATA_SZ,
                    0))) {
                _VTSP_TRACE(__FILE__,__LINE__);
                goto critical_error;
            }
#endif
#ifdef VTSP_ENABLE_RECORD
            OSAL_snprintf(qname, sizeof(qname), "vtsp-flowr%dq", flowIndex);
            if (NULL == (dsp_ptr->flowObj[flowIndex].flowToAppQ =
                    OSAL_msgQCreate(
                    qname,
                    OSAL_MODULE_AUDIO_VE, OSAL_MODULE_AUDIO_VE,
                    OSAL_DATA_STRUCT__VTSP_FlowMsg,
                    _VTSP_Q_FLOW_GET_NUM_MSG,
                    _VTSP_Q_FLOW_GET_DATA_SZ,
                    0))) {
                _VTSP_TRACE(__FILE__,__LINE__);
                goto critical_error;
            }
#endif
            flowIndex++;
        }
    }
#endif

    /*
     * Create RTCP Queues
     */
    if (NULL == (q_ptr->rtcpMsg = OSAL_msgQCreate(
            "vtsp-rtcpmsgq",
            OSAL_MODULE_AUDIO_VE, OSAL_MODULE_AUDIO_VE,
            OSAL_DATA_STRUCT__VTSP_RtcpCmdMsg,
            _VTSP_Q_RTCP_NUM_MSG,
            _VTSP_Q_RTCP_MSG_SZ,
            0))) {
        _VTSP_TRACE(__FILE__,__LINE__);
        goto critical_error;
    }
    if (NULL == (q_ptr->rtcpEvent = OSAL_msgQCreate(
            "vtsp-rtcpevtq",
            OSAL_MODULE_AUDIO_VE, OSAL_MODULE_AUDIO_VE,
            OSAL_DATA_STRUCT__VTSP_RtcpEventMsg,
            _VTSP_Q_RTCP_NUM_EVENT,
            _VTSP_Q_RTCP_EVENT_SZ,
            0)))     {
        _VTSP_TRACE(__FILE__,__LINE__);
        goto critical_error;
    }

    if (NULL == (q_ptr->stunSendQ = OSAL_msgQCreate(
            "vtsp-pktsendq",
            OSAL_MODULE_AUDIO_VE, OSAL_MODULE_AUDIO_VE,
            OSAL_DATA_STRUCT_VTSP_Stun,
            _VTSP_Q_STUN_NUM_MSG,
            sizeof(VTSP_Stun),
            0))) { 
        _VTSP_TRACE(__FILE__,__LINE__);
        goto critical_error;
    }
    if (NULL == (q_ptr->stunRecvQ = OSAL_msgQCreate(
            "vtsp-pktrecvq",
            OSAL_MODULE_AUDIO_VE, OSAL_MODULE_AUDIO_VE,
            OSAL_DATA_STRUCT_VTSP_Stun,
            _VTSP_Q_STUN_NUM_MSG, 
            sizeof(VTSP_Stun),
            0))) { 
        _VTSP_TRACE(__FILE__,__LINE__);
        goto critical_error;
    }
    /* 
     * End of Q context code
     * --------
     */
    /*
     * Initialize Hardware
     * Test the CPLD interface
     */
    OSAL_taskDelay(1000);   /* XXX guarantee serial console is idle */
    result = 0;
    if (0 != (result |= VHW_init())) {
        OSAL_logMsg("%s:%d Can't init telephony hardware (err 0x%x)\n",
                __FILE__, __LINE__, result);
    } 

    /* 
     * Initialize low level hardware
     */
    result = 0;
    if (0 != (result |= TIC_init())) {
        OSAL_logMsg("%s:%d Can't initialize telephony hardware (err 0x%x): "
                "check connections, board type, and CPLD revision\n", 
                __FILE__, __LINE__, result);
    }
    /*
     * Init TIC interfaces
     * Note, TIC infc may or may not be active after init, depending on driver.
     */
    _VTSPR_FOR_ALL_INFC(infc) {
        tic_ptr  = _VTSPR_infcToTicPtr(dsp_ptr, infc);
        infcType = _VTSPR_infcToTicInfcType(infc);
        chan     = _VTSPR_infcToChan(infc);
        if (TIC_OK != TIC_initInfc(tic_ptr, infcType, chan)) {
            _VTSP_TRACE(__FILE__,__LINE__);
        }
        TIC_control(tic_ptr, TIC_CONTROL_LINE_STATE, TIC_STATE_ACTIVE);
    }

    /* Set voice processing defaults
     */
    _VTSPR_defaults(dsp_ptr);

    /*
     * Initialize Multi-frame Coder queues, pointers and shutdown semaphore
     */
    ret_val = _VTSPR_multiEncodeInit(vtspr_ptr, q_ptr, dsp_ptr);
    if (-1 == ret_val) {
        _VTSP_TRACE(__FILE__,__LINE__);
        goto critical_error;
    }

    ret_val = _VTSPR_multiDecodeInit(vtspr_ptr, q_ptr, dsp_ptr);
    if (-1 == ret_val) {
        _VTSP_TRACE(__FILE__,__LINE__);
        goto critical_error;
    }

    if (NULL == (vtspr_ptr->task10ms.finishSemId = OSAL_semCountCreate(0))) { 
        _VTSP_TRACE(__FILE__,__LINE__);
        goto critical_error;
    }


#ifdef OSAL_MT
    /* register the vTPR task message queue ID to the VHW, then start the VHW */
    VHW_msgQid_register(vtspr_ptr->q_ptr->heartbeat);
#endif   

#ifdef VHW_CPLD_REVISION
    /*
     * Print the CPLD version
     */
    vtspr_ptr->pldRev = VHW_CPLD_REVISION;
    OSAL_logMsg("%s:%d Using %s vPort CPLD revision = 0x%x\n", 
            __FILE__, __LINE__, (int)VHW_driverName, vtspr_ptr->pldRev);
#endif



    /*
     * Initialize RTP, set up network ports.
     */
    _VTSPR_FOR_ALL_INFC_PP(infc) {
        for (streamId = 0; streamId < _VTSP_STREAM_PER_INFC; streamId++) {
            rtp_ptr = _VTSPR_streamIdToRtpPtr(vtspr_ptr->net_ptr, infc,
                    streamId);
            if (_VTSPR_RTP_OK != _VTSPR_rtpInit(rtp_ptr)) { 
                _VTSP_TRACE(__FILE__,__LINE__);
                VTSPR_shutdown(); /* VTSPR Task must stop itself  */
                return (VTSP_E_INIT);
            }
            rtcp_ptr = _VTSPR_streamIdToRtcpPtr(vtspr_ptr->net_ptr, infc,
                    streamId);
            if (_VTSPR_RTP_OK != _VTSPR_rtcpInit(rtcp_ptr, vtspr_ptr->net_ptr,
                        streamId, infc)) { 
                _VTSP_TRACE(__FILE__,__LINE__);
                VTSPR_shutdown(); /* VTSPR Task must stop itself  */
                return (VTSP_E_INIT);
            }
        }
    }

    /*
     * Create the multi-encode tasks
     */
    ret_val = _VTSPR_multiEncodeTaskInit(vtspr_ptr);
    if (-1 == ret_val) {
        _VTSP_TRACE(__FILE__,__LINE__);
        return (ret_val);
    }
    ret_val = _VTSPR_multiDecodeTaskInit(vtspr_ptr);
    if (-1 == ret_val) {
        _VTSP_TRACE(__FILE__,__LINE__);
        return (ret_val);
    }

    /*
     * Call the VTSPR task
     */
    VTSPR_task(vtspr_ptr);

    return (result);

    /* 
     * Critical error, shutdown
     */
critical_error:
    VTSPR_shutdown();
    /* 
     * VTSPR Task must stop itself 
     */
    return (VTSP_E_INIT);
}

/* 
 * ======== VTSPR_shutdown() =======
 *
 * Stop task, wait for task to stop.
 * Free all resources if they exist
 */
void VTSPR_shutdown(
    void)
{
    VTSPR_Obj          *vtspr_ptr;
#if defined(VTSP_ENABLE_ILBC) || defined(VTSP_ENABLE_G722P1) || \
        defined(VTSP_ENABLE_SILK) || defined(VTSP_ENABLE_GAMRNB) || \
        defined(VTSP_ENABLE_GAMRWB) || defined(VTSP_ENABLE_G723) || \
        defined(VTSP_ENABLE_G729_ACCELERATOR) || \
        defined(VTSP_ENABLE_G726_ACCELERATOR)
    VTSPR_Queues       *q_ptr;
    _VTSPR_MultiRawMsg  msg;
    vint i;
#endif
    _VTSP_TRACE(__FILE__, __LINE__);

    vtspr_ptr = &VTSPR_obj;

    /* Stop the task by setting flag
     */
    vtspr_ptr->task10ms.taskEnable |= VTSPR_TASK_STOP;
    vtspr_ptr->task10ms.taskEnable &= ~VTSPR_TASK_RUN;

#if defined(VTSP_ENABLE_ILBC) || defined(VTSP_ENABLE_G722P1) || \
        defined(VTSP_ENABLE_SILK) || defined(VTSP_ENABLE_GAMRNB) || \
        defined(VTSP_ENABLE_GAMRWB) || defined(VTSP_ENABLE_G723) || \
        defined(VTSP_ENABLE_G729_ACCELERATOR) || \
        defined(VTSP_ENABLE_G726_ACCELERATOR)
    q_ptr = vtspr_ptr->q_ptr;
    /* Message Multi-Enc tasks to shutdown */
    msg.cmd = _VTSPR_MULTI_CODER_CMD_SHUTDOWN;
#endif

#if defined(VTSP_ENABLE_ILBC) || defined(VTSP_ENABLE_G722P1) || \
    defined(VTSP_ENABLE_SILK) || defined(VTSP_ENABLE_GAMRNB) || \
    defined(VTSP_ENABLE_GAMRWB)
    /*
     * Shutdown 20ms encoder tasks
     */
    if (OSAL_SUCCESS != OSAL_msgQSend(q_ptr->data20Raw, (char *)&msg,
             sizeof(_VTSPR_MultiRawMsg), OSAL_NO_WAIT, NULL)) {
        _VTSP_TRACE(__FILE__, __LINE__);
    }
    vtspr_ptr->multiEncTask20ms.taskEnable |= VTSPR_TASK_STOP;
    vtspr_ptr->multiEncTask20ms.taskEnable &= ~VTSPR_TASK_RUN;

    OSAL_semAcquire(vtspr_ptr->multiEncTask20ms.finishSemId, 2000);
    OSAL_semDelete(vtspr_ptr->multiEncTask20ms.finishSemId);

    /* Free Multi-frame queues on shutdown */
    OSAL_msgQDelete(q_ptr->data20Raw);
    for (i = 0; i < _VTSP_STREAM_NUM; i++) {
        OSAL_msgQDelete(q_ptr->data20Enc[i]);
    }
    /* 
     * Shutdown 20ms decoder tasks
     */
    if (OSAL_SUCCESS != OSAL_msgQSend(q_ptr->data20Pkt, (char *)&msg,
             sizeof(_VTSPR_MultiPktMsg), OSAL_NO_WAIT, NULL)) {
        _VTSP_TRACE(__FILE__, __LINE__);
    }
    vtspr_ptr->multiDecTask20ms.taskEnable |= VTSPR_TASK_STOP;
    vtspr_ptr->multiDecTask20ms.taskEnable &= ~VTSPR_TASK_RUN;

    OSAL_semAcquire(vtspr_ptr->multiDecTask20ms.finishSemId, 2000);
    OSAL_semDelete(vtspr_ptr->multiDecTask20ms.finishSemId);

    /* Free Multi-frame queues on shutdown */
    OSAL_msgQDelete(q_ptr->data20Pkt);
    for (i = 0; i < _VTSP_STREAM_NUM; i++) {
        OSAL_msgQDelete(q_ptr->data20Dec[i]);
    }

#endif
#if defined(VTSP_ENABLE_ILBC) || defined(VTSP_ENABLE_G723)
    /*
     * Shutdown 30ms Encoder Task
     */
    if (OSAL_SUCCESS != OSAL_msgQSend(q_ptr->data30Raw, (char *)&msg,
             sizeof(_VTSPR_MultiRawMsg), OSAL_NO_WAIT, NULL)) {
        _VTSP_TRACE(__FILE__, __LINE__);
    }    

    vtspr_ptr->multiEncTask30ms.taskEnable |= VTSPR_TASK_STOP;
    vtspr_ptr->multiEncTask30ms.taskEnable &= ~VTSPR_TASK_RUN;

    OSAL_semAcquire(vtspr_ptr->multiEncTask30ms.finishSemId, 2000);
    OSAL_semDelete(vtspr_ptr->multiEncTask30ms.finishSemId);

    /* Free Multi-frame queues on shutdown */
    OSAL_msgQDelete(q_ptr->data30Raw);

    for (i = 0; i < _VTSP_STREAM_NUM; i++) {
        OSAL_msgQDelete(q_ptr->data30Enc[i]);
    }

    /*
     * Shutdown 30ms Decoder Task
     */
    if (OSAL_SUCCESS != OSAL_msgQSend(q_ptr->data30Pkt, (char *)&msg,
             sizeof(_VTSPR_MultiPktMsg), OSAL_NO_WAIT, NULL)) {
        _VTSP_TRACE(__FILE__, __LINE__);
    }    

    vtspr_ptr->multiDecTask30ms.taskEnable |= VTSPR_TASK_STOP;
    vtspr_ptr->multiDecTask30ms.taskEnable &= ~VTSPR_TASK_RUN;

    OSAL_semAcquire(vtspr_ptr->multiDecTask30ms.finishSemId, 2000);
    OSAL_semDelete(vtspr_ptr->multiDecTask30ms.finishSemId);

    /* Free Multi-frame queues on shutdown */
    OSAL_msgQDelete(q_ptr->data30Pkt);

    for (i = 0; i < _VTSP_STREAM_NUM; i++) {
        OSAL_msgQDelete(q_ptr->data30Dec[i]);
    }

#endif
#if (defined(VTSP_ENABLE_G729_ACCELERATOR) || \
       defined(VTSP_ENABLE_G726_ACCELERATOR))
    /*
     * Shutdown 10ms Encoder Task
     */
    if (OSAL_SUCCESS != OSAL_msgQSend(q_ptr->data10Raw, (char *)&msg,
             sizeof(_VTSPR_MultiRawMsg), OSAL_NO_WAIT, NULL)) {
        _VTSP_TRACE(__FILE__, __LINE__);
    }    

    vtspr_ptr->multiEncTask10ms.taskEnable |= VTSPR_TASK_STOP;
    vtspr_ptr->multiEncTask10ms.taskEnable &= ~VTSPR_TASK_RUN;

    OSAL_semAcquire(vtspr_ptr->multiEncTask10ms.finishSemId, 2000);
    OSAL_semDelete(vtspr_ptr->multiEncTask10ms.finishSemId);

    /* Free Multi-frame queues on shutdown */
    OSAL_msgQDelete(q_ptr->data10Raw);

    for (i = 0; i < _VTSP_STREAM_NUM; i++) {
        OSAL_msgQDelete(q_ptr->data10Enc[i]);
    }

    /*
     * Shutdown 10ms Decoder Task
     */
    if (OSAL_SUCCESS != OSAL_msgQSend(q_ptr->data10Pkt, (char *)&msg,
             sizeof(_VTSPR_MultiPktMsg), OSAL_NO_WAIT, NULL)) {
        _VTSP_TRACE(__FILE__, __LINE__);
    }    

    vtspr_ptr->multiDecTask10ms.taskEnable |= VTSPR_TASK_STOP;
    vtspr_ptr->multiDecTask10ms.taskEnable &= ~VTSPR_TASK_RUN;

    OSAL_semAcquire(vtspr_ptr->multiDecTask10ms.finishSemId, 2000);
    OSAL_semDelete(vtspr_ptr->multiDecTask10ms.finishSemId);

    /* Free Multi-frame queues on shutdown */
    OSAL_msgQDelete(q_ptr->data10Pkt);

    for (i = 0; i < _VTSP_STREAM_NUM; i++) {
        OSAL_msgQDelete(q_ptr->data10Dec[i]);
    }
#endif
    return;
}

