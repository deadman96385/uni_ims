/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 9111 $ $Date: 2009-03-12 08:59:56 +0800 (Thu, 12 Mar 2009) $
 */


#include "cids.h"

/* 
 * TONE lookup table for tone generation
 */
int _CIDS_dtmfTable[16][2] = {
    {941, 1336}, {697, 1209}, {697, 1336},  /* 0 1 2 */
    {697, 1477}, {770, 1209}, {770, 1336},  /* 3 4 5 */
    {770, 1477}, {852, 1209}, {852, 1336},  /* 6 7 8 */
    {852, 1477}, {941, 1209}, {941, 1477},  /* 9 * # */
    {697, 1633}, {770, 1633}, {852, 1633},  /* A B C */
    {941, 1633}                             /* D     */
};

/* 
 * ======== _CIDS_charToDtmfTone() ========
 */
void _CIDS_charToDtmfTone(
    char            dtmf,
    TONE_Params    *toneParams_ptr)
{
    uvint index;

    toneParams_ptr->numCads = 1;
    toneParams_ptr->make1   = 80;
    toneParams_ptr->break1  = 80;
    toneParams_ptr->repeat1 = 1;
    toneParams_ptr->repeat  = 1;

    index = dtmf - '0';
    if (index <= 9) { 
        toneParams_ptr->tone1 = _CIDS_dtmfTable[index][0];
        toneParams_ptr->tone2 = _CIDS_dtmfTable[index][1];
        return;
    }
    switch (dtmf) { 
        case 'A':
            index = 12;
            break;
        case 'B':
            index = 13;
            break;
        case 'C':
            index = 14;
            break;
        case 'D':
            index = 15;
            break;
        case '*':
            index = 10;
            break;
        case '#':
            index = 11;
            break;
        default:
            toneParams_ptr->tone1 = 0;
            toneParams_ptr->tone2 = 0;
            return;
    }

    toneParams_ptr->tone1 = _CIDS_dtmfTable[index][0];
    toneParams_ptr->tone2 = _CIDS_dtmfTable[index][1];

    return;
}

/*
 * -CIDS_init()
 *  Initializes the CIDS object.
 * -Returns
 *  None 
 */
void CIDS_init(
    CIDS_Obj      *cidsObj_ptr,
    GLOBAL_Params *globals_ptr,
    CIDS_Params   *cidsParams_ptr)
{
    _CIDS_Internal *internal_ptr;
    FSKS_LParams    fsksParams;
    TONE_Params    *toneParams_ptr;
    vint            msgSize;
    
    internal_ptr = &cidsObj_ptr->internal;
    
    /*
     * Store FSKS object pointer/ Tone object pointer.
     */
    internal_ptr->fsksObj_ptr = cidsParams_ptr->fsksObj_ptr;
    internal_ptr->toneObj_ptr = cidsParams_ptr->toneObj_ptr;
    internal_ptr->globals_ptr = globals_ptr;

    /*
     * Set up tone module if required.
     * For now support only 1 cadence. 
     */
    toneParams_ptr          = &internal_ptr->toneParams;
    toneParams_ptr->tone1   = cidsParams_ptr->tone1Freq;
    toneParams_ptr->t1Pw    = cidsParams_ptr->tone1Pwr;
    toneParams_ptr->tone2   = cidsParams_ptr->tone2Freq;
    toneParams_ptr->t2Pw    = cidsParams_ptr->tone2Pwr;
    toneParams_ptr->numCads = 1;
    toneParams_ptr->make1   = -1;
    toneParams_ptr->break1  = 0;
    toneParams_ptr->repeat1 = 1;
    toneParams_ptr->repeat  = 1;
    
    /*
     * Start from Init state and set the times.
     */
    internal_ptr->state      = _CIDS_STATE_INIT;
    internal_ptr->dataType   = cidsParams_ptr->dataType;
    internal_ptr->sigType    = cidsParams_ptr->signalType;
    internal_ptr->callTime   = CIDS_BLOCK_SIZE >> 3;
    internal_ptr->tSilStart  = cidsParams_ptr->startTime;
    internal_ptr->tSignal    = cidsParams_ptr->signalTime;
    internal_ptr->tSilPreSez = cidsParams_ptr->preSezTime;
    internal_ptr->tSilEnd    = cidsParams_ptr->endTime;
    internal_ptr->tOffHook   = cidsParams_ptr->offTimeout;
    internal_ptr->tOnHook    = cidsParams_ptr->onTimeout;
    internal_ptr->tOnSil     = cidsParams_ptr->onSilTime;
    internal_ptr->offReq     = (internal_ptr->tOffHook < 0) ? 0 : 1;
    internal_ptr->onReq      = (internal_ptr->tOnHook < 0) ? 0 : 1;
    internal_ptr->ringTime   = cidsParams_ptr->ringTime;
    internal_ptr->extension  = cidsParams_ptr->extension;
    cidsObj_ptr->stat        = CIDS_STAT_IDLE;

    if (internal_ptr->extension & CIDS_LOCALS_EXT_DATA_ONLY) { 
        /* 
         * Skip entire state machine except DATA Transmission state(s)
         */
        cidsObj_ptr->ctrl  &= ~(CIDS_CTRL_TX);
        internal_ptr->state = _CIDS_STATE_START;
    }

    /*
     * Check if battery reversal is requested.
     */
    if (0 != cidsParams_ptr->battRev) {
        internal_ptr->battRev = CIDS_STAT_BATTREV;
    }
    else {
        internal_ptr->battRev = 0;
    }

    /*
     * STAS: Init tone to mono tone.
     */
    if (CIDS_SIG_STAS == internal_ptr->sigType) {
        
        toneParams_ptr->ctrlWord  = TONE_MONO;
        TONE_init(internal_ptr->toneObj_ptr, globals_ptr, toneParams_ptr);
    }
    /*
     * DTAS: Init tone to dual tone.
     */
    else if (CIDS_SIG_DTAS == internal_ptr->sigType) {
        
        toneParams_ptr->ctrlWord  = TONE_DUAL;
        TONE_init(internal_ptr->toneObj_ptr, globals_ptr, toneParams_ptr);
        
    }
    if (CIDS_DATA_FSK == internal_ptr->dataType) { 
    
        /*
         * Init FSKS module.
         */
        fsksParams.fSPACE_FREQ    = cidsParams_ptr->spaceFreq;
        fsksParams.fMARK_FREQ     = cidsParams_ptr->markFreq;
        fsksParams.pSPACE_PWR     = cidsParams_ptr->spacePwr;
        fsksParams.pMARK_PWR      = cidsParams_ptr->markPwr;
        fsksParams.pBLOCKSIZE     = CIDS_BLOCK_SIZE;
        fsksParams.pSTUFFMARKBITS = 0;
        fsksParams.pSEIZ_LEN      = cidsParams_ptr->sezLen;
        fsksParams.pTRAIL_LEN     = 0; 
        fsksParams.pMARK_LEN      = cidsParams_ptr->markLen; 
        fsksParams.pSUBMODE       = FSKS_ONHOOK;
        /*
         * If error checking algorithm is CRC, use Japanese. 
         * All other use checksum.
         */
        fsksParams.pMODE = (CIDS_CRC == cidsParams_ptr->cType) ? 
                FSKS_JAPAN : FSKS_USA;

        /*
         * Save to check for Japanese CID
         */
        internal_ptr->pMode = fsksParams.pMODE;

        if (FSKS_JAPAN == internal_ptr->pMode) {
            /*
             * Process the CID message for Japanese CID.
             */
            internal_ptr->tSignal    = CIDS_LOCALS_SIGNALTIME_JP;
            msgSize = cidsParams_ptr->msgSz;
            cidsParams_ptr->msgSz = 
                CIDS_processMessageJcid(cidsParams_ptr->msg_ptr, msgSize);
        }
        FSKS_init(internal_ptr->fsksObj_ptr, 
                globals_ptr, 
                &fsksParams, 
                cidsParams_ptr->msg_ptr, 
                cidsParams_ptr->msgSz);
    }
    else if (CIDS_DATA_DTMF == internal_ptr->dataType) { 
        internal_ptr->msg_ptr       = cidsParams_ptr->msg_ptr;
        internal_ptr->dtmfSendReady = 0;
        internal_ptr->dtmfSendIndex = 0;
        internal_ptr->dtmfSendLen   = cidsParams_ptr->msgSz;
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
int CIDS_run(
    CIDS_Obj *cidsObj_ptr)
{
    _CIDS_Internal *internal_ptr;
    vint           *dst_ptr;
    TONE_Obj       *toneObj_ptr;
    FSKS_Obj       *fsksObj_ptr;
    uvint           callTime;
    uvint           stat;
    uvint           state;
    uvint           ctrl;

    /*
     * Store frequently used data in stack/ registers.
     */
    internal_ptr = &cidsObj_ptr->internal;
    state        = internal_ptr->state;
    callTime     = internal_ptr->callTime;
    dst_ptr      = cidsObj_ptr->dst_ptr;
    fsksObj_ptr  = internal_ptr->fsksObj_ptr;
    toneObj_ptr  = internal_ptr->toneObj_ptr;
    stat         = cidsObj_ptr->stat;
    ctrl         = cidsObj_ptr->ctrl;

    /*
     * This is done at so many places that it is done here even when valid data
     * will be overwrtitten.
     */
    COMM_fill(dst_ptr, 0, CIDS_BLOCK_SIZE);

    /*
     * Abort caller ID if the user goes offhook in any state before CID is 
     * done.
     * If the ofhook was a TE going offhook, during ACK time in Japanese CID,
     * do not abort.
     */
    if (0 != (ctrl & CIDS_CTRL_OFFHOOK) && (0 == internal_ptr->offReq)) {
        state = _CIDS_STATE_DONE;
        stat  = CIDS_STAT_ABORT;
        goto cids_restore;
    }
    
    /*
     * Run state machine.
     */
    switch (state) {
        
        /*
         * Init state, where the CIDS starts. Always consumes time equal to 
         * CIDS_BLOCK_SIZE samples time.
         */
        case _CIDS_STATE_INIT:

            /*
             * Abort if offhook : this cannot be TE offhook.
             */
            if (0 != (ctrl & CIDS_CTRL_OFFHOOK)) {
                state = _CIDS_STATE_DONE;
                stat  = 0;
            }
            
            /*
             * If battery reversal is required, indicate it as status. Let
             * let phone be in ONHOOK Idle.
             */
            stat |= internal_ptr->battRev;
            state = _CIDS_STATE_SILSTART;

            break;
        
        /*
         * Start silence
         */
        case _CIDS_STATE_SILSTART:

            /*
             * Abort if offhook : this cannot be TE offhook.
             */
            if (0 != (ctrl & CIDS_CTRL_OFFHOOK)) {
                state = _CIDS_STATE_DONE;
                stat  = 0;
            }
            
            /*
             * Fall to signal state if start silence is over.
             */
            if (internal_ptr->tSilStart <= 0) {
                /*
                 * Exit of state, along with exit conditions.
                 */
                stat &= ~CIDS_STAT_XMIT;
                state = _CIDS_STATE_SIGNAL;
            }
            else {
                stat |= CIDS_STAT_XMIT;
                internal_ptr->tSilStart -= callTime;
                break;
            }
           
        case _CIDS_STATE_SIGNAL:

            /*
             * Abort if offhook : this cannot be TE offhook.
             */
            if (0 != (ctrl & CIDS_CTRL_OFFHOOK)) {
                if (FSKS_JAPAN != internal_ptr->pMode) {
                    state = _CIDS_STATE_DONE;
                    stat  = 0;
                }
                else {
                    stat &= ~CIDS_STAT_RING;
                    stat &= ~CIDS_STAT_RING_SHORT;
                    stat &= ~CIDS_STAT_XMIT;
                    state = _CIDS_STATE_OFFHOOK;
                }
            }
            
            /*
             * Fall to pre-seize silence state if signal generation is over.
             */
            if (internal_ptr->tSignal <= 0) {
                stat &= ~CIDS_STAT_RING;
                stat &= ~CIDS_STAT_RING_SHORT;
                stat &= ~CIDS_STAT_XMIT;
                state = _CIDS_STATE_OFFHOOK;
            }
            else {
               
                /*
                 * If SHORT_RING signal is required, set the ring state.
                 */
                if (CIDS_SIG_SHORT_RING == internal_ptr->sigType) {
                    stat |= CIDS_STAT_RING_SHORT;
                    if (0 == internal_ptr->ringTime) {
                        /*
                         * Set another ring for 1 second.
                         */
                        internal_ptr->ringTime = 1000;
                    }
                    internal_ptr->ringTime -= callTime;
                }

                /*
                 * If RPAS signal is required, set the ring state.
                 */
                if (CIDS_SIG_RPAS == internal_ptr->sigType) {
                    stat |= CIDS_STAT_RING;
                }
                /*
                 * Else generate STAS or DTAS
                 */
                else if ((CIDS_SIG_STAS == internal_ptr->sigType) || 
                        (CIDS_SIG_DTAS == internal_ptr->sigType)) {
                    stat |= CIDS_STAT_XMIT;
                    toneObj_ptr->dst_ptr = dst_ptr;
                    TONE_generate(toneObj_ptr);
                }
                internal_ptr->tSignal -= callTime;
                break;
            }
            
        /*
         * This state is required only for Japanese CID.
         * XXX: the above comment is incorrect, all configs use this state
         *          except the timeout used is zero
         * This state implements timeout for TE going offhook.
         * If no offhook is detected within this time, CIDS is aborted.
         * CAR is generated during this time.
         */
        case _CIDS_STATE_OFFHOOK:
          
            /*
             * Check if hook detection time expired.
             */
            if (internal_ptr->tOffHook <= 0) {

                stat  &= ~CIDS_STAT_RING;
                stat  &= ~CIDS_STAT_RING_SHORT;
                /*
                 * Exit if offHook time expires
                 */
                if (1 == internal_ptr->offReq) {
                    state = _CIDS_STATE_WAITING;
                    stat  |= CIDS_STAT_RING;
                }
                else {
                    state = _CIDS_STATE_SILPRESEIZE;
                    break;
                }
                
            }
            else {
                
                /*
                 * Body of offhook detection.
                 */
                if (0 != (CIDS_CTRL_OFFHOOK & ctrl)) {
                    /*
                     * Offhook detected. Change state.
                     */
                    state = _CIDS_STATE_SILPRESEIZE;
                    stat  |= CIDS_STAT_OFFHOOK;
                    stat  &= ~CIDS_STAT_RING;
                    stat  &= ~CIDS_STAT_RING_SHORT;
                }
                else {
                    /*
                     * Continue ringing till not offhook.
                     */
                    if (FSKS_JAPAN != internal_ptr->pMode) {
                        stat |= CIDS_STAT_RING;
                    }
                    internal_ptr->tOffHook -= callTime;
                    break;
                }
                
            }
            
        /*
         * Pre Seize silence
         */
        case _CIDS_STATE_SILPRESEIZE:
            
            /*
             * Fall to start state if pre-seize time is over.
             */
            if (internal_ptr->tSilPreSez <= 0) {
                stat &= ~CIDS_STAT_XMIT;
                if (FSKS_JAPAN == internal_ptr->pMode) {                
                    stat |= CIDS_STAT_SEIZE_OFF;
                    stat |= CIDS_STAT_REL_OFF;
                }
                state = _CIDS_STATE_START;
            }
            else {
                stat |= CIDS_STAT_XMIT;
                internal_ptr->tSilPreSez -= callTime;
                break;
            }

        /*
         * Data Transmit start state. The time of this state varies depending
         * on data length and mark/seize number of bits.
         */
        case _CIDS_STATE_START:
            
            if (0 != (internal_ptr->extension & CIDS_LOCALS_EXT_DATA_ONLY) &&
                    (0 == (ctrl & CIDS_CTRL_TX))) { 
                /* Wait for application control, before starting FSK */
                break;
            }
            stat |= CIDS_STAT_XMIT;

           fsksObj_ptr->dst_ptr = dst_ptr;
           if (FSKS_DONE == FSKS_run(fsksObj_ptr)) {
               stat &= ~CIDS_STAT_XMIT;
               state = _CIDS_STATE_SILEND;
               if (internal_ptr->extension & CIDS_LOCALS_EXT_DATA_ONLY) { 
                   /* 
                    * Skip entire state machine except DATA Transmission
                    * state(s)
                    */
                   state = _CIDS_STATE_DONE;
                   cidsObj_ptr->ctrl &= ~(CIDS_CTRL_TX);
               }
            }
            else if (CIDS_DATA_DTMF == internal_ptr->dataType) { 
                if (internal_ptr->dtmfSendReady > 0) { 
                    toneObj_ptr->dst_ptr = dst_ptr;
                    if (TONE_DONE == TONE_generate(toneObj_ptr)) { 
                        internal_ptr->dtmfSendIndex++;
                        if (internal_ptr->dtmfSendIndex > 
                                internal_ptr->dtmfSendLen) { 
                            /* Done with all tones */
                            stat &= ~CIDS_STAT_XMIT;
                            state = _CIDS_STATE_SILEND;
                        }
                        else {
                            /* Set up next tone */
                            internal_ptr->dtmfSendReady = 0;
                            break;
                        }
                    }
                }
                else {
                    /* Init for first or subsequent DTMF tones */
                    internal_ptr->toneParams.ctrlWord  = TONE_DUAL;
                    _CIDS_charToDtmfTone(
                            internal_ptr->msg_ptr[internal_ptr->dtmfSendIndex],
                            &internal_ptr->toneParams);
                    TONE_init(internal_ptr->toneObj_ptr, 
                            internal_ptr->globals_ptr, 
                            &internal_ptr->toneParams);
                    internal_ptr->dtmfSendReady = 1;
                    break;
                }
            }
            else {
                break;
            }
            
        /*
         * End silence state.
         */
        case _CIDS_STATE_SILEND:

            /*
             * Fall to onhook state if end silence time is over.
             */
            if (internal_ptr->tSilEnd <= 0) {
                stat &= ~CIDS_STAT_XMIT;
                state = _CIDS_STATE_ONHOOK;
            }
            else {
                stat |= CIDS_STAT_XMIT;
                internal_ptr->tSilEnd -= callTime;
                break;
            }
            
        /*
         * This state is required only for Japanese CID.
         * This state implements timeout for TE going onhook.
         * If no onhook is detected within this time, CIDS is aborted.
         */
        case _CIDS_STATE_ONHOOK:
            
            /*
             * Check if hook detection time expired.
             */
            if (internal_ptr->tOnHook <= 0) {

                stat  &= ~CIDS_STAT_XMIT;

                /*
                 * Check if hook detection was requested and CID timed out.
                 */
                if (0 == internal_ptr->onReq) {
                    state = _CIDS_STATE_SILONHOOK;
                } 
                else {
                    state = _CIDS_STATE_WAITING;
                    break;
                }
                
            }
            else {
                
                /*
                 * Body of onhook detection.
                 */
                if (0 != (CIDS_CTRL_ONHOOK & ctrl)) {
                    /*
                     * Onhook detected. Change state.
                     */
                    state = _CIDS_STATE_SILONHOOK;
                    stat  &= ~CIDS_STAT_OFFHOOK;
                    stat  &= ~CIDS_STAT_XMIT;
                }
                else {
                    /*
                     * Continue till onhook.
                     */
                    stat |= CIDS_STAT_XMIT;
                    internal_ptr->tOnHook -= callTime;
                    break;
                }
                
            }

        case _CIDS_STATE_SILONHOOK:
            
            /*
             * Fall to waiting state if onhook silence time is over.
             */
            if (internal_ptr->tOnSil <= 0) {
                stat &= ~CIDS_STAT_XMIT;
                if (FSKS_JAPAN == internal_ptr->pMode) {                
                    stat &= ~CIDS_STAT_SEIZE_OFF;
                    stat &= ~CIDS_STAT_REL_OFF;
                }
                stat |= CIDS_STAT_RING;
                state = _CIDS_STATE_WAITING;
            }
            else {
                stat |= CIDS_STAT_XMIT;
                internal_ptr->tOnSil -= callTime;
                break;
            }
            
        /*
         * Wait till the phone goes offhook to reverse battery.
         */
        case _CIDS_STATE_WAITING:
            
            /*
             * If the TE has gone offhook, reverse battery. 
             * If the user does not pick up before timeout number of rings,
             * reverse battery.
             */
            if (FSKS_JAPAN == internal_ptr->pMode) {
                if ((0 != (CIDS_CTRL_OFFHOOK & ctrl)) ||
                        (0 != (CIDS_CTRL_TIMEOUT & ctrl))) {
                    /*
                    * Clear the seize and release off bits
                    */
                    stat &= ~CIDS_STAT_SEIZE_OFF;
                    stat &= ~CIDS_STAT_BATTREV;
                    stat &= ~CIDS_STAT_REL_OFF;
                    state = _CIDS_STATE_DONE;
                    stat  = 0;
                    break;
                }
            }
            else {
                state = _CIDS_STATE_DONE;
                stat  = 0;
            }
                
        /*
         * Finished generating.
         */
        case _CIDS_STATE_DONE:
            /*
             * Abort if offhook : this cannot be TE offhook.
             */
            if (0 != (ctrl & CIDS_CTRL_OFFHOOK)) {
                stat  = 0;
            }
            break;
            
    }

cids_restore:    
    /*
     * Store to object from stack. 
     */
    internal_ptr->state = state;
    if (stat != cidsObj_ptr->stat) { 
        stat |= CIDS_STAT_CHANGED;
    }
    else { 
        stat &= ~(CIDS_STAT_CHANGED);
    }

    cidsObj_ptr->stat   = stat;
  
    /*
     * Return state.
     */
    return (state);
    
}

/*
 * -CIDS_processMessageJcid();
 *  Japanese CID message has parity bits and header in it.
 *  This function takes a message and converts it to Japanese CID data.
 *  User must allocate string of length 128 pointed by msg_ptr no matter what.
 * -Returns:
 *  Length of stuffed and processed message.
 *  msg_ptr is processed and modified to length = msgLen.
 *  
 */ 
int CIDS_processMessageJcid(
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
        pMsg_ptr[5] = 0xc0; /* SERVICE 100000010 */
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
