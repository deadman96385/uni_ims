/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 25287 $ $Date: 2014-03-24 16:37:42 +0800 (Mon, 24 Mar 2014) $
 */

#ifndef _SIP_CLIB_H_
#define _SIP_CLIB_H_

#include "sip_types.h"

/* 
 * Define below if time or stdlib libraries are not available:
 * #define SIP_USE_CLIB_RAND_GEN
 */

/* Macro definitions */
#define SIP_MAX(a,b)        ((a)>(b)?(a):(b))
#define SIP_MIN(a,b)        ((a)>(b)?(b):(a))
#define SIP_CLIB_ULONG_MAX  (0xFFFFFFFF)

void SIP_initCLib(uint32 seed);

void SIP_destroyCLib(void);

uint32 SIP_randInt(uint32 l, uint32 h);

#endif
