/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL AND
 * PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2005-2010 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 12523 $ $Date: 2010-07-13 18:57:44 -0400 (Tue, 13 Jul 2010) $
 *
 */

#include "_vc_private.h"
#include <jbv.h>
#include <vtsp_constant.h>
#include "_vc_rtcp.h"

#define CLEAR_FLAG(rtcp_ptr, flag) rtcp_ptr->configure.rtcpFeedbackSendMask &= ~flag;

static inline vint _VC_getBitrateLevel(
    uint64 sample_time)
{
    uint64 temp = (BITRATE_STAT_PERIOD * 205) >> 8; /* get 0.8 x BITRATE_STAT_PERIOD */
    vint level = -1;

    /*
     * BITRATE_FACTOR_LEVEL3: sample_time >= 0.8 x BITRATE_STAT_PERIOD
     * BITRATE_FACTOR_LEVEL2: 0.8 x BITRATE_STAT_PERIOD > sample_time >= 0.4 x BITRATE_STAT_PERIOD
     * BITRATE_FACTOR_LEVEL1: 0.4 x BITRATE_STAT_PERIOD > sample_time >= 0.2 x BITRATE_STAT_PERIOD
     * BITRATE_FACTOR_LEVEL0: 0.2 x BITRATE_STAT_PERIOD > sample_time
     */
    if (sample_time >= temp) {
        level = BITRATE_FACTOR_LEVEL3;
    } else if((sample_time >= (temp >> 1)) && (sample_time < temp)) {
        level = BITRATE_FACTOR_LEVEL2;
    } else if((sample_time >= (temp >> 2)) && (sample_time < (temp >> 1))) {
        level = BITRATE_FACTOR_LEVEL1;
    } else {
        level = BITRATE_FACTOR_LEVEL0;
    }

    return level;
}

static uint32 _VC_estimateBitrate(
     _VC_RtpBitrateStat *br_stat)
{
    uint32 prev_br;
    uint32 cur_br = 0;
    uint32 est_br = 0;
    uint64 sample_time;
    vint   level;

    prev_br = br_stat->bitrate;
    sample_time = br_stat->lastTime - br_stat->startTime;
    level = _VC_getBitrateLevel(sample_time);

    /*
     * [FORMULA]:
     * alpha = level/base;
     * e_br = p_br*(1-alpha) + alpha*c_br;
     * */
    if (level > 0 && prev_br > 0) {
        cur_br = (uint32)((((br_stat->totalSize - br_stat->lastSize) << 3) * 1000000 / sample_time) >> 10);
        est_br = (prev_br - ((level * prev_br) >> BITRATE_FACTOR_BASE_BIT) +
            ((level * cur_br) >> BITRATE_FACTOR_BASE_BIT));
    } else if(level == 0 && prev_br > 0) {
        est_br = prev_br;
    } else if(level >= BITRATE_FACTOR_LEVEL1 && prev_br == 0){
        cur_br = (uint32)((((br_stat->totalSize - br_stat->lastSize) << 3) * 1000000 / sample_time) >> 10);
        est_br = cur_br;
    } else {
        est_br = 0;
    }
    OSAL_logMsg("%s: est_br %u, prev_br %u, cur_br %u, sample_time %llu, level %d, size %llu\n",
            __FUNCTION__, est_br, prev_br, cur_br, sample_time, level,
            br_stat->totalSize - br_stat->lastSize);
    return est_br;
}

/*
 * ======== _VC_rtcpUtilUpdateRtcp() ========
 *
 * Update the RTCP data structure with the latest from
 *      the jitter buffer and from the outgoing RTP packets.
 */
static void _VC_rtcpUtilProcessRtp(
    _VC_NetObj      *net_ptr,
    _VC_RtcpObject  *rtcp_ptr,
    _VC_RtcpNtpTime *time_ptr)
{
    uvint extendedMax;
    uvint expected;
    uvint expectedInterval;
    uvint receivedInterval;
    uvint lostInterval;
    OSAL_SelectTimeval  currentTime;    /* Current time as OSAL Timeval */
    uint32 currentTimeMs;               /* Current time as milliseconds */
    uint32 timeDiffMs;

    _VC_RtpObject   *rtp_tmp_ptr;
    _VC_RtpRtcpInfoObject   rtpInfo;
    _VC_RtpRtcpInfoObject   *rtpInfo_ptr;
    RTP_Pkt         *packet_ptr;

    /* For cross thread safety, it is important that the following occurs in this order:
     *  1) Lock outgoing Video RTP
     *  2) Get the system time
     *  3) Copy data from the outgoing Video RTP
     *  4) Unlock the outgoing Video RTP
     *  5) Copy incoming Video RTP data by using the jitter buffer. */

    rtp_tmp_ptr = _VC_streamIdToRtpPtr(net_ptr, rtcp_ptr->streamId);

    /* Lock the RTP Object */
    OSAL_semAcquire(rtp_tmp_ptr->info.mutexLock, OSAL_WAIT_FOREVER);

    OSAL_memCpy(&rtpInfo, &rtp_tmp_ptr->info, sizeof(rtpInfo));
    OSAL_selectGetTime(&currentTime);

    /* Done with the RTP Object */
    OSAL_semGive(rtp_tmp_ptr->info.mutexLock);
    rtpInfo_ptr = &rtpInfo;

    time_ptr->msw = _VC_SEC_TO_NTP_MSW(currentTime.sec);
    time_ptr->lsw = _VC_USEC_TO_NTP_LSW(currentTime.usec);

    /*
     * Sender SSRC comes from the latest RTP packet sent
     * Media SSRC comes from the latest RTP packet received.
     */
        packet_ptr = &rtp_tmp_ptr->sendRtpObj.pkt;
        rtcp_ptr->ssrc = packet_ptr->rtpMinHdr.ssrc;
        packet_ptr = &rtp_tmp_ptr->recvRtpObj.pkt;
        rtcp_ptr->mediaSsrc = packet_ptr->rtpMinHdr.ssrc;

    /* === Calculate information about incoming RTP === */

    /*
     * Calculated the number of lost packets using the algorithm in A.3 of
     * RFC 3550.
     */
    extendedMax = rtpInfo_ptr->cycles + rtpInfo_ptr->maxSequence;
    /* Expected number of packets since the beginning of the call */
    expected    = extendedMax - rtpInfo_ptr->baseSequence + 1;
    /* The total number of packets lost since the beginning of the call */
    rtcp_ptr->lost        = expected - rtpInfo_ptr->received;
    if (rtcp_ptr->lost > 0x7fffff) {
        rtcp_ptr->lost = 0x7fffff;
    }
    else if (rtcp_ptr->lost < (-0x7fffff)) {
        rtcp_ptr->lost = 0x800000;
    }
    rtcp_ptr->extendedSeqn = extendedMax;

    /*
     * Calculate the fraction.
     */
    if (expected - rtcp_ptr->expectedPrior > (1 << 15) && expected < rtcp_ptr->expectedPrior) {
        expectedInterval = expected;
        rtcp_ptr->expectedPrior = expected;
        receivedInterval = expectedInterval;
    } else {
        /* Find the number of packets expected between now and the previously sent RTCP report */
        expectedInterval       = expected - rtcp_ptr->expectedPrior;
        /* Find the number of packets expected between now and the previously sent RTCP report */
        rtcp_ptr->expectedPrior = expected;
        /* Find the actual number of packets received between now and the previously sent RTCP report */
        receivedInterval       = rtpInfo_ptr->received - rtcp_ptr->receivedPrior;
    }

    /* Accumulate expected packet total for TMMBR. */
    rtcp_ptr->feedback.expectedPacketTotal += expectedInterval;

    /* Store the count of total received packets for the next time this method is called */
    rtcp_ptr->receivedPrior = rtpInfo_ptr->received;
    /* Find the number of lost packets between now and the previously sent RTCP report */
    lostInterval = expectedInterval - receivedInterval;

    /* Accumulate lost packet total for TMMBR. */
    rtcp_ptr->feedback.lostPacketTotal += lostInterval;

    if (rtcp_ptr->feedback.lostPacketTotal > rtcp_ptr->feedback.expectedPacketTotal) {
        OSAL_logMsg("%s: this should not happen, lostPacketTotal:%d, expectedPacketTotal:%d, expected:%d, received:%d, cycles:%u, maxSeq:%u, baseSeq:%u\n",
                __FUNCTION__, rtcp_ptr->feedback.lostPacketTotal, rtcp_ptr->feedback.expectedPacketTotal,
                expected, rtpInfo_ptr->received, rtpInfo_ptr->cycles, rtpInfo_ptr->maxSequence, rtpInfo_ptr->baseSequence);
    }

    if (( 0 == expectedInterval) || (lostInterval <= 0)) {
        rtcp_ptr->fracLost = 0;
    }
    else {
        rtcp_ptr->fracLost = (lostInterval << 8) / expectedInterval;
    }

    /* === Calculate information about outgoing RTP === */

    /*
     * Calculate the RTP time stamp for RTCP SR for Video.
     * By calculating the elapsed time since the first frame and current time,
     * we can calculate the RTP sample increments as
     * Video RTP Sample increments = timeElapsed * Video clock rate.
     */
    currentTimeMs = (currentTime.sec * 1000) + (currentTime.usec / 1000);
    timeDiffMs = currentTimeMs - rtpInfo_ptr->firstRtpTime;
    rtcp_ptr->rtpTime = rtpInfo_ptr->firstRtpTime + (timeDiffMs * _VC_VIDEO_CLOCK_RATE_IN_KHZ);

    rtcp_ptr->rtpSendPacketCount = rtpInfo_ptr->sendPacketCount;
    rtcp_ptr->rtpSendOctetCount = rtpInfo_ptr->sendOctetCount;
    rtcp_ptr->curRxBitrate = _VC_estimateBitrate(&rtpInfo_ptr->rxBitrateStat);
}
/*
 * ======== _VC_rtcpUtilBuildNackRows() ========
 *
 * Populate an array of FCI rows for a generic NACK
 *    jbv_ptr - a pointer to the jitter buffer
 *    fciRows_ptr - a pointer to an array of 32 bit FCI rows
 *    bufferSize - the size of the array pointed to by fciRows_ptr
 */
static vint _VC_rtcpUtilBuildNackRows(
        JBV_PacketLoss  *jbvPacketloss_ptr,
        uint32          *fciRows_ptr,
        vint            bufferSize)
{
    int             lostItr;
    int             previousPID;    /* The PID of the current FCI row. */
    int             numLostPackets;
    int             rowsUsed;
    uint16*         seqn_ptr;

    seqn_ptr        = jbvPacketloss_ptr->lostSeqn;
    numLostPackets  = jbvPacketloss_ptr->lostSeqnLength;

    rowsUsed    = 0;
    previousPID = 0;
    for (lostItr = 0; lostItr < numLostPackets; lostItr++) {
        if ((seqn_ptr[lostItr] > previousPID + 16) || (rowsUsed == 0)) {
            if (rowsUsed < (_VTSP_RTCP_MSG_FB_NACK_SZ - 2)) {
                rowsUsed++;
                previousPID  = seqn_ptr[lostItr];
                fciRows_ptr[rowsUsed - 1] = seqn_ptr[lostItr] << 16;
            }
            else {
                continue;
            }
        }
        else {
            /* The JBV_PacketLoss guarantees that the packet sequence number in its array are
             * in order. This means that the shift amount will never be negative. */
            fciRows_ptr[rowsUsed - 1] |= 1 << (seqn_ptr[lostItr] - previousPID - 1);
        }
    }

    return rowsUsed;
}

static uint32 _VC_rtcpUtilRunTmmbrFsm2(
    _VC_RtcpObject      *rtcp_ptr)
{
    _VC_RtcpFeedback *feedback_ptr;
    uint32      lost_permillage = 0;
    vint        state;
    vint        dir;
    uint32      bitrate_kbps;
    uint32      mask = 0;

    feedback_ptr = &rtcp_ptr->feedback;
    state = feedback_ptr->state;
    dir = feedback_ptr->direction;
    bitrate_kbps = rtcp_ptr->curRxBitrate;

    OSAL_logMsg("%s: TMMBR - state:%u dir:%d bitrate_kbps: %u expected:%u lost:%u\n",
            __FUNCTION__, state, dir, bitrate_kbps, feedback_ptr->expectedPacketTotal,
            feedback_ptr->lostPacketTotal);

    /*
     * if the bitrate is zero, it means we should accmulate more pkts in the first time.
     * if the expected pkts is less than 500, should accmulate more pkts to reduce the tmmbr frequency.
     *
     * */
    if (bitrate_kbps == 0 ||
            feedback_ptr->expectedPacketTotal < 500) {
        return mask;
    }

    /* if lostPacketTotal is greater than expectedPacketTotal, workround */
    if (feedback_ptr->expectedPacketTotal < feedback_ptr->lostPacketTotal) {
        OSAL_logMsg("%s: expectedPacketTotal is less than lostPacketTotal, workround here\n",
                __FUNCTION__);
        feedback_ptr->expectedPacketTotal = 0;
        feedback_ptr->lostPacketTotal = 0;
        return mask;
    }

    /*
     * if the state is pending, resned TMMBR feedback with the previous config.
     * when the resend count is greater than 2, clean the sendFailCount.
     * */
    if (feedback_ptr->state == _VC_TMMBR_STATE_PENDING && feedback_ptr->tmmbrSendFailCount <= 2) {
        OSAL_logMsg("%s: not received TMMBN, then send the previous bitrate once more, count=%d\n",
                __FUNCTION__, feedback_ptr->tmmbrSendFailCount);
        feedback_ptr->tmmbrSendFailCount ++;
        mask = VTSP_MASK_RTCP_FB_TMMBR;
        return mask;
    } else if(feedback_ptr->state == _VC_TMMBR_STATE_PENDING && feedback_ptr->tmmbrSendFailCount > 2){
        OSAL_logMsg("%s, not received TMMBN, tmmbrSendFailCount > 2", __FUNCTION__);
        feedback_ptr->tmmbrSendFailCount = 0;
    }

    /* calculate the lost permillage if we have observed sufficient packets. */
    lost_permillage = (feedback_ptr->lostPacketTotal * 1000) /
            feedback_ptr->expectedPacketTotal;

    switch (dir) {
        case _VC_TMMBR_DIR_LEVEL:
            if (lost_permillage >= 10) {
                feedback_ptr->step = lost_permillage >= 30 ?
                        (bitrate_kbps * 205) >> 10 : /* if lost_permillage > 3%, step = 0.2 * bitrate */
                        (bitrate_kbps * 102) >> 10;  /* if 3% > lost_permillage > 1%, step = 0.1 * bitrate*/
                feedback_ptr->direction = _VC_TMMBR_DIR_DOWN;
                feedback_ptr->state = _VC_TMMBR_STATE_PENDING;
                feedback_ptr->sendTmmbrInKbps = bitrate_kbps - feedback_ptr->step;
                mask = VTSP_MASK_RTCP_FB_TMMBR;
            }
            break;
        case _VC_TMMBR_DIR_DOWN:
            if (lost_permillage >= 10) {
                feedback_ptr->step = lost_permillage >= 30 ?
                        (bitrate_kbps * 205) >> 10 : /* if lost_permillage > 3%, step = 0.2 * bitrate */
                        (bitrate_kbps * 102) >> 10;  /* if 3% > lost_permillage > 1%, step = 0.1 * bitrate*/
                feedback_ptr->direction = _VC_TMMBR_DIR_DOWN;
                feedback_ptr->state = _VC_TMMBR_STATE_PENDING;
                feedback_ptr->sendTmmbrInKbps = bitrate_kbps - feedback_ptr->step;
                mask = VTSP_MASK_RTCP_FB_TMMBR;
            } else {
                if (feedback_ptr->step < _VC_TMMBR_STEP_MIN) {
                    feedback_ptr->step = 0;
                    feedback_ptr->direction = _VC_TMMBR_DIR_LEVEL;
                } else if (lost_permillage < 5){
                    feedback_ptr->step = feedback_ptr->step >> 1;
                    feedback_ptr->direction = _VC_TMMBR_DIR_UP;
                    feedback_ptr->state = _VC_TMMBR_STATE_PENDING;
                    feedback_ptr->sendTmmbrInKbps = bitrate_kbps + feedback_ptr->step;
                    mask = VTSP_MASK_RTCP_FB_TMMBR;
                } else if (lost_permillage >= 5) {
                    feedback_ptr->step = feedback_ptr->step >> 1;
                    feedback_ptr->state = _VC_TMMBR_STATE_PENDING;
                    feedback_ptr->sendTmmbrInKbps = bitrate_kbps - feedback_ptr->step;
                    mask = VTSP_MASK_RTCP_FB_TMMBR;
                }
            }
            break;
        case _VC_TMMBR_DIR_UP:
            if (lost_permillage >= 10) {
                feedback_ptr->step = lost_permillage >= 30 ?
                        (bitrate_kbps * 205) >> 10 : /* if lost_permillage > 3%, step = 0.2 * bitrate */
                        (bitrate_kbps * 102) >> 10;  /* if 3% > lost_permillage > 1%, step = 0.1 * bitrate*/
                feedback_ptr->direction = _VC_TMMBR_DIR_DOWN;
                feedback_ptr->state = _VC_TMMBR_STATE_PENDING;
                feedback_ptr->sendTmmbrInKbps = bitrate_kbps - feedback_ptr->step;
                mask = VTSP_MASK_RTCP_FB_TMMBR;
            } else {
                if (feedback_ptr->step < _VC_TMMBR_STEP_MIN) {
                    feedback_ptr->step = 0;
                    feedback_ptr->direction = _VC_TMMBR_DIR_LEVEL;
                } else if(lost_permillage <= 5){
                    feedback_ptr->state = _VC_TMMBR_STATE_PENDING;
                    feedback_ptr->sendTmmbrInKbps = bitrate_kbps + feedback_ptr->step;
                    mask = VTSP_MASK_RTCP_FB_TMMBR;
                } else if(lost_permillage > 5) {
                    feedback_ptr->direction = _VC_TMMBR_DIR_DOWN;
                    feedback_ptr->step = feedback_ptr->step >> 1;
                    feedback_ptr->state = _VC_TMMBR_STATE_PENDING;
                    feedback_ptr->sendTmmbrInKbps = bitrate_kbps - feedback_ptr->step;
                    mask = VTSP_MASK_RTCP_FB_TMMBR;
                }
            }
            break;
        default:
            OSAL_logMsg("invalid tmmbr state, %d\n", feedback_ptr->state);
            break;
    }

    OSAL_logMsg("%s: TMMBR - state:%d->%d, dir:%d->%d, sendTmmbrInKbps:%u, step:%u, lost_permillage:%u, mask:0x%x\n",
            __FUNCTION__, state, feedback_ptr->state, dir, feedback_ptr->direction,
            feedback_ptr->sendTmmbrInKbps, feedback_ptr->step, lost_permillage, mask);

    /* Reset the variables that keeps track of incoming video payload statistics. */
    feedback_ptr->expectedPacketTotal = 0;
    feedback_ptr->lostPacketTotal = 0;

    return mask;
}

/*
 * ======== _VC_rtcpUtilRunTmmbrFsm() ========
 *
 * Run the TMMBR state machine and Update the RTCP Feedback structure
 */
static uint32 _VC_rtcpUtilRunTmmbrFsm(
    _VC_RtcpObject      *rtcp_ptr)
{
    _VC_RtcpFeedback *feedback_ptr;
    uint32            mask = 0;
    uint32            bitrateKbps;
    uint16            lostPercentage = 0;
    feedback_ptr = &rtcp_ptr->feedback;

    _VC_RTCP_LOG("TMMBR - state:%u expected:%u lost:%u\n", feedback_ptr->tmmbrState,
            feedback_ptr->expectedPacketTotal, feedback_ptr->lostPacketTotal);

    /* Every time before we send RTCP Run the TMMBR state machine. */
    if (feedback_ptr->tmmbrState == _VC_TMMBR_STATE_INHIBIT) {
        if (feedback_ptr->expectedPacketTotal >= _VC_TMMBR_N_INHIBT) {
            /* Check to see if we have local AS BW parameter set. */
            if (feedback_ptr->sendTmmbrInKbps != 0 &&  rtcp_ptr->localVideoAsBwKbps != 0) {
                /* Transition to appropriate State as we have ignored sufficient packets. */
                if (feedback_ptr->sendTmmbrInKbps == rtcp_ptr->localVideoAsBwKbps) {
                    /* Transition to Normal state if TMMBR same as AS parameter. */
                    feedback_ptr->tmmbrState = _VC_TMMBR_STATE_NORMAL;
                    _VC_RTCP_LOG("TMMBR - Transition Inhibit to Normal");
                }
                else {
                    /* Transition to Back off state. */
                    feedback_ptr->tmmbrState = _VC_TMMBR_STATE_BACK_OFF;
                    _VC_RTCP_LOG("TMMBR - Transition Inhibit to Backoff");
                }
            }

            /* Reset the variables that keeps track of incoming video payload statistics. */
            feedback_ptr->expectedPacketTotal = 0;
            feedback_ptr->lostPacketTotal = 0;
        }
    }
    else if (feedback_ptr->tmmbrState == _VC_TMMBR_STATE_NORMAL) {
        if (feedback_ptr->expectedPacketTotal >= _VC_TMMBR_N_NORMAL) {
            /* calculate the lost percentage if we have observed sufficient packets. */
            lostPercentage = (feedback_ptr->lostPacketTotal * 100) /
                    feedback_ptr->expectedPacketTotal;

            _VC_RTCP_LOG("TMMBR - Normal lostPercentage:%u\n", lostPercentage);

            if (lostPercentage >= _VC_TMMBR_PLR_1) {
                /* TMMBR should be sent. */
                mask = VTSP_MASK_RTCP_FB_TMMBR;
                /* Request MBR to be half AS. */
                feedback_ptr->sendTmmbrInKbps = (rtcp_ptr->localVideoAsBwKbps >> 1);
                /* Transition to Waiting state. */
                feedback_ptr->recvTmmbnInKbps = 0;
                feedback_ptr->tmmbrSendFailCount = 0;
                feedback_ptr->tmmbrState = _VC_TMMBR_STATE_WAITING;
                _VC_RTCP_LOG("TMMBR - Transition Normal to Waiting");
            }

            /* Reset the variables for the next observation window. */
            feedback_ptr->expectedPacketTotal = 0;
            feedback_ptr->lostPacketTotal = 0;
        }
    }
    else if (feedback_ptr->tmmbrState == _VC_TMMBR_STATE_WAITING) {
        if (feedback_ptr->expectedPacketTotal >= _VC_TMMBR_N_WAITING) {
            /* Check if we have received TMMBN. */
            if (feedback_ptr->recvTmmbnInKbps > 0) {
                if (feedback_ptr->recvTmmbnInKbps >= rtcp_ptr->localVideoAsBwKbps) {
                    /* Transition to Normal state. TMMBN >= Local AS Bandwidth. */
                    feedback_ptr->tmmbrState = _VC_TMMBR_STATE_NORMAL;
                    _VC_RTCP_LOG("TMMBR - Transition Waiting to Normal");
                }
                else {
                    /* Transition to Back off state as we got TMMBN < Local AS Bandwidth. */
                    feedback_ptr->tmmbrState = _VC_TMMBR_STATE_BACK_OFF;
                    _VC_RTCP_LOG("TMMBR - Transition Waiting to Backoff");
                }
            }
            else {
                if (feedback_ptr->tmmbrSendFailCount <= 2) {
                    /* Failed to receive TMMBN. Re-transmit the TMMBR. */
                    feedback_ptr->tmmbrSendFailCount++;
                    mask = VTSP_MASK_RTCP_FB_TMMBR;
                    feedback_ptr->recvTmmbnInKbps = 0;
                }
                else {
                    /*
                     * Failed to receive TMMBN even after re-transmitting twice.
                     * Transition to Inhibit state. Assume last sent TMMBR is good.
                     */
                    feedback_ptr->tmmbrState = _VC_TMMBR_STATE_INHIBIT;
                    _VC_RTCP_LOG("TMMBR - Transition Waiting to Inhibit");
                }
            }

            /* Reset the variables for the next observation window. */
            feedback_ptr->expectedPacketTotal = 0;
            feedback_ptr->lostPacketTotal = 0;
        }
    }
    else if (feedback_ptr->tmmbrState == _VC_TMMBR_STATE_BACK_OFF) {
        if (feedback_ptr->expectedPacketTotal >= _VC_TMMBR_N_BACK_OFF) {
            /* calculate the lost percentage if we have observed sufficient packets. */
            lostPercentage = (feedback_ptr->lostPacketTotal * 100) /
                    feedback_ptr->expectedPacketTotal;

            _VC_RTCP_LOG("TMMBR - Backoff lostPercentage:%u\n", lostPercentage);

            if (lostPercentage < _VC_TMMBR_PLR_2) {
                /* TMMBR should be sent. Increase MBR 25%. */
                mask = VTSP_MASK_RTCP_FB_TMMBR;
                bitrateKbps = feedback_ptr->sendTmmbrInKbps + (feedback_ptr->sendTmmbrInKbps >> 2);
                /* When increasing DO NOT increase more than the local AS BW parameter. */
                if (bitrateKbps >= rtcp_ptr->localVideoAsBwKbps) {
                    feedback_ptr->sendTmmbrInKbps = rtcp_ptr->localVideoAsBwKbps;
                }
                else {
                    feedback_ptr->sendTmmbrInKbps = bitrateKbps;
                }

                /* Transition to Waiting state. */
                feedback_ptr->recvTmmbnInKbps = 0;
                feedback_ptr->tmmbrSendFailCount = 0;
                feedback_ptr->tmmbrState = _VC_TMMBR_STATE_WAITING;
                _VC_RTCP_LOG("TMMBR - Transition Back off to Waiting. Increase TMMBR");
            }
            else if (lostPercentage >= _VC_TMMBR_PLR_1) {
                /* TMMBR should be sent. Decrease TMMBR 50% */
                mask = VTSP_MASK_RTCP_FB_TMMBR;
                bitrateKbps = (feedback_ptr->sendTmmbrInKbps >> 1);
                /* When decreaseing DO NOT decrease less than _VC_TMMBR_LOW_KBPS. */
                if (bitrateKbps < _VC_TMMBR_LOW_KBPS) {
                    feedback_ptr->sendTmmbrInKbps = _VC_TMMBR_LOW_KBPS;
                }
                else {
                    feedback_ptr->sendTmmbrInKbps = bitrateKbps;
                }

                /* Transition to Waiting state. */
                feedback_ptr->recvTmmbnInKbps = 0;
                feedback_ptr->tmmbrSendFailCount = 0;
                feedback_ptr->tmmbrState = _VC_TMMBR_STATE_WAITING;
                _VC_RTCP_LOG("TMMBR - Transition Back off to Waiting. Decrease TMMBR");
            }

            /* Reset the variables for the next observation window. */
            feedback_ptr->expectedPacketTotal = 0;
            feedback_ptr->lostPacketTotal = 0;
        }
    }

    return mask;
}

/*
 * ======== _VC_rtcpUtilitProcessJbvRtcpInfo() ========
 * Update the RTCP Object with any new information from the jitter buffer.
 *
 * Parameters:
 *   - obj_ptr:        A pointer to the jitter buffer object that should be read
 *   - rtcp_ptr:       A pointer to the RTCP object that should be updated
 *   - jbvRtcpInfo_ptr A pointer to a JBV_RtcpInfo structure that will be populated with information from JBV
 * Returns:
 *   - feedbackMask:   A mask of the types of RTCP feedback that should be sent in this time slot
 */
static uint32 _VC_rtcpUtilitProcessJbvRtcpInfo(
    JBV_Obj             *obj_ptr,
    _VC_RtcpObject      *rtcp_ptr,
    JBV_RtcpInfo        *jbvRtcpInfo_ptr)
{
    uint32             feedbackMask;
    _VC_RtcpFeedback  *rtcpPrivate_ptr;
    vint               pliLoss;

    //get the current time
    OSAL_SelectTimeval  currentTime;    /* Current time as OSAL Timeval */
    OSAL_selectGetTime(&currentTime);
    uint32 currentTimeMs = (currentTime.sec * 1000) + (currentTime.usec / 1000);

    /* The feedback mask may get changed in other threads, so make a copy of the current value */
    feedbackMask = rtcp_ptr->configure.rtcpFeedbackSendMask;
    rtcpPrivate_ptr = &rtcp_ptr->feedback;

    /* Get updates from the jitter buffer regarding RTCP */
    JBV_getRtcpInfo(obj_ptr, jbvRtcpInfo_ptr);

    if (feedbackMask & VTSP_MASK_RTCP_FB_FIR) {
        rtcpPrivate_ptr->firWaiting = OSAL_TRUE;
    }

    if (rtcpPrivate_ptr->firWaiting) {
        /* Each time a new FIR request is sent, the sequence number must be increased by
         * one. However, if this is a retransmission, then the sequence number
         * must NOT be incremented. */
    //check the time has elapsed since the last fir to avoid frequently sending FIR
        _VC_RTCP_LOG("keyFrameRead=%d, currentTimeMs=%u, lastFirSend=%u\n"
            , jbvRtcpInfo_ptr->keyFrameRead, currentTimeMs, rtcp_ptr->feedback.lastFirSend);
        if (jbvRtcpInfo_ptr->keyFrameRead &&
            ((currentTimeMs - rtcp_ptr->feedback.lastFirSend) < 2000)) {
            /* A completed sync frame was read by the application. This means the application's
             * request to send an FIR should be cancelled. The sequence number should be updated
             * for the next FIR. */
            //rtcp_ptr->feedback.firSeqNumber++;
            rtcp_ptr->feedback.firWaiting = OSAL_FALSE;
            feedbackMask &= ~VTSP_MASK_RTCP_FB_FIR;
            CLEAR_FLAG(rtcp_ptr, VTSP_MASK_RTCP_FB_FIR);
        }
        else if (jbvRtcpInfo_ptr->keyFrameDropped) {
            /* A sync frame was dropped to to an error in it's transmission. This
             * means the FIR is no longer a 'retransmission.' Instead, this is
             * a request for a new sync frame. */
            _VC_RTCP_LOG("key Frame droped, and after that no more key frame has been recv");
            rtcp_ptr->feedback.firSeqNumber++;
        } else {
            rtcp_ptr->feedback.firSeqNumber++;
        }
    }

    /* If a new I-Frame has been received, reset the PLI frame-loss counter */
    if (rtcp_ptr->keyFramesRecv != jbvRtcpInfo_ptr->keyFramesRecv) {
        rtcp_ptr->lostAtPriorPli = 0;
        rtcp_ptr->keyFramesRecv = jbvRtcpInfo_ptr->keyFramesRecv;
    }

    /* Find the number of frames that have been lost since the last I-Frame and
     * that have not been factored into a previously sent PLI. */
    pliLoss = jbvRtcpInfo_ptr->packetLoss.lostSinceIdr - rtcp_ptr->lostAtPriorPli;
    _VC_RTCP_LOG("Frame loss: %d frames (max allowed is %d)", pliLoss, _VC_RTCP_PLI_N);
    /* If too many frames were lost, send a PLI */
    if ( pliLoss >= _VC_RTCP_PLI_N) {
        rtcp_ptr->configure.rtcpFeedbackSendMask |= VTSP_MASK_RTCP_FB_PLI;
        feedbackMask |= VTSP_MASK_RTCP_FB_PLI;
        /* Update the PLI loss counter so that another PLI can be sent if
         * another VTSP_MASK_RTCP_FB_PLI frames are lost. */
        rtcp_ptr->lostAtPriorPli = jbvRtcpInfo_ptr->packetLoss.lostSinceIdr;
    }

    /* Run the TMMBR state machine which returns bitmask indicating we should send TMMBR or not. */
    feedbackMask |= _VC_rtcpUtilRunTmmbrFsm2(rtcp_ptr);

    return feedbackMask;
}

/*
 * ======== _VC_rtcpReceptionReportBlock() ========
 *
 * Generate a Reception Report Block packet that is placed at the current offset in the
 * message payload buffer. This block is common to both Sender and Receiver Report.
 */
static vint _VC_rtcpReceptionReportBlock(
    _VC_RtcpObject   *rtcp_ptr,
    _VTSP_RtcpCmdMsg *msg_ptr,
    vint              offset,
    uint32            currentNtpTime)
{
    uint32 temp32;
    vint  newoffset = offset;

    msg_ptr->msg.payload[newoffset++] = OSAL_netHtonl(rtcp_ptr->mediaSsrc);         /* SSRC of the Media Source */
    /* Fraction Lost + Cumulative # of packets lost. */
    temp32 = ((rtcp_ptr->fracLost & 0xff) << 24) | rtcp_ptr->lost;
    msg_ptr->msg.payload[newoffset++] = OSAL_netHtonl(temp32);
    msg_ptr->msg.payload[newoffset++] = OSAL_netHtonl(rtcp_ptr->extendedSeqn); /* Extended highest sequence number received. */
    msg_ptr->msg.payload[newoffset++] = OSAL_netHtonl(0); /* Interarrival jitter. */
    msg_ptr->msg.payload[newoffset++] = OSAL_netHtonl(rtcp_ptr->lastSR); /* last SR timestamp. */

    temp32 = currentNtpTime - rtcp_ptr->recvLastSR;
    msg_ptr->msg.payload[newoffset++] = OSAL_netHtonl(temp32); /* Delay since last SR. */

    msg_ptr->payloadSize += (newoffset - offset);

    return (newoffset);
}

/*
 * ======== _VC_rtcpSenderReport() ========
 *
 * Generate a Sender Report RTCP packet that is placed at the current offset in the
 * message payload buffer.
 */
static vint _VC_rtcpSenderReport(
    _VC_RtcpObject      *rtcp_ptr,
    VTSP_StreamDir       streamDir,
    _VC_RtcpNtpTime     *ntpTime_ptr,
    _VTSP_RtcpCmdMsg    *msg_ptr,
    vint                 offset)
{
    uint32            temp32;
    vint              newoffset = offset;
    uint32            ntpTime;
    vint              includeRR;

    /*
     * If RECV is active, create a receive report (RR).
     */
    includeRR = (VTSP_STREAM_DIR_RECVONLY == streamDir) | (VTSP_STREAM_DIR_SENDRECV == streamDir);

    temp32  = 0x2 << 30; /* version number */
    temp32 |= 0 << 29;   /* padding */
    if (includeRR) {
        temp32 |= 1 << 24;   /* Reception Report Block Count (RC) */
        temp32 |= _VTSP_RTCP_PTYPE_SR << 16; /* Sender Report Packet Type */
        temp32 |= 12; /* RFC 3550 Section 6.4.1 - Length = 7 + 6 - 1  */
    }
    else {
        temp32 |= 0 << 24;   /* Reception Report Block Count (RC) */
        temp32 |= _VTSP_RTCP_PTYPE_SR << 16; /* Sender Report Packet Type */
        temp32 |= 6;  /* RFC 3550 Section 6.4.1 - Length = 7 + 0 - 1  */
    }

    /* Construct the Header. length = 2. */
    msg_ptr->msg.payload[newoffset++] = OSAL_netHtonl(temp32);
    msg_ptr->msg.payload[newoffset++] = OSAL_netHtonl(rtcp_ptr->ssrc);  /* SSRC of this RTCP packet sender */
    msg_ptr->msg.payload[newoffset++] = OSAL_netHtonl(ntpTime_ptr->msw);   /* Current NTP Timestamp MSW */
    msg_ptr->msg.payload[newoffset++] = OSAL_netHtonl(ntpTime_ptr->lsw);   /* Current NTP Timestamp LSW */
    msg_ptr->msg.payload[newoffset++] = OSAL_netHtonl(rtcp_ptr->rtpTime);       /* RTP Timestamp */
    msg_ptr->msg.payload[newoffset++] = OSAL_netHtonl(rtcp_ptr->rtpSendPacketCount); /* Sender's packet count */
    msg_ptr->msg.payload[newoffset++] = OSAL_netHtonl(rtcp_ptr->rtpSendOctetCount); /* Sender's octet count */
    msg_ptr->payloadSize += (newoffset - offset);

    if (includeRR) {
        ntpTime = _VC_NTP_64_TO_32(ntpTime_ptr->msw, ntpTime_ptr->lsw);
        newoffset = _VC_rtcpReceptionReportBlock(rtcp_ptr, msg_ptr, newoffset, ntpTime);
    }

    return (newoffset);
}

/*
 * ======== _VC_rtcpReceiverReport() ========
 *
 * Generate a Receiver Report RTCP packet that is placed at the current offset in the
 * message payload buffer.
 */
static vint _VC_rtcpReceiverReport(
    _VC_RtcpObject   *rtcp_ptr,
    _VC_RtcpNtpTime  *currentTime,
    _VTSP_RtcpCmdMsg *msg_ptr,
    vint              offset)

{
    uint32            temp32;
    vint               newoffset = offset;
    uint32  ntpTime;

    temp32  = 0x2 << 30; /* version number */
    temp32 |= 0 << 29;   /* padding */
    temp32 |= 1 << 24;   /* Reception Report Block Count (RC) usually 1 */
    temp32 |= _VTSP_RTCP_PTYPE_RR << 16; /* Receiver Report Packet Type */
    temp32 |= 7; /* RFC 3550 Section 6.4.2 - Length = 2 + 6 - 1  */

    /* Construct the Header. length = 2. */
    msg_ptr->msg.payload[newoffset++] = OSAL_netHtonl(temp32);
    msg_ptr->msg.payload[newoffset++] = OSAL_netHtonl(rtcp_ptr->ssrc);  /* SSRC of this RTCP packet sender */
    msg_ptr->payloadSize += (newoffset - offset);

    /* Construct the Reception Report Block. length = 6. */
    ntpTime = _VC_NTP_64_TO_32(currentTime->msw, currentTime->lsw);
    newoffset = _VC_rtcpReceptionReportBlock(rtcp_ptr, msg_ptr, newoffset, ntpTime);

    return (newoffset);
}

/*
 * ======== _VC_rtcpEmptyReceiverReport() ========
 *
 * Generate a Empty Receiver Report RTCP packet that is placed at the current offset in the
 * message payload buffer.
 */
static vint _VC_rtcpEmptyReceiverReport(
    _VC_RtcpObject   *rtcp_ptr,
    _VTSP_RtcpCmdMsg *msg_ptr,
    vint              offset)
{
    uint32            temp32;
    vint               newoffset = offset;

    temp32  = 0x2 << 30; /* version number */
    temp32 |= 0 << 29;   /* padding */
    temp32 |= 0 << 24;   /* Reception Report Block Count (RC) = 0  */
    temp32 |= _VTSP_RTCP_PTYPE_RR << 16; /* Receiver Report Packet Type */
    temp32 |= 1; /* RFC 3550 Section 6.4.2 - Length = 1 */

    /* Construct the Header. length = 2. */
    msg_ptr->msg.payload[newoffset++] = OSAL_netHtonl(temp32);
    msg_ptr->msg.payload[newoffset++] = OSAL_netHtonl(rtcp_ptr->ssrc); /* SSRC of this RTCP packet sender */
    msg_ptr->payloadSize += (newoffset - offset);

    return (newoffset);
}

/*
 * ======== _VC_rtcpSourceDescription() ========
 *
 * Generate a Source Description RTCP packet that is placed at the current offset in the
 * message payload buffer.
 */
static vint _VC_rtcpSourceDescription(
    _VC_NetObj       *net_ptr,
    _VTSP_RtcpCmdMsg *msg_ptr,
    vint              offset,
    uint32            ssrc)
{
    uint32           temp32;
    uint32           *dst_ptr;
    uint32           *data_ptr;
    vint             loop;
    vint             newoffset = offset;
    vint             cnameSizeAs32bitWord;

    cnameSizeAs32bitWord = net_ptr->rtcpCname.length;

    temp32 = 0x2 << 30; /* version number */
    temp32 |= 0 << 29; /* padding */
    temp32 |= 1 << 24; /* Source Count (SC) usually 1 */
    temp32 |= _VTSP_RTCP_PTYPE_SDES << 16; /* Receiver Report Packet Type */
    temp32 |= cnameSizeAs32bitWord + 1; /* RFC 3550 Section 6.5 - Length = SSRC + CNAME item Size. */

    /* Source Descrption Header. */
    msg_ptr->msg.payload[newoffset++] = OSAL_netHtonl(temp32);
    /* Chunk starts with SSRC. */
    msg_ptr->msg.payload[newoffset++] = ssrc; /* SSRC of this RTCP packet sender */
    msg_ptr->payloadSize += (newoffset - offset);

    /* copy the CNAME SDES item. */
    dst_ptr = &(msg_ptr->msg.payload[newoffset]);
    data_ptr = net_ptr->rtcpCname.cname;
    for (loop = 0; loop < cnameSizeAs32bitWord; loop++) {
        *dst_ptr++ = *data_ptr++;
    }

    /* Calculate the newoffset and update payloadSize. */
    newoffset += cnameSizeAs32bitWord;
    msg_ptr->payloadSize += cnameSizeAs32bitWord;

    return (newoffset);
}

/*
 * ======== _VC_rtcpExtendedStatSummBlock() ========
 * NOT YET IMPLEMENTED
 * Section of Extended Report (XR) packet (RFC 3611).
 * Create Statistics Sumary Report Block (RFC 3611 Section 4.6).
 */
static vint _VC_rtcpExtendedStatSummBlock(
    _VC_RtcpObject   *rtcp_ptr,
    _VTSP_RtcpCmdMsg *msg_ptr,
    vint              offset)
{
    uint32          temp32;
    vint            newoffset = offset;

    temp32  = _VTSP_RTCP_XR_BTYPE_SS << 24; /* BT=6 */
    temp32 |= (1 << 23); /* lost OK */
    temp32 |= (1 << 21); /* jit OK */
    temp32 |= (9);              /* block length = 9 */
    msg_ptr->msg.payload[newoffset++] = OSAL_netHtonl(temp32);
    msg_ptr->msg.payload[newoffset++] = OSAL_netHtonl(rtcp_ptr->ssrc);

    /* NOT YET IMPLEMENTED. */
    temp32  = ((0) & 0xffff) << 16;      /* begin */
    temp32 |= ((0) & 0xffff0000) >> 16;  /* end */
    msg_ptr->msg.payload[newoffset++] = OSAL_netHtonl(temp32);         /* begin|end_seq */
    msg_ptr->msg.payload[newoffset++] = OSAL_netHtonl((0)); /* lost_packets */
    msg_ptr->msg.payload[newoffset++] = OSAL_netHtonl((0)); /* dup_packets */
    msg_ptr->msg.payload[newoffset++] = OSAL_netHtonl((0)); /* min_jitter */
    msg_ptr->msg.payload[newoffset++] = OSAL_netHtonl((0)); /* max_jitter */
    msg_ptr->msg.payload[newoffset++] = OSAL_netHtonl((0)); /* mean_jitter */
    msg_ptr->msg.payload[newoffset++] = OSAL_netHtonl((0)); /* dev_jitter */
    msg_ptr->msg.payload[newoffset++] = OSAL_netHtonl((0)); /* min_ttl_... ... (unsupported) */
    msg_ptr->payloadSize += (newoffset - offset);

    return (newoffset);
}

/*
 * ======== _VC_rtcpExtendedMetricsBlock() ========
 * NOT YET IMPLEMENTED
 * Section of Extended Report (XR) packet (RFC 3611).
 * Create MR block; VoIP Metrics Report Block (RFC 3611 Section 4.7)
 */
static vint _VC_rtcpExtendedMetricsBlock(
    _VC_RtcpObject   *rtcp_ptr,
    _VTSP_RtcpCmdMsg *msg_ptr,
    vint              offset)
{
    uint32          temp32;
    uvint           delay;
    vint            newoffset = offset;

    temp32  = _VTSP_RTCP_XR_BTYPE_VMR << 24;   /* BT=7 */
    temp32 |= (8);                                  /* block length */
    msg_ptr->msg.payload[newoffset++] = OSAL_netHtonl(temp32);
    msg_ptr->msg.payload[newoffset++] = OSAL_netHtonl(rtcp_ptr->ssrc);

    /* NOT YET IMPLEMENTED. */
    temp32  = ((0) & 0xff) << 24;           /* loss rate */
    temp32 |= ((0) & 0xff) << 16;           /* discard rate */
    msg_ptr->msg.payload[newoffset++] = OSAL_netHtonl(temp32);

    msg_ptr->msg.payload[newoffset++] = 0; /* burst, gap dur: unsupported */

    /* end sys delay = jit + decode + encode + playout delays(VHW) */
    delay   = (0);
    temp32  = (delay & 0xff);                       /* round trip delay */
    temp32 |= ((0)) << 16;        /* end sys delay */
    msg_ptr->msg.payload[newoffset++] = OSAL_netHtonl(temp32);

    /* signal level, noise level, Gmin unsupported */
    temp32 = 127;
    msg_ptr->msg.payload[newoffset++] = OSAL_netHtonl(temp32);

    /* R factor, ... MOS-CQ: unsupported */
    msg_ptr->msg.payload[newoffset++] = 0;

    /* RX config: unsupported */
    temp32  = ((0) & 0xffff);              /* JB nominal */
    msg_ptr->msg.payload[newoffset++] = OSAL_netHtonl(temp32);

    temp32  = ((0) & 0xffff) << 16;        /* JB maximum */
    temp32 |= ((0) & 0xffff);              /* JB abs max */
    msg_ptr->msg.payload[newoffset++] = OSAL_netHtonl(temp32);
    msg_ptr->payloadSize += (newoffset - offset);

    return (newoffset);
}

/*
 * ======== _VC_rtcpExtendedReport() ========
 * NOT YET IMPLEMENTED
 * This function creates a complete Extended report (XR) (RFC 3611 Section 2).
 */
static vint _VC_rtcpExtendedReport(
    _VC_RtcpObject   *rtcp_ptr,
    _VTSP_RtcpCmdMsg *msg_ptr,
    vint              offset)
{
    uint32            temp32;
    vint              newoffset = offset;

    temp32  = 0x2 << 30; /* version number */
    temp32 |= 0 << 29;   /* padding and reserved */
    temp32 |= _VTSP_RTCP_PTYPE_XR << 16; /* packet type */
    temp32 |= (20);      /* total length = 1+9+10 */

    msg_ptr->msg.payload[newoffset++] = OSAL_netHtonl(temp32);
    msg_ptr->msg.payload[newoffset++] = OSAL_netHtonl(rtcp_ptr->ssrc); /* SSRC of sender */
    msg_ptr->payloadSize += (newoffset - offset);

    /* Construct the VoIP Metrics Block Block. length = 9. */
    newoffset = _VC_rtcpExtendedMetricsBlock(rtcp_ptr, msg_ptr, newoffset);

    /* Construct the Statistics Summary Block. length = 10. */
    newoffset = _VC_rtcpExtendedStatSummBlock(rtcp_ptr, msg_ptr, newoffset);

    return (newoffset);
}

/*
 * ======== _VC_rtcpFeedbackGenericNack() ========
 *
 * Generate a Generic NACK RTCP Feedback packet that is placed at the current offset in the
 * message payload buffer.
 *
 * See RFC 4585 Section 6.2.1
 */
static vint _VC_rtcpFeedbackGenericNack(
    _VC_RtcpObject   *rtcp_ptr,
    JBV_PacketLoss   *jbvPacketloss_ptr,
    _VTSP_RtcpCmdMsg *msg_ptr,
    vint              offset)
{
    uint32      temp32;
    vint        newoffset = offset;
    vint        rowItr;
    vint        rowsUsed;
    uint32      nack[_VTSP_RTCP_MSG_FB_NACK_SZ - 2];

    rowsUsed = _VC_rtcpUtilBuildNackRows(jbvPacketloss_ptr, nack, _VTSP_RTCP_MSG_FB_NACK_SZ -2);
    if (0 == rowsUsed) {
        return offset;
    }

    temp32  = 0x2 << 30; /* version number */
    temp32 |= 0 << 29;   /* padding */
    temp32 |= _VTSP_RTCP_FMT_NACK << 24;  /* Feedback Message Type for Generic NACK */
    temp32 |= _VTSP_RTCP_PTYPE_RTPFB << 16; /* Generic NACK is a Transport Layer FB Message - RTPFB */
    /* The 'length' field is 2 + the number of rows of NACK entries. Because JBV_PACKET_LOSS_NACK_ROW_MAX
     * is substantially less than 2^16, there is no need for: & 0xFFFF */
    temp32 |= rowsUsed + 2;

    msg_ptr->msg.payload[newoffset++] = OSAL_netHtonl(temp32);
    msg_ptr->msg.payload[newoffset++] = OSAL_netHtonl(rtcp_ptr->ssrc); /* SSRC of this RTCP packet sender */
    msg_ptr->msg.payload[newoffset++] = OSAL_netHtonl(rtcp_ptr->mediaSsrc); /* SSRC of the Media Source */

    /* FCI Entry for Generic NACK. */
    for (rowItr = 0; rowItr < rowsUsed; rowItr++) {
        msg_ptr->msg.payload[newoffset++] = OSAL_netHtonl(nack[rowItr]);
    }

    msg_ptr->payloadSize += (newoffset - offset);

    return (newoffset);
}

/*
 * ======== _VC_rtcpFeedbackTmmbr() ========
 *
 * Generate a TMMBR Feedback packet that is placed at the current offset in the
 * message payload buffer.
 */
static vint _VC_rtcpFeedbackTmmbr(
    _VC_RtcpObject   *rtcp_ptr,
    _VTSP_RtcpCmdMsg *msg_ptr,
    uint32            bitrateInKbps,
    vint              offset)
{
    uint32 temp32;
    vint newoffset = offset;

    temp32 = 0x2 << 30; /* version number */
    temp32 |= 0 << 29; /* padding */
    temp32 |= _VTSP_RTCP_FMT_TMMBR << 24; /* Feedback Message Type for TMMBR */
    temp32 |= _VTSP_RTCP_PTYPE_RTPFB << 16; /* TMMBR is a Transport Layer FB Message - RTPFB */
    temp32 |= 4; /* length = 2 + 2n where n=1 refers to number of FCI entries  */

    msg_ptr->msg.payload[newoffset++] = OSAL_netHtonl(temp32);
    msg_ptr->msg.payload[newoffset++] = OSAL_netHtonl(rtcp_ptr->ssrc); /* SSRC of this RTCP packet sender */
    /* SSRC of the Media Source SHALL NOT be used. Set to 0. RFC 5104 - Section 4.2.1.2 */
    msg_ptr->msg.payload[newoffset++] = OSAL_netHtonl(0);

    /* FCI Entry for TMMBR. */
    msg_ptr->msg.payload[newoffset++] = OSAL_netHtonl(rtcp_ptr->mediaSsrc); /* SSRC of the Media Source */
    temp32 = 10 << 26;  /* MxTBR Exponent (6 bits). 1 kbps = 2^10 bps. */
    temp32 |= (bitrateInKbps) << 9; /* MxTBR Mantissa (17 bits). */
    temp32 |= 15; /* RTP layer (H264) Packet Overhead. Approx = 15 bytes  */
    msg_ptr->msg.payload[newoffset++] = OSAL_netHtonl(temp32);

    msg_ptr->payloadSize += (newoffset - offset);

    return (newoffset);
}

/*
 * ======== _VC_rtcpFeedbackTmmbn() ========
 *
 * Generate a TMMBN Feedback packet that is placed at the current offset in the
 * message payload buffer.
 */
static vint _VC_rtcpFeedbackTmmbn(
    _VC_RtcpObject   *rtcp_ptr,
    _VTSP_RtcpCmdMsg *msg_ptr,
    uint32            bitrateInKbps,
    vint              offset)
{
    uint32 temp32;
    vint newoffset = offset;

    temp32 = 0x2 << 30; /* version number */
    temp32 |= 0 << 29; /* padding */
    temp32 |= _VTSP_RTCP_FMT_TMMBN << 24; /* Feedback Message Type for TMMBN */
    temp32 |= _VTSP_RTCP_PTYPE_RTPFB << 16; /* TMMBN is a Transport Layer FB Message - RTPFB */
    temp32 |= 4; /* length = 2 + 2n where n=1 refers to number of FCI entries  */

    msg_ptr->msg.payload[newoffset++] = OSAL_netHtonl(temp32);
    msg_ptr->msg.payload[newoffset++] = OSAL_netHtonl(rtcp_ptr->ssrc); /* SSRC of this RTCP packet sender */
    /* SSRC of the Media Source SHALL NOT be used. Set to 0. RFC 5104 - Section 4.2.2.2 */
    msg_ptr->msg.payload[newoffset++] = OSAL_netHtonl(0);

    /* FCI Entry for TMMBN. */
    msg_ptr->msg.payload[newoffset++] = OSAL_netHtonl(rtcp_ptr->ssrc); /* SSRC of the Sender */
    temp32 = 10 << 26;  /* MxTBR Exponent (6 bits). 1 kbps = 2^10 bps. */
    temp32 |= (bitrateInKbps) << 9; /* MxTBR Mantissa (17 bits) */
    temp32 |= 15; /* RTP layer (H264) Packet Overhead. Approx = 15 bytes  */
    msg_ptr->msg.payload[newoffset++] = OSAL_netHtonl(temp32);

    msg_ptr->payloadSize += (newoffset - offset);

    return (newoffset);
}

/*
 * ======== _VC_rtcpFeedbackPli() ========
 *
 * Generate a PLI Feedback packet that is placed at the current offset in the
 * message payload buffer.
 */
static vint _VC_rtcpFeedbackPli(
    _VC_RtcpObject   *rtcp_ptr,
    _VTSP_RtcpCmdMsg *msg_ptr,
    vint              offset)
{

    uint32 temp32;
    vint newoffset = offset;

    temp32 = 0x2 << 30; /* version number */
    temp32 |= 0 << 29; /* padding */
    temp32 |= _VTSP_RTCP_FMT_PLI << 24; /* Feedback Message Type for PLI */
    temp32 |= _VTSP_RTCP_PTYPE_PSFB << 16; /* PLI is Payload Specific FB Message - PSFB */
    temp32 |= 2; /* length = 2 */

    msg_ptr->msg.payload[newoffset++] = OSAL_netHtonl(temp32);
    msg_ptr->msg.payload[newoffset++] = OSAL_netHtonl(rtcp_ptr->ssrc);      /* SSRC of this RTCP packet sender */
    msg_ptr->msg.payload[newoffset++] = OSAL_netHtonl(rtcp_ptr->mediaSsrc); /* SSRC of the Media Source */

    /* There is no FCI Entry for PLI. */
    msg_ptr->payloadSize += (newoffset - offset);

    return (newoffset);
}

/*
 * ======== _VC_rtcpFeedbackFir() ========
 *
 * Generate a FIR Feedback packet that is placed at the current offset in the
 * message payload buffer.
 */
static vint _VC_rtcpFeedbackFir(
    _VC_RtcpObject   *rtcp_ptr,
    uint8             sequenceNumber,
    _VTSP_RtcpCmdMsg *msg_ptr,
    vint              offset)
{

    uint32 temp32;
    vint newoffset = offset;

    temp32 = 0x2 << 30; /* version number */
    temp32 |= 0 << 29; /* padding */
    temp32 |= _VTSP_RTCP_FMT_FIR << 24; /* Feedback Message Type for FIR */
    temp32 |= _VTSP_RTCP_PTYPE_PSFB << 16; /* FIR is Payload Specific FB Message - PSFB */
    temp32 |= 4; /* length = 2 + 2n where n=1 refers to number of FCI entries */

    //save the current time to the lastFirSend as this Fir has been sent out sucessfully
    OSAL_SelectTimeval  currentTime;  /* Current time as OSAL Timeval */
    OSAL_selectGetTime(&currentTime);
    rtcp_ptr->feedback.lastFirSend  = (currentTime.sec * 1000) + (currentTime.usec / 1000);
    rtcp_ptr->sendFirCount += 1;
    _VC_RTCP_LOG("RTCP send fir count: %d\n",rtcp_ptr->sendFirCount);

    msg_ptr->msg.payload[newoffset++] = OSAL_netHtonl(temp32);
    msg_ptr->msg.payload[newoffset++] = OSAL_netHtonl(rtcp_ptr->ssrc);      /* SSRC of this RTCP packet sender */
    msg_ptr->msg.payload[newoffset++] = 0; /* The SSRC of the Media Source must be zero. */

    /* FCI Entry for FIR. */
    msg_ptr->msg.payload[newoffset++] = OSAL_netHtonl(rtcp_ptr->mediaSsrc); /* SSRC of the Media Source */
    temp32  = sequenceNumber << 24; /* Command Seqn number. 1st 8 bits. */
    msg_ptr->msg.payload[newoffset++] = OSAL_netHtonl(temp32);

    msg_ptr->payloadSize += (newoffset - offset);

    return (newoffset);
}

/*
 * ======== _VC_rtcpBye() ========
 *
 */
vint _VC_rtcpBye(
    _VC_Queues   *q_ptr,
    _VC_NetObj   *net_ptr,
    uvint         infc,
    uvint         streamId)
{
    OSAL_logMsg("%s:%d :: UNIMPLEMENTED!", __FILE__, __LINE__);
    _VTSP_RtcpCmdMsg       message;
    _VTSP_RtcpCmdMsg       *msg_ptr;
    uint32                 temp32;
    uint32                 ssrc;
    vint                   offset = 0;
    _VC_RtcpObject         *rtcp_ptr;

    rtcp_ptr    = _VC_streamIdToRtcpPtr(net_ptr, streamId);

    /*
     * Determine whether it is appropriate to send the BYE packet. A packet may
     * only be sent if either an RTP or RTCP packet has been sent.
     */
    if ((0 == rtcp_ptr->rtpSendPacketCount) &&
            (0 == rtcp_ptr->sendPacketCount)) {
        return (_VC_RTP_OK);
    }

    /*
     * Create next message.
     */
    message.command = _VTSP_RTCP_CMD_SEND;
    message.infc = infc;
    message.streamId = streamId;
    msg_ptr = &message;
    msg_ptr->payloadSize = 0;

    /* Sender SSRC. */
    ssrc = OSAL_netHtonl(rtcp_ptr->ssrc);

    /* Add empty RR block if stream is inactive. RFC 3550 Section 6.1 */
    offset = _VC_rtcpEmptyReceiverReport(rtcp_ptr, msg_ptr, offset);

    /* ADD Source Description (SDES) block. RFC 3550 Section 6.1 */
    offset = _VC_rtcpSourceDescription(net_ptr, msg_ptr, offset, ssrc);

    /* ADD Goodbye (BYE) block. */
    temp32  = 0x2 << 30; /* version number */
    temp32 |= 0 << 29;   /* padding */
    temp32 |= 1 << 24;   /* SSRC Count (SC) usually 1 */
    temp32 |= _VTSP_RTCP_PTYPE_BYE << 16; /* packet type */
    temp32 |= 1; /* RFC 3550 Section 6.6 - Length = 1  */

    msg_ptr->msg.payload[offset++] = OSAL_netHtonl(temp32);
    msg_ptr->msg.payload[offset++] = ssrc; /* SSRC of sender */
    msg_ptr->payloadSize += 2;

    /* convert the payload size into bytes. */
    msg_ptr->payloadSize *= sizeof(uint32);

    /* Send message. */
    if (OSAL_SUCCESS != OSAL_msgQSend(q_ptr->rtcpMsg, (char *) msg_ptr,
            sizeof(_VTSP_RtcpCmdMsg), OSAL_NO_WAIT, NULL)) {
        _VC_TRACE(__FILE__, __LINE__);
        return (_VC_RTP_ERROR);
    }

    return (_VC_RTP_OK);
}
/*
 * ======== _VC_rtcpSend() ========
 *
 * This function is used to exchange data with an RTCP stream.
 */
vint _VC_rtcpSend(
    _VC_Obj          *vc_ptr,
    _VC_StreamObj    *stream_ptr,
    vint              infc,
    vint              streamId)
{
    VTSP_StreamDir    dir;
    JBV_Obj          *jbv_ptr;
    _VC_Dsp          *dsp_ptr;
    _VC_Queues       *q_ptr;
    _VC_RtcpObject   *rtcp_ptr;
    _VC_NetObj       *net_ptr;
    JBV_RtcpInfo     jbvRtcpInfo;
    JBV_PacketLoss    packetLoss;
    _VTSP_RtcpCmdMsg  message;
    _VTSP_RtcpCmdMsg *msg_ptr;

    vint              offset = 0;
    uint32            feedbackMask;
    _VC_RtcpNtpTime   currentTime;

    q_ptr       = vc_ptr->q_ptr;
    net_ptr     = vc_ptr->net_ptr;
    dsp_ptr     = vc_ptr->dsp_ptr;

    rtcp_ptr    = _VC_streamIdToRtcpPtr(net_ptr, streamId);


    stream_ptr = dsp_ptr->streamObj_ptr[rtcp_ptr->streamId];
    jbv_ptr = &stream_ptr->dec.jbObj;
    dir = stream_ptr->streamParam.dir;

    /* Update the _VC_RtcpObject Object with any new information from outgoing RTP Packets. */
    _VC_rtcpUtilProcessRtp(net_ptr, rtcp_ptr, &currentTime);

    /* Update the JBV_RtcpInfo Object with any new information from the jitter buffer. */
    feedbackMask = _VC_rtcpUtilitProcessJbvRtcpInfo(jbv_ptr, rtcp_ptr, &jbvRtcpInfo);
    packetLoss = jbvRtcpInfo.packetLoss;

    //_VC_LOG("RTCP send - lostSeqnLength:%u\n", packetLoss.lostSeqnLength);

    OSAL_memSet(&message, 0, sizeof(_VTSP_RtcpCmdMsg));

    /* Create next message. */
    message.command  = _VTSP_RTCP_CMD_SEND;
    message.infc     = rtcp_ptr->infc;
    message.streamId = rtcp_ptr->streamId;
    msg_ptr = &message;
    /* Create a Full Compound RTCP packet. RFC 4584 Section 3.1 and 3.5.3  */
    if ((VTSP_STREAM_DIR_SENDONLY == dir) || (VTSP_STREAM_DIR_SENDRECV == dir)) {
        /* Add SR block if we are actively sending. */
        offset = _VC_rtcpSenderReport(rtcp_ptr, dir, &currentTime, msg_ptr, offset);
    }
    else if (VTSP_STREAM_DIR_RECVONLY == dir) {
        /* Add RR block if we are not sending. */
        offset = _VC_rtcpReceiverReport(rtcp_ptr, &currentTime, msg_ptr, offset);
    }
    else if (VTSP_STREAM_DIR_INACTIVE == dir) {
        /* Add empty RR block if stream is inactive. */
        offset = _VC_rtcpEmptyReceiverReport(rtcp_ptr, msg_ptr, offset);
    }

    /* ADD Source Description (SDES) block .*/
    offset = _VC_rtcpSourceDescription(net_ptr, msg_ptr, offset, rtcp_ptr->ssrc);

    if (rtcp_ptr->configure.enableMask & VTSP_MASK_RTCP_XR) {
        /* ADD Extended Report if needed. RFC 3611. */
        offset = _VC_rtcpExtendedReport(rtcp_ptr, msg_ptr, offset);
    }

    /* Add any SDP negotiated Feedback Messages that are currently pending. */
    if (rtcp_ptr->configure.enableMask & VTSP_MASK_RTCP_FB_NACK) {
        /* Add Generic NACK RTCP feedback. Determining when to send NACK is handled inside this method */
        offset = _VC_rtcpFeedbackGenericNack(rtcp_ptr, &packetLoss, msg_ptr, offset);
    }

    if ((rtcp_ptr->configure.enableMask & VTSP_MASK_RTCP_FB_TMMBR) &&
            (feedbackMask & VTSP_MASK_RTCP_FB_TMMBR)) {
        /* Add TMMBR RTCP feedback. Determining when to send TMMBR is handled inside this method */
        offset = _VC_rtcpFeedbackTmmbr(rtcp_ptr, msg_ptr, rtcp_ptr->feedback.sendTmmbrInKbps, offset);
	OSAL_logMsg("%s: TMMBR sendTmmbrInKbps=%u Kbps\n", __FUNCTION__, rtcp_ptr->feedback.sendTmmbrInKbps);
    }

    if ((rtcp_ptr->configure.enableMask & VTSP_MASK_RTCP_FB_TMMBN) &&
            (feedbackMask & VTSP_MASK_RTCP_FB_TMMBN)) {
        /* Add TMMBN RTCP feedback. */
        offset = _VC_rtcpFeedbackTmmbn(rtcp_ptr, msg_ptr, rtcp_ptr->feedback.sendTmmbnInKbps, offset);
        /* TMMBN has been sent, remove this flag */
        CLEAR_FLAG(rtcp_ptr, VTSP_MASK_RTCP_FB_TMMBN);
        OSAL_logMsg("%s: TMMBN sendTmmbnInKbps=%u Kbps\n", __FUNCTION__, rtcp_ptr->feedback.sendTmmbnInKbps);
    }

    if ((rtcp_ptr->configure.enableMask & VTSP_MASK_RTCP_FB_PLI) &&
                (feedbackMask & VTSP_MASK_RTCP_FB_PLI)) {
        /* should send PLI (during huge packet loss) */
        /* Add PLI RTCP feedback. */
        offset = _VC_rtcpFeedbackPli(rtcp_ptr, msg_ptr, offset);
        /* PLI has been sent, remove this flag */
        CLEAR_FLAG(rtcp_ptr, VTSP_MASK_RTCP_FB_PLI);
    }

    if ((rtcp_ptr->configure.enableMask & VTSP_MASK_RTCP_FB_FIR) &&
                (feedbackMask & VTSP_MASK_RTCP_FB_FIR)) {
        offset = _VC_rtcpFeedbackFir(rtcp_ptr, rtcp_ptr->feedback.firSeqNumber, msg_ptr, offset);
        /* FIR has been sent, remove this flag */
        CLEAR_FLAG(rtcp_ptr, VTSP_MASK_RTCP_FB_FIR);
        OSAL_logMsg("%s: FIR firSeqNumber=%u\n", __FUNCTION__, rtcp_ptr->feedback.firSeqNumber);
    }

    /* convert the payload size into bytes. */
    msg_ptr->payloadSize *= sizeof(uint32);

    /* Send message. */
    rtcp_ptr->sendPacketCount += 1;
    _VC_LOG("RTCP send packet count: %d, lostSeqnLength:%u\n",
		    rtcp_ptr->sendPacketCount, packetLoss.lostSeqnLength);
    if (OSAL_SUCCESS != OSAL_msgQSend(q_ptr->rtcpMsg, (char *) msg_ptr,
                    sizeof(_VTSP_RtcpCmdMsg), OSAL_NO_WAIT, NULL)) {
        _VC_TRACE(__FILE__, __LINE__);
        return (_VC_RTP_ERROR);
    }

    return (_VC_RTP_OK);
}
