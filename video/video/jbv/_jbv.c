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
 * ======== _JBV_updateFramePeriod() ========
 *
 * Updates frame period.
 * Returns:
 */
void _JBV_updateFramePeriod(
    JBV_Obj  *obj_ptr,
    JBV_Unit *unit_ptr)
{
    vint avg;
    /*
     * Update frame period.
     */
    if (unit_ptr->firstInSeq) {
        /*
         * Init
         */
        if (0 == obj_ptr->frames) {
            obj_ptr->fpsTime = unit_ptr->atime;
        }
        obj_ptr->frames++;

        /*
         * Jitter will cause frames to arrive in bursts.
         * This logic can be tweaked.
         */
        if (obj_ptr->jitter > 500000) {
            avg = 64;
        }
        else if (obj_ptr->jitter > 300000) {
            avg = 32;
        }
        else if (obj_ptr->jitter > 100000) {
            avg = 16;
        }
        else {
            avg = 8;
        }
        if (obj_ptr->frames >= avg) {
            if (obj_ptr->fpsTime < unit_ptr->atime) {
                obj_ptr->framePeriod +=
                        (unit_ptr->atime - obj_ptr->fpsTime) / avg;
                obj_ptr->framePeriod >>= 1;
            }
            DBG("Frame period is %llu (%d fps)\n", obj_ptr->framePeriod,
                    (int)(1000000L / obj_ptr->framePeriod));
            obj_ptr->frames = 1;
            obj_ptr->fpsTime = unit_ptr->atime;
        }
    }
}

/*
 * ======== _JBV_updateJitter() ========
 *
 * Updates jitter when a new packet arrives.
 * Returns:
 */
void _JBV_updateJitter(
    JBV_Obj  *obj_ptr,
    JBV_Unit *unit_ptr)
{
    uint64 atime1;
    uint64 atime2;
    uint64 atdiff;
    uint64 tsdiff;
    uint64 jitter;
    uint64 jdiff;
    uint64    ts1;
    uint64    ts2;
    if ((obj_ptr->lastTs < unit_ptr->ts) &&
            (obj_ptr->lastSeqn < unit_ptr->seqn)) {
        ts1 = unit_ptr->ts;
        ts2 = obj_ptr->lastTs;
        atime1 = unit_ptr->atime;
        atime2 = obj_ptr->lastAtime;
        /*
         * Find jitter.
         * tsdiff - atdiff
         */
        tsdiff = ts1 - ts2;
        atdiff = atime1 - atime2;
        if (tsdiff > atdiff) {
            /*
             * Jitter calculation on late packets only because packets can be 
             * sitting in the network buffer and can arrive all at the same 
             * time and ruin jitter calculation.
             * 1 ms (1000) is a good time.
             */
            if (atdiff > 1000) {
                jitter = tsdiff - atdiff;
            }
            else {
                return;
            }
        }
        else {
            jitter = atdiff - tsdiff;
        }

        if (jitter > JBV_MAX_JITTER) {
            /*
             * Save against bad timestamp and bit errors in it.
             * If jitter limit is sky, we will never recover.
             */
            jitter = JBV_MAX_JITTER;
            DBG("jitter > JBV_MAX_JITTER");
        }

        if (jitter < obj_ptr->jitter) {
            /*
             * Jitter is decayed Q16, but we never go below current jitter.
             */
            jdiff = obj_ptr->jitter - jitter;
            jdiff *= obj_ptr->decayRt;
            jdiff >>= 16;
            jitter += jdiff;
        }
        if (obj_ptr->minJitter > jitter) {
            jitter = obj_ptr->minJitter;
        }
        obj_ptr->jitter = jitter;

        DBG("New jitter is %llu tsdiff=%llu atdiff=%llu\n",
                obj_ptr->jitter, tsdiff, atdiff);
    }
}

/*
 * ======== _JBV_findTsMax() ========
 *
 * Finds maximum ts of a complete packet.
 * Returns:
 * Time stamp.
 * Returns:
 * >=0 : Success, seqn of ts max
 * -1 : No packet
 */
vint _JBV_findTsMax(
    JBV_Obj  *obj_ptr,
    uint64   *ts_ptr)
{
    uint64    tsMax;
    uint64    foundTs;
    uint16    seqn;
    uint16    xseqn;
    vint      m1Seqn;
    JBV_Unit *unit_ptr;


    /*
     * Find the latest packet.
     */
    tsMax = 0;
    foundTs = tsMax;
    m1Seqn = -1;
    for (seqn = 0; seqn < _JBV_SEQN_MAXDIFF; seqn++) {
        unit_ptr = &obj_ptr->unit[seqn];
        if (!unit_ptr->valid) {
            tsMax = foundTs;
            continue;
        }
        if (tsMax < unit_ptr->ts) {
            tsMax = unit_ptr->ts;
        }
        if (tsMax == unit_ptr->ts) {
            /*
             * Find last packet in sequence
             */
            if (unit_ptr->mark) {
                /*
                 * Maximum seqn
                 */
                xseqn = seqn;

                /*
                 * Now find min seqn.
                 */
                while (1) {
                    if (!obj_ptr->unit[xseqn].valid) {
                        tsMax = foundTs;
                        break;
                    }
                    if (tsMax != obj_ptr->unit[xseqn].ts) {
                        tsMax = foundTs;
                        break;
                    }
                    if (obj_ptr->unit[xseqn].firstInSeq) {
                        m1Seqn = xseqn;
                        foundTs = tsMax;
                        //DBG("Latest complete packet is from %u to %u, "
                        //      "ts=%llu\n", m1Seqn, mSeqn, tsMax);
                        break;
                    }
                    xseqn = (xseqn - 1) & (_JBV_SEQN_MAXDIFF - 1);
                    if (xseqn == seqn) {
                        tsMax = foundTs;
                        break;
                    }
                }
            }
        }
    }

    if (m1Seqn < 0) {
        /*
         * No sequence complete in JB.
         */
        DBG("No sequence complete in JB");
        return (JB_NO_PACKETS_READY);
    }

    *ts_ptr = obj_ptr->unit[m1Seqn].ts;
    return (m1Seqn);
}


/*
 * ======== _JBV_cleanup() ========
 *
 * Drops too old and zombie packets.
 * Returns:
 */
void _JBV_cleanup(
    JBV_Obj  *obj_ptr)
{
    uint64    ts1;
    uint64    ts2;
    uint64    tsMax;
    uint16    seqn;
    uint8     nalu;
    JBV_Unit *unit_ptr;


    if (_JBV_findTsMax(obj_ptr, &tsMax) < 0) {
        DBG("No Packets Ready");
        return;
    }

    /*
     * Drop the packet older the last drawn.
     */
    ts1 = tsMax;
    ts2 = obj_ptr->lastDrawnTs;
    if (ts1 < ts2) {
        /*
         * Wrap around.
         */
        ts1 = 0;
    }
    else {
        ts1 = ts2;
    }

    /*
     * Now ts1 is minimum time stamp below which packets need to be discarded.
     */
    for (seqn = 0; seqn < _JBV_SEQN_MAXDIFF; seqn++) {
        unit_ptr = &obj_ptr->unit[seqn];
        if (!unit_ptr->valid) {
            continue;
        }

        /*
         * Drop. If dropped, wait till next key frame.
         */
        if (unit_ptr->ts < ts1) {

            /*
             * Do not drop H.264 SPS, PPS.
             */
            if (VTSP_CODER_VIDEO_H264 == unit_ptr->ptype) {
                nalu = unit_ptr->data_ptr[0] & 0x1F;
                if ((7 == nalu) || (8 == nalu) || (unit_ptr->key)) {
                    continue;
                }
            }

            unit_ptr->valid = 0;
            obj_ptr->drop++;
            DBG("********DROPPED %s packet at location"
                    " %u ts gap is %llu ts is %llu tsMax is %llu\n",
                    unit_ptr->key ? "key" : "non-key", seqn, ts1,
                    unit_ptr->ts, tsMax);
#ifdef JBV_DROP_KEY_TO_KEY
            /*
             * Will not work with coders that dont send key frame periodically
             */
            obj_ptr->dropTillNextKey = 1;
#endif
            continue;
        }
#ifdef JBV_DROP_KEY_TO_KEY
        /*
         * Will not work with coders that dont send key frame periodically
         */
        if (obj_ptr->dropTillNextKey) {
            /*
             * More than one key packets in buffer?
             */
            if (unit_ptr->key) {
                obj_ptr->dropTillNextKey = 0;
                continue;
            }
            unit_ptr->valid = 0;
            obj_ptr->drop++;
            ERR("********DROPPED %s packet at location %u ts gap is "
                    "%llu ts is %llu tsMax is %llu\n",
                    unit_ptr->key ? "key" : "non-key", seqn, ts1,
                    unit_ptr->ts, tsMax);
        }
#endif
    }
}

/*
 * ======== _JBV_findOldestSequence() ========
 *
 * Finds oldest sequence of packets in a group of segmented packets.
 * Also finds level with this packet in JB.
 * Returns:
 * 0 : Sequence found
 * -1 : Sequence not found
 */
vint _JBV_findOldestSequence(
    JBV_Obj  *obj_ptr,
    uint16   *firstSeq_ptr,
    uint16   *lastSeq_ptr,
    uint64   *level_ptr)
{
    vint mSeqn;
    vint m1Seqn;
    uint64 tsMin;
    uint16 seqn;
    uint16 xseqn;
    uint64 tsMax;
    vint maxSeqn;
    uint64 foundTs;
    JBV_Unit *unit_ptr;

    maxSeqn = _JBV_findTsMax(obj_ptr, &tsMax);
    if (maxSeqn < 0) {
        DBG("No Packets Ready");
        return (JB_NO_PACKETS_READY);
    }

    /*
     * Find oldest complete sequence.
     */
    tsMin = ~(uint64)0;
    foundTs = tsMin;
    mSeqn = -1;
    m1Seqn = -1;
    seqn = maxSeqn;
    while (1) {
        unit_ptr = &obj_ptr->unit[seqn];

        if (!unit_ptr->valid) {
            tsMin = foundTs;
            seqn = (seqn - 1) & (_JBV_SEQN_MAXDIFF - 1);
            if (seqn == maxSeqn) {
                break;
            }
            continue;
        }
        if (tsMin > unit_ptr->ts) {
            tsMin = unit_ptr->ts;
        }
        if (tsMin == unit_ptr->ts) {
            /*
             * Find last packet in sequence
             */
            if (unit_ptr->mark) {
                /*
                 * Maximum seqn
                 */
                xseqn = seqn;

                /*
                 * Now find min seqn.
                 */
                while (1) {
                    if (!obj_ptr->unit[xseqn].valid) {
                        tsMin = foundTs;
                        break;
                    }
                    if (tsMin != obj_ptr->unit[xseqn].ts) {
                        tsMin = foundTs;
                        break;
                    }
                    if (obj_ptr->unit[xseqn].firstInSeq) {
                        mSeqn = seqn;
                        m1Seqn = xseqn;
                        foundTs = tsMin;
                        break;
                    }
                    xseqn = (xseqn - 1) & (_JBV_SEQN_MAXDIFF - 1);
                    if (xseqn == seqn) {
                        tsMin = foundTs;
                        break;
                    }
                }
            }
        }
        seqn = (seqn - 1) & (_JBV_SEQN_MAXDIFF - 1);
        if (seqn == maxSeqn) {
            break;
        }
    }


    /*
     * Find how much data is in buffer.
     */

    if ((m1Seqn >= 0) && (mSeqn >= 0)) {
        *firstSeq_ptr = (uint16)m1Seqn;
        *lastSeq_ptr = (uint16)mSeqn;
        *level_ptr = (tsMax - tsMin);
        if (tsMax == tsMin) {
            *level_ptr = obj_ptr->framePeriod; /* We have one frame in */
        }
        return (0);
    }
    ERR("Sequence NOT FOUND maxSeqn=%d, m1Seqn=%d mSeqn=%d, tsMax=%llu, "
            "tsMin=%llu", maxSeqn, m1Seqn, mSeqn, tsMax, tsMin);
    return (JB_ERROR);
}
