/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 5312 $ $Date: 2008-02-18 19:05:41 -0600 (Mon, 18 Feb 2008) $
 */

#include <string.h>
#include <stdio.h>
#include <osal.h>
#include "h263dec.h"

extern OSAL_SemId VID_dspLock;

#define DBG(fmt, args...) \
        OSAL_logMsg("%s %d:" fmt, __FILE__, __LINE__, ## args)

/*
 * ======== H263_dec() ========
 * This function feeds data to the decoder to run it. Data is recieved from
 * RTP.
 * Returns:
 *  1: Success.
 *  0: Failed (encoded buffer is not consumed).
 */
int H263_dec(
    H263_DecObj  *obj_ptr,
    Video_Packet *pkt_ptr)
{
#ifndef EXCLUDE_FFMPEG_LIBRARY
    AVPacket pkt;
#endif
    int ret = 0;

    if ((NULL == obj_ptr)
#ifndef EXCLUDE_FFMPEG_LIBRARY
            || (NULL == pkt_ptr)
#endif
            ) {
        DBG("FAILURE\n");
        return (0);
    }

    if (NULL == pkt_ptr->buf_ptr) {
        DBG("FAILURE\n");
        return (0);
    }

#ifndef EXCLUDE_FFMPEG_LIBRARY
    if (NULL == obj_ptr->ctx_ptr) {
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

#ifndef EXCLUDE_FFMPEG_LIBRARY
    av_init_packet(&pkt);
    pkt.pts = pkt_ptr->ts;
    pkt.size = pkt_ptr->sz;
    pkt.flags = 0;

    pkt.data = (void *)pkt_ptr->buf_ptr;
    pkt.stream_index = 0;
    pkt.duration = 0;
    pkt.priv = NULL;
    pkt.destruct = NULL;
    pkt.pos = -1;
    pkt.convergence_duration = 0;

    ret = avcodec_decode_video2(obj_ptr->ctx_ptr, obj_ptr->pic_ptr,
                    &obj_ptr->picReady, &pkt);
#endif

    return (ret);

}

/*
 * ======== H263_decGetData() ========
 * This function gets decoded data from the decoder.
 * Returns:
 *  1: Success.
 *  0: Failed (no buffer ready.).
 */
int H263_decGetData(
    H263_DecObj   *obj_ptr,
    Video_Picture *pic_ptr)
{
#ifndef EXCLUDE_FFMPEG_LIBRARY
    int xY;
    int h;
#endif

    if ((NULL == obj_ptr) || (NULL == pic_ptr)) {
        DBG("FAILURE\n");
        return (0);
    }

#ifndef EXCLUDE_FFMPEG_LIBRARY
    if (NULL == obj_ptr->ctx_ptr) {
        DBG("FAILURE\n");
        return (0);
    }
#endif

    pic_ptr->size = 0;
    pic_ptr->base_ptr = 0;

    if (0 == obj_ptr->picReady) {
        return (0);
    }

#ifndef EXCLUDE_FFMPEG_LIBRARY
    xY = obj_ptr->pic_ptr->linesize[0];
    h = obj_ptr->ctx_ptr->height;
    pic_ptr->size = (xY * h * 3) >> 1;
    if (pic_ptr->size > (VIDEO_WIDTH_MAX * VIDEO_HEIGHT_MAX * 4)) {
        /*
         * This will save against dynamic decoder data resolution buffer
         * overflow.
         */
        pic_ptr->size = 0;
        DBG("FAILURE\n");
        return (0);
    }

    memcpy(obj_ptr->outbuf_ptr, obj_ptr->pic_ptr->data[0], xY * h);
    memcpy(obj_ptr->outbuf_ptr + xY * h, obj_ptr->pic_ptr->data[1], xY * h >> 2);
    memcpy(obj_ptr->outbuf_ptr + xY * h + (xY * h >> 2),
            obj_ptr->pic_ptr->data[2], xY * h >> 2);

    pic_ptr->ts = obj_ptr->pic_ptr->pts;
    pic_ptr->base_ptr = obj_ptr->outbuf_ptr;

    pic_ptr->format = VIDEO_FORMAT_YCbCr_420_P;
    pic_ptr->width = xY;
    pic_ptr->height = h;
    pic_ptr->stride = xY;
#endif
    pic_ptr->bpp = 12;

    obj_ptr->picReady = 0;
    return (1);
}

/*
 * ======== H263_decInit() ========
 * This function inits the decoder and starts it.
 * Returns:
 * 0 : Success
 * -1 : Failure
 */
int H263_decInit(
    H263_DecObj    *obj_ptr,
    H263_DecParams *params_ptr)
{
    if ((NULL == obj_ptr) || (NULL == params_ptr)) {
        DBG("FAILURE\n");
        return (-1);
    }

    OSAL_memSet(obj_ptr, 0, sizeof(H263_DecObj));

    obj_ptr->outbuf_ptr = OSAL_memAlloc(
            VIDEO_WIDTH_MAX * VIDEO_HEIGHT_MAX * 4, 0);
    if (NULL == obj_ptr->outbuf_ptr) {
        DBG("FAILURE\n");
        return (-1);
    }

#ifndef EXCLUDE_FFMPEG_LIBRARY
    obj_ptr->codec_ptr = avcodec_find_decoder(CODEC_ID_H263);
    if (NULL == obj_ptr->codec_ptr) {
        DBG("FAILURE\n");
        goto _RETURN_FAIL_HANDLE;
    }

    obj_ptr->ctx_ptr = avcodec_alloc_context();
    if (NULL == obj_ptr->ctx_ptr) {
        DBG("FAILURE\n");
        goto _RETURN_FAIL_HANDLE;
    }
#endif

    if (OSAL_FAIL == OSAL_semAcquire(VID_dspLock, OSAL_WAIT_FOREVER)) {
        goto _RETURN_FAIL_CTX;
    }

#ifndef EXCLUDE_FFMPEG_LIBRARY
    if(avcodec_open(obj_ptr->ctx_ptr, obj_ptr->codec_ptr) < 0) {
        DBG("FAILURE\n");
        OSAL_semGive(VID_dspLock);
        goto _RETURN_FAIL_CTX;
    }
#endif

    if (OSAL_FAIL == OSAL_semGive(VID_dspLock)) {
        goto _RETURN_FAIL;
    }

    /*
     * New frame.
     */
#ifndef EXCLUDE_FFMPEG_LIBRARY
    obj_ptr->pic_ptr = avcodec_alloc_frame();
    if (NULL == obj_ptr->pic_ptr) {
        DBG("FAILURE\n");
        goto _RETURN_FAIL;
    }
#endif

    DBG("FFMPEG H.263 decode init complete\n");
    return (0);

_RETURN_FAIL:
#ifndef EXCLUDE_FFMPEG_LIBRARY
    avcodec_close(obj_ptr->ctx_ptr);
#endif
_RETURN_FAIL_CTX:
#ifndef EXCLUDE_FFMPEG_LIBRARY
    av_free(obj_ptr->ctx_ptr);
_RETURN_FAIL_HANDLE:
#endif
    OSAL_memFree(obj_ptr->outbuf_ptr, 0);
    return (-1);

}

/*
 * ======== H263_decShut() ========
 * This function shuts the decoder.
 * Returns:
 */
void H263_decShut(
    H263_DecObj *obj_ptr)
{
    if (NULL == obj_ptr) {
        DBG("FAILURE\n");
        return;
    }

#ifndef EXCLUDE_FFMPEG_LIBRARY
    if (NULL == obj_ptr->ctx_ptr) {
        DBG("FAILURE\n");
        return;
    }
#endif
    /*
     * Stop all decoding activity.
     */
    obj_ptr->sleepm = 1;

    OSAL_memFree(obj_ptr->outbuf_ptr, 0);

    if (OSAL_FAIL == OSAL_semAcquire(VID_dspLock, OSAL_WAIT_FOREVER)) {
        return;
    }
#ifndef EXCLUDE_FFMPEG_LIBRARY
    if(avcodec_close(obj_ptr->ctx_ptr) < 0) {
        DBG("FAILURE\n");
    }
#endif
    if (OSAL_FAIL == OSAL_semGive(VID_dspLock)) {
        return;
    }

#ifndef EXCLUDE_FFMPEG_LIBRARY
    av_free(obj_ptr->pic_ptr);
    av_free(obj_ptr->ctx_ptr);
    obj_ptr->ctx_ptr = NULL;
    obj_ptr->pic_ptr = NULL;
#endif
    DBG("FFMPEG H.263 decode shut down complete\n");

}
