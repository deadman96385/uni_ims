/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004-5 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 1985 $ $Date: 2007-02-15 23:50:34 -0500 (Thu, 15 Feb 2007) $
 *
 */

/*
 * VTSPR Command processing functions for Tone generation
 */
#include "vtspr.h"
#include "_vtspr_private.h"

/*
 * ======== _VTSPR_updateToneSeq ========
 *
 * Update internal tone params based on VTSP command for a tone sequence
 * or a single tone (not a sequence, no tone list).
 *
 * This function also handles GENF (quad) tone params, sequenced and
 * non-sequenced.
 */
VTSP_Return _VTSPR_updateToneSeq(
    VTSPR_DSP           *dsp_ptr,
    VTSPR_ToneSeq       *toneSeq_ptr,
    uvint               *toneList_ptr,
    vint                 tId,
    uint32               maxTime,
    vint                 numToneIds,
    uint32               control,
    uint32               repeat,    /* repeat for the sequence */
    uvint                toneType)
{
#ifndef VTSP_ENABLE_MP_LITE
    TONE_Params         *toneParam_ptr;
#endif
#ifdef VTSP_ENABLE_TONE_QUAD
    GENF_Params         *genfParam_ptr;
#endif
    vint                 index;

    if (NULL == toneList_ptr) { 
        /* Handle the non-sequence TONE or non-sequence GENF case */
        if ((0 == repeat) || (0 == maxTime)) { 
            /* Sometimes args purposefully 0; turn off tone */
            return (VTSP_E_ARG);
        }
        toneSeq_ptr->toneNumToneIds = 1;
        toneSeq_ptr->toneControl = 0;
        toneSeq_ptr->toneSeqRepeat = 1;
        toneSeq_ptr->toneTime = maxTime;
        toneSeq_ptr->toneSeqIndex = 0;
        toneSeq_ptr->toneSeqRepeatCnt = 0;
        toneSeq_ptr->tonePreRetVal = 0;
        toneSeq_ptr->toneRetVal = 0;
        toneSeq_ptr->toneEdge = VTSP_EVENT_INACTIVE;
        toneSeq_ptr->toneDone = VTSP_EVENT_ACTIVE;
        toneSeq_ptr->toneIdList[0] = tId;
        return (VTSP_OK);
    }

    if ((0 == repeat) || (0 == numToneIds)) { 
        /* Sometimes args purposefully 0; turn off tone */
        return (VTSP_E_ARG);
    }

    /* Handle the sequenced TONE or sequenced GENF case */
    for (index = (numToneIds - 1); index >= 0; index--) {
        /* All tone repeats set to one, not set by VTSP_configTone() */
        tId = *(toneList_ptr + index);
#ifndef VTSP_ENABLE_MP_LITE
        if (VTSPR_TONE_DUAL == toneType) { 
            toneParam_ptr = dsp_ptr->toneParams_ptr[tId];
            if (NULL == toneParam_ptr) {
                /* tone template is not configured, play nothing */
                toneSeq_ptr->toneIdList[0] = 0;
                toneSeq_ptr->toneNumToneIds = 0;
                return (VTSP_E_ARG);
            }
            /* Tone Param is always set Repeat=1 in sequence mode */
            toneParam_ptr->repeat = 1;
        }
#endif
#ifdef VTSP_ENABLE_TONE_QUAD
        else if (VTSPR_TONE_QUAD == toneType) { 
            genfParam_ptr = dsp_ptr->toneQuadParams_ptr[tId];
            if (NULL == genfParam_ptr) {
                /* tone template is not configured, play nothing */
                toneSeq_ptr->toneIdList[0] = 0;
                toneSeq_ptr->toneNumToneIds = 0;
                return (VTSP_E_ARG);
            }
            /* GENF Param is always set Repeat=1 in sequence mode */
            genfParam_ptr->sRepeat = 1;
        }
#endif
        toneSeq_ptr->toneIdList[index] = tId;
    }

    toneSeq_ptr->toneNumToneIds = numToneIds;
    toneSeq_ptr->toneControl = control;
    toneSeq_ptr->toneSeqRepeat = repeat;
    toneSeq_ptr->toneTime = maxTime;
    toneSeq_ptr->toneSeqIndex = 0;
    toneSeq_ptr->toneSeqRepeatCnt = 0;
    toneSeq_ptr->tonePreRetVal = 0;
    toneSeq_ptr->toneRetVal = 0;
    toneSeq_ptr->toneEdge = VTSP_EVENT_INACTIVE;
    toneSeq_ptr->toneDone = VTSP_EVENT_ACTIVE;

    return (VTSP_OK);
}

