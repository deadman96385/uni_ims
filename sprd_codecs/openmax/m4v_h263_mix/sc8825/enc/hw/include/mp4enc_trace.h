#ifndef _MP4ENC_TRACE_H_
#define _MP4ENC_TRACE_H_

#include "video_common.h"
#include "mp4enc_mode.h"
#include "mp4enc_global.h"

void Mp4Enc_Trace_MBC_result_one_macroblock(ENC_VOP_MODE_T *pVop_mode, ENC_MB_MODE_T *pMb_mode);
void Mp4Enc_Trace_MEA_result_one_macroblock(ENC_VOP_MODE_T *pVop_mode);
// void printf_src_blkInt16MB (DCT_IO_BUFFER_T * pDct_io_bfr);
void Mp4Enc_Trace_OneBlkIDctCoef_forASICCompare(int16 *pBlk);

#endif //_MP4ENC_TRACE_H_
