/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 7821 $ $Date: 2008-10-14 15:50:50 -0500 (Tue, 14 Oct 2008) $
 */
#include <osal_types.h>
#include <osal.h>
#include "sip_sip.h"
#include "sip_xport.h"
#include "sip_list.h"
#include "_sip_tcpRecv.h"
#include "sip_mem_pool.h"
#include <ims_net.h>

#define TCPRECV_END_OF_PACKET   "\r\n\r\n"
#define TCPRECV_END_OF_LINE     "\r\n"
#define TCPRECV_CONTENT_LENGTH  "Content-Length:"
#define TCPRECV_SHORT_CONTENT_LENGTH "\r\nl:"


vint TCPRECV_initPacket(tTcpPacket *p_ptr, uint16 maxBufferLen)
{
    /* Allocate memory from memory pool */
    if (NULL == (p_ptr->pLayer4Buf =
            (tLayer4Buffer *)SIP_memPoolAlloc(eSIP_OBJECT_LAYER_4_BUF))) {
        return (SIP_FAILED);
    }

    p_ptr->begin_ptr = p_ptr->pLayer4Buf->buf;
    p_ptr->end_ptr = p_ptr->begin_ptr;
    p_ptr->len = p_ptr->contentLen = 0;
    p_ptr->maxLen = (vint)maxBufferLen;
    return (SIP_OK);
}

void TCPRECV_freePacket(tTcpPacket *p_ptr)
{
    if (p_ptr->pLayer4Buf) {
        SIP_memPoolFree(eSIP_OBJECT_LAYER_4_BUF,
                (tDLListEntry *)p_ptr->pLayer4Buf);
        p_ptr->pLayer4Buf = NULL;
    }

    p_ptr->begin_ptr = p_ptr->end_ptr = NULL;
    p_ptr->len = p_ptr->maxLen = p_ptr->contentLen = 0;
    return;
}

static void _TCPRECV_resetPacket(tTcpPacket *p_ptr)
{
    p_ptr->end_ptr = p_ptr->begin_ptr;
    p_ptr->len = p_ptr->contentLen = 0;
    return;
}

static vint _TCPRECV_getContentLength(
    char  *payload_ptr,
    vint   payloadLen)
{
    vint length = 0;
    char *cl_ptr;

    if (NULL != (cl_ptr =
            OSAL_strncasescan((const char*)payload_ptr,
            payloadLen, TCPRECV_CONTENT_LENGTH))) {
        /* Found the Content-Length header field */
        cl_ptr += sizeof(TCPRECV_CONTENT_LENGTH);
        if (' ' == *cl_ptr) cl_ptr++; /* Get rid of white space */
        length = OSAL_atoi(cl_ptr);
    }
    else if (NULL != (cl_ptr =
            OSAL_strncasescan((const char*)payload_ptr,
            payloadLen, TCPRECV_SHORT_CONTENT_LENGTH))) {
        /* Found the Content-Length header field */
        cl_ptr += sizeof(TCPRECV_SHORT_CONTENT_LENGTH);
        if (' ' == *cl_ptr) cl_ptr++; /* Get rid of white space */
        length = OSAL_atoi(cl_ptr);
    }
    return (length);
}

static char* _TCPRECV_getEnd(
    char  *payload_ptr,
    vint   payloadLen)
{
    char *end_ptr;

    if (NULL != (end_ptr = OSAL_strncasescan(payload_ptr, payloadLen,
            TCPRECV_END_OF_PACKET))) {
        /* Found the end of the document */
        end_ptr += (sizeof(TCPRECV_END_OF_PACKET) - 1);
    }
    return (end_ptr);
}

static OSAL_Status _TCPRECV_readData(
    OSAL_NetSockId sockId,
    vint          *len_ptr,
    tTcpPacket    *p_ptr)
{
    vint bytes = *len_ptr;
    if (bytes > (p_ptr->maxLen - p_ptr->len)) {
        bytes = (p_ptr->maxLen - p_ptr->len);
    }
    if (OSAL_SUCCESS != IMS_NET_SOCKET_RECEIVE(&sockId,
            p_ptr->end_ptr, &bytes, OSAL_NET_RECV_NO_FLAGS)) {
        return (OSAL_FAIL);
    }
    p_ptr->end_ptr += bytes;
    p_ptr->len += bytes;
    *len_ptr = bytes;
    return (OSAL_SUCCESS);
}


OSAL_Status TCPRECV_read(
    OSAL_NetSockId sockId,
    char          *payload_ptr,
    uint16         payloadLen,
    tTcpPacket    *p_ptr,
    vint          *readBytes_ptr)
{
    vint  ret;
    char *end_ptr;
    vint bytes;
    
    ret = payloadLen;
    if (OSAL_SUCCESS != IMS_NET_SOCKET_RECEIVE(&sockId, (void*)payload_ptr, &ret, OSAL_NET_RECV_PEEK)) {
        return (OSAL_FAIL);
    }
    if (0 == ret) {
        *readBytes_ptr = 0;
        _TCPRECV_resetPacket(p_ptr);
        return (OSAL_SUCCESS);
    }
    if (0 == p_ptr->contentLen) {
        /* Check for the end of the document */
        if (NULL == (end_ptr = _TCPRECV_getEnd(payload_ptr, ret))) {
            bytes = ret;
            if (OSAL_SUCCESS != _TCPRECV_readData(sockId, &bytes, p_ptr)) {
                return (OSAL_FAIL);
            }
            if (bytes != ret) {
                /* Then we are at the max size of the buffer or the socket closed */
                *readBytes_ptr = p_ptr->len;
                _TCPRECV_resetPacket(p_ptr);
                return (OSAL_SUCCESS);
            }
            /* There's still more */
            *readBytes_ptr = -1;
            return (OSAL_SUCCESS);
        }
        else {
            bytes = end_ptr - payload_ptr;
            if (OSAL_SUCCESS != _TCPRECV_readData(sockId, &bytes, p_ptr)) {
                return (OSAL_FAIL);
            }
            if (bytes != (end_ptr - payload_ptr)) {
                /* Then we are at the max size of the buffer or the socket closed */
                *readBytes_ptr = p_ptr->len;
                _TCPRECV_resetPacket(p_ptr);
                return (OSAL_SUCCESS);
            }
            /* Check for the content Len */
            if (0 == (p_ptr->contentLen = _TCPRECV_getContentLength(p_ptr->begin_ptr, p_ptr->len))) {
                /* then we have everything */
                *readBytes_ptr = p_ptr->len;
                _TCPRECV_resetPacket(p_ptr);
                return (OSAL_SUCCESS);
            }
            /* There's still more */
            *readBytes_ptr = -1;
            return (OSAL_SUCCESS);
        }
    }

    if (p_ptr->contentLen <= ret) {
        /* Then we have the rest */
        bytes = p_ptr->contentLen;
        if (OSAL_SUCCESS != _TCPRECV_readData(sockId, &bytes, p_ptr)) {
            return (OSAL_FAIL);
        }
        *readBytes_ptr = p_ptr->len;
        _TCPRECV_resetPacket(p_ptr);
        return (OSAL_SUCCESS);
    }
    else {
        /* We don't have it all yet, read as much as we can */
        bytes = ret;
        if (OSAL_SUCCESS != _TCPRECV_readData(sockId, &bytes, p_ptr)) {
            return (OSAL_FAIL);
        }
        if (bytes != ret) {
            /* Then we are at the max size of the buffer or the socket closed */
            *readBytes_ptr = p_ptr->len;
            _TCPRECV_resetPacket(p_ptr);
            return (OSAL_SUCCESS);
        }
        p_ptr->contentLen -= bytes;
        /* There's still more */
        *readBytes_ptr = -1;
        return (OSAL_SUCCESS);
    }
}

