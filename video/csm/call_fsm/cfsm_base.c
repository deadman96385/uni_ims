/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2012 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Revision: 30280 $ $Date: 2014-12-09 14:24:57 +0800 (Tue, 09 Dec 2014) $
 */
#include <rpm.h>
#include "_csm_event.h"
#include "_csm_isi.h"
#include "cfsm.h"
#include "_cfsm.h"
#include "_csm.h"
#include "_csm_isi_call.h"
#include "_csm_response.h"

/*
 * ======== CFSM_init ========
 * Initialize an instance of the state machine.
 *
 * Return Values:
 * CFSM_Context - an initialize context for a new state machine.
 */
CFSM_Context_Ptr CFSM_init(
    CFSM_Context_Ptr   context_ptr,
    CSM_OutputEvent   *csmOutput_ptr)
{
    context_ptr->csmOutput_ptr = csmOutput_ptr;
    context_ptr->active         = OSAL_FALSE;
    context_ptr->isIpConference = OSAL_FALSE;
    context_ptr->isEmergency  = OSAL_FALSE;
    context_ptr->lastErrCode = -1;
    /* Start in RESET state */
    context_ptr->currentState_ptr = (CFSM_State_Ptr)(&_CFSM_STATE_RESET);
    /* Creat ringing timer and its sending message timer. */
    if (0 == context_ptr->timerId) {
        if (0 == (context_ptr->timerId = OSAL_tmrCreate())) {
            return (NULL);
        }
    }
    else {
        /* Make sure time is stopped. */
        OSAL_tmrStop(context_ptr->timerId);
    }
    if (0 == context_ptr->retryTmrId) {
        if (0 == (context_ptr->retryTmrId = OSAL_tmrCreate())) {
            return (NULL);
        }
    }
    else {
        /* Make sure time is stopped. */
        OSAL_tmrStop(context_ptr->retryTmrId);
    }

    return context_ptr;
}

/*
 * ======== CFSM_destroy ========
 * Shutdown an instance of the state machine.
 *
 * Return Values:
 *    none
 */
void CFSM_destroy(
    CFSM_Context_Ptr context_ptr)
{
    OSAL_tmrDelete(context_ptr->timerId);
    OSAL_tmrDelete(context_ptr->retryTmrId);
}

static OSAL_Boolean _CFSM_shouldProcessEvent(
    CFSM_Context_Ptr  context_ptr,
    CSM_EventCall_Ptr event_ptr)
{
    if (CSM_CALL_EVENT_TYPE_ISI == event_ptr->type) {
        if (event_ptr->id == context_ptr->callId) {
            return (OSAL_TRUE);
        }
        return (OSAL_FALSE);
    }
    return (OSAL_TRUE);
}

/*
 * ======== CFSM_processEvent ========
 * Process the incoming event. This function handles any pre or post event
 * processing. In addition, it calls the process event function associated
 * with the current state.
 *
 * Return Values:
 * none
 */
void CFSM_processEvent(
    CFSM_Context_Ptr  context_ptr,
    CSM_EventCall_Ptr event_ptr)
{
    if (OSAL_TRUE == _CFSM_shouldProcessEvent(context_ptr, event_ptr)) {
        context_ptr->currentState_ptr->processEvent(context_ptr, event_ptr);
    }
    else if ((CSM_CALL_EVT_REASON_SERVICE_ACTIVE == event_ptr->reason) ||
            (CSM_CALL_EVT_REASON_SERVICE_INACTIVE == event_ptr->reason)) {
        context_ptr->currentState_ptr->processEvent(context_ptr, event_ptr);
    }
    else if (CSM_CALL_EVT_REASON_DISCONNECT == event_ptr->reason) {
        /*
         * CSM_CALL_EVT_REASON_DISCONNECT events not for this context
         * should be processed as CSM_CALL_EVT_REASON_OTHER_DISCONNECT.
         * This is because some call states need to know when another 
         * call has disconnected.
         */
        event_ptr->reason = CSM_CALL_EVT_REASON_OTHER_DISCONNECT;
        context_ptr->currentState_ptr->processEvent(context_ptr, event_ptr);
        event_ptr->reason = CSM_CALL_EVT_REASON_DISCONNECT;
    }
}

/*
 * ======== _CFSM_stateEnter ========
 * Enter the new state. If a function is available that is run when entering
 * the state, it is run by this function. Any common code that is run before
 * or after the function can be run at this time.
 *
 * Return Values:
 * none
 */
static void _CFSM_stateEnter(
    CFSM_Context_Ptr  context_ptr,
    CSM_EventCall_Ptr event_ptr)
{
    CSM_dbgPrintf("entering state %s\n", _CFSM_toString(context_ptr));
    if (context_ptr->currentState_ptr->stateEnter != NULL) {
        context_ptr->currentState_ptr->stateEnter(context_ptr, event_ptr);
    }
}

/*
 * ======== _CFSM_stateExit ========
 * Exit the current state. If a function is available that is run when exiting
 * the state, it is run by this function. Any common code that is run before or
 * after the function can be run at this time.
 *
 * Return Values:
 * none
 */
static void _CFSM_stateExit(
    CFSM_Context_Ptr  context_ptr,
    CSM_EventCall_Ptr event_ptr)
{
    CSM_dbgPrintf("exiting state %s\n", _CFSM_toString(context_ptr));
    if (context_ptr->currentState_ptr->stateExit != NULL) {
        context_ptr->currentState_ptr->stateExit(context_ptr, event_ptr);
    }
}

/*
 * ======== _CFSM_clearNegExchange() ========
 * Clear negotiation exchange status once it's reported.
 *
 * Return:
 *  None.
 */
static void _CFSM_clearNegExchange(
    CFSM_Context_Ptr  context_ptr)
{
    CSM_CallObject   *call_ptr;

    /*
     * Get call object.
     * The callId might be zerop when an outgoing call is
     * made but haven't get call id from ISI. CSM_getCall() will get the call
     * with the callId zero.
     */
    call_ptr = CSM_getCall(context_ptr->callId);
    if ((CSM_CALL_NEG_EXCHANGE_REMOTE_PROPOSED == call_ptr->negExchange) ||
            (CSM_CALL_NEG_EXCHANGE_REMOTE_ACCEPTED == call_ptr->negExchange) ||
            (CSM_CALL_NEG_EXCHANGE_REMOTE_REJECTED == call_ptr->negExchange) ||
            (CSM_CALL_NEG_EXCHANGE_MEDIA_CHANGED == call_ptr->negExchange)) {
        /* Those status need to be report once, clear it. */
        call_ptr->negExchange = CSM_CALL_NEG_EXCHANGE_NONE;
    }
}

/*
 * ======== _CFSM_setState ========
 * This function is used by to change the state of the state machine. It first
 * checks to see if a change occurred. If it did, it runs the state exit
 * function associated with the current state. Then it changes the state and
 * runs the state enter function of the new state.
 *
 * Return Values:
 * none
 */
void _CFSM_setState(
    CFSM_Context_Ptr  context_ptr,
    CFSM_State_Ptr    nextState_ptr,
    CSM_EventCall_Ptr event_ptr)
{
    CSM_GlobalObj    *_CSM_globalObj_ptr;
    CSM_OutputEvent  *csmOutput_ptr;
    
    /*
     * Check if the state has changed
     */
    if (context_ptr->currentState_ptr != nextState_ptr) {
        /* Exist previous state */
        _CFSM_stateExit(context_ptr, event_ptr);
        /* Set new state */
        context_ptr->currentState_ptr = nextState_ptr;
        /* Enter new state*/
        _CFSM_stateEnter(context_ptr, event_ptr);
        /* Send a call report while call state changed. */
        _CSM_globalObj_ptr = CSM_getObject();
        csmOutput_ptr = &_CSM_globalObj_ptr->queue.csmOutputEvent;
        /* If current state is INITIALIZING, should not report CLCC */
        if ((&_CFSM_STATE_INITIALIZING != context_ptr->currentState_ptr) && 
                (&_CFSM_STATE_HANGINGUP != context_ptr->currentState_ptr)) {
            _CSM_generateMonitorReport(&_CSM_globalObj_ptr->evt.call,
                    csmOutput_ptr);
        }
        /* Clear negExchange after the status reported. */    
        _CFSM_clearNegExchange(context_ptr);
    }
}

/*
 * ======== _CFSM_toString ========
 * This function is a helper to get the state name string.
 *
 * Return Values:
 * none
 */
const char *_CFSM_toString(
    CFSM_Context_Ptr context_ptr)
{
    return (context_ptr->currentState_ptr->stateName_ptr);
}

static CSM_CallParticipants* _searchParticipants(
    CSM_CallObject *call_ptr,
    const char     *participant_ptr)
{
    CSM_CallParticipants *p_ptr;
    vint x;
    /* Find an entry for this participant. */
    for (x = 0; x < CSM_EVENT_MAX_CALL_LIST_SIZE; x++) {
        p_ptr = &call_ptr->participants[x];
        if (0 != p_ptr->callIndex) {
            if (0 == OSAL_strcmp(p_ptr->number, participant_ptr)) {
                return (p_ptr);
            }
        }
    }
    return (NULL);
}

static void _insertParticipant(
    CSM_CallObject       *call_ptr,
    CSM_Address          *participant_ptr)
{
    CSM_CallParticipants *p_ptr;
    vint x;
    /* Find an empty entry for this participant. */
    for (x = 0; x < CSM_EVENT_MAX_CALL_LIST_SIZE; x++) {
        p_ptr = &call_ptr->participants[x];
        if (0 == p_ptr->callIndex) {
            /* copy in the new participant info. */
            p_ptr->type = participant_ptr->type;
            OSAL_strncpy(p_ptr->number, participant_ptr->number, CSM_NUMBER_STRING_SZ);
            OSAL_strncpy(p_ptr->alpha, participant_ptr->alpha, CSM_ALPHA_STRING_SZ);
            /* Generate a callIndex for this participant. */
            p_ptr->callIndex = CSM_getCallIndex();
            p_ptr->numberType = participant_ptr->numberType;
            return;
        }
    }
    return;
}

static vint _removeParticipant(
    CSM_CallObject        *call_ptr,
    CSM_Address           *participant_ptr)
{
    CSM_CallParticipants *p_ptr = _searchParticipants(
            call_ptr, participant_ptr->number);
    if (NULL != p_ptr) {
        /* Found the participant to remove. */
        OSAL_logMsg("%s %s: removing participant:%s index:%d",
                IR92_DEBUG_TAG, __FUNCTION__, p_ptr->number, p_ptr->callIndex);
        CSM_clearCallIndex(p_ptr->callIndex);
        p_ptr->callIndex = 0;
        return (CSM_OK);
    }
    return (CSM_ERR);
}

static void _addParticipant(
    CSM_CallObject        *call_ptr,
    CSM_Address           *participant_ptr)
{
    CSM_CallParticipants *p_ptr = _searchParticipants(
            call_ptr, participant_ptr->number);
    if (NULL != p_ptr) {
        /* They already exist, no further processing needed. */
        OSAL_logMsg("%s %s: participant:%s index:%d all ready exists",
                IR92_DEBUG_TAG, __FUNCTION__, p_ptr->number, p_ptr->callIndex);
        return;
    }
    OSAL_logMsg("%s %s: inserting a participant:%s",
            IR92_DEBUG_TAG, __FUNCTION__, participant_ptr->number);
    _insertParticipant(call_ptr, participant_ptr);
}

/*
 * ======== _CFSM_getParticipantsCount ========
 * This function is a helper to get the total participant numbers in CC.
 *
 * Return Values:
 *  Total participant numbers in CC.
 */
int _CFSM_getParticipantsCount(
    CSM_CallObject *call_ptr)
{
    CSM_CallParticipants *p_ptr;
    vint x;
    int count = 0;

    for (x = 0; x < CSM_EVENT_MAX_CALL_LIST_SIZE; x++) {
        p_ptr = &call_ptr->participants[x];
        if (0 != p_ptr->callIndex) {
            count++;
        }
    }
    return (count);
}

void _CFSM_processParticipantInfo(
    CFSM_Context_Ptr  context_ptr,
    CSM_EventCall_Ptr event_ptr)
{
    CSM_CallObject* call_ptr;
    /* A participant is reported being added or removed. */
    call_ptr = CSM_getCall(context_ptr->callId);
    if (0 == event_ptr->u.remoteParty.isAdded) {
        /* Then let's removed them from the list of participants. */
        if (CSM_OK == _removeParticipant(
                call_ptr, &event_ptr->u.remoteParty.address)) {
            /* Then some one was removed let's notify. */
            CSM_sendRemoteDisconnect(NULL, context_ptr->csmOutput_ptr);
        }
    }
    else {
        /* Then let's add them. */
        _addParticipant(call_ptr, &event_ptr->u.remoteParty.address);
    }
}

void _CFSM_processGroupParticipantInfo(
    CFSM_Context_Ptr  context_ptr,
    CSM_EventCall_Ptr event_ptr)
{
    vint    notication;
    CSM_OutputCall  *csmOutputCall_ptr = &context_ptr->csmOutput_ptr->evt.call;

    if (0 == event_ptr->u.remoteParty.isAdded) {
        CSM_dbgPrintf("%s %s: removing participant:%s\n",
                IR92_DEBUG_TAG, __FUNCTION__,
                event_ptr->u.remoteParty.address.number);
    } else {
        CSM_dbgPrintf("%s %s: adding participant:%s\n",
                IR92_DEBUG_TAG, __FUNCTION__,
                event_ptr->u.remoteParty.address.number);
    }

    /* Report participant status. */
    notication = context_ptr->supsrvNotication;
    context_ptr->supsrvNotication |= CSM_SUPSRV_CALL_PARTICIPANT_STATUS;
    OSAL_memCpy(csmOutputCall_ptr->u.supsrvInfo.participant.number,
            event_ptr->u.remoteParty.address.number,
            sizeof(csmOutputCall_ptr->u.supsrvInfo.participant.number));
    csmOutputCall_ptr->u.supsrvInfo.participant.status =
            event_ptr->u.remoteParty.status;
    _CSM_generateSupsrvReport(CSM_getCall(context_ptr->callId),
            context_ptr->csmOutput_ptr, NULL);
    context_ptr->supsrvNotication = notication;
}

/*
 * ======== _CFSM_convertCidTypeToIsi() ========
 * Private function to convert cid type from CSM type to ISI type.
 *
 * Return:
 *  ISI_SessionCidType Coverted ISI cid type.
 */
static ISI_SessionCidType _CFSM_convertCidTypeToIsi(
    CSM_CallCidType type)
{
    if (CSM_CALL_CID_TYPE_INVOCATION == type) {
        return (ISI_SESSION_CID_TYPE_INVOCATION);
    }
    else if (CSM_CALL_CID_TYPE_SUPPRESSION == type) {
        return (ISI_SESSION_CID_TYPE_SUPPRESSION);
    }
    else if (CSM_CALL_CID_TYPE_USE_ALIAS == type) {
        return (ISI_SESSION_CID_TYPE_USE_ALIAS);
    }
    return (ISI_SESSION_CID_TYPE_NONE);
}

/*
 * ======== _CFSM_initiateCall ========
 * This function is a helper to initiate a call, set state and send
 * CSM_Responses accordingly
 *
 * Return Values:
 * none
 */
void _CFSM_initiateCall(
    CFSM_Context_Ptr  context_ptr,
    CSM_EventCall_Ptr event_ptr)
{
    ISI_Id          callId;
    int             isiReturnCode;
    int             isiErrorCode;
    int             sessionType;
    CSM_IsiService *service_ptr;
    CSM_CallObject *call_ptr;

    OSAL_logMsg("%s:%d\n", __FUNCTION__, __LINE__);

    service_ptr = CSM_isiGetServiceViaId(context_ptr->isiMngr_ptr,
            context_ptr->serviceId);
    if (context_ptr->isEmergency) {
        sessionType = ISI_SESSION_TYPE_AUDIO | ISI_SESSION_TYPE_EMERGENCY;
        /* Always set cid type to none for emergency failover */
        event_ptr->cidType = CSM_CALL_CID_TYPE_NONE;
    }
    else {
        sessionType = ISI_SESSION_TYPE_AUDIO;
        if (event_ptr->callSessionType & CSM_CALL_SESSION_TYPE_VIDEO) {
            sessionType |= ISI_SESSION_TYPE_VIDEO;
        }        
    }

    if (CSM_TRASPORT_PROTO_RT_MEDIA_SRTP == service_ptr->rtMedia) {
        sessionType |= ISI_SESSION_TYPE_SECURITY_AUDIO;
    }

    isiReturnCode = _CSM_isiInitiateCall(&callId,
            context_ptr->serviceId,
            context_ptr->protoName_ptr,
            event_ptr->u.remoteAddress,
            NULL,
            sessionType, 
            _CFSM_convertCidTypeToIsi(event_ptr->cidType));
    context_ptr->callId = callId;
    if (ISI_RETURN_OK != isiReturnCode) {
        switch (isiReturnCode) {
            case ISI_RETURN_INVALID_SERVICE_ID:
                isiErrorCode = 30;
                break;
            case ISI_RETURN_FAILED:
            case ISI_RETURN_NOT_INIT:
            case ISI_RETURN_MUTEX_ERROR:
            default:
                isiErrorCode = 0;
                break;
        }
        CSM_sendError(isiErrorCode, "ISI Error", context_ptr->csmOutput_ptr);
        _CFSM_setState(context_ptr,
                (CFSM_State_Ptr) &_CFSM_STATE_TERMINATED, event_ptr);
    }
    else {
        context_ptr->callId = callId;
        /* Get call object. */
        call_ptr = CSM_getCall(callId);
        /* Set negExchang and cache session type. */
        _CFSM_setNegExchange(context_ptr,
                CSM_CALL_NEG_EXCHANGE_LOCAL_PROPOSED);
        call_ptr->negSessionType = call_ptr->sessionType = sessionType;
    }
}

/*
 * ======== _CFSM_initiateConfCall ========
 * This function is a helper to initiate a call, set state and send
 * CSM_Responses accordingly
 *
 * Return Values:
 * none
 */
void _CFSM_initiateConfCall(
    CFSM_Context_Ptr  context_ptr,
    CSM_EventCall_Ptr event_ptr,
    char             *rsrcList_ptr)
{
    ISI_Id          callId;
    int             isiReturnCode;
    int             isiErrorCode;
    int             sessionType;
    CSM_IsiService *service_ptr;
    CSM_CallObject *call_ptr;
    char            remoteAddress[CSM_EVENT_STRING_SZ + 1];
    OSAL_snprintf(remoteAddress, 
                            CSM_EVENT_STRING_SZ, "%s", context_ptr->remoteAddress_ptr);

    OSAL_logMsg("%s:%d\n", __FUNCTION__, __LINE__);

    service_ptr = CSM_isiGetServiceViaId(context_ptr->isiMngr_ptr,
            context_ptr->serviceId);
    if (context_ptr->isEmergency) {
        sessionType = ISI_SESSION_TYPE_AUDIO | ISI_SESSION_TYPE_EMERGENCY;
        /* Always set cid type to none for emergency failover */
        event_ptr->cidType = CSM_CALL_CID_TYPE_NONE;
    }
    else {
        sessionType = ISI_SESSION_TYPE_AUDIO;
        if (event_ptr->callSessionType & CSM_CALL_SESSION_TYPE_VIDEO) {
            sessionType |= ISI_SESSION_TYPE_VIDEO;
        }        
    }

    if (CSM_TRASPORT_PROTO_RT_MEDIA_SRTP == service_ptr->rtMedia) {
        sessionType |= ISI_SESSION_TYPE_SECURITY_AUDIO;
    }

    isiReturnCode = _CSM_isiInitiateConfCall(&callId,
            context_ptr->serviceId,
            context_ptr->protoName_ptr,
            remoteAddress,
            context_ptr->cnap_ptr,
            sessionType,
            _CFSM_convertCidTypeToIsi(event_ptr->cidType),
            rsrcList_ptr);

    context_ptr->callId = callId;
    if (ISI_RETURN_OK != isiReturnCode) {
        switch (isiReturnCode) {
            case ISI_RETURN_INVALID_SERVICE_ID:
                isiErrorCode = 30;
                break;
            case ISI_RETURN_FAILED:
            case ISI_RETURN_NOT_INIT:
            case ISI_RETURN_MUTEX_ERROR:
            default:
                isiErrorCode = 0;
                break;
        }
        CSM_sendError(isiErrorCode, "ISI Error", context_ptr->csmOutput_ptr);
        _CFSM_setState(context_ptr,
                (CFSM_State_Ptr) &_CFSM_STATE_TERMINATED, event_ptr);
    }
    else {
        context_ptr->callId = callId;
        /* Get call object. */
        call_ptr = CSM_getCall(callId);
        /* Set negExchang and cache session type. */
        _CFSM_setNegExchange(context_ptr,
                CSM_CALL_NEG_EXCHANGE_LOCAL_PROPOSED);
        call_ptr->negSessionType = call_ptr->sessionType = sessionType;
    }
}

/*
 * ======== _CFSM_terminateCall ========
 * This function is a helper to terminate call, set state and send CSM_Responses accordingly
 *
 * Return Values:
 * none
 */
void _CFSM_terminateCall(
    CFSM_Context_Ptr  context_ptr,
    CSM_EventCall_Ptr event_ptr)
{
    CSM_dbgPrintf("\n");

    /*
     * If a disconnect fails it's because the call is already gone.
     * So just ignore the return value.
     */
    _CSM_isiTerminateCall(context_ptr->callId, context_ptr->protoName_ptr,
        event_ptr->reasonDesc);
}

/*
 * ======== _CFSM_rejectCall ========
 * This function is a helper to reject call, set state and send CSM_Responses accordingly
 *
 * Return Values:
 * none
 */
void _CFSM_rejectCall(
    CFSM_Context_Ptr    context_ptr,
    CSM_EventCall_Ptr   event_ptr,
    char               *reason_ptr)
{
    CSM_dbgPrintf("\n");

    /*
     * If a reject fails it's because the call is already gone.
     * So just ignore the return value.
     */
    _CSM_isiRejectCall(context_ptr->callId, context_ptr->protoName_ptr,
            reason_ptr);
}

/*
 * ======== _CFSM_isFailoverToCs========
 * This function is private helper to check if the call is ip emergency call
 * and need to failover to CS emergency call when call failed.
 *
 * Return Values:
 *   OSAL_TRUE: It's ip emergency call and need to failover to CS emergency call.
 *   OSAL_FALSE: It's not ip emergency call or no need to failover to CS emergency call.
 */
OSAL_Boolean _CFSM_isFailoverToCs(
    CFSM_Context_Ptr  context_ptr)
{
    /*
     * If it's PS emergency call now and it needs failover to CS.
     */
    if ((context_ptr->isEmergency) &&
            (OSAL_strcmp(CSM_ISI_PROTOCOL_GSM_NAME,
            context_ptr->protoName_ptr)) &&
            (RPM_isEmergencyFailoverToCs())) {
        return (OSAL_TRUE);
    }

    return (OSAL_FALSE);
}

/*
 * ======== _CFSM_failoverToCs========
 * This function is private helper to process the emergency call failing over
 * to CS domain.
 *
 * Return Values:
 *   OSAL_TRUE: Failover to CS processed.
 *   OSAL_FALSE: Process failed.
 */
OSAL_Boolean _CFSM_failoverToCs(
    CFSM_Context_Ptr  context_ptr,
    CSM_EventCall_Ptr event_ptr)
{
    CSM_IsiService *service_ptr;
    char            address[CSM_EVENT_STRING_SZ + 1];

    /* Get CS service and normalize address */
    service_ptr = CSM_isiNormalizeOutboundAddress(
            context_ptr->isiMngr_ptr, context_ptr->remoteAddress_ptr,
            address, CSM_EVENT_STRING_SZ, RPM_FEATURE_TYPE_CALL_CS_EMERGENCY);

    if (NULL != service_ptr) {
        /* Then we have a service let's attempt the call. */
        context_ptr->serviceId = service_ptr->serviceId;
        context_ptr->protoName_ptr = service_ptr->protoName;
        /* Let's re-write the address with the normalized one. */
        OSAL_snprintf(event_ptr->u.remoteAddress,
                CSM_EVENT_STRING_SZ, "%s", address);
    }
    else {
        return (OSAL_FALSE);
    }

    return (OSAL_TRUE);
}

/*
 * ======== _CFSM_sendCallIndex() ========
 * This function is private helper to send CSM event with call index for the
 * response of AT+CDU.
 *
 * Return Values:
 *   None.
 */
void _CFSM_sendCallIndex(
    CFSM_Context_Ptr  context_ptr)
{
    CSM_CallObject* call_ptr;
    CSM_OutputEvent *csmOutput_ptr = context_ptr->csmOutput_ptr;
    CSM_OutputCall  *csmOutputCall_ptr = &csmOutput_ptr->evt.call;

    /* A participant is reported being added or removed. */
    call_ptr = CSM_getCall(context_ptr->callId);

    /* Construct the event */
    csmOutput_ptr->type = CSM_EVENT_TYPE_CALL;
    csmOutputCall_ptr->reason = CSM_OUTPUT_REASON_CALL_INDEX;
    csmOutputCall_ptr->reasonDesc[0] = 0;

    csmOutputCall_ptr->u.callIdx = call_ptr->participants[0].callIndex;

    /* Notify user */
    CSM_sendOutputEvent(csmOutput_ptr);
}


/*
 * ======== _CFSM_setNegExchange() ========
 * Function to set negotiation exchange status and send call report if
 * required.
 *
 * RETURN:
 *   None.
 */
void _CFSM_setNegExchange(
    CFSM_Context_Ptr    context_ptr,
    CSM_CallNegExchange negExchange)
{
    ISI_Id          callId;
    CSM_CallObject *call_ptr;

    callId = context_ptr->callId;
    /* Get call object. */
    call_ptr = CSM_getCall(callId);

    call_ptr->negExchange = negExchange;
    if ((CSM_CALL_NEG_EXCHANGE_REMOTE_PROPOSED == negExchange) ||
            (CSM_CALL_NEG_EXCHANGE_REMOTE_ACCEPTED == negExchange) ||
            (CSM_CALL_NEG_EXCHANGE_REMOTE_REJECTED == negExchange)) {
        /*
         * Remote proposed, accepted or rejected media change,
         * send call report.
         */
        _CSM_generateMonitorReport(NULL, context_ptr->csmOutput_ptr);
    }
    else if (CSM_CALL_NEG_EXCHANGE_MEDIA_CHANGED == negExchange) {
        /* Media changed uncontionally by local or remote. */
        _CSM_generateMonitorReport(NULL, context_ptr->csmOutput_ptr);
    }
    else {
        return;
    }

    /* Clear negExchange after reported. */    
    call_ptr->negExchange = CSM_CALL_NEG_EXCHANGE_NONE;
}


/*
 * ======== _CFSM_acceptCallModifyConditional() ========
 *
 * Private helper method for processing call modify event.
 *
 * RETURN:
 */
void  _CFSM_processCallModify(
    CSM_EventCall_Ptr      event_ptr,
    const CFSM_Context_Ptr context_ptr)
{
    ISI_Id               callId;
    CSM_CallObject      *call_ptr;
    CSM_CallSessionType  sessionType;
    ISI_SessionDirection dir;
    CSM_GlobalObj       *_CSM_globalObj_ptr;
    

    callId = context_ptr->callId;
    /* Get call object. */
    call_ptr = CSM_getCall(callId);
    sessionType = _CSM_isiGetSessionType(callId);

    /* Check if it's media upgrade. */
    if ((0 == (call_ptr->sessionType & CSM_CALL_SESSION_TYPE_VIDEO)) &&
            (0 != (sessionType & CSM_CALL_SESSION_TYPE_VIDEO))) {
        /* It's call upgrade, check call state. */
        if (&_CFSM_STATE_HOLD_LOCAL == context_ptr->currentState_ptr) {
            /* The call is on hold, reject upgrade. */
            _CSM_isiRejectCallModify(callId, "", context_ptr->protoName_ptr);
            return;
        }
        call_ptr->negSessionType = sessionType;
        /* Remote upgrade to video call. Set negExchange and send report */
        _CFSM_setNegExchange(context_ptr,
                CSM_CALL_NEG_EXCHANGE_REMOTE_PROPOSED);
        return;
    }
    
    /* Not an upgrade, accept the modification immediately. */
    _CSM_isiAcceptCallModify(callId, context_ptr->protoName_ptr);

    /* Get direction. Only get audio direction for checking held status. */
    if (ISI_RETURN_OK == _CSM_isiGetCallDirection(
            callId, ISI_SESSION_TYPE_AUDIO, &dir)) {
        _CSM_globalObj_ptr = CSM_getObject();
        /* Check is the call is been held or unheld. */
        if (dir != context_ptr->dir) {
            if (ISI_SESSION_DIR_SEND_RECV == context_ptr->dir) {
                /* The call is been held. */
                context_ptr->supsrvNotication |=
                        CSM_SUPSRV_MT_CALL_IS_HELD;

            }
            else if (ISI_SESSION_DIR_SEND_RECV == dir) {
                /* The call is been unheld. */
                context_ptr->supsrvNotication |=
                        CSM_SUPSRV_MT_CALL_IS_UNHELD;
            }
            context_ptr->dir = dir;
            /* Currently we report both supplementary event and call event. */
            _CSM_generateSupsrvReport(call_ptr, context_ptr->csmOutput_ptr, NULL);
            _CSM_generateMonitorReport(&_CSM_globalObj_ptr->evt.call,
                    context_ptr->csmOutput_ptr);
            /* Clear supsrv notification flag after reported. */
            context_ptr->supsrvNotication = 0;
        }
    }

    /*
     * If session type doesn't change, just accept it and no need up update
     * call status.
     */
    if (call_ptr->sessionType == sessionType) {
        return;
    }

    call_ptr->sessionType = sessionType;
    /* Media changed, send report.*/
    _CFSM_setNegExchange(context_ptr, CSM_CALL_NEG_EXCHANGE_MEDIA_CHANGED);

    return;
}

/*
 * ======== CFSM_getCallStatus() ========
 * Pubic function to get call status for reporting call report according
 * to current call state.
 *
 * Return:
 *   CSM_CallStatus The call status.
 */
CSM_CallStatus CFSM_getCallStatus(
    CFSM_Context_Ptr  context_ptr)
{
    CSM_CallStatus status = CSM_CALL_STATUS_IDLE;
    
    CFSM_State_Ptr curState_ptr = context_ptr->currentState_ptr;

    if (CSM_CALL_DIR_MOBILE_ORIGINATED == context_ptr->direction) {
        if ((CFSM_State_Ptr)&_CFSM_STATE_DIALING == curState_ptr) {
            status = CSM_CALL_STATUS_CALLING_MO;
        }
        else if ((CFSM_State_Ptr)&_CFSM_STATE_TRYING == curState_ptr) {
            status = CSM_CALL_STATUS_CONNECTING_MO;
        }
        else if ((CFSM_State_Ptr)&_CFSM_STATE_WAITING == curState_ptr) {
            status = CSM_CALL_STATUS_ALERTING_MO;
        }
        else if ((CFSM_State_Ptr)&_CFSM_STATE_CALL == curState_ptr) {
            if (ISI_SESSION_DIR_SEND_RECV == context_ptr->dir) {
                status = CSM_CALL_STATUS_ACTIVE;
            }
            else {
                /* Must be held by remote. */
                status = CSM_CALL_STATUS_CALL_HOLD_MT;
            }
        }
        else if ((CFSM_State_Ptr)&_CFSM_STATE_IP_CONF == curState_ptr) {
            status = CSM_CALL_STATUS_ACTIVE;
        }
        else if ((CFSM_State_Ptr)&_CFSM_STATE_CS_CONF == curState_ptr) {
            status = CSM_CALL_STATUS_ACTIVE;
        }
        else if ((CFSM_State_Ptr)&_CFSM_STATE_TERMINATED == curState_ptr) {
            status = CSM_CALL_STATUS_RELEASED_MO;
        }
        else if ((CFSM_State_Ptr)&_CFSM_STATE_HANGINGUP == curState_ptr) {
            status = CSM_CALL_STATUS_RELEASED_MO;
        }
        else if ((CFSM_State_Ptr)&_CFSM_STATE_HOLD_LOCAL == curState_ptr) {
            status = CSM_CALL_STATUS_CALL_HOLD_MO;
        }
        else if ((CFSM_State_Ptr)&_CFSM_STATE_IMS_CONF == curState_ptr) {
            status = CSM_CALL_STATUS_ACTIVE;
        }
        else {
            /* Should not be here. */
            CSM_dbgPrintf("Invlaid state.\n");
        }
    }
    else {
        /* MT call */
        if ((CFSM_State_Ptr)&_CFSM_STATE_RINGING == curState_ptr) {
            status = CSM_CALL_STATUS_ALERTING_MT;
        }
        else if ((CFSM_State_Ptr)&_CFSM_STATE_ANSWERED == curState_ptr) {
            /* The same status as last STATE: RINGING. */
            status = CSM_CALL_STATUS_ALERTING_MT;
        }
        else if ((CFSM_State_Ptr)&_CFSM_STATE_CALL == curState_ptr) {
            status = CSM_CALL_STATUS_ACTIVE;
        }
        else if ((CFSM_State_Ptr)&_CFSM_STATE_IP_CONF == curState_ptr) {
            status = CSM_CALL_STATUS_ACTIVE;
        }
        else if ((CFSM_State_Ptr)&_CFSM_STATE_CS_CONF == curState_ptr) {
            status = CSM_CALL_STATUS_ACTIVE;
        }
        else if ((CFSM_State_Ptr)&_CFSM_STATE_TERMINATED == curState_ptr) {
            status = CSM_CALL_STATUS_RELEASED_MT;
        }
        else if ((CFSM_State_Ptr)&_CFSM_STATE_HOLD_LOCAL == curState_ptr) {
            status = CSM_CALL_STATUS_CALL_HOLD_MO;
        }
        else if ((CFSM_State_Ptr)&_CFSM_STATE_IMS_CONF == curState_ptr) {
            status = CSM_CALL_STATUS_ACTIVE;
        }
        else {
            /* Should not be here. */
            CSM_dbgPrintf("Invlaid state.\n");
        }
    }
    return (status);
}

/*
 * ======== _CFSM_processMediaControl() ========
 *
 * Private helper method to media control command.
 *
 * RETURN:
 */
void _CFSM_processMediaControl(
    CSM_EventCall_Ptr      event_ptr,
    const CFSM_Context_Ptr context_ptr)
{
    ISI_Id          callId;
    CSM_CallObject *call_ptr;
    CSM_CallSessionType sessionType;

    callId = context_ptr->callId;
    /* Get call object. */
    call_ptr = CSM_getCall(callId);
    sessionType = _CSM_isiGetSessionType(callId);

    CSM_dbgPrintf("CSM_CALL_EVT_REASON_AT_CMD_MEDIA_CONTROL: "
            "negExchange:%d, negStatus:%d, callSessionType:%d\n",
            call_ptr->negExchange, event_ptr->negStatus, 
            event_ptr->callSessionType);

    /* Send OK to indicate command is processed first. */
    CSM_sendOk(NULL, context_ptr->csmOutput_ptr);

    if (CSM_CALL_NEG_STATUS_PROPOSED == event_ptr->negStatus) {
        /* Local proposed media changes. */
        _CSM_isiModifySessionType(callId, event_ptr->callSessionType);
        call_ptr->negExchange = CSM_CALL_NEG_EXCHANGE_LOCAL_PROPOSED;
        call_ptr->negSessionType = event_ptr->callSessionType;
    }
    else if (CSM_CALL_NEG_STATUS_UNCONDITIONAL == event_ptr->negStatus) {
        /* Downgrade or cancel call.
         *
         * Once ISI upgrades call to video, event callsessionType is video.
         * When the real sessionType is audio and the sessionTypeIsi is still video,
         * this invitation isn't accepted by remote yet.
         * And when the callSessionType is audio, this would cancel upgrading video.
         *
         * Note: it's would be automatically accepted if downgrading a video call. 
         */
        if ((call_ptr->sessionType == event_ptr->callSessionType) &&
            (call_ptr->sessionType != sessionType)) {
            _CSM_isiCancelCallModify(callId, event_ptr->callSessionType);
        }else {
            /* Local set media unconditionally. */
            _CSM_isiModifySessionType(callId, event_ptr->callSessionType);
        }

        call_ptr->negSessionType = event_ptr->callSessionType;
        /* Set negExchange. */
        _CFSM_setNegExchange(context_ptr, CSM_CALL_NEG_EXCHANGE_LOCAL_CHANGED);
    }
    else if (CSM_CALL_NEG_STATUS_ACCEPTED == event_ptr->negStatus) {
        /* Local accepts the media change. */
        call_ptr->sessionType = call_ptr->negSessionType;
        _CSM_isiAcceptCallModify(callId, context_ptr->protoName_ptr);
        _CFSM_setNegExchange(context_ptr, CSM_CALL_NEG_EXCHANGE_MEDIA_CHANGED);
    }
    else if (CSM_CALL_NEG_STATUS_REJECTED == event_ptr->negStatus) {
        _CSM_isiRejectCallModify(callId, CSM_CALL_USER_REJECT_MEDIA_CHANGE_STR,
                    context_ptr->protoName_ptr);
    }
}

/*
 * ======== _CFSM_processRemoteAnswer() ========
 * Function to process remote answers the call.
 *
 * RETURN:
 */
void _CFSM_processRemoteAnswer(
    const CFSM_Context_Ptr context_ptr)
{
    ISI_Id              callId;
    CSM_CallObject     *call_ptr;
    CSM_CallSessionType sessionType;

    callId = context_ptr->callId;
    /* Get call object. */
    call_ptr = CSM_getCall(callId);
    sessionType = _CSM_isiGetSessionType(callId);
    /* Get direction. */
     _CSM_isiGetCallDirection(callId, ISI_SESSION_TYPE_AUDIO,
             &context_ptr->dir);
    if (call_ptr->sessionType != sessionType) {
        /*
         * Remote answers with different media, send report to indicate
         * remote rejects the media change.
         */
        _CFSM_setNegExchange(context_ptr,
                CSM_CALL_NEG_EXCHANGE_REMOTE_REJECTED);
    }
    else {
        /* Remoete accepts the media change. */
        _CFSM_setNegExchange(context_ptr,
                CSM_CALL_NEG_EXCHANGE_REMOTE_ACCEPTED);
    }            
    call_ptr->sessionType = sessionType;
    /* Set negExchange for reporting it in active state. */
    call_ptr->negExchange = CSM_CALL_NEG_EXCHANGE_MEDIA_CHANGED;

    /* Check media convert. If there is it, report it. */
    if (_CSM_isiCheckSupsrvHeader(callId, ISI_SUPSRV_HFEXIST_MEDIA_CONVERT)) {
        context_ptr->supsrvNotication |=
                CSM_SUPSRV_MO_CALL_MEDIA_CONVERT;
    }
    /* report unsolicited event to app if needed */
    if (CSM_SUPSRV_CALL_NONE != context_ptr->supsrvNotication) {
        _CSM_generateSupsrvReport(
             CSM_getCall(context_ptr->callId),
             context_ptr->csmOutput_ptr, NULL);
    }
}

/*
 * ======== _CFSM_sendInitRegistration() ========
 * Function to notify service to send initial registration.
 *
 * RETURN:
 */
void _CFSM_sendInitRegistration(
    const CFSM_Context_Ptr context_ptr)
{
    CSM_PrivateInputEvt    *event_ptr;

    event_ptr = &context_ptr->isiMngr_ptr->csmInputEvent;
    event_ptr->type = CSM_PRIVATE_EVENT_TYPE_SERVICE;
    event_ptr->evt.service.reason = CSM_SERVICE_REASON_ACTIVATE;
    event_ptr->evt.service.serviceId = context_ptr->serviceId;

    CSM_isiSendEvent(context_ptr->isiMngr_ptr, event_ptr);
}

/*
 * ======== _CFSM_sendRingingTimeout() ========
 * Function to send ringing state timeout event to CSM
 *
 * RETURN:
 */
vint _CFSM_sendRingingTimeout(
    const CFSM_Context_Ptr context_ptr)
{
    CSM_CallObject         *call_ptr;
    CSM_PrivateInputEvt    *event_ptr;

    call_ptr = CSM_getCall(context_ptr->callId);
    event_ptr = &call_ptr->csmInternalInputEvent;
    event_ptr->type = CSM_PRIVATE_EVENT_TYPE_INTERNAL;
    event_ptr->evt.call.reason = CMS_INT_REASON_EVT_RINGING_TIMEOUT_CB;
    event_ptr->evt.call.id = context_ptr->callId;
    event_ptr->evt.call.serviceId = context_ptr->serviceId;

    /* Send the event to CSM */
    if (OSAL_SUCCESS != OSAL_msgQSend(context_ptr->isiMngr_ptr->eventQ,
            (char *)event_ptr, sizeof(CSM_PrivateInputEvt),
            OSAL_NO_WAIT, NULL)) {
        return (CSM_ERR);
    }
    return (CSM_OK);

}

