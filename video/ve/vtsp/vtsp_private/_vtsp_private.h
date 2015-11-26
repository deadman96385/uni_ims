/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004-2010 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 29711 $ $Date: 2014-11-06 12:42:22 +0800 (Thu, 06 Nov 2014) $
 *
 */

#ifndef _VTSP_PRIVATE_H_
#define _VTSP_PRIVATE_H_

#include <osal.h>
#include "../../../include_ac702/tic.h"
#include "vtsp.h"

/*
 * --------
 *  Debug Macros
 * --------
 */

#ifdef VTSP_ENABLE_TRACE
#define _VTSP_TRACE(x,y)    \
        OSAL_logMsg("%s:%d\n", x, y);
#else
#define _VTSP_TRACE(x,y) 
#endif

/* 
 * Runtime macros
 */
#define _VTSP_VERIFY_INIT                   \
        if (NULL == _VTSP_object_ptr) {     \
            return (VTSP_E_INIT);           \
        }

#define _VTSP_VERIFY_STARTED                \
        if (VTSP_OK != _VTSP_object_ptr->start) {     \
            return (VTSP_E_INIT);                     \
        }

/*
 * Delta between epoch time(00:00 GMT, Jan 1, 1970) and
 * ntp time(00:00 UTC Jan 1, 1900). This value fits in
 * an unsigned 32 bit int, but not a signed int.
 */
#define _VTSP_EPOCH_TO_NTPTIME        (2208988800U)

#define _VTSP_SEC_TO_NTP_MSW(X) \
    ( X + ((uint32) _VTSP_EPOCH_TO_NTPTIME) )
#define _VTSP_USEC_TO_NTP_LSW(X) \
    ( (uint32) ( ( ( (uint64) (X) ) << 32) / 1000000) )

#define _VTSP_NTP_MSW_TO_SEC(X) \
        ( X - ((uint32) _VTSP_EPOCH_TO_NTPTIME) )
#define _VTSP_NTP_LSW_TO_USEC(X) \
    ( (uint32) ( ( ( (uint64) (X) ) * 1000000 ) >> 32) )

#define _VTSP_NTP_64_TO_32(MSW, LSW) \
    ( (MSW << 16) | (LSW >> 16) )

#define _VTSP_NTP_32_TO_MSEC(X) ( (uint32) ( ( ( (uint64) (X) ) * 1000 ) >> 16) )

/*
 * --------
 *  Private
 * --------
 */
#define _VTSP_INFC_FXS_NUM       (TIC_INFC_FXS_NUM)     
#define _VTSP_INFC_FXO_NUM       (TIC_INFC_FXO_NUM) 
#define _VTSP_INFC_AUDIO_NUM     (TIC_INFC_AUDIO_NUM)      
#define _VTSP_INFC_NUM           (TIC_INFC_NUM)       
#define _VTSP_INFC_FXS_FIRST     (0)
#define _VTSP_INFC_FXS_LAST      (_VTSP_INFC_FXS_NUM - 1)
#define _VTSP_INFC_FXO_FIRST     (_VTSP_INFC_FXS_NUM)
#define _VTSP_INFC_FXO_LAST      (_VTSP_INFC_FXS_NUM + _VTSP_INFC_FXO_NUM - 1)
#define _VTSP_INFC_AUDIO_FIRST   (_VTSP_INFC_FXS_NUM + _VTSP_INFC_FXO_NUM)
#define _VTSP_INFC_AUDIO_LAST    (_VTSP_INFC_NUM - 1)

#define _VTSP_INFC_FXO_FLASH_MS  (500)      /* XXX Replace with TIC const */

/* FXS infc# that connects to FXS<->PSTN Relay */
#define _VTSP_INFC_FXS_PSTN      (0)     


#define _VTSP_NUM_TONE_TEMPL        (30)
#define _VTSP_NUM_TONE_SEQUENCE_MAX (30)
#define _VTSP_NUM_RING_TEMPL        (10)
#define _VTSP_NUM_UTD_TEMPL         (15)    /* XXX Must match UTD alg */

#define _VTSP_STREAM_PER_INFC   (3)
#define _VTSP_STREAM_NUM        (_VTSP_STREAM_PER_INFC * _VTSP_INFC_NUM)

/* # tone generators per infc */
#define _VTSP_TONE_GEN_PER_INFC (3) 

/*
 * Define the maximum number of simultaneous iLBC enabled streams.
 */
#if defined(VTSP_ENABLE_ILBC)
#define _VTSP_ILBC_ENCODERS_MAX (1)
#else
#define _VTSP_ILBC_ENCODERS_MAX (0)
#endif

/*
 * Define the maximum number of simultaneous G723 enabled streams.
 */
#if defined(VTSP_ENABLE_G723)
#define _VTSP_G723_ENCODERS_MAX (3)
#else
#define _VTSP_G723_ENCODERS_MAX (0)
#endif

/*
 * XXX: Added constants for creating a unique deviceId to talk between user and 
 * kernel space in linux.
 */
#define _VTSP_Q_ID_GLOBAL_EVENT             (0)
#define _VTSP_Q_ID_CMD                      (1)
#define _VTSP_Q_ID_INFC_EVENT(infc)         (2  + infc)
#define _VTSP_Q_ID_INFC_CID(infc)           (10 + infc)
#define _VTSP_Q_ID_FLOW_TO_NET(flowIndex)   (30 + flowIndex) 
#define _VTSP_Q_ID_FLOW_TO_INFC(flowIndex)  (40 + flowIndex) 
#define _VTSP_Q_ID_RTCP_MSG                 (50)
#define _VTSP_Q_ID_RTCP_EVENT               (51)
#define _VTSP_Q_ID_STUN_SEND                (20) 
#define _VTSP_Q_ID_STUN_RECV                (21) 

#define _VTSP_TASK_PRIORITY                 (OSAL_TASK_PRIO_VTSPR)
#define _VTSP_TASK_RTCP_MSG_PRIORITY        (OSAL_TASK_PRIO_NRT)
#define _VTSP_TASK_RTCP_SKT_PRIORITY        (OSAL_TASK_PRIO_NRT)

/*
 * These constants set task parameters for RTCP related tasks.
 */
#define _VTSP_TASK_RTCP_MSG_STACKSIZE       (OSAL_STACK_SZ_LARGE)
#define _VTSP_TASK_RTCP_SKT_STACKSIZE       (OSAL_STACK_SZ_LARGE)
#define _VTSP_TASK_RTCP_CONTROL_PORT        (8000)

/*
 * These constants are related to flows. The first two are number of flows per
 * interface and the second is the total number of flows.
 *
 * The last two are the number of messages in a flow queue and the size of each
 * message. Each message contains multiple voice blocks and a header.
 */
#define _VTSP_FLOW_PER_INFC      (_VTSP_STREAM_PER_INFC)
#define _VTSP_FLOW_NUM           (_VTSP_STREAM_NUM)
#define _VTSP_Q_FLOW_GET_NUM_MSG (6)
#define _VTSP_Q_FLOW_PUT_NUM_MSG (6)
#define _VTSP_Q_FLOW_PUT_MASK    (0x7)
#if (_VTSP_Q_FLOW_PUT_NUM_MSG >= _VTSP_Q_FLOW_PUT_MASK)
#error "FLOW: NUMBER OF FLOWS MUST BE >= THAN THE MASK"
#endif
#define _VTSP_Q_FLOW_PAYLOAD_SZ  (5 * VTSP_BLOCK_MAX_SZ)


/* 
 * CN Attenuation Config boundary checks 
 */
#define _VTSP_CONFIG_CN_PWR_ATN_MIN   (0)
#define _VTSP_CONFIG_CN_PWR_ATN_MAX   (-35)

/*
 * RTCP command from VTSPR to network task.
 */
#define _VTSP_RTCP_CMD_CLOSE    (0)
#define _VTSP_RTCP_CMD_OPEN     (1)
#define _VTSP_RTCP_CMD_SEND     (2)
#define _VTSP_RTCP_CMD_SHUTDOWN (3)

/*
 * Define the maximum length of a RTCP CNAME. The CNAME is packet into an array
 * of uvints for performance reasons.
 */
#define _VTSP_RTCP_CNAME_SZ         (29)
#define _VTSP_RTCP_CNAME_CHARS      (_VTSP_RTCP_CNAME_SZ * sizeof(uvint))

/*
 * Define the maximum length of each supported message. 
 */
#define _VTSP_RTCP_MSG_RR_SZ   (6 + 2)
#define _VTSP_RTCP_MSG_SR_SZ   (_VTSP_RTCP_MSG_RR_SZ + 5)
#define _VTSP_RTCP_MSG_XR_SZ   (2 + 8 + 9) /* XR + MR + SS */
#define _VTSP_RTCP_MSG_SDES_SZ (_VTSP_RTCP_CNAME_SZ + 3)
#define _VTSP_RTCP_MSG_BYE_SZ  (2)

/* This limits the maximum number of FCI rows in the NACK. Only N-2 rows will be used. */
#define _VTSP_RTCP_MSG_FB_NACK_SZ  (6)
#define _VTSP_RTCP_MSG_FB_TMMBR_SZ (5)
#define _VTSP_RTCP_MSG_FB_TMMBN_SZ (5)
#define _VTSP_RTCP_MSG_FB_PLI_SZ   (3)
#define _VTSP_RTCP_MSG_FB_FIR_SZ   (5)

/*
 * Define the longest supported message.
 */
#define _VTSP_RTCP_PAYLOAD_SZ (_VTSP_RTCP_MSG_SR_SZ + _VTSP_RTCP_MSG_SDES_SZ + \
                               _VTSP_RTCP_MSG_XR_SZ + _VTSP_RTCP_MSG_FB_NACK_SZ + \
                               _VTSP_RTCP_MSG_FB_TMMBR_SZ + _VTSP_RTCP_MSG_FB_TMMBN_SZ + \
                               _VTSP_RTCP_MSG_FB_PLI_SZ + _VTSP_RTCP_MSG_FB_FIR_SZ + \
                               _VTSP_RTCP_MSG_BYE_SZ)

#define _VTSP_RTCP_SOCKET_STATE_CLOSED (0)
#define _VTSP_RTCP_SOCKET_STATE_OPEN   (1)
#define _VTSP_RTCP_SOCKET_STATE_ACTIVE (2)

/*
 * Define standard RTCP Packet Types.
 */
#define _VTSP_RTCP_PTYPE_SR          (200)
#define _VTSP_RTCP_PTYPE_RR          (201)
#define _VTSP_RTCP_PTYPE_SDES        (202)
#define _VTSP_RTCP_PTYPE_BYE         (203)
#define _VTSP_RTCP_PTYPE_RTPFB       (205) /* Transport layer FB - RFC 4585. */
#define _VTSP_RTCP_PTYPE_PSFB        (206) /* Payload-specific FB - RFC 4585. */
#define _VTSP_RTCP_PTYPE_XR          (207)

/* Define standard RTCP XR Block types (RFC 3611). */
#define _VTSP_RTCP_XR_BTYPE_SS       (6)   /* Statistics Summary Report Block. */
#define _VTSP_RTCP_XR_BTYPE_VMR      (7)   /* VoIP Metrics Report Block. */


#define _VTSP_RTCP_SDES_TYPE_CNAME   (1)

/* RTCP Feedback Message Type for Transport layer FB (RTPFB). */
#define _VTSP_RTCP_FMT_NACK          (1) /* Generic NACK. */
#define _VTSP_RTCP_FMT_TMMBR         (3) /* Temporary Maximum Media Stream Bit Rate Request. */
#define _VTSP_RTCP_FMT_TMMBN         (4) /* Temporary Maximum Media Stream Bit Rate Notification. */

/* RTCP Feedback Message Type for Payload-specific FB (PSFB). */
#define _VTSP_RTCP_FMT_PLI           (1) /* Picture Loss Indication. */
#define _VTSP_RTCP_FMT_FIR           (4) /* Full Intra Request. */

/*
 * Get and put queue must be the same size because that use the same message
 * structure that was defined with _VTSP_Q_FLOW_PAYLOAD_SZ.
 */
#define _VTSP_Q_FLOW_PUT_DATA_SZ (sizeof(_VTSP_FlowMsg))
#define _VTSP_Q_FLOW_GET_DATA_SZ (sizeof(_VTSP_FlowMsg))

#ifdef VTSP_ENABLE_TIMEOUT_FIX
/* 
 * On some Gnu/Linux, must not use ms wait, 
 * can only use NO_WAIT for WAIT_FOREVER. 
 */
#define _VTSP_Q_CMD_TIMEOUT        (OSAL_WAIT_FOREVER)
#else
#define _VTSP_Q_CMD_TIMEOUT        (400)    /* ms timeout for cmdQ send */
#endif

#define _VTSP_Q_STUN_NUM_MSG       (8)
#define _VTSP_RESERVED_DATA_SZ     (50)

typedef enum { 
    _VTSP_CMD_TRACE                     = 0,  /* XXX */
    _VTSP_CMD_NO_OP                     = 1,
    _VTSP_CMD_TONE_LOCAL_DIGIT          = 2,  /* XXX */
    _VTSP_CMD_TONE_LOCAL                = 3,
    _VTSP_CMD_TONE_PEER                 = 4,  /* XXX */
    _VTSP_CMD_DETECT                    = 5,
    _VTSP_CMD_RING                      = 6,
    _VTSP_CMD_INFC_CONTROL_HOOK         = 7,
    _VTSP_CMD_INFC_CONTROL_IO           = 8,
    _VTSP_CMD_INFC_CONTROL_GAIN         = 9,
    _VTSP_CMD_CID_OFFHOOK               = 12,
    _VTSP_CMD_CID_GET_DATA              = 13, /* XXX  */
    _VTSP_CMD_INFC_LINE_STATUS          = 14,
    _VTSP_CMD_CONFIG                    = 15,
    _VTSP_CMD_QUERY                     = 16,
    _VTSP_CMD_START                     = 17,
    _VTSP_CMD_GET_VERSION               = 18,
    _VTSP_CMD_GET_INFO                  = 19,
    _VTSP_CMD_SHUTDOWN                  = 20,
    _VTSP_CMD_STREAM_START              = 21,
    _VTSP_CMD_STREAM_END                = 22,
    _VTSP_CMD_STREAM_MODIFY             = 23,
    _VTSP_CMD_STREAM_MODIFY_DIR         = 24,
    _VTSP_CMD_STREAM_MODIFY_ENCODER     = 25,
    _VTSP_CMD_STREAM_RFC4733_PEER       = 26, 
    _VTSP_CMD_STREAM_MODIFY_CONFMASK    = 27,
    _VTSP_CMD_TONE_LOCAL_SEQUENCE       = 28,
    _VTSP_CMD_FLOW_OPEN                 = 29,
    _VTSP_CMD_FLOW_CLOSE                = 30,
    _VTSP_CMD_FLOW_ABORT                = 31,
    _VTSP_CMD_RTCP_CNAME                = 32,
    _VTSP_CMD_FXO_SET_HOOK              = 33,
    _VTSP_CMD_FXO_FLASH                 = 34,

    _VTSP_CMD_STREAM_QUERY              = 36,
    _VTSP_CMD_TONE_QUAD_LOCAL           = 40,
    _VTSP_CMD_TONE_QUAD_LOCAL_SEQUENCE  = 41,
    _VTSP_CMD_CID_TX                    = 42,
    _VTSP_CMD_INFC_CONTROL_AUDIO        = 43,

    _VTSP_CMD_STREAM_TONE               = 60,
    _VTSP_CMD_STREAM_TONE_SEQUENCE      = 61,
    _VTSP_CMD_STREAM_TONE_QUAD          = 62,
    _VTSP_CMD_STREAM_TONE_QUAD_SEQUENCE = 63,
    _VTSP_CMD_INFC_GENERATE_PULSE       = 70,
    _VTSP_CMD_STREAM_VIDEO_REQUEST_RESOLUTION = 79,
    _VTSP_CMD_STREAM_VIDEO_REQUEST_KEY        = 80, /* Ask the application to generate a key frame */
    _VTSP_CMD_STREAM_VIDEO_START              = 81,
    _VTSP_CMD_STREAM_VIDEO_MODIFY             = 82,
    _VTSP_CMD_STREAM_VIDEO_MODIFY_DIR         = 83,
    _VTSP_CMD_STREAM_VIDEO_MODIFY_ENCODER     = 84,
    _VTSP_CMD_STREAM_VIDEO_MODIFY_CONFMASK    = 85,
    _VTSP_CMD_STREAM_VIDEO_QUERY              = 86,
    _VTSP_CMD_STREAM_VIDEO_SYNC               = 87,
    _VTSP_CMD_STREAM_VIDEO_END                = 88
} _VTSP_CmdMsgCode;

typedef struct { 
    uint32              arg0;
    uint32              arg1;
    uint32              arg2;
    uint32              arg3;
    uint32              arg4;
    uint32              arg5;
    uint8               arg6[_VTSP_RESERVED_DATA_SZ];
} _VTSP_CmdMsgArg;

typedef struct { 
    uint32              templCode;
    union {
        int16               data[22];
        uint32              data32[10];
        char                data8[60];
    } u;
} _VTSP_CmdMsgConfig;

typedef struct { 
    uint32              type;
    uvint               duration;
    int16               data[9];
} _VTSP_CmdMsgRfc4733;

typedef struct { 
    uint32              code;
    int32               arg;
} _VTSP_CmdMsgControl;

/*
 * VTSP Flow CmdMsg
 */
typedef struct {
    uvint flowIndex;
    uvint flowId;
    uvint streamId;
    uvint flowDir;
    uvint key;
    uvint coder;
} _VTSP_CmdMsgFlowOpen;

typedef struct {
    uvint flowIndex;
    uvint streamId;
} _VTSP_CmdMsgFlow;

/*
 *  RTCP CNAME CmdMsg
 */
typedef struct {
    uint32  cname[_VTSP_RTCP_CNAME_SZ];
} _VTSP_CmdMsgRtcpCname;

/*
 * RTCP Private structures
 */
typedef struct {
    vint command;
    vint infc;
    vint streamId;
    vint payloadSize;
    union {
        uint32 payload[_VTSP_RTCP_PAYLOAD_SZ];
        struct {
            vint   infc;
            OSAL_NetAddress remoteAddr;
            OSAL_NetAddress localAddr;
            vint   tos;
        } open;
    } msg;
} _VTSP_RtcpCmdMsg;

typedef struct {
    vint   streamId;
    vint   infc;
    vint   reason;
    uint32 ssrc;
    VTSP_NtpTime receivedTime;
    uint32 arg1;
    uint32 arg2;
    uint32 arg3;
    uint32 arg4;
    uint32 arg5;
    uint32 arg6;
} _VTSP_RtcpEventMsg;

typedef struct {
    vint               infc;
    vint               active;
    OSAL_NetSockId     socketFd;
    OSAL_NetAddress    ipAddr;
} _VTSP_RtcpSktObj;

typedef struct {
    OSAL_TaskId taskId;
    OSAL_MsgQId msgQId;
    OSAL_MsgQId msgQVideoId;
    OSAL_MsgQGrpId msgQGrp;
    OSAL_SemId  finishSemId;
    uvint       stackSize;
    uvint       taskPriority;
} _RTCP_MsgTaskContext;

typedef struct {
    OSAL_TaskId        taskId; 
    OSAL_MsgQId        msgQId;
    OSAL_MsgQId        msgQVideoId;
    OSAL_SemId         finishSemId;
    uvint              stackSize;
    uvint              taskPriority;
    _VTSP_RtcpSktObj **rtcpSkt_ptr;
    _VTSP_RtcpSktObj **rtcpSktVideo_ptr;
} _RTCP_SktTaskContext;

typedef struct {
    _RTCP_MsgTaskContext msgTaskContext;
    _RTCP_SktTaskContext sktTaskContext;
    uint16               controlPort;
    uint16               numInfc;
    uint16               numStreamId;
    uint16               numRtcp;
} _RTCP_TaskContext;

/*
 * Shared variables
 */
typedef struct {
    uvint  coder;
    vint   blockSize;
    uvint  key;
    uint32 control;
    uint32 duration;
    uint8  payload[_VTSP_Q_FLOW_PAYLOAD_SZ];
} _VTSP_FlowMsg;

typedef struct { 
    uvint           numToneIds;
    uint32          control;
    uint32          repeat;
    uint32          maxTime;
    uvint           streamId;
    uvint           toneIds[_VTSP_NUM_TONE_SEQUENCE_MAX];
} _VTSP_CmdMsgToneSequence;

typedef struct {
    uvint  streamId;
    uint32 timeStampDecoding;
    uint32 timeStampArrival;
} _VTSP_CmdMsgSync;

/* All elements of CmdMsg must be fixed/known size for
 * IPC between processors
 */
typedef struct { 
    _VTSP_CmdMsgCode    code;
    uint16              infc;
    union { 
        _VTSP_CmdMsgArg           arg;
        VTSP_Stream               stream;
        VTSP_StreamVideo          streamVideo;
        _VTSP_CmdMsgRfc4733       rfc4733;
        _VTSP_CmdMsgConfig        config;
        _VTSP_CmdMsgControl       control;
        _VTSP_CmdMsgToneSequence  toneSequence;
        _VTSP_CmdMsgFlowOpen      flowOpen;
        _VTSP_CmdMsgFlow          flow;
        _VTSP_CmdMsgRtcpCname     cname;
        _VTSP_CmdMsgSync          sync;
    } msg;
} _VTSP_CmdMsg;


#define _VTSP_Q_CMD_NUM_MSG         (2 * _VTSP_INFC_NUM * _VTSP_STREAM_PER_INFC)
#define _VTSP_Q_CMD_MSG_SZ          (sizeof(_VTSP_CmdMsg))
#define _VTSP_Q_GLOBAL_NUM_MSG      (32)
#define _VTSP_Q_GLOBAL_MSG_SZ       (sizeof(VTSP_EventMsg))
#define _VTSP_Q_EVENT_NUM_MSG       (32)
#define _VTSP_Q_EVENT_MSG_SZ        (sizeof(VTSP_EventMsg))

/*
 * These constants define queue charactistics of queues shared between VTSPR and
 * the RTCP tasks that share these queues.
 */
#define _VTSP_Q_RTCP_NUM_MSG        (8)
#define _VTSP_Q_RTCP_MSG_SZ         (sizeof(_VTSP_RtcpCmdMsg))
#define _VTSP_Q_RTCP_NUM_EVENT      (8)
#define _VTSP_Q_RTCP_EVENT_SZ       (sizeof(_VTSP_RtcpEventMsg))

typedef struct {
    /* application read only, VTSPR write only
     */
    OSAL_MsgQId    eventQ;
    OSAL_MsgQId    cidQ;
    OSAL_MsgQId    blockHdrGetQ[_VTSP_STREAM_PER_INFC];
    OSAL_MsgQId    blockDataGetQ[_VTSP_STREAM_PER_INFC];

    uint32      streamIds;
    uint32      streamVideoIds;

    /* VTSPR read only, application write only
     */
    OSAL_MsgQId    blockHdrPutQ[_VTSP_STREAM_PER_INFC];
    OSAL_MsgQId    blockDataPutQ[_VTSP_STREAM_PER_INFC];

    /*
     * Flow queues. The FromApp queue is write by app, read by VTSP. The ToApp
     * queue is read by app, write by VTSP.
     */
    OSAL_MsgQId    flowDataFromAppQ[_VTSP_FLOW_PER_INFC];
    OSAL_MsgQId    flowDataToAppQ[_VTSP_FLOW_PER_INFC];

} _VTSP_InfcObj;

typedef struct { 
    /* globalEventQ: application read only, VTSPR write only
     */
    OSAL_MsgQId      globalEventQ;
    /* cmdQ: application write only, VTSPR read only
     */
    OSAL_MsgQId      cmdQ;

    /* RTCP task context */
    _RTCP_TaskContext rtcpTaskContext;

    /* Video object */
    OSAL_MsgQGrpId   infcEventQGrp;
    OSAL_MsgQGrpId   blockHdrGetQGrp;

    OSAL_SemId       shutdownSemId;

    OSAL_MsgQId      stunSendQ;
    OSAL_MsgQId      stunRecvQ;

    OSAL_MsgQId      cmdQVideo;
    OSAL_MsgQId      evtQVideo;
    
    OSAL_MsgQId      stunSendQVideo;
    OSAL_MsgQId      stunRecvQVideo;
    
    VTSP_Return      start;
    uvint            taskPriority;
    uvint            taskAddStackSz;

    _VTSP_InfcObj   infc[_VTSP_INFC_NUM];

    /* Configuration data
     */
    VTSP_QueryData  config;

    /*
     * Template configs
     */
    vint            cidFormat;

} _VTSP_Object;

#define _VTSP_OBJECT_SZ    (sizeof(_VTSP_Object))

typedef struct { 
    uint16  len;
    uint8   data[256];
} _VTSP_CIDData;


typedef struct { 
    TIC_RingParams   ticParam;
    uvint            cidsBreakNum;
} _VTSP_RingTemplate;

/*
 * Externs
 * --------
 */

extern _VTSP_Object  *_VTSP_object_ptr;
extern char const D2_Release_VTSP[];
extern char const D2_Release_VTSP_RT[];
extern char const D2_Release_VTSP_HW[];

/*
 * --------
 * XXX Below protos go in seperate file later
 *
 */
VTSP_Return _VTSP_putCmd(
        uvint   infc,
        void   *data_ptr,
        vint    video);

VTSP_Return _VTSP_isInfcFxs(
        uvint     infc);
VTSP_Return _VTSP_isInfcFxo(
        uvint     infc);
VTSP_Return _VTSP_isInfcValid(
        uvint     infc);
VTSP_Return _VTSP_isStreamIdValid(
        uvint     infc,
        uvint     streamId);

VTSP_Return _VTSP_default(
        void);

/* _stream.c */
VTSP_Return _VTSP_isStreamIdValid(
        uvint     infc,
        uvint     streamId);

VTSP_Return _VTSP_isStreamStarted(
        uvint     infc,
        uvint     streamId);

VTSP_Return _VTSP_isStreamDirValid(
        uvint infc, 
        uvint streamId, 
        uvint dir);

VTSP_Return _VTSP_isStreamPeerValid(
        uvint        infc,
        VTSP_Stream *stream_ptr);
VTSP_Return _VTSP_ipcPutStreamData(
        uvint        infc,
        VTSP_Stream *stream_ptr);
VTSP_Return _VTSP_isStreamIdValid(
        uvint infc, 
        uvint streamId);
VTSP_Return _VTSP_isStreamConfMaskValid(
        uvint  infc, 
        uvint  streamId, 
        uint32 confMask);

VTSP_Return _VTSP_isStreamVideoDirValid(
        uvint infc, 
        uvint streamId, 
        uvint dir);

VTSP_Return _VTSP_isStreamVideoPeerValid(
        uvint        infc,
        VTSP_StreamVideo *stream_ptr);

VTSP_Return _VTSP_isStreamVideoIdValid(
        uvint infc, 
        uvint streamId);

VTSP_Return _VTSP_isStreamVideoConfMaskValid(
        uvint  infc, 
        uvint  streamId, 
        uint32 confMask);

VTSP_Return _VTSP_modifyStreamVideoConfMask(
        uvint             infc,
        VTSP_StreamVideo *stream_ptr,
        uint32            confMask);

VTSP_Return _VTSP_isInfcAudio(
        uvint     infc);

/*
 * flow
 */
uvint _VTSP_flowGetIndex(
        vint flowId);
uvint _VTSP_flowGetInterface(
        vint flowId);
uvint _VTSP_flowGetKey(
        vint flowId);
uvint _VTSP_flowGetStreamId(
        vint flowId);
vint _VTSP_flowGetId(
        uvint infc,
        uvint streamId,
        uvint key);
VTSP_Return _VTSP_isFlowDirValid(
        uvint dir);
VTSP_Return _VTSP_isFlowKeyValid(
        uvint dir);
VTSP_Return _VTSP_isFlowValid(
        vint flowId);
VTSP_Return _VTSP_isFlowIdle(
        vint  flowId);
VTSP_Return _VTSP_isFlowOpen(
        vint  flowId);
VTSP_Return _VTSP_isFlowBlockSizeValid(
        uvint blockSize);
VTSP_Return _VTSP_isFlowControlValid(
        uint32 control);
/* control
 */
VTSP_Return _VTSP_isToneTemplIdValid(
        uvint infc, 
        uvint id);
VTSP_Return _VTSP_isRingTemplIdValid(
        uvint infc, 
        uvint id);
VTSP_Return _VTSP_putCidData(
        uvint           infc,
        VTSP_CIDData   *cid_ptr);
VTSP_Return _VTSP_getCidData(
    uvint           infc, 
    uint8          *cidData_ptr);
VTSP_Return _VTSP_setCidCountryCode(
        uvint code,
        uvint infc);
VTSP_Return _VTSP_ring(
        uvint           infc,
        uvint           templateRing,
        uvint           numRings,
        uvint           maxTime);
VTSP_Return _VTSP_cidFormat(
    VTSP_CIDData   *cid_ptr);


/* 
 * _init.c
 */
VTSP_Return _VTSP_setObj(void);

/*
 * RTCP Prototypes
 */
VTSP_Return _VTSP_rtcpInit(
    _RTCP_TaskContext *rtcpContext_ptr,
    uvint              numInfc,
    uvint              numStreamId);
VTSP_Return _VTSP_rtcpShutdown(
    _RTCP_TaskContext *rtcpContext_ptr);
OSAL_TaskReturn _VTSP_rtcpTaskMsg(
    OSAL_TaskArg taskArg);
OSAL_TaskReturn _VTSP_rtcpTaskSocket(
    OSAL_TaskArg taskArg);
_VTSP_RtcpSktObj* _VTSP_streamIdToRtcpSktPtr(
    _RTCP_SktTaskContext *sktTask_ptr,
    uvint                 infc, 
    uvint                 streamId);

/*
 * VTSPR public function
 */
OSAL_TaskId VTSPR_init(
    void);

#endif

