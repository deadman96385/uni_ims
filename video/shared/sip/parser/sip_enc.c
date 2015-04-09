/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 29644 $ $Date: 2014-11-03 19:15:40 +0800 (Mon, 03 Nov 2014) $
 */

#include "sip_sip.h"
#include "sip_token.h"
#include "sip_list.h"
#include "sip_abnfcore.h"
#include "sip_parser_enc.h"
#include "sip_sdp_enc.h"
#include "sip_mem.h"
#include "sip_msgcodes.h"
#include "sip_mem_pool.h"

/* local protoypes */
static vint _ENC_Request          (tFSM *pFSM, tL4Packet *pBuff, tSipIntMsg *pMsg);
static vint _ENC_Response         (tFSM *pFSM, tL4Packet *pBuff, tSipIntMsg *pMsg);
static vint _ENC_HeaderField      (tFSM *pFSM, tL4Packet *pBuff, tSipIntMsg *pMsg, vint);
static vint _ENC_SdpInfo          (tFSM *pFSM, tL4Packet *pBuff, tSipIntMsg *pMsg);
static vint _ENC_NotifyBody       (tFSM *pFSM, tL4Packet *pBuff, tSipIntMsg *pMsg);
static vint _ENC_BinaryMessageBody(tFSM *pFSM, tL4Packet *pBuff, tSipIntMsg *pMsg);
static vint _ENC_Multipart        (tFSM *pFSM, tL4Packet *pBuff, tSipIntMsg *pMsg);
static vint _ENC_UriPlus          (tL4Packet *pBuff, tUriPlus *pUri);

/* Header Field Handlers */
static vint _ENC_Accept            (tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg);
static vint _ENC_AcceptEncoding    (tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg);
static vint _ENC_AcceptLanguage    (tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg);
static vint _ENC_AlertInfo         (tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg);
static vint _ENC_Allow             (tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg);
static vint _ENC_AllowEvents       (tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg);
static vint _ENC_Authorization     (tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg);
static vint _ENC_CallId            (tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg);
static vint _ENC_CSeq              (tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg);
static vint _ENC_Contact           (tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg);
static vint _ENC_ContentDisp       (tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg);
static vint _ENC_ContentEncoding   (tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg);
static vint _ENC_ContentLength     (tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg);
static vint _ENC_ContentType       (tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg);
static vint _ENC_ETag              (tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg);
static vint _ENC_Event             (tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg);
static vint _ENC_Expires           (tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg);
static vint _ENC_From              (tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg);
static vint _ENC_IfMatch           (tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg);
static vint _ENC_MaxForwards       (tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg);
static vint _ENC_MinExpires        (tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg);
static vint _ENC_MinSE             (tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg);
static vint _ENC_Organization      (tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg);
static vint _ENC_ProxyAuthenticate (tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg);
static vint _ENC_ProxyAuthorization(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg);
static vint _ENC_RAck              (tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg);
static vint _ENC_Require           (tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg);
static vint _ENC_Route             (tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg);
static vint _ENC_RecRoute          (tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg);
static vint _ENC_ReferTo           (tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg);
static vint _ENC_ReferredBy        (tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg);
static vint _ENC_Replaces          (tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg);
static vint _ENC_RSeq              (tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg);
static vint _ENC_Server            (tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg);
static vint _ENC_ServiceRoute      (tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg);
static vint _ENC_SessionExpires    (tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg);
static vint _ENC_SubState          (tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg);
static vint _ENC_Supported         (tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg);
static vint _ENC_To                (tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg);
static vint _ENC_UserAgent         (tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg);
static vint _ENC_Via               (tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg);
static vint _ENC_WWWAuthenticate   (tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg);
static vint _ENC_PAccessNwInfo     (tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg);

/* authorization table handlers */
static vint _ENC_DomainArg   (tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg);
static vint _ENC_UsernameArg (tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg);
static vint _ENC_RealmArg    (tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg);
static vint _ENC_NonceArg    (tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg);
static vint _ENC_ResponseArg (tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg);
static vint _ENC_AutsArg     (tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg);
static vint _ENC_QopArg      (tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg);
static vint _ENC_NcArg       (tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg);
static vint _ENC_AlgArg      (tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg);
static vint _ENC_CnonceArg   (tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg);
static vint _ENC_OpaqueArg   (tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg);
static vint _ENC_StaleArg    (tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg);
static vint _ENC_UriArg      (tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg);
static vint _ENC_B64UserPwArg(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg);

/* helpers */
static vint _ENC_PutNumberHelper(tFSM *pFSM, uint32 num, tL4Packet *pBuff);
static vint _ENC_Authenticate(
        tFSM       *pFSM,
        tL4Packet  *pBuff,
        tDLList    *pAuthList,
        const char *pString,
        tSipMsgType msgType);
static vint _ENC_PutAuthString(const char* pString, tL4Packet *pBuff, vint useQuotes);
static vint _ENC_StringHelper(tL4Packet *pBuff, tSipIntMsg *pMsg, tHdrFld hf);
static vint _ENC_RouteHlpr(tL4Packet *pBuff, tDLList *pList);
static vint _ENC_WriteUri(tL4Packet *pBuff, tUri *pUri);
static vint _ENC_ReferToHlpr(vint useBrackets, tL4Packet *pBuff, tReferToHF *pReferTo);
static void _ENC_PutUnknownHF(tL4Packet *pBuff, tSipIntMsg *pMsg);
static vint _ENC_EventHlpr(tL4Packet *pBuff, tEventHF *pEvt);

static const tTokenizer _IntMethodTable [] = 
{
    {  SIP_FIRST_METHOD_STR,    eSIP_FIRST_METHOD, 0 },
    {  SIP_INVITE_METHOD_STR,   eSIP_INVITE,   0},
    {  SIP_CANCEL_METHOD_STR,   eSIP_CANCEL,   0},
    {  SIP_BYE_METHOD_STR,      eSIP_BYE,      0},
    {  SIP_OPTIONS_METHOD_STR,  eSIP_OPTIONS,  0},
    {  SIP_REGISTER_METHOD_STR, eSIP_REGISTER, 0},
    {  SIP_ACK_METHOD_STR,      eSIP_ACK,      0},
    {  SIP_NOTIFY_METHOD_STR,   eSIP_NOTIFY,   0},
    {  SIP_REFER_METHOD_STR,    eSIP_REFER,    0},
    {  SIP_MESSAGE_METHOD_STR,  eSIP_MESSAGE,  0},
    {  SIP_SUBSCRIBE_METHOD_STR,eSIP_SUBSCRIBE,0},
    {  SIP_INFO_METHOD_STR,     eSIP_INFO,     0},
    {  SIP_PRACK_METHOD_STR,    eSIP_PRACK,    0},
    {  SIP_UPDATE_METHOD_STR,   eSIP_UPDATE,   0},
    {  SIP_PUBLISH_METHOD_STR,  eSIP_PUBLISH,  0},
    {  SIP_ERROR_METHOD_STR,    eSIP_ERROR,    0}
};
    
/* 
 * For encoding, the order in this table must remain the same as defined in sip_sip_const.h 
 * THIS IS NOT TRUE FOR DECODING! Having the encoding table in the same order as the
 * header field enumerations defined in sip_sip_const.h has massive advantages in performance 
 */
static const tTokenizer _IntHFTable[] =
{
    { SIP_ACCEPT_HF_STR,             eSIP_ACCEPT_HF,             _ENC_Accept            },
    { SIP_ACCEPT_ENCODING_HF_STR,    eSIP_ACCEPT_ENCODING_HF,    _ENC_AcceptEncoding    },
    { SIP_ACCEPT_LANGUAGE_HF_STR,    eSIP_ACCEPT_LANGUAGE_HF,    _ENC_AcceptLanguage    },
    { SIP_ALERT_INFO_HF_STR,         eSIP_ALERT_INFO_HF,         _ENC_AlertInfo         },
    { SIP_ALLOW_HF_STR,              eSIP_ALLOW_HF,              _ENC_Allow             },
    { SIP_ALLOW_EVENTS_HF_STR,       eSIP_ALLOW_EVENTS_HF,       _ENC_AllowEvents       },
    { SIP_AUTHORIZATION_HF_STR,      eSIP_AUTHORIZATION_HF,      _ENC_Authorization     },
    { SIP_CALL_ID_HF_STR,            eSIP_CALL_ID_HF,            _ENC_CallId            }, 
    { SIP_CONTACT_HF_STR,            eSIP_CONTACT_HF,            _ENC_Contact           }, 
    { SIP_CONTENT_DISP_HF_STR,       eSIP_CONTENT_DISP_HF,       _ENC_ContentDisp       },
    { SIP_CONTENT_ENCODING_HF_STR,   eSIP_CONTENT_ENCODING_HF,   _ENC_ContentEncoding   },
    { SIP_CSEQ_HF_STR,               eSIP_CSEQ_HF,               _ENC_CSeq              },
    { SIP_ETAG_HF_STR,               eSIP_ETAG_HF,               _ENC_ETag              },
    { SIP_EVENT_HF_STR,              eSIP_EVENT_HF,              _ENC_Event             },
    { SIP_EXPIRES_HF_STR,            eSIP_EXPIRES_HF,            _ENC_Expires           },
    { SIP_FROM_HF_STR,               eSIP_FROM_HF,               _ENC_From              }, 
    { SIP_IF_MATCH_HF_STR,           eSIP_IF_MATCH_HF,           _ENC_IfMatch           }, 
    { SIP_MAX_FORWARDS_HF_STR,       eSIP_MAX_FORWARDS_HF,       _ENC_MaxForwards       },
    { SIP_MIN_EXPIRES_HF_STR,        eSIP_MIN_EXPIRES_HF,        _ENC_MinExpires        },
    { SIP_MIN_SE_HF_STR,             eSIP_MIN_SE_HF,             _ENC_MinSE             },
    { SIP_ORGANIZATION_HF_STR,       eSIP_ORGANIZATION_HF,       _ENC_Organization      },
    { SIP_PROXY_AUTHENTICATE_HF_STR, eSIP_PROXY_AUTHENTICATE_HF, _ENC_ProxyAuthenticate },
    { SIP_PROXY_AUTHORIZATION_HF_STR,eSIP_PROXY_AUTHORIZATION_HF,_ENC_ProxyAuthorization},
    { SIP_RACK_HF_STR,               eSIP_RACK_HF,               _ENC_RAck              },
    { SIP_RECORD_ROUTE_HF_STR,       eSIP_RECORD_ROUTE_HF,       _ENC_RecRoute          },
    { SIP_REFER_TO_HF_STR,           eSIP_REFER_TO_HF,           _ENC_ReferTo           },
    { SIP_REFERRED_BY_HF_STR,        eSIP_REFERRED_BY_HF,        _ENC_ReferredBy        },
    { SIP_REPLACES_HF_STR,           eSIP_REPLACES_HF,           _ENC_Replaces          },
    { SIP_REQUIRE_HF_STR,            eSIP_REQUIRE_HF,            _ENC_Require           },
    { SIP_ROUTE_HF_STR,              eSIP_ROUTE_HF,              _ENC_Route             },
    { SIP_RSEQ_HF_STR,               eSIP_RSEQ_HF,               _ENC_RSeq              },
    { SIP_SERVER_HF_STR,             eSIP_SERVER_HF,             _ENC_Server            },
    { SIP_SERVICE_ROUTE_HF_STR,      eSIP_SERVICE_ROUTE_HF,      _ENC_ServiceRoute      },
    { SIP_SESSION_EXPIRES_HF_STR,    eSIP_SESSION_EXPIRES_HF,    _ENC_SessionExpires    },
    { SIP_SUB_STATE_HF_STR,          eSIP_SUB_STATE_HF,          _ENC_SubState          }, 
    { SIP_SUPPORTED_HF_STR,          eSIP_SUPPORTED_HF,          _ENC_Supported         }, 
    { SIP_TO_HF_STR,                 eSIP_TO_HF,                 _ENC_To                }, 
    { SIP_P_ACCESS_NW_INFO_HF_STR,   eSIP_P_ACCESS_NW_INFO_HF,   _ENC_PAccessNwInfo     }, 
    { SIP_USER_AGENT_HF_STR,         eSIP_USER_AGENT_HF,         _ENC_UserAgent         },
    { SIP_VIA_HF_STR,                eSIP_VIA_HF,                _ENC_Via               },
    { SIP_WWW_AUTHENTICATE_HF_STR,   eSIP_WWW_AUTHENTICATE_HF,   _ENC_WWWAuthenticate   },
    { SIP_CONTENT_TYPE_HF_STR,       eSIP_CONTENT_TYPE_HF,       _ENC_ContentType       }, 
    { SIP_CONTENT_LENGTH_HF_STR,     eSIP_CONTENT_LENGTH_HF,     _ENC_ContentLength     }, 
};

/* table used for compact form */
/* 
 * For encoding, the order in this table must remain the same as defined in sipcnst.h 
 * THIS IS NOT TRUE FOR DECODING! Having the encoding table in the same order as the
 * header fiEld enumerations defined in sipcnst.h has massive advantages in performance 
 */
static const tTokenizer _IntCHFTable[] =
{
    { NULL,                        eSIP_ACCEPT_HF,             _ENC_Accept            },
    { NULL,                        eSIP_ACCEPT_ENCODING_HF,    _ENC_AcceptEncoding    },
    { NULL,                        eSIP_ACCEPT_LANGUAGE_HF,    _ENC_AcceptLanguage    },
    { NULL,                        eSIP_ALLOW_HF,              _ENC_Allow             },
    { NULL,                        eSIP_ALLOW_EVENTS_HF,       _ENC_AllowEvents       },
    { NULL,                        eSIP_AUTHORIZATION_HF,      _ENC_Authorization     },
    { SIP_CALL_ID_CHF_STR,         eSIP_CALL_ID_HF,            _ENC_CallId            }, 
    { SIP_CONTACT_CHF_STR,         eSIP_CONTACT_HF,            _ENC_Contact           }, 
    { NULL,                        eSIP_CONTENT_DISP_HF,       _ENC_ContentDisp       },
    { SIP_CONTENT_ENCODING_CHF_STR,eSIP_CONTENT_ENCODING_HF,   _ENC_ContentEncoding   },
    { NULL,                        eSIP_CSEQ_HF,               _ENC_CSeq              },
    { NULL,                        eSIP_ETAG_HF,               _ENC_ETag              },
    { NULL,                        eSIP_EVENT_HF,              _ENC_Event             },
    { NULL,                        eSIP_EXPIRES_HF,            _ENC_Expires           },
    { SIP_FROM_CHF_STR,            eSIP_FROM_HF,               _ENC_From              },
    { NULL,                        eSIP_IF_MATCH_HF,           _ENC_IfMatch           },
    { NULL,                        eSIP_MAX_FORWARDS_HF,       _ENC_MaxForwards       },
    { NULL,                        eSIP_MIN_EXPIRES_HF,        _ENC_MinExpires        },
    { NULL,                        eSIP_MIN_SE_HF,             _ENC_MinSE             },
    { NULL,                        eSIP_ORGANIZATION_HF,       _ENC_Organization      },
    { NULL,                        eSIP_PROXY_AUTHENTICATE_HF, _ENC_ProxyAuthenticate },
    { NULL,                        eSIP_PROXY_AUTHORIZATION_HF,_ENC_ProxyAuthorization},
    { NULL,                        eSIP_RACK_HF,               _ENC_RAck              },
    { NULL,                        eSIP_RECORD_ROUTE_HF,       _ENC_RecRoute          },
    { SIP_REFER_TO_CHF_STR,        eSIP_REFER_TO_HF,           _ENC_ReferTo           },
    { NULL,                        eSIP_REFERRED_BY_HF,        _ENC_ReferredBy        },
    { NULL,                        eSIP_REPLACES_HF,           _ENC_Replaces          },
    { NULL,                        eSIP_REQUIRE_HF,            _ENC_Require           },
    { NULL,                        eSIP_ROUTE_HF,              _ENC_Route             },
    { NULL,                        eSIP_RSEQ_HF,               _ENC_RSeq              },
    { SIP_SERVER_CHF_STR,          eSIP_SERVER_HF,             _ENC_Server            },
    { NULL,                        eSIP_SERVICE_ROUTE_HF,      _ENC_ServiceRoute      },
    { NULL,                        eSIP_SESSION_EXPIRES_HF,    _ENC_SessionExpires    },
    { NULL,                        eSIP_SUB_STATE_HF,          _ENC_SubState          }, 
    { SIP_SUPPORTED_CHF_STR,       eSIP_SUPPORTED_HF,          _ENC_Supported         }, 
    { SIP_TO_CHF_STR,              eSIP_TO_HF,                 _ENC_To                }, 
    { NULL,                        eSIP_P_ACCESS_NW_INFO_HF,   _ENC_PAccessNwInfo     }, 
    { NULL,                        eSIP_USER_AGENT_HF,         _ENC_UserAgent         },
    { SIP_VIA_CHF_STR,             eSIP_VIA_HF,                _ENC_Via               },
    { NULL,                        eSIP_WWW_AUTHENTICATE_HF,   _ENC_WWWAuthenticate   },
    { SIP_CONTENT_TYPE_CHF_STR,    eSIP_CONTENT_TYPE_HF,       _ENC_ContentType       }, 
    { SIP_CONTENT_LENGTH_CHF_STR,  eSIP_CONTENT_LENGTH_HF,     _ENC_ContentLength     }, 
};

/* 
 * This table defines the header fields that have encoding order presidence.
 * In other words, the header fields defined in this table will be the first 
 * to be encoded in the message.  Feel free to modify. 
 */
static const tHdrFld _ENC_HFOrder[] = {
    eSIP_VIA_HF,
    eSIP_MAX_FORWARDS_HF,
    eSIP_FROM_HF,
    eSIP_TO_HF,
    eSIP_CALL_ID_HF,
    eSIP_CSEQ_HF,
    eSIP_CONTACT_HF };

/* handlers for authorization arguments */
static const tTokenizer _IntAuthTable[] = 
{
    {   SIP_DOMAIN_HF_ARG_STR,   eSIP_DOMAIN_HF_ARG,   _ENC_DomainArg   },
    {   SIP_USERNAME_HF_ARG_STR, eSIP_USERNAME_HF_ARG, _ENC_UsernameArg }, 
    {   SIP_REALM_HF_ARG_STR,    eSIP_REALM_HF_ARG,    _ENC_RealmArg    },
    {   SIP_NONCE_HF_ARG_STR,    eSIP_NONCE_HF_ARG,    _ENC_NonceArg    },
    {   SIP_QOP_HF_ARG_STR,      eSIP_QOP_HF_ARG,      _ENC_QopArg      },
    {   SIP_NC_HF_ARG_STR,       eSIP_NC_HF_ARG,       _ENC_NcArg       },
    {   SIP_ALGORITHM_HF_ARG_STR,eSIP_ALGORITHM_HF_ARG,_ENC_AlgArg      },
    {   SIP_CNONCE_HF_ARG_STR,   eSIP_CNONCE_HF_ARG,   _ENC_CnonceArg   },
    {   SIP_OPAQUE_HF_ARG_STR,   eSIP_OPAQUE_HF_ARG,   _ENC_OpaqueArg   },
    {   SIP_STALE_HF_ARG_STR,    eSIP_STALE_HF_ARG,    _ENC_StaleArg    },
    {   SIP_RESPONSE_HF_ARG_STR, eSIP_RESPONSE_HF_ARG, _ENC_ResponseArg },
    {   SIP_AUTS_HF_ARG_STR,     eSIP_AUTS_HF_ARG,     _ENC_AutsArg     },
    {   SIP_URI_HF_ARG_STR,      eSIP_URI_HF_ARG,      _ENC_UriArg      },
    {   SIP_DUMMY_STR,           eSIP_B64_USER_PW_HF_ARG, _ENC_B64UserPwArg },
};

/* table for request line args */
static const tTokenizer _IntUriParmTable[] = 
{
    {   SIP_DUMMY_STR,         eURI_SCHEME_DUMMY,   0    },
    {   SIP_URI_TYPE_SIP_STR,  eURI_SCHEME_SIP,     0    },
    {   SIP_URI_TYPE_SIPS_STR, eURI_SCHEME_SIPS,    0    },
    {   SIP_URI_TYPE_TEL_STR,  eURI_SCHEME_TEL,     0    },
    {   SIP_URI_TYPE_IM_STR,   eURI_SCHEME_IM,      0    },
    {   SIP_URI_TYPE_URN_STR,  eURI_SCHEME_URN,     0    }
};

static const tTokenizer _IntTransTypeTable[] =
{
    {   SIP_TRANSPORT_UDP_STR,  eTransportUdp, 0    },
    {   SIP_TRANSPORT_TCP_STR,  eTransportTcp, 0    },
    {   SIP_TRANSPORT_TLS_STR,  eTransportTls, 0    },
};

static const tTokenizer _IntSubStateArgTable[] =
{
    {   SIP_SUBS_HF_ACTIVE_ARG_STR, eSIP_SUBS_HF_ACTIVE_ARG, 0  },
    {   SIP_SUBS_HF_PEND_ARG_STR,   eSIP_SUBS_HF_PEND_ARG,   0  },
    {   SIP_SUBS_HF_TERM_ARG_STR,   eSIP_SUBS_HF_TERM_ARG,   0  },
};

static const tTokenizer _IntContentType_HFTable[] = 
{
    {   SIP_CONTENT_TYPE_SDP_STR,       eCONTENT_TYPE_SDP,       0 },
    {   SIP_CONTENT_TYPE_SIPFRAG_STR,   eCONTENT_TYPE_SIPFRAG,   0 },
    {   SIP_CONTENT_TYPE_TEXT_STR,      eCONTENT_TYPE_TEXT,      0 },
    {   SIP_CONTENT_TYPE_MULTIPART_STR, eCONTENT_TYPE_MULTIPART, 0 },
    {   SIP_CONTENT_TYPE_3GPPSMS_STR,   eCONTENT_TYPE_3GPPSMS,   0 },
    {   SIP_CONTENT_TYPE_RSRC_LISTS,    eCONTENT_TYPE_RSRC_LISTS, 0}
};

/* Table for capabilities enum to string */
static const tTokenizer _IntCapabilitiesTable[] =
{
    {NULL,                                  eSIP_CAPS_NONE,                     0},
    {SIP_CAPS_DISCOVERY_VIA_PRESENCE_STR,   eSIP_CAPS_DISCOVERY_VIA_PRESENCE,   0},
    {SIP_CAPS_IP_VOICE_CALL_STR,            eSIP_CAPS_IP_VOICE_CALL,            0},
    {SIP_CAPS_IP_VIDEO_CALL_STR,            eSIP_CAPS_IP_VIDEO_CALL,            0},
    {SIP_CAPS_MESSAGING_STR,                eSIP_CAPS_MESSAGING,                0},
    {SIP_CAPS_SMS_STR,                      eSIP_CAPS_SMS,                      0},
    {SIP_CAPS_FILE_TRANSFER_STR,            eSIP_CAPS_FILE_TRANSFER,            0},
    {SIP_CAPS_IMAGE_SHARE_STR,              eSIP_CAPS_IMAGE_SHARE,              0},
    {SIP_CAPS_VIDEO_SHARE_STR,              eSIP_CAPS_VIDEO_SHARE,              0},
    {SIP_CAPS_VIDEO_SHARE_WITHOUT_CALL_STR, eSIP_CAPS_VIDEO_SHARE_WITHOUT_CALL, 0},
    {SIP_CAPS_CHAT_STR,                     eSIP_CAPS_CHAT,                     0},
    {SIP_CAPS_SOCIAL_PRESENCE_STR,          eSIP_CAPS_SOCIAL_PRESENCE,          0},
    {SIP_CAPS_GEOLOCATION_PUSH_STR,         eSIP_CAPS_GEOLOCATION_PUSH,         0},
    {SIP_CAPS_GEOLOCATION_PULL_STR,         eSIP_CAPS_GEOLOCATION_PULL,         0},
    {SIP_CAPS_FILE_TRANSFER_HTTP_STR,       eSIP_CAPS_FILE_TRANSFER_HTTP,       0},
    {SIP_CAPS_FILE_TRANSFER_THUMBNAIL_STR,  eSIP_CAPS_FILE_TRANSFER_THUMBNAIL,  0},
    {SIP_CAPS_FILE_TRANSFER_STORE_FWD_STR,  eSIP_CAPS_FILE_TRANSFER_STORE_FWD,  0},
    {SIP_CAPS_EMERGENCY_REG_STR,            eSIP_CAPS_EMERGENCY_REG,            0},
    {SIP_CAPS_RCS_TELEPHONY_CS_STR,         eSIP_CAPS_RCS_TELEPHONY_CS,         0},
    {SIP_CAPS_RCS_TELEPHONY_VOLTE_STR,      eSIP_CAPS_RCS_TELEPHONY_VOLTE,      0},
    {SIP_CAPS_RCS_TELEPHONY_VOHSPA_STR,     eSIP_CAPS_RCS_TELEPHONY_VOHSPA,     0},
    {SIP_CAPS_RCS_TELEPHONY_NONE_STR,       eSIP_CAPS_RCS_TELEPHONY_NONE,       0},
    {SIP_CAPS_SRVCC_ALERTING_STR,           eSIP_CAPS_SRVCC_ALERTING,           0},
    {SIP_CAPS_SRVCC_MID_CALL_STR,           eSIP_CAPS_SRVCC_MID_CALL,           0},
};

/* Table for capabilities to type string */
static const tTokenizer _IntCapabilitiesTypeTable[] =
{
    {NULL,                             eSIP_CAPS_NONE,                     0},
    {SIP_CAPS_TYPE_IARI_STR,           eSIP_CAPS_DISCOVERY_VIA_PRESENCE,   0},
    {SIP_CAPS_TYPE_ICSI_STR,           eSIP_CAPS_IP_VOICE_CALL,            0},
    {SIP_CAPS_TYPE_ICSI_STR,           eSIP_CAPS_IP_VIDEO_CALL,            0},
    {SIP_CAPS_TYPE_ICSI_STR,           eSIP_CAPS_MESSAGING,                0},
    {SIP_CAPS_TYPE_PLUS_3GPP_STR,      eSIP_CAPS_SMS,                      0},
    {SIP_CAPS_TYPE_IARI_STR,           eSIP_CAPS_FILE_TRANSFER,            0},
    {SIP_CAPS_TYPE_IARI_STR,           eSIP_CAPS_IMAGE_SHARE,              0},
    {SIP_CAPS_TYPE_PLUS_3GPP_STR,      eSIP_CAPS_VIDEO_SHARE,              0},
    {SIP_CAPS_TYPE_IARI_STR,           eSIP_CAPS_VIDEO_SHARE_WITHOUT_CALL, 0},
    {SIP_CAPS_TYPE_IARI_STR,           eSIP_CAPS_CHAT,                     0},
    {SIP_CAPS_TYPE_IARI_STR,           eSIP_CAPS_SOCIAL_PRESENCE,          0},
    {SIP_CAPS_TYPE_IARI_STR,           eSIP_CAPS_GEOLOCATION_PUSH,         0},
    {SIP_CAPS_TYPE_IARI_STR,           eSIP_CAPS_GEOLOCATION_PULL,         0},
    {SIP_CAPS_TYPE_IARI_STR,           eSIP_CAPS_FILE_TRANSFER_HTTP,       0},
    {SIP_CAPS_TYPE_IARI_STR,           eSIP_CAPS_FILE_TRANSFER_THUMBNAIL,  0},
    {SIP_CAPS_TYPE_IARI_STR,           eSIP_CAPS_FILE_TRANSFER_STORE_FWD,  0},
    {SIP_DUMMY_STR,                    eSIP_CAPS_EMERGENCY_REG,            0},
    {SIP_CAPS_TYPE_RCS_TELEPHONY_STR,  eSIP_CAPS_RCS_TELEPHONY_CS,         0},
    {SIP_CAPS_TYPE_RCS_TELEPHONY_STR,  eSIP_CAPS_RCS_TELEPHONY_VOLTE,      0},
    {SIP_CAPS_TYPE_RCS_TELEPHONY_STR,  eSIP_CAPS_RCS_TELEPHONY_VOHSPA,     0},
    {SIP_CAPS_TYPE_RCS_TELEPHONY_STR,  eSIP_CAPS_RCS_TELEPHONY_NONE,       0},
    {SIP_DUMMY_STR,                    eSIP_CAPS_SRVCC_ALERTING,           0},
    {SIP_DUMMY_STR,                    eSIP_CAPS_SRVCC_MID_CALL,           0},
};

/* Table for capaiblities type to string */
static const tTokenizer _IntCapsTypeStringTable[] =
{
    { SIP_CAPS_TYPE_PLUS_3GPP_STR,      eSIP_CAPS_TYPE_PLUS_3GPP,     0},
    { SIP_CAPS_TYPE_ICSI_STR,           eSIP_CAPS_TYPE_ICSI,          0},
    { SIP_CAPS_TYPE_IARI_STR,           eSIP_CAPS_TYPE_IARI,          0},
    { SIP_CAPS_TYPE_RCS_TELEPHONY_STR,  eSIP_CAPS_TYPE_RCS_TELEPHONY, 0},
    { SIP_DUMMY_STR,                    eSIP_CAPS_TYPE_OTHERS,        0},
};

/* 
 *****************************************************************************
 * ================ENC_Msg()===================
 *
 * This function encodes an internal tSipIntMsg object into an external SIP message 
 *
 * pMsg = A pointer to a tSipIntMsg to be encoded.
 *
 * pBuff = A pointer to an object used to manage encoding the internal SIP
 *         format.
 *
 * useCompactForm = TRUE: Use compact form. FALSE: Don't
 *
 * RETURNS:
 *         SIP_OK: Successful
 *         SIP_FAILED: Could not encode message
 *
 ******************************************************************************
 */
vint ENC_Msg(
    tSipIntMsg *pMsg,
    tL4Packet  *pBuff,
    vint        useCompactForm)
{
    tFSM   MsgFSM;
    vint   status;

    OSAL_memSet(&MsgFSM, 0, sizeof(tFSM));

    if (pMsg->msgType == eSIP_REQUEST)
        MsgFSM.pfHandler = (tpfTokenHndlr)_ENC_Request;
    else
        MsgFSM.pfHandler = (tpfTokenHndlr)_ENC_Response;

    status = MsgFSM.pfHandler(&MsgFSM, pBuff, pMsg);
    if (status != SIP_OK) {
        goto finishUp;
    }

    status = _ENC_HeaderField(&MsgFSM, pBuff, pMsg, useCompactForm);
    if (status != SIP_OK) {
        goto finishUp;
    }
        
    if (MsgFSM.isEndOfPacket) {
        goto finishUp;
    }

    if (pMsg->ContentLength == 0) {
        TOKEN_Put("", ABNF_EOL, pBuff);
    }

finishUp:
    /* Not technically a requirement...but nice for diagnostic output */
    *pBuff->pCurr = '\0';
    return status;
}

/* 
 *****************************************************************************
 * ================ENC_Uri()===================
 *
 * This function encodes a uri from a pointer to a tUri object. 
 *
 * "sip:sparrish@d2tech.com"
 * "sips:randmaa@client.d2tech.com:5060;maddr=10.1.1.1;transport=udp"
 *
 * pUri = A pointer to a tUri object to encode
 *
 * pTarget = A pointer to the target string to write the uri to
 *
 * pLength = A pointer to an unsigned int that will be populated with the 
 *           length of the encoded string.
 *
 * useEscChars  = If '1' then special chars in the username will be used
 *                (i.e. @ = %40 ) 
 *
 * RETURNS:
 *         SIP_OK: The uri was valid and decoded
 *         SIP_FAILED: The string could not be decoded.
 *
 ******************************************************************************
 */
vint ENC_Uri(
    tUri    *pUri,
    char    *pTarget,
    uint32  *pLength,
    vint     useEscChars)
{
    uint32 x;
    uint32 size;
    vint status;
    tL4Packet pkt;
    char s[MAX_IPV6_STR_LEN];
    char tmpStr[MAX_IPV6_STR_LEN];
    char at[2] = "@";
   
    pkt.frame = pTarget;
    pkt.pCurr = pkt.pStart = pTarget;
    pkt.length = *pLength;
    pkt.isOutOfRoom = FALSE;

    /* place the uri 'scheme' if one exists */
    if (TOKEN_IntLookup(pUri->scheme, _IntUriParmTable, eURI_SCHEME_LAST, &x) == SIP_OK) {
        if (x != 0) {
            if((status = TOKEN_Put(_IntUriParmTable[x].pExt, ":", &pkt)) != SIP_OK) {
                *pLength = 0;
                return status;
            }
        }
    }

    /* Process URN */
    if (eURI_SCHEME_URN == pUri->scheme) {
        if (pUri->uid[0] != 0) {
            if ((status = TOKEN_Put("", pUri->uid, &pkt)) != SIP_OK) {
                *pLength = 0;
                return status;
            }
        }
        if (pUri->uss[0] != 0) {
            if ((status = TOKEN_Put(":", pUri->uss, &pkt)) != SIP_OK) {
                *pLength = 0;
                return status;
            }
        }
    }

    /* now place the name */
    if (pUri->user[0] != 0) {
        if (eURI_SCHEME_TEL == pUri->scheme) {
            at[0] = 0;
            /* Don't allow special chars for tel scheme...override this */
            useEscChars = 0;
        }
        if (useEscChars) {
            if ((status = TOKEN_PutSpecChar(pUri->user, at, &pkt)) != SIP_OK) {
                *pLength = 0;
                return status;
            }
        }
        else {
            if ((status = TOKEN_Put(pUri->user, at, &pkt)) != SIP_OK) {
                *pLength = 0;
                return status;
            }
        }
    }
    
    /* now place the address */
    if (pUri->host.addressType == eNwAddrIPv6) {
        /* ipv6 */
        VoIP_IpV6Int2Ext(pUri->host.x.ip, tmpStr);
        OSAL_snprintf(s, sizeof(s), "[%s]", tmpStr);
        if ((status = TOKEN_Put(s, "", &pkt)) != SIP_OK) {
            *pLength = 0;    
            return status;
        }
    }
    else if (pUri->host.addressType == eNwAddrIPv4) {
        /* ipv4 */
        VoIP_IpV4Int2Ext(pUri->host.x.ip, s);
        if ((status = TOKEN_Put(s, "", &pkt)) != SIP_OK) {
            *pLength = 0;    
            return status;
        }
    }
    else {
        /* must be a domain name */
        if ((status = TOKEN_Put(pUri->host.x.domainName, "", &pkt)) != SIP_OK) {
            *pLength = 0;
            return status;
        }
    }
    
    /* place the port */
    if (pUri->host.port != 0) {
        /* now the port number */
        OSAL_itoa(pUri->host.port, s, MAX_IPV6_STR_LEN);
        if ((status = TOKEN_Put(":", s, &pkt)) != SIP_OK) {
            *pLength = 0;
            return status;
        }
    }

    /* place gruu */
    if (0 != pUri->szGruu[0]) {
        TOKEN_Put(";", SIP_GR_URI_PARM_STR, &pkt);
        TOKEN_Put("=", "", &pkt);
        if ((status = TOKEN_Put(pUri->szGruu, "", &pkt)) != SIP_OK) {
            *pLength = 0;
            return status;
        }
    }

    /* place the lskpmc */
    if (pUri->szLskpmc[0] != 0) {
        TOKEN_Put(";", SIP_LSKPMC_URI_PARM_STR, &pkt);
        TOKEN_Put("=", pUri->szLskpmc, &pkt);
    }

    /* place the lr */
    if (pUri->lr) {
        TOKEN_Put(";", SIP_LR_URI_PARM_STR, &pkt);
    }

    /* now the maddr */
    if (pUri->host.addressType == eNwAddrIPv6) {
        if (!OSAL_netIpv6IsAddrZero(pUri->maddr.v6)) {
            if (VoIP_IpV6Int2Ext(pUri->maddr, tmpStr) == SIP_OK) {
                TOKEN_Put(";", SIP_MADDR_URI_PARM_STR, &pkt);
                OSAL_snprintf(s, sizeof(s), "[%s]", tmpStr);
                TOKEN_Put("=", s, &pkt);
            }
            else {
                *pLength = 0;
                return (SIP_FAILED);
            }
        }
    }
    else if (pUri->host.addressType == eNwAddrIPv4) {
        if (pUri->maddr.v4.ul != 0) {
            if (VoIP_IpV4Int2Ext(pUri->maddr, s) == SIP_OK) {
                TOKEN_Put(";", SIP_MADDR_URI_PARM_STR, &pkt);
                TOKEN_Put("=", s, &pkt);
            }
            else {
                *pLength = 0;
                return (SIP_FAILED);
            }
        }
    }

    /* now the ttl */
    if (pUri->ttl != 0) {
        OSAL_itoa(pUri->ttl, s, MAX_IPV6_STR_LEN);
        TOKEN_Put(";", SIP_TTL_URI_PARM_STR, &pkt);
        TOKEN_Put("=", s, &pkt);
    }

    /* now the transport type */
    if (pUri->transport != eTransportNone) {
        size = (sizeof(_IntTransTypeTable) / sizeof(_IntTransTypeTable[0]));
        if (TOKEN_IntLookup(pUri->transport, _IntTransTypeTable, size, &x) == SIP_OK) {
            TOKEN_Put(";", SIP_TRANSPORT_URI_PARM_STR, &pkt);
            TOKEN_Put("=", _IntTransTypeTable[x].pExt, &pkt);
        }
        else {
            SIP_DebugLog(SIP_DB_ENCODE_LVL_1, "ENC_Uri: Failed encoding Unknown transtype :%d", 
                    (int)pUri->transport, 0, 0);
            *pLength = 0;
            return (SIP_FAILED);
        }
    }
    /* now the 'user' */
    if (pUri->szUserParam[0] != 0) {
        TOKEN_Put(";", SIP_USER_HF_ARG_STR, &pkt);
        TOKEN_Put("=", pUri->szUserParam, &pkt);
    }

    /* now the 'phone-context' */
    if (pUri->szPhoneCxtParam[0] != 0) {
        TOKEN_Put(";", SIP_PHONE_CXT_HF_ARG_STR, &pkt);
        TOKEN_Put("=", pUri->szPhoneCxtParam, &pkt);
    }

    /* now the 'session' */
    if (pUri->szSessionParam[0] != 0) {
        TOKEN_Put(";", SIP_SESSION_HF_ARG_STR, &pkt);
        TOKEN_Put("=", pUri->szSessionParam, &pkt);
    }

    /* Now the "psbr" or "lbfh" */
    if (pUri->arg.szPsbr[0] != 0) {
        /* Then there's something to write */
        if (pUri->argType == eURI_ARG_TYPE_LBFH) {
            TOKEN_Put(";", SIP_LBFH_URI_PARM_STR, &pkt);
        }
        else {
            TOKEN_Put(";", SIP_PSBR_URI_PARM_STR, &pkt);
        }
        TOKEN_Put("=", pUri->arg.szPsbr, &pkt);
    }

    /* place the conf */
    if (pUri->szConf[0] != 0) {
        TOKEN_Put(";", SIP_CONF_URI_PARM_STR, &pkt);
        TOKEN_Put("=", pUri->szConf, &pkt);
    }

    /* place the ftag */
    if (pUri->szFtag[0] != 0) {
        TOKEN_Put(";", SIP_FTAG_URI_PARM_STR, &pkt);
        TOKEN_Put("=", pUri->szFtag, &pkt);
    }

    /* now the method */
#if 1
    /* WARNING: this needs more support.  It's a problem because we
     * don't want to populate it if it's zero.  The problem is that 
     * eSIP_INVITE is a value of zero
     */
    if (pUri->method != eSIP_ERROR && pUri->method != eSIP_FIRST_METHOD) {
        if (TOKEN_IntLookup(pUri->method, _IntMethodTable, eSIP_LAST_METHOD, &x) == SIP_OK) {
            TOKEN_Put(";", SIP_METHOD_URI_PARM_STR, &pkt);
            TOKEN_Put("=", _IntMethodTable[x].pExt, &pkt);
        }
        else {
            SIP_DebugLog(SIP_DB_ENCODE_LVL_1, "ENC_Uri: Failed encoding Unknown method :%d", 
                    (int)pUri->method, 0, 0);
            return (SIP_FAILED);
        }
    }
#endif

    /* calculate the length */
    *pLength = EXTBUF_LEN(&pkt);
    return status;
}

vint ENC_Event(
    tEventHF   *pEvent,
    char       *pTarget,
    uint32     *pLength)
{
    vint      status;
    tL4Packet pkt;
    
    pkt.frame = pTarget;
    pkt.pCurr = pkt.pStart = pTarget;
    pkt.length = *pLength;
    pkt.isOutOfRoom = FALSE;

    /* place "Event:" */
    if((status = TOKEN_Put(SIP_EVENT_HF_STR, ": ", &pkt)) != SIP_OK) {
        return status;
    }
    return _ENC_EventHlpr(&pkt,  pEvent);
}

vint ENC_ReferTo(
    tReferToHF *pReferTo,
    char       *pTarget,
    uint32     *pLength)
{
    vint status;
    tL4Packet pkt;

    pkt.frame = pTarget;
    pkt.pCurr = pkt.pStart = pTarget;
    pkt.length = *pLength;
    pkt.isOutOfRoom = FALSE;    

    status = _ENC_ReferToHlpr(FALSE, &pkt, pReferTo);
    *pLength = (pkt.pCurr - pkt.pStart);
    return status;
}

vint ENC_Route(
    tDLList    *pRouteList,
    char       *pTarget,
    uint32     *pLength)
{
    vint status;
    tL4Packet pkt;

    pkt.frame = pTarget;
    pkt.pCurr = pkt.pStart = pTarget;
    pkt.length = *pLength;
    pkt.isOutOfRoom = FALSE;  
    
    status = _ENC_RouteHlpr(&pkt, pRouteList);
    *pLength = (pkt.pCurr - pkt.pStart);
    return status;
}

static vint _ENC_StringHelper(
    tL4Packet  *pBuff,
    tSipIntMsg *pMsg,
    tHdrFld     hf)
{
    tHdrFldList *pCurr;
    pCurr = pMsg->pHFList;
    while (pCurr) {
        if (hf == pCurr->hf) {
            /* found it */
            return TOKEN_Put(pCurr->pField, SIP_CRLF, pBuff);
        }
        pCurr = pCurr->pNext;
    }
    return TOKEN_Put(SIP_CRLF, "", pBuff);
}

static void _ENC_PutUnknownHF(
    tL4Packet  *pBuff,
    tSipIntMsg *pMsg)
{
    tHdrFldList *pCurr;
    pCurr = pMsg->pHFList;
    while (pCurr) {
        if (eSIP_LAST_HF == pCurr->hf) {
            /* found it */
            TOKEN_Put(pCurr->pStart, SIP_CRLF, pBuff);
        }
        pCurr = pCurr->pNext;
    }
    return;
}

static vint _ENC_PutNumberHelper(
    tFSM *pFSM,
    uint32 num,
    tL4Packet *pBuff)
{
    char str[SIP_MAX_BASETEN_NUM_STRING];

    UNUSED(pFSM);

    if (0 >= (OSAL_itoa(num, str, SIP_MAX_BASETEN_NUM_STRING)))
        return (SIP_FAILED);
    else
        return TOKEN_Put(str, SIP_CRLF, pBuff);
}

static vint _ENC_Response(
    tFSM *pFSM,
    tL4Packet *pBuff,
    tSipIntMsg *pMsg)
{
    /* encode the protocol and version */
    uint16 code;
    vint status = SIP_OK;
    const char *pStr;
    char str[SIP_MAX_BASETEN_NUM_STRING];
    
    UNUSED(pFSM);

    if (pMsg->msgType != eSIP_RESPONSE)
        return (SIP_FAILED);
    
    /* now place the protocol-version */
    if ((status = TOKEN_Put(SIP_VERSION_STR, " ", pBuff)) != SIP_OK)
       return status;

    /* now put the code */
    code = (uint16)MSGCODE_GetNum(pMsg->code);
    if (0 >= OSAL_itoa(code, str, SIP_MAX_BASETEN_NUM_STRING))
        return (SIP_FAILED);
    
    TOKEN_Put(str, " ", pBuff);
    
    /* now put an appropriate string for the code */
    if (pMsg->pReasonPhrase)
        pStr = pMsg->pReasonPhrase->msg;
    else 
        pStr = MSGCODE_GetStr(pMsg->code);

    return TOKEN_Put(pStr, SIP_CRLF, pBuff);
}


/* table specifiying the order of auth arguments in reqeusts */
static tSipHdrAuthHFArg _AuthArgOrderReq[] = {
    eSIP_B64_USER_PW_HF_ARG, /* this one is for 'basic' authentication only */   
    eSIP_USERNAME_HF_ARG,   
    eSIP_REALM_HF_ARG,
    eSIP_NONCE_HF_ARG,
    eSIP_URI_HF_ARG,
    eSIP_RESPONSE_HF_ARG,
    eSIP_AUTS_HF_ARG,
    eSIP_OPAQUE_HF_ARG, 
};

static tSipHdrAuthHFArg _AuthArgOrderResp[] = {
    eSIP_REALM_HF_ARG,     
    eSIP_DOMAIN_HF_ARG,     
    eSIP_NONCE_HF_ARG,
    eSIP_OPAQUE_HF_ARG, 
    eSIP_STALE_HF_ARG,
    eSIP_ALGORITHM_HF_ARG,
    eSIP_QOP_HF_ARG,  
};    

/*
 * ======== _ENC_Authenticate() ========
 * This function creates the Authentication message. This is used in common by
 * all the authorization functions.
 * This common authenticate function checks if each of the Authentication fields
 * is NULL. if there are any fields to parse , it just puts it in the required
 * format. This function returns SIP_OK once it is done
 * 
 * RETURNS OK - Function was successful
 */
static vint _ENC_Authenticate(
        tFSM       *pFSM,
        tL4Packet  *pBuff,
        tDLList    *pAuthList,
        const char *pString,
        tSipMsgType msgType)
{
    tDLListEntry *pEntry;
    vint status = SIP_OK;
    vint flag = FALSE;
    
    pEntry = NULL;
    while (DLLIST_GetNext(pAuthList, &pEntry)) {
        tAuthorizationHFE *pAuth = (tAuthorizationHFE *)pEntry;
        uint32 orderSize = 0;
        uint32 bitmap = pAuth->presence;
        uint32 cnt;

        if (flag) {
             TOKEN_Put(pString, ": ", pBuff);
        }

        flag = TRUE;

        if (pAuth->type == eSIP_AUTH_TYPE_DIGEST) {
            TOKEN_Put(SIP_DIGEST_HF_ARG_STR, " ", pBuff);
        }
        else {
            TOKEN_Put(SIP_BASIC_HF_ARG_STR, " ", pBuff);
        }
        if (msgType == eSIP_REQUEST)
            orderSize = (sizeof(_AuthArgOrderReq)/sizeof(tSipHdrAuthHFArg));
        else
            orderSize = (sizeof(_AuthArgOrderResp)/sizeof(tSipHdrAuthHFArg));

        for (cnt = 0 ; cnt < orderSize ; cnt++) {
            /* first do the ones that are required by order */
            tSipHdrAuthHFArg arg;
            if (msgType == eSIP_REQUEST)
                arg = _AuthArgOrderReq[cnt];
            else
                arg = _AuthArgOrderResp[cnt];

            if (HF_ARG_PRESENCE_EXISTS(&bitmap, arg)) {
                /* put the token and value, for basic auth using the 
                    B64UserPw variable there is no token, just a value */
                if (arg != eSIP_B64_USER_PW_HF_ARG) {
                    if ((status = TOKEN_Put(_IntAuthTable[arg].pExt, "=", pBuff)) != SIP_OK) {
                        return status;
                    }
                }
                pFSM->pfHandler = (tpfTokenHndlr)_IntAuthTable[arg].pfHandler;
                if ((status = pFSM->pfHandler(pFSM, pBuff, pAuth)) != SIP_OK) {
                    return status;
                }
                HF_ARG_CLR_PRESENCE(&bitmap, arg);
                if (bitmap)
                    TOKEN_Put(",", " ", pBuff);
                else
                    break;
            }
        }
        cnt = 0;
        while (bitmap != 0) {
            if (bitmap & 0x01  && cnt < eSIP_LAST_AUTH_ARG) {
                if (cnt != eSIP_B64_USER_PW_HF_ARG) {
                    /* place the cnt token */
                    if ((status = TOKEN_Put(_IntAuthTable[cnt].pExt, "=", pBuff)) != SIP_OK) {
                        return status;
                    }
                }
                pFSM->pfHandler = (tpfTokenHndlr)_IntAuthTable[cnt].pfHandler;
                if ((status = pFSM->pfHandler(pFSM, pBuff, pAuth)) != SIP_OK) {
                    return status;
                }
                bitmap = bitmap >> 1;
                if (bitmap)
                    TOKEN_Put(",", " ", pBuff);
            }
            else {
                bitmap = bitmap >> 1;
            }
            
            cnt++;
        }

        TOKEN_Put("", SIP_CRLF, pBuff);
    }/* end of the while loop */
    if (flag == FALSE) TOKEN_Put("", SIP_CRLF, pBuff);
    return status;
}

/*
 * ======== _ENC_PutAuthString() ========
 * Helper function for adding "=" and "," as required by the SIP message
 * 
 * RETURNS OK - Function was successful
 */
static vint _ENC_PutAuthString(const char* pString, tL4Packet *pBuff, vint useQuotes) 
{
    if (useQuotes) {
        TOKEN_Put("\"", "", pBuff);
    }
    return TOKEN_Put(pString, useQuotes?"\"":"", pBuff);
}

static vint _ENC_Request(
    tFSM       *pFSM,
    tL4Packet  *pBuff,
    tSipIntMsg *pMsg)
{
    /* encode the method */
    vint status;
    uint32 x;

    UNUSED(pFSM);

    if (TOKEN_IntLookup(pMsg->method, _IntMethodTable, eSIP_LAST_METHOD, &x) != SIP_OK )
         return (SIP_FAILED);

    if ((status = TOKEN_Put(_IntMethodTable[x].pExt, " ", pBuff)) != SIP_OK)
        return status;

    /* put the target URI */
    if ((status = _ENC_WriteUri(pBuff, &pMsg->requestUri)) != SIP_OK)
        return status;

    TOKEN_Put(" ", "", pBuff);

    /* now place the protocol-version */
    return TOKEN_Put(SIP_VERSION_STR, SIP_CRLF, pBuff);
}



static vint _ENC_HeaderField(
    tFSM       *pFSM,
    tL4Packet  *pBuff,
    tSipIntMsg *pMsg,
    vint        useCompact)
{
    /* get the bit map representing what fields to populate */
    tHdrFld hf;
    uint32 orderSize = (sizeof(_ENC_HFOrder)/sizeof(tHdrFld));
    uint32 cnt;
    vint status;
    const tTokenizer *pToken;
    tPres64Bits presBits = pMsg->x.ECPresenceMasks;
    
    /* first do the ones that are required by order */
    for (cnt = 0 ; cnt < orderSize ; cnt++) {
        hf = _ENC_HFOrder[cnt]; 
        if (HF_PresenceExists(&presBits, hf)) {
            HF_ClrPresence(&presBits, hf);
            /* then do something about it */
            pToken = (useCompact && (_IntCHFTable[hf].pExt != NULL)) ? &_IntCHFTable[hf] : &_IntHFTable[hf];
            if ((status = TOKEN_Put(pToken->pExt, ": ", pBuff)) != SIP_OK) {
                return status;
            }
            
            pFSM->pfHandler = (tpfTokenHndlr)pToken->pfHandler;
            if ((status = pFSM->pfHandler(pFSM, pBuff, pMsg)) != SIP_OK) {
                SIP_DebugLog(SIP_DB_ENCODE_LVL_1, "_ENC_HeaderField: Failed hMsg:%X when encoding hf:%s", 
                (int)pMsg, (int)pToken->pExt, 0);
                return status;
            }
        }
    } /* end of the for loop */


    /* now the other ones */
    hf = eSIP_HF_START;
    while (hf < eSIP_LAST_HF) {
        if (hf == eSIP_CONTENT_TYPE_HF) {
            /* 
             * Now place all "unknown" header fields before we attempt to 
             * include the content-type header field.  This way the 
             * content-type and content-length header fields are always last. 
             */
            _ENC_PutUnknownHF(pBuff, pMsg);
        }
        if (HF_PresenceExists(&presBits, hf)) {
            /* find and call the hf handler for this */
            pToken = (useCompact && (_IntCHFTable[hf].pExt != NULL)) ? &_IntCHFTable[hf] : &_IntHFTable[hf];
            if ((status = TOKEN_Put(pToken->pExt, ": ", pBuff)) != SIP_OK) {
                return status;
            }
            
            pFSM->pfHandler = (tpfTokenHndlr)_IntHFTable[hf].pfHandler;
            if ((status = pFSM->pfHandler(pFSM, pBuff, pMsg)) != SIP_OK) {
                SIP_DebugLog(SIP_DB_ENCODE_LVL_1, "_ENC_HeaderField: Failed hMsg:%X when encoding hf:%s", 
                (int)pMsg, (int)pToken->pExt, 0);
                return status;
            }
        }
        hf++;
    } /* end of while loop */

    return (SIP_OK);
}

void SipEncodeInit(void)
{
    return;
}

static vint _ENC_Authorization(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg)
{
    tSipIntMsg *pMsg = (tSipIntMsg*)hMsg;
    return _ENC_Authenticate(pFSM, pBuff, &pMsg->AuthorizationList, 
            SIP_AUTHORIZATION_HF_STR, pMsg->msgType);
}

static vint _ENC_CallId(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg)
{
    tSipIntMsg *pMsg = (tSipIntMsg*)hMsg;
    return TOKEN_Put(pMsg->szCallId, SIP_CRLF, pBuff);
}

static vint _ENC_ETag(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg)
{
    return _ENC_StringHelper(pBuff, hMsg, eSIP_ETAG_HF); 
}

static vint _ENC_IfMatch(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg)
{
    return _ENC_StringHelper(pBuff, hMsg, eSIP_IF_MATCH_HF); 
}

static vint _ENC_ContentLength(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg)
{
    tSipIntMsg *pMsg = (tSipIntMsg*)hMsg;
    int len;
    tL4Packet pkt;
    tSipMsgBody *pMsgBody;
    char *pBody;
    tFSM fsm;
    vint status = SIP_OK;

    pMsgBody = (tSipMsgBody *)SIP_memPoolAlloc(eSIP_OBJECT_SIP_MSG_BODY);
    if (NULL == pMsgBody) {
        return (SIP_NO_MEM);
    }

    pBody = pMsgBody->msg;

    pkt.frame  = (tSipHandle)pBody;
    pkt.pCurr  = pBody;
    pkt.pStart = pBody;
    pkt.length = PAYLOAD_SCRATCH_PAD_SIZE;
    pkt.isOutOfRoom = FALSE;
    
    OSAL_memSet(&fsm, 0, sizeof(tFSM));

    if (pMsg->ContentType == eCONTENT_TYPE_SDP) {
        /* first encode the SDP information before setting the content length */
        if (_ENC_SdpInfo(&fsm, &pkt, pMsg) != SIP_OK) {
            SIP_DebugLog(SIP_DB_ENCODE_LVL_1, "_ENC_ContentLength: Error could encode 'SDP' hMsg:%X", 
                    (int)hMsg, 0, 0);
            
            status = SIP_FAILED;
            goto errorExit;
        }
    }
    else if (pMsg->ContentType == eCONTENT_TYPE_MULTIPART) {
        /* For YTL! Encode sdp and then anything in the text message. */
        if (_ENC_Multipart(&fsm, &pkt, pMsg) != SIP_OK) {
            SIP_DebugLog(SIP_DB_ENCODE_LVL_1, "_ENC_ContentLength: Error could encode 'Multipart/mixed' hMsg:%X",
                    (int)hMsg, 0, 0);

            status = SIP_FAILED;
            goto errorExit;
        }
    }
    else if (pMsg->ContentType == eCONTENT_TYPE_SIPFRAG) {
        if (_ENC_NotifyBody(&fsm, &pkt, pMsg) != SIP_OK) {
            SIP_DebugLog(SIP_DB_ENCODE_LVL_1, "_ENC_ContentLength: Error encoding 'Notify Body' hMsg:%X", 
                    (int)hMsg, 0, 0);
            
            status = SIP_FAILED;
            goto errorExit;
        }
    }
    else if (pMsg->ContentType > eCONTENT_TYPE_NONE &&
             pMsg->ContentType <= eCONTENT_TYPE_LAST) {
        if (_ENC_BinaryMessageBody(&fsm, &pkt, pMsg) != SIP_OK) {
            SIP_DebugLog(SIP_DB_ENCODE_LVL_1, "_ENC_ContentLength: Error encoding 'Text Message Body' hMsg:%X", 
                    (int)hMsg, 0, 0);
            
            status = SIP_FAILED;
            goto errorExit;
        }
    }
    len = EXTBUF_LEN(&pkt);
    /* SIP_DebugLog(SIP_DB_SDP_ENC_LVL_3, "_ENC_ContentLength: encoded 'message body' (len=%d.) %s", len, (int)pBody, 0); */
    if (len >= 4) {
        /* then subtract the first and last set of <cr>,<ln> chars */
        pMsg->ContentLength = len - 4;
        /* Content lengh handling for different content type */
        if ((eCONTENT_TYPE_SIPFRAG == pMsg->ContentType) ||
                (eCONTENT_TYPE_MULTIPART == pMsg->ContentType) ||
                (eCONTENT_TYPE_SDP == pMsg->ContentType)) {
            /* 
             * Add 2 to ContentLength for the last <cr>,<ln>. 
             * this is sometimes an interop issue.
             */
            _ENC_PutNumberHelper(pFSM, pMsg->ContentLength + 2, pBuff);
        }
        else {
            /*
             * Don't add 2 to binary message body content length.  it causes 400 Bad Request
             * when sending 3GPP2 SMS in Verizon network.
             */
            _ENC_PutNumberHelper(pFSM, pMsg->ContentLength, pBuff);
        }
        OSAL_memCpy(pBuff->pCurr, pBody, len);
        pBuff->pCurr += len; 
    }
    else {
        _ENC_PutNumberHelper(pFSM, pMsg->ContentLength, pBuff);
    }
    /*SIP_DebugLog(SIP_DB_SDP_ENC_LVL_3, "_ENC_ContentLength: encoded message %s", (int)pBuff->pStart, 0, 0);*/

errorExit:
    SIP_memPoolFree(eSIP_OBJECT_SIP_MSG_BODY, (tDLListEntry *)pMsgBody);
    return (status);
}

static vint _ENC_NotifyBody(tFSM *pFSM, tL4Packet *pBuff, tSipIntMsg *pMsg)
{
    vint  status;
    uint32   code;
    char  str[SIP_MAX_BASETEN_NUM_STRING];
    const char *pStr;

    /* space out the body with a empty line */
    TOKEN_Put(ABNF_EOL, "", pBuff);

    /* now place the protocol-version */
    if ((status = TOKEN_Put(SIP_VERSION_STR, " ", pBuff)) != SIP_OK)
       return status;

    /* now put the code */
    code = (uint16)MSGCODE_GetNum(pMsg->sipfragCode);
    if (0 >= OSAL_itoa(code, str, SIP_MAX_BASETEN_NUM_STRING))
        return (SIP_FAILED);

    if ((status = TOKEN_Put(str, " ", pBuff)) != SIP_OK)
       return status;

    /* now put the appropriate string for the code */
    pStr = MSGCODE_GetStr(pMsg->sipfragCode);
    if (pStr) {
        return TOKEN_Put(pStr, SIP_CRLF, pBuff);
    }
    else {
        return TOKEN_Put(SIP_CRLF, "", pBuff);
    }
}

static vint _ENC_Multipart(tFSM *pFSM, tL4Packet *pBuff, tSipIntMsg *pMsg)
{
    int len;
    tL4Packet pkt;
    tSipMsgBody *pMsgBody;
    char *pBody;
    tFSM fsm;
    
    pMsgBody = (tSipMsgBody *)SIP_memPoolAlloc(eSIP_OBJECT_SIP_MSG_BODY);
    if (NULL == pMsgBody) {
        return (SIP_NO_MEM);
    }

    pBody = pMsgBody->msg;

    pkt.frame  = (tSipHandle)pBody;
    pkt.pCurr  = pBody;
    pkt.pStart = pBody;
    pkt.length = PAYLOAD_SCRATCH_PAD_SIZE;
    pkt.isOutOfRoom = FALSE;

    OSAL_memSet(&fsm, 0, sizeof(tFSM));

    /* Let's encode the sdp First and into a temporary buffer */
    if (_ENC_SdpInfo(&fsm, &pkt, pMsg) != SIP_OK) {
        SIP_memPoolFree(eSIP_OBJECT_SIP_MSG_BODY, (tDLListEntry *)pMsgBody);
    }
    /* Get the length of the encoded sdp */
    len = pkt.pCurr - pkt.pStart;

    /* Now let's set up the SDP part in the target buffer */
    TOKEN_Put(SIP_CRLF, "", pBuff);
    TOKEN_Put("--++", SIP_CRLF, pBuff);
    TOKEN_Put("Content-Type: ", SIP_CONTENT_TYPE_SDP_STR, pBuff);
    TOKEN_Put(SIP_CRLF, "", pBuff);
    TOKEN_Put("Content-Length: ", "", pBuff);
    _ENC_PutNumberHelper(pFSM, len - 2, pBuff);
    /* copy the sdp */
    OSAL_memCpy(pBuff->pCurr, pkt.pStart, len);
    /* We are done with the temporary buffer so let's free */
    SIP_memPoolFree(eSIP_OBJECT_SIP_MSG_BODY, (tDLListEntry *)pMsgBody);
    /* advance the pCurr */
    pBuff->pCurr += len;
    TOKEN_Put(SIP_CRLF, "", pBuff);

    /* Now put the second part... anything that's in the text */
    if ((NULL != pMsg->pMsgBody)) {
        TOKEN_Put(pMsg->pMsgBody->msg, "", pBuff);
    }
    TOKEN_Put(SIP_CRLF, "--++--", pBuff);
    TOKEN_Put(SIP_CRLF, "", pBuff);
    return (SIP_OK);
}

static vint _ENC_BinaryMessageBody(tFSM *pFSM, tL4Packet *pBuff, tSipIntMsg *pMsg)
{
    UNUSED(pFSM);
    
    if (pMsg->pMsgBody) {
        /* space out the body with a empty line */
        TOKEN_Put(ABNF_EOL, "", pBuff);
        /* now place the text message */
    
        /* copy the payload */
        OSAL_memCpy(pBuff->pCurr, pMsg->pMsgBody->msg, pMsg->ContentLength);
        /* advance the pCurr */
        pBuff->pCurr += pMsg->ContentLength;
        return TOKEN_Put(SIP_CRLF, "", pBuff);
    }
    else {
        return (SIP_OK);
    }
}

static vint _ENC_ContentType(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg)
{
    tSipIntMsg *pMsg = (tSipIntMsg*)hMsg;
    uint32 x;
    vint tableSize = sizeof(_IntContentType_HFTable) / sizeof(_IntContentType_HFTable[0]);
    
    if (TOKEN_IntLookup(pMsg->ContentType, _IntContentType_HFTable, tableSize, &x) == SIP_OK) {
        return TOKEN_Put(_IntContentType_HFTable[x].pExt, SIP_CRLF, pBuff);
    }
    else if (pMsg->ContentType == eCONTENT_TYPE_LAST) {
        /* Then that means we don't understand the content type 
         * so we look inside the list of header fields from the 
         * application developer to see if me wants to specify a custom one
         */
        return _ENC_StringHelper(pBuff, pMsg, eSIP_CONTENT_TYPE_HF);
    }
    return (SIP_FAILED);
}

static vint _ENC_Expires(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg)
{
    tSipIntMsg *pMsg = (tSipIntMsg*)hMsg;
    return _ENC_PutNumberHelper(pFSM, pMsg->Expires, pBuff);
}

static vint _ENC_From(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg)
{
    tSipIntMsg *pMsg = (tSipIntMsg*)hMsg;
    return _ENC_UriPlus(pBuff, &pMsg->From);
}

static vint _ENC_MaxForwards(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg)
{
    return _ENC_StringHelper(pBuff, hMsg, eSIP_MAX_FORWARDS_HF);
}

static vint _ENC_MinExpires(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg)
{
    tSipIntMsg *pMsg = (tSipIntMsg*)hMsg;
    return _ENC_PutNumberHelper(pFSM, pMsg->MinExpires, pBuff);
}

static vint _ENC_MinSE(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg)
{
    tSipIntMsg *pMsg = (tSipIntMsg*)hMsg;
    return _ENC_PutNumberHelper(pFSM, pMsg->MinSE, pBuff);
}

static vint _ENC_ProxyAuthenticate(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg)
{
    tSipIntMsg *pMsg = (tSipIntMsg*)hMsg;
    return _ENC_Authenticate(pFSM, pBuff, &pMsg->AuthorizationList, 
            SIP_PROXY_AUTHENTICATE_HF_STR, pMsg->msgType);
}
    

static vint _ENC_ProxyAuthorization(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg) 
{
    tSipIntMsg *pMsg = (tSipIntMsg*)hMsg;
    return _ENC_Authenticate(pFSM, pBuff, &pMsg->AuthorizationList, 
            SIP_PROXY_AUTHORIZATION_HF_STR, pMsg->msgType);
}

static vint _ENC_Route(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg)
{
    tSipIntMsg *pMsg = (tSipIntMsg*)hMsg;
    UNUSED(pFSM);

    return _ENC_RouteHlpr(pBuff, &pMsg->RouteList);
}

static vint _ENC_ServiceRoute(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg)
{
    tSipIntMsg *pMsg = (tSipIntMsg*)hMsg;
    UNUSED(pFSM);

    return _ENC_RouteHlpr(pBuff, &pMsg->ServiceRouteList);
}

static vint _ENC_RecRoute(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg)
{
    tSipIntMsg *pMsg = (tSipIntMsg*)hMsg;
    UNUSED(pFSM);

    return _ENC_RouteHlpr(pBuff, &pMsg->RecRouteList);
}

static vint _ENC_RouteHlpr(tL4Packet *pBuff, tDLList *pList)
{
    tDLListEntry *pEntry;
    int           value;
    vint          status;

    pEntry = NULL;
    while (0 != (value = DLLIST_GetNext(pList, (tDLListEntry **) &pEntry))) {
        tRouteHFE *pRoute = (tRouteHFE *)pEntry;
       
        TOKEN_Put("<", "", pBuff);
        
        if ((status = _ENC_WriteUri(pBuff, &pRoute->uri)) != SIP_OK)
            return status;
         
        TOKEN_Put(">", "", pBuff);

        if (value == 2)
            TOKEN_Put(",", "", pBuff);
    }
    return TOKEN_Put(SIP_CRLF, "", pBuff);
}

static vint _ENC_ReferTo(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg)
{
    tSipIntMsg *pMsg = (tSipIntMsg*)hMsg;
    UNUSED(pFSM);
    return _ENC_ReferToHlpr(TRUE, pBuff, &pMsg->ReferTo);
}

static vint _ENC_RSeq(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg)
{
    tSipIntMsg *pMsg = (tSipIntMsg*)hMsg;
    return _ENC_PutNumberHelper(pFSM, pMsg->RSeq, pBuff);
}

static vint _ENC_RAck(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg)
{
    tSipIntMsg *pMsg = (tSipIntMsg*)hMsg;
    uint32 x;
    char str[SIP_MAX_BASETEN_NUM_STRING];

    UNUSED(pFSM);

    /* put the seq number */
    if (0 < OSAL_itoa(pMsg->RAck.seqNum, str, SIP_MAX_BASETEN_NUM_STRING))
        TOKEN_Put(str, " ", pBuff);
    
    /* put the cseq number number */
    if (0 < OSAL_itoa(pMsg->RAck.cseq.seqNum, str, SIP_MAX_BASETEN_NUM_STRING))
        TOKEN_Put(str, " ", pBuff);
    
    /* put the method */
    if (TOKEN_IntLookup(pMsg->RAck.cseq.method, _IntMethodTable, eSIP_LAST_METHOD, &x) != SIP_OK )
         return (SIP_FAILED);
    
    return TOKEN_Put(_IntMethodTable[x].pExt, SIP_CRLF, pBuff);
}

static vint _ENC_ReferToHlpr(vint useBrackets, tL4Packet *pBuff, tReferToHF *pReferTo)
{
    uint32 x;

    if (useBrackets) {
        TOKEN_Put("<", "", pBuff);
    }
    if (_ENC_WriteUri(pBuff, &pReferTo->uriPlus.uri) != SIP_OK) {
        if (useBrackets) {
            TOKEN_Put(">", SIP_CRLF, pBuff);
        }
        return (SIP_FAILED);
    }

    /* encode method */
    if ((eSIP_FIRST_METHOD < pReferTo->method) &&
            (eSIP_LAST_METHOD > pReferTo->method)) {
        if (TOKEN_IntLookup(pReferTo->method, _IntMethodTable, eSIP_LAST_METHOD,
                &x) == SIP_OK) {
            TOKEN_Put(";", SIP_METHOD_URI_PARM_STR, pBuff);
            TOKEN_Put("=", _IntMethodTable[x].pExt, pBuff);
        }
    }

    /* now attempt to encode any replaces info */
    if (pReferTo->replaces.szCallId[0] != 0) {
        TOKEN_Put("?", SIP_REPLACES_HF_STR, pBuff);
        TOKEN_Put("=", "", pBuff);
        TOKEN_PutSpecChar(pReferTo->replaces.szCallId, "", pBuff);
        TOKEN_Put("%3B", SIP_TO_TAG_HF_ARG_STR, pBuff);
        TOKEN_Put("%3D", "", pBuff);
        TOKEN_PutSpecChar(pReferTo->replaces.szToTag, "", pBuff);
        TOKEN_Put("%3B", SIP_FROM_TAG_HF_ARG_STR, pBuff);
        TOKEN_Put("%3D", "", pBuff);
        TOKEN_PutSpecChar(pReferTo->replaces.szFromTag, "", pBuff);
    }

    if (useBrackets)
        return TOKEN_Put(">", SIP_CRLF, pBuff);
    else
        return (SIP_OK);
}

static vint _ENC_WriteUri(tL4Packet *pBuff, tUri *pUri)
{
    uint32 len = 0;
    char *bufEnd = EXTBUF_END(pBuff);
    
    /* check if buffer length is not expired */
    len = bufEnd - pBuff->pCurr;
    if (ENC_Uri(pUri, pBuff->pCurr, &len, 1) == SIP_OK) {
        pBuff->pCurr += len;
        return (SIP_OK);
    }
    return (SIP_FAILED);
}

static vint _ENC_Replaces(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg)
{
    tSipIntMsg *pMsg = (tSipIntMsg*)hMsg;
    TOKEN_Put(pMsg->Replaces.szCallId, ";", pBuff);
    TOKEN_Put(SIP_TO_TAG_HF_ARG_STR, "=", pBuff);
    TOKEN_Put(pMsg->Replaces.szToTag, ";", pBuff);
    TOKEN_Put(SIP_FROM_TAG_HF_ARG_STR, "=", pBuff);
    TOKEN_Put(pMsg->Replaces.szFromTag, "", pBuff);
    if (pMsg->Replaces.earlyFlag) {
        TOKEN_Put(";", SIP_EARLY_FLAG_TAG_HF_ARG_STR, pBuff);
    }
    return TOKEN_Put(SIP_CRLF, "", pBuff);
}

static vint _ENC_To(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg)
{
    tSipIntMsg *pMsg = (tSipIntMsg*)hMsg;
    return _ENC_UriPlus(pBuff, &pMsg->To);
}

static vint _ENC_Via(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg)
{
    vint status;
    tDLListEntry *pEntry;
    tSipIntMsg *pMsg = (tSipIntMsg*)hMsg;
    uint32 size;
    vint writeViaStr = FALSE;
    char str[MAX_IPV6_STR_LEN];
    char tmpStr[MAX_IPV6_STR_LEN];
    uint32 x;
    tTransportType transtype;
    tSipUriScheme  scheme;

    UNUSED(pFSM);

    pEntry = NULL;
    while (DLLIST_GetNext(&pMsg->ViaList, &pEntry)) {
        tViaHFE *pVia = (tViaHFE *)pEntry;
   
        if (writeViaStr)
            TOKEN_Put(SIP_VIA_HF_STR, ": ", pBuff);
        else
            writeViaStr = TRUE;

        TOKEN_Put(pVia->szVersion, "/", pBuff);

        size = (sizeof(_IntTransTypeTable) / sizeof(_IntTransTypeTable[0]));
        if (TOKEN_IntLookup(pVia->uri.transport, _IntTransTypeTable,
                size, &x) == SIP_OK) {
            TOKEN_Put(_IntTransTypeTable[x].pExt, " ", pBuff);
        }
        else {
            SIP_DebugLog(SIP_DB_ENCODE_LVL_1, "_ENC_Via: Failed encoding "
                    "transport type hMsg:%X transtype:%d",
                    (int)pMsg, (int)pVia->uri.transport, 0);
            return (SIP_FAILED);
        }

        /*
         * now the uri but force the scheme and transport not to print,
         * so cache the scheme and transport temporarily.
         */
        scheme = pVia->uri.scheme;
        transtype = pVia->uri.transport;
        pVia->uri.scheme = eURI_SCHEME_DUMMY;
        pVia->uri.transport = eTransportNone;
        if ((status = _ENC_WriteUri(pBuff, &pVia->uri)) != SIP_OK) {
            SIP_DebugLog(SIP_DB_ENCODE_LVL_1,
                    "_ENC_Via: Failed encoding URI hMsg:%X",
                    (int)pMsg, 0, 0);
            /* reinstate the original values */
            pVia->uri.scheme = scheme;
            pVia->uri.transport = transtype;
            return status;
        }
        /* reinstate the original values */
        pVia->uri.scheme = scheme;
        pVia->uri.transport = transtype;

        /* add 'keep' parameter in Via */
        if (pVia->keep == OSAL_TRUE) {
            TOKEN_Put(";", SIP_KEEP_HF_ARG_STR, pBuff);
        }
        /* now the rport */
        if (pVia->rport != 0) {
            TOKEN_Put(";", SIP_RPORT_HF_ARG_STR, pBuff);
            if (pVia->rport > 1) {
                OSAL_itoa(pVia->rport, str, MAX_IPV6_STR_LEN);
                TOKEN_Put("=", str, pBuff);
            }
        }
        /* now the received */
        if (eNwAddrIPv6 == pVia->uri.host.addressType) {
            if (!OSAL_netIpv6IsAddrZero(pVia->received.v6)) {
                if (VoIP_IpV6Int2Ext(pVia->received, tmpStr) == SIP_OK) {
                    TOKEN_Put(";", SIP_RECEIVED_HF_ARG_STR, pBuff);
                    OSAL_snprintf(str, sizeof(str), "%s", tmpStr);
                    TOKEN_Put("=", str, pBuff);
                }
            }
        }
        else if (eNwAddrIPv4 == pVia->uri.host.addressType) {
            if (pVia->received.v4.ul != 0) {
                if (VoIP_IpV4Int2Ext(pVia->received, str) == SIP_OK) {
                    TOKEN_Put(";", SIP_RECEIVED_HF_ARG_STR, pBuff);
                    TOKEN_Put("=", str, pBuff);
                }
            }
        }

        if (pVia->szBranch[0] != 0) {
            TOKEN_Put(";", SIP_BRANCH_HF_ARG_STR, pBuff);
            TOKEN_Put("=", pVia->szBranch, pBuff);
        }
        TOKEN_Put(SIP_CRLF, "", pBuff);
    }/* end of the while loop */
    return (SIP_OK);
}


static vint _ENC_WWWAuthenticate(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg) 
{
    tSipIntMsg *pMsg = (tSipIntMsg*)hMsg;
    return _ENC_Authenticate(pFSM, pBuff, &pMsg->AuthorizationList, 
            SIP_WWW_AUTHENTICATE_HF_STR, pMsg->msgType);
}

static vint _ENC_CSeq(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg)
{
    tSipIntMsg *pMsg = (tSipIntMsg*)hMsg;
    uint32 x;
    char str[SIP_MAX_BASETEN_NUM_STRING];

    UNUSED(pFSM);

    /* put the trans number */
    if (OSAL_itoa(pMsg->CSeq.seqNum, str, SIP_MAX_BASETEN_NUM_STRING))
        TOKEN_Put(str, " ", pBuff);
    
    /* put the method */
    if (TOKEN_IntLookup(pMsg->CSeq.method, _IntMethodTable, eSIP_LAST_METHOD, &x) != SIP_OK )
         return (SIP_FAILED);
    
    return TOKEN_Put(_IntMethodTable[x].pExt, SIP_CRLF, pBuff);
}

static vint _ENC_SessionExpires(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg)
{
    tSipIntMsg *pMsg = (tSipIntMsg*)hMsg;
    char str[SIP_MAX_BASETEN_NUM_STRING];

    UNUSED(pFSM);

    /* put the expiration */
    if (OSAL_itoa(pMsg->SessionTimer.expires, str, SIP_MAX_BASETEN_NUM_STRING))
        TOKEN_Put(str, "", pBuff);

    if (eSIP_REFRESHER_UAC == pMsg->SessionTimer.refresher)
        TOKEN_Put(";refresher=uac", "", pBuff);
    else if (eSIP_REFRESHER_UAS == pMsg->SessionTimer.refresher)
        TOKEN_Put(";refresher=uas", "", pBuff);

    return TOKEN_Put(SIP_CRLF, "", pBuff);
}

/*
 * ======== _ENC_ContactCapabilitiesHlpr() ========
 * Private helper routine to encode capabilities from a bitmap to string.
 *
 * Return:
 *  SIP_OK: Encode successfully.
 *  SIP_FAILED: Failed to encode capabilities.
 */
static vint _ENC_ContactCapabilitiesHlpr(
    tL4Packet *pBuff,
    uint32     capsBitmap)
{
    vint   tIdx; /* type index */
    uint32 caps; /* capabilities bitmask */
    char  *capsTypeStr_ptr; /* pointer to caps type string */
    char  *capsStr_ptr; /* pointer to caps string */
    vint   numCapsOfType; /* Number of caps enabled in a specific type */
    uint32 x;
    uint32 capsTableSize;
    uint32 capsTypeTableSize;

    capsTableSize =
            (sizeof(_IntCapabilitiesTable) / sizeof(_IntCapabilitiesTable[0]));
    capsTypeTableSize =
            (sizeof(_IntCapabilitiesTypeTable) /
            sizeof(_IntCapabilitiesTypeTable[0]));

    /* 
     * Encode caps type by type.
     * The reason is IARI and ICSI caps need to be placed together, example
     * as:
     * +g.3gpp.IARI-ref="capabilities1,capabilities2"
     *
     * So loop each type.
     */
    for (tIdx = eSIP_CAPS_TYPE_FIRST; tIdx < eSIP_CAPS_TYPE_LAST; tIdx++) {
        capsTypeStr_ptr = NULL;
        numCapsOfType = 0;
        /* Look up the type string. */
        if (SIP_OK == TOKEN_IntLookup(tIdx, _IntCapsTypeStringTable,
                eSIP_CAPS_TYPE_LAST, &x)) {
            capsTypeStr_ptr = (char*)_IntCapsTypeStringTable[x].pExt;
        }
        /* Loop each capabilities of the bitmap */
        for (caps = eSIP_CAPS_FIRST; caps <= eSIP_CAPS_LAST;) {
            /* Look up the type. */
            if (SIP_OK == TOKEN_IntLookup(caps, _IntCapabilitiesTypeTable,
                    capsTypeTableSize, &x)) {
                if (0 != OSAL_strcmp(_IntCapabilitiesTypeTable[x].pExt,
                        capsTypeStr_ptr)) {
                    /* Not the type */
                    caps = (caps << 1);
                    continue;
                }
            }

            /* Check if the bitmap is set. */
            if (0 != (capsBitmap & caps)) {
                /* Look up the caps string. */
                if (SIP_OK == TOKEN_IntLookup(caps, _IntCapabilitiesTable,
                        capsTableSize, &x)) {
                    capsStr_ptr = (char*)_IntCapabilitiesTable[x].pExt;
                }
                else {
                    /* Should not be here. */
                    caps = (caps << 1);
                    continue;
                }

                /* Add this capabilities, see if need a type string. */
                if ((eSIP_CAPS_TYPE_ICSI == tIdx) ||
                        (eSIP_CAPS_TYPE_IARI == tIdx) ||
                        (eSIP_CAPS_TYPE_RCS_TELEPHONY == tIdx)) {
                    /* It's ICSI, IARI or RCS Telephony. */
                    /*
                     * Ip voice call and ip video call has the same icsi
                     * string, but ip video has ";video" string appends to
                     * icsi string as:
                     * +g.3gpp.icsi-ref="...";video
                     * so don't write the icsi twice if both ip audio call
                     * and ip video call set.
                     */
                    if ((eSIP_CAPS_IP_VIDEO_CALL == caps) &&
                            (0 != (capsBitmap & eSIP_CAPS_IP_VOICE_CALL)) &&
                            (eSIP_CAPS_TYPE_ICSI == tIdx)) {
                        /* Skip ip video call caps set*/
                        caps = (caps << 1);
                        continue;
                    }

                    if (0 == numCapsOfType) {
                        /*
                         * It's the first entry, semi-colon and the type string.
                         */
                        TOKEN_Put(";", capsTypeStr_ptr, pBuff);
                        /* Then start quote and caps string. */
                        TOKEN_Put("\"", capsStr_ptr, pBuff);
                    }
                    else {
                        /* Not the first one, put a comma and caps string */
                        TOKEN_Put(",", capsStr_ptr, pBuff);
                    }
                }
                else {
                    /* None ICSI, IARI or RCS Telephony type, put semi-colon and caps */
                    TOKEN_Put(";", capsStr_ptr, pBuff);
                }
                numCapsOfType++;
            }
            caps = (caps << 1);
        }
        /* End of the type, see if we need add end quote. */
        if ((eSIP_CAPS_TYPE_ICSI == tIdx) ||
                (eSIP_CAPS_TYPE_IARI == tIdx) ||
                (eSIP_CAPS_TYPE_RCS_TELEPHONY == tIdx)) {
            if (0 != numCapsOfType) {
                /* Add end quote. */
                TOKEN_Put("\"", "", pBuff);
                /* Add ";video" to the end of icsi string if ip video call set. */
                if ((0 != (eSIP_CAPS_IP_VIDEO_CALL & capsBitmap)) &&
                        (eSIP_CAPS_TYPE_ICSI == tIdx)) {
                    TOKEN_Put(SIP_CAPS_IP_VIDEO_CALL_SUB_STR, "", pBuff);
                }
            }
        }
    }

    return (SIP_OK);
}
static vint _ENC_Contact(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg)
{
    /* put the display name of there is one */
    vint status;
    tDLListEntry *pEntry;
    tSipIntMsg *pMsg = (tSipIntMsg*)hMsg;
    vint useQuotes = FALSE;
    char str[SIP_MAX_BASETEN_NUM_STRING];
    
    UNUSED(pFSM);

    pEntry = NULL;
    while (DLLIST_GetNext(&pMsg->ContactList, &pEntry)) {
        tContactHFE *pContact = (tContactHFE *)pEntry;
        if (pContact->szDisplayName[0] != 0) {
            if (OSAL_strchr(pContact->szDisplayName, ' ')) {
                useQuotes=TRUE;
                /* place the " */
                TOKEN_Put("\"", "", pBuff);
            }
            TOKEN_Put(pContact->szDisplayName,
                    useQuotes ? "\" " : " ", pBuff);
        }
    
        /* now put the uri */
        TOKEN_Put("<", "", pBuff);
        if ((status = _ENC_WriteUri(pBuff, &pContact->uri)) != SIP_OK)
            return status;

        TOKEN_Put(">", "", pBuff);
        /* now place the contact parameters */
        if (pContact->szUser[0] != 0) {
            TOKEN_Put(";", SIP_CONTACT_HF_USER_ARG_STR, pBuff);
            TOKEN_Put("=", pContact->szUser, pBuff);
        }
        /* now the expires */
        if (pContact->expires != 0) {
            OSAL_itoa(pContact->expires, str, SIP_MAX_BASETEN_NUM_STRING);
            TOKEN_Put(";", SIP_CONTACT_HF_EXPIRES_ARG_STR, pBuff);
            TOKEN_Put("=", str, pBuff);
        }
        /* now the q */
        if (pContact->q[0] != 0) {
            TOKEN_Put(";", SIP_CONTACT_HF_Q_ARG_STR, pBuff);
            TOKEN_Put("=", pContact->q, pBuff);
        }
        /* now the im session indicator */
        if (pContact->isImSession != 0) {
            TOKEN_Put(";", SIP_CONTACT_HF_IM_SESSION_ARG_STR, pBuff);
        }

        /* Now encode capabilities */
        if (0 != pContact->capabilitiesBitmap) {
            _ENC_ContactCapabilitiesHlpr(pBuff, pContact->capabilitiesBitmap);
        }

        /* new add sip instance */
        if (pContact->szInstance[0] != 0) {
            TOKEN_Put(";", SIP_CONTACT_HF_SIP_INSTANCE_ARG_STR, pBuff);
            TOKEN_Put("=\"<", pContact->szInstance, pBuff);
            TOKEN_Put(">\"", "", pBuff);
        }
        TOKEN_Put(SIP_CRLF, "", pBuff);
    } /* end of the while loop */
    return (SIP_OK);
}


static vint _ENC_EventHlpr(tL4Packet *pBuff, tEventHF *pEvt)
{
    /* place the event stuff */
    if (pEvt->szPackage[0] != 0) {
        TOKEN_Put(pEvt->szPackage, "", pBuff);

        /* place any params if they exist */
        if (pEvt->szId[0] != '\0') {
            TOKEN_Put(";id=", pEvt->szId, pBuff);
        }
        if (pEvt->szParam[0] != '\0') {
            TOKEN_Put(";param=", pEvt->szParam, pBuff);
        }
        TOKEN_Put(SIP_CRLF, "", pBuff);
        return (SIP_OK);
    }
    return (SIP_FAILED);
}

static vint _ENC_Event(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg)
{
    tSipIntMsg *pMsg;
    
    pMsg = (tSipIntMsg*)hMsg;

    return _ENC_EventHlpr(pBuff, &pMsg->Event);
}

static vint _ENC_SubState(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg)
{
    tSipIntMsg *pMsg;
    vint status;
    char str[SIP_MAX_BASETEN_NUM_STRING];
    uint32 x;
    
    pMsg = (tSipIntMsg*)hMsg;

    /* place the event arg */
    if (TOKEN_IntLookup(pMsg->SubState.arg, _IntSubStateArgTable, eSIP_SUBS_HF_LAST_ARG, &x) == SIP_OK) {
        if((status = TOKEN_Put(_IntSubStateArgTable[x].pExt, "", pBuff)) != SIP_OK) {
            return status;
        }
        else {
            /* place any params if they exist */
            if (pMsg->SubState.expires != 0) {
                if (0 >= OSAL_itoa(pMsg->SubState.expires, str,
                        SIP_MAX_BASETEN_NUM_STRING))
                    return (SIP_FAILED);
                else
                    TOKEN_Put(";expires=", str, pBuff);
            }
            if (pMsg->SubState.szReason[0] != '\0') {
                TOKEN_Put(";reason=", pMsg->SubState.szReason, pBuff);
            }
            TOKEN_Put(SIP_CRLF, "", pBuff);
        }
        return (SIP_OK);
    }
    else {
        return (SIP_FAILED);
    }
}


static vint _ENC_SdpInfo(tFSM *pFSM, tL4Packet *pBuff, tSipIntMsg *pMsg)
{
    tSdpMsg   *pCurrSdpMsg;
    if ( pMsg->pSessDescr ) {
        pCurrSdpMsg = pMsg->pSessDescr;
        while ( pCurrSdpMsg ) {
            
            TOKEN_Put("", ABNF_EOL, pBuff);
         
            if ( SDPENC_Exec(pFSM, pBuff, pCurrSdpMsg) != SIP_OK ) {
                SIP_DebugLog(SIP_DB_ENCODE_LVL_1, "_ENC_SdpInfo: SDPENC_Exec FAILED", 0, 0, 0);
                RETURN_ERROR(pFSM, 0);
            }

            pCurrSdpMsg = pCurrSdpMsg->next;
        }
    }
    return (SIP_OK);
}


static vint _ENC_DomainArg(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg)
{
    tAuthorizationHFE *pAuth = (tAuthorizationHFE*) hIntMsg;

    UNUSED(pFSM);

    return _ENC_PutAuthString(pAuth->domain, pBuff, TRUE);
}

static vint _ENC_UsernameArg(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg)
{
    tAuthorizationHFE *pAuth = (tAuthorizationHFE*) hIntMsg;

    UNUSED(pFSM);

    return _ENC_PutAuthString(pAuth->szUsername, pBuff, TRUE);
}

static vint _ENC_RealmArg(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg)
{
    tAuthorizationHFE *pAuth = (tAuthorizationHFE*) hIntMsg;

    UNUSED(pFSM);

    return _ENC_PutAuthString(pAuth->szRealm, pBuff, TRUE);
}

static vint _ENC_NonceArg(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg)
{
    tAuthorizationHFE *pAuth = (tAuthorizationHFE*) hIntMsg;

    UNUSED(pFSM);

    return _ENC_PutAuthString(pAuth->szNonce, pBuff, TRUE);
}

static vint _ENC_ResponseArg(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg)
{
    tAuthorizationHFE *pAuth = (tAuthorizationHFE*) hIntMsg;

    UNUSED(pFSM);

    return _ENC_PutAuthString(pAuth->szResponse, pBuff, TRUE);
}

static vint _ENC_AutsArg(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg)
{
    tAuthorizationHFE *pAuth = (tAuthorizationHFE*) hIntMsg;

    UNUSED(pFSM);

    return _ENC_PutAuthString(pAuth->szAuts, pBuff, TRUE);
}

static vint _ENC_B64UserPwArg(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg)
{
    tAuthorizationHFE *pAuth = (tAuthorizationHFE*) hIntMsg;

    UNUSED(pFSM);

    return _ENC_PutAuthString(pAuth->szB64UserPw, pBuff, FALSE);
}

static vint _ENC_QopArg(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg)
{
    vint status;    
    tAuthorizationHFE *pAuth = (tAuthorizationHFE*) hIntMsg;

    UNUSED(pFSM);

    if (pAuth->qop == eSIP_QOP_AUTH) {
        status = _ENC_PutAuthString(SIP_QOP_AUTH_STR, pBuff, FALSE);
    }
    else if (pAuth->qop == eSIP_QOP_AUTH_INT) {
        status = _ENC_PutAuthString(SIP_QOP_AUTH_INT_STR, pBuff, FALSE);
    }
    else {
        status = TOKEN_Put("", "", pBuff);
    }
    return status;
}

static vint _ENC_NcArg(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg)
{
    tAuthorizationHFE *pAuth = (tAuthorizationHFE*) hIntMsg;

    UNUSED(pFSM);

    return _ENC_PutAuthString(pAuth->szNC, pBuff, FALSE);
}

static vint _ENC_AlgArg(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg)
{
    tAuthorizationHFE *pAuth = (tAuthorizationHFE*) hIntMsg;
    vint status;

    UNUSED(pFSM);

    if (pAuth->alg == eAUTH_ALG_MD5) {
        status = _ENC_PutAuthString(SIP_AUTH_ALG_MD5_STR, pBuff, FALSE);
    }
    else if (pAuth->alg == eAUTH_ALG_AKAV1_MD5) {
        status = _ENC_PutAuthString(SIP_AUTH_ALG_AKAV1_MD5_STR, pBuff, FALSE);
    }
    else if (pAuth->alg == eAUTH_ALG_AKAV2_MD5) {
        status = _ENC_PutAuthString(SIP_AUTH_ALG_AKAV2_MD5_STR, pBuff, FALSE);
    }
    else {
        status = TOKEN_Put("\"", "\"", pBuff);
    }
    return status;
}

static vint _ENC_CnonceArg(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg)
{
    tAuthorizationHFE *pAuth = (tAuthorizationHFE*) hIntMsg;

    UNUSED(pFSM);

    return _ENC_PutAuthString(pAuth->szCNonce, pBuff, TRUE);
}

static vint _ENC_OpaqueArg(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg)
{
    tAuthorizationHFE *pAuth = (tAuthorizationHFE*) hIntMsg;

    UNUSED(pFSM);

    return _ENC_PutAuthString(pAuth->szOpaque, pBuff, TRUE);
}

static vint _ENC_StaleArg(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg)
{
    tAuthorizationHFE *pAuth = (tAuthorizationHFE*) hIntMsg;

    UNUSED(pFSM);

    return _ENC_PutAuthString(pAuth->stale?SIP_TRUE_STR:SIP_FALSE_STR, 
            pBuff, FALSE);
}

static vint _ENC_UriArg(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hIntMsg)
{
    tAuthorizationHFE *pAuth = (tAuthorizationHFE*) hIntMsg;
    vint status;

    UNUSED(pFSM);
    
    TOKEN_Put("\"", "", pBuff);
    if ((status = _ENC_WriteUri(pBuff, &pAuth->uri)) != SIP_OK) 
        return status;
    
    return TOKEN_Put("\"", "", pBuff);
}


static vint _ENC_UriPlus(tL4Packet *pBuff, tUriPlus *pUri)
{
    vint status;
    /* put the display name of there is one */
    vint useQuotes = FALSE;
    
    if (pUri->szDisplayName[0] != 0) {
        if (OSAL_strchr(pUri->szDisplayName, ' ') ) {
            useQuotes=TRUE;
            /* place the " */
            TOKEN_Put("\"", "", pBuff);
        }
        TOKEN_Put(pUri->szDisplayName, useQuotes ? "\" " : " ", pBuff);
    }

    /* place angle brackets */
    TOKEN_Put("<", "", pBuff);

    /* now put the uri */
    if ((status = _ENC_WriteUri(pBuff, &pUri->uri)) != SIP_OK) 
        return status;

    TOKEN_Put(">", "", pBuff);

    /* now the 'tag' if it exists */
    if (pUri->szTag[0] != 0) {
        TOKEN_Put(";tag=", pUri->szTag, pBuff);
    }

    /* now the 'user' if it exists */
    if (pUri->szUser[0] != 0) {
        TOKEN_Put(";user=", pUri->szUser, pBuff);
    }
    return TOKEN_Put(SIP_CRLF, "", pBuff);
}

static vint _ENC_AlertInfo(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg)
{
    /* only use for Communication Waiting tone (TS 24.615)*/
    TOKEN_Put(SIP_ALERT_INFO_HF_CALL_WAITING_STR, SIP_CRLF, pBuff);
    return (SIP_OK);
}

/* these are generic handlers for the header fields that are strings only */
static vint _ENC_Accept(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg)
{
    return _ENC_StringHelper(pBuff, hMsg, eSIP_ACCEPT_HF);
}
static vint _ENC_AcceptEncoding(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg)
{
    return _ENC_StringHelper(pBuff, hMsg, eSIP_ACCEPT_ENCODING_HF);
}
static vint _ENC_AcceptLanguage(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg)
{
    return _ENC_StringHelper(pBuff, hMsg,eSIP_ACCEPT_LANGUAGE_HF );
}
static vint _ENC_Allow(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg)
{
    return _ENC_StringHelper(pBuff, hMsg, eSIP_ALLOW_HF);
}
static vint _ENC_AllowEvents(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg)
{
    return _ENC_StringHelper(pBuff, hMsg, eSIP_ALLOW_EVENTS_HF);
}
static vint _ENC_ContentDisp(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg)
{
    return _ENC_StringHelper(pBuff, hMsg, eSIP_CONTENT_DISP_HF);
}
static vint _ENC_ContentEncoding(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg)
{
    return _ENC_StringHelper(pBuff, hMsg, eSIP_CONTENT_ENCODING_HF);
}
static vint _ENC_Organization(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg)
{
    return _ENC_StringHelper(pBuff, hMsg, eSIP_ORGANIZATION_HF);
}
static vint _ENC_Require(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg)
{
    return _ENC_StringHelper(pBuff, hMsg, eSIP_REQUIRE_HF); 
}
static vint _ENC_Server(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg)
{
    return _ENC_StringHelper(pBuff, hMsg, eSIP_SERVER_HF);
}
static vint _ENC_Supported(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg)
{
    return _ENC_StringHelper(pBuff, hMsg, eSIP_SUPPORTED_HF);
}

/*
 * ======== _ENC_PAccessNwInfo() ========
 * Private helper routine to encode P-Access-Network-Info header field.
 *
 * Return:
 *  SIP_OK: Encode successfully.
 *  SIP_FAILED: Failed to encode.
 */
static vint _ENC_PAccessNwInfo(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg)
{
    tSipIntMsg *pMsg;
    tNwAccess  *pNwAccess;
    
    pMsg = (tSipIntMsg*)hMsg;
    pNwAccess = &pMsg->nwAccess;

    switch (pNwAccess->type) {
        case eNwAccessTypeGeran:
            TOKEN_Put(SIP_PANI_GERAN_STR, pNwAccess->id, pBuff);
            break;
        case eNwAccessTypeUtranFdd:
            TOKEN_Put(SIP_PANI_UTRAN_FDD_STR, pNwAccess->id, pBuff);
            break;
        case eNwAccessTypeUtranTdd:
            TOKEN_Put(SIP_PANI_UTRAN_TDD_STR, pNwAccess->id, pBuff);
            break;
        case eNwAccessTypeEUtranFdd:
            TOKEN_Put(SIP_PANI_E_UTRAN_FDD_STR, pNwAccess->id, pBuff);
            break;
        case eNwAccessTypeEUtranTdd:
            TOKEN_Put(SIP_PANI_E_UTRAN_TDD_STR, pNwAccess->id, pBuff);
            break;
        case eNwAccessType80211:
            TOKEN_Put(SIP_PANI_IEEE_802_11_STR, pNwAccess->id, pBuff);
            break;
        /* Add more if needed. */
        default:
            return (SIP_FAILED);
    }

    TOKEN_Put("", SIP_CRLF, pBuff);

    return (SIP_OK);
}

static vint _ENC_UserAgent(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg)
{
    return _ENC_StringHelper(pBuff, hMsg, eSIP_USER_AGENT_HF); 
}
static vint _ENC_ReferredBy(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg)
{
    return _ENC_StringHelper(pBuff, hMsg, eSIP_REFERRED_BY_HF);
}

