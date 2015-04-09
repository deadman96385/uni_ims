/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL AND
 * PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2005-2010 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 28481 $ $Date: 2014-08-26 14:54:55 +0800 (Tue, 26 Aug 2014) $
 *
 */
#include "vtspr.h"
#include <osal_net.h>
#include "voice_net.h"

/*
 * ======== _VTSPR_netSocket() ========
 *
 * Open socket, set any necessary socket flags
 *
 */
OSAL_INLINE OSAL_NetSockId _VTSPR_netSocket(
    OSAL_NetSockType type,
    int              flags)
{
    OSAL_NetSockId socketFd;

    /*
     * Create and reserve a socket descriptor. It will be bound when the
     * network is configured.
     */
    if (OSAL_FAIL == VOICE_NET_SOCKET(&socketFd, type)) {
        _VTSP_TRACE(__FILE__, __LINE__);
        return (socketFd);
    }
    if (OSAL_FAIL == VOICE_NET_SET_SOCKET_OPTIONS(&socketFd,
            OSAL_NET_SOCK_NONBLOCKING, 1)) {
        _VTSP_TRACE(__FILE__, __LINE__);
        return (_VTSPR_RTP_ERROR);
    }
    if (OSAL_FAIL == VOICE_NET_SET_SOCKET_OPTIONS(&socketFd,
            OSAL_NET_SOCK_REUSE, 1)) {
        _VTSP_TRACE(__FILE__, __LINE__);
        return (_VTSPR_RTP_ERROR);
    }
    if (OSAL_FAIL == VOICE_NET_SET_SOCKET_OPTIONS(&socketFd,
            OSAL_NET_IP_TOS, flags)) {
        _VTSP_TRACE(__FILE__, __LINE__);
        return (_VTSPR_RTP_ERROR);
    }

    return (socketFd);
}

/*
 * ======== _VTSPR_netClose() ========
 *
 * Close socket
 *
 */
OSAL_INLINE vint _VTSPR_netClose(
    OSAL_NetSockId socketFd)
{
    if (OSAL_SUCCESS != VOICE_NET_CLOSE_SOCKET(&socketFd)) { 
        _VTSP_TRACE(__FILE__, __LINE__);
        return (_VTSPR_RTP_ERROR);
    }

    return (_VTSPR_RTP_OK);
}

/*
 * ======== _VTSPR_netIsSocketIdValid() ========
 *
 * This function returns OSAL_SUCCESS if passed in socket ID is valid.
 *
 * Returns:
 *  OSAL_SUCCESS: Socket ID is valid.
 *  OSAL_FAIL:    Socket ID in invalid.
 */
OSAL_INLINE OSAL_Status _VTSPR_netIsSocketIdValid(
    OSAL_NetSockId  socketFd)
{
    if (OSAL_SUCCESS != VOICE_NET_IS_SOCKETID_VALID(&socketFd)) {
        _VTSP_TRACE(__FILE__, __LINE__);
        return (OSAL_FAIL);
    }
    return (OSAL_SUCCESS);
}

/*
 * ======== _VTSPR_netBind() ========
 *
 * Bind socket, set any necessary socket flags
 *
 */
OSAL_INLINE vint _VTSPR_netBind(
    OSAL_NetSockId  socketFd,
    OSAL_NetAddress address)
{
    if (OSAL_FAIL == VOICE_NET_BIND_SOCKET(&socketFd, &address)) {
        _VTSP_TRACE(__FILE__, __LINE__);
        return (_VTSPR_RTP_ERROR);
    }
    return (_VTSPR_RTP_OK);
}

/*
 * ======== _VTSPR_netRecvfrom() ========
 */
int _VTSPR_netRecvfrom(
    OSAL_NetSockId   socketFd,
    void            *buf_ptr,
    int              maxSize,
    OSAL_NetAddress *recvAddr)
{
    int             retVal;

    retVal = maxSize;
    if (OSAL_FAIL == VOICE_NET_SOCKET_RECEIVE_FROM(
            &socketFd, buf_ptr, &retVal, recvAddr)) {
        retVal = 0;
    }

    return (retVal);
}

/*
 * ======== _VTSPR_netSendto() ========
 */
uvint _VTSPR_netSendto(
    OSAL_NetSockId   socketFd,
    void            *buf_ptr,
    int              bufSize,
    OSAL_NetAddress  sendAddr)
{
    int             retval;

    retval = bufSize;
    if (OSAL_SUCCESS == VOICE_NET_SOCKET_SEND_TO(
            &socketFd, buf_ptr, &retval, &sendAddr)) {
        return (retval);
    }
    return (0);
}

/*
 * ======== _VTSPR_netIsSameAddr() ========
 *
 * Compare two OSAL_NetAddress to see if they
 * are same or not.
 */
OSAL_INLINE OSAL_Boolean _VTSPR_netIsSameAddr(
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
        if (addr1.ipv4 == addr2.ipv4) {
            return (OSAL_TRUE);
        }
        else {
            return (OSAL_FALSE);
        }
    }
}

