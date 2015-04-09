/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 30140 $ $Date: 2014-12-01 18:13:45 +0800 (Mon, 01 Dec 2014) $
 *
 */

#include <osal.h>
#include <isi.h>
#include <isip.h>
#include <csm_event.h>
#include "_tapp.h"
#include "_tapp_mock_sapp.h"

extern TAPP_GlobalObj *global_ptr;
extern char *_TAPP_ActionTypeStrings[];
extern char *_TAPP_EventTypeStrings[];

extern TAPP_EnumObj _TAPP_IsipCodeTable[];
extern TAPP_EnumObj _TAPP_IsipCallReasonTable[];
extern TAPP_EnumObj _TAPP_IsipServiceReasonTable[];
extern TAPP_EnumObj _TAPP_IsipSystemReasonTable[];
extern TAPP_EnumObj _TAPP_IsipStatusTable[];
extern TAPP_EnumObj _TAPP_IsipServerTypeTable[];
extern TAPP_EnumObj _TAPP_IsipSessionDirTable[];
extern TAPP_EnumObj _TAPP_IsipResourceStatusTable[];
extern TAPP_EnumObj _TAPP_IsipSessionTypeTable[];
extern TAPP_EnumObj _TAPP_IsipTextReasonTable[];
extern TAPP_EnumObj _TAPP_IsipMessageTypeTable[];
extern TAPP_EnumObj _TAPP_IsipSessionCidTypeTable[];
extern TAPP_EnumObj _TAPP_RpcIsiFuncTypeTable[];
extern TAPP_EnumObj _TAPP_NetworkAccessTypeTable[];
extern TAPP_EnumObj _TAPP_IsipUssdReasonTable[];
extern TAPP_EnumObj _TAPP_IsipPresReasonTable[];

/*
 * ======== TAPP_rptInit() ========
 *
 * Funtion to initialize report sub-module 
 *
 * Returns:
 *  None.
 */
void TAPP_rptInit()
{
    if (NULL == global_ptr) {
        return;
    }

    if (OSAL_FAIL == OSAL_fileOpen(&global_ptr->report.fileId,
            global_ptr->report.fileName, 
            OSAL_FILE_O_APPEND | OSAL_FILE_O_CREATE | 
            OSAL_FILE_O_WRONLY | OSAL_FILE_O_TRUNC,
            00755)) {
        global_ptr->report.fileId = -1;
    }

    /* Open xml report file. */
    if (OSAL_FAIL == OSAL_fileOpen(&global_ptr->xmlReport.fileId,
            global_ptr->xmlReport.fileName,
            OSAL_FILE_O_APPEND | OSAL_FILE_O_CREATE |
            OSAL_FILE_O_WRONLY | OSAL_FILE_O_TRUNC,
            00755)) {
        global_ptr->xmlReport.fileId = -1;
    }
    return;
}

/*
 * ======== TAPP_rptShutdown() ========
 *
 * Funtion to shutdown TAPP report sub-module 
 *
 * Returns:
 *  None.
 */
void TAPP_rptShutdown()
{
    if (NULL == global_ptr) {
        return;
    }

    if (-1 != global_ptr->report.fileId) {
        OSAL_fileClose(&global_ptr->report.fileId);
    }

    if (-1 != global_ptr->xmlReport.fileId) {
        OSAL_fileClose(&global_ptr->xmlReport.fileId);
    }

    return;
}

/*
 * ======== TAPP_rptXmlOutput() ========
 * Funtion to print formated string to xml report file
 *
 * Returns:
 *  None.
 */
void TAPP_rptXmlOutput(
    const char     *format_ptr,
    int             arg1,
    int             arg2,
    int             arg3)
{
    char strBuf[1024];
    vint size;

    if ((NULL == global_ptr) || (-1 == global_ptr->xmlReport.fileId)) {
        return;
    }

    size = OSAL_snprintf(strBuf, 1024, format_ptr, arg1, arg2, arg3);
    OSAL_fileWrite(&global_ptr->xmlReport.fileId, strBuf, &size);
}

/*
 * ======== TAPP_rptSetTestResult() ========
 * Set current test case result to TAPP_ReportResult.
 */
void TAPP_rptSetTestResult(
    TAPP_ReportResult *rpt_ptr,
    OSAL_Boolean       pass)
{
    rpt_ptr->testCase[rpt_ptr->totalCases].pass = pass;
    rpt_ptr->totalCases++;
    if (OSAL_FALSE == pass) {
        rpt_ptr->failureCases++;
    }
}

/*
 * ======== TAPP_rptSetTestName() ========
 * Set current test case name to TAPP_ReportResult.
 */
void TAPP_rptSetTestName(
    TAPP_ReportResult *rpt_ptr,
    char              *name_ptr)
{
    OSAL_strncpy(rpt_ptr->testCase[rpt_ptr->totalCases].name,
            name_ptr,
            sizeof(rpt_ptr->testCase[rpt_ptr->totalCases].name));
}

/*
 * ======== TAPP_rptOutput() ========
 *
 * Funtion to print formated string to report file
 *
 * Returns:
 *  None.
 */
void TAPP_rptOutput(
    const char     *format_ptr,
    int             arg1,
    int             arg2,
    int             arg3)
{
    char strBuf[1024];
    vint size;

    if ((NULL == global_ptr) || (-1 == global_ptr->report.fileId)) {
        return;
    }

    size = OSAL_snprintf(strBuf, 1024, format_ptr, arg1, arg2, arg3);
    OSAL_fileWrite(&global_ptr->report.fileId, strBuf, &size);
}

/*
 * ======== TAPP_rptOutputAT ========
 *
 * This function is used for printing AT command buffers
 *
 * RETURN:
 *     none
 */
void TAPP_rptOutputAT(
    const char *at_ptr)
{
    char c;
    int x = 0;
    char buffer[TAPP_AT_COMMAND_STRING_SZ];

    OSAL_snprintf(buffer, TAPP_AT_COMMAND_STRING_SZ, "%s", at_ptr);
    while (0 != *at_ptr) {
        c = *at_ptr++;
        if (c == '\r') {
            buffer[x] = '-';
        }
        else if  (c == '\n') {
            buffer[x] = '^';
        }
        else if  (c == ' ') {
            buffer[x] = '~';
        }
        else if  (c == 26) {
            /* Control-Z */
            buffer[x] = '#';
        }
        else {
            buffer[x] = c;
        }
        x++;
    }
    buffer[x] = 0;
    TAPP_rptOutput("AT:%s\n", (int)buffer, 0, 0);
}

/*
 * ======== TAPP_rptOutputCsm() ========
 *
 * Function to print Csm input event to report file
 *
 * Returns:
 *  None.
 */
void TAPP_rptOutputCsm(
    CSM_InputEvent *csm_ptr)
{
    if (CSM_EVENT_TYPE_RADIO == csm_ptr->type) {
        TAPP_rptOutput("\n    CSM Event Type: %s\n",
                (int)"CSM_EVENT_TYPE_RADIO", 0, 0);
        if (CSM_RADIO_REASON_IP_CHANGE ==
                csm_ptr->evt.radio.reason) {
            TAPP_rptOutput("        CSM Reason: %s\n", 
                    (int)"CSM_RADIO_REASON_IP_CHANGE", 0, 0);
            TAPP_rptOutput("        CSM IP: %s\n",
                    (int)csm_ptr->evt.radio.address, 0, 0);
            TAPP_rptOutput("        CSM Name: %s\n", 
                    (int)csm_ptr->evt.radio.infcName, 0, 0);
        }
        else if (CSM_RADIO_REASON_IP_CHANGE_EMERGENCY ==
                csm_ptr->evt.radio.reason) {
            TAPP_rptOutput("        CSM Reason: %s\n",
                    (int)"CSM_RADIO_REASON_IP_CHANGE_EMERGENCY", 0, 0);
            TAPP_rptOutput("        CSM IP: %s\n",
                    (int)csm_ptr->evt.radio.address, 0, 0);
            TAPP_rptOutput("        CSM Name: %s\n", 
                    (int)csm_ptr->evt.radio.infcName, 0, 0);
        }
        else if (CSM_RADIO_REASON_RADIO_CHANGE_EMERGENCY ==
                csm_ptr->evt.radio.reason) {
            TAPP_rptOutput("        CSM Reason: %s\n",
                    (int)"CSM_RADIO_REASON_RADIO_CHANGE_EMERGENCY", 0, 0);
            TAPP_rptOutput("        isEmergencyFailoverToCs: %d\n",
                    csm_ptr->evt.radio.isEmergencyFailoverToCs, 0, 0);
            TAPP_rptOutput("        isEmergencyRegRequired: %d\n",
                    csm_ptr->evt.radio.isEmergencyRegRequired, 0, 0);
        }
    }
}

/*
 * ======== TAPP_rptOutputIsip() ========
 *
 * Function to print ISIP_Message to report file
 *
 * Returns:
 *  None.
 */
void TAPP_rptOutputIsip(
    ISIP_Message *isip_ptr)
{
    char addr[OSAL_NET_IPV6_STR_MAX];

    TAPP_rptOutput("\n    ISIP Code:%s\n",
            (int)_TAPP_IsipCodeTable[isip_ptr->code].string,
            0, 0);
    TAPP_rptOutput("    ISIP Protocol:%d\n",
            (int)isip_ptr->protocol, 0, 0);
    if (ISIP_CODE_CALL == isip_ptr->code) {
        TAPP_rptOutput("        Reason:%s\n",
                (int)_TAPP_IsipCallReasonTable[
                isip_ptr->msg.call.reason].string, 0, 0);
        if (0 != isip_ptr->msg.call.reasonDesc) {
            TAPP_rptOutput("        reasonDesc:%s\n",
                  (int)isip_ptr->msg.call.reasonDesc, 0 ,0);
        }
        TAPP_rptOutput("        Status:%s\n",
                (int)_TAPP_IsipStatusTable[
                isip_ptr->msg.call.status].string, 0, 0);
        if (0 != isip_ptr->msg.call.from[0]) {
            TAPP_rptOutput("        from:%s\n",
                  (int)isip_ptr->msg.call.from, 0 ,0);
        }
        if (0 != isip_ptr->msg.call.to[0]) {
            TAPP_rptOutput("        to:%s\n",
                  (int)isip_ptr->msg.call.to, 0, 0);
        }
        if (0 != isip_ptr->msg.call.participants[0]) {
            TAPP_rptOutput("        participants:%s\n",
                  (int)isip_ptr->msg.call.participants, 0, 0);
        }
        TAPP_rptOutput("        type:%d\n",
                isip_ptr->msg.call.type,
                0, 0);
        TAPP_rptOutput("        Audio Dir:%d\n",
                (int) isip_ptr->msg.call.audioDirection,
                0, 0);
        TAPP_rptOutput("        Video Dir:%d\n",
                (int)isip_ptr->msg.call.videoDirection,
                0, 0);
        TAPP_rptOutput("        Resource status:%d\n",
                isip_ptr->msg.call.rsrcStatus,
                0, 0);
        if (ISIP_CALL_REASON_INITIATE ==
                isip_ptr->msg.call.reason) {
            TAPP_rptOutput("        Sesseion CID Type: %s\n",
                    (int)_TAPP_IsipSessionCidTypeTable[
                    isip_ptr->msg.call.cidType].string, 0, 0);
        }
    }
    else if (ISIP_CODE_SYSTEM == isip_ptr->code) {
        TAPP_rptOutput("        Reason:%s\n",
                (int)_TAPP_IsipSystemReasonTable[
                isip_ptr->msg.system.reason].string, 0, 0);
        TAPP_rptOutput("        Status:%s\n",
            (int)_TAPP_IsipStatusTable[
            isip_ptr->msg.system.status].string, 0, 0);
    }
    else if (ISIP_CODE_SERVICE == isip_ptr->code) {
        TAPP_rptOutput("        Reason:%s\n",
                (int)_TAPP_IsipServiceReasonTable[
                isip_ptr->msg.service.reason].string, 0, 0);
        TAPP_rptOutput("        Status:%s\n",
                (int)_TAPP_IsipStatusTable[
                isip_ptr->msg.service.status].string, 0, 0);
        TAPP_rptOutput("        serviceId(index):%d\n",
                isip_ptr->id, 0, 0);
        if (ISIP_SERVICE_REASON_NET == isip_ptr->msg.service.reason) {
            OSAL_netAddressToString(addr,
                    &isip_ptr->msg.service.settings.interface.address);
            TAPP_rptOutput("        Ip: %s\n", (int)addr, 0, 0);
        }
        else if (ISIP_SERVICE_REASON_SERVER == 
                isip_ptr->msg.service.reason) {
            TAPP_rptOutput("        server:%s\n",
                    (int)_TAPP_IsipServerTypeTable[
                    isip_ptr->msg.service.server].string, 0, 0);
        }
        else if (ISIP_SERVICE_REASON_AUTH == isip_ptr->msg.service.reason) {
            TAPP_rptOutput("        name:%s\n",
                    (int)isip_ptr->msg.service.settings.credentials.username,
                    0, 0);
            TAPP_rptOutput("        password:%s\n",
                    (int)isip_ptr->msg.service.settings.credentials.password,
                    0, 0);
            TAPP_rptOutput("        realm:%s\n",
                    (int)isip_ptr->msg.service.settings.credentials.realm,
                    0, 0);
        }
        else if (ISIP_SERVICE_REASON_EMERGENCY == 
                isip_ptr->msg.service.reason) {
            TAPP_rptOutput("        isEmergency:%d\n",
                    isip_ptr->msg.service.settings.isEmergency, 0, 0);
        }
        else if (ISIP_SERVICE_REASON_IMEI_URI == 
                isip_ptr->msg.service.reason) {
            TAPP_rptOutput("        imeiUri:%s\n",
                    (int)isip_ptr->msg.service.settings.imeiUri, 0, 0);
        }
        else if (ISIP_SERVICE_REASON_BSID == isip_ptr->msg.service.reason) {
            TAPP_rptOutput("        cgi-type:%s\n",
                    (int)_TAPP_NetworkAccessTypeTable[
                    isip_ptr->msg.service.settings.bsId.type].string, 0, 0);
            TAPP_rptOutput("        cgi:%s\n",
                    (int)isip_ptr->msg.service.settings.bsId.szBsId, 0, 0);
        }
        else if (ISIP_SERVICE_REASON_DEACTIVATE == isip_ptr->msg.service.reason) {
            TAPP_rptOutput("        reasonDesc:%s\n",
                    (int)isip_ptr->msg.service.reasonDesc, 0, 0);
        }
    }
    else if (ISIP_CODE_MESSAGE == isip_ptr->code) {
        TAPP_rptOutput("        Reason:%s\n",
                (int)_TAPP_IsipTextReasonTable[
                isip_ptr->msg.system.reason].string, 0, 0);
        TAPP_rptOutput("        Type: %d\n",
                (int)isip_ptr->msg.message.type, 0, 0);
        TAPP_rptOutput("        serviceId(index):%d\n",
            isip_ptr->msg.message.serviceId, 0, 0);
        TAPP_rptOutput("        ID: %s\n",
                (int)isip_ptr->msg.message.messageId, 0, 0);
        TAPP_rptOutput("        PDU Len: %d\n",
                (int)isip_ptr->msg.message.pduLen, 0, 0);
        TAPP_rptOutput("        to: %s\n",
                (int)isip_ptr->msg.message.to, 0, 0);
        TAPP_rptOutput("        from: %s\n",
                (int)isip_ptr->msg.message.from, 0, 0);
    }
    else if (ISIP_CODE_USSD == isip_ptr->code) {
        TAPP_rptOutput("        Reason:%s\n",
                (int)_TAPP_IsipUssdReasonTable[
                isip_ptr->msg.ussd.reason].string, 0, 0);
        TAPP_rptOutput("        serviceId(index):%d\n",
            isip_ptr->msg.ussd.serviceId, 0, 0);
        TAPP_rptOutput("        to: %s\n",
                (int)isip_ptr->msg.ussd.to, 0, 0);
        TAPP_rptOutput("        message: %s\n",
                (int)isip_ptr->msg.ussd.message, 0, 0);
    }
    else if (ISIP_CODE_PRESENCE== isip_ptr->code) {
        TAPP_rptOutput("        Reason:%s\n",
                (int)_TAPP_IsipPresReasonTable[
                isip_ptr->msg.presence.reason].string, 0, 0);
        TAPP_rptOutput("        serviceId(index):%d\n",
            isip_ptr->msg.presence.serviceId, 0, 0);
        TAPP_rptOutput("        chatId:%d\n",
            isip_ptr->msg.presence.chatId, 0, 0);
        TAPP_rptOutput("        presense string: %s\n",
                (int)isip_ptr->msg.presence.presence, 0, 0);
    }
}

/*
 * ======== TAPP_rptOutputAction() ========
 *
 * Funtion to print TAPP_Action to report file
 *
 * Returns:
 *  None.
 */
void TAPP_rptOutputAction(
    TAPP_Action *action_ptr)
{
    if (0 != action_ptr->testCaseName[0]) {
        /* It's a start of a test case, print the test case name */
        TAPP_rptOutput("\n\n**** Executing Test case: <%s> ****",
                (int)action_ptr->testCaseName, 0, 0);
        TAPP_rptSetTestName(&global_ptr->reportResult,
                action_ptr->testCaseName);
    }

    TAPP_rptOutput("\n[Action]: %s ",
            (int)((_TAPP_ActionTypeStrings[action_ptr->type])), 0, 0);

    /* Print out action */
    switch (action_ptr->type) {
        case TAPP_ACTION_TYPE_ISSUE_AT:
        case TAPP_ACTION_TYPE_ISSUE_GSM_AT:
            TAPP_rptOutputAT(action_ptr->msg.at);
            break;
        case TAPP_ACTION_TYPE_ISSUE_ISIP:
        case TAPP_ACTION_TYPE_VALIDATE_ISIP:
            TAPP_rptOutputIsip(&action_ptr->msg.isip);
            break;
        case TAPP_ACTION_TYPE_ISSUE_CSM:
            TAPP_rptOutputCsm(&action_ptr->msg.csm);
            break;
        case TAPP_ACTION_TYPE_ISSUE_XCAP:
            TAPP_rptOutput("xcap event error=%d\n", (int)action_ptr->msg.mockXcapEvt.error, 0, 0);
            break;
        case TAPP_ACTION_TYPE_VALIDATE_XCAP:
            TAPP_rptOutput("xcap command action op=%d type=%d\n",
                                (int)action_ptr->msg.mockXcapCmd.op,
                                (int)action_ptr->msg.mockXcapCmd.opType,
                                0);
            break;
        case TAPP_ACTION_TYPE_ISSUE_ISI_RPC:
        case TAPP_ACTION_TYPE_VALIDATE_ISI_GET_EVT:
        case TAPP_ACTION_TYPE_VALIDATE_ISI_RPC_RETURN:
            TAPP_rptOutput("RPC Func = %s\n", (int)_TAPP_RpcIsiFuncTypeTable[
                    action_ptr->msg.rpcMsg.funcType].string, 0, 0);
            break;
        case TAPP_ACTION_TYPE_PAUSE:
            TAPP_rptOutput("%d(ms)\n", (int)action_ptr->u.pause, 0, 0);
            break;
        case TAPP_ACTION_TYPE_VALIDATE_AT:
        case TAPP_ACTION_TYPE_VALIDATE_GSM_AT:
            TAPP_rptOutputAT(action_ptr->msg.at);
            break;
        case TAPP_ACTION_TYPE_CLEAN_ISIP:
            break;
        case TAPP_ACTION_TYPE_REPEAT_START:
            TAPP_rptOutput("repeat time:%d\n", (int)action_ptr->u.repeat, 0, 0);
            break;
        case TAPP_ACTION_TYPE_REPEAT_END:
            break;
        default:
            TAPP_rptOutput("Invalid action type.\n", 0, 0, 0);
            break;
    }
}

/*
 * ======== TAPP_rptOutputActionResult() ========
 *
 * Funtion to print the result of a TAPP_Action to report file
 *
 * Returns:
 *  None.
 */
void TAPP_rptOutputActionResult(
    TAPP_Action *action_ptr,
    TAPP_Event  *event_ptr,
    TAPP_Return  result)
{
    if (TAPP_PASS == result) {
        TAPP_rptOutput("=>PASS\n", 0, 0, 0);
        if (OSAL_TRUE == action_ptr->endOfTestCase) {
            TAPP_rptSetTestResult(&global_ptr->reportResult,
                    OSAL_TRUE);
        }
        return;
    }

    TAPP_rptOutput("=>FAIL\n", 0, 0, 0);
    /* Print out event */
    TAPP_rptOutput("  Received: %s ",
            (int)(_TAPP_EventTypeStrings[event_ptr->type]), 0, 0);
    /* Set to xml report .*/
    TAPP_rptSetTestResult(&global_ptr->reportResult, OSAL_FALSE);
    switch (event_ptr->type) {
        case TAPP_EVENT_TYPE_ISIP:
            TAPP_rptOutputIsip(&event_ptr->msg.isip);
            break;
        case TAPP_EVENT_TYPE_AT_INFC:
        case TAPP_EVENT_TYPE_GSM_DEV:
            TAPP_rptOutputAT(event_ptr->msg.at);
            break;
        case TAPP_EVENT_TYPE_XCAP:
            TAPP_rptOutput("xcap command event op=%d type=%d uri=%s\n",
                                (int)event_ptr->msg.xcapCmd.op,
                                (int)event_ptr->msg.xcapCmd.opType,
                                (int)event_ptr->msg.xcapCmd.uri_ptr);
            break;
        case TAPP_EVENT_TYPE_TIMEOUT:
            TAPP_rptOutput("\n", 0, 0, 0);
            break;
        case TAPP_EVENT_TYPE_ISI_RPC:
            TAPP_rptOutput("RPC Func = %s\n", (int)_TAPP_RpcIsiFuncTypeTable[
                    event_ptr->msg.rpc.funcType].string, 0, 0);
            break;
        default:
            TAPP_rptOutput("Invalid event type.\n\n", 0, 0, 0);
            break;
    }
}

/*
 * ========TAPP_rptGenXmlReport() ========
 * Generate xml report.
 */
void TAPP_rptGenXmlReport(
    TAPP_GlobalObj *global_ptr)
{
    TAPP_ReportResult *rpt_ptr = &global_ptr->reportResult;
    int               i;

    /* Output xml header and tag */
    TAPP_rptXmlOutput("    <testsuite name=\"%s\" tests = \"%02d\" "
            "failures=\"%02d\">\n",
            (int)global_ptr->testsuiteName, rpt_ptr->totalCases,
            rpt_ptr->failureCases);
    /* Output test case result. */
    for (i = 0; i < rpt_ptr->totalCases; i++) {
        if (rpt_ptr->testCase[i].pass) {
            TAPP_rptXmlOutput(
                    "        <testcase classname=\"%s\" name=\"%s\" "
                    "result=\"%s\"/>\n",
                    (int)(int)global_ptr->testsuiteName,
                    (int)rpt_ptr->testCase[i].name,
                    (int)"PASS");
        }
        else {
            /* Failure case */
            TAPP_rptXmlOutput(
                    "        <testcase classname=\"%s\" name=\"%s\" "
                    "result=\"%s\">\n",
                    (int)(int)global_ptr->testsuiteName,
                    (int)rpt_ptr->testCase[i].name,
                    (int)"FAIL");
            TAPP_rptXmlOutput(
                    "            <failure message=\"%s\" />\n"
                    "        </testcase>\n",
                    (int)"Test case failed", 0, 0);
        }
    }
        
    TAPP_rptXmlOutput("    </testsuite>\n", 0, 0, 0);
}

