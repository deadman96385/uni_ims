/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL AND
 * PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2005-2010 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 28481 $ $Date: 2014-08-26 14:54:55 +0800 (Tue, 26 Aug 2014) $
 *
 */

#ifndef __VTSPR_NET_H_
#define __VTSPR_NET_H_
#include "osal.h"
#include "osal_net.h"

/*
 * Shared Variables
 */

/*
 * Prototypes
 */
OSAL_NetSockId _VTSPR_netSocket(
    OSAL_NetSockType type,
    int              flags);
vint _VTSPR_netClose(
    OSAL_NetSockId socketFd);
OSAL_Status _VTSPR_netIsSocketIdValid(
    OSAL_NetSockId  socketFd);
vint _VTSPR_netBind(
    OSAL_NetSockId  socketFd,
    OSAL_NetAddress address);
int _VTSPR_netRecvfrom(
    OSAL_NetSockId   socketFd,
    void            *buf_ptr,
    int              maxSize,
    OSAL_NetAddress *sentAddr);
uvint _VTSPR_netSendto(
    OSAL_NetSockId socketFd,
    void            *buf_ptr,
    int              bufSize,
    OSAL_NetAddress  sendAddr);
OSAL_Boolean _VTSPR_netIsSameAddr(
    OSAL_NetAddress addr1,
    OSAL_NetAddress addr2);
#endif /* End __VTSPR_NET_H_ */
