/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2007 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 30369 $ $Date: 2014-12-11 19:09:13 +0800 (Thu, 11 Dec 2014) $
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE 1
#endif


#include <stdio.h>
#include <stdlib.h>
#include <osal_types.h>
#include <osal.h>
#include <osal_msg.h>
#include <osal_log.h>
#include <csm_event.h>
#include "../../proxy.h"
#include "proxy_io.h"
#include "../../_proxy_ut.h"

#include "sipc_client.h"

#define PRXY_SPIPE_CHANNEL_ID    (10)
#define PRXY_IO_TASK_NAME        "prxy_io"
#define PRXY_IO_TASK_STACK_BYTES (4096)
#define PRXY_IO_MESSAGE_SIZE_MAX (512)
#define PRXY_MAX_QUEUE_DEPTH     (16)
#define PRXY_IO_MSGQ_DEPTH              (2)
#define PRXY_IO_ERROR_RECOVERY_DELAY    (200)
#define PRXY_IO_OPEN_PIPE_DELAY         (2000)

/* Global pointer to the PRXY queue  */
OSAL_MsgQId _PRXY_io_q = 0;

typedef struct {
    OSAL_TaskId         taskId;
    vint                spipeId;
    uint8               ioBuffer[PRXY_IO_MESSAGE_SIZE_MAX];
} PRXY_IoObj;

static PRXY_IoObj _PRXY_ioObj;
static PRXY_IoObj *_PRXY_ioObj_ptr = &_PRXY_ioObj;

/*
 *  ======== _PRXY_ioReadDataFromDevice() ========
 *  This function is used to read data from device.
 *
 *  Return Values:
 *      size of read data
 */
static vint _PRXY_ioReadDataFromDevice(
    uint8   channelId,     
    uint8  *buf_ptr,
    uint32  size)
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
            return (-1);
        }
        PRXY_dbgPrintf("got data %s chunk size:%d", buf_ptr, size);
        remainSize -= size;
        readSize += size;
    }
  
    return (readSize);
}

/*
 *  ======== _PRXY_ioReadLineFromDevice() ========
 *  This function is used to read data from device.
 *
 *  Return Values:
 *      size of the line
 */
static vint _PRXY_ioReadLineFromDevice(
    uint8   channelId,
    uint8  *buf_ptr,
    uint32  size)
{
    vint  readSize;
    vint  remainSize;
    vint  i, done;

    /* Read until new line or maximum data size is read or error */
    remainSize = size-1;
    readSize   = 0;
    done = 0;
    while ((remainSize > 0) && (!done)) {
        size = 1;
        if (0 > (size = spipe_read(channelId,
                (void *)((char *)buf_ptr),
                size,
                -1))) {
            OSAL_logMsg("%s:%d Read file FAIL.\n", __FUNCTION__, __LINE__);
            return (readSize);
        }
        buf_ptr[size] = 0;
        PRXY_dbgPrintf("got data %s chunk size:%d", buf_ptr, size);

        /* check new line */
        for (i=0; i<size; i++) {
            if ((*(char *)buf_ptr) == '\n') {
                done = 1;
                PRXY_dbgPrintf("got newline at %d", readSize+i);
                break;
            }
        }
        buf_ptr += size;
        remainSize -= size;
        readSize += size;
    }
    return (readSize);
}

/*
 * ======== _PRXY_ioDaemon() ========
 *
 * Main processing loop for vamd io
 */
static void _PRXY_ioDaemon(
    void *arg_ptr)
{
    int spipeId;

    while (0 == _PRXY_ioObj_ptr->spipeId) {
        if (0 == (spipeId = spipe_open(PRXY_SPIPE_CHANNEL_ID))) {
            PRXY_dbgPrintf("SPIPE Created Fail!!\n");
        } else {
            /* Notify PRXY_MUX IO is ready now. */
            _PRXY_ioObj_ptr->spipeId = spipeId;
            PRXY_dbgPrintf("SPIPE open OK pipe id:%d!!\n", spipeId);
        }
        OSAL_taskDelay(PRXY_IO_OPEN_PIPE_DELAY);
    }

_PRXY_IO_DAEMON_LOOP:
    /* Wait forever */
    if (0 < (_PRXY_ioReadLineFromDevice(
            PRXY_SPIPE_CHANNEL_ID,
            _PRXY_ioObj_ptr->ioBuffer,
            PRXY_IO_MESSAGE_SIZE_MAX))) {
        PRXY_dbgPrintf(" Get Data == > %s\n", _PRXY_ioObj_ptr->ioBuffer);
        /* write to q for gapp task */
        if (OSAL_SUCCESS != OSAL_msgQSend(_PRXY_io_q, _PRXY_ioObj_ptr->ioBuffer,
                PRXY_IO_MESSAGE_SIZE_MAX, OSAL_NO_WAIT, NULL)) {
            PRXY_dbgPrintf("Failed to write GAPP PRXY IO Q.\n");
        }

    } else {
        OSAL_logMsg("%s:%d Failed to read fd for PROXY IO messages\n",
                     __FUNCTION__, __LINE__);
        OSAL_taskDelay(PRXY_IO_ERROR_RECOVERY_DELAY);
    }
    goto _PRXY_IO_DAEMON_LOOP;

}


/*
 * ======== PRXY_init ========
 *
 * This public routine to initialize the PRXY sub module.  This function will
 * create MUX_IO channel and msgQ for sending msg to gapp. 
 *
 * RETURN:
 *     PRXY_Return
 */
OSAL_Status PRXY_ioInit(
    const char *proxyName_ptr)
{
    vint spipeId;
    /* Init MUX_IO */
    OSAL_memSet(_PRXY_ioObj_ptr, 0, sizeof(PRXY_IoObj));
    if (0 == (spipeId = spipe_open(PRXY_SPIPE_CHANNEL_ID))) {
        PRXY_dbgPrintf("SPIPE Created Fail!!\n");
        /* still fall through to start the io task */
    } else {
        /* Notify PRXY_MUX IO is ready now. */
        PRXY_dbgPrintf("SPIPE open OK pipe id:%d!!\n", spipeId);
        _PRXY_ioObj_ptr->spipeId = spipeId;
    }

    /* Start PRXY_IO task */
    if (0 == (_PRXY_ioObj_ptr->taskId = OSAL_taskCreate(
            PRXY_IO_TASK_NAME,
            OSAL_TASK_PRIO_NRT,
            PRXY_IO_TASK_STACK_BYTES,
            (OSAL_TaskPtr)_PRXY_ioDaemon,
            (void *)_PRXY_ioObj_ptr))) {
        PRXY_dbgPrintf("Failed creating %s daemon\n", PRXY_IO_TASK_NAME);
        return (OSAL_FAIL);
    }

    /* Create q for gapp_task */
    if (0 == (_PRXY_io_q = OSAL_msgQCreate(PRXY_IO_MSG_Q_NAME, 
            OSAL_MODULE_PRXY, OSAL_MODULE_GAPP, OSAL_DATA_STRUCT_String,
            PRXY_MAX_QUEUE_DEPTH, PRXY_IO_MESSAGE_SIZE_MAX, 0))) {
        return (OSAL_FAIL);
    }
    return (OSAL_SUCCESS);
}


/*
 * ======== PRXY_getEventQueue ========
 *
 * This public routine to get the private msgQ Fd descriptor
 *
 * RETURN:
 *     PRXY_RETURN_OK
 */
void PRXY_ioGetEventQueue(
    OSAL_MsgQId *queue_ptr)
{
    *queue_ptr = _PRXY_io_q;
}

/*
 * ======== PRXY_destroy() ========
 *
 * This function closes the proxy file descriptor.
 *
 * Return Values:
 * Nothing
 */
void PRXY_ioDestroy(
    void)
{
    PRXY_dbgPrintf("ioDestroy!!!\n");
    spipe_close(PRXY_SPIPE_CHANNEL_ID);
    if (0 != _PRXY_io_q) {
        OSAL_msgQDelete(_PRXY_io_q);
    }

    /* delete task */
    OSAL_taskDelete(_PRXY_ioObj_ptr->taskId);
}

/*
 *  ======== _PRXY_ioWriteDevice() ========
 *  This function is used to write data to the device.
 *
 *  Return Values:
 *  PRXY_RETURN_OK: success to read.
 *  PRXY_RETURN_FAILED: fail to read.
 */
static PRXY_Return _PRXY_ioWriteDevice(
    void *buf_ptr,
    vint bufSize)
{
    vint                writtenSize, size;
    vint                remainSize;
    int                 channelId = PRXY_SPIPE_CHANNEL_ID;

    /* write until required data size is writtens */
    remainSize = size = bufSize;
    writtenSize   = 0;
    while (remainSize > 0) {
        size = remainSize;
        if (0 > (size = spipe_write(channelId,
                (void *)((char *)buf_ptr + writtenSize),
                size,
                -1))) {
            OSAL_logMsg("%s:%d write buffer to device FAIL.\n", __FUNCTION__, __LINE__);
            return (PRXY_RETURN_FAILED);
        }
        PRXY_dbgPrintf("written data chunk size:%d data=%s", bufSize, buf_ptr);
        remainSize -= size;
        writtenSize += size;
    }

    return (PRXY_RETURN_OK);
}

/*
 * ======== PRXY_write() ===================
 * This function writes to the proxy slave interface.
 *
 * Returns:
 * Size of write, -1 for failure.
 */
    vint PRXY_ioWrite(
        char    *buf_ptr,
        vint     size)
{
    vint ret = PRXY_RETURN_FAILED;

    if ((0 != size) && (_PRXY_io_q >= 0)) {
        PRXY_printAT("D2AtLog_Output", buf_ptr);
        ret = _PRXY_ioWriteDevice(buf_ptr, size);
    }
    return (ret);
}

