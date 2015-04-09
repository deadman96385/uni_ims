/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 29644 $ $Date: 2014-11-03 19:15:40 +0800 (Mon, 03 Nov 2014) $
 */

#ifndef _SIP_SIP_SYNTAX_H_
#define _SIP_SIP_SYNTAX_H_


#define SIP_VERSION_STR "SIP/2.0"

#define SIP_TRUE_STR  "TRUE"
#define SIP_FALSE_STR "FALSE"

/* METHODS */
#define SIP_FIRST_METHOD_STR     "FIRSTMETHOD"
#define SIP_INVITE_METHOD_STR    "INVITE"
#define SIP_CANCEL_METHOD_STR    "CANCEL"
#define SIP_BYE_METHOD_STR       "BYE"
#define SIP_OPTIONS_METHOD_STR   "OPTIONS"
#define SIP_REGISTER_METHOD_STR  "REGISTER"
#define SIP_ACK_METHOD_STR       "ACK"
#define SIP_NOTIFY_METHOD_STR    "NOTIFY"
#define SIP_REFER_METHOD_STR     "REFER"
#define SIP_MESSAGE_METHOD_STR   "MESSAGE"
#define SIP_SUBSCRIBE_METHOD_STR "SUBSCRIBE"
#define SIP_INFO_METHOD_STR      "INFO"
#define SIP_PRACK_METHOD_STR     "PRACK"
#define SIP_UPDATE_METHOD_STR    "UPDATE"
#define SIP_PUBLISH_METHOD_STR   "PUBLISH"
#define SIP_ERROR_METHOD_STR     "ERROR"


/* HEADER FIELDS */
#define SIP_ACCEPT_HF_STR              "Accept"
#define SIP_ACCEPT_ENCODING_HF_STR     "Accept-Encoding"
#define SIP_ACCEPT_LANGUAGE_HF_STR     "Accept-Language"
#define SIP_ALERT_INFO_HF_STR          "Alert-Info"
#define SIP_ALLOW_HF_STR               "Allow"
#define SIP_ALLOW_EVENTS_HF_STR        "Allow-Events"
#define SIP_AUTHORIZATION_HF_STR       "Authorization"
#define SIP_CALL_ID_HF_STR             "Call-ID"
#define SIP_CONTACT_HF_STR             "Contact"
#define SIP_CONTENT_DISP_HF_STR        "Content-Disposition"
#define SIP_CONTENT_ENCODING_HF_STR    "Content-Encoding"
#define SIP_CONTENT_LENGTH_HF_STR      "Content-Length"
#define SIP_CONTENT_TYPE_HF_STR        "Content-Type"
#define SIP_CSEQ_HF_STR                "CSeq"
#define SIP_ETAG_HF_STR                "SIP-ETag"
#define SIP_EVENT_HF_STR               "Event"
#define SIP_EXPIRES_HF_STR             "Expires"
#define SIP_FROM_HF_STR                "From"
#define SIP_IF_MATCH_HF_STR            "SIP-If-Match"
#define SIP_MAX_FORWARDS_HF_STR        "Max-Forwards"
#define SIP_MIN_EXPIRES_HF_STR         "Min-Expires"
#define SIP_MIN_SE_HF_STR              "Min-SE"
#define SIP_ORGANIZATION_HF_STR        "Organization"
#define SIP_PROXY_AUTHENTICATE_HF_STR  "Proxy-Authenticate"
#define SIP_PROXY_AUTHORIZATION_HF_STR "Proxy-Authorization"
#define SIP_RACK_HF_STR                "RAck"
#define SIP_RECORD_ROUTE_HF_STR        "Record-Route"
#define SIP_REFER_TO_HF_STR            "Refer-To"
#define SIP_REFERRED_BY_HF_STR         "Referred-By"
#define SIP_REPLACES_HF_STR            "Replaces"
#define SIP_REQUIRE_HF_STR             "Require"
#define SIP_ROUTE_HF_STR               "Route"
#define SIP_RSEQ_HF_STR                "RSeq"
#define SIP_SERVER_HF_STR              "Server"
#define SIP_SERVICE_ROUTE_HF_STR       "Service-Route"
#define SIP_SESSION_EXPIRES_HF_STR     "Session-Expires"
#define SIP_SUB_STATE_HF_STR           "Subscription-State"
#define SIP_SUPPORTED_HF_STR           "Supported"
#define SIP_TO_HF_STR                  "To"
#define SIP_USER_AGENT_HF_STR          "User-Agent"
#define SIP_VIA_HF_STR                 "Via"
#define SIP_WWW_AUTHENTICATE_HF_STR    "WWW-Authenticate"
#define SIP_P_ACCESS_NW_INFO_HF_STR    "P-Access-Network-Info"
#define SIP_RETRYAFTER_HF_STR          "Retry-After"

/* Compact forms for header field */
#define SIP_CALL_ID_CHF_STR             "i"
#define SIP_CONTACT_CHF_STR             "m"
#define SIP_CONTENT_ENCODING_CHF_STR    "e"
#define SIP_CONTENT_LENGTH_CHF_STR      "l"
#define SIP_CONTENT_TYPE_CHF_STR        "c"
#define SIP_FROM_CHF_STR                "f"
#define SIP_REFER_TO_CHF_STR            "r"
#define SIP_SERVER_CHF_STR              "s"
#define SIP_SUPPORTED_CHF_STR           "k"
#define SIP_TO_CHF_STR                  "t"
#define SIP_VIA_CHF_STR                 "v"

/* Request line args */
#define SIP_URI_TYPE_SIP_STR           "sip"
#define SIP_URI_TYPE_SIPS_STR          "sips"
#define SIP_URI_TYPE_TEL_STR           "tel"
#define SIP_URI_TYPE_IM_STR            "im"
#define SIP_URI_TYPE_URN_STR           "urn"

/* HEADER FIELD ARGS */
#define SIP_TAG_HF_ARG_STR             "tag"
#define SIP_USER_HF_ARG_STR            "user"
#define SIP_PHONE_CXT_HF_ARG_STR       "phone-context"
#define SIP_SESSION_HF_ARG_STR         "session"
#define SIP_BRANCH_HF_ARG_STR          "branch"
#define SIP_RECEIVED_HF_ARG_STR        "received"
#define SIP_RPORT_HF_ARG_STR           "rport"
#define SIP_KEEP_HF_ARG_STR            "keep"
#define SIP_LSKPMC_URI_PARM_STR        "lskpmc"

#define SIP_TRANSPORT_URI_PARM_STR       "transport"
#define SIP_METHOD_URI_PARM_STR          "method"
#define SIP_MADDR_URI_PARM_STR           "maddr"
#define SIP_TTL_URI_PARM_STR             "ttl"
#define SIP_LR_URI_PARM_STR              "lr"
#define SIP_PSBR_URI_PARM_STR            "psbr"
#define SIP_LBFH_URI_PARM_STR            "lbfh"
#define SIP_CONF_URI_PARM_STR            "conf"
#define SIP_FTAG_URI_PARM_STR            "ftag"
#define SIP_GR_URI_PARM_STR              "gr"
#define SIP_TO_TAG_HF_ARG_STR            "to-tag"
#define SIP_FROM_TAG_HF_ARG_STR          "from-tag"
#define SIP_EARLY_FLAG_TAG_HF_ARG_STR    "early-flag"

/* HEADER FIELD ARGS FOR Authorization */

#define SIP_DIGEST_HF_ARG_STR          "Digest"
#define SIP_BASIC_HF_ARG_STR           "Basic"

#define SIP_DOMAIN_HF_ARG_STR          "domain"
#define SIP_USERNAME_HF_ARG_STR        "username"
#define SIP_REALM_HF_ARG_STR           "realm"
#define SIP_NONCE_HF_ARG_STR           "nonce"
#define SIP_QOP_HF_ARG_STR             "qop"
#define SIP_NC_HF_ARG_STR              "nc"
#define SIP_RESPONSE_HF_ARG_STR        "response"
#define SIP_AUTS_HF_ARG_STR            "auts"
#define SIP_ALGORITHM_HF_ARG_STR       "algorithm"
#define SIP_CNONCE_HF_ARG_STR          "cnonce"
#define SIP_OPAQUE_HF_ARG_STR          "opaque"
#define SIP_STALE_HF_ARG_STR           "stale"
#define SIP_URI_HF_ARG_STR             "uri"

#define SIP_REFERRED_BY_ARG_CID        "cid"

#define SIP_SESSION_EXPIRES_REFRESHER_HF_ARG_STR "refresher"
#define SIP_SESSION_EXPIRES_UAC_HF_ARG_STR       "uac"
#define SIP_SESSION_EXPIRES_UAS_HF_ARG_STR       "uas"

#define SIP_CONTACT_HF_Q_ARG_STR                "q"
#define SIP_CONTACT_HF_EXPIRES_ARG_STR          "expires"
#define SIP_CONTACT_HF_USER_ARG_STR             "user"
#define SIP_CONTACT_HF_IM_SESSION_ARG_STR       "+g.oma.sip-im"
#define SIP_CONTACT_HF_IM_CONF_ISFOCUS_ARG_STR  "isfocus"
#define SIP_CONTACT_HF_SIP_INSTANCE_ARG_STR     "+sip.instance"
#define SIP_CONTACT_HF_PUB_GRUU_ARG_STR         "pub-gruu"

/* Capabilities arg string to look up */
#define SIP_CAPS_ARG_SMS_STR           "+g.3gpp.smsip"
#define SIP_CAPS_ARG_VIDEO_SHARE_STR   "+g.3gpp.cs-voice"
#define SIP_CAPS_ARG_IARI_STR          "+g.3gpp.iari-ref"
#define SIP_CAPS_ARG_ICSI_STR          "+g.3gpp.icsi-ref"
#define SIP_CAPS_ARG_RCS_TELEPHONY_STR "+g.gsma.rcs.telephony"

#define SIP_TRANSPORT_UDP_STR "UDP"
#define SIP_TRANSPORT_TCP_STR "TCP"
#define SIP_TRANSPORT_TLS_STR "TLS"

#define SIP_AUTH_ALG_MD5_STR  "MD5"
#define SIP_AUTH_ALG_AKAV1_MD5_STR  "AKAv1-MD5"
#define SIP_AUTH_ALG_AKAV2_MD5_STR  "AKAv2-MD5"
#define SIP_QOP_AUTH_STR      "auth"
#define SIP_QOP_AUTH_INT_STR  "auth-int"

#define SIP_SUBS_HF_ACTIVE_ARG_STR  "active"
#define SIP_SUBS_HF_PEND_ARG_STR    "pending"
#define SIP_SUBS_HF_TERM_ARG_STR    "terminated"

#define SIP_SUBS_HF_EXPIRES_PARM_STR  "expires"
#define SIP_SUBS_HF_REASON_PARM_STR   "reason"

#define SIP_EVENT_HF_REFER_PKG_STR  "refer"
#define SIP_EVENT_HF_CONFERENCE_PKG_STR "conference"

#define SIP_EVENT_HF_PARAM_PARM_STR "param"
#define SIP_EVENT_HF_ID_PARM_STR    "id"

#define SIP_ANONYMOUS_NAME "anonymous"

#define SIP_CONTENT_TYPE_SIPFRAG_STR   "message/sipfrag"
#define SIP_CONTENT_TYPE_SDP_STR       "application/sdp"
#define SIP_CONTENT_TYPE_TEXT_STR      "text/plain"
#define SIP_CONTENT_TYPE_3GPPSMS_STR   "application/vnd.3gpp.sms"
#define SIP_CONTENT_TYPE_MULTIPART_STR "multipart/mixed;boundary=\"++\""
#define SIP_CONTENT_TYPE_MULTIPART_DEC_STR "multipart/mixed"
#define SIP_CONTENT_TYPE_MULTIPART_BOUNDRY_DEC_STR "boundary"
#define SIP_CONTENT_TYPE_RSRC_LISTS    "application/resource-lists+xml"

#define SIP_SUPPORTED_REPLACES_STR   "replaces"
#define SIP_EVENT_NORESOURCES_STR    "noresources"

#define SIP_CONTACT_HF_OMA_SIP_IM_STR         "+g.oma.sip-im"
#define SIP_CONTACT_HF_OMA_SIP_IM_LRG_SMS_STR "+g.oma.sip-im.large-message.sms"
#define SIP_CONTACT_HF_OMA_SIP_IM_LRG_MMS_STR "+g.oma.sip-im.large-message.mms"
#define SIP_CONTACT_HF_OMA_SIP_IM_LRG_MSG_STR "+g.oma.sip-im.large-message"
#define SIP_CONTACT_HF_OMA_SIP_IM_SMS_STR     "+g.oma.sip-im.sms"
#define SIP_CONTACT_HF_OMA_SIP_IM_VCARD_STR   "+g.oma.sip-im.vcard"
#define SIP_CONTACT_HF_OMA_PUSH_CRBT_UA_STR   "+g.oma.pusheventapp=\"crbt.ua\""
#define SIP_CONTACT_HF_OMA_PUSH_FS_UA_STR     "+g.oma.pusheventapp=\"fs.ua\""
#define SIP_CONTACT_HF_OMA_PUSH_MMS_UA_STR    "+g.oma.pusheventapp=\"mms.ua\""
#define SIP_CONTACT_HF_OMA_PUSH_NAB_UA_STR    "+g.oma.pusheventapp=\"nab.ua\""
#define SIP_CONTACT_HF_OMA_PUSH_SMS_UA_STR    "+g.oma.pusheventapp=\"sms.ua\""
#define SIP_CONTACT_HF_OMA_PUSH_VMS_UA_STR    "+g.oma.pusheventapp=\"vms.ua\""

#define SIP_ALERT_INFO_HF_CALL_WAITING_STR    "<urn:alert:service:call-waiting>"

#define SIP_UNKNOWN_STR              "unknown"

#define SIP_DUMMY_STR ""

/* Capabilities string */
#define SIP_CAPS_DISCOVERY_VIA_PRESENCE_STR \
        "urn%3Aurn-7%3A3gpp-application.ims.iari.rcse.dp"
#define SIP_CAPS_IP_VOICE_CALL_STR \
        "urn%3Aurn-7%3A3gpp-service.ims.icsi.mmtel" 
#define SIP_CAPS_IP_VIDEO_CALL_STR SIP_CAPS_IP_VOICE_CALL_STR
#define SIP_CAPS_IP_VIDEO_CALL_SUB_STR ";video"

#define SIP_CAPS_SMS_STR "+g.3gpp.smsip"
#define SIP_CAPS_MESSAGING_STR \
        "urn%3Aurn-7%3A3gpp-service.ims.icsi.oma.cpm.msg;" \
        "urn%3Aurn-7%3A3gpp-service.ims.icsi.oma.cpm.largemsg"
        
#define SIP_CAPS_FILE_TRANSFER_STR \
        "urn%3Aurn-7%3A3gpp-application.ims.iari.rcse.ft"
#define SIP_CAPS_IMAGE_SHARE_STR \
        "urn%3Aurn-7%3A3gpp-application.ims.iari.gsma-is"
#define SIP_CAPS_VIDEO_SHARE_STR "+g.3gpp.cs-voice"
#define SIP_CAPS_VIDEO_SHARE_WITHOUT_CALL_STR \
        "urn%3Aurn-7%3A3gpp-application.ims.iari.gsma-vs"
#define SIP_CAPS_CHAT_STR \
        "urn%3Aurn-7%3A3gpp-application.ims.iari.rcse.im"
#define SIP_CAPS_SOCIAL_PRESENCE_STR \
        "urn%3Aurn-7%3A3gpp-application.ims.iari.rcse.sp" 
#define SIP_CAPS_GEOLOCATION_PUSH_STR \
        "urn%3Aurn-7%3A3gpp-application.ims.iari.rcs.geopush"
#define SIP_CAPS_GEOLOCATION_PULL_STR \
        "urn%3Aurn-7%3A3gpp-application.ims.iari.rcs.geopull"
#define SIP_CAPS_GEOLOCATION_PULL_STR \
        "urn%3Aurn-7%3A3gpp-application.ims.iari.rcs.geopull"
#define SIP_CAPS_FILE_TRANSFER_HTTP_STR \
        "urn%3Aurn-7%3A3gpp-application.ims.iari.rcse.fthttp"
#define SIP_CAPS_FILE_TRANSFER_THUMBNAIL_STR \
        "urn%3Aurn-7%3A3gpp-application.ims.iari.rcse.ftthumb"
#define SIP_CAPS_FILE_TRANSFER_STORE_FWD_STR \
        "urn%3Aurn-7%3A3gpp-application.ims.iari.rcse.ftstandfw"
#define SIP_CAPS_EMERGENCY_REG_STR "sos"
#define SIP_CAPS_RCS_TELEPHONY_NONE_STR "none"
#define SIP_CAPS_RCS_TELEPHONY_CS_STR "cs"
#define SIP_CAPS_RCS_TELEPHONY_VOLTE_STR "volte"
#define SIP_CAPS_RCS_TELEPHONY_VOHSPA_STR "vohspa"
#define SIP_CAPS_SRVCC_ALERTING_STR "+g.3gpp.srvcc-alerting"
#define SIP_CAPS_SRVCC_MID_CALL_STR "+g.3gpp.mid-call"

/* Capabilities type string */
#define SIP_CAPS_TYPE_PLUS_3GPP_STR "+g.3gpp."
#define SIP_CAPS_TYPE_IARI_STR      "+g.3gpp.iari-ref="
#define SIP_CAPS_TYPE_ICSI_STR      "+g.3gpp.icsi-ref="
/* RCS Telephony type */
#define SIP_CAPS_TYPE_RCS_TELEPHONY_STR "+g.gsma.rcs.telephony="

/* Network Access Info string */
#define SIP_PANI_GERAN_STR         "3GPP-GERAN;cgi-3gpp="
#define SIP_PANI_UTRAN_FDD_STR     "3GPP-UTRAN-FDD;utran-cell-id-3gpp="
#define SIP_PANI_UTRAN_TDD_STR     "3GPP-UTRAN-TDD;utran-cell-id-3gpp="
#define SIP_PANI_E_UTRAN_FDD_STR   "3GPP-E-UTRAN-FDD;utran-cell-id-3gpp="
#define SIP_PANI_E_UTRAN_TDD_STR   "3GPP-E-UTRAN-TDD;utran-cell-id-3gpp="
#define SIP_PANI_IEEE_802_11_STR   "IEEE-802.11;i-wlan-node-id="

#endif
