/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 7821 $ $Date: 2008-10-14 15:50:50 -0500 (Tue, 14 Oct 2008) $
 */
#include <osal_select.h>
#include <osal_net.h>
#include "sip_sip.h"
#include "sip_port.h"
#include "sip_parser_dec.h"
#include "sip_voipnet.h"
#include "sip_xport.h"
#include "_sip_tcpRecv.h"
#include "_sip_descr.h"
#include <ims_net.h>


/* static pool of active (in use) descriptors */
static tDescrPool     _TRANSDESCR_ActivePool;

/* Buffer used to reaed data from the network into */
static char*          _pTRANSDESCR_Buffer = NULL;

/* A descriptor used to wake the select statement when need be */
static tDescr        *_pTRANSDESCR_Control = NULL;

/* Callback when reading data from the network */
static tpfDescrDataCB _TRANSDESCR_CallBack = NULL;

/* Callback when there is an error from the network */
static tpfDescrDataCB _TRANSDESCR_ErrCallBack = NULL;


static void _TRANSDESCR_AwakeSelect(void)
{
    OSAL_NetAddress Target;
    vint            bytes;
    char            val;
    
    if (NULL != _pTRANSDESCR_Control && 0 != _pTRANSDESCR_Control->fd) {
        /*
         * Now wake up the server by sending a 1 byte message to 
         * 127.0.0.1:defaultport
         */
        OSAL_memSet(&Target, 0, sizeof(Target));
        Target.port = _pTRANSDESCR_Control->lclAddr.port;
        Target.ipv4 = _pTRANSDESCR_Control->lclAddr.ipv4;
        val = 0;
        bytes = 1;
        if (OSAL_SUCCESS != IMS_NET_SOCKET_SEND_TO(
                &_pTRANSDESCR_Control->fd, &val, &bytes, &Target)) {
            SIP_DebugLog(SIP_DB_TRANSPORT_LVL_1, 
                    "_AwakeSelect: Failed\n", 0, 0, 0);
        } 
    }
    return;
}

static void _TRANSDESCR_TcpAccept(
    OSAL_NetSockId   fd, 
    OSAL_NetAddress *lclAddr_ptr,
    tNwAccess       *nwAccess_ptr)
{
    OSAL_NetSockId  newFd;
    tDescr         *pDescr;
    OSAL_NetAddress addr;
    
    if (OSAL_SUCCESS == IMS_NET_ACCEPT_ON_SOCKET(&fd, &newFd, &addr)) {
        /* Then we don't know about it, allocate a new one */
        if (NULL == (pDescr = TRANSDESCR_Alloc(newFd, lclAddr_ptr, 
                eTRANSPORT_PERSIST_SERVER, eTransportTcp, nwAccess_ptr))) {
            IMS_NET_CLOSE_SOCKET(&newFd);
            return;
        }
        SIP_DebugLog(SIP_DB_TRANSPORT_LVL_3,
                "_TRANSDESCR_TcpAccept: Accepted newFd:%d rmtAddr", (int)pDescr->fd, 0, 0);
        SIP_DebugLogAddr(SIP_DB_TRANSPORT_LVL_3, &addr);
        
        pDescr->rmtAddr = addr;
        TRANSDESCR_Enable(pDescr);
    }
    return;
}

vint TRANSDESCR_Init(
   tpfDescrDataCB callBack,
   tpfDescrDataCB errCallBack)
{
    OSAL_NetAddress addr;

    /* allocate a buffer from the heap to use */
    if ((_pTRANSDESCR_Buffer =
            SIP_malloc(SIP_MAX_LAYER_4_PACKET_SIZE + 1)) == NULL) {
        return (SIP_NO_MEM);
    }

    /* Set the call back routine that called when data is read the network */
    _TRANSDESCR_CallBack = callBack;
    
    /* Set the error call back routine that called when error from the network */
    _TRANSDESCR_ErrCallBack = errCallBack;

    /* initialize the pool */
    OSAL_memSet(&_TRANSDESCR_ActivePool, 0, sizeof(tDescrPool));
    
    /* create mutex for the descriptor pool */
    SIP_MutexInit(&_TRANSDESCR_ActivePool.lock);
    
    /* Init SSL related stuff */
    IMS_NET_SSL_INIT();
    
    addr.ipv4 = OSAL_netHtonl(OSAL_NET_INADDR_LOOPBACK);
    addr.type = OSAL_NET_SOCK_UDP;
    addr.port = 0;
    /* Now create and init the descriptor used to wake the select */
    if (NULL == (_pTRANSDESCR_Control = TRANSDESCR_Alloc(0, &addr, eTRANSPORT_REUSE,
            eTransportUdp, NULL))) {
        SIP_free(_pTRANSDESCR_Buffer);
        _pTRANSDESCR_Buffer = NULL;
        SIP_MutexDestroy(_TRANSDESCR_ActivePool.lock);
        return (SIP_FAILED);
    }

    /* Then there's no socket yet, create one */
    if (TRANSDESCR_Create(_pTRANSDESCR_Control, &addr) != SIP_OK) {
        TRANSDESCR_Free(_pTRANSDESCR_Control);
        SIP_free(_pTRANSDESCR_Buffer);
        _pTRANSDESCR_Buffer = NULL;
        SIP_MutexDestroy(_TRANSDESCR_ActivePool.lock);
        return (SIP_FAILED);
    }
    TRANSDESCR_Enable(_pTRANSDESCR_Control);
    return (SIP_OK);
}

void TRANSDESCR_Destroy(void)
{
    /* This should be called after TRANS_KillModule() */
    tDescr       *pDescr; 
    vint          x;
    
    /* Kill the descriptor active list */
    for (x = 0 ; x < SIP_MAX_TRANSPORT_DESCRIPTORS ; x++) {
        pDescr = &_TRANSDESCR_ActivePool.descr[x];
        if (eTRANSPORT_NONE != pDescr->mode) {
            /* 
             * Disable active descriptors, force the mode to normal so 
             * the socket also gets destroyed 
             */
            pDescr->mode = eTRANSPORT_NORMAL;
            TRANSDESCR_Disable(pDescr, 1);
        }
    }
    
    SIP_MutexDestroy(_TRANSDESCR_ActivePool.lock);

    /* Now free the buffer used to read data */
    if (_pTRANSDESCR_Buffer) {
        SIP_free(_pTRANSDESCR_Buffer);
        _pTRANSDESCR_Buffer = NULL;
    }
    return;
}

tDescr* TRANSDESCR_Alloc(
    OSAL_NetSockId   fd,
    OSAL_NetAddress *lclAddr_ptr,
    tDescrMode       mode,
    tTransportType   type,
    tNwAccess       *nwAccess_ptr)
{
    tDescr       *pDescr;
    vint          x;
    
    SIP_Lock(_TRANSDESCR_ActivePool.lock);
    for (x = 0 ; x < SIP_MAX_TRANSPORT_DESCRIPTORS ; x++) {
        pDescr = &_TRANSDESCR_ActivePool.descr[x];
        /* Lock here so no 2 threads get the same descriptor resource */
        if (eTRANSPORT_NONE == pDescr->mode) {
            /* Found an available one */
            if (eTransportTcp == type) {
                if (SIP_OK != TCPRECV_initPacket(&pDescr->packet,
                        SIP_MAX_LAYER_4_PACKET_SIZE)) {
                    SIP_Unlock(_TRANSDESCR_ActivePool.lock);
                    return (NULL);
                }
            }
            pDescr->mode = mode;
            pDescr->fd = fd;
            OSAL_netAddrPortCpy(&pDescr->lclAddr, lclAddr_ptr);
            pDescr->type = type;
            pDescr->ttl = 0;
            pDescr->isEnabled = 0;
            pDescr->rmtAddr.ipv4 = 0;
            pDescr->rmtAddr.port = 0;
            if (NULL != nwAccess_ptr) {
                pDescr->nwAccess = *nwAccess_ptr;
            }
            SIP_Unlock(_TRANSDESCR_ActivePool.lock);
            return (pDescr);
        }
    }
    SIP_Unlock(_TRANSDESCR_ActivePool.lock);
    return (NULL);
};

/*
 * ======== TRANSDESCR_updateNwAccessIfno() ========
 * Function to update network access info in a TRANSDESCR.
 *
 * Return:
 *  None.
 */
void TRANSDESCR_updateNwAccessInfo(
    tDescr    *descr_ptr,
    tNwAccess *nwAccess_ptr)
{
    if ((NULL == descr_ptr) || (NULL == nwAccess_ptr)) {
        return;
    }
    /* Copy the nwAccess. */
    descr_ptr->nwAccess = *nwAccess_ptr;
    return;
}

void TRANSDESCR_Free(
    tDescr *pDescr) 
{
    SIP_DebugLog(SIP_DB_TRANSPORT_LVL_3,
            "TRANSDESCR_Free: fd:%d", (int)pDescr->fd, 0, 0);
    TCPRECV_freePacket(&pDescr->packet);
    pDescr->mode = eTRANSPORT_NONE;
    pDescr->type = eTransportNone;
    /* Clean SSL */
    OSAL_memSet(&pDescr->ssl, 0, sizeof(OSAL_NetSslId)); 
    return;
};

void TRANSDESCR_Disable(
    tDescr *pDescr,
    vint    force)
{
    /* If force is valid then force the descriptor to close regardless of mode*/
    if (0 == force) {
        switch (pDescr->mode) {
            case eTRANSPORT_PERSIST_CLIENT:
            case eTRANSPORT_PERSIST_SERVER:
            case eTRANSPORT_DONTCLOSESOCKET:
            case eTRANSPORT_REUSE:
            case eTRANSPORT_NONE:
                /* 
                 * Do not attempt to disable or destroy anything for this 
                 * descriptor.
                 */
                return;
            default:
                /* Otherwise continue to process */
                break;
        }
    }
    
    /* remove the pDescr from the list of active ones */
    OSAL_selectRemoveId(&pDescr->fd, &_TRANSDESCR_ActivePool.fdBitmap);
    _TRANSDESCR_AwakeSelect();
    
    /* Only attempt to close sockets if the mode is "normal" */
    if (pDescr->mode == eTRANSPORT_NORMAL) {
        TRANSDESCR_DestroySocket(pDescr);
    }
    /* Send the descriptor to the "Free" list for use later */
    TRANSDESCR_Free(pDescr);
    return;
}

void TRANSDESCR_Enable(
    tDescr *pDescr)
{
    
    /* insert into fdTable */
    pDescr->isEnabled = 1;
    OSAL_selectAddId(&pDescr->fd, &_TRANSDESCR_ActivePool.fdBitmap);
    _TRANSDESCR_AwakeSelect();
    return;
}

tDescr* TRANSDESCR_MapFd(
    OSAL_NetSockId fd)
{
    /* look for this in the in of active descriptors */
    tDescr   *pDescr;
    vint      x;
    
    for (x = 0 ; x < SIP_MAX_TRANSPORT_DESCRIPTORS ; x++) {
        pDescr = &_TRANSDESCR_ActivePool.descr[x];
        if (eTRANSPORT_NONE != pDescr->mode && pDescr->fd == fd) {
            SIP_DebugLog(SIP_DB_TRANSPORT_LVL_3,
                    "_TRANSDESCR_MapFd: found fd:%d", (int)pDescr->fd, 0, 0);
            return (pDescr);
        }   
    }
    return (NULL);
}

tDescr* TRANSDESCR_MapLclAddress(
    OSAL_NetAddress *addr_ptr,
    tTransportType transType)
{
    /* look for this in the in of active descriptors */
    tDescr   *pDescr;
    vint      x;

    for (x = 0 ; x < SIP_MAX_TRANSPORT_DESCRIPTORS ; x++) {
        pDescr = &_TRANSDESCR_ActivePool.descr[x];
        if (eTRANSPORT_NONE != pDescr->mode &&
                OSAL_netIsAddrEqual(&pDescr->lclAddr, addr_ptr) && 
                pDescr->type == transType) {
            SIP_DebugLog(SIP_DB_TRANSPORT_LVL_3,
                "TRANSDESCR_MapLclAddress: found transport:%d address:", 
                transType, 0, 0);
            SIP_DebugLogAddr(SIP_DB_TRANSPORT_LVL_3, &pDescr->lclAddr);
            return (pDescr);
        }   
    }
    return (NULL);
    
}

tDescr* TRANSDESCR_MapLclAddressPort(
    OSAL_NetAddress *addr_ptr,
    tTransportType   transType)
{
    /* look for this in the in of active descriptors */
    tDescr   *pDescr;
    vint      x;

    for (x = 0 ; x < SIP_MAX_TRANSPORT_DESCRIPTORS ; x++) {
        pDescr = &_TRANSDESCR_ActivePool.descr[x];
        if (eTRANSPORT_NONE != pDescr->mode &&
                OSAL_netIsAddrPortEqual(&pDescr->lclAddr, addr_ptr) &&
                pDescr->type == transType) {
            SIP_DebugLog(SIP_DB_TRANSPORT_LVL_3,
                "TRANSDESCR_MapLclAddressPort: found transport:%d address:",
                transType, 0, 0);
            SIP_DebugLogAddr(SIP_DB_TRANSPORT_LVL_3, &pDescr->lclAddr);
            return (pDescr);
        }
    }
    return (NULL);

}

/*
 *****************************************************************************
 * ================TRANSDESCR_MapLocalIp()===================
 *
 * This function used to look for a matched descriptors according to local
 * address, transport type and mode.
 *
 * RETURNS:
 *      tDescr: A handle to the descriptors matches the conditions.
 *      NULL:   Could not find out the descriptors matches the conditions.
 *
 ******************************************************************************
 */
tDescr* TRANSDESCR_MapLocalIp(
    OSAL_NetAddress *addr_ptr,
    tTransportType   transType,
    tDescrMode       mode)
{
    /* look for this in the in of active descriptors */
    tDescr   *pDescr;
    vint      x;

    for (x = 0 ; x < SIP_MAX_TRANSPORT_DESCRIPTORS ; x++) {
        pDescr = &_TRANSDESCR_ActivePool.descr[x];
        if (mode == pDescr->mode &&
                OSAL_netIsAddrEqual(&pDescr->lclAddr, addr_ptr) &&
                pDescr->type == transType) {
            SIP_DebugLog(SIP_DB_TRANSPORT_LVL_3,
                "TRANSDESCR_MapLocalIp: found transport:%d fd:%d",
                transType, (int)pDescr->fd, 0);
            SIP_DebugLogAddr(SIP_DB_TRANSPORT_LVL_3, &pDescr->lclAddr);
            return (pDescr);
        }
    }
    return (NULL);

}

tDescr* TRANSDESCR_MapPort(
    uint16         lclPort, 
    tTransportType transType)
{
    /* look for this in the in of active descriptors */
    tDescr   *pDescr;
    vint      x;

    for (x = 0 ; x < SIP_MAX_TRANSPORT_DESCRIPTORS ; x++) {
        pDescr = &_TRANSDESCR_ActivePool.descr[x];
        if (eTRANSPORT_NONE != pDescr->mode && pDescr->lclAddr.port == lclPort && 
                pDescr->type == transType) {
             SIP_DebugLog(SIP_DB_TRANSPORT_LVL_3,
                "TRANSDESCR_MapPort: found port:%d transport:%d\n", 
                (int)pDescr->lclAddr.port, transType, 0);
            return (pDescr);
        }   
    }
    return (NULL);
    
}

static tDescr* _TRANSDESCR_MatchRemoteIp(
    OSAL_NetAddress *pRmtAddr,
    tTransportType   transType,
    tDescrMode       mode)
{
    /* look for this in the in of active descriptors */
    tDescr   *pDescr;
    vint      x;

    for (x = 0 ; x < SIP_MAX_TRANSPORT_DESCRIPTORS ; x++) {
        pDescr = &_TRANSDESCR_ActivePool.descr[x];
        if (eTRANSPORT_NONE != pDescr->mode &&
                OSAL_netIsAddrEqual(&pDescr->rmtAddr, pRmtAddr) &&
                pDescr->type == transType &&
                pDescr->mode == mode) {
             SIP_DebugLog(SIP_DB_TRANSPORT_LVL_3,
              "_TRANSDESCR_MatchRemoteIp: found rmtAddr:%d transport:%d\n",
              (int)pDescr->rmtAddr.ipv4, transType, 0);
            return (pDescr);
        }
    }
    return (NULL);

}

tDescr* TRANSDESCR_MapRemoteIp(
    OSAL_NetAddress rmtAddr[],
    vint            rmtAddrSize,
    tTransportType  transType,
    tDescrMode      mode)
{
    tDescr   *pDescr;
    vint      x;

    for (x = 0 ; x < rmtAddrSize ; x++) {
        if (0 == rmtAddr[x].ipv4) {
            /* No addresses to check, bail */
            break;
        }
        if (NULL != (pDescr = _TRANSDESCR_MatchRemoteIp(
                &rmtAddr[x], transType, mode))) {
            return pDescr;
        }
    }
    return (NULL);
}

vint TRANSDESCR_Create(
    tDescr *pDescr,
    OSAL_NetAddress *addr_ptr)
{
    /* then the port does exist so create one */
    if (OSAL_SUCCESS != IMS_NET_SOCKET(&pDescr->fd, addr_ptr->type)) {
        return (SIP_FAILED);
    }

    OSAL_netAddrPortCpy(addr_ptr, &pDescr->lclAddr);
           
    if (OSAL_SUCCESS != IMS_NET_BIND_SOCKET(&pDescr->fd, addr_ptr)) {
        IMS_NET_CLOSE_SOCKET(&pDescr->fd);
        return (SIP_FAILED);
    }
    if (0 == pDescr->lclAddr.port) {
        /* 
         * Then the calling code did not specifiy the port, so let's see that 
         * the arbitrary one is.
         */
        if (OSAL_SUCCESS == IMS_NET_GET_SOCKET_ADDRESS(&pDescr->fd, addr_ptr)) {
            OSAL_netAddrPortCpy(&pDescr->lclAddr, addr_ptr);
        }
    }
    return (SIP_OK);
}

vint TRANSDESCR_TcpConn(
    tDescr          *pDescr,
    OSAL_NetAddress *pAddr)
{
    OSAL_NetAddress addr;

    SIP_DebugLog(SIP_DB_TRANSPORT_LVL_3,
            "TRANSDESCR_TcpConn: Connect to rmtAddr:\n",
            pDescr->lclAddr.port, 0, 0);
    SIP_DebugLogAddr(SIP_DB_TRANSPORT_LVL_3, &pDescr->rmtAddr);

    /* now connect to the remote end */
    if (OSAL_SUCCESS != IMS_NET_CONNECT_SOCKET(&pDescr->fd,
            &pDescr->rmtAddr)) {
        SIP_DebugLog(SIP_DB_TRANSPORT_LVL_3,
            "TRANSDESCR_TcpConn : Connect socket failed. rmtAddr:",
                0, 0, 0);
        SIP_DebugLogAddr(SIP_DB_TRANSPORT_LVL_3, &pDescr->rmtAddr);
        IMS_NET_CLOSE_SOCKET(&pDescr->fd);
        return (SIP_FAILED);
    }

    if (OSAL_SUCCESS == IMS_NET_GET_SOCKET_ADDRESS(&pDescr->fd, &addr)) {
        OSAL_netAddrPortCpy(&pDescr->lclAddr, &addr);
        SIP_DebugLog(SIP_DB_TRANSPORT_LVL_3,
            "TRANSDESCR_TcpConn : Connected. lclAddr:", 0, 0, 0);
        SIP_DebugLogAddr(SIP_DB_TRANSPORT_LVL_3, &pDescr->lclAddr);
    }
    return (SIP_OK);

}

/* ======== TRANSDESCR_TlsConn() ========
 * Connect TLS. Socket must be exist before call this routine.
 *
 * Return
 *  SIP_OK: Connect successfully.
 *  SIP_FAILED: Fail to connect TLS.
 */
vint TRANSDESCR_TlsConn(
    tDescr     *pDescr)
{
    /* now connect to the remote end */
    if (OSAL_SUCCESS != IMS_NET_CONNECT_SOCKET(&pDescr->fd,
            &pDescr->rmtAddr)) {
        SIP_DebugLog(SIP_DB_TRANSPORT_LVL_1,
                    "TRANSDESCR_TlsConn : Failed to connect to:%X\n",
                    pDescr->rmtAddr.ipv4, 0, 0);
        IMS_NET_CLOSE_SOCKET(&pDescr->fd);
        return (SIP_FAILED);
    }

    /* Now let's make this connection secure via TLS (SSL V3) */
    if (OSAL_SUCCESS != IMS_NET_SSL(&pDescr->ssl,
            OSAL_NET_SSL_METHOD_CLIENT_TLSV1)) {
        SIP_DebugLog(SIP_DB_TRANSPORT_LVL_1,
                "TRANSDESCR_TlsConn : Failed to init SSL\n", 0, 0, 0);
        IMS_NET_CLOSE_SOCKET(&pDescr->fd);
        return (SIP_FAILED);
    }
    
    /* Connect the SSL object with the socket */    
    if (OSAL_SUCCESS != IMS_NET_SSL_SET_SOCKET(&pDescr->fd, &pDescr->ssl)) {
        SIP_DebugLog(SIP_DB_TRANSPORT_LVL_1,
                "TRANSDESCR_TlsConn : Failed 'set' SSL\n", 0, 0, 0);
        IMS_NET_SSL_CLOSE(&pDescr->ssl, &pDescr->fd);
        return (SIP_FAILED);
    }

    /* Now connect to the remote party via SSL */
    if (OSAL_SUCCESS != IMS_NET_SSL_CONNECT(&pDescr->ssl, &pDescr->fd)) {
        SIP_DebugLog(SIP_DB_TRANSPORT_LVL_1,
                "TRANSDESCR_TlsConn : Failed to 'connect' SSL\n",
                0, 0, 0);
        IMS_NET_SSL_CLOSE(&pDescr->ssl, &pDescr->fd);
        return (SIP_FAILED);
    }
    SIP_DebugLog(SIP_DB_TRANSPORT_LVL_3,
                "TRANSDESCR_TlsConn : Successfull SSL connection!\n",
                0, 0, 0);
    return (SIP_OK);
}

vint TRANSDESCR_CreateTcpListen(
    tDescr     *pDescr)
{
    OSAL_NetAddress addr;
    
    SIP_DebugLog(SIP_DB_TRANSPORT_LVL_3,
            "TRANSDESCR_CreateTcpListen : Creating socket port:x0x%X",
            pDescr->lclAddr.port, 0, 0);
    
    /* then the port does exist so create one */
    if (OSAL_SUCCESS != IMS_NET_SOCKET(&pDescr->fd, pDescr->lclAddr.type)) {
        return (SIP_FAILED);
    }
   
#if 0
    /* set up the ttl if it is set */
    if (pDescr->ttl != 0) {
        if (setsockopt(pDescr->fd, SIP_IPPROTO_IP, SIP_IP_TTL,
                (char*)&pDescr->ttl, sizeof(pDescr->ttl)) != 0) {
        }
    } /* end of ttl */
#endif
    
    OSAL_netAddrPortCpy(&addr, &pDescr->lclAddr);
    
    /* now bind to the local end */
    if (pDescr->lclAddr.port != 0) {
        if (OSAL_SUCCESS != IMS_NET_BIND_SOCKET(&pDescr->fd, &addr)) {
            IMS_NET_CLOSE_SOCKET(&pDescr->fd);
            return (SIP_FAILED);
        }
    }
    
    if (OSAL_SUCCESS != IMS_NET_LISTEN_ON_SOCKET(&pDescr->fd)) {
        SIP_DebugLog(SIP_DB_TRANSPORT_LVL_1,
                "TRANSDESCR_CreateTcpListen: listen() failed. fd:%d", 
                pDescr->fd, 0, 0);
        IMS_NET_CLOSE_SOCKET(&pDescr->fd);
        return (SIP_FAILED);
    }
    SIP_DebugLog(SIP_DB_TRANSPORT_LVL_3,
            "TRANSDESCR_CreateTcpListen : Successully. port:0x%X, fd:%d",
            pDescr->lclAddr.port, pDescr->fd, 0);
    return (SIP_OK);
}

void TRANSDESCR_DestroySocket(
    tDescr *pDescr) 
{
    SIP_DebugLog(SIP_DB_TRANSPORT_LVL_3,
        "TRANSDESCR_DestroySocket: pDescr:%x", (int)pDescr, 0, 0);
    if (0 != pDescr->fd) {
        switch (pDescr->type) {
            case eTransportTls:
                IMS_NET_SSL_CLOSE(&pDescr->ssl, &pDescr->fd);
                break;
            default:
                IMS_NET_CLOSE_SOCKET(&pDescr->fd);
                break;
        }
        pDescr->fd = 0;
    }
}

void TRANSDESCR_Recv(void)
{
    OSAL_SelectSet      fdset;
    vint                bytesRead;
    tLocalIpConn        lclConn;
    tRemoteIpConn       rmtConn;
    OSAL_SelectTimeval  wait;
    OSAL_Boolean        val;
    OSAL_Status         ret;
    tDescr             *pDescr;
    OSAL_Boolean        isTimedOut;
    vint                idx;
    
    fdset = _TRANSDESCR_ActivePool.fdBitmap;
    /* 
     * Use timeout on select() so that it never gets stuck. Timeout should be
     * long to not waste CPU time.
     */
    wait.sec = 5;
    wait.usec = 0;

    if (OSAL_SUCCESS != OSAL_select(&fdset, NULL, &wait, &isTimedOut)) { 
        /* Do not let a select wildly spin so delay on failure */
        OSAL_taskDelay(100);
        return;
    }
    if (isTimedOut == OSAL_TRUE) {
        /* Then we timed out, nothing to process */
        return;
    }
        
    for (idx = 0 ; idx < SIP_MAX_TRANSPORT_DESCRIPTORS ; idx++) {
        pDescr = &_TRANSDESCR_ActivePool.descr[idx];
        /* Don't Process if it's not active */
        if (eTRANSPORT_NONE == pDescr->mode) {
            continue;
        }
            
        /* Check if this fd and descriptor are valid or 'enabled' */
        ret = OSAL_selectIsIdSet(&pDescr->fd, &fdset, &val);
        if ((OSAL_FAIL == ret) || (OSAL_FALSE == val) || 
                (0 == pDescr->isEnabled)) {
            /* Then this fd is not to be processed */
            continue;
        }
        bytesRead = 0;
        
        switch (pDescr->type) {
            case eTransportUdp:
                SIP_DebugLog(SIP_DB_TRANSPORT_LVL_2,
                        "TRANSDRVR_Recv: read UDP data from fd %d\n", 
                        pDescr->fd, 0, 0);
                bytesRead = SIP_MAX_LAYER_4_PACKET_SIZE;
                if (OSAL_SUCCESS != IMS_NET_SOCKET_RECEIVE_FROM(&pDescr->fd,
                        _pTRANSDESCR_Buffer, &bytesRead, &rmtConn.addrSet[0])) {
                    bytesRead = -1;
                    break;
                }
                rmtConn.addrIdx = 0;
                break;
            case eTransportTcp:
               SIP_DebugLog(SIP_DB_TRANSPORT_LVL_2,
                        "TRANSDRVR_Recv: read TCP data on fd %d\n",
                        pDescr->fd, 0, 0);
                if (pDescr->mode == eTRANSPORT_REUSE) {
                    /* 
                     * Then we know that we are here because the "listen" was 
                     * triggered so we don't 'read' but rather 'accept' 
                     */
                    _TRANSDESCR_TcpAccept(pDescr->fd, &pDescr->lclAddr,
                            &pDescr->nwAccess);
                    continue;
                }
                /* Otherwise we want to read the interface */
                bytesRead = SIP_MAX_LAYER_4_PACKET_SIZE;
#if 1
                if (OSAL_SUCCESS != TCPRECV_read(pDescr->fd,
                        _pTRANSDESCR_Buffer, SIP_MAX_LAYER_4_PACKET_SIZE,
                        &pDescr->packet, &bytesRead)) {
                    bytesRead = -1;
                }
                else {
                    /*
                     * Else we are successful, but we could have '-1' which
                     * means we are waiting for more data, so let's
                     * just continue.
                     */
                    if (-1 == bytesRead) {
                        /*
                         * Let's sleep for a bit to allow more data
                         * to get here before we continue to process
                         */
                        continue;
                    }
                    /* Copy back the packet */
                    OSAL_memCpy(_pTRANSDESCR_Buffer, pDescr->packet.begin_ptr, bytesRead);
                }

#else 
                if (OSAL_SUCCESS != TCPRECV_read(pDescr->fd,
                        _pTRANSDESCR_Buffer, &bytesRead)) {
                    bytesRead = -1;
                }
                else {
                    /*
                     * Else we are successful, but we could have '-1' which
                     * means we are waiting for more data, so let's
                     * just continue.
                     */
                    if (-1 == bytesRead) {
                        /*
                         * Let's sleep for a bit to allow more data
                         * to get here before we continue to process
                         */
                        OSAL_taskDelay(10);
                        continue;
                    }
                }
#endif
                rmtConn.addrSet[0] = pDescr->rmtAddr;
                rmtConn.addrIdx = 0;
                break;
            case eTransportTls:
                SIP_DebugLog(SIP_DB_TRANSPORT_LVL_2, 
                        "TRANSDRVR_Recv: read TLS data on fd %d\n", 
                        pDescr->fd, 0, 0);
                /* Otherwise we want to read the interface */
                bytesRead = SIP_MAX_LAYER_4_PACKET_SIZE;
                if (OSAL_SUCCESS != IMS_NET_SSL_RECEIVE(&pDescr->ssl,
                        _pTRANSDESCR_Buffer, &bytesRead, &pDescr->fd)) {
                    bytesRead = -1;
                }
                rmtConn.addrSet[0] = pDescr->rmtAddr;
                rmtConn.addrIdx = 0;
                break;
            default:
                break;
        } /* End of switch statement */
        if (bytesRead > 1) {
            /* 
             * There is data! Populate the address that the data came from FOR 
             * DEFAULT RESOURCES ONLY! Clients and servers shouldn't do this.
             */
            lclConn.fd = pDescr->fd;
            lclConn.addr.port = pDescr->lclAddr.port;
            lclConn.nwAccess = pDescr->nwAccess;
            OSAL_netAddrClear(&lclConn.addr);
            lclConn.rport = 0;
            _pTRANSDESCR_Buffer[bytesRead] = 0;
            /* call the callback that will proces the data read */
            (*_TRANSDESCR_CallBack)(pDescr->type, &lclConn, &rmtConn, 
                    _pTRANSDESCR_Buffer, bytesRead, pDescr->fd);
        }
        else if (-1 == bytesRead) {
            SIP_DebugLog(SIP_DB_TRANSPORT_LVL_1,
                    "TRANSDRVR_Recv fatal error: fd:%d\n", pDescr->fd, 0, 0);
            switch (pDescr->type) {
                 case eTransportTls:
                 case eTransportTcp:
                    /* 
                     * Then the remote end has shutdown this connection
                     * disable it and then return since we are goig to remove 
                     * the descriptor from the active list.
                     */
                    TRANSDESCR_Disable(pDescr, 1);
                 default:
                    break;
            }
            (*_TRANSDESCR_ErrCallBack)(pDescr->type, &lclConn, &rmtConn,
                                _pTRANSDESCR_Buffer, bytesRead, pDescr->fd);
            return;
        }
        else if (0 == bytesRead) {
            SIP_DebugLog(SIP_DB_TRANSPORT_LVL_1, 
                    "TRANSDRVR_Recv error '0' bytes read fd:%d\n", 
                    pDescr->fd, 0, 0);
            switch (pDescr->type) {
                 case eTransportTls:
                 case eTransportTcp:
                    /*
                     * Then the remote end has shutdown this connection
                     * disable it and then return since we are goig to remove
                     * the descriptor from the active list.
                     */
                    TRANSDESCR_Disable(pDescr, 1);
                 default:
                    break;
            }
            (*_TRANSDESCR_ErrCallBack)(pDescr->type, &lclConn, &rmtConn,
                                _pTRANSDESCR_Buffer, bytesRead, pDescr->fd);
            return;
        }
        else {
            /* It's one byte, this is probably a byte used to wake a select */
            SIP_DebugLog(SIP_DB_TRANSPORT_LVL_2, 
                    "TRANSDRVR_Recv received select wake up byte fd:%d\n",
                    pDescr->fd, 0, 0);
        }
    } /* End of top for loop */
}

/* 
 *****************************************************************************
 * ================TRANSDRVR_UdpSend()===================
 * 
 * This function sends a UDP data packet to the network via the transport 
 * resource specified in pTrans.
 *
 * pTrans  = The transport resource that we are sending the data from
 * pData   = A pointer to the data that we are going to send 
 * dataLen = The length in bytes of the data packet that we are going to send.
 * 
 * RETURNS:
 *      SIP_OK:      data was sent
 *      SIP_FAILED:  There was an error with underlying IP layer software.
 *
 ******************************************************************************
 */   
vint TRANSDESCR_UdpSend(
    tDescr          *pDescr,
    OSAL_NetAddress *pAddr,
    char            *pData,
    uint32           dataLen)
{
    vint            numSent;
    OSAL_Status     ret;
    char            ipAddr[16];
    char            ipv6Addr[39];
    
    OSAL_NetSockId fd = pDescr->fd;

    SIP_DebugLog(SIP_DB_TRANSPORT_LVL_2, "TRANSDRVR_UdpSend: fd:%d", fd, 0, 0);

    SIP_DebugLog(SIP_DB_TRANSPORT_LVL_2,
            "TRANSDRVR_UdpSend : sendto from %d to %d", 
            (int)pDescr->lclAddr.port, (int)pAddr->port, 0);

    if ((pAddr->type == OSAL_NET_SOCK_UDP_V6) || (pAddr->type == OSAL_NET_SOCK_TCP_V6)) {
        if (OSAL_SUCCESS == OSAL_netAddressToString((int8 *)ipv6Addr, pAddr)) {
            SIP_DebugLog(SIP_DB_TRANSPORT_LVL_1, "Remote IP(v6):%s (network byte order)",
                 (int)ipv6Addr, 0 , 0);
        }
        else {
            SIP_DebugLog(SIP_DB_TRANSPORT_LVL_1, "Remote IP(v6) is invaild",
                    0 , 0 , 0);
        }
    }
    else {
        if (OSAL_SUCCESS == OSAL_netAddressToString((int8 *)ipAddr, pAddr)) {
            SIP_DebugLog(SIP_DB_TRANSPORT_LVL_1, "Remote IP(v4):%s (network byte order)",
                 (int)ipAddr, 0 , 0);
        }
        else {
            SIP_DebugLog(SIP_DB_TRANSPORT_LVL_1, "Remote IP(v4) is invaild",
                    0 , 0 , 0);
        }
    }

    SIP_DebugLog(SIP_DB_TRANSPORT_LVL_1, "OUTBOUND UDP MSG\n", 0, 0, 0);

#ifdef SIP_DEBUG_LOG
    OSAL_logString(pData);
#endif
    
    /* now send it */
    numSent = dataLen;
    ret = IMS_NET_SOCKET_SEND_TO(&fd, pData, &numSent, pAddr);
    if ((OSAL_SUCCESS == ret) && ((uint32)numSent == dataLen)) {
        return (SIP_OK);
    }
    return (SIP_FAILED);
}

/* 
 *****************************************************************************
 * ================TRANSDESCR_TcpSend()===================
 * 
 * This function sends a TCP packet 
 *
 * pTrans  = The transport resource that we are sending the data from
 * pData   = A pointer to the data that we are going to send 
 * dataLen = The length in bytes of the data packet that we are going to send.
 * 
 * RETURNS:
 *      SIP_OK:      data was sent
 *      SIP_FAILED:  There was an error with underlying IP layer.
 *
 ******************************************************************************
 */
vint TRANSDESCR_TcpSend(
    tDescr      *pDescr,
    char        *pData,
    uint32       dataLen)
{
    vint numSent;
    OSAL_Status ret;

    SIP_DebugLog(SIP_DB_TRANSPORT_LVL_2, "TRANSDESCR_TcpSend: fd:%d\n", 
                 (int)pDescr->fd, 0, 0);

    SIP_DebugLog(SIP_DB_TRANSPORT_LVL_1, "OUTBOUND TCP MSG\n", 0, 0, 0);
#ifdef SIP_DEBUG_LOG
    OSAL_logString(pData);
#endif

    /* now send it */
    numSent = dataLen;
    ret = IMS_NET_SOCKET_SEND(&pDescr->fd, pData, &numSent);
    if (((uint32)numSent != dataLen) || (OSAL_SUCCESS != ret)) {
        SIP_DebugLog(SIP_DB_TRANSPORT_LVL_1,
                "TRANSDESCR_TcpSend: failed fd:%d\n", pDescr->fd, 0, 0);
        /* XXX */
        /*         
        SIP_DebugLog(SIP_DB_TRANSPORT_LVL_1,
             "TRANSDESCR_TcpSend: errno is: %d\n", errno, 0, 0);
        */
        return (SIP_FAILED);
    }
    return (SIP_OK);
}

/* 
 *****************************************************************************
 * ================TRANSDESCR_TlsSend()===================
 * 
 * This function sends a TCP packet 
 *
 * pTrans  = The transport resource that we are sending the data from
 * pData   = A pointer to the data that we are going to send 
 * dataLen = The length in bytes of the data packet that we are going to send.
 * 
 * RETURNS:
 *      SIP_OK:      data was sent
 *      SIP_FAILED:  There was an error with underlying IP layer.
 *
 ******************************************************************************
 */
vint TRANSDESCR_TlsSend(
    tDescr      *pDescr,
    char        *pData,
    uint32       dataLen)
{
    vint numSent;
    OSAL_Status ret;
    
    SIP_DebugLog(SIP_DB_TRANSPORT_LVL_2, "TRANSDESCR_TlsSend: fd:%d\n", 
                 (int)pDescr->fd, 0, 0);

    SIP_DebugLog(SIP_DB_TRANSPORT_LVL_1, "OUTBOUND TCP MSG\n", 0, 0, 0);

#ifdef SIP_DEBUG_LOG
    OSAL_logString(pData);
#endif

    /* now send it */
    numSent = dataLen;
    ret = IMS_NET_SSL_SEND(&pDescr->ssl, pData, &numSent, &pDescr->fd);
    if (((uint32)numSent != dataLen) || (OSAL_SUCCESS != ret)) {
        SIP_DebugLog(SIP_DB_TRANSPORT_LVL_1,
                "TRANSDESCR_TlsSend : failed fd:%d\n", pDescr->fd, 0, 0);
        return (SIP_FAILED);
    }
    return (SIP_OK);
}

vint TRANSDESCR_Switch(
    OSAL_NetSockId  fd, 
    vint            onOff)
{
    tDescr       *pDescr;
    vint          status;
    vint          x;
    
    /* 
     * Search for the fd on the active list and disabled it from being processed
     * in the select statement. 
     */
    status = SIP_NOT_FOUND;
    for (x = 0 ; x < SIP_MAX_TRANSPORT_DESCRIPTORS ; x++) {
        pDescr = &_TRANSDESCR_ActivePool.descr[x];
        if (onOff == -1) {
            /* 
             * Then that means to disable the whole interface. You can only do 
             * this if you are planing on destroying the whole stack.
             */
            pDescr->isEnabled = 0;
            OSAL_selectRemoveId(&pDescr->fd, &_TRANSDESCR_ActivePool.fdBitmap);
            status = SIP_OK;
        }
        else if (pDescr->fd == fd) { 
            /* found it, now switch the mode */
            if (onOff) {
                /* Then enable */
                pDescr->isEnabled = 1;
                OSAL_selectAddId(&pDescr->fd,
                        &_TRANSDESCR_ActivePool.fdBitmap);
            }
            else {
                pDescr->isEnabled = 0;
                OSAL_selectRemoveId(&pDescr->fd,
                        &_TRANSDESCR_ActivePool.fdBitmap);
            }
            status = SIP_OK;
            break;
        }
    } /* End of for */
    _TRANSDESCR_AwakeSelect();
    return (status);
}

vint TRANSDESCR_replaceServerSocket(
    OSAL_NetSockId   fd,
    OSAL_NetAddress *pAddr,
    tTransportType   tType,
    tNwAccess       *nwAccess_ptr,
    uint16           oldPort)
{
    OSAL_NetAddress addr;
    tDescr* pDescr;

    OSAL_netAddrCpy(&addr, pAddr);
    addr.port = oldPort;
    /* Find if there is an existing server socket */
    if (0 != oldPort &&
        NULL != (pDescr = TRANSDESCR_MapLclAddressPort(&addr, tType))) {
        /* Found existing server socket */

        /* Freeze the existing default interface */
        TRANSDESCR_Switch(pDescr->fd, 0);
        TRANSDESCR_Disable(pDescr, 1);
    }

    /* If port is zero means to close the server socket, then return */
    if (0 == pAddr->port) {
        return (SIP_OK);
    }

    /* Allocate a new server descriptor */
    if (NULL == (pDescr = TRANSDESCR_Alloc(fd, pAddr,
            eTRANSPORT_REUSE, tType, nwAccess_ptr))) {
        SIP_DebugLog(SIP_DB_TRANSPORT_LVL_1, "TRANSDESCR_ReplaceServerSocket failed. Addr:", 
                 0, 0, 0);
        SIP_DebugLogAddr(SIP_DB_TRANSPORT_LVL_1, pAddr);
        return (SIP_FAILED);
    }
 
    TRANSDESCR_Enable(pDescr);


    return (SIP_OK);
}

vint TRANSDESCR_ReplaceInterace(
    uint16         oldPort,
    tTransportType transType,
    OSAL_NetSockId newFd,
    uint16         newPort)
{
    tDescr* pDescr;

    if (NULL == (pDescr = TRANSDESCR_MapPort(oldPort, transType))) {
        /* Can't find it! */
        return (SIP_FAILED);
    }

    /* Freeze the existing default interface */
    TRANSDESCR_Switch(pDescr->fd, 0);
    /* Destroy the existing socket */
    TRANSDESCR_DestroySocket(pDescr);
    /* Copy in the new socket info */
    pDescr->fd = newFd;
    pDescr->lclAddr.port = newPort;
    /* Turn the fd back on */
    TRANSDESCR_Switch(pDescr->fd, 1);
    return (SIP_OK);
}

void TRANSDESCR_CloseAll(void)
{
    tDescr       *pDescr;
    vint          x;

    SIP_Lock(_TRANSDESCR_ActivePool.lock);

    for (x = 0 ; x < SIP_MAX_TRANSPORT_DESCRIPTORS ; x++) {
        pDescr = &_TRANSDESCR_ActivePool.descr[x];
        /* Lock here so no 2 threads get the same descriptor resource */
        if (eTRANSPORT_PERSIST_CLIENT == pDescr->mode) {
            /* Found a client */

            /* Take it out of service */
            pDescr->isEnabled = 0;

            /* Clear the remote address */
            pDescr->rmtAddr.ipv4 = 0;
            pDescr->rmtAddr.port = 0;
            pDescr->mode = eTRANSPORT_NORMAL;
            /* Bug 7407, Diable the Transcriptor or we leak them */
            TRANSDESCR_Disable(pDescr, 1);
        }
        else if ((eTRANSPORT_DONTCLOSESOCKET == pDescr->mode) ||
                ((eTRANSPORT_REUSE == pDescr->mode) &&
                (eTransportTcp == pDescr->type))) {
            /* Disale the descriptor set from app */
            pDescr->mode = eTRANSPORT_NONE;
            TRANSDESCR_Disable(pDescr, 1);
        }
    }
    SIP_Unlock(_TRANSDESCR_ActivePool.lock);
    return;
}

/* 
 * ======== TRANSDESCR_Close() ========
 *
 * This function to remove a tDescr from the _TRANSDESCR_ActivePool
 *
 * Returns: 
 *   None.
 */
void TRANSDESCR_Close(
    OSAL_NetSockId fd)
{
    tDescr       *pDescr;
    vint          x;

    SIP_Lock(_TRANSDESCR_ActivePool.lock);

    for (x = 0 ; x < SIP_MAX_TRANSPORT_DESCRIPTORS ; x++) {
        pDescr = &_TRANSDESCR_ActivePool.descr[x];
        if (pDescr->fd == fd) {
            /* Found the fd */
            pDescr->isEnabled = 0;
            pDescr->mode = eTRANSPORT_NONE;
            TRANSDESCR_Disable(pDescr, 1);
        }
    }

    SIP_Unlock(_TRANSDESCR_ActivePool.lock);
    return;
}

