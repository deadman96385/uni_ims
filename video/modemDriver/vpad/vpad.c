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
    } muxInFifo;  /* write data to MUX input FIFO, MUX will read them */
    struct {
        MODEM_Terminal  videoCmdEvtQ;
        MODEM_Terminal  videoStreamQ;
        MODEM_Terminal  voiceStreamQ;
        MODEM_Terminal  isiRpcQ;
        MODEM_Terminal  isiEvtRpcQ;
        MODEM_Terminal  sipQ;
        MODEM_Terminal  isipQ;
        MODEM_Terminal  csmEvtQ;
    } muxOutFifo;   /* read data from these FIFO, MUX will write data to them */
    VPR_Comm        vprCommBuffer;
} VPAD_Obj;

static VPAD_Obj  VPAD_driver; // C standard says this is zero.
static VPAD_Obj *VPAD_ptr = &VPAD_driver;

/*
 *  ======== _VPAD_readDataFromFifo() ========
 *  This function is used to read data from fifo.
 *
 *  Return Values:
 *  OSAL_SUCCESS: success to read.
 *  OSAL_FAIL: fail to read.
 */
OSAL_Status _VPAD_readDataFromFifo(
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
        OSAL_logMsg("%s:%d File select FAIL . Error %s(%d).\n",
                    __FUNCTION__, __LINE__,
                    strerror(errno), errno);
        OSAL_fileClose(fid_ptr);
        *fid_ptr = 0;
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
 *  ======== VPAD_writeVideoCmdEvt() ========
 *  This function is used to write video event to VPMD
 *
 *  Return Values:
 *  OSAL_SUCCESS: success to write command.
 *  OSAL_FAIL: fail to write command.
 */
OSAL_Status VPAD_writeVideoCmdEvt(
    void *buf_ptr,
    vint  size)
{
    VPR_Comm *vpr_ptr = (VPR_Comm *)buf_ptr;
    vpr_ptr->targetModule = VPMD_VIDEO_CMDEVT_TARGET;
    vpr_ptr->targetSize = sizeof(VPR_Comm);

    /* If a fd doesn't exist then create it. */
    if (0 == VPAD_ptr->muxInFifo.videoCmdEvtQ.fid) {
        if (OSAL_SUCCESS != OSAL_fileOpen(
                &VPAD_ptr->muxInFifo.videoCmdEvtQ.fid,
                VPAD_VIDEO_CMDEVT_W_FIFO, OSAL_FILE_O_RDWR, 0)) {
            VPAD_ptr->muxInFifo.videoCmdEvtQ.fid = 0;
            OSAL_logMsg("%s:%d Open %s device FAIL.\n", __FUNCTION__, __LINE__,
                    VPAD_VIDEO_CMDEVT_W_FIFO);
            return (OSAL_FAIL);
        }
    }

    /* Now write */
    if (OSAL_FAIL == OSAL_fileWrite(&VPAD_ptr->muxInFifo.videoCmdEvtQ.fid,
            vpr_ptr, &vpr_ptr->targetSize)) {
        OSAL_logMsg("%s:%d Write FIFO FAIL.\n", __FUNCTION__, __LINE__);
        return (OSAL_FAIL);
    }

    return (OSAL_SUCCESS);
}

/*
 *  ======== VPAD_readVideoCmdEvt() ========
 *  This function is used to read video command from VPMD
 *
 *  Return Values:
 *  OSAL_SUCCESS: success to read command.
 *  OSAL_FAIL: fail to read command.
 */
OSAL_Status VPAD_readVideoCmdEvt(
    void *buf_ptr,
    vint  size,
    vint  timeout)
{
    /* If a fd doesn't exist then create it. */
    if (0 == VPAD_ptr->muxOutFifo.videoCmdEvtQ.fid) {
        if (OSAL_SUCCESS != OSAL_fileOpen(
                &VPAD_ptr->muxOutFifo.videoCmdEvtQ.fid,
                VPAD_VIDEO_CMDEVT_R_FIFO, OSAL_FILE_O_RDWR, 0)) {
            VPAD_ptr->muxOutFifo.videoCmdEvtQ.fid = 0;
            OSAL_logMsg("%s:%d Open %s device FAIL.\n", __FUNCTION__, __LINE__,
                    VPAD_VIDEO_CMDEVT_R_FIFO);
            return (OSAL_FAIL);
        }
    }

    /* Now Read */
    return _VPAD_readDataFromFifo(&VPAD_ptr->muxOutFifo.videoCmdEvtQ.fid,
            buf_ptr, size, timeout);
}

/*
 *  ======== VPAD_writeVideoStream() ========
 *  This function is used to write video stream to VPMD
 *
 *  Return Values:
 *  OSAL_SUCCESS: success to write stream.
 *  OSAL_FAIL: fail to write stream.
 */
OSAL_Status VPAD_writeVideoStream(
    void *buf_ptr,
    vint  size)
{
    VPR_Comm *vpr_ptr = (VPR_Comm *)buf_ptr;
    vpr_ptr->targetModule = VPMD_VIDEO_STREAM_TARGET;
    vpr_ptr->targetSize = sizeof(VPR_Comm);

    /* If a fd doesn't exist then create it. */
    if (0 == VPAD_ptr->muxInFifo.videoStreamQ.fid) {
        if (OSAL_SUCCESS != OSAL_fileOpen(
                &VPAD_ptr->muxInFifo.videoStreamQ.fid,
                VPAD_VIDEO_STREAM_W_FIFO, OSAL_FILE_O_RDWR, 0)) {
            VPAD_ptr->muxInFifo.videoStreamQ.fid = 0;
            OSAL_logMsg("%s:%d Open %s device FAIL.\n", __FUNCTION__, __LINE__,
                    VPAD_VIDEO_STREAM_W_FIFO);
            return (OSAL_FAIL);
        }
    }

    /* Now write */
    if (OSAL_FAIL == OSAL_fileWrite(&VPAD_ptr->muxInFifo.videoStreamQ.fid,
            vpr_ptr, &vpr_ptr->targetSize)) {
        OSAL_logMsg("%s:%d Write FIFO FAIL.\n", __FUNCTION__, __LINE__);
        return (OSAL_FAIL);
    }

    return (OSAL_SUCCESS);
}

/*
 *  ======== VPAD_readVideoStream() ========
 *  This function is used to read video stream from VPMD
 *
 *  Return Values:
 *  OSAL_SUCCESS: success to read event.
 *  OSAL_FAIL: fail to read event.
 */
OSAL_Status VPAD_readVideoStream(
    void *buf_ptr,
    vint  size,
    vint  timeout)
{
    /* If a fd doesn't exist then create it. */
    if (0 == VPAD_ptr->muxOutFifo.videoStreamQ.fid) {
        if (OSAL_SUCCESS != OSAL_fileOpen(
                &VPAD_ptr->muxOutFifo.videoStreamQ.fid,
                VPAD_VIDEO_STREAM_R_FIFO, OSAL_FILE_O_RDWR, 0)) {
            VPAD_ptr->muxOutFifo.videoStreamQ.fid = 0;
            OSAL_logMsg("%s:%d Open %s device FAIL.\n", __FUNCTION__, __LINE__,
                    VPAD_VIDEO_STREAM_R_FIFO);
            return (OSAL_FAIL);
        }
    }

    /* Now Read */
    return _VPAD_readDataFromFifo(&VPAD_ptr->muxOutFifo.videoStreamQ.fid,
            buf_ptr, size, timeout);
}

/*
 *  ======== VPAD_writeVoiceStream() ========
 *  This function is used to write voice stream to VPMD
 *
 *  Return Values:
 *  OSAL_SUCCESS: success to write stream.
 *  OSAL_FAIL: fail to write stream.
 */
OSAL_Status VPAD_writeVoiceStream(
    void *buf_ptr,
    vint  size)
{
    VPR_Comm *vpr_ptr = (VPR_Comm *)buf_ptr;
    vpr_ptr->targetModule = VPMD_VOICE_STREAM_TARGET;
    vpr_ptr->targetSize = sizeof(VPR_Comm);

    /* If a fd doesn't exist then create it. */
    if (0 == VPAD_ptr->muxInFifo.voiceStreamQ.fid) {
        if (OSAL_SUCCESS != OSAL_fileOpen(
                &VPAD_ptr->muxInFifo.voiceStreamQ.fid,
                VPAD_VOICE_STREAM_W_FIFO, OSAL_FILE_O_RDWR, 0)) {
            VPAD_ptr->muxInFifo.voiceStreamQ.fid = 0;
            OSAL_logMsg("%s:%d Open %s device FAIL.\n", __FUNCTION__, __LINE__,
                    VPAD_VOICE_STREAM_W_FIFO);
            return (OSAL_FAIL);
        }
    }

    /* Now write */
    if (OSAL_FAIL == OSAL_fileWrite(&VPAD_ptr->muxInFifo.voiceStreamQ.fid,
            vpr_ptr, &vpr_ptr->targetSize)) {
        OSAL_logMsg("%s:%d Write FIFO FAIL.\n", __FUNCTION__, __LINE__);
        return (OSAL_FAIL);
    }

    return (OSAL_SUCCESS);
}

/*
 *  ======== VPAD_readVoiceStream() ========
 *  This function is used to read voice stream from VPMD
 *
 *  Return Values:
 *  OSAL_SUCCESS: success to read stream.
 *  OSAL_FAIL: fail to read stream.
 */
OSAL_Status VPAD_readVoiceStream(
    void *buf_ptr,
    vint  size,
    vint  timeout)
{
    /* If a fd doesn't exist then create it. */
    if (0 == VPAD_ptr->muxOutFifo.voiceStreamQ.fid) {
        if (OSAL_SUCCESS != OSAL_fileOpen(
                &VPAD_ptr->muxOutFifo.voiceStreamQ.fid,
                VPAD_VOICE_STREAM_R_FIFO, OSAL_FILE_O_RDWR, 0)) {
            VPAD_ptr->muxOutFifo.voiceStreamQ.fid = 0;
            OSAL_logMsg("%s:%d Open %s device FAIL.\n", __FUNCTION__, __LINE__,
                    VPAD_VOICE_STREAM_R_FIFO);
            return (OSAL_FAIL);
        }
    }

    /* Now Read */
    return _VPAD_readDataFromFifo(&VPAD_ptr->muxOutFifo.voiceStreamQ.fid,
            buf_ptr, size, timeout);
}

/*
 *  ======== VPAD_writeIsiRpc() ========
 *  This function is used to call ISI function.
 *
 *  Return Values:
 *  OSAL_SUCCESS: success to write.
 *  OSAL_FAIL: fail to write.
 */
OSAL_Status VPAD_writeIsiRpc(
    void *buf_ptr,
    vint  size)
{
    VPR_Comm *vpr_ptr = &(VPAD_ptr->vprCommBuffer);

    /* wrap inside vpr_comm buffer */
    OSAL_memCpy(vpr_ptr, buf_ptr, size);
    vpr_ptr->targetSize = size;
    size = sizeof(VPR_Comm);
    buf_ptr = vpr_ptr;
    vpr_ptr->targetModule = VPMD_ISI_RPC_TARGET;

    /* If a fd doesn't exist then create it. */
    if (0 == VPAD_ptr->muxInFifo.isiRpcQ.fid) {
        if (OSAL_SUCCESS != OSAL_fileOpen(&VPAD_ptr->muxInFifo.isiRpcQ.fid,
                VPAD_ISI_RPC_W_FIFO, OSAL_FILE_O_RDWR, 0)) {
            VPAD_ptr->muxInFifo.isiRpcQ.fid = 0;
            OSAL_logMsg("%s:%d Open %s device FAIL.\n", __FUNCTION__, __LINE__,
                    VPAD_ISI_RPC_W_FIFO);
            return (OSAL_FAIL);
        }
    }

    /* Now write */
    if (OSAL_FAIL == OSAL_fileWrite(&VPAD_ptr->muxInFifo.isiRpcQ.fid,
            buf_ptr, &size)) {
        OSAL_logMsg("%s:%d Write FIFO FAIL.\n", __FUNCTION__, __LINE__);
        return (OSAL_FAIL);
    }

    return (OSAL_SUCCESS);
}

/*
 *  ======== VPAD_readIsiRpc() ========
 *  This function is used to read ISI function return from VPMD.
 *
 *  Return Values:
 *  OSAL_SUCCESS: success to read.
 *  OSAL_FAIL: fail to read.
 */
OSAL_Status VPAD_readIsiRpc(
    void *buf_ptr,
    vint  size,
    vint  timeout)
{
    /* If a fd doesn't exist then create it. */
    if (0 == VPAD_ptr->muxOutFifo.isiRpcQ.fid) {
        if (OSAL_SUCCESS != OSAL_fileOpen(&VPAD_ptr->muxOutFifo.isiRpcQ.fid,
                VPAD_ISI_RPC_R_FIFO, OSAL_FILE_O_RDWR, 0)) {
            VPAD_ptr->muxOutFifo.isiRpcQ.fid = 0;
            OSAL_logMsg("%s:%d Open %s device FAIL.\n", __FUNCTION__, __LINE__,
                    VPAD_ISI_RPC_R_FIFO);
            return (OSAL_FAIL);
        }
    }

    /* Now Read */
    return _VPAD_readDataFromFifo(&VPAD_ptr->muxOutFifo.isiRpcQ.fid, buf_ptr,
            size, timeout);
}

/*
 *  ======== VPAD_writeIsiEvtRpc() ========
 *  This function is used to call ISI function, ISI_getEvent().
 *
 *  Return Values:
 *  OSAL_SUCCESS: success to write.
 *  OSAL_FAIL: fail to write.
 */
OSAL_Status VPAD_writeIsiEvtRpc(
    void *buf_ptr,
    vint  size)
{
    VPR_Comm *vpr_ptr = &(VPAD_ptr->vprCommBuffer);

    /* wrap inside vpr_comm buffer */
    OSAL_memCpy(vpr_ptr, buf_ptr, size);
    vpr_ptr->targetSize = size;
    size = sizeof(VPR_Comm);
    buf_ptr = vpr_ptr;

    vpr_ptr->targetModule = VPMD_ISI_EVT_RPC_TARGET;

    /* If a fd doesn't exist then create it. */
    if (0 == VPAD_ptr->muxInFifo.isiEvtRpcQ.fid) {
        if (OSAL_SUCCESS != OSAL_fileOpen(&VPAD_ptr->muxInFifo.isiEvtRpcQ.fid,
                VPAD_ISI_EVT_RPC_W_FIFO, OSAL_FILE_O_RDWR, 0)) {
            VPAD_ptr->muxInFifo.isiEvtRpcQ.fid = 0;
            OSAL_logMsg("%s:%d Open %s device FAIL.\n", __FUNCTION__, __LINE__,
                    VPAD_ISI_EVT_RPC_W_FIFO);
            return (OSAL_FAIL);
        }
    }

    /* Now write */
    if (OSAL_FAIL == OSAL_fileWrite(&VPAD_ptr->muxInFifo.isiEvtRpcQ.fid,
            buf_ptr, &size)) {
        OSAL_logMsg("%s:%d Write FIFO FAIL.\n", __FUNCTION__, __LINE__);
        return (OSAL_FAIL);
    }

    return (OSAL_SUCCESS);
}

/*
 *  ======== VPAD_readIsiEvtRpc() ========
 *  This function is used to read the result of ISI function, ISI_getEvent().
 *
 *  Return Values:
 *  OSAL_SUCCESS: success to read.
 *  OSAL_FAIL: fail to read.
 */
OSAL_Status VPAD_readIsiEvtRpc(
    void *buf_ptr,
    vint  size,
    vint  timeout)
{
    /* If a fd doesn't exist then create it. */
    if (0 == VPAD_ptr->muxOutFifo.isiEvtRpcQ.fid) {
        if (OSAL_SUCCESS != OSAL_fileOpen(&VPAD_ptr->muxOutFifo.isiEvtRpcQ.fid,
                VPAD_ISI_EVT_RPC_R_FIFO, OSAL_FILE_O_RDONLY, 0)) {
            VPAD_ptr->muxOutFifo.isiEvtRpcQ.fid = 0;
            OSAL_logMsg("%s:%d Open %s fifo FAIL.\n", __FUNCTION__, __LINE__,
                    VPAD_ISI_EVT_RPC_R_FIFO);
            return (OSAL_FAIL);
        }
    }

    /* Now Read */
    return _VPAD_readDataFromFifo(&VPAD_ptr->muxOutFifo.isiEvtRpcQ.fid,
            buf_ptr, size, timeout);
}

/*
 *  ======== VPAD_writeSip() ========
 *  This function is used to write sip command or data to VPR
 *
 *  Return Values:
 *  OSAL_SUCCESS: success to write.
 *  OSAL_FAIL: fail to write.
 */
OSAL_Status VPAD_writeSip(
    void *buf_ptr,
    vint  size)
{
    VPR_Comm *vpr_ptr = &(VPAD_ptr->vprCommBuffer);

    /* wrap inside vpr_comm buffer */
    OSAL_memCpy(vpr_ptr, buf_ptr, size);
    vpr_ptr->targetSize = size;
    size = sizeof(VPR_Comm);
    buf_ptr = vpr_ptr;

    vpr_ptr->targetModule = VPMD_SIP_TARGET;

    /* If a fd doesn't exist then create it. */
    if (0 == VPAD_ptr->muxInFifo.sipQ.fid) {
        if (OSAL_SUCCESS != OSAL_fileOpen(&VPAD_ptr->muxInFifo.sipQ.fid,
                VPAD_SIP_W_FIFO, OSAL_FILE_O_RDWR, 0)) {
            VPAD_ptr->muxInFifo.sipQ.fid = 0;
            OSAL_logMsg("%s:%d Open %s device FAIL.\n", __FUNCTION__, __LINE__,
                    VPAD_SIP_W_FIFO);
            return (OSAL_FAIL);
        }
    }

    /* Now write */
    if (OSAL_FAIL == OSAL_fileWrite(&VPAD_ptr->muxInFifo.sipQ.fid,
            buf_ptr, &size)) {
        OSAL_logMsg("%s:%d Write FIFO FAIL.\n", __FUNCTION__, __LINE__);
        return (OSAL_FAIL);
    }

    return (OSAL_SUCCESS);
}

/*
 *  ======== VPAD_readSip() ========
 *  This function is used to read sip data from VPR
 *
 *  Return Values:
 *  OSAL_SUCCESS: success to read.
 *  OSAL_FAIL: fail to read.
 */
OSAL_Status VPAD_readSip(
    void *buf_ptr,
    vint  size,
    vint  timeout)
{
    /* If a fd doesn't exist then create it. */
    if (0 == VPAD_ptr->muxOutFifo.sipQ.fid) {
        if (OSAL_SUCCESS != OSAL_fileOpen(&VPAD_ptr->muxOutFifo.sipQ.fid,
                VPAD_SIP_R_FIFO, OSAL_FILE_O_RDWR, 0)) {
            VPAD_ptr->muxOutFifo.sipQ.fid = 0;
            OSAL_logMsg("%s:%d Open %s device FAIL.\n", __FUNCTION__, __LINE__,
                    VPAD_SIP_R_FIFO);
            return (OSAL_FAIL);
        }
    }

    /* Now Read */
    return _VPAD_readDataFromFifo(&VPAD_ptr->muxOutFifo.sipQ.fid, buf_ptr,
            size, timeout);
}

/*
 *  ======== VPAD_writeIsip() ========
 *  This function is used to write isip message to VPR
 *
 *  Return Values:
 *  OSAL_SUCCESS: success to write.
 *  OSAL_FAIL: fail to write.
 */
OSAL_Status VPAD_writeIsip(
    void *buf_ptr,
    vint  size)
{
    VPR_Comm *vpr_ptr = &(VPAD_ptr->vprCommBuffer);

    /* wrap inside vpr_comm buffer */
    OSAL_memCpy(vpr_ptr, buf_ptr, size);
    vpr_ptr->targetSize = size;
    size = sizeof(VPR_Comm);
    buf_ptr = vpr_ptr;

    vpr_ptr->targetModule = VPMD_ISIP_TARGET;

    /* If a fd doesn't exist then create it. */
    if (0 == VPAD_ptr->muxInFifo.isipQ.fid) {
        if (OSAL_SUCCESS != OSAL_fileOpen(&VPAD_ptr->muxInFifo.isipQ.fid,
                VPAD_ISIP_W_FIFO, OSAL_FILE_O_RDWR, 0)) {
            VPAD_ptr->muxInFifo.isipQ.fid = 0;
            OSAL_logMsg("%s:%d Open %s device FAIL.\n", __FUNCTION__, __LINE__,
                    VPAD_ISIP_W_FIFO);
            return (OSAL_FAIL);
        }
    }

    /* Now write */
    if (OSAL_FAIL == OSAL_fileWrite(&VPAD_ptr->muxInFifo.isipQ.fid,
            buf_ptr, &size)) {
        OSAL_logMsg("%s:%d Write FIFO FAIL.\n", __FUNCTION__, __LINE__);
        return (OSAL_FAIL);
    }

    return (OSAL_SUCCESS);
}

/*
 *  ======== VPAD_readIsip() ========
 *  This function is used to read isip message from VPR
 *
 *  Return Values:
 *  OSAL_SUCCESS: success to read.
 *  OSAL_FAIL: fail to read.
 */
OSAL_Status VPAD_readIsip(
    void *buf_ptr,
    vint  size,
    vint  timeout)
{
    /* If a fd doesn't exist then create it. */
    if (0 == VPAD_ptr->muxOutFifo.isipQ.fid) {
        if (OSAL_SUCCESS != OSAL_fileOpen(&VPAD_ptr->muxOutFifo.isipQ.fid,
                VPAD_ISIP_R_FIFO, OSAL_FILE_O_RDWR, 0)) {
            VPAD_ptr->muxOutFifo.isipQ.fid = 0;
            OSAL_logMsg("%s:%d Open %s device FAIL.\n", __FUNCTION__, __LINE__,
                    VPAD_ISIP_R_FIFO);
            return (OSAL_FAIL);
        }
    }

    /* Now Read */
    return _VPAD_readDataFromFifo(&VPAD_ptr->muxOutFifo.isipQ.fid, buf_ptr,
            size, timeout);
}

/*
 *  ======== VPAD_writeCsmEvt() ========
 *  This function is used to write CSM event to VPR
 *
 *  Return Values:
 *  OSAL_SUCCESS: success to write.
 *  OSAL_FAIL: fail to write.
 */
OSAL_Status VPAD_writeCsmEvt(
    void *buf_ptr,
    vint  size)
{
    VPR_Comm *vpr_ptr = &(VPAD_ptr->vprCommBuffer);

    /* wrap inside vpr_comm buffer */
    OSAL_memCpy(vpr_ptr, buf_ptr, size);
    vpr_ptr->targetSize = size;
    size = sizeof(VPR_Comm);
    buf_ptr = vpr_ptr;
    vpr_ptr->targetModule = VPMD_CSM_EVT_TARGET;

    /* If a fd doesn't exist then create it. */
    if (0 == VPAD_ptr->muxInFifo.csmEvtQ.fid) {
        if (OSAL_SUCCESS != OSAL_fileOpen(&VPAD_ptr->muxInFifo.csmEvtQ.fid,
                VPAD_CSM_EVT_W_FIFO, OSAL_FILE_O_RDWR, 0)) {
            VPAD_ptr->muxInFifo.csmEvtQ.fid = 0;
            OSAL_logMsg("%s:%d Open %s device FAIL.\n", __FUNCTION__, __LINE__,
                    VPAD_CSM_EVT_W_FIFO);
            return (OSAL_FAIL);
        }
    }

    /* Now write */
    if (OSAL_FAIL == OSAL_fileWrite(&VPAD_ptr->muxInFifo.csmEvtQ.fid,
            buf_ptr, &size)) {
        OSAL_logMsg("%s:%d Write FIFO FAIL.\n", __FUNCTION__, __LINE__);
        return (OSAL_FAIL);
    }

    return (OSAL_SUCCESS);
}

/*
 * ======== VPAD_getVoiceStreamReadFd() ========
 * This function is to get voice stream read fd
 *
 * Return Values:
 * NULL: Failed to get queue.
 * Otherwise: OSAL_MsgQId of the queue.
 */
int VPAD_getVoiceStreamReadFd(
    void)
{
    /* If a fd doesn't exist then create it. */
    if (0 == VPAD_ptr->muxOutFifo.voiceStreamQ.fid) {
        if (OSAL_SUCCESS != OSAL_fileOpen(
                &VPAD_ptr->muxOutFifo.voiceStreamQ.fid,
                VPAD_VOICE_STREAM_R_FIFO, OSAL_FILE_O_RDWR, 0)) {
            VPAD_ptr->muxOutFifo.voiceStreamQ.fid = 0;
            OSAL_logMsg("%s:%d Open %s device FAIL.\n", __FUNCTION__, __LINE__,
                    VPAD_VOICE_STREAM_R_FIFO);
            return (0);
        }
    }

    return (VPAD_ptr->muxOutFifo.voiceStreamQ.fid);
}

/*
 * ======== VPAD_isReady() ========
 * This function is to query whether vpad has established communications with
 * vpmd.
 * Since the querier is in different process with vpad mux daemon,
 * we used the trick by quering whether write fifo is established
 * by vpad mux daemon to get the readiness status.
 *
 * Return Values:
 * OSAL_TRUE: vpad is ready
 * OSAL_FALSE: vpad is still trying to get in contact with vpmd.
 */
OSAL_Boolean VPAD_isReady()
{
    return OSAL_fileExists(VPAD_VOICE_STREAM_W_FIFO);
}

#ifdef VP4G_PLUS_MODEM_TEST

/*
 * ======== VPAD_getVideoCmdEvtReadFd() ========
 * This function is to get video event/command read queue.
 *
 * Return Values:
 *   The fid of the device.
 */
int VPAD_getVideoCmdEvtReadFd(
    void)
{
    /* If a fd doesn't exist then create it. */
    if (0 == VPAD_ptr->muxOutFifo.videoCmdEvtQ.fid) {
        if (OSAL_SUCCESS != OSAL_fileOpen(
                &VPAD_ptr->muxOutFifo.videoCmdEvtQ.fid,
                VPAD_VIDEO_CMDEVT_R_FIFO, OSAL_FILE_O_RDWR, 0)) {
            VPAD_ptr->muxOutFifo.videoCmdEvtQ.fid = 0;
            OSAL_logMsg("%s:%d Open %s device FAIL.\n", __FUNCTION__, __LINE__,
                    VPAD_VIDEO_CMDEVT_R_FIFO);
            return (OSAL_FAIL);
        }
    }

    return (VPAD_ptr->muxOutFifo.videoCmdEvtQ.fid);
}

/*
 * ======== VPAD_getVideoStreamReadFd() ========
 * This function is to get video stream read queue.
 *
 * Return Values:
 *   The fid of the device.
 */
int VPAD_getVideoStreamReadFd(
    void)
{
    /* If a fd doesn't exist then create it. */
    if (0 == VPAD_ptr->muxOutFifo.videoStreamQ.fid) {
        if (OSAL_SUCCESS != OSAL_fileOpen(
                &VPAD_ptr->muxOutFifo.videoStreamQ.fid,
                VPAD_VIDEO_STREAM_R_FIFO, OSAL_FILE_O_RDWR, 0)) {
            VPAD_ptr->muxOutFifo.videoStreamQ.fid = 0;
            OSAL_logMsg("%s:%d Open %s device FAIL.\n", __FUNCTION__, __LINE__,
                    VPAD_VIDEO_STREAM_R_FIFO);
            return (OSAL_FAIL);
        }
    }

    return (VPAD_ptr->muxOutFifo.videoStreamQ.fid);
}

/*
 * ======== VPAD_getSipReadFd() ========
 * This function is to get sip read queue.
 *
 * Return Values:
 *   The fid of the device.
 */
int VPAD_getSipReadFd(
    void)
{
    /* If a fd doesn't exist then create it. */
    if (0 == VPAD_ptr->muxOutFifo.sipQ.fid) {
        if (OSAL_SUCCESS != OSAL_fileOpen(&VPAD_ptr->muxOutFifo.sipQ.fid,
                VPAD_SIP_R_FIFO, OSAL_FILE_O_RDWR, 0)) {
            VPAD_ptr->muxOutFifo.sipQ.fid = 0;
            OSAL_logMsg("%s:%d Open %s device FAIL.\n", __FUNCTION__, __LINE__,
                    VPAD_SIP_R_FIFO);
            return (OSAL_FAIL);
        }
    }

    return (VPAD_ptr->muxOutFifo.sipQ.fid);
}

/*
 * ======== VPAD_getCsmEvtReadFd() ========
 * This function is to get csm event read queue.
 *
 * Return Values:
 *   The fid of the device.
 */
int VPAD_getCsmEvtReadFd(
    void)
{
    /* If a fd doesn't exist then create it. */
    if (0 == VPAD_ptr->muxOutFifo.csmEvtQ.fid) {
        if (OSAL_SUCCESS != OSAL_fileOpen(&VPAD_ptr->muxOutFifo.csmEvtQ.fid,
                VPAD_CSM_EVT_R_FIFO, OSAL_FILE_O_RDWR, 0)) {
            VPAD_ptr->muxOutFifo.csmEvtQ.fid = 0;
            OSAL_logMsg("%s:%d Open %s device FAIL.\n", __FUNCTION__, __LINE__,
                    VPAD_CSM_EVT_R_FIFO);
            return (OSAL_FAIL);
        }
    }

    return (VPAD_ptr->muxOutFifo.csmEvtQ.fid);
}

/*
 * ======== VPAD_getIsipReadFd() ========
 * This function is to get isip read queue.
 *
 * Return Values:
 *   The fid of the device.
 */
int VPAD_getIsipReadFd(
    void)
{
    /* If a fd doesn't exist then create it. */
    if (0 == VPAD_ptr->muxOutFifo.isipQ.fid) {
        if (OSAL_SUCCESS != OSAL_fileOpen(&VPAD_ptr->muxOutFifo.isipQ.fid,
                VPAD_ISIP_R_FIFO, OSAL_FILE_O_RDWR, 0)) {
            VPAD_ptr->muxOutFifo.isipQ.fid = 0;
            OSAL_logMsg("%s:%d Open %s device FAIL.\n", __FUNCTION__, __LINE__,
                    VPAD_ISIP_R_FIFO);
            return (OSAL_FAIL);
        }
    }

    return (VPAD_ptr->muxOutFifo.isipQ.fid);
}
#endif /* VP4G_PLUS_MODEM_TEST */
