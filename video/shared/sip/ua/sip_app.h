/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 30336 $ $Date: 2014-12-11 10:24:15 +0800 (Thu, 11 Dec 2014) $
 */

#ifndef _SIP_APP_H_
#define _SIP_APP_H_

#include "sip_sip.h"

#include "sip_timers.h"
#include "sip_msgcodes.h"
#include "sip_xport.h"
#include "sip_xact.h"
#include "sip_ua.h"

/* These enumerations are used to to specifiy to the 
 * stack the field used to match incoming requests to UA's.  
 * For example, if sSipConfig.matchType == SIP_REQUEST_MATCH_REQUEST_URI
 * then the stack will try to match a UA to an incoming SIP request
 * based on the 'requestUri' in the SIP request.
 * If sSipConfig.matchType == SIP_REQUEST_MATCH_FROM_AOR, the the 
 * stack will try to match a UA to an incoming SIP request
 * based on the 'AOR' i the 'From:' header field.
 * NOTE: The default is SIP_REQUEST_MATCH_REQUEST_URI
 */
typedef enum sSipMatchType
{
    SIP_REQUEST_MATCH_NONE = 0,
    SIP_REQUEST_MATCH_REQUEST_URI,
    SIP_REQUEST_MATCH_FROM_AOR,
    SIP_REQUEST_MATCH_ANY,
}tSipMatchType;

/* configuration info used in SIP_Init() */
typedef struct sSipConfig
{
    uint32         randomGenSeed;   /* A value to seed the random number generator...typically ZERO */
    uint32         maxUA;           /* MAX number of UA's */
    uint8          maxDialogsPerUa; /* The max number of dialogs per UA */
    uint32         maxTransactions; /* max number of transactions, figure about 8 - 10 per UA */
    tSipMatchType  matchType;       /* if '0' or 'SIP_REQUEST_MATCH_NONE' then 
                                    * 'SIP_REQUEST_MATCH_REQUEST_URI' is assumed */
    tpfSipProxy    pfProxy;
    char          *pProxyFqdn; 
    uint16         udpPort; 
    OSAL_NetSockId udpFd;
    uint16         tcpPort; 
    OSAL_NetSockId tcpFd;
    uint32         mtu;
}tSipConfig;

vint SIP_Init(tSipConfig *pConfig);

vint SIP_Destroy(void);

vint SIP_SwitchNetworkInterface(
    OSAL_NetSockId  fd, 
    vint            onOff);

vint SIP_ReplaceUdpDefault(
    OSAL_NetSockId    newUdpFd,
    uint16 newUdpPort);

void SIP_CloseAllConnections(void);

void SIP_CloseConnection(
    OSAL_NetSockId fd);

void SIP_SetDebugLogLevel(
    uint32 moduleEnum, 
    int    level);

void SIP_DebugLogApp(
    int      level, 
    char    *pStr, 
    uint32   arg1, 
    uint32   arg2, 
    uint32   arg3);

void SIP_SetDebugLogCallBack(
    tpfSipDebugCB pfCallback);

tTransportType SIP_getTransportType(
    tSipHandle hUa);

vint SIP_replaceServerSocket(
    OSAL_NetSockId   newFd,
    OSAL_NetAddress *pAddr,
    tTransportType   tType,
    tNwAccess       *nwAccess_ptr);

vint SIP_updateAccessNwInfo(
    OSAL_NetSockId  fd,
    tNwAccess      *nwAccess_ptr);

vint SIP_clientConnect(
    tSipHandle      hUa,
    tLocalIpConn   *pLclConn,
    tTransportType  type);

void SIP_setTimers(
    uint32 t1,
    uint32 t2,
    uint32 t4);

#if defined(PROVIDER_CMCC)
void SIP_setTcallTimer(
    uint32  tcall);
#endif

#endif
