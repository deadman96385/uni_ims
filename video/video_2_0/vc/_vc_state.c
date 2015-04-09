/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004-2010 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 12522 $ $Date: 2010-07-13 18:52:44 -0400 (Tue, 13 Jul 2010) $
 *
 */

#include "_vc_private.h"

/*
 * ======== _VC_algStateStream ========
 * Set/Reset algorithm state for stream processing
 * and initialize algorithms as necessary
 */
void _VC_algStateStream(
    _VC_Dsp   *dsp_ptr,
    uvint        streamId,
    uint32       clearMask,
    uint32       setMask)
{
    _VC_StreamObj  *stream_ptr;
    uint32            oldState;
    uint32            newState;

    stream_ptr = _VC_streamIdToStreamPtr(dsp_ptr, streamId);

    oldState = stream_ptr->algStreamState;
    newState = oldState | setMask;
    newState = newState & (~clearMask);

    if (0 != (setMask & _VC_ALG_STREAM_JB)) {
        JBV_init(&stream_ptr->dec.jbObj);
    }

    if (0 != (clearMask & _VC_ALG_STREAM_JB)) {
        _VC_TRACE(__FILE__, __LINE__);
        JBV_init(&stream_ptr->dec.jbObj);
    }

    stream_ptr->algStreamState = newState;
}
