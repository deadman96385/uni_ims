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
#include <video.h>
#include <dsp_scale.h>
#include "h263enc.h"

extern OSAL_SemId VID_dspLock;

#define DBG(fmt, args...) \
        OSAL_logMsg("%s %d:" fmt, __FILE__, __LINE__, ## args)

/*
 * ======== _H263_encFindGobStart() ========
 * This function finds start position of a GOB.
 * in_ptr points to the start of bitstream
 * end_ptr points to the end of bitstream
 * start1_ptr will be set to the start of packet (start sequence removed), or
 *  NULL if no packet found
 * start2_ptr will be set to the end of packet or NULL if packet end is end of
 *  bitsream.
 * Returns:
 */
static void _H263_encFindGobStart(
    uint8  *in_ptr,
    uint8  *end_ptr,
    uint8 **start1_ptr,
    uint8 **start2_ptr)
{
    *start1_ptr = NULL;
    *start2_ptr = NULL;

    /*
     *  Find start code 16 0 bits and a 1 (its not byte aligned)
     */
    while (in_ptr < (end_ptr - 3)) {
    /*
     * FFMPEG byte aligns.
     */
#if 0
        if (0 == in_ptr[1]) {
            /*
             * Need to find 8 more 0 bits in in_ptr[0] and in_ptr[2]
             */

            byte = in_ptr[0];
            
            if (byte & 1) {
                pre = 0;
            }
            else if (byte & 2) {
                pre = 1;
            }
            else if (byte & 4) {
                pre = 2;
            }
            else if (byte & 8) {
                pre = 3;
            }
            else if (byte & 16) {
                pre = 4;
            }
            else if (byte & 32) {
                pre = 5;
            }
            else if (byte & 64) {
                pre = 6;
            }
            else if (byte & 128) {
                pre = 7;
            }
            else {
                pre = 8;
            }

            byte = in_ptr[2];
            if (byte & 128) {
                post = 0;
            }
            else if (byte & 64) {
                post = 1;
            }
            else if (byte & 32) {
                post = 2;
            }
            else if (byte & 16) {
                post = 3;
            }
            else if (byte & 8) {
                post = 4;
            }
            else if (byte & 4) {
                post = 5;
            }
            else if (byte & 2) {
                post = 6;
            }
            else if (byte & 1) {
                post = 7;
            }
            else {
                post = 8;
            }
        }
#else

        if (0 == in_ptr[0] && (0 == in_ptr[1]) && (0x80 & in_ptr[2])) {
            *start1_ptr = &in_ptr[0];
            break;
        }

#endif
        in_ptr++;
    }
    
    in_ptr += 3;
    
    while (in_ptr < (end_ptr - 3)) {
        if (0 == in_ptr[0] && (0 == in_ptr[1]) && (0x80 & in_ptr[2])) {
            *start2_ptr = &in_ptr[0];
            break;
        }

        in_ptr++;
    }

    if (NULL != *start1_ptr) {
        if (NULL == *start2_ptr) {
            *start2_ptr = end_ptr;
        }
    }
}


/*
 * ======== H263_enc() ========
 * This function feeds data to the encoder to run it.
 * Returns:
 *  1: Success.
 *  0: Failed (picture buffer is not consumed).
 */
int H263_enc(
    H263_EncObj *obj_ptr,
    Video_Picture  *pic_ptr)
{
#ifndef EXCLUDE_FFMPEG_LIBRARY
    int sizeY;
    int sizeUV;
    int ret;
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
#ifndef EXCLUDE_FFMPEG_LIBRARY
    sizeY = obj_ptr->ctx_ptr->width * obj_ptr->ctx_ptr->height;
    sizeUV = (obj_ptr->ctx_ptr->width >> 1) * (obj_ptr->ctx_ptr->height >> 1);

    if (!pic_ptr->muted) {
        Dsp_scale((void *)(void *)obj_ptr->inbuf_ptr,
                obj_ptr->ctx_ptr->width, obj_ptr->ctx_ptr->height, VIDEO_FORMAT_YCbCr_420_P,
                (void *)pic_ptr->base_ptr,
                pic_ptr->width, pic_ptr->height, pic_ptr->format);
    }
    else {
        memset((uint8 *)obj_ptr->inbuf_ptr, 16, sizeY);
        memset((uint8 *)obj_ptr->inbuf_ptr + sizeY, 128, sizeUV * 2);
    }


    obj_ptr->pic_ptr->data[0]     = (void *)obj_ptr->inbuf_ptr;
    obj_ptr->pic_ptr->data[1]     = (void *)obj_ptr->inbuf_ptr + sizeY;
    obj_ptr->pic_ptr->data[2]     = (void *)obj_ptr->inbuf_ptr + sizeY + sizeUV;
    obj_ptr->pic_ptr->data[3]     = NULL;
    obj_ptr->pic_ptr->linesize[0] = obj_ptr->ctx_ptr->width;
    obj_ptr->pic_ptr->linesize[1] = obj_ptr->ctx_ptr->width / 2;
    obj_ptr->pic_ptr->linesize[2] = obj_ptr->ctx_ptr->width / 2;
    obj_ptr->pic_ptr->linesize[3] = 0;

    obj_ptr->pic_ptr->pts = pic_ptr->ts;

    /*
     * encode and return size of encoded data buffer.
     */

    ret = avcodec_encode_video(obj_ptr->ctx_ptr,
            (void *)obj_ptr->outbuf,
            sizeof(obj_ptr->outbuf),
            obj_ptr->pic_ptr);
    obj_ptr->len = ret;

    obj_ptr->pts = pic_ptr->ts;
    if (ret <= 0) {
        obj_ptr->len = 0;
        return (0);
    }
#endif
    return(1);
    
}

/*
 * ======== H263_encGetData() ========
 * This function gets encoded data from the encoder.
 * In pkt_ptr:
 * buf_ptr will be written to with packet data (buf_ptr must point to memory of
 *  minimum storage H263_MAX_PKT_SIZE.
 * sz will be written with packet size.
 * ts will be written with packet time stamp.
 * mark will be written with mark bit for the packet.
 * Returns:
 *  1: Success.
 *  0: Failed (no buffer ready.).
 */
int H263_encGetData(
    H263_EncObj  *obj_ptr,
    Video_Packet *pkt_ptr)
{
    int f;
    int p;
    int sbit;
    int ebit;
    int src;
    int i;
    int u;
    int s;
    int a;
    int dbq;
    int trb;
    int tr;
    int index;
    int sz;
    uint8 *start1_ptr;
    uint8 *start2_ptr;
    uint8 *buf_ptr;

    if ((NULL == obj_ptr) || (NULL == pkt_ptr)) {
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
        return (0);
    }

    buf_ptr = (uint8*)obj_ptr->outbuf;

    if (obj_ptr->len <= 0) {
        return (0);
    }

    if (obj_ptr->pkt.num < 0) {
        /*
         * Got a new buffer.
         */

        /*
         * Make header
         */
        if ((0 != buf_ptr[0]) && 
                (0 != buf_ptr[1]) && 
                (0x20 != (buf_ptr[2] >> 2))) {
            DBG("Invalid H263 packet\n");
            return (0);
        }

        tr = ((buf_ptr[2] & 3) << 6) + (buf_ptr[3] >> 2);

        if (0x2 != (buf_ptr[3] & 3)) {
            DBG("Invalid H263 packet\n");
            return (0);
        }

        f = 0;
        ebit = 0;
        sbit = 0;
        src = (buf_ptr[4] >> 2) & 7;
        i = (buf_ptr[4] >> 1) & 1;
        u = (buf_ptr[4]) & 1;
        s = (buf_ptr[5] >> 7) & 1;
        a = (buf_ptr[5] >> 6) & 1;
        p = (buf_ptr[5] >> 5) & 1;

        if (0 != p) {
            DBG("P/B not supported\n");
            return (0);
        }

        memset(&obj_ptr->pkt, 0, sizeof(obj_ptr->pkt));
        obj_ptr->pkt.buf_ptr = (uint8 *)obj_ptr->outbuf;
        obj_ptr->pkt.end_ptr = (uint8 *)obj_ptr->outbuf + obj_ptr->len;
        obj_ptr->pkt.ts = obj_ptr->pts / 11; /* 90000 */
        obj_ptr->pkt.hdr[0] = (f << 7) + (p << 6) + (sbit << 3) + ebit;
        obj_ptr->pkt.hdr[1] = 
                (src << 5) + (i << 4) + (u << 3) + (s << 2) + (a << 1);
        /* TRB 0 for mode A, DBQ 0 cause PB not used */
        trb = 0;
        dbq = 0;
        obj_ptr->pkt.hdr[2] = (dbq << 3) + trb; 
        tr = 0; /* TR Must be 0 for mode A */
        obj_ptr->pkt.hdr[3] = tr; 
        /*
         * Run a loop to find GOB packets in a stream.
         */
        i = 0;
        _H263_encFindGobStart(
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
            sz = obj_ptr->pkt.start2_ptr[i] - obj_ptr->pkt.start1_ptr[i];
            if ((sz <= 0) || (sz > (H263_ENC_MAX_RTP_SIZE - sizeof(obj_ptr->pkt.hdr)))) {
                DBG("Warning! packet not acceptable, its too large."
                        " Limit encoder output size\n");
                obj_ptr->pkt.num = -1;
                obj_ptr->len = 0;
                return (0);
            }
            i++;
            if (i >= H263_ENC_MAX_PKTS) {
                DBG("Losing data, increase packet buffer\n");
                break;
            }
            _H263_encFindGobStart(
                obj_ptr->pkt.start2_ptr[i - 1],
                obj_ptr->pkt.end_ptr,
                &obj_ptr->pkt.start1_ptr[i],
                &obj_ptr->pkt.start2_ptr[i]);
        }

        /*
         * Now we have a list of packets with start sequence removed.
         */
        obj_ptr->pkt.index = 0;
    }

    /*
     * Put some logic here to get H.263 RTP packets from H.263 packets.
     */
    index = obj_ptr->pkt.index;
    start1_ptr = obj_ptr->pkt.start1_ptr[index];
    start2_ptr = obj_ptr->pkt.start2_ptr[index];
    if ((NULL == start1_ptr) || (NULL == start2_ptr)) {
        /*
         * If no more data in buffer, mark it free, return.
         */
        obj_ptr->pkt.num = -1;
        obj_ptr->len = 0;
        return(0);
    }
   
    while (1) {
        if ((NULL == start2_ptr) ||
                ((start2_ptr - start1_ptr) >  
                 (H263_ENC_MAX_RTP_SIZE - sizeof(obj_ptr->pkt.hdr)))) {
            if (index > 0) {
                start2_ptr = obj_ptr->pkt.start2_ptr[index - 1];
            }
            break;
        }
        index++;
        start2_ptr = obj_ptr->pkt.start2_ptr[index];
    }
   
    sz = start2_ptr - start1_ptr;
    if ((sz <= 0) || (sz > (H263_ENC_MAX_RTP_SIZE - sizeof(obj_ptr->pkt.hdr)))) {
        DBG("This is a bug!!\n");
        obj_ptr->pkt.num = -1;
        obj_ptr->len = 0;
        return (0);
    }
    else {
        /*
         * Will fit entirely in one RTP packet.
         */
        memcpy(pkt_ptr->buf_ptr, obj_ptr->pkt.hdr, sizeof(obj_ptr->pkt.hdr));
        memcpy(pkt_ptr->buf_ptr + sizeof(obj_ptr->pkt.hdr), start1_ptr, sz);
        pkt_ptr->sz = sz + sizeof(obj_ptr->pkt.hdr);
        pkt_ptr->ts = obj_ptr->pkt.ts;
        pkt_ptr->mark = 0;
        if ((NULL == obj_ptr->pkt.start1_ptr[index])) {
            pkt_ptr->mark = 1;
        }
        obj_ptr->pkt.index = index;
        return (1);
    }

    return (0);
}

/*
 * ======== H263_encInit() ========
 * This function inits the encoder and starts it.
 * Returns:
 * 0 : Success
 * -1 : Failure
 */
int H263_encInit(
    H263_EncObj    *obj_ptr,
    H263_EncParams *params_ptr)
{
    if ((NULL == obj_ptr) || (NULL == params_ptr)) {
        DBG("FAILURE\n");
        return (-1);
    }

    OSAL_memSet(obj_ptr, 0, sizeof(H263_EncObj));

    obj_ptr->inbuf_ptr = OSAL_memAlloc(
            params_ptr->width * params_ptr->height * 4, 0);
    if (NULL == obj_ptr->inbuf_ptr) {
        DBG("FAILURE\n");
        return (-1);
    }

#ifndef EXCLUDE_FFMPEG_LIBRARY
    obj_ptr->codec_ptr = avcodec_find_encoder(CODEC_ID_H263);
    if (NULL == obj_ptr->codec_ptr) {
        DBG("FAILURE\n");
        return (-1);
    }

    obj_ptr->ctx_ptr = avcodec_alloc_context();
    if (NULL == obj_ptr->ctx_ptr) {
        DBG("FAILURE\n");
        return (-1);
    }

    /*
     * resolution must be a multiple of two
     */
    obj_ptr->ctx_ptr->width = params_ptr->width;
    obj_ptr->ctx_ptr->height = params_ptr->height;
    obj_ptr->ctx_ptr->rtp_payload_size = 512;

    //obj_ptr->ctx_ptr->rc_max_rate = params_ptr->maxBitrate;
    //obj_ptr->ctx_ptr->rc_buffer_size = params_ptr->maxBitrate;

    /*
     * TS ticks per second, VCD provides in usec
     */
    obj_ptr->ctx_ptr->time_base = (AVRational){1, 1000000};

    /*
     * H.263 supports only YUV420 planar.
     * Since planar, stored in YYYYYYYY..., VV..., UU...
     * 4:2:0 is downsample of both U and V by 2 in horizontal and by 2 in
     * vertical direction
     */
    obj_ptr->ctx_ptr->pix_fmt = PIX_FMT_YUV420P;
#endif

    if (OSAL_FAIL == OSAL_semAcquire(VID_dspLock, OSAL_WAIT_FOREVER)) {
        return (-1);
    }

#ifndef EXCLUDE_FFMPEG_LIBRARY
    if(avcodec_open(obj_ptr->ctx_ptr, obj_ptr->codec_ptr) < 0) {
        DBG("FAILURE\n");
        OSAL_semGive(VID_dspLock);
        return (-1);
    }
#endif

    if (OSAL_FAIL == OSAL_semGive(VID_dspLock)) {
        return (-1);
    }

    /*
     * New frame.
     */
#ifndef EXCLUDE_FFMPEG_LIBRARY
    obj_ptr->pic_ptr = avcodec_alloc_frame();
    if (NULL == obj_ptr->pic_ptr) {
        DBG("FAILURE\n");
        return (-1);
    }
#endif
    obj_ptr->pkt.num = -2;
    DBG("FFMPEG H.263 encode init complete\n");

    return (0);
}

/*
 * ======== H263_encShut() ========
 * This function shuts the encoder.
 * Returns:
 */
void H263_encShut(
    H263_EncObj *obj_ptr)
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
     * Stop all encoding activity.
     */
    obj_ptr->sleepm = 1;

    OSAL_memFree(obj_ptr->inbuf_ptr, 0);

    if (OSAL_FAIL == OSAL_semAcquire(VID_dspLock, OSAL_WAIT_FOREVER)) {
        return;
    }
#ifndef EXCLUDE_FFMPEG_LIBRARY
    if(avcodec_close(obj_ptr->ctx_ptr) < 0) {
        DBG("FAILURE\n");
        return;
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
    DBG("FFMPEG H.263 encode shutdown complete\n");

}

/*
 * ======== H263_encRequestResolution() ========
 * Res change request. Scaling is available.
 * Returns:
 * 0 : Success
 * -1 : Failure
 */
int H263_encRequestResolution(
    H263_EncObj *obj_ptr,
    int          width,
    int          height)
{
    H263_EncParams params;

    if (NULL == obj_ptr) {
        return (-1);
    }
#ifndef EXCLUDE_FFMPEG_LIBRARY
    if (NULL == obj_ptr->ctx_ptr) {
        return (-1);
    }
#endif

    if ((width <= 0) || (height < 0)) {
        return (-1);
    }

    memcpy(&params, &obj_ptr->paramCache, sizeof(params));
    params.width = width;
    params.height = height;
    H263_encShut(obj_ptr);
    
    return (H263_encInit(obj_ptr, &params));
}
