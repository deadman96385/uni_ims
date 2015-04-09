/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004-2007 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 28394 $ $Date: 2014-08-21 10:24:05 +0800 (Thu, 21 Aug 2014) $
 *
 */

#include "vtspr.h"
#include "_vtspr_private.h"

/*
 * ======== _VTSPR_callerIdSend() ========
 *
 * Caller ID Send (Type 1 and Type 2) on all interfaces
 *
 * Call after TIC_run()
 * 
 */
void _VTSPR_callerIdSend(
    VTSPR_Obj    *vtspr_ptr,
    VTSPR_Queues *q_ptr,
    VTSPR_DSP    *dsp_ptr)
{
#ifdef VTSP_ENABLE_CIDS
    VTSPR_FxsInfcObj *fxs_ptr;
    VTSPR_ChanObj    *chan_ptr;
    _VTSPR_CidsObj   *cids_ptr;
    TIC_Obj          *tic_ptr;
#ifdef VTSP_ENABLE_TONE_QUAD
    VTSPR_ToneSeq    *toneSeq_ptr;
    GENF_Obj         *genf_ptr;
    GLOBAL_Params    *globals_ptr;
    vint              tId;
#endif
    vint              infc;
    uint32            mask;
    vint              cidStat;
    TIC_RingParams    ringParams;    
    vint              cidRet;
#ifdef VTSP_ENABLE_BENCHMARK
     _VTSPR_benchmarkStart(vtspr_ptr, _VTSPR_BENCHMARK_CIDS, 1);
#endif
    /* 
     * Physical Interface Caller ID Send
     * --------
     */
    _VTSPR_FOR_ALL_FXS(infc) {
        tic_ptr  = _VTSPR_infcToTicPtr(dsp_ptr, infc);
        chan_ptr = _VTSPR_infcToChanPtr(dsp_ptr, infc);
        cids_ptr = chan_ptr->cids_ptr;
        mask     = chan_ptr->algChannelState;

        if (0 != (mask & VTSPR_ALG_CHAN_CIDS)) { 
            /* CallerID-Send active
             */
            if (TIC_IMMEDIATE_OFFHOOK == 
                    TIC_getStatus(tic_ptr, TIC_GET_STATUS_HOOK_IMMEDIATE)) {
                cids_ptr->cidsObj.ctrl |= CIDS_CTRL_OFFHOOK; 
                cids_ptr->cidsObj.ctrl &= ~CIDS_CTRL_ONHOOK; 
            }
            else {
                cids_ptr->cidsObj.ctrl &= ~CIDS_CTRL_OFFHOOK; 
                cids_ptr->cidsObj.ctrl |= CIDS_CTRL_ONHOOK; 
            }

            cidRet = CIDS_run(&cids_ptr->cidsObj);
            cidStat = cids_ptr->cidsObj.stat;
            if (CIDS_STAT_CHANGED & cidStat) {
                /* Status changed after run,
                 * use status as telephony control
                 */
                if (CIDS_STAT_XMIT & cidStat) {
                    cids_ptr->cidsEvent = VTSP_EVENT_ACTIVE;
                }

                if (CIDS_STAT_BATTREV & cidStat) {
                    TIC_control(tic_ptr, TIC_CONTROL_POLARITY_REV, 0);
                }

                if (CIDS_STAT_RING & cidStat) { 
                    fxs_ptr  = &dsp_ptr->fxsInfc[infc];
                    fxs_ptr->ringEvent = VTSP_EVENT_ACTIVE;
                    TIC_setRingTable(tic_ptr, 
                            &fxs_ptr->ringTemplate_ptr->ticParam);
                }

                if (CIDS_STAT_RING_SHORT & cidStat) { 
                    /*
                     * Used only for Japanese Caller ID short ring.
                     */
                    ringParams.numCads = 1;
                    ringParams.make1   = 50;  /* 500 ms ring */
                    ringParams.break1  = 50;  /* 500 ms silence */
                    ringParams.make2   = 0;
                    ringParams.break2  = 0;
                    ringParams.make3   = 0;
                    ringParams.break3  = 0;
                    ringParams.repeat  = 5; 
                    TIC_setRingTable(tic_ptr, &ringParams);
                }
                
            }
            if (CIDS_DONE == cidRet) {
                /* 
                 * CIDS Done - Set 'normal' battery after transmission
                 */
                TIC_control(tic_ptr, TIC_CONTROL_POLARITY_FWD, 0);
                _VTSPR_algStateChan(dsp_ptr, infc, VTSPR_ALG_CHAN_CIDS, 0); 

                if (CIDS_STAT_ABORT & cidStat) {
                    cids_ptr->cidsEvent = VTSP_EVENT_HALTED;
                }
                else {
                    cids_ptr->cidsEvent = VTSP_EVENT_COMPLETE;
                }
            }
        }

        if (0 != (mask & VTSPR_ALG_CHAN_CIDCWS)) { 
            /*
             * DTMF digit detection is set elsewhere for calling CIDCWS
             */
            if (CIDCWS_DONE == CIDCWS_run(&cids_ptr->cidcwsObj)) { 
                cids_ptr->cidsEvent = VTSP_EVENT_COMPLETE;
                _VTSPR_algStateChan(dsp_ptr, infc, VTSPR_ALG_CHAN_CIDCWS, 0); 
            }
            else if (0 != (cids_ptr->cidcwsObj.ctrl & CIDCWS_CTRL_TONE_INIT)) {
#ifdef VTSP_ENABLE_TONE_QUAD
                toneSeq_ptr = &chan_ptr->toneQuadSeq;
                cids_ptr->cidcwsObj.ctrl &= ~(CIDCWS_CTRL_TONE_INIT);
                globals_ptr = dsp_ptr->globals_ptr;
                toneSeq_ptr->toneSeqIndex++;
                tId = toneSeq_ptr->toneIdList[toneSeq_ptr->toneSeqIndex];
                genf_ptr = toneSeq_ptr->toneObj_ptr;
                genf_ptr->dst_ptr = chan_ptr->toneOut_ary;
                GENF_init(genf_ptr, globals_ptr,
                        dsp_ptr->toneQuadParams_ptr[tId], 0);
#endif
            }
        }
    }
#ifdef VTSP_ENABLE_BENCHMARK
     _VTSPR_benchmarkStop(vtspr_ptr, _VTSPR_BENCHMARK_CIDS, 1);
#endif    
#endif /* VTSP_ENABLE_CIDS */
}

