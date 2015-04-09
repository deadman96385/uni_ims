/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 5312 $ $Date: 2008-02-18 19:05:41 -0600 (Mon, 18 Feb 2008) $
 */

#include "h264.h"

/*
 * ======== _H264_encFindFullPacket() ========
 * This function finds a packet in H.264 stream. Start of a packet is marked by
 * start code sequence.
 * in_ptr points to the start of bitstream
 * end_ptr points to the end of bitstream
 * start1_ptr will be set to the start of packet (start sequence removed), or
 *  NULL if no packet found
 * start2_ptr will be set to the end of packet or NULL if packet end is end of
 *  bitsream.
 * Returns:
 */
static void _H264_encFindFullPacket(
    uint8  *in_ptr,
    uint8  *end_ptr,
    uint8 **start1_ptr,
    uint8 **start2_ptr)
{
    *start1_ptr = NULL;
    *start2_ptr = NULL;

    /*
     *  Find start code 23 0 bits and a 1
     */
    while (in_ptr < (end_ptr - 3)) {
        if ((in_ptr[0] == 0) &&
                (in_ptr[1] == 0) &&
                (in_ptr[2] == 1)) {
            *start1_ptr = &in_ptr[3];
            break;
        }
        in_ptr++;
    }

    /*
     * Now start1_ptr points to the next start code.
     * If start sequence not found then return.
     */
    if (NULL == *start1_ptr) {
        return;
    }
    
    in_ptr = *start1_ptr;

    /*
     * Find next start code
     */
    while (in_ptr < (end_ptr - 3)) {
        if ((in_ptr[0] == 0) &&
                (in_ptr[1] == 0) &&
                (in_ptr[2] == 1)) {
            if (0 == in_ptr[-1]) {
                /*
                 * For 4 byte start code
                 */
                *start2_ptr = &in_ptr[-1];
            }
            else {
                *start2_ptr = &in_ptr[0];
            }
            break;
        }
        in_ptr++;
    }

    /*
     * Now start2_ptr points to the next start code (or its NULL).
     */
    if (NULL == *start2_ptr) {
        *start2_ptr = end_ptr;
    }
}

/*
 * ======== H264_encGetData() ========
 * This function gets encoded data from the encoder.
 * In pkt_ptr:
 * buf_ptr will be written to with packet data (buf_ptr must point to memory of
 *  minimum storage H264_MAX_PKT_SIZE.
 * sz will be written with packet size.
 * ts will be written with packet time stamp.
 * mark will be written with mark bit for the packet.
 * Returns:
 *  1: Success.
 *  0: Failed (no buffer ready.).
 */
int H264_encGetData(
    Video_EncObj  *obj_ptr,
    Video_Packet *pkt_ptr,
    uint8 *data_ptr,
    vint szData,
    uint64 tsMs)
{
    int i = -1;
    int index;
    int consumed;
    int sz;
    uint8 hdr[2];
    uint8 *start1_ptr;
    uint8 *start2_ptr;
    int nal;
    int end;

    if ((NULL == obj_ptr) || (NULL == pkt_ptr)) {
        DBG("FAILURE\n");
        return (0);
    }

    if (NULL == pkt_ptr->buf_ptr) {
        DBG("FAILURE\n");
        return (0);
    }

    end = 0;

    if (obj_ptr->pkt.num < 0) {

        /* Convert time stamp from ms to us. */
        obj_ptr->pts = tsMs * 1000;

        /* Copy the Data to a buffer. */
        memcpy(obj_ptr->outbuf, data_ptr, szData);

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
        OSAL_memSet(&obj_ptr->pkt, 0, sizeof(obj_ptr->pkt));
        obj_ptr->pkt.buf_ptr = (uint8 *)obj_ptr->outbuf;
        obj_ptr->pkt.end_ptr = (uint8 *)obj_ptr->outbuf + obj_ptr->len;
        obj_ptr->pkt.ts = obj_ptr->pts;

        /*
         * Run a loop to find packets in a stream.
         */
        i = 0;
        _H264_encFindFullPacket(
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
            i++;
            if (i >= ENC_MAX_PKTS) {
                DBG("Losing data, increase packet buffer\n");
                break;
            }
            _H264_encFindFullPacket(
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

    /*
     * Put some logic here to get H.264 RTP packets from H.264 packets.
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
        return (0);
    }

    sz = start2_ptr - start1_ptr;
    consumed = obj_ptr->pkt.consumed;
    nal = H264_READ_NALU(start1_ptr[0]);
    DBG("nal: %d", nal);

    /*
     * Don't send SEI if encoder generates it
     */
    if (NALU_SEI == nal) {
        DBG("Drop SEI");
        obj_ptr->pkt.num = -1;
        obj_ptr->len = 0;
        return (0);
    }

    if (sz <= H264_ENC_MAX_RTP_SIZE) {
        /*
         * Will fit entirely in one RTP packet.
         */
        OSAL_memCpy(pkt_ptr->buf_ptr, start1_ptr, sz);
        pkt_ptr->sz = sz;
        pkt_ptr->ts = obj_ptr->pkt.ts;

        index++;
        obj_ptr->pkt.index = index;
        obj_ptr->pkt.consumed = 0;
        /*
         * If no more packets in this frame, mark it.
         */
        if (NULL == obj_ptr->pkt.start1_ptr[index]) {
            pkt_ptr->mark = 1;
        }
        else {
            pkt_ptr->mark = 0;
        }
        obj_ptr->lastType = nal;
        return (1);
    }

    /*
     * Will need fragmenting.
     * Make header.
     */
    hdr[0] = NALU_FU_A | (start1_ptr[0] & 0x60);    /* FU - A, Type = 28, and NRI */
    hdr[1] = H264_READ_NALU(start1_ptr[0]);         /* Actual type */

    if (0 == consumed) {
        /*
         * First packet.
         */
        hdr[1] |= 1 << 7;        /* set start bit */
        OSAL_memCpy(pkt_ptr->buf_ptr, hdr, 2);
        OSAL_memCpy((char *)pkt_ptr->buf_ptr + 2, (char *)start1_ptr + 1, H264_ENC_MAX_RTP_SIZE - 2);
        pkt_ptr->sz = H264_ENC_MAX_RTP_SIZE;
        pkt_ptr->mark = 0;
        obj_ptr->pkt.consumed += H264_ENC_MAX_RTP_SIZE - 1;
    }
    else if ((consumed + H264_ENC_MAX_RTP_SIZE - 2) < sz) {
        /*
         * Intermediate packet.
         */
        OSAL_memCpy(pkt_ptr->buf_ptr, hdr, 2);
        OSAL_memCpy((char *)pkt_ptr->buf_ptr + 2, (char *)start1_ptr + consumed, H264_ENC_MAX_RTP_SIZE - 2);
        pkt_ptr->sz = H264_ENC_MAX_RTP_SIZE;
        pkt_ptr->mark = 0;
        obj_ptr->pkt.consumed += H264_ENC_MAX_RTP_SIZE - 2;
    }
    else {
        /*
         * End packet.
         */
        hdr[1] |= 1 << 6;        /* set end bit */
        OSAL_memCpy(pkt_ptr->buf_ptr, hdr, 2);
        OSAL_memCpy((char *)pkt_ptr->buf_ptr + 2, (char *)start1_ptr + consumed, sz - consumed);
        pkt_ptr->sz = sz - consumed + 2;
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
        end = 1;
    }

    pkt_ptr->ts = obj_ptr->pkt.ts;

    if (end) {
        obj_ptr->lastType = nal;
    }

    return (1);
}
