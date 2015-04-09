/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004-2009 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 30131 $ $Date: 2014-12-01 14:45:03 +0800 (Mon, 01 Dec 2014) $
 *
 */
#include "vtspr.h"
#include "_vtspr_private.h"

#ifndef VTSP_ENABLE_MP_LITE

#ifdef VTSP_ENABLE_MEASURE_IMPULSE
/* Test code for measuring impulse response of the echo return from Tx to Rx */
uvint _impulseTime  = 0;
uvint _impulseNum  = 0;
uvint _impulseSent  = 0;
vint  _impulseLevel = 8000;
vint  _delayFound   = 0;
uvint _diffOld      = 0;
vint  _sameCount    = 0;
#endif

#ifdef VTSP_ENABLE_MEASURE_LEVELS
vint _rms = 0;
vint _runningCnt = 0;
#endif

/*
 * ======== _VTSPR_audioGain() ========
 *
 * This function implements software gain control in Q15 math.
 *
 * XXX This function will only work when (vint == int32).
 */
void _VTSPR_audioGain(
    vint       *audio_ptr,
    int32       gain,
    vint        nsamp)
{
    vint            cnt;
    int32           sample;
    register vint *gain_ptr;

    gain_ptr = audio_ptr;

    for (cnt = nsamp - 1; cnt >= 0; cnt--) {
        sample = gain * (*gain_ptr);
        sample += 16384;
        sample = (sample >> 15);
        if (32767 < sample) {
            sample = 32767;
        }
        else if (-32768 > sample) {
            sample = -32768;
        }
        *(gain_ptr++) = sample;
    }
}

/*
 * ======== _VTSPR_audioMeasure() ========
 *
 * Measure linear RMS of the audio buffer and return dBr measurement.
 *
 * The value returned is dBr * 10.  The value needs dividing by 10 for dBr.
 *
 */
vint _VTSPR_audioMeasure(
    vint         *audio_ptr,
    vint          numSamples10ms)
{
    vint  dbr;
    vint  lrms;

    lrms = COMM_lrms(audio_ptr, numSamples10ms);
    /* 4004 lin=>720 */
    dbr  = COMM_lin2db(lrms) - COMM_lin2db(_VTSPR_globals.p0DBIN);

    return (dbr);
}

/*
 * ======== _VTSPR_measureLevels() ========
 *
 * Measure linear RMS of the audio buffer and return dBr measurement.
 *
 * The value returned is dBr * 10.  The value needs dividing by 10 for dBr.
 *
 */
void _VTSPR_measureLevels(
        vint *audioIn_ary,
        vint  numSamples10ms)
{
#ifdef VTSP_ENABLE_MEASURE_LEVELS
    vint loop;
    vint maxSample;
    vint rmsCurrent;

    maxSample = 0;
    rmsCurrent = 0;

    /*
     * Check maximum sample values, and 
     * compute the RMS level at rx_ptr
     * use averaging of the result
     * rms = (7/8) * rms_old + (1/8) * rms_new
     */
    for (loop = 0; loop < numSamples10ms; loop++) {
        if (COMM_ABS(audioIn_ary[loop]) > maxSample) {
            maxSample = COMM_ABS(audioIn_ary[loop]);
        }
        if (8192 < maxSample) {
            OSAL_logMsg("%s:%d maxSample = %d\n", (int)__FILE__, __LINE__,
                    maxSample, 0);
        }
    }

    rmsCurrent = COMM_lrms(audioIn_ary, numSamples10ms);
    _rms = _rms - ((_rms - rmsCurrent) >> 3);
    _runningCnt++;
    if (0 == (0x7f & _runningCnt)) {
        /*
         * Check results about every second
         */
        OSAL_logMsg("%s:%d Rx RMS = %d\n", (int)__FILE__, __LINE__, _rms, 0);
        OSAL_logMsg("%s:%d maxSample = %d\n", (int)__FILE__, __LINE__,
                maxSample, 0);
    
    }
#endif
}

/*
 * ======== _VTSPR_generateImpulse() ========
 *
 * Measure Impulse.
 *
 */
void _VTSPR_generateImpulse(
        vint *audioOut_ary,
        vint  numSamples10ms)
{
#ifdef VTSP_ENABLE_MEASURE_IMPULSE    
    vint cnt;

    if (!_delayFound) {
        _impulseTime++;

        for (cnt = 0; cnt < numSamples10ms; cnt++) {
            /* write over the output with zeros */
            audioOut_ary[cnt] = 0;
        }

        if (0 == (0xFF & _impulseTime)) {
            /*
             * Send impulse once per 1.27 sec
             */
            audioOut_ary[0] = _impulseLevel;
            _impulseSent = _impulseTime;
            OSAL_logMsg("TX impulse #%d @ %d\n", ++_impulseNum, _impulseSent);
        }
    }
#endif
}

/*
 * ======== _VTSPR_detectImpulse() ========
 *
 * Measure Impulse.
 *
 */
void _VTSPR_detectImpulse(
    VTSPR_DSP     *dsp_ptr,
    VTSPR_ChanObj *chan_ptr,
    vint           infc)
{
#ifdef VTSP_ENABLE_MEASURE_IMPULSE    
    vint  cnt;
    vint  chan;
    uvint diff;
    
    if (!_delayFound) {
        for (cnt = 0; cnt < chan_ptr->numSamples10ms; cnt++) {
            /* write over the output with zeros */
            if (chan_ptr->audioIn_ary[cnt] > (350)) {
                diff = _impulseTime - _impulseSent;
                OSAL_logMsg("RX impulse @ frame=%d, diff=%d, sample[%d]=%d\n",
                        _impulseTime, diff, cnt, chan_ptr->audioIn_ary[cnt]);
                        
                if (diff != _diffOld) {
                    _diffOld = diff;
                    _sameCount = 0;
                }
                
                else {
                    _sameCount++;
                }
                
                if (_sameCount > 2) {
//                    _delayFound = 1;
                }
#ifdef VTSP_ENABLE_AEC
                /* 
                 * Configure AEC to use sample offset and nAllignBufs and Reset
                 */
                if (diff <= VTSPR_NALLIGNMENT_BUFS) { /* protect bad config */
                    chan_ptr->aec_ptr->nAllignBufs =  diff + 1;
                    dsp_ptr->aecNearParams_ptr->pNEAR_END_DELAY = cnt-3;
                    chan = _VTSPR_infcToChan(infc);
                    _VTSPR_algStateChan(dsp_ptr, chan, VTSPR_ALG_CHAN_AEC, 0);
                    _VTSPR_algStateChan(dsp_ptr, chan, 0, VTSPR_ALG_CHAN_AEC);
                }
#endif
                break;
            }
        }
    }
#endif
}

/*
 * ======== _VTSPR_audioRxFormat() ========
 * Format audio received from the hardware driver.
 * !!! Important Note !!!
 * Currently no support for mixed speech format systems.
 */
void _VTSPR_audioRxFormat(
    VTSPR_Obj    *vtspr_ptr,
    VTSPR_DSP    *dsp_ptr)
{
    VTSPR_ChanObj *chan_ptr;
    DCRM_Obj      *dcrm_ptr;
    vint          *rx_ptr;
    vint           infc;

    _VTSPR_FOR_ALL_INFC(infc) { 
        chan_ptr = _VTSPR_infcToChanPtr(dsp_ptr, infc);

#ifdef VTSP_ENABLE_AUDIO_SRC
        rx_ptr = chan_ptr->audioIn_ary;
#else
        rx_ptr = chan_ptr->rx_ptr;
#endif
#ifdef VHW_AUDIO_MULAW
        /* Conver from mu-law to 14-bit linear */
        COMM_mu2lin(chan_ptr->audioIn_ary, rx_ptr, chan_ptr->numSamples10ms);
#else
        /* Shift all samples down by VHW_AUDIO_SHIFT bits */
        COMM_shiftRight(chan_ptr->audioIn_ary, rx_ptr,
                VHW_AUDIO_SHIFT, chan_ptr->numSamples10ms);
#endif
        _VTSPR_detectImpulse(dsp_ptr, chan_ptr, infc);

        _VTSPR_netlogSend(vtspr_ptr, 0 + 10*infc, chan_ptr->audioIn_ary,
                chan_ptr->numSamples10ms * sizeof(vint), infc);

        /* Remove DC from linear input */
        dcrm_ptr          = &chan_ptr->dcrmNearObj;
        dcrm_ptr->src_ptr = chan_ptr->audioIn_ary;
        dcrm_ptr->dst_ptr = chan_ptr->audioIn_ary;
#ifdef VTSP_ENABLE_BENCHMARK
       _VTSPR_benchmarkStart(vtspr_ptr, _VTSPR_BENCHMARK_DCRM_NEAR, 1);
#endif
        DCRM_run(&chan_ptr->dcrmNearObj);
#ifdef VTSP_ENABLE_BENCHMARK
       _VTSPR_benchmarkStop(vtspr_ptr, _VTSPR_BENCHMARK_DCRM_NEAR, 1);
#endif
        chan_ptr->dbRx = _VTSPR_audioMeasure(chan_ptr->audioIn_ary,
                chan_ptr->numSamples10ms);

        _VTSPR_measureLevels(chan_ptr->audioIn_ary, chan_ptr->numSamples10ms);
    }
}

/*
 * ======== _VTSPR_audioRoutFx() ========
 * Process Rout(Rin) for ECSR for align canceller buffers.
 */
void _VTSPR_audioRoutFx(
    VTSPR_DSP    *dsp_ptr)
{
#ifdef VTSP_ENABLE_ECSR
    VTSPR_ChanObj *chan_ptr;
    _VTSPR_EcObj  *ec_ptr;
    vint          *rin_ptr; 
    vint           infc;

    _VTSPR_FOR_ALL_FX(infc) { 
        chan_ptr = _VTSPR_infcToChanPtr(dsp_ptr, infc);
        ec_ptr   = chan_ptr->ec_ptr;

        /*
         * Assign the current Rin pointer, which point to the newest
         * voice sample buffer.
         */
        rin_ptr = ec_ptr->ecRin_ary[(ec_ptr->nAllignBufs - 1)];

        /*
         * Age the Rin buffer
         * Also, copy the new audioOut to newest Rin
         */
        COMM_copy(ec_ptr->ecRin_ary[0], ec_ptr->ecRin_ary[1],
                ((ec_ptr->nAllignBufs - 1) * chan_ptr->numSamples10ms));
        COMM_copy(rin_ptr, chan_ptr->audioOut_ary, chan_ptr->numSamples10ms);
    }
#endif /* VTSP_ENABLE_ECSR */
}

/*
 * ======== _VTSPR_audioTxFormat() ========
 * Format audio for transmission to the hardware driver.
 * !!! Important Note !!!
 * Currently no support for mixed speech format systems.
 */
void _VTSPR_audioTxFormat(
    VTSPR_Obj    *vtspr_ptr,
    VTSPR_DSP    *dsp_ptr)
{
    VTSPR_ChanObj *chan_ptr;
    vint          *tx_ptr;
    vint           infc;
    uint32         mask;

    _VTSPR_FOR_ALL_INFC(infc) { 
        chan_ptr = _VTSPR_infcToChanPtr(dsp_ptr, infc);
        mask     = _VTSPR_getAlgStateChan(chan_ptr, infc);

        chan_ptr->dbTx = _VTSPR_audioMeasure(chan_ptr->audioOut_ary,
                chan_ptr->numSamples10ms);

        /* 
         * Apply gain in dsp->codec_tx->infc direction, if any.
         * Done after handling ECSR Rin copy.
         */
        if (0 != chan_ptr->gainAudioOut) {
            _VTSPR_audioGain(chan_ptr->audioOut_ary, chan_ptr->gainAudioOut,
                             chan_ptr->numSamples10ms);
        }

        if (0 == infc) {
            _VTSPR_generateImpulse(chan_ptr->audioOut_ary,
                chan_ptr->numSamples10ms);
        }

        _VTSPR_netlogSend(vtspr_ptr, 1 + 10*infc, chan_ptr->audioOut_ary,
                chan_ptr->numSamples10ms * sizeof(vint), infc);

        /*
         * Mute audio out to infc if requested
         */
        if (0 != (VTSPR_ALG_CHAN_MUTE_OUT & mask)) {
            /* Audio Mute
             * Zero audio out, to the phone
             */
            COMM_fill(chan_ptr->audioOut_ary, 0, chan_ptr->numSamples10ms);
        }
#ifdef VTSP_ENABLE_AUDIO_SRC
        tx_ptr = chan_ptr->audioOut_ary;
#else
        tx_ptr = chan_ptr->tx_ptr;
#endif

#ifdef VHW_AUDIO_MULAW
        /* Convert from 14-bit to mu-law */
        COMM_lin2mu(tx_ptr, chan_ptr->audioOut_ary, chan_ptr->numSamples10ms);
#else
        /* Shift all samples left by VHW_AUDIO_SHIFT bits */
        COMM_shiftLeft(tx_ptr, chan_ptr->audioOut_ary,
                VHW_AUDIO_SHIFT, chan_ptr->numSamples10ms);
#endif
    }
}

/*
 * ======== _VTSPR_removeEchoFx() ========
 * Remove line echo (for the benefit of network) from Fx physical interfaces.
 * !!! Important Note !!!
 * This function supports narrowband only.
 */
void _VTSPR_audioRemoveEchoFx(
        VTSPR_Obj    *vtspr_ptr,
        VTSPR_DSP    *dsp_ptr)
{
    VTSPR_ChanObj  *chan_ptr;
#ifdef VTSP_ENABLE_ECSR    
    _VTSPR_EcObj   *ec_ptr;
    EC_Obj         *ecsr_ptr;
    NFE_Object     *nfe_ptr;
    NLP_Obj        *nlp_ptr;
    vint            runNlp;
#endif
#ifdef VTSP_ENABLE_FMTD    
    _VTSPR_FmtdObj *fmtd_ptr;
#endif
#if defined(VTSP_ENABLE_ECSR) || defined(VTSP_ENABLE_FMTD)
    uint32          mask;
#endif
    vint            infc;

    _VTSPR_FOR_ALL_FX(infc) {
        chan_ptr = _VTSPR_infcToChanPtr(dsp_ptr, infc);
#if defined(VTSP_ENABLE_ECSR) || defined(VTSP_ENABLE_FMTD)
        mask     = _VTSPR_getAlgStateChan(chan_ptr, infc);
#endif

        /*
         * Set appropriate pointers.
         */
#ifdef VTSP_ENABLE_ECSR    
        ec_ptr   = chan_ptr->ec_ptr;
        ecsr_ptr = &ec_ptr->ecsrObj;
        nlp_ptr  = &ec_ptr->nlpObj;
        nfe_ptr  = &chan_ptr->nfeInfcObj;
        runNlp   = 1;

        /*
         * Echo canceller
         *
         * NOTE: The echo canceller ECSR must always be called for FXO and FXS.
         * If echo cancellation is not desired, then
         * set the bypass bit and run.
         * NLP does not always have to be called.  However if it is
         * not called, the input buffer must be copied into the output
         * buffer.
         */
#ifdef VTSP_ENABLE_FMTD
        if (0 != (VTSPR_ALG_CHAN_ECSR_FMTD & mask)) { 
            /* Bypass EC for phase reversal modem/fax tone */
            fmtd_ptr = chan_ptr->fmtd_ptr;
            if ((mask & VTSPR_ALG_CHAN_FAX) && 
                    ((1 == fmtd_ptr->fmtdPhaseRevInfc) || (1 == fmtd_ptr->fmtdPhaseRevPeer))) { 
                /* 
                 * FAX detected and Phase Reverse detected.
                 * Modify EC behavior for the current blocktime,
                 * turn off EC.
                 */
                mask &= ~(VTSPR_ALG_CHAN_ECSR);
            }
        }
#endif

        if (0 != (VTSPR_ALG_CHAN_ECSR & mask)) {
            ecsr_ptr->control &= ~(EC_BYP | EC_FRZ);
            if (VTSPR_ALG_CHAN_ECSR_FRZ & mask) { 
                ecsr_ptr->control |= EC_FRZ;
                chan_ptr->echoCancEvent = VTSP_EVENT_ECHO_CANC_FRZ;
            }
            else {
                chan_ptr->echoCancEvent = VTSP_EVENT_ECHO_CANC_ON;
            }
            _VTSPR_netlogSend(vtspr_ptr, 2 + 10*infc, ecsr_ptr->sin_ptr,
                    chan_ptr->numSamples10ms * sizeof(vint), infc);
            _VTSPR_netlogSend(vtspr_ptr, 4 + 10*infc, ecsr_ptr->rin_ptr,
                    chan_ptr->numSamples10ms * sizeof(vint), infc);
#ifdef VTSP_ENABLE_BENCHMARK
            _VTSPR_benchmarkStart(vtspr_ptr, _VTSPR_BENCHMARK_ECSR, 0);
#endif
            ECSR_run(ecsr_ptr, nfe_ptr);
            _VTSPR_netlogSend(vtspr_ptr, 3 + 10*infc, ecsr_ptr->sout_ptr,
                   chan_ptr->numSamples10ms * sizeof(vint), infc);
            /*
             * If the NLP is not to be run, copy data.
             */
            if (VTSPR_ALG_CHAN_NLP & mask) {
                NLP_run(nlp_ptr, (void *)ecsr_ptr, nfe_ptr);

                _VTSPR_netlogSend(vtspr_ptr, 5 + 10*infc, nlp_ptr->dst_ptr,
                        chan_ptr->numSamples10ms * sizeof(vint), infc);
            }
            else {
                runNlp = 0;
            }
#ifdef VTSP_ENABLE_BENCHMARK
            _VTSPR_benchmarkStop(vtspr_ptr, _VTSPR_BENCHMARK_ECSR, 0);
#endif
        }
        else {
            /*
             * Bypass canceller and do not run NLP.
             */
            ecsr_ptr->control |= EC_BYP;
            chan_ptr->echoCancEvent = VTSP_EVENT_ECHO_CANC_OFF;
            ECSR_run(ecsr_ptr, nfe_ptr);
            runNlp = 0;
        }
        /*
         * Copy if not running NLP.  Also, force NLP status to
         * make sure NLP isn't in ACTIVE state.
         */
        if (0 == runNlp) {
            nlp_ptr->status &= ~NLP_BLK_ACT;
            COMM_copy(chan_ptr->ecNlp_ary, chan_ptr->ecSout_ary,
                    chan_ptr->numSamples10ms);
        } 
#else
       COMM_copy(chan_ptr->ecNlp_ary, chan_ptr->audioIn_ary,
               chan_ptr->numSamples10ms); 
#endif /* VTSP_ENABLE_ESCR */
    }
}

/*
 * ======== _VTSPR_audioNear() ========
 * Process near end audio
 * Run:
 *  DTMF
 *  FSKR/CIDS
 *  TONE
 *  NFE
 *  MUTE
 */
void _VTSPR_audioNear(
    VTSPR_Obj   *vtspr_ptr,
    VTSPR_DSP   *dsp_ptr)
{
    VTSPR_ChanObj  *chan_ptr;
#ifdef VTSP_ENABLE_CIDR
    _VTSPR_CidrObj *cidr_ptr;
#endif
#ifdef VTSP_ENABLE_FMTD
    _VTSPR_FmtdObj *fmtd_ptr;
    FMTD_Obj       *fmtdInfc_ptr;
    FMTD_Obj       *fmtdPeer_ptr;
    vint            detectInfc1;
    vint            detectPeer1;
    vint            detectInfc2;
    vint            detectPeer2;
    vint            leInfc;
    vint            lePeer;
    vint            teInfc;
    vint            tePeer;
    vint            chan;
#endif
#ifdef VTSP_ENABLE_FXO
#ifdef VTSP_ENABLE_UTD
    _VTSPR_UtdObj  *utd_ptr;
    UTD_Obj        *utdObj_ptr;
    NFE_Object     *nfe_ptr;
#endif
#ifdef VTSP_ENABLE_CIDR
    TIC_Obj        *tic_ptr;
    CAS_Obj        *cas_ptr;
    FSKR_Obj       *fskr_ptr;
    VTSPR_ToneSeq  *toneSeq_ptr;
    vint            tId;
    vint            fskrTe;
    vint            data;
    vint            data1;
    vint            data2;
#endif
#endif    
    vint            infc;
#if defined(VTSP_ENABLE_DTMF) || defined(VTSP_ENABLE_FMTD) || \
        defined(VTSP_ENABLE_FXO) || defined(VTSP_ENABLE_CIDR)
    uint32          mask;
#endif
#ifdef VTSP_ENABLE_DTMF
    _VTSPR_DtmfObj *dtmf_ptr;
    vint           *nlpOut_ptr;
    DTMF_Obj       *dtmfObj_ptr;
    vint            digit1;
    vint            digit2;
    vint            detect;
    vint            lastDigit;
    vint            tmpBuf[VTSPR_NSAMPLES_10MS_8K >> 1];
#endif
#if defined(VTSP_ENABLE_DTMF) || defined(VTSP_ENABLE_FMTD)
    vint            detect1;
    vint            detect2;
    vint            le;
    vint            te;
#endif
   
    _VTSPR_FOR_ALL_FX(infc) {

#ifdef VTSP_ENABLE_FMTD
        chan     = _VTSPR_infcToChan(infc);
#endif
        chan_ptr = _VTSPR_infcToChanPtr(dsp_ptr, infc);
#if defined(VTSP_ENABLE_DTMF) || defined(VTSP_ENABLE_FMTD) || \
        defined(VTSP_ENABLE_FXO) || defined(VTSP_ENABLE_CIDR)
        mask     = _VTSPR_getAlgStateChan(chan_ptr, infc);
#endif

#ifdef VTSP_ENABLE_DTMF
        /*
         * DTMF detect
         */
        if (0 != (VTSPR_ALG_CHAN_DTMF & mask) || 
                0 != (VTSPR_ALG_CHAN_CIDCWS & mask) ) {

            /* 
             * DTMF Detector runs in two cases.
             * 1. detector enabled by VTSPR or VTSP application
             * 2. callerID call waiting is active
             */
            dtmf_ptr    = chan_ptr->dtmf_ptr;
            dtmfObj_ptr = &dtmf_ptr->dtmfObj;
            nlpOut_ptr  = chan_ptr->ecNlp_ary;
            
            /* 
             * Run DTMF detection on audio In.
             * DTMF_detect must be run twice, for each 5ms audio
             *
             * The first 5ms 'early' flag is meaningless since only
             * last audio 'early' matters.
             */
            /* dot: ecNlp->dtmf_detect; */
            dtmfObj_ptr->src_ptr = nlpOut_ptr;

#ifdef VTSP_ENABLE_BENCHMARK
            _VTSPR_benchmarkStart(vtspr_ptr, _VTSPR_BENCHMARK_DTMF, 0);
#endif
            DTMF_detect(dtmfObj_ptr);
            digit1  = dtmfObj_ptr->digit;
            detect1 = dtmfObj_ptr->detected;

            /*
             * Perform DTMF remove only after 3 early consecutive early
             * detects have been reported.
             */
            if (dtmfObj_ptr->early) {
                dtmf_ptr->earlyCount++;
            }
            else {
                dtmf_ptr->earlyCount = 0;
            }
                
            dtmfObj_ptr->dst_ptr = &tmpBuf[0];
            if (dtmf_ptr->dtmfRemove) {
                if (dtmf_ptr->earlyCount >= 3) {
                    dtmfObj_ptr->dst_ptr = dtmfObj_ptr->src_ptr;
                }
            }
            /* dot: dtmf_detect->dtmf_remove->ecNlp; */
            DTMF_remove(dtmfObj_ptr);
                
            /*
             * Process the next 5ms data 
             */
            dtmfObj_ptr->src_ptr = &nlpOut_ptr[VTSPR_NSAMPLES_10MS_8K >> 1];
            DTMF_detect(dtmfObj_ptr);
            digit2  = dtmfObj_ptr->digit;
            detect2 = dtmfObj_ptr->detected;

            if (dtmfObj_ptr->early) {
                dtmf_ptr->earlyCount++;
            }
            else {
                dtmf_ptr->earlyCount = 0;
            }
            dtmfObj_ptr->dst_ptr = &tmpBuf[0];
            if (dtmf_ptr->dtmfRemove) {
                if (dtmf_ptr->earlyCount >= 3) {
                    dtmfObj_ptr->dst_ptr = dtmfObj_ptr->src_ptr;
                }
            }
            DTMF_remove(dtmfObj_ptr);

#ifdef VTSP_ENABLE_BENCHMARK
            _VTSPR_benchmarkStop(vtspr_ptr, _VTSPR_BENCHMARK_DTMF, 0);
#endif

            /*
             * Check DTMF detections using Bug 2976 enhancements.
             * This logic OR's the 5 ms DTMF_detect results together and assumes
             * there are no DTMF transitions which happen faster than 20 ms.
             * Note, the DTMF module is configurable for minimum 25 ms tone
             * duration, and 25 ms interdigit interval duration.
             */
            lastDigit = dtmf_ptr->dtmfDigit;
            le        = -1;
            te        = -1;
            detect = (detect1 | detect2);

            if (detect) {
                /*
                 * Digit is currently detected
                 * Check if it is a leading edge
                 */
                if (-1 == lastDigit) {
                    if (detect1) {
                        le = digit1;
                        lastDigit = digit1;
                    }
                    else {
                        le = digit2;
                        lastDigit = digit2;
                    }
                }
            }
            else {
                /*
                 * Digit is not currently detected.  
                 * Check if it is a tailing edge
                 */
                if (lastDigit > -1) {
                    /* Tailing edge is detected */
                    te = lastDigit;
                }
                lastDigit = -1;
            }

            /*
             * Update the detection results.  
             * Event processor checks dtmfLe and dtmfTe and generates events
             * based on these values.
             */
            dtmf_ptr->dtmfDigit = lastDigit;
            dtmf_ptr->dtmfLe    = le;
            dtmf_ptr->dtmfTe    = te;

#ifdef VTSP_ENABLE_CIDR
            cidr_ptr = chan_ptr->cidr_ptr;
            if (VTSPR_INFC_TYPE_FXO == _VTSPR_infcToType(infc)) { 
                /*
                 * ETSI DTMF caller ID receive.
                 * A, B, D indicate start of a CID field
                 * C indicates end of CID.
                 * Trigger event on leading edge.
                 */
                /* dot: dtmf->cid_dtmf_detect; */
                if ((0 == cidr_ptr->inDtmfCid) && (le >= 0)) {
                    data = (uint8)_VTSPR_dtmfToChar(le & 0xFF);
                    if ((('A' == data) || ('B' == data) || ('D' == data))) {
                        /*
                         * Start of CID
                         */
                        cidr_ptr->dtmfCidCount = 0;
                        cidr_ptr->dtmfData_ary[cidr_ptr->dtmfCidCount] = data;
                        cidr_ptr->inDtmfCid = 1;
                        if (sizeof(_VTSP_CIDData) == ++cidr_ptr->dtmfCidCount) {
                            cidr_ptr->dtmfCidCount = sizeof(_VTSP_CIDData) - 1;
                        }
                    }
                }
                else if ((cidr_ptr->inDtmfCid > 0) && (le >= 0)) {
                    data = (uint8)_VTSPR_dtmfToChar(le & 0xFF);
                    /*
                     * In CID
                     */
                    if ('C' == data) {
                        /*
                         * C, end of DTMF CID
                         */
                        cidr_ptr->dtmfData_ary[cidr_ptr->dtmfCidCount] = 0;
                        cidr_ptr->inDtmfCid = 0;
                        cidr_ptr->dtmfCidEvt = 1;
                    }
                    else if ((data >= 35) && (data <= 68)) {
                        /*
                         * Range includes #, *, 0-9, A-D
                         */
                        cidr_ptr->inDtmfCid = 1;
                        cidr_ptr->dtmfData_ary[cidr_ptr->dtmfCidCount] = data;
                        if (sizeof(_VTSP_CIDData) == ++cidr_ptr->dtmfCidCount) {
                            cidr_ptr->dtmfCidCount = sizeof(_VTSP_CIDData) - 1;
                        }
                    }
                }
                else if ((cidr_ptr->inDtmfCid > 0) && (-1 == le)) {
                    /*
                     * Timeout
                     */
                    if (100 == cidr_ptr->inDtmfCid++) {
                        /*
                         * Timeout after 1 second
                         */
                        cidr_ptr->inDtmfCid = 0;
                        cidr_ptr->dtmfData_ary[0] = 0;
                        cidr_ptr->dtmfCidCount = 0;
                    }
                }
            }
#endif /* VTSP_ENABLE_CIDR */
        }
#endif /* VTSP_ENABLE_DTMF */

#ifdef VTSP_ENABLE_FMTD
        /*
         * FMTD detect
         */
        /* dot: ecNlp->fmtd_detect; */
        fmtd_ptr = chan_ptr->fmtd_ptr;
        if (0 != (VTSPR_ALG_CHAN_FMTD & mask)) {
            fmtdInfc_ptr   = &fmtd_ptr->fmtdInfcObj;
            fmtdPeer_ptr   = &fmtd_ptr->fmtdPeerObj;
            
            fmtdInfc_ptr->src_ptr = chan_ptr->ecNlp_ary;
            fmtdPeer_ptr->src_ptr = chan_ptr->audioOut_ary;

            /* 
             * Reuse the detect1 and detect2 variables from DTMF processing.
             * For FMTD's purpose, they signify the detection status before
             * and after calling FMTD.
             *
             * Implement full duplex detect as per Bug 2118
             */
#ifdef VTSP_ENABLE_BENCHMARK
            _VTSPR_benchmarkStart(vtspr_ptr, _VTSPR_BENCHMARK_FMTD, 0);
#endif

            detect1 = (fmtdInfc_ptr->detected | fmtdPeer_ptr->detected);
            detectInfc1 = (fmtdInfc_ptr->detected);
            detectPeer1 = (fmtdPeer_ptr->detected);
            FMTD_detect(fmtdInfc_ptr);
            FMTD_detect(fmtdPeer_ptr);
            detectInfc2 = (fmtdInfc_ptr->detected);
            detectPeer2 = (fmtdPeer_ptr->detected);
            detect2 = (fmtdInfc_ptr->detected | fmtdPeer_ptr->detected);

#ifdef VTSP_ENABLE_BENCHMARK
            _VTSPR_benchmarkStop(vtspr_ptr, _VTSPR_BENCHMARK_FMTD, 0);
#endif
            fmtd_ptr->fmtdTypeInfc = VTSP_EVENT_INACTIVE;
            fmtd_ptr->fmtdTypePeer = VTSP_EVENT_INACTIVE;

            /* Determine FMTD detection bits that have turned on */
            le = (detect1 ^ detect2) & detect2;
            leInfc = (detectInfc1 ^ detectInfc2) & detectInfc2;
            lePeer = (detectPeer1 ^ detectPeer2) & detectPeer2;
            if (0 != le) {
                /*
                 * Remember that a FAX was detected sometime during this call.
                 * This will switch to G711U regardless of T38.
                 *
                 * Bug 2438
                 * Disable NLP for the remainder of this call, until the next   
                 * SEIZE event, as per customer requirement.
                 * Fax mode with phase reversal disables ECSR.
                 */
                _VTSPR_algStateChan(dsp_ptr, chan, VTSPR_ALG_CHAN_NLP,
                        VTSPR_ALG_CHAN_FAX); 
            }
            if (0 != leInfc) {
                /*
                 * Check what type of detection was flagged by FMTD on infc
                 */
                if (leInfc & (1 << FMTD_PHASE_BIT)) {
                    fmtd_ptr->fmtdPhaseRevInfc = 1;
                }

                if (detectInfc2 & (1 << FMTD_ANSWER_BIT)) {
                    fmtd_ptr->fmtdTypeInfc = VTSP_EVENT_FAX_ANSWER;
                }
                else if (detectInfc2 & (1 << FMTD_FC_BIT)) {
                    fmtd_ptr->fmtdTypeInfc = VTSP_EVENT_FAX_CALLING;
                }
                else if (detectInfc2 & (1 << FMTD_V21_BIT)) {
                    fmtd_ptr->fmtdTypeInfc = VTSP_EVENT_FAX_PREAMBLE;
                }
                else if (detectInfc2 != 0) {
                    fmtd_ptr->fmtdTypeInfc = VTSP_EVENT_FAX_OTHER;
                }
            }
            if (0 != lePeer) {
                /*
                 * Check what type of detection was flagged by FMTD on peer
                 */
                if (lePeer & (1 << FMTD_PHASE_BIT)) {
                    fmtd_ptr->fmtdPhaseRevPeer = 1;
                }

                if (detectPeer2 & (1 << FMTD_ANSWER_BIT)) {
                    fmtd_ptr->fmtdTypePeer = VTSP_EVENT_FAX_ANSWER;
                }
                else if (detectPeer2 & (1 << FMTD_FC_BIT)) {
                    fmtd_ptr->fmtdTypePeer = VTSP_EVENT_FAX_CALLING;
                }
                else if (detectPeer2 & (1 << FMTD_V21_BIT)) {
                    fmtd_ptr->fmtdTypePeer = VTSP_EVENT_FAX_PREAMBLE;
                }
                else if (detectPeer2 != 0){
                    fmtd_ptr->fmtdTypePeer = VTSP_EVENT_FAX_OTHER;
                }
            }

            /* Determine FMTD detection bits that have turned off */
            teInfc = (detectInfc1 ^ detectInfc2) & detectInfc1;
            tePeer = (detectPeer1 ^ detectPeer2) & detectPeer1;
            if (0 != teInfc) {
                /*
                 * Check which detection bit was reset on infc
                 */
                if (detectInfc1 & (1 << FMTD_ANSWER_BIT)) {
                    fmtd_ptr->fmtdTypeInfc = VTSP_EVENT_FAX_ANSWER;
                }
                else if (detectInfc1 & (1 << FMTD_FC_BIT)) {
                    fmtd_ptr->fmtdTypeInfc = VTSP_EVENT_FAX_CALLING;
                }
                else if (detectInfc1 & (1 << FMTD_V21_BIT)) {
                    fmtd_ptr->fmtdTypeInfc = VTSP_EVENT_FAX_PREAMBLE;
                }
                else if (detectInfc1 != 0) {
                    fmtd_ptr->fmtdTypeInfc = VTSP_EVENT_FAX_OTHER;
                }
            }
            
            if (0 != tePeer) {
                /*
                 * Check which detection bit was reset on peer
                 */
                if (detectPeer1 & (1 << FMTD_ANSWER_BIT)) {
                    fmtd_ptr->fmtdTypePeer = VTSP_EVENT_FAX_ANSWER;
                }
                else if (detectPeer1 & (1 << FMTD_FC_BIT)) {
                    fmtd_ptr->fmtdTypePeer = VTSP_EVENT_FAX_CALLING;
                }
                else if (detectPeer1 & (1 << FMTD_V21_BIT)) {
                    fmtd_ptr->fmtdTypePeer = VTSP_EVENT_FAX_PREAMBLE;
                }
                else if (detectPeer1 != 0) {
                    fmtd_ptr->fmtdTypePeer = VTSP_EVENT_FAX_OTHER;
                }
            }

            /* 
             * Event processor checks fmtdLe and fmtdTe and generates events
             * based on these values.
             */
            fmtd_ptr->fmtdLeInfc    = leInfc;
            fmtd_ptr->fmtdLePeer    = lePeer;
            fmtd_ptr->fmtdTeInfc    = teInfc;
            fmtd_ptr->fmtdTePeer    = tePeer;
        }

        /* 
         * Detect silence for fax timeout.  If exceeds silence time, 
         * then turn off fax mode.
         */
        if (mask & VTSPR_ALG_CHAN_FAX) {
            if (fmtd_ptr->fmtdSilenceTime < fmtd_ptr->fmtdSilenceTimeMax) {
                if ((chan_ptr->dbRx < dsp_ptr->faxSilRxDB) &&
                        (chan_ptr->dbTx < dsp_ptr->faxSilTxDB)) {
                    /* Silence detected */
                    fmtd_ptr->fmtdSilenceTime++;
                }
                else {
                    /* Audio detected */
                    fmtd_ptr->fmtdSilenceTime = 0;
                }
            }
            else {
                /* Time out */
                fmtd_ptr->fmtdTimeoutEvent = VTSP_EVENT_FAX_TIMEOUT;
                fmtd_ptr->fmtdSilenceTime = 0;
                _VTSPR_algStateChan(dsp_ptr, chan, VTSPR_ALG_CHAN_FAX, 0); 
            }
        }
#endif /* VTSP_ENABLE_FMTD */
    }

#ifdef VTSP_ENABLE_FXO
    _VTSPR_FOR_ALL_FXO(infc) {
        
        chan_ptr = _VTSPR_infcToChanPtr(dsp_ptr, infc);
        mask     = _VTSPR_getAlgStateChan(chan_ptr, infc);

        /*
         * UTD
         */
        if (0 != (VTSPR_ALG_CHAN_UTD & mask)) {
#ifdef VTSP_ENABLE_UTD        
            nfe_ptr    = &chan_ptr->nfeInfcObj;
            utd_ptr    = chan_ptr->utd_ptr;
            utdObj_ptr = &utd_ptr->utdObj;
            /*
             * Points to note here.
             * 1. UTD detection lasts for at least 3 calls, so running at 10 ms
             *    with UTD configured for 5 ms does not need logic for
             *    detection.
             * 2. UTD uses NFE object for source data.
             * 3. Tone retval is the tone template Id set by default/user.
             * 4. UTD_detect returns two-byte value: 
             *      hi-byte is tone code,
             *      lo-byte is retVal (vtsp tone template Id)
             */
            detect1 = utdObj_ptr->detect;
         
            UTD_detect(utdObj_ptr, nfe_ptr);
            nfe_ptr->src_ptr += (VTSPR_NSAMPLES_10MS_8K >> 1);
            UTD_detect(utdObj_ptr, nfe_ptr);
            nfe_ptr->src_ptr -= (VTSPR_NSAMPLES_10MS_8K >> 1);
            
            detect2 = utdObj_ptr->detect;
            /* Determine UTD detection bits that have turned on */
            le = (detect1 ^ detect2) & detect2;
            /* Determine UTD detection bits that have turned off */
            te = (detect1 ^ detect2) & detect1;

            /* 
             * Event processor checks utdLe and utdTe and generates events
             * based on these values.
             */
            utd_ptr->utdLe = le;
            utd_ptr->utdTe = te;
#endif
        }

        /*
         * FSKR and CID type I and type II recieve.
         */
#ifdef VTSP_ENABLE_CIDR
        fskrTe = 0;
        cidr_ptr = chan_ptr->cidr_ptr;
        if (0 != ((VTSPR_ALG_CHAN_FSKR_ONH | VTSPR_ALG_CHAN_FSKR_OFH) & mask)) {
            fskr_ptr = &cidr_ptr->fskrObj;
            
            /* 
             * Run FSKR detection on audio In.
             * for a bug in FSKR, it cannot be run on 10 ms, rather on 5 ms.
             */
            /* dot: ecNlp->fskr_detect; */
            fskr_ptr->src_ptr = chan_ptr->ecNlp_ary;

            FSKR_run(fskr_ptr);
            detect1 = fskr_ptr->status;
            data1 = fskr_ptr->output;
            fskr_ptr->src_ptr += (VTSPR_NSAMPLES_10MS_8K >> 1);
            FSKR_run(fskr_ptr);
            detect2 = fskr_ptr->status;
            data2 = fskr_ptr->output;

            if (FSKR_DATA_VALID == detect1) {
                cidr_ptr->fskrData_ary[cidr_ptr->fskrCnt] = data1;
                cidr_ptr->fskrCsum += data1;
                if (sizeof(_VTSP_CIDData) == ++cidr_ptr->fskrCnt) {
                    cidr_ptr->fskrCnt = sizeof(_VTSP_CIDData) - 1;
                }
            }

            if (FSKR_DATA_VALID == detect2) {
                cidr_ptr->fskrData_ary[cidr_ptr->fskrCnt] = data2;
                cidr_ptr->fskrCsum += data2;
                if (sizeof(_VTSP_CIDData) == ++cidr_ptr->fskrCnt) {
                    cidr_ptr->fskrCnt = sizeof(_VTSP_CIDData) - 1;
                }
            }
            
            /*
             * Check leading and trailing edges.
             */
            if ((FSKR_DATA_VALID == detect1) || (FSKR_PRESENT == detect1) ||
                   (FSKR_DATA_VALID == detect2) || (FSKR_PRESENT == detect2)) {

                if (0 == cidr_ptr->fskrDetect) {
                    /*
                     * Leading edge of detection. Reset counter.
                     */
                    cidr_ptr->fskrDetect = 1;
                    cidr_ptr->fskrCnt    = 0;
                    cidr_ptr->fskrCsum   = 0;
                    cidr_ptr->fskrValid  = 0;
                }
            }
            else {
                if (0 != cidr_ptr->fskrDetect) {
                    /*
                     * Trailing edge
                     * Calculate checksum.
                     */
                    fskrTe = 1;
                    data   = (uvint)cidr_ptr->fskrData_ary[
                            --cidr_ptr->fskrCnt];
                    cidr_ptr->fskrCsum -= data;
                    cidr_ptr->fskrCsum = 
                            0x100 - (cidr_ptr->fskrCsum & 0xFF);

                    if (cidr_ptr->fskrCsum == data) {
                        /*
                         * Valid FSKR detection.
                         */
                        cidr_ptr->fskrValid = 1;
                        if (0 != cidr_ptr->cid2run) {
                            cidr_ptr->cid2Detect = 1;
                        }
                    }
                    else {
                        cidr_ptr->fskrValid = 0;
                    }
                    cidr_ptr->fskrDetect = 0;
                    cidr_ptr->fskrData_ary[cidr_ptr->fskrCnt] = 0;
                }
                else {
                    if (0 != cidr_ptr->fskrValid) {
                        cidr_ptr->fskrValid = 0;
                    }
                }
            }
        }

        /*
         * CAS
         */
        /* dot: ecNlp->cas_detect; */
        if (0 != (VTSPR_ALG_CHAN_CAS & mask)) {
            cas_ptr = &cidr_ptr->casObj;
            
            /* 
             * Run CAS detection on audio In.
             */

            cas_ptr->src_ptr = chan_ptr->ecNlp_ary;
            CAS_run(cas_ptr);
            detect1 = cas_ptr->detected;
            cas_ptr->src_ptr += (VTSPR_NSAMPLES_10MS_8K >> 1);
            CAS_run(cas_ptr);
            detect2 = cas_ptr->detected;
            
            /* Determine CAS detection bits that have turned on */
            if (detect1 | detect2) {
                cidr_ptr->casLe = 1;
                cidr_ptr->casTe = 0;
            }
            else {
                if (cidr_ptr->casLe) {
                    cidr_ptr->casLe = 0;
                    cidr_ptr->casTe = 1;
                    cidr_ptr->cid2run = 1;
                    cidr_ptr->cid2count = 0;
                }
            }
        }
       
        /*
         * CAS has stopped.
         */
        if (cidr_ptr->cid2run) {
            tic_ptr  = _VTSPR_infcToTicPtr(dsp_ptr, infc);
            if (0 == cidr_ptr->cid2count) {
                /*
                 * Disable echo canceller. This is important.
                 */
                _VTSPR_algStateChan(dsp_ptr, infc,
                        VTSPR_ALG_CHAN_ECSR | VTSPR_ALG_CHAN_NLP, 0);
            }
            if (1 == cidr_ptr->cid2count) {
                /*
                 * Detect parallel phone. This takes time, results available in
                 * next 10 ms.
                 */
                TIC_getStatus(tic_ptr, TIC_GET_STATUS_PARALLEL_PHONE_START);
            }
            else if (2 == cidr_ptr->cid2count) {
                /*
                 * If a parallel phone is detected, do not ACK and abort CID
                 */
                if (TIC_PARALLEL_OFFHOOK == TIC_getStatus(tic_ptr,
                        TIC_GET_STATUS_PARALLEL_PHONE)) {
                    _VTSPR_algStateChan(dsp_ptr, infc, 0,
                            VTSPR_ALG_CHAN_ECSR | VTSPR_ALG_CHAN_NLP);
                    TIC_getStatus(tic_ptr,
                            TIC_GET_STATUS_PARALLEL_PHONE_END);
                    cidr_ptr->cid2run = 0;
                }
                else
                {
                    /*
                     * Do ACK.
                     */
                    toneSeq_ptr = &chan_ptr->toneSeq;
                    toneSeq_ptr->toneNumToneIds = 1;
                    toneSeq_ptr->toneControl = !VTSP_TONE_BREAK_MIX;
                    toneSeq_ptr->toneSeqRepeat = 1;
                    toneSeq_ptr->toneTime = 1000;
                    toneSeq_ptr->toneSeqIndex = 0; /*XXX check this change */
                    toneSeq_ptr->toneSeqRepeatCnt = 0;
                    toneSeq_ptr->tonePreRetVal = 0;
                    toneSeq_ptr->toneRetVal = 0;
                    toneSeq_ptr->toneIdList[0] = VTSPR_TONE_ID_CIDACK;
                    tId = toneSeq_ptr->toneIdList[toneSeq_ptr->toneSeqIndex];
                    dsp_ptr->toneParams_ptr[tId]->repeat = 1;
                    _VTSPR_algStateChan(dsp_ptr, infc, 0, VTSPR_ALG_CHAN_TONE); 
                }
            }
            else if (8 == cidr_ptr->cid2count) {
                /*
                 * Stop ACK (60 ms) and start receiving FSKR.
                 */
                _VTSPR_algStateChan(dsp_ptr, infc, VTSPR_ALG_CHAN_TONE,
                        VTSPR_ALG_CHAN_FSKR_OFH);
            }
            else if ((120 == cidr_ptr->cid2count) || fskrTe) {
                /*
                 * Timeout / TE on FSKR. Stop it.
                 */
                _VTSPR_algStateChan(dsp_ptr, infc, VTSPR_ALG_CHAN_FSKR_OFH,
                        VTSPR_ALG_CHAN_ECSR | VTSPR_ALG_CHAN_NLP);
                cidr_ptr->cid2run = 0;
                TIC_getStatus(tic_ptr, TIC_GET_STATUS_PARALLEL_PHONE_END);
            }
            /*
             * Mute data to far end.
             */
            COMM_fill(chan_ptr->ecNlp_ary, 0, chan_ptr->numSamples10ms);
            cidr_ptr->cid2count++;
        }
#endif
    }
#endif /* FXO */


    /* 
     * All Physical Fx infc audio channel Processing
     */
    _VTSPR_FOR_ALL_INFC(infc) {
        chan_ptr = _VTSPR_infcToChanPtr(dsp_ptr, infc);
#if defined(VTSP_ENABLE_DTMF) || defined(VTSP_ENABLE_FMTD) || \
        defined(VTSP_ENABLE_FXO) || defined(VTSP_ENABLE_CIDR)
        mask     = _VTSPR_getAlgStateChan(chan_ptr, infc);
#endif
        /* 
         * Generate Tone to the Near End
         * Dual tone or Genf tone
         */
        _VTSPR_audioNearTone(dsp_ptr, infc, chan_ptr);
#ifdef VTSP_ENABLE_STREAM_16K
        /* 
         * Do the upsampling all the time in this option.
         *
         * XXX 
         * when we split this info NB/WB audio interface
         * processing, then there is no upsampling here.
         * XXX
         */
        if (VTSPR_NSAMPLES_10MS_8K == chan_ptr->numSamples10ms) {
            _VTSPR_upSample(&chan_ptr->udsAudioUp,
                    chan_ptr->audioToPeer_ary, chan_ptr->ecNlp_ary);
        }
        else {
            COMM_copy(chan_ptr->audioToPeer_ary, chan_ptr->ecNlp_ary,
                    VTSPR_NSAMPLES_STREAM);
        }
#else
        COMM_copy(chan_ptr->audioToPeer_ary, chan_ptr->ecNlp_ary,
                VTSPR_NSAMPLES_STREAM);
#endif /* end VTSP_ENABLE_STREAM_16K */
    }
}

/*
 * ======== _VTSPR_audioPreFilter() ========
 * Pre-filter audio for ecNlp_ary audio gain and noise floor measurement
 *
 * Bug 2606 comment 33
 * !!! Important Note !!!
 * In this function, NFE only supports narrowband. Thus, wideband interfaces 
 * are only measuring noise on half of the buffer.
 * In the future, NFE here should be replaced with BND.
 */
void _VTSPR_audioPreFilter(
    VTSPR_DSP   *dsp_ptr)
{
    VTSPR_ChanObj  *chan_ptr;
    vint            infc;
    uint32          mask;

    _VTSPR_FOR_ALL_INFC(infc) {
        chan_ptr = _VTSPR_infcToChanPtr(dsp_ptr, infc);
        mask     = chan_ptr->algChannelState;       

        /* 
         * Run NFE on ecSout_ary, because we don't want to measure noise from
         * NLP output.
         * This is strange for systems with no ECSR, which do not use ecSout_ary
         */
#ifdef VTSP_ENABLE_BENCHMARK
       _VTSPR_benchmarkStart(&VTSPR_obj, _VTSPR_BENCHMARK_NFE, 1);
#endif
#ifdef VTSP_ENABLE_NFE
        /*
         * Run NFE on audioIn_ary.
         */
        if (0 != (VTSPR_ALG_CHAN_NFE_INFC & mask)) {
            NFE_run(&chan_ptr->nfeInfcObj);
        }
        /*
         * Run NFE on audioOut_ary.
         */
        if (0 != (VTSPR_ALG_CHAN_NFE_PEER & mask)) {
            NFE_run(&chan_ptr->nfePeerObj);
        }
#else
#ifndef VTSP_ENABLE_AEC
        /*
         * Run BND on audioOut_ary.
         */
        if (0 != (VTSPR_ALG_CHAN_NFE_PEER & mask)) {
            BND_run(chan_ptr->bndPeer_ptr);
        }
#endif
#endif
#ifdef VTSP_ENABLE_BENCHMARK
       _VTSPR_benchmarkStop(&VTSPR_obj, _VTSPR_BENCHMARK_NFE, 1);
#endif
        /* 
         * Apply software gain to ecNlp_ary
         */
        if (0 != chan_ptr->gainAudioIn) {        
            _VTSPR_audioGain(chan_ptr->ecNlp_ary, chan_ptr->gainAudioIn,
                    chan_ptr->numSamples10ms);
        }

        /* 
         * Audio Mute
         * Zero audio in from the phone
         */
        if (0 != (VTSPR_ALG_CHAN_MUTE_IN & mask)) { 
            COMM_fill(chan_ptr->ecNlp_ary, 0, chan_ptr->numSamples10ms);
        }
    }
}

/*
 * ======== _VTSPR_audioTxDcrm() ========
 * Run DCRM on audioOut_ary
 */
void _VTSPR_audioTxDcrm(
    VTSPR_Obj   *vtspr_ptr,        
    VTSPR_DSP   *dsp_ptr)
{
    VTSPR_ChanObj  *chan_ptr;
    vint            infc;
    uint32          mask;

    _VTSPR_FOR_ALL_INFC(infc) {

        chan_ptr = _VTSPR_infcToChanPtr(dsp_ptr, infc);
        mask     = _VTSPR_getAlgStateChan(chan_ptr, infc);
        /*
         * Run DCRM peer.
         * Some soft phones have DC in their signals.
         * This DC value can cause the NFE peer to train to high background
         * noise levels. During discontinuous transmission, NFE's high noise
         * floor value is used erroneously to generate comfort noise at DC
         * level. This could sound very loud. Removing DC enables NFE to train
         * to true noise floor and NSE to generate correct comfort noise.
         */
#ifdef VTSP_ENABLE_BENCHMARK
       _VTSPR_benchmarkStart(vtspr_ptr, _VTSPR_BENCHMARK_DCRM_PEER, 1);
#endif
        if (0 != (VTSPR_ALG_CHAN_DCRM_PEER & mask)) {
            DCRM_run(&chan_ptr->dcrmPeerObj);
        }
#ifdef VTSP_ENABLE_BENCHMARK
       _VTSPR_benchmarkStop(vtspr_ptr, _VTSPR_BENCHMARK_DCRM_PEER, 1);
#endif
    }
}
#endif /* VTSP_ENABLE_MP_LITE. */

