/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004-2010 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev$ $Date$
 *
 */

#include <osal.h>
#include "_csm_event.h"
#include "_csm.h"
#include "_csm_response.h"

#include "gbam.h"
#include "gba.h"


#ifndef GBAM_DEBUG
#define GBAM_dbgPrintf(x, ...)
#else
#define GBAM_dbgPrintf OSAL_logMsg
#endif

/* Global object. */
static GBAM_GlobalObj *_GBAM_globalObj_ptr = NULL;


/*
 * ======== _GBAM_eventTask() ========
 *
 * This function is the entry point for a thread that handles events from GBA.
 * This thread waits (blocks) on OSAL_msgQRecv() processes events as they are
 * received.
 *
 * Returns:
 *   Nothing and never.
 */
OSAL_TaskReturn _GBAM_eventTask(
    OSAL_TaskArg taskArg)
{
    GBA_Event event;
    uint32          msTimeout;
    /* An area in .bss to construct CSM output event to send. */
    static CSM_OutputEvent csmOutputEvent;

    msTimeout = 1000;

    /* Loop forever to receive event from CSM and process it */
    while (1) {
        if (GBA_getEvent(&event, msTimeout)) {
            switch (event.type) {
                case GBA_EVENT_AKA_CHALLENGE:
                    GBAM_akaArbitorSetTarget(GBAM_AKA_GBA_REQ);
                    CSM_sendAkaChallenge(event.u.akaChallenge.rand_ptr,
                            event.u.akaChallenge.autn_ptr, &csmOutputEvent);
                    break;
                default:
                    GBAM_dbgPrintf("GBAM_eventTask not supported event:%d\n",
                            event.type);
                    break;
            }
        }
        else {
            OSAL_taskDelay(100);
        }

    }
    GBAM_dbgPrintf("GBAM_eventTask exit.\n");

    return(0);
}

/*
 * ======== GBAM_init() ========
 *
 * Public routine for initializing the GBAM application
 *
 * Returns:
 *      CSM_OK: function exits normally.
 *      CSM_ERR: in case of error
 */
vint GBAM_init(
     void)
{
    /* Allocate memory for global object */
    _GBAM_globalObj_ptr = OSAL_memCalloc(1, sizeof(GBAM_GlobalObj), 0);
    OSAL_memSet(_GBAM_globalObj_ptr, 0, sizeof(GBAM_GlobalObj));


    /* Create input event queue to send event to CSM */
    if (0 == (_GBAM_globalObj_ptr->csmEventInQ = OSAL_msgQCreate(
                CSM_INPUT_EVENT_QUEUE_NAME,
                OSAL_MODULE_GBAM, OSAL_MODULE_CSM_PUBLIC,
                OSAL_DATA_STRUCT_CSM_InputEvent,
                CSM_INPUT_EVENT_MSGQ_LEN, sizeof(CSM_InputEvent), 0))) {
        GBAM_dbgPrintf("%s:%d ERROR opening CSM Input Event Q\n",
                __FILE__, __LINE__);
        return (CSM_ERR);
    }

    /* Create GBAM thread */
    if (0 == (_GBAM_globalObj_ptr->tId = OSAL_taskCreate(
            GBAM_TASK_NAME,
            OSAL_TASK_PRIO_NRT, GBAM_TASK_SIZE,
            (void *)_GBAM_eventTask,
            (void *)0))) {
        GBAM_dbgPrintf("Error starting GBAM thread\n");
        return (CSM_ERR);
    }

    /* Init the GBA module */
    if (1 != GBA_init()) {

        CSM_dbgPrintf("%s:%d Failed to init GBA\n", __FUNCTION__, __LINE__);
        return (CSM_ERR);
    }

    if (NULL == (_GBAM_globalObj_ptr->akaLock = OSAL_semMutexCreate())){
        return (CSM_ERR);
    }
    else {
        return (CSM_OK);
    }
}

/*
 * ======== GBAM_shutdown() ========
 *
 * Public routine for shutting down the ISI manager package.
 *
 * Returns:
 *      CSM_OK: function exits normally.
 *      CSM_ERR: in case of error
 */
vint GBAM_shutdown(
    void)
{
    if (NULL == _GBAM_globalObj_ptr) {
        return (CSM_OK);
    }

    /* Delete CSM event task */
    OSAL_taskDelete(_GBAM_globalObj_ptr->tId);

    /* Delete CSM input event Q */
    OSAL_msgQDelete(_GBAM_globalObj_ptr->csmEventInQ);

    /* Free global object */
    OSAL_memFree(_GBAM_globalObj_ptr, 0);
    _GBAM_globalObj_ptr = NULL;

    GBA_shutdown();

    OSAL_semDelete(_GBAM_globalObj_ptr->akaLock);

    return (CSM_OK);
}

vint GBAM_sendAkaAuthResp(
    CSM_ServiceEvt *serviceEvt_ptr)
{
    GBA_Command *cmd_ptr = &_GBAM_globalObj_ptr->gbaCommand;

    switch (serviceEvt_ptr->reason) {
        case CSM_SERVICE_REASON_ISIM_AKA_RESPONSE_SUCCESS:
            cmd_ptr->type = GBA_COMMAND_AKA_RESPONSE_SUCCESS;
            break;
        case CSM_SERVICE_REASON_ISIM_AKA_RESPONSE_NETWORK_FAILURE:
            cmd_ptr->type = GBA_COMMAND_AKA_RESPONSE_NETWORK_FAILURE;
            break;
        case CSM_SERVICE_REASON_ISIM_AKA_RESPONSE_SYNC_FAILURE:
            cmd_ptr->type = GBA_COMMAND_AKA_RESPONSE_SYNC_FAILURE;
            break;
        default:
            cmd_ptr->type = GBA_COMMAND_AKA_RESPONSE_NETWORK_FAILURE;
            break;
    }
    cmd_ptr->u.akaResponse.respLength = 
            serviceEvt_ptr->u.aka.resLength;
    OSAL_memCpy(cmd_ptr->u.akaResponse.resp,
            serviceEvt_ptr->u.aka.response,
            GBA_AKA_AUTH_RESP_SZ);

    OSAL_memCpy(cmd_ptr->u.akaResponse.auts,
            serviceEvt_ptr->u.aka.auts,
            GBA_AKA_AUTH_AUTS_SZ);
    
    OSAL_memCpy(cmd_ptr->u.akaResponse.ck,
            serviceEvt_ptr->u.aka.ck,
            GBA_AKA_AUTH_CK_SZ);

    OSAL_memCpy(cmd_ptr->u.akaResponse.ik,
            serviceEvt_ptr->u.aka.ik,
            GBA_AKA_AUTH_IK_SZ);

    GBA_sendCommand(cmd_ptr);

    return (CSM_OK);
}

vint GBAM_akaArbitorSetTarget(GBAM_AkaArbitor akaRequester)
{
    OSAL_semAcquire(_GBAM_globalObj_ptr->akaLock, OSAL_WAIT_FOREVER);

    /* we got the lock and could safely remember the target */
    _GBAM_globalObj_ptr->lastAkaTarget = akaRequester;
    return (CSM_OK);
}

GBAM_AkaArbitor GBAM_akaArbitorGetTarget(void)
{
    GBAM_AkaArbitor target;

    target = _GBAM_globalObj_ptr->lastAkaTarget;

    /* now is safe to allow next aka request */
    OSAL_semGive(_GBAM_globalObj_ptr->akaLock);

    return (target);
}

