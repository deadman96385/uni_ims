/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 29509 $ $Date: 2014-10-28 16:00:56 +0800 (Tue, 28 Oct 2014) $
 */

#include "sip_sip.h"
#include "sip_token.h"
#include "sip_clib.h"
#include "sip_list.h"
#include "sip_abnfcore.h"
#include "sip_mem.h"
#include "sip_msgcodes.h"
#include "sip_parser_dec.h"
#include "sip_sdp_dec.h"
#include "sip_mem_pool.h"

/* local protoypes */
static vint _DEC_Request            (tFSM *pFSM, tL4Packet *pBuff, tSipIntMsg *pMsg);
static vint _DEC_Response           (tFSM *pFSM, tL4Packet *pBuff, tSipIntMsg *pMsg);
static vint _DEC_HeaderFields       (tFSM *pFSM, tL4Packet *pBuff, tSipIntMsg *pMsg, vint*);
static vint _DEC_SdpInfo            (tFSM *pFSM, tL4Packet *pBuff, tSipHandle  hMsg);
static vint _DEC_NotifyBody         (tFSM *pFSM, tL4Packet *pBuff, tSipIntMsg *pMsg);
static vint _DEC_BinaryMessageBody  (tFSM *pFSM, tL4Packet *pBuff, tSipIntMsg *pMsg);
static vint _DEC_MultipartBody      (tFSM *pFSM, tL4Packet *pBuff, tSipIntMsg *pMsg);
static vint _DEC_HandleProtocolError(tFSM *pFSM, tSipIntMsg *pMsg);

/* Header Field Handlers */
static vint _DEC_Accept            (tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg);
static vint _DEC_AcceptEncoding    (tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg);
static vint _DEC_AcceptLanguage    (tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg);
static vint _DEC_Allow             (tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg);
static vint _DEC_AllowEvents       (tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg);
static vint _DEC_Authorization     (tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg);
static vint _DEC_CallId            (tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg);
static vint _DEC_CSeq              (tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg);
static vint _DEC_Contact           (tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg);
static vint _DEC_ContentDisp       (tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg);
static vint _DEC_ContentEncoding   (tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg);
static vint _DEC_ContentLength     (tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg);
static vint _DEC_ContentType       (tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg);
static vint _DEC_Event             (tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg);
static vint _DEC_ETag              (tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg);
static vint _DEC_Expires           (tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg);
static vint _DEC_From              (tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg);
static vint _DEC_IfMatch           (tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg);
static vint _DEC_MaxForwards       (tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg);
static vint _DEC_MinExpires        (tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg);
static vint _DEC_MinSE             (tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg);
static vint _DEC_Organization      (tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg);
static vint _DEC_ProxyAuthenticate (tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg);
static vint _DEC_ProxyAuthorization(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg);
static vint _DEC_RAck              (tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg);
static vint _DEC_RecordRoute       (tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg);
static vint _DEC_ReferTo           (tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg);
static vint _DEC_ReferredBy        (tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg);
static vint _DEC_Replaces          (tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg);
static vint _DEC_Route             (tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg);
static vint _DEC_Require           (tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg);
static vint _DEC_RSeq              (tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg);
static vint _DEC_Server            (tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg);
static vint _DEC_ServiceRoute      (tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg);
static vint _DEC_SessionExpires    (tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg);
static vint _DEC_Supported         (tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg);
static vint _DEC_To                (tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg);
static vint _DEC_UserAgent         (tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg);
static vint _DEC_Via               (tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg);
static vint _DEC_HFSkip            (tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg);
static vint _DEC_WWW_Authenticate  (tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg);
static vint _DEC_P_AccessNwInfo    (tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg);
static vint _DEC_RetryAfter        (tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg);
static vint _DEC_Version           (tFSM *pFSM, tL4Packet *pBuff, tSipIntMsg *pMsg);

/* decoder helpers */
static vint  _DEC_TransportHlpr  (tFSM *pFSM, tL4Packet  *pBuff, tSipHandle  hType);
static vint  _DEC_UserHlpr       (tFSM *pFSM, tL4Packet  *pBuff, tSipHandle hStr);
static vint  _DEC_ArgHlpr        (tFSM *pFSM, tL4Packet  *pBuff, tSipHandle hStr);
static vint  _DEC_RouteHlpr      (tFSM *pFSM, tL4Packet *pBuff, tDLList *pList);
static vint  _DEC_IPAddrHlpr     (tFSM *pFSM, tL4Packet *pBuff, tSipHandle hAddr);
static vint  _DEC_MethodHlpr     (tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMethod);
static vint  _DEC_UintHlpr       (tFSM *pFSM, tL4Packet *pBuff, tSipHandle hNum);
static vint  _DEC_DummyHlpr      (tFSM *pFSM, tL4Packet *pBuff, tSipHandle hValue);
static vint  _DEC_DecodeUriPlus  (tFSM *pFSM, tL4Packet *pBuff, tUriPlus *pUriPlus);
static int   _DEC_NameAndUriHlpr (tFSM *pFSM, tL4Packet *pBuff, tDecodeArgs args[]);
static vint  _DEC_Authenticate   (tFSM *pFSM, tL4Packet *pBuff, tSipIntMsg *pMsg, tHdrFld hdr);
static vint  _DEC_StringHlpr     (tFSM *pFSM, tL4Packet *pBuff, char *pTargetStr, uint32 maxSize);
static vint  _DEC_ReferToStringHlpr (tFSM *pFSM, tL4Packet *pBuff, char *pTargetStr, uint32 maxSize);
static vint  _DEC_UriDecode      (vint withScheme, char *str, uint16 size, tUri *pUri);
static vint  _DEC_ViaStr         (char *pViaStr, int len, tViaHFE *pVia);
static vint  _DEC_GetUriParam    (tFSM *pFSM, tL4Packet *pBuff);
static vint  _DEC_GetContactParam(tFSM *pFSM, tL4Packet *pBuff);
static vint  _DEC_GetReferToParam(tFSM *pFSM, tL4Packet *pBuff);
static vint  _DEC_Rport          (tFSM *pFSM, tL4Packet *pBuff, tSipHandle hNum);
static vint  _DEC_KeepAlive      (tFSM *pFSM, tL4Packet *pBuff, tSipHandle hNum);
static vint  _DEC_ReplacesHlpr   (tFSM *pFSM, tL4Packet *pBuff, tReplacesHF *pReplaces);
static vint  _DEC_ReferToHlpr    (tFSM *pFSM, tL4Packet *pBuff, tReplacesHF *pReplaces);
static char  _DEC_Hex2Abnf       (tFSM *pFSM, tL4Packet *pBuff);
static vint  _DEC_EventHlpr      (tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg);
static vint  _DEC_ExpiresHlpr    (tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg);
static vint  _DEC_SubStateHlpr   (tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg);
static vint  _DEC_ContentTypeHlpr(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg);
static vint  _DEC_UriUsername    (char *pName, vint namelen, char *pTarget, vint maxTargetLen);
static vint  _DEC_Unknown(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg);
static tContactHFE*  _DEC_ContactHlpr(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg);
static vint  _DEC_MultipartBoundaryHlpr(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg);
static vint  _DEC_GruuHlpr       (tFSM *pFSM, tL4Packet *pBuff, tSipHandle hpGruu);
static vint  _DEC_LskpmcHlpr     (tFSM *pFSM, tL4Packet *pBuff, tSipHandle hlskpmc);

/* helpers for authentication parsing */
static vint _DEC_StringArgHlpr(tFSM *pFSM, tL4Packet *pBuff, char *pTarget, uint32 *targetSize);
static vint _DEC_DomainArg(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg);
static vint _DEC_UsernameArg(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg);
static vint _DEC_RealmArg(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg);
static vint _DEC_NonceArg(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg);
static vint _DEC_ResponseArg(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg);
static vint _DEC_AutsArg(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg);
static vint _DEC_CnonceArg(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg);
static vint _DEC_QopArg(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg);
static vint _DEC_NcArg(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg);
static vint _DEC_AlgArg(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg);
static vint _DEC_OpaqueArg(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg);
static vint _DEC_StaleArg(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg);
static vint _DEC_UriArg(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg);
static vint _DEC_SubState(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg);

/* Capabilities decode helper */
static vint  _DEC_CapabilitiesHlpr (tFSM *pFSM, tL4Packet *pBuff, uint32 caps,
        uint32 *capsBitmap_ptr);

static const tTokenizer _ExtMethodTable [] = 
{
    {  SIP_FIRST_METHOD_STR,    eSIP_FIRST_METHOD, _DEC_HFSkip },
    {  SIP_INVITE_METHOD_STR,   eSIP_INVITE,   _DEC_HFSkip },
    {  SIP_CANCEL_METHOD_STR,   eSIP_CANCEL,   _DEC_HFSkip },
    {  SIP_BYE_METHOD_STR,      eSIP_BYE,      _DEC_HFSkip },
    {  SIP_OPTIONS_METHOD_STR,  eSIP_OPTIONS,  _DEC_HFSkip },
    {  SIP_REGISTER_METHOD_STR, eSIP_REGISTER, _DEC_HFSkip },
    {  SIP_ACK_METHOD_STR,      eSIP_ACK,      _DEC_HFSkip },
    {  SIP_NOTIFY_METHOD_STR,   eSIP_NOTIFY,   _DEC_HFSkip },
    {  SIP_REFER_METHOD_STR,    eSIP_REFER,    _DEC_HFSkip },
    {  SIP_MESSAGE_METHOD_STR,  eSIP_MESSAGE,  _DEC_HFSkip },
    {  SIP_SUBSCRIBE_METHOD_STR,eSIP_SUBSCRIBE,_DEC_HFSkip },
    {  SIP_INFO_METHOD_STR,     eSIP_INFO,     _DEC_HFSkip },
    {  SIP_PRACK_METHOD_STR,    eSIP_PRACK,    _DEC_HFSkip },
    {  SIP_UPDATE_METHOD_STR,   eSIP_UPDATE,   _DEC_HFSkip },
    {  SIP_PUBLISH_METHOD_STR,  eSIP_PUBLISH,  _DEC_HFSkip },
    {  SIP_ERROR_METHOD_STR,    eSIP_ERROR,    _DEC_HFSkip }
};


static const tTokenizer _ExtHFtable[] =
{
    { SIP_ACCEPT_HF_STR,                eSIP_ACCEPT_HF,              _DEC_Accept            }, 
    { SIP_ACCEPT_ENCODING_HF_STR,       eSIP_ACCEPT_ENCODING_HF,     _DEC_AcceptEncoding    },
    { SIP_ACCEPT_LANGUAGE_HF_STR,       eSIP_ACCEPT_LANGUAGE_HF,     _DEC_AcceptLanguage    },
    { SIP_ALLOW_HF_STR,                 eSIP_ALLOW_HF,               _DEC_Allow             },
    { SIP_ALLOW_EVENTS_HF_STR,          eSIP_ALLOW_EVENTS_HF,        _DEC_AllowEvents       },
    { SIP_AUTHORIZATION_HF_STR,         eSIP_AUTHORIZATION_HF,       _DEC_Authorization     },
    { SIP_CALL_ID_HF_STR,               eSIP_CALL_ID_HF,             _DEC_CallId            },
    { SIP_CONTACT_HF_STR,               eSIP_CONTACT_HF,             _DEC_Contact           }, 
    { SIP_CONTENT_DISP_HF_STR,          eSIP_CONTENT_DISP_HF,        _DEC_ContentDisp       },
    { SIP_CONTENT_ENCODING_HF_STR,      eSIP_CONTENT_ENCODING_HF,    _DEC_ContentEncoding   }, 
    { SIP_CONTENT_LENGTH_HF_STR,        eSIP_CONTENT_LENGTH_HF,      _DEC_ContentLength     }, 
    { SIP_CONTENT_TYPE_HF_STR,          eSIP_CONTENT_TYPE_HF,        _DEC_ContentType       }, 
    { SIP_CSEQ_HF_STR,                  eSIP_CSEQ_HF,                _DEC_CSeq              },
    { SIP_ETAG_HF_STR,                  eSIP_ETAG_HF,                _DEC_ETag              },
    { SIP_EVENT_HF_STR,                 eSIP_EVENT_HF,               _DEC_Event             },
    { SIP_EXPIRES_HF_STR,               eSIP_EXPIRES_HF,             _DEC_Expires           },
    { SIP_FROM_HF_STR,                  eSIP_FROM_HF,                _DEC_From              }, 
    { SIP_IF_MATCH_HF_STR,              eSIP_IF_MATCH_HF,            _DEC_IfMatch           }, 
    { SIP_MAX_FORWARDS_HF_STR,          eSIP_MAX_FORWARDS_HF,        _DEC_MaxForwards       },
    { SIP_MIN_EXPIRES_HF_STR,           eSIP_MIN_EXPIRES_HF,         _DEC_MinExpires        },
    { SIP_MIN_SE_HF_STR,                eSIP_MIN_SE_HF,              _DEC_MinSE             },
    { SIP_ORGANIZATION_HF_STR,          eSIP_ORGANIZATION_HF,        _DEC_Organization      },
    { SIP_PROXY_AUTHENTICATE_HF_STR,    eSIP_PROXY_AUTHENTICATE_HF,  _DEC_ProxyAuthenticate },
    { SIP_PROXY_AUTHORIZATION_HF_STR,   eSIP_PROXY_AUTHORIZATION_HF, _DEC_ProxyAuthorization},
    { SIP_RACK_HF_STR,                  eSIP_RACK_HF,                _DEC_RAck              },
    { SIP_RECORD_ROUTE_HF_STR,          eSIP_RECORD_ROUTE_HF,        _DEC_RecordRoute       },
    { SIP_REFER_TO_HF_STR,              eSIP_REFER_TO_HF,            _DEC_ReferTo           },
    { SIP_REFERRED_BY_HF_STR,           eSIP_REFERRED_BY_HF,         _DEC_ReferredBy        },
    { SIP_REPLACES_HF_STR,              eSIP_REPLACES_HF,            _DEC_Replaces          },
    { SIP_REQUIRE_HF_STR,               eSIP_REQUIRE_HF,             _DEC_Require           },
    { SIP_ROUTE_HF_STR,                 eSIP_ROUTE_HF,               _DEC_Route             },
    { SIP_RSEQ_HF_STR,                  eSIP_RSEQ_HF,                _DEC_RSeq              },
    { SIP_SERVER_HF_STR,                eSIP_SERVER_HF,              _DEC_Server            },
    { SIP_SERVICE_ROUTE_HF_STR,         eSIP_SERVICE_ROUTE_HF,       _DEC_ServiceRoute      },
    { SIP_SESSION_EXPIRES_HF_STR,       eSIP_SESSION_EXPIRES_HF,     _DEC_SessionExpires    },
    { SIP_SUB_STATE_HF_STR,             eSIP_SUB_STATE_HF,           _DEC_SubState          },
    { SIP_SUPPORTED_HF_STR,             eSIP_SUPPORTED_HF,           _DEC_Supported         },
    { SIP_TO_HF_STR,                    eSIP_TO_HF,                  _DEC_To                },
    { SIP_USER_AGENT_HF_STR,            eSIP_USER_AGENT_HF,          _DEC_UserAgent         },
    { SIP_VIA_HF_STR,                   eSIP_VIA_HF,                 _DEC_Via               },
    { SIP_WWW_AUTHENTICATE_HF_STR,      eSIP_WWW_AUTHENTICATE_HF,    _DEC_WWW_Authenticate  },
    { SIP_P_ACCESS_NW_INFO_HF_STR,      eSIP_P_ACCESS_NW_INFO_HF,    _DEC_P_AccessNwInfo    },
    { SIP_RETRYAFTER_HF_STR,            eSIP_RETRY_AFTER_HF,         _DEC_RetryAfter        },
};

static const tTokenizer _ExtCHFtable[] =
{
    { SIP_CALL_ID_CHF_STR,               eSIP_CALL_ID_HF,             _DEC_CallId           }, /* 'i' */
    { SIP_CONTACT_CHF_STR,               eSIP_CONTACT_HF,             _DEC_Contact          }, /* 'm' */
    { SIP_CONTENT_ENCODING_CHF_STR,      eSIP_CONTENT_ENCODING_HF,    _DEC_ContentEncoding  }, /* 'e' */
    { SIP_CONTENT_LENGTH_CHF_STR,        eSIP_CONTENT_LENGTH_HF,      _DEC_ContentLength    }, /* 'l' */
    { SIP_CONTENT_TYPE_CHF_STR,          eSIP_CONTENT_TYPE_HF,        _DEC_ContentType      }, /* 'c' */
    { SIP_FROM_CHF_STR,                  eSIP_FROM_HF,                _DEC_From             }, /* 'f' */
    { SIP_REFER_TO_CHF_STR,              eSIP_REFER_TO_HF,            _DEC_ReferTo          }, /* 'r' */
    { SIP_SERVER_CHF_STR,                eSIP_SERVER_HF,              _DEC_Server           }, /* 's' */
    { SIP_SUPPORTED_CHF_STR,             eSIP_SUPPORTED_HF,           _DEC_Supported        }, /* 'k' */
    { SIP_TO_CHF_STR,                    eSIP_TO_HF,                  _DEC_To               }, /* 't' */
    { SIP_VIA_CHF_STR,                   eSIP_VIA_HF,                 _DEC_Via              }, /* 'v' */
};

static const tTokenizer _ExtContactHFArgTable[] = 
{
    {   SIP_CONTACT_HF_Q_ARG_STR,               eSIP_CONTACT_HF_Q_ARG,                  0   },
    {   SIP_CONTACT_HF_EXPIRES_ARG_STR,         eSIP_CONTACT_HF_EXPIRES_ARG,            _DEC_UintHlpr   },
    {   SIP_CONTACT_HF_USER_ARG_STR,            eSIP_CONTACT_HF_USER_ARG,               0               },
    {   SIP_CONTACT_HF_IM_SESSION_ARG_STR,      eSIP_CONTACT_HF_IM_SESSION_ARG,         0               },
    {   SIP_CONTACT_HF_IM_CONF_ISFOCUS_ARG_STR, eSIP_CONTACT_HF_IM_CONF_ISFOCUS_ARG,    0               },
    {   SIP_CONTACT_HF_PUB_GRUU_ARG_STR,        eSIP_CONTACT_HF_PUB_GRUU_ARG,           _DEC_GruuHlpr   },
};

static const tTokenizer _ExtURI_ParmTable[] = 
{
    {   SIP_LR_URI_PARM_STR         ,  eSIP_LR_URI_PARM,         _DEC_DummyHlpr     },
    {   SIP_MADDR_URI_PARM_STR      ,  eSIP_MADDR_URI_PARM,      _DEC_IPAddrHlpr    },
    {   SIP_TTL_URI_PARM_STR        ,  eSIP_TTL_URI_PARM,        _DEC_UintHlpr      },
    {   SIP_METHOD_URI_PARM_STR     ,  eSIP_METHOD_URI_PARM,     _DEC_MethodHlpr    },
    {   SIP_TRANSPORT_URI_PARM_STR  ,  eSIP_TRANSPORT_URI_PARM,  _DEC_TransportHlpr },
    {   SIP_USER_HF_ARG_STR         ,  eSIP_USER_URI_PARM,       _DEC_UserHlpr      },
    {   SIP_PHONE_CXT_HF_ARG_STR    ,  eSIP_PHONE_CXT_URI_PARM,  _DEC_UserHlpr      },
    {   SIP_SESSION_HF_ARG_STR      ,  eSIP_SESSION_URI_PARM,    _DEC_UserHlpr      },
    {   SIP_PSBR_URI_PARM_STR       ,  eSIP_PSBR_URI_PARM,       _DEC_ArgHlpr       },
    {   SIP_LBFH_URI_PARM_STR       ,  eSIP_LBFH_URI_PARM,       _DEC_ArgHlpr       },
    {   SIP_CONF_URI_PARM_STR       ,  eSIP_CONF_URI_PARM,       _DEC_ArgHlpr       },
    {   SIP_FTAG_URI_PARM_STR       ,  eSIP_FTAG_URI_PARM,       _DEC_ArgHlpr       },
    {   SIP_GR_URI_PARM_STR         ,  eSIP_GR_URI_PARM,         _DEC_ArgHlpr       },
    {   SIP_LSKPMC_URI_PARM_STR     ,  eSIP_LSKPMC_URI_PARM,     _DEC_LskpmcHlpr    },
};

static const tTokenizer _ExtVia_HFArgTable[] = 
{
    {   SIP_BRANCH_HF_ARG_STR   ,  eSIP_BRANCH_HF_ARG,    0                },
    {   SIP_RECEIVED_HF_ARG_STR ,  eSIP_RECEIVED_HF_ARG,  _DEC_IPAddrHlpr  },
    {   SIP_RPORT_HF_ARG_STR    ,  eSIP_RPORT_HF_ARG,     0                },
    {   SIP_KEEP_HF_ARG_STR     ,  eSIP_KEEP_HF_ARG,      0                }
};

static const tTokenizer _ExtSubState_HFArgTable[] = 
{
    {   SIP_SUBS_HF_ACTIVE_ARG_STR, eSIP_SUBS_HF_ACTIVE_ARG, _DEC_DummyHlpr   },
    {   SIP_SUBS_HF_PEND_ARG_STR,   eSIP_SUBS_HF_PEND_ARG,   _DEC_DummyHlpr   },
    {   SIP_SUBS_HF_TERM_ARG_STR ,  eSIP_SUBS_HF_TERM_ARG,   _DEC_DummyHlpr   },
};

static const tTokenizer _ExtSubState_HFParmTable[] = 
{
    {   SIP_SUBS_HF_EXPIRES_PARM_STR, eSIP_SUBS_HF_EXPIRES_PARM, _DEC_UintHlpr  },
    {   SIP_SUBS_HF_REASON_PARM_STR , eSIP_SUBS_HF_REASON_PARM,  0              },
};

static const tTokenizer _ExtReplaces_HFArgTable[] = 
{
    /* below you see 'replaces' string mapped to call_id.  This is correct
     * because we use this table for 'Replaces' and 'ReferTo' 
     */
    {   SIP_REPLACES_HF_STR,           eSIP_CALL_ID_HF_ARG,    0  },
    {   SIP_TO_TAG_HF_ARG_STR,         eSIP_TO_TAG_HF_ARG,     0  },
    {   SIP_FROM_TAG_HF_ARG_STR ,      eSIP_FROM_TAG_HF_ARG,   0  },
    {   SIP_EARLY_FLAG_TAG_HF_ARG_STR, eSIP_EARLY_FLAG_HF_ARG, 0  },
};

static const tTokenizer _ExtEvent_HFParmTable[] = 
{
    {   SIP_EVENT_HF_ID_PARM_STR,    eSIP_EVENT_HF_ID_PARM,    0 },
    {   SIP_EVENT_HF_PARAM_PARM_STR, eSIP_EVENT_HF_PARAM_PARM, 0 }
};

static const tTokenizer _ExtContentType_HFTable[] = 
{
    {   SIP_CONTENT_TYPE_SDP_STR,           eCONTENT_TYPE_SDP,       0 },
    {   SIP_CONTENT_TYPE_SIPFRAG_STR,       eCONTENT_TYPE_SIPFRAG,   0 },
    {   SIP_CONTENT_TYPE_MULTIPART_DEC_STR, eCONTENT_TYPE_MULTIPART, _DEC_MultipartBoundaryHlpr },
    {   SIP_CONTENT_TYPE_3GPPSMS_STR,       eCONTENT_TYPE_3GPPSMS,   0},
};

/* table for request line args */
static const tTokenizer _ExtUriParmTable[] = 
{
    {   SIP_DUMMY_STR,         eURI_SCHEME_DUMMY,  0  },
    {   SIP_URI_TYPE_SIP_STR,  eURI_SCHEME_SIP,    0  },
    {   SIP_URI_TYPE_SIPS_STR, eURI_SCHEME_SIPS,   0  },
    {   SIP_URI_TYPE_TEL_STR,  eURI_SCHEME_TEL,    0  },
    {   SIP_URI_TYPE_IM_STR,   eURI_SCHEME_IM,     0  },
    {   SIP_URI_TYPE_URN_STR,  eURI_SCHEME_URN,    0  }
};

static const tTokenizer _ExtTransportTypeTable[] = 
{
    {   SIP_TRANSPORT_UDP_STR,  eTransportUdp,  0   },
    {   SIP_TRANSPORT_TCP_STR,  eTransportTcp,  0   },
    {   SIP_TRANSPORT_TLS_STR,  eTransportTls,  0   },
};

static const tTokenizer _ExtAuthTable[] = 
{
    {   SIP_DOMAIN_HF_ARG_STR,   eSIP_DOMAIN_HF_ARG,   _DEC_DomainArg   },
    {   SIP_USERNAME_HF_ARG_STR, eSIP_USERNAME_HF_ARG, _DEC_UsernameArg }, 
    {   SIP_REALM_HF_ARG_STR,    eSIP_REALM_HF_ARG,    _DEC_RealmArg    },
    {   SIP_NONCE_HF_ARG_STR,    eSIP_NONCE_HF_ARG,    _DEC_NonceArg    },
    {   SIP_QOP_HF_ARG_STR,      eSIP_QOP_HF_ARG,      _DEC_QopArg      },
    {   SIP_NC_HF_ARG_STR,       eSIP_NC_HF_ARG,       _DEC_NcArg       },
    {   SIP_ALGORITHM_HF_ARG_STR,eSIP_ALGORITHM_HF_ARG,_DEC_AlgArg      },
    {   SIP_CNONCE_HF_ARG_STR,   eSIP_CNONCE_HF_ARG,   _DEC_CnonceArg   },
    {   SIP_OPAQUE_HF_ARG_STR,   eSIP_OPAQUE_HF_ARG,   _DEC_OpaqueArg   },
    {   SIP_STALE_HF_ARG_STR,    eSIP_STALE_HF_ARG,    _DEC_StaleArg    },
    {   SIP_RESPONSE_HF_ARG_STR, eSIP_RESPONSE_HF_ARG, _DEC_ResponseArg },
    {   SIP_AUTS_HF_ARG_STR,     eSIP_AUTS_HF_ARG,     _DEC_AutsArg     },
    {   SIP_URI_HF_ARG_STR,      eSIP_URI_HF_ARG,      _DEC_UriArg      },
    {   SIP_DUMMY_STR,           eSIP_B64_USER_PW_HF_ARG, _DEC_HFSkip   },
    
};

/* Table for capabilities string to enum and hadler */
static const tTokenizer _ExtCapsArgTable[] =
{
    {SIP_CAPS_ARG_SMS_STR,           eSIP_CAPS_ARG_SMS,           0},
    {SIP_CAPS_ARG_VIDEO_SHARE_STR,   eSIP_CAPS_ARG_VIDEO_SHARE,   0},
    {SIP_CAPS_ARG_IARI_STR,          eSIP_CAPS_ARG_IARI,          0},
    {SIP_CAPS_ARG_ICSI_STR,          eSIP_CAPS_ARG_ICSI,          0},
    {SIP_CAPS_ARG_RCS_TELEPHONY_STR, eSIP_CAPS_ARG_RCS_TELEPHONY, 0},
};

/*
 * Table for capabilities string to enum, this table includes caps type
 * IARI and ICSI
 */
static const tTokenizer _ExtCapabilitiesTable[] =
{
    {SIP_CAPS_DISCOVERY_VIA_PRESENCE_STR,   eSIP_CAPS_DISCOVERY_VIA_PRESENCE,   0},
    {SIP_CAPS_IP_VOICE_CALL_STR,            eSIP_CAPS_IP_VOICE_CALL,            0},
    {SIP_CAPS_MESSAGING_STR,                eSIP_CAPS_MESSAGING,                0},
    {SIP_CAPS_FILE_TRANSFER_STR,            eSIP_CAPS_FILE_TRANSFER,            0},
    {SIP_CAPS_IMAGE_SHARE_STR,              eSIP_CAPS_IMAGE_SHARE,              0},
    {SIP_CAPS_VIDEO_SHARE_WITHOUT_CALL_STR, eSIP_CAPS_VIDEO_SHARE_WITHOUT_CALL, 0},
    {SIP_CAPS_CHAT_STR,                     eSIP_CAPS_CHAT,                     0},
    {SIP_CAPS_SOCIAL_PRESENCE_STR,          eSIP_CAPS_SOCIAL_PRESENCE,          0},
    {SIP_CAPS_GEOLOCATION_PUSH_STR,         eSIP_CAPS_GEOLOCATION_PUSH,         0},
    {SIP_CAPS_GEOLOCATION_PULL_STR,         eSIP_CAPS_GEOLOCATION_PULL,         0},
    {SIP_CAPS_FILE_TRANSFER_HTTP_STR,       eSIP_CAPS_FILE_TRANSFER_HTTP,       0},
    {SIP_CAPS_FILE_TRANSFER_THUMBNAIL_STR,  eSIP_CAPS_FILE_TRANSFER_THUMBNAIL,  0},
    {SIP_CAPS_FILE_TRANSFER_STORE_FWD_STR,  eSIP_CAPS_FILE_TRANSFER_STORE_FWD,  0},
};

/* 
 *****************************************************************************
 * ================DEC_Init===================
 *
 * This function itializes the decoder module.  Moreover, the mandatory
 * header fields are specified in bitmasks for each of the possible SIP 
 * methods.  If your adding a SIP method, add your code here in this 
 * function
 *
 * RETURNS:
 *         Nothing
 * 
 ******************************************************************************
 */
void DEC_Init(void)
{
    /* If need be, add any specific init code here */
    return;
}


/* 
 *****************************************************************************
 * ================DEC_Msg()===================
 *
 * This function decodes an external SIP message and populates an internal 
 * tSipIntMsg object.
 *
 * pBuff = A pointer to an object used to manage parsing the external SIP
 *         format.
 *
 * pMsg = A pointer to a tSipIntMsg to be populated with the dat parsed from 
 *        pBuff.
 *
 * pIsUsingCompactForm = A pointer to a vint value.  This will be populated
 * with TRUE if the sip message was using compact form and FALSE if it didn't.
 * Please note that if any part of the message is in "compact" form 
 * then this will be true.
 *
 * NOTE:  If a part of the messge is misunderstood, decoding continues just
 *        as specified in RFC3261. It's a "Do your best" philosophy.
 *
 * RETURNS:
 *         SIP_PROTO_ERROR: Error with parsing the message
 *         SDP_PROTO_ERROR: Error with parsing the SDP message
 *
 ******************************************************************************
 */
vint DEC_Msg(
    tL4Packet  *pBuff, 
    tSipIntMsg *pMsg, 
    vint       *pIsUsingCompact)
{
    tFSM   MsgFSM;
    uint32    x;
    vint isUsingCompactForm = FALSE;
    
    OSAL_memSet(&MsgFSM, 0, sizeof(tFSM));

    MsgFSM.pfGetToken = TOKEN_Get;
    MsgFSM.Status = SIP_OK;

    /* to skip empty lines */
    TOKEN_SkipEmptyLines(pBuff, &MsgFSM.isEndOfPacket);
   
    if (MsgFSM.isEndOfPacket) {
          SIP_MSG_CODE(pMsg, eSIP_RSP_NOT_ACCEPTABLE);
          MsgFSM.Status = SIP_PROTO_ERROR;
          goto label_DEC_Msg_ErrorHndl;
    }

    /* get the first token to see of it's response or request */
    TOKEN_Get(&MsgFSM, pBuff, " ");
    if (TOKEN_ExtLookup(&MsgFSM.CurrToken, _ExtMethodTable, eSIP_LAST_METHOD, &x) == SIP_OK) {
        pMsg->method = (tSipMethod)_ExtMethodTable[x].Int;
        _DEC_Request(&MsgFSM, pBuff, pMsg);
    }
    else if (_DEC_Version(&MsgFSM, pBuff, pMsg) == SIP_OK) {
        _DEC_Response(&MsgFSM, pBuff, pMsg);
    }
    else {
        /* assume it's a messed up request or an unsupported one */
        pMsg->msgType = eSIP_REQUEST;
        pMsg->method = eSIP_ERROR;
        SIP_MSG_CODE(pMsg, eSIP_RSP_METHOD_NOT_ALLOWED);
        _DEC_Request(&MsgFSM, pBuff, pMsg);
    }

    TOKEN_SkipEmptyLines(pBuff, &MsgFSM.isEndOfPacket);
    if (MsgFSM.isEndOfPacket) {
        MsgFSM.Status = SIP_PROTO_ERROR;
        goto label_DEC_Msg_ErrorHndl;
    }
   
    if (_DEC_HeaderFields(&MsgFSM, pBuff, 
            pMsg, &isUsingCompactForm) != SIP_OK) {
        SIP_DebugLog(SIP_DB_DECODE_LVL_1, "DEC_HeaderFields failed", 0, 0, 0);
        MsgFSM.Status = SIP_PROTO_ERROR;
    }

    SIP_DebugLog(SIP_DB_DECODE_LVL_3, "DEC_Msg: Finished with header fields", 0, 0, 0);
    
    TOKEN_SkipEmptyLines(pBuff, &MsgFSM.isEndOfPacket);
    if (MsgFSM.isEndOfPacket)
        goto label_DEC_Msg_ErrorHndl;
    
    if (pMsg->ContentLength != 0) {
        /* Then there must be something else */
        if (pMsg->ContentType == eCONTENT_TYPE_SDP) {
            /* SDPAnnouncement decoding */
            MsgFSM.pfHandler  = (tpfTokenHndlr)_DEC_SdpInfo;
            if (MsgFSM.pfHandler(&MsgFSM, pBuff, pMsg) != SIP_OK) {
                SIP_MSG_CODE(pMsg, eSIP_RSP_NOT_ACCEPTABLE_HERE);
            }
        }
        else if (pMsg->ContentType == eCONTENT_TYPE_SIPFRAG) {
            /* don't worry about what this function returns, continue anyway */
            _DEC_NotifyBody(&MsgFSM, pBuff, pMsg);
        }
        else if (pMsg->ContentType == eCONTENT_TYPE_MULTIPART) {
            MsgFSM.pfHandler  = (tpfTokenHndlr)_DEC_MultipartBody;
            if (MsgFSM.pfHandler(&MsgFSM, pBuff, pMsg) != SIP_OK) {
                SIP_MSG_CODE(pMsg, eSIP_RSP_NOT_ACCEPTABLE_HERE);
            }
        }
        else {
            /* don't worry about what this function returns, continue anyway */
             _DEC_BinaryMessageBody(&MsgFSM, pBuff, pMsg);
        }
    }
label_DEC_Msg_ErrorHndl:
    if (pMsg->msgType == eSIP_REQUEST && MsgFSM.Status != SIP_OK)
        _DEC_HandleProtocolError(&MsgFSM, pMsg);
    
    if (pIsUsingCompact)
        *pIsUsingCompact = isUsingCompactForm;
    
    return MsgFSM.Status;
}

vint DEC_HeaderFields(
    tSipIntMsg *pMsg, 
    char       *pHdrFld)
{
    tFSM fsm;
    tL4Packet buff;
    vint dummy;

    OSAL_memSet(&fsm, 0, sizeof(tFSM));
    buff.frame = 0;
    buff.isOutOfRoom = FALSE;
    buff.length = OSAL_strlen(pHdrFld);
    buff.pCurr = buff.pStart = pHdrFld;
    return _DEC_HeaderFields(&fsm, &buff, pMsg, &dummy);
}

static vint _DEC_NotifyBody(tFSM *pFSM, tL4Packet *pBuff, tSipIntMsg *pMsg)
{
    char *pEnd;
    uint16   code;
    vint  bIsEndOfMsg;
    /* This function should NOT return error even if we
     * don't know what the body of the message says 
     */
    
    /* get the first token which is the version, 
     * but we don't need it so blow it off 
     */
    if (TOKEN_Get(pFSM, pBuff, " ") == SIP_OK) {
        TOKEN_SkipWS(pBuff);
        /* this should be the response code */
        if (TOKEN_Get(pFSM, pBuff, " ") == SIP_OK) {
            code = (uint16)OSAL_strtoul(pFSM->CurrToken.pStart, &pEnd, 10);
            if ((pMsg->sipfragCode = MSGCODE_GetInt((int)code)) == eSIP_RSP_LAST_RESPONSE_CODE) {
                /* then this code is not understood */
                pMsg->sipfragCode = eSIP_RSP_CODE_UNKNOWN;
            }
            else {
                TOKEN_SkipEmptyLines(pBuff, &bIsEndOfMsg);
                return (SIP_OK);
            }
        }
    }
    /* if are here then this message is messed */
    pMsg->sipfragCode = eSIP_RSP_CODE_UNKNOWN;
    TOKEN_SkipEmptyLines(pBuff, &bIsEndOfMsg);
    return (SIP_FAILED);
}

static vint _DEC_BinaryMessageBody(tFSM *pFSM, tL4Packet *pBuff, tSipIntMsg *pMsg)
{
    int   len;
    len = pMsg->ContentLength;
    if (len > 0) {
        /* make sure it aint too big */
        if (len > SIP_MAX_TEXT_MSG_SIZE) {
            len = SIP_MAX_TEXT_MSG_SIZE;
            pMsg->ContentLength = len;
        }
        if (len > pBuff->length) {
            len = pBuff->length;
            pMsg->ContentLength = len;
        }
        pMsg->pMsgBody = (tSipMsgBody *)SIP_memPoolAlloc(eSIP_OBJECT_SIP_MSG_BODY);
        if (pMsg->pMsgBody) {
            OSAL_memCpy(pMsg->pMsgBody->msg, pBuff->pCurr, len);

            /* terminating as if it was string, such as cwi xml */
            pMsg->pMsgBody->msg[len] = 0;

            /* advance the mark in the packet */
            pBuff->pCurr += len;
            return (SIP_OK);
        }
        else {
            return (SIP_NO_MEM);
        }
    }
    return (SIP_OK);
}

static vint _DEC_MultipartBodyHlpr(char *pStart, vint length, tSipIntMsg *pMsg)
{
    vint isUsingCompactForm = FALSE;
    tL4Packet bi;
    tFSM fsm;
    tSipIntMsg *pTempMsg;
    vint hdrSize;

    bi.frame = 0;
    bi.pCurr = bi.pStart = pStart;
    bi.length = length;
    bi.isOutOfRoom = FALSE;
    OSAL_memSet(&fsm, 0, sizeof(tFSM));
    fsm.Status = SIP_OK;
    fsm.pfGetToken = TOKEN_Get;
    fsm.pfHandler  = (tpfTokenHndlr)_DEC_SdpInfo;

    /* Now get the content type and length: */

    if (NULL == (pTempMsg = SIP_allocMsg())) {
        return (SIP_FAILED);
    }

    /* set '-1' as default value of ContentLength*/
    pTempMsg->ContentLength = 0xFFFFFFFF;
    _DEC_HeaderFields(&fsm, &bi, pTempMsg, &isUsingCompactForm);

    if ((vint)pTempMsg->ContentLength == -1) {
        /* The Content-Length field is not present. then update length manually. */
        hdrSize = fsm.CurrToken.pStart - pStart;
        pTempMsg->ContentLength = (length >= hdrSize) ? (length - hdrSize) : 0;
    }

    if (pTempMsg->ContentLength != 0) {
        /* Then there must be something else */
        if (pTempMsg->ContentType == eCONTENT_TYPE_SDP) {
            /* SDPAnnouncement decoding xxx pTempMsg vs pMsg for multipart */
            if (_DEC_SdpInfo(&fsm, &bi, pMsg) != SIP_OK) {
                SIP_freeMsg(pTempMsg);
                SIP_DebugLog(SIP_DB_DECODE_LVL_1,
                        "_DEC_MultipartBodyHlpr SDP failed to parse", 0, 0, 0);
                return (SIP_FAILED);
            }
            else {
                SIP_DebugLog(SIP_DB_DECODE_LVL_1, 
                        "_DEC_MultipartBodyHlpr SDP successfully parsed", 0, 0, 0);
            }
        }
        else {
            /* don't worry about what this function returns, continue anyway */
            int oldLen = pMsg->ContentLength; // xxx short term solution for multipart
            pMsg->ContentLength = pTempMsg->ContentLength;
            _DEC_BinaryMessageBody(&fsm, &bi, pMsg);
            SIP_DebugLog(SIP_DB_DECODE_LVL_1,
                    "_DEC_MultipartBodyHlpr pMsgBody:%s \n partLen:%d fullLen:%x \n",
                    (int)pMsg->pMsgBody->msg, pMsg->ContentLength, oldLen);
            pMsg->ContentLength = oldLen;
        }
    }
    SIP_freeMsg(pTempMsg);
    return (SIP_OK);
}


static vint _DEC_MultipartBody(tFSM *pFSM, tL4Packet *pBuff, tSipIntMsg *pMsg)
{
    /* Init new buffer's & FSM. */
    vint lenLeft;
    char boundary[SIP_BOUNDRY_MAX_LEN + 8]; /* Add a few bytes for seperators, EOL's */

    if (0 == pMsg->boundary[0]) {
        return (SIP_FAILED);
    }

    OSAL_snprintf(boundary, sizeof(boundary) - 1, "%s%s%s", "--", pMsg->boundary, SIP_CRLF);

    if (SIP_OK != TOKEN_GetBlock(pFSM, pBuff, boundary) || pFSM->isEndOfPacket) {
        return (SIP_FAILED);
    }

    while (SIP_OK == TOKEN_GetBlock(pFSM, pBuff, boundary)) {
        _DEC_MultipartBodyHlpr(pFSM->CurrToken.pStart, pFSM->CurrToken.length, pMsg);
    }
    /* No more boundaries. Let's process the remaining chuck. First look for the 'end' boundary value */
    OSAL_snprintf(boundary, sizeof(boundary) - 1, "%s%s%s", "--", pMsg->boundary, "--");
    if (SIP_OK == TOKEN_GetBlock(pFSM, pBuff, boundary)) {
        _DEC_MultipartBodyHlpr(pFSM->CurrToken.pStart, pFSM->CurrToken.length, pMsg);
    }
    else {
        /* Then there's no last 'end' boundry value, process what we have. */
        lenLeft = pBuff->length - (pBuff->pCurr - pBuff->pStart);
        _DEC_MultipartBodyHlpr(pBuff->pCurr, lenLeft, pMsg);
    }
    return (SIP_OK);
}

/* 
 *****************************************************************************
 * ================DEC_Uri()===================
 *
 * This function decodes a string representing a uri and places the data in 
 * to a tUri object specifed by pUri.  Examples of URI's are...
 *
 * "sip:sparrish@d2tech.com"
 * "sips:randmaa@client.d2tech.com:5060;maddr=10.1.1.1;transport=udp"
 *
 * str = A pointer to a string containing the uri
 *
 * size = The length of the string
 *
 * pUri = The target Uri to place the decoded data into
 *
 * RETURNS:
 *         SIP_OK: The uri was valid and decoded
 *         SIP_FAILED: The string could not be decoded.
 *
 ******************************************************************************
 */
vint DEC_Uri(
    char   *str, 
    uint16  size, 
    tUri   *pUri)
{
    /* calls to the private uri decoder.
     * The TRUE means that there is a scheme 
     */
    if (size >= SIP_URI_STRING_MAX_SIZE)
        return (SIP_FAILED);
    else 
        return _DEC_UriDecode(TRUE, str, size, pUri); 
}

/* 
 *****************************************************************************
 * ================DEC_UriNoScheme()===================
 *
 * This function decodes a string representing a uri, but with no "scheme" and
 * places the data into a tUri object specifed by pUri. An example of a URI with 
 * no scheme is...
 *
 * "ipbx.hq.d2tech.com:5060"
 *
 * str = A pointer to a string containing the uri
 *
 * size = The length of the string
 *
 * pUri = The target Uri to place the decoded data into
 *
 * RETURNS:
 *         SIP_OK: The uri was valid and decoded
 *         SIP_FAILED: The string could not be decoded.
 *
 ******************************************************************************
 */
vint DEC_UriNoScheme(
    char   *str, 
    uint16  size, 
    tUri   *pUri)
{
    /* calls to the private uri decoder.
     * The TRUE means that there is a scheme 
     */
    if (size >= SIP_URI_STRING_MAX_SIZE)
        return (SIP_FAILED);
    else 
        return _DEC_UriDecode(FALSE, str, size, pUri); 
}

vint DEC_ReferTo2Replaces(
    char        *str, 
    uint16       size,
    tReplacesHF *pReplaces)
{
    tL4Packet bi;
    tFSM fsm;
    
    bi.frame = 0;
    bi.pCurr = bi.pStart = str;
    bi.length = size;
    bi.isOutOfRoom = FALSE;

    OSAL_memSet(&fsm, 0, sizeof(tFSM));

    SIP_DebugLog(SIP_DB_DECODE_LVL_3, "DEC_ReferTo2Replaces: about to decode 'replaces'", 0, 0, 0);

    return _DEC_ReferToHlpr(&fsm, &bi, pReplaces);
}


static vint _DEC_Response(tFSM *pFSM, tL4Packet *pBuff, tSipIntMsg *pMsg)
{
    vint haveCode = FALSE;
    char myToken[] = " \r\n";
    char *pMyToken = myToken;
    char *pEnd;
    uint16 code;
        
    pMsg->msgType = eSIP_RESPONSE;
    
    while (TOKEN_Get(pFSM, pBuff, pMyToken) == SIP_OK) {
        if (pFSM->isEndOfPacket) return (SIP_FAILED); /* this message is messed up */
        
        if (ABNF_ISCRLF(pFSM->CurrToken.pDmtr)) {
            if (!haveCode) {
                code = (uint16)OSAL_strtoul(pFSM->CurrToken.pStart, &pEnd, 10);
                if ((pMsg->code = MSGCODE_GetInt((int)code)) == eSIP_RSP_LAST_RESPONSE_CODE) {
                    /* then this code is not understood but we should keep processing */
                    pMsg->code = eSIP_RSP_CODE_UNKNOWN;
                }
            }
            else {
                /* must be a reason phrase */
                pMsg->pReasonPhrase = (tSipText *)SIP_memPoolAlloc(eSIP_OBJECT_SIP_TEXT);
                if (NULL == pMsg->pReasonPhrase) {
                    return (SIP_FAILED);
                }
                TOKEN_copyToBuffer(pMsg->pReasonPhrase->msg,
                        sizeof(pMsg->pReasonPhrase->msg), &pFSM->CurrToken);
            }
            return (SIP_OK);
        }
        else {
            haveCode = TRUE;
            code = (uint16)OSAL_strtoul(pFSM->CurrToken.pStart, &pEnd, 10);
            if ((pMsg->code = MSGCODE_GetInt((int)code)) == eSIP_RSP_LAST_RESPONSE_CODE) {
                /* then this code is not understood but we should keep processing */
                pMsg->code = eSIP_RSP_CODE_UNKNOWN;
            }
            /* we are no longer interested in spaces */
            pMyToken = &myToken[1];
        }
    }
    return (SIP_OK);
}

static vint _DEC_Request(tFSM *pFSM, tL4Packet *pBuff, tSipIntMsg *pMsg)
{
    /* now get the requestUri */
    if (TOKEN_Get(pFSM, pBuff, " \r\n") != SIP_OK) {
        return (SIP_FAILED);
    }
    
    if (_DEC_UriDecode(TRUE, pFSM->CurrToken.pStart, 
        pFSM->CurrToken.length, &pMsg->requestUri) != SIP_OK) {
        /* problem with the uri string */
        SIP_MSG_CODE(pMsg, eSIP_RSP_ADDR_INCOMPLETE);
    }
    
    if (pFSM->isEndOfPacket) {
        return (SIP_FAILED);
    }
        
    if (ABNF_ISCRLF(pFSM->CurrToken.pDmtr)) {
        /* serious issue with the end of request (start) line */
        SIP_MSG_CODE(pMsg, eSIP_RSP_VERSION_NO_SUPPORT);
        /* return ok so we still keep processing the message */
        return (SIP_OK);
    }
    
    /* now check the version string */
    if (TOKEN_Get(pFSM, pBuff, SIP_CRLF) == SIP_OK) {
        if (_DEC_Version(pFSM, pBuff, pMsg) != SIP_OK) {
            /* problem with the version string */
            SIP_MSG_CODE(pMsg, eSIP_RSP_VERSION_NO_SUPPORT);
        }
    }
    else {
        /* serious issue with the end of request (start) line */
        return (SIP_FAILED);
    }
    return (SIP_OK);
}

static vint _DEC_HeaderFields(tFSM *pFSM, tL4Packet *pBuff, tSipIntMsg *pMsg, vint *pUsingCompact)
{
    vint   status;
    uint32 x;
    uint32 tableSize;
    vint   isUsingCompactForm = FALSE;
#if (SIP_DEBUG_LOG)
    char dStr[255];
#endif

    while (TOKEN_Get(pFSM, pBuff, ":\r\n") == SIP_OK) {
#if (SIP_DEBUG_LOG)
        TOKEN_copyToBuffer(dStr, sizeof(dStr), &pFSM->CurrToken);
        SIP_DebugLog(SIP_DB_DECODE_LVL_3, "DEC_HeaderFields: Decoding HF:%s", (int)dStr, 0, 0);
#endif

        if (pFSM->isEndOfPacket) { 
            break; 
        }
                
        if (ABNF_ISCRLF(pFSM->CurrToken.pDmtr)) { 
            if (pFSM->CurrToken.length == 0) {
                /* then this is an empty line */
                break;
            }
            else {
                /* then you have some unwanted characters.
                 * Just ignore them and keep processing 
                 */
                continue;
            }
        }
        
        /* then check compact form */
        if (pFSM->CurrToken.length == 1) {
            tableSize = (sizeof(_ExtCHFtable) / sizeof(_ExtCHFtable[0]));
            status = TOKEN_ExtLookup(&pFSM->CurrToken, _ExtCHFtable, tableSize, &x);
            if (status != SIP_OK) {
                /* 
                 * Header field is unknown. Get the rest of the 
                 * next line and copy 
                 */
                pFSM->pfHandler = (tpfTokenHndlr)_DEC_Unknown;
            }
            else {
                HF_SetPresence(&pMsg->x.DCPresenceMasks, (tHdrFld)_ExtCHFtable[x].Int);
                /* for testing, set the encoding presence as well */
                HF_SetPresence(&pMsg->x.ECPresenceMasks, (tHdrFld)_ExtCHFtable[x].Int); 
                pFSM->pfHandler = (tpfTokenHndlr)_ExtCHFtable[x].pfHandler;
                isUsingCompactForm = TRUE;
            }
        } /* end of if where length is one */
        else {
            tableSize = (sizeof(_ExtHFtable) / sizeof(_ExtHFtable[0]));
            status = TOKEN_ExtLookup(&pFSM->CurrToken, _ExtHFtable, tableSize, &x);
            if (status != SIP_OK) {
                /* 
                 * Header field is unknown. Get the rest of the next 
                 * line and copy 
                 */
                pFSM->pfHandler = (tpfTokenHndlr)_DEC_Unknown;
            }
            else {
                HF_SetPresence(&pMsg->x.DCPresenceMasks, (tHdrFld)_ExtHFtable[x].Int);
                /* for testing, set the encoding presence as well */
                HF_SetPresence(&pMsg->x.ECPresenceMasks, (tHdrFld)_ExtHFtable[x].Int);
                pFSM->pfHandler = (tpfTokenHndlr)_ExtHFtable[x].pfHandler;
            }
        }
               
        /* get rid of white space */
        TOKEN_SkipWS(pBuff);
        /* launch request line arg */
        status = pFSM->pfHandler(pFSM, pBuff, pMsg);
        if (status != SIP_OK) {
            /* there must be a fundamental parser error but we want to keep going */
            if (pFSM->isEndOfPacket) { 
                break; 
            }
            if (!(ABNF_ISCRLF(pFSM->CurrToken.pDmtr))) {
                SIP_DebugLog(SIP_DB_DECODE_LVL_1, "DEC_HeaderFields: Error parsing the %s header field but we will keep going", (int)dStr, 0, 0);
                if (TOKEN_Get(pFSM, pBuff, "\r\n") != SIP_OK) { 
                    break; 
                }
            }
        }
    } /* end of while */
    *pUsingCompact = isUsingCompactForm;
    return (SIP_OK);
}

static vint _DEC_HandleProtocolError(tFSM *pFSM, tSipIntMsg *pMsg)
{
    if (pFSM->Status == SIP_PROTO_ERROR) {

    }
    else if (pFSM->Status == SDP_PROTO_ERROR) {

        /* add special handling here */
        pMsg->msgType = eSIP_RESPONSE;
        pMsg->code = (tSipMsgCodes)pFSM->ErrorCode;
        /* copy the DC presence to EC so what we got is what is returned */
        pMsg->x.ECPresenceMasks = pMsg->x.DCPresenceMasks;
    }
    return (SIP_OK);
}

static vint _DEC_HFSkip(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg)
{
    /* just get the next line */
    UNUSED(hMsg);
    return TOKEN_Get(pFSM, pBuff, SIP_CRLF);
}

static vint _DEC_HFhelper(
    tFSM       *pFSM, 
    tL4Packet  *pBuff, 
    tSipIntMsg *pMsg, 
    tHdrFld     hf,
    uint32      maxSize,
    tL4Packet  *pOutBuff)
{
    int          len;
    char        *pStr;
    char        *pStart;
    tHdrFldList *pList;
    int          offset;

    pStart = pFSM->CurrToken.pStart;
    
    if (!pStart) {
        return (SIP_FAILED);
    }

    if (TOKEN_Get(pFSM, pBuff, SIP_CRLF) == SIP_OK) {
        offset = (pFSM->CurrToken.pStart - pStart);
        len = CALC_MIN(offset + pFSM->CurrToken.length, (SIP_HF_STR_SIZE_MAX - 1));
        /* then insert it in the list */
        pList = (tHdrFldList *)SIP_memPoolAlloc(eSIP_OBJECT_HF_LIST);
        if (pList) {
            pStr = pList->pStart;
            OSAL_memCpy(pStr, pStart, len);
            pStr[len] = 0;
            pList->hf = hf;
            pList->pField = pStr + offset;
            pList->pNext = NULL;
            HF_Insert(&pMsg->pHFList, pList);
            /* 
             * Now init the out buffer for more processing if the user 
             * needs this.
             */
            if (NULL != pOutBuff) {
                pOutBuff->frame = 0;
                pOutBuff->isOutOfRoom = FALSE;
                pOutBuff->length = pFSM->CurrToken.length;
                pOutBuff->pCurr = pOutBuff->pStart = (pStr + offset);
            }
            return (SIP_OK);
        }
        else {
            return (SIP_NO_MEM);
        }
    }
    return (SIP_FAILED);
}

static vint _DEC_Event(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg) 
{
    tSipIntMsg  *pMsg = (tSipIntMsg*)hMsg;
    tL4Packet    buff;
    tFSM         fsm;

    if (SIP_OK == _DEC_HFhelper(pFSM, pBuff, pMsg, eSIP_EVENT_HF, 
            SIP_EVENT_HF_STR_SIZE, &buff)) {
        OSAL_memSet(&fsm, 0, sizeof(tFSM));        
        return (_DEC_EventHlpr(&fsm, &buff, pMsg));
    }
    return (SIP_FAILED);
}
    
static vint _DEC_SubState(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg) 
{
    tSipIntMsg  *pMsg = (tSipIntMsg*)hMsg;
    tL4Packet    buff;
    tFSM         fsm;

    if (SIP_OK == _DEC_HFhelper(pFSM, pBuff, pMsg, eSIP_SUB_STATE_HF, 
            SIP_SUB_STATE_HF_STR_SIZE, &buff)) {
        OSAL_memSet(&fsm, 0, sizeof(tFSM));        
        return (_DEC_SubStateHlpr(&fsm, &buff, pMsg));
    }
    return (SIP_FAILED);
}

static vint _DEC_Authorization (tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg) {
    tSipIntMsg *pMsg = (tSipIntMsg*)hMsg;
    return _DEC_Authenticate(pFSM, pBuff, pMsg, eSIP_AUTHORIZATION_HF); 
}

static vint _DEC_CallId(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg)
{
    tSipIntMsg *pMsg = (tSipIntMsg*)hMsg;
    if (TOKEN_Get(pFSM, pBuff, SIP_CRLF) == SIP_OK) {
        if (pFSM->CurrToken.length >= SIP_CALL_ID_HF_STR_SIZE)
            return (SIP_NO_ROOM);
        else
            TOKEN_copyToBuffer(pMsg->szCallId, sizeof(pMsg->szCallId), &pFSM->CurrToken);
    }
    return (SIP_OK);
}

static vint _DEC_ETag(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg)
{
    return _DEC_HFhelper(pFSM, pBuff, hMsg, eSIP_ETAG_HF, SIP_ETAG_HF_STR_SIZE, NULL);
}

static vint _DEC_IfMatch(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg)
{
    return _DEC_HFhelper(pFSM, pBuff, hMsg, eSIP_IF_MATCH_HF, SIP_IF_MATCH_HF_STR_SIZE, NULL);
}

static vint _DEC_ContentLength(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg)
{
    tSipIntMsg *pMsg = (tSipIntMsg*)hMsg;
    
    char *pEnd;
    if (TOKEN_Get(pFSM, pBuff, SIP_CRLF) == SIP_OK) {
        pMsg->ContentLength = OSAL_strtoul(pFSM->CurrToken.pStart, &pEnd, 10);
        return (SIP_OK);
    }
    return (SIP_FAILED);
}

static vint _DEC_MinExpires(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg)
{
    tSipIntMsg  *pMsg = (tSipIntMsg*)hMsg;
    char *pEnd;

    if (TOKEN_Get(pFSM, pBuff, SIP_CRLF) == SIP_OK) {
        pMsg->MinExpires = OSAL_strtoul(pFSM->CurrToken.pStart, &pEnd, 10);
        return (SIP_OK);
    }
    return (SIP_FAILED);
}

static vint _DEC_MinSE(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg)
{
    tSipIntMsg *pMsg = (tSipIntMsg*)hMsg;

    char *pEnd;
    if (TOKEN_Get(pFSM, pBuff, SIP_CRLF) == SIP_OK) {
        pMsg->MinSE = OSAL_strtoul(pFSM->CurrToken.pStart, &pEnd, 10);
        return (SIP_OK);
    }
    return (SIP_FAILED);
}
static vint _DEC_ContentType(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg) 
{
    tSipIntMsg  *pMsg = (tSipIntMsg*)hMsg;
    tL4Packet    buff;
    tFSM         fsm;

    if (SIP_OK == _DEC_HFhelper(pFSM, pBuff, pMsg, eSIP_CONTENT_TYPE_HF, 
            SIP_CONTENT_TYPE_HF_STR_SIZE, &buff)) {
        OSAL_memSet(&fsm, 0, sizeof(tFSM));        
        return (_DEC_ContentTypeHlpr(&fsm, &buff, pMsg));
    }
    return (SIP_FAILED);
}

static vint _DEC_MultipartBoundaryHlpr(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg)
{
    tSipIntMsg *pMsg = (tSipIntMsg*)hMsg;
    if (pFSM->isEndOfPacket || ABNF_ISCRLF(pFSM->CurrToken.pDmtr)) {
        // nothing to do.
        return (SIP_FAILED);
    }
    /* Let's get the 'boundry' value. */
    if (TOKEN_Get(pFSM, pBuff, "=\r\n") == SIP_OK) {
        if (pFSM->isEndOfPacket || ABNF_ISCRLF(pFSM->CurrToken.pDmtr)) {
            // nothing to do.
            return (SIP_FAILED);
        }
        if (TRUE == TOKEN_iCmpToken(&pFSM->CurrToken,
                SIP_CONTENT_TYPE_MULTIPART_BOUNDRY_DEC_STR)) {
            /* then get the value inside the qoutes. */
            if (TOKEN_Get(pFSM, pBuff, "\";\r\n") == SIP_OK) {
                if (pFSM->isEndOfPacket || ABNF_ISCRLF(pFSM->CurrToken.pDmtr) || ';' == *pFSM->CurrToken.pDmtr) {
                    /* We got something and there were no qoutes. Let's process it anyway. */
                    TOKEN_copyToBuffer(pMsg->boundary, sizeof(pMsg->boundary), &pFSM->CurrToken);
                    return (SIP_OK);
                }
                /* Get the other qoute. */
                if (TOKEN_Get(pFSM, pBuff, "\"\r\n") == SIP_OK) {
                    /* got it. */
                    TOKEN_copyToBuffer(pMsg->boundary, sizeof(pMsg->boundary), &pFSM->CurrToken);
                    return (SIP_OK);
                }
            }
        }
    }
    return (SIP_FAILED);
}

static vint _DEC_ContentTypeHlpr(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg)
{
    tSipIntMsg *pMsg = (tSipIntMsg*)hMsg;
    uint32  x;
    vint status;
    vint tableSize;
    status = SIP_FAILED;
    if (TOKEN_Get(pFSM, pBuff, "; \r\n") == SIP_OK) {
        tableSize = sizeof(_ExtContentType_HFTable) / sizeof(_ExtContentType_HFTable[0]);
        status = TOKEN_ExtLookup(&pFSM->CurrToken, _ExtContentType_HFTable, tableSize, &x);
        if (status == SIP_OK) {
            pMsg->ContentType = (tSipContentType)_ExtContentType_HFTable[x].Int;
            pFSM->pfHandler = (tpfTokenHndlr)_ExtContentType_HFTable[x].pfHandler;
            if (pFSM->pfHandler) {
                pFSM->pfHandler(pFSM, pBuff, hMsg);
            }
        }
        else {
            pMsg->ContentType = eCONTENT_TYPE_LAST;
        }
        if (pFSM->isEndOfPacket) {
            return (status);
        }
        /* make sure you go to the end of the line */
        if (!ABNF_ISCRLF(pFSM->CurrToken.pDmtr)) {
            TOKEN_Get(pFSM, pBuff, SIP_CRLF);
        }
    }
    return (status);
}

static vint _DEC_ExpiresHlpr(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg)
{
    tSipIntMsg *pMsg = (tSipIntMsg*)hMsg;
    char *pEnd;
    if (TOKEN_Get(pFSM, pBuff, SIP_CRLF) == SIP_OK) {
        pMsg->Expires = OSAL_strtoul(pFSM->CurrToken.pStart, &pEnd, 10);
        return (SIP_OK);
    }
    return (SIP_FAILED);
}

static vint _DEC_Expires(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg) 
{
    tSipIntMsg  *pMsg = (tSipIntMsg*)hMsg;
    tL4Packet    buff;
    tFSM         fsm;

    if (SIP_OK == _DEC_HFhelper(pFSM, pBuff, pMsg, eSIP_EXPIRES_HF, 
            SIP_EXPIRES_HF_STR_SIZE, &buff)) {
        OSAL_memSet(&fsm, 0, sizeof(tFSM));        
        return (_DEC_ExpiresHlpr(&fsm, &buff, pMsg));
    }
    return (SIP_FAILED);
}

static vint _DEC_From(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg)
{
    tSipIntMsg *pMsg = (tSipIntMsg*)hMsg;
    return _DEC_DecodeUriPlus(pFSM, pBuff, &pMsg->From); 
}
static vint _DEC_ReferTo(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg)
{
    tSipIntMsg *pMsg = (tSipIntMsg*)hMsg;
    tFSM fsm;
    tL4Packet buff;
    
    OSAL_memSet(&fsm, 0, sizeof(tFSM));
    if (TOKEN_Get(pFSM, pBuff, "?\r\n") == SIP_OK) {
        buff.frame = 0;
        buff.isOutOfRoom = FALSE;
        buff.length = pFSM->CurrToken.length;
        buff.pCurr = buff.pStart = pFSM->CurrToken.pStart;
        _DEC_DecodeUriPlus(&fsm, &buff, &pMsg->ReferTo.uriPlus);
        if (pFSM->CurrToken.pDmtr && (*pFSM->CurrToken.pDmtr == '?')) {
            /* then we also have "replaces" info */
            _DEC_ReferToHlpr(pFSM, pBuff, &pMsg->ReferTo.replaces);
        }
    }
    return (SIP_OK);
}
/*
 * This header field contains an authentication challenge is included as a part
 * of the 407 (Proxy Authentication Required) response.
 */
static vint _DEC_Authenticate(tFSM *pFSM, tL4Packet *pBuff, tSipIntMsg *pMsg, tHdrFld hdr) 
{
    tAuthorizationHFE *pAuth;
    char *pCurr;

    pCurr = NULL;

    if (NULL == (pAuth = (tAuthorizationHFE*)SIP_memPoolAlloc(eSIP_OBJECT_AUTH_HF)))
        return (SIP_NO_MEM);
    else {

        pAuth->hdrFld = hdr;

        TOKEN_SkipWS(pBuff);
        
        if (TOKEN_Get(pFSM, pBuff, " \r\n") == SIP_OK) {
            if (TOKEN_iCmpToken(&pFSM->CurrToken, SIP_DIGEST_HF_ARG_STR) == TRUE) 
                pAuth->type = eSIP_AUTH_TYPE_DIGEST;
            else 
                pAuth->type = eSIP_AUTH_TYPE_BASIC;

            if (pFSM->isEndOfPacket || ABNF_ISCRLF(pFSM->CurrToken.pDmtr)) {
                DLLIST_InitEntry(&pAuth->dll);
                return DLLIST_Enqueue(&pMsg->AuthorizationList, &pAuth->dll);
            }
        }
        TOKEN_SkipWS(pBuff);
        while (TOKEN_Get(pFSM, pBuff, ":=,\r\n") == SIP_OK) {
            uint32 x;
            vint status;

            if (pFSM->isEndOfPacket) break;
            if (ABNF_ISCRLF(pFSM->CurrToken.pDmtr)) {
                /*
                 * There may have CRLF in between challenge. So need to check the stuff after CRLF.
                 *
                 * Store current position and lookup next token
                 * If it's not an auth parameters, then restore the position and return.
                 */
                pCurr = pBuff->pCurr;
                continue;
            }

            if (*pFSM->CurrToken.pDmtr == ',') continue;

            /*
             * The ":" belong to next SIP header. So it is time to finish
             * Authentication parsing.
             */
            if (*pFSM->CurrToken.pDmtr == ':') { 
                if (NULL != pCurr) {
                    /* Restore pCurr and return */
                    pBuff->pCurr = pCurr;
                    break;
                }
                else {
                    SIP_DebugLog(SIP_DB_DECODE_LVL_1, "_DEC_Authenticate: "
                            "Found : but no stored pCurr", 0, 0, 0);
                    return (SIP_BAD_HF);
                }
            }
                                     
            status = TOKEN_ExtLookup(&pFSM->CurrToken, _ExtAuthTable, eSIP_LAST_AUTH_ARG, &x);
            if (status == SIP_BADPARM || status == SIP_NOT_FOUND) {
                if (NULL != pCurr) {
                    /* Restore pCurr and return */
                    pBuff->pCurr = pCurr;
                    break;
                }
                else {
                    continue;
                }
            }
            /* This is a known parameter, set pCurr to NULL */
            pCurr = NULL;       
            pFSM->pfHandler = (tpfTokenHndlr)_ExtAuthTable[x].pfHandler;
            
            if (pFSM->pfHandler(pFSM, pBuff, pAuth) == SIP_OK)
                HF_ARG_SET_PRESENCE(&pAuth->presence, _ExtAuthTable[x].Int);

            if (pFSM->isEndOfPacket) break;

            if (ABNF_ISCRLF(pFSM->CurrToken.pDmtr)) {
                /*
                 * There may have CRLF in between challenge. So need to check the stuff after CRLF.
                 *
                 * Store current position and lookup next token
                 * If it's not an auth parameters, then restore the position and return.
                 */
                pCurr = pBuff->pCurr;
            }
            TOKEN_SkipWS(pBuff);
        }
    }
    DLLIST_InitEntry(&pAuth->dll);
    return DLLIST_Enqueue(&pMsg->AuthorizationList, &pAuth->dll);
}        

/*
 * ======== _DEC_ProxyAuthenticate() ========
 * This function decodes the Proxy-Authenticate message. 
 * The Proxy authorization and WWW authentication seem to have the
 * same fields and hence a common parser has been written
 * RfC: 2616  has the fields defined in detail.
 * 
 * RETURNS SIP_OK - Function was successful
 *         SIP_FAILED - Function Failed
 */
static vint _DEC_ProxyAuthenticate(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg) 
{
    tSipIntMsg *pMsg = (tSipIntMsg*)hMsg;
    return _DEC_Authenticate(pFSM, pBuff, pMsg, eSIP_PROXY_AUTHENTICATE_HF);

}
/*
 * ======== _DEC_ProxyAuthorization() ========
 * This function decodes the Proxy-Authorization message. 
 * 
 * RETURNS SIP_OK - Function was successful
 *         SIP_FAILED - Function Failed
 */
static vint _DEC_ProxyAuthorization(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg) 
{
    tSipIntMsg *pMsg = (tSipIntMsg*)hMsg;
    return _DEC_Authenticate(pFSM, pBuff, pMsg, eSIP_PROXY_AUTHORIZATION_HF); 
   
}
/*
 * ======== _DEC_WWW_Authenticate() ========
 * This function decodes the WWW-Authenticate message. 
 * The Proxy authorization and WWW authentication seem to have the
 * same fields and hence a common parser has been written
 * RfC: 2616  has the fields defined in detail.
 * 
 * RETURNS SIP_OK - Function was successful
 *         SIP_FAILED - Function Failed
 */
static vint _DEC_WWW_Authenticate(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg) 
{
    tSipIntMsg *pMsg = (tSipIntMsg*)hMsg;
    return _DEC_Authenticate(pFSM, pBuff, pMsg, eSIP_WWW_AUTHENTICATE_HF); 
}

static vint _DEC_ServiceRoute(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg)
{
    tSipIntMsg *pMsg = (tSipIntMsg*)hMsg;
    return _DEC_RouteHlpr(pFSM, pBuff, &pMsg->ServiceRouteList);
}

static vint _DEC_RecordRoute(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg)
{
    tSipIntMsg *pMsg = (tSipIntMsg*)hMsg;
    return _DEC_RouteHlpr(pFSM, pBuff, &pMsg->RecRouteList);
}

static vint _DEC_Route(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg)
{
    tSipIntMsg *pMsg = (tSipIntMsg*)hMsg;
    return _DEC_RouteHlpr(pFSM, pBuff, &pMsg->RouteList);
}

static vint _DEC_RouteHlpr(tFSM *pFSM, tL4Packet *pBuff, tDLList *pList)
{
    tRouteHFE *pRoute = NULL;
    while (TOKEN_Get(pFSM, pBuff,"<>, \r\n") == SIP_OK) {
        if (pFSM->CurrToken.length > 0) {
            if (NULL != (pRoute = (tRouteHFE *)SIP_memPoolAlloc(eSIP_OBJECT_ROUTE_HF))) {
                DLLIST_InitEntry(&pRoute->dll);
                if (_DEC_UriDecode(TRUE, pFSM->CurrToken.pStart, pFSM->CurrToken.length, &pRoute->uri) == SIP_OK) {
                    DLLIST_Enqueue(pList, &pRoute->dll);
                }
                else {
                    SIP_memPoolFree(eSIP_OBJECT_ROUTE_HF, (tDLListEntry *)pRoute);
                }
            }
        }
        if (pFSM->isEndOfPacket || ABNF_ISCRLF(pFSM->CurrToken.pDmtr)) {
            break;
        }
    }
    return (SIP_OK);
}

static vint _DEC_RSeq(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg)
{
    tSipIntMsg *pMsg = (tSipIntMsg*)hMsg;
    char *pEnd;
    if (TOKEN_Get(pFSM, pBuff, SIP_CRLF) == SIP_OK) {
        pMsg->RSeq = OSAL_strtoul(pFSM->CurrToken.pStart, &pEnd, 10);
        return (SIP_OK);
    }
    return (SIP_FAILED);
}

static vint _DEC_RAck(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg)
{
    tSipIntMsg *pMsg = (tSipIntMsg*)hMsg;
    char *pEnd;
    uint32 x;
    vint status;
    if (TOKEN_Get(pFSM, pBuff, " \r\n") == SIP_OK) {
        pMsg->RAck.seqNum = OSAL_strtoul(pFSM->CurrToken.pStart, &pEnd, 10);
        if ((!pFSM->isEndOfPacket) && (!ABNF_ISCRLF(pFSM->CurrToken.pDmtr))) {
            if (TOKEN_Get(pFSM, pBuff, " \r\n") == SIP_OK) {
                pMsg->RAck.cseq.seqNum = OSAL_strtoul(pFSM->CurrToken.pStart, &pEnd, 10);
                if ((!pFSM->isEndOfPacket) && (!ABNF_ISCRLF(pFSM->CurrToken.pDmtr))) {
                    if (TOKEN_Get(pFSM, pBuff, "\r\n") == SIP_OK) {
                        status = TOKEN_ExtLookup(&pFSM->CurrToken, _ExtMethodTable, eSIP_LAST_METHOD, &x);
                        if (status == SIP_OK) {
                            pMsg->RAck.cseq.method = (tSipMethod)_ExtMethodTable[x].Int;
                            return (SIP_OK);
                        }
                    }
                }
            }
        }
    }
    pMsg->RAck.cseq.method = eSIP_ERROR;
    return (SIP_FAILED);
}

static vint _DEC_To(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg)
{
    tSipIntMsg *pMsg = (tSipIntMsg*)hMsg;
    return _DEC_DecodeUriPlus(pFSM, pBuff, &pMsg->To); 
}
static vint _DEC_Via(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg)
{
    tSipIntMsg *pMsg = (tSipIntMsg*)hMsg;
    tViaHFE *pVia;
    
    while (TOKEN_Get(pFSM, pBuff, ",\r\n") == SIP_OK) {
        if (pFSM->CurrToken.length > 0) {
            if (NULL != (pVia = (tViaHFE *)SIP_memPoolAlloc(eSIP_OBJECT_VIA_HF))) {
                if (_DEC_ViaStr(pFSM->CurrToken.pStart, pFSM->CurrToken.length, pVia) == SIP_OK) {
                    DLLIST_InitEntry(&pVia->dll);
                    DLLIST_Enqueue(&pMsg->ViaList, &pVia->dll);    
                }
                else {
                    SIP_memPoolFree(eSIP_OBJECT_VIA_HF, (tDLListEntry *)pVia);
                }
            }
        }
        if (pFSM->isEndOfPacket || ABNF_ISCRLF(pFSM->CurrToken.pDmtr)) 
            break;
    }
    return (SIP_OK);
}

static vint _DEC_ViaStr(char *pViaStr, int len, tViaHFE *pVia)
{
    uint32    x;
    uint32    tableSize;
    char     *pTemp = NULL;    /* init isn't necessary, but makes compiler happy */
    uint8     temp;
    tL4Packet bi;
    tFSM      fsm;
    int       bytesLeft;
    int       numBytes;
        
    bi.frame = 0;
    bi.pCurr = bi.pStart = pViaStr;
    bi.length = len;
    bi.isOutOfRoom = FALSE;
    
    OSAL_memSet(&fsm, 0, sizeof(tFSM));

    /* first handle the first part of the via i.e. SIP/2.0/UDP */
    temp = 3;
    bytesLeft = (SIP_VERSION_STR_SIZE - 3);
    tableSize = sizeof(_ExtTransportTypeTable)/sizeof(_ExtTransportTypeTable[0]);
    while (temp != 0 && TOKEN_Get(&fsm, &bi, " /") == SIP_OK) {
        if(fsm.CurrToken.length > 0) {
            if (temp == 3) {
                numBytes = CALC_MIN((int)fsm.CurrToken.length, bytesLeft);
                OSAL_memCpy(pVia->szVersion, fsm.CurrToken.pStart, numBytes);
                pVia->szVersion[numBytes] = '/';
                numBytes++;
                pVia->szVersion[numBytes] = 0;
                bytesLeft -= numBytes;
                if (bytesLeft > 0) {
                    pTemp = (pVia->szVersion + numBytes);
                }
            }
            else if (temp == 2) {
                if (bytesLeft > 0) {
                    numBytes = CALC_MIN((int)fsm.CurrToken.length, bytesLeft);
                    OSAL_memCpy(pTemp, fsm.CurrToken.pStart, numBytes);
                    pTemp[numBytes] = 0;
                }
            }
            else {
                if (TOKEN_ExtLookup(&fsm.CurrToken, _ExtTransportTypeTable, tableSize, &x) 
                        == SIP_OK) {
                    pVia->uri.transport = (tTransportType)_ExtTransportTypeTable[x].Int;
                }
                else {
                    pVia->uri.transport = eTransportNone;
                }
            }
            temp--;
        }
    }

    if (fsm.isEndOfPacket) {
        return (SIP_FAILED);  
    }
   
    /* now get and process the URI part.  
     * Remember that there will be no 'scheme' in a via (i.e. sip: 
     */
    if (TOKEN_Get(&fsm, &bi, " ;") == SIP_OK) {
        if (fsm.CurrToken.length != 0) {
            _DEC_UriDecode(FALSE, fsm.CurrToken.pStart, fsm.CurrToken.length, &pVia->uri);
        }
    }

    if (fsm.isEndOfPacket) {
        /* then there's nothing else, I return onkay here 
         * even if there isn't a branch for backwards compatibility with 2543 
         */
        return (SIP_OK);
    }

    /* now process all via parameters like branch, etc. */
    tableSize = sizeof(_ExtVia_HFArgTable) / sizeof(_ExtVia_HFArgTable[0]);
    while (_DEC_GetUriParam(&fsm, &bi) == SIP_OK) {
        /* first see if it's in the Via param table */
        if (TOKEN_ExtLookup(&fsm.CurrToken, _ExtVia_HFArgTable, tableSize, &x) == SIP_OK) {
            fsm.pfHandler = _ExtVia_HFArgTable[x].pfHandler;
            if (_ExtVia_HFArgTable[x].Int == eSIP_BRANCH_HF_ARG) {
                /* use a special handler for the via branch */
                _DEC_StringHlpr(&fsm, &bi, pVia->szBranch, SIP_BRANCH_HF_STR_SIZE);
            }
            else if (_ExtVia_HFArgTable[x].Int == eSIP_RPORT_HF_ARG) {
                _DEC_Rport(&fsm, &bi, &pVia->rport);
            }
            else if (_ExtVia_HFArgTable[x].Int == eSIP_KEEP_HF_ARG) {
                _DEC_KeepAlive(&fsm, &bi, &pVia->keepaliveFeq);
            }
            else {/* must be received */
                fsm.pfHandler(&fsm, &bi, &pVia->received);
            }
        }
        else {
            /* try the URI param table */
            if (TOKEN_ExtLookup(&fsm.CurrToken, _ExtURI_ParmTable, 
                    eSIP_LAST_URI_PARM, &x) == SIP_OK) {
                fsm.pfHandler = _ExtURI_ParmTable[x].pfHandler;
                if(_ExtURI_ParmTable[x].Int == eSIP_MADDR_URI_PARM)
                    fsm.pfHandler(&fsm, &bi, &pVia->uri.maddr);
                else if(_ExtURI_ParmTable[x].Int == eSIP_TTL_URI_PARM)
                    fsm.pfHandler(&fsm, &bi, &pVia->uri.ttl);
                else if(_ExtURI_ParmTable[x].Int == eSIP_LSKPMC_URI_PARM)
                    fsm.pfHandler(&fsm, &bi, pVia->uri.szLskpmc);
            }
        }
    }
    /* there could still be an rport */
    if (TOKEN_iCmpToken(&fsm.CurrToken, SIP_RPORT_HF_ARG_STR)) {
        pVia->rport = 1;
    }
    return (SIP_OK);
}

static vint _DEC_Version(tFSM *pFSM, tL4Packet *pBuff, tSipIntMsg *pMsg)
{
    UNUSED(pMsg);
    UNUSED(pBuff);
    if (TOKEN_iCmpToken(&pFSM->CurrToken, SIP_VERSION_STR))
        return (SIP_OK);
    return (SIP_FAILED);
}
static vint _DEC_CSeq(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg)
{
    tSipIntMsg *pMsg = (tSipIntMsg*)hMsg;
    char *pEnd;
    uint32 x;
    vint status;
    if (TOKEN_Get(pFSM, pBuff, " \r\n") == SIP_OK) {
        pMsg->CSeq.seqNum = OSAL_strtoul(pFSM->CurrToken.pStart, &pEnd, 10);
        if ((!pFSM->isEndOfPacket) && (!ABNF_ISCRLF(pFSM->CurrToken.pDmtr))) {
            if (TOKEN_Get(pFSM, pBuff, "\r\n") == SIP_OK) {
                status = TOKEN_ExtLookup(&pFSM->CurrToken, _ExtMethodTable, eSIP_LAST_METHOD, &x);
                if (status == SIP_OK) {
                    pMsg->CSeq.method = (tSipMethod)_ExtMethodTable[x].Int;
                    return (SIP_OK);
                }
            }
        }        
    }
    pMsg->CSeq.method = eSIP_ERROR;
    SIP_MSG_CODE(pMsg, eSIP_RSP_METHOD_NOT_ALLOWED);
    return (SIP_FAILED);
}

static vint _DEC_SessionExpires(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg)
{
    tSipIntMsg *pMsg = (tSipIntMsg*)hMsg;
    char *pEnd;
    if (TOKEN_Get(pFSM, pBuff, " ;\r\n") == SIP_OK) {
        pMsg->SessionTimer.expires = OSAL_strtoul(pFSM->CurrToken.pStart, &pEnd, 10);
        if ((!pFSM->isEndOfPacket) && (!ABNF_ISCRLF(pFSM->CurrToken.pDmtr))) {
            /* Then look for 'refresher' param and determine if it's 'uac' or 'uas' */
            while (TOKEN_Get(pFSM, pBuff, "=\r\n") == SIP_OK) {
                if ((pFSM->isEndOfPacket) || (ABNF_ISCRLF(pFSM->CurrToken.pDmtr))) {
                    break;
                }
                if (TOKEN_iCmpToken(&pFSM->CurrToken, SIP_SESSION_EXPIRES_REFRESHER_HF_ARG_STR)) {
                    if (TOKEN_Get(pFSM, pBuff, ";\r\n") == SIP_OK) {
                        if (TOKEN_iCmpToken(&pFSM->CurrToken, SIP_SESSION_EXPIRES_UAC_HF_ARG_STR)) {
                            pMsg->SessionTimer.refresher = eSIP_REFRESHER_UAC;
                        }
                        else if (TOKEN_iCmpToken(&pFSM->CurrToken, SIP_SESSION_EXPIRES_UAS_HF_ARG_STR)) {
                            pMsg->SessionTimer.refresher = eSIP_REFRESHER_UAS;
                        }
                        else {
                            pMsg->SessionTimer.refresher = eSIP_REFRESHER_NONE;
                        }
                        if ((pFSM->isEndOfPacket) || (ABNF_ISCRLF(pFSM->CurrToken.pDmtr))) {
                            break;
                        }
                    }
                }
            }
        }
    }
    return (SIP_OK);
}

static vint _DEC_SubStateHlpr(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg)
{
    tSipIntMsg *pMsg = (tSipIntMsg*)hMsg;
    uint32 x;
    vint status;
    vint flag;
    uint32 Int;
    
    flag = FALSE;
    while (TOKEN_Get(pFSM, pBuff, "; =\r\n") == SIP_OK) {
        if (pFSM->CurrToken.length > 0) {
            if (flag == FALSE) {       
                /* then we have the argument */
                status = TOKEN_ExtLookup(&pFSM->CurrToken, _ExtSubState_HFArgTable, eSIP_SUBS_HF_LAST_ARG, &x);
                if (status == SIP_OK) {
                    pMsg->SubState.arg = (tSipSubStateHFArg)_ExtSubState_HFArgTable[x].Int;
                    flag = TRUE;
                }
            }
            else {
                /* check for parameters */
                if (*pFSM->CurrToken.pDmtr == '=') {
                    status = TOKEN_ExtLookup(&pFSM->CurrToken, _ExtSubState_HFParmTable, eSIP_SUBS_HF_LAST_PARM, &x);
                    if (status == SIP_OK) {
                        Int = _ExtSubState_HFParmTable[x].Int;
                        if (Int == eSIP_SUBS_HF_EXPIRES_PARM) {
                            _ExtSubState_HFParmTable[x].pfHandler(pFSM, pBuff, &pMsg->SubState.expires);
                        }
                        else if (Int == eSIP_SUBS_HF_REASON_PARM){
                            _DEC_StringHlpr(pFSM, pBuff, pMsg->SubState.szReason, SIP_MAX_REASON_PHRASE_STR_LEN);
                        }
                    }
                }
            }
        }
        if (pFSM->isEndOfPacket || ABNF_ISCRLF(pFSM->CurrToken.pDmtr)) {
            break;
        }
    }
    return (SIP_OK);
}

static vint _DEC_EventHlpr(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg)
{
    tSipIntMsg *pMsg;
    uint32      x;
    vint        status;
    vint        flag;
    uint32      Int;
    
    pMsg = (tSipIntMsg*)hMsg;
    flag = FALSE;

    while (TOKEN_Get(pFSM, pBuff, "; =\r\n") == SIP_OK) {
        if (pFSM->CurrToken.length > 1) {
            if (flag == FALSE) {       
                /* then we have the "Package" argument */
                flag = TRUE;
                TOKEN_copyToBuffer(pMsg->Event.szPackage, sizeof(pMsg->Event.szPackage), &pFSM->CurrToken);
            }
            else {
                /* check for parameters */
                if (NULL != pFSM->CurrToken.pDmtr && *pFSM->CurrToken.pDmtr == '=') {
                    status = TOKEN_ExtLookup(&pFSM->CurrToken, _ExtEvent_HFParmTable, eSIP_EVENT_HF_LAST_PARM, &x);
                    if (status == SIP_OK) {
                        Int = _ExtEvent_HFParmTable[x].Int;
                        if (Int == eSIP_EVENT_HF_ID_PARM) {
                            _DEC_StringHlpr(pFSM, pBuff, pMsg->Event.szId, SIP_EVENT_HF_ID_STR_SIZE);
                        }
                        else if (Int == eSIP_EVENT_HF_PARAM_PARM){
                            _DEC_StringHlpr(pFSM, pBuff, pMsg->Event.szParam, SIP_EVENT_HF_PARAM_STR_SIZE);
                        }
                        
                    }
                }
            }
        }
        if (pFSM->isEndOfPacket || ABNF_ISCRLF(pFSM->CurrToken.pDmtr)) {
            break;
        }
    }
    return (SIP_OK);
}

static int _DEC_NameAndUriHlpr(tFSM *pFSM, tL4Packet *pBuff, tDecodeArgs args[])
{
    uint8    dQuoteCnt = 0;
    char  escSeq[] = " \";<>\r\n";
    char *pEscSeq;
    int   numElements;
    pEscSeq = escSeq;

    /* initialize the args object */
    for (numElements = 0; numElements < MAX_URI_ARGS; numElements++) {
        args[numElements].l = 0;
        args[numElements].s = NULL;
    }
    numElements = 0;

    while (TOKEN_Get(pFSM, pBuff, pEscSeq) == SIP_OK 
            && numElements < MAX_URI_ARGS) {
        if (pFSM->isEndOfPacket || ABNF_ISCRLF(pFSM->CurrToken.pDmtr)) {
            if (args[numElements].s == NULL)
                args[numElements].s = pFSM->CurrToken.pStart;
            args[numElements].l += pFSM->CurrToken.length;
            numElements++;
            return numElements;
        }
        
        switch (*(pFSM->CurrToken.pDmtr)) {
        case '\"':
            dQuoteCnt++;
            /* clear out the space as part of the sequence */
            pEscSeq = &escSeq[1];
            if (!(dQuoteCnt & 0x01)) /* then you have the other quote */
            {
                args[numElements].s = pFSM->CurrToken.pStart;
                args[numElements].l = pFSM->CurrToken.length;
                /* reinstate the space */
                pEscSeq = &escSeq[0];
            }
            break;
        case '<':
            /* then you want to look for the other bracket but NOT the ';' or ' ' */
            pEscSeq = &escSeq[3];
            /* step off the current arg and get ready for the next one */
            if (args[numElements].s != NULL)
                numElements++;
            break;
        case ';':
            args[numElements].s = pFSM->CurrToken.pStart;
            args[numElements].l = pFSM->CurrToken.length;
            numElements++;
            return numElements;
        case '>':
            args[numElements].s = pFSM->CurrToken.pStart;
            args[numElements].l = pFSM->CurrToken.length;
            numElements++;
            return numElements;
        case ' ':
            if (args[numElements].s == NULL)
                args[numElements].s = pFSM->CurrToken.pStart;
            args[numElements].l = (uint16)(pFSM->CurrToken.pDmtr - args[numElements].s);
            break;
        default:
            break;
        }
    }
    return numElements;
}

/*
 * ======== _DEC_UnknownArgHlpr() ========
 * Private helper routine to clear out unknown arguments string
 *
 * Return:
 *   SIP_OK: Unknown argument processed.
 *   SIP_FAILED: Failed on unknown argument process.
 */
static vint _DEC_UnknownArgHlpr(
    tFSM         *pFSM, 
    tL4Packet    *pBuff)
{
    /* It's the end of packet, nothing need to be processed. */
    if (pFSM->isEndOfPacket) {
        return (SIP_OK);
    }

    /* If current delimter is '=' then read out the rest. */ 
    if ('=' == *(pFSM->CurrToken.pDmtr)) {
        if (SIP_OK == TOKEN_Get(pFSM, pBuff, "\",; \r\n")) {
            if (pFSM->isEndOfPacket) {
                return (SIP_OK);
            }
            if ('\"' == *(pFSM->CurrToken.pDmtr)) {
                /* Here is a start quote, then read until end quote. */
                if (SIP_OK != TOKEN_Get(pFSM, pBuff, "\"")) {
                    /* A arg without end quote. */
                    return (SIP_FAILED);
                }
            }
        }
    }

    return (SIP_OK);
}

/*
 * ======== _DEC_ContactArgStringHlpr() ========
 * Private helper routine to decode argument string in Contact header
 * field.
 *
 * Return:
 *   SIP_OK: Decode done.
 *   SIP_FAILED: Decode failed.
 */
static vint _DEC_ContactArgStringHlpr(
    tFSM         *pFSM, 
    tL4Packet    *pBuff, 
    char         *pTargetStr,
    uint32        maxSize)
{
    if (TOKEN_Get(pFSM, pBuff, ",; \r\n") == SIP_OK) {
        TOKEN_copyToBuffer(pTargetStr, maxSize, &pFSM->CurrToken);
    }
    return (SIP_OK);
}

/*
 * ======== _DEC_CapabilitiesHlpr() ========
 * Private helper routine to decode capabilities string and set the
 * capabilities to *capsBitmap_ptr.
 *
 * Return:
 *   SIP_OK: Decode done.
 *   SIP_FAILED: Decode failed.
 */
static vint _DEC_CapabilitiesHlpr(
    tFSM       *pFSM,
    tL4Packet  *pBuff,
    uint32      caps,
    uint32     *capsBitmap_ptr)
{
    uint16 tableSize;
    uint32 x;
    uint16 lenLeft;
    uint16 videoStrLen;

    SIP_DebugLog(SIP_DB_DECODE_LVL_3, "_DEC_CapabilitiesHlpr: caps:%x",
            caps, 0, 0);

    if (eSIP_CAPS_ARG_SMS == caps) {
        *capsBitmap_ptr |= eSIP_CAPS_SMS;
        return (SIP_OK);
    }
    if (eSIP_CAPS_ARG_VIDEO_SHARE == caps) {
        *capsBitmap_ptr |= eSIP_CAPS_VIDEO_SHARE;
        return (SIP_OK);
    }

    /* Process IARI and ICSI type */
    if ((eSIP_CAPS_ARG_IARI == caps) ||
            (eSIP_CAPS_ARG_ICSI == caps)) {
        tableSize = (sizeof(_ExtCapabilitiesTable) /
                sizeof(_ExtCapabilitiesTable[0]));
        /* Decode the rest of capabilities. */
        while (SIP_OK == TOKEN_Get(pFSM, pBuff, ",\"")) {
            SIP_DebugLog(SIP_DB_DECODE_LVL_3, "_DEC_CapabilitiesHlpr: "
                    "Got token:%s", (int)pFSM->CurrToken.pStart, 0, 0);
            if (0 == pFSM->CurrToken.length) {
                continue;
            }
            /* Look up the capabilities */
            if (SIP_OK == TOKEN_ExtLookup(&pFSM->CurrToken, 
                    _ExtCapabilitiesTable, tableSize, &x)) {
                /* Found the capabilities */
                *capsBitmap_ptr |= _ExtCapabilitiesTable[x].Int;
            }
            if ('\"' == *(pFSM->CurrToken.pDmtr)) {
                /* Since length is not zero, it's the end quote */
                if (eSIP_CAPS_ARG_IARI == caps) {
                    break;
                }

                /*
                 * It's ICSI type.
                 * Ip voice call and ip video call has the same icsi
                 * string, but ip video has ";video" string appends to
                 * icsi string as:
                 * +g.3gpp.icsi-ref="...";video
                 * so see if ";video" is there after icsi string.
                 */
                lenLeft = pBuff->length - (pBuff->pCurr - pBuff->pStart);
                videoStrLen = OSAL_strlen(SIP_CAPS_IP_VIDEO_CALL_SUB_STR);
                if ((lenLeft >= videoStrLen) &&
                        (0 == OSAL_strncmp(pBuff->pCurr,
                        SIP_CAPS_IP_VIDEO_CALL_SUB_STR,
                        videoStrLen))) {
                    /* Skip the ";" in the begining and the rest */
                    if ((SIP_OK != TOKEN_Get(pFSM, pBuff, ";")) ||
                            (SIP_OK != TOKEN_Get(pFSM, pBuff, ";, \r\n"))) {
                        SIP_DebugLog(SIP_DB_DECODE_LVL_3,
                                "_DEC_CapabilitiesHlpr: Failed to parse "
                                "\";video\"", 0, 0, 0);
                        return (SIP_FAILED);
                    }
                    /* Set IP video call and unset ip voice call. */
                    *capsBitmap_ptr |= eSIP_CAPS_IP_VIDEO_CALL;
                    *capsBitmap_ptr &= ~eSIP_CAPS_IP_VOICE_CALL;
                }
                break;
            }
        }
    }

    return (SIP_OK);
}

static vint _DEC_Contact(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg)
{
    tFSM          fsm;
    tL4Packet     buff;

    /* Get the whole header field. */
    if (SIP_OK == TOKEN_Get(pFSM, pBuff, "\r\n")) {
        OSAL_memSet(&fsm, 0, sizeof(tFSM));
        buff.frame = 0;
        buff.isOutOfRoom = 0;
        buff.length = pFSM->CurrToken.length;
        buff.pStart = buff.pCurr = pFSM->CurrToken.pStart;
        while (NULL !=  _DEC_ContactHlpr(&fsm, &buff, hMsg)) {
            /* Exit if it reaches the end of Contact header field */
            if (fsm.isEndOfPacket) {
                break;
            }
        } 
    }
    return (SIP_OK);    
}

static tContactHFE* _DEC_ContactHlpr(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg)
{
    tSipIntMsg   *pMsg = (tSipIntMsg*)hMsg;
    int           numElements;
    tDecodeArgs   args[MAX_URI_ARGS];
    tContactHFE  *pContact;
    uint32        x;
    vint          status;

    SIP_DebugLog(SIP_DB_DECODE_LVL_3, "_DEC_ContactHlpr", 0, 0, 0);
    numElements = _DEC_NameAndUriHlpr(pFSM, pBuff, args);
    
    SIP_DebugLog(SIP_DB_DECODE_LVL_3, "numElements: %d", numElements, 0, 0);
    if (numElements == 0) {
        return (NULL); /* we have nothing */
    }

    if (NULL == (pContact = (tContactHFE *)SIP_memPoolAlloc(eSIP_OBJECT_CONTACT_HF))) {
        return (NULL);
    }

    if (numElements == 1) {
        _DEC_UriDecode(TRUE, args[0].s, args[0].l, &pContact->uri);
    }
    else {
        x = CALC_MIN(args[0].l, (SIP_DISPLAY_NAME_STR_SIZE - 1));
        OSAL_memCpy(pContact->szDisplayName, args[0].s, x);
        pContact->szDisplayName[x] = 0;
        _DEC_UriDecode(TRUE, args[1].s, args[1].l, &pContact->uri);
    }
    if (pFSM->isEndOfPacket || ABNF_ISCRLF(pFSM->CurrToken.pDmtr)) {
        DLLIST_InitEntry(&pContact->dll);
        DLLIST_Enqueue(&pMsg->ContactList, &pContact->dll);
        return (pContact);
    }

    while (_DEC_GetContactParam(pFSM, pBuff) == SIP_OK) {
        status = TOKEN_ExtLookup(&pFSM->CurrToken, 
                _ExtContactHFArgTable, eSIP_CONTACT_HF_LAST_ARG, &x);
        if (status == SIP_OK) { 
            /* found it so launch method handler it */
            pFSM->pfHandler = (tpfTokenHndlr)_ExtContactHFArgTable[x].pfHandler;

            if (_ExtContactHFArgTable[x].Int == eSIP_CONTACT_HF_EXPIRES_ARG) {
                pFSM->pfHandler(pFSM, pBuff, &pContact->expires);
                SIP_DebugLog(SIP_DB_DECODE_LVL_3, "_DEC_Contact: handling the "
                        "expires :%d", (int)pContact->expires, 0, 0);
            }
            else if (_ExtContactHFArgTable[x].Int == eSIP_CONTACT_HF_Q_ARG) {
                _DEC_StringHlpr(pFSM, pBuff, pContact->q, SIP_Q_VALUE_STR_SIZE);
            }
            else if (_ExtContactHFArgTable[x].Int == eSIP_CONTACT_HF_USER_ARG) {
                _DEC_ContactArgStringHlpr(pFSM, pBuff, pContact->szUser,
                        SIP_USER_STR_SIZE);
            }
            else if (_ExtContactHFArgTable[x].Int ==
                    eSIP_CONTACT_HF_IM_SESSION_ARG) {
                pContact->isImSession = 1;
            }
            else if (_ExtContactHFArgTable[x].Int ==
                    eSIP_CONTACT_HF_IM_CONF_ISFOCUS_ARG) {
                SIP_DebugLog(SIP_DB_DECODE_LVL_3, "IM conference:  detected "
                        "isFocus (line %d)", (int)__LINE__, 0, 0);
                pContact->isFocus = 1;
            }
            else if (_ExtContactHFArgTable[x].Int ==
                    eSIP_CONTACT_HF_PUB_GRUU_ARG) {
                pFSM->pfHandler(pFSM, pBuff, pContact->szPublicGruu);
            }
        }
        /* Loop up capabilityis table */
        else if (SIP_OK == TOKEN_ExtLookup(&pFSM->CurrToken, 
                _ExtCapsArgTable, eSIP_CAPS_TYPE_LAST, &x)) {
            /* It's a capabilities argument, process it */
            _DEC_CapabilitiesHlpr(pFSM, pBuff, _ExtCapsArgTable[x].Int,
                    &pContact->capabilitiesBitmap);
        }
        else {
            /* It's argument we don't recognize. */
            SIP_DebugLog(SIP_DB_DECODE_LVL_3, "Unknown argument :%s\n"
                    ,(int)pFSM->CurrToken.pStart, 0, 0);
            if (SIP_OK != _DEC_UnknownArgHlpr(pFSM, pBuff)) {
                SIP_DebugLog(SIP_DB_DECODE_LVL_3,
                        "Failed to process nnknown argument.\n", 0, 0, 0);
            }
                
        }
        
        /* comma seperate two Contact entries. */
        if (pFSM->isEndOfPacket || ABNF_ISCRLF(pFSM->CurrToken.pDmtr) ||
                (',' == *(pFSM->CurrToken.pDmtr))) {
            break;
        }
    } /* end of while */

    DLLIST_InitEntry(&pContact->dll);
    DLLIST_Enqueue(&pMsg->ContactList, &pContact->dll);
    return (pContact);
}

static vint _DEC_GruuHlpr(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hGruu)
{
    char *pGruu = (char*) hGruu;
    if (TOKEN_Get(pFSM, pBuff, "\" \r\n") == SIP_OK) {
        if (TOKEN_Get(pFSM, pBuff, "\" \r\n") == SIP_OK) {
            TOKEN_copyToBuffer(pGruu, SIP_PUB_GRUU_STR_SIZE, &pFSM->CurrToken);
        }
    }
    return (SIP_OK);
}

static vint _DEC_Replaces(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg)
{
    tSipIntMsg *pMsg = (tSipIntMsg*)hMsg;
    
    /* get the callid */
    if (TOKEN_Get(pFSM, pBuff, " ;\r\n") == SIP_OK) {
        TOKEN_copyToBuffer(pMsg->Replaces.szCallId,
                sizeof(pMsg->Replaces.szCallId), &pFSM->CurrToken);
    }
    return _DEC_ReplacesHlpr(pFSM, pBuff, &pMsg->Replaces);
}

static vint _DEC_ReplacesHlpr(tFSM *pFSM, tL4Packet *pBuff, tReplacesHF *pReplaces)
{
    vint        status;
    uint32         x;
    
    /* We know the uri part of the referto.
     * Now see if there is any replaces info 
     */
    while (_DEC_GetUriParam(pFSM, pBuff) == SIP_OK) {
        status = TOKEN_ExtLookup(&pFSM->CurrToken, 
                _ExtReplaces_HFArgTable, eSIP_LAST_REPLACES_HF_ARG, &x);
        if (status == SIP_OK) { 
            /* found it so launch a handler */
            if (_ExtReplaces_HFArgTable[x].Int == eSIP_EARLY_FLAG_HF_ARG) {
                pReplaces->earlyFlag = TRUE;
            }
            else if (_ExtReplaces_HFArgTable[x].Int == eSIP_TO_TAG_HF_ARG){
                _DEC_StringHlpr(pFSM, pBuff, pReplaces->szToTag, SIP_TAG_HF_STR_SIZE);
            }
            else if (_ExtReplaces_HFArgTable[x].Int == eSIP_FROM_TAG_HF_ARG){
                _DEC_StringHlpr(pFSM, pBuff, pReplaces->szFromTag, SIP_TAG_HF_STR_SIZE);
            }
            else if (_ExtReplaces_HFArgTable[x].Int == eSIP_CALL_ID_HF_ARG) {
                _DEC_StringHlpr(pFSM, pBuff, pReplaces->szCallId, SIP_CALL_ID_HF_STR_SIZE);
            }
        }
        if (pFSM->isEndOfPacket || ABNF_ISCRLF(pFSM->CurrToken.pDmtr)) break;
    } /* end of while */
    
    SIP_DebugLog(SIP_DB_DECODE_LVL_3, "_DEC_ReplacesHlpr: early flag:%d to:%s from:%s", 
                 (int)pReplaces->earlyFlag, (int)pReplaces->szToTag, (int)pReplaces->szFromTag);
    return (SIP_OK);
}
static vint _DEC_ReferToHlpr(tFSM *pFSM, tL4Packet *pBuff, tReplacesHF *pReplaces)
{
    vint        status;
    uint32      x;
    
    /* We know the uri part of the referto.
     * Now see if there is any replaces info 
     */
    while (_DEC_GetReferToParam(pFSM, pBuff) == SIP_OK) {
        status = TOKEN_ExtLookup(&pFSM->CurrToken, 
                _ExtReplaces_HFArgTable, eSIP_LAST_REPLACES_HF_ARG, &x);
        if (status == SIP_OK) { 
            /* found it so launch a handler */
            if (_ExtReplaces_HFArgTable[x].Int == eSIP_EARLY_FLAG_HF_ARG) {
                pReplaces->earlyFlag = TRUE;
            }
            else if (_ExtReplaces_HFArgTable[x].Int == eSIP_TO_TAG_HF_ARG){
                _DEC_ReferToStringHlpr(pFSM, pBuff, pReplaces->szToTag, SIP_TAG_HF_STR_SIZE);
                SIP_DebugLog(SIP_DB_DECODE_LVL_3, "_DEC_ReferToHlpr: to-tag:%s", 
                             (int)pReplaces->szToTag, 0, 0);
            }
            else if (_ExtReplaces_HFArgTable[x].Int == eSIP_FROM_TAG_HF_ARG){
                _DEC_ReferToStringHlpr(pFSM, pBuff, pReplaces->szFromTag, SIP_TAG_HF_STR_SIZE);
                SIP_DebugLog(SIP_DB_DECODE_LVL_3, "_DEC_ReferToHlpr: from-tag:%s", 
                             (int)pReplaces->szFromTag, 0, 0);
            }
            else if (_ExtReplaces_HFArgTable[x].Int == eSIP_CALL_ID_HF_ARG) {
                _DEC_ReferToStringHlpr(pFSM, pBuff, pReplaces->szCallId, SIP_CALL_ID_HF_STR_SIZE);
                SIP_DebugLog(SIP_DB_DECODE_LVL_3, "_DEC_ReferToHlpr: call-id:%s", 
                             (int)pReplaces->szCallId, 0, 0);
            }
        }
        if (pFSM->isEndOfPacket || ABNF_ISCRLF(pFSM->CurrToken.pDmtr)) break;
    } /* end of while */
    
    SIP_DebugLog(SIP_DB_DECODE_LVL_3, "_DEC_ReferToHlpr: early flag:%d to:%s from:%s", 
                 (int)pReplaces->earlyFlag, (int)pReplaces->szToTag, (int)pReplaces->szFromTag);
    return (SIP_OK);
}
static vint _DEC_GetUriParam(
    tFSM      *pFSM, 
    tL4Packet *pBuff)
{
    uint16 numLines = 0;
    SIP_DebugLog(SIP_DB_DECODE_LVL_3, "_DEC_GetUriParam() \n", 0, 0, 0);

    while (TOKEN_Get(pFSM, pBuff, ",; =\r\n") == SIP_OK) {

        SIP_DebugLog(SIP_DB_DECODE_LVL_3,
                    "_DEC_GetUriParam: found a hf param with length =  %d\n",
                    pFSM->CurrToken.length, 0, 0);
                    
        if (pFSM->isEndOfPacket || ABNF_ISCRLF(pFSM->CurrToken.pDmtr)) {
            /* we may have a token, but there must not be an argument to it */
            SIP_DebugLog(SIP_DB_DECODE_LVL_3,
                    "_DEC_GetUriParam: found a hf param with no arg %s\n",
                    (int)pFSM->CurrToken.pStart, 0, 0);
            return (SIP_FAILED);
        }
        if (pFSM->CurrToken.length == 0) {
            if (',' == *(pFSM->CurrToken.pDmtr)) {
                /* Must be a seperator between two entries. */
                return (SIP_FAILED);
            }
            continue;
        }
        if (*(pFSM->CurrToken.pDmtr) == ' ') {
            /* the only way to tell if the end of the line has been
             * reached with the "SkipUntilEsc" function is to 
             * check the lines counter in pFSM->CurrLine.
             */
            numLines = pFSM->CurrLine;
            /* then there is lws to an equal sign */
            SIP_DebugLog(SIP_DB_DECODE_LVL_3,
                    "_DEC_GetUriParam: removing LWS before an equals sign", 0, 0, 0);
            if (TOKEN_SkipUntilEsc(pFSM, pBuff, "=;\r\n") != SIP_OK) {
                return (SIP_FAILED);
            }
            if (numLines != pFSM->CurrLine) {
                return (SIP_FAILED);
            }
        }
        
        SIP_DebugLog(SIP_DB_DECODE_LVL_3,
                "_DEC_GetUriParam: found a hf param %s",
                (int)pFSM->CurrToken.pStart, 0, 0);
        return (SIP_OK);
    }
    return (SIP_FAILED);
}

/* ======== _DEC_GetContactParam() ========
 * Private function to get next parameter in the Contact header field.
 *
 * Return:
 *  SIP_OK: Got a parameter.
 *  SIP_FAIL: No parameter gotten.
 */
static vint _DEC_GetContactParam(
    tFSM      *pFSM, 
    tL4Packet *pBuff)
{
    uint16 numLines = 0;
    SIP_DebugLog(SIP_DB_DECODE_LVL_3, "_DEC_GetContactParam() \n", 0, 0, 0);

    while (TOKEN_Get(pFSM, pBuff, ",; =\r\n") == SIP_OK) {

        if (pFSM->isEndOfPacket || ABNF_ISCRLF(pFSM->CurrToken.pDmtr)) {
            /* we may have a token, but there must not be an argument to it */
            SIP_DebugLog(SIP_DB_DECODE_LVL_3,
                    "_DEC_GetContactParam: found a hf param with no arg %s\n",
                    (int)pFSM->CurrToken.pStart, 0, 0);
            return (SIP_OK);
        }
        if (pFSM->CurrToken.length == 0) {
            if (',' == *(pFSM->CurrToken.pDmtr)) {
                /* Must be a seperator between two Contact */
                return (SIP_FAILED);
            }
            continue;
        }
        if (*(pFSM->CurrToken.pDmtr) == ' ') {
            /* the only way to tell if the end of the line has been
             * reached with the "SkipUntilEsc" function is to 
             * check the lines counter in pFSM->CurrLine.
             */
            numLines = pFSM->CurrLine;
            /* then there is lws to an equal sign */
            SIP_DebugLog(SIP_DB_DECODE_LVL_3,
                    "_DEC_GetContactParam: removing LWS before an equals sign", 0, 0, 0);
            if (TOKEN_SkipUntilEsc(pFSM, pBuff, "=;\r\n") != SIP_OK) {
                return (SIP_FAILED);
            }
            if (numLines != pFSM->CurrLine) {
                return (SIP_FAILED);
            }
        }
        
        SIP_DebugLog(SIP_DB_DECODE_LVL_3,
                "_DEC_GetContactParam: found a hf param %s",
                (int)pFSM->CurrToken.pStart, 0, 0);
        return (SIP_OK);
    }
    return (SIP_FAILED);
}

static vint _DEC_GetReferToParam(
    tFSM      *pFSM, 
    tL4Packet *pBuff)
{
    char a;
    while (TOKEN_Get(pFSM, pBuff, "%=\r\n") == SIP_OK) {
        if (pFSM->isEndOfPacket || ABNF_ISCRLF(pFSM->CurrToken.pDmtr)) {
            /* we may have a token, but there must not be an argument to it */
            return (SIP_FAILED);
        }
        if (pFSM->CurrToken.length == 0) {
            continue;
        }
        a = _DEC_Hex2Abnf(pFSM, pBuff);
        if (a == ' ') {
            continue;
        }
        SIP_DebugLog(SIP_DB_DECODE_LVL_3, "_DEC_GetReferToParam: found a param", 0, 0, 0);
        return (SIP_OK);
    }
    return (SIP_FAILED);
}

static vint _DEC_UriUsername(
    char   *pName, 
    vint    namelen, 
    char   *pTarget,
    vint    maxTargetLen)
{
    /* decode the username portion of a 
     * URI accounting for escape sequences
     * (a.k.a. 'special chars')
     */
    vint x;
    vint y;
    char a;
    for (x = 0, y = 0 ; (x < namelen) && (y < (maxTargetLen - 1)) ; y++) {
        a = 0;
        if (ABNF_ISEQUAL(pName)) {
            a = '=';
        }
        else if (ABNF_ISSEMICOLON(pName)) {
            a = ';';
        }
        else if (ABNF_ISATSIGN(pName)) {
            a = '@';
        }
        else if (ABNF_ISSPACE(pName)) {
            a = ' ';
        }
#ifndef SIP_D2_MOD        
        else if (ABNF_ISPOUND(pName)) {
            a = '#';
        }
#endif        
        if (a != 0) {
            pTarget[y] = a;
            pName += 3;
            x += 3;
        }
        else {
            pTarget[y] = *pName;
            pName++;
            x++;
        }
    }
    /* NULL out the pTarget */
    pTarget[y] = 0;
    /* return the length */
    return y;
}

static void _DEC_UriDecodeArgs(
    tFSM          *pFSM,
    tL4Packet     *pBuff,
    uint16         internal,
    tUri          *pUri,
    tpfTokenHndlr  pfHandler) 
{
    switch (internal) {
        case eSIP_LR_URI_PARM:
            pUri->lr = TRUE;
            break;
        case eSIP_TTL_URI_PARM:
            pfHandler(pFSM, pBuff, &pUri->ttl);
            break;
        case eSIP_METHOD_URI_PARM:
            pfHandler(pFSM, pBuff, &pUri->method);
            break;
        case eSIP_MADDR_URI_PARM:
            pfHandler(pFSM, pBuff, &pUri->maddr);
            break;
        case eSIP_TRANSPORT_URI_PARM:
            pfHandler(pFSM, pBuff, &pUri->transport);
            break;
        case eSIP_USER_URI_PARM: 
            pfHandler(pFSM, pBuff, pUri->szUserParam);
            break;
        case eSIP_PHONE_CXT_URI_PARM:
            pfHandler(pFSM, pBuff, pUri->szPhoneCxtParam);
            break;
        case eSIP_SESSION_URI_PARM:
            pfHandler(pFSM, pBuff, pUri->szSessionParam);
            break;
        case eSIP_PSBR_URI_PARM:
            pUri->argType = (eUriArgType)eSIP_PSBR_URI_PARM;
            pfHandler(pFSM, pBuff, pUri->arg.szPsbr);
            break;
        case eSIP_LBFH_URI_PARM: 
            pUri->argType = (eUriArgType)eSIP_LBFH_URI_PARM;
            pfHandler(pFSM, pBuff, pUri->arg.szLbfh);
            break;
        case eSIP_CONF_URI_PARM: 
            pfHandler(pFSM, pBuff, pUri->szConf);
            break;
        case eSIP_FTAG_URI_PARM: 
            pfHandler(pFSM, pBuff, pUri->szFtag);
            break;
        case eSIP_GR_URI_PARM: 
            pfHandler(pFSM, pBuff, pUri->szGruu);
            break;
        case eSIP_LSKPMC_URI_PARM:
            pfHandler(pFSM, pBuff, pUri->szLskpmc);
            break;
        default:
            break;
    } /* End of switch */
    return;        
}

static vint _DEC_UriDecode(
    vint    withScheme,
    char   *str,
    uint16  size,
    tUri   *pUri)
{
#define URI_DECODE_STATE_START              (0)
#define URI_DECODE_STATE_START_SCHEME       (1)
#define URI_DECODE_STATE_START_SCHEME_TEL   (2)
#define URI_DECODE_STATE_GET_PORT           (3)
#define URI_DECODE_STATE_GET_PARAMS         (4)
#define URI_DECODE_STATE_GET_IPV6_ADDR      (5)
#define URI_DECODE_STATE_DONE               (6)
#define URI_DECODE_STATE_START_SCHEME_URN   (7)
#define URI_DECODE_STATE_GET_USS            (8)

    tL4Packet bi;
    tFSM      fsm;
    uint32    x;
    char     *pEnd;
    char     *pUser;
    char     *pUid;
    char     *pUss;
    vint      userLen;
    char      match[8];
    vint      state;
    char      delimiter;

    bi.frame = 0;
    bi.pCurr = bi.pStart = str;
    bi.length = size;
    bi.isOutOfRoom = FALSE;

    OSAL_memSet(&fsm, 0, sizeof(tFSM));

    pUser = pUri->user;
    pUid = pUri->uid;
    pUss = pUri->uss;
    userLen = SIP_URI_USER_STR_SIZE;

    SIP_DebugLog(SIP_DB_DECODE_LVL_3, "_DEC_UriDecode: Start", 0, 0, 0);

    OSAL_snprintf(match, 8, "%s", " :");
    state = URI_DECODE_STATE_START;
    if (0 == withScheme) {
        OSAL_snprintf(match, 8, "%s", " :@;[");
        state = URI_DECODE_STATE_START_SCHEME;
    }

    while (TOKEN_Get(&fsm, &bi, match) == SIP_OK) {
        
        if (fsm.CurrToken.length == 0 && ('[' != *(fsm.CurrToken.pDmtr)) &&
                (URI_DECODE_STATE_GET_IPV6_ADDR != state)) {
            continue;
        }

        delimiter = '\0';
        if (!fsm.isEndOfPacket) {
            delimiter = *(fsm.CurrToken.pDmtr);
        }

        switch (state) {
            case URI_DECODE_STATE_START:
                switch (delimiter) {
                    case '\0':
                    case ':': 
                        pUri->scheme = eURI_SCHEME_DUMMY;
                        if (SIP_OK == TOKEN_ExtLookup(&fsm.CurrToken, _ExtUriParmTable, eURI_SCHEME_LAST, &x)) {
                            pUri->scheme = (tSipUriScheme)_ExtUriParmTable[x].Int;
                            SIP_DebugLog(SIP_DB_DECODE_LVL_3, "DEC_UriDecode: Setting scheme to :%s",
                                (int)_ExtUriParmTable[x].pExt, 0, 0);
                        }
                        if (eURI_SCHEME_TEL == pUri->scheme) {
                            state = URI_DECODE_STATE_START_SCHEME_TEL;
                            OSAL_snprintf(match, 8, "%s", " :;");
                        }
                        else if (eURI_SCHEME_URN == pUri->scheme) {
                            state = URI_DECODE_STATE_START_SCHEME_URN;
                            OSAL_snprintf(match, 8, "%s", " :");
                        }
                        else {
                            state = URI_DECODE_STATE_START_SCHEME;
                            OSAL_snprintf(match, 8, "%s", " :@;[");
                        }
                        break;
                    default:
                        break;
                }
                break;
            case URI_DECODE_STATE_START_SCHEME:
                switch (delimiter) {
                    case '[':
                        /* must be ipv6 address */
                        OSAL_snprintf(match, 8, "%s", "]");
                        state = URI_DECODE_STATE_GET_IPV6_ADDR;
                        break; 
                    case '\0':
                    case ':':
                        /* must be the domain */
                        VoIP_IpAddrExt2Int(fsm.CurrToken.pStart,
                                fsm.CurrToken.length, 0, &pUri->host);
                        OSAL_snprintf(match, 8, "%s", ";=");
                        state = URI_DECODE_STATE_GET_PORT;
                        break;
                    case '@':
                        /* set the name */
                        if (userLen != SIP_URI_USER_STR_SIZE) {
                            /* then we now there are multiple '@' signs in the user name */
                            *pUser = '@';
                            pUser++;
                            userLen--;
                        }
                        x = _DEC_UriUsername(fsm.CurrToken.pStart,
                                fsm.CurrToken.length, pUser, userLen);
                        pUser += x;
                        userLen -= x;
                        break;
                    case ';':
                        /* must be the domain */
                        VoIP_IpAddrExt2Int(fsm.CurrToken.pStart,
                                fsm.CurrToken.length, 0, &pUri->host);
                        OSAL_snprintf(match, 8, "%s", ";=");
                        state = URI_DECODE_STATE_GET_PARAMS;
                        break;
                    default:
                        break;
                }
                break;
            case URI_DECODE_STATE_START_SCHEME_TEL:
                /* must be the username */
                switch (delimiter) {
                    case '\0':
                    case ':':
                        /* must be the username */
                        _DEC_UriUsername(fsm.CurrToken.pStart,
                                fsm.CurrToken.length, pUser, userLen);
                        OSAL_snprintf(match, 8, "%s", ";=");
                        state = URI_DECODE_STATE_GET_PORT;
                        break;
                    case ';':
                        /* must be the username */
                        _DEC_UriUsername(fsm.CurrToken.pStart,
                                fsm.CurrToken.length, pUser, userLen);
                        OSAL_snprintf(match, 8, "%s", ";=");
                        state = URI_DECODE_STATE_GET_PARAMS;
                        break;
                    default:
                        break;
                }
                break;
            case URI_DECODE_STATE_START_SCHEME_URN:
                switch (delimiter) {
                    case '\0':
                    case ':':
                        /* must be the namespace id */
                        _DEC_UriUsername(fsm.CurrToken.pStart,
                                fsm.CurrToken.length, pUid,
                                SIP_URI_UID_STR_SIZE);
                        OSAL_snprintf(match, 8, "%s", ":");
                        state = URI_DECODE_STATE_GET_USS;
                        break;
                    default:
                        break;
                }
                break;
            case URI_DECODE_STATE_GET_USS:
                switch (delimiter) {
                    case '\0':
                    case ':':
                        /* must be the namespace specific string */
                        _DEC_UriUsername(fsm.CurrToken.pStart,
                                fsm.CurrToken.length, pUss,
                                SIP_URI_USS_STR_SIZE);
                        OSAL_snprintf(match, 8, "%s", ";=");
                        state = URI_DECODE_STATE_GET_PARAMS;
                        break;
                    default:
                        break;
                }
                break;
            case URI_DECODE_STATE_GET_IPV6_ADDR:
                switch (delimiter) {
                    case '\0':
                    case ']':
                        /* ipv6 address */
                        VoIP_IpAddrExt2Int(fsm.CurrToken.pStart,
                                fsm.CurrToken.length, 0, &pUri->host);
                        OSAL_snprintf(match, 8, "%s", ":;");
                        /* keep in the same state */
                        break;
                    case ':':
                        /* next is port */
                        OSAL_snprintf(match, 8, "%s", ";=");
                        state = URI_DECODE_STATE_GET_PORT;
                        break;
                    case ';':
                        /* next is param */
                        OSAL_snprintf(match, 8, "%s", ";=");
                        state = URI_DECODE_STATE_GET_PARAMS;
                        break;
                }
                break;
            case URI_DECODE_STATE_GET_PORT:
                pUri->host.port = (uint16)OSAL_strtoul(
                        fsm.CurrToken.pStart, &pEnd, 10);
                OSAL_snprintf(match, 8, "%s", ";=");
                state = URI_DECODE_STATE_GET_PARAMS;
                break;
            case URI_DECODE_STATE_GET_PARAMS:
                switch (delimiter) {
                    case ';':
                    case '=':
                    case '\0':
                        if (TOKEN_ExtLookup(&fsm.CurrToken, _ExtURI_ParmTable,
                                eSIP_LAST_URI_PARM, &x) == SIP_OK) {
                            _DEC_UriDecodeArgs(&fsm, &bi,
                                    _ExtURI_ParmTable[x].Int, pUri,
                                    _ExtURI_ParmTable[x].pfHandler);
                        }
                        break;
                    default:
                        break;
                }
                break;
            default:
                break;
        }
    } /* end of while */
    SIP_DebugLog(SIP_DB_DECODE_LVL_3, "_DEC_UriDecode: Finished", 0, 0, 0);
    return (SIP_OK);
}

static vint _DEC_DecodeUriPlus(tFSM *pFSM, tL4Packet *pBuff, tUriPlus *pUriPlus)
{
    int   numElements;
    tDecodeArgs args[MAX_URI_ARGS];
    uint32 size;
           
    numElements = _DEC_NameAndUriHlpr(pFSM, pBuff, args);

    if (numElements == 0) return (SIP_OK); /* we have nothing */
    
    if (numElements == 1)
        _DEC_UriDecode(TRUE, args[0].s, args[0].l, &pUriPlus->uri);
    else {
        size = CALC_MIN(args[0].l, (SIP_DISPLAY_NAME_STR_SIZE - 1));
        OSAL_memCpy(pUriPlus->szDisplayName, args[0].s, size);
        pUriPlus->szDisplayName[size] = 0;
        _DEC_UriDecode(TRUE, args[1].s, args[1].l, &pUriPlus->uri);
    }

    if (pFSM->isEndOfPacket || ABNF_ISCRLF(pFSM->CurrToken.pDmtr)) return (SIP_OK);

    /* now check for parms like tag and user etc. */
    while (_DEC_GetUriParam(pFSM, pBuff) == SIP_OK) {
        if (TOKEN_iCmpToken(&pFSM->CurrToken, SIP_TAG_HF_ARG_STR) == TRUE) {
            _DEC_StringHlpr(pFSM, pBuff, pUriPlus->szTag, SIP_TAG_HF_STR_SIZE);
        }
        else if (TOKEN_iCmpToken(&pFSM->CurrToken, SIP_USER_HF_ARG_STR) == TRUE) {
            _DEC_StringHlpr(pFSM, pBuff, pUriPlus->szUser, SIP_USER_STR_SIZE);
        }
        if (pFSM->isEndOfPacket || ABNF_ISCRLF(pFSM->CurrToken.pDmtr)) break;
    } /* end of while */
    return (SIP_OK);
}


/*============================== _DEC_SdpInfo ==================================*/
/*
 * DESCRIPTION: SDP announcement decoder wrapper. It calls to SDP decoder entry point
 *              that defined in sdpcv object.
 *              The syntax(ABNF) definition see in RFC2327.
 *----------------------------------------------------------------------------------*/
static vint _DEC_SdpInfo(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg)
{
    tSipIntMsg   *pMsg = (tSipIntMsg *)hMsg;
        
    if (NULL == (pMsg->pSessDescr = SDP_AllocMsg())) {
        SIP_DebugLog(SIP_DB_DECODE_LVL_1, "_DEC_SdpInfo: Failed to allocate memory hMsg:%X", 
                (int)hMsg, 0, 0);
        return SIP_NO_MEM;
    }
    
    if (SDPDEC_Exec(pFSM, pBuff, pMsg->pSessDescr) != SIP_OK) {
        SIP_DebugLog(SIP_DB_DECODE_LVL_1, "_DEC_SdpInfo: SDPDEC_Exec Failed:%X", (int)hMsg, 0, 0);
        SDP_DeallocMsg(pMsg->pSessDescr);
        pMsg->pSessDescr = NULL;
        SIP_MSG_CODE(pMsg, eSIP_RSP_NOT_ACCEPT);
        TOKEN_SkipToEnd(pBuff);
        /* we return OK here and set the status to SIP_OK because we want to keep 
         * processing the request even though we don't know what the payload is.  
         * We let the UA decide what he wants to do 
         */
        pFSM->Status = SIP_OK;
        return (SIP_OK);
    }
    else {
        TOKEN_SkipToEnd(pBuff);
        return SIP_OK;
    }
}

static vint _DEC_DomainArg(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg)
{
    uint32 len;
    tAuthorizationHFE *pAuth = (tAuthorizationHFE*) hMsg;
    /* 
     * Domain is unsupported right now, 
     * nobody seems to use it just copy the string 
     */
    len = MAX_DOMAIN_NAME_LEN;
    return _DEC_StringArgHlpr(pFSM, pBuff, pAuth->domain, &len);
}
static vint _DEC_UsernameArg(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg)
{
    uint32 len;
    tAuthorizationHFE *pAuth = (tAuthorizationHFE*) hMsg;
    len = SIP_USERNAME_ARG_STR_SIZE;
    return _DEC_StringArgHlpr(pFSM, pBuff, pAuth->szUsername, &len);
}
static vint _DEC_RealmArg(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg)
{
    uint32 len;
    tAuthorizationHFE *pAuth = (tAuthorizationHFE*) hMsg;
    len = SIP_REALM_ARG_STR_SIZE;
    return _DEC_StringArgHlpr(pFSM, pBuff, pAuth->szRealm, &len);
}

static vint _DEC_NonceArg(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg)
{
    uint32 len;
    tAuthorizationHFE *pAuth = (tAuthorizationHFE*) hMsg;
    len = SIP_NONCE_ARG_STR_SIZE;
    _DEC_StringArgHlpr(pFSM, pBuff, pAuth->szNonce, &len);
    /* Alsways return OK for empty string */
    return (SIP_OK);
}

static vint _DEC_CnonceArg(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg)
{
    uint32 len;
    tAuthorizationHFE *pAuth = (tAuthorizationHFE*) hMsg;
    len = SIP_CNONCE_ARG_STR_SIZE;
    return _DEC_StringArgHlpr(pFSM, pBuff, pAuth->szCNonce, &len);
}
static vint _DEC_OpaqueArg(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg)
{
    uint32 len;
    tAuthorizationHFE *pAuth = (tAuthorizationHFE*) hMsg;
    len = SIP_OPAQUE_ARG_STR_SIZE;
    return _DEC_StringArgHlpr(pFSM, pBuff, pAuth->szOpaque, &len);
}
static vint _DEC_NcArg(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg)
{
    uint32 len;
    tAuthorizationHFE *pAuth = (tAuthorizationHFE*) hMsg;
    len = SIP_NONCE_CNT_ARG_STR_SIZE;
    return _DEC_StringArgHlpr(pFSM, pBuff, pAuth->szNC, &len);
}
static vint _DEC_StaleArg(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg)
{
    tAuthorizationHFE *pAuth = (tAuthorizationHFE*) hMsg;
    if (TOKEN_Get(pFSM, pBuff, " ,\r\n") == SIP_OK) {
        if (TOKEN_iCmpToken(&pFSM->CurrToken, SIP_TRUE_STR) == TRUE) {
            pAuth->stale = TRUE;
        }
        else {
            pAuth->stale = FALSE;
        }
        return (SIP_OK);
    }
    return (SIP_FAILED);
}
static vint _DEC_AlgArg(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg)
{
    tAuthorizationHFE *pAuth = (tAuthorizationHFE*) hMsg;
    if (TOKEN_Get(pFSM, pBuff, " ,\r\n") == SIP_OK) {
        if (TOKEN_iCmpToken(&pFSM->CurrToken, SIP_AUTH_ALG_AKAV1_MD5_STR) == TRUE) {
            pAuth->alg = eAUTH_ALG_AKAV1_MD5;
        }
        else if (TOKEN_iCmpToken(&pFSM->CurrToken, SIP_AUTH_ALG_AKAV2_MD5_STR) == TRUE) {
            pAuth->alg = eAUTH_ALG_AKAV2_MD5;
        }
        else if (TOKEN_iCmpToken(&pFSM->CurrToken, SIP_AUTH_ALG_MD5_STR) == TRUE) {
            pAuth->alg = eAUTH_ALG_MD5;
        }        
        else {
            pAuth->alg = eAUTH_ALG_NONE;
        }
        return (SIP_OK);
    }
    return (SIP_FAILED);
}

static vint _DEC_ResponseArg(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg)
{
    uint32 len;
    tAuthorizationHFE *pAuth = (tAuthorizationHFE*) hMsg;
    len = SIP_RESPONSE_ARG_STR_SIZE;
    _DEC_StringArgHlpr(pFSM, pBuff, pAuth->szNonce, &len);
    /* Alsways return OK for empty string */
    return (SIP_OK);
}

static vint _DEC_AutsArg(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg)
{
    uint32 len;
    tAuthorizationHFE *pAuth = (tAuthorizationHFE*) hMsg;
    len = SIP_AUTS_ARG_STR_SIZE;
    _DEC_StringArgHlpr(pFSM, pBuff, pAuth->szAuts, &len);
    /* Alsways return OK for empty string */
    return (SIP_OK);
}

static vint _DEC_UriArg(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg)
{
    uint32 len;
    char buff[SIP_URI_ARG_STR_SIZE];
    tAuthorizationHFE *pAuth = (tAuthorizationHFE*) hMsg;
    
    len = SIP_URI_ARG_STR_SIZE;
    if (_DEC_StringArgHlpr(pFSM, pBuff, buff, &len) == SIP_OK) {
        return _DEC_UriDecode(TRUE, buff, (uint16)len, &pAuth->uri);
    }
    return (SIP_FAILED);
}
static vint _DEC_QopArg(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg)
{
    tAuthorizationHFE *pAuth = (tAuthorizationHFE*) hMsg;
    if (TOKEN_Get(pFSM, pBuff, "\",\r\n") == SIP_OK) {
        if (pFSM->CurrToken.pDmtr && *pFSM->CurrToken.pDmtr == '"') {
            /*
             * ZK: If a quote was present, find the closing quote.
             */
            if (TOKEN_Get(pFSM, pBuff, "\",\r\n") != TRUE) {
                return (SIP_FAILED);
            }
        }
        if (TOKEN_iCmpToken(&pFSM->CurrToken, SIP_QOP_AUTH_STR) == TRUE) 
            pAuth->qop = eSIP_QOP_AUTH;
        else if (TOKEN_iCmpToken(&pFSM->CurrToken, SIP_QOP_AUTH_INT_STR) == TRUE) 
            pAuth->qop = eSIP_QOP_AUTH_INT;
        else
            pAuth->qop = eSIP_QOP_NONE;

        return (SIP_OK);
    }
    return (SIP_FAILED);
}
static vint _DEC_StringArgHlpr(
    tFSM      *pFSM, 
    tL4Packet *pBuff, 
    char      *pTarget, 
    uint32    *targetSize)
{
    uint32 len = 0;
    uint32 maxSize = *targetSize;
    if (TOKEN_Get(pFSM, pBuff, "\",\r\n") == SIP_OK) {
        if (pFSM->CurrToken.pDmtr && *pFSM->CurrToken.pDmtr == '"') {
            /* then try to get a second qoutation mark */
            if (TOKEN_Get(pFSM, pBuff, "\",\r\n") != SIP_OK) {
                *targetSize = len;
                return (SIP_FAILED);
            }
        }
        if (pFSM->CurrToken.length > 0) {
            *targetSize = TOKEN_copyToBuffer(pTarget, maxSize, &pFSM->CurrToken);
            return (SIP_OK);
        }
    }
    *targetSize = len;
    return (SIP_FAILED);
}
static vint _DEC_StringHlpr (
    tFSM         *pFSM, 
    tL4Packet    *pBuff, 
    char         *pTargetStr,
    uint32        maxSize)
{
    if (TOKEN_Get(pFSM, pBuff, "; \r\n") == SIP_OK) {
        TOKEN_copyToBuffer(pTargetStr, maxSize, &pFSM->CurrToken);
    }
    return (SIP_OK);
}

static vint _DEC_ReferToStringHlpr (
    tFSM         *pFSM, 
    tL4Packet    *pBuff, 
    char         *pTargetStr,
    uint32        maxSize)
{
    char a;
    int len;
_DEC_ReferToStringHlpr_label:
    if (TOKEN_Get(pFSM, pBuff, "%>\r\n") == SIP_OK) {
        a = _DEC_Hex2Abnf(pFSM, pBuff);
        if (a == ' ') {
            goto _DEC_ReferToStringHlpr_label;
        }
        else if (a == '@' || a == '.') {
            len = TOKEN_copyToBuffer(pTargetStr, maxSize, &pFSM->CurrToken);
            pTargetStr += len;
            *pTargetStr = a;
            pTargetStr++;
            *pTargetStr = 0;
            maxSize -= len;
            goto _DEC_ReferToStringHlpr_label;
        }
        else {
            /* then just copy what we have */
            TOKEN_copyToBuffer(pTargetStr, maxSize, &pFSM->CurrToken);
        }
    }
    return (SIP_OK);
}
static vint _DEC_MethodHlpr (
    tFSM       *pFSM,
    tL4Packet  *pBuff,
    tSipHandle hMethod)
{
    uint32 x;
    tSipMethod *pMethod = (tSipMethod*)hMethod;
    if (TOKEN_Get(pFSM, pBuff, "; \r\n") == SIP_OK) {
        if (TOKEN_ExtLookup(&pFSM->CurrToken, _ExtMethodTable, eSIP_LAST_METHOD, &x) == SIP_OK) {
            *pMethod = (tSipMethod)_ExtMethodTable[x].Int;
        }
    }
    return (SIP_OK);
}
static vint _DEC_IPAddrHlpr (
    tFSM       *pFSM, 
    tL4Packet  *pBuff, 
    tSipHandle hAddr)
{
    tIPAddr  *pAddr = (tIPAddr*)hAddr;
    if (TOKEN_Get(pFSM, pBuff, "; \r\n") == SIP_OK) {
        if (pFSM->CurrToken.length > 0) {
            VoIP_IpV4Ext2Int(pFSM->CurrToken.pStart, pFSM->CurrToken.length, pAddr);
        }
    }
    return (SIP_OK);
}
static vint  _DEC_UintHlpr (
    tFSM       *pFSM, 
    tL4Packet  *pBuff, 
    tSipHandle  hNum)
{
    uint32 *pNum = (uint32*)hNum;
    char *pEnd;
    if (TOKEN_Get(pFSM, pBuff, ",; \r\n") == SIP_OK) {
        if (pFSM->CurrToken.length > 0) {
            *pNum = OSAL_strtoul(pFSM->CurrToken.pStart, &pEnd, 10);
        }
    }
    return (SIP_OK);
}

static vint  _DEC_Rport (
    tFSM       *pFSM, 
    tL4Packet  *pBuff, 
    tSipHandle  hNum)
{
    uint32 *pNum = (uint32*)hNum;
    char *pEnd;
    uint16 num;

    /* For the 'rport', there may not be a value so we
     * simply mark it as present.  Using a value of '1'
     * is okay since that can never be a valid IP Port 
     * in a SIP packet
     */
    *pNum = 1;
    if (!(pFSM->isEndOfPacket) && !(*pFSM->CurrToken.pDmtr == ';') ) {
        if (TOKEN_Get(pFSM, pBuff, "; \r\n") == SIP_OK) {
            if (pFSM->CurrToken.length > 0) {
                num = (uint16)OSAL_strtoul(pFSM->CurrToken.pStart, &pEnd, 10);
                SIP_DebugLog(SIP_DB_DECODE_LVL_3, "_DEC_Rport: setting rport to :%d", (int)num, 0, 0);
                *pNum = num;
            }
        }
    }
    return (SIP_OK);
}

static vint  _DEC_KeepAlive (
    tFSM       *pFSM, 
    tL4Packet  *pBuff, 
    tSipHandle  hNum)
{
    uint32 *pNum = (uint32*)hNum;
    char *pEnd;
    uint16 num;

    *pNum = 0;
    if (!(pFSM->isEndOfPacket) && !(*pFSM->CurrToken.pDmtr == ';') ) {
        if (TOKEN_Get(pFSM, pBuff, "; \r\n") == SIP_OK) {
            if (pFSM->CurrToken.length > 0) {
                num = (uint16)OSAL_strtoul(pFSM->CurrToken.pStart, &pEnd, 10);
                SIP_DebugLog(SIP_DB_DECODE_LVL_3,
                        "_DEC_KeepAlive: setting keep-alives to :%d",
                        (int)num, 0, 0);
                *pNum = num;
            }
        }
    }
    return (SIP_OK);
}

static vint  _DEC_TransportHlpr (
    tFSM       *pFSM, 
    tL4Packet  *pBuff, 
    tSipHandle  hType)
{
    uint32 x;
    tTransportType *pType = (tTransportType*)hType;
    if (TOKEN_Get(pFSM, pBuff, "; \r\n") == SIP_OK) {
        uint32 tableSize = sizeof(_ExtTransportTypeTable)/sizeof(_ExtTransportTypeTable[0]);
        if(TOKEN_ExtLookup(&pFSM->CurrToken, _ExtTransportTypeTable, tableSize, &x) == SIP_OK) {
            *pType = (tTransportType)_ExtTransportTypeTable[x].Int;
        }
        else {
            *pType = eTransportNone;
        }
    }
    return (SIP_OK);
}
static vint _DEC_UserHlpr(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hStr)
{
    char* pUser = (char*) hStr;
    return _DEC_StringHlpr(pFSM, pBuff, pUser, SIP_USER_STR_SIZE);
}
static vint _DEC_LskpmcHlpr(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hStr)
{
    char* pLskpmc = (char*) hStr;
    return _DEC_StringHlpr(pFSM, pBuff, pLskpmc, SIP_LSKPMC_STR_SIZE);
}

static vint _DEC_ArgHlpr(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hStr)
{
    char* pStr = (char*) hStr;
    return _DEC_StringHlpr(pFSM, pBuff, pStr, SIP_URI_ARG_STR_SIZE);
}
static char _DEC_Hex2Abnf(tFSM *pFSM, tL4Packet *pBuff)
{
    char *pDmtr;
    if (!pFSM->CurrToken.pDmtr) {
        return 0;
    }
    else {
        pDmtr = pFSM->CurrToken.pDmtr;
    }

    if (ABNF_ISEQUAL(pDmtr)) {
        pBuff->pCurr += 2;
        return '=';
    }
    else if (ABNF_ISSEMICOLON(pDmtr)) {
        pBuff->pCurr += 2;
        return ';';
    }
    else if (ABNF_ISATSIGN(pDmtr)) {
        pBuff->pCurr += 2;
        return '@';
    }
    else if (ABNF_ISPERIOD(pDmtr)) {
        pBuff->pCurr += 2;
        return '.';
    }
    else if (ABNF_ISPOUND(pDmtr)) {
        pBuff->pCurr += 2;
        return '#';
    }
    else if (ABNF_ISSPACE(pDmtr)) {
        pBuff->pCurr += 2;
        return ' ';
    }
    return 0;
}

/* all generic strings type header feilds that are stored in pMSg->pHFlist */
static vint _DEC_Accept(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg)
{
    return _DEC_HFhelper(pFSM, pBuff, hMsg, eSIP_ACCEPT_HF, SIP_ACCEPT_HF_STR_SIZE, NULL);
}
static vint _DEC_AcceptEncoding(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg)
{
    return _DEC_HFhelper(pFSM, pBuff, hMsg, eSIP_ACCEPT_ENCODING_HF, SIP_ACCEPT_ENCODING_HF_STR_SIZE, NULL);
}
static vint _DEC_AcceptLanguage(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg)
{
    return _DEC_HFhelper(pFSM, pBuff, hMsg, eSIP_ACCEPT_LANGUAGE_HF, SIP_ACCEPT_LANGUAGE_HF_STR_SIZE, NULL);
}
static vint _DEC_Allow(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg)
{
    return _DEC_HFhelper(pFSM, pBuff, hMsg, eSIP_ALLOW_HF, SIP_ALLOW_HF_STR_SIZE, NULL);
}
static vint _DEC_AllowEvents(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg)
{
    return _DEC_HFhelper(pFSM, pBuff, hMsg, eSIP_ALLOW_EVENTS_HF, SIP_ALLOW_EVENTS_HF_STR_SIZE, NULL);
}
static vint _DEC_ContentDisp(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg)
{
    return _DEC_HFhelper(pFSM, pBuff, hMsg, eSIP_CONTENT_DISP_HF, SIP_CONTENT_DISP_HF_STR_SIZE, NULL);
}
static vint _DEC_MaxForwards(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg)
{
    return _DEC_HFhelper(pFSM, pBuff, hMsg, eSIP_MAX_FORWARDS_HF, SIP_MAX_BASETEN_NUM_STRING, NULL);
}
static vint _DEC_Organization(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg)
{
    return _DEC_HFhelper(pFSM, pBuff, hMsg, eSIP_ORGANIZATION_HF, SIP_ORGANIZATION_HF_STR_SIZE, NULL);
}
static vint _DEC_Require(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg)
{
     return _DEC_HFhelper(pFSM, pBuff, hMsg, eSIP_REQUIRE_HF, SIP_REQUIRE_HF_STR_SIZE, NULL);
}
static vint _DEC_Server(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg)
{
   return _DEC_HFhelper(pFSM, pBuff, hMsg, eSIP_SERVER_HF, SIP_SERVER_HF_STR_SIZE, NULL);
}
static vint _DEC_Supported(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg)
{
    return _DEC_HFhelper(pFSM, pBuff, hMsg, eSIP_SUPPORTED_HF, SIP_SUPPORTED_HF_STR_SIZE, NULL);
}
static vint _DEC_UserAgent(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg)
{
    return _DEC_HFhelper(pFSM, pBuff, hMsg, eSIP_USER_AGENT_HF, SIP_USER_AGENT_HF_STR_SIZE, NULL);
}
static vint _DEC_ContentEncoding(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg)
{
    return _DEC_HFhelper(pFSM, pBuff, hMsg, eSIP_CONTENT_ENCODING_HF, SIP_CONTENT_ENCODING_HF_STR_SIZE, NULL);
}
static vint _DEC_ReferredBy(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg)
{
    return _DEC_HFhelper(pFSM, pBuff, hMsg, eSIP_REFERRED_BY_HF, SIP_URI_STRING_MAX_SIZE, NULL);
}
static vint _DEC_Unknown(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg)
{
    return _DEC_HFhelper(pFSM, pBuff, hMsg, eSIP_LAST_HF, SIP_UNKNOWN_HF_STR_SIZE, NULL); 
}
static vint _DEC_P_AccessNwInfo(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg)
{
    return _DEC_DummyHlpr(pFSM, pBuff, hMsg);
}

static vint _DEC_RetryAfter(tFSM *pFSM, tL4Packet *pBuff, tSipHandle hMsg)
{
    tSipIntMsg  *pMsg = (tSipIntMsg*)hMsg;
    char *pEnd;

    if (TOKEN_Get(pFSM, pBuff, SIP_CRLF) == SIP_OK) {
        pMsg->retryAfterPeriod = OSAL_strtoul(pFSM->CurrToken.pStart, &pEnd, 10);
        return (SIP_OK);
    }
    return (SIP_FAILED);
}

static vint  _DEC_DummyHlpr (
    tFSM       *pFSM, 
    tL4Packet  *pBuff, 
    tSipHandle  hValue)
{
    UNUSED(hValue);
    return TOKEN_Get(pFSM, pBuff, ";\r\n");
}

   
