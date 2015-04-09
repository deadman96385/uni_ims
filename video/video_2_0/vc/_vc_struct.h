/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004-2010 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 12523 $ $Date: 2010-07-13 18:57:44 -0400 (Tue, 13 Jul 2010) $
 */

#ifndef _VC_STRUCT_H_
#define _VC_STRUCT_H_

#include <osal.h>
#include <jbv.h>
#include <vtsp.h>
#include <vci.h>
#include "../../ve/vtsp/vtsp_private/_vtsp_private.h"
#include "_vc_const.h"

/* A structure for containing all fields of the RTP object that will be read by RTCP */
typedef struct {
    OSAL_SemId         mutexLock;
    uvint              cycles;
    uvint              maxSequence;
    uvint              baseSequence;
    uvint              received;
    uint32             firstRtpTime;    /*
                                         * Video RTP timestamp of the first video frame (in samples)
                                         * This value is initialized randomly.
                                         */
    uint32             firstRtpTimeMs;  /*
                                         * OSAL timestamp in milliseconds
                                         * corresponding to the first Video Frame.
                                         * Used for calculating RTP timestamp for RTCP SR
                                         */
    /*
     * The following state information is maintained for RTCP data collection.
     */
    uvint              sendPacketCount;
    uvint              sendOctetCount;
} _VC_RtpRtcpInfoObject;

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
    vint               lastCoder;       /* prev. dyn. coder of any type */
    vint               lastLocalCoder;  /* previous local coder */
    vint               lastLocVCoder;   /* prev local vox coder not CN / DTMF */
    uint64             tsMs;            /* Video Frame timestamp in milliseconds */
    uint32             rtpTime;         /* Video RTP timestamp in samples */

    vint               payloadOffset;
    vint               inUse;
    vint               sendActive;
    vint               recvActive;
    vint               tos;
    uint32             tsRandom;    /* Flag indicating Video RTP timestamp is randomly initialized .*/
    uint16             seqRandom;   /* Flag indicating Video RTP seqn is randomly initialized .*/
    uvint              probation;
    uvint              badSequence;
    vint               open;
    /*
     * There must be room for one complete max buffer.
     * Use RTP_EXTRA_STORAGE for extra room??
     */
    uint8              netBuffer[RTP_BUFSZ_MAX];
    uint8              fromNetBuffer[RTP_BUFSZ_MAX];
    vint               netBufferLen;

    _VC_RtpRtcpInfoObject   info;
} _VC_RtpObject;


/* Information used for RTCP that MUST NOT be written to from outside the RTCP send/receive task. */
typedef struct {
    /* Boolean indicating that application has requested an FIR to be sent and a
     * response has not yet been received. In this case, sending another FIR is a
     * 'repetition' and is forbidden from increasing the sequence number. */
    vint               firWaiting;
    /* A counter for the number of (non repeated) FIR that have been sent. This
     * is used as the sequence number. */
    uint8              firSeqNumber;
    /* The time when the most recent FIR that has been acted upon was received. */
    VTSP_NtpTime       lastFir;
    /* The time when the most recent PLI that has been acted upon was received. */
    VTSP_NtpTime       lastPli;

    /* The following variables are for TMMBR/TMMBN feature. */

    /* bitrate value in kbps that was last sent in TMMBN. */
    uint32  sendTmmbnInKbps;
    /* bitrate value in kbps that was last sent in TMMBN. */
    uint32  recvTmmbnInKbps;
    /* MBR value in kbps that was last sent in TMMBR. */
    uint32  sendTmmbrInKbps;
    /* MBR value in kbps that was last sent in TMMBR. */
    uint32  recvTmmbrInKbps;

    /*
     * Variable that keeps track of TMMBR state.
     */
    vint    tmmbrState;
    vint    tmmbrSendFailCount;
    /* Accumulator for excepted packets in every rtcp reporting interval. */
    uint16  expectedPacketTotal;
    /* Accumulator for lost packets in every rtcp reporting interval. */
    uint16  lostPacketTotal;
} _VC_RtcpFeedback;


/* Information used for RTCP that may be safely written to from outside
 * of the RTCP send/receive task. */
typedef struct {
    /*
     * The reduced minimum RTCP interval in milli seconds
     * RFC 3550 Section 6.2
     * Rtcp reduced mininum Interval  = 360,000 / sessionBandwidthInkbps.
     */
    vint            reducedMinIntervalMillis;
    /* Mask to keep track of RTCP messages that are allowed to be sent. */
    uint32          enableMask;
    /* Mask to keep track of RTCP messages that should be sent at the next available time slot. */
    uint32          rtcpFeedbackSendMask;
} _VC_RtcpConfigure;


typedef struct {
    vint           infc;
    vint           streamId;
    OSAL_NetAddress remoteAddr;
    OSAL_NetAddress localAddr;
    int32          currentCount;
    int32          sendCountFixed;
    vint           sendPacketCount;
    vint           rtpSendPacketCount;
    vint           rtpSendOctetCount;
    uint32         enableMask;
    vint           tos;
    uint32         ssrc;       /* The SSRC of the RTP packets being transmitted to the remote end */
    VTSP_NtpTime   roundTripTime;   /* Sometimes called RTT, 'Round Trip Delay', or RTD */

    /* Information about incoming RTP packets */
    uint32  mediaSsrc;      /* The SSRC of the RTP packets being received from the remote end */
    uvint   expectedPrior;  /* For fraction loss calculations as per RFC 3550 A3 */
    uvint   receivedPrior;  /* For fraction loss calculations as per RFC 3550 A3 */
    vint    lost;           /* The total number of packets lost since the previously sent RTCP packet(s). */
    uint8   fracLost;       /* The fraction of packets lost, in the format needed for RTCP RR blocks */
    uint32  extendedSeqn;   /* Extended highest sequence number of RTP packets received */
    vint    lostAtPriorPli; /* The number of frames that were lost when the previous PLI was sent. If no
                             * PLI has been sent since the last I-Frame was received, this value is 0. */
    vint    keyFramesRecv;  /* Total number of key frames received over the course of this call */

    /* Information about incoming RTCP packets */
    uint32              lastSR;     /* LSR parse directly from last incoming RTCP SR */
    uint32              recvLastSR; /* Time when the lastSR was parsed (in the same units as lastSR) */


    OSAL_SelectTimeval  ntpTime;     /* Video RTCP SR - NTP time. */
    uint32              rtpTime; /* Video RTCP SR - RTP time. */

    /* Parameters for configuring the sending of RTCP. This may be safely written to from any thread. */
    _VC_RtcpConfigure       configure;
    /* Structure to keep track of RTCP feedback related parameters. */
    _VC_RtcpFeedback        feedback;
    /* Local - Video RTP Session bandwidth in kbps - AS bandwidth parameter. */
    uint32          localVideoAsBwKbps;
} _VC_RtcpObject;

/*
 * The  CNAME: Canonical End-Point Identifier SDES Item (RFC 3550 Section 6.5.1)
 * must be uint32 aligned.
 * The max characters allowed in CNAME is _VTSP_RTCP_CNAME_SZ * sizeof(uint32) / size(char).
 * The message is terminated by one or more (upto 3) NULL characters to maintain uint32 alignment.
 */
typedef struct {
    vint    length; /* Length of CNAME SDES item in 32-bit words. */
    uint32  cname[_VTSP_RTCP_CNAME_SZ + 3];
} _VC_RtcpCname;

/*
 * Video Internal block Data
 */
typedef struct {
    _VC_RtpObject   rtpObj[_VTSP_STREAM_NUM];
    _VC_RtcpObject  rtcpObj[_VTSP_STREAM_NUM];
    _VC_RtcpCname   rtcpCname;
} _VC_NetObj;

typedef struct {
    VC_Event   event;
    vint       codec;
    char       eventDesc[VCI_EVENT_DESC_STRING_SZ + 1];
} _VC_AppEventMsg;

typedef struct {
    OSAL_NetAddress local;
    OSAL_NetAddress remote;
} _VC_AddresPair;

typedef struct {
    _VC_CmdCode cmd;
    uint16      streamId;
    union {
        uint32              feedbackMask;
        _VC_AddresPair      address;
        _VTSP_CmdMsgConfig  config;
    };
} _VC_RtcpCmdMsg;

/*
 * VC_Queues
 * --------
 */
typedef struct {
    /*
     * Event msgs to VTSP
     */
    OSAL_MsgQId       eventQ;
    VTSP_EventMsg     eventMsg;              /* Temp msg storage */

    /*
     * Cmds from VTSP
     */
    OSAL_MsgQId       cmdQ;
    _VTSP_CmdMsg      cmdMsg;
    OSAL_MsgQId       rtcpMsg;    /* Outgoing packet from the RTCP task */
    OSAL_MsgQId       rtcpEvent;  /* Incoming packet to the RTCP task */
    OSAL_MsgQId       rtcpCmdQ;   /* Command message queue to control the RTCP task */
    /*
     * Event msgs to APP (Java Video Codec Engine - VCE)
     */
    OSAL_MsgQId       appEventQ;
} _VC_Queues;

/*
 * Stream struct.
 */
typedef struct {
    /*
     * Algorithm state.
     */
    uint32               algStreamState;

    /*
     * Stream config params.
     */
    VTSP_StreamVideo     streamParam;
    vint                 curActive;
    vint                 rtcpEnable;

    /*
     * Only encoder task can write to these outside of sem
     */
    struct {
        Video_EncObj     VideoEncObj;
        vint             encRunning;
        vint             marker;
        vint             requestedWidth;
        vint             requestedHeight;
    } enc;


    /*
     * Only decoder task can write to these outside of sem
     */
    struct {
        JBV_Obj          jbObj;
        vint             decRunning;
    } dec;

} _VC_StreamObj;

/*
 * DSP struct
 */
typedef struct {
    /*
     * Streams
     */
    _VC_StreamObj     *streamObj_ptr[_VTSP_STREAM_NUM];

    /* Interface/Stream Limiting Variables */
    vint               curActiveInfcs;
    vint               curActiveStreams;

    JBV_Pkt            jbPutPkt;
    JBV_Pkt            jbGetPkt;
} _VC_Dsp;

typedef struct {
    OSAL_TaskId      taskId;
    uvint            stackSize;
    uvint            taskPriority;
    void            *func_ptr;
    int8             name[16];
    void            *arg_ptr;
} _VC_TaskObj;

/*
 * Main object for VC
 */
typedef struct {
    _VC_Queues      *q_ptr;
    _VC_Dsp         *dsp_ptr;
    _VC_NetObj      *net_ptr;

    /* Video controller tasks */
    _VC_TaskObj      taskMain;
    _VC_TaskObj      taskRtp;
    _VC_TaskObj      taskRtcp;

    /* RTP Encoder and Decoder bitrates. */
    uint32           bitrateEnc;
    uint32           bitrateDec;

    /* Video Controller Init flag. */
    vint             vcInit;

} _VC_Obj;

#endif

