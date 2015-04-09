/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 28625 $ $Date: 2014-09-01 15:12:45 +0800 (Mon, 01 Sep 2014) $
 */
  
#ifndef _MC_H_
#define _MC_H_

/* Return codes. */
#define MC_OK                     (0)
#define MC_ERR                    (-1)

/* Task Related settings */

#define MC_TASK_STACK_BYTES       (8192)
#define MC_ISI_INFC_TASK_NAME     "McIsiCmd"
#define MC_VTSP_EVT_TASK_NAME     "McEvent"
#define MC_MAX_QUEUE_DEPTH        (8)

/* Diagnostic audio test. */
#define MC_PAYLOAD_PLAY_SZ         400
#define MC_PAYLOAD_RECORD_SZ       800
#define MC_AUDIO_FILE_NAME_LENGTH  64
#define MC_AUDIO_PLAY_EVENT        11
#define MC_AUDIO_RECORD_EVENT      12

/* Generally used definitions */
#define MC_STRING_SZ              (128)

/*Interfaces in vTSP. Set to 1 for handset */
#define MC_INFC_NUM               (1)

/* Retry timer in ms when registering MC to ISI */
#define MC_REGISTER_TIMER_MS      (5000)

/* Streams in vTSP. */
#define MC_STREAM_NUM             (3)

/* Define min/max rtp-rtcp inactivity timer */
#define MC_MIN_RTP_RTCP_INACTIVE_TIMER  (10)
#define MC_MAX_RTP_RTCP_INACTIVE_TIMER  (40)

#ifdef MC_DEBUG
#define MC_dbgPrintf OSAL_logMsg
#else
#define MC_dbgPrintf(x, ...)
#endif
 
/* These are stream modify operations. */
typedef enum {
    MC_STREAM_OP_DIR = 0,
    MC_STREAM_OP_START,
    MC_STREAM_OP_MODIFY,
    MC_STREAM_OP_END
} MC_StreamOp;

typedef enum {
    MC_MEDIA_TYPE_AUDIO = 1,
    MC_MEDIA_TYPE_VIDEO
} MC_MediaType;

typedef struct {
    OSAL_TaskId tId;
    uvint       stackSz;
    uvint       pri;
    void       *func_ptr;
    char        name[16];
    void       *arg_ptr;
} MC_TaskObj;

/* Object used for IPC communication */ 
typedef struct {
    union {
        ISIP_Message    isi;
        /* Add other objects here */
    } u;
} MC_Msg;

typedef struct {
    vint   semaphore;
} MC_VtspCodecControl;

typedef struct {
    ISIP_Message isiMsg;
    OSAL_MsgQId  isiEvt;
} MC_Event;

typedef struct {
    ISI_Id   callId;
    vint     protocol;
    vint     streamId;
    vint     rtpCheckingDisable; /* If this is 1, do not need to process RTP inactive */
} MC_StreamInfo;

typedef struct {
    uint8 SQCIF;        /* 1 - 32 */
    uint8 QCIF;         /* 1 - 32 */
    uint8 CIF;          /* 1 - 32 */
    uint8 CIF4;         /* 1 - 32 */
    uint8 CIF16;        /* 1 - 32 */
    struct {
        uint8 left;     /* 1 - 255 */
        uint8 right;    /* 1 - 255 */
    } PAR_ratio;
    struct {
        uint8 cd;       /* 1 - 127 */
        int   cf;       /* 1000, 1001 */
        int SQCIFMPI;   /* 1 to 2048 */
        int QCIFMPI;    /* 1 to 2048 */
        int CIFMPI;     /* 1 to 2048 */
        int CIF4MPI;    /* 1 to 2048 */
        int CIF16MPI;   /* 1 to 2048 */
        int CUSTOMMPI;  /* 1 to 2048 */
    } CPCF;
    int BPP;            /* 0 - 65536 */
    uint8 HRD;          /* 0 or 1 */
    struct {
        int Xmax;
        int Ymax;
        int MPI;
    } CUSTOM;
    uint8 PROFILE;      /* 0 - 10 */
    uint8 LEVEL;        /* 0 - 100 */
    uint8 INTERLACE;    /* 0 or 1 */
    struct {
        uint8 F;        /* Possible values are 0 - 1 */
        uint8 I;        /* Possible values are 0 - 1 */
        uint8 J;        /* Possible values are 0 - 1 */
        uint8 T;        /* Possible values are 0 - 1 */
        uint8 K;        /* posible values 1 - 4 */
        uint8 N;     /* Possible values are 1 - 4 */
        uint8 P;     /* Possible values are 1 - 4 */
    } annex;            /* "F", "I", "J", "T", "K", "N", and "P". */
    int  maxBr;
    int  maxMbps;
} H263_Params;

typedef struct {
    char szCoderName[ISI_CODER_STRING_SZ + 1];
    int  coderNum;
    int  rate;
    MC_MediaType relates;
    union {
        struct {
            int annexb;
        } g729;
        struct {
            int  octetAlign;
            int  modeSet; /* Mode-set bit mask */
        } amr;
        struct {
            int  pmode;
            char plevel[8];
            struct {
                int id;
                char uri[VTSP_EXTMAP_URI_SZ];
            } extmap;
            int  maxBr;
            int  maxMbps;
        } h264;
        H263_Params h263;
    } props;
} MC_Coder;

typedef struct {
    VTSP_Stream vtspStream;
    vint        isStarted;
    uint32      recvPktCount;
    vint        rtpInactiveTmr;
    vint        rtcpInactiveTmr;
    ISI_Id      serviceId;
} MC_AudioStream;

typedef struct {
    VTSP_StreamVideo vtspStream;
    vint             isStarted;
    ISI_Id           serviceId;
} MC_VideoStream;

typedef struct {
    uvint  vInfc;
    uvint  vStreamId;
    uint32 vLatency;    /* Video path decode latency. JBV-to-Render */
    uint32 aLatency;    /* Audio path decode latency. JB-to-Playout */
    uint32 vNtp;        /* Latest Video RTCP SR - NTP time in milliseconds */
    uint32 aNtp;        /* Latest Audio RTCP SR - NTP time in milliseconds */
    uint32 vRtcpRtp;    /* Latest Video RTCP SR - RTP timestamp in milliseconds */
    uint32 aRtcpRtp;    /* Latest Audio RTCP SR - RTP timestamp in milliseconds */
    uint32 vLatestRtp;  /* Latest Video RTP packet timestamp in milliseconds */
    uint32 aLatestRtp;  /* Latest Audio RTP packet timestamp in milliseconds */
    vint   enabled;     /* Flag to indicate if lip sync is enabled or not. */
} MC_LipSync;

/* VTSP object to manage VTSP commands and events */
typedef struct {
    MC_TaskObj          evtTask;
    MC_VtspCodecControl codecControl;
    char                scratch[MC_STRING_SZ];
    struct {
        OSAL_TmrId      id;
        MC_Event        event;
    } tmr;
    struct {
        MC_AudioStream  audio[MC_STREAM_NUM];
        MC_VideoStream  video[MC_STREAM_NUM];
        MC_LipSync      syncEngine;
        MC_StreamInfo   streamInfo[MC_STREAM_NUM];
        VTSP_Context   *vtsp_ptr;
        uint16          firstRtpPort;
        vint            autoCallProgressTones;
    } vtsp;
    struct {         
        ISI_Id         isiId;
        ISI_TelEvent   evt;
        ISI_Id         isiServiceId;
        int            numDigits;
    } telEvt;
    VTSP_Stun       vtspPacketSend;
    VTSP_Stun       vtspPacketRecv;
    MC_Event        event;
    int             lastLoss;
    int             bitrate;
    MC_Coder        coders[ISI_CODER_NUM];
    MC_Coder        decoders[ISI_CODER_NUM];
    vint            inactivityExp;
    vint            rtcpInterval;
} MC_VtspObj;

/* ISI interface object */
typedef struct {
    MC_TaskObj       task;
    MC_Event         event;
    struct {
        OSAL_MsgQId  readIpcId;
        OSAL_MsgQId  writeIpcId;
        MC_Msg       msg;    
    } queue;
} MC_IsiObj;

/*
 * This is the MC global object.
 * Anything allocated statically must be placed in this object.
 *
 * MC_Isi and MC_Vtsp are refactored as two modules with independent globals
 *
typedef struct {
    MC_VtspObj  vtspObj;
    MC_IsiObj   isiObj;
} MC_GlobalObj;
 *
 */

vint _MC_vtspTimerInit(
    MC_VtspObj  *vtspObj_ptr);

void _MC_vtspTimerDestroy(
    MC_VtspObj  *vtspObj_ptr);

void MC_isiMediaCmd(
    MC_VtspObj   *vtsp_ptr,
    ISIP_Message *cmd_ptr);

vint MC_sendEvent(
    MC_Event *evt_ptr);

MC_StreamInfo* MC_vtspGetStreamInfoViaCallId(
    MC_VtspObj    *vtsp_ptr,
    ISI_Id         callId);

void MC_telEvtIsiEvt(
    ISI_Id              telEvtId,
    ISI_Id              serviceId,
    ISIP_TelEvtReason   reason,
    ISI_TelEvent        evt,
    ISIP_Message       *isi_ptr);

void _MC_LipSync_calculateAudioVideoSkew(
    MC_LipSync    *syncEngine_ptr);

MC_VtspObj *_MC_vtspGetObjPtr(void);

void _MC_vtspDupIsiEventQid(
    MC_IsiObj   *isi_ptr);

int MC_vtspInit(void *cfg_ptr);

vint MC_vtspAllocate(void);

vint MC_vtspStart(void);

vint MC_vtspDestroy(void);

#endif
