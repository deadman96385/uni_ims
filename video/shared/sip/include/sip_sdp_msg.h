/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 29711 $ $Date: 2014-11-06 12:42:22 +0800 (Thu, 06 Nov 2014) $
 */

#ifndef _SIP_SDP_MSG_H_
#define _SIP_SDP_MSG_H_

#include "sip_types.h"
#include "sip_voipnet.h"

#define DYNAMIC_PAYLOAD_TYPE_START 96

   /* Max user name length */
#define MAX_SDP_USERNAME         128
   /* Max number of repeat offsets */
#define MAX_SDP_REPEAT_OFFSETS   10
   /* Max number of time zones */
#define MAX_SDP_ZONES            10
   /* Max encription key length */
#define MAX_SDP_ENCRYPT_KEY      40
   /* Max number of SDP transport formats for a single media */
#define MAX_SDP_FORMATS          12
   /* max number of bytes that an RTP\AVP name can be */
#define MAX_RTPTYPE_NAME_LEN     32
   /* max number of bytes of an SDP attribute string */
#define MAX_SDP_ATTR_STR_LEN     128
   /* max number of bytes of fingerprint */
#define MAX_SDP_FP_STR_LEN       64


/* Check if parameter is present
*/
#define SDP_IS_PARM_PRESENT(pMsg, parmId)    \
                  ((pMsg)->parmPresenceMask & (1 << parmId))


/* Set parameter presence flag
*/
#define SDP_SET_PARM_PRESENT(pMsg, parmId)    \
                  ((pMsg)->parmPresenceMask |= (1 << parmId))

/* SDP attribute types recommended in RFC 2327
*/
typedef enum sSdpAttrType
{
   /* The following attributes corresponds to some of SDP parameters */
   eSdpPAttrOrigin,       /* o=, value is of type t_SdpOrigin* */
   eSdpPAttrName,         /* s=, value is of type char* */
   eSdpPAttrInfo,         /* i=, value is of type char* */
   eSdpPAttrURI,          /* u=, value is of type char* */
   eSdpPAttrEMail,        /* e=, value is of type char* */
   eSdpPAttrPhone,        /* p=, value is of type char* */
   eSdpPAttrTime,         /* t=, list of RANGE (start, stop) values */
   eSdpPAttrRepeat,       /* r=, value of type t_SdpRepeat* */
   eSdpPAttrZone,         /* z=, value of type t_SdpTimeZone* */
   eSdpPAttrEncriptKey,   /* k=, value of type t_SdpEncript* */
   eSdpPAttrBwInfo,       /* b=, value of type tSdpBwInfo* */

   /* The following attributes corresponds to SDP atributes a= */
   eSdpAttrRtpMap,        /* a=rtpmap:<payload type> <encoding name>/<clock rate>[/<encoding parms>] */
   eSdpAttrCat,           /* a=cat:<category> */
   eSdpAttrKeywds,        /* a=keywds:<keywords> */
   eSdpAttrTool,          /* a=tool:<name and version of tool> */
   eSdpAttrPTime,         /* a=ptime:<packet time> */
   eSdpAttrMaxPTime,      /* a=maxptime:<packet time> */
   eSdpAttrMsrpAcm,       /* a=msrp-acm */
   eSdpAttrRecvOnly,      /* a=recvonly */
   eSdpAttrSendRecv,      /* a=sendrecv */
   eSdpAttrSendOnly,      /* a=sendonly */
   eSdpAttrInactive,      /* a=inactive */
   eSdpAttrOrient,        /* a=orient:<whiteboard orientation> */
   eSdpAttrType,          /* a=type:<conference type> */
   eSdpAttrCharset,       /* a=charset:<character set> */
   eSdpAttrSdpLang,       /* a=sdplang:<language tag> */
   eSdpAttrLang,          /* a=lang:<language tag> */
   eSdpAttrFrameRate,     /* a=framerate:<frame rate> */
   eSdpAttrQuality,       /* a=quality:<quality> */
   eSdpAttrFmtp,          /* a=fmtp:<format> <format specific parameters> */
   eSdpAttrExtMap,        /* a=extmap:<value>["/"<direction>] <URI> <extensionattributes> - RFC 5285 */
   eSdpAttrSilenceSupp,   /* a=silenceSupp:<on | off> */
   eSdpAttrRtcp,                  /* a=rtcp: port [NET ADDR] */
   eSdpAttrT38FaxVersion,         /* a=T38FaxVersion: digit */
   eSdpAttrT38MaxBitRate,         /* a=T38MaxBitRate: digit */
   eSdpAttrT38FaxFillBitRemoval,  /* a=T38FaxFillBitRemoval: digit */
   eSdpAttrT38FaxTranscodingMMR,  /* a=T38FaxTranscodingMMR: digit */
   eSdpAttrT38FaxTranscodingJBIG, /* a=T38FaxTranscodingJBIG: digit */
   eSdpAttrT38FaxRateManagement,  /* a=T38FaxRateManagement:
                                     transferredTCF | localTCF */
   eSdpAttrT38FaxMaxBuffer,       /* a=T38FaxMaxBuffer: digit */
   eSdpAttrT38FaxMaxDatagram,     /* a=T38FaxMaxDatagram: digit */
   eSdpAttrT38FaxUdpEC,           /* a=eSdpAttrT38FaxUdpEC:
                                     t38UDPRedundancy | t38UDPFEC */
   eSdpAttrCrypto,                /* a=crypto:<tag> <crypto-suite> <key-params>
                                   * [<session-params>] */
   eSdpAttrEncryption,            /* a=encryption:<status> */
   eSdpAttrPath,                  /* a=path:<path & uri> */
   eSdpAttrAcceptTypes,           /* a=accept-types:<content types that are supported> */
   eSdpAttrAcceptWrappedTypes,    /* a=accept-wrapped-types:<content types that are supported> */
   eSdpAttrCurr,                  /* a=curr: precondition-type SP status-type SP direction-tag */
   eSdpAttrDes,                   /* a=des: precondition-type SP strength-tag SP status-type SP direction-tag */
   eSdpAttrConf,                  /* a=conf: precondition-type SP status-type SP direction-tag */
   eSdpAttrFileSelector,          /* a=file-selector:name: name of file transferred
                                   *                 type: file type
                                   *                 size: file size
                                   *                 hash: sha1 hash of file
                                   */
   eSdpAttrFileDisposition,       /* a=file-disposition:<render | attachment> */
   eSdpAttrFileTransferId,        /* a=file-transfer-id: IR.79/RFC5547 req't. */
   eSdpAttrSetup,                 /* a=setup:<passive/active> etc. */
   eSdpAttrFingerprint,           /* a=fingerprint: hash-func fingerprint */
   eSdpAttrFramesize,             /* a=framesize:<payload type number> <width>-<height> */
   eSdpAttrTcap,                  /* a=tcap: additional supported capabilities */
   eSdpAttrPcfg,                  /* a=pcfg: potential configurations */
   eSdpAttrRtcpfb,                /* a=rtcp-fb: rtcp feedback attributes */
   eSdpAttrLast
}  tSdpAttrType ;

/* SDP RtpMap attribute */
typedef struct sSdpAttrRtpMap
{
   char    szEncodingName[MAX_RTPTYPE_NAME_LEN];
   uint16  payloadType;
   uint16  numChan;
   uint32  clockRate;
} tSdpAttrRtpMap;

/* SDP Fmtp attribute */
typedef struct sSdpAttrFmtp
{
   char    szArg[MAX_SDP_ATTR_STR_LEN];
   uint16  payloadType;
} tSdpAttrFmtp;

/* SDP extmap attribute */
typedef struct sSdpAttrExtMap
{
   char    extmapUri[MAX_SDP_ATTR_STR_LEN];
   uint16  extmapId;
} tSdpAttrExtMap;

/* SDP framesize attribute */
typedef struct sSdpAttrFramesize
{
   uint16  payloadType;
   uint16  width;
   uint16  height;
} tSdpAttrFramesize;

/* SDP tcap attribute */
typedef struct sSdpAttrTcap
{
   char    tcapStr[MAX_SDP_ATTR_STR_LEN];
   uint16  tcapId;
} tSdpAttrTcap;

/* SDP potential config attribute */
typedef struct sSdpAttrPcfg
{
   char    pcfgStr[MAX_SDP_ATTR_STR_LEN];
   uint16  pcfgId;
} tSdpAttrPcfg;

/* SDP rtcp-fb attribute */
typedef struct sSdpAttrRtcpfb
{
   char    rtcpfbStr[MAX_SDP_ATTR_STR_LEN];
   uint16  rtcpfbId;
} tSdpAttrRtcpfb;

/* SDP Crypto attribute */
typedef struct sSdpAttrCrypt
{
    uint32        tag;
    char          cryptoSuite[32 + 1];
    char          keyParamsAes80[MAX_SDP_ENCRYPT_KEY+1];
    char          keyParamsAes32[MAX_SDP_ENCRYPT_KEY+1];
} tSdpAttrCrypto;

/* SDP Encryption attribute */
typedef struct sSdpAttrEncrypt
{
    char status[32 + 1];
} tSdpAttrEncryption;

/* Precondition Status */
typedef tSdpAttrType tPrecStatus;

/* Precondition Type */
typedef enum ePrecType
{
    ePrecTypeQos
} tPrecType;

/* Precondition Strength tag */
typedef enum ePrecStrength
{
    ePrecStrengthMandatory,
    ePrecStrengthOptional,
    ePrecStrengthNone,
    ePrecStrengthFailure,
    ePrecStrengthUnknown
} tPrecStrength;

/* Precondition status type */
typedef enum ePrecSType
{
    ePrecStatusTypeE2e,
    ePrecStatusTypeLocal,
    ePrecStatusTypeRemote
} tPrecStatusType;

/* Precondition direction tag */
typedef enum ePrecDirTag
{
    ePrecDirNone,
    ePrecDirSend,
    ePrecDirRecv,
    ePrecDirSendRecv
} tPrecDir;

/* SDP precondition attribute */
typedef struct sSdpAttrPrec
{
    tPrecStatus         status;
    tPrecType           type;
    tPrecStrength       strength;
    tPrecStatusType     statusType;
    tPrecDir            dir;
} tSdpAttrPrecondition;

/* SDP attr fingerprint hash function */
typedef enum sSdpFpHash
{
    eSdpHashSha1,    /* sha-1 */
    eSdpHashSha224,  /* sha-224 */
    eSdpHashSha256,  /* sha-256 */
    eSdpHashSha384,  /* sha-384 */
    eSdpHashSha512,  /* sha-512 */
    eSdpHashMd5,     /* md5 */
    eSdpHashMd2,     /* md2 */
} tSdpHash;

/* SDP fingerprint attribute */
typedef struct sSdpAttrFingerprint
{
    tSdpHash hash;
    char fingerprint[MAX_SDP_FP_STR_LEN];
} tSdpAttrFingerprint;

/* SDP setup enum */
typedef enum sSdpSetupRole
{
    eSdpSetupActive = 1,    /* active */
    eSdpSetupActpass,   /* actpass */
    eSdpSetupPassive,   /* passive */
} tSdpSetupRole;

/* SDP setup attribute */
typedef struct sSdpAttrSetup
{
    tSdpSetupRole role;
} tSdpAttrSetup;

/* SDP session version */
typedef uint32 tSdpSessVersion;

/* SDP session ID */
typedef uint32 tSdpSessId;

/* SDP owner
   o=<username> <session id> <version> <network type> <address type> <address>
*/
typedef struct sSdpOrigin
{
   char             userName[MAX_SDP_USERNAME];
   tSdpSessId       sessId;
   tSdpSessVersion  sessVersion;
   tNetworkType     nwType;        /* t_NetworkType  */
   tNetworkAddress  nwAddress;     /* Network address */
} tSdpOrigin;

/* SDP bandwidth modifier */
typedef enum sSdpBwModifier
{
   eSdpBwCT,         /* Conference total */
   eSdpBwAS,         /* Application-specific maximum */
   eSdpBwRS,         /* RTCP bandwidth allocated to active data senders */
   eSdpBwRR,         /* RTCP bandwidth allocated to active data receivers */
   eSdpBwTIAS,       /* Transport Independent Application Specific Maximum */
} tSdpBwModifier;

/* SDP bandwidth */
typedef struct sSdpBw
{
   tSdpBwModifier   bwModifier;
   uint16           bw;
} tSdpBw ;

typedef struct sSdpValue
{
   struct sSdpValue *next;      /* To support list of values */
   uint16            valType;   /* One of SIP_VAL_.. constants */
   uint16            valLength; /* Value length */
   union {
      uint32   uparm;           /* DWORD parameter */
      char    *cparm;           /* Pointer parameter */
      struct {                  /* Range parameter */
         uint32   loval;
         uint32   hival;
      } r;
      tSdpAttrRtpMap       rtpMap;
      tSdpAttrFmtp         fmtp;
      tSdpAttrExtMap       extMap;
      tSdpOrigin           origin;
      tSdpAttrCrypto       crypto;
      tSdpAttrEncryption   encryption;
      tSdpAttrPrecondition precondition;
      tSdpAttrFingerprint  fingerprint;
      tSdpAttrSetup        setup;
      tSdpBw               bwInfo;
      tSdpAttrTcap         tcap;
      tSdpAttrPcfg         pcfg;
      tSdpAttrRtcpfb       rtcpfb;
      char                 szStr[MAX_SDP_ATTR_STR_LEN];
      tSdpAttrFramesize    framesize;
   } x;
} tSdpValue;


/* Attribute */
typedef struct sAttribute
{
   tDLListEntry dll;    /* Must always be first in any DLL managed structure */
   struct sAttribute *next;

   uint32       id;          /* Attribute id */
   tSdpValue    value;       /* Value(s) */
} tAttribute ;

/* SDP parameter type */
typedef enum eSdpParm
{
   eSdpProtVersion,       /* v= (protocol version) */
   eSdpOrigin,            /* o= (owner/creator) and session identifier */
   eSdpName,              /* s= (session name) */
   eSdpInfo,              /* i=* (session information) */
   eSdpURI,               /* u=* (URI of description) */
   eSdpEMail,             /* e=* (email address) */
   eSdpPhone,             /* p=* (phone number) */
   eSdpConnInfo,          /* c=* (connection information) */
   eSdpBWInfo,            /* b=* (bandwidth information) */
   eSdpTimeZone,          /* z=* (time zone adjustment) */
   eSdpEncriptKey,        /* k=* (encription key) */
      /* Time description */
   eSdpTime,              /* t= (time the session is active) */
   eSdpRepeatTime,        /* r=* (zero o more repeat times) */
   eSdpAttr,              /* a=* (attribute line(s)) */
      /* Media description */
   eSdpMedia,             /* m= (media name and transport address) */
} tSdpParm;


/* SDP version */
typedef uint16 tSdpVersion;

/* IP Multicast address extension */
typedef struct sIPMCastAddrExt
{
   uint8    ttl;   /* TTL */
   uint8    num;   /* Number of MC addresses in the group */
} tIPMCastAddrExt;


/* SDP connection information */
typedef struct sSdpConnInfo
{
   tDLListEntry dll;    /* Must always be first in any DLL managed structure */

   struct sSdpConnInfo *next;

   tNetworkType     nwType;
   tNetworkAddress  nwAddress;
   union {
      tIPMCastAddrExt  extMCast;   /* IP Multicast address extension */
   } x;
} tSdpConnInfo;


/* SDP repeat interval */
typedef struct sSdpRepeat
{
   uint32   interval;
   uint32   duration;
   uint32   offsets[MAX_SDP_REPEAT_OFFSETS];
} tSdpRepeat;


/* SDP time zone */
typedef struct sSdpTimeZone
{
   uint32   time[MAX_SDP_ZONES];
   uint32   offset[MAX_SDP_ZONES];
} tSdpTimeZone;


/* SDP encription method */
typedef enum eSdpEncriptMethod
{
   eSdpEncrClear,
   eSdpEncrBase64,
   eSdpEncrUri,
   eSdpEncrPrompt
} tSdpEncriptMethod ;


/* SDP encription key */
typedef struct sSdpEncript
{
   tSdpEncriptMethod   method;
   char                key[MAX_SDP_ENCRYPT_KEY];
}  tSdpEncript;

/* SDP Media type */
typedef enum eSdpMediaType
{
   eSdpMediaNone,
   eSdpMediaAudio,
   eSdpMediaImage,
   eSdpMediaVideo,
   eSdpMediaApplication,
   eSdpMediaMsMessage,
   eSdpNas,
   eSdpNasRadius,
   eSdpNasTacacs,
   eSdpNasDiameter,
   eSdpNasL2TP,
   eSdpNasLogin
} tSdpMediaType;


/* SDP port. 
   Depends on transport, but keep it simple for now 
*/
typedef uint16    tSdpPort;


/* SDP transport format. Meaning of this field depends on transport type */
typedef uint16    tSdpFormat;


/* SDP media description */
typedef struct sSdpMedia
{
   tDLListEntry dll;    /* Must always be first in any DLL managed structure */

   struct sSdpMedia *next;

   /* Parameter presence mask in the descriptor */
   uint32              parmPresenceMask;

   /* e_SdpMedia: m= parameters */
   tSdpMediaType       mediaType;
   tSdpPort            port;
   uint16              nPorts;
   tTransportType      transpType;
   uint16              nFormats;
   tSdpFormat          formats[MAX_SDP_FORMATS];

   tSdpConnInfo        connInfo;   /* e_SdpMediaConnInfo: c= */
   tSdpBw              bwInfo;     /* e_SdpBWInfo: b= */
   tAttribute         *pParmAttr;  /* e_SdpMediaTitle:i=, e_SdpMediaEncriptKey:k= */
   tAttribute         *pAttr;      /* e_SdpAttr: a= */
} tSdpMedia;


typedef struct sSdpMsg
{
   tDLListEntry dll;    /* Must always be first in any DLL managed structure */

   struct sSdpMsg   *next;
   /* Presense flag */
   uint32                  parmPresenceMask;

   /* Session description level */
   tSdpVersion         version;    /* e_SdpProtVersion: v= */
   tSdpConnInfo        connInfo;   /* e_SdpConnInfo: c= */
   tAttribute         *pParmAttr; /* o=,s=,i=,u=,e=,p=,t=,r=,z=,k= */
   tAttribute         *pAttr;     /* e_SdpAttr: a= */

   /* Media stream(s) */
   tSdpMedia           media;      /* e_SdpMedia: m= and other media-level parameters */

   vint                 careFlag;

} tSdpMsg;

/* Static payload types per RFC 1890  */
typedef struct sRtpAvp {
    const char *encodingName;
    uint32 clockRate;
    uint32 numChan;
} tRtpAvp;


/********************************************************** 
 * Message helpers 
 *********************************************************/

tSdpMsg* SDP_AllocMsg(void);

void SDP_DeallocMsg(tSdpMsg *pMsg);

void SDP_DeallocMedia(tSdpMedia *pMedia);

tSdpMsg* SDP_CopyMsg(tSdpMsg *pMsg);

vint SDP_InsertAttr(
    tAttribute **ppAttr, 
    tAttribute  *pAttr);

tAttribute* SDP_FindAttr(
    tAttribute  *pAttrList, 
    tSdpAttrType attribute);

#endif   /* ifndef  __SDPMSG_H__ */

