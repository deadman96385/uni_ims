/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004-2009 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 28518 $ $Date: 2014-08-27 18:43:57 +0800 (Wed, 27 Aug 2014) $
 *
 */

#include "vtspr.h"
#include "_vtspr_private.h"

/*
 * ======== VTSPR_netlogInit() ========
 *
 * XXX Until there is a shutdown function, bufs are never deallocated
 */
void _VTSPR_netlogInit(
    VTSPR_Obj *vtspr_ptr)
{
#ifdef VTSP_ENABLE_NETLOG
    vint           index;
    vint           bufNum;
    VTSPR_Netlog  *log_ptr;
    void          *sendBuf_ptr;

    log_ptr = vtspr_ptr->netlog_ptr;
    if (0 == log_ptr) {
        _VTSP_TRACE(__FUNCTION__, __LINE__);
        return;
    }

    for (index = VTSPR_NETLOG_NUM; index >= 0; index--) {
        /* Close any open UDP send sockets */
        if (log_ptr->socket_ary[index] > 0) {
            _VTSPR_netClose(log_ptr->socket_ary[index]);
            log_ptr->socket_ary[index] = 0;
        }
    }

    for (bufNum = 0; bufNum < VTSPR_NETLOG_SENDBUF_NUM; bufNum++) {
        sendBuf_ptr = log_ptr->sendBufList[bufNum];
        if (0 == sendBuf_ptr) {
            /* Alloc buffers */
            sendBuf_ptr = OSAL_memCalloc(1, VTSPR_NETLOG_SENDBUF_SZ, 0);
            log_ptr->sendBufList[bufNum] = sendBuf_ptr;
        }
        else {
            /* Zero buffers */
            OSAL_memSet(sendBuf_ptr, 0, VTSPR_NETLOG_SENDBUF_SZ);
        }
        log_ptr->sendBufLen[bufNum] = 0;
    }
    log_ptr->sendBufIndex = 0;
#endif
}

/*
 * ======== VTSPR_netlogSend() ========
 *
 * Units of bufLen is BYTES
 */
void _VTSPR_netlogSend(
    VTSPR_Obj  *vtspr_ptr,
    vint        bufId,
    void       *buf_ptr,
    vint        bufLen,
    vint        infc)
{
#ifdef VTSP_ENABLE_NETLOG
    uint8           *sendBuf_ptr;
    VTSPR_Netlog    *log_ptr;
    vint             bufNum;
    uint32           tmp;
    uint8           *tmp_ptr = (uint8 *)&tmp;

    log_ptr = vtspr_ptr->netlog_ptr;
    if (0 == log_ptr) {
        _VTSP_TRACE(__FILE__, __LINE__);
        return;
    }

    if (0 == (log_ptr->enable & (1 << infc))) {
        if (log_ptr->socket_ary[bufId] > 0) {
            _VTSPR_netClose(log_ptr->socket_ary[bufId]);
            log_ptr->socket_ary[bufId] = 0;
        }
        return;
    }

    if ((bufId >= VTSPR_NETLOG_NUM) || (bufId < 0)) {
        _VTSP_TRACE(__FILE__, __LINE__);
        return;
    }
    if ((bufLen < 0) || (bufLen + 12 > VTSPR_NETLOG_SENDBUF_SZ)) {
        _VTSP_TRACE(__FILE__, __LINE__);
        return;
    }

    bufNum = log_ptr->sendBufIndex;
    sendBuf_ptr = log_ptr->sendBufList[bufNum];
    if (0 == sendBuf_ptr) {
        _VTSP_TRACE(__FILE__, __LINE__);
        return;
    }
    log_ptr->sendBufIndex++;

    /* Open socket if not open yet */
    if (log_ptr->socket_ary[bufId] <= 0) {
        log_ptr->socket_ary[bufId] = _VTSPR_netSocket(OSAL_NET_SOCK_UDP, 0);
        log_ptr->ssrc_ary[bufId] = OSAL_netHtonl(10020 + bufId);
        log_ptr->ts_ary[bufId] = 0;
        log_ptr->seq_ary[bufId] = 0;
        log_ptr->sendPort_ary[bufId] = OSAL_netHtons(bufId + 10020);

        /* Set destination ip address from remoteIP configuration */
        log_ptr->sendAddr_ary[bufId] = OSAL_netHtonl(log_ptr->remoteIP);
    }
    if (OSAL_SUCCESS != _VTSPR_netIsSocketIdValid(log_ptr->socket_ary[bufId])) {
        /* Can not open socket */
        _VTSP_TRACE(__FILE__, __LINE__);
        return;
    }

    /* Create/Increment RTP header on UDP buf */
    *(sendBuf_ptr + 0)  = 0x80;    /* RTP ver + pad + ext + csrcNum */
    *(sendBuf_ptr + 1)  = 0x00;    /* pType */
    tmp = OSAL_netHtonl(log_ptr->seq_ary[bufId]);
    *(sendBuf_ptr + 2)  = tmp_ptr[0];
    *(sendBuf_ptr + 3)  = tmp_ptr[1];
    tmp = OSAL_netHtonl(log_ptr->ts_ary[bufId]);
    *(sendBuf_ptr + 4)  = tmp_ptr[0];
    *(sendBuf_ptr + 5)  = tmp_ptr[1];
    *(sendBuf_ptr + 6)  = tmp_ptr[2];
    *(sendBuf_ptr + 7)  = tmp_ptr[3];
    tmp = log_ptr->ssrc_ary[bufId];
    *(sendBuf_ptr + 8)  = tmp_ptr[0];
    *(sendBuf_ptr + 9)  = tmp_ptr[1];
    *(sendBuf_ptr + 10) = tmp_ptr[2];
    *(sendBuf_ptr + 11) = tmp_ptr[3];
    log_ptr->ts_ary[bufId] += (bufLen / sizeof(vint));
    log_ptr->seq_ary[bufId]++;
    if (0) {
        /* Send vint-buffers as uint32 payload */
        /* Copy buf to UDP buf.  bufLen is in bytes. */
        COMM_octetCopy((int8 *)(sendBuf_ptr + 12), (int8 *)buf_ptr, bufLen);
    }
    else {
        /* Send vint-buffers as Packed uint16 payload */
        /* Pack 32-bit words formatted as 0x0000xxxx into 16-bit buffer */
        vint     sampleCnt;
        vint     numSamples;
        int16   *dst_ptr;
        vint    *src_ptr;

        if (0 != (bufLen & 0x3)) {
            /* Cant handle odd lengthed buffers; must be word multiple */
            return;
        }

        /*
         * sending int16 words to netlog instead of vint
         */
        src_ptr = (vint *)buf_ptr;
        dst_ptr = (int16 *)(sendBuf_ptr + 12);
        numSamples = bufLen / sizeof(vint);
        bufLen = bufLen / sizeof(int16);
        for (sampleCnt = 0; sampleCnt < numSamples; sampleCnt++) {
            dst_ptr[sampleCnt] = (int16)src_ptr[sampleCnt];
        }
    }

    log_ptr->sendBufLen[bufNum] = bufLen + 12;
    log_ptr->sendBufSock[bufNum] = log_ptr->socket_ary[bufId];
    log_ptr->sendBufAddr[bufNum].ipv4 = log_ptr->sendAddr_ary[bufId];
    log_ptr->sendBufAddr[bufNum].port = log_ptr->sendPort_ary[bufId];

    if (log_ptr->sendBufIndex >= VTSPR_NETLOG_SENDBUF_NUM) {
        for (bufNum = 0; bufNum < log_ptr->sendBufIndex; bufNum++) {
            sendBuf_ptr = log_ptr->sendBufList[bufNum];
            bufLen      = log_ptr->sendBufLen[bufNum];
            if ((0 != log_ptr->sendBufSock[bufNum]) &&
                    (0 != log_ptr->sendBufAddr[bufNum].ipv4) &&
                    (0 != log_ptr->sendBufAddr[bufNum].port)) {
                /* Send UDP buf to socket */
                _VTSPR_netSendto(
                        log_ptr->sendBufSock[bufNum],
                        sendBuf_ptr,
                        bufLen,
                        log_ptr->sendBufAddr[bufNum]);
            }
            else {
                OSAL_logMsg("bufId=%d, socket_ary=%d, sendAddr_ary=%x,"
                        " sendPort_ary=%d", bufId, log_ptr->socket_ary[bufId],
                        log_ptr->sendAddr_ary[bufId],
                        log_ptr->sendPort_ary[bufId]);
            }
            log_ptr->sendBufSock[bufNum] = 0;
            log_ptr->sendBufAddr[bufNum].ipv4 = 0;
            log_ptr->sendBufAddr[bufNum].port = 0;
            log_ptr->sendBufLen[bufNum] = 0;
        }
        log_ptr->sendBufIndex = 0;
    }
    return;
#endif
}

