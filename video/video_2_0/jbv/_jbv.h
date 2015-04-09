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

/*
 * Second to usec/ms multiplier
 */
#define _JBV_SEC_TO_USEC      (1000000)
#define _JBV_SEC_TO_MSEC      (1000)

/*
 * Timestamp range, max 32-bit value
 */
#define _JBV_TS_RANGE         (0xFFFFFFFFLL)

/*
 * Init timestamp value, maximum value of uint64.
 */
#define _JBV_TS_INIT          (0xFFFFFFFFFFFFFFFFLL)

/* JBV Packet drop flags. */
typedef enum {
    JBV_DROP_INVALID  = 0,
    JBV_DROP_OVERFLOW = 1,
    JBV_DROP_TOO_OLD  = 2,
} JBV_PacketDrop;

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
    vint      seqn);

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
    JBV_Pkt *pkt_ptr);


int32 _JBV_stateMachine(
    int32  state,
    uint64  jitter,
    uint64 level,
    uint64 framePeriod);

void _JBV_dropOldestFrame(
    JBV_Obj  *obj_ptr);

/*
 * ======== _JBV_checkForPacketLoss() ========
 *
 * Check for packet loss and updates JBV_PacketLoss
 * Returns:
 */
void _JBV_checkForPacketLoss(
    JBV_Obj     *obj_ptr,
    JBV_Unit    *unit_ptr,
    JBV_Timeval *tv_ptr);

/*
 * ======== _JBV_dropPacket() ========
 *
 * Drop a packet and update JBV Packet Drop metrics
 * Returns:
 */
void _JBV_dropPacket(
    JBV_Obj  *obj_ptr,
    JBV_Unit *packet_ptr,
    JBV_PacketDrop packetDropFlag);

/*
 * ======== _JBV_dropOlderPackets() ========
 *
 * Drops JBV units that have same or older timestamps than
 * the given timeStamp
 * Returns:
 */
void _JBV_dropOlderPackets(
    JBV_Obj  *obj_ptr,
    uint64 timeStamp);
