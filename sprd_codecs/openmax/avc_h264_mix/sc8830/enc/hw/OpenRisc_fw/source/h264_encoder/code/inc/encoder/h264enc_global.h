#ifndef _H264ENC_GLOBAL_H_
#define _H264ENC_GLOBAL_H_

#include "sci_types.h"


extern uint32 g_nFrame_enc;
extern ENC_IMAGE_PARAMS_T *g_enc_image_ptr;
// extern ENC_SLICE_HEADER_T *g_slice_header_ptr;
extern MMEncConfig * g_h264_enc_config;
extern ENC_MB_CACHE_T *g_mb_cache_ptr;
extern int32 g_is_yuv_frm_malloced;

extern int	g_decimate_en;

extern double g_PSNR [3];

#ifdef SIM_IN_WIN
//extern FILE * g_bit_stat_fp;
//extern FILE *  g_src_out_fp;		//for debugging fetch and pre-filter
#endif

//extern uint32 * g_src_out_frm[3];

//extern PIX_SIZE_T x264_pixel_size[7];
//const int32 x264_size2pixel[5][5];
extern int32 x264_scan8[16+2*4];
extern int32 i_qp0_cost_table[52];
extern int32 i_qp0_cost2_table[52];

extern int32 g_mb_pred_mode4x4_fix[13];
extern int32 g_mb_pred_mode8x8c_fix[7];
extern int32 g_mb_pred_mode16x16_fix[7];

extern uint8 g_qpPerRem_tbl [52][2];
extern uint32 g_qpPerF_tbl [9];
extern uint8 g_vlc_hw_tbl [406*8];

extern uint8 or1200_print;

LOCAL extern uint8 *s_inter_mem_bfr_ptr;		
LOCAL extern uint8 *s_extra_mem_bfr_ptr;
PUBLIC extern uint32 g_inter_malloced_size;
PUBLIC extern uint32 g_extra_malloced_size;
PUBLIC extern int	g_stream_type;

extern uint32 inter_malloc_mem_start_addr;
extern uint32 total_inter_malloc_size;
extern uint32 extra_malloc_mem_start_addr;
extern uint32 total_extra_malloc_size;
extern uint32 bs_start_addr;
extern uint32 bs_buffer_length;
//extern uint32 vld_table_addr;
extern uint32 input_buffer_update;
extern uint32 cpu_will_be_reset;
extern uint32 g_not_first_reset;
//extern uint32 video_size_get;
extern uint32 video_buffer_malloced;
extern uint32 or_addr_offset;

#ifdef _LIB
	extern uint8 one_frame_stream[ONEFRAME_BITSTREAM_BFR_SIZE];
	extern uint32 off_frame_strm;
#endif

#if defined(SIM_IN_WIN)
	#if defined(JPEG_ENC)||defined(MPEG4_ENC)||defined(H264_ENC)
		extern FILE *s_pEnc_input_src_file;
		extern FILE *s_pEnc_output_bs_file;
		extern FILE *s_pEnc_output_recon_file;
	#else
		extern FILE *s_pDec_input_bs_file; //file pointer of H.264 bit stream
		extern FILE *s_pDec_recon_yuv_file; //file pointer of reconstructed YUV sequence
		extern FILE *s_pDec_disp_yuv_file; //file pointer of display YUV sequence
	#endif //#if defined(JPEG_ENC)
#else 
	extern uint8 *ptrInbfr;
	extern int g_streamLen;
	extern uint8 *ptrOutBfr;
	extern uint8 *g_pOutBfr;
#endif 	//#if defined(SIM_IN_WIN)

#endif //_H264ENC_GLOBAL_H_