/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2006-2010 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 30256 $ $Date: 2014-12-08 17:30:17 +0800 (Mon, 08 Dec 2014) $
 *
 */
#include "vtspr.h"
#include "_vtspr_private.h"

#if defined(VTSP_ENABLE_GAMRNB_ACCELERATOR) || \
        defined(VTSP_ENABLE_GAMRWB_ACCELERATOR)
#include <dsp.h>
#endif

#ifdef VTSP_ENABLE_BENCHMARK
extern VTSPR_Obj VTSPR_obj;
#endif
/*
 * ======== _VTSPR_audioLinearToCoded() ========
 *
 * Generate encoded voice block
 * by the voice coders.
 */
OSAL_INLINE uvint _VTSPR_audioLinearToCoded(
    VTSPR_StreamObj   *stream_ptr,
    vint              *src_ptr, /* either 80 or 160 vints, depends on encType */
    vint              *dst_ptr,
    uvint              encType,
    uvint              voiceActive,
#ifdef VTSP_ENABLE_DTMFR
    DR_EventObj       *drEvent_ptr,
#endif
#if !defined(VTSP_ENABLE_MP_LITE) || defined(VTSP_ENABLE_NFE) || defined(VTSP_ENABLE_AEC)
    vint               noiseFloor,
#endif
    vint               infc)
{
    uint32  mask;
#if defined(VTSP_ENABLE_G711P1) && !defined(VTSP_ENABLE_G711P1_ACCELERATOR)
    uint8  *pkt_ptr;
    uint32  extension;
    uvint   mode;
#endif

    /*
     * Encode CN only if noiseFloor on channel is below a definite voice level,
     * and NLP is active.
     *
     * Don't encode CN if this stream is receiving audio from any conference
     * peer.  We don't want to do silence compression on summed audio, because
     * we are not checking for voice activity on the peers.
     *
     * Don't encode CN if DTMFR is active.
     */
    mask = stream_ptr->algStreamState;
#if !defined(VTSP_ENABLE_MP_LITE) || defined(VTSP_ENABLE_NFE) || defined(VTSP_ENABLE_AEC)
    if ((noiseFloor < VTSPR_CN_THRESHOLD)
            && (0 == voiceActive)
            && (0 == stream_ptr->streamParam.confMask)
#ifdef VTSP_ENABLE_DTMFR
            && (0 == (stream_ptr->drEncodeObj.status & DR_IN_DIGIT))
#endif
            && (0 == (mask & 
                   (VTSPR_ALG_STREAM_TONE | VTSPR_ALG_STREAM_TONE_QUAD)))
        ) {

        /* Do comfort noise if G711U/A/G726 coder is configured for CN.
         *
         * NOTE: this does not change encoder type in stream obj.  stream
         * encoder type does not change for CN packets.
         */
        if ((VTSP_CODER_G711U == encType) &&
                (0 != (stream_ptr->streamParam.silenceComp
                    & VTSP_MASK_CODER_G711U))) {
            encType = VTSP_CODER_CN;
        }
        else if ((VTSP_CODER_G711A == encType) &&
                (0 != (stream_ptr->streamParam.silenceComp
                    & VTSP_MASK_CODER_G711A))) {
            encType = VTSP_CODER_CN;
        }
#ifdef VTSP_ENABLE_16K_MU
        else if ((VTSP_CODER_16K_MU == encType) &&
                (0 != (stream_ptr->streamParam.silenceComp
                    & VTSP_MASK_CODER_16K_MU))) {
            encType = VTSP_CODER_CN;
        }
#endif /* end VTSP_ENABLE_16K_MU */
#ifdef VTSP_ENABLE_G726
        else if ((VTSP_CODER_G726_32K == encType) &&
                (0 != (stream_ptr->streamParam.silenceComp
                    & VTSP_MASK_CODER_G726_32K))) {
            encType = VTSP_CODER_CN;
        }
#endif
#ifdef VTSP_ENABLE_G711P1
        else if ((VTSP_CODER_G711P1U == encType) &&
                (0 != (stream_ptr->streamParam.silenceComp
                    & VTSP_MASK_CODER_G711P1U))) {
            encType = VTSP_CODER_CN;
        }
        else if ((VTSP_CODER_G711P1A == encType) &&
                (0 != (stream_ptr->streamParam.silenceComp
                    & VTSP_MASK_CODER_G711P1A))) {
            encType = VTSP_CODER_CN;
        }
#endif
#ifdef VTSP_ENABLE_G722
        else if ((VTSP_CODER_G722 == encType) &&
                (0 != (stream_ptr->streamParam.silenceComp
                    & VTSP_MASK_CODER_G722))) {
            encType = VTSP_CODER_CN;
        }
#endif
    }
#endif
    switch (encType) {

        case VTSP_CODER_CN:
#if !defined(VTSP_ENABLE_MP_LITE) || defined(VTSP_ENABLE_NFE) || defined(VTSP_ENABLE_AEC)
            /*
             * Do silence compression here.
             *
             * CN blocks are always 10 ms.
             *
             * CN blocks are generated in the following cases;
             *  1. encodeTime[CN] has elapsed.
             *  2. if encodeTime[CN] is zero, it is infinite (never elapses).
             *  3. noiseFloor has changed significantly (beyond hysterisis).
             */
            stream_ptr->blockLen = VTSP_BLOCK_G711_CN_SZ;

            if ((noiseFloor <= stream_ptr->cnPower +
                    VTSPR_CN_HYSTERESIS) &&
                    (noiseFloor >= stream_ptr->cnPower -
                    VTSPR_CN_HYSTERESIS)) {
                /*
                 * No block.
                 * noiseFloor has not changed significantly.
                 */
                stream_ptr->blockLen = 0;
            }

            /*
             * Always generate CN block if cnPktTime has reached zero.
             * If equal zero, then packet time is infinite (only generate
             * pkts when noise floor changes).
             */
            if (0 != stream_ptr->streamParam.encodeTime[VTSP_CODER_CN]) {
                stream_ptr->cnPktTime -= 10;        /* 10 ms */
                if (stream_ptr->cnPktTime <= 0) {
                    /*
                     * CN Packet time expired.
                     * Force CN generation, even if set to zero above
                     */
                    stream_ptr->blockLen = VTSP_BLOCK_G711_CN_SZ;
                    stream_ptr->cnPktTime =
                            stream_ptr->streamParam.encodeTime[VTSP_CODER_CN];
                }
            }

            if (stream_ptr->blockLen > 0) {
                /*
                 * create CN block
                 */
                stream_ptr->cnPower  = noiseFloor;
                *((uint8 *)dst_ptr)  = -noiseFloor;
                stream_ptr->count.encodeCN++;
            }
            break;
#endif
        case VTSP_CODER_G711U:

            /*
             * Encode G711U  
             */
            COMM_lin2mu(src_ptr, src_ptr, VTSPR_NSAMPLES_10MS_8K);
            COMM_pack32((uint32 *)dst_ptr, src_ptr, VTSPR_NSAMPLES_10MS_8K);
            stream_ptr->blockLen = VTSP_BLOCK_G711_10MS_SZ;
            break;

        case VTSP_CODER_G711A:

            /*
             * Encode G711A
             */
            COMM_shiftRight(src_ptr, src_ptr, 1, VTSPR_NSAMPLES_10MS_8K);

            COMM_lin2a(src_ptr, src_ptr, VTSPR_NSAMPLES_10MS_8K);
            COMM_pack32((uint32 *)dst_ptr, src_ptr, VTSPR_NSAMPLES_10MS_8K);
            stream_ptr->blockLen = VTSP_BLOCK_G711_10MS_SZ;
            break;

#if defined(VTSP_ENABLE_G711P1) && !defined(VTSP_ENABLE_G711P1_ACCELERATOR)
        case VTSP_CODER_G711P1U:
        case VTSP_CODER_G711P1A:
            /*
             * Convert to Q.15 from Q.13.
             */
            COMM_shiftLeft(src_ptr, src_ptr, 2, VTSPR_NSAMPLES_10MS_16K);
            /*
             * Encode G711P1(U/A) 2 x 5ms @ 16kHz
             */
            pkt_ptr = (uint8 *)dst_ptr;
            stream_ptr->blockLen = G711P1_encode(&stream_ptr->g711p1EncObj,
                    &src_ptr[0], &pkt_ptr[1]);

            stream_ptr->blockLen += G711P1_encode(&stream_ptr->g711p1EncObj,
                    &src_ptr[(VTSPR_NSAMPLES_10MS_16K >> 1)],
                    &pkt_ptr[stream_ptr->blockLen + 1]);
            /* Add mode+1 octet to front of encoded packet (RFC 5391) */
            extension = stream_ptr->streamParam.extension;
            if (0 != (extension & VTSP_MASK_EXT_G711P1_R3_96)) {
                mode = G711P1_MODE_R3;
            }
            else if (0 != (extension & VTSP_MASK_EXT_G711P1_R2A_80)) {
                mode = G711P1_MODE_R2A;
            }
            else if (0 != (extension & VTSP_MASK_EXT_G711P1_R2B_80)) {
                mode = G711P1_MODE_R2B;
            }
            else if (0 != (extension & VTSP_MASK_EXT_G711P1_R1_64)) {
                mode = G711P1_MODE_R1;
            }
            pkt_ptr[0] = mode + 1;
            
            stream_ptr->blockLen++; 
            break;
#endif

#ifdef VTSP_ENABLE_16K_MU
        case VTSP_CODER_16K_MU:

            /*
             * Convert 160 mu-law samples
             */
            COMM_lin2mu(src_ptr, src_ptr, VTSPR_NSAMPLES_10MS_16K);
            COMM_pack32(dst_ptr, src_ptr, VTSPR_NSAMPLES_10MS_16K);
            stream_ptr->blockLen = VTSP_BLOCK_16K_MU_SZ;
            break;
#endif /* end VTSP_ENABLE_16K_MU */

#if defined(VTSP_ENABLE_G729) && !defined(VTSP_ENABLE_G729_ACCELERATOR) 
        case VTSP_CODER_G729:
            /*
             * Convert to Q.15 from Q.13.
             */
            COMM_shiftLeft(src_ptr, src_ptr, 2, VTSPR_NSAMPLES_10MS_8K);
            /*
             * Run G.729A
             */
            if (0 != (stream_ptr->streamParam.silenceComp &
                    VTSP_MASK_CODER_G729)) {
                stream_ptr->g729EncObj.vad_enable = 1;
            }
            else {
                stream_ptr->g729EncObj.vad_enable = 0;
            }

#ifdef VTSP_ENABLE_BENCHMARK
            if (0 == stream_ptr->streamParam.streamId) {
                _VTSPR_benchmarkStart(&VTSPR_obj,
                        _VTSPR_BENCHMARK_G729_ENCODE_ID0, 0);
            }
            else {
                _VTSPR_benchmarkStart(&VTSPR_obj,
                        _VTSPR_BENCHMARK_G729_ENCODE_ID1, 0);
            }
#endif

            stream_ptr->blockLen =
                    G729AB_encode(&stream_ptr->g729EncObj, src_ptr,
                            (short *)dst_ptr);

#ifdef VTSP_ENABLE_BENCHMARK
            if (0 == stream_ptr->streamParam.streamId) {
                _VTSPR_benchmarkStop(&VTSPR_obj,
                        _VTSPR_BENCHMARK_G729_ENCODE_ID0, 0);
            }
            else {
                _VTSPR_benchmarkStop(&VTSPR_obj,
                        _VTSPR_BENCHMARK_G729_ENCODE_ID1, 0);
            }
#endif
            break;
#endif
#ifdef VTSP_ENABLE_G722
        case VTSP_CODER_G722:
            /* Convert to Q.13 from Q.14. */
            COMM_shiftLeft(src_ptr, src_ptr, 1, VTSPR_NSAMPLES_10MS_16K);
            /*
             * Run G.722
             */
#ifdef VTSP_ENABLE_BENCHMARK
            if (0 == stream_ptr->streamParam.streamId) {
                _VTSPR_benchmarkStart(&VTSPR_obj,
                        _VTSPR_BENCHMARK_G722_ENCODE_ID0, 0);
            }
            else {
                _VTSPR_benchmarkStart(&VTSPR_obj,
                        _VTSPR_BENCHMARK_G722_ENCODE_ID1, 0);
            }
#endif
            stream_ptr->blockLen =
                    g722_encode(&stream_ptr->g722EncObj, src_ptr,
                            (uint8 *)dst_ptr);
#ifdef VTSP_ENABLE_BENCHMARK
            if (0 == stream_ptr->streamParam.streamId) {
                _VTSPR_benchmarkStop(&VTSPR_obj,
                        _VTSPR_BENCHMARK_G722_ENCODE_ID0, 0);
            }
            else {
                _VTSPR_benchmarkStop(&VTSPR_obj,
                        _VTSPR_BENCHMARK_G722_ENCODE_ID1, 0);
            }
#endif
            break;
#endif

#if defined(VTSP_ENABLE_G726) && !defined(VTSP_ENABLE_G726_ACCELERATOR) 
        case VTSP_CODER_G726_32K:

            /*
             * Run G.726
             */
            stream_ptr->g726EncObj.src_ptr = src_ptr;
            stream_ptr->g726EncObj.dst_ptr = dst_ptr;

#ifdef VTSP_ENABLE_BENCHMARK
            if (0 == stream_ptr->streamParam.streamId) {
                _VTSPR_benchmarkStart(&VTSPR_obj,
                        _VTSPR_BENCHMARK_G726_ENCODE_ID0, 0);
            }
            else {
                _VTSPR_benchmarkStart(&VTSPR_obj,
                        _VTSPR_BENCHMARK_G726_ENCODE_ID1, 0);
            }
#endif
            stream_ptr->blockLen = G726_encode(&stream_ptr->g726EncObj);
#ifdef VTSP_ENABLE_BENCHMARK
            if (0 == stream_ptr->streamParam.streamId) {
                _VTSPR_benchmarkStop(&VTSPR_obj,
                        _VTSPR_BENCHMARK_G726_ENCODE_ID0, 0);
            }
            else {
                _VTSPR_benchmarkStop(&VTSPR_obj,
                        _VTSPR_BENCHMARK_G726_ENCODE_ID1, 0);
            }
#endif

            break;
#endif

#if defined(VTSP_ENABLE_ILBC) || defined(VTSP_ENABLE_G723) || \
    defined(VTSP_ENABLE_G722P1) || defined(VTSP_ENABLE_GAMRNB) || \
    defined(VTSP_ENABLE_GAMRWB) || defined(VTSP_ENABLE_SILK) || \
    defined(VTSP_ENABLE_G729_ACCELERATOR) || \
    defined(VTSP_ENABLE_G726_ACCELERATOR) || \
    defined(VTSP_ENABLE_G711P1_ACCELERATOR)
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
#if defined(VTSP_ENABLE_GAMRNB) && !defined(VTSP_ENABLE_GAMRNB_ACCELERATOR)
        case VTSP_CODER_GAMRNB_20MS_OA:
        case VTSP_CODER_GAMRNB_20MS_BE:
#endif
#if defined(VTSP_ENABLE_GAMRWB) && !defined(VTSP_ENABLE_GAMRWB_ACCELERATOR)
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
#ifdef VTSP_ENABLE_G711P1_ACCELERATOR
        case VTSP_CODER_G711P1U:
        case VTSP_CODER_G711P1A:
#endif
            stream_ptr->blockLen = _VTSPR_multiEncode(src_ptr, dst_ptr,
                                                    stream_ptr, encType, infc);
            break;
#endif
        /*
         * Encode with external Codec.
         */
#if !defined(VTSP_ENABLE_GAMRNB) && defined(VTSP_ENABLE_GAMRNB_ACCELERATOR)
        case VTSP_CODER_GAMRNB_20MS_OA:
            stream_ptr->blockLen = DSP_encode(DSP_CODER_TYPE_AMRNB_OA,
                    (uint8*)dst_ptr, src_ptr);
            break;
        case VTSP_CODER_GAMRNB_20MS_BE:
            stream_ptr->blockLen = DSP_encode(DSP_CODER_TYPE_AMRNB_BE,
                    (uint8*)dst_ptr, src_ptr);
            break;
#endif
#if !defined(VTSP_ENABLE_GAMRWB) && defined(VTSP_ENABLE_GAMRWB_ACCELERATOR)
        case VTSP_CODER_GAMRWB_20MS_OA:
            stream_ptr->blockLen = DSP_encode(DSP_CODER_TYPE_AMRWB_OA,
                    (uint8*)dst_ptr, src_ptr);
            break;
        case VTSP_CODER_GAMRWB_20MS_BE:
            stream_ptr->blockLen = DSP_encode(DSP_CODER_TYPE_AMRWB_BE,
                    (uint8*)dst_ptr, src_ptr);
            break;
#endif

#ifdef VTSP_ENABLE_DTMFR
        case VTSP_CODER_DTMF:
            /* fall through */
        case VTSP_CODER_TONE:
            stream_ptr->blockLen =
                    DR_encode(&stream_ptr->drEncodeObj, drEvent_ptr, dst_ptr);
            break;
#endif
        default:
            _VTSP_TRACE(__FILE__, __LINE__);
            stream_ptr->blockLen = 0;
            stream_ptr->countEncode--;
            stream_ptr->count.encodePkt--;
            break;
    }
    return (encType);
}


/*
 * ======== _VTSPR_audioCodedToLinear() ========
 *
 * Generate linear voice data
 * from coded block.
 *
 *
 *
 * IMPORTANT:
 * If pSize is zero, decIn_ptr will not be valid.
 *
 */
OSAL_INLINE void _VTSPR_audioCodedToLinear(
    VTSPR_Obj         *vtspr_ptr,
    VTSPR_StreamObj   *stream_ptr,
    vint              *decIn_ptr,  
    vint              *decOut_ptr,/* 80 or 160 samples, depending on decType */
    vint               pSize,
    vint               decType,
#ifndef VTSP_ENABLE_MP_LITE
    PLC_Obj           *plc_ptr,
#endif
    vint               infc)
{
#ifndef VTSP_ENABLE_MP_LITE
    TIC_Obj *tic_ptr;
    vint     nPowerDb;
#endif
#if defined(VTSP_ENABLE_ILBC) || defined(VTSP_ENABLE_G723) || \
    defined(VTSP_ENABLE_G722P1) || defined(VTSP_ENABLE_GAMRNB) || \
    defined(VTSP_ENABLE_GAMRWB) || defined(VTSP_ENABLE_SILK) || \
    defined(VTSP_ENABLE_G729_ACCELERATOR) || \
    defined(VTSP_ENABLE_G726_ACCELERATOR) || defined(VTSP_ENABLE_G711P1)
    vint     plcFlag = 0;
#endif
#if defined(VTSP_ENABLE_G711P1) && !defined(VTSP_ENABLE_G711P1_ACCELERATOR) 
    uint8   *pkt_ptr;
    uvint    encBytesPer5ms;
#endif

    /*
     * Decode or set to run PLC.
     */
    stream_ptr->countDecode++;
    if (0 != pSize) {
        /* If no packet or invalid packet, do not need to increase count. */
        stream_ptr->count.decodePkt++;
        stream_ptr->count.decodeBytes += pSize;
    }
    switch (decType) {

        case VTSP_CODER_G711U:
            if (0 != pSize) {
                COMM_unpack32(decOut_ptr, (uint32 *)decIn_ptr, pSize);
                COMM_mu2lin(decOut_ptr, decOut_ptr, pSize);
            }
#ifndef VTSP_ENABLE_MP_LITE
            else {
                plc_ptr->ctrlStat |= PLC_CTRL_BPI;
            }
#endif
            break;

#ifdef VTSP_ENABLE_16K_MU
        case VTSP_CODER_16K_MU:
            if (0 != pSize) {
                COMM_unpack32(decOut_ptr, (uint32 *)decIn_ptr, pSize);
                COMM_mu2lin(decOut_ptr, decOut_ptr, pSize);
            }
#ifndef VTSP_ENABLE_MP_LITE
            else {
                plc_ptr->ctrlStat |= PLC_CTRL_BPI;
            }
#endif
            break;

#endif /* end VTSP_ENABLE_16K_MU */

        case VTSP_CODER_G711A:
            if (0 != pSize) {
                COMM_unpack32(decOut_ptr, (uint32 *)decIn_ptr, pSize);
                COMM_a2lin(decOut_ptr, decOut_ptr, pSize);
                /*
                 * Convert to alaw levels.
                 */
                COMM_shiftLeft(decOut_ptr, decOut_ptr, 1,
                        VTSPR_NSAMPLES_10MS_8K);
            }
#ifndef VTSP_ENABLE_MP_LITE
            else {
                plc_ptr->ctrlStat |= PLC_CTRL_BPI;
            }
#endif

            break;

#if defined(VTSP_ENABLE_G711P1) && !defined(VTSP_ENABLE_G711P1_ACCELERATOR)
        case VTSP_CODER_G711P1U:
        case VTSP_CODER_G711P1A:
#warning XXX This does not work yet XXX          
            if (0 != pSize) {
                pkt_ptr = (uint8 *)decIn_ptr; 
                /* 
                 * Get mode from first octet to determine encoded bytes per 5ms
                 */
                switch ((pkt_ptr[0]-1)) {
                    case G711P1_MODE_R1:
                        encBytesPer5ms = JB_G711P1_MODE_R1_VOICE_SZ >> 1;
                        break;
                    case G711P1_MODE_R2A:
                    case G711P1_MODE_R2B:
                        encBytesPer5ms = JB_G711P1_MODE_R2_VOICE_SZ >> 1;
                        break;
                    case G711P1_MODE_R3:
                        encBytesPer5ms = JB_G711P1_MODE_R3_VOICE_SZ >> 1;
                        break;
                    default:
                        OSAL_logMsg("ERROR: G711P1 bad mode received\n");
                        break;
                }
                plcFlag = G711P1_ERRORFLAG_PACKET_OK;
                /*
                 * Decode G711P1(U/A) 2 x 5ms @ 16kHz
                 */
                G711P1_decode(&stream_ptr->g711p1DecObj, &pkt_ptr[1],
                        &decOut_ptr[0], plcFlag);

                G711P1_decode(&stream_ptr->g711p1DecObj,
                        &pkt_ptr[encBytesPer5ms + 1],
                        &decOut_ptr[(VTSPR_NSAMPLES_10MS_16K >> 1)], plcFlag);
                /*
                 * Convert to Q.13 from Q.15.
                 */
                COMM_shiftRight(decOut_ptr, decOut_ptr, 2,
                        VTSPR_NSAMPLES_10MS_16K);
            }
            else {
                plcFlag = G711P1_ERRORFLAG_PACKET_LOSS;
                noPacket[0] = 0x3;
                pkt_ptr = noPacket;
            }
            break;
#endif

#if defined(VTSP_ENABLE_G729) && !defined(VTSP_ENABLE_G729_ACCELERATOR)
        case VTSP_CODER_G729:
           
#ifdef VTSP_ENABLE_BENCHMARK
            if (0 == stream_ptr->streamParam.streamId) {
                _VTSPR_benchmarkStart(vtspr_ptr,
                        _VTSPR_BENCHMARK_G729_DECODE_ID0, 0);
            }
            else {
                _VTSPR_benchmarkStart(vtspr_ptr,
                        _VTSPR_BENCHMARK_G729_DECODE_ID1, 0);
            }
#endif
            
            /*
             * Run G.729AB decode.
             */
            G729AB_decode(&stream_ptr->g729DecObj, (short *)decIn_ptr,
                    decOut_ptr, pSize, pSize == 0);

#ifdef VTSP_ENABLE_BENCHMARK
            if (0 == stream_ptr->streamParam.streamId) {
                _VTSPR_benchmarkStop(vtspr_ptr,
                        _VTSPR_BENCHMARK_G729_DECODE_ID0, 0);
            }
            else {
                _VTSPR_benchmarkStop(vtspr_ptr,
                        _VTSPR_BENCHMARK_G729_DECODE_ID1, 0);
            }
#endif
            /*
             * Convert to Q.13 from Q.15.
             */
            COMM_shiftRight(decOut_ptr, decOut_ptr, 2,
                    VTSPR_NSAMPLES_10MS_8K);
            break;
#endif
#ifdef VTSP_ENABLE_G722
        case VTSP_CODER_G722:
            /*
             * Run G.722 decode.
             */
            if (0 != pSize) {
#ifdef VTSP_ENABLE_BENCHMARK
                if (0 == stream_ptr->streamParam.streamId) {
                    _VTSPR_benchmarkStart(vtspr_ptr,
                            _VTSPR_BENCHMARK_G722_DECODE_ID0, 0);
                }
                else {
                    _VTSPR_benchmarkStart(vtspr_ptr,
                            _VTSPR_BENCHMARK_G722_DECODE_ID1, 0);
                }
#endif
                g722_decode(&stream_ptr->g722DecObj, (uint8 *)decIn_ptr,
                        decOut_ptr);

#ifdef VTSP_ENABLE_BENCHMARK
                if (0 == stream_ptr->streamParam.streamId) {
                    _VTSPR_benchmarkStop(vtspr_ptr,
                            _VTSPR_BENCHMARK_G722_DECODE_ID0, 0);
                }
                else {
                    _VTSPR_benchmarkStop(vtspr_ptr,
                            _VTSPR_BENCHMARK_G722_DECODE_ID1, 0);
                }
#endif
                /*
                 * Convert to Q.14 from Q.13.
                 */
                COMM_shiftRight(decOut_ptr, decOut_ptr, 1,
                        VTSPR_NSAMPLES_10MS_16K);
            }
#ifndef VTSP_ENABLE_MP_LITE
            else {
                plc_ptr->ctrlStat |= PLC_CTRL_BPI;
            }            
#endif

            break;
#endif
#if defined(VTSP_ENABLE_G726) && !defined(VTSP_ENABLE_G726_ACCELERATOR)
        case VTSP_CODER_G726_32K:
            /*
             * Run G.726
             */
            if (0 != pSize) {
                stream_ptr->g726DecObj.src_ptr = decIn_ptr;
                stream_ptr->g726DecObj.dst_ptr = decOut_ptr;
#ifdef VTSP_ENABLE_BENCHMARK
            if (0 == stream_ptr->streamParam.streamId) {
                _VTSPR_benchmarkStart(vtspr_ptr,
                        _VTSPR_BENCHMARK_G726_DECODE_ID0, 0);
            }
            else {
                _VTSPR_benchmarkStart(vtspr_ptr,
                        _VTSPR_BENCHMARK_G726_DECODE_ID1, 0);
            }
#endif
                G726_decode(&stream_ptr->g726DecObj);
#ifdef VTSP_ENABLE_BENCHMARK
            if (0 == stream_ptr->streamParam.streamId) {
                _VTSPR_benchmarkStop(vtspr_ptr,
                        _VTSPR_BENCHMARK_G726_DECODE_ID0, 0);
            }
            else {
                _VTSPR_benchmarkStop(vtspr_ptr,
                        _VTSPR_BENCHMARK_G726_DECODE_ID1, 0);
            }
#endif
            }
#ifndef VTSP_ENABLE_MP_LITE
            else {
                plc_ptr->ctrlStat |= PLC_CTRL_BPI;
            }
#endif

            break;
#endif

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
#if defined(VTSP_ENABLE_GAMRNB) && !defined(VTSP_ENABLE_GAMRNB_ACCELERATOR)
        case VTSP_CODER_GAMRNB_20MS_OA:
        case VTSP_CODER_GAMRNB_20MS_BE:
#endif
#if defined(VTSP_ENABLE_GAMRWB) && !defined(VTSP_ENABLE_GAMRWB_ACCELERATOR)
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
            plcFlag = _VTSPR_multiDecode(decIn_ptr, decOut_ptr, pSize,
                    stream_ptr, decType, infc);
#ifndef VTSP_ENABLE_MP_LITE
            if (plcFlag) {
                plc_ptr->ctrlStat |= PLC_CTRL_BPI;
            }
#endif
            break;
#endif
        /*
         * Decode with external DSP.
         */
#if defined(VTSP_ENABLE_GAMRNB_ACCELERATOR) && !defined(VTSP_ENABLE_GAMRNB)
        case VTSP_CODER_GAMRNB_20MS_OA:
            DSP_decode(DSP_CODER_TYPE_AMRNB_OA, decOut_ptr, (uint8*)decIn_ptr,
                    pSize);
            break;
        case VTSP_CODER_GAMRNB_20MS_BE:
            DSP_decode(DSP_CODER_TYPE_AMRNB_BE, decOut_ptr, (uint8*)decIn_ptr,
                    pSize);
            break;
#endif
#if defined(VTSP_ENABLE_GAMRWB_ACCELERATOR) && !defined(VTSP_ENABLE_GAMRWB)
        case VTSP_CODER_GAMRWB_20MS_OA:
            DSP_decode(DSP_CODER_TYPE_AMRWB_OA, decOut_ptr, (uint8*)decIn_ptr,
                    pSize);
            break;
        case VTSP_CODER_GAMRWB_20MS_BE:
            DSP_decode(DSP_CODER_TYPE_AMRWB_BE, decOut_ptr, (uint8*)decIn_ptr,
                    pSize);
            break;
#endif
        /*
         * Comfort noise packet. Run NSE. NSE object was
         * initialized when coder type changed to CN.
         */
        case VTSP_CODER_CN:

#ifndef VTSP_ENABLE_MP_LITE
            stream_ptr->nseObj.dst_ptr = decOut_ptr;

            /*
             * Change NSE power if CN update.
             */
            if (0 != pSize) {
                nPowerDb = -(*(uint8 *)decIn_ptr); /* Get CN packet power */
                stream_ptr->nseObj.pwr = 
                        (nPowerDb + vtspr_ptr->dsp_ptr->cnPwrAttenDb) << 1;
            }
            NSE_generate(&stream_ptr->nseObj);
            stream_ptr->countCN++;
            stream_ptr->count.encodeCN++;
#endif
            break;

#if defined(VTSP_ENABLE_DTMFR) && !defined(VTSP_ENABLE_MP_LITE)
        case VTSP_CODER_DTMF:
            /* fall through */
        case VTSP_CODER_TONE:
            /* If pSize is zero, DR_Decode generates silence.
             */
            DR_decode(stream_ptr->drDecodeObj_ptr,
                    decIn_ptr, decOut_ptr, pSize);
            if (stream_ptr->drDecodeObj_ptr->flash) {
                if (0 == stream_ptr->drDecodeObj_ptr->flashDone) {
                    stream_ptr->drDecodeObj_ptr->flashDone = 1;
                    tic_ptr  = _VTSPR_infcToTicPtr(vtspr_ptr->dsp_ptr, infc);
                    TIC_control(tic_ptr, TIC_CONTROL_HOOK_FLASH, 0);
                }
            }
            else {
                stream_ptr->drDecodeObj_ptr->flashDone = 0;
            }
            break;
#endif

        case VTSP_CODER_UNAVAIL:
            /*
             * This condition will occur if pkts are received on
             * an unopened or unconfigured stream.
             */
            /* fall through to default */
            _VTSP_TRACE(__FILE__, __LINE__);
        default:
            /*
             * PLC for invalid packet.
             * Zero dest in case PLC does not run.
             */
            stream_ptr->countDecode--;
            if (0 != pSize) {
                stream_ptr->count.decodePkt--;
            }
#ifndef VTSP_ENABLE_MP_LITE
            plc_ptr->ctrlStat |= PLC_CTRL_BPI;
#endif
            COMM_fill(decOut_ptr, 0, VTSPR_NSAMPLES_STREAM);
            _VTSP_TRACE(__FILE__, __LINE__);
            break;
    }

#ifndef VTSP_ENABLE_MP_LITE
    /*
     * Run PLC
     */
    if (0 != (VTSPR_ALG_STREAM_PLC & stream_ptr->algStreamState)) {
        /* 
         * If coder is WB, set PLC for WB, else clear for NB
         */
        if(_VTSPR_isCoderWb(decType)) {
            plc_ptr->ctrlStat |= PLC_CTRL_WB;
        }

        /*
         * Set PLC src/dst pointer to decOut_ptr.
         *
         * If no packet was received, set the PLC bad packet flag
         * in the PLC object:
         *
         * plc_ptr->ctrlStat |= PLC_CTRL_BPI.
         *
         * If good packet was received, clear the PLC bad pkt flag
         * in the PLC object:
         *
         * plc_ptr->ctrlStat &= ~PLC_CTRL_BPI.
         */
        plc_ptr->pkt_ptr = decOut_ptr;
        PLC_run(plc_ptr);
        if (plc_ptr->ctrlStat & PLC_CTRL_BPI) {
            stream_ptr->count.runPLC++;
        }

        /*
         * If PLC has gone to absolute silence, use NFE and NSE
         * to generate comfort noise to line.
         * Update NSE power only when not generating noise thru NSE. This is to
         * avoid feedback loop of noise floor.
         */
        if (0 != (PLC_STAT_SIL & plc_ptr->ctrlStat)) {
            /*
             * Generate comfort noise at noise floor last reported
             * by NFE only if requested.
             * NSE among PLC and SID is shared and is legal.
             */
             stream_ptr->nseObj.dst_ptr = decOut_ptr;
#if defined(PROVIDER_LGUPLUS)
             /* 
              * To meet customer request, provide a value to mute NSE()
              */
             if (vtspr_ptr->dsp_ptr->cnPwrAttenDb != -35) {
                NSE_generate(&stream_ptr->nseObj);
             }
#else
             NSE_generate(&stream_ptr->nseObj);
#endif
             stream_ptr->countNSE++;
             stream_ptr->count.runNSE++;
        }
        else if (VTSP_CODER_CN != decType) {
#ifndef VTSP_ENABLE_MP_LITE
            /*
             * Update NSE power only when a legal voice packet was received.
             */
#ifdef VTSP_ENABLE_NFE
            nPowerDb = stream_ptr->nfe_ptr->noiseFloor;
#else
#ifdef VTSP_ENABLE_AEC
            nPowerDb = stream_ptr->aec_ptr->noiseFloor / 10;
#else
            nPowerDb = stream_ptr->bnd_ptr->bndNoise / 10;
#endif
#endif
            stream_ptr->nseObj.pwr = 
                    (nPowerDb + vtspr_ptr->dsp_ptr->cnPwrAttenDb) << 1;
#endif /* VTSP_ENABLE_MP_LITE. */
        }
    }
#endif
    return;
}


