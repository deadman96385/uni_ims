#ifndef _MP4ENC_TRACE_H_
#define _MP4ENC_TRACE_H_

#include "video_common.h"
#include "mp4enc_mode.h"
#include "mp4enc_global.h"

void Mp4Enc_Trace_MBC_result_one_macroblock(ENC_VOP_MODE_T *vop_mode_ptr, ENC_MB_MODE_T *mb_mode_ptr);
void Mp4Enc_Trace_MEA_result_one_macroblock(ENC_VOP_MODE_T *vop_mode_ptr);
void Mp4Enc_Trace_OneBlkIDctCoef_forASICCompare(int16 *pBlk);

#endif //_MP4ENC_TRACE_H_
