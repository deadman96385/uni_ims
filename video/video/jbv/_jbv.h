/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 5312 $ $Date: 2008-02-18 19:05:41 -0600 (Mon, 18 Feb 2008) $
 */

#define _JBV_H263_MODE(f, p) \
    ((f) == 0) ? 4 : \
            ((f) == 1) && ((p) == 0) ? 8 : \
                    ((f) == 1) && ((p) == 1) ? \
                            12 : -1

#define _JBV_USEC_TO_90K(usec) \
    (((usec) * 5898UL) >> 16) /* divide by 11.1111 */

#define _JBV_90K_TO_USEC(k90) \
    (((k90) * 728178UL) >> 16) /* mul by 11.111 */

typedef enum {
    JB_OK = 0,
    JB_ERROR = -1,
    JB_NO_PACKETS_READY = -2,
} JB_Return;

/*
 * _jbv.c
 */

/*
 * ======== _JBV_updateFramePeriod() ========
 *
 * Updates frame period.
 * Returns:
 */
void _JBV_updateFramePeriod(
    JBV_Obj  *obj_ptr,
    JBV_Unit *unit_ptr);

/*
 * ======== _JBV_updateJitter() ========
 *
 * Updates jitter when a new packet arrives.
 * Returns:
 */
void _JBV_updateJitter(
    JBV_Obj  *obj_ptr,
    JBV_Unit *unit_ptr);

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
    uint64   *ts_ptr);

/*
 * ======== _JBV_cleanup() ========
 *
 * Drops too old and zombie packets.
 * Returns:
 */
void _JBV_cleanup(
    JBV_Obj  *obj_ptr);

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
    uint64   *level_ptr);

/*
 * _jbv_coder.c
 */

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
    JBV_Pkt  *pkt_ptr);

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
    JBV_Pkt *pkt_ptr);

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
    JBV_Pkt  *pkt_ptr);

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
    JBV_Pkt *pkt_ptr);
