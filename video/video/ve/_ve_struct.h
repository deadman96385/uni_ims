/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004-2010 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 12523 $ $Date: 2010-07-13 18:57:44 -0400 (Tue, 13 Jul 2010) $
 */

#ifndef _VE_STRUCT_H_
#define _VE_STRUCT_H_

#include <osal.h>
#include <rtp.h>
#include <vtsp.h>
#include "../../ve/vtsp/vtsp_private/_vtsp_private.h"
#include <video.h>
//#include <vdd.h>
//#include <vcd.h>

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
    uint32             rtpTime;        /* wallclock time, for rtp timestamp */
    vint               payloadOffset;
    VTSP_StreamDir     dir;
    vint               inUse;
    vint               sendActive;
    vint               recvActive;
    vint               duration;
    vint               lastPayloadOffset;
    vint               tos;
    uint32             tsRandom;
    uint16             seqRandom;
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

    /*
     * There must be room for one complete max buffer.
     * Use RTP_EXTRA_STORAGE for extra room??
     */
    uint8              netBuffer[RTP_BUFSZ_MAX];
    uint8              fromNetBuffer[RTP_BUFSZ_MAX];
    vint               netBufferLen;

} _VE_RtpObject;

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
} _VE_RtcpObject;

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
} _VE_RtcpCname;


/*
 * Voice Internal block Data
 */
typedef struct {
    _VE_RtpObject   rtpObj[_VTSP_STREAM_NUM];
    _VE_RtcpObject  rtcpObj[_VTSP_STREAM_NUM];
    _VE_RtcpCname   rtcpCname;
} _VE_NetObj;

/*
 * VE_Queues
 * --------
 */
typedef struct {
    /*
     * Event msgs to application context
     */
    OSAL_MsgQId   eventQ;
    VTSP_EventMsg eventMsg;              /* Temp msg storage */

    /*
     * Cmds from application context
     */
    OSAL_MsgQId  cmdQ;
    _VTSP_CmdMsg cmdMsg;

    OSAL_MsgQId rtcpMsg;
    OSAL_MsgQId rtcpEvent;

    OSAL_MsgQId stunSendQ;
    OSAL_MsgQId stunRecvQ;
} _VE_Queues;

/*
 * Stream struct.
 */
typedef struct {
    /*
     * Buffers
     */

    /*
     * Algorithm objects
     */
    /*
     * Algorithm state.
     */
    uint32               algStreamState;

    /*
     * Stream config params.
     */
    VTSP_StreamVideo     streamParam;
    
    vint                 curActive;

    /*
     * Only encoder task can write to these outside of sem
     */
    struct {
        H263_EncObj      h263EncObj;
        H264_EncObj      h264EncObj;
        vint             encInited;
        Video_Picture    streamEncPic;
        uvint            countEncode;
        uvint            encodeBytes;
        uvint            encodePkt;
        vint             marker;
        vint             lastEncoder;
        vint             requestKey;
        vint             requestedWidth;
        vint             requestedHeight;
    } enc;

    /*
     * Only decoder task can write to these outside of sem
     */
    struct {
        H263_DecObj      h263DecObj;
        H264_DecObj      h264DecObj;
        vint             decInited;
        Video_Picture    streamDecPic;
        uvint            countDecode;
        uvint            decodeBytes;
        uvint            decodePkt;
        vint             decoderType;
        uvint            stunEvent;
        vint             width;
        vint             height;
        JBV_Obj          jbObj;
    } dec;
    
} _VE_StreamObj;


/*
 * DSP struct
 */
typedef struct {

    /*
     * Streams
     */
    _VE_StreamObj     *streamObj_ptr[_VTSP_STREAM_NUM];
    
    /* Interface/Stream Limiting Variables */
    vint               curActiveInfcs;
    vint               curActiveStreams;

    H263_EncParams     h263EncParams;
    H263_DecParams     h263DecParams;
    H264_EncParams     h264EncParams;
    H264_DecParams     h264DecParams;
    JBV_Params         jbParams;
    JBV_Pkt            jbPkt;
    Video_Picture      pic;
} _VE_Dsp;

typedef struct {
    _VE_Queues      *q_ptr;
    _VE_Dsp         *dsp_ptr;
    OSAL_TaskId      taskId;
    uvint            taskEnable;
    uvint            stackSize;
    uvint            taskPriority;
    OSAL_SemId       finishSemId;
} _VE_TaskContext;

/* 
 * Main object for VE
 */
typedef struct {
    _VE_Queues      *q_ptr;
    _VE_Dsp         *dsp_ptr;
    _VE_NetObj      *net_ptr;

    /* VE main task (also decodes) */
    _VE_TaskContext  taskMain;

    /* Encode task contexts */
    _VE_TaskContext  taskEncode;

    uint32           bitrateEnc;
    uint32           bitrateDec;
    OSAL_SemId       encStopSem;
    JBV_Pkt          jbPkt;
} _VE_Obj;

#endif

