/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 26841 $ $Date: 2014-06-10 16:49:31 +0800 (Tue, 10 Jun 2014) $
 */
#ifndef _SIP_XPORT_H_
#define _SIP_XPORT_H_

typedef struct sLocalIpConn
{
    OSAL_NetAddress addr;
    OSAL_NetSockId  fd;
    vint            rport;
    tNwAccess       nwAccess;
}tLocalIpConn;

typedef struct sRemoteIpConn
{
    OSAL_NetAddress addrSet[MAX_DNS_IP_ADDRESSES + 1];
    vint            addrIdx;
}tRemoteIpConn;

typedef struct sLayer4Buffer
{
    tDLListEntry    dll;
    char            buf[SIP_MAX_LAYER_4_PACKET_SIZE + 1];
}tLayer4Buffer;

typedef vint (*tpfTRANSPORT_ProxyCB)(
    char           *pAor,
    char           *pFqdn);

typedef vint (*tpfTRANSPORT_RcvCB)(
    tTransportType  transType,
    tLocalIpConn   *pLocalConn,
    tRemoteIpConn  *pRemoteConn,
    tSipIntMsg     *pMsg);

vint TRANSPORT_Init(
    tpfTRANSPORT_RcvCB pfClientCb,
    tpfTRANSPORT_RcvCB pfServerCb,
    tpfTRANSPORT_RcvCB pfErrorCb,
    uint32             mtu);

void TRANSPORT_KillModule(
    void);

void TRANSPORT_StartServer(
    uint16              UdpServerPort,
    OSAL_NetSockId      udpFd,
    uint16              TcpServerPort,
    OSAL_NetSockId      tcpFd);

vint TRANSPORT_replaceServerSocket(
    OSAL_NetSockId   newFd,
    OSAL_NetAddress *pAddr,
    tTransportType   tType,
    tNwAccess       *nwAccess_ptr);

tSipHandle TRANSPORT_ClientAlloc(
    tUri            *pUri,
    tLocalIpConn    *pLclConn,
    tTransportType   type);

tSipHandle TRANSPORT_ServerAlloc(
    tLocalIpConn       *pLocalConn,
    tRemoteIpConn      *pRemoteConn,
    tTransportType      transType);

vint TRANSPORT_Dealloc(
    tSipHandle          hTransport);

vint TRANSPORT_Send(
    tSipHandle          hTransport, 
    tSipIntMsg         *pMsgToSend);

tTransportType TRANSPORT_GetTransportType(
    tSipHandle          hTransport);

vint TRANSPORT_GetTransportAddr(
    tSipHandle          hTransport, 
    OSAL_NetAddress    *pAddr, 
    OSAL_NetSockId     *pFd);

void TRANSPORT_AddUser(
    tSipHandle          hTransport);

void TRANSPORT_SetLocalConn(
    tSipHandle          hTransport, 
    tLocalIpConn       *pLclConn);

vint TRANSPORT_AdvanceRmtAddr(
    tSipHandle          hTransport);

vint TRANSPORT_DescriptorSwitch(
    OSAL_NetSockId      fd, 
    vint                onOff);

vint TRANSPORT_ReplaceUdpDefault(
    OSAL_NetSockId    newUdpFd,
    uint16 newUdpPort);

vint TRANSPORT_ProxyMsg(
    tLocalIpConn       *pLclConn,
    tSipIntMsg         *pMsg,
    tUri               *pTargetUri);

vint TRANSPORT_ProxyError(
    tTransportType      transType,
    tLocalIpConn       *pLclConn,
    tRemoteIpConn      *pRmtConn,
    tSipIntMsg         *pMsg);

void TRANSPORT_CloseAllConnections(
    void);

void TRANSPORT_CloseConnection(
    OSAL_NetSockId fd);

vint TRANSPORT_updateAccessNwInfo(
    OSAL_NetSockId  fd,
    tNwAccess      *nwAccess_ptr);
#endif

