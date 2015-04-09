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
#include <osal_random.h>
#include <resolv.h>
#include <mn_api.h>
#include <osal_file.h>


/* Maximum DNS query result. */
#define _OSAL_NET_DNS_RESULT_MAX (5)

#define OSAL_NET_SSL_FIFO_DEPTH    (4)

typedef struct {
    vint msgType;
    char data[2000];  /* XXX */
    uint32 dataLen;
} OSAL_NetSslMsg;

typedef enum {
    OSAL_NET_SSL_HANDSHAK_SUCCESS,
    OSAL_NET_SSL_DE_SUCCESS,
    OSAL_NET_SSL_EN_SUCCESS,
    OSAL_NET_SSL_FAIL,
}OSAL_NetSslResult;

typedef struct {
    OSAL_NetSockId sockId;
    uint16 port;
} OSAL_NetServerInfo;
OSAL_NetServerInfo osalServerlInfo[4]; /* XXX */
#if defined(OSAL_NET_ENABLE_DNS) && !defined(OSAL_RESOLVE_A_ONLY)
/* Global netid for osal net internal use. */
static TCPIP_NETID_T _globalNetid;
/*
 * ======== _OSAL_netSplitDot() ========
 *
 * This function is used to tokenize a string with token dot(.) .
 *
 * Returns:
 *  Next location of dot in the string.
 */
static int8 *_OSAL_netSplitDot(
    int8 *str_ptr)
{
    int i;
    int len;

    len = strlen((const char *)str_ptr);
    i = 0;
    while (i < len) {
        if ('.' == str_ptr[i]) {
            return (&str_ptr[i]);
        }
        i++;
    }
    return (NULL);
}

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
    uint16 x;

    OSAL_randomGetOctets((char *)&x, sizeof(x));
    
    return (x);
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
    buf_ptr[len++] = strlen((const char *)query_ptr);
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

#ifdef OSAL_NET_ENABLE_SSL
/*
 * ======== _OSAL_netSslReadDataFromFifo() ========
 *
 * This function is used to read data from ssl fifo.
 *
 * Returns:
 *  SUCCESS: data size.
 *  ERROR: OSAL_FAIL.
 */  
OSAL_Status _OSAL_netSslReadDataFromFifo(
    OSAL_NetSslId  *ssl_ptr,
    void           *buf_ptr,
    vint            size,
    vint            timeout)
{
    OSAL_SelectSet      fdSet;
    OSAL_SelectTimeval  time;
    OSAL_SelectTimeval *timeout_ptr;
    OSAL_Boolean        isTimedOut;
    vint                readSize = -1;
    vint                remainSize;

    /* If a fd doesn't exist then create it. */
    if (0 == ssl_ptr->fid) {
        if (OSAL_SUCCESS != OSAL_fileOpen(&ssl_ptr->fid,
                OSAL_NET_SSL_FIFO, OSAL_FILE_O_RDWR, 0)) {
            ssl_ptr->fid = 0;
            OSAL_logMsg("%s:%d Open %s device FAIL.\n", __FUNCTION__, __LINE__,
                    ssl_ptr->ssl_ptr);
            return (OSAL_FAIL);
        }
    } 

    OSAL_memSet(&time, 0, sizeof(OSAL_SelectTimeval));

    if (OSAL_WAIT_FOREVER == timeout) {
        timeout_ptr = NULL;
    }
    else {
        time.sec= timeout / 1000;
        time.usec= timeout % 1000;
        timeout_ptr = &time;
    }

    /* Select Fd and read file. */
    OSAL_selectSetInit(&fdSet);
    OSAL_selectAddId(&ssl_ptr->fid, &fdSet);
    if (OSAL_FAIL ==
            OSAL_select(&fdSet, NULL, timeout_ptr, &isTimedOut)) {
        OSAL_logMsg("%s:%d File select FAIL.\n", __FUNCTION__, __LINE__);
        return (OSAL_FAIL);
    }
    /* Read until required data size is read */
    remainSize = size;
    readSize   = 0;
    while (size > 0) {
        size = remainSize;
        if (OSAL_FAIL == OSAL_fileRead(&ssl_ptr->fid,
                (void *)((char *)buf_ptr + readSize), &size)) {
            OSAL_logMsg("%s:%d Read file FAIL.\n", __FUNCTION__, __LINE__);
            return (readSize);
        }
        remainSize -= size;
        readSize += size;
    }

    return (readSize);
}

/*
 * ======== _OSAL_netSslWriteDataToFifo() ========
 *
 * This function is used to write data to ssl fifo.
 *
 * Returns:
 *  ERROR: OSAL_FAIL
 *  SUCCESS: OSAL_SUCCESS
 */  
OSAL_Status _OSAL_netSslWriteDataToFifo(
    OSAL_NetSslId  *ssl_ptr,
    void           *buf_ptr,
    vint            size)
{
    /* If a fd doesn't exist then create it. */
    if (0 == ssl_ptr->fid) {
        OSAL_logMsg("%s:%d write %s fifo but it is not opened yet.\n",
                __FUNCTION__, __LINE__, OSAL_NET_SSL_FIFO);
        if (OSAL_SUCCESS != OSAL_fileOpen(&ssl_ptr->fid,
                OSAL_NET_SSL_FIFO, OSAL_FILE_O_RDWR, 0)) {
            ssl_ptr->fid = 0;
            OSAL_logMsg("%s:%d Open %s fifo FAIL.\n",
                    __FUNCTION__, __LINE__, OSAL_NET_SSL_FIFO);
            return (OSAL_FAIL);
        }
    }

    /* Now Write */
    if (OSAL_FAIL == OSAL_fileWrite(&ssl_ptr->fid, buf_ptr, &size)) {
        OSAL_logMsg("%s:%d Sending to %s FAIL.\n", __FUNCTION__, __LINE__,
                ssl_ptr->ssl_ptr);
        return (OSAL_FAIL);
    }

    return (OSAL_SUCCESS);
}

/*
 * ======== _OSAL_netSslSendDataCb() ========
 *
 * This function is a callback function of SSL module and 
 * used to send encripted data to SSL server.
 * 
 *
 * Returns:
 * void
 */  
void _OSAL_netSslSendDataCb(
    void  *this_ptr,
    char  *data_ptr,
    uint32 dataLen)
{
    OSAL_NetSslMsg msg;
    OSAL_NetSslId *ssl_ptr;
    char           dataBuf[2000];
    
    if(NULL == this_ptr) {
        OSAL_logMsg("%s:%d\n", __FILE__, __LINE__);
        return;
    }
    
    ssl_ptr = (OSAL_NetSslId *)this_ptr;
    if (data_ptr != NULL) {
        OSAL_memCpy(dataBuf, data_ptr, dataLen);
        if(OSAL_SUCCESS ==
             OSAL_netSocketSend(&ssl_ptr->sockId, data_ptr, dataLen)) {
             msg.msgType = OSAL_NET_SSL_EN_SUCCESS;
             SSL_AsyncMessageProc(ssl_ptr->ssl_ptr, SSL_RECV_MESSAGE_SEND_SUCC,
                    dataLen);                
        }
        else {
            OSAL_logMsg("%s:%d Fail to Send encriptied data to peer.\n", __FILE__, __LINE__);
            OSAL_netCloseSocket(&ssl_ptr->sockId);
        }
    }
}

/*
 * ======== _OSAL_netSslRecvDataCb() ========
 *
 * This function is a callback function of SSL module and
 * used to receive the decripted data from SSL module, then pass
 * to the application via ssl fifo.
 *
 * Returns:
 * void
 */  
void _OSAL_netSslRecvDataCb(
    void* this_ptr,
    char* data_ptr,
    uint32 dataLen )
{
    OSAL_NetSslMsg msg;
    OSAL_NetSslId *ssl_ptr;


    if(NULL == this_ptr) {
        OSAL_logMsg("%s:%d\n", __FILE__, __LINE__);
        return;
    }
    
    ssl_ptr = (OSAL_NetSslId *) this_ptr;
    /* Back up the decrypted data. */
    if (data_ptr != NULL) {
        OSAL_memSet(msg.data, 0, sizeof(msg.data));
        OSAL_memCpy(msg.data, data_ptr, dataLen);
        msg.dataLen = dataLen;
        msg.msgType = OSAL_NET_SSL_DE_SUCCESS;
    }
    else {
        msg.msgType =  OSAL_NET_SSL_FAIL;
        SSL_AsyncMessageProc(ssl_ptr->ssl_ptr, SSL_RECV_MESSAGE_RECV_FAIL,
                dataLen); 
    }

    if(OSAL_SUCCESS ==
            _OSAL_netSslWriteDataToFifo(ssl_ptr, &msg,
                    sizeof(OSAL_NetSslMsg))) {
        /* Inform SSL this data has been backed up. */ 
        SSL_AsyncMessageProc(ssl_ptr->ssl_ptr, SSL_RECV_MESSAGE_RECV_SUCC,
                dataLen);
    }
    else {
        SSL_AsyncMessageProc(ssl_ptr->ssl_ptr, SSL_RECV_MESSAGE_RECV_FAIL,
                dataLen); 
    }
}

/*
 * ======== _OSAL_netSslPostMessageCb() ========
 *
 * This function is a callback function of SSL module and
 * used to post the message for the processing result of SSL module.
 *
 * Returns:
 * void
 */
void _OSAL_netSslPostMessageCb(
    void* this_ptr,
    uint32 messageId)
{
    OSAL_NetSslMsg msg;
    OSAL_NetSslId *ssl_ptr;
    
    if(NULL == this_ptr) {
        return;
    }

    ssl_ptr = (OSAL_NetSslId *) this_ptr;
    switch (messageId) {
        case SSL_SEND_MESSAGE_HANDSHAKE_SUCC:
            msg.msgType = OSAL_NET_SSL_HANDSHAK_SUCCESS;
            break;         
         case SSL_SEND_MESSAGE_FAIL:
            msg.msgType = OSAL_NET_SSL_FAIL;
            break;           
         case SSL_SEND_MESSAGE_SEND_END:
         case SSL_SEND_MESSAGE_CLOSE_BY_SERVER:
         case SSL_SEND_MESSAGE_CANCLED_BY_USER:
         case SSL_SEND_MESSAGE_NULL:
         case SSL_SEND_MESSAGE_MAX:
            /* XXX */  
            break;
         default:
            break;
    }

    _OSAL_netSslWriteDataToFifo(ssl_ptr, &msg, sizeof(OSAL_NetSslMsg)); 
}

/*
 * ======== _OSAL_netShowCertInfoCb() ========
 *
 * This function is a callback function of SSL module and
 * used to notify the application the certification error, then the application
 * must decide to stop or continue the handsake procedure and 
 * notify the SSL modules.
 *
 * Returns:
 *  ERROR: OSAL_FAIL
 *  SUCCESS: OSAL_SUCCESS
 */
OSAL_Status _OSAL_netShowCertInfoCb(
        void* this_ptr)
{
    OSAL_NetSslId  *ssl_ptr;
    OSAL_Boolean    isContinue;

    isContinue = OSAL_TRUE;
    if(NULL == this_ptr) {
        return;
    }
    ssl_ptr = (OSAL_NetSslId *)this_ptr;    
    /* notify the upper application. */

    /* get the upper application's decision. XXX */

    /* Pass User's Decision on cert to SSL Module. */
    SSL_UserCnfCert(ssl_ptr->ssl_ptr, isContinue);

    return (OSAL_SUCCESS);
}

/*
 * ======== _OSAL_netSslSetServer() ========
 *
 * This function is used to store the socket id and server port number.
 *
 * Returns:
 *  ERROR: OSAL_FAIL
 *  SUCCESS: OSAL_SUCCESS
 */
OSAL_Status _OSAL_netSslSetServer(
    OSAL_NetSockId  *socket_ptr,
    uint16 port)
{
    int index;

    for (index = 0; index < 4; index++) {
        if (0 == osalServerlInfo[index].sockId) {
            osalServerlInfo[index].sockId =  *socket_ptr;
            osalServerlInfo[index].port = port;
            return (OSAL_SUCCESS);
        }
    }

    return (OSAL_FAIL);
}

/*
 * ======== _OSAL_netSslSearchServer() ========
 *
 * This function is used to find the corresponding socket id and 
 * server port number.
 *
 * Returns:
 *  ERROR: OSAL_FAIL
 *  SUCCESS: OSAL_SUCCESS 
 */
OSAL_Status _OSAL_netSslSearchServer(
    OSAL_NetSslId     *ssl_ptr,
    OSAL_NetSockId  sockId)
{
    int index;

    for (index = 0; index < 4; index++) {
        if (sockId == osalServerlInfo[index].sockId) {
            ssl_ptr->port =  osalServerlInfo[index].port;
            OSAL_memSet(&osalServerlInfo[index], 0, sizeof(OSAL_NetServerInfo));
            return (OSAL_SUCCESS);
        }
    }

    return (OSAL_FAIL);
}
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
    TCPIP_NETID_T netid;
    int           retval;

    if (NULL == socket_ptr) {
        return (OSAL_FAIL);
    }
    /* Use loopback netid for now and modify it when binding. */
    netid = sci_getLoopBackNetid();


    switch (type) {
        case OSAL_NET_SOCK_TCP:
            retval = sci_sock_socket(AF_INET, SOCK_STREAM, 0, netid);
            break;
        case OSAL_NET_SOCK_UDP:
            retval = sci_sock_socket(AF_INET, SOCK_DGRAM, 0, netid);
            break;
        case OSAL_NET_SOCK_TCP_V6:
            retval = sci_sock_socket(AF_INET6, SOCK_STREAM, 0, netid);
            break;
        case OSAL_NET_SOCK_UDP_V6:
            retval = sci_sock_socket(AF_INET6, SOCK_DGRAM, 0, netid);
            break;
        default:
            return (OSAL_FAIL);
    }

    if (TCPIP_SOCKET_INVALID != retval) {
        *socket_ptr = retval;
        return (OSAL_SUCCESS);
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
    if (NULL == socket_ptr) {
        return (OSAL_FAIL);
    }

    if (0 == sci_sock_socketclose(*socket_ptr)) {
        *socket_ptr = TCPIP_SOCKET_INVALID;
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
    if (*socket_ptr != TCPIP_SOCKET_INVALID) {
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
    struct sci_sockaddrext *addr_ptr;
    struct sci_sockaddr6    addrV6;
    struct sci_sockaddr     addrV4;
    int                     size;

    if ((OSAL_NET_SOCK_TCP_V6 == address_ptr->type) ||
            (OSAL_NET_SOCK_UDP_V6 == address_ptr->type)) {
        OSAL_memSet(&addrV6, 0, sizeof(addrV6));
        addrV6.sin6_family = AF_INET6;
        addrV6.sin6_port   = address_ptr->port;
        OSAL_memCpy(&addrV6.sin6_addr, address_ptr->ipv6,
                sizeof(addrV6.sin6_addr));
        addr_ptr = (struct sci_sockaddrext *)&addrV6;
        size = sizeof(addrV6);
    }
    else {
        OSAL_memSet(&addrV4, 0, sizeof(addrV4));
        addrV4.family  = AF_INET;
        addrV4.port    = address_ptr->port;
        addrV4.ip_addr = address_ptr->ipv4;
        addr_ptr = (struct sci_sockaddrext *)&addrV4;
        size = sizeof(addrV4);
    }
    if (0 == sci_sock_connect(*socket_ptr, addr_ptr, size)) {
 #ifdef OSAL_NET_ENABLE_SSL
    if ((OSAL_NET_SOCK_TCP == address_ptr->type) ||
            (OSAL_NET_SOCK_TCP_V6 == address_ptr->type)) {
        _OSAL_netSslSetServer(socket_ptr, address_ptr->port);
     }
 #endif
        return (OSAL_SUCCESS);
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

    struct sci_sockaddrext *addr_ptr;
    struct sci_sockaddr6    addrV6;
    struct sci_sockaddr     addrV4;
    int                     size;
    TCPIP_NETID_T           netid = 0;
    MN_GPRS_PDP_ADDR_T      ipAddr;
    int                     ret;
    if ((NULL == address_ptr) || (NULL == socket_ptr)) {
        return (OSAL_FAIL);
    }

    /* Modify netid to real one if it's not loopback */
    if ((!OSAL_netIsAddrLoopback(address_ptr)) &&
            (!OSAL_netIsAddrZero(address_ptr))) {
        if (OSAL_netIsAddrIpv6(address_ptr)) {
            /* ipv6 */
            ipAddr.length = sizeof(address_ptr->ipv6);
            OSAL_memCpy(ipAddr.value_arr, address_ptr->ipv6,
                    ipAddr.length);
        }
        else {
            /* ipv4 */
            ipAddr.length = sizeof(address_ptr->ipv4);
            OSAL_memCpy(ipAddr.value_arr, &address_ptr->ipv4,
                    ipAddr.length);
        }
        /* Look up netid from ip address. */
        if ((ret = MN_getNetIdByIpAddr(MN_DUAL_SYS_1, ipAddr)) <= 0) {
            OSAL_logMsg("Failed to get net id. ret:%d\n", ret);
            return (OSAL_FAIL);
        }
        netid = (TCPIP_NETID_T)ret;
        if (-1 == sci_sock_modifynetid(*socket_ptr, netid)) {
            OSAL_logMsg("Failed to modify netid %d for socket:%x\n",
                    netid, *socket_ptr);
            return (OSAL_FAIL);
        }
#if defined(OSAL_NET_ENABLE_DNS) && !defined(OSAL_RESOLVE_A_ONLY)
        /* Cache netid to _globalNetid. */
        _globalNetid = netid;
#endif
    }
 
    if ((OSAL_NET_SOCK_TCP_V6 == address_ptr->type) ||
            (OSAL_NET_SOCK_UDP_V6 == address_ptr->type)) {
        OSAL_memSet(&addrV6, 0, sizeof(addrV6));
        addrV6.sin6_family = AF_INET6;
        addrV6.sin6_port   = address_ptr->port;
        OSAL_memCpy(&addrV6.sin6_addr, address_ptr->ipv6,
                sizeof(addrV6.sin6_addr));
        addr_ptr = (struct sci_sockaddrext *)&addrV6;
        size = sizeof(addrV6);
    }
    else {
        OSAL_memSet(&addrV4, 0, sizeof(addrV4));
        addrV4.family  = AF_INET;
        addrV4.port    = address_ptr->port;
        addrV4.ip_addr = address_ptr->ipv4;
        addr_ptr = (struct sci_sockaddrext *)&addrV4;
        size = sizeof(addrV4);
    }
    if (0 == (ret = sci_sock_bind(*socket_ptr, addr_ptr, size))) {
        return (OSAL_SUCCESS);
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
    struct sci_sockaddrext  addr;
    struct sci_sockaddr6   *addrV6_ptr;
    struct sci_sockaddr    *addrV4_ptr;

    if ((socket_ptr == NULL) || (address_ptr == NULL)) {
        return (OSAL_FAIL);
    }

    /* Get ip and port. */
    if (0 != sci_sock_getsockname(*socket_ptr, &addr)) {
        return (OSAL_FAIL);
    }

    /* Get socket type. */
    OSAL_netGetSocketOptions(socket_ptr, OSAL_NET_SOCK_TYPE,
            (int *)&address_ptr->type);

    if (AF_INET == addr.sa_family) {
        addrV4_ptr = (struct sci_sockaddr *)&addr;
        address_ptr->ipv4 = addrV4_ptr->ip_addr;
        address_ptr->port = addrV4_ptr->port;
    }
    else if (AF_INET6 == addr.sa_family) {
        addrV6_ptr = (struct sci_sockaddr6 *)&addr;
        OSAL_memCpy(address_ptr->ipv6, &addrV6_ptr->sin6_addr,
                sizeof(address_ptr->ipv6));
        address_ptr->port = addrV6_ptr->sin6_port;
    }

    return (OSAL_SUCCESS);
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
    if (0 == sci_sock_listen(*socket_ptr, 1)) {
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
    struct sci_sockaddrext  addr;
    struct sci_sockaddr6   *addrV6_ptr;
    struct sci_sockaddr    *addrV4_ptr;
    int                     len;
    int                     retval;

    if ((socket_ptr == NULL) || (address_ptr == NULL) ||
            (address_ptr == NULL)) {
        return (OSAL_FAIL);
    }

    OSAL_memSet(&addr, 0, sizeof(addr));
    if ((OSAL_NET_SOCK_TCP_V6 == address_ptr->type) ||
            (OSAL_NET_SOCK_UDP_V6 == address_ptr->type)) {
        len = sizeof(struct sci_sockaddr6);
    }
    else {
        len = sizeof(struct sci_sockaddr);
    }

    retval = sci_sock_accept(*socket_ptr, &addr, &len);
    if (-1 != retval) {
        if ((OSAL_NET_SOCK_TCP_V6 == address_ptr->type) ||
                (OSAL_NET_SOCK_UDP_V6 == address_ptr->type)) {
            addrV6_ptr = (struct sci_sockaddr6 *)&addr;
            OSAL_memCpy(address_ptr->ipv6, &addrV6_ptr->sin6_addr,
                    sizeof(address_ptr->ipv6));
            address_ptr->port = addrV6_ptr->sin6_port;

        }
        else {
            addrV4_ptr = (struct sci_sockaddr *)&addr;
            address_ptr->ipv4 = addrV4_ptr->ip_addr;
            address_ptr->port = addrV4_ptr->port;
        }
        *newSocket_ptr = retval;
        return (OSAL_SUCCESS);
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
    int   retval;
    vint  flgs = 0;

    if (0 != (flags & OSAL_NET_RECV_PEEK)) {
        flgs |= MSG_PEEK;
    }

    retval = sci_sock_recv(*socket_ptr, buf_ptr, *size_ptr, flgs);

    if (retval >= 0) {
        *size_ptr = retval;
        return (OSAL_SUCCESS);
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
    struct sci_sockaddrext  addr;
    struct sci_sockaddr6   *addrV6_ptr;
    struct sci_sockaddr    *addrV4_ptr;
    int                     retval;

    retval = sci_sock_recvfrom(*socket_ptr, buf_ptr, *size_ptr, 0, &addr);

    if (retval >= 0) {
        *size_ptr = retval;
        if (AF_INET == addr.sa_family) {
            addrV4_ptr = (struct sci_sockaddr *)&addr;
            address_ptr->type = OSAL_NET_SOCK_UDP;
            address_ptr->ipv4 = addrV4_ptr->ip_addr;
            address_ptr->port = addrV4_ptr->port;
        }
        else if (AF_INET6 == addr.sa_family) {
            addrV6_ptr = (struct sci_sockaddr6 *)&addr;
            address_ptr->type = OSAL_NET_SOCK_UDP_V6;
            OSAL_memCpy(address_ptr->ipv6, &addrV6_ptr->sin6_addr,
                    sizeof(address_ptr->ipv6));
            address_ptr->port = addrV6_ptr->sin6_port;
        }
        else {
            /* Neither AF_INET nor AF_INET6. Should not go to here */
            return (OSAL_FAIL);
        }
        return (OSAL_SUCCESS);
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
    int retval;

    retval = sci_sock_send(*socket_ptr, buf_ptr, *size_ptr, 0);

    if (retval >= 0) {
        *size_ptr = retval;
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
    struct sci_sockaddrext  addr;
    struct sci_sockaddr6   *addrV6_ptr;
    struct sci_sockaddr    *addrV4_ptr;
    int                     retval;

    OSAL_memSet(&addr, 0, sizeof(addr));
    if ((OSAL_NET_SOCK_TCP_V6 == address_ptr->type) ||
            (OSAL_NET_SOCK_UDP_V6 == address_ptr->type)) {
        addrV6_ptr = (struct sci_sockaddr6 *)&addr;
        addrV6_ptr->sin6_family = AF_INET6;
        addrV6_ptr->sin6_port = address_ptr->port;
        OSAL_memCpy(&addrV6_ptr->sin6_addr, address_ptr->ipv6,
                sizeof(address_ptr->ipv6));

    }
    else {
        addrV4_ptr = (struct sci_sockaddr *)&addr;
        addrV4_ptr->family = AF_INET;
        addrV4_ptr->port = address_ptr->port;
        addrV4_ptr->ip_addr = address_ptr->ipv4;
    }

    retval = sci_sock_sendto(*socket_ptr, buf_ptr, *size_ptr, 0, &addr);

    if (retval >= 0) {
        *size_ptr = retval;
        return (OSAL_SUCCESS);
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

    switch (option) {
        case OSAL_NET_SOCK_REUSE:
            retval = sci_sock_setsockopt(*socket_ptr, SO_REUSEADDR,
                    (void *)&value);
            break;
        case OSAL_NET_SOCK_NONBLOCKING:
            retval = sci_sock_setsockopt(*socket_ptr, SO_NONBLOCK,
                    (void *)&value);
            break;
        case OSAL_NET_IP_TOS:
            return (OSAL_SUCCESS);
        case OSAL_NET_SOCK_RCVTIMEO_SECS:
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
    struct sci_sockaddrext  addr;
    int                     retval;
    int                     optvalue;

    switch (option) {
        case OSAL_NET_SOCK_REUSE:
            if (-1 == (retval = sci_sock_getsockopt(*socket_ptr, SO_REUSEADDR,
                    (void *)&optvalue))) {
                return (OSAL_FAIL);
            }

            if (optvalue) {
                *value_ptr = OSAL_TRUE;
            }
            else {
                *value_ptr = OSAL_FALSE;
            }
            break;
        case OSAL_NET_SOCK_NONBLOCKING:
            if (-1 == (retval = sci_sock_getsockopt(*socket_ptr, SO_NONBLOCK,
                    (void *)&optvalue))) {
                return (OSAL_FAIL);
            }

            if (optvalue) {
                *value_ptr = OSAL_TRUE;
            }
            else {
                *value_ptr = OSAL_FALSE;
            }
            break;
        case OSAL_NET_SOCK_TYPE:
            if (-1 == (retval = sci_sock_getsockopt(*socket_ptr, SO_TYPE,
                    (void *)&optvalue))) {
                return (OSAL_FAIL);
            }

            if (0 != sci_sock_getsockname(*socket_ptr, &addr)) {
                return (OSAL_FAIL);
            }

            if (AF_INET == addr.sa_family) {
                if (SOCK_DGRAM == optvalue) {
                    *value_ptr = OSAL_NET_SOCK_UDP;
                }
                else if (SOCK_STREAM == optvalue) {
                    *value_ptr = OSAL_NET_SOCK_TCP;
                }
                else {
                    return (OSAL_FAIL);
                }
            }
            else if (AF_INET6 == addr.sa_family) {
                if (SOCK_DGRAM == optvalue) {
                    *value_ptr = OSAL_NET_SOCK_UDP_V6;
                }
                else if (SOCK_STREAM == optvalue) {
                    *value_ptr = OSAL_NET_SOCK_TCP_V6;
                }
                else {
                    return (OSAL_FAIL);
                }
            }
            break; 
        case OSAL_NET_IP_TOS:
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
    if (0 == inet_pton(AF_INET, (const char *)str_ptr, &address_ptr->ipv4)) {
        address_ptr->type = OSAL_NET_SOCK_UDP;
        return (OSAL_SUCCESS);
    }

    /* Not an ipv4 address, try ipv6 */
    if (0 == inet_pton(AF_INET6, (const char *)str_ptr, address_ptr->ipv6)) {
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
        inet_ntop(AF_INET6, address_ptr->ipv6, (char *)str_ptr,
                OSAL_NET_IPV6_STR_MAX);
    }
    else {
        inet_ntop(AF_INET, &address_ptr->ipv4, (char *)str_ptr,
                OSAL_NET_IPV4_STR_MAX);
    }

    return (OSAL_SUCCESS);
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
    /* XXX */
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
        /* XXX */
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
    int retval;

    retval = SSL_Init();
    if (SSL_OK != retval) {
        return (OSAL_FAIL);
    }

    OSAL_memSet(&osalServerlInfo, 0, sizeof(osalServerlInfo));
    
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
    SSL_CALLBACK_T ssl_callback;

    if (NULL == (ssl_ptr->ssl_ptr = SSL_Create((void *)ssl_ptr, 0, OSAL_TRUE))) {
        OSAL_logMsg("%s:%d Failed to SSL_Create().\n", __FILE__, __LINE__);
        return (OSAL_FAIL);
    }

    switch (method) {
        /* Note: At present only SSL_PROTOCOLSSL(SSLv3.0) is supported ! */
        case OSAL_NET_SSL_METHOD_CLIENT_SSLV3:
        case OSAL_NET_SSL_METHOD_CLIENT_TLSV1:
        case OSAL_NET_SSL_METHOD_CLIENT_ALL:
            SSL_ProtocolChoose(ssl_ptr->ssl_ptr, SSL_PROTOCOLSSL, OSAL_TRUE);
            break;
        case OSAL_NET_SSL_METHOD_SERVER_SSLV3:
        case OSAL_NET_SSL_METHOD_SERVER_TLSV1:
        case OSAL_NET_SSL_METHOD_SERVER_ALL:            
        default:
            OSAL_logMsg("%s:%d unsupported ssl method=%d \n", method, __FILE__, __LINE__);
            return (OSAL_FAIL);
    }

    /* Create result message fifo */
    OSAL_fileFifoDelete(OSAL_NET_SSL_FIFO);
    /* currently, there is only one fifo. XXX */
    if (OSAL_FAIL == OSAL_fileFifoCreate(
                OSAL_NET_SSL_FIFO, OSAL_NET_SSL_FIFO_DEPTH,
                sizeof(OSAL_NetSslMsg))) {
        OSAL_logMsg("%s:%d Could not create %s fifo.\n", __FUNCTION__,
                __LINE__, OSAL_NET_SSL_FIFO);
        return (OSAL_FAIL);
    }
    
    /* Set callback */
    ssl_callback.decryptout_cb  = (SecurityDecryptDataOutput)_OSAL_netSslRecvDataCb; 
    ssl_callback.encryptout_cb  = (SecurityEncryptDataOutput)_OSAL_netSslSendDataCb; 
    ssl_callback.postmessage_cb = (SecurityPostMessage)_OSAL_netSslPostMessageCb; 
    ssl_callback.showcert_cb    = (SecurityShowCertInfo)_OSAL_netShowCertInfoCb; 
    SSL_AsyncRegCallback(ssl_ptr->ssl_ptr, &ssl_callback);


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
    /* XXX */
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
    int size = 2000; /* XXX */
    char buf[2000]; /* XXX */
    char ipStr[OSAL_NET_IPV6_STR_MAX];
    OSAL_NetSslMsg msg;

    /* Begin handshake. */
    if (SSL_OK != SSL_HandShake(ssl_ptr->ssl_ptr, ssl_ptr->ipStr,
                ssl_ptr->port, SSL_ASYNC)) {
        OSAL_logMsg("%s:%dfail to ssl handshake\n", __FILE__, __LINE__);        
        return (OSAL_FAIL);
    }
#if 0
    OSAL_netSslReceive(ssl_ptr, buf, &size);

    if(0 < _OSAL_netSslReadDataFromFifo(ssl_ptr, buf, size,
        OSAL_WAIT_FOREVER)) {
        OSAL_memCpy(&msg, buf, sizeof(OSAL_NetSslMsg));
        if (OSAL_NET_SSL_HANDSHAK_SUCCESS == msg.msgType) {
    return (OSAL_SUCCESS);
        }
    }
#endif
    return (OSAL_FAIL);
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
    /* XXX */
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
    if (ssl_ptr->fid) {
        OSAL_fileClose(&ssl_ptr->fid);
        ssl_ptr->fid = 0;
    }
    if (NULL != ssl_ptr->ssl_ptr) {
        OSAL_fileFifoDelete(ssl_ptr->ssl_ptr);
        SSL_Close(ssl_ptr->ssl_ptr, OSAL_TRUE);
        ssl_ptr->ssl_ptr = NULL;
    }
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
    /* XXX */
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
    if ((NULL == ssl_ptr) || (NULL == buf_ptr) ||
            (NULL == size_ptr)) {
        return (OSAL_FAIL);
    }
    
    SSL_EncryptPasser(ssl_ptr->ssl_ptr, buf_ptr, *size_ptr);
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
    vint  bytes;
    OSAL_NetSslMsg *msg_ptr;

    if ((NULL == ssl_ptr) || (NULL == buf_ptr) ||
            (NULL == size_ptr)) {
        return (OSAL_FAIL);
    }

    if(OSAL_SUCCESS != OSAL_netSocketReceive(&ssl_ptr->sockId,
            buf_ptr, &bytes, OSAL_NET_RECV_NO_FLAGS)) {
        return (OSAL_FAIL);
    } 
    if (0 == bytes) {
        return (OSAL_SUCCESS);
    }
    else if (0 > bytes) {
        return (OSAL_FAIL);
    }

    SSL_DecryptPasser(ssl_ptr->ssl_ptr, buf_ptr, bytes);

    if(0 < _OSAL_netSslReadDataFromFifo(ssl_ptr, buf_ptr, *size_ptr,
        OSAL_WAIT_FOREVER)) {
        msg_ptr = (OSAL_NetSslMsg *) buf_ptr;
        if (OSAL_NET_SSL_DE_SUCCESS == msg_ptr->msgType) {
            OSAL_memCpy(buf_ptr, msg_ptr->data, msg_ptr->dataLen);
            return (OSAL_SUCCESS);
        }
    }

    return (OSAL_FAIL);
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
    ssl_ptr->sockId = *socket_ptr;
    _OSAL_netSslSearchServer(ssl_ptr, *socket_ptr);
    return (OSAL_SUCCESS);
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
    /* XXX */
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
    /* XXX */
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
#ifdef OSAL_RESOLVE_A_ONLY
    struct sci_hostent *host_ptr = NULL;
    OSAL_NetAddress     addr;
    char               *ansp_ptr;
    char                ansBuf[OSAL_NET_DNS_ANS_STR_MAX];
    int                 i;
    int                 j;

    /* Use sci_gethostbyname to query hostname. XXX Currently only A. */
    if (OSAL_NET_RESOLVE_A == qtype) {
        if (NULL == (host_ptr = sci_gethostbyname((char *)query_ptr))) {
            OSAL_logMsg("Failed to query ip address of %s\n", query_ptr);
            return (OSAL_FAIL);
        }

        ansp_ptr = (char *)ans_ptr;
        /* 5 for Max */
        for (i = 0; i < _OSAL_NET_DNS_RESULT_MAX; i++) {
            if (NULL ==  host_ptr->h_addr_list[i]) {
                break;
            }

            OSAL_memCpy(&addr.ipv4, host_ptr->h_addr_list[i], host_ptr->h_length);

            if (0 == addr.ipv4) {
                break;
            }
            /* Conver to string. */
            addr.type = OSAL_NET_SOCK_UDP;
            OSAL_netAddressToString((int8 *)ansBuf, &addr);
            j = OSAL_snprintf(ansp_ptr, ansSize - 1, "%s\n", ansBuf);
            /* Check if we have enough room */
            if (OSAL_strlen(ansBuf) >= ansSize) {
                /* Break out of the 'records' processing loop */
                break;
            }
            ansSize -= j;
            ansp_ptr += j;
        }
        OSAL_logMsg("DNS Query Result:%s\n", ans_ptr);
        return (OSAL_SUCCESS);
    }

    return (OSAL_FAIL);
#else
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
    int                 records;
    int                 j;
    ns_rr               rr;
    char                mn[OSAL_NET_DNS_ANS_STR_MAX];
    uint8              *cp_ptr;
    uint32              ip;
    char               *ansp_ptr;
    ns_msg              msg;
    char                ipv6_str[OSAL_NET_IPV6_STR_MAX];
    int                 order;
    int                 preference;
    char                ansBuf[OSAL_NET_DNS_ANS_STR_MAX];
    uint8               flagLength;
    char                flag[8];
    uint8               serviceLength;
    char                service[OSAL_NET_DNS_ANS_STR_MAX];
    TCPIP_IPADDR_T      dnsAddr1;
    TCPIP_IPADDR_T      dnsAddr2;


    /*
     * Check for NULL or invalid string.  Return addr 0x0 if NULL.
     */
    if (NULL == query_ptr) {
        return (OSAL_FAIL);
    }
    if (OSAL_strlen((const char *)query_ptr) < 2) {
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
        OSAL_strncpy((char *)ans_ptr, (const char *)query_ptr, ansSize);
        return (OSAL_SUCCESS);
    }

    /* Get DNS server. XXX need to handle ipv6 case. */
    sci_getdnsbynetid(&dnsAddr1, &dnsAddr2, _globalNetid);

    servAddr.ipv4 = dnsAddr1;
    servAddr.type = OSAL_NET_SOCK_UDP;
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
    if ((OSAL_strlen((const char *)query_ptr) + 32) > ansSize) {
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
            return (OSAL_FAIL);
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

    OSAL_memSet(buf, 0, sizeof(buf));
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
#if 0
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
#endif
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
        ansp_ptr = (char *)ans_ptr;
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

                OSAL_memCpy(addr.ipv6, cp_ptr, sizeof(addr.ipv6));
                addr.type = OSAL_NET_SOCK_UDP_V6;
                /*
                 * Put in ans buffer.  First check and see if we have room.
                 */
                if (OSAL_NET_IPV6_STR_MAX >= ansSize) {
                    /* Break out of the 'records' processing loop */
                    break;
                }

                OSAL_netAddressToString((int8 *)ipv6_str, &addr);
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

        if (ans_ptr == (int8 *)ansp_ptr) {
            return (OSAL_FAIL);
        }
    }

    return (OSAL_SUCCESS);
#endif /* !OSAL_RESOLVE_A_ONLY */
#else
    return (OSAL_FAIL);
#endif
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
    OSAL_NetAddress addr;
    char           *char_ptr;
    char           *pos_ptr;
    char            res_ptr[OSAL_NET_IPV6_STR_MAX];
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
        if (OSAL_SUCCESS == OSAL_netStringToAddress((int8 *)res_ptr, &addr)) {
            address_ptr->port = 0;
            address_ptr->ipv4 = addr.ipv4;
            OSAL_memCpy(address_ptr->ipv6, addr.ipv6, sizeof(addr.ipv6));
            return (OSAL_SUCCESS);
        }
    }
    return (OSAL_FAIL);
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
    else if (OSAL_netHtonl(OSAL_NET_INADDR_LOOPBACK) ==
            addr_ptr->ipv4) {
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

    OSAL_memCpy(dst_ptr, src_ptr, sizeof(OSAL_NetAddress));
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
    if ((OSAL_NET_SOCK_UDP_V6 == addrA_ptr->type) ||
           (OSAL_NET_SOCK_TCP_V6 == addrA_ptr->type)) {
        return (OSAL_TRUE);
    }

    return (OSAL_FALSE);
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
