/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2003-2007 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * +D2Tech+ Rev: 2115  Date: 2007-08-01 15:15:20 -0700 (Wed, 01 Aug 2007) 
 * +D2Tech+ Release Version: alg_core3-g726
 *
 *
 *  This is ITU's G.726 ADPCM encoder/decoder.
 *
 *  The format of input buffer of the encoder object can be selected 
 *  from linear, mu-law or A-law.
 *  The output buffer of the encoder object is packed ADPCM codewords, the
 *  size is returned by G726_encode().
 *
 */

#ifndef _G726_H_
#define _G726_H_

#include <comm.h>

/* G726 modes */
#define G726_LINEAR  (0)
#define G726_ULAW    (256)
#define G726_ALAW    (512)
/* Data rates */
#define G726_16KBPS  (0)
#define G726_24KBPS  (1)
#define G726_32KBPS  (2)
#define G726_40KBPS  (3)

/*
 *  
 *  ========= G726 OBJECT ========
 */
/*
 * Internal Object:
 */
typedef struct {
    char internal_ary[0xe0];
} _G726_Internal;

typedef struct {
    _G726_Internal  internal;
    vint           *src_ptr;
    vint           *dst_ptr;
} G726_Obj;

/*
 * Function Prototypes
 */

void G726_init(
    G726_Obj *g726Obj_ptr,  /* pointer to G726_Obj */
    vint      mode);        /* data rate and data type */

vint  G726_encode(
    G726_Obj *g726Obj_ptr); /* pointer to G726_Obj */

vint  G726_decode(
    G726_Obj *g726Obj_ptr); /* pointer to G726_Obj */

/*
 * Additional functions for testing only
 */
void G726_init_encode(
    vint input,
    G726_Obj *g726Obj_ptr);

void G726_init_decode(
    vint input,
    G726_Obj *g726Obj_ptr);

#endif

