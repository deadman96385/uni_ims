/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004-2009 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 1985 $ $Date: 2007-02-15 23:50:34 -0500 (Thu, 15 Feb 2007) $
 *
 */

/*
 * Tone generation for both near end and far end (remote network peer)
 */
#include "vtspr.h"
#include "_vtspr_private.h"

/*
 * ======== _VTSPR_audioToneSeq() ========
 *
 * Generate tone audio sequence to near end or far end.
 * Handles both dual TONE and GENF tone.
 */
void _VTSPR_audioToneSeq(
    VTSPR_DSP        *dsp_ptr,
    vint              infc,
    VTSPR_StreamObj  *stream_ptr,
    VTSPR_ToneSeq    *tone_ptr,
    uvint             toneType)
{
#ifdef VTSP_ENABLE_TONE_QUAD    
    GENF_Obj      *toneQuad_ptr;
#endif
    uvint          streamId;
    uvint          mask;
    uvint          onFlag;
    uvint          offFlag;
    uvint          eventFlag;
    uvint          event;

    if (NULL != stream_ptr) { 
        streamId = stream_ptr->streamParam.streamId;
    }
    else { 
        streamId = 0;  /* avoid warning */
    }

    tone_ptr->toneTime     -= (VTSPR_NSAMPLES_10MS_8K >> 3);
    tone_ptr->tonePreRetVal = tone_ptr->toneRetVal;
    tone_ptr->toneDone      = VTSP_EVENT_ACTIVE;

    if (VTSPR_TONE_QUAD == toneType) {
#ifdef VTSP_ENABLE_TONE_QUAD         
        /* Run GENF alg on the object */
        tone_ptr->toneRetVal = GENF_tone(tone_ptr->toneObj_ptr);
        toneQuad_ptr = (GENF_Obj *)tone_ptr->toneObj_ptr;
        /* As default state, assume: in tone break */
        tone_ptr->toneEdge = VTSP_EVENT_TRAILING;   
        if (GENF_TONE == toneQuad_ptr->mode) { 
            tone_ptr->toneEdge = VTSP_EVENT_LEADING;    /* in tone make */
        }
        if (GENF_DONE == tone_ptr->toneRetVal) { 
            tone_ptr->toneDone = VTSP_EVENT_COMPLETE;
            tone_ptr->toneEdge = VTSP_EVENT_TRAILING;   
        }
#endif
    }
    else {
#ifndef VTSP_ENABLE_MP_LITE
        /* Run dual TONE alg on the object */
        tone_ptr->toneRetVal = TONE_generate(tone_ptr->toneObj_ptr);
        /* Set toneEdge for VTSP_TONE_BREAK_MIX */
        if ((TONE_STATE_BRK1 | TONE_STATE_BRK2 |
                TONE_STATE_BRK3) & tone_ptr->tonePreRetVal) {
            tone_ptr->toneEdge = VTSP_EVENT_TRAILING;   /* in tone break */
        }
        else {
            tone_ptr->toneEdge = VTSP_EVENT_LEADING;    /* in tone make */
        }
        if (TONE_DONE == tone_ptr->toneRetVal) { 
            tone_ptr->toneDone = VTSP_EVENT_COMPLETE;
            tone_ptr->toneEdge = VTSP_EVENT_TRAILING;   /* tone done break */
        }
#endif
    }

    eventFlag = 0;
    onFlag    = 0;
    offFlag   = 0;
    event     = 0;

    if (tone_ptr->toneTime < 1) { /* XXX was <= 0; sign problem? */
        /* tone is completed by timeout */
        offFlag = 1;
        eventFlag = 1;
        event = VTSP_EVENT_MAX_TIMEOUT;
    }
    else if (VTSP_EVENT_COMPLETE == tone_ptr->toneDone) {
        /* Check for next tone sequence */
        tone_ptr->toneSeqIndex++;
        if (tone_ptr->toneSeqIndex >= tone_ptr->toneNumToneIds) {
            /* tone sequence completed, check for Seq repeat */
            tone_ptr->toneSeqRepeatCnt++;
            if (tone_ptr->toneSeqRepeatCnt >= tone_ptr->toneSeqRepeat) {
                /* tone sequences complete */
                offFlag = 1;
                eventFlag = 1;
                event = VTSP_EVENT_COMPLETE;
            }
            else {
                /* re-init to repeat tone sequence */
                tone_ptr->toneSeqIndex = 0;
                onFlag = 1;
                /* For re-init, do not report HALTED */
                eventFlag = 1;
                event = VTSP_EVENT_INACTIVE;
            }
        }
        else {
            /* re-init with next the tone Id */
            onFlag = 1;
            /* For re-init, do not report HALTED */
            eventFlag = 1;
            event = VTSP_EVENT_INACTIVE;
        }
    }

    /* 
     * Handle on/off state transitions */
    if (NULL == stream_ptr) { 
        /* Channel tone */
        if (VTSPR_TONE_QUAD == toneType) { 
            mask = VTSPR_ALG_CHAN_TONE_QUAD;
        }
        else {
            mask = VTSPR_ALG_CHAN_TONE;
        }
        if (onFlag) { 
            _VTSPR_algStateChan(dsp_ptr, infc, 0, mask); 
        }
        else if (offFlag) { 
            _VTSPR_algStateChan(dsp_ptr, infc, mask, 0); 
        }
    }
    else {
        /* Stream tone */
        if (VTSPR_TONE_QUAD == toneType) { 
            mask = VTSPR_ALG_STREAM_TONE_QUAD;
        }
        else {
            mask = VTSPR_ALG_STREAM_TONE;
        }
        if (onFlag) { 
            _VTSPR_algStateStream(dsp_ptr, infc, streamId, 0, mask); 
        }
        else if (offFlag) { 
            _VTSPR_algStateStream(dsp_ptr, infc, streamId, mask, 0);
        }
    }

    /* Events are handled after state transition to force certain events */
    if (eventFlag) {
        tone_ptr->toneEvent = event;
    }

    return;
}

/*
 * ======== _VTSPR_audioNearTone() ========
 *
 * Generate tone audio to near end
 */
void _VTSPR_audioNearTone(
    VTSPR_DSP     *dsp_ptr,
    vint           infc,
    VTSPR_ChanObj *chan_ptr)
{
#ifndef VTSP_ENABLE_MP_LITE
    VTSPR_ToneSeq *tone_ptr;
#endif
    uint32         mask;

    mask = _VTSPR_getAlgStateChan(chan_ptr, infc);
    /*
     * Generate dual/single/quad tone if requested.
     * GENF/Quad takes priority over TONE.
     */
    if (0 != (VTSPR_ALG_CHAN_TONE_QUAD & mask)) {
#ifdef VTSP_ENABLE_TONE_QUAD         
        tone_ptr = &chan_ptr->toneQuadSeq;
        _VTSPR_audioToneSeq(dsp_ptr, infc, NULL /* stream_ptr */,
                tone_ptr, VTSPR_TONE_QUAD);
#endif
    }
#ifndef VTSP_ENABLE_MP_LITE
    else if (0 != (VTSPR_ALG_CHAN_TONE & mask)) {
        tone_ptr = &chan_ptr->toneSeq;
        _VTSPR_audioToneSeq(dsp_ptr, infc, NULL /* stream_ptr */,
                tone_ptr, VTSPR_TONE_DUAL);
    }
#endif

    return;
}

/*
 * ======== _VTSPR_audioPeerTone() ========
 *
 * Generate tone audio to peer stream.
 *
 */
void _VTSPR_audioPeerTone(
    VTSPR_DSP        *dsp_ptr,
    vint              infc,
    VTSPR_StreamObj  *stream_ptr)
{
#ifndef VTSP_ENABLE_MP_LITE
    VTSPR_ToneSeq *tone_ptr;
#endif
    uint32         streamMask;

    streamMask = stream_ptr->algStreamState;

    /*
     * Generate dual/single/quad tone if requested.
     * GENF/Quad takes priority over TONE.
     */
    if (0 != (VTSPR_ALG_STREAM_TONE_QUAD & streamMask)) {
#ifdef VTSP_ENABLE_TONE_QUAD         
        tone_ptr = &stream_ptr->toneQuadSeq;
        _VTSPR_audioToneSeq(dsp_ptr, infc, stream_ptr /* stream_ptr */,
                tone_ptr, VTSPR_TONE_QUAD);
#endif        
    }
#ifndef VTSP_ENABLE_MP_LITE
    else if (0 != (VTSPR_ALG_STREAM_TONE & streamMask)) {
        tone_ptr = &stream_ptr->toneSeq;
        _VTSPR_audioToneSeq(dsp_ptr, infc, stream_ptr /* stream_ptr */,
                tone_ptr, VTSPR_TONE_DUAL);
    }
#endif

    return;
}

