/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 28813 $ $Date: 2014-09-12 11:08:01 +0800 (Fri, 12 Sep 2014) $
 *
 */

#ifndef __TAPP_XML_H_
#define __TAPP_XML_H_


#define TAPP_XML_TYPE_ATTR              "type"
#define TAPP_XML_TIMEOUT_ATTR           "timeout"
#define TAPP_XML_NAME_ATTR              "name"
#define TAPP_XML_REPEAT_ATTR            "repeat"
#define TAPP_XML_ONFAIL_ATTR            "onfail"

#define TAPP_XML_ATTR_ISSUE_AT          "issue at"
#define TAPP_XML_ATTR_VALIDATE_AT       "validate at"
#define TAPP_XML_ATTR_ISSUE_GSM_AT      "issue gsm at"
#define TAPP_XML_ATTR_VALIDATE_GSM_AT   "validate gsm at"
#define TAPP_XML_ATTR_ISSUE_ISIP        "issue isip"
#define TAPP_XML_ATTR_VALIDATE_ISIP     "validate isip"
#define TAPP_XML_ATTR_ISSUE_CSM         "issue csm"
#define TAPP_XML_ATTR_PAUSE             "pause"
#define TAPP_XML_ATTR_CLEAN_ISIP        "clean isip"
#define TAPP_XML_ATTR_ISSUE_XCAP        "issue xcap"
#define TAPP_XML_ATTR_VALIDATE_XCAP     "validate xcap"
#define TAPP_XML_ATTR_ISSUE_ISI_RPC     "issue isi rpc"
#define TAPP_XML_ATTR_VALIDATE_ISI_RPC_RETURN  "validate isi rpc return"
#define TAPP_XML_ATTR_VALIDATE_ISI_GET_EVT     "validate isi get event"
#define TAPP_XML_ATTR_CLEAN_ISI_RPC_EVT     "clean isi rpc event"
#define TAPP_XML_ISIP_TAG               "isip"
#define TAPP_XML_CSM_TAG                "csm"
#define TAPP_XML_TYPE_TAG               "type"
#define TAPP_XML_REASON_TAG             "reason"
#define TAPP_XML_IP_TAG                 "ip"
#define TAPP_XML_INFC_TAG               "infc"
#define TAPP_XML_NETTYPE_TAG            "nettype"
#define TAPP_XML_CODE_TAG               "code"
#define TAPP_XML_PROTOCOL_TAG           "protocol"
#define TAPP_XML_ID_TAG                 "id"
#define TAPP_XML_SERVICE_TAG            "service"
#define TAPP_XML_SERVER_TAG             "server"
#define TAPP_XML_SERVER_TYPE_TAG        "serverType"
#define TAPP_XML_STATUS_TAG             "status"
#define TAPP_XML_ADDRESS_TAG            "address"
#define TAPP_XML_URI_TAG                "uri"
#define TAPP_XML_USERNAME_TAG           "username"
#define TAPP_XML_PASSWORD_TAG           "password"
#define TAPP_XML_REALM_TAG              "realm"
#define TAPP_XML_SYSTEM_TAG             "system"
#define TAPP_XML_CALL_TAG               "call"
#define TAPP_XML_XCAP_TAG               "xcap"
#define TAPP_XML_COMMAND_TAG            "command"
#define TAPP_XML_EVENT_TAG              "event"
#define TAPP_XML_ISI_RPC_TAG            "rpc"

#define TAPP_XML_AUDIODIR_TAG           "audioDirection"
#define TAPP_XML_VIDEODIR_TAG           "videoDirection"
#define TAPP_XML_RESOURCE_STATUS_TAG    "rsrcStatus"
#define TAPP_XML_TO_TAG                 "to"
#define TAPP_XML_FROM_TAG               "from"
#define TAPP_XML_CIDTYPE_TAG            "cidtype"
#define TAPP_XML_SUBJECT_TAG            "subject"
#define TAPP_XML_EVENT_TAG              "event"
#define TAPP_XML_CALL_ID_TAG            "callId"
#define TAPP_XML_SERVICE_ID_TAG         "serviceId"
#define TAPP_XML_CALL_TRANSFER_ID       "transferTargetCallId"
#define TAPP_XML_STOP_TAG               "stop"
#define TAPP_XML_CONTINUE_TAG           "continue"
#define TAPP_XML_MESSAGE_TAG            "message"
#define TAPP_XML_USSD_TAG               "ussd"
#define TAPP_XML_PDULEN_TAG             "pduLen"
#define TAPP_XML_MESSAGE_ID_TAG         "messageId"
#define TAPP_XML_SERVICE_ID_TAG         "serviceId"
#define TAPP_XML_FAILOVER_TAG           "isEmergencyFailoverToCs"
#define TAPP_XML_EMGCY_REG_REQ_TAG      "isEmergencyRegRequired"
#define TAPP_XML_IMEIURI_TAG            "imeiUri"
#define TAPP_XML_FUNC_ATTR              "func"
#define TAPP_XML_PARAM_TAG              "param"
#define TAPP_XML_IS_DONT_CARE_ATTR      "dontcare"
#define TAPP_XML_SUPSRV_HFEXIST_TAG     "supsrvHfExist"
#define TAPP_XML_HISTORY_INFO_TAG       "historyInfo"
#define TAPP_XML_REASON_DESC_TAG        "reasonDesc"
#define TAPP_XML_PARTICIPANTS_TAG       "participants"
#define TAPP_XML_CALL_TYPE              "callType"

#define TAPP_XML_PRESENCE_TAG           "presence"
#define TAPP_XML_SHOW_TAG               "show"
#define TAPP_XML_REASON_DESC_TAG        "reasonDesc"


#define TAPP_XML_TEL_EVT_ARG0_TAG       "arg0"
#define TAPP_XML_TEL_EVT_ARG1_TAG       "arg1"

TAPP_Return _TAPP_xmlParseTestCase(
    TAPP_GlobalObj *global_ptr,
    const char     *folderPath,
    ezxml_t         xml_ptr);

#endif
