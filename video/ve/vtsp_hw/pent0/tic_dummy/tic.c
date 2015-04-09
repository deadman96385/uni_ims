/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 7132 $ $Date: 2008-07-24 17:12:23 -0700 (Thu, 24 Jul 2008) $
 */

/*
 * TIC driver source
 */
#include "tic.h"
#include "_tic.h"

#ifndef OSAL_KERNEL_EMULATION
#include <module.h>
#endif

/*
 * User defined nSamples per 10ms status for each infc
 */
TIC_Status TIC_infcNSamples10ms[TIC_INFC_NUM] = {
    TIC_NSAMPLES_10MS_8K,               /* infc 0 */
};

/*
 * ======== TIC_init() ========
 * Initializes the low level TIC hardware.
 *
 * Return Values:
 * 0 : Success.
 * !0: Error (see error codes)
 */
TIC_Return TIC_init(
    void)
{
    return (0);
}


/*
 * ======== TIC_shutdown() ========
 * Closes the TIC module.
 *
 * Return Values:
 * 0 : Success.
 * !0: Error (see error codes)
 */
TIC_Return TIC_shutdown(
    void)
{

    return (TIC_OK);
   
}

/*
 * ======== TIC_initInfc() ========
 * Initializes a TIC interface on TIC object with specified TIC type.
 * This function cannot rely on TIC_run to schedule low level tasks.
 *
 * Return Values:
 * 0 : Success.
 * !0: Error (see error codes)
 */
TIC_Return TIC_initInfc(
    TIC_Obj *ticObj_ptr,
    vint     infcType,
    uvint    chan)
{

    /*
     * Initialize the object based on TIC object.
     */
    switch (infcType) {
        
        case TIC_INFCTYPE_AUDIO:
            break;
        
        case TIC_INFCTYPE_FXS:
        case TIC_INFCTYPE_FXO:
        default:
            return (TIC_STATUS_ERROR);
    }
   
    /*
     * Store interface type in the object
     * XXX add code for DECT here
     */
    ticObj_ptr->infcType = infcType;
    ticObj_ptr->chan     = chan;
    ticObj_ptr->nSamples10ms = TIC_infcNSamples10ms[chan];
    if ((TIC_NSAMPLES_10MS_8K != ticObj_ptr->nSamples10ms) &&
            (TIC_NSAMPLES_10MS_16K != ticObj_ptr->nSamples10ms)) {
        printf("%s:%d Invalid value for nSamples10ms on chan %d: %d\n",
                __FILE__, __LINE__, chan, ticObj_ptr->nSamples10ms);
        return (TIC_ERROR_INIT);
    }

    return (TIC_OK);
}

/*
 * ======== TIC_control() ============
 * This function controls the TIC physical interface.
 *
 * Return Values:
 *  TIC_Status enumerations
 */
TIC_Return TIC_control(
    TIC_Obj         *ticObj_ptr,
    TIC_ControlCode  ticCode, 
    vint             ticArg)
{
#if TIC_INFC_FXS_NUM > 0
    TIC_FxsObj   *fxs_ptr;
#endif
#if TIC_INFC_FXO_NUM > 0            
    TIC_FxoObj   *fxo_ptr;
#endif
#if TIC_INFC_AUDIO_NUM > 0            
    TIC_AudioObj *audio_ptr;
#endif

    /* 
     * Get pointer to specific interface object
     */
    switch (ticObj_ptr->infcType) {
#if TIC_INFC_FXS_NUM > 0
        case TIC_INFCTYPE_FXS:
            fxs_ptr = &(ticObj_ptr->fxs);
            switch (ticCode) {
                default:
                    return (TIC_ERROR_NOT_SUPPORTED);
            }
            break;
#endif
#if TIC_INFC_FXO_NUM > 0            
        case TIC_INFCTYPE_FXO:
            fxo_ptr = &(ticObj_ptr->fxo);
            switch (ticCode) {
                default:
                    return (TIC_ERROR_NOT_SUPPORTED);
            }
            break;
#endif
#if TIC_INFC_AUDIO_NUM > 0            
        case TIC_INFCTYPE_AUDIO:
            /* XXX add code for Audio interface type here */
            audio_ptr = &(ticObj_ptr->audio);
            switch (ticCode) {
                case TIC_CONTROL_LINE_STATE:
                    audio_ptr->state = ticArg;
                    return (0);
                case TIC_CONTROL_AUDIO_ATTACH:
                    if (1 == ticArg) {
                        /* Detach */
                        VHW_detach();
                    }
                    else {
                        /* Attach */
                        VHW_attach();
                    }

                    return (0);
                    
                default:
                    return (TIC_ERROR_NOT_SUPPORTED);
            }
            break;
#endif
        default:
            return (TIC_ERROR_UNKNOWN_INTERFACE);
    }
    /* Should never get here */
    return (TIC_ERROR_UNKNOWN_INTERFACE);
}

/*
 * ======== TIC_getStatus() ============
 * This function queries status from the TIC physical interface.
 *
 * Return Vaules:
 * TIC_Status enumerations
 */
TIC_Status TIC_getStatus(
    TIC_Obj           *ticObj_ptr,
    TIC_GetStatusCode  ticCode)
{
#if TIC_INFC_FXS_NUM > 0
    TIC_FxsObj   *fxs_ptr;
#endif
#if TIC_INFC_FXO_NUM > 0            
    TIC_FxoObj   *fxo_ptr;
#endif
#if TIC_INFC_AUDIO_NUM > 0            
    TIC_AudioObj *audio_ptr;
#endif
    TIC_Status  status;
    /* 
     * Get pointer to specific interface object
     */
    switch (ticObj_ptr->infcType) {
#if TIC_INFC_FXS_NUM > 0
        case TIC_INFCTYPE_FXS:
            fxs_ptr = &(ticObj_ptr->fxs);
            switch (ticCode) {
                default:
                    return (TIC_ERROR_NOT_SUPPORTED);
            }
            break;
#endif            
#if TIC_INFC_FXO_NUM > 0
        case TIC_INFCTYPE_FXO:
            fxo_ptr = &(ticObj_ptr->fxo);
            switch (ticCode) {
                default:
                    return (TIC_ERROR_NOT_SUPPORTED);
            }
            break;
#endif
#if TIC_INFC_AUDIO_NUM > 0            
        case TIC_INFCTYPE_AUDIO:
            /* add code for DECT here */
            audio_ptr = &(ticObj_ptr->audio);
            switch (ticCode) {
                case TIC_GET_STATUS_HOOK:
                    status = audio_ptr->sezRelFlag;
                    return (status);
                    
                case TIC_GET_STATUS_NSAMPLES_10MS:
                    return (ticObj_ptr->nSamples10ms);
                    
                default:
                    return (TIC_ERROR_NOT_SUPPORTED);
            }
            break;
#endif
        default:
            return (TIC_ERROR_UNKNOWN_INTERFACE);
    }
}

/*
 * ======== TIC_run() ========
 * This function must be called every 10 ms to update the hardware layer of TIC
 * with the specified TIC object parameters.
 *
 * Return Values:
 * 0 : Success.
 * !0: Error (see error codes)
 */
TIC_Return TIC_run(
    TIC_Obj *ticObj_ptr)
{
    switch (ticObj_ptr->infcType) {

        /*
         * DECT
         */
        case TIC_INFCTYPE_AUDIO:

            /*
             * Process commands.
             */

            
            return (TIC_OK);

        /*
         * FXS
         */
        case TIC_INFCTYPE_FXS:
    
        /*
         * FXO
         */
        case TIC_INFCTYPE_FXO:
            
            return (TIC_ERROR_NOT_SUPPORTED);
            
    }
    
    /*
     * Add code here for DECT
     * XXX
     */

    return (TIC_ERROR_UNKNOWN_INTERFACE);
}

/*
 * ======== TIC_setRingTable() ========
 * Sets up ring table for ring generation.
 * Note: Stops the ring if the phone is ringing.
 *
 * Return Values:
 * 0 : Success.
 * !0: Error (see error codes)
 */
TIC_Return TIC_setRingTable(
    TIC_Obj        *ticObj_ptr,
    TIC_RingParams *ringParams_ptr)
{
    return (TIC_ERROR_NOT_SUPPORTED);
}

/*
 * ======== TIC_writeReadByteCmd() ========
 * Sends control bytes to physical interface.
 *
 * Return Values:
 * Size of message to return
 */
vint TIC_writeReadByteCmd(
    TIC_Obj         *ticObj_ptr,
    uint8           *msg_ptr, 
    vint             msgSize)
{
    return (0);
}

/*
 * ======== TIC_getByteEvent() ========
 * Gets status bytes from the physical interface.
 *
 * Return Values:
 * number of bytes in msg_ptr
 */
vint TIC_getByteEvent(
    TIC_Obj         *ticObj_ptr,
    uint8           *msg_ptr, 
    vint             msgSize)
{
    return (0);
}

/* TIC functions exported for Linux kernel modules */
#ifndef OSAL_KERNEL_EMULATION
EXPORT_SYMBOL(TIC_init);
EXPORT_SYMBOL(TIC_run);
EXPORT_SYMBOL(TIC_shutdown);
EXPORT_SYMBOL(TIC_initInfc);
EXPORT_SYMBOL(TIC_setRingTable);
EXPORT_SYMBOL(TIC_control);
EXPORT_SYMBOL(TIC_getStatus);
EXPORT_SYMBOL(TIC_writeReadByteCmd);
EXPORT_SYMBOL(TIC_getByteEvent);
#endif
