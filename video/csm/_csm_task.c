/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004-2012 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 29738 $ $Date: 2014-11-10 14:31:33 +0800 (Mon, 10 Nov 2014) $
 *
 */

#include <csm_event.h>
#include "_csm_event.h"
#include "_csm.h"
#include "_csm_isi_service.h"

/*
 * ======== _CSM_prepare() ========
 *
 * This is CSM init actions to be done at the task thread time
 *
 * Return Values:
 * None.
 */
void _CSM_prepare(
    CSM_GlobalObj   *csmObj_ptr
)
{
    /* 
     * Big D2 Banner with version information.
     */
    OSAL_logMsg("\n"
        "             D2 Technologies, Inc.\n"
        "      _   _  __                           \n"
        "     / | '/  /_  _  /_ _  _  /_  _  ._   _\n"
        "    /_.'/_  //_'/_ / // //_///_//_///_'_\\ \n"
        "                                _/        \n"
        "\n"
        "        Unified Communication Products\n");
    OSAL_logMsg(
        "               www.d2tech.com\n");

}

/*
 * ======== _CSM_processEvt() ========
 *
 * This is CSM_ event processing in task thread.
 * It handles events from csm input and sends interesting ones to output/isi
 *
 * Return Values:
 * None.
 */
void _CSM_processEvt(
    CSM_GlobalObj   *csmObj_ptr,
    OSAL_MsgQId      fromq)
{
    CSM_OutputEvent     *csmOutput_ptr = &csmObj_ptr->queue.csmOutputEvent;
    CSM_InputEvent      *csmInput_ptr;
    CSM_PrivateInputEvt *csmIntInput_ptr;

    if (fromq == csmObj_ptr->queue.inEvtQ) {
        csmInput_ptr = &csmObj_ptr->queue.inputEvt.publicEvt;
        CSM_dbgPrintf("Recv'd an InputEvent message type=%d\n",
                csmInput_ptr->type);
        /* Process the EM message */
        switch (csmInput_ptr->type) {
            case CSM_EVENT_TYPE_CALL:
                CSM_callsConvertToInternalEvt(CSM_INPUT_EVT_TYPE_EXT,
                        &csmInput_ptr->evt.call, &csmObj_ptr->evt.call);
                // run Calls Package
                CSM_callsProcessEvent(&csmObj_ptr->evt.call, csmOutput_ptr);
                break;
            case CSM_EVENT_TYPE_SMS:
                CSM_smsConvertToInternalEvt(CSM_INPUT_EVT_TYPE_EXT,
                        &csmInput_ptr->evt.sms, &csmObj_ptr->evt.sms);
                // run SMS Package
                CSM_smsProcessEvent(&csmObj_ptr->smsManager,
                        &csmObj_ptr->evt.sms, csmOutput_ptr);
                break;
            case CSM_EVENT_TYPE_SERVICE:
                CSM_serviceConvertToInternalEvt(CSM_INPUT_EVT_TYPE_EXT,
                        &csmInput_ptr->evt.service, &csmObj_ptr->evt.service);
                // run Accounts Package
                CSM_serviceProcessEvent(&csmObj_ptr->accountManager,
                        &csmObj_ptr->evt.service, csmOutput_ptr);
                break;
            case CSM_EVENT_TYPE_RADIO:
                // run RPM Package
                CSM_rpmProcessEvent(&csmInput_ptr->evt.radio);
                break;            
            case CSM_EVENT_TYPE_SUPSRV:
#ifdef INCLUDE_SUPSRV
                // run SUPSRV Package
                CSM_supSrvProcessEvent(&csmObj_ptr->supSrvManager, 
                        &csmInput_ptr->evt.supSrv, csmOutput_ptr);
#endif
                break;
            case CSM_EVENT_TYPE_USSD:
                CSM_ussdConvertToInternalEvt(CSM_INPUT_EVT_TYPE_EXT,
                        &csmInput_ptr->evt.ussd, &csmObj_ptr->evt.ussd);
                // run USSD Package
                CSM_ussdProcessEvent(&csmObj_ptr->ussdManager, 
                        &csmObj_ptr->evt.ussd, csmOutput_ptr);
                break;
            default:
                OSAL_logMsg("%s:%d error bad event type\n",
                        __FUNCTION__, __LINE__);
                break;
        }
    }
    else if (fromq == csmObj_ptr->queue.privateInEvtQ) {
        csmIntInput_ptr = &csmObj_ptr->queue.inputEvt.privateEvt;
        CSM_dbgPrintf("Recv'd an Private InputEvent message type=%d\n",
                csmIntInput_ptr->type);
        switch (csmIntInput_ptr->type) {
            case CSM_PRIVATE_EVENT_TYPE_SERVICE:
                CSM_serviceConvertToInternalEvt(CSM_INPUT_EVT_TYPE_INT,
                        &csmIntInput_ptr->evt.service,
                        &csmObj_ptr->evt.service);
                // run Accounts Package
                CSM_serviceProcessEvent(&csmObj_ptr->accountManager,
                        &csmObj_ptr->evt.service, csmOutput_ptr);
                break;
            case CSM_PRIVATE_EVENT_TYPE_CALL:
                CSM_callsConvertToInternalEvt(CSM_INPUT_EVT_TYPE_INT,
                        &csmIntInput_ptr->evt.call, &csmObj_ptr->evt.call);
                // run Calls Package
                CSM_callsProcessEvent(&csmObj_ptr->evt.call, csmOutput_ptr);
                break;
            case CSM_PRIVATE_EVENT_TYPE_SMS:
                CSM_smsConvertToInternalEvt(CSM_INPUT_EVT_TYPE_INT,
                        &csmIntInput_ptr->evt.sms, &csmObj_ptr->evt.sms);
                // run SMS Package
                CSM_smsProcessEvent(&csmObj_ptr->smsManager,
                        &csmObj_ptr->evt.sms, csmOutput_ptr);
                break;
            case CSM_PRIVATE_EVENT_TYPE_USSD:
                CSM_ussdConvertToInternalEvt(CSM_INPUT_EVT_TYPE_INT,
                        &csmIntInput_ptr->evt.ussd, &csmObj_ptr->evt.ussd);
                // run USSD Package
                CSM_ussdProcessEvent(&csmObj_ptr->ussdManager, 
                        &csmObj_ptr->evt.ussd, csmOutput_ptr);
                break;
            case CSM_PRIVATE_EVENT_TYPE_SUPSRV:
#ifdef INCLUDE_SUPSRV
                CSM_xcapProcessEvent(&csmObj_ptr->supSrvManager,
                        &csmIntInput_ptr->evt.xcap, csmOutput_ptr);
#endif
                break;
            case CSM_PRIVATE_EVENT_TYPE_INTERNAL:
                CSM_internalConvertToInternalEvt(&csmIntInput_ptr->evt.call,
                        &csmObj_ptr->evt.call);
                // run internal event
                CSM_internalProcessEvent(&csmObj_ptr->evt.call, csmOutput_ptr);
                break;
            default:
                OSAL_logMsg("%s:%d error bad event type\n",
                        __FUNCTION__, __LINE__);
                break;
        }
    }
}
/*
 * ======== _CSM_task() ========
 *
 * Main processing loop for voice
 */
void  _CSM_task(
    OSAL_TaskArg arg_ptr)
{
    CSM_GlobalObj       *csmObj_ptr    = (CSM_GlobalObj *)arg_ptr;
    OSAL_MsgQId          fromq;


    /* Block on EventManager command, event, response group queue */
_CSM_WORKER_TASK_LOOP:
    while (0 >= OSAL_msgQGrpRecv(&csmObj_ptr->queue.groupInEvt,
            (char *)&csmObj_ptr->queue.inputEvt,
            sizeof(csmObj_ptr->queue.inputEvt), OSAL_WAIT_FOREVER,
            &fromq, NULL)) {
        OSAL_taskDelay(100);
    }

    _CSM_processEvt(csmObj_ptr, fromq);

    goto _CSM_WORKER_TASK_LOOP;
}
