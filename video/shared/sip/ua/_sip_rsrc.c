/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 29950 $ $Date: 2014-11-20 09:42:47 +0800 (Thu, 20 Nov 2014) $
 */

#include "sip_app.h"
#include "sip_dbase_endpt.h"
#include "sip_dbase_sys.h"
#include "sip_xact.h"
#include "sip_xport.h"
#include "sip_ua.h"
#include "sip_timers.h"
#include "sip_dialog.h"
#include "_sip_helpers.h"
#include "_sip_rsrc.h"


static SIP_Rsrcs _SIP_rsrcs;

/* 
 *****************************************************************************
 * ================_SIP_errHandlerTask===================
 *
 * This function is used as the handler for the Error Task.
 * 
 * arg_ptr: The task object that has the task ID and OSAL queue ID.
 *
 * Returns: 
 *  Never returns. 
 * 
 ******************************************************************************
 */
static void _SIP_errHandlerTask(
    void *arg_ptr)
{
    tSipIpcMsg   msg;
    SIP_RsrcObj *err_ptr;
    
    err_ptr = (SIP_RsrcObj*)arg_ptr;
    while (1) {
        if (0 < OSAL_msgQRecv(err_ptr->msgQId, (char *)&msg, sizeof(tSipIpcMsg), 
                OSAL_WAIT_FOREVER, NULL)) {
            /* Got a message. Send down the 'error' UA. */
            UA_Entry(&msg);
        }
        else {
            OSAL_taskDelay(100);
        }
    }
}

/* 
 *****************************************************************************
 * ================_SIP_nicHandlerTask===================
 *
 * This function is used as the handler for the NIC Task.
 * 
 * arg_ptr: The task object that has all task data.
 *
 * Returns: 
 *  Never returns. 
 * 
 ******************************************************************************
 */
static int32 _SIP_nicHandlerTask(
    void *arg_ptr)
{
    SIP_Rsrcs   *rsrcs_ptr;
    
    rsrcs_ptr = (SIP_Rsrcs*)arg_ptr;

   /* this function below will never return */
    TRANSPORT_StartServer(
            rsrcs_ptr->nicUdpPort, 
            rsrcs_ptr->nicUdpFd,
            rsrcs_ptr->nicTcpPort,
            rsrcs_ptr->nicTcpFd);
    return (0);
}

/* 
 *****************************************************************************
 * ================SIP_rsrcDispatcher===================
 *
 * This function is what the SIP stack uses to dispatch queue
 * messages from the network task or the timer task to the UA task.
 *
 * hContext  : The UA's 'taskId'.  Which is actually interpritted as
 *             a pointer to the UA's tUaEvent object.
 
 * msg_ptr  : A pointer to the SIP defined IPC message.  
 *
 * Returns: 
 *  SIP_OK    : The message was properly dispatched to the appropriate task.
 *  SIP_FAILED: There was an error trying to dispatch the message.
 * 
 ******************************************************************************
 */

vint SIP_rsrcDispatcher(
    tSipHandle  hContext, 
    tSipIpcMsg *msg_ptr)
{
    
    /* 
     * hContext is really a handle to the UA's 'event' object which 
     * contains everything needed to handling events destrined for the
     * application.  
     */
    msg_ptr->hContext = hContext;
    if (hContext == 0) {
        /* Send it to the error task */
        if (OSAL_SUCCESS != OSAL_msgQSend(_SIP_rsrcs.err.msgQId,
                (char *)msg_ptr, sizeof(tSipIpcMsg), OSAL_WAIT_FOREVER, NULL)) {
            return (SIP_FAILED);
        }
    }
    else {
        /* send to the UA's queue. */
        if (OSAL_SUCCESS != OSAL_msgQSend(((tUaEvent*)hContext)->msgQId,
                (char *)msg_ptr, sizeof(tSipIpcMsg), OSAL_WAIT_FOREVER, NULL)) {
            return (SIP_FAILED);
        }
    }
    return(SIP_OK);
}

/* 
 *****************************************************************************
 * ================SIP_rsrcTmrDispatcher===================
 *
 * This function is what the SIP stack uses to dispatch queue
 * messages from the timer task to the UA task.
 *
 * hContext  : The UA's 'taskId'.  Which is actually interpritted as
 *             a pointer to the UA's tUaEvent object.
 
 * msg_ptr  : A pointer to the SIP defined IPC message.  
 *
 * Returns: 
 *  SIP_OK    : The message was properly dispatched to the appropriate task.
 *  SIP_FAILED: There was an error trying to dispatch the message.
 * 
 ******************************************************************************
 */

vint SIP_rsrcTmrDispatcher(
    tSipHandle  hContext, 
    tSipIpcMsg *msg_ptr)
{
    
    /* 
     * hContext is really a handle to the UA's 'event' object which 
     * contains everything needed to handling events destrined for the
     * application.  
     */
    msg_ptr->hContext = hContext;
    if (hContext == 0) {
        /* Send it to the error task */
        if (OSAL_SUCCESS != OSAL_msgQSend(_SIP_rsrcs.err.msgQId,
                (char *)msg_ptr, sizeof(tSipIpcMsg), OSAL_NO_WAIT, NULL)) {
            return (SIP_FAILED);
        }
    }
    else {
        /* send to the UA's queue. */
        if (OSAL_SUCCESS != OSAL_msgQSend(((tUaEvent*)hContext)->msgQId,
                (char *)msg_ptr, sizeof(tSipIpcMsg), OSAL_NO_WAIT, NULL)) {
            return (SIP_FAILED);
        }
    }
    return(SIP_OK);
}

/* 
 *****************************************************************************
 * ================SIP_rsrcInit===================
 *
 * This function is used as a "one stop shop" to initialize the SIP stack.
 * It will initialize any system resources needed internally by the stack.
 * 
 * pConfig = A pointer to a config object that contains stack configuration
 *           info see sip_app.h for more.
 * RETURNS:
 *         SIP_OK:     Initialization was successful
 *         SIP_FAILED: Initialization failed.  Could not allocate and or 
 *                     initialize the system resources required by the stack. 
 * 
 ******************************************************************************
 */
vint SIP_rsrcInit(tSipConfig *pConfig)
{
    /* Zero out the object used to manage internal resources */
    OSAL_memSet(&_SIP_rsrcs, 0, sizeof(SIP_Rsrcs));

    /* Init and launch the Error task */
    if (0 == (_SIP_rsrcs.err.msgQId = OSAL_msgQCreate(
            SIP_PORT_SIP_ERR_QUEUE_NAME,
            OSAL_MODULE_SIP, OSAL_MODULE_SIP, OSAL_DATA_STRUCT_tSipIpcMsg,
            SIP_MSGQ_LEN, sizeof(tSipIpcMsg), 0))) {
        return (SIP_FAILED);
    }
    
    if (0 == (_SIP_rsrcs.err.taskId = OSAL_taskCreate(
            SIP_PORT_SIP_ERR_TASK_NAME,
            OSAL_TASK_PRIO_NRT,
            SIP_PORT_SIP_ERR_TASK_STACK,
            (OSAL_TaskPtr)_SIP_errHandlerTask,
            (void *)&_SIP_rsrcs.err))) {
        OSAL_msgQDelete(_SIP_rsrcs.err.msgQId);
        return (SIP_FAILED);
    }

    /* SIP network interface task. One for all SIP traffic. */
    _SIP_rsrcs.nicUdpFd = pConfig->udpFd;
    _SIP_rsrcs.nicUdpPort = pConfig->udpPort;
    _SIP_rsrcs.nicTcpFd = pConfig->tcpFd;
    _SIP_rsrcs.nicTcpPort = pConfig->tcpPort;
    _SIP_rsrcs.nic.msgQId = 0; /* not used */
    
    if (0 == (_SIP_rsrcs.nic.taskId = OSAL_taskCreate(
            SIP_PORT_SIP_NIC_TASK_NAME,
            OSAL_TASK_PRIO_NIC,
            SIP_PORT_SIP_NIC_TASK_STACK,
            _SIP_nicHandlerTask,
            (void *)&_SIP_rsrcs))) {
        OSAL_taskDelete(_SIP_rsrcs.err.taskId);
        OSAL_msgQDelete(_SIP_rsrcs.err.msgQId);
        return (SIP_FAILED);
    }
    return (SIP_OK);
}

/* 
 *****************************************************************************
 * ================SIP_Destroy===================
 *
 * This function is used as a "one stop shop" to destroy the 
 * resources the SIP Stack uses.
 * RETURNS:
 *    Nothing.
 * 
 ******************************************************************************
 */
void SIP_rsrcDestroy(void)
{
    OSAL_taskDelete(_SIP_rsrcs.nic.taskId);
    
    /* Kill error task */
    OSAL_taskDelete(_SIP_rsrcs.err.taskId);
    OSAL_msgQDelete(_SIP_rsrcs.err.msgQId);
    
    return;
}

