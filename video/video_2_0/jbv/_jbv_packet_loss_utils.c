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
 * ======== _JBV_updatePacketDrop() ========
 *
 * Update JBV Packet Drop metrics
 * Returns:
 */
static void _JBV_updatePacketDrop(
        JBV_RtcpInfo  *rtcpInfo_ptr,
        JBV_PacketDrop packetDropFlag) {
    JBV_PacketLoss* packetLossInfo_ptr;

    packetLossInfo_ptr = &rtcpInfo_ptr->packetLoss;

    switch (packetDropFlag) {
        case JBV_DROP_OVERFLOW:
            packetLossInfo_ptr->jbvOverflowDrop++;
            JBV_errLog("JBV PacketDrop - JBV overflow\n");
            break;
        case JBV_DROP_TOO_OLD:
            packetLossInfo_ptr->tooOldPacketDrop++;
            JBV_errLog("JBV PacketDrop - packet too old\n");
            break;
        case JBV_DROP_INVALID:
        default:
            packetLossInfo_ptr->invalidPacketDrop++;
            JBV_errLog("JBV PacketDrop - invalid packet\n");
            break;

    }

    /* Update the total packetDrop count. */
    packetLossInfo_ptr->totalPacketDrop++;
}

/*
 * ======== _JBV_dropPacket() ========
 *
 * Drop a packet and update JBV Packet Drop metrics
 * Returns:
 */
void _JBV_dropPacket(
    JBV_Obj  *obj_ptr,
    JBV_Unit *packet_ptr,
    JBV_PacketDrop packetDropFlag) {

    JBV_RtcpInfo *rtcpInfo_ptr = &obj_ptr->rtcpInfo;

    if (NULL != packet_ptr) {
        /* Drop the packet by invalidating it. */
        packet_ptr->valid = 0;
        if (packet_ptr->key) {
            rtcpInfo_ptr->keyFrameDropped = OSAL_TRUE;
        }
    }

    _JBV_updatePacketDrop(rtcpInfo_ptr, packetDropFlag);
}
/*
 * ======== _JBV_dropOlderPackets() ========
 *
 * Drops JBV units that have same or older timestamps than
 * the given timeStamp
 * Returns:
 */
void _JBV_dropOlderPackets(
        JBV_Obj  *obj_ptr,
        uint64 timeStamp) {

    uint16    seqn;
    JBV_Unit *unit_ptr;

    for (seqn = 0; seqn < _JBV_SEQN_MAXDIFF; seqn++) {
        unit_ptr = &obj_ptr->unit[seqn];
        /*
         * Invalidate all packets that have same or older timestamp
         * compared to given timestamp.
         */
        if (unit_ptr->valid && (unit_ptr->ts < timeStamp)&& (_JBV_TS_INIT != timeStamp)) {
            /* Drop the packet by invalidating it. */
            unit_ptr->valid = 0;
            _JBV_dropPacket(obj_ptr, unit_ptr, JBV_DROP_TOO_OLD);
            JBV_wrnLog("JBV dropped %s packet at location %u[%u] ts %llu",
                    unit_ptr->key ? "key" : "non-key", seqn, unit_ptr->seqn,
                    unit_ptr->ts);
        }
    }
}
/*
 * ======== _JBV_checkForPacketLoss() ========
 *
 * Check for packet loss and updates JBV_PacketLoss
 * Returns:
 */
void _JBV_checkForPacketLoss(
    JBV_Obj  *obj_ptr,
    JBV_Unit *unit_ptr,
    JBV_Timeval *tv_ptr)
{
    vint i;
    vint rtp_seqn;
    vint rtpSeqnDiff;
    uint64 curTime;
    JBV_PacketLoss* packetLossInfo_ptr;

    packetLossInfo_ptr = &obj_ptr->rtcpInfo.packetLoss;

    /* Skip the first packet. */
    if (obj_ptr->lastSeqn != 0) {
        /* Update maximum received seqn number */
        if (obj_ptr->lastSeqn >= obj_ptr->lastMaxSeqn) {
            obj_ptr->lastMaxSeqn = obj_ptr->lastSeqn;
        }
        /* Find time stamp of current time. */
        curTime = ((uint64)tv_ptr->sec) * _JBV_SEC_TO_USEC + (uint64)tv_ptr->usec;
        curTime -= obj_ptr->initTime;

        /* Calculate the difference between the incoming packet RTP Seqn and last Rcvd MAXIMUM RTP Seqn. */
        rtpSeqnDiff = unit_ptr->seqn - obj_ptr->lastMaxSeqn;

        /* Check for out of order packet. */
        if (rtpSeqnDiff < 0) {
            JBV_dbgLog("PacketLoss - Out of order packet!! last packet: %u current packet: %u",
                    obj_ptr->lastSeqn, unit_ptr->seqn);

            /* Remove this packet from lost list. Traverse from last to first element. */
            for (i = packetLossInfo_ptr->lostSeqnLength - 1; i >= 0; i--) {
                if (packetLossInfo_ptr->lostSeqn[i] == unit_ptr->seqn) {
                    /* Delete by left shift the elements. */
                    OSAL_memMove(packetLossInfo_ptr->lostSeqn + i,
                            packetLossInfo_ptr->lostSeqn + i + 1,
                            (packetLossInfo_ptr->lostSeqnLength - i - 1) * sizeof(uint16));
                    /* Decrement the lostPacketCount. */
                    packetLossInfo_ptr->lostPacketCount--;
                    packetLossInfo_ptr->lostSeqnLength--;
                    break;
                }
                else {
                    continue;
                }
            }

            /* Print the updated list. */
            JBV_dbgLog("PacketLoss - TimeStamp: %llu Count %u", curTime,
                    packetLossInfo_ptr->lostPacketCount);
        }
        else if (rtpSeqnDiff == 0) {
            JBV_dbgLog("PacketLoss - Redundant packet!! last packet: %u current packet: %u",
                    obj_ptr->lastSeqn, unit_ptr->seqn);
        }
        else if (rtpSeqnDiff == 1) {
            /* Packets arriving in order. No packet loss. */
//            JBV_dbgLog("PacketLoss - In order packet!! last packet: %u current packet: %u",
//                    obj_ptr->lastSeqn, unit_ptr->seqn);
        }
        else if (rtpSeqnDiff > 1) {
            JBV_dbgLog("PacketLoss - Lost packet!! last packet: %u current packet: %u",
                    obj_ptr->lastSeqn, unit_ptr->seqn);

            /* Check for extreme packet loss. */
            if (rtpSeqnDiff >= (JBV_PACKET_LOSS_CACHE_SIZE >> 1)) {
                /* We need more than half cache to be empty. */
                JBV_dbgLog("PacketLoss - Extreme Packet Loss!!");

                /* TODO send PLI and Generic Nack as we will lose many data. */

                packetLossInfo_ptr->lostSeqnLength = 0;
                /* Completely flush the cache and fill it half. */
                for (rtp_seqn = unit_ptr->seqn - (JBV_PACKET_LOSS_CACHE_SIZE >> 1);
                        rtp_seqn < unit_ptr->seqn; rtp_seqn++) {
                    packetLossInfo_ptr->lostSeqn[packetLossInfo_ptr->lostSeqnLength] = rtp_seqn;
                    packetLossInfo_ptr->lostSeqnLength++;
                }
            }
            else {
                /* We need less than half cache. Check to see if we have space. */
                if (rtpSeqnDiff > JBV_PACKET_LOSS_CACHE_SIZE - packetLossInfo_ptr->lostSeqnLength) {
                    JBV_dbgLog("JBV_PacketLoss Cache not enough space. Erase and Reduce.");
                    /* Cut down the cache by half. */
                    packetLossInfo_ptr->lostSeqnLength -= (JBV_PACKET_LOSS_CACHE_SIZE >> 1);
                    /* Delete half of the cache by left shift the remaining elements. */
                    OSAL_memMove(packetLossInfo_ptr->lostSeqn,
                            packetLossInfo_ptr->lostSeqn + (JBV_PACKET_LOSS_CACHE_SIZE >> 1),
                            packetLossInfo_ptr->lostSeqnLength * sizeof(uint16));
                }

                /* Add the in-between missing packets to lost list. */
                for (rtp_seqn = obj_ptr->lastMaxSeqn + 1; rtp_seqn < unit_ptr->seqn; rtp_seqn++) {
                    packetLossInfo_ptr->lostSeqn[packetLossInfo_ptr->lostSeqnLength] = rtp_seqn;
                    packetLossInfo_ptr->lostSeqnLength++;
                }
            }

            /* Update the lostPacketCount. */
            packetLossInfo_ptr->lostPacketCount += (rtpSeqnDiff - 1);

            /* Print the updated list. */
            JBV_dbgLog("PacketLoss - TimeStamp %llu Count %u", curTime,
                    packetLossInfo_ptr->lostPacketCount);
        }
    }
}
