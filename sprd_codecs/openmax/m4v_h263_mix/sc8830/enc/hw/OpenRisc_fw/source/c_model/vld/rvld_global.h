/*rvld_global.h*/
#ifndef _RVLD_GLOBAL_H_
#define _RVLD_GLOBAL_H_

#include "sc8810_video_header.h"
//#include "rvdec_global.h"


//extern int g_intra_qp_to_idx [30+1];
//extern int g_inter_qp_to_idx [30+1];

extern uint32 intra_cbp_len [MAX_INTRA_QP_REGIONS][2][(MAX_CBP+7)/8];
extern uint32 intra_8x8_dsc_len [MAX_INTRA_QP_REGIONS][2][4][(MAX_8x8_DSC+7)/8];
extern uint32 intra_luma_4x4_dsc_len [MAX_INTRA_QP_REGIONS][3][(MAX_4x4_DSC+7)/8];
extern uint32 intra_luma_2x2_dsc_len [MAX_INTRA_QP_REGIONS][2][(MAX_2x2_DSC+7)/8];
extern uint32 intra_chroma_4x4_dsc_len [MAX_INTRA_QP_REGIONS][(MAX_4x4_DSC+7)/8];
extern uint32 intra_chroma_2x2_dsc_len [MAX_INTRA_QP_REGIONS][2][(MAX_2x2_DSC+7)/8];
extern uint32 intra_level_dsc_len [MAX_INTRA_QP_REGIONS][(MAX_LEVEL_DSC+7)/8];
extern uint32 inter_cbp_len [MAX_INTER_QP_REGIONS][(MAX_CBP+7)/8];
extern uint32 inter_8x8_dsc_len [MAX_INTER_QP_REGIONS][4][(MAX_8x8_DSC+7)/8];
extern uint32 inter_luma_4x4_dsc_len [MAX_INTER_QP_REGIONS][(MAX_4x4_DSC+7)/8];
extern uint32 inter_luma_2x2_dsc_len [MAX_INTER_QP_REGIONS][2][(MAX_2x2_DSC+7)/8];
extern uint32 inter_chroma_4x4_dsc_len [MAX_INTER_QP_REGIONS][(MAX_4x4_DSC+7)/8];
extern uint32 inter_chroma_2x2_dsc_len [MAX_INTER_QP_REGIONS][2][(MAX_2x2_DSC+7)/8];
extern uint32 inter_level_dsc_len [MAX_INTER_QP_REGIONS][(MAX_LEVEL_DSC+7)/8];


extern HUFF_DEC_S	g_intra_cbp_dsc[MAX_INTRA_QP_REGIONS][2];
extern HUFF_DEC_S	g_intra_8x8_dsc[MAX_INTRA_QP_REGIONS][2][4];
extern HUFF_DEC_S	g_intra_l4x4_dsc[MAX_INTRA_QP_REGIONS][3];
extern HUFF_DEC_S	g_intra_l2x2_dsc[MAX_INTRA_QP_REGIONS][2];
extern HUFF_DEC_S	g_intra_c4x4_dsc[MAX_INTRA_QP_REGIONS];
extern HUFF_DEC_S	g_intra_c2x2_dsc[MAX_INTRA_QP_REGIONS][2];
extern HUFF_DEC_S	g_intra_lev_dsc[MAX_INTRA_QP_REGIONS];	

extern HUFF_DEC_S	g_inter_cbp_dsc[MAX_INTER_QP_REGIONS];
extern HUFF_DEC_S	g_inter_8x8_dsc[MAX_INTER_QP_REGIONS][4];
extern HUFF_DEC_S	g_inter_l4x4_dsc[MAX_INTER_QP_REGIONS];
extern HUFF_DEC_S	g_inter_l2x2_dsc[MAX_INTER_QP_REGIONS][2];
extern HUFF_DEC_S	g_inter_c4x4_dsc[MAX_INTER_QP_REGIONS];
extern HUFF_DEC_S	g_inter_c2x2_dsc[MAX_INTER_QP_REGIONS][2];
extern HUFF_DEC_S	g_inter_lev_dsc[MAX_INTER_QP_REGIONS];	


extern uint32		g_rvld_huff_tab[641 + 14];  
extern uint32		g_rvld_cache_buf[432];
// extern uint32		g_vsp_dct_buf[256];

extern uint32		g_max_num_len[17];

//extern uint32		g_rvld_intra_code[MAX_INTRA_QP_REGIONS][4*1024];
//extern uint32		g_rvld_inter_code[MAX_INTER_QP_REGIONS][4*1024];
//extern uint32		g_rvld_intra_max_base[MAX_INTRA_QP_REGIONS][1*1024];
//extern uint32		g_rvld_inter_max_base[MAX_INTER_QP_REGIONS][1*1024];

extern uint32		g_code_pos;
extern uint32		g_max_base_pos;

extern int	g_max_cnt_len_dsc8x8[17];

extern int	g_max_cnt_len_dsclev[17];


extern RVLD_MODE_T		* g_rvld_mode_ptr;

extern uint8 g_dsc_to_l0 [108];
extern uint8 g_dsc_to_l1 [108];
extern uint8 g_dsc_to_l2 [108];
extern uint8 g_dsc_to_l3 [108];

extern int g_mb_y;
extern int g_mb_x;

void RvldInit ();

void InitHuffTab ();

int GetCodeLength (MAX_REG_ARR_T *	max_reg_arr_ptr, uint32 bsm_rdata);

uint32 GetMinValue (MAX_REG_ARR_T *	max_reg_arr_ptr, int code_len);

void LoadMaxRegArray (
					  int	mb_type, 
					  int	dsc_type, 
					  int	blk_type,
					  int	cx,
					  int	dsc2x2_type,
					  int	is_intra_lev
					  );

uint32 RvldDecodeCBP (int mb_type, int32	cbp_type);

int RvldHuffDecoder (
					 MAX_REG_ARR_T *	max_reg_arr_ptr, 
					 int				base_addr, 
					 int				coeff_type,
					 int				cbp_type
					 );

void RvldDecodeBlk4x4 (
					   int	mb_type, 
					   int	blk_type,
					   int	blk_id
					   );

uint32 GetDsc4x4Code (int dsc4x4_tab_idx, int dsc4x4_addr);

void TestVecInit ();

void PrintfMBCoeff ();

void rvld_PrintfHuffTab ();

void PrintfHuffTabSdram (int qp);

void PrintfCmd (int cmd_type, int addr, int val, int mask);

void rvld_mb_ctr (/*MB_CACHE_T *mb_cache_ptr*/);

#ifndef _ARM_
extern FILE * g_rvld_trace_fp;
extern FILE * g_cbp_val_fp;
extern FILE * g_huff_dec_pf;
extern FILE * g_mb_coeff_fp;
extern FILE * g_huff_tab_fp;
extern FILE * g_huff_sdram_fp;
extern FILE * g_bitstream_fp;

extern FILE * g_cache_sta_fp;

extern FILE * g_acc_stat_fp;

extern FILE * g_rvld_tv_cmd_fp;
// extern int	  g_cmd_cnt;
#endif

#endif