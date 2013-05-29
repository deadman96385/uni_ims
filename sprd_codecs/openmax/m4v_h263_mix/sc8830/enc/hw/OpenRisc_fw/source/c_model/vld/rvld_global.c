/*rvld_global.c*/
#include "rvld_mode.h"

HUFF_DEC_S	g_intra_cbp_dsc[MAX_INTRA_QP_REGIONS][2];
HUFF_DEC_S	g_intra_8x8_dsc[MAX_INTRA_QP_REGIONS][2][4];
HUFF_DEC_S	g_intra_l4x4_dsc[MAX_INTRA_QP_REGIONS][3];
HUFF_DEC_S	g_intra_l2x2_dsc[MAX_INTRA_QP_REGIONS][2];
HUFF_DEC_S	g_intra_c4x4_dsc[MAX_INTRA_QP_REGIONS];
HUFF_DEC_S	g_intra_c2x2_dsc[MAX_INTRA_QP_REGIONS][2];
HUFF_DEC_S	g_intra_lev_dsc[MAX_INTRA_QP_REGIONS];	

HUFF_DEC_S	g_inter_cbp_dsc[MAX_INTER_QP_REGIONS];
HUFF_DEC_S	g_inter_8x8_dsc[MAX_INTER_QP_REGIONS][4];
HUFF_DEC_S	g_inter_l4x4_dsc[MAX_INTER_QP_REGIONS];
HUFF_DEC_S	g_inter_l2x2_dsc[MAX_INTER_QP_REGIONS][2];
HUFF_DEC_S	g_inter_c4x4_dsc[MAX_INTER_QP_REGIONS];
HUFF_DEC_S	g_inter_c2x2_dsc[MAX_INTER_QP_REGIONS][2];
HUFF_DEC_S	g_inter_lev_dsc[MAX_INTER_QP_REGIONS];	

/*huffman table, 641 for huffman table, and 14 for cache tag*/
uint32		g_rvld_huff_tab[641 + 14];  
uint32		g_rvld_cache_buf[432];

//uint32		g_vsp_dct_buf[256];

//uint32		g_rvld_intra_code[MAX_INTRA_QP_REGIONS][4*1024];
//uint32		g_rvld_inter_code[MAX_INTER_QP_REGIONS][4*1024];
//uint32		g_rvld_intra_max_base[MAX_INTRA_QP_REGIONS][1*1024];
//uint32		g_rvld_inter_max_base[MAX_INTER_QP_REGIONS][1*1024];

uint32		g_code_pos;
uint32		g_max_base_pos;

int	g_max_cnt_len_dsc8x8[17] = {
	0, 1, 2, 6, 11, 9, 6, 6, 2,
	0, 0, 0, 0, 0, 0, 0
};

int	g_max_cnt_len_dsclev[17] = {
	0, 1, 1, 2, 2, 2, 3, 3, 3, 
	4, 6, 7, 6, 7, 6, 2, 18
};


uint32		g_context_pos = 0;

uint32		g_max_num_len[17];

RVLD_MODE_T		* g_rvld_mode_ptr;	
