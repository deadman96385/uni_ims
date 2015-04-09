/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 7251 $ $Date: 2008-08-06 17:00:42 -0500 (Wed, 06 Aug 2008) $
 */

#ifndef _SIP_DESCR_H__
#define _SIP_DESCR_H__

#include "_sip_tcpRecv.h"

/* A callback called when data has been read from the network interface */
typedef void (*tpfDescrDataCB)(
    tTransportType type,    
    tLocalIpConn  *pLclConn, 
    tRemoteIpConn *pRmtConn, 
    char          *pBuffer, 
    vint           bufferLen,
    OSAL_NetSockId sockId);

typedef enum eDescrMode
{
    eTRANSPORT_NONE = 0,
    eTRANSPORT_NORMAL,
    eTRANSPORT_PERSIST_SERVER,
    eTRANSPORT_PERSIST_CLIENT,
    eTRANSPORT_REUSE,
    eTRANSPORT_DONTCLOSESOCKET,
} tDescrMode;

typedef struct sDescr
{
    OSAL_NetSslId   ssl;
    OSAL_NetSockId  fd;
    OSAL_NetAddress lclAddr;
    OSAL_NetAddress rmtAddr;
    tDescrMode      mode;
    vint            isEnabled;
    tTransportType  type;
    uint32          ttl; /* For multicast only, 
                          * this is also used as a flag 
                          * to indictate multicast 
                          */
    tTcpPacket     packet;
    tNwAccess      nwAccess; /* Network access info. */
} tDescr;

typedef struct sDescrPool 
{
    tSipMutex       lock;
    tDescr          descr[SIP_MAX_TRANSPORT_DESCRIPTORS];
    OSAL_SelectSet  fdBitmap;
} tDescrPool;

vint TRANSDESCR_Init(
   tpfDescrDataCB    callBack,
   tpfDescrDataCB    errCallBack);

void TRANSDESCR_Destroy(
    void);

tDescr* TRANSDESCR_Alloc(
    OSAL_NetSockId   fd,
    OSAL_NetAddress *addr_ptr,
    tDescrMode       mode,
    tTransportType   type,
    tNwAccess       *nwAccess_ptr);

void TRANSDESCR_Free(
    tDescr          *pDescr);

void TRANSDESCR_Disable(
    tDescr *pDescr,
    vint    force);

void TRANSDESCR_Enable(
    tDescr          *pDescr);

tDescr* TRANSDESCR_MapFd(
    OSAL_NetSockId   fd);

tDescr* TRANSDESCR_MapPort(
    uint16           lclPort, 
    tTransportType   transType);

tDescr* TRANSDESCR_MapRemoteIp(
    OSAL_NetAddress rmtAddr[],
    vint            rmtAddrSize,
    tTransportType  transType,
    tDescrMode      mode);

void TRANSDESCR_CloseAll(
    void);

vint TRANSDESCR_Create(
    tDescr          *pDescr,
    OSAL_NetAddress *addr_ptr);

vint TRANSDESCR_TcpConn(
    tDescr          *pDescr,
    OSAL_NetAddress *pAddr);

vint TRANSDESCR_CreateTcpListen(
    tDescr          *pDescr);

vint TRANSDESCR_TlsConn(
    tDescr          *pDescr);

void TRANSDESCR_DestroySocket(
    tDescr *pDescr);

void TRANSDESCR_Recv(
    void);

vint TRANSDESCR_UdpSend(
    tDescr          *pDescr,
    OSAL_NetAddress *pAddr,
    char            *pData,
    uint32           dataLen);

vint TRANSDESCR_TcpSend(
    tDescr          *pDescr,
    char            *pData,
    uint32           dataLen);

vint TRANSDESCR_TlsSend(
    tDescr          *pDescr,
    char            *pData,
    uint32           dataLen);

vint TRANSDESCR_Switch(
    OSAL_NetSockId   fd, 
    vint             onOff);

vint TRANSDESCR_ReplaceInterace(
    uint16         oldPort,
    tTransportType transType,
    OSAL_NetSockId newFd,
    uint16         newPort);

tDescr* TRANSDESCR_MapLclAddress(
    OSAL_NetAddress *addr_ptr,
    tTransportType transType);

tDescr* TRANSDESCR_MapLclAddressPort(
    OSAL_NetAddress *addr_ptr,
    tTransportType transType);

tDescr* TRANSDESCR_MapLocalIp(
    OSAL_NetAddress *addr_ptr,
    tTransportType   transType,
    tDescrMode       mode);

void TRANSDESCR_terminate(
    tDescr       *pDescr);

vint TRANSDESCR_replaceServerSocket(
    OSAL_NetSockId   fd,
    OSAL_NetAddress *pAddr,
    tTransportType   tType,
    tNwAccess       *nwAccess_ptr,
    uint16           oldPort);

vint TRANSDESCR_TcpConn(
    tDescr          *pDescr,
    OSAL_NetAddress *pAddr);

void TRANSDESCR_Close(
    OSAL_NetSockId fd);

void TRANSDESCR_updateNwAccessInfo(
    tDescr    *descr_ptr,
    tNwAccess *nwAccess_ptr);

# endif
