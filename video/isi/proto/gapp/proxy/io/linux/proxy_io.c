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

#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <strings.h>
#include <errno.h>
#include <termios.h>
#include <pthread.h>
#include <string.h>

#include <osal_types.h>
#include <osal.h>
#include <osal_msg.h>
#include <osal_log.h>

#include <csm_event.h>

#include "gsm.h"
#include "proxy_io.h"
#include "../../proxy.h"
#include "../../_proxy_ut.h"
//#include "../../_cmd_mngr.h"

/* Global pointer to the PRXY queue */
static _PRXY_QParams _PRXY_q;

/*
 * ======== _PRXY_setup ========
 *
 * This private helper routine is used to setup the D2 character device 
 * for filtering AT commands from RILD 
 *
 * RETURN:
 */
static void _PRXY_ioSetup(
    int   proxyFd, 
    char *masterName_ptr)
{
    struct termios  options;

    if (tcgetattr(proxyFd, &options) == -1) {
        PRXY_dbgPrintf("%s %d Error getting tty attributes %s's master - "
                "%s(%d).\n", __FUNCTION__, __LINE__, masterName_ptr, 
                strerror(errno), errno);
    }

    PRXY_dbgPrintf("%s %d Master input baud rate is %d\n", __FUNCTION__,
            __LINE__, (int) cfgetispeed(&options));
    PRXY_dbgPrintf("%s %d Master output baud rate is %d\n", __FUNCTION__,
            __LINE__, (int) cfgetospeed(&options));

    cfmakeraw(&options);
    options.c_cc[VMIN] = 1;
    options.c_cc[VTIME] = 10;

    // The baud rate, word length, and handshake options can be set as follows:
    cfsetispeed(&options, B19200);    // Set 19200 baud
    cfsetospeed(&options, B19200);    // Set 19200 baud
    options.c_cflag |= (CS8);  // RTS flow control of input

    PRXY_dbgPrintf("%s %d Input baud rate changed to %d\n", __FUNCTION__, 
            __LINE__, (int) cfgetispeed(&options));
    PRXY_dbgPrintf("%s %d Output baud rate changed to %d\n", __FUNCTION__, 
            __LINE__, (int) cfgetospeed(&options));

    // Cause the new options to take effect immediately.
    if (tcsetattr(proxyFd, TCSANOW, &options) == -1) {
        PRXY_dbgPrintf("%s %d Error setting tty attributes %s's master - "
                "%s(%d).\n", __FUNCTION__, __LINE__, masterName_ptr, 
                strerror(errno), errno);
    }
    }

#ifndef GAPP_DISABLE_AT_DEVICE 
/*
 * ======== _PRXY_KillRil ========
 *
 * This private helper routine is used to kill the RILD process
 *
 * RETURN:
 *     
 */
static void _PRXY_ioKillRil(
    void)
{
    vint x;
    vint bytes;
    int  fd;
    char buffer[64+1];
    /* Loop through all the processes and kill RILD */
    for (x = PRXY_RILD_SEARCH_START ; x < PRXY_RILD_SEARCH_END ; x++) {
        OSAL_snprintf(buffer, 64, PRXY_RILD_SEARCH_DIR, x);
        fd = open(buffer, O_RDONLY);
        if (0 < fd) {
            bytes = read(fd, buffer, 64);
            close(fd);
            if (bytes > 10) {
                buffer[bytes] = 0;
                if (0 == OSAL_strncmp(PRXY_RILD_PROCESS, buffer,
                        sizeof(PRXY_RILD_PROCESS) - 1)) {
                    /* Found it! now kill it. */
                    PRXY_dbgPrintf("%s %d kill process #: %d", __FUNCTION__, 
                            __LINE__, x);
                    OSAL_snprintf(buffer, 64, PRXY_RILD_KIL_CMD, x);
                    system(buffer);
                    return;
                }
            }
        }
    }
    PRXY_dbgPrintf("%s %d COULD NOT FIND AND KILL RILD!!!!", __FUNCTION__, 
            __LINE__);
}
#endif

/*
 * ======== PRXY_init ========
 *
 * This public routine to initialize the PRXY sub module.  This function will
 * create a fake /dev device for RILD to attach to.  It will also move the 
 * GSM_AT device to a new name.  It also restarts RILD process.
 *
 * RETURN:
 *     PRXY_Return
 */
OSAL_Status PRXY_ioInit(
    const char *proxyName_ptr)
{
    char* pt_name;
    int pt_masterMITM;
    int symlinkError;
#ifndef GAPP_DISABLE_AT_DEVICE 
    static char devicePath[256];

    /*
     * First move the original interface before making the proxy. 
     * Rename the smd0 to something. 
     */
    OSAL_snprintf(devicePath, 256, "%s%s", proxyName_ptr, "Org");
    if (0 != rename(proxyName_ptr, devicePath)) {
        PRXY_dbgPrintf("%s %d Could not move the original AT interface from:%s\n"
               " to %s", __FUNCTION__, __LINE__, proxyName_ptr, devicePath);
    }
    else {
        PRXY_dbgPrintf("%s %d %s is now %s\n", __FUNCTION__, __LINE__, 
                proxyName_ptr, devicePath);
    }

    /* Kill (which will restart) the original RILD. */
    _PRXY_ioKillRil();
#endif
    /* Create a new character device for RILD to attached to us */
    pt_masterMITM = open("/dev/ptmx",O_RDWR);

    pt_name = ptsname(pt_masterMITM);
    PRXY_dbgPrintf("%s %d Slave end name is '%s'\n", __FUNCTION__, __LINE__, 
            pt_name);
    grantpt(pt_masterMITM);
    unlockpt(pt_masterMITM);

    PRXY_dbgPrintf("%s %d Softlinking %s to %s.\n",
            __FUNCTION__, __LINE__, pt_name, proxyName_ptr);

    symlinkError = symlink(pt_name, proxyName_ptr);
    if (symlinkError < 0) {
        PRXY_dbgPrintf("%s %d Could not create symlink from %s to %s: %s\n",
                __FUNCTION__, __LINE__, pt_name, proxyName_ptr, 
                strerror(symlinkError));
    }

    symlinkError = chmod(proxyName_ptr, S_IRWXU | S_IRWXG | S_IRWXO);
    if (symlinkError < 0) {
        PRXY_dbgPrintf("%s %d Could not chmod 777 %s : %s\n",
                __FUNCTION__, __LINE__, pt_name, strerror(symlinkError));
    }

    //Important to make the pseudo-terminal raw.
    _PRXY_ioSetup(pt_masterMITM, "D2-GSM master");

    _PRXY_q.fid = pt_masterMITM;
    _PRXY_q.sz = 512;
    OSAL_strcpy(_PRXY_q.name, proxyName_ptr);

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
    *queue_ptr = (OSAL_MsgQId)&_PRXY_q;
}

/*
 * ======== PRXY_ioDestroy() ========
 *
 * This function closes the proxy file descriptor.
 *
 * Return Values:
 * Nothing
 */
void PRXY_ioDestroy(
    void)
{
    if (0 != _PRXY_q.fid) {
        close(_PRXY_q.fid);
        _PRXY_q.fid = 0;
        unlink(_PRXY_q.name);
        _PRXY_q.name[0] = 0;
    }
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
    vint num = -1;

    if ((0 != size) && (_PRXY_q.fid >= 0)) {
        PRXY_printAT("D2AtLog_Output", buf_ptr);
        num = write(_PRXY_q.fid, buf_ptr, size);
    }
    return (num);
}

