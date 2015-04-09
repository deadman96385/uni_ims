/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004-2010 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 30131 $ $Date: 2014-12-01 14:45:03 +0800 (Mon, 01 Dec 2014) $
 */

#ifndef _VTSPR_STRUCT_H_
#define _VTSPR_STRUCT_H_

#include <osal.h>
#include "osal_net.h"
#include "osal_types.h"
#include "rtp.h"

#if defined(VTSP_ENABLE_G726_ACCELERATOR) || \
        defined(VTSP_ENABLE_G729_ACCELERATOR) || \
        defined(VTSP_ENABLE_G711P1_ACCELERATOR) || \
        defined(VTSP_ENABLE_GAMRNB_ACCELERATOR) || \
        defined(VTSP_ENABLE_GAMRWB_ACCELERATOR)
/* Include if DSP Accelerator is used */
#include <dsp.h>
#endif

/*
 * RTP Object.
 *
 * This data structure retains information about each xmit session.
 */
typedef struct {
    int                socket;
    OSAL_NetAddress    remoteAddr;
    OSAL_NetAddress    localAddr;
    RTP_Obj            sendRtpObj;
    RTP_Obj            recvRtpObj;
    vint               lastCoder;      /* prev. dyn. coder of any type */
    vint               lastLocalCoder; /* previous local coder */
    vint               lastLocVCoder;  /* prev local vox coder not CN / DTMF */
    vint               framesM1;       /* number of 10ms encoded, minus 1 */
    uint32             lastDtmfTime;   /* timestamp of previous DTMF packet */
    uint32             rtpTime;        /* Audio RTP timestamp in samples */

    uint32             firstRtpTime;    /*
                                         * Audio RTP timestamp of the first audio packet (in samples)
                                         * This value is initialized randomly.
                                         */
    uint32             firstRtpTimeMs;  /*
                                         * OSAL timestamp in milliseconds
                                         * corresponding to the first audio packet.
                                         * Used for calculating RTP timestamp for RTCP SR
                                         */

    vint               payloadOffset;
    VTSP_StreamDir     dir;
    vint               inUse;
    vint               sendActive;
    vint               recvActive;
    vint               tos;
    uint32             tsRandom;        /* Flag indicating audio RTP timestamp is randomly initialized .*/
    uint16             seqRandom;       /* Flag indicating audio RTP seqn is randomly initialized .*/
    /*
     * The following state information is maintained for RTCP data collection.
     */
    uvint              sendPacketCount;
    uvint              sendOctetCount;

    uvint              maxSequence;
    uvint              probation;
    uvint              baseSequence;
    uvint              badSequence;
    uvint              cycles;
    uvint              received;
    uvint              receivedPrior;
    uvint              expectedPrior;

    uint32             receiveTime;
    uint32             lastReceiveTime;
    uint32             lastTimeStamp;
    int32              jitter;

    uint32             lastSR;
    uint32             recvLastSR;
    vint               open;
    OSAL_SelectTimeval ntpTime;         /* Audio RTCP SR - NTP time . */
    uint32             rtcpRtpTime;     /* Audio RTCP SR - RTP time . */
    vint               localDecoder;

    /*
     * There must be room for one complete max buffer.
     * Use RTP_EXTRA_STORAGE for extra room??
     */
    uint8              netBuffer[RTP_BUFSZ_MAX];
    vint               netBufferLen;
} _VTSPR_RtpObject;

typedef struct {
    vint           infc;
    vint           streamId;
    uint32         xSeed;
    uint32         ySeed;
    OSAL_NetAddress remoteAddr;
    OSAL_NetAddress localAddr;
    int32          currentCount;
    int32          sendCount;
    int32          sendCountFixed;
    vint           sendPacketCount;
    uint32         enableMask;
    vint           tos;
} _VTSPR_RtcpObject;

/*
 * The SDES-CNAME packet must be be uint32 aligned. The first word is a header.
 * The second word is the ssrc. The next 2 octets are header followed by up to
 * _VTSP_RTCP_CNAME_SZ * sizeof(uint32) characters. The message is terminated by
 * one or more NULL characters. This forces the maximum message size to be 3
 * larger than the size maxium size of the string.
 */
typedef struct {
    vint    length;
    uint32  cname[_VTSP_RTCP_CNAME_SZ + 3];
} _VTSPR_RtcpCname;


/*
 * Voice Internal block Data
 */
typedef struct {
    /*
     * vtsp-rtp block interface
     * temporary buffers
     */
    uint8              blockToNet[VTSP_PAYLOAD_TO_NET_MAX_SZ];

    _VTSPR_RtpObject   rtpObj[_VTSP_STREAM_NUM];
    _VTSPR_RtcpObject  rtcpObj[_VTSP_STREAM_NUM];
    _VTSPR_RtcpCname   rtcpCname[_VTSP_INFC_NUM];
} VTSPR_NetObj;

/*
 * Generic Multi-frame encoder task message structures
 */
typedef struct {
    vint   cmd;
    uvint  encType;
    vint   infc;
    vint   streamId;
    vint   initFlag;
    vint   msgSize;
    uint32 silenceComp;
    uint32 extension;
    vint   enc_ary[VTSPR_MULTI_NSAMPLE_MAX];
} _VTSPR_MultiRawMsg;

typedef struct {
    vint  msgSize;
    uint8 coded_ary[VTSPR_MAX_CODED_BYTES];
} _VTSPR_MultiCodedMsg;

/*
 * Generic Multi-frame decoder task message structures
 */
typedef struct {
    vint   cmd;
    uvint  decType;
    vint   infc;
    vint   streamId;
    vint   initFlag;
    uint32 extension;
    vint   payloadSize;
    uint8  pkt_ary[VTSPR_MAX_CODED_BYTES];
} _VTSPR_MultiPktMsg;

typedef struct {
    vint   msgSize;
    vint   decoded_ary[VTSPR_MULTI_NSAMPLE_MAX];
} _VTSPR_MultiDecodedMsg;
/*
 * VTSPR Generic Multi-frame coder Objects
 */
typedef struct {
    OSAL_MsgQId     pktData;
    OSAL_MsgQId     decData;
    _VTSPR_MultiPktMsg   pktMsg;
    vint            decMsgSz;
    vint            decOffset;
    vint            plcActive;
    vint            lastFrameSize;
    vint            coderSamples10ms;
    uint8           sidDecoded[VTSPR_MULTI_NSAMPLE_MAX];
    vint            recvSpeech_ary[VTSPR_MULTI_NSAMPLE_MAX];
    vint            playSpeech_ary[VTSPR_MULTI_NSAMPLE_MAX];
#ifdef VTSP_ENABLE_G723
    G723A_DecObj    g723decObj;
#endif
#ifdef VTSP_ENABLE_ILBC
    ILBC_DecObj     iLBCdecObj;
#endif
#ifdef VTSP_ENABLE_G722P1
    G722P1_DecObj   g722p1decObj;
    uint8          *g722p1Scratch_ptr;
#endif
#ifdef VTSP_ENABLE_SILK
    SILK_DecObj     silkDecObj;
#endif
#ifdef VTSP_ENABLE_GAMRNB
    GAMRNB_DecObj   gamrnbDecObj;
#endif
#ifdef VTSP_ENABLE_GAMRWB
    GAMRWB_DecObj   gamrwbDecObj;
#endif
#ifdef VTSP_ENABLE_G711P1_ACCELERATOR
    DSP_Instance    decG711p1Instance;
#endif
#ifdef VTSP_ENABLE_G729_ACCELERATOR
    DSP_Instance    decG729Instance;
#endif
#ifdef VTSP_ENABLE_G726_ACCELERATOR
    DSP_Instance    decG726Instance;
#endif
#ifdef VTSP_ENABLE_GAMRNB_ACCELERATOR
    DSP_Instance    decGamrnbInstance;
#endif
#ifdef VTSP_ENABLE_GAMRWB_ACCELERATOR
    DSP_Instance    decGamrwbInstance;
#endif
} _VTSPR_MultiDecObj;

typedef struct {
    /*
     * The encoder need to accumulate up to 30ms of data before the encoder is
     * called.
     */
    OSAL_MsgQId         rawData;
    OSAL_MsgQId         encData;
    _VTSPR_MultiRawMsg  rawMsg;
    vint                encMsgSz;
    vint                encOffset;
    vint                coderSamples10ms;
#ifdef VTSP_ENABLE_G723
    G723A_EncObj        g723encObj;
#endif
#ifdef VTSP_ENABLE_ILBC
    ILBC_EncObj         iLBCencObj;
#endif
#ifdef VTSP_ENABLE_G722P1
    G722P1_EncObj       g722p1encObj;
    uint8              *g722p1Scratch_ptr;
#endif
#ifdef VTSP_ENABLE_G711P1_ACCELERATOR
    DSP_Instance        encG711p1Instance;
#endif
#ifdef VTSP_ENABLE_G729_ACCELERATOR
    DSP_Instance        encG729Instance;
#endif
#ifdef VTSP_ENABLE_G726_ACCELERATOR
    DSP_Instance        encG726Instance;
#endif
#ifdef VTSP_ENABLE_GAMRNB_ACCELERATOR
    DSP_Instance        encGamrnbInstance;
#endif
#ifdef VTSP_ENABLE_GAMRWB_ACCELERATOR
    DSP_Instance        encGamrwbInstance;
#endif
#ifdef VTSP_ENABLE_GAMRNB
    GAMRNB_EncObj       gamrnbEncObj;
#endif
#ifdef VTSP_ENABLE_GAMRWB
    GAMRWB_EncObj       gamrwbEncObj;
#endif
#ifdef VTSP_ENABLE_SILK
    SILK_EncObj         silkEncObj;
#endif
} _VTSPR_MultiEncObj;

/*
 * Tone sequence object, used to abstract both single tone templates and
 * sequence tones.  Also used for quad (genf).
 * Note: some types are very important, such as tone time, which must be
 * uint32 to match VTSP public and VTSP command msg.
 */
typedef struct {
    uvint   toneEvent;
    uvint   toneEventType;
    vint    toneNumToneIds;
    uint32  toneControl;
    uint32  toneSeqRepeat;
    vint    toneSeqIndex;
    uint32  toneSeqRepeatCnt;
    uint32  tonePreRetVal;
    uint32  toneRetVal;
    uvint   toneIdList[_VTSP_NUM_TONE_SEQUENCE_MAX];
    uint32  toneTime;
    void   *toneObj_ptr;
    uint32  toneEdge;
    uint32  toneDone;
} VTSPR_ToneSeq;

/*
 * VTSPR_Queues
 * --------
 */
typedef struct {
    /*
     * Event msgs to application context
     */
    OSAL_MsgQId   eventGlobalQ;
    OSAL_MsgQId   eventInfcQ[_VTSP_INFC_NUM];
    VTSP_EventMsg eventMsg;              /* Temp msg storage */

    /*
     * Cmds from application context
     */
    OSAL_MsgQId  cmdQ;
    _VTSP_CmdMsg cmdMsg;

    /*
     * VTSP sends on cidQ[infc] for FXS interfaces
     * VTSPR receives on cidQ[infc] for FXS interfaces
     * (For callerId-Send)
     *
     * VTSP receives on cidQ[infc] for FXO interfaces
     * VTSPR sends on cidQ[infc] for FXO interfaces
     * (For callerId-Recv)
     *
     * For FXS: Q direction is VAPP -> vTSP -> VTSP_RT -> infc
     * For FXO: Q direction is VAPP <- vTSP <- VTSP_RT <- infc
     */
    OSAL_MsgQId cidQ[_VTSP_INFC_NUM];

    /*
     * VTSP send unencoded data on dataRaw[]. It receives encoded data on
     * dataEnc[].
     */
    OSAL_MsgQId data10Raw;
    OSAL_MsgQId data10Enc[_VTSP_STREAM_NUM];
    OSAL_MsgQId data20Raw;
    OSAL_MsgQId data20Enc[_VTSP_STREAM_NUM];
    OSAL_MsgQId data30Raw;
    OSAL_MsgQId data30Enc[_VTSP_STREAM_NUM];

    /*
     * Send data packets on dataPkt[]. Receive decoded data on dataDec[]
     */
    OSAL_MsgQId data10Pkt;
    OSAL_MsgQId data10Dec[_VTSP_STREAM_NUM];
    OSAL_MsgQId data20Pkt;
    OSAL_MsgQId data20Dec[_VTSP_STREAM_NUM];
    OSAL_MsgQId data30Pkt;
    OSAL_MsgQId data30Dec[_VTSP_STREAM_NUM];

    OSAL_MsgQId rtcpMsg;
    OSAL_MsgQId rtcpEvent;

    OSAL_MsgQId stunSendQ;
    OSAL_MsgQId stunRecvQ;
} VTSPR_Queues;

/*
 * Stream struct.
 */
typedef struct {
    /*
     * Buffers
     */
    vint streamEncIn_ary[VTSPR_NSAMPLES_10MS_MAX];
    vint streamDecOut_ary[VTSPR_NSAMPLES_STREAM];
    vint streamDec_ary[VTSPR_NSAMPLES_STREAM];
    vint confPeer_ary[VTSPR_NSAMPLES_STREAM];
#ifdef VTSP_ENABLE_STREAM_16K
#ifndef VTSP_ENABLE_MP_LITE
    UDS_Obj udsStreamUp;
    UDS_Obj udsStreamDown;
#endif
#endif

    /* Multi-frame encode and decode messaging objects*/
    _VTSPR_MultiEncObj multiEncObj;
    _VTSPR_MultiDecObj multiDecObj;

    /* Queue Id for 10, 20, and 30ms encoder tasks */
    OSAL_MsgQId        rawData10;
    OSAL_MsgQId        encData10;
    OSAL_MsgQId        rawData20;
    OSAL_MsgQId        encData20;
    OSAL_MsgQId        rawData30;
    OSAL_MsgQId        encData30;

    /* Queue Id for 10, 20, and 30ms decoder task */
    OSAL_MsgQId        pktData10;
    OSAL_MsgQId        decData10;
    OSAL_MsgQId        pktData20;
    OSAL_MsgQId        decData20;
    OSAL_MsgQId        pktData30;
    OSAL_MsgQId        decData30;
    /*
     * Algorithm objects
     */
#if defined(VTSP_ENABLE_G729) && !defined(VTSP_ENABLE_G729_ACCELERATOR)
    G729AB_EncObj g729EncObj;
    G729AB_DecObj g729DecObj;
#endif
#ifdef VTSP_ENABLE_G722
    G722_EncObj   g722EncObj;
    G722_DecObj   g722DecObj;
#endif
#if defined(VTSP_ENABLE_G726) && !defined(VTSP_ENABLE_G726_ACCELERATOR)
    G726_Obj      g726EncObj;
    G726_Obj      g726DecObj;
#endif
#if defined(VTSP_ENABLE_G711P1) && !defined(VTSP_ENABLE_G711P1_ACCELERATOR)
    G711P1_EncObj g711p1EncObj;
    G711P1_DecObj g711p1DecObj;
#endif
#ifndef VTSP_ENABLE_MP_LITE
    PLC_Obj       plcObj;
    TONE_Obj      toneObj;
    NSE_Obj       nseObj;
#endif
#ifdef VTSP_ENABLE_NFE
    NFE_Object   *nfe_ptr; /* NFE from Peer is a part of channel object */
#endif
#ifdef VTSP_ENABLE_AEC
    AEC_Obj      *aec_ptr;
#else
#ifndef VTSP_ENABLE_MP_LITE
    BND_Obj      *bnd_ptr;
#endif
#endif
#ifdef VTSP_ENABLE_T38
    FR38_Obj     *fr38Obj_ptr; /* points to the channel object member */
    vint          fr38Active;
    int16        *fr38Jb_ptr;  /* points to the channel object member */
    int16        *fr38Mdm_ptr;
#endif

#ifdef VTSP_ENABLE_DTMFR
    DR_EncodeObj  drEncodeObj;
    DR_DecodeObj *drDecodeObj_ptr;
#endif
    /*
     * Algorithm state.
     */
    uint32   algStreamState;
    vint     tId;            /* XXX Used, but never set */
    vint     decoderType;
    vint     lastEncoder;
    vint     lastExtension;
    vint     blockLen;
    vint     marker;       /* Set to 1 if currently deleting silence */
    vint     cnPktTime;    /* msec for CN packet generation period */
    vint     cnPower;      /* Last noise floor, used for CN generation */
    vint     framesSent;   /* Set to 0 at start, formerly sampsSent */
    vint     silFrameCnt;  /* Counts silence frames */

    uvint    stunEvent;    /* Set when STUN packet received */
    uint32   lastTimeStamp;
    uint32   lastTimeArrival;

#ifndef VTSP_ENABLE_MP_LITE
    /*
     * Tones for remote peer
     */
    uvint          toneEvent;
    VTSPR_ToneSeq  toneSeq;
#endif
#ifdef VTSP_ENABLE_TONE_QUAD
    VTSPR_ToneSeq  toneQuadSeq;
    GENF_Obj       genfObj;
#endif
    /*
     * JB objects.
     */
    JB_Obj    jbObj;
    JB_Params jbParams;
    /*
     * Debug/development
     */
    uvint countEncode;
    uvint countDecode;
    uvint countNSE;
    uvint countCN;

    struct {
        uvint encodeBytes;
        uvint decodeBytes;
        uvint encodePkt;
        uvint decodePkt;
        uvint encodeCN;
        uvint decodeCN;
        uvint runNSE;
        uvint runPLC;
    } count;

    /*
     * Stream config params.
     */
    VTSP_Stream streamParam;

    vint        curActive;

} VTSPR_StreamObj;

/*
 * Flow Object
 */
typedef struct {
    OSAL_MsgQId        flowFromAppQ;
    OSAL_MsgQId        flowToAppQ;
    uvint              infc;
    uvint              flowId;
    uvint              flowDir;
    uvint              key;
    uvint              lastState;
    uvint              streamId;
    uvint              playState;
    uvint              playControl;
    uvint              playPayloadIndex;
    uvint              playCurrentOffset;
    vint               playLastCoder;
    vint               playSize;
    uint32             playDuration;
    uvint              recState;
    uvint              recControl;
    uvint              recPayloadIndex;
    vint               recCoder;
    vint               recLastCoder;
    uint32             recDuration;
    _VTSP_FlowMsg      playMsg;
    _VTSP_FlowMsg      recMsg;
} _VTSPR_FlowObj;

/*
 * please keep #ifdef OUTSIDE the structure. WinCE/WinMobile won't compile
 * otherwise.
 */
/*
 * UTD Dummy objects.
 * These are used to define max sizeof() for UTD during memory allocs.
 * These typedefs are not used for structure assignment.
 */
#ifdef VTSP_ENABLE_UTD
typedef struct {
    union {
        UTD_Tonedef     _utdToneParam;
        UTD_SITTonedef  _utdSITParam;
    };
} _VTSPR_utdParamUnion;
typedef struct {
        int16 _utdTransInternalParam[UTD_TRANSLATED_ARRAY_INT16_SZ];
} _VTSPR_utdTransDataUnion;
#endif /* end VTSP_ENABLE_UTD */

#ifdef VTSP_ENABLE_CIDS
typedef struct {
    /*
     * CID-Send is only applicable to FXS interfaces
     * Foreign Exchange Station (FXS)
     */
    uvint                  cidsEvent;
    VTSP_EventTypeCallerId cidType;
    CIDS_Obj               cidsObj;
    CIDS_Params           *cidsParam_ptr;
    CIDCWS_Obj             cidcwsObj;
    CIDCWS_Params         *cidcwsParam_ptr;
    FSKS_Obj               fsksObj;
} _VTSPR_CidsObj;
#endif

#ifdef VTSP_ENABLE_CIDR
typedef struct {
    /*
     * XXX The cid may be encapsulated into a CIDR submodule.
     */
    uint8    fskrData_ary[sizeof(_VTSP_CIDData)];
    vint     fskrCnt;
    vint     fskrDetect;
    vint     fskrCsum;
    vint     fskrValid;
    vint     cid2Detect;
    vint     dtmfCidCount;
    vint     dtmfCidEvt;
    vint     inDtmfCid;
    uint8    dtmfData_ary[sizeof(_VTSP_CIDData)];
    vint     casLe;
    vint     casTe;
    vint     cid2count;
    vint     cid2run;
    FSKR_Obj fskrObj;
    CAS_Obj  casObj;
} _VTSPR_CidrObj;
#endif

#ifdef VTSP_ENABLE_UTD
typedef struct {
    vint       utdLe;
    vint       utdTe;
    UTD_Obj    utdObj;
    int16      utdPar0[28];        /* UTD tone structure */
    int16      utdPar1[28];        /* UTD tone structure */
    int16      utdPar2[28];        /* UTD tone structure */
    int16      utdPar3[1];         /* UTD tone structure end */
} _VTSPR_UtdObj;
#endif

#ifdef VTSP_ENABLE_FMTD
typedef struct {
    FMTD_Obj fmtdInfcObj;
    FMTD_Obj fmtdPeerObj;
    vint     fmtdLeInfc;
    vint     fmtdLePeer;
    vint     fmtdTeInfc;
    vint     fmtdTePeer;
    vint     fmtdTypeInfc;
    vint     fmtdTypePeer;
    vint     fmtdPhaseRevInfc;
    vint     fmtdPhaseRevPeer;
    vint     fmtdSilenceTime;
    vint     fmtdSilenceTimeMax;
    vint     fmtdTimeoutEvent;
} _VTSPR_FmtdObj;
#endif

#ifdef VTSP_ENABLE_DTMF
typedef struct {
    DTMF_Obj dtmfObj;
    vint     dtmfLe;
    vint     dtmfTe;
    vint     dtmfDigit;
    uvint    dtmfRemove;
    uvint    earlyCount;
} _VTSPR_DtmfObj;
#endif

#ifdef VTSP_ENABLE_T38
typedef struct {
    /*
     * FR38 objects
     */
    FR38_Obj    fr38Obj;
    /*
     * T.38 Packet object. This is used to store the actual UDPTL payload.
     */
    FR38_T38Obj t38ObjIn;
    FR38_T38Obj t38ObjOut;
    FR38_Status t38Status;

    /*
     * Jitter buffer allocated for one instance of fr38 Object
     */
    int16       fr38Jb[_VTSPR_FR38_JB_SZ];
    vint        fr38Active;
    vint        fr38Event;
    vint        fr38EventLast;
    int16       fr38MdmContext[_VTSPR_FR38V3_MDM_SIZE];
} _VTSPR_FR38Obj;
#endif

#ifdef VTSP_ENABLE_ECSR
typedef struct {
    EC_Obj  ecsrObj;
    NLP_Obj nlpObj;
    vint    ecDly_ary[VTSPR_EC_DLYLEN];
    vint    ecFull_ary[VTSPR_EC_FULLLEN];
    vint    ecRin_ary[VTSPR_NALLIGNMENT_BUFS][VTSPR_NSAMPLES_10MS_MAX];
    vint    nAllignBufs; /* dynamic allignment parameter */
} _VTSPR_EcObj;
#endif

#ifdef VTSP_ENABLE_AEC
typedef struct {
    AEC_Obj aecNearObj;
    vint    aecRin_ary[VTSPR_NALLIGNMENT_BUFS][VTSPR_NSAMPLES_AUDIO];
    vint    aecRout_ary[VTSPR_NALLIGNMENT_BUFS][VTSPR_NSAMPLES_AUDIO];
    vint    aecfilt_ary[AEC_FILTER_BUFSZ_MAX];
    vint    aecRinHist_ary[AEC_HIST_BUFSZ_MAX];
    vint    nAllignBufs; /* dynamic allignment parameter */
} _VTSPR_AecObj;
#endif

/*
 * Foreign Exchange audio Channel struct: FXS + FXO
 * Analog Interface audio channel
 */
typedef struct {
    /*
     * Buffers
     */
    vint    *tx_ptr;
    vint    *rx_ptr;
    vint     audioIn_ary[VTSPR_NSAMPLES_10MS_MAX];
    vint     audioOut_ary[VTSPR_NSAMPLES_10MS_MAX];
    vint     audioToPeer_ary[VTSPR_NSAMPLES_STREAM];
    vint     toneOut_ary[VTSPR_NSAMPLES_10MS_MAX];
    vint     ecSout_ary[VTSPR_NSAMPLES_10MS_MAX];    /* XXX Rename this buf */
    vint     ecNlp_ary[VTSPR_NSAMPLES_10MS_MAX];     /* XXX Rename this buf */
#if defined(VTSP_ENABLE_STREAM_16K) || defined (VTSP_ENABLE_AUDIO_SRC)
#ifndef VTSP_ENABLE_MP_LITE
    UDS_Obj  udsAudioUp;
    UDS_Obj  udsAudioDown;
#endif
#endif

    /*
     * Algorithm states & processing control
     */
    uint32  algChannelState;
    vint    numSamples10ms;
    uint32  ecControl;
    vint    echoCancEvent;
    vint    echoCancEventLast;
    vint    dbRx;
    vint    dbTx;
    int32   gainAudioOut;
    int32   gainAudioIn;

    /*
     * Audio processing algorithms
     */
#ifdef VTSP_ENABLE_NFE
    NFE_Object   nfeInfcObj;
    NFE_Object   nfePeerObj;
#endif
#ifndef VTSP_ENABLE_MP_LITE
    DCRM_Obj     dcrmNearObj;
    DCRM_Obj     dcrmPeerObj;
#endif
#ifdef VTSP_ENABLE_DTMFR
    DR_EventObj  drEventObj;
    DR_DecodeObj drDecodeObj;
#endif
#ifndef VTSP_ENABLE_MP_LITE
    /*
     * Objects for tone generation to the FXS local device
     */
    VTSPR_ToneSeq  toneSeq;
    TONE_Obj       toneObj;
#endif
#ifdef VTSP_ENABLE_TONE_QUAD
    VTSPR_ToneSeq  toneQuadSeq;
    GENF_Obj       toneQuadObj;
#endif

    /* Data for Caller ID Send */
    _VTSP_CIDData  cidsData;

    /*
     * Interface-specific objects for run-time allocation
     * If the interface does not have the object allocated
     * (for example FXS will not have UtdObj allocated)
     * then the ptr will be NULL.
     */
#ifdef VTSP_ENABLE_CIDS
    _VTSPR_CidsObj *cids_ptr;
#endif
#ifdef VTSP_ENABLE_CIDR
    _VTSPR_CidrObj *cidr_ptr;
#endif
#ifdef VTSP_ENABLE_UTD
    _VTSPR_UtdObj  *utd_ptr;
#endif
#ifdef VTSP_ENABLE_FMTD
    _VTSPR_FmtdObj *fmtd_ptr;
#endif
#ifdef VTSP_ENABLE_DTMF
    _VTSPR_DtmfObj *dtmf_ptr;
#endif
#ifdef VTSP_ENABLE_ECSR
    _VTSPR_EcObj   *ec_ptr;
#endif
#ifdef VTSP_ENABLE_AEC
    _VTSPR_AecObj  *aec_ptr;
#else
#ifndef VTSP_ENABLE_MP_LITE
    BND_Obj        *bndNear_ptr;
    BND_Obj        *bndPeer_ptr;
#endif
#endif
#ifdef VTSP_ENABLE_T38
    _VTSPR_FR38Obj *fr38_ptr;
#endif

    vint            curActive;

} VTSPR_ChanObj;


/*
 * FXS Physical Infc struct
 */
typedef struct {
    /*
     * Hardware Interface
     */
    TIC_Obj  fxs;
    vint     ringEvent;
    vint     ringTime;
    vint     ringCountMax;
    vint     ringNum;
    vint     state;
    vint     flashEvent;
    vint     hookEvent;
    vint     pulseEvent;
    vint     keypadEvent;
    vint     battery;

#ifdef VTSP_ENABLE_PSTN
    /* PSTN events - valid for FXS that connects to PSTN relay only
     */
    vint     pstnRingEvent;
    vint     pstnHookEvent;
#endif

    /* ring cadence to use for ring */
    _VTSP_RingTemplate *ringTemplate_ptr;
} VTSPR_FxsInfcObj;


/*
 * FXO Physical Infc struct
 */
typedef struct {
    /*
     * Hardware Interface
     */
    TIC_Obj  fxo;
    vint     state;
    vint     ringEdge;
    vint     ringNum;
    vint     flashEvent;
    vint     hookEvent;
    vint     battPol;
    vint     disconEvent;
    vint     pulseEvent;
} VTSPR_FxoInfcObj;

/*
 * Audio Physical Infc struct
 */
typedef struct {
    /*
     * Hardware Interface
     */
    TIC_Obj  audio;

    /* XXX does audio need these object members? */
    vint     ringEvent;
    vint     ringTime;
    vint     ringCountMax;
    vint     ringNum;
    vint     state;
    vint     flashEvent;
    vint     hookEvent;
    vint     pulseEvent;
    vint     keypadEvent;
    vint     battery;
    vint     attachState;
} VTSPR_AudioInfcObj;

/*
 * DSP struct
 */
typedef struct {
    /*
     * Time management
     */
    uvint heartbeat;
    uvint heartbeat1s;
    vint  tick10ms;
    vint  tick1ms;
    vint  tick1Mhz;
    vint  measure;
    int32 measureAvg;
    int32 measureHi;
    uint32  measureStart;
    uint32  measureStop;
    uint32  measureSum;
    int32   procTickAvg;
    int32   procTickHi;
    int32   measureCount;

    /*
     * Physical Hardware Infc
     */
#if _VTSP_INFC_FXS_NUM > 0
    VTSPR_FxsInfcObj    fxsInfc[_VTSP_INFC_FXS_NUM];
#endif
#if _VTSP_INFC_FXO_NUM > 0
    VTSPR_FxoInfcObj    fxoInfc[_VTSP_INFC_FXO_NUM];
#endif
#if _VTSP_INFC_AUDIO_NUM > 0
    VTSPR_AudioInfcObj  audioInfc[_VTSP_INFC_AUDIO_NUM];
#endif

    /*
     * Foreign Exchange Audio Channel objects.
     * Contains all channels: FXS, AUDIO, and FXO.
     */
    VTSPR_ChanObj       chan[_VTSP_INFC_NUM];

    /*
     * Streams
     */
    VTSPR_StreamObj     *streamObj_ptr[_VTSP_STREAM_NUM];

    /* Interface/Stream Limiting Variables */
    vint                curActiveInfcs;
    vint                curActiveStreams;

    /* CN power attenuation variable */
    vint                cnPwrAttenDb;

    /*
     * Flows
     */
    _VTSPR_FlowObj      flowObj[_VTSP_FLOW_NUM];

    /*
     * Temp buffers common to all channels. Scratch packet space.
     */
    JB_Pkt              jbPkt;
    vint                confLocal_ary[VTSPR_NSAMPLES_STREAM];
#ifdef VTSP_ENABLE_G722P1
    uint8               g722p1EncScratch_ary[G722P1_SCRATCH_NBYTES];
    uint8               g722p1DecScratch_ary[G722P1_SCRATCH_NBYTES];
#endif
    vint                faxSilRxDB;
    vint                faxSilTxDB;
    /*
     * Jitter buffer pointers for the JB_OBj on each stream
     */
    JB_Pkt             *jbBuffer_ptr[_VTSP_STREAM_NUM];

    /*
     * Global and local params for algs.
     */
    GLOBAL_Params      *globals_ptr;
    vint                fmtdGlobalPowerMinInfc;
    vint                fmtdGlobalPowerMinPeer;
    vint               *dbTable_ptr;
#ifdef VTSP_ENABLE_NFE
    NFE_Local          *nfeParams_ptr;
#endif
#ifndef VTSP_ENABLE_MP_LITE
    DCRM_Params        *dcrmNearParams_ptr;
    DCRM_Params        *dcrmPeerParams_ptr;
#endif
#ifdef VTSP_ENABLE_ECSR
    EC_Params          *ecsrParams_ptr;
    NLP_Params         *nlpParams_ptr;
#endif
#ifdef VTSP_ENABLE_AEC
    AEC_Params         *aecNearParams_ptr;
#else
#ifndef VTSP_ENABLE_MP_LITE
    BND_Params         *bndParams_ptr;
#endif
#endif
#ifdef VTSP_ENABLE_DTMF
    DTMF_Params        *dtmfParams_ptr;
#endif
#ifdef VTSP_ENABLE_FMTD
    FMTD_Params        *fmtdParams_ptr;
#endif
#ifdef VTSP_ENABLE_CIDR
    FSKR_Params        *fskrParams_ptr;
#endif
#ifdef VTSP_ENABLE_T38
    FR38_Params        *fr38Params_ptr;
    FR38V3_GLOBAL_Params *fr38Global_ptr;
#endif
    JB_Params          *jbDefParams_ptr;
#ifndef VTSP_ENABLE_MP_LITE
    NSE_Params         *nseParams_ptr;
    TONE_Params        *dtmfToneParams_ptr;
    TONE_Params        *toneParams_ptr[_VTSP_NUM_TONE_TEMPL + 1]; /* Many */
#endif
#ifdef VTSP_ENABLE_TONE_QUAD
    GENF_Params        *toneQuadParams_ptr[_VTSP_NUM_TONE_TEMPL]; /* Many */
#endif
    _VTSP_RingTemplate *ringTemplate_ptr[_VTSP_NUM_RING_TEMPL];
#ifdef VTSP_ENABLE_UTD
    vint                utdParamNum;
    void               *utdParams_ptr[_VTSP_NUM_UTD_TEMPL];
    void               *utdTransData_ptr[_VTSP_NUM_UTD_TEMPL];
    int16              *utdTransTable_ptr[_VTSP_NUM_UTD_TEMPL];
    int16               utdZero[1];
#endif

} VTSPR_DSP;

#ifdef VTSP_ENABLE_NETLOG
typedef struct {
#define VTSPR_NETLOG_NUM          (40)
#define VTSPR_NETLOG_SENDBUF_NUM  (2)
#define VTSPR_NETLOG_SENDBUF_SZ   (4*VTSPR_MULTI_NSAMPLE_MAX + 12)
    int                 socket_ary[VTSPR_NETLOG_NUM];
    uint16              sendPort_ary[VTSPR_NETLOG_NUM];
    uint32              sendAddr_ary[VTSPR_NETLOG_NUM];
    uint32              ssrc_ary[VTSPR_NETLOG_NUM];
    uint32              seq_ary[VTSPR_NETLOG_NUM];
    uint32              ts_ary[VTSPR_NETLOG_NUM];
    /* Buffering for network output */
    uint32              sendBufSock[VTSPR_NETLOG_SENDBUF_NUM];
    uint32              sendBufLen[VTSPR_NETLOG_SENDBUF_NUM];
    uint32              sendBufIndex;
    uint8              *sendBufList[VTSPR_NETLOG_SENDBUF_NUM];
    OSAL_NetAddress     sendBufAddr[VTSPR_NETLOG_SENDBUF_NUM];
    vint                enable;
    uint32              remoteIP;
} VTSPR_Netlog;
#endif

#ifdef VTSP_ENABLE_BENCHMARK
typedef struct {
    vint   ctr;
    uint32 measureStart[_VTSPR_BENCHMARK_NUM];
    uint32 measureStop[_VTSPR_BENCHMARK_NUM];
    uint32 measureSum[_VTSPR_BENCHMARK_NUM];
    uint32 measureTicAvg[_VTSPR_BENCHMARK_NUM];
    uint32 measureTicHi[_VTSPR_BENCHMARK_NUM];
    uint32 measureTicPeak[_VTSPR_BENCHMARK_NUM];
} VTSPR_Benchmark;
#endif

typedef struct {
    VTSPR_Queues      *q_ptr;
    VTSPR_DSP         *dsp_ptr;
#ifdef VTSP_ENABLE_BENCHMARK
    VTSPR_Benchmark   *benchmark_ptr;
#endif
    OSAL_TaskId        taskId;
    uvint              taskEnable;
    uvint              stackSize;
    uvint              taskPriority;
    OSAL_SemId         finishSemId;
} VTSPR_TaskContext;

/*
 * Main object for VTSPR
 */
typedef struct {
    VTSPR_Queues      *q_ptr;
    VTSPR_DSP         *dsp_ptr;
    VTSPR_NetObj      *net_ptr;
    vint               pldRev;

    /* VTSPR main 10ms task */
    VTSPR_TaskContext  task10ms;

    /* Multi encode task contexts */
    VTSPR_TaskContext  multiEncTask10ms;
    VTSPR_TaskContext  multiEncTask20ms;
    VTSPR_TaskContext  multiEncTask30ms;

    /* Multi decode task contexts */
    VTSPR_TaskContext  multiDecTask10ms;
    VTSPR_TaskContext  multiDecTask20ms;
    VTSPR_TaskContext  multiDecTask30ms;

    uvint              dbMask;
#ifdef VTSP_ENABLE_NETLOG
    VTSPR_Netlog      *netlog_ptr;
#endif

#ifdef VTSP_ENABLE_BENCHMARK
    VTSPR_Benchmark   *benchmark_ptr;
#endif
#ifdef VTSPR_ENABLE_AUTOSTART
    vint               autoStartCmd;
#endif
    uint32 bitrate;
} VTSPR_Obj;

#endif

