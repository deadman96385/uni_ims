/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 5312 $ $Date: 2008-02-18 19:05:41 -0600 (Mon, 18 Feb 2008) $
 */

#ifndef _JBV_H_
#define _JBV_H_

#include <video.h>

#define LOG_TAG "JBV"

#ifdef VIDEO_DEBUG_LOG
# define DBG(fmt, args...) \
        OSAL_logMsg("%s %d:" fmt, __FUNCTION__, __LINE__, ## args)
#define ERR(fmt, args...) \
        OSAL_logMsg("%s %d: ERROR" fmt, __FUNCTION__, __LINE__, ## args)
#else
# define DBG(fmt, args...) 
# define ERR(fmt, args...) 
#endif



/*
 * This is maximum difference between two seqn in JB.
 */
#define _JBV_SEQN_MAXDIFF    (256)

#define JBV_MAX_JITTER       (1000000) /* in usecs */

#define JBV_INIT_NO_DROP_USEC (2000000)

typedef struct {
    uint64   ts;
    uint64   atime;
    int      ptype;
    uint16   seqn;
    uint16   offset;
    vint     valid;
    vint     mark;
    uvint    key;
    uvint    firstInSeq;
    uint8    data_ptr[VIDEO_MAX_RTP_SZ + 128];
} JBV_Unit;

typedef struct {
    uint64   firstAtime;
    uint64   firstTs;
    uint64   lastTs;
    uint64   lastDrawnTs;
    uint64   lastDrawnSeqn;
    uint16   lastSeqn;
    uint64   lastAtime;
    uint16   decayRt;
    uint64   jitter;
    uint64   iTime;
    uint64   framePeriod;
    uint64   lastCtime;
    uint64   fpsTime;
    vint     frames;
    vint     drop;
    vint     ready;
    vint     loss;
    uint64   minJitter;
    vint     accmRate;
    vint     accm;
    vint     level;
    vint     dropTillNextKey;
    vint     offset;
    vint     ppsRecvd;
    vint     spsRecvd;
    JBV_Unit unit[_JBV_SEQN_MAXDIFF];
    uint8    data[VIDEO_NET_BUFFER_SZ];
} JBV_Obj;

typedef struct {
    uint16 decayRt;   /* High envelop decay rate Q16 */
    uint32 minJitter; /* ms jitter minimum */
    vint   accmRate;  /* rate at which to slowly accumulate packets */
} JBV_Params;

typedef struct {
    uint32   ts;                    /* Time Stamp, increments by samples */
    vint     valid;                 /* Valid flag */
    uint64   atime;                 /* Packet arrival time, local clock */
    vint     type;                  /* Packet type */
    uint16   seqn;                  /* Sequence number */
    uint32   pSize;                 /* Packet size in octets */
    vint     mark;
    uint8   *data_ptr;
} JBV_Pkt;

/*
 * Prototypes
 */

int JBV_init(
    JBV_Obj    *obj_ptr,
    JBV_Params *params_ptr);

void JBV_putPkt(
    JBV_Obj *obj_ptr,
    JBV_Pkt *pkt_ptr);

void JBV_getPkt(
    JBV_Obj *obj_ptr,
    JBV_Pkt *pkt_ptr);

#endif
