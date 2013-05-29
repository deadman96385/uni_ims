

#ifndef VP8_YV12EXTEND_H
#define VP8_YV12EXTEND_H

#include "vp8_yv12config.h"

    void vp8_yv12_extend_frame_borders(YV12_BUFFER_CONFIG *ybf);

    /* Copy Y,U,V buffer data from src to dst, filling border of dst as well. */

    void vp8_yv12_copy_frame(YV12_BUFFER_CONFIG *src_ybc, YV12_BUFFER_CONFIG *dst_ybc);
    void vp8_yv12_copy_frame_yonly(YV12_BUFFER_CONFIG *src_ybc, YV12_BUFFER_CONFIG *dst_ybc);


#endif //VP8_YV12EXTEND_H