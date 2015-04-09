/*
 * THIS IS AN UNPUBLISHED WORK CONVETINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIEVETRY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004-2010 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 18777 $ $Date: 2012-11-09 01:47:11 -0800 (Fri, 09 Nov 2012) $
 *
 */

#include <osal.h>
#include <csm_event.h>
#include <pdu_hlpr.h>
#include "_csm_ut.h"
#include "_csm_ut_utils.h"

/*
 * Flag for running USSD menu
 */
vint CSM_UT_ussdRun = 1;

/*
 * ======== CSM_UT_ussdQuit ========
 *
 * Exit USSD menu
 *
 * Returns: 
 *      UT_PASS: Success
 */
UT_Return CSM_UT_ussdQuit(
    int arg0)
{
    CSM_UT_ussdRun = 0;
    return UT_PASS;
}

/*
 * ======== CSM_UT_ussd_end ========
 *
 * End the USSD
 *
 * Returns: 
 *      UT_PASS: Success
 */
UT_Return CSM_UT_ussd_end (
    int arg0)
{
    CSM_InputUssd *ussdEvt_ptr;
    CSM_InputEvent     csmEvt_ptr;

    ussdEvt_ptr = &csmEvt_ptr.evt.ussd;
    csmEvt_ptr.type = CSM_EVENT_TYPE_USSD;
    ussdEvt_ptr->reason = CSM_USSD_REASON_AT_CMD_DISCONNECT_USSD;
    ussdEvt_ptr->encType = CSM_USSD_ENCTYPE_ASCII;
    OSAL_strncpy(ussdEvt_ptr->reasonDesc, "CSM_USSD_REASON_AT_CMD_SEND_USSD",
            CSM_EVENT_STRING_SZ);

    ussdEvt_ptr->message[0] = '\0';
    CSM_UT_writeCsmEvent(&csmEvt_ptr);
    
    return UT_PASS;
}

/*
 * ======== CSM_UT_ussd_send ========
 *
 * Send a USSD
 *
 * Returns: 
 *      UT_PASS: Success
 */
UT_Return CSM_UT_ussd_send (
    int arg0)
{
    CSM_InputUssd *ussdEvt_ptr;
    CSM_InputEvent     csmEvt_ptr;
    char          buf[1024];
    int bytes;

    ussdEvt_ptr = &csmEvt_ptr.evt.ussd;
    csmEvt_ptr.type = CSM_EVENT_TYPE_USSD;
    ussdEvt_ptr->reason = CSM_USSD_REASON_AT_CMD_SEND_USSD;
    ussdEvt_ptr->encType = CSM_USSD_ENCTYPE_ASCII;
    OSAL_strncpy(ussdEvt_ptr->reasonDesc, "CSM_USSD_REASON_AT_CMD_SEND_USSD",
            CSM_EVENT_STRING_SZ);

    OSAL_logMsg("\nSend: ");
    _CSM_UT_getLine(buf, 1024);
    bytes = PDU_pduBytesToHexString((unsigned char *)buf, 
                OSAL_strlen(buf), ussdEvt_ptr->message);
    ussdEvt_ptr->message[bytes] = '\0';
    CSM_UT_writeCsmEvent(&csmEvt_ptr);

    return UT_PASS;
}

/*
 * Menu options
 */
CSM_UT_TestTableItem CSM_UT_ussdTable[] =
{
    { "send", CSM_UT_ussd_send, "send ussd" },
    { "end",  CSM_UT_ussd_end,  "end ussd" },
    { "q",    CSM_UT_ussdQuit,  "Quit (exit call menu)"}
};

/*
 * ======== CSM_UT_ussd ========
 *
 * Main menu for USSD
 *
 * Returns: 
 *      UT_PASS: Success
 */
UT_Return CSM_UT_ussd (
    void)
{
    CSM_UT_ussdRun = 1;
    vint              item;
    vint              itemMax;
    CSM_UT_TestTableItem *item_ptr;
    CSM_UT_TestFunc      *func_ptr;
    int32             arg;
    char              buf[10];
    vint              printMenu;

    printMenu = 1;
    OSAL_taskDelay(500);    /* cleans up printing */
    itemMax = sizeof(CSM_UT_ussdTable)/sizeof(CSM_UT_ussdTable[0]);
    while (CSM_UT_ussdRun) {
        if (printMenu > 0) {
            printMenu = 0;
            OSAL_logMsg("\n"
                    "=====================\n"
                    "D2 CSM ussd test suite\n"
                    "=====================\n"
                    "Command  Description\n"
                    "-------  -----------\n");

            for (item = 0; item < itemMax; item++) {
                item_ptr = &CSM_UT_ussdTable[item];
                OSAL_logMsg("%-9s%s\n", item_ptr->cmd, item_ptr->desc);
            }
        }

        OSAL_logMsg("\nCmd: ");
        _CSM_UT_getLine(buf, 2 * sizeof(CSM_UT_ussdTable[0].cmd));
        for (item = 0, func_ptr = NULL; item < itemMax; item++) {
            item_ptr = &CSM_UT_ussdTable[item];
            if (0 == OSAL_strncmp(buf, (char *)item_ptr->cmd, sizeof(buf))) {
                func_ptr = item_ptr->func_ptr;
                break;
            }
        }

        if (NULL != func_ptr) {
            OSAL_logMsg("\n\n\n"
                    "==============================================\n"
                    "%s\n"
                    "==============================================\n\n\n",
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
    OSAL_logMsg("\n%s:%d  UT_menu return\n\n", __FILE__, __LINE__);

    return UT_PASS;
}

