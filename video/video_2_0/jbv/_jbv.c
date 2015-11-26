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
 * Frame period is the ts difference between two adjacent frame.
 *
 * Returns: none
 */
void _JBV_updateFramePeriod(
    JBV_Obj  *obj_ptr,
    vint      seqn)
{
    JBV_Unit *unit_ptr;
    uint64    elapsedTime;

    unit_ptr = &obj_ptr->unit[seqn];

    /*
     * Update Frame period for every Frames.
     * Packets belonging to same frame is expected to have same time stamp.
     * Only update frame period if we detect time stamp change (which means we have a new frame).
     */
    if ((unit_ptr->mark) && (unit_ptr->nalu != NALU_PPS) && (unit_ptr->nalu != NALU_SPS)
            && (obj_ptr->lastSeqn < unit_ptr->seqn)) {
        if (obj_ptr->statisticFramesReceived> 1) {
            elapsedTime = unit_ptr->ts - obj_ptr->statisticFirstTs;
            if (obj_ptr->statisticFramesReceived < elapsedTime) {
                /* garuantee that obj_ptr->framePeriod is greater than 0 */
                obj_ptr->framePeriod = elapsedTime / (obj_ptr->statisticFramesReceived -1);
                JBV_dbgLog("DBG framePeriod=%llu, statisticFramesReceived=%d, elapsedTime=%llu\n",
                        obj_ptr->framePeriod, obj_ptr->statisticFramesReceived, elapsedTime);
                //if the framePeriod abnormal, restart the statistic data.
                if((obj_ptr->framePeriod > JBV_FRAME_PERIOD_MAX_USEC) || (obj_ptr->framePeriod < JBV_FRAME_PERIOD_MIN_USEC)){
                    JBV_wrnLog("framePeriod abnormal: framePeriod=%llu, statisticFramesReceived=%d,reset it\n",
                            obj_ptr->framePeriod, obj_ptr->statisticFramesReceived);
                    obj_ptr->statisticFirstTs = unit_ptr->ts;
                    obj_ptr->statisticFramesReceived = 1;
                    obj_ptr->framePeriod = JBV_INIT_FRAME_PERIOD_USEC;
                }
                else if(elapsedTime > (60*1000000)){
                // restart the statistic data when the statistic duration is too long.
                    JBV_wrnLog("statistic time longer than 1min, reset it\n");
                    obj_ptr->statisticFirstTs = unit_ptr->ts;
                    obj_ptr->statisticFramesReceived = 1;
                }
            } else {
                JBV_wrnLog("elapsedTime %u, obj_ptr->statisticFramesReceived %d\n",
                        elapsedTime, obj_ptr->statisticFramesReceived);
            }
/*
            JBV_dbgLog("currentSeqn:%hu currentTs:%llu firstTs:%llu"
                    "Frame period is %llu (%llu fps)"
                    "Frame Count:%d\n",
                    unit_ptr->seqn,    unit_ptr->ts, obj_ptr->firstTs,
                    obj_ptr->framePeriod,
                    1000000L / obj_ptr->framePeriod, obj_ptr->totalFramesReceived);
*/
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
    uint64 jitter;
    uint64 jdiff;
    uint64 fps;
    int64  currentDelay;

    /*
     * Calculate Jitter based on Frames.
     * Packets belonging to same frame is expected to have same time stamp.
     * Only update jitter if we detect time stamp change (which means we have a new frame).
     */
    if ((obj_ptr->lastTs < unit_ptr->ts) && (obj_ptr->lastSeqn < unit_ptr->seqn)) {

        /*
         * Calculate the difference between arrival time and time stamp.
         * This could be negative as the clocks may not be synchronized.
         */
        currentDelay = unit_ptr->atime - unit_ptr->ts;

        if (obj_ptr->minDelay == 0) {
            /* First packet after JBV_init. currentDelay will be the minDelay. */
            obj_ptr->minDelay = currentDelay;
        }

        if (currentDelay < obj_ptr->minDelay) {
            /*
             * We have received a packet with minimum delay.
             * Update minDelay. Current packet jitter is 0.
             */
            obj_ptr->minDelay = currentDelay;
            jitter = 0;
        }
        else {
            /*
             * Calculate Jitter. (RFC 5481 - Section 4.2)
             * PDV(i) = D(i) - Dmin
             * This will always be positive.
             */
            jitter = currentDelay - obj_ptr->minDelay;
        }


        if (jitter > JBV_MAX_JITTER_USEC) {
            /*
             * Save against bad timestamp and bit errors in it.
             * If jitter limit is sky, we will never recover.
             */
            jitter = JBV_MAX_JITTER_USEC;
            JBV_dbgLog("jitter > JBV_MAX_JITTER\n");
        }

        if (jitter < obj_ptr->jitter) {
            /*
             * Current packet jitter is less than
             * Decay jitter by 1% of the jitter difference.
             */
            /* Jitter is decayed Q16, but we never go below current jitter.  */

            jdiff = obj_ptr->jitter - jitter;
            jdiff *= JBV_DECAY_RATE_NUMERATOR;
            jdiff >>= 16;
            jitter += jdiff;
        }
        else {
            /*
             * Current packet jitter is more
             * Increase jitter based on Input fps
             */
            jdiff = jitter - obj_ptr->jitter;
            if(0 == obj_ptr->framePeriod )
	    {
		obj_ptr->framePeriod = 50000;
	        JBV_errLog("JBV framePeriod is 0,change to 50ms");
            }
            fps = 1000000 / obj_ptr->framePeriod;
            /* Don't go lower than 1 frame per second. */
            if (fps < 1) {
                fps = 1;
            }

            jitter = obj_ptr->jitter + (jdiff/fps);
        }

        obj_ptr->jitter = jitter;
/*
        JBV_dbgLog("New jitter is %llu currentDelay=%lld minDelay=%lld\n",
                obj_ptr->jitter, currentDelay, obj_ptr->minDelay);
*/
    }
}

/*
 * ======== _JBV_ceilPrec() ========
 * Does ceiling on a number while adjusting to a precision.
 *
 * Return Values:
 * Ceiled and quantized to prec output for input.
 */
static uint64 _JBV_ceilPrec(
    uint64 input,
    uint64 prec)
{
    uint64 rem;

    rem = input % prec;
    if (0 != rem) {             /* Avoid accumulation */
        input = input - rem;
        input = input + prec;
    }

    return (input);
}

/*
 * ======== _JBV_stateMachine() ========
 * Runs the JBV level state machine after calculating the level.
 *
 * Return Values:
 * New level state.
 */
int32 _JBV_stateMachine(
    int32  state,
    uint64  jitter,
    uint64 level,
    uint64 framePeriod)
{
    uint64 epy;
    uint64 nl;
    uint64 nr;
    uint64 nh;
    uint64 top;

    /*
     * Ceiled and quantized jitter to framePeriod.
     */
    jitter = _JBV_ceilPrec(jitter, framePeriod);

    epy = 0;
    /* Nominal low */
    nl = (jitter < framePeriod) ? 0 : jitter - framePeriod;
    nl = (nl < (jitter >> 1)) ? nl : (jitter >> 1); /* Find the min fo two. */
    /* Normal level */
    nr  = jitter;
    /* Nominal high */
    nh  = jitter + framePeriod;
    /* Top level */
    top = jitter << 1;

    //JBV_dbgLog("JBV SM jitter=%llu, lvl=%llu, epy=%llu, nl=%llu, nr=%llu, nh=%llu, "
    //        "top=%llu\n", jitter, level, epy, nl, nr, nh, top);

    /*
     * If below or equal to empty level, switch to empty state.
     */
    if (level <= epy) {
        JBV_wrnLog("JBV_STATE_EMPTY level=%llu, epy=%llu, jitter=%llu, framePeriod=%llu (%llu fps)\n", level, epy, jitter, framePeriod, 1000000L / framePeriod);
        state = JBV_STATE_EMPTY;
    }
    /*
     * If below nominal low and last state was empty, keep state.
     * If below nominal low and last state was not empty, switch to accm state.
     */
    else if (level <= nl) {
        if (JBV_STATE_EMPTY == state) {
            JBV_dbgLog("JBV_STATE_EMPTY level=%llu, nl=%llu, jitter=%llu, framePeriod=%llu (%llu fps)\n", level, nl, jitter, framePeriod, 1000000L / framePeriod);
        }
        else {
            JBV_dbgLog("JBV_STATE_ACCM level=%llu, nl=%llu\n, jitter=%llu, framePeriod=%llu (%llu fps)\n", level, nl, jitter, framePeriod, 1000000L / framePeriod);
            state = JBV_STATE_ACCM;
        }
    }
    /*
     * If below normal level, switch to accum state.
     */
    else if (level <= nr) {
        JBV_dbgLog("JBV_STATE_ACCM level=%llu, nr=%llu, jitter=%llu, framePeriod=%llu (%llu fps)\n", level, nr, jitter, framePeriod, 1000000L / framePeriod);
        state = JBV_STATE_ACCM;
    }
    /*
     * If below nominal high, switch to normal state.
     */
    else if (level <= nh) {
        JBV_dbgLog("JBV_STATE_NORM level=%llu, nh=%llu, jitter=%llu, framePeriod=%llu (%llu fps)\n", level, nh, jitter, framePeriod, 1000000L / framePeriod);
        state = JBV_STATE_NORM;
    }
    /*
     * If below or at full level, switch to leak state.
     */
    else if (level <= top) {
        JBV_wrnLog("JBV_STATE_LEAK level=%llu, top=%llu, jitter=%llu, framePeriod=%llu (%llu fps)\n", level, top, jitter, framePeriod, 1000000L / framePeriod);
        state = JBV_STATE_LEAK;
    }
    /*
     * Go to full state when level reaches above top level.
     */
    else {
        JBV_wrnLog("JBV_STATE_FULL level=%llu, top=%llu, jitter=%llu, framePeriod=%llu (%llu fps)\n", level, top, jitter, framePeriod, 1000000L / framePeriod);
        state = JBV_STATE_FULL;
    }

    return (state);
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
    vint      nextSeqn;
    JBV_Unit *unit_ptr;


    /*
     * Find the latest packet.
     */
    tsMax = 0;
    foundTs = tsMax;
    m1Seqn = -1;
    for (seqn = 0; seqn < _JBV_SEQN_MAXDIFF; seqn++) {
        unit_ptr = &obj_ptr->unit[seqn];
        /* Check if the JBV unit is valid. */
        if (!unit_ptr->valid) {
            tsMax = foundTs;
            continue;
        }

        /* Valid JBV unit. */
        if (tsMax < unit_ptr->ts) {
            /*JBV unit time stamp is greater (newer packet). Update tsMax. */
            tsMax = unit_ptr->ts;
        }
        else if (tsMax > unit_ptr->ts){
            if(foundTs > unit_ptr->ts){
                tsMax = foundTs;
                continue;
            }
            tsMax = unit_ptr->ts;
        }
        /* Try to find last packet (mark bit set) in sequence. */
        if (!unit_ptr->mark) {
            continue;
        }
        /* Maximum seqn     */
        xseqn = seqn;

        /*
         * Handle frames with the same timestamp.
         * Set firstInSeq for next packet has the same ts after
         * mark pkt.
         */
        nextSeqn = JBV_NEXT_SEQN(seqn);
        if ((obj_ptr->unit[nextSeqn].valid) &&
                (unit_ptr->ts == obj_ptr->unit[nextSeqn].ts)) {
            /*
             * Next packet has the same ts, it's different frame,
             * set firstInSeq */
            obj_ptr->unit[nextSeqn].firstInSeq = 1;
        }

        /* Now find min seqn. */
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
                break;
            }
            xseqn = JBV_PREVIOUS_SEQN(xseqn);
            if (xseqn == seqn) {
                tsMax = foundTs;
                break;
            }
        }
    }

    if (m1Seqn < 0) {
        /* No sequence complete in JB. */
        JBV_dbgLog("No sequence complete in JB\n");
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
    uint64    tsMax;

    if (_JBV_findTsMax(obj_ptr, &tsMax) < 0) {
        JBV_dbgLog("No Packets Ready\n");
        return;
    }

    /*
     * Drop packets that are too old. (Older than the last drawn)
     */
    _JBV_dropOlderPackets(obj_ptr, obj_ptr->lastDrawnTs);
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

    *level_ptr = 0;

    maxSeqn = _JBV_findTsMax(obj_ptr, &tsMax);
    if (maxSeqn < 0) {
        JBV_dbgLog("No Packets Ready\n");
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
            seqn = JBV_PREVIOUS_SEQN(seqn);
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
                    xseqn = JBV_PREVIOUS_SEQN(xseqn);
                    if (xseqn == seqn) {
                        tsMin = foundTs;
                        break;
                    }
                }
            }
        }
        seqn = JBV_PREVIOUS_SEQN(seqn);
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
        *level_ptr = (tsMax - tsMin) + obj_ptr->framePeriod;
        if (tsMax == tsMin) {
            *level_ptr = obj_ptr->framePeriod; /* We have one frame in */
        }
        //JBV_dbgLog("Found oldest frame %d~%d level:%llu\n", m1Seqn, mSeqn,
        //        *level_ptr);
        return (0);
    }
    JBV_errLog("Sequence NOT FOUND maxSeqn=%d, m1Seqn=%d mSeqn=%d, tsMax=%llu, "
            "tsMin=%llu\n", maxSeqn, m1Seqn, mSeqn, tsMax, tsMin);
    return (JB_ERROR);
}
