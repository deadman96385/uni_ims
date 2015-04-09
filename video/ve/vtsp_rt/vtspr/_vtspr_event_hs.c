/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004-2007 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 30131 $ $Date: 2014-12-01 14:45:03 +0800 (Mon, 01 Dec 2014) $
 *
 */
#include "vtspr.h"
#include "_vtspr_private.h"

/*
 * ======== _VTSPR_genEventHs() ========
 *
 * Get msg from pool, fill from event msg,
 * 
 */
void _VTSPR_genEventHs(
    VTSPR_Obj    *vtspr_ptr,
    VTSPR_Queues *q_ptr,
    VTSPR_DSP    *dsp_ptr)
{
    VTSPR_ChanObj       *chan_ptr;
    VTSPR_AudioInfcObj  *audio_ptr;
    vint                 ticHook;
    vint                 ticFlash;
    vint                 ticSleep;
    vint                 infc;
    void                *tic_ptr;
#ifndef VTSP_ENABLE_MP_LITE
    VTSPR_ToneSeq       *tone_ptr;
#endif
#ifdef VTSP_ENABLE_AEC
    _VTSPR_AecObj       *aec_ptr;
#endif    

    /*
     * Channel Events
     */
    _VTSPR_FOR_ALL_AUDIO(infc) {  /* All Handset infc */
        audio_ptr = _VTSPR_infcToInfcPtr(dsp_ptr, infc);
        tic_ptr   = _VTSPR_infcToTicPtr(dsp_ptr, infc);
        chan_ptr  = _VTSPR_infcToChanPtr(dsp_ptr, infc);

        /* 
         * Collect a key action from the DECT keypad
         */
         if (TIC_KEYPAD_KEY_FLAG == TIC_getStatus(tic_ptr, 
                         TIC_GET_STATUS_KEYPAD_EVENT)) {
            /*
             * Report first (or only) Key Pressed event
             */
            q_ptr->eventMsg.code = VTSP_EVENT_MSG_CODE_KEY_DETECT;
            q_ptr->eventMsg.infc = infc;
            q_ptr->eventMsg.tick = dsp_ptr->tick1ms;
            q_ptr->eventMsg.msg.keyDetect.key = 
                TIC_getStatus(tic_ptr, TIC_GET_STATUS_KEYPAD_VALUE);
            q_ptr->eventMsg.handset = 
                TIC_getStatus(tic_ptr, TIC_GET_STATUS_HANDSET_ID);

            VTSPR_sendEvent(q_ptr, &q_ptr->eventMsg, infc);
         }

        /* AUDIO hook status
         *
         * Audio channel control is also handled below
         * Caller-ID Send state is also handled below
         */
        ticHook = TIC_getStatus(tic_ptr, TIC_GET_STATUS_HOOK);
        if (ticHook != audio_ptr->hookEvent) {
            q_ptr->eventMsg.code = VTSP_EVENT_MSG_CODE_HOOK;
            q_ptr->eventMsg.infc = infc;
            q_ptr->eventMsg.tick = dsp_ptr->tick1ms;
            q_ptr->eventMsg.handset = TIC_getStatus(tic_ptr, 
                TIC_GET_STATUS_HANDSET_ID);
            q_ptr->eventMsg.msg.hook.edgeType = VTSP_EVENT_LEADING;
            if (TIC_SEIZE == ticHook) {
                q_ptr->eventMsg.msg.hook.reason = VTSP_EVENT_HOOK_SEIZE;
                VTSPR_sendEvent(q_ptr, &q_ptr->eventMsg, infc);
                audio_ptr->hookEvent = ticHook;
                /*
                 * enable NFE and warm-init.
                 * Also enable DCRM automatically.
                 */
                _VTSPR_algStateChan(dsp_ptr, infc, 0,
                        VTSPR_ALG_CHAN_DCRM_PEER |
                        VTSPR_ALG_CHAN_NFE_INFC  | 
                        VTSPR_ALG_CHAN_NFE_PEER);
            }
            else if (TIC_RELEASE == ticHook) { 
                /* 
                 * if release status is 0 then dont change algs. We do not
                 * want this for japanese caller id as it internally goes 
                 * back to release state.
                 */
                q_ptr->eventMsg.msg.hook.reason = VTSP_EVENT_HOOK_RELEASE;
                VTSPR_sendEvent(q_ptr, &q_ptr->eventMsg, infc);
                audio_ptr->hookEvent = ticHook;
                /*
                 * Disable NFE & DCRM.
                 */
                _VTSPR_algStateChan(dsp_ptr, infc, VTSPR_ALG_CHAN_DCRM_PEER | 
                        VTSPR_ALG_CHAN_NFE_INFC | VTSPR_ALG_CHAN_NFE_PEER, 0); 
            }
            else { 
                /* "unknown" or "other" hook state 
                 * - dont send event 
                 * - dont update hookEvent
                 */
            }
        }

        /* AUDIO flash status */
        ticFlash = TIC_getStatus(tic_ptr, TIC_GET_STATUS_HOOK_FLASH);
        if (ticFlash != audio_ptr->flashEvent) {
            q_ptr->eventMsg.code = VTSP_EVENT_MSG_CODE_HOOK;
            q_ptr->eventMsg.infc = infc;
            q_ptr->eventMsg.tick = dsp_ptr->tick1ms;
            q_ptr->eventMsg.handset = 
                TIC_getStatus(tic_ptr, TIC_GET_STATUS_HANDSET_ID);
            if (TIC_FLASH == ticFlash) { 
                q_ptr->eventMsg.msg.hook.reason = VTSP_EVENT_HOOK_FLASH;
                q_ptr->eventMsg.msg.hook.edgeType = VTSP_EVENT_LEADING;
                VTSPR_sendEvent(q_ptr, &q_ptr->eventMsg, infc);
            }
        }
        audio_ptr->flashEvent = ticFlash;

        /*
         * AUDIO Driver attach/detach state
         */
        ticSleep = TIC_getStatus(tic_ptr, TIC_GET_STATUS_DRIVER_ATTACH);
        if (ticSleep != audio_ptr->attachState) {
            q_ptr->eventMsg.code = VTSP_EVENT_MSG_CODE_HW;
            q_ptr->eventMsg.infc = infc;
            q_ptr->eventMsg.tick = dsp_ptr->tick1ms;
            if (TIC_DRIVER_ATTACHED == ticSleep) { 
                q_ptr->eventMsg.msg.hw.reason = 
                        VTSP_EVENT_HW_AUDIO_ATTACH;
            }
            else {
                q_ptr->eventMsg.msg.hw.reason = 
                        VTSP_EVENT_HW_AUDIO_DETACH;
            }
            VTSPR_sendEvent(q_ptr, &q_ptr->eventMsg, infc);
        }
        audio_ptr->attachState = ticSleep;

#ifndef VTSP_ENABLE_MP_LITE
        /* Generate Tone Events */
        tone_ptr    = &chan_ptr->toneSeq;
        _VTSPR_genEventTone(dsp_ptr, q_ptr, infc, NULL, tone_ptr);
#endif
#ifdef VTSP_ENABLE_TONE_QUAD
        tone_ptr    = &chan_ptr->toneQuadSeq;
        _VTSPR_genEventTone(dsp_ptr, q_ptr, infc, NULL, tone_ptr);
#endif

#ifdef VTSP_ENABLE_AEC
        /* XXX Generate AEC events XXX TBD */
        aec_ptr = chan_ptr->aec_ptr;        
#endif
    }
}

