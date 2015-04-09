/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004-2012 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 30369 $ $Date: 2014-12-11 19:09:13 +0800 (Thu, 11 Dec 2014) $
 *
 */
#include <csm_event.h>
#include <rpm.h>
#include "_csm_event.h"
#include "_csm_calls.h"
#include "_csm_response.h"
#include "_csm_isi_call.h"
#include "_csm.h"
#include "_csm_print.h"
#include "_csm_utils.h"
#include "call_fsm/cfsm.h"

/* 
 * Global class pointer for this sub-module 
 */
static CSM_CallManager *mCallManager_ptr;

/*
 * Constant string used when parsing presence documents that
 * indicate whether or not a participant is joining or leaving a
 * conferenced call.
 */
static const char _CSM_FROM_XML_TAG[]   = "from";
static const char _CSM_SHOW_XML_TAG[]   = "show";
static const char _CSM_STATUS_XML_TAG[] = "status";
 
static const char _CSM_UNAVAILABLE_VALUE[]          = "Unavailable";
static const char _CSM_STATUS_CONNECTED_VALUE[]     = "connected";
static const char _CSM_STATUS_DISCONNECTED_VALUE[]  = "disconnected";
static const char _CSM_STATUS_ONHOLD_VALUE[]        = "on-hold";
static const char _CSM_STATUS_MUTEDVIAFOCUS_VALUE[] = "muted-via-focus";
static const char _CSM_STATUS_PENDING_VALUE[]       = "pending";
static const char _CSM_STATUS_ALERTING_VALUE[]      = "alerting";
static const char _CSM_STATUS_DIALING_VALUE[]       = "dialing-in";
static const char _CSM_STATUS_DISCONNECTING_VALUE[] = "disconnecting";

static const char _CSM_HISTORY_INFO_CAUSE[] = "cause=";
static const vint _CSM_HISTORY_INFO_CAUSE_SIZE = sizeof(_CSM_HISTORY_INFO_CAUSE)-1;
#define _CSM_HISTORY_INFO_INDEX "index="
static const vint _CSM_HISTORY_INFO_INDEX_SIZE = sizeof(_CSM_HISTORY_INFO_INDEX)-1;
/*
 * ======== _isActiveState ========
 *
 * Private helper routine used check the call state is 
 * active or held
 *
 * Returns: 
 *      CSM_OK: function exits normally.
 *      CSM_ERR: in case of error
 */
static vint _isActiveState(vint state)
{
    if (CSM_CALL_STATE_ACTIVE == state ||
            CSM_CALL_STATE_HOLD == state) {
        return (CSM_OK);
    }
    return (CSM_ERR);
}

/*
 * ======== _canConferenceCalls ========
 *
 * Private helper routine used determine if call conference can 
 * be performed
 *
 * Returns: 
 *      0: if FALSE
 *      >0: if conference is posible
 */
static vint _canConferenceCalls(
    const char *protoName_ptr)
{
    vint            count = 0;
    vint            x;
    CSM_CallObject *call_ptr;

    /* Loop over the call objects */
    for (x = 0; x < (CSM_NUMBER_OF_CALLS_MAX + 1); x++) {
        call_ptr = &mCallManager_ptr->callObj[x];
        /* 
         * Check if it's in use, matches the required protocol and 
         * is in active state
         */
        if (OSAL_TRUE == call_ptr->inUse) {
            if (0 != OSAL_strcmp(protoName_ptr, 
                    call_ptr->callFsm.protoName_ptr)) {
                return (0);
            }
            if (CSM_ERR == _isActiveState(call_ptr->callFsm.state)) {
                return (0);
            }
            count++;
        }
    }
    return (count);
}

/*
 * ======== _mergeIpCalls ========
 *
 * Private helper routine used to merge IMS calls
 *
 * Returns: 
 *    none
 */
static int _mergeIpCalls(
    CSM_CallObject *conf_ptr)
{
    CSM_CallObject *call_ptr;
    vint            count = 0;
    CSM_CallObject *confCalls[2];
    vint x, y;
    int ret = CSM_ERR;

    CSM_IpConference *ip_ptr = &mCallManager_ptr->confManager.u.ip;
    OSAL_memSet(ip_ptr, 0, sizeof(CSM_IpConference));

    /* Let's collect the calls that is not in conference to merge. */
    for (x = 0; x < (CSM_NUMBER_OF_CALLS_MAX + 1) && count < 2; x++) {
        call_ptr = &mCallManager_ptr->callObj[x];
        if (OSAL_TRUE == call_ptr->inUse &&
                CSM_OK == _isActiveState(call_ptr->callFsm.state)) {
            if (CSM_CALL_SINGLE_PARTY == call_ptr->multiParty) {
                confCalls[count++] = call_ptr;
            }
        }
    }

    y = -1;
    for (x = 0; x < count; x++) {
        call_ptr = confCalls[x];
        /* Copy the participant */
        while (y < (CSM_EVENT_MAX_CALL_LIST_SIZE -1)) {
            y++;
            if (conf_ptr->participants[y].callIndex == 0) {
                conf_ptr->participants[y] = call_ptr->participants[0];
                call_ptr->callFsm.isIpConference = OSAL_TRUE;

                ip_ptr->invitations[x] = call_ptr->callFsm.callId;

                CSM_dbgPrintf("Merging call to:%s", 
                        call_ptr->participants[0].number);
                ret = CSM_OK;
                break;
            }
        };
    }

    return (ret);
}

/*
 * ======== _CSM_mergeCsCalls ========
 *
 * Private helper routine used to merge CS calls
 *
 * Returns: 
 *      CSM_OK: function exits normally.
 *      CSM_ERR: in case of error
 */
static vint _CSM_mergeCsCalls(
    void)
{
    CSM_CallObject *call_ptr;
    vint            count = 0;
    CSM_CallObject *confCalls[CSM_NUMBER_OF_CALLS_MAX];
    vint            x;

    CSM_CsConference *cs_ptr = &mCallManager_ptr->confManager.u.cs;

    /* Let's collect the calls to merge. */
    OSAL_memSet(confCalls, 0, sizeof(confCalls));
    for (x = 0; x < (CSM_NUMBER_OF_CALLS_MAX + 1) &&
            count < CSM_NUMBER_OF_CALLS_MAX; x++) {
        call_ptr = &mCallManager_ptr->callObj[x];
        if (OSAL_TRUE == call_ptr->inUse &&
                CSM_OK == _isActiveState(call_ptr->callFsm.state)) {
            confCalls[count++] = call_ptr;
        }
    }

    if (0 == cs_ptr->confId) {
        /* Then there's no existing conference call. */
        if (ISI_RETURN_OK != _CSM_isiConferenceStart(
                &cs_ptr->confId,
                confCalls[0]->callFsm.callId,
                confCalls[1]->callFsm.callId)) {
            OSAL_logMsg("%s: ISI We failed to conference calls "
                    "callIdOne:%d, callIdTwo:%d", __FUNCTION__,
                   confCalls[0]->callFsm.callId, confCalls[1]->callFsm.callId);
            return (CSM_ERR);
        }
        cs_ptr->numCalls = 2;
    }
    else {
        /* Then there's an existing conference. */
        if (confCalls[cs_ptr->numCalls]) {
            if (ISI_RETURN_OK != _CSM_isiConferenceAdd(cs_ptr->confId,
                    confCalls[cs_ptr->numCalls]->callFsm.callId)) {
                OSAL_logMsg("%s: ISI We failed to add to a conference"
                        " confId:%d, callId:%d", __FUNCTION__, cs_ptr->confId,
                        confCalls[0]->callFsm.callId);
                return (CSM_ERR);
            }           
        }
        else {
            OSAL_logMsg("%s: The call index %d's callObj isn't exist. "
                    "ISI We failed to add it to a conference."
                    , __FUNCTION__, cs_ptr->numCalls);
            return (CSM_ERR);
        }
        cs_ptr->numCalls++;
    }
    return (CSM_OK);
}

/*
 * ======== CSM_getActiveCallCount() ========
 *
 * Returns the number of active call.
 * All calls in conference call will be count to one call.
 * XXX Assume there will be CS and PS conference call exists at the same time.
 *
 * Returns:
 *      Returns the number of calls objects in use.
 */
static vint CSM_getActiveCallCount()
{
    vint            x;
    vint            count;
    vint            hasConferenceCall;
    CSM_CallObject *call_ptr;

    count = 0;
    hasConferenceCall = 0;
    /* Then there is more than one calls. */
    for (x = 0; x < (CSM_NUMBER_OF_CALLS_MAX + 1); x++) {
        call_ptr = &mCallManager_ptr->callObj[x];
        if (OSAL_TRUE == call_ptr->inUse &&
                CSM_OK == _isActiveState(call_ptr->callFsm.state)) {
            if (CSM_CALL_SINGLE_PARTY == call_ptr->multiParty) {
                count++;
            }
            else {
                /* It's conference call. */
                if (!hasConferenceCall) {
                    hasConferenceCall = 1;
                    count++;
                }
                /* Otherwise the conference call is already count. */
            }
        }
    }

    CSM_dbgPrintf("Current active call number:%d\n", count);
    return (count);
}

/*
 * ======== CSM_getCallIndex ========
 *
 * Public routine to get an available call index
 *
 * Return Values:
 *     vint the call index
 */
vint CSM_getCallIndex(
    void)
{
    vint x;
    for (x = 1; x <= CSM_EVENT_MAX_CALL_LIST_SIZE; x++) {
        if (0 == mCallManager_ptr->availableCallIndex[x]) {
            mCallManager_ptr->availableCallIndex[x] = 1;
            mCallManager_ptr->numCalls++;
            CSM_dbgPrintf("callIndex:%d\n", x);
            return x;
        }
    }
    return 0;
}

/*
 * ======== CSM_clearCallIndex ========
 *
 * Public routine to clear the avaiable call index array
 * at a given call index
 *
 * Return Values:
 *     none
 */
void CSM_clearCallIndex(
    vint callIndex) 
{
    CSM_dbgPrintf("callIndex:%d\n", callIndex);
    mCallManager_ptr->availableCallIndex[callIndex] = 0;
    mCallManager_ptr->numCalls--;
}

/*
 * ======== CSM_setCallIndex ========
 *
 * Public routine to set the avaiable call index array
 * at a given call index
 *
 * Return Values:
 *     none
 */
void CSM_setCallIndex(
    vint callIndex) 
{
    CSM_dbgPrintf("callIndex:%d\n", callIndex);
    mCallManager_ptr->availableCallIndex[callIndex] = 1;
    mCallManager_ptr->numCalls++;
}

/*
 * ======== CSM_getCall ========
 *
 * Public routine get a CSM Call Object for a given call index
 *
 * Return Values:
 *     the CSM_CallObject*
 */
CSM_CallObject* CSM_getCall(
    ISI_Id callId)
{
    vint index;
    for (index = 0; index < (CSM_NUMBER_OF_CALLS_MAX + 1); index++) {
        if ((OSAL_TRUE == mCallManager_ptr->callObj[index].inUse) &&
                (callId == mCallManager_ptr->callObj[index].callFsm.callId)) {
            return (&mCallManager_ptr->callObj[index]);
        }
    }
    return (NULL);
}

/*
 * ======== _CSM_getFreeCall ========
 *
 * Public routine get a free CSM Call Object
 *
 * Return Values:
 *     the CSM_CallObject*
 */
CSM_CallObject* _CSM_getFreeCall(
    void)
{
    vint index;
    /* Loop to find a call object not "inUse" */
    for (index = 0; index < (CSM_NUMBER_OF_CALLS_MAX + 1); index++) {
        if ((OSAL_FALSE == mCallManager_ptr->callObj[index].inUse)) {
            return (&mCallManager_ptr->callObj[index]);
        }
    }
    return (NULL);
}

/*
 * ======== _CSM_initializeCall ========
 *
 * Private helper routine for initializing a call object and it's internal
 * Call FSM
 *
 * Return Values:
 *     none
 */
static void _CSM_initializeCall(
    CSM_CallObject     *callObject_ptr,
    CSM_CallDirection   direction,
    CSM_CallState       state,
    CSM_CallMultiParty  multiParty,
    ISI_Id              serviceId,
    ISI_Id              callId,
    OSAL_Boolean        isIpConference,
    vint                callIndex,
    CSM_CallSessionType callSessionType,
    CSM_OutputEvent    *csmOutput_ptr)
{
    /* This Call is free set initialize for outbound call */
    callObject_ptr->inUse = OSAL_TRUE;
    /* Get a new call index if it's not given */
    callObject_ptr->participants[0].callIndex = (callIndex != 0)?
            callIndex : CSM_getCallIndex();
    callObject_ptr->mode = CSM_CALL_MODE_VOICE;
    callObject_ptr->sessionType = callSessionType;
    callObject_ptr->negExchange = CSM_CALL_NEG_EXCHANGE_NONE;
    
    callObject_ptr->multiParty = multiParty;
    callObject_ptr->participants[0].alpha[0] = 0;
    callObject_ptr->participants[0].type = 
            CSM_CALL_ADDRESS_NATIONAL;
    /* SRVCC related info */
    callObject_ptr->vccInfo.isVcc  = OSAL_FALSE;
    callObject_ptr->vccInfo.csCallId  = 0;
    callObject_ptr->vccInfo.videoCallId  = 0;

    /* Init the FSM */
    CFSM_init(&(callObject_ptr->callFsm), csmOutput_ptr);
    callObject_ptr->callFsm.direction      = direction;
    callObject_ptr->callFsm.state          = state;    
    callObject_ptr->callFsm.active         = OSAL_TRUE;
    callObject_ptr->callFsm.serviceId      = serviceId;
    callObject_ptr->callFsm.callId         = callId;
    callObject_ptr->callFsm.isIpConference = isIpConference;
    callObject_ptr->callFsm.isFocusOwner   = OSAL_FALSE;
    callObject_ptr->callFsm.isPreconditionUsed = OSAL_FALSE;
    callObject_ptr->callFsm.isiMngr_ptr    = mCallManager_ptr->isiMngr_ptr;
}

/*
 * ======== CSM_decrementCsConference ========
 *
 * Public decrementing the active CS conference call legs
 *
 * Return Values:
 *     none
 */
void CSM_decrementCsConference(
    void)
{
    CSM_CsConference *cs_ptr = &mCallManager_ptr->confManager.u.cs;
    cs_ptr->numCalls--;
    if (1 >= cs_ptr->numCalls) {
        cs_ptr->numCalls = 0;
        cs_ptr->confId = 0;
    }
}

/*
 * ======== CSM_getCsConferenceId ========
 *
 * Public routine for getting the CS Converence ID
 *
 * Return Values:
 *     vint the conference ID
 */
vint CSM_getCsConferenceId(
    void)
{
    return (mCallManager_ptr->confManager.u.cs.confId);
}

/*
 * ======== CSM_getCsConferenceCallNums ========
 *
 * Public routine for getting the CS Converence call numbers
 *
 * Return Values:
 *     vint the conference call numbers
 */
vint CSM_getCsConferenceCallNums(
    void)
{
    return (mCallManager_ptr->confManager.u.cs.numCalls);
}

/*
 * ======== CSM_getIpConferenceInvitation ========
 *
 * Public routine for getting the IP Conference Invitation
 *
 * Return Values:
 *     vint the conference invitation value
 */
vint CSM_getIpConferenceInvitation(
    void)
{
    vint x;
    vint value;
    CSM_IpConference *ip_ptr = &mCallManager_ptr->confManager.u.ip;
    /* then look for someone to invite. */
    for (x = 0; x < (CSM_NUMBER_OF_CALLS_MAX + 1); x++) {
        value = ip_ptr->invitations[x];
        if (0 != value) {
            ip_ptr->invitations[x] = 0;
            return (value);
        }
    }
    return (0);
}

/*
 * ======== CSM_getIsiMngr ========
 *
 * Public routine for getting the IP Conference Invitation
 *
 * Return Values:
 *     vint the conference invitation value
 */
CSM_IsiMngr* CSM_getIsiMngr(
    void)
{
    return (mCallManager_ptr->isiMngr_ptr);
}

/*
 * ======== _CSM_callGetSessionType() ========
 * Prvicate function to get call session type bases on negotiation status
 * for reporting call list.
 *
 * Return:
 *   CSM_CallSesstionType The call session type to report.
 */
static int  _CSM_callGetSessionType(
    CSM_CallObject    *call_ptr)
{
    CSM_dbgPrintf("negExchange:%d, negSessionType:%d, callSessionType:%d\n",
            call_ptr->negExchange, call_ptr->negSessionType,
            call_ptr->sessionType);
    if (CSM_CALL_NEG_EXCHANGE_REMOTE_PROPOSED == call_ptr->negExchange) {
        /* Report proprose the meida, negSessionType is one to report. */
        return (call_ptr->negSessionType);
    }
    /* Otherwise, report callSessionType. */
    return (call_ptr->sessionType);
}

#if defined(PROVIDER_CMCC)
/*
 * ======== _CSM_checkCallSession() ========
 *
 * Private helper routine for checking current call session.
 * 
 *
 * Return Values:
 *     OSAL_TRUE: There is call in sessonType.
 *     OSAL_FALSE: There is no call in such seesionType.
 */
static OSAL_Boolean _CSM_checkCallSession(
    int filterSessionType)
{
    OSAL_Boolean    isSessionType;
    vint            index;
    CSM_CallObject *callObject_ptr;

    /*
     * Check for active calls.
     */
    isSessionType = OSAL_FALSE;
    for (index = 0; index < (CSM_NUMBER_OF_CALLS_MAX + 1); index++) {
        callObject_ptr = &(mCallManager_ptr->callObj[index]);
        if (filterSessionType == _CSM_callGetSessionType(callObject_ptr)) {
            isSessionType = OSAL_TRUE;
            break;
        }
    }
    return (isSessionType);
}
#endif

/*
 * ======== _getCallNegStatus() ========
 * Prvicate function to get negStatus for reporting call report.
 *
 * Return:
 *   CSM_CallNegStatus The negStatus to report.
 */
static CSM_CallNegStatus _CSM_callGetCallNegStatus(
    CSM_CallObject    *call_ptr)
{
    CSM_CallNegStatus status = CSM_CALL_NEG_STATUS_INVALID;
    switch (call_ptr->negExchange) {
        case CSM_CALL_NEG_EXCHANGE_MEDIA_CHANGED:
            status = CSM_CALL_NEG_STATUS_UNCONDITIONAL;
            break;
        case CSM_CALL_NEG_EXCHANGE_REMOTE_PROPOSED:
            status = CSM_CALL_NEG_STATUS_PROPOSED;
            break;
        case CSM_CALL_NEG_EXCHANGE_REMOTE_ACCEPTED:
            status = CSM_CALL_NEG_STATUS_ACCEPTED;
            break;
        case CSM_CALL_NEG_EXCHANGE_REMOTE_REJECTED:
            status = CSM_CALL_NEG_STATUS_REJECTED;
            break;
        default:
            break;
    }
    
    return (status);
}

/*
 * ======== _CSM_getCoderDescription ========
 *
 * Private helper routine for get coder SDP_md for CMCCSI event.
 *
 * Return Values:
 *     none
 */
static void _CSM_getCoderDescription(
    ISI_Id  callId,
    char   *coderDesc_ptr)
{
    char    tempBuffer[CSM_CODER_DESCRIPTION_STRING_SZ + 1];
    char    rtpmapBuffer[CSM_CODER_DESCRIPTION_STRING_SZ + 1];
    char    framesizeBuffer[CSM_CODER_DESCRIPTION_STRING_SZ + 1];
    char    fmtpBuffer[CSM_CODER_DESCRIPTION_STRING_SZ + 1];
    char    framerateBuffer[CSM_CODER_DESCRIPTION_STRING_SZ + 1];
    vint    isH263 = 1;
    int     codecenum;
    int     rate;

    char   *s_ptr;
    int    *int_ptr;
    char   *str_ptr;
    int     haveString;
    
    OSAL_memSet(tempBuffer, 0, sizeof(tempBuffer));
    OSAL_memSet(rtpmapBuffer, 0, sizeof(rtpmapBuffer));
    OSAL_memSet(framesizeBuffer, 0, sizeof(framesizeBuffer));
    OSAL_memSet(fmtpBuffer, 0, sizeof(fmtpBuffer));
    OSAL_memSet(framerateBuffer, 0, sizeof(framerateBuffer));
    OSAL_memSet(coderDesc_ptr, 0, OSAL_strlen(coderDesc_ptr));

    ISI_getCoderDescription(callId, "h263", tempBuffer);
    if (0 == OSAL_strlen(tempBuffer)) {
        ISI_getCoderDescription(callId, "h264", tempBuffer);
        isH263 = 0;
    }

    s_ptr = OSAL_strtok(tempBuffer, "=");
    while (NULL != s_ptr) {
        int_ptr = NULL;
        str_ptr = NULL;
        haveString = 0;
    
        if (!OSAL_strcasecmp(s_ptr, "enum")) {
            int_ptr = &codecenum;
        }
        else if (!OSAL_strcasecmp(s_ptr, "rate")) {
            int_ptr = &rate;
        }
        else if (!OSAL_strcasecmp(s_ptr, "framesize")) {
            if (0 == OSAL_strlen(framesizeBuffer)) {
                OSAL_snprintf(framesizeBuffer, sizeof(framesizeBuffer), "a=framesize:%d ", codecenum);
            }
            str_ptr = framesizeBuffer + OSAL_strlen(framesizeBuffer);
            haveString = 1;
        }
        else if (!OSAL_strcasecmp(s_ptr, "framerate")) {
            if (0 == OSAL_strlen(framerateBuffer)) {
                OSAL_snprintf(framerateBuffer, sizeof(framerateBuffer), "a=framerate:");
            }
            str_ptr = framerateBuffer + OSAL_strlen(framerateBuffer);
            haveString = 1;
        }
        else {
            if (0 == OSAL_strlen(fmtpBuffer)) {
                OSAL_snprintf(fmtpBuffer, sizeof(fmtpBuffer), "a=fmtp:%d ", codecenum);
            }
            str_ptr = fmtpBuffer + OSAL_strlen(fmtpBuffer);
            haveString = 1;
            OSAL_snprintf(str_ptr, sizeof(fmtpBuffer), "%s=", s_ptr);
            str_ptr = fmtpBuffer + OSAL_strlen(fmtpBuffer);
        }
    
        /*
         * We got token, let's get the value.
         */
        s_ptr = OSAL_strtok(NULL, ";");
        if (NULL == s_ptr) {
            break;
        }

        if (NULL != int_ptr) {
            *int_ptr = OSAL_atoi(s_ptr);
        }
        else if (0 != haveString) {
            OSAL_snprintf(str_ptr, OSAL_strlen(s_ptr) + 2, "%s;", s_ptr);
        }
        s_ptr = OSAL_strtok(NULL, "= \r\n");
    }

    OSAL_snprintf(rtpmapBuffer, sizeof(rtpmapBuffer), "a=rtpmap:%d %s/90000",
            codecenum, 
            1==isH263?"H263":"H264");

    /* remove the ';' at the end of buffer if exist. */
    if (';' == rtpmapBuffer[OSAL_strlen(rtpmapBuffer) - 1]) {
        rtpmapBuffer[OSAL_strlen(rtpmapBuffer) - 1] = 0;
    }
    if (';' == framesizeBuffer[OSAL_strlen(framesizeBuffer) - 1]) {
        framesizeBuffer[OSAL_strlen(framesizeBuffer) - 1] = 0;
    }
    if (';' == fmtpBuffer[OSAL_strlen(fmtpBuffer) - 1]) {
        fmtpBuffer[OSAL_strlen(fmtpBuffer) - 1] = 0;
    }
    if (';' == framerateBuffer[OSAL_strlen(framerateBuffer) - 1]) {
        framerateBuffer[OSAL_strlen(framerateBuffer) - 1] = 0;
    }

    str_ptr = coderDesc_ptr + OSAL_strlen(coderDesc_ptr);
    OSAL_snprintf(str_ptr, 10, "%d", codecenum);

    if (0 != OSAL_strlen(rtpmapBuffer)) {
        str_ptr = coderDesc_ptr + OSAL_strlen(coderDesc_ptr);
        if (str_ptr != coderDesc_ptr) {
            OSAL_snprintf(str_ptr, 7, "\\0D\\0A");
            str_ptr = coderDesc_ptr + OSAL_strlen(coderDesc_ptr);
        }
        OSAL_snprintf(str_ptr, OSAL_strlen(rtpmapBuffer) + 1, "%s", rtpmapBuffer);
    }

    if (0 != OSAL_strlen(framesizeBuffer)) {
        str_ptr = coderDesc_ptr + OSAL_strlen(coderDesc_ptr);
        if (str_ptr != coderDesc_ptr) {
            OSAL_snprintf(str_ptr, 7, "\\0D\\0A");
            str_ptr = coderDesc_ptr + OSAL_strlen(coderDesc_ptr);
        }
        OSAL_snprintf(str_ptr, OSAL_strlen(framesizeBuffer) + 1, "%s", framesizeBuffer);
    }

    if (0 != OSAL_strlen(fmtpBuffer)) {
        str_ptr = coderDesc_ptr + OSAL_strlen(coderDesc_ptr);
        if (str_ptr != coderDesc_ptr) {
            OSAL_snprintf(str_ptr, 7, "\\0D\\0A");
            str_ptr = coderDesc_ptr + OSAL_strlen(coderDesc_ptr);
        }
        OSAL_snprintf(str_ptr, OSAL_strlen(fmtpBuffer) + 1, "%s", fmtpBuffer);
    }

    if (0 != OSAL_strlen(framerateBuffer)) {
        str_ptr = coderDesc_ptr + OSAL_strlen(coderDesc_ptr);
        if (str_ptr != coderDesc_ptr) {
            OSAL_snprintf(str_ptr, 7, "\\0D\\0A");
            str_ptr = coderDesc_ptr + OSAL_strlen(coderDesc_ptr);
        }
        OSAL_snprintf(str_ptr, OSAL_strlen(framerateBuffer) + 1, "%s", framerateBuffer);
    }
}

/*
 * ======== _CSM_generateCallsList ========
 *
 * Private helper routine for generating call list for CLCC/CMCCSI reports
 *
 * Return Values:
 *     none
 */
static void _CSM_generateCallsList(
    CSM_CallEvt     *callEvt_ptr,
    CSM_OutputEvent *csmOutput_ptr)
{
    vint                  index;
    vint                  responseSize;
    vint                  participantIndex;
    CSM_CallObject       *callObject_ptr;
    CSM_CallSummary      *summary_ptr;
    CSM_CallParticipants *participant_ptr;
    vint                  summary[CSM_EVENT_MAX_CALL_LIST_SIZE + 1] = { 0 };
    vint                  idx;
    CSM_OutputCall       *csmOutputCall_ptr = &csmOutput_ptr->evt.call;

    /*
     * For each active call create an entry in the call list.
     */
    OSAL_strncpy(csmOutputCall_ptr->reasonDesc, "Call List",
            sizeof(csmOutputCall_ptr->reasonDesc));
    /*
     * Walk thru the call objects and create the call reports.
     */
    callObject_ptr = mCallManager_ptr->callObj;
    summary_ptr = csmOutputCall_ptr->u.report.calls;
    responseSize = 0;
    for (index = 0; index < (CSM_NUMBER_OF_CALLS_MAX + 1); index++) {
        if (OSAL_TRUE == callObject_ptr->inUse) {
            participant_ptr = callObject_ptr->participants;
            participantIndex = 0;
            /* Summarize each participants information */
            while ((responseSize < CSM_EVENT_MAX_CALL_LIST_SIZE) &&
                    (participantIndex < CSM_EVENT_MAX_CALL_LIST_SIZE)) {
                /* If there a callIndex then this participant entry is valid. */
                if (0 != participant_ptr->callIndex) {
                    /* 
                     * If the call already exists in the summary then ignore 
                     * this participant entry 
                     */
                    idx = participant_ptr->callIndex;
                    if (summary[idx] == 0) {
                        summary[idx] = 1;
                        summary_ptr->idx = participant_ptr->callIndex;
                        summary_ptr->isMultiParty = callObject_ptr->multiParty;
                        summary_ptr->direction = 
                                callObject_ptr->callFsm.direction;
                        summary_ptr->negStatus =
                                _CSM_callGetCallNegStatus(callObject_ptr);
                        summary_ptr->callSessionType =
                                _CSM_callGetSessionType(callObject_ptr);
                        summary_ptr->status =
                                CFSM_getCallStatus(&callObject_ptr->callFsm);
                        summary_ptr->numberType = participant_ptr->numberType;
                        summary_ptr->state = callObject_ptr->callFsm.state;
                        summary_ptr->mode = callObject_ptr->mode;
                        OSAL_strncpy(summary_ptr->normalizedAddress, 
                            participant_ptr->normalizedAddress, 
                            sizeof(summary_ptr->normalizedAddress));
                        OSAL_strncpy(summary_ptr->number, 
                                participant_ptr->number,
                                sizeof(summary_ptr->number));
                        summary_ptr->type = participant_ptr->type;
                        OSAL_strncpy(summary_ptr->alpha, participant_ptr->alpha,
                                sizeof(summary_ptr->alpha));
                        summary_ptr->state = callObject_ptr->callFsm.state;

                        _CSM_getCoderDescription(callObject_ptr->callFsm.callId, summary_ptr->coderSdpMd); 

                        /* Update to next available summary record. */
                        summary_ptr++;
                        responseSize++;
                    }
                }
                /* update to next available participant */
                participant_ptr++;
                participantIndex++;
            }
        }
        callObject_ptr++;
    }
    /* Done. Send the CLCC now */
    csmOutputCall_ptr->u.report.numCalls = responseSize;
}

/*
 * ======== _generateClccReport ========
 *
 * Private helper routine for generating CLCC reports
 *
 * Return Values:
 *     none
 */
void _CSM_generateClccReport(
    CSM_CallEvt     *callEvt_ptr,
    CSM_OutputEvent *csmOutput_ptr)
{
    CSM_OutputCall       *csmOutputCall_ptr = &csmOutput_ptr->evt.call;

    csmOutput_ptr->type = CSM_EVENT_TYPE_CALL;
    csmOutputCall_ptr->reason = CSM_OUTPUT_REASON_CALL_LIST;

    _CSM_generateCallsList(callEvt_ptr, csmOutput_ptr);

    CSM_sendOutputEvent(csmOutput_ptr);
}

/*
 * ======== _generateMonitorReport ========
 *
 * Private helper routine for generating CMCCSI reports
 *
 * Return Values:
 *     none
 */
void _CSM_generateMonitorReport(
    CSM_CallEvt     *callEvt_ptr,
    CSM_OutputEvent *csmOutput_ptr)
{
    CSM_OutputCall       *csmOutputCall_ptr = &csmOutput_ptr->evt.call;

    csmOutput_ptr->type = CSM_EVENT_TYPE_CALL;
    csmOutputCall_ptr->reason = CSM_OUTPUT_REASON_CALL_MONITOR;

    _CSM_generateCallsList(callEvt_ptr, csmOutput_ptr);

    CSM_sendOutputEvent(csmOutput_ptr);
}

/*
 * ======== _generateReport ========
 *
 * Private helper routine for generating reports
 *
 * Return Values:
 *     none
 */
static void _generateReport(
    CSM_CallEvt       *callEvt_ptr,
    CSM_CallObject    *callObject_ptr,
    CSM_OutputReason   reason,
    const char        *reasonDesc_ptr,
    CSM_OutputEvent   *csmOutput_ptr)
{
    CSM_OutputCall *csmOutputCall_ptr = &csmOutput_ptr->evt.call;

    /* Construct the event */
    csmOutput_ptr->type = CSM_EVENT_TYPE_CALL;
    csmOutputCall_ptr->reason = reason;
    OSAL_strncpy(csmOutputCall_ptr->reasonDesc, reasonDesc_ptr,
            sizeof(csmOutputCall_ptr->reasonDesc));
    if (CSM_OUTPUT_REASON_INITIALIZING_EVENT != reason) {
        OSAL_strncpy(csmOutputCall_ptr->u.clipReport.number,
                callObject_ptr->participants[0].number,
                sizeof(csmOutputCall_ptr->u.clipReport.number));
        OSAL_strncpy(csmOutputCall_ptr->u.clipReport.alpha,
                callObject_ptr->participants[0].alpha,
                sizeof(csmOutputCall_ptr->u.clipReport.alpha));
        csmOutputCall_ptr->u.clipReport.type =
                callObject_ptr->participants[0].type;
        csmOutputCall_ptr->u.clipReport.callSessionType =
                callObject_ptr->sessionType;
    }
    /* Notify user */
    CSM_sendOutputEvent(csmOutput_ptr);
}

/*
 * ======== _CSM_generateSupsrvReport ========
 *
 * Private helper routine for generating supsrv Notification reports
 *
 * Return Values:
 *     none
 */
 void _CSM_generateSupsrvReport(
    CSM_CallObject  *callObject_ptr,
    CSM_OutputEvent *csmOutput_ptr,
    char            *reasonDesc_ptr)
{
    CSM_OutputCall *csmOutputCall_ptr = &csmOutput_ptr->evt.call;
    vint i;

    /* Construct the event */
    csmOutput_ptr->type = CSM_EVENT_TYPE_CALL;
    csmOutputCall_ptr->reason = CSM_OUTPUT_REASON_CALL_EXTRA_INFO;
    if (NULL != reasonDesc_ptr) {
        OSAL_strncpy(csmOutputCall_ptr->reasonDesc, reasonDesc_ptr,
                sizeof(csmOutputCall_ptr->reasonDesc));
    }

    /* conf-call supsrv events */
    csmOutputCall_ptr->u.supsrvInfo.idx = callObject_ptr->participants[0].callIndex;
    csmOutputCall_ptr->u.supsrvInfo.supsrvNotification = callObject_ptr->callFsm.supsrvNotication;
    csmOutputCall_ptr->u.supsrvInfo.numHistories = callObject_ptr->numHistories;
    for (i=0; i<callObject_ptr->numHistories; i++) {
        OSAL_memCpy(&csmOutputCall_ptr->u.supsrvInfo.callHistories[i],
                    &callObject_ptr->callHistories[i],
                    sizeof(CSM_CallHistory));
    }

    /* Notify user */
    CSM_sendOutputEvent(csmOutput_ptr);
}

/*
 * ======== _CSM_isIncomingEventVcc ========
 *
 * Private helper routine for checking if call event is VCC by looking at the 
 * call state.  Should only be called for NEW_INCOMING events.
 *
 * NOTE: this relies on the fact that incoming VCCs are NOT in 
 *      the INCOMING state
 *
 * Return Values:
 *     CSM_CallObject*: if succesful
 *     NULL: if not
 */
static OSAL_Boolean _CSM_isIncomingEventVcc(
    CSM_CallEvt   *callEvt_ptr)
{
    ISI_CallState callState;

    /* Get the call state. */
    if (ISI_RETURN_OK == _CSM_isiGetCallState(callEvt_ptr->id, &callState)) {
        if ((ISI_CALL_STATE_ACTIVE == callState) ||
                (ISI_CALL_STATE_ONHOLD == callState)) {
            return (OSAL_TRUE);
        }
    }
    return (OSAL_FALSE);
}

/*
 * ======== _CSM_isCallInUse() ========
 *
 * Private helper routine for checking if there is call in use other than
 * the call object call_ptr.
 *
 * Return Values:
 *     OSAL_TRUE: There is call in use.
 *     OSAL_FALSE: There is no call in use.
 */
OSAL_Boolean _CSM_isCallInUse(
    CSM_CallObject *call_ptr)
{
    OSAL_Boolean    isActive;
    vint            index;
    CSM_CallObject *callObject_ptr;

    /*
     * Check for active calls.
     */
    isActive = OSAL_FALSE;
    for (index = 0; index < (CSM_NUMBER_OF_CALLS_MAX + 1); index++) {
        callObject_ptr = &(mCallManager_ptr->callObj[index]);
        if ((OSAL_TRUE == callObject_ptr->inUse) &&
                (callObject_ptr != call_ptr)) {
            /* There is call in use and it's not call_ptr */
            isActive = OSAL_TRUE;
            break;
        }
    }

    return (isActive);
}

/*
 * ======== _CSM_isEmergencyCall() ========
 *
 * Private helper routine for checking if there is call in use and
 * it is emergency call other than the call object call_ptr.
 *
 * Return Values:
 *     OSAL_TRUE: There is call which is in use and emergency.
 *     OSAL_FALSE: There is no call which is in use and emergency.
 */
OSAL_Boolean _CSM_isEmergencyCall(
    CSM_CallObject *call_ptr)
{
    vint            index;
    CSM_CallObject *callObject_ptr;

    /*
     * Check for active calls.
     */
    for (index = 0; index < (CSM_NUMBER_OF_CALLS_MAX + 1); index++) {
        callObject_ptr = &(mCallManager_ptr->callObj[index]);
        if ((OSAL_TRUE == callObject_ptr->inUse) &&
                (OSAL_TRUE == callObject_ptr->callFsm.isEmergency) &&
                (callObject_ptr != call_ptr)) {
            /* There is call in ringing state */
            return (OSAL_TRUE);
        }
    }

    return (OSAL_FALSE);
}

/*
 * ======== CSM_isCallInRing() ========
 *
 * Public helper routine for checking if there is call in ringing state
 * other than the call object call_ptr.
 *
 * Return Values:
 *     OSAL_TRUE: There is call in ringing state.
 *     OSAL_FALSE: There is no call in ringing state.
 */
OSAL_Boolean CSM_isCallInRing(
    CSM_CallObject *call_ptr)
{
    vint            index;
    CSM_CallObject *callObject_ptr;

    /*
     * Check for active calls.
     */
    for (index = 0; index < (CSM_NUMBER_OF_CALLS_MAX + 1); index++) {
        callObject_ptr = &(mCallManager_ptr->callObj[index]);
        if (((CSM_CALL_STATE_INCOMING == callObject_ptr->callFsm.state) || 
                (CSM_CALL_STATE_WAITING == callObject_ptr->callFsm.state)) &&
                (callObject_ptr != call_ptr)) {
            /* There is call in ringing state */
            return (OSAL_TRUE);
        }
    }

    return (OSAL_FALSE);
}

/*
 * ======== _CSM_getCallIpConf========
 *
 * Private helper routine for getting IP conference call object.
 *
 * Return Values:
 *     CSM_CallObject*: if succesful
 *     NULL: if not
 */
CSM_CallObject* _CSM_getCallIpConf(
    void)
{
    vint            index;
    CSM_CallObject *callObject_ptr;

    for (index = 0; index < (CSM_NUMBER_OF_CALLS_MAX + 1); index++) {
        callObject_ptr = &(mCallManager_ptr->callObj[index]);
        if (callObject_ptr->callFsm.isIpConference == OSAL_TRUE) {
            /* There is call in IP conference */
            return (callObject_ptr);
        }
    }

    return (NULL);
}

/*
 * ======== _CSM_mapOtherConfCallState() ========
 *
 * Private helper routine for checking if there is a other call in conference
 * is in the state you want to check.
 *
 * Return Values:
 *     OSAL_TRUE: There is an another call in conference in that stea.
 *     OSAL_FALSE: There is no other call in conference in that state.
 */
OSAL_Boolean _CSM_mapOtherConfCallState(
    CSM_CallObject *call_ptr,
    CSM_CallState      state)
{
    vint            index;
    CSM_CallObject *callObject_ptr;

    /*
     * Check for other conference calls.
     */
    for (index = 0; index < (CSM_NUMBER_OF_CALLS_MAX + 1); index++) {
        callObject_ptr = &(mCallManager_ptr->callObj[index]);
        if ((CSM_CALL_CONFERENCE == callObject_ptr->multiParty) &&
                (callObject_ptr != call_ptr)) {
            if (state == callObject_ptr->callFsm.state) {
                /* 
                 * There is an another call in conference 
                 * in that state you want to check.
                 */
                return (OSAL_TRUE);
            }
        }
    }

    return (OSAL_FALSE);
}

/*
 * ======== _CSM_generateInitilizingCallEvent() ========
 *
 * Private helper routine for generating initilizing call or waiting event.
 *
 * Return Values:
 *     None
 */
void _CSM_generateInitilizingCallEvent(
    CSM_CallObject  *callObject_ptr,
    OSAL_Boolean     isRsrcReady,
    OSAL_Boolean     isActive,
    CSM_OutputEvent *csmOutput_ptr)
{
    if (OSAL_FALSE == isRsrcReady) {
        /* Send initializing event */
        _generateReport(NULL, callObject_ptr,
                CSM_OUTPUT_REASON_INITIALIZING_EVENT,
                "Incoming Call Initializing", csmOutput_ptr);
        return;
    }
        
    if (OSAL_FALSE == isActive) {
        /* No other active call, then send 'incoming' report. */
        _generateReport(NULL, callObject_ptr, 
                CSM_OUTPUT_REASON_INCOMING_EVENT, "Incoming Call",
                csmOutput_ptr);
        return;
    }

    /* Then there is other active call, then send 'waiting' report. */
    _generateReport(NULL, callObject_ptr,
            CSM_OUTPUT_REASON_WAITING_EVENT, "Incoming Call Waiting",
            csmOutput_ptr);

    return;
}

/*
 *  ======== _CSM_decodeHistoryInfo() ========
 *
 *  Private helper method for parsing history-info into csm event struct
 *
 * service_ptr : the service used to normalize the phone number regarding to RPM
 * historyInfo_ptr : input string for parsing
 * callHistories_ptr : output buffer to store up to CSM_EVENT_MAX_CALL_LIST_SIZE hi-items
 *
 * sample header: History-Info: <sip:user2_public1@home1.net>index=1,
 *                              <sip:User-C@example.com;cause=302>index=1.1
 *
 *  RETURN:
 *      output result of hi-item parsed
 */
vint _CSM_decodeHistoryInfo(
    CSM_IsiService  *service_ptr,
    char            *historyInfo_ptr,
    CSM_CallHistory *callHistories_ptr)
{
    char tempHistory[ISI_HISTORY_INFO_STRING_SZ];
    char tempUri[ISI_ADDRESS_STRING_SZ];
    char tempCause[CSM_HISTORY_INDEX_STRING_SZ];
    char *sipUri_ptr, *uriEnd_ptr, *cause_ptr, *nextUri_ptr, *indexStr_ptr;
    vint numHistories, hisize, strsize;
    CSM_CallHistory *curHistory_ptr;

    CSM_utilUrlDecode(historyInfo_ptr,tempHistory);
    numHistories = 0;
    sipUri_ptr = nextUri_ptr = &tempHistory[0];
    while ((NULL != sipUri_ptr) && (numHistories<CSM_HISTORY_MAX_LIST_SIZE)) {
        nextUri_ptr = OSAL_strchr(sipUri_ptr, ',');
        if (NULL == nextUri_ptr) {
            hisize = OSAL_strlen(sipUri_ptr);
        } else {
            hisize = nextUri_ptr - sipUri_ptr;
            nextUri_ptr += 1; /* skip that "," */
        }
        curHistory_ptr = &callHistories_ptr[numHistories];
        if ((NULL == sipUri_ptr) || ('<' != *sipUri_ptr)) {
            break;
        }
        sipUri_ptr += 1;

        indexStr_ptr = OSAL_strnscan(sipUri_ptr, hisize, ">");
        if ((NULL == indexStr_ptr) || ('>' != *indexStr_ptr)) {
            break;
        }
        uriEnd_ptr = indexStr_ptr;
        indexStr_ptr += 1;

        OSAL_strncpy(tempUri, sipUri_ptr, uriEnd_ptr-sipUri_ptr+1);
        tempUri[uriEnd_ptr-sipUri_ptr+1] = 0;

        /* decode cause */
        cause_ptr = OSAL_strnscan(sipUri_ptr,
                (uriEnd_ptr-sipUri_ptr),
                _CSM_HISTORY_INFO_CAUSE);
        if (NULL == cause_ptr) {
            curHistory_ptr->cause = CSM_CALL_CAUSE_CODE_NONE;
        } else {
            /* assert (0 == OSAL_strncmp(cause_ptr, "cause=", 6)) */
            cause_ptr += _CSM_HISTORY_INFO_CAUSE_SIZE;
            strsize = 0;
            while ( ((*cause_ptr)>='0') && ((*cause_ptr)<='9') ) {
                tempCause[strsize++] = *cause_ptr++;
            }
            tempCause[strsize++] = 0;
            curHistory_ptr->cause = (CSM_CallCauseCode)(OSAL_atoi(tempCause));
        }

        /* decode hiIndex
         * assert (0 == OSAL_strncmp(indexStr_ptr,
         *      _CSM_HISTORY_INFO_INDEX,
         *      6))
         */
        indexStr_ptr += _CSM_HISTORY_INFO_INDEX_SIZE;
        strsize = 0;
        while ( (',' != (*indexStr_ptr)) &&
                (0 != (*indexStr_ptr)) &&
                (strsize < (CSM_HISTORY_INDEX_STRING_SZ - 1)) ) {
            curHistory_ptr->hiIndex[strsize++] = *indexStr_ptr++;
        }
        curHistory_ptr->hiIndex[strsize++] = 0;

        /* decode and normalize SIP URI */
        CSM_isiNormalizeInboundAddress(service_ptr,
                tempUri,
                curHistory_ptr->number, CSM_NUMBER_STRING_SZ,
                &curHistory_ptr->type,
                curHistory_ptr->alpha, CSM_ALPHA_STRING_SZ);

        /**/
        sipUri_ptr = nextUri_ptr;
        numHistories += 1;
    }
    return numHistories;
}

/*
 * ======== _CSM_callProcessIncoming ========
 *
 * Private helper routine for processing incoming call event 
 *
 * Return Values:
 *     CSM_CallObject*: if succesful
 *     NULL: if not
 */
static CSM_CallObject* _CSM_callProcessIncoming(
    CSM_CallEvt   *callEvt_ptr,
    CSM_OutputEvent *csmOutput_ptr)
{
    char                subject[CSM_ALPHA_STRING_SZ] = {0};
    char                from[CSM_ALPHA_STRING_SZ] = {0};
    CSM_IsiService     *service_ptr;
    CSM_CallObject     *callObject_ptr;
    CSM_CallState       callState;
    OSAL_Boolean        isActive;
    ISI_ResourceStatus  rsrcStatus;
    OSAL_Boolean        isRsrcReady;
    char                historyInfo[ISI_HISTORY_INFO_STRING_SZ];
    char               *uri_ptr;
    uvint               uriSize;
    vint                supsrvNotication;

    callState = CSM_CALL_STATE_INCOMING;
    /*
     * Check for active calls.
     */
    isActive = _CSM_isCallInUse(NULL);

    /*
     * Grab an available call object to create a new call.
     */
    if (NULL == (callObject_ptr = _CSM_getFreeCall())) {
        return (NULL);
    }

    /* Get the service pointer and name for this call. */
    service_ptr = CSM_isiGetServiceViaId(mCallManager_ptr->isiMngr_ptr, 
            callEvt_ptr->serviceId);

    /* Get call resource status */
    _CSM_isiGetCallResourceStatus(callEvt_ptr->id, &rsrcStatus,
        &csmOutput_ptr->evt.call.u.resourceMedia);
    /* Check if both local and remote are ready */
    if ((rsrcStatus & ISI_RESOURCE_STATUS_REMOTE_READY) &&
            (rsrcStatus & ISI_RESOURCE_STATUS_LOCAL_READY)) {
        isRsrcReady = OSAL_TRUE;
#if   defined(PROVIDER_CMCC)
        if (OSAL_TRUE == isActive) {
            if (OSAL_TRUE == _CSM_checkCallSession((
                    CSM_CALL_SESSION_TYPE_VIDEO | CSM_CALL_SESSION_TYPE_AUDIO))) {
                _CSM_isiRejectCall(callEvt_ptr->id, service_ptr->protoName,
                        CSM_CALL_REJECT_REASON_BUSY);
                return (NULL);
            }
            /* It's audio call. */
            /* 
             * assumed INCLUDE_SUPSRV in provider CMCC config
             * now check the supsrv setting. 
             */
            if (SUPSRV_RES_ENABLE == SUPSRV_getLocalCwSetting()) {
                callState = CSM_CALL_STATE_WAITING;
            } else {
                /* CW is disabled, reject the call with user busy per CMCC spec */
                _CSM_isiRejectCall(callEvt_ptr->id, service_ptr->protoName,
                        CSM_CALL_REJECT_REASON_BUSY);
                return (NULL);
            }
        }
#else
        /* If another call is active, state is waiting */
        if (OSAL_TRUE == isActive) {
            callState = CSM_CALL_STATE_WAITING;
        }
#endif
    }
    else {
        /* Resource is not ready yet */
        callState = CSM_CALL_STATE_INITIALIZING;
        isRsrcReady = OSAL_FALSE;
    }

    /* If there is emergency call already, reject all incoming call. */
    if (OSAL_TRUE == _CSM_isEmergencyCall(callObject_ptr)) {
        _CSM_isiRejectCall(callEvt_ptr->id, service_ptr->protoName, NULL);
        return (NULL);
    }

    /*
     * Check if there is other call is in ringing state or
     * there are over 2 calls.
     * If yes, reject this call and do not need to create the call.
     * If there are 2 calls, reject the incoming call.
     */
    if ((OSAL_TRUE == CSM_isCallInRing(callObject_ptr)) ||
            (CSM_getActiveCallCount() >= 2)) {
        _CSM_isiRejectCall(callEvt_ptr->id, service_ptr->protoName, NULL);
        return (NULL);
    }

    /*
     * Initialize the Call Object and it's FSM
     */
    _CSM_initializeCall(callObject_ptr, CSM_CALL_DIR_MOBILE_TERMINATED, 
            callState,  CSM_CALL_SINGLE_PARTY, callEvt_ptr->serviceId, 
            callEvt_ptr->id, OSAL_FALSE, 0, 
            _CSM_isiGetSessionType(callEvt_ptr->id), csmOutput_ptr);

    callObject_ptr->callFsm.protoName_ptr = service_ptr->protoName;

    /* Get the call header info */
    _CSM_isiGetCallHeader(callEvt_ptr->id, subject, from);

    /* Get direction. */
    _CSM_isiGetCallDirection(callEvt_ptr->id, ISI_SESSION_TYPE_AUDIO,
            &callObject_ptr->callFsm.dir);

    /* Let's normalize the address. */
    CSM_isiNormalizeInboundAddress(service_ptr, from,
            callObject_ptr->participants[0].number, CSM_NUMBER_STRING_SZ,
            &callObject_ptr->participants[0].type,
            callObject_ptr->participants[0].alpha, CSM_ALPHA_STRING_SZ);
    OSAL_strncpy(callObject_ptr->participants[0].normalizedAddress,
            from, CSM_ALPHA_STRING_SZ);
    /* 
     * Participants address normalized is copied from 'from', 
     * it's URI type address, so numberType can
     * be fixed to CSM_CALL_NUMBER_TYPE_URI.
     */
    callObject_ptr->participants[0].numberType = CSM_CALL_NUMBER_TYPE_URI;

    
    /* Then generate initializing, incoming or waiting report */
    _CSM_generateInitilizingCallEvent(callObject_ptr, isRsrcReady,
            isActive, csmOutput_ptr);

    /* also get the supsrv header and prepare history info data */
    /* get the supsrv header */
    if (_CSM_isiCheckSupsrvHeader(callEvt_ptr->id,
            ISI_SUPSRV_HFEXIST_HISTORY_INFO)) {
        /* Get history info */
        _CSM_isiGetSupsrvHistoryInfo(callEvt_ptr->id, historyInfo);
        callObject_ptr->numHistories = _CSM_decodeHistoryInfo(
                service_ptr, historyInfo, callObject_ptr->callHistories);
        /* generate extra supsrv call event, such as history-info  */
        /* invite with history-info means this is MT call that forwarded to me */
        callObject_ptr->callFsm.supsrvNotication |=
                    CSM_SUPSRV_MT_CALL_IS_FORWARDED;
    }
    if (_CSM_isiCheckSupsrvHeader(callEvt_ptr->id,
            ISI_SUPSRV_HFEXIST_MEDIA_CONVERT)) {
        supsrvNotication = callObject_ptr->callFsm.supsrvNotication;
        callObject_ptr->callFsm.supsrvNotication =
                CSM_SUPSRV_MT_CALL_MEDIA_CONVERT;
        _CSM_generateSupsrvReport(callObject_ptr, csmOutput_ptr, NULL);
        callObject_ptr->callFsm.supsrvNotication = supsrvNotication;
    }
    if (callObject_ptr->callFsm.supsrvNotication &
            CSM_SUPSRV_MT_CALL_IS_FORWARDED) {
        supsrvNotication = callObject_ptr->callFsm.supsrvNotication;
        callObject_ptr->callFsm.supsrvNotication =
                CSM_SUPSRV_MT_CALL_IS_FORWARDED;
        _CSM_generateSupsrvReport(callObject_ptr, csmOutput_ptr, NULL);
        callObject_ptr->callFsm.supsrvNotication = supsrvNotication;
    }
    

    /* Check if the call is call to alias URI. */
    if (CSM_OK == _CSM_utilGetValue(callEvt_ptr->reasonDesc,
            CSM_ISI_ALIAS_URI_STR, &uri_ptr, &uriSize)) {
        *(uri_ptr + uriSize) = 0;
        callObject_ptr->numHistories = 0;
        callObject_ptr->callFsm.supsrvNotication =
                CSM_SUPSRV_MT_CALL_ALIAS_URI;
        _CSM_generateSupsrvReport(callObject_ptr, csmOutput_ptr, uri_ptr);
    }
    return (callObject_ptr);
}

/*
 * ======== _CSM_sendSrvccSuccessTimeout() ========
 * Function to send SRVCC success timeout event to CSM
 *
 * RETURN:
 */
static vint _CSM_sendSrvccSuccessTimeout(
    CSM_CallObject *call_ptr)
{
    CSM_PrivateInputEvt    *event_ptr;

    event_ptr = &call_ptr->csmInternalInputEvent;
    event_ptr->type = CSM_PRIVATE_EVENT_TYPE_INTERNAL;
    event_ptr->evt.call.reason = CMS_INT_REASON_EVT_SRVCC_SUCCESS_TIMEOUT_CB;
    event_ptr->evt.call.id = call_ptr->callFsm.callId;
    event_ptr->evt.call.serviceId = call_ptr->callFsm.serviceId;

    /* Send the event to CSM */
    if (OSAL_SUCCESS != OSAL_msgQSend(call_ptr->callFsm.isiMngr_ptr->eventQ,
            (char *)event_ptr, sizeof(CSM_PrivateInputEvt),
            OSAL_NO_WAIT, NULL)) {
        return (CSM_ERR);
    }
    return (CSM_OK);

}

/*
 * ======== _CSM_aSrvccSuccessTimerCb ========
 * The call back function when aSRVCC success and wait call
 * terminating message from network but timeout
 *
 * RETURN:
 */
static int32 _CSM_srvccSuccessTimerCb(
    void *arg_ptr)
{
    CSM_CallObject *call_ptr = (CSM_CallObject *)arg_ptr;

    if (CSM_OK != _CSM_sendSrvccSuccessTimeout(call_ptr)) {
        /* Message send failed and retry it */
        OSAL_tmrStart(call_ptr->callFsm.retryTmrId, _CSM_srvccSuccessTimerCb,
                arg_ptr, CSM_CALL_SEND_MSG_RETRY_MS);
    }

    return (0);
}

/*
 * ======== _CSM_callProcessVccStart========
 *
 * Private helper routine for processing VCC Start event
 *
 * Return Values:
 *     None
 */
static  void _CSM_callProcessVccStart(
    CSM_CallEvt    *callEvt_ptr)
{
    CSM_CallObject *call_ptr;
    vint            x;
    uint8           onGoingSrvccCall = 0;
    uint32          srvccStatus = 0; 

    /* Looking for active call first. */
    for (x = 0; x < (CSM_NUMBER_OF_CALLS_MAX + 1); x++) {
        call_ptr = &mCallManager_ptr->callObj[x];
        if (OSAL_TRUE == call_ptr->inUse) {
            /* 
             * If active call was been found, active call need to do
             * SRVCC process anyway.
             */
            if (CSM_CALL_STATE_ACTIVE == call_ptr->callFsm.state) {
                call_ptr->vccInfo.isVcc = OSAL_TRUE;
                /* If start event is from CSM, ignore CS call id */
                if (NULL != callEvt_ptr) {
                    call_ptr->vccInfo.csCallId = callEvt_ptr->id;
                }
                onGoingSrvccCall = 1;
                srvccStatus = 0;
                break;
            }
        }
    }
    /* Then looking for hold call. */
    for (x = 0; x < (CSM_NUMBER_OF_CALLS_MAX + 1); x++) {
        call_ptr = &mCallManager_ptr->callObj[x];
        if (OSAL_TRUE == call_ptr->inUse) {
            /* 
             * To support hold call SRVCC, the mid-call feature need
             * to be supported in UE and network side both. Checking
             * the SRVCC status from ISI
             */
            if (CSM_CALL_STATE_HOLD == call_ptr->callFsm.state) {
                _CSM_isiGetCallSrvccStatus(call_ptr->callFsm.callId,
                        &srvccStatus);
                if (srvccStatus && (ISI_SRVCC_STATUS_MID_CALL_SEND ||
                        ISI_SRVCC_STATUS_MID_CALL_RECEIVE)) {
                    call_ptr->vccInfo.isVcc = OSAL_TRUE;
                    onGoingSrvccCall = 1;
                }
                srvccStatus = 0;
                break;
            }
        }
    }
    /*
     * As description in 3GPP TS 24.237, to support one on-going call
     * and one alerting call SRVCC, the UE need to has DRVCC and mid-call
     * capabilities. Currently we don't support DRVCC. So do aSRVCC only
     * if there is one alerting call only.
     */
    if (0 == onGoingSrvccCall) {
        /* No active & hold call. Looking for alerting call now.  */
        for (x = 0; x < (CSM_NUMBER_OF_CALLS_MAX + 1); x++) {
            call_ptr = &mCallManager_ptr->callObj[x];
            if (OSAL_TRUE == call_ptr->inUse) {
                if ((CSM_CALL_STATE_ALERTING == call_ptr->callFsm.state) ||
                        ((CSM_CALL_STATE_INCOMING == call_ptr->callFsm.state))) {
                    _CSM_isiGetCallSrvccStatus(call_ptr->callFsm.callId,
                            &srvccStatus);
                    if (srvccStatus && (ISI_SRVCC_STATUS_ALERTING_SEND ||
                            ISI_SRVCC_STATUS_ALERTING_RECEIVE)) {
                        call_ptr->vccInfo.isVcc = OSAL_TRUE;
                        onGoingSrvccCall = 1;
                    }
                    srvccStatus = 0;
                    break;
                }
            }
        }
    }

    if (onGoingSrvccCall) {
        CSM_dbgPrintf("Found call for SRVCC\n");
    }
    else {
        CSM_dbgPrintf("Cannot find call for SRVCC\n");
    }
}

/*
 * ======== _CSM_callProcessVccSuccess========
 *
 * Private helper routine for processing VCC success event
 *
 * Return Values:
 *     None
 */
static void _CSM_callProcessVccSuccess(
    int     gmStatus)
{
    uint16          sessionType;
    CSM_IsiService *service_ptr;
    CSM_CallObject *call_ptr;
    vint            x;
    uint8           isVideoCall = 0;

    /* Looking for call whcih has been marked to transfer. */
    for (x = 0; x < (CSM_NUMBER_OF_CALLS_MAX + 1); x++) {
        call_ptr = &mCallManager_ptr->callObj[x];
        if (OSAL_TRUE == call_ptr->vccInfo.isVcc) {
            if ((CSM_CALL_STATE_ACTIVE == call_ptr->callFsm.state) ||
                    (CSM_CALL_STATE_HOLD == call_ptr->callFsm.state)) {
                /* Find active call or hold call for transfer. */
                /* Get the call session type */
                if (ISI_RETURN_OK == _CSM_isiGetCallSessionType(
                        call_ptr->callFsm.callId, &sessionType)) {
                    if ((ISI_SESSION_TYPE_VIDEO |
                            ISI_SESSION_TYPE_SECURITY_VIDEO) &
                            sessionType) {
                        /* Backup call ID for video */
                        call_ptr->vccInfo.videoCallId = call_ptr->callFsm.callId;
                        /* Video call, remove audio and do call modify */
                        _CSM_isiCallRemoveAudio(call_ptr->callFsm.callId);
                        isVideoCall = 1;
                    }
                }
                if (isVideoCall) {
                    /* Does not diconnect if call is with video. */
                    isVideoCall = 0;
                }
                else {
                    if (gmStatus) {
                        /* Stop existing timer. */
                        OSAL_tmrStop(call_ptr->callFsm.timerId);
                        OSAL_tmrStop(call_ptr->callFsm.retryTmrId);
                        /* Start a timer to wait BYE send from network */
                        OSAL_tmrStart(call_ptr->callFsm.timerId,
                                _CSM_srvccSuccessTimerCb, call_ptr,
                                CSM_CALL_SRVCC_NET_TIMEOUT_VALUE);
                    }
                    else {
                        /* Release call internally. */
                        _CSM_isiLocalTerminateCall(call_ptr);
                    }
                }
                call_ptr->vccInfo.isVcc = OSAL_FALSE;
                /*
                 * The following codes are used only when D2 handle CS
                 * & PS call both.
                 */
                /* Swap IMS call info to call object */
                if (0 != call_ptr->vccInfo.csCallId) {
                    service_ptr = CSM_isiGetServiceViaProtocol(
                            mCallManager_ptr->isiMngr_ptr,
                            CSM_ISI_PROTOCOL_GSM, 0);
                    call_ptr->callFsm.protoName_ptr = service_ptr->protoName;
                    call_ptr->callFsm.serviceId = service_ptr->serviceId;
                    call_ptr->callFsm.callId = call_ptr->vccInfo.csCallId;
                }
            }
            else if ((CSM_CALL_STATE_ALERTING == call_ptr->callFsm.state) ||
                    (CSM_CALL_STATE_INCOMING == call_ptr->callFsm.state)) {
                /* Find alerting call for transfer. */
                call_ptr->vccInfo.isVcc = OSAL_FALSE;
                if (gmStatus) {
                    /* Stop existing timer. */
                    OSAL_tmrStop(call_ptr->callFsm.timerId);
                    OSAL_tmrStop(call_ptr->callFsm.retryTmrId);
                    /* Start a timer to wait 486 (MO)/CANCEL (MT) from network */
                    OSAL_tmrStart(call_ptr->callFsm.timerId,
                            _CSM_srvccSuccessTimerCb, call_ptr,
                            CSM_CALL_SRVCC_NET_TIMEOUT_VALUE);
                }
                else {
                    /* Release call internally. */
                    _CSM_isiLocalTerminateCall(call_ptr);
                }
            }
        }
        else if (OSAL_TRUE == call_ptr->inUse) {
            /*
             * Terminate other call which is not be transfered by SRVCC. Since
             * 3GPP TS24.237 doesn't has the description about how to handle
             * the call which is not transfer by SRVCC. So just send BYE if gm
             * is available, otherwise, release internally.
             */
            if (gmStatus) {
                _CSM_isiTerminateCall(call_ptr->callFsm.callId,
                        call_ptr->callFsm.protoName_ptr, NULL);
            }
            else {
                /* Release call internally. */
                _CSM_isiLocalTerminateCall(call_ptr);
            }
        }
    }
}

/*
 * ======== _CSM_callProcessVccFailure========
 *
 * Private helper routine for processing VCC failure event
 *
 * Return Values:
 *     None
 */
static void _CSM_callProcessVccFailure(
    void)
{
    CSM_IsiService *service_ptr;
    CSM_CallObject *call_ptr;
    vint            x;

    for (x = 0; x < (CSM_NUMBER_OF_CALLS_MAX + 1); x++) {
        call_ptr = &mCallManager_ptr->callObj[x];
        if (OSAL_TRUE == call_ptr->vccInfo.isVcc) {
            if (0 != call_ptr->vccInfo.csCallId) {
                /* 
                 * CS call handling. If using PS call only, the codes
                 * can be ignored. 
                 */
                service_ptr = CSM_isiGetServiceViaProtocol(
                        mCallManager_ptr->isiMngr_ptr,
                        CSM_ISI_PROTOCOL_GSM, 0);
                /* Terminate CS call to clean up CS call leg */
                _CSM_isiTerminateCall(call_ptr->vccInfo.csCallId,
                        service_ptr->protoName, NULL);
                call_ptr->vccInfo.csCallId = 0;
            }
            call_ptr->vccInfo.isVcc = OSAL_FALSE;
            /* Re-invite the PS call */
            _CSM_isiCallModify(call_ptr->callFsm.callId,
                    CSM_CALL_SRVCC_FAILURE_REASON_STR);
        }
        else if (OSAL_TRUE == call_ptr->inUse) {
            /* Also re-invite non-SRVCC call */
            _CSM_isiCallModify(call_ptr->callFsm.callId, NULL);
        }
    }
}

/*
 * ======== _CSM_callProcessEmergency() ========
 *
 * Private helper routine for processing emergency call event
 *
 * Return Values:
 *   None.
 */
static void _CSM_callProcessEmergency(
    CSM_CallEvt   *callEvt_ptr,
    CSM_CallObject  **newCallObject_ptr,
    CSM_OutputEvent    *csmOutput_ptr)
{
    CSM_CallObject *callObject_ptr;
    vint            x;
    vint            callIndex;

    callIndex = 0;
    /*
     * We have to make sure underlying protocol has free space for emergency 
     * call. We have to terminate all exsiting non-emergency call now.
     */
    for (x = 0; x < (CSM_NUMBER_OF_CALLS_MAX + 1); x++) {
        callObject_ptr = &mCallManager_ptr->callObj[x];
        /* Check if it's in use */
        if ((OSAL_TRUE == callObject_ptr->inUse) &&
                (OSAL_FALSE == callObject_ptr->callFsm.isEmergency)) {
            /* Disconnect the call */    
            _CSM_isiTerminateCall(callObject_ptr->callFsm.callId, 
                    callObject_ptr->callFsm.protoName_ptr, NULL);
        }
    }

    /*
     * See if there is available call object for emergency call.
     */
    if (CSM_NUMBER_OF_CALLS_MAX <= mCallManager_ptr->numCalls) {
        /* No available call. Force to get the first call object to use */
        callObject_ptr = &mCallManager_ptr->callObj[0];
    
        /*
         * We don't want to use the same call index as the call index
         * we are going to free. So get a call index now to prevent
         * call index conflict.
         */
        callIndex = CSM_getCallIndex();

        /*
         * Set inUse to false so that it can be found when calling
         * _CSM_getFreeCall() later.
         * This make sure we will always have availabe csm call object
         * for emergency call.
         */
        if (OSAL_TRUE == callObject_ptr->inUse) {
            callObject_ptr->inUse = OSAL_FALSE;
            /* Free call index */
            for (x = 0; x < CSM_EVENT_MAX_CALL_LIST_SIZE; x++) {
                if (0 != callObject_ptr->participants[x].callIndex) {
                    CSM_clearCallIndex(
                            callObject_ptr->participants[x].callIndex);
                    callObject_ptr->participants[x].callIndex = 0;
                }
            }
        }
    }

    /* Grab an available call object to create a new call. */
    if (NULL == (callObject_ptr = _CSM_getFreeCall())) {
        /*
         * Should not be here, we already checked the call number and free
         * call object if needed.
         */
        OSAL_logMsg("%s %d: Cannot get free call object for emergency call.\n",
                 __FUNCTION__, __LINE__);
        return;
    }

    /*
     * Initialize the Call Object and it's FSM
     */
    _CSM_initializeCall(callObject_ptr, 
            CSM_CALL_DIR_MOBILE_ORIGINATED, CSM_CALL_STATE_DIALING, 
            CSM_CALL_SINGLE_PARTY, callEvt_ptr->serviceId, 0,
            OSAL_FALSE, callIndex, CSM_CALL_SESSION_TYPE_AUDIO, csmOutput_ptr);

    /* Copy the remote address */
    OSAL_strncpy(callObject_ptr->participants[0].number, 
            callEvt_ptr->u.remoteAddress, 
            sizeof(callObject_ptr->participants[0].number));

    *newCallObject_ptr = callObject_ptr;
    return;
}

/*
 * ======== _CSM_callProcessDtmfDetect() ========
 *
 * Private helper routine for processing DTMF is detected.
 * MC detect a DTMF from remote, and CSM will send a output event.
 *
 * Return Values:
 *   None.
 */
static void _CSM_callProcessDtmfDetect(
    vint                digit,
    CSM_OutputEvent    *csmOutput_ptr)
{
    CSM_OutputCall *csmOutputCall_ptr = &csmOutput_ptr->evt.call;

    csmOutput_ptr->type = CSM_EVENT_TYPE_CALL;
    csmOutputCall_ptr->reason = CSM_OUTPUT_REASON_CALL_DTMF_DETECT;
    /* convert digit num to ASCII */
    if (10 == digit) {
        csmOutputCall_ptr->u.digit = '*';
    }
    else if (11 == digit) {
        csmOutputCall_ptr->u.digit = '#';
    }
    else if (12 == digit) {
        csmOutputCall_ptr->u.digit = 'A';
    }
    else if (13 == digit) {
        csmOutputCall_ptr->u.digit = 'B';
    }
    else if (14 == digit) {
        csmOutputCall_ptr->u.digit = 'C';
    }
    else if (15 == digit) {
        csmOutputCall_ptr->u.digit = 'D';
    }
    else {
        csmOutputCall_ptr->u.digit = digit + 48;
    }
    CSM_sendOutputEvent(csmOutput_ptr);
}

/*
 * ======== _CSM_processSpecialEvents ========
 *
 * Private helper routine for processing special call events
 * 
 * Return Values:
 *     as argument CSM_CallObject*:  
 *          returns a new call object if created as a result of 
 *          this special event.
 *     OSAL_TURE if caller should contintue to process this event.
 *     OSAL_FALSE if event should be discarded.
 */
static OSAL_Boolean _CSM_processSpecialEvents(
    CSM_CallEvt   *callEvt_ptr,
    CSM_CallObject  **newCallObject_ptr,
    CSM_OutputEvent *csmOutput_ptr)
{
    vint            index;
    vint            isActive;
    CSM_CallObject *callObject_ptr;
    CSM_IsiService* service_ptr;
    char           *param_ptr;

    switch (callEvt_ptr->reason) {
        case CSM_CALL_EVT_REASON_AT_CMD_CONFERENCE:
            /* 
             * First check that _areAllSameType call are of the same 
             * type (cs or ip). 
             */
            if (0 != _canConferenceCalls(CSM_ISI_PROTOCOL_MODEM_IMS_NAME)) {
                if (NULL != (callObject_ptr = _CSM_getCallIpConf())) {
                    /* 
                     * If there is a exsting IP CC, merege the new call to it. 
                     */
                    if (CSM_ERR == _mergeIpCalls(callObject_ptr)) {
                        CSM_sendError(3, "operation not allowed", csmOutput_ptr);
                        *newCallObject_ptr = NULL;
                        return (OSAL_FALSE);
                    }
                    CSM_sendOk(NULL, csmOutput_ptr);
                    *newCallObject_ptr = NULL;
                    return (OSAL_TRUE);
                }
                else {
                    service_ptr = CSM_isiGetServiceViaProtocol(
                            mCallManager_ptr->isiMngr_ptr,
                            CSM_ISI_PROTOCOL_MODEM_IMS, 0);
                    /*
                     * Grab an available call object to create a new call.
                     */
                    if (NULL != (callObject_ptr = _CSM_getFreeCall())) {
                        /* Do the IP merge */
                        if (CSM_ERR == _mergeIpCalls(callObject_ptr)) {
                            CSM_sendError(3, "operation not allowed", csmOutput_ptr);
                            *newCallObject_ptr = NULL;
                            return (OSAL_FALSE);
                        }
                        /* Initialize the Call and FSM */
                        callObject_ptr->inUse = OSAL_TRUE;
                        callObject_ptr->mode = CSM_CALL_MODE_VOICE;
                        callObject_ptr->callFsm.state = 
                                CSM_CALL_STATE_DIALING;
                        callObject_ptr->callFsm.direction = 
                                CSM_CALL_DIR_MOBILE_ORIGINATED;
                        CFSM_init(&(callObject_ptr->callFsm), csmOutput_ptr);
                        callObject_ptr->callFsm.active = OSAL_TRUE;
                        callObject_ptr->callFsm.isIpConference = OSAL_TRUE;
                        callObject_ptr->callFsm.serviceId = 
                                service_ptr->serviceId;
                        callObject_ptr->callFsm.protoName_ptr = 
                                service_ptr->protoName;
                        callObject_ptr->callFsm.isiMngr_ptr = 
                                mCallManager_ptr->isiMngr_ptr;
                        if (0 != (callObject_ptr->sessionType &
                                CSM_CALL_SESSION_TYPE_VIDEO)) {
                            /* Set the address of the video conf server */
                            OSAL_snprintf(callEvt_ptr->u.remoteAddress, 
                                    CSM_EVENT_STRING_SZ, "%s",
                                    service_ptr->videoconf);
                        }
                        else {
                            /*
                             * Set the remote address of* the audio conf. server
                             */
                            OSAL_snprintf(callEvt_ptr->u.remoteAddress, 
                                    CSM_EVENT_STRING_SZ, "%s",
                                    service_ptr->audioconf);
                        }
                        *newCallObject_ptr = callObject_ptr;
                        return (OSAL_TRUE);
                    }
                }
            }
            else if (0 != _canConferenceCalls(CSM_ISI_PROTOCOL_GSM_NAME)) {
                CSM_dbgPrintf("We have CS calls to conference");
                if (CSM_OK == _CSM_mergeCsCalls()) {
                    CSM_sendOk(NULL, csmOutput_ptr);
                    break;
                }
            }
            CSM_sendError(3, "operation not allowed", csmOutput_ptr);
            *newCallObject_ptr = NULL;
            return (OSAL_FALSE);

        case CSM_CALL_EVT_REASON_NEW_INCOMING:
            /* Check for VCC event */
            if (_CSM_isIncomingEventVcc(callEvt_ptr)) {
               _CSM_callProcessVccStart(callEvt_ptr); 
            }
            /* Normal new incoming call */
            else if (mCallManager_ptr->numCalls < CSM_NUMBER_OF_CALLS_MAX) {
                *newCallObject_ptr =_CSM_callProcessIncoming(
                        callEvt_ptr, csmOutput_ptr);
                return (OSAL_TRUE);
            }
            break;
        case CSM_CALL_EVT_REASON_AT_CMD_DIAL:
            /* Check for emergency call */
            if (callEvt_ptr->isEmergency) {
                _CSM_callProcessEmergency(callEvt_ptr, newCallObject_ptr,
                        csmOutput_ptr);
                return (OSAL_TRUE);
            }

            /* User wants to dials.  Check against the max max */
            if (mCallManager_ptr->numCalls < CSM_NUMBER_OF_CALLS_MAX) {
                /*
                 * Grab an available call object to create a new call.
                 */
                if (NULL == (callObject_ptr = _CSM_getFreeCall())) {
                    return (OSAL_TRUE);
                }

                /*
                 * Initialize the Call Object and it's FSM
                 */
                _CSM_initializeCall(callObject_ptr, 
                      CSM_CALL_DIR_MOBILE_ORIGINATED, CSM_CALL_STATE_DIALING, 
                      CSM_CALL_SINGLE_PARTY, callEvt_ptr->serviceId, 0,
                      OSAL_FALSE, 0, callEvt_ptr->callSessionType, csmOutput_ptr);

                /* Remove URI parameters */
                if (NULL != (param_ptr =
                        OSAL_strscan(callEvt_ptr->u.remoteAddress, ";"))) {
                    *param_ptr = 0;
                }

                /* Copy the remote address */
                OSAL_strncpy(callObject_ptr->participants[0].number, 
                        callEvt_ptr->u.remoteAddress, 
                        sizeof(callObject_ptr->participants[0].number));
                /* use RPM for numbertype detection AT+CDU ? 
                 * CSM_utilOutBoundIpCallNormalize? */
                if ( ('s'==callObject_ptr->participants[0].number[0]) ||
                        ('t'==callObject_ptr->participants[0].number[0])) {
                    callObject_ptr->participants[0].numberType = CSM_CALL_NUMBER_TYPE_URI;
                } else {
                    callObject_ptr->participants[0].numberType = CSM_CALL_NUMBER_TYPE_TON;
                }

                *newCallObject_ptr = callObject_ptr;
                return (OSAL_TRUE);
            }
            else {
                CSM_sendError(3, "operation not allowed", csmOutput_ptr);
                *newCallObject_ptr = NULL;
                return (OSAL_FALSE);
            }
        case CSM_CALL_EVT_REASON_AT_CMD_SRVCC_START:
        case CSM_CALL_EVT_REASON_SRVCC_START:
            _CSM_callProcessVccStart(NULL);
            break;
        case CSM_CALL_EVT_REASON_AT_CMD_SRVCC_SUCCESS:
        case CSM_CALL_EVT_REASON_SRVCC_SUCCESS:
            _CSM_callProcessVccSuccess(callEvt_ptr->extraArgument);
            break;
        case CSM_CALL_EVT_REASON_AT_CMD_SRVCC_FAILED:
        case CSM_CALL_EVT_REASON_SRVCC_FAILURE:
            _CSM_callProcessVccFailure();
            break;
        case CSM_CALL_EVT_REASON_AT_CMD_ANSWER:
        case CSM_CALL_EVT_REASON_AT_CMD_SWAP:
        case CSM_CALL_EVT_REASON_AT_CMD_END:
        case CSM_CALL_EVT_REASON_AT_CMD_HOLD_ALL_EXCEPT_X:
        case CSM_CALL_EVT_REASON_AT_CMD_RELEASE_AT_X:
            /*
             * If there are no active call report and error.
             */
            if (mCallManager_ptr->numCalls < CSM_NUMBER_OF_CALLS_MAX) {
                /*
                 * Check for active calls.
                 */
                isActive = OSAL_FALSE;
                for (index = 0; index < (CSM_NUMBER_OF_CALLS_MAX + 1); index++) {
                    callObject_ptr = &(mCallManager_ptr->callObj[index]);
                    if (OSAL_TRUE == callObject_ptr->inUse) {
                        isActive = OSAL_TRUE;
                        break;
                    }
                }
                if (OSAL_FALSE == isActive) {
                    CSM_sendError(0, "No Active Calls", csmOutput_ptr);
                }
            }
            break;
        case CSM_CALL_EVT_REASON_AT_CMD_REPORT:
            /* User requesting a call report */
            _CSM_generateClccReport(callEvt_ptr, csmOutput_ptr);
            /* Send OK */
            CSM_sendOk(NULL, csmOutput_ptr);
            break;
        case CSM_CALL_EVT_REASON_DIGIT_DONE:
            /* User dialing DTMF digits */
            CSM_sendOk(NULL, csmOutput_ptr);
            break;
        case CSM_CALL_EVT_REASON_DTMFDECT_LEADING:
            /* DTMF detected from inband or RFC4733. */
        case CSM_CALL_EVT_REASON_DTMFDECT_OOB:
            /* OOB is that DTMF is detected from SIP INFO. */
            /*
             * DTMF Inband and RFC4733 is leading edge, or SIP INFO,
             * then send output event.
             */
            _CSM_callProcessDtmfDetect(callEvt_ptr->u.digit, csmOutput_ptr);
            break;
        case CSM_CALL_EVT_REASON_VE_AEC_CMD:
            CSM_sendOk(NULL, csmOutput_ptr);
            /* Control AEC enable or disable */
            _CSM_isiMediaCntrlAecByPass(callEvt_ptr->u.aecEnable);
            break;
        case CSM_CALL_EVT_REASON_VE_GAIN_CTRL:
            if ((-40 > callEvt_ptr->u.gain.tx) ||
                    (40 < callEvt_ptr->u.gain.tx) ||
                    (-40 > callEvt_ptr->u.gain.rx) ||
                    (40 < callEvt_ptr->u.gain.rx)) {
                CSM_sendError(0, "operation not allowed", csmOutput_ptr);
            }
            else {
                CSM_sendOk(NULL, csmOutput_ptr);
                _CSM_isiMediaCntrlGain(callEvt_ptr->u.gain.tx,
                        callEvt_ptr->u.gain.rx);
            }
            break;
        case CSM_CALL_EVT_REASON_VE_CN_GAIN_CTRL:
            if ((-35 > callEvt_ptr->u.gain.tx) ||
                    (0 < callEvt_ptr->u.gain.tx)) {
                CSM_sendError(0, "operation not allowed", csmOutput_ptr);
            }
            else {
                CSM_sendOk(NULL, csmOutput_ptr);
                _CSM_isiMediaCntrlCnGain(callEvt_ptr->u.gain.tx);
            }
            break;
        case CSM_CALL_EVT_REASON_EARLY_MEDIA:
            CSM_sendEarlyMedia(csmOutput_ptr);
            break;
        default:
            break;
    }
    *newCallObject_ptr = NULL;
    return (OSAL_TRUE);
}

/*
 * ======== _CSM_decodeIsiPresenceDoc() ========
 *
 * Private helper for decoding the ISI presence doc for IP conference
 * subscription status
 * 
 * Return Values:
 *     none
 */
static vint _CSM_decodeIsiPresenceDoc(
    char          *doc_ptr,
    CSM_CallEvt *callEvt_ptr)
{
    ezxml_t               xml_ptr;
    ezxml_t               tag_ptr;
    const char           *value_ptr;
    CSM_Address          *address_ptr;
    CSM_IsiService       *service_ptr;

    address_ptr = &callEvt_ptr->u.remoteParty.address;
    service_ptr = CSM_isiGetServiceViaId(
            mCallManager_ptr->isiMngr_ptr, callEvt_ptr->serviceId);

    CSM_dbgPrintf("%s parsing: %s\n",__FUNCTION__, doc_ptr);

    if (NULL == (xml_ptr = ezxml_parse_str(doc_ptr, OSAL_strlen(doc_ptr)))) {
        return (CSM_ERR);
    }

    
    /* Let's get the from field. It's mandatory. */
    if (NULL != (tag_ptr = ezxml_get(xml_ptr, _CSM_FROM_XML_TAG, 0, ""))) {
        /* Copy the from field. */
        if (NULL != (value_ptr = ezxml_txt(tag_ptr))) {
            /* Normalize the address */
            CSM_isiNormalizeInboundAddress(service_ptr, value_ptr,
                    address_ptr->number, CSM_NUMBER_STRING_SZ,
                    (CSM_CallAddressType*)&address_ptr->type,
                    address_ptr->alpha, CSM_ALPHA_STRING_SZ);
        }
        else {
            ezxml_free(xml_ptr);
            return (CSM_ERR);
        }
    }
    else {
        ezxml_free(xml_ptr);
        return (CSM_ERR);
    }

    /* Let's see if this person is coming or going from the call. */

    /* Assume they are coming (joining the conference). */
    callEvt_ptr->u.remoteParty.isAdded = 1;
    if (NULL != (tag_ptr = ezxml_get(xml_ptr, _CSM_SHOW_XML_TAG, 0, ""))) {
        /* See if the 'show' value is 'unavailable' or not. */
        if (NULL != (value_ptr = ezxml_txt(tag_ptr))) {
            if (0 == OSAL_strcasecmp(value_ptr, _CSM_UNAVAILABLE_VALUE)) {
                callEvt_ptr->u.remoteParty.isAdded = 0;
            }
        }
    }

    /* Get participant status */
    if (NULL != (tag_ptr = ezxml_get(xml_ptr, _CSM_STATUS_XML_TAG, 0, ""))) {
        /* check the pariticipant status */
        if (NULL != (value_ptr = ezxml_txt(tag_ptr))) {
            /* Set output event participant status. */
            if (0 == OSAL_strcasecmp(value_ptr, _CSM_STATUS_CONNECTED_VALUE)) {
                callEvt_ptr->u.remoteParty.status =
                        CSM_PARTICIPANT_STATUS_CONNECTED;
            }
            else if (0 == OSAL_strcasecmp(value_ptr,
                    _CSM_STATUS_DISCONNECTED_VALUE)) {
                callEvt_ptr->u.remoteParty.status =
                        CSM_PARTICIPANT_STATUS_DISCONNECTED;
            }
            else if (0 == OSAL_strcasecmp(value_ptr,
                    _CSM_STATUS_ONHOLD_VALUE)) {
                callEvt_ptr->u.remoteParty.status =
                        CSM_PARTICIPANT_STATUS_ONHOLD;
            }
            else if (0 == OSAL_strcasecmp(value_ptr,
                    _CSM_STATUS_MUTEDVIAFOCUS_VALUE)) {
                callEvt_ptr->u.remoteParty.status =
                        CSM_PARTICIPANT_STATUS_MUTEDVIAFOCUS;
            }
            else if (0 == OSAL_strcasecmp(value_ptr,
                    _CSM_STATUS_PENDING_VALUE)) {
                callEvt_ptr->u.remoteParty.status =
                        CSM_PARTICIPANT_STATUS_PENDING;
            }
            else if (0 == OSAL_strcasecmp(value_ptr,
                    _CSM_STATUS_ALERTING_VALUE)) {
                callEvt_ptr->u.remoteParty.status =
                        CSM_PARTICIPANT_STATUS_ALERTING;
            }
            else if (0 == OSAL_strcasecmp(value_ptr,
                    _CSM_STATUS_DIALING_VALUE)) {
                callEvt_ptr->u.remoteParty.status =
                        CSM_PARTICIPANT_STATUS_DIALING;
            }
            else {
                callEvt_ptr->u.remoteParty.status =
                        CSM_PARTICIPANT_STATUS_DISCONNECTING;
            }
        }
    }

    ezxml_free(xml_ptr);
    return (CSM_OK);
}

/*
 * ======== _CSM_processSpecialPresenceEvents() ========
 *
 * Private helper processing special events due to call presense
 * 
 * Return Values:
 *     none
 */
static vint _CSM_processSpecialPresenceEvents(
    CSM_CallEvt   *callEvt_ptr)
{
    vint   ret;
    ISI_Id callId = 0;

    /* Switch on the call input event reason */
    switch (callEvt_ptr->reason) {
        case CSM_CALL_EVT_REASON_PARTICIPANT_INFO:
            /* For this event the 'id' is the presId, Let's 
             * read the presence info. */
            while (ISI_RETURN_OK == (ret = _CSM_isiReadPresence(
                    callEvt_ptr->id, &callId, callEvt_ptr->u.remoteAddress,
                    mCallManager_ptr->confManager.scratch,
                    CSM_PRESENCE_STRING_SZ)));

            if (ISI_RETURN_DONE != ret) {
                /* Nothing to process. */
                return (CSM_ERR);
            }
            /* We have the presence string, let's decode. */
            if (CSM_OK != _CSM_decodeIsiPresenceDoc(
                    mCallManager_ptr->confManager.scratch, callEvt_ptr)) {
                /* Nothing to process let's return error. */
                return (CSM_ERR);
            }
            /* Let's rewrite the id in the call event to indicate the callId. */
            callEvt_ptr->id = callId;
            break;
        default:
            break;
    }
    return (CSM_OK);
}

static OSAL_Boolean _CSM_isConferenceCallIndex(
    vint callIndex)
{
    vint            index;
    CSM_CallObject *conCallObj_ptr;

    conCallObj_ptr = _CSM_getCallIpConf();
    if (!conCallObj_ptr) {
        CSM_dbgPrintf("No ip conference call\n");
        return (OSAL_FALSE);
    }
    for (index = 0; index < CSM_NUMBER_OF_CALLS_MAX; index++) {
        CSM_dbgPrintf("Call id in ip conf:%d\n",
                conCallObj_ptr->participants[index].callIndex);
        if (callIndex == conCallObj_ptr->participants[index].callIndex) {
            return (OSAL_TRUE);
        }
    }
    return (OSAL_FALSE);
}
/*
 * ======== _CSM_tearDownTerminatedCalls() ========
 *
 * Private helper routine cleaning up terminated call objects
 * 
 * Return Values:
 *     none
 */
static void _CSM_tearDownTerminatedCalls()
{
    vint            index, x;
    CSM_CallObject *callObject_ptr;
    CSM_dbgPrintf("\n");

    callObject_ptr = mCallManager_ptr->callObj;

    for (index = 0; index < (CSM_NUMBER_OF_CALLS_MAX + 1); index++) {
        if ((callObject_ptr->callFsm.active == OSAL_FALSE) &&
                (OSAL_TRUE == callObject_ptr->inUse)) {
            callObject_ptr->inUse = OSAL_FALSE;
            for (x = 0; x < CSM_EVENT_MAX_CALL_LIST_SIZE; x++) {
                if (0 != callObject_ptr->participants[x].callIndex) {
                    CSM_dbgPrintf("x:%d callIndex:%d\n",
                            x, callObject_ptr->participants[x].callIndex);
                    /* If the call is conference call, clear all the calls. */
                    if ((CSM_CALL_CONFERENCE != callObject_ptr->multiParty) &&
                            (CSM_CALL_IMS_CONF != callObject_ptr->multiParty)){
                        /*
                         * If the call is in the conf call list, don't clear it.
                         */
                        if (OSAL_FALSE == _CSM_isConferenceCallIndex(
                                callObject_ptr->participants[x].callIndex)) {
                            CSM_clearCallIndex(
                                    callObject_ptr->participants[x].callIndex);
                            callObject_ptr->participants[x].callIndex = 0;
                        }
                    }
                    else {
                        CSM_clearCallIndex(
                                callObject_ptr->participants[x].callIndex);
                        callObject_ptr->participants[x].callIndex = 0;
                    }
                    
                }
            }
            CFSM_init(&(callObject_ptr->callFsm), NULL);
        }
        callObject_ptr++;
    }
}


/*
 * ======== _dialImsConfCall ========
 *
 * Private helper routine used to dial a new IMS conf call
 *
 * Returns: 
 *    CSM_CallObject the new conf call created
 */
static CSM_CallObject *_dialImsConfCall(
    CSM_CallEvt     *callEvt_ptr,
    CSM_OutputEvent *csmOutput_ptr,
    char            *participants_ptr)
{
    CSM_IsiService  *service_ptr;
    CSM_CallObject  *callObject_ptr;

    service_ptr = CSM_isiGetServiceViaProtocol(
                            mCallManager_ptr->isiMngr_ptr,
                            CSM_ISI_PROTOCOL_MODEM_IMS, 0);
    /*
     * Grab an available call object to create a new call.
     */
    if (NULL != (callObject_ptr = _CSM_getFreeCall())) {    
        /* Initialize the Call and FSM */
        callObject_ptr->inUse = OSAL_TRUE;
        callObject_ptr->participants[0].callIndex = CSM_getCallIndex();
        callObject_ptr->mode = CSM_CALL_MODE_VOICE;
        callObject_ptr->sessionType = callEvt_ptr->callSessionType;
        callObject_ptr->negExchange = CSM_CALL_NEG_EXCHANGE_NONE;
        callObject_ptr->callFsm.state = 
                CSM_CALL_STATE_DIALING;
        callObject_ptr->callFsm.direction = 
                CSM_CALL_DIR_MOBILE_ORIGINATED;
                
        callObject_ptr->multiParty = CSM_CALL_IMS_CONF;
        callObject_ptr->participants[0].alpha[0] = 0;
        callObject_ptr->participants[0].type = 
                CSM_CALL_ADDRESS_NATIONAL;
        /* SRVCC related info */
        callObject_ptr->vccInfo.isVcc  = OSAL_FALSE;
        callObject_ptr->vccInfo.csCallId  = 0;
        callObject_ptr->vccInfo.videoCallId  = 0;
    
        CFSM_init(&(callObject_ptr->callFsm), csmOutput_ptr);
        callObject_ptr->callFsm.active = OSAL_TRUE;
        callObject_ptr->callFsm.isIpConference = OSAL_TRUE;
        callObject_ptr->callFsm.isFocusOwner = OSAL_TRUE;
        callObject_ptr->callFsm.serviceId = 
                service_ptr->serviceId;
        callObject_ptr->callFsm.protoName_ptr = 
                service_ptr->protoName;
        callObject_ptr->callFsm.isiMngr_ptr = 
                mCallManager_ptr->isiMngr_ptr;
        
        /* when enter the state, it will get the participants from scratch */
        OSAL_strncpy(mCallManager_ptr->confManager.scratch,
                participants_ptr,
                CSM_PRESENCE_STRING_SZ);
        callObject_ptr->callFsm.remoteParticipants_ptr = 
                mCallManager_ptr->confManager.scratch;

        if (0 != (callObject_ptr->sessionType & CSM_CALL_SESSION_TYPE_VIDEO)) {
            /* Set the remote address of the video conf. server. */
            OSAL_strncpy(callObject_ptr->participants[0].number,
                service_ptr->videoconf,
                sizeof(callObject_ptr->participants[0].number));

            OSAL_strncpy(callObject_ptr->participants[0].normalizedAddress,
                service_ptr->videoconf,
                sizeof(callObject_ptr->participants[0].normalizedAddress));
        }
        else {
            /* Set the remote address of the audio conf. server. */
            OSAL_strncpy(callObject_ptr->participants[0].number,
                service_ptr->audioconf,
                sizeof(callObject_ptr->participants[0].number));

            OSAL_strncpy(callObject_ptr->participants[0].normalizedAddress,
                service_ptr->audioconf,
                sizeof(callObject_ptr->participants[0].normalizedAddress));
        }
        /* conf-factory number is always URI type */
        callObject_ptr->participants[0].numberType = CSM_CALL_NUMBER_TYPE_URI;

        /* xxx parse the participants into call object participants */
        /* xxx present group member status */
        return (callObject_ptr);
    }
    return (NULL);
}

/*
 * ======== _addToImsConfCall ========
 *
 * Private helper routine used to add a party to IMS conf call
 *
 * Returns: 
 *    none
 */
static int _addToImsConfCall(
    CSM_CallObject *conf_ptr,
    char *participants_ptr)
{
    CSM_CallObject *call_ptr;
    vint            count = 0;
    CSM_CallObject *confCalls[2];
    vint x;

    /* Let's find the call to add to. */
    for (x = 0; x < (CSM_NUMBER_OF_CALLS_MAX + 1) && count < 2; x++) {
        call_ptr = &mCallManager_ptr->callObj[x];
        if ((OSAL_TRUE == call_ptr->inUse) &&
                (CSM_OK == _isActiveState(call_ptr->callFsm.state)) &&
                (CSM_CALL_IMS_CONF == call_ptr->multiParty)) {
            confCalls[count++] = call_ptr;
        }
    }

    if (1 != count) {
        return (CSM_ERR);
    }

    call_ptr = confCalls[0];
    CSM_dbgPrintf("%s:%d add %s to confcall\n", __FUNCTION__, __LINE__, participants_ptr);
    OSAL_strncpy(mCallManager_ptr->confManager.scratch,
                participants_ptr,
                CSM_PRESENCE_STRING_SZ);
    call_ptr->callFsm.remoteParticipants_ptr = mCallManager_ptr->confManager.scratch;

    return (CSM_OK);
}

/*
 * ======== _CSM_processConfCallEvents ========
 *
 * Private helper routine for processing conf call events
 * 
 * Return Values:
 *     as argument CSM_CallObject*:  
 *          returns a new call object if created as a result of 
 *          this conf-call event.
 *     OSAL_TURE if caller should contintue to process this event.
 *     OSAL_FALSE if event should be discarded.
 */
static OSAL_Boolean _CSM_processConfCallEvents(
    CSM_CallEvt     *callEvt_ptr,
    CSM_CallObject  **newCallObject_ptr,
    CSM_OutputEvent *csmOutput_ptr)
{
    CSM_CallObject *callObject_ptr;

    switch (callEvt_ptr->reason) {
        case CSM_CALL_EVT_REASON_AT_CMD_CONF_DIAL:
            if (0 != _canConferenceCalls(CSM_ISI_PROTOCOL_MODEM_IMS_NAME)) {
                /* don't allow any existing IMS call before dialing a new group call */
                CSM_sendError(3, "operation not allowed", csmOutput_ptr);
                *newCallObject_ptr = NULL;
                return (OSAL_FALSE);
            }

            if (NULL == (callObject_ptr = _dialImsConfCall(
                    callEvt_ptr, csmOutput_ptr, callEvt_ptr->u.remoteParticipants))) {
                CSM_sendError(3, "operation not allowed", csmOutput_ptr);
                *newCallObject_ptr = NULL;
                return (OSAL_FALSE);
            }

            *newCallObject_ptr = callObject_ptr;
            return (OSAL_TRUE);
            break;
        case CSM_CALL_EVT_REASON_AT_CMD_CONF_ADD_PARTY:
            CSM_dbgPrintf("%s:%d check confcall\n", __FUNCTION__, __LINE__);
            if ((1 != _canConferenceCalls(CSM_ISI_PROTOCOL_MODEM_IMS_NAME)) ||
                    (NULL == (callObject_ptr = _CSM_getCallIpConf()))) {
                /* must have exactly 1 conf-call for modification */
                CSM_sendError(3, "operation not allowed", csmOutput_ptr);
                *newCallObject_ptr = NULL;
                return (OSAL_FALSE);
            }

            CSM_dbgPrintf("%s:%d doing add party to confcall\n", __FUNCTION__, __LINE__);
            if (CSM_ERR == _addToImsConfCall(callObject_ptr,
                    callEvt_ptr->u.remoteParticipants)) {
                CSM_sendError(3, "operation not allowed", csmOutput_ptr);
                *newCallObject_ptr = NULL;
                return (OSAL_FALSE);
            }

            CSM_sendOk(NULL, csmOutput_ptr);
            *newCallObject_ptr = NULL;
            return (OSAL_TRUE);
            break;
        default:
            /* not my reason, pass on. */
            return (OSAL_TRUE);
            break;
    }
}

/*
 * ======== CSM_callsInit() ========
 *
 * Public initializer method for the CSM Calls manager package
 * 
 * Return Values:
 *     none
 */
vint CSM_callsInit(
    CSM_CallManager    *callManager_ptr,
    CSM_IsiMngr        *isiMngr_ptr)
{
    int index;
    CSM_CallObject  *callObject_ptr;

    /*
     * Allocate call objects for 1 more than the maximum number of simultaneous
     * calls. This allows for an incoming emer
     */
    mCallManager_ptr = callManager_ptr;
    mCallManager_ptr->isiMngr_ptr = isiMngr_ptr;

    OSAL_memSet(&mCallManager_ptr->confManager, 0, 
            sizeof(CSM_ConferenceManager));

    for (index = 0; index < (CSM_NUMBER_OF_CALLS_MAX + 1); index++) {
        callObject_ptr = &(mCallManager_ptr->callObj[index]);
        CFSM_init(&(callObject_ptr->callFsm), NULL);
        callObject_ptr++;
    }

    return (CSM_OK);
}

/*
 * ======== CSM_callsProcessEvent() ========
 *
 * Main entry point into the Calls Package.
 *
 * Returns: 
 *      CSM_OK: function exits normally.
 *      CSM_ERR: in case of error
 */
vint CSM_callsProcessEvent(
    CSM_CallEvt     *callEvt_ptr,
    CSM_OutputEvent *csmOutput_ptr)
{
    CSM_CallObject *newCallObject_ptr;
    CSM_CallObject *callObject_ptr;
    vint            index;
    CSM_IsiService *service_ptr;
    char            address[CSM_EVENT_STRING_SZ + 1];
    RPM_FeatureType callType;

    if (CSM_CALL_EVT_REASON_START <= callEvt_ptr->reason) {
        /* then it's an event print the event with service details. */
        CSM_dbgPrintf("%s Event %d for %s\n", IR92_DEBUG_TAG,
                callEvt_ptr->reason,
                CSM_isiProtoNameViaServiceId(mCallManager_ptr->isiMngr_ptr, 
                        callEvt_ptr->serviceId));
    }

    /*
     *  Process any special presence events. The events are used
     *  during IP conference calling
     */
    if (CSM_OK != _CSM_processSpecialPresenceEvents(callEvt_ptr)) {
        /*
         * Then the event was related to presence and there was an ERROR.
         * Let's quietly discard.
         */
        CSM_dbgPrintf("%s:%d Processed by SpecialPresenceEvents failed\n",
                __FUNCTION__, __LINE__);
        return (CSM_OK);
    }

    /*
     * (1) Process any special events like, Dial, incoming, merge,
     * split, swap.
     */
     if (OSAL_FALSE == _CSM_processSpecialEvents(callEvt_ptr, 
             &newCallObject_ptr, csmOutput_ptr)) {
        CSM_dbgPrintf("%s:%d Processed by SpecialEvents failed\n",
                __FUNCTION__, __LINE__);
        return (CSM_OK);
     }

    /*
     * (1.1) Process any special conf-call events like, Dial, adhoc, modify
     */
     if (OSAL_FALSE == _CSM_processConfCallEvents(callEvt_ptr, 
             &newCallObject_ptr, csmOutput_ptr)) {
        CSM_dbgPrintf("%s:%d Processed by ConfCallEvents failed\n",
                __FUNCTION__, __LINE__);
        return (CSM_OK);
     }

    /*
     * (2) Process the events in the existing Calls.
     * Skip processing of any new call objects
     */
    callObject_ptr = mCallManager_ptr->callObj;
    for (index = 0; index < (CSM_NUMBER_OF_CALLS_MAX + 1); index++) {
        if ((callObject_ptr != newCallObject_ptr) &&
                (OSAL_TRUE == callObject_ptr->callFsm.active)) {
            CFSM_processEvent(&(callObject_ptr->callFsm), callEvt_ptr);
        }
        callObject_ptr++;
    }

    if (newCallObject_ptr != NULL) {
        /* Got a new call object.  Process it. */
        /*
         * Set the service ID based on RPM.
         */
        if (CSM_CALL_EVT_REASON_AT_CMD_DIAL == callEvt_ptr->reason) {
            /* Set rpm feature tpye bases on emergnecy call field */
            if (callEvt_ptr->isEmergency) {
                callType = RPM_FEATURE_TYPE_CALL_EMERGENCY;
            }
            else {
                callType = RPM_FEATURE_TYPE_CALL_NORMAL;
            }
            /* Then figure out the service to use and set the serviceId. */
            service_ptr = CSM_isiNormalizeOutboundAddress(
                    mCallManager_ptr->isiMngr_ptr, callEvt_ptr->u.remoteAddress,
                    address, CSM_EVENT_STRING_SZ, callType);
            OSAL_strncpy(newCallObject_ptr->participants[0].normalizedAddress,
                    address, CSM_ALPHA_STRING_SZ);

            if (NULL != service_ptr) {
                /* Then we have a service let's attempt the call. */
                newCallObject_ptr->callFsm.serviceId = service_ptr->serviceId;
                newCallObject_ptr->callFsm.protoName_ptr =
                        service_ptr->protoName;
                /* Let's re-write the address with the normalized one. */
                OSAL_snprintf(callEvt_ptr->u.remoteAddress, 
                        CSM_EVENT_STRING_SZ, "%s", address);
                newCallObject_ptr->callFsm.isEmergency =
                        callEvt_ptr->isEmergency;
                /* Cache pointer of remote address for CS failover */
                newCallObject_ptr->callFsm.remoteAddress_ptr =
                        newCallObject_ptr->participants[0].number;
            }
            else {
                /* Let's immediately return an error. */
                CSM_sendError(30, NULL, csmOutput_ptr);
                /* Mark the call as terminated. */
                newCallObject_ptr->callFsm.active = OSAL_FALSE;
            }
            
        }
        else if (CSM_CALL_EVT_REASON_AT_CMD_CONF_DIAL == callEvt_ptr->reason) {
            newCallObject_ptr->callFsm.remoteAddress_ptr =
                    newCallObject_ptr->participants[0].number;
        }
        CFSM_processEvent(&(newCallObject_ptr->callFsm), callEvt_ptr);
    }

    /*
     * (3) Tear down terminated CFSM, promote background to foreground
     */
    _CSM_tearDownTerminatedCalls();

    return (CSM_OK);
}

/*
 * ======== _CSM_callTerminateBySrvccSuccessTimer() ========
 *
 * The private helper to terminate call when SRVCC success
 * timer is timeout.
 *
 * Returns:
 */
void _CSM_callTerminateBySrvccSuccessTimer(
    CSM_CallEvt     *callEvt_ptr)
{
    CSM_CallObject  *call_ptr;
    call_ptr = CSM_getCall(callEvt_ptr->id);

    _CSM_isiLocalTerminateCall(call_ptr);
}

/*
 * ======== _CSM_callRejectByRingingTimer() ========
 *
 * The private helper to reject incoming call when 
 * rining timer is timeout.
 *
 * Returns:
 */
void _CSM_callRejectByRingingTimer(
    CSM_CallEvt     *callEvt_ptr)
{
    CSM_CallObject  *call_ptr;
    call_ptr = CSM_getCall(callEvt_ptr->id);
    /*
     * If a reject fails it's because the call is already gone.
     * So just ignore the return value.
     */
    _CSM_isiRejectCall(call_ptr->callFsm.callId, call_ptr->callFsm.protoName_ptr,
            CSM_CALL_REJECT_REASON_BUSY);
}

/*
 * ======== CSM_internalProcessEvent() ========
 *
 * The entry point to process CSM internal event.
 *
 * Returns:
 *      CSM_OK: function exits normally.
 *      CSM_ERR: in case of error
 */
vint CSM_internalProcessEvent(
    CSM_CallEvt     *callEvt_ptr,
    CSM_OutputEvent *csmOutput_ptr)
{
    switch (callEvt_ptr->reason) {
        case CSM_CALL_EVT_REASON_RINGING_TIMEOUT_CB:
            _CSM_callRejectByRingingTimer(callEvt_ptr);
            break;
        case CSM_CALL_EVT_REASON_SRVCC_SUCCESS_TIMEOUT_CB:
            _CSM_callTerminateBySrvccSuccessTimer(callEvt_ptr);
            break;
        default:
            break;
    }

    return 0;
}

/*
 * ======== CSM_callsOnSrvccChange() ========
 *
 * Callback from RPM to notify the CSM CallManager that an SRVCC has occured
 *
 * Returns: 
 *      none
 */
void CSM_callsOnSrvccChange(
    RPM_EventType eventType)
{
    CSM_dbgPrintf("eventType=%d\n", eventType);
}

/*
 * ======== CSM_callsShutdown() ========
 *
 * Main entry point into the Calls Package.
 *
 * Returns: 
 *      CSM_OK: function exits normally.
 *      CSM_ERR: in case of error
 */
vint CSM_callsShutdown()
{
    CSM_dbgPrintf("\n");

    return (CSM_OK);
}

/*
 * ======== CSM_callsNumberInUse() ========
 *
 * Returns the number of calls objects in use.
 *
 * Returns:
 *      Returns the number of calls objects in use.
 */
vint CSM_callsNumberInUse()
{
    return (mCallManager_ptr->numCalls);
}

/*
 * ======== CSM_callsConvertToInternalEvt() ========
 *
 * This function is used to convert CSM input event to CSM call internal event.
 *
 * Returns:
 *
 */
void CSM_callsConvertToInternalEvt(
    CSM_InputEvtType    type,
    void               *inputCallEvt_ptr,
    CSM_CallEvt        *csmCallEvt_ptr)
{
    CSM_InputCall       *csmExtCallEvt_ptr;
    CSM_InputIsiCall    *csmIntCallEvt_ptr;
    CSM_CallEvtReason    reason;

    OSAL_memSet(csmCallEvt_ptr, 0, sizeof(CSM_CallEvt));
    if (CSM_INPUT_EVT_TYPE_EXT == type) {
        csmExtCallEvt_ptr = (CSM_InputCall *)inputCallEvt_ptr;
        switch (csmExtCallEvt_ptr->reason) {
            case CSM_CALL_REASON_AT_CMD_DIAL:
                reason = CSM_CALL_EVT_REASON_AT_CMD_DIAL;
                break;
            case CSM_CALL_REASON_AT_CMD_REPORT:
                reason = CSM_CALL_EVT_REASON_AT_CMD_REPORT;
                break;
            case CSM_CALL_REASON_AT_CMD_ANSWER:
                reason = CSM_CALL_EVT_REASON_AT_CMD_ANSWER;
                break;
            case CSM_CALL_REASON_AT_CMD_END:
                reason = CSM_CALL_EVT_REASON_AT_CMD_END;
                break;
            case CSM_CALL_REASON_AT_CMD_SWAP:
                reason = CSM_CALL_EVT_REASON_AT_CMD_SWAP;
                break;
            case CSM_CALL_REASON_AT_CMD_END_ALL_ACTIVE:
                reason = CSM_CALL_EVT_REASON_AT_CMD_END_ALL_ACTIVE;
                break;
            case CSM_CALL_REASON_AT_CMD_END_ALL_HELD_OR_WAITING:
                reason = CSM_CALL_EVT_REASON_AT_CMD_END_ALL_HELD_OR_WAITING;
                break;
            case CSM_CALL_REASON_AT_CMD_HOLD_ALL_EXCEPT_X:
                reason = CSM_CALL_EVT_REASON_AT_CMD_HOLD_ALL_EXCEPT_X;
                break;
            case CSM_CALL_REASON_AT_CMD_DIGIT:
                reason = CSM_CALL_EVT_REASON_AT_CMD_DIGIT;
                break;
            case CSM_CALL_REASON_AT_CMD_RELEASE_AT_X:
                reason = CSM_CALL_EVT_REASON_AT_CMD_RELEASE_AT_X;
                break;
            case CSM_CALL_REASON_AT_CMD_CONFERENCE:
                reason = CSM_CALL_EVT_REASON_AT_CMD_CONFERENCE;
                break;
            case CSM_CALL_REASON_AT_CMD_CONF_DIAL:
                reason = CSM_CALL_EVT_REASON_AT_CMD_CONF_DIAL;
                break;
            case CSM_CALL_REASON_AT_CMD_CONF_ADD_PARTY:
                reason = CSM_CALL_EVT_REASON_AT_CMD_CONF_ADD_PARTY;
                break;
            case CSM_CALL_REASON_AT_CMD_SRVCC_START:
                reason = CSM_CALL_EVT_REASON_AT_CMD_SRVCC_START;
                break;
            case CSM_CALL_REASON_AT_CMD_SRVCC_SUCCESS:
                reason = CSM_CALL_EVT_REASON_AT_CMD_SRVCC_SUCCESS;
                break;
            case CSM_CALL_REASON_AT_CMD_SRVCC_FAILED:
                reason = CSM_CALL_EVT_REASON_AT_CMD_SRVCC_FAILED;
                break;
            case CSM_CALL_REASON_RESOURCE_INDICATION:
                reason = CSM_CALL_EVT_REASON_RESOURCE_INDICATION;
                break;
            case CSM_CALL_REASON_AT_CMD_MEDIA_CONTROL:
                reason = CSM_CALL_EVT_REASON_AT_CMD_MEDIA_CONTROL;
                break;
            case CSM_CALL_REASON_VE_AEC_CMD:
                reason = CSM_CALL_EVT_REASON_VE_AEC_CMD;
                break;
            case CSM_CALL_REASON_VE_GAIN_CTRL:
                reason = CSM_CALL_EVT_REASON_VE_GAIN_CTRL;
                break;
            case CSM_CALL_REASON_VE_CN_GAIN_CTRL:
                reason = CSM_CALL_EVT_REASON_VE_CN_GAIN_CTRL;
                break;
            default:
                reason = CSM_CALL_EVT_REASON_INVALID;
                OSAL_logMsg("Invalid CSM call resaon:%d\n",
                        csmExtCallEvt_ptr->reason);
        }
        /*
         * Copy csm input event that from external(AT) to
         * csm internal call event
         */
        csmCallEvt_ptr->type          = csmExtCallEvt_ptr->type;
        csmCallEvt_ptr->reason        = reason;
        csmCallEvt_ptr->extraArgument = csmExtCallEvt_ptr->extraArgument;
        csmCallEvt_ptr->isEmergency   = csmExtCallEvt_ptr->isEmergency;
        csmCallEvt_ptr->emergencyType = csmExtCallEvt_ptr->emergencyType;
        csmCallEvt_ptr->cidType       = csmExtCallEvt_ptr->cidType;
        csmCallEvt_ptr->isRsrcReady   = csmExtCallEvt_ptr->isRsrcReady;
        csmCallEvt_ptr->callSessionType = csmExtCallEvt_ptr->callSessionType;
        csmCallEvt_ptr->negStatus = csmExtCallEvt_ptr->negStatus;
        OSAL_memCpy(csmCallEvt_ptr->reasonDesc, csmExtCallEvt_ptr->reasonDesc,
                sizeof(csmCallEvt_ptr->reasonDesc));
        OSAL_memCpy(&csmCallEvt_ptr->u, &csmExtCallEvt_ptr->u,
                sizeof(csmCallEvt_ptr->u));
    }
    else if (CSM_INPUT_EVT_TYPE_INT == type) {
        csmIntCallEvt_ptr = (CSM_InputIsiCall *)inputCallEvt_ptr;
        switch (csmIntCallEvt_ptr->reason) {
            case CSM_CALL_REASON_EVT_TRYING:
                reason = CSM_CALL_EVT_REASON_TRYING;
                break;
            case CSM_CALL_REASON_EVT_ACKNOWLEDGED:
                reason = CSM_CALL_EVT_REASON_ACKNOWLEDGED;
                break;
            case CSM_CALL_REASON_EVT_FAILED:
                reason = CSM_CALL_EVT_REASON_FAILED;
                break;
            case CSM_CALL_REASON_EVT_DISCONNECT:
                reason = CSM_CALL_EVT_REASON_DISCONNECT;
                break;
            case CSM_CALL_REASON_EVT_HOLD:
                reason = CSM_CALL_EVT_REASON_HOLD;
                break;
            case CSM_CALL_REASON_EVT_RESUME:
                reason = CSM_CALL_EVT_REASON_RESUME;
                break;
            case CSM_CALL_REASON_EVT_ACCEPTED:
                reason = CSM_CALL_EVT_REASON_ACCEPTED;
                break;
            case CSM_CALL_REASON_EVT_ACCEPT_ACK:
                reason = CSM_CALL_EVT_REASON_ACCEPT_ACK;
                break;
            case CSM_CALL_REASON_EVT_REJECTED:
                reason = CSM_CALL_EVT_REASON_REJECTED;
                break;
            case CSM_CALL_REASON_EVT_VCC_HANDOFF:
                reason = CSM_CALL_EVT_REASON_VCC_HANDOFF;
                break;
            case CSM_CALL_REASON_EVT_DIGIT_DONE:
                reason = CSM_CALL_EVT_REASON_DIGIT_DONE;
                break;
            case CSM_CALL_REASON_EVT_XFER_FAILED:
                reason = CSM_CALL_EVT_REASON_XFER_FAILED;
                break;
            case CSM_CALL_REASON_EVT_XFER_DONE:
                reason = CSM_CALL_EVT_REASON_XFER_DONE;
                break;
            case CSM_CALL_REASON_EVT_PARTICIPANT_INFO:
                reason = CSM_CALL_EVT_REASON_PARTICIPANT_INFO;
                break;
            case CSM_CALL_REASON_EVT_MODIFY:
                reason = CSM_CALL_EVT_REASON_MODIFY;
                break;
            case CSM_CALL_REASON_EVT_BEING_FORWARDED:
                reason = CSM_CALL_EVT_REASON_BEING_FORWARDED;
                break;
            case CSM_CALL_REASON_EVT_SRVCC_START:
                reason = CSM_CALL_EVT_REASON_SRVCC_START;
                break;
            case CSM_CALL_REASON_EVT_SRVCC_SUCCESS:
                reason = CSM_CALL_EVT_REASON_SRVCC_SUCCESS;
                break;
            case CSM_CALL_REASON_EVT_SRVCC_FAILURE:
                reason = CSM_CALL_EVT_REASON_SRVCC_FAILURE;
                break;
            case CSM_CALL_REASON_EVT_MODIFY_COMPLETED:
                reason = CSM_CALL_EVT_REASON_MODIFY_COMPLETED;
                break;
            case CSM_CALL_REASON_EVT_MODIFY_FAILED:
                reason = CSM_CALL_EVT_REASON_MODIFY_FAILED;
                break;
            case CSM_CALL_REASON_EVT_SERVICE_ACTIVE:
                reason = CSM_CALL_EVT_REASON_SERVICE_ACTIVE;
                break;
            case CSM_CALL_REASON_EVT_SERVICE_INACTIVE:
                reason = CSM_CALL_EVT_REASON_SERVICE_INACTIVE;
                break;
            case CSM_CALL_REASON_EVT_DTMFDECT_OOB:
                reason = CSM_CALL_EVT_REASON_DTMFDECT_OOB;
                csmCallEvt_ptr->u.digit = csmIntCallEvt_ptr->u.digit;
                break;
            case CSM_CALL_REASON_EVT_DTMFDECT_LEADING:
                reason = CSM_CALL_EVT_REASON_DTMFDECT_LEADING;
                csmCallEvt_ptr->u.digit = csmIntCallEvt_ptr->u.digit;
                break;
            case CSM_CALL_REASON_EVT_DTMFDECT_TRAILING:
                reason = CSM_CALL_EVT_REASON_DTMFDECT_TRAILING;
                csmCallEvt_ptr->u.digit = csmIntCallEvt_ptr->u.digit;
                break;
            case CSM_CALL_REASON_EVT_VIDEO_REQUEST_KEY:
                reason = CSM_CALL_EVT_REASON_VIDEO_REQUEST_KEY;
                break;
            case CSM_CALL_REASON_EVT_EARLY_MEDIA:
                reason = CSM_CALL_EVT_REASON_EARLY_MEDIA;
                break;
            case CSM_CALL_REASON_EVT_NEW_INCOMING:
            default:
                reason = CSM_CALL_EVT_REASON_NEW_INCOMING;
                break;
        }
        /*
         * Copy csm input event that from internal(ISI) to
         * csm internal call event
         */
        csmCallEvt_ptr->type          = csmIntCallEvt_ptr->type;
        csmCallEvt_ptr->reason        = reason;
        csmCallEvt_ptr->id            = csmIntCallEvt_ptr->id;
        csmCallEvt_ptr->serviceId     = csmIntCallEvt_ptr->serviceId;
        csmCallEvt_ptr->extraArgument = csmIntCallEvt_ptr->extraArgument;
        OSAL_memCpy(csmCallEvt_ptr->reasonDesc, csmIntCallEvt_ptr->reasonDesc,
                sizeof(csmCallEvt_ptr->reasonDesc));
    }
}

/*
 * ======== CSM_internalConvertToInternalEvt() ========
 *
 * This function is used to convert CSM internal event.
 *
 * Returns:
 *
 */
void CSM_internalConvertToInternalEvt(
    void               *inputCallEvt_ptr,
    CSM_CallEvt        *csmCallEvt_ptr)
{
    CSM_InputIsiCall    *csmIntCallEvt_ptr;
    CSM_CallEvtReason    reason;

    OSAL_memSet(csmCallEvt_ptr, 0, sizeof(CSM_CallEvt));
    csmIntCallEvt_ptr = (CSM_InputIsiCall *)inputCallEvt_ptr;

    switch (csmIntCallEvt_ptr->reason) {
        case CMS_INT_REASON_EVT_RINGING_TIMEOUT_CB:
            reason = CSM_CALL_EVT_REASON_RINGING_TIMEOUT_CB;
            break;
        case CMS_INT_REASON_EVT_SRVCC_SUCCESS_TIMEOUT_CB:
            reason = CSM_CALL_EVT_REASON_SRVCC_SUCCESS_TIMEOUT_CB;
            break;
        default:
            reason = CSM_CALL_EVT_REASON_INVALID;
            break;
    }

    csmCallEvt_ptr->type          = CSM_INPUT_EVT_TYPE_INT;
    csmCallEvt_ptr->reason        = reason;
    csmCallEvt_ptr->id            = csmIntCallEvt_ptr->id;
    csmCallEvt_ptr->serviceId     = csmIntCallEvt_ptr->serviceId;
    csmCallEvt_ptr->extraArgument = csmIntCallEvt_ptr->extraArgument;
    OSAL_memCpy(csmCallEvt_ptr->reasonDesc, csmIntCallEvt_ptr->reasonDesc,
            sizeof(csmCallEvt_ptr->reasonDesc));
}


/*
 * ======== _CSM_generateCallModificationEvent() ========
 *
 * Private helper routine for generating call modification.
 *
 * Return Values:
 *     None
 */
void _CSM_generateCallModification(
    CSM_CallObject  *callObject_ptr,
    CSM_OutputEvent *csmOutput_ptr)
{
    _generateReport(NULL, callObject_ptr,
                CSM_OUTPUT_REASON_CALL_MODIFY_EVENT,
                "Call Modification", csmOutput_ptr);
    return;
}

/*
 * ======== CSM_generateKeyFrameRequestEvent() ========
 *
 * Private helper routine for generating video key frame request.
 *
 * Return Values:
 *     None
 */
void CSM_generateVideoRequestKeyEvent(
    CSM_OutputEvent *csmOutput_ptr)
{
    CSM_OutputCall *csmOutputCall_ptr = &csmOutput_ptr->evt.call;

    /* Construct the event */
    csmOutput_ptr->type = CSM_EVENT_TYPE_CALL;
    csmOutputCall_ptr->reason = CSM_OUTPUT_REASON_CALL_VIDEO_REQUEST_KEY;
    csmOutputCall_ptr->reasonDesc[0] = '\0';
    CSM_sendOutputEvent(csmOutput_ptr);
    return;
}

