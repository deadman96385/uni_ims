/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004-2010 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Revision: 25157 $ $Date: 2014-03-17 00:59:26 +0800 (Mon, 17 Mar 2014) $
 */

#include <osal.h>
#include <osal_net.h>
#include <vpr_comm.h>
#include <vpad_vpmd.h>
#include <vier_net.h>
#include "_vier.h"

VIER_Obj *_VIER_Obj_ptr = NULL;

/* 
 * ======== _VIER_processVprComm() ========
 *
 * Private function to process VPR_Comm from modem processor.
 *
 * Returns:
 *  OSAL_SUCCESS: Success.
 *  OSAL_FAIL:    Failure.
 */
OSAL_Status _VIER_processVprComm(
    VIER_Obj *vier_ptr,
    VPR_Comm *comm_ptr)
{
    VPR_Net     *net_ptr;
    VIER_Socket *sock_ptr;
    uint8       *msg_ptr;

    if (VPR_TYPE_VTSP_CMD == comm_ptr->type) {
        /*
         * Video vtsp command from modem processor, send to video engine.
         */
        VIER_dbgPrintf("Got command from VTSP.\n");
        msg_ptr = (uint8 *)&comm_ptr->u.vtspCmd;
        if (OSAL_SUCCESS != (OSAL_msgQSend(_VIER_Obj_ptr->queue.cmdQVideo,
                msg_ptr, _VTSP_Q_CMD_MSG_SZ, OSAL_NO_WAIT, NULL))) {
            VIER_dbgPrintf("Failed to write video engine vtsp command Q.\n");
            return (OSAL_FAIL);
        }
    }
    else if (VPR_TYPE_RTCP_EVT == comm_ptr->type) {
        /* 
         * Video rtcp event from modem processor, send to video engine.
         */
        VIER_dbgPrintf("Got rtcp event from VTSP.\n");
        msg_ptr = (uint8 *)&comm_ptr->u.vtspRtcpEvt;
        if (OSAL_SUCCESS != OSAL_msgQSend(vier_ptr->queue.rtcpEvtQId,
                msg_ptr, _VTSP_Q_RTCP_EVENT_SZ, OSAL_NO_WAIT, NULL)) {
            VIER_dbgPrintf("Failed to write rtcp event Q.\n");
            return (OSAL_FAIL);
        }
    }
    else if (VPR_TYPE_NET == comm_ptr->type) {
        net_ptr = &comm_ptr->u.vprNet;
        /* Video rtp from modem processor, get vier socket */
        if (NULL == (sock_ptr =
                _VIER_getSocketById(vier_ptr, net_ptr->referenceId))) {
            VIER_dbgPrintf("Failed to find vier socket:%d.\n",
                    net_ptr->referenceId);
            return (OSAL_FAIL);
        }

        OSAL_logMsg("[D2Log] _VIER_processVprComm, type = %d, netType = %d\n",
                comm_ptr->type, net_ptr->type);
        switch (net_ptr->type) {
            case VPR_NET_TYPE_RTP_RECV_PKT:
                /* Write to msg Q */
                if (OSAL_SUCCESS != OSAL_msgQSend(sock_ptr->qId, comm_ptr,
                        sizeof(VPR_Comm), OSAL_WAIT_FOREVER, NULL)) {
                    VIER_dbgPrintf("Failed to write vier socket Q.\n");
                    return (OSAL_FAIL);
                }
                break;
            case VPR_NET_TYPE_ERROR:
                VIER_dbgPrintf("Receive network error from modem. Type:%d\n",
                        net_ptr->u.status.evtType);
                if (VPR_NET_STATUS_OPEN_ERROR == net_ptr->u.status.evtType) {
                    /*
                     * There is socket create error from modem processor.
                     * Set error flag so VIER won't send rtp packe to VPR.
                     */
                    sock_ptr->error = OSAL_TRUE;
                }
                break;
            default:
                VIER_dbgPrintf("Invalid VPR net type:%d\n", net_ptr->type);
                return (OSAL_FAIL);
        }
    }
    else if (VPR_TYPE_NETWORK_MODE == comm_ptr->type) {
        /* Network mode changed */
        vier_ptr->networkMode = comm_ptr->u.networkMode;
        VIER_dbgPrintf("Network mode changed to %d\n", vier_ptr->networkMode);
    }

    return (OSAL_SUCCESS);
}

/* 
 * ======== _VIER_recvVtspEvt() ========
 * This is used to receive video vtsp event from video engine.
 * If there is any event, send it to VPAD.
 *
 * Returns:
 *  OSAL_SUCCESS: Success.
 *  OSAL_FAIL:    Failure.
 */
OSAL_Status _VIER_recvVtspEvt(
    VIER_Obj *vier_ptr,
    VPR_Comm *comm_ptr)
{
    vint   size;
    uint8 *msg_ptr;

    msg_ptr = (uint8 *)&comm_ptr->u.vtspEvt;
    if (0 < (size = OSAL_msgQRecv(vier_ptr->queue.evtQVideo, msg_ptr,
            _VTSP_Q_EVENT_MSG_SZ, OSAL_NO_WAIT, NULL))) {
        if (_VTSP_Q_EVENT_MSG_SZ != size) {
            VIER_dbgPrintf("Incorrect vtsp event message size:%d\n", size);
            return (OSAL_FAIL);
        }

        comm_ptr->type = VPR_TYPE_VTSP_EVT;
        if (OSAL_SUCCESS != VPAD_WRITE_VIDEO_CMDEVT(comm_ptr, sizeof(VPR_Comm))) {
            VIER_dbgPrintf("Fail to write vtsp event message to VPAD\n");
            return (OSAL_FAIL);
        }
    }

    return (OSAL_SUCCESS);
}

/*
 * ======== _VIER_recvRtcpCmd() ========
 *
 * Private function to receive rtcp command message from video engine and
 * send to vpr
 *
 * Returns:
 *  OSAL_SUCCESS: Success.
 *  OSAL_FAIL:    Failure.
 */
OSAL_Status _VIER_recvRtcpCmd(
    VIER_Obj *vier_ptr,
    VPR_Comm *comm_ptr)
{
    vint   size;
    uint8 *msg_ptr; 

    msg_ptr = (uint8 *)&comm_ptr->u.vtspRtcpCmd;
    if (0 < (size = OSAL_msgQRecv(vier_ptr->queue.rtcpCmdQId, msg_ptr,
            _VTSP_Q_RTCP_MSG_SZ, OSAL_NO_WAIT, NULL))) {
        if (_VTSP_Q_RTCP_MSG_SZ != size) {
            VIER_dbgPrintf("Incorrect rtcp size:%d\n", size);
            return (OSAL_FAIL);
        }

#ifndef VIER_DISABLE_RTCP
        comm_ptr->type = VPR_TYPE_RTCP_CMD;
        if (OSAL_SUCCESS != VPAD_WRITE_VIDEO_CMDEVT(comm_ptr,
                sizeof(VPR_Comm))) {
            VIER_dbgPrintf("Fail to write rtcp msg to VPAD\n");
            return (OSAL_FAIL);
        }
#endif /* not defined VIER_DISABLE_RTCP */
    }

    return (OSAL_SUCCESS);
}

/*
 * ======== _VIER_daemon() ========
 *
 * This function is the VIER thread to read and process cmd from VPAD.
 *
 * Returns: 
 *  0: Normal exit.
 */
OSAL_TaskReturn _VIER_daemon(
    OSAL_TaskArg arg_ptr) 
{
    VIER_Obj     *vier_ptr = (VIER_Obj *)arg_ptr;
    VPR_Comm     *comm_ptr = &vier_ptr->commRecv;
#ifdef INCLUDE_VPAD
    vint          timeout = 10;
#endif

_VIER_DAEMON_WAIT_VPAD:
    /* Wait for VPAD ready */
    while (OSAL_FALSE == VPAD_IS_READY()) {
        /* let's sleep for a while. */
        OSAL_logMsg("%s:%d VPAD is not ready, waiting...\n",
                __FUNCTION__, __LINE__);
        OSAL_taskDelay(VPAD_ERROR_RECOVERY_DELAY);
    }

_VIER_DAEMON_LOOP:

    /*
     * Check if there are any messages to process, and if yes, get a message.
     * It blocks 10ms.
     */
    if (OSAL_SUCCESS == VPAD_READ_VIDEO_STREAM(comm_ptr, sizeof(VPR_Comm),
            timeout)) {
        _VIER_processVprComm(vier_ptr, comm_ptr);
    } else {
        /* query vpad readiness */
        if (OSAL_FALSE == VPAD_IS_READY()) {
            OSAL_logMsg("%s:%d VPAD failed to contact VPMD.\n",
                 __FUNCTION__, __LINE__);
            OSAL_taskDelay(VPAD_ERROR_RECOVERY_DELAY);
            goto _VIER_DAEMON_WAIT_VPAD;
        }
    }

    /*
     * Check if there is any vtsp command/event message to process.
     */
    if (OSAL_SUCCESS == VPAD_READ_VIDEO_CMDEVT(comm_ptr, sizeof(VPR_Comm),
            OSAL_NO_WAIT)) {
        _VIER_processVprComm(vier_ptr, comm_ptr);
    }

    /* 
     * Receive and process rtcp command msg from video engine.
     * It's non-blocking.
     */
    if (OSAL_SUCCESS != _VIER_recvRtcpCmd(vier_ptr, comm_ptr)) {
        VIER_dbgPrintf("Failed on receiving rtcp from video engine\n");
    }

    /* 
     * Receive and process video vtsp event msg from video engine.
     * It's non-blocking.
     */
    if (OSAL_SUCCESS != _VIER_recvVtspEvt(vier_ptr, comm_ptr)) {
        VIER_dbgPrintf("Failed on receiving vtsp event from video engine\n");
    }

    goto _VIER_DAEMON_LOOP;
    return 0;
}

/*
 * ======== _VIER_constructVideoPacket() ========
 *
 * This function is to construct VPR command object for sending video rtp
 * or rtcp packet.
 *
 * Returns: 
 * OSAL_SUCCESS: VPR_Comm object constructed.
 * OSAL_FAIL: Error in VPR_Comm object construction.
 */
OSAL_Status _VIER_constructVideoPacket(
    OSAL_NetSockId          referenceId,
    const uint8            *packet_ptr,
    const uint32            packetLen,
    const OSAL_NetAddress  *lclAddress_ptr,
    const OSAL_NetAddress  *rmtAddress_ptr,
    VPR_Comm               *comm_ptr,
    VPR_NetType             netType)
{
    if (VPR_NET_MAX_DATA_SIZE_OCTETS < packetLen) {
        return (OSAL_FAIL);
    }
    OSAL_memSet(comm_ptr, 0, sizeof(VPR_Comm));
    comm_ptr->type = VPR_TYPE_NET;
    comm_ptr->u.vprNet.type = netType;
    comm_ptr->u.vprNet.referenceId = referenceId;
    if (lclAddress_ptr) {
        comm_ptr->u.vprNet.localAddress = *lclAddress_ptr;
    }
    if (rmtAddress_ptr){
        comm_ptr->u.vprNet.remoteAddress = *rmtAddress_ptr;
    }
    comm_ptr->u.vprNet.u.packet.chunkNumber = 1;
    comm_ptr->u.vprNet.u.packet.packetEnd= 1;
    comm_ptr->u.vprNet.u.packet.tosValue = 0;
    OSAL_memCpy(comm_ptr->u.vprNet.u.packet.packetData, packet_ptr, packetLen);
    comm_ptr->u.vprNet.u.packet.packetLen = packetLen;
    return (OSAL_SUCCESS);
}

/*
 * ======== _VIER_allocSocket() ========
 *
 * Private function to allocate a VIER_Socket 
 *
 * Returns: 
 * -1: No available vier socket.
 * Otherwise: The index of vier socket.
 */
VIER_Socket* _VIER_allocSocket(
    VIER_Obj *vierObj_ptr)
{
    vint              idx;
    _VIER_MsgQParams *arg_ptr;

    if (NULL == vierObj_ptr) {
        return (NULL);
    }

    for (idx = 0; idx < VIER_MAX_VIDEO_STREAMS; idx++) {
        if (-1 == vierObj_ptr->socket[idx].socketId) {
            arg_ptr = (_VIER_MsgQParams*)vierObj_ptr->socket[idx].qId;
            /* Get fid from qId and set it to socketId */
            vierObj_ptr->socket[idx].socketId = arg_ptr->fid;
            return (&vierObj_ptr->socket[idx]);
        }
    }

    return (NULL);
}

/*
 * ======== _VIER_getSocketById() ========
 *
 * Private function to get vier socket index by socket id.
 *
 * Returns: 
 * NULL: Cannot find vier socket.
 * Otherwise: The pointer of found vier socket.
 */
VIER_Socket* _VIER_getSocketById(
    VIER_Obj      *vierObj_ptr,
    OSAL_NetSockId socketId)
{
    vint idx;

    if (NULL == vierObj_ptr) {
        return (NULL);
    }

    for (idx = 0; idx < VIER_MAX_VIDEO_STREAMS; idx++) {
        if (socketId == vierObj_ptr->socket[idx].socketId) {
            return (&vierObj_ptr->socket[idx]);
        }
    }

    return (NULL);
}

/*
 * ======== _VIER_freeSocket() ========
 *
 * Private function to free a VIER_Socket 
 *
 * Returns:
 * OSAL_SUCCESS: Success.
 * OSAL_FAIL: Failed.
 */
OSAL_Status _VIER_freeSocket(
    VIER_Socket *sock_ptr)
{
    VPR_Comm    *comm_ptr;

    if (NULL == sock_ptr) {
        return (OSAL_FAIL);
    }
    /* Flush msgQ */
    comm_ptr = &_VIER_Obj_ptr->commRtpRecv;
    while (0 <= OSAL_msgQRecv(sock_ptr->qId, comm_ptr, sizeof(VPR_Comm), 
            OSAL_NO_WAIT, NULL)) {
    }

    /* Clean data */
    OSAL_memSet(&sock_ptr->localAddress, 0, sizeof(OSAL_NetAddress));
    OSAL_memSet(&sock_ptr->remoteAddress, 0, sizeof(OSAL_NetAddress));
    sock_ptr->socketId = VIER_SOCKET_ID_NONE;

    return (OSAL_SUCCESS);
}

/*
 * ======== _VIER_isVprHostedSocket() ========
 *
 * Private function to check if the socket is VPR hosted socket
 *
 * Returns:
 * OSAL_TRUE: It's VPR hosted socket.
 * OSAL_FALSE: It's not VPR hosted socket.
 */
OSAL_Boolean _VIER_isVprHostedSocket(
    OSAL_NetSockId   *socket_ptr)
{
    vint idx;

    /* Loop and search if the socket id is in the VPR_Socket pool */
    for (idx = 0; idx < VIER_MAX_VIDEO_STREAMS; idx++) {
        if (*socket_ptr == _VIER_Obj_ptr->socket[idx].socketId) {
            return (OSAL_TRUE);
        }
    }

    /* Cannot find it, must be local socket */
    return (OSAL_FALSE);
}

/* 
 * ======== _VIER_netSocket() ========
 *
 * Private function to allocate vier socket that will be used internally
 * inside vier.
 *
 * Returns:
 *  OSAL_SUCCESS: Success.
 *  OSAL_FAIL:    Failure.
 */
OSAL_Status _VIER_netSocket(
    OSAL_NetSockId   *socket_ptr,
    OSAL_NetSockType  type)
{
    VIER_Socket *sock_ptr;

    /* Get vier socket by socket id */
    if (NULL == (sock_ptr = _VIER_allocSocket(_VIER_Obj_ptr))) {
        VIER_dbgPrintf("Failed to get available vier socket.\n");
        return (OSAL_FAIL);
    }

    sock_ptr->type = type;
    /* Assign back */
    *socket_ptr = sock_ptr->socketId;

    VIER_dbgPrintf("Allocated VIER socket id:%d\n", *socket_ptr);
    return (OSAL_SUCCESS);
}

/* 
 * ======== _VIER_netCloseSocket() ========
 *
 * Private function to close and free vier socket.
 *
 * Returns:
 *  OSAL_SUCCESS: Success.
 *  OSAL_FAIL:    Failure.
 */
OSAL_Status _VIER_netCloseSocket(
    OSAL_NetSockId   *socket_ptr)
{
    VIER_Socket *sock_ptr;
    VPR_Comm    *comm_ptr;

    /* Get vier socket by socket id */
    if (NULL == (sock_ptr = _VIER_getSocketById(_VIER_Obj_ptr, *socket_ptr))) {
        VIER_dbgPrintf("Failed to find vier socket.\n");
        return (OSAL_FAIL);
    }

    comm_ptr = &_VIER_Obj_ptr->commSend;
    /* Send commend to VPR to close socket */
    if (OSAL_SUCCESS != _VIER_constructVideoPacket(
            sock_ptr->socketId, NULL, 0,
            NULL, NULL,
            comm_ptr, VPR_NET_TYPE_CLOSE)) {
        return (OSAL_FAIL);
    }
    VIER_dbgPrintf("Free VIER socket id:%d\n", *socket_ptr);
    /* Free VIER_Socket */
    _VIER_freeSocket(sock_ptr);
    /* Tell VPR to close the corresponding socket */
    return VPAD_WRITE_VIDEO_STREAM(comm_ptr, sizeof(VPR_Comm));
}

/* 
 * ======== _VIER_netBindSocket() ========
 *
 * Private function to bind vier socket.
 *
 * Returns:
 *  OSAL_SUCCESS: Success.
 *  OSAL_FAIL:    Failure.
 */
OSAL_Status _VIER_netBindSocket(
    OSAL_NetSockId   *socket_ptr,
    OSAL_NetAddress  *address_ptr)
{
    VIER_Socket *sock_ptr;
    VPR_Comm    *comm_ptr;

    /* Get vier socket by socket id */
    if (NULL == (sock_ptr = _VIER_getSocketById(_VIER_Obj_ptr, *socket_ptr))) {
        VIER_dbgPrintf("Failed to find vier socket.\n");
        return (OSAL_FAIL);
    }

    sock_ptr->localAddress = *address_ptr;
    /*
     * Set error as false. If there is error from VPR,
     * it will be set to ture and RTP packet will be not sent to modem.
     */
    sock_ptr->error = OSAL_FALSE;

    /*
     * Send bind message to VPR, and VPR will create and bind the socket.
     */
    comm_ptr = &_VIER_Obj_ptr->commSend;
    OSAL_memSet(comm_ptr, 0, sizeof(VPR_Comm));
    comm_ptr->type = VPR_TYPE_NET;
    comm_ptr->u.vprNet.type = VPR_NET_TYPE_CREATE_VIDEO_RTP;
    comm_ptr->u.vprNet.referenceId = sock_ptr->socketId;
    comm_ptr->u.vprNet.localAddress = *address_ptr;
    comm_ptr->u.vprNet.u.packet.tosValue = 0;

    /* Write to modem processor */
    return VPAD_WRITE_VIDEO_STREAM(comm_ptr, sizeof(VPR_Comm));
}

/* 
 * ======== _VIER_netSocketReceiveFrom() ========
 *
 * Private function to receive video packet from vier socket.
 *
 * Returns:
 *  OSAL_SUCCESS: Success.
 *  OSAL_FAIL:    Failure.
 */
OSAL_Status _VIER_netSocketReceiveFrom(
    OSAL_NetSockId  *socket_ptr,
    void            *buf_ptr,
    vint            *size_ptr,
    OSAL_NetAddress *address_ptr)
{
    uint32       size;
    VPR_Net     *net_ptr;
    VPR_Comm    *comm_ptr;
    VIER_Socket *sock_ptr;

    uint32       temp32;
    uint32      *packed_ptr;
    uint16       seqn;
	static uint32 total_num;

    //OSAL_logMsg("[D2Log] _VIER_netSocketReceiveFrom\n");
    /* Get vier socket by socket id */
    if (NULL == (sock_ptr = _VIER_getSocketById(_VIER_Obj_ptr, *socket_ptr))) {
        VIER_dbgPrintf("Failed to find vier socket.\n");
        return (OSAL_FAIL);
    }

    comm_ptr = &_VIER_Obj_ptr->commRtpRecv;
    /* Read from corresponding msg Q*/
    if (sizeof(VPR_Comm) !=
            OSAL_msgQRecv(sock_ptr->qId, comm_ptr, sizeof(VPR_Comm), 
            OSAL_NO_WAIT, NULL)) {
	//OSAL_logMsg("[D2Log] _VIER_netSocketReceiveFrom, NULL\n");
        return (OSAL_FAIL);
    }

    /* Verify it's a video rtp packet. */
    if (VPR_TYPE_NET == comm_ptr->type) {
        net_ptr = &comm_ptr->u.vprNet;
        if (VPR_NET_TYPE_RTP_RECV_PKT == net_ptr->type) {
            size = (net_ptr->u.packet.packetLen < (uint32)*size_ptr) ?
                    net_ptr->u.packet.packetLen : (uint32)*size_ptr;
            OSAL_memCpy(buf_ptr, net_ptr->u.packet.packetData, size);
            *size_ptr = size;
            *address_ptr = net_ptr->remoteAddress;

            packed_ptr   = (uint32 *)net_ptr->u.packet.packetData;
            temp32       = OSAL_netNtohl(*packed_ptr);
            packed_ptr++;
            seqn         = temp32 & 0xFFFF;
			total_num++;
            OSAL_logMsg("_VIER_netSocketReceiveFrom seq = %d\n",seqn);
			OSAL_logMsg("_VIER_netSocketReceiveFrom total_num = %d\n", total_num);
            return (OSAL_SUCCESS);
        } 
    }
    OSAL_logMsg("[D2Log] _VIER_netSocketReceiveFrom, FAIL\n");
    return (OSAL_FAIL);
}

/* 
 * ======== _VIER_netSocketSendTo() ========
 *
 * Private function to send video packet to vier socket.
 *
 * Returns:
 *  OSAL_SUCCESS: Success.
 *  OSAL_FAIL:    Failure.
 */
OSAL_Status _VIER_netSocketSendTo(
    OSAL_NetSockId  *socket_ptr,
    void            *buf_ptr,
    vint            *size_ptr,
    OSAL_NetAddress *address_ptr)
{
    VPR_Comm    *comm_ptr;
    VIER_Socket *sock_ptr;

    /* Get vier socket by socket id */
    if (NULL == (sock_ptr = _VIER_getSocketById(_VIER_Obj_ptr, *socket_ptr))) {
        VIER_dbgPrintf("Failed to find vier socket.\n");
        return (OSAL_FAIL);
    }

    /*
     * If there is network error from VPR, ViER will not send video packets
     * to modem.
     */
    if (OSAL_TRUE == sock_ptr->error) {
        return (OSAL_FAIL);
    }

    /* Construct video rtp packet */
    comm_ptr = &_VIER_Obj_ptr->commSend;
    if (OSAL_SUCCESS != _VIER_constructVideoPacket(
            sock_ptr->socketId, buf_ptr, *size_ptr,
            &sock_ptr->localAddress,
            address_ptr,
            comm_ptr, VPR_NET_TYPE_RTP_SEND_PKT)) {
        return (OSAL_FAIL);
    }
    /* Write to modem processor */
    return VPAD_WRITE_VIDEO_STREAM(comm_ptr, sizeof(VPR_Comm));
}

/* 
 * ======== _VIER_netSetSocketOptions() ========
 *
 * Private function to set socket options to vier socket.
 *
 * Returns:
 *  OSAL_SUCCESS: Success.
 *  OSAL_FAIL:    Failure.
 */
OSAL_Status _VIER_netSetSocketOptions(
    OSAL_NetSockId  *socket_ptr,
    OSAL_NetSockopt  option,
    int              value)
{
    VIER_Socket *sock_ptr;

    /* Get vier socket by socket id */
    if (NULL == (sock_ptr = _VIER_getSocketById(_VIER_Obj_ptr, *socket_ptr))) {
        VIER_dbgPrintf("Failed to find vier socket.\n");
        return (OSAL_FAIL);
    }

    /* Currently only store tos for sending RTP packets */
    switch (option) {
        case OSAL_NET_IP_TOS:
            sock_ptr->tos = value;
            break;
        default:
            break;
    }

    return (OSAL_SUCCESS);
}

