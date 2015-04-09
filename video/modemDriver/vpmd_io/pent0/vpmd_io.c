/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2013 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Revision$ $Date$
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE 1
#endif

#include <features.h>
#include <termios.h>

#include <osal.h>
#include "vpad_vpmd.h"
#include "vpmd_io.h"


#define VPMD_IO_DEVICE                  OSAL_IPC_FOLDER"vpxd"

#define VPMD_IO_TASK_NAME               "vpmd_io"
#define VPMD_IO_TASK_STACK_BYTES        (4096)
#define VPMD_IO_MSGQ_DEPTH              (2)

#define VPMD_IO_ERROR_RECOVERY_DELAY    (1000)

typedef struct {
    OSAL_TaskId         taskId;
    MODEM_Terminal      ioPty;
    VPMD_IoDataHandler  readHandler;
    VPMD_IoReadyHandler readyHandler;
    uint8               ioBuffer[VPMD_IO_MESSAGE_SIZE_MAX];
} VPMD_IoObj;

static VPMD_IoObj _VPMD_ioObj;
static VPMD_IoObj *_VPMD_ioObj_ptr = &_VPMD_ioObj;

/*
 *  ======== _VPMD_ioReadDataFromDevice() ========
 *  This function is used to read data from device.
 *
 *  Return Values:
 *  OSAL_SUCCESS: success to read.
 *  OSAL_FAIL: fail to read.
 */
OSAL_Status _VPMD_ioReadDataFromDevice(
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
        VPMD_ioDbgPrintf("%s:%d got data chunk size:%d", __FUNCTION__, __LINE__,
                size);
        remainSize -= size;
        readSize += size;
    }

    return (OSAL_SUCCESS);
}

/*
 * ======== _VPMD_ioTerminalSetup ========
 *
 * This private helper routine is used to setup the D2 character device
 * for communicating with VPMD.
 *
 * RETURN:
 * OSAL_SUCCESS: Setup successfully
 * OSAL_FAIL: Failed to setup
 */
static OSAL_Status _VPMD_ioTerminalSetup(
    int   masterFd,
    char *masterName_ptr,
    int   size)
{
    struct termios  options;

    if (tcgetattr(masterFd, &options) == -1) {
        OSAL_logMsg("%s %d Error getting tty attributes %s's master - "
                "%s(%d).\n", __FUNCTION__, __LINE__, masterName_ptr,
                strerror(errno), errno);
    }

    OSAL_logMsg("%s %d Master input baud rate is %d\n", __FUNCTION__,
            __LINE__, (int) cfgetispeed(&options));
    OSAL_logMsg("%s %d Master output baud rate is %d\n", __FUNCTION__,
            __LINE__, (int) cfgetospeed(&options));

    cfmakeraw(&options);
    /* Max c_cc is 255 */
    options.c_cc[VMIN] = (size > 255)? 255: size;
    options.c_cc[VTIME] = 0;

    // The baud rate, word length, and handshake options can be set as follows:
    cfsetispeed(&options, B230400);    // Set to highest rate, 230400 baud
    cfsetospeed(&options, B230400);    // Set to highest reate 230400 baud
    options.c_cflag |= (CS8);  // RTS flow control of input

    OSAL_logMsg("%s %d Input baud rate changed to %d\n", __FUNCTION__,
            __LINE__, (int) cfgetispeed(&options));
    OSAL_logMsg("%s %d Output baud rate changed to %d\n", __FUNCTION__,
            __LINE__, (int) cfgetospeed(&options));


    // Cause the new options to take effect immediately.
    if (tcsetattr(masterFd, TCSANOW, &options) == -1) {
        OSAL_logMsg("%s %d Error setting tty attributes %s's master - "
                "%s(%d).\n", __FUNCTION__, __LINE__, masterName_ptr,
                strerror(errno), errno);
        return (OSAL_FAIL);
    }
    return (OSAL_SUCCESS);
}

/*
 * ======== _VPMD_ioInitMaster ========
 *
 * Private function to initialize master char device.
 *
 * RETURN:
 * OSAL_SUCCESS: Initialized successfully
 * OSAL_FAIL: Initialize failed.
 */
static OSAL_Status _VPMD_ioInitMaster(
    const char     *devicePath_ptr,
    MODEM_Terminal *terminal_ptr,
    int             size)
{
    char* pt_name;
    int   pt_master;
    int   symlinkError;
    char  masterName[64];

    /* Create a new character device for RILD to attached to us */
    pt_master = open("/dev/ptmx",O_RDWR);

    pt_name = ptsname(pt_master);
    OSAL_logMsg("%s %d Slave end name is '%s'\n", __FUNCTION__, __LINE__,
            pt_name);

    grantpt(pt_master);
    unlockpt(pt_master);

    OSAL_logMsg("%s %d Softlinking %s to %s.\n", __FUNCTION__, __LINE__,
            pt_name, devicePath_ptr);

    symlinkError = symlink(pt_name, devicePath_ptr);
    if (symlinkError < 0) {
        OSAL_logMsg("%s %d Could not create symlink from %s to %s: %s\n",
                __FUNCTION__, __LINE__, pt_name, devicePath_ptr,
                strerror(symlinkError));
        return (OSAL_FAIL);
    }

    symlinkError = chmod(devicePath_ptr, S_IRWXU | S_IRWXG | S_IRWXO);
    if (symlinkError < 0) {
        OSAL_logMsg("%s %d Could not chmod 777 %s : %s\n",
                __FUNCTION__, __LINE__, pt_name, strerror(symlinkError));
        return (OSAL_FAIL);
    }

    OSAL_snprintf(masterName, sizeof(masterName), "%s%s", "Master-",
            devicePath_ptr);
    if (OSAL_SUCCESS != _VPMD_ioTerminalSetup(pt_master, masterName, size)) {
        return (OSAL_FAIL);
    }

    terminal_ptr->fid = pt_master;
    terminal_ptr->sz = size;
    OSAL_strcpy(terminal_ptr->name, masterName);
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
_VPMD_IO_DAEMON_LOOP:
    /* Wait forever */
    if (0 == _VPMD_ioObj_ptr->ioPty.fid) {
        if (OSAL_SUCCESS != OSAL_fileOpen(&_VPMD_ioObj_ptr->ioPty.fid,
                VPMD_IO_DEVICE, OSAL_FILE_O_RDWR, 0)) {
            _VPMD_ioObj_ptr->ioPty.fid = 0;
            OSAL_logMsg("%s:%d Open %s device FAIL.\n", __FUNCTION__, __LINE__,
                    VPMD_IO_DEVICE);
            OSAL_taskDelay(VPMD_IO_ERROR_RECOVERY_DELAY);
            goto _VPMD_IO_DAEMON_LOOP;
        }
        else {
            /* Notify VPMD_MUX IO is ready now. */
            _VPMD_ioObj_ptr->readyHandler(OSAL_TRUE);
        }
    }
    if (OSAL_SUCCESS == _VPMD_ioReadDataFromDevice(
            &_VPMD_ioObj_ptr->ioPty.fid,
            _VPMD_ioObj_ptr->ioBuffer,
            VPMD_IO_MESSAGE_SIZE_MAX,
            OSAL_WAIT_FOREVER)) {
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
    VPMD_ioDbgPrintf("%s:%d finished", __FUNCTION__, __LINE__);
    return 0;
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
    OSAL_Status ret;

    /* XXX Keep or remove this? */
    char cmdBuffer[64];

    /* initialize the global object */
    OSAL_memSet(_VPMD_ioObj_ptr, 0, sizeof(_VPMD_ioObj));

    /* Register callback function */
    _VPMD_ioObj_ptr->readyHandler = ioReadyHandler;
    _VPMD_ioObj_ptr->readHandler  = ioReadHandler;

    /* Let clean house.  This is needed for running on an actual device. */
    OSAL_snprintf(cmdBuffer, sizeof(cmdBuffer), "rm %s", VPMD_IO_DEVICE);
    system(cmdBuffer);

    /* initialize simulated modem device */
    ret = _VPMD_ioInitMaster(VPMD_IO_DEVICE,
            &_VPMD_ioObj_ptr->ioPty,
            VPMD_IO_MESSAGE_SIZE_MAX);
    if (OSAL_SUCCESS != ret) {
        VPMD_ioDestroy();
        return (ret);
    }
    /* Notify VPMD_MUX IO is ready now. */
    _VPMD_ioObj_ptr->readyHandler(OSAL_TRUE);

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
void VPMD_ioDestroy()
{
    if (0 != _VPMD_ioObj_ptr->ioPty.fid) {
        OSAL_fileClose(&_VPMD_ioObj_ptr->ioPty.fid);
        OSAL_fileDelete(VPMD_IO_DEVICE);
    }
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
    OSAL_SelectSet      fdSet;

    /* If a fd doesn't exist then create it. */
    if (0 == _VPMD_ioObj_ptr->ioPty.fid) {
        if (OSAL_SUCCESS != OSAL_fileOpen(&_VPMD_ioObj_ptr->ioPty.fid,
                VPMD_IO_DEVICE, OSAL_FILE_O_RDWR, 0)) {
            _VPMD_ioObj_ptr->ioPty.fid = 0;
            OSAL_logMsg("%s:%d Open %s device FAIL.\n", __FUNCTION__, __LINE__,
                    VPMD_IO_DEVICE);
            OSAL_taskDelay(VPMD_IO_ERROR_RECOVERY_DELAY);
            return (OSAL_FAIL);
        }
        else {
            /* Notify VPMD_MUX IO is ready now. */
            _VPMD_ioObj_ptr->readyHandler(OSAL_TRUE);
        }
    }

    /* Select Fd and write file. wait forever*/
    OSAL_selectSetInit(&fdSet);
    OSAL_selectAddId(&_VPMD_ioObj_ptr->ioPty.fid, &fdSet);

    if (OSAL_FAIL == OSAL_select(NULL, &fdSet, NULL, NULL)) {
        OSAL_logMsg("%s:%d File select FAIL.\n", __FUNCTION__, __LINE__);
        return (OSAL_FAIL);
    }

    /* Now write */
    if (OSAL_FAIL == OSAL_fileWrite(&_VPMD_ioObj_ptr->ioPty.fid, buf_ptr,
            size_ptr)) {
        OSAL_logMsg("%s:%d Sending to %s FAIL.\n", __FUNCTION__, __LINE__,
                VPMD_IO_DEVICE);
        return (OSAL_FAIL);
    }
    return (OSAL_SUCCESS);
}

