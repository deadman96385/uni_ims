/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2003-2007 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * +D2Tech+ Rev: 241  Date: 2009-04-14 15:29:40 -0700 (Tue, 14 Apr 2009) 
 * +D2Tech+ Release Version: trunk-bnd
 */

#ifndef _BND_H_
#define _BND_H_

#include <d2types.h>
#include <comm.h>

/*
 * Constants for BND Local parameters
 */
#define BND_pNOISE_THRESH_DEF      (-30)
#define BND_tSUSTAIN_DEF           (3)    /* default voice sustain time */
#define BND_tHOLDOVER_DEF          (15)   /* default voice hold time */

/*
 * Constants for BND voice detection
 */
#define BND_CHAN_ACTIVE_CLEAR   (0)
#define BND_CHAN_ACTIVE_SET     (1)
#define BND_VOICE_ACTIVE_CLEAR  (0)
#define BND_VOICE_ACTIVE_SET    (1)

/*
 * Constants for BND initialization
 */
#define BND_COLD_START          (0)
#define BND_WARM_START          (1)

/*
 * Internal Object:
 */
typedef struct {
    char internal_ary[0x1a8];
} _BND_Internal;
/*
 * Local parameters structure: User configured.
 */
typedef struct {
    uvint pBLKSIZE;
    vint  pNOISE_THRESH;
    uvint tSUSTAIN;
    uvint tHOLDOVER;
} BND_Params;

/*
 * Object definition:
 */ 
typedef struct {
    _BND_Internal  internal;        /* Internal */
    vint           bndNoise;        /* background noise in dBm * 10 */
    uvint          chanActive;      /* 1st tier voice detection flag */
    uvint          voiceActive;     /* 2nd tier voice detection flag */
    vint          *src_ptr;         /* Source data pointer */
} BND_Obj;

/*
 * Function prototypes:
 */ 
void BND_init(
    BND_Obj       *bndObj_ptr,
    GLOBAL_Params *global_ptr,
    BND_Params    *bndParams_ptr,
    vint           startMode);

void BND_run(
    BND_Obj *bndObj_ptr);


#endif /* _BND_H_ */
