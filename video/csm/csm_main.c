/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2012 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 29993 $ $Date: 2014-11-21 12:03:21 +0800 (Fri, 21 Nov 2014) $
 */

#include <osal_types.h>
#include <osal.h>
#include <osal_msg.h>

#include <vpr.h>
#ifdef INCLUDE_RIR
#include <rir.h>
#endif
#ifdef INCLUDE_MC
#include <mc_main.h>
#endif
#include <sapp_main.h>
#ifdef INCLUDE_ISIM
#include <isim.h>
#else
#ifdef INCLUDE_MGT
#include <mgt.h>
#endif
#endif

#ifdef INCLUDE_GBA
#include <gbam.h>
#endif
#include <settings.h>
#include <ezxml_mem.h>
#include "_csm.h"
#include "csm_main.h"
#include "_csm_print.h"
#ifdef INCLUDE_4G_PLUS
#include "vpad_vpmd.h"
#endif
#include <isi_rpc.h>

/* Global object. */
static CSM_GlobalObj *_CSM_globalObj_ptr;

/* Forward delcaration for GAPP library functions */
#ifdef INCLUDE_GAPP
int GAPP_init(void *cfg_ptr);
void GAPP_shutdown(void);
#endif

/*
 * ======== CSM_getObject() ========
 *
 * This function returns a pointer to the CSM global object singleton.
 *
 * WARNING this must be called after _CSM_start() exits
 * 
 * Returns:
 *  A pointer to the CSM_GlobalObj singleton
 *  
 */
CSM_GlobalObj* CSM_getObject(
    void)
{
    return (_CSM_globalObj_ptr);
}

/*
 * ======== _CSM_initQueues() ========
 *
 * This function is a private helper used to initialize CSM command & event
 * message Q's.  These Q's are use to receive CSM_Commands from external
 * applications and send CSM_Events back.
 *
 * Returns:
 * CSM_OK: function exits normally.
 * CSM_ERR: in case of error
 *  
 */
static vint _CSM_initQueues(
    CSM_GlobalObj *csmObj_ptr)
{
    /* 
     * Create Event Queue (Recv only)
     *
     * Incoming events from GAPP AT, RIR
     */
    if (0 == (csmObj_ptr->queue.inEvtQ = OSAL_msgQCreate(
            CSM_INPUT_EVENT_QUEUE_NAME,
            OSAL_MODULE_REV_ONLY, OSAL_MODULE_CSM_PUBLIC,
            OSAL_DATA_STRUCT_CSM_InputEvent,
            CSM_INPUT_EVENT_MSGQ_LEN,
            sizeof(CSM_InputEvent), 0))) {
        OSAL_logMsg("%s:%d CSM_ERR\n", __FUNCTION__, __LINE__);
        return (CSM_ERR);
    }
    /* 
     * Create Private Event Queue (Recv only)
     *
     * Incoming events from ISI & XCAP
     */
    if (0 == (csmObj_ptr->queue.privateInEvtQ = OSAL_msgQCreate(
            CSM_PRIVATE_INPUT_EVT_MSGQ_NAME,
            OSAL_MODULE_CSM_PRIVATE, OSAL_MODULE_CSM_PRIVATE,
            OSAL_DATA_STRUCT_CSM_PrivateInputEvt,
            CSM_PRIVATE_INPUT_EVT_MSGQ_LEN,
            sizeof(CSM_PrivateInputEvt), 0))) {
        OSAL_logMsg("%s:%d CSM_ERR\n", __FUNCTION__, __LINE__);
        return (CSM_ERR);
    }
    /* Create input event group queue */
    if (OSAL_SUCCESS != OSAL_msgQGrpCreate(&csmObj_ptr->queue.groupInEvt)) {
        OSAL_msgQDelete(csmObj_ptr->queue.inEvtQ);
        OSAL_msgQDelete(csmObj_ptr->queue.privateInEvtQ);
        OSAL_logMsg("%s: ERROR!\n", __FUNCTION__);
        return (CSM_ERR);
    }
    if (OSAL_SUCCESS != OSAL_msgQGrpAddQ(&csmObj_ptr->queue.groupInEvt,
            csmObj_ptr->queue.inEvtQ)) {
        OSAL_msgQDelete(csmObj_ptr->queue.inEvtQ);
        OSAL_msgQDelete(csmObj_ptr->queue.privateInEvtQ);
        OSAL_logMsg("%s: ERROR!\n", __FUNCTION__);
        return (CSM_ERR);
    }
    if (OSAL_SUCCESS != OSAL_msgQGrpAddQ(&csmObj_ptr->queue.groupInEvt,
            csmObj_ptr->queue.privateInEvtQ)) {
        OSAL_msgQDelete(csmObj_ptr->queue.inEvtQ);
        OSAL_msgQDelete(csmObj_ptr->queue.privateInEvtQ);
        OSAL_logMsg("%s: ERROR!\n", __FUNCTION__);
        return (CSM_ERR);
    }

    /*
     * Create Output Event Queue (Send only)
     *
     * These SolicitedResponse go back to GSM_AT Looper process
     * for conversion to AT and forward upward
     */
    if (0 == (csmObj_ptr->queue.outEvtQ = OSAL_msgQCreate(
            CSM_OUTPUT_EVENT_QUEUE_NAME,
            OSAL_MODULE_CSM_PUBLIC, OSAL_MODULE_GAPP,
            OSAL_DATA_STRUCT_CSM_OutputEvent,
            CSM_OUTPUT_EVENT_MSGQ_LEN,
            sizeof(CSM_OutputEvent), 0))) {
        OSAL_msgQDelete(csmObj_ptr->queue.inEvtQ);        
        OSAL_logMsg("%s:%d CSM_ERR\n", __FUNCTION__, __LINE__);
        return (CSM_ERR);
    }

    /*
     * Create  ISIM (4gApp) Output Event Queue (Send only)
     */
#ifdef INCLUDE_ISIM
    if (0 == (csmObj_ptr->queue.isimOutEvtQ = OSAL_msgQCreate(
            CSM_OUTPUT_ISIM_EVENT_QUEUE_NAME,
            OSAL_MODULE_CSM_PUBLIC, OSAL_MODULE_ISIM,
            OSAL_DATA_STRUCT_CSM_OutputEvent,
            CSM_OUTPUT_ISIM_EVENT_MSGQ_LEN,
            sizeof(CSM_OutputEvent), 0))) {
        OSAL_msgQDelete(csmObj_ptr->queue.outEvtQ);
        OSAL_msgQDelete(csmObj_ptr->queue.inEvtQ);
        OSAL_logMsg("%s:%d CSM_ERR\n", __FUNCTION__, __LINE__);
        return (CSM_ERR);
    }
#else
    /*
     * If ISIM does not enable, isimOutEvtQ need point to outEvtQ.
     * CSM output event of ISIM will send to other protocol application by
     * CSM output event queue.
     */
    csmObj_ptr->queue.isimOutEvtQ = csmObj_ptr->queue.outEvtQ;
#ifdef INCLUDE_MGT
    if (0 == (csmObj_ptr->queue.mgtOutEvtQ = OSAL_msgQCreate(
            CSM_OUTPUT_ISIM_EVENT_QUEUE_NAME,
            OSAL_MODULE_CSM_PUBLIC, OSAL_MODULE_MGT,
            OSAL_DATA_STRUCT_CSM_OutputEvent,
            CSM_OUTPUT_ISIM_EVENT_MSGQ_LEN,
            sizeof(CSM_OutputEvent), 0))) {
        OSAL_msgQDelete(csmObj_ptr->queue.outEvtQ);
        OSAL_msgQDelete(csmObj_ptr->queue.inEvtQ);
        OSAL_logMsg("%s:%d CSM_ERR\n", __FUNCTION__, __LINE__);
        return (CSM_ERR);
    }
#else
    csmObj_ptr->queue.isimOutEvtQ = csmObj_ptr->queue.outEvtQ;
#endif
#endif

    return (CSM_OK);
}

/*
 * ======== _CSM_initProtocols() ========
 *
 * This function is a private helper used to initialize the internal vPort4G
 * protocols (SAPP, MC, GAPP, RIR, etc.)
 *
 * Returns:
 * CSM_OK: function exits normally.
 * CSM_ERR: in case of error
 *
 */
static vint _CSM_initProtocols(
    CSM_GlobalObj *csmObj_ptr)
{
    void *cfg_ptr;

    CSM_dbgPrintf("\n");

#ifdef INCLUDE_GAPP
    cfg_ptr = SETTINGS_cfgMemAlloc(SETTINGS_TYPE_GAPP);
    if (SETTINGS_RETURN_OK != SETTINGS_getContainer(SETTINGS_TYPE_GAPP,
            CSM_GAPP_XML_FILE_NAME, cfg_ptr)) {
        SETTINGS_memFreeDoc(SETTINGS_TYPE_GAPP, cfg_ptr);
        OSAL_logMsg("%s:%d ERROR opening gapp settings\n",
                __FUNCTION__, __LINE__);
        return (CSM_ERR);
    }
    /*
     * Init GAPP: GSM Application + AT worker/looper
     */
    if (0 != GAPP_init(cfg_ptr)) {
        SETTINGS_memFreeDoc(SETTINGS_TYPE_GAPP, cfg_ptr);
        OSAL_logMsg("Failed to init GAPP!\n");
        return (CSM_ERR);
    }
    SETTINGS_memFreeDoc(SETTINGS_TYPE_GAPP, cfg_ptr);
#endif

/* In Seattle, SAPP is initilized by mCUE. */
#ifndef CSM_DISABLE_SAPP
    cfg_ptr = SETTINGS_cfgMemAlloc(SETTINGS_TYPE_SAPP);
    if (SETTINGS_RETURN_OK != SETTINGS_getContainer(SETTINGS_TYPE_SAPP,
            CSM_SAPP_XML_FILE_NAME, cfg_ptr)) {
        SETTINGS_memFreeDoc(SETTINGS_TYPE_SAPP, cfg_ptr);
        OSAL_logMsg("%s:%d ERROR opening settings\n",
                __FUNCTION__, __LINE__);
        return (CSM_ERR);
    }
    /*
     * Init SAPP: SIP Application 
     */
    if (0 != SAPP_init(cfg_ptr, NULL)) {
        SETTINGS_memFreeDoc(SETTINGS_TYPE_SAPP, cfg_ptr);
        OSAL_logMsg("Failed to init SAPP!\n");
        return (CSM_ERR);
    }
    SETTINGS_memFreeDoc(SETTINGS_TYPE_SAPP, cfg_ptr);
#endif
#ifdef INCLUDE_MC
    cfg_ptr = SETTINGS_cfgMemAlloc(SETTINGS_TYPE_MC);
    if (SETTINGS_RETURN_OK != SETTINGS_getContainer(SETTINGS_TYPE_MC,
            CSM_MC_XML_FILE_NAME, cfg_ptr)) {
        SETTINGS_memFreeDoc(SETTINGS_TYPE_MC, cfg_ptr);
        OSAL_logMsg("%s:%d ERROR opening settings\n",
                __FUNCTION__, __LINE__);
        return (CSM_ERR);
    }
    /*
     * Init MC: Media controller (Audio only) 
     */
    if (0 != MC_init(cfg_ptr)) {
        SETTINGS_memFreeDoc(SETTINGS_TYPE_MC, cfg_ptr);
        OSAL_logMsg("Failed to init MC!\n");
        return (CSM_ERR);
    }
    SETTINGS_memFreeDoc(SETTINGS_TYPE_MC, cfg_ptr);
#endif

#ifdef INCLUDE_RIR
    /*
     * Init RIR: Radio Interface Reporter
     */
    /* Get the XML init info */
    cfg_ptr = SETTINGS_cfgMemAlloc(SETTINGS_TYPE_RIR);
    if (SETTINGS_RETURN_OK != SETTINGS_getContainer(SETTINGS_TYPE_RIR,
            CSM_RIR_XML_FILE_NAME, cfg_ptr)) {
        SETTINGS_memFreeDoc(SETTINGS_TYPE_RIR, cfg_ptr);
        OSAL_logMsg("%s:%d ERROR opening rir.xml\n", __FUNCTION__, __LINE__);
        return (CSM_ERR);
    }

    if (0 != RIR_init(cfg_ptr)) {
        SETTINGS_memFreeDoc(SETTINGS_TYPE_RIR, cfg_ptr);
        OSAL_logMsg("Failed to init RIR!\n");
        return (CSM_ERR);
    }
    SETTINGS_memFreeDoc(SETTINGS_TYPE_RIR, cfg_ptr);
#endif

#ifdef INCLUDE_GBA
    /* 
     * Init both GBAM and GBA
     */
    if (0 != GBAM_init()) {
        OSAL_logMsg("Failed to init GBAM!\n");
        return (CSM_ERR);
    }
#endif

#ifdef INCLUDE_HTTP
    HTTP_allocate();
    HTTP_start();
#endif

    return (CSM_OK);
}


/*
 * ======== _CSM_allocateProtocols() ========
 *
 * This function is a private helper used to initialize the internal vPort4G
 * protocols (SAPP, MC, GAPP, RIR, etc.)
 *
 * Returns:
 *      OSAL_SUCCESS: function exits normally.
 *      OSAL_FAIL: in case of error
 *  
 */
static vint _CSM_allocateProtocols(
    CSM_GlobalObj *csmObj_ptr)
{
#ifdef INCLUDE_GAPP
    void *cfg_ptr;
#endif

    CSM_dbgPrintf("\n");

#ifdef INCLUDE_GAPP
    /* xxx refactor later */
    cfg_ptr = SETTINGS_cfgMemAlloc(SETTINGS_TYPE_GAPP);
    if (SETTINGS_RETURN_OK != SETTINGS_getContainer(SETTINGS_TYPE_GAPP,
            NULL, cfg_ptr)) {
        SETTINGS_memFreeDoc(SETTINGS_TYPE_GAPP, cfg_ptr);
        OSAL_logMsg("%s:%d ERROR opening gapp settings\n",
                __FUNCTION__, __LINE__);
        return (OSAL_FAIL);
    }
    /*
     * Init GAPP: GSM Application + AT worker/looper
     */
    if (0 != GAPP_init(cfg_ptr)) {
        SETTINGS_memFreeDoc(SETTINGS_TYPE_GAPP, cfg_ptr);
        OSAL_logMsg("Failed to init GAPP!\n");
        return (OSAL_FAIL);
    }
    SETTINGS_memFreeDoc(SETTINGS_TYPE_GAPP, cfg_ptr);
#endif

/* In Seattle, SAPP is initilized by mCUE. */
#ifndef CSM_DISABLE_SAPP
    /*
     * Init SAPP: SIP Application, including SIP stack
     */
    if (OSAL_SUCCESS != SAPP_allocate()) {
        OSAL_logMsg("Failed to allocate SAPP!\n");
        return (OSAL_FAIL);
    }

#endif

#ifdef INCLUDE_MC
    if (OSAL_SUCCESS != MC_allocate()) {
        OSAL_logMsg("Failed to allocate MC!\n");
        return (OSAL_FAIL);
    }
#endif

#ifdef INCLUDE_RIR

#endif

#ifdef INCLUDE_GBA
    /* xxx refactor later */
    /* 
     * Init both GBAM and GBA
     */
    if (0 != GBAM_init()) {
        OSAL_logMsg("Failed to init GBAM!\n");
        return (OSAL_FAIL);
    }
#endif
    return (OSAL_SUCCESS);
}

/*
 * ======== _CSM_startProtocols() ========
 *
 * This function is a private helper used to initialize the internal vPort4G
 * protocols (SAPP, MC, GAPP, RIR, etc.)
 *
 * Returns:
 *      OSAL_SUCCESS: function exits normally.
 *      OSAL_FAIL: in case of error
 *  
 */
static vint _CSM_startProtocols(
    CSM_GlobalObj *csmObj_ptr)
{
#ifdef INCLUDE_RIR
    void *cfg_ptr;
#endif

    CSM_dbgPrintf("\n");

#ifdef INCLUDE_GAPP
    
#endif

/* In Seattle, SAPP is initilized by mCUE. */
#ifndef CSM_DISABLE_SAPP
    if (OSAL_SUCCESS != SAPP_start()) {
        OSAL_logMsg("Failed to start SAPP!\n");
        return (OSAL_FAIL);
    }
#endif

#ifdef INCLUDE_MC
    if (OSAL_SUCCESS != MC_start()) {
        OSAL_logMsg("Failed to start MC!\n");
        return (OSAL_FAIL);
    }
#endif

#ifdef INCLUDE_RIR
    /*
     * Init RIR: Radio Interface Reporter
     */
    /* xxx refactor later */
    /* Get the XML init info */
    cfg_ptr = SETTINGS_cfgMemAlloc(SETTINGS_TYPE_RIR);
    if (SETTINGS_RETURN_OK != SETTINGS_getContainer(SETTINGS_TYPE_RIR,
            NULL, cfg_ptr)) {
        SETTINGS_memFreeDoc(SETTINGS_TYPE_RIR, cfg_ptr);
        OSAL_logMsg("%s:%d ERROR opening rir.xml\n", __FUNCTION__, __LINE__);
        return (OSAL_FAIL);
    }

    if (0 != RIR_init(cfg_ptr)) {
        SETTINGS_memFreeDoc(SETTINGS_TYPE_RIR, cfg_ptr);
        OSAL_logMsg("Failed to init RIR!\n");
        return (OSAL_FAIL);
    }
    SETTINGS_memFreeDoc(SETTINGS_TYPE_RIR, cfg_ptr);
#endif

#ifdef INCLUDE_GBA
    
#endif
    return (OSAL_SUCCESS);
}

/*
 * ======== _CSM_initManagers() ========
 *
 * This function is a private helper used to initialize the internal CSM
 * package managers responsible for accounts, calling, suplementary services, 
 * sms, radio policy, etc.
 *
 * Returns: 
 * CSM_OK: function exits normally.
 * CSM_ERR: in case of error
 *  
 */
static vint _CSM_initManagers(
    CSM_GlobalObj *csmObj_ptr,
    void          *cfg_ptr)
{
    CSM_dbgPrintf("\n");

    /*  Init ISI Manager */
    if (CSM_OK != CSM_isiInit(&csmObj_ptr->isiManager,
            csmObj_ptr->queue.privateInEvtQ)) {
        return (CSM_ERR);
    }

    /*  Init CALL package */
    if (CSM_OK != CSM_callsInit(&csmObj_ptr->callManager,
            &csmObj_ptr->isiManager)) {
        return (CSM_ERR);
    }

    /*  Init SMS package */
    if (CSM_OK != CSM_smsInit(&csmObj_ptr->smsManager, 
            &csmObj_ptr->isiManager, cfg_ptr)) {
        return (CSM_ERR);
    }

    /*  Init Account Package */
    if (CSM_OK != CSM_serviceInit(&csmObj_ptr->accountManager,
            &csmObj_ptr->isiManager,
#ifdef INCLUDE_SUPSRV
            &csmObj_ptr->supSrvManager,
#endif
            cfg_ptr)) {
        return (CSM_ERR);
    }
    
    /*  Init Radio Policy Manager (RPM) */
    if (CSM_OK != CSM_rpmInit(&csmObj_ptr->accountManager)) {
        return (CSM_ERR);
    }

#ifdef INCLUDE_SUPSRV
    /*  Init Supplementary Service package */
    if (CSM_OK != CSM_supSrvInit(&csmObj_ptr->supSrvManager,
            csmObj_ptr->queue.privateInEvtQ)) {
        return (CSM_ERR);
    }
#endif

    /*  Init USSD package */
    if (CSM_OK != CSM_ussdInit(&csmObj_ptr->ussdManager, 
        &csmObj_ptr->isiManager)) {
        return (CSM_ERR);
    }
    
    /*  All good */
    return (CSM_OK);
}


/*
 * ======== _CSM_allocateManagers() ========
 *
 * This function is a private helper used to initialize the internal CSM
 * package managers responsible for accounts, calling, suplementary services, 
 * sms, radio policy, etc.
 *
 * Returns:
 *      OSAL_SUCCESS: function exits normally.
 *      OSAL_FAIL: in case of error
 *  
 */
static vint _CSM_allocateManagers(
    CSM_GlobalObj *csmObj_ptr,
    void          *cfg_ptr)
{
    ISI_Return ret;
    CSM_dbgPrintf("\n");
    /******** special not fully modulized subsystems ********/
    /* Init ISI native */
    ret = ISI_allocate(NULL);
    if (ISI_RETURN_OK != ret) {
        OSAL_logMsg("The ISI module could not be initialized ERROR:%s\n",
                CSM_isiPrintReturnString(ret));
        return (OSAL_FAIL);
    }
    /* Init ISI server */
    if (OSAL_SUCCESS != ISI_serverAllocate()) {
        OSAL_logMsg("%s:%d ERROR initial ISI Server\n", __FUNCTION__, __LINE__);
        return (OSAL_FAIL);
    }
    /*  allocate ISI Manager */
    if (OSAL_SUCCESS != CSM_isiAllocate(&csmObj_ptr->isiManager,
            csmObj_ptr->queue.privateInEvtQ)) {
        return (OSAL_FAIL);
    }

    /*  allocate CALL package */
    if (CSM_OK != CSM_callsInit(&csmObj_ptr->callManager,
            &csmObj_ptr->isiManager)) {
        return (OSAL_FAIL);
    }

    /*  allocate SMS package */
    if (CSM_OK != CSM_smsInit(&csmObj_ptr->smsManager, 
            &csmObj_ptr->isiManager, cfg_ptr)) {
        return (OSAL_FAIL);
    }

    /*  allocate Account Package */
    if (CSM_OK != CSM_serviceInit(&csmObj_ptr->accountManager,
            &csmObj_ptr->isiManager,
#ifdef INCLUDE_SUPSRV
            &csmObj_ptr->supSrvManager,
#endif
            cfg_ptr)) {
        return (OSAL_FAIL);
    }
    
    /*  allocate Radio Policy Manager (RPM) */
    if (CSM_OK != CSM_rpmInit(&csmObj_ptr->accountManager)) {
        return (OSAL_FAIL);
    }

#ifdef INCLUDE_SUPSRV
    /*  allocate Supplementary Service package */
    /* xxx refactor on next milestone */
    if (CSM_OK != CSM_supSrvInit(&csmObj_ptr->supSrvManager,
            csmObj_ptr->queue.privateInEvtQ)) {
        return (OSAL_FAIL);
    }
#endif

    /*  allocate USSD package */
    if (CSM_OK != CSM_ussdInit(&csmObj_ptr->ussdManager, 
        &csmObj_ptr->isiManager)) {
        return (OSAL_FAIL);
    }
    
    /*  All good */
    return (OSAL_SUCCESS);
}


/*
 * ======== _CSM_startManagers() ========
 *
 * This function is a private helper used to initialize the internal CSM
 * package managers responsible for accounts, calling, suplementary services, 
 * sms, radio policy, etc.
 *
 * Returns:
 *      OSAL_SUCCESS: function exits normally.
 *      OSAL_FAIL: in case of error
 *  
 */
static vint _CSM_startManagers(
    CSM_GlobalObj *csmObj_ptr,
    void          *cfg_ptr)
{
    CSM_dbgPrintf("\n");
    if (OSAL_SUCCESS != ISI_serverStart()) {
        return (OSAL_FAIL);
    }
    /*  start ISI Manager */
    if (OSAL_SUCCESS != CSM_isiStart(&csmObj_ptr->isiManager)) {
        return (OSAL_FAIL);
    }
    /*  All good */
    return (OSAL_SUCCESS);
}

/*
 * ======== _CSM_shutdownManagers() ========
 *
 * This function is a private helper used to de-initialize the internal CSM
 * package managers responsible for accounts, calling, suplementary services, 
 * sms, radio policy, etc.
 *
 * Returns: 
 * CSM_OK: function exits normally.
 * CSM_ERR: in case of error
 *  
 */
static vint _CSM_shutdownManagers(
    CSM_GlobalObj *csmObj_ptr)
{
    CSM_dbgPrintf("\n");

    /* Shut down the CALLs package */
    if (CSM_OK != CSM_callsShutdown()) {
        return (CSM_ERR);
    }

    /* Shut down the SMS package */
    if (CSM_OK != CSM_smsShutdown(&csmObj_ptr->smsManager)) {
        return (CSM_ERR);
    }

    /* Shut down the Account package */
    if (CSM_OK != CSM_serviceShutdown(&csmObj_ptr->accountManager)) {
        return (CSM_ERR);
    }

    /* Shut down the RPM package */
    if (CSM_OK != CSM_rpmShutdown()) {
        return (CSM_ERR);
    }

    /* Shut down the ISI package */
    if (CSM_OK != CSM_isiShutdown(&csmObj_ptr->isiManager)) {
        return (CSM_ERR);
    }

#ifdef INCLUDE_SUPSRV
    /* Shut down the SUPSRV package */
    if (CSM_OK != CSM_supSrvShutdown(&csmObj_ptr->supSrvManager)) {
        return (CSM_ERR);
    }
#endif

    /* Shut down the USSD package */
    if (CSM_OK != CSM_ussdShutdown(&csmObj_ptr->ussdManager)) {
        return (CSM_ERR);
    }
    return (CSM_OK);
}

/*
 * ======== _CSM_destroyManagers() ========
 *
 * This function is a private helper used to de-initialize the internal CSM
 * package managers responsible for accounts, calling, suplementary services, 
 * sms, radio policy, etc.
 *
 * Returns: 
 * CSM_OK: function exits normally.
 * CSM_ERR: in case of error
 *  
 */
static vint _CSM_destroyManagers(
    CSM_GlobalObj *csmObj_ptr)
{
    CSM_dbgPrintf("\n");

    /* Shut down the CALLs package */
    if (CSM_OK != CSM_callsShutdown()) {
        return (CSM_ERR);
    }

    /* Shut down the SMS package */
    if (CSM_OK != CSM_smsShutdown(&csmObj_ptr->smsManager)) {
        return (CSM_ERR);
    }

    /* Shut down the Account package */
    if (CSM_OK != CSM_serviceShutdown(&csmObj_ptr->accountManager)) {
        return (CSM_ERR);
    }

    /* Shut down the RPM package */
    if (CSM_OK != CSM_rpmShutdown()) {
        return (CSM_ERR);
    }

    /* Shut down the ISI package */
    if (CSM_OK != CSM_isiDestroy(&csmObj_ptr->isiManager)) {
        return (CSM_ERR);
    }

#ifdef INCLUDE_SUPSRV
    /* Shut down the SUPSRV package */
    if (CSM_OK != CSM_supSrvShutdown(&csmObj_ptr->supSrvManager)) {
        return (CSM_ERR);
    }
#endif

    /* Shut down the USSD package */
    if (CSM_OK != CSM_ussdShutdown(&csmObj_ptr->ussdManager)) {
        return (CSM_ERR);
    }
    return (CSM_OK);
}

/*
 * ======== _CSM_shutdownQueues() ========
 *
 * This function is a private helper used to de-initialize the CSM message Q's
 * for commands and events
 *
 * Returns: 
 * CSM_OK: function exits normally.
 * CSM_ERR: in case of error
 *  
 */
static vint _CSM_shutdownQueues(
    CSM_GlobalObj *csmObj_ptr)
{
    CSM_dbgPrintf("\n");

    if (OSAL_SUCCESS != OSAL_msgQDelete(csmObj_ptr->queue.inEvtQ)) {
        return (CSM_ERR);
    }
    if (OSAL_SUCCESS != OSAL_msgQDelete(csmObj_ptr->queue.privateInEvtQ)) {
        return (CSM_ERR);
    }
    if (OSAL_SUCCESS != OSAL_msgQDelete(csmObj_ptr->queue.outEvtQ)) {
        return (CSM_ERR);
    }
    if (OSAL_SUCCESS != OSAL_msgQDelete(csmObj_ptr->queue.isimOutEvtQ)) {
        return (CSM_ERR);
    }
    if (OSAL_SUCCESS != OSAL_msgQGrpDelete(&csmObj_ptr->queue.groupInEvt)) {
        return (CSM_ERR);
    }
    return (CSM_OK);
}

/*
 * ======== _CSM_shutdownProtocols() ========
 *
 * This function is a private helper used to de-initialize the internal vPort4G
 * protocols (SAPP, MC, GAPP, RIR, etc.)
 *
 * Returns: 
 * CSM_OK: function exits normally.
 * CSM_ERR: in case of error
 *  
 */
static vint _CSM_shutdownProtocols(
    CSM_GlobalObj *csmObj_ptr)
{
    CSM_dbgPrintf("\n");

#ifdef INCLUDE_RIR
    /*
     * Stop RIR
     */
    RIR_shutdown();
#endif
#ifdef INCLUDE_MC
    /*
     * Stop MC
     */
    MC_shutdown();
#endif
#ifndef CSM_DISABLE_SAPP
    /*
     * Stop SAPP
     */
    SAPP_shutdown();
#endif
#ifdef INCLUDE_GAPP
    /*
     * Stop GAPP
     */
    GAPP_shutdown();
#endif
    
#ifdef INCLUDE_GBA
    /*
     * Stop both gbam and gba
     */
    GBAM_shutdown();
#endif

#ifdef INCLUDE_HTTP
    HTTP_destroy();
#endif
    return (CSM_OK);
}


/*
 * ======== _CSM_destroyProtocols() ========
 *
 * This function is a private helper used to de-initialize the internal vPort4G
 * protocols (SAPP, MC, GAPP, RIR, etc.)
 *
 * Returns: 
 * CSM_OK: function exits normally.
 * CSM_ERR: in case of error
 *  
 */
static vint _CSM_destroyProtocols(void)
{
    CSM_dbgPrintf("\n");

#ifdef INCLUDE_RIR
    /*
     * Stop RIR
     */
    RIR_shutdown();
#endif
#ifdef INCLUDE_MC
    /*
     * Stop MC
     */
    MC_shutdown();
#endif
#ifndef CSM_DISABLE_SAPP
    /*
     * Stop SAPP
     */
    SAPP_shutdown();
#endif
#ifdef INCLUDE_GAPP
    /*
     * Stop GAPP
     */
    GAPP_shutdown();
#endif
    
#ifdef INCLUDE_GBA
    /*
     * Stop both gbam and gba
     */
    GBAM_shutdown();
#endif

    return (CSM_OK);
}

/*
 * ======== _CSM_start() ========
 *
 * This function is a private helper used to initialize all the internal
 * packages, protocols, message queues, etc needed for CSM to operate
 *
 * Returns:
 * CSM_OK: function exits normally.
 * CSM_ERR: in case of error
 *  
 */
static vint _CSM_start(
    void          *cfg_ptr,
    CSM_GlobalObj *csmObj_ptr)
{
    CSM_dbgPrintf("\n");
    /*
     * Initialize the CSM message queues
     */
    _CSM_initQueues(csmObj_ptr);

    /*
     * Init vPort4G Protocols (SAPP, MC, GAPP, RIR, etc).
     */
    _CSM_initProtocols(csmObj_ptr);

    /*
     * Initialize the sub packages (call, sms, account, rpm, isi)
     */
    _CSM_initManagers(csmObj_ptr, cfg_ptr);

    /*
     * Initialize isim thread.
     */
#ifdef INCLUDE_ISIM
    ISIM_init();
#else
#ifdef INCLUDE_MGT
    MGT_init();
#endif
#endif

    if (OSAL_FAIL == CSM_start(csmObj_ptr)) {
            /* Error.  Clean up and exit. */
        _CSM_shutdownManagers(csmObj_ptr);
        _CSM_shutdownProtocols(csmObj_ptr);
        _CSM_shutdownQueues(csmObj_ptr);
        return (CSM_ERR);
    }
    return (CSM_OK);
}


/*
 * ======== _CSM_deallocate() ========
 *
 * Internal routine for free up the CSM module resources
 *
 * Returns:
 *      OSAL_SUCCESS: function exits normally.
 *      OSAL_FAIL: in case of error
 */
vint _CSM_deallocate(
    CSM_GlobalObj *csmObj_ptr)
{
    vint ret = OSAL_SUCCESS;

    if (CSM_ERR == _CSM_shutdownQueues(csmObj_ptr)) {
        ret = OSAL_FAIL;
    }

    /*
     * Clean house and exit
     */
    OSAL_memFree(_CSM_globalObj_ptr, 0);

    return (ret);
}

/*
 * ======== _CSM_stop() ========
 *
 * Internal routine for stoping the CSM module task
 *
 * Returns:
 *      OSAL_SUCCESS: function exits normally.
 *      OSAL_FAIL: in case of error
 */
vint _CSM_stop(
     CSM_GlobalObj *csmObj_ptr)
{
    OSAL_taskDelete(csmObj_ptr->task.tId);
    return (OSAL_SUCCESS);
}

/*
 * ======== CSM_allocate() ========
 *
 * Public routine for allocating the CSM module resource
 *
 * Returns:
 *      OSAL_SUCCESS: function exits normally.
 *      OSAL_FAIL: in case of error
 */
vint CSM_allocate(void)
{
    /*
     * Init and clear the global object that manages emcrything for
     * this process
     */
    _CSM_globalObj_ptr = OSAL_memCalloc(1, sizeof(CSM_GlobalObj), 0);
    OSAL_memSet(_CSM_globalObj_ptr, 0, sizeof(CSM_GlobalObj));

    if (CSM_ERR == _CSM_initQueues(_CSM_globalObj_ptr)) {
        return (OSAL_FAIL);
    }
    return (OSAL_SUCCESS);
}

/*
 * ======== CSM_start() ========
 *
 * Public routine for starting the CSM module tasks/actions
 *
 * Returns:
 *      OSAL_SUCCESS: function exits normally.
 *      OSAL_FAIL: in case of error
 */
vint CSM_start(
     void *obj_ptr)
{
    CSM_GlobalObj *csmObj_ptr = (CSM_GlobalObj *)obj_ptr;
    CSM_TaskObj *task_ptr;

    /*
     * Init & start CSM serialized worker / looper task
     */
    task_ptr = &csmObj_ptr->task;
    task_ptr->tId      = 0;
    task_ptr->stackSz  = CSM_TASK_STACK_BYTES;
    task_ptr->pri      = OSAL_TASK_PRIO_NRT;
    task_ptr->func_ptr = _CSM_task;
    OSAL_snprintf(task_ptr->name, sizeof(task_ptr->name), "%s",
            CSM_SIP_TASK_NAME);
    task_ptr->arg_ptr  = (void *)csmObj_ptr;
    /* Start task */
    if (0 == (task_ptr->tId = OSAL_taskCreate(task_ptr->name,
            (OSAL_TaskPrio)task_ptr->pri,
            task_ptr->stackSz,
            (OSAL_TaskPtr)task_ptr->func_ptr,
            (void *)task_ptr->arg_ptr))) {

        return (OSAL_FAIL);
    }
    return (OSAL_SUCCESS);
}

/*
 * ======== CSM_destroy() ========
 *
 * Public routine for shutting down the ISI manager package.
 *
 * Returns:
 *      OSAL_SUCCESS: function exits normally.
 *      OSAL_FAIL: in case of error
 */
vint CSM_destroy(
     void *obj_ptr)
{
    CSM_GlobalObj *csmObj_ptr = (CSM_GlobalObj *)obj_ptr;

    if (NULL == csmObj_ptr) {
        return (CSM_OK);
    }

    _CSM_stop(csmObj_ptr);
    _CSM_deallocate(csmObj_ptr);

    return (CSM_OK);
}
/*
 * ======== CSM_init() ========
 *
 * This function is a public entry point into the CSM library used to
 * initialize all the private internals of CSM.
 *
 * Returns:
 * CSM_OK: function exits normally.
 * CSM_ERR: in case of error
 *
 */
vint CSM_init(
    void *cfg_ptr)
{
    CSM_dbgPrintf("\n");

    /*
     * Init and clear the global object that manages emcrything for
     * this process
     */
    _CSM_globalObj_ptr = OSAL_memCalloc(1, sizeof(CSM_GlobalObj), 0);
    OSAL_memSet(_CSM_globalObj_ptr, 0, sizeof(CSM_GlobalObj));

    /* Start VPR */
    if (OSAL_FAIL == VPR_startDaemon()) {
        OSAL_logMsg("%s:%d ERROR!!! VPR start failed.\n", __FILE__, __LINE__);
        return (-1);
    }

    /*
     * Start CSM
     */
    if (CSM_OK != _CSM_start(cfg_ptr, _CSM_globalObj_ptr)) {
        OSAL_logMsg("%s:%d ERROR Failed to Init the CSM Application\n",
                __FUNCTION__, __LINE__);
        return (CSM_ERR);
    }
     
    return (CSM_OK);
}

/*
 * ======== CSM_shutdown() ========
 *
 * This function is a public exit point for the CSM library used to
 * de-initialize all the private internals of CSM.
 *
 * Returns:
 * CSM_OK: function exits normally.
 * CSM_ERR: in case of error
 *  
 */
void CSM_shutdown(
    void)
{
    CSM_dbgPrintf("\n");

    /*
     * Shutdown the CSM task.
     */
    _CSM_stop(_CSM_globalObj_ptr);

    /*
     * Shutdown CSM package managers (call, account, sup, sms, rpm, etc)
     */
    if (CSM_OK != _CSM_shutdownManagers(_CSM_globalObj_ptr)) {
        OSAL_logMsg("Failed to shutdown a package\n");
    }

    /*
     * Delete queues.
     */
    if (CSM_OK != _CSM_shutdownQueues(_CSM_globalObj_ptr)) {
        OSAL_logMsg("Failed to destroy Q's\n");
    }
    /* Destroy EZXML  */
    EZXML_destroy();

    /*
     * Shutdown protocol componenets SAPP, MC, GAPP, RIR, etc.
     */
    if (CSM_OK != _CSM_shutdownProtocols(_CSM_globalObj_ptr)) {
        OSAL_logMsg("Failed to destroy Protocols\n");
    }

    /*
     * Shutdown isim thread.
     */
#ifdef INCLUDE_ISIM
    if (CSM_OK != ISIM_shutdown()) {
        OSAL_logMsg("Failed to shutdown ISIM thread\n");
    }
#else
#ifdef INCLUDE_MGT
    if (CSM_OK != MGT_shutdown()) {
        OSAL_logMsg("Failed to shutdown MGT.\n");
    }
#endif
#endif
    
    /* Stop VPR */
    if (OSAL_SUCCESS != VPR_stopDaemon()) {
        OSAL_logMsg("Failed to stop VPR.\n");
    }

    /*
     * Clean house and exit
     */
    OSAL_memFree(_CSM_globalObj_ptr, 0);
    return;
}

static void *_CSM_vport4gCfg_ptr;

/*
 * ======== CSM_vport4gInit() ========
 *
 * This function is a public entry point into the CSM library used to
 * initialize all the private internals of CSM.
 *
 * Returns:
 *      OSAL_SUCCESS: function exits normally.
 *      OSAL_FAIL: in case of error
 *
 */
vint CSM_vport4gInit(void)
{
    /* Init EZXML */
    EZXML_init();
    _CSM_vport4gCfg_ptr = SETTINGS_cfgMemAlloc(SETTINGS_TYPE_CSM);
    if (SETTINGS_RETURN_OK != SETTINGS_getContainer(SETTINGS_TYPE_CSM,
            NULL, _CSM_vport4gCfg_ptr)) {
        OSAL_logMsg("ERROR reading CSM cfg from NV RAM\n");
        SETTINGS_memFreeDoc(SETTINGS_TYPE_CSM, _CSM_vport4gCfg_ptr);
        return (OSAL_FAIL);
    }
    CSM_dbgPrintf("\n");
#ifdef INCLUDE_4G_PLUS
    /******** vpmd allocation stage ********/
    if (OSAL_FAIL == VPMD_allocate()) {
            OSAL_logMsg("%s:%d ERROR!!! VPMD start failed.\n", __FILE__, __LINE__);
            SETTINGS_memFreeDoc(SETTINGS_TYPE_CSM, _CSM_vport4gCfg_ptr);
            return (OSAL_FAIL);
    }
    /******** vpr allocation stage ********/
    if (OSAL_FAIL == VPR_allocate()) {
            OSAL_logMsg("%s:%d ERROR!!! VPR start failed.\n", __FILE__, __LINE__);
            SETTINGS_memFreeDoc(SETTINGS_TYPE_CSM, _CSM_vport4gCfg_ptr);
            return (OSAL_FAIL);
    }
#endif
    /*
     * Start CSM
     */
    /******** allocation stage ********/
    if (OSAL_FAIL == CSM_allocate()) {
        OSAL_logMsg("%s:%d ERROR!!! allocate failed.\n", __FILE__, __LINE__);
        return (OSAL_FAIL);
    }
    if (OSAL_FAIL == _CSM_allocateManagers(_CSM_globalObj_ptr, _CSM_vport4gCfg_ptr)) {
        OSAL_logMsg("%s:%d ERROR!!! allocate manager failed.\n", __FILE__, __LINE__);
        return (OSAL_FAIL);
    }
    if (OSAL_FAIL == _CSM_allocateProtocols(_CSM_globalObj_ptr)) {
        OSAL_logMsg("%s:%d ERROR!!! allocate protocol failed.\n", __FILE__, __LINE__);
        return (OSAL_FAIL);
    }
#ifdef INCLUDE_ISIM
    ISIM_allocate();
#else
#ifdef INCLUDE_MGT
    MGT_init();
#endif

#endif

#ifdef INCLUDE_HTTP
    HTTP_allocate();
#endif

    
    /******** task start stage ********/
    if (OSAL_FAIL == _CSM_startProtocols(_CSM_globalObj_ptr)) {
        OSAL_logMsg("%s:%d ERROR!!! start protocols failed.\n", __FILE__, __LINE__);
        return (OSAL_FAIL);
    }
    if (OSAL_FAIL == _CSM_startManagers(_CSM_globalObj_ptr, _CSM_vport4gCfg_ptr)) {
        OSAL_logMsg("%s:%d ERROR!!! start manager failed.\n", __FILE__, __LINE__);
        return (OSAL_FAIL);
    }
    if (OSAL_FAIL == CSM_start(_CSM_globalObj_ptr)) {
        OSAL_logMsg("%s:%d ERROR!!! start csm failed.\n", __FILE__, __LINE__);
        return (OSAL_FAIL);
    }
#ifdef INCLUDE_4G_PLUS
    if (OSAL_FAIL == VPR_start()) {
        OSAL_logMsg("%s:%d ERROR!!! start vpr failed.\n", __FILE__, __LINE__);
        return (OSAL_FAIL);
    }
#endif
#ifdef INCLUDE_ISIM
    ISIM_start();
#else
#ifdef INCLUDE_MGT
    /* MGT_start(); */
#endif
#endif

#ifdef INCLUDE_HTTP
    HTTP_start();
#endif

    SETTINGS_memFreeDoc(SETTINGS_TYPE_CSM, _CSM_vport4gCfg_ptr);

    return (OSAL_SUCCESS);
}

/*
 * ======== CSM_vport4gShutdown() ========
 *
 * This function is a public exit point for the vport4g used to
 * de-initialize all the vport4g.
 *
 * Returns: 
 * N/A
 *  
 */
void CSM_vport4gShutdown(void)
{
    CSM_dbgPrintf("\n");

    /*
     * Shutdown CSM package managers (call, account, sup, sms, rpm, etc)
     */
    if (CSM_OK != _CSM_destroyManagers(_CSM_globalObj_ptr)) {
        OSAL_logMsg("Failed to destroy a package\n");
    }
    /*
     * Shutdown the CSM task.
     */
    CSM_destroy(_CSM_globalObj_ptr);

    /*
     * Shutdown protocol componenets SAPP, MC, GAPP, RIR, etc.
     */
    if (CSM_OK != _CSM_destroyProtocols()) {
        OSAL_logMsg("Failed to destroy Protocols\n");
    }

    /*
     * Shutdown isim thread.
     */
#ifdef INCLUDE_ISIM
    if (CSM_OK != ISIM_shutdown()) {
        OSAL_logMsg("Failed to shutdown ISIM thread\n");
    }
#else
#ifdef INCLUDE_MGT
    if (CSM_OK != MGT_shutdown()) {
        OSAL_logMsg("Failed to shutdown MGT.\n");
    }
#endif
#endif
#ifdef INCLUDE_4G_PLUS    
    /* Stop VPR */
    VPR_destory();
#endif

#ifdef INCLUDE_HTTP
    HTTP_destroy();
#endif

    EZXML_destroy();
    
    return;
}
