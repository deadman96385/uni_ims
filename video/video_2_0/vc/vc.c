/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004-2010 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 12523 $ $Date: 2010-07-13 18:57:44 -0400 (Tue, 13 Jul 2010) $
 *
 */
#include "_vc_private.h"
#include <vier.h>

#include "_vc_rtcp.h"
/*
 * Video Controller structure, private to VC
 */
_VC_Obj _VC_obj;

/*
 * ======== _VC_selfInit() ========
 *
 * Initializes the internal objects of _VC_Obj
 *
 */
static vint _VC_selfInit(
    void)
{
    _VC_Obj        *vc_ptr;
    _VC_Queues     *q_ptr;
    _VC_Dsp        *dsp_ptr;
    _VC_RtpObject  *rtp_ptr;
    _VC_RtcpObject *rtcp_ptr;
    vint            infc;                   /* must be signed */
    vint            index;
    vint            streamId;

    vc_ptr = &_VC_obj;

    vc_ptr->vcInit = 1;
    /*
     * Alloc & Init Queue Obj Structure (_VC_Queues)
     */
    if (NULL == (vc_ptr->q_ptr = OSAL_memCalloc(1,
                    sizeof(_VC_Queues), 0))) {
        _VC_TRACE(__FILE__, __LINE__);
        return (_VC_ERROR_INIT);
    }
    q_ptr = vc_ptr->q_ptr;

    /*
     * Alloc & Init Video processing context (_VC_Dsp)
     */
    if (NULL == (vc_ptr->dsp_ptr = OSAL_memCalloc(1,
                    sizeof(_VC_Dsp), 0))) {
        _VC_LOG("fatal malloc error, size=%d", sizeof(_VC_Dsp));

        return (_VC_ERROR_INIT);
    }
    dsp_ptr = vc_ptr->dsp_ptr;

    /*
     * Alloc & Init structures for all streams (_VC_StreamObj)
     */

    for (index = 0; index < _VTSP_STREAM_NUM; index++) {
        if (NULL == (dsp_ptr->streamObj_ptr[index]
                        = (_VC_StreamObj *) OSAL_memCalloc(1,
                                sizeof(_VC_StreamObj), 0))) {
            _VC_TRACE(__FILE__, __LINE__);
            return (_VC_ERROR_INIT);
        }
    }

    /*
     * Alloc & Init block exchange context (_VC_NetObj)
     */
    if (NULL == (vc_ptr->net_ptr = OSAL_memCalloc(1,
                    sizeof(_VC_NetObj), 0))) {
        _VC_TRACE(__FILE__, __LINE__);
        return (_VC_ERROR_INIT);
    }


    /*
     * Set Q context
     * --------
     * This Q context is created by VTSP layer.
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
        _VC_TRACE(__FILE__, __LINE__);
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
        _VC_TRACE(__FILE__, __LINE__);
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
        _VC_TRACE(__FILE__, __LINE__);
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
        _VC_TRACE(__FILE__, __LINE__);
        goto critical_error;
    }

    /* Init the queue used for sending events to the application (Java VCE getEvent). */
    if (NULL == (q_ptr->appEventQ = OSAL_msgQCreate(
            "vc-appevtqVideo",
            OSAL_MODULE_VIDEO_VE, OSAL_MODULE_VIDEO_VE,
            OSAL_DATA_STRUCT__VC_AppEventMsg,
            _VC_APP_Q_MAX_DEPTH,
            _VC_APP_Q_EVENT_MSG_SZ,
            0))) {
        _VC_TRACE(__FILE__, __LINE__);
        goto critical_error;
    }

    /* Queue for sending commands to the RTCP Send/Receive Task */
    if (NULL == (q_ptr->rtcpCmdQ = OSAL_msgQCreate(
            "vc-rtcpcmdqVideo",
            OSAL_MODULE_VIDEO_VE,
#ifdef INCLUDE_4G_PLUS
            OSAL_MODULE_VIER,
#else
            OSAL_MODULE_AUDIO_VE,
#endif
            OSAL_DATA_STRUCT__VTSP_RtcpCmdMsg,
            _VC_RTCP_Q_MAX_DEPTH,
            _VC_RTCP_Q_CMD_MSG_SZ,
            0))) {
        _VC_TRACE(__FILE__, __LINE__);
        goto critical_error;
    }

    /*
     * End of Q context code
     * --------
     */

    /* Write Init event on the App Queue. */
    _VC_sendAppEvent(q_ptr, VC_EVENT_INIT_COMPLETE, "VC - Q init complete", -1);

    /* Set video processing defaults */
    _VC_defaults(dsp_ptr);

    /*
     * Initialize RTP, set up network ports.
     */
    for (infc = VTSP_INFC_VIDEO; infc == VTSP_INFC_VIDEO; infc++) {
        for (streamId = 0; streamId < _VTSP_STREAM_PER_INFC; streamId++) {
            rtp_ptr = _VC_streamIdToRtpPtr(vc_ptr->net_ptr, streamId);
            if (_VC_RTP_OK != _VC_rtpInit(rtp_ptr)) {
                _VC_TRACE(__FILE__, __LINE__);
                goto critical_error;
            }
            rtcp_ptr = _VC_streamIdToRtcpPtr(vc_ptr->net_ptr, streamId);
            if (_VC_RTP_OK != _VC_rtcpInit(rtcp_ptr, vc_ptr->net_ptr,
                        streamId, infc)) {
                _VC_TRACE(__FILE__, __LINE__);
                goto critical_error;
            }
        }
    }
    return (_VC_OK);

    /*
     * Critical error, shutdown
     */
critical_error:
    /* VC Task must stop itself. */
    VC_shutdown();
    return (_VC_ERROR_INIT);
}

/*
 * ======== _VC_getCommand() ========
 *
 * This function reads commands from VTSP.  These commands are read
 * from the IPC used specifically used to for commands from VTSP
 *
 * Return Values:
 * _VC_OK  A command from ISI was successfully read
 * _VC_ERROR There was a error when reading the IPC.
 *          No attempt should be made to process the data in cmd_ptr
 */
static vint _VC_getCommand(
    OSAL_MsgQId  *id_ptr,
    _VTSP_CmdMsg *cmd_ptr)
{
    int32 ret;
    if (_VTSP_Q_CMD_MSG_SZ != (ret = OSAL_msgQRecv(*id_ptr, (char*)cmd_ptr, _VTSP_Q_CMD_MSG_SZ,
            OSAL_WAIT_FOREVER, NULL))) {
        OSAL_logMsg("%s: failed to receive VTSP, ret %d\n", __FUNCTION__, ret);
        return (_VC_ERROR);
    }
    return (_VC_OK);
}

/*
 * ======== _VC_alarmHandler() ========
 *
 * The handler to catch SIGALRM
 *
 */
void _VC_alarmHandler(int signo)
{
    /*
     * Maybe, we will put the rtcpSend in timmer in the future.
     * But now, this handler is just used to avoid _VC_rtcpSendRecvTask
     * terminated by SIGALRM.
     *
     */
    return;
}

/*
 * ======== _VC_rtcpSendRecvTask() ========
 *
 * This task is responsible for Sending and Receiving RTCP packets.
 *
 * Return Values:
 * None.
 */
static void _VC_rtcpSendRecvTask(
    void *arg_ptr)
{
    _VC_Obj         *vc_ptr;
    _VC_Queues      *q_ptr;
    _VC_Dsp         *dsp_ptr;
    _VC_StreamObj   *stream_ptr;
    _VC_RtcpObject  *rtcp_ptr;
    vint            streamId;

    /* Get the Stream Id. */
    stream_ptr = (_VC_StreamObj *)arg_ptr;
    streamId   = stream_ptr->streamParam.streamId;
    vc_ptr = &_VC_obj;
    q_ptr = vc_ptr->q_ptr;
    dsp_ptr = vc_ptr->dsp_ptr;
    rtcp_ptr    = _VC_streamIdToRtcpPtr(vc_ptr->net_ptr, streamId);

    /* RTCP TMMBR defaults. */
    rtcp_ptr->feedback.tmmbrState = _VC_TMMBR_STATE_INHIBIT;
    rtcp_ptr->feedback.state = _VC_TMMBR_STATE_DONE;
    rtcp_ptr->feedback.direction = _VC_TMMBR_DIR_LEVEL;
    rtcp_ptr->feedback.sendTmmbrInKbps = 0;
    rtcp_ptr->feedback.expectedPacketTotal = 0;
    rtcp_ptr->feedback.lostPacketTotal = 0;

    /* register the handler to catch SIGALRM */
    OSAL_taskRegisterSignal(SIGALRM, _VC_alarmHandler);

    _VC_LOG("RTCP Task running\n");

_VC_RTCP_TASK_LOOP:

    /* Read any commands, such as configuration updates. */
    if (_VC_OK != _VC_rtcpReadCommand(q_ptr, dsp_ptr, vc_ptr->net_ptr, stream_ptr)) {
        _VC_TRACE(__FILE__,__LINE__);
    }

    /*
     * RTCP packets are sent whenever the network is active. They are
     * not sent if the stream peer is local.
     */
    if (VTSP_STREAM_PEER_NETWORK == stream_ptr->streamParam.peer) {
        _VC_rtcpSend(vc_ptr, stream_ptr, VTSP_INFC_VIDEO, streamId);
    }

    /*
     * Get any RTCP packets from network.
     */
    if (_VC_OK != _VC_rtcpRecv(q_ptr, dsp_ptr, vc_ptr->net_ptr)) {
        _VC_TRACE(__FILE__, __LINE__);
    }

    /* For now use the RTCP min interval as delay. This should be modified to timer. */
    //OSAL_taskDelay(rtcp_ptr->configure.reducedMinIntervalMillis);

    if (1 == stream_ptr->rtcpEnable) {
        goto _VC_RTCP_TASK_LOOP;
    }
    else {
        _VC_LOG("RTCP Task exited\n");
    }
}

vint _VC_startRtcpSendRecvTask(
        _VC_StreamObj *stream_ptr)
{
    _VC_Obj    *vc_ptr;
    _VC_TaskObj *task_ptr;
    vc_ptr = &_VC_obj;
    task_ptr = &vc_ptr->taskRtcp;

    /* Init the RTCP send recv task  */
    task_ptr->taskId = 0;
    task_ptr->stackSize = _VC_TASK_STACK_SZ;
    task_ptr->taskPriority = _VC_TASK_RTCP_PRIORITY;
    task_ptr->func_ptr = _VC_rtcpSendRecvTask;
    OSAL_snprintf(task_ptr->name, sizeof(task_ptr->name), "%s",
            VC_RTCP_INFC_TASK_NAME);
    task_ptr->arg_ptr = (void *) stream_ptr;

    if (0 == (task_ptr->taskId = OSAL_taskCreate(
            task_ptr->name,
            task_ptr->taskPriority,
            task_ptr->stackSize,
            task_ptr->func_ptr,
            task_ptr->arg_ptr))){

        //_VC_LOG("Failed to create RTCP Task!!!");
        OSAL_logMsg("%s: Failed to create RTCP Task!!!\n" , __FUNCTION__);
        return _VC_ERROR;
    }
    return _VC_OK;
}

/*
 * ======== _VC_rtpRecvTask() ========
 *
 * This task processes the incoming RTP traffic
 * It loops forever waiting rtp and rtcp packets.
 *
 * Return Values:
 * None.
 */
static void _VC_rtpRecvTask(
    void *arg_ptr)
{
    _VC_Obj         *vc_ptr;
    _VC_StreamObj   *stream_ptr;

    stream_ptr = (_VC_StreamObj *)arg_ptr;
    vc_ptr = &_VC_obj;
    OSAL_logMsg("%s: RTP Task running\n", __FUNCTION__);

_VC_RTP_TASK_LOOP:
    /* Check for incoming RTP data */
    if (VTSP_OK != _VC_rtpRecv(vc_ptr)) {
        _VC_TRACE(__FUNCTION__, __LINE__);
    }

    if (1 == stream_ptr->dec.decRunning) {
        goto _VC_RTP_TASK_LOOP;
    }
    else {
        OSAL_logMsg("%s: RTP Task exited\n", __FUNCTION__);
    }
}

vint _VC_startRtpRecvTask(
    _VC_StreamObj *stream_ptr)
{
    _VC_Obj     *vc_ptr;
    _VC_TaskObj *task_ptr;

    vc_ptr = &_VC_obj;
    task_ptr = &vc_ptr->taskRtp;

    /* Init the RTP recv task  */
    task_ptr->taskId = 0;
    task_ptr->stackSize = _VC_TASK_STACK_SZ;
    task_ptr->taskPriority = _VC_TASK_RTP_PRIORITY;
    task_ptr->func_ptr = _VC_rtpRecvTask;
    OSAL_snprintf(task_ptr->name, sizeof(task_ptr->name), "%s",
            VC_RTP_INFC_TASK_NAME);
    task_ptr->arg_ptr = (void *)stream_ptr;

    if (0 == (task_ptr->taskId = OSAL_taskCreate(
        task_ptr->name,
        task_ptr->taskPriority,
        task_ptr->stackSize,
        task_ptr->func_ptr,
        task_ptr->arg_ptr))){

        OSAL_logMsg("Failed to create RTP Task!!!");
        _VC_TRACE(__FUNCTION__, __LINE__);
        return _VC_ERROR;
    }
    return _VC_OK;
}

/*
 * ======== _VC_vtspCommandTask() ========
 *
 * This task is to read commands from VTSP.  It loops forever
 * waiting for commands on the IPC that VTSP wrote.
 * Commands read from the IPC are then sent to an OSAL
 * queue processed by another task.
 *
 * Return Values:
 * None.
 */
static void _VC_vtspCommandTask(
    void *arg_ptr)
{
    _VC_Obj         *vc_ptr;
    _VC_Queues      *q_ptr;
    _VTSP_CmdMsg    *cmd_ptr;

    vc_ptr = (_VC_Obj *)arg_ptr;
    q_ptr = vc_ptr->q_ptr;
    cmd_ptr = &q_ptr->cmdMsg;

     /*
     * Big D2 Banner with version information.
     */

     /*
     * OSAL_logMsg("\n"
     *   "             D2 Technologies, Inc.\n"
     *   "      _   _  __                           \n"
     *   "     / | '/  /_  _  /_ _  _  /_  _  ._   _\n"
     *   "    /_.'/_  //_'/_ / // //_///_//_///_'_\\ \n"
     *   "                                _/        \n"
     *   "\n"
     *   "        Unified Communication Products\n");
     *OSAL_logMsg("               www.d2tech.com\n");
     *
     *OSAL_logMsg("%s:%d RUNNING\n", __FILE__, __LINE__);
     */
_VC_VTSP_COMMAND_TASK_LOOP:
    /* Block on command message from VTSP. */
    while (_VC_OK != _VC_getCommand(&q_ptr->cmdQ, cmd_ptr));

    _VC_runDnCmd(vc_ptr);

    /* Loop as long as the VC is initialized. */
    if (1 == vc_ptr->vcInit) {
        goto _VC_VTSP_COMMAND_TASK_LOOP;
    }
    else {
        OSAL_logMsg("%s:%d _VC_vtspCommandTask exited\n", __FILE__, __LINE__);
    }
}

/*
 * ======== VC_init() ========
 * Initializes the Video Controller Module.
 * This will create the main VC task.
 *
 * Returns:
 * _VC_OK         All resources were successfully initialized
 * _VC_ERROR_INIT     Failed to init one of the resources needed to read commands
 */
vint VC_init(
    void)
{
    _VC_Obj     *vc_ptr;
    _VC_TaskObj *task_ptr;

    /* Init the Video Controller private data object. */
    vc_ptr = &_VC_obj;
    OSAL_memSet(vc_ptr, 0, sizeof(_VC_obj));
    task_ptr = &vc_ptr->taskMain;

    /* Initialize internal objects and queues. */
    if (_VC_ERROR_INIT == _VC_selfInit()) {
        return _VC_ERROR_INIT;
    }

    /* Init the task used to read and process commands from VTSP. */
    task_ptr->taskId = 0;
    task_ptr->stackSize = _VC_TASK_STACK_SZ;
    task_ptr->taskPriority = _VC_TASK_MAIN_PRIORITY;
    task_ptr->func_ptr = _VC_vtspCommandTask;
    OSAL_snprintf(task_ptr->name, sizeof(task_ptr->name), "%s",
            VC_VTSP_INFC_TASK_NAME);
    task_ptr->arg_ptr = (void *)vc_ptr;

    if (0 == (task_ptr->taskId = OSAL_taskCreate(
        task_ptr->name,
        task_ptr->taskPriority,
        task_ptr->stackSize,
        task_ptr->func_ptr,
        task_ptr->arg_ptr))){
        VC_shutdown();
        return _VC_ERROR_INIT;
    }

    _VC_LOG("Done\n");

    /* Return success. */
    return (_VC_OK);
}

/*
 * ======== VC_shutdown() =======
 * Stop task, wait for task to stop.
 * Free all resources if they exist
 */
void VC_shutdown(
    void)
{
    _VC_Obj          *vc_ptr;
    vint              index;

    _VC_TRACE(__FILE__, __LINE__);

    vc_ptr = &_VC_obj;

    /* This will exit cause the _VC_vtspCommandTask to stop looping. */
    vc_ptr->vcInit = 0;

    /* Delete VTSP, RTCP queues. */
    if (NULL != vc_ptr->q_ptr->cmdQ) {
        OSAL_msgQDelete(vc_ptr->q_ptr->cmdQ);
    }
    if (NULL != vc_ptr->q_ptr->eventQ) {
        OSAL_msgQDelete(vc_ptr->q_ptr->eventQ);
    }
    if (NULL != vc_ptr->q_ptr->rtcpMsg) {
        OSAL_msgQDelete(vc_ptr->q_ptr->rtcpMsg);
    }
    if (NULL != vc_ptr->q_ptr->rtcpEvent) {
        OSAL_msgQDelete(vc_ptr->q_ptr->rtcpEvent);
    }
    if (NULL != vc_ptr->q_ptr->rtcpCmdQ) {
        OSAL_msgQDelete(vc_ptr->q_ptr->rtcpCmdQ);
    }

    /* Dealloc _VC_NetObj. */
    OSAL_memSet(vc_ptr->net_ptr, 0, sizeof(_VC_NetObj));
    OSAL_memFree(vc_ptr->net_ptr, 0);

    /* Dealloc all the streams _VC_StreamObj. */
    for (index = 0; index < _VTSP_STREAM_NUM; index++) {
        OSAL_memSet(vc_ptr->dsp_ptr->streamObj_ptr[index], 0, sizeof(_VC_StreamObj));
        OSAL_memFree(vc_ptr->dsp_ptr->streamObj_ptr[index], 0);
    }

    /* Dealloc the DSP structure _VC_Dsp. */
    OSAL_memSet(vc_ptr->dsp_ptr, 0, sizeof(_VC_Dsp));
    OSAL_memFree(vc_ptr->dsp_ptr, 0);

    /* Dealloc the Queues _VC_Queues. */
    OSAL_memSet(vc_ptr->q_ptr, 0, sizeof(_VC_Queues));
    OSAL_memFree(vc_ptr->q_ptr, 0);

    _VC_LOG("Exit complete\n");

    return;
}

/*
 * ======== VC_getAppEvent() =======
 * getEvent from App Event Q.
 */
vint VC_getAppEvent(
    VC_Event *event_ptr,
    char     *eventDesc_ptr,
    vint     *codecType_ptr,
    vint      timeout)
{

    _VC_Obj        *vc_ptr;
    _VC_Queues     *q_ptr;
    _VC_AppEventMsg msg;

    vc_ptr = &_VC_obj;
    q_ptr = vc_ptr->q_ptr;

    /* Set the timeout object */
    if (timeout < 0) {
        /* Then wait indefinitely */
        timeout = OSAL_WAIT_FOREVER;
    }
    else {
        /* Don't wait at all */
        timeout = OSAL_NO_WAIT;
    }

    if (_VC_APP_Q_EVENT_MSG_SZ != OSAL_msgQRecv(q_ptr->appEventQ, (char*)&msg,
            _VC_APP_Q_EVENT_MSG_SZ,
            timeout, NULL)) {
        /* Default to Event None in case of error. */
        *event_ptr = VC_EVENT_NONE;
        *codecType_ptr = -1;
        OSAL_strncpy(eventDesc_ptr, "VC - No Event", VCI_EVENT_DESC_STRING_SZ);
    }
    else {
        *event_ptr = msg.event;
        *codecType_ptr = msg.codec;
        OSAL_strncpy(eventDesc_ptr, msg.eventDesc, VCI_EVENT_DESC_STRING_SZ);
    }
    return _VC_OK;
}

/*
 * ======== VC_sendEncodedFrame() =======
 * Send the H264 or H263 Encoded Frame.
 */
vint VC_sendEncodedFrame(
    uint8 *data_ptr,
    vint   length,
    uint64 tsMs,
    uint8  rcsRtpExtnPayload)
{
    _VC_Obj  *vc_ptr;
    vc_ptr = &_VC_obj;

    return _VC_videoStreamSendEncodedData(vc_ptr, data_ptr, length, tsMs, rcsRtpExtnPayload);
}
/*
 * ======== VC_getEncodedFrame() =======
 * Get the H264 or H263 Encoded Frame.
 */
vint VC_getEncodedFrame(
    uint8 **data_ptr,
    vint   *length,
    uint64 *tsMs,
    vint   *flags,
    uint8  *rcsRtpExtnPayload)
{
    vint ret;
    _VC_Obj  *vc_ptr;
    vc_ptr = &_VC_obj;
    ret = _VC_videoStreamGetDataToDecode(vc_ptr, data_ptr, length, tsMs, flags, rcsRtpExtnPayload);

    return ret;
}

/*
 * ======== VCI_sendFIR() =======
 * Trigger an RTCP FIR
 */
vint VC_sendFIR(
    void)
{
    _VC_TRACE(__FILE__, __LINE__);
    /* The DSP will be null if not fully initialized */
    if (_VC_obj.dsp_ptr) {
        _VC_videoStreamSendFir(&_VC_obj);
        return _VC_OK;
    }
    return _VC_ERROR;
}
