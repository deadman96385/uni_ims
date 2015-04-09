/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 29374 $ $Date: 2014-10-16 14:57:45 +0800 (Thu, 16 Oct 2014) $
 */

#ifndef _SIP_HDRFLDS_H_
#define _SIP_HDRFLDS_H_

#include "sip_list.h"

/* a few macros */
#define HF_ARG_SET_PRESENCE(pMask, field) ((*pMask) = ((*pMask) | (0x01 << field)))
#define HF_ARG_CLR_PRESENCE(pMask, field) ((*pMask) = ((*pMask) & (~(0x01 << field))))
#define HF_ARG_PRESENCE_EXISTS(pMask, field) (((*pMask) & (0x01 << field))?TRUE:FALSE)

/* SIP version number */
#define SIP_VERSION_STR "SIP/2.0"

/* header field configuration info */
#define SIP_ACCEPT_HF_STR_SIZE           (128)
#define SIP_ACCEPT_ENCODING_HF_STR_SIZE  (64)
#define SIP_ACCEPT_LANGUAGE_HF_STR_SIZE  (64)
#define SIP_ALLOW_HF_STR_SIZE            (128)
#define SIP_ALLOW_EVENTS_HF_STR_SIZE     (64)
#define SIP_CONTENT_DISP_HF_STR_SIZE     (64)
#define SIP_CONTENT_TYPE_HF_STR_SIZE     (128)
#define SIP_CONTENT_ENCODING_HF_STR_SIZE (128)
#define SIP_EVENT_HF_STR_SIZE            (128)
#define SIP_ETAG_HF_STR_SIZE             (32)
#define SIP_EXPIRES_HF_STR_SIZE          (16) 
#define SIP_IF_MATCH_HF_STR_SIZE         (32)
#define SIP_SUB_STATE_HF_STR_SIZE        (128)
#define SIP_EVENT_HF_PARAM_STR_SIZE      (32)
#define SIP_EVENT_HF_ID_STR_SIZE         (16)
#define SIP_EVENT_HF_PACKAGE_STR_SIZE    (32)
#define SIP_ORGANIZATION_HF_STR_SIZE     (128)
#define SIP_REQUIRE_HF_STR_SIZE          (64)
#define SIP_SERVER_HF_STR_SIZE           (64)
#define SIP_SUPPORTED_HF_STR_SIZE        (128)
#define SIP_USER_AGENT_HF_STR_SIZE       (128)
#define SIP_CALL_ID_HF_RAND_MIN_LEN      (8)
#define SIP_CALL_ID_HF_RAND_MAX_LEN      (32)
#define SIP_REFERRED_BY_CID_MAX_SIZE     (128)
#define SIP_USER_STR_SIZE                (64)
#define SIP_CAPABILITIES_STR_SIZE        (512)
#define SIP_INSTANCE_STR_SIZE            (64)
#define SIP_VERSION_STR_SIZE             (16)
#define SIP_UNKNOWN_HF_STR_SIZE          (128) 
#define SIP_HF_STR_SIZE_MAX              (256)
#define SIP_Q_VALUE_STR_SIZE             (8)
#define SIP_METHOD_STR_SIZE              (8)

#define SIP_TAG_HF_RAND_MIN_LEN         (5)
#define SIP_TAG_HF_RAND_MAX_LEN         (15)

#define SIP_BRANCH_HF_RAND_MIN_LEN      (8)
#define SIP_BRANCH_HF_RAND_MAX_LEN      (32)

#define SIP_MAGIC_COOKIE_STR_DFLT       ("z9hG4bK")
#define SIP_MAGIC_COOKIE_STR_LEN        (7)

#define SIP_URI_UID_STR_SIZE            (32)
#define SIP_URI_USS_STR_SIZE            (32)
#define SIP_CALL_ID_HF_STR_SIZE         (256)
#define SIP_MAX_REASON_PHRASE_STR_LEN   (128)
#define SIP_URI_STRING_MAX_SIZE         (384)
#define SIP_PUB_GRUU_STR_SIZE           (SIP_URI_STRING_MAX_SIZE + SIP_INSTANCE_STR_SIZE)
#define SIP_BRANCH_HF_STR_SIZE          (256)
#define SIP_TAG_HF_STR_SIZE             (256)
#define SIP_LSKPMC_STR_SIZE             (12)

#ifdef SIP_HF_USE_SHORT_HF
#define SIP_URI_USER_STR_SIZE           (32)
#define SIP_DISPLAY_NAME_STR_SIZE       (32)
#define SIP_URI_ARG_STR_SIZE            (64)
#else
#define SIP_URI_USER_STR_SIZE           (320)
#define SIP_DISPLAY_NAME_STR_SIZE       (64)
#define SIP_URI_ARG_STR_SIZE            (128)
#endif

/* Access Network Info id string size */
#define SIP_ACCESS_NW_INFO_ID_STR_SIZE   (64)

/* Used for authorization */
#define SIP_USERNAME_ARG_STR_SIZE        (64)
#define SIP_REALM_ARG_STR_SIZE           (64)
#define SIP_PASSWORD_ARG_STR_SIZE        (64)
#define SIP_NONCE_ARG_STR_SIZE           (128)
#define SIP_CNONCE_ARG_STR_SIZE          (128)
#define SIP_NONCE_CNT_ARG_STR_SIZE       (12) 
#define SIP_OPAQUE_ARG_STR_SIZE          (128)
#define SIP_RESPONSE_ARG_STR_SIZE        (128)
#define SIP_AUTS_ARG_STR_SIZE            (32)
#define SIP_CNONCE_RAND_MIN_LEN          (8)
#define SIP_CNONCE_RAND_MAX_LEN          (15)
#define SIP_BOUNDRY_MAX_LEN              (64)

/* This is really unused */
#define SIP_B64_USER_PW_STR_SIZE         (8)
/* Authorization End here*/

typedef uint32 tMinSeHF;
typedef uint32 tContentLengthHF;
typedef uint32 tExpiresHF;
typedef uint32 tMinExpiresHF;
typedef uint32 tRSeqHF;

typedef tSipContentType tContentTypeHF;

typedef char tCallIdHF[SIP_CALL_ID_HF_STR_SIZE];

typedef char tBoundry[SIP_BOUNDRY_MAX_LEN];

typedef struct sHdrFldList
{
    tDLListEntry    dll;    /* Must always be first in any DLL managed structure */
    tHdrFld             hf;
    char                pStart[SIP_HF_STR_SIZE_MAX];
    char               *pField;
    struct sHdrFldList *pNext;
}tHdrFldList;

typedef enum eUriArgType{
    eURI_ARG_TYPE_PSBR = 0,
    eURI_ARG_TYPE_LBFH,
}eUriArgType;

typedef struct sUri
{
    tDLListEntry    dll;    /* Must always be first in any DLL managed structure */
    tSipUriScheme   scheme;
    char            user[SIP_URI_USER_STR_SIZE];
    char            uid[SIP_URI_UID_STR_SIZE];
    char            uss[SIP_URI_USS_STR_SIZE];
    tNetworkAddress host;
     /* parameters */
    uint32          method;
    tIPAddr         maddr;
    uint32          ttl;
    tTransportType  transport;
    vint            lr;
    char            szUserParam[SIP_USER_STR_SIZE];
    char            szPhoneCxtParam[SIP_USER_STR_SIZE];
    char            szSessionParam[SIP_USER_STR_SIZE];
    char            szConf[SIP_URI_ARG_STR_SIZE];
    char            szFtag[SIP_URI_ARG_STR_SIZE];
    char            szGruu[SIP_URI_ARG_STR_SIZE];
    char            szLskpmc[SIP_LSKPMC_STR_SIZE];
    eUriArgType     argType;
    union {
        char szPsbr[SIP_URI_ARG_STR_SIZE];
        char szLbfh[SIP_URI_ARG_STR_SIZE];
    }arg;
} tUri;

typedef struct sContactHFE
{
    tDLListEntry dll;    /* Must always be first in any DLL managed structure */

    tUri    uri;
    char    szDisplayName[SIP_DISPLAY_NAME_STR_SIZE];
    uint32  expires;
    char    q[SIP_Q_VALUE_STR_SIZE];
    char    szUser[SIP_USER_STR_SIZE];
    uint32  isImSession;
    uint32  capabilitiesBitmap; /* bitmap of capabilities */
    uint32  isFocus; /* RFC4579, represents dialog belongs to a conference */
    char    szInstance[SIP_INSTANCE_STR_SIZE];
    char    szPublicGruu[SIP_PUB_GRUU_STR_SIZE];
} tContactHFE;

typedef struct sUriPlus
{
    tUri uri;
    char szDisplayName[SIP_DISPLAY_NAME_STR_SIZE];
    char szTag[SIP_TAG_HF_STR_SIZE];
    char szUser[SIP_USER_STR_SIZE];
} tUriPlus;

typedef struct sRouteHFE
{
    tDLListEntry dll;    /* Must always be first in any DLL managed structure */

    tUri    uri;
} tRouteHFE;

typedef tUriPlus tFromHF;
typedef tUriPlus tToHF; 

typedef struct sCSeq
{
    uint32     seqNum;
    tSipMethod method;
} tCSeq;

typedef struct sRAck
{
    uint32     seqNum;
    tCSeq      cseq;
} tRAck;

typedef struct sViaHFE
{
    tDLListEntry dll;    /* Must always be first in any DLL managed structure */

    tUri            uri;
    char            szVersion[SIP_VERSION_STR_SIZE];
    char            szBranch[SIP_BRANCH_HF_STR_SIZE];
    tIPAddr         received;
    uint16          rport;
    OSAL_Boolean    keep;
    uint32          keepaliveFeq;
}tViaHFE;

typedef struct sAuthorizationHFE
{
    tDLListEntry dll;    /* Must always be first in any DLL managed structure */

    tHdrFld         hdrFld;
    tSipAuthType    type;
    char            szRealm[SIP_REALM_ARG_STR_SIZE];
    char            domain[MAX_DOMAIN_NAME_LEN];
    char            szNonce[SIP_NONCE_ARG_STR_SIZE];
    char            szOpaque[SIP_OPAQUE_ARG_STR_SIZE];
    vint            stale;
    tSipAuthAlg     alg;
    tSipAuthQop     qop;
    char            szUsername[SIP_USERNAME_ARG_STR_SIZE];    
    tUri            uri;
    char            szCNonce[SIP_CNONCE_ARG_STR_SIZE];
    char            szNC[SIP_NONCE_CNT_ARG_STR_SIZE];
    char            szResponse[SIP_RESPONSE_ARG_STR_SIZE];
    char            szAuts[SIP_AUTS_ARG_STR_SIZE];
    char            szB64UserPw[SIP_B64_USER_PW_STR_SIZE];
    uint32          presence;
}tAuthorizationHFE;

typedef struct sSubStateHF
{
    tSipSubStateHFArg arg;
    uint32            expires;
    char              szReason[SIP_MAX_REASON_PHRASE_STR_LEN];
}tSubStateHF;

typedef struct sEventHF
{
    char        szPackage[SIP_EVENT_HF_PACKAGE_STR_SIZE];
    char        szId[SIP_EVENT_HF_ID_STR_SIZE];
    char        szParam[SIP_EVENT_HF_PARAM_STR_SIZE];
}tEventHF;

typedef struct sReplacesHF
{
   char szToTag[SIP_TAG_HF_STR_SIZE];
   char szFromTag[SIP_TAG_HF_STR_SIZE];
   char szCallId[SIP_CALL_ID_HF_STR_SIZE];
   vint earlyFlag;
}tReplacesHF;

typedef struct sReferToHF
{
    tUriPlus    uriPlus;
    tReplacesHF replaces;
    tSipMethod  method;
}tReferToHF;

/* these are used for bits representing which header fields are being used */
typedef struct sPres64Bits
{
    uint32 one;
    uint32 two;
} tPres64Bits;

typedef struct sPres64BitEntry
{
    tPres64Bits mand;
    tPres64Bits optn;
} tPres64BitEntry;

typedef struct sAUTH_Cred
{
    tDLListEntry  dll;
    char          szUsername[SIP_USERNAME_ARG_STR_SIZE];
    char          szRealm[SIP_REALM_ARG_STR_SIZE];
    uint32        types;
    char          szPassword[SIP_PASSWORD_ARG_STR_SIZE];
    struct {
        char      k[SIP_AUTH_KEY_SIZE];
        vint      kLen;
        char      op[SIP_AUTH_OP_SIZE];
        vint      opLen;
        char      amf[SIP_AUTH_AMF_SIZE];
        vint      amfLen;
    } aka;
} tAUTH_Cred;

typedef struct sSessionExpiresHF
{
    uint32        expires;
    tSipRefresher refresher;
} tSessionExpiresHF;

typedef enum eNwAccessType
{
    eNwAccessTypeNone = 0,
    eNwAccessTypeGeran,
    eNwAccessTypeUtranFdd,
    eNwAccessTypeUtranTdd,
    eNwAccessTypeEUtranFdd,
    eNwAccessTypeEUtranTdd,
    eNwAccessType80211,
}tNwAccessType;

typedef struct {
    tNwAccessType   type;
    char            id[SIP_ACCESS_NW_INFO_ID_STR_SIZE];
}tNwAccess;

vint HF_IsEqualUri(
    tUri *pA, 
    tUri *pB);

vint HF_IsEqualUriPlus(
    tUriPlus *pA, 
    tUriPlus *pB);

vint HF_IsEqualVia(
    tViaHFE *pA, 
    tViaHFE *pB);

vint HF_PresenceExists(
    tPres64Bits *pBitMap, 
    tHdrFld      hdrFld);

void HF_SetPresence(
    tPres64Bits *pBitMap, 
    tHdrFld      hdrFld);

void HF_ClrPresence(
    tPres64Bits *pBitMap, 
    tHdrFld      hdrFld);

void HF_GenerateCallId(
    char *pDomain, 
    char *pTargetStr);

uint32 HF_GenerateSeqNum(void);

void HF_GenerateTag(
    char *pTargetStr);

void HF_GenerateBranch(
    char *pTargetStr);

vint HF_MagicCookieExists(
    char* pCookie);

void HF_CleanUri(
    tUri *pUri);

void HF_CleanUriPlus(
    tUriPlus *pUri);

void HF_CleanVia(
    tViaHFE *pVia);

void HF_CleanRoute(
    tRouteHFE *pRoute);

void HF_CleanContact(
    tContactHFE *pContact);

void HF_CleanAuthorization(
    tAuthorizationHFE *pAuth);

vint HF_MakeVia(
    tDLList        *pViaList,
    tUri           *pUri,
    char           *pBranch,
    tIPAddr         received,
    OSAL_Boolean    keep);

vint HF_MakeViaBranch(
    tDLList *pViaList, 
    char    *pBranch);

void HF_CatViaBranch(
    char    *pBranch);

void HF_Insert(
    tHdrFldList **ppHead, 
    tHdrFldList  *pItem);

tHdrFldList* HF_Find(
    tHdrFldList **ppHead, 
    tHdrFld       hf);

void HF_Delete(
    tHdrFldList **ppHead, 
    tHdrFld       hf);

vint HF_CopyInsert(
    tHdrFldList **ppHead,
    tHdrFld       hf,
    char         *pStr,
    uint32        offset);

vint HF_CopyAll(
    tHdrFldList **ppTo,
    tHdrFldList  *pFrom,
    tPres64Bits  *pBitMap);

void HF_DeleteAll(
    tHdrFldList **ppHFList);

void HF_UpdateNC(
    char *pTargetStr);

void HF_GenerateCNonce(
    char *pTargetStr);

vint HF_CheckRequiredHF(
    tHdrFldList **ppHead,
    char         *value_ptr);

vint HF_CheckSupportedHF(
    tHdrFldList **ppHead,
    char         *value_ptr);

vint HF_CheckUnKnownHF(
    tHdrFldList **ppHead,
    char         *hdr_ptr);

#endif
