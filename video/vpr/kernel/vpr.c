/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004-2010 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev$ $Date$
 */

#include <osal.h>
#include <vpr_comm.h>
#include "_vpr.h"

/* Global VPR_Obj pointer */
VPR_kernObj *_VPR_Kern_Obj_ptr = NULL;

/*
 * ======== _VPR_kernQueueRecvTask() ========
 *
 * This task is to receive command or event from VPR user space.
 *
 * Return:
 *  None
 */
static OSAL_TaskReturn _VPR_kernQueueRecvTask(
    OSAL_TaskArg arg_ptr)
{
    VPR_kernObj    *vpr_ptr;
    VPR_Comm       *comm_ptr;
    VPR_Net        *net_ptr;

    vpr_ptr = (VPR_kernObj*) arg_ptr;
    comm_ptr = &vpr_ptr->commQueueRecv;

    /* Create message queue */
    if (NULL == (_VPR_Kern_Obj_ptr->queue.vprToUserQ = OSAL_msgQCreate(
            VPR_KERNEL_TO_USER_Q_NAME,
            OSAL_MODULE_VPR_KERNEL, OSAL_MODULE_VPR, OSAL_DATA_STRUCT_VPR_Comm,
            VPR_MSGQ_DEPTH, sizeof(VPR_Comm), 0))) {
        OSAL_logMsg("%s:%d Could not crate %s queue.\n", __FUNCTION__, __LINE__,
                VPR_KERNEL_TO_USER_Q_NAME);
        return (OSAL_FAIL);
    }

    /* Create receiving queue which is from user space */
    if (NULL == (vpr_ptr->queue.vprFromUserQ = OSAL_msgQCreate(
            VPR_USER_TO_KERNEL_Q_NAME,
            OSAL_MODULE_VPR, OSAL_MODULE_VPR_KERNEL, OSAL_DATA_STRUCT_VPR_Comm,
            VPR_MSGQ_DEPTH, sizeof(VPR_Comm), 0))) {
        OSAL_logMsg("%s:%d Could not crate %s queue.\n", __FUNCTION__, __LINE__,
                VPR_USER_TO_KERNEL_Q_NAME);
        return (0);
    }
    vpr_ptr->taskRunning = OSAL_TRUE;

    while (OSAL_TRUE == vpr_ptr->taskRunning) {
        /* Clear before read */
        OSAL_memSet(comm_ptr, sizeof(VPR_Comm), 0);

        /*
         * Check if there are any messages to process.
         */
        if (0 >= OSAL_msgQRecv(_VPR_Kern_Obj_ptr->queue.vprFromUserQ, comm_ptr,
                sizeof(VPR_Comm), OSAL_WAIT_FOREVER, NULL)) {
            OSAL_taskDelay(100);
            continue;
        }

        switch (comm_ptr->type) {
            case VPR_TYPE_NETWORK_MODE:
                vpr_ptr->networkMode = comm_ptr->u.networkMode;
                OSAL_logMsg("%s:%d Mode=%d\n", __FUNCTION__, __LINE__,
                        vpr_ptr->networkMode);
                break;
            case VPR_TYPE_NET:
                net_ptr = &comm_ptr->u.vprNet;
                switch (net_ptr->type) {
                    case VPR_NET_TYPE_RTP_RECV_PKT:
                        _VPR_dispatchVoiceRtp(vpr_ptr, comm_ptr);
                        break;
                    default:
                        OSAL_logMsg("%s:%d Invalid VPR net type %d\n",
                                __FUNCTION__, __LINE__, net_ptr->type);
                        break;
                }
                break;
            default:
                OSAL_logMsg("%s:%d Invalid VPR type %d\n",
                        __FUNCTION__, __LINE__, comm_ptr->type);
                break;
        }
    }

    vpr_ptr->taskRunning = OSAL_FALSE;
    return (0);
}

/*
 * ======== VPR_kernInit() ========
 *
 * Init VPR kernel module.
 *
 * Return:
 *  OSAL_SUCCESS: init successful
 *  OSAL_FAIL: init failed
 */
OSAL_Status VPR_kernInit(
    void)
{
    vint    idx;

    /* Allocate memory for VPR_kernObj */
    if (NULL == (_VPR_Kern_Obj_ptr =
            OSAL_memCalloc(1, sizeof(VPR_kernObj), 0))) {
        OSAL_logMsg("%s:%d Failed to allocate memory for global VPR_Obj\n",
                __FUNCTION__, __LINE__);
        return (OSAL_FAIL);
    }

    /* Init voice rtp socket */
    for (idx = 0; idx < VPR_MAX_VOICE_STREAMS; idx++) {
        _VPR_Kern_Obj_ptr->voiceRtpSockets[idx].id = VPR_SOCKET_ID_NONE;
        _VPR_Kern_Obj_ptr->voiceRtpSockets[idx].referenceId =
                VPR_SOCKET_ID_NONE;
    }

    /* Create FIFO for vpr voice rtp sockets */
    for (idx = 0; idx < VPR_MAX_VOICE_STREAMS; idx++) {
        OSAL_snprintf(_VPR_Kern_Obj_ptr->voiceRtpSockets[idx].fifoPath,
                sizeof(_VPR_Kern_Obj_ptr->voiceRtpSockets[idx].fifoPath),
                "%s%d", VPR_VOER_READ_FIFO_NAME, idx);
        /* Delete FIFO */
        OSAL_fileFifoDelete(_VPR_Kern_Obj_ptr->voiceRtpSockets[idx].fifoPath);
        /* Create FIFO */
        if (OSAL_SUCCESS != OSAL_fileFifoCreate(
                _VPR_Kern_Obj_ptr->voiceRtpSockets[idx].fifoPath,
                VPR_MSGQ_DEPTH, sizeof(VPR_Comm))) {
            OSAL_logMsg("%s:%d Failed creating %s FIFO", __FUNCTION__, __LINE__,
                    _VPR_Kern_Obj_ptr->voiceRtpSockets[idx].fifoPath);
            return (OSAL_FAIL);
        }
    }

    /* Create recv task */
    if (0 == (_VPR_Kern_Obj_ptr->vprQueueRecvTaskId = OSAL_taskCreate(
            VPR_KERNEL_RECV_Q_TASK, OSAL_TASK_PRIO_NRT,
            VPR_KERNEL_TASK_STACK_BYTES, _VPR_kernQueueRecvTask,
            (void *)_VPR_Kern_Obj_ptr))) {
        OSAL_logMsg("%s:%d Failed creating %s daemon\n",
                __FUNCTION__, __LINE__, VPR_KERNEL_RECV_Q_TASK);
        return (OSAL_FAIL);
    }

    /* Set default mode to LTE */
    _VPR_Kern_Obj_ptr->networkMode = VPR_NETWORK_MODE_LTE;

    return (OSAL_SUCCESS);
}

/*
 * ======== VPR_kernShutdown() ========
 *
 * Shutdown VPR kernel module.
 *
 * Return:
 *  None
 */
void VPR_kernShutdown(
    void)
{
    vint    idx;

    _VPR_Kern_Obj_ptr->taskRunning = OSAL_FALSE;
    OSAL_taskDelete(_VPR_Kern_Obj_ptr->vprQueueRecvTaskId);
    OSAL_msgQDelete(_VPR_Kern_Obj_ptr->queue.vprToUserQ);
    OSAL_msgQDelete(_VPR_Kern_Obj_ptr->queue.vprFromUserQ);

    for (idx = 0; idx < VPR_MAX_VOICE_STREAMS; idx++) {
        _VPR_Kern_Obj_ptr->voiceRtpSockets[idx].id = -1;
        _VPR_Kern_Obj_ptr->voiceRtpSockets[idx].referenceId = -1;
    }

    OSAL_memFree(_VPR_Kern_Obj_ptr, 0);
}

