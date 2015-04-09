/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004-2010 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 12523 $ $Date: 2010-07-13 18:57:44 -0400 (Tue, 13 Jul 2010) $
 *
 */
#include "ve.h"
#include "_ve_private.h"
#include <vier.h>

/* 
 * VE structure, private to VE
 */
_VE_Obj _VE_obj;

/*
 * ======== _VE_postInitEncode() ========
 *
 * Soft-dsp initialization.
 *
 * Returns:
 *  zero    = successful init
 *  nonzero = bit mask of error code
 *
 */
OSAL_TaskReturn _VE_postInitEncode(
    OSAL_TaskArg arg)
{
    _VE_Obj  *ve_ptr;
    ve_ptr = &_VE_obj;

    if (NULL == (ve_ptr->taskEncode.finishSemId = OSAL_semCountCreate(0))) {
        _VE_TRACE(__FILE__,__LINE__);
        return (VTSP_E_INIT);
    }

    /*
     * Call the VE task
     */
    _VE_taskEncode(ve_ptr);

    return (VTSP_OK);
}


/*
 * ======== _VE_postInit() ========
 *
 * Soft-dsp initialization.
 *
 * Returns:  
 *  zero    = successful init
 *  nonzero = bit mask of error code
 *
 */
OSAL_TaskReturn _VE_postInit(
    OSAL_TaskArg arg)
{
    _VE_Obj    *ve_ptr;
    _VE_Queues *q_ptr;
    _VE_Dsp    *dsp_ptr;
    _VE_RtpObject  *rtp_ptr;
    _VE_RtcpObject *rtcp_ptr;
    vint          infc;                   /* must be signed */
    vint          index;
    vint          streamId;

    ve_ptr = &_VE_obj;

    /*
     * Alloc & Init Queue Obj Structure
     */
    if (NULL == (ve_ptr->q_ptr = OSAL_memCalloc(1,
                    sizeof(_VE_Queues), 0))) {
        _VE_TRACE(__FILE__,__LINE__);
        return (VTSP_E_INIT);
    }
    q_ptr = ve_ptr->q_ptr;

    /* 
     * Alloc & Init voice processing context
     */
    if (NULL == (ve_ptr->dsp_ptr = OSAL_memCalloc(1,
                    sizeof(_VE_Dsp), 0))) {
        OSAL_logMsg("%s:%d fatal malloc error, size=%d\n", 
                __FILE__, __LINE__, sizeof(_VE_Dsp));

        return (VTSP_E_INIT);
    }

    dsp_ptr = ve_ptr->dsp_ptr;

    for (index = 0; index < _VTSP_STREAM_NUM; index++) {
        if (NULL == (dsp_ptr->streamObj_ptr[index]
                        = (_VE_StreamObj *) OSAL_memCalloc(1,
                                sizeof(_VE_StreamObj), 0))) {
            _VE_TRACE(__FILE__,__LINE__);
            return (VTSP_E_INIT);
        }
    }

    /*
     * Alloc & Init block exchange context
     */
    if (NULL == (ve_ptr->net_ptr = OSAL_memCalloc(1,
                    sizeof(_VE_NetObj), 0))) {
        _VE_TRACE(__FILE__,__LINE__);
        return (VTSP_E_INIT);
    }


    /*
     * Set Q context
     * --------
     * This Q context is created by VTSP layer.
     *
     */
    if (NULL == (q_ptr->cmdQ = OSAL_msgQCreate(
                VIDEO_VTSP_CMD_Q_NAME,
#ifdef INCLUDE_4G_PLUS
                OSAL_MODULE_VIER,
#else
                OSAL_MODULE_AUDIO_VE,
#endif
                OSAL_MODULE_VIDEO_VE,
                OSAL_DATA_STRUCT__VTSP_CmdMsg,
                _VTSP_Q_CMD_NUM_MSG,
                _VTSP_Q_CMD_MSG_SZ,
                0))) { 
        _VE_TRACE(__FILE__,__LINE__);
        goto critical_error;
    }

    if (NULL == (q_ptr->eventQ =
                OSAL_msgQCreate(
                VIDEO_VTSP_EVT_Q_NAME,
                OSAL_MODULE_VIDEO_VE,
#ifdef INCLUDE_4G_PLUS
                OSAL_MODULE_VIER,
#else
                OSAL_MODULE_AUDIO_VE,
#endif
                OSAL_DATA_STRUCT_VTSP_EventMsg,
                _VTSP_Q_EVENT_NUM_MSG,
                _VTSP_Q_EVENT_MSG_SZ,
                0))) {
        _VE_TRACE(__FILE__,__LINE__);
        goto critical_error;
    }

    /*
     * Create RTCP Queues
     */
    if (NULL == (q_ptr->rtcpMsg = OSAL_msgQCreate(
            VIDEO_VTSP_RTCP_MSG_Q_NAME,
            OSAL_MODULE_VIDEO_VE,
#ifdef INCLUDE_4G_PLUS
            OSAL_MODULE_VIER,
#else
            OSAL_MODULE_AUDIO_VE,
#endif
            OSAL_DATA_STRUCT__VTSP_RtcpCmdMsg,
            _VTSP_Q_RTCP_NUM_MSG,
            _VTSP_Q_RTCP_MSG_SZ,
            0))) {
        _VE_TRACE(__FILE__,__LINE__);
        goto critical_error;
    }
    if (NULL == (q_ptr->rtcpEvent = OSAL_msgQCreate(
            VIDEO_VTSP_RTCP_EVT_Q_NAME,
#ifdef INCLUDE_4G_PLUS
            OSAL_MODULE_VIER,
#else
            OSAL_MODULE_AUDIO_VE,
#endif
            OSAL_MODULE_VIDEO_VE,
            OSAL_DATA_STRUCT__VTSP_RtcpEventMsg,
            _VTSP_Q_RTCP_NUM_EVENT,
            _VTSP_Q_RTCP_EVENT_SZ,
            0)))     {
        _VE_TRACE(__FILE__,__LINE__);
        goto critical_error;
    }

    if (NULL == (q_ptr->stunSendQ = OSAL_msgQCreate(
            "vtsp-pktsendqVideo",
            OSAL_MODULE_AUDIO_VE, OSAL_MODULE_VIDEO_VE,
            OSAL_DATA_STRUCT_VTSP_Stun,
            _VTSP_Q_STUN_NUM_MSG,
            sizeof(VTSP_Stun),
            0))) { 
        _VE_TRACE(__FILE__,__LINE__);
        goto critical_error;
    }
    if (NULL == (q_ptr->stunRecvQ = OSAL_msgQCreate(
            "vtsp-pktrecvqVideo",
            OSAL_MODULE_VIDEO_VE, OSAL_MODULE_AUDIO_VE,
            OSAL_DATA_STRUCT_VTSP_Stun,
            _VTSP_Q_STUN_NUM_MSG, 
            sizeof(VTSP_Stun),
            0))) { 
        _VE_TRACE(__FILE__,__LINE__);
        goto critical_error;
    }
    /* 
     * End of Q context code
     * --------
     */

    /* Set video processing defaults
     */
    _VE_defaults(dsp_ptr);


    if (NULL == (ve_ptr->taskMain.finishSemId = OSAL_semCountCreate(0))) {
        _VE_TRACE(__FILE__,__LINE__);
        goto critical_error;
    }


    if (NULL == (ve_ptr->encStopSem = OSAL_semMutexCreate())) {
        _VE_TRACE(__FILE__,__LINE__);
        goto critical_error;
    }

    /*
     * Initialize RTP, set up network ports.
     */
    for (infc = VTSP_INFC_VIDEO; infc == VTSP_INFC_VIDEO; infc++) {
        for (streamId = 0; streamId < _VTSP_STREAM_PER_INFC; streamId++) {
            rtp_ptr = _VE_streamIdToRtpPtr(ve_ptr->net_ptr, infc,
                    streamId);
            if (_VE_RTP_OK != _VE_rtpInit(rtp_ptr)) {
                _VE_TRACE(__FILE__,__LINE__);
                VE_shutdown(); /* VE Task must stop itself  */
                return (VTSP_E_INIT);
            }
            rtcp_ptr = _VE_streamIdToRtcpPtr(ve_ptr->net_ptr, infc,
                    streamId);
            if (_VE_RTP_OK != _VE_rtcpInit(rtcp_ptr, ve_ptr->net_ptr,
                        streamId, infc)) { 
                _VE_TRACE(__FILE__,__LINE__);
                VE_shutdown(); /* VE Task must stop itself  */
                return (VTSP_E_INIT);
            }
        }
    }
#if 0
    /*
     * Drain command queue and discard old commands.
     */
    while (OSAL_msgQRecv(q_ptr->cmdQ,
            &q_ptr->cmdMsg,
            _VTSP_Q_CMD_MSG_SZ, OSAL_NO_WAIT, NULL) > 0);
#endif
    /*
     * Call the VE task
     */
    _VE_task(ve_ptr);

    return (VTSP_OK);

    /* 
     * Critical error, shutdown
     */
critical_error:
    VE_shutdown();
    /* 
     * VE Task must stop itself
     */
    return (VTSP_E_INIT);
}

/*
 * ======== VE_init() ========
 *
 * Soft-dsp initialization.
 *
 * Returns:
 *
 */
OSAL_TaskId VE_init(
    void)
{
    _VE_Obj    *ve_ptr;

    /* Init the device private data object
     */
    ve_ptr = &_VE_obj;
    OSAL_memSet(ve_ptr, 0, sizeof(_VE_obj));
    if (0 != VID_dspInit()) {
        return (0);
    }

    /*
     * Initialize for waiting, Start the task.
     */
    ve_ptr->taskMain.taskEnable = _VE_TASK_WAIT;
    ve_ptr->taskEncode.taskEnable = _VE_TASK_WAIT;

    ve_ptr->taskMain.taskPriority = _VE_TASK_PRIORITY;
    ve_ptr->taskMain.stackSize = _VE_TASK_STACK_SZ;
    ve_ptr->taskMain.taskId = OSAL_taskCreate("ve",
            ve_ptr->taskMain.taskPriority, ve_ptr->taskMain.stackSize,
            (void *)_VE_postInit, NULL);

    ve_ptr->taskEncode.taskPriority = _VE_ENCODE_TASK_PRIORITY;
    ve_ptr->taskEncode.stackSize = _VE_ENCODE_TASK_STACK_SZ;
    ve_ptr->taskEncode.taskId = OSAL_taskCreate("ve-enc",
            ve_ptr->taskEncode.taskPriority, ve_ptr->taskEncode.stackSize,
            (void *)_VE_postInitEncode, NULL);

    OSAL_logMsg("%s:%d Done\n", __FUNCTION__, __LINE__);


    return (ve_ptr->taskMain.taskId);
}


/* 
 * ======== VE_shutdown() =======
 *
 * Stop task, wait for task to stop.
 * Free all resources if they exist
 */
void VE_shutdown(
    void)
{
    _VE_Obj          *ve_ptr;

    _VE_TRACE(__FUNCTION__, __LINE__);

    ve_ptr = &_VE_obj;

    /*
     * Stop the tasks by setting flags
     */

    ve_ptr->taskEncode.taskEnable |= _VE_TASK_STOP;
    ve_ptr->taskEncode.taskEnable &= ~_VE_TASK_RUN;

    OSAL_semAcquire(ve_ptr->taskEncode.finishSemId, OSAL_WAIT_FOREVER);

    ve_ptr->taskMain.taskEnable |= _VE_TASK_STOP;
    ve_ptr->taskMain.taskEnable &= ~_VE_TASK_RUN;

    OSAL_semAcquire(ve_ptr->taskMain.finishSemId, OSAL_WAIT_FOREVER);

    OSAL_semDelete(ve_ptr->taskMain.finishSemId);
    OSAL_semDelete(ve_ptr->taskEncode.finishSemId);
    OSAL_semDelete(ve_ptr->encStopSem);

    if (NULL != ve_ptr->q_ptr->cmdQ) {
        OSAL_msgQDelete(ve_ptr->q_ptr->cmdQ);
    }

    if (NULL != ve_ptr->q_ptr->eventQ) {
        OSAL_msgQDelete(ve_ptr->q_ptr->eventQ);
    }

    if (NULL != ve_ptr->q_ptr->rtcpMsg) {
        OSAL_msgQDelete(ve_ptr->q_ptr->rtcpMsg);
    }

    if (NULL != ve_ptr->q_ptr->rtcpEvent) {
        OSAL_msgQDelete(ve_ptr->q_ptr->rtcpEvent);
    }

    if (NULL != ve_ptr->q_ptr->stunSendQ) {
        OSAL_msgQDelete(ve_ptr->q_ptr->stunSendQ);
    }

    if (NULL != ve_ptr->q_ptr->stunRecvQ) {
        OSAL_msgQDelete(ve_ptr->q_ptr->stunRecvQ);
    }

    OSAL_memSet(ve_ptr->dsp_ptr, 0, sizeof(_VE_Dsp));
    OSAL_memFree(ve_ptr->dsp_ptr, 0);

    OSAL_memSet(ve_ptr->q_ptr, 0, sizeof(_VE_Queues));
    OSAL_memFree(ve_ptr->q_ptr, 0);

    OSAL_memSet(ve_ptr->net_ptr, 0, sizeof(_VE_NetObj));
    OSAL_memFree(ve_ptr->net_ptr, 0);
    OSAL_logMsg("%s:%d Exit complete\n", __FUNCTION__, __LINE__);

    VID_dspShutdown();

    return;
}
