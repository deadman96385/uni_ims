/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 5312 $ $Date: 2008-02-18 19:05:41 -0600 (Mon, 18 Feb 2008) $
 */

#include <osal.h>
#include <vtsp_constant.h>
#include "jbv.h"
#include "_jbv.h"

/*
 * ======== _JBV_processH263() ========
 *
 * Process an incoming H263 packet.
 * Returns:
 * -1 : Failed.
 * 0  : Success
 */
vint _JBV_processH263(
    JBV_Obj  *obj_ptr,
    JBV_Unit *unit_ptr,
    JBV_Pkt  *pkt_ptr)
{
    vint tmp;

    /*
     * Find key packet and first in a sequence of sub packets.
     */
    tmp = _JBV_H263_MODE((pkt_ptr->data_ptr[0] >> 7) & 1,
            (pkt_ptr->data_ptr[0] >> 6) & 1);
    if (tmp < 0) {
        return (-1);
    }
    else {
        /*
         * First packet in sequence ? Tell by picture start code
         * 0000 0000 0000 0000 1000 00
         */
        if ((0 == pkt_ptr->data_ptr[tmp]) && (0 == pkt_ptr->data_ptr[tmp + 1])
                && (0x80 == (pkt_ptr->data_ptr[tmp + 2] & 0xFC))) {
            unit_ptr->firstInSeq = 1;
        }
        else {
            unit_ptr->firstInSeq = 0;
        }

        /*
         * Key packet (non P frame)?
         */
        if (4 == tmp) {
            unit_ptr->key = !((pkt_ptr->data_ptr[1] >> 4) & 1);
        }
        else {
            unit_ptr->key = !((pkt_ptr->data_ptr[4] >> 7) & 1);
        }
    }

    /*
     * Store.
     */
    unit_ptr->ts = (uint64)pkt_ptr->ts;
    unit_ptr->ts -= (uint64)obj_ptr->firstTs;
    unit_ptr->ts = _JBV_90K_TO_USEC(unit_ptr->ts);
    unit_ptr->atime = pkt_ptr->atime;
    unit_ptr->atime -=  obj_ptr->firstAtime;
    unit_ptr->mark = pkt_ptr->mark;
    unit_ptr->ptype = pkt_ptr->type;
    unit_ptr->seqn = pkt_ptr->seqn;
    unit_ptr->offset = pkt_ptr->pSize;
    unit_ptr->valid = 1;
    memcpy(unit_ptr->data_ptr, pkt_ptr->data_ptr, pkt_ptr->pSize);

    return (0);
}

/*
 * ======== _JBV_reassembeH263() ========
 *
 * Reassembles a packet sequence for H263.
 * Returns:
 * -1 : Sequence not ready, packet not ready
 * 0  : Sequence ready, packet ready
 */
vint _JBV_reassembleH263(
    JBV_Obj *obj_ptr,
    uint16   m1Seqn,
    uint16   mSeqn,
    JBV_Pkt *pkt_ptr)
{
    vint f;
    vint p;
    vint sbit;
    vint ebit;
    vint lastEbit;
    uint8 join1;
    uint8 join2;
    vint   mode;
    uint16 seqn;
    uint8 *buf_ptr;
    JBV_Unit *unit_ptr;
    vint pktsz;
    uint64 ts;

    lastEbit = 0;
    obj_ptr->offset = 0;
    seqn = m1Seqn;
    while (1) {

        unit_ptr = &obj_ptr->unit[seqn];

        buf_ptr = unit_ptr->data_ptr;
        pktsz = unit_ptr->offset;

        f = (buf_ptr[0] >> 7) & 1;
        p = (buf_ptr[0] >> 6) & 1;

        mode = _JBV_H263_MODE(f, p);
        if (mode < 0) {
            ERR("Invalid packet mode\n");
            obj_ptr->offset = 0;
            return (-1);
        }

        sbit = (buf_ptr[0] >> 3) & 7;
        ebit = buf_ptr[0] & 7;

        if ((obj_ptr->offset + pktsz) > (int)sizeof(obj_ptr->data)) {
            DBG("Increase buffer size\n");
            obj_ptr->offset = 0;
            return (-1);
        }

        /*
         * Copy data in buffer
         */
        if ((0 == lastEbit) || (0 == sbit)) {
            memcpy((uint8 *)obj_ptr->data + obj_ptr->offset,
                    buf_ptr + mode, pktsz - mode);
            obj_ptr->offset += pktsz - mode;
        }
        else {
            join1 = *((uint8 *)obj_ptr->data + obj_ptr->offset - 1);
            join1 >>= lastEbit;
            join1 <<= lastEbit;
            join2 = buf_ptr[mode];
            join2 <<= sbit;
            join2 >>= sbit;

            *((uint8 *)obj_ptr->data + obj_ptr->offset - 1) = join1 + join2;
            memcpy((uint8 *)obj_ptr->data + obj_ptr->offset,
                    buf_ptr + mode + 1, pktsz - mode - 1);
            obj_ptr->offset += pktsz - mode - 1;
        }

        if (1 == unit_ptr->mark) {
            /*
             * Packet complete.
             */
            break;
        }

        lastEbit = ebit;

        if (seqn == mSeqn) {
            ERR("Mark not received\n");
            obj_ptr->offset = 0;
            return (-1);
        }

        /*
         * Next packet
         */
        seqn = (seqn + 1) & (_JBV_SEQN_MAXDIFF - 1);
    }

    /*
     * Now return packet.
     */
    if (obj_ptr->offset > 0) {
        /*
         * Packet reassembled.
         */
        pkt_ptr->data_ptr = obj_ptr->data;
        pkt_ptr->pSize = obj_ptr->offset;
        pkt_ptr->atime = unit_ptr->atime;
        /*
         * This coders has time base of 90000
         */
        ts = _JBV_USEC_TO_90K(unit_ptr->ts);
        pkt_ptr->ts = (uint32)ts;
        pkt_ptr->type = unit_ptr->ptype;
        pkt_ptr->mark = 1;
        pkt_ptr->valid = 1;
        pkt_ptr->seqn = unit_ptr->seqn;

        /*
         * Mark sequence invalid.
         */
        seqn = m1Seqn;
        while (1) {
            unit_ptr =  &obj_ptr->unit[seqn];
            unit_ptr->valid = 0;
            if (seqn == mSeqn) {
                break;
            }
            seqn = (seqn + 1) & (_JBV_SEQN_MAXDIFF - 1);
        }
    }
    return (0);
}

/*
 * ======== _JBV_processH264() ========
 *
 * Process an incoming H264 packet.
 * Returns:
 * -1 : Failed.
 * 0  : Success
 */
vint _JBV_processH264(
    JBV_Obj  *obj_ptr,
    JBV_Unit *unit_ptr,
    JBV_Pkt  *pkt_ptr)
{
#ifdef MOSAIC_PREVENTION
    static uint8 latestSeqn = 0;
    static uint8 seqnInited = 0;
    static uint8 dropToNextKey = 0;
#endif
    uint8 *buf_ptr = pkt_ptr->data_ptr;
    uint8  nalu = buf_ptr[0] & 0x1F;
    uint64 ts;
    uint16 cseqn;
    uint16 seqn;
    vint m1Seqn;
    JBV_Unit *unit1_ptr;


    /*
     * Is it key frame?
     * IDR or fragment IDR is key.
     *
     * These are the packet types we support from RFC:
     */
    unit_ptr->key = 0;

    /*
     * Make sure we get SPS followed by PPS
     */
    if (7 == nalu) {
        obj_ptr->spsRecvd = 1;
        DBG("SPS packet: profile_idc 0x%02x, "
                "constraint_iop 0x%02x, "
                "level_idc 0x%02x\n",
                buf_ptr[1],
                buf_ptr[2],
                buf_ptr[3]);
    }
    if (0 == obj_ptr->spsRecvd) {
        ERR("Packet received before SPS. Discarding nalu %d\n", nalu);
        return(-1);
    }

    if (8 == nalu) {
        obj_ptr->ppsRecvd = 1;
        DBG("PPS packet\n");
    }
    if ((0 == obj_ptr->ppsRecvd) && (7 != nalu)) {
        ERR("Packet received before PPS. Discarding nalu %d\n", nalu);
        return(-1);
    }
    switch (nalu) {
        case 1:
            break;
        case 2:
            break;
        case 3:
            break;
        case 4:
            break;
        case 5:
            unit_ptr->key = 1;
            break;
        case 6:
            break;
        case 7:
            break;
        case 8:
            break;
        case 24:
            break;
        case 28:
            if (5 == (buf_ptr[1] & 0x1F)) {
                unit_ptr->key = 1;
            }
            break;
        default:
            ERR("Invalid packet mode\n");
            return(-1);
    }
    DBG("nalu=%d\n", nalu);

    ts = (uint64)pkt_ptr->ts;
    ts -= (uint64)obj_ptr->firstTs;
    ts = _JBV_90K_TO_USEC(ts);

    cseqn = pkt_ptr->seqn & (_JBV_SEQN_MAXDIFF - 1);

#ifdef MOSAIC_PREVENTION
    if (seqnInited == 0) {
        latestSeqn = ((cseqn - 1) & (_JBV_SEQN_MAXDIFF - 1));
        seqnInited = 1;
    }

    if (latestSeqn != ((cseqn - 1) & (_JBV_SEQN_MAXDIFF - 1))) {
        dropToNextKey = 1;
    }
#endif

    /*
     * Is this first pqcket with firstInSeq of the sequence?
     * Find a packet with same time stamp, but lowest seqn.
     */
    seqn = (cseqn - 1) & (_JBV_SEQN_MAXDIFF - 1);
    m1Seqn = -1;
    while (1) {
        unit1_ptr = &obj_ptr->unit[seqn];

        if (!unit1_ptr->valid) {
            seqn = (seqn - 1) & (_JBV_SEQN_MAXDIFF - 1);
            if (seqn == cseqn) {
                break;
            }
            continue;
        }

        if ((ts == unit1_ptr->ts) &&
                unit1_ptr->firstInSeq) {
            m1Seqn = seqn;
        }
        seqn = (seqn - 1) & (_JBV_SEQN_MAXDIFF - 1);
        if (seqn == cseqn) {
            break;
        }
    }

    /*
     * Now compare the seqn.
     */
    if (m1Seqn < 0) {
        /*
         * No other packet of same TS found with firstInSeqn set.
         * This packet is the first in the sequence
         */
        if ((nalu == 28) && 
            ((*(pkt_ptr->data_ptr+1) >> 7) & 1) != 1) {
            unit_ptr->firstInSeq = 0;
        } else {
            unit_ptr->firstInSeq = 1;
        }
    }
    else {
        /*
         * Another packet exists with same TS.
         */
        unit_ptr->firstInSeq = 0;

    }

    /*
     * SPS marks first packet of new sequence.
     */
    if (7 == nalu) {
        unit_ptr->firstInSeq = 1;
        /* If any packet with firstInSeq set, clear it */
        if (0 < m1Seqn) {
            obj_ptr->unit[m1Seqn].firstInSeq = 0;
        }    
    }

#ifdef MOSAIC_PREVENTION
    if ((unit_ptr->firstInSeq == 1) &&
        (dropToNextKey == 1) &&
        (unit_ptr->key == 1)) {
        dropToNextKey = 0;
    }

    if (dropToNextKey == 1) {
        return (-2);
    }
    else {
        latestSeqn = pkt_ptr->seqn & (_JBV_SEQN_MAXDIFF - 1); 
    }
#endif

    /*
     * Store.
     */
    unit_ptr->ts = ts;
    unit_ptr->mark = pkt_ptr->mark;
    unit_ptr->atime = pkt_ptr->atime;
    unit_ptr->atime -=  obj_ptr->firstAtime;
    unit_ptr->ptype = pkt_ptr->type;
    unit_ptr->seqn = pkt_ptr->seqn;
    unit_ptr->offset = pkt_ptr->pSize;
    unit_ptr->valid = 1;
    memcpy(unit_ptr->data_ptr, pkt_ptr->data_ptr, pkt_ptr->pSize);

    return (0);
}

/*
 * ======== _JBV_reassembeH264() ========
 *
 * Reassembles a packet sequence for H264.
 * Returns:
 * -1 : Sequence not ready, packet not ready
 * 0  : Sequence ready, packet ready
 */
vint _JBV_reassembleH264(
    JBV_Obj *obj_ptr,
    uint16   m1Seqn,
    uint16   mSeqn,
    JBV_Pkt *pkt_ptr)
{
    uint8 nalu;
    uint16 seqn;
    uint8 *buf_ptr;
    JBV_Unit *unit_ptr;
    uint64 ts;
    int sz;
    int pktsz;
    int offset;
    uint8 fuIndicator;
    uint8 fuHeader;
    uint8 startBit;
    uint8 endBit;
    uint8 nalType;
    uint8 reconstructedNal;


    obj_ptr->offset = 0;
    seqn = m1Seqn;
    while (1) {

        unit_ptr = &obj_ptr->unit[seqn];


        buf_ptr = unit_ptr->data_ptr;
        pktsz = unit_ptr->offset;

        nalu = buf_ptr[0] & 0x1F;
        offset = obj_ptr->offset;

        if (6 == nalu) {
            /*
             * Discard SEI. Optional and causes issues with OMX.
             * But keep the unit occupied so to find a sequence.
             */
        }
        else if ((1 == nalu) || (5 == nalu)
                || (7 == nalu) || (8 == nalu)) {
            /*
             * First 4 bytes are start sequence 0001.
             * Combine PPS, SPS, SEI, and its following packets till mark
             */
            if ((offset + pktsz + 4) > (int)sizeof(obj_ptr->data)) {
                DBG("Increase buffer size\n");
                obj_ptr->offset = 0;
                return (-1);
            }

            memcpy((uint8 *)obj_ptr->data + offset + 4, buf_ptr, pktsz);
            obj_ptr->data[offset + 0] = 0;
            obj_ptr->data[offset + 1] = 0;
            obj_ptr->data[offset + 2] = 0;
            obj_ptr->data[offset + 3] = 1;

            obj_ptr->offset += pktsz + 4;
        }
        else if (28 == nalu) {

            /*
             * FU-A. put in buf but dont increment buf until end bit
             * is received.
             */
            fuIndicator = *buf_ptr;
            fuHeader = *(buf_ptr + 1);
            startBit = (fuHeader >> 7) & 1;
            endBit = (fuHeader >> 6) & 1;
            nalType = fuHeader & 0x1F;
            reconstructedNal = (fuIndicator & 0xE0) | nalType;

            /*
             * skip the FU indicator, FU header
             */
            buf_ptr += 2;
            pktsz -= 2;

            if (startBit) {
                if (((int)sizeof(obj_ptr->data)) >=
                        (pktsz + 5)) {
                    memcpy((uint8 *)obj_ptr->data + 5 + offset, buf_ptr, pktsz);
                }
                else {
                    DBG("Increase buffer size\n");
                    obj_ptr->offset = 0;
                    return (-1);
                }
                obj_ptr->data[offset + 0] = 0;
                obj_ptr->data[offset + 1] = 0;
                obj_ptr->data[offset + 2] = 0;
                obj_ptr->data[offset + 3] = 1;
                obj_ptr->data[offset + 4] = reconstructedNal;
                obj_ptr->offset += pktsz + 5;
            }
            else {
                if (((int)sizeof(obj_ptr->data) - offset) >=
                        (pktsz)) {
                    memcpy((uint8 *)obj_ptr->data + offset,
                            buf_ptr, pktsz);
                    obj_ptr->offset += pktsz;
                }
                else {
                    DBG("Increase buffer size\n");
                    obj_ptr->offset = 0;
                    return (-1);
                }
            }
            if (endBit) {
            }
        }
        else if (24 == nalu) {
            /*
             * STAP-A (one packet, multiple nals: aggregation)
             * Insert start sequence between individual packets.
             */

            /*
             * Discard the STAP-A NAL header because it is an
             * encapsulation and H.264 does not care about it.
             */
            buf_ptr++;
            pktsz--;

            /*
             * Now get individual NAL units, add start sequence,
             * recombine them and give to decoder.
             */
            do {
                /*
                 * Get size of a NAL, then process it.
                 * Discard size because its part of encapsulation and
                 * H.264 does not care about it.
                 */
                sz = OSAL_netHtons((unsigned short)*(unsigned short *)buf_ptr);
                buf_ptr += 2;
                pktsz -= 2;

                if ((sz <= 0) || sz > pktsz) {
                    break;
                }

                /*
                 * First 4 bytes are start sequence 0001. Make sure the buffer
                 * fed is a complete NAL.
                 */
                if ((offset + 4 + sz) > (int)sizeof(obj_ptr->data)) {
                    OSAL_logMsg("%s %d: increase buffer size, lost data\n",
                            __FILE__, __LINE__);
                    return (0);
                }
                memcpy((uint8 *)obj_ptr->data + 4 + offset, buf_ptr, sz);

                obj_ptr->data[offset +  0] = 0;
                obj_ptr->data[offset +  1] = 0;
                obj_ptr->data[offset +  2] = 0;
                obj_ptr->data[offset +  3] = 1;

                offset += (sz + 4);

                pktsz -= sz;
                buf_ptr += sz;
            } while (pktsz > 2);

            if (offset < 0) {
                return (0);
            }
            obj_ptr->offset = offset;
        }
        else {
            ERR("Invalid packet mode\n");
            obj_ptr->offset = 0;
            return (-1);
        }


        if (1 == unit_ptr->mark) {
            /*
             * Packet complete.
             */
            break;
        }

        if (seqn == mSeqn) {
            ERR("Mark not received\n");
            obj_ptr->offset = 0;
            return (-1);
        }

        /*
         * Next packet
         */
        DBG("at %d at loc %d\n", obj_ptr->offset, seqn);
        seqn = (seqn + 1) & (_JBV_SEQN_MAXDIFF - 1);
    }

    /*
     * Now return packet.
     */
    if (obj_ptr->offset > 0) {
        /*
         * Packet reassembled.
         */
        pkt_ptr->data_ptr = obj_ptr->data;
        pkt_ptr->pSize = obj_ptr->offset;
        pkt_ptr->atime = unit_ptr->atime;
        /*
         * This coders has time base of 90000
         */
        ts = _JBV_USEC_TO_90K(unit_ptr->ts);
        pkt_ptr->ts = (uint32)ts;
        pkt_ptr->type = unit_ptr->ptype;
        pkt_ptr->mark = 1;
        pkt_ptr->valid = 1;
        pkt_ptr->seqn = unit_ptr->seqn;

        /*
         * Mark sequence invalid.
         */
        seqn = m1Seqn;
        while (1) {
            unit_ptr =  &obj_ptr->unit[seqn];
            unit_ptr->valid = 0;
            if (seqn == mSeqn) {
                break;
            }
            seqn = (seqn + 1) & (_JBV_SEQN_MAXDIFF - 1);
        }
    }
    return (0);
}

