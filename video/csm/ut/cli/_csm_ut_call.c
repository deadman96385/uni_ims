/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004-2010 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 18777 $ $Date: 2012-11-09 01:47:11 -0800 (Fri, 09 Nov 2012) $
 *
 */

#include <osal.h>
#include <csm_event.h>
#include "_csm_ut.h"
#include "_csm_ut_utils.h"

vint CSM_UT_callRun = 1;

/*
 * ======== _CSM_UT_callResourceReady ========
 *
 * Generate a resource ready event and write to CSM.
 */
UT_Return _CSM_UT_callResourceReady(
    int arg0)
{
    CSM_InputEvent     csmEvt_ptr;
    CSM_InputCall     *callEvt_ptr;
    char               buf[40];

    callEvt_ptr = &csmEvt_ptr.evt.call;
    csmEvt_ptr.type = CSM_EVENT_TYPE_CALL;
    callEvt_ptr->reason = CSM_CALL_REASON_RESOURCE_INDICATION,
    OSAL_strncpy(callEvt_ptr->reasonDesc,
            "CSM_CALL_REASON_RESOURCE_INDICATION,", CSM_EVENT_STRING_SZ);
    callEvt_ptr->type = CSM_CALL_EVENT_TYPE_AT;
    callEvt_ptr->isEmergency = 0;
    callEvt_ptr->extraArgument = 0;

    OSAL_logMsg("\nAudio resource (0: Not ready, 1: Ready) : ");
    _CSM_UT_getLine(buf, 2);

    if (OSAL_atoi(buf) == 1) {
        callEvt_ptr->u.resourceStatus.audioReady = 1;
    }
    else {
        callEvt_ptr->u.resourceStatus.audioReady = 0;
    }

    OSAL_logMsg("\nVideo resource (0: Not ready, 1: Ready) : ");
    _CSM_UT_getLine(buf, 2);

    if (OSAL_atoi(buf) == 1) {
        callEvt_ptr->u.resourceStatus.videoReady = 1;
    }
    else {
        callEvt_ptr->u.resourceStatus.videoReady = 0;
    }

    CSM_UT_writeCsmEvent(&csmEvt_ptr);    

    return UT_PASS;
}

/*
 * ======== _CSM_UT_callSrvccStart ========
 *
 * Generate a SRVCC start event and write to CSM.
 */
UT_Return _CSM_UT_callSrvccStart(
    int arg0)
{
    CSM_InputEvent     csmEvt_ptr;
    CSM_InputCall     *callEvt_ptr;

    callEvt_ptr = &csmEvt_ptr.evt.call;
    csmEvt_ptr.type = CSM_EVENT_TYPE_CALL;
    callEvt_ptr->reason = CSM_CALL_REASON_AT_CMD_SRVCC_START;
    OSAL_strncpy(callEvt_ptr->reasonDesc,
            "CSM_CALL_REASON_AT_CMD_SRVCC_START", CSM_EVENT_STRING_SZ);
    callEvt_ptr->type = CSM_CALL_EVENT_TYPE_AT;
    callEvt_ptr->isEmergency = 0;
    callEvt_ptr->extraArgument = 0;
    callEvt_ptr->u.remoteAddress[0] = 0;

    CSM_UT_writeCsmEvent(&csmEvt_ptr);

    return UT_PASS;
}

/*
 * ======== _CSM_UT_callSrvccSuccess ========
 *
 * Generate a SRVCC success event and write to CSM.
 */
UT_Return _CSM_UT_callSrvccSuccess(
    int arg0)
{
    CSM_InputEvent     csmEvt_ptr;
    CSM_InputCall     *callEvt_ptr;
    char               buf[40];

    callEvt_ptr = &csmEvt_ptr.evt.call;
    csmEvt_ptr.type = CSM_EVENT_TYPE_CALL;
    callEvt_ptr->reason = CSM_CALL_REASON_AT_CMD_SRVCC_SUCCESS;
    OSAL_strncpy(callEvt_ptr->reasonDesc,
            "CSM_CALL_REASON_AT_CMD_SRVCC_SUCCESS", CSM_EVENT_STRING_SZ);
    callEvt_ptr->type = CSM_CALL_EVENT_TYPE_AT;
    callEvt_ptr->isEmergency = 0;
    callEvt_ptr->extraArgument = 0;
    callEvt_ptr->u.remoteAddress[0] = 0;

    OSAL_logMsg("\nGm status (0: Not available, 1: Available) : ");
    _CSM_UT_getLine(buf, 2);

    if (OSAL_atoi(buf) == 1) {
        callEvt_ptr->extraArgument = 1;
    }
    else {
        callEvt_ptr->extraArgument = 0;
    }

    CSM_UT_writeCsmEvent(&csmEvt_ptr);

    return UT_PASS;
}

/*
 * ======== _CSM_UT_callSrvccFailure ========
 *
 * Generate a SRVCC failure event and write to CSM.
 */
UT_Return _CSM_UT_callSrvccFailure(
    int arg0)
{
    CSM_InputEvent     csmEvt_ptr;
    CSM_InputCall     *callEvt_ptr;

    callEvt_ptr = &csmEvt_ptr.evt.call;
    csmEvt_ptr.type = CSM_EVENT_TYPE_CALL;
    callEvt_ptr->reason = CSM_CALL_REASON_AT_CMD_SRVCC_FAILED;
    OSAL_strncpy(callEvt_ptr->reasonDesc,
            "CSM_CALL_REASON_AT_CMD_SRVCC_FAILED", CSM_EVENT_STRING_SZ);
    callEvt_ptr->type = CSM_CALL_EVENT_TYPE_AT;
    callEvt_ptr->isEmergency = 0;
    callEvt_ptr->extraArgument = 0;
    callEvt_ptr->u.remoteAddress[0] = 0;

    CSM_UT_writeCsmEvent(&csmEvt_ptr);

    return UT_PASS;
}

/*
 * ======== _CSM_UT_callDial ========
 *
 * Generate a ATD event, get the dial number, and write to CSM
 */
UT_Return _CSM_UT_callDial(
    int arg0)
{
    CSM_InputEvent     csmEvt_ptr;
    CSM_InputCall     *callEvt_ptr;
    char               buf[40];

    callEvt_ptr = &csmEvt_ptr.evt.call;
    csmEvt_ptr.type = CSM_EVENT_TYPE_CALL;
    callEvt_ptr->reason = CSM_CALL_REASON_AT_CMD_DIAL;
    OSAL_strncpy(callEvt_ptr->reasonDesc,
            "CSM_EVENT_CALL_REASON_AT_CMD_DIAL", CSM_EVENT_STRING_SZ);
    callEvt_ptr->type = CSM_CALL_EVENT_TYPE_AT;
    callEvt_ptr->isEmergency = 0;
    callEvt_ptr->extraArgument = 0;
    callEvt_ptr->u.remoteAddress[0] = 0;
    callEvt_ptr->u.digit = 'x';
    OSAL_logMsg("\nCall Session Type (0: Voice, 1: Video) : ");
    _CSM_UT_getLine(buf, 2);
    if (OSAL_atoi(buf) == 1) {
        callEvt_ptr->callSessionType = CSM_CALL_SESSION_TYPE_VIDEO | CSM_CALL_SESSION_TYPE_AUDIO;
    }
    else {
        callEvt_ptr->callSessionType = CSM_CALL_SESSION_TYPE_AUDIO;
    }   

    OSAL_logMsg("\nCall to : ");
    _CSM_UT_getLine(buf, 40);
    OSAL_snprintf(callEvt_ptr->u.remoteAddress, CSM_EVENT_STRING_SZ, "%s", buf);

    CSM_UT_writeCsmEvent(&csmEvt_ptr);    

    return UT_PASS;
}

/*
 * ======== _CSM_UT_callAnswer ========
 *
 * Generate a ATA event and write to CSM
 */
UT_Return _CSM_UT_callAnswer(
    int arg0)
{
    CSM_InputEvent     csmEvt_ptr;
    CSM_InputCall     *callEvt_ptr;

    callEvt_ptr = &csmEvt_ptr.evt.call;
    csmEvt_ptr.type = CSM_EVENT_TYPE_CALL;
    callEvt_ptr->reason = CSM_CALL_REASON_AT_CMD_ANSWER;
    OSAL_strncpy(callEvt_ptr->reasonDesc,
            "CSM_EVENT_CALL_REASON_AT_CMD_ANSWER", CSM_EVENT_STRING_SZ);
    callEvt_ptr->type = CSM_CALL_EVENT_TYPE_AT;
    callEvt_ptr->extraArgument = 0;
    callEvt_ptr->u.remoteAddress[0] = 0;
    callEvt_ptr->u.digit = 'x';

    CSM_UT_writeCsmEvent(&csmEvt_ptr);
    return UT_PASS;
}

/*
 * ======== _CSM_UT_callEnd ========
 *
 * Generate a ATH event and write to CSM
 */
UT_Return _CSM_UT_callEnd(
    int arg0)
{
    CSM_InputEvent     csmEvt_ptr;
    CSM_InputCall     *callEvt_ptr;

    callEvt_ptr = &csmEvt_ptr.evt.call;
    csmEvt_ptr.type = CSM_EVENT_TYPE_CALL;
    callEvt_ptr->reason = CSM_CALL_REASON_AT_CMD_END;
    OSAL_strncpy(callEvt_ptr->reasonDesc,
            "", CSM_EVENT_STRING_SZ);
    callEvt_ptr->type = CSM_CALL_EVENT_TYPE_AT;
    callEvt_ptr->extraArgument = 0;
    callEvt_ptr->u.remoteAddress[0] = 0;
    callEvt_ptr->u.digit = 'x';

    CSM_UT_writeCsmEvent(&csmEvt_ptr);
    return UT_PASS;
}

/*
 * ======== _CSM_UT_callEndHeld ========
 *
 * Generate a AT+CHLD=0 event and write to CSM
 */
UT_Return _CSM_UT_callEndHeld(
    int arg0)
{
    CSM_InputEvent     csmEvt_ptr;
    CSM_InputCall     *callEvt_ptr;

    callEvt_ptr = &csmEvt_ptr.evt.call;
    csmEvt_ptr.type = CSM_EVENT_TYPE_CALL;
    callEvt_ptr->reason = CSM_CALL_REASON_AT_CMD_END_ALL_HELD_OR_WAITING;
    OSAL_strncpy(callEvt_ptr->reasonDesc,
            "CSM_CALL_REASON_AT_CMD_END_ALL_HELD_OR_WAITING",
            CSM_EVENT_STRING_SZ);
    callEvt_ptr->type = CSM_CALL_EVENT_TYPE_AT;
    callEvt_ptr->extraArgument = 0;
    callEvt_ptr->u.remoteAddress[0] = 0;
    callEvt_ptr->u.digit = 'x';

    CSM_UT_writeCsmEvent(&csmEvt_ptr);
    return UT_PASS;
}

/*
 * ======== _CSM_UT_callEndActive ========
 *
 * Generate a AT+CHLD=1 event and write to CSM
 */
UT_Return _CSM_UT_callEndActive(
    int arg0)
{
    CSM_InputEvent     csmEvt_ptr;
    CSM_InputCall     *callEvt_ptr;

    callEvt_ptr = &csmEvt_ptr.evt.call;
    csmEvt_ptr.type = CSM_EVENT_TYPE_CALL;
    callEvt_ptr->reason = CSM_CALL_REASON_AT_CMD_END_ALL_ACTIVE;
    OSAL_strncpy(callEvt_ptr->reasonDesc,
            "CSM_CALL_REASON_AT_CMD_END_ALL_ACTIVE", CSM_EVENT_STRING_SZ);
    callEvt_ptr->type = CSM_CALL_EVENT_TYPE_AT;
    callEvt_ptr->extraArgument = 0;
    callEvt_ptr->u.remoteAddress[0] = 0;
    callEvt_ptr->u.digit = 'x';

    CSM_UT_writeCsmEvent(&csmEvt_ptr);
    return UT_PASS;
}

/*
 * ======== _CSM_UT_callSwap ========
 *
 * Generate a AT+CHLD=2 event and write to CSM
 */
UT_Return _CSM_UT_callSwap(
    int arg0)
{
    CSM_InputEvent     csmEvt_ptr;
    CSM_InputCall     *callEvt_ptr;

    callEvt_ptr = &csmEvt_ptr.evt.call;
    csmEvt_ptr.type = CSM_EVENT_TYPE_CALL;
    callEvt_ptr->reason = CSM_CALL_REASON_AT_CMD_SWAP;
    OSAL_strncpy(callEvt_ptr->reasonDesc,
            "CSM_CALL_REASON_AT_CMD_SWAP", CSM_EVENT_STRING_SZ);
    callEvt_ptr->type = CSM_CALL_EVENT_TYPE_AT;
    callEvt_ptr->extraArgument = 0;
    callEvt_ptr->u.remoteAddress[0] = 0;
    callEvt_ptr->u.digit = 'x';

    CSM_UT_writeCsmEvent(&csmEvt_ptr);
    return UT_PASS;
}

/*
 * ======== _CSM_UT_callMerge ========
 *
 * Generate a AT+CHLD=3 event and write to CSM
 */
UT_Return _CSM_UT_callMerge(
    int arg0)
{
    CSM_InputEvent     csmEvt_ptr;
    CSM_InputCall     *callEvt_ptr;

    callEvt_ptr = &csmEvt_ptr.evt.call;
    csmEvt_ptr.type = CSM_EVENT_TYPE_CALL;
    callEvt_ptr->reason = CSM_CALL_REASON_AT_CMD_CONFERENCE;
    OSAL_strncpy(callEvt_ptr->reasonDesc,
            "CSM_CALL_REASON_AT_CMD_CONFERENCE", CSM_EVENT_STRING_SZ);
    callEvt_ptr->type = CSM_CALL_EVENT_TYPE_AT;
    callEvt_ptr->extraArgument = 0;
    callEvt_ptr->u.remoteAddress[0] = 0;
    callEvt_ptr->u.digit = 'x';

    CSM_UT_writeCsmEvent(&csmEvt_ptr);
    return UT_PASS;
}

/*
 * ======== _CSM_UT_callReleseX ========
 *
 * Generate a AT+CHLD=1x event, get call index, and write to CSM
 */
UT_Return _CSM_UT_callReleaseX(
    int arg0)
{
    CSM_InputEvent     csmEvt_ptr;
    CSM_InputCall     *callEvt_ptr;
    char               buf[2];

    callEvt_ptr = &csmEvt_ptr.evt.call;
    csmEvt_ptr.type = CSM_EVENT_TYPE_CALL;
    callEvt_ptr->reason = CSM_CALL_REASON_AT_CMD_RELEASE_AT_X;
    OSAL_strncpy(callEvt_ptr->reasonDesc,
            "User Triggered", CSM_EVENT_STRING_SZ);
    callEvt_ptr->type = CSM_CALL_EVENT_TYPE_AT;
    callEvt_ptr->isEmergency = 0;
    callEvt_ptr->extraArgument = 0;
    callEvt_ptr->u.remoteAddress[0] = 0;
    callEvt_ptr->u.digit = 'x';

    OSAL_logMsg("\nIndex : ");
    _CSM_UT_getLine(buf, 2);
    callEvt_ptr->u.callIndex = OSAL_atoi(buf);

    CSM_UT_writeCsmEvent(&csmEvt_ptr);

    return UT_PASS;
}

/*
 * ======== _CSM_UT_callHoldExceptX ========
 *
 * Generate a AT+CHLD=2x event, get call index, and write to CSM
 */
UT_Return _CSM_UT_callHoldExceptX(
    int arg0)
{
    CSM_InputEvent     csmEvt_ptr;
    CSM_InputCall     *callEvt_ptr;
    char               buf[2];

    callEvt_ptr = &csmEvt_ptr.evt.call;
    csmEvt_ptr.type = CSM_EVENT_TYPE_CALL;
    callEvt_ptr->reason = CSM_CALL_REASON_AT_CMD_HOLD_ALL_EXCEPT_X;
    OSAL_strncpy(callEvt_ptr->reasonDesc,
            "CSM_CALL_REASON_AT_CMD_HOLD_ALL_EXCEPT_X", CSM_EVENT_STRING_SZ);
    callEvt_ptr->type = CSM_CALL_EVENT_TYPE_AT;
    callEvt_ptr->isEmergency = 0;
    callEvt_ptr->extraArgument = 0;
    callEvt_ptr->u.remoteAddress[0] = 0;
    callEvt_ptr->u.digit = 'x';

    OSAL_logMsg("\nIndex : ");
    _CSM_UT_getLine(buf, 2);
    callEvt_ptr->u.callIndex = OSAL_atoi(buf);

    CSM_UT_writeCsmEvent(&csmEvt_ptr);

    return UT_PASS;
}

/*
 * ======== _CSM_UT_callClcc ========
 *
 * Generate a AT+CLCC event and write to CSM
 */
UT_Return _CSM_UT_callClcc(
    int arg0)
{
    CSM_InputEvent     csmEvt_ptr;
    CSM_InputCall     *callEvt_ptr;

    callEvt_ptr = &csmEvt_ptr.evt.call;
    csmEvt_ptr.type = CSM_EVENT_TYPE_CALL;
    callEvt_ptr->reason = CSM_CALL_REASON_AT_CMD_REPORT;
    OSAL_strncpy(callEvt_ptr->reasonDesc,
            "CSM_CALL_REASON_AT_CMD_REPORT", CSM_EVENT_STRING_SZ);
    callEvt_ptr->type = CSM_CALL_EVENT_TYPE_AT;
    callEvt_ptr->extraArgument = 0;
    callEvt_ptr->u.remoteAddress[0] = 0;
    callEvt_ptr->u.digit = 'x';

    CSM_UT_writeCsmEvent(&csmEvt_ptr);
    return UT_PASS;
}

/*
 * ======== _CSM_UT_callDtmf ========
 *
 * Generate a VTS(DTMF) event, get the digit, and write to CSM
 */
UT_Return _CSM_UT_callDtmf(
    int arg0)
{
    CSM_InputEvent     csmEvt_ptr;
    CSM_InputCall     *callEvt_ptr;
    char               buf[2];

    callEvt_ptr = &csmEvt_ptr.evt.call;
    csmEvt_ptr.type = CSM_EVENT_TYPE_CALL;
    callEvt_ptr->reason = CSM_CALL_REASON_AT_CMD_DIGIT;
    OSAL_strncpy(callEvt_ptr->reasonDesc,
            "CSM_EVENT_CALL_REASON_AT_CMD_DIGIT", CSM_EVENT_STRING_SZ);
    callEvt_ptr->type = CSM_CALL_EVENT_TYPE_AT;
    callEvt_ptr->isEmergency = 0;
    callEvt_ptr->extraArgument = 500;
    callEvt_ptr->u.remoteAddress[0] = 0;

    OSAL_logMsg("\nDigit : ");
    _CSM_UT_getLine(buf, 2);
    callEvt_ptr->u.digit = buf[0];

    CSM_UT_writeCsmEvent(&csmEvt_ptr);

    return UT_PASS;
}

/*
 * ======== _CSM_UT_callQuit ========
 *
 * Quit the call menu and return to main menu
 */
static UT_Return _CSM_UT_callQuit(
    int arg0)
{
    CSM_UT_callRun = 0;
    return UT_PASS;
}

/*
 * ======== _CSM_UT_callAnswer ========
 *
 * Generate a ATA event and write to CSM
 */
UT_Return _CSM_UT_callMediaControl(
    int arg0)
{
    CSM_InputEvent     csmEvt_ptr;
    CSM_InputCall     *callEvt_ptr;
    char               buf[40];

    callEvt_ptr = &csmEvt_ptr.evt.call;
    csmEvt_ptr.type = CSM_EVENT_TYPE_CALL;
    callEvt_ptr->reason = CSM_CALL_REASON_AT_CMD_MEDIA_CONTROL;
    OSAL_strncpy(callEvt_ptr->reasonDesc,
            "CSM_CALL_REASON_AT_CMD_MEDIA_CONTROL", CSM_EVENT_STRING_SZ);
    callEvt_ptr->type = CSM_CALL_EVENT_TYPE_AT;
    callEvt_ptr->extraArgument = 0;
    callEvt_ptr->u.remoteAddress[0] = 0;
    callEvt_ptr->u.digit = 'x';
    OSAL_logMsg("\nNegStatus (1: Unconditional, 2: Proposed, 3: Accepted, 4: Rejected) : ");
    _CSM_UT_getLine(buf, 2);
    callEvt_ptr->negStatus = OSAL_atoi(buf);
    if (callEvt_ptr->negStatus == 1 || callEvt_ptr->negStatus == 2) {
        OSAL_logMsg("\nCall Session Type (0: Voice, 1: Video) : ");
        _CSM_UT_getLine(buf, 2);
        if (OSAL_atoi(buf) == 1) {
            callEvt_ptr->callSessionType = CSM_CALL_SESSION_TYPE_VIDEO | CSM_CALL_SESSION_TYPE_AUDIO;
        }
        else {
            callEvt_ptr->callSessionType = CSM_CALL_SESSION_TYPE_AUDIO;
        }   
    }    

    CSM_UT_writeCsmEvent(&csmEvt_ptr);
    return UT_PASS;
}


CSM_UT_TestTableItem CSM_UT_callTable[] =
{
    { "atd",       _CSM_UT_callDial,        "Dial an outgoing call"},
    { "ata",       _CSM_UT_callAnswer,      "Answer an incoming call"},
    { "ath",       _CSM_UT_callEnd,         "End all call"},
    { "chld=0",    _CSM_UT_callEndHeld,     "End all held call"},
    { "chld=1",    _CSM_UT_callEndActive,   "End active call"},
    { "chld=2",    _CSM_UT_callSwap,        "Call swap"},
    { "chld=3",    _CSM_UT_callMerge,       "Merge all held call to conference"},
    { "chld=1x",   _CSM_UT_callReleaseX,    "Release indicated call"},
    { "chld=2x",   _CSM_UT_callHoldExceptX, "Hold all call except indicated call"},
    { "clcc",      _CSM_UT_callClcc,        "Report call status"},
    { "vts",       _CSM_UT_callDtmf,        "Send DTMF tone"},
    { "q",         _CSM_UT_callQuit,        "Quit (exit call menu"},
    { "atmc",      _CSM_UT_callMediaControl,"Send a media control"},
    { "cireph:0",  _CSM_UT_callSrvccStart,  "Send a SRVCC start command"},
    { "cireph:1",  _CSM_UT_callSrvccSuccess,"Send a SRVCC success command"},
    { "cireph:2",  _CSM_UT_callSrvccFailure,"Send a SRVCC failure command"},
    { "rsrc",      _CSM_UT_callResourceReady, "Send audio/video resource status"},
};

/*
 * ======== CSM_UT_call ========
 *
 * The main menu for the call opertion
 */
UT_Return CSM_UT_call (
)
{
    CSM_UT_callRun = 1;
    vint              item;
    vint              itemMax;
    CSM_UT_TestTableItem *item_ptr;
    CSM_UT_TestFunc      *func_ptr;
    int32             arg;
    char              buf[10];
    vint              printMenu;

    printMenu = 1;
    OSAL_taskDelay(500);    /* cleans up printing */
    itemMax = sizeof(CSM_UT_callTable)/sizeof(CSM_UT_callTable[0]);
    while (CSM_UT_callRun) {
        if (printMenu > 0) {
            printMenu = 0;
            OSAL_logMsg("\n"
                    "===========================\n"
                    "D2 CSM call test suite\n"
                    "===========================\n"
                    "Command  Description\n"
                    "-------  -----------------\n");

            for (item = 0; item < itemMax; item++) {
                item_ptr = &CSM_UT_callTable[item];
                OSAL_logMsg("%-9s%s\n", item_ptr->cmd, item_ptr->desc);
            }
        }

        OSAL_logMsg("\nCmd: ");
        _CSM_UT_getLine(buf, 2 * sizeof(CSM_UT_callTable[0].cmd));
        for (item = 0, func_ptr = NULL; item < itemMax; item++) {
            item_ptr = &CSM_UT_callTable[item];
            if (0 == OSAL_strncmp(buf, (char *)item_ptr->cmd, sizeof(buf))) {
                func_ptr = item_ptr->func_ptr;
                break;
            }
        }

        if (NULL != func_ptr) {
            OSAL_logMsg("\n"
                    "==============================================\n"
                    "%s\n"
                    "==============================================\n",
                    item_ptr->desc);
            arg = 0;
            func_ptr(arg);
            printMenu = 0;
        }
        else {
            /* Unknown cmd, print help */
            printMenu = 1;
        }        
    }
    OSAL_logMsg("\n%s:%d  UT_menu return\n", __FILE__, __LINE__);

    return UT_PASS;
}

