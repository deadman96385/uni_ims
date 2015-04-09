/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004-2010 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Revision: 1381 $ $Date: 2006-12-05 14:44:53 -0800 (Tue, 05 Dec 2006) $
 */

#include <osal.h>
#include <vpr_comm.h>
#include "vpad_vpmd.h"
#include "../vpmd_mux/vpmd_mux.h"

typedef struct {
    struct {
        MODEM_Terminal  videoCmdEvtQ;
        MODEM_Terminal  videoStreamQ;
        MODEM_Terminal  voiceStreamQ;
        MODEM_Terminal  isiRpcQ;
        MODEM_Terminal  isiEvtRpcQ;
        MODEM_Terminal  sipQ;
        MODEM_Terminal  isipQ;
        MODEM_Terminal  csmEvtQ;
    } muxOutFifo; /* read data from FIFO, MUX will fill data in these FIFO */
    VPR_Comm            vprCommBuffer;
    OSAL_SemId          writeMutex;
} VPMD_Obj;

static VPMD_Obj  VPMD_driver;
static VPMD_Obj *VPMD_ptr = &VPMD_driver;

/*
 *  ======== _VPMD_readDataFromFifo() ========
 *  This function is used to read data from Fifo.
 *
 *  Return Values:
 *  OSAL_SUCCESS: success to read.
 *  OSAL_FAIL: fail to read.
 */
OSAL_Status _VPMD_readDataFromFifo(
    vint *fid_ptr,
    void *buf_ptr,
    vint  size,
    vint  timeout)
{
    OSAL_SelectSet      fdSet;
    OSAL_SelectTimeval  time;
    OSAL_SelectTimeval *timeout_ptr;
    OSAL_Boolean        isTimedOut;
    vint                readSize;
    vint                remainSize;

    OSAL_memSet(&time, 0, sizeof(OSAL_SelectTimeval));

    if (OSAL_WAIT_FOREVER == timeout) {
        timeout_ptr = NULL;
    }
    else {
        time.sec= timeout / 1000;
        time.usec= timeout % 1000;
        timeout_ptr = &time;
    }

    /* Select Fd and read file. */
    OSAL_selectSetInit(&fdSet);
    OSAL_selectAddId(fid_ptr, &fdSet);

    if (OSAL_FAIL ==
            OSAL_select(&fdSet, NULL, timeout_ptr, &isTimedOut)) {
        OSAL_logMsg("%s:%d File select FAIL.\n", __FUNCTION__, __LINE__);
        return (OSAL_FAIL);
    }
    if (isTimedOut == OSAL_TRUE) {
        return (OSAL_FAIL);
    }

    /* Read until required data size is read */
    remainSize = size;
    readSize   = 0;
    while (remainSize > 0) {
        size = remainSize;
        if (OSAL_FAIL == OSAL_fileRead(fid_ptr,
                (void *)((char *)buf_ptr + readSize), &size)) {
            OSAL_logMsg("%s:%d Read file FAIL.\n", __FUNCTION__, __LINE__);
            return (OSAL_FAIL);
        }
        remainSize -= size;
        readSize += size;
    }

    return (OSAL_SUCCESS);
}

/*
 * ======== VPMD_allocate() ========
 *
 * Public routine for allocating the VPMD module resource
 *
 * Returns:
 *      OSAL_SUCCESS: function exits normally.
 *      OSAL_FAIL: in case of error
 */
OSAL_Status VPMD_allocate(void)
{
    if (OSAL_SUCCESS != VPMD_muxInit()) {
        return (OSAL_FAIL);
    }
    if (NULL == (VPMD_ptr->writeMutex = OSAL_semMutexCreate())) {
        OSAL_logMsg("%s %d Create Mutex FAILED.\n", __FUNCTION__, __LINE__);
        return (OSAL_FAIL);
    }
    return (OSAL_SUCCESS);
}

/*
 *  ======== VPMD_init() ========
 *  This function is used to initialize VPMD
 *
 *  Return Values:
 *  OSAL_SUCCESS: success to initialize VPMD.
 *  OSAL_FAIL: fail to initialize VPMD.
 */
OSAL_Status VPMD_init(void)
{
    if (OSAL_SUCCESS != VPMD_muxInit()) {
        return (OSAL_FAIL);
    }
    if (NULL == (VPMD_ptr->writeMutex = OSAL_semMutexCreate())) {
        OSAL_logMsg("%s %d Create Mutex FAILED.\n", __FUNCTION__, __LINE__);
        return (OSAL_FAIL);
    }

    return (OSAL_SUCCESS);
}

/*
 *  ======== VPMD_destroy() ========
 *  This function is used to shutdown VPMD
 *
 *  Return Values:
 *  OSAL_SUCCESS: success to shutdown VPMD.
 *  OSAL_FAIL: fail to shutdown VPMD.
 */
void VPMD_destroy(
    void)
{
    VPMD_muxDestroy();
    OSAL_semDelete(VPMD_ptr->writeMutex);
}

/*
 *  ======== VPMD_isReady() ========
 *  This function is used to return readiness of vpmd/vpad comm
 *
 *  Return Values:
 *  OSAL_TRUE: vpmd is talking to vpad
 *  OSAL_FALSE: fvpmd is not talking to vpad yet
 */
OSAL_Boolean VPMD_isReady(
    void)
{
    return VPMD_muxIsReady();
}

/*
 *  ======== VPMD_writeVideoCmdEvt() ========
 *  This function is used to write video command/event to VPAD
 *
 *  Return Values:
 *  OSAL_SUCCESS: success to write command.
 *  OSAL_FAIL: fail to write command.
 */
OSAL_Status VPMD_writeVideoCmdEvt(
    void *buf_ptr,
    vint  size)
{
    VPR_Comm *vpr_ptr = (VPR_Comm *)buf_ptr;
    vpr_ptr->targetModule = VPMD_VIDEO_CMDEVT_TARGET;
    vpr_ptr->targetSize = sizeof(VPR_Comm);

    OSAL_semAcquire(VPMD_ptr->writeMutex, OSAL_WAIT_FOREVER);

    /* Now Write */
    if (OSAL_FAIL == VPMD_muxWriteDevice(buf_ptr, &size)) {
        OSAL_semGive(VPMD_ptr->writeMutex);
        return (OSAL_FAIL);
    }

    OSAL_semGive(VPMD_ptr->writeMutex);
    return (OSAL_SUCCESS);
}

/*
 *  ======== VPMD_readVideoCmdEvt() ========
 *  This function is used to read video command/event from VPAD
 *
 *  Return Values:
 *  OSAL_SUCCESS: success to read command.
 *  OSAL_FAIL: fail to read command.
 */
OSAL_Status VPMD_readVideoCmdEvt(
    void *buf_ptr,
    vint  size,
    vint  timeout)
{
    /* If a fd doesn't exist then create it. */
    if (0 == VPMD_ptr->muxOutFifo.videoCmdEvtQ.fid) {
        if (OSAL_SUCCESS != OSAL_fileOpen(
                &VPMD_ptr->muxOutFifo.videoCmdEvtQ.fid,
                VPMD_VIDEO_CMDEVT_R_FIFO, OSAL_FILE_O_RDWR, 0)) {
            VPMD_ptr->muxOutFifo.videoCmdEvtQ.fid = 0;
            OSAL_logMsg("%s:%d Open %s device FAIL.\n", __FUNCTION__, __LINE__,
                    VPMD_VIDEO_CMDEVT_R_FIFO);
            return (OSAL_FAIL);
        }
    }

    /* Now Read */
    return _VPMD_readDataFromFifo(&VPMD_ptr->muxOutFifo.videoCmdEvtQ.fid,
            buf_ptr, size, timeout);
}

/*
 *  ======== VPMD_writeVideoStream() ========
 *  This function is used to write video stream to VPAD
 *
 *  Return Values:
 *  OSAL_SUCCESS: success to write stream.
 *  OSAL_FAIL: fail to write stream.
 */
OSAL_Status VPMD_writeVideoStream(
    void *buf_ptr,
    vint  size)
{
    VPR_Comm *vpr_ptr = (VPR_Comm *)buf_ptr;
    vpr_ptr->targetModule = VPMD_VIDEO_STREAM_TARGET;
    vpr_ptr->targetSize = sizeof(VPR_Comm);

    OSAL_semAcquire(VPMD_ptr->writeMutex, OSAL_WAIT_FOREVER);

    /* Now Write */
    if (OSAL_FAIL == VPMD_muxWriteDevice(buf_ptr, &size)) {
        OSAL_semGive(VPMD_ptr->writeMutex);
        return (OSAL_FAIL);
    }

    OSAL_semGive(VPMD_ptr->writeMutex);
    return (OSAL_SUCCESS);
}

/*
 *  ======== VPMD_readVideoStream() ========
 *  This function is used to read video stream from VPAD
 *
 *  Return Values:
 *  OSAL_SUCCESS: success to read event.
 *  OSAL_FAIL: fail to read event.
 */
OSAL_Status VPMD_readVideoStream(
    void *buf_ptr,
    vint  size,
    vint  timeout)
{
    /* If a fd doesn't exist then create it. */
    if (0 == VPMD_ptr->muxOutFifo.videoStreamQ.fid) {
        if (OSAL_SUCCESS != OSAL_fileOpen(
                &VPMD_ptr->muxOutFifo.videoStreamQ.fid,
                VPMD_VIDEO_STREAM_R_FIFO, OSAL_FILE_O_RDWR, 0)) {
            VPMD_ptr->muxOutFifo.videoStreamQ.fid = 0;
            OSAL_logMsg("%s:%d Open %s device FAIL.\n", __FUNCTION__, __LINE__,
                    VPMD_VIDEO_STREAM_R_FIFO);
            return (OSAL_FAIL);
        }
    }

    /* Now Read */
    return _VPMD_readDataFromFifo(&VPMD_ptr->muxOutFifo.videoStreamQ.fid,
            buf_ptr, size, timeout);
}

/*
 *  ======== VPMD_writeVoiceStream() ========
 *  This function is used to write voice stream to VPAD
 *
 *  Return Values:
 *  OSAL_SUCCESS: success to write stream.
 *  OSAL_FAIL: fail to write stream.
 */
OSAL_Status VPMD_writeVoiceStream(
    void *buf_ptr,
    vint  size)
{
    VPR_Comm *vpr_ptr = (VPR_Comm *)buf_ptr;
    vpr_ptr->targetModule = VPMD_VOICE_STREAM_TARGET;
    vpr_ptr->targetSize = sizeof(VPR_Comm);

    OSAL_semAcquire(VPMD_ptr->writeMutex, OSAL_WAIT_FOREVER);

    /* Now Write */
    if (OSAL_FAIL == VPMD_muxWriteDevice(buf_ptr, &size)) {
        OSAL_semGive(VPMD_ptr->writeMutex);
        return (OSAL_FAIL);
    }

    OSAL_semGive(VPMD_ptr->writeMutex);
    return (OSAL_SUCCESS);
}

/*
 *  ======== VPMD_readVoiceStream() ========
 *  This function is used to read voice stream from VPAD
 *
 *  Return Values:
 *  OSAL_SUCCESS: success to read stream.
 *  OSAL_FAIL: fail to read stream.
 */
OSAL_Status VPMD_readVoiceStream(
    void *buf_ptr,
    vint  size,
    vint  timeout)
{
    /* If a fd doesn't exist then create it. */
    if (0 == VPMD_ptr->muxOutFifo.voiceStreamQ.fid) {
        if (OSAL_SUCCESS != OSAL_fileOpen(
                &VPMD_ptr->muxOutFifo.voiceStreamQ.fid,
                VPMD_VOICE_STREAM_R_FIFO, OSAL_FILE_O_RDWR, 0)) {
            VPMD_ptr->muxOutFifo.voiceStreamQ.fid = 0;
            OSAL_logMsg("%s:%d Open %s device FAIL.\n", __FUNCTION__, __LINE__,
                    VPMD_VOICE_STREAM_R_FIFO);
            return (OSAL_FAIL);
        }
    }

    /* Now Read */
    return _VPMD_readDataFromFifo(&VPMD_ptr->muxOutFifo.voiceStreamQ.fid,
            buf_ptr, size, timeout);
}

/*
 *  ======== VPMD_writeIsiRpc() ========
 *  This function is used to write ISI function return to VPAD.
 *
 *  Return Values:
 *  OSAL_SUCCESS: success to write.
 *  OSAL_FAIL: fail to write.
 */
OSAL_Status VPMD_writeIsiRpc(
    void *buf_ptr,
    vint  size)
{
    VPR_Comm *vpr_ptr = &(VPMD_ptr->vprCommBuffer);

    OSAL_semAcquire(VPMD_ptr->writeMutex, OSAL_WAIT_FOREVER);

    /* wrap inside vpr_comm buffer */
    OSAL_memCpy(vpr_ptr, buf_ptr, size);
    vpr_ptr->targetSize = size;
    size = sizeof(VPR_Comm);
    buf_ptr = vpr_ptr;

    vpr_ptr->targetModule = VPMD_ISI_RPC_TARGET;

    /* Now Write */
    if (OSAL_FAIL == VPMD_muxWriteDevice(buf_ptr, &size)) {
        OSAL_semGive(VPMD_ptr->writeMutex);
        return (OSAL_FAIL);
    }

    OSAL_semGive(VPMD_ptr->writeMutex);
    return (OSAL_SUCCESS);
}

/*
 *  ======== VPMD_readIsiRpc() ========
 *  This function is used to read ISI function call from VPAD.
 *
 *  Return Values:
 *  OSAL_SUCCESS: success to read.
 *  OSAL_FAIL: fail to read.
 */
OSAL_Status VPMD_readIsiRpc(
    void *buf_ptr,
    vint  size,
    vint  timeout)
{
    /* If a fd doesn't exist then create it. */
    if (0 == VPMD_ptr->muxOutFifo.isiRpcQ.fid) {
        if (OSAL_SUCCESS != OSAL_fileOpen(&VPMD_ptr->muxOutFifo.isiRpcQ.fid,
                VPMD_ISI_RPC_R_FIFO, OSAL_FILE_O_RDWR, 0)) {
            VPMD_ptr->muxOutFifo.isiRpcQ.fid = 0;
            OSAL_logMsg("%s:%d Open %s device FAIL.\n", __FUNCTION__, __LINE__,
                    VPMD_ISI_RPC_R_FIFO);
            return (OSAL_FAIL);
        }
    }

    /* Now Read */
    return _VPMD_readDataFromFifo(&VPMD_ptr->muxOutFifo.isiRpcQ.fid, buf_ptr,
            size, timeout);
}

/*
 *  ======== VPMD_writeIsiEvtRpc() ========
 *  This function is used to write ISI events and return to VPAD.
 *  This is for ISI_getEvent().
 *
 *  Return Values:
 *  OSAL_SUCCESS: success to write.
 *  OSAL_FAIL: fail to write.
 */
OSAL_Status VPMD_writeIsiEvtRpc(
    void *buf_ptr,
    vint  size)
{
    VPR_Comm *vpr_ptr = &(VPMD_ptr->vprCommBuffer);

    OSAL_semAcquire(VPMD_ptr->writeMutex, OSAL_WAIT_FOREVER);

    /* wrap inside vpr_comm buffer */
    OSAL_memCpy(vpr_ptr, buf_ptr, size);
    vpr_ptr->targetSize = size;
    size = sizeof(VPR_Comm);
    buf_ptr = vpr_ptr;

    vpr_ptr->targetModule = VPMD_ISI_EVT_RPC_TARGET;

    /* Now write */
    if (OSAL_FAIL == VPMD_muxWriteDevice(buf_ptr, &size)) {
        OSAL_semGive(VPMD_ptr->writeMutex);
        return (OSAL_FAIL);
    }

    OSAL_semGive(VPMD_ptr->writeMutex);
    return (OSAL_SUCCESS);
}

/*
 *  ======== VPMD_readIsiEvtRpc() ========
 *  This function is used to read ISI function call and the function
 *  is, ISI_getEvent(), from VPAD.
 *
 *  Return Values:
 *  OSAL_SUCCESS: success to read.
 *  OSAL_FAIL: fail to read.
 */
OSAL_Status VPMD_readIsiEvtRpc(
    void *buf_ptr,
    vint  size,
    vint  timeout)
{
    /* If a fd doesn't exist then create it. */
    if (0 == VPMD_ptr->muxOutFifo.isiEvtRpcQ.fid) {
        if (OSAL_SUCCESS != OSAL_fileOpen(&VPMD_ptr->muxOutFifo.isiEvtRpcQ.fid,
                VPMD_ISI_EVT_RPC_R_FIFO, OSAL_FILE_O_RDONLY, 0)) {
            VPMD_ptr->muxOutFifo.isiEvtRpcQ.fid = 0;
            OSAL_logMsg("%s:%d Open %s fifo FAIL.\n", __FUNCTION__, __LINE__,
                    VPMD_ISI_EVT_RPC_R_FIFO);
            return (OSAL_FAIL);
        }
    }

    /* Now Read */
    return _VPMD_readDataFromFifo(&VPMD_ptr->muxOutFifo.isiEvtRpcQ.fid, buf_ptr,
            size, timeout);
}

/*
 *  ======== VPMD_writeIsip() ========
 *  This function is used to write SIP packet
 *
 *  Return Values:
 *  OSAL_SUCCESS: success to write.
 *  OSAL_FAIL: fail to write.
 */
OSAL_Status VPMD_writeIsip(
    void *buf_ptr,
    vint  size)
{
    VPR_Comm *vpr_ptr = &(VPMD_ptr->vprCommBuffer);

    OSAL_semAcquire(VPMD_ptr->writeMutex, OSAL_WAIT_FOREVER);

    /* wrap inside vpr_comm buffer */
    OSAL_memCpy(vpr_ptr, buf_ptr, size);
    vpr_ptr->targetSize = size;
    size = sizeof(VPR_Comm);
    buf_ptr = vpr_ptr;

    vpr_ptr->targetModule = VPMD_ISIP_TARGET;

    /* Now Write */
    if (OSAL_FAIL == VPMD_muxWriteDevice(buf_ptr, &size)) {
        OSAL_semGive(VPMD_ptr->writeMutex);
        return (OSAL_FAIL);
    }

    OSAL_semGive(VPMD_ptr->writeMutex);
    return (OSAL_SUCCESS);
}

/*
 *  ======== VPMD_readIsip() ========
 *  This function is used to read isip message
 *
 *  Return Values:
 *  OSAL_SUCCESS: success to read.
 *  OSAL_FAIL: fail to read.
 */
OSAL_Status VPMD_readIsip(
    void *buf_ptr,
    vint  size,
    vint  timeout)
{
    /* If a fd doesn't exist then create it. */
    if (0 == VPMD_ptr->muxOutFifo.isipQ.fid) {
        if (OSAL_SUCCESS != OSAL_fileOpen(&VPMD_ptr->muxOutFifo.isipQ.fid,
                VPMD_ISIP_R_FIFO, OSAL_FILE_O_RDWR, 0)) {
            VPMD_ptr->muxOutFifo.isipQ.fid = 0;
            OSAL_logMsg("%s:%d Open %s device FAIL.\n", __FUNCTION__, __LINE__,
                    VPMD_ISIP_R_FIFO);
            return (OSAL_FAIL);
        }
    }

    /* Now Read */
    return _VPMD_readDataFromFifo(&VPMD_ptr->muxOutFifo.isipQ.fid, buf_ptr,
            size, timeout);
}

/*
 *  ======== VPMD_writeSip() ========
 *  This function is used to write isip message
 *
 *  Return Values:
 *  OSAL_SUCCESS: success to write.
 *  OSAL_FAIL: fail to write.
 */
OSAL_Status VPMD_writeSip(
    void *buf_ptr,
    vint  size)
{
    VPR_Comm *vpr_ptr = &(VPMD_ptr->vprCommBuffer);

    OSAL_semAcquire(VPMD_ptr->writeMutex, OSAL_WAIT_FOREVER);

    /* wrap inside vpr_comm buffer */
    OSAL_memCpy(vpr_ptr, buf_ptr, size);
    vpr_ptr->targetSize = size;
    size = sizeof(VPR_Comm);
    buf_ptr = vpr_ptr;

    vpr_ptr->targetModule = VPMD_SIP_TARGET;

    /* Now Write */
    if (OSAL_FAIL == VPMD_muxWriteDevice(buf_ptr, &size)) {
        OSAL_semGive(VPMD_ptr->writeMutex);
        return (OSAL_FAIL);
    }

    OSAL_semGive(VPMD_ptr->writeMutex);
    return (OSAL_SUCCESS);
}

/*
 *  ======== VPMD_readSip() ========
 *  This function is used to read sip packet
 *
 *  Return Values:
 *  OSAL_SUCCESS: success to read.
 *  OSAL_FAIL: fail to read.
 */
OSAL_Status VPMD_readSip(
    void *buf_ptr,
    vint  size,
    vint  timeout)
{
    /* If a fd doesn't exist then create it. */
    if (0 == VPMD_ptr->muxOutFifo.sipQ.fid) {
        if (OSAL_SUCCESS != OSAL_fileOpen(&VPMD_ptr->muxOutFifo.sipQ.fid,
                VPMD_SIP_R_FIFO, OSAL_FILE_O_RDWR, 0)) {
            VPMD_ptr->muxOutFifo.sipQ.fid = 0;
            OSAL_logMsg("%s:%d Open %s device FAIL.\n", __FUNCTION__, __LINE__,
                    VPMD_SIP_R_FIFO);
            return (OSAL_FAIL);
        }
    }

    /* Now Read */
    return _VPMD_readDataFromFifo(&VPMD_ptr->muxOutFifo.sipQ.fid, buf_ptr,
            size, timeout);
}

/*
 *  ======== VPMD_readCsmEvt() ========
 *  This function is used to read CSM event from VPAD.
 *
 *  Return Values:
 *  OSAL_SUCCESS: success to read.
 *  OSAL_FAIL: fail to read.
 */
OSAL_Status VPMD_readCsmEvt(
    void *buf_ptr,
    vint  size,
    vint  timeout)
{
    /* If a fd doesn't exist then create it. */
    if (0 == VPMD_ptr->muxOutFifo.csmEvtQ.fid) {
        if (OSAL_SUCCESS != OSAL_fileOpen(&VPMD_ptr->muxOutFifo.csmEvtQ.fid,
                VPMD_CSM_EVT_R_FIFO, OSAL_FILE_O_RDWR, 0)) {
            VPMD_ptr->muxOutFifo.csmEvtQ.fid = 0;
            OSAL_logMsg("%s:%d Open %s device FAIL.\n", __FUNCTION__, __LINE__,
                    VPMD_CSM_EVT_R_FIFO);
            return (OSAL_FAIL);
        }
    }

    /* Now Read */
    return _VPMD_readDataFromFifo(&VPMD_ptr->muxOutFifo.csmEvtQ.fid, buf_ptr,
            size, timeout);
}

/*
 * ======== VPMD_getVoiceStreamReadFd() ========
 * This function is to get voice stream read fid.
 *
 * Return Values:
 *   The fid of the device.
 */
int VPMD_getVoiceStreamReadFd(
    void)
{
    /* If a fd doesn't exist then create it. */
    if (0 == VPMD_ptr->muxOutFifo.voiceStreamQ.fid) {
        if (OSAL_SUCCESS != OSAL_fileOpen(
                &VPMD_ptr->muxOutFifo.voiceStreamQ.fid,
                VPMD_VOICE_STREAM_R_FIFO, OSAL_FILE_O_RDWR, 0)) {
            VPMD_ptr->muxOutFifo.voiceStreamQ.fid = 0;
            OSAL_logMsg("%s:%d Open %s device FAIL.\n", __FUNCTION__, __LINE__,
                    VPMD_VOICE_STREAM_R_FIFO);
            return (OSAL_FAIL);
        }
    }

    return (VPMD_ptr->muxOutFifo.voiceStreamQ.fid);
}

/*
 * ======== VPMD_getVideoCmdEvtReadFd() ========
 * This function is to get video event/command read queue.
 *
 * Return Values:
 *   The fid of the device.
 */
int VPMD_getVideoCmdEvtReadFd(
    void)
{
    /* If a fd doesn't exist then create it. */
    if (0 == VPMD_ptr->muxOutFifo.videoCmdEvtQ.fid) {
        if (OSAL_SUCCESS != OSAL_fileOpen(
                &VPMD_ptr->muxOutFifo.videoCmdEvtQ.fid,
                VPMD_VIDEO_CMDEVT_R_FIFO, OSAL_FILE_O_RDWR, 0)) {
            VPMD_ptr->muxOutFifo.videoCmdEvtQ.fid = 0;
            OSAL_logMsg("%s:%d Open %s device FAIL.\n", __FUNCTION__, __LINE__,
                    VPMD_VIDEO_CMDEVT_R_FIFO);
            return (OSAL_FAIL);
        }
    }

    return (VPMD_ptr->muxOutFifo.videoCmdEvtQ.fid);
}

/*
 * ======== VPMD_getVideoStreamReadFd() ========
 * This function is to get video stream read queue.
 *
 * Return Values:
 *   The fid of the device.
 */
int VPMD_getVideoStreamReadFd(
    void)
{
    /* If a fd doesn't exist then create it. */
    if (0 == VPMD_ptr->muxOutFifo.videoStreamQ.fid) {
        if (OSAL_SUCCESS != OSAL_fileOpen(
                &VPMD_ptr->muxOutFifo.videoStreamQ.fid,
                VPMD_VIDEO_STREAM_R_FIFO, OSAL_FILE_O_RDWR, 0)) {
            VPMD_ptr->muxOutFifo.videoStreamQ.fid = 0;
            OSAL_logMsg("%s:%d Open %s device FAIL.\n", __FUNCTION__, __LINE__,
                    VPMD_VIDEO_STREAM_R_FIFO);
            return (OSAL_FAIL);
        }
    }

    return (VPMD_ptr->muxOutFifo.videoStreamQ.fid);
}

/*
 * ======== VPMD_getSipReadFd() ========
 * This function is to get sip read queue.
 *
 * Return Values:
 *   The fid of the device.
 */
int VPMD_getSipReadFd(
    void)
{
    /* If a fd doesn't exist then create it. */
    if (0 == VPMD_ptr->muxOutFifo.sipQ.fid) {
        if (OSAL_SUCCESS != OSAL_fileOpen(&VPMD_ptr->muxOutFifo.sipQ.fid,
                VPMD_SIP_R_FIFO, OSAL_FILE_O_RDWR, 0)) {
            VPMD_ptr->muxOutFifo.sipQ.fid = 0;
            OSAL_logMsg("%s:%d Open %s device FAIL.\n", __FUNCTION__, __LINE__,
                    VPMD_SIP_R_FIFO);
            return (OSAL_FAIL);
        }
    }

    return (VPMD_ptr->muxOutFifo.sipQ.fid);
}

/*
 * ======== VPMD_getCsmEvtReadFd() ========
 * This function is to get csm event read queue.
 *
 * Return Values:
 *   The fid of the device.
 */
int VPMD_getCsmEvtReadFd(
    void)
{
    /* If a fd doesn't exist then create it. */
    if (0 == VPMD_ptr->muxOutFifo.csmEvtQ.fid) {
        if (OSAL_SUCCESS != OSAL_fileOpen(&VPMD_ptr->muxOutFifo.csmEvtQ.fid,
                VPMD_CSM_EVT_R_FIFO, OSAL_FILE_O_RDWR, 0)) {
            VPMD_ptr->muxOutFifo.csmEvtQ.fid = 0;
            OSAL_logMsg("%s:%d Open %s device FAIL.\n", __FUNCTION__, __LINE__,
                    VPMD_CSM_EVT_R_FIFO);
            return (OSAL_FAIL);
        }
    }

    return (VPMD_ptr->muxOutFifo.csmEvtQ.fid);
}

/*
 * ======== VPMD_getIsipReadFd() ========
 * This function is to get isip read queue.
 *
 * Return Values:
 *   The fid of the device.
 */
int VPMD_getIsipReadFd(
    void)
{
    /* If a fd doesn't exist then create it. */
    if (0 == VPMD_ptr->muxOutFifo.isipQ.fid) {
        if (OSAL_SUCCESS != OSAL_fileOpen(&VPMD_ptr->muxOutFifo.isipQ.fid,
                VPMD_ISIP_R_FIFO, OSAL_FILE_O_RDWR, 0)) {
            VPMD_ptr->muxOutFifo.isipQ.fid = 0;
            OSAL_logMsg("%s:%d Open %s device FAIL.\n", __FUNCTION__, __LINE__,
                    VPMD_ISIP_R_FIFO);
            return (OSAL_FAIL);
        }
    }

    return (VPMD_ptr->muxOutFifo.isipQ.fid);
}
