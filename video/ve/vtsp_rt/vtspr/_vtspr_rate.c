/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2006-2007 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 30131 $ $Date: 2014-12-01 14:45:03 +0800 (Mon, 01 Dec 2014) $
 *
 */
#include "vtspr.h"
#include "_vtspr_private.h"

#if defined(VTSP_ENABLE_STREAM_16K) || defined(VTSP_ENABLE_AUDIO_SRC)
#ifndef VTSP_ENABLE_MP_LITE
/*
 * ======== _VTSPR_upSample() ========
 *
 * Up sample audio buffer of 80 vints @ 8kHz to 160 vints @ 16kHz
 *
 */
void _VTSPR_upSample(
        UDS_Obj *udsObj_ptr,
        vint    *dst_ptr,
        vint    *src_ptr)
{
    udsObj_ptr->dst_ptr = dst_ptr;
    udsObj_ptr->src_ptr = src_ptr;

    UDS_upSample(udsObj_ptr);
}

/*
 * ======== _VTSPR_downSample() ========
 *
 * Down sample audio buffer of 160 vints @ 16kHz to 80 vints @ 8kHz
 *
 */
void _VTSPR_downSample(
        UDS_Obj *udsObj_ptr,
        vint    *dst_ptr,
        vint    *src_ptr)
{
    udsObj_ptr->dst_ptr = dst_ptr;
    udsObj_ptr->src_ptr = src_ptr;

    UDS_downSample(udsObj_ptr);
}
#endif
#endif
