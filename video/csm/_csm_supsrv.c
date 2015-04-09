/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004-2012 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 30357 $ $Date: 2014-12-11 16:11:02 +0800 (Thu, 11 Dec 2014) $
 *
 */

#include "supsrv.h"
#include "supsrv_task.h"
#include "csm_event.h"
#include "_csm_response.h"
#include "_csm_print.h"
#include "_csm_supsrv.h"
#include "_csm.h"

#ifdef INCLUDE_GBA
#include <gba.h>
#endif

static SUPSRV_Mngr  *_CSM_supSrvMngr_ptr;

vint _CSM_writeSupSrvCsmEvent(
    void         *arg_ptr,
    SUPSRV_Evt   *evt_ptr);

/*
 * ======== _CSM_supSrvConvertSupsrvToCsm() ========
 *
 * Private function to covert SUPSRV_Output to CSM_OutputSupSrv
 *
 * Returns: 
 *   None.
 */
static void _CSM_supSrvConvertSupsrvToCsm(
    SUPSRV_Output    *supEvt_ptr,
    CSM_OutputSupSrv *csmEvt_ptr)
{
    /* XXX 
     * Currently execept the reason the rest structure of
     * SUPSRV_Output and CSM_OutputSupSrv are the same.
     * Let's just copy it for now.
     */
    *csmEvt_ptr = *((CSM_OutputSupSrv *)supEvt_ptr);
    /* Convert reason */
    if (SUPSRV_OUTPUT_REASON_OK == supEvt_ptr->reason) {
        csmEvt_ptr->reason = CSM_OUTPUT_REASON_OK;
    }
    else if (SUPSRV_OUTPUT_REASON_ERROR == supEvt_ptr->reason) {
        csmEvt_ptr->reason = CSM_OUTPUT_REASON_ERROR;
    }
    else if (SUPSRV_OUTPUT_REASON_QUERY_RESULT == supEvt_ptr->reason) {
        csmEvt_ptr->reason = CSM_OUTPUT_REASON_SUPSRV_QUERY_RESULT;
    }
}

/*
 * ======== _CSM_supSrvConvertCsmToSupsrv() ========
 *
 * Private function to covert CSM_InputSupSrv to SUPSRV_Input.
 *
 * Returns: 
 *   None.
 */
static void _CSM_supSrvConvertCsmToSupsrv(
    CSM_InputSupSrv  *csmEvt_ptr,
    SUPSRV_Input     *supEvt_ptr)
{
    /* XXX 
     * Currently the structure of SUPSRV_Output and CSM_OutputSupSrv are the same.
     * Let's just copy it for now.
     */
    *supEvt_ptr = *((SUPSRV_Input *)csmEvt_ptr);
}

#if defined(PROVIDER_CMCC)
/*
 * ======== _CSM_supsrvUpdateLocalCw()======== 
 * This implemented local supsrv setting for CW
 *
 * Return:
 *  CSM_OK:  Operation executed successfully
 *  CSM_ERR: Operation executed failed
 */
vint _CSM_supsrvUpdateLocalCw(
    SUPSRV_XcapObj  *xcap_ptr,
    SUPSRV_Input    *evt_ptr,
    CSM_OutputEvent *csmOutput_ptr)
{
    csmOutput_ptr->type = CSM_EVENT_TYPE_SUPSRV;
    csmOutput_ptr->evt.supSrv.reasonDesc[0] = '\0';
    csmOutput_ptr->evt.supSrv.errorCode = CSM_SUPSRV_ERROR_NONE;

    switch (evt_ptr->status.genReqStatus) {
        case SUPSRV_QUERY:
            /* get local CW setting */
            csmOutput_ptr->evt.supSrv.queryEnb.genResStatus =
                SUPSRV_getLocalCwSetting();
            /* generate query result */
            csmOutput_ptr->evt.supSrv.reason = CSM_OUTPUT_REASON_SUPSRV_QUERY_RESULT;
            csmOutput_ptr->evt.supSrv.cmdType = SUPSRV_CMD_GET_CW;
            csmOutput_ptr->evt.supSrv.prov.genProv = SUPSRV_PROVISIONED;
            break;
        case SUPSRV_DISABLE:
        case SUPSRV_ENABLE:
            /* set local CW setting */
            SUPSRV_setLocalCwSetting(evt_ptr->status.genReqStatus);
            /* generate ok response */
            csmOutput_ptr->evt.supSrv.reason = CSM_OUTPUT_REASON_OK;
            csmOutput_ptr->evt.supSrv.cmdType = SUPSRV_CMD_CW_OPERATION;
            break;
        default:
            return (CSM_ERR);
    }

    CSM_sendOutputEvent(csmOutput_ptr);

    return (CSM_OK);
}
#endif

/*
 * ======== CSM_supSrvInit() ========
 *
 * Initialization routine for the CSM SUPSRV manager package
 *
 * Returns: 
 * CSM_OK: function exits normally.
 * CSM_ERR: in case of error
 */
vint CSM_supSrvInit(
    SUPSRV_Mngr  *supSrvMngr_ptr,
    OSAL_MsgQId   qId)
{
    CSM_dbgPrintf("\n");

    /* Cache the manager pointer for use in this package */
    _CSM_supSrvMngr_ptr = supSrvMngr_ptr;
    supSrvMngr_ptr->arg_ptr = qId;

    /* init supsrv and call back function for SUPSRV_EventsTask. */
    if (SUPSRV_OK != SUPSRV_init(supSrvMngr_ptr, _CSM_writeSupSrvCsmEvent)) {
        return (CSM_ERR);
    }

    return (CSM_OK);
}

/*
 * ======== CSM_supSrvProcessEvent() ========
 *
 * Main entry point into the SUPSRV Package.
 *
 * Returns: 
 * CSM_OK: function exits normally.
 * CSM_ERR: in case of error
 */
vint CSM_supSrvProcessEvent(
    SUPSRV_Mngr     *supSrvMngr_ptr,
    CSM_InputSupSrv *csmSupSrvEvt_ptr,
    CSM_OutputEvent *csmOutput_ptr)
{
    SUPSRV_Input *supSrvEvt_ptr;

    supSrvEvt_ptr = &supSrvMngr_ptr->inEvt;
    /* Convert CSM_InputSupSrv to SUPSRV_Input. */
    _CSM_supSrvConvertCsmToSupsrv(csmSupSrvEvt_ptr, supSrvEvt_ptr);

    switch (supSrvEvt_ptr->reason) {
        case SUPSRV_EVENT_REASON_AT_CMD_OIP:
            if (SUPSRV_ENABLE == supSrvEvt_ptr->status.genReqStatus) {
                if (CSM_OK != SUPSRV_activateOip(
                        &supSrvMngr_ptr->supSrvXcap)) {
                    CSM_sendSupSrvError(SUPSRV_ERROR_UNKNOWN,
                            "Faild to Send command", csmOutput_ptr);
                }
            }
            else if (SUPSRV_DISABLE == supSrvEvt_ptr->status.genReqStatus) {
                if (CSM_OK != SUPSRV_deactivateOip(
                        &supSrvMngr_ptr->supSrvXcap)) {
                    CSM_sendSupSrvError(SUPSRV_ERROR_UNKNOWN,
                            "Faild to Send command", csmOutput_ptr);
                }
            }
            else if (SUPSRV_QUERY == supSrvEvt_ptr->status.genReqStatus) {
                if (CSM_OK != SUPSRV_queryOip(&supSrvMngr_ptr->supSrvXcap)) {
                    CSM_sendSupSrvError(SUPSRV_ERROR_UNKNOWN,
                            "Faild to Send command", csmOutput_ptr);
                }
            }
            else {
                CSM_sendSupSrvError(SUPSRV_ERROR_UNKNOWN,
                        "Set falied status value", csmOutput_ptr);
            }
            break;
        case SUPSRV_EVENT_REASON_AT_CMD_OIR:
            if ((SUPSRV_OIR_DEFAULT ==
                    supSrvEvt_ptr->status.oirReqStatus) ||
                    (SUPSRV_OIR_INVOCATION ==
                    supSrvEvt_ptr->status.oirReqStatus)) {
                if (CSM_OK != SUPSRV_activateOir(
                        &supSrvMngr_ptr->supSrvXcap)) {
                    CSM_sendSupSrvError(SUPSRV_ERROR_UNKNOWN,
                            "Faild to Send command", csmOutput_ptr);
                }
            }
            else if (SUPSRV_OIR_SUPPRESSION ==
                    supSrvEvt_ptr->status.oirReqStatus) {
                if (CSM_OK != SUPSRV_deactivateOir(
                        &supSrvMngr_ptr->supSrvXcap)) {
                    CSM_sendSupSrvError(SUPSRV_ERROR_UNKNOWN,
                            "Faild to Send command", csmOutput_ptr);
                }
            }
            else if (SUPSRV_OIR_QUERY == supSrvEvt_ptr->status.oirReqStatus) {
                if (CSM_OK != SUPSRV_queryOir(&supSrvMngr_ptr->supSrvXcap)) {
                    CSM_sendSupSrvError(SUPSRV_ERROR_UNKNOWN,
                            "Faild to Send command", csmOutput_ptr);
                }
            }
            else {
                CSM_sendSupSrvError(SUPSRV_ERROR_UNKNOWN,
                        "Set falied status value", csmOutput_ptr);
            }
            break;
        case SUPSRV_EVENT_REASON_AT_CMD_TIP:
            if (SUPSRV_ENABLE == supSrvEvt_ptr->status.genReqStatus) {
                if (CSM_OK != SUPSRV_activateTip(&supSrvMngr_ptr->supSrvXcap)) {
                    CSM_sendSupSrvError(SUPSRV_ERROR_UNKNOWN,
                            "Faild to Send command", csmOutput_ptr);
                }
            }
            else if (SUPSRV_DISABLE== supSrvEvt_ptr->status.genReqStatus) {
                if (CSM_OK !=
                        SUPSRV_deactivateTip(&supSrvMngr_ptr->supSrvXcap)) {
                    CSM_sendSupSrvError(SUPSRV_ERROR_UNKNOWN,
                            "Faild to Send command", csmOutput_ptr);
                }
            }
            else if (SUPSRV_QUERY == supSrvEvt_ptr->status.genReqStatus) {
                if (CSM_OK != SUPSRV_queryTip(&supSrvMngr_ptr->supSrvXcap)) {
                    CSM_sendSupSrvError(SUPSRV_ERROR_UNKNOWN,
                            "Faild to Send command", csmOutput_ptr);
                }
            }
            else {
                CSM_sendSupSrvError(SUPSRV_ERROR_UNKNOWN,
                            "Set falied status value", csmOutput_ptr);
            }
            break;
        case SUPSRV_EVENT_REASON_AT_CMD_TIR:
            if (SUPSRV_ENABLE == supSrvEvt_ptr->status.genReqStatus) {
                if (CSM_OK != SUPSRV_activateTir(&supSrvMngr_ptr->supSrvXcap)) {
                    CSM_sendSupSrvError(SUPSRV_ERROR_UNKNOWN,
                            "Faild to Send command", csmOutput_ptr);
                }
            }
            else if (SUPSRV_DISABLE== supSrvEvt_ptr->status.genReqStatus) {
                if (CSM_OK !=
                        SUPSRV_deactivateTir(&supSrvMngr_ptr->supSrvXcap)) {
                    CSM_sendSupSrvError(SUPSRV_ERROR_UNKNOWN,
                            "Faild to Send command", csmOutput_ptr);
                }
            }
            else if (SUPSRV_QUERY == supSrvEvt_ptr->status.genReqStatus) {
                if (CSM_OK != SUPSRV_queryTir(&supSrvMngr_ptr->supSrvXcap)) {
                    CSM_sendSupSrvError(SUPSRV_ERROR_UNKNOWN,
                            "Faild to Send command", csmOutput_ptr);
                }
            }
            else {
                CSM_sendSupSrvError(SUPSRV_ERROR_UNKNOWN,
                            "Set falied status value", csmOutput_ptr);
            }
            break;
        case SUPSRV_EVENT_REASON_AT_CMD_CW:
#if defined(PROVIDER_CMCC)
            _CSM_supsrvUpdateLocalCw(&supSrvMngr_ptr->supSrvXcap,
                    supSrvEvt_ptr, csmOutput_ptr);
#else
            if (CSM_OK != SUPSRV_updateCw(&supSrvMngr_ptr->supSrvXcap,
                    supSrvEvt_ptr)) {
                CSM_sendSupSrvError(SUPSRV_ERROR_UNKNOWN,
                            "Faild to Send command", csmOutput_ptr);
            }
#endif
            break;
        case SUPSRV_EVENT_REASON_AT_CMD_CF:
            if (CSM_OK != SUPSRV_updateCd(&supSrvMngr_ptr->supSrvXcap,
                    supSrvEvt_ptr)) {
                CSM_sendSupSrvError(SUPSRV_ERROR_UNKNOWN,
                        "Faild to Send command", csmOutput_ptr);
            }
            break;
        case SUPSRV_EVENT_REASON_AT_CMD_CB:
            if ((SUPSRV_EVENT_CB_MODE_BAOC == supSrvEvt_ptr->mode.cbMode) ||
                    (SUPSRV_EVENT_CB_MODE_BOIC ==
                    supSrvEvt_ptr->mode.cbMode) ||
                    (SUPSRV_EVENT_CB_MODE_BOIC_EXHC ==
                    supSrvEvt_ptr->mode.cbMode)) {
                if (CSM_OK != SUPSRV_updateCboc(&supSrvMngr_ptr->supSrvXcap,
                        supSrvEvt_ptr)) {
                    CSM_sendSupSrvError(SUPSRV_ERROR_UNKNOWN,
                            "Faild to Send command", csmOutput_ptr);
                }
            }
            else {
                if (CSM_OK != SUPSRV_updateCbic(&supSrvMngr_ptr->supSrvXcap,
                        supSrvEvt_ptr)) {
                    CSM_sendSupSrvError(SUPSRV_ERROR_UNKNOWN,
                            "Faild to Send command", csmOutput_ptr);
                }
            }
            break;
        default:
            return (CSM_ERR);
    }
    return (CSM_OK);
}

/*
 * ======== CSM_xcapProcessEvent() ========
 *
 * Main entry point into the XCAP Package.
 *
 * Returns: 
 * CSM_OK: function exits normally.
 * CSM_ERR: in case of error
 */
vint CSM_xcapProcessEvent(
    SUPSRV_Mngr        *supSrvMngr_ptr,
    SUPSRV_XcapEvt     *xcapEvt_ptr,
    CSM_OutputEvent    *csmOutput_ptr)
{
    SUPSRV_Output *supSrvEvt_ptr;

    supSrvEvt_ptr = &supSrvMngr_ptr->outEvt;

    switch (xcapEvt_ptr->reason) {
        case SUPSRV_REASON_XCAP_EVT:
            /* Initial the output event */
            OSAL_memSet(csmOutput_ptr, 0, sizeof(CSM_OutputEvent));
            csmOutput_ptr->type = CSM_EVENT_TYPE_SUPSRV;
            SUPSRV_xcapEventProcess(supSrvMngr_ptr,
                    &xcapEvt_ptr->evt, supSrvEvt_ptr);
            /* Convert to CSM_OutputSupSrv event */
            _CSM_supSrvConvertSupsrvToCsm(supSrvEvt_ptr, &csmOutput_ptr->evt.supSrv);
            CSM_sendOutputEvent(csmOutput_ptr);
            break;
        default:
            return (CSM_ERR);
    }
    return (CSM_OK);
}
/*
 * ======== CSM_supSrvShutdown() ========
 *
 * Main entry point into the SupSrv Package.
 *
 * Returns: 
 *      CSM_OK: function exits normally.
 *      CSM_ERR: in case of error
 */
vint CSM_supSrvShutdown(
    SUPSRV_Mngr *supSrvMngr_ptr) 
{
    CSM_dbgPrintf("%s:%d\n", __FUNCTION__, __LINE__);

    SUPSRV_shutdown(supSrvMngr_ptr);
    _CSM_supSrvMngr_ptr = NULL;

    return (CSM_OK);
}

/*
 * =========_CSM_writeSupSrvCsmEvent()=========
 * This function write CSM event for supsrv.
 *
 * Return
 *   CSM_OK: Write event successfully
 *   CSM_ERR: Write event failed
 */
vint _CSM_writeSupSrvCsmEvent(
    void        *arg_ptr, 
    SUPSRV_Evt  *evt_ptr)
{
    CSM_PrivateInputEvt      csmEvt;
    OSAL_MsgQId              supSrvCsmEvtQId;

    supSrvCsmEvtQId = arg_ptr;
    if (SUPSRV_EVENT_TYPE == evt_ptr->type) {
        /* CSM_PRIVATE_EVENT_TYPE_SUPSRV */
        csmEvt.type = CSM_EVENT_TYPE_SUPSRV;
        OSAL_memCpy(&csmEvt.evt.xcap, &evt_ptr->xcap, sizeof(SUPSRV_XcapEvt));
        /* send event */
        if (OSAL_SUCCESS != OSAL_msgQSend(supSrvCsmEvtQId, (void *)&csmEvt,
                sizeof(CSM_PrivateInputEvt), OSAL_NO_WAIT, NULL)) {
            CSM_dbgPrintf("%s: Failed to send event.", __FUNCTION__);
            return (CSM_ERR);
        }
        return (CSM_OK);
    }
    return (CSM_ERR);
}

/*
 * ======== CSM_supSrvOnRadioChange() ========
 *
 * Callback from RPM to notify the CSM supsrv that 
 *     a change to the supsrv radio has occurred
 *
 * Returns: 
 *      none
 */
void CSM_supSrvOnRadioChange(
    RPM_RadioInterface *radioInfc_ptr,
    RPM_RadioType       radioType)
{
    OSAL_NetAddress ipAddr;

    OSAL_netAddrHton(&ipAddr, &radioInfc_ptr->ipAddr);
    /* radioType is assumed RPM_RADIO_TYPE_LTE_SS at this moment and ignored here */

#ifdef CSM_DEBUG
    {
        char addrStr[48];
        OSAL_netAddressToString(addrStr, &ipAddr);
        CSM_dbgPrintf("Set SupSrv ip addr=%s, type=%d\n",
                addrStr, radioInfc_ptr->radioType);
    }
#endif

    /* 
     * the radio infc info need to pass down to gaa/gba/_xcap_xact 
     * where HTTP_setup() is called and bind to the ip.
     */

    /* 
     * for _xcap_xact and _xcap_xact_gaa, 
     * which both will setup http when start a supsrv/xcap transaction 
     */
    SUPSRV_processIpChange(_CSM_supSrvMngr_ptr, &ipAddr);
    
#ifdef INCLUDE_GBA
    /* for gba which could bootstrape at any time or triggered by xcap transaction */
    GBA_processIpChange(&ipAddr);
#endif

}

/*
 * ======== CSM_supsrvProvisioning() ========
 *
 * provisioning supsrv server for digest auth
 *
 * Returns:
 *      none
 */
void CSM_supSrvProvisioning(
    char *rootUri,
    char *authName,
    char *authSecret)
{
    if (NULL != rootUri) {
        OSAL_strncpy(_CSM_supSrvMngr_ptr->supSrvXcap.server,
                rootUri, sizeof(_CSM_supSrvMngr_ptr->supSrvXcap.server));
    }

    /* Get user */
    if (NULL != authName) {
        OSAL_strncpy(_CSM_supSrvMngr_ptr->supSrvXcap.username,
                authName, sizeof(_CSM_supSrvMngr_ptr->supSrvXcap.username));
    }

    /* Get password */
    if (NULL != authSecret) {
        OSAL_strncpy(_CSM_supSrvMngr_ptr->supSrvXcap.password,
                authSecret, sizeof(_CSM_supSrvMngr_ptr->supSrvXcap.password));
    }
}

/*
 * ======== CSM_supSrvSetXcapUri() ========
 *
 * Update URI. It's for XUI features.
 * Note:
 *     1. If REG OK, uri should set to the default(1st) public user identity
 *        received in P-Associated-URI header. Mostly, it's the same as IMPU.
 *     2. If REG FAIL, uri sets to the default public user identity
 *        received during the last successful registration.
 */
void CSM_supSrvSetXcapUri(
    char *uri)
{
    if (NULL != uri) {
        OSAL_strncpy(_CSM_supSrvMngr_ptr->supSrvXcap.uri,
                uri, sizeof(_CSM_supSrvMngr_ptr->supSrvXcap.uri));
    }
}
/*
 * ======== CSM_supSrvSetImpu() ========
 *
 * Update IMPU. For XCAP'cmd *x3gpp_ptr.
 */
void CSM_supSrvSetImpu(
    char *impu)
{
    if (NULL != impu) {
        OSAL_strncpy(_CSM_supSrvMngr_ptr->supSrvXcap.impu,
                impu, sizeof(_CSM_supSrvMngr_ptr->supSrvXcap.impu));
    }
}
