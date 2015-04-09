
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

#include <sip_sip.h>
#include <sip_xport.h>
#include <sip_ua.h>
#include <sip_hdrflds.h>
#include <sip_mem_pool.h>
#include <sip_app.h>

#include "sr_net.h"
#include "_vpr.h"
#include "_vpr_sr.h"

/* External VPR global object */
extern VPR_Obj *_VPR_Obj_ptr;

/*
 * ======== SR_Start() ========
 * Public function to start SR.
 *
 * Returns: 
 * OSAL_SUCCESS
 * 
 */

OSAL_Status SR_start(
    void)
{
    /* Do nothing */
    return (OSAL_SUCCESS);
}

/*
 * ======== SR_allocate() ========
 * Public function to initialize SR.
 *
 * Returns: 
 * OSAL_SUCCESS
 * 
 */
OSAL_Status SR_allocate(
    void)
{
    /* Do nothing */
    return (OSAL_SUCCESS);
}

/*
 * ======== SR_init() ========
 * Public function to initialize SR.
 *
 * Returns: 
 * OSAL_SUCCESS: SR successfully initialized.
 * OSAL_FAIL: Error in SR initialization.
 */
OSAL_Status SR_init(
    void)
{
    /* Do nothing */
    return (OSAL_SUCCESS);
}

/* 
 * ======== SR_shutdown() ========
 * Public function to shutdown SR.
 *
 * Returns: 
 * OSAL_SUCCESS: SR shutdown successfully 
 * OSAL_FAIL: Error in SR shutdown
 */
OSAL_Status SR_shutdown(
    void)
{
    /* Do nothing */
    return (OSAL_SUCCESS);
}

/* 
 * ======== SR_processReceivedSipPacket() ========
 *
 * This function is to check if sip packet is need to redirect to oppsite
 * processor and redirect it.
 *
 * Returns:
 *  OSAL_SUCCESS: Success.
 *  OSAL_FAIL:    Failure.
 */
OSAL_Status SR_processReceivedSipPacket(
    OSAL_NetSockId sockId,
    tSipIntMsg    *pMsg,
    char          *buf_ptr,
    int            len)
{
    return (_VPR_srProcessReceivedSipPacket(
            _VPR_Obj_ptr, sockId, pMsg, buf_ptr, len));
}

/* 
 * ======== SR_addCallId() ========
 *
 * This function is add a dialog id for VPR to filter incoming SIP packet for
 * packet redirection.
 *
 * Returns:
 *  OSAL_SUCCESS: Success
 *  OSAL_FAIL:    Failure.
 */
OSAL_Status SR_addCallId(
    char *id_ptr)
{
    _VPR_srAddCallId(_VPR_Obj_ptr, id_ptr);
    return (OSAL_SUCCESS);
}

/* 
 * ======== SR_removeCallId() ========
 *
 * This function is remove a dialog id for VPR to filter incoming SIP packet for
 * packet redirection.
 *
 * Returns:
 *  OSAL_SUCCESS: Success
 *  OSAL_FAIL:    Failure.
 */
OSAL_Status SR_removeCallId(
    char *id_ptr)
{
    _VPR_srRemoveCallId(_VPR_Obj_ptr, id_ptr);
    return (OSAL_SUCCESS);
}

/*
 * ======== SR_setNetworkMode() ========
 *
 * This function is to set what network mode used, wifi or lte.
 *
 * Returns:
 *  OSAL_SUCCESS: Success
 *  OSAL_FAIL:    Failure.
 */
OSAL_Status SR_setNetworkMode(
    char *mode_ptr)
{
    VPR_NetworkMode mode;

    /* Determine network mode */
    if (0 == OSAL_strncmp(mode_ptr, "lte", 3)) {
        mode = VPR_NETWORK_MODE_LTE;
    }
    else {
        mode = VPR_NETWORK_MODE_WIFI;
    }

    /* Tell VPR the mode changed */
    _VPR_setNetworkMode(_VPR_Obj_ptr, mode);
    return (OSAL_SUCCESS);
}

/* 
 * ======== SR_clientConnect() ========
 *
 * This function is to connect make SIP connection to VPR in WIFI mode
 * so that VPR could redirect packet to SR.
 * For VPR, nothing needs to do.
 *
 * Returns:
 *  SIP_OK: Success
 *  SIP_FAILED: Failure.
 */
vint SR_clientConnect(
    tSipHandle      hUa,
    tLocalIpConn   *pLclConn,
    tTransportType  type)
{
    return (SIP_clientConnect(hUa, pLclConn, type));
}

/* 
 * ======== SR_setRegSecData() ========
 *
 * This function is used to set IPSec/Reg information from MSAPP to ASAPP.
 *
 * Returns:
 *  SIP_OK: Success
 *  SIP_FAILED: Failure.
 */
OSAL_Status SR_setRegSecData(
    char           *preconfiguredRoute,
    char           *secSrvHfs,
    int             secSrvHfNum,
    int             secSrvStrlen,
    OSAL_IpsecSa   *inboundSAc,
    OSAL_IpsecSa   *inboundSAs,
    OSAL_IpsecSa   *outboundSAc)
{
    return (_VPR_srSendRegSecCfg(preconfiguredRoute,
            secSrvHfs, secSrvHfNum, secSrvStrlen,
            inboundSAc, inboundSAs, outboundSAc));
}

/* 
 * ======== SR_getRegSecData() ========
 *
 * This function is used to get IPSec/Reg information from MSAPP to ASAPP.
 * This is not used in VPR
 *
 * Returns:
 *  SIP_OK: Success
 *  SIP_FAILED: Failure.
 */
OSAL_Status SR_getRegSecData(
    char           *preconfiguredRoute,
    int             routeLen,
    char           *secSrvHfs,
    int             secSrvHfNum,
    int             secSrvStrlen,
    uint16         *portUs,
    uint16         *portUc,
    OSAL_IpsecSa   *inboundSAc,
    OSAL_IpsecSa   *inboundSAs,
    OSAL_IpsecSa   *outboundSAc)
{
    return (OSAL_SUCCESS);
}

/*
 * ======== SR_netSocket() ========
 *
 * Wrapper function of net socket function.
 * It calls OSAL net function and does VPR processing for SR socket.
 *
 * Returns:
 *  OSAL_SUCCESS: Success.
 *  OSAL_FAIL:    Failure.
 */
OSAL_Status SR_netSocket(
    OSAL_NetSockId   *socket_ptr,
    OSAL_NetSockType  type)
{
    OSAL_Status status;

    if (OSAL_SUCCESS == (status = OSAL_netSocket(socket_ptr, type))) {
        if (OSAL_FAIL == _VPR_srAllocSocket(_VPR_Obj_ptr, *socket_ptr,
                type)) {
            /* Fail to allocate VPR_SrSocket */
            VPR_dbgPrintf("Fail to allocate VPR_SrSocket %d type:%d\n",
                    *socket_ptr, type);
            return (OSAL_FAIL);
        }
        return (OSAL_SUCCESS);
    }
    
    return (OSAL_FAIL);
}

/* 
 * ======== SR_netCloseSocket() ========
 *
 * Wrapper function of net socket function.
 * It calls OSAL net function and does VPR processing for SR socket.
 *
 * Returns:
 *  OSAL_SUCCESS: Success.
 *  OSAL_FAIL:    Failure.
 */
OSAL_Status SR_netCloseSocket(
    OSAL_NetSockId *socket_ptr)
{
    if (OSAL_FAIL == _VPR_srFreeSocket(_VPR_Obj_ptr, *socket_ptr)) {
        VPR_dbgPrintf("Fail to free VPR_SrSocket %d\n", *socket_ptr);
    }

    return OSAL_netCloseSocket(socket_ptr);
}

/* 
 * ======== SR_netConnectSocket() ========
 *
 * Wrapper function of net socket function.
 * It calls OSAL net function and does VPR processing for SR socket.
 * depends on wifi mode or 4G mode.
 *
 * Returns:
 *  OSAL_SUCCESS: Success.
 *  OSAL_FAIL:    Failure.
 */
OSAL_Status SR_netConnectSocket(
    OSAL_NetSockId  *socket_ptr,
    OSAL_NetAddress *address_ptr)
{
    OSAL_Status status;

    /* Just return success in wifi mode. */
    if (VPR_NETWORK_MODE_WIFI == _VPR_Obj_ptr->networkMode) {
        return (OSAL_SUCCESS);
    }

    if (OSAL_SUCCESS == (status =
            OSAL_netConnectSocket(socket_ptr, address_ptr))) {
        /* XXX Need to store address to VPR_SrSocket? */
    }
    return status;
}

/* 
 * ======== SR_netBindSocket() ========
 *
 * Wrapper function of net socket function.
 * It calls OSAL net function and does VPR processing for SR socket.
 *
 * Returns:
 *  OSAL_SUCCESS: Success.
 *  OSAL_FAIL:    Failure.
 */
OSAL_Status SR_netBindSocket(
    OSAL_NetSockId  *socket_ptr,
    OSAL_NetAddress *address_ptr)
{
    VPR_SrSocket *sock_ptr;

    /* If bind local loopback address then no need associate to VRP_Socket */
    if (OSAL_TRUE == OSAL_netIsAddrLoopback(address_ptr)) {
        if (NULL != (sock_ptr = _VPR_srGetSocketById(_VPR_Obj_ptr,
                *socket_ptr))) {
            VPR_dbgPrintf("It's local loopback socket %d, free VRP_Socket.\n",
                    *socket_ptr);
            /* Free this VPR_SrSocket */
            if (OSAL_FAIL == _VPR_srFreeSocket(_VPR_Obj_ptr, *socket_ptr)) {
                VPR_dbgPrintf("Fail to free VPR_SrSocket %d\n", *socket_ptr);
            }
        }
    }    

    /* If in wifi mode, no need to bind socket for MSAPP */
    if (VPR_NETWORK_MODE_WIFI == _VPR_Obj_ptr->networkMode) {
        return (OSAL_SUCCESS);
    }
    return OSAL_netBindSocket(socket_ptr, address_ptr);
}

/* 
 * ======== SR_netGetSocketAddress() ========
 *
 * Wrapper function of net socket function.
 * It calls OSAL net function and does VPR processing for SR socket.
 *
 * Returns:
 *  OSAL_SUCCESS: Success.
 *  OSAL_FAIL:    Failure.
 */
OSAL_Status SR_netGetSocketAddress(
    OSAL_NetSockId  *socket_ptr,
    OSAL_NetAddress *address_ptr)
{
    OSAL_Status     status;
    VPR_SrSocket    *sock_ptr;
    OSAL_NetSockType type;

    /* Just return success in wifi mode. */
    if (VPR_NETWORK_MODE_WIFI == _VPR_Obj_ptr->networkMode) {
        return (OSAL_SUCCESS);
    }

    if (OSAL_SUCCESS == (status =
            OSAL_netGetSocketAddress(socket_ptr, address_ptr))) {
        /* Store address to VPR_SrSocket */
        if (NULL != (sock_ptr = _VPR_srGetSocketById(_VPR_Obj_ptr,
                *socket_ptr))) {
            /* Cache type */
            type = sock_ptr->localAddress.type;
            sock_ptr->localAddress = *address_ptr;
            /* Restore type */
            sock_ptr->localAddress.type = type;
        }
        else {
            VPR_dbgPrintf("Fail to find VPR_SrSocket %d for SR.\n", *socket_ptr);
        }
    }

    return (status);
}

/* 
 * ======== SR_netListenOnSocket() ========
 *
 * Wrapper function of net socket function.
 * It calls OSAL net function and does VPR processing for SR socket.
 *
 * Returns:
 *  OSAL_SUCCESS: Success.
 *  OSAL_FAIL:    Failure.
 */
OSAL_Status SR_netListenOnSocket(
    OSAL_NetSockId *socket_ptr)
{
    /* Just return success in wifi mode. */
    if (VPR_NETWORK_MODE_WIFI == _VPR_Obj_ptr->networkMode) {
        return (OSAL_SUCCESS);
    }

    return OSAL_netListenOnSocket(socket_ptr);
}

/* 
 * ======== SR_netAcceptOnSocket() ========
 *
 * Wrapper function of net socket function.
 * It calls OSAL net function and does VPR processing for SR socket.
 *
 * Returns:
 *  OSAL_SUCCESS: Success.
 *  OSAL_FAIL:    Failure.
 */
OSAL_Status SR_netAcceptOnSocket(
    OSAL_NetSockId  *socket_ptr,
    OSAL_NetSockId  *newSocket_ptr,
    OSAL_NetAddress *address_ptr)
{
    VPR_SrSocket   *sock_ptr;

    /* Just return success in wifi mode. */
    if (VPR_NETWORK_MODE_WIFI == _VPR_Obj_ptr->networkMode) {
        return (OSAL_SUCCESS);
    }

    if (OSAL_SUCCESS == OSAL_netAcceptOnSocket(socket_ptr, newSocket_ptr,
            address_ptr)) {
        /* use new socket to replace old socket in _VPR_Obj */
        if (NULL != (sock_ptr = _VPR_srGetSocketById(_VPR_Obj_ptr,
                *socket_ptr))) {
            sock_ptr->id = *newSocket_ptr;
        }

        return (OSAL_SUCCESS);
    }
    return (OSAL_FAIL);
}

/* 
 * ======== SR_netSocketReceive() ========
 *
 * Wrapper function of net socket function.
 * It calls OSAL net function and does VPR processing for SR socket.
 *
 * Returns:
 *  OSAL_SUCCESS: Success.
 *  OSAL_FAIL:    Failure.
 */
OSAL_Status SR_netSocketReceive(
    OSAL_NetSockId      *socket_ptr,
    void                *buf_ptr,
    vint                *size_ptr,
    OSAL_NetReceiveFlags flags)
{
    OSAL_Status ret;
    ret = OSAL_netSocketReceive(socket_ptr, buf_ptr, size_ptr, flags);
    return (ret);
}

/* 
 * ======== SR_netSocketReceiveFrom() ========
 *
 * Wrapper function of net socket function.
 * It calls OSAL net function and does VPR processing for SR socket.
 *
 * Returns:
 *  OSAL_SUCCESS: Success.
 *  OSAL_FAIL:    Failure.
 */
OSAL_Status SR_netSocketReceiveFrom(
    OSAL_NetSockId  *socket_ptr,
    void            *buf_ptr,
    vint            *size_ptr,
    OSAL_NetAddress *address_ptr)
{
    OSAL_Status ret;
    ret = OSAL_netSocketReceiveFrom(socket_ptr, buf_ptr, size_ptr, address_ptr);
    return (ret);
}

/* 
 * ======== SR_netSocketSend() ========
 *
 * Wrapper function of net socket function.
 * It calls OSAL net function and does VPR processing for SR socket.
 *
 * Returns:
 *  OSAL_SUCCESS: Success.
 *  OSAL_FAIL:    Failure.
 */
OSAL_Status SR_netSocketSend(
    OSAL_NetSockId  *socket_ptr,
    void            *buf_ptr,
    vint            *size_ptr)
{
    /* Just return success in wifi mode. */
    if (VPR_NETWORK_MODE_WIFI == _VPR_Obj_ptr->networkMode) {
        return (OSAL_SUCCESS);
    }
    return OSAL_netSocketSend(socket_ptr, buf_ptr, size_ptr);
}

/* 
 * ======== SR_netSocketSendTo() ========
 *
 * Wrapper function of net socket function.
 * It calls OSAL net function and does VPR processing for SR socket.
 *
 * Returns:
 *  OSAL_SUCCESS: Success.
 *  OSAL_FAIL:    Failure.
 */
OSAL_Status SR_netSocketSendTo(
    OSAL_NetSockId  *socket_ptr,
    void            *buf_ptr,
    vint            *size_ptr,
    OSAL_NetAddress *address_ptr)
{
    /* Just return success in wifi mode. */
    if (VPR_NETWORK_MODE_WIFI == _VPR_Obj_ptr->networkMode) {
        return (OSAL_SUCCESS);
    }
    return OSAL_netSocketSendTo(socket_ptr, buf_ptr, size_ptr, address_ptr);
}

/* 
 * ======== SR_netSetSocketOptions() ========
 *
 * Wrapper function of net socket function.
 * It calls OSAL net function and does VPR processing for SR socket.
 *
 * Returns:
 *  OSAL_SUCCESS: Success.
 *  OSAL_FAIL:    Failure.
 */
OSAL_Status SR_netSetSocketOptions(
    OSAL_NetSockId  *socket_ptr,
    OSAL_NetSockopt  option,
    int              value)
{
    return OSAL_netSetSocketOptions(socket_ptr, option, value);
}

/* 
 * ======== SR_netSslInit() ========
 *
 * Wrapper function of net socket function.
 * It calls OSAL net function and does VPR processing for SR socket.
 *
 * Returns:
 *  OSAL_SUCCESS: Success.
 *  OSAL_FAIL:    Failure.
 */
OSAL_Status SR_netSslInit(void)
{
    return OSAL_netSslInit();
}

/* 
 * ======== SR_netSslConnect() ========
 *
 * Wrapper function of net socket function.
 * It calls OSAL net function and does VPR processing for SR socket.
 *
 * Returns:
 *  OSAL_SUCCESS: Success.
 *  OSAL_FAIL:    Failure.
 */
OSAL_Status SR_netSslConnect(
    OSAL_NetSslId  *ssl_ptr,
    OSAL_NetSockId *socket_ptr)
{
    return OSAL_netSslConnect(ssl_ptr);
}

/* 
 * ======== SR_netSslAccept() ========
 *
 * Wrapper function of net socket function.
 * It calls OSAL net function and does VPR processing for SR socket.
 *
 * Returns:
 *  OSAL_SUCCESS: Success.
 *  OSAL_FAIL:    Failure.
 */
OSAL_Status SR_netSslAccept(
    OSAL_NetSslId  *ssl_ptr,
    OSAL_NetSockId *socket_ptr)
{
    return OSAL_netSslAccept(ssl_ptr);
}

/* 
 * ======== SR_netSsl() ========
 *
 * Wrapper function of net socket function.
 * It calls OSAL net function and does VPR processing for SR socket.
 *
 * Returns:
 *  OSAL_SUCCESS: Success.
 *  OSAL_FAIL:    Failure.
 */
OSAL_Status SR_netSsl(
    OSAL_NetSslId     *ssl_ptr,
    OSAL_NetSslMethod  method)
{
    return OSAL_netSsl(ssl_ptr, method);
}

/* 
 * ======== SR_netSslClose() ========
 *
 * Wrapper function of net socket function.
 * It calls OSAL net function and does VPR processing for SR socket.
 *
 * Returns:
 *  OSAL_SUCCESS: Success.
 *  OSAL_FAIL:    Failure.
 */
OSAL_Status SR_netSslClose(
    OSAL_NetSslId  *ssl_ptr,
    OSAL_NetSockId *socket_ptr)
{
    return OSAL_netSslClose(ssl_ptr);
}

/* 
 * ======== SR_netSslSend() ========
 *
 * Wrapper function of net socket function.
 * It calls OSAL net function and does VPR processing for SR socket.
 *
 * Returns:
 *  OSAL_SUCCESS: Success.
 *  OSAL_FAIL:    Failure.
 */
OSAL_Status SR_netSslSend(
    OSAL_NetSslId  *ssl_ptr,
    void           *buf_ptr,
    vint           *size_ptr,
    OSAL_NetSockId *socket_ptr)
{
    return OSAL_netSslSend(ssl_ptr, buf_ptr, size_ptr);
}

/* 
 * ======== SR_netSslReceive() ========
 *
 * Wrapper function of net socket function.
 * It calls OSAL net function and does VPR processing for SR socket.
 *
 * Returns:
 *  OSAL_SUCCESS: Success.
 *  OSAL_FAIL:    Failure.
 */
OSAL_Status SR_netSslReceive(
    OSAL_NetSslId  *ssl_ptr,
    void           *buf_ptr,
    vint           *size_ptr,
    OSAL_NetSockId *socket_ptr)
{
    return OSAL_netSslReceive(ssl_ptr, buf_ptr, size_ptr);
}

/* 
 * ======== SR_netSslSetSocket() ========
 *
 * Wrapper function of net socket function.
 * It calls OSAL net function and does VPR processing for SR socket.
 *
 * Returns:
 *  OSAL_SUCCESS: Success.
 *  OSAL_FAIL:    Failure.
 */
OSAL_Status SR_netSslSetSocket(
    OSAL_NetSockId *socket_ptr,
    OSAL_NetSslId  *ssl_ptr)
{
    VPR_SrSocket *sock_ptr;

    /* Look for VPR socket */
    if (NULL != (sock_ptr = _VPR_srGetSocketById(_VPR_Obj_ptr,
            *socket_ptr))) {
        /* Store OSAL_NetSslId in VPR socket */
        sock_ptr->ssl_ptr = ssl_ptr;
    }
    return OSAL_netSslSetSocket(socket_ptr, ssl_ptr);
}

/* 
 * ======== SR_netSslSetCert() ========
 *
 * Wrapper function of net socket function.
 * It calls OSAL net function and does VPR processing for SR socket.
 *
 * Returns:
 *  OSAL_SUCCESS: Success.
 *  OSAL_FAIL:    Failure.
 */
OSAL_Status SR_netSslSetCert(
    OSAL_NetSslId   *ssl_ptr,
    OSAL_NetSslCert *cert_ptr)
{
    return OSAL_netSslSetCert(ssl_ptr, cert_ptr);
}

/* 
 * ======== SR_netSslCertDestroy() ========
 *
 * Wrapper function of net socket function.
 * It calls OSAL net function and does VPR processing for SR socket.
 *
 * Returns:
 *  OSAL_SUCCESS: Success.
 *  OSAL_FAIL:    Failure.
 */
OSAL_Status SR_netSslCertDestroy(
    OSAL_NetSslCert  *cert_ptr)
{
    return OSAL_netSslCertDestroy(cert_ptr);
}

/* 
 * ======== SR_netSslCertGen() ========
 *
 * Wrapper function of net socket function.
 * It calls OSAL net function and does VPR processing for SR socket.
 *
 * Returns:
 *  OSAL_SUCCESS: Success.
 *  OSAL_FAIL:    Failure.
 */
OSAL_Status SR_netSslCertGen(
    OSAL_NetSslCert *cert_ptr,
    vint             bits,
    vint             serial,
    vint             days)
{
    return OSAL_netSslCertGen(cert_ptr, bits, serial, days);
}

/* 
 * ======== SR_netSslGetFingerprint() ========
 *
 * Wrapper function of net socket function.
 * It calls OSAL net function and does VPR processing for SR socket.
 *
 * Returns:
 *  OSAL_SUCCESS: Success.
 *  OSAL_FAIL:    Failure.
 */
OSAL_Status SR_netSslGetFingerprint(
    OSAL_NetSslCert *cert_ptr,
    OSAL_NetSslHash  hash,
    unsigned char   *fingerprint,
    vint             fpMaxLen,
    vint            *fpLen_ptr)
{
    return OSAL_netSslGetFingerprint(
            cert_ptr,
            hash,
            fingerprint,
            fpMaxLen,
            fpLen_ptr);
}

/* 
 * ======== SR_netResolve() ========
 *
 * Wrapper function of net socket function.
 * It calls OSAL net function and does VPR processing for SR socket.
 *
 * Returns:
 *  OSAL_SUCCESS: Success.
 *  OSAL_FAIL:    Failure.
 */
OSAL_Status SR_netResolve(
    int8                *query_ptr,
    int8                *ans_ptr,
    vint                 ansSize,
    vint                 timeoutsec,
    vint                 retries,
    OSAL_NetResolveType  qtype)
{
    return OSAL_netResolve(query_ptr, ans_ptr, ansSize, timeoutsec, retries,
            qtype);
}
