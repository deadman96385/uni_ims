/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2007 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * +D2Tech+ Rev: 3778  Date: 2009-12-14 15:29:39 -0800 (Mon, 14 Dec 2009) 
 * +D2Tech+ Release Version: alg_core3-uds
 */

#ifndef _UDS_H_
#define _UDS_H_

#include <comm.h>

typedef enum {
    UDS_SAMPLERATE_8K,
    UDS_SAMPLERATE_16K,
    UDS_SAMPLERATE_24K,
    UDS_SAMPLERATE_32K,
    UDS_SAMPLERATE_48K
} UDS_SampleRate;

/*
 * Internal Object:
 */
typedef struct {
    char internal_ary[0x98];
} _UDS_Internal;

/*
 * UDS object
 */
typedef struct {
    _UDS_Internal   internal;       /* internal object */
    vint           *src_ptr;        /* source pointer */
    vint           *dst_ptr;        /* destination pointer */
} UDS_Obj;

/*
 * Function Prototypes:
 */
void UDS_init(
    UDS_Obj        *udsObj_ptr,
    UDS_SampleRate  lowSampleRate,
    UDS_SampleRate  highSampleRate);

void UDS_upSample(
    UDS_Obj        *udsObj_ptr);

void UDS_downSample(
    UDS_Obj        *udsObj_ptr);

#endif /* _UDS_H_ */

