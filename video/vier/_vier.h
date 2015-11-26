/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004-2010 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Revision: 23244 $ $Date: 2013-12-02 16:27:24 +0800 (Mon, 02 Dec 2013) $
 */

#ifndef __VIER_H_
#define __VIER_H_

#define VIER_TASK_NAME             "VES_vier"
#define VIER_TASK_STACK_BYTES      (4096)
#define VIER_SOCKET_Q_NAME         "vier-socket-q"
#define VIER_SOCKET_Q_DEPTH        (2)

#define VIER_SOCKET_ID_NONE        (-1)
#define VIER_MAX_VIDEO_STREAMS     (VPR_MAX_VIDEO_STREAMS)
#define VIER_PACKET_SIZE_MAX       (1500) /* It's max rtp size */

#ifndef VIER_DEBUG
#define VIER_dbgPrintf(fmt, args...)
#else
#define VIER_dbgPrintf(fmt, args...) \
         OSAL_logMsg("[%s:%d] " fmt, __FUNCTION__, __LINE__, ## args)
#endif

/* This struct is used to get the fid inside a msgQ */
typedef struct {
    char    name[128];
    int     fid;
    uint32  sz;
} _VIER_MsgQParams;

typedef struct {
    OSAL_MsgQId      qId;
    OSAL_NetSockId   socketId;
    OSAL_NetAddress  localAddress;
    OSAL_NetAddress  remoteAddress;
    OSAL_NetSockType type;
    vint             tos;
    OSAL_Boolean     error;
} VIER_Socket;

typedef struct {
    OSAL_TaskId     taskId;
    VIER_Socket     socket[VIER_MAX_VIDEO_STREAMS];
    VPR_Comm        commRecv;
    VPR_Comm        commSend;
    VPR_Comm        commRtpRecv; /* For rtp recv task of ve use */
    VPR_NetworkMode networkMode;
    struct {
        OSAL_MsgQId    cmdQVideo;
        OSAL_MsgQId    evtQVideo;
        OSAL_MsgQId    rtcpCmdQId;
        OSAL_MsgQId    rtcpEvtQId;
    } queue;
} VIER_Obj;


/* Private functions those will be used in vier.c */
OSAL_TaskReturn _VIER_daemon(
    OSAL_TaskArg arg_ptr);

OSAL_Status _VIER_constructVideoPacket(
    OSAL_NetSockId          referenceId,
    const uint8            *packet_ptr,
    const uint32            packetLen,
    const OSAL_NetAddress  *lclAddress_ptr,
    const OSAL_NetAddress  *rmtAddress_ptr,
    VPR_Comm               *comm_ptr,
    VPR_NetType             netType);

VIER_Socket* _VIER_allocSocket(
    VIER_Obj *vierObj_ptr);

VIER_Socket* _VIER_getSocketById(
    VIER_Obj      *vierObj_ptr,
    OSAL_NetSockId socketId);

OSAL_Status _VIER_freeSocket(
    VIER_Socket *sock_ptr);

OSAL_Boolean _VIER_isVprHostedSocket(
    OSAL_NetSockId   *socket_ptr);

OSAL_Status _VIER_netSocket(
    OSAL_NetSockId   *socket_ptr,
    OSAL_NetSockType  type);

OSAL_Status _VIER_netCloseSocket(
    OSAL_NetSockId   *socket_ptr);

OSAL_Status _VIER_netBindSocket(
    OSAL_NetSockId   *socket_ptr,
    OSAL_NetAddress  *address_ptr);

OSAL_Status _VIER_netSocketReceiveFrom(
    OSAL_NetSockId  *socket_ptr,
    void            *buf_ptr,
    vint            *size_ptr,
    OSAL_NetAddress *address_ptr);

OSAL_Status _VIER_netSocketSendTo(
    OSAL_NetSockId  *socket_ptr,
    void            *buf_ptr,
    vint            *size_ptr,
    OSAL_NetAddress *address_ptr);

OSAL_Status _VIER_netSetSocketOptions(
    OSAL_NetSockId  *socket_ptr,
    OSAL_NetSockopt  option,
    int              value);

#endif
