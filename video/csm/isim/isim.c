/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004-2010 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 29388 $ $Date: 2014-10-17 10:09:39 +0800 (Fri, 17 Oct 2014) $
 *
 */

#include <osal.h>
#include <csm_event.h>
#include <settings.h>
#include <milenage.h>
#include "isim.h"
#include <ezxml_mem.h>

/* Global object. */
static ISIM_GlobalObj *_ISIM_globalObj_ptr = NULL;

/* local functions */
static vint _ISIM_accountInit(void);
vint _ISIM_prepare(void);

/*
 * ======== _ISIM_pduHexCharToInt() ========
 *
 * Private function to convert hex character to int
 *
 * Returns:
 *    0: Failed to convert.
 *    Non-zero: converted int.
 */
static int _ISIM_pduHexCharToInt(
    char c)
{
    if (c >= '0' && c <= '9') {
        return (c - '0');
    }
    if (c >= 'A' && c <= 'F') {
        return (c - 'A' + 10);
    }
    if (c >= 'a' && c <= 'f') {
        return (c - 'a' + 10);
    }
    return 0;
}

/*
 * ======== _ISIM_pduHexStringToBytes() ========
 *
 * Private function to convert hex string to bytes
 *
 * Returns:
 *    Converted bytes size.
 */
static int _ISIM_pduHexStringToBytes(
    char          *in_ptr,
    unsigned char *out_ptr)
{
    int sz, i;

    char a,b;

    sz = OSAL_strlen(in_ptr);

    for (i = 0 ; i < sz ; i += 2) {
        a = _ISIM_pduHexCharToInt(in_ptr[i]) << 4;
        b = _ISIM_pduHexCharToInt(in_ptr[i + 1]);
        out_ptr[i / 2] = a | b;
    }
    return (i / 2);
}

/*
 * ======== _ISIM_printBytes() ========
 *
 * Private function to print the input string with length specified
 *
 * Returns:
 *    None
 */
static void _ISIM_printBytes(
    unsigned char *s_ptr,
    int len)
{
    int x;
    for (x = 0 ; x < len ; x++) {
        OSAL_logMsg("%02X ", s_ptr[x]);
    }
    OSAL_logMsg("\n");
}

/*
 * ======== _ISIM_ipsecCalculateAKAResp() ========
 *
 * Calculate the AKA response by given rand and autn
 *
 * Returns:
 *      ISIM_OK: function exits normally.
 *      ISIM_ERR: in case of error
 */
vint _ISIM_ipsecCalculateAKAResp(
    uint8           *rand_ptr,
    uint8           *autn_ptr,
    uint8           *outResp)
{
    vint    i;
    uint8  *sqn_ptr;
    uint8   ki[ISIM_AUTH_AKA_KI_SZ];
    uint8   op[ISIM_AUTH_AKA_OP_SZ];
    uint8   opc[ISIM_AUTH_AKA_OPC_SZ];
    uint8   amf[ISIM_AUTH_AKA_AMF_SZ];
    uint8   ck[ISIM_AUTH_AKA_CKLEN];
    uint8   ik[ISIM_AUTH_AKA_IKLEN];
    uint8   ak[ISIM_AUTH_AKA_AKLEN];
    uint8   sqn[ISIM_AUTH_AKA_SQNLEN];
    uint8   xmac[ISIM_AUTH_AKA_MACLEN];
    uint8   res[ISIM_AUTH_AKA_RESLEN];
    int     count;
    int     useOp = 0;

    /* Copy ki, op, opc, and amf */
    OSAL_memSet(ki, 0, ISIM_AUTH_AKA_KI_SZ);
    OSAL_memSet(amf, 0, ISIM_AUTH_AKA_AMF_SZ);

    /* Convert ki, op, opc from string to byte */
    count = _ISIM_pduHexStringToBytes(_ISIM_globalObj_ptr->ki, ki);
    OSAL_logMsg("%s, KI = \n", __FUNCTION__);
    _ISIM_printBytes(ki, count);

    /* Check OP string size. It have to be 32 */
    if (OSAL_strlen(_ISIM_globalObj_ptr->op) == (ISIM_AUTH_AKA_OP_SZ * 2)) {
        OSAL_memSet(op, 0, ISIM_AUTH_AKA_OP_SZ);
        count = _ISIM_pduHexStringToBytes(_ISIM_globalObj_ptr->op, op);
        OSAL_logMsg("%s, OP = ", __FUNCTION__);
        _ISIM_printBytes(op, count);
        useOp = 1;
    }
    /* No valid OP, check OP string size. It have to be 32 */
    else if (OSAL_strlen(_ISIM_globalObj_ptr->opc) ==
            (ISIM_AUTH_AKA_OPC_SZ * 2)) {
        OSAL_memSet(opc, 0, ISIM_AUTH_AKA_OPC_SZ);
        count = _ISIM_pduHexStringToBytes(_ISIM_globalObj_ptr->opc, opc);
        OSAL_logMsg("%s, OPC = ", __FUNCTION__);
        _ISIM_printBytes(opc, count);
        useOp = 0;
    }
    else {
        OSAL_logMsg("%s, no valid OP or OPC value\n", __FUNCTION__);
        return -1;
    }

    sqn_ptr = autn_ptr;

    if (useOp == 1) {
        f2345(ki, rand_ptr, res, ck, ik, ak, op);
    }
    else {
        f2345_opc(ki, rand_ptr, res, ck, ik, ak, opc);
    }

    /* Compute sequence number SQN */
    for (i = 0 ; i < ISIM_AUTH_AKA_SQNLEN; ++i) {
        sqn[i] = (char) (sqn_ptr[i] ^ ak[i]);
    }

    /* Verify MAC in the challenge */
    if (useOp == 1) {
        f1(ki, rand_ptr, sqn, amf, xmac, op);
    }
    else {
        f1_opc(ki, rand_ptr, sqn, amf, xmac, opc);
    }

    OSAL_memCpy(outResp, res, ISIM_AUTH_AKA_RESLEN);

    return 0;
}

/*
 * ======== _ISIM_writeCsmEvent() ========
 *
 * Function to write CSM_InputEvent to CSM.
 *
 * Returns:
 *      ISIM_OK:  Event wrote successfully.
 *      ISIM_ERR: Event wrote failed.
 */
vint _ISIM_writeCsmEvent(
    const CSM_InputEvent *csmEvt_ptr)
{
    if (OSAL_SUCCESS != OSAL_msgQSend(_ISIM_globalObj_ptr->csmEventInQ,
            (void *)csmEvt_ptr, sizeof(CSM_InputEvent), OSAL_NO_WAIT, NULL)) {
        OSAL_logMsg("%s: Failed to write command", __FUNCTION__);
        return (ISIM_ERR);
    }
    return (ISIM_OK);
}

/*
 * ======== _ISIM_writeCsmServiceEvent() ========
 *
 * Function to write CSM_InputEvent of service to CSM.
 *
 * Returns:
 *      ISIM_OK:  Event wrote successfully.
 *      ISIM_ERR: Event wrote failed.
 */
vint _ISIM_writeCsmServiceEvent(
    vint            reason,
    char           *reasonDesc,
    CSM_InputEvent *csmEvt_ptr)
{
    CSM_InputService   *service_ptr;

    service_ptr = &csmEvt_ptr->evt.service;

    csmEvt_ptr->type    = CSM_EVENT_TYPE_SERVICE;
    service_ptr->reason = (CSM_ServiceReason)reason;
    OSAL_strncpy(service_ptr->reasonDesc, reasonDesc,
            sizeof(service_ptr->reasonDesc));

    /* Send to CSM */
    if (ISIM_OK != _ISIM_writeCsmEvent(csmEvt_ptr)) {
        OSAL_logMsg("%s: Failed to write service command", __FUNCTION__);
        return (ISIM_ERR);
    }
    return (ISIM_OK);
}

/*
 * ======== _ISIM_sendAkaResponse() ========
 *
 * Function to send AKA response to CSM
 *
 * resp_ptr: NULL terminated string of AKA response
 * length : The length of response
 *
 * Returns:
 *      ISIM_OK: function exits normally.
 *      ISIM_ERR: in case of error
 */
vint _ISIM_sendAkaResponse(
    char           *resp_ptr,
    int             length,
    CSM_InputEvent *csmInputEvt_ptr)
{
    CSM_InputService *service_ptr;

    service_ptr = &csmInputEvt_ptr->evt.service;

    /* Construct CSM Event */
    csmInputEvt_ptr->type = CSM_EVENT_TYPE_SERVICE;
    OSAL_memCpy(service_ptr->u.aka.response, resp_ptr, CSM_AKA_RESP_STRING_SZ);
    if (length > 0) {
        service_ptr->reason = CSM_SERVICE_REASON_ISIM_AKA_RESPONSE_SUCCESS;
        OSAL_strncpy(service_ptr->reasonDesc,
                "CSM_ACCOUNT_REASON_ISIM_AKA_RESPONSE",
                sizeof(service_ptr->reasonDesc));
        service_ptr->u.aka.resLength = length;
    }
    else {
        service_ptr->reason =
                CSM_SERVICE_REASON_ISIM_AKA_RESPONSE_NETWORK_FAILURE;
        OSAL_strncpy(service_ptr->reasonDesc,
                "CSM_ACCOUNT_REASON_ISIM_AKA_RESPONSE_NETWORK_FAILURE",
                sizeof(service_ptr->reasonDesc));
        service_ptr->u.aka.resLength = 0;
    }
    /* Send to CSM */
    return (_ISIM_writeCsmEvent(csmInputEvt_ptr));
}

/*
 * ======== _ISIM_processCsmEvent() ========
 *
 * This function is process the CSM_OutputEvent from CSM.
 *
 * Returns:
 *     ISIM_OK: Event processed successfully.
 *     ISIM_ERR: Event processed failed
 */
vint _ISIM_processCsmEvent(
    CSM_OutputEvent  *outputEvt_ptr,
    CSM_InputEvent   *inputEvt_ptr)
{
    CSM_OutputService *service_ptr;
    char outResp[ISIM_AUTH_AKA_RESP_SZ];
    int ret = 0;

    OSAL_memSet(outResp, 0, ISIM_AUTH_AKA_RESP_SZ);

    switch (outputEvt_ptr->type) {
        case CSM_EVENT_TYPE_SERVICE:
            service_ptr = &outputEvt_ptr->evt.service;
            if (CSM_OUTPUT_REASON_SERVICE_AUTH_CHALLENGE ==
                    service_ptr->reason) {
                ISIM_dbgPrintf("Got AKA Challenge");
                ret =  _ISIM_ipsecCalculateAKAResp(
                        (uint8 *)service_ptr->u.aka.akaRand,
                        (uint8 *)service_ptr->u.aka.akaAutn, (uint8 *)outResp);
                if (ret == 0) {
                    _ISIM_sendAkaResponse(outResp, ISIM_AUTH_AKA_RESLEN,
                            inputEvt_ptr);
                }
                else {
                    _ISIM_sendAkaResponse(outResp, 0, inputEvt_ptr);
                }
            }
            break;
        default:
            OSAL_logMsg("%s:%d ERROR bad response type\n", __func__, __LINE__);
            break;
    }

    return (ISIM_OK);
}


/*
 * ======== ISIM_csmEventTask() ========
 *
 * This function is the entry point for a thread that handles events from CSM.
 * This thread waits (blocks) on OSAL_msgQRecv() processes events as they are
 * received.
 *
 * Returns:
 *   Nothing and never.
 */
static void _ISIM_csmEventTask(
    OSAL_TaskArg taskArg)
{
    OSAL_MsgQId     qId;
    uint32          msTimeout;
    OSAL_Boolean    timeout;

    /* refactored startup actions */
    _ISIM_prepare();
    
    qId = taskArg;
    msTimeout = 1000;

    /* Loop forever to receive event from CSM and process it */
    while (1) {
        if (0 < OSAL_msgQRecv(qId,
                (char *)&_ISIM_globalObj_ptr->csmOutputEvent,
                sizeof(CSM_OutputEvent),
                msTimeout, 
                &timeout)) {
            /*
             * Processing the event may result in sending a CSM_InputEvent so provide
             * a reference to where this thread can construct CSM input events.
             */
            _ISIM_processCsmEvent(&_ISIM_globalObj_ptr->csmOutputEvent,
                    &_ISIM_globalObj_ptr->csmInputEvent);
        }
        else {
            OSAL_taskDelay(100);
        }
    }
}

/*
 * ======== _ISIM_accountInit() ========
 *
 * Function to read account information from xml configuration file.
 *
 * Returns:
 *      ISIM_OK: function exits normally.
 *      ISIM_ERR: in case of error
 */
static vint _ISIM_accountInit(void)
{
    void             *cfg_ptr = SETTINGS_cfgMemAlloc(SETTINGS_TYPE_ISIM);
    char             *value_ptr;

    CSM_InputEvent    csmEvt;
    CSM_InputService *service_ptr;
    CSM_InputSms     *sms_ptr;
    char              smsc[CSM_EVENT_STRING_SZ] = {0};
    char              imeiUri[CSM_EVENT_STRING_SZ] = {0};

    service_ptr = &csmEvt.evt.service;
    sms_ptr  = &csmEvt.evt.sms;

    OSAL_memSet(service_ptr, 0, sizeof(CSM_InputService));
    /* Get the XML init info */
    if (SETTINGS_RETURN_OK != SETTINGS_getContainer(SETTINGS_TYPE_ISIM,
            ISIM_CONFIG_DEFAULT_PATH"/isim.xml", cfg_ptr)) {
        SETTINGS_memFreeDoc(SETTINGS_TYPE_ISIM, cfg_ptr);
        OSAL_logMsg("%s:%d ERROR opening ISIM settings\n",
                __FUNCTION__, __LINE__);
        return (ISIM_ERR);
    }

    /* Read the account service settings */
    /* Get pcscf */
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_ISIM,
            SETTINGS_NESTED_NONE, cfg_ptr, SETTINGS_TAG_SERVICE, NULL,
            NULL, SETTINGS_PARM_PCSCF))) {
        OSAL_strncpy(service_ptr->u.pcscf, value_ptr, CSM_EVENT_LONG_STRING_SZ);
    }
    ISIM_dbgPrintf("pcsf=%s", service_ptr->u.pcscf);
    /* Send to CSM */
    if (ISIM_OK != _ISIM_writeCsmServiceEvent(CSM_SERVICE_REASON_SET_PCSCF,
            "CSM_SERVICE_REASON_SET_PCSCF", &csmEvt)) {
        SETTINGS_memFreeDoc(SETTINGS_TYPE_ISIM, cfg_ptr);
        return (ISIM_ERR);
    }

    /* Set emergency pcscf same as pcscf */
    ISIM_dbgPrintf("emergency pcsf=%s", service_ptr->u.pcscf);
    /* Send to CSM */
    if (ISIM_OK != 
            _ISIM_writeCsmServiceEvent(CSM_SERVICE_REASON_SET_EMGCY_PCSCF,
            "CSM_SERVICE_REASON_SET_EMGCY_PCSCF", &csmEvt)) {
        SETTINGS_memFreeDoc(SETTINGS_TYPE_ISIM, cfg_ptr);
        return (ISIM_ERR);
    }

    /* Get impu */
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_ISIM,
            SETTINGS_NESTED_NONE, cfg_ptr, SETTINGS_TAG_SERVICE, NULL,
            NULL, SETTINGS_PARM_IMPU))) {
        OSAL_strncpy(service_ptr->u.impu, value_ptr, CSM_EVENT_STRING_SZ);
    }
    ISIM_dbgPrintf("impu=%s", service_ptr->u.impu);
    /* Send to CSM */
    if (ISIM_OK != _ISIM_writeCsmServiceEvent(CSM_SERVICE_REASON_SET_IMPU,
            "CSM_SERVICE_REASON_SET_IMPU", &csmEvt)) {
        SETTINGS_memFreeDoc(SETTINGS_TYPE_ISIM, cfg_ptr);
        return (ISIM_ERR);
    }

    /* Get impi */
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_ISIM,
            SETTINGS_NESTED_NONE, cfg_ptr, SETTINGS_TAG_SERVICE, NULL,
            NULL, SETTINGS_PARM_IMPI))) {
        OSAL_strncpy(service_ptr->u.impi, value_ptr, CSM_EVENT_STRING_SZ);
    }
    ISIM_dbgPrintf("impi=%s", service_ptr->u.impi);
    /* Send to CSM */
    if (ISIM_OK != _ISIM_writeCsmServiceEvent(CSM_SERVICE_REASON_SET_IMPI,
            "CSM_SERVICE_REASON_SET_IMPI", &csmEvt)) {
        SETTINGS_memFreeDoc(SETTINGS_TYPE_ISIM, cfg_ptr);
        return (ISIM_ERR);
    }

    /* Get domain */
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_ISIM,
            SETTINGS_NESTED_NONE, cfg_ptr, SETTINGS_TAG_SERVICE, NULL,
            NULL, SETTINGS_PARM_DOMAIN))) {
        OSAL_strncpy(service_ptr->u.domain, value_ptr, CSM_EVENT_STRING_SZ);
    }
    ISIM_dbgPrintf("domain=%s", service_ptr->u.domain);
    /* Send to CSM */
    if (ISIM_OK != _ISIM_writeCsmServiceEvent(CSM_SERVICE_REASON_SET_DOMAIN,
            "CSM_SERVICE_REASON_SET_DOMAIN", &csmEvt)) {
        SETTINGS_memFreeDoc(SETTINGS_TYPE_ISIM, cfg_ptr);
        return (ISIM_ERR);
    }

    /* Get password */
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_ISIM,
            SETTINGS_NESTED_NONE, cfg_ptr, SETTINGS_TAG_SERVICE, NULL,
            NULL, SETTINGS_PARM_PASSWORD))) {
        OSAL_strncpy(service_ptr->u.password, value_ptr, CSM_EVENT_STRING_SZ);
    }
    ISIM_dbgPrintf("password=%s", service_ptr->u.password);
    /* Send to CSM */
    if (ISIM_OK != _ISIM_writeCsmServiceEvent(CSM_SERVICE_REASON_SET_PASSWORD,
            "CSM_SERVICE_REASON_SET_PASSWORD", &csmEvt)) {
        SETTINGS_memFreeDoc(SETTINGS_TYPE_ISIM, cfg_ptr);
        return (ISIM_ERR);
    }

    /* Get v2 conference server */
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_ISIM,
            SETTINGS_NESTED_NONE, cfg_ptr, SETTINGS_TAG_SERVICE, NULL,
            NULL, SETTINGS_PARM_AUDIO_CONF_SERVER))) {
        OSAL_strncpy(service_ptr->u.audioConfServer, value_ptr,
                CSM_EVENT_STRING_SZ);
        ISIM_dbgPrintf("audio conf server=%s", service_ptr->u.audioConfServer);
        /* Send to CSM */
        if (ISIM_OK != _ISIM_writeCsmServiceEvent(
                CSM_SERVICE_REASON_SET_AUDIO_CONF_SERVER,
                "CSM_SERVICE_REASON_SET_AUDIO_CONF_SERVER", &csmEvt)) {
            SETTINGS_memFreeDoc(SETTINGS_TYPE_ISIM, cfg_ptr);
            return (ISIM_ERR);
        }
    }

    /* Get video conference server */
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_ISIM,
            SETTINGS_NESTED_NONE, cfg_ptr, SETTINGS_TAG_SERVICE, NULL,
            NULL, SETTINGS_PARM_VIDEO_CONF_SERVER))) {
        OSAL_strncpy(service_ptr->u.videoConfServer, value_ptr,
                CSM_EVENT_STRING_SZ);
        ISIM_dbgPrintf("video conf server=%s", service_ptr->u.videoConfServer);
        /* Send to CSM */
        if (ISIM_OK != _ISIM_writeCsmServiceEvent(
                CSM_SERVICE_REASON_SET_VIDEO_CONF_SERVER,
                "CSM_SERVICE_REASON_SET_VIDEO_CONF_SERVER", &csmEvt)) {
            SETTINGS_memFreeDoc(SETTINGS_TYPE_ISIM, cfg_ptr);
            return (ISIM_ERR);
        }
    }
    else {
        if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_ISIM,
                SETTINGS_NESTED_NONE, cfg_ptr, SETTINGS_TAG_SERVICE, NULL,
                NULL, SETTINGS_PARM_AUDIO_CONF_SERVER))) {
            OSAL_strncpy(service_ptr->u.videoConfServer, value_ptr,
                    CSM_EVENT_STRING_SZ);
            ISIM_dbgPrintf("video conf server=%s",
                    service_ptr->u.videoConfServer);
            /* Send to CSM */
            if (ISIM_OK != _ISIM_writeCsmServiceEvent(
                    CSM_SERVICE_REASON_SET_VIDEO_CONF_SERVER,
                    "CSM_SERVICE_REASON_SET_VIDEO_CONF_SERVER", &csmEvt)) {
                SETTINGS_memFreeDoc(SETTINGS_TYPE_ISIM, cfg_ptr);
                return (ISIM_ERR);
            }
        }
    }

    /* Get smsc */
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_ISIM,
            SETTINGS_NESTED_NONE, cfg_ptr, SETTINGS_TAG_SERVICE, NULL,
            NULL, SETTINGS_PARM_SMSC))) {
        OSAL_strncpy(smsc, value_ptr, CSM_EVENT_STRING_SZ);
    }

    /* Get imei uri */
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_ISIM,
            SETTINGS_NESTED_NONE, cfg_ptr, SETTINGS_TAG_SERVICE, NULL,
            NULL, SETTINGS_PARM_IMEI_URI))) {
        OSAL_strncpy(imeiUri, value_ptr, CSM_EVENT_STRING_SZ);
    }

    /* Get AKA KI */
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_ISIM,
            SETTINGS_NESTED_NONE, cfg_ptr, SETTINGS_TAG_SERVICE, NULL,
            NULL, SETTINGS_PARM_AKA_KI))) {
        OSAL_strncpy(_ISIM_globalObj_ptr->ki, value_ptr,
                CSM_EVENT_STRING_SZ);
    }

    /* Get AKA OP */
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_ISIM,
            SETTINGS_NESTED_NONE, cfg_ptr, SETTINGS_TAG_SERVICE, NULL,
            NULL, SETTINGS_PARM_AKA_OP))) {
        OSAL_strncpy(_ISIM_globalObj_ptr->op, value_ptr,
                CSM_EVENT_STRING_SZ);
    }

    /* Get AKA OPC */
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_ISIM,
            SETTINGS_NESTED_NONE, cfg_ptr, SETTINGS_TAG_SERVICE, NULL,
            NULL, SETTINGS_PARM_AKA_OPC))) {
        OSAL_strncpy(_ISIM_globalObj_ptr->opc, value_ptr,
                CSM_EVENT_STRING_SZ);
    }

    /* Send event to CSM to congigure smsc address if smsc presented */
    if (0 != smsc[0]) {
        ISIM_dbgPrintf("smsc=%s", smsc);
        /* Construct CSM Event for smsc configuration */
        csmEvt.type = CSM_EVENT_TYPE_SMS;
        sms_ptr->reason = CSM_SMS_REASON_SET_SMSC;
        /* Set up reason */
        OSAL_strncpy(sms_ptr->reasonDesc, "CSM_SMS_REASON_SET_SMSC",
                sizeof(sms_ptr->reasonDesc));
        /* Set up smsc */
        OSAL_strncpy(sms_ptr->smsc, smsc, sizeof(sms_ptr->smsc));

        /* Send to CSM for SIP service */
        if (ISIM_OK != _ISIM_writeCsmEvent(&csmEvt)) {
            SETTINGS_memFreeDoc(SETTINGS_TYPE_ISIM, cfg_ptr);
            return (ISIM_ERR);
        }
    }

    /* Send event to CSM to congigure IMEI URI if IMEI presented */
    if (0 != imeiUri[0]) {
        ISIM_dbgPrintf("imeiUri=%s", imeiUri);
        OSAL_strncpy(service_ptr->u.imeiUri, imeiUri, CSM_EVENT_STRING_SZ);
        /* Send to CSM */
        if (ISIM_OK != _ISIM_writeCsmServiceEvent(
                CSM_SERVICE_REASON_SET_IMEI_URI,
                "CSM_SERVICE_REASON_SET_IMEI_URI", &csmEvt)) {
            SETTINGS_memFreeDoc(SETTINGS_TYPE_ISIM, cfg_ptr);
            return (ISIM_ERR);
        }
    }

    /* Get instance id */
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_ISIM,
            SETTINGS_NESTED_NONE, cfg_ptr, SETTINGS_TAG_SERVICE, NULL,
            NULL, SETTINGS_PARM_INSTANCE_ID))) {
        OSAL_strncpy(service_ptr->u.instanceId, value_ptr,
                CSM_INSTANCE_STRING_SZ);
        /* Send to CSM */
        if (ISIM_OK != _ISIM_writeCsmServiceEvent(
                CSM_SERVICE_REASON_SET_INSTANCE_ID,
                "CSM_SERVICE_REASON_SET_INSTANCE_ID", &csmEvt)) {
            SETTINGS_memFreeDoc(SETTINGS_TYPE_ISIM, cfg_ptr);
            return (ISIM_ERR);
        }
    }

#ifdef ISIM_ENABLE_IMS_REGISTRATION
    /* Send to CSM */
    if (ISIM_OK != _ISIM_writeCsmServiceEvent(CSM_SERVICE_REASON_IMS_ENABLE,
            "CSM_SERVICE_REASON_IMS_ENABLE", &csmEvt)) {
        SETTINGS_memFreeDoc(SETTINGS_TYPE_ISIM, cfg_ptr);
        return (ISIM_ERR);
    }
#endif
    SETTINGS_memFreeDoc(SETTINGS_TYPE_ISIM, cfg_ptr);

    return (ISIM_OK);
}

 /*
 * ======== _ISIM_prepare() ========
 *
 * Internal routine for doing the ISIM startup actions
 *
 * Returns:
 *      ISIM_OK: function exits normally.
 *      ISIM_ERR: in case of error
 */
vint _ISIM_prepare(
     void)
{
    /* at task thread time and all modules are allocated */
    return _ISIM_accountInit();
}

/*
 * ======== _ISIM_deallocate() ========
 *
 * Internal routine for free up the ISIM module resources
 *
 * Returns:
 *      ISIM_OK: function exits normally.
 *      ISIM_ERR: in case of error
 */
vint _ISIM_deallocate(
     void)
{
#ifndef ISIM_DISABLE_CSM_EVENT_RECV
    /* Delete CSM output isim Q */
    OSAL_msgQDelete(_ISIM_globalObj_ptr->csmIsimQ);
#endif
    /* Delete CSM input event Q */
    OSAL_msgQDelete(_ISIM_globalObj_ptr->csmEventInQ);

    /* Free global object */
    OSAL_memFree(_ISIM_globalObj_ptr, 0);
    _ISIM_globalObj_ptr = NULL;
    
    return (ISIM_OK);
}

/*
 * ======== _ISIM_stop() ========
 *
 * Internal routine for stoping the ISIM module task
 *
 * Returns:
 *      ISIM_OK: function exits normally.
 *      ISIM_ERR: in case of error
 */
vint _ISIM_stop(
     void)
{
#ifndef ISIM_DISABLE_CSM_EVENT_RECV
    /* Delete CSM event task */
    OSAL_taskDelete(_ISIM_globalObj_ptr->csmEventTaskId);
#endif
    return (ISIM_OK);
}

/*
 * ======== ISIM_allocate() ========
 *
 * Public routine for allocating the ISIM module resource
 *
 * Returns:
 *      ISIM_OK: function exits normally.
 *      ISIM_ERR: in case of error
 */
vint ISIM_allocate(
     void)
{
    /* Allocate memory for global object */
    _ISIM_globalObj_ptr = OSAL_memCalloc(1, sizeof(ISIM_GlobalObj), 0);
    OSAL_memSet(_ISIM_globalObj_ptr, 0, sizeof(ISIM_GlobalObj));

#ifndef ISIM_DISABLE_CSM_EVENT_RECV
    /* Create output event queue to receive event from CSM */
    if (0 == (_ISIM_globalObj_ptr->csmIsimQ = OSAL_msgQCreate(
            CSM_OUTPUT_ISIM_EVENT_QUEUE_NAME,
            OSAL_MODULE_CSM_PUBLIC, OSAL_MODULE_ISIM,
            OSAL_DATA_STRUCT_CSM_OutputEvent,
            CSM_OUTPUT_ISIM_EVENT_MSGQ_LEN, sizeof(CSM_OutputEvent), 0))) {
        OSAL_logMsg("%s:%d ERROR opening CSM ISIM output event Q\n",
                __FILE__, __LINE__);
        return (ISIM_ERR);
    }
#endif
    /* Create input event queue to send event to CSM */
    if (0 == (_ISIM_globalObj_ptr->csmEventInQ = OSAL_msgQCreate(
                CSM_INPUT_EVENT_QUEUE_NAME,
                OSAL_MODULE_ISIM, OSAL_MODULE_CSM_PUBLIC,
                OSAL_DATA_STRUCT_CSM_InputEvent,
                CSM_INPUT_EVENT_MSGQ_LEN, sizeof(CSM_InputEvent), 0))) {
        OSAL_logMsg("%s:%d ERROR opening CSM Input Event Q\n",
                __FILE__, __LINE__);
        return (ISIM_ERR);
    }

    return (ISIM_OK);
}

/*
 * ======== ISIM_start() ========
 *
 * Public routine for starting the ISIM module tasks/actions
 *
 * Returns:
 *      ISIM_OK: function exits normally.
 *      ISIM_ERR: in case of error
 */
vint ISIM_start(
     void)
{
#ifndef ISIM_DISABLE_CSM_EVENT_RECV
    /* Create ISIM thread */
    if (0 == (_ISIM_globalObj_ptr->csmEventTaskId = OSAL_taskCreate(
            ISIM_CSM_EVENT_TASK_NAME,
            OSAL_TASK_PRIO_NRT, ISIM_CSM_EVENT_TASK_STACK_SZ,
            (OSAL_TaskPtr)_ISIM_csmEventTask,
            (void *)_ISIM_globalObj_ptr->csmIsimQ))) {
        OSAL_logMsg("Error starting ISIM thread\n");
        return (ISIM_ERR);
    }
#endif
    return (ISIM_OK);
}

/*
 * ======== ISIM_destroy() ========
 *
 * Public routine for shutting down the ISI manager package.
 *
 * Returns:
 *      ISIM_OK: function exits normally.
 *      ISIM_ERR: in case of error
 */
vint ISIM_destroy(
    void)
{
    if (NULL == _ISIM_globalObj_ptr) {
        return (ISIM_OK);
    }

    _ISIM_stop();
    _ISIM_deallocate();

    return (ISIM_OK);
}

/*
 * ======== ISIM_init() ========
 *
 * Public routine for initializing the ISIM application
 * ** refactored **
 *
 * Returns:
 *      ISIM_OK: function exits normally.
 *      ISIM_ERR: in case of error
 */
vint ISIM_init(
     void)
{    
    if (ISIM_ERR == ISIM_allocate()) {
        return (ISIM_ERR);
    }
    if (ISIM_ERR == ISIM_start()) {
        return (ISIM_ERR);
    }
    return (ISIM_OK);
}

/*
 * ======== ISIM_shutdown() ========
 *
 * Public routine for shutting down the ISI manager package.
 *
 * Returns:
 *      ISIM_OK: function exits normally.
 *      ISIM_ERR: in case of error
 */
vint ISIM_shutdown(
    void)
{
    return ISIM_destroy();
}

