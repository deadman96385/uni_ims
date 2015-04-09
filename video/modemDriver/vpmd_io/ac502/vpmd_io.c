/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2014 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Revision: 29915 $ $Date: 2014-11-19 11:05:56 +0800 (Wed, 19 Nov 2014) $
 */

#include <osal.h>
#include "vpad_vpmd.h"
#include "vpmd_io.h"
#include "mux_api.h"
#include "mux_all.h"

#include "sipc_client.h"

#define VPMD_SPIPE_CHANNEL_ID           (9)
#define VPMD_IO_TASK_NAME               "vpmd_io"
#define VPMD_IO_TASK_STACK_BYTES        (4096)
#define VPMD_IO_MSGQ_DEPTH              (2)
#define VPMD_IO_ERROR_RECOVERY_DELAY    (200)
#define VPMD_IO_OPEN_PIPE_DELAY         (2000)

typedef struct {
    OSAL_TaskId         taskId;
    VPMD_IoDataHandler  readHandler;
    VPMD_IoReadyHandler readyHandler;
    vint                spipeId;
    vint                remainSize;
    vint                readSize;
    uint8               ioBuffer[VPMD_IO_MESSAGE_SIZE_MAX];
} VPMD_IoObj;

static VPMD_IoObj *_VPMD_ioObj_ptr;

/*
 *  ======== _VPMD_ioReadDataFromDevice() ========
 *  This function is used to read data from device.
 *
 *  Return Values:
 *  OSAL_SUCCESS: success to read.
 *  OSAL_FAIL: fail to read.
 */
static OSAL_Status _VPMD_ioReadDataFromDevice(
    vint channelId,
    void *buf_ptr,
    vint  size)
{
    vint                readSize;
    vint                remainSize;

    /* Read until required data size is read */
    remainSize = size;
    readSize   = 0;
    while (remainSize > 0) {
        size = remainSize;
        if (0 > (size = spipe_read(channelId,
                (void *)((char *)buf_ptr + readSize),
                size,
                -1))) {
            OSAL_logMsg("%s:%d Read file FAIL.\n", __FUNCTION__, __LINE__);
            return (OSAL_FAIL);
        }
        VPMD_ioDbgPrintf("got data chunk size:%d", size);
        remainSize -= size;
        readSize += size;
    }

    return (OSAL_SUCCESS);
}

/*
 * ======== _VPMD_ioDaemon() ========
 *
 * Main processing loop for vamd io
 */
int32 _VPMD_ioDaemon(
    void *arg_ptr)
{
    int readSize;
    int spipeId;

    while (0 == _VPMD_ioObj_ptr->spipeId) {
        if (0 == (spipeId=spipe_open(VPMD_SPIPE_CHANNEL_ID))) {
            VPMD_ioDbgPrintf("SPIPE Created Fail!!\n");
        } else {
            /* Notify VPMD_MUX IO is ready now. */
            _VPMD_ioObj_ptr->spipeId = spipeId;
            _VPMD_ioObj_ptr->readyHandler(OSAL_TRUE);
            VPMD_ioDbgPrintf("SPIPE open OK pipe id:%d!!\n", spipeId);
        }
        OSAL_taskDelay(VPMD_IO_OPEN_PIPE_DELAY);
    }

_VPMD_IO_DAEMON_LOOP:
    /* Wait forever */

    if (0 < (readSize= _VPMD_ioReadDataFromDevice(
            VPMD_SPIPE_CHANNEL_ID,
            _VPMD_ioObj_ptr->ioBuffer,
            VPMD_IO_MESSAGE_SIZE_MAX))) {
        _VPMD_ioObj_ptr->readHandler(
                _VPMD_ioObj_ptr->ioBuffer,
                VPMD_IO_MESSAGE_SIZE_MAX);
    } else {
        OSAL_logMsg("%s:%d Failed to read fd for VPMD messages\n",
                     __FUNCTION__, __LINE__);
        OSAL_taskDelay(VPMD_IO_ERROR_RECOVERY_DELAY);
    }
    goto _VPMD_IO_DAEMON_LOOP;

    /*
     * Clean up and signal
     */
    VPMD_ioDbgPrintf("finished");
    return 0;
}

/*
 *  ======== _VPMD_ioRegDeviceDataReadyHandler() ========
 *  This function is used to read data from device.
 *
 *  Return Values:
 *  OSAL_SUCCESS: success to read.
 *  OSAL_FAIL: fail to read.
 */
OSAL_Status _VPMD_ioRegDeviceDataReadyHandler(
        VPMD_IoDataHandler handler)
{
    _VPMD_ioObj_ptr->readHandler = handler;
    VPMD_ioDbgPrintf("mux callback function Register.\n");
    return (OSAL_SUCCESS);
}

/*
 *  ======== VPMD_ioInit() ========
 *  This function is used to init the io device and start the reading task
 *
 *  Return Values:
 *  OSAL_SUCCESS: success to read.
 *  OSAL_FAIL: fail to read.
 */
OSAL_Status VPMD_ioInit(
    VPMD_IoReadyHandler ioReadyHandler,
    VPMD_IoDataHandler  ioReadHandler)
{
    vint spipeId;

    /* checking null callback functions */
    if ((ioReadyHandler == NULL) || (ioReadHandler == NULL)) {
        VPMD_ioDbgPrintf("ioReadyHandler or ioReadHandler is Null!!\n");
        return (OSAL_FAIL);
    }

    _VPMD_ioObj_ptr = OSAL_memCalloc(1, sizeof(VPMD_IoObj), 0);
    _VPMD_ioObj_ptr->remainSize = VPMD_IO_MESSAGE_SIZE_MAX;
    _VPMD_ioObj_ptr->readSize = 0;

    _VPMD_ioObj_ptr->readyHandler = (VPMD_IoReadyHandler)ioReadyHandler;
    _VPMD_ioRegDeviceDataReadyHandler((VPMD_IoDataHandler)ioReadHandler);    

    /* Start VPMD_IO task */
    if (0 == (_VPMD_ioObj_ptr->taskId = OSAL_taskCreate(
            VPMD_IO_TASK_NAME,
            OSAL_TASK_PRIO_NRT,
            VPMD_IO_TASK_STACK_BYTES,
            _VPMD_ioDaemon,
            (void *)_VPMD_ioObj_ptr))) {
        VPMD_ioDbgPrintf("Failed creating %s daemon\n", VPMD_IO_TASK_NAME);
        return (OSAL_FAIL);
    }

    return (OSAL_SUCCESS);
}

/*
 *  ======== VPMD_ioDestroy() ========
 *  This function is used to close the io device.
 *
 *  Return Values:
 *  OSAL_SUCCESS: success to read.
 *  OSAL_FAIL: fail to read.
 */
void VPMD_ioDestroy(
        void)
{
    VPMD_ioDbgPrintf("ioDestroy!!!\n");
    spipe_close(VPMD_SPIPE_CHANNEL_ID);
    _VPMD_ioRegDeviceDataReadyHandler(NULL);
}

/*
 *  ======== VPMD_ioWriteDevice() ========
 *  This function is used to write data to the device.
 *
 *  Return Values:
 *  OSAL_SUCCESS: success to read.
 *  OSAL_FAIL: fail to read.
 */
OSAL_Status VPMD_ioWriteDevice(
    void *buf_ptr,
    vint *size_ptr)
{
    vint                writtenSize, size;
    vint                remainSize;
    int                 channelId = VPMD_SPIPE_CHANNEL_ID;


    /* write until required data size is writtens */
    remainSize = size = *size_ptr;
    writtenSize   = 0;
    while (remainSize > 0) {
        size = remainSize;
        if (0 > (size = spipe_write(channelId,
                (void *)((char *)buf_ptr + writtenSize),
                size,
                -1))) {
            OSAL_logMsg("%s:%d write buffer to device FAIL.\n", __FUNCTION__, __LINE__);
            *size_ptr = writtenSize;
            return (OSAL_FAIL);
        }
        VPMD_ioDbgPrintf("written data chunk size:%d", *size_ptr);
        remainSize -= size;
        writtenSize += size;
    }
    *size_ptr = writtenSize;
    return (OSAL_SUCCESS);
}

