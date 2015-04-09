/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2012 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Revision: 25157 $ $Date: 2014-03-16 09:59:26 -0700 (Sun, 16 Mar 2014) $
 */
#include <osal.h>
#include "codec_object.h"
#include "codecSem.h"
#include "h264.h"

extern OSAL_SemId Codec_SemLock;

/*
 * ======== _codecDecQueueInputBuffer() ========
 * This function feeds data to the decoder to run it. Data is received from
 * RTP.
 * Returns:
 *  1: Success.
 *  0: Failed (encoded buffer is not consumed).
 */
vint _codecDecQueueInputBuffer(
    DataObj_Ptr dataObj_ptr)
{
    /* Decode */
#ifndef EXCLUDE_FFMPEG_LIBRARY
    AVPacket pkt;
#endif
    int ret = 0;

    if (NULL == dataObj_ptr) {
        DBG("FAILURE\n");
        return (0);
    }

#ifndef EXCLUDE_FFMPEG_LIBRARY
    if (NULL == dataObj_ptr->ctx_ptr) {
        DBG("FAILURE\n");
        return (0);
    }
    av_init_packet(&pkt);
    pkt.pts = dataObj_ptr->tsMs;
    pkt.size = dataObj_ptr->length;
    pkt.flags = 0;

    pkt.data = (void *) dataObj_ptr->buf_ptr;
    pkt.stream_index = 0;
    pkt.duration = 0;
    pkt.priv = NULL;
    pkt.destruct = NULL;
    pkt.pos = -1;
    pkt.convergence_duration = 0;

    ret = avcodec_decode_video2(dataObj_ptr->ctx_ptr, dataObj_ptr->pic_ptr,
            &dataObj_ptr->picReady, &pkt);
#endif
    return (ret);
}

/*
 * ======== _codecDecDequeueOutputBuffer() ========
 * This function is called by other modules which need to get received raw video data.
 * Ex: [View] To display received raw video data.
 * Returns:
 *  1: Success.
 *  0: Failed (encoded buffer is not consumed).
 */
vint _codecDecDequeueOutputBuffer(
    DataObj_Ptr    dataObj_ptr)
{
#ifndef EXCLUDE_FFMPEG_LIBRARY
    int size = 0;
    int xY;
    int h;
#endif

    if (NULL == dataObj_ptr){
        DBG("FAILURE\n");
        return (0);
    }

#ifndef EXCLUDE_FFMPEG_LIBRARY
    if (NULL == dataObj_ptr->ctx_ptr) {
        DBG("FAILURE\n");
        return (0);
    }
#endif
    if (0 == dataObj_ptr->picReady) {
        return (0);
    }

    memset(dataObj_ptr->outbuf, 0, sizeof(dataObj_ptr->outbuf));

#ifndef EXCLUDE_FFMPEG_LIBRARY
    xY = dataObj_ptr->pic_ptr->linesize[0];
    h = dataObj_ptr->ctx_ptr->height;
    size = (xY * h * 3) >> 1;
    if (size > (VIDEO_WIDTH_MAX * VIDEO_HEIGHT_MAX * 4)) {
        /*
         * This will save against dynamic decoder data resolution buffer
         * overflow.
         */
        size = 0;
        DBG("FAILURE\n");
        return (0);
    }
    memcpy(dataObj_ptr->outbuf, dataObj_ptr->pic_ptr->data[0], xY * h);
    memcpy(dataObj_ptr->outbuf + xY * h, dataObj_ptr->pic_ptr->data[1],
            xY * h >> 2);
    memcpy(dataObj_ptr->outbuf + xY * h + (xY * h >> 2),
            dataObj_ptr->pic_ptr->data[2], xY * h >> 2);

    dataObj_ptr->tsMs = dataObj_ptr->pic_ptr->pts;
    dataObj_ptr->format = VIDEO_FORMAT_YCbCr_420_P;
    dataObj_ptr->width = xY;
    dataObj_ptr->height = h;

#endif

    dataObj_ptr->picReady = 0;

    /* Currently, we only have 1 view. */
    dataObj_ptr->id = 0;
    return (1);
}

/*
 * ======== _CODEC_DecodeTask() ========
 *
 * Decoding task.
 *
 * Return Values:
 * None.
 */
static void _CODEC_DecodeTask(void *arg_ptr) {
    CODEC_Ptr    codec_ptr;
    int          nal;
    OSAL_Boolean keyFrame;

    keyFrame              = OSAL_FALSE;
    VC_EncodedFrame frame = { NULL, 0, 0, 0, 0 };

    codec_ptr = (CODEC *) arg_ptr;
    /*
     * Big D2 Banner with version information.
     */
    OSAL_logMsg("\n"
            "             D2 Technologies, Inc.\n"
            "      _   _  __                           \n"
            "     / | '/  /_  _  /_ _  _  /_  _  ._   _\n"
            "    /_.'/_  //_'/_ / // //_///_//_///_'_\\ \n"
            "                                _/        \n"
            "\n"
            "        Unified Communication Products\n");
    OSAL_logMsg("               www.d2tech.com\n");

    OSAL_logMsg("%s:%d RUNNING\n", __FILE__, __LINE__);

_CODEC_DECODING_TASK_LOOP:
    /* Block on event message from VCI. */
    while (_CODEC_OK == VCI_getEncodedFrame(&frame)) {
        //OSAL_logMsg("%s:%d %d %d %d\n", __FILE__, __LINE__, frame.length, frame.flags, frame.rcsRtpExtnPayload);
        codec_ptr->dec.data.buf_ptr           = frame.data_ptr;
        codec_ptr->dec.data.length            = frame.length;
        codec_ptr->dec.data.tsMs              = frame.tsMs;
        codec_ptr->dec.data.flags             = frame.flags;
        codec_ptr->dec.data.rcsRtpExtnPayload = frame.rcsRtpExtnPayload;
        codec_ptr->codecDecoder_ptr->codecQueueInputBuffer(
               &(codec_ptr->dec.data));

        /*
         * Aaron, RTCP-FB, to identify SPS, PPS, or I-frame. Only H264.
         * It should check SPS, PPS, I-frame.
         * Currently, I only check SPS, PPS, I-Frame in the beginning.
         * TODO: We may check SPS, PPS, I-frame in each GOP???
         */
        if(VTSP_CODER_VIDEO_H264 == codec_ptr->dec.codecType) {
            nal = H264_READ_NALU(frame.data_ptr[4]);
            if (NALU_IDR == nal || NALU_SPS == nal || NALU_PPS == nal) {
                keyFrame = OSAL_TRUE;
            }
            if (frame.length < 0 || keyFrame == OSAL_FALSE) {
                OSAL_logMsg("%s:%d DECODER, sendFIR \n", __FILE__, __LINE__);
                VCI_sendFIR();
            }
        }

        OSAL_memFree(codec_ptr->dec.data.buf_ptr, 0);
        usleep(20000);
    }

    usleep(20000);

    /* Loop as long as the decoder is initialized. */
    if (OSAL_TRUE == codec_ptr->dec.started) {
        goto _CODEC_DECODING_TASK_LOOP;
    } else {
        OSAL_logMsg("%s:%d _CODEC_DecodeTask exited\n", __FILE__, __LINE__);
    }
}

/*
 * ======== _codecInit() ========
 * Initialize the DEC codec and start decoding.
 */
static void _codecInit(
    CODEC_Ptr codec_ptr,
    vint      codecType)
{
#ifndef EXCLUDE_FFMPEG_LIBRARY
    AVCodec *avCodec_ptr;
#endif
    _CODEC_TaskObj *task_ptr = &codec_ptr->dec.task;

    codec_ptr->dec.started = OSAL_TRUE;

    codec_ptr->dec.codecType = codecType;

    OSAL_memSet(&(codec_ptr->dec.data), 0, sizeof(DataObj));

    if (codec_ptr->dec.data.codecInited) {
        return;
    }

    /* Allocate input buffer for raw video frame. */
    codec_ptr->dec.data.buf_ptr = OSAL_memAlloc(
        VIDEO_WIDTH_MAX * VIDEO_HEIGHT_MAX * 4, 0);

    if (NULL == codec_ptr->dec.data.buf_ptr) {
        DBG("FAILURE\n");
        return ;
    }
#ifndef EXCLUDE_FFMPEG_LIBRARY
    if (codecType == VTSP_CODER_VIDEO_H263) {
        avCodec_ptr = avcodec_find_decoder(CODEC_ID_H263);
    } else if (codecType == VTSP_CODER_VIDEO_H264) {
        avCodec_ptr = avcodec_find_decoder(CODEC_ID_H264);
    } else {
           DBG("FAILURE: Unsupported CodecType = %d\n", codecType);
           return;
    }
    if (NULL == avCodec_ptr) {
        DBG("FAILURE\n");
        goto _RETURN_FAIL_HANDLE;
    }
    codec_ptr->dec.data.ctx_ptr = avcodec_alloc_context();
    if (NULL == codec_ptr->dec.data.ctx_ptr) {
        DBG("FAILURE\n");
        goto _RETURN_FAIL_HANDLE;
    }
#endif
    if (OSAL_FAIL == OSAL_semAcquire(Codec_SemLock, OSAL_WAIT_FOREVER)) {
        goto _RETURN_FAIL_CTX;
    }

#ifndef EXCLUDE_FFMPEG_LIBRARY
    if (avcodec_open(codec_ptr->dec.data.ctx_ptr, avCodec_ptr) < 0) {
        DBG("FAILURE\n");
        OSAL_semGive(Codec_SemLock);
        goto _RETURN_FAIL_CTX;
    }
#endif
    if (OSAL_FAIL == OSAL_semGive(Codec_SemLock)) {
        goto _RETURN_FAIL;
    }

    /*
     * New frame.
     */
#ifndef EXCLUDE_FFMPEG_LIBRARY
    codec_ptr->dec.data.pic_ptr = avcodec_alloc_frame();
    if (NULL == codec_ptr->dec.data.pic_ptr) {
        DBG("FAILURE\n");
        goto _RETURN_FAIL;
    }
#endif

    DBG("FFMPEG decode init complete\n");
    goto _INIT_DECODE_COMPLETE;

_RETURN_FAIL:
#ifndef EXCLUDE_FFMPEG_LIBRARY
    avcodec_close(codec_ptr->dec.data.ctx_ptr);
#endif
_RETURN_FAIL_CTX:
#ifndef EXCLUDE_FFMPEG_LIBRARY
    av_free(codec_ptr->dec.data.ctx_ptr);
_RETURN_FAIL_HANDLE:
#endif
    OSAL_memFree(codec_ptr->dec.data.buf_ptr, 0);

_INIT_DECODE_COMPLETE:

    codec_ptr->dec.data.codecInited = OSAL_TRUE;

    CODEC_dbgPrintf("Decoder init complete\n");

    /* Init the task used to feed decoder. */
    task_ptr->taskId = 0;
    task_ptr->stackSize = _CODEC_TASK_STACK_SZ;
    task_ptr->taskPriority = _CODEC_TASK_DEC_PRIORITY;
    task_ptr->func_ptr = _CODEC_DecodeTask;

    OSAL_snprintf(task_ptr->name, sizeof(task_ptr->name), "%s",
    CODEC_DEC_TASK_NAME);

    task_ptr->arg_ptr = (void *) codec_ptr;

    if (0 == (task_ptr->taskId = OSAL_taskCreate(task_ptr->name,
                    task_ptr->taskPriority, task_ptr->stackSize,
                    task_ptr->func_ptr, task_ptr->arg_ptr))) {
        codec_ptr->codecDecoder_ptr->codecRelease(codec_ptr);
        return;
    }
}

/*
 * ======== _codecRelease() ========
 * Terminate the decoder and release it.
 */
static void _codecRelease(
    CODEC_Ptr codec_ptr)
{
    codec_ptr->dec.started = OSAL_FALSE;

    if (!codec_ptr->dec.data.codecInited) {
        return;
    }
    if (codec_ptr->dec.data.codecInited) {
        if (NULL == &(codec_ptr->dec.data)) {
            DBG("FAILURE\n");
            return;
        }

#ifndef EXCLUDE_FFMPEG_LIBRARY
        if (NULL == codec_ptr->dec.data.ctx_ptr) {
            DBG("FAILURE\n");
            return;
        }
#endif
        /*
         * Stop all decoding activity.
         */
        OSAL_memFree(codec_ptr->dec.data.buf_ptr, 0);

        if (OSAL_FAIL == OSAL_semAcquire(Codec_SemLock, OSAL_WAIT_FOREVER)) {
            return;
        }
#ifndef EXCLUDE_FFMPEG_LIBRARY
        if (avcodec_close(codec_ptr->dec.data.ctx_ptr) < 0) {
            DBG("FAILURE\n");
        }
#endif
        if (OSAL_FAIL == OSAL_semGive(Codec_SemLock)) {
            return;
        }

#ifndef EXCLUDE_FFMPEG_LIBRARY
        av_free(codec_ptr->dec.data.pic_ptr);
        av_free(codec_ptr->dec.data.ctx_ptr);
        codec_ptr->dec.data.ctx_ptr = NULL;
        codec_ptr->dec.data.pic_ptr = NULL;
#endif
        DBG("FFMPEG decode shut down complete\n");
        codec_ptr->dec.data.codecInited = OSAL_FALSE;
    }
}

const CodecObject _CodecDecoder = {
    _codecInit,                     /* codec initial */
    _codecDecQueueInputBuffer,      /* queue encoded frames to codec */
    _codecDecDequeueOutputBuffer,   /* Called by other modules, pass raw video data */
    NULL,                           /* Nothing to do on codecModify */
    NULL,                           /* Nothing to do on codecEncodeFIR */
    _codecRelease,                  /* codec terminate */
    OSAL_FALSE,                     /* Is this an encoder? */
};
