/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004-2010 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Revision: 1381 $ $Date: 2006-12-05 14:44:53 -0800 (Tue, 05 Dec 2006) $
 */
#ifndef ___VPR_H__
#define ___VPR_H__

#define VPR_QUEUE_RECV_TASK_NAME        "vpr-queue"
#define VPR_VIDEO_TASK_NAME             "vpr-video"
#define VPR_VPMD_TASK_NAME              "vpr-vpmd"
#define VPR_MSGQ_DEPTH                  (8)

#define VPR_TASK_STACK_BYTES            (4096)
#define VPR_VIDEO_RTP_SZ_MAX            (1500)

#define VPR_SOCKET_ID_NONE              (-1)

#define VPR_VIDEO_CMD_QUEUE_NAME        "vtsp-cmdqVideo"
#define VPR_VIDEO_EVT_QUEUE_NAME        "vtsp-evtqVideo"
#define VPR_VIDEO_RTCP_MSG_QUEUE_NAME   "vtsp-rtcpmsgqVideo"
#define VPR_VIDEO_RTCP_EVT_QUEUE_NAME   "vtsp-rtcpevtqVideo"
#define VPR_SR_ISIP_SEND_QUEUE_NAME     "isi.ipc.protocol"
#define VPR_SR_ISIP_RECV_QUEUE_NAME     "vpr.isi.ipc"
#define VPR_CSM_INPUT_EVT_QUEUE_NAME    "csm.evt.in.q"

#define VPR_VIDEO_THREAD_CMD_FIFO_NAME  OSAL_IPC_FOLDER"vpr-video-thread-cmd"
#define VPR_VOER_READ_FIFO_NAME         OSAL_IPC_FOLDER"vpr-voer-read"

#define VPR_CALL_ID_LIST_MAX_ENTRIES    SIP_DIALOGS_MAX_ACTIVE
#define VPR_CALL_ID_STRING_SIZE         SIP_CALL_ID_HF_STR_SIZE

/* VE is kernel mode */
#ifndef OSAL_KERNEL_EMULATION
#define VPR_RECV_KERN_TASK_NAME         "vpr-kernel"
#define VPR_USER_TO_KERN_Q_NAME         "vpr-user2kernel"
#define VPR_KERN_TO_USER_Q_NAME         "vpr-kernel2user"
#endif

#ifndef VPR_DEBUG
#define VPR_dbgPrintf(fmt, args...)
#else
#define VPR_dbgPrintf(fmt, args...) \
         OSAL_logMsg("[%s:%d] " fmt, __FUNCTION__, __LINE__, ## args)
#endif

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
    /*
     * The fd inside the queue(qId) which will return to
     * vtspr as a socket id for further operation.
     */
    OSAL_NetSockId   id;
    /* Reference socket id which passed from VIER to mapping to a VPR_Socket */
    OSAL_NetSockId   referenceId;
    OSAL_NetAddress  localAddress;
    OSAL_NetAddress  remoteAddress;
    OSAL_NetSockType type;
    OSAL_NetSockType tos;
    OSAL_Boolean     error;
    OSAL_NetSslId   *ssl_ptr;
    struct {
        uint8        data[VPR_PACKET_BUFFER_SIZE_MAX];
        vint         length;
    } packetBuffer;  /* Buffer to cache packets from SR */
} VPR_SrSocket;

typedef char VPR_CallId[VPR_CALL_ID_STRING_SIZE];

/* Struct of VPR_Obj */
typedef struct {
    OSAL_TaskId  vprQueueRecvTaskId;
    OSAL_TaskId  vprVideoTaskId;
    OSAL_TaskId  vprVpmdTaskId;
#ifndef OSAL_KERNEL_EMULATION
    OSAL_TaskId  vprKernTaskId;
#endif
#ifdef OSAL_KERNEL_EMULATION
    VPR_Socket   voiceRtpSockets[VPR_MAX_VOICE_STREAMS];
#endif
    VPR_Socket   videoRtpSockets[VPR_MAX_VIDEO_STREAMS];
    VPR_SrSocket srSockets[VPR_MAX_SR_SOCKETS];
    struct {
        OSAL_MsgQId    videoCmdQ;
        OSAL_MsgQId    videoEvtQ;
        OSAL_MsgQId    videoRtcpCmdQ;
        OSAL_MsgQId    videoRtcpEvtQ;
        OSAL_MsgQId    isipSendQ;
        OSAL_MsgQId    isipRecvQ;
        OSAL_MsgQId    csmEvtQ;
#ifndef OSAL_KERNEL_EMULATION
        OSAL_MsgQId    vprToKernelQ;
        OSAL_MsgQId    vprFromKernelQ;
#endif
        OSAL_MsgQGrpId groupQ;
    } queue;
    struct {
        int vpmdVideoStreamFd;
        int vpmdVideoCmdEvtFd;
        int vpmdVoiceStreamFd;
        int vpmdSipFd;
        int vpmdIsipFd;
        int vpmdCsmEvtFd;
        int videoTaskCmdFifoFd;
    } fd;
    VPR_Comm        commQueueRecv;
    VPR_Comm        commSend;
    VPR_Comm        commVideoTask; /* For video thread use */
    VPR_Comm        commVpmdTask; /* For vpmd task use */
    VPR_Comm        commSr; /* For MSAPP/SIP task use */
    VPR_Comm        commSrNic; /* For MSAPP/SIP NIC task use */
    tCallIdHF       callIdList[VPR_CALL_ID_LIST_MAX_ENTRIES]; /* call id list */
    VPR_NetworkMode networkMode; /* The mode is what radio interface is used */
} VPR_Obj;

VPR_Socket* _VPR_getVideoSocketByReferenceId(
    VPR_Obj       *vpr_ptr,
    OSAL_NetSockId referenceId);

VPR_Socket* _VPR_createVideoSocket(
    VPR_Obj   *vpr_ptr,
    VPR_Net   *net_ptr);

OSAL_Status _VPR_closeVideoSocket(
    VPR_Obj   *vpr_ptr,
    VPR_Net   *net_ptr);

OSAL_Status _VPR_sendVideoRtp(
    VPR_Obj           *vpr_ptr,
    VPR_Net           *net_ptr,
    uint8             *packet_ptr,
    uint32             packetLen);

OSAL_Status _VPR_init(
    VPR_Obj **pVpr_ptr);

void _VPR_shutdown(
    VPR_Obj **pVpr_ptr);

OSAL_Status VPR_getAvalibleVoerSockedId(
    VPR_Obj *vpr_ptr);

vint VPR_getVoerSocketbyId (
    VPR_Obj      *vprObj_ptr,
    OSAL_NetSockId socketId);

OSAL_Status _VPR_sendVtspEvt(
    VPR_Obj  *vpr_ptr,
    VPR_Comm *comm_ptr);

OSAL_Status _VPR_recvVtspCmd(
    VPR_Obj  *vpr_ptr,
    VPR_Comm *comm_ptr);

OSAL_Status _VPR_sendRtcpCmd(
    VPR_Obj  *vpr_ptr,
    VPR_Comm *comm_ptr);

OSAL_Status _VPR_recvRtcpEvt(
    VPR_Obj   *vpr_ptr,
    VPR_Comm  *comm_ptr);

OSAL_Status _VPR_sendCsmEvt(
    VPR_Obj  *vpr_ptr,
    VPR_Comm *comm_ptr);

OSAL_Status VPR_constructVoerPacket(
    OSAL_NetSockId         *socket_ptr,
    const uint8            *packet_ptr,
    const uint32            packetLen,
    const OSAL_NetAddress  *lclAddress_ptr,
    const OSAL_NetAddress  *rmtAddress_ptr,
    VPR_Comm               *comm_ptr,
    OSAL_NetSockType        tos,
    VPR_NetType             type);

OSAL_Status _VPR_dispatchVoiceRtp(
        VPR_Obj   *vpr_ptr,
    VPR_Comm  *comm_ptr);

VPR_Socket* _VPR_allocSocket(
    VPR_Obj *vprObj_ptr);

VPR_Socket* _VPR_getSocketById(
    VPR_Obj      *vprObj_ptr,
    OSAL_NetSockId socketId);

OSAL_Status _VPR_freeSocket(
    VPR_Socket *sock_ptr);

OSAL_Status _VPR_netVoiceCreateRtcpSocket(
    VPR_Socket *sock_ptr);

OSAL_Status _VPR_sendVideoRtpError(
    VPR_Comm         *comm_ptr,
    VPR_NetStatusType statusType);

OSAL_Status _VPR_constructVideoRtpPacket(
    const uint8            *packet_ptr,
    const uint32            packetLen,
    const OSAL_NetAddress  *lclAddress_ptr,
    const OSAL_NetAddress  *rmtAddress_ptr,
    OSAL_NetSockId          socketId,
    VPR_Comm               *comm_ptr);

OSAL_Status _VPR_setNetworkMode(
    VPR_Obj  *vpr_ptr,
    VPR_NetworkMode mode);


OSAL_Status _VPR_allocateVpmd(void);
OSAL_Status _VPR_allocateVideo(void);
OSAL_Status _VPR_allocateKernel(void);

#endif

