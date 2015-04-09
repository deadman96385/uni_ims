/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004-2010 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev$ $Date$
 */

#ifndef __VPR_H_
#define __VPR_H_

#define VPR_KERNEL_RECV_Q_TASK      "vpr-kern-task"
#define VPR_KERNEL_TASK_STACK_BYTES (4096)

#define VPR_KERNEL_TO_USER_Q_NAME   "vpr-kernel2user"
#define VPR_USER_TO_KERNEL_Q_NAME   "vpr-user2kernel"
#define VPR_MSGQ_DEPTH              (8)

#define VPR_VOER_READ_FIFO_NAME     OSAL_IPC_FOLDER"vpr-voer-read"
#define VPR_SOCKET_ID_NONE          (-1)

typedef struct {
    /* FIFO information */
    char                fifoPath[OSAL_MSG_PATH_NAME_SIZE_MAX];
    OSAL_FileId         fifoId;
    /*
     * The fd is the FIFO which will return to
     * vtspr as a socket id for further operation.
     */
    OSAL_NetSockId      id; 
    /* Reference socket id which passed from VIER to mapping to a VPR_Socket */
    OSAL_NetSockId      referenceId;
    OSAL_NetAddress     localAddress;
    OSAL_NetAddress     remoteAddress;
    OSAL_NetSockType    type;
    OSAL_NetSockType    tos;
    vint                nonBlock;
    vint                reuse;
    OSAL_Boolean        error;
    OSAL_NetSslId      *ssl_ptr;
} VPR_Socket;

typedef struct {
    OSAL_TaskId         vprQueueRecvTaskId; /* recevie vprFromUserQ */
    OSAL_Boolean        taskRunning;
    struct {
        OSAL_MsgQId     vprToUserQ;
        OSAL_MsgQId     vprFromUserQ;
    } queue;
    VPR_Socket          voiceRtpSockets[VPR_MAX_VOICE_STREAMS];
    VPR_Comm            commQueueRecv;
    VPR_Comm            commSend;
    VPR_NetworkMode     networkMode;
} VPR_kernObj;


VPR_Socket* _VPR_allocSocket(
    VPR_kernObj    *vprObj_ptr);

OSAL_Status _VPR_freeSocket(
    VPR_Socket *sock_ptr);

VPR_Socket* _VPR_getSocketById(
    VPR_kernObj    *vprObj_ptr,
    OSAL_NetSockId  socketId);

OSAL_Status _VPR_dispatchVoiceRtp(
    VPR_kernObj    *vpr_ptr,
    VPR_Comm       *comm_ptr);


#endif // __VPR_H_
