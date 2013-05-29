/*tv_mea.h*/
#ifndef _TV_MEA_H
#define _TV_MEA_H
#include <stdio.h>
#include "hmea_mode.h"

//extern FILE	*	g_mea_tv_cmd_fp;
//extern FILE	*	g_mv_pred_fp;
//extern FILE	*	g_pe_result_fp;
//extern FILE	*	g_blk_result_fp;
//extern FILE	*	g_imp_dcs_fp;
//extern FILE	*	g_mb_res_fp;
//extern FILE	*	g_src_frm_fp;
//extern FILE	*	g_srch_wind_fp;

// MBC
extern FILE	*	g_src_mb_fp;
extern FILE	*	g_buf_params_fp;
extern FILE	*	g_mb_pred_fp;
extern FILE	*	g_mb_dct_res_fp;
extern FILE	*	g_mb_idct_res_fp;
extern FILE	*	g_idct_out_fp;
extern FILE	*	g_mb_rec_fp;
extern FILE	*	g_mbc_out_fp;

// DBK
extern FILE *	g_dbk_out_fp;

// MEA
extern FILE	*	g_fm_src_y_fp;
extern FILE	*	g_fm_src_uv_fp;
extern FILE	*	g_fm_ref_y_fp;
extern FILE	*	g_fm_ref_uv_fp;
extern FILE	*	g_ime_mb_sad_fp;
extern FILE	*	g_fme_mb_sad_fp;
extern FILE	*	g_mb_ime_fp;
extern FILE	*	g_src_mb_mea_fp;
extern FILE	*	g_mea_mca_mb_fp;
extern FILE	*	g_mea_ppa_buf_fp;
extern FILE	*	g_ime_best_sad_fp;
extern FILE	*	g_fme_best_sad_fp;
extern FILE	*	g_mea_config_fp;

// IEA
extern FILE	*	g_mb_iea_in_fp;
extern FILE	*	g_mb_iea_out_fp;
extern FILE	*	g_src_mb_iea_fp;

// PPA
extern FILE	*	g_vlc_para_fp;
extern FILE	*	g_mca_para_fp;
extern FILE	*	g_ppal_buf_fp;
extern FILE	*	g_dct_para_fp;
extern FILE	*	g_dct_cbp_fp;
extern FILE *	g_dbk_para_fp;
extern FILE	*	g_ppa_config_fp;

// VLC
extern FILE *	g_bsm_out_fp;
extern FILE *	g_bsm_totalbits_fp;
extern FILE	*	g_dct_out_fp;
extern FILE	*	g_vlc_nC_fp;
extern FILE	*	g_vlc_offset_fp;
extern FILE	*	g_vlc_golomb_fp;
extern FILE	*	g_vlc_trace_level_fp;
extern FILE	*	g_vlc_trace_run_fp;
extern FILE	*	g_vlc_trace_token_fp;
extern FILE	*	g_vlc_table_coeff_fp;
extern FILE	*	g_vlc_table_zero_fp;
extern FILE	*	g_vlc_table_run_fp;

// DCT
extern FILE *	g_dct_in_fp;
extern FILE	*	g_idct_in_fp;

// GLB REG
extern FILE	*	g_vsp_glb_reg_fp;

typedef struct 
{
	uint32		sad_y_4x4[16][3];	// 0:dc 1:h 2:v
	uint32		sad_y_4x4_min;	// 0:dc 1:h 2:v
	uint32		sad_y_8x8[4][3];
	uint32		sad_y_16x16[3];		// 0:dc 1:h 2:v
	uint32		sad_c_8x8[3];	// 0:cb 1:cr
	uint32		iea_ipred_type;		// 0:4x4 1:16x16
	uint32		dc_pred_16;		// [7:0]
} IEA_SAD;

typedef struct 
{
	uint16		total_cost_min;	// total_cost_min[9:0]
	uint8		mv_valid;	// mv_valid[7:0]
	uint8		partition_mode;
	int8		imv_y_blk0;	// imv_y_blk0[5:0]
	int8		imv_x_blk0;	// imv_x_blk0[6:0]
	int8		imv_y_blk1;	// imv_y_blk1[5:0]
	int8		imv_x_blk1;	// imv_x_blk1[6:0]
	int8		imv_y_blk2;	// imv_y_blk2[5:0]
	int8		imv_x_blk2;	// imv_x_blk2[6:0]
	int8		imv_y_blk3;	// imv_y_blk3[5:0]
	int8		imv_x_blk3;	// imv_x_blk3[6:0]
} MEA_INFO;

void TVMeaInit ();
#include "h264enc_mode.h"
void VLCSplitInit(ENC_IMAGE_PARAMS_T *img_ptr);
void VLCSplitDeinit(ENC_IMAGE_PARAMS_T *img_ptr);
//void PrintfSrcFrame (uint32 * frm_y_ptr, uint32 * frm_uv_ptr, int width, int height);
//void PrintfSrchWind (uint32 * srch_wind_ptr);
//void PrintfMvPred (MOTION_VECTOR_T * mv_pred_ptr);
//void PrintfPeResult (int is_sp, int cost_pe0, int cost_pe1, int cost_pe2, int cost_pe3);
//void PrintfBlkResult (int blk_cost, MOTION_VECTOR_T * mv);
//void PrintfImpDcs (int dc_cost, int hor_cost, int ver_cost, int left_avail, int top_avail);
//void PrintfMBResult (MODE_DECISION_T * mode_dcs_ptr);
//void PrintfCmd (int cmd_type, int addr, int val, int mask);

// MBC
void PrintfSrcMB (uint32 * src_mb_ptf, ENC_MB_MODE_T *mb_info_ptr);
void PrintfMBCBufParam(ENC_IMAGE_PARAMS_T *img_ptr, ENC_MB_MODE_T *mb_info_ptr, ENC_MB_CACHE_T *mb_cache_ptr);
void PrintfPred(ENC_MB_MODE_T *mb_info_ptr);
void PrintfDCTIn(ENC_MB_MODE_T *mb_info_ptr);
void PrintfIDCTOut(ENC_MB_MODE_T *mb_info_ptr);
void PrintfRecPreFilter();

// DBK
void Print_DBK_Out();
void Print_DBK_Frame(ENC_IMAGE_PARAMS_T *img_ptr);

// MEA
void Print_sad_mv_cost(uint32 sad, uint32 mvd_cost, uint8* comment, FILE *fp);
void Print_best_sad_mv_cost(uint32 sad, uint8* comment, FILE *fp);
void Print_IME_Src_MB(MEA_INFO *mea_info);
void Print_IME_block(uint32 iframe);
void Print_SrcRef_Frame(ENC_IMAGE_PARAMS_T *img_ptr);
void Print_MEA_CFG(ENC_IMAGE_PARAMS_T *img_ptr, uint32 qp);

// IEA
void Print_IEA_Mode(ENC_IMAGE_PARAMS_T *img_ptr, ENC_MB_MODE_T *mb_info_ptr, ENC_MB_CACHE_T *mb_cache_ptr);
void Print_IEA_SAD(IEA_SAD *iea_sad);

// PPA
void PrintfVLCParaBuf(ENC_IMAGE_PARAMS_T *img_ptr, ENC_MB_MODE_T *mb_info_ptr, ENC_MB_CACHE_T *mb_cache_ptr);
void PrintfPPALineBuf(ENC_IMAGE_PARAMS_T *img_ptr, ENC_MB_MODE_T *mb_info_ptr, ENC_MB_CACHE_T *mb_cache_ptr);
void PrintfDCTParaBuf(ENC_IMAGE_PARAMS_T *img_ptr, ENC_MB_MODE_T *mb_info_ptr, ENC_MB_CACHE_T *mb_cache_ptr, short dct_buf[]);
void PrintfDBKParaBuf(ENC_IMAGE_PARAMS_T *img_ptr, int *slice_info);
void Print_PPA_CFG(ENC_IMAGE_PARAMS_T *img_ptr, uint32 qp);

// DCT
void PrintfIDCTIn(short dct_buf[]);

// VLC
void PrintfDCTOut(ENC_MB_MODE_T *mb_info_ptr, short dct_buf[]);
void PrintBSMOut(ENC_IMAGE_PARAMS_T *img_ptr);
void PrintBSMLineNum();
void PrintfnCValue(ENC_MB_CACHE_T *mb_cache_ptr);
void PrintfCavlcOffset(ENC_MB_MODE_T *mb_info_ptr, uint8* comment);
void PrintfGolombSyntax();
void PrintfVLCTable();
#endif
