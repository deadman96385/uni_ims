/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004-2010 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 30131 $ $Date: 2014-12-01 14:45:03 +0800 (Mon, 01 Dec 2014) $
 *
 */

/*
 * RFC2833 DTMF Relay
 */

#include "vtsp_rt_dr.h"
#include <osal_net.h>

#ifdef VTSP_ENABLE_DTMFR

/*
 * XXX the extern below creates a link time dependancy on vtspr (reverse dep)
 * in order to optimize data space usage
 */
extern vint _VTSPR_toneDtmfFreq[16][2];

/*
 * ======== _DR_encodePack() ========
 *
 * Performs DTMFR data packing
 *
 * Returns: packet size (in bytes)
 */
vint _DR_encodePack(
    DR_EncodeObj *encObj_ptr,
    void         *dst_ptr,
    uvint         end,
    uint32        dur)
{
    uint32 *word_ptr;
    uint32  payload;
    uint32  blockSize;

    /* Set fixed blocksize */
    if (DR_SAMPLE_RATE_16K == encObj_ptr->sampleRate) {
        blockSize = DR_BLOCKSIZE_16K;
    }
    else {
        blockSize = DR_BLOCKSIZE_8K;
    }

    word_ptr = (uint32 *)dst_ptr;

    if (DR_RELAY_MODE_EVENT == encObj_ptr->relayMode) {
        /* telephone-event type */
        /* payload */
        if (dur < 40) {
            /* Shortest digit allowable, 5 ms */
            dur = 40;
        }
        payload = dur;
        payload |= encObj_ptr->power << 16;
        payload |= encObj_ptr->digit << 24;
        if (end > 0) {
            payload |= 0x800000;
        }
        /* Format into correct byte order for network transmission */
        *word_ptr = OSAL_netHtonl(payload);
        return (DR_NUM_BYTES_EVENT);
    }
    else {
        /* telephone-tone type */
        /* first word */
        payload = blockSize;
        payload |= encObj_ptr->divMod3 << 22;
        payload |= encObj_ptr->modulation << 23;
        *word_ptr++ = OSAL_netHtonl(payload);

        /* second word */
        payload = encObj_ptr->freq1;
        payload |= encObj_ptr->freq2 << 16;
        *word_ptr++ = OSAL_netHtonl(payload);

        /* third word */
        payload = encObj_ptr->freq3;
        payload |= encObj_ptr->freq4 << 16;
        *word_ptr = OSAL_netHtonl(payload);
        return (DR_NUM_BYTES_TONE);
    }
}

/*
 * ======== DR_encode() ========
 * Performs DTMFR Encoding
 *
 * Returns: packet size (in bytes)
 */
vint DR_encode(
    DR_EncodeObj      *encObj_ptr,
    DR_EventObj       *encEvent_ptr,       /* read only */
    void              *dst_ptr)
{
    uint8  *byte_ptr;
    uvint   pSize;
    int32   dur;
    uint32  blockSize;
    uint32  startDuration;

    /* Set fixed blocksize and startDuration */
    if (DR_SAMPLE_RATE_16K == encObj_ptr->sampleRate) {
        blockSize = DR_BLOCKSIZE_16K;
        startDuration = DR_START_DUR_16K;
    }
    else {
        blockSize = DR_BLOCKSIZE_8K;
        startDuration = DR_START_DUR_8K;
    }

    pSize    = 0;
    byte_ptr = (uint8 *)dst_ptr;

    /* Get newEvent for use in this function */
    encObj_ptr->newEvent = (encEvent_ptr->event ^ encObj_ptr->lastEvent) &
            encEvent_ptr->event;

    /* Set defaults */
    encObj_ptr->status &= ~(DR_DIGIT_END);

    /*
     * The DR encoder can be forced to end a digit through the event object.
     * This might be done if DTMF Relay is disabled or DTMF detection is
     * disabled or if another telephony event (like CIDCW) interrupts DR.
     */
    if (DR_STOP_SET == encEvent_ptr->stop) {
        if (0 != (encObj_ptr->status & DR_IN_DIGIT)) {
            /* Set TE event if inside a digit and DR_DIGIT_RESET is set.  This
             * will end a digit currently being encoded.
             */
            encObj_ptr->newEvent |= DR_EVENT_TE;
        }
        encEvent_ptr->stop = DR_STOP_CLEAR;
    }

    /*
     * A special case is a leading edge while already IN_DIGIT. Then, we need to
     * finish the current digit.
     */
    if ((0 != (encObj_ptr->newEvent & DR_EVENT_LE)) &&
            (0 != (encObj_ptr->status & DR_IN_DIGIT))) {
            encEvent_ptr->event |= DR_EVENT_TE;
            encObj_ptr->newEvent |= DR_EVENT_TE;
    }

    /* If end bit is set, send redundant "E"s */
    if (encObj_ptr->end > 0) {
        pSize += _DR_encodePack(encObj_ptr, byte_ptr, 1,
                encObj_ptr->dur);
    }

    /*
     * Process the trailing edge case, and set the END bits.
     * However, ignore any trailing edge when not IN_DIGIT.
     */
    if (((encObj_ptr->totalTime >= (encObj_ptr->playTime - blockSize)) ||
            (0 != (DR_EVENT_TE & encObj_ptr->newEvent))) &&
            (0 != (DR_IN_DIGIT & encObj_ptr->status)) &&
            (0 == encObj_ptr->end)) {
        encObj_ptr->dur += blockSize;

        /* Send redundant "E"s */
        pSize += _DR_encodePack(encObj_ptr, byte_ptr + pSize, 1,
                encObj_ptr->dur);

        /* Update rtp timestamp for next packet */
        encObj_ptr->totalTime = encObj_ptr->dur;
        encObj_ptr->end = 1;
    }

    /*
     * Process the steady state case - already in digit and no new event
     * create "dtmf continuation" payload for this 10 ms block
     *
     * Timestamp offset is 0.
     * Duration sent is actual duration + lengthened amount.
     */
    if (0 != (DR_IN_DIGIT & encObj_ptr->status) && (encObj_ptr->end == 0)) {
        encObj_ptr->dur += blockSize;
        if (encObj_ptr->dur > startDuration) {
            dur = encObj_ptr->dur;
        }
        else {
            dur = startDuration;
        }

        pSize += _DR_encodePack(encObj_ptr, byte_ptr + pSize, 0, dur);
        encObj_ptr->totalTime = encObj_ptr->dur;
    }

    /*
     * Process the leading edge case.
     * New DTMF event is triggered here
     * update the new information, and then encode the first packet.
     */
    if (0 != (DR_EVENT_LE & encObj_ptr->newEvent)) {
        /* Set relayMode on LE */
        encObj_ptr->relayMode = encEvent_ptr->relayMode;
        encObj_ptr->status |= (DR_IN_DIGIT);
        encObj_ptr->dur     = -encObj_ptr->detectTeTime +
                encObj_ptr->detectLeTime;
        encObj_ptr->digit = encEvent_ptr->newDigit;
        encObj_ptr->playTime = encEvent_ptr->newPlayTime;
        encObj_ptr->power = encEvent_ptr->newPower;
        encObj_ptr->totalTime = encObj_ptr->dur;
    }

    /* Store the lastEvent for next time */
    encObj_ptr->lastEvent = encEvent_ptr->event;

    if (encObj_ptr->dur > 0xff00) {
        /* Prevent wraparound of duration.
         * Limits max tone length to approx 8 sec. (See RFC2833)
         */
        encObj_ptr->dur = 0xff00;
    }

    return (pSize);
}

/*
 * ======== DR_decode() ========
 *
 * Performs DTMFR Decoding
 *
 * Returns: linear size (in bytes)
 */
vint DR_decode(
    DR_DecodeObj      *decObj_ptr,
    void              *src_ptr,
    void              *dst_ptr,
    uvint              pSize)
{
    JB_DtmfPkt *dtmfPkt_ptr;
    uint32      blockSize;

    /* Set fixed blocksize */
    if (DR_SAMPLE_RATE_16K == decObj_ptr->sampleRate) {
        blockSize = DR_BLOCKSIZE_16K;
    }
    else {
        blockSize = DR_BLOCKSIZE_8K;
    }

    /* Handle no-pkt case.
     */
    if (pSize != sizeof(JB_DtmfPkt)) {
        /*
         * Pkt is null or invalid.
         * Always play silence.  Do not play PLC, because prior history may
         * include extraneous tone.
         */
        COMM_fill(dst_ptr, 0, blockSize);
        return (sizeof(uint16) * blockSize);
    }

    dtmfPkt_ptr = (JB_DtmfPkt *)src_ptr;

    /*
     * If new evt occurs and tone is already active, the new evt is ignored.
     * Note, evt 16 is generate FLASH.
     */
    if ((0 == (DR_IN_DIGIT & decObj_ptr->status))) {
        /* Initialize tone for new digit */
        if (dtmfPkt_ptr->pwr < (int8)(-55)) {
            /*
             * DTMF power too low.  See RFC.
             * Ignore event.
             */
            COMM_fill(dst_ptr, 0, blockSize);
            return (sizeof(uint16) * blockSize);
        }

        dtmfPkt_ptr->pwr -= 3;  /* Adjust for dual tone pwr */
        if (dtmfPkt_ptr->pwr > (int8)DR_DECODE_DIGIT_DB_MAX) {
            /* Limit max generated tone to DR_DECODE_DIGIT_DB_MAX dB
             */
            dtmfPkt_ptr->pwr = DR_DECODE_DIGIT_DB_MAX;
        }

        if (dtmfPkt_ptr->eventType) {
            /* RFC4733 Telephone-event type */
            if (16 == dtmfPkt_ptr->evt) {
                decObj_ptr->flash = 1;
            }
            else {
                dtmfPkt_ptr->evt &= 0x000F;
#ifndef VTSP_ENABLE_MP_LITE 
#ifdef VTSP_ENABLE_TONE_QUAD
                decObj_ptr->toneQuadObj_ptr->dst_ptr = dst_ptr;
                decObj_ptr->toneQuadParam_ptr->ctrlWord = GENF_DUAL;
                /* Do Dual/Quad Tone */
                decObj_ptr->toneQuadParam_ptr->tone.quad.tone1 =
                         _VTSPR_toneDtmfFreq[(uvint)dtmfPkt_ptr->evt][0];
                decObj_ptr->toneQuadParam_ptr->tone.quad.tone2 =
                         _VTSPR_toneDtmfFreq[(uvint)dtmfPkt_ptr->evt][1];
                decObj_ptr->toneQuadParam_ptr->tone.quad.t1Pw =
                    dtmfPkt_ptr->pwr << 1;
                decObj_ptr->toneQuadParam_ptr->tone.quad.t2Pw =
                         dtmfPkt_ptr->pwr << 1;
                GENF_init(decObj_ptr->toneQuadObj_ptr, decObj_ptr->globals_ptr,
                        decObj_ptr->toneQuadParam_ptr, 0);
#else
                decObj_ptr->toneObj_ptr->dst_ptr = dst_ptr;
                decObj_ptr->toneParam_ptr->tone1 =
                        _VTSPR_toneDtmfFreq[(uvint)dtmfPkt_ptr->evt][0];
                decObj_ptr->toneParam_ptr->t1Pw = dtmfPkt_ptr->pwr << 1;
                decObj_ptr->toneParam_ptr->tone2 =
                        _VTSPR_toneDtmfFreq[(uvint)dtmfPkt_ptr->evt][1];
                decObj_ptr->toneParam_ptr->t2Pw = dtmfPkt_ptr->pwr << 1;
                TONE_init(decObj_ptr->toneObj_ptr, decObj_ptr->globals_ptr,
                        decObj_ptr->toneParam_ptr);
#endif
#endif /* VTSP_ENABLE_MP_LITE. */
                decObj_ptr->digit = dtmfPkt_ptr->evt;
            }
        }
        else {
            /* RFC4733 Telephone TONE type */
            if (0 != dtmfPkt_ptr->modFreq) {
                /* Do Modulation Tone */
#ifdef VTSP_ENABLE_TONE_QUAD
                decObj_ptr->toneQuadObj_ptr->dst_ptr = dst_ptr;
                decObj_ptr->toneQuadParam_ptr->ctrlWord = GENF_MOD;
                decObj_ptr->toneQuadParam_ptr->tone.mod.carrier =
                        dtmfPkt_ptr->freq1;
                if (dtmfPkt_ptr->divMod3) {
                    decObj_ptr->toneQuadParam_ptr->tone.mod.signal
                        = dtmfPkt_ptr->modFreq / 3;
                }
                else {
                    decObj_ptr->toneQuadParam_ptr->tone.mod.signal =
                             dtmfPkt_ptr->modFreq;
                }
                decObj_ptr->toneQuadParam_ptr->tone.mod.mIndex = 100;
                decObj_ptr->toneQuadParam_ptr->tone.mod.power =
                    dtmfPkt_ptr->pwr << 1;
#else
                /* AM Tones not supported play silence */
                COMM_fill(dst_ptr, 0, blockSize);
                return (sizeof(uint16) * blockSize);
#endif

            }
            else {
#ifndef VTSP_ENABLE_MP_LITE 
#ifdef VTSP_ENABLE_TONE_QUAD
                decObj_ptr->toneQuadObj_ptr->dst_ptr = dst_ptr;
                decObj_ptr->toneQuadParam_ptr->ctrlWord = GENF_QUAD;
                /* Do Quad Tone */
                decObj_ptr->toneQuadParam_ptr->tone.quad.tone1 =
                        dtmfPkt_ptr->freq1;
                decObj_ptr->toneQuadParam_ptr->tone.quad.tone2 =
                        dtmfPkt_ptr->freq2;
                decObj_ptr->toneQuadParam_ptr->tone.quad.tone3 =
                        dtmfPkt_ptr->freq3;
                decObj_ptr->toneQuadParam_ptr->tone.quad.tone4 =
                        dtmfPkt_ptr->freq4;
                decObj_ptr->toneQuadParam_ptr->tone.quad.t1Pw =
                         dtmfPkt_ptr->pwr << 1;
                decObj_ptr->toneQuadParam_ptr->tone.quad.t2Pw =
                         dtmfPkt_ptr->pwr << 1;
                decObj_ptr->toneQuadParam_ptr->tone.quad.t3Pw =
                         dtmfPkt_ptr->pwr << 1;
                decObj_ptr->toneQuadParam_ptr->tone.quad.t4Pw =
                         dtmfPkt_ptr->pwr << 1;
                GENF_init(decObj_ptr->toneQuadObj_ptr, decObj_ptr->globals_ptr,
                        decObj_ptr->toneQuadParam_ptr, 0);
#else
                /* Do Dual Tone */
                decObj_ptr->toneObj_ptr->dst_ptr = dst_ptr;
                decObj_ptr->toneParam_ptr->tone1 = dtmfPkt_ptr->freq1;
                decObj_ptr->toneParam_ptr->t1Pw = dtmfPkt_ptr->pwr << 1;
                decObj_ptr->toneParam_ptr->tone2 = dtmfPkt_ptr->freq2;
                decObj_ptr->toneParam_ptr->t2Pw = dtmfPkt_ptr->pwr << 1;
                TONE_init(decObj_ptr->toneObj_ptr, decObj_ptr->globals_ptr,
                        decObj_ptr->toneParam_ptr);
#endif
#endif /* VTSP_ENABLE_MP_LITE. */
            }
        }
        decObj_ptr->edge   = DR_EVENT_LE;
        decObj_ptr->status |= DR_IN_DIGIT;
    }

    /* Play tone if within digit
     */
    if (0 != (DR_IN_DIGIT & decObj_ptr->status)) {
        /* Output tone to dst */
        if (decObj_ptr->flash) {
            COMM_fill(dst_ptr, 0, blockSize);
        }
        else {
#ifndef VTSP_ENABLE_MP_LITE 
#ifdef VTSP_ENABLE_TONE_QUAD
            if (GENF_DONE == GENF_tone(decObj_ptr->toneQuadObj_ptr)) {
                /*
                 * Tone max duration should never be exceeded.
                 * Should never get here.
                 */
            }
#else
            if (TONE_DONE == TONE_generate(decObj_ptr->toneObj_ptr)) {
                /*
                 * Tone max duration should never be exceeded.
                 * Should never get here.
                 */
            }
#endif
#endif /* VTSP_ENABLE_MP_LITE. */
        }
    }
    else {
        /* Should never get here.
         */
        COMM_fill(dst_ptr, 0, blockSize);
        return (sizeof(uint16) * blockSize);
    }

    /* If end bit set, init state.
     */
    if (dtmfPkt_ptr->end) {
        decObj_ptr->edge   = DR_EVENT_TE;
        if (0 == (DR_IN_DIGIT & decObj_ptr->status)) {
            /* If got end and not within a digit, this is a repeated end,
             * just generate silence.
             *
             * Should never get here.
             */
            COMM_fill(dst_ptr, 0, blockSize);
        }
        DR_decodeInit(decObj_ptr);
    }
    return (sizeof(uint16) * blockSize);
}

/*
 * ======== DR_decodeInit() ========
 *
 * Initialize DTMFR Decoder
 *
 * Returns: void
 */
void DR_decodeInit(
    DR_DecodeObj *decObj_ptr)
{
    /* Init Decode */
    decObj_ptr->status    = 0;
    decObj_ptr->flash     = 0;
    decObj_ptr->flashDone = 0;
}

/*
 * ======== DR_encodeInit() ========
 *
 * Initialize DTMFR Encoder
 *
 * Returns: void
 */
void DR_encodeInit(
    DR_EncodeObj *encObj_ptr)
{
    /* Init Encode */
    encObj_ptr->status = 0;
    encObj_ptr->lastEvent = 0;
    encObj_ptr->end = 0;
}

/*
 * ======== DR_increaseEnd() ========
 *
 * This function is the increase end filed when the DR end packet sent.
 *
 * Returns: void
 */
void DR_increaseEnd(
    DR_EncodeObj *encObj_ptr)
{
    encObj_ptr->end++;
    if (encObj_ptr->end > (vint)encObj_ptr->redundancy) {
        encObj_ptr->end = 0;
        encObj_ptr->status &= ~(DR_IN_DIGIT);
        encObj_ptr->status |= (DR_DIGIT_END);
    }
}

#endif /* VTSP_ENABLE_DTMFR */
