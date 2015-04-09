/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2003-2007 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * +D2Tech+ Rev: 122  Date: 2007-08-01 14:36:22 -0700 (Wed, 01 Aug 2007) 
 * +D2Tech+ Release Version: trunk-plc
 */

#ifndef _PLC_H_
#define _PLC_H_

#include <comm.h>

/* 
 * Buffer size for this algorithm
 */
#define PLC_BLOCKSZ_8K   (80)             /* 8K sample rate */
#define PLC_BLOCKSZ      (PLC_BLOCKSZ_8K) /* for backward compatibility */
#define PLC_BLOCKSZ_16K  (160)            /* 16K sample rate */

/* 
 * Control/Status word values
 */

/* Set this bit to indicate a bad packet, unset for good packet */
#define PLC_CTRL_BPI (0x0001)

/* Set this bit to indicate 16 Ksamp/sec data; otherwise, 8 Ksamp/sec data */
#define PLC_CTRL_WB  (0x0010)

/* This flag is set by the PLC when it is producing silence at its output */
#define PLC_STAT_SIL (0x0100)
   
/*
 * Internal Object:
 */
typedef struct {
    char internal_ary[0x1f8c];
} _PLC_Internal;

/* 
 * PLC object
 */
typedef struct {
    _PLC_Internal  internal;
    vint           ctrlStat;  /* Control word, set to 0x0 at PLC_init() */
    vint          *pkt_ptr;   /* Packet payload pointer, source and dest. */
} PLC_Obj;

/*
 * Function prototypes
 */
void PLC_init(
    PLC_Obj *plcObj_ptr);

void PLC_run(
    PLC_Obj *plcObj_ptr);

#endif /* _PLC_ */

