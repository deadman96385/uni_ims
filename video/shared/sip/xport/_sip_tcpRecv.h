/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 7251 $ $Date: 2008-08-06 17:00:42 -0500 (Wed, 06 Aug 2008) $
 */

#ifndef _SIP_TCP_RECV_H__
#define _SIP_TCP_RECV_H__

typedef struct {
    char  *begin_ptr;
    char  *end_ptr;
    vint   len;
    vint   maxLen;
    vint   contentLen;
    tLayer4Buffer *pLayer4Buf;
} tTcpPacket;

#if 0
OSAL_Status TCPRECV_read(
    OSAL_NetSockId   sockId,
    char            *payload_ptr,
    vint            *payloadLen_ptr);
#endif

vint TCPRECV_initPacket(
    tTcpPacket *p_ptr,
    uint16      maxBufferLen);

void TCPRECV_freePacket(tTcpPacket *p_ptr);

OSAL_Status TCPRECV_read(
    OSAL_NetSockId sockId,
    char          *payload_ptr,
    uint16         payloadLen,
    tTcpPacket    *p_ptr,
    vint          *readBytes_ptr);

# endif
