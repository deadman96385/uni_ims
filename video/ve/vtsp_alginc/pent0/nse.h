/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2003-2006 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * +D2Tech+ Rev: 5  Date: 2006-09-21 13:12:05 -0700 (Thu, 21 Sep 2006) 
 * +D2Tech+ Release Version: trunk-nse
 */

#ifndef _NSE_H_
#define _NSE_H_

#include <comm.h>

/* 
 * Macros
 */

/* 
 * Maximum and minimum blockSz
 */
#define NSE_BLOCKSZ_MIN (1)    
#define NSE_BLOCKSZ_MAX (8000) 

/* 
 * Maximum and minimum noise power in 0.5 dB steps
 */
#define NSE_PWR_MIN     (-140)
#define NSE_PWR_MAX     (0)

/* 
 * Local Parameters Structure
 */
typedef struct {
    uint32 seed;    /* Seed value */
    uvint  blockSz; /* Block size */
} NSE_Params;

/*
 * Internal Object:
 */
typedef struct {
    char internal_ary[0x14];
} _NSE_Internal;

/*
 * NSE Object
 */
typedef struct {
    _NSE_Internal  internal;
    vint           pwr;
    vint          *dst_ptr;
} NSE_Obj;


/*
 * Function prototypes
 */
void NSE_init(
    NSE_Obj       *nseObj_ptr,  /* Pointer to NSE object struct */
    GLOBAL_Params *globals_ptr, /* Pointer to global parameters struct */
    NSE_Params    *locals_ptr); /* Pointer to NSE local parameters struct */

void NSE_generate(
    NSE_Obj *nseObj_ptr);       /* Pointer to NSE object struct */

#endif
