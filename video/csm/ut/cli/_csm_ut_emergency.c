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

vint CSM_UT_emergencyRun = 1;


/*
 * ======== _CSM_UT_emergency_setIp() ========
 *
 * The CLI for user to input a new IP address of emergency
 * registration and set to CSM.
 *
 */
UT_Return _CSM_UT_emergency_setIp(
    vint arg)
{
    char buf[20];

    OSAL_logMsg("IP address: ");
    _CSM_UT_getLine(buf, 20);
    OSAL_logMsg("Get input IP Adrress : %s\n", buf);
    CSMUT_ipEventSend(buf, 1);

    return (UT_PASS);
}

/*
 * ======== _CSM_UT_emergency_regConfig()  ========
 *
 * The CLI to setup emergency registraion configuration.
 *
 */
UT_Return _CSM_UT_emergency_regConfig(
    vint arg)
{
    CSM_InputEvent event;
    char buf[2];

    OSAL_logMsg("isEmergencyFailoverToCs: ");
    _CSM_UT_getLine(buf, 2);
    event.evt.radio.isEmergencyFailoverToCs = OSAL_atoi(buf);

    OSAL_logMsg("isEmergencyRegRequired: ");
    _CSM_UT_getLine(buf, 2);
    event.evt.radio.isEmergencyRegRequired = OSAL_atoi(buf);

    /* Construct CSM Event */
    event.type = CSM_EVENT_TYPE_RADIO;
    event.evt.radio.reason = CSM_RADIO_REASON_RADIO_CHANGE_EMERGENCY;

    /* Send to CSM */
    CSM_UT_writeCsmEvent(&event);

    return (UT_PASS);
}

/*
 * ======== _CSM_UT_emergency_call ========
 *
 * Generate a emergency call event, get the dial number,
 * and write to CSM
 */
UT_Return _CSM_UT_emergency_call(
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
    callEvt_ptr->isEmergency = 1;
    callEvt_ptr->extraArgument = 0;
    callEvt_ptr->u.remoteAddress[0] = 0;
    callEvt_ptr->u.digit = 'x';

    OSAL_logMsg("\nCall to : ");
    _CSM_UT_getLine(buf, 40);
    OSAL_snprintf(callEvt_ptr->u.remoteAddress, CSM_EVENT_STRING_SZ, "%s", buf);

    CSM_UT_writeCsmEvent(&csmEvt_ptr);

    return UT_PASS;
}

/*
 * ======== _CSM_UT_emergency_quit ========
 *
 * Quit the emergency menu and return to main menu
 */
static UT_Return _CSM_UT_emergency_quit(
    int arg0)
{
    CSM_UT_emergencyRun = 0;
    return UT_PASS;
}

CSM_UT_TestTableItem CSM_UT_emergencyTable[] =
{
    { "eip",          _CSM_UT_emergency_setIp,        "Setup an IP address for emergency registration" },
    { "eregconf",  _CSM_UT_emergency_regConfig, "Setup emergency registration configuration" },
    { "eatd",        _CSM_UT_emergency_call,           "Dial an emergency call"},
    { "q",             _CSM_UT_emergency_quit,          "Quit (exit emergency menu"}
};

/*
 * ======== CSM_UT_emergency ========
 *
 * The main menu for the emergency opertion
 */
UT_Return CSM_UT_emergency (
)
{
    CSM_UT_emergencyRun = 1;
    vint              item;
    vint              itemMax;
    CSM_UT_TestTableItem *item_ptr;
    CSM_UT_TestFunc      *func_ptr;
    int32             arg;
    char              buf[10];
    vint              printMenu;

    printMenu = 1;
    OSAL_taskDelay(500);    /* cleans up printing */
    itemMax = sizeof(CSM_UT_emergencyTable)/sizeof(CSM_UT_emergencyTable[0]);
    while (CSM_UT_emergencyRun) {
        if (printMenu > 0) {
            printMenu = 0;
            OSAL_logMsg("\n"
                    "===========================\n"
                    "D2 CSM emergency test suite\n"
                    "================================================\n"
                    "Command        Description\n"
                    "-------        ---------------------------------------\n");

            for (item = 0; item < itemMax; item++) {
                item_ptr = &CSM_UT_emergencyTable[item];
                OSAL_logMsg("%-15s%s\n", item_ptr->cmd, item_ptr->desc);
            }
        }

        OSAL_logMsg("\nCmd: ");
        _CSM_UT_getLine(buf, 2 * sizeof(CSM_UT_emergencyTable[0].cmd));
        for (item = 0, func_ptr = NULL; item < itemMax; item++) {
            item_ptr = &CSM_UT_emergencyTable[item];
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

