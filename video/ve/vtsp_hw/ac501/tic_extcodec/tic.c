/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2009 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 12522 $ $Date: 2010-07-13 15:52:44 -0700 (Tue, 13 Jul 2010) $
 */

/*
 * TIC driver source
 */
#include "tic.h"
#include "_tic.h"

/*
 * User defined nSamples per 10ms status for each infc
 */
TIC_Status TIC_infcNSamples10ms[TIC_INFC_NUM] = {
#ifdef VTSP_ENABLE_STREAM_8K
    TIC_NSAMPLES_10MS_8K,                /* infc 0 */
#else
    TIC_NSAMPLES_10MS_16K,               /* infc 0 */
#endif
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
    return (TIC_OK);
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
            return (TIC_ERROR_UNKNOWN_INTERFACE);
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
        OSAL_logMsg("%s:%d Invalid value for nSamples10ms on chan %d: %d\n",
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
            fxs_ptr = &(ticObj_ptr->infc.fxs);
            switch (ticCode) {
                default:
                    return (TIC_ERROR_NOT_SUPPORTED);
            }
            break;
#endif
#if TIC_INFC_FXO_NUM > 0
        case TIC_INFCTYPE_FXO:
            fxo_ptr = &(ticObj_ptr->infc.fxo);
            switch (ticCode) {
                default:
                    return (TIC_ERROR_NOT_SUPPORTED);
            }
            break;
#endif
#if TIC_INFC_AUDIO_NUM > 0
        case TIC_INFCTYPE_AUDIO:
            /* XXX add code for Audio interface type here */
            audio_ptr = &(ticObj_ptr->infc.audio);
            switch (ticCode) {
                case TIC_CONTROL_LINE_STATE:
                    audio_ptr->state = ticArg;
                    return (TIC_OK);
                case TIC_CONTROL_AUDIO_ATTACH:
                    if (1 == ticArg) {
                        /* Detach */
                        _VHW_detach();
                    }
                    else {
                        /* Attach */
                        _VHW_attach();
                    }

                    return (TIC_OK);
                default:
                    return (TIC_ERROR_NOT_SUPPORTED);
            }
#endif
        default:
            return (TIC_ERROR_UNKNOWN_INTERFACE);
    }
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
            fxs_ptr = &(ticObj_ptr->infc.fxs);
            switch (ticCode) {
                default:
                    return (TIC_STATUS_ERROR);
            }
            break;
#endif
#if TIC_INFC_FXO_NUM > 0
        case TIC_INFCTYPE_FXO:
            fxo_ptr = &(ticObj_ptr->infc.fxo);
            switch (ticCode) {
                default:
                    return (TIC_STATUS_ERROR);
            }
            break;
#endif
#if TIC_INFC_AUDIO_NUM > 0
        case TIC_INFCTYPE_AUDIO:
            /* add code for DECT here */
            audio_ptr = &(ticObj_ptr->infc.audio);
            switch (ticCode) {
                case TIC_GET_STATUS_HOOK:
                    status = (TIC_Status) audio_ptr->sezRelFlag;
                    return (status);
                case TIC_GET_STATUS_NSAMPLES_10MS:
                    return (ticObj_ptr->nSamples10ms);
                case TIC_GET_STATUS_DRIVER_ATTACH:
                    if (VHW_isSleeping()){
                        return (TIC_DRIVER_DETACHED);
                    }
                    else {
                        return (TIC_DRIVER_ATTACHED);
                    }

                default:
                    return (TIC_STATUS_ERROR);
            }
#endif
        default:
            return (TIC_STATUS_ERROR);
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

