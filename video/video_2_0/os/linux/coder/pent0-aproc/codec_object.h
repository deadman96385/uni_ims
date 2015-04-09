/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2012 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Revision: 25157 $ $Date: 2014-03-16 09:59:26 -0700 (Sun, 16 Mar 2014) $
 */
#ifndef CODEC_OBJECT_H_
#define CODEC_OBJECT_H_

#include <osal.h>
#include <vtsp.h>
#include <vci.h>
#include <video.h>

#ifndef EXCLUDE_FFMPEG_LIBRARY
#include <x264.h>
#include <libavcodec/avcodec.h>
#endif

/*
 * Max packet fragments in a stream.
 */
#define CODEC_ENC_MAX_PKTS     (64)

/* Task related settings. */
#define CODEC_DEC_TASK_NAME        "Codec-Dec-Task"
#define CODEC_ENC_TASK_NAME        "Codec-Enc-Task"
#define _CODEC_TASK_DEC_PRIORITY   (OSAL_TASK_PRIO_VDEC)
#define _CODEC_TASK_ENC_PRIORITY   (OSAL_TASK_PRIO_VENC)
#define _CODEC_TASK_STACK_SZ       (OSAL_STACK_SZ_LARGE)

#ifndef VIDEO_DEBUG_LOG
#define CODEC_dbgPrintf(fmt, args...)
#else
#define CODEC_dbgPrintf(fmt, args...) \
         OSAL_logMsg("[%s:%d] " fmt, __FUNCTION__, __LINE__, ## args)
#endif

/*
 *Returns
 */
#define _CODEC_OK               (1)
#define _CODEC_ERROR            (-1)
#define _CODEC_ERROR_INIT       (-2)

typedef enum {
    CODEC_ENCODER                 = 0,
    CODEC_DECODER                 = 0,
} CODEC_TYPE;

/*
 * Define generic pointer types for CodecObject.
 */
typedef struct _CodecObject   *CodecObject_Ptr;
typedef struct _CODEC         *CODEC_Ptr;
typedef struct _DataObj       *DataObj_Ptr;

typedef struct {
    OSAL_TaskId      taskId;
    uvint            stackSize;
    uvint            taskPriority;
    void            *func_ptr;
    int8             name[16];
    void            *arg_ptr;
} _CODEC_TaskObj;

/*
 * Define ENC/DEC object, including H263 & H264.
 */
typedef struct _DataObj {
#ifndef EXCLUDE_FFMPEG_LIBRARY
    /* H263 ENC/DEC, H264 DEC */
    AVCodecContext     *ctx_ptr;
    AVFrame            *pic_ptr;
    /* H264 ENC */
    x264_t             *enc_ptr;
    x264_picture_t      picOut;
#endif
    /* Common parts */
    uint8              *buf_ptr;
    uint8               outbuf[VIDEO_WIDTH_MAX * VIDEO_HEIGHT_MAX * 4];
    int                 id;
    VideoFormat         format;
    vint                length;
    uint64              tsMs;
    vint                flags;              /* DEC (ENC is unused?) */
    uint8               rcsRtpExtnPayload;
    int                 picReady;           /* H263 */
    int                 width;              /* H264 */
    int                 height;             /* H264 */
    OSAL_Boolean        codecInited;
    OSAL_Boolean        sendFIR;
    int                 maxBitrate;
} DataObj;

/*
 * Define the CODEC interface.
 */
typedef struct _CODEC {
    CodecObject_Ptr         codecEncoder_ptr;
    CodecObject_Ptr         codecDecoder_ptr;
    struct _enc {
        vint                codecType;
        _CODEC_TaskObj      task;
        OSAL_Boolean        started;
        DataObj             data;
    } enc;
    struct _dec {
        vint                codecType;
        _CODEC_TaskObj      task;
        OSAL_Boolean        started;
        DataObj             data;
    } dec;
} CODEC;

/*
 * Define function pointer types for CodecObject methods
 */
typedef void (*Codec_Init)(CODEC_Ptr, vint);
typedef vint (*Codec_QueueInputBuffer)(DataObj_Ptr);
typedef vint (*Codec_DequeueOutputBuffer)(DataObj_Ptr);
typedef void (*Codec_Modify)(CODEC_Ptr);
typedef void (*Codec_EncodeFIR)(CODEC_Ptr);
typedef void (*Codec_Release)(CODEC_Ptr);

/*
 * Define the CodecObject.
 */
typedef struct _CodecObject {
    Codec_Init                  codecInit;
    Codec_QueueInputBuffer      codecQueueInputBuffer;
    Codec_DequeueOutputBuffer   codecDequeueOutputBuffer;
    Codec_Modify                codecModify;
    Codec_EncodeFIR             codecEncodeFIR;
    Codec_Release               codecRelease;
    OSAL_Boolean                isEncoder;
} CodecObject;

extern const CodecObject   _CodecH264Encoder;
extern const CodecObject   _CodecH263Encoder;
extern const CodecObject   _CodecDecoder;

#endif /* CODEC_OBJECT_H_ */
