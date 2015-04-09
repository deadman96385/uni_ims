/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 29950 $ $Date: 2014-11-20 09:42:47 +0800 (Thu, 20 Nov 2014) $
 */

#ifndef _SIP_RSRC_H_
#define _SIP_RSRC_H_

typedef struct {
    OSAL_TaskId  taskId;
    OSAL_MsgQId  msgQId;
} SIP_RsrcObj;

typedef struct  {
    SIP_RsrcObj     err;
    SIP_RsrcObj     nic;
    uint16          nicUdpPort; 
    OSAL_NetSockId  nicUdpFd;
    uint16          nicTcpPort; 
    OSAL_NetSockId  nicTcpFd;
} SIP_Rsrcs;

typedef int32 (*SIP_taskHandler)(void);

vint SIP_rsrcDispatcher(
    tSipHandle  hContext, 
    tSipIpcMsg *msg_ptr);

vint SIP_rsrcTmrDispatcher(
    tSipHandle  hContext, 
    tSipIpcMsg *msg_ptr);

vint SIP_rsrcInit(tSipConfig *pConfig);

void SIP_rsrcDestroy(void);

#endif
