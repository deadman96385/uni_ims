/******************************************************************************
 ** File Name:	  common_trace.c                                              *
 ** Author:       Xiaowei Luo                                                 *
 ** DATE:         11/19/2008                                                  *
 ** Copyright:    2006 Spreatrum, Incoporated. All Rights Reserved.           *
 ** Description:   c model of bsmr module in mpeg4 decoder                    *
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
#include "sc8810_video_header.h"
/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    extern   "C" 
    {
#endif

void fprintf_oneWord_hex (FILE * pf_txt, int val)
{
	int i;
	int tmp;

	if(pf_txt == NULL) return;
	
	for (i = 7; i >= 0; i--)
	{
		tmp = (val >> (4*i)) & 0xf;		
		fprintf (pf_txt, "%0x", tmp);//FPRINTF (pf_txt, "%0x", tmp);//weihu

	}

	fprintf (pf_txt, "\n");//FPRINTF (pf_txt, "\n");//weihu
}

#if defined(H264_DEC)
void PrintfRecFrame (DEC_IMAGE_PARAMS_T *img_ptr, DEC_STORABLE_PICTURE_T * g_dec_picture_ptr)
{
if(g_vector_enable_flag&TRACE_ENABLE_MBC)
{
	int			i;
	uint32		val;
	uint32 *	p_frame_ptr;
	int			frame_size;

	int			frame_width  = img_ptr->width;
	int			frame_height = img_ptr->height;

	//printf Y
	frame_size = (frame_width/4) * frame_height;
	p_frame_ptr = (uint32 *)g_dec_picture_ptr->imgY;
	for (i = 0; i < frame_size; i++)
	{
		val = p_frame_ptr[i];
		fprintf (g_fp_rec_frm_tv, "%08x\n", val);
	}

	//printf UV
	frame_size = (frame_width/4) * (frame_height/2);
	p_frame_ptr = (uint32 *)g_dec_picture_ptr->imgU;
	for (i = 0; i < frame_size; i++)
	{
		val = p_frame_ptr[i];
		fprintf (g_fp_rec_frm_tv, "%08x\n", val);
	}
}
}
#endif

void FormatPrintHexNum(int num, FILE *fp)
{
	//hi4,himi4,milo4,lo4
	unsigned char hi8, lo8;

	hi8 = (num >> 8) & 0xFF;
	lo8 = num & 0xFF;
	
	if(hi8 < 0x10)
		FPRINTF(fp, "0");				
	FPRINTF(fp, "%x",hi8);	

	if(lo8 < 0x10)
		FPRINTF(fp, "0");				
	FPRINTF(fp, "%x\n",lo8);					
}

void FomatPrint4Pix(uint8 *pLine, FILE *fp)
{
	int i;
#if _DEBUG_
	if((pLine[3] == 0xa0)&&(pLine[2] == 0xa0)&&(pLine[1]== 0x9e)&&(pLine[0]== 0x98))
	{
		pLine[0] = pLine[0];
	}
#endif //_DEBUG_
	
	for(i = 0; i < 4; i++)
	{
		if(pLine[3-i] < 0x10)
		{
			FPRINTF(fp, "0");
		}
		FPRINTF(fp, "%x",pLine[3-i]);
	}
	FPRINTF(fp, "\n");	
}

int g_trace_enable_flag;
int g_vector_enable_flag;

#define OPEN_VECTOR_FILE(module_flag, tv_file, module_file_name) {\
	if(g_vector_enable_flag&module_flag)\
{\
	strcpy(module_tv_file_path, test_vect_file_path);\
        assert(NULL != (tv_file = fopen(strcat(module_tv_file_path, module_file_name),"w")));\
}\
}

#define CLOSE_VECTOR_FILE(module_flag, tv_file) {\
	if(g_vector_enable_flag&module_flag)\
{\
	fclose(tv_file);\
}\
}

int InitVectorFiles()
{	
#ifdef _ARM_
#else
	char module_tv_file_path[200];
	#if defined(MPEG4_ENC)
		char test_vect_file_path[200] = "../../test_vectors/";

		OPEN_VECTOR_FILE(VECTOR_ENABLE_DBK, g_fp_dbk_tv, "dbk_out.txt");
		OPEN_VECTOR_FILE(VECTOR_ENABLE_DBK, g_fp_dbk_mb_tv, "dbk_mb_out.txt");
 		OPEN_VECTOR_FILE(VECTOR_ENABLE_MCA, g_fp_mca_tv, "mca_out.txt");

		OPEN_VECTOR_FILE(VECTOR_ENABLE_BSM, g_fp_bsm_tv, "bsmw_out.txt");

		OPEN_VECTOR_FILE(VECTOR_ENABLE_VLC, g_fp_vlc_tv, "vlc_out.txt");
		
		OPEN_VECTOR_FILE(VECTOR_ENABLE_MEA, g_fp_mea_tv, "mea_reference_fetch.txt");
		OPEN_VECTOR_FILE(VECTOR_ENABLE_MEA, g_fp_mea_src_mb_tv, "mea_out_smcu.txt");
		OPEN_VECTOR_FILE(VECTOR_ENABLE_MEA, g_fp_mea_ref_mb_tv, "mea_out_rmcu.txt");
		OPEN_VECTOR_FILE(VECTOR_ENABLE_MEA, g_fp_mea_src_frm_y_tv, "mea_src_frm_y.txt");
		OPEN_VECTOR_FILE(VECTOR_ENABLE_MEA, g_fp_mea_src_frm_u_tv, "mea_src_frm_u.txt");
		OPEN_VECTOR_FILE(VECTOR_ENABLE_MEA, g_fp_mea_src_frm_v_tv, "mea_src_frm_v.txt");

		OPEN_VECTOR_FILE(VECTOR_ENABLE_DCT, g_fp_dct_tv, "dct_out.txt");
		OPEN_VECTOR_FILE(VECTOR_ENABLE_DCT, g_fp_idct_tv, "idct_out.txt");
		OPEN_VECTOR_FILE(VECTOR_ENABLE_MBC, g_fp_mbc_tv, "mbc_out.txt");
	#elif defined(MPEG4_DEC)
		char test_vect_file_path[200] = "../../test_vectors/";
	
		OPEN_VECTOR_FILE(VECTOR_ENABLE_DBK, g_fp_dbk_tv, "dbk_out.txt");
		OPEN_VECTOR_FILE(VECTOR_ENABLE_DBK, g_fp_dbk_mb_tv, "dbk_mb_out.txt");
		OPEN_VECTOR_FILE(VECTOR_ENABLE_DBK, g_fp_dbk_frame_tv, "dbk_frame_out.txt");
		OPEN_VECTOR_FILE(VECTOR_ENABLE_DBK, g_fp_before_flt_tv, "dbk_before_filter.txt");

		OPEN_VECTOR_FILE(VECTOR_ENABLE_VLD, g_fp_vld_tv, "vld_out.txt");	

 		OPEN_VECTOR_FILE(VECTOR_ENABLE_MCA, g_fp_mca_tv, "mca_out.txt");

		OPEN_VECTOR_FILE(VECTOR_ENABLE_DCT, g_fp_idct_tv, "idct_out.txt");
				
		OPEN_VECTOR_FILE(VECTOR_ENABLE_MBC, g_fp_mbc_tv, "mbc_out.txt");
		OPEN_VECTOR_FILE(VECTOR_ENABLE_MBC, g_fp_rec_frm_tv, "rec_frame.txt");
	#elif defined(JPEG_ENC)
		char test_vect_file_path[200] = "test_vectors/";

		OPEN_VECTOR_FILE(VECTOR_ENABLE_VLC, g_fp_vlc_tv, "vlc_out.txt");

		OPEN_VECTOR_FILE(VECTOR_ENABLE_BSM, g_fp_bsm_tv, "bsmw_out.txt");

		OPEN_VECTOR_FILE(VECTOR_ENABLE_DCT, g_fp_dct_tv, "dct_out.txt");

		OPEN_VECTOR_FILE(VECTOR_ENABLE_MEA, g_fp_mea_src_mb_tv, "mea_out_smcu.txt");
		OPEN_VECTOR_FILE(VECTOR_ENABLE_MEA, g_fp_mea_ref_mb_tv, "mea_out_rmcu.txt");
		OPEN_VECTOR_FILE(VECTOR_ENABLE_MEA, g_fp_mea_src_frm_y_tv, "mea_src_frm_y.txt");
		OPEN_VECTOR_FILE(VECTOR_ENABLE_MEA, g_fp_mea_src_frm_u_tv, "mea_src_frm_u.txt");
		OPEN_VECTOR_FILE(VECTOR_ENABLE_MEA, g_fp_mea_src_frm_v_tv, "mea_src_frm_v.txt");		
	#elif defined(JPEG_DEC)
		char test_vect_file_path[200] = "test_vectors/";
		
		OPEN_VECTOR_FILE(VECTOR_ENABLE_DBK, g_fp_dbk_tv, "dbk_out.txt");
		OPEN_VECTOR_FILE(VECTOR_ENABLE_DBK, g_fp_dbk_mb_tv, "dbk_mb_out.txt");
		OPEN_VECTOR_FILE(VECTOR_ENABLE_DBK, g_fp_dbk_frame_tv, "dbk_frame_out.txt");
		
		OPEN_VECTOR_FILE(VECTOR_ENABLE_VLD, g_fp_vld_tv, "vld_out.txt");
		OPEN_VECTOR_FILE(VECTOR_ENABLE_DCT, g_fp_idct_tv, "idct_out.txt");
		
		OPEN_VECTOR_FILE(VECTOR_ENABLE_MBC, g_fp_mbc_tv, "mbc_out.txt");
		OPEN_VECTOR_FILE(VECTOR_ENABLE_MBC, g_fp_rec_frm_tv, "rec_frame.txt");
	#elif defined(H264_DEC)
		char test_vect_file_path[200] = "../../test_vectors/";
	
		OPEN_VECTOR_FILE(VECTOR_ENABLE_DBK, g_fp_dbk_tv, "dbk_out.txt");
		OPEN_VECTOR_FILE(VECTOR_ENABLE_DBK, g_fp_dbk_mb_tv, "dbk_mb_out.txt");
//		OPEN_VECTOR_FILE(VECTOR_ENABLE_DBK, g_fp_dbk_frame_tv, "dbk_frame_out.txt");
//		OPEN_VECTOR_FILE(VECTOR_ENABLE_DBK, g_fp_before_flt_tv, "dbk_before_filter.txt");
 
 		OPEN_VECTOR_FILE(VECTOR_ENABLE_MCA, g_fp_mca_tv, "mca_out.txt");
		
//		OPEN_VECTOR_FILE(VECTOR_ENABLE_DCT, g_fp_isqt_tv, "iqt_out.txt");
		
		OPEN_VECTOR_FILE(VECTOR_ENABLE_MBC, g_fp_mbc_tv, "mbc_out.txt");

		OPEN_VECTOR_FILE(VECTOR_ENABLE_MBC, g_fp_vld_tv, "vld_out.txt");

		OPEN_VECTOR_FILE(VECTOR_ENABLE_DCT, g_fp_idct_tv, "idct_out.txt");
		OPEN_VECTOR_FILE(VECTOR_ENABLE_MBC, g_fp_rec_frm_tv, "rec_frame.txt");
	#elif defined(REAL_DEC)
		char test_vect_file_path[200] = "../../test_vectors/";
	
		OPEN_VECTOR_FILE(VECTOR_ENABLE_DBK, g_fp_dbk_tv, "dbk_out.txt");
		OPEN_VECTOR_FILE(VECTOR_ENABLE_DBK, g_fp_dbk_mb_tv, "dbk_mb_out.txt");
//		OPEN_VECTOR_FILE(VECTOR_ENABLE_DBK, g_fp_dbk_frame_tv, "dbk_frame_out.txt");
//		OPEN_VECTOR_FILE(VECTOR_ENABLE_DBK, g_fp_before_flt_tv, "dbk_before_filter.txt");
 
 		OPEN_VECTOR_FILE(VECTOR_ENABLE_MCA, g_fp_mca_tv, "mca_out.txt");
		
	//	OPEN_VECTOR_FILE(VECTOR_ENABLE_DCT, g_fp_isqt_tv, "iqt_out.txt");
		
		OPEN_VECTOR_FILE(VECTOR_ENABLE_MBC, g_fp_mbc_tv, "mbc_out.txt");

		OPEN_VECTOR_FILE(VECTOR_ENABLE_MBC, g_fp_vld_tv, "vld_out.txt");

		OPEN_VECTOR_FILE(VECTOR_ENABLE_DCT, g_fp_idct_tv, "idct_out.txt");
		OPEN_VECTOR_FILE(VECTOR_ENABLE_MBC, g_fp_rec_frm_tv, "rec_frame.txt");
     #elif defined(VP8_DEC)
		char test_vect_file_path[200] = "../../test_vectors/";
		
		OPEN_VECTOR_FILE(VECTOR_ENABLE_DBK, g_fp_dbk_tv, "dbk_out.txt");
//		OPEN_VECTOR_FILE(VECTOR_ENABLE_DBK, g_fp_dbk_mb_tv, "dbk_mb_out.txt");
		OPEN_VECTOR_FILE(VECTOR_ENABLE_DBK, g_fp_dbk_para_tv, "dbk_para.txt");
		OPEN_VECTOR_FILE(VECTOR_ENABLE_DBK, g_fp_rec_frm_tv, "rec_frame.txt");
//		OPEN_VECTOR_FILE(VECTOR_ENABLE_DBK, g_fp_dbk_info_tv, "dbk_filt.txt");
		//		OPEN_VECTOR_FILE(VECTOR_ENABLE_DBK, g_fp_dbk_frame_tv, "dbk_frame_out.txt");
		//		OPEN_VECTOR_FILE(VECTOR_ENABLE_DBK, g_fp_before_flt_tv, "dbk_before_filter.txt");
#ifndef MCA_TV_SPLIT
//		OPEN_VECTOR_FILE(VECTOR_ENABLE_MCA, g_fp_ref_frm0_tv, "ref_frame_0.txt");
//		OPEN_VECTOR_FILE(VECTOR_ENABLE_MCA, g_fp_ref_frm1_tv, "ref_frame_1.txt");
//		OPEN_VECTOR_FILE(VECTOR_ENABLE_MCA, g_fp_ref_frm2_tv, "ref_frame_2.txt");
//		OPEN_VECTOR_FILE(VECTOR_ENABLE_MCA, g_fp_vp8_mca_in_luma_tv, "mca_in_luma.txt");
//		OPEN_VECTOR_FILE(VECTOR_ENABLE_MCA, g_fp_vp8_mca_in_chroma_tv, "mca_in_chroma.txt");
//		OPEN_VECTOR_FILE(VECTOR_ENABLE_MCA, g_fp_mca_hor_tv, "mca_hor_out.txt");
#ifdef MBC_TV
		OPEN_VECTOR_FILE(VECTOR_ENABLE_MCA, g_fp_mca_tv, "mbc_ipred.txt");
#else
		OPEN_VECTOR_FILE(VECTOR_ENABLE_MCA, g_fp_mca_tv, "mca_out.txt");	// vp8_pred_out.txt, mbc_ipred.txt
#endif
		OPEN_VECTOR_FILE(VECTOR_ENABLE_MCA, g_fp_mca_para_tv, "mca_para.txt");
#endif
		//		OPEN_VECTOR_FILE(VECTOR_ENABLE_DCT, g_fp_isqt_tv, "iqt_out.txt");		
		OPEN_VECTOR_FILE(VECTOR_ENABLE_MBC, g_fp_mbc_tv, "mbc_out.txt");	// vp8_rec_out.txt
		OPEN_VECTOR_FILE(VECTOR_ENABLE_MBC, g_fp_mbc_para_tv, "mbc_para.txt");
		FPRINTF_PPA (g_fp_mbc_para_tv, "//STD=0010 ENC=0\n");
		OPEN_VECTOR_FILE(VECTOR_ENABLE_MBC, g_fp_vld_tv, "vld_out.txt");
//		OPEN_VECTOR_FILE(VECTOR_ENABLE_DCT, g_fp_vp8_iq_tv, "vp8_iq_out.txt");	// vp8_iq_out.txt
#ifdef MBC_TV
		OPEN_VECTOR_FILE(VECTOR_ENABLE_DCT, g_fp_idct_tv, "mbc_idct_in.txt");	// mbc_idct_in.txt
#else
		OPEN_VECTOR_FILE(VECTOR_ENABLE_DCT, g_fp_idct_tv, "idct_out.txt");	// mbc_idct_in.txt
#endif
		OPEN_VECTOR_FILE(VECTOR_ENABLE_DCT, g_fp_dct_para_tv, "dct_para.txt");
		FPRINTF_PPA (g_fp_dct_para_tv, "//dct_mode=0 vsp_standard=2 scale_enable=0\n");

		// Frame Header VLD
		OPEN_VECTOR_FILE(VECTOR_ENABLE_COUNT, g_fp_vp8_bs_off_tv, "bs_offset.txt");
		OPEN_VECTOR_FILE(VECTOR_ENABLE_FW, g_fp_vp8_fh_cfg_tv, "vpx_cfg.txt");
		OPEN_VECTOR_FILE(VECTOR_ENABLE_FW, g_fp_vp8_ppa_cfg_tv, "ppa_cfg.txt");
		OPEN_VECTOR_FILE(VECTOR_ENABLE_FW, g_fp_vp8_prob_tv, "vpx_tbuf_prob.txt");
		OPEN_VECTOR_FILE(VECTOR_ENABLE_FW, g_fp_vp8_part_buf_tv, "partition_in.txt");
		OPEN_VECTOR_FILE(VECTOR_ENABLE_FW, g_fp_vp8_isyn_buf_tv, "ppa_in.txt");
		OPEN_VECTOR_FILE(VECTOR_ENABLE_FW, g_fp_vp8_ppal_buf_tv, "ppa_linebuf.txt");

		// GLB REG
		OPEN_VECTOR_FILE(VECTOR_ENABLE_FW, g_vsp_glb_reg_fp, "vsp_glb_reg.txt");
    #else
		assert(0);
	#endif

    #if defined(MCcache)//weihu
        OPEN_VECTOR_FILE(VECTOR_ENABLE_CACHE, g_fp_mc_cache, "cache_hit.txt");
    #endif
//	OPEN_VECTOR_FILE(VECTOR_ENABLE_FW, g_fw_tv_cmd, "sw_fw_tv_cmd.dat");	
//	OPEN_VECTOR_FILE(VECTOR_ENABLE_FW, g_fw_tv_cmd_info, "cmd_info.dat"); 
// 	OPEN_VECTOR_FILE(VECTOR_ENABLE_FW, g_fw_tv_cmd_data, "cmd_data.dat");
// 	OPEN_VECTOR_FILE(VECTOR_ENABLE_FW, g_fw_tv_cmd_data_notes, "sw_cmd_data.dat");
#endif
	return 1;
}

int DenitVectorFiles(void)
{
#ifdef _ARM_	
#else	
	#if defined(MPEG4_ENC)
		CLOSE_VECTOR_FILE(VECTOR_ENABLE_DBK, g_fp_dbk_tv);
 		CLOSE_VECTOR_FILE(VECTOR_ENABLE_MCA, g_fp_mca_tv);

		CLOSE_VECTOR_FILE(VECTOR_ENABLE_BSM, g_fp_bsm_tv);

		CLOSE_VECTOR_FILE(VECTOR_ENABLE_VLC, g_fp_vlc_tv);
		
		CLOSE_VECTOR_FILE(VECTOR_ENABLE_MEA, g_fp_mea_tv);
		CLOSE_VECTOR_FILE(VECTOR_ENABLE_MEA, g_fp_mea_src_mb_tv);	
		CLOSE_VECTOR_FILE(VECTOR_ENABLE_MEA, g_fp_mea_ref_mb_tv);
		CLOSE_VECTOR_FILE(VECTOR_ENABLE_MEA, g_fp_mea_src_frm_y_tv);
		CLOSE_VECTOR_FILE(VECTOR_ENABLE_MEA, g_fp_mea_src_frm_u_tv);
		CLOSE_VECTOR_FILE(VECTOR_ENABLE_MEA, g_fp_mea_src_frm_v_tv);

		CLOSE_VECTOR_FILE(VECTOR_ENABLE_DCT, g_fp_dct_tv);
		CLOSE_VECTOR_FILE(VECTOR_ENABLE_DCT, g_fp_idct_tv);
		CLOSE_VECTOR_FILE(VECTOR_ENABLE_MBC, g_fp_mbc_tv);
	#elif defined(MPEG4_DEC)
		CLOSE_VECTOR_FILE(VECTOR_ENABLE_DBK, g_fp_dbk_tv);
		CLOSE_VECTOR_FILE(VECTOR_ENABLE_DBK, g_fp_dbk_mb_tv);
		CLOSE_VECTOR_FILE(VECTOR_ENABLE_DBK, g_fp_dbk_frame_tv);
		CLOSE_VECTOR_FILE(VECTOR_ENABLE_DBK, g_fp_before_flt_tv);

		CLOSE_VECTOR_FILE(VECTOR_ENABLE_VLD, g_fp_vld_tv);

 		CLOSE_VECTOR_FILE(VECTOR_ENABLE_MCA, g_fp_mca_tv);
		
		CLOSE_VECTOR_FILE(VECTOR_ENABLE_DCT, g_fp_idct_tv);
		
		CLOSE_VECTOR_FILE(VECTOR_ENABLE_MBC, g_fp_mbc_tv);
		CLOSE_VECTOR_FILE(VECTOR_ENABLE_MBC, g_fp_rec_frm_tv);

	#elif defined(JPEG_ENC)
		CLOSE_VECTOR_FILE(VECTOR_ENABLE_VLC, g_fp_vlc_tv);

		CLOSE_VECTOR_FILE(VECTOR_ENABLE_BSM, g_fp_bsm_tv);

		CLOSE_VECTOR_FILE(VECTOR_ENABLE_DCT, g_fp_dct_tv);

		CLOSE_VECTOR_FILE(VECTOR_ENABLE_MEA, g_fp_mea_src_mb_tv);
		CLOSE_VECTOR_FILE(VECTOR_ENABLE_MEA, g_fp_mea_ref_mb_tv);
		CLOSE_VECTOR_FILE(VECTOR_ENABLE_MEA, g_fp_mea_src_frm_y_tv);
		CLOSE_VECTOR_FILE(VECTOR_ENABLE_MEA, g_fp_mea_src_frm_u_tv);
		CLOSE_VECTOR_FILE(VECTOR_ENABLE_MEA, g_fp_mea_src_frm_v_tv);
	#elif defined(JPEG_DEC)		
		CLOSE_VECTOR_FILE(VECTOR_ENABLE_DBK, g_fp_dbk_tv);
		CLOSE_VECTOR_FILE(VECTOR_ENABLE_DBK, g_fp_dbk_mb_tv);
		CLOSE_VECTOR_FILE(VECTOR_ENABLE_DBK, g_fp_dbk_frame_tv);

		CLOSE_VECTOR_FILE(VECTOR_ENABLE_VLD, g_fp_vld_tv);
		CLOSE_VECTOR_FILE(VECTOR_ENABLE_DCT, g_fp_idct_tv);
		
		CLOSE_VECTOR_FILE(VECTOR_ENABLE_MBC, g_fp_mbc_tv);
		CLOSE_VECTOR_FILE(VECTOR_ENABLE_MBC, g_fp_rec_frm_tv);
	#elif defined(H264_DEC)
		CLOSE_VECTOR_FILE(VECTOR_ENABLE_DBK, g_fp_dbk_tv);
		CLOSE_VECTOR_FILE(VECTOR_ENABLE_DBK, g_fp_dbk_mb_tv);
		CLOSE_VECTOR_FILE(VECTOR_ENABLE_DBK, g_fp_dbk_frame_tv);
		CLOSE_VECTOR_FILE(VECTOR_ENABLE_DBK, g_fp_before_flt_tv);

 		CLOSE_VECTOR_FILE(VECTOR_ENABLE_MCA, g_fp_mca_tv);
		
		CLOSE_VECTOR_FILE(VECTOR_ENABLE_DCT, g_fp_isqt_tv);
		
		CLOSE_VECTOR_FILE(VECTOR_ENABLE_MBC, g_fp_mbc_tv);
	#elif defined(REAL_DEC)
		CLOSE_VECTOR_FILE(VECTOR_ENABLE_DBK, g_fp_dbk_tv);
		CLOSE_VECTOR_FILE(VECTOR_ENABLE_DBK, g_fp_dbk_mb_tv);
//		CLOSE_VECTOR_FILE(VECTOR_ENABLE_DBK, g_fp_dbk_frame_tv);
//		CLOSE_VECTOR_FILE(VECTOR_ENABLE_DBK, g_fp_before_flt_tv);

 		CLOSE_VECTOR_FILE(VECTOR_ENABLE_MCA, g_fp_mca_tv);
		
		CLOSE_VECTOR_FILE(VECTOR_ENABLE_MBC, g_fp_mbc_tv);

 		CLOSE_VECTOR_FILE(VECTOR_ENABLE_MBC, g_fp_vld_tv);

		CLOSE_VECTOR_FILE(VECTOR_ENABLE_MBC, g_fp_idct_tv);

		CLOSE_VECTOR_FILE(VECTOR_ENABLE_MBC, g_fp_rec_frm_tv);
    #elif defined(VP8_DEC)
		CLOSE_VECTOR_FILE(VECTOR_ENABLE_DBK, g_fp_dbk_tv);
//		CLOSE_VECTOR_FILE(VECTOR_ENABLE_DBK, g_fp_dbk_mb_tv);
		CLOSE_VECTOR_FILE(VECTOR_ENABLE_DBK, g_fp_dbk_para_tv);
//		CLOSE_VECTOR_FILE(VECTOR_ENABLE_DBK, g_fp_dbk_info_tv);
//		CLOSE_VECTOR_FILE(VECTOR_ENABLE_DBK, g_fp_dbk_frame_tv);
		CLOSE_VECTOR_FILE(VECTOR_ENABLE_DBK, g_fp_before_flt_tv);
		CLOSE_VECTOR_FILE(VECTOR_ENABLE_DBK, g_fp_rec_frm_tv);
//		CLOSE_VECTOR_FILE(VECTOR_ENABLE_MCA, g_fp_vp8_mca_in_luma_tv);
//		CLOSE_VECTOR_FILE(VECTOR_ENABLE_MCA, g_fp_vp8_mca_in_chroma_tv);
//		CLOSE_VECTOR_FILE(VECTOR_ENABLE_MCA, g_fp_mca_hor_tv);
		CLOSE_VECTOR_FILE(VECTOR_ENABLE_MCA, g_fp_mca_tv);
		CLOSE_VECTOR_FILE(VECTOR_ENABLE_MCA, g_fp_mca_para_tv);
//		CLOSE_VECTOR_FILE(VECTOR_ENABLE_DCT, g_fp_vp8_iq_tv);
		CLOSE_VECTOR_FILE(VECTOR_ENABLE_DCT, g_fp_isqt_tv);		
		CLOSE_VECTOR_FILE(VECTOR_ENABLE_MBC, g_fp_mbc_tv);
		CLOSE_VECTOR_FILE(VECTOR_ENABLE_MBC, g_fp_mbc_para_tv);
		CLOSE_VECTOR_FILE(VECTOR_ENABLE_MBC, g_fp_idct_tv);
		CLOSE_VECTOR_FILE(VECTOR_ENABLE_MBC, g_fp_dct_para_tv);
		CLOSE_VECTOR_FILE(VECTOR_ENABLE_MBC, g_fp_vld_tv);

		// Frame Header VLD
		CLOSE_VECTOR_FILE(VECTOR_ENABLE_COUNT, g_fp_vp8_bs_off_tv);
		CLOSE_VECTOR_FILE(VECTOR_ENABLE_FW, g_fp_vp8_fh_cfg_tv);
		CLOSE_VECTOR_FILE(VECTOR_ENABLE_FW, g_fp_vp8_ppa_cfg_tv);
		CLOSE_VECTOR_FILE(VECTOR_ENABLE_FW, g_fp_vp8_prob_tv);
		CLOSE_VECTOR_FILE(VECTOR_ENABLE_FW, g_fp_vp8_part_buf_tv);
		CLOSE_VECTOR_FILE(VECTOR_ENABLE_FW, g_fp_vp8_isyn_buf_tv);
		CLOSE_VECTOR_FILE(VECTOR_ENABLE_FW, g_fp_vp8_ppal_buf_tv);

		// GLB REG	
		CLOSE_VECTOR_FILE(VECTOR_ENABLE_FW, g_vsp_glb_reg_fp);
    #else
		assert(0);
	#endif
    
    #if defined(MCcache)//weihu
         CLOSE_VECTOR_FILE(VECTOR_ENABLE_CACHE, g_fp_mc_cache);
    #endif
//	CLOSE_VECTOR_FILE(VECTOR_ENABLE_FW, g_fw_tv_cmd);	
//	CLOSE_VECTOR_FILE(VECTOR_ENABLE_FW, g_fw_tv_cmd_info);
//	CLOSE_VECTOR_FILE(VECTOR_ENABLE_FW, g_fw_tv_cmd_data); 
// 	CLOSE_VECTOR_FILE(VECTOR_ENABLE_FW, g_fw_tv_cmd_data_notes);
#endif
	return 1;
}

#define OPEN_TRACE_FILE(module_flag, trace_file, module_file_name) {\
	if(g_trace_enable_flag&module_flag)\
{\
	strcpy(module_trace_file_path, trace_file_path);\
        assert(NULL != (trace_file = fopen(strcat(module_trace_file_path, module_file_name),"w")));\
}\
}

#define CLOSE_TRACE_FILE(module_flag, trace_file) {\
	if(g_trace_enable_flag&module_flag)\
{\
	fclose(trace_file);\
}\
}

int InitTraceFiles()
{
#if defined(SIM_IN_WIN)
	char module_trace_file_path[200];
	#if defined(MPEG4_ENC)
		char trace_file_path[200] = "../../trace/";
	#elif defined(MPEG4_DEC)
		char trace_file_path[200] = "../../trace/";
	#elif defined(JPEG_ENC)
		char trace_file_path[200] = "trace/";
	#elif defined(JPEG_DEC)
		char trace_file_path[200] = "trace/";
	#elif defined(H264_DEC)
		char trace_file_path[200] = "../../trace/";
	#elif defined(REAL_DEC)
		char trace_file_path[200] = "../../trace/";
    #elif defined(VP8_DEC)
		char trace_file_path[200] = "../../trace/";
	#else
		assert(0);
	#endif

	OPEN_TRACE_FILE(TRACE_ENABLE_DBK, g_fp_trace_dbk, "trace_dbk.txt");
	OPEN_TRACE_FILE(TRACE_ENABLE_FW, g_fp_trace_fw, "trace_fw.txt");
	OPEN_TRACE_FILE(TRACE_ENABLE_FW, g_fp_dbp, "trace_dpb.txt");
	OPEN_TRACE_FILE(TRACE_ENABLE_FW, g_fp_trace_fw_bs, "trace_fw_bs.txt");
	OPEN_TRACE_FILE(TRACE_ENABLE_VLD, g_fp_trace_vld, "trace_vld.txt");
	OPEN_TRACE_FILE(TRACE_ENABLE_MCA, g_fp_trace_mca, "trace_mca.txt");
		
	if(( g_stream_type == STREAM_ID_AVS) ||( g_stream_type == STREAM_ID_H264)||
	   ( g_stream_type==STREAM_ID_REAL8) ||(g_stream_type == STREAM_ID_REAL9))
	{
		OPEN_TRACE_FILE(TRACE_ENABLE_ISQT, g_fp_trace_isqt, "trace_isqt.txt");
		OPEN_TRACE_FILE(TRACE_ENABLE_IPRED, g_fp_trace_ipred, "trace_ipred.txt");
		OPEN_TRACE_FILE(TRACE_ENABLE_IPRED, g_ipred_log_fp, "ipred_log.txt");
		OPEN_TRACE_FILE(TRACE_ENABLE_FW, pFmoFile, "trace_fmo.txt");
		OPEN_TRACE_FILE(TRACE_ENABLE_FW, g_fp_trace_mb_info, "trace_mb_info.txt");
		OPEN_TRACE_FILE(TRACE_ENABLE_MBC, g_fp_trace_mbc, "trace_mbc.txt");//weihu
    }else
	{
		OPEN_TRACE_FILE(TRACE_ENABLE_DCT, g_pf_before_iqidct, "trace_before_iqidct.txt");
		OPEN_TRACE_FILE(TRACE_ENABLE_DCT, g_pf_after_iq, "trace_after_iq.txt");
		OPEN_TRACE_FILE(TRACE_ENABLE_DCT, g_pf_after_iqidct, "trace_after_iqidct.txt");

		OPEN_TRACE_FILE(TRACE_ENABLE_MBC, g_fp_trace_mbc, "trace_mbc.txt");
		
		if((g_stream_type == STREAM_ID_H264) || (g_stream_type == STREAM_ID_REAL8)||( g_stream_type == STREAM_ID_REAL9)	)
		{
			OPEN_TRACE_FILE(TRACE_ENABLE_IPRED, g_fp_trace_ipred, "ipred_log.txt");
		}

	#if (defined(JPEG_ENC)||defined(MPEG4_ENC))
		OPEN_TRACE_FILE(TRACE_ENABLE_MEA, g_fp_trace_mea, "trace_mea.txt");
		OPEN_TRACE_FILE(TRACE_ENABLE_MEA, g_fp_trace_mea_src_mb, "trace_mea_src_mb.txt");
		OPEN_TRACE_FILE(TRACE_ENABLE_MEA, g_fp_trace_mea_ref_mb, "trace_mea_ref_mb.txt");
		OPEN_TRACE_FILE(TRACE_ENABLE_MEA, g_fp_trace_mea_sad, "trace_mea_sad.txt");
		OPEN_TRACE_FILE(TRACE_ENABLE_VLC, g_fp_trace_vlc, "trace_vlc.txt");
	#endif //(defined(JPEG_ENC)||(MPEG4_ENC))
	
	}
#endif //defined(SIM_IN_WIN)   

	return 1;
}

int DenitTraceFiles()
{
#if defined(SIM_IN_WIN)
	CLOSE_TRACE_FILE(TRACE_ENABLE_DBK, g_fp_trace_dbk);
	CLOSE_TRACE_FILE(TRACE_ENABLE_FW, g_fp_trace_fw);
	CLOSE_TRACE_FILE(TRACE_ENABLE_FW, g_fp_dbp);
	CLOSE_TRACE_FILE(TRACE_ENABLE_FW, g_fp_trace_fw_bs);
	CLOSE_TRACE_FILE(TRACE_ENABLE_VLD, g_fp_trace_vld);
	CLOSE_TRACE_FILE(TRACE_ENABLE_MCA, g_fp_trace_mca);

	if(( g_stream_type == STREAM_ID_AVS) ||( g_stream_type == STREAM_ID_H264)||
	   ( g_stream_type==STREAM_ID_REAL8) ||(g_stream_type == STREAM_ID_REAL9))
	{
		CLOSE_TRACE_FILE(TRACE_ENABLE_VLD, g_fp_trace_vld);
		CLOSE_TRACE_FILE(TRACE_ENABLE_ISQT, g_fp_trace_isqt);
		CLOSE_TRACE_FILE(TRACE_ENABLE_IPRED, g_fp_trace_ipred);	
		CLOSE_TRACE_FILE(TRACE_ENABLE_MBC, g_ipred_log_fp);
		CLOSE_TRACE_FILE(TRACE_ENABLE_FW, pFmoFile);
		CLOSE_TRACE_FILE(TRACE_ENABLE_FW, g_fp_trace_mb_info);
		CLOSE_TRACE_FILE(TRACE_ENABLE_MBC, g_fp_trace_mbc);//weihu
	}else
	{
		CLOSE_TRACE_FILE(TRACE_ENABLE_DCT, g_pf_before_iqidct);
		CLOSE_TRACE_FILE(TRACE_ENABLE_DCT, g_pf_after_iq);
		CLOSE_TRACE_FILE(TRACE_ENABLE_DCT, g_pf_after_iqidct);
		CLOSE_TRACE_FILE(TRACE_ENABLE_MBC, g_fp_trace_mbc);
		
		if((g_stream_type == STREAM_ID_REAL8)||( g_stream_type == STREAM_ID_REAL9)	)
		{
			CLOSE_TRACE_FILE(TRACE_ENABLE_IPRED, g_fp_trace_ipred);
		}

	#if (defined(JPEG_ENC)||defined(MPEG4_ENC))
		CLOSE_TRACE_FILE(TRACE_ENABLE_MEA, g_fp_trace_mea);
		CLOSE_TRACE_FILE(TRACE_ENABLE_MEA, g_fp_trace_mea_src_mb);
		CLOSE_TRACE_FILE(TRACE_ENABLE_MEA, g_fp_trace_mea_ref_mb);
		CLOSE_TRACE_FILE(TRACE_ENABLE_MEA, g_fp_trace_mea_sad);

		CLOSE_TRACE_FILE(TRACE_ENABLE_VLC, g_fp_trace_vlc);
	#endif
	}
#endif // SIM_IN_WIN   

	return 1;
}

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End 




