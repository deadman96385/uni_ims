/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2006-2007 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * +D2Tech+ Rev: 2115  Date: 2007-08-01 15:15:20 -0700 (Wed, 01 Aug 2007) 
 * +D2Tech+ Release Version: alg_core3-dcrm
 */

#ifndef _DCRM_
#define _DCRM_

#include "d2types.h"

/* 
 * Constants / Macros:
 */

/* 
 * Valid function return codes
 */
#define DCRM_OK  (0)  /* Function executed normally */
#define DCRM_ERR (-1) /* Function execution did not complete due to an error */

/* 
 * Define the min, max, and default values for the local parameters
 */
#define DCRM_BLOCKSZ_MIN (1)   /* Minimum block size in samples */
#define DCRM_BLOCKSZ_MAX (160) /* Maximum block size in samples */
#define DCRM_BLOCKSZ_DEF (80)  /* Typical block size in samples */

/*
 *  The following structure contains the local parameters used by DCRM:
 */
typedef struct {
    vint blockSz; /* Number of samples to process per call */
} DCRM_Params;

/*
 * Internal Object:
 */
typedef struct {
    char internal_ary[0xc];
} _DCRM_Internal;

/* 
 * DCRM object:
 * One object is allocated per port.
 */
typedef struct {
    _DCRM_Internal  internal; /* internal object */
    vint           *src_ptr;  /* source buffer pointer */
    vint           *dst_ptr;  /* destination buffer pointer */
} DCRM_Obj;

/* 
 * Function Prototypes:
 */
vint DCRM_init(
    DCRM_Obj    *dcrmObj_ptr, /* Pointer to DCRM object */
    DCRM_Params *locals_ptr); /* Pointer to DCRM local parameters struct */

vint DCRM_run(
    DCRM_Obj *dcrmObj_ptr); /* Pointer to DCRM object */

#endif /* _DCRM_ */
