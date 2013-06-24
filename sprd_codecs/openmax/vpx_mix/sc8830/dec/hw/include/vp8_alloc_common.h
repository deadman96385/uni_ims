
#ifndef VP8_ALLOC_COMMON_H
#define VP8_ALLOC_COMMON_H

void vp8_create_common(VP8_COMMON *oci);
void vp8_remove_common(VP8_COMMON *oci);
void vp8_de_alloc_frame_buffers(VP8_COMMON *oci);
int vp8_alloc_frame_buffers(VP8_COMMON *oci, int width, int height);
void vp8_setup_version(VP8_COMMON *oci);
int vp8_init_frame_buffers(VP8_COMMON *oci);

#endif //VP8_ALLOC_COMMON_H
