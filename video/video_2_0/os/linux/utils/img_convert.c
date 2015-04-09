/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 12262 $ $Date: 2010-06-10 18:55:56 -0400 (Thu, 10 Jun 2010) $
 */

#include <sys/types.h>
#include <stdint.h>
#ifndef EXCLUDE_FFMPEG_LIBRARY
#include <libavutil/avutil.h>
#include <libswscale/swscale.h>
#endif
#include <video.h>

/*
 * ======== IMG_formatConvert() ========
 * Scales/color converts
 * !!! Must provide big enough buffers or crash!
 * Returns:
 */
void IMG_formatConvert(
    void         *dstData_ptr,
    int           dstW,
    int           dstH,
    VideoFormat   dstFormat,
    void         *srcData_ptr,
    int           srcW,
    int           srcH,
    VideoFormat   srcFormat)
{
#ifndef EXCLUDE_FFMPEG_LIBRARY
    struct SwsContext *sws_ptr;
    int sformat;
    int dformat;
    uint8_t *src_ptr[4];
    uint8_t *dst_ptr[4];
    int srcStride[4];
    int dstStride[4];
#endif

    /*
     * SRC
     */
    switch (srcFormat) {
        case VIDEO_FORMAT_YCbCr_420_P:
#ifndef EXCLUDE_FFMPEG_LIBRARY
            src_ptr[0] = srcData_ptr;
            src_ptr[1] = srcData_ptr + srcW * srcH;
            src_ptr[2] = srcData_ptr + srcW * srcH + ((srcW * srcH) / 4);
            src_ptr[3] =  NULL;

            srcStride[0] = srcW;
            srcStride[1] = srcW / 2;
            srcStride[2] = srcW / 2;
            srcStride[3] = 0;

            sformat = PIX_FMT_YUV420P;
#endif
            break;

        case VIDEO_FORMAT_YCbCr_420_SP:
#ifndef EXCLUDE_FFMPEG_LIBRARY
            src_ptr[0] = srcData_ptr;
            src_ptr[1] = srcData_ptr + srcW * srcH;
            src_ptr[2] = srcData_ptr + srcW * srcH + ((srcW * srcH) / 4);
            src_ptr[3] =  NULL;

            srcStride[0] = srcW;
            srcStride[1] = srcW;
            srcStride[2] = srcW;
            srcStride[3] = 0;

            sformat = PIX_FMT_NV21;
#endif
            break;

        case VIDEO_FORMAT_YUYV:
#ifndef EXCLUDE_FFMPEG_LIBRARY
            src_ptr[0] = srcData_ptr;
            src_ptr[1] = NULL;
            src_ptr[2] = NULL;
            src_ptr[3] = NULL;

            srcStride[0] = srcW * 2;
            srcStride[1] = 0;
            srcStride[2] = 0;
            srcStride[3] = 0;

            sformat = PIX_FMT_YUYV422;
#endif
            break;

        case VIDEO_FORMAT_RGB_565:
#ifndef EXCLUDE_FFMPEG_LIBRARY
            src_ptr[0] = srcData_ptr;
            src_ptr[1] = NULL;
            src_ptr[2] = NULL;
            src_ptr[3] = NULL;

            srcStride[0] = srcW * 2;
            srcStride[1] = 0;
            srcStride[2] = 0;
            srcStride[3] = 0;

            sformat = PIX_FMT_RGB565;
#endif
            break;

        case VIDEO_FORMAT_RGB_32:
#ifndef EXCLUDE_FFMPEG_LIBRARY
            src_ptr[0] = srcData_ptr;
            src_ptr[1] = NULL;
            src_ptr[2] = NULL;
            src_ptr[3] = NULL;

            srcStride[0] = srcW * 4;
            srcStride[1] = 0;
            srcStride[2] = 0;
            srcStride[3] = 0;

            sformat = PIX_FMT_RGB32;
#endif
            break;

        case VIDEO_FORMAT_RGB_24:
#ifndef EXCLUDE_FFMPEG_LIBRARY
            src_ptr[0] = srcData_ptr;
            src_ptr[1] = NULL;
            src_ptr[2] = NULL;
            src_ptr[3] = NULL;

            srcStride[0] = srcW * 3;
            srcStride[1] = 0;
            srcStride[2] = 0;
            srcStride[3] = 0;

            sformat = PIX_FMT_RGB24;
#endif
            break;
        default:
            return;
    }

    /*
     * DST
     */
    switch (dstFormat) {
        case VIDEO_FORMAT_YCbCr_420_P:
#ifndef EXCLUDE_FFMPEG_LIBRARY
            dst_ptr[0] = dstData_ptr;
            dst_ptr[1] = dstData_ptr + dstW * dstH;
            dst_ptr[2] = dstData_ptr + dstW * dstH + ((dstW * dstH) / 4);
            dst_ptr[3] =  NULL;

            dstStride[0] = dstW;
            dstStride[1] = dstW / 2;
            dstStride[2] = dstW / 2;
            dstStride[3] = 0;

            dformat = PIX_FMT_YUV420P;
#endif
            break;

        case VIDEO_FORMAT_YCbCr_420_SP:
#ifndef EXCLUDE_FFMPEG_LIBRARY
            dst_ptr[0] = dstData_ptr;
            dst_ptr[1] = dstData_ptr + dstW * dstH;
            dst_ptr[2] = dstData_ptr + dstW * dstH + ((dstW * dstH) / 4);
            dst_ptr[3] =  NULL;

            dstStride[0] = dstW;
            dstStride[1] = dstW;
            dstStride[2] = dstW;
            dstStride[3] = 0;

            dformat = PIX_FMT_NV21;
#endif
            break;

        case VIDEO_FORMAT_YUYV:
#ifndef EXCLUDE_FFMPEG_LIBRARY
            dst_ptr[0] = dstData_ptr;
            dst_ptr[1] = NULL;
            dst_ptr[2] = NULL;
            dst_ptr[3] = NULL;

            dstStride[0] = dstW * 2;
            dstStride[1] = 0;
            dstStride[2] = 0;
            dstStride[3] = 0;

            dformat = PIX_FMT_YUYV422;
#endif
            break;

        case VIDEO_FORMAT_RGB_565:
#ifndef EXCLUDE_FFMPEG_LIBRARY
            dst_ptr[0] = dstData_ptr;
            dst_ptr[1] = NULL;
            dst_ptr[2] = NULL;
            dst_ptr[3] = NULL;

            dstStride[0] = dstW * 2;
            dstStride[1] = 0;
            dstStride[2] = 0;
            dstStride[3] = 0;

            dformat = PIX_FMT_RGB565;
#endif
            break;

        case VIDEO_FORMAT_RGB_32:
#ifndef EXCLUDE_FFMPEG_LIBRARY
            dst_ptr[0] = dstData_ptr;
            dst_ptr[1] = NULL;
            dst_ptr[2] = NULL;
            dst_ptr[3] = NULL;

            dstStride[0] = dstW * 4;
            dstStride[1] = 0;
            dstStride[2] = 0;
            dstStride[3] = 0;

            dformat = PIX_FMT_RGB32;
#endif
            break;

        case VIDEO_FORMAT_RGB_24:
#ifndef EXCLUDE_FFMPEG_LIBRARY
            dst_ptr[0] = dstData_ptr;
            dst_ptr[1] = NULL;
            dst_ptr[2] = NULL;
            dst_ptr[3] = NULL;

            dstStride[0] = dstW * 3;
            dstStride[1] = 0;
            dstStride[2] = 0;
            dstStride[3] = 0;

            dformat = PIX_FMT_RGB24;
#endif
            break;

        default:
            return;
    }


    /*
     * Not handling small images.
     */
    if ((srcW < 32) || (srcH < 32) || (dstW < 32) || (dstH < 32)) {
        return;
    }

    /*
     * Scale!
     */
#ifndef EXCLUDE_FFMPEG_LIBRARY
    sws_ptr = sws_getContext(
            srcW, srcH, sformat,
            dstW, dstH, dformat,
            SWS_BILINEAR, NULL, NULL, NULL);

    sws_scale(sws_ptr, (const uint8_t * const*)src_ptr, srcStride, 0, srcH, dst_ptr, dstStride);
    sws_freeContext(sws_ptr);
#endif
}
