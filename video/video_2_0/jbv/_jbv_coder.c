/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 5312 $ $Date: 2008-02-18 19:05:41 -0600 (Mon, 18 Feb 2008) $
 */

#include <osal.h>
#include "jbv.h"
#include "_jbv.h"

/*
 * ======== _JBV_ntohs() ========
 *
 * change net's byte sequence to host's.
 *
 */
inline unsigned short _JBV_ntohs(unsigned short data)
{
	unsigned int x;

	x = 1;

	/* if the host is little endian, byte reversal; otherwise no reversal */
	if (*((unsigned char *)&x))
	{
		/* little endian */
		return (((data & 0xFF00) >> 8) + ((data & 0x00FF) << 8));
	}
	else
	{
	        /* big endian */
		return data;
	}

}



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
         * unit_ptr->key = 1 -> I-Frame
         * unit_ptr->key = 0 -> P-Frame
         */
        if (4 == tmp) {
            /*
             * Mode-A
             */
            unit_ptr->key = !((pkt_ptr->data_ptr[1] >> 4) & 1);
        }
        else {
            /*
             * Mode-B or Mode-C
             */
            unit_ptr->key = !((pkt_ptr->data_ptr[4] >> 7) & 1);
        }
    }

    if (unit_ptr->key && pkt_ptr->mark) {
        obj_ptr->rtcpInfo.packetLoss.lostSinceIdr = 0;
        obj_ptr->rtcpInfo.keyFramesRecv ++;
    }

    /*
     * Store.
     */
    unit_ptr->ts = (uint64)pkt_ptr->tsOrig - (uint64)obj_ptr->firstUnNormTs;
    unit_ptr->ts = _JBV_90K_TO_USEC(unit_ptr->ts);
    unit_ptr->tsOrig = pkt_ptr->tsOrig;
    unit_ptr->atime = pkt_ptr->atime;
    unit_ptr->atime -=  obj_ptr->firstAtime;
    unit_ptr->mark = pkt_ptr->mark;
    unit_ptr->rcsRtpExtnPayload = pkt_ptr->rcsRtpExtnPayload;
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
            JBV_errLog("Invalid packet mode\n");
            obj_ptr->offset = 0;
            return (-1);
        }

        sbit = (buf_ptr[0] >> 3) & 7;
        ebit = buf_ptr[0] & 7;

        if ((obj_ptr->offset + pktsz) > (int)sizeof(obj_ptr->data)) {
            JBV_dbgLog("Increase buffer size\n");
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
            JBV_errLog("Mark not received\n");
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
        pkt_ptr->tsOrig = unit_ptr->tsOrig;
        pkt_ptr->type = unit_ptr->ptype;
        pkt_ptr->mark = 1;
        pkt_ptr->valid = 1;
        pkt_ptr->seqn = unit_ptr->seqn;
        pkt_ptr->rcsRtpExtnPayload = unit_ptr->rcsRtpExtnPayload;

        pkt_ptr->firstSeqn = obj_ptr->unit[m1Seqn].seqn;
        pkt_ptr->lastSeqn = obj_ptr->unit[mSeqn].seqn;

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
 * -1 : Failed. This means the packet got dropped
 * 0  : Success. This means packet was added to JBV
 */
vint _JBV_processH264(
    JBV_Obj  *obj_ptr,
    JBV_Unit *unit_ptr,
    JBV_Pkt  *pkt_ptr,
    vint        *updateFirstTs)
{
    uint8    *buf_ptr = pkt_ptr->data_ptr;
    uint8     nalu = H264_READ_NALU(buf_ptr[0]);
    uint64    ts;
    vint      tsOvfl;
    int64     tsDiff;
    uint64    tsLast;
    uint16    cseqn;
    uint16    seqn;
    vint      m1Seqn;
    uint32    elapsedTimeUs;
    JBV_PacketObserver *observer_ptr;
    JBV_Unit *unit1_ptr;

    tsOvfl = obj_ptr->tsOvfl;
    tsLast = obj_ptr->tsLast90K;
    observer_ptr = &obj_ptr->rtcpInfo.observer;

    /*
     * Is it key frame?
     * IDR or fragment IDR is key.
     *
     * These are the packet types we support from RFC:
     */
    unit_ptr->key = 0;

    //JBV_dbgLog("nalu:%d, pktSz:%d, seqn:%d\n", nalu, pkt_ptr->pSize, pkt_ptr->seqn);
    switch (nalu) {
        case NALU_NON_IDR:
        case NALU_PARTITION_A:
        case NALU_PARTITION_B:
        case NALU_PARTITION_C:
        case NALU_SEI:
        case NALU_SPS:
        case NALU_PPS:
        case NALU_STAP_A:
            break;
        case NALU_IDR:
            unit_ptr->key = 1;
            break;
        case NALU_FU_A:
            /* Check the FU Header for NAL unit type. */
            if (NALU_IDR == H264_READ_NALU(buf_ptr[1])) {
                unit_ptr->key = 1;
            }
            break;
        default:
            /* Invalid packet. Drop it by not adding it to the buffer. */
            _JBV_dropPacket(obj_ptr, NULL, JBV_DROP_INVALID);

            return(-1);
    }
    //JBV_dbgLog("nalu:%d, pktSz:%d, seqn:%d\n", nalu, pkt_ptr->pSize,
    //            pkt_ptr->seqn);

    /*
     * Correct time stamp wrap-around.
     * Timestamp may wrap around many times during a call.
     */
    ts = (uint64)pkt_ptr->tsOrig + _JBV_TS_RANGE * tsOvfl;
    tsDiff = ts - tsLast;
    if (tsDiff < -(_JBV_TS_RANGE >> 2)) {
        tsOvfl++;
        ts += _JBV_TS_RANGE;
        JBV_wrnLog("overflow tsDiff:%llu ts:%llu\n", tsDiff, ts);
    }
    if (tsDiff > (_JBV_TS_RANGE >> 2)) {
        /* Packet from past time reference. */
        ts -= _JBV_TS_RANGE;
    }

    if(ts < obj_ptr->firstUnNormTs){
        *updateFirstTs = 1;
        obj_ptr->firstUnNormTs = pkt_ptr->tsOrig;
        JBV_wrnLog("We got a packet that should arrive earlier than 1rst arrived one.  RTP_TS = %u, seq = %u",
                pkt_ptr->tsOrig, pkt_ptr->seqn);
    }

    /* Keep the 90K ts for restoring to obj later */
    tsLast = ts;

    /* Align to first pkt and conver to usec unit */
    ts -= (uint64)obj_ptr->firstUnNormTs;
    ts = _JBV_90K_TO_USEC(ts);

    /*
     * Check for packets that are too old.
     * Incoming packets that are older than last drawn frame.
     */
    if ((_JBV_TS_INIT != obj_ptr->lastDrawnTs) && (ts < obj_ptr->lastDrawnTs)) {
        /* Packet too Old.  Drop it by not adding it to the buffer. */
        _JBV_dropPacket(obj_ptr, unit_ptr, JBV_DROP_TOO_OLD);
        JBV_wrnLog("seqn:%d, Incoming packetTs:%llu, lastDrawnTs:%llu, tslast:%llu, firstUnNormTs:%llu\n",
                pkt_ptr->seqn, ts, obj_ptr->lastDrawnTs, tsLast, obj_ptr->firstUnNormTs);
        return (-1);
    }

    cseqn = JBV_SEQN_FROM_RTP_SEQN(pkt_ptr->seqn);
    unit1_ptr = &obj_ptr->unit[cseqn];

    /* Check for JBV overflow */
    if ((unit1_ptr->valid) && (unit1_ptr->ts != ts)) {
        /* JBV over flow. We are going to overwrite previous undrawn pkt. */
        _JBV_dropPacket(obj_ptr, unit_ptr, JBV_DROP_OVERFLOW);
        JBV_wrnLog("Overwrite location:%d seqn:%d ts:%llu",
                cseqn, pkt_ptr->seqn, ts);
    }
    /*
     * Is this first packet with firstInSeq of the sequence?
     * Find a packet with same time stamp, but lowest seqn.
     */
    seqn = JBV_PREVIOUS_SEQN(cseqn);
    m1Seqn = -1;
    while (1) {
        unit1_ptr = &obj_ptr->unit[seqn];

        if (!unit1_ptr->valid) {
            seqn = JBV_PREVIOUS_SEQN(seqn);
            if (seqn == cseqn) {
                break;
            }
            continue;
        }

        if ((ts == unit1_ptr->ts) && (unit1_ptr->firstInSeq)&&(!unit1_ptr->mark)) {
            /* Found the packet with firstInSeq */
            m1Seqn = seqn;
        }

        seqn = JBV_PREVIOUS_SEQN(seqn);
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
        if ((nalu == NALU_FU_A) && (H264_READ_FU_START_BIT(buf_ptr[1]) & 1) != 1) {
            /* If nalu is 28 and start bit is not set */
            unit_ptr->firstInSeq = 0;
        }
        else {
            /* Else */
            unit_ptr->firstInSeq = 1;
        }
    }
    else {
        /* Another packet with firstInSeq set exists with same TS. */
        unit_ptr->firstInSeq = 0;
    }
    /*
     * SPS marks first packet of new sequence.
     */
    if (NALU_SPS == nalu) {
        unit_ptr->firstInSeq = 1;
       /*bug480990 SPS,PPS and I frame(FU-A) with the same TS, but SPS&PPS mark==1 */
       /* if ((0 <= m1Seqn) &&
                (NALU_SPS != H264_READ_NALU(obj_ptr->unit[m1Seqn].data_ptr[0]))) {*/
            /*
             * There is already a packet with firstInSeq set
             * and it's not NALU_SPS, then clear it.
             */
       /*     obj_ptr->unit[m1Seqn].firstInSeq = 0;
        }*/
        /*sps pps with same timestamp, but sps has no mark*/
        pkt_ptr->mark = 1;
    }
    if(NALU_PPS == nalu)
    {
       unit_ptr->firstInSeq = 1;
       pkt_ptr->mark = 1;
       OSAL_logMsg("make marker pps firstinseq and mark");
    }

    /*
     * STAP-A marks first packet of new sequence
     */
    if (NALU_STAP_A == nalu) {
        JBV_dbgLog("recv a NALU_STAP_A pkt, mark is %d, fistInSeq %d -> %d\n",
                pkt_ptr->mark, unit_ptr->firstInSeq, 1);
        unit_ptr->firstInSeq = 1;
    }

    /* Update Observer with details of the new incoming packet. */
    observer_ptr->bytesReceived += pkt_ptr->pSize;
    observer_ptr->counter++;
    if (observer_ptr->counter == 1) {
        /* Upon reset, save the arrival time of the first packet observed. */
        observer_ptr->firstTs = pkt_ptr->atime;
    }
    else if (observer_ptr->counter == JBV_OBSERVATION_WINDOW) {
        /* Calculate elapsed time in microseconds between first and last packet observed. */
        elapsedTimeUs = (pkt_ptr->atime - observer_ptr->firstTs);
        /* Elapsed time must be positive non zero value. */
        if (elapsedTimeUs <= 0) {
            JBV_errLog("This should not happen!! currentTs:%llu tsFirst:%llu\n",
                    pkt_ptr->atime, observer_ptr->firstTs);
        }
        else {
            /* Calculate packet rate. (i.e) # packets per second. */
            observer_ptr->packetRate = ((JBV_OBSERVATION_WINDOW * 1000000) / elapsedTimeUs);
            /* Calculate bitrate in kbps of the video media received within the observation window. */
            observer_ptr->bitrateInKbps = (observer_ptr->bytesReceived * 8000 / elapsedTimeUs);
            /* Save the seqn of the last packet in the observation window for which these stats are applicable. */
            observer_ptr->lastSeqn = pkt_ptr->seqn;

            JBV_dbgLog("%u video media payload received at bitrate(kbps):%u at packet rate:%u\n",
                    JBV_OBSERVATION_WINDOW,
                    observer_ptr->bitrateInKbps, observer_ptr->packetRate);
        }
        /* Reset the Observer.*/
        observer_ptr->counter = 0;
        observer_ptr->bytesReceived = 0;
    }

    /* Store the paket as JBV_Unit. */
    obj_ptr->tsOvfl     = tsOvfl;
    obj_ptr->tsLast90K  = tsLast;
    if (unit_ptr->key && obj_ptr->tsLatestIdr != pkt_ptr->tsOrig) {
        obj_ptr->tsLatestIdr = pkt_ptr->tsOrig;
    }

    unit_ptr->ts        = ts;
    unit_ptr->nalu      = unit_ptr->key? NALU_IDR: nalu;
    unit_ptr->tsOrig    = pkt_ptr->tsOrig;
    unit_ptr->mark      = pkt_ptr->mark;
    unit_ptr->rcsRtpExtnPayload = pkt_ptr->rcsRtpExtnPayload;
    unit_ptr->atime     = pkt_ptr->atime;
    unit_ptr->atime    -= obj_ptr->firstAtime;
    unit_ptr->ptype     = pkt_ptr->type;
    unit_ptr->seqn      = pkt_ptr->seqn;
    unit_ptr->offset    = pkt_ptr->pSize;
    unit_ptr->valid     = 1;
    OSAL_memCpy(unit_ptr->data_ptr, pkt_ptr->data_ptr, pkt_ptr->pSize);

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
    uint32  *naluBitMask_ptr,
    JBV_Pkt *pkt_ptr)
{
    uint8     nalu;
    uint16    seqn;
    uint8    *buf_ptr;
    JBV_Unit *unit_ptr;
    int       sz;
    int       pktsz;
    int       offset;
    uint8     fuIndicator;
    uint8     fuHeader;
    uint8     startBit;
    uint8     nalType;
    uint8     reconstructedNal;
    vint      framesReceived;

    framesReceived = 0;
    *naluBitMask_ptr = 0;
    obj_ptr->offset = 0;
    seqn = m1Seqn;
    while (1) {

        unit_ptr = &obj_ptr->unit[seqn];

        buf_ptr = unit_ptr->data_ptr;
        pktsz = unit_ptr->offset;

        nalu = H264_READ_NALU(buf_ptr[0]);
        offset = obj_ptr->offset;

        if (NALU_SEI == nalu) {
            /*
             * Discard SEI. Optional and causes issues with OMX.
             * But keep the unit occupied so to find a sequence.
             */
        }
        else if ((NALU_NON_IDR == nalu) || (NALU_IDR == nalu)
                || (NALU_SPS == nalu) || (NALU_PPS == nalu) || (12 == nalu)) {

            /*
             * First 4 bytes are start sequence 0001.
             * Combine PPS, SPS, SEI, and its following packets till mark
             */
            if ((offset + pktsz + 4) > (int)sizeof(obj_ptr->data)) {
                JBV_dbgLog("Increase buffer size\n");
                obj_ptr->offset = 0;
                return (-1);
            }

            OSAL_memCpy((uint8 *)obj_ptr->data + offset + 4, buf_ptr, pktsz);
            obj_ptr->data[offset + 0] = 0;
            obj_ptr->data[offset + 1] = 0;
            obj_ptr->data[offset + 2] = 0;
            obj_ptr->data[offset + 3] = 1;

            obj_ptr->offset += pktsz + 4;

            /* Set nalu bit mask */
            *naluBitMask_ptr |= (1 << nalu);
        }
        else if (NALU_FU_A == nalu) {

            /*
             * FU-A. put in buf but dont increment buf until end bit
             * is received.
             */
            fuIndicator = *buf_ptr;
            fuHeader = *(buf_ptr + 1);
            startBit = H264_READ_FU_START_BIT(fuHeader) & 1;
            nalType = H264_READ_NALU(fuHeader);
#if defined(PROVIDER_LGUPLUS)
            reconstructedNal = (fuIndicator & 0xE0) | nalType | 0x40;
#else
            reconstructedNal = (fuIndicator & 0xE0) | nalType;
#endif
            /* skip the FU indicator, FU header */
            buf_ptr += 2;
            pktsz -= 2;

            if (startBit) {
                if (((int)sizeof(obj_ptr->data)) >=
                        (pktsz + 5)) {
                    OSAL_memCpy((uint8 *)obj_ptr->data + 5 + offset, buf_ptr, pktsz);
                }
                else {
                    JBV_dbgLog("Increase buffer size\n");
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
                    OSAL_memCpy((uint8 *)obj_ptr->data + offset,
                            buf_ptr, pktsz);
                    obj_ptr->offset += pktsz;
                }
                else {
                    JBV_dbgLog("Increase buffer size\n");
                    obj_ptr->offset = 0;
                    return (-1);
                }
            }

            /* Set nal bit mask */
            *naluBitMask_ptr |= (1 << nalType);
        }
        else if (NALU_STAP_A == nalu) {
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
                sz = _JBV_ntohs((unsigned short)*(unsigned short *)buf_ptr);

                buf_ptr += 2;
                pktsz -= 2;

                *naluBitMask_ptr |= (1 << H264_READ_NALU(buf_ptr[0]));

                if ((sz <= 0) || sz > pktsz) {
                    JBV_dbgLog("sz %d expire the range [0, %d]\n", sz, pktsz);
                    break;
                }

                /*
                 * First 4 bytes are start sequence 0001. Make sure the buffer
                 * fed is a complete NAL.
                 */
                if ((offset + 4 + sz) > (int)sizeof(obj_ptr->data)) {
                    JBV_dbgLog("increase buffer size, lost data\n");
                    return (-1);
                }
                OSAL_memCpy((uint8 *)obj_ptr->data + 4 + offset, buf_ptr, sz);

                obj_ptr->data[offset +  0] = 0;
                obj_ptr->data[offset +  1] = 0;
                obj_ptr->data[offset +  2] = 0;
                obj_ptr->data[offset +  3] = 1;

                offset += (sz + 4);

                pktsz -= sz;
                buf_ptr += sz;

                framesReceived++;

            } while (pktsz > 2);

            if (offset < 0) {
                return (-1);
            }
            obj_ptr->offset = offset;
        }
        else {
            JBV_errLog("Invalid packet mode\n");
            obj_ptr->offset = 0;
            return (-1);
        }

        if (1 == unit_ptr->mark) {
            if(NALU_STAP_A != nalu) {
                framesReceived++;
            }
            /* Packet complete.  */
            break;
        }

        if (seqn == mSeqn) {
            JBV_errLog("Mark not received\n");
            obj_ptr->offset = 0;
            return (-1);
        }

        /* Next packet */
        //JBV_dbgLog("at %d at loc %d\n", obj_ptr->offset, seqn);
        seqn = JBV_NEXT_SEQN(seqn);
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
        pkt_ptr->tsOrig = unit_ptr->tsOrig;
        pkt_ptr->type = unit_ptr->ptype;
        pkt_ptr->rcsRtpExtnPayload = unit_ptr->rcsRtpExtnPayload;
        pkt_ptr->mark = 1;
        pkt_ptr->valid = 1;
        pkt_ptr->seqn = unit_ptr->seqn;

        pkt_ptr->firstSeqn = obj_ptr->unit[m1Seqn].seqn;
        pkt_ptr->lastSeqn = obj_ptr->unit[mSeqn].seqn;
        pkt_ptr->naluBitMask = *naluBitMask_ptr;

        OSAL_logMsg("%s, mSeqn %u, m1Seqn %u, firstSeqn %u, lastSeqn %u\n",
                __FUNCTION__, mSeqn, m1Seqn, pkt_ptr->firstSeqn, pkt_ptr->lastSeqn);
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
            seqn = JBV_NEXT_SEQN(seqn);
        }
    }

  //  obj_ptr->totalFramesReceived += framesReceived;

    if (unit_ptr->key) {
        obj_ptr->rtcpInfo.packetLoss.lostSinceIdr = 0;
        obj_ptr->rtcpInfo.keyFramesRecv++;
    }

    return (0);
}
