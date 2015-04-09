/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004-2010 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 30347 $ $Date: 2014-12-11 12:16:45 +0800 (Thu, 11 Dec 2014) $
 *
 */
#include "_supsrv.h"
#include "supsrv.h"
#include "supsrv_task.h"

#include "xcap.h"
#include "xcap_helper.h"
#include "xcap_xml_helper.h"
#include "xcap_xml_rls_parse.h"
#include "xcap_xml_reslist_parse.h"
#include "xcap_resources.h"

#if defined(PROVIDER_CMCC)
/* assuming supsrv is a singleton in the system */
SUPSRV_Mngr *_SUPSRV_mngrObj_ptr;
#endif

/*
 * ======== SUPSRV_allocate() ========
 *
 * Public routine for allocating the supsrv module resource
 *
 * Returns:
 *      OSAL_SUCCESS: function exits normally.
 *      OSAL_FAIL: in case of error
 */
vint SUPSRV_allocate(
    SUPSRV_Mngr  *supsrvMngr_ptr,
    pfonResultCB  funcCB)
{
    SUPSRV_XcapObj    *supSrv_ptr;

    if (supsrvMngr_ptr == NULL) {
        return (SUPSRV_ERR);
    }

    supsrvMngr_ptr->onResultCB = funcCB;
    supSrv_ptr = &supsrvMngr_ptr->supSrvXcap;
    SUPSRV_initTrans(supSrv_ptr);

    OSAL_netAddrClear(&supSrv_ptr->infcAddress);

#if defined(PROVIDER_CMCC)
    _SUPSRV_mngrObj_ptr = supsrvMngr_ptr;

    /* default config for CW setting */
    _SUPSRV_mngrObj_ptr->localSettings.cwSetting = SUPSRV_RES_ENABLE;
#endif

    return (OSAL_SUCCESS);
}

/*
 * ======== SUPSRV_start() ========
 *
 * Public routine for starting the supsrv module tasks/actions
 *
 * Returns:
 *      OSAL_SUCCESS: function exits normally.
 *      OSAL_FAIL: in case of error
 */
vint SUPSRV_start(
     SUPSRV_Mngr *supsrvMngr_ptr)
{
    if (NULL == (supsrvMngr_ptr->tId = OSAL_taskCreate(
            SUPSRV_XCAP_TASK_NAME, OSAL_TASK_PRIO_NRT,
            SUPSRV_XCAP_TASK_SIZE,
            (void *)SUPSRV_EventsTask, supsrvMngr_ptr))){
        return(OSAL_FAIL);
    }
    return (OSAL_SUCCESS);
}

/*
 * ======== _SUPSRV_deallocate() ========
 *
 * Internal routine for free up the supsrv module resources
 *
 * Returns:
 *      OSAL_SUCCESS: function exits normally.
 *      OSAL_FAIL: in case of error
 */
vint _SUPSRV_deallocate(
     SUPSRV_Mngr *supSrvMngr_ptr)
{
    return (OSAL_SUCCESS);
}

/*
 * ======== _SUPSRV_stop() ========
 *
 * Internal routine for stoping the supsrv module task
 *
 * Returns:
 *      OSAL_SUCCESS: function exits normally.
 *      OSAL_FAIL: in case of error
 */
vint _SUPSRV_stop(
     SUPSRV_Mngr *supSrvMngr_ptr)
{
    _SUPSRV_dbgPrintf("%s:%d\n", __FUNCTION__, __LINE__);
    if (supSrvMngr_ptr->tId) {
        _SUPSRV_dbgPrintf("%s:%d OSAL_taskDelete %p\n", __FUNCTION__, __LINE__, supSrvMngr_ptr->tId);
        OSAL_taskDelete(supSrvMngr_ptr->tId);
    }

    return (OSAL_SUCCESS);
}

/*
 * ======== SUPSRV_destroy() ========
 *
 * Public routine for shutting down the supsrv package.
 *
 * Returns:
 *      OSAL_SUCCESS: function exits normally.
 *      OSAL_FAIL: in case of error
 */
vint SUPSRV_destroy(
    SUPSRV_Mngr *supSrvMngr_ptr)
{
    _SUPSRV_stop(supSrvMngr_ptr);
    _SUPSRV_deallocate(supSrvMngr_ptr);

    return (OSAL_SUCCESS);
}

/*
 * ======== SUPSRV_init() ========
 *
 * 1. Initialization routine for the SUPSRV package
 * 2. The SUPSRV_Mngr should be allocated before calling SUPSRV_init
 * 3. pfonResultCB is the callback function for sending back the
 *    xcap result.
 *
 * Returns: 
 * SUPSRV_OK: function exits normally.
 * SUPSRV_ERR: in case of error
 */
vint SUPSRV_init(
        SUPSRV_Mngr  *supSrvMngr_ptr,
        pfonResultCB  funcCB)
{
    SUPSRV_XcapObj *supSrv_ptr;
    int             counter;
    
    _SUPSRV_dbgPrintf("%s:%d\n", __FUNCTION__, __LINE__);
    supSrv_ptr = &supSrvMngr_ptr->supSrvXcap;
    
    SUPSRV_allocate(supSrvMngr_ptr, funcCB);
    
    /* init my sub-modules as well */
    /* Init the XCAP module */
    if (1 != XCAP_allocate(&supSrv_ptr->xcap,
            supSrv_ptr->timeout)) {
        _SUPSRV_dbgPrintf("%s:%d Failed to init XCAP\n", __FUNCTION__, __LINE__); 
        return (SUPSRV_ERR);
    }
    XCAP_start(&supSrv_ptr->xcap);

    /* Wait for the XCAP event queue to initialize, but don't wait forever */
    counter = SUPSRV_XCAP_INIT_CUNTER;
    while (0 == supSrv_ptr->xcap.evtq && 0 < counter) {
        OSAL_taskDelay(SUPSRV_XCAP_INIT_CUNTER);
        counter--;
    }
    /* Should be started after XCAP event queue is ready  */
    SUPSRV_start(supSrvMngr_ptr);
    return (SUPSRV_OK);
}

/*
 * ======== SUPSRV_shutdown() ========
 *
 * Main entry point into the SupSrv Package.
 *
 * Returns: 
 *      SUPSRV_OK: function exits normally.
 *      SUPSRV_ERR: in case of error
 */
vint SUPSRV_shutdown(
    SUPSRV_Mngr *supSrvMngr_ptr) 
{
    SUPSRV_XcapObj *supSrv_ptr;

    _SUPSRV_dbgPrintf("%s:%d\n", __FUNCTION__, __LINE__);
    supSrv_ptr = &supSrvMngr_ptr->supSrvXcap;
    
    SUPSRV_destroy(supSrvMngr_ptr);

    /* destroy my sub-modules as well */
    XCAP_destroy(&supSrv_ptr->xcap);

    return (SUPSRV_OK);
}


/*
 * ======== SUPSRV_initTrans()======== 
 * This function set transcation data =0 as initial
 *
 * Return:    
 *   None
 */
void SUPSRV_initTrans(
    SUPSRV_XcapObj *xcap_ptr)
{
    vint index;

    for (index = 0 ; index < SUPSRV_MAX_TRANSACTIONS ; index++) {
        xcap_ptr->trans[index].cmdCnt = 0;
    }
    return;
}


/*
 * ======== SUPSRV_queryOip()========
 * This function will send query event to XCAP for getting OIP status
 *
 * Return:
 *  SUPSRV_OK:  Operation executed successfully
 *  SUPSRV_ERR: Operation executed failed
 */
vint SUPSRV_queryOip(
    SUPSRV_XcapObj *xcap_ptr)
{
    /* OIR and OIP are the same doc */
    if (SUPSRV_OK != _SUPSRV_fetchDocument(xcap_ptr, SUPSRV_CMD_GET_OIP,
                XCAP_SIMSERVS_AUID, XCAP_USERS, XCAP_SIMSERVS_OIR_DOC)) {
        /* The command to fetch the document failed */
        return (SUPSRV_ERR);
    }
    return (SUPSRV_OK);
}

/*
 * ======== SUPSRV_activateOip()========
 * This function will active OIP status with _SUPSRV_updateOip()
 *
 * Return:
 *  SUPSRV_OK:  Operation executed successfully
 *  SUPSRV_ERR: Operation executed failed
 */
vint SUPSRV_activateOip(
    SUPSRV_XcapObj *xcap_ptr)
{
    if (SUPSRV_OK != _SUPSRV_updateOip(xcap_ptr, 1)) {
        /* The command to fetch the document failed */
        return (SUPSRV_ERR);
    }
    return (SUPSRV_OK);
}

/*
 * ======== SUPSRV_deactivateOip()========
 * This function will deactive OIP status with _SUPSRV_updateOip()
 *
 * Return:
 *  SUPSRV_OK:  Operation executed successfully
 *  SUPSRV_ERR: Operation executed failed
 */
vint SUPSRV_deactivateOip(
    SUPSRV_XcapObj *xcap_ptr)
{
    if (SUPSRV_OK != _SUPSRV_updateOip(xcap_ptr, 0)) {
        /* The command to fetch the document failed */
        return (SUPSRV_ERR);
    }
    return (SUPSRV_OK);
}

/*
 * ======== SUPSRV_queryOir()========
 * This function will send query event to XCAP for getting OIR status
 *
 * Return:
 *  SUPSRV_OK:  Operation executed successfully
 *  SUPSRV_ERR: Operation executed failed
 */
vint SUPSRV_queryOir(
    SUPSRV_XcapObj *xcap_ptr)
{
    if (SUPSRV_OK != _SUPSRV_fetchDocument(xcap_ptr, SUPSRV_CMD_GET_OIR, 
                XCAP_SIMSERVS_AUID, XCAP_USERS, XCAP_SIMSERVS_OIR_DOC)) {
        /* The command to fetch the document failed */
        return (SUPSRV_ERR);
    }
    return (SUPSRV_OK);
}

/*
 * ======== SUPSRV_activateOir()========
 * This function will active OIR status with _SUPSRV_updateOir()
 *
 * Return:
 *  SUPSRV_OK:  Operation executed successfully
 *  SUPSRV_ERR: Operation executed failed
 */
vint SUPSRV_activateOir(
    SUPSRV_XcapObj *xcap_ptr)
{
    if (SUPSRV_OK != _SUPSRV_updateOir(xcap_ptr, 1)) {
        /* The command to fetch the document failed */
        return (SUPSRV_ERR);
    }
    return (SUPSRV_OK);
}

/*
 * ======== SUPSRV_deactivateOir()========
 * This function will deactive OIR status with _SUPSRV_updateOir()
 *
 * Return:
 *  SUPSRV_OK:  Operation executed successfully
 *  SUPSRV_ERR: Operation executed failed
 */
vint SUPSRV_deactivateOir(
    SUPSRV_XcapObj *xcap_ptr)
{
    if (SUPSRV_OK != _SUPSRV_updateOir(xcap_ptr, 0)) {
        /* The command to fetch the document failed */
        return (SUPSRV_ERR);
    }
    return (SUPSRV_OK);
}


/*
 * ======== SUPSRV_queryTip()========
 * This function will send query event to XCAP for getting TIP status
 *
 * Return:
 *  SUPSRV_OK:  Operation executed successfully
 *  SUPSRV_ERR: Operation executed failed
 */
vint SUPSRV_queryTip(
    SUPSRV_XcapObj *xcap_ptr)
{
    if (SUPSRV_OK != _SUPSRV_fetchDocument(xcap_ptr,
            SUPSRV_CMD_GET_TIP, XCAP_SIMSERVS_AUID, 
            XCAP_USERS, XCAP_SIMSERVS_TIR_DOC)) {
        /* The command to fetch the document failed */
        return (SUPSRV_ERR);
    }
    return (SUPSRV_OK);
}

/*  ======== SUPSRV_activateTip()========
 *   This function will active TIP status with  
 *   _SUPSRV_updateTIP() 
 *
 * Return:
 *  SUPSRV_OK:  Operation executed successfully
 *  SUPSRV_ERR: Operation executed failed
 */
vint SUPSRV_activateTip(
    SUPSRV_XcapObj *xcap_ptr)
{
    if (SUPSRV_OK != _SUPSRV_updateTip(xcap_ptr, 1)) {
        /* The command to fetch the document failed */
        return (SUPSRV_ERR);
    }
    return (SUPSRV_OK);
}

/* 
 * ======== SUPSRV_deactivateTip()========
 * This function will active TIP status with _SUPSRV_updateTir()
 *
 * Return:
 *  SUPSRV_OK:  Operation executed successfully
 *  SUPSRV_ERR: Operation executed failed
 */
vint SUPSRV_deactivateTip(
    SUPSRV_XcapObj *xcap_ptr)
{
    if (SUPSRV_OK != _SUPSRV_updateTip(xcap_ptr, 0)) {
        /* The command to fetch the document failed */
        return (SUPSRV_ERR);
    }
    return (SUPSRV_OK);

}


/*
 * ======== SUPSRV_queryTir()========
 * This function will send query event to XCAP for getting TIR status
 *
 * Return:
 *  SUPSRV_OK:  Operation executed successfully
 *  SUPSRV_ERR: Operation executed failed
 */
vint SUPSRV_queryTir(
    SUPSRV_XcapObj *xcap_ptr)
{
    if (SUPSRV_OK != _SUPSRV_fetchDocument(xcap_ptr,
            SUPSRV_CMD_GET_TIR, XCAP_SIMSERVS_AUID, 
            XCAP_USERS, XCAP_SIMSERVS_TIR_DOC)) {
        /* The command to fetch the document failed */
        return (SUPSRV_ERR);
    }
    return (SUPSRV_OK);
}

/*  ======== SUPSRV_activateTir()========
 *   This function will active TIR status with  
 *   _SUPSRV_updateTIR() 
 *
 * Return:
 *  SUPSRV_OK:  Operation executed successfully
 *  SUPSRV_ERR: Operation executed failed
 *      Return:    SUPSRV_OK
 *                    SUPSRV_ERR
 */
vint SUPSRV_activateTir(
    SUPSRV_XcapObj *xcap_ptr)
{
    if (SUPSRV_OK != _SUPSRV_updateTir(xcap_ptr, 1)) {
        /* The command to fetch the document failed */
        return (SUPSRV_ERR);
    }
    return (SUPSRV_OK);
}

/* 
 * ======== SUPSRV_activateTir()========
 * This function will active TIR status with _SUPSRV_updateTir()
 *
 * Return:
 *  SUPSRV_OK:  Operation executed successfully
 *  SUPSRV_ERR: Operation executed failed
 */
vint SUPSRV_deactivateTir(
    SUPSRV_XcapObj *xcap_ptr)
{
    if (SUPSRV_OK != _SUPSRV_updateTir(xcap_ptr, 0)) {
        /* The command to fetch the document failed */
        return (SUPSRV_ERR);
    }
    return (SUPSRV_OK);

}


/*
 * ======== SUPSRV_updateCD()======== 
 * This function make uri for CD with call forward rule and send command
 * to XCAP
 *
 * Return:
 *  SUPSRV_OK:  Operation executed successfully
 *  SUPSRV_ERR: Operation executed failed
 */
vint SUPSRV_updateCd(
    SUPSRV_XcapObj *xcap_ptr,
    SUPSRV_Input   *evt_ptr)
{
    char doc[SUPSRV_SCRATCH_SZ];
    char noReplyTimer[SUPSRV_STR_SZ];
    char media[SUPSRV_STR_SZ];
    char timeRange[SUPSRV_TIME_STR_SZ];

    _SUPSRV_dbgPrintf("%s\n", __FUNCTION__);
    
    xcap_ptr->protoType = evt_ptr->mode.cfMode;
    if (SUPSRV_QUERY == evt_ptr->status.genReqStatus) { 
        return (_SUPSRV_queryCd(xcap_ptr));
    }

    if (SUPSRV_DISABLE == evt_ptr->status.genReqStatus) {
            /* Construct doc */
        switch (evt_ptr->mode.cfMode) {
            case SUPSRV_EVENT_CF_MODE_UNCONDITION:
                /* CFU */
                /* Construct doc */
                OSAL_snprintf(doc, SUPSRV_SCRATCH_SZ, 
                        SUPSRV_XCAP_CD_DEACTIVATE_DOC,"");
                break;
            case SUPSRV_EVENT_CF_MODE_BUSY:
                /* CFB */
                /* Construct doc */
                OSAL_snprintf(doc, SUPSRV_SCRATCH_SZ, 
                        SUPSRV_XCAP_CD_DEACTIVATE_DOC,"<busy/>");
                break;
            case SUPSRV_EVENT_CF_MODE_NOREPLY:
                /* CFNA, or CFNR, No Reply */
                /* Construct doc */
                OSAL_snprintf(doc, SUPSRV_SCRATCH_SZ, 
                        SUPSRV_XCAP_CD_DEACTIVATE_DOC,"<no-answer/>");
                break;
            case SUPSRV_EVENT_CF_MODE_NOTREACH:
                /* CFR, not Reachable */
                /* Construct doc */
                OSAL_snprintf(doc, SUPSRV_SCRATCH_SZ, 
                        SUPSRV_XCAP_CD_DEACTIVATE_DOC,"<not-reachable/>");
                break;
            case SUPSRV_EVENT_CF_MODE_NOTLOGIN:
                /* CFL, not Logged in */
                /* Construct doc */
                OSAL_snprintf(doc, SUPSRV_SCRATCH_SZ,
                        SUPSRV_XCAP_CD_DEACTIVATE_DOC,"<not-registered/>");
                break;
            case SUPSRV_EVENT_CF_MODE_TIME:
                /* CFT, Time range of the day */
                /* Construct doc */
                OSAL_snprintf(doc, SUPSRV_SCRATCH_SZ, 
                        SUPSRV_XCAP_CD_DEACTIVATE_DOC,"<time/>");
                break;
            default:
                /* invalid call forward value. Do nothing */
                return (SUPSRV_ERR);
        }
    }   
    else {
        /*
         * Configure default NoReplyTimer which is set
         * when "no reply" is enabled
         */
        noReplyTimer[0] = 0;
        /* construct media condition */
        if (0 == OSAL_strcmp(evt_ptr->ruleParams.mediaType, "")) {
            OSAL_strcpy(media, evt_ptr->ruleParams.mediaType);
        }
        else {
            OSAL_snprintf(media, SUPSRV_STR_SZ, SUPSRV_XCAP_MEDIA,
                    evt_ptr->ruleParams.mediaType);
        }
        switch (evt_ptr->mode.cfMode) {
            case SUPSRV_EVENT_CF_MODE_UNCONDITION:
                /* CFU */
                /* Construct doc */
                OSAL_snprintf(doc, SUPSRV_SCRATCH_SZ, 
                        SUPSRV_XCAP_CD_DOC,
                        noReplyTimer,                /* NoReplyTimer */
                        "rule1",                     /* rule id */
                        "",                          /* condition reason */
                        media,                       /* condition media */
                        evt_ptr->ruleParams.cfwNumber   /* forward to */
                        );
                break;
            case SUPSRV_EVENT_CF_MODE_BUSY:
                /* CFB */
                /* Construct doc */
                OSAL_snprintf(doc, SUPSRV_SCRATCH_SZ, 
                        SUPSRV_XCAP_CD_DOC,
                        noReplyTimer,                /* NoReplyTimer */
                        "rule2",                     /* rule id */
                        "<busy/>",                   /* condition reason */
                        media,                       /* condition media */
                        evt_ptr->ruleParams.cfwNumber   /* forward to */
                        );

                break;
            case SUPSRV_EVENT_CF_MODE_NOREPLY:
                /* set NoReplyTimer, if it is not NONE. */
                if (SUPSRV_NOREPLY_NONE != evt_ptr->ruleParams.noReplyTimer) {
                    OSAL_snprintf(noReplyTimer, SUPSRV_STR_SZ,
                            SUPSRV_XCAP_CD_NO_REPLY_TIMER,
                            evt_ptr->ruleParams.noReplyTimer);
                }
                else {
                    /* If the timer is not set, set a default value, 20 sec. */
                    OSAL_snprintf(noReplyTimer, SUPSRV_STR_SZ,
                            SUPSRV_XCAP_CD_NO_REPLY_TIMER, SUPSRV_NOREPLY_TIMER_DEFAULT);
                }
                /* CFNA, or CFNR, No Reply */
                /* Construct doc */
                OSAL_snprintf(doc, SUPSRV_SCRATCH_SZ, 
                        SUPSRV_XCAP_CD_DOC,
                        noReplyTimer,                /* NoReplyTimer */
                        "rule3",                     /* rule id */
                        "<no-answer/>",              /* condition reason */
                        media,                       /* condition media */
                        evt_ptr->ruleParams.cfwNumber   /* forward to */
                        );

                break;
            case SUPSRV_EVENT_CF_MODE_NOTREACH:
                /* CFR, not Reachable */
                /* Construct doc */
                OSAL_snprintf(doc, SUPSRV_SCRATCH_SZ, 
                        SUPSRV_XCAP_CD_DOC,
                        noReplyTimer,                /* NoReplyTimer */
                        "rule4",                     /* rule id */
                        "<not-reachable/>",          /* condition reason */
                        media,                       /* condition media */
                        evt_ptr->ruleParams.cfwNumber   /* forward to */
                        );

                break;
            case SUPSRV_EVENT_CF_MODE_NOTLOGIN:
                /* CFL, not Logged in */
                /* Construct doc */
                OSAL_snprintf(doc, SUPSRV_SCRATCH_SZ, 
                        SUPSRV_XCAP_CD_DOC,
                        noReplyTimer,                /* NoReplyTimer */
                        "rule4",                     /* rule id */
                        "<not-registered/>",         /* condition reason */
                        media,                       /* condition media */
                        evt_ptr->ruleParams.cfwNumber   /* forward to */
                        );

                break;
            case SUPSRV_EVENT_CF_MODE_TIME:
                /* CFT, time range of the day */
                /* Construct doc */
                OSAL_snprintf(timeRange, SUPSRV_TIME_STR_SZ,
                            SUPSRV_XCAP_CD_TIME_RANGE,
                            evt_ptr->ruleParams.timeRangeOfTheDay);
                OSAL_snprintf(doc, SUPSRV_SCRATCH_SZ, 
                        SUPSRV_XCAP_CD_DOC,
                        noReplyTimer,                /* NoReplyTimer */
                        "rule6",                     /* rule id */
                        timeRange,                   /* condition reason */
                        media,                       /* condition media */
                        evt_ptr->ruleParams.cfwNumber   /* forward to */
                        );

                break;
            default:
                /* invalid call forward value. Do nothing */
                return (SUPSRV_ERR);
        }
    }
    if (SUPSRV_OK != _SUPSRV_updateService(xcap_ptr, doc, 
                SUPSRV_CMD_CD_OPERATION)) {
        /* The command to fetch the document failed */
        _SUPSRV_dbgPrintf("%s: CFU set failed\n", __FUNCTION__);
        return (SUPSRV_ERR);
    }
    return (SUPSRV_OK);
}


/*
 * ======== SUPSRV_updateCbic()========
 * This function make uri for incoming call barring and send command to XCAP
 *
 * Return:
 *  SUPSRV_OK:  Operation executed successfully
 *  SUPSRV_ERR: Operation executed failed
 */
vint SUPSRV_updateCbic(
    SUPSRV_XcapObj  *xcap_ptr,
    SUPSRV_Input    *evt_ptr)
{
    char doc[SUPSRV_SCRATCH_SZ];

    xcap_ptr->protoType = evt_ptr->mode.cbMode;
    
    if (SUPSRV_QUERY == evt_ptr->status.genReqStatus) {
        return(_SUPSRV_queryCbic(xcap_ptr));
    }
    else if (SUPSRV_DISABLE == evt_ptr->status.genReqStatus) {
        /* Construct doc */
        OSAL_snprintf(doc, SUPSRV_SCRATCH_SZ, 
                SUPSRV_XCAP_CBIC_DEACTIVATE_DOC);
    }
    else if (SUPSRV_ENABLE == evt_ptr->status.genReqStatus){
        switch(evt_ptr->mode.cbMode) {
            case SUPSRV_EVENT_CB_MODE_BAIC:
                /* Construct doc */
                OSAL_snprintf(doc, SUPSRV_SCRATCH_SZ, 
                        SUPSRV_XCAP_CBIC_ACTIVATE_DOC,
                        "rule1",   /* rule id */
                        "false"
                        );
                break;
             case SUPSRV_EVENT_CB_MODE_BICR:
                /* Construct doc */
                OSAL_snprintf(doc, SUPSRV_SCRATCH_SZ, 
                        SUPSRV_XCAP_CBIC_ACTIVATE_DOC,
                        "rule1", /* rule id */
                        "true"   /*
                                  * Set internation barring condition as true,
                                  * but need to check if the configuration is 
                                  * correct
                                  */
                        );
                break;
            default:
                /* XXX Other cb mode add here */
                break;
        }
    }
    else {
        return (SUPSRV_ERR);
    }
    if (SUPSRV_OK != _SUPSRV_updateService(xcap_ptr, doc, 
                SUPSRV_CMD_CBIC_OPERATION)) {
        /* The command to fetch the document failed */
        _SUPSRV_dbgPrintf("%s: CBIC set failed\n", __FUNCTION__);
        return (SUPSRV_ERR);
    }   
    return (SUPSRV_OK);
}


/*
 * ======== SUPSRV_updateCboc()========
 * This function make uri for outgoing call barring and send command to XCAP
 *
 * Return:
 *  SUPSRV_OK:  Operation executed successfully
 *  SUPSRV_ERR: Operation executed failed
 */
vint SUPSRV_updateCboc(
    SUPSRV_XcapObj   *xcap_ptr,
    SUPSRV_Input     *evt_ptr)
{
    char doc[SUPSRV_SCRATCH_SZ];
    
    xcap_ptr->protoType = evt_ptr->mode.cbMode;
    if (SUPSRV_QUERY == evt_ptr->status.genReqStatus) {
        return(_SUPSRV_queryCboc(xcap_ptr));
    }
    else if (SUPSRV_DISABLE == evt_ptr->status.genReqStatus) {
        /* Construct doc */
        OSAL_snprintf(doc, SUPSRV_SCRATCH_SZ, 
                SUPSRV_XCAP_CBOC_DEACTIVATE_DOC);
    }
    else if (SUPSRV_ENABLE == evt_ptr->status.genReqStatus){
        switch(evt_ptr->mode.cbMode) {
            case SUPSRV_EVENT_CB_MODE_BAOC:
                /* Construct doc */
                OSAL_snprintf(doc, SUPSRV_SCRATCH_SZ,
                        SUPSRV_XCAP_CBOG_ACTIVATE_DOC,
                        "rule5",  /* rule id */
                        "<serv-cap-international>false"
                        "</serv-cap-international>\n");
                break;
            case SUPSRV_EVENT_CB_MODE_BOIC:
                /* Construct doc */
                /*
                 * Set Roaming barring condition as true, but need to check
                 * if the configuration is correct.
                 */
                OSAL_snprintf(doc, SUPSRV_SCRATCH_SZ, 
                        SUPSRV_XCAP_CBOG_ACTIVATE_DOC,
                        "rule5",  /* rule id */
                        "<serv-cap-international>true"
                        "</serv-cap-international>\n");
                break;
            case SUPSRV_EVENT_CB_MODE_BOIC_EXHC:
                /* Construct doc */
                OSAL_snprintf(doc, SUPSRV_SCRATCH_SZ,
                        SUPSRV_XCAP_CBOG_ACTIVATE_DOC,
                        "rule5",  /* rule id */
                        "<serv-cap-international-exHC>true"
                        "</serv-cap-international-exHC>\n");
                break;
            default:
                break;
        }
    }
    else {
        return (SUPSRV_ERR);
    }
    if (SUPSRV_OK != _SUPSRV_updateService(xcap_ptr, doc,
                SUPSRV_CMD_CBOG_OPERATION)) {
        /* The command to fetch the document failed */
        _SUPSRV_dbgPrintf("%s: CBOC set failed\n", __FUNCTION__);
        return (SUPSRV_ERR);
    }
    return (SUPSRV_OK);
}


/*
 * ======== SUPSRV_updateCw()======== 
 * This function make uri for call waiting and send command to XCAP
 *
 * Return:
 *  SUPSRV_OK:  Operation executed successfully
 *  SUPSRV_ERR: Operation executed failed
 */

vint SUPSRV_updateCw(
    SUPSRV_XcapObj  *xcap_ptr,
    SUPSRV_Input    *evt_ptr)
{
    char doc[SUPSRV_SCRATCH_SZ];

    if (SUPSRV_QUERY == evt_ptr->status.genReqStatus) {
        return(_SUPSRV_queryCw(xcap_ptr));
    }
    else if (SUPSRV_DISABLE == evt_ptr->status.genReqStatus) {
        /* Construct doc */
        OSAL_snprintf(doc, SUPSRV_SCRATCH_SZ, 
            SUPSRV_XCAP_CW_DOC,"false");
    }
    else if (SUPSRV_ENABLE == evt_ptr->status.genReqStatus) {
        OSAL_snprintf(doc, SUPSRV_SCRATCH_SZ, 
                SUPSRV_XCAP_CW_DOC,"true");
    }
    else {
        return (SUPSRV_ERR);
    }

    if (SUPSRV_OK != _SUPSRV_updateService(xcap_ptr, doc, 
            SUPSRV_CMD_CW_OPERATION)) {
        /* The command to fetch the document failed */
        _SUPSRV_dbgPrintf("%s: CBOC set failed\n", __FUNCTION__);
        return (SUPSRV_ERR);
    }
    return (SUPSRV_OK);
}


/*
 * ======== SUPSRV_xcapEventProcess()======== 
 * This function is to parse the XCAP query result
 *
 * Returns:
 *  SUPSRV_OK:  Result parsed successfully
 *  SUPSRV_ERR: Result parse failed.
 */
vint SUPSRV_xcapEventProcess(
    SUPSRV_Mngr     *supSrvMngr_ptr,
    XCAP_Evt        *xcapEvt_ptr,
    SUPSRV_Output   *out_ptr)
{
    vint              code;
    vint              owner;
    SUPSRV_CmdType cmdType;

    owner = _SUPSRV_getXcapEvtOwner(supSrvMngr_ptr);

    out_ptr->errorCode = SUPSRV_ERROR_NONE;
    out_ptr->cmdType = supSrvMngr_ptr->supSrvXcap.trans[owner].cmdType;
    cmdType = out_ptr->cmdType;
    /* Just assign back to any mode */
    out_ptr->mode.cfMode = supSrvMngr_ptr->supSrvXcap.protoType;
    
    if (0 != xcapEvt_ptr->error){
        /* +CME ERROR: <err> */
        switch(xcapEvt_ptr->error){
            case XCAP_EVT_ERR_NET:
                /*err =31 :network timeout */
                out_ptr->errorCode = SUPSRV_ERROR_NET_TIMEOUT;
                break;
            case XCAP_EVT_ERR_HTTP:
                /*err =30 : no network service */
                out_ptr->errorCode = SUPSRV_ERROR_NO_NET_SERVICE;
                break;
            default:
                /* err=100 :unknow */
                out_ptr->errorCode = SUPSRV_ERROR_UNKNOWN;
                break;                  
        }
        out_ptr->reason = SUPSRV_OUTPUT_REASON_ERROR;
    }
    else{
        if (NULL == xcapEvt_ptr->hdr_ptr) {
            out_ptr->errorCode = SUPSRV_ERROR_UNKNOWN;
            out_ptr->reason = SUPSRV_OUTPUT_REASON_ERROR;
        }
        else {
            code = _SUPSRV_getXcapResponseCode(xcapEvt_ptr);
            /* 200 OK */
            if (200 == code){
                if (SUPSRV_CMD_OIR_OPERATION == cmdType ||
                        SUPSRV_CMD_OIP_OPERATION == cmdType ||
                        SUPSRV_CMD_TIP_OPERATION == cmdType ||
                        SUPSRV_CMD_TIR_OPERATION == cmdType ||
                        SUPSRV_CMD_CW_OPERATION == cmdType ||
                        SUPSRV_CMD_CD_OPERATION == cmdType ||
                        SUPSRV_CMD_CBIC_OPERATION == cmdType ||
                        SUPSRV_CMD_CBICR_OPERATION == cmdType ||
                        SUPSRV_CMD_CBOG_OPERATION == cmdType ||
                        SUPSRV_CMD_CBOIC_OPERATION == cmdType) {
                    /* means sent Supplementary setting OK  */
                    out_ptr->reason = SUPSRV_OUTPUT_REASON_OK;
                }
                else { 
                    /* Parser query result*/
                    out_ptr->reason = SUPSRV_OUTPUT_REASON_QUERY_RESULT;
                    if (SUPSRV_OK != _SUPSRV_parseSupSrvQueryResult(
                            xcapEvt_ptr, out_ptr, cmdType)){
                        out_ptr->reason = SUPSRV_OUTPUT_REASON_ERROR;
                    }
                }     
            }
            else {
                /* Put error code to reasonDesc */
                out_ptr->errorCode = SUPSRV_ERROR_HTTP;
                OSAL_snprintf(out_ptr->reasonDesc, sizeof(out_ptr->reasonDesc), "%d", code);
                out_ptr->reason = SUPSRV_OUTPUT_REASON_ERROR;
            }
        }
    }
    supSrvMngr_ptr->supSrvXcap.trans[owner].cmdCnt = 0;
    return (SUPSRV_OK);
}

/*
 * ======== SUPSRV_processIpChange()======== 
 * This function is to store the IP address/radio interface used for SUPSRV access
 *
 * Returns:
 *  SUPSRV_OK:  processed successfully
 *  SUPSRV_ERR: processed failed
 */
vint SUPSRV_processIpChange(
    SUPSRV_Mngr     *supSrvMngr_ptr,
    OSAL_NetAddress *ipAddr_ptr)
{
    OSAL_netAddrCpy(&supSrvMngr_ptr->supSrvXcap.infcAddress, ipAddr_ptr);

    return (SUPSRV_OK);
}


#if defined(PROVIDER_CMCC)

/*
 * ======== SUPSRV_getLocalCwSetting()======== 
 * This function is to query local CW setting with immediate result.
 *
 * Returns:
 *  SUPSRV_GenResStatus:  The CW setting
 */
SUPSRV_GenResStatus SUPSRV_getLocalCwSetting(void)
{
    return _SUPSRV_mngrObj_ptr->localSettings.cwSetting;
}

/*
 * ======== SUPSRV_setLocalCwSetting()======== 
 * This function is to set local CW setting and is immediately effective.
 *
 * Returns:
 *  SUPSRV_GenResStatus:  The CW setting
 */
vint SUPSRV_setLocalCwSetting(
    SUPSRV_GenResStatus status)
{
    _SUPSRV_mngrObj_ptr->localSettings.cwSetting = status;

    return (SUPSRV_OK);
}
#endif
