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

	//fprintf (pf_txt, "\n");//FPRINTF (pf_txt, "\n");//weihu
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
	uint8  *	p;//jzy
	int			j;

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

	if (g_nFrame_dec_h264==0)
	{
		fprintf (g_fp_rec_frmy_tv,"frame_width=%d,frame_height=%d\n",frame_width,frame_height);
		fprintf (g_fp_rec_frmuv_tv,"frame_width=%d,frame_height=%d\n",frame_width,frame_height);

		frame_size = (frame_width/8) * frame_height;
		p = g_dec_picture_ptr->imgY;
		for (i = 0; i < frame_size; i++)
		{
			for (j = 0; j<8; j++)
			{
			val = p[8*i+7-j];		
			fprintf (g_fp_rec_frmy_tv, "%02x", val);
			}
			fprintf (g_fp_rec_frmy_tv, "\n");

		}

		frame_size =(frame_width/8) * (frame_height/2);
		p = g_dec_picture_ptr->imgU;
		for (i = 0; i < frame_size; i++)
		{
			for (j = 0; j<8; j++)
			{
			val = p[8*i+7-j];		
			fprintf (g_fp_rec_frmuv_tv, "%02x", val);
			}
			fprintf (g_fp_rec_frmuv_tv, "\n");

		}
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
	if((pLine[3] != 0x00)&&(pLine[2] == 0xa0)&&(pLine[1]== 0x9e)&&(pLine[0]== 0x98))
	{
		pLine[0] = pLine[0];
	}
#endif //_DEBUG_
	
	for(i = 0; i < 4; i++)
	{
		if(pLine[3-i] < 0x10)
		{
			fprintf(fp, "0");//FPRINTF(fp, "0");//weihu
		}
		fprintf(fp, "%x",pLine[3-i]);//FPRINTF
	}
	//fprintf(fp, "\n");//FPRINTF	
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
#elif !defined(_LIB) 
	char module_tv_file_path[200];
	#if defined(MPEG4_ENC)
		char test_vect_file_path[200] = "../../test_vectors/";

		OPEN_VECTOR_FILE(VECTOR_ENABLE_DBK, g_fp_dbk_tv, "dbk_para.txt");
		OPEN_VECTOR_FILE(VECTOR_ENABLE_DBK, g_fp_dbk_mb_tv, "dbk_out.txt");
// 		OPEN_VECTOR_FILE(VECTOR_ENABLE_MCA, g_fp_mca_tv, "mca_out.txt");

		OPEN_VECTOR_FILE(VECTOR_ENABLE_BSM, g_fp_bsm_tv, "bsm_enc_out.txt");
		OPEN_VECTOR_FILE(VECTOR_ENABLE_BSM, g_fp_bsm_total_bits_tv, "bsm_total_bits.txt");

		OPEN_VECTOR_FILE(TRACE_ENABLE_MEA, g_fp_mea_tv, "mea_reference_fetch.txt");

		OPEN_VECTOR_FILE(VECTOR_ENABLE_FW, g_fp_global_tv, "vsp_glb_reg.txt");
		OPEN_VECTOR_FILE(VECTOR_ENABLE_MEA, g_fp_mea_src_frm_y_tv, "mea_fm_src_y.txt");
		OPEN_VECTOR_FILE(VECTOR_ENABLE_MEA, g_fp_mea_src_frm_u_tv, "mea_fm_src_uv.txt");
//		OPEN_VECTOR_FILE(VECTOR_ENABLE_MEA, g_fp_mea_src_frm_v_tv, "mea_src_frm_v.txt");
		OPEN_VECTOR_FILE(VECTOR_ENABLE_MEA, g_fp_mea_src_mb_tv, "ime_cur_buf.txt");//james
		OPEN_VECTOR_FILE(VECTOR_ENABLE_MEA, g_fp_mea_mb_ime_tv, "ime_fbuf.txt");//james
		OPEN_VECTOR_FILE(VECTOR_ENABLE_MEA, g_fp_mea_mb_tv, "mea_cbuf.txt");//james
//		OPEN_VECTOR_FILE(VECTOR_ENABLE_MEA, g_fp_mea_mb_tv, "mbc_src_mb.txt");//james
		OPEN_VECTOR_FILE(VECTOR_ENABLE_MEA, g_fp_mea_mca_mb_tv, "mea_pbuf.txt");//james
//		OPEN_VECTOR_FILE(VECTOR_ENABLE_MEA, g_fp_mea_mca_mb_tv, "mca_out.txt");//james
		OPEN_VECTOR_FILE(VECTOR_ENABLE_MEA, g_fp_mea_ref_frm_y_tv, "mea_fm_ref_y.txt");//james
		OPEN_VECTOR_FILE(VECTOR_ENABLE_MEA, g_fp_mea_ref_frm_u_tv, "mea_fm_ref_uv.txt");//james
		OPEN_VECTOR_FILE(VECTOR_ENABLE_MEA, g_fp_ime_mb_sad_tv, "ime_mb_sad.txt");//james
		OPEN_VECTOR_FILE(VECTOR_ENABLE_MEA, g_fp_fme_mb_sad_tv, "fme_mb_sad.txt");//james
		OPEN_VECTOR_FILE(VECTOR_ENABLE_MEA, g_fp_ime_best_sad_tv, "ime_best_sad.txt");//james
		OPEN_VECTOR_FILE(VECTOR_ENABLE_MEA, g_fp_fme_best_sad_tv, "fme_best_sad.txt");//james
		OPEN_VECTOR_FILE(VECTOR_ENABLE_MEA, g_fp_mea_ppa_buf_tv, "mea_para_buf.txt");//james
		OPEN_VECTOR_FILE(VECTOR_ENABLE_MEA, g_fp_mea_cfg_tv, "mea_config_frame.txt");//james
		if(g_vector_enable_flag&VECTOR_ENABLE_MEA)
		{
			fprintf(g_fp_mea_cfg_tv, "frm_cnt=%x\n", g_input->frame_num_enc);
			fprintf(g_fp_mea_cfg_tv, "0x%x \t\t // Width\n", g_input->pic_width);
			fprintf(g_fp_mea_cfg_tv, "0x%x \t\t // Height\n", g_input->pic_height);
			fprintf(g_fp_mea_cfg_tv, "%x \t\t // Search Rounds\n", MAX_SEARCH_CYCLE);
			fprintf(g_fp_mea_cfg_tv, "%x \t\t // QP\n", g_input->step_P);
		}
		OPEN_VECTOR_FILE(VECTOR_ENABLE_DCT, g_fp_frame_cfg, "mea_config.txt");
		OPEN_VECTOR_FILE(VECTOR_ENABLE_DCT, g_fp_mvlc_cfg, "mvlc_cfg.txt");
		if(g_vector_enable_flag&VECTOR_ENABLE_DCT)
		{
			fprintf(g_fp_mvlc_cfg, "frm_cnt=%x\n", g_input->frame_num_enc);
			fprintf(g_fp_mvlc_cfg, "mb_x_max=%x\n", g_input->pic_width/MB_SIZE);
			fprintf(g_fp_mvlc_cfg, "mb_y_max=%x\n", g_input->pic_height/MB_SIZE);
		}
		//iea
		OPEN_VECTOR_FILE(VECTOR_ENABLE_IEA, g_fp_iea_src_mb_tv, "iea_cbuf.txt");
		OPEN_VECTOR_FILE(VECTOR_ENABLE_IEA, g_fp_iea_in_tv, "iea_in.txt");
//		OPEN_VECTOR_FILE(VECTOR_ENABLE_IEA, g_fp_iea_out_tv, "iea_out.txt");

		OPEN_VECTOR_FILE(VECTOR_ENABLE_DCT, g_fp_huff_tab_tv, "vlc_table.txt");
#ifndef VLC_TV_SPLIT
		OPEN_VECTOR_FILE(VECTOR_ENABLE_PPA, g_fp_vlc_para_tv, "vlc_para.txt");
		OPEN_VECTOR_FILE(VECTOR_ENABLE_PPA, g_fp_dct_para_tv, "dct_para.txt");
		OPEN_VECTOR_FILE(VECTOR_ENABLE_DCT, g_fp_dct_out_tv, "dct_out.txt");
		OPEN_VECTOR_FILE(VECTOR_ENABLE_VLC, g_fp_vlc_tv, "vlc_out.txt");
		if(g_vector_enable_flag&VECTOR_ENABLE_PPA)
			fprintf(g_fp_dct_para_tv, "//dct_mode=1 vsp_standard=1 scale_enable=0\n");
#endif
		//ppa
		OPEN_VECTOR_FILE(VECTOR_ENABLE_PPA, g_fp_ppa_cfg_tv, "ppa_cfg.txt");
		OPEN_VECTOR_FILE(VECTOR_ENABLE_PPA, g_fp_ppa_in_tv, "ppa_in.txt");
		OPEN_VECTOR_FILE(VECTOR_ENABLE_PPA, g_fp_mbc_para_tv, "mbc_para.txt");
		if(g_vector_enable_flag&VECTOR_ENABLE_PPA)
			fprintf(g_fp_mbc_para_tv, "//STD=1  ENC=1\n");
		OPEN_VECTOR_FILE(VECTOR_ENABLE_PPA, g_fp_cbp_in_tv, "cbp_in.txt");
		
		OPEN_VECTOR_FILE(VECTOR_ENABLE_DCT, g_fp_idct_tv, "idct_out.txt");
		//OPEN_VECTOR_FILE(VECTOR_ENABLE_MBC, g_fp_mbc_tv, "mbc_mb_rec.txt");
		OPEN_VECTOR_FILE(VECTOR_ENABLE_MBC, g_fp_mbc_tv, "mbc_out.txt");
		OPEN_VECTOR_FILE(VECTOR_ENABLE_MBC, g_fp_mbc_ipred, "mbc_ipred.txt");

//		OPEN_VECTOR_FILE(VECTOR_ENABLE_MCA, g_fp_mcapara_tv, "mca_para.txt");
		OPEN_VECTOR_FILE(VECTOR_ENABLE_MCA, g_fp_rec_frm_tv, "rec_frame.txt");

		OPEN_VECTOR_FILE(VECTOR_ENABLE_MBC, g_fp_mbc_dct_out_tv, "mbc_dctout.txt");
		OPEN_VECTOR_FILE(VECTOR_ENABLE_MBC, g_fp_mbc_idct_in_tv, "mbc_idctin.txt");

		OPEN_VECTOR_FILE(VECTOR_ENABLE_DCT, g_fp_dct_tv, "dct_out.txt");
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

		OPEN_VECTOR_FILE(VECTOR_ENABLE_DCT, g_fp_idct_tv, "idct_in.txt");//jzy		
		OPEN_VECTOR_FILE(VECTOR_ENABLE_DCT, g_fp_isqt_tv, "idct_out.txt");
		
		OPEN_VECTOR_FILE(VECTOR_ENABLE_MBC, g_fp_mbc_tv, "mbc_out.txt");
		OPEN_VECTOR_FILE(VECTOR_ENABLE_VLD, g_fp_parser_tv, "ppa_in.txt");
		OPEN_VECTOR_FILE(VECTOR_ENABLE_DCT, g_fp_dctpara_tv, "dct_para.txt");
		//int vsp_standard=(g_glb_reg_ptr->VSP_CFG0 >> 8) & 0xf;
		fprintf(g_fp_dctpara_tv, "dct_mode=0 vsp_standard=4 scale_enable=0\n");//jzy		
		OPEN_VECTOR_FILE(VECTOR_ENABLE_MBC, g_fp_mbcpara_tv, "mbc_para.txt");
		fprintf(g_fp_mbcpara_tv, "STD=0100  ENC=0\n");//jzy
		OPEN_VECTOR_FILE(VECTOR_ENABLE_MCA, g_fp_mcapara_tv, "mca_para.txt");
		OPEN_VECTOR_FILE(VECTOR_ENABLE_DBK, g_fp_dbkpara_tv, "dbk_para.txt");

		OPEN_VECTOR_FILE(VECTOR_ENABLE_MBC, g_fp_vld_tv, "vld_out.txt");
		OPEN_VECTOR_FILE(VECTOR_ENABLE_VLD, g_fp_vld_cabac_tbl_data, "vld_cabac_table.dat");

		OPEN_VECTOR_FILE(VECTOR_ENABLE_MBC, g_fp_rec_frm_tv, "rec_frame.txt");
		OPEN_VECTOR_FILE(VECTOR_ENABLE_MBC, g_fp_rec_frmy_tv, "rec_frame_y.txt");//jzy
		OPEN_VECTOR_FILE(VECTOR_ENABLE_MBC, g_fp_rec_frmuv_tv, "rec_frame_uv.txt");//jzy
		OPEN_VECTOR_FILE(VECTOR_ENABLE_MBC, g_fp_global_tv, "vsp_glb_reg.txt");
		OPEN_VECTOR_FILE(VECTOR_ENABLE_MBC, g_fp_mbc_ipred, "mbc_ipred.txt");//jzy
		OPEN_VECTOR_FILE(VECTOR_ENABLE_MBC, g_fp_mbc_idctin, "mbc_idctin.txt");//jzy

		OPEN_VECTOR_FILE(VECTOR_ENABLE_MCA, g_fp_iquant_tv, "iquant.txt");//jzy
		OPEN_VECTOR_FILE(VECTOR_ENABLE_MCA, g_fp_hadarm_tv, "hadarm.txt");//jzy
		OPEN_VECTOR_FILE(VECTOR_ENABLE_MCA, g_fp_psi_tv, "ppa_slice_in.txt");//jzy
		OPEN_VECTOR_FILE(VECTOR_ENABLE_MCA, g_fp_iqw_tv, "iqw.txt");//jzy
		OPEN_VECTOR_FILE(VECTOR_ENABLE_MCA, g_fp_colin_tv, "colbuffer_in.txt");//jzy
		OPEN_VECTOR_FILE(VECTOR_ENABLE_MCA, g_fp_colout_tv, "colbuffer_out.txt");//jzy
		OPEN_VECTOR_FILE(VECTOR_ENABLE_MCA, g_fp_mcain_luma_tv, "mca_in_luma.txt");//jzy
		OPEN_VECTOR_FILE(VECTOR_ENABLE_MCA, g_fp_mcain_chroma_tv, "mca_in_chroma.txt");//jzy
		OPEN_VECTOR_FILE(TRACE_ENABLE_MCA, g_fp_trace_mcaflt, "trace_mcaflt.txt");
		OPEN_VECTOR_FILE(VECTOR_ENABLE_MCA, g_fp_wpin_tv, "wp_in.txt");//jzy
		OPEN_VECTOR_FILE(VECTOR_ENABLE_MCA, g_fp_wpwt_tv, "wp_weight.txt");//jzy
		//OPEN_VECTOR_FILE(VECTOR_ENABLE_MCA, g_fp_wpwt1_tv, "wp_weight1.txt");//jzy
		OPEN_VECTOR_FILE(VECTOR_ENABLE_MCA, g_fp_wprt_tv, "wp_out.txt");//jzy
		OPEN_VECTOR_FILE(VECTOR_ENABLE_VLD, g_fp_slicedata_offset, "bs_offset.txt");//weihu
		OPEN_VECTOR_FILE(VECTOR_ENABLE_VLD, g_fp_MBtype, "hvld_mb_type.txt");//weihu
		OPEN_VECTOR_FILE(VECTOR_ENABLE_VLD, g_fp_global, "ppa_cfg.txt");//weihu
		OPEN_VECTOR_FILE(VECTOR_ENABLE_VLD, g_fp_vld_global, "hvld_cfg.txt");//weihu
	#elif defined(REAL_DEC)
		char test_vect_file_path[200] = "../../test_vectors/";
	
		OPEN_VECTOR_FILE(VECTOR_ENABLE_DBK, g_fp_dbk_tv, "dbk_out.txt");
		OPEN_VECTOR_FILE(VECTOR_ENABLE_DBK, g_fp_dbk_mb_tv, "dbk_mb_out.txt");
//		OPEN_VECTOR_FILE(VECTOR_ENABLE_DBK, g_fp_dbk_frame_tv, "dbk_frame_out.txt");
//		OPEN_VECTOR_FILE(VECTOR_ENABLE_DBK, g_fp_before_flt_tv, "dbk_before_filter.txt");
 
 		OPEN_VECTOR_FILE(VECTOR_ENABLE_MCA, g_fp_mca_tv, "mca_out.txt");
		OPEN_VECTOR_FILE(VECTOR_ENABLE_DCT, g_fp_isqt_tv, "idct_out.txt");
		OPEN_VECTOR_FILE(VECTOR_ENABLE_DCT, g_fp_idct_tv, "idct_in.txt");//jzy
		
	//	OPEN_VECTOR_FILE(VECTOR_ENABLE_DCT, g_fp_isqt_tv, "iqt_out.txt");

		OPEN_VECTOR_FILE(VECTOR_ENABLE_MBC, g_fp_mbcpara_tv, "mbc_para.txt");
		OPEN_VECTOR_FILE(VECTOR_ENABLE_MBC, g_fp_mbc_tv, "mbc_out.txt");
		OPEN_VECTOR_FILE(VECTOR_ENABLE_MBC, g_fp_parser_tv, "ppa_in.txt");
		
		OPEN_VECTOR_FILE(VECTOR_ENABLE_MBC, g_fp_vld_tv, "vld_out.txt");
	
		OPEN_VECTOR_FILE(VECTOR_ENABLE_MBC, g_fp_rec_frm_tv, "rec_frame.txt");
		OPEN_VECTOR_FILE(VECTOR_ENABLE_MBC, g_fp_rec_frmy_tv, "rec_frame_y.txt");//jzy
		OPEN_VECTOR_FILE(VECTOR_ENABLE_MBC, g_fp_rec_frmuv_tv, "rec_frame_uv.txt");//jzy
		OPEN_VECTOR_FILE(VECTOR_ENABLE_MBC, g_fp_mbc_ipred, "mbc_ipred.txt");//jzy
		OPEN_VECTOR_FILE(VECTOR_ENABLE_MBC, g_fp_mbc_idctin, "mbc_idctin.txt");//jzy

		OPEN_VECTOR_FILE(VECTOR_ENABLE_MCA, g_fp_iquant_tv, "iquant.txt");//jzy
		OPEN_VECTOR_FILE(VECTOR_ENABLE_MCA, g_fp_hadarm_tv, "hadarm.txt");//jzy
		OPEN_VECTOR_FILE(VECTOR_ENABLE_MCA, g_fp_psi_tv, "ppa_slice_in.txt");//jzy
		OPEN_VECTOR_FILE(VECTOR_ENABLE_MCA, g_fp_iqw_tv, "iqw.txt");//jzy
		OPEN_VECTOR_FILE(VECTOR_ENABLE_MCA, g_fp_colin_tv, "colbuffer_in.txt");//jzy
		OPEN_VECTOR_FILE(VECTOR_ENABLE_MCA, g_fp_colout_tv, "colbuffer_out.txt");//jzy
		OPEN_VECTOR_FILE(VECTOR_ENABLE_MCA, g_fp_mcain_luma_tv, "mca_in_luma.txt");//jzy
		OPEN_VECTOR_FILE(VECTOR_ENABLE_MCA, g_fp_mcain_chroma_tv, "mca_in_chroma.txt");//jzy
		OPEN_VECTOR_FILE(TRACE_ENABLE_MCA, g_fp_trace_mcaflt, "trace_mcaflt.txt");
		OPEN_VECTOR_FILE(VECTOR_ENABLE_MCA, g_fp_wpin_tv, "wp_in.txt");//jzy
		OPEN_VECTOR_FILE(VECTOR_ENABLE_MCA, g_fp_wpwt_tv, "wp_weight.txt");//jzy
		//OPEN_VECTOR_FILE(VECTOR_ENABLE_MCA, g_fp_wpwt1_tv, "wp_weight1.txt");//jzy
		OPEN_VECTOR_FILE(VECTOR_ENABLE_MCA, g_fp_wprt_tv, "wp_out.txt");//jzy
     #elif defined(VP8_DEC)
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
	
    #else
		assert(0);
	#endif

    #if defined(MCcache)//weihu
        OPEN_VECTOR_FILE(VECTOR_ENABLE_CACHE, g_fp_mc_cache, "cache_hit.txt");
    #endif
	OPEN_VECTOR_FILE(VECTOR_ENABLE_FW, g_fw_tv_cmd, "sw_fw_tv_cmd.dat");	
	OPEN_VECTOR_FILE(VECTOR_ENABLE_FW, g_fw_tv_cmd_info, "cmd_info.dat"); 
 	OPEN_VECTOR_FILE(VECTOR_ENABLE_FW, g_fw_tv_cmd_data, "cmd_data.dat");
 	OPEN_VECTOR_FILE(VECTOR_ENABLE_FW, g_fw_tv_cmd_data_notes, "sw_cmd_data.dat");
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
		CLOSE_VECTOR_FILE(VECTOR_ENABLE_BSM, g_fp_bsm_total_bits_tv);
		
		CLOSE_VECTOR_FILE(VECTOR_ENABLE_MEA, g_fp_mea_tv);
	
//		CLOSE_VECTOR_FILE(VECTOR_ENABLE_MEA, g_fp_mea_src_frm_y_tv);
//		CLOSE_VECTOR_FILE(VECTOR_ENABLE_MEA, g_fp_mea_src_frm_u_tv);
//		CLOSE_VECTOR_FILE(VECTOR_ENABLE_MEA, g_fp_mea_src_frm_v_tv);
#ifndef _LIB
		fprintf(g_fp_global_tv, "f_01234567_89abcdef\n");
#endif
		CLOSE_VECTOR_FILE(VECTOR_ENABLE_FW, g_fp_global_tv);
		CLOSE_VECTOR_FILE(VECTOR_ENABLE_MEA, g_fp_mea_src_mb_tv);
		CLOSE_VECTOR_FILE(VECTOR_ENABLE_MEA, g_fp_mea_mb_ime_tv);
		CLOSE_VECTOR_FILE(VECTOR_ENABLE_MEA, g_fp_mea_mca_mb_tv);
		CLOSE_VECTOR_FILE(VECTOR_ENABLE_MEA, g_fp_mea_mb_tv);
		CLOSE_VECTOR_FILE(VECTOR_ENABLE_MEA, g_fp_mea_src_frm_y_tv);
		CLOSE_VECTOR_FILE(VECTOR_ENABLE_MEA, g_fp_mea_src_frm_u_tv);
		CLOSE_VECTOR_FILE(VECTOR_ENABLE_MEA, g_fp_ime_mb_sad_tv);
		CLOSE_VECTOR_FILE(VECTOR_ENABLE_MEA, g_fp_fme_mb_sad_tv);
		CLOSE_VECTOR_FILE(VECTOR_ENABLE_MEA, g_fp_ime_best_sad_tv);
		CLOSE_VECTOR_FILE(VECTOR_ENABLE_MEA, g_fp_fme_best_sad_tv);
		CLOSE_VECTOR_FILE(VECTOR_ENABLE_MEA, g_fp_mea_ppa_buf_tv);
		CLOSE_VECTOR_FILE(VECTOR_ENABLE_MEA, g_fp_mea_cfg_tv);
		CLOSE_VECTOR_FILE(VECTOR_ENABLE_DCT, g_fp_frame_cfg);
		CLOSE_VECTOR_FILE(VECTOR_ENABLE_DCT, g_fp_mvlc_cfg);
		//iea
		CLOSE_VECTOR_FILE(VECTOR_ENABLE_IEA, g_fp_iea_src_mb_tv);
		CLOSE_VECTOR_FILE(VECTOR_ENABLE_IEA, g_fp_iea_in_tv);
//		CLOSE_VECTOR_FILE(VECTOR_ENABLE_IEA, g_fp_iea_out_tv);

		
#ifndef VLC_TV_SPLIT
		CLOSE_VECTOR_FILE(VECTOR_ENABLE_PPA, g_fp_vlc_para_tv);
		CLOSE_VECTOR_FILE(VECTOR_ENABLE_PPA, g_fp_dct_para_tv);
		CLOSE_VECTOR_FILE(VECTOR_ENABLE_DCT, g_fp_dct_out_tv);
		CLOSE_VECTOR_FILE(VECTOR_ENABLE_VLC, g_fp_vlc_tv);
#endif
		CLOSE_VECTOR_FILE(VECTOR_ENABLE_DCT, g_fp_huff_tab_tv);
		//ppa
		CLOSE_VECTOR_FILE(VECTOR_ENABLE_PPA, g_fp_ppa_cfg_tv);
		CLOSE_VECTOR_FILE(VECTOR_ENABLE_PPA, g_fp_ppa_in_tv);
		CLOSE_VECTOR_FILE(VECTOR_ENABLE_PPA, g_fp_mbc_para_tv);
		CLOSE_VECTOR_FILE(VECTOR_ENABLE_PPA, g_fp_cbp_in_tv);

		CLOSE_VECTOR_FILE(VECTOR_ENABLE_DCT, g_fp_idct_tv);
		CLOSE_VECTOR_FILE(VECTOR_ENABLE_MBC, g_fp_mbc_tv);
		CLOSE_VECTOR_FILE(VECTOR_ENABLE_MBC, g_fp_mbc_ipred);

//		CLOSE_VECTOR_FILE(VECTOR_ENABLE_MCA, g_fp_mcapara_tv);
		CLOSE_VECTOR_FILE(VECTOR_ENABLE_MCA, g_fp_rec_frm_tv);

		CLOSE_VECTOR_FILE(VECTOR_ENABLE_MBC, g_fp_mbc_dct_out_tv);
		CLOSE_VECTOR_FILE(VECTOR_ENABLE_MBC, g_fp_mbc_idct_in_tv);

		CLOSE_VECTOR_FILE(VECTOR_ENABLE_DCT, g_fp_dct_tv);
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
		CLOSE_VECTOR_FILE(VECTOR_ENABLE_DCT, g_fp_idct_tv);//weihu
		
		CLOSE_VECTOR_FILE(VECTOR_ENABLE_MBC, g_fp_mbc_tv);
		CLOSE_VECTOR_FILE(VECTOR_ENABLE_MBC, g_fp_parser_tv);
		CLOSE_VECTOR_FILE(VECTOR_ENABLE_DCT, g_fp_dctpara_tv);
        CLOSE_VECTOR_FILE(VECTOR_ENABLE_DBK, g_fp_dbkpara_tv);
		CLOSE_VECTOR_FILE(VECTOR_ENABLE_MBC, g_fp_mbcpara_tv);
		CLOSE_VECTOR_FILE(VECTOR_ENABLE_MCA, g_fp_mcapara_tv);
		CLOSE_VECTOR_FILE(VECTOR_ENABLE_VLD, g_fp_vld_cabac_tbl_data);//weihu
		CLOSE_VECTOR_FILE(VECTOR_ENABLE_MBC, g_fp_vld_tv);
		CLOSE_VECTOR_FILE(VECTOR_ENABLE_MBC, g_fp_mbc_ipred);//jzy
		CLOSE_VECTOR_FILE(VECTOR_ENABLE_MBC, g_fp_mbc_idctin);//jzy
		CLOSE_VECTOR_FILE(VECTOR_ENABLE_MBC, g_fp_rec_frmy_tv);//jzy
		CLOSE_VECTOR_FILE(VECTOR_ENABLE_MBC, g_fp_rec_frmuv_tv);//jzy

		CLOSE_VECTOR_FILE(VECTOR_ENABLE_MCA, g_fp_iquant_tv);//jzy
		CLOSE_VECTOR_FILE(VECTOR_ENABLE_MCA, g_fp_hadarm_tv);//jzy
		CLOSE_VECTOR_FILE(VECTOR_ENABLE_MCA, g_fp_psi_tv);//jzy
		CLOSE_VECTOR_FILE(VECTOR_ENABLE_MCA, g_fp_iqw_tv);//jzy
		CLOSE_VECTOR_FILE(VECTOR_ENABLE_MCA, g_fp_colin_tv);//jzy
		CLOSE_VECTOR_FILE(VECTOR_ENABLE_MCA, g_fp_colout_tv);//jzy
		CLOSE_VECTOR_FILE(VECTOR_ENABLE_MCA, g_fp_mcain_luma_tv);//jzy
		CLOSE_VECTOR_FILE(VECTOR_ENABLE_MCA, g_fp_mcain_chroma_tv);//jzy
		CLOSE_VECTOR_FILE(TRACE_ENABLE_MCA, g_fp_trace_mcaflt);
		CLOSE_VECTOR_FILE(VECTOR_ENABLE_MCA, g_fp_wpin_tv);//jzy
		CLOSE_VECTOR_FILE(VECTOR_ENABLE_MCA, g_fp_wpwt_tv);//jzy
//		CLOSE_VECTOR_FILE(VECTOR_ENABLE_MCA, g_fp_wpwt1_tv);//jzy
		CLOSE_VECTOR_FILE(VECTOR_ENABLE_MCA, g_fp_wprt_tv);//jzy
		CLOSE_VECTOR_FILE(VECTOR_ENABLE_VLD, g_fp_slicedata_offset);//weihu
		CLOSE_VECTOR_FILE(VECTOR_ENABLE_VLD, g_fp_MBtype);//weihu
		CLOSE_VECTOR_FILE(VECTOR_ENABLE_VLD, g_fp_global);//weihu
		CLOSE_VECTOR_FILE(VECTOR_ENABLE_VLD, g_fp_vld_global);//weihu
	#elif defined(REAL_DEC)
		CLOSE_VECTOR_FILE(VECTOR_ENABLE_DBK, g_fp_dbk_tv);
		CLOSE_VECTOR_FILE(VECTOR_ENABLE_DBK, g_fp_dbk_mb_tv);
//		CLOSE_VECTOR_FILE(VECTOR_ENABLE_DBK, g_fp_dbk_frame_tv);
//		CLOSE_VECTOR_FILE(VECTOR_ENABLE_DBK, g_fp_before_flt_tv);

 		CLOSE_VECTOR_FILE(VECTOR_ENABLE_MCA, g_fp_mca_tv);
		
		CLOSE_VECTOR_FILE(VECTOR_ENABLE_MBC, g_fp_mbc_tv);

 		CLOSE_VECTOR_FILE(VECTOR_ENABLE_MBC, g_fp_vld_tv);
		
		CLOSE_VECTOR_FILE(VECTOR_ENABLE_DCT, g_fp_isqt_tv);
		CLOSE_VECTOR_FILE(VECTOR_ENABLE_MBC, g_fp_idct_tv);
		
		CLOSE_VECTOR_FILE(VECTOR_ENABLE_MBC, g_fp_mbcpara_tv);
		CLOSE_VECTOR_FILE(VECTOR_ENABLE_MBC, g_fp_rec_frm_tv);
		CLOSE_VECTOR_FILE(VECTOR_ENABLE_MBC, g_fp_rec_frmy_tv);//jzy
		CLOSE_VECTOR_FILE(VECTOR_ENABLE_MBC, g_fp_rec_frmuv_tv);//jzy
		CLOSE_VECTOR_FILE(VECTOR_ENABLE_MBC, g_fp_mbc_ipred);//jzy
		CLOSE_VECTOR_FILE(VECTOR_ENABLE_MBC, g_fp_mbc_idctin);//jzy

		CLOSE_VECTOR_FILE(VECTOR_ENABLE_MCA, g_fp_iquant_tv);//jzy
		CLOSE_VECTOR_FILE(VECTOR_ENABLE_MCA, g_fp_hadarm_tv);//jzy
		CLOSE_VECTOR_FILE(VECTOR_ENABLE_MCA, g_fp_psi_tv);//jzy
		CLOSE_VECTOR_FILE(VECTOR_ENABLE_MCA, g_fp_iqw_tv);//jzy
		CLOSE_VECTOR_FILE(VECTOR_ENABLE_MCA, g_fp_colin_tv);//jzy
		CLOSE_VECTOR_FILE(VECTOR_ENABLE_MCA, g_fp_colout_tv);//jzy
		CLOSE_VECTOR_FILE(VECTOR_ENABLE_MCA, g_fp_mcain_luma_tv);//jzy
		CLOSE_VECTOR_FILE(VECTOR_ENABLE_MCA, g_fp_mcain_chroma_tv);//jzy
		CLOSE_VECTOR_FILE(TRACE_ENABLE_MCA, g_fp_trace_mcaflt);
		CLOSE_VECTOR_FILE(VECTOR_ENABLE_MCA, g_fp_wpin_tv);//jzy
		CLOSE_VECTOR_FILE(VECTOR_ENABLE_MCA, g_fp_wpwt_tv);//jzy
//		CLOSE_VECTOR_FILE(VECTOR_ENABLE_MCA, g_fp_wpwt1_tv);//jzy
		CLOSE_VECTOR_FILE(VECTOR_ENABLE_MCA, g_fp_wprt_tv);//jzy
    #elif defined(VP8_DEC)
		CLOSE_VECTOR_FILE(VECTOR_ENABLE_DBK, g_fp_dbk_tv);
		CLOSE_VECTOR_FILE(VECTOR_ENABLE_DBK, g_fp_dbk_mb_tv);
		CLOSE_VECTOR_FILE(VECTOR_ENABLE_DBK, g_fp_dbk_frame_tv);
		CLOSE_VECTOR_FILE(VECTOR_ENABLE_DBK, g_fp_before_flt_tv);
		
		CLOSE_VECTOR_FILE(VECTOR_ENABLE_MCA, g_fp_mca_tv);
		
		CLOSE_VECTOR_FILE(VECTOR_ENABLE_DCT, g_fp_isqt_tv);
		
		CLOSE_VECTOR_FILE(VECTOR_ENABLE_MBC, g_fp_mbc_tv);
       
    #else
		assert(0);
	#endif
    
    #if defined(MCcache)//weihu
         CLOSE_VECTOR_FILE(VECTOR_ENABLE_CACHE, g_fp_mc_cache);
    #endif
	CLOSE_VECTOR_FILE(VECTOR_ENABLE_FW, g_fw_tv_cmd);	
	CLOSE_VECTOR_FILE(VECTOR_ENABLE_FW, g_fw_tv_cmd_info);
	CLOSE_VECTOR_FILE(VECTOR_ENABLE_FW, g_fw_tv_cmd_data); 
 	CLOSE_VECTOR_FILE(VECTOR_ENABLE_FW, g_fw_tv_cmd_data_notes);
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
	//	OPEN_TRACE_FILE(TRACE_ENABLE_FW, g_fp_slicedata_offset, "bs_offset.txt");//weihu
	//	OPEN_TRACE_FILE(TRACE_ENABLE_FW, g_fp_MBtype, "hvld_mb_type.txt");//weihu
	//	OPEN_TRACE_FILE(TRACE_ENABLE_FW, g_fp_global, "ppa_cfg.txt");//weihu
		
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
	//	CLOSE_TRACE_FILE(TRACE_ENABLE_FW, g_fp_slicedata_offset, "trace_slicedata_off.txt");//weihu
	//	CLOSE_TRACE_FILE(TRACE_ENABLE_FW, g_fp_MBtype, "trace_mb_type.txt");//weihu
	//	CLOSE_TRACE_FILE(TRACE_ENABLE_FW, g_fp_global, "trace_globalreg.txt");//weihu
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














































