/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2007 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 29683 $ $Date: 2014-11-04 14:18:29 +0800 (Tue, 04 Nov 2014) $
 */

#include <osal.h>

#include "../../../../csm/csm_event.h"

#include "proxy.h"
#include "_cmd_mngr.h"
#include "_proxy_ut.h"

/*
 * ======== PRXY_printCsmInputEvent ========
 *
 * This public method is used for printing a CSM Input Event
 *
 * RETURN:
 *     none
 */
void PRXY_printCsmInputEvent(
    const CSM_InputEvent* csmEvt_ptr)
{
    const CSM_InputCall* call_ptr = &csmEvt_ptr->evt.call;
    OSAL_logMsg("reason:%d", call_ptr->reason);
    OSAL_logMsg("desc:%s", call_ptr->reasonDesc);
    OSAL_logMsg("remoteAddress:%s", call_ptr->u.remoteAddress);
    OSAL_logMsg("privacy:%d", call_ptr->cidType);
    OSAL_logMsg("digit:%c", call_ptr->u.digit);
}

/*
 * ======== PRXY_printAT ========
 *
 * This public method is used for printing AT command buffers
 *
 * RETURN:
 *     none
 */
void PRXY_printAT(
    const char *prefix,
    const char *at_ptr)
{
    int  i;
    int  j;
    int  at_len;
    char buffer[PRXY_AT_COMMAND_SIZE + 1];
    const char * hexTable_ptr = "0123456789ABCDEF";
    at_len = OSAL_strlen(at_ptr);
    for (i = 0, j = 0; i < at_len && j < PRXY_AT_COMMAND_SIZE; i++) {
        if (at_ptr[i] >= 0x20 && at_ptr[i] <= 0x7E) {
            buffer[j++] = at_ptr[i];
        }
        else {
            buffer[j++] = '\\';
            buffer[j++] = hexTable_ptr[(at_ptr[i] >> 4) & 0xF];
            buffer[j++] = hexTable_ptr[at_ptr[i] & 0xF];
        }
    }
    buffer[j] = '\0';
    if (j == PRXY_AT_COMMAND_SIZE) {
        OSAL_logMsg("Warning: AT String is too long. Trancate it. len=%d \n",
                at_len);
    }
    OSAL_logMsg("%s: '%s'\n", prefix, buffer);
}

/*
 * ======== PXRY_testAtParse ========
 *
 * This public method is use to test AT command parsing routines
 *
 * RETURN:
 *     none
 */
void PXRY_testAtParse()
{
    CSM_InputEvent   csmEvt;
    PRXY_CommandMngr cmdMngr;

    if (PRXY_RETURN_FAILED == _PRXY_cmdMngrParseAtCommand(&cmdMngr,
            "AT+CHLD=0", 
            &csmEvt)) {
        OSAL_logMsg("AT+CHLD=0 FAILED");
    }
    else {
        PRXY_printCsmInputEvent(&csmEvt);
    }

    if (PRXY_RETURN_FAILED == _PRXY_cmdMngrParseAtCommand(&cmdMngr,
            "AT+CHLD=1",
            &csmEvt)) {
        OSAL_logMsg("AT+CHLD=1 FAILED");
    }
    else {
        PRXY_printCsmInputEvent(&csmEvt);
    }

    if (PRXY_RETURN_FAILED == _PRXY_cmdMngrParseAtCommand(&cmdMngr,
            "AT+CHLD=10", 
            &csmEvt)) {
        OSAL_logMsg("AT+CHLD=10 FAILED");
    }
    else {
        PRXY_printCsmInputEvent(&csmEvt);
    }

    if (PRXY_RETURN_FAILED == _PRXY_cmdMngrParseAtCommand(&cmdMngr,
            "AT+CHLD=11", 
            &csmEvt)) {
        OSAL_logMsg("AT+CHLD=11 FAILED");
    }
    else {
        PRXY_printCsmInputEvent(&csmEvt);
    }

    if (PRXY_RETURN_FAILED == _PRXY_cmdMngrParseAtCommand(&cmdMngr,
            "AT+CHLD=12", 
            &csmEvt)) {
        OSAL_logMsg("AT+CHLD=12 FAILED");
    }
    else {
        PRXY_printCsmInputEvent(&csmEvt);
    }

    if (PRXY_RETURN_FAILED == _PRXY_cmdMngrParseAtCommand(&cmdMngr,
            "AT+CHLD=13",
            &csmEvt)) {
        OSAL_logMsg("AT+CHLD=13 FAILED");
    }
    else {
        PRXY_printCsmInputEvent(&csmEvt);
    }

    if (PRXY_RETURN_FAILED == _PRXY_cmdMngrParseAtCommand(&cmdMngr,
            "AT+CHLD=14", 
            &csmEvt)) {
        OSAL_logMsg("AT+CHLD=14 FAILED");
    }
    else {
        PRXY_printCsmInputEvent(&csmEvt);
    }

    if (PRXY_RETURN_FAILED == _PRXY_cmdMngrParseAtCommand(&cmdMngr,
            "AT+CHLD=15", 
            &csmEvt)) {
        OSAL_logMsg("AT+CHLD=15 FAILED");
    }
    else {
        PRXY_printCsmInputEvent(&csmEvt);
    }

    if (PRXY_RETURN_FAILED == _PRXY_cmdMngrParseAtCommand(&cmdMngr,
            "AT+CHLD=16",
            &csmEvt)) {
        OSAL_logMsg("AT+CHLD=16 FAILED");
    }
    else {
        PRXY_printCsmInputEvent(&csmEvt);
    }

    if (PRXY_RETURN_FAILED == _PRXY_cmdMngrParseAtCommand(&cmdMngr,
            "AT+CHLD=2", 
            &csmEvt)) {
        OSAL_logMsg("AT+CHLD=2 FAILED");
    }
    else {
        PRXY_printCsmInputEvent(&csmEvt);
    }

    if (PRXY_RETURN_FAILED == _PRXY_cmdMngrParseAtCommand(&cmdMngr,
            "AT+CHLD=21", 
            &csmEvt)) {
        OSAL_logMsg("AT+CHLD=21 FAILED");
    }
    else {
        PRXY_printCsmInputEvent(&csmEvt);
    }

    if (PRXY_RETURN_FAILED == _PRXY_cmdMngrParseAtCommand(&cmdMngr,
            "AT+CHLD=22", 
            &csmEvt)) {
        OSAL_logMsg("AT+CHLD=22 FAILED");
    }
    else {
        PRXY_printCsmInputEvent(&csmEvt);
    }

    if (PRXY_RETURN_FAILED == _PRXY_cmdMngrParseAtCommand(&cmdMngr,
            "AT+CHLD=23", 
            &csmEvt)) {
        OSAL_logMsg("AT+CHLD=23 FAILED");
    }
    else {
        PRXY_printCsmInputEvent(&csmEvt);
    }

    if (PRXY_RETURN_FAILED == _PRXY_cmdMngrParseAtCommand(&cmdMngr,
            "AT+CHLD=24", 
            &csmEvt)) {
        OSAL_logMsg("AT+CHLD=24 FAILED");
    }
    else {
        PRXY_printCsmInputEvent(&csmEvt);
    }

    if (PRXY_RETURN_FAILED == _PRXY_cmdMngrParseAtCommand(&cmdMngr,
            "AT+CHLD=25", 
            &csmEvt)) {
        OSAL_logMsg("AT+CHLD=25 FAILED");
    }
    else {
        PRXY_printCsmInputEvent(&csmEvt);
    }

    if (PRXY_RETURN_FAILED == _PRXY_cmdMngrParseAtCommand(&cmdMngr,
            "AT+CHLD=26", 
            &csmEvt)) {
        OSAL_logMsg("AT+CHLD=26 FAILED");
    }
    else {
        PRXY_printCsmInputEvent(&csmEvt);
    }

    if (PRXY_RETURN_FAILED == _PRXY_cmdMngrParseAtCommand(&cmdMngr,
            "AT+CHLD=2", &csmEvt)) {
        OSAL_logMsg("AT+CHLD=2 FAILED");
    }
    else {
        PRXY_printCsmInputEvent(&csmEvt);
    }

    if (PRXY_RETURN_FAILED == _PRXY_cmdMngrParseAtCommand(&cmdMngr,
            "ATD13122099351I;", &csmEvt)) {
        OSAL_logMsg("ATD13122099351I; FAILED");
    }
    else {
        PRXY_printCsmInputEvent(&csmEvt);
    }

    if (PRXY_RETURN_FAILED == _PRXY_cmdMngrParseAtCommand(&cmdMngr,
            "ATD131229351;", &csmEvt)) {
        OSAL_logMsg("ATD131229351; FAILED");
    }
    else {
        PRXY_printCsmInputEvent(&csmEvt);
    }

    if (PRXY_RETURN_FAILED == _PRXY_cmdMngrParseAtCommand(&cmdMngr,
            "ATD18055643424", &csmEvt)) {
        OSAL_logMsg("ATD18055643424 FAILED");
    }
    else {
        PRXY_printCsmInputEvent(&csmEvt);
    }

    if (PRXY_RETURN_FAILED == _PRXY_cmdMngrParseAtCommand(&cmdMngr,
            "ATH", &csmEvt)) {
        OSAL_logMsg("ATH FAILED");
    }
    else {
        PRXY_printCsmInputEvent(&csmEvt);
    }

    if (PRXY_RETURN_FAILED == _PRXY_cmdMngrParseAtCommand(&cmdMngr,
            "AT+CHUP", &csmEvt)) {
        OSAL_logMsg("AT+CHUP FAILED");
    }
    else {
        PRXY_printCsmInputEvent(&csmEvt);
    }

    if (PRXY_RETURN_FAILED == _PRXY_cmdMngrParseAtCommand(&cmdMngr,
            "AT+CLCC", &csmEvt)) {
        OSAL_logMsg("AT+CLCC FAILED");
    }
    else {
        PRXY_printCsmInputEvent(&csmEvt);
    }

    if (PRXY_RETURN_FAILED == _PRXY_cmdMngrParseAtCommand(&cmdMngr,
            "ATA", &csmEvt)) {
        OSAL_logMsg("ATA FAILED");
    }
    else {
        PRXY_printCsmInputEvent(&csmEvt);
    }

    if (PRXY_RETURN_FAILED == _PRXY_cmdMngrParseAtCommand(&cmdMngr,
            "AT+VTS=*", &csmEvt)) {
        OSAL_logMsg("AT+VTS=* FAILED");
    }
    else {
        PRXY_printCsmInputEvent(&csmEvt);
    }

    if (PRXY_RETURN_FAILED == _PRXY_cmdMngrParseAtCommand(&cmdMngr,
            "AT+CCFC", &csmEvt)) {
        OSAL_logMsg("AT+CCFC FAILED");
    }
    else {
        PRXY_printCsmInputEvent(&csmEvt);
    }
}


