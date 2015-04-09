/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004-2010 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Revision: 1381 $ $Date: 2006-12-05 14:44:53 -0800 (Tue, 05 Dec 2006) $
 */

#include <osal_net.h>
#include <osal_mem.h>

extern struct socket *sockfd_lookup(
    int  fd,
    int *err);

extern int sock_setsockopt(
    struct socket *sock,
    int            level,
    int            optname,
    char          *optval,
    int            optlen);

extern void fput(
    struct file *file);

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
    if (*socket_ptr >= 0) {
        return (OSAL_SUCCESS);
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
    struct socket      *sock_ptr;
    struct msghdr       msg;
    struct iovec        iov;
    mm_segment_t        oldfs;
    struct sockaddr_in  address;
    struct sockaddr_in6 address_v6;
    int                 retval;
    
    retval = -1;
    oldfs = get_fs();
    set_fs(KERNEL_DS);
    
    sock_ptr = sockfd_lookup(*socket_ptr, &retval);
    if (!sock_ptr) {
        set_fs(oldfs);
        return (OSAL_FAIL);
    }

    if ((OSAL_NET_SOCK_TCP_V6 == address_ptr->type) ||
            (OSAL_NET_SOCK_UDP_V6 == address_ptr->type)) {
        OSAL_memSet(&address_v6, 0, sizeof(address_v6));
        address_v6.sin6_family = AF_INET6;
        address_v6.sin6_port = address_ptr->port;
        OSAL_memCpy(&address_v6.sin6_addr, address_ptr->ipv6, 
                sizeof(address_ptr->ipv6));
        msg.msg_name = (struct sockaddr *)&address_v6;
        msg.msg_namelen = sizeof(address_v6);
    }
    else {
        OSAL_memSet(&address, 0, sizeof(address));
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = address_ptr->ipv4;
        address.sin_port = address_ptr->port;
        msg.msg_name = (struct sockaddr *)&address;
        msg.msg_namelen = sizeof(address);
    }
 
    iov.iov_base = buf_ptr;
    iov.iov_len = *size_ptr;
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;
    msg.msg_control = NULL;
    msg.msg_controllen = 0;
    if (sock_ptr->file->f_flags & O_NONBLOCK) {
        msg.msg_flags = MSG_DONTWAIT;
    }
    else {
        msg.msg_flags = 0;
    }

    retval = *size_ptr;
    *size_ptr = sock_sendmsg(sock_ptr, &msg, retval);
    if (retval != *size_ptr) {
        fput(sock_ptr->file);
        set_fs(oldfs);
        return(OSAL_FAIL);
    }
    
    fput(sock_ptr->file);
    set_fs(oldfs);
    return(OSAL_SUCCESS);
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
    struct socket      *sock_ptr;
    struct iovec        iov;
    struct msghdr       msg;
    mm_segment_t        oldfs;
    struct sockaddr_in  address;
    struct sockaddr_in6 address_v6;
    int                 retval;
    
    retval = -1;
    
    oldfs = get_fs();
    set_fs(KERNEL_DS);

    sock_ptr = sockfd_lookup(*socket_ptr, &retval);
    if(!sock_ptr || !buf_ptr) {
        set_fs(oldfs);
        return (OSAL_FAIL);
    }

    if ((OSAL_NET_SOCK_TCP_V6 == address_ptr->type) ||
            (OSAL_NET_SOCK_UDP_V6 == address_ptr->type)) {
        OSAL_memSet(&address_v6, 0, sizeof(address_v6));
        address_v6.sin6_family = AF_INET6;
        address_v6.sin6_port = address_ptr->port;
        OSAL_memCpy(&address_v6.sin6_addr, address_ptr->ipv6, 
                sizeof(struct in6_addr));
        msg.msg_name = (struct sockaddr *)&address_v6;
        msg.msg_namelen = sizeof(address_v6);
    }
    else {
        OSAL_memSet(&address, 0, sizeof(address));
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = address_ptr->ipv4;
        address.sin_port = address_ptr->port;
        msg.msg_name = (struct sockaddr *)&address;
        msg.msg_namelen = sizeof(address);
    }

    msg.msg_control = NULL;
    msg.msg_controllen = 0;
    msg.msg_iovlen = 1;
    msg.msg_iov = &iov;
    iov.iov_len = *size_ptr;
    iov.iov_base = buf_ptr;
    if (sock_ptr->file->f_flags & O_NONBLOCK) {
        msg.msg_flags = MSG_DONTWAIT;
    }
    else {
        msg.msg_flags = 0;
    }

    *size_ptr = sock_recvmsg(sock_ptr, &msg, *size_ptr, msg.msg_flags);

    if ((*size_ptr >= 0) && (address_ptr != NULL)) {
        if ((OSAL_NET_SOCK_TCP_V6 == address_ptr->type) ||
                (OSAL_NET_SOCK_UDP_V6 == address_ptr->type)) {
            OSAL_memCpy(address_ptr->ipv6, &address_v6.sin6_addr, 
                    sizeof(struct in6_addr));
            address_ptr->port = address_v6.sin6_port;
        }
        else {
            address_ptr->ipv4 = address.sin_addr.s_addr;
            address_ptr->port = address.sin_port;
        }
    }
    
    fput(sock_ptr->file);
   
    set_fs(oldfs);
    return(OSAL_SUCCESS);
}

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
    mm_segment_t oldfs;
    int          family;
    int          s_type;
    int          protocol;
    
    *socket_ptr = -1;

    switch(type) {
        case OSAL_NET_SOCK_TCP:
            family = AF_INET;
            s_type = SOCK_STREAM;
            protocol = IPPROTO_TCP;
            break;

        case OSAL_NET_SOCK_UDP:
            family = AF_INET;
            s_type = SOCK_DGRAM;
            protocol = IPPROTO_UDP;
            break;

        case OSAL_NET_SOCK_TCP_V6:
            family = AF_INET6;
            s_type = SOCK_STREAM;
            protocol = IPPROTO_TCP;
            break;

        case OSAL_NET_SOCK_UDP_V6:
            family = AF_INET6;
            s_type = SOCK_DGRAM;
            protocol = IPPROTO_UDP;
            break;

        default:
            return (OSAL_FAIL);
    }

    oldfs = get_fs();
    set_fs(KERNEL_DS);
    *socket_ptr = OSAL_syscall3(__NR_socket, family, s_type, protocol);
    set_fs(oldfs);

    if (*socket_ptr >= 0) {
        return(OSAL_SUCCESS);
    }
    return(OSAL_FAIL);
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
    mm_segment_t   oldfs;
    
    oldfs = get_fs();
    set_fs(KERNEL_DS);
    OSAL_syscall3(__NR_close, (long)*socket_ptr, (long)0, (long)0);
    set_fs(oldfs);
    return(OSAL_SUCCESS);
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
    int                 retval;
    struct socket      *sock_ptr;
    mm_segment_t        oldfs;
    struct sockaddr_in  address;
    struct sockaddr_in6  address_v6;
    struct sockaddr     *addr_ptr;
    int                  size;
   
    retval = -1;
    if ((OSAL_NET_SOCK_TCP_V6 == address_ptr->type) ||
            (OSAL_NET_SOCK_UDP_V6 == address_ptr->type)) {
        OSAL_memSet(&address_v6, 0, sizeof(address_v6));
        address_v6.sin6_family = AF_INET6;
        address_v6.sin6_port = address_ptr->port;
        OSAL_memCpy(&address_v6.sin6_addr, address_ptr->ipv6, 
                sizeof(address_ptr->ipv6));
        addr_ptr = (struct sockaddr *)&address_v6;
        size = sizeof(address_v6);
    }
    else {
        OSAL_memSet(&address, 0, sizeof(address));
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = address_ptr->ipv4;
        address.sin_port = address_ptr->port;
        addr_ptr = (struct sockaddr *)&address;
        size = sizeof(address);
    }
    oldfs = get_fs();
    set_fs(KERNEL_DS);

    sock_ptr = sockfd_lookup(*socket_ptr, &retval);
    if (sock_ptr) {
        retval = sock_ptr->ops->bind(sock_ptr, addr_ptr, size);
        fput(sock_ptr->file);
    }
    set_fs(oldfs);

    if (retval < 0) {
        return (OSAL_FAIL);
    }
    return (OSAL_SUCCESS);
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
    int            retval;
    int            optvalue;
    struct socket *sock_ptr;
    mm_segment_t   oldfs;
    
    oldfs = get_fs();
    set_fs(KERNEL_DS);
    
    sock_ptr = sockfd_lookup(*socket_ptr, &retval);
    if (sock_ptr) {
        switch (option) {
            case OSAL_NET_SOCK_REUSE:
                if (1 == value) {
                    optvalue = 1;
                }
                else {
                    optvalue = 0;
                }
                retval = sock_setsockopt(sock_ptr, SOL_SOCKET, SO_REUSEADDR,
                        (char *)&optvalue, sizeof(optvalue));
                if (retval >= 0) {
                    fput(sock_ptr->file);
                    set_fs(oldfs);
                    return (OSAL_SUCCESS);
                }
                break;

            case OSAL_NET_SOCK_NONBLOCKING:
                if (1 == value) {
                    sock_ptr->file->f_flags |= O_NONBLOCK;
                }
                else {
                    sock_ptr->file->f_flags &= ~O_NONBLOCK;
                }
                fput(sock_ptr->file);
                set_fs(oldfs);
                return (OSAL_SUCCESS);
                break;

            case OSAL_NET_IP_TOS:
                optvalue = value;
                retval = sock_ptr->ops->setsockopt(sock_ptr, SOL_IP, IP_TOS,
                        (char *)&optvalue, sizeof(optvalue));
                if (retval >= 0) {
                    fput(sock_ptr->file);
                    set_fs(oldfs);
                    return (OSAL_SUCCESS);
                }
                break;

            default:
                break;
        }
        fput(sock_ptr->file);
    }
        
    set_fs(oldfs);
    return (OSAL_FAIL);
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
    if ((0 != addr_ptr[0]) ||
            (0 != addr_ptr[1]) ||
            (0 != addr_ptr[2]) ||
            (0 != addr_ptr[3]) ||
            (0 != addr_ptr[4]) ||
            (0 != addr_ptr[5]) ||
            (0 != addr_ptr[6]) ||
            (0 != addr_ptr[7])) {
        return (OSAL_FALSE);
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
    else if (OSAL_netHtonl(OSAL_NET_INADDR_LOOPBACK) ==
            addr_ptr->ipv4) {
        /* ipv4 */
        return (OSAL_TRUE);
    }

    return (OSAL_FALSE);
}

EXPORT_SYMBOL(OSAL_netSetSocketOptions);
EXPORT_SYMBOL(OSAL_netSocketReceiveFrom);
EXPORT_SYMBOL(OSAL_netCloseSocket);
EXPORT_SYMBOL(OSAL_netBindSocket);
EXPORT_SYMBOL(OSAL_netSocket);
EXPORT_SYMBOL(OSAL_netSocketSendTo);
EXPORT_SYMBOL(OSAL_netNtohl);
EXPORT_SYMBOL(OSAL_netHtonl);
EXPORT_SYMBOL(OSAL_netNtohs);
EXPORT_SYMBOL(OSAL_netHtons);
EXPORT_SYMBOL(OSAL_netIsAddrZero);
EXPORT_SYMBOL(OSAL_netIsAddrLoopback);
EXPORT_SYMBOL(OSAL_netIsSocketIdValid);

