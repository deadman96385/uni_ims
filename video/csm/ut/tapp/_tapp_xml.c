/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 30186 $ $Date: 2014-12-03 12:04:17 +0800 (Wed, 03 Dec 2014) $
 *
 */

#include <osal.h>
#include <isi.h>
#include <isip.h>
#include <csm_event.h>
#include "../csm/_csm_isi_call.h"
#include <ezxml.h>
#include <xcap.h>
#include <isi_xdr.h>
#include "_tapp.h"
#include "_tapp_report.h"
#include "_tapp_xml.h"
#include "_tapp_xml_xcap.h"


TAPP_EnumObj _TAPP_DataTypeTable[] = {
    {TAPP_DATA_TYPE_INTEGER, "int"   },
    {TAPP_DATA_TYPE_STRING,  "string"},
    {TAPP_DATA_TYPE_HEX,     "hex"   },
    {TAPP_DATA_TYPE_ENUM,    "enum"  },
    {TAPP_DATA_TYPE_ADDR,    "addr"  },
    {TAPP_DATA_TYPE_XML_DOC, "xmlDoc"},
    {TAPP_DATA_TYPE_MEDIA_ATTR, "media"}
};

TAPP_EnumObj _TAPP_IsipCodeTable[] = {
    {ISIP_CODE_INVALID,    "ISIP_CODE_INVALID"    },
    {ISIP_CODE_SERVICE,    "ISIP_CODE_SERVICE"    },
    {ISIP_CODE_CALL,       "ISIP_CODE_CALL"       },
    {ISIP_CODE_MESSAGE,    "ISIP_CODE_MESSAGE"    },
    {ISIP_CODE_PRESENCE,   "ISIP_CODE_PRESENCE"   },
    {ISIP_CODE_TEL_EVENT,  "ISIP_CODE_TEL_EVENT"  },
    {ISIP_CODE_MEDIA,      "ISIP_CODE_MEDIA"      },
    {ISIP_CODE_SYSTEM,     "ISIP_CODE_SYSTEM"     },
    {ISIP_CODE_CHAT   ,    "ISIP_CODE_CHAT"       },
    {ISIP_CODE_FILE,       "ISIP_CODE_FILE"       },
    {ISIP_CODE_DIAGNOSTIC, "ISIP_CODE_DIAGNOSTIC" },
    {ISIP_CODE_USSD,       "ISIP_CODE_USSD"       }
};

TAPP_EnumObj _TAPP_IsipServiceReasonTable[] = {
    {ISIP_SERVICE_REASON_INVALID,    "ISIP_SERVICE_REASON_INVALID"},
    {ISIP_SERVICE_REASON_CREATE,     "ISIP_SERVICE_REASON_CREATE"},
    {ISIP_SERVICE_REASON_DESTROY,    "ISIP_SERVICE_REASON_DESTROY"},
    {ISIP_SERVICE_REASON_ACTIVATE,   "ISIP_SERVICE_REASON_ACTIVATE"},
    {ISIP_SERVICE_REASON_DEACTIVATE, "ISIP_SERVICE_REASON_DEACTIVATE"},
    {ISIP_SERVICE_REASON_HANDOFF,    "ISIP_SERVICE_REASON_HANDOFF"},
    {ISIP_SERVICE_REASON_BLOCKUSER,  "ISIP_SERVICE_REASON_BLOCKUSER"},
    {ISIP_SERVICE_REASON_IDENTITY,   "ISIP_SERVICE_REASON_IDENTITY"},
    {ISIP_SERVICE_REASON_SERVER,     "ISIP_SERVICE_REASON_SERVER"},
    {ISIP_SERVICE_REASON_CODERS,     "ISIP_SERVICE_REASON_CODERS"},
    {ISIP_SERVICE_REASON_AUTH,       "ISIP_SERVICE_REASON_AUTH"},
    {ISIP_SERVICE_REASON_URI,        "ISIP_SERVICE_REASON_URI"},
    {ISIP_SERVICE_REASON_BSID,       "ISIP_SERVICE_REASON_BSID"},
    {ISIP_SERVICE_REASON_NET,        "ISIP_SERVICE_REASON_NET"},
    {ISIP_SERVICE_REASON_FILE,       "ISIP_SERVICE_REASON_FILE"},
    {ISIP_SERVICE_REASON_AUTH_AKA_CHALLENGE,
            "ISIP_SERVICE_REASON_AUTH_AKA_CHALLENGE"},
    {ISIP_SERVICE_REASON_AUTH_AKA_RESPONSE,
            "ISIP_SERVICE_REASON_AUTH_AKA_RESPONSE"},
    {ISIP_SERVICE_REASON_CAPABILITIES,"ISIP_SERVICE_REASON_CAPABILITIES"},
    {ISIP_SERVICE_REASON_PORT,        "ISIP_SERVICE_REASON_PORT"},
    {ISIP_SERVICE_REASON_EMERGENCY,   "ISIP_SERVICE_REASON_EMERGENCY"},
    {ISIP_SERVICE_REASON_IMEI_URI,    "ISIP_SERVICE_REASON_IMEI_URI"},
    {ISIP_SERVICE_REASON_IPSEC,       "ISIP_SERVICE_REASON_IPSEC"},
    {ISIP_SERVICE_REASON_INSTANCEID, "ISIP_SERVICE_REASON_INSTANCEID"},
    {ISIP_SERVICE_REASON_SET_PROVISIONING_DATA,
            "ISIP_SERVICE_REASON_SET_PROVISIONING_DATA"}
};

TAPP_EnumObj _TAPP_IsipStatusTable[] = {
    {ISIP_STATUS_INVALID, "ISIP_STATUS_INVALID"},
    {ISIP_STATUS_TRYING,  "ISIP_STATUS_TRYING" },
    {ISIP_STATUS_DONE,    "ISIP_STATUS_DONE"   },
    {ISIP_STATUS_FAILED,  "ISIP_STATUS_FAILED" }
};

TAPP_EnumObj _TAPP_IsipCallReasonTable[] = {
    {ISIP_CALL_REASON_INVALID,      "ISIP_CALL_REASON_INVALID"},
    {ISIP_CALL_REASON_INITIATE,     "ISIP_CALL_REASON_INITIATE"},
    {ISIP_CALL_REASON_TERMINATE,    "ISIP_CALL_REASON_TERMINATE"},
    {ISIP_CALL_REASON_ACCEPT,       "ISIP_CALL_REASON_ACCEPT"},
    {ISIP_CALL_REASON_ACK,          "ISIP_CALL_REASON_ACK"},
    {ISIP_CALL_REASON_REJECT,       "ISIP_CALL_REASON_REJECT"},
    {ISIP_CALL_REASON_TRYING,       "ISIP_CALL_REASON_TRYING"},
    {ISIP_CALL_REASON_ERROR,        "ISIP_CALL_REASON_ERROR"},
    {ISIP_CALL_REASON_FAILED,       "ISIP_CALL_REASON_FAILED"},
    {ISIP_CALL_REASON_CREDENTIALS,  "ISIP_CALL_REASON_CREDENTIALS"},
    {ISIP_CALL_REASON_HOLD,         "ISIP_CALL_REASON_HOLD"},
    {ISIP_CALL_REASON_RESUME,       "ISIP_CALL_REASON_RESUME"},
    {ISIP_CALL_REASON_FORWARD,      "ISIP_CALL_REASON_FORWARD"},
    {ISIP_CALL_REASON_TRANSFER_PROGRESS, "ISIP_CALL_REASON_TRANSFER_PROGRESS"},
    {ISIP_CALL_REASON_TRANSFER_ATTEMPT,  "ISIP_CALL_REASON_TRANSFER_ATTEMPT"},
    {ISIP_CALL_REASON_TRANSFER_BLIND,    "ISIP_CALL_REASON_TRANSFER_BLIND"},
    {ISIP_CALL_REASON_TRANSFER_COMPLETED,"ISIP_CALL_REASON_TRANSFER_COMPLETED"},
    {ISIP_CALL_REASON_TRANSFER_FAILED,   "ISIP_CALL_REASON_TRANSFER_FAILED"},
    {ISIP_CALL_REASON_TRANSFER_ATTENDED, "ISIP_CALL_REASON_TRANSFER_ATTENDED"},
    {ISIP_CALL_REASON_TRANSFER_CONSULT,  "ISIP_CALL_REASON_TRANSFER_CONSULT"},
    {ISIP_CALL_REASON_MODIFY,            "ISIP_CALL_REASON_MODIFY"},
    {ISIP_CALL_REASON_CANCEL_MODIFY,     "ISIP_CALL_REASON_CANCEL_MODIFY"},
    {ISIP_CALL_REASON_ACCEPT_MODIFY,     "ISIP_CALL_REASON_ACCEPT_MODIFY"},
    {ISIP_CALL_REASON_REJECT_MODIFY,     "ISIP_CALL_REASON_REJECT_MODIFY"},
    {ISIP_CALL_REASON_VDX,               "ISIP_CALL_REASON_VDX"},
    {ISIP_CALL_REASON_HANDOFF,           "ISIP_CALL_REASON_HANDOFF"},
    {ISIP_CALL_REASON_BEING_FORWARDED,   "ISIP_CALL_REASON_BEING_FORWARDED"},
    {ISIP_CALL_REASON_HANDOFF_SUCCESS,   "ISIP_CALL_REASON_HANDOFF_SUCCESS"},
    {ISIP_CALL_REASON_HANDOFF_FAILED,    "ISIP_CALL_REASON_HANDOFF_FAILED"},
    {ISIP_CALL_REASON_ACCEPT_ACK,        "ISIP_CALL_REASON_ACCEPT_ACK"},
    {ISIP_CALL_REASON_CONF_KICK,         "ISIP_CALL_REASON_CONF_KICK"},
    {ISIP_CALL_REASON_RTP_INACTIVE_TMR_DISABLE,
            "ISIP_CALL_REASON_RTP_INACTIVE_TMR_DISABLE"},
    {ISIP_CALL_REASON_RTP_INACTIVE_TMR_ENABLE,
            "ISIP_CALL_REASON_RTP_INACTIVE_TMR_ENABLE"},
    {ISIP_CALL_REASON_TRANSFER_CONF,     "ISIP_CALL_REASON_TRANSFER_CONF"},
    {ISIP_CALL_REASON_INITIATE_CONF,     "ISIP_CALL_REASON_INITIATE_CONF"},
    {ISIP_CALL_REASON_VIDEO_REQUEST_KEY, "ISIP_CALL_REASON_VIDEO_REQUEST_KEY"},
    {ISIP_CALL_REASON_EARLY_MEDIA,       "ISIP_CALL_REASON_EARLY_MEDIA"},
    {ISIP_CALL_REASON_LAST,              "ISIP_CALL_REASON_LAST"},
};

TAPP_EnumObj _TAPP_IsipSessionCidTypeTable[] = {
    {ISI_SESSION_CID_TYPE_NONE,        "ISI_SESSION_CID_TYPE_NONE"},
    {ISI_SESSION_CID_TYPE_INVOCATION,  "ISI_SESSION_CID_TYPE_INVOCATION"},
    {ISI_SESSION_CID_TYPE_SUPPRESSION, "ISI_SESSION_CID_TYPE_SUPPRESSION"},
    {ISI_SESSION_CID_TYPE_USE_ALIAS,   "ISI_SESSION_CID_TYPE_USE_ALIAS"}
};

TAPP_EnumObj _TAPP_IsipSystemReasonTable[] = {
    {ISIP_SYSTEM_REASON_START,     "ISIP_SYSTEM_REASON_START"},
    {ISIP_SYSTEM_REASON_SHUTDOWN,  "ISIP_SYSTEM_REASON_SHUTDOWN" },
    {ISIP_SYSTEM_REASON_NOT_SUPPORTED, "ISIP_SYSTEM_REASON_NOT_SUPPORTED" }
};

TAPP_EnumObj _TAPP_IsipServerTypeTable[] = {
    {ISI_SERVER_TYPE_STUN,    "ISI_SERVER_TYPE_STUN"   },
    {ISI_SERVER_TYPE_PROXY,   "ISI_SERVER_TYPE_PROXY"  },
    {ISI_SERVER_TYPE_REGISTRAR,      "ISI_SERVER_TYPE_REGISTRAR"       },
    {ISI_SERVER_TYPE_OUTBOUND_PROXY, "ISI_SERVER_TYPE_OUTBOUND_PROXY"  },
    {ISI_SERVER_TYPE_RELAY,   "ISI_SERVER_TYPE_RELAY"   },
    {ISI_SERVER_TYPE_STORAGE, "ISI_SERVER_TYPE_STORAGE" },
    {ISI_SERVER_TYPE_CHAT,    "ISI_SERVER_TYPE_CHAT"    },
    {ISI_SERVER_TYPE_INVALID, "ISI_SERVER_TYPE_INVALID" },
};

TAPP_EnumObj _TAPP_IsipPortTypeTable[] = {
    {ISI_PORT_TYPE_SIP,      "ISI_PORT_TYPE_SIP"     },
    {ISI_PORT_TYPE_AUDIO,    "ISI_PORT_TYPE_AUDIO"   },
    {ISI_PORT_TYPE_VIDEO,    "ISI_PORT_TYPE_VIDEO"   },
    {ISI_PORT_TYPE_INVALID,  "ISI_PORT_TYPE_INVALID" }
};

TAPP_EnumObj _TAPP_IsipSessionTypeTable[] = {
    {ISI_SESSION_TYPE_NONE,         "ISI_SESSION_TYPE_NONE"   },
    {ISI_SESSION_TYPE_AUDIO,        "ISI_SESSION_TYPE_AUDIO"  },
    {ISI_SESSION_TYPE_VIDEO,        "ISI_SESSION_TYPE_VIDEO"  },
    {ISI_SESSION_TYPE_CHAT,         "ISI_SESSION_TYPE_CHAT"   },
    {ISI_SESSION_TYPE_EMERGENCY,    "ISI_SESSION_TYPE_EMERGENCY"   },
    {ISI_SESSION_TYPE_SECURITY_AUDIO,   "ISI_SESSION_TYPE_SECURITY_AUDIO" },
    {ISI_SESSION_TYPE_SECURITY_VIDEO,   "ISI_SESSION_TYPE_SECURITY_VIDEO" },
    {ISI_SESSION_TYPE_CONFERENCE,       "ISI_SESSION_TYPE_CONFERENCE"     },
    {ISI_SESSION_TYPE_SECURITY_CHAT,    "ISI_SESSION_TYPE_SECURITY_CHAT"  },
    {ISI_SESSION_TYPE_INVALID,          "ISI_SESSION_TYPE_INVALID"        }
};

TAPP_EnumObj _TAPP_IsipSessionDirTable[] = {
    {ISI_SESSION_DIR_NONE,     "ISI_SESSION_DIR_NONE"     },
    {ISI_SESSION_DIR_INACTIVE, "ISI_SESSION_DIR_INACTIVE" },
    {ISI_SESSION_DIR_RECV_ONLY,"ISI_SESSION_DIR_RECV_ONLY"},
    {ISI_SESSION_DIR_SEND_ONLY,"ISI_SESSION_DIR_SEND_ONLY"},
    {ISI_SESSION_DIR_SEND_RECV,"ISI_SESSION_DIR_SEND_RECV"},
    {ISI_SESSION_DIR_INVALID,  "ISI_SESSION_DIR_INVALID"  }
};

TAPP_EnumObj _TAPP_IsipResourceStatusTable[] = {
    {ISI_RESOURCE_STATUS_NOT_READY, "ISI_RESOURCE_STATUS_NOT_READY"},
    {ISI_RESOURCE_STATUS_REMOTE_READY, "ISI_RESOURCE_STATUS_REMOTE_READY" },
    {ISI_RESOURCE_STATUS_LOCAL_READY ,"ISI_RESOURCE_STATUS_LOCAL_READY"},
    {ISI_RESOURCE_STATUS_FAILURE, "ISI_RESOURCE_STATUS_FAILURE"}
};

TAPP_EnumObj _TAPP_IsipTelEventTable[] = {
    {ISI_TEL_EVENT_DTMF,         "ISI_TEL_EVENT_DTMF"},
    {ISI_TEL_EVENT_DTMF_OOB,     "ISI_TEL_EVENT_DTMF_OOB"},
    {ISI_TEL_EVENT_VOICEMAIL,    "ISI_TEL_EVENT_VOICEMAIL"},
    {ISI_TEL_EVENT_CALL_FORWARD, "ISI_TEL_EVENT_CALL_FORWARD"},
    {ISI_TEL_EVENT_FEATURE,      "ISI_TEL_EVENT_FEATURE"},
    {ISI_TEL_EVENT_FLASHHOOK,    "ISI_TEL_EVENT_FLASHHOOK"},
    {ISI_TEL_EVENT_RESERVED_1,   "ISI_TEL_EVENT_RESERVED_1"},
    {ISI_TEL_EVENT_RESERVED_2,   "ISI_TEL_EVENT_RESERVED_2"},
    {ISI_TEL_EVENT_RESERVED_3,   "ISI_TEL_EVENT_RESERVED_3"},
    {ISI_TEL_EVENT_RESERVED_4,   "ISI_TEL_EVENT_RESERVED_4"},
    {ISI_TEL_EVENT_RESERVED_5,   "ISI_TEL_EVENT_RESERVED_5"},
    {ISI_TEL_EVENT_SEND_USSD,    "ISI_TEL_EVENT_SEND_USSD"},
    {ISI_TEL_EVENT_GET_SERVICE_ATTIBUTE,  "ISI_TEL_EVENT_GET_SERVICE_ATTIBUTE"},
    {ISI_TEL_EVENT_DTMF_STRING,  "ISI_TEL_EVENT_DTMF_STRING"},
    {ISI_TEL_EVENT_DTMF_DETECT,  "ISI_TEL_EVENT_DTMF_DETECT"},
    {ISI_TEL_EVENT_INVALID,      "ISI_TEL_EVENT_INVALID"}
};

TAPP_EnumObj _TAPP_IsipTelEvtReasonTable[] = {
    {ISIP_TEL_EVENT_REASON_INVALID,     "ISIP_TEL_EVENT_REASON_INVALID"},
    {ISIP_TEL_EVENT_REASON_NEW,         "ISIP_TEL_EVENT_REASON_NEW"},
    {ISIP_TEL_EVENT_REASON_ERROR,       "ISIP_TEL_EVENT_REASON_ERROR"},
    {ISIP_TEL_EVENT_REASON_COMPLETE,    "ISIP_TEL_EVENT_REASON_COMPLETE"}
};

TAPP_EnumObj _TAPP_IsipTextReasonTable[] = {
    {ISIP_TEXT_REASON_INVALID,    "ISIP_TEXT_REASON_INVALID"},
    {ISIP_TEXT_REASON_NEW,        "ISIP_TEXT_REASON_NEW"},
    {ISIP_TEXT_REASON_ERROR,      "ISIP_TEXT_REASON_ERROR"},
    {ISIP_TEXT_REASON_COMPLETE,   "ISIP_TEXT_REASON_COMPLETE"},
    {ISIP_TEXT_REASON_REPORT,     "ISIP_TEXT_REASON_REPORT"},
    {ISIP_TEXT_REASON_COMPOSING_ACTIVE, "ISIP_TEXT_REASON_COMPOSING_ACTIVE"},
    {ISIP_TEXT_REASON_COMPOSING_IDLE,   "ISIP_TEXT_REASON_COMPOSING_IDLE"},
    {ISIP_TEXT_REASON_NEW_NO_EVENT,     "ISIP_TEXT_REASON_NEW_NO_EVENT"}
};

TAPP_EnumObj _TAPP_IsipUssdReasonTable[] = {
    {ISIP_USSD_REASON_INVALID,    "ISIP_USSD_REASON_INVALID"},
    {ISIP_USSD_REASON_SEND,       "ISIP_USSD_REASON_SEND"},
    {ISIP_USSD_REASON_REPLY,      "ISIP_USSD_REASON_REPLY"},
    {ISIP_USSD_REASON_REQUEST,    "ISIP_USSD_REASON_REQUEST"},
    {ISIP_USSD_REASON_NOTIFY,     "ISIP_USSD_REASON_NOTIFY"},
    {ISIP_USSD_REASON_DISCONNECT, "ISIP_USSD_REASON_DISCONNECT"},
    {ISIP_USSD_REASON_SEND_ERROR, "ISIP_USSD_REASON_SEND_ERROR"},
    {ISIP_USSD_REASON_SEND_OK,    "ISIP_USSD_REASON_SEND_OK"}
};

TAPP_EnumObj _TAPP_IsipPresReasonTable[] = {
    {ISIP_PRES_REASON_INVALID,              "ISIP_PRES_REASON_INVALID"},
    {ISIP_PRES_REASON_PRESENCE,             "ISIP_PRES_REASON_PRESENCE"},
    {ISIP_PRES_REASON_CONTACT_ADD,          "ISIP_PRES_REASON_CONTACT_ADD"},
    {ISIP_PRES_REASON_CONTACT_RM,           "ISIP_PRES_REASON_CONTACT_RM"},
    {ISIP_PRES_REASON_SUB_TO_PRES,          "ISIP_PRES_REASON_SUB_TO_PRES"},
    {ISIP_PRES_REASON_UNSUB_TO_PRES,        "ISIP_PRES_REASON_UNSUB_TO_PRES"},
    {ISIP_PRES_REASON_SUB_ALLOWED,          "ISIP_PRES_REASON_SUB_ALLOWED"},
    {ISIP_PRES_REASON_SUB_REFUSED,          "ISIP_PRES_REASON_SUB_REFUSED"},
    {ISIP_PRES_REASON_COMPLETE,             "ISIP_PRES_REASON_COMPLETE"},
    {ISIP_PRES_REASON_CAPABILITIES,         "ISIP_PRES_REASON_CAPABILITIES"},
    {ISIP_PRES_REASON_CAPABILITIES_REQUEST, "ISIP_PRES_REASON_CAPABILITIES_REQUEST"},
    {ISIP_PRES_REASON_ERROR,                "ISIP_PRES_REASON_ERROR"}
};

TAPP_EnumObj _TAPP_IsipMessageTypeTable[] = {
    {ISI_MSG_TYPE_TEXT,       "ISI_MSG_TYPE_TEXT"},
    {ISI_MSG_TYPE_PDU_3GPP,   "ISI_MSG_TYPE_PDU_3GPP"},
    {ISI_MSG_TYPE_PDU_3GPP2,  "ISI_MSG_TYPE_PDU_3GPP2"}
};

TAPP_EnumObj _TAPP_RpcIsiFuncTypeTable[] = {
    {ISI_INIT,                       "ISI_INIT"},
    {ISI_GET_VERSION,                "ISI_GET_VERSION"},
    {ISI_SHUTDOWN,                   "ISI_SHUTDOWN"},
    {ISI_GET_EVENT,                  "ISI_GET_EVENT"},
    {ISI_ALLOC_SERVICE,              "ISI_ALLOC_SERVICE"},
    {ISI_ACTIVATE_SERVICE,           "ISI_ACTIVATE_SERVICE"},
    {ISI_DEACTIVATE_SERVICE,         "ISI_DEACTIVATE_SERVICE"},
    {ISI_FREE_SERVICE,               "ISI_FREE_SERVICE"},
    {ISI_SERVICE_MAKE_CID_PRIVATE,   "ISI_SERVICE_MAKE_CID_PRIVATE"},
    {ISI_SERVICE_SET_BSID,           "ISI_SERVICE_SET_BSID"},
    {ISI_SERVICE_SET_IMEI_URI,       "ISI_SERVICE_SET_IMEI_URI"},
    {ISI_SERVICE_SET_INTERFACE,      "ISI_SERVICE_SET_INTERFACE"},
    {ISI_SERVICE_SET_URI,            "ISI_SERVICE_SET_URI"},
    {ISI_SERVICE_SET_CAPABILITIES,   "ISI_SERVICE_SET_CAPABILITIES"},
    {ISI_SERVICE_SET_SERVER,         "ISI_SERVICE_SET_SERVER"},
    {ISI_SERVICE_SET_PORT,           "ISI_SERVICE_SET_PORT"},
    {ISI_SERVICE_SET_IPSEC,          "ISI_SERVICE_SET_IPSEC"},
    {ISI_SERVICE_SET_FILE_PATH,      "ISI_SERVICE_SET_FILE_PATH"},
    {ISI_SERVICE_SET_CREDENTIALS,    "ISI_SERVICE_SET_CREDENTIALS"},
    {ISI_SERVICE_SET_EMERGENCY,      "ISI_SERVICE_SET_EMERGENCY"},
    {ISI_SERVICE_SET_DEVICEID,       "ISI_SERVICE_SET_DEVICEID"},
    {ISI_SERVICE_BLOCK_USER,         "ISI_SERVICE_BLOCK_USER"},
    {ISI_SERVICE_UNBLOCK_USER,       "ISI_SERVICE_UNBLOCK_USER"},
    {ISI_ADD_CODER_TO_SERVICE,       "ISI_ADD_CODER_TO_SERVICE"},
    {ISI_REMOVE_CODER_FROM_SERVICE,  "ISI_REMOVE_CODER_FROM_SERVICE"},
    {ISI_SERVICE_FORWARD_CALLS,      "ISI_SERVICE_FORWARD_CALLS"},
    {ISI_INITIATE_CALL,              "ISI_INITIATE_CALL"},
    {ISI_SEND_USSD,                  "ISI_SEND_USSD"},
    {ISI_TERMINATE_CALL,             "ISI_TERMINATE_CALL"},
    {ISI_MODIFY_CALL,                "ISI_MODIFY_CALL"},
    {ISI_ACCEPT_CALL_MODIFY,         "ISI_ACCEPT_CALL_MODIFY"},
    {ISI_REJECT_CALL_MODIFY,         "ISI_REJECT_CALL_MODIFY"},
    {ISI_GET_CALL_STATE,             "ISI_GET_CALL_STATE"},
    {ISI_ADD_CODER_TO_CALL,          "ISI_ADD_CODER_TO_CALL"},
    {ISI_REMOVE_CODER_FROM_CALL,     "ISI_REMOVE_CODER_FROM_CALL"},
    {ISI_GET_CODER_DESCRIPTION,      "ISI_GET_CODER_DESCRIPTION"},
    {ISI_GET_CALL_HANDOFF,           "ISI_GET_CALL_HANDOFF"},
    {ISI_GET_CALL_HEADER,            "ISI_GET_CALL_HEADER"},
    {ISI_GET_SUPSRV_HEADER,          "ISI_GET_SUPSRV_HEADER"},
    {ISI_GET_SUPSRV_HISTORY_INFO,    "ISI_GET_SUPSRV_HISTORY_INFO"},
    {ISI_SET_CALL_SESSION_DIRECTION, "ISI_SET_CALL_SESSION_DIRECTION"},
    {ISI_GET_CALL_SESSION_DIRECTION, "ISI_GET_CALL_SESSION_DIRECTION"},
    {ISI_SET_CALL_SESSION_TYPE,      "ISI_SET_CALL_SESSION_TYPE"},
    {ISI_GET_CALL_SESSION_TYPE,      "ISI_GET_CALL_SESSION_TYPE"},
    {ISI_ACKNOWLEDGE_CALL,           "ISI_ACKNOWLEDGE_CALL"},
    {ISI_HOLD_CALL,                  "ISI_HOLD_CALL"},
    {ISI_RESUME_CALL,                "ISI_RESUME_CALL"},
    {ISI_ACCEPT_CALL,                "ISI_ACCEPT_CALL"},
    {ISI_REJECT_CALL,                "ISI_REJECT_CALL"},
    {ISI_FORWARD_CALL,               "ISI_FORWARD_CALL "},
    {ISI_BLIND_TRANSFER_CALL,        "ISI_BLIND_TRANSFER_CALL"},
    {ISI_ATTENDED_TRANSFER_CALL,     "ISI_ATTENDED_TRANSFER_CALL"},
    {ISI_CONSULTATIVE_TRANSFER_CALL, "ISI_CONSULTATIVE_TRANSFER_CALL"},
    {ISI_GENERATE_TONE,              "ISI_GENERATE_TONE"},
    {ISI_STOP_TONE,                  "ISI_STOP_TONE"},
    {ISI_SEND_TELEVENT_TO_REMOTE,    "ISI_SEND_TELEVENT_TO_REMOTE,  "},
    {ISI_SEND_TELEVENT_STRING_TO_REMOTE, "ISI_SEND_TELEVENT_STRING_TO_REMOTE"},
    {ISI_GET_TELEVENT_FROM_REMOTE,   "ISI_GET_TELEVENT_FROM_REMOTE"},
    {ISI_GET_TELEVENT_RESPONSE,      "ISI_GET_TELEVENT_RESPONSE"},
    {ISI_START_CONF_CALL,            "ISI_START_CONF_CALL"},
    {ISI_ADD_CALL_TO_CONF,           "ISI_ADD_CALL_TO_CONF"},
    {ISI_REMOVE_CALL_FROM_CONF,      "ISI_REMOVE_CALL_FROM_CONF"},
    {ISI_SEND_MESSAGE,               "ISI_SEND_MESSAGE"},
    {ISI_SEND_MESSAGE_REPORT,        "ISI_SEND_MESSAGE_REPORT"},
    {ISI_READ_MESSAGE_REPORT,        "ISI_READ_MESSAGE_REPORT"},
    {ISI_GET_MESSAGE_HEADER,         "ISI_GET_MESSAGE_HEADER"},
    {ISI_READ_MESSAGE,               "ISI_READ_MESSAGE"},
    {ISI_SEND_FILE,                  "ISI_SEND_FILE"},
    {ISI_ACCEPT_FILE,                "ISI_ACCEPT_FILE"},
    {ISI_ACKNOWLEDGE_FILE,           "ISI_ACKNOWLEDGE_FILE"},
    {ISI_REJECT_FILE,                "ISI_REJECT_FILE"},
    {ISI_CANCEL_FILE,                "ISI_CANCEL_FILE"},
    {ISI_READ_FILE_PROGRESS,         "ISI_READ_FILE_PROGRESS"},
    {ISI_GET_FILE_HEADER,            "ISI_GET_FILE_HEADER"},
    {ISI_ADD_CONTACT,                "ISI_ADD_CONTACT"},
    {ISI_REMOVE_CONTACT,             "ISI_REMOVE_CONTACT"},
    {ISI_READ_CONTACT,               "ISI_READ_CONTACT"},
    {ISI_SUBSCRIBE_TO_PRESENCE,      "ISI_SUBSCRIBE_TO_PRESENCE"},
    {ISI_UNSUBSCRIBE_FROM_PRESENCE,  "ISI_UNSUBSCRIBE_FROM_PRESENCE"},
    {ISI_READ_SUBSCRIBE_TO_PRESENCE_REQUEST, "ISI_READ_SUBSCRIBE_TO_PRESENCE_REQUEST"},
    {ISI_READ_SUBSCRIPTION_TO_PRESENCE_RESPONSE, "ISI_READ_SUBSCRIPTION_TO_PRESENCE_RESPONSE"},
    {ISI_ALLOW_SUBSCRIPTION_TO_PRESENCE, "ISI_ALLOW_SUBSCRIPTION_TO_PRESENCE"},
    {ISI_DENY_SUBSCRIPTION_TO_PRESENCE, "ISI_DENY_SUBSCRIPTION_TO_PRESENCE"},
    {ISI_SEND_PRESENCE,              "ISI_SEND_PRESENCE"},
    {ISI_READ_PRESENCE,              "ISI_READ_PRESENCE"},
    {ISI_SEND_CAPABILITIES,          "ISI_SEND_CAPABILITIES"},
    {ISI_READ_CAPABILITIES,          "ISI_READ_CAPABILITIES"},
    {ISI_SET_CHAT_NICKNAME,          "ISI_SET_CHAT_NICKNAME"},
    {ISI_INITIATE_GROUP_CHAT,        "ISI_INITIATE_GROUP_CHAT"},
    {ISI_INITIATE_GROUP_CHAT_ADHOC,  "ISI_INITIATE_GROUP_CHAT_ADHOC"},
    {ISI_INITIATE_CHAT,              "ISI_INITIATE_CHAT"},
    {ISI_ACCEPT_CHAT,                "ISI_ACCEPT_CHAT"},
    {ISI_REJECT_CHAT,                "ISI_REJECT_CHAT"},
    {ISI_ACKNOWLEDGE_CHAT,           "ISI_ACKNOWLEDGE_CHAT"},
    {ISI_DISCONNECT_CHAT,            "ISI_DISCONNECT_CHAT"},
    {ISI_GET_CHAT_HEADER,            "ISI_GET_CHAT_HEADER"},
    {ISI_GET_GROUP_CHAT_HEADER,      "ISI_GET_GROUP_CHAT_HEADER"},
    {ISI_READ_GROUP_CHAT_PRESENCE,   "ISI_READ_GROUP_CHAT_PRESENCE"},
    {ISI_INVITE_GROUP_CHAT,          "ISI_INVITE_GROUP_CHAT"},
    {ISI_JOIN_GROUP_CHAT,            "ISI_JOIN_GROUP_CHAT"},
    {ISI_KICK_GROUP_CHAT,            "ISI_KICK_GROUP_CHAT"},
    {ISI_DESTROY_GROUP_CHAT,         "ISI_DESTROY_GROUP_CHAT"},
    {ISI_SEND_CHAT_MESSAGE,          "ISI_SEND_CHAT_MESSAGE,"},
    {ISI_COMPOSING_CHAT_MESSAGE,     "ISI_COMPOSING_CHAT_MESSAGE"},
    {ISI_SEND_CHAT_MESSAGE_REPORT,   "ISI_SEND_CHAT_MESSAGE_REPORT,"},
    {ISI_SEND_CHAT_FILE,             "ISI_SEND_CHAT_FILE"},
    {ISI_SEND_PRIVATE_GROUP_CHAT_MESSAGE, "ISI_SEND_PRIVATE_GROUP_CHAT_MESSAGE"},
    {ISI_SEND_GROUP_CHAT_PRESENCE,   "ISI_SEND_GROUP_CHAT_PRESENCE"},
    {ISI_GET_CHAT_STATE,             "ISI_GET_CHAT_STATE"},
    {ISI_MEDIA_CONTROL,              "ISI_MEDIA_CONTROL"},
    {ISI_GET_SERVICE_ATRIBUTE,       "ISI_GET_SERVICE_ATRIBUTE"},
    {ISI_SET_AKA_AUTH_RESP,          "ISI_SET_AKA_AUTH_RESP"},
    {ISI_GET_AKA_AUTH_CHALLENGE,     "ISI_GET_AKA_AUTH_CHALLENGE"},
    {ISI_SERVICE_GET_IPSEC,          "ISI_SERVICE_GET_IPSEC"},
    {ISI_DIAG_AUDIO_RECORD,          "ISI_DIAG_AUDIO_RECORD"},
    {ISI_DIAG_AUDIO_PLAY,            "ISI_DIAG_AUDIO_PLAY"},
    {ISI_GET_NEXT_SERVICE,           "ISI_GET_NEXT_SERVICE"},
    {ISI_SET_FEATURE,                "ISI_SET_FEATURE"},
    {ISI_SERVICE_SET_FEATURE,        "ISI_SERVICE_SET_FEATURE"},
    {ISI_SERVICE_GET_FEATURE,        "ISI_SERVICE_GET_FEATURE"},
    {ISI_SET_CALL_RESOURCE_STATUS,   "ISI_SET_CALL_RESOURCE_STATUS"},
    {ISI_GET_CALL_RESOURCE_STATUS,   "ISI_GET_CALL_RESOURCE_STATUS"},
    {ISI_GET_CALL_SRVCC_STATUS,      "ISI_GET_CALL_SRVCC_STATUS"},
    {ISI_READ_USSD,                  "ISI_READ_USSD"},
    {ISI_SET_PROVISIONING_DATA,      "ISI_SET_PROVISIONING_DATA"},
    {ISI_UPDATE_CALL_SESSION,        "ISI_UPDATE_CALL_SESSION"},
};

TAPP_EnumObj _TAPP_RpcIsiRetValTable[] = {
    {ISI_RETURN_OK,                       "ISI_RETURN_OK"},
    {ISI_RETURN_FAILED,                   "ISI_RETURN_FAILED"},
    {ISI_RETURN_TIMEOUT,                  "ISI_RETURN_TIMEOUT"},
    {ISI_RETURN_INVALID_CONF_ID,          "ISI_RETURN_INVALID_CONF_ID"},
    {ISI_RETURN_INVALID_CALL_ID,          "ISI_RETURN_INVALID_CALL_ID"},
    {ISI_RETURN_INVALID_TEL_EVENT_ID,     "ISI_RETURN_INVALID_TEL_EVENT_ID"},
    {ISI_RETURN_INVALID_SERVICE_ID,       "ISI_RETURN_INVALID_SERVICE_ID"},
    {ISI_RETURN_INVALID_PRESENCE_ID,      "ISI_RETURN_INVALID_PRESENCE_ID"},
    {ISI_RETURN_INVALID_MESSAGE_ID,       "ISI_RETURN_INVALID_MESSAGE_ID"},
    {ISI_RETURN_INVALID_COUNTRY,          "ISI_RETURN_INVALID_COUNTRY"},
    {ISI_RETURN_INVALID_PROTOCOL,         "ISI_RETURN_INVALID_PROTOCOL"},
    {ISI_RETURN_INVALID_CREDENTIALS,      "ISI_RETURN_INVALID_CREDENTIALS"},
    {ISI_RETURN_INVALID_SESSION_DIR,      "ISI_RETURN_INVALID_SESSION_DIR"},
    {ISI_RETURN_INVALID_SERVER_TYPE,      "ISI_RETURN_INVALID_SERVER_TYPE"},
    {ISI_RETURN_INVALID_ADDRESS,          "ISI_RETURN_INVALID_ADDRESS"},
    {ISI_RETURN_INVALID_TEL_EVENT,        "ISI_RETURN_INVALID_TEL_EVENT"},
    {ISI_RETURN_INVALID_CODER,            "ISI_RETURN_INVALID_CODER"},
    {ISI_RETURN_NOT_SUPPORTED,            "ISI_RETURN_NOT_SUPPORTED"},
    {ISI_RETURN_DONE,                     "ISI_RETURN_DONE"},
    {ISI_RETURN_SERVICE_ALREADY_ACTIVE,   "ISI_RETURN_SERVICE_ALREADY_ACTIVE"},
    {ISI_RETURN_SERVICE_NOT_ACTIVE,       "ISI_RETURN_SERVICE_NOT_ACTIVE"},
    {ISI_RETURN_SERVICE_BUSY,             "ISI_RETURN_SERVICE_BUSY"},
    {ISI_RETURN_NOT_INIT,                 "ISI_RETURN_NOT_INIT"},
    {ISI_RETURN_MUTEX_ERROR,              "ISI_RETURN_MUTEX_ERROR"},
    {ISI_RETURN_INVALID_TONE,             "ISI_RETURN_INVALID_TONE"},
    {ISI_RETURN_INVALID_CHAT_ID,          "ISI_RETURN_INVALID_CHAT_ID"},
    {ISI_RETURN_INVALID_FILE_ID,          "ISI_RETURN_INVALID_FILE_ID"},
    {ISI_RETURN_OK_4G_PLUS,               "ISI_RETURN_OK_4G_PLUS"},
    {ISI_RETURN_NO_MEM,                   "ISI_RETURN_NO_MEM"},
    {ISI_RETURN_LAST,                     "ISI_RETURN_LAST"}
};

TAPP_EnumObj _TAPP_NetworkAccessTypeTable[] = {
    {ISI_NETWORK_ACCESS_TYPE_NONE,           "ISI_NETWORK_ACCESS_TYPE_NONE"},
    {ISI_NETWORK_ACCESS_TYPE_3GPP_GERAN,     "ISI_NETWORK_ACCESS_TYPE_3GPP_GERAN"},
    {ISI_NETWORK_ACCESS_TYPE_3GPP_UTRAN_FDD, "ISI_NETWORK_ACCESS_TYPE_3GPP_UTRAN_FDD"},
    {ISI_NETWORK_ACCESS_TYPE_3GPP_UTRAN_TDD, "ISI_NETWORK_ACCESS_TYPE_3GPP_UTRAN_TDD"},
    {ISI_NETWORK_ACCESS_TYPE_3GPP_E_UTRAN_FDD, "ISI_NETWORK_ACCESS_TYPE_3GPP_E_UTRAN_FDD"},
    {ISI_NETWORK_ACCESS_TYPE_3GPP_E_UTRAN_TDD, "ISI_NETWORK_ACCESS_TYPE_3GPP_E_UTRAN_TDD"},
    {ISI_NETWORK_ACCESS_TYPE_IEEE_802_11,      "ISI_NETWORK_ACCESS_TYPE_IEEE_802_11"}
};

/* 
 * ======== _TAPP_hexCharToInt() ========
 * 
 * Private function to convert hex character to int
 *
 * Returns: 
 *    0: Failed to convert.
 *    Non-zero: converted int.
 */
static int _TAPP_hexCharToInt(
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
 * ========_TAPP_hexStringToBytes() ========
 * 
 * Private function to convert hex string to bytes
 *
 * Returns: 
 *    Converted bytes size.
 */
int _TAPP_hexStringToBytes(
    char          *in_ptr,
    unsigned char *out_ptr) {
    int sz, i;

    char a,b;
    
    sz = OSAL_strlen(in_ptr);

    for (i = 0 ; i < sz ; i += 2) {
        a = _TAPP_hexCharToInt(in_ptr[i]) << 4;
        b = _TAPP_hexCharToInt(in_ptr[i + 1]);
        out_ptr[i / 2] = a | b;
    }
    return (i / 2);
}

/*
 * ======== _TAPP_getIsipValueByTag() ========
 *
 * Private function to parse ISIP tag with data type.
 * If type = enum, need to give an enum table, size ,and default
 * vaule, otherwise give NULL.
 *
 * Returns:
 *  TAPP_PASS: parsed the value of tag done or can not find the tag.
 *  TAPP_FAIL: The type is different from XML file
 */
TAPP_Return _TAPP_getIsipValueByTag(
    ezxml_t         xmlisip_ptr,
    void           *res_ptr,
    TAPP_DataType   type,
    char           *tag_ptr,
    TAPP_EnumObj   *enumString_ptr,
    int             enumSize,
    int             enumDefault)
{
    ezxml_t  child_ptr;
    vint     index;
    char     xmlType;
    int      isFail = 0;
    OSAL_NetAddress *addr_ptr, addr;
    char      *value_ptr;
    char      *txt_ptr;

    /* parse tag */
    if (NULL == (child_ptr = ezxml_child(xmlisip_ptr, tag_ptr))) {
        /* Can not find tag */
        isFail = 1;
    }

    if (NULL != (value_ptr = (char*)ezxml_attr(child_ptr,
            TAPP_XML_TYPE_TAG))) {
        /* Got type, compare it */
        for (index =0; index < TAPP_DATA_TYPE_LAST; index++) {
            if (!OSAL_strcmp(_TAPP_DataTypeTable[index].string, value_ptr)) {
                xmlType = _TAPP_DataTypeTable[index].index;
                break;
            }
        }
        if (type != xmlType) {
            /* Type not match */
            return (TAPP_FAIL);
        }
    }

    switch (type) {
        case TAPP_DATA_TYPE_INTEGER:
            if (!isFail) {
                *(int *)res_ptr = atoi(ezxml_txt(child_ptr));
            }
            else {
                *(int *)res_ptr = 0;
            }
            break;
        case TAPP_DATA_TYPE_STRING:
            if (!isFail) {
                OSAL_strncpy((char*)res_ptr, ezxml_txt(child_ptr),
                        TAPP_MOCK_XCAP_BODY_SZ);
            }
            else {
                OSAL_strncpy((char*)res_ptr, "", TAPP_AT_COMMAND_STRING_SZ);
            }
            break;
        case TAPP_DATA_TYPE_HEX:
            if (!isFail) {
                _TAPP_hexStringToBytes((char*)ezxml_txt(child_ptr),
                        (unsigned char*)res_ptr);
            }
            else {
                OSAL_strncpy((char*)res_ptr, "", TAPP_AT_COMMAND_STRING_SZ);
            }
            break;
        case TAPP_DATA_TYPE_ENUM:
            if (isFail) {
                *(int *)res_ptr = enumDefault;
                break;
            }
            while (!isFail) {
                txt_ptr = ezxml_txt(child_ptr);
                if (NULL != enumString_ptr) {
                    for (index = 0; index < enumSize; index++) {
                        if (!OSAL_strcmp(enumString_ptr[index].string,
                                txt_ptr)) {
                            *(int *)res_ptr += enumString_ptr[index].index;
                            /* Find if there is another one */
                            if (NULL == (child_ptr = ezxml_next(child_ptr))) {
                                isFail = 1;
                            }
                            break;
                        }
                    }
                    if (index >= enumSize) {
                        /* Enum incorrect */
                        TAPP_dbgPrintf("Invalid enum:%s index:%d, enumSize:%d\n",
                                txt_ptr, index, enumSize);
                        return (TAPP_FAIL);
                    }
                }
            }
            break;
        case TAPP_DATA_TYPE_ADDR:
            addr_ptr = (OSAL_NetAddress *)res_ptr;
            if (OSAL_FAIL == OSAL_netStringToAddress(ezxml_txt(child_ptr),
                    &addr)) {
                OSAL_netAddrNtoh(addr_ptr, addr_ptr);
                addr_ptr->ipv4 = 0;
            }
            else {
                /* CSM send ip to SAPP by host byte order */
                OSAL_netAddrNtoh(addr_ptr, &addr);
            }
            break;
        default:
            break;
    }

    return (TAPP_PASS);
}

/*
 * ======== _TAPP_paserIsipTelEvent() ========
 *
 * Private function to parse isip with event tag.
 *
 * Returns:
 *  TAPP_PASS: Tag parsing done.
 *  TAPP_FAIL: Failed to parse tag.
 */
TAPP_Return _TAPP_paserIsipTelEvent(
    ezxml_t         xmlIsip_ptr,
    ISIP_TelEvent  *telEvt_ptr)
{
    if (TAPP_PASS != _TAPP_getIsipValueByTag(xmlIsip_ptr,&telEvt_ptr->reason,
            TAPP_DATA_TYPE_ENUM, TAPP_XML_REASON_TAG,
            _TAPP_IsipTelEvtReasonTable, 
            sizeof(_TAPP_IsipTelEvtReasonTable)/sizeof(TAPP_EnumObj),
            ISIP_TEL_EVENT_REASON_INVALID)) {
        return (TAPP_FAIL);
    }
    if (TAPP_PASS != _TAPP_getIsipValueByTag(xmlIsip_ptr,&telEvt_ptr->evt,
            TAPP_DATA_TYPE_ENUM, TAPP_XML_EVENT_TAG,
            _TAPP_IsipTelEventTable, ISI_TEL_EVENT_INVALID,
            ISI_TEL_EVENT_INVALID)) {
        return (TAPP_FAIL);
    }
    if (TAPP_PASS != _TAPP_getIsipValueByTag(xmlIsip_ptr, &telEvt_ptr->callId,
            TAPP_DATA_TYPE_INTEGER, TAPP_XML_CALL_ID_TAG, NULL, 0, 0)) {
        return (TAPP_FAIL);
    }
    _TAPP_getIsipValueByTag(xmlIsip_ptr, &telEvt_ptr->settings.args.arg0,
            TAPP_DATA_TYPE_STRING, TAPP_XML_TEL_EVT_ARG0_TAG, NULL, 0, 0);
    _TAPP_getIsipValueByTag(xmlIsip_ptr, &telEvt_ptr->settings.args.arg1,
            TAPP_DATA_TYPE_STRING, TAPP_XML_TEL_EVT_ARG1_TAG, NULL, 0, 0);
    return (TAPP_PASS);
}

/*
 * ======== _TAPP_paserIsipText() ========
 *
 * Private function to parse isip with message tag.
 *
 * Returns:
 *  TAPP_PASS: Tag parsing done.
 *  TAPP_FAIL: Failed to parse tag.
 */
TAPP_Return _TAPP_paserIsipText(
    ezxml_t         xmlIsip_ptr,
    ISIP_Text      *text_ptr)
{
    /* Get Message reason */
    if (TAPP_PASS != _TAPP_getIsipValueByTag(xmlIsip_ptr,&text_ptr->reason,
            TAPP_DATA_TYPE_ENUM, TAPP_XML_REASON_TAG,
            _TAPP_IsipTextReasonTable, 
            sizeof(_TAPP_IsipTextReasonTable) / sizeof(TAPP_EnumObj),
            ISIP_TEXT_REASON_INVALID)) {
        return (TAPP_FAIL);
    }
    /* Get status */
    if (TAPP_PASS != _TAPP_getIsipValueByTag(xmlIsip_ptr,&text_ptr->type,
            TAPP_DATA_TYPE_ENUM, TAPP_XML_TYPE_TAG,
            _TAPP_IsipMessageTypeTable,
            sizeof(_TAPP_IsipMessageTypeTable) / sizeof(TAPP_EnumObj),
            ISI_MSG_TYPE_PDU_3GPP)) {
        return (TAPP_FAIL);
    }
    /* Get message content */
    if (TAPP_PASS != _TAPP_getIsipValueByTag(xmlIsip_ptr,&text_ptr->message,
            TAPP_DATA_TYPE_HEX, TAPP_XML_MESSAGE_TAG,
            NULL, 0,0)) {
        return (TAPP_FAIL);
    }
    /* Get message Id */
    if (TAPP_PASS != _TAPP_getIsipValueByTag(xmlIsip_ptr,
            &text_ptr->messageId,
            TAPP_DATA_TYPE_STRING,
            TAPP_XML_MESSAGE_ID_TAG, NULL, 0, 0)) {
        return (TAPP_FAIL);
    }
    /* Get pduLen */
    if (TAPP_PASS != _TAPP_getIsipValueByTag(xmlIsip_ptr,
            &text_ptr->pduLen,
            TAPP_DATA_TYPE_INTEGER,
            TAPP_XML_PDULEN_TAG, NULL, 0, 0)) {
        return (TAPP_FAIL);
    }
    /* Get to */
    if (TAPP_PASS != _TAPP_getIsipValueByTag(xmlIsip_ptr,
            &text_ptr->to,
            TAPP_DATA_TYPE_STRING,
            TAPP_XML_TO_TAG, NULL, 0, 0)) {
        return (TAPP_FAIL);
    }
    /* Get from */
    if (TAPP_PASS != _TAPP_getIsipValueByTag(xmlIsip_ptr,
            &text_ptr->from,
            TAPP_DATA_TYPE_STRING,
            TAPP_XML_FROM_TAG, NULL, 0, 0)) {
        return (TAPP_FAIL);
    }
    /* Get serviceId */
    if (TAPP_PASS != _TAPP_getIsipValueByTag(xmlIsip_ptr, &text_ptr->serviceId,
            TAPP_DATA_TYPE_INTEGER, TAPP_XML_SERVICE_ID_TAG, NULL, 0, 0)) {
        return (TAPP_FAIL);
    }
    /* Get serviceId */
    if (TAPP_PASS != _TAPP_getIsipValueByTag(xmlIsip_ptr, &text_ptr->serviceId,
            TAPP_DATA_TYPE_INTEGER, TAPP_XML_SERVICE_ID_TAG, NULL, 0, 0)) {
        return (TAPP_FAIL);
    }
    /* Get reason Description */
    if (TAPP_PASS != _TAPP_getIsipValueByTag(xmlIsip_ptr,
            text_ptr->reasonDesc, TAPP_DATA_TYPE_STRING,
            TAPP_XML_REASON_DESC_TAG, NULL, 0, 0)) {
        return (TAPP_FAIL);
    }

    return (TAPP_PASS);
}
/*
 * ======== _TAPP_paserIsipUssd() ========
 *
 * Private function to parse isip with ussd tag.
 *
 * Returns:
 *  TAPP_PASS: Tag parsing done.
 *  TAPP_FAIL: Failed to parse tag.
 */
TAPP_Return _TAPP_paserIsipUssd(
    ezxml_t         xmlIsip_ptr,
    ISIP_Ussd      *ussd_ptr)
{
    /* Get Message reason */
    if (TAPP_PASS != _TAPP_getIsipValueByTag(xmlIsip_ptr,&ussd_ptr->reason,
            TAPP_DATA_TYPE_ENUM, TAPP_XML_REASON_TAG,
            _TAPP_IsipUssdReasonTable, 
            sizeof(_TAPP_IsipUssdReasonTable) / sizeof(TAPP_EnumObj),
            ISIP_USSD_REASON_INVALID)) {
        return (TAPP_FAIL);
    }
    
    /* Get message content */
    if (TAPP_PASS != _TAPP_getIsipValueByTag(xmlIsip_ptr,&ussd_ptr->message,
            TAPP_DATA_TYPE_HEX, TAPP_XML_MESSAGE_TAG,
            NULL, 0,0)) {
        return (TAPP_FAIL);
    }
    /* Get to */
    if (TAPP_PASS != _TAPP_getIsipValueByTag(xmlIsip_ptr,
            &ussd_ptr->to,
            TAPP_DATA_TYPE_STRING,
            TAPP_XML_TO_TAG, NULL, 0, 0)) {
        return (TAPP_FAIL);
    }
    /* Get serviceId */
    if (TAPP_PASS != _TAPP_getIsipValueByTag(xmlIsip_ptr, &ussd_ptr->serviceId,
            TAPP_DATA_TYPE_INTEGER, TAPP_XML_SERVICE_ID_TAG, NULL, 0, 0)) {
        return (TAPP_FAIL);
    }

    return (TAPP_PASS);
}

/*
 * ======== _TAPP_paserIsipCall() ========
 *
 * Private function to parse isip with call tag.
 *
 * Returns:
 *  TAPP_PASS: Tag parsing done.
 *  TAPP_FAIL: Failed to parse tag.
 */
TAPP_Return _TAPP_paserIsipCall(
    ezxml_t       xmlIsip_ptr,
    ISIP_Call    *call_ptr)
{
    if (TAPP_PASS != _TAPP_getIsipValueByTag(xmlIsip_ptr, &call_ptr->reason,
            TAPP_DATA_TYPE_ENUM, TAPP_XML_REASON_TAG,
            _TAPP_IsipCallReasonTable, ISIP_CALL_REASON_LAST,
            ISIP_CALL_REASON_INVALID)) {
        return (TAPP_FAIL);
    }
    if (TAPP_PASS != _TAPP_getIsipValueByTag(xmlIsip_ptr, &call_ptr->status,
            TAPP_DATA_TYPE_ENUM, TAPP_XML_STATUS_TAG, _TAPP_IsipStatusTable,
            ISIP_STATUS_LAST, ISIP_STATUS_INVALID)) {
        return (TAPP_FAIL);
    }
    /* Get serviceId */
    if (TAPP_PASS != _TAPP_getIsipValueByTag(xmlIsip_ptr, &call_ptr->serviceId,
            TAPP_DATA_TYPE_INTEGER, TAPP_XML_SERVICE_ID_TAG, NULL, 0, 0)) {
        return (TAPP_FAIL);
    }
    if (TAPP_PASS != _TAPP_getIsipValueByTag(xmlIsip_ptr, &call_ptr->type,
            TAPP_DATA_TYPE_ENUM, TAPP_XML_TYPE_TAG,
            _TAPP_IsipSessionTypeTable,
            sizeof(_TAPP_IsipSessionTypeTable) / sizeof(TAPP_EnumObj),
            ISI_SESSION_TYPE_NONE)) {
        return (TAPP_FAIL);
    }
    if (TAPP_PASS != _TAPP_getIsipValueByTag(xmlIsip_ptr,
            &call_ptr->audioDirection,
            TAPP_DATA_TYPE_ENUM, TAPP_XML_AUDIODIR_TAG,
            _TAPP_IsipSessionDirTable,
            sizeof(_TAPP_IsipSessionDirTable) / sizeof(TAPP_EnumObj),
            ISI_SESSION_DIR_NONE)) {
        return (TAPP_FAIL);
    }
    if (TAPP_PASS != _TAPP_getIsipValueByTag(xmlIsip_ptr,
            &call_ptr->videoDirection,
            TAPP_DATA_TYPE_ENUM, TAPP_XML_VIDEODIR_TAG,
            _TAPP_IsipSessionDirTable,
            sizeof(_TAPP_IsipSessionDirTable) / sizeof(TAPP_EnumObj),
            ISI_SESSION_DIR_NONE)) {
        return (TAPP_FAIL);
    }
    /* If resource status is not given, set default to ready */
    if (TAPP_PASS != _TAPP_getIsipValueByTag(xmlIsip_ptr,
            &call_ptr->rsrcStatus,
            TAPP_DATA_TYPE_ENUM, TAPP_XML_RESOURCE_STATUS_TAG,
            _TAPP_IsipResourceStatusTable,
            sizeof(_TAPP_IsipResourceStatusTable) / sizeof(TAPP_EnumObj),
            ISI_RESOURCE_STATUS_LOCAL_READY |
            ISI_RESOURCE_STATUS_REMOTE_READY)) {
        return (TAPP_FAIL);
    }
    if (TAPP_PASS != _TAPP_getIsipValueByTag(xmlIsip_ptr,
            &call_ptr->to, TAPP_DATA_TYPE_STRING, TAPP_XML_TO_TAG,
            NULL, 0, 0)) {
        return (TAPP_FAIL);
    }   
    if (TAPP_PASS != _TAPP_getIsipValueByTag(xmlIsip_ptr,
            &call_ptr->subject, TAPP_DATA_TYPE_STRING, TAPP_XML_SUBJECT_TAG,
            NULL, 0, 0)) {
        return (TAPP_FAIL);
    }
    switch (call_ptr->reason) {
        case ISIP_CALL_REASON_INITIATE:
            if (TAPP_PASS != _TAPP_getIsipValueByTag(xmlIsip_ptr,
                    &call_ptr->from, TAPP_DATA_TYPE_STRING, TAPP_XML_FROM_TAG,
                    NULL, 0, 0)) {
                return (TAPP_FAIL);
            }
            if (TAPP_PASS != _TAPP_getIsipValueByTag(xmlIsip_ptr,
                    &call_ptr->cidType, TAPP_DATA_TYPE_ENUM, 
                    TAPP_XML_CIDTYPE_TAG, _TAPP_IsipSessionCidTypeTable, 
                    sizeof(_TAPP_IsipSessionCidTypeTable)/sizeof(TAPP_EnumObj), 
                    ISI_SESSION_CID_TYPE_NONE)) {
                return (TAPP_FAIL);
            }
            /* invite with history-info flag and history-info string */
            if (TAPP_PASS != _TAPP_getIsipValueByTag(xmlIsip_ptr,
                    &call_ptr->supsrvHfExist, TAPP_DATA_TYPE_INTEGER,
                    TAPP_XML_SUPSRV_HFEXIST_TAG, NULL, 0, 0)) {
                return (TAPP_FAIL);
            }
            if (TAPP_PASS != _TAPP_getIsipValueByTag(xmlIsip_ptr,
                    call_ptr->historyInfo, TAPP_DATA_TYPE_STRING,
                    TAPP_XML_HISTORY_INFO_TAG, NULL, 0, 0)) {
                return (TAPP_FAIL);
            }
            if (TAPP_PASS != _TAPP_getIsipValueByTag(xmlIsip_ptr,
                    call_ptr->reasonDesc, TAPP_DATA_TYPE_STRING,
                    TAPP_XML_REASON_DESC_TAG, NULL, 0, 0)) {
                return (TAPP_FAIL);
            }
            break;
        case ISIP_CALL_REASON_ACK:
            if (TAPP_PASS != _TAPP_getIsipValueByTag(xmlIsip_ptr,
                    &call_ptr->from, TAPP_DATA_TYPE_STRING, TAPP_XML_FROM_TAG,
                    NULL, 0, 0)) {
                return (TAPP_FAIL);
            }
            /* alert-info-cw header supsrvHfExist */
            if (TAPP_PASS != _TAPP_getIsipValueByTag(xmlIsip_ptr,
                    &call_ptr->supsrvHfExist, TAPP_DATA_TYPE_INTEGER,
                    TAPP_XML_SUPSRV_HFEXIST_TAG, NULL, 0, 0)) {
                return (TAPP_FAIL);
            }
            break;
        case ISIP_CALL_REASON_BEING_FORWARDED:
            if (TAPP_PASS != _TAPP_getIsipValueByTag(xmlIsip_ptr,
                    &call_ptr->from, TAPP_DATA_TYPE_STRING, TAPP_XML_FROM_TAG,
                    NULL, 0, 0)) {
                return (TAPP_FAIL);
            }
            break;
        case ISIP_CALL_REASON_MODIFY:
            if (TAPP_PASS != _TAPP_getIsipValueByTag(xmlIsip_ptr,
                    &call_ptr->from, TAPP_DATA_TYPE_STRING, TAPP_XML_FROM_TAG,
                    NULL, 0, 0)) {
                return (TAPP_FAIL);
            }
            if (TAPP_PASS != _TAPP_getIsipValueByTag(xmlIsip_ptr,
                    &call_ptr->rmtAudioAddr, TAPP_DATA_TYPE_ADDR,
                    "rmtAudioAddr",
                    NULL, 0, 0)) {
                return (TAPP_FAIL);
            }
            if (TAPP_PASS != _TAPP_getIsipValueByTag(xmlIsip_ptr,
                    &call_ptr->rmtAudioCntlPort, TAPP_DATA_TYPE_INTEGER,
                    "rmtAudioCntlPort",
                    NULL, 0, 0)) {
                return (TAPP_FAIL);
            }
            break;
        case ISIP_CALL_REASON_ACCEPT:
            if (TAPP_PASS != _TAPP_getIsipValueByTag(xmlIsip_ptr,
                    &call_ptr->rmtAudioAddr, TAPP_DATA_TYPE_ADDR,
                    "rmtAudioAddr",
                    NULL, 0, 0)) {
                return (TAPP_FAIL);
            }
            if (TAPP_PASS != _TAPP_getIsipValueByTag(xmlIsip_ptr,
                    &call_ptr->rmtAudioCntlPort, TAPP_DATA_TYPE_INTEGER,
                    "rmtAudioCntlPort",
                    NULL, 0, 0)) {
                return (TAPP_FAIL);
            }
            break;    
        case ISIP_CALL_REASON_ACCEPT_ACK:
            break;    
        case ISIP_CALL_REASON_FAILED:
            if (TAPP_PASS != _TAPP_getIsipValueByTag(xmlIsip_ptr,
                    call_ptr->reasonDesc, TAPP_DATA_TYPE_STRING,
                    TAPP_XML_REASON_DESC_TAG, NULL, 0, 0)) {
                return (TAPP_FAIL);
            }
            break;
        case ISIP_CALL_REASON_REJECT:
            if (TAPP_PASS != _TAPP_getIsipValueByTag(xmlIsip_ptr,
                    call_ptr->reasonDesc, TAPP_DATA_TYPE_STRING,
                    TAPP_XML_REASON_DESC_TAG, NULL, 0, 0)) {
                return (TAPP_FAIL);
            }
            break;
        case ISIP_CALL_REASON_TRANSFER_CONSULT:
            if (TAPP_PASS != _TAPP_getIsipValueByTag(xmlIsip_ptr,
                    &call_ptr->transferTargetCallId,
                    TAPP_DATA_TYPE_INTEGER, TAPP_XML_CALL_TRANSFER_ID,
                    NULL, 0, 0)) {
                return (TAPP_FAIL);
            }
            break;
        case ISIP_CALL_REASON_TRANSFER_CONF:
            if (TAPP_PASS != _TAPP_getIsipValueByTag(xmlIsip_ptr,
                    &call_ptr->to,
                    TAPP_DATA_TYPE_STRING, TAPP_XML_TO_TAG,
                    NULL, 0, 0)) {
                return (TAPP_FAIL);
            }
            if (TAPP_PASS != _TAPP_getIsipValueByTag(xmlIsip_ptr,
                    &call_ptr->participants,
                    TAPP_DATA_TYPE_STRING, TAPP_XML_PARTICIPANTS_TAG,
                    NULL, 0, 0)) {
                return (TAPP_FAIL);
            }
            break;
        case ISIP_CALL_REASON_INITIATE_CONF:
             if (TAPP_PASS != _TAPP_getIsipValueByTag(xmlIsip_ptr,
                    &call_ptr->from, TAPP_DATA_TYPE_STRING, 
                    TAPP_XML_FROM_TAG, NULL, 0, 0)) {
                return (TAPP_FAIL);
            }
            if (TAPP_PASS != _TAPP_getIsipValueByTag(xmlIsip_ptr,
                    &call_ptr->cidType, TAPP_DATA_TYPE_ENUM, 
                    TAPP_XML_CIDTYPE_TAG, _TAPP_IsipSessionCidTypeTable, 
                    sizeof(_TAPP_IsipSessionCidTypeTable)/sizeof(TAPP_EnumObj), 
                    ISI_SESSION_CID_TYPE_NONE)) {
                return (TAPP_FAIL);
            }
            /* invite with history-info flag and history-info string */
            if (TAPP_PASS != _TAPP_getIsipValueByTag(xmlIsip_ptr,
                    &call_ptr->supsrvHfExist, TAPP_DATA_TYPE_INTEGER,
                    TAPP_XML_SUPSRV_HFEXIST_TAG, NULL, 0, 0)) {
                return (TAPP_FAIL);
            }
            if (TAPP_PASS != _TAPP_getIsipValueByTag(xmlIsip_ptr,
                    call_ptr->historyInfo, TAPP_DATA_TYPE_STRING,
                    TAPP_XML_HISTORY_INFO_TAG, NULL, 0, 0)) {
                return (TAPP_FAIL);
            }
            if (TAPP_PASS != _TAPP_getIsipValueByTag(xmlIsip_ptr,
                    call_ptr->reasonDesc, TAPP_DATA_TYPE_STRING,
                    TAPP_XML_REASON_DESC_TAG, NULL, 0, 0)) {
                return (TAPP_FAIL);
            }
            if (TAPP_PASS != _TAPP_getIsipValueByTag(xmlIsip_ptr,
                    &call_ptr->participants, TAPP_DATA_TYPE_STRING, 
                    TAPP_XML_PARTICIPANTS_TAG, NULL, 0, 0)) {
                return (TAPP_FAIL);
            }
            break;
        default:
            break;        
    }
    return (TAPP_PASS);
}

/*
 * ======== _TAPP_paserIsipService() ========
 *
 * Private function to parse isip with service tag.
 *
 * Returns:
 *  TAPP_PASS: Tag parsing done.
 *  TAPP_FAIL: Failed to parse tag.
 */
TAPP_Return _TAPP_paserIsipService(
    ezxml_t       xmlIsip_ptr,
    ISIP_Service *service_ptr)
{
    if (TAPP_PASS != _TAPP_getIsipValueByTag(xmlIsip_ptr,&service_ptr->reason,
            TAPP_DATA_TYPE_ENUM, TAPP_XML_REASON_TAG,
            _TAPP_IsipServiceReasonTable, ISIP_SERVICE_REASON_LAST,
            ISIP_SERVICE_REASON_INVALID)) {
        return (TAPP_FAIL);
    }
    if (TAPP_PASS != _TAPP_getIsipValueByTag(xmlIsip_ptr,&service_ptr->status,
            TAPP_DATA_TYPE_ENUM, TAPP_XML_STATUS_TAG,
            _TAPP_IsipStatusTable, ISIP_STATUS_LAST, ISIP_STATUS_INVALID)) {
        return (TAPP_FAIL);
    }
    if (TAPP_PASS != _TAPP_getIsipValueByTag(xmlIsip_ptr,&service_ptr->server,
            TAPP_DATA_TYPE_ENUM, TAPP_XML_SERVER_TYPE_TAG,
            _TAPP_IsipServerTypeTable,
            sizeof(_TAPP_IsipServerTypeTable)/sizeof(TAPP_EnumObj),
            ISI_SERVER_TYPE_INVALID)) {
        return (TAPP_FAIL);
    }
    switch (service_ptr->reason) {
        case ISIP_SERVICE_REASON_CREATE:           
            if (TAPP_PASS != _TAPP_getIsipValueByTag(xmlIsip_ptr,
                    &service_ptr->settings.country,
                    TAPP_DATA_TYPE_STRING, "country", NULL, 0, 0)) {
                return (TAPP_FAIL);
            }         
            break;
        case ISIP_SERVICE_REASON_NET:
            if (TAPP_PASS != _TAPP_getIsipValueByTag(xmlIsip_ptr,
                    &service_ptr->settings.interface.name,
                    TAPP_DATA_TYPE_STRING, "name", NULL, 0, 0)) {
                return (TAPP_FAIL);
            }
            if (TAPP_PASS != _TAPP_getIsipValueByTag(xmlIsip_ptr,
                    &service_ptr->settings.interface.address,
                    TAPP_DATA_TYPE_ADDR, TAPP_XML_ADDRESS_TAG, NULL, 0, 0)) {
                return (TAPP_FAIL);
            }
            break;
        case ISIP_SERVICE_REASON_BLOCKUSER:
            if (TAPP_PASS != _TAPP_getIsipValueByTag(xmlIsip_ptr,
                    &service_ptr->settings.uri,
                    TAPP_DATA_TYPE_STRING, TAPP_XML_URI_TAG, NULL, 0, 0)) {
                return (TAPP_FAIL);
            }
            break;
        case ISIP_SERVICE_REASON_IDENTITY:
             if (TAPP_PASS != _TAPP_getIsipValueByTag(xmlIsip_ptr,
                    &service_ptr->settings.identityHide,
                    TAPP_DATA_TYPE_INTEGER, "identityHide", NULL, 0, 0)) {
                return (TAPP_FAIL);
            }
            break;
        case ISIP_SERVICE_REASON_SERVER:
            if (TAPP_PASS != _TAPP_getIsipValueByTag(xmlIsip_ptr,
                    &service_ptr->settings.server,
                    TAPP_DATA_TYPE_STRING, TAPP_XML_SERVER_TAG, NULL, 0, 0)) {
                return (TAPP_FAIL);
            }
            break;
        case ISIP_SERVICE_REASON_CODERS:
            /* Add parsing coders if needed. */
            break;
        case ISIP_SERVICE_REASON_AUTH:
            if (TAPP_PASS != _TAPP_getIsipValueByTag(xmlIsip_ptr,
                    &service_ptr->settings.credentials.username,
                    TAPP_DATA_TYPE_STRING, TAPP_XML_USERNAME_TAG, NULL, 0, 0)) {
                return (TAPP_FAIL);
            }
            if (TAPP_PASS != _TAPP_getIsipValueByTag(xmlIsip_ptr,
                    &service_ptr->settings.credentials.password,
                    TAPP_DATA_TYPE_STRING, TAPP_XML_PASSWORD_TAG, NULL, 0, 0)) {
                return (TAPP_FAIL);
            }
            if (TAPP_PASS != _TAPP_getIsipValueByTag(xmlIsip_ptr,
                    &service_ptr->settings.credentials.realm,
                    TAPP_DATA_TYPE_STRING, TAPP_XML_REALM_TAG, NULL, 0, 0)) {
                return (TAPP_FAIL);
            }
            break;
        case ISIP_SERVICE_REASON_URI:
            if (TAPP_PASS != _TAPP_getIsipValueByTag(xmlIsip_ptr,
                    &service_ptr->settings.uri,
                    TAPP_DATA_TYPE_STRING, TAPP_XML_URI_TAG, NULL, 0, 0)) {
                return (TAPP_FAIL);
            }
            break;
        case ISIP_SERVICE_REASON_BSID:
            if (TAPP_PASS != _TAPP_getIsipValueByTag(xmlIsip_ptr,
                    &service_ptr->settings.bsId.type,
                    TAPP_DATA_TYPE_ENUM, "bsid-type",
                    _TAPP_NetworkAccessTypeTable, 4,
                    ISI_NETWORK_ACCESS_TYPE_NONE)) {
                return (TAPP_FAIL);
            }
            if (TAPP_PASS != _TAPP_getIsipValueByTag(xmlIsip_ptr,
                    &service_ptr->settings.bsId.szBsId,
                    TAPP_DATA_TYPE_STRING, "bsid", NULL, 0, 0)) {
                return (TAPP_FAIL);
            }
            break;          
        case ISIP_SERVICE_REASON_FILE:
            if (TAPP_PASS != _TAPP_getIsipValueByTag(xmlIsip_ptr,
                    &service_ptr->settings.file.filePath,
                    TAPP_DATA_TYPE_STRING, "filePath", NULL, 0, 0)) {
                return (TAPP_FAIL);
            }
            if (TAPP_PASS != _TAPP_getIsipValueByTag(xmlIsip_ptr,
                    &service_ptr->settings.file.filePrepend,
                    TAPP_DATA_TYPE_STRING, "filePrepend", NULL, 0, 0)) {
                return (TAPP_FAIL);
            }
            break;
        case ISIP_SERVICE_REASON_AUTH_AKA_RESPONSE:
            if (TAPP_PASS != _TAPP_getIsipValueByTag(xmlIsip_ptr,
                    &service_ptr->settings.akaAuthResp.resp,
                    TAPP_DATA_TYPE_HEX, "akaAuthResp", NULL, 0, 0)) {
                return (TAPP_FAIL);
            }
            if (TAPP_PASS != _TAPP_getIsipValueByTag(xmlIsip_ptr,
                    &service_ptr->settings.akaAuthResp.ck,
                    TAPP_DATA_TYPE_HEX, "ck", NULL, 0, 0)) {
                return (TAPP_FAIL);
            }
            if (TAPP_PASS != _TAPP_getIsipValueByTag(xmlIsip_ptr,
                    &service_ptr->settings.akaAuthResp.ik,
                    TAPP_DATA_TYPE_HEX, "ik", NULL, 0, 0)) {
                return (TAPP_FAIL);
            }
            if (TAPP_PASS != _TAPP_getIsipValueByTag(xmlIsip_ptr,
                    &service_ptr->settings.akaAuthChallenge.rand,
                    TAPP_DATA_TYPE_HEX, "rand", NULL, 0, 0)) {
                return (TAPP_FAIL);
            }
            if (TAPP_PASS != _TAPP_getIsipValueByTag(xmlIsip_ptr,
                    &service_ptr->settings.akaAuthChallenge.autn,
                    TAPP_DATA_TYPE_HEX, "autn", NULL, 0, 0)) {
                return (TAPP_FAIL);
            }
            break;
        case ISIP_SERVICE_REASON_CAPABILITIES:
            if (TAPP_PASS != _TAPP_getIsipValueByTag(xmlIsip_ptr,
                    &service_ptr->settings.capabilities,
                    TAPP_DATA_TYPE_STRING, "capabilities", NULL, 0, 0)) {
                return (TAPP_FAIL);
            }
            break;
        case ISIP_SERVICE_REASON_PORT:
            if (TAPP_PASS != _TAPP_getIsipValueByTag(xmlIsip_ptr,
                    &service_ptr->settings.port.portType,
                    TAPP_DATA_TYPE_ENUM, "portType", _TAPP_IsipPortTypeTable,
                    ISI_PORT_TYPE_INVALID, ISI_PORT_TYPE_INVALID)) {
                return (TAPP_FAIL);
            }
            if (TAPP_PASS != _TAPP_getIsipValueByTag(xmlIsip_ptr,
                    &service_ptr->settings.port.portNum,
                    TAPP_DATA_TYPE_INTEGER, "portNum", NULL, 0, 0)) {
                return (TAPP_FAIL);
            }
            if (TAPP_PASS != _TAPP_getIsipValueByTag(xmlIsip_ptr,
                    &service_ptr->settings.port.poolSize,
                    TAPP_DATA_TYPE_INTEGER, "poolSize", NULL, 0, 0)) {
                return (TAPP_FAIL);
            }
            break;
        case ISIP_SERVICE_REASON_EMERGENCY:
            if (TAPP_PASS != _TAPP_getIsipValueByTag(xmlIsip_ptr,
                    &service_ptr->settings.isEmergency,
                    TAPP_DATA_TYPE_INTEGER, "isEmergency", NULL, 0, 0)) {
                return (TAPP_FAIL);
            }
            break;
        case ISIP_SERVICE_REASON_IMEI_URI:
            if (TAPP_PASS != _TAPP_getIsipValueByTag(xmlIsip_ptr,
                    &service_ptr->settings.imeiUri,
                    TAPP_DATA_TYPE_STRING, "imeiUri", NULL, 0, 0)) {
                return (TAPP_FAIL);
            }
            break;
        case ISIP_SERVICE_REASON_DEACTIVATE:
            if (TAPP_PASS != _TAPP_getIsipValueByTag(xmlIsip_ptr,
                    &service_ptr->reasonDesc,
                    TAPP_DATA_TYPE_STRING, "reasonDesc", NULL, 0, 0)) {
                return (TAPP_FAIL);
            }
            break;
        case ISIP_SERVICE_REASON_IPSEC:
            if (TAPP_PASS != _TAPP_getIsipValueByTag(xmlIsip_ptr,
                    &service_ptr->settings.ipsec.cfg.protectedPort,
                    TAPP_DATA_TYPE_INTEGER, "portNum", NULL, 0, 0)) {
                return (TAPP_FAIL);
            }
            if (TAPP_PASS != _TAPP_getIsipValueByTag(xmlIsip_ptr,
                    &service_ptr->settings.ipsec.cfg.protectedPortPoolSz,
                    TAPP_DATA_TYPE_INTEGER, "portPoolSize", NULL, 0, 0)) {
                return (TAPP_FAIL);
            }
            if (TAPP_PASS != _TAPP_getIsipValueByTag(xmlIsip_ptr,
                    &service_ptr->settings.ipsec.cfg.spi,
                    TAPP_DATA_TYPE_INTEGER, "spi", NULL, 0, 0)) {
                return (TAPP_FAIL);
            }
            if (TAPP_PASS != _TAPP_getIsipValueByTag(xmlIsip_ptr,
                    &service_ptr->settings.ipsec.cfg.spiPoolSz,
                    TAPP_DATA_TYPE_INTEGER, "spiPoolSize", NULL, 0, 0)) {
                return (TAPP_FAIL);
            }
            if (TAPP_PASS != _TAPP_getIsipValueByTag(xmlIsip_ptr,
                    &service_ptr->settings.ipsec.resp.portUc,
                    TAPP_DATA_TYPE_INTEGER, "portuc", NULL, 0, 0)) {
                return (TAPP_FAIL);
            }
            if (TAPP_PASS != _TAPP_getIsipValueByTag(xmlIsip_ptr,
                    &service_ptr->settings.ipsec.resp.portUs,
                    TAPP_DATA_TYPE_INTEGER, "portus", NULL, 0, 0)) {
                return (TAPP_FAIL);
            }
            if (TAPP_PASS != _TAPP_getIsipValueByTag(xmlIsip_ptr,
                    &service_ptr->settings.ipsec.resp.portPc,
                    TAPP_DATA_TYPE_INTEGER, "portpc", NULL, 0, 0)) {
                return (TAPP_FAIL);
            }
            if (TAPP_PASS != _TAPP_getIsipValueByTag(xmlIsip_ptr,
                    &service_ptr->settings.ipsec.resp.portPs,
                    TAPP_DATA_TYPE_INTEGER, "portps", NULL, 0, 0)) {
                return (TAPP_FAIL);
            }
            if (TAPP_PASS != _TAPP_getIsipValueByTag(xmlIsip_ptr,
                    &service_ptr->settings.ipsec.resp.spiUc,
                    TAPP_DATA_TYPE_INTEGER, "spiuc", NULL, 0, 0)) {
                return (TAPP_FAIL);
            }
            if (TAPP_PASS != _TAPP_getIsipValueByTag(xmlIsip_ptr,
                    &service_ptr->settings.ipsec.resp.spiUs,
                    TAPP_DATA_TYPE_INTEGER, "spius", NULL, 0, 0)) {
                return (TAPP_FAIL);
            }
            if (TAPP_PASS != _TAPP_getIsipValueByTag(xmlIsip_ptr,
                    &service_ptr->settings.ipsec.resp.spiPc,
                    TAPP_DATA_TYPE_INTEGER, "spipc", NULL, 0, 0)) {
                return (TAPP_FAIL);
            }
            if (TAPP_PASS != _TAPP_getIsipValueByTag(xmlIsip_ptr,
                    &service_ptr->settings.ipsec.resp.spiPs,
                    TAPP_DATA_TYPE_INTEGER, "spips", NULL, 0, 0)) {
                return (TAPP_FAIL);
            }
            break;
        case ISIP_SERVICE_REASON_ACTIVATE:

              if (TAPP_PASS != _TAPP_getIsipValueByTag(xmlIsip_ptr,
                    &service_ptr->reasonDesc,
                    TAPP_DATA_TYPE_STRING, "reasonDesc", NULL, 0, 0)) {
                return (TAPP_FAIL);
            }
            break;

        case ISIP_SERVICE_REASON_DESTROY:
        case ISIP_SERVICE_REASON_HANDOFF:
        default:
            break;
    }

    return (TAPP_PASS);
}

/*
 * ======== _TAPP_paserIsipSystem() ========
 *
 * Private function to parse isip with system tag.
 *
 * Returns:
 *  TAPP_PASS: Tag parsing done.
 *  TAPP_FAIL: Failed to parse tag.
 */
TAPP_Return _TAPP_paserIsipSystem(
    ezxml_t       xmlIsip_ptr,
    ISIP_System  *systm_ptr)
{
    if (TAPP_PASS != _TAPP_getIsipValueByTag(xmlIsip_ptr,&systm_ptr->reason,
            TAPP_DATA_TYPE_ENUM, TAPP_XML_REASON_TAG,
            _TAPP_IsipSystemReasonTable, ISIP_SYSTEM_REASON_LAST,
            ISIP_SYSTEM_REASON_LAST)) {
        return (TAPP_FAIL);
    }
    if (TAPP_PASS != _TAPP_getIsipValueByTag(xmlIsip_ptr,&systm_ptr->status,
            TAPP_DATA_TYPE_ENUM, TAPP_XML_STATUS_TAG,
            _TAPP_IsipStatusTable, ISIP_STATUS_LAST, ISIP_STATUS_INVALID)) {
        return (TAPP_FAIL);
    }
    return (TAPP_PASS);
}

/*
 * ======== _TAPP_paserIsipPres() ========
 *
 * Private function to parse isip with pres tag.
 *
 * Returns:
 *  TAPP_PASS: Tag parsing done.
 *  TAPP_FAIL: Failed to parse tag.
 */
TAPP_Return _TAPP_paserIsipPres(
    ezxml_t       xmlIsip_ptr,
    ISIP_Presence  *pres_ptr)
{
    char from[ISI_ADDRESS_STRING_SZ + 1];
    char show[ISI_EVENT_DESC_STRING_SZ + 1];
    char status[ISI_EVENT_DESC_STRING_SZ + 1];

    /* Get Message reason */
    if (TAPP_PASS != _TAPP_getIsipValueByTag(xmlIsip_ptr, &pres_ptr->reason,
            TAPP_DATA_TYPE_ENUM, TAPP_XML_REASON_TAG,
            _TAPP_IsipPresReasonTable, 
            sizeof(_TAPP_IsipPresReasonTable) / sizeof(TAPP_EnumObj),
            ISIP_PRES_REASON_INVALID)) {
        return (TAPP_FAIL);
    }

    /* Get serviceId */
    if (TAPP_PASS != _TAPP_getIsipValueByTag(xmlIsip_ptr, &pres_ptr->serviceId,
            TAPP_DATA_TYPE_INTEGER, TAPP_XML_SERVICE_ID_TAG, NULL, 0, 0)) {
        return (TAPP_FAIL);
    }

    /* Get presence string */
    if (TAPP_PASS != _TAPP_getIsipValueByTag(xmlIsip_ptr, from,
            TAPP_DATA_TYPE_STRING, TAPP_XML_FROM_TAG,
            NULL, 0,0)) {
        return (TAPP_FAIL);
    }
    if (TAPP_PASS != _TAPP_getIsipValueByTag(xmlIsip_ptr, show,
            TAPP_DATA_TYPE_STRING, TAPP_XML_SHOW_TAG,
            NULL, 0,0)) {
        return (TAPP_FAIL);
    }
    if (TAPP_PASS != _TAPP_getIsipValueByTag(xmlIsip_ptr, status,
            TAPP_DATA_TYPE_STRING, TAPP_XML_STATUS_TAG,
            NULL, 0,0)) {
        return (TAPP_FAIL);
    }
    OSAL_snprintf(pres_ptr->presence, sizeof(pres_ptr->presence), 
            "<presence-info><from>%s</from><show>%s</show><status>%s</status>"
            "</presence-info>", from, show, status);

    return (TAPP_PASS);
}

/*
 * ======== _TAPP_xmlParseIsip() ========
 *
 * Private function to parse isip tag
 *
 * Returns:
 *  TAPP_PASS: Isip tag parsed
 *  TAPP_FAIL: Failed to parse isip tag
 */
TAPP_Return _TAPP_xmlParseIsip(
    ezxml_t       xmlIsip_ptr,
    ISIP_Message *isip_ptr)
{
    ezxml_t         isipChild_ptr;

    OSAL_memSet(isip_ptr, 0, sizeof(ISIP_Message));
    /* get code */
    if (TAPP_PASS != _TAPP_getIsipValueByTag(xmlIsip_ptr,
            (void *)&isip_ptr->code,
            TAPP_DATA_TYPE_ENUM, TAPP_XML_CODE_TAG, _TAPP_IsipCodeTable,
            ISIP_SERVICE_REASON_LAST, ISIP_CODE_INVALID)) {
        return (TAPP_FAIL);
    }

    /* Parse id */
    if (TAPP_PASS != _TAPP_getIsipValueByTag(xmlIsip_ptr, &isip_ptr->id,
            TAPP_DATA_TYPE_INTEGER, TAPP_XML_ID_TAG, NULL, 0, 0)) {
        return (TAPP_FAIL);
    }

    /* get protocoal */
    if (TAPP_PASS != _TAPP_getIsipValueByTag(xmlIsip_ptr, &isip_ptr->protocol,
            TAPP_DATA_TYPE_INTEGER, TAPP_XML_PROTOCOL_TAG, NULL, 0, 0)) {
        return (TAPP_FAIL);
    }

    switch (isip_ptr->code) {
        case ISIP_CODE_SERVICE:
            if (NULL == (isipChild_ptr = ezxml_child(xmlIsip_ptr,
                    TAPP_XML_SERVICE_TAG))) {
                return (TAPP_FAIL);
            }
            if (TAPP_PASS != _TAPP_paserIsipService(isipChild_ptr,
                    &isip_ptr->msg.service)) {
                return (TAPP_FAIL);
            }
            break;
        case ISIP_CODE_SYSTEM:
            if (NULL == (isipChild_ptr = ezxml_child(xmlIsip_ptr,
                    TAPP_XML_SYSTEM_TAG))) {
                return (TAPP_FAIL);
            }
            if (TAPP_PASS != _TAPP_paserIsipSystem(isipChild_ptr,
                    &isip_ptr->msg.system)) {
                return (TAPP_FAIL);
            }
            break;
        case ISIP_CODE_CALL:
            if (NULL == (isipChild_ptr = ezxml_child(xmlIsip_ptr,
                    TAPP_XML_CALL_TAG))) {
                return (TAPP_FAIL);
            }
            if (TAPP_PASS != _TAPP_paserIsipCall(isipChild_ptr,
                    &isip_ptr->msg.call)) {
                return (TAPP_FAIL);
            }
            break;
        case ISIP_CODE_TEL_EVENT:
            if (NULL == (isipChild_ptr = ezxml_child(xmlIsip_ptr,
                    TAPP_XML_EVENT_TAG))) {
                return (TAPP_FAIL);
            }
            if (TAPP_PASS != _TAPP_paserIsipTelEvent(isipChild_ptr,
                    &isip_ptr->msg.event)) {
                return (TAPP_FAIL);
            }           
            break;
        case ISIP_CODE_MESSAGE:
            if (NULL == (isipChild_ptr = ezxml_child(xmlIsip_ptr,
                    TAPP_XML_MESSAGE_TAG))) {
                return (TAPP_FAIL);
            }
            if (TAPP_PASS != _TAPP_paserIsipText(isipChild_ptr,
                    &isip_ptr->msg.message)) {
                return (TAPP_FAIL);
            }
            break;
        case ISIP_CODE_USSD:
            if (NULL == (isipChild_ptr = ezxml_child(xmlIsip_ptr,
                    TAPP_XML_USSD_TAG))) {
                return (TAPP_FAIL);
            }
            if (TAPP_PASS != _TAPP_paserIsipUssd(isipChild_ptr,
                    &isip_ptr->msg.ussd)) {
                return (TAPP_FAIL);
            }
            break;
        case ISIP_CODE_PRESENCE:
            if (NULL == (isipChild_ptr = ezxml_child(xmlIsip_ptr,
                    TAPP_XML_PRESENCE_TAG))) {
                return (TAPP_FAIL);
            }
            if (TAPP_PASS != _TAPP_paserIsipPres(isipChild_ptr,
                    &isip_ptr->msg.presence)) {
                return (TAPP_FAIL);
            }
            break;
        default:
            break;
    }
    return (TAPP_PASS);
}

/*
 * ======== _TAPP_xmlParseCsm() ========
 *
 * Private function to parse csm tag
 * Currently only need to support CSM_EVENT_TYPE_RADIO type.
 *
 * Returns:
 *  TAPP_PASS: Csm tag parsed
 *  TAPP_FAIL: Failed to parse csm tag
 */
TAPP_Return _TAPP_xmlParseCsm(
    ezxml_t         xmlCsm_ptr,
    CSM_InputEvent *csm_ptr)
{
    ezxml_t     child_ptr;

    /* Parse type */
    if (NULL == (child_ptr = ezxml_child(xmlCsm_ptr, TAPP_XML_TYPE_TAG))) {
        return (TAPP_FAIL);
    }

    if (0 == OSAL_strncmp(ezxml_txt(child_ptr), "CSM_EVENT_TYPE_RADIO",
            OSAL_strlen("CSM_EVENT_TYPE_RADIO"))) {
        csm_ptr->type = CSM_EVENT_TYPE_RADIO;
    }
    else if (0 == OSAL_strncmp(ezxml_txt(child_ptr), "CSM_EVENT_TYPE_SERVICE",
            OSAL_strlen("CSM_EVENT_TYPE_SERVICE"))) {
        csm_ptr->type = CSM_EVENT_TYPE_SERVICE;
    }
    else {
        return (TAPP_FAIL);
    }

    /* Parse reason */
    if (NULL == (child_ptr = ezxml_child(xmlCsm_ptr, TAPP_XML_REASON_TAG))) {
        return (TAPP_FAIL);
    }

    if (0 == OSAL_strncmp(ezxml_txt(child_ptr),
            "CSM_RADIO_REASON_IP_CHANGE_EMERGENCY",
            OSAL_strlen("CSM_RADIO_REASON_IP_CHANGE_EMERGENCY"))) {
        csm_ptr->evt.radio.reason = CSM_RADIO_REASON_IP_CHANGE_EMERGENCY;
    }
    else if (0 == OSAL_strncmp(ezxml_txt(child_ptr), "CSM_RADIO_REASON_IP_CHANGE",
            OSAL_strlen("CSM_RADIO_REASON_IP_CHANGE"))) {
        csm_ptr->evt.radio.reason = CSM_RADIO_REASON_IP_CHANGE;
    }
    else if (0 == OSAL_strncmp(ezxml_txt(child_ptr),
            "CSM_RADIO_REASON_RADIO_CHANGE_EMERGENCY",
            OSAL_strlen("CSM_RADIO_REASON_RADIO_CHANGE_EMERGENCY"))) {
        csm_ptr->evt.radio.reason = CSM_RADIO_REASON_RADIO_CHANGE_EMERGENCY;
    }
    else if (0 == OSAL_strncmp(ezxml_txt(child_ptr),
            "CSM_SERVICE_REASON_SET_IMEI_URI",
            OSAL_strlen("CSM_SERVICE_REASON_SET_IMEI_URI"))) {
        csm_ptr->evt.service.reason = CSM_SERVICE_REASON_SET_IMEI_URI;
    }
    else if (0 == OSAL_strncmp(ezxml_txt(child_ptr),
            "CSM_SERVICE_REASON_SET_IMPU",
            OSAL_strlen("CSM_SERVICE_REASON_SET_IMPU"))) {
        csm_ptr->evt.service.reason = CSM_SERVICE_REASON_SET_IMPU;
    }
    else if (0 == OSAL_strncmp(ezxml_txt(child_ptr),
            "CSM_SERVICE_REASON_SET_IMPI",
            OSAL_strlen("CSM_SERVICE_REASON_SET_IMPI"))) {
        csm_ptr->evt.service.reason = CSM_SERVICE_REASON_SET_IMPI;
    }
    else if (0 == OSAL_strncmp(ezxml_txt(child_ptr),
            "CSM_SERVICE_REASON_SET_DOMAIN",
            OSAL_strlen("CSM_SERVICE_REASON_SET_DOMAIN"))) {
        csm_ptr->evt.service.reason = CSM_SERVICE_REASON_SET_DOMAIN;
    }
    else if (0 == OSAL_strncmp(ezxml_txt(child_ptr),
            "CSM_SERVICE_REASON_SET_PASSWORD",
            OSAL_strlen("CSM_SERVICE_REASON_SET_PASSWORD"))) {
        csm_ptr->evt.service.reason = CSM_SERVICE_REASON_SET_PASSWORD;
    }
    else if (0 == OSAL_strncmp(ezxml_txt(child_ptr),
            "CSM_SERVICE_REASON_SET_PCSCF",
            OSAL_strlen("CSM_SERVICE_REASON_SET_PCSCF"))) {
        csm_ptr->evt.service.reason = CSM_SERVICE_REASON_SET_PCSCF;
    }
    else if (0 == OSAL_strncmp(ezxml_txt(child_ptr),
            "CSM_SERVICE_REASON_UPDATE_CGI",
            OSAL_strlen("CSM_SERVICE_REASON_UPDATE_CGI"))) {
        csm_ptr->evt.service.reason = CSM_SERVICE_REASON_UPDATE_CGI;
    }
    else if (0 == OSAL_strncmp(ezxml_txt(child_ptr),
            "CSM_SERVICE_REASON_IMS_ENABLE",
            OSAL_strlen("CSM_SERVICE_REASON_IMS_ENABLE"))) {
        csm_ptr->evt.service.reason = CSM_SERVICE_REASON_IMS_ENABLE;
    }
    else if (0 == OSAL_strncmp(ezxml_txt(child_ptr),
            "CSM_SERVICE_REASON_IMS_DISABLE",
            OSAL_strlen("CSM_SERVICE_REASON_IMS_DISABLE"))) {
        csm_ptr->evt.service.reason = CSM_SERVICE_REASON_IMS_DISABLE;
    }
    else {
        return (TAPP_FAIL);
    }

    if (CSM_EVENT_TYPE_SERVICE == csm_ptr->type) {
        if (CSM_SERVICE_REASON_SET_IMPU == csm_ptr->evt.service.reason) {
            if (NULL == (child_ptr = ezxml_child(xmlCsm_ptr,
                    "impu"))) {
                return (TAPP_FAIL);
            }
            OSAL_strncpy(csm_ptr->evt.service.u.impu,
                    ezxml_txt(child_ptr), CSM_EVENT_STRING_SZ + 1);
        }
        else if (CSM_SERVICE_REASON_SET_IMPI == csm_ptr->evt.service.reason) {
            if (NULL == (child_ptr = ezxml_child(xmlCsm_ptr,
                    "impi"))) {
                return (TAPP_FAIL);
            }
            OSAL_strncpy(csm_ptr->evt.service.u.impi,
                    ezxml_txt(child_ptr), CSM_EVENT_STRING_SZ + 1);
        }
        else if (CSM_SERVICE_REASON_SET_DOMAIN == csm_ptr->evt.service.reason) {
            if (NULL == (child_ptr = ezxml_child(xmlCsm_ptr,
                    "domain"))) {
                return (TAPP_FAIL);
            }
            OSAL_strncpy(csm_ptr->evt.service.u.domain,
                    ezxml_txt(child_ptr), CSM_EVENT_STRING_SZ + 1);
        }
        else if (CSM_SERVICE_REASON_SET_PASSWORD ==
                csm_ptr->evt.service.reason) {
            if (NULL == (child_ptr = ezxml_child(xmlCsm_ptr,
                    "password"))) {
                return (TAPP_FAIL);
            }
            OSAL_strncpy(csm_ptr->evt.service.u.password,
                    ezxml_txt(child_ptr), CSM_EVENT_STRING_SZ + 1);
        }
        else if (CSM_SERVICE_REASON_SET_PCSCF == csm_ptr->evt.service.reason) {
            if (NULL == (child_ptr = ezxml_child(xmlCsm_ptr,
                    "pcscf"))) {
                return (TAPP_FAIL);
            }
            OSAL_strncpy(csm_ptr->evt.service.u.pcscf,
                    ezxml_txt(child_ptr), CSM_EVENT_STRING_SZ + 1);
        }
        else if (CSM_SERVICE_REASON_SET_IMEI_URI == csm_ptr->evt.service.reason) {
            /* Parse IMEI URI */
            if (NULL == (child_ptr = ezxml_child(xmlCsm_ptr,
                    TAPP_XML_IMEIURI_TAG))) {
                return (TAPP_FAIL);
            }
            OSAL_strncpy(csm_ptr->evt.service.u.imeiUri,
                    ezxml_txt(child_ptr),
                    sizeof(csm_ptr->evt.service.u.imeiUri));
        }
        else if (CSM_SERVICE_REASON_UPDATE_CGI == csm_ptr->evt.service.reason) {
            /* CGI type */
            if (NULL == (child_ptr = ezxml_child(xmlCsm_ptr, "cgi-type"))) {
                return (TAPP_FAIL);
            }
            if (0 == OSAL_strncmp(ezxml_txt(child_ptr), "3GPP-GERAN",
                    OSAL_strlen("3GPP-GERAN"))) {
                csm_ptr->evt.service.u.cgi.type =
                        CSM_SERVICE_ACCESS_TYPE_3GPP_GERAN;
            }
            else if (0 == OSAL_strncmp(ezxml_txt(child_ptr), "3GPP_UTRAN_FDD",
                    OSAL_strlen("3GPP_UTRAN_FDD"))) {
                csm_ptr->evt.service.u.cgi.type =
                        CSM_SERVICE_ACCESS_TYPE_3GPP_UTRAN_FDD;
            }
            else if (0 == OSAL_strncmp(ezxml_txt(child_ptr), "3GPP_UTRAN_TDD",
                    OSAL_strlen("3GPP_UTRAN_TDD"))) {
                csm_ptr->evt.service.u.cgi.type =
                        CSM_SERVICE_ACCESS_TYPE_3GPP_UTRAN_TDD;
            }
            else {
                return (TAPP_FAIL);
            }
            /* fill in CGI */
            if (NULL == (child_ptr = ezxml_child(xmlCsm_ptr, "cgi"))) {
                return (TAPP_FAIL);
            }
            OSAL_strncpy(csm_ptr->evt.service.u.cgi.id, ezxml_txt(child_ptr),
                    CSM_CGI_STRING_SZ + 1);
        }
    }
    else if (CSM_EVENT_TYPE_RADIO == csm_ptr->type){

        if ((CSM_RADIO_REASON_IP_CHANGE == csm_ptr->evt.radio.reason) ||
                (CSM_RADIO_REASON_IP_CHANGE_EMERGENCY ==
                csm_ptr->evt.radio.reason)) {
            /* Parse ip address */
            if (NULL == (child_ptr = ezxml_child(xmlCsm_ptr, TAPP_XML_IP_TAG))) {
                return (TAPP_FAIL);
            }
            
            OSAL_strcpy(csm_ptr->evt.radio.address, ezxml_txt(child_ptr));

            /* Parse infc name */
            if (NULL == (child_ptr = ezxml_child(xmlCsm_ptr, TAPP_XML_INFC_TAG))) {
                return (TAPP_FAIL);
            }

            OSAL_strncpy(csm_ptr->evt.radio.infcName, ezxml_txt(child_ptr),
                    sizeof(csm_ptr->evt.radio.infcName));

            /* Pare network type "wifi" or "lte" */
            if (NULL == (child_ptr = ezxml_child(xmlCsm_ptr,
                    TAPP_XML_NETTYPE_TAG))) {
                csm_ptr->evt.radio.networkType = CSM_RADIO_NETWORK_TYPE_LTE;
            }

            if (0 == OSAL_strncmp(ezxml_txt(child_ptr), "wifi",
                    OSAL_strlen("wifi"))) {
                csm_ptr->evt.radio.networkType = CSM_RADIO_NETWORK_TYPE_WIFI;
            }
            else if (0 == OSAL_strncmp(ezxml_txt(child_ptr), "lte",
                    OSAL_strlen("lte"))) {
                csm_ptr->evt.radio.networkType = CSM_RADIO_NETWORK_TYPE_LTE;
            }
            else if (0 == OSAL_strncmp(ezxml_txt(child_ptr), "lte_ss",
                    OSAL_strlen("lte_ss"))) {
                csm_ptr->evt.radio.networkType = CSM_RADIO_NETWORK_TYPE_LTE_SS;
            }
            else {
                csm_ptr->evt.radio.networkType = CSM_RADIO_NETWORK_TYPE_NONE;
            }
        }
        else if (CSM_RADIO_REASON_RADIO_CHANGE_EMERGENCY ==
                csm_ptr->evt.radio.reason) {
            /* Parse isEmergencyFailoverToCs */
            if (NULL == (child_ptr = ezxml_child(xmlCsm_ptr,
                    TAPP_XML_FAILOVER_TAG))) {
                return (TAPP_FAIL);
            }
            csm_ptr->evt.radio.isEmergencyFailoverToCs =
                    OSAL_atoi(ezxml_txt(child_ptr));
            /* Parse isEmergencyRegRequired */
            if (NULL == (child_ptr = ezxml_child(xmlCsm_ptr,
                    TAPP_XML_EMGCY_REG_REQ_TAG))) {
                return (TAPP_FAIL);
            }
            csm_ptr->evt.radio.isEmergencyRegRequired =
                    OSAL_atoi(ezxml_txt(child_ptr));
        }
    }
    return (TAPP_PASS);
}

/*
 * ======== _TAPP_xmlParseRpc() ========
 *
 * Private function to parse rpc tag
 * Currently only need to support VPR .
 *
 * Returns:
 *  TAPP_PASS: Csm tag parsed
 *  TAPP_FAIL: Failed to parse csm tag
 */
TAPP_Return _TAPP_xmlParseRpc(
    ezxml_t        xmlRpc_ptr,
    TAPP_Action   *action_ptr)
{
    ezxml_t       child_ptr;
    char         *value_ptr;
    char         *valueCid_ptr;
    int           index;
    int           typeIdx;
    TAPP_DataType xmlType;
    ISI_Return    retVal;
    char         *data_ptr;
    RPC_Message  *rpc_ptr;
    OSAL_FileId   fid;
    char          xmlDoc[ISI_PROVISIONING_DATA_STRING_SZ + 1];
    vint          size;
    char          mediaAttr[ISI_MEDIA_ATTR_STRING_SZ + 1];

    rpc_ptr = &action_ptr->msg.rpcMsg;
    OSAL_memSet(rpc_ptr, 0, sizeof(RPC_Message));
    ISI_xdrEncodeInit(&rpc_ptr->isiXdr);
    /* Get function call type */
    _TAPP_getIsipValueByTag(xmlRpc_ptr, &rpc_ptr->funcType,
            TAPP_DATA_TYPE_ENUM, TAPP_XML_FUNC_ATTR,
            _TAPP_RpcIsiFuncTypeTable,
            ISI_FUNC_LAST, ISI_FUNC_LAST);
    /* put function call type */
    if (TAPP_ACTION_TYPE_ISSUE_ISI_RPC == action_ptr->type) {
        ISI_xdrPutInteger(&rpc_ptr->isiXdr,
                action_ptr->msg.rpcMsg.funcType);
    }
    /* get xdr data */
    typeIdx = 0;
    child_ptr = ezxml_child(xmlRpc_ptr, TAPP_XML_PARAM_TAG);
    while (NULL != child_ptr) {
        /* Get data type */
        if (NULL == (value_ptr = (char*)ezxml_attr(child_ptr,
                TAPP_XML_TYPE_TAG))) {
            continue;
        }
        
        for (index =0; index < TAPP_DATA_TYPE_LAST; index++) {
            if (!OSAL_strcmp(_TAPP_DataTypeTable[index].string, value_ptr)) {
                    xmlType = _TAPP_DataTypeTable[index].index;
                    break;
            }
        }
        /* Get don't care attr */
        value_ptr = (char *)ezxml_attr(child_ptr, TAPP_XML_IS_DONT_CARE_ATTR);
        if (NULL != value_ptr) {
            rpc_ptr->isDontCare[typeIdx] = atoi(value_ptr);
        }
        data_ptr = ezxml_txt(child_ptr);
        /*  The return value must be the first prarmeter
               *  in VALIDATE ISI EVT/RPC RETURN action type
               */
        if (TAPP_ACTION_TYPE_ISSUE_ISI_RPC != action_ptr->type &&
                TAPP_DATA_TYPE_ENUM == xmlType && 0 == typeIdx) {
            for (index =0; index < ISI_RETURN_LAST; index++) {
                if (!OSAL_strcmp(_TAPP_RpcIsiRetValTable[index].string,
                        data_ptr)) {
                    retVal = _TAPP_RpcIsiRetValTable[index].index;
                    /* Put return value */
                    ISI_xdrPutInteger(&rpc_ptr->isiXdr, retVal);
                    rpc_ptr->dataType[typeIdx] = TAPP_ISI_RPC_DATA_TYPE_INT;
                    break;
                }
            }
            
        }
        else if (TAPP_DATA_TYPE_INTEGER == xmlType) {
            /* check if serviceId index */
            value_ptr = (char *)ezxml_attr(child_ptr, TAPP_XML_SERVICE_ID_TAG);
            /* check if callId index */
            valueCid_ptr = (char *)ezxml_attr(child_ptr, TAPP_XML_CALL_ID_TAG);
            if (NULL != valueCid_ptr) {
                rpc_ptr->id.idType = TAPP_ID_TYPE_CALL;
                rpc_ptr->id.idIdx = atoi(data_ptr);
                rpc_ptr->id.paramIdx = atoi(valueCid_ptr);
            }
            else if (NULL != value_ptr){
                rpc_ptr->id.idType = TAPP_ID_TYPE_SERVICE;
                rpc_ptr->id.idIdx = atoi(data_ptr);
                rpc_ptr->id.paramIdx = atoi(value_ptr);
            }

            ISI_xdrPutInteger(&rpc_ptr->isiXdr, atoi(data_ptr)); 
            
            rpc_ptr->dataType[typeIdx] = TAPP_ISI_RPC_DATA_TYPE_INT;
        }
        else if (TAPP_DATA_TYPE_MEDIA_ATTR == xmlType) {
            /* construct media attribute */
            CSM_constructSessionAttributesXml(mediaAttr, atoi(data_ptr),
            ISI_MEDIA_ATTR_STRING_SZ);
            ISI_xdrPutString(&rpc_ptr->isiXdr, mediaAttr,
                    OSAL_strlen(mediaAttr) + 1); 
        }
        else if (TAPP_DATA_TYPE_STRING == xmlType) {
            ISI_xdrPutString(&rpc_ptr->isiXdr, data_ptr,
                    OSAL_strlen(data_ptr) + 1);
            rpc_ptr->dataType[typeIdx] = TAPP_ISI_RPC_DATA_TYPE_STR;
        }
        else if (TAPP_DATA_TYPE_XML_DOC == xmlType) {
            /* Open file */
            if (OSAL_SUCCESS != OSAL_fileOpen(&fid, data_ptr,
                    OSAL_FILE_O_RDONLY, 00755)) {
                return (TAPP_FAIL);
            }
            /* Read file */
            size = ISI_PROVISIONING_DATA_STRING_SZ;
            if (OSAL_SUCCESS != OSAL_fileRead(&fid, xmlDoc, &size)) {
                OSAL_fileClose(&fid);
                return (TAPP_FAIL);
            }
            /* Close file */
            OSAL_fileClose(&fid);
            ISI_xdrPutString(&rpc_ptr->isiXdr, xmlDoc, 
                    OSAL_strlen(xmlDoc) + 1);
            rpc_ptr->dataType[typeIdx] = TAPP_ISI_RPC_DATA_TYPE_STR;
        }
        typeIdx ++;
        child_ptr = ezxml_next(child_ptr);
    }
    return (TAPP_PASS);
}

/*
 * ======== _TAPP_xmlParseAction() ========
 *
 * Private function to parse action tag
 *
 * Returns:
 *  TAPP_PASS: Action tag parsed
 *  TAPP_FAIL: Failed to parse action tag
 */
TAPP_Return _TAPP_xmlParseAction(
    ezxml_t      xmlAction_ptr,
    TAPP_Action *action_ptr)
{
    char    *type_ptr;
    char    *timeout_ptr;
    ezxml_t  isip_ptr;
    ezxml_t  csm_ptr;
    ezxml_t  xml_ptr, xmlChild_ptr;
    char    *fail_ptr;

    type_ptr = (char *)ezxml_attr(xmlAction_ptr, TAPP_XML_TYPE_ATTR);
    if (NULL == type_ptr) {
        /* No type */
        TAPP_dbgPrintf("No type in action\n");
        return (TAPP_FAIL);
    }

   if (0 == OSAL_strncmp(type_ptr, TAPP_XML_ATTR_ISSUE_AT,
            OSAL_strlen(TAPP_XML_ATTR_ISSUE_AT))) {
        /* Issue at */
        action_ptr->type = TAPP_ACTION_TYPE_ISSUE_AT;
        OSAL_strncpy(action_ptr->msg.at, ezxml_txt(xmlAction_ptr),
                TAPP_AT_COMMAND_STRING_SZ);        
    }
    else if (0 == OSAL_strncmp(type_ptr, TAPP_XML_ATTR_VALIDATE_AT,
            OSAL_strlen(TAPP_XML_ATTR_VALIDATE_AT))) {
        /* Validate at */
        action_ptr->type = TAPP_ACTION_TYPE_VALIDATE_AT;
        OSAL_strncpy(action_ptr->msg.at, ezxml_txt(xmlAction_ptr),
                TAPP_AT_COMMAND_STRING_SZ);
    }
    else if (0 == OSAL_strncmp(type_ptr, TAPP_XML_ATTR_ISSUE_GSM_AT,
            OSAL_strlen(TAPP_XML_ATTR_ISSUE_GSM_AT))) {
        /* Issue gsm at */
        action_ptr->type = TAPP_ACTION_TYPE_ISSUE_GSM_AT;
        OSAL_strncpy(action_ptr->msg.at, ezxml_txt(xmlAction_ptr),
                TAPP_AT_COMMAND_STRING_SZ);
    }
    else if (0 == OSAL_strncmp(type_ptr, TAPP_XML_ATTR_VALIDATE_GSM_AT,
            OSAL_strlen(TAPP_XML_ATTR_VALIDATE_GSM_AT))) {
        /* Validate gsm at */
        action_ptr->type = TAPP_ACTION_TYPE_VALIDATE_GSM_AT;
        OSAL_strncpy(action_ptr->msg.at, ezxml_txt(xmlAction_ptr),
                TAPP_AT_COMMAND_STRING_SZ);
    }
    else if (0 == OSAL_strncmp(type_ptr, TAPP_XML_ATTR_ISSUE_ISIP,
            OSAL_strlen(TAPP_XML_ATTR_ISSUE_ISIP))) {
        /* Issue isip */
        isip_ptr = ezxml_child(xmlAction_ptr, TAPP_XML_ISIP_TAG);
        if (NULL == isip_ptr) {
            TAPP_dbgPrintf("Parse action failed: Missing isip tag\n");
            return (TAPP_FAIL);
        }
        action_ptr->type = TAPP_ACTION_TYPE_ISSUE_ISIP;
        if (TAPP_FAIL == _TAPP_xmlParseIsip(isip_ptr,
                &action_ptr->msg.isip)) {
            TAPP_dbgPrintf("Parse isip failed\n");
            return (TAPP_FAIL);
        }
    }
    else if (0 == OSAL_strncmp(type_ptr, TAPP_XML_ATTR_VALIDATE_ISIP,
            OSAL_strlen(TAPP_XML_ATTR_VALIDATE_ISIP))) {
        /* Validate isip */
        isip_ptr = ezxml_child(xmlAction_ptr, TAPP_XML_ISIP_TAG);
        if (NULL == isip_ptr) {
            TAPP_dbgPrintf("Parse action failed: Missing isip tag\n");
            return (TAPP_FAIL);
        }
        action_ptr->type = TAPP_ACTION_TYPE_VALIDATE_ISIP;
        if (TAPP_FAIL == _TAPP_xmlParseIsip(isip_ptr,
                &action_ptr->msg.isip)) {
            TAPP_dbgPrintf("Parse isip failed\n");
            return (TAPP_FAIL);
        }
    }
    else if (0 == OSAL_strncmp(type_ptr, TAPP_XML_ATTR_ISSUE_CSM,
            OSAL_strlen(TAPP_XML_ATTR_ISSUE_CSM))) {
        /* Issue csm */
        csm_ptr = ezxml_child(xmlAction_ptr, TAPP_XML_CSM_TAG);
        if (NULL == csm_ptr) {
            TAPP_dbgPrintf("Parse action failed: Missing csm tag\n");
            return (TAPP_FAIL);
        }
        action_ptr->type = TAPP_ACTION_TYPE_ISSUE_CSM;
        if (TAPP_FAIL == _TAPP_xmlParseCsm(csm_ptr, &action_ptr->msg.csm)) {
            TAPP_dbgPrintf("Parse csm failed\n");
            return (TAPP_FAIL);
        }
    }
    else if (0 == OSAL_strncmp(type_ptr, TAPP_XML_ATTR_PAUSE,
            OSAL_strlen(TAPP_XML_ATTR_PAUSE))) {
        /* Pause */
        action_ptr->type = TAPP_ACTION_TYPE_PAUSE;
        action_ptr->u.pause = OSAL_atoi(ezxml_txt(xmlAction_ptr));
    }
    else if (0 == OSAL_strncmp(type_ptr, TAPP_XML_ATTR_CLEAN_ISIP,
            OSAL_strlen(TAPP_XML_ATTR_CLEAN_ISIP))) {
        /* clean isip */
        action_ptr->type = TAPP_ACTION_TYPE_CLEAN_ISIP;
    } 
    else if (0 == OSAL_strncmp(type_ptr, TAPP_XML_ATTR_ISSUE_XCAP,
            OSAL_strlen(TAPP_XML_ATTR_ISSUE_XCAP))) {
        /* Issue xcan event, so look down <xcap> <event> subtree */
        xml_ptr = ezxml_child(xmlAction_ptr, TAPP_XML_XCAP_TAG);
        if (NULL == xml_ptr) {
            TAPP_dbgPrintf("Parse action failed: Missing xcap tag\n");
            return (TAPP_FAIL);
        }
        if (NULL == (xmlChild_ptr = ezxml_child(xml_ptr,
                    TAPP_XML_EVENT_TAG))) {
            TAPP_dbgPrintf("Parse action failed: Missing event tag\n");
            return (TAPP_FAIL);
        }
        action_ptr->type = TAPP_ACTION_TYPE_ISSUE_XCAP;
        if (TAPP_PASS != TAPP_xmlParseXcapEvt(xmlChild_ptr,
                &action_ptr->msg.mockXcapEvt)) {
            TAPP_dbgPrintf("Failed Parse xcap event action\n");
            return (TAPP_FAIL);
        }
    }
    else if (0 == OSAL_strncmp(type_ptr, TAPP_XML_ATTR_VALIDATE_XCAP,
            OSAL_strlen(TAPP_XML_ATTR_VALIDATE_XCAP))) {
        /* Issue xcan event, so look down <xcap> <command> subtree */
        xml_ptr = ezxml_child(xmlAction_ptr, TAPP_XML_XCAP_TAG);
        if (NULL == xml_ptr) {
            TAPP_dbgPrintf("Parse action failed: Missing xcap tag\n");
            return (TAPP_FAIL);
        }
        if (NULL == (xmlChild_ptr = ezxml_child(xml_ptr,
                    TAPP_XML_COMMAND_TAG))) {
            TAPP_dbgPrintf("Parse action failed: Missing command tag\n");
            return (TAPP_FAIL);
        }
        action_ptr->type = TAPP_ACTION_TYPE_VALIDATE_XCAP;
        if (TAPP_PASS != TAPP_xmlParseXcapCmd(xmlChild_ptr,
                &action_ptr->msg.mockXcapCmd)) {
            TAPP_dbgPrintf("Failed Parse xcap command action\n");
            return (TAPP_FAIL);
        }
    }
    else if (0 == OSAL_strncmp(type_ptr, TAPP_XML_ATTR_ISSUE_ISI_RPC,
            OSAL_strlen(TAPP_XML_ATTR_ISSUE_ISI_RPC))) {
        /* Issue rpc */
        xml_ptr = ezxml_child(xmlAction_ptr, TAPP_XML_ISI_RPC_TAG);
        if (NULL == xml_ptr) {
            TAPP_dbgPrintf("Parse action failed: Missing rpc tag\n");
            return (TAPP_FAIL);
        }
        action_ptr->type = TAPP_ACTION_TYPE_ISSUE_ISI_RPC;
        if (TAPP_FAIL == _TAPP_xmlParseRpc(xml_ptr, action_ptr)) {
            TAPP_dbgPrintf("Parse isi rpc failed\n");
            return (TAPP_FAIL);
        }
    }
    else if (0 == OSAL_strncmp(type_ptr, 
            TAPP_XML_ATTR_VALIDATE_ISI_RPC_RETURN,
            OSAL_strlen(TAPP_XML_ATTR_VALIDATE_ISI_RPC_RETURN))) {
        /* Validate rpc */
        xml_ptr = ezxml_child(xmlAction_ptr, TAPP_XML_ISI_RPC_TAG);
        if (NULL == xml_ptr) {
            TAPP_dbgPrintf("Parse action failed: Missing isip tag\n");
            return (TAPP_FAIL);
        }
        action_ptr->type = TAPP_ACTION_TYPE_VALIDATE_ISI_RPC_RETURN;
        if (TAPP_FAIL == _TAPP_xmlParseRpc(xml_ptr, action_ptr )) {
            TAPP_dbgPrintf("Parse isi rpc failed\n");
            return (TAPP_FAIL);
        }
    }
    else if (0 == OSAL_strncmp(type_ptr, TAPP_XML_ATTR_VALIDATE_ISI_GET_EVT,
            OSAL_strlen(TAPP_XML_ATTR_VALIDATE_ISI_GET_EVT))) {
        /* Validate rpc */
        xml_ptr = ezxml_child(xmlAction_ptr, TAPP_XML_ISI_RPC_TAG);
        if (NULL == xml_ptr) {
            TAPP_dbgPrintf("Parse action failed: Missing isip tag\n");
            return (TAPP_FAIL);
        }
        action_ptr->type = TAPP_ACTION_TYPE_VALIDATE_ISI_GET_EVT;
        if (TAPP_FAIL == _TAPP_xmlParseRpc(xml_ptr, action_ptr )) {
            TAPP_dbgPrintf("Parse isi rpc failed\n");
            return (TAPP_FAIL);
        }
    }
    else if (0 == OSAL_strncmp(type_ptr, TAPP_XML_ATTR_CLEAN_ISI_RPC_EVT,
            OSAL_strlen(TAPP_XML_ATTR_CLEAN_ISI_RPC_EVT))) {
        /* clean isi server event */
        action_ptr->type = TAPP_ACTION_TYPE_CLEAN_ISI_EVT;
    } 
    else {
        /* Invalid type */
        TAPP_dbgPrintf("Invalid action type: %s\n", type_ptr);
    }

    timeout_ptr = (char *)ezxml_attr(xmlAction_ptr, TAPP_XML_TIMEOUT_ATTR);
    if (NULL != timeout_ptr) {
        action_ptr->u.timeout = OSAL_atoi(timeout_ptr);
    }
    else if (TAPP_ACTION_TYPE_PAUSE != action_ptr->type) {
        /* Give default timeout value */
        action_ptr->u.timeout = 1000;
    }

    fail_ptr = (char *)ezxml_attr(xmlAction_ptr, TAPP_XML_ONFAIL_ATTR);
    if (NULL != fail_ptr) {
        if (0 == OSAL_strncmp(type_ptr, TAPP_XML_CONTINUE_TAG,
                OSAL_strlen(TAPP_XML_CONTINUE_TAG))) {
            /* continue when failed */
            action_ptr->stop = 0;
        }
        else if (0 == OSAL_strncmp(type_ptr, TAPP_XML_STOP_TAG,
                OSAL_strlen(TAPP_XML_CONTINUE_TAG))) {
            /* stop when failed */
            action_ptr->stop = 1;
        }
    }
    else {
        /* default:  stop when failed */
        action_ptr->stop = 1;
    }
    _TAPP_printAction(action_ptr);

    return (TAPP_PASS);
}

/*
 * ======== _TAPP_xmlParseTestCase() ========
 *
 * Private function to parse test case tag
 *
 * Returns:
 *  TAPP_PASS: Test case tag parsed
 *  TAPP_FAIL: Failed to parse test case tag
 */
TAPP_Return _TAPP_xmlParseTestCase(
    TAPP_GlobalObj *global_ptr,
    const char     *folderPath,
    ezxml_t         xml_ptr)
{
    ezxml_t      testcase_ptr;
    ezxml_t      include_ptr;
    ezxml_t      action_ptr;
    char        *repeat_ptr;
    char        *name_ptr;
    TAPP_Action  action;
    vint         idx;
    vint         repeatStart;
    char         filePath[TAPP_SCRATCH_STRING_SZ];

    OSAL_memSet(&action, 0, sizeof(TAPP_Action));

    /* Loop all test cases */
    testcase_ptr = ezxml_child(xml_ptr, TAPP_XML_TESTCASE_TAG);
    while (NULL != testcase_ptr) {
        /*
         * Got a testcase tag, process it.
         * Get repeat time
         */
        repeatStart = 0;
        if (NULL != (repeat_ptr = (char *)ezxml_attr(testcase_ptr,
                TAPP_XML_REPEAT_ATTR))) {
            action.type = TAPP_ACTION_TYPE_REPEAT_START;
            repeatStart = action.u.repeat = OSAL_atoi(repeat_ptr);
            /* Add a repeat action and get the repeat loop index */
            if (TAPP_PASS != _TAPP_addAction(global_ptr, &action,
                    &repeatStart)) {
                OSAL_logMsg("%s:%d Failed to add action to action list\n",
                        __FILE__, __LINE__);
                return (TAPP_FAIL);
            }
            /* Clean up TAPP_Action */
            OSAL_memSet(&action, 0, sizeof(TAPP_Action));
        }
    
        /* Check if it includes other files */
        include_ptr = ezxml_child(testcase_ptr, TAPP_XML_INCLUDE_TAG);
        while (NULL != include_ptr) {
            OSAL_snprintf(filePath, TAPP_SCRATCH_STRING_SZ, "%s%s", 
                    folderPath, ezxml_txt(include_ptr));
            if (TAPP_FAIL ==
                    TAPP_readTestCase(global_ptr, filePath)) {
                return (TAPP_FAIL);
            }
            include_ptr = ezxml_next(include_ptr);
        }
        /* Get test case name for report output */
        if (NULL != (name_ptr = (char *)ezxml_attr(testcase_ptr,
                TAPP_XML_NAME_ATTR))) {
            OSAL_strncpy(action.testCaseName, name_ptr,
                    sizeof(action.testCaseName));
        }

        /* Read actions within a test case */
        action_ptr = ezxml_child(testcase_ptr, TAPP_XML_ACTION_TAG);
        while (NULL != action_ptr) {
            if (TAPP_PASS == _TAPP_xmlParseAction(action_ptr, &action)) {
                /* Add action to action list */
                if (TAPP_PASS != _TAPP_addAction(global_ptr, &action, &idx)) {
                    OSAL_logMsg("%s:%d Failed to add action to action list\n",
                            __FILE__, __LINE__);
                    return (TAPP_FAIL);
                }
                /* Clean up TAPP_Action */
                OSAL_memSet(&action, 0, sizeof(TAPP_Action));
            }
            else {
                /* Failed to parse action, return failed */
                OSAL_logMsg("%s:%d Failed to parse action\n", __FILE__,
                        __LINE__);
                return (TAPP_FAIL);
            }
            action_ptr = ezxml_next(action_ptr);
        } 
        /* Look if the test case is repeated */
        if (0 != repeatStart) {
            /* Add a repeat end action and store the repeat start index to it */
            action.type = TAPP_ACTION_TYPE_REPEAT_END;
            action.u.repeat = repeatStart;
            /* Add a repeat action */
            if (TAPP_PASS != _TAPP_addAction(global_ptr, &action,
                    &idx)) {
                OSAL_logMsg("%s:%d Failed to add action to action list\n",
                        __FILE__, __LINE__);
                return (TAPP_FAIL);
            }
            /* Clean up TAPP_Action */
            OSAL_memSet(&action, 0, sizeof(TAPP_Action));
        }
        testcase_ptr = ezxml_next(testcase_ptr);
    }

    return (TAPP_PASS);
}
