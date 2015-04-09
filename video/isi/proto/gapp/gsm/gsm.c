/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2007 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 25157 $ $Date: 2014-03-17 00:59:26 +0800 (Mon, 17 Mar 2014) $
 */

#include "_gsm_cfg.h"
#include "gsm.h"
#include "_gsm.h"

static OSAL_Boolean _GSM_isInitialized = OSAL_FALSE;
static GSM_Obj  *_GSM_obj_ptr = NULL;

/*
 * This array contains AT commands that have 'special' responses.
 * If a command is issued and it is identified as requiring
 * a 'special' response, then value in the second string (the result)
 * if searched for when looking for a response.  In other words
 * this array contains an additional response to search for
 * when the command completes.
 *
 */
static GSM_Result _GSM_AT_RESULTS[] = {
    {   "AT+CRSM", "+CRSM"  },
    {   "AT+CMGS", ">"  },
    {   "AT+CREG", "+CREG"  },
    {   "AT+CDU" , "+CDU"  },
};

/*
 * Searches the above array to find commands that require an
 * additional 'special' response.
 */
static const char* _GSM_getResult(const char *cmd_ptr)
{
    vint x;
    vint size = sizeof(_GSM_AT_RESULTS) / sizeof(GSM_Result);
    for (x = 0 ; x < size ; x++) {
        if (0 == OSAL_strncmp(_GSM_AT_RESULTS[x].cmd_ptr, cmd_ptr,
                OSAL_strlen(_GSM_AT_RESULTS[x].cmd_ptr))) {
            return _GSM_AT_RESULTS[x].result_ptr;
        }
    }
    return NULL;
}

/* 
 * ======== GSM_init() ========
 * 
 * This function is used to initialize and configure the GSM interface.  This 
 * function will open the configuration file specified by the configFile_ptr 
 * parameter.  It will then open a serial connection to the device driver and 
 * begin launching the AT commands specified in the configuration file.  
 * If access to the device driver is successful and all the AT commands execute
 * successfully then the function will return GSM_RETURN_OK. 
 *
 * Return Values: 
 * GSM_RETURN_OK: The GSM interface has been successfully initialized.  
 * GSM_RETURN_FAILED: The GSM interface could NOT be initialized.  
 *                    There was an error initializing a system resource needed
 *                    by this module. 
 * GSM_RETURN_DEV: The GSM interface could NOT be opened/initialized.  There 
 *                 was an error accessing the GSM device driver.
 * GSM_RETURN_ERR: There was an error opening and loading the configFile or the
 *                 GSM module is already initialized.
 */
GSM_Return GSM_init(
    const char *configFile_ptr, /* A pointer to a NULL terminated string 
                           * containing the absolute path to the initialization
                           * file used to initialize and configure the GSM 
                           * interface. */
    OSAL_Boolean extDialCmdEnabled)

{
    if (OSAL_TRUE == _GSM_isInitialized) {
        /* Then the GSM module is already initialized */
        return (GSM_RETURN_ERR);
    }

    if (NULL == configFile_ptr) {
        /* The configuration file is mandatory */
        return (GSM_RETURN_DEV);
    }

    _GSM_obj_ptr = OSAL_memAlloc(sizeof(GSM_Obj), 0);
    if (NULL == _GSM_obj_ptr) {
        /* Error with memory allocation */
        return (GSM_RETURN_FAILED);
    }
    
    /* Init the GSM_Obj */
    if (GSM_RETURN_OK != GSM_initAllResources(_GSM_obj_ptr)) {
        /* 
         * Then there was an issue allocating and initializing 
         * system resources 
         */
        OSAL_memFree(_GSM_obj_ptr, 0);
        return (GSM_RETURN_FAILED);
    }

    /* Open the device driver and load all AT commands */
    if (GSM_RETURN_OK != GSM_initDev(_GSM_obj_ptr, configFile_ptr)) {
        /* Error getting access to the underlying GSM device driver */
        GSM_destroyAllResources(_GSM_obj_ptr);
        OSAL_memFree(_GSM_obj_ptr, 0);
        return (GSM_RETURN_DEV);
    }

    /* Launch the task that handles everything */
    if (GSM_RETURN_OK != GSM_initTask(_GSM_obj_ptr)) {
        /* Error launching the task, clean house and return */
        GSM_destroyDev(_GSM_obj_ptr);
        GSM_destroyAllResources(_GSM_obj_ptr);
        OSAL_memFree(_GSM_obj_ptr, 0);
        return (GSM_RETURN_FAILED);
    }

    _GSM_obj_ptr->extDialCmdEnabled = extDialCmdEnabled;
    _GSM_isInitialized = OSAL_TRUE;
    return (GSM_RETURN_OK);
}

/* 
 * ======== GSM_destroy() ========
 * 
 * This function is used to destroy the GSM module.  This function will
 * place the GSM interface into a quiet state, close the device driver 
 * to the GSM interface, and then release any internal system resources 
 * used to accomodate this module.
 *
 * Note that the return value of this function is purely informative.
 * If an error was detected when trying to close and release any resource
 * then a value of GSM_RETURN_FAILED is returned. However there is very 
 * little a developer can so in this situation.  In this case developers
 * must consider the failure to have "leaked" a resource.
 * 
 * Return Values: 
 * GSM_RETURN_OK: The GSM module was successfully shut down.  
 * GSM_RETURN_FAILED: A failure was detected; however, the function still 
 *                    attempted to close and release as many resources as 
 *                    possible.
 * GSM_RETURN_ERR: The GSM module is not initialized
 */
GSM_Return GSM_destroy(void)
{
    GSM_Return ret;

    if (OSAL_FALSE == _GSM_isInitialized) {
        /* Then the GSM module is not initialized */
        return (GSM_RETURN_ERR);
    }

    ret = GSM_RETURN_OK;
        
    /* Destroy the task */
    if (GSM_RETURN_OK != GSM_destroyTask(_GSM_obj_ptr)) {
        ret = GSM_RETURN_FAILED;
    }

    /* Close up the GSM device driver */
    if (GSM_RETURN_OK != GSM_destroyDev(_GSM_obj_ptr)) {
        ret = GSM_RETURN_FAILED;
    }
    
    /* Free/release all system resource */
    if (GSM_RETURN_OK != GSM_destroyAllResources(_GSM_obj_ptr)) {
        ret = GSM_RETURN_FAILED;
    }

    /* Free up the global memory */
    OSAL_memFree(_GSM_obj_ptr, 0);
    _GSM_isInitialized = OSAL_FALSE;
    return (ret);
}

/* 
 * ======== GSM_atCmd() ========
 * 
 * This function will issue an AT command to the underlying GSM serial 
 * interface.  If this function is successful then this GSM module will write
 * an identifier to the id_ptr parameter.  Applications can use the value 
 * written to id_ptr to associate AT commands to AT command responses.  The 
 * timeoutMs parameter is used to specify an expiry for the command.  If this 
 * timeout expires and the AT command has still not received a response, then 
 * the GSM module will consider the AT command to have failed and will generate
 * an event signifying the AT command's failure to execute.  
 * If timeoutSecs is "0" (zero) then no expiry will be used.  The AT command 
 * will simply be issued and no response will be generated to the application.
 * If this value is "-1" then the GSM module will "wait forever" for a 
 * response.
 * 
 * This is a non-blocking function; meaning, tasks calling this interface will
 * not be placed into a "wait" or "blocked" state.  This function issues the 
 * AT command in the GSM module's command pipe and then returns 
 * immediately.  The responses to AT commands, whether successful or not, are 
 * generated as events to the application.
 *
 * Return Values: 
 * GSM_RETURN_OK: The GSM AT command was successfully issued.  
 * GSM_RETURN_FAILED: There was an error with issuing the AT Command.  
 *                    The Command was not successfully issued. 
 * GSM_RETURN_ERR: The GSM module is not initialized.
 */
GSM_Return GSM_atCmd(
    const char *cmd_ptr, /* A pointer to a NULL terminated AT command */
    GSM_Id     *id_ptr,  /* A pointer to an ID for associating cmd responses */
    vint        timeoutSecs) /* Timeout in secs for this command to complete */
{
    GSM_Cmd cmd;
    const char *result_ptr;

    if (OSAL_FALSE == _GSM_isInitialized) {
        /* Then the GSM module is not initialized */
        return (GSM_RETURN_ERR);
    }

    /* Set the command type */
    cmd.type = GSM_CMD_TYPE_CMD;
    /* 
     * Let's generate a unique id for this command. 
     * Note, this function takes a mutex to get a unique number 
     */
    if (GSM_RETURN_OK != GSM_getUniqueId(&_GSM_obj_ptr->idMngr,
            &cmd.u.cmd.id)) {
        /* Can't generate a unique ID */
        return (GSM_RETURN_FAILED);
    }

    GSM_dbgPrintf("%s %d: AT gsmId:%d, AT:%s\n",
            __FUNCTION__, __LINE__, cmd.u.cmd.id, cmd_ptr);

    /* Copy the timeout */
    cmd.u.cmd.timeoutSecs = timeoutSecs;
    /* Copy the command string */
    OSAL_snprintf(cmd.u.cmd.cmd, GSM_BUFFER_SZ, "%s", cmd_ptr);

    cmd.u.cmd.result[0] = 0;
    result_ptr = _GSM_getResult(cmd_ptr);
    if (NULL != result_ptr) {
        /* Copy the command string */
        OSAL_snprintf(cmd.u.cmd.result, GSM_BUFFER_SZ, "%s", result_ptr);
    }

    /* Send it down the command pipe */
    if (GSM_RETURN_OK != GSM_writeApiCommand(_GSM_obj_ptr, &cmd)) {
        /* Could not write the command */
        return (GSM_RETURN_FAILED);
    }
    /* All is good copy the id_ptr */
    if (NULL != id_ptr) {
        *id_ptr = cmd.u.cmd.id;
    }
    return (GSM_RETURN_OK);
}


/* 
 * ======== GSM_getEvent() ========
 * 
 * This function is called by applications to retrieve events from the GSM 
 * module.  It can be used to "poll" for the existence of waiting events 
 * or it can wait (block) until an event is generated from the GSM interface.
 * Events are generated by the GSM module as a result of the completion of an
 * AT command or they can be "unsolicited"; meaning, there was no AT command 
 * that stimulated the event, rather it is a status event received from the 
 * GSM protocol (GSM network).
 * If this function is successful, an event is waiting to be processed at 
 * the memory pointed to by event_ptr.  If it is unsuccessful then no data 
 * will be written.  If timeoutMs is "0" then the function will NOT block or
 * wait for an event.  If timeoutMs is "-1" then the function will wait 
 * indefinitely for an event to be generated.  Otherwise, timeoutMs represents
 * the number of milliseconds to wait (block) before returning.  
 *
 * Return Values: 
 * GSM_RETURN_OK: An event has been successfully read and is populated in 
 *                the memory pointed to by event_ptr.  
 * GSM_RETURN_FAILED: No event has been read. There was an error reading the 
 *                    underlying device driver serial interface. 
 * GSM_RETURN_TIMEOUT: The timeout specified in the timeoutMs parameter has 
 *                     expired.  There is no event to process.
 * GSM_RETURN_ERR: The GSM module is not initialized. 
 */
GSM_Return GSM_getEvent(
    GSM_Event *event_ptr,  /* A ptr to a GSM_Event where the evt is written */
    vint       timeoutMs)  /* Timeout in milliseconds to wait for event */
{
    uint32 timeout;
    
    if (OSAL_FALSE == _GSM_isInitialized) {
        /* Then the GSM module is not initialized */
        return (GSM_RETURN_ERR);
    }

    /* Validate the event_ptr arg */
    if (NULL == event_ptr) {
        return (GSM_RETURN_FAILED);
    }

    switch (timeoutMs) {
    case 0:
        timeout = OSAL_NO_WAIT;
        break;
    case -1:
        timeout = OSAL_WAIT_FOREVER;
        break;
    default:
        timeout = timeoutMs;
        break;
    }
    return (GSM_readEvent(_GSM_obj_ptr, event_ptr, timeout));
}

/* 
 * ======== GSM_getEventQueue() ========
 * 
 * This function is used to retrieve the ID of the queue used by the GSM 
 * module to write events as they are received. As an alternative to using the
 * GSM_getEvent() API to retrieve GSM events, an application may use this API 
 * to retrieve the queue ID of the GSM event queue and then group this queue 
 * with others as a way of possibly serializing GSM events with other system 
 * or user events within the application.
 *
 * Return Values: 
 * GSM_RETURN_OK: The event queue ID was successfully written to queueId_ptr.
 * GSM_RETURN_FAILED: The event queue was not retrieved.  There was an error 
 *                    with a parameter.
 * GSM_RETURN_ERR: The GSM module is not initialized. 
 * 
 */
GSM_Return GSM_getEventQueue(
    OSAL_MsgQId *queueId_ptr)
{
    if (OSAL_FALSE == _GSM_isInitialized) {
        /* Then the GSM module is not initialized */
        return (GSM_RETURN_ERR);
    }
    /* validate the queueId_ptr */
    if (NULL == queueId_ptr) {
        return (GSM_RETURN_FAILED);
    }
    *queueId_ptr = _GSM_obj_ptr->evtQId;
    return (GSM_RETURN_OK);
}

GSM_Return GSM_getProxyQueue(
    OSAL_MsgQId *queueId_ptr)
{
    if (OSAL_FALSE == _GSM_isInitialized) {
        /* Then the GSM module is not initialized */
        return (GSM_RETURN_ERR);
    }
    /* validate the queueId_ptr */
    if (NULL == queueId_ptr) {
        return (GSM_RETURN_FAILED);
    }
    *queueId_ptr = _GSM_obj_ptr->evtQId;
    return (GSM_RETURN_OK);
}


GSM_Return GSM_cmdReadSmsMem(
    void   *arg_ptr,
    GSM_Id *gsmId_ptr,
    char   *number_ptr)
{
    return (GSM_RETURN_ERR);
}

GSM_Return GSM_cmdDeleteSmsMem(
    void    *arg_ptr,
    GSM_Id  *gsmId_ptr,
    char    *number_ptr)
{
    return (GSM_RETURN_ERR);
}

GSM_Return GSM_cmdDisableCallForwarding(
    void   *arg_ptr,
    GSM_Id *gsmId_ptr,
    int     type)
{
    return (GSM_RETURN_ERR);
}

GSM_Return GSM_cmdEnableCallForwardingNoReply(
    void    *arg_ptr,
    GSM_Id  *gsmId_ptr,
    int      type,
    char    *to_ptr,
    int      timeout)
{
    return (GSM_RETURN_ERR);
}

GSM_Return GSM_cmdEnableCallForwarding(
    void    *arg_ptr,
    GSM_Id  *gsmId_ptr,
    int      type,
    char    *to_ptr)
{
    return (GSM_RETURN_ERR);
}

GSM_Return GSM_cmdDialDigit(
    void   *arg_ptr,
    GSM_Id *gsmId_ptr,
    char    digit,
    int     duration)
{
    char buffer[GSM_SCRATCH_BUFFER_SZ + 1];

    /*
     * AT$VTS=x,1/0 is for digit start/end.
     * Set duration as MAX_INT32 for dial digit start in PROXY.
     * Therefore need to convert the duration before sending to GSM device 
     * for digit start(AT$VTS=x,1).
     */
    if (MAX_INT32 == duration) {
        duration = 1;
    }

    OSAL_snprintf(buffer, GSM_SCRATCH_BUFFER_SZ,
            "%s%c,%d", "AT$VTS=", digit, duration);
    return GSM_atCmd(buffer, gsmId_ptr, GSM_CMD_TO_SECS);
}

/*
 * Note this currently requests the registration state but does not
 * command the registration state to register.  this is because
 * the Android PhoneApp does this.
 */
GSM_Return GSM_cmdRegister(
    void   *arg_ptr,
    GSM_Id *gsmId_ptr)
{
    char buffer[GSM_SCRATCH_BUFFER_SZ + 1];
    OSAL_snprintf(buffer, GSM_SCRATCH_BUFFER_SZ, "%s", "AT+CREG?");
    return GSM_atCmd(buffer, gsmId_ptr, GSM_CMD_TO_SECS);
}

/*
 * Note this currently commands the device to de-reg and then requests
 * the registration state.
 */
GSM_Return GSM_cmdDeregister(
    void   *arg_ptr,
    GSM_Id *gsmId_ptr)
{
    char buffer[GSM_SCRATCH_BUFFER_SZ + 1];
    OSAL_snprintf(buffer, GSM_SCRATCH_BUFFER_SZ, "%s%d;%s",
            "AT+COPS=", 2, "+CREG?");
    return GSM_atCmd(buffer, gsmId_ptr, GSM_CMD_TO_SECS);
}

GSM_Return GSM_cmdSendSms(
    void   *arg_ptr,
    GSM_Id *gsmId_ptr,
    char   *to_ptr,
    char   *msg_ptr,
    int     requestReport)
{
    char buffer[GSM_SCRATCH_BUFFER_SZ + 1];
    OSAL_snprintf(buffer, GSM_SCRATCH_BUFFER_SZ, "%s%d", "AT+CMGS=", (OSAL_strlen(msg_ptr) / 2) - 1);
    return GSM_atCmd(buffer, gsmId_ptr, GSM_CMD_TO_SECS);
}

GSM_Return GSM_cmdWriteSms(
    void   *arg_ptr,
    GSM_Id *gsmId_ptr,
    char   *msg_ptr)
{
    char buffer[GSM_BUFFER_SZ + 1];
    OSAL_snprintf(buffer, GSM_BUFFER_SZ, "%s", msg_ptr);
    return GSM_atCmd(buffer, gsmId_ptr, GSM_CMD_TO_SECS);
}

GSM_Return GSM_cmdDial(
    void   *arg_ptr,
    GSM_Id *gsmId_ptr,
    char   *to_ptr,
    int     blockId)
{
    char buffer[GSM_SCRATCH_BUFFER_SZ + 1];
    if (0 == blockId) {
        /* Default */
        OSAL_snprintf(buffer, GSM_SCRATCH_BUFFER_SZ, "%s%s;", "ATD", to_ptr);
    }
    else if(1 == blockId) {
        /* Invocation */
        OSAL_snprintf(buffer, GSM_SCRATCH_BUFFER_SZ, "%s%s%s;", "ATD", to_ptr, "I");
    }
    else {
        /* Suppression */
        OSAL_snprintf(buffer, GSM_SCRATCH_BUFFER_SZ, "%s%s%s;", "ATD", to_ptr, "i");
    }
    return GSM_atCmd(buffer, gsmId_ptr, GSM_CMD_TO_SECS);
}

GSM_Return GSM_cmdHangup(
    void   *arg_ptr,
    GSM_Id *gsmId_ptr)
{
    return GSM_atCmd("ATH", gsmId_ptr, GSM_CMD_TO_SECS);
}

GSM_Return GSM_cmdReject(
    void   *arg_ptr,
    GSM_Id *gsmId_ptr)
{
    //return GSM_atCmd("ATH", gsmId_ptr, GSM_CMD_TO_SECS);
    return GSM_atCmd("AT+CHLD=0", gsmId_ptr, GSM_CMD_TO_SECS);
}

GSM_Return GSM_cmdCallHoldZero(
    void   *arg_ptr,
    GSM_Id *gsmId_ptr)
{
    return GSM_atCmd("AT+CHLD=0", gsmId_ptr, GSM_CMD_TO_SECS);
}

GSM_Return GSM_cmdCallHoldTwo(
    void   *arg_ptr,
    GSM_Id *gsmId_ptr)
{
    return GSM_atCmd("AT+CHLD=2", gsmId_ptr, GSM_CMD_TO_SECS);
}

GSM_Return GSM_cmdCallHoldOneIndex(
    void   *arg_ptr,
    GSM_Id *gsmId_ptr,
    int     callIndex)
{
    char buffer[GSM_SCRATCH_BUFFER_SZ + 1];
    OSAL_snprintf(buffer, GSM_SCRATCH_BUFFER_SZ, "%s%d;", "AT+CHLD=1", callIndex);
    return GSM_atCmd(buffer, gsmId_ptr, GSM_CMD_TO_SECS);
}

GSM_Return GSM_cmdCallHoldTwoIndex(
    void   *arg_ptr,
    GSM_Id *gsmId_ptr,
    int     callIndex)
{
    char buffer[GSM_SCRATCH_BUFFER_SZ + 1];
    OSAL_snprintf(buffer, GSM_SCRATCH_BUFFER_SZ, "%s%d;", "AT+CHLD=2", callIndex);
    return GSM_atCmd(buffer, gsmId_ptr, GSM_CMD_TO_SECS);
}

GSM_Return GSM_cmdHoldThree(
    void   *arg_ptr,
    GSM_Id *gsmId_ptr)
{
    return GSM_atCmd("AT+CHLD=3", gsmId_ptr, GSM_CMD_TO_SECS);
}

GSM_Return GSM_cmdCallAnswer(
    void   *arg_ptr,
    GSM_Id *gsmId_ptr)
{
    return GSM_atCmd("ATA", gsmId_ptr, GSM_CMD_TO_SECS);
}


GSM_Return GSM_cmdCallMute(
    void   *arg_ptr,
    GSM_Id *gsmId_ptr,
    int     mute)
{
    return (-1);
}

GSM_Return GSM_cmdSendUssd(
    void   *arg_ptr,
    GSM_Id *gsmId_ptr,
    char   *to_ptr)
{
    return (-1);
}

GSM_Return GSM_cmdIsim(
    void                *arg_ptr,
    GSM_Id              *gsmId_ptr,
    vint                 attribute, /* ISIM attribute to get, for example domain, PCFSC */
    char                *arg1_ptr,
    char                *arg2_ptr)
{
    return (-1);
}


GSM_Return GSM_cmdForward(
    void   *arg_ptr,
    GSM_Id *gsmId_ptr,
    char   *to_ptr)
{
    return (-1);
}

GSM_Return GSM_cmdBlindTransfer(
    void   *arg_ptr,
    GSM_Id *gsmId_ptr,
    char   *to_ptr)
{
    return (-1);
}

GSM_Return GSM_cmdFmcDialDigitString(
    void   *arg_ptr,
    GSM_Id *gsmId_ptr,
    char   *dialSequence_ptr)
{
    return (-1);
}

GSM_Return GSM_cmdFmcDialDigit(
    void   *arg_ptr,
    GSM_Id *gsmId_ptr,
    char    digit,
    int     duration)
{
    return (-1);
}

GSM_Return GSM_cmdFmcDial(
    void   *arg_ptr,
    GSM_Id *gsmId_ptr,
    char   *to_ptr)
{
    return (-1);
}

/*
 * ======== GSM_cmdSetMediaProfiles() ========
 *
 * Send AT command to set media profile.
 * Currently it only supports setting m-line.
 *
 * Return:
 *  GSM_RETURN_OK: Command sent.
 *  GSM_RETURN_ERR: Command sent failed.
 */
GSM_Return GSM_cmdSetMediaProfiles(
    void   *arg_ptr,
    GSM_Id *gsmId_ptr)
{
    char buffer[GSM_SCRATCH_BUFFER_SZ + 1];

    /* Set media profile for audio. */
    OSAL_snprintf(buffer, GSM_SCRATCH_BUFFER_SZ, "%s=%d,\"%s\"",
            _GSM_CMD_CDEFMP,
            GSM_MEDIA_PROFILE_ID_AUDIO, GSM_MEDIA_PROFILE_STR_AUDIO);
    if (GSM_RETURN_OK != GSM_atCmd(buffer, gsmId_ptr, GSM_CMD_TO_SECS)) {
        return (GSM_RETURN_ERR);
    }

    /* Set media profile for audio+video */
    OSAL_snprintf(buffer, GSM_SCRATCH_BUFFER_SZ, "%s=%d,\"%s\"",
            _GSM_CMD_CDEFMP, GSM_MEDIA_PROFILE_ID_VIDEO,
            GSM_MEDIA_PROFILE_STR_AUDIO_VIDEO);

    if (GSM_RETURN_OK != GSM_atCmd(buffer, gsmId_ptr, GSM_CMD_TO_SECS)) {
        return (GSM_RETURN_ERR);
    }

    return (GSM_RETURN_OK);
}

/*
 * ======== GSM_cmdDialUri() ========
 *
 * Send AT command to dial an to an URI.
 * This is extended dial AT command set.
 *
 * Return:
 *  GSM_RETURN_OK: Command sent.
 *  GSM_RETURN_ERR: Command sent failed.
 */
GSM_Return GSM_cmdDialUri(
    void   *arg_ptr,
    GSM_Id *gsmId_ptr,
    char   *to_ptr,
    int     mpIdx)
{
    char buffer[GSM_SCRATCH_BUFFER_SZ + 1];

    /*
     * Populate CDU command.
     * AT+CDU=<action>,<URI>,<client>,<mpidx>
     * Example:
     * AT+CDU=1, "sip:123@domain", 1, 1
     * Note: action will always be 1 for dial and client will always be 1 for
     * MMTel
     */
    OSAL_snprintf(buffer, GSM_SCRATCH_BUFFER_SZ, "%s=1,\"%s\",1,%d",
            _GSM_CMD_CDU, to_ptr, mpIdx);
    return (GSM_atCmd(buffer, gsmId_ptr, GSM_CMD_TO_SECS));
}

/*
 * ======== GSM_cmdMediaControl() ========
 *
 * Send AT command to set, propose, accept or reject
 * the media description for the call.
 *
 * Return:
 *  GSM_RETURN_OK: Command sent.
 *  GSM_RETURN_ERR: Command sent failed.
 */
GSM_Return GSM_cmdMediaControl(
    void   *arg_ptr,
    GSM_Id *gsmId_ptr,
    vint    callIdx,
    vint    negStatus,
    char   *sdpMd_ptr)
{
    char buffer[GSM_SCRATCH_BUFFER_SZ + 1];

    /*
     * AT+CCMMD=<ccidx>,<neg_status>,[,<SDP_md>]
     */
    if (NULL == sdpMd_ptr) {
        OSAL_snprintf(buffer, GSM_SCRATCH_BUFFER_SZ, "%s=%d,%d",
                _GSM_CMD_CCMMD, callIdx, negStatus);
    }
    else {
        OSAL_snprintf(buffer, GSM_SCRATCH_BUFFER_SZ, "%s=%d,%d,\"%s\"",
                _GSM_CMD_CCMMD, callIdx, negStatus, sdpMd_ptr);
    }
    return (GSM_atCmd(buffer, gsmId_ptr, GSM_CMD_TO_SECS));
}
