/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004-2010 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Revision: 1381 $ $Date: 2006-12-05 14:44:53 -0800 (Tue, 05 Dec 2006) $
 */
#include <osal.h>
#include <osal_net.h>
#include <vpr_comm.h>
#include <vpad_vpmd.h>
#include <vier_net.h>
#include <voer_net.h>
#include <sip_sip.h>
#include <sip_hdrflds.h>
#include <sip_mem_pool.h>
#include "_vpr.h"
#include "_vpr_daemon.h"

extern VPR_Obj *_VPR_Obj_ptr;

void VPR_destory(void)
{
    _VPR_daemonStop(_VPR_Obj_ptr);
    _VPR_shutdown(&_VPR_Obj_ptr);
    VPMD_destroy();    
}


/*
 * ======== VPR_stopDaemon() ========
 *
 * Function to stop VRP daemon
 *
 * Returns: 
 * None.
 */
void VPR_stopDaemon()
{
    _VPR_daemonStop(_VPR_Obj_ptr);
    _VPR_shutdown(&_VPR_Obj_ptr);
    VPMD_destroy();
}

/*
 * ======== VPR_startDaemon() ========
 *
 * Function to start VRP daemon
 *
 * Returns: 
 * OSAL_SUCCESS: VPR daemon started successfully.
 * OSAL_FAIL: VPR daemon started failed.
 */
OSAL_Status VPR_startDaemon()
{
    /* Initialize VPMD */
    if (OSAL_FAIL == VPMD_init()) {
        OSAL_logMsg("%s:%d Init VPMD failed.\n", __FUNCTION__, __LINE__);
        return (OSAL_FAIL);
    }

    /* Initialize VPR */
    if (OSAL_FAIL == _VPR_init(&_VPR_Obj_ptr)) {
        VPR_dbgPrintf("VPR initialization failed\n");
        return (OSAL_FAIL);
    }

    return _VPR_daemonGo(_VPR_Obj_ptr);
}

/*
 * ======== VPR_allocate() ========
 *
 * Public routine for allocating the VPR module resource
 *
 * Returns:
 *      OSAL_SUCCESS: function exits normally.
 *      OSAL_FAIL: in case of error
 */
OSAL_Status VPR_allocate(void)
{ 
    /* Allocate memory for VPR_Obj */
    if (NULL == (_VPR_Obj_ptr = OSAL_memCalloc(1, sizeof(VPR_Obj), 0))) {
        VPR_dbgPrintf("Failed to allocate memory for global VPR_Obj\n");
        return (OSAL_FAIL);
    }
    /* Allocate for VMPD */
    if (OSAL_SUCCESS != _VPR_allocateVpmd()) {
        VPR_dbgPrintf("Failed to allocate _VPR_allocateVpmd \n");
        return (OSAL_FAIL);
    }
    /* Allocate for VIDEO */
    if (OSAL_SUCCESS != _VPR_allocateVideo()) {
        VPR_dbgPrintf("Failed to allocate _VPR_allocateVideo\n");
        return (OSAL_FAIL);
    }
    /* Allocate for Kernel */
    if (OSAL_SUCCESS != _VPR_allocateKernel()) {
        VPR_dbgPrintf("Failed to allocate _VPR_allocateKernel\n");
        return (OSAL_FAIL);
    }
        /* Set default mode to LTE */
    _VPR_Obj_ptr->networkMode = VPR_NETWORK_MODE_LTE;
    return (OSAL_SUCCESS);
}

OSAL_Status VPR_start(void)
{
    if (OSAL_SUCCESS != _VPR_startIsipTask(_VPR_Obj_ptr)) {
        OSAL_logMsg("Failed to start VPR queue task\n");
        return (OSAL_FAIL);
    }
    if (OSAL_SUCCESS != _VPR_startVideoTask(_VPR_Obj_ptr)) {
        OSAL_logMsg("Failed to start video task\n");
        return (OSAL_FAIL);
    }        
    if (OSAL_SUCCESS != _VPR_startVpmdTask(_VPR_Obj_ptr)) {
        OSAL_logMsg("Failed to start VPMD task\n");
        return (OSAL_FAIL);
    }   
    if (OSAL_SUCCESS != _VPR_startKernelTask(_VPR_Obj_ptr)) {
        OSAL_logMsg("Failed to start Kernel task\n");
        return (OSAL_FAIL);
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
    OSAL_Status ret;

    if (VPR_NETWORK_MODE_WIFI == _VPR_Obj_ptr->networkMode) {
        /* Get voer socket by socket id */
        if (NULL == (sock_ptr = _VPR_allocSocket(_VPR_Obj_ptr))) {
            VPR_dbgPrintf("Failed to get available vpr socket.\n");
            return (OSAL_FAIL);
        }
        sock_ptr->type = type;
        /* Assign back */
        *socket_ptr = sock_ptr->id;
        VPR_dbgPrintf("VPR Create Ref. socket:%d\n", *socket_ptr);
        return (OSAL_SUCCESS);
    }
    else if (VPR_NETWORK_MODE_LTE == _VPR_Obj_ptr->networkMode) {
        /* 4G */
        ret = OSAL_netSocket(socket_ptr, type);
        VPR_dbgPrintf("Create socket in 4G mode. Id:%d\n", *socket_ptr);
        return (ret);
    }
    else {
        VPR_dbgPrintf("Could not create socket."
                "The network mode is not WIFI or LTE.\n");
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
    if (VPR_NETWORK_MODE_LTE == _VPR_Obj_ptr->networkMode) {
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
    
    /* get socket index from socket id */
    if (NULL == (sock_ptr = _VPR_getSocketById(_VPR_Obj_ptr,
            *socket_ptr))) {
        return (OSAL_netBindSocket(socket_ptr, address_ptr));
    }
    else {
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
            VPR_dbgPrintf("VPR Create and Bind RTCP socket:%d\n",
                    *socket_ptr);
            /* Clear the reference socket */
            _VPR_freeSocket(sock_ptr);              
            return (OSAL_netBindSocket(socket_ptr, address_ptr));
        } 
        /* Send command to VOER to create a socket */
        if (OSAL_SUCCESS == VPR_constructVoerPacket(&sock_ptr->id, NULL,
                0, &sock_ptr->localAddress, NULL,
                &_VPR_Obj_ptr->commSend, sock_ptr->tos,
                VPR_NET_TYPE_CREATE_VOICE_RTP)) {
            VPR_dbgPrintf("Send command to create VOER socket. Id:%d\n",
                    sock_ptr->id);
            return (VPMD_writeVoiceStream(&_VPR_Obj_ptr->commSend,
                    sizeof(VPR_Comm)));
        }
        return (OSAL_SUCCESS);
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

    /* get socket index from socket id */
    if (NULL == (sock_ptr = _VPR_getSocketById(_VPR_Obj_ptr,
            *socket_ptr))) {
        return (OSAL_netSetSocketOptions(socket_ptr, option, value));
    }
    else {
        /* set the option to VPR voer socket object */
        switch(option) {
            case OSAL_NET_SOCK_NONBLOCKING:
                sock_ptr->nonBlock= value;
                break;
            case OSAL_NET_SOCK_REUSE:
                sock_ptr->reuse = value;
                break;
            case OSAL_NET_IP_TOS:
                sock_ptr->tos = (OSAL_NetSockType)value;
                break;
            case OSAL_NET_SOCK_RCVTIMEO_SECS:
            default:
                break;
        }
    }
    return (OSAL_SUCCESS);

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
    VPR_Socket *sock_ptr;
    OSAL_NetSockId id;
    /* get socket index from socket id */
    id = *socket_ptr;
    if (NULL == (sock_ptr = _VPR_getSocketById(_VPR_Obj_ptr,
            id))) {
        return (OSAL_netSocketSendTo(socket_ptr, buf_ptr, size_ptr,
                address_ptr));
    }
    else {   
        /*
             * If there is network error from VOER, VPR will not send RTP packets.
             */
        if (OSAL_TRUE == sock_ptr->error) {
            return (OSAL_FAIL);
        }
        if (OSAL_SUCCESS == VPR_constructVoerPacket(&sock_ptr->id, buf_ptr,
                *size_ptr, &sock_ptr->localAddress,
                address_ptr, &_VPR_Obj_ptr->commSend, sock_ptr->tos,
                VPR_NET_TYPE_RTP_SEND_PKT)) {
            return (VPMD_writeVoiceStream(&_VPR_Obj_ptr->commSend,
                    sizeof(VPR_Comm)));
        }
    }
    return (OSAL_FAIL);
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
    uint32      size;
    vint        readSz;
    VPR_Net    *net_ptr;
    VPR_Comm   *comm_ptr;
    VPR_Socket *sock_ptr;

    OSAL_SelectSet      fdSet;
    OSAL_SelectTimeval  timeout;
    OSAL_Boolean        isTimedOut;

    /* Get voer socket by socket id */
    if (NULL == (sock_ptr = _VPR_getSocketById(_VPR_Obj_ptr,
            *socket_ptr))) {
        return (OSAL_netSocketReceiveFrom(socket_ptr, buf_ptr, size_ptr,
            address_ptr));
    }

    /* NO WAIT */
    OSAL_memSet(&timeout, 0, sizeof(OSAL_SelectTimeval));
    /* Select Fd and read file. */
    OSAL_selectSetInit(&fdSet);
    OSAL_selectAddId(&sock_ptr->fifoId, &fdSet);

    if (OSAL_FAIL ==
            OSAL_select(&fdSet, NULL, &timeout, &isTimedOut)) {
        OSAL_logMsg("%s:%d File select FAIL.\n", __FUNCTION__, __LINE__);
        return (OSAL_FAIL);
    }
    if (isTimedOut == OSAL_TRUE) {
        return (OSAL_FAIL);
    }


    comm_ptr = &_VPR_Obj_ptr->commSend;
    readSz = sizeof(VPR_Comm);
    /* Read from FIFO */
    if (OSAL_FAIL == OSAL_fileRead(&sock_ptr->fifoId, comm_ptr, &readSz)) {
        OSAL_logMsg("%s:%d Read file FAIL.\n", __FUNCTION__, __LINE__);
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
    return (OSAL_FAIL);
}

/* 
 * ======== VPR_netVoiceConnectSocket() ========
 *
 * Wrapper function of net socket function.
 * It calls OSAL net function or internal handler net function 
 * depends on wifi mode or 4G mode.
 *
 * Returns:
 *  OSAL_SUCCESS: Success.
 *  OSAL_FAIL:    Failure.
 */
OSAL_Status VPR_netVoiceConnectSocket(
    OSAL_NetSockId  *socket_ptr,
    OSAL_NetAddress *address_ptr)
{
    VPR_Socket *sock_ptr;
    
    /* get socket index from socket id */
    if (NULL == (sock_ptr = _VPR_getSocketById(_VPR_Obj_ptr,
            *socket_ptr))) {
        return (OSAL_netConnectSocket(socket_ptr, address_ptr));
    }
    else {
        /* Check if RTCP */
        if (OSAL_TRUE == OSAL_netIsAddrLoopback(address_ptr)) {
            /* Create a real socket for rtcp */    
            if (OSAL_SUCCESS != OSAL_netSocket(
                    socket_ptr, sock_ptr->type)) {
                return (OSAL_FAIL);
            }
            VPR_dbgPrintf("VPR Create and Connect RTCP socket:%d\n",
                    *socket_ptr);
            /*Clear the reference socket*/
            _VPR_freeSocket(sock_ptr);              
            return (OSAL_netConnectSocket(socket_ptr,
                    address_ptr));
        }   
        return (OSAL_SUCCESS);
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
    VPR_Socket   *sock_ptr;
    VPR_Comm    *comm_ptr;

    VPR_dbgPrintf("VPR close socket id =%d\n", *socket_ptr);
    /* Get vier socket by socket id */
    if (NULL == (sock_ptr = _VPR_getSocketById(_VPR_Obj_ptr, *socket_ptr))) {
        return (OSAL_netCloseSocket(socket_ptr));
    }
    comm_ptr = &_VPR_Obj_ptr->commSend;
    /* Send commend to VPR to close socket */
    if (OSAL_SUCCESS != VPR_constructVoerPacket(
            &sock_ptr->id, NULL, 0, NULL, NULL,
            comm_ptr, sock_ptr->tos, VPR_NET_TYPE_CLOSE)) {
        return (OSAL_FAIL);
    }
    VPR_dbgPrintf("VPR close Ref. socket id =%d\n", sock_ptr->id);
    /* Free VPR_Socket */
    _VPR_freeSocket(sock_ptr);
    /* Tell VPR to close the corresponding socket */
    return VPMD_writeVoiceStream(comm_ptr, sizeof(VPR_Comm));
}
