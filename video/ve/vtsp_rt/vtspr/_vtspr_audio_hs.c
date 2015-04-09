/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004-2008 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 30131 $ $Date: 2014-12-01 14:45:03 +0800 (Mon, 01 Dec 2014) $
 *
 */
#include "vtspr.h"
#include "_vtspr_private.h"

#ifndef VTSP_ENABLE_MP_LITE
vint _AEC_state_old = 10000;
 
/*
 * ======== _VTSPR_audioRxFormatHs() ========
 * Format audio received from the handset hardware driver.
 */
void _VTSPR_audioRxFormatHs(
    VTSPR_Obj    *vtspr_ptr,
    VTSPR_DSP    *dsp_ptr)
{
#ifdef VTSP_ENABLE_AUDIO_SRC
    VTSPR_ChanObj *chan_ptr;
    vint           infc;
    /*
     * If audio inteface is using sample rate higher than the stream rate
     * do SRC to stream rate here
     */ 
    _VTSPR_FOR_ALL_AUDIO(infc) {
        chan_ptr = _VTSPR_infcToChanPtr(dsp_ptr, infc);

        if (VTSPR_NSAMPLES_10MS_MAX > VTSPR_NSAMPLES_STREAM) {
            _VTSPR_downSample(&chan_ptr->udsAudioDown, chan_ptr->audioIn_ary, 
                    chan_ptr->rx_ptr);
        }
    }
#endif
}

/*
 * ======== _VTSPR_audioTxFormatHs() ========
 * Format audio received from the handset hardware driver.
 */
void _VTSPR_audioTxFormatHs(
    VTSPR_Obj    *vtspr_ptr,
    VTSPR_DSP    *dsp_ptr)
{
#ifdef VTSP_ENABLE_AUDIO_SRC
    VTSPR_ChanObj *chan_ptr;
    vint           infc;

    /*
     * If audio inteface is using sample rate higher than the stream rate
     * do SRC to stream rate here
     */ 
    _VTSPR_FOR_ALL_AUDIO(infc) { 
        chan_ptr = _VTSPR_infcToChanPtr(dsp_ptr, infc);

        if (VTSPR_NSAMPLES_10MS_MAX > VTSPR_NSAMPLES_STREAM) {
            _VTSPR_upSample(&chan_ptr->udsAudioUp, chan_ptr->tx_ptr, 
                    chan_ptr->audioOut_ary);
        }
    }
#endif
}
 
/*
 * ======== _VTSPR_audioRoutHs() ========
 * Process Rout (Rin) for AEC to align canceller buffers and run 
 * AEC_computeRout().
 */
void _VTSPR_audioRoutHs(
    VTSPR_DSP    *dsp_ptr)
{
#ifdef VTSP_ENABLE_AEC
    VTSPR_ChanObj *chan_ptr;
    _VTSPR_AecObj *aec_ptr;
    AEC_Obj       *aecObj_ptr;
    vint          *rin_ptr; 
    vint          *rout_ptr;
    vint           infc;
    uvint          mask;

    _VTSPR_FOR_ALL_AUDIO(infc) { 
        /* 
         * Format audio
         */
        chan_ptr = _VTSPR_infcToChanPtr(dsp_ptr, infc);
        aec_ptr  = chan_ptr->aec_ptr;
        mask     = chan_ptr->algChannelState;
        /*
         * Assign the current Rin and Rout pointers, which point to the newest
         * voice sample buffers.
         */ 
        rin_ptr  = aec_ptr->aecRin_ary[(aec_ptr->nAllignBufs - 1)];
        rout_ptr = aec_ptr->aecRout_ary[(aec_ptr->nAllignBufs - 1)];
        /*
         * Age the Rin and Rout buffers
         * Also, copy the new audioOut to newest Rin
         */
        COMM_copy(aec_ptr->aecRin_ary[0], aec_ptr->aecRin_ary[1],
                ((aec_ptr->nAllignBufs - 1) * chan_ptr->numSamples10ms));
        COMM_copy(rin_ptr, chan_ptr->audioOut_ary, chan_ptr->numSamples10ms);
        COMM_copy(aec_ptr->aecRout_ary[0], aec_ptr->aecRout_ary[1],
                ((aec_ptr->nAllignBufs - 1) * chan_ptr->numSamples10ms));
        /*
         * Run AEC for Rout
         */
        aecObj_ptr           = &aec_ptr->aecNearObj;
        aecObj_ptr->rin_ptr  = rin_ptr;
        aecObj_ptr->rout_ptr = rout_ptr;

#ifdef VTSP_ENABLE_BENCHMARK
        _VTSPR_benchmarkStart(&VTSPR_obj,
                _VTSPR_BENCHMARK_AEC_COMPUTE_ROUT, 0);
#endif
        AEC_computeRout(aecObj_ptr);
#ifdef VTSP_ENABLE_BENCHMARK
        _VTSPR_benchmarkStop(&VTSPR_obj,
                _VTSPR_BENCHMARK_AEC_COMPUTE_ROUT, 0);
#endif

        /* Copy Rout to audioOut */
        COMM_copy(chan_ptr->audioOut_ary, rout_ptr, chan_ptr->numSamples10ms);
    }
#endif /* VTSP_ENABLE_AEC */
}

/*
 * ======== _VTSPR_removeEchoHs() ========
 * Remove acoustic echo (for the benefit of network) for audio devices.
 */
void _VTSPR_audioRemoveEchoHs(
    VTSPR_Obj *vtspr_ptr,
    VTSPR_DSP *dsp_ptr)
{
    VTSPR_ChanObj *chan_ptr;
#ifdef VTSP_ENABLE_AEC
    _VTSPR_AecObj *aec_ptr;
    AEC_Obj       *aecObj_ptr;
#endif
    vint           infc;
    uint32         mask;

    /*
     * Do for all AUDIO handset physical interfaces.
     */
    _VTSPR_FOR_ALL_AUDIO(infc) {
        /* 
         * Format audio
         */
        chan_ptr = _VTSPR_infcToChanPtr(dsp_ptr, infc);
        mask     = chan_ptr->algChannelState;

#ifndef VTSP_ENABLE_AEC
        mask &= (~VTSPR_ALG_CHAN_AEC); /* turn off the if statement below */
        /* Measure for voice activity on the near end */
        BND_run(chan_ptr->bndNear_ptr);
#endif
        /*
         * Set appropriate pointers.
         */

#ifdef VTSP_ENABLE_AEC
        /*
         * Run AEC for Sout
         * point rin_ptr and rout_ptr to the oldest time aligned buffers
         */
        aec_ptr              = chan_ptr->aec_ptr;
        aecObj_ptr           = &aec_ptr->aecNearObj;
        aecObj_ptr->rin_ptr  = aec_ptr->aecRin_ary[0];  /* input */
        aecObj_ptr->rout_ptr = aec_ptr->aecRout_ary[0]; /* input */

#ifdef VTSP_ENABLE_BENCHMARK
        _VTSPR_benchmarkStart(&VTSPR_obj,
                _VTSPR_BENCHMARK_AEC_COMPUTE_SOUT, 0);
#endif
        AEC_computeSout(aecObj_ptr);
#ifdef VTSP_ENABLE_BENCHMARK
        _VTSPR_benchmarkStop(&VTSPR_obj,
                _VTSPR_BENCHMARK_AEC_COMPUTE_SOUT, 0);
#endif
 
#ifdef _VTSPR_AEC_STATE_DEBUG
        if (_AEC_state_old != aecObj_ptr->state) {
            if (aecObj_ptr->state > 2) {
                printf("\nAEC state=%d\n", aecObj_ptr->state);
            }
            _AEC_state_old = aecObj_ptr->state;
        }
#endif
        if (((AEC_STATE_FAR_ACTIVE == aecObj_ptr->state) || 
                (AEC_STATE_DBL_TALK == aecObj_ptr->state)) && 
                (aecObj_ptr->control & AEC_CONTROL_AEC_HALF_DUPLEX)) {
            COMM_fill(aecObj_ptr->sin_ptr, 0, chan_ptr->numSamples10ms);
        }
            
        _VTSPR_netlogSend(vtspr_ptr, 0 + 10*infc, aecObj_ptr->rin_ptr,
                chan_ptr->numSamples10ms * sizeof(vint), infc);
        _VTSPR_netlogSend(vtspr_ptr, 1 + 10*infc, aecObj_ptr->sin_ptr,
                chan_ptr->numSamples10ms * sizeof(vint), infc);
        _VTSPR_netlogSend(vtspr_ptr, 2 + 10*infc, aecObj_ptr->sout_ptr,
                chan_ptr->numSamples10ms * sizeof(vint), infc);
#else
        /*
         * Bypass AEC
         * Copy audioIn to ecNlp
         */
        COMM_copy(chan_ptr->ecNlp_ary, chan_ptr->audioIn_ary,
                chan_ptr->numSamples10ms);
#endif

    }
}
#endif
