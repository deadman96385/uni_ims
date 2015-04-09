/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 5312 $ $Date: 2008-02-18 19:05:41 -0600 (Mon, 18 Feb 2008) $
 */

#ifndef _H263_ENC_H_
#define _H263_ENC_H_

#ifndef EXCLUDE_FFMPEG_LIBRARY
#include <libavcodec/avcodec.h>
#endif
#include <osal_types.h>
#include <video.h>

/*
 * Max RTP size supported.
 */
#define H263_ENC_MAX_RTP_SIZE (VIDEO_MAX_RTP_SZ)

/*
 * Max packet fragments in a stream.
 */
#define H263_ENC_MAX_PKTS     (64)

typedef struct {
    int width;
    int height;
    int stride;
    int rotation;
    int fps;
    int profile;
    int level;
    int maxBitrate;
} H263_EncParams;

/*
 * Encoder/Decoder objects.
 * Note: These objects are all provate and application has no data in them.
 * Keep this private feature for swapable coders.
 */
typedef struct {
#ifndef EXCLUDE_FFMPEG_LIBRARY
    AVCodec        *codec_ptr;
    AVCodecContext *ctx_ptr;
    AVFrame        *pic_ptr;
#endif
    char            outbuf[VIDEO_NET_BUFFER_SZ];
    char           *inbuf_ptr;
    uint64          pts;
    int             len;
    int             sleepm;
    struct {
        uint8      *buf_ptr;
        uint8      *end_ptr;
        uint64      ts;
        int         num;
        uint8      *start1_ptr[H263_ENC_MAX_PKTS];
        uint8      *start2_ptr[H263_ENC_MAX_PKTS];
        int         index;
        uint8       hdr[4];
    } pkt;
    H263_EncParams  paramCache;
} H263_EncObj;

/*
 * Encode
 */

int H263_encInit(
    H263_EncObj    *obj_ptr,
    H263_EncParams *params_ptr);

int H263_encGetData(
    H263_EncObj  *obj_ptr,
    Video_Packet *pkt_ptr);

void H263_encShut(
    H263_EncObj *obj_ptr);

int H263_enc(
    H263_EncObj *obj_ptr,
    Video_Picture  *pic_ptr);

int H263_encRequestResolution(
    H263_EncObj *obj_ptr,
    int          width,
    int          height);

#endif
