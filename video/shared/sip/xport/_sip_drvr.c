/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 28728 $ $Date: 2014-09-05 17:01:31 +0800 (Fri, 05 Sep 2014) $
 */

#include "sip_sip.h"
#include "sip_port.h"
#include "sip_parser_dec.h"
#include "sip_xport.h"
#include "_sip_descr.h"
#include "_sip_drvr.h"
#include "sip_mem_pool.h"
#include <sr.h>

static tpfTRANSPORT_RcvCB   pfTransClientCb = NULL;
static tpfTRANSPORT_RcvCB   pfTransServerCb = NULL;
static tpfTRANSPORT_RcvCB   pfTransErrorCb  = NULL;

static vint _TRANSDRVR_Decode(
    tSipIntMsg  **ppMsg, 
     char         *pB, 
     int           len)
{
    vint        status;
    tL4Packet   pkt;
        
    /* decode the message */
    OSAL_memSet(&pkt, 0, sizeof(tL4Packet));

    /* make sure that we don't shoot out of bounds */
    if (len > SIP_MAX_LAYER_4_PACKET_SIZE) 
        len = SIP_MAX_LAYER_4_PACKET_SIZE;
            
    pkt.frame = (tSipHandle)pB;
    pkt.length = (uint16)len;
    pkt.pCurr = (char *)pB;
    pkt.pStart = (char *)pB;

    /* Provide NULL termination for SAFETY! */
    (*(pB + len)) = 0;
    //SIP_DebugLog(SIP_DB_TRANSPORT_LVL_1, "INBOUND MSG: (len=%d.)\n%s\n",
    //        len, (int)pB, 0);
    SIP_DebugLog(SIP_DB_TRANSPORT_LVL_1, "INBOUND MSG: (len=%d.)\n", len, 0, 0);
#ifdef SIP_DEBUG_LOG
    OSAL_logString(pB);
#endif

    /* Discard small dummy packets */
    if (len <= SIP_DUMMY_PACKET_MAX) {
        return (SIP_NO_DATA);
    }

    *ppMsg = SIP_allocMsg();
    if (!*ppMsg) {
        SIP_DebugLog(SIP_DB_TRANSPORT_LVL_2,
                "_TRANSDRVR_Decode -no more memory\n", 0, 0, 0);
        return (SIP_NO_MEM);
    }
            
    SIP_DebugLog(SIP_DB_TRANSPORT_LVL_3,
            "_TRANSDRVR_Decode -Decoding message\n", 0, 0, 0);
    
    status = DEC_Msg(&pkt, *ppMsg, &((*ppMsg)->isCompactForm));
    if (status != SIP_OK)  {
        SIP_DebugLog(SIP_DB_TRANSPORT_LVL_1,
                "_TRANSDRVR_Decode -error when decoding...STATUS:%d\n",
                status, 0, 0);
        SIP_freeMsg(*ppMsg);
        return (SIP_FAILED);
    }
    return (status);
}

static vint _TRANSDRVR_CallBack(
    tSipIntMsg     *pMsg,
    tRemoteIpConn  *pRmtConn,
    tLocalIpConn   *pLclConn,
    tTransportType  type)
{
    tDLListEntry  *pEntry;
    vint           status;
    tViaHFE       *pVia;
    uint16         tmp[OSAL_NET_IPV6_WORD_SZ];

    if (pMsg->msgType == eSIP_REQUEST) {
        pEntry = NULL;
        if (DLLIST_GetNext(&pMsg->ViaList, &pEntry)) {
            pVia = (tViaHFE *)pEntry;
            /* Handle the received parameter */
            if (pVia->uri.host.addressType == eNwAddrDomainName) { 
                if (OSAL_netIsAddrIpv6(&pRmtConn->addrSet[0])) {
                    OSAL_netIpv6Ntoh(pVia->received.v6, pRmtConn->addrSet[0].ipv6);
                }
                else {
                    pVia->received.v4.ul = OSAL_netNtohl(pRmtConn->addrSet[0].ipv4);
                }
            }
            else if (pVia->uri.host.addressType == eNwAddrIPv6) {
                OSAL_netIpv6Ntoh(tmp, pRmtConn->addrSet[0].ipv6);
                if (OSAL_memCmp(pVia->uri.host.x.ip.v6, tmp,
                        sizeof(pVia->uri.host.x.ip.v6))) {
                    OSAL_netIpv6Ntoh(pVia->received.v6, pRmtConn->addrSet[0].ipv6);
                }
            }
            else if ((pVia->uri.host.addressType == eNwAddrIPv4) &&
                    (pVia->uri.host.x.ip.v4.ul !=
                    OSAL_netNtohl(pRmtConn->addrSet[0].ipv4)) ) {
                pVia->received.v4.ul = OSAL_netNtohl(pRmtConn->addrSet[0].ipv4);
            }

            /* handle the port.   If rport is non-zero
             * that means that we want the remote port 
             * to be the 'source' port in the IP header when 
             * we originally received the request.  Since this
             * was set as the default in TRANSDRVR_Recv we do 
             * nothing here.
             */
            if (pVia->rport == 0) {
                if (pVia->uri.host.port == 0) {
                    pRmtConn->addrSet[0].port =
                            OSAL_netHtons(SIP_DEFAULT_IPADDR_PORT);
                }
                else {
                    pRmtConn->addrSet[0].port =
                            OSAL_netHtons(pVia->uri.host.port);
                }
            }
            else {
                /* then's there a rport so set it up, we will 
                 * populate rport for responses to this transaction 
                 */
                pLclConn->rport = TRUE;
            }
            if ((pVia->uri.host.addressType == eNwAddrIPv6) &&
                    (!OSAL_netIpv6IsAddrZero(pVia->uri.maddr.v6))) {
                OSAL_netIpv6Hton(pRmtConn->addrSet[0].ipv6, pVia->uri.maddr.v6);
            }
            else if ((pVia->uri.host.addressType == eNwAddrIPv4) &&
                    (pVia->uri.maddr.v4.ul != 0)) {
                /* If the "maddr" parameter 
                 * exists in the 'Via' header field 
                 * then use that IP address.  See 18.2.2
                 */
                pRmtConn->addrSet[0].ipv4 = OSAL_netHtonl(pVia->uri.maddr.v4.ul);
            }
        }
        status = pfTransServerCb(type, pLclConn, pRmtConn, pMsg);
    }
    else {
        /* it's a response */
        pEntry = NULL;
        if (DLLIST_GetNext(&pMsg->ViaList, &pEntry)) {
            pVia = (tViaHFE *)pEntry;
            if (pVia->rport == 1) {
                /* If the rport in a response is '1' it means that 
                 * the rport parameter was present in the via but there 
                 * was no value in it. 
                 * For example: 10.0.0.1:5060;rport;branch=blahblah" 
                 * This behavior was observed by other vendors at SIPit.
                 * So we should zero it out.
                 */
                pVia->rport = 0;
            }
        }
        status = pfTransClientCb(type, pLclConn, pRmtConn, pMsg);
    }
    
    if (status != SIP_OK) {
        /* then it was not successfully handed off */
        SIP_DebugLog(SIP_DB_TRANSPORT_LVL_1,
                "_TRANSDRVR_CallBack: msg:%X not handed off, status:%d", 
                (int)pMsg, status, 0);
        SIP_freeMsg(pMsg);
    }
    return status;
}

static vint _TRANSDRVR_ErrCallBack(
    tSipIntMsg     *pMsg,
    tRemoteIpConn  *pRmtConn,
    tLocalIpConn   *pLclConn,
    tTransportType  type)
{
    pfTransErrorCb(type, pLclConn, pRmtConn, pMsg);
    return 0;
}

void TRANSDRVR_ProcessData(
    tTransportType type,    
    tLocalIpConn  *pLclConn, 
    tRemoteIpConn *pRmtConn, 
    char          *pBuffer, 
    vint           bufferLen,
    OSAL_NetSockId sockId)
{
    tSipIntMsg *pMsg;
    if (_TRANSDRVR_Decode(&pMsg, pBuffer, bufferLen) == SIP_OK) {
        /* Check if sip packet needs to be redirected */
        if (OSAL_SUCCESS == SR_processReceivedSipPacket(
                sockId, pMsg, pBuffer,bufferLen)) {
            /*
             * Sip packet is redirected, free pMsg and no need further process
             * here.
             */
            SIP_freeMsg(pMsg);
            return;
        }

        _TRANSDRVR_CallBack(pMsg, pRmtConn, pLclConn, type);
    }
    return;
}

void TRANSDRVR_ProcessErr(
    tTransportType type,
    tLocalIpConn  *pLclConn,
    tRemoteIpConn *pRmtConn,
    char          *pBuffer,
    vint           bufferLen,
    OSAL_NetSockId sockId)
{
    _TRANSDRVR_ErrCallBack(NULL, pRmtConn, pLclConn, type);
    return;
}

vint TRANSDRVR_Init(
    tpfTRANSPORT_RcvCB pfClientCb,
    tpfTRANSPORT_RcvCB pfServerCb,
    tpfTRANSPORT_RcvCB pfErrorCb)

{
    pfTransClientCb = pfClientCb;
    pfTransServerCb = pfServerCb;
    pfTransErrorCb  = pfErrorCb;
    
    return (SIP_OK);
}

void TRANSDRVR_KillModule(void)
{
    /* This should be called after TRANS_KillModule() */
    
    /* Clear out the callbacks to be safe */
    pfTransClientCb = NULL;
    pfTransServerCb = NULL;

    return;
}

void TRANSDRVR_DestroyTrans(
    tTransport *pTrans)
{
    if (pTrans->numUsers) {
        /* decrement the number of users */
        pTrans->numUsers--;    
    }

    /* if there are no more users then it's okay to delete */
    if (pTrans->numUsers <= 0) {
        /* a little extra protection here */
        pTrans->numUsers = 0;
        
        /* free the pTrans back to the pool */
        SIP_memPoolFree(eSIP_OBJECT_TRANSPORT, (tDLListEntry *)pTrans);
    }
    return;
}


tTransport* TRANSDRVR_CreateTrans(void) 
{
    tTransport   *pTrans;

    if (NULL == (pTrans = (tTransport *)SIP_memPoolAlloc(eSIP_OBJECT_TRANSPORT))) {
        return (NULL);
    }

    /* initialize the object */
    OSAL_memSet(pTrans, 0, sizeof(tTransport));
    pTrans->numUsers = 1;
    return (pTrans);
};



