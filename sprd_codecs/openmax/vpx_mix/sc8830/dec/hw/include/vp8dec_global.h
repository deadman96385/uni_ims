

#ifndef VP8DEC_GLOBAL_H
#define VP8DEC_GLOBAL_H

#include "vp8dec_reg.h" //derek

extern unsigned int g_nFrame_dec;
extern unsigned int g_stream_offset;
extern unsigned char* y_buffer;
extern unsigned char* u_buffer;
extern unsigned char* v_buffer;

extern uint32 g_readFileSize;

extern char weightscale4x4[6][4][4];//6 6*2+2=14*16=224B
extern char weightscale8x8[2][8][8];//2 2*2+2=6*64=384B
extern char g_list0_map_addr[16];//weihu

extern unsigned char or1200_print;


PUBLIC extern INPUT_PARA_T *g_input;
PUBLIC extern int	g_stream_type;

extern uint32 inter_malloc_mem_start_addr;
extern uint32 total_inter_malloc_size;
extern uint32 extra_malloc_mem_start_addr;
extern uint32 total_extra_malloc_size;
extern uint32 frame_buf_size;
extern uint32 bs_start_addr;
extern uint32 vld_table_addr;
extern uint32 input_buffer_update;
extern uint32 cpu_will_be_reset;
extern uint32 g_not_first_reset;
extern uint32 video_size_get;
extern uint32 video_buffer_malloced;
//extern uint32 rate_control_en;
//extern uint32 target_rate;
extern uint32 file_end;
extern uint32 s_bFisrtUnit;
extern uint32 or_addr_offset;

#if defined(SIM_IN_WIN)
	#if defined(MPEG4_ENC)
		extern FILE *s_pEnc_input_src_file;
		extern FILE *s_pEnc_output_bs_file;
		extern FILE *s_pEnc_output_recon_file;
	#else
		extern FILE *s_pDec_input_bs_file; //file pointer of H.264 bit stream
		extern FILE *s_pDec_recon_yuv_file; //file pointer of decoded and filtered YUV sequence
	#endif //#if defined(MPEG4_ENC)
#else 
	extern uint8 *ptrInbfr;
	extern uint8 videoStream[];
	extern int g_streamLen;
	extern uint8 *ptrOutBfr;
	extern uint8 *g_pOutBfr;
#endif 	//#if defined(SIM_IN_WIN)

#endif //VP8DEC_GLOBAL_H

