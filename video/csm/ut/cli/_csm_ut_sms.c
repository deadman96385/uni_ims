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
#include <pdu_hlpr.h>
#include <pdu_3gpp.h>
#include "_csm_ut.h"
#include "_csm_ut_utils.h"

/*
 * Flag for running SMS menu
 */
vint CSM_UT_smsRun = 1;
/*
 * ======== CSM_UT_sms_send ========
 *
 * Send out a SMS
 *
 * Returns: 
 *      UT_PASS: Success
 */
UT_Return CSM_UT_sms_send(
    vint arg)
{
    CSM_InputSms       *smsEvt_ptr;
    CSM_InputEvent      csmEvt_ptr;
    char                buf[CSM_SMS_STRING_SZ+1];
    int                 pos;

    smsEvt_ptr = &csmEvt_ptr.evt.sms;
    csmEvt_ptr.type = CSM_EVENT_TYPE_SMS;
    smsEvt_ptr->reason = CSM_SMS_REASON_AT_CMD_SEND_MESSAGE;
    OSAL_logMsg("\nType - 0. 3GPP(Default) 1. 3GPP2: ");
    _CSM_UT_getLine(buf, CSM_SMS_STRING_SZ);
    if (0 == OSAL_atoi(buf)) {
        smsEvt_ptr->type = CSM_SMS_TYPE_PDU_3GPP;
    }
    else {
        smsEvt_ptr->type = CSM_SMS_TYPE_PDU_3GPP2;
    }
    OSAL_strncpy(smsEvt_ptr->reasonDesc, "CSM_EVENT_SMS_REASON_CMD_SEND",
            CSM_EVENT_STRING_SZ);
    smsEvt_ptr->remoteAddress[0] = 0;

    OSAL_logMsg("\nSend to : ");
    _CSM_UT_getLine(buf, CSM_SMS_STRING_SZ);
    OSAL_snprintf(smsEvt_ptr->remoteAddress, CSM_EVENT_STRING_SZ, "%s", buf);
    OSAL_logMsg("\nMessage : ");
    _CSM_UT_getLine(buf, CSM_SMS_STRING_SZ);
    OSAL_snprintf(smsEvt_ptr->message, CSM_EVENT_STRING_SZ, "%s", buf);

    if (CSM_SMS_TYPE_PDU_3GPP == smsEvt_ptr->type) {
        PDU_3gppEncodeSubmitTPDU(0, smsEvt_ptr->remoteAddress,
                smsEvt_ptr->message, smsEvt_ptr->pdu, CSM_SMS_STRING_SZ);

        pos = OSAL_strlen(smsEvt_ptr->pdu);
        /* Add ctrl+z and 0 end of pdu string */
        smsEvt_ptr->pdu[pos] = 26;
        smsEvt_ptr->pdu[pos + 1] = 0;
    }
    smsEvt_ptr->msgLen = OSAL_strlen(smsEvt_ptr->pdu);

    CSM_UT_writeCsmEvent(&csmEvt_ptr);

    return UT_PASS;
}

/*
 * ======== CSM_UT_sms_smsc ========
 *
 * Set SMSC address
 *
 * Returns: 
 *      UT_PASS: Success
 */
UT_Return CSM_UT_sms_smsc(
    vint arg)
{
    CSM_InputSms       *smsEvt_ptr;
    CSM_InputEvent      csmEvt_ptr;
    char                buf[CSM_SMS_STRING_SZ+1];

    smsEvt_ptr = &csmEvt_ptr.evt.sms;
    csmEvt_ptr.type = CSM_EVENT_TYPE_SMS;
    smsEvt_ptr->reason = CSM_SMS_REASON_SET_SMSC;
    smsEvt_ptr->type = CSM_SMS_TYPE_PDU_3GPP;
    OSAL_strncpy(smsEvt_ptr->reasonDesc, "CSM_EVENT_SMS_REASON_CMD_SEND",
            CSM_EVENT_STRING_SZ);
    smsEvt_ptr->remoteAddress[0] = 0;

    OSAL_logMsg("\nSMSC Address : ");
    _CSM_UT_getLine(buf, CSM_SMS_STRING_SZ);
    OSAL_snprintf(smsEvt_ptr->smsc, CSM_EVENT_STRING_SZ, "%s", buf);
    CSM_UT_writeCsmEvent(&csmEvt_ptr);
    return UT_PASS;
}

/*
 * ======== CSM_UT_sms_quit ========
 *
 * Exit SMS menu
 *
 * Returns: 
 *      UT_PASS: Success
 */
UT_Return CSM_UT_sms_quit(
    vint arg)
{
    CSM_UT_smsRun = 0;
    return UT_PASS;
}

/*
 * Menu options
 */
CSM_UT_TestTableItem CSM_UT_smsTable[] =
{
    { "send",      CSM_UT_sms_send,        "Send sms" },
    { "smsc",      CSM_UT_sms_smsc,        "Set smsc" },
    { "q",         CSM_UT_sms_quit,        "Quit (exit sms menu"}
};

/*
 * ======== CSM_UT_sms ========
 *
 * Main menu for SMS
 *
 * Returns: 
 *      UT_PASS: Success
 */
UT_Return CSM_UT_sms(
    void)
{
    vint                    item;
    vint                    itemMax;
    CSM_UT_TestTableItem    *item_ptr;
    CSM_UT_TestFunc         *func_ptr;
    int32                   arg;
    char                    buf[10];
    vint                    printMenu;

    CSM_UT_smsRun = 1;
    printMenu = 1;
    OSAL_taskDelay(500);    /* cleans up printing */
    itemMax = sizeof(CSM_UT_smsTable)/sizeof(CSM_UT_smsTable[0]);
    while (CSM_UT_smsRun) {
        if (printMenu > 0) {
            printMenu = 0;
            OSAL_logMsg("\n"
                    "=====================\n"
                    "D2 CSM ussd test suite\n"
                    "=====================\n"
                    "Command  Description\n"
                    "-------  -----------\n");

            for (item = 0; item < itemMax; item++) {
                item_ptr = &CSM_UT_smsTable[item];
                OSAL_logMsg("%-9s%s\n", item_ptr->cmd, item_ptr->desc);
            }
        }

        OSAL_logMsg("\nCmd: ");
        _CSM_UT_getLine(buf, 2 * sizeof(CSM_UT_smsTable[0].cmd));
        for (item = 0, func_ptr = NULL; item < itemMax; item++) {
            item_ptr = &CSM_UT_smsTable[item];
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

