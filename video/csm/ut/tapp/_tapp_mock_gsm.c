/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2006 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 21141 $ $Date: 2013-06-20 17:07:08 +0800 (Thu, 20 Jun 2013) $
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE 1
#endif

#include <features.h>
#include <termios.h>

#include <osal.h>
#include <isi.h>
#include <isip.h>
#include <csm_event.h>

#include "_tapp.h"
#include "_tapp_mock_gsm.h"

/*
 * ======== TAPP_mockGsmValidateAt() ========
 *
 * This function is used to filter ignore AT command in GSM device.
 * EX: AT+CLCC is sent by a periodic _gsm timer. 
 * Thus, we can ignore it, if would not to verify it from AT interface.
 * 
 * Return Values:
 * TAPP_PASS: If action at is not AT+CLCC and getting event at is not CLCC, 
 *            return PASS.
 *            If action at is not AT+CLCC and getting event at is CLCC, 
 *            continue to find next event in GSM device that is not CLCC.
 *            If action at is AT+CLCC and getting event is CLCC, return PASS.
 * TAPP_FAIL: Cannot get any event.
 */
TAPP_Return _TAPP_mockGsmAtFilter(
    TAPP_GlobalObj  *global_ptr,
    TAPP_Action     *action_ptr,
    TAPP_Event      *event_ptr)
{
    while (TAPP_PASS == TAPP_getInputEvent(global_ptr, action_ptr, 
            event_ptr, action_ptr->u.timeout)) {
        if (NULL != OSAL_strscan(event_ptr->msg.at, TAPP_AT_CMD_CLCC)) {
            if (NULL != OSAL_strscan(action_ptr->msg.at, TAPP_AT_CMD_CLCC)) {
                /* The event is the one we want to validate, don't filter it */
                return (TAPP_PASS);
            }
            else {
                /* filter this event */
                continue;
            }
        }
        else {
            /* It's not the event we want to filter */
            return (TAPP_PASS);
        }
    }
    return (TAPP_FAIL);
}

/*
 * ======== TAPP_mockGsmInit() ========
 *
 * This function is used to initialize and configure Mock GSM interface.
 * This function will create a GSM device
 * 
 * Return Values:
 * TAPP_PASS: initialize successfully.
 * TAPP_FAIL: initialize fialed.
 */
TAPP_Return TAPP_mockGsmInit(
    TAPP_GlobalObj *global_ptr)
{
    char* pt_name;
    int   pt_masterMITM;
    int   symlinkError;
    struct termios  options;

    /* Create a new character device for RILD to attached to us */
    pt_masterMITM = open("/dev/ptmx",O_RDWR);

    pt_name = ptsname(pt_masterMITM);
    TAPP_dbgPrintf("%s %d Slave end name is '%s'\n", __FUNCTION__, __LINE__, 
            pt_name);
    grantpt(pt_masterMITM);
    unlockpt(pt_masterMITM);

    TAPP_dbgPrintf("%s %d Softlinking %s to %s.\n",
            __FUNCTION__, __LINE__, pt_name, global_ptr->gsmDevName);

    symlinkError = symlink(pt_name, global_ptr->gsmDevName);
    if (symlinkError < 0) {
        OSAL_logMsg("%s %d Could not create symlink from %s to %s: %s\n",
                __FUNCTION__, __LINE__, pt_name, global_ptr->gsmDevName, 
                strerror(symlinkError));
        return (TAPP_FAIL);
    }

    symlinkError = chmod(global_ptr->gsmDevName, S_IRWXU | S_IRWXG | S_IRWXO);
    if (symlinkError < 0) {
        OSAL_logMsg("%s %d Could not chmod 777 %s : %s\n",
                __FUNCTION__, __LINE__, pt_name, strerror(symlinkError));
        return (TAPP_FAIL);
    }

    /* setup caracter device */
    if (tcgetattr(pt_masterMITM, &options) == -1) {
        OSAL_logMsg("%s %d Error getting tty attributes master - "
                "%s(%d).\n", __FUNCTION__, __LINE__, strerror(errno), errno);
        return (TAPP_FAIL);
    }

    TAPP_dbgPrintf("%s %d Master input baud rate is %d\n", __FUNCTION__,
            __LINE__, (int) cfgetispeed(&options));
    TAPP_dbgPrintf("%s %d Master output baud rate is %d\n", __FUNCTION__,
            __LINE__, (int) cfgetospeed(&options));

    cfmakeraw(&options);
    options.c_cc[VMIN] = 1;
    options.c_cc[VTIME] = 10;

    // The baud rate, word length, and handshake options can be set as follows:
    cfsetispeed(&options, B19200);    // Set 19200 baud
    cfsetospeed(&options, B19200);    // Set 19200 baud
    options.c_cflag |= (CS8);  // RTS flow control of input

    TAPP_dbgPrintf("%s %d Input baud rate changed to %d\n", __FUNCTION__, 
            __LINE__, (int) cfgetispeed(&options));
    TAPP_dbgPrintf("%s %d Output baud rate changed to %d\n", __FUNCTION__, 
            __LINE__, (int) cfgetospeed(&options));

    // Cause the new options to take effect immediately.
    if (tcsetattr(pt_masterMITM, TCSANOW, &options) == -1) {
        OSAL_logMsg("%s %d Error setting tty attributes master - "
                "%s(%d).\n", __FUNCTION__, __LINE__, strerror(errno), errno);
        return (TAPP_FAIL);
    }

    /* Set GSM Fd */
    global_ptr->fd.gsm = pt_masterMITM;

    return (TAPP_PASS);
}

/*
 * ======== TAPP_mockGsmShutdown() ========
 *
 * This function is used to close Mock GSM device.
 * 
 * Return Values:
 * nothing.
 */
void TAPP_mockGsmShutdown(
    TAPP_GlobalObj *global_ptr)
{
    /* close gsm device */
    if (0 != global_ptr->fd.gsm) {
        OSAL_fileClose(&global_ptr->fd.gsm);
        global_ptr->fd.gsm = 0;
        unlink(global_ptr->gsmDevName);
    }
}

/*
 * ======== TAPP_mockGsmIssueAt() ========
 *
 * This function is used to issue an AT response.
 * 
 * Return Values:
 * TAPP_PASS: write AT result to GSM device sucessfully.
 * TAPP_FAIL: write AT result to GSM device fail.
 */
TAPP_Return TAPP_mockGsmIssueAt(
    TAPP_GlobalObj *global_ptr,
    char           *at_ptr)
{
    char  buff[TAPP_AT_COMMAND_STRING_SZ];
    vint  sendSize;

    if (NULL == global_ptr) {
        return (TAPP_FAIL);
    }

    if (NULL == at_ptr) {
        return (TAPP_FAIL);
    }

    /* write AT response to GSM device node. */
    sendSize = OSAL_snprintf(buff, TAPP_AT_COMMAND_STRING_SZ, "%s", at_ptr);
    if (OSAL_FAIL == OSAL_fileWrite(&global_ptr->fd.gsm, buff, &sendSize)) {
        return (TAPP_FAIL);
    }

    return (TAPP_PASS);
}

/*
 * ======== TAPP_mockGsmValidateAt() ========
 *
 * This function is used to validate AT command.
 * 
 * Return Values:
 * TAPP_PASS: the result is same the expected value.
 * TAPP_FAIL: the result is different with the expected value.
 */
TAPP_Return TAPP_mockGsmValidateAt(
    TAPP_GlobalObj *global_ptr,
    TAPP_Action    *action_ptr)
{
    TAPP_Event *event_ptr;

    if ((NULL == global_ptr) || (NULL == action_ptr)) {
        return (TAPP_FAIL);
    }

    event_ptr = &global_ptr->tappEvt;

    /* get input event and ignore some AT command */
    if (TAPP_PASS != _TAPP_mockGsmAtFilter(global_ptr, action_ptr, 
            event_ptr)) {
        return (TAPP_FAIL);
    }

    if (TAPP_EVENT_TYPE_GSM_DEV == event_ptr->type) {
        if (0 == OSAL_strcmp(event_ptr->msg.at, action_ptr->msg.at)) {
            return (TAPP_PASS);
        }
    }
    return (TAPP_FAIL);
}

