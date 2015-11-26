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
#include "vpad_io.h"
#include "vpad_mux.h"
#include "_vpad_mux_helper.h"

static VPAD_muxObj  _VPAD_muxDriver; // C standard says this is zero.
static VPAD_muxObj *_VPAD_muxPtr = &_VPAD_muxDriver;

#ifdef VPAD_MUX_STATS

/*
 * ======== _VPAD_muxStatsUpdate() ========
 *
 * Private function to update statistics with the size increased.
 *
 * RETURN:
 * none
 */
void _VPAD_muxStatsUpdate(
    MODEM_Terminal *q_ptr,
    vint            size)
{
    q_ptr->pktCount++;
    q_ptr->szCount += size;
}

/*
 * ======== _VPAD_muxCollectVideoCounters ========
 *
 * Private function to collect videoCmdEvtQ statistics
 *
 * RETURN:
 * none
 */
static void _VPAD_muxCollectVideoCounters(
    VPR_Comm       *vpr_ptr)
{
    switch (vpr_ptr->type) {
        case VPR_TYPE_VTSP_CMD:
            _VPAD_muxPtr->videoCounters.vtspCmdCount += 1;
            break;
        case VPR_TYPE_VTSP_EVT:
            _VPAD_muxPtr->videoCounters.vtspEvtCount += 1;
            break;
        case VPR_TYPE_RTCP_CMD:
            _VPAD_muxPtr->videoCounters.vtspRtcpCmdCount += 1;
            break;
        case VPR_TYPE_RTCP_EVT:
            _VPAD_muxPtr->videoCounters.vtspRtcpEventMsgCount += 1;
            break;
        case VPR_TYPE_NET:
            _VPAD_muxPtr->videoCounters.vprNetCount += 1;
            break;
        case VPR_TYPE_ISIP:
        case VPR_TYPE_CSM_EVT:
        case VPR_TYPE_NETWORK_MODE:
        default:
            break;
    }
}

/*
 * ======== _VPAD_muxVideoCountersReport ========
 *
 * Private function to report videoCmdEvtQ statistics
 *
 * RETURN:
 * none
 */
static void _VPAD_muxVideoCountersReport()
{
    OSAL_logMsg("%16s \t %d\n", \
            "vtspCmdCount",
            _VPAD_muxPtr->videoCounters.vtspCmdCount);
    OSAL_logMsg("%16s \t %d\n", \
            "vtspEvtCount",
            _VPAD_muxPtr->videoCounters.vtspEvtCount);
     OSAL_logMsg("%16s \t %d\n", \
            "vtspRtcpCmdCount",
            _VPAD_muxPtr->videoCounters.vtspRtcpCmdCount);
     OSAL_logMsg("%16s \t %d\n", \
            "vtspRtcpEventMsgCount",
            _VPAD_muxPtr->videoCounters.vtspRtcpEventMsgCount);
     OSAL_logMsg("%16s \t %d\n", \
            "vprNetCount",
            _VPAD_muxPtr->videoCounters.vprNetCount);
}

/*
 * ======== _VPAD_muxStatsReport ========
 *
 * Private function to check and report statistics periodically
 *
 * RETURN:
 * none
 */
static void _VPAD_muxStatsReport()
{
    OSAL_TimeVal myTimeVal;

    OSAL_timeGetTimeOfDay(&myTimeVal);
    VPAD_muxDbgPrintf("[D2Log] _VPAD_muxStatsReport, timeSec: %d, timeUsec:%d, lastTpSec: %d\n",
	myTimeVal.sec, myTimeVal.usec, _VPAD_muxPtr->lastStatTimeStamp.sec);
    if (myTimeVal.sec <
            (_VPAD_muxPtr->lastStatTimeStamp.sec+VPAD_MUX_STAT_TIMER)) {
        return;
    }

    _VPAD_muxPtr->lastStatTimeStamp = myTimeVal;
    OSAL_logMsg("\n[statistics dump]\n");
    OSAL_logMsg("%36s \t pkt, \t sz, \t, time sec:%d usec:%d\n",
            "qname", myTimeVal.sec, myTimeVal.usec);

    _VPAD_MUX_DUMP_ONE_STAT(_VPAD_muxPtr->inFifo.videoCmdEvtQ);
    _VPAD_MUX_DUMP_ONE_STAT(_VPAD_muxPtr->inFifo.videoStreamQ);
   // _VPAD_MUX_DUMP_ONE_STAT(_VPAD_muxPtr->inFifo.voiceStreamQ);
   // _VPAD_MUX_DUMP_ONE_STAT(_VPAD_muxPtr->inFifo.isiRpcQ);
   // _VPAD_MUX_DUMP_ONE_STAT(_VPAD_muxPtr->inFifo.isiEvtRpcQ);
   // _VPAD_MUX_DUMP_ONE_STAT(_VPAD_muxPtr->inFifo.sipQ);
   // _VPAD_MUX_DUMP_ONE_STAT(_VPAD_muxPtr->inFifo.isipQ);
   // _VPAD_MUX_DUMP_ONE_STAT(_VPAD_muxPtr->inFifo.csmEvtQ);

    _VPAD_MUX_DUMP_ONE_STAT(_VPAD_muxPtr->outFifo.videoCmdEvtQ);
    _VPAD_MUX_DUMP_ONE_STAT(_VPAD_muxPtr->outFifo.videoStreamQ);
   // _VPAD_MUX_DUMP_ONE_STAT(_VPAD_muxPtr->outFifo.voiceStreamQ);
   // _VPAD_MUX_DUMP_ONE_STAT(_VPAD_muxPtr->outFifo.isiRpcQ);
   // _VPAD_MUX_DUMP_ONE_STAT(_VPAD_muxPtr->outFifo.isiEvtRpcQ);
   // _VPAD_MUX_DUMP_ONE_STAT(_VPAD_muxPtr->outFifo.sipQ);
   // _VPAD_MUX_DUMP_ONE_STAT(_VPAD_muxPtr->outFifo.isipQ);
   // _VPAD_MUX_DUMP_ONE_STAT(_VPAD_muxPtr->outFifo.csmEvtQ);
   OSAL_logMsg("\n");
#ifdef VPAD_MUX_DEBUG
    _VPAD_muxVideoCountersReport();
#endif

}
#endif // VPAD_MUX_STATS

/*
 * ======== _VPMD_muxInitFifo ========
 *
 * Private function to initialize fifo as reading queue
 *
 * RETURN:
 * OSAL_SUCCESS: Initialized successfully
 * OSAL_FAIL: Initialize failed.
 */
static OSAL_Status _VPAD_muxInitFifo(
    const char     *devicePath_ptr,
    MODEM_Terminal *terminal_ptr,
    int             size)
{
    OSAL_strcpy(terminal_ptr->name, devicePath_ptr);
    OSAL_fileFifoDelete(devicePath_ptr); /* rm $devicePath_ptr */
    OSAL_fileFifoCreate(devicePath_ptr, VPAD_FIFO_MSG_COUNT, size);
    if (OSAL_SUCCESS != OSAL_fileOpen(&terminal_ptr->fid,
            devicePath_ptr, OSAL_FILE_O_RDWR, 0)) {
        terminal_ptr->fid = 0;
        VPAD_destroy();
        OSAL_logMsg("%s:%d Open %s device FAIL.\n", __FUNCTION__, __LINE__,
                devicePath_ptr);
        return (OSAL_FAIL);
    }

    /* Chmod to 666, let FIFO could be read/write */
    chmod(devicePath_ptr, 0666);

    return (OSAL_SUCCESS);
}

/*
 * ======== _VPAD_muxDestroyFifo ========
 *
 * Private function to initialize fifo as reading queue
 *
 * RETURN:
 * OSAL_SUCCESS: Initialized successfully
 * OSAL_FAIL: Initialize failed.
 */
static OSAL_Status _VPAD_muxDestroyFifo(
    const char     *devicePath_ptr,
    MODEM_Terminal *terminal_ptr)
{
    if (NULL == terminal_ptr) {
        return (OSAL_FAIL);
    }

    if (0 != terminal_ptr->fid) {
        OSAL_fileClose(&terminal_ptr->fid);
        terminal_ptr->fid = 0;
    }

    OSAL_fileFifoDelete(devicePath_ptr); /* rm $devicePath_ptr */

    VPAD_muxDbgPrintf("Fifo deleted: %s\n", devicePath_ptr);
    return (OSAL_SUCCESS);
}

/*
 *  ======== _VPAD_muxWriteDataToFifo() ========
 *  This function is used to write data to fifo.
 *
 *  Return Values:
 *  OSAL_SUCCESS: success to read.
 *  OSAL_FAIL: fail to read.
 */
OSAL_Status _VPAD_muxWriteDataToFifo(
    MODEM_Terminal *toFifo,
    void           *buf_ptr,
    vint            size,
    vint            timeout)
{
    vint *fid_ptr = &toFifo->fid;
    char *name_ptr = toFifo->name;


    /* If a fd doesn't exist then create it. */
    if (0 == *fid_ptr) {
        OSAL_logMsg("%s:%d write %s fifo but it is not opened yet.\n",
                __FUNCTION__, __LINE__, name_ptr);
        if (OSAL_SUCCESS != OSAL_fileOpen(fid_ptr,
                name_ptr, OSAL_FILE_O_RDWR, 0)) {
            *fid_ptr = 0;
            OSAL_logMsg("%s:%d Open %s fifo FAIL.\n",
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

    /* Update statistics if needed */
    _VPAD_MUX_STATS_UPDATE(toFifo, size);

    return (OSAL_SUCCESS);
}

/*
 *  ======== _VPAD_muxFifoInit() ========
 *  Private function to initialize read/write FIFOs.
 *
 *  Return Values:
 *  OSAL_SUCCESS: success to initialize FIFOs.
 *  OSAL_FAIL: fail to initialize FIFOs.
 */
OSAL_Status _VPAD_muxFifoInit()
{
    /* initialize fifo device for reading */
    if (OSAL_SUCCESS != _VPAD_muxInitFifo(VPAD_VIDEO_CMDEVT_R_FIFO,
            &_VPAD_muxPtr->outFifo.videoCmdEvtQ, sizeof(VPR_Comm))) {
        return (OSAL_FAIL);
    }
    if (OSAL_SUCCESS != _VPAD_muxInitFifo(VPAD_VIDEO_STREAM_R_FIFO,
            &_VPAD_muxPtr->outFifo.videoStreamQ, sizeof(VPR_Comm))) {
        return (OSAL_FAIL);
    }
    if (OSAL_SUCCESS != _VPAD_muxInitFifo(VPAD_VOICE_STREAM_R_FIFO,
            &_VPAD_muxPtr->outFifo.voiceStreamQ, sizeof(VPR_Comm))) {
        return (OSAL_FAIL);
    }
    if (OSAL_SUCCESS != _VPAD_muxInitFifo(VPAD_ISI_RPC_R_FIFO,
            &_VPAD_muxPtr->outFifo.isiRpcQ, sizeof(VPR_Comm))) {
        return (OSAL_FAIL);
    }
    if (OSAL_SUCCESS != _VPAD_muxInitFifo(VPAD_ISI_EVT_RPC_R_FIFO,
            &_VPAD_muxPtr->outFifo.isiEvtRpcQ, sizeof(VPR_Comm))) {
        return (OSAL_FAIL);
    }
    if (OSAL_SUCCESS != _VPAD_muxInitFifo(VPAD_SIP_R_FIFO,
            &_VPAD_muxPtr->outFifo.sipQ, sizeof(VPR_Comm))) {
        return (OSAL_FAIL);
    }
    if (OSAL_SUCCESS != _VPAD_muxInitFifo(VPAD_ISIP_R_FIFO,
            &_VPAD_muxPtr->outFifo.isipQ, sizeof(VPR_Comm))) {
        return (OSAL_FAIL);
    }
    if (OSAL_SUCCESS != _VPAD_muxInitFifo(VPAD_CSM_EVT_R_FIFO,
            &_VPAD_muxPtr->outFifo.csmEvtQ, sizeof(VPR_Comm))) {
        return (OSAL_FAIL);
    }

    /* initialize fifo device for writing */
    if (OSAL_SUCCESS != _VPAD_muxInitFifo(VPAD_VIDEO_CMDEVT_W_FIFO,
            &_VPAD_muxPtr->inFifo.videoCmdEvtQ, sizeof(VPR_Comm))) {
        return (OSAL_FAIL);
    }
    if (OSAL_SUCCESS != _VPAD_muxInitFifo(VPAD_VIDEO_STREAM_W_FIFO,
            &_VPAD_muxPtr->inFifo.videoStreamQ, sizeof(VPR_Comm))) {
        return (OSAL_FAIL);
    }
    if (OSAL_SUCCESS != _VPAD_muxInitFifo(VPAD_VOICE_STREAM_W_FIFO,
            &_VPAD_muxPtr->inFifo.voiceStreamQ, sizeof(VPR_Comm))) {
        return (OSAL_FAIL);
    }
    if (OSAL_SUCCESS != _VPAD_muxInitFifo(VPAD_ISI_RPC_W_FIFO,
            &_VPAD_muxPtr->inFifo.isiRpcQ, sizeof(VPR_Comm))) {
        return (OSAL_FAIL);
    }
    if (OSAL_SUCCESS != _VPAD_muxInitFifo(VPAD_ISI_EVT_RPC_W_FIFO,
            &_VPAD_muxPtr->inFifo.isiEvtRpcQ, sizeof(VPR_Comm))) {
        return (OSAL_FAIL);
    }
    if (OSAL_SUCCESS != _VPAD_muxInitFifo(VPAD_SIP_W_FIFO,
            &_VPAD_muxPtr->inFifo.sipQ, sizeof(VPR_Comm))) {
        return (OSAL_FAIL);
    }
    if (OSAL_SUCCESS != _VPAD_muxInitFifo(VPAD_ISIP_W_FIFO,
            &_VPAD_muxPtr->inFifo.isipQ, sizeof(VPR_Comm))) {
        return (OSAL_FAIL);
    }
    if (OSAL_SUCCESS != _VPAD_muxInitFifo(VPAD_CSM_EVT_W_FIFO,
            &_VPAD_muxPtr->inFifo.csmEvtQ, sizeof(VPR_Comm))) {
        return (OSAL_FAIL);
    }

    return (OSAL_SUCCESS);
}

/*
 * ======== _VPAD_muxWriteDeviceTask ========
 *
 * Private function to write message to IPC device
 *
 * RETURN:
 *
 */
static OSAL_TaskReturn _VPAD_muxWriteDeviceTask(
    OSAL_TaskArg arg_ptr)
{
    VPR_Comm       *vpr_ptr = &(_VPAD_muxPtr->writeTaskBuf);
    vint            size;
    OSAL_SelectSet  fdSet;
    OSAL_Boolean    flag;
    uint32   cnt = 0;

    size = sizeof(VPR_Comm);

_VPAD_WRITE_TASK_LOOP:

    if (OSAL_FALSE == _VPAD_muxPtr->isVpmdReady) {
        /* vpmd have been lost and we need to wait for recovery */
        OSAL_taskDelay(VPAD_MUX_ERROR_RECOVERY_DELAY);
        goto _VPAD_WRITE_TASK_LOOP;
    }

    /* Add all fifo fd to fdSet */
    OSAL_selectSetInit(&fdSet);
    OSAL_selectAddId(&_VPAD_muxPtr->inFifo.videoCmdEvtQ.fid, &fdSet);
    OSAL_selectAddId(&_VPAD_muxPtr->inFifo.videoStreamQ.fid, &fdSet);
    OSAL_selectAddId(&_VPAD_muxPtr->inFifo.voiceStreamQ.fid, &fdSet);
    OSAL_selectAddId(&_VPAD_muxPtr->inFifo.isiRpcQ.fid, &fdSet);
    OSAL_selectAddId(&_VPAD_muxPtr->inFifo.isiEvtRpcQ.fid, &fdSet);
    OSAL_selectAddId(&_VPAD_muxPtr->inFifo.sipQ.fid, &fdSet);
    OSAL_selectAddId(&_VPAD_muxPtr->inFifo.isipQ.fid, &fdSet);
    OSAL_selectAddId(&_VPAD_muxPtr->inFifo.csmEvtQ.fid, &fdSet);

    if (OSAL_FAIL == OSAL_select(&fdSet, NULL, NULL, NULL)) {
        OSAL_logMsg("%s:%d Failed to seletec fd for writing to device.\n",
             __FUNCTION__, __LINE__);
        OSAL_taskDelay(VPAD_MUX_ERROR_RECOVERY_DELAY);
        goto _VPAD_WRITE_TASK_LOOP;
    }

    /* check which fifo is selected and read the message */
    if (OSAL_SUCCESS == OSAL_selectIsIdSet(
            &_VPAD_muxPtr->inFifo.videoCmdEvtQ.fid, &fdSet, &flag)) {
        if (OSAL_TRUE == flag) {
            if (OSAL_SUCCESS == OSAL_fileRead(
                    &_VPAD_muxPtr->inFifo.videoCmdEvtQ.fid,
                    vpr_ptr, &size)) {

                /* Swap endian if needed. */
                _VPAD_MUX_SWAP_VPR_COMM_ENDIAN(vpr_ptr, VPAD_DATA_TO_IPC);

                VPAD_ioWriteDevice((void *)vpr_ptr, &size);

                /* Update statistics if needed */
                _VPAD_MUX_STATS_UPDATE(&_VPAD_muxPtr->inFifo.videoCmdEvtQ, size);
                /* Update video counters */
                _VPAD_MUX_COLLECT_VIDEO_COUNTERS(vpr_ptr);
            }
        }
    }
    if (OSAL_SUCCESS == OSAL_selectIsIdSet(
            &_VPAD_muxPtr->inFifo.videoStreamQ.fid, &fdSet, &flag)) {
        if (OSAL_TRUE == flag) {
            if (OSAL_SUCCESS == OSAL_fileRead(
                    &_VPAD_muxPtr->inFifo.videoStreamQ.fid,
                    vpr_ptr, &size)) {
                /* Swap endian if needed. */
                _VPAD_MUX_SWAP_VPR_COMM_ENDIAN(vpr_ptr, VPAD_DATA_TO_IPC);

                VPAD_ioWriteDevice((void *)vpr_ptr, &size);

                /* Update statistics if needed */
                _VPAD_MUX_STATS_UPDATE(&_VPAD_muxPtr->inFifo.videoStreamQ, size);
            }
        }
    }
    if (OSAL_SUCCESS == OSAL_selectIsIdSet(
            &_VPAD_muxPtr->inFifo.voiceStreamQ.fid, &fdSet, &flag)) {
        if (OSAL_TRUE == flag) {
            if (OSAL_SUCCESS == OSAL_fileRead(
                    &_VPAD_muxPtr->inFifo.voiceStreamQ.fid,
                    vpr_ptr, &size)) {
                /* Swap endian if needed. */
                _VPAD_MUX_SWAP_VPR_COMM_ENDIAN(vpr_ptr, VPAD_DATA_TO_IPC);

                VPAD_ioWriteDevice((void *)vpr_ptr, &size);

                /* Update statistics if needed */
                _VPAD_MUX_STATS_UPDATE(&_VPAD_muxPtr->inFifo.voiceStreamQ, size);
            }
        }
    }
    if (OSAL_SUCCESS == OSAL_selectIsIdSet(
            &_VPAD_muxPtr->inFifo.isiRpcQ.fid, &fdSet, &flag)) {
        if (OSAL_TRUE == flag) {
            if (OSAL_SUCCESS == OSAL_fileRead(
                    &_VPAD_muxPtr->inFifo.isiRpcQ.fid,
                    vpr_ptr, &size)) {
                /* Swap endian if needed. */
                _VPAD_MUX_SWAP_VPR_COMM_ENDIAN(vpr_ptr, VPAD_DATA_TO_IPC);

                VPAD_ioWriteDevice((void *)vpr_ptr, &size);

                /* Update statistics if needed */
                _VPAD_MUX_STATS_UPDATE(&_VPAD_muxPtr->inFifo.isiRpcQ, size);
            }
        }
    }
    if (OSAL_SUCCESS == OSAL_selectIsIdSet(
            &_VPAD_muxPtr->inFifo.isiEvtRpcQ.fid, &fdSet, &flag)) {
        if (OSAL_TRUE == flag) {
            if (OSAL_SUCCESS == OSAL_fileRead(
                    &_VPAD_muxPtr->inFifo.isiEvtRpcQ.fid,
                    vpr_ptr, &size)) {
                /* Swap endian if needed. */
                _VPAD_MUX_SWAP_VPR_COMM_ENDIAN(vpr_ptr, VPAD_DATA_TO_IPC);

                VPAD_ioWriteDevice((void *)vpr_ptr, &size);

                /* Update statistics if needed */
                _VPAD_MUX_STATS_UPDATE(&_VPAD_muxPtr->inFifo.isiEvtRpcQ, size);
            }
        }
    }
    if (OSAL_SUCCESS == OSAL_selectIsIdSet(
            &_VPAD_muxPtr->inFifo.sipQ.fid, &fdSet, &flag)) {
        if (OSAL_TRUE == flag) {
            if (OSAL_SUCCESS == OSAL_fileRead(
                    &_VPAD_muxPtr->inFifo.sipQ.fid,
                    vpr_ptr, &size)) {
                /* Swap endian if needed. */
                _VPAD_MUX_SWAP_VPR_COMM_ENDIAN(vpr_ptr, VPAD_DATA_TO_IPC);

                VPAD_ioWriteDevice((void *)vpr_ptr, &size);

                /* Update statistics if needed */
                _VPAD_MUX_STATS_UPDATE(&_VPAD_muxPtr->inFifo.sipQ, size);
            }
        }
    }
    if (OSAL_SUCCESS == OSAL_selectIsIdSet(
            &_VPAD_muxPtr->inFifo.isipQ.fid, &fdSet, &flag)) {
        if (OSAL_TRUE == flag) {
            if (OSAL_SUCCESS == OSAL_fileRead(
                    &_VPAD_muxPtr->inFifo.isipQ.fid,
                    vpr_ptr, &size)) {
                /* Swap endian if needed. */
                _VPAD_MUX_SWAP_VPR_COMM_ENDIAN(vpr_ptr, VPAD_DATA_TO_IPC);

                VPAD_ioWriteDevice((void *)vpr_ptr, &size);

                /* Update statistics if needed */
                _VPAD_MUX_STATS_UPDATE(&_VPAD_muxPtr->inFifo.isipQ, size);
           }
        }
    }
    if (OSAL_SUCCESS == OSAL_selectIsIdSet(
            &_VPAD_muxPtr->inFifo.csmEvtQ.fid, &fdSet, &flag)) {
        if (OSAL_TRUE == flag) {
            if (OSAL_SUCCESS == OSAL_fileRead(
                    &_VPAD_muxPtr->inFifo.csmEvtQ.fid,
                    vpr_ptr, &size)) {
                /* Swap endian if needed. */
                _VPAD_MUX_SWAP_VPR_COMM_ENDIAN(vpr_ptr, VPAD_DATA_TO_IPC);

                VPAD_ioWriteDevice((void *)vpr_ptr, &size);

                /* Update statistics if needed */
                _VPAD_MUX_STATS_UPDATE(&_VPAD_muxPtr->inFifo.csmEvtQ, size);
            }
        }
    }

    /* Output statistics report if defined, every 16 times print once. */
    if (((cnt ++) & 0xf) == 0) {
        _VPAD_MUX_STATS_REPORT();
    }
    goto _VPAD_WRITE_TASK_LOOP;

    return (0);
}

/*
 * ======== _VPAD_muxInitWrite() ========
 *
 * This function is to create write FIFOs and write task.
 *
 * Return Values:
 *  OSAL_SUCCESS: Write FIFOs and task created successfully.
 *  OSAL_FAIL: Failed to create write FIFOs or task.
 */
OSAL_Status _VPAD_muxInitWrite()
{
    if (OSAL_SUCCESS != _VPAD_muxFifoInit()) {
        OSAL_logMsg("!!!FIFO create failed!!!\n");
        return (OSAL_FAIL);
    }

    /*
     * Create Task that will receive message from FIFO and
     * write to IO device.
     */
    if (0 == _VPAD_muxPtr->writeTaskId) {
        /* only create the task once */
        if (0 == (_VPAD_muxPtr->writeTaskId = OSAL_taskCreate(
                VPAD_WRITE_TASK_NAME,
                OSAL_TASK_PRIO_NRT,
                VPAD_TASK_STACK_BYTES,
                (void *)_VPAD_muxWriteDeviceTask,
                0))) {
            VPAD_destroy();
            OSAL_logMsg("%s:%d Create Task Failed.\n", __FUNCTION__, __LINE__);
            return (OSAL_FAIL);
        }
    }

    return (OSAL_SUCCESS);
}

/*
 * ======== _VPAD_writeVpadReady() ========
 *
 * This function is used to write ready event to VPMD.
 *
 * Return Values:
 *  OSAL_SUCCESS: success to write.
 *  OSAL_FAIL: fail to write.
 */
OSAL_Status _VPAD_writeVpadReady(
    OSAL_Boolean echo)
{
    vint size;

    VPR_Comm *vpr_ptr = &(_VPAD_muxPtr->vprCommBuffer);

    vpr_ptr->targetSize = 0;
    /* If it's echo the ready event, set the target to VPMD_VPAD_TARGET. */
    if (echo) {
        vpr_ptr->targetModule = VPMD_VPAD_TARGET;
    }
    else {
        vpr_ptr->targetModule = VPMD_VPMD_TARGET;
    }

    size = sizeof(VPR_Comm);

    /* Swap endian if needed. */
    _VPAD_MUX_SWAP_VPR_COMM_ENDIAN(vpr_ptr, VPAD_DATA_TO_IPC);

    if (OSAL_FAIL == VPAD_ioWriteDevice((void *)vpr_ptr, &size)) {
        return (OSAL_FAIL);
    }

    return (OSAL_SUCCESS);
}

/*
 *  ======== _VPAD_muxFifoDestroy() ========
 *  Private function to destroy read/write FIFOs.
 *
 *  Return Values:
 *  OSAL_SUCCESS: success to initialize FIFOs.
 *  OSAL_FAIL: fail to initialize FIFOs.
 */
void _VPAD_muxFifoDestroy()
{
    /* Delete read FIFOs .*/
    _VPAD_muxDestroyFifo(VPAD_VIDEO_CMDEVT_R_FIFO,
            &_VPAD_muxPtr->outFifo.videoCmdEvtQ);

    _VPAD_muxDestroyFifo(VPAD_VIDEO_STREAM_R_FIFO,
            &_VPAD_muxPtr->outFifo.videoStreamQ);

    _VPAD_muxDestroyFifo(VPAD_VOICE_STREAM_R_FIFO,
            &_VPAD_muxPtr->outFifo.voiceStreamQ);

    _VPAD_muxDestroyFifo(VPAD_ISI_RPC_R_FIFO,
            &_VPAD_muxPtr->outFifo.isiRpcQ);

    _VPAD_muxDestroyFifo(VPAD_ISI_EVT_RPC_R_FIFO,
            &_VPAD_muxPtr->outFifo.isiEvtRpcQ);

    _VPAD_muxDestroyFifo(VPAD_SIP_R_FIFO,
            &_VPAD_muxPtr->outFifo.sipQ);

    _VPAD_muxDestroyFifo(VPAD_ISIP_R_FIFO,
            &_VPAD_muxPtr->outFifo.isipQ);

    _VPAD_muxDestroyFifo(VPAD_CSM_EVT_R_FIFO,
            &_VPAD_muxPtr->outFifo.csmEvtQ);

    /* Destroy write fifo devices */
    _VPAD_muxDestroyFifo(VPAD_VIDEO_CMDEVT_W_FIFO,
            &_VPAD_muxPtr->inFifo.videoCmdEvtQ);

    _VPAD_muxDestroyFifo(VPAD_VIDEO_STREAM_W_FIFO,
            &_VPAD_muxPtr->inFifo.videoStreamQ);

    _VPAD_muxDestroyFifo(VPAD_VOICE_STREAM_W_FIFO,
            &_VPAD_muxPtr->inFifo.voiceStreamQ);

    _VPAD_muxDestroyFifo(VPAD_ISI_RPC_W_FIFO,
            &_VPAD_muxPtr->inFifo.isiRpcQ);

    _VPAD_muxDestroyFifo(VPAD_ISI_EVT_RPC_W_FIFO,
            &_VPAD_muxPtr->inFifo.isiEvtRpcQ);

    _VPAD_muxDestroyFifo(VPAD_SIP_W_FIFO,
            &_VPAD_muxPtr->inFifo.sipQ);

    _VPAD_muxDestroyFifo(VPAD_ISIP_W_FIFO,
            &_VPAD_muxPtr->inFifo.isipQ);

    _VPAD_muxDestroyFifo(VPAD_CSM_EVT_W_FIFO,
            &_VPAD_muxPtr->inFifo.csmEvtQ);

}

/*
 * ======== _VPAD_muxReadDeviceTask ========
 *
 * Private function to read message from IPC device
 *
 * RETURN:
 *
 */
static OSAL_TaskReturn _VPAD_muxReadDeviceTask(
    OSAL_TaskArg arg_ptr)
{
    VPR_Comm *vpr_ptr = &(_VPAD_muxPtr->readTaskBuf);
    vint      size;
    uint32    cnt = 0;

    size = sizeof(VPR_Comm);

_VPAD_READ_TASK_LOOP:
    OSAL_memSet(vpr_ptr, 0, sizeof(vpr_ptr));
    VPAD_muxDbgPrintf("[D2Log] _VPAD_muxReadDeviceTask, ioReadDevice\n");
    if (OSAL_SUCCESS != VPAD_ioReadDevice(vpr_ptr, size)) {
        if (_VPAD_muxPtr->isVpmdReady) {
            /* lost vpmd connection */
            _VPAD_muxPtr->isVpmdReady = OSAL_FALSE;
            _VPAD_muxFifoDestroy();
            OSAL_logMsg("%s: VPMD connection is lost.\n", __FUNCTION__);
        }
        OSAL_logMsg("%s: VPMD read fail, retry later.\n");
        OSAL_taskDelay(VPAD_MUX_ERROR_RECOVERY_DELAY);
        goto _VPAD_READ_TASK_LOOP;
    }
    VPAD_muxDbgPrintf("[D2Log] _VPAD_muxReadDeviceTask, ioReadDevice, SUCCESS, type = %d\n",
         vpr_ptr->targetModule);

    /* Swap endian if needed. */
    _VPAD_MUX_SWAP_VPR_COMM_ENDIAN(vpr_ptr, VPAD_DATA_FROM_IPC);

    switch (vpr_ptr->targetModule) {
        case VPMD_VIDEO_CMDEVT_TARGET:
            _VPAD_muxWriteDataToFifo(&_VPAD_muxPtr->outFifo.videoCmdEvtQ,
                    vpr_ptr, vpr_ptr->targetSize, OSAL_WAIT_FOREVER);
            OSAL_logMsg("%s: receive VPMD_VIDEO_CMDEVT_TARGET, type %d\n", __FUNCTION__, vpr_ptr->type);
            break;
        case VPMD_VIDEO_STREAM_TARGET:
            _VPAD_muxWriteDataToFifo(&_VPAD_muxPtr->outFifo.videoStreamQ,
                    vpr_ptr, vpr_ptr->targetSize, OSAL_WAIT_FOREVER);
            break;
        case VPMD_VOICE_STREAM_TARGET:
            _VPAD_muxWriteDataToFifo(&_VPAD_muxPtr->outFifo.voiceStreamQ,
                    vpr_ptr, vpr_ptr->targetSize, OSAL_WAIT_FOREVER);
            break;
        case VPMD_ISI_RPC_TARGET:
            _VPAD_muxWriteDataToFifo(&_VPAD_muxPtr->outFifo.isiRpcQ,
                    vpr_ptr, vpr_ptr->targetSize, OSAL_WAIT_FOREVER);
            break;
        case VPMD_ISI_EVT_RPC_TARGET:
            _VPAD_muxWriteDataToFifo(&_VPAD_muxPtr->outFifo.isiEvtRpcQ,
                    vpr_ptr, vpr_ptr->targetSize, OSAL_WAIT_FOREVER);
            break;
        case VPMD_SIP_TARGET:
            _VPAD_muxWriteDataToFifo(&_VPAD_muxPtr->outFifo.sipQ,
                    vpr_ptr, vpr_ptr->targetSize, OSAL_WAIT_FOREVER);
            break;
        case VPMD_ISIP_TARGET:
            _VPAD_muxWriteDataToFifo(&_VPAD_muxPtr->outFifo.isipQ,
                    vpr_ptr, vpr_ptr->targetSize, OSAL_WAIT_FOREVER);
            break;
        case VPMD_CSM_EVT_TARGET:
            _VPAD_muxWriteDataToFifo(&_VPAD_muxPtr->outFifo.csmEvtQ,
                    vpr_ptr, vpr_ptr->targetSize, OSAL_WAIT_FOREVER);
            break;
        case VPMD_VPAD_TARGET:
            /* Remote is up and ready. */
            if (!_VPAD_muxPtr->isVpmdReady) {
                OSAL_logMsg("VPMD is ready now.\n");
                /*
                 * VPMD is ready, let's create all the FIFOs for VPAD users
                 * to acceess and crate the write task.
                 */
                if (OSAL_SUCCESS != _VPAD_muxInitWrite()) {
                    OSAL_logMsg("!!!Write init failed!!!\n");
                }
                _VPAD_muxPtr->isVpmdReady = OSAL_TRUE;
            }
            else {
                /* VPMD is already ready. */
                OSAL_logMsg("VPMD is already ready.\n");
            }

            /* Send echo back to VPMD. */
            if (OSAL_SUCCESS != _VPAD_writeVpadReady(OSAL_TRUE)) {
                OSAL_logMsg("Failed to echo back VPMD ready event.\n");
            }
            break;
        case VPMD_VPMD_TARGET:
            /* It's an echo back event from VPMD. */
            if (!_VPAD_muxPtr->isVpmdReady) {
                OSAL_logMsg("VPMD is ready now with echo.\n");
                /*
                 * VPMD is ready, let's create all the FIFOs for VPAD users
                 * to acceess and crate the write task.
                 */
                if (OSAL_SUCCESS != _VPAD_muxInitWrite()) {
                    OSAL_logMsg("!!!Write init failed!!!\n");
                }
                _VPAD_muxPtr->isVpmdReady = OSAL_TRUE;
            }
            else {
                /* VPMD is already ready. */
                OSAL_logMsg("VPMD is already ready with echo.\n");
            }
            break;
        default:
            OSAL_logMsg("_VPAD_mux unknown target:%d\n", vpr_ptr->targetModule);
            OSAL_logMsg("_VPAD_mux unknown target:%d\n", vpr_ptr->targetModule);
            break;
    }
    /* Output statistics report if defined, every 16 times print once */
    if (((cnt ++) & 0xf) == 0) {
        _VPAD_MUX_STATS_REPORT();
    }

    goto _VPAD_READ_TASK_LOOP;

    return (0);
}

/*
 *  ======== VPAD_init() ========
 *  This function is used to initialize VPAD
 *
 *  Return Values:
 *  OSAL_SUCCESS: success to initialize VPAD.
 *  OSAL_FAIL: fail to initialize VPAD.
 */
OSAL_Status VPAD_init(
    void)
{
    /* Clear up fifo node just in case it's not been deleted. */
    _VPAD_muxFifoDestroy();

    /* Init IO */
    VPAD_ioInit();

    /*
     * Create Task that will receive message from device and
     * write to FIFO.
     */
    if (0 == (_VPAD_muxPtr->readTaskId = OSAL_taskCreate(
            VPAD_READ_TASK_NAME,
            OSAL_TASK_PRIO_VTSPR,
            VPAD_TASK_STACK_BYTES,
            (void *)_VPAD_muxReadDeviceTask,
            0))) {
        VPAD_destroy();
        OSAL_logMsg("%s:%d Create Task Failed.\n", __FUNCTION__, __LINE__);
        return (OSAL_FAIL);
    }

    /* Inform VPMD that VPAD is ready. */
    if (OSAL_SUCCESS != _VPAD_writeVpadReady(OSAL_FALSE)) {
        OSAL_logMsg("Failed to send VPAD ready event.\n");
        /* Don't care if it write failed. VPMD might not be ready yet. */
    }
    return (OSAL_SUCCESS);
}

/*
 *  ======== VPAD_destroy() ========
 *  This function is used to shutdown VPAD
 *
 *  Return Values:
 */
void VPAD_destroy(
    void)
{
    /* Destroy FIFOs. */
    _VPAD_muxFifoDestroy();

    OSAL_taskDelete(_VPAD_muxPtr->writeTaskId);
    OSAL_taskDelete(_VPAD_muxPtr->readTaskId);

    VPAD_ioDestroy();
}
