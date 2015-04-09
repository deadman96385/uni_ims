/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004-2010 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 28394 $ $Date: 2014-08-21 10:24:05 +0800 (Thu, 21 Aug 2014) $
 */

#include "cidcws.h"

/*
 * -CIDCWS_init()
 *  Initializes the CIDCWS object.
 * -Returns
 *  None 
 */

void CIDCWS_init(
    CIDCWS_Obj    *cidsObj_ptr,
    GLOBAL_Params *globals_ptr,
    CIDCWS_Params *cidsParams_ptr)
{
    _CIDCWS_Internal *internal_ptr;
    FSKS_LParams     fsksParams;
    TONE_Params      toneParams;
    vint             msgSize; 
    
    internal_ptr = &cidsObj_ptr->internal;
    
    /*
     * Store FSKS object pointer/ Tone object pointer.
     */
    internal_ptr->fsksObj_ptr = cidsParams_ptr->fsksObj_ptr;
    internal_ptr->toneObj_ptr = cidsParams_ptr->toneObj_ptr;
#ifdef VTSP_ENABLE_TONE_QUAD
    internal_ptr->genfObj_ptr = cidsParams_ptr->genfObj_ptr;
#endif
    internal_ptr->sigType     = cidsParams_ptr->signalType;
    internal_ptr->ackType     = cidsParams_ptr->ackType;

    /*
     * Set up tone module for SAS. For CAS, TONE will be re-initialized.
     */
    toneParams.tone1    = cidsParams_ptr->sasFreq;
    toneParams.t1Pw     = cidsParams_ptr->sasPwr;
    toneParams.ctrlWord = TONE_MONO;
    toneParams.numCads  = 1;
    toneParams.make1    = -1;
    toneParams.break1   = 0;
    toneParams.repeat1  = 1;
    toneParams.repeat   = 1;
    TONE_init(internal_ptr->toneObj_ptr, globals_ptr, &toneParams);
    
    /*
     * Init FSKS module.
     */
    fsksParams.fSPACE_FREQ    = cidsParams_ptr->spaceFreq;
    fsksParams.fMARK_FREQ     = cidsParams_ptr->markFreq;
    fsksParams.pSPACE_PWR     = cidsParams_ptr->spacePwr;
    fsksParams.pMARK_PWR      = cidsParams_ptr->markPwr;
    fsksParams.pBLOCKSIZE     = CIDCWS_BLOCK_SIZE;
    fsksParams.pSTUFFMARKBITS = 0;
    fsksParams.pSEIZ_LEN      = cidsParams_ptr->sezLen;
    fsksParams.pTRAIL_LEN     = 20; 
    fsksParams.pMARK_LEN      = cidsParams_ptr->markLen; 
    fsksParams.pSUBMODE       = FSKS_OFFHOOK;
    /*
     * If error checking algorithm is CRC, use Japanese. 
     * All other use checksum.
     */
    fsksParams.pMODE = (CIDCWS_CRC == cidsParams_ptr->cType) ? 
            FSKS_JAPAN : FSKS_USA;
    if (FSKS_JAPAN == fsksParams.pMODE) {
        msgSize = cidsParams_ptr->msgSz;
        cidsParams_ptr->msgSz = 
            CIDCWS_processMessageJcid(cidsParams_ptr->msg_ptr, msgSize);
    }
    FSKS_init(internal_ptr->fsksObj_ptr, globals_ptr, &fsksParams, 
            cidsParams_ptr->msg_ptr, cidsParams_ptr->msgSz);

    /*
     * Start from Init state and set the times.
     */
    cidsObj_ptr->dtmfDigit    = -1;
    internal_ptr->state       = _CIDCWS_STATE_SILSTART;
    internal_ptr->callTime    = CIDCWS_BLOCK_SIZE >> 3;
    internal_ptr->tSilStart   = cidsParams_ptr->startTime;
    internal_ptr->tSas        = cidsParams_ptr->sasTime;
    internal_ptr->tSilIntr    = cidsParams_ptr->intrTime;
    internal_ptr->tCasMake    = cidsParams_ptr->casTime;
    internal_ptr->tCasBreak   = CIDCWS_LOCALS_CASBREAKTIME_NTT;
    internal_ptr->tAck        = cidsParams_ptr->ackTimeout;
    internal_ptr->tSilPreXmit = cidsParams_ptr->preXmitTime;
    internal_ptr->tSilEnd     = cidsParams_ptr->endTime;
    internal_ptr->toneInit    = 0;
    internal_ptr->casPwr      = cidsParams_ptr->casPwr - 6;
    internal_ptr->p0db        = globals_ptr->p0DBOUT;
    internal_ptr->extension   = cidsParams_ptr->extension;
    internal_ptr->pMode       = fsksParams.pMODE;

    if (internal_ptr->extension & CIDCWS_LOCALS_EXT_DATA_ONLY) { 
        /* 
         * Skip entire state machine except DATA Transmission
         * state(s)
         */
        internal_ptr->state = _CIDCWS_STATE_START;
        cidsObj_ptr->ctrl &= ~(CIDCWS_CTRL_TX);
    }

}


/*
 * -CID_run()
 *  Run the caller ID send module. Fills the buffer pointed by CID_Obj.dst_ptr
 *  with caller ID data.
 *
 * -Returns
 *  0 : If done generating CID data.
 *  x : If not done.
 */
int CIDCWS_run(
    CIDCWS_Obj *cidsObj_ptr)
{
    _CIDCWS_Internal *internal_ptr;
    vint             *dst_ptr;
    TONE_Obj         *toneObj_ptr;
    FSKS_Obj         *fsksObj_ptr;
#ifdef VTSP_ENABLE_TONE_QUAD
    GENF_Obj         *genfObj_ptr;
#endif
    TONE_Params       toneParams;
    GLOBAL_Params     globals;
    uvint             callTime;
    uvint             state;
    uvint             ctrl;

    /*
     * Store frequently used data in stack/ registers.
     */
    internal_ptr    = &cidsObj_ptr->internal;
    state           = internal_ptr->state;
    callTime        = internal_ptr->callTime;
    dst_ptr         = cidsObj_ptr->dst_ptr;
    fsksObj_ptr     = internal_ptr->fsksObj_ptr;
    toneObj_ptr     = internal_ptr->toneObj_ptr;
#ifdef VTSP_ENABLE_TONE_QUAD
    genfObj_ptr     = internal_ptr->genfObj_ptr;
#endif
    globals.p0DBOUT = internal_ptr->p0db;
    ctrl            = cidsObj_ptr->ctrl;

    /*
     * This is done at so many places that it is done here even when valid data
     * will be overwrtitten.
     */
    COMM_fill(dst_ptr, 0, CIDCWS_BLOCK_SIZE);

    /*
     * Run state machine.
     */
    switch (state) {
        
        /*
         * Start silence
         */
        case _CIDCWS_STATE_SILSTART:

            /*
             * Fall to signal state if start silence is over.
             */
            if (internal_ptr->tSilStart <= 0) {
                /*
                 * Exit of state, along with exit conditions.
                 */
                state = _CIDCWS_STATE_SAS;
            }
            else {
                internal_ptr->tSilStart -= callTime;
                break;
            }
           
        /*
         * Generate SAS signal
         * For JP Type2 CID, it should be IIT signal NOT SAS signal
         * The format of JP Call Waiting Caller Id is as follows.
         * vvvvvvvvv       vvvvv      vvvvv     vvvvvvvv     vvvvvv
         * IIT-A           IIT-B      IIT-B     DTMF(C&D)    Modem Signal
         */
        case _CIDCWS_STATE_SAS:
            if (internal_ptr->tSas <= 0) {
                if (FSKS_JAPAN == internal_ptr->pMode) {
                      state = _CIDCWS_STATE_SAS_IIT;
                }
                  else {    
                state = _CIDCWS_STATE_SILINTR;
            }
            }
            else {
                if (FSKS_JAPAN == internal_ptr->pMode) {
                    internal_ptr->tSas -= callTime;
#ifdef VTSP_ENABLE_TONE_QUAD
                    toneRetVal= GENF_tone(genfObj_ptr);
                    if (GENF_TONE == genfObj_ptr->mode) {
                /*
                         * tone make, copy data from toneOut_ary to audioOut_ary
                         */
                        COMM_copy(cidsObj_ptr->dst_ptr, 
                                genfObj_ptr->dst_ptr,
                                CIDCWS_BLOCK_SIZE);
                        cidsObj_ptr->ctrl &= ~(CIDCWS_CTRL_TONE_TRAILING);
                    }
                    if (GENF_DONE == toneRetVal) {
                        cidsObj_ptr->ctrl &= ~(CIDCWS_CTRL_TONE_TRAILING);
                        cidsObj_ptr->ctrl |= CIDCWS_CTRL_TONE_INIT;
                        state = _CIDCWS_STATE_SAS_IIT;
                    }
                    else {
                        /* Tone not done, Continue generating tone */
                        break;
                    }
#endif
                }
                else {
                    /*
                 * Else generate SAS
                 */
                toneObj_ptr->dst_ptr = dst_ptr;
                TONE_generate(toneObj_ptr);
                internal_ptr->tSas -= callTime;
                break;
            }
            }
            
        /*
         * generate IIT-B for JP Type2 CID 
         */
        case _CIDCWS_STATE_SAS_IIT:
#ifdef VTSP_ENABLE_TONE_QUAD
                    toneRetVal= GENF_tone(genfObj_ptr);
                    if (GENF_TONE == genfObj_ptr->mode) {
                        /*
                         * tone make, copy data from toneOut_ary to audioOut_ary
                         */
                        COMM_copy(cidsObj_ptr->dst_ptr, 
                                genfObj_ptr->dst_ptr,
                                CIDCWS_BLOCK_SIZE);
                        cidsObj_ptr->ctrl &= ~(CIDCWS_CTRL_TONE_TRAILING);
                    }
                    if (GENF_DONE == toneRetVal) {
                        cidsObj_ptr->ctrl &= ~(CIDCWS_CTRL_TONE_TRAILING);
                        state = _CIDCWS_STATE_SILINTR;
                    }
                    else {
                        /* Tone not done, Continue generating tone */
                        break;
                    }
#endif
        /*
         * Start silence
         */
        case _CIDCWS_STATE_SILINTR:

            /*
             * Fall to signal state if start silence is over.
             */
            if (internal_ptr->tSilIntr <= 0) {
                /*
                 * Exit of state, along with exit conditions.
                 */
                state = _CIDCWS_STATE_CAS;

                /*
                 * Initialize tone module, for the next state.
                 * If this is CAS tone.
                 */
                if (CIDCWS_SIG_CAS == internal_ptr->sigType) {

                    toneParams.tone1    = CIDCWS_CAS_FREQ1;
                    toneParams.tone2    = CIDCWS_CAS_FREQ2;
                    toneParams.t1Pw     = internal_ptr->casPwr;
                    toneParams.t2Pw     = toneParams.t1Pw;
                    toneParams.ctrlWord = TONE_DUAL;
                    toneParams.numCads  = 1;
                    toneParams.make1    = internal_ptr->tCasMake;
                    toneParams.break1   = 0;
                    toneParams.repeat1  = 1;
                    toneParams.repeat   = 1;
                }
                /*
                 * This is first portion of CAT tone.
                 * Note that the CAT tone parameters cannot be configured
                 * individualy.  
                 * CAT part 'a' time = CAT part 'b' time = CAT part 'c' time =
                 *     tCAS
                 */
                else {
                    toneParams.tone1    = CIDCWS_CAT_FREQ1;
                    toneParams.tone2    = CIDCWS_CAT_FREQ2;
                    toneParams.t1Pw     = internal_ptr->casPwr;
                    toneParams.t2Pw     = toneParams.t1Pw;
                    toneParams.ctrlWord = TONE_DUAL;
                    toneParams.numCads  = 1;
                    toneParams.make1    = internal_ptr->tCasMake;
                    toneParams.break1   = internal_ptr->tCasBreak;
                    toneParams.repeat1  = 1;
                    toneParams.repeat   = 1;
                }
                TONE_init(internal_ptr->toneObj_ptr, &globals, &toneParams);
            }
            else {
                internal_ptr->tSilIntr -= callTime;
                break;
            }

        /*
         * Generate CAS/CAT signal
         */
        case _CIDCWS_STATE_CAS:

            /*
             *
             */
            toneObj_ptr->dst_ptr = dst_ptr;
            if (TONE_DONE == TONE_generate(toneObj_ptr)) {
                
                if ((CIDCWS_SIG_CAT == internal_ptr->sigType) &&
                        (0 == internal_ptr->toneInit)) {
                    
                    internal_ptr->toneInit = 1;

                    toneParams.tone1    = CIDCWS_CAT_FREQ3;
                    toneParams.tone2    = CIDCWS_CAT_FREQ4;
                    toneParams.t1Pw     = internal_ptr->casPwr;
                    toneParams.t2Pw     = toneParams.t1Pw;
                    toneParams.ctrlWord = TONE_DUAL;
                    toneParams.numCads  = 1;
                    toneParams.make1    = internal_ptr->tCasMake;
                    toneParams.break1   = 0;
                    toneParams.repeat1  = 1;
                    toneParams.repeat   = 1;
                    
                    TONE_init(internal_ptr->toneObj_ptr, &globals,
                            &toneParams);
                    /*
                     * Keep generating CAT tone, part 'c'.
                     */
                    break;
                }
                
                state = _CIDCWS_STATE_ACK;
            }
            else {
                /* Tone not done, Continue generating tone */
                break;
            }
            
        /*
         * The ACK state.
         */
        case _CIDCWS_STATE_ACK:

            /*
             * Check if ack detection in requested.
             */
            if (CIDCWS_ACK_NONE == internal_ptr->ackType) {
                state = _CIDCWS_STATE_SILPREXMIT;
            }
            /*
             * If timeout, abort CID send.
             */
            else if (internal_ptr->tAck < 0) {
                state = _CIDCWS_STATE_DONE;
                break;
                
            }
            /*
             * Body of detection.
             */
            else {
                if (CIDCWS_ACK_DTMF_D == cidsObj_ptr->dtmfDigit) {
                    /*
                     * DTMF digit D detected. Change state.
                     */
                    state = _CIDCWS_STATE_SILPREXMIT;
                }
                else {
                    internal_ptr->tAck -= callTime;
                }
                break;
            }
            
        /*
         * Pre Seize silence
         */
        case _CIDCWS_STATE_SILPREXMIT:
            
            /*
             * Fall to start state if pre-xmit silence is over.
             */
            if (internal_ptr->tSilPreXmit <= 0) {
                /*
                 * Exit of state, along with exit conditions.
                 */
                state = _CIDCWS_STATE_START;
            }
            else {
                internal_ptr->tSilPreXmit -= callTime;
                break;
            }

        /*
         * FSKS start state. The time of this state varies depending on data
         * length and mark/seize number of bits.
         */
        case _CIDCWS_STATE_START:
            
            if (0 != (internal_ptr->extension & CIDCWS_LOCALS_EXT_DATA_ONLY) &&
                    (0 == (ctrl & CIDCWS_CTRL_TX))) { 
                /* Wait for application control, before starting FSK */
                break;
            }

            fsksObj_ptr->dst_ptr = dst_ptr;
            if (FSKS_DONE == FSKS_run(fsksObj_ptr)) {
                state = _CIDCWS_STATE_SILEND;
                if (internal_ptr->extension & CIDCWS_LOCALS_EXT_DATA_ONLY) { 
                    /* 
                     * Skip entire state machine except DATA Transmission
                     * state(s)
                     */
                    state = _CIDCWS_STATE_DONE;
                    cidsObj_ptr->ctrl &= ~(CIDCWS_CTRL_TX);
                }

            }
            else {
                break;
            }
            
        /*
         * End silence state.
         */
        case _CIDCWS_STATE_SILEND:

            /*
             * Fall to done state if end silence time is over.
             */
            if (internal_ptr->tSilEnd <= 0) {
                state = _CIDCWS_STATE_DONE;
            }
            else {
                internal_ptr->tSilEnd -= callTime;
                break;
            }
           
        /*
         * Finished generating.
         */
        case _CIDCWS_STATE_DONE:
            break;
            
    }
    
    /*
     * Store to object from stack. 
     */
    if (state != internal_ptr->state) { 
        /* State change */
    }

    internal_ptr->state = state;
  
    /*
     * Return state.
     */
    return (state);
    
}


/*
 * -_CIDCWS_stuffParityEven
 *  Stuffs even parity on the LSB of input data value acc.
 *  Left shifts data in acc by 1.
 * -Returns:
 *  Data stuffed with parity bit.
 */
#if 0
static uint8 _CIDCWS_stuffParityEven(
    uint8 acc)
{
    uint8 parity;

    parity =  acc & 0x1;
    parity ^= (acc >> 1) & 0x1;
    parity ^= (acc >> 2) & 0x1;
    parity ^= (acc >> 3) & 0x1;
    parity ^= (acc >> 4) & 0x1;
    parity ^= (acc >> 5) & 0x1;
    parity ^= (acc >> 6) & 0x1;
    
    return ((acc << 1) + parity);

}
#endif
        

/*
 * -CIDCWS_processMessageJcid();
 *  Japanese CID message has parity bits and header in it.
 *  This function takes a message and converts it to Japanese CID data.
 *  User must allocate string of length 128 pointed by msg_ptr no matter what.
 * -Returns:
 *  Length of stuffed and processed message.
 *  msg_ptr is processed and modified to length = msgLen.
 *  
 */ 
int CIDCWS_processMessageJcid(
    uint8 *msg_ptr,
    int    msgLen)
{
    vint    i;
    vint    j;
    uint8   lclBuf[256];
    uint8  *pMsg_ptr;
    uint8  *lclMsg_ptr;
    vint    len;
    vint    parityLen;
    uint8   tmpMsg;
    
    len = 0;
    pMsg_ptr = &lclBuf[0];

    /*
     * Skip service type as it needs to be set to 0xc0 always. Usual parity 
     * check does not work for this octet.
     */
    lclMsg_ptr = msg_ptr + 1;

    if (msg_ptr != NULL) {

        /* The following order should not be changed */
        pMsg_ptr[0] = 0x90; /* DLE 10010000 */
        pMsg_ptr[1] = 0x81; /* SOH 10000001 */
        pMsg_ptr[2] = 0x87; /* HEADER 10000111 */
        pMsg_ptr[3] = 0x90; /* DLE 10010000 */
        pMsg_ptr[4] = 0x82; /* STX 100000010 */
        pMsg_ptr[5] = 0x41; /* SERVICE 01000001 */
        len += 6;

    /*
         * Message string.
     */
        for (i = 0; i < msgLen - 1; i++) {
            parityLen = 0;
            tmpMsg = lclMsg_ptr[i];
            pMsg_ptr[len] = lclMsg_ptr[i];
            for (j = 0; j < 8; j++) {
                parityLen += (tmpMsg & 0x01);
                tmpMsg >>= 1;
    }
            if (0 != (parityLen & 0x1)) pMsg_ptr[len] |= 0x80;
            len++;
        }        
    
        /* 
         * Trailing octets
         */
        pMsg_ptr[len] = 0x90; len++; /* DLE */
        pMsg_ptr[len] = 0x03; len++; /* ETX */
        pMsg_ptr[len] = 0x00;        /* dummy */
        /*
         * Copy local buffer to buffer used by FSKS
         */
        for (i = 0; i < sizeof(lclBuf); i++) {
            *msg_ptr++ = *pMsg_ptr++;
}
    }
    return len;
}
