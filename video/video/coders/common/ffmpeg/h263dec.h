/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 5312 $ $Date: 2008-02-18 19:05:41 -0600 (Mon, 18 Feb 2008) $
 */


#ifndef _H263_DEC_H_
#define _H263_DEC_H_

#ifndef EXCLUDE_FFMPEG_LIBRARY
#include <libavcodec/avcodec.h>
#endif
#include <osal_types.h>
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
} H263_DecObj;

typedef struct {
    int width;
    int height;
    int profile;
    int level;
} H263_DecParams;

/*
 * Decode
 */
int H263_decInit(
    H263_DecObj    *obj_ptr,
    H263_DecParams *params_ptr);

int H263_decGetData(
    H263_DecObj    *obj_ptr,
    Video_Picture  *pic_ptr);

void H263_decShut(
    H263_DecObj *obj_ptr);

int H263_dec(
    H263_DecObj  *obj_ptr,
    Video_Packet *pkt_ptr);

#endif
