/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 30075 $ $Date: 2014-11-27 10:31:32 +0800 (Thu, 27 Nov 2014) $
 */

#ifndef _SIP_SESSION_H_
#define _SIP_SESSION_H_

typedef enum {
    SIP_PROTO_UDP = 0,
    SIP_PROTO_TCP,
    SIP_PROTO_TCPACT,
    SIP_PROTO_TCPPASS,
    SIP_PROTO_SSLTCP,
    SIP_PROTO_LAST,
} SIP_CandProto;

typedef enum {
    SIP_CAN_HOST = 0,
    SIP_CAN_LOCAL,
    SIP_CAN_SRFLX,
    SIP_CAN_PRFLX,
    SIP_CAN_RELAY,
    SIP_CAN_STUN,
    SIP_CAN_LAST,
} SIP_CandType;

typedef struct {
    uint8         component;
    uint8         foundation;
    uint8         generation;
    uint8         network;
    uint32        ip;
    uint16        port;
    SIP_CandProto protocol;
    SIP_CandType  candidate;
    char          priority[32 + 1];
    char          pwd[32 + 1];
    char          ufrag[32 + 1];
} SIP_Candidate;


#define STR_RTCP_FB_RTP_SAVPF "RTP/SAVPF"
#define STR_RTCP_FB_RTP_AVPF "RTP/AVPF"
#define STR_RTCP_FB_NACK "nack"
#define STR_RTCP_FB_NACK_PLI "nack pli"
#define STR_RTCP_FB_CCM_TMMBR "ccm tmmbr"
#define STR_RTCP_FB_CCM_FIR "ccm fir"

typedef struct sPrecondition {
    uint16              numStatus;      /* number of status */
    uint8               isConfReceived;
    uint8               isMet;
    struct {
        tPrecStatus     status;
        tPrecType       type;
        tPrecStrength   strength;
        tPrecStatusType statusType;
        tPrecDir        dir;
    } status[SIP_PREC_STATUS_SIZE_MAX];
} tPrecondition;

typedef struct sFingerprint {
    tSdpHash hash;
    char     fingerprint[MAX_SDP_FP_STR_LEN];
} tFingerprint;

typedef struct sMediaCoder
{
    uint8   payloadType;
    uint8   decodePayloadType;  /* Decode payload type. This is used for asymmetric payload type */
    uint32  clockRate;
    char    encodingName[MAX_SESSION_MEDIA_STR + 1];
    char    fmtp[MAX_SESSION_MEDIA_LARGE_STR + 1];
    int     width;
    int     height;
    int     framerate;
    char    sps[MAX_SESSION_MEDIA_STR + 1];
    struct {
        int id;
        char uri[MAX_SDP_ATTR_STR_LEN + 1];
    } extmap;
    uint32  bandwidth;
}tMediaCoder;

typedef struct sMedia
{
    uint8           packetRate;
    uint8           maxPacketRate;
    uint8           numCoders;
    uint32          numSdpCand;
    uint16          rmtRtpPort;
    uint16          rmtRtcpPort;
    uint16          lclRtpPort;
    uint16          lclRtcpPort;
    /* Local - RTP Session bandwidth in kbps - AS bandwidth parameter. */
    uint32          lclAsBwKbps;
    /* Remote party - RTP Session bandwidth in kbps - AS bandwidth parameter. */
    uint32          rmtAsBwKbps;
    vint            supportSrtp;
    vint            supportAVPF;
    vint            useSrtp;
    vint            useAVPF;
    vint            use_FB_NACK;
    vint            use_FB_TMMBR;
    vint            use_FB_TMMBN;
    vint            use_FB_PLI;
    vint            use_FB_FIR;
    char            cryptoSuite[MAX_SESSION_MEDIA_STR + 1];
    char            srtpKeyParamsAes80[MAX_SESSION_SRTP_PARAMS + 1];
    char            srtpKeyParamsAes32[MAX_SESSION_SRTP_PARAMS + 1];
    SIP_Candidate   candidates[MAX_SESSION_SIT_CANDIDATES];
    tSdpMediaType   mediaType;
    tSdpAttrType    lclDirection;
    tSdpAttrType    rmtDirection;
    tSdpAttrType    lclOriDir;
    char            acceptTypes[MAX_SESSION_MEDIA_LARGE_STR + 1];
    char            acceptWrappedTypes[MAX_SESSION_MEDIA_LARGE_STR + 1];
    char            path[MAX_SESSION_MEDIA_LARGE_STR + 1];
    uint8           fax;            /* for a=Fax */

    /*
     * RFC 5547 additional parameters
     * A Session Description Protocol (SDP) Offer/Answer Mechanism
     * to Enable File Transfer
     */
    /* description of file, e.g. "check out this photo!" */
    char            fileSubject[MAX_SESSION_MEDIA_LARGE_STR + 1];
    /* name of file, e.g. "Chicago.jpg" */
    char            fileSelectorName[MAX_SESSION_MEDIA_LARGE_STR + 1];
    /* type of file, e.g. "image/jpg" */
    char            fileSelectorType[MAX_SESSION_MEDIA_LARGE_STR + 1];
    vint            fileSelectorSize;
    vint            fileSelectorHash;
    /* disposition of file transfer, e.g. "render" or "attachment" */
    char            fileDisposition[MAX_SESSION_MEDIA_LARGE_STR + 1];
    /* unique Id for file transfer */
    char            fileTransferId[MAX_SESSION_MEDIA_LARGE_STR + 1];

    tPrecondition   precondition;
    tFingerprint    fingerprint;
    tSdpSetupRole   setupRole;
    tMediaCoder     aCoders[SYSDB_MAX_NUM_CODERS];
    int             bwAs; /* bandwidth AS */
    int             bwRr; /* bandwidth RR */
    int             bwRs; /* bandwidth RS */
    struct {
        vint T38FaxVersion;
        vint T38MaxBitRate;
        vint T38FaxFillBitRemoval;
        vint T38FaxTranscodingMMR;
        vint T38FaxTranscodingJBIG;
        char T38FaxRateManagement[MAX_SESSION_MEDIA_STR + 1];
        vint T38FaxMaxBuffer;
        vint T38FaxMaxDatagram;
        char T38FaxUdpEC[MAX_SESSION_MEDIA_STR + 1];
    } t38;
    int             framerate;
}tMedia;

typedef struct sSession
{
    int             numMedia;
    tMedia          media[MAX_SESSION_MEDIA_STREAMS];
    tNetworkAddress lclAddr;
    tSdpAttrType    lclDirection;
    uint32          lclSessId;
    uint32          lclSessVersion;
    tNetworkAddress rmtAddr;
    tSdpAttrType    rmtDirection;
    uint32          rmtSessId;
    uint32          rmtSessVersion;
    char            otherPayload[MAX_SESSION_PAYLOAD_SIZE + 1];
}tSession;

typedef struct sSessionEntry
{
    char        szName[SIP_USERNAME_ARG_STR_SIZE + 1];
    uint8       defaultNumCoders;
    char        defaultCoders[SYSDB_MAX_NUM_CODERS];
    tPacketRate defaultPrate;
    vint        isNew;
    tSession    sess;
}tSessionEntry;

tRtpAvp *SDP_staticAudioPTtoRtpAvp(uint16 pt);

tSdpMsg* SESSION_MakeSdp(
    tSipHandle   hSession);

tSdpMsg* SESSION_Encode(
    tSipHandle   hSession,
    tSession    *pMediaSess);

vint SESSION_Decode(
    tSipHandle   hSession,
    tSdpMsg     *pSdp);

void SESSION_Init(
    tSipHandle   hSession, 
    char        *pName,
    char        *aCoders,
    tPacketRate *pPrate);

void SESSION_Destroy(tSipHandle hSession);

void SESSION_ResetIsNew(tSipHandle hSession);

#endif
