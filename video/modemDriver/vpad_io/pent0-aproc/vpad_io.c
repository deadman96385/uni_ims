/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2013 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Revision$ $Date$
 */

#include <osal.h>
#include "vpad_io.h"

#define VPAD_IO_DEVICE           OSAL_IPC_FOLDER"vpxd"

/* This struct is used to get the fid inside a msgQ */
typedef struct {
    char    name[128];
    int     fid;
    uint32  size;
} VPAD_IoPty;

typedef struct {
    OSAL_TaskId         taskId;
    VPAD_IoPty          ioPty;
} VPAD_IoObj;

static VPAD_IoObj   _VPAD_ioObj;
static VPAD_IoObj  *_VPAD_ioObj_ptr = &_VPAD_ioObj;

/*
 *  ======== _VPAD_ioReadDataFromDevice() ========
 *  This function is used to read data from device.
 *
 *  Return Values:
 *  OSAL_SUCCESS: success to read.
 *  OSAL_FAIL: fail to read.
 */
OSAL_Status _VPAD_ioReadDataFromDevice(
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
        if (0 == size) {
            OSAL_logMsg("%s:%d Read size==0.\n", __FUNCTION__, __LINE__);
            /* for pts style io, we should restart new fd */
            OSAL_fileClose(fid_ptr);
            *fid_ptr = 0;
            return (OSAL_FAIL);
        }
        VPAD_ioDbgPrintf("%s:%d got data chunk size:%d", __FUNCTION__, __LINE__,
                size);
        remainSize -= size;
        readSize += size;
    }

    return (OSAL_SUCCESS);
}

/*
 * ======== _VPAD_ioPrepareDevice ========
 *
 * This private helper routine is used to open/config the ttyUSB device
 * for read/write to jiwu2 soc.
 *
 * RETURN:
 * OSAL_SUCCESS: Setup successfully
 * OSAL_FAIL: Failed to setup
 */
static OSAL_Status _VPAD_ioPrepareDevice(
    void)
{
    /* If a fd doesn't exist then create it. */
    if (0 == _VPAD_ioObj_ptr->ioPty.fid) {
        VPAD_ioDbgPrintf("_VPAD_ioPrepareDevice to open %s\n", VPAD_IO_DEVICE);
        if (OSAL_SUCCESS != OSAL_fileOpen(&_VPAD_ioObj_ptr->ioPty.fid,
                VPAD_IO_DEVICE, OSAL_FILE_O_RDWR, 0)) {
            _VPAD_ioObj_ptr->ioPty.fid = 0;
            OSAL_logMsg("%s:%d Open %s device FAIL. Error %s(%d).\n",
                    __FUNCTION__, __LINE__,
                    VPAD_IO_DEVICE,
                    strerror(errno), errno);
            return (OSAL_FAIL);
        }
        _VPAD_ioObj_ptr->ioPty.size = VPAD_IO_MESSAGE_SIZE_MAX;
        OSAL_strcpy(_VPAD_ioObj_ptr->ioPty.name, VPAD_IO_DEVICE);
    }
    return (OSAL_SUCCESS);
}

/*
 *  ======== VPAD_ioInit() ========
 *  This function is used to init the io device and start the reading task
 *
 *  Return Values:
 *  OSAL_SUCCESS: success.
 *  OSAL_FAIL: fail.
 */
OSAL_Status VPAD_ioInit(
    void)
{
    VPAD_ioDbgPrintf("VPAD_ioInit\n");

    OSAL_memSet(_VPAD_ioObj_ptr, 0, sizeof(_VPAD_ioObj));

    _VPAD_ioPrepareDevice();

    return (OSAL_SUCCESS);
}

/*
 *  ======== VPAD_ioDestroy() ========
 *  This function is used to close the io device.
 *
 *  Return Values:
 *
 */
void VPAD_ioDestroy(
    void)
{
    if (0 != _VPAD_ioObj_ptr->ioPty.fid) {
        OSAL_fileClose(&_VPAD_ioObj_ptr->ioPty.fid);
    }
}

/*
 *  ======== VPAD_ioReadDevice() ========
 *  This function is used to read data from the device.
 *
 *  Return Values:
 *  OSAL_SUCCESS: success to read.
 *  OSAL_FAIL: fail to read.
 */
OSAL_Status VPAD_ioReadDevice(
    void *buf_ptr,
    vint  size)
{
    /* If a fd doesn't exist then create it. */
    if (0 == _VPAD_ioObj_ptr->ioPty.fid) {
        if (OSAL_SUCCESS != OSAL_fileOpen(&_VPAD_ioObj_ptr->ioPty.fid,
                VPAD_IO_DEVICE, OSAL_FILE_O_RDWR, 0)) {
            _VPAD_ioObj_ptr->ioPty.fid = 0;
            OSAL_logMsg("%s:%d Open %s device FAIL.\n", __FUNCTION__, __LINE__,
                    VPAD_IO_DEVICE);
            return (OSAL_FAIL);
        }
    }

    /* Now Read */
    if (OSAL_SUCCESS != _VPAD_ioReadDataFromDevice(&_VPAD_ioObj_ptr->ioPty.fid,
            buf_ptr, size, OSAL_WAIT_FOREVER)) {
        return (OSAL_FAIL);
    }

    return (OSAL_SUCCESS);
}

/*
 *  ======== VPAD_ioWriteDevice() ========
 *  This function is used to write data to the device.
 *
 *  Return Values:
 *  OSAL_SUCCESS: success to write.
 *  OSAL_FAIL: fail to write.
 */
OSAL_Status VPAD_ioWriteDevice(
    void *buf_ptr,
    vint *size_ptr)
{
    /* If a fd doesn't exist then create it. */
    _VPAD_ioPrepareDevice();

    /* Now write */
    if (OSAL_FAIL == OSAL_fileWrite(&_VPAD_ioObj_ptr->ioPty.fid, buf_ptr,
            size_ptr)) {
        OSAL_logMsg("%s:%d Sending to %s FAIL.\n", __FUNCTION__, __LINE__,
                VPAD_IO_DEVICE);
        return (OSAL_FAIL);
    }

    return (OSAL_SUCCESS);
}


