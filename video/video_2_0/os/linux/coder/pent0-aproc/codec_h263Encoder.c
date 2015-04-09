/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2012 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Revision: 25157 $ $Date: 2014-03-16 09:59:26 -0700 (Sun, 16 Mar 2014) $
 */
#include <osal.h>
#include "codec_object.h"
#include <img_convert.h>
#include <codecSem.h>

/* Semaphore for initializing/shutting encoder. */
extern OSAL_SemId Codec_SemLock;

/*
 * ======== _codecEncQueueInputBuffer() ========
 * This function is called by Camera to send captured video frame to the encoder.
 * Returns:
 *  1: Success
 *  0: Failed
 */
static vint _codecEncQueueInputBuffer(
    DataObj_Ptr    dataObj_ptr)
{

    /* Encode */
#ifndef EXCLUDE_FFMPEG_LIBRARY
    /* For image conversion */
    /* Size for YUV 420 */
    int array_size = dataObj_ptr->width * dataObj_ptr->height * 3 / 2;
    char tempBuf_ptr[array_size];

    int sizeY;
    int sizeUV;
    int ret;

    if (NULL == dataObj_ptr) {
        DBG("FAILURE\n");
        return (0);
    }

    if (NULL == dataObj_ptr->ctx_ptr) {
        DBG("FAILURE\n");
        return (0);
    }

    /*
     * Size of Y component, for 4:2:0, there is downsampling in X and Y
     * dimensions, so U size is 1/4 of Y size. Same for V size.
     * ???????????
     */

    sizeY = dataObj_ptr->ctx_ptr->width * dataObj_ptr->ctx_ptr->height;
    sizeUV = (dataObj_ptr->ctx_ptr->width >> 1) * (dataObj_ptr->ctx_ptr->height >> 1);

    IMG_formatConvert((void *)(void *)tempBuf_ptr,
        dataObj_ptr->ctx_ptr->width, dataObj_ptr->ctx_ptr->height, VIDEO_FORMAT_YCbCr_420_P,
        (void *)dataObj_ptr->buf_ptr,
        dataObj_ptr->width, dataObj_ptr->height, dataObj_ptr->format);

    dataObj_ptr->pic_ptr->data[0]     = (void *)tempBuf_ptr;
    dataObj_ptr->pic_ptr->data[1]     = (void *)tempBuf_ptr + sizeY;
    dataObj_ptr->pic_ptr->data[2]     = (void *)tempBuf_ptr + sizeY + sizeUV;
    dataObj_ptr->pic_ptr->data[3]     = NULL;
    dataObj_ptr->pic_ptr->linesize[0] = dataObj_ptr->ctx_ptr->width;
    dataObj_ptr->pic_ptr->linesize[1] = dataObj_ptr->ctx_ptr->width / 2;
    dataObj_ptr->pic_ptr->linesize[2] = dataObj_ptr->ctx_ptr->width / 2;
    dataObj_ptr->pic_ptr->linesize[3] = 0;

    dataObj_ptr->pic_ptr->pts = dataObj_ptr->tsMs;

    /*
     * encode and return size of encoded data buffer.
     */

    ret = avcodec_encode_video(dataObj_ptr->ctx_ptr,
            (void *)dataObj_ptr->outbuf,
            sizeof(dataObj_ptr->outbuf),
            dataObj_ptr->pic_ptr);
    dataObj_ptr->length = ret;

    if (ret <= 0) {
        dataObj_ptr->length = 0;
        return (0);
    }
#endif
    return(1);
}

/*
 * ======== _codecEncDequeueOutputBuffer() ========
 * This function retrieves data from the encoder.
 * Returns:
 *  1: Success.
 *  0: Failed (Invalid data).
 */
static vint _codecEncDequeueOutputBuffer(
    DataObj_Ptr    dataObj_ptr)
{
    /* Note: dabaObj_ptr->tsMs has been written by CAMERA. */

    if(dataObj_ptr->length == 0)    return (0);

    dataObj_ptr->rcsRtpExtnPayload = 0;
    return (1);
}

/*
 * ======== _CODEC_EncodeTask() ========
 *
 * Encoding task.
 *
 * Return Values:
 * None.
 */
static void _CODEC_EncodeTask(
    void *arg_ptr)
{
    CODEC_Ptr       codec_ptr;

    VC_EncodedFrame frame = {NULL, 0, 0, 0, 0};

    codec_ptr = (CODEC *)arg_ptr;

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

_CODEC_ENCODING_TASK_LOOP:
    /* Block on event message from VCI. */
    while (_CODEC_OK == codec_ptr->codecEncoder_ptr->codecDequeueOutputBuffer(&(codec_ptr->enc.data))) {
        frame.data_ptr          = codec_ptr->enc.data.outbuf;
        frame.length            = codec_ptr->enc.data.length;
        frame.flags             = codec_ptr->enc.data.flags;
        frame.tsMs              = codec_ptr->enc.data.tsMs;
        frame.rcsRtpExtnPayload = codec_ptr->enc.data.rcsRtpExtnPayload;
        VCI_sendEncodedFrame(&frame);
        usleep(20000);
    }

    usleep(20000);

    /* Loop as long as the encoder is initialized. */
    if (OSAL_TRUE == codec_ptr->enc.started) {
        goto _CODEC_ENCODING_TASK_LOOP;
    }
    else {
        OSAL_logMsg("%s:%d _CODEC_EncodeTask exited\n", __FILE__, __LINE__);
    }
}

/*
 * ======== _codecInit() ========
 * Initialize the ENC codec and start encodes.
 */
static void _codecInit(
    CODEC_Ptr   codec_ptr,
    vint        codecType)
{
#ifndef EXCLUDE_FFMPEG_LIBRARY
    AVCodec *avCodec_ptr;
#endif
    _CODEC_TaskObj *task_ptr = &codec_ptr->enc.task;

    int height = codec_ptr->enc.data.height;
    int width = codec_ptr->enc.data.width;

    CODEC_dbgPrintf("H263Encoder init start\n");

    codec_ptr->enc.started = OSAL_TRUE;
    codec_ptr->enc.codecType = codecType;

    OSAL_memSet(&(codec_ptr->enc.data), 0, sizeof(DataObj));

    if(codec_ptr->enc.data.codecInited) {
        OSAL_logMsg("%s:%d Encoder initialized before..\n", __FILE__, __LINE__);
        return;
    }
    /* Allocate input buffer for raw video frame. */
    codec_ptr->enc.data.buf_ptr = OSAL_memAlloc(
        width * height * 4, 0);
    if (NULL == codec_ptr->enc.data.buf_ptr) {
        DBG("FAILURE\n");
        return;
    }

#ifndef EXCLUDE_FFMPEG_LIBRARY
    avCodec_ptr = avcodec_find_encoder(CODEC_ID_H263);
    if (NULL == avCodec_ptr) {
        DBG("FAILURE\n");
        return;
    }

    codec_ptr->enc.data.ctx_ptr = avcodec_alloc_context();
    if (NULL == codec_ptr->enc.data.ctx_ptr) {
        DBG("FAILURE\n");
        return;
    }

    /*
     * resolution must be a multiple of two
     */
    codec_ptr->enc.data.ctx_ptr->width = width;
    codec_ptr->enc.data.ctx_ptr->height = height;
    codec_ptr->enc.data.ctx_ptr->rtp_payload_size = 512;
    codec_ptr->enc.data.ctx_ptr->bit_rate = 512000;

    /*
     * TS ticks per second, CAMERA provides in usec
     */
    codec_ptr->enc.data.ctx_ptr->time_base = (AVRational){1, 1000000};

    /*
     * H.263 supports only YUV420 planar.
     * Since planar, stored in YYYYYYYY..., VV..., UU... (YV12)
     * 4:2:0 is downsample of both U and V by 2 in horizontal and by 2 in
     * vertical direction
     */
    codec_ptr->enc.data.ctx_ptr->pix_fmt = PIX_FMT_YUV420P;
#endif

    if (OSAL_FAIL == OSAL_semAcquire(Codec_SemLock, OSAL_WAIT_FOREVER)) {
        return;
    }

#ifndef EXCLUDE_FFMPEG_LIBRARY
    if(avcodec_open(codec_ptr->enc.data.ctx_ptr, avCodec_ptr) < 0) {
        DBG("FAILURE\n");
        OSAL_semGive(Codec_SemLock);
        return;
    }
#endif

    if (OSAL_FAIL == OSAL_semGive(Codec_SemLock)) {
        return;
    }

    /*
     * New frame.
     */
#ifndef EXCLUDE_FFMPEG_LIBRARY
    codec_ptr->enc.data.pic_ptr = avcodec_alloc_frame();
    if (NULL == codec_ptr->enc.data.pic_ptr) {
        DBG("FAILURE\n");
        return;
    }
#endif

    DBG("FFMPEG H.263 encode init complete\n");

    codec_ptr->enc.data.codecInited = 1;

    CODEC_dbgPrintf("Encoder init complete\n");

    /* Init the task used to feed encoder. */
    task_ptr->taskId = 0;
    task_ptr->stackSize = _CODEC_TASK_STACK_SZ;
    task_ptr->taskPriority = _CODEC_TASK_ENC_PRIORITY;
    task_ptr->func_ptr = _CODEC_EncodeTask;

    OSAL_snprintf(task_ptr->name, sizeof(task_ptr->name), "%s",
            CODEC_DEC_TASK_NAME);

    task_ptr->arg_ptr = (void *)codec_ptr;

    if (0 == (task_ptr->taskId = OSAL_taskCreate(
        task_ptr->name,
        task_ptr->taskPriority,
        task_ptr->stackSize,
        task_ptr->func_ptr,
        task_ptr->arg_ptr))){
        codec_ptr->codecEncoder_ptr->codecRelease(codec_ptr);
        return;
    }
}

/*
 * ======== _codecRelease() ========
 * Terminate the encoder and release it.
 */
static void _codecRelease(
    CODEC_Ptr   codec_ptr)
{
    codec_ptr->enc.started = OSAL_FALSE;

    if(!codec_ptr->enc.data.codecInited) {
        return;
    }

#ifndef EXCLUDE_FFMPEG_LIBRARY
    if (NULL == codec_ptr->enc.data.ctx_ptr) {
        DBG("FAILURE\n");
        return;
    }
#endif

   /*
    * Stop all encoding activity.
    */
    OSAL_memFree(codec_ptr->enc.data.buf_ptr, 0);

    if (OSAL_FAIL == OSAL_semAcquire(Codec_SemLock, OSAL_WAIT_FOREVER)) {
        return;
    }
#ifndef EXCLUDE_FFMPEG_LIBRARY
    if(avcodec_close(codec_ptr->enc.data.ctx_ptr) < 0) {
        DBG("FAILURE\n");
        return;
    }
#endif
    if (OSAL_FAIL == OSAL_semGive(Codec_SemLock)) {
        return;
    }

#ifndef EXCLUDE_FFMPEG_LIBRARY
    av_free(codec_ptr->enc.data.pic_ptr);
    av_free(codec_ptr->enc.data.ctx_ptr);

    codec_ptr->enc.data.ctx_ptr = NULL;
    codec_ptr->enc.data.pic_ptr = NULL;
#endif
    DBG("FFMPEG H.263 encode shutdown complete\n");

    codec_ptr->enc.data.codecInited = 0;
}

const CodecObject _CodecH263Encoder = {
    _codecInit,                    /* codec initial */
    _codecEncQueueInputBuffer,     /* Received captured data from Camera */
    _codecEncDequeueOutputBuffer,  /* Get encoded data from encoder */
    NULL,                          /* Nothing to do on codecModify */
    NULL,                          /* Nothing to do on codecEncodeFIR */
    _codecRelease,                 /* codec terminate */
    OSAL_TRUE,                     /* Is this an encoder? */
};
