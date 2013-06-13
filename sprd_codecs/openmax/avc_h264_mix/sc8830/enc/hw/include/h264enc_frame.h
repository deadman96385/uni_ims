#ifndef _H264ENC_FRAME_H_
#define _H264ENC_FRAME_H_

#include "h264enc_mode.h"

void h264enc_reference_update( ENC_IMAGE_PARAMS_T *img_ptr );
void h264enc_reference_build_list( ENC_IMAGE_PARAMS_T *img_ptr, int i_poc, int i_slice_type );

#endif //_H264ENC_FRAME_H_