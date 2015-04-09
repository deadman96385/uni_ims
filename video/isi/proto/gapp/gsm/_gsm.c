/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2007 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 28044 $ $Date: 2014-08-11 15:49:43 +0800 (Mon, 11 Aug 2014) $
 */

#include "_gsm_cfg.h"
#include "gsm.h"
#include "_gsm.h"

const char GSM_XML_TIMEOUT_ATTR[]   = "timeout";
const char GSM_XML_DELAY_ATTR[]     = "delay";
const char GSM_XML_AT_CMD_TAG[]     = "at-cmds";
const char GSM_XML_AT_TAG[]         = "at";
const char GSM_XML_DEVICE_TAG[]     = "device";
const char GSM_XML_SERIAL_TAG[]     = "serial";
const char GSM_XML_BAUD_TAG[]       = "baud";

/* Response codes both numeric and alpha pending ATV0 or ATV1 */
//const char GSM_CMD_OK[]             = "OK\r";
const char GSM_CMD_OK[]             = "0\r";
//const char GSM_CMD_CONNECT[]      = "CONNECT\r";
const char GSM_CMD_CONNECT[]        = "1\r";
//const char GSM_CMD_RING[]         = "RING\r";
const char GSM_CMD_RING[]           = "2\r";
//const char GSM_NO_CARRIER[]         = "NO CARRIER";
const char GSM_NO_CARRIER[]         = "3\r";
//const char GSM_CMD_ERROR[]          = "ERROR\r";
const char GSM_CMD_ERROR[]          = "4\r";
//const char GSM_CMD_NO_DIALTONE[]  = "NO DIALTONE\r";
const char GSM_CMD_NO_DIALTONE[]    = "5\r";
//const char GSM_CMD_BUSY[]         = "BUSY\r";
const char GSM_CMD_BUSY[]           = "6\r";
//const char GSM_CMD_NO_ANSWER[]    = "NO ANSWER\r";
const char GSM_CMD_NO_ANSWER[]      = "7\r";
const char GSM_CMD_CME_ERROR[]      = "+CME ERROR";
const char GSM_CMD_CMS_ERROR[]      = "+CMS ERROR";
//const char GSM_CMD_PROMT[]          = "\r\n> ";
const char GSM_CMD_PROMT[]          = ">";
const char GSM_CMD_CALL_REPORT[]    = "AT+CLCC";
const char GSM_CMD_CALL_STATUS[]    = "+CLCC";
const char GSM_CMD_DIAL[]           = "ATD";
const char GSM_CMD_DIAL_URI[]       = "AT+CDU";
const char GSM_CMD_DIGIT_PAUSE[]    = "AT+VTS=P";
const char GSM_CALL_RING2[]         = "+CRING: VOICE";
const char GSM_CALL_WAITING[]       = "+CCWA";
const char GSM_CALL_REPORT[]        = "CALL REPORT\r\n";

const char GSM_CMD_SRVCC[]          = "+CIREPH:0";
const char GSM_SMS[]                = "+CMT:";
const char GSM_CRLN[]               = "\r\n";
const char GSM_CR[]                 = "\r";

const char GSM_CMD_CALL_EXT_REPORT[] = "AT+CLCCS";
const char GSM_CMD_CALL_EXT_STATUS[] = "+CLCCS";
const char GSM_CMD_CALL_MONITOR[]    = "+CMCCSI";

/* 
 * ======== GSM_getUniqueId() ========
 *
 * This function generates a unique value used as an identifier 
 * to associate commands with there respective results.
 *
 * Return Values: 
 * GSM_RETURN_OK: A unique ID number was generated.  
 * GSM_RETURN_FAILED: No ID was generated.  There was an error with
 *                    taking a mutex.
 */   
GSM_Return GSM_getUniqueId(
    GSM_IdMngr *mngr_ptr, 
    GSM_Id     *id_ptr)
{
    /* Take a mutex */
    if (OSAL_FAIL == (OSAL_semAcquire(mngr_ptr->mutex, OSAL_WAIT_FOREVER))) {
        return (GSM_RETURN_FAILED);
    }

    *id_ptr = mngr_ptr->id++;

    /* Handle any rollover */
    if (0 == mngr_ptr->id) {
        mngr_ptr->id++;
    }
    
    OSAL_semGive(mngr_ptr->mutex);
    return (GSM_RETURN_OK);
}

/* 
 * ======== _GSM_checkResult() ========
 *
 * This function inspects a NULL terminated string (result_ptr) to see if it
 * matches the target string or either any toher generic codes suchas "OK" "ERROR", etc.
 * The function returns an appropriate return code.
 *
 * Return Values: 
 * GSM_RETURN_OK: The string was a result containing the target string or another generic result code.
 * GSM_RETURN_ERR: The string was a result containing an "ERROR".
 * GSM_RETURN_FAILED: The string was something else. This means that the string in result is
 * really an unsolicited event.
 */
static GSM_Return _GSM_checkResult(
    const char *result_ptr,
    const char *target_ptr)
{
    const char *pos_ptr;
    const char *delimiter_ptr;
    vint count = 10;
    pos_ptr = result_ptr;
    /* Loop but never more than several times. */

    GSM_dbgPrintf("%s %d comparing :%s to:%s\n", __FUNCTION__, __LINE__,
            result_ptr, target_ptr);

    while (0 != count--) {
        delimiter_ptr = OSAL_strscan(pos_ptr, GSM_CRLN);
        if (NULL != delimiter_ptr) {
            if (NULL != target_ptr && 0 != *target_ptr) {
                /* Then see if the result matches the target. */
                if (0 == OSAL_strncmp(target_ptr, pos_ptr,
                        OSAL_strlen(target_ptr))) {
                    /* We have a match. */
                    return (GSM_RETURN_OK);
                }
            }
            /* Otherwise let's look for a generic error or success. */
            if (0 == OSAL_strncmp(pos_ptr, GSM_CMD_OK,
                    sizeof(GSM_CMD_OK) - 1)) {
                return (GSM_RETURN_OK);
            }
            else if (0 == OSAL_strncmp(pos_ptr, GSM_CMD_CME_ERROR,
                    sizeof(GSM_CMD_CME_ERROR) - 1)) {
                return (GSM_RETURN_ERR);
            }
            else if (0 == OSAL_strncmp(pos_ptr, GSM_CMD_CMS_ERROR,
                    sizeof(GSM_CMD_CMS_ERROR) - 1)) {
                return (GSM_RETURN_ERR);
            }
            else if (0 == OSAL_strncmp(pos_ptr, GSM_CMD_ERROR,
                    sizeof(GSM_CMD_ERROR) - 1)) {
                return (GSM_RETURN_ERR);
            }
            else if (0 == OSAL_strncmp(pos_ptr, GSM_CMD_PROMT,
                    sizeof(GSM_CMD_PROMT) - 1)) {
                return (GSM_RETURN_OK);
            }
            pos_ptr = delimiter_ptr + 2;
        }
        else {
            delimiter_ptr = OSAL_strscan(pos_ptr, GSM_CR);
            if (NULL != delimiter_ptr) {
                /* Then we have the end. */
                if (NULL != target_ptr  && 0 != *target_ptr) {
                    /* Then see if the result matches the target. */
                    if (0 == OSAL_strncmp(target_ptr, pos_ptr,
                            OSAL_strlen(target_ptr))) {
                        /* We have a match. */
                        return (GSM_RETURN_OK);
                    }
                }
                /* Otherwise let's look for a generic error or success. */
                if (0 == OSAL_strncmp(pos_ptr, GSM_CMD_OK,
                        sizeof(GSM_CMD_OK) - 1)) {
                    return (GSM_RETURN_OK);
                }
                else if (0 == OSAL_strncmp(pos_ptr, GSM_CMD_CME_ERROR,
                        sizeof(GSM_CMD_CME_ERROR) - 1)) {
                    return (GSM_RETURN_ERR);
                }
                else if (0 == OSAL_strncmp(pos_ptr, GSM_CMD_CMS_ERROR,
                        sizeof(GSM_CMD_CMS_ERROR) - 1)) {
                    return (GSM_RETURN_ERR);
                }
                else if (0 == OSAL_strncmp(pos_ptr, GSM_CMD_ERROR,
                        sizeof(GSM_CMD_ERROR) - 1)) {
                    return (GSM_RETURN_ERR);
                }
                else if (0 == OSAL_strncmp(pos_ptr, GSM_CMD_PROMT,
                        sizeof(GSM_CMD_PROMT) - 1)) {
                    return (GSM_RETURN_OK);
                }
            }
            return (GSM_RETURN_FAILED);
        }
    }
    return (GSM_RETURN_FAILED);
}

/* 
 * ======== _GSM_openDev() ========
 *
 * This function opens a device driver, more specifically the GSM
 * interface driver.  
 *
 * Return Values: 
 * The File descriptor to the device driver or -1 if 
 * function failed. 
 */
static GSM_Fd _GSM_openDev(
    char *device_ptr,
    vint  baud)
{
    GSM_Fd        fd;
    struct termio term;

    /*
     * Open tty device
     */
    if ((fd = open(device_ptr, O_RDWR | O_NDELAY)) >= 0) {
   
        /* Set tty state */
        ioctl(fd, TCGETA, &term);
        term.c_cflag |= CLOCAL | HUPCL;
        term.c_cflag &= ~CBAUD;
        term.c_cflag |= baud;
        term.c_lflag &= ~(ICANON | ECHO); /* to force raw mode */
        term.c_iflag &= ~ICRNL; /* to avoid non-needed blank lines */
        term.c_cc[VMIN] = 0;
        term.c_cc[VTIME] = 10; /* inter-digit timeout of 1 sec */
        ioctl(fd, TCSETA, &term);
        fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) & (~O_NDELAY)); /* No delay */
    }
    return (fd);
}

/* 
 * ======== _GSM_closeFd() ========
 *
 * This function closes a file descriptor.
 *
 * Return Values: 
 * Nothing
 */
static void _GSM_closeFd(
    GSM_Fd fd)
{
    if (fd >= 0) {
        close(fd);
    }
}

/* 
 * ======== GSM_writeFd() ===================
 * This function writes to a file descriptor..
 *
 * Returns:
 * Size of write, -1 for failure.
 */  
static vint _GSM_writeFd(
    GSM_Fd   fd,
    char    *buf_ptr,
    vint     size)
{
    vint num = -1;

    if (fd >= 0) {
        num = write(fd, buf_ptr, size);
    }
    return (num);
}

/* 
 * ======== GSM_readFd() ===================
 * This function reads a file descriptor.
 *
 * Returns:
 * Size of read, -1 for failure.
 */  
static vint _GSM_readFd(
    GSM_Fd  fd,
    char   *buf_ptr,
    vint    size,
    vint    timeoutSecs)
{
    OSAL_SelectSet      fdset;
    OSAL_SelectTimeval  tval;
    OSAL_SelectTimeval *tval_ptr;
    OSAL_Boolean        isTimedOut;
    vint num = -1;

    if (fd >= 0) {
        /* Set up the timeval object */
        if (-1 == timeoutSecs) {
            /* Then the timeout is forever */
            tval_ptr = NULL;
        }
        else {
            tval.sec = timeoutSecs;
            /* Turn millisecs to microsecs */
            tval.usec = 0;
            tval_ptr = &tval;
        }
        OSAL_memSet(&fdset, 0, sizeof(OSAL_SelectSet));
        OSAL_selectAddId(&fd, &fdset);
        if (OSAL_SUCCESS == OSAL_select(&fdset, NULL, tval_ptr,
                &isTimedOut)) {
            /* 
             * OSAL will return 'OSAL_SUCCESS' on timeout so check if 
             * it's a real timeout 
             */
            if (isTimedOut != OSAL_TRUE) {
                /* Then it's NOT a timeout */
                num = read(fd, buf_ptr, size);
            }
        }
    }
    return (num);
}

/* 
 * ======== GSM_writeAtCommand() ========
 *
 * Send an AT command to the GSM serial interface.
 *
 * Returns: 
 *  GSM_RETURN_OK     : Success
 *  GSM_RETURN_FAILED : Failed
 */
static GSM_Return _GSM_writeAtCommand(
    int         fd,
    const char *at_ptr,
    char       *buff_ptr,
    vint        buffSize)
{
    vint  bytes;
    vint  sendSize;
    
    sendSize = OSAL_snprintf(buff_ptr, buffSize, "%s\r", at_ptr);
    bytes = _GSM_writeFd(fd, buff_ptr, sendSize);
    if (bytes != sendSize) {
        return (GSM_RETURN_FAILED);
    }
    return (GSM_RETURN_OK);
}

/* 
 * ======== _GSM_getXmlTagText() ========
 *
 * This function retrieves the text of the XML tag specified 
 * in tag_ptr.
 *
 * Returns: 
 *  A pointer to a string containing the text to the tag or
 *  NULL if the tag could not be found or if there is no text.
 *  
 */
static char* _GSM_getXmlTagText(
    ezxml_t     xml_ptr, 
    const char *tag_ptr)
{
    ezxml_t   child_ptr;
    if (NULL == (child_ptr = ezxml_child(xml_ptr, tag_ptr))) {
        return (NULL);
    }
    return ezxml_txt(child_ptr);
}

/* 
 * ======== _GSM_getXmlTagText() ========
 *
 * This function retrieves the text of the XML tag nested underneath 
 * a parent tag.
 *
 * Returns: 
 *  A pointer to a string containing the text to the nested tag or
 *  NULL if the tag could not be found or if there is no text.
 *  
 */
static char* _GSM_getXmlNestedTagText(
    ezxml_t     xml_ptr, 
    const char *parentTag_ptr, 
    const char *childTag_ptr)
{
    ezxml_t   child_ptr;
    /* Get the parent tag.  If its exists then search for the child tag */
    if (NULL == (child_ptr = ezxml_child(xml_ptr, parentTag_ptr))) {
        return (NULL);
    }
    /* Get the child */
    if (NULL == (child_ptr = ezxml_child(child_ptr, childTag_ptr))) {
        return (NULL);
    }
    return ezxml_txt(child_ptr);
}

/* 
 * ======== _GSM_getXmlAtCmd() ========
 *
 * This function retrieves an AT command specified in an XML file.
 * Commands are specified in xml files using the following convention.
 *
 * <at timeout=50>ATCMD</at>
 *
 * Return Values: 
 * GSM_RETURN_OK: A valid AT command was retrieved.
 * GSM_RETURN_FAILED: An AT command was not found. 
 */
static GSM_Return _GSM_getXmlAtCmd(
    ezxml_t    xml_ptr, 
    char     **cmd_ptr, 
    vint      *timeout_ptr,
    vint      *delay_ptr)
{
    char const *attr_ptr;
    char       *c_ptr;

    /* Get the command timeout value */
    if (NULL == (attr_ptr = ezxml_attr(xml_ptr, GSM_XML_TIMEOUT_ATTR))) {
        /* No timeout value set to zero 0 */
        *timeout_ptr = 0;
    }
    else {
        *timeout_ptr = OSAL_atoi(attr_ptr);
    }

    /* Get the command delay value */
    if (NULL == (attr_ptr = ezxml_attr(xml_ptr, GSM_XML_DELAY_ATTR))) {
        /* No timeout value set to zero 0 */
        *delay_ptr = 0;
    }
    else {
        *delay_ptr = OSAL_atoi(attr_ptr);
    }
    
    /* Get the command */
    if (NULL == (c_ptr = ezxml_txt(xml_ptr))) {
        return (GSM_RETURN_FAILED);
    }
    *cmd_ptr = c_ptr;
    return (GSM_RETURN_OK);
}

/* 
 * ======== _GSM_loadAtCommands() ========
 *
 * This function will parse an xml file and retrieve AT commands specified 
 * in the file. For each AT command specified the function will execute the
 * AT command.
 *
 * AT commands are specified using the following convention.
 * <at timeout=50>ATCMD</at>
 *
 * Return Values: 
 * GSM_RETURN_OK: Function was successful.  All specified AT commands
 *                successfully completed.
 * GSM_RETURN_FAILED: One of the AT commands in the xml file failed to execute.
 */
static GSM_Return _GSM_loadAtCommands(
    GSM_Fd  devFd, 
    ezxml_t xml_ptr,
    char   *buff_ptr,
    vint    buffSize)
{
    /* Get the "AT" tag */
    ezxml_t     child_ptr;
    char       *cmd_ptr;
    vint        timeoutSecs;
    vint        delaySecs;
    vint        bytes;
    
    /* Get the parent tag.  If its exists then search for the child tag */
    if (NULL == (child_ptr = ezxml_child(xml_ptr, GSM_XML_AT_CMD_TAG))) {
        /* Then there are no commands, this is allowed so return OK */
        return (GSM_RETURN_OK);
    }
    /* Loop and get each AT command and issue */
    child_ptr = ezxml_child(child_ptr, GSM_XML_AT_TAG);
    while (NULL != child_ptr) {
        if (GSM_RETURN_OK == _GSM_getXmlAtCmd(child_ptr,
                &cmd_ptr, &timeoutSecs, &delaySecs)) {
            
            GSM_dbgPrintf("%s %d AT CMD: %s timeout:%d delay:%d\n",
                    __FUNCTION__, __LINE__, cmd_ptr, timeoutSecs, delaySecs);

            /* Check if we should wait befoire issuing commands. */
            if (delaySecs != 0) {
                OSAL_taskDelay(delaySecs * 1000);
            }
            
            if (GSM_RETURN_OK != _GSM_writeAtCommand(devFd, cmd_ptr, buff_ptr,
                    buffSize)) {
                /* There was an error */
                return (GSM_RETURN_FAILED);
            }
            /* Read the response */
            if (0 >= (bytes = _GSM_readFd(devFd, buff_ptr, buffSize,
                    timeoutSecs))) {
                /* Then this command has failed, return failure */
                GSM_dbgPrintf("%s %d AT CMD failed\n",
                    __FUNCTION__, __LINE__);
                return (GSM_RETURN_FAILED);
            }
            /* NULL terminate the result code that was read */
            buff_ptr[bytes] = 0;
            if (GSM_RETURN_ERR == _GSM_checkResult(buff_ptr, NULL)) {
                /* 
                 * Then the GSM interface say the command failed.  
                 * Return an error 
                 */
                GSM_dbgPrintf("%s %d AT CMD got failed result\n",
                    __FUNCTION__, __LINE__);
                return (GSM_RETURN_FAILED);
            }
        }
        else {
            GSM_dbgPrintf("%s %d BAD AT CMD..skipping\n",
                    __FUNCTION__, __LINE__);
        }
        child_ptr = ezxml_next(child_ptr);
    }
    return (GSM_RETURN_OK);
}

/* 
 * ======== _GSM_writeEvent() ========
 *
 * This function writes a GSM event to the OSAL queue used communicate
 * with the GSM application.  Note that this function is a blocking function.
 *
 * Return Values: 
 * GSM_RETURN_OK: Function was successful. The event was successfully written 
 *                to the queue.
 * GSM_RETURN_FAILED: There was an error writing to the OSAL queue.
 */
static GSM_Return _GSM_writeEvent(
    GSM_Obj     *gsm_ptr,
    const char  *arg_ptr,
    vint         id)
{
    OSAL_MsgQId      ipcId;
    
    /* Construct the event object */
    gsm_ptr->event.id = id;
    OSAL_snprintf(gsm_ptr->event.arg, GSM_EVENT_ARG_LEN, "%s", arg_ptr);
    
    if (0 == (ipcId = OSAL_msgQCreate(GSM_EVENT_MSG_Q_NAME, 
            OSAL_MODULE_GAPP_GSM, OSAL_MODULE_GAPP_GSM,
            OSAL_DATA_STRUCT_GSM_Event,
            GSM_EVT_Q_LEN, sizeof(GSM_Event), 0))) {
        return (GSM_RETURN_FAILED);
    }
    
    if (OSAL_SUCCESS != OSAL_msgQSend(ipcId, (char *)&gsm_ptr->event,
            sizeof(GSM_Event), OSAL_WAIT_FOREVER, NULL)) { 
        OSAL_msgQDelete(ipcId);
        return (GSM_RETURN_FAILED);
    }
    OSAL_msgQDelete(ipcId);
    
    GSM_dbgPrintf("%s %d: Wrote event  id:%d arg:%s\n",
            __FUNCTION__, __LINE__, (int)gsm_ptr->event.id, gsm_ptr->event.arg);
    
    return (GSM_RETURN_OK);
}

static int32 _GSM_reportTimerExpiry(
    void *arg_ptr)
{
    GSM_Obj *gsm_ptr;
    GSM_Cmd *cmd_ptr;

    gsm_ptr = (GSM_Obj *)arg_ptr;
    cmd_ptr = &gsm_ptr->report.command;
    
    /* Build up the tmr command event and send to cmd queue */
    cmd_ptr->type = GSM_CMD_TYPE_TMR;
    cmd_ptr->u.tmr.type = GSM_TMR_TYPE_REPORT;
    cmd_ptr->u.tmr.arg = 0;
    GSM_writeApiCommand(gsm_ptr, cmd_ptr);
    return (1);
}

/* 
 * ======== _GSM_executeApiCommand() ========
 *
 * This function is called when the an AT command is received from 
 * the GSM API.  It will execute the AT command within a task created
 * by this module for handling AT commands.
 * 
 * The success or failure of this AT command is written to the event
 * queue used to communicate with the application.
 *
 * Return Values: 
 * Nothing
 */
static void _GSM_executeApiCommand(
    GSM_Obj *gsm_ptr,
    GSM_Cmd *cmd_ptr)
{
    vint       bytes;
    GSM_Return ret;
    char      *result_ptr;

    result_ptr = gsm_ptr->readScratchPad.result;

    /* 
     * Check if this command is "AT+VTS=P", if so we won't send the command to 
     * the GSM interface instead we will "pause" for one second
     */
    if (0 == OSAL_strncmp(cmd_ptr->u.cmd.cmd, GSM_CMD_DIGIT_PAUSE,
            sizeof(GSM_CMD_DIGIT_PAUSE) - 1)) {
        /* Wait a second and return 'ok' */
        OSAL_taskDelay(1000);
        _GSM_writeEvent(gsm_ptr, GSM_CMD_OK, cmd_ptr->u.cmd.id);
        return;
    }
    
    /* Write the command to the GSM device */
    if (GSM_RETURN_OK != _GSM_writeAtCommand(gsm_ptr->devFd, 
            cmd_ptr->u.cmd.cmd, gsm_ptr->writeScratchPad, GSM_BUFFER_SZ)) {
        /* Could write the command, return failure */
        _GSM_writeEvent(gsm_ptr, GSM_CMD_ERROR, cmd_ptr->u.cmd.id);
        GSM_dbgPrintf("%s %d Failed to write command!!!\n", __FUNCTION__, __LINE__);
        return;
    }
    
    /* 
     * If it's a dial command, then we are starting a new call so we kick the 
     * call report timer is it's not already running.
     */
    if ((0 == OSAL_strncmp(cmd_ptr->u.cmd.cmd, GSM_CMD_DIAL,
            sizeof(GSM_CMD_DIAL) - 1)) ||
            (0 == OSAL_strncmp(cmd_ptr->u.cmd.cmd, GSM_CMD_DIAL_URI,
            sizeof(GSM_CMD_DIAL_URI) - 1))) {
        if (OSAL_FALSE == gsm_ptr->report.timerOn) {
            if (OSAL_SUCCESS == OSAL_tmrPeriodicStart(gsm_ptr->report.tmrId,
                    _GSM_reportTimerExpiry, gsm_ptr,
                    GSM_TMR_REPORT_TIMEOUT_MS)) {
                gsm_ptr->report.timerOn = OSAL_TRUE;
            }
        }
    }

GSM_EXECUTE_API_COMMAND_LABEL:

    /* Now read for the response */
    if (0 >= (bytes = _GSM_readFd(gsm_ptr->devFd, result_ptr, GSM_BUFFER_SZ, 
            cmd_ptr->u.cmd.timeoutSecs))) {
        /* Then we timed out, send error */
        GSM_dbgPrintf("%s %d API command timed out!\n", __FUNCTION__,
                __LINE__);
        _GSM_writeEvent(gsm_ptr, GSM_CMD_ERROR, cmd_ptr->u.cmd.id);
        return;
    }
    /* NULL terminate the data you just read */
    result_ptr[bytes] = 0;
    ret = _GSM_checkResult(result_ptr, cmd_ptr->u.cmd.result);
    if (ret == GSM_RETURN_FAILED) {
        /* 
         * Then this result code is not for us. Pump it to the event queue
         * so the application can process it and then try again to get a 
         * response for this command.
         */
        _GSM_writeEvent(gsm_ptr, result_ptr, 0);
        GSM_dbgPrintf("%s %d Is this not a response to a command!!!\n",
               __FUNCTION__, __LINE__);
        goto GSM_EXECUTE_API_COMMAND_LABEL;

    }
    _GSM_writeEvent(gsm_ptr, result_ptr, cmd_ptr->u.cmd.id);
    return;
}

static GSM_Return _GSM_sendCallReport(
    const char  *header_ptr,
    GSM_Obj     *gsm_ptr)
{
    vint         bytes;
    GSM_Return   ret;
    char        *result_ptr;
    char        *report_ptr;
    const char  *cmdCallReport;
    
    result_ptr = gsm_ptr->report.result;
    report_ptr = gsm_ptr->report.report;

    if (gsm_ptr->extDialCmdEnabled) {
        /*
         * XXX Currently don't request call report when using ext dial cmd.
         * It casuses unsolicited event not been processd.
         */
        //cmdCallReport = GSM_CMD_CALL_EXT_REPORT;
        return (GSM_RETURN_FAILED);
    }
    else {
        cmdCallReport = GSM_CMD_CALL_REPORT;
    }
    /* Write the command to the GSM device */
    if (GSM_RETURN_OK != _GSM_writeAtCommand(gsm_ptr->devFd, 
            cmdCallReport, gsm_ptr->writeScratchPad, GSM_BUFFER_SZ)) {
        /* Couldn't write the command, so skip this for now, try later */
        return (GSM_RETURN_ERR);
    }

GSM_CALL_REPORT_LABEL:

    /* Now read for the response */
    if (0 >= (bytes = _GSM_readFd(gsm_ptr->devFd, result_ptr, GSM_BUFFER_SZ,
            GSM_CMD_REPORT_TIMEOUT_SECS))) {
        /* Then we timed out, send error */
        GSM_dbgPrintf("%s %d Call Status timed out!\n", __FUNCTION__, __LINE__);
        return (GSM_RETURN_ERR);
    }
    /* NULL terminate the data you just read */
    result_ptr[bytes] = 0;
    GSM_dbgPrintf("%s %d Call Status result code: %s\n", __FUNCTION__, __LINE__,
            result_ptr);
    ret = _GSM_checkResult(result_ptr, NULL);
    if (ret == GSM_RETURN_ERR) {
        /* The command had an error.  Return Error */
        return (GSM_RETURN_ERR);
    }
    if (ret == GSM_RETURN_FAILED) {
        /*
         * Then this result code is not for us. Pump it to the event queue
         * so the application can process it and then try again to get a
         * response for this command.
         */
        _GSM_writeEvent(gsm_ptr, result_ptr, 0);
        goto GSM_CALL_REPORT_LABEL;
    }
    
    OSAL_snprintf(report_ptr, GSM_BUFFER_SZ, "%s%s", header_ptr, result_ptr);
    _GSM_writeEvent(gsm_ptr, report_ptr, 0);
    /* If it's contains at least one report then return OK. */
    if ((0 == OSAL_strncmp(result_ptr, GSM_CMD_CALL_STATUS,
            sizeof(GSM_CMD_CALL_STATUS) - 1)) ||
            (0 == OSAL_strncmp(result_ptr, GSM_CMD_CALL_EXT_STATUS,
            sizeof(GSM_CMD_CALL_EXT_STATUS) - 1))) {
        return (GSM_RETURN_OK);
    }
    return (GSM_RETURN_FAILED);
}

/* 
 * ======== _GSM_checkCmccsi() ========
 *
 * This function is check if the result_ptr is CMCCSI event.
 * If yes, consider it as a call report.
 *
 * Return Values: 
 *  GSM_RETURN_OK: It's CMCCSI.
 *  GSM_RETURN_FAILED: It's not CMCCSI.
 */
static GSM_Return _GSM_checkCmccsi(
    GSM_Obj  *gsm_ptr,
    char     *result_ptr)
{
    char        *report_ptr;
    
    report_ptr = gsm_ptr->report.report;

    GSM_dbgPrintf("%s %d Received unsolicited event :%s\n", __FUNCTION__,
            __LINE__, result_ptr);
    
    if(GSM_RETURN_OK == _GSM_checkResult(result_ptr,
            GSM_CMD_CALL_MONITOR)) {
        OSAL_snprintf(report_ptr, GSM_BUFFER_SZ, "%s%s", GSM_CALL_REPORT,
                result_ptr);
        _GSM_writeEvent(gsm_ptr, report_ptr, 0);
        return (GSM_RETURN_OK);
    }
    return (GSM_RETURN_FAILED);
}

static GSM_Return _GSM_checkRing(
    GSM_Obj  *gsm_ptr,
    char     *result_ptr)
{
    GSM_dbgPrintf("%s %d Received unsolicited event :%s\n", __FUNCTION__,
            __LINE__, result_ptr);
    
    if (GSM_RETURN_OK == _GSM_checkResult(result_ptr, GSM_CMD_RING) ||
            GSM_RETURN_OK == _GSM_checkResult(result_ptr, GSM_CALL_RING2)) {
        if (OSAL_FALSE == gsm_ptr->report.timerOn) {
            if (OSAL_SUCCESS == OSAL_tmrPeriodicStart(gsm_ptr->report.tmrId,
                    _GSM_reportTimerExpiry, gsm_ptr,
                    GSM_TMR_REPORT_TIMEOUT_MS)) {
                gsm_ptr->report.timerOn = OSAL_TRUE;
            }
            _GSM_writeEvent(gsm_ptr, result_ptr, 0);
            /* Also immediately send a call report. */
            _GSM_sendCallReport(GSM_CALL_REPORT, gsm_ptr);
        }
        return (GSM_RETURN_OK);
    }
    else if(GSM_RETURN_OK == _GSM_checkResult(result_ptr, GSM_CALL_WAITING)) {
        if (OSAL_FALSE == gsm_ptr->report.timerOn) {
            if (OSAL_SUCCESS == OSAL_tmrPeriodicStart(gsm_ptr->report.tmrId,
                    _GSM_reportTimerExpiry, gsm_ptr,
                    GSM_TMR_REPORT_TIMEOUT_MS)) {
                gsm_ptr->report.timerOn = OSAL_TRUE;
            }
        }
        _GSM_writeEvent(gsm_ptr, result_ptr, 0);
        /* Also immediately send a call report. */
        _GSM_sendCallReport(GSM_CALL_REPORT, gsm_ptr);
        return (GSM_RETURN_OK);
    }
    return (GSM_RETURN_FAILED);
}

static GSM_Return _GSM_checkDisconnect(
    GSM_Obj  *gsm_ptr,
    char     *result_ptr)
{
    GSM_dbgPrintf("%s %d Received unsolicited event :%s\n", __FUNCTION__,
            __LINE__, result_ptr);
    
    if (GSM_RETURN_OK == _GSM_checkResult(result_ptr, GSM_NO_CARRIER)) {
        /* Then try to include the call status resport with it */
        if (GSM_RETURN_ERR == _GSM_sendCallReport(GSM_NO_CARRIER, gsm_ptr)) {
            /*
             * Then the call status command failed. In this case just
             * forward the "no carrier" event back up
             */
            _GSM_writeEvent(gsm_ptr, result_ptr, 0);
        }
        /* This function successfully process this result so return ok */
        return (GSM_RETURN_OK);
    }
    return (GSM_RETURN_FAILED);
}

static GSM_Return _GSM_checkSrvcc(
    GSM_Obj  *gsm_ptr,
    char     *result_ptr)
{
    GSM_dbgPrintf("%s %d Received unsolicited event :%s\n", __FUNCTION__,
            __LINE__, result_ptr);

    if (GSM_RETURN_OK == _GSM_checkResult(result_ptr, GSM_CMD_SRVCC)) {
        if (OSAL_FALSE == gsm_ptr->report.timerOn) {
            if (OSAL_SUCCESS == OSAL_tmrPeriodicStart(gsm_ptr->report.tmrId,
                    _GSM_reportTimerExpiry, gsm_ptr,
                    GSM_TMR_REPORT_TIMEOUT_MS)) {
                gsm_ptr->report.timerOn = OSAL_TRUE;
            }
            _GSM_writeEvent(gsm_ptr, result_ptr, 0);
            /* Also immediately send a call report. */
            _GSM_sendCallReport(GSM_CALL_REPORT, gsm_ptr);
        }
        return (GSM_RETURN_OK);
    }
    return (GSM_RETURN_FAILED);
}

static GSM_Return _GSM_checkSms(
    GSM_Obj  *gsm_ptr,
    char     *result_ptr,
    GSM_Fd    devFd)
{
    GSM_dbgPrintf("%s %d Received unsolicited event :%s\n", __FUNCTION__,
            __LINE__, result_ptr);
    
    if (0 != OSAL_strncmp(result_ptr, GSM_SMS, sizeof(GSM_SMS) - 1)) {
        /* Then this is not an SMS message */
        return (GSM_RETURN_FAILED);
    }
    /* Then we have the complete message, send it to the application */
    _GSM_writeEvent(gsm_ptr, result_ptr, 0);
    return (GSM_RETURN_OK);
}

static void _GSM_executeTimerEvent(
    GSM_Obj  *gsm_ptr,
    GSM_Cmd  *command_ptr)
{
    GSM_Return ret;
    
    if(GSM_TMR_TYPE_REPORT == command_ptr->u.tmr.type) {
        //GSM_dbgPrintf("%s %d kicking call report", __FUNCTION__, __LINE__);
        ret = _GSM_sendCallReport(GSM_CALL_REPORT, gsm_ptr);
        if (GSM_RETURN_FAILED == ret) {
            /* 
             * Then there are no active calls
             */
            //GSM_dbgPrintf("%s %d Call report timer is off", __FUNCTION__, __LINE__);
            OSAL_tmrStop(gsm_ptr->report.tmrId);
            gsm_ptr->report.timerOn = OSAL_FALSE;
        }
    }
}


/* 
 * ======== _GSM_task() ========
 *
 * This function is the task handler for the GSM task created to execute AT
 * commands.
 * 
 * This function will "select()" (wait) for incoming AT commands that 
 * where stimulated by the GSM_atCmd() interface.  Once they are received
 * the AT commands are executed (issued to the underlying device driver) 
 * within the context of this task.
 *
 * Return Values: 
 * Never.  Loops forever.
 */
static void _GSM_task(
    void *arg_ptr)
{
    GSM_Obj         *gsm_ptr;
    OSAL_Boolean     isTimeout; 
    OSAL_SelectSet   fdSet;
    OSAL_Boolean     flag;
    GSM_Fd           cmdFd;
    GSM_Fd           devFd;
    char            *result_ptr;
    GSM_Cmd         *command_ptr;
    vint             bytes;
    
    gsm_ptr = (GSM_Obj*)arg_ptr;

    /* 
     * We use these things over and over so let's cache 'em 
     * for better performance
     */
    devFd = gsm_ptr->devFd;
    cmdFd = gsm_ptr->cmdPipe[GSM_CMD_PIPE_READ]; 
    result_ptr = gsm_ptr->readScratchPad.result;
    command_ptr = &gsm_ptr->readScratchPad.command;
    

    /* 
     * When this task launches the command pipe and device fd
     * are already set in the GSM object's fdset.
     */

GSM_READ_TASK_LABEL:

    /* Copy the fd set every cycle because it will be written by select */
    fdSet = gsm_ptr->fdSet;
    if (OSAL_SUCCESS == OSAL_select(&fdSet, NULL, NULL, &isTimeout)) {

        /* Check which file descriptor */
        if (OSAL_SUCCESS == OSAL_selectIsIdSet(&devFd, &fdSet, &flag)) {
            if (OSAL_TRUE == flag) {
                /* Then we have an "unsolicited" result */
                if (0 < (bytes = _GSM_readFd(devFd, result_ptr,
                        GSM_BUFFER_SZ, GSM_READ_DFLT_TIMEOUT_SECS))) {
                    /* 
                     * Then we have a valid result and it's "unsolicited".
                     * Let's write it to the event queue.  But first NULL 
                     * terminate the result.
                     */
                    result_ptr[bytes] = 0;
                    GSM_dbgPrintf("%s %d: Received 'unsolicited' result:%s bytes:%d\n",
                            __FUNCTION__, __LINE__, result_ptr, bytes);
                    
                    /* Check if it's for unsolicited +CMCCSI */
                    if (GSM_RETURN_OK == _GSM_checkCmccsi(gsm_ptr, result_ptr)) {
                        /* Then the result was consumed by the _GSM_checkCmccsi */
                        goto GSM_READ_TASK_LABEL;
                    }
                    /* Check if it's for ring control */
                    if (GSM_RETURN_OK == _GSM_checkRing(gsm_ptr, result_ptr)) {
                        /* Then the result was consumed by the GSM_checkRing */
                        goto GSM_READ_TASK_LABEL;
                    }
                    if (GSM_RETURN_OK == _GSM_checkDisconnect(gsm_ptr,
                            result_ptr)) {
                        /* Then the result was consumed by the GSM_checkdisc */
                        goto GSM_READ_TASK_LABEL;
                    }
                    if (GSM_RETURN_OK == _GSM_checkSrvcc(gsm_ptr,
                            result_ptr)) {
                        /* Then the result was consumed by the GSM_checkSrvcc */
                        goto GSM_READ_TASK_LABEL;
                    }
                    if (GSM_RETURN_OK == _GSM_checkSms(gsm_ptr,
                            result_ptr, devFd)) {
                        /* Then the result was consumed by the GSM_checkSms */
                        goto GSM_READ_TASK_LABEL;
                    }
                    
                    /* 
                     * Otherwise, it hasn't been process in the GSM module so
                     * pass it upwards to the app.
                     */
                    _GSM_writeEvent(gsm_ptr, result_ptr, 0);
                }
            }
        }
        if (OSAL_SUCCESS == OSAL_selectIsIdSet(&cmdFd, &fdSet, &flag)) {
            if (OSAL_TRUE == flag) {
                /* Then we have a command */
                if (0 < (bytes = _GSM_readFd(cmdFd, (char*)command_ptr, 
                        sizeof(GSM_Cmd), GSM_READ_DFLT_TIMEOUT_SECS))) {
                    if (GSM_CMD_TYPE_CMD == command_ptr->type) {
                        /* Then we have a valid command */
                        GSM_dbgPrintf("%s %d: Recv'd 'AT' cmd id:%d len:%d Command:%s\n",
                            __FUNCTION__, __LINE__, command_ptr->u.cmd.id,
                            bytes, command_ptr->u.cmd.cmd);
                        _GSM_executeApiCommand(gsm_ptr, command_ptr);
                    }
                    else {
                        /* else it's a timer event */
                        GSM_dbgPrintf("%s %d: Recv'd 'Tmr' Event Type:%d\n",
                            __FUNCTION__, __LINE__, command_ptr->u.tmr.type);
                        _GSM_executeTimerEvent(gsm_ptr, command_ptr);
                    }
                }
            }
        }
    }
    goto GSM_READ_TASK_LABEL;
}

/* 
 * ======== GSM_writeApiCommand() ========
 *
 * This function writes an AT command to the command pipe of this GSM module.
 * This "pipe" then gets serviced by the _GSM_task() task handler.
 * 
 * Return Values: 
 * GSM_RETURN_OK: Function successfully write the AT command to the pipe.
 * GSM_RETURN_FAILED: There was an error writing to the command pipe.
 */
GSM_Return GSM_writeApiCommand(
    GSM_Obj *gsm_ptr,
    GSM_Cmd *cmd_ptr)
{
    vint bytes;

    bytes = _GSM_writeFd(gsm_ptr->cmdPipe[GSM_CMD_PIPE_WRITE],
            (char*)cmd_ptr, sizeof(GSM_Cmd));
    if (bytes != sizeof(GSM_Cmd)) {
        /* Not everything was written */
        return (GSM_RETURN_FAILED);
    }
    return (GSM_RETURN_OK);
}

/* 
 * ======== GSM_readEvent() ========
 *
 * This function reads from the event queue used to communicate results
 * to the GSM application.
 *
 * If the function successfully reads an event it will copy it to the 
 * memory specified by the event_ptr.
 *
 * Note that this function can be used as a 'blocking' function or a
 * 'non-blocking' function depending upon the value of the 'timeout'
 * parameter.
 * 
 * Return Values: 
 * GSM_RETURN_OK: An event was successfully read and copied to event_ptr.
 * GSM_RETURN_FAILED: There was an error reading the event queue
 * GSM_RETURN_TIMEOUT: The function timed out.  There is NO event to process.
 */
GSM_Return GSM_readEvent(
    GSM_Obj   *gsm_ptr,
    GSM_Event *event_ptr,
    uint32     timeout)
{
    int32        ret;
    OSAL_Boolean isTo;
    
    isTo = OSAL_FALSE;
    ret = OSAL_msgQRecv(gsm_ptr->evtQId, (char *)event_ptr,
            sizeof(GSM_Event), timeout, &isTo);
    
    GSM_dbgPrintf("The return from OSAL read is :%d\n", ret);
    
    if (0 >= ret) {
        if (OSAL_TRUE == isTo) {
            return (GSM_RETURN_TIMEOUT);
        }
        return (GSM_RETURN_FAILED);
    }
    return (GSM_RETURN_OK);
}

/* 
 * ======== GSM_initTask() ========
 *
 * This function will initialize the GSM task used to process AT commands.
 *
 * Return Values: 
 * GSM_RETURN_OK: The GSM task was successfully created.
 * GSM_RETURN_FAILED: There was an error initializing the GSM task.
 */
GSM_Return GSM_initTask(
    GSM_Obj *gsm_ptr)
{
    GSM_Task *task_ptr;

    task_ptr = &gsm_ptr->task;
    
    task_ptr->id = 0;
    task_ptr->stackSz = GSM_READ_TASK_STACK_SZ;
    task_ptr->pri      = OSAL_TASK_PRIO_NRT;
    task_ptr->func_ptr = _GSM_task;
    OSAL_snprintf(task_ptr->name, sizeof(task_ptr->name), "%s", "GSM Driver");
    task_ptr->arg_ptr  = (void *)gsm_ptr;
    if (0 == (task_ptr->id = OSAL_taskCreate(task_ptr->name,
            task_ptr->pri,
            task_ptr->stackSz,
            task_ptr->func_ptr,
            (void *)task_ptr->arg_ptr))) {
        return (GSM_RETURN_FAILED);
    }
    return (GSM_RETURN_OK);
}

/* 
 * ======== GSM_destroyTask() ========
 *
 * This function will destroy the GSM task used to process AT commands.
 *
 * Return Values: 
 * GSM_RETURN_OK: The GSM task was destroyed
 * GSM_RETURN_FAILED: There was an error destroying the GSM task.
 */
GSM_Return GSM_destroyTask(
    GSM_Obj *gsm_ptr)
{
    if (OSAL_FAIL == OSAL_taskDelete(gsm_ptr->task.id)) {
        return (GSM_RETURN_FAILED);
    }
    return (GSM_RETURN_OK);
}

/* 
 * ======== GSM_initAllResources() ========
 *
 * This function will create and initialize all system resources
 * needed by the GSM module internally.  This includes...
 * 1) The Command Pipe
 * 2) The Event Queue
 * 3) a mutex used to generate unique identifiers
 * 4) File descriptor bitmaps used to 'select()' on
 *
 * Return Values: 
 * GSM_RETURN_OK: All system resources were successfully created/initialized.
 * GSM_RETURN_FAILED: There was an error creating/initializing a 
 *                    system resource.
 */
GSM_Return GSM_initAllResources(
    GSM_Obj *gsm_ptr)
{
    GSM_IdMngr   *mngr_ptr;
    
    /* Init the manager used to issue unique ID values */
    mngr_ptr = &gsm_ptr->idMngr;
    mngr_ptr->id = 1;
    if (0 == (mngr_ptr->mutex = OSAL_semMutexCreate())) {
        return (GSM_RETURN_FAILED);
    }
    
    /* Init the OSAL queue used for the event queue */
    if (0 == (gsm_ptr->evtQId = OSAL_msgQCreate(
            GSM_EVENT_MSG_Q_NAME,
            OSAL_MODULE_GAPP_GSM, OSAL_MODULE_GAPP,
            OSAL_DATA_STRUCT_GSM_Event,
            GSM_EVT_Q_LEN, 
            sizeof(GSM_Event),
            0))) {
        OSAL_semDelete(mngr_ptr->mutex);
        return (GSM_RETURN_FAILED);
    }

    /* 
     * Init the pipe used to queue commands. Pass an array of fd's 
     * for read and write directions. [0] is for writing and [1] 
     * for reading.
     */
    if (0 > pipe(gsm_ptr->cmdPipe)) {
        /* We failed to open the pipe */
        OSAL_semDelete(mngr_ptr->mutex);
        OSAL_msgQDelete(gsm_ptr->evtQId);
        return (GSM_RETURN_FAILED);
    }

    /* Init the timer object needed to generate call reports */
    if (0 == (gsm_ptr->report.tmrId = OSAL_tmrCreate())) {
        OSAL_semDelete(mngr_ptr->mutex);
        OSAL_msgQDelete(gsm_ptr->evtQId);
        close(gsm_ptr->cmdPipe[GSM_CMD_PIPE_READ]);
        close(gsm_ptr->cmdPipe[GSM_CMD_PIPE_WRITE]);
        return (GSM_RETURN_FAILED);
    }
    
    gsm_ptr->report.timerOn = OSAL_FALSE;
        
    /* 0 the fd bit map and add read fd of the cmd pipe */
    if (OSAL_FAIL == OSAL_selectSetInit(&gsm_ptr->fdSet) || 
            OSAL_FAIL == OSAL_selectAddId(&gsm_ptr->cmdPipe[GSM_CMD_PIPE_READ],
            &gsm_ptr->fdSet)) {
        OSAL_semDelete(mngr_ptr->mutex);
        OSAL_msgQDelete(gsm_ptr->evtQId);
        close(gsm_ptr->cmdPipe[GSM_CMD_PIPE_READ]);
        close(gsm_ptr->cmdPipe[GSM_CMD_PIPE_WRITE]);
        OSAL_tmrDelete(gsm_ptr->report.tmrId);
        return (GSM_RETURN_FAILED);
    }

    return (GSM_RETURN_OK);
}

/* 
 * ======== GSM_destroyAllResources() ========
 *
 * This function will destroy ('free') system resources
 * needed by this GSM module.  This includes...
 * 1) The Command Pipe
 * 2) The Event Queue
 * 3) a mutex used to generate unique identifiers
 *
 * Return Values: 
 * GSM_RETURN_OK: All system resources were successfully freed.
 * GSM_RETURN_FAILED: There was an error freeing one of the 
 *                    system resources.
 */
GSM_Return GSM_destroyAllResources(
    GSM_Obj *gsm_ptr)
{
    GSM_Return ret;
    
    ret = GSM_RETURN_OK;
    
    /* Stop and free the timer used for generating call reports */
    OSAL_tmrStop(gsm_ptr->report.tmrId);
    OSAL_tmrDelete(gsm_ptr->report.tmrId);
    
    /* Free the ID manager mutex */
    if (OSAL_FAIL == OSAL_semDelete(gsm_ptr->idMngr.mutex)) {
        ret = GSM_RETURN_FAILED;
    }

    if (OSAL_FAIL == OSAL_msgQDelete(gsm_ptr->evtQId)) {
        ret = GSM_RETURN_FAILED;
    }
    
    /* Free up the cmd pipe */
    close(gsm_ptr->cmdPipe[GSM_CMD_PIPE_READ]);
    close(gsm_ptr->cmdPipe[GSM_CMD_PIPE_WRITE]);
    return (ret);
}

/* 
 * ======== GSM_initDev() ========
 *
 * This function is used to initialize the underlying GSM device by using 
 * the linux "dev" driver.
 *
 * This function will open an "io" path the device and then issue 
 * AT commands that are specified in an XML file to initialize the underlying
 * GSM device.
 *
 * If the device driver can not be opened or if there is an error detected 
 * with the AT commands in the .xml file used to init the device driver then
 * an error is returned.
 *
 * Return Values: 
 * GSM_RETURN_OK: The underlying GSM device was successfully opened and 
 *                initialized.
 * GSM_RETURN_ERR: There was an error with either...
 *                 1) Opening the device driver
 *                 2) parsing the xml file used to init the device
 *                 3) the execution of one of the AT commands specified in
 *                    the xml file.
 * GSM_RETURN_FAILED: Error with placing the GSM device in to a state where
 *                    this GSM module can read/write to it.
 */
GSM_Return GSM_initDev(
    GSM_Obj    *gsm_ptr, 
    const char *configFile_ptr)
{
    GSM_Fd   devFd;
    ezxml_t  xml_ptr;
    char    *serial_ptr;
    char    *dev_ptr;
    int      baud;
    
    if (NULL == (xml_ptr = ezxml_parse_file(configFile_ptr))) {
        /* Could not open and read file, try the default directory */
        OSAL_snprintf(gsm_ptr->writeScratchPad, GSM_BUFFER_SZ, "%s%s",
                GSM_CONFIG_DEFAULT_PATH, configFile_ptr);
        if (NULL == (xml_ptr =
                ezxml_parse_file(gsm_ptr->writeScratchPad))) {
            GSM_dbgPrintf("%s %d Failed to open config file %s\n",
                    __FUNCTION__, __LINE__, gsm_ptr->writeScratchPad);
            return (GSM_RETURN_ERR);
        }
    }

    /* Get the device drive absolute path */
    if (NULL == (dev_ptr = _GSM_getXmlTagText(xml_ptr, GSM_XML_DEVICE_TAG))) {
        /* Error getting the path to the serial driver */
        ezxml_free(xml_ptr);
        return (GSM_RETURN_ERR);
    }

    /* Get any serial connection information */
    if (NULL == (serial_ptr = _GSM_getXmlNestedTagText(xml_ptr,
            GSM_XML_SERIAL_TAG, GSM_XML_BAUD_TAG))) {
        /* Can't get the serial connection info, return failure */
        ezxml_free(xml_ptr);
        return (GSM_RETURN_ERR);
    }
    /* Convert the string */
    baud = OSAL_atoi(serial_ptr);

    /* Open the terminal */
#ifndef GSM_TEST_FORCE_FD
    devFd = _GSM_openDev(dev_ptr, baud);
#else
    devFd = GSM_TEST_FORCE_FD;
#endif
    if (0 > devFd) {
        /* Failed to open the device driver return failure */
        ezxml_free(xml_ptr);
        return (GSM_RETURN_DEV);
    }

    if (GSM_RETURN_OK != _GSM_loadAtCommands(devFd, xml_ptr, 
            gsm_ptr->writeScratchPad, GSM_BUFFER_SZ)) {
        ezxml_free(xml_ptr);
        return (GSM_RETURN_ERR);
    }

    /* All is well, let's add the device to the fd's to read */
    gsm_ptr->devFd = devFd;

    /* Add the dev to the fd set */
    if (OSAL_FAIL == OSAL_selectAddId(&devFd, &gsm_ptr->fdSet)) {
        ezxml_free(xml_ptr);
        return (GSM_RETURN_FAILED);
    }
    ezxml_free(xml_ptr);
    return (GSM_RETURN_OK);
}

/* 
 * ======== GSM_destroyDev() ========
 *
 * This function is used to close the underlying GSM device
 * (linux "dev" device).
 *
 * Return Values: 
 * GSM_RETURN_OK: Always.
 */
GSM_Return GSM_destroyDev(
    GSM_Obj *gsm_ptr)
{
    _GSM_closeFd(gsm_ptr->devFd);
    return (GSM_RETURN_OK);
}


