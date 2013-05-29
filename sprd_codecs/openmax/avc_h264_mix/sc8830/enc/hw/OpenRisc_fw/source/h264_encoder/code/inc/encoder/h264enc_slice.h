
#ifndef _H264ENC_SLICE_H_
#define _H264ENC_SLICE_H_

PUBLIC int32 h264enc_slicetype_decide(ENC_IMAGE_PARAMS_T *img_ptr);
PUBLIC void h264enc_slice_init (ENC_IMAGE_PARAMS_T *img_ptr, int32 nal_type, int32 slice_type, int32 global_qp);
PUBLIC int32 h264enc_slice_write (ENC_IMAGE_PARAMS_T *img_ptr);

#endif