/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 5312 $ $Date: 2008-02-18 19:05:41 -0600 (Mon, 18 Feb 2008) $
 */

#include <osal.h>
#include "h264enc.h"
#include <dsp_scale.h>
#include <video.h>


#define DBG(fmt, args...) \
        OSAL_logMsg("%s %d:" fmt, __FILE__, __LINE__, ## args)

/*
 * ======== _H264_encFindFullPacket() ========
 * This function finds a packet in H.264 stream. Start of a packet is marked by
 * start code sequence.
 * in_ptr points to the start of bitstream
 * end_ptr points to the end of bitstream
 * start1_ptr will be set to the start of packet (start sequence removed), or
 *  NULL if no packet found
 * start2_ptr will be set to the end of packet or NULL if packet end is end of
 *  bitsream.
 * Returns:
 */
static void _H264_encFindFullPacket(
    uint8  *in_ptr,
    uint8  *end_ptr,
    uint8 **start1_ptr,
    uint8 **start2_ptr)
{
    *start1_ptr = NULL;
    *start2_ptr = NULL;

    /*
     *  Find start code 23 0 bits and a 1
     */
    while (in_ptr < (end_ptr - 3)) {
        if ((in_ptr[0] == 0) &&
                (in_ptr[1] == 0) &&
                (in_ptr[2] == 1)) {
            *start1_ptr = &in_ptr[3];
            break;
        }
        in_ptr++;
    }

    /*
     * Now start1_ptr points to the next start code.
     * If start sequence not found then return.
     */
    if (NULL == *start1_ptr) {
        return;
    }
    
    in_ptr = *start1_ptr;

    /*
     * Find next start code
     */
    while (in_ptr < (end_ptr - 3)) {
        if ((in_ptr[0] == 0) &&
                (in_ptr[1] == 0) &&
                (in_ptr[2] == 1)) {
            if (0 == in_ptr[-1]) {
                /*
                 * For 4 byte start code
                 */
                *start2_ptr = &in_ptr[-1];
            }
            else {
                *start2_ptr = &in_ptr[0];
            }
            break;
        }
        in_ptr++;
    }

    /*
     * Now start2_ptr points to the next start code (or its NULL).
     */
    if (NULL == *start2_ptr) {
        *start2_ptr = end_ptr;
    }
}


/*
 * ======== H264_enc() ========
 * This function feeds data to the encoder to run it.
 * Returns:
 *  1: Success.
 *  0: Failed (picture buffer is not consumed).
 */
int H264_enc(
    H264_EncObj   *obj_ptr,
    Video_Picture *pic_ptr)
{
    int sizeY;
    int sizeUV;
#ifndef EXCLUDE_FFMPEG_LIBRARY
    int ret;
    x264_nal_t *nal_ptr;
    int nnal;
    int i;
#endif

    if ((NULL == obj_ptr) || (NULL == pic_ptr)) {
        DBG("FAILURE\n");
        return (0);
    }
#ifndef EXCLUDE_FFMPEG_LIBRARY
    if (NULL == obj_ptr->enc_ptr) {
        DBG("FAILURE\n");
        return (0);
    }
#endif

    if (obj_ptr->sleepm) {
        /*
         * Sleeping, discard packet.
         */
        DBG("FAILURE\n");
        return (0);
    }

    /*
     * Size of Y component, for 4:2:0, there is downsampling in X and Y
     * dimensions, so U size is 1/4 of Y size. Same for V size.
     * ???????????
     */
    sizeY = obj_ptr->width * obj_ptr->height;
    sizeUV = (obj_ptr->width >> 1) * (obj_ptr->height >> 1);

    if (!pic_ptr->muted) {
        Dsp_scale((void *)(void *)obj_ptr->inbuf_ptr,
                obj_ptr->width, obj_ptr->height, VIDEO_FORMAT_YCbCr_420_P,
                (void *)pic_ptr->base_ptr,
                pic_ptr->width, pic_ptr->height, pic_ptr->format);
    }
    else {
        memset((uint8 *)obj_ptr->inbuf_ptr, 16, sizeY);
        memset((uint8 *)obj_ptr->inbuf_ptr + sizeY, 128, sizeUV * 2);
    }

#ifndef EXCLUDE_FFMPEG_LIBRARY
    obj_ptr->pic.img.i_csp = X264_CSP_I420;
    obj_ptr->pic.img.i_plane = 3;
    obj_ptr->pic.i_pts = pic_ptr->ts;
    obj_ptr->pic.i_type = X264_TYPE_AUTO;

    obj_ptr->pic.img.plane[0] = (void *)obj_ptr->inbuf_ptr;
    obj_ptr->pic.img.i_stride[0] = obj_ptr->width;
    obj_ptr->pic.img.plane[1] = (void *)(obj_ptr->inbuf_ptr + sizeY);
    obj_ptr->pic.img.i_stride[1] = obj_ptr->width / 2;
    obj_ptr->pic.img.plane[2] = (void *)(obj_ptr->inbuf_ptr + sizeY + sizeUV);
    obj_ptr->pic.img.i_stride[2] = obj_ptr->width / 2;

    ret = x264_encoder_encode(obj_ptr->enc_ptr, &nal_ptr, &nnal, &obj_ptr->pic,
            &obj_ptr->picOut);

    obj_ptr->len = 0;
    obj_ptr->pts = obj_ptr->picOut.i_pts;
    if (ret <= 0) {
        return (0);
    }

    memset(obj_ptr->outbuf, 0, sizeof(obj_ptr->outbuf));
    for (i = 0; i < nnal; i++) {
        if ((obj_ptr->len + nal_ptr->i_payload) > (int)sizeof(obj_ptr->outbuf)) {
            /*
             *
             */
            DBG("Increase buffer size\n");
            return (0);
        }
        memcpy(&obj_ptr->outbuf[obj_ptr->len], nal_ptr->p_payload,
                nal_ptr->i_payload);
        obj_ptr->len += nal_ptr->i_payload;
        nal_ptr++;
    }
#endif
    return(1);
}


/*
 * ======== H264_encGetData() ========
 * This function gets encoded data from the encoder.
 * In pkt_ptr:
 * buf_ptr will be written to with packet data (buf_ptr must point to memory of
 *  minimum storage H264_MAX_PKT_SIZE.
 * sz will be written with packet size.
 * ts will be written with packet time stamp.
 * mark will be written with mark bit for the packet.
 * Returns:
 *  1: Success.
 *  0: Failed (no buffer ready.).
 */
int H264_encGetData(
    H264_EncObj   *obj_ptr,
    Video_Packet  *pkt_ptr)
{
    int i = -1;
    int index;
    int consumed;
    int sz;
    uint8 hdr[2];
    uint8 *start1_ptr;
    uint8 *start2_ptr;
    uint8 *buf_ptr;
    int nal;
    int end;

    if ((NULL == obj_ptr) || (NULL == pkt_ptr)) {
        DBG("FAILURE\n");
        return (0);
    }

    if (NULL == pkt_ptr->buf_ptr) {
        DBG("FAILURE\n");
        return (0);
    }

#ifndef EXCLUDE_FFMPEG_LIBRARY
    if (NULL == obj_ptr->enc_ptr) {
        DBG("FAILURE\n");
        return (0);
    }
#endif

    if (obj_ptr->sleepm) {
        return (0);
    }

    buf_ptr = (uint8*)obj_ptr->outbuf;

    if (obj_ptr->len <= 0) {
        return (0);
    }

    end = 0;

    if (obj_ptr->pkt.num < 0) {

        /*
         * Got a new buffer.
         */
        obj_ptr->pkt.num = 0;
        memset(&obj_ptr->pkt, 0, sizeof(obj_ptr->pkt));
        obj_ptr->pkt.buf_ptr = buf_ptr;
        obj_ptr->pkt.end_ptr = buf_ptr + obj_ptr->len;
        obj_ptr->pkt.ts = obj_ptr->pts / 11; /* 90000 */
        /*
         * Run a loop to find packets in a stream.
         */
        i = 0;
        _H264_encFindFullPacket(
                obj_ptr->pkt.buf_ptr,
                obj_ptr->pkt.end_ptr,
                &obj_ptr->pkt.start1_ptr[i],
                &obj_ptr->pkt.start2_ptr[i]);

        if (NULL == obj_ptr->pkt.start1_ptr[i])  {
            /*
             * Nothing.
             */
            obj_ptr->len = 0;
            obj_ptr->pkt.num = -1;
            return (0);
        }
      
        while (
                (NULL != obj_ptr->pkt.start1_ptr[i]) &&
                (NULL != obj_ptr->pkt.start2_ptr[i])
                ) {
            i++;
            if (i >= H264_ENC_MAX_PKTS) {
                DBG("Losing data, increase packet buffer\n");
                break;
            }
            _H264_encFindFullPacket(
                obj_ptr->pkt.start2_ptr[i - 1],
                obj_ptr->pkt.end_ptr,
                &obj_ptr->pkt.start1_ptr[i],
                &obj_ptr->pkt.start2_ptr[i]);
        }

        /*
         * Now we have a list of packets with start sequence removed.
         */
        obj_ptr->pkt.index = 0;
        obj_ptr->pkt.consumed = 0;
    }

    /*
     * Put some logic here to get H.264 RTP packets from H.264 packets.
     */
    index = obj_ptr->pkt.index;
    start1_ptr = obj_ptr->pkt.start1_ptr[index];
    start2_ptr = obj_ptr->pkt.start2_ptr[index];
    if ((NULL == start1_ptr) || (NULL == start2_ptr)) {
        /*
         * If no more data in buffer, mark it free, return.
         */
        obj_ptr->pkt.num = -1;
        return (0);
    }

    sz = start2_ptr - start1_ptr;
    consumed = obj_ptr->pkt.consumed;
    nal = start1_ptr[0] & 0x1F;
    if (sz <= H264_ENC_MAX_RTP_SIZE) {
        /*
         * Will fit entirely in one RTP packet.
         */
        memcpy(pkt_ptr->buf_ptr, start1_ptr, sz);
        pkt_ptr->sz = sz;
        pkt_ptr->ts = obj_ptr->pkt.ts;

        index++;
        obj_ptr->pkt.index = index;
        obj_ptr->pkt.consumed = 0;
        /*
         * If no more packets in this frame, mark it.
         */
        if (NULL == obj_ptr->pkt.start1_ptr[index]) {
            pkt_ptr->mark = 1;
        }
        else {
            pkt_ptr->mark = 0;
        }
        obj_ptr->lastType = nal;
        return (1);
    }

    /*
     * Will need fragmenting.
     * Make header.
     */
    hdr[0] = 28 | (start1_ptr[0] & 0x60);  /* FU - A, Type = 28, and NRI */
    hdr[1] = start1_ptr[0] & 0x1F;         /* Actual type */

    if (0 == consumed) {
        /*
         * First packet.
         */
        hdr[1] |= 1 << 7;        /* set start bit */
        memcpy(pkt_ptr->buf_ptr, hdr, 2);
        memcpy((uint8 *)pkt_ptr->buf_ptr + 2, start1_ptr + 1, H264_ENC_MAX_RTP_SIZE - 2);
        pkt_ptr->sz = H264_ENC_MAX_RTP_SIZE;
        pkt_ptr->mark = 0;
        obj_ptr->pkt.consumed += H264_ENC_MAX_RTP_SIZE - 1;
    }
    else if ((consumed + H264_ENC_MAX_RTP_SIZE - 2) < sz) {
        /*
         * Intermediate packet.
         */
        memcpy(pkt_ptr->buf_ptr, hdr, 2);
        memcpy((uint8 *)pkt_ptr->buf_ptr + 2, start1_ptr + consumed, H264_ENC_MAX_RTP_SIZE - 2);
        pkt_ptr->sz = H264_ENC_MAX_RTP_SIZE;
        pkt_ptr->mark = 0;
        obj_ptr->pkt.consumed += H264_ENC_MAX_RTP_SIZE - 2;
    }
    else {
        /*
         * End packet.
         */
        hdr[1] |= 1 << 6;        /* set end bit */
        memcpy(pkt_ptr->buf_ptr, hdr, 2);
        memcpy((uint8 *)pkt_ptr->buf_ptr + 2, start1_ptr + consumed, sz - consumed);
        pkt_ptr->sz = sz - consumed + 2;
        obj_ptr->pkt.consumed = 0;
        index++;
        obj_ptr->pkt.index = index;
        /*
         * If no more packets in this frame, mark it.
         */
        if (NULL == obj_ptr->pkt.start1_ptr[index]) {
            pkt_ptr->mark = 1;
        }
        else {
            pkt_ptr->mark = 0;
        }
        end = 1;
    }

    pkt_ptr->ts = obj_ptr->pkt.ts;

    if (end) {
        obj_ptr->lastType = nal;
    }

    return (1);
}

/*
 * ======== H264_encInit() ========
 * This function inits the encoder and starts it.
 * Returns:
 *  0: OK
 * -1: FAIL
 */
int H264_encInit(
        H264_EncObj    *obj_ptr,
        H264_EncParams *params_ptr)
{
#ifndef EXCLUDE_FFMPEG_LIBRARY
    x264_param_t *p_ptr = &obj_ptr->params;
#endif
    if ((NULL == obj_ptr) || (NULL == params_ptr)) {
        DBG("FAILURE\n");
        return (-1);
    }

    OSAL_memSet(obj_ptr, 0, sizeof(H264_EncObj));

    memcpy(&obj_ptr->paramCache, params_ptr, sizeof(obj_ptr->paramCache));

    obj_ptr->inbuf_ptr = OSAL_memAlloc(
            params_ptr->width * params_ptr->height * 4, 0);
    if (NULL == obj_ptr->inbuf_ptr) {
        DBG("FAILURE\n");
        goto _RETURN_FAIL;
    }

    obj_ptr->width = params_ptr->width;
    obj_ptr->height = params_ptr->height;
#ifndef EXCLUDE_FFMPEG_LIBRARY
    memset(p_ptr, 0, sizeof(obj_ptr->params));
    
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

    p_ptr->i_width = params_ptr->width;
    p_ptr->i_height = params_ptr->height;

    obj_ptr->enc_ptr = x264_encoder_open(p_ptr);
    if(NULL == obj_ptr->enc_ptr) {
        DBG("FAILURE\n");
        goto _RETURN_FAIL;
    }
#endif
    obj_ptr->pkt.num = -2;

    DBG("X264 H.264 encode init complete\n");

    return (0);

_RETURN_FAIL:
    OSAL_memFree(obj_ptr->inbuf_ptr, 0);

    return (-1);

}

/*
 * ======== H264_encodeShut() ========
 * This function shuts the encoder.
 * Returns:
 */
void H264_encShut(
    H264_EncObj *obj_ptr)
{
#ifndef EXCLUDE_FFMPEG_LIBRARY
    x264_nal_t *nal_ptr;
    int nnal;
#endif

    if (NULL == obj_ptr) {
        DBG("FAILURE\n");
        return;
    }

#ifndef EXCLUDE_FFMPEG_LIBRARY
    if (NULL == obj_ptr->enc_ptr) {
        DBG("FAILURE\n");
        return;
    }
#endif

    /*
     * Stop all encoding activity.
     */
    obj_ptr->sleepm = 1;

    OSAL_memFree(obj_ptr->inbuf_ptr, 0);

#ifndef EXCLUDE_FFMPEG_LIBRARY
    while (x264_encoder_delayed_frames(obj_ptr->enc_ptr)) {
        x264_encoder_encode(obj_ptr->enc_ptr, &nal_ptr, &nnal, NULL,
                &obj_ptr->picOut);
    }

    x264_encoder_close(obj_ptr->enc_ptr);
    obj_ptr->enc_ptr = NULL;
#endif
    DBG("X264 H.264 encode shutdown complete\n");
}

/*
 * ======== H264_encRequestKeyFrame() ========
 * Key frame request (async)
 * Returns:
 * 0 : Success
 * -1 : Failure
 */
int H264_encRequestKeyFrame(
    H264_EncObj *obj_ptr)
{
    if (NULL == obj_ptr) {
        DBG("FAILURE\n");
        return (-1);
    }
#ifndef EXCLUDE_FFMPEG_LIBRARY
    if (NULL == obj_ptr->enc_ptr) {
        DBG("FAILURE\n");
        return (-1);
    }
    x264_encoder_intra_refresh(obj_ptr->enc_ptr);
#endif
    return (0);
}

/*
 * ======== H264_encRequestResolution() ========
 * Res change request. Scaling is available.
 * Returns:
 * 0 : Success
 * -1 : Failure
 */
int H264_encRequestResolution(
    H264_EncObj *obj_ptr,
    int          width,
    int          height)
{
    H264_EncParams params;

    if (NULL == obj_ptr) {
        return (-1);
    }
#ifndef EXCLUDE_FFMPEG_LIBRARY
    if (NULL == obj_ptr->enc_ptr) {
        return (-1);
    }
#endif

    if ((width <= 0) || (height < 0)) {
        return (-1);
    }

    memcpy(&params, &obj_ptr->paramCache, sizeof(params));
    params.width = width;
    params.height = height;
    H264_encShut(obj_ptr);
    
    return (H264_encInit(obj_ptr, &params));
}
