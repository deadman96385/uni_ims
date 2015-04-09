/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 5312 $ $Date: 2008-02-18 19:05:41 -0600 (Mon, 18 Feb 2008) $
 */


#ifndef _H264_ENC_H_
#define _H264_ENC_H_

#ifndef EXCLUDE_FFMPEG_LIBRARY
#include <x264.h>
#endif
#include <video.h>

/*
 * Max RTP size supported.
 */
#define H264_ENC_MAX_RTP_SIZE (VIDEO_MAX_RTP_SZ)

/*
 * Max packet fragments in a stream.
 */
#define H264_ENC_MAX_PKTS     (64)

typedef struct {
    int width;
    int height;
    int stride;
    int rotation;
    int fps;
    int profile;
    int level;
    int maxBitrate;
} H264_EncParams;

/*
 * Encoder/Decoder objects.
 * Note: These objects are all provate and application has no data in them.
 * Keep this private feature for swapable coders.
 */
typedef struct {
#ifndef EXCLUDE_FFMPEG_LIBRARY
    x264_t          *enc_ptr;
    x264_param_t    params;
    x264_picture_t  pic;
    x264_picture_t  picOut;
#endif
    char            outbuf[VIDEO_NET_BUFFER_SZ];
    char           *inbuf_ptr;
    uint64          pts;
    int             len;
    int             width;
    int             height;
    int             lastType;
    int             sleepm;
    struct {
        uint8      *buf_ptr;
        uint8      *end_ptr;
        uint64      ts;
        int         num;
        int         consumed;
        uint8      *start1_ptr[H264_ENC_MAX_PKTS];
        uint8      *start2_ptr[H264_ENC_MAX_PKTS];
        int         index;
    } pkt;
    H264_EncParams  paramCache;
} H264_EncObj;


/*
 * Encode
 */

int H264_encInit(
    H264_EncObj    *obj_ptr,
    H264_EncParams *params_ptr);

int H264_encGetData(
    H264_EncObj  *obj_ptr,
    Video_Packet *pkt_ptr);

void H264_encShut(
    H264_EncObj *obj_ptr);

int H264_enc(
    H264_EncObj *obj_ptr,
    Video_Picture  *pic_ptr);

int H264_encRequestKeyFrame(
    H264_EncObj *obj_ptr);

int H264_encRequestResolution(
    H264_EncObj *obj_ptr,
    int          width,
    int          height);

#endif
