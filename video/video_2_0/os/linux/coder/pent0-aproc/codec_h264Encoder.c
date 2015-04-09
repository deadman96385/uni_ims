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
    /* For image conversion */
    /* Size for YUV 420 */
    int array_size = dataObj_ptr->width * dataObj_ptr->height * 3 / 2;
    char tempBuf_ptr[array_size];

#ifndef EXCLUDE_FFMPEG_LIBRARY
    int sizeY;
    int sizeUV;
    x264_nal_t     *nal_ptr;
    x264_picture_t  pic;
    int             nnal;
    int             ret;
    int             i;
#endif
    if (NULL == dataObj_ptr) {
        DBG("FAILURE\n");
        return (0);
    }
#ifndef EXCLUDE_FFMPEG_LIBRARY
    if (NULL == dataObj_ptr->enc_ptr) {
        DBG("FAILURE\n");
        return (0);
    }
#endif

    IMG_formatConvert((void *)(void *)tempBuf_ptr,
        dataObj_ptr->width, dataObj_ptr->height, VIDEO_FORMAT_YCbCr_420_P,
        (void *)dataObj_ptr->buf_ptr,
        dataObj_ptr->width, dataObj_ptr->height, dataObj_ptr->format);

#ifndef EXCLUDE_FFMPEG_LIBRARY
    /*
     * Size of Y component, for 4:2:0, there is downsampling in X and Y
     * dimensions, so U size is 1/4 of Y size. Same for V size.
     * ???????????
     */
    sizeY = dataObj_ptr->width * dataObj_ptr->height;
    sizeUV = (dataObj_ptr->width >> 1) * (dataObj_ptr->height >> 1);
    pic.img.i_csp = X264_CSP_I420;
    pic.img.i_plane = 3;
    pic.i_pts = dataObj_ptr->tsMs;

    pic.i_type = (dataObj_ptr->sendFIR)? X264_TYPE_KEYFRAME : X264_TYPE_AUTO;
    if(dataObj_ptr->sendFIR) {
        pic.i_type = X264_TYPE_KEYFRAME;
        dataObj_ptr->sendFIR = OSAL_FALSE;
    }
    else {
        pic.i_type = X264_TYPE_AUTO;
    }

    pic.img.plane[0] = (void *)tempBuf_ptr;
    pic.img.i_stride[0] = dataObj_ptr->width;
    pic.img.plane[1] = (void *)(tempBuf_ptr + sizeY);
    pic.img.i_stride[1] = dataObj_ptr->width / 2;
    pic.img.plane[2] = (void *)(tempBuf_ptr + sizeY + sizeUV);
    pic.img.i_stride[2] = dataObj_ptr->width / 2;
    ret = x264_encoder_encode(dataObj_ptr->enc_ptr, &nal_ptr, &nnal, &pic,
         &dataObj_ptr->picOut);
    dataObj_ptr->length = 0;
    dataObj_ptr->tsMs = dataObj_ptr->picOut.i_pts;
    if (ret <= 0) {
        return (0);
    }
    memset(dataObj_ptr->outbuf, 0, sizeof(dataObj_ptr->outbuf));

    /* Copy encoded data into output buffer. */
    for (i = 0; i < nnal; i++) {
        if ((dataObj_ptr->length + nal_ptr->i_payload) >
                (int)sizeof(dataObj_ptr->outbuf)) {
            DBG("Increase buffer size\n");
            return (0);
        }
        memcpy(&dataObj_ptr->outbuf[dataObj_ptr->length], nal_ptr->p_payload,
            nal_ptr->i_payload);
        dataObj_ptr->length += nal_ptr->i_payload;
        nal_ptr++;
    }
#endif
    return(1);
}

/*
 * ======== _codecEncDequeueOutputBuffer() ========
 * This function retrieves data from the encoder.
 * Returns:
 *  1: Success.
 *  0: Failed (encoded buffer is not consumed).
 */
static vint _codecEncDequeueOutputBuffer(
    DataObj_Ptr    dataObj_ptr)
{

    /* Note: dabaObj_ptr->tsMs has been written by x264_encoder. */

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
    _CODEC_TaskObj *task_ptr = &codec_ptr->enc.task;
    CODEC_dbgPrintf("H264Encoder init start\n");

    int height = codec_ptr->enc.data.height;
    int width = codec_ptr->enc.data.width;

    codec_ptr->enc.started = OSAL_TRUE;
    codec_ptr->enc.codecType = codecType;

    OSAL_memSet(&(codec_ptr->enc.data), 0, sizeof(DataObj));

    if(codec_ptr->enc.data.codecInited) {
        OSAL_logMsg("%s:%d Encoder initialized before..\n", __FILE__, __LINE__);
        return;
    }
#ifndef EXCLUDE_FFMPEG_LIBRARY
    x264_param_t params;
    x264_param_t *p_ptr = &params;
#endif
    if (NULL == &(codec_ptr->enc.data)) {
        DBG("FAILURE\n");
        return;
    }

    /* Allocate input buffer for raw video frame. */
    codec_ptr->enc.data.buf_ptr = OSAL_memAlloc(
            width * height * 4, 0);
    if (NULL == codec_ptr->enc.data.buf_ptr) {
        DBG("FAILURE\n");
        goto _RETURN_FAIL;
    }

    codec_ptr->enc.data.width = width;
    codec_ptr->enc.data.height = height;

#ifndef EXCLUDE_FFMPEG_LIBRARY
    memset(p_ptr, 0, sizeof(params));

    x264_param_default(p_ptr);
#endif

#ifndef EXCLUDE_FFMPEG_LIBRARY
#if 1
    if (x264_param_default_preset(p_ptr, "ultrafast", "zerolatency") < 0) {
        goto _RETURN_FAIL;
    }
    if (x264_param_apply_profile(p_ptr, "baseline") < 0)  {
        goto _RETURN_FAIL;
    }

    p_ptr->i_csp = X264_CSP_I420;
    p_ptr->i_level_idc = 31;
    p_ptr->i_timebase_num = 1;
    p_ptr->i_timebase_den = 1000000;
    p_ptr->i_keyint_max = 30;

#else
    int i;
    p_ptr->i_threads = 0;
    p_ptr->b_sliced_threads = 1;
    p_ptr->b_deterministic = 1;
    p_ptr->i_sync_lookahead = 0;
    p_ptr->i_csp = 1;
    p_ptr->i_level_idc = 12;
    p_ptr->i_frame_total = 0;
    p_ptr->i_nal_hrd = 0;

    p_ptr->vui.i_sar_height = 0;
    p_ptr->vui.i_sar_width = 0;
    p_ptr->vui.i_overscan = 0;
    p_ptr->vui.i_vidformat = 5;
    p_ptr->vui.b_fullrange = 0;
    p_ptr->vui.i_colorprim = 2;
    p_ptr->vui.i_transfer = 2;
    p_ptr->vui.i_colmatrix = 2;
    p_ptr->vui.i_chroma_loc = 0;

    p_ptr->i_frame_reference = 2;
    p_ptr->i_keyint_max = 250;
    p_ptr->i_keyint_min = 0;
    p_ptr->i_scenecut_threshold = 40;
    p_ptr->b_intra_refresh = 0;
    p_ptr->i_bframe = 0;
    p_ptr->i_bframe_adaptive = 1;
    p_ptr->i_bframe_bias = 0;
    p_ptr->i_bframe_pyramid = 2;
    p_ptr->b_deblocking_filter = 1;
    p_ptr->i_deblocking_filter_alphac0 = 0;
    p_ptr->i_deblocking_filter_beta = 0;
    p_ptr->b_cabac = 0;
    p_ptr->i_cabac_init_idc = 0;
    p_ptr->b_interlaced = 0;
    p_ptr->b_constrained_intra = 0;
    p_ptr->i_cqm_preset = 0;
    p_ptr->psz_cqm_file = 0x0;
    for (i = 0; i < 16; i++) {
        p_ptr->cqm_4iy[i] =  p_ptr->cqm_4ic[i] = p_ptr->cqm_4py[i] =  p_ptr->cqm_4pc[i]
                = '\020';
    }
    for (i = 0; i < 64; i++) {
        p_ptr->cqm_8iy[i] =  p_ptr->cqm_8py[i] = '\020';
    }

    p_ptr->p_log_private = 0x0;
    p_ptr->i_log_level = 2;
    p_ptr->b_visualize = 0;
    p_ptr->psz_dump_yuv = 0x0;
    p_ptr->analyse.intra = 3;
    p_ptr->analyse.inter = 275;
    p_ptr->analyse.b_transform_8x8 = 0;
    p_ptr->analyse.i_weighted_pred = 0;
    p_ptr->analyse.b_weighted_bipred = 1;
    p_ptr->analyse.i_direct_mv_pred = 1;
    p_ptr->analyse.i_chroma_qp_offset = 0;
    p_ptr->analyse.i_me_method = 1;
    p_ptr->analyse.i_me_range = 16;
    p_ptr->analyse.i_mv_range = -1;
    p_ptr->analyse.i_mv_range_thread = -1;
    p_ptr->analyse.i_subpel_refine = 6;
    p_ptr->analyse.b_chroma_me = 1;
    p_ptr->analyse.b_mixed_references = 1;
    p_ptr->analyse.i_trellis = 1;
    p_ptr->analyse.b_fast_pskip = 1;
    p_ptr->analyse.b_dct_decimate = 1;
    p_ptr->analyse.i_noise_reduction = 0;
    p_ptr->analyse.f_psy_rd = 1;
    p_ptr->analyse.f_psy_trellis = 0;
    p_ptr->analyse.b_psy = 1;
    p_ptr->analyse.i_luma_deadzone[0] = 21;
    p_ptr->analyse.i_luma_deadzone[1] = 11;
    p_ptr->analyse.b_psnr = 0;
    p_ptr->analyse.b_ssim = 0;

    p_ptr->rc.i_rc_method = 2;
    p_ptr->rc.i_qp_constant = 23;
    p_ptr->rc.i_qp_min = 10;
    p_ptr->rc.i_qp_max = 51;
    p_ptr->rc.i_qp_step = 4;
    p_ptr->rc.i_bitrate = 80;
    p_ptr->rc.f_rf_constant = 23;
    p_ptr->rc.f_rf_constant_max = 0;
    p_ptr->rc.f_rate_tolerance = 1;
    p_ptr->rc.i_vbv_max_bitrate = 0;
    p_ptr->rc.i_vbv_buffer_size = 0;
    p_ptr->rc.f_vbv_buffer_init = 0.899999976;
    p_ptr->rc.f_ip_factor = 1.39999998;
    p_ptr->rc.f_pb_factor = 1.29999995;
    p_ptr->rc.i_aq_mode = 1;
    p_ptr->rc.f_aq_strength = 1;
    p_ptr->rc.b_mb_tree = 1;
    p_ptr->rc.i_lookahead = 40;
    p_ptr->rc.b_stat_write = 0;
    p_ptr->rc.psz_stat_out = "x264_2pass.log";
    p_ptr->rc.b_stat_read = 0;
    p_ptr->rc.psz_stat_in = "x264_2pass.log";
    p_ptr->rc.f_qcompress = 0.600000024;
    p_ptr->rc.f_qblur = 0.5;
    p_ptr->rc.f_complexity_blur = 20;
    p_ptr->rc.zones = 0x0;
    p_ptr->rc.i_zones = 0;
    p_ptr->rc.psz_zones = 0x0;

    p_ptr->b_aud = 0;
    p_ptr->b_repeat_headers = 1;
    p_ptr->b_annexb = 1;
    p_ptr->i_sps_id = 0;
    p_ptr->b_vfr_input = 0;
    p_ptr->i_fps_num = 10;
    p_ptr->i_fps_den = 1;
    p_ptr->i_timebase_num = 1;
    p_ptr->i_timebase_den = 10;
    p_ptr->b_dts_compress = 0;
    p_ptr->b_tff = 1;
    p_ptr->b_pic_struct = 0;
    p_ptr->i_slice_max_size = 0;
    p_ptr->i_slice_max_mbs = 0;
    p_ptr->i_slice_count = 0;
    p_ptr->param_free = 0;
#endif

    p_ptr->i_width = width;
    p_ptr->i_height = height;

    /*
     * Default rate control is 1(CRF).
     * bitrate's unit is KBps.
     * x264_encoder_parameters() could check settings.
     */
    p_ptr->rc.i_vbv_max_bitrate = 600;
    p_ptr->rc.i_vbv_buffer_size = 600;
    p_ptr->rc.f_vbv_buffer_init = 600;

    codec_ptr->enc.data.enc_ptr = x264_encoder_open(p_ptr);
    if(NULL == codec_ptr->enc.data.enc_ptr) {
        DBG("FAILURE\n");
        goto _RETURN_FAIL;
    }
#endif
    DBG("X264 H.264 encode init complete\n");
    goto _INIT_ENCODE_COMPLETE;

_RETURN_FAIL:
    OSAL_memFree(codec_ptr->enc.data.buf_ptr, 0);
    return;

_INIT_ENCODE_COMPLETE:
    codec_ptr->enc.data.codecInited = 1;
    codec_ptr->enc.data.sendFIR = OSAL_FALSE;

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
void _codecModify(
    CODEC_Ptr  codec_ptr)
{
#ifndef EXCLUDE_FFMPEG_LIBRARY
    x264_param_t getParams;
    x264_param_t *get_ptr = &getParams;
    x264_encoder_parameters(codec_ptr->enc.data.enc_ptr, get_ptr);

    get_ptr->rc.i_bitrate = get_ptr->rc.i_vbv_max_bitrate =
            get_ptr->rc.i_vbv_buffer_size = codec_ptr->enc.data.maxBitrate;

    x264_encoder_reconfig(codec_ptr->enc.data.enc_ptr, get_ptr);
#endif
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
    x264_nal_t *nal_ptr;
    int nnal;
#endif

#ifndef EXCLUDE_FFMPEG_LIBRARY
    if (NULL == codec_ptr->enc.data.enc_ptr) {
        DBG("FAILURE\n");
        return;
    }
#endif

    /*
     * Stop all encoding activity.
     */
    OSAL_memFree(codec_ptr->enc.data.buf_ptr, 0);

#ifndef EXCLUDE_FFMPEG_LIBRARY
    while (x264_encoder_delayed_frames(codec_ptr->enc.data.enc_ptr)) {
        x264_encoder_encode(codec_ptr->enc.data.enc_ptr, &nal_ptr, &nnal, NULL,
                &codec_ptr->enc.data.picOut);
    }

    x264_encoder_close(codec_ptr->enc.data.enc_ptr);
    codec_ptr->enc.data.enc_ptr = NULL;
#endif
    DBG("X264 H.264 encode shutdown complete\n");

    codec_ptr->enc.data.codecInited = 0;
}

const CodecObject _CodecH264Encoder = {
    _codecInit,                    /* codec initial */
    _codecEncQueueInputBuffer,     /* Received captured data from Camera */
    _codecEncDequeueOutputBuffer,  /* Get encoded data from encoder */
    _codecModify,                  /* Notify ENC to set maximum bitrate */
    NULL,                          /* Nothing need to do */
    _codecRelease,                 /* codec terminate */
    OSAL_TRUE,                     /* Is this an encoder? */
};
