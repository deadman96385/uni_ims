/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004-2010 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 12522 $ $Date: 2010-07-14 06:52:44 +0800 (Wed, 14 Jul 2010) $
 *
 */
#include "vtspr.h"
#include "_vtspr_private.h"

/*
 * ======== _VTSPR_genEventFxs() ========
 *
 * Get msg from pool, fill from event msg,
 * 
 */
void _VTSPR_genEventFxs(
    VTSPR_Obj    *vtspr_ptr,
    VTSPR_Queues *q_ptr,
    VTSPR_DSP    *dsp_ptr)
{
#ifdef VTSP_ENABLE_FXS
    VTSPR_ChanObj    *chan_ptr;
    VTSPR_FxsInfcObj *fxs_ptr;
#ifdef VTSP_ENABLE_CIDS
    _VTSPR_CidsObj   *cids_ptr;
#endif
#ifdef VTSP_ENABLE_ECSR
    _VTSPR_EcObj     *ec_ptr;
#endif      
#ifdef VTSP_ENABLE_PULSE_DETECT
    vint              ticPulse;
#endif
#ifdef VTSP_ENABLE_GR909
    uint32            ticGr909Status;
    uint32            ticGr909Mask;
    uint32            index;
#endif
    vint              ticBattery;
    vint              ticRingNum;
    vint              ticHook;
    vint              ticFlash;
    vint              infc;
    uint32            chanMask;
    vint              state;
    void             *tic_ptr;

    _VTSPR_FOR_ALL_FXS(infc) {        /* FXS interfaces */

        fxs_ptr  = _VTSPR_infcToInfcPtr(dsp_ptr, infc);
        tic_ptr  = _VTSPR_infcToTicPtr(dsp_ptr, infc);
        chan_ptr = _VTSPR_infcToChanPtr(dsp_ptr, infc);
        chanMask = _VTSPR_getAlgStateChan(chan_ptr, infc);
#ifdef VTSP_ENABLE_CIDS
        cids_ptr = chan_ptr->cids_ptr;
#endif
        /*
         * FXS battery polarity status
         */
        ticBattery = TIC_getStatus(tic_ptr, TIC_GET_STATUS_POLARITY);
        if (ticBattery != fxs_ptr->battery) {
            q_ptr->eventMsg.code = VTSP_EVENT_MSG_CODE_HOOK;
            q_ptr->eventMsg.infc = infc;
            q_ptr->eventMsg.tick = dsp_ptr->tick1ms;
            if (TIC_BATT_POL_FORWARD == ticBattery) { 
                q_ptr->eventMsg.msg.hook.reason = VTSP_EVENT_HOOK_POLARITY_FWD;
                q_ptr->eventMsg.msg.hook.edgeType = VTSP_EVENT_LEADING;
                VTSPR_sendEvent(q_ptr, &q_ptr->eventMsg, infc);
            }
            if (TIC_BATT_POL_REVERSE == ticBattery) { 
                q_ptr->eventMsg.msg.hook.reason = VTSP_EVENT_HOOK_POLARITY_REV;
                q_ptr->eventMsg.msg.hook.edgeType = VTSP_EVENT_LEADING;
                VTSPR_sendEvent(q_ptr, &q_ptr->eventMsg, infc);
            }
            fxs_ptr->battery = ticBattery;
        }

        /* FXS ring generation events
         */
        if (VTSP_EVENT_ACTIVE == fxs_ptr->ringEvent) {
            state       = TIC_getStatus(tic_ptr, TIC_GET_STATUS_LINE_STATE);
            ticRingNum  = TIC_getStatus(tic_ptr, TIC_GET_STATUS_RING_NUM);
            if (ticRingNum != fxs_ptr->ringNum) {
                /* change in ring # indicates LE/TE of ring
                 */

                if (ticRingNum > 0) { 
                    /*
                     * There is a ring trailing edge for previous ring.
                     */
                    q_ptr->eventMsg.code =
                            VTSP_EVENT_MSG_CODE_RING_GENERATE;
                    q_ptr->eventMsg.infc = infc;
                    q_ptr->eventMsg.tick = dsp_ptr->tick1ms;
                    q_ptr->eventMsg.msg.ringDetect.reason =
                            VTSP_EVENT_ACTIVE;
                    q_ptr->eventMsg.msg.ringDetect.ringNum = 
                            ticRingNum - 1;
                    q_ptr->eventMsg.msg.ringDetect.edgeType =
                            VTSP_EVENT_TRAILING;
                    VTSPR_sendEvent(q_ptr, &q_ptr->eventMsg, infc);
                }

                if (ticRingNum != fxs_ptr->ringCountMax) { 
                    /*
                     * This is a ring leading edge for new ring.
                     */
                    q_ptr->eventMsg.code = VTSP_EVENT_MSG_CODE_RING_GENERATE;
                    q_ptr->eventMsg.infc = infc;
                    q_ptr->eventMsg.tick = dsp_ptr->tick1ms;
                    q_ptr->eventMsg.msg.ringDetect.reason = VTSP_EVENT_ACTIVE;
                    q_ptr->eventMsg.msg.ringDetect.ringNum = ticRingNum;
                    q_ptr->eventMsg.msg.ringDetect.edgeType =
                            VTSP_EVENT_LEADING;
                    VTSPR_sendEvent(q_ptr, &q_ptr->eventMsg, infc);
                }
            }

            /* Check for ring stopped
             *
             * check for state change out of ring state for Halted
             * (infc seized or ringStop)
             * check ringTime exceeded
             * check ringCountMax exceeded
                 */
            if ((TIC_STATE_RINGING != state) && 
                    (TIC_STATE_RING_PAUSE != state) &&
                    ((TIC_STATE_RINGING == fxs_ptr->state) || 
                     (TIC_STATE_RING_PAUSE == fxs_ptr->state))) { 
                    /* old state was ring - so ring has halted
                     */
                fxs_ptr->ringEvent = VTSP_EVENT_HALTED;
            }

            /* Check for timeouts, which may override Halted event.
             */
            fxs_ptr->ringTime -= 10;
            if (fxs_ptr->ringTime <= 0) { 
                fxs_ptr->ringEvent = VTSP_EVENT_MAX_TIMEOUT;
            }
            if (ticRingNum == fxs_ptr->ringCountMax) { 
                fxs_ptr->ringEvent = VTSP_EVENT_MAX_REPEAT;
            }
        }

        /* If ring stopped, 
         *  - set ring controls
         *  - set caller id send control
         *  - send trailing event
         *
         *  the change in state from the ring control will generate the
         *  event.
         */
        if ((VTSP_EVENT_MAX_TIMEOUT == fxs_ptr->ringEvent) ||
                (VTSP_EVENT_MAX_REPEAT == fxs_ptr->ringEvent) ||
                (VTSP_EVENT_HALTED == fxs_ptr->ringEvent)) { 
            /*
             * # sec or # repeat exceeded, or line status change
             */
            q_ptr->eventMsg.code = VTSP_EVENT_MSG_CODE_RING_GENERATE;
            q_ptr->eventMsg.infc = infc;
            q_ptr->eventMsg.tick = dsp_ptr->tick1ms;
            q_ptr->eventMsg.msg.ringDetect.ringNum =
                    TIC_getStatus(tic_ptr, TIC_GET_STATUS_RING_NUM);
            q_ptr->eventMsg.msg.ringDetect.reason = fxs_ptr->ringEvent;
            q_ptr->eventMsg.msg.ringDetect.edgeType = VTSP_EVENT_TRAILING;
            VTSPR_sendEvent(q_ptr, &q_ptr->eventMsg, infc);

            fxs_ptr->ringEvent = VTSP_EVENT_INACTIVE;

            /* Stop ring
             */
            TIC_control(tic_ptr, TIC_CONTROL_STOP_RING, 0);
            TIC_control(tic_ptr, TIC_CONTROL_POLARITY_FWD , 0);
#ifdef VTSP_ENABLE_CIDS
            /* Stop CIDS
             */
            cids_ptr->cidsObj.ctrl |= CIDS_CTRL_TIMEOUT; 
#endif
        }

        /* After ring events handled, update state info.
         * Line state may have changed from start of event code above, so
         * update from TIC.
         */
        fxs_ptr->ringNum = TIC_getStatus(tic_ptr, TIC_GET_STATUS_RING_NUM);
        fxs_ptr->state   = TIC_getStatus(tic_ptr, TIC_GET_STATUS_LINE_STATE);

        /* FXS hook status
         *
         * Audio channel control is also handled below
         * Caller-ID Send state is also handled below
         */
        ticHook = TIC_getStatus(tic_ptr, TIC_GET_STATUS_HOOK);
        if (ticHook != fxs_ptr->hookEvent) {
            q_ptr->eventMsg.code = VTSP_EVENT_MSG_CODE_HOOK;
            q_ptr->eventMsg.infc = infc;
            q_ptr->eventMsg.tick = dsp_ptr->tick1ms;
            q_ptr->eventMsg.msg.hook.edgeType = VTSP_EVENT_LEADING;
            if (TIC_SEIZE == ticHook) {
                /* 
                 * if seize status is 0 then disable CIDS. We do not want this
                 * for japanese caller id as it internally goes to seize state.
                 */
#ifdef VTSP_ENABLE_CIDS
                if (0 == (cids_ptr->cidsObj.stat & CIDS_STAT_SEIZE_OFF))  {
#endif
                    q_ptr->eventMsg.msg.hook.reason = VTSP_EVENT_HOOK_SEIZE;
                    VTSPR_sendEvent(q_ptr, &q_ptr->eventMsg, infc);
                    fxs_ptr->hookEvent = ticHook;

                    if (0 == (VTSPR_ALG_CHAN_T38 & chanMask)) {
                        /*
                         * If channel has not already been set for T38,
                         * enable EC, NLP, NFE and warm-init.
                         * Also enable DTMF, FMTD, DCRM automatically.
                         * Disable CIDS.
                         *
                         * For T38, channel may be set to T38 before
                         * the Fax seizes.
                         */
                        _VTSPR_algStateChan(dsp_ptr, infc,
                                VTSPR_ALG_CHAN_CIDS,
                                VTSPR_ALG_CHAN_DCRM_PEER |
                                VTSPR_ALG_CHAN_NFE_INFC  | 
                                VTSPR_ALG_CHAN_ECSR      | 
                                VTSPR_ALG_CHAN_NLP       | 
                                VTSPR_ALG_CHAN_DTMF      |
                                VTSPR_ALG_CHAN_FMTD      | 
                                VTSPR_ALG_CHAN_NFE_PEER);
                    }
#ifdef VTSP_ENABLE_CIDS
                }
#endif
            }
            else if (TIC_RELEASE == ticHook) { 
                /* 
                 * if release status is 0 then dont change algs. We do not
                 * want this for japanese caller id as it internally goes 
                 * back to release state.
                 */
#ifdef VTSP_ENABLE_CIDS
                if (0 == (cids_ptr->cidsObj.stat & CIDS_STAT_REL_OFF))  {
#endif
                    q_ptr->eventMsg.msg.hook.reason = VTSP_EVENT_HOOK_RELEASE;
                    VTSPR_sendEvent(q_ptr, &q_ptr->eventMsg, infc);
                    fxs_ptr->hookEvent = ticHook;
                    /*
                     * Disable EC, NLP, NFE, DTMF, FMTD, DCRM.
                     */
                    _VTSPR_algStateChan(dsp_ptr, infc, 
                            VTSPR_ALG_CHAN_DCRM_PEER |
                            VTSPR_ALG_CHAN_NFE_INFC | VTSPR_ALG_CHAN_ECSR | 
                            VTSPR_ALG_CHAN_NLP | VTSPR_ALG_CHAN_DTMF |
                            VTSPR_ALG_CHAN_FMTD | VTSPR_ALG_CHAN_NFE_PEER |
                            VTSPR_ALG_CHAN_FAX | VTSPR_ALG_CHAN_T38, 
                            0); 
#ifdef VTSP_ENABLE_CIDS
                }
#endif
            }
            else { 
                /* "unknown" or "other" hook state 
                 * - dont send event 
                 * - dont update hookEvent
                 */
            }
        }

        /* FXS flash status */
        ticFlash = TIC_getStatus(tic_ptr, TIC_GET_STATUS_HOOK_FLASH);
        if (ticFlash != fxs_ptr->flashEvent) {
            q_ptr->eventMsg.code = VTSP_EVENT_MSG_CODE_HOOK;
            q_ptr->eventMsg.infc = infc;
            q_ptr->eventMsg.tick = dsp_ptr->tick1ms;
            if (TIC_FLASH == ticFlash) { 
                q_ptr->eventMsg.msg.hook.reason = VTSP_EVENT_HOOK_FLASH;
                q_ptr->eventMsg.msg.hook.edgeType = VTSP_EVENT_LEADING;
                VTSPR_sendEvent(q_ptr, &q_ptr->eventMsg, infc);
            }
        }
        fxs_ptr->flashEvent = ticFlash;

#ifdef VTSP_ENABLE_GR909
        for (index=0; index < TIC_GR909_ALL; index++) {
            switch (index) {
                case 0:
                    q_ptr->eventMsg.msg.gr909.reason = VTSP_EVENT_TYPE_GR909_HEMF; 
                    ticGr909Status = TIC_getStatus(tic_ptr, TIC_GET_STATUS_GR909_HEMF);
                    break;
                case 1:
                    q_ptr->eventMsg.msg.gr909.reason = VTSP_EVENT_TYPE_GR909_FEMF; 
                    ticGr909Status = TIC_getStatus(tic_ptr, TIC_GET_STATUS_GR909_FEMF);
                    break;
                case 2:
                    q_ptr->eventMsg.msg.gr909.reason = VTSP_EVENT_TYPE_GR909_RFAULT; 
                    ticGr909Status = TIC_getStatus(tic_ptr, TIC_GET_STATUS_GR909_RFAULT);
                    break;
                case 3:
                    q_ptr->eventMsg.msg.gr909.reason = VTSP_EVENT_TYPE_GR909_ROFFHOOK; 
                    ticGr909Status = TIC_getStatus(tic_ptr, TIC_GET_STATUS_GR909_ROFFHOOK);
                    break;
                case 4:
                    q_ptr->eventMsg.msg.gr909.reason = VTSP_EVENT_TYPE_GR909_REN; 
                    ticGr909Status = TIC_getStatus(tic_ptr, TIC_GET_STATUS_GR909_REN);
                    break;
            }
            /*
             * ticGr909Status:
             *               bit 1: indicates if we've done the GR909 diagnostic
             *               bit 0: indicates the GR909 test result
             * If bit 1 is zero, that means we don't run the GR909 diagnostic in that GR909 item,
             * then search next GR909 item.
             */
            if (0 == (ticGr909Status & 0x2)) {
                continue;
            }
            q_ptr->eventMsg.code = VTSP_EVENT_MSG_CODE_GR909;
            q_ptr->eventMsg.infc = infc;
            q_ptr->eventMsg.tick = dsp_ptr->tick1ms;
            q_ptr->eventMsg.msg.gr909.status = ticGr909Status & 0x01;
            VTSPR_sendEvent(q_ptr, &q_ptr->eventMsg, infc);
        }
#endif
        /* FXS pulse detection */
#ifdef VTSP_ENABLE_PULSE_DETECT
        ticPulse = TIC_getStatus(tic_ptr, TIC_GET_STATUS_PULSE_DIGIT);
        if (ticPulse != fxs_ptr->pulseEvent) { 
            /* Change in status */
            fxs_ptr->pulseEvent = ticPulse;
            if (0 != (ticPulse & (1 << TIC_PDD_READY_BIT))) { 
                /* Create event msg with the digit */
                q_ptr->eventMsg.code = VTSP_EVENT_MSG_CODE_HOOK;
                q_ptr->eventMsg.infc = infc;
                q_ptr->eventMsg.tick = dsp_ptr->tick1ms;
                q_ptr->eventMsg.msg.hook.reason = VTSP_EVENT_HOOK_PULSE;
                q_ptr->eventMsg.msg.hook.edgeType = VTSP_EVENT_TRAILING;
                /* Convert the tic pulse number to be the ASCII format */
                q_ptr->eventMsg.msg.hook.digit = 
                        (ticPulse & TIC_PDD_DIGIT_MASK) | 0x30;
                VTSPR_sendEvent(q_ptr, &q_ptr->eventMsg, infc);
            }
#ifdef VTSP_ENABLE_PULSE_DETECT_EARLY 
            /* 
             * XXX WARNING: 
             * can not report early, because it occurs too frequently 
             */
            else if (0 != (ticPulse & (1 << TIC_PDD_EARLY_BIT))) { 
                /* Report start of pulse detect as early detection */
                q_ptr->eventMsg.code = VTSP_EVENT_MSG_CODE_HOOK;
                q_ptr->eventMsg.infc = infc;
                q_ptr->eventMsg.tick = dsp_ptr->tick1ms;
                q_ptr->eventMsg.msg.hook.reason = VTSP_EVENT_HOOK_PULSE;
                q_ptr->eventMsg.msg.hook.edgeType = VTSP_EVENT_LEADING;
                q_ptr->eventMsg.msg.hook.digit = ~(0);
                VTSPR_sendEvent(q_ptr, &q_ptr->eventMsg, infc);
            }
#endif
        }
#endif
    }
   /* 
     * FXS Infc events
     */
    _VTSPR_FOR_ALL_FXS(infc) {
        chan_ptr    = _VTSPR_infcToChanPtr(dsp_ptr, infc);
#ifdef VTSP_ENABLE_CIDS
        /* CallerID Send Events
         */
        cids_ptr = chan_ptr->cids_ptr;
        if (VTSP_EVENT_HALTED == cids_ptr->cidsEvent) {
            cids_ptr->cidsEvent  = VTSP_EVENT_INACTIVE;
            q_ptr->eventMsg.code = VTSP_EVENT_MSG_CODE_CID_GENERATE;
            q_ptr->eventMsg.infc = infc;
            q_ptr->eventMsg.tick = dsp_ptr->tick1ms;
            q_ptr->eventMsg.msg.cidGenerate.reason = VTSP_EVENT_HALTED;
            q_ptr->eventMsg.msg.cidGenerate.type = cids_ptr->cidType;
            VTSPR_sendEvent(q_ptr, &q_ptr->eventMsg, infc);
        }
        else if (VTSP_EVENT_ACTIVE == cids_ptr->cidsEvent) {
            cids_ptr->cidsEvent  = VTSP_EVENT_INACTIVE;
            q_ptr->eventMsg.code = VTSP_EVENT_MSG_CODE_CID_GENERATE;
            q_ptr->eventMsg.infc = infc;
            q_ptr->eventMsg.tick = dsp_ptr->tick1ms;
            q_ptr->eventMsg.msg.cidGenerate.reason = VTSP_EVENT_ACTIVE;
            q_ptr->eventMsg.msg.cidGenerate.type   = cids_ptr->cidType;
            VTSPR_sendEvent(q_ptr, &q_ptr->eventMsg, infc);
        }
        else if (VTSP_EVENT_COMPLETE == cids_ptr->cidsEvent) {
            cids_ptr->cidsEvent  = VTSP_EVENT_INACTIVE;
            q_ptr->eventMsg.code = VTSP_EVENT_MSG_CODE_CID_GENERATE;
            q_ptr->eventMsg.infc = infc;
            q_ptr->eventMsg.tick = dsp_ptr->tick1ms;
            q_ptr->eventMsg.msg.cidGenerate.reason = VTSP_EVENT_COMPLETE;
            q_ptr->eventMsg.msg.cidGenerate.type   = cids_ptr->cidType;
            VTSPR_sendEvent(q_ptr, &q_ptr->eventMsg, infc);
        }
        else if ((VTSP_EVENT_CID_PRIMARY_TIMEOUT == cids_ptr->cidsEvent)
                || (VTSP_EVENT_CID_INCOMING_TIMEOUT == cids_ptr->cidsEvent) ) {
            cids_ptr->cidsEvent  = VTSP_EVENT_INACTIVE;
            q_ptr->eventMsg.code = VTSP_EVENT_MSG_CODE_CID_GENERATE;
            q_ptr->eventMsg.infc = infc;
            q_ptr->eventMsg.tick = dsp_ptr->tick1ms;
            q_ptr->eventMsg.msg.cidGenerate.reason = cids_ptr->cidsEvent;
            q_ptr->eventMsg.msg.cidGenerate.type   = cids_ptr->cidType;
            VTSPR_sendEvent(q_ptr, &q_ptr->eventMsg, infc);
        }
#endif
        /* Echo canceller events */
        if ((0 != chan_ptr->echoCancEvent) && 
                (chan_ptr->echoCancEvent != chan_ptr->echoCancEventLast)) {
            q_ptr->eventMsg.code = VTSP_EVENT_MSG_CODE_EC;
            q_ptr->eventMsg.infc = infc;
            q_ptr->eventMsg.tick = dsp_ptr->tick1ms;
            q_ptr->eventMsg.msg.echoCanc.reason = chan_ptr->echoCancEvent;
#ifdef VTSP_ENABLE_ECSR            
            ec_ptr = chan_ptr->ec_ptr;
            q_ptr->eventMsg.msg.echoCanc.status = ec_ptr->ecsrObj.status;
            q_ptr->eventMsg.msg.echoCanc.rerl = ec_ptr->ecsrObj.rerl;
#endif
            VTSPR_sendEvent(q_ptr, &q_ptr->eventMsg, infc);
            chan_ptr->echoCancEventLast = chan_ptr->echoCancEvent;
        }
    }
#endif
}



