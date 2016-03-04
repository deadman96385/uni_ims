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
 * JBV mutex is used to prevent get and put at the same time.
 */
static OSAL_SemId mJBVMutex = NULL;

/*
 * ======== JBV_statePrint() ========
 *
 * Print the current state of JBV.
 * Returns: void
 */
static void JBV_statePrint(
    JBV_Obj     *obj_ptr,
    JBV_Pkt     *pkt_ptr)
{
    vint i;
    uvint frameCount = 0;
    uvint packetCount = 0;
    vint seqn = obj_ptr->lastSeqn;

    JBV_InfoLog("+-------------------------------------------------------+");
    if (NULL != pkt_ptr) {
        JBV_InfoLog("RTP seqn %u", pkt_ptr->seqn);
    }
    if (!obj_ptr->unit[JBV_PREVIOUS_SEQN(seqn)].valid) {
        JBV_InfoLog("RTP packst lost or disorder");
    }

    JBV_InfoLog("JBV Jitter %llu", obj_ptr->jitter);
    JBV_InfoLog("Oldest frame duration %llu", obj_ptr->level);

    for (i = 0; i < _JBV_SEQN_MAXDIFF; i++) {
        if (obj_ptr->unit[i].valid) {
            packetCount++;
            if (obj_ptr->unit[i].mark) {
                frameCount++;
            }
        }
    }
    JBV_InfoLog("Current JBV Frame Count %u", frameCount);
    JBV_InfoLog("Current JBV packet Count %u", packetCount);
    JBV_InfoLog("JBV received framePeriod %llu FPS(%llu)",
            obj_ptr->framePeriod, 1000000 / obj_ptr->framePeriod);
    JBV_InfoLog("JBV PacketLoss - LostCount %u",
            obj_ptr->rtcpInfo.packetLoss.lostPacketCount);
    JBV_InfoLog("JBV PacketDrop - Invalid %u overflow %u tooOld %u total %u",
            obj_ptr->rtcpInfo.packetLoss.invalidPacketDrop,
            obj_ptr->rtcpInfo.packetLoss.jbvOverflowDrop,
            obj_ptr->rtcpInfo.packetLoss.tooOldPacketDrop,
            obj_ptr->rtcpInfo.packetLoss.totalPacketDrop);
    JBV_InfoLog("+-------------------------------------------------------+");
}

/*
 * ======== JBV_putPkt() ========
 *
 * Puts a packet in the buffer.
 * NOTE: getPkt and putPkt should be synchronized. If not it could cause crashes.
 */
static void _JBV_putPkt(
    JBV_Obj     *obj_ptr,
    JBV_Pkt     *pkt_ptr,
    JBV_Timeval *tv_ptr)
{
    uint16    lastSeqnNextFrame;
    uint16    firstSeqnNextFrame;
    uint16    seqn;
    JBV_Unit *unit_ptr;
    uint64    level;
    vint      state;
    vint      updateFirstTs = 0;

    /* Return if jbv or pkt ptr is null. */
    if ((NULL == obj_ptr) || (NULL == pkt_ptr)) {
        return;
    }

    /* Return if packet is not valid. */
    if (!pkt_ptr->valid) {
        return;
    }

    /* Return if type not H264 or H263. */
    if (pkt_ptr->type != VTSP_CODER_VIDEO_H263 &&
            pkt_ptr->type != VTSP_CODER_VIDEO_H264) {
        return;
    }

    state = obj_ptr->state;

    /*
     * It is highly unlikely that for real time comm., seqn
     * will be out of order by 256 places.
     * Even if it is, the packet is too out of place and
     * has no hope of recovery.
     */
    seqn = JBV_SEQN_FROM_RTP_SEQN(pkt_ptr->seqn);
    unit_ptr = &obj_ptr->unit[seqn];

    /*
     * Time stamp, arrival time adjustment.
     */
    if (!obj_ptr->ready) {
        obj_ptr->firstAtime    = pkt_ptr->atime;
        obj_ptr->firstUnNormTs = pkt_ptr->tsOrig;
        updateFirstTs = 1;
        obj_ptr->initTime  = ((uint64)tv_ptr->sec) * _JBV_SEC_TO_USEC + (uint64)tv_ptr->usec;
        obj_ptr->tsLast90K = pkt_ptr->tsOrig;
        obj_ptr->ready     = 1;

        OSAL_logMsg("not ready -> ready seqn:%u Ts:%llu",
                pkt_ptr->seqn, obj_ptr->firstUnNormTs);
    }

    if (obj_ptr->firstAtime > pkt_ptr->atime) {
        /*
         * This warp around wont "ever" happen. OK maybe 600000 years later.
         */
        obj_ptr->firstAtime = pkt_ptr->atime;
    }

    /*
     * Copy the data.
     */
    unit_ptr->key = 0;
    if (pkt_ptr->pSize <= sizeof(unit_ptr->data_ptr)) {
        switch(pkt_ptr->type) {
            case VTSP_CODER_VIDEO_H263:
                if (0 != _JBV_processH263(obj_ptr, unit_ptr, pkt_ptr)) {
                    JBV_errLog("invalid H263 packet\n");
                    return;
                }
                break;
            case VTSP_CODER_VIDEO_H264:
                if (0 != _JBV_processH264(obj_ptr, unit_ptr, pkt_ptr, &updateFirstTs)) {
                    /* Packet got dropped. Return. */
                    return;
                }
                break;
            default:
                /* Invalid packet. Drop it. */
                _JBV_dropPacket(obj_ptr, NULL, JBV_DROP_INVALID);
                JBV_errLog("This should never happen");
                return;
        }

        /* Cache the first packet time stamp for Frame period calculation. */
        if (updateFirstTs) {
            obj_ptr->firstTs  = unit_ptr->ts;
            OSAL_logMsg("cache first seqn:%u first Ts:%llu",
                    pkt_ptr->seqn, obj_ptr->firstTs);
        }

        JBV_dbgLog("Put packet seqn:%u at location %u of size %u ts %llu "
                "atime %llu key=%02x, mark=%02x firstinSeq=%02x\n",
                pkt_ptr->seqn, seqn, unit_ptr->offset, unit_ptr->ts,
                unit_ptr->atime, unit_ptr->key, unit_ptr->mark,
                unit_ptr->firstInSeq);

        /*
         * Update jitter based on last packet.
         */
        _JBV_updateJitter(obj_ptr, unit_ptr);

        /*
         * Update frame period.
         */
        _JBV_updateFramePeriod(obj_ptr, seqn);

        /*
         * Check for packet loss.
         */
        _JBV_checkForPacketLoss(obj_ptr, unit_ptr, tv_ptr);

        /*
         * Update last instances.
         */
        obj_ptr->lastTs = unit_ptr->ts;
        obj_ptr->lastAtime = unit_ptr->atime;
        obj_ptr->lastSeqn = unit_ptr->seqn;
    }
    else {
        /* Incoming packet size is larger than what we can fit.
         * Invalid packet. Drop it.
         */
        _JBV_dropPacket(obj_ptr, unit_ptr, JBV_DROP_INVALID);
        JBV_errLog("Packet is larger than what can fit pSize=%d, fit=%d\n",
                pkt_ptr->pSize, sizeof(unit_ptr->data_ptr));
        return;
    }

    /* Update level and also find oldest frame */
    _JBV_findOldestSequence(obj_ptr, &firstSeqnNextFrame, &lastSeqnNextFrame,
            &level);
    /* Run state machine */
    state = _JBV_stateMachine(state, obj_ptr->jitter, level,
            obj_ptr->framePeriod + obj_ptr->framePeriodOffset);

    /* Store */
    obj_ptr->firstSeqnNextFrame = firstSeqnNextFrame;
    obj_ptr->lastSeqnNextFrame  = lastSeqnNextFrame;
    obj_ptr->level              = level;
    obj_ptr->state              = state;

    //JBV_statePrint(obj_ptr, pkt_ptr);
}

/*
 * ======== _JBV_getPkt() ========
 *
 * Gets a packet to decode from the buffer.
 * NOTE: getPkt and putPkt should be synchronized. If not it could cause crashes.
 */
static void _JBV_getPkt(
    JBV_Obj     *obj_ptr,
    JBV_Pkt     *pkt_ptr,
    JBV_Timeval *tv_ptr)
{
    JBV_Unit *unit_ptr;
    uint16    lastSeqnNextFrame;
    uint16    firstSeqnNextFrame;
    uint16    eSeqn;
    uint16    lastDrawnSeqn;
    uint64    level;
    uint64    curTime;
    uint64    curTimeDiff;
    uint64    lastCtime;
    uint64    nextDrawTime;
    uint64    lastDrawnTs;
    uint64    tsDiff;
    vint      state;
    vint      leak;
    uint32    naluBitMask;

    if ((NULL == obj_ptr) || (NULL == pkt_ptr)) {
        return;
    }

    /*
     * Cached references to object.
     */
    level              = obj_ptr->level;
    state              = obj_ptr->state;
    eSeqn              = obj_ptr->eSeqn;
    lastCtime          = obj_ptr->lastCtime;
    lastDrawnTs        = obj_ptr->lastDrawnTs;
    lastDrawnSeqn      = obj_ptr->lastDrawnSeqn;
    nextDrawTime       = obj_ptr->nextDrawTime;
    firstSeqnNextFrame = obj_ptr->firstSeqnNextFrame;
    lastSeqnNextFrame  = obj_ptr->lastSeqnNextFrame;

    leak  = 0;

    pkt_ptr->pSize = 0;
    pkt_ptr->valid = 0;
    pkt_ptr->data_ptr = NULL;

    if (!obj_ptr->ready) {
        return;
    }

    if (0 == level) {
        JBV_dbgLog("No Packet Available framePeriod=%llu\n",
                obj_ptr->framePeriod);
        return;
    }

    /*
     * Check if the buffer has recently been initialized with initLevel > 0.
     * This allows the buffer to initially fill up to some non-zero delay.
     */
    if (obj_ptr->initLevel > 0) {
        if (level < obj_ptr->initLevel) {
            JBV_dbgLog("Init level hasn't reached. lev:%llu,initLev:%llu\n",
                    level, obj_ptr->initLevel);
            return;
        }
        else {
            obj_ptr->initLevel = 0;  /* Proceed normally. Don't test again. */
            JBV_dbgLog("Init level rached. level:%llu, initLevel:%llu\n",
                    level, obj_ptr->initLevel);
        }
    }
    unit_ptr =  &obj_ptr->unit[lastSeqnNextFrame];
    /*
     * Adjust draw rate based on calculated frame rate.
     * But give packets of same time stamp.
     */
    if ((lastDrawnTs != unit_ptr->ts)) {
        /*
         * Find time stamp of current time.
         */
        curTime = ((uint64)tv_ptr->sec) * _JBV_SEC_TO_USEC + (uint64)tv_ptr->usec;
        curTime -= obj_ptr->initTime;

        /* Draw rule bases on JBV state */
        switch (state) {
            case JBV_STATE_NORM:
                obj_ptr->accm = 0;
                if (0 == nextDrawTime) {
                    /* First draw, set next draw time */
                    nextDrawTime = curTime + obj_ptr->framePeriod +
                            obj_ptr->framePeriodOffset;
                }
                else {
                    /* Normal draw, check if it's time to draw */
                    if (curTime < nextDrawTime) {
                        JBV_dbgLog("JBV_STATE_NORM not time to draw. "
                                "curTime:%llu, lastDrawTime:%llu\n",
                                curTime, lastCtime);
                        return;
                    }
                    /* Increase next draw time by one frame period */
                    nextDrawTime += (obj_ptr->framePeriod +
                            obj_ptr->framePeriodOffset);
                }
                //JBV_dbgLog("JBV_STATE_NORM curTime:%llu lastDrawTime:%llu\n",
                //        curTime, lastCtime);
                break;
            case JBV_STATE_ACCM:
                if (0 == nextDrawTime) {
                    /* First draw, set next draw time */
                    nextDrawTime = curTime + obj_ptr->framePeriod +
                            obj_ptr->framePeriodOffset;
                    JBV_dbgLog("JBV_STATE_ACCM First draw in ACCM state, "
                            "not good\n");
                }
                else {
                    if (curTime < nextDrawTime) {
                        JBV_dbgLog("JBV_STATE_ACCM not time to draw. "
                                "curTime:%llu, lastDrawTime:%llu\n",
                                curTime, lastCtime);
                        return;
                    }
                    nextDrawTime = curTime + obj_ptr->framePeriod +
                            obj_ptr->framePeriodOffset;
                }
                obj_ptr->accm++;
                //JBV_dbgLog("JBV_STATE_ACCM "
                //        "curTime:%llu lastDrawTime:%llu accm:%d\n",
                //        curTime, lastCtime, obj_ptr->accm);
                break;
            case JBV_STATE_LEAK:
                obj_ptr->accm = 0;
                nextDrawTime = curTime + obj_ptr->framePeriod +
                        obj_ptr->framePeriodOffset;
                //JBV_dbgLog("JBV_STATE_LEAK curTime:%llu lastDrawTime:%llu\n",
                //        curTime, lastCtime);
                break;
            case JBV_STATE_EMPTY:
                /* EMPTY */
                obj_ptr->accm = 0;
                JBV_dbgLog("JBV_STATE_EMPTY "
                        "curTime:%llu lastDrawTime:%llu accm:%d\n",
                        curTime, lastCtime, obj_ptr->accm);
                return;
                break;
            case JBV_STATE_FULL:
                /* FULL state, draw a frame */
                obj_ptr->accm = 0;
                leak = 1;
                nextDrawTime = curTime + obj_ptr->framePeriod +
                        obj_ptr->framePeriodOffset;
                //JBV_dbgLog("JBV_STATE_FULL curTime:%llu lastDrawTime:%llu\n",
                //        curTime, lastCtime);
                break;
            default:
                JBV_dbgLog("Invalid JBV state: %d", state);
                break;
        }

        /*
         * Check for if accm count exceeds accumulate rate
         */
        if (obj_ptr->accm >= obj_ptr->accmRate) {
            /*
             * Accm this packet.
             */
            obj_ptr->accm = 0;
            JBV_dbgLog("Accm a frame.  curTime:%llu lastDrawTime:%llu\n",
                    curTime, lastCtime);
            return;
        }

        /*
         * Check if it's early for unexpected frame on:
         * 1. All non-first-draw frame when mosaic prevention is on.
         * 2. Non-first-draw and non-leak frame when mosaic prevention is off
         */
        if ((_JBV_TS_INIT != lastDrawnTs) &&
                (!leak || obj_ptr->eMscPrvt)) {
            /*
             * Normal draw, check if it's too early for unexpected frame.
             * This mechanisim reduces possibility of missing frame
             *
             * *Note: SPS/PPS/IDR doesn't not considered as unexpected frame.
             */
            tsDiff = unit_ptr->ts - lastDrawnTs;
            curTimeDiff = curTime - lastCtime;
            /*
             * (1 != unit_ptr->key) is for H263.
             */
            if ((firstSeqnNextFrame != eSeqn) &&
                    (NALU_IDR != unit_ptr->nalu) &&
                    (NALU_PPS != unit_ptr->nalu) &&
                    (NALU_SPS != unit_ptr->nalu) &&
                    (1 != unit_ptr->key)) {

                /*
                 * Next expected frame is not ready, check if it's time
                 * to draw the oldest frame we have
                 * *Note: a
                 */
                if (tsDiff > curTimeDiff) {
                    JBV_dbgLog("Unexpected frame. Too early for seqn:%u "
                            "eSeqn:%d level:%llu tsDiff:%llu curTmDiff:%llu\n",
                            firstSeqnNextFrame, eSeqn, level, tsDiff,
                            curTimeDiff);
                    return;
                } else {
                    /*
                     * Going to draw frame which is not expected.
                     * Means there is frame missing.
                     */
                    JBV_dbgLog("Drawing unexpected frame. seqn:%u eSeqn:%d "
                            "level:%llu tsDiff:%llu curTimeDiff:%llu",
                            firstSeqnNextFrame, eSeqn, level, tsDiff,
                            curTimeDiff);

                    /* Store the statistic that a missing frame was skipped */
                    obj_ptr->rtcpInfo.packetLoss.lostSinceIdr++;

                    if (1 == obj_ptr->eMscPrvt) {
                        /*
                         * We are going to draw an unexpected frame, means
                         * there is some frame missing.
                         * Don't draw if for mosaic prevention.
                         */

                        /* TODO:
                         * Consider sending some notification to the APP
                         * so that it may choose to send an FIR
                         */
                        JBV_dbgLog("Mosaic prevention enabled. There is "
                                "Missing frame, don't draw non-IDR frame "
                                "nalu:%d\n", unit_ptr->nalu);
                        return ;
                    }
                }
            }
        }
        lastCtime = curTime;
     }

    switch(unit_ptr->ptype) {
        case VTSP_CODER_VIDEO_H263:
            /*
             * Series ready ? yes : give. no : return
             */
            if (0 == _JBV_reassembleH263(obj_ptr, firstSeqnNextFrame,
                    lastSeqnNextFrame, pkt_ptr)) {
                JBV_dbgLog("Got packet seqn=%u at location %u, size %u, "
                        "new level %llu, state:0x%x, curTime:%llu, nextDrawTime:%llu\n",
			unit_ptr->seqn, lastSeqnNextFrame, pkt_ptr->pSize, level, state, curTime, nextDrawTime);
                lastDrawnTs = obj_ptr->unit[lastSeqnNextFrame].ts;
                lastDrawnSeqn = lastSeqnNextFrame;
            }
            break;

        case VTSP_CODER_VIDEO_H264:
            /*
             * Series ready ? yes : give. no : return
             */
            if (0 == _JBV_reassembleH264(obj_ptr, firstSeqnNextFrame,
                        lastSeqnNextFrame, &naluBitMask, pkt_ptr)) {
		if (state & (JBV_STATE_LEAK | JBV_STATE_FULL)) {
                        JBV_wrnLog("Got packet seqn=%u at location %u, size %u, "
                                "new level %llu, state:0x%x, curTime:%llu, nextDrawTime:%llu\n",
			        unit_ptr->seqn, lastSeqnNextFrame, pkt_ptr->pSize, level, state, curTime, nextDrawTime);
		}
                lastDrawnTs = obj_ptr->unit[lastSeqnNextFrame].ts;
                lastDrawnSeqn = lastSeqnNextFrame;
                    }
            break;
        default:
            /* Invalid packet. Drop it. */
            _JBV_dropPacket(obj_ptr, NULL, JBV_DROP_INVALID);
            JBV_errLog("This should never happen");
            return;
    }

    /* Now drop hopeless packets */
    _JBV_cleanup(obj_ptr);
    /* Update expect seqn for next frame */
    eSeqn = JBV_NEXT_SEQN(lastDrawnSeqn);
    /* Update level and also find oldest frame */
    _JBV_findOldestSequence(obj_ptr, &firstSeqnNextFrame, &lastSeqnNextFrame,
            &level);
    /* Run state machine */
    state = _JBV_stateMachine(state, obj_ptr->jitter, level,
            obj_ptr->framePeriod + obj_ptr->framePeriodOffset);

    /* Store */
    obj_ptr->firstSeqnNextFrame = firstSeqnNextFrame;
    obj_ptr->lastSeqnNextFrame  = lastSeqnNextFrame;
    obj_ptr->lastCtime          = lastCtime;
    obj_ptr->level              = level;
    obj_ptr->nextDrawTime       = nextDrawTime;
    obj_ptr->state              = state;
    obj_ptr->eSeqn              = eSeqn;
    obj_ptr->lastDrawnTs        = lastDrawnTs;
    obj_ptr->lastDrawnSeqn      = lastDrawnSeqn;
}

/*
 * ======== JBV_init() ========
 *
 * Init the video jitter buffer.
 * Returns:
 * 0 : Success
 * -1 : Failed.
 */
int JBV_init(
    JBV_Obj    *obj_ptr)
{
    if (NULL == obj_ptr) {
        return (-1);
    }

    /* Initialize the JBV Mutex. Available to use. */
    if (NULL == mJBVMutex) {
        mJBVMutex = OSAL_semCountCreate(1);
    }

    /* Wait till mutex is ready. */
    OSAL_semAcquire(mJBVMutex, OSAL_WAIT_FOREVER);

    /* Set all JBV_Obj memory to zero. */
    OSAL_memSet(obj_ptr, 0, sizeof(JBV_Obj));

    obj_ptr->accmRate  = JBV_ACCM_RATE;
    obj_ptr->initLevel = JBV_INIT_LEVEL;
    obj_ptr->eMscPrvt  = JBV_MOSAIC_PREVENTION;

    /*
     * Start with init jitter and then adapt.
     * This will work till 10 fps, but not under.
     */
    obj_ptr->jitter = JBV_INIT_JITTER_USEC;

    /*
     * Start with init frame period.
     * This will also be adapted.
     */
    obj_ptr->framePeriod = JBV_INIT_FRAME_PERIOD_USEC;
    obj_ptr->framePeriodOffset = 0;

    /* Set lastDrawnTs to init value to indicate nothing is draw yet */
    obj_ptr->lastDrawnTs = _JBV_TS_INIT;

    /* Init state */
    obj_ptr->state = JBV_STATE_EMPTY;

    OSAL_logMsg("accmRate:%d, initLevel:%llu, eMscPrvt:%d\n",
            obj_ptr->accmRate, obj_ptr->initLevel, obj_ptr->eMscPrvt);

    /* Give the mutex. */
    OSAL_semGive(mJBVMutex);

    return (0);
}

/*
 * ======== JBV_putPkt() ========
 *
 * Puts a packet in the buffer.
 * NOTE: getPkt and putPkt should be synchronized. If not it could cause crashes.
 */
void JBV_putPkt(
    JBV_Obj     *obj_ptr,
    JBV_Pkt     *pkt_ptr,
    JBV_Timeval *tv_ptr)
{
    /* Wait till mutex is ready. */
    OSAL_semAcquire(mJBVMutex, OSAL_WAIT_FOREVER);

    _JBV_putPkt(obj_ptr, pkt_ptr, tv_ptr);

    /* Give the mutex. */
    OSAL_semGive(mJBVMutex);
}

/*
 * ======== JBV_getPkt() ========
 *
 * Gets a packet to decode from the buffer.
 * NOTE: getPkt and putPkt should be synchronized. If not it could cause crashes.
 */
void JBV_getPkt(
    JBV_Obj     *obj_ptr,
    JBV_Pkt     *pkt_ptr,
    JBV_Timeval *tv_ptr)
{
    /* Wait till mutex is ready. */
    OSAL_semAcquire(mJBVMutex, OSAL_WAIT_FOREVER);

    _JBV_getPkt(obj_ptr, pkt_ptr, tv_ptr);

    /* Give the mutex. */
    OSAL_semGive(mJBVMutex);
}

/*
 * ======== JBV_getRtcpInfo() ========
 *
 * - Destructive Read -
 * Uses the mutex lock to safely remove the necessary information from
 * the jitter buffer and place it in the JBV RTCP Info.
 *
 * Parameters:
 *      obj_ptr     - A pointer to the relevant jitter buffer object
 *      dest_ptr    - Memory allocated to write the RTCP info to
 */
void JBV_getRtcpInfo(
    JBV_Obj         *obj_ptr,
    JBV_RtcpInfo    *dest_ptr)
{
    JBV_RtcpInfo  *jbvInfo_ptr;

    /* Wait till mutex is ready. */
    OSAL_semAcquire(mJBVMutex, OSAL_WAIT_FOREVER);


    jbvInfo_ptr = &obj_ptr->rtcpInfo;
    OSAL_memCpy(dest_ptr, jbvInfo_ptr, sizeof(JBV_RtcpInfo));

    /* Clear the RTCP info from the jitter buffer */
    jbvInfo_ptr->packetLoss.lostSeqnLength = 0;
    jbvInfo_ptr->keyFrameDropped    = OSAL_FALSE;
    jbvInfo_ptr->keyFrameRead       = OSAL_FALSE;

    /* Give the mutex. */
    OSAL_semGive(mJBVMutex);
}

/*
 * ======== JBV_fpsAdjust() ========
 *
 *  Adjust JBV frame period offset.
 *
 *     audioVideoSkew = audioTime - videoTime (in milliseconds)
 * the value is +ve if audio is lagging,
 *         this means we have to slow down video play back (increase frame period).
 * the value is -ve if audio is leading,
 *         this means we have to increase video play back (decrease frame period).
 */
void JBV_fpsAdjust(
    JBV_Obj   *obj_ptr,
    vint       audioVideoSkew)
{
    vint maxFramePeriodOffset;

    if (NULL == obj_ptr) {
        return;
    }

    JBV_dbgLog("audioVideoSkew:%d rtp-framePeriod:%llu framePeriodOffset:%d",
            audioVideoSkew, obj_ptr->framePeriod, obj_ptr->framePeriodOffset);

    if (audioVideoSkew > JBV_LIP_SYNC_LAGGING_AUDIO_SKEW_MS) {
        /*
         * Increase frame period offset (proportional to the skew)
         * to adjust video playback. Bound the frame period offset (+/-)
         * to not more than 10% the RTP frame period.
         */
        obj_ptr->framePeriodOffset +=
                JBV_LIP_SYNC_FRAME_PERIOD_OFFSET(audioVideoSkew);
        maxFramePeriodOffset = (obj_ptr->framePeriod / 10);
        if (maxFramePeriodOffset < obj_ptr->framePeriodOffset) {
            obj_ptr->framePeriodOffset = maxFramePeriodOffset;
        }
        JBV_dbgLog("new framePeriodOffset:%d", obj_ptr->framePeriodOffset);
    }
    else if (audioVideoSkew < -JBV_LIP_SYNC_LEADING_AUDIO_SKEW_MS) {
        /*
         * Decrease frame period offset (proportional to the skew)
         * to adjust video playback. Bound the frame period offset (+/-)
         * to not more than 10% the RTP frame period.
         */
        obj_ptr->framePeriodOffset +=
                JBV_LIP_SYNC_FRAME_PERIOD_OFFSET(audioVideoSkew);
        maxFramePeriodOffset = -(obj_ptr->framePeriod / 10);
        if (obj_ptr->framePeriodOffset < maxFramePeriodOffset) {
            obj_ptr->framePeriodOffset = maxFramePeriodOffset;
        }
        JBV_dbgLog("new framePeriodOffset:%d", obj_ptr->framePeriodOffset);
    }
    else {
        JBV_dbgLog("audioVideoSkew is within the acceptable rage [-45, 75]");
    }
}
