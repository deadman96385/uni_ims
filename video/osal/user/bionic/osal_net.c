/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Revision: 1381 $ $Date: 2006-12-05 14:44:53 -0800 (Tue, 05 Dec 2006) $
 */

#include <osal_net.h>
#include <osal_select.h>
#include <osal_mem.h>
#include <osal_string.h>
#include <osal_log.h>
#include <osal_task.h>

#ifdef OSAL_NET_ENABLE_DNS
# ifndef OSAL_RESOLVE_A_ONLY
/* 
 * ======== _OSAL_netGetTid() ========
 *
 * This function is used to get a task ID for DNS query.
 *
 * Returns:
 *  16-bit task ID from system clock.
 */   
static uint16 _OSAL_netGetTid(
    void)
{
    struct timeval time;

    gettimeofday(&time, NULL);

    return ((uint16)time.tv_usec);
}

/* 
 * ======== _OSAL_netPrepareDnsQuery() ========
 *
 * This function is used to get a task ID for DNS query.
 *
 * Returns:
 *  DNS packet size.
 */   
static vint _OSAL_netPrepareDnsQuery(
    uint16  type,
    int8   *query_ptr,
    int8   *buf_ptr,
    vint   *size_ptr)
{
    int   len;
    int   qlen;
    int8 *part_ptr;

    /*
     * Prepare query buffer
     */
    len = 0;
    buf_ptr[len++] = _OSAL_netGetTid(); /* TID */
    buf_ptr[len++] = _OSAL_netGetTid(); /* TID */
    buf_ptr[len++] = 0x01;    /* FLAGS, recursion required */
    buf_ptr[len++] = 0x00;    /* FLAGS */
    buf_ptr[len++] = 0x00;    /* QDCOUNT */
    buf_ptr[len++] = 0x01;    /* QDCOUNT, 1 only */
    buf_ptr[len++] = 0x00;    /* ANCOUNT */ 
    buf_ptr[len++] = 0x00;    /* ANCOUNT */
    buf_ptr[len++] = 0x00;    /* NSCOUNT */
    buf_ptr[len++] = 0x00;    /* NSCOUNT */
    buf_ptr[len++] = 0x00;    /* ARCOUNT */
    buf_ptr[len++] = 0x00;    /* ARCOUNT */

    /*
     * Encode name to be resolved. Use no pointers in DNS name (not anyway in
     * single name encoding).
     */
    qlen = len;
    while (NULL != (part_ptr = _OSAL_netSplitDot(query_ptr))) {
        buf_ptr[len++] = part_ptr - query_ptr;
        memcpy(&buf_ptr[len], query_ptr, buf_ptr[len - 1]);
        len += part_ptr - query_ptr;
        query_ptr = part_ptr + 1;
    }
    buf_ptr[len++] = strlen(query_ptr);
    if (buf_ptr[len - 1] != 0) {
        memcpy(&buf_ptr[len], query_ptr, buf_ptr[len - 1]);
        len += buf_ptr[len - 1];
        buf_ptr[len++] = 0x00;  /* EOS */
    }

    buf_ptr[len++] = type >> 8;    /* TYPE */
    buf_ptr[len++] = type & 0xFF;  /* TYPE SRV = 0x21 , A = 1 */
    buf_ptr[len++] = 0x00;         /* CLASS */
    buf_ptr[len++] = 0x01;         /* CLASS INET */
    buf_ptr[len]   = 0;

    *size_ptr = len - qlen;


    return (len);
}
#endif
#endif

/* 
 * ======== OSAL_netSocket() ========
 *
 * This function is used to create a socket of a given type.
 *
 * Returns:
 *  OSAL_SUCCESS: Success.
 *  OSAL_FAIL:    Failure with proper OSAL error code set.
 */   
OSAL_Status OSAL_netSocket(
    OSAL_NetSockId   *socket_ptr,
    OSAL_NetSockType  type)
{
    int retval;

    if (socket_ptr == NULL) {
        return (OSAL_FAIL);
    }

    switch (type) {
        case OSAL_NET_SOCK_TCP:
            retval = socket(AF_INET, SOCK_STREAM, 0);
            break;
        case OSAL_NET_SOCK_UDP:
            retval = socket(AF_INET, SOCK_DGRAM, 0);
            break;
        case OSAL_NET_SOCK_TCP_V6:
            retval = socket(AF_INET6, SOCK_STREAM, 0);
            break;
        case OSAL_NET_SOCK_UDP_V6:
            retval = socket(AF_INET6, SOCK_DGRAM, 0);
            break;
#ifdef OSAL_IPSEC_ENABLE
        case OSAL_NET_SOCK_IPSEC:
            retval = socket(PF_KEY, SOCK_RAW, PF_KEY_V2);
            break;
#endif
        default:
            return (OSAL_FAIL);
    }

    if (retval >= 0) {
        *socket_ptr = retval;
        return(OSAL_SUCCESS);
    }

    return (OSAL_FAIL);
}

/* 
 * ======== OSAL_netCloseSocket() ========
 *
 * This function is used to close a socket.
 *
 * Returns:
 *  OSAL_SUCCESS: Success.
 *  OSAL_FAIL:    Failure with proper OSAL error code set.
 */
OSAL_Status OSAL_netCloseSocket(
    OSAL_NetSockId *socket_ptr)
{
    if (socket_ptr == NULL) {
        return (OSAL_FAIL);
    }

    if (0 == close(*socket_ptr)) {
        *socket_ptr = -1;
        return (OSAL_SUCCESS);
    }

    return (OSAL_FAIL);
}

/*
 * ======== OSAL_netIsSocketIdValid() ========
 *
 * This function returns OSAL_SUCCESS if passed in socket ID is valid.
 *
 * Returns:
 *  OSAL_SUCCESS: Socket ID is valid.
 *  OSAL_FAIL:    Socket ID in invalid.
 */ 
OSAL_Status OSAL_netIsSocketIdValid(
    OSAL_NetSockId *socket_ptr)
{
    if (socket_ptr == NULL) {
        return (OSAL_FAIL);
    }

    if (*socket_ptr >= 0) {
        return (OSAL_SUCCESS);
    }

    return (OSAL_FAIL);
}

/* 
 * ======== OSAL_netConnectSocket() ========
 *
 * This function is used to connect to a peer socket.
 *
 * Returns:
 *  OSAL_SUCCESS: Success.
 *  OSAL_FAIL:    Failure with proper OSAL error code set.
 */
OSAL_Status OSAL_netConnectSocket(
    OSAL_NetSockId  *socket_ptr,
    OSAL_NetAddress *address_ptr)
{
    struct sockaddr_in serverAddr;
    struct sockaddr_in6  serverAddrV6;
    struct sockaddr     *addr_ptr;
    int                  size;

    if ((socket_ptr == NULL) || (address_ptr == NULL)) {
        return (OSAL_FAIL);
    }

    if ((OSAL_NET_SOCK_TCP_V6 == address_ptr->type) ||
            (OSAL_NET_SOCK_UDP_V6 == address_ptr->type)) {
        OSAL_memSet(&serverAddrV6, 0, sizeof(serverAddrV6));
        serverAddrV6.sin6_family = AF_INET6;
        serverAddrV6.sin6_port   = address_ptr->port;
        OSAL_memCpy(&serverAddrV6.sin6_addr, address_ptr->ipv6,
                sizeof(address_ptr->ipv6));
        addr_ptr = (struct sockaddr *)&serverAddrV6;
        size = sizeof(serverAddrV6);
    }
    else {
        memset(&serverAddr, 0, sizeof(serverAddr));
        serverAddr.sin_family      = AF_INET;
        serverAddr.sin_port        = address_ptr->port;
        serverAddr.sin_addr.s_addr = address_ptr->ipv4;
        addr_ptr = (struct sockaddr *)&serverAddr;
        size = sizeof(serverAddr);
    }
    if (0 == connect(*socket_ptr, addr_ptr, size)) {
        return(OSAL_SUCCESS);
    }

    return (OSAL_FAIL);
}

/* 
 * ======== OSAL_netBindSocket() ========
 *
 * This function is used to bind a socket to address/port pair.
 *
 * Returns:
 *  OSAL_SUCCESS: Success.
 *  OSAL_FAIL:    Failure with proper OSAL error code set.
 */   
OSAL_Status OSAL_netBindSocket(
    OSAL_NetSockId  *socket_ptr,
    OSAL_NetAddress *address_ptr)
{
    struct sockaddr_in bindAddr;
    struct sockaddr_in6  bindAddrV6;
    struct sockaddr     *addr_ptr;
    int                  size;

    if ((socket_ptr == NULL) || (address_ptr == NULL)) {
        return (OSAL_FAIL);
    }

    if ((OSAL_NET_SOCK_TCP_V6 == address_ptr->type) ||
            (OSAL_NET_SOCK_UDP_V6 == address_ptr->type)) {
        OSAL_memSet(&bindAddrV6, 0, sizeof(bindAddrV6));
        bindAddrV6.sin6_family = AF_INET6;
        bindAddrV6.sin6_port   = address_ptr->port;
        OSAL_memCpy(&bindAddrV6.sin6_addr, address_ptr->ipv6,
                sizeof(address_ptr->ipv6));
        addr_ptr = (struct sockaddr *)&bindAddrV6;
        size = sizeof(bindAddrV6);
    }
    else {
        memset(&bindAddr, 0, sizeof(bindAddr));
        bindAddr.sin_family      = AF_INET;
        bindAddr.sin_port        = address_ptr->port;
        bindAddr.sin_addr.s_addr = address_ptr->ipv4;
        addr_ptr = (struct sockaddr *)&bindAddr;
        size = sizeof(bindAddr);
    }

    if (0 == bind(*socket_ptr, addr_ptr, size)) {
        return(OSAL_SUCCESS);
    }

    return (OSAL_FAIL);
}

/*
 * ======== OSAL_netGetSocketAddress() ========
 *
 * This function is used to get address/port pair on which a socket is bound.
 *
 * Returns:
 *  OSAL_SUCCESS: Success.
 *  OSAL_FAIL:    Failure with proper OSAL error code set.
 */   
OSAL_Status OSAL_netGetSocketAddress(
    OSAL_NetSockId  *socket_ptr,
    OSAL_NetAddress *address_ptr)
{
    struct sockaddr_storage sockStor;
    struct sockaddr_in   *addr_ptr;
    struct sockaddr_in6  *addrV6_ptr;
    socklen_t             len;

    if ((socket_ptr == NULL) || (address_ptr == NULL)) {
        return (OSAL_FAIL);
    }

    len = sizeof(sockStor);
    if (0 != getsockname(*socket_ptr, (struct sockaddr *)&sockStor, &len)) {
        return (OSAL_FAIL);
    }

    OSAL_netGetSocketOptions(socket_ptr, OSAL_NET_SOCK_TYPE, (int *)&address_ptr->type);
    if (AF_INET == sockStor.ss_family) {
        addr_ptr = ((struct sockaddr_in *)&sockStor);
        address_ptr->ipv4 = addr_ptr->sin_addr.s_addr;
        address_ptr->port = addr_ptr->sin_port;
    }
    else if (AF_INET6 == sockStor.ss_family) {
        addrV6_ptr = ((struct sockaddr_in6 *)&sockStor);
        OSAL_memCpy(address_ptr->ipv6, &addrV6_ptr->sin6_addr,
                sizeof(address_ptr->ipv6));
        address_ptr->port = addrV6_ptr->sin6_port;
    }

    return(OSAL_SUCCESS);
}

/*
 * ======== OSAL_netListenOnSocket() ========
 *
 * This function is used to listen on a socket for peer connections.
 *
 * Returns:
 *  OSAL_SUCCESS: Success.
 *  OSAL_FAIL:    Failure with proper OSAL error code set.
 */   
OSAL_Status OSAL_netListenOnSocket(
    OSAL_NetSockId *socket_ptr)
{
    if (socket_ptr == NULL) {
        return (OSAL_FAIL);
    }

    if (0 == listen(*socket_ptr, 1)) {
        return (OSAL_SUCCESS);
    }

    return (OSAL_FAIL);
}

/*
 * ======== OSAL_netAcceptOnSocket() ========
 *
 * This function is used to accept a peer connection on a socket.
 *
 * Returns:
 *  OSAL_SUCCESS: Success.
 *  OSAL_FAIL:    Failure with proper OSAL error code set.
 */   
OSAL_Status OSAL_netAcceptOnSocket(
    OSAL_NetSockId  *socket_ptr,
    OSAL_NetSockId  *newSocket_ptr,
    OSAL_NetAddress *address_ptr)
{
    struct sockaddr_in clientAddr;
    struct sockaddr_in6  clientAddrV6;
    struct sockaddr     *addr_ptr;
    socklen_t          len;
    int                retval;

    if ((socket_ptr == NULL) || (address_ptr == NULL) || 
            (address_ptr == NULL)) {
        return (OSAL_FAIL);
    }

    if ((OSAL_NET_SOCK_TCP_V6 == address_ptr->type) ||
            (OSAL_NET_SOCK_UDP_V6 == address_ptr->type)) {
        len = sizeof(clientAddrV6);
        OSAL_memSet(&clientAddrV6, 0, len);
        addr_ptr = (struct sockaddr *)&clientAddrV6;
    }
    else {
    len = sizeof(clientAddr);
        OSAL_memSet(&clientAddr, 0, len);
        addr_ptr = (struct sockaddr *)&clientAddr;
    }
    retval = accept(*socket_ptr, addr_ptr, &len);
    if (retval >= 0) {
        if ((OSAL_NET_SOCK_TCP_V6 == address_ptr->type) ||
                (OSAL_NET_SOCK_UDP_V6 == address_ptr->type)) {
            OSAL_memCpy(address_ptr->ipv6, &clientAddrV6.sin6_addr,
                    sizeof(address_ptr->ipv6));
            address_ptr->port = clientAddrV6.sin6_port;
        }
        else {
            address_ptr->ipv4 = clientAddr.sin_addr.s_addr;
            address_ptr->port = clientAddr.sin_port;
        }
        *newSocket_ptr = retval;
        return(OSAL_SUCCESS);
    }

    return (OSAL_FAIL);
}

/*
 * ======== OSAL_netSocketReceive() ========
 *
 * This function is used to receive data on a socket.
 *
 * Returns:
 *  OSAL_SUCCESS: Success.
 *  OSAL_FAIL:    Failure with proper OSAL error code set.
 */   
OSAL_Status OSAL_netSocketReceive(
    OSAL_NetSockId      *socket_ptr,
    void                *buf_ptr,
    vint                *size_ptr,
    OSAL_NetReceiveFlags flags)
{
    ssize_t retval;
    vint flgs = 0;

    if (0 != (flags & OSAL_NET_RECV_PEEK)) {
        flgs |= MSG_PEEK;
    }

    retval = recv(*socket_ptr, buf_ptr, *size_ptr, flgs);

    if (retval >= 0) {
        *size_ptr = retval;
        return(OSAL_SUCCESS);
    }

    return (OSAL_FAIL);
}

/*
 * ======== OSAL_netSocketReceiveFrom() ========
 *
 * This function is used to receive data on a socket and to return
 * sender's address.
 *
 * Returns:
 *  OSAL_SUCCESS: Success.
 *  OSAL_FAIL:    Failure with proper OSAL error code set.
 */   
OSAL_Status OSAL_netSocketReceiveFrom(
    OSAL_NetSockId  *socket_ptr,
    void            *buf_ptr,
    vint            *size_ptr,
    OSAL_NetAddress *address_ptr)
{
    struct sockaddr_storage sockStor;
    struct sockaddr_in   *fromAddr_ptr;
    struct sockaddr_in6  *fromAddrV6_ptr;
    socklen_t            len;
    ssize_t              retval;

    len = sizeof(sockStor);
    retval = recvfrom(*socket_ptr, buf_ptr, *size_ptr, 0,
            (struct sockaddr *)&sockStor, &len);

    if (retval >= 0) {
        *size_ptr = retval;
        if (AF_INET == sockStor.ss_family) {
            fromAddr_ptr = ((struct sockaddr_in *)&sockStor);
            address_ptr->type = OSAL_NET_SOCK_UDP;
            address_ptr->ipv4 = fromAddr_ptr->sin_addr.s_addr;
            address_ptr->port = fromAddr_ptr->sin_port;
        }
        else if (AF_INET6 == sockStor.ss_family) {
            fromAddrV6_ptr = ((struct sockaddr_in6 *)&sockStor);
            address_ptr->type = OSAL_NET_SOCK_UDP_V6;
            OSAL_memCpy(address_ptr->ipv6, &fromAddrV6_ptr->sin6_addr,
                    sizeof(address_ptr->ipv6));
            address_ptr->port = fromAddrV6_ptr->sin6_port;
        }
        else {
            /* Neither AF_INET nor AF_INET6. Should not go to here */
            return (OSAL_FAIL);
        }
        return(OSAL_SUCCESS);
    }

    return (OSAL_FAIL);
}

/*
 * ======== OSAL_netSocketSend() ========
 *
 * This function is used to send data on a socket.
 *
 * Returns:
 *  OSAL_SUCCESS: Success.
 *  OSAL_FAIL:    Failure with proper OSAL error code set.
 */   
OSAL_Status OSAL_netSocketSend(
    OSAL_NetSockId  *socket_ptr,
    void            *buf_ptr,
    vint            *size_ptr)
{
    ssize_t retval;

    retval = send(*socket_ptr, buf_ptr, *size_ptr, 0);

    if (retval >= 0) {
        *size_ptr = retval;
        return(OSAL_SUCCESS);
    }

    return (OSAL_FAIL);
}

/*
 * ======== OSAL_netSocketSendTo() ========
 *
 * This function is used to send data on a socket to a specified recipient.
 *
 * Returns:
 *  OSAL_SUCCESS: Success.
 *  OSAL_FAIL:    Failure with proper OSAL error code set.
 */
OSAL_Status OSAL_netSocketSendTo(
    OSAL_NetSockId  *socket_ptr,
    void            *buf_ptr,
    vint            *size_ptr,
    OSAL_NetAddress *address_ptr)
{
    struct sockaddr_in   toAddr;
    struct sockaddr_in6  toAddrV6;
    struct sockaddr     *addr_ptr;
    socklen_t            len;
    ssize_t              retval;

    if ((OSAL_NET_SOCK_TCP_V6 == address_ptr->type) ||
            (OSAL_NET_SOCK_UDP_V6 == address_ptr->type)) {
        len = sizeof(toAddrV6);
        OSAL_memSet(&toAddrV6, 0, len);
        toAddrV6.sin6_family = AF_INET6;
        toAddrV6.sin6_port = address_ptr->port;
        OSAL_memCpy(&toAddrV6.sin6_addr, address_ptr->ipv6, 
                sizeof(address_ptr->ipv6));
        addr_ptr = (struct sockaddr *)&toAddrV6;
    }
    else {
        len = sizeof(toAddr);
        OSAL_memSet(&toAddr, 0, len);
        toAddr.sin_family = AF_INET;
        toAddr.sin_port = address_ptr->port;
        toAddr.sin_addr.s_addr = address_ptr->ipv4;
        addr_ptr = (struct sockaddr *)&toAddr;
    }

    retval = sendto(*socket_ptr, buf_ptr, *size_ptr, 0, addr_ptr, len);

    if (retval >= 0) {
        *size_ptr = retval;
        return(OSAL_SUCCESS);
    }
    
    return (OSAL_FAIL);
}

/*
 * ======== OSAL_netSetSocketOptions() ========
 *
 * This function is used to set options of a socket.
 *
 * Returns:
 *  OSAL_SUCCESS: Success.
 *  OSAL_FAIL:    Failure with proper OSAL error code set.
 */   
OSAL_Status OSAL_netSetSocketOptions(
    OSAL_NetSockId  *socket_ptr,
    OSAL_NetSockopt  option,
    int              value)
{
    int retval;
    int optvalue;
    struct timeval tv;

    if (socket_ptr == NULL) {
        return (OSAL_FAIL);
    }

    switch (option) {
        case OSAL_NET_SOCK_REUSE:
            if (1 == value) {
                optvalue = 1;
            }
            else {
                optvalue = 0;
            }
            retval = setsockopt(*socket_ptr, SOL_SOCKET, SO_REUSEADDR,
                    &optvalue, sizeof(optvalue));
            break;
        case OSAL_NET_SOCK_NONBLOCKING:
            retval = fcntl(*socket_ptr, F_GETFL);
            if (-1 == retval) {
                break;
            }
            if (1 == value) {
                retval |= O_NONBLOCK;
            }
            else {
                retval &= ~O_NONBLOCK;
            }
            retval = fcntl(*socket_ptr, F_SETFL, retval);
            break;
        case OSAL_NET_IP_TOS:
            optvalue = value;
            retval = setsockopt(*socket_ptr, SOL_IP, IP_TOS,
                    (char *)&optvalue, sizeof(optvalue));
            break;
        case OSAL_NET_SOCK_RCVTIMEO_SECS:
            tv.tv_sec = value;
            tv.tv_usec = 0;
            retval = setsockopt(*socket_ptr, SOL_SOCKET,
                    SO_RCVTIMEO, &tv, sizeof(tv));
            break;
        default:
            return (OSAL_FAIL);
    }

    if (0 == retval) {
        return (OSAL_SUCCESS);
    }

    return (OSAL_FAIL);
}

/*
 * ======== OSAL_netGetSocketOptions() ========
 *
 * This function is used to get options of a socket.
 *
 * Returns:
 *  OSAL_SUCCESS: Success.
 *  OSAL_FAIL:    Failure with proper OSAL error code set.
 */
OSAL_Status OSAL_netGetSocketOptions(
    OSAL_NetSockId  *socket_ptr,
    OSAL_NetSockopt  option,
    int    *value_ptr)
{
    int       retval;
    socklen_t len;
    struct sockaddr_storage sockStor;
    int       type;       

    if ((socket_ptr == NULL) || (value_ptr == NULL)) {
        return (OSAL_FAIL);
    }

    switch (option) {
        case OSAL_NET_SOCK_REUSE:
            len = sizeof(int);
            retval = getsockopt(*socket_ptr, SOL_SOCKET, SO_REUSEADDR, 
                    value_ptr, &len);
            break;
        case OSAL_NET_SOCK_NONBLOCKING:
            retval = fcntl(*socket_ptr, F_GETFL);
            if (-1 == retval) {
                break;
            }
            if (retval & O_NONBLOCK) {
                *value_ptr = OSAL_TRUE;
            }
            else {
                *value_ptr = OSAL_FALSE;
            }
            break;
         case OSAL_NET_IP_TOS:
            len = sizeof(int);
            retval = getsockopt(*socket_ptr, SOL_IP, IP_TOS, 
                    value_ptr, &len);
            break;
         case OSAL_NET_SOCK_TYPE:
            len = sizeof(int);
            retval = getsockopt(*socket_ptr, SOL_SOCKET, SO_TYPE, 
                    &type, &len);

            len = sizeof(sockStor);
            if (0 != getsockname(*socket_ptr, (struct sockaddr *)&sockStor, &len)) {
                return (OSAL_FAIL);
            }

            if (AF_INET == sockStor.ss_family) {
                if (SOCK_DGRAM == type) {
                    *value_ptr = OSAL_NET_SOCK_UDP;
                }
                else if (SOCK_STREAM == type) {
                    *value_ptr = OSAL_NET_SOCK_TCP;
                }
                else if (SOCK_RAW == type) {
                    *value_ptr = OSAL_NET_SOCK_IPSEC;
                }
                else {
                    return (OSAL_FAIL);
                }
            }
            else if (AF_INET6 == sockStor.ss_family) {
                if (SOCK_DGRAM == type) {
                    *value_ptr = OSAL_NET_SOCK_UDP_V6;
                }
                else if (SOCK_STREAM == type) {
                    *value_ptr = OSAL_NET_SOCK_TCP_V6;
                }
                else if (SOCK_RAW == type) {
                    *value_ptr = OSAL_NET_SOCK_IPSEC;
                }
                else {
                    return (OSAL_FAIL);
                }
            }
            break;            
        default:
            return (OSAL_FAIL);
    }

    if (0 == retval) {
        return (OSAL_SUCCESS);
    }

    return (OSAL_FAIL);
}

/*
 * ======== OSAL_netStringToAddress() ========
 *
 * This function is used to convert a ipv4 address string to 32-bit address
 * or convert a ipv6 address string to 128-bit address.
 *
 * Returns:
 *  OSAL_SUCCESS: Success.
 *  OSAL_FAIL:    Failure with proper OSAL error code set.
 */ 
OSAL_Status OSAL_netStringToAddress(
    int8            *str_ptr,
    OSAL_NetAddress *address_ptr)
{
    /* Try to convert to ipv4 address */
    if (1 == inet_pton(AF_INET, (const char*)str_ptr, &address_ptr->ipv4)) {
        address_ptr->type = OSAL_NET_SOCK_UDP;
        return (OSAL_SUCCESS);
    }

    /* Not an ipv4 address, try ipv6 */
    if (1 == inet_pton(AF_INET6, (const char*)str_ptr, address_ptr->ipv6)) {
        address_ptr->type = OSAL_NET_SOCK_UDP_V6;
        return (OSAL_SUCCESS);
    }

    return (OSAL_FAIL);
}

/*
 * ======== OSAL_netAddressToString() ========
 *
 * This function is used to convert a 32-bit ipv4 address
 * or 128-bit ipv6 address to string.
 * notation string.
 *
 * Returns:
 *  OSAL_SUCCESS: Success.
 *  OSAL_FAIL:    Failure with proper OSAL error code set.
 */ 
OSAL_Status OSAL_netAddressToString(
    int8            *str_ptr,
    OSAL_NetAddress *address_ptr)
{
    if ((OSAL_NET_SOCK_TCP_V6 == address_ptr->type) ||
            OSAL_NET_SOCK_UDP_V6 == address_ptr->type) {
        /* ipv6 */
        inet_ntop(AF_INET6, address_ptr->ipv6, (char*)str_ptr, OSAL_NET_IPV6_STR_MAX);
    }
    else {
        /* ipv4 */
        inet_ntop(AF_INET, &address_ptr->ipv4, (char*)str_ptr, OSAL_NET_IPV4_STR_MAX);
    }

    return(OSAL_SUCCESS);
}

/*
 * ======== OSAL_netSslSetCert() ========
 *
 * This function is used to set certificate to SSL
 *
 * Returns:
 *  OSAL_SUCCESS: Success.
 *  OSAL_FAIL:    Failure with proper OSAL error code set.
 */
OSAL_Status OSAL_netSslSetCert(
    OSAL_NetSslId   *ssl_ptr,
    OSAL_NetSslCert *cert_ptr)
{
#ifdef OSAL_NET_ENABLE_SSL
    ssl_ptr->cert_ptr = cert_ptr;

    return (OSAL_SUCCESS);
#else
    return (OSAL_FAIL);
#endif
}

/*
 * ======== OSAL_netSslSetCertVerifyCB() ========
 *
 * This function is used to set callback function for remote certificate
 * validation. User could implemet their validation function to validate
 * remote certificate.
 * If the callback function is not assiged, it won't validate remote
 * certificate.
 *
 * Returns:
 *  OSAL_SUCCESS: Success.
 *  OSAL_FAIL:    Failure with proper OSAL error code set.
 */ 
OSAL_Status OSAL_netSslSetCertVerifyCB(
    OSAL_NetSslId  *ssl_ptr,
    void           *certValidateCB)
{
#ifdef OSAL_NET_ENABLE_SSL
    ssl_ptr->certValidateCB = certValidateCB;
    return (OSAL_SUCCESS);
#else
    return (OSAL_FAIL);
#endif
    
}

/*
 * ======== OSAL_netSslSetCertVerify() ========
 *
 * This function is called in OSAL_netSsl() to set callback function
 * for remote certificate validation.
 * Also see OSAL_netSslSetCertVerifyCB().
 *
 * Returns:
 *  OSAL_SUCCESS: Success.
 *  OSAL_FAIL:    Failure with proper OSAL error code set.
 */ 
OSAL_Status OSAL_netSslSetCertVerify(
    OSAL_NetSslId  *ssl_ptr)
{
#ifdef OSAL_NET_ENABLE_SSL
    if (NULL != ssl_ptr->certValidateCB) {
        SSL_CTX_set_verify(ssl_ptr->ctx_ptr, SSL_VERIFY_PEER,
                ssl_ptr->certValidateCB);

        /* Set the verification depth to 1 */
        SSL_CTX_set_verify_depth(ssl_ptr->ctx_ptr, 1);
    }

    return (OSAL_SUCCESS);
#else
    return (OSAL_FAIL);
#endif
}

/*
 * ======== OSAL_netSslInit() ========
 *
 * This function is used to init SSL library.
 *
 * Returns:
 *  OSAL_SUCCESS: Success.
 *  OSAL_FAIL:    Failure with proper OSAL error code set.
 */ 
OSAL_Status OSAL_netSslInit(
    void)
{
#ifdef OSAL_NET_ENABLE_SSL
    SSL_load_error_strings();
    SSL_library_init();
    return (OSAL_SUCCESS);
#else
    return (OSAL_FAIL);
#endif
}

/*
 * ======== OSAL_netSsl() ========
 *
 * This function is used to get an SSL connection object.
 *
 * Returns:
 *  OSAL_SUCCESS: Success.
 *  OSAL_FAIL:    Failure.
 */ 
OSAL_Status OSAL_netSsl(
    OSAL_NetSslId     *ssl_ptr,
    OSAL_NetSslMethod  method)
{
#ifdef OSAL_NET_ENABLE_SSL
    OSAL_NetSslCert *cert_ptr;

    switch (method) {
        case OSAL_NET_SSL_METHOD_SERVER_SSLV3:
            if (NULL == (ssl_ptr->ctx_ptr = 
                    SSL_CTX_new(SSLv3_method()))) {
                return (OSAL_FAIL);
            }
            break;
        case OSAL_NET_SSL_METHOD_SERVER_TLSV1:
            if (NULL == (ssl_ptr->ctx_ptr = 
                    SSL_CTX_new(TLSv1_method()))) {
                return (OSAL_FAIL);
            }
            break;
        case OSAL_NET_SSL_METHOD_SERVER_ALL:
            /* Supports SSL Version 2 & 3 and TLS version 1 */
            if (NULL == (ssl_ptr->ctx_ptr = 
                    SSL_CTX_new(SSLv23_method()))) {
                return (OSAL_FAIL);
            }
            break;
        case OSAL_NET_SSL_METHOD_CLIENT_SSLV3:
            if (NULL == (ssl_ptr->ctx_ptr = 
                    SSL_CTX_new(SSLv3_client_method()))) {
                return (OSAL_FAIL);
            }
            break;
        case OSAL_NET_SSL_METHOD_CLIENT_TLSV1:
            if (NULL == (ssl_ptr->ctx_ptr = 
                    SSL_CTX_new(TLSv1_client_method()))) {
                return (OSAL_FAIL);
            }
            break;
        case OSAL_NET_SSL_METHOD_CLIENT_ALL:
            /* Supports SSL Version 2 & 3 and TLS version 1 */
            if (NULL == (ssl_ptr->ctx_ptr = 
                    SSL_CTX_new(SSLv23_client_method()))) {
                return (OSAL_FAIL);
            }
            break;
        default:
            return (OSAL_FAIL);
    }

    OSAL_netSslSetCertVerify(ssl_ptr);

    /* Set certificate if exsits */
    cert_ptr = ssl_ptr->cert_ptr;
    if (NULL != cert_ptr) {
        if (0 >= SSL_CTX_use_certificate(ssl_ptr->ctx_ptr, cert_ptr->x509_ptr)) {
            return (OSAL_FAIL);
        }

        if (0 >= SSL_CTX_use_PrivateKey(ssl_ptr->ctx_ptr, cert_ptr->pKey_ptr)) {
            return (OSAL_FAIL);
        }

        if (!SSL_CTX_check_private_key(ssl_ptr->ctx_ptr)) {
            return (OSAL_FAIL);
        }
    }

    if (NULL == (ssl_ptr->ssl_ptr = SSL_new(ssl_ptr->ctx_ptr))) {
        SSL_CTX_free(ssl_ptr->ctx_ptr);
        return (OSAL_FAIL);
    }
    return (OSAL_SUCCESS);
#else
    return (OSAL_FAIL);
#endif
}

/*
 * ======== OSAL_netSslCertGen() ========
 *
 * This function is used to generate a certificate
 *
 * Returns:
 *  OSAL_SUCCESS: Success.
 *  OSAL_FAIL:    Failure with proper OSAL error code set.
 */ 
OSAL_Status OSAL_netSslCertGen(
    OSAL_NetSslCert *cert_ptr,
    vint             bits,
    vint             serial,
    vint             days)
{
#ifdef OSAL_NET_ENABLE_SSL
    X509      *x;
    EVP_PKEY  *pk;
    RSA       *rsa;
    X509_NAME *name_ptr;
    BIGNUM*    exponent;

    if (NULL == cert_ptr->pKey_ptr) {
        cert_ptr->pKey_ptr = EVP_PKEY_new();
    }

    if (NULL == cert_ptr->x509_ptr) {
        cert_ptr->x509_ptr = X509_new();
    }

    pk = (cert_ptr->pKey_ptr);
    x = (cert_ptr->x509_ptr);

    rsa = RSA_new();
    exponent = BN_new();
    /* The exponent is an odd number, typically 3, 17 or 65537. */
    BN_set_word(exponent, 0x10001);
    RSA_generate_key_ex(rsa, bits, exponent, NULL);
    BN_free(exponent);
    if (!EVP_PKEY_assign_RSA(pk, rsa)) {
        RSA_free(rsa);
        return (OSAL_FAIL);
    }
    
    X509_set_version(x, 0);
    ASN1_INTEGER_set(X509_get_serialNumber(x), serial);
    X509_gmtime_adj(X509_get_notBefore(x), 0);
    X509_gmtime_adj(X509_get_notAfter(x), (long)(60 * 60 * 24 * days));
    X509_set_pubkey(x, pk);

    name_ptr = X509_get_subject_name(x);

    /*
     * This function creates and adds the entry, working out the
     * correct string type and performing checks on its length.
     * Normally we'd check the return value for errors...
     */
    X509_NAME_add_entry_by_txt(name_ptr,"C",
                MBSTRING_ASC, (unsigned char *)"US", -1, -1, 0);
    X509_NAME_add_entry_by_txt(name_ptr,"CN",
                MBSTRING_ASC, (unsigned char *)"D2 Technologies", -1, -1, 0);

    /*
     * Its self signed so set the issuer name to be the same as the
     * subject.
     */
    X509_set_issuer_name(x, name_ptr);

    /* Sign */
    if (!X509_sign(x, pk, EVP_sha1())) {
        return (OSAL_FAIL);
    }

    /* Verify if the public key matches */
    if (0 >= X509_verify(x, X509_get_pubkey(x))) {
        return (OSAL_FAIL);
    }

    /* Debug print */
    /* X509_print_fp(stdout, x); */

    return (OSAL_SUCCESS);
#else
    return (OSAL_FAIL);
#endif
}

/*
 * ======== OSAL_netSslConnect() ========
 *
 * This function is used to connect to a server over SSL layer.
 *
 * Returns:
 *  OSAL_SUCCESS: Success.
 *  OSAL_FAIL:    Failure with proper OSAL error code set.
 */ 
OSAL_Status OSAL_netSslConnect(
    OSAL_NetSslId *ssl_ptr)
{
#ifdef OSAL_NET_ENABLE_SSL
    int retval;

    retval = SSL_connect(ssl_ptr->ssl_ptr);
    
    if (retval <= 0) {
        return (OSAL_FAIL);
    }
    
    return (OSAL_SUCCESS);
#else
    return (OSAL_FAIL);
#endif
}

/*
 * ======== OSAL_netSslAccept() ========
 *
 * This function is used to accept client's connection over SSL layer.
 *
 * Returns:
 *  OSAL_SUCCESS: Success.
 *  OSAL_FAIL:    Failure with proper OSAL error code set.
 */ 
OSAL_Status OSAL_netSslAccept(
    OSAL_NetSslId *ssl_ptr)
{
#ifdef OSAL_NET_ENABLE_SSL
    int retval;

    retval = SSL_accept(ssl_ptr->ssl_ptr);
    
    if (retval <= 0) {
        return (OSAL_FAIL);
    }
    
    return (OSAL_SUCCESS);
#else
    return (OSAL_FAIL);
#endif
}

/*
 * ======== OSAL_netSslClose() ========
 *
 * This function is used to close an SSL connection and free the connection
 * object.
 *
 * Returns:
 *  OSAL_SUCCESS: Success.
 *  OSAL_FAIL:    Failure with proper OSAL error code set.
 */ 
OSAL_Status OSAL_netSslClose(
    OSAL_NetSslId *ssl_ptr)
{
#ifdef OSAL_NET_ENABLE_SSL
    int retval;

    retval = SSL_shutdown(ssl_ptr->ssl_ptr);
    if (retval <= 0) {
        return (OSAL_FAIL);
    }

    retval = SSL_clear(ssl_ptr->ssl_ptr);
    if (retval <= 0) {
        return (OSAL_FAIL);
    }
    SSL_free(ssl_ptr->ssl_ptr);
    SSL_CTX_free(ssl_ptr->ctx_ptr);
    ssl_ptr->ssl_ptr = NULL;
    ssl_ptr->ctx_ptr = NULL;
    ssl_ptr->cert_ptr = NULL;
    return (OSAL_SUCCESS);
#else
    return (OSAL_FAIL);
#endif
}

/*
 * ======== OSAL_netIsSslIdValid() ========
 *
 * This function returns OSAL_SUCCESS if passed in SSL ID is valid.
 *
 * Returns
 *  OSAL_SUCCESS: SSL ID is valid.
 *  OSAL_FAIL:    SSL ID in invalid.
 */ 
OSAL_Status OSAL_netIsSslIdValid(
    OSAL_NetSslId *ssl_ptr)
{
#ifdef OSAL_NET_ENABLE_SSL
    if ((NULL == ssl_ptr->ssl_ptr) || (NULL == ssl_ptr->ctx_ptr)) {
        return (OSAL_FAIL);
    }
    return (OSAL_SUCCESS);
#else
    return (OSAL_FAIL);
#endif
}

/*
 * ======== OSAL_netSslSend() ========
 *
 * This function is used to send data on SSL connection.
 *
 * Returns:
 *  OSAL_SUCCESS: Success.
 *  OSAL_FAIL:    Failure with proper OSAL error code set.
 */ 
OSAL_Status OSAL_netSslSend(
    OSAL_NetSslId *ssl_ptr,
    void          *buf_ptr,
    vint          *size_ptr)
{
#ifdef OSAL_NET_ENABLE_SSL
    int retval;
    
    retval = SSL_write(ssl_ptr->ssl_ptr, buf_ptr, *size_ptr);
    if (retval > 0) {
        *size_ptr = retval;
        return (OSAL_SUCCESS);
    }
#endif

    return (OSAL_FAIL);
}

/*
 * ======== OSAL_netSslReceive() ========
 *
 * This function is used to receive data on SSL connection.
 *
 * Returns:
 *  OSAL_SUCCESS: Success.
 *  OSAL_FAIL:    Failure with proper OSAL error code set.
 */ 
OSAL_Status OSAL_netSslReceive(
    OSAL_NetSslId *ssl_ptr,
    void          *buf_ptr,
    vint          *size_ptr)
{
#ifdef OSAL_NET_ENABLE_SSL
    int retval;
    
    retval = SSL_read(ssl_ptr->ssl_ptr, buf_ptr, *size_ptr);
    if (retval >= 0) {
        *size_ptr = retval;
        return (OSAL_SUCCESS);
    }
#endif

    return (OSAL_FAIL);
}

/*
 * ======== OSAL_netSslSetSocket() ========
 *
 * This function is used to associate a socket with a SSL connection object.
 *
 * Returns:
 *  OSAL_SUCCESS: Success.
 *  OSAL_FAIL:    Failure with proper OSAL error code set.
 */ 
OSAL_Status OSAL_netSslSetSocket(
    OSAL_NetSockId *socket_ptr,
    OSAL_NetSslId  *ssl_ptr)
{
#ifdef OSAL_NET_ENABLE_SSL
    int retval;

    retval = SSL_set_fd(ssl_ptr->ssl_ptr, *socket_ptr);
    if (retval > 0) {
        return (OSAL_SUCCESS);
    }
#endif

    return (OSAL_FAIL);
}

/*
 * ======== OSAL_netSslCertDestroy() ========
 *
 * This function is used to destroy certificate object
 *
 * Returns:
 *  OSAL_SUCCESS: Success.
 *  OSAL_FAIL:    Failure with proper OSAL error code set.
 */ 
OSAL_Status OSAL_netSslCertDestroy(
    OSAL_NetSslCert  *cert_ptr)
{
#ifdef OSAL_NET_ENABLE_SSL
    if (NULL != cert_ptr->pKey_ptr) {
        EVP_PKEY_free(cert_ptr->pKey_ptr);
    }

    if (NULL != cert_ptr->x509_ptr) {
        X509_free(cert_ptr->x509_ptr);
    }
    cert_ptr->x509_ptr = NULL;
    cert_ptr->pKey_ptr = NULL;
    return (OSAL_SUCCESS); 
#else
    return (OSAL_FAIL);
#endif
}

/*
 * ======== OSAL_netSslGetFingerprint() ========
 *
 * This function is used to get the fingerprint of a certificate
 *
 * Returns:
 *  OSAL_SUCCESS: Success.
 *  OSAL_FAIL:    Failure with proper OSAL error code set.
 */ 
OSAL_Status OSAL_netSslGetFingerprint(
    OSAL_NetSslCert *cert_ptr,
    OSAL_NetSslHash  hash,
    unsigned char   *fingerprint,
    vint             fpMaxLen,
    vint            *fpLen_ptr)
{
#ifdef OSAL_NET_ENABLE_SSL
    const EVP_MD    *digest;
    unsigned int     n;
    unsigned char    md[EVP_MAX_MD_SIZE];

    switch (hash) {
        case (OSAL_NET_SSL_HASH_SHA1):
            digest = EVP_get_digestbyname("sha1");
            break;
        case (OSAL_NET_SSL_HASH_MD5):
            digest = EVP_get_digestbyname("md5");
            break;
        default:
            return (OSAL_FAIL);
    }

    X509_digest(cert_ptr->x509_ptr, digest, md, &n);
    if ((int)n > fpMaxLen) {
        *fpLen_ptr = 0;
        return (OSAL_FAIL);
    }
    OSAL_memCpy(fingerprint, md, n);
    *fpLen_ptr = n;

    return (OSAL_SUCCESS);
#else
    return (OSAL_FAIL);
#endif    
}

/*
 * ======== OSAL_netResolve ========
 *
 * This function is used to make a DNS query and get response from a server.
 *
 * Returns:
 *  OSAL_SUCCESS: Success.
 *  OSAL_FAIL:    Failure with proper OSAL error code set.
 */
OSAL_Status OSAL_netResolve(
    int8                *query_ptr,
    int8                *ans_ptr,
    vint                 ansSize,
    vint                 timeoutsec,
    vint                 retries,
    OSAL_NetResolveType  qtype)
{
#ifdef OSAL_NET_ENABLE_DNS
# ifndef OSAL_RESOLVE_A_ONLY
    OSAL_NetSockId      sock;
    OSAL_Status         retval;
    OSAL_SelectTimeval  time;
    OSAL_SelectSet      fds;
    OSAL_Boolean        val;
    OSAL_NetAddress     servAddr;
    OSAL_NetAddress     addr;
    uint16              type;
    vint                rc;
    vint                len;
    vint                qlen;
    vint                i;
    int8                buf[OSAL_NET_DNS_UDP_MSG_MAX]; /* Max 512 is defined by RFC */
    int                 fid;
    int                 records;
    int                 j;
    ns_rr               rr;
    char                mn[OSAL_NET_DNS_ANS_STR_MAX];
    uint8              *cp_ptr;
    uint32              ip;
    char               *ansp_ptr;
    ns_msg              msg;
    uint16              ipv6[OSAL_NET_IPV6_WORD_SZ];
    char                ipv6_str[OSAL_NET_IPV6_STR_MAX];
    int                 order;
    int                 preference;
    char                ansBuf[OSAL_NET_DNS_ANS_STR_MAX];
    uint8               flagLength;
    char                flag[8];
    uint8               serviceLength;
    char                service[OSAL_NET_DNS_ANS_STR_MAX];

    /*
     * Check for NULL or invalid string.  Return addr 0x0 if NULL.
     */
    if (NULL == query_ptr) { 
        return (OSAL_FAIL);
    }
    if (strlen(query_ptr) < 2) { 
        return (OSAL_FAIL);
    }

    /*
     * Check if address is already an IP address.
     * If so, then no need to use DNS.
     */
    if (OSAL_SUCCESS == OSAL_netStringToAddress(query_ptr, &addr)) { 
        /* 
         * Original string is IP addr
         */
        strncpy(ans_ptr, query_ptr, ansSize);
        return (OSAL_SUCCESS);
    }

    /*
     * Get DNS server name.
     */
    fid = open("/etc/resolv.conf", O_RDONLY);
    if (fid < 0) {
        return (OSAL_FAIL);
    }

    /*
     * Correct thing in Linux is to get it from /etc/resolv.conf
     */
    do {
        rc = read(fid, buf, 1);
        if ((1 == rc) && buf[0] == 'n') {
            rc = read(fid, buf, 1);
            if ((1 == rc) && (buf[0] == 'a')) {
                rc = read(fid, buf, 1);
                if ((1 == rc) && (buf[0] == 'm')) {
                    rc = read(fid, buf, 1);
                    if ((1 == rc) && (buf[0] == 'e')) {
                        rc = read(fid, buf, 1);
                        if ((1 == rc) && (buf[0] == 's')) {
                            rc = read(fid, buf, 1);
                            if ((1 == rc) && (buf[0] == 'e')) {
                                rc = read(fid, buf, 1);
                                if ((1 == rc) && (buf[0] == 'r')) {
                                    rc = read(fid, buf, 1);
                                    if ((1 == rc) && (buf[0] == 'v')) {
                                        rc = read(fid, buf, 1);
                                        if ((1 == rc) && (buf[0] == 'e')) {
                                            rc = read(fid, buf, 1);
                                            if ((1 == rc) && (buf[0] == 'r')) {
                                                goto _OSAL_NET_RESOLVE_SNAME;
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    } while (1 == rc);

    close(fid);
    return (OSAL_FAIL);

_OSAL_NET_RESOLVE_SNAME:
    i = 0;
    do {
        rc = read(fid, &buf[i], 1);
        if ('\n' == buf[i]) {
            break;
        }
        else if ((' ' == buf[i]) || ('\t' == buf[i])) {
            i = i;
        }
        else {
            i++;
        }
    } while ((i < sizeof(buf)) && (1 == rc));
    close(fid);
    
    if (('\n' == buf[i]) || (' ' == buf[i]) || ('\t' == buf[i])) {
        buf[i] = 0;
    }
    else {
        buf[i+1] = 0;
    }

    /*
     * Convert string to address
     */
    if (OSAL_FAIL == (retval = OSAL_netStringToAddress(buf, &servAddr))) {
        return (OSAL_FAIL);
    }

    servAddr.port = OSAL_netHtons(53);

    /*
     * Map query type.
     */
    switch (qtype) {
        case OSAL_NET_RESOLVE_A:
            type = 0x0001;
            break;
        case OSAL_NET_RESOLVE_SRV:
            type = 0x0021;
            break;
        case OSAL_NET_RESOLVE_AAAA:
            type = 0x001C;
            break;
        case OSAL_NET_RESOLVE_NAPTR:
            type = 0x0023;
            break;
        default:
            return (OSAL_FAIL);
    }

    /*
     * Save against buffer overflow of ans_ptr.
     * ans_ptr will be used as scratch space and its size must be minimum size
     * of query_ptr + 32.
     */
    if ((strlen(query_ptr) + 32) > ansSize) {
        return (OSAL_FAIL);
    }

    /* 
     * Create Socket
     */
    retval = OSAL_netSocket(&sock, OSAL_NET_SOCK_UDP);
    if(OSAL_FAIL == retval) {
        return (OSAL_FAIL);
    }
    
    /* 
     * Set socket non blocking. This is required because ARP could block sendTo
     * function indefinitely.
     */
    retval = OSAL_netSetSocketOptions(&sock, OSAL_NET_SOCK_NONBLOCKING,
            OSAL_TRUE);
    if(OSAL_FAIL == retval) {
        return (OSAL_FAIL);
    }

    /*
     * Prepare DNS SRV query
     */
    len = _OSAL_netPrepareDnsQuery(type, query_ptr, ans_ptr, &qlen);

    while (retries--) {
        /*
         * Send DNS query. Allow single SRV query at a time.
         */
        rc = len;
        retval = OSAL_netSocketSendTo(&sock, ans_ptr, &rc, &servAddr);
        if ((rc != len) || (OSAL_SUCCESS != retval)) {
            OSAL_netCloseSocket(&sock);
            return(OSAL_FAIL);
        }

        OSAL_selectSetInit(&fds);
        OSAL_selectAddId(&sock, &fds);

        time.sec  = timeoutsec;
        time.usec = 0;
        if (OSAL_FAIL == OSAL_select(&fds, (OSAL_SelectSet *)0, &time, &val)) {
            OSAL_netCloseSocket(&sock);
            return (OSAL_FAIL);
        }
        
        if (OSAL_FAIL == OSAL_selectIsIdSet(&sock, &fds, &val)) {
            /*
             * Error out
             */
            OSAL_netCloseSocket(&sock);
            return (OSAL_FAIL);
        }

        if (OSAL_TRUE == val) {
            break;
        }
    }
      
    /*
     * Timeout on retries ?
     */
    if (-1 == retries) {
        OSAL_netCloseSocket(&sock);
        return (OSAL_FAIL);
    }

    memset(buf, 0, sizeof(buf));
    rc = sizeof(buf);
    retval = OSAL_netSocketReceiveFrom(&sock, buf,  &rc, &addr);
    if (OSAL_SUCCESS != retval) {
        OSAL_netCloseSocket(&sock);
        return (OSAL_FAIL);
    }
   
    /*
     * No more xacts, close socket.
     */
    OSAL_netCloseSocket(&sock);

    if ((addr.port != servAddr.port) || (addr.ipv4 != servAddr.ipv4) || 
            (ans_ptr[0] != buf[0]) || (ans_ptr[1] != buf[1])) {
        return (OSAL_FAIL);
    }


    /*
     * This code was removed because of GPL issues of DissectAnswerRecords().
     * But left in place for future development of DNS protocol for platforms
     * where libresolv.a or equivalent is not available.
     * XXX: Do not remove (ZK).
     */
    if (0) {
# if 0
        /*
         * Start parsing common DNS response.
         */

        if (0 == (buf[2] >> 7)) {
            /*
             * Not a response.
             */
            return (OSAL_FAIL);
        }
        if ((buf[2] >> 3) & 0xF) {
            /*
             * Not a standard query.
             */
            return (OSAL_FAIL);
        }
        if (buf[3] & 0xF) {
            /*
             * Error code set in flags.
             */
            return (OSAL_FAIL);
        }
        if ((0 != buf[4]) || (1 != buf[5])) {
            /*
             * Query is invalid. Asked 1, got 0 or many.
             */
            return (OSAL_FAIL);
        }
        if (0 == ((uint16)buf[6] << 8) + (uint16)buf[7]) {
            /*
             * No Answer RRs
             */
            return (OSAL_FAIL);
        }
        /*
         * Make sure query is same as query in response.
         */
        for (i = 12; i < (qlen + 12); i++) {
            /*
             * Mismatch. Not got what was asked.
             */
            if (buf[i] != ans_ptr[i]) {
                return (OSAL_FAIL);
            }
        }

        /*
         * Parse answers from buf[i],
         */
        memset(ans_ptr, 0, ansSize);
        if (DissectAnswerRecords(buf, i, 0, rc,
                ans, ans_ptr, ansSize) < 0) {
            return (OSAL_FAIL);
        }
# endif
    }
    else {
        /*
         * Parse DNS packet
         */
        if (0 != ns_initparse((unsigned char *)buf, rc, &msg)) {
            return (OSAL_FAIL);
        }

        /*
         * Find answers.
         */
        if (ns_msg_getflag(msg, ns_f_rcode) != ns_r_noerror) {
            return (OSAL_FAIL);
        }
        records = ns_msg_count(msg, ns_s_an);
        if (0 == records) {
            return (OSAL_FAIL);
        }

        /*
         * Parse records.
         */
        i = 0;
        ansp_ptr = ans_ptr;
        while (i < records) {

            /*
             * Skip record if error
             */
            if (ns_parserr(&msg, ns_s_an, i, &rr)) {
                i++;
                continue;
            }

            /*
             * Get pointer to record.
             */
            cp_ptr = (uint8 *)ns_rr_rdata(rr);
            
            /*
             * Find what we want from a string.
             */
            if (OSAL_NET_RESOLVE_A == qtype) {
                /* 
                 * Determine is the record has useful IP information.
                 * If 'type' is A then it's an address and we should process.
                 * Otherwise ignore it.
                 */
                if (ns_t_a != ns_rr_type(rr)) {
                    i++;
                    continue;
                }
                /*
                 * Do not go past the end of msg (4 octets = 32 bit IP)
                 */
                if ((cp_ptr + 4) > ns_msg_end(msg)) {
                    i++;
                    continue;
                }
                ip = ns_get32(cp_ptr);
                
                /*
                 * Put in ans buffer.  First check and see if we have room.
                 */
                if (16 >= ansSize) {
                    /* Break out of the 'records' processing loop */
                    break;
                }
                
                j = OSAL_snprintf(ansp_ptr, ansSize - 1, "%d.%d.%d.%d\n",
                        (ip >> 24) & 0xff,
                        (ip >> 16) & 0xff,
                        (ip >> 8) & 0xff,
                        (ip) & 0xff);
                ansSize -= j;
                ansp_ptr += j;
            }
            else if (OSAL_NET_RESOLVE_SRV == qtype) {
                /*
                 * Minimum SRV size is 6 octets
                 */
                if ((cp_ptr + 6) > ns_msg_end(msg)) {
                    i++;
                    continue;
                }
                /*
                 * Find / uncompress SRV server name
                 * Do not go past end of message.
                 */
                if (ns_name_uncompress(ns_msg_base(msg),
                        ns_msg_end(msg), cp_ptr + 6, mn, sizeof(mn)) < 0) {
                    /*
                     * If error then continue.
                     */
                    i++;
                    continue;
                }
                /*
                 * Put in ans buffer, 
                 *  first 2 octets = weight
                 *  second 2 octets = prio
                 *  thrid three octets = port
                 *  then follows server name
                 */
                
                /* Check if we have enough room */
                if ((OSAL_strlen(mn) + 19) >= ansSize) {
                    /* Break out of the 'records' processing loop */
                    break;
                }
                j = OSAL_snprintf(ansp_ptr, ansSize - 1, "%d:%d:%d:%s\n",
                        ns_get16(cp_ptr),
                        ns_get16(cp_ptr + 2),
                        ns_get16(cp_ptr + 4),
                        mn);
                ansSize -= j;
                ansp_ptr += j;
            }
            else if (OSAL_NET_RESOLVE_AAAA == qtype) {
                /* 
                 * Determine is the record has useful IP information.
                 * If 'type' is A then it's an address and we should process.
                 * Otherwise ignore it.
                 */
                if (ns_t_aaaa != ns_rr_type(rr)) {
                    i++;
                    continue;
                }
                /*
                 * Do not go past the end of msg (16 octets = 128 bit IP)
                 */
                if ((cp_ptr + 16) > ns_msg_end(msg)) {
                    i++;
                    continue;
                }

                OSAL_memCpy(ipv6, cp_ptr, sizeof(ipv6));
                /*
                 * Put in ans buffer.  First check and see if we have room.
                 */
                if (INET6_ADDRSTRLEN >= ansSize) {
                    /* Break out of the 'records' processing loop */
                    break;
                }

                inet_ntop(AF_INET6, ipv6, ipv6_str, INET6_ADDRSTRLEN);
                j = OSAL_snprintf(ansp_ptr, ansSize - 1, "%s\n",
                        ipv6_str);
                ansSize -= j;
                ansp_ptr += j;
            }
            else if (OSAL_NET_RESOLVE_NAPTR == qtype) {
                /*
                 * NAPTR answer format:
                 * |Order(2Bytes)|Preference(2Bytes)|Flag length(1Byte)|Flag(xBytes)|
                 * |ServiceLength(1Bytes)|Service(yBytes)|...(don't care for the rest)|
                 *
                 */

                /*
                 * Check if the answer type is NAPTR
                 */
                if (ns_t_naptr != ns_rr_type(rr)) {
                    i++;
                    continue;
                }
                /* Get order */
                order = ns_get16(cp_ptr);
                cp_ptr += 2;
                /* Get preference */
                preference = ns_get16(cp_ptr);
                cp_ptr += 2;
                /* Get flag length */
                flagLength = *((unsigned char*)cp_ptr);
                cp_ptr += 1;
                /* Get flag */
                OSAL_strncpy(flag, (char *)cp_ptr,
                        (sizeof(flag) < (flagLength + 1))?
                        sizeof(flag): (flagLength + 1));
                cp_ptr += flagLength;
                /* Get service length */
                serviceLength =  *((unsigned char*)cp_ptr);
                cp_ptr += 1;
                /* Get service */
                OSAL_strncpy(service, (char *)cp_ptr,
                        (sizeof(service) < (serviceLength + 1))?
                        sizeof(service): (serviceLength + 1));
                /*
                 * Put in ans buffer,
                 *  first 2 octets = order
                 *  second 2 octets = preference
                 *  then follows flag
                 *  then follows service name
                 */
                OSAL_snprintf(ansBuf, sizeof(ansBuf), "%d:%d:%s:%s\n",
                        order, preference, flag, service);

                /* Check if we have enough room */
                if (OSAL_strlen(ansBuf) >= ansSize) {
                    /* Break out of the 'records' processing loop */
                    break;
                }
                j = OSAL_snprintf(ansp_ptr, ansSize - 1, "%s", ansBuf);

                ansSize -= j;
                ansp_ptr += j;
            }


            /*
             * Next record
             */
            i++;
        }
        *ansp_ptr = 0;
        
        if (ans_ptr == ansp_ptr) {
            return (OSAL_FAIL);
        }
    }


    return (OSAL_SUCCESS);
# else

    struct hostent  *remoteHost;
    struct in_addr   addr;
    int              cnt;
    int              bytes;
    int              retryCount;

    retryCount = 0;
    while (NULL == (remoteHost = gethostbyname(query_ptr))) {
        if ((TRY_AGAIN != h_errno) || (retryCount >= OSAL_NET_DNS_RETRY_MAX)) {
            /* Return failure if error is not TRY_AGAIN or retry exceeds maximum. */
            return(OSAL_FAIL);
        }
        OSAL_logMsg("gethostbyname() failed with TRY_AGAIN error. Retrying...\n");
        retryCount++;
        OSAL_taskDelay(1000);
    }

    /* Fill out the DNS buffer */
    cnt = 0;
    while ((unsigned long *)remoteHost->h_addr_list[cnt]) {
        addr.s_addr = *(unsigned long *)remoteHost->h_addr_list[cnt];
        bytes = OSAL_snprintf(ans_ptr, ansSize, "%s\n", inet_ntoa(addr));
        if (bytes >= ansSize) {
            /* Truncated! Let's bail! */
            break;
        }
        ans_ptr += bytes;
        ansSize -= bytes;
        cnt++;
    }
    *ans_ptr = 0;
    return (OSAL_SUCCESS);
# endif /* !OSAL_RESOLVE_A_ONLY */
#endif /* !OSAL_NET_ENABLE_DNS */
    return (OSAL_FAIL);
}


/*
 * ======== OSAL_netGetHostByName ========
 * 
 * This function will resolve a name to an OSAL_NetAddress.
 * This function performs a DNS A record lookup. Note that this function will
 * return the first address of the DNS A record.  Even if the DNS A record 
 * returned several addresses, only the first one in the list is returned in
 * address_ptr.
 * 
 * Returns:
 *  OSAL_SUCCESS: Success.  The DNS A record IP address will be populated in 
 *                          the address_ptr parameter. Note that the 'port'
 *                          field in the OSAL_NetAddress will always be '0'.
 *  OSAL_FAIL:    The name in name_ptr could not be resolved to an IP address.
 */
OSAL_Status OSAL_netGetHostByName(
    int8            *name_ptr,
    int8            *ans_ptr,
    vint             ansSize,
    OSAL_NetAddress *address_ptr)
{
#ifdef OSAL_NET_ENABLE_DNS
# ifndef OSAL_RESOLVE_A_ONLY
    OSAL_NetAddress addr;
    char           *char_ptr;
    char           *pos_ptr;
    char            res_ptr[16];
    vint            retVal;


    /* Make query, DNS A records. timeout is 4 seconds.2 retries on timeout */
    if ((OSAL_NET_SOCK_UDP_V6 == address_ptr->type) ||
            (OSAL_NET_SOCK_TCP_V6 == address_ptr->type)) {
        retVal = OSAL_netResolve(name_ptr,
                ans_ptr, ansSize, 2, 4, OSAL_NET_RESOLVE_AAAA);
    }
    else {
        /* Make query, DNS A records. timeout is 4 seconds.2 retries on timeout */
        retVal = OSAL_netResolve(name_ptr,
                ans_ptr, ansSize, 2, 4, OSAL_NET_RESOLVE_A);
    }

    if (OSAL_SUCCESS == retVal) {
        /* Copy DNS buffer for conversion of the first IP in the list */
        OSAL_memCpy(res_ptr, ans_ptr, OSAL_NET_IPV6_STR_MAX);

        /* Find eol's and convert to '0' */
        pos_ptr = res_ptr;
        while ((char_ptr = strstr(pos_ptr, "\n")) != NULL) {
            *char_ptr = 0;
            pos_ptr = char_ptr + 1;
        }

        /* 
         * We only return the first IP address in the list. In the future
         * we could return all addressses.
         */
        /* Now convert the string to a long */
        addr.type = address_ptr->type;
        if (OSAL_SUCCESS == OSAL_netStringToAddress(res_ptr, &addr)) {
            address_ptr->port = 0;
            address_ptr->ipv4 = addr.ipv4;
            OSAL_memCpy(address_ptr->ipv6, addr.ipv6, sizeof(addr.ipv6));
            return (OSAL_SUCCESS);
        }
    }
    return (OSAL_FAIL);
# else
    struct addrinfo hints, *res;
    struct sockaddr_in *ipv4_ptr;
    struct sockaddr_in6 *ipv6_ptr;

    OSAL_memSet(&hints, 0, sizeof(hints));
    /* Set protocol family */
    if ((OSAL_NET_SOCK_UDP == address_ptr->type) ||
            (OSAL_NET_SOCK_TCP == address_ptr->type)) {
        hints.ai_family = AF_INET;
    }
    else {
        hints.ai_family = AF_INET6;
    }

    /* Get address info */
    if (0 != getaddrinfo((char*)name_ptr, NULL, &hints, &res)) {
        return (OSAL_FAIL);
    }

    /* Get first result in the list */
    if (res->ai_family == AF_INET) {
        ipv4_ptr = (struct sockaddr_in *)res->ai_addr;
        address_ptr->ipv4 = ipv4_ptr->sin_addr.s_addr;
    }
    else {
        ipv6_ptr = (struct sockaddr_in6 *)res->ai_addr;
        OSAL_memCpy(address_ptr->ipv6, &ipv6_ptr->sin6_addr, sizeof(address_ptr->ipv6));
    }

    /* Free the memory */
    freeaddrinfo(res);
    return (OSAL_SUCCESS);
# endif
#else
    return (OSAL_FAIL);
#endif
}

/*
 * ======== OSAL_netNtohl() ========
 *
 * See help for corresponding unix function ntohl, htonl, ntohs, htons.
 *
 * Returns:
 */ 
uint32 OSAL_netNtohl(
    uint32 val)
{
    return (ntohl(val));
}

/*
 * ======== OSAL_netHtonl() ========
 *
 * See help for corresponding unix function ntohl, htonl, ntohs, htons.
 *
 * Returns:
 */ 
uint32 OSAL_netHtonl(
    uint32 val)
{
    return (htonl(val));
}

/*
 * ======== OSAL_netNtohs() ========
 *
 * See help for corresponding unix function ntohl, htonl, ntohs, htons.
 *
 * Returns:
 */ 
uint16 OSAL_netNtohs(
    uint16 val)
{
    return (ntohs(val));
}

/*
 * ======== OSAL_netHtons() ========
 *
 * See help for corresponding unix function ntohl, htonl, ntohs, htons.
 *
 * Returns:
 */ 
uint16 OSAL_netHtons(
    uint16 val)
{
    return (htons(val));
}

/*
 * ======== OSAL_netIpv6Ntoh() ========
 *
 * Convert ipv6 address from  network to host byte order.
 *
 * Returns:
 */
void OSAL_netIpv6Ntoh(
    uint16 *dst_ptr,
    uint16 *src_ptr)
{
    int i;

    for (i = 0; i < OSAL_NET_IPV6_WORD_SZ; i++) {
        dst_ptr[i] = OSAL_netNtohs(src_ptr[i]);
    }
    return;
}

/*
 * ======== OSAL_netIpv6Hton() ========
 *
 * Convert ipv6 address from host to network byte order.
 *
 * Returns:
 */
void OSAL_netIpv6Hton(
    uint16 *dst_ptr,
    uint16 *src_ptr)
{
    int i;

    for (i = 0; i < OSAL_NET_IPV6_WORD_SZ; i++) {
        dst_ptr[i] = OSAL_netHtons(src_ptr[i]);
    }
    return;
}

/*
 * ======== OSAL_netAddrHton() ========
 *
 * Copy and convert OSAL_NetAddress ipv4 and ipv6 from host to network byte order.
 *
 * Returns:
 */
void OSAL_netAddrHton(
    OSAL_NetAddress *dst_ptr,
    OSAL_NetAddress *src_ptr)
{
    dst_ptr->type = src_ptr->type;
    /* ipv4 */
    dst_ptr->ipv4 = OSAL_netHtonl(src_ptr->ipv4);
    /* ipv6 */
    OSAL_netIpv6Hton(dst_ptr->ipv6, src_ptr->ipv6);

    return;
}

/*
 * ======== OSAL_netAddrNtoh() ========
 *
 * Copy and convert OSAL_NetAddress ipv4 and ipv6 from network to host byte order.
 *
 * Returns:
 */
void OSAL_netAddrNtoh(
    OSAL_NetAddress *dst_ptr,
    OSAL_NetAddress *src_ptr)
{
    dst_ptr->type = src_ptr->type;
    /* ipv4 */
    dst_ptr->ipv4 = OSAL_netNtohl(src_ptr->ipv4);
    /* ipv6 */
    OSAL_netIpv6Ntoh(dst_ptr->ipv6, src_ptr->ipv6);

    return;
}

/*
 * ======== OSAL_netAddrPortHton() ========
 *
 * Copy and convert OSAL_NetAddress ipv4, ipv6 and port from host to network byte order.
 *
 * Returns:
 */
void OSAL_netAddrPortHton(
    OSAL_NetAddress *dst_ptr,
    OSAL_NetAddress *src_ptr)
{
    /* type and ip address */
    OSAL_netAddrHton(dst_ptr, src_ptr);
    /* port */
    dst_ptr->port = OSAL_netHtons(src_ptr->port);

    return;
}

/*
 * ======== OSAL_netAddrPortNtoh() ========
 *
 * Copy and convert OSAL_NetAddress ipv4, ipv6 and port from network to host byte order.
 *
 * Returns:
 */
void OSAL_netAddrPortNtoh(
    OSAL_NetAddress *dst_ptr,
    OSAL_NetAddress *src_ptr)
{
    /* type and ip address */
    OSAL_netAddrNtoh(dst_ptr, src_ptr);
    /* port */
    dst_ptr->port = OSAL_netNtohs(src_ptr->port);

    return;
}

/*
 * ======== OSAL_netIpv6IsAddrZero() ========
 *
 * Check if ipv6 address is zero
 *
 * Returns:
 *   OSAL_TRUE: zero
 *   OSAL_FALSE: not zero
 */

OSAL_Boolean OSAL_netIpv6IsAddrZero(
    uint16 *addr_ptr)
{
    vint i;

    for (i = 0; i < OSAL_NET_IPV6_WORD_SZ; i++) {
        if (0 != addr_ptr[i]) {
            return (OSAL_FALSE);
        }
    }

    return (OSAL_TRUE);
}

/*
 * ======== OSAL_netIsAddrZero() ========
 *
 * Check if ipv4/ipv6 address is zero
 *
 * Returns:
 *   OSAL_TRUE: zero
 *   OSAL_FALSE: not zero
 */
OSAL_Boolean OSAL_netIsAddrZero(
    OSAL_NetAddress *addr_ptr)
{
    if ((OSAL_NET_SOCK_UDP_V6 == addr_ptr->type) ||
           (OSAL_NET_SOCK_TCP_V6 == addr_ptr->type)) {
        /* ipv6 */
        return OSAL_netIpv6IsAddrZero(addr_ptr->ipv6);
    }

    /* ipv4 */
    if (0 == addr_ptr->ipv4) {
        return (OSAL_TRUE);
    }

    return (OSAL_FALSE);
}

/*
 * ======== OSAL_netIsAddrLoopback() ========
 *
 * Check if ipv4/ipv6 address is loopback address
 *
 * Returns:
 *   OSAL_TRUE: zero
 *   OSAL_FALSE: not zero
 */
OSAL_Boolean OSAL_netIsAddrLoopback(
    OSAL_NetAddress *addr_ptr)
{
    if ((OSAL_NET_SOCK_UDP_V6 == addr_ptr->type) ||
           (OSAL_NET_SOCK_TCP_V6 == addr_ptr->type)) {
        /* ipv6 */
        if ((addr_ptr->ipv6[0] == 0) &&
                (addr_ptr->ipv6[1] == 0) &&
                (addr_ptr->ipv6[2] == 0) &&
                (addr_ptr->ipv6[3] == 0) &&
                (addr_ptr->ipv6[4] == 0) &&
                (addr_ptr->ipv6[5] == 0) &&
                (addr_ptr->ipv6[6] == 0) &&
                (addr_ptr->ipv6[7] == OSAL_netHtons(1))) {
            return (OSAL_TRUE);
        }
    }
    else if (0x7F000001 == addr_ptr->ipv4) {
        /* ipv4 */
        return (OSAL_TRUE);
    }

    return (OSAL_FALSE);
}

/*
 * ======== OSAL_netAddrCpy() ========
 *
 * Copy type and ip address
 *
 * Returns:
 */
void OSAL_netAddrCpy(
    OSAL_NetAddress *dst_ptr,
    OSAL_NetAddress *src_ptr)
{
    dst_ptr->type = src_ptr->type;
    dst_ptr->ipv4 = src_ptr->ipv4;
    OSAL_memCpy(dst_ptr->ipv6, src_ptr->ipv6, OSAL_NET_IPV6_WORD_SZ * 2); 
    return;
}

/*
 * ======== OSAL_netIpv6AddrCpy() ========
 *
 * Copy type and ip address
 *
 * Returns:
 */
void OSAL_netIpv6AddrCpy(
    uint16  *dst_ptr,
    uint16  *src_ptr)
{
    OSAL_memCpy(dst_ptr, src_ptr, OSAL_NET_IPV6_WORD_SZ * 2);
    return;
}

/*
 * ======== OSAL_netAddrCpy() ========
 *
 * Copy whole OSAL_NetAddress
 *
 * Returns:
 */
void OSAL_netAddrPortCpy(
    OSAL_NetAddress *dst_ptr,
    OSAL_NetAddress *src_ptr)
{
    if (NULL == src_ptr) {
        OSAL_netAddrClear(dst_ptr);
        return;
    }

    return OSAL_memCpy(dst_ptr, src_ptr, sizeof(OSAL_NetAddress));
}

/*
 * ======== OSAL_netIsAddrPortEqual() ========
 *
 * Compare if two OSAL_NetAddress is equal
 *
 * Returns:
 *  OSAL_TRUE: Two address is equal
 *  OSAL_FALSE: Otherwise
 */
OSAL_Boolean OSAL_netIsAddrPortEqual(
    OSAL_NetAddress *addrA_ptr,
    OSAL_NetAddress *addrB_ptr)
{
    if (addrA_ptr->type != addrB_ptr->type ||
            addrA_ptr->port != addrB_ptr->port) {
        return (OSAL_FALSE);
    }
    else if ((OSAL_NET_SOCK_UDP_V6 == addrA_ptr->type) ||
           (OSAL_NET_SOCK_TCP_V6 == addrA_ptr->type)) {
        /* IPv6 Address type */
        if ( 0 != OSAL_memCmp(addrA_ptr->ipv6, addrB_ptr->ipv6,
                sizeof(addrA_ptr->ipv6))) {
            return (OSAL_FALSE);
        }
    }
    else {
        /* IPv4 Address type */
        if (addrA_ptr->ipv4 != addrB_ptr->ipv4) {
            return (OSAL_FALSE);
        }
    }
    return (OSAL_TRUE);
}

/*
 * ======== OSAL_netIsAddrEqual() ========
 *
 * Compare if the type and ip address is equal
 *
 * Returns:
 *  OSAL_TRUE: Two address is equal
 *  OSAL_FALSE: Otherwise
 */
OSAL_Boolean OSAL_netIsAddrEqual(
    OSAL_NetAddress *addrA_ptr,
    OSAL_NetAddress *addrB_ptr)
{
    if (addrA_ptr->type != addrB_ptr->type) {
        return (OSAL_FALSE);
    }
    else if ((OSAL_NET_SOCK_UDP_V6 == addrA_ptr->type) ||
           (OSAL_NET_SOCK_TCP_V6 == addrA_ptr->type)) {
        /* IPv6 Address type */
        if ( 0 != OSAL_memCmp(addrA_ptr->ipv6, addrB_ptr->ipv6,
                sizeof(addrA_ptr->ipv6))) {
            return (OSAL_FALSE);
        }
    }
    else {
        /* IPv4 Address type */
        if (addrA_ptr->ipv4 != addrB_ptr->ipv4) {
            return (OSAL_FALSE);
        }
    }
    return (OSAL_TRUE);
}

/*
 * ======== OSAL_netIsAddrIpv6() ========
 *
 * Check if an OSAL_NetAddress is ipv6
 *
 * Returns:
 *  OSAL_TRUE: ipv6
 *  OSAL_FALSE: Otherwise
 */
OSAL_Boolean OSAL_netIsAddrIpv6(
    OSAL_NetAddress *addrA_ptr)
{
    return ((OSAL_NET_SOCK_UDP_V6 == addrA_ptr->type) ||
           (OSAL_NET_SOCK_TCP_V6 == addrA_ptr->type));
}

/*
 * ======== OSAL_netAddrClear() ========
 *
 * Set address to zero
 *
 * Returns:
 */
void OSAL_netAddrClear(
    OSAL_NetAddress *addr_ptr)
{
    addr_ptr->ipv4 = 0;
    OSAL_memSet(&addr_ptr->ipv6, 0, OSAL_NET_IPV6_WORD_SZ * 2);
}

/*
 * ======== OSAL_netIpv6AddrAnyInit() ========
 *
 * Set IPv6 address to use any.
 *
 * Returns:
 */
void OSAL_netIpv6AddrAnyInit(
    uint16 *addr_ptr)
{
    vint i;
    for (i = 0; i < OSAL_NET_IPV6_WORD_SZ; i++) {
        addr_ptr[i] = 0;
    }
}
