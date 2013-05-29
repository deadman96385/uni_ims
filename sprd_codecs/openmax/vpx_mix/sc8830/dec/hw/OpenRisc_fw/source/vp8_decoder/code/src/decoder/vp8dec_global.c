#include "sci_types.h"
#include "video_common.h"


uint32 g_nFrame_dec;
uint32 g_stream_offset;

uint32 g_readFileSize = 20*1024*1024;

char weightscale4x4[6][4][4];//6 6*2+2=14*16=224B
char weightscale8x8[2][8][8];//2 2*2+2=6*64=384B
char g_list0_map_addr[16];//weihu

uint8 or1200_print;		// derek-2012-09-26

//used for malloc		
PUBLIC uint32 g_inter_malloced_size;
PUBLIC uint32 g_extra_malloced_size;

PUBLIC INPUT_PARA_T *g_input;
PUBLIC int	g_stream_type;

PUBLIC uint32 inter_malloc_mem_start_addr;
PUBLIC uint32 total_inter_malloc_size;
PUBLIC uint32 extra_malloc_mem_start_addr;
PUBLIC uint32 total_extra_malloc_size;
PUBLIC uint32 frame_buf_size;
PUBLIC uint32 bs_start_addr;
PUBLIC uint32 bs_buffer_length;
PUBLIC uint32 vld_table_addr;

PUBLIC uint32 input_buffer_update;	//input buffer���ݸ����ˣ���ҪBSM����load����
PUBLIC uint32 cpu_will_be_reset;	//arm����reset openrisc cpu��֪ͨ�˳�ǰ�����ֳ�
PUBLIC uint32 g_not_first_reset;	//���ǵ�һ��reset����Ҫ�ָ��ֳ�//weihu
PUBLIC uint32 video_size_get;		//���image size��
PUBLIC uint32 video_buffer_malloced;//�Ѿ���arm��������D
//PUBLIC uint32 rate_control_en;
//PUBLIC uint32 target_rate;
PUBLIC uint32 file_end;
PUBLIC uint32 s_bFisrtUnit;
PUBLIC uint32 or_addr_offset;

#if defined(SIM_IN_WIN) 
	#if defined(MPEG4_ENC)
		FILE *s_pEnc_input_src_file;
		FILE *s_pEnc_output_bs_file;
		FILE *s_pEnc_output_recon_file;
	#else
		FILE *s_pDec_input_bs_file; //file pointer of H.264 bit stream
		FILE *s_pDec_recon_yuv_file; //file pointer of decoded and filtered YUV sequence
		FILE *s_pDec_debug_file;
	#endif //#if defined(MPEG4_ENC)
#else 
	uint8 *ptrInbfr;
	int g_streamLen;

	uint8 *ptrOutBfr; // to store the decoded frame
	uint8 *g_pOutBfr;
#endif

	

	
