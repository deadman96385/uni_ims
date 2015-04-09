/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2006 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 26767 $ $Date: 2014-06-05 19:26:37 +0800 (Thu, 05 Jun 2014) $
 */

#include <osal.h>
#include <isi.h>
#include <isip.h>
#include <csm_event.h>

#include "_tapp.h"
#include "_tapp_at_infc.h"

/*
 * ======== TAPP_atInfcInit() ========
 *
 * This function is used to initialize and configure AT interface.
 *
 * Return Values:
 * TAPP_PASS: open AT interface successful
 * TAPP_FAIL: fail to open AT interface
 */
TAPP_Return TAPP_atInfcInit(
    TAPP_GlobalObj *global_ptr)
{
    /* Open device */
    if (OSAL_FAIL == OSAL_fileOpen(&global_ptr->fd.atInfc, 
            global_ptr->gsmDevName, OSAL_FILE_O_RDWR | OSAL_FILE_O_NDELAY, 0)) {
        OSAL_logMsg("%s %d: Could not open AT interfac device.\n", 
                __FUNCTION__, __LINE__);
        return (TAPP_FAIL);
    }

    return (TAPP_PASS);
}

/*
 * ======== TAPP_atInfcShutdown() ========
 *
 * This function is used to close AT interface device.
 *
 * Return Values:
 * nothing.
 */
void TAPP_atInfcShutdown(
    TAPP_GlobalObj *global_ptr)
{
    /* close gsm device */
    if (global_ptr->fd.atInfc >= 0) {
        OSAL_fileClose(&global_ptr->fd.atInfc);
    }
}

/*
 * ======== TAPP_atInfcIssueAt() ========
 *
 * This function is used to write AT command to AT interface device node.
 *
 * Return Values:
 * TAPP_PASS: success to write AT command to device node.
 * TAPP_FAIL: faile to write AT command to device node. 
 */
TAPP_Return TAPP_atInfcIssueAt(
    TAPP_GlobalObj *global_ptr,
    char           *at_ptr)
{
    char  buff[TAPP_AT_COMMAND_STRING_SZ];
    vint  sendSize;

    if (NULL == global_ptr) {
        return (TAPP_FAIL);
    }

    if (NULL == at_ptr) {
        return (TAPP_FAIL);
    }

    /* write AT command to AT interface device node. */
    sendSize = OSAL_snprintf(buff, TAPP_AT_COMMAND_STRING_SZ, "%s", at_ptr);
    if (OSAL_FAIL == OSAL_fileWrite(&global_ptr->fd.atInfc, buff, &sendSize)) {
        return (TAPP_FAIL);
    }

    return (TAPP_PASS);
}
/*
 * ======== _TAPP_removeScts() ========
 *
 * To remove time stamp from AT event. After that, we can compare the content of SMS.
 *
 * Return Values:
 * TAPP_PASS: success
 * TAPP_FAIL: faile to get time stamp
 */

static TAPP_Return _TAPP_removeScts(
    char* at) 
{
    char       *scts_ptr;
    char       *sctsEnd_ptr;

    if (NULL == (scts_ptr = OSAL_strchr(at, ','))) {
        return (TAPP_FAIL);
    }
    
    if (NULL == (scts_ptr = OSAL_strchr(scts_ptr + 1, ','))) {
        return (TAPP_FAIL);
    }
    if (scts_ptr[1] != '\"') {
        return (TAPP_FAIL);
    }
    scts_ptr += 2;
    if (NULL == (sctsEnd_ptr = OSAL_strchr(scts_ptr, ','))) {
        return (TAPP_FAIL);
    }
    if (NULL == (sctsEnd_ptr = OSAL_strchr(sctsEnd_ptr + 1, ','))) {
        return (TAPP_FAIL);
    }
    sctsEnd_ptr--;
    if (*sctsEnd_ptr != '\"') {
        return (TAPP_FAIL);
    }
    OSAL_memMove(scts_ptr,  sctsEnd_ptr, OSAL_strlen(sctsEnd_ptr) + 1);
    return (TAPP_PASS);
}
/*
 * ======== TAPP_atInfcValidateAt() ========
 *
 * This function is used to validate AT
 *
 * Return Values:
 * TAPP_PASS: the result is same the expected value.
 * TAPP_FAIL: the result is different with the expected value.
 */
TAPP_Return TAPP_atInfcValidateAt(
    TAPP_GlobalObj *global_ptr,
    TAPP_Action    *action_ptr)
{
    TAPP_Event *event_ptr;

    if ((NULL == global_ptr) || (NULL == action_ptr)) {
        return (TAPP_FAIL);
    }

    event_ptr = &global_ptr->tappEvt;

    /* get input event */
    if (TAPP_PASS != TAPP_getInputEvent(global_ptr, action_ptr, event_ptr, 
            action_ptr->u.timeout)) {
        return (TAPP_FAIL);
    }

    if (TAPP_EVENT_TYPE_AT_INFC == event_ptr->type) {
        if (NULL != OSAL_strscan(event_ptr->msg.at, "+CMT:")) {
            //Remove time stamp if present
            _TAPP_removeScts(event_ptr->msg.at);
            _TAPP_removeScts(action_ptr->msg.at);
        }
        if (0 == OSAL_strcasecmp(event_ptr->msg.at, action_ptr->msg.at)) {
            return (TAPP_PASS);
        }
    }
    return (TAPP_FAIL);
}

