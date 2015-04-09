/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2008 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 29023 $ $Date: 2014-09-25 17:28:52 +0800 (Thu, 25 Sep 2014) $
 *
 */

#include <osal.h>
#include "dsp.h"
#include "dsp_extern.h"

/*
 * ======== DSP_init() ========
 * Called once upon start up to initialize DSP object
 *
 * Return Value:
 *
 */
DSP_Return DSP_init(
    void)
{
    if (DSPEXT_OK != DSPExt_init()) {
        return (DSP_ERROR);
    }
    return (DSP_OK);
}

/*
 * ======== DSP_shutdown() ========
 * This function is used to shutdown DSP
 *
 * Return Value:
 *
 */
void DSP_shutdown(
    void)
{
    DSPExt_shutdown();
}

/*
 * ======== DSP_getInstance() ========
 * Returns the next available (uninitialized) instance for the given coderType
 *
 * Return Value:
 *  NULL
 */
DSP_Instance DSP_getInstance(
    DSP_CoderType       coderType,
    vint                encDec)
{
    /*
     * Always return NULL, because instance is not used in this platform(ac908).
     */
    return (NULL);
}

/*
 * ======== DSP_encodeInit() ========
 * Calls PLATFORM specific encoder init function
 * Return Value:
 *      instance, or error
 */
DSP_Instance DSP_encodeInit(
    DSP_Instance        instance,       /* specifies instance */
    DSP_CoderType       coderType,      /* enumeration for coder type */
    uint32              dspInitParam)   /* coder specific init parameter */
{
    DSPExt_CoderType    type;
    vint                rateMode;
    vint                vadEnable;

    /* Check if turn on VAD */
    vadEnable = dspInitParam & DSP_VAD_ENABLE;

    /* Indicate coder type and bit rate */
    switch (coderType) {
        case DSP_CODER_TYPE_AMRNB_OA:
        case DSP_CODER_TYPE_AMRNB_BE:
            if (DSP_CODER_TYPE_AMRNB_OA == coderType) {
                type = DSPEXT_CODER_TYPE_AMRNB_OA;
            }
            else {
                type = DSPEXT_CODER_TYPE_AMRNB_BE;
            }

            if (0 != (dspInitParam & DSP_GAMRNB_BITRATE_475)) {
                rateMode = DSPEXT_AMRNB_FRAME_TYPE_475;
            }
            else if (0 != (dspInitParam & DSP_GAMRNB_BITRATE_515)){
                rateMode = DSPEXT_AMRNB_FRAME_TYPE_515;
            }
            else if (0 != (dspInitParam & DSP_GAMRNB_BITRATE_590)) {
                rateMode = DSPEXT_AMRNB_FRAME_TYPE_590;
            }
            else if (0 != (dspInitParam & DSP_GAMRNB_BITRATE_670)) {
                rateMode = DSPEXT_AMRNB_FRAME_TYPE_670;
            }
            else if (0 != (dspInitParam & DSP_GAMRNB_BITRATE_740)) {
                rateMode = DSPEXT_AMRNB_FRAME_TYPE_740;
            }
            else if (0 != (dspInitParam & DSP_GAMRNB_BITRATE_795)) {
                rateMode = DSPEXT_AMRNB_FRAME_TYPE_795;
            }
            else if (0 != (dspInitParam & DSP_GAMRNB_BITRATE_1020)) {
                rateMode = DSPEXT_AMRNB_FRAME_TYPE_1020;
            }
            else {
                rateMode = DSPEXT_AMRNB_FRAME_TYPE_1220;
            }
            break;
        case DSP_CODER_TYPE_AMRWB_OA:
        case DSP_CODER_TYPE_AMRWB_BE:
            if (DSP_CODER_TYPE_AMRWB_OA == coderType) {
                type = DSPEXT_CODER_TYPE_AMRWB_OA;
            }
            else {
                type = DSPEXT_CODER_TYPE_AMRWB_BE;
            }

            if (0 != (dspInitParam & DSP_GAMRWB_BITRATE_660)) {
                rateMode = DSPEXT_AMRWB_FRAME_TYPE_660;
            }
            else if (0 != (dspInitParam & DSP_GAMRWB_BITRATE_885)){
                rateMode = DSPEXT_AMRWB_FRAME_TYPE_885;
            }
            else if (0 != (dspInitParam & DSP_GAMRWB_BITRATE_1265)){
                rateMode = DSPEXT_AMRWB_FRAME_TYPE_1265;
            }
            else if (0 != (dspInitParam & DSP_GAMRWB_BITRATE_1425)){
                rateMode = DSPEXT_AMRWB_FRAME_TYPE_1425;
            }
            else if (0 != (dspInitParam & DSP_GAMRWB_BITRATE_1585)){
                rateMode = DSPEXT_AMRWB_FRAME_TYPE_1585;
            }
            else if (0 != (dspInitParam & DSP_GAMRWB_BITRATE_1825)){
                rateMode = DSPEXT_AMRWB_FRAME_TYPE_1825;
            }
            else if (0 != (dspInitParam & DSP_GAMRWB_BITRATE_1985)){
                rateMode = DSPEXT_AMRWB_FRAME_TYPE_1985;
            }
            else if (0 != (dspInitParam & DSP_GAMRWB_BITRATE_2305)){
                rateMode = DSPEXT_AMRWB_FRAME_TYPE_2305;
            }
            else {
                rateMode = DSPEXT_AMRWB_FRAME_TYPE_2385;
            }
            break;
        default:
            /* Not yet implemented */
            OSAL_logMsg("%s:%d DSP coder Type not implemented\n",
                    __FILE__,__LINE__);
            return ((DSP_Instance)NULL);
    }

    /* Start external codec */
    if (DSPEXT_OK != DSPExt_codecStart(type, rateMode, vadEnable)) {
        OSAL_logMsg("%s:%d External codec start FAILED.\n",
                __FUNCTION__, __LINE__);
        return ((DSP_Instance)NULL);
    }

    /* This platform does not need to return DSP_Instance */
    return ((DSP_Instance)NULL);
}

/*
 * ======== DSP_decodeInit() ========
 * Calls PLATFORM specific decoder init function
 * Return Value:
 *      instance, or error
 */
DSP_Instance DSP_decodeInit(
    DSP_Instance        instance,       /* specifies instance */
    DSP_CoderType       coderType,      /* enumeration for coder type */
    uint32              dspInitParam)   /* coder specific init parameter */
{
    DSPExt_CoderType    type;
    vint                rateMode;
    vint                vadEnable;

    /* Check if turn on VAD */
    vadEnable = dspInitParam & DSP_VAD_ENABLE;

    /* Indicate coder type and bit rate */
    switch (coderType) {
        case DSP_CODER_TYPE_AMRNB_OA:
        case DSP_CODER_TYPE_AMRNB_BE:
            if (DSP_CODER_TYPE_AMRNB_OA == coderType) {
                type = DSPEXT_CODER_TYPE_AMRNB_OA;
            }
            else {
                type = DSPEXT_CODER_TYPE_AMRNB_BE;
            }

            if (0 != (dspInitParam & DSP_GAMRNB_BITRATE_475)) {
                rateMode = DSPEXT_AMRNB_FRAME_TYPE_475;
            }
            else if (0 != (dspInitParam & DSP_GAMRNB_BITRATE_515)){
                rateMode = DSPEXT_AMRNB_FRAME_TYPE_515;
            }
            else if (0 != (dspInitParam & DSP_GAMRNB_BITRATE_590)) {
                rateMode = DSPEXT_AMRNB_FRAME_TYPE_590;
            }
            else if (0 != (dspInitParam & DSP_GAMRNB_BITRATE_670)) {
                rateMode = DSPEXT_AMRNB_FRAME_TYPE_670;
            }
            else if (0 != (dspInitParam & DSP_GAMRNB_BITRATE_740)) {
                rateMode = DSPEXT_AMRNB_FRAME_TYPE_740;
            }
            else if (0 != (dspInitParam & DSP_GAMRNB_BITRATE_795)) {
                rateMode = DSPEXT_AMRNB_FRAME_TYPE_795;
            }
            else if (0 != (dspInitParam & DSP_GAMRNB_BITRATE_1020)) {
                rateMode = DSPEXT_AMRNB_FRAME_TYPE_1020;
            }
            else {
                rateMode = DSPEXT_AMRNB_FRAME_TYPE_1220;
            }
            break;
        case DSP_CODER_TYPE_AMRWB_OA:
        case DSP_CODER_TYPE_AMRWB_BE:
            if (DSP_CODER_TYPE_AMRWB_OA == coderType) {
                type = DSPEXT_CODER_TYPE_AMRWB_OA;
            }
            else {
                type = DSPEXT_CODER_TYPE_AMRWB_BE;
            }

            if (0 != (dspInitParam & DSP_GAMRWB_BITRATE_660)) {
                rateMode = DSPEXT_AMRWB_FRAME_TYPE_660;
            }
            else if (0 != (dspInitParam & DSP_GAMRWB_BITRATE_885)){
                rateMode = DSPEXT_AMRWB_FRAME_TYPE_885;
            }
            else if (0 != (dspInitParam & DSP_GAMRWB_BITRATE_1265)){
                rateMode = DSPEXT_AMRWB_FRAME_TYPE_1265;
            }
            else if (0 != (dspInitParam & DSP_GAMRWB_BITRATE_1425)){
                rateMode = DSPEXT_AMRWB_FRAME_TYPE_1425;
            }
            else if (0 != (dspInitParam & DSP_GAMRWB_BITRATE_1585)){
                rateMode = DSPEXT_AMRWB_FRAME_TYPE_1585;
            }
            else if (0 != (dspInitParam & DSP_GAMRWB_BITRATE_1825)){
                rateMode = DSPEXT_AMRWB_FRAME_TYPE_1825;
            }
            else if (0 != (dspInitParam & DSP_GAMRWB_BITRATE_1985)){
                rateMode = DSPEXT_AMRWB_FRAME_TYPE_1985;
            }
            else if (0 != (dspInitParam & DSP_GAMRWB_BITRATE_2305)){
                rateMode = DSPEXT_AMRWB_FRAME_TYPE_2305;
            }
            else {
                rateMode = DSPEXT_AMRWB_FRAME_TYPE_2385;
            }
            break;
        default:
            /* Not yet implemented */
            OSAL_logMsg("%s:%d DSP coder Type not implemented\n",
                    __FILE__,__LINE__);
            return ((DSP_Instance)NULL);
    }

    /* Start external codec */
    if (DSPEXT_OK != DSPExt_codecStart(type, rateMode, vadEnable)) {
        return ((DSP_Instance)NULL);
    }

    /* This platform does not need to return DSP_Instance */
    return ((DSP_Instance)NULL);
}

/*
 * ======== DSP_codecStop() ========
 * stop codec
 *
 * Return Value:
 *  DSP_ERROR: failed.
 *  DSP_OK: success.
 *
 */
DSP_Return DSP_codecStop(
    void)
{
    if (DSPEXT_OK != DSPExt_codecStop()) {
        return (DSP_ERROR);
    }
    return (DSP_OK);
}

/*
 * ======== DSP_encode() ========
 * Calls PLATFORM specific encode function
 *
 * Return Value:
 *   number of bytes in packet_ptr
 *
 */
vint DSP_encode(                    /* number of encoded bytes, or error */
    DSP_CoderType   coderType,      /* enumeration for coder type */
    uint8          *packet_ptr,     /* pointer to packet data */
    vint           *speech_ptr)     /* pointer to speech samples */
{
    DSPExt_CoderType type;

    switch (coderType) {
        case DSP_CODER_TYPE_AMRNB_OA:
            type = DSPEXT_CODER_TYPE_AMRNB_OA;
            break;
        case DSP_CODER_TYPE_AMRNB_BE:
            type = DSPEXT_CODER_TYPE_AMRNB_BE;
            break;
        case DSP_CODER_TYPE_AMRWB_OA:
            type = DSPEXT_CODER_TYPE_AMRWB_OA;
            break;
        case DSP_CODER_TYPE_AMRWB_BE:
            type = DSPEXT_CODER_TYPE_AMRWB_BE;
            break;
        default:
            OSAL_logMsg("%s:%d The warning coder type %d\n",
                    __FUNCTION__, __LINE__, coderType);
            return (0);
    }
    return DSPExt_Encode(type, packet_ptr);
}

/*
 * ======== DSP_decode() ========
 * Calls PLATFORM specific decode function
 *
 * Return Value:
 *
 */
void DSP_decode(                    /* number of decoded bytes, or error */
    DSP_CoderType   coderType,      /* enumeration for coder type */
    vint           *speech_ptr,     /* pointer to speech samples */
    uint8          *packet_ptr,     /* pointer to packet data */
    uvint           pSize)          /* number of bytes in packet_ptr */
{
    DSPExt_CoderType    type;

    switch (coderType) {
        case DSP_CODER_TYPE_AMRNB_OA:
            type = DSPEXT_CODER_TYPE_AMRNB_OA;
            break;
        case DSP_CODER_TYPE_AMRNB_BE:
            type = DSPEXT_CODER_TYPE_AMRNB_BE;
            break;
        case DSP_CODER_TYPE_AMRWB_OA:
            type = DSPEXT_CODER_TYPE_AMRWB_OA;
            break;
        case DSP_CODER_TYPE_AMRWB_BE:
            type = DSPEXT_CODER_TYPE_AMRWB_BE;
            break;
        default:
            OSAL_logMsg("%s:%d The warning coder type %d\n",
                    __FUNCTION__, __LINE__, coderType);
            return;
    }
    DSPExt_Decode(type, packet_ptr, pSize);
}

