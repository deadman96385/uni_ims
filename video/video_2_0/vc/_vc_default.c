/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004-2010 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 12637 $ $Date: 2010-07-28 20:25:38 -0400 (Wed, 28 Jul 2010) $
 *
 */

/*
 * This file is the default initialization of algortihms
 * and objects for the vtspr 
 */
#include "_vc_private.h"
 
/*
 * ======== _VC_defaults() ========
 *
 * Set default parameters for DSP object.
 */
void _VC_defaults(
    _VC_Dsp *dsp_ptr)
{
    _VC_StreamObj  *stream_ptr;
    vint            streamId;

    dsp_ptr->curActiveInfcs = 0;
    dsp_ptr->curActiveStreams = 0;

    /*
     * Init streams.
     */
    for (streamId = _VTSP_STREAM_PER_INFC - 1; streamId >= 0; streamId--) {
        stream_ptr = _VC_streamIdToStreamPtr(dsp_ptr, streamId);
        stream_ptr->enc.encRunning = 0;
        stream_ptr->dec.decRunning = 0;
    }
}

