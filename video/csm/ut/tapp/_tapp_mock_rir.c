/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2006 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 28044 $ $Date: 2014-08-11 15:49:43 +0800 (Mon, 11 Aug 2014) $
 */

#include <osal.h>
#include <isi.h>
#include <isip.h>
#include <csm_event.h>
#include "_tapp.h"
#include <ezxml.h>
#include "_tapp_mock_rir.h"
extern TAPP_GlobalObj *global_ptr;

 /* TAPP mock RIR */

/*
 * ======== TAPP_mockRirInit() ========
 *
 * This function is used to create message Q that 
 * can write  input evnet to CSM.
 * 
 * Return Values:
 * TAPP_PASS: initialize successfully.
 * TAPP_FAIL: initialize fialed.
 */
vint TAPP_mockRirInit(
    TAPP_GlobalObj *global_ptr)
{    
  /*
     * Create Queue to send new IP info to CSM.
     */
    if (0 == (global_ptr->queue.csmEvt = OSAL_msgQCreate(
            CSM_INPUT_EVENT_QUEUE_NAME,
            OSAL_MODULE_TAPP, OSAL_MODULE_CSM_PUBLIC,
            OSAL_DATA_STRUCT_CSM_InputEvent,
            CSM_INPUT_EVENT_MSGQ_LEN, sizeof(CSM_InputEvent), 0))) {
        OSAL_logMsg("%s:%d Message queue failure.\n", __FUNCTION__, __LINE__);
        return (TAPP_FAIL);        
    }
    return (TAPP_PASS);
}

/*
 * ======== TAPP_mockRirShutdown() ========
 *
 * This function is used to shutdown Mock RIR.
 * 
 * Return Values:
 *   TAPP_PASS: shutdown sucessfully.
 *   TAPP_FAIL: shutdown failed
 */
vint TAPP_mockRirShutdown(
    TAPP_GlobalObj *global_ptr)
{    
    /* Delete queues. */
    if (OSAL_SUCCESS != OSAL_msgQDelete(global_ptr->queue.csmEvt)) {
        return (TAPP_FAIL);
    }
    return (TAPP_PASS);
}
 
/*
 * ======== TAPP_mockRirIssueCsm() ========
 *
 * This function is used to issue an RIR response.
 * 
 * Return Values:
 * TAPP_PASS: write event to CSM sucessfully.
 * TAPP_FAIL: write event to CSM fail.
 */
vint TAPP_mockRirIssueCsm(
    TAPP_GlobalObj *global_ptr,
    TAPP_Action    *action_ptr)
{
    CSM_InputEvent  *csmEvent_ptr;
    
    if (TAPP_ACTION_TYPE_ISSUE_CSM != action_ptr->type) {
        return (TAPP_FAIL);
    }
    csmEvent_ptr = &action_ptr->msg.csm;
    /*
     * Send new IP info.
     */
    if (OSAL_SUCCESS != OSAL_msgQSend(global_ptr->queue.csmEvt,
            (void *)csmEvent_ptr, sizeof(CSM_InputEvent), OSAL_NO_WAIT,
            NULL)){
        OSAL_logMsg("%s:%d Sent message Q failure.\n", __FUNCTION__, __LINE__);
        return (TAPP_FAIL);
    }
    return (TAPP_PASS);

/*
 * ======== RIR_init() ========
 *
 * This function is Mock RIR initial..
 * This function will call the mock rir initial function
 * 
 * Return Values:
 *   0: initialize successfully.
 *   -1: initialize fialed.
 */  
}
int RIR_init(
    char *xmlDoc_ptr,
    int   xmlDocLen)
{
    if (TAPP_PASS != TAPP_mockRirInit(global_ptr)) {
        return(-1);
    }
    return (0);
}

/*
 * ======== RIR_shutdown() ========
 *
 * This function is to shutdown Mock SAPP .
 * This function will call the TAPP_mockSappShutdown()  function
 * 
 * Return Values:
 *   0: initialize successfully.
 *   -1: initialize fialed.
 */
int RIR_shutdown()
{
    if (TAPP_PASS != TAPP_mockRirShutdown(global_ptr)) {
        return(-1);
    }
    return (0);
}

