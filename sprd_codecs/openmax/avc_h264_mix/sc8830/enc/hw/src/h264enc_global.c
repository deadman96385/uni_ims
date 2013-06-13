/******************************************************************************
 ** File Name:    h264enc_global.c											  *
 ** Author:       Xiaowei Luo                                                 *
 ** DATE:         12/14/2006                                                  *
 ** Copyright:    2006 Spreatrum, Incoporated. All Rights Reserved.           *
 ** Description:                                                              *
 *****************************************************************************/
/******************************************************************************
 **                   Edit    History                                         *
 **---------------------------------------------------------------------------* 
 ** DATE          NAME            DESCRIPTION                                 * 
 ** 12/14/2006    Xiaowei Luo     Create.                                     *
 *****************************************************************************/
/*----------------------------------------------------------------------------*
**                        Dependencies                                        *
**---------------------------------------------------------------------------*/
#include "sc6800x_video_header.h"
/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    extern   "C" 
    {
#endif

uint32 g_nFrame_enc;
ENC_IMAGE_PARAMS_T *g_enc_image_ptr;
ENC_MB_CACHE_T *g_mb_cache_ptr;
MMEncConfig * g_h264_enc_config;
uint8 or1200_print;		// derek-2012-08-10
int	  g_decimate_en = 0;	// derek-2012-08-14, SKIP done in MEA
uint8 * g_vlc_hw_ptr;

double g_PSNR [3] = {0, 0, 0};

int32 g_is_yuv_frm_malloced;

#ifdef SIM_IN_WIN
//	FILE * g_bit_stat_fp;
	FILE * stat_out;
#endif


//used for malloc
LOCAL uint8 *s_inter_mem_bfr_ptr;		
LOCAL uint8 *s_extra_mem_bfr_ptr;		
PUBLIC uint32 g_inter_malloced_size;
PUBLIC uint32 g_extra_malloced_size;

PUBLIC int32 g_stream_type;

PUBLIC uint32 inter_malloc_mem_start_addr;
PUBLIC uint32 total_inter_malloc_size;
PUBLIC uint32 extra_malloc_mem_start_addr;
PUBLIC uint32 total_extra_malloc_size;
PUBLIC uint32 bs_start_addr;
PUBLIC uint32 bs_buffer_length;

PUBLIC uint32 input_buffer_update;	//input buffer内容更新了，需要BSM重新load数据
PUBLIC uint32 cpu_will_be_reset;	//arm将会reset openrisc cpu，通知退出前保存现场
PUBLIC uint32 g_not_first_reset;	//不是第一次reset，需要恢复现场//weihu
//PUBLIC uint32 video_size_get;		//获得image size了
PUBLIC uint32 video_buffer_malloced;//已经向arm申请了在D
PUBLIC uint32 or_addr_offset;

#ifdef _LIB
	uint8 one_frame_stream[ONEFRAME_BITSTREAM_BFR_SIZE];
	uint32 off_frame_strm = 0;
#endif

RC_BU_PARAS rc_bu_paras;
RC_GOP_PARAS rc_gop_paras;
RC_PIC_PARAS rc_pic_paras;
uint32 BU_bit_stat;
uint32 prev_qp;
	
	
#if defined(SIM_IN_WIN) 
#if defined(JPEG_ENC)||defined(MPEG4_ENC)||defined(H264_ENC)
	FILE *s_pEnc_input_src_file;
	FILE *s_pEnc_output_bs_file;
	FILE *s_pEnc_output_recon_file;
#endif
#if defined(JPEG_DEC)|| defined(MPEG4_DEC)||defined(H264_DEC)
	FILE *s_pDec_input_bs_file; //file pointer of H.264 bit stream
	FILE *s_pDec_recon_yuv_file; //file pointer of reconstructed YUV sequence
	FILE *s_pDec_disp_yuv_file; //file pointer of display YUV sequence
#endif //#if defined(JPEG_ENC)
#else 
    uint8 *ptrInbfr;
    int g_streamLen;
    uint8 *ptrOutBfr; // to store the decoded frame
    uint8 *g_pOutBfr;
#endif

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End 

