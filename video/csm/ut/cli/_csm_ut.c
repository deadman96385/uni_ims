/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004-2010 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 20293 $ $Date: 2013-04-01 16:07:12 +0800 (週一, 01 四月 2013) $
 *
 */

#include <osal.h>
#include <csm_main.h>
#include <csm_event.h>
#include <ezxml.h>
#include "_csm_ut.h"
#include "_csm_ut_call.h"
#include "_csm_ut_sms.h"
#include "_csm_ut_ussd.h"
#include "_csm_ut_event.h"
#include "_csm_ut_utils.h"
#include "_csm_ut_supsrv.h"
#include "_csm_ut_service.h"
#include "_csm_ut_emergency.h"

#define CSM_XML_DOC_PATH_NAME_SIZE  (128)
#define CSM_UT_MAX_QUEUE_DEPTH      (16)

uvint CSM_UT_run = 1;
CSM_UT_GlobalObj *global_ptr = NULL;

/*
 * ======== CSMUT_ipEventSend()  ========
 * This function is used to send a new IP event to CSM queue.
 *
 * param addr : The new address to update.
 * param isEmergency : The IP is for emergency or not.
 *
 * Return:
 *
 */
void CSMUT_ipEventSend(
    char   *addr_ptr,
    int     isEmergency)
{
    CSM_InputEvent event;

    OSAL_logMsg("%s:%d\n", __FUNCTION__, __LINE__);

    /* Send the IP change event to CSM_InputEvent */
    event.type = CSM_EVENT_TYPE_RADIO;
    if (isEmergency == 0) {
        event.evt.radio.reason = CSM_RADIO_REASON_IP_CHANGE;
    }
    else {
        event.evt.radio.reason = CSM_RADIO_REASON_IP_CHANGE_EMERGENCY;
    }

    event.evt.radio.networkType = CSM_RADIO_NETWORK_TYPE_LTE;
    OSAL_strcpy(event.evt.radio.address, addr_ptr);
    OSAL_strncpy((char*)&event.evt.radio.infcName, "rnmet0", 128);

    /*
     * Send new IP info.
     */
    if (UT_PASS != CSM_UT_writeCsmEvent(&event)) {
        OSAL_logMsg("%s %d: Protocol %s notified of new IP\n",
                __FILE__, __LINE__, CSM_INPUT_EVENT_QUEUE_NAME);
    }
}

/*
 * ======== CSM_UT_writeCsmEvent() ========
 *
 * Function to write CSM_InputEvent to CSM.
 *
 * Returns:
 *    UT_PASS: Event wrote successfully.
 *    UT_FAIL: Event wrote failed.
 */
UT_Return CSM_UT_writeCsmEvent(
    const CSM_InputEvent *csmEvt_ptr)
{
    if (OSAL_SUCCESS != OSAL_msgQSend(global_ptr->csmInputEvtQ,
            (void *)csmEvt_ptr, sizeof(CSM_InputEvent), OSAL_NO_WAIT, NULL)) {
        OSAL_logMsg("%s: Failed to write command", __FUNCTION__);
        return (UT_FAIL);
    }
    return (UT_PASS);
}

/*
 * ======== CSM_UT_setIp()  ========
 *
 * The CLI for user to input a new IP address of nornal
 * registration and set to CSM.
 *
 */
UT_Return CSM_UT_setIp(
    vint arg)
{
    char buf[20];

    OSAL_logMsg("IP address: ");
    _CSM_UT_getLine(buf, 20);
    OSAL_logMsg("Get input IP Adrress : %s\n", buf);
    CSMUT_ipEventSend(buf, 0);

    return UT_PASS;
}

/*
 * ======== CSM_UT_init() ========
 *
 * The initial function to init CSM.
 *
 */
UT_Return CSM_UT_init(
    vint arg)
{
    /* Init CSM UT Global obj */
    global_ptr = OSAL_memCalloc(1, sizeof(CSM_UT_GlobalObj), 0);
    OSAL_memSet(global_ptr, 0, sizeof(CSM_UT_GlobalObj));

    /* Create CSM output/input event message queue. */
    if (0 == (global_ptr->csmOutputEvtQ = OSAL_msgQCreate(
            CSM_OUTPUT_EVENT_QUEUE_NAME,
            OSAL_MODULE_CSM_PUBLIC, OSAL_MODULE_CSM_UT_CLI,
            OSAL_DATA_STRUCT_CSM_OutputEvent,
            CSM_OUTPUT_EVENT_MSGQ_LEN,
            sizeof(CSM_OutputEvent), 0))) {
        OSAL_logMsg("%s:%d ERROR createing CSM_Response Q\n",
                __FUNCTION__, __LINE__);
        return (UT_FAIL);
    }
    if (0 == (global_ptr->csmInputEvtQ = OSAL_msgQCreate(
            CSM_INPUT_EVENT_QUEUE_NAME,
            OSAL_MODULE_CSM_UT_CLI, OSAL_MODULE_CSM_PUBLIC,
            OSAL_DATA_STRUCT_CSM_InputEvent,
            CSM_INPUT_EVENT_MSGQ_LEN,
            sizeof(CSM_InputEvent), 0))) {
        OSAL_logMsg("%s:%d ERROR createing CSM_Input Q\n",
                __FUNCTION__, __LINE__);
        return (UT_FAIL);
    }

    /* Init CSMUT_event task to receive CSM output event. */
    global_ptr->csmEvtTaskId = OSAL_taskCreate(
            "csmUtEvtTask",
#if defined(OSAL_PTHREADS)
            0,         /* very low priority */
#elif defined(OSAL_WINCE)
            251,
#elif defined(OSAL_VXWORKS) || defined(OSAL_NUCLEUS)
            254,
#else
#error PORT
#endif
            OSAL_STACK_SZ_LARGE,
            (void *)CSMUT_eventTaskBlock,
            (void *)&global_ptr->csmOutputEvtQ);
    if (0 == global_ptr->csmEvtTaskId) {
        OSAL_logMsg("%s:%d Task create fail.\n", __FUNCTION__, __LINE__);
        return (UT_FAIL);
    }

    return (UT_PASS);
}

/*
 * ======== CSM_UT_quit() ========
 *
 * quit csm cli ut and shutdown CSM.
 *
 */
UT_Return CSM_UT_quit(
    vint arg)
{
    CSM_UT_run = 0;

    if (NULL != global_ptr) {
        /* Delete get output event task */
        OSAL_taskDelete(global_ptr->csmEvtTaskId);
        /* Delete message queue */
        OSAL_msgQDelete(global_ptr->csmOutputEvtQ);
        OSAL_msgQDelete(global_ptr->csmInputEvtQ);
    }

    return (UT_PASS);
}

/*
 * ======== CSM_UT_memory() ========
 *
 * This show memory usage of VxWorks.
 *
 */
#ifdef OSAL_VXWORKS
UT_Return CSM_UT_memory(
    vint arg)
{
    memShow(0);
    return (UT_PASS);
}
#endif

/*
 * The CLI first level menu table.
 */
CSM_UT_TestTableItem CSM_UT_testTable[] =
{
    { "init",       CSM_UT_init,        "CSM init for all test" },
    { "service",    CSM_UT_service,     "API test for AT command in service relevant function" },
    { "setip",      CSM_UT_setIp,       "Setup an IP address for normal registration" },
    { "call",       CSM_UT_call,        "API test for AT command in call relevant function" },
    { "sms",        CSM_UT_sms,         "API test for AT command in SMS relevant function" },
    { "ussd",       CSM_UT_ussd,        "API test for AT command in USSD relevant function" },
#ifdef OSAL_VXWORKS
    { "memory",     CSM_UT_memory,      "Show UT memory usage in vxWorks" },
#endif
    { "supsrv",     CSM_UT_supsrv,      "API test for supsrv features" },
    { "emergency",  CSM_UT_emergency,   "API test for AT command in emergency relevant function" },
    { "q",          CSM_UT_quit,        "Quit (exit csm_ut)"}
};

/*
 * ======== CSM_UT_menu() ========
 *
 * The CLI first that provide init, setip and call/sms test entry.
 *
 */
OSAL_TaskReturn CSM_UT_menu(
    void)
{
    vint              item;
    vint              itemMax;
    CSM_UT_TestTableItem *item_ptr;
    CSM_UT_TestFunc      *func_ptr;
    int32             arg;
    vint              printMenu;
    char              buf[10];

    printMenu = 1;
    CSM_UT_run = 1;

    OSAL_taskDelay(500);    /* cleans up printing */
    itemMax = sizeof(CSM_UT_testTable)/sizeof(CSM_UT_testTable[0]);
    while (CSM_UT_run) {
        if (printMenu > 0) {
            printMenu = 0;
            OSAL_logMsg("\n\n"
                    "================================================\n"
                    "D2 Tech CSM Unit Test\n"
                    "================================================\n"
                    "Command        Description\n"
                    "-------        ---------------------------------------\n");

            for (item = 0; item < itemMax; item++) {
                item_ptr = &CSM_UT_testTable[item];
                OSAL_logMsg("%-15s%s\n", item_ptr->cmd, item_ptr->desc);
            }
        }

        OSAL_logMsg("\n\n== CSM_UT == Cmd: ");

#ifndef UT_FIXED_CMD
        _CSM_UT_getLine(buf, 2 * sizeof(CSM_UT_testTable[0].cmd));
#else
        OSAL_taskDelay(2000);
        OSAL_strcpy(buf, UT_FIXED_CMD);
#endif

        for (item = 0, func_ptr = NULL; item < itemMax; item++) {
            item_ptr = &CSM_UT_testTable[item];
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
            /*
             * In the future the arg will be passed from the user's cmd line
             * after parsing the cmd line for white space, etc.
             */
            arg = 0;
            func_ptr(arg);
            printMenu = 0;
        }
        else {
            /* Unknown cmd, print help */
            printMenu = 1;
        }
    }
    OSAL_logMsg("\n%s:%d  UT_menu return\n\n", __FILE__, __LINE__);

    return (UT_PASS);
}

