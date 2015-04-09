/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2013 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Revision$ $Date$
 */

#include <osal.h>
#include <vpr_comm.h>
#include "vpad_vpmd.h"
#include "vpmd_io.h"
#include "vpmd_mux.h"

static VPMD_muxObj  _VPMD_muxDriver;
static VPMD_muxObj *_VPMD_muxPtr = &_VPMD_muxDriver;

/*
 *  ======== _VPMD_muxWriteDataToFifo() ========
 *  This function is used to write data to fifo.
 *
 *  Return Values:
 *  OSAL_SUCCESS: success to read.
 *  OSAL_FAIL: fail to read.
 */
OSAL_Status _VPMD_muxWriteDataToFifo(
    vint *fid_ptr,
    char *name_ptr,
    void *buf_ptr,
    vint  size,
    vint  timeout)
{
    /* If a fd doesn't exist then create it. */
    if (0 == *fid_ptr) {
        if (OSAL_SUCCESS != OSAL_fileOpen(fid_ptr,
                name_ptr, OSAL_FILE_O_RDWR, 0)) {
            *fid_ptr = 0;
            OSAL_logMsg("%s:%d Open %s device FAIL.\n",
                    __FUNCTION__, __LINE__, name_ptr);
            return (OSAL_FAIL);
        }
    }

    /* Now Write */
    if (OSAL_FAIL == OSAL_fileWrite(fid_ptr, buf_ptr, &size)) {
        OSAL_logMsg("%s:%d Sending to %s FAIL.\n", __FUNCTION__, __LINE__,
                name_ptr);
        return (OSAL_FAIL);
    }

    return (OSAL_SUCCESS);
}

/*
 *  ======== VPMD_muxWriteDevice() ========
 *  This function is used to write data to the device, wrapper for real io.
 *
 *  Return Values:
 *  OSAL_SUCCESS: success to read.
 *  OSAL_FAIL: fail to read.
 */
OSAL_Status VPMD_muxWriteDevice(
    void *buf_ptr,
    vint *size_ptr)
{
    VPR_Comm *vpr_ptr = (VPR_Comm *)buf_ptr;

    /* Simple abstraction here. may add task/thread to handle write in future */
    VPMD_muxDbgPrintf("%s:%d written targetModule:%d\n",
                    __FUNCTION__, __LINE__, vpr_ptr->targetModule);

    return (VPMD_ioWriteDevice(vpr_ptr, size_ptr));
}

/*
 *  ======== _VPMD_writeVpmdReady() ========
 *  This function is used to write ready event to VPAD.
 *
 *  Return Values:
 *  OSAL_SUCCESS: success to write.
 *  OSAL_FAIL: fail to write.
 */
OSAL_Status _VPMD_writeVpmdReady(
    OSAL_Boolean echo)
{
    vint size;

    VPR_Comm *vpr_ptr = &(_VPMD_muxPtr->vprCommBuffer);

    vpr_ptr->targetSize = 0;
    /* If it's echo the ready event, set the target to VPMD_VPMD_TARGET. */
    if (echo) {
        vpr_ptr->targetModule = VPMD_VPMD_TARGET;
    }
    else {
        vpr_ptr->targetModule = VPMD_VPAD_TARGET;
    }

    size = sizeof(VPR_Comm);

    if (OSAL_FAIL == VPMD_muxWriteDevice(vpr_ptr, &size)) {
        return (OSAL_FAIL);
    }

    return (OSAL_SUCCESS);
}

/*
 * ======== _VPMD_muxIoReadyHandler() ========
 * Function is used to send VPMD ready to VPAD, if VPMD IO is ready.
 *
 * Return Values:
 *
 */
void _VPMD_muxIoReadyHandler(
    OSAL_Boolean    isReady)
{
    if (OSAL_TRUE == isReady) {
        /* Send ready event to VPAD. */
        _VPMD_writeVpmdReady(OSAL_FALSE);
    }
}

/*
 * ======== _VPMD_muxDataHandler() ========
 * Function to process data from VPAD.
 *
 * Return Values:
 *  OSAL_SUCCESS: Data processed.
 *  OSAL_FAIL: Failed to process data.
 */
OSAL_Status _VPMD_muxDataHandler(
    void *buf_ptr,
    vint  size)
{
    VPR_Comm *vpr_ptr = (VPR_Comm *)buf_ptr;
    OSAL_Status result = OSAL_FAIL;

    VPMD_muxDbgPrintf("%s:%d handling targetModule: %d.\n",
                    __FUNCTION__, __LINE__, vpr_ptr->targetModule);
    switch (vpr_ptr->targetModule) {
        case VPMD_VIDEO_CMDEVT_TARGET:
            result = _VPMD_muxWriteDataToFifo(
                    &_VPMD_muxPtr->outFifo.videoCmdEvtQ.fid,
                    _VPMD_muxPtr->outFifo.videoCmdEvtQ.name,
                    buf_ptr,
                    vpr_ptr->targetSize,
                    OSAL_WAIT_FOREVER);
            break;
        case VPMD_VIDEO_STREAM_TARGET:
            result = _VPMD_muxWriteDataToFifo(
                    &_VPMD_muxPtr->outFifo.videoStreamQ.fid,
                    _VPMD_muxPtr->outFifo.videoStreamQ.name,
                    buf_ptr,
                    vpr_ptr->targetSize,
                    OSAL_WAIT_FOREVER);
            break;
        case VPMD_VOICE_STREAM_TARGET:
            result = _VPMD_muxWriteDataToFifo(
                    &_VPMD_muxPtr->outFifo.voiceStreamQ.fid,
                    _VPMD_muxPtr->outFifo.voiceStreamQ.name,
                    buf_ptr,
                    vpr_ptr->targetSize,
                    OSAL_WAIT_FOREVER);
            break;
        case VPMD_ISI_RPC_TARGET:
            result = _VPMD_muxWriteDataToFifo(
                    &_VPMD_muxPtr->outFifo.isiRpcQ.fid,
                    _VPMD_muxPtr->outFifo.isiRpcQ.name,
                    buf_ptr,
                    vpr_ptr->targetSize,
                    OSAL_WAIT_FOREVER);
            break;
        case VPMD_ISI_EVT_RPC_TARGET:
            result = _VPMD_muxWriteDataToFifo(
                    &_VPMD_muxPtr->outFifo.isiEvtRpcQ.fid,
                    _VPMD_muxPtr->outFifo.isiEvtRpcQ.name,
                    buf_ptr,
                    vpr_ptr->targetSize,
                    OSAL_WAIT_FOREVER);
            break;
        case VPMD_SIP_TARGET:
            result = _VPMD_muxWriteDataToFifo(
                    &_VPMD_muxPtr->outFifo.sipQ.fid,
                    _VPMD_muxPtr->outFifo.sipQ.name,
                    buf_ptr,
                    vpr_ptr->targetSize,
                    OSAL_WAIT_FOREVER);
            break;
        case VPMD_ISIP_TARGET:
            result = _VPMD_muxWriteDataToFifo(
                    &_VPMD_muxPtr->outFifo.isipQ.fid,
                    _VPMD_muxPtr->outFifo.isipQ.name,
                    buf_ptr,
                    vpr_ptr->targetSize,
                    OSAL_WAIT_FOREVER);
            break;
        case VPMD_CSM_EVT_TARGET:
            result = _VPMD_muxWriteDataToFifo(
                    &_VPMD_muxPtr->outFifo.csmEvtQ.fid,
                    _VPMD_muxPtr->outFifo.csmEvtQ.name,
                    buf_ptr,
                    vpr_ptr->targetSize,
                    OSAL_WAIT_FOREVER);
            break;
        case VPMD_VPMD_TARGET:
            /* Remote is up and ready. */
            if (!_VPMD_muxPtr->isVpadReady) {
                _VPMD_muxPtr->isVpadReady = OSAL_TRUE;
                OSAL_logMsg("VPAD is ready now.\n");
            }
            else {
                /* VPAD is already ready. */
                OSAL_logMsg("VPAD is already ready.\n");
            }
            /* Send echo back to VPAD. */
            result = _VPMD_writeVpmdReady(OSAL_TRUE);
            if (OSAL_SUCCESS != result) {
                OSAL_logMsg("Failed to echo back VPAD ready event.\n");
            }
            break;
        case VPMD_VPAD_TARGET:
            /* It's an echo back event. */
            if (!_VPMD_muxPtr->isVpadReady) {
                _VPMD_muxPtr->isVpadReady = OSAL_TRUE;
                OSAL_logMsg("VPAD is ready now with echo.\n");
            }
            else {
                /* VPAD is already ready. */
                OSAL_logMsg("VPAD is already ready with echo.\n");
            }
            result = OSAL_SUCCESS;
            break;
        default:
            OSAL_logMsg("%s:%d Unknown targetModule %d.\n",
                    __FUNCTION__, __LINE__, vpr_ptr->targetModule);
            return (OSAL_FAIL);
    }

    if (OSAL_SUCCESS != result) {
        OSAL_logMsg("%s:%d _VPMD_muxWriteDataToFifo failed for targetModule "
                "%d\n", __FUNCTION__, __LINE__, vpr_ptr->targetModule);
    }
    return (result);
}

/*
 * ======== _VPMD_muxInitFifo ========
 *
 * Private function to initialize fifo as reading queue
 *
 * RETURN:
 * OSAL_SUCCESS: Initialized successfully
 * OSAL_FAIL: Initialize failed.
 */
static OSAL_Status _VPMD_muxInitFifo(
    const char     *devicePath_ptr,
    MODEM_Terminal *terminal_ptr,
    int             size)
{
    OSAL_Status ret;

    OSAL_strcpy(terminal_ptr->name, devicePath_ptr);
    ret = OSAL_fileFifoDelete(devicePath_ptr); /* rm $devicePath_ptr */
    if (OSAL_SUCCESS != ret) {
        terminal_ptr->fid = 0;
        OSAL_logMsg("%s:%d remove %s fifo FAIL. ignored\n", __FUNCTION__,
                __LINE__, devicePath_ptr);
    }
    ret = OSAL_fileFifoCreate(devicePath_ptr, VPMD_FIFO_MSG_COUNT, size);
    if (OSAL_SUCCESS != ret) {
        terminal_ptr->fid = 0;
        OSAL_logMsg("%s:%d create %s fifo FAIL.\n", __FUNCTION__, __LINE__,
                devicePath_ptr);
        return (OSAL_FAIL);
    }
    if (OSAL_SUCCESS != OSAL_fileOpen(&terminal_ptr->fid,
            devicePath_ptr, OSAL_FILE_O_RDWR, 0)) {
        terminal_ptr->fid = 0;
        OSAL_logMsg("%s:%d Open %s fifo FAIL.\n", __FUNCTION__, __LINE__,
                devicePath_ptr);
        return (OSAL_FAIL);
    }

    return (OSAL_SUCCESS);
}

/*
 *  ======== VPMD_muxInit() ========
 *  This function is used to initialize VPMD Mux
 *
 *  Return Values:
 *  OSAL_SUCCESS: success to initialize VPMD Mux.
 *  OSAL_FAIL: fail to initialize VPMD Mux.
 */
OSAL_Status VPMD_muxInit(
    void)
{
    VPMD_ioInit((void *)_VPMD_muxIoReadyHandler, (void *)_VPMD_muxDataHandler);

    /* initialize the global object */
    OSAL_memSet(_VPMD_muxPtr, 0, sizeof(VPMD_muxObj));

    /*
     * Init Read FIFO. Read data from IO device and put the data to this FIFO.
     */
    /* initialize video commadn/event FIFO */
    if (OSAL_SUCCESS != _VPMD_muxInitFifo(VPMD_VIDEO_CMDEVT_R_FIFO,
            &_VPMD_muxPtr->outFifo.videoCmdEvtQ, sizeof(VPR_Comm))) {
        VPMD_destroy();
        return (OSAL_FAIL);
    }
    /* initialize video stream FIFO */
    if (OSAL_SUCCESS != _VPMD_muxInitFifo(VPMD_VIDEO_STREAM_R_FIFO,
            &_VPMD_muxPtr->outFifo.videoStreamQ, sizeof(VPR_Comm))) {
        VPMD_destroy();
        return (OSAL_FAIL);
    }
    /* initialize voice stream FIFO */
    if (OSAL_SUCCESS != _VPMD_muxInitFifo(VPMD_VOICE_STREAM_R_FIFO,
            &_VPMD_muxPtr->outFifo.voiceStreamQ, sizeof(VPR_Comm))) {
        VPMD_destroy();
        return (OSAL_FAIL);
    }
    /* initialize isi RPC FIFO */
    if (OSAL_SUCCESS != _VPMD_muxInitFifo(VPMD_ISI_RPC_R_FIFO,
            &_VPMD_muxPtr->outFifo.isiRpcQ, sizeof(VPR_Comm))) {
        VPMD_destroy();
        return (OSAL_FAIL);
    }
    /* initialize isi event RPC FIFO */
    if (OSAL_SUCCESS != _VPMD_muxInitFifo(VPMD_ISI_EVT_RPC_R_FIFO,
            &_VPMD_muxPtr->outFifo.isiEvtRpcQ, sizeof(VPR_Comm))) {
        VPMD_destroy();
        return (OSAL_FAIL);
    }
    /* initialize sip FIFO */
    if (OSAL_SUCCESS != _VPMD_muxInitFifo(VPMD_SIP_R_FIFO,
            &_VPMD_muxPtr->outFifo.sipQ, sizeof(VPR_Comm))) {
        VPMD_destroy();
        return (OSAL_FAIL);
    }
    /* initialize isip FIFO */
    if (OSAL_SUCCESS != _VPMD_muxInitFifo(VPMD_ISIP_R_FIFO,
            &_VPMD_muxPtr->outFifo.isipQ, sizeof(VPR_Comm))) {
        VPMD_destroy();
        return (OSAL_FAIL);
    }
    /* initialize csm evnet FIFO */
    if (OSAL_SUCCESS != _VPMD_muxInitFifo(VPMD_CSM_EVT_R_FIFO,
            &_VPMD_muxPtr->outFifo.csmEvtQ, sizeof(VPR_Comm))) {
        VPMD_destroy();
        return (OSAL_FAIL);
    }

    return (OSAL_SUCCESS);
}

/*
 *  ======== VPMD_muxDestroy() ========
 *  This function is used to shutdown VPMD Mux
 *
 *  Return Values:
 *  OSAL_SUCCESS: success to shutdown VPMD mux.
 *  OSAL_FAIL: fail to shutdown VPMD mux.
 */
void VPMD_muxDestroy(
    void)
{
    if (0 != _VPMD_muxPtr->outFifo.videoCmdEvtQ.fid) {
        OSAL_fileClose(&_VPMD_muxPtr->outFifo.videoCmdEvtQ.fid);
    }
    if (0 != _VPMD_muxPtr->outFifo.videoStreamQ.fid) {
        OSAL_fileClose(&_VPMD_muxPtr->outFifo.videoStreamQ.fid);
    }
    if (0 != _VPMD_muxPtr->outFifo.voiceStreamQ.fid) {
        OSAL_fileClose(&_VPMD_muxPtr->outFifo.voiceStreamQ.fid);
    }
    if (0 != _VPMD_muxPtr->outFifo.isiRpcQ.fid) {
        OSAL_fileClose(&_VPMD_muxPtr->outFifo.isiRpcQ.fid);
    }
    if (0 != _VPMD_muxPtr->outFifo.isiEvtRpcQ.fid) {
        OSAL_fileClose(&_VPMD_muxPtr->outFifo.isiEvtRpcQ.fid);
    }
    if (0 != _VPMD_muxPtr->outFifo.sipQ.fid) {
        OSAL_fileClose(&_VPMD_muxPtr->outFifo.sipQ.fid);
    }
    if (0 != _VPMD_muxPtr->outFifo.isipQ.fid) {
        OSAL_fileClose(&_VPMD_muxPtr->outFifo.isipQ.fid);
    }
    if (0 != _VPMD_muxPtr->outFifo.csmEvtQ.fid) {
        OSAL_fileClose(&_VPMD_muxPtr->outFifo.csmEvtQ.fid);
    }
    
    VPMD_ioDestroy();
}

/*
 *  ======== VPMD_muxIsReady() ========
 *  This function is returen the readiness of VPMD MUX.
 *
 *  Return Values:
 *  OSAL_TRUE: VPMD MUX is ready.
 *  OSAL_FAIL: VPMD MUX is not ready.
 */
OSAL_Boolean VPMD_muxIsReady()
{
    return (_VPMD_muxPtr->isVpadReady);
}
