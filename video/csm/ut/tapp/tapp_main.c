/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2006 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 29252 $ $Date: 2014-10-10 08:15:06 +0800 (Fri, 10 Oct 2014) $
 */

#include <osal.h>
#include <isi.h>
#include <isip.h>
#include <csm_event.h>
#include "_tapp.h"
#include "_tapp_report.h"
#include "_tapp_mock_sapp.h"
#include "_tapp_mock_vpmd.h"
#include "_tapp_mock_gsm.h"
#include "_tapp_mock_rir.h"
#include "_tapp_mock_xcap.h"
#include "_tapp_at_infc.h"
#include <ezxml_mem.h>
/* TAPP Global Object pointer */
TAPP_GlobalObj *global_ptr = NULL;

#define D2_VPORT_REVISION D2_Release_CSM

extern char const D2_Release_CSM[];
extern char const D2_Release_CSM_UT_TAPP[];

/* 
 * ======== main() ========
 *
 * This function is the "main" process entry point. It initializes 
 * sub-modules, threads, and loads any configuration data needed 
 *
 * Returns: 
 *   Nothing and never.
 */
OSAL_ENTRY
{
    TAPP_Return  testResult;
    TAPP_Return  finalResult;
    vint         idx;
    TAPP_Action *action_ptr;
    char         strBuf[32];
    vint         strLen;

    testResult = TAPP_PASS;
    finalResult = TAPP_PASS;
    
    if (4 > argc) {
        OSAL_logMsg ("\n\nStart Error --- Usage is: %s <osal queue path> "
                "<init file> "
                "<test case file> [optional more test cases files]\n\n",
            argv_ptr[0]);
        return (-1);
    }
    /* Create the message queues at a given location */
    OSAL_strcpy(OSAL_msgQPathName, argv_ptr[1]);

    EZXML_init();

    /* Initialize TAPP */
    if (TAPP_PASS != TAPP_init(&global_ptr, argv_ptr[2])) {
        OSAL_logMsg("%s:%d TAPP init failed\n", __FILE__, __LINE__);
        return (-1);
    }

    strLen = sizeof(strBuf);
    OSAL_timeGetISO8601((uint8 *)strBuf, &strLen);
    TAPP_rptOutput("================================\n", 0, 0, 0);
    TAPP_rptOutput("TAPP Test Result\n", 0, 0, 0);
    TAPP_rptOutput("%s\n", (int)strBuf, 0, 0);
    TAPP_rptOutput("CSM Revision:%s\n", (int)D2_Release_CSM, 0, 0);
    TAPP_rptOutput("TAPP Revision:%s\n", (int)D2_Release_CSM_UT_TAPP, 0, 0);
    TAPP_rptOutput("Test Suite:%s\n", (int)global_ptr->testsuiteName, 0, 0);
    TAPP_rptOutput("================================\n", 0, 0, 0);

    /* Read and validate test cases */
    for (idx = 3; idx < argc; idx++) {
        if (TAPP_PASS != TAPP_readTestCase(global_ptr, argv_ptr[idx])) {
            TAPP_rptOutput("!!!!!!TAPP terminated!!!!!!\n", 0, 0, 0);
            OSAL_logMsg("%s:%d Read test case failed\n", __FILE__, __LINE__);
            goto _TAPP_MAIN_SHUTDOWN;
        }
    }

    /* Get next action */
    while (TAPP_PASS == TAPP_getNextAction(global_ptr, &action_ptr)) {
        /* Got an action, dispatch it */
        TAPP_rptOutputAction(action_ptr);
        TAPP_dbgPrintf("Execute action:");
        _TAPP_printAction(action_ptr);
        testResult = TAPP_PASS;
        switch (action_ptr->type) {
            case TAPP_ACTION_TYPE_ISSUE_ISIP:
                testResult = TAPP_mockSappIssueIsip(global_ptr,
                        &action_ptr->msg.isip);
                break; 
            case TAPP_ACTION_TYPE_VALIDATE_ISIP:
            case TAPP_ACTION_TYPE_CLEAN_ISIP:
                testResult = TAPP_mockSappValidateIsip(global_ptr,
                        action_ptr);
                break;
            case TAPP_ACTION_TYPE_ISSUE_XCAP:
                testResult = TAPP_mockXcapIssueEvt(global_ptr,
                        &action_ptr->msg.mockXcapEvt);
                break; 
            case TAPP_ACTION_TYPE_VALIDATE_XCAP:
                testResult = TAPP_mockXcapValidateCmd(global_ptr,
                        action_ptr);
                break;
            case TAPP_ACTION_TYPE_ISSUE_AT:
                testResult = TAPP_atInfcIssueAt(global_ptr,
                        action_ptr->msg.at);
                break; 
            case TAPP_ACTION_TYPE_VALIDATE_AT:
                testResult = TAPP_atInfcValidateAt(global_ptr, action_ptr);
                break; 
            case TAPP_ACTION_TYPE_ISSUE_GSM_AT:
                testResult = TAPP_mockGsmIssueAt(global_ptr,
                        action_ptr->msg.at);
                break; 
            case TAPP_ACTION_TYPE_VALIDATE_GSM_AT:
                testResult = TAPP_mockGsmValidateAt(global_ptr, action_ptr);
                break; 
            case TAPP_ACTION_TYPE_ISSUE_CSM:
                testResult = TAPP_mockRirIssueCsm(global_ptr, action_ptr);
                break; 
            case TAPP_ACTION_TYPE_PAUSE:
                OSAL_taskDelay(action_ptr->u.pause);
                break;
            case TAPP_ACTION_TYPE_REPEAT_START:
                TAPP_processRepeatStart(global_ptr, action_ptr);
                break;
            case TAPP_ACTION_TYPE_REPEAT_END:
                TAPP_processRepeatEnd(global_ptr, action_ptr);
                break;
            case TAPP_ACTION_TYPE_ISSUE_ISI_RPC:
                testResult = TAPP_mockVpmdIssueIsiRpc(global_ptr,
                        action_ptr);
                break; 
            case TAPP_ACTION_TYPE_VALIDATE_ISI_RPC_RETURN:
                testResult = TAPP_mockVpmdValidateIsiRpc(global_ptr,
                        action_ptr);
                break;
            case TAPP_ACTION_TYPE_VALIDATE_ISI_GET_EVT:
                testResult = TAPP_mockVpmdValidateIsiGetEvt(global_ptr,
                        action_ptr);
                break;
            case TAPP_ACTION_TYPE_CLEAN_ISI_EVT:
                testResult = TAPP_mockVpmdCleanIsiEvt(global_ptr,
                        action_ptr);
                break;
            default:
                OSAL_logMsg("%s:%d Invalid action type:%d\n",
                        __FILE__, __LINE__, action_ptr->type);
                testResult = TAPP_FAIL;
                break;
        }

        TAPP_rptOutputActionResult(action_ptr, &global_ptr->tappEvt,
                testResult);
        if (TAPP_FAIL == testResult) {
            /* Set final result to fail if one test case failed. */
            finalResult = TAPP_FAIL;
            if (0 == action_ptr->stop) {
                /* continue next test case  */
                continue;
            }
            /* Terminate the loop on failure */
            TAPP_rptOutput("!!! Test Failed - TAPP Terminated !!!\n", 0, 0, 0);
            break;
        }
    }

    TAPP_rptOutput("\n*****************\nTest Result:%s\n*****************\n",
            (int)((TAPP_PASS == finalResult)?"PASS":"FAIL"), 0, 0);
    OSAL_logMsg("%s:%d Exiting TAPP. Test result:%d\n", __FILE__, __LINE__,
            finalResult);

    TAPP_rptGenXmlReport(global_ptr);
_TAPP_MAIN_SHUTDOWN:
    /* Shutdown TAPP */
    TAPP_shutdown(global_ptr);
    
    /* Destroy EZXML  */
    EZXML_destroy();

    /* Return the test result */
    if (TAPP_PASS == finalResult) {
        return (0);
    }
    return (-1);
}
OSAL_EXIT

