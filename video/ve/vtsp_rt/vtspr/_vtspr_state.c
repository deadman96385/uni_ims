/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004-2010 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 30131 $ $Date: 2014-12-01 14:45:03 +0800 (Mon, 01 Dec 2014) $
 *
 */

/*
 * This file handles commands sent to the driver which are pending
 * in the down-queue
 */
#include "vtspr.h"
#include "_vtspr_private.h"

/*
 * ======== _VTSPR_initUtdTranslate() ========
 *
 * Translates and initializes the UTD parameter table.
 * Note: Read the UTD man page prior to modifying this code.
 */
void _VTSPR_utdTranslate(
    VTSPR_DSP        *dsp_ptr)
{
#ifdef VTSP_ENABLE_UTD
    vint       tId;
    /*
     * Translate params.
     * If the Params structure words are zero, UTD will skip the translate.
     */
    for (tId = 0; tId < _VTSP_NUM_UTD_TEMPL; tId++) {
        UTD_translate(dsp_ptr->utdParams_ptr[tId],
                dsp_ptr->utdTransData_ptr[tId]);
    }
    /*
     * Set up the table, for all initialized tones.
     */
    for (tId = 0; tId < _VTSP_NUM_UTD_TEMPL; tId++) {
        dsp_ptr->utdTransTable_ptr[tId] = dsp_ptr->utdTransData_ptr[tId];
    }
    /*
     * The last member in the list must point to a dummy word of zero,
     * unless the length of the list is exactly 15.
     */
    dsp_ptr->utdZero[0] = 0;
    if (dsp_ptr->utdParamNum < _VTSP_NUM_UTD_TEMPL) {
        dsp_ptr->utdTransTable_ptr[dsp_ptr->utdParamNum] = dsp_ptr->utdZero;
    }
#endif
}

/*
 * ======== _VTSPR_initCoder() ========
 *
 * Inits a coder.
 */
void _VTSPR_initCoder(
    VTSPR_DSP       *dsp_ptr,
    VTSPR_StreamObj *stream_ptr,
    vint             encDec,
    vint             type)
{
#if defined(VTSP_ENABLE_G711P1) && !defined(VTSP_ENABLE_G711P1_ACCELERATOR)
    uint32 extension;
    vint   speechFormat;
    vint   bitRate;
#endif
#if defined(VTSP_ENABLE_GAMRNB_ACCELERATOR) || \
        defined(VTSP_ENABLE_GAMRWB_ACCELERATOR)
    uint32  dspInitParam;
#endif

    switch (type) {
        case VTSP_CODER_G711A:
        case VTSP_CODER_G711U:
        case VTSP_CODER_CN:
#ifdef VTSP_ENABLE_16K_MU
        case VTSP_CODER_16K_MU:
#endif /* end VTSP_ENABLE_16K_MU */
            stream_ptr->cnPower = 0;
            break;

#if defined(VTSP_ENABLE_G726) && !defined(VTSP_ENABLE_G726_ACCELERATOR)
        case VTSP_CODER_G726_32K:
            if (VTSPR_DECODER == encDec) {
                G726_init(&stream_ptr->g726DecObj, G726_32KBPS);
            }
            else {
                G726_init(&stream_ptr->g726EncObj, G726_32KBPS);
                stream_ptr->cnPower = 0;
            }
            break;
#endif

#if defined(VTSP_ENABLE_G729) && !defined(VTSP_ENABLE_G729_ACCELERATOR)
        case VTSP_CODER_G729:
            if (VTSPR_DECODER == encDec) {
                G729AB_decode_init(&stream_ptr->g729DecObj);
            }
            else {
                G729AB_encode_init(&stream_ptr->g729EncObj);
            }
            break;
#endif

#if defined(VTSP_ENABLE_G711P1) && !defined(VTSP_ENABLE_G711P1_ACCELERATOR)
        case VTSP_CODER_G711P1U:
        case VTSP_CODER_G711P1A:
            /*
             * Get G711P1 mode from extension
             */
            extension = stream_ptr->streamParam.extension;
            if (0 != (extension & VTSP_MASK_EXT_G711P1_R3_96)) {
                bitRate = G711P1_MODE_R3;
            }
            else if (0 != (extension & VTSP_MASK_EXT_G711P1_R2A_80)) {
                bitRate = G711P1_MODE_R2A;
            }
            else if (0 != (extension & VTSP_MASK_EXT_G711P1_R2B_80)) {
                bitRate = G711P1_MODE_R2B;
            }
            else if (0 != (extension & VTSP_MASK_EXT_G711P1_R1_64)) {
                bitRate = G711P1_MODE_R1;
            }
            else {
                OSAL_logMsg("ERROR: G711P1 bad extension");
            }
            /*
             * Get speech format from coder type
             */
            if (VTSP_CODER_G711P1U == type) {
                speechFormat = G711P1_SPEECH_FORMAT_ULAW;
            } else {
                speechFormat = G711P1_SPEECH_FORMAT_ALAW;
            }
            if (VTSPR_DECODER == encDec) {
                G711P1_decodeInit(&stream_ptr->g711p1DecObj, 
                        bitRate, G711P1_SPEECH_FORMAT_ULAW);
            }
            else {
                G711P1_encodeInit(&stream_ptr->g711p1EncObj,
                        G711P1_SAMPLE_RATE_16K, bitRate,
                        G711P1_SPEECH_FORMAT_ULAW);
            }
            break;
#endif

#ifdef VTSP_ENABLE_G722
        case VTSP_CODER_G722:
            if (VTSPR_DECODER == encDec) {
                g722_decodeInit(&stream_ptr->g722DecObj);
            }
            else {
                g722_encodeInit(&stream_ptr->g722EncObj);
            }
            break;
#endif
        /* Multi-frame encoders init below */    
#if defined(VTSP_ENABLE_ILBC) || defined(VTSP_ENABLE_G723) || \
    defined(VTSP_ENABLE_G722P1) || defined(VTSP_ENABLE_GAMRNB) || \
    defined(VTSP_ENABLE_GAMRWB) || defined(VTSP_ENABLE_SILK) || \
    defined(VTSP_ENABLE_G729_ACCELERATOR) || \
    defined(VTSP_ENABLE_G726_ACCELERATOR)
#ifdef VTSP_ENABLE_ILBC
        case VTSP_CODER_ILBC_20MS:
        case VTSP_CODER_ILBC_30MS:
#endif            
#ifdef VTSP_ENABLE_G723
        case VTSP_CODER_G723_30MS:
#endif
#ifdef VTSP_ENABLE_G722P1
        case VTSP_CODER_G722P1_20MS:
#endif
#ifdef VTSP_ENABLE_GAMRNB
        case VTSP_CODER_GAMRNB_20MS_OA:
        case VTSP_CODER_GAMRNB_20MS_BE:
#endif
#ifdef VTSP_ENABLE_GAMRWB
        case VTSP_CODER_GAMRWB_20MS_OA:
        case VTSP_CODER_GAMRWB_20MS_BE:
#endif
#ifdef VTSP_ENABLE_SILK
        case VTSP_CODER_SILK_20MS_8K:
        case VTSP_CODER_SILK_20MS_16K:
        case VTSP_CODER_SILK_20MS_24K:
#endif
#ifdef VTSP_ENABLE_G729_ACCELERATOR
        case VTSP_CODER_G729:
#endif
#ifdef VTSP_ENABLE_G726_ACCELERATOR
        case VTSP_CODER_G726_32K:
#endif
            _VTSPR_multiCoderInit(stream_ptr, encDec, type);
            break;
#endif

#ifdef VTSP_ENABLE_T38
        case VTSP_CODER_T38:
            FR38_init(stream_ptr->fr38Obj_ptr, dsp_ptr->fr38Global_ptr,
                    dsp_ptr->fr38Params_ptr, stream_ptr->fr38Jb_ptr,
                    NULL, (unsigned short *)stream_ptr->fr38Mdm_ptr);
            break;
#endif
#ifdef VTSP_ENABLE_GAMRNB_ACCELERATOR
        case VTSP_CODER_GAMRNB_20MS_OA:
        case VTSP_CODER_GAMRNB_20MS_BE:
            dspInitParam = 0;
            /* Get bit rate */
            if (0 != (stream_ptr->streamParam.extension &
                    VTSP_MASK_EXT_GAMRNB_20MS_475)) {
                dspInitParam = DSP_GAMRNB_BITRATE_475;
            }
            else if (0 != (stream_ptr->streamParam.extension &
                    VTSP_MASK_EXT_GAMRNB_20MS_515)) {
                dspInitParam = DSP_GAMRNB_BITRATE_515;
            }
            else if (0 != (stream_ptr->streamParam.extension &
                    VTSP_MASK_EXT_GAMRNB_20MS_59)) {
                dspInitParam = DSP_GAMRNB_BITRATE_590;
            }
            else if (0 != (stream_ptr->streamParam.extension &
                    VTSP_MASK_EXT_GAMRNB_20MS_67)) {
                dspInitParam = DSP_GAMRNB_BITRATE_670;
            }
            else if (0 != (stream_ptr->streamParam.extension &
                    VTSP_MASK_EXT_GAMRNB_20MS_74)) {
                dspInitParam = DSP_GAMRNB_BITRATE_740;
            }
            else if (0 != (stream_ptr->streamParam.extension &
                    VTSP_MASK_EXT_GAMRNB_20MS_795)) {
                dspInitParam = DSP_GAMRNB_BITRATE_795;
            }
            else if (0 != (stream_ptr->streamParam.extension &
                    VTSP_MASK_EXT_GAMRNB_20MS_102)) {
                dspInitParam = DSP_GAMRNB_BITRATE_1020;
            }
            else {
                /* user 12.2 as default rate */
                dspInitParam = DSP_GAMRNB_BITRATE_1220;
            }

            if (VTSP_CODER_GAMRNB_20MS_OA == type) {
                /* get Vad */
                if (0 != (stream_ptr->streamParam.silenceComp &
                        VTSP_MASK_CODER_GAMRNB_20MS_OA)) {
                    dspInitParam |= DSP_VAD_ENABLE;
                }
                /* Init */
                if (VTSPR_DECODER == encDec) {
                    DSP_decodeInit(stream_ptr->multiDecObj.decGamrnbInstance,
                            DSP_CODER_TYPE_AMRNB_OA, dspInitParam);
                }
                else {
                    DSP_encodeInit(stream_ptr->multiDecObj.decGamrnbInstance,
                            DSP_CODER_TYPE_AMRNB_OA, dspInitParam);
                }
            }
            else {
                /* get Vad */
                if (0 != (stream_ptr->streamParam.silenceComp &
                        VTSP_MASK_CODER_GAMRNB_20MS_BE)) {
                    dspInitParam |= DSP_VAD_ENABLE;
                }
                /* Init */
                if (VTSPR_DECODER == encDec) {
                    DSP_decodeInit(stream_ptr->multiDecObj.decGamrnbInstance,
                            DSP_CODER_TYPE_AMRNB_BE, dspInitParam);
                }
                else {
                    DSP_encodeInit(stream_ptr->multiDecObj.decGamrnbInstance,
                            DSP_CODER_TYPE_AMRNB_BE, dspInitParam);
                }
            }
            break;
#endif
#ifdef VTSP_ENABLE_GAMRWB_ACCELERATOR
        case VTSP_CODER_GAMRWB_20MS_OA:
        case VTSP_CODER_GAMRWB_20MS_BE:
            dspInitParam = 0;
            /* Get bit rate */
            if (0 != (stream_ptr->streamParam.extension &
                    VTSP_MASK_EXT_GAMRWB_20MS_660)) {
                dspInitParam = DSP_GAMRWB_BITRATE_660;
            }
            else if (0 != (stream_ptr->streamParam.extension &
                    VTSP_MASK_EXT_GAMRWB_20MS_885)) {
                dspInitParam = DSP_GAMRWB_BITRATE_885;
            }
            else if (0 != (stream_ptr->streamParam.extension &
                    VTSP_MASK_EXT_GAMRWB_20MS_1265)) {
                dspInitParam = DSP_GAMRWB_BITRATE_1265;
            }
            else if (0 != (stream_ptr->streamParam.extension &
                    VTSP_MASK_EXT_GAMRWB_20MS_1425)) {
                dspInitParam = DSP_GAMRWB_BITRATE_1425;
            }
            else if (0 != (stream_ptr->streamParam.extension &
                    VTSP_MASK_EXT_GAMRWB_20MS_1585)) {
                dspInitParam = DSP_GAMRWB_BITRATE_1585;
            }
            else if (0 != (stream_ptr->streamParam.extension &
                    VTSP_MASK_EXT_GAMRWB_20MS_1825)) {
                dspInitParam = DSP_GAMRWB_BITRATE_1825;
            }
            else if (0 != (stream_ptr->streamParam.extension &
                    VTSP_MASK_EXT_GAMRWB_20MS_1985)) {
                dspInitParam = DSP_GAMRWB_BITRATE_1985;
            }
            else if (0 != (stream_ptr->streamParam.extension &
                    VTSP_MASK_EXT_GAMRWB_20MS_2305)) {
                dspInitParam = DSP_GAMRWB_BITRATE_2305;
            }
            else {
                dspInitParam = DSP_GAMRWB_BITRATE_2385;
            }

            if (VTSP_CODER_GAMRWB_20MS_OA == type) {
                /* get Vad */
                if (0 != (stream_ptr->streamParam.silenceComp &
                        VTSP_MASK_CODER_GAMRWB_20MS_OA)) {
                    dspInitParam |= DSP_VAD_ENABLE;
                }
                /* Init */
                if (VTSPR_DECODER == encDec) {
                    DSP_decodeInit(stream_ptr->multiDecObj.decGamrwbInstance,
                            DSP_CODER_TYPE_AMRWB_OA, dspInitParam);
                }
                else {
                    DSP_encodeInit(stream_ptr->multiDecObj.decGamrwbInstance,
                            DSP_CODER_TYPE_AMRWB_OA, dspInitParam);
                }
            }
            else {
                /* get Vad */
                if (0 != (stream_ptr->streamParam.silenceComp &
                        VTSP_MASK_CODER_GAMRWB_20MS_BE)) {
                    dspInitParam |= DSP_VAD_ENABLE;
                }
                /* Init */
                if (VTSPR_DECODER == encDec) {
                    DSP_decodeInit(stream_ptr->multiDecObj.decGamrwbInstance,
                            DSP_CODER_TYPE_AMRWB_BE, dspInitParam);
                }
                else {
                    DSP_encodeInit(stream_ptr->multiDecObj.decGamrwbInstance,
                            DSP_CODER_TYPE_AMRWB_BE, dspInitParam);
                }
            }
            break;
#endif
        default:
            break;

    }
}

/*
 * ======== _VTSPR_getAlgStateChan ========
 * Return the algorithm state on the specified channel.
 * Handles all types of channels.
 */
OSAL_INLINE uint32 _VTSPR_getAlgStateChan(
    VTSPR_ChanObj *chan_ptr,
    uvint          infc)
{
    uint32 algChannelState;

    algChannelState = chan_ptr->algChannelState;

    return(algChannelState);
}

/*
 * ======== _VTSPR_algStateChan ========
 * Set/Reset algorithm state for interface channel processing
 * and initialize algorithms as necessary
 * Handles all types of channels.
 */
void _VTSPR_algStateChan(
    VTSPR_DSP        *dsp_ptr,
    vint              infc,
    uint32            clearMask,
    uint32            setMask)
{
    VTSPR_ChanObj    *chan_ptr;
    GLOBAL_Params    *globals_ptr;
#ifdef VTSP_ENABLE_ECSR
    _VTSPR_EcObj     *ec_ptr;
    EC_Obj           *ecsr_ptr;
    NLP_Obj          *nlp_ptr;
    vint             *rin_ptr;
    vint             *sin_ptr;
#endif
#if defined(VTSP_ENABLE_ECSR) || defined(VTSP_ENABLE_CIDR) || \
    defined(VTSP_ENABLE_DTMF) || defined(VTSP_ENABLE_FMTD)
    vint             *nlpOut_ptr;
#endif
#ifndef VTSP_ENABLE_MP_LITE
    TONE_Obj         *tone_ptr;
#endif
#ifdef VTSP_ENABLE_NFE
    NFE_Object       *nfe_ptr;
#endif
#ifndef VTSP_ENABLE_MP_LITE
    DCRM_Obj         *dcrm_ptr;
#endif
#ifdef VTSP_ENABLE_DTMF
    _VTSPR_DtmfObj   *dtmf_ptr;
    DTMF_Obj         *dtmfObj_ptr;
#endif
#ifdef VTSP_ENABLE_FMTD
    GLOBAL_Params     fmtdGlobals;
    _VTSPR_FmtdObj   *fmtd_ptr;
    FMTD_Obj         *fmtdObj_ptr;
    vint              fmtdScale;
#endif
#ifdef VTSP_ENABLE_TONE_QUAD
    GENF_Obj         *toneQuad_ptr;
#endif
#ifdef VTSP_ENABLE_UTD
    _VTSPR_UtdObj    *utd_ptr;
    UTD_Obj          *utdObj_ptr;
    vint              utdSaveMask;
#endif
#ifdef VTSP_ENABLE_CIDS
    _VTSPR_CidsObj   *cids_ptr;
#endif
#ifdef VTSP_ENABLE_T38
    _VTSPR_FR38Obj   *fr38_ptr;
#endif
#ifdef VTSP_ENABLE_CIDR
    _VTSPR_CidrObj   *cidr_ptr;
    CAS_Obj          *cas_ptr;
    FSKR_Obj         *fskr_ptr;
#endif
#ifndef VTSP_ENABLE_MP_LITE
    TONE_Params      *params_ptr;
    VTSPR_ToneSeq    *toneSeq_ptr;
    vint              tId;
#endif
#if defined(VTSP_ENABLE_ECSR) || defined(VTSP_ENABLE_NFE)
    vint             *sout_ptr;
#endif
    uint32            oldState;
    uint32            newState;
    uint32            changeMask;
    uint32            onMask;
    uint32            offMask;
#if defined(VTSP_ENABLE_STREAM_16K) || defined(VTSP_ENABLE_AUDIO_SRC)
#ifndef VTSP_ENABLE_MP_LITE
    vint              lowRate;
    vint              highRate;
#endif
#endif

    chan_ptr = _VTSPR_infcToChanPtr(dsp_ptr, infc);
    oldState = chan_ptr->algChannelState;
    newState = oldState | setMask;
    newState = newState & (~clearMask);

    /*
     * Alg pointers, connections.
     */
#ifdef VTSP_ENABLE_ECSR
    sin_ptr     = chan_ptr->audioIn_ary;
#endif
    /* 
     * ecsr, nlp, and nfe all require 8 K data, from sout_ptr.
     * dtmf, fmtd, cas, fskr, all require 8 K data, from nlpOut_ptr.
     * nlp and DTMF may write data, so be careful at run time.
     * Their pointers are set up at init time, but the 8K data must be valid
     * at run time. Therefore, call _get8ReadPtr() at run time.
     */
#if defined(VTSP_ENABLE_ECSR) || defined(VTSP_ENABLE_NFE)
    sout_ptr    = chan_ptr->ecSout_ary;
#endif
#if defined(VTSP_ENABLE_ECSR) || defined(VTSP_ENABLE_CIDR) || \
    defined(VTSP_ENABLE_DTMF) || defined(VTSP_ENABLE_FMTD)
    nlpOut_ptr  = chan_ptr->ecNlp_ary;
#endif

    globals_ptr = dsp_ptr->globals_ptr;

    /* Check for resource conflicts
     */
    if ((0 != (newState & ( VTSPR_ALG_CHAN_CIDS | VTSPR_ALG_CHAN_CIDCWS))) &&
            (0 != ((newState | clearMask) & VTSPR_ALG_CHAN_TONE) ||
            (0 != ((newState | clearMask) & VTSPR_ALG_CHAN_TONE_QUAD)))) { 
#ifndef VTSP_ENABLE_MP_LITE
        /*
         * CIDS or CIDCWS or is currently running (or turning on),
         * and
         * TONE or QUAD is turning currently running (or turning on), or
         * turning off.
         *
         * CIDS & CIDCWS have priority over TONE.
         * Therefore, in this case TONE is halted.
         * Application must wait for CID Complete/Halted event
         * prior to starting Tone
         */
        if (0 == (VTSPR_ALG_CHAN_TONE & clearMask)) {
            /*
             * Not trying to clear the tone generation state. Thus, tone
             * generation is either on, or being turned on, so send the
             * halted event
             */
            chan_ptr->toneSeq.toneEvent = VTSP_EVENT_HALTED;
        }
#endif
        if (0 == (VTSPR_ALG_CHAN_TONE_QUAD & clearMask)) {
#ifdef VTSP_ENABLE_TONE_QUAD            
            chan_ptr->toneQuadSeq.toneEvent = VTSP_EVENT_HALTED;
#endif
        }
        /* Disable the tone generation algorithm state */
        setMask &= ~(VTSPR_ALG_CHAN_TONE | VTSPR_ALG_CHAN_TONE_QUAD);
        clearMask &= ~(VTSPR_ALG_CHAN_TONE | VTSPR_ALG_CHAN_TONE_QUAD);
        newState &= ~(VTSPR_ALG_CHAN_TONE | VTSPR_ALG_CHAN_TONE_QUAD);
    }

    if ((0 != (newState & VTSPR_ALG_CHAN_DR_DEC)) &&
            (0 != (newState & VTSPR_ALG_CHAN_TONE) || 
            (0 != (newState & VTSPR_ALG_CHAN_TONE_QUAD)))) { 
        /* 
         * New state conflict: DTMF relay and tone generate.
         * Terminate DTMF relay decoding in this instance.
         */
        clearMask &= ~(VTSPR_ALG_CHAN_DR_DEC);
    }
    if ((0 != (newState & VTSPR_ALG_CHAN_TONE_QUAD)) &&
            (0 != (newState & VTSPR_ALG_CHAN_TONE))) { 
        /* 
         * State conflict: Tone and Tone Quad.
         * Terminate Tone in this instance.
         */
        clearMask &= ~(VTSPR_ALG_CHAN_TONE);
    }

    /*
     * Algs that have turned on or off
     */
    changeMask  = oldState ^ newState;
    /*
     * Algs that have turned on
     */
    onMask      = changeMask & newState;
    /*
     * Algs that have turned off
     */
    offMask     = changeMask & oldState;

    /* 
     * State logic for ALL infc types 
     */

#ifndef VTSP_ENABLE_MP_LITE
    /*
     * For tone use setMask. Tone may need to be initialized in the middle of
     * old tone.
     */
    if (0 != (setMask & VTSPR_ALG_CHAN_TONE)) {

        /*
         * Tone to infc, set toneIds[] in channel object before TONE_init().
         * Start with the first toneId.
         */
        toneSeq_ptr = &chan_ptr->toneSeq;
        tId = toneSeq_ptr->toneIdList[toneSeq_ptr->toneSeqIndex];
        params_ptr = dsp_ptr->toneParams_ptr[tId];
        tone_ptr = toneSeq_ptr->toneObj_ptr;

        if (VTSPR_NSAMPLES_10MS_16K == chan_ptr->numSamples10ms) {
            params_ptr->ctrlWord |= TONE_SAMPLERATE_16K; /* 160 samples */
        }
        else {
            params_ptr->ctrlWord &= (~TONE_SAMPLERATE_16K); /* 80 samples */
        }
        TONE_init(tone_ptr, globals_ptr, params_ptr);
        tone_ptr->dst_ptr = chan_ptr->toneOut_ary;
        if (0 != (VTSPR_ALG_CHAN_TONE & oldState)) {
            /* Halting tone generation, send the halted event */
            toneSeq_ptr->toneEvent = VTSP_EVENT_HALTED;
        }
    }
#endif
#ifdef VTSP_ENABLE_TONE_QUAD
    if (0 != (setMask & VTSPR_ALG_CHAN_TONE_QUAD)) { 
        /*
         * Tone to infc, set toneIdList[] in channel object before TONE_init().
         * Start with the first toneId.
         */
        toneSeq_ptr = &chan_ptr->toneQuadSeq; 
        tId = toneSeq_ptr->toneIdList[toneSeq_ptr->toneSeqIndex];

        /* XXX May need to add support for 'warm' argument to GENF_init() */
        toneQuad_ptr = toneSeq_ptr->toneObj_ptr;
        toneQuad_ptr->dst_ptr = chan_ptr->toneOut_ary;
        GENF_init(toneQuad_ptr, globals_ptr,
                dsp_ptr->toneQuadParams_ptr[tId], 0);
        if (0 != (VTSPR_ALG_CHAN_TONE_QUAD & oldState)) {
            /* Halting tone generation, send the halted command */
            toneSeq_ptr->toneEvent = VTSP_EVENT_HALTED;
        }
    }
#endif
    if (0 != (clearMask & VTSPR_ALG_CHAN_TONE)) { 
        /*
         * Command to turn off the tone.
         * Clear the tone destination buffer of previous tone data.
         * XXX: Replace with NSE turn on.
         */
        COMM_fill(chan_ptr->audioOut_ary, 0, chan_ptr->numSamples10ms);
#ifndef VTSP_ENABLE_MP_LITE
        if (0 != (VTSPR_ALG_CHAN_TONE & oldState)) {
            /* Halting tone generation, send the halted event */
            chan_ptr->toneSeq.toneEvent = VTSP_EVENT_HALTED;
        }
#endif
    }
#ifdef VTSP_ENABLE_TONE_QUAD
    if (0 != (clearMask & VTSPR_ALG_CHAN_TONE_QUAD)) { 
        /*
         * Command to turn off the tone.
         * Clear the tone destination buffer of previous tone data.
         */
        COMM_fill(chan_ptr->audioOut_ary, 0, chan_ptr->numSamples10ms);
        if (0 != (VTSPR_ALG_CHAN_TONE_QUAD & oldState)) {
            /* Halting tone generation, send the halted event */
            chan_ptr->toneQuadSeq.toneEvent = VTSP_EVENT_HALTED;
        }
    }
#endif
#ifndef VTSP_ENABLE_MP_LITE
    if (0 != (onMask & VTSPR_ALG_CHAN_DCRM_PEER)) {
        dcrm_ptr = &chan_ptr->dcrmPeerObj;
        DCRM_init(dcrm_ptr, dsp_ptr->dcrmPeerParams_ptr);
        dcrm_ptr->src_ptr = chan_ptr->audioOut_ary;
        dcrm_ptr->dst_ptr = chan_ptr->audioOut_ary;
    }
#endif
#ifdef VTSP_ENABLE_NFE
    if (0 != (onMask & VTSPR_ALG_CHAN_NFE_PEER)) {
        /*
         * Init NFE
         */
        nfe_ptr = &chan_ptr->nfePeerObj;
        NFE_init(nfe_ptr,  globals_ptr, dsp_ptr->nfeParams_ptr,
                NFE_WARM_START);
        nfe_ptr->src_ptr  = chan_ptr->audioOut_ary;
        nfe_ptr->xmit_ptr = NULL;
    }
#endif


    /* 
     * State logic for AUDIO infc types 
     */
  if (VTSPR_INFC_TYPE_AUDIO == _VTSPR_infcToType(infc)) {
#ifdef VTSP_ENABLE_AEC
    if (0 != (offMask & VTSPR_ALG_CHAN_AEC)) { 
        /* Set AEC to bypass mode */
        chan_ptr->aec_ptr->aecNearObj.control |= AEC_CONTROL_AEC_BYPASS;
    }
#endif
    if (0 != (onMask & VTSPR_ALG_CHAN_AEC)) {
#ifdef VTSP_ENABLE_AEC
        /*
         * Init AEC 
         */
        AEC_init(&chan_ptr->aec_ptr->aecNearObj, dsp_ptr->globals_ptr,
                dsp_ptr->aecNearParams_ptr, AEC_INIT_WARM);
#else
#ifndef VTSP_ENABLE_MP_LITE
        BND_init(chan_ptr->bndNear_ptr, dsp_ptr->globals_ptr, 
                dsp_ptr->bndParams_ptr, BND_WARM_START);
        BND_init(chan_ptr->bndPeer_ptr, dsp_ptr->globals_ptr, 
                dsp_ptr->bndParams_ptr, BND_WARM_START);
#endif
#endif
    }
    else if (0 != (onMask & VTSPR_ALG_CHAN_NFE_INFC)) {
#ifdef VTSP_ENABLE_NFE
        /*
         * Init just NFE but not ECSR and NFE
         */
        nfe_ptr = &chan_ptr->nfeInfcObj;
        NFE_init(nfe_ptr,  globals_ptr, dsp_ptr->nfeParams_ptr,
                NFE_WARM_START);
        nfe_ptr->src_ptr  = sout_ptr;
        nfe_ptr->xmit_ptr = NULL;
#endif
    }
  }

    /* 
     * State logic for FX infc types 
     */
  if (VTSPR_INFC_TYPE_AUDIO != _VTSPR_infcToType(infc)) {
#ifdef VTSP_ENABLE_ECSR
    ec_ptr      = chan_ptr->ec_ptr;
    rin_ptr     = ec_ptr->ecRin_ary[0];    /* Note RIN uses Delayed Buffer */
#endif
    if (0 != (onMask & VTSPR_ALG_CHAN_ECSR)) {
        /*
         * Init ECSR, or AEC
         * NLP, NFE
         */
#ifdef VTSP_ENABLE_ECSR
        ecsr_ptr = &ec_ptr->ecsrObj;
        ecsr_ptr->control = EC_ERL;
        ECSR_init(ecsr_ptr, globals_ptr->p0DBIN, dsp_ptr->ecsrParams_ptr,
                EC_WARMSTART);

        nlp_ptr = &ec_ptr->nlpObj;
        NLP_init(nlp_ptr, globals_ptr, dsp_ptr->nlpParams_ptr);
        nlp_ptr->control = NLP_CNTR_NFE;


        ecsr_ptr->sin_ptr  = sin_ptr;
        ecsr_ptr->rin_ptr  = rin_ptr;
        ecsr_ptr->sout_ptr = sout_ptr;
        nlp_ptr->dst_ptr   = nlpOut_ptr;
        nlp_ptr->src_ptr   = sout_ptr;
#endif /* VTSP_ENABLE_ECSR */
#ifdef VTSP_ENABLE_NFE
        nfe_ptr = &chan_ptr->nfeInfcObj;
        NFE_init(nfe_ptr,  globals_ptr, dsp_ptr->nfeParams_ptr, NFE_WARM_START);
        nfe_ptr->src_ptr  = sout_ptr;
        nfe_ptr->xmit_ptr = NULL;
#endif
    }
#ifdef VTSP_ENABLE_ECSR
    else if (0 != (onMask & VTSPR_ALG_CHAN_NLP)) {
        /*
         * Init just NLP but not ECSR and NLP
         */
        nlp_ptr = &ec_ptr->nlpObj;
        NLP_init(nlp_ptr, globals_ptr, dsp_ptr->nlpParams_ptr);
        nlp_ptr->control = NLP_CNTR_NFE;
        nlp_ptr->dst_ptr = nlpOut_ptr;
        nlp_ptr->src_ptr = sout_ptr;
    }
#endif
    else if (0 != (onMask & VTSPR_ALG_CHAN_NFE_INFC)) {
#ifdef VTSP_ENABLE_NFE
        /*
         * Init just NFE but not ECSR and NFE
         */
        nfe_ptr = &chan_ptr->nfeInfcObj;
        NFE_init(nfe_ptr,  globals_ptr, dsp_ptr->nfeParams_ptr,
                NFE_WARM_START);
        nfe_ptr->src_ptr  = sout_ptr;
        nfe_ptr->xmit_ptr = NULL;
#endif
    }
    else if (0 != (offMask & VTSPR_ALG_CHAN_ECSR)) { 
        /* Else if EC is turned off */
        /* Ensure Freeze bit is always cleared when EC is turned off. */
        newState &= ~VTSPR_ALG_CHAN_ECSR_FRZ;
    }

#ifdef VTSP_ENABLE_DTMF
    dtmf_ptr = chan_ptr->dtmf_ptr;
    if (0 != (onMask & VTSPR_ALG_CHAN_DTMF) ||
            (0 != (onMask & VTSPR_ALG_CHAN_CIDCWS))) {
        /*
         * Init DTMF detector.
         */
        dtmfObj_ptr = &dtmf_ptr->dtmfObj;
        DTMF_init(dtmfObj_ptr, globals_ptr, dsp_ptr->dtmfParams_ptr);
        dtmfObj_ptr->src_ptr = nlpOut_ptr;
        dtmf_ptr->dtmfRemove = 1;
    }

    if (0 != (onMask & VTSPR_ALG_CHAN_DTMF) ||
            (0 != (offMask & VTSPR_ALG_CHAN_DTMF)) ||
            (0 != (onMask & VTSPR_ALG_CHAN_CIDCWS)) ||
            (0 != (offMask & VTSPR_ALG_CHAN_CIDCWS))) {
        /* Init DTMF event state
         */
        /* When DTMF turns on or off, reset DTMF relay.
         */
#ifdef VTSP_ENABLE_DTMFR
        chan_ptr->drEventObj.stop = DR_STOP_SET;
#endif
        dtmf_ptr->dtmfDigit  = -1;
        dtmf_ptr->dtmfLe     = -1;
        dtmf_ptr->dtmfTe     = -1;
#ifdef VTSP_ENABLE_CIDR
        if (VTSPR_INFC_TYPE_FXO == _VTSPR_infcToType(infc)) {
            cidr_ptr = chan_ptr->cidr_ptr;
            cidr_ptr->dtmfCidCount = 0;
            cidr_ptr->dtmfCidEvt = 0;
            cidr_ptr->inDtmfCid = 0;
        }
#endif
    }
#endif

#ifdef VTSP_ENABLE_FMTD
    fmtd_ptr = chan_ptr->fmtd_ptr;
    if (0 != (onMask & VTSPR_ALG_CHAN_FMTD)) {
        /*
         * Init FMTD detector.
         */
        /* 
         * Bug 2073
         * User programmed parameter should be -20 to 0 dB from the default.
         */
        fmtdScale = COMM_MAX(-20, COMM_MIN(0, (dsp_ptr->fmtdGlobalPowerMinInfc 
                - VTSPR_FMTD_POWER_MIN_DETECT_DEF)));
        /* convert dB steps to 0.5 dB steps */
        fmtdScale = COMM_db2lin((fmtdScale << 1) + 40);
        fmtdScale = ((fmtdScale * globals_ptr->p0DBIN) >> 11);
        fmtdGlobals.p0DBIN = fmtdScale;
        fmtdGlobals.p0DBOUT = fmtdScale;

        fmtdObj_ptr = &fmtd_ptr->fmtdInfcObj;
        FMTD_init(fmtdObj_ptr, &fmtdGlobals, dsp_ptr->fmtdParams_ptr);
        fmtdObj_ptr->src_ptr = nlpOut_ptr;

        fmtdScale = COMM_MAX(-20, COMM_MIN(0, (dsp_ptr->fmtdGlobalPowerMinPeer 
               - VTSPR_FMTD_POWER_MIN_DETECT_DEF)));
        /* convert dB steps to 0.5 dB steps */
        fmtdScale = COMM_db2lin((fmtdScale << 1) + 40);
        fmtdScale = ((fmtdScale * globals_ptr->p0DBIN) >> 11);
        fmtdGlobals.p0DBIN = fmtdScale;
        fmtdGlobals.p0DBOUT = fmtdScale;

        fmtdObj_ptr = &fmtd_ptr->fmtdPeerObj;
        FMTD_init(fmtdObj_ptr, &fmtdGlobals, dsp_ptr->fmtdParams_ptr);
        fmtdObj_ptr->src_ptr = chan_ptr->audioOut_ary;
    }
    if (0 != (onMask & VTSPR_ALG_CHAN_FMTD) || 
            0 != (offMask & VTSPR_ALG_CHAN_FMTD) ||
            0 != (offMask & VTSPR_ALG_CHAN_FAX)) {
        /* Init FMTD event state when:
         *  FMTD detector is turned on or off.
         */
        fmtd_ptr->fmtdLeInfc = 0;
        fmtd_ptr->fmtdLePeer = 0;
        fmtd_ptr->fmtdTeInfc = 0;
        fmtd_ptr->fmtdTePeer = 0;
        fmtd_ptr->fmtdPhaseRevInfc = 0;
        fmtd_ptr->fmtdPhaseRevPeer = 0;
        fmtd_ptr->fmtdSilenceTime = 0;
        newState &= ~(VTSPR_ALG_CHAN_FAX); /* turn off Fax clear channel */
    }
#endif
  }
#if defined(VTSP_ENABLE_DTMFR) && !defined(VTSP_ENABLE_MP_LITE)
    if ((0 != (onMask & VTSPR_ALG_CHAN_DR_DEC)) ||
            (0 != (offMask & VTSPR_ALG_CHAN_DR_DEC))) {
        /* Init DTMF Decode Tone state
         */
        DR_decodeInit(&chan_ptr->drDecodeObj);
    }
#endif

#ifdef VTSP_ENABLE_UTD
    if (VTSPR_INFC_TYPE_FXO == _VTSPR_infcToType(infc)) {
        utd_ptr = chan_ptr->utd_ptr;    
        if (0 != (onMask & VTSPR_ALG_CHAN_UTD)) {
            /*
             * Init UTD
             */
            utdObj_ptr = &utd_ptr->utdObj;
            /*
             * Note: UTD_init() always resets detectionMask.
             * Save & restore the detectionMask outside of UTD_init.
             */
            utdSaveMask = utdObj_ptr->detectionMask;
            UTD_init(utdObj_ptr, dsp_ptr->utdTransTable_ptr, globals_ptr);
            utdObj_ptr->detectionMask = utdSaveMask;
            utd_ptr->utdLe = 0;
            utd_ptr->utdTe = 0;
        }
        if (0 != (onMask & VTSPR_ALG_CHAN_UTD) ||
                0 != (offMask & VTSPR_ALG_CHAN_UTD)) {
            utd_ptr->utdLe = 0;
            utd_ptr->utdTe = 0;
        }
    }
#endif
#ifdef VTSP_ENABLE_CIDR
    if (VTSPR_INFC_TYPE_FXO == _VTSPR_infcToType(infc)) {
        cidr_ptr = chan_ptr->cidr_ptr;
        if (0 != (onMask & VTSPR_ALG_CHAN_CAS)) {
            /*
             * Init CAS detector.
             */
            cas_ptr = &cidr_ptr->casObj;
            CAS_init(cas_ptr, globals_ptr, CAS_COLDSTART);
            cas_ptr->src_ptr = nlpOut_ptr;
        }
        if (0 != (onMask & VTSPR_ALG_CHAN_CAS) ||
                0 != (offMask & VTSPR_ALG_CHAN_CAS)) {
            /* Init CAS event state when:
             *  CAS detector is turned on or off.
             */
            cidr_ptr->casLe = 0;
            cidr_ptr->casTe = 0;
        }
        if (0 != (onMask & VTSPR_ALG_CHAN_FSKR_ONH)) {
            /*
             * Init FSKR ONHOOK
             */
            fskr_ptr = &cidr_ptr->fskrObj;
            FSKR_init(fskr_ptr, globals_ptr, dsp_ptr->fskrParams_ptr,
                    FSKR_ONHOOK);
            fskr_ptr->src_ptr = nlpOut_ptr;
        }
        else if (0 != (onMask & VTSPR_ALG_CHAN_FSKR_OFH)) {
            /*
             * Init FSKR OFFHOOK
             */
            fskr_ptr = &cidr_ptr->fskrObj;
            FSKR_init(fskr_ptr, globals_ptr, dsp_ptr->fskrParams_ptr,
                    FSKR_OFFHOOK);
            fskr_ptr->src_ptr = nlpOut_ptr;
        }
        if (0 != (onMask &
                (VTSPR_ALG_CHAN_FSKR_ONH | VTSPR_ALG_CHAN_FSKR_OFH)) ||
                0 != (offMask &
                (VTSPR_ALG_CHAN_FSKR_ONH | VTSPR_ALG_CHAN_FSKR_OFH))) {
            /* Init FSKR data state:
             */
            cidr_ptr->fskrCnt = 0;
            cidr_ptr->fskrDetect = 0;
            cidr_ptr->fskrCsum = 0;
            cidr_ptr->fskrValid = 0;
        }
    }
#endif

#ifdef VTSP_ENABLE_CIDS
    if (VTSPR_INFC_TYPE_FXS == _VTSPR_infcToType(infc)) {
        cids_ptr = chan_ptr->cids_ptr;
        if (0 != (onMask & VTSPR_ALG_CHAN_CIDS)) {
            cids_ptr->cidsObj.ctrl = CIDS_CTRL_IDLE;
            /*
             * There is only one CIDS parameters structure, so we have to
             *  set the values that are interface-specific prior to calling
             *  CIDS_init().
             */
            cids_ptr->cidsParam_ptr->fsksObj_ptr = &cids_ptr->fsksObj;
            cids_ptr->cidsParam_ptr->toneObj_ptr = &chan_ptr->toneObj;
            cids_ptr->cidsParam_ptr->msg_ptr     = chan_ptr->cidsData.data;

            CIDS_init(&cids_ptr->cidsObj, dsp_ptr->globals_ptr,
                    cids_ptr->cidsParam_ptr);
        }
        if (0 != (onMask & VTSPR_ALG_CHAN_CIDCWS)) {
            /*
             * There is only one CIDCWS parameters structure, so we have to set
             *  the values that are interface-specific prior to calling
             *  CIDCWS_init().
             */
            cids_ptr->cidcwsParam_ptr->fsksObj_ptr = &cids_ptr->fsksObj;
            cids_ptr->cidcwsParam_ptr->toneObj_ptr = &chan_ptr->toneObj;
            cids_ptr->cidcwsParam_ptr->msg_ptr = chan_ptr->cidsData.data;
#ifdef VTSP_ENABLE_TONE_QUAD        
            /*
             * Initial the parameter for Japanese CIDCW
             */
            toneSeq_ptr = &chan_ptr->toneQuadSeq; 
            cids_ptr->cidcwsParam_ptr->genfObj_ptr = toneSeq_ptr->toneObj_ptr;
            toneSeq_ptr->toneIdList[0]=1;
            toneSeq_ptr->toneIdList[1] = 2;
            toneSeq_ptr->toneNumToneIds = 2;
            /*
             * toneControl: voice is not interrupted during tone break times.
             */
            toneSeq_ptr->toneControl = VTSP_TONE_BREAK_MIX;
            toneSeq_ptr->toneTime = (uint32)(~0);
            toneSeq_ptr->toneSeqIndex = 0;
            toneSeq_ptr->toneSeqRepeat = 1;
            toneSeq_ptr->toneSeqRepeatCnt = 0;
            toneSeq_ptr->tonePreRetVal = 0;
            toneSeq_ptr->toneRetVal = 0;
            toneSeq_ptr->toneEdge = 0;
            tId = toneSeq_ptr->toneIdList[toneSeq_ptr->toneSeqIndex];

            toneQuad_ptr = toneSeq_ptr->toneObj_ptr;
            toneQuad_ptr->dst_ptr = chan_ptr->toneOut_ary;
            GENF_init(toneQuad_ptr, globals_ptr,
                    dsp_ptr->toneQuadParams_ptr[tId], 0);
#endif
            CIDCWS_init(&cids_ptr->cidcwsObj, dsp_ptr->globals_ptr,
                    cids_ptr->cidcwsParam_ptr);
        }
        if ((0 != (offMask & VTSPR_ALG_CHAN_CIDS)) ||
                (0 != (offMask & VTSPR_ALG_CHAN_CIDCWS))) {
            /* Clear CID data */
            chan_ptr->cidsData.len = 0;
            /* Set cid state to abort state */
            chan_ptr->cids_ptr->cidsObj.stat = CIDS_STAT_ABORT;
        }
    }
#else /* !VTSP_ENABLE_CIDS */
    if ((0 != (onMask & VTSPR_ALG_CHAN_CIDS)) ||
            (0 != (onMask & VTSPR_ALG_CHAN_CIDCWS))) {
        /* turn off */
        onMask &= ~(VTSPR_ALG_CHAN_CIDS | VTSPR_ALG_CHAN_CIDCWS);
        newState &= ~(VTSPR_ALG_CHAN_CIDS | VTSPR_ALG_CHAN_CIDCWS);
    }
#endif

#ifdef VTSP_ENABLE_T38
    /* Initialize T38 events on the channel. */
    if ((0 != (onMask & VTSPR_ALG_CHAN_T38)) ||
            (0 != (offMask & VTSPR_ALG_CHAN_T38))) {
        fr38_ptr = chan_ptr->fr38_ptr;
        fr38_ptr->fr38Event = VTSP_EVENT_INACTIVE;
        fr38_ptr->fr38EventLast = VTSP_EVENT_INACTIVE;
    }
#endif

#if defined(VTSP_ENABLE_STREAM_16K) || defined(VTSP_ENABLE_AUDIO_SRC)
#ifndef VTSP_ENABLE_MP_LITE
    /*
     * XXX
     * Maybe need a mask or something to decide when to re-init UDS
     * for the audio processing.
     */
    if (chan_ptr->numSamples10ms < VTSPR_NSAMPLES_STREAM) {
        lowRate = chan_ptr->numSamples10ms;
        highRate = VTSPR_NSAMPLES_STREAM;
    }
    else {
        lowRate = VTSPR_NSAMPLES_STREAM;
        highRate = VTSPR_NSAMPLES_10MS_MAX;
    }
    lowRate = _VTSPR_samples10msToUdsMode(lowRate);
    highRate = _VTSPR_samples10msToUdsMode(highRate);

    UDS_init(&chan_ptr->udsAudioUp, (UDS_SampleRate)lowRate,
            (UDS_SampleRate)highRate);
    UDS_init(&chan_ptr->udsAudioDown, (UDS_SampleRate)lowRate,
            (UDS_SampleRate)highRate);
#endif
#endif /* end VTSP_ENABLE_STREAM_16K */

    chan_ptr->algChannelState = newState;
}


/*
 * ======== _VTSPR_algStateStream ========
 * Set/Reset algorithm state for stream processing
 * and initialize algorithms as necessary
 */
void _VTSPR_algStateStream(
    VTSPR_DSP   *dsp_ptr,
    uvint        infc,
    uvint        streamId,
    uint32       clearMask,
    uint32       setMask)
{
    VTSPR_StreamObj  *stream_ptr;
#ifndef VTSP_ENABLE_MP_LITE
    TONE_Obj         *tone_ptr;
    PLC_Obj          *plc_ptr;
    VTSPR_ToneSeq    *toneSeq_ptr;
    TONE_Params      *toneParams_ptr;    
    vint              tId;
#endif
    GLOBAL_Params    *globals_ptr;
    uint32            oldState;
    uint32            newState;
    uint32            changeMask;
    uint32            onMask;
#ifdef VTSP_ENABLE_TONE_QUAD
    GENF_Obj         *toneQuad_ptr;
#endif

    stream_ptr = _VTSPR_streamIdToStreamPtr(dsp_ptr, infc, streamId);

    oldState = stream_ptr->algStreamState;
    newState = oldState | setMask;
    newState = newState & (~clearMask);

    /*
     * Algs that have turned on or off
     */
    changeMask  = oldState ^ newState;
    /*
     * Algs that have turned on
     */
    onMask      = changeMask & newState;

    globals_ptr = dsp_ptr->globals_ptr;

    /*
     * Init algorithms.
     * Coders will be init automatically by audio processing block.
     */
#ifdef VTSP_ENABLE_STREAM_16K
#ifndef VTSP_ENABLE_MP_LITE
    /*
     * XXX
     * Do we need to have a mask to decide when to init this stuff?
     */
    UDS_init(&stream_ptr->udsStreamUp, UDS_SAMPLERATE_8K,
            UDS_SAMPLERATE_16K);
    UDS_init(&stream_ptr->udsStreamDown, UDS_SAMPLERATE_8K, 
            UDS_SAMPLERATE_16K);
#endif
#endif /* end VTSP_ENABLE_STREAM_16K */

#ifdef VTSP_ENABLE_TONE_QUAD
    if ((0 != (newState & VTSPR_ALG_STREAM_TONE_QUAD)) &&
            (0 != (newState & VTSPR_ALG_STREAM_TONE))) { 
        /* 
         * State conflict: Tone and Tone Quad.
         * Terminate Tone in this instance.
         */
        clearMask &= ~(VTSPR_ALG_STREAM_TONE);
    }
#endif

    /* 
     * The GENF object for Quad Tone is in the stream obj,
     * but the tone configs are always from the dsp obj.
     */
    if (0 != (setMask & VTSPR_ALG_STREAM_TONE_QUAD)) { 
#ifdef VTSP_ENABLE_TONE_QUAD
        toneSeq_ptr = &stream_ptr->toneQuadSeq; 
        tId = toneSeq_ptr->toneIdList[toneSeq_ptr->toneSeqIndex];
        /* XXX May need to add support for 'warm' argument to GENF_init() */
        toneQuad_ptr = toneSeq_ptr->toneObj_ptr;
        GENF_init(toneQuad_ptr, globals_ptr,
                dsp_ptr->toneQuadParams_ptr[tId], 0);
        toneQuad_ptr->dst_ptr = stream_ptr->confPeer_ary;
        if (0 != (VTSPR_ALG_STREAM_TONE_QUAD & oldState)) {
            /* Halting tone generation, send the halted command */
            toneSeq_ptr->toneEvent = VTSP_EVENT_HALTED;
        }
#endif
    }
#ifndef VTSP_ENABLE_MP_LITE
    else if (0 != (setMask & VTSPR_ALG_STREAM_TONE)) { 
        /*
         * Tone to network, set toneIdList[] in channel object before
         * TONE_init().  Start with the first toneId.
         * The tone object for Stream Tone is in the stream obj,
         * but the tone configs are always from the dsp obj.
         */
        toneSeq_ptr = &stream_ptr->toneSeq;
        tId = toneSeq_ptr->toneIdList[toneSeq_ptr->toneSeqIndex];
        toneParams_ptr = dsp_ptr->toneParams_ptr[tId];

        /* Set the TONE sample rate before each init. */
#ifdef VTSP_ENABLE_STREAM_16K        
        toneParams_ptr->ctrlWord |= TONE_SAMPLERATE_16K; /* 160 samples */
#else
        toneParams_ptr->ctrlWord &= (~TONE_SAMPLERATE_16K); /* 80 samples */
#endif
        tone_ptr = toneSeq_ptr->toneObj_ptr;
        TONE_init(tone_ptr, globals_ptr, toneParams_ptr);
        tone_ptr->dst_ptr = stream_ptr->confPeer_ary;
        if (0 != (VTSPR_ALG_CHAN_TONE & oldState)) {
            /* Halting tone generation, send the halted event */
            toneSeq_ptr->toneEvent = VTSP_EVENT_HALTED;
        }
    }

    if (0 != (clearMask & VTSPR_ALG_STREAM_TONE)) { 
        /*
         * Command to turn off the tone.
         * Ensure HALTED event is sent.
         */
        toneSeq_ptr = &stream_ptr->toneSeq;
        if (0 != (VTSPR_ALG_STREAM_TONE & oldState)) {
            /* Halting tone generation, send the halted event */
            toneSeq_ptr->toneEvent = VTSP_EVENT_HALTED;
        }
    }
    if (0 != (clearMask & VTSPR_ALG_STREAM_TONE_QUAD)) { 
#ifdef VTSP_ENABLE_TONE_QUAD
        /*
         * Command to turn off the tone.
         * Ensure HALTED event is sent.
         */
        toneSeq_ptr = &stream_ptr->toneQuadSeq;
        if (0 != (VTSPR_ALG_STREAM_TONE_QUAD & oldState)) {
            /* Halting tone generation, send the halted event */
            toneSeq_ptr->toneEvent = VTSP_EVENT_HALTED;
        }
#endif
    }
    if (0 != (onMask & VTSPR_ALG_STREAM_PLC)) {
        /*
         * Init PLC.
         * XXX: Warning this will zero PLC's pkt_ptr (dest ptr).
         */
        plc_ptr = &stream_ptr->plcObj;
        PLC_init(plc_ptr);
    }
#endif /* VTSP_ENABLE_MP_LITE */
    if (0 != (setMask & VTSPR_ALG_STREAM_JB) ||
            0 != (clearMask & VTSPR_ALG_STREAM_JB)) {
        /*
         * Flush the jitter buffer.
         * This is supposed to happen at the start of a new session/stream,
         * or at the end of a stream.
         * JB_FALSE = do_flush_only will reset the statistics.
         */
        JB_init(&stream_ptr->jbObj,  &stream_ptr->jbParams);
        JB_resetStats(&stream_ptr->jbObj);
        stream_ptr->framesSent = 0;
    }

    if (0 != (onMask & VTSPR_ALG_STREAM_T38)) {
#ifdef VTSP_ENABLE_T38
        ; /* Nothing needs to be done, just set the bit in streamState. */
#else
        /* 
         * T38 not enabled. Always turn off the algorithm
         */
        newState &= ~(VTSPR_ALG_STREAM_T38);
        /* 
         * XXX Should not get here
         */
        /* _VTSP_TRACE(__FILE__, __LINE__); */
#endif
    }

    stream_ptr->algStreamState = newState;
}
