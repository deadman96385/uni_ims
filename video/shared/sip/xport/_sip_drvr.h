/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 24709 $ $Date: 2014-02-20 17:27:12 +0800 (Thu, 20 Feb 2014) $
 */

#ifndef _SIP_DRVR_H__
#define _SIP_DRVR_H__

typedef struct {
    union {
        struct {
            uint16  priority;
            uint16  weight;
            uint16  port;
            char    target[MAX_DOMAIN_NAME_LEN + 1];
        }srv;
        struct {
            uint16 order;
            uint16 preference;
            char   flag[MAX_FLAG_STR_LEN + 1];
            char   service[MAX_SERVICE_STR_LEN + 1];
        }naptr;
    }u;
} tDnsAns;

typedef struct sTransport
{
    tDLListEntry    e; /* must be first */

    tRemoteIpConn   rmtConn;
    tLocalIpConn    lclConn;
    tDescr         *pDescr;
    int             numUsers;
    
    /* Scratch area for reading, writing dns entries */
    struct {
        tDnsAns     ans[MAX_DNS_SRV_ANS];
        char        buffer[MAX_DNS_BUFFER_SIZE_BYTES];
        char        domain[MAX_DOMAIN_NAME_LEN + 1];
    } dnsScratch;
}tTransport;

vint TRANSDRVR_Init(
    tpfTRANSPORT_RcvCB  pfClientCb,
    tpfTRANSPORT_RcvCB  pfServerCb,
    tpfTRANSPORT_RcvCB  pfErrorCb);

void TRANSDRVR_KillModule(
    void);

tTransport* TRANSDRVR_CreateTrans(
    void);

void TRANSDRVR_DestroyTrans(
    tTransport         *pTrans);

void TRANSDRVR_ProcessData(
    tTransportType      type,    
    tLocalIpConn       *pLclConn, 
    tRemoteIpConn      *pRmtConn, 
    char               *pBuffer, 
    vint                bufferLen,
    OSAL_NetSockId      sockId);

void TRANSDRVR_ProcessErr(
    tTransportType type,
    tLocalIpConn  *pLclConn,
    tRemoteIpConn *pRmtConn,
    char          *pBuffer,
    vint           bufferLen,
    OSAL_NetSockId sockId);

vint TRANSDRVR_ReplaceUdpDefault(
    OSAL_NetSockId    newUdpFd,
    uint16 newUdpPort);

#endif

