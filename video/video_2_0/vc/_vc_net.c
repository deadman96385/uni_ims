/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL AND
 * PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2005-2010 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 12652 $ $Date: 2010-07-29 19:44:47 -0400 (Thu, 29 Jul 2010) $
 *
 */
#include "_vc_private.h"
#include "video_net.h"
/*
 * Set to a positive value to start looping back data.
 * For example if
 * _VC_netLoopbackCount = -1 , data will never be looped back
 * if
 * _VC_netLoopbackCount = 100, first 100 packets will be looped back,
 * encoded data will not be sent while looping back.
 */
static int _VC_netLoopbackCount = -1;

/*
 * ======== _VC_netSocket() ========
 *
 * Open socket, set any necessary socket flags
 *
 */
OSAL_INLINE OSAL_NetSockId _VC_netSocket(
    OSAL_NetSockType type,
    int              flags)
{
    OSAL_NetSockId socketFd;

    /*
     * Create and reserve a socket descriptor. It will be bound when the
     * network is configured.
     */
    if (OSAL_FAIL == VIDEO_NET_SOCKET(&socketFd, type)) {
        OSAL_logMsg("%s %d\n", __FILE__, __LINE__);
        return (socketFd);
    }
    if (OSAL_FAIL == VIDEO_NET_SET_SOCKET_OPTIONS(&socketFd,
            OSAL_NET_SOCK_NONBLOCKING, 1)) {
        OSAL_logMsg("%s %d\n", __FILE__, __LINE__);
        return (_VC_RTP_ERROR);
    }
    if (OSAL_FAIL == VIDEO_NET_SET_SOCKET_OPTIONS(&socketFd,
            OSAL_NET_SOCK_REUSE, 1)) {
        OSAL_logMsg("%s %d\n", __FILE__, __LINE__);
        return (_VC_RTP_ERROR);
    }
    if (OSAL_FAIL == VIDEO_NET_SET_SOCKET_OPTIONS(&socketFd,
            OSAL_NET_IP_TOS, flags)) {
        OSAL_logMsg("%s %d\n", __FILE__, __LINE__);
        return (_VC_RTP_ERROR);
    }

    return (socketFd);
}

/*
 * ======== _VC_netClose() ========
 *
 * Close socket
 *
 */
OSAL_INLINE vint _VC_netClose(
    OSAL_NetSockId socketFd)
{
    if (OSAL_SUCCESS != VIDEO_NET_CLOSE_SOCKET(&socketFd)) { 
        _VC_TRACE(__FILE__, __LINE__);
        return (_VC_RTP_ERROR);
    }

    return (_VC_RTP_OK);
}

/*
 * ======== _VC_netBind() ========
 *
 * Bind socket, set any necessary socket flags
 *
 */
OSAL_INLINE vint _VC_netBind(
    OSAL_NetSockId  socketFd,
    OSAL_NetAddress address)
{
    if (OSAL_FAIL == VIDEO_NET_BIND_SOCKET(&socketFd, &address)) {
        _VC_TRACE(__FILE__, __LINE__);
        return (_VC_RTP_ERROR);
    }
    return (_VC_RTP_OK);
}

/*
 * ======== _VC_netRecvfrom() ========
 */
int _VC_netRecvfrom(
    OSAL_NetSockId   socketFd,
    void            *buf_ptr,
    int              maxSize,
    OSAL_NetAddress *recvAddr)
{
    int             retVal;

    retVal = maxSize;
    if (OSAL_FAIL == VIDEO_NET_SOCKET_RECEIVE_FROM(
            &socketFd, buf_ptr, &retVal, recvAddr)) {
        return (0);
    }

    if (_VC_netLoopbackCount > 0) {
        int ret = retVal;
        DBG("Recvd %d bytes, sending back...\n", ret);
        if (OSAL_SUCCESS == VIDEO_NET_SOCKET_SEND_TO(
                &socketFd, buf_ptr, &ret, recvAddr)) {
            DBG("Sent %d bytes back...\n", ret);
        }
        _VC_netLoopbackCount--;
    }

    return (retVal);
}

/*
 * ======== _VC_netSendto() ========
 */
uvint _VC_netSendto(
    OSAL_NetSockId   socketFd,
    void            *buf_ptr,
    int              bufSize,
    OSAL_NetAddress  sendAddr)
{
    int             retval;

    retval = bufSize;
    if (_VC_netLoopbackCount > 0) {
        return (retval);
    }
    else {
        if (OSAL_SUCCESS == VIDEO_NET_SOCKET_SEND_TO(
                &socketFd, buf_ptr, &retval, &sendAddr)) {
            return (retval);
        }
        OSAL_logMsg("_VC_netSendto failed");
        return (0);
    }
}

/*
 * ======== _VC_netIsSameAddr() ========
 *
 * Compare two OSAL_NetAddress to see if they
 * are same or not.
 */
OSAL_INLINE OSAL_Boolean _VC_netIsSameAddr(
    OSAL_NetAddress addr1,
    OSAL_NetAddress addr2)
{
    /*
     * Check type socket type
     */
    if (addr1.type != addr2.type) {
        return (OSAL_FALSE);
    }

    if (addr1.type > OSAL_NET_SOCK_TCP) { /* IPV6 */
        if (0 == OSAL_memCmp(addr1.ipv6, addr2.ipv6, 16)) {
            return (OSAL_TRUE);
        }
        else {
            return (OSAL_FALSE);
        }
    }
    else {
        return (addr1.ipv4 == addr2.ipv4);
    }
}
