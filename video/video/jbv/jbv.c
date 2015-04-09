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

//#define JBV_H264_WAIT_FOR_SPS_PPS

/*
 * ======== JBV_init() ========
 *
 * Init the video jitter buffer.
 * Returns:
 * 0 : Success
 * -1 : Failed.
 */
int JBV_init(
    JBV_Obj    *obj_ptr,
    JBV_Params *params_ptr)
{
    if ((NULL == obj_ptr) || (NULL == params_ptr)) {
        return (-1);
    }

    memset(obj_ptr, 0, sizeof(JBV_Obj));
    obj_ptr->decayRt = params_ptr->decayRt & 0xFFFF;
    obj_ptr->accmRate = params_ptr->accmRate;

    /*
     * All JB calculations in usec.
     */
    obj_ptr->minJitter = (uint64)params_ptr->minJitter;
    obj_ptr->minJitter *= 1000;
    /*
     * Start with 100 ms jitter and then adapt.
     * This will work till 10 fps, but not under.
     */
    obj_ptr->jitter = 100000;

    /*
     * Start with frame rate of 5 fps.
     * This will also be adapted.
     */
    obj_ptr->framePeriod = 200000;

    return (0);
}

/*
 * ======== JBV_putPkt() ========
 *
 * Puts a packet in the buffer.
 */
void JBV_putPkt(
    JBV_Obj *obj_ptr,
    JBV_Pkt *pkt_ptr)
{
    uint16    seqn;
    JBV_Unit *unit_ptr;
    OSAL_SelectTimeval time;

    if ((NULL == obj_ptr) || (NULL == pkt_ptr)) {
        return;
    }

    if (!pkt_ptr->valid) {
        return;
    }

    /*
     * The condition thats important every time.
     */
    if (obj_ptr->jitter < obj_ptr->minJitter) {
        obj_ptr->jitter = obj_ptr->minJitter;
    }

    /*
     * It is highly unlikely that for real time comm., seqn
     * will be out of order by 256 places.
     * Even if it is, the packet is too out of place and 
     * has no hope of recovery.
     */
    seqn = pkt_ptr->seqn & (_JBV_SEQN_MAXDIFF - 1);
    unit_ptr = &obj_ptr->unit[seqn];

    /*
     * Time stamp, arrival time adjustment.
     */
    if (!obj_ptr->ready) {
        obj_ptr->firstAtime = pkt_ptr->atime;
        obj_ptr->firstTs = pkt_ptr->ts;
        OSAL_selectGetTime(&time);
        obj_ptr->iTime = time.sec * 1000000 + time.usec;
        obj_ptr->ready = 1;
    }

    if (obj_ptr->firstTs > pkt_ptr->ts) {
        /*
         * XXX:
         * This needs better protection.
         */
        obj_ptr->firstTs = pkt_ptr->ts;
        ERR("Warning! time stamp wrap around\n");
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
                    obj_ptr->drop++;
                    ERR("invalid H263 packet\n");
                    return;
                }
                break;
            case VTSP_CODER_VIDEO_H264:
                if (0 != _JBV_processH264(obj_ptr, unit_ptr, pkt_ptr)) {
                    obj_ptr->drop++;
                    ERR("invalid H264 packet\n");
                    return;
                }
                break;
            default:
                obj_ptr->drop++;
                ERR("invalid packet\n");
                return;
        }

        DBG("Put packet seqn=%u at location %u of size %u ts %llu atime %llu"
                " key=%02x, mark=%02x firstinSeq=%02x\n", pkt_ptr->seqn, seqn, 
                unit_ptr->offset, unit_ptr->ts, unit_ptr->atime,
                unit_ptr->key, unit_ptr->mark, unit_ptr->firstInSeq);

        /*
         * Update jitter based on last packet.
         */
        _JBV_updateJitter(obj_ptr, unit_ptr);

        /*
         * Update frame period.
         */
        _JBV_updateFramePeriod(obj_ptr, unit_ptr);

        /*
         * Update last instances.
         */
        obj_ptr->lastTs = unit_ptr->ts;
        obj_ptr->lastAtime = unit_ptr->atime;
        obj_ptr->lastSeqn = unit_ptr->seqn;
    }
    else {
        obj_ptr->drop++;
        ERR("JBV: Packet is larger than what can fit pSize=%d, fit=%d\n",
                pkt_ptr->pSize, sizeof(unit_ptr->data_ptr));
        return;
    }

    /*
     * Now drop hopeless packets.
     */
    OSAL_selectGetTime(&time);
    if ((time.sec * 1000000 + time.usec - obj_ptr->iTime) >
            JBV_INIT_NO_DROP_USEC) {
        _JBV_cleanup(obj_ptr);
    }
}

/*
 * ======== JBV_getPkt() ========
 *
 * Gets a packet to decode from the buffer.
 */
void JBV_getPkt(
    JBV_Obj *obj_ptr,
    JBV_Pkt *pkt_ptr)
{
    JBV_Unit *unit_ptr;
    uint16   mSeqn;
    uint16   m1Seqn;
    uint64   level;
    uint64   cTime;
    vint     retVal;
    OSAL_SelectTimeval time;

    if ((NULL == obj_ptr) || (NULL == pkt_ptr)) {
        return;
    }

    pkt_ptr->pSize = 0;
    pkt_ptr->valid = 0;
    pkt_ptr->data_ptr = NULL;

    if (!obj_ptr->ready) {
        return;
    }
    
    /*
     * Find oldest packet's time stamp with mark set.
     */
    retVal = _JBV_findOldestSequence(obj_ptr, &m1Seqn, &mSeqn, &level);
    if (0 == retVal) {
        unit_ptr =  &obj_ptr->unit[mSeqn];

        /*
         * Adjust draw rate based on calculated frame rate.
         * But give packets of same time stamp.
         */
        if (obj_ptr->lastDrawnTs != unit_ptr->ts) {
            /*
             * Find time stamp of current time.
             */
            OSAL_selectGetTime(&time);
            cTime = time.sec * 1000000 + time.usec;
            cTime -= obj_ptr->iTime;

            /*
             * This check will make sure that if we are running low, 
             * we return if we have drawn as recently
             * as one frame period ago.
             */
            if (level < (obj_ptr->jitter + obj_ptr->framePeriod)) {
                if ((cTime - obj_ptr->lastCtime) < obj_ptr->framePeriod) {
                    return;
                }
            }

            /*
             * Before anything, accumulate if buffer running low.
             */
            if (level < obj_ptr->jitter) {
                obj_ptr->accm++;
            }
            else {
                obj_ptr->accm = 0;
            }
            if (obj_ptr->accm >= obj_ptr->accmRate) {
                /*
                 * Accm this packet.
                 */
                obj_ptr->accm = 0;
                obj_ptr->loss++;
                return;
            }
            obj_ptr->lastCtime = cTime;
        }


        switch(unit_ptr->ptype) {
            case VTSP_CODER_VIDEO_H263:
                /*
                 * Series ready ? yes : give. no : return
                 */
                if (0 == _JBV_reassembleH263(obj_ptr, m1Seqn, mSeqn, pkt_ptr)) {
                    DBG("Got packet at location %u, size %u, new level %llu\n",
                            mSeqn, pkt_ptr->pSize, level);
                    obj_ptr->lastDrawnTs = obj_ptr->unit[mSeqn].ts;
                    obj_ptr->lastDrawnSeqn = mSeqn;
                }
                break;
            case VTSP_CODER_VIDEO_H264:
                /*
                 * Series ready ? yes : give. no : return
                 */
                if (0 == _JBV_reassembleH264(obj_ptr, m1Seqn, mSeqn, pkt_ptr)) {
                    DBG("Got packet seqn=%u at location %u, size %u, new level "
                            "%llu\n", unit_ptr->seqn, mSeqn, pkt_ptr->pSize, level);
                    obj_ptr->lastDrawnTs = obj_ptr->unit[mSeqn].ts;
                    obj_ptr->lastDrawnSeqn = mSeqn;
                }
                break;
            default:
                ERR("invalid packet\n");
                obj_ptr->drop++;
                return;
        }
    }
    else {
        /*
         * For informational purposes.
         */
        OSAL_selectGetTime(&time);
        cTime = time.sec * 1000000 + time.usec;
        cTime -= obj_ptr->iTime;
        if ((cTime - obj_ptr->lastCtime) > obj_ptr->framePeriod) {
            if (retVal == JB_NO_PACKETS_READY) {
                //DBG("No Packet Available framePeriod=%llu\n",
                       // obj_ptr->framePeriod);
            }
            else {
                ERR("ERROR loss?, check this\n");
            }
            obj_ptr->loss++;
        }
    }
}
