/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 13916 $ $Date: 2011-01-29 15:54:59 -0600 (Sat, 29 Jan 2011) $
 */
#include <osal_types.h>
#include <osal.h>
#include <osal_msg.h>

#include <ezxml.h>

#include <sip_sip.h>
#include <sip_xport.h>
#include <sip_ua.h>
#include <sip_app.h>
#include <sip_debug.h>

#include "isi.h"
#include "isip.h"

#include "mns.h"

#ifdef INCLUDE_SIMPLE
#include "xcap.h"
#include "_simple_types.h"
#endif

#include "_sapp.h"
#include "_sapp_parse_helper.h"
#include "_sapp_dialog.h"
#include "_sapp_call_settings.h"

static const char _SAPP_CS_CONTENT_TYPE[]       = "application/fsservice+xml";
static const char _SAPP_FS_SERVICES_TAG[]       = "fs-services";
static const char _SAPP_FS_SERVICE_TAG[]        = "fs-service";
static const char _SAPP_ACTION_ATTR[]           = "action";

/* tags and values used for enabling/disabling settings */
static const char _SAPP_ACTION_ACTIVATE[]       = "serviceinfo-activate";
static const char _SAPP_ACTION_DEACTIVATE[]     = "serviceinfo-deactivate";
static const char _SAPP_RESULT_ATTR[]           = "result";
static const char _SAPP_RESULT_SUCCESS[]        = "success";
static const char _SAPP_RESULT_FAIL[]           = "failure";
static const char _SAPP_ARGUMENT_TEXT[]         = "text";

/* tags and values used for retrieving settings */
static const char _SAPP_ACTION_CONFIRM[]        = "serviceinfo-confirm";
static const char _SAPP_STATUS_ATTR[]           = "status";
static const char _SAPP_STATUS_ENABLED[]        = "subscriber_reg";
static const char _SAPP_STATUS_DISABLED[]       = "operator_reg";
static const char _SAPP_STATUS_NOT_SUPPORTED[]  = "operator_not_reg";
static const char _SAPP_ARGUMENT_TDN[]          = "tdn";

/* NOTE These values must match what is in D2C's AccountSettings class */
enum {
    ACCOUNT_SETTING_NONE = 0,
    ACCOUNT_SETTING_RETRIEVE_CFU,
    ACCOUNT_SETTING_RETRIEVE_CFB,
    ACCOUNT_SETTING_RETRIEVE_CFNR,
    ACCOUNT_SETTING_RETRIEVE_CFNL,
    ACCOUNT_SETTING_RETRIEVE_VM_ROUTING,
    ACCOUNT_SETTING_RETRIEVE_MCN,
    ACCOUNT_SETTING_RETRIEVE_CAW,
    ACCOUNT_SETTING_RETRIEVE_LAST,
    ACCOUNT_SETTING_RETRIEVE_CLIR = ACCOUNT_SETTING_RETRIEVE_LAST,
    
    ACCOUNT_SETTING_ENABLE_CFU,
    ACCOUNT_SETTING_ENABLE_CFB,
    ACCOUNT_SETTING_ENABLE_CFNR,
    ACCOUNT_SETTING_ENABLE_CFNL,
    ACCOUNT_SETTING_ENABLE_VM_ROUTING,
    ACCOUNT_SETTING_ENABLE_MCN,
    ACCOUNT_SETTING_ENABLE_CAW,
    ACCOUNT_SETTING_ENABLE_CLIR,
    
    ACCOUNT_SETTING_DISABLE_CFU,
    ACCOUNT_SETTING_DISABLE_CFB,
    ACCOUNT_SETTING_DISABLE_CFNR,
    ACCOUNT_SETTING_DISABLE_CFNL,
    ACCOUNT_SETTING_DISABLE_VM_ROUTING,
    ACCOUNT_SETTING_DISABLE_MCN,
    ACCOUNT_SETTING_DISABLE_CAW,
    ACCOUNT_SETTING_DISABLE_CLIR /* 24 */
};

static void _SAPP_callSettingsIsiEvt(
    ISI_Id              serviceId,
    vint                protocolId,
    vint                cmd,
    vint                cmdValue,
    char               *argument_ptr,
    ISIP_Message       *isi_ptr)
{
    isi_ptr->code = ISIP_CODE_TEL_EVENT;
    isi_ptr->protocol = protocolId;
    isi_ptr->msg.event.reason = ISIP_TEL_EVENT_REASON_NEW;
    isi_ptr->msg.event.evt = ISI_TEL_EVENT_RESERVED_2;
    isi_ptr->msg.event.serviceId = serviceId;
    
    /* Set up your arg0.  This is the command */
    isi_ptr->msg.event.settings.args.arg0 = cmd;
    isi_ptr->msg.event.settings.args.arg1 = cmdValue;
    if (NULL != argument_ptr) {
        OSAL_strncpy(isi_ptr->msg.event.reasonDesc,
                argument_ptr, ISI_EVENT_DESC_STRING_SZ);
    }
    return;
}

typedef struct
{
   vint          active;
   vint          deactive;
   vint          confirm;
   const char   *tag_ptr;
   const char   *tag_data_ptr;
} SAPP_CallSettings;

static SAPP_CallSettings _SAPP_Cmds[8] = {
    { ACCOUNT_SETTING_ENABLE_CFU,        ACCOUNT_SETTING_DISABLE_CFU,        ACCOUNT_SETTING_RETRIEVE_CFU,         "cfu",    "cfu-data"   },
    { ACCOUNT_SETTING_ENABLE_CFB,        ACCOUNT_SETTING_DISABLE_CFB,        ACCOUNT_SETTING_RETRIEVE_CFB,         "cfob",   "cfob-data"  },
    { ACCOUNT_SETTING_ENABLE_CFNR,       ACCOUNT_SETTING_DISABLE_CFNR,       ACCOUNT_SETTING_RETRIEVE_CFNR,        "cfnr",   "cfnr-data"  },
    { ACCOUNT_SETTING_ENABLE_CFNL,       ACCOUNT_SETTING_DISABLE_CFNL,       ACCOUNT_SETTING_RETRIEVE_CFNL,        "cfnl",   "cfnl-data"  },
    { ACCOUNT_SETTING_ENABLE_VM_ROUTING, ACCOUNT_SETTING_DISABLE_VM_ROUTING, ACCOUNT_SETTING_RETRIEVE_VM_ROUTING,  "vms",    "vms-data"   },
    { ACCOUNT_SETTING_ENABLE_MCN,        ACCOUNT_SETTING_DISABLE_MCN,        ACCOUNT_SETTING_RETRIEVE_MCN,         "mcn",    "mcn-data"   },
    { ACCOUNT_SETTING_ENABLE_CAW,        ACCOUNT_SETTING_DISABLE_CAW,        ACCOUNT_SETTING_RETRIEVE_CAW,         "caw",    "caw-data"   },
    { ACCOUNT_SETTING_ENABLE_CLIR,       ACCOUNT_SETTING_DISABLE_CLIR,       ACCOUNT_SETTING_RETRIEVE_CLIR,        "clir",   "clir-data"  },
};

static SAPP_CallSettings* _SAPP_callSettingsGetCmd(
    const char *feature_ptr)
{
    vint x;
    vint size = sizeof(_SAPP_Cmds) / sizeof(_SAPP_Cmds[0]);
    const char *int_ptr;
    for (x = 0 ; x < size ; x++) {
        int_ptr = _SAPP_Cmds[x].tag_ptr;
        if (0 == OSAL_strncasecmp(feature_ptr, int_ptr, OSAL_strlen(int_ptr))) {
            return &_SAPP_Cmds[x];
        }
    }
    return (NULL);
}

static vint _SAPP_callSettingsRetrieve(
    ezxml_t    xml_ptr,
    vint      *status_ptr,
    char     **argument_ptr)
{
    const char *value_ptr;
    vint status;
    /* xml_ptr is the <xxx-data> tag at this point */
    if (NULL == (value_ptr = ezxml_attr(xml_ptr, _SAPP_STATUS_ATTR))) {
        /* This is mandatory so return error if it doesn't exist */
        return (SAPP_ERR);
    }
    if (0 == OSAL_strncasecmp(value_ptr, _SAPP_STATUS_ENABLED,
            sizeof(_SAPP_STATUS_ENABLED) - 1)) {
        status = 1;
    }
    else if (0 == OSAL_strncasecmp(value_ptr,
            _SAPP_STATUS_DISABLED, sizeof(_SAPP_STATUS_DISABLED) - 1)) {
        status = 0;
    }
    else if (0 == OSAL_strncasecmp(value_ptr, _SAPP_STATUS_NOT_SUPPORTED,
            sizeof(_SAPP_STATUS_NOT_SUPPORTED) - 1)) {
        status = 0;
    }
    else {
        return (SAPP_ERR);
    }
    *status_ptr = status;
    /* We have the value of the command.  Let's get any arguments */
    *argument_ptr = (char*)ezxml_attr(xml_ptr, _SAPP_ARGUMENT_TDN);
    return (SAPP_OK);
}

static vint _SAPP_callSettingsEnableDisable(
    ezxml_t    xml_ptr,
    vint      *result_ptr,
    char     **argument_ptr)
{
    const char *value_ptr;
    vint result;
    /* xml_ptr is the <xxx-data> tag at this point */
    if (NULL == (value_ptr = ezxml_attr(xml_ptr, _SAPP_RESULT_ATTR))) {
        /* This is mandatory so return error if it doesn't exist */
        return (SAPP_ERR);
    }
    if (0 == OSAL_strncasecmp(value_ptr, _SAPP_RESULT_SUCCESS,
            sizeof(_SAPP_RESULT_SUCCESS) - 1)) {
        result = 1;
    }
    else if (0 == OSAL_strncasecmp(value_ptr, _SAPP_RESULT_FAIL,
            sizeof(_SAPP_RESULT_FAIL) - 1)) {
        result = 0;
    }
    else {
        return (SAPP_ERR);
    }
    *result_ptr = result;
    /* We have the value of the command.  Let's get any arguments */
    *argument_ptr = (char*)ezxml_attr(xml_ptr, _SAPP_ARGUMENT_TEXT);
    return (SAPP_OK);
}

static vint _SAPP_callSettingsDecode(
    SAPP_ServiceObj   *service_ptr,
    char              *contentType_ptr,
    char              *payload_ptr,
     vint              payloadSize,
    ISIP_Message      *isi_ptr)
{
    ezxml_t      xml_ptr;
    ezxml_t      child_ptr;
    const char  *value_ptr;
    const char  *action_ptr;
    SAPP_CallSettings *s_ptr;
    vint               cmd;
    vint               result;
    vint               status;
    char              *argument_ptr;

    /* Check if it's 'application/fsservice+xml' */
    if (0 != OSAL_strncasecmp(contentType_ptr, _SAPP_CS_CONTENT_TYPE,
            sizeof(_SAPP_CS_CONTENT_TYPE) - 1)) {
        /* We don't understand, quietly ignore */
        return (SAPP_ERR);
    }

    OSAL_memCpy(service_ptr->payloadStratch, payload_ptr, payloadSize);
    if (NULL == (xml_ptr = ezxml_parse_str(
            service_ptr->payloadStratch,  payloadSize))) {
        return (SAPP_ERR);
    }

    /* Check for the mandatory root tags */
    if (NULL == (value_ptr = ezxml_name(xml_ptr))) {
        ezxml_free(xml_ptr);
        return (SAPP_ERR);
    }
    if (0 != OSAL_strcasecmp(value_ptr, _SAPP_FS_SERVICES_TAG)) {
        ezxml_free(xml_ptr);
        return (SAPP_ERR);
    }

    /* Get the child element */
    if (NULL == (child_ptr = ezxml_child(xml_ptr, _SAPP_FS_SERVICE_TAG))) {
        /* can't find it, return error */
        ezxml_free(xml_ptr);
        return (SAPP_ERR);
    }

    /* Verify the attr  */
    if (NULL == (value_ptr = ezxml_attr(child_ptr, _SAPP_ACTION_ATTR))) {
        /* This is mandatory  */
        ezxml_free(xml_ptr);
        return (SAPP_ERR);
    }
    action_ptr = value_ptr;

    /*
     * Let's get the 'feature' e.g. <cfu>, etc. and the
     * nest child data...<cfu-data>
     */
    if (NULL == (child_ptr = child_ptr->child)) {
        /* This is mandatory */
        ezxml_free(xml_ptr);
        return (SAPP_ERR);
    }
    if (NULL == (value_ptr = ezxml_name(child_ptr))) {
        /* No feature!! return */
        ezxml_free(xml_ptr);
        return (SAPP_ERR);
    }

    /* See if we know the feature */
    if (NULL == (s_ptr = _SAPP_callSettingsGetCmd((const char*)value_ptr))) {
        /* Then we don't know it.  Let's bail */
        ezxml_free(xml_ptr);
        return (SAPP_ERR);
    }

    /* based on the 'action' let's get the right command */
    if (0 == OSAL_strncasecmp(action_ptr, _SAPP_ACTION_ACTIVATE,
            sizeof(_SAPP_ACTION_ACTIVATE) - 1)) {
        cmd = s_ptr->active;
    }
    else if (0 == OSAL_strncasecmp(action_ptr, _SAPP_ACTION_DEACTIVATE,
            sizeof(_SAPP_ACTION_DEACTIVATE) - 1)) {
        cmd = s_ptr->deactive;
    }
    else if (0 == OSAL_strncasecmp(action_ptr, _SAPP_ACTION_CONFIRM,
            sizeof(_SAPP_ACTION_CONFIRM) - 1)) {
        cmd = s_ptr->confirm;
    }
    else {
        /* We don't understand this command */
        ezxml_free(xml_ptr);
        return (SAPP_OK);
    }

    /* Let's get the xxx-data tag */
    if (NULL == (child_ptr = ezxml_child(child_ptr, s_ptr->tag_data_ptr))) {
        /* can't find it, return */
        ezxml_free(xml_ptr);
        return (SAPP_OK);
    }
    
    if (cmd <= ACCOUNT_SETTING_RETRIEVE_LAST) {
        if (SAPP_OK != _SAPP_callSettingsRetrieve(child_ptr, &status,
                &argument_ptr)) {
            ezxml_free(xml_ptr);
            return (SAPP_OK);
        }
        /*
         * We got everything we need.  By now we have the following...
         * 'cmd' = The command that was issued
         * 's_ptr' = details about the command we wanted.
         * 'status' = The status or value of the setting.
         * 'argument_ptr' The argument for the command.  For examplke the 'tdn'.
         *
         */
        _SAPP_callSettingsIsiEvt(service_ptr->isiServiceId, 
                service_ptr->protocolId, cmd,
                status, argument_ptr, isi_ptr);
    }
    else {
        /* Then it is a disable/enable command */
        if (SAPP_OK != _SAPP_callSettingsEnableDisable(child_ptr, &result,
                &argument_ptr)) {
            ezxml_free(xml_ptr);
            return (SAPP_OK);
        }
        /*
         * We got everything we need.  By now we have the following...
         * 'cmd' = The command that was issued
         * 's_ptr' = details about the command we wanted.
         * 'result' = The result of the settings command, success or fail.
         * 'argument_ptr' The argument for the command.  For example the 'test'.
         *
         */
        _SAPP_callSettingsIsiEvt(service_ptr->isiServiceId,
                service_ptr->protocolId, cmd,
                result, argument_ptr, isi_ptr);
    }
    /* Free the xml object since we don't need it anymore */
    ezxml_free(xml_ptr);
    return (SAPP_OK);
}

/*
 * ======== SAPP_callSettingsEvent() ========
 * This function is the entry point for SIP events that pertain to
 * call settings info reporting.
 *
 * Returns:
 *  SAPP_ERR: The SIP MESSAGE related event was not handled by this
 *            routine.  Further processing of this SIP event should continue.
 *  SAPP_OK: The SIP MESSAGE related event was handled by this
 *            routine and no further processing of this SIP event is needed.
 */
vint SAPP_callSettingsEvent(
    SAPP_ServiceObj   *service_ptr,
    tUaAppEvent       *sipEvt_ptr,
    SAPP_Event        *evt_ptr)
{
    char *contentType_ptr;
    int ret = SAPP_ERR;
    switch (sipEvt_ptr->header.type) {
        case eUA_TEXT_MESSAGE:
            if (NULL == (contentType_ptr =
                SAPP_parseHfValue(SAPP_CONTENT_TYPE_HF, sipEvt_ptr))) {
                /*
                 * Then there is no content-type, hence no "content" to process
                 * quietly ignore.
                 */
                break;
            }
            if (SAPP_OK == _SAPP_callSettingsDecode(
                    service_ptr, contentType_ptr,
                    sipEvt_ptr->msgBody.payLoad.data,
                    sipEvt_ptr->msgBody.payLoad.length,
                    &evt_ptr->isiMsg)) {
                ret = SAPP_OK;
            }
            break;
        default:
            break;
    } /* End of switch */ 
    return (ret);
}




