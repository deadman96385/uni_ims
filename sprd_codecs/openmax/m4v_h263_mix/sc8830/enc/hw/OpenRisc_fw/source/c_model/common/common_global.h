/******************************************************************************
 ** File Name:      common_global.h	                                          *
 ** Author:         Xiaowei Luo                                               *
 ** DATE:           11/20/2007                                                *
 ** Copyright:      2007 Spreatrum, Incoporated. All Rights Reserved.         *
 ** Description:    VSP bsm Driver for video codec.	  						  *
 *****************************************************************************/
/******************************************************************************
 **                   Edit    History                                         *
 **---------------------------------------------------------------------------* 
 ** DATE          NAME            DESCRIPTION                                 * 
 ** 11/20/2007    Xiaowei Luo     Create.                                     *
 *****************************************************************************/
#ifndef _COMMON_GLOBAL_H_
#define _COMMON_GLOBAL_H_

/*----------------------------------------------------------------------------*
**                        Dependencies                                        *
**---------------------------------------------------------------------------*/
#include "video_common.h"
#include "vsp_drv_sc8800g.h"
/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    extern   "C" 
    {
#endif

extern int g_tv_cmd_num;
extern int g_tv_cmd_data_num;

extern VSP_DCAM_REG_T * g_dcam_reg_ptr;
extern VSP_GLB_REG_T  * g_glb_reg_ptr;
extern VSP_AHBM_REG_T * g_ahbm_reg_ptr;
extern VSP_VLD_REG_T  * g_vld_reg_ptr;
extern VSP_HVLD_REG_T * g_hvld_reg_ptr;
extern VSP_RVLD_REG_T * g_rvld_reg_ptr;
extern VSP_BSM_REG_T  * g_bsm_reg_ptr;
extern VSP_MBC_REG_T  * g_mbc_reg_ptr;
extern VSP_DBK_REG_T  * g_dbk_reg_ptr;
extern VSP_MEA_REG_T  * g_mea_reg_ptr;
extern VSP_DCT_REG_T  * g_dct_reg_ptr;
extern VSP_MCA_REG_T  * g_mca_reg_ptr;
extern VSP_VLC_REG_T  * g_vlc_reg_ptr;
#ifdef PARSER_CMODEL
extern VSP_PPA_BUF_T  * g_ppa_buf_ptr;
extern VSP_PAR_REG_T  * g_par_reg_ptr;
#endif

extern uint8 g_block_num_in_one_mcu;

extern int	g_stream_type;
//////////////////////////////////////////////////////////////////////////
//  TestVector FILES
//////////////////////////////////////////////////////////////////////////
#ifdef SIM_IN_WIN
extern FILE *g_fp_vld_cabac_tbl_data;
extern FILE *g_fw_tv_cmd_data_notes;
extern FILE *g_fw_tv_cmd_info;
extern FILE *g_fw_tv_cmd_data;
extern FILE *g_fw_tv_cmd;
extern FILE *g_fp_vld_tv_run;
extern FILE *g_fp_vld_tv_lev;
extern FILE *g_fp_dbk_tv;
extern FILE *g_fp_before_flt_tv;
extern FILE *g_fp_dbk_mb_tv;
extern FILE *g_fp_dbk_frame_tv;
extern FILE *g_fp_mca_tv;
extern FILE *g_fp_vld_coeffPos;
extern FILE *g_fp_dct_tv;
extern FILE *g_fp_vlc_tv;
extern FILE *g_fp_mea_tv;
extern FILE *g_fp_mea_src_mb_tv;
extern FILE *g_fp_mea_prefilter_mb_tv;
extern FILE *g_fp_mea_src_frm_y_tv;
extern FILE *g_fp_mea_src_frm_u_tv;
extern FILE *g_fp_mea_src_frm_v_tv;
extern FILE *g_fp_global_tv;//james
extern FILE *g_fp_mea_mb_ime_tv;//james
extern FILE *g_fp_mea_mca_mb_tv;//james
extern FILE *g_fp_mea_mb_tv;//james
extern FILE *g_fp_mea_ref_frm_y_tv;//james
extern FILE *g_fp_mea_ref_frm_u_tv;//james
extern FILE *g_fp_mea_ppa_buf_tv;//james
extern FILE *g_fp_ime_mb_sad_tv;//james
extern FILE *g_fp_fme_mb_sad_tv;//james
extern FILE *g_fp_ime_best_sad_tv;//james
extern FILE *g_fp_fme_best_sad_tv;//james
extern FILE *g_fp_mea_cfg_tv;//james
extern FILE *g_fp_frame_cfg;//james
extern FILE *g_fp_mvlc_cfg;//james

//iea
extern FILE *g_fp_iea_src_mb_tv;//james
extern FILE *g_fp_iea_in_tv;//james
//extern FILE *g_fp_iea_out_tv;//james
//ppa
extern FILE *g_fp_vlc_para_tv;//james
extern FILE *g_fp_dct_para_tv;//james
extern FILE *g_fp_ppa_cfg_tv;//james
extern FILE *g_fp_ppa_in_tv;//james
extern FILE *g_fp_mbc_para_tv;//james
extern FILE *g_fp_cbp_in_tv;//james

//dct
extern FILE *g_fp_huff_tab_tv;
extern FILE *g_fp_dct_out_tv;
//mbc
extern FILE *g_fp_mbc_dct_out_tv;
extern FILE *g_fp_mbc_idct_in_tv;

extern FILE *g_fp_mbc_tv;
extern FILE *g_fp_bsm_tv;
extern FILE *g_fp_bsm_total_bits_tv;
extern FILE *g_fp_isqt_tv;
extern FILE *g_fp_ipred_tv;
extern FILE *g_fp_rec_frm_tv;
extern FILE *g_fp_vld_tv;
extern FILE *g_fp_parser_tv;//weihu
extern FILE *g_fp_idct_tv;
extern FILE *g_fp_rec_frmy_tv;//jzy
extern FILE *g_fp_rec_frmuv_tv;//jzy
extern FILE *g_fp_global_tv;//weihu
extern FILE *g_fp_dctpara_tv;
extern FILE *g_fp_mbcpara_tv;
extern FILE *g_fp_dbkpara_tv;
extern FILE *g_fp_mcapara_tv;
extern FILE *g_fp_iquant_tv;//jzy
extern FILE *g_fp_hadarm_tv;//jzy
extern FILE *g_fp_psi_tv;//jzy
extern FILE *g_fp_iqw_tv;//jzy
extern FILE *g_fp_colin_tv;//jzy
extern FILE *g_fp_colout_tv;//jzy
extern FILE *g_fp_mcain_luma_tv;//jzy
extern FILE *g_fp_mcain_chroma_tv;//jzy
extern FILE *g_fp_wpin_tv;//jzy
extern FILE *g_fp_wpwt_tv;//jzy,weight
//extern FILE *g_fp_wpwt1_tv;//jzy,shift
extern FILE *g_fp_wprt_tv;//jzy,result
extern FILE *g_fp_mbc_ipred;//jzy
extern FILE *g_fp_mbc_idctin;	
//////////////////////////////////////////////////////////////////////////
//  TRACE FILES
//////////////////////////////////////////////////////////////////////////
extern FILE *g_fp_trace_isqt;
extern FILE *g_fp_trace_ipred;
extern FILE *g_fp_trace_dbk;
extern FILE *g_fp_trace_mbc;
extern FILE *g_ipred_log_fp;
extern FILE *g_fp_trace_mca;
extern FILE *g_fp_trace_mea;
extern FILE *g_fp_trace_mea_src_mb;
extern FILE *g_fp_trace_mea_ref_mb;
extern FILE *g_fp_trace_mea_sad;
extern FILE *g_fp_trace_vlc;
extern FILE *g_fp_trace_vld;
extern FILE *g_fp_trace_fw;
extern FILE *g_fp_trace_fw_bs;
extern FILE *g_fp_dbp;
extern FILE *g_pf_before_iqidct;
extern FILE *g_pf_after_iq;
extern FILE *g_pf_after_iqidct;
extern FILE *pFmoFile;
extern FILE *g_fp_trace_mb_info;
extern FILE *g_fp_slicedata_offset;//weihu
extern FILE *g_fp_MBtype;//weihu
extern FILE *g_fp_global;//weihu
extern FILE *g_fp_vld_global;//weihu
extern FILE *g_fp_trace_mcaflt;

// #define VLD_FPRINTF		//fprintf
#define VLC_FPRINTF		fprintf
//#define DBK_FPRINTF		fprintf
#define IPRD_FPRINTF	fprintf
#define MCA_FPRINTF		fprintf
#define PARSER_FPRINTF  fprintf
#endif
#define SAFE_FREE(bfr) {\
	assert(bfr != PNULL);\
	free(bfr);\
	bfr = PNULL;\
}

void * safe_malloc(uint32 mem_size);

#define PRINTF_TV_NOTES	 fprintf

#define OUTPUT_FW_CMD_VECTOR(CmdType, RegAddr, value, pstring)	\
{\
	if(g_vector_enable_flag&VECTOR_ENABLE_FW)\
	{\
		fprintf(g_fw_tv_cmd,"%x,%08x,%08x \t  ",CmdType, RegAddr, value);\
		PRINTF_TV_NOTES(g_fw_tv_cmd,"//%s", pstring);\
		fprintf(g_fw_tv_cmd,"\n");\
		FFLUSH(g_fw_tv_cmd);\
	}\
	g_tv_cmd_num++;\
}
/**----------------------------------------------------------------------------*
**                           Function Prototype                               **
**----------------------------------------------------------------------------*/
PUBLIC void VSP_Init_CModel (void);
PUBLIC void VSP_Delete_CModel(void);
PUBLIC void AllocMCUBuf(void);
#ifdef SIM_IN_WIN
void fprintf_oneWord_hex (FILE * pf_txt, int val);
void FormatPrintHexNum(int num, FILE *fp);
void FomatPrint4Pix(uint8 *pLine, FILE *fp);
#endif
int InitVectorFiles();
int DenitVectorFiles(void);
int InitTraceFiles();
int DenitTraceFiles();

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End 
#endif  //_COMMON_GLOBAL_H_
