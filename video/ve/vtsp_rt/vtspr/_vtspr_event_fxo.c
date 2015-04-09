/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004-2008 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 9111 $ $Date: 2009-03-12 08:59:56 +0800 (Thu, 12 Mar 2009) $
 *
 */
#include "vtspr.h"
#include "_vtspr_private.h"

/*
 * ======== _VTSPR_genEventFxo() ========
 *
 * Get msg from pool, fill from event msg,
 * 
 */
void _VTSPR_genEventFxo(
    VTSPR_Obj    *vtspr_ptr,
    VTSPR_Queues *q_ptr,
    VTSPR_DSP    *dsp_ptr)
{
#ifdef VTSP_ENABLE_FXO
    VTSPR_ChanObj    *chan_ptr;
#ifdef VTSP_ENABLE_UTD
    _VTSPR_UtdObj    *utd_ptr;
#endif
#ifdef VTSP_ENABLE_ECSR
    _VTSPR_EcObj     *ec_ptr;
#endif    
#ifdef VTSP_ENABLE_FXO
    VTSPR_FxoInfcObj *fxo_ptr;
#ifdef VTSP_ENABLE_CIDR    
    _VTSPR_CidrObj   *cidr_ptr;
#endif
#ifdef VTSP_ENABLE_PULSE_GENERATE
    vint              ticPulseEvent;
#endif
    vint              ticRingEdge;
    vint              battPol;
    vint              ticDiscon;
#endif
    vint              ticRingNum;
    vint              ticHook;
    vint              ticFlash;
    vint              infc;
    vint              state;
    void             *tic_ptr;

    /* 
     * Physical Interface events
     */
    _VTSPR_FOR_ALL_FXO(infc) { /* FXO Interfaces */
        fxo_ptr     = _VTSPR_infcToInfcPtr(dsp_ptr, infc);
        tic_ptr     = _VTSPR_infcToTicPtr(dsp_ptr, infc);
        chan_ptr    = _VTSPR_infcToChanPtr(dsp_ptr, infc);

        state       = TIC_getStatus(tic_ptr, TIC_GET_STATUS_LINE_STATE);
        ticRingEdge = TIC_getStatus(tic_ptr, TIC_GET_STATUS_RING_EDGE);
        ticRingNum  = TIC_getStatus(tic_ptr, TIC_GET_STATUS_RING_NUM);

        if ((fxo_ptr->state != state) || 
                (fxo_ptr->ringEdge != ticRingEdge)) {
            q_ptr->eventMsg.code = VTSP_EVENT_MSG_CODE_RING_DETECT;
            q_ptr->eventMsg.infc = infc;
            q_ptr->eventMsg.tick = dsp_ptr->tick1ms;
            if (TIC_STATE_POWER_DOWN == state) {
                /*
                 * Power down from PSTN
                 */
                q_ptr->eventMsg.msg.ringDetect.reason
                        = VTSP_EVENT_HOOK_POWER_DOWN;
                VTSPR_sendEvent(q_ptr, &q_ptr->eventMsg, infc);
            }
            else if (TIC_STATE_RINGING == state) {
                q_ptr->eventMsg.msg.ringDetect.reason = VTSP_EVENT_ACTIVE;
                q_ptr->eventMsg.msg.ringDetect.ringNum = ticRingNum;
                if (TIC_RING_MAKE == ticRingEdge) {
                    q_ptr->eventMsg.msg.ringDetect.edgeType =
                        VTSP_EVENT_RING_MAKE;
                }
                else {
                    q_ptr->eventMsg.msg.ringDetect.edgeType =
                        VTSP_EVENT_RING_BREAK;
                }
                VTSPR_sendEvent(q_ptr, &q_ptr->eventMsg, infc);
            }
            else if (TIC_STATE_ACTIVE == state) {
                /*
                 * Normal active state.
                 * report the last number of rings
                 */
                if (fxo_ptr->state != 0) { /* skip the first initial state after system boot up */
                    q_ptr->eventMsg.msg.ringDetect.reason = VTSP_EVENT_INACTIVE;
                    q_ptr->eventMsg.msg.ringDetect.ringNum = fxo_ptr->ringNum;
                    q_ptr->eventMsg.msg.ringDetect.edgeType = VTSP_EVENT_RING_BREAK;
                    VTSPR_sendEvent(q_ptr, &q_ptr->eventMsg, infc);
                }
            }
            fxo_ptr->state = state;
            fxo_ptr->ringEdge = ticRingEdge;
            fxo_ptr->ringNum = ticRingNum;
        }

        
        /* FXO hook status
         *
         * Audio channel control is also handled below
         * Caller-ID Send state is also handled below
         */
        ticHook = TIC_getStatus(tic_ptr, TIC_GET_STATUS_HOOK);
        if (ticHook != fxo_ptr->hookEvent) {
            q_ptr->eventMsg.code = VTSP_EVENT_MSG_CODE_HOOK;
            q_ptr->eventMsg.infc = infc;
            q_ptr->eventMsg.tick = dsp_ptr->tick1ms;
            q_ptr->eventMsg.msg.hook.edgeType = VTSP_EVENT_LEADING;
            if (TIC_SEIZE == ticHook) {
                    q_ptr->eventMsg.msg.hook.reason = VTSP_EVENT_HOOK_SEIZE;
                    VTSPR_sendEvent(q_ptr, &q_ptr->eventMsg, infc);
                    fxo_ptr->hookEvent = ticHook;

                    /*
                     * Enable EC, NLP, NFE and warm-init.
                     * Also enable DCRM automatically.
                     * Disable CIDS.
                     * Disable FSKR which runs always during ONHOOK.
                     */
                    _VTSPR_algStateChan(dsp_ptr, infc,
                            VTSPR_ALG_CHAN_CAS |
                            VTSPR_ALG_CHAN_FSKR_ONH |
                            VTSPR_ALG_CHAN_FSKR_OFH |
                            VTSPR_ALG_CHAN_DTMF,
                            VTSPR_ALG_CHAN_DCRM_PEER |
                            VTSPR_ALG_CHAN_NFE_INFC | VTSPR_ALG_CHAN_ECSR | 
                            VTSPR_ALG_CHAN_NLP | VTSPR_ALG_CHAN_NFE_PEER |
                            VTSPR_ALG_CHAN_UTD);
            }
            else if (TIC_RELEASE == ticHook) { 
                    q_ptr->eventMsg.msg.hook.reason = VTSP_EVENT_HOOK_RELEASE;
                    VTSPR_sendEvent(q_ptr, &q_ptr->eventMsg, infc);
                    fxo_ptr->hookEvent = ticHook;
                    /*
                     * Disable EC, NLP, NFE, DTMF, FMTD, DCRM, CAS.
                     * Enable FSKR for onhook CID receive.
                     */
                    _VTSPR_algStateChan(dsp_ptr, infc, 
                            VTSPR_ALG_CHAN_CAS |
                            VTSPR_ALG_CHAN_FSKR_ONH |
                            VTSPR_ALG_CHAN_FSKR_OFH |
                            VTSPR_ALG_CHAN_DTMF |
                            VTSPR_ALG_CHAN_DCRM_PEER |
                            VTSPR_ALG_CHAN_NFE_INFC | VTSPR_ALG_CHAN_ECSR | 
                            VTSPR_ALG_CHAN_NLP | VTSPR_ALG_CHAN_NFE_PEER |
                            VTSPR_ALG_CHAN_UTD,
                            0); 
            }
        }

        /* FXO flash status */
        ticFlash = TIC_getStatus(tic_ptr, TIC_GET_STATUS_HOOK_FLASH);
        if (ticFlash != fxo_ptr->flashEvent) {
            q_ptr->eventMsg.code = VTSP_EVENT_MSG_CODE_HOOK;
            q_ptr->eventMsg.infc = infc;
            q_ptr->eventMsg.tick = dsp_ptr->tick1ms;
            q_ptr->eventMsg.msg.hook.reason = VTSP_EVENT_HOOK_FLASH;
            if (TIC_FLASH == ticFlash) { 
                q_ptr->eventMsg.msg.hook.edgeType = VTSP_EVENT_LEADING;
                VTSPR_sendEvent(q_ptr, &q_ptr->eventMsg, infc);
            }
            else {
                if (fxo_ptr->flashEvent != 0) { /* skip the first initial state after system boot up */
                    q_ptr->eventMsg.msg.hook.edgeType = VTSP_EVENT_TRAILING;
                    VTSPR_sendEvent(q_ptr, &q_ptr->eventMsg, infc);
                }
            }
            fxo_ptr->flashEvent = ticFlash;
        }

        /*
         * FXO battery polarity status
         */
        battPol = TIC_getStatus(tic_ptr, TIC_GET_STATUS_POLARITY);
        if (battPol != fxo_ptr->battPol) {
            q_ptr->eventMsg.code = VTSP_EVENT_MSG_CODE_HOOK;
            q_ptr->eventMsg.infc = infc;
            q_ptr->eventMsg.tick = dsp_ptr->tick1ms;
            if (TIC_BATT_POL_FORWARD == battPol) { 
                q_ptr->eventMsg.msg.hook.reason = VTSP_EVENT_HOOK_POLARITY_REV;
                q_ptr->eventMsg.msg.hook.edgeType = VTSP_EVENT_LEADING;
                VTSPR_sendEvent(q_ptr, &q_ptr->eventMsg, infc);
            }
            if (TIC_BATT_POL_REVERSE == battPol) { 
                q_ptr->eventMsg.msg.hook.reason = VTSP_EVENT_HOOK_POLARITY_FWD;
                q_ptr->eventMsg.msg.hook.edgeType = VTSP_EVENT_LEADING;
                VTSPR_sendEvent(q_ptr, &q_ptr->eventMsg, infc);
            }
            fxo_ptr->battPol = battPol;
        }

        /*
         * FXO PSTN disconnect event
         */
        ticDiscon = TIC_getStatus(tic_ptr, TIC_GET_STATUS_DISCONNECT);
        if (ticDiscon != fxo_ptr->disconEvent) {
            /*
             * Only send PSTN disconnect event when the state changes
             * to disconnect
             */      
            if (TIC_DISCONNECT_DISC == ticDiscon)
            {
                q_ptr->eventMsg.code = VTSP_EVENT_MSG_CODE_HOOK;
                q_ptr->eventMsg.infc = infc;
                q_ptr->eventMsg.msg.hook.reason = VTSP_EVENT_HOOK_DISCONNECT;
                q_ptr->eventMsg.msg.hook.edgeType = VTSP_EVENT_LEADING;
                q_ptr->eventMsg.tick = dsp_ptr->tick1ms;
                VTSPR_sendEvent(q_ptr, &q_ptr->eventMsg, infc);
            }
            fxo_ptr->disconEvent = ticDiscon;
        }

#ifdef VTSP_ENABLE_PULSE_GENERATE
        ticPulseEvent = TIC_getStatus(tic_ptr, TIC_GET_STATUS_PULSE_GENERATE_EVENT);
        if (ticPulseEvent != fxo_ptr->pulseEvent) {
            if (fxo_ptr->pulseEvent != 0) { /* skip the first initial state after system boot up */
                q_ptr->eventMsg.code = VTSP_EVENT_MSG_CODE_HOOK;
                q_ptr->eventMsg.infc = infc;
                q_ptr->eventMsg.msg.hook.reason = VTSP_EVENT_HOOK_PULSE;
                if (TIC_PULSE_GENERATE_ACTIVE == ticPulseEvent) {
                    q_ptr->eventMsg.msg.hook.edgeType = VTSP_EVENT_ACTIVE;
                }
                else {
                    q_ptr->eventMsg.msg.hook.edgeType = VTSP_EVENT_COMPLETE;
                }
                q_ptr->eventMsg.tick = dsp_ptr->tick1ms;
                VTSPR_sendEvent(q_ptr, &q_ptr->eventMsg, infc);
            }
            fxo_ptr->pulseEvent = ticPulseEvent;
        }
#endif
    }

    _VTSPR_FOR_ALL_FXO(infc) {
        chan_ptr = _VTSPR_infcToChanPtr(dsp_ptr, infc);
#ifdef VTSP_ENABLE_UTD    
        /*
         * UTD detect
         */
        utd_ptr = chan_ptr->utd_ptr;
        if (0 != utd_ptr->utdLe) {
            q_ptr->eventMsg.code = VTSP_EVENT_MSG_CODE_TONE_DETECT;
            q_ptr->eventMsg.infc = infc;
            q_ptr->eventMsg.tick = dsp_ptr->tick1ms;
            q_ptr->eventMsg.msg.toneDetect.reason   = VTSP_EVENT_ACTIVE;
            q_ptr->eventMsg.msg.toneDetect.detect   = VTSP_DETECT_UTD;
            q_ptr->eventMsg.msg.toneDetect.edgeType = VTSP_EVENT_LEADING;
            q_ptr->eventMsg.msg.toneDetect.tone     = utd_ptr->utdLe & 0xFF;
            VTSPR_sendEvent(q_ptr, &q_ptr->eventMsg, infc);
        }
                
        if (0 != utd_ptr->utdTe) {
            q_ptr->eventMsg.code = VTSP_EVENT_MSG_CODE_TONE_DETECT;
            q_ptr->eventMsg.infc = infc;
            q_ptr->eventMsg.tick = dsp_ptr->tick1ms;
            q_ptr->eventMsg.msg.toneDetect.reason   = VTSP_EVENT_ACTIVE;
            q_ptr->eventMsg.msg.toneDetect.detect   = VTSP_DETECT_UTD;
            q_ptr->eventMsg.msg.toneDetect.edgeType = VTSP_EVENT_TRAILING;
            q_ptr->eventMsg.msg.toneDetect.tone     = utd_ptr->utdTe & 0xFF;
            VTSPR_sendEvent(q_ptr, &q_ptr->eventMsg, infc);
        }
#endif

#ifdef VTSP_ENABLE_CIDR
        /*
         * DMTF CID detect
         */
        cidr_ptr = chan_ptr->cidr_ptr;
        if (0 != cidr_ptr->dtmfCidEvt) {
            q_ptr->eventMsg.code = VTSP_EVENT_MSG_CODE_CID_DETECT;
            q_ptr->eventMsg.infc = infc;
            q_ptr->eventMsg.tick = dsp_ptr->tick1ms;
            q_ptr->eventMsg.msg.cidDetect.reason = VTSP_EVENT_COMPLETE;
            q_ptr->eventMsg.msg.cidDetect.type   = VTSP_EVENT_TYPE_CID_DTMF;
            cidr_ptr->dtmfCidEvt = 0;
            VTSPR_sendEvent(q_ptr, &q_ptr->eventMsg, infc);
            OSAL_msgQSend(q_ptr->cidQ[infc], cidr_ptr->dtmfData_ary,
                    sizeof(_VTSP_CIDData), OSAL_NO_WAIT, NULL);
        }
        /*
         * FSKR CID detect
         */
        if ((0 != cidr_ptr->fskrValid) && (0 == cidr_ptr->cid2Detect)) {
            q_ptr->eventMsg.code = VTSP_EVENT_MSG_CODE_CID_DETECT;
            q_ptr->eventMsg.infc = infc;
            q_ptr->eventMsg.tick = dsp_ptr->tick1ms;
            q_ptr->eventMsg.msg.cidDetect.reason = VTSP_EVENT_COMPLETE;
            q_ptr->eventMsg.msg.cidDetect.type = VTSP_EVENT_TYPE_CID_ONHOOK;
            VTSPR_sendEvent(q_ptr, &q_ptr->eventMsg, infc);
            cidr_ptr->fskrValid = 0;
            OSAL_msgQSend(q_ptr->cidQ[infc], cidr_ptr->fskrData_ary,
                    sizeof(_VTSP_CIDData), OSAL_NO_WAIT, NULL);
        }
        else if (0 != cidr_ptr->cid2Detect) {
            q_ptr->eventMsg.code = VTSP_EVENT_MSG_CODE_CID_DETECT;
            q_ptr->eventMsg.infc = infc;
            q_ptr->eventMsg.tick = dsp_ptr->tick1ms;
            q_ptr->eventMsg.msg.cidDetect.reason = VTSP_EVENT_COMPLETE;
            q_ptr->eventMsg.msg.cidDetect.type = VTSP_EVENT_TYPE_CID_OFFHOOK;
            VTSPR_sendEvent(q_ptr, &q_ptr->eventMsg, infc);
            cidr_ptr->cid2Detect = 0;
            cidr_ptr->fskrValid = 0;
            OSAL_msgQSend(q_ptr->cidQ[infc], cidr_ptr->fskrData_ary,
                    sizeof(_VTSP_CIDData), OSAL_NO_WAIT, NULL);
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
            q_ptr->eventMsg.msg.echoCanc.rerl   = ec_ptr->ecsrObj.rerl;
#endif
            chan_ptr->echoCancEventLast         = chan_ptr->echoCancEvent;
            VTSPR_sendEvent(q_ptr, &q_ptr->eventMsg, infc);
        }
    }
#endif
}
