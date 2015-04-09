/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 5312 $ $Date: 2008-02-18 19:05:41 -0600 (Mon, 18 Feb 2008) $
 */


#ifndef _H264_DEC_H_
#define _H264_DEC_H_

#ifndef EXCLUDE_FFMPEG_LIBRARY
#include <libavcodec/avcodec.h>
#endif
#include <video.h>

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
    char           *outbuf_ptr;
    int             picReady;
    int             sleepm;
} H264_DecObj;

typedef struct {
    int width;
    int height;
    int profile;
    int level;
} H264_DecParams;

/*
 * Decode
 */
int H264_decInit(
    H264_DecObj    *obj_ptr,
    H264_DecParams *params_ptr);

int H264_decGetData(
    H264_DecObj    *obj_ptr,
    Video_Picture  *pic_ptr);

void H264_decShut(
    H264_DecObj *obj_ptr);

int H264_dec(
    H264_DecObj  *obj_ptr,
    Video_Packet *pkt_ptr);

#endif
