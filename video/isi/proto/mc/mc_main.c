/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 30135 $ $Date: 2014-12-01 16:07:35 +0800 (Mon, 01 Dec 2014) $
 */

#include <osal_types.h>
#include <osal.h>
#include <osal_msg.h>
#include <settings.h>

#include "isi.h"
#include "isip.h"
#include "vtsp.h"
#include "_mc.h"
#include "_mc_tone.h"

/* Global object. */
static MC_IsiObj    *_MC_globalIsiObj_ptr;

/* prototypes */
vint MC_isiAllocate(void);
vint MC_isiStart(void);
vint MC_isiDestroy(void);

/* 
 * ======== _MC_systemIsiEvt() ========
 *
 * This populates an ISI event related to 'system' lemcl events.
 *
 * Return Values: 
 * None
 */   
static void _MC_systemIsiEvt(
    ISIP_SystemReason   reason,
    ISIP_Status         status,
    char               *ipcName_ptr,    
    char               *audioName_ptr,
    char               *streamName_ptr,
    ISIP_Message       *isi_ptr)
{
    isi_ptr->id = 0;
    isi_ptr->code = ISIP_CODE_SYSTEM;
    isi_ptr->protocol = ISI_PROTOCOL_VTSP;
    isi_ptr->msg.system.reason = reason;
    isi_ptr->msg.system.status = status;
    /* 
     * Tell ISI of the IPC names we are using for ISI communication, 
     * Audio control (tones) and stream control. 
     */
    isi_ptr->msg.system.protocolIpc[0] = 0;
    if (NULL != ipcName_ptr) {
        OSAL_snprintf(isi_ptr->msg.system.protocolIpc,
                ISI_ADDRESS_STRING_SZ, "%s", ipcName_ptr);
    }
        
    isi_ptr->msg.system.mediaIpc[0] = 0;
    if (NULL != audioName_ptr) {
        OSAL_snprintf(isi_ptr->msg.system.mediaIpc,
                ISI_ADDRESS_STRING_SZ, "%s", audioName_ptr);
    }
    
    isi_ptr->msg.system.streamIpc[0] = 0;
    if (NULL != streamName_ptr) {
        OSAL_snprintf(isi_ptr->msg.system.streamIpc,
                ISI_ADDRESS_STRING_SZ, "%s", streamName_ptr);
    }
    return;
}





/* 
 * ======== _MC_isiSystemCmd() ========
 * This function is the entry point for commands from ISI related to 
 * the protocol or "system".
 *
 * Return Values:
 * None.
 */
static void _MC_isiSystemCmd(
    MC_VtspObj    *vtspObj_ptr, 
    ISIP_Message  *cmd_ptr)
{
    if (ISIP_SYSTEM_REASON_START == cmd_ptr->msg.system.reason &&
            ISIP_STATUS_DONE == cmd_ptr->msg.system.status) {
        /* Then we successfully contacted/registered ISI */
        _MC_vtspTimerDestroy(vtspObj_ptr);
        MC_dbgPrintf("%s: MC Successfully registered with ISI\n", __FUNCTION__);        
    }
    else if (ISIP_SYSTEM_REASON_SHUTDOWN == cmd_ptr->msg.system.reason &&
            ISIP_STATUS_DONE == cmd_ptr->msg.system.status) {
        /* Restart the registration timer */
        _MC_vtspTimerInit(vtspObj_ptr);
    }
    return;
}

/* 
 * ======== _MC_isiTelEventSendDigit() ========
 * This function processes a ISI message and calls appropriate audio functions.
 * 
 * Return Values:
 * Nothing.
 * 
 */
static void _MC_isiTelEventSendDigit(
    MC_VtspObj    *vtspObj_ptr,
    ISIP_Message  *cmd_ptr,
    ISIP_Message  *isi_ptr,
    MC_StreamInfo *info_ptr,
    ISIP_TelEvent *te_ptr,    
    int           digit,
    int           duration)
{
    VTSP_Stream       *stream_ptr;
    uint32             dtmfMask;
    VTSP_Rfc4733Event  event;
    vint               infc;
    
    infc = 0; // infc always zero for MP.

    /* 
     * Find the coder that this stream is using for encoding data and then 
     * get the DTMF relay mask to determine if we do relay or tone.
     */
    stream_ptr = &vtspObj_ptr->vtsp.audio[info_ptr->streamId].vtspStream;
    dtmfMask = (1 << stream_ptr->encoder);

    /* Check if DTMFR is active */
    if ((stream_ptr->encodeType[VTSP_CODER_DTMF] != VTSP_CODER_UNAVAIL) && 
            (dtmfMask & stream_ptr->dtmfRelay)) {
        MC_dbgPrintf("%s: Commanding DTMF Tone. DTMF Relay encodeType:%d\n",
                __FUNCTION__, stream_ptr->encodeType[VTSP_CODER_DTMF]);

         /* Command DTMF relay */
        event.eventNum   = digit;   
        event.volume     = MC_TONE_DTMF_RELAY_VOLUME; 
        event.duration   = duration; 
        event.endPackets = 3;
        if (VTSP_OK != VTSP_streamSendRfc4733(infc, info_ptr->streamId,
                 VTSP_RFC4733_TYPE_EVENT, &event)) {
             /* Then notify ISI of the error */
             MC_telEvtIsiEvt(cmd_ptr->id, te_ptr->serviceId, 
                    ISIP_TEL_EVENT_REASON_ERROR, te_ptr->evt, isi_ptr);
             return;
        }
#ifdef MC_ENABLE_TONE_LOCAL
        /* 
         * Generate the tone locally to the user as well.  If the 
         * command fails there is no need to tell ISI about the failure. 
         */
        MC_toneControl(infc, OSAL_TRUE, digit, duration);
#endif
    }
    else {
        MC_dbgPrintf("%s: Commanding DTMF Tone\n", __FUNCTION__);
        /* 
         * Then generate tone.  However if the encoder can not correctly
         * encode DTMF as 'detectable' tone then do nothing.
         */
        switch (stream_ptr->encoder) {
            case VTSP_CODER_G711U:
            case VTSP_CODER_G711A:
            case VTSP_CODER_G726_32K:
#ifndef MC_NO_TONE
                if (MC_OK != MC_toneStreamControl(infc, info_ptr->streamId,
                        OSAL_TRUE, digit, duration)) {
                    MC_telEvtIsiEvt(cmd_ptr->id, te_ptr->serviceId, 
                            ISIP_TEL_EVENT_REASON_ERROR, te_ptr->evt, isi_ptr);
                    MC_dbgPrintf("%s: MC_toneStreamControl Failed\n",
                            __FUNCTION__);
                    break;
                }
#endif
#ifdef MC_ENABLE_TONE_LOCAL
                /* 
                 * Generate the tone locally to the user as well.  If the 
                 * command fails there is no need to tell ISI about 
                 * the failure. 
                 */
                MC_toneControl(infc, OSAL_TRUE, digit, duration);
#endif
                break;
            default:
                /* Could not generate the tone. Return Error  */
                MC_telEvtIsiEvt(cmd_ptr->id, te_ptr->serviceId, 
                        ISIP_TEL_EVENT_REASON_ERROR, te_ptr->evt, isi_ptr);
                MC_dbgPrintf("%s: Can't send DTMF tone for coder:%d\n",
                        __FUNCTION__, stream_ptr->encoder);
                break;
        }
    }

    /*
     * If the duration is MAX_INT32, the MAX_INT32 is for indicating the 
     * start of a digit. Need to send a complete event so that the rest TEL
     * event can be proceeded in ISI.
     */
    if (MAX_INT32 == duration) {
        /* Then notify ISI */
        MC_telEvtIsiEvt(vtspObj_ptr->telEvt.isiId, vtspObj_ptr->telEvt.isiServiceId,
                ISIP_TEL_EVENT_REASON_COMPLETE, vtspObj_ptr->telEvt.evt,
                &vtspObj_ptr->event.isiMsg);
        /* Clear this field for next time */
        vtspObj_ptr->telEvt.isiId = 0;
        MC_sendEvent(&vtspObj_ptr->event);
    }
}

/* 
 * ======== _MC_isiTelEventCmd() ========
 * This function processes a ISI message and calls appropriate audio functions.
 * 
 * Return Values:
 * Nothing.
 * 
 */
static void _MC_isiTelEventCmd(
    MC_VtspObj   *vtspObj_ptr,
    ISIP_Message *cmd_ptr,
    ISIP_Message *isi_ptr)
{
    ISIP_TelEvent     *te_ptr;
    MC_StreamInfo     *info_ptr;
    vint               digit;
    vint               duration;
    char               digits[ISI_ADDRESS_STRING_SZ];
    int                numDigits;
    int                i;
    
    /* The interface number 'infc' is always zero for the handset. */
    te_ptr = &cmd_ptr->msg.event;

    MC_dbgPrintf("%s:%d", __FUNCTION__, __LINE__);
        
    /* We only want the "new" reason */
    if (ISIP_TEL_EVENT_REASON_NEW != te_ptr->reason) {
        /* Then we are not interested in this event. Tell ISI about error. */
        MC_telEvtIsiEvt(cmd_ptr->id, te_ptr->serviceId, 
                ISIP_TEL_EVENT_REASON_ERROR, te_ptr->evt, isi_ptr);
        return;
    }
    
    /* Only DTMF telephone events are supported */
    OSAL_memSet(digits, 0, sizeof(digits));
    if (ISI_TEL_EVENT_DTMF == te_ptr->evt) {
        /* Single character mode */
        digits[0] = te_ptr->settings.args.arg0;
        digits[1] = 0; // null terminate
    }
    else if (ISI_TEL_EVENT_DTMF_STRING == te_ptr->evt) {
        /* Multiple character mode */        
        OSAL_strncpy(digits, te_ptr->settings.args.string, 
                ISI_ADDRESS_STRING_SZ);
    }
    else { 
        /* Then the event is not supported return an error */
        MC_telEvtIsiEvt(cmd_ptr->id, te_ptr->serviceId, 
                ISIP_TEL_EVENT_REASON_ERROR, te_ptr->evt, isi_ptr);
        return;
    }
    
    /* Map the callId of this ISI command to the stream */
    if (NULL == (info_ptr = MC_vtspGetStreamInfoViaCallId(
            vtspObj_ptr, te_ptr->callId))) {
        /* Can't find any info for this stream. Must not exist. Report error */
        MC_telEvtIsiEvt(cmd_ptr->id, te_ptr->serviceId, 
                ISIP_TEL_EVENT_REASON_ERROR, te_ptr->evt, isi_ptr);
        MC_dbgPrintf("%s:%d ERROR", __FUNCTION__, __LINE__);
        return;
    }

    /* Get number of characters to process */
    numDigits = OSAL_strlen(digits);

    /* Get the duration in ms to generate */
    duration = te_ptr->settings.args.arg1;

    MC_dbgPrintf("%s: processing %d chars=%s durMs=%d\n", __FUNCTION__, 
            numDigits, digits, duration);

    /* 
     * Cache the event ID and event type.  they will be used when 
     * the tone completes
     */
    vtspObj_ptr->telEvt.isiId = cmd_ptr->id;
    vtspObj_ptr->telEvt.evt = te_ptr->evt;
    vtspObj_ptr->telEvt.isiServiceId = te_ptr->serviceId;
    vtspObj_ptr->telEvt.numDigits = numDigits;

    for (i = 0; i < numDigits; i++) {
        if (',' != digits[i]) {
            /* Convert digit to generate from ascii char to enumerate value */
            digit = MC_toneAsciiToDigit(digits[i]);
            _MC_isiTelEventSendDigit(vtspObj_ptr, cmd_ptr, isi_ptr, info_ptr, 
                    te_ptr,digit, duration);
            /* Sleep for digit duration */
            OSAL_taskDelay(duration + 100); // add 100ms break to each tone.
        }
        else {
            OSAL_taskDelay(2000); // sleep 1 second for pause ','
            vtspObj_ptr->telEvt.numDigits--; // since we wont get an event for this
            // Bug 8232, if last digit sleep then send the complete
            if (0 == vtspObj_ptr->telEvt.numDigits) {
                /* Then notify ISI */
                MC_telEvtIsiEvt(vtspObj_ptr->telEvt.isiId,
                        vtspObj_ptr->telEvt.isiServiceId,
                        ISIP_TEL_EVENT_REASON_COMPLETE,
                        vtspObj_ptr->telEvt.evt, &vtspObj_ptr->event.isiMsg);
                /* Clear this field for next time */
                vtspObj_ptr->telEvt.isiId = 0;
                MC_sendEvent(&vtspObj_ptr->event);  
            }
        }
    }
}


/* 
 * ======== _MC_getCommand() ========
 *
 * This function reads commands from ISI.  These commands are read 
 * from the IPC used specifically used to for commands from ISI
 *
 * Return Values: 
 * MC_OK  A command from ISI was successfully read
 * MC_ERR There was a error when reading the IPC.
 *          No attempt should be made to process the data in cmd_ptr
 */ 
static vint _MC_getCommand(
    OSAL_MsgQId  *id_ptr,
    ISIP_Message *cmd_ptr)
{
    vint            size;
    size = sizeof(ISIP_Message);
    
    if (size != OSAL_msgQRecv(*id_ptr, (char*)cmd_ptr, size,
            OSAL_WAIT_FOREVER, NULL)) {
        return (MC_ERR);
    }
    return (MC_OK);
}

/* 
 * ======== _MC_isiProcessCmd() ========
 *
 * This is ISI command read task.  It loops foremcr
 * waiting for commands on the IPC that ISI wrote.
 * Commands read from the IPC are then sent to an OSAL
 * queue processed by another task.
 *
 * Return Values: 
 * None.
 */
void _MC_isiProcessCmd(
    MC_IsiObj    *isiObj_ptr,
    MC_VtspObj   *vtspObj_ptr,
    ISIP_Message *cmd_ptr)
{
    ISIP_Message *msg_ptr;
    
    msg_ptr = &isiObj_ptr->event.isiMsg;

    /* 
     * Before we do anything clear our the buffer used to construct isi events.
     * Since isiObj_ptr->code will be ISIP_CODE_INVALID after this call, then 
     * We know if there is an event to write to isi at the bottom of this
     * function by seeing if anyone changed the isi event buffer from 
     * ISIP_CODE_INVALID to something else.
     */
    OSAL_memSet(msg_ptr, 0, sizeof(ISIP_Message));

    MC_dbgPrintf("%s: Recv'd an ISI command code=%d in the ISI IPC\n", 
            __FUNCTION__, cmd_ptr->code);
    
    /* Process the ISI command */
    switch (cmd_ptr->code) {
        case ISIP_CODE_MEDIA:
            MC_isiMediaCmd(vtspObj_ptr, cmd_ptr);
            break;
        case ISIP_CODE_TEL_EVENT:
            _MC_isiTelEventCmd(vtspObj_ptr, cmd_ptr, msg_ptr);
            break;
        case ISIP_CODE_SYSTEM:
            _MC_isiSystemCmd(vtspObj_ptr, cmd_ptr);
            break;
        case ISIP_CODE_DIAGNOSTIC:
        default:
            MC_dbgPrintf("%s: Failed to process ISI command\n", __FUNCTION__);
            break;
    }
    
    if (msg_ptr->code != ISIP_CODE_INVALID) {
        /* Then there is an event waiting to be written to ISI */
        MC_sendEvent(&isiObj_ptr->event);
    }
}

/* 
 * ======== _MC_isiCommandTask() ========
 *
 * This is ISI command read task.  It loops foremcr
 * waiting for commands on the IPC that ISI wrote.
 * Commands read from the IPC are then sent to an OSAL
 * queue processed by another task.
 *
 * Return Values: 
 * None.
 */   
static void _MC_isiCommandTask(
    void *arg_ptr)
{
    MC_IsiObj    *isiObj_ptr;
    MC_VtspObj   *vtspObj_ptr;
    ISIP_Message *cmd_ptr;
    
    isiObj_ptr = (MC_IsiObj *)arg_ptr;
    /* any better way than this? */
    vtspObj_ptr = _MC_vtspGetObjPtr();
    
    cmd_ptr = &isiObj_ptr->queue.msg.u.isi;
    
_MC_ISI_COMMAND_TASK_LOOP:
    
    /* Block on command message from ISI. */
    while (MC_OK != _MC_getCommand(&isiObj_ptr->queue.readIpcId, cmd_ptr));

    _MC_isiProcessCmd(isiObj_ptr, vtspObj_ptr, cmd_ptr);

    goto _MC_ISI_COMMAND_TASK_LOOP;
}

/*
 * ======== _MC_isiDeallocate() ========
 *
 * Internal routine for free up the MC_isi module resources
 *
 * Returns:
 *      OSAL_SUCCESS: function exits normally.
 *      OSAL_FAIL: in case of error
 */
vint _MC_isiDeallocate(
    MC_IsiObj   *isiObj_ptr)
{
    /* Kill the IPC */
    if (OSAL_SUCCESS != OSAL_msgQDelete(isiObj_ptr->queue.readIpcId)) {
        return(OSAL_FAIL);
    }
    return (OSAL_SUCCESS);
}

/*
 * ======== _MC_isiStop() ========
 *
 * Internal routine for stoping the MC_isi module task
 *
 * Returns:
 *      OSAL_SUCCESS: function exits normally.
 *      OSAL_FAIL: in case of error
 */
vint _MC_isiStop(
    MC_IsiObj   *isiObj_ptr)
{
    /* Delete task. */
    if (OSAL_SUCCESS != OSAL_taskDelete(isiObj_ptr->task.tId)) {
        return(OSAL_FAIL);
    }
    return (OSAL_SUCCESS);
}

/* 
 * ======== _MC_isiConfig() ========
 * This function initializes the following.
 * 1) The IPC used to read commands from ISI 
 * 2) The object for the MC_Isi Module
 *
 * Return Values:
 * MC_OK All resources were successfully initialized
 * MC_ERR Failed to init one of the resources needed to read commands
 *          from ISI.
 */
int _MC_isiConfig(
    void        *cfg_ptr,
    MC_IsiObj   *isiObj_ptr)
{
    char      *value_ptr;
    char      *ipc_ptr;

    /* Get the names of the ipc interfaces */
    ipc_ptr = NULL;

    /* Get "this" */
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_MC,
            SETTINGS_NESTED_NONE, cfg_ptr, SETTINGS_TAG_INTERFACE,
            NULL, NULL, SETTINGS_PARM_THIS))) {
        ipc_ptr = value_ptr;
    }

    if (NULL == ipc_ptr) {
        /* The IPC names are manditory */
        return (MC_ERR);
    }

    /* assumption MC_vtsp module is inited already with isiEvent queue id */
    _MC_vtspDupIsiEventQid(isiObj_ptr);

    /* Init ISI Cmd interface. */
    /* Create IPC for ISI commands. */
    if (0 == (isiObj_ptr->queue.readIpcId = OSAL_msgQCreate(ipc_ptr, 
            OSAL_MODULE_ISI, OSAL_MODULE_MC, OSAL_DATA_STRUCT_ISIP_Message,
            MC_MAX_QUEUE_DEPTH, sizeof(ISIP_Message), 0))) {
        MC_dbgPrintf("Try 'sudo'\n");
        MC_vtspDestroy();
        return (OSAL_FAIL);
    }

    return (MC_OK);
}


/*
 * ======== MC_isiAllocate() ========
 *
 * Public routine for allocating the MC_isi module resource
 *
 * Returns:
 *      OSAL_SUCCESS: function exits normally.
 *      OSAL_FAIL: in case of error
 */
vint MC_isiAllocate(void)
{
    void *cfg_ptr;
    
    /* Init the global object tht manages everything for this process */
    _MC_globalIsiObj_ptr = OSAL_memCalloc(1, sizeof(MC_IsiObj), 0);

    cfg_ptr = SETTINGS_cfgMemAlloc(SETTINGS_TYPE_MC);
    if (SETTINGS_RETURN_OK != SETTINGS_getContainer(SETTINGS_TYPE_MC,
            NULL, cfg_ptr)) {
        SETTINGS_memFreeDoc(SETTINGS_TYPE_MC, cfg_ptr);
        OSAL_logMsg("%s:%d ERROR opening settings\n",
                __FUNCTION__, __LINE__);
        return (OSAL_FAIL);
    }
    /*
     * Init MC_Isi module
     */
    if (MC_OK != _MC_isiConfig(cfg_ptr, _MC_globalIsiObj_ptr)) {
        SETTINGS_memFreeDoc(SETTINGS_TYPE_MC, cfg_ptr);
        OSAL_logMsg("Failed to init MC!\n");
        return (OSAL_FAIL);
    }
    SETTINGS_memFreeDoc(SETTINGS_TYPE_MC, cfg_ptr);

    return (OSAL_SUCCESS);
}

/*
 * ======== MC_isiStart() ========
 *
 * Public routine for starting the MC_isi module tasks/actions
 *
 * Returns:
 *      OSAL_SUCCESS: function exits normally.
 *      OSAL_FAIL: in case of error
 */
vint MC_isiStart(void)
{
    MC_IsiObj   *isiObj_ptr = _MC_globalIsiObj_ptr;
    MC_TaskObj  *task_ptr;
    
    task_ptr = &isiObj_ptr->task;

    /* Init the task used to read and process commands from ISI */
    task_ptr->tId      = 0;
    task_ptr->stackSz  = MC_TASK_STACK_BYTES;
    task_ptr->pri      = OSAL_TASK_PRIO_NRT;
    task_ptr->func_ptr = _MC_isiCommandTask;
    OSAL_snprintf(task_ptr->name, sizeof(task_ptr->name), "%s",
            MC_ISI_INFC_TASK_NAME);
    task_ptr->arg_ptr  = (void *)isiObj_ptr;
    if (0 == (task_ptr->tId = OSAL_taskCreate(
            task_ptr->name,
            (OSAL_TaskPrio)task_ptr->pri,
            task_ptr->stackSz,
            (OSAL_TaskPtr)task_ptr->func_ptr,
            (void *)task_ptr->arg_ptr))) {
        OSAL_msgQDelete(isiObj_ptr->queue.readIpcId);
        return (OSAL_FAIL);
    }
    return (OSAL_SUCCESS);
}

int MC_isiInit(void *cfg_ptr)
{
    /* Init the global object tht manages everything for this process */
    _MC_globalIsiObj_ptr = OSAL_memCalloc(1, sizeof(MC_IsiObj), 0);

    if (MC_OK != _MC_isiConfig(cfg_ptr, _MC_globalIsiObj_ptr)) {
        OSAL_logMsg("Failed to Init the MC Application\n");
        return (-1);
    }
    
    /* Init the task used to read and process commands from ISI */
    if (OSAL_FAIL == MC_isiStart()) {
        _MC_isiDeallocate(_MC_globalIsiObj_ptr);
        return (MC_ERR);
    }
    
    return (0);
}

/*
 * ======== MC_isiDestroy() ========
 *
 * Public routine for shutting down the MC_isi manager package.
 *
 * Returns:
 *      OSAL_SUCCESS: function exits normally.
 *      OSAL_FAIL: in case of error
 */
vint MC_isiDestroy(void)
{
    vint ret1, ret2;
    
    ret1 = _MC_isiStop(_MC_globalIsiObj_ptr);
    ret2 = _MC_isiDeallocate(_MC_globalIsiObj_ptr);

    if ((ret1 == OSAL_SUCCESS) && (ret2 == OSAL_SUCCESS)) {
        return (OSAL_SUCCESS);
    } else {
        return (OSAL_FAIL);
    }
}

/* deprecated */
int MC_init(void *cfg_ptr)
{
    /* init MC_Vtsp module first */
    if (MC_OK != MC_vtspInit(cfg_ptr)) {
        OSAL_logMsg("Failed to Init the MC vtsp \n");
        return (-1);
    }
    
    /* then init MC_Isi module */
    if (MC_OK != MC_isiInit(cfg_ptr)) {
        OSAL_logMsg("Failed to Init the MC isi\n");
        return (-1);
    }

    return (0);
}

/* deprecated */
void MC_shutdown(void)
{
    /* Tell ISI that we are going down */
    _MC_systemIsiEvt((ISIP_SystemReason)ISIP_SYSTEM_REASON_SHUTDOWN, 
            ISIP_STATUS_DONE, 
            NULL, NULL, NULL, &_MC_globalIsiObj_ptr->event.isiMsg);
    
    MC_sendEvent(&_MC_globalIsiObj_ptr->event);
    
    /* Clean house and exit */
    MC_isiDestroy();
    MC_vtspDestroy();

    return;
}

/*
 * ======== MC_allocate() ========
 *
 * Public routine for allocate the MC  modules tasks/actions
 *
 * Returns:
 *      OSAL_SUCCESS: function exits normally.
 *      OSAL_FAIL: in case of error
 */
vint MC_allocate(void)
{
    /* init MC_Vtsp module first */
    if (OSAL_SUCCESS != MC_vtspAllocate()) {
        OSAL_logMsg("Failed to allocate the MC vtsp \n");
        return (OSAL_FAIL);
    }
    
    /* then init MC_Isi module */
    if (OSAL_SUCCESS != MC_isiAllocate()) {
        OSAL_logMsg("Failed to allocate the MC isi\n");
        return (OSAL_FAIL);
    }

    return (OSAL_SUCCESS);
}

/*
 * ======== MC_start() ========
 *
 * Public routine for starting the MC  modules tasks/actions
 *
 * Returns:
 *      OSAL_SUCCESS: function exits normally.
 *      OSAL_FAIL: in case of error
 */
vint MC_start(void)
{
    /* init MC_Vtsp module first */
    if (OSAL_SUCCESS != MC_vtspStart()) {
        OSAL_logMsg("Failed to start the MC vtsp \n");
        return (OSAL_FAIL);
    }
    
    /* then init MC_Isi module */
    if (OSAL_SUCCESS != MC_isiStart()) {
        OSAL_logMsg("Failed to start the MC isi\n");
        return (OSAL_FAIL);
    }

    return (OSAL_SUCCESS);
}

/*
 * ======== MC_destroy() ========
 *
 * Public routine for stoping the MC  modules tasks/actions
 *
 * Returns:
 *      OSAL_SUCCESS: function exits normally.
 *      OSAL_FAIL: in case of error
 */
void MC_destroy(void)
{
    /* Clean house and exit */
    MC_isiDestroy();
    MC_vtspDestroy();

    return;
}

