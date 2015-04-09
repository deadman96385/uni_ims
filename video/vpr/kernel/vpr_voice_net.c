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

extern VPR_kernObj *_VPR_Kern_Obj_ptr;

/*
 * ======== _VPR_allocSocket() ========
 *
 * Private function to allocate a VPR_Socket
 *
 * Returns:
 * -1: No available vpr socket.
 * Otherwise: The index of vpr socket.
 */
VPR_Socket* _VPR_allocSocket(
    VPR_kernObj    *vprObj_ptr)
{
    vint              idx;

    if (NULL == vprObj_ptr) {
        return (NULL);
    }

    for (idx = 0; idx < VPR_MAX_VOICE_STREAMS; idx++) {
        if (-1 == vprObj_ptr->voiceRtpSockets[idx].id) {
            /* Open FIFO */
            if (OSAL_SUCCESS != OSAL_fileOpen(
                    &vprObj_ptr->voiceRtpSockets[idx].fifoId,
                    vprObj_ptr->voiceRtpSockets[idx].fifoPath,
                    OSAL_FILE_O_RDWR, 0)) {
                OSAL_logMsg("%s:%d Open %s FIFO failed.\n", __FUNCTION__,
                        __LINE__, vprObj_ptr->voiceRtpSockets[idx].fifoPath);
                return (NULL);
            }
            /* Get fd from fd and set it to socketId */
            vprObj_ptr->voiceRtpSockets[idx].id =
                    vprObj_ptr->voiceRtpSockets[idx].fifoId;
            return (&vprObj_ptr->voiceRtpSockets[idx]);
        }
    }
    return (NULL);
}

/*
 * ======== _VPR_freeSocket() ========
 *
 * Private function to free a VPR_Socket
 *
 * Returns:
 * OSAL_SUCCESS: Success.
 * OSAL_FAIL: Failed.
 */
OSAL_Status _VPR_freeSocket(
    VPR_Socket *sock_ptr)
{

    if (NULL == sock_ptr) {
        return (OSAL_FAIL);
    }

    /* Close FIFO */
    OSAL_fileClose(&sock_ptr->fifoId);

    /* Clean data */
    OSAL_memSet(&sock_ptr->localAddress, 0, sizeof(OSAL_NetAddress));
    OSAL_memSet(&sock_ptr->remoteAddress, 0, sizeof(OSAL_NetAddress));
    sock_ptr->id = VPR_SOCKET_ID_NONE;
    sock_ptr->referenceId = VPR_SOCKET_ID_NONE;
    return (OSAL_SUCCESS);
}

/*
 * ======== _VPR_getSocketById() ========
 *
 * Private function to get vpr socket index by socket id.
 *
 * Returns:
 * NULL: Cannot find vpr socket.
 * Otherwise: The pointer of found vpr socket.
 */
VPR_Socket* _VPR_getSocketById(
    VPR_kernObj    *vprObj_ptr,
    OSAL_NetSockId  socketId)
{
    vint idx;

    if (NULL == vprObj_ptr) {
        return (NULL);
    }

    for (idx = 0; idx < VPR_MAX_VOICE_STREAMS; idx++) {
        if (socketId == vprObj_ptr->voiceRtpSockets[idx].id) {
            return (&vprObj_ptr->voiceRtpSockets[idx]);
        }
    }

    return (NULL);
}

/*
 * ======== _VPR_dispatchVoiceRtp() ========
 *
 * Private function to dispatch received rtp packet to corresponding VPR_Socket
 *
 * Returns:
 * OSAL_SUCCESS: voice dispatched successfully.
 * OSAL_FAIL: Error on dispatching voice rtp packet.
 */
OSAL_Status _VPR_dispatchVoiceRtp(
    VPR_kernObj    *vpr_ptr,
    VPR_Comm       *comm_ptr)
{
    VPR_Socket *sock_ptr = NULL;
    VPR_Net    *net_ptr;
    vint        size;

    net_ptr = &comm_ptr->u.vprNet;

    /* Get socket */
    if (NULL == (sock_ptr = _VPR_getSocketById(vpr_ptr,
            net_ptr->referenceId))) {
        OSAL_logMsg("%s:%d Failed to find voer socket:%d.\n",
                __FUNCTION__, __LINE__, net_ptr->referenceId);
        return (OSAL_FAIL);
    }

    /* Write to FIFO */
    size = sizeof(VPR_Comm);
    if (OSAL_SUCCESS != OSAL_fileWrite(&sock_ptr->fifoId, comm_ptr, &size)) {
        OSAL_logMsg("%s:%d Failed to write fifo.\n", __FUNCTION__, __LINE__);
        return (OSAL_FAIL);
    }

    return (OSAL_SUCCESS);
}

/*
 * ======== VPR_constructVoerPacket() ========
 *
 * This function is to construct VPR command object for sending rtp packet.
 *
 * Returns:
 * OSAL_SUCCESS: VPR_Comm object constructed.
 * OSAL_FAIL: Error in VPR_Comm object construction.
 */
OSAL_Status VPR_constructVoerPacket(
    OSAL_NetSockId         *socket_ptr,
    const uint8            *packet_ptr,
    const uint32            packetLen,
    const OSAL_NetAddress  *lclAddress_ptr,
    const OSAL_NetAddress  *rmtAddress_ptr,
    VPR_Comm               *comm_ptr,
    OSAL_NetSockType        tos,
    VPR_NetType             type)
{
    if (VPR_NET_MAX_DATA_SIZE_OCTETS < packetLen) {
        return (OSAL_FAIL);
    }
    OSAL_memSet(comm_ptr, 0, sizeof(VPR_Comm));

    comm_ptr->type = VPR_TYPE_NET;
    comm_ptr->u.vprNet.type = type;
    comm_ptr->u.vprNet.referenceId = *socket_ptr;

    if (lclAddress_ptr) {
        comm_ptr->u.vprNet.localAddress = *lclAddress_ptr;
    }
    if (rmtAddress_ptr){
        comm_ptr->u.vprNet.remoteAddress = *rmtAddress_ptr;
    }
    comm_ptr->u.vprNet.u.packet.chunkNumber = 1;
    comm_ptr->u.vprNet.u.packet.packetEnd = 1;
    if (VPR_NET_TYPE_RTP_SEND_PKT == type) {
        comm_ptr->u.vprNet.u.packet.tosValue = tos;
    }
    else if (VPR_NET_TYPE_RTP_RECV_PKT == type) {
        comm_ptr->u.vprNet.u.packet.tosValue = 0;
    }
    if (0 != packetLen) {
        OSAL_memCpy(comm_ptr->u.vprNet.u.packet.packetData, packet_ptr,
                packetLen);
        comm_ptr->u.vprNet.u.packet.packetLen = packetLen;
    }
    return (OSAL_SUCCESS);
}

/*
 * ======== VPR_netVoiceSocket() ========
 *
 * Wrapper function of net socket function.
 * It calls OSAL net function or internal handler net function
 * depends on wifi mode or 4G mode.
 *
 * Returns:
 *  OSAL_SUCCESS: Success.
 *  OSAL_FAIL:    Failure.
 */
OSAL_Status VPR_netVoiceSocket(
    OSAL_NetSockId   *socket_ptr,
    OSAL_NetSockType  type)
{
    VPR_Socket *sock_ptr;

    if (VPR_NETWORK_MODE_LTE == _VPR_Kern_Obj_ptr->networkMode) {
        return (OSAL_netSocket(socket_ptr, type));
    }
    else if (VPR_NETWORK_MODE_WIFI == _VPR_Kern_Obj_ptr->networkMode) {
        /* Get voer socket by socket id */
        if (NULL == (sock_ptr = _VPR_allocSocket(_VPR_Kern_Obj_ptr))) {
            OSAL_logMsg("%s:%d Failed to get available vpr socket.\n",
                    __FUNCTION__, __LINE__);
            return (OSAL_FAIL);
        }
        sock_ptr->type = type;
        /* Assign back */
        *socket_ptr = sock_ptr->id;
        OSAL_logMsg("%s:%d VPR Create Ref. socket:%d\n", __FUNCTION__, __LINE__,
                *socket_ptr);
        return (OSAL_SUCCESS);
    }
    else {
        OSAL_logMsg("%s:%d Could not create socket. "
                "The network mode is not WIFI or LTE.\n",
                __FUNCTION__, __LINE__);
        return (OSAL_FAIL);
    }
}

/*
 * ======== VPR_netIsSocketIdValid() ========
 *
 * This function returns OSAL_SUCCESS if passed in socket ID is valid.
 *
 * Returns:
 *  OSAL_SUCCESS: Socket ID is valid.
 *  OSAL_FAIL:    Socket ID in invalid.
 */
OSAL_Status VPR_netIsSocketIdValid(
    OSAL_NetSockId *socket_ptr)
{
    vint    i;

    if (VPR_NETWORK_MODE_LTE == _VPR_Kern_Obj_ptr->networkMode) {
        return (OSAL_netIsSocketIdValid(socket_ptr));
    }
    else {
        return (OSAL_SUCCESS);
    }
}

/*
 * ======== VPR_netVoiceBindSocket() ========
 *
 * Wrapper function of net socket function.
 * It calls OSAL net function or internal handler net function
 * depends on wifi mode or 4G mode.
 *
 * Returns:
 *  OSAL_SUCCESS: Success.
 *  OSAL_FAIL:    Failure.
 */
OSAL_Status VPR_netVoiceBindSocket(
    OSAL_NetSockId  *socket_ptr,
    OSAL_NetAddress *address_ptr)
{
    VPR_Socket *sock_ptr;

    if (VPR_NETWORK_MODE_LTE == _VPR_Kern_Obj_ptr->networkMode) {
        return (OSAL_netBindSocket(socket_ptr, address_ptr));
    }
    else {
        if (NULL == (sock_ptr = _VPR_getSocketById(_VPR_Kern_Obj_ptr,
                *socket_ptr))) {
            OSAL_logMsg("%s:%d Could not get vpr socket by Id\n",
                    __FUNCTION__, __LINE__);
            return (OSAL_FAIL);
        }
        sock_ptr->localAddress = *address_ptr;
        /* Check if RTCP */
        if (OSAL_TRUE == OSAL_netIsAddrLoopback(address_ptr)) {
            /* Create a real socket for rtcp */
            if (OSAL_SUCCESS != OSAL_netSocket(
                    socket_ptr, sock_ptr->type)) {
                return (OSAL_FAIL);
            }
            OSAL_netSetSocketOptions(socket_ptr,
                    OSAL_NET_IP_TOS, sock_ptr->tos);
            OSAL_logMsg("VPR Create and Bind RTCP socket:%d\n", *socket_ptr);
            /* Clear the reference socket */
            _VPR_freeSocket(sock_ptr);
            return (OSAL_netBindSocket(socket_ptr, address_ptr));
        }
        /* Send command to VOER to create a socket */
        if (OSAL_SUCCESS == VPR_constructVoerPacket(&sock_ptr->id, NULL,
                0, &sock_ptr->localAddress, NULL,
                &_VPR_Kern_Obj_ptr->commSend, sock_ptr->tos,
                VPR_NET_TYPE_CREATE_VOICE_RTP)) {
            OSAL_logMsg("Send command to create VOER socket. Id:%d\n",
                    sock_ptr->id);
            if (OSAL_SUCCESS == OSAL_msgQSend(
                    _VPR_Kern_Obj_ptr->queue.vprToUserQ,
                    &_VPR_Kern_Obj_ptr->commSend, sizeof(VPR_Comm),
                    OSAL_NO_WAIT, NULL)) {
                return (OSAL_SUCCESS);
            }
        }
        return (OSAL_FAIL);
    }
}

/*
 * ======== VPR_netVoiceSetSocketOptions() ========
 *
 * Wrapper function of net socket function.
 * It calls OSAL net function or internal handler net function
 * depends on wifi mode or 4G mode.
 *
 * Returns:
 *  OSAL_SUCCESS: Success.
 *  OSAL_FAIL:    Failure.
 */
OSAL_Status VPR_netVoiceSetSocketOptions(
    OSAL_NetSockId  *socket_ptr,
    OSAL_NetSockopt  option,
    int              value)
{
    VPR_Socket *sock_ptr;

    if (VPR_NETWORK_MODE_LTE == _VPR_Kern_Obj_ptr->networkMode) {
        return (OSAL_netSetSocketOptions(socket_ptr, option, value));
    }
    else {
        if (NULL == (sock_ptr = _VPR_getSocketById(_VPR_Kern_Obj_ptr,
                *socket_ptr))) {
            OSAL_logMsg("%s:%d Could not get vpr socket by Id\n",
                    __FUNCTION__, __LINE__);
            return (OSAL_FAIL);
        }

        /* set the option to VPR voer socket object */
        switch(option) {
            case OSAL_NET_SOCK_NONBLOCKING:
                sock_ptr->nonBlock= value;
                break;
            case OSAL_NET_SOCK_REUSE:
                sock_ptr->reuse = value;
                break;
            case OSAL_NET_IP_TOS:
                sock_ptr->tos = value;
                break;
            case OSAL_NET_SOCK_RCVTIMEO_SECS:
            default:
                break;
        }
        return (OSAL_SUCCESS);
    }
}

/*
 * ======== VPR_netVoiceSocketSendTo() ========
 *
 * Wrapper function of net socket function.
 * It calls OSAL net function or internal handler net function
 * depends on wifi mode or 4G mode.
 *
 * Returns:
 *  OSAL_SUCCESS: Success.
 *  OSAL_FAIL:    Failure.
 */
OSAL_Status VPR_netVoiceSocketSendTo(
    OSAL_NetSockId  *socket_ptr,
    void            *buf_ptr,
    vint            *size_ptr,
    OSAL_NetAddress *address_ptr)
{
    VPR_Socket     *sock_ptr;
    OSAL_NetSockId  id;

    /* get socket index from socket id */
    id = *socket_ptr;

    if (VPR_NETWORK_MODE_LTE == _VPR_Kern_Obj_ptr->networkMode) {
        return (OSAL_netSocketSendTo(socket_ptr, buf_ptr, size_ptr,
                address_ptr));
    }
    else {
        if (NULL == (sock_ptr = _VPR_getSocketById(_VPR_Kern_Obj_ptr,
                *socket_ptr))) {
            OSAL_logMsg("%s:%d Could not get vpr socket by Id\n",
                    __FUNCTION__, __LINE__);
            return (OSAL_FAIL);
        }

        /*
         * If there is network error from VOER, VPR will not send RTP packets.
         */
        if (OSAL_TRUE == sock_ptr->error) {
            return (OSAL_FAIL);
        }
        if (OSAL_SUCCESS == VPR_constructVoerPacket(&sock_ptr->id, buf_ptr,
                *size_ptr, &sock_ptr->localAddress, address_ptr,
                &_VPR_Kern_Obj_ptr->commSend, sock_ptr->tos,
                VPR_NET_TYPE_RTP_SEND_PKT)) {
            if (OSAL_SUCCESS == OSAL_msgQSend(
                    _VPR_Kern_Obj_ptr->queue.vprToUserQ,
                    &_VPR_Kern_Obj_ptr->commSend, sizeof(VPR_Comm),
                    OSAL_NO_WAIT, NULL)) {
                return (OSAL_SUCCESS);
            }
        }

        return (OSAL_FAIL);
    }
}

/*
 * ======== VPR_netVoiceSocketReceiveFrom() ========
 *
 * Wrapper function of net socket function.
 * It calls OSAL net function or internal handler net function
 * depends on wifi mode or 4G mode.
 *
 * Returns:
 *  OSAL_SUCCESS: Success.
 *  OSAL_FAIL:    Failure.
 */
OSAL_Status VPR_netVoiceSocketReceiveFrom(
    OSAL_NetSockId  *socket_ptr,
    void            *buf_ptr,
    vint            *size_ptr,
    OSAL_NetAddress *address_ptr)
{
    uint32              size;
    vint                readSz;
    VPR_Net            *net_ptr;
    VPR_Comm           *comm_ptr;
    VPR_Socket         *sock_ptr;
    OSAL_SelectSet      fdSet;
    OSAL_SelectTimeval  timeout;
    OSAL_Boolean        isTimedOut;

    if (VPR_NETWORK_MODE_LTE == _VPR_Kern_Obj_ptr->networkMode) {
        return (OSAL_netSocketReceiveFrom(socket_ptr, buf_ptr, size_ptr,
            address_ptr));
    }
    else {
        if (NULL == (sock_ptr = _VPR_getSocketById(_VPR_Kern_Obj_ptr,
                *socket_ptr))) {
            OSAL_logMsg("%s:%d Could not get vpr socket by Id\n",
                    __FUNCTION__, __LINE__);
            return (OSAL_FAIL);
        }

        /* NO WAIT */
        OSAL_memSet(&timeout, 0, sizeof(OSAL_SelectTimeval));

        /* Select Fd and read file. */
        OSAL_selectSetInit(&fdSet);
        OSAL_selectAddId(&sock_ptr->fifoId, &fdSet);

        if (OSAL_FAIL == OSAL_select(&fdSet, NULL, &timeout, &isTimedOut)) {
            return (OSAL_FAIL);
        }
        if (isTimedOut == OSAL_TRUE) {
            return (OSAL_FAIL);
        }

        comm_ptr = &_VPR_Kern_Obj_ptr->commSend;
        readSz = sizeof(VPR_Comm);
        /* Read from FIFO */
        if (OSAL_FAIL == OSAL_fileRead(&sock_ptr->fifoId, comm_ptr, &readSz)) {
            //OSAL_logMsg("%s:%d Read file FAIL.\n", __FUNCTION__, __LINE__);
            return (OSAL_FAIL);
        }

        /* Verify it's a voice rtp packet. */
        if (VPR_TYPE_NET == comm_ptr->type) {
            net_ptr = &comm_ptr->u.vprNet;
            if (VPR_NET_TYPE_RTP_RECV_PKT == net_ptr->type) {
                size = (net_ptr->u.packet.packetLen < (uint32)(*size_ptr)) ?
                        net_ptr->u.packet.packetLen : (uint32)(*size_ptr);
                OSAL_memCpy(buf_ptr, net_ptr->u.packet.packetData, size);
                *size_ptr = size;
                *address_ptr = net_ptr->remoteAddress;
                return (OSAL_SUCCESS);
            }
        }
        OSAL_logMsg("%s:%d Invalid rtp packet.\n", __FUNCTION__, __LINE__);
        return (OSAL_FAIL);
    }
}

/*
 * ======== VPR_netVoiceCloseSocket() ========
 *
 * Wrapper function of net socket function.
 * It calls OSAL net function or internal handler net function
 * depends on wifi mode or 4G mode.
 *
 * Returns:
 *  OSAL_SUCCESS: Success.
 *  OSAL_FAIL:    Failure.
 */
OSAL_Status VPR_netVoiceCloseSocket(
    OSAL_NetSockId *socket_ptr)
{
    VPR_Socket *sock_ptr;

    if (VPR_NETWORK_MODE_LTE == _VPR_Kern_Obj_ptr->networkMode) {
        return (OSAL_netCloseSocket(socket_ptr));
    }
    else {
        if (NULL == (sock_ptr = _VPR_getSocketById(_VPR_Kern_Obj_ptr,
                *socket_ptr))) {
            OSAL_logMsg("%s:%d Could not get vpr socket by Id\n",
                    __FUNCTION__, __LINE__);
            return (OSAL_FAIL);
        }

        /* Send commend to VPR to close socket */
        if (OSAL_SUCCESS != VPR_constructVoerPacket(
                &sock_ptr->id, NULL, 0, NULL, NULL,
                &_VPR_Kern_Obj_ptr->commSend,
                sock_ptr->tos, VPR_NET_TYPE_CLOSE)) {
            return (OSAL_FAIL);
        }
        OSAL_logMsg("%s:%d VPR close Ref. socket id =%d\n",
                __FUNCTION__, __LINE__, sock_ptr->id);
        /* Free VPR_Socket */
        _VPR_freeSocket(sock_ptr);

        if (OSAL_SUCCESS == OSAL_msgQSend(
                _VPR_Kern_Obj_ptr->queue.vprToUserQ,
                &_VPR_Kern_Obj_ptr->commSend, sizeof(VPR_Comm),
                OSAL_NO_WAIT, NULL)) {
            return (OSAL_SUCCESS);
        }
        return (OSAL_FAIL);
    }
}

EXPORT_SYMBOL(VPR_netVoiceSocket);
EXPORT_SYMBOL(VPR_netVoiceBindSocket);
EXPORT_SYMBOL(VPR_netVoiceSetSocketOptions);
EXPORT_SYMBOL(VPR_netVoiceSocketSendTo);
EXPORT_SYMBOL(VPR_netVoiceSocketReceiveFrom);
EXPORT_SYMBOL(VPR_netVoiceCloseSocket);
EXPORT_SYMBOL(VPR_netIsSocketIdValid);
