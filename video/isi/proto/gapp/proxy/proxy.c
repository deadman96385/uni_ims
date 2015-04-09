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
#include <errno.h>
#include <string.h>
#include <osal_types.h>
#include <osal.h>
#include <osal_msg.h>
#include <osal_log.h>

#include <csm_event.h>

#include "proxy.h"
#include "proxy_io.h"
#include "_proxy_ut.h"
#include "_cmd_mngr.h"
#ifndef GAPP_DISABLE_GSM 
#include <gsm.h>
#endif

/* Global pointer to the csm evt queue  */
OSAL_MsgQId _PRXY_csm_q = 0;

OSAL_Status PRXY_init(
    const char *proxyName_ptr)
{

    /* Create csm input event q */
    if (0 == (_PRXY_csm_q = OSAL_msgQCreate(CSM_INPUT_EVENT_QUEUE_NAME,
            OSAL_MODULE_GAPP, OSAL_MODULE_CSM_PUBLIC,
            OSAL_DATA_STRUCT_CSM_InputEvent,
            CSM_INPUT_EVENT_MSGQ_LEN,
            sizeof(CSM_InputEvent), 0))) {
        return (OSAL_FAIL);
    }
    return (PRXY_ioInit(proxyName_ptr));
    }

void PRXY_destroy(
    void)
{
    PRXY_ioDestroy();
    /* Delete csm q */
    if (0 != _PRXY_csm_q) {
        OSAL_msgQDelete(_PRXY_csm_q);
        }
    }

/*
 * ======== PRXY_generateNetworkRegEvent() ========
 *
 * Private function to generate network registration status according to IMS
 * registration status.
 *
 * RETURN:
 *   None
 */
static void PRXY_generateNetworkRegEvent(
    PRXY_CommandMngr *cmdMngr_ptr)
{
    PRXY_NetRegState regStatus;

    regStatus = cmdMngr_ptr->networkReg.csmState;

    /* Lac and cid is required for mode 2 */
    if ((2 == cmdMngr_ptr->networkReg.mode) && 
            ((PRXY_NET_REG_STATE_ACTIVE == regStatus) ||
            (PRXY_NET_REG_STATE_ROAMING == regStatus))) {
         OSAL_snprintf(cmdMngr_ptr->scratch, PRXY_AT_COMMAND_SIZE,
                "%s: %d,%d,%s,%s%s",
                PRXY_AT_RESPONSE_CREG, cmdMngr_ptr->networkReg.mode,
                regStatus, cmdMngr_ptr->networkReg.locationAreaCode,
                cmdMngr_ptr->networkReg.cellId, PRXY_AT_RESPONSE_CR_LN);
}
    else {
        /* No lac and cid is needed */
        OSAL_snprintf(cmdMngr_ptr->scratch, PRXY_AT_COMMAND_SIZE, "%s: %d,%d%s",
            PRXY_AT_RESPONSE_CREG,
            cmdMngr_ptr->networkReg.mode, regStatus, PRXY_AT_RESPONSE_CR_LN);
}

    PRXY_dbgPrintf("Network registration event:%s\n", cmdMngr_ptr->scratch);
}


/*
 * ======== _PRXY_writeCsmEvent ========
 *
 * This private helper routine is used to write an event to the CSM 
 * Input Event queue
 *
 * RETURN:
 *     PRXY_RETURN_OK: if the event was processed
 *     PRXY_RETURN_WAIT: wait for a solicited response
 *     PRXY_RETURN_CONTINUE: continue on handling all event types
 */
static PRXY_Return _PRXY_writeCsmEvent(
    const CSM_InputEvent *csmEvt_ptr)
{
    OSAL_Status ret;

    if (0 != _PRXY_csm_q) {
        ret = OSAL_msgQSend(_PRXY_csm_q, (void *)csmEvt_ptr, sizeof(CSM_InputEvent), 
                OSAL_NO_WAIT, NULL);
        
        if (OSAL_SUCCESS == ret) {
            return (PRXY_RETURN_OK);
        }
    }
    PRXY_dbgPrintf("%s: Failed to write command\n", __FUNCTION__);
    return (PRXY_RETURN_FAILED);
}

/*
 * ======== PRXY_processAtCommand ========
 *
 * This public routine is used to process an AT command from the application
 *
 * RETURN:
 *     PRXY_RETURN_OK: if the event was processed
 *     PRXY_RETURN_WAIT: wait for a solicited response
 *     PRXY_RETURN_CONTINUE: continue on handling all event types
 */
PRXY_Return PRXY_processAtCommand(
    PRXY_CommandMngr *cmdMngr_ptr,
    char             *at_ptr)
{
    /* First check if it's a SMS command. */
    vint ret = -1; 

    PRXY_printAT("D2AtLog_Input", at_ptr); 
    ret = _PRXY_cmdMngrParseSmsAtCommand(cmdMngr_ptr, at_ptr, 
            &cmdMngr_ptr->csmCommand);
    if (PRXY_RETURN_OK == ret) {
        //Clear buffer before receiving new message content.
        cmdMngr_ptr->csmCommand.evt.sms.message[0] = '\0';
        /* 
         * Than it was processed. Send the prompt and return 'OK' indicating 
         * that we are done with it. 
         */
        OSAL_snprintf(cmdMngr_ptr->scratch, PRXY_AT_COMMAND_SIZE, "%s", "> \r");
        PRXY_dbgPrintf("%s %d: writing %s\n", __FUNCTION__, __LINE__, 
                cmdMngr_ptr->scratch);
        PRXY_ioWrite(cmdMngr_ptr->scratch, OSAL_strlen(cmdMngr_ptr->scratch));
        return (PRXY_RETURN_OK);
    }
    else if (PRXY_RETURN_WAIT == ret) {
        if (PRXY_RETURN_OK != _PRXY_writeCsmEvent(&cmdMngr_ptr->csmCommand)) {
            /* couldn't write the event, let's send back an error. */
            PRXY_dbgPrintf("%s %d: Failed to write command\n", __FUNCTION__,
                    __LINE__);
            /* Let's write an error back. */
            OSAL_snprintf(cmdMngr_ptr->scratch, PRXY_AT_COMMAND_SIZE, "%s %d", 
                    PRXY_AT_RESPONSE_ERROR, 0);
            PRXY_ioWrite(cmdMngr_ptr->scratch, OSAL_strlen(cmdMngr_ptr->scratch));
            /* Return OK indicating that we are done processing the request. */
            return (PRXY_RETURN_OK);
        }
        return (PRXY_RETURN_WAIT);
    }
    else if (PRXY_RETURN_CONTINUE == ret) {
        /* Waiting for more PDU data */
        return (PRXY_RETURN_OK);
    }

    if (PRXY_RETURN_OK == _PRXY_cmdMngrParseSupSrvAtCommand(at_ptr,
            &cmdMngr_ptr->csmCommand)){
        PRXY_AccessDomain targetDomain = _PRXY_getSupSrvAccessDomain(cmdMngr_ptr);
        
        if (PRXY_CS_DOMAIN == targetDomain) {
#ifndef GAPP_DISABLE_GSM            
            /* radio policy determined it should be passed to CS domain */
            if (GSM_RETURN_OK != GSM_atCmd(at_ptr, 
                    &cmdMngr_ptr->passThruCommandId, 
                    PRXY_AT_COMMAND_TIMEOUT_SECS)) {
                PRXY_dbgPrintf("%s %d: Failed to write AT\n", __FUNCTION__, __LINE__);

                /* Let's write an error back. */
                OSAL_snprintf(cmdMngr_ptr->scratch, PRXY_AT_COMMAND_SIZE, "%s %d", 
                        PRXY_AT_RESPONSE_ERROR, 0);
                PRXY_ioWrite(cmdMngr_ptr->scratch, OSAL_strlen(cmdMngr_ptr->scratch));
                /* Return OK indicating that we are done processing the request. */
                return (PRXY_RETURN_OK);
            }
            PRXY_dbgPrintf("%s %d: supsrv 'pass-thru' to AT Device id:%d %s\n",
                __FUNCTION__, __LINE__, cmdMngr_ptr->passThruCommandId, at_ptr);
#else
            /* should not be here in production build, only for testing purpose */
            PRXY_dbgPrintf("Platform didn't support gapp gsm but supsrv uses cs domain");
            return (PRXY_RETURN_OK);
#endif
        }
        else if (PRXY_PS_DOMAIN == targetDomain) {
            /* radio policy determined to use PS domain Ut/XCAP protocol */
    
            if (PRXY_RETURN_OK != _PRXY_writeCsmEvent(&cmdMngr_ptr->csmCommand)) {
                /* couldn't write the event, let's send back an error. */
                PRXY_dbgPrintf("%s %d: Failed to write AT\n", __FUNCTION__, __LINE__);
                /* Let's write an error back. */
            OSAL_snprintf(cmdMngr_ptr->scratch, PRXY_AT_COMMAND_SIZE, "%s %d", PRXY_AT_RESPONSE_ERROR, 0);
            PRXY_ioWrite(cmdMngr_ptr->scratch, OSAL_strlen(cmdMngr_ptr->scratch));
                /* Returned 'OK' indicating that we are done processing the request. */
                return (PRXY_RETURN_OK);
            }
        }
        else {
            /* no domain attached and won't be able to issue supsrv config */
            PRXY_dbgPrintf("%s %d: not cs/ps:%d\n", __FUNCTION__, __LINE__, targetDomain);
            /* Let's write an error back. */
            OSAL_snprintf(cmdMngr_ptr->scratch, PRXY_AT_COMMAND_SIZE, "%s %d", 
                    PRXY_AT_RESPONSE_ERROR, 0);
            PRXY_ioWrite(cmdMngr_ptr->scratch, OSAL_strlen(cmdMngr_ptr->scratch));
            /* Return OK indicating that we are done processing the request. */
            return (PRXY_RETURN_OK);
        }
        /* wait for the CS/PS stack processing result */
        return (PRXY_RETURN_WAIT);
    }

    /* Check if it is network registration command. */
    if (PRXY_RETURN_OK == _PRXY_cmdMngrParseNetworkRegAtCommand(
            cmdMngr_ptr, at_ptr)) {
#ifndef GAPP_DISABLE_GSM            
        if (GSM_RETURN_OK != GSM_atCmd(at_ptr, 
                &cmdMngr_ptr->nonPassThruCommandId, PRXY_AT_COMMAND_TIMEOUT_SECS)) {
            PRXY_dbgPrintf("%s %d: Failed to write command\n", __FUNCTION__, __LINE__);

            /* Let's consider there is no GSM modem and return IMS status. */
            PRXY_generateNetworkRegEvent(cmdMngr_ptr);
            PRXY_ioWrite(cmdMngr_ptr->scratch, OSAL_strlen(cmdMngr_ptr->scratch));
            /* Returned 'OK' indicating that we are done processing the request. */
            return (PRXY_RETURN_OK);
        }
        return (PRXY_RETURN_WAIT);
#else        
        /* Let's consider there is no GSM modem and return IMS status. */
        PRXY_generateNetworkRegEvent(cmdMngr_ptr);
        PRXY_ioWrite(cmdMngr_ptr->scratch, OSAL_strlen(cmdMngr_ptr->scratch));
        /* Returned 'OK' indicating that we are done processing the request. */
        return (PRXY_RETURN_OK);
#endif
    }

    /* Check if it is network registration command. */
    if (PRXY_RETURN_OK == _PRXY_cmdMngrParseMediaProfileAtCommand(
            cmdMngr_ptr, at_ptr, &cmdMngr_ptr->csmCommand)) {
        /* Send out a csm event*/
        _PRXY_writeCsmEvent(&cmdMngr_ptr->csmCommand);
        /* Write response. */
        PRXY_ioWrite(cmdMngr_ptr->scratch, OSAL_strlen(cmdMngr_ptr->scratch));
        /* Returned 'OK' indicating that we are done processing the request. */
        return (PRXY_RETURN_OK);
    }

    /* Check if it is ussd command. */
    ret = _PRXY_cmdMngrParseUssdAtCommand(cmdMngr_ptr, at_ptr, 
            &cmdMngr_ptr->csmCommand);
    if (PRXY_RETURN_OK == ret) {
        if (PRXY_RETURN_OK != _PRXY_writeCsmEvent(&cmdMngr_ptr->csmCommand)) {
            /* couldn't write the event, let's send back an error. */
            PRXY_dbgPrintf("%s %d: Failed to write command\n", __FUNCTION__, 
                    __LINE__);
            /* Let's write an error back. */
            OSAL_snprintf(cmdMngr_ptr->scratch, PRXY_AT_COMMAND_SIZE, "%s %d", 
                    PRXY_AT_RESPONSE_ERROR, 0);
            PRXY_ioWrite(cmdMngr_ptr->scratch, OSAL_strlen(cmdMngr_ptr->scratch));
        }
        return (PRXY_RETURN_OK);
    }
    else if (PRXY_RETURN_CONTINUE == ret) {
        OSAL_snprintf(cmdMngr_ptr->scratch, PRXY_AT_COMMAND_SIZE, "%s%s", 
                        PRXY_AT_RESPONSE_STR_OK, PRXY_AT_RESPONSE_CR_LN);
        PRXY_ioWrite(cmdMngr_ptr->scratch, OSAL_strlen(cmdMngr_ptr->scratch));
        return (PRXY_RETURN_OK);
    }

    /* Check if it is service command */
    if (PRXY_RETURN_OK == _PRXY_cmdMngrParseServiceAtCommand(at_ptr,
            &cmdMngr_ptr->csmCommand)) {
        /* Then it's a command for CSM. */
        if (PRXY_RETURN_OK != _PRXY_writeCsmEvent(&cmdMngr_ptr->csmCommand)) {
            /* couldn't write the event, let's send back an error. */
            PRXY_dbgPrintf("%s %d: Failed to write command\n", __FUNCTION__,
                    __LINE__);
            /* Let's write an error back. */
            OSAL_snprintf(cmdMngr_ptr->scratch, PRXY_AT_COMMAND_SIZE, "%s %d", 
                    PRXY_AT_RESPONSE_ERROR, 0);
            PRXY_ioWrite(cmdMngr_ptr->scratch, OSAL_strlen(cmdMngr_ptr->scratch));
            /* Return OK indicating that we are done processing the request. */
            return (PRXY_RETURN_OK);
        }
        /* Send OK for service at command */
        OSAL_snprintf(cmdMngr_ptr->scratch, PRXY_AT_COMMAND_SIZE, "%s%s", 
                PRXY_AT_RESPONSE_STR_OK, PRXY_AT_RESPONSE_CR_LN);
        PRXY_ioWrite(cmdMngr_ptr->scratch, OSAL_strlen(cmdMngr_ptr->scratch));
        PRXY_dbgPrintf("%s %s: Wrote to CSM %s\n",
                IR92_DEBUG_TAG, __FUNCTION__, at_ptr);
        return (PRXY_RETURN_OK);
    }

    /* Check if it is radio command */
    if (PRXY_RETURN_OK == _PRXY_cmdMngrParseRadioAtCommand(at_ptr,
            &cmdMngr_ptr->csmCommand)) {
        /* Then it's a command for CSM. */
        if (PRXY_RETURN_OK != _PRXY_writeCsmEvent(&cmdMngr_ptr->csmCommand)) {
            /* couldn't write the event, let's send back an error. */
            PRXY_dbgPrintf("%s %d: Failed to write command\n", __FUNCTION__,
                    __LINE__);
            /* Let's write an error back. */
            OSAL_snprintf(cmdMngr_ptr->scratch, PRXY_AT_COMMAND_SIZE, "%s %d", 
                    PRXY_AT_RESPONSE_ERROR, 0);
            PRXY_ioWrite(cmdMngr_ptr->scratch, OSAL_strlen(cmdMngr_ptr->scratch));
            /* Return OK indicating that we are done processing the request. */
            return (PRXY_RETURN_OK);
        }
        /* Send OK for service at command */
        OSAL_snprintf(cmdMngr_ptr->scratch, PRXY_AT_COMMAND_SIZE, "%s%s", 
                PRXY_AT_RESPONSE_STR_OK, PRXY_AT_RESPONSE_CR_LN);
        PRXY_ioWrite(cmdMngr_ptr->scratch, OSAL_strlen(cmdMngr_ptr->scratch));
        PRXY_dbgPrintf("%s %s: Wrote to CSM %s\n",
                IR92_DEBUG_TAG, __FUNCTION__, at_ptr);
        return (PRXY_RETURN_OK);
    }

    /* 
     * Else it must have been 'failed' which means that the SMS 
     * processor is not interested. 
     */
    if (PRXY_RETURN_OK != _PRXY_cmdMngrParseAtCommand(cmdMngr_ptr, at_ptr, 
            &cmdMngr_ptr->csmCommand)) {
#ifndef GAPP_DISABLE_GSM            
        if (GSM_RETURN_OK != GSM_atCmd(at_ptr, &cmdMngr_ptr->passThruCommandId,
                PRXY_AT_COMMAND_TIMEOUT_SECS)) {
            PRXY_dbgPrintf("%s %d: Failed to write command\n", __FUNCTION__, 
                    __LINE__);
            /* Let's write an error back. */
            OSAL_snprintf(cmdMngr_ptr->scratch, PRXY_AT_COMMAND_SIZE, "%s %d", 
                    PRXY_AT_RESPONSE_ERROR, 0);
            PRXY_ioWrite(cmdMngr_ptr->scratch, OSAL_strlen(cmdMngr_ptr->scratch));
            /* Return OK indicating that we are done processing the request. */
            return (PRXY_RETURN_OK);
        }
        PRXY_dbgPrintf("%s %s: Wrote 'pass-thru' to AT Device id:%d %s\n",
                IR92_DEBUG_TAG, __FUNCTION__, cmdMngr_ptr->passThruCommandId, 
                at_ptr);
#else
    /* Let's write an error back. */
        OSAL_snprintf(cmdMngr_ptr->scratch, PRXY_AT_COMMAND_SIZE, "%s %d",
                PRXY_AT_RESPONSE_ERROR, 0);
        PRXY_ioWrite(cmdMngr_ptr->scratch, OSAL_strlen(cmdMngr_ptr->scratch));
        /* Return OK indicating that we are done processing the request. */
        return (PRXY_RETURN_OK);
#endif
    }
    else {
        /* Then it's a command for CSM. */
        if (PRXY_RETURN_OK != _PRXY_writeCsmEvent(&cmdMngr_ptr->csmCommand)) {
            /* couldn't write the event, let's send back an error. */
            PRXY_dbgPrintf("%s %d: Failed to write command\n", __FUNCTION__,
                    __LINE__);
            /* Let's write an error back. */
            OSAL_snprintf(cmdMngr_ptr->scratch, PRXY_AT_COMMAND_SIZE, "%s %d", 
                    PRXY_AT_RESPONSE_ERROR, 0);
            PRXY_ioWrite(cmdMngr_ptr->scratch, OSAL_strlen(cmdMngr_ptr->scratch));
            /* Return OK indicating that we are done processing the request. */
            return (PRXY_RETURN_OK);
        }
        PRXY_dbgPrintf("%s %s: Wrote to CSM %s\n",
                IR92_DEBUG_TAG, __FUNCTION__, at_ptr);
    }
    /* Return 'wait' so the AT looper waits for a response for this command. */
    return (PRXY_RETURN_WAIT);
}

/*
 * ======== PRXY_processCsmOutputEvent ========
 *
 * This public routine is used to process a CSM Output event
 *
 * RETURN:
 *     PRXY_RETURN_OK: if the event was processed
 *     PRXY_RETURN_WAIT: wait for a solicited response
 *     PRXY_RETURN_CONTINUE: continue on handling all event types
 */
PRXY_Return PRXY_processCsmOutputEvent(
    PRXY_CommandMngr *cmdMngr_ptr,
    CSM_OutputEvent  *event_ptr)
{
    const CSM_OutputService *service_ptr;
    CSM_InputCall           *call_ptr;

    PRXY_dbgPrintf("%s %d\n", __FUNCTION__, __LINE__);
    /*
     * Perform the following...
     * 0) Check if this is IntExt table update/init event
     * 1) Check if the response is a response to a command.
     * 2) If yes then then return 'OK' to start processing the next command.
     * 3) If the response is not a response to a command then just send 
     *    it and return.
     */
    if (PRXY_RETURN_OK == _PRXY_cmdMngrConstructAtEvent(cmdMngr_ptr, event_ptr, 
            cmdMngr_ptr->scratch, PRXY_AT_COMMAND_SIZE)) {
        /* 
         * Then it's an 'event' from CSM, just pass it up and then return, 
         * no further processing is needed. 
     */
        PRXY_ioWrite(cmdMngr_ptr->scratch, OSAL_strlen(cmdMngr_ptr->scratch));
        PRXY_dbgPrintf("%s %s: Wrote event to RILD %s\n",
                IR92_DEBUG_TAG, __FUNCTION__, cmdMngr_ptr->scratch);
        /*
         * Return 'CONTINUE' indicating that we processed the event it does
         * not affect the handling of existing commands.
         */
        return (PRXY_RETURN_CONTINUE);
    }
    /* Otherwise let's see if it's a result to a previous command. */
    if (PRXY_RETURN_OK == _PRXY_cmdMngrConstructAtResult(cmdMngr_ptr,
            event_ptr, cmdMngr_ptr->scratch, PRXY_AT_COMMAND_SIZE)) {

        PRXY_ioWrite(cmdMngr_ptr->scratch, OSAL_strlen(cmdMngr_ptr->scratch));
        PRXY_dbgPrintf("%s %s: Wrote cmd result to RILD %s\n",
                    IR92_DEBUG_TAG, __FUNCTION__, cmdMngr_ptr->scratch);
        /*
         * Return 'OK' indicating that we process the event and we 
         * should process more commands if they exist.
         */
        return (PRXY_RETURN_OK);
    }

    /*
     * Cache CSM network registration status.
     * If CSM registration is changed, process it in GAPP 
     * to do mix with modem network registration status.
     */
    if (CSM_EVENT_TYPE_SERVICE == event_ptr->type) {
        service_ptr = &event_ptr->evt.service;
        switch (service_ptr->reason) {
            case CSM_OUTPUT_REASON_SERVICE_STATE:
                cmdMngr_ptr->networkReg.csmState       = service_ptr->state;
                cmdMngr_ptr->networkReg.isCsmRegChange = OSAL_TRUE;
                cmdMngr_ptr->networkReg.isEmergency = service_ptr->isEmergency;
                PRXY_dbgPrintf("%s %d reasonDesc:%s\n", __FUNCTION__, __LINE__,
                        service_ptr->reasonDesc);
                return (PRXY_RETURN_CONTINUE);
            default:
                break;
        }
    }

    /* Check for call initializing event */
    if ((CSM_EVENT_TYPE_CALL == event_ptr->type) &&
            (CSM_OUTPUT_REASON_INITIALIZING_EVENT ==
            event_ptr->evt.call.reason)) {
        /*
         * Call initializing. It's an incoming call but the resource is not
         * ready. Since GAPP don't do delicate bearer establishment, so let's
         * simulate a resource ready event to indicate CSM the resource is
         * ready.
         */
        cmdMngr_ptr->csmCommand.type = CSM_EVENT_TYPE_CALL;
        call_ptr = &cmdMngr_ptr->csmCommand.evt.call;
        call_ptr->reason = CSM_CALL_REASON_RESOURCE_INDICATION;
        call_ptr->u.resourceStatus.callId =
                event_ptr->evt.call.u.resourceMedia.callId;
        call_ptr->u.resourceStatus.videoReady = 1;
        call_ptr->u.resourceStatus.audioReady = 1;
        /* Send to CSM */ 
        _PRXY_writeCsmEvent(&cmdMngr_ptr->csmCommand);
    } 
    return (PRXY_RETURN_WAIT);
}

/*
 * ======== PRXY_processGsmEvent ========
 *
 * This public routine is used to process a GSM Event
 *
 * RETURN:
 *     PRXY_RETURN_OK: if the event was processed
 *     PRXY_RETURN_WAIT: otherwise
 */
PRXY_Return PRXY_processGsmEvent(
    PRXY_CommandMngr *cmdMngr_ptr,
    GSM_Id            gsmId,
    char             *result_ptr,
    vint              resultLen)
{
    if ((PRXY_INVALID_COMMAND_ID != gsmId) && 
            (gsmId == cmdMngr_ptr->passThruCommandId)) {
        /* Send it up to the application. */
        PRXY_ioWrite(result_ptr, resultLen);
        PRXY_dbgPrintf("%s %s: Wrote 'pass-thru' to RILD id:%d %s\n",
                IR92_DEBUG_TAG, __FUNCTION__, gsmId, result_ptr);

        /* Clear the ID. */
        cmdMngr_ptr->passThruCommandId = PRXY_INVALID_COMMAND_ID;
        return (PRXY_RETURN_OK);
    }

    if ((PRXY_INVALID_COMMAND_ID != gsmId) && 
            (gsmId == cmdMngr_ptr->nonPassThruCommandId)) {
        if (NULL != OSAL_strscan(result_ptr, PRXY_AT_RESPONSE_CREG)) {
            /*
             * If GSM event is the response of 'AT+CREG?', 
             * do mix status with CSM network registration in GAPP.
             */
            return (PRXY_RETURN_CONTINUE);
        }
    }

    return (PRXY_RETURN_WAIT);
}


/* 
 * ======== _PRXY_getSupSrvAccessDomain() ========
 * This function query which access domain (CS/PS) to use for supsrv config.
 *
 * Return Values:
 * The result is the radio technology CS/PS
 */
PRXY_AccessDomain _PRXY_getSupSrvAccessDomain(
    PRXY_CommandMngr *cmdMngr_ptr)
{
#if defined(GAPP_DISABLE_SUPPLEMENTARY_SERVICE_AT_CMD)
    /* this platform don't have supsrv radio policy and always use CS domain */
    PRXY_dbgPrintf("%s %d supsrv to use CS\n", __FUNCTION__, __LINE__);
    return (PRXY_CS_DOMAIN);
#elif defined(PROVIDER_CMCC)
    /* CMCC policy */
    if (PRXY_NET_REG_STATE_ACTIVE == cmdMngr_ptr->networkReg.csmState) {
        PRXY_dbgPrintf("%s %d supsrv to use PS\n", __FUNCTION__, __LINE__);
        return (PRXY_PS_DOMAIN);
    }
    else if (PRXY_NET_REG_STATE_ACTIVE == cmdMngr_ptr->networkReg.gsmState) {
        PRXY_dbgPrintf("%s %d supsrv to use CS\n", __FUNCTION__, __LINE__);
        return (PRXY_CS_DOMAIN);
    }
    else {
        PRXY_dbgPrintf("%s %d supsrv to use NULL\n", __FUNCTION__, __LINE__);
        return (PRXY_NULL_DOMAIN);
    }
#else
    /* default and generic policy, try to use PS whenever possible */
    return (PRXY_PS_DOMAIN);
#endif
}
