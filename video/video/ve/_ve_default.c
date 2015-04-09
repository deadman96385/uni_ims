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
#include "_ve_private.h"
 
/*
 * ======== _VE_defaults() ========
 *
 * Set default parameters for DSP object.
 */
void _VE_defaults(
    _VE_Dsp *dsp_ptr)
{
    _VE_StreamObj  *stream_ptr;
    vint            streamId;      /* must be signed */
    vint            infc;          /* must be signed */

    
    dsp_ptr->curActiveInfcs = 0;
    dsp_ptr->curActiveStreams = 0;
    dsp_ptr->jbParams.decayRt = 64881;
    dsp_ptr->jbParams.minJitter = 0;
    dsp_ptr->jbParams.accmRate = 10;

    /*
     * All streams.
     */
    for (infc = VTSP_INFC_VIDEO; infc == VTSP_INFC_VIDEO; infc++) {

        /*
         * Init streams.
         */
        for (streamId = _VTSP_STREAM_PER_INFC - 1; streamId >= 0; streamId--) { 

            stream_ptr = _VE_streamIdToStreamPtr(dsp_ptr, infc, streamId);

            stream_ptr->enc.lastEncoder = VTSP_CODER_UNAVAIL;
            stream_ptr->dec.decoderType = VTSP_CODER_UNAVAIL;

        }
    }
}

