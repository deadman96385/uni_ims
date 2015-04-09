/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 29962 $ $Date: 2014-11-20 12:36:51 +0800 (Thu, 20 Nov 2014) $
 */

#include "sip_sip.h"
#include "sip_hdrflds.h"
#include "sip_parser_enc.h"
#include "sip_xact.h"
#include "sip_xport.h"
#include "_sip_descr.h"
#include "_sip_drvr.h"
#include "_sip_resolv.h"
#include "sip_mem_pool.h"

static uint16  _TRANSPORT_DefaultUdpPort = 0;
static uint16  _TRANSPORT_DefaultTcpPort = 0;
static vint  _TRANSPORT_Mtu = 0;

/* 
 *****************************************************************************
 * ================TRANSPORT_Init()===================
 *
 * This function MUST be called before any packets can be read (received)
 * from the network.  It set's the callback handlers that will pass the
 * incoming parsed SIP messages.
 *
 * pfClientCb: A pointer to the callback function to handle all incoming 
 *             Client-Side packets (incoming responses)
 *
 * pfServerCb: A pointer to the callback function to handle all incoming 
 *             Server-Side packets (incoming requests)
 *
 * mtu: A value as a criterion. 
 *             If a request sip message lenght larger yhan this value, 
 *             the message to must be sent useing TCP
 *    
 *  
 * RETURNS:
 *     SIP_OK: The transport module was successfully initialized
 *     SIP_FAILED: The transport module was not successfully initialized.
 *
 ******************************************************************************
 */
vint TRANSPORT_Init(
    tpfTRANSPORT_RcvCB pfClientCb,
    tpfTRANSPORT_RcvCB pfServerCb,
    tpfTRANSPORT_RcvCB pfErrorCb,
    uint32             mtu)
{
    _TRANSPORT_DefaultUdpPort = 0;
    _TRANSPORT_DefaultTcpPort = 0;
    _TRANSPORT_Mtu = mtu;
        
    /* Init the sub-module that manages descriptors */
    if (SIP_OK != TRANSDESCR_Init(TRANSDRVR_ProcessData, TRANSDRVR_ProcessErr)) {
        return (SIP_FAILED);
    }
    if (SIP_OK != TRANSDRVR_Init(pfClientCb, pfServerCb, pfErrorCb)) {
        TRANSDESCR_Destroy();
        return (SIP_FAILED);
    }
    return (SIP_OK);
}

/* 
 *****************************************************************************
 * ================TRANSPORT_KillModule()===================
 *
 * This function is used to stop all use of transport objects and descriptors
 * and then "free" all memory back to the heap.
 *
 * NOTE!!!!!   This function MUST be called AFTER TRANS_KillModule().
 * This is because all transactions must be killed before we kill
 * any transport related objects.
 * 
 * RETURNS:
 *     Nothing
 ******************************************************************************
 */
void TRANSPORT_KillModule(void)
{
    /* Kill the sub-module that manages descriptors */
    TRANSDESCR_Destroy();
    /* Kill the sub-module that manages the transport resources */
    TRANSDRVR_KillModule();
}

/* 
 *****************************************************************************
 * ================TRANSPORT_StartServer()===================
 *
 * This function is the main function that will initailize the default 
 * server port(s).  For example port 5060.  It will then start performing the
 * select statement to identify data on any and all active file descriptors
 * NOTE, this function does not return and is bascially the thread handler for 
 * the thread allocated to deal with all network traffic.
 *
 * UdpServerPort = The default port for the SIP stack to receive incoming
 *                network packets.  this is typically 
 *                port #5060 as per SIP RFC3261.  If ZERO, then no UDP
 *                interface will be initialized
 *
 * udpFd: The file descriptor of the default UDP port to use.  If ZERO, then 
 *        the SIP Stack will create and bind an IP Stack socket to port 
 *        "UdpServerPort".  Otherwise, if NOT-ZERO, then the SIP Stack will 
 *        make NO atempt to create and bind a socket.  In this case, creating 
 *        and binding the socket to use as the default UDP interface is up to 
 *        the application.
 *
 * TcpServerPort = The default port for the SIP stack to listen for incoming
 *                TCP connections.  this is typically port #5060 as per 
 *                SIP RFC3261.  If ZERO, then no TCP interface will be 
 *                initialized.
 *
 * tcpFd = The file descriptor of the default TCP port to use.  If ZERO, then 
 *        the SIP Stack will create and bind an IP Stack socket to port 
 *        "tcpServerPort".  Otherwise, if NOT-ZERO, then the SIP Stack will 
 *        make NO atempt to create and bind a socket.  In this case, creating 
 *        and binding the socket to use as the default UDP interface is up to 
 *        the application.
 *
 * RETURNS:
 *     Nothing and NEVER
 *
 ******************************************************************************
 */
void TRANSPORT_StartServer(
    uint16         UdpServerPort,
    OSAL_NetSockId udpFd,
    uint16         TcpServerPort,
    OSAL_NetSockId tcpFd) 
{
    tDescr         *pDescr;
    OSAL_NetAddress addr;
    
    /* UDP */
    if (UdpServerPort != 0) {
        /* put it in network byte order */
        addr.port = OSAL_netHtons(UdpServerPort);
        addr.ipv4 = OSAL_NET_INADDR_ANY;
        addr.type = OSAL_NET_SOCK_UDP;
        /*
         * XXX Currently server sockets are not used. Leave it in the future.
         */
        if (NULL == (pDescr = TRANSDESCR_Alloc(udpFd, &addr,
                eTRANSPORT_REUSE, eTransportUdp, NULL))) {
            return;
        }
        
        if (0 == udpFd) {
            /* Then there's no socket yet, create one */
            if (TRANSDESCR_Create(pDescr, &addr) != SIP_OK) {
                TRANSDESCR_Free(pDescr);
                return;
            }
        }
        TRANSDESCR_Enable(pDescr);
        /* Store network byte order server port. */
        _TRANSPORT_DefaultUdpPort = addr.port;
    }
    
    /* TCP */
    if (TcpServerPort != 0) {
        /* put it in network byte order */
        addr.port = OSAL_netHtons(TcpServerPort);
        addr.ipv4 = OSAL_NET_INADDR_ANY;
        addr.type = OSAL_NET_SOCK_TCP;
        
        /*
         * XXX Currently server sockets are not used. Leave it in the future.
         */
        if (NULL == (pDescr = TRANSDESCR_Alloc(tcpFd, &addr,
                eTRANSPORT_REUSE, eTransportTcp, NULL))) {
            return;
        }
        
        if (0 == tcpFd) {
            /* Then there's no socket yet, create one */
            if (TRANSDESCR_CreateTcpListen(pDescr) != SIP_OK) {
                TRANSDESCR_Free(pDescr);
                return;
            }
        }
        TRANSDESCR_Enable(pDescr);
        _TRANSPORT_DefaultTcpPort = TcpServerPort;
    }

    for (;;) {
        TRANSDESCR_Recv();
    }
}

/* 
 *****************************************************************************
 * ================TRANSPORT_DescriptorSwitch()===================
 *
 * This function will disable/enable a file descriptor used for reading
 * data from the network interface thread.
 * NOTE, the file descriptor must be the persistent one that is always
 * used to service the network interface thread. 
 *
 * fd = The file descriptor that the user wants to enable or disable
 *
 * onOff = '1' = enable, '0' = disable, '-1' disable all.
 *
 * RETURNS:
 *     SIP_OK: The file descriptor was successfully enabled/disabled
 *     SIP_NOT_FOUND: The file descriptor specified in 'fd' was invalid
 *
 ******************************************************************************
 */
vint TRANSPORT_DescriptorSwitch(
    OSAL_NetSockId  fd, 
    vint            onOff)
{
    if (TRANSDESCR_Switch(fd, onOff) != SIP_OK) {
        SIP_DebugLog(SIP_DB_TRANSPORT_LVL_1,
                "TRANSPORT_DescriptorSwitch: Failed fd:%d in not valid", 
                (int)fd, 0, 0);
        return (SIP_NOT_FOUND);
    }
    return (SIP_OK);
}

/* 
 *****************************************************************************
 * ================TRANSPORT_ReplaceUdpDefault()===================
 *
 * This function will replace the existing default UDP server interface
 * with the specified interface. Note, if there are transactions currently 
 * using the existing default interface, they wil continue to use that 
 * interface until the transaction terminates.
 *
 * newUdpFd = The file descriptor of the new UDP interface to switch to.
 *
 * newUdpPort = The port of the new interface to switch to.
 *
 * RETURNS:
 *     SIP_OK: The interface was successfully switched 
 *     SIP_FAILED: The interface was no switched.
 *
 ******************************************************************************
 */
vint TRANSPORT_ReplaceUdpDefault(
    OSAL_NetSockId  newUdpFd,
    uint16          newUdpPort)
{
    newUdpPort = OSAL_netHtons(newUdpPort);

    if (SIP_OK != TRANSDESCR_ReplaceInterace(_TRANSPORT_DefaultUdpPort,
            eTransportUdp, newUdpFd, newUdpPort)) {
        SIP_DebugLog(SIP_DB_TRANSPORT_LVL_1,
            "TRANSPORT_ReplaceUdpDefault: Failed -fd:%d -port:%d", 
            (int)newUdpFd, newUdpPort, 0);
        return (SIP_FAILED);
    }
    /*
     * If we are here then everything worked out.
     * Let's update hte default UDP port value.
     */
    _TRANSPORT_DefaultUdpPort = newUdpPort;
    return (SIP_OK);
}

vint TRANSPORT_replaceServerSocket(
    OSAL_NetSockId   fd,
    OSAL_NetAddress *pAddr,
    tTransportType   tType,
    tNwAccess       *nwAccess_ptr)
{
    OSAL_NetAddress addr;
    uint16 *oldPort_ptr;

    OSAL_netAddrPortHton(&addr, pAddr);

    oldPort_ptr = NULL;
    if (eTransportUdp == tType) {
        oldPort_ptr = &_TRANSPORT_DefaultUdpPort;
    }
    else if (eTransportTcp == tType) {
        oldPort_ptr = &_TRANSPORT_DefaultTcpPort;
    }

    if (SIP_OK != TRANSDESCR_replaceServerSocket(fd, &addr, tType, nwAccess_ptr,
            *oldPort_ptr)) {
        SIP_DebugLog(SIP_DB_TRANSPORT_LVL_1,
            "TRANSPORT_replaceServerSocket: Failed addr:", 0, 0, 0);
        SIP_DebugLogAddr(SIP_DB_TRANSPORT_LVL_1, &addr);
        return (SIP_FAILED);
    }
    /*
     * If we are here then everything worked out.
     * Let's update hte default port value.
     */
    *oldPort_ptr = addr.port;

    return (SIP_OK);
}

/* 
 *****************************************************************************
 * ================TRANSPORT_ClientAlloc()===================
 *
 * This function allocates a transport resource from the server pool. 
 *
 * pUri = A pointer to a tUri object that contains the URI of the remote 
 *        device that the Client-Side is trying to send a request to.
 *
 * pLclConn = A pointer to a tLocalIpConn object that describes what
 *            local IP Stack interface should be used for this particular 
 *            Client-Side transaction.
 * RETURNS:
 *      tSipHandle: A handle to the newly allocated transport resource.
 *      NULL:   Could not allocate the resource.  This could also be 
 *                  because there was an error with the underlying IP
 *                  layer software.  i.e. UDP socket could not be 
 *                  opened, etc...
 *
 ******************************************************************************
 */
tSipHandle TRANSPORT_ClientAlloc(
    tUri            *pUri,
    tLocalIpConn    *pLclConn,
    tTransportType   type)
{
    tTransport    *pTrans;
    tDescr        *pDescr;
    tTransportType transport;
        
    transport = eTransportNone;
    if (NULL == (pTrans = TRANSDRVR_CreateTrans())) {
        return (NULL);
    }

    if (pLclConn) {
        /* copy the local Interface info */
        pTrans->lclConn.fd = pLclConn->fd;
        pTrans->lclConn.rport = pLclConn->rport;
        /* place the address and port in network byte order */
        pTrans->lclConn.addr.type = pLclConn->addr.type;
        pTrans->lclConn.nwAccess = pLclConn->nwAccess;
        if (OSAL_TRUE == OSAL_netIsAddrIpv6(&pLclConn->addr)) {
            OSAL_netIpv6Hton(pTrans->lclConn.addr.ipv6, pLclConn->addr.ipv6);
        }
        else {
            pTrans->lclConn.addr.ipv4 = OSAL_netHtonl(pLclConn->addr.ipv4);
        }

        pTrans->lclConn.addr.port = OSAL_netHtons(pLclConn->addr.port);

        SIP_DebugLog(SIP_DB_TRANSPORT_LVL_3, "TRANSPORT_ClientAlloc: Using "
                "Local connection fd:%d addr:", pLclConn->fd, 0, 0);
        SIP_DebugLogAddr(SIP_DB_TRANSPORT_LVL_3, &pTrans->lclConn.addr);
    }

    if (TRANSPORT_GetHostByUri(pTrans, pUri, TRUE, &transport) != SIP_OK) {
        TRANSPORT_Dealloc(pTrans);
        return (NULL);
    }

    /* If there is no specific transport type, use the default value. */
    if (eTransportNone != type) {
        transport = type;
    }
    SIP_DebugLog(SIP_DB_TRANSPORT_LVL_3,
            "TRANSPORT_ClientAlloc: init resrc transport=%d, rmtAddr:",
            transport, 0, 0);
    SIP_DebugLogAddr(SIP_DB_TRANSPORT_LVL_3, &pTrans->rmtConn.addrSet[0]);

    /* 
     * Otherwise if pLclConn is NULL then the pTrans->lclConn 
     * will be all zero.  This will force the use of the default 
     * server port.
     */
    switch (transport) {
        case eTransportUdp:
            /* If there is no udp port specified then assume the default */
            if (!pTrans->lclConn.addr.port) {
                pTrans->lclConn.addr.port = _TRANSPORT_DefaultUdpPort;
            }

            /* Check if the application specified the descriptor to use. */
            if (pTrans->lclConn.fd != 0) {
                /* See if we already know about it, if so then use that */
                if (NULL == (pDescr = TRANSDESCR_MapFd(pTrans->lclConn.fd))) {
                    /* Then we don't know about it, allocate a new one */
                    if (NULL == (pDescr = TRANSDESCR_Alloc(
                            pTrans->lclConn.fd,
                            &pTrans->lclConn.addr,
                            eTRANSPORT_DONTCLOSESOCKET,
                            eTransportUdp,
                            &pTrans->lclConn.nwAccess))) {
                        TRANSPORT_Dealloc(pTrans);
                        return (NULL);
                    }
                    pTrans->pDescr = pDescr;
                    TRANSDESCR_Enable(pDescr);
                }
                else {
                    pTrans->pDescr = pDescr;
                }
            }
            else {
                /* We always gives fd for connection, if no must be some problem */
                SIP_DebugLog(SIP_DB_TRANSPORT_LVL_1,
                        "TRANSPORT_ClientAlloc: Failed, no fd, local address:",
                        0, 0, 0);
                SIP_DebugLogAddr(SIP_DB_TRANSPORT_LVL_3, &pTrans->lclConn.addr);
                TRANSPORT_Dealloc(pTrans);
                return (NULL);
            }
            break;
        case eTransportTcp:
            /* Check if the application specified the descriptor to use. */
            if (pTrans->lclConn.fd != 0) {
                /* See if we already know about it, if so then use that */
                if (NULL == (pDescr = TRANSDESCR_MapFd(pTrans->lclConn.fd))) {
                    /* Then we don't know about it, allocate a new one */
                    if (NULL == (pDescr = TRANSDESCR_Alloc(
                            pTrans->lclConn.fd,
                            &pTrans->lclConn.addr,
                            eTRANSPORT_DONTCLOSESOCKET,
                            eTransportTcp,
                            &pTrans->lclConn.nwAccess))) {
                        TRANSPORT_Dealloc(pTrans);
                        return (NULL);
                    }

                    pDescr->rmtAddr = pTrans->rmtConn.addrSet[0];

                    if (TRANSDESCR_TcpConn(pDescr, &pTrans->lclConn.addr) != SIP_OK) {
                        TRANSDESCR_Free(pDescr);
                        TRANSPORT_Dealloc(pTrans);
                        return (NULL);
                    }

                    pTrans->pDescr = pDescr;
                    TRANSDESCR_Enable(pDescr);
                }
                else {
                    pTrans->pDescr = pDescr;
                }
            }
            else {
                /* We always gives fd for connection, if no must be some problem */
                SIP_DebugLog(SIP_DB_TRANSPORT_LVL_1,
                        "TRANSPORT_ClientAlloc: Failed, no fd, local address:",
                        0, 0, 0);
                SIP_DebugLogAddr(SIP_DB_TRANSPORT_LVL_3, &pTrans->lclConn.addr);
                TRANSPORT_Dealloc(pTrans);
                return (NULL);
            }
            break;
        case eTransportTls:
            /* Check if the application specified the descriptor to use. */
            if (pTrans->lclConn.fd != 0) {
                /* See if we already know about it, if so then use that */
                if (NULL == (pDescr = TRANSDESCR_MapFd(pTrans->lclConn.fd))) {
                    /* Then we don't know about it, allocate a new one */
                    if (NULL == (pDescr = TRANSDESCR_Alloc(
                            pTrans->lclConn.fd,
                            &pTrans->lclConn.addr,
                            eTRANSPORT_DONTCLOSESOCKET,
                            eTransportTls,
                            &pTrans->lclConn.nwAccess))) {
                        TRANSPORT_Dealloc(pTrans);
                        return (NULL);
                    }
                    pDescr->rmtAddr = pTrans->rmtConn.addrSet[0];
                    /* Let's connect */
                    if (TRANSDESCR_TlsConn(pDescr) != SIP_OK) {
                        TRANSDESCR_Free(pDescr);
                        TRANSPORT_Dealloc(pTrans);
                        return (NULL);
                    }

                    pTrans->pDescr = pDescr;
                    TRANSDESCR_Enable(pDescr);
                }
                else {
                    /* Found mapped fd */
                    pTrans->pDescr = pDescr;
                }
            }
            else {
                /* We always gives fd for connection, if no must be some problem */
                SIP_DebugLog(SIP_DB_TRANSPORT_LVL_1,
                        "TRANSPORT_ClientAlloc: Failed, no fd, local address:",
                        0, 0, 0);
                SIP_DebugLogAddr(SIP_DB_TRANSPORT_LVL_3, &pTrans->lclConn.addr);
                TRANSPORT_Dealloc(pTrans);
                return (NULL);
            }
            break;
        default:
            return (NULL);
    } /* End of switch */
    
    return (pTrans);
}

/* 
 *****************************************************************************
 * ================TRANSPORT_ServerAlloc()===================
 *
 * This function allocates a transport resource from the server pool. 
 *
 * pLocalConn = A pointer to a tLocalIpConn object that describes what
 *            local IP Stack interface should be used for this particular 
 *            Server-Side transaction.  This is typically the same 
 *            local interface that was used when the originating request was 
 *            received.  For example if a request was recevied on port 5060
 *            Then this object will contain the transport resource information
 *            that describes the 5060 IP interface.
 *
 * pRemoteConn = A pointer to tRemoteIpConn object that describes the remote
 *            connection information.
 *
 * transType = The type of Server transaction (i.e.UDP or TCP)
 *
 * RETURNS:
 *      tSipHandle: A handle to the newly allocated transport resource.
 *      NULL:   Could not allocate the resource.  This could also be 
 *                  because there was an error with the underlying IP
 *                  layer software.  i.e. a UDP socket closed, etc...
 *
 ******************************************************************************
 */
tSipHandle TRANSPORT_ServerAlloc(
     tLocalIpConn   *pLocalConn, 
     tRemoteIpConn  *pRemoteConn,
     tTransportType  transType)
{
    tTransport *pTrans;
    tDescr     *pDescr;

    if (NULL == (pTrans = TRANSDRVR_CreateTrans())) {
        return (NULL);
    }    

    pTrans->lclConn = *pLocalConn;
    pTrans->rmtConn = *pRemoteConn;
    
    switch (transType) {
        case eTransportUdp:
            /* Check if the application specified the descriptor to use. */
            if (pTrans->lclConn.fd != 0) {
                /* See if we already know about it, if so then use that */
                if (NULL == (pDescr = TRANSDESCR_MapFd(pTrans->lclConn.fd))) {
                    return (NULL);
                }
                pTrans->pDescr = pDescr;
            }
            else {
                if (NULL == (pDescr = TRANSDESCR_MapPort(pTrans->lclConn.addr.port,
                        eTransportUdp))) {
                    return (NULL);
                }
                pTrans->pDescr = pDescr;
            }
            break;
            
        case eTransportTcp:
            /* Check if the application specified the descriptor to use. */
            if (pTrans->lclConn.fd != 0) {
                /* See if we already know about it, if so then use that */
                if (NULL == (pDescr = TRANSDESCR_MapFd(pTrans->lclConn.fd))) {
                    return (NULL);
                }
                pTrans->pDescr = pDescr;
            }
            else {
                if (NULL == (pDescr = TRANSDESCR_MapRemoteIp(
                        pTrans->rmtConn.addrSet, 1,
                        eTransportTcp, eTRANSPORT_PERSIST_SERVER))) {
                    /* 
                     * The TCP server must already exist, if not then we have a 
                     * serious issue jsut return error. 
                     */
                    return (NULL);
                }
                pTrans->pDescr = pDescr;
            }
            break;
        case eTransportTls:
            /* Check if the application specified the descriptor to use. */
            if (pTrans->lclConn.fd != 0) {
                /* See if we already know about it, if so then use that */
                if (NULL == (pDescr = TRANSDESCR_MapFd(pTrans->lclConn.fd))) {
                    return (NULL);
                }
                pTrans->pDescr = pDescr;
            }
            else {
                if (NULL == (pDescr = TRANSDESCR_MapRemoteIp(
                        pTrans->rmtConn.addrSet, 1,
                        eTransportTls, eTRANSPORT_REUSE))) {
                    /* 
                     * The TLS server connection will use the same 
                     * connection/descriptor as the TLS client connection.
                     * So if this doesn't exist then we have a serious issue. 
                     */
                    return (NULL);
                }
                pTrans->pDescr = pDescr;
            }
            break;
        default:
            return (NULL);
    } /* End of switch */
    
    SIP_DebugLog(SIP_DB_TRANSPORT_LVL_1,
            "TRANSPORT_ServerAlloc: Successfull -hTransport:%X type:%d", 
            (int)pTrans, transType, 0);
    
    return (pTrans);
}
    

/* 
 *****************************************************************************
 * ================TRANSPORT_AddUser()===================
 *
 * This function increases the number of users for a transport 
 * resource.
 * 
 * hTransport = A handle to a transport object 
 * 
 * RETURNS:
 *      Nothing
 *
 ******************************************************************************
 */
void TRANSPORT_AddUser(
    tSipHandle hTransport)
{
    tTransport *pTrans = (tTransport*)hTransport;
    pTrans->numUsers++;
}

/* 
 *****************************************************************************
 * ================TRANSPORT_Dealloc()===================
 *
 * This function returns a transport resource ("frees") back to the pool 
 * it came from.
 * 
 * hTransport = A handle to a transport object 
 * 
 * RETURNS:
 *      SIP_OK:     The resource was successfully returned.
 *      SIP_BADPARM:   The hTransport value was NULL (invalid)
 *
 ******************************************************************************
 */
vint TRANSPORT_Dealloc(
    tSipHandle hTransport)
{
    tTransport *pTrans = (tTransport*)hTransport;
    
    if (!pTrans) {
        SIP_DebugLog(SIP_DB_TRANSPORT_LVL_1,
                "TRANSPORT_Dealloc: FAILED!  hTransport is NULL", 0, 0, 0);
        return (SIP_BADPARM);
    }
    
    SIP_DebugLog(SIP_DB_TRANSPORT_LVL_3,
            "TRANSPORT_Dealloc: -hTransport: %X, numUsers:%d", 
        (int)pTrans, pTrans->numUsers, 0);
    
    if (pTrans->numUsers <= 1 && pTrans->pDescr) {
        /* Kill/free the descriptor */
        TRANSDESCR_Disable(pTrans->pDescr, 0);
    }
    
    /* Now free the transport resource */
    TRANSDRVR_DestroyTrans(pTrans);
    return (SIP_OK);
}

/* 
 *****************************************************************************
 * ================TRANSPORT_Send()===================
 * 
 * This function encodes and sends a SIP message to the network via 
 * the transport resource specified by hTransport.
 * 
 * hTransport = A handle to the transport resource to be used to send the msg
 *
 * pMsgToSend = A pointer to the SIP message object to send to the network.
 * 
 * RETURNS:
 *      SIP_OK:               The message was successfully sent
 *      SIP_BADPARM:          hTransport was invalid
 *      SIP_RESOURCE_UNAVAIL: The transport resource is not ready
 *      SIP_FAILED:           The message could not be sent due to some issue
 *                            with the underlying IP software layer.
 *
 ******************************************************************************
 */
vint TRANSPORT_Send(
    tSipHandle   hTransport, 
    tSipIntMsg  *pMsgToSend)
{
    vint             status;
    tViaHFE         *pVia;
    tContactHFE     *pContact;
    tL4Packet        pkt;
    vint             bufferLen;
    tLayer4Buffer   *pLayer4Buf;
    char            *pBuffer;
    tTransport      *pTrans;
    tDLListEntry    *pEntry;
    OSAL_NetAddress *pAddr;
    tDescr          *pDescr;
    OSAL_NetAddress  addr;

    pTrans = (tTransport*) hTransport;
    SIP_DebugLog(SIP_DB_TRANSPORT_LVL_2,
            "TRANSPORT_Send: hTransport=%X, pMsg=%X",
            (int)pTrans, (int)pMsgToSend, 0);

    if (!pTrans) {
        SIP_DebugLog(SIP_DB_TRANSPORT_LVL_1,
                "TRANSPORT_Send: hTransport=NULL!", 0, 0, 0);
        return (SIP_BADPARM);
    }
    if (pTrans->pDescr->isEnabled == 0) {
        SIP_DebugLog(SIP_DB_TRANSPORT_LVL_1,
                "TRANSPORT_Send: tDescr isEnabled:0", 0, 0, 0);
        return (SIP_RESOURCE_UNAVAIL);
    }
    if (pTrans->numUsers == 0) {
        SIP_DebugLog(SIP_DB_TRANSPORT_LVL_1,
                "TRANSPORT_Send: hTransport numUsers:0", (int)pTrans, 0, 0);
        return (SIP_RESOURCE_UNAVAIL);
    }

    /* Populate P-ANI except CANCEL and ACK. */
    if ((eNwAccessTypeNone != pTrans->lclConn.nwAccess.type) &&
            (eSIP_CANCEL != pMsgToSend->method) &&
            (eSIP_ACK != pMsgToSend->method)) {
        HF_SetPresence(&pMsgToSend->x.ECPresenceMasks,
                eSIP_P_ACCESS_NW_INFO_HF);
        pMsgToSend->nwAccess = pTrans->pDescr->nwAccess;
    }
    else {
        HF_ClrPresence(&pMsgToSend->x.ECPresenceMasks,
                eSIP_P_ACCESS_NW_INFO_HF);
    }

    /* Make sure the via protocol type is correctly
     * populated to match the protocol used. This is done for
     * backwards compatibity with RFC2543 as well as compliance with RFC 3263.
     * See TRANSPORT_ClientAlloc().
     */
    pVia = NULL;
    if (HF_PresenceExists(&pMsgToSend->x.ECPresenceMasks, eSIP_VIA_HF)) {
        pEntry = NULL;
        if (DLLIST_GetNext(&pMsgToSend->ViaList, &pEntry)) {
            pVia = (tViaHFE *)pEntry;
            pVia->uri.transport = pTrans->pDescr->type;
            if (pMsgToSend->msgType == eSIP_REQUEST) {

                if (pTrans->lclConn.addr.port != 
                        OSAL_netHtons(SIP_DEFAULT_IPADDR_PORT)) {
                    if (0 == pTrans->lclConn.addr.port) {
                        /*
                         * Then the port is zero, populate the port that is being
                         * used for this descr.
                         */
                        pVia->uri.host.port = OSAL_netNtohs(pTrans->pDescr->lclAddr.port);
                    }
                    else {
                        pVia->uri.host.port = OSAL_netNtohs(pTrans->lclConn.addr.port);
                    }

                }
                if (pTrans->lclConn.rport) {
                    /* a 'one' will trigger to include the rport param
                     * but not to populate it
                     */
                    pVia->rport = 1;
                }
#if 0 /* YTL disable */
                if (pTrans->lclConn.addr.ul != 0) {
                    pVia->uri.host.addressType = eNwAddrIPv4;
                    pVia->uri.host.x.ipV4.ul =
                            OSAL_netNtohl(pTrans->lclConn.addr.ul);
                }
#endif
            }
            else {
                /* it's a response */
                if (pTrans->lclConn.rport) {
                    pVia->rport = 
                            OSAL_netNtohs(pTrans->rmtConn.addrSet[0].port);
                }
            }
        }
    }
    /* If the local connection IP address is populated then place 
     * it in the contact HF as well 
     */
    if (OSAL_netIsAddrIpv6(&pTrans->lclConn.addr) &&
            !OSAL_netIpv6IsAddrZero(pTrans->lclConn.addr.ipv6)) {
        /* ipv6 and address is not zero */
        if (HF_PresenceExists(&pMsgToSend->x.ECPresenceMasks, eSIP_CONTACT_HF)) {
            pEntry = NULL;
            if (DLLIST_GetNext(&pMsgToSend->ContactList, &pEntry)) {
                pContact = (tContactHFE *)pEntry;
                /* If the address type is domain name, do not change to IP. */
                if (eNwAddrDomainName != pContact->uri.host.addressType) {
                    pContact->uri.host.addressType = eNwAddrIPv6;
                    OSAL_netIpv6Ntoh(pContact->uri.host.x.ip.v6, pTrans->lclConn.addr.ipv6);
                    if (pTrans->lclConn.addr.port != 0) {
                        pContact->uri.host.port = OSAL_netNtohs(pTrans->lclConn.addr.port);
                    }
                    pContact->uri.transport = pTrans->pDescr->type;
                }
            }
        }
    }
    else if (!OSAL_netIsAddrIpv6(&pTrans->lclConn.addr) &&
            0 != pTrans->lclConn.addr.ipv4) {
        /* ipv4 and address is not zero*/
        if (HF_PresenceExists(&pMsgToSend->x.ECPresenceMasks, 
                eSIP_CONTACT_HF)) {
            pEntry = NULL;
            if (DLLIST_GetNext(&pMsgToSend->ContactList, &pEntry)) {
                pContact = (tContactHFE *)pEntry;
                /* If the address type is domain name, do not change to IP. */
                if (eNwAddrDomainName != pContact->uri.host.addressType) {
                    pContact->uri.host.addressType = eNwAddrIPv4;
                    pContact->uri.host.x.ip.v4.ul = 
                            OSAL_netNtohl(pTrans->lclConn.addr.ipv4);
                    if (pTrans->lclConn.addr.port != 0) {
                        pContact->uri.host.port = 
                                OSAL_netNtohs(pTrans->lclConn.addr.port);
                    }
                    else {
                        /*
                         * Then the port is zero, populate the port that is being
                         * used for this descr.
                         */
                        pContact->uri.host.port =
                                OSAL_netNtohs(pTrans->pDescr->lclAddr.port);
                    }
                    pContact->uri.transport = pTrans->pDescr->type;
                }
            }
        }
    }
    else {
        /* address is zero */
        if (HF_PresenceExists(&pMsgToSend->x.ECPresenceMasks,
                eSIP_CONTACT_HF)) {
            /* Don't modify Contact for call forward */
            if (eSIP_RSP_MOVED_TEMP != pMsgToSend->code) {
                pEntry = NULL;
                if (DLLIST_GetNext(&pMsgToSend->ContactList, &pEntry)) {
                    pContact = (tContactHFE *)pEntry;
                    pContact->uri.host.port =
                            OSAL_netNtohs(pTrans->pDescr->lclAddr.port);
                    pContact->uri.transport = pTrans->pDescr->type;
                }
            }
        }
    }

    pLayer4Buf = (tLayer4Buffer *)SIP_memPoolAlloc(eSIP_OBJECT_LAYER_4_BUF);
    if (!pLayer4Buf) {
        return (SIP_NO_MEM);
    }

    pBuffer = pLayer4Buf->buf;
    pkt.frame = (tSipHandle)pBuffer;
    pkt.length = SIP_MAX_LAYER_4_PACKET_SIZE;
    pkt.pCurr = pBuffer;
    pkt.pStart = pBuffer;
    pkt.isOutOfRoom = FALSE;
    
    status = ENC_Msg(pMsgToSend, &pkt, pMsgToSend->isCompactForm);
    if (status != SIP_OK) {
        SIP_memPoolFree(eSIP_OBJECT_LAYER_4_BUF, (tDLListEntry *)pLayer4Buf);
        SIP_DebugLog(SIP_DB_TRANSPORT_LVL_1,
                "TRANSPORT_Send ENC_Msg failed...status: %d\n", status, 0, 0);
        return (SIP_FAILED);
    }

    bufferLen = EXTBUF_LEN(&pkt);

    /*
     * A request MUST be sent using TCP if sip message is larger than
     * _TRANSPORT_Mtu.
     * If _TRANSPORT_Mtu is set to 0, disalbe this feature.
     */
    SIP_DebugLog(SIP_DB_TRANSPORT_LVL_2,
            "TRANSPORT_Send: bufferLen=%d", bufferLen, 0, 0);
    if ((eTransportUdp == pTrans->pDescr->type) &&
            (_TRANSPORT_Mtu) && (_TRANSPORT_Mtu < bufferLen) &&
            (pMsgToSend->msgType == eSIP_REQUEST)) {
        /*
         * Change the transport protocol in the top Via.
         */
        pVia->uri.transport = eTransportTcp;
        pkt.pCurr = pBuffer;
        pkt.pStart = pBuffer;
        status = ENC_Msg(pMsgToSend, &pkt, pMsgToSend->isCompactForm);
        if (status != SIP_OK) {
            SIP_memPoolFree(eSIP_OBJECT_LAYER_4_BUF,
                    (tDLListEntry *)pLayer4Buf);
            SIP_DebugLog(SIP_DB_TRANSPORT_LVL_1,
                    "TRANSPORT_Send ENC_Msg failed...status: %d\n", status, 0, 0);
            return (SIP_FAILED);
        }

        /* See if we already know about it, if so then use that */
        OSAL_netAddrPortCpy(&addr, &pTrans->lclConn.addr);
        if (OSAL_netIsAddrIpv6(&addr)) {
            addr.type = OSAL_NET_SOCK_TCP_V6;
        }
        else {
            addr.type = OSAL_NET_SOCK_TCP;
        }

        if (NULL == (pDescr = TRANSDESCR_MapLocalIp(&addr, eTransportTcp,
                eTRANSPORT_DONTCLOSESOCKET))) {
            SIP_DebugLog(SIP_DB_TRANSPORT_LVL_1,
                    "TRANSPORT_Send TRANSDESCR_MapLocalIp failed.\n", 0, 0, 0);
            return (SIP_FAILED);
        }
        if (pDescr->isEnabled) {
            /* Found mapped fd */
            pTrans->pDescr = pDescr;
        }
        else {
             pDescr->rmtAddr = pTrans->rmtConn.addrSet[0];
             if (TRANSDESCR_TcpConn(pDescr, &pTrans->lclConn.addr) != SIP_OK) {
                TRANSDESCR_Free(pDescr);
                 return (SIP_FAILED);
             }
             pTrans->pDescr = pDescr;
             TRANSDESCR_Enable(pDescr);
        }
    }

    switch (pTrans->pDescr->type) {
        case eTransportUdp:
            /* must be udp */
            pAddr = &pTrans->rmtConn.addrSet[pTrans->rmtConn.addrIdx];
            status = TRANSDESCR_UdpSend(pTrans->pDescr, pAddr, pBuffer,
                    bufferLen);
            if (status != SIP_OK && pMsgToSend->msgType == eSIP_RESPONSE) {
                /* Then we try one more time after we attempt to resolve the via 
                 * uri.  See section 5 of rfc 3263 
                 */
                SIP_DebugLog(SIP_DB_TRANSPORT_LVL_1,
                        "TRANSPORT_Send: OUTGOING (UDP) MSG retry !!!",
                        0, 0, 0);
                
                if (pVia) {
                    if (SIP_OK == (status = 
                            TRANSPORT_GetHostByUri (pTrans, &pVia->uri, FALSE,
                            &pVia->uri.transport))) {
                        status = TRANSDESCR_UdpSend(pTrans->pDescr, pAddr, 
                                pBuffer, bufferLen);
                    }
                }
            }
            break;
        case eTransportTcp:
            status = TRANSDESCR_TcpSend(pTrans->pDescr, pBuffer, bufferLen);
            break;
        case eTransportTls:
            status = TRANSDESCR_TlsSend(pTrans->pDescr, pBuffer, bufferLen);
            break;
        default:
            status = SIP_BADPARM;
            break;
    }
        
    if (status != SIP_OK) {
        SIP_DebugLog(SIP_DB_TRANSPORT_LVL_1,
                "TRANSPORT_Send: failed status:%d", status, 0, 0);
    }

    /* dealloc the buffer */
    SIP_memPoolFree(eSIP_OBJECT_LAYER_4_BUF, (tDLListEntry *)pLayer4Buf);
    return (status);
}

/* 
 *****************************************************************************
 * ================TRANSPORT_GetTransportType()===================
 *
 * This function returns a transport type i.e. "UDP", "TCP", "TLS" 
 * 
 * hTransport = A handle to a transport object 
 * 
 * RETURNS:
 *      eTransportUdp
 *      eTransportTcp
 *      eTransportTls
 *
 ******************************************************************************
 */
tTransportType TRANSPORT_GetTransportType(
    tSipHandle hTransport)
{
    tTransport *pTrans = (tTransport*)hTransport;
    if (!hTransport)
        return (eTransportNone);
    return (pTrans->pDescr->type);
}

/* 
 *****************************************************************************
 * ================TRANSPORT_GetTransportAddr()===================
 *
 * This function returns a the remote address and fd associated with a 
 * transport resource.
 * 
 * hTransport = A handle to a transport object 
 * 
 * RETURNS:
 *      SIP_OK: The values were succesfully returned in the pAddr and pFd
 *              parameters.
 *      SIP_FAILED: There is no remote address set for this transport resource.
 ******************************************************************************
 */
vint TRANSPORT_GetTransportAddr(
    tSipHandle       hTransport,
    OSAL_NetAddress *pAddr,
    OSAL_NetSockId  *pFd)
{
    OSAL_NetAddress  *pA;
    tTransport       *pTrans;
    
    pTrans = (tTransport*)hTransport;
    pA = &pTrans->rmtConn.addrSet[pTrans->rmtConn.addrIdx];
    if (0 == pA->port || OSAL_netIsAddrZero(pA)) {
        return (SIP_FAILED);
    }
    *pAddr = *pA;
    *pFd = pTrans->pDescr->fd;
    return (SIP_OK);
}

/* 
 *****************************************************************************
 * ================TRANSPORT_SetLocalConn()===================
 *
 * This function sets up a local connection object associated with a transport 
 * resource.
 * 
 * hTransport = A handle to a transport object 
 *
 * pLclConn = A pointer to the lcoal connection object to copy to the transport
 *            resource. 
 * 
 * RETURNS:
 *      Nothing.
 ******************************************************************************
 */
void TRANSPORT_SetLocalConn(
    tSipHandle    hTransport, 
    tLocalIpConn *pLclConn)
{
    tTransport *pTrans;
    if (hTransport && pLclConn) { 
        pTrans = (tTransport*)hTransport;
        /* place the address and port in network byte order */
        OSAL_netAddrPortHton(&pTrans->lclConn.addr, &pLclConn->addr);
        pTrans->lclConn.rport = pLclConn->rport;
        /* DO NOT COPY THE FD HERE */
    }
    return;
}

/* 
 *****************************************************************************
 * ================TRANSPORT_AdvanceRmtAddr()===================
 *
 * This function advances an index that indicates which remote IP address value
 * inside an array is the current IP address of the remote party 
 * 
 * hTransport = A handle to a transport object 
 *
 * RETURNS:
 *      SIP_OK: The index was advanced.
 *      SIP_FAILED: No more advancing is possible, the index is at the end of 
 *                  the array.
 ******************************************************************************
 */
vint TRANSPORT_AdvanceRmtAddr(
    tSipHandle hTransport)
{
    tTransport *pTrans = (tTransport*)hTransport;
    if (pTrans->rmtConn.addrIdx < MAX_DNS_IP_ADDRESSES) {
        pTrans->rmtConn.addrIdx++;
        if (pTrans->rmtConn.addrSet[pTrans->rmtConn.addrIdx].ipv4 != 0) {
            return (SIP_OK);
        }
    }
    return (SIP_FAILED);
}

vint TRANSPORT_ProxyMsg(
    tLocalIpConn  *pLclConn,
    tSipIntMsg    *pMsg,
    tUri          *pTargetUri)
{
    tTransport trans;
    tDescr     descr;
    
    trans.numUsers = 1;
    trans.lclConn = *pLclConn;
    trans.rmtConn.addrIdx = 0;
    trans.pDescr = &descr;
    descr.fd = pLclConn->fd;
    descr.lclAddr.port = pLclConn->addr.port;
    
    if (TRANSPORT_GetHostByUri(&trans, pTargetUri, FALSE, &descr.type) ==
            SIP_OK) {
        /* All is good so send it */
        return TRANSPORT_Send(&trans, pMsg); 
    }
    return (SIP_FAILED);
}

vint TRANSPORT_ProxyError(
    tTransportType transType,
    tLocalIpConn  *pLclConn,
    tRemoteIpConn *pRmtConn,
    tSipIntMsg    *pMsg)
{
    tTransport trans;
    tDescr     descr;
    
    trans.numUsers = 1;
    trans.lclConn = *pLclConn;
    trans.rmtConn = *pRmtConn;
    trans.pDescr = &descr;
    descr.fd = pLclConn->fd;
    descr.lclAddr.port = pLclConn->addr.port;
    descr.type = transType;
    return TRANSPORT_Send(&trans, pMsg); 
}

/*
 *****************************************************************************
 * ================TRANSPORT_CloseAllConnections()===================
 *
 * This function will try to close all existing TCP/UDP connections used by SIP.
 * This functiin will not actually close the TCP/UDP connection, it just set the
 * transport descriptor to NORMAL so that the descriptor will be destroyed after
 * all the transactions which use the descriptor are all terminated.
 * This function is typically used before registration to make sure the new
 * registration will not use an bad connection which is still alive.
 *
 * RETURNS:
 *     Nothing
 *
 ******************************************************************************
 */
void TRANSPORT_CloseAllConnections(void)
{
    TRANSDESCR_CloseAll();
}

/*
 *****************************************************************************
 * ================TRANSPORT_CloseConnection()===================
 *
 * This function will try to close an existing TCP/UDP connections used by SIP.
 * This functiin will not actually close the TCP/UDP connection, it just
 * remove the transport descriptor from the transport descriptor pool.
 *
 * fd = The file descriptor that the user wants to close.
 *
 * RETURNS:
 *     Nothing
 *
 ******************************************************************************
 */
void TRANSPORT_CloseConnection(
    OSAL_NetSockId fd)
{
    TRANSDESCR_Close(fd);
}

/*
 * ======== TRANSPORT_updateAccessNwInfo() ========
 * Update network access info of a fd.
 *
 * Returns
 *   SIP_OK: Successul.
 *   SIP_FAILED: Failed.
 */
vint TRANSPORT_updateAccessNwInfo(
    OSAL_NetSockId  fd,
    tNwAccess      *nwAccess_ptr)
{
    tDescr   *pDescr;
    
    if (NULL == (pDescr = TRANSDESCR_MapFd(fd))) {
        return (SIP_FAILED);
    }

    pDescr->nwAccess = *nwAccess_ptr;
    return (SIP_OK);
}
