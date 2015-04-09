/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 5312 $ $Date: 2008-02-18 19:05:41 -0600 (Mon, 18 Feb 2008) $
 */

#include "h263.h"

/*
 * ======== _H263_encFindGobStart() ========
 * This function finds start position of a GOB.
 * in_ptr points to the start of bitstream
 * end_ptr points to the end of bitstream
 * start1_ptr will be set to the start of packet (start sequence removed), or
 *  NULL if no packet found
 * start2_ptr will be set to the end of packet or NULL if packet end is end of
 *  bitsream.
 * Returns:
 */
static void _H263_encFindGobStart(
    uint8  *in_ptr,
    uint8  *end_ptr,
    uint8 **start1_ptr,
    uint8 **start2_ptr)
{
    *start1_ptr = NULL;
    *start2_ptr = NULL;

    /*
     *  Find start code 16 0 bits and a 1 (its not byte aligned)
     */
    while (in_ptr < (end_ptr - 3)) {
    /*
     * FFMPEG byte aligns.
     */
#if 0
        if (0 == in_ptr[1]) {
            /*
             * Need to find 8 more 0 bits in in_ptr[0] and in_ptr[2]
             */

            byte = in_ptr[0];
            
            if (byte & 1) {
                pre = 0;
            }
            else if (byte & 2) {
                pre = 1;
            }
            else if (byte & 4) {
                pre = 2;
            }
            else if (byte & 8) {
                pre = 3;
            }
            else if (byte & 16) {
                pre = 4;
            }
            else if (byte & 32) {
                pre = 5;
            }
            else if (byte & 64) {
                pre = 6;
            }
            else if (byte & 128) {
                pre = 7;
            }
            else {
                pre = 8;
            }

            byte = in_ptr[2];
            if (byte & 128) {
                post = 0;
            }
            else if (byte & 64) {
                post = 1;
            }
            else if (byte & 32) {
                post = 2;
            }
            else if (byte & 16) {
                post = 3;
            }
            else if (byte & 8) {
                post = 4;
            }
            else if (byte & 4) {
                post = 5;
            }
            else if (byte & 2) {
                post = 6;
            }
            else if (byte & 1) {
                post = 7;
            }
            else {
                post = 8;
            }
        }
#else

        if (0 == in_ptr[0] && (0 == in_ptr[1]) && (0x80 & in_ptr[2])) {
            *start1_ptr = &in_ptr[0];
            break;
        }

#endif
        in_ptr++;
    }
    
    in_ptr += 3;
    
    while (in_ptr < (end_ptr - 3)) {
        if (0 == in_ptr[0] && (0 == in_ptr[1]) && (0x80 & in_ptr[2])) {
            *start2_ptr = &in_ptr[0];
            break;
        }

        in_ptr++;
    }

    if (NULL != *start1_ptr) {
        if (NULL == *start2_ptr) {
            *start2_ptr = end_ptr;
        }
    }
}

/*
 * ======== _H263_modeA_hdr() ========
 * This function generates H263 mode-A's header.
 *
 *   0                   1                   2                   3
 *   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |F|P|SBIT |EBIT | SRC |I|U|S|A|R      |DBQ| TRB |    TR         |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 * Note: F = 0, P = 0
 * Returns:
 */
void _H263_modeA_hdr(uint8 *buf_ptr, Video_EncObj *obj_ptr) {
    int tr;
    int trb;
    int dbq;
    int f;
    int ebit;
    int sbit;
    int src;
    int i;
    int u;
    int s;
    int a;
    int p;

    tr = ((buf_ptr[2] & 3) << 6) + (buf_ptr[3] >> 2);
    f = 0;
    ebit = 0;
    sbit = 0;
    src = (buf_ptr[4] >> 2) & 7;
    i = (buf_ptr[4] >> 1) & 1;
    u = (buf_ptr[4]) & 1;
    s = (buf_ptr[5] >> 7) & 1;
    a = (buf_ptr[5] >> 6) & 1;
    p = (buf_ptr[5] >> 5) & 1;

    obj_ptr->pkt.hdr[0] = (f << 7) + (p << 6) + (sbit << 3) + ebit;
    obj_ptr->pkt.hdr[1] =
            (src << 5) + (i << 4) + (u << 3) + (s << 2) + (a << 1);
    /* TRB 0 for mode A, DBQ 0 cause PB not used */
    trb = 0;
    dbq = 0;
    obj_ptr->pkt.hdr[2] = (dbq << 3) + trb;
    tr = 0; /* TR Must be 0 for mode A */
    obj_ptr->pkt.hdr[3] = tr;
}

/*
 * ======== _H263_modeB_hdr() ========
 * This function generates H263 mode-B's header.
 *
 *   0                   1                   2                   3
 *   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |F|P|SBIT |EBIT | SRC | QUANT   |  GOBN   |   MBA           |R  |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |I|U|S|A| HMV1        | VMV1        | HMV2        | VMV2        |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 * Note: F = 1, P = 0
 * Returns:
 */
void _H263_modeB_hdr(uint8 *buf_ptr, Video_EncObj *obj_ptr) {
    int f;
    int ebit;
    int sbit;
    int src;
    int i;
    int u;
    int s;
    int a;
    int p;
    //int quant;

    f = 1;
    ebit = 0;
    sbit = 0;
    src = (buf_ptr[4] >> 2) & 7;
    i = (buf_ptr[4] >> 1) & 1;
    u = (buf_ptr[4]) & 1;
    s = (buf_ptr[5] >> 7) & 1;
    a = (buf_ptr[5] >> 6) & 1;
    p = (buf_ptr[5] >> 5) & 1;
    if (p != 0)
    {
        DBG("Not supported mode: P = %d\n", p);
        return;
    }
    //quant = buf_ptr[5] & 31;

    obj_ptr->pkt.hdr[0] = (f << 7) + (p << 6) + (sbit << 3) + ebit;
    obj_ptr->pkt.hdr[1] = (src << 5);
    obj_ptr->pkt.hdr[4] = (i << 7) + (u << 6) + (s << 5) + (a << 4);
}

int H263_encGetData(
    Video_EncObj  *obj_ptr,
    Video_Packet *pkt_ptr,
    uint8 *data_ptr,
    vint szData,
    uint64 tsMs)
{
    int i;
    int index;
    int sz;
    int consumed;

    uint8 *start1_ptr;
    uint8 *start2_ptr;
    uint8 *buf_ptr;

    if ((NULL == obj_ptr) || (NULL == pkt_ptr)) {
        DBG("FAILURE\n");
        return (0);
    }

    if (NULL == pkt_ptr->buf_ptr) {
        DBG("FAILURE\n");
        return (0);
    }

    if (obj_ptr->pkt.num < 0) {

        /* Convert time stamp from ms to us. */
        obj_ptr->pts = tsMs * 1000;
        /* Copy the Data to a buffer. */
        OSAL_memCpy(obj_ptr->outbuf, data_ptr, szData);
        buf_ptr = (uint8*)obj_ptr->outbuf;

        if (szData <= 0) {
            return (0);
        }

        /*
         * 90000 from usec, div by 11
         */
        obj_ptr->pts /= 11;
        obj_ptr->len = szData;

        /*
         * Got a new buffer.
         */

        /*
         * Make header
         */
        if ((0 != buf_ptr[0]) &&
                (0 != buf_ptr[1]) &&
                (0x20 != (buf_ptr[2] >> 2))) {
            DBG("Invalid H263 packet\n");
            return (0);
        }

        if (0x2 != (buf_ptr[3] & 3)) {
            DBG("Invalid H263 packet\n");
            return (0);
        }


        memset(&obj_ptr->pkt, 0, sizeof(obj_ptr->pkt));
        obj_ptr->pkt.buf_ptr = (uint8 *)obj_ptr->outbuf;
        obj_ptr->pkt.end_ptr = (uint8 *)obj_ptr->outbuf + obj_ptr->len;
        obj_ptr->pkt.ts = obj_ptr->pts;

        /*
         * Run a loop to find GOB packets in a stream.
         */
        i = 0;
        _H263_encFindGobStart(
                obj_ptr->pkt.buf_ptr,
                obj_ptr->pkt.end_ptr,
                &obj_ptr->pkt.start1_ptr[i],
                &obj_ptr->pkt.start2_ptr[i]);

        if (NULL == obj_ptr->pkt.start1_ptr[i])  {
            /*
             * In case there is no start code in this coder.
             */
            DBG("No start code.");

            obj_ptr->pkt.start1_ptr[i] = obj_ptr->pkt.buf_ptr;
            obj_ptr->pkt.start2_ptr[i] = obj_ptr->pkt.end_ptr;
        }

        while (
                (NULL != obj_ptr->pkt.start1_ptr[i]) &&
                (NULL != obj_ptr->pkt.start2_ptr[i])
                ) {
            sz = obj_ptr->pkt.start2_ptr[i] - obj_ptr->pkt.start1_ptr[i];
            i++;
            if (i >= ENC_MAX_PKTS) {
                DBG("Losing data, increase packet buffer\n");
                break;
            }
            _H263_encFindGobStart(
                obj_ptr->pkt.start2_ptr[i - 1],
                obj_ptr->pkt.end_ptr,
                &obj_ptr->pkt.start1_ptr[i],
                &obj_ptr->pkt.start2_ptr[i]);
        }

        /*
         * Now we have a list of packets with start sequence removed.
         */
        obj_ptr->pkt.index = 0;
        obj_ptr->pkt.consumed = 0;
    }
    else {
        buf_ptr = (uint8*)obj_ptr->outbuf;
    }

    /*
     * Put some logic here to get H.263 RTP packets from H.263 packets.
     */
    index = obj_ptr->pkt.index;
    start1_ptr = obj_ptr->pkt.start1_ptr[index];
    start2_ptr = obj_ptr->pkt.start2_ptr[index];
    if ((NULL == start1_ptr) || (NULL == start2_ptr)) {
        /*
         * If no more data in buffer, mark it free, return.
         */
        obj_ptr->pkt.num = -1;
        obj_ptr->len = 0;
        return(0);
    }
    sz = start2_ptr - start1_ptr;

    consumed = obj_ptr->pkt.consumed;
    if (sz > (H263_ENC_MAX_RTP_SIZE - H263_MODE_A_HDR_SIZE)) {
        /*
         * Packet size is too large, fragment it! (Mode-B)
         */

        _H263_modeB_hdr(buf_ptr, obj_ptr);

        OSAL_memCpy(pkt_ptr->buf_ptr, obj_ptr->pkt.hdr, H263_MODE_B_HDR_SIZE);

        if(consumed == 0) {
            /*
             * First packet
             */
            OSAL_memCpy((char *)pkt_ptr->buf_ptr + H263_MODE_B_HDR_SIZE, (char *)start1_ptr + consumed, H263_ENC_MAX_RTP_SIZE - H263_MODE_B_HDR_SIZE);
            pkt_ptr->sz = H263_ENC_MAX_RTP_SIZE;
            pkt_ptr->mark = 0;
            obj_ptr->pkt.consumed += H263_ENC_MAX_RTP_SIZE - H263_MODE_B_HDR_SIZE;

        }
        else if ((consumed + H263_ENC_MAX_RTP_SIZE - H263_MODE_B_HDR_SIZE) < sz) {
            /*
             * Intermediate packet
             */
            OSAL_memCpy((char *)pkt_ptr->buf_ptr + H263_MODE_B_HDR_SIZE, (char *)start1_ptr + consumed, H263_ENC_MAX_RTP_SIZE - H263_MODE_B_HDR_SIZE);
            pkt_ptr->sz = H263_ENC_MAX_RTP_SIZE;
            pkt_ptr->mark = 0;
            obj_ptr->pkt.consumed += H263_ENC_MAX_RTP_SIZE - H263_MODE_B_HDR_SIZE;

        }
        else {
            /*
             * End packet
             */
            OSAL_memCpy((char *)pkt_ptr->buf_ptr + H263_MODE_B_HDR_SIZE, (char *)start1_ptr + consumed, sz - consumed);
            pkt_ptr->sz = sz - consumed + H263_MODE_B_HDR_SIZE;
            obj_ptr->pkt.consumed = 0;
            index++;
            obj_ptr->pkt.index = index;
            /*
             * If no more packets in this frame, mark it.
             */
            if (NULL == obj_ptr->pkt.start1_ptr[index]) {
                pkt_ptr->mark = 1;
            }
            else {
                pkt_ptr->mark = 0;
            }

        }
        pkt_ptr->ts = obj_ptr->pkt.ts;
        return (1);
    }
    else {
        /*
         * Will fit entirely in one RTP packet. (Mode-A)
         */

        _H263_modeA_hdr(buf_ptr, obj_ptr);

        OSAL_memCpy(pkt_ptr->buf_ptr, obj_ptr->pkt.hdr, H263_MODE_A_HDR_SIZE);
        OSAL_memCpy((char *)pkt_ptr->buf_ptr + H263_MODE_A_HDR_SIZE, (char *)start1_ptr, sz);
        pkt_ptr->sz = sz + H263_MODE_A_HDR_SIZE;
        pkt_ptr->ts = obj_ptr->pkt.ts;

        index++;

        if ((NULL == obj_ptr->pkt.start1_ptr[index])) {
            pkt_ptr->mark = 1;
        }
        else {
            pkt_ptr->mark = 0;
        }
        obj_ptr->pkt.consumed = 0;

        obj_ptr->pkt.index = index;
        return (1);
    }
}
