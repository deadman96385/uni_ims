

#ifndef VP8_SWAP_YV12BUFFER_H
#define VP8_SWAP_YV12BUFFER_H

#include "vp8_yv12config.h"
#include "vp8dec.h"

void vp8_swap_yv12_buffer(YV12_BUFFER_CONFIG *new_frame, YV12_BUFFER_CONFIG *last_frame);
void vp8_copy_yv12_buffer(VPXHandle *vpxHandle,VP8_COMMON *cm, YV12_BUFFER_CONFIG *src_frame, YV12_BUFFER_CONFIG *dst_frame);
void vp8_check_yv12_buffer(VP8_COMMON *cm, YV12_BUFFER_CONFIG *src_frame);

#endif //VP8_SWAP_YV12BUFFER_H