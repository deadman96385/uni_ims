/*tv_mea.c*/

#include <stdio.h>
#include "common_global.h"
#include "mmcodec.h"
#include "hmea_global.h"
#include "bsm_global.h"
#include "tv_mea.h"
#include "buffer_global.h"
#include "h264enc_global.h"
#include "h264enc_rc.h"
#include "h264enc_bitstrm.h"
#include "hvlc_global.h"
#include "hdbk_mode.h"

//FILE	*	g_mea_tv_cmd_fp;
//FILE	*	g_mv_pred_fp;
//FILE	*	g_pe_result_fp;
//FILE	*	g_blk_result_fp;
//FILE	*	g_imp_dcs_fp;
//FILE	*	g_mb_res_fp;
//FILE	*	g_src_frm_fp;
//FILE	*	g_srch_wind_fp;

// MBC
FILE	*	g_src_mb_fp;
FILE	*	g_buf_params_fp;
FILE	*	g_mb_pred_fp;
FILE	*	g_mb_dct_res_fp;
FILE	*	g_mb_idct_res_fp;
FILE	*	g_idct_out_fp;
FILE	*	g_mb_rec_fp;
FILE	*	g_mbc_out_fp;

// DBK
FILE	*	g_dbk_out_fp;

// MEA
FILE	*	g_fm_src_y_fp;
FILE	*	g_fm_src_uv_fp;
FILE	*	g_fm_ref_y_fp;
FILE	*	g_fm_ref_uv_fp;
FILE	*	g_ime_mb_sad_fp;
FILE	*	g_fme_mb_sad_fp;
FILE	*	g_mb_ime_fp;
FILE	*	g_src_mb_mea_fp;
FILE	*	g_mea_mca_mb_fp;
FILE	*	g_mea_ppa_buf_fp;
FILE	*	g_ime_best_sad_fp;
FILE	*	g_fme_best_sad_fp;
FILE	*	g_mea_config_fp;

// IEA
FILE	*	g_mb_iea_in_fp;
FILE	*	g_mb_iea_out_fp;
FILE	*	g_src_mb_iea_fp;

// PPA
FILE	*	g_vlc_para_fp;
FILE	*	g_mca_para_fp;
FILE	*	g_ppal_buf_fp;
FILE	*	g_dct_para_fp;
FILE	*	g_dct_cbp_fp;
FILE	*	g_dbk_para_fp;
FILE	*	g_ppa_config_fp;

// VLC
FILE	*	g_bsm_out_fp;
FILE	*	g_bsm_totalbits_fp;
FILE	*	g_dct_out_fp;
FILE	*	g_vlc_nC_fp;
FILE	*	g_vlc_offset_fp;
FILE	*	g_vlc_golomb_fp;
FILE	*	g_vlc_trace_level_fp;
FILE	*	g_vlc_trace_run_fp;
FILE	*	g_vlc_trace_token_fp;
FILE	*	g_vlc_table_coeff_fp;
FILE	*	g_vlc_table_zero_fp;
FILE	*	g_vlc_table_run_fp;

// DCT
FILE	*	g_dct_in_fp;
FILE	*	g_idct_in_fp;

// GLB REG
FILE	*	g_vsp_glb_reg_fp;

#define HEX

uint32 bsm_line;
void TVMeaInit ()
{
//	g_mea_tv_cmd_fp		= fopen ("..\\..\\test_vectors\\mea_tv_cmd.dat", "w");
//	g_mv_pred_fp		= fopen ("..\\..\\test_vectors\\mv_pred.txt", "w");
//	g_pe_result_fp		= fopen ("..\\..\\test_vectors\\pe_result.txt", "w");
//	g_blk_result_fp		= fopen ("..\\..\\test_vectors\\blk_result.txt", "w");
//	g_imp_dcs_fp		= fopen ("..\\..\\test_vectors\\imp_dcs.txt", "w");
//	g_mb_res_fp			= fopen ("..\\..\\test_vectors\\mb_res.txt", "w");
//	g_src_frm_fp		= fopen ("..\\..\\test_vectors\\src_frame.txt", "w");
//	g_srch_wind_fp		= fopen ("..\\..\\test_vectors\\srch_wind.txt", "w");

	// MBC
	g_src_mb_fp			= fopen ("..\\..\\test_vectors\\mea_cbuf.txt", "w");	// mbc_src_mb.txt
	g_mb_pred_fp		= fopen ("..\\..\\test_vectors\\mbc_ipred.txt", "w");	// mbc_mb_pred.txt
	g_mb_dct_res_fp		= fopen ("..\\..\\test_vectors\\mbc_dctout.txt", "w");	// mbc_mb_dct_res.txt
	g_mb_idct_res_fp	= fopen ("..\\..\\test_vectors\\mbc_idctin.txt", "w");	// mbc_mb_idct_res.txt
	g_idct_out_fp		= fopen ("..\\..\\test_vectors\\idct_out.txt", "w");
	g_mb_rec_fp			= fopen ("..\\..\\test_vectors\\mbc_mb_rec.txt", "w");
	g_mbc_out_fp		= fopen ("..\\..\\test_vectors\\mbc_out.txt", "w");

	// DBK
	g_dbk_out_fp		= fopen ("..\\..\\test_vectors\\dbk_out.txt", "w");
	g_fp_rec_frm_tv		= fopen ("..\\..\\test_vectors\\rec_frame.txt", "w");

	// MEA
	g_fm_src_y_fp		= fopen ("..\\..\\test_vectors\\mea_fm_src_y.txt", "w");
	g_fm_src_uv_fp		= fopen ("..\\..\\test_vectors\\mea_fm_src_uv.txt", "w");
	g_fm_ref_y_fp		= fopen ("..\\..\\test_vectors\\mea_fm_ref_y.txt", "w");
	g_fm_ref_uv_fp		= fopen ("..\\..\\test_vectors\\mea_fm_ref_uv.txt", "w");
	g_mb_ime_fp			= fopen ("..\\..\\test_vectors\\ime_fbuf.txt", "w");
	g_src_mb_mea_fp		= fopen ("..\\..\\test_vectors\\ime_cur_buf.txt", "w");
	g_mea_mca_mb_fp		= fopen ("..\\..\\test_vectors\\mea_pbuf.txt", "w");	// mca_out.txt
#if MEA_PATTERN
	g_ime_mb_sad_fp		= fopen ("..\\..\\test_vectors\\ime_mb_sad.txt", "w");
	g_fme_mb_sad_fp		= fopen ("..\\..\\test_vectors\\fme_mb_sad.txt", "w");
	g_ime_best_sad_fp	= fopen ("..\\..\\test_vectors\\ime_best_sad.txt", "w");
	g_fme_best_sad_fp	= fopen ("..\\..\\test_vectors\\fme_best_sad.txt", "w");
#endif
	g_mea_config_fp		= fopen ("..\\..\\test_vectors\\mea_config.txt", "w");

	// IEA
	g_src_mb_iea_fp		= fopen ("..\\..\\test_vectors\\iea_cbuf.txt", "w");
	g_mb_iea_in_fp		= fopen ("..\\..\\test_vectors\\iea_in.txt", "w");
	g_mb_iea_out_fp		= fopen ("..\\..\\test_vectors\\iea_out.txt", "w");
	FPRINTF_IEA (g_mb_iea_in_fp, "STD=0100\n");	// 0100 : H264
	FPRINTF_IEA (g_mb_iea_in_fp, "ENC=1\n");

	// PPA
//	g_ppal_buf_fp		= fopen ("..\\..\\test_vectors\\ppa_line_buf.txt", "w");
#ifndef VLC_TV_SPLIT
//	g_vlc_para_fp		= fopen ("..\\..\\test_vectors\\vlc_para.txt", "w");	// ppa_vlc_para.txt
	g_mca_para_fp		= fopen ("..\\..\\test_vectors\\vlc_para.txt", "w");	// mca_para.txt, 56-bit version of vlc_para.txt
#endif
	g_dct_para_fp		= fopen ("..\\..\\test_vectors\\dct_para.txt", "w");	// ppa_dct_para.txt
	FPRINTF_PPA (g_dct_para_fp, "//dct_mode=%d vsp_standard=%d scale_enable=%d\n", 0, 4, 0);
	g_dct_cbp_fp		= fopen ("..\\..\\test_vectors\\cbp_in.txt", "w");
	g_buf_params_fp		= fopen ("..\\..\\test_vectors\\mbc_para.txt", "w");	// mbc_buf_params.txt
	FPRINTF_MBC (g_buf_params_fp, "//STD=0100\tENC=1\n");	// 0100 : H264
	g_mea_ppa_buf_fp	= fopen ("..\\..\\test_vectors\\ppa_in.txt", "w");	// mea_para_buf.txt
	g_dbk_para_fp		= fopen ("..\\..\\test_vectors\\dbk_para.txt", "w");
	g_ppa_config_fp		= fopen ("..\\..\\test_vectors\\ppa_cfg.txt", "w");

	// VLC
	bsm_line = 0;
	g_bsm_out_fp		= fopen ("..\\..\\test_vectors\\bsm_enc_out.txt", "w");
	g_bsm_totalbits_fp	= fopen ("..\\..\\test_vectors\\bsm_total_bits.txt", "w");
#ifndef VLC_TV_SPLIT
	g_dct_out_fp		= fopen ("..\\..\\test_vectors\\dct_out.txt", "w");
	g_vlc_nC_fp			= fopen ("..\\..\\test_vectors\\vlc_nC.txt", "w");
	g_vlc_offset_fp		= fopen ("..\\..\\test_vectors\\vlc_out.txt", "w");	// vlc_offset.txt
//	g_vlc_golomb_fp		= fopen ("..\\..\\test_vectors\\vlc_golomb.txt", "w");
//	g_vlc_trace_level_fp= fopen ("..\\..\\test_vectors\\vlc_trace_level.txt", "w");
//	g_vlc_trace_run_fp	= fopen ("..\\..\\test_vectors\\vlc_trace_run_before.txt", "w");
//	g_vlc_trace_token_fp= fopen ("..\\..\\test_vectors\\vlc_trace_coeff.txt", "w");
//	g_vlc_table_coeff_fp= fopen ("..\\..\\test_vectors\\vlc_table_coeff_token.txt", "w");
//	g_vlc_table_zero_fp	= fopen ("..\\..\\test_vectors\\vlc_table_total_zero.txt", "w");
//	g_vlc_table_run_fp	= fopen ("..\\..\\test_vectors\\vlc_table_run_before.txt", "w");
#endif

	// DCT
	g_dct_in_fp			= fopen ("..\\..\\test_vectors\\dct_in.txt", "w");
//	g_idct_in_fp		= fopen ("..\\..\\test_vectors\\idct_in.txt", "w");

	// GLB REG
	g_vsp_glb_reg_fp	= fopen ("..\\..\\test_vectors\\vsp_glb_reg.txt", "w");

//	assert (g_mea_tv_cmd_fp != NULL);
//	assert (g_mv_pred_fp != NULL);
//	assert (g_pe_result_fp != NULL);
//	assert (g_blk_result_fp != NULL);
//	assert (g_imp_dcs_fp != NULL);
//	assert (g_mb_res_fp != NULL);
//	assert (g_src_frm_fp != NULL);
//	assert (g_srch_wind_fp != NULL);
}

#ifdef VLC_TV_SPLIT
void VLCSplitInit(ENC_IMAGE_PARAMS_T *img_ptr)
{
	if( ((g_nFrame_enc%FRAME_X) == 0) && (img_ptr->sh.i_first_mb == 0) )
	{
		FILE *dir_config;
		unsigned char s[100];
		sprintf(s, "../../test_vectors/vlc/frame_%03d/vlc_para.txt", g_nFrame_enc);
        assert(NULL != (g_vlc_para_fp = fopen(s, "w")));
		sprintf(s, "../../test_vectors/vlc/frame_%03d/dct_out.txt", g_nFrame_enc);
		assert(NULL != (g_dct_out_fp = fopen(s, "w")));
		sprintf(s, "../../test_vectors/vlc/frame_%03d/vlc_nC.txt", g_nFrame_enc);
		assert(NULL != (g_vlc_nC_fp = fopen(s, "w")));
		sprintf(s, "../../test_vectors/vlc/frame_%03d/vlc_out.txt", g_nFrame_enc); // vlc_offset.txt
		assert(NULL != (g_vlc_offset_fp = fopen(s, "w")));
//		sprintf(s, "../../test_vectors/vlc/frame_%03d/vlc_golomb.txt", g_nFrame_enc);
//		assert(NULL != (g_vlc_golomb_fp = fopen(s, "w")));
		sprintf(s, "../../test_vectors/vlc/frame_%03d/vlc_trace_level.txt", g_nFrame_enc);
		assert(NULL != (g_vlc_trace_level_fp = fopen(s, "w")));
		sprintf(s, "../../test_vectors/vlc/frame_%03d/vlc_trace_run_before.txt", g_nFrame_enc);
		assert(NULL != (g_vlc_trace_run_fp = fopen(s, "w")));
		sprintf(s, "../../test_vectors/vlc/frame_%03d/vlc_trace_coeff.txt", g_nFrame_enc);
		assert(NULL != (g_vlc_trace_token_fp = fopen(s, "w")));
		sprintf(s, "../../test_vectors/vlc/frame_%03d/vlc_table_coeff_token.txt", g_nFrame_enc);
		assert(NULL != (g_vlc_table_coeff_fp = fopen(s, "w")));
		sprintf(s, "../../test_vectors/vlc/frame_%03d/vlc_table_total_zero.txt", g_nFrame_enc);
		assert(NULL != (g_vlc_table_zero_fp = fopen(s, "w")));
		sprintf(s, "../../test_vectors/vlc/frame_%03d/vlc_table_run_before.txt", g_nFrame_enc);
		assert(NULL != (g_vlc_table_run_fp = fopen(s, "w")));


		sprintf(s, "../../test_vectors/vlc/frame_%03d/config.txt", g_nFrame_enc);
		assert(NULL != (dir_config = fopen(s, "w")));
		fprintf (dir_config, "//vsp_standard\n");
		fprintf (dir_config, "// h263 =0 , mp4=1, vp8=2,flv=3,h264=4,real8=5,real9=6\n");
		fprintf (dir_config, "4\n");
		fprintf (dir_config, "//block number\n");
		fprintf (dir_config, "%x\n", (img_ptr->frame_size_in_mbs));
		fprintf (dir_config, "//pic x size\n");
		fprintf (dir_config, "%x\n", (img_ptr->width+15)&0xfffffff0);
		fprintf (dir_config, "//pic y size\n");
		fprintf (dir_config, "%x\n", (img_ptr->height+15)&0xfffffff0);
		fprintf (dir_config, "//weight enable\n");
		fprintf (dir_config, "0\n");
		fclose(dir_config);
	}
}

void VLCSplitDeinit(ENC_IMAGE_PARAMS_T *img_ptr)
{
	if( (((g_nFrame_enc%FRAME_X) == (FRAME_X-1)) || ((g_nFrame_enc+1) == g_input->frame_num_dec))
		/*&& (img_ptr->sh.i_first_mb == 0)*/ )
	{
//		PrintfGolombSyntax();
		PrintfVLCTable();

		fclose(g_vlc_para_fp);
		fclose(g_dct_out_fp);
		fclose(g_vlc_nC_fp);
		fclose(g_vlc_offset_fp);
//		fclose(g_vlc_golomb_fp);
		fclose(g_vlc_trace_level_fp);
		fclose(g_vlc_trace_run_fp);
		fclose(g_vlc_trace_token_fp);
		fclose(g_vlc_table_coeff_fp);
		fclose(g_vlc_table_zero_fp);
		fclose(g_vlc_table_run_fp);
	}
}
#endif


/*void PrintfSrcFrame (uint32 * frm_y_ptr, uint32 * frm_uv_ptr, int width, int height)
{
	int			i;
	int			j;
	uint32		data;
	uint32	*	frm_ptr; 

	//printf Y
	frm_ptr = frm_y_ptr;
	for (j = 0; j < height; j++)
	{
		for (i = 0; i < width; i++)
		{
			data = frm_ptr[i];

			FPRINTF (g_src_frm_fp, "%08x\n", data);
		}

		frm_ptr += width;
	}

	frm_ptr = frm_uv_ptr;
	for (j = 0; j < height/2; j++)
	{
		for (i = 0; i < width; i++)
		{
			data = frm_ptr[i];
			
			FPRINTF (g_src_frm_fp, "%08x\n", data);
		}
		
		frm_ptr += width;
	}
}*/

int little_endian_map[4] = {1, 0, 3, 2};
void PrintfSrcMB (uint32 * src_mb_ptf, ENC_MB_MODE_T *mb_info_ptr)
{
	int		i, j;
	uint8 * pSrc = (uint8*)src_mb_ptf;

	FPRINTF_MBC (g_src_mb_fp, "//frame_cnt=%d, mb_x=%d, mb_y=%d\n", g_nFrame_enc, g_mode_dcs_ptr->mb_x, g_mode_dcs_ptr->mb_y);
	if(mb_info_ptr->type==I_4x4)
	{
		int		k, z;
		
		//  Print Order
		//	0-1 0-2 0-3 0-4		2-1 2-2 2-3 2-4
		//	0-5 0-6 0-7 0-8		2-5 2-6 2-7 2-8
		//	1-1 1-2 1-3 1-4		3-1 3-2 3-3 3-4
		//	1-5 1-6 1-7 1-8		3-5 3-6 3-7 3-8
		for (z = 0; z < 4; z++)		// Y
		{
			for (k = 0; k < 4; k++)
			{
				for (i = 0; i < 4; i++)
				{
					pSrc = (uint8 *)src_mb_ptf + 16*little_endian_map[i] + 4*k + 64*z;
					for (j = 3; j >= 0; j--)
					{
#ifdef HEX
						FPRINTF_MBC (g_src_mb_fp, "%02x", pSrc[j]);
#else
						FPRINTF_MBC (g_src_mb_fp, "%4d", pSrc[j]);
#endif
					}
					if (i%2==1)
						FPRINTF_MBC (g_src_mb_fp, "\n");
				}
			}
		}
		
		/*for (z = 0; z < 4; z++)		// UV 4x4
		{
			for (k = 0; k < 2; k++)
			{
				for (i = 0; i < 4; i++)
				{
					pSrc = (uint8 *)src_mb_ptf + 8*little_endian_map[i] + 4*k + 32*z + 256;
					for (j = 3; j >= 0; j--)
					{
#ifdef HEX
						FPRINTF_MBC (g_src_mb_fp, "%02x", pSrc[j]);
#else
						FPRINTF_MBC (g_src_mb_fp, "%4d", pSrc[j]);
#endif
					}
					if (i%2==1)
						FPRINTF_MBC (g_src_mb_fp, "\n");
				}
			}
		}*/
		pSrc = (uint8*)src_mb_ptf + 256;
		for (i = 32; i < 48; i++)
		{
			for (j = 7; j >= 0; j--)
#ifdef HEX
				FPRINTF_MBC (g_src_mb_fp, "%02x", pSrc[j]);
#else
				FPRINTF_MBC (g_src_mb_fp, "%4d", pSrc[j]);
#endif
			pSrc +=8;
			FPRINTF_MBC (g_src_mb_fp, "\n");
		}
	}
	else
	{
		for (i = 0; i < 48; i++)
		{
			for (j = 7; j >= 0; j--)
#ifdef HEX
				FPRINTF_MBC (g_src_mb_fp, "%02x", pSrc[j]);
#else
				FPRINTF_MBC (g_src_mb_fp, "%4d", pSrc[j]);
#endif
			pSrc +=8;
			FPRINTF_MBC (g_src_mb_fp, "\n");
		}
	}

	// IEA Source MB
	FPRINTF_IEA (g_src_mb_iea_fp, "//frame_cnt=%d, mb_x=%d, mb_y=%d\n", g_nFrame_enc, g_mode_dcs_ptr->mb_x, g_mode_dcs_ptr->mb_y);
	pSrc = (uint8*)src_mb_ptf;
	for (i = 0; i < 48; i++)
	{
		for (j = 7; j >= 0; j--)
#ifdef HEX
			FPRINTF_IEA (g_src_mb_iea_fp, "%02x", pSrc[j]);
#else
			FPRINTF_IEA (g_src_mb_iea_fp, "%4d", pSrc[j]);
#endif
		pSrc +=8;
		FPRINTF_IEA (g_src_mb_iea_fp, "\n");
	}
}

/*void PrintfSrchWind (uint32 * srch_wind_ptr)
{
	int			i;
	uint32		data;
	
	for (i = 0; i < 32*64 +16; i++)
	{
		data = srch_wind_ptr[i];
		
		FPRINTF (g_srch_wind_fp, "%08x\n", data);
	}
}*/

/*void PrintfMvPred (MOTION_VECTOR_T * mv_pred_ptr)
{
	int32	mv_xy;

	mv_xy = ((mv_pred_ptr->y << 16) | (mv_pred_ptr->x & 0xffff));

	FPRINTF (g_mv_pred_fp, "%08x\n", mv_xy);
}*/

/*void PrintfPeResult (int is_sp, int cost_pe0, int cost_pe1, int cost_pe2, int cost_pe3)
{
	FPRINTF (g_pe_result_fp, "%08x\n", cost_pe0);

	if (!is_sp)
	{
		FPRINTF (g_pe_result_fp, "%08x\n", cost_pe1);
		FPRINTF (g_pe_result_fp, "%08x\n", cost_pe2);
		FPRINTF (g_pe_result_fp, "%08x\n", cost_pe3);
	}
}*/

/*void PrintfBlkResult (int blk_cost, MOTION_VECTOR_T * mv)
{
	int32	mv_xy;
	static int blk_res_cnt = 0;

	if (blk_res_cnt == 164)
		printf ("");

	FPRINTF (g_blk_result_fp, "%08x\n", blk_cost);
	
	mv_xy = ((mv->y << 16) | (mv->x & 0xffff));

	FPRINTF (g_blk_result_fp, "%08x\n", mv_xy);

	blk_res_cnt += 2;
}*/

/*int imp_test_cnt = 0;
void PrintfImpDcs (int dc_cost, int hor_cost, int ver_cost, int left_avail, int top_avail)
{
	if ((dc_cost == 0x188) || (hor_cost == 0x188) || (ver_cost == 0x188))
		printf ("");

	FPRINTF (g_imp_dcs_fp, "%08x\n", dc_cost);	
	imp_test_cnt++;

	if (left_avail)
	{
		FPRINTF (g_imp_dcs_fp, "%08x\n", hor_cost);	
		imp_test_cnt++;
	}

	if (top_avail)
	{
		FPRINTF (g_imp_dcs_fp, "%08x\n", ver_cost);	
		imp_test_cnt++;
	}
}*/

/*void PrintfMBResult (MODE_DECISION_T * mode_dcs_ptr)
{
	int		intra_y_mode;
	int		intra_c_mode;
	int		data;

	intra_y_mode = mode_dcs_ptr->intra16_mode_y;
	intra_c_mode = mode_dcs_ptr->intra_mode_c;

	data = (mode_dcs_ptr->mb_type & 7);
	
	if (mode_dcs_ptr->mb_type == INTRA_MB_16X16)
	{
		
		data |= ((intra_c_mode & 3) << 8 ) | ((intra_y_mode & 3) << 4);

	}
	
	FPRINTF (g_mb_res_fp, "%08x\n", data);

	if (mode_dcs_ptr->mb_type != INTRA_MB_16X16)
	{
		data = (mode_dcs_ptr->blk_mv[0].y << 16) | (mode_dcs_ptr->blk_mv[0].x & 0xffff);
		FPRINTF (g_mb_res_fp, "%08x\n", data);

		data = (mode_dcs_ptr->blk_mv[1].y << 16) | (mode_dcs_ptr->blk_mv[1].x & 0xffff);
		FPRINTF (g_mb_res_fp, "%08x\n", data);

		data = (mode_dcs_ptr->blk_mv[2].y << 16) | (mode_dcs_ptr->blk_mv[2].x & 0xffff);
		FPRINTF (g_mb_res_fp, "%08x\n", data);

		data = (mode_dcs_ptr->blk_mv[3].y << 16) | (mode_dcs_ptr->blk_mv[3].x & 0xffff);
		FPRINTF (g_mb_res_fp, "%08x\n", data);		
	}
}*/

/*void PrintfCmd (int cmd_type, int addr, int val, int mask)
{
	if ((cmd_type == 0) || (cmd_type == 1))
	{
		FPRINTF(g_mea_tv_cmd_fp, "%x,%08x,%08x\n", cmd_type, addr, val);
	}
	else
	{
		FPRINTF(g_mea_tv_cmd_fp, "%x,%08x,%08x,%08x,%08x\n", cmd_type, addr, 0, mask, val);		
	}
}*/

extern int32 x264_scan8[16+2*4];
extern int32 g_blk_rec_ord_tbl [16];
void PrintfMBCBufParam(ENC_IMAGE_PARAMS_T *img_ptr, ENC_MB_MODE_T *mb_info_ptr, ENC_MB_CACHE_T *mb_cache_ptr)
{
	int i;
	uint32 temp=0;
	//int16 *dc_ptr;

	FPRINTF_MBC (g_buf_params_fp, "//frame_cnt=%d, mb_x=%d, mb_y=%d\n", g_nFrame_enc, g_mode_dcs_ptr->mb_x, g_mode_dcs_ptr->mb_y);
	temp |= (mb_info_ptr->type<=I_16x16);		// bit-0 : 1 for Intra MB
	//temp |= ((mb_info_ptr->type==P_SKIP) << 1); // bit-1 : 1 for SKIP
	temp |= ((mb_info_ptr->type==I_PCM) << 1); // bit-1 : 1 for IPCM
	temp |= (((mb_cache_ptr->mb_avail_d<<3) | (mb_cache_ptr->mb_avail_a<<2) | (mb_cache_ptr->mb_avail_b<<1) | mb_cache_ptr->mb_avail_c) << 2);	// bit-2-5 : MB availability
	temp |= (((mb_info_ptr->type==I_4x4)?0:((mb_info_ptr->type==I_16x16)?1:0)) << 6);	// bit-6-7 : Intra Pred Type, 0 for 4x4, 1 for 16x16, 2 for 8x8
	temp |= (mb_info_ptr->i_chroma_pred_mode << 8); // bit-8-9 : Intra Croma Pred Type
	temp |= ((img_ptr->mb_y+0)<<10);			// bit-10-16 MB_Y_ID
	temp |= ((((img_ptr->curr_mb_nr+1) == img_ptr->frame_size_in_mbs)||img_ptr->slice_end)<<31);			// bit-31 IS_LAST_MB
	FPRINTF_MBC (g_buf_params_fp, "%08x\n", temp);

	temp = 0;
	if (mb_info_ptr->type==I_16x16)
	{
		temp |= mb_info_ptr->i_intra16x16_pred_mode;
		FPRINTF_MBC (g_buf_params_fp, "%08x\n", temp);	// LUMA_MODE0
		temp = 0;
		FPRINTF_MBC (g_buf_params_fp, "%08x\n", temp);	// LUMA_MODE1
	}
	else if (mb_info_ptr->type==I_4x4)
	{
		temp |= (((mb_info_ptr->intra4x4_pred_mode[7]&0xf)<<28) | ((mb_info_ptr->intra4x4_pred_mode[6]&0xf)<<24) |
			((mb_info_ptr->intra4x4_pred_mode[5]&0xf)<<20) | ((mb_info_ptr->intra4x4_pred_mode[4]&0xf)<<16) |
			((mb_info_ptr->intra4x4_pred_mode[3]&0xf)<<12) | ((mb_info_ptr->intra4x4_pred_mode[2]&0xf)<<8) |
			((mb_info_ptr->intra4x4_pred_mode[1]&0xf)<<4) | (mb_info_ptr->intra4x4_pred_mode[0]&0xf));
		FPRINTF_MBC (g_buf_params_fp, "%08x\n", temp);	// LUMA_MODE0
		temp = 0;
		temp |= (((mb_info_ptr->intra4x4_pred_mode[15]&0xf)<<28) | ((mb_info_ptr->intra4x4_pred_mode[14]&0xf)<<24) |
			((mb_info_ptr->intra4x4_pred_mode[13]&0xf)<<20) | ((mb_info_ptr->intra4x4_pred_mode[12]&0xf)<<16) |
			((mb_info_ptr->intra4x4_pred_mode[11]&0xf)<<12) | ((mb_info_ptr->intra4x4_pred_mode[10]&0xf)<<8) |
			((mb_info_ptr->intra4x4_pred_mode[9]&0xf)<<4) | (mb_info_ptr->intra4x4_pred_mode[8]&0xf));
		FPRINTF_MBC (g_buf_params_fp, "%08x\n", temp);	// LUMA_MODE1
	}
	else
	{
		temp = 0;
		FPRINTF_MBC (g_buf_params_fp, "%08x\n", temp);	// LUMA_MODE0
		temp = 0;
		FPRINTF_MBC (g_buf_params_fp, "%08x\n", temp);	// LUMA_MODE1
	}

	temp = 0;
	// MBC Stage don't need CBP when encode
	/*for(i=0; i<16; i++)
	{
		temp |= ((mb_cache_ptr->nnz[x264_scan8[g_blk_rec_ord_tbl[i]]]!=0)<<i);		// CBP for each Y 4x4
	}
	for(i=16; i<24; i++)
	{
		temp |= ((mb_cache_ptr->nnz[x264_scan8[i]]!=0)<<i);		// CBP for each CbCr 4x4
	}*/
		// Should be (vsp_dct_io_0 + COEFF_CHROMA_DC_BASE + is_v*2)
		// OR (vsp_dct_io_0 + NZFLAG_CHROMA_DC_BASE + is_v)
		// OR ((int16 *)(vsp_dct_io_1 + HDCT_COEFF_CHROMA_DC_BASE))
		/*dc_ptr = (int16 *)(vsp_dct_io_1 + 200);
		for(i=16; i<20; i++)
		{
			temp |= (((mb_cache_ptr->nnz[x264_scan8[i]]!=0)||(dc_ptr[i-16]!=0))<<i);
		}
		dc_ptr = (int16 *)(vsp_dct_io_1 + 200+2);
		for(i=20; i<24; i++)
		{
			temp |= (((mb_cache_ptr->nnz[x264_scan8[i]]!=0)||(dc_ptr[i-20]!=0))<<i);
		}*/
	temp |= ((img_ptr->mb_x+0)<<24);
	FPRINTF_MBC (g_buf_params_fp, "%08x\n", temp);


	// MEA PPA BUF
	FPRINTF_MEA (g_mea_ppa_buf_fp, "//frame_cnt=%d, mb_x=%d, mb_y=%d\n", g_nFrame_enc, g_mode_dcs_ptr->mb_x, g_mode_dcs_ptr->mb_y);

	temp = 0;
	if( (mb_info_ptr->type == P_L0) || (mb_info_ptr->type == P_SKIP) )
		temp |= ((g_16x16_sad - g_16x16_mvd_cost) << 0);	// inter_sad[15:0]
	else if(mb_info_ptr->type == P_8x8)
		temp |= ((g_8x8_sad[0]+g_8x8_sad[1]+g_8x8_sad[2]+g_8x8_sad[3] -
			(g_8x8_mvd_cost[0]+g_8x8_mvd_cost[1]+g_8x8_mvd_cost[2]+g_8x8_mvd_cost[3])) << 0);	// inter_sad[15:0]
	FPRINTF_MEA (g_mea_ppa_buf_fp, "%08x", temp);
	temp = 0;
	temp |= (mb_info_ptr->type<=I_16x16);		// is_intra[0]
	temp |= (((mb_info_ptr->type==I_4x4) || (mb_info_ptr->type==P_8x8)) << 1);	// mb_partition[1], 1:4x4 or 8x8
	// bit[2] = 0
	temp |= (img_ptr->mb_y << 3);	// cur_mb_y[9:3]
	temp |= (img_ptr->mb_x << 10);	// cur_mb_x[16:10]
	//temp |= ((mb_info_ptr->type==P_SKIP) << 17); // is_skip[17]
	temp |= (mb_info_ptr->qp << 18);	// QP[23:18]
	// bit[30:24] = 0
	temp |= ((((img_ptr->curr_mb_nr+1) == img_ptr->frame_size_in_mbs)||img_ptr->slice_end)<<31);	// is_last_mb[31]
	FPRINTF_MEA (g_mea_ppa_buf_fp, "%08x\n", temp);

	temp = 0;
	if (mb_info_ptr->type==I_16x16)
	{
		for(i=0; i<8; i++)
			temp |= ((mb_info_ptr->i_intra16x16_pred_mode&0xf) << (i*4));
		FPRINTF_MEA (g_mea_ppa_buf_fp, "%08x", temp); // intra_pred_mode1
		//temp |= mb_info_ptr->i_intra16x16_pred_mode;
		FPRINTF_MEA (g_mea_ppa_buf_fp, "%08x\n", temp);	// intra_pred_mode0
		temp = 0;
		FPRINTF_MEA (g_mea_ppa_buf_fp, "%08x", temp);
		temp |= (mb_info_ptr->i_chroma_pred_mode << 0); // chrom_pred_mode[1:0]
		FPRINTF_MEA (g_mea_ppa_buf_fp, "%08x\n", temp);
	}
	else if (mb_info_ptr->type==I_4x4)
	{
		temp |= (((mb_info_ptr->intra4x4_pred_mode[15]&0xf)<<28) | ((mb_info_ptr->intra4x4_pred_mode[14]&0xf)<<24) |
			((mb_info_ptr->intra4x4_pred_mode[13]&0xf)<<20) | ((mb_info_ptr->intra4x4_pred_mode[12]&0xf)<<16) |
			((mb_info_ptr->intra4x4_pred_mode[11]&0xf)<<12) | ((mb_info_ptr->intra4x4_pred_mode[10]&0xf)<<8) |
			((mb_info_ptr->intra4x4_pred_mode[9]&0xf)<<4) | (mb_info_ptr->intra4x4_pred_mode[8]&0xf));
		FPRINTF_MEA (g_mea_ppa_buf_fp, "%08x", temp);	// intra_pred_mode1
		temp = 0;
		temp |= (((mb_info_ptr->intra4x4_pred_mode[7]&0xf)<<28) | ((mb_info_ptr->intra4x4_pred_mode[6]&0xf)<<24) |
			((mb_info_ptr->intra4x4_pred_mode[5]&0xf)<<20) | ((mb_info_ptr->intra4x4_pred_mode[4]&0xf)<<16) |
			((mb_info_ptr->intra4x4_pred_mode[3]&0xf)<<12) | ((mb_info_ptr->intra4x4_pred_mode[2]&0xf)<<8) |
			((mb_info_ptr->intra4x4_pred_mode[1]&0xf)<<4) | (mb_info_ptr->intra4x4_pred_mode[0]&0xf));
		FPRINTF_MEA (g_mea_ppa_buf_fp, "%08x\n", temp);	// intra_pred_mode0
		temp = 0;
		FPRINTF_MEA (g_mea_ppa_buf_fp, "%08x", temp);
		temp |= (mb_info_ptr->i_chroma_pred_mode << 0); // chrom_pred_mode[1:0]
		FPRINTF_MEA (g_mea_ppa_buf_fp, "%08x\n", temp);
	}
	else
	{
		temp |= ((g_mode_dcs_ptr->blk_mv[1].y & 0xff) << 0);	// [7:0]: mv_l0_1_y
		// 8'h0
		temp |= ((g_mode_dcs_ptr->blk_mv[1].x & 0x1ff) << 16);	// [24:16]: mv_l0_1_x
		FPRINTF_MEA (g_mea_ppa_buf_fp, "%08x", temp);
		temp = 0;
		temp |= ((g_mode_dcs_ptr->blk_mv[0].y & 0xff) << 0);	// [7:0]: mv_l0_0_y
		// 8'h0
		temp |= ((g_mode_dcs_ptr->blk_mv[0].x & 0x1ff) << 16);	// [24:16]: mv_l0_0_x
		FPRINTF_MEA (g_mea_ppa_buf_fp, "%08x\n", temp);
		
		temp = 0;
		temp |= ((g_mode_dcs_ptr->blk_mv[3].y & 0xff) << 0);	// [7:0]: mv_l0_3_y
		// 8'h0
		temp |= ((g_mode_dcs_ptr->blk_mv[3].x & 0x1ff) << 16);	// [24:16]: mv_l0_3_x
		FPRINTF_MEA (g_mea_ppa_buf_fp, "%08x", temp);
		temp = 0;
		temp |= ((g_mode_dcs_ptr->blk_mv[2].y & 0xff) << 0);	// [7:0]: mv_l0_2_y
		// 8'h0
		temp |= ((g_mode_dcs_ptr->blk_mv[2].x & 0x1ff) << 16);	// [24:16]: mv_l0_2_x
		FPRINTF_MEA (g_mea_ppa_buf_fp, "%08x\n", temp);
	}

	temp = 0;
	for(i=0; i<15; i++)
	{
		FPRINTF_MEA (g_mea_ppa_buf_fp, "%08x", temp);
		FPRINTF_MEA (g_mea_ppa_buf_fp, "%08x\n", temp);
	}
}

void PrintfPred(ENC_MB_MODE_T *mb_info_ptr)
{
	int		i,j;
	uint8	*pred = (uint8 *)vsp_mea_out_bfr;
	
	FPRINTF_MBC (g_mb_pred_fp, "//frame_cnt=%d, mb_x=%d, mb_y=%d\n", g_nFrame_enc, g_mode_dcs_ptr->mb_x, g_mode_dcs_ptr->mb_y);
	if (mb_info_ptr->type==I_4x4)
	{
		int		k, z;
		
		for (z = 0; z < 4; z++)		// Y
		{
			for (k = 0; k < 4; k++)
			{
				for (i = 0; i < 4; i++)
				{
					pred = (uint8 *)vsp_mea_out_bfr + 16*little_endian_map[i] + 4*k + 64*z;
					for (j = 3; j >= 0; j--)
					{
#ifdef HEX
						FPRINTF_MBC (g_mb_pred_fp, "%02x", pred[j]);
#else
						FPRINTF_MBC (g_mb_pred_fp, "%4d", pred[j]);
#endif
					}
					if (i%2==1)
						FPRINTF_MBC (g_mb_pred_fp, "\n");
				}
			}
		}
		
		/*for (z = 0; z < 4; z++)		// UV 4x4
		{
			for (k = 0; k < 2; k++)
			{
				for (i = 0; i < 4; i++)
				{
					pred = (uint8 *)vsp_mea_out_bfr + 8*little_endian_map[i] + 4*k + 32*z + 256;
					for (j = 3; j >= 0; j--)
					{
#ifdef HEX
						FPRINTF_MBC (g_mb_pred_fp, "%02x", pred[j]);
#else
						FPRINTF_MBC (g_mb_pred_fp, "%4d", pred[j]);
#endif
					}
					if (i%2==1)
						FPRINTF_MBC (g_mb_pred_fp, "\n");
				}
			}
		}*/
		pred = (uint8 *)vsp_mea_out_bfr + 256;
		for (i = 32; i < 48; i++)
		{
			for (j = 7; j >= 0; j--)
#ifdef HEX
				FPRINTF_MBC (g_mb_pred_fp, "%02x", pred[j]);
#else
				FPRINTF_MBC (g_mb_pred_fp, "%4d", pred[j]);
#endif
			pred +=8;
			FPRINTF_MBC (g_mb_pred_fp, "\n");
		}
	}
	else
	{
		for (i = 0; i < 48; i++)
		{
			for (j = 7; j >= 0; j--)
#ifdef HEX
				FPRINTF_MBC (g_mb_pred_fp, "%02x", pred[j]);
#else
				FPRINTF_MBC (g_mb_pred_fp, "%4d", pred[j]);
#endif
			pred +=8;
			FPRINTF_MBC (g_mb_pred_fp, "\n");
		}
	}

	// MEA MCA Buf
	FPRINTF_MEA (g_mea_mca_mb_fp, "//frame_cnt=%d, mb_x=%d, mb_y=%d\n", g_nFrame_enc, g_mode_dcs_ptr->mb_x, g_mode_dcs_ptr->mb_y);
	pred = (uint8 *)vsp_mea_out_bfr;
	if( /*(mb_info_ptr->type==P_SKIP) ||*/ (mb_info_ptr->type<=I_16x16) )
	{
		for (i = 0; i < 48; i++)
		{
			for (j = 7; j >= 0; j--)
#ifdef HEX
				FPRINTF_MEA (g_mea_mca_mb_fp, "%02x", 0);
#else
				FPRINTF_MEA (g_mea_mca_mb_fp, "%4d", 0);
#endif
			FPRINTF_MEA (g_mea_mca_mb_fp, "\n");
		}
	}
	else
	{
		for (i = 0; i < 48; i++)
		{
			for (j = 7; j >= 0; j--)
#ifdef HEX
				FPRINTF_MEA (g_mea_mca_mb_fp, "%02x", pred[j]);
#else
				FPRINTF_MEA (g_mea_mca_mb_fp, "%4d", pred[j]);
#endif
			pred +=8;
			FPRINTF_MEA (g_mea_mca_mb_fp, "\n");
		}
	}
}

extern uint32	mea_src_buf0[96];
void PrintfDCTIn(ENC_MB_MODE_T *mb_info_ptr)
{
	int		i,j;

	uint8 * pSrc = (uint8 *)mea_src_buf0;
	uint8 * pRef = (uint8 *)vsp_mea_out_bfr;
	
	FPRINTF_MBC (g_mb_dct_res_fp, "//frame_cnt=%d, mb_x=%d, mb_y=%d\n", g_nFrame_enc, g_mode_dcs_ptr->mb_x, g_mode_dcs_ptr->mb_y);
	if (mb_info_ptr->type==I_4x4)
	{
		int		k, z;

		for (z = 0; z < 4; z++)		// Y
		{
			for (k = 0; k < 4; k++)
			{
				for (i = 0; i < 4; i++)
				{
					pSrc = (uint8 *)mea_src_buf0 + 16*little_endian_map[i] + 4*k + 64*z;
					pRef = (uint8 *)vsp_mea_out_bfr + 16*little_endian_map[i] + 4*k + 64*z;
					for (j = 3; j >= 0; j--)
					{
#ifdef HEX
						FPRINTF_MBC (g_mb_dct_res_fp, "%04x", ((int16)pSrc[j] - (int16)pRef[j])&0xffff);
#else
						FPRINTF_MBC (g_mb_dct_res_fp, "%4d", (int16)pSrc[j] - (int16)pRef[j]);
#endif
					}
					if (i%2==1)
						FPRINTF_MBC (g_mb_dct_res_fp, "\n");
				}
			}
		}

		/*for (z = 0; z < 4; z++)		// UV 4x4
		{
			for (k = 0; k < 2; k++)
			{
				for (i = 0; i < 4; i++)
				{
					pSrc = (uint8 *)mea_src_buf0 + 8*little_endian_map[i] + 4*k + 32*z + 256;
					pRef = (uint8 *)vsp_mea_out_bfr + 8*little_endian_map[i] + 4*k + 32*z + 256;
					for (j = 3; j >= 0; j--)
					{
#ifdef HEX
						FPRINTF_MBC (g_mb_dct_res_fp, "%04x", ((int16)pSrc[j] - (int16)pRef[j])&0xffff);
#else
						FPRINTF_MBC (g_mb_dct_res_fp, "%4d", (int16)pSrc[j] - (int16)pRef[j]);
#endif
					}
					if (i%2==1)
						FPRINTF_MBC (g_mb_dct_res_fp, "\n");
				}
			}
		}*/
		pSrc = (uint8 *)mea_src_buf0 + 256;
		pRef = (uint8 *)vsp_mea_out_bfr + 256;
		for (i = 32; i < 48; i++)
		{
			for (j = 7; j >= 0; j--)
#ifdef HEX
				FPRINTF_MBC (g_mb_dct_res_fp, "%04x", ((int16)pSrc[j] - (int16)pRef[j])&0xffff);
#else
				FPRINTF_MBC (g_mb_dct_res_fp, "%4d", (int16)pSrc[j] - (int16)pRef[j]);
#endif
			pSrc += 8;
			pRef += 8;
			FPRINTF_MBC (g_mb_dct_res_fp, "\n");
		}
	}
	else
	{
		for (i = 0; i < 48; i++)
		{
			for (j = 7; j >= 0; j--)
#ifdef HEX
				FPRINTF_MBC (g_mb_dct_res_fp, "%04x", ((int16)pSrc[j] - (int16)pRef[j])&0xffff);
#else
				FPRINTF_MBC (g_mb_dct_res_fp, "%4d", (int16)pSrc[j] - (int16)pRef[j]);
#endif
			pSrc += 8;
			pRef += 8;
			FPRINTF_MBC (g_mb_dct_res_fp, "\n");
		}
	}


	FPRINTF_DCT (g_dct_in_fp, "//frame_cnt=%d, mb_x=%d, mb_y=%d\n", g_nFrame_enc, g_mode_dcs_ptr->mb_x, g_mode_dcs_ptr->mb_y);
	pSrc = (uint8 *)mea_src_buf0;
	pRef = (uint8 *)vsp_mea_out_bfr;
	for (i = 0; i < 96; i++)
	{
		for (j = 3; j >= 0; j--)
#ifdef HEX
			FPRINTF_DCT (g_dct_in_fp, "%04x", ((int16)pSrc[j] - (int16)pRef[j])&0xffff);
#else
			FPRINTF_DCT (g_dct_in_fp, "%4d", (int16)pSrc[j] - (int16)pRef[j]);
#endif
		pSrc += 4;
		pRef += 4;
		FPRINTF_DCT (g_dct_in_fp, "\n");
	}
}

void PrintfDCTOut(ENC_MB_MODE_T *mb_info_ptr, short dct_buf[])
{
	int		i,j;
	int offset, blk_x, blk_y, count;
	unsigned __int64 temp = 0;

	FPRINTF_VLC (g_dct_out_fp, "//frame_cnt=%d, Y_DC_Valid=%d, mb_x=%d, mb_y=%d\n", 
		g_nFrame_enc, mb_info_ptr->type==I_16x16, g_mode_dcs_ptr->mb_x, g_mode_dcs_ptr->mb_y);

	// Count total non-zeros
	for (i =0; i<16; i++)	// Y
	{
		blk_x = (i%4);
		blk_y = (i>>2);
		offset = blk_x*4 + blk_y*16*4;
		count = 0;

		if(mb_info_ptr->type==I_16x16)
			dct_buf[offset] = dct_buf[384+i];	// replace DC
		else
			dct_buf[384+i] = dct_buf[offset];	// For accordance with dct nz-flag count
		//	count -= (dct_buf[offset-64]==0);

		for(j=0; j<4; j++)
		{
			count += (dct_buf[offset+0]!=0);
			count += (dct_buf[offset+1]!=0);
			count += (dct_buf[offset+2]!=0);
			count += (dct_buf[offset+3]!=0);
			offset +=16;
		}
		dct_buf[408+(i>>1)] |= ((count&0xff) << ((i%2)*8));
	}
	for (i =0; i<8; i++)	// UV
	{
		blk_x = (i%2);
		blk_y = ((i>>1)%2);
		offset = 256 + (i>>2)*64 + blk_x*4 + blk_y*8*4;
		count = 0;
		
		//dct_buf[offset] = dct_buf[400+i];// replace DC

		for(j=0; j<4; j++)
		{
			count += (dct_buf[offset+0]!=0);
			count += (dct_buf[offset+1]!=0);
			count += (dct_buf[offset+2]!=0);
			count += (dct_buf[offset+3]!=0);
			offset +=8;
		}
		count -= (dct_buf[offset-32]!=0);
		count += (dct_buf[400+i]!=0);
		dct_buf[416+(i>>1)] |= ((count&0xff) << ((i%2)*8));
	}

	offset = 384;	// Y DC
	count = 0;
	temp = 0;
	for(j=0; j<16; j++)
	{
		count += (dct_buf[offset+j]!=0);
		temp |= ((__int64)(dct_buf[offset+j]!=0) << j);		// Y DC nz-flag
	}
	dct_buf[420] |= ((count&0xff) << 0);
	dct_buf[421] |= ((temp&0xff) << 8);
	dct_buf[422] |= (((temp>>8)&0xff) << 0);

	for(i=0; i<2; i++)
	{
		offset = 400 + 4*i;	// UV DC
		count = 0;
		temp = 0;
		for(j=0; j<4; j++)
		{
			count += (dct_buf[offset+j]!=0);
			temp |= ((__int64)(dct_buf[offset+j]!=0) << j);
		}
		if(i==0)
		{
			dct_buf[420] |= ((count&0xff) << 8);
			dct_buf[422] |= ((temp&0xff) << 8);	// U DC nz-flag
		}
		else
		{
			dct_buf[421] |= ((count&0xff) << 0);
			dct_buf[423] |= ((temp&0xff) << 0);	// V DC nz-flag
		}
	}

	for (i = 0; i < 106; i++)
	{
		for (j = 3; j >= 0; j--)
#ifdef HEX
			FPRINTF_VLC (g_dct_out_fp, "%04x", dct_buf[4*i+j]&0xffff);
#else
			FPRINTF_VLC (g_dct_out_fp, "%4d", dct_buf[4*i+j]);
#endif
		FPRINTF_VLC (g_dct_out_fp, "\n" );
	}

	// Replace DC
	for (i =0; i<8; i++)	// UV
	{
		blk_x = (i%2);
		blk_y = ((i>>1)%2);
		offset = 256 + (i>>2)*64 + blk_x*4 + blk_y*8*4;
		dct_buf[offset] = dct_buf[400+i];// replace DC
	}

	// YUV nz-flag
	for (j=0; j<6; j++)
	{
		offset = 64*j;
		temp = 0;
		for(i=0; i<64; i++)
			temp |= ((__int64)(dct_buf[offset+i]!=0) << i);
		FPRINTF_VLC (g_dct_out_fp, "%016I64x\n", (__int64)temp);
	}
}


void PrintBSMOut(ENC_IMAGE_PARAMS_T *img_ptr)
{
	int	i,j;
	int	end, shift;
	int zero_count = 0;
//	int start_code;
//	int start_code_count = 0;

//	static int sps_pps_passed = 0;
//	static int slice_start = 0;

	/*if( (g_nFrame_enc==0) && (sps_pps_passed==0) )
	{
		start_code = 2;
		sps_pps_passed = 1;
	}
	else
		start_code = 0;*/

	clear_bsm_fifo();
	end	= (g_bsm_reg_ptr->TOTAL_BITS>>3);

//	if(img_ptr->sh.i_first_mb == 0)
		FPRINTF_VLC (g_bsm_out_fp, "//frame_cnt=%d slice_nr=%d\n", g_nFrame_enc, img_ptr->slice_nr);

//	shift = slice_start;
	shift = 0;
	
	for(i=0; i<(end+shift); i+=8)
	{
		for(j=0/*slice_start*/; j<8; j++)
		{
			if((i+j)<(end+shift))
			{
				if(zero_count==2)
				{
					if( (g_enc_image_ptr->pOneFrameBitstream[i+j-shift]==0x0) ||
						(g_enc_image_ptr->pOneFrameBitstream[i+j-shift]==0x1) ||
						(g_enc_image_ptr->pOneFrameBitstream[i+j-shift]==0x2) ||
						(g_enc_image_ptr->pOneFrameBitstream[i+j-shift]==0x3) )
					{
						//if(start_code_count > 0)
						//{
							FPRINTF_VLC (g_bsm_out_fp, "%02x", 0x03);
							shift++;
						/*}
						else
						{
							start_code_count++;
							FPRINTF_VLC (g_bsm_out_fp, "%02x", g_enc_image_ptr->pOneFrameBitstream[i+j-shift]);
						}*/
					}
					else
						FPRINTF_VLC (g_bsm_out_fp, "%02x", g_enc_image_ptr->pOneFrameBitstream[i+j-shift]);
					zero_count = 0;
				}
				else
				{
					if(g_enc_image_ptr->pOneFrameBitstream[i+j-shift] == 0)
						zero_count++;
					else
						zero_count = 0;
					FPRINTF_VLC (g_bsm_out_fp, "%02x", g_enc_image_ptr->pOneFrameBitstream[i+j-shift]);
				}
			}
			else
			{
				//if( ((img_ptr->curr_mb_nr+1) == img_ptr->frame_size_in_mbs) || is_sps )
				//{
					if(j != 0)
					{
						for(; j<8; j++)
							FPRINTF_VLC (g_bsm_out_fp, "%02x", 0);
						FPRINTF_VLC (g_bsm_out_fp, "\n");
						bsm_line++;
						//slice_start = 0;
					}
				//}
				//else
				//	slice_start = j;
				return;
			}
		}
		//slice_start = 0;
		FPRINTF_VLC (g_bsm_out_fp, "\n");
		bsm_line++;
	}

	/*if( (img_ptr->curr_mb_nr+1) == img_ptr->frame_size_in_mbs)
	{
		if(slice_start != 0)
		{
			for(j=slice_start; j<8; j++)
				FPRINTF_VLC (g_bsm_out_fp, "%02x", 0);
			FPRINTF_VLC (g_bsm_out_fp, "\n");
			slice_start = 0;
		}
	}*/
}
void PrintBSMLineNum()
{
	FILE *bsm_line_fp = fopen ("..\\..\\test_vectors\\bsm_enc_length.txt", "w");
	FPRINTF_VLC (bsm_line_fp, "%x\n", bsm_line);
	fclose(bsm_line_fp);
}


void PrintfnCValue(ENC_MB_CACHE_T *mb_cache_ptr)
{
	int		i;
	unsigned __int64 temp = 0;

	uint8 zScanOrder[16] =
	{
		0,  1,  4,  5,
		2,  3,  6,  7,
		8,  9, 12, 13,
		10, 11, 14, 15
	};

	FPRINTF_VLC (g_vlc_nC_fp, "//frame_cnt=%d, mb_x=%d, mb_y=%d\n", g_nFrame_enc, g_mode_dcs_ptr->mb_x, g_mode_dcs_ptr->mb_y);
	
	for (i = 0; i < 16; i+=4)
	{
		temp |= ( ((__int64)(mb_cache_ptr->nnz[x264_scan8[zScanOrder[i+0]]-8]&0x7f)<<8) | (__int64)(mb_cache_ptr->nnz[x264_scan8[zScanOrder[i+0]]-1]&0x7f) );
		temp |= ( ((__int64)(mb_cache_ptr->nnz[x264_scan8[zScanOrder[i+1]]-8]&0x7f)<<24) | ((__int64)(mb_cache_ptr->nnz[x264_scan8[zScanOrder[i+1]]-1]&0x7f)<<16) );
		temp |= ( ((__int64)(mb_cache_ptr->nnz[x264_scan8[zScanOrder[i+2]]-8]&0x7f)<<40) | ((__int64)(mb_cache_ptr->nnz[x264_scan8[zScanOrder[i+2]]-1]&0x7f)<<32) );
		temp |= ( ((__int64)(mb_cache_ptr->nnz[x264_scan8[zScanOrder[i+3]]-8]&0x7f)<<56) | ((__int64)(mb_cache_ptr->nnz[x264_scan8[zScanOrder[i+3]]-1]&0x7f)<<48) );
		FPRINTF_VLC (g_vlc_nC_fp, "%016I64x\n", (__int64)temp);
		temp = 0;
	}
	for (i = 16; i < 24; i+=4)
	{
		temp |= ( ((__int64)(mb_cache_ptr->nnz[x264_scan8[i+0]-8]&0x7f)<<8) | (__int64)(mb_cache_ptr->nnz[x264_scan8[i+0]-1]&0x7f) );
		temp |= ( ((__int64)(mb_cache_ptr->nnz[x264_scan8[i+1]-8]&0x7f)<<24) | ((__int64)(mb_cache_ptr->nnz[x264_scan8[i+1]-1]&0x7f)<<16) );
		temp |= ( ((__int64)(mb_cache_ptr->nnz[x264_scan8[i+2]-8]&0x7f)<<40) | ((__int64)(mb_cache_ptr->nnz[x264_scan8[i+2]-1]&0x7f)<<32) );
		temp |= ( ((__int64)(mb_cache_ptr->nnz[x264_scan8[i+3]-8]&0x7f)<<56) | ((__int64)(mb_cache_ptr->nnz[x264_scan8[i+3]-1]&0x7f)<<48) );
		FPRINTF_VLC (g_vlc_nC_fp, "%016I64x\n", (__int64)temp);
		temp = 0;
	}
}

void PrintfCavlcOffset(ENC_MB_MODE_T *mb_info_ptr, uint8* comment)
{
	int	i,j;
	int	start, end;

	unsigned __int64 bits = 0;
	uint32 total_bits = mb_info_ptr->cavlc_end_bits-mb_info_ptr->cavlc_start_bits;

	//FPRINTF_VLC (g_vlc_offset_fp, "//frame_cnt=%d, mb_x=%d, mb_y=%d\n", g_nFrame_enc, g_mode_dcs_ptr->mb_x, g_mode_dcs_ptr->mb_y);

	clear_bsm_fifo();
	start = (mb_info_ptr->cavlc_start_bits>>3);
	end	= (mb_info_ptr->cavlc_end_bits>>3);

	//FPRINTF_VLC (g_vlc_offset_fp, "//Start_bytes=%x, Start_bits=%x, End_bytes=%x, End_bits=%x\n", start, (mb_info_ptr->cavlc_start_bits&0x7), end, mb_info_ptr->cavlc_end_bits&0x7);
	//FPRINTF_VLC (g_vlc_offset_fp, "//Start_bits=%x, End_bytes=%x, End_bits=%x\n", (mb_info_ptr->cavlc_start_bits&0x7), (end-start+1)%8, mb_info_ptr->cavlc_end_bits&0x7);
	if(total_bits==0)
		return;
	FPRINTF_VLC (g_vlc_offset_fp, "//Total_bits=%d\n", total_bits);
	if(comment!=NULL)
		FPRINTF_VLC (g_vlc_offset_fp, "//%s\n", comment);

#if 0
	for(i=start; i<=end; i+=8)
	{
		for(j=7; j>=0; j--)
		{
			if((i+j)<end)
				FPRINTF_VLC (g_vlc_offset_fp, "%02x", g_enc_image_ptr->pOneFrameBitstream[i+j]);
			else if((i+j)==end)
				FPRINTF_VLC (g_vlc_offset_fp, "%02x", (jpeg_bsmr.data[0] >> 24)&0xff);
			else
				FPRINTF_VLC (g_vlc_offset_fp, "%02x", 0);
		}
		FPRINTF_VLC (g_vlc_offset_fp, "\n");
	}
#else
	for(i=start; i<=end; i+=8)
	{
		for(j=7; j>=0; j--)
		{
			if((i+j)<end)
				bits |= ((__int64)g_enc_image_ptr->pOneFrameBitstream[i+j] << ((7-j)*8));
			else if((i+j)==end)
				bits |= ((__int64)((jpeg_bsmr.data[0]>>0x18)&0xff) << ((7-j)*8));
		}
	}
	bits <<= (mb_info_ptr->cavlc_start_bits&0x7);
	for(j=0; j<8; j++)
		FPRINTF_VLC (g_vlc_offset_fp, "%02x", ((__int64)bits>>(j*8))&0xff);
	FPRINTF_VLC (g_vlc_offset_fp, "\n");
#endif
}

void PrintfGolombSyntax()
{
	int32 i, j, k;
	int32 val[200];
	int	start;
	int end;
	uint32 start_bits, end_bits;

	// UE
	for(k=0; k<64; k++)
		val[k] = k;
	for(k=1; k<14; k++)
		val[63+k] = k*579;
	val[77] = 8160;

	for(k=0; k<78; k++)
	{
		start_bits = g_bsm_reg_ptr->TOTAL_BITS;
		WRITE_UE_V(val[k]);
		end_bits = g_bsm_reg_ptr->TOTAL_BITS;

		clear_bsm_fifo();

		start = (start_bits>>3);
		end = (end_bits>>3);

		//FPRINTF_VLC (g_vlc_golomb_fp, "UE_value=%4d\n", val[k]);
		FPRINTF_VLC (g_vlc_golomb_fp, "UE_value=%08x\n", val[k]);
		FPRINTF_VLC (g_vlc_golomb_fp, "Start_bits=%x, Valid_bits=%x\n", (start_bits&0x7), end_bits-start_bits);

		for(i=start; i<=end; i+=8)
		{
			for(j=7; j>=0; j--)
			{
				if((i+j)<end)
					FPRINTF_VLC (g_vlc_golomb_fp, "%02x", g_enc_image_ptr->pOneFrameBitstream[i+j]);
				else if((i+j)==end)
					FPRINTF_VLC (g_vlc_golomb_fp, "%02x", (jpeg_bsmr.data[0] >> 24)&0xff);
				else
					FPRINTF_VLC (g_vlc_golomb_fp, "%02x", 0);
			}
			FPRINTF_VLC (g_vlc_golomb_fp, "\n\n");
		}
	}
	
	// SE
	for(k=0; k<52; k++)
		val[k] = k-26;
	for(k=1; k<6; k++)
		val[51+k] = -k*379;
	val[57] = -2048;
	for(k=1; k<6; k++)
		val[57+k] = k*357;
	val[63] = 2048;

	for(k=0; k<64; k++)
	{
		start_bits = g_bsm_reg_ptr->TOTAL_BITS;
		WRITE_SE_V(val[k]);
		end_bits = g_bsm_reg_ptr->TOTAL_BITS;
		
		clear_bsm_fifo();
		
		start = (start_bits>>3);
		end = (end_bits>>3);
		
		//FPRINTF_VLC (g_vlc_golomb_fp, "SE_value=%4d\n", val[k]);
		FPRINTF_VLC (g_vlc_golomb_fp, "SE_value=%08x\n", val[k]);
		FPRINTF_VLC (g_vlc_golomb_fp, "Start_bits=%x, Valid_bits=%x\n", (start_bits&0x7), end_bits-start_bits);
		
		for(i=start; i<=end; i+=8)
		{
			for(j=7; j>=0; j--)
			{
				if((i+j)<end)
					FPRINTF_VLC (g_vlc_golomb_fp, "%02x", g_enc_image_ptr->pOneFrameBitstream[i+j]);
				else if((i+j)==end)
					FPRINTF_VLC (g_vlc_golomb_fp, "%02x", (jpeg_bsmr.data[0] >> 24)&0xff);
				else
					FPRINTF_VLC (g_vlc_golomb_fp, "%02x", 0);
			}
			FPRINTF_VLC (g_vlc_golomb_fp, "\n\n");
		}
	}

	//TE
	for(k=0; k<4; k++)
		val[k] = k;
	for(k=0; k<6; k++)
		val[4+k] = k;
	for(k=0; k<10; k++)
	{
		start_bits = g_bsm_reg_ptr->TOTAL_BITS;
		WRITE_TE_V((k>3)+1, val[k]);
		end_bits = g_bsm_reg_ptr->TOTAL_BITS;
		
		clear_bsm_fifo();
		
		start = (start_bits>>3);
		end = (end_bits>>3);
		
		//FPRINTF_VLC (g_vlc_golomb_fp, "TE_value=%4d, x=%d\n", val[k], (k>3)+1);
		FPRINTF_VLC (g_vlc_golomb_fp, "TE_value=%08x, x=%x\n", val[k], (k>3)+1);
		FPRINTF_VLC (g_vlc_golomb_fp, "Start_bits=%x, Valid_bits=%x\n", (start_bits&0x7), end_bits-start_bits);
		
		for(i=start; i<=end; i+=8)
		{
			for(j=7; j>=0; j--)
			{
				if((i+j)<end)
					FPRINTF_VLC (g_vlc_golomb_fp, "%02x", g_enc_image_ptr->pOneFrameBitstream[i+j]);
				else if((i+j)==end)
					FPRINTF_VLC (g_vlc_golomb_fp, "%02x", (jpeg_bsmr.data[0] >> 24)&0xff);
				else
					FPRINTF_VLC (g_vlc_golomb_fp, "%02x", 0);
			}
			FPRINTF_VLC (g_vlc_golomb_fp, "\n\n");
		}
	}
}


void PrintfVLCTable()
{
	int32 i, j, k;
	uint32 temp = 0;

	// coeff_token, width[12:8] data[7:0], width=1~16
	// x264_coeff_token[4] is for chroma DC, need not output
	for(i=1; i<17; i++)		// total_coeff[3:0], 1~16, 0 is not needed
		for(j=0; j<4; j++)	// tail_one[1:0], 0~3
			for(k=0; k<4; k++)	// offset[1:0], [0<=nc<2, 2<=nc<4, 4<=nc<8, 8<nc]
			{
				temp |= x264_coeff_token[k][i*4+j].i_bits;
				temp |= (x264_coeff_token[k][i*4+j].i_size << 8);
				FPRINTF_VLC (g_vlc_table_coeff_fp, "%04x\n", temp);
				temp = 0;
			}

	// total_zero, width[7:4] data[3:0], width<16
	// chroma DC need not output
	for(i=0; i<16; i++)		// total_zero[3:0], 0~15
		for(j=0; j<15; j++)	// tzvcindex[3:0], 1~15
			{
				temp |= x264_total_zeros[j][i].i_bits;
				temp |= (x264_total_zeros[j][i].i_size << 4);
				FPRINTF_VLC (g_vlc_table_zero_fp, "%02x\n", temp);
				temp = 0;
			}

	// run_before, width[7:4] data[3:0], width<16
	for(i=0; i<7; i++)		// zero_left[2:0], 0~6
		for(j=0; j<15; j++)	// run_before[3:0], 0~14
		{
			temp |= x264_run_before[i][j].i_bits;
			temp |= (x264_run_before[i][j].i_size << 4);
			FPRINTF_VLC (g_vlc_table_run_fp, "%02x\n", temp);
			temp = 0;
		}
}


void PrintfIDCTIn(short dct_buf[])
{
	int		i,j;

	FPRINTF_DCT (g_idct_in_fp, "//frame_cnt=%d, mb_x=%d, mb_y=%d\n", g_nFrame_enc, g_mode_dcs_ptr->mb_x, g_mode_dcs_ptr->mb_y);
	for (i = 0; i < 108; i++)
	{
		for (j = 3; j >= 0; j--)
#ifdef HEX
			FPRINTF_DCT (g_idct_in_fp, "%04x", dct_buf[4*i+j]&0xffff);
#else
			FPRINTF_DCT (g_idct_in_fp, "%4d", dct_buf[4*i+j]);
#endif
		FPRINTF_DCT (g_idct_in_fp, "\n" );
	}	
	FPRINTF_DCT (g_idct_in_fp, "%016x\n",0 );
}

// Scan vsp_dct_io_0 in [0 1 4 5]
// extern int32 g_blk_rec_ord_tbl [16];
void PrintfIDCTOut(ENC_MB_MODE_T *mb_info_ptr)
{
	int		i,j, k;
	int16 * dct_io_bfr = (int16*)vsp_dct_io_0;
	int		offset, z;

	FPRINTF_MBC (g_mb_idct_res_fp, "//frame_cnt=%d, mb_x=%d, mb_y=%d\n", g_nFrame_enc, g_mode_dcs_ptr->mb_x, g_mode_dcs_ptr->mb_y);
	if (mb_info_ptr->type==I_4x4)
	{
		for (i = 0; i < 16; i++)	// Luma
		{
			dct_io_bfr = (int16*)vsp_dct_io_0 + g_blk_rec_ord_tbl[i]*16;

			offset = 0;
			for (k = 0; k < 2; k++)
			{
				for (j = 7; j >= 0; j--)
				{
	#ifdef HEX
					FPRINTF_MBC (g_mb_idct_res_fp, "%04x", dct_io_bfr[offset+j]&0xffff);
	#else
					FPRINTF_MBC (g_mb_idct_res_fp, "%4d", dct_io_bfr[offset+j]);
	#endif
				}
				FPRINTF_MBC (g_mb_idct_res_fp, "\n");
				offset +=8;
			}
		}
		
		/*for (i = 0; i < 8; i++)	// Chroma BLK 4x4
		{
			dct_io_bfr = (int16*)vsp_dct_io_0 + 256 + i*16;
			
			for (j = 7; j >= 0; j--)
			{
#ifdef HEX
				FPRINTF_MBC (g_mb_idct_res_fp, "%04x", dct_io_bfr[j]&0xffff);
#else
				FPRINTF_MBC (g_mb_idct_res_fp, "%4d", dct_io_bfr[j]);
#endif
			}
			FPRINTF_MBC (g_mb_idct_res_fp, "\n");
			for (j = 15; j >= 8; j--)
			{
#ifdef HEX
				FPRINTF_MBC (g_mb_idct_res_fp, "%04x", dct_io_bfr[j]&0xffff);
#else
				FPRINTF_MBC (g_mb_idct_res_fp, "%4d", dct_io_bfr[j]);
#endif
			}
			FPRINTF_MBC (g_mb_idct_res_fp, "\n");			
		}*/
		for (k = 0; k < 4; k++)	// Chroma Normal 8X8
		{
			for (i = 0; i < 4; i++)
			{
				dct_io_bfr = (int16*)vsp_dct_io_0 + (16+2*k)*16;
				
				dct_io_bfr +=1*16;
				for (j = 3; j >= 0; j--)
#ifdef HEX
					FPRINTF_MBC (g_mb_idct_res_fp, "%04x", dct_io_bfr[j+4*i]&0xffff);
#else
					FPRINTF_MBC (g_mb_idct_res_fp, "%4d", dct_io_bfr[j+4*i]);
#endif
				dct_io_bfr -=1*16;				
				for (j = 3; j >= 0; j--)
#ifdef HEX
					FPRINTF_MBC (g_mb_idct_res_fp, "%04x", dct_io_bfr[j+4*i]&0xffff);
#else
					FPRINTF_MBC (g_mb_idct_res_fp, "%4d", dct_io_bfr[j+4*i]);
#endif
				FPRINTF_MBC (g_mb_idct_res_fp, "\n");
			}
		}
	}
	else
	{
		for (k = 0; k < 4; k++)	// Luma
		{
			for (i = 0; i < 4; i++)
			{
				dct_io_bfr = (int16*)vsp_dct_io_0 + g_blk_rec_ord_tbl[4*k]*16;

				dct_io_bfr +=1*16;
				for (j = 3; j >= 0; j--)
#ifdef HEX
					FPRINTF_MBC (g_mb_idct_res_fp, "%04x", dct_io_bfr[j+4*i]&0xffff);
#else
					FPRINTF_MBC (g_mb_idct_res_fp, "%4d", dct_io_bfr[j+4*i]);
#endif
				dct_io_bfr -=1*16;
				for (j = 3; j >= 0; j--)
#ifdef HEX
					FPRINTF_MBC (g_mb_idct_res_fp, "%04x", dct_io_bfr[j+4*i]&0xffff);
#else
					FPRINTF_MBC (g_mb_idct_res_fp, "%4d", dct_io_bfr[j+4*i]);
#endif
				FPRINTF_MBC (g_mb_idct_res_fp, "\n");
				dct_io_bfr +=5*16;				
				for (j = 3; j >= 0; j--)
#ifdef HEX
					FPRINTF_MBC (g_mb_idct_res_fp, "%04x", dct_io_bfr[j+4*i]&0xffff);
#else
					FPRINTF_MBC (g_mb_idct_res_fp, "%4d", dct_io_bfr[j+4*i]);
#endif
				dct_io_bfr -=1*16;
				for (j = 3; j >= 0; j--)
#ifdef HEX
					FPRINTF_MBC (g_mb_idct_res_fp, "%04x", dct_io_bfr[j+4*i]&0xffff);
#else
					FPRINTF_MBC (g_mb_idct_res_fp, "%4d", dct_io_bfr[j+4*i]);
#endif
				FPRINTF_MBC (g_mb_idct_res_fp, "\n");
			}
		}

		for (k = 0; k < 4; k++)	// Chroma
		{
			for (i = 0; i < 4; i++)
			{
				dct_io_bfr = (int16*)vsp_dct_io_0 + (16+2*k)*16;
				
				dct_io_bfr +=1*16;
				for (j = 3; j >= 0; j--)
#ifdef HEX
					FPRINTF_MBC (g_mb_idct_res_fp, "%04x", dct_io_bfr[j+4*i]&0xffff);
#else
					FPRINTF_MBC (g_mb_idct_res_fp, "%4d", dct_io_bfr[j+4*i]);
#endif
				dct_io_bfr -=1*16;				
				for (j = 3; j >= 0; j--)
#ifdef HEX
					FPRINTF_MBC (g_mb_idct_res_fp, "%04x", dct_io_bfr[j+4*i]&0xffff);
#else
					FPRINTF_MBC (g_mb_idct_res_fp, "%4d", dct_io_bfr[j+4*i]);
#endif
				FPRINTF_MBC (g_mb_idct_res_fp, "\n");
			}
		}
	}


	// idct_out.txt
	FPRINTF_MBC (g_idct_out_fp, "//frame_cnt=%d, mb_x=%d, mb_y=%d\n", g_nFrame_enc, g_mode_dcs_ptr->mb_x, g_mode_dcs_ptr->mb_y);
	for (k = 0; k < 4; k++)	// Luma
	{
		for (i = 0; i < 4; i++)
		{
			for (z = 0; z < 4; z++)
			{
				dct_io_bfr = (int16*)vsp_dct_io_0 + g_blk_rec_ord_tbl[4*k+z]*16;
				for (j = 3; j >= 0; j--)
#ifdef HEX
					FPRINTF_MBC (g_idct_out_fp, "%04x", dct_io_bfr[j+4*i]&0xffff);
#else
					FPRINTF_MBC (g_idct_out_fp, "%4d", dct_io_bfr[j+4*i]);
#endif
				FPRINTF_MBC (g_idct_out_fp, "\n");
			}
		}
	}
	
	for (k = 0; k < 4; k++)	// Chroma
	{
		for (i = 0; i < 4; i++)
		{
			for (z = 0; z < 2; z++)
			{
				dct_io_bfr = (int16*)vsp_dct_io_0 + (16+2*k+z)*16;
				for (j = 3; j >= 0; j--)
#ifdef HEX
					FPRINTF_MBC (g_idct_out_fp, "%04x", dct_io_bfr[j+4*i]&0xffff);
#else
					FPRINTF_MBC (g_idct_out_fp, "%4d", dct_io_bfr[j+4*i]);
#endif
				FPRINTF_MBC (g_idct_out_fp, "\n");
			}
		}
	}
	// 14 lines of zeros
	for(i=0; i<14; i++)
		FPRINTF_MBC (g_idct_out_fp, "%016x\n", 0);
}

void PrintfRecPreFilter(uint32 intra_44)
{
	int		i,j;
	uint8 * pMbc = 	(uint8 *)vsp_mbc_out_bfr + 26*4;

	FPRINTF_MBC (g_mb_rec_fp,	"//frame_cnt=%d, mb_x=%d, mb_y=%d\n", g_nFrame_enc, g_mode_dcs_ptr->mb_x, g_mode_dcs_ptr->mb_y);
	FPRINTF_MBC (g_mbc_out_fp,	"//frame_cnt=%d, mb_x=%d, mb_y=%d\n", g_nFrame_enc, g_mode_dcs_ptr->mb_x, g_mode_dcs_ptr->mb_y);

	if(intra_44)
	{
		int	k, z;

		for (z = 0; z < 4; z++) // y-dir
		{
			for (k = 0; k < 4; k++) // x-dir
			{
				for (i = 0; i < 4; i++)
				{
					pMbc = (uint8 *)vsp_mbc_out_bfr + 26*4 + 24*little_endian_map[i] + 4*k + 96*z;
					for (j = 3; j >= 0; j--)
					{
#ifdef HEX
						FPRINTF_MBC (g_mbc_out_fp, "%02x", pMbc[j]);
#else
						FPRINTF_MBC (g_mbc_out_fp, "%4d", pMbc[j]);
#endif
					}
					if(i%2==1)
						FPRINTF_MBC (g_mbc_out_fp, "\n");
				}
			}
		}
		/*for (z = 0; z < 2; z++)		// UV 4x4
		{
			for (k = 0; k < 2; k++)
			{
				for (i = 0; i < 4; i++)
				{
					pMbc = (uint8 *)vsp_mbc_out_bfr + 100*4 + 13*4 + 12*little_endian_map[i] + 4*k + 48*z;
					for (j = 3; j >= 0; j--)
					{
#ifdef HEX
						FPRINTF_MBC (g_mb_rec_fp, "%02x", pMbc[j]);
#else
						FPRINTF_MBC (g_mb_rec_fp, "%4d", pMbc[j]);
#endif
					}
					if(i%2==1)
						FPRINTF_MBC (g_mb_rec_fp, "\n");
				}
			}
		}
		for (z = 0; z < 2; z++)
		{
			for (k = 0; k < 2; k++)
			{
				for (i = 0; i < 4; i++)
				{
					pMbc = (uint8 *)vsp_mbc_out_bfr + 136*4 + 13*4 + 12*little_endian_map[i] + 4*k + 48*z;
					for (j = 3; j >= 0; j--)
					{
#ifdef HEX
						FPRINTF_MBC (g_mb_rec_fp, "%02x", pMbc[j]);
#else
						FPRINTF_MBC (g_mb_rec_fp, "%4d", pMbc[j]);
#endif
					}
					if(i%2==1)
						FPRINTF_MBC (g_mb_rec_fp, "\n");
				}
			}
		}*/
		pMbc = (uint8 *)vsp_mbc_out_bfr + U_OFFSET_BUF_C*4 + 18*4;
		for (i = 0; i < 8; i++)
		{
			for (j = 7; j >= 0; j--)
#ifdef HEX
				FPRINTF_MBC (g_mbc_out_fp, "%02x", pMbc[j]);
#else
				FPRINTF_MBC (g_mbc_out_fp, "%4d", pMbc[j]);
#endif
			FPRINTF_MBC (g_mbc_out_fp, "\n");
			pMbc += 16;
		}
		
		pMbc = (uint8 *)vsp_mbc_out_bfr + V_OFFSET_BUF_C*4 + 18*4;
		for (i = 0; i < 8; i++)
		{
			for (j = 7; j >= 0; j--)
#ifdef HEX
				FPRINTF_MBC (g_mbc_out_fp, "%02x", pMbc[j]);
#else
				FPRINTF_MBC (g_mbc_out_fp, "%4d", pMbc[j]);
#endif
			FPRINTF_MBC (g_mbc_out_fp, "\n");
			pMbc += 16;
		}
	}
	else
	{
		for (i = 0; i < 16; i++)
		{
			for (j = 7; j >= 0; j--)
#ifdef HEX
				FPRINTF_MBC (g_mbc_out_fp, "%02x", pMbc[j]);
#else
				FPRINTF_MBC (g_mbc_out_fp, "%4d", pMbc[j]);
#endif
			FPRINTF_MBC (g_mbc_out_fp, "\n");
			for (j = 15; j >= 8; j--)
#ifdef HEX
				FPRINTF_MBC (g_mbc_out_fp, "%02x", pMbc[j]);
#else
				FPRINTF_MBC (g_mbc_out_fp, "%4d", pMbc[j]);
#endif
			FPRINTF_MBC (g_mbc_out_fp, "\n");
			pMbc += 24;
		}

		pMbc = (uint8 *)vsp_mbc_out_bfr + U_OFFSET_BUF_C*4 + 18*4;
		for (i = 0; i < 8; i++)
		{
			for (j = 7; j >= 0; j--)
#ifdef HEX
				FPRINTF_MBC (g_mbc_out_fp, "%02x", pMbc[j]);
#else
				FPRINTF_MBC (g_mbc_out_fp, "%4d", pMbc[j]);
#endif
			FPRINTF_MBC (g_mbc_out_fp, "\n");
			pMbc += 16;
		}

		pMbc = (uint8 *)vsp_mbc_out_bfr + V_OFFSET_BUF_C*4 + 18*4;
		for (i = 0; i < 8; i++)
		{
			for (j = 7; j >= 0; j--)
#ifdef HEX
				FPRINTF_MBC (g_mbc_out_fp, "%02x", pMbc[j]);
#else
				FPRINTF_MBC (g_mbc_out_fp, "%4d", pMbc[j]);
#endif
			FPRINTF_MBC (g_mbc_out_fp, "\n");
			pMbc += 16;
		}
	}


	// mbc_mb_rec.txt
	pMbc = 	(uint8 *)vsp_mbc_out_bfr + 26*4;
	{
		for (i = 0; i < 16; i++)
		{
			for (j = 7; j >= 0; j--)
#ifdef HEX
				FPRINTF_MBC (g_mb_rec_fp, "%02x", pMbc[j]);
#else
				FPRINTF_MBC (g_mb_rec_fp, "%4d", pMbc[j]);
#endif
			FPRINTF_MBC (g_mb_rec_fp, "\n");
			for (j = 15; j >= 8; j--)
#ifdef HEX
				FPRINTF_MBC (g_mb_rec_fp, "%02x", pMbc[j]);
#else
				FPRINTF_MBC (g_mb_rec_fp, "%4d", pMbc[j]);
#endif
			FPRINTF_MBC (g_mb_rec_fp, "\n");
			pMbc += 24;
		}
		
		pMbc = (uint8 *)vsp_mbc_out_bfr + U_OFFSET_BUF_C*4 + 18*4;
		for (i = 0; i < 8; i++)
		{
			for (j = 7; j >= 0; j--)
#ifdef HEX
				FPRINTF_MBC (g_mb_rec_fp, "%02x", pMbc[j]);
#else
				FPRINTF_MBC (g_mb_rec_fp, "%4d", pMbc[j]);
#endif
			FPRINTF_MBC (g_mb_rec_fp, "\n");
			pMbc += 16;
		}
		
		pMbc = (uint8 *)vsp_mbc_out_bfr + V_OFFSET_BUF_C*4 + 18*4;
		for (i = 0; i < 8; i++)
		{
			for (j = 7; j >= 0; j--)
#ifdef HEX
				FPRINTF_MBC (g_mb_rec_fp, "%02x", pMbc[j]);
#else
				FPRINTF_MBC (g_mb_rec_fp, "%4d", pMbc[j]);
#endif
			FPRINTF_MBC (g_mb_rec_fp, "\n");
			pMbc += 16;
		}
	}
}


void Print_DBK_Out()
{
	uint32 i, j;
	uint32 temp;
	
	int mb_x = (g_glb_reg_ptr->VSP_CTRL0 >> 0) & 0x7f;
	int mb_y = (g_glb_reg_ptr->VSP_CTRL0 >> 8) & 0x7f;
	char * dbk_out_buf = (char*)vsp_dbk_out_bfr;
	
	FPRINTF_DBK (g_dbk_out_fp, "//frame_cnt=%d, mb_x=%d, mb_y=%d\n", g_nFrame_enc, mb_x, mb_y);
	
	for(i=0; i<108; i++)
	{
		for(j=0; j<4; j++)
		{
			temp = dbk_out_buf[i*8+7-j]&0xff;
			FPRINTF_DBK(g_dbk_out_fp, "%02x", temp);
		}			  
		for(j=0; j<4; j++)
		{
			temp = dbk_out_buf[i*8+3-j]&0xff;
			FPRINTF_DBK(g_dbk_out_fp, "%02x", temp);
		}			 
		FPRINTF_DBK(g_dbk_out_fp,"\n");
		if(i==59)
		{
			for(j=0; j<4; j++)
			{
				FPRINTF_DBK(g_dbk_out_fp, "%016x\n", 0);
			}
			//i++;
		}
		if((i==83)||(i==107))
		{
			for(j=0;j<8;j++)
			{
				FPRINTF_DBK(g_dbk_out_fp, "%016x\n", 0);
			}
		}
	}
}


void Print_DBK_Frame(ENC_IMAGE_PARAMS_T *img_ptr)
{
	int width = img_ptr->width;
//#if CROP_1080P
//	uint32 height = IClip(0, 1080, img_ptr->height);
//#else
	int height = img_ptr->height;
//#endif
	uint8 *pRec_Y = img_ptr->pYUVRecFrame->imgY;
	uint8 *pRec_UV = img_ptr->pYUVRecFrame->imgUV;
	int block_x, offset;
	int mb_row;
	int mb_col;
//	uint8 *v_buffer = calloc(width*height/4, 1);

	FPRINTF_DBK (g_fp_rec_frm_tv, "//frame_cnt=%d, width=%d, height=%d\n", g_nFrame_enc, width, height);
	// Print In Frame
	for( mb_row=0; mb_row < height; mb_row++)
	{
		for( mb_col=0; mb_col < width; mb_col+=8)
		{
			for( block_x=7; block_x>=0; block_x--)
			{
				offset = mb_row*width + mb_col + block_x;
#ifdef HEX
				FPRINTF_DBK (g_fp_rec_frm_tv, "%02x", (pRec_Y[offset]&0xff));
#else
				FPRINTF_DBK (g_fp_rec_frm_tv, "%4d", (pRec_Y[offset]));
#endif
			}
			FPRINTF_DBK (g_fp_rec_frm_tv, "\n");
		}
	}

	for( mb_row=0; mb_row < (height/2); mb_row++)
	{
		for( mb_col=0; mb_col < width; mb_col+=8)
		{
			for( block_x=7; block_x>=0; block_x--)
			{
				offset = mb_row*width + mb_col + block_x;
#ifdef HEX
				FPRINTF_DBK (g_fp_rec_frm_tv, "%02x", (pRec_UV[offset]&0xff));
#else
				FPRINTF_DBK (g_fp_rec_frm_tv, "%4d", (pRec_UV[offset]));
#endif
			}
			FPRINTF_DBK (g_fp_rec_frm_tv, "\n");
		}
	}

	/*for( mb_row=0; mb_row < (height/2); mb_row++)
	{
		for( mb_col=0; mb_col < width; mb_col+=16)
		{
			for( block_x=7; block_x>=0; block_x--)
			{
				offset = mb_row*width + mb_col + 2*block_x;
				v_buffer[offset>>1] = pRec_UV[offset+1];
#ifdef HEX
				FPRINTF_DBK (g_fp_rec_frm_tv, "%02x", (pRec_UV[offset]&0xff));
#else
				FPRINTF_DBK (g_fp_rec_frm_tv, "%4d", (pRec_UV[offset]));
#endif
			}
			FPRINTF_DBK (g_fp_rec_frm_tv, "\n");
		}
	}
	for( mb_row=0; mb_row < (height/2); mb_row++)
	{
		for( mb_col=0; mb_col < (width/2); mb_col+=8)
		{
			for( block_x=7; block_x>=0; block_x--)
			{
				offset = mb_row*(width/2) + mb_col + block_x;
#ifdef HEX
				FPRINTF_DBK (g_fp_rec_frm_tv, "%02x", (v_buffer[offset]&0xff));
#else
				FPRINTF_DBK (g_fp_rec_frm_tv, "%4d", (v_buffer[offset]));
#endif
			}
			FPRINTF_DBK (g_fp_rec_frm_tv, "\n");
		}
	}*/
}



void Print_sad_mv_cost(uint32 sad, uint32 mvd_cost, uint8* comment, FILE *fp)
{
	FPRINTF_MEA (fp, "%08x", sad);
	FPRINTF_MEA (fp, "\t\t// %s : SAD\n", comment);
	FPRINTF_MEA (fp, "%08x", mvd_cost);
	FPRINTF_MEA (fp, "\t\t// MVD_Cost\n");
	FPRINTF_MEA (fp, "%08x", sad+mvd_cost);
	FPRINTF_MEA (fp, "\t\t// SAD+MVD_Cost\n");
}

void Print_best_sad_mv_cost(uint32 sad, uint8* comment, FILE *fp)
{
	FPRINTF_MEA (fp, "%08x", sad);
	FPRINTF_MEA (fp, "\t\t// %s\n", comment);
}


void Print_IME_Src_MB(MEA_INFO *mea_info)
{
	int		i, j;
	uint8 * pSrc = (uint8*)mea_src_buf0;
	unsigned __int64 temp = 0;
	uint32 temp2 = 0;

	ENC_MB_MODE_T * mb_info_ptr	 = (g_enc_image_ptr->mb_info + g_enc_image_ptr->curr_mb_nr);
	ENC_MB_MODE_T * tmb_info_ptr =  mb_info_ptr - g_enc_image_ptr->frame_width_in_mbs;

	// MEA Source MB to IEA/FME
	FPRINTF_MEA (g_src_mb_mea_fp, "//frame_cnt=%d, mb_x=%d, mb_y=%d\n", g_nFrame_enc, g_mode_dcs_ptr->mb_x, g_mode_dcs_ptr->mb_y);

	for (i = 0; i < 32; i++)	// Y
	{
		for (j = 7; j >= 0; j--)
#ifdef HEX
			FPRINTF_MEA (g_src_mb_mea_fp, "%02x", pSrc[j]);
#else
		FPRINTF_MEA (g_src_mb_mea_fp, "%4d", pSrc[j]);
#endif
		pSrc +=8;
		FPRINTF_MEA (g_src_mb_mea_fp, "\n");
	}
	//pSrc = (uint8*)mea_src_buf0 + 256;
	for (i = 0; i < 16; i++)
	{
		for (j = 3; j >= 0; j--)	// V
#ifdef HEX
			FPRINTF_MEA (g_src_mb_mea_fp, "%02x", pSrc[64+j]);
#else
		FPRINTF_MEA (g_src_mb_mea_fp, "%4d", pSrc[64+j]);
#endif
		for (j = 3; j >= 0; j--)	// U
#ifdef HEX
			FPRINTF_MEA (g_src_mb_mea_fp, "%02x", pSrc[j]);
#else
		FPRINTF_MEA (g_src_mb_mea_fp, "%4d", pSrc[j]);
#endif
		pSrc +=4;
		FPRINTF_MEA (g_src_mb_mea_fp, "\n");
	}
	
	temp = 0;
#if 0
	if(g_mode_dcs_ptr->mb_type <= INTRA_MB_4X4)
	{
		//temp |= ((__int64)mea_info->partition_mode << 1);	// partition_mode
		temp |= ((__int64)(g_enc_image_ptr->mb_y&0x7f) << 2);	// cur_mb_y[6:0]
		temp |= ((__int64)(g_enc_image_ptr->mb_x&0x7f) << 9);	// cur_mb_x[6:0]
		//temp |= ((__int64)(mea_info->mv_valid&0xff) << 16);		// mv_valid[7:0]
		if(g_mode_dcs_ptr->top_avail)
		{
			if(tmb_info_ptr->type==I_16x16)
			{
				temp2 |= ((tmb_info_ptr->i_intra16x16_pred_mode & 0x3) << 0);
				temp2 |= ((tmb_info_ptr->i_intra16x16_pred_mode & 0x3) << 2);
				temp2 |= ((tmb_info_ptr->i_intra16x16_pred_mode & 0x3) << 4);
				temp2 |= ((tmb_info_ptr->i_intra16x16_pred_mode & 0x3) << 6);
			}
			else if (tmb_info_ptr->type==I_4x4)
			{
				temp2 |= ((tmb_info_ptr->intra4x4_pred_mode[12] & 0x3) << 0);
				temp2 |= ((tmb_info_ptr->intra4x4_pred_mode[13] & 0x3) << 2);
				temp2 |= ((tmb_info_ptr->intra4x4_pred_mode[14] & 0x3) << 4);
				temp2 |= ((tmb_info_ptr->intra4x4_pred_mode[15] & 0x3) << 6);
			}
			temp |= ((__int64)(temp2&0xff) << 24);		// top_mb_pred_mode[7:0]
			temp |= ((__int64)(g_mode_dcs_ptr->top_avail&0x1) << 32);	// top_mb_avail
			temp |= ((__int64)(((tmb_info_ptr->type==I_4x4) || (tmb_info_ptr->type==P_8x8))&0x1) << 33);	// top_mb_partition, 0:16x16, 1:4x4 or 8x8
			temp |= ((__int64)((tmb_info_ptr->type<=I_16x16)&0x1) << 34); // top_mb_is_intra
		}
		temp |= ((__int64)(g_mode_dcs_ptr->lambda&0x1f) << 35); // lambda[4:0]
		temp |= ((__int64)(mb_info_ptr->qp&0x3f) << 40); // QP[5:0]
		// 2'b0
		//temp |= ((__int64)(mea_info->total_cost_min&0xfff) << 48); // total_cost_min[11:0]
		FPRINTF_MEA (g_src_mb_mea_fp, "%016I64x\n", (__int64)temp);
		
		temp = 0;
		/*temp |= ((__int64)(mea_info->imv_x_blk3&0x7f) << 0);
		temp |= ((__int64)(mea_info->imv_y_blk3&0x3f) << 7);
		temp |= ((__int64)(mea_info->imv_x_blk2&0x7f) << 16);
		temp |= ((__int64)(mea_info->imv_y_blk2&0x3f) << 23);
		temp |= ((__int64)(mea_info->imv_x_blk1&0x7f) << 32);
		temp |= ((__int64)(mea_info->imv_y_blk1&0x3f) << 39);
		temp |= ((__int64)(mea_info->imv_x_blk0&0x7f) << 48);
		temp |= ((__int64)(mea_info->imv_y_blk0&0x3f) << 55);*/
		FPRINTF_MEA (g_src_mb_mea_fp, "%016I64x\n", (__int64)temp);
	}
	else
#endif
	{
		temp |= ((__int64)mea_info->partition_mode << 1);	// partition_mode
		temp |= ((__int64)(g_enc_image_ptr->mb_y&0x7f) << 2);	// cur_mb_y[6:0]
		temp |= ((__int64)(g_enc_image_ptr->mb_x&0x7f) << 9);	// cur_mb_x[6:0]
		temp |= ((__int64)(mea_info->mv_valid&0xff) << 16);		// mv_valid[7:0]
		if(g_mode_dcs_ptr->top_avail)
		{
			if(tmb_info_ptr->type==I_16x16)
			{
				temp2 |= ((tmb_info_ptr->i_intra16x16_pred_mode & 0x3) << 0);
				temp2 |= ((tmb_info_ptr->i_intra16x16_pred_mode & 0x3) << 2);
				temp2 |= ((tmb_info_ptr->i_intra16x16_pred_mode & 0x3) << 4);
				temp2 |= ((tmb_info_ptr->i_intra16x16_pred_mode & 0x3) << 6);
			}
			else if (tmb_info_ptr->type==I_4x4)
			{
				temp2 |= ((tmb_info_ptr->intra4x4_pred_mode[12] & 0x3) << 0);
				temp2 |= ((tmb_info_ptr->intra4x4_pred_mode[13] & 0x3) << 2);
				temp2 |= ((tmb_info_ptr->intra4x4_pred_mode[14] & 0x3) << 4);
				temp2 |= ((tmb_info_ptr->intra4x4_pred_mode[15] & 0x3) << 6);
			}
			temp |= ((__int64)(temp2&0xff) << 24);		// top_mb_pred_mode[7:0]
			temp |= ((__int64)(g_mode_dcs_ptr->top_avail&0x1) << 32);	// top_mb_avail
			temp |= ((__int64)(((tmb_info_ptr->type==I_4x4) || (tmb_info_ptr->type==P_8x8))&0x1) << 33);	// top_mb_partition, 0:16x16, 1:4x4 or 8x8
			temp |= ((__int64)((tmb_info_ptr->type<=I_16x16)&0x1) << 34); // top_mb_is_intra
		}
		temp |= ((__int64)(g_mode_dcs_ptr->lambda&0x1f) << 35); // lambda[4:0]
		temp |= ((__int64)(mb_info_ptr->qp&0x3f) << 40); // QP[5:0]
		// 2'b0
		temp |= ((__int64)(mea_info->total_cost_min&0xfff) << 48); // total_cost_min[11:0]
		FPRINTF_MEA (g_src_mb_mea_fp, "%016I64x\n", (__int64)temp);

		temp = 0;
		temp |= ((__int64)(mea_info->imv_x_blk3&0x7f) << 0);
		temp |= ((__int64)(mea_info->imv_y_blk3&0x3f) << 7);
		temp |= ((__int64)(mea_info->imv_x_blk2&0x7f) << 16);
		temp |= ((__int64)(mea_info->imv_y_blk2&0x3f) << 23);
		temp |= ((__int64)(mea_info->imv_x_blk1&0x7f) << 32);
		temp |= ((__int64)(mea_info->imv_y_blk1&0x3f) << 39);
		temp |= ((__int64)(mea_info->imv_x_blk0&0x7f) << 48);
		temp |= ((__int64)(mea_info->imv_y_blk0&0x3f) << 55);
		FPRINTF_MEA (g_src_mb_mea_fp, "%016I64x\n", (__int64)temp);
	}
}

void Print_IME_block(uint32 iframe)
{
	int32 i, j, k;
	int32 start_pos_x = g_mode_dcs_ptr->mb_x*16;
	int32 start_pos_y = g_mode_dcs_ptr->mb_y*16;
	uint8 *ref_frm_y_ptr = (uint8 *)g_ref_frame[0];

	int32 mb_num_x = (g_glb_reg_ptr->VSP_CFG1 & 0x1ff);
	int32 mb_num_y = ((g_glb_reg_ptr->VSP_CFG1 >> 12) & 0x1ff);
	int32 width = mb_num_x * MB_SIZE;
	int32 height = mb_num_y * MB_SIZE;

	int32 x_min,x_max;
	int32 y_min,y_max;
	int32 x_win_min,x_win_max;
	int32 y_win_min,y_win_max;
	int32 x,y;
	int32 x_cor,y_cor;
	uint8 range_valid_n;

	x_min = 0; x_max = width - 1;
	y_min = 0; y_max = height - 1;
	x_win_min = IClip(x_min, x_max, start_pos_x-SEARCH_RANGE_X);
	x_win_max = IClip(x_min, x_max, start_pos_x+SEARCH_RANGE_X+MB_SIZE-1);
	y_win_min = IClip(y_min, y_max, start_pos_y-SEARCH_RANGE_Y);
	y_win_max = IClip(y_min, y_max, start_pos_y+SEARCH_RANGE_Y+MB_SIZE-1);

	FPRINTF_MEA (g_mb_ime_fp, "//frame_cnt=%d, mb_x=%d, mb_y=%d\n", g_nFrame_enc, g_mode_dcs_ptr->mb_x, g_mode_dcs_ptr->mb_y);

	if(iframe)
	{
		for (i=0; i<4; i++)
		{
			for (j=0; j<14; j++)
			{
				for (k=13; k>=0; k--)
				{
#ifdef HEX
					FPRINTF_MEA (g_mb_ime_fp, "%2s", "xx");
#else
					FPRINTF_MEA (g_mb_ime_fp, "%4s ", "xxx");
#endif
				}
				FPRINTF_MEA (g_mb_ime_fp, "\n");
			}
		}
	}
	else
	{
		for (i=0; i<4; i++)
		{
			if(g_mode_dcs_ptr->mb_type == INTER_MB_16X16)
			{
				x_cor = (start_pos_x + (i%2)*8 + (g_16x16_sad_mv[MVX]>>2)) - 3;
				y_cor = (start_pos_y + (i/2)*8 + (g_16x16_sad_mv[MVY]>>2)) - 3;
			}
			else if(g_mode_dcs_ptr->mb_type == INTER_MB_8X8)
			{
				x_cor = (start_pos_x + (i%2)*8 + (g_8x8_sad_mv[i][MVX]>>2)) - 3;
				y_cor = (start_pos_y + (i/2)*8 + (g_8x8_sad_mv[i][MVY]>>2)) - 3;
			}
						
			for (j=0; j<14; j++)
			{
				y = y_cor+j;
				//y = IClip(y_min, y_max, y_cor+j);	// 1. duplicate boundary pixels
				for (k=13; k>=0; k--)
				{
					x= x_cor+k;
					range_valid_n = (y < y_min) || (y > y_max) || (x < x_min) || (x > x_max) || (y < y_win_min) || (y > y_win_max) || (x < x_win_min) || (x > x_win_max);
					//x = IClip(x_min, x_max, x_cor+k);	// 1. duplicate boundary pixels
					if( range_valid_n ) // 2. fill "x" when not available
#ifdef HEX
						FPRINTF_MEA (g_mb_ime_fp, "%2s", "xx");
#else
						FPRINTF_MEA (g_mb_ime_fp, "%4s ", "xxx");
#endif
					else
#ifdef HEX
						FPRINTF_MEA (g_mb_ime_fp, "%02x", ref_frm_y_ptr[x+y*width]);
#else
						FPRINTF_MEA (g_mb_ime_fp, "%4d ", ref_frm_y_ptr[x+y*width]);
#endif
				}
				FPRINTF_MEA (g_mb_ime_fp, "\n");
			}
		}
	}
}

void Print_SrcRef_Frame(ENC_IMAGE_PARAMS_T *img_ptr)
{
	int32 i, j, k;
	uint8 *src_y	= (uint8 *)img_ptr->pYUVSrcFrame->imgY;
	uint8 *src_uv	= (uint8 *)img_ptr->pYUVSrcFrame->imgUV;
	uint8 *ref_y	= (uint8 *)img_ptr->pYUVRefFrame->imgY;
	uint8 *ref_uv	= (uint8 *)img_ptr->pYUVRefFrame->imgUV;

	// Separate Source
	unsigned char s[100];
	FILE *y_fp;
	FILE *uv_fp;
	sprintf(s, "../../test_vectors/src_yuv/src_y_%d.txt", g_nFrame_enc);
    assert(NULL != (y_fp = fopen(s, "w")));
	sprintf(s, "../../test_vectors/src_yuv/src_uv_%d.txt", g_nFrame_enc);
    assert(NULL != (uv_fp = fopen(s, "w")));

	//FPRINTF_MEA (g_fm_src_y_fp, "//frame_cnt=%d\n", g_nFrame_enc);
	//FPRINTF_MEA (g_fm_src_uv_fp, "//frame_cnt=%d\n", g_nFrame_enc);

	// Src_Y
	for (j=0; j<img_ptr->height; j++)
	{
		for (i=0; i<(int)g_input->ori_width; i+=8)
		{
			for (k=7; k>=0; k--)
			{
				if( ((i+k)<(int)img_ptr->crop_x) || ((i+k)>=(int)(img_ptr->crop_x+img_ptr->width)) )
				{
					FPRINTF_MEA (g_fm_src_y_fp, "%02x", 0);
					FPRINTF_MEA (y_fp, "%02x", 0);
				}
				else
				{
					FPRINTF_MEA (g_fm_src_y_fp, "%02x", src_y[i+k-img_ptr->crop_x]);
					FPRINTF_MEA (y_fp, "%02x", src_y[i+k-img_ptr->crop_x]);
				}
			}
			FPRINTF_MEA (g_fm_src_y_fp, "\n");
			FPRINTF_MEA (y_fp, "\n");
		}
		src_y += img_ptr->width;
	}
	// Src_UV
	for (j=0; j<(img_ptr->height>>1); j++)
	{
		for (i=0; i<(int)g_input->ori_width; i+=8)
		{
			for (k=7; k>=0; k--)
			{
				if( ((i+k)<(int)img_ptr->crop_x) || ((i+k)>=(int)(img_ptr->crop_x+img_ptr->width)) )
				{
					FPRINTF_MEA (g_fm_src_uv_fp, "%02x", 0);
					FPRINTF_MEA (uv_fp, "%02x", 0);
				}
				else
				{
					FPRINTF_MEA (g_fm_src_uv_fp, "%02x", src_uv[i+k-img_ptr->crop_x]);
					FPRINTF_MEA (uv_fp, "%02x", src_uv[i+k-img_ptr->crop_x]);
				}
			}
			FPRINTF_MEA (g_fm_src_uv_fp, "\n");
			FPRINTF_MEA (uv_fp, "\n");
		}
		src_uv += img_ptr->width;
	}
	fclose(y_fp);
	fclose(uv_fp);

	// Ref_YUV
	if(img_ptr->pYUVSrcFrame->i_type != SLICE_TYPE_I)
	{
		for (j=0; j<img_ptr->frame_size_in_mbs*32; j++)
		{
			for (i=7; i>=0; i--)
				FPRINTF_MEA (g_fm_ref_y_fp, "%02x", ref_y[i]);
			FPRINTF_MEA (g_fm_ref_y_fp, "\n");
			ref_y +=8;
		}
		for (j=0; j<img_ptr->frame_size_in_mbs*16; j++)
		{
			for (i=7; i>=0; i--)
				FPRINTF_MEA (g_fm_ref_uv_fp, "%02x", ref_uv[i]);
			FPRINTF_MEA (g_fm_ref_uv_fp, "\n");
			ref_uv +=8;
		}
	}
}


void Print_MEA_CFG(ENC_IMAGE_PARAMS_T *img_ptr, uint32 qp)
{
	//uint i;
//	FPRINTF_MEA (g_mea_config_fp, "//frame_cnt=%d, slice_nr=%d\n", g_nFrame_enc, img_ptr->slice_nr);
	FPRINTF_MEA (g_mea_config_fp, "work_mode=%x\n", 1);	// 1 : encode, 0 : decode
	FPRINTF_MEA (g_mea_config_fp, "standard=%x\n", 4);	// 2:VP8, 4:H264
	FPRINTF_MEA (g_mea_config_fp, "mb_x_max=%x\n", g_input->pic_width>>4);
	FPRINTF_MEA (g_mea_config_fp, "mb_y_max=%x\n", g_input->pic_height>>4);
	FPRINTF_MEA (g_mea_config_fp, "frm_type=%x\n", (img_ptr->pYUVSrcFrame->i_type==SLICE_TYPE_I)?0:1);
	FPRINTF_MEA (g_mea_config_fp, "Slice_Qp=%x\n", qp);
	FPRINTF_MEA (g_mea_config_fp, "MB_Qp=%x\n", qp);
	FPRINTF_MEA (g_mea_config_fp, "srch_16_max=%x\n", (MEA_SEARCH_ROUND_16));
	FPRINTF_MEA (g_mea_config_fp, "srch_8_max=%x\n", (MEA_SEARCH_ROUND_8));
//	FPRINTF_MEA (g_mea_config_fp, "srch_16_max=%x\n", (MEA_SEARCH_ROUND_16 - (g_nFrame_enc%3)));
//	FPRINTF_MEA (g_mea_config_fp, "srch_8_max=%x\n", (MEA_SEARCH_ROUND_8 + (g_nFrame_enc%3)));
	FPRINTF_MEA (g_mea_config_fp, "deblocking_eb=%x\n", 1);
	FPRINTF_MEA (g_mea_config_fp, "mca_rounding=%x\n", 0);
	FPRINTF_MEA (g_mea_config_fp, "slice_ver_num=%x\n", img_ptr->frame_height_in_mbs / SLICE_MB);	// Y-dir mb line numbers of one slice
//	FPRINTF_MEA (g_mea_config_fp, "skip_threshold=%x\n", MEA_SKIP_THRESHOLD);
}


void Print_PPA_CFG(ENC_IMAGE_PARAMS_T *img_ptr, uint32 qp)
{
	uint i;
//	FPRINTF_PPA (g_ppa_config_fp, "//frame_cnt=%d, slice_nr=%d\n", g_nFrame_enc, img_ptr->slice_nr);
	FPRINTF_PPA (g_ppa_config_fp, "work_mode = %x\n", 1);	// 1 : encode, 0 : decode
	FPRINTF_PPA (g_ppa_config_fp, "standard = %x\n", 4);	// 2:VP8, 4:H264
	FPRINTF_PPA (g_ppa_config_fp, "Slice_first_mb_x = %x\n", img_ptr->sh.i_first_mb % img_ptr->frame_width_in_mbs);
	FPRINTF_PPA (g_ppa_config_fp, "Slice_first_mb_y = %x\n", img_ptr->sh.i_first_mb / img_ptr->frame_width_in_mbs);
	FPRINTF_PPA (g_ppa_config_fp, "Slice_last_mb_y = %x\n", IClip(0, img_ptr->frame_height_in_mbs-1, (img_ptr->sh.i_first_mb/img_ptr->frame_width_in_mbs)+(img_ptr->frame_height_in_mbs/SLICE_MB)-1));
	FPRINTF_PPA (g_ppa_config_fp, "Picwidth_in_mb = %x\n", img_ptr->frame_width_in_mbs);
	FPRINTF_PPA (g_ppa_config_fp, "Picheight_in_mb = %x\n", img_ptr->frame_height_in_mbs);
	FPRINTF_PPA (g_ppa_config_fp, "Slice_type = %x\n", (img_ptr->pYUVSrcFrame->i_type==SLICE_TYPE_I)?0:1);
	FPRINTF_PPA (g_ppa_config_fp, "Slice_num = %x\n", img_ptr->slice_nr/*img_ptr->sh.i_first_mb / img_ptr->slice_mb*/);
	FPRINTF_PPA (g_ppa_config_fp, "constrained_intra_pred_flag = %x\n", img_ptr->pps->b_constrained_intra_pred);
	FPRINTF_PPA (g_ppa_config_fp, "direct_spatial_mv_pred_flag = %x\n", 0);
	FPRINTF_PPA (g_ppa_config_fp, "direct_8x8_inference_flag = %x\n", 0);
	FPRINTF_PPA (g_ppa_config_fp, "chroma_qp_index_offset = %x\n", img_ptr->i_chroma_qp_offset);
	FPRINTF_PPA (g_ppa_config_fp, "second_chroma_qp_index_offset = %x\n", 0);
	FPRINTF_PPA (g_ppa_config_fp, "SliceQPy = %x\n", qp);
	FPRINTF_PPA (g_ppa_config_fp, "disable_deblocking_filter_idc = %x\n", img_ptr->sh.i_disable_deblocking_filter_idc);
	FPRINTF_PPA (g_ppa_config_fp, "weighted_bipred_idc = %x\n", 0);
	FPRINTF_PPA (g_ppa_config_fp, "Curr_POC = %x\n", img_ptr->sh.i_poc_lsb);
	FPRINTF_PPA (g_ppa_config_fp, "ppa_info_vdb_en = %x\n", 0);
	FPRINTF_PPA (g_ppa_config_fp, "skip_threshold = %x\n", MEA_SKIP_THRESHOLD);
	FPRINTF_PPA (g_ppa_config_fp, "transform_8x8_mode_flag = %x\n", 0);

	for(i=0; i<6; i++)
		FPRINTF_PPA (g_ppa_config_fp, "0\n", 0);
}


void Print_IEA_Mode(ENC_IMAGE_PARAMS_T *img_ptr, ENC_MB_MODE_T *mb_info_ptr, ENC_MB_CACHE_T *mb_cache_ptr)
{
	uint32 temp=0;
	ENC_MB_MODE_T * lmb_info_ptr  = mb_info_ptr-1;
	ENC_MB_MODE_T * tmb_info_ptr  = mb_info_ptr - img_ptr->frame_width_in_mbs;

	FPRINTF_IEA (g_mb_iea_in_fp, "//frame_cnt=%d, mb_x=%d, mb_y=%d\n", g_nFrame_enc, g_mode_dcs_ptr->mb_x, g_mode_dcs_ptr->mb_y);
	temp |= (1 << 0);	// [0]:Ipred_y16_dc
	temp |= (1 << 1);	// [1]:Ipred_y16_h
	temp |= (1 << 2);	// [2]:Ipred_y16_v
	temp |= (1 << 3);	// [3]:Ipred_y4_dc
	temp |= (1 << 4);	// [4]:Ipred_y4_h
	temp |= (1 << 5);	// [5]:Ipred_y4_v
	temp |= (0 << 6);	// [6]:Ipred_y8_dc
	temp |= (0 << 7);	// [7]:Ipred_y8_h
	temp |= (0 << 8);	// [8]:Ipred_y8_v
	FPRINTF_IEA (g_mb_iea_in_fp, "%08x\n", temp);

	temp = 0;
	temp |= (mb_cache_ptr->mb_avail_b << 0);	// [0]:Top_Avail
	temp |= (mb_cache_ptr->mb_avail_a << 1);	// [1]:Left_Avail
	if(mb_cache_ptr->mb_avail_a)
		temp |= ((lmb_info_ptr->type<=I_16x16) << 2);	// [2]:Left_mb_is_intra

	if(mb_cache_ptr->mb_avail_b)
	{
		temp |= ((tmb_info_ptr->type<=I_16x16) << 3);	// [3]:Top_mb_is_intra
		temp |= ((tmb_info_ptr->type==I_16x16) << 4);	// [4]:top_mb_ipred_partition
		if(tmb_info_ptr->type==I_16x16)
			temp |= (tmb_info_ptr->i_intra16x16_pred_mode << 8);	// [15:8]:Top_mb_ipred_mode
		else if (tmb_info_ptr->type==I_4x4)
		{
			temp |= ((tmb_info_ptr->intra4x4_pred_mode[12] & 0x3) << 8);	// [15:8]:Top_mb_ipred_mode
			temp |= ((tmb_info_ptr->intra4x4_pred_mode[13] & 0x3) << 10);
			temp |= ((tmb_info_ptr->intra4x4_pred_mode[14] & 0x3) << 12);
			temp |= ((tmb_info_ptr->intra4x4_pred_mode[15] & 0x3) << 14);
		}
	}
	FPRINTF_IEA (g_mb_iea_in_fp, "%08x\n", temp);

	temp = 0;
	temp |= (g_mode_dcs_ptr->lambda << 0);	// [4:0]:Lambda
	temp |= (img_ptr->mb_x << 8);	// [14:8]:Mb_x_id
	FPRINTF_IEA (g_mb_iea_in_fp, "%08x\n", temp);
}


void Print_IEA_SAD(IEA_SAD *iea_sad)
{
	uint32 i;
	unsigned __int64 temp = 0;

	FPRINTF_IEA (g_mb_iea_out_fp, "//frame_cnt=%d, mb_x=%d, mb_y=%d\n", g_nFrame_enc, g_mode_dcs_ptr->mb_x, g_mode_dcs_ptr->mb_y);

	FPRINTF_IEA (g_mb_iea_out_fp, "%012I64x\n", (__int64)iea_sad->dc_pred_16);	// sad_16x16_dc_ipred[7:0]
	FPRINTF_IEA (g_mb_iea_out_fp, "%012I64x\n", (__int64)iea_sad->sad_y_16x16[0]);	// sad_16x16_dc[16:0]
	FPRINTF_IEA (g_mb_iea_out_fp, "%012I64x\n", (__int64)iea_sad->sad_y_16x16[1]);	// sad_16x16_h[16:0]
	FPRINTF_IEA (g_mb_iea_out_fp, "%012I64x\n", (__int64)iea_sad->sad_y_16x16[2]);	// sad_16x16_v[16:0]

	for(i=0; i<16; i++)
	{
		temp = 0;
		temp |= ((__int64)iea_sad->sad_y_4x4[i][0] << 0);
		temp |= ((__int64)iea_sad->sad_y_4x4[i][1] << 16);
		temp |= ((__int64)iea_sad->sad_y_4x4[i][2] << 32);
		FPRINTF_IEA (g_mb_iea_out_fp, "%012I64x\n", temp);
	}
	FPRINTF_IEA (g_mb_iea_out_fp, "%012I64x\n", (__int64)iea_sad->sad_y_4x4_min);	// sad_4x4_min[16:0]

	FPRINTF_IEA (g_mb_iea_out_fp, "%012I64x\n", (__int64)iea_sad->sad_c_8x8[0]);	// sad_8x8_dc[16:0]
	FPRINTF_IEA (g_mb_iea_out_fp, "%012I64x\n", (__int64)iea_sad->sad_c_8x8[1]);	// sad_8x8_h[16:0]
	FPRINTF_IEA (g_mb_iea_out_fp, "%012I64x\n", (__int64)iea_sad->sad_c_8x8[2]);	// sad_8x8_v[16:0]

	temp = 0;
	if(iea_sad->iea_ipred_type==1)
	{
		temp |= (g_mode_dcs_ptr->intra16_mode_y << 0);
	}
	else
	{
		for(i=0; i<16; i++)
			temp |= ((__int64)(g_mode_dcs_ptr->intra4x4_pred_mode[i]& 0x3) << (2*i));
	}
	temp |= ((__int64)g_mode_dcs_ptr->intra_mode_c << 32);
	temp |= ((__int64)iea_sad->iea_ipred_type << 34);
	FPRINTF_IEA (g_mb_iea_out_fp, "%012I64x\n", temp);
}


void PrintfVLCParaBuf(ENC_IMAGE_PARAMS_T *img_ptr, ENC_MB_MODE_T *mb_info_ptr, ENC_MB_CACHE_T *mb_cache_ptr)
{
	int i;
	uint32 temp1=0;
	uint32 temp2=0;

#ifdef VLC_TV_SPLIT
	FPRINTF_VLC (g_vlc_para_fp, "//frame_cnt=%d, mb_x=%d, mb_y=%d\n", g_nFrame_enc, g_mode_dcs_ptr->mb_x, g_mode_dcs_ptr->mb_y);
#else
	FPRINTF_PPA (g_mca_para_fp, "//frame_cnt=%d, mb_x=%d, mb_y=%d\n", g_nFrame_enc, g_mode_dcs_ptr->mb_x, g_mode_dcs_ptr->mb_y);
#endif
	//temp |= (mb_info_ptr->type << 0);	// [2:0]: mb_type
	temp1 |= (img_ptr->mb_y << 4);		// [10:4]: cur_mb_y
	temp1 |= (img_ptr->mb_x << 12);		// [18:12]: cur_mb_x
	temp1 |= ((mb_info_ptr->type==P_SKIP) << 20); // [20]: is_skipped
#ifdef RC_BU
	if (img_ptr->curr_mb_nr == img_ptr->sh.i_first_mb)
		temp1 |= (((mb_info_ptr->qp - img_ptr->qp) & 0x3f) << 22);	// [27:22]: mb_qp_delta
	else
		temp1 |= (((mb_info_ptr->qp - prev_qp) & 0x3f) << 22);	// [27:22]: mb_qp_delta
#else
	temp1 |= (((mb_info_ptr->qp - img_ptr->qp) & 0x3f) << 22);	// [27:22]: mb_qp_delta
#endif
	temp2 |= ((mb_info_ptr->i_cbp_luma & 0xf) << 0);	// [5:0]: CBP_BLK[3:0]
	temp2 |= ((mb_info_ptr->i_cbp_chroma & 0x3) << 4);	// [5:0]: CBP_BLK[5:4]
	// 1'b0 transform_size_8x8_flag[6]
	temp2 |= ((((img_ptr->curr_mb_nr+1) == img_ptr->frame_size_in_mbs)||img_ptr->slice_end) << 8);	// is_last_mb[8]
	temp2 |= ((mb_info_ptr->mb_type_val&0x3f) << 10);	// [15:10] mb_type_val
#ifdef VLC_TV_SPLIT
	FPRINTF_VLC (g_vlc_para_fp, "%07x\n", temp1);
	FPRINTF_VLC (g_vlc_para_fp, "%07x\n", temp2);
#else
	// g_mca_para_fp
	FPRINTF_PPA (g_mca_para_fp, "%07x", temp2);
	FPRINTF_PPA (g_mca_para_fp, "%07x\n", temp1);
#endif

	temp1 = 0;
	temp2 = 0;

	if (mb_info_ptr->type==I_16x16)
	{
		temp1 |= mb_info_ptr->i_intra16x16_pred_mode;	// [1:0]: Intra16x16_luma_pred_mode
		temp1 |= ((mb_info_ptr->i_chroma_pred_mode & 0x3) << 16); // [17:16]: intra_chroma_pred_mode
		temp2 |= (((img_ptr->sh.i_type==SLICE_TYPE_P)?1:0) << 2); // [3:2]: slice_type, 0=I_SLICE, 1=P_SLICE, 2=B_SLICE
#ifdef VLC_TV_SPLIT
		FPRINTF_VLC (g_vlc_para_fp, "%07x\n", temp1);
		FPRINTF_VLC (g_vlc_para_fp, "%07x\n", temp2);
		for(i=0; i<46; i++)
			FPRINTF_VLC (g_vlc_para_fp, "%07x\n", 0);
#else
		// g_mca_para_fp
		FPRINTF_PPA (g_mca_para_fp, "%07x", temp2);
		FPRINTF_PPA (g_mca_para_fp, "%07x\n", temp1);
		for(i=0; i<23; i++)
			FPRINTF_PPA (g_mca_para_fp, "%014x\n", 0);
#endif
	}
	else if(mb_info_ptr->type==I_4x4)
	{
		temp1 |= ((mb_info_ptr->prev_intra4x4_pred_mode_flag & 0xffff) << 0 ); // [15:0]: prev_intra4x4_pred_mode_flag
		temp1 |= ((mb_info_ptr->i_chroma_pred_mode & 0x3) << 16); // [17:16]: intra_chroma_pred_mode
		temp2 |= (((img_ptr->sh.i_type==SLICE_TYPE_P)?1:0) << 2); // [3:2]: slice_type, 0=I_SLICE, 1=P_SLICE, 2=B_SLICE
#ifdef VLC_TV_SPLIT
		FPRINTF_VLC (g_vlc_para_fp, "%07x\n", temp1);
		FPRINTF_VLC (g_vlc_para_fp, "%07x\n", temp2);
		for(i=0; i<43; i++)
			FPRINTF_VLC (g_vlc_para_fp, "%07x\n", 0);
		FPRINTF_VLC (g_vlc_para_fp, "%07x\n", mb_info_ptr->rem_intra4x4_pred_mode[0]); // [27:0]: rem_intra4x4_pred_mode
		FPRINTF_VLC (g_vlc_para_fp, "%07x\n", mb_info_ptr->rem_intra4x4_pred_mode[1]); // [27:0]: rem_intra4x4_pred_mode
		FPRINTF_VLC (g_vlc_para_fp, "%07x\n", mb_info_ptr->rem_intra4x4_pred_mode[2]); // [27:0]: rem_intra4x4_pred_mode
#else
		// g_mca_para_fp
		FPRINTF_PPA (g_mca_para_fp, "%07x", temp2);
		FPRINTF_PPA (g_mca_para_fp, "%07x\n", temp1);
		for(i=0; i<21; i++)
			FPRINTF_PPA (g_mca_para_fp, "%014x\n", 0);
		FPRINTF_PPA (g_mca_para_fp, "%07x", mb_info_ptr->rem_intra4x4_pred_mode[0]); // [27:0]: rem_intra4x4_pred_mode
		FPRINTF_PPA (g_mca_para_fp, "%07x\n", 0);
		FPRINTF_PPA (g_mca_para_fp, "%07x", mb_info_ptr->rem_intra4x4_pred_mode[2]); // [27:0]: rem_intra4x4_pred_mode
		FPRINTF_PPA (g_mca_para_fp, "%07x\n", mb_info_ptr->rem_intra4x4_pred_mode[1]); // [27:0]: rem_intra4x4_pred_mode
#endif
	}
	else if(mb_info_ptr->type!=P_SKIP)
	{
#ifdef VLC_TV_SPLIT
		//FPRINTF_VLC (g_vlc_para_fp, "%07x\n", temp);	// Ref_idx_l1 is fixed
		temp1 |= (((img_ptr->sh.i_type==SLICE_TYPE_P)?1:0) << 2); // [3:2]: slice_type, 0=I_SLICE, 1=P_SLICE, 2=B_SLICE
		FPRINTF_VLC (g_vlc_para_fp, "%07x\n", 0);	// Ref_idx_l0 & sub_mbmode are fixed
		FPRINTF_VLC (g_vlc_para_fp, "%07x\n", temp1);
		FPRINTF_VLC (g_vlc_para_fp, "%07x\n", 0);	// reserved

		temp1 = 0;
		temp1 |= ((mb_info_ptr->mvd_8x8[0][1] & 0x1fff) << 14);	// [26:14]: mvd_l0_0_y
		temp1 |= ((mb_info_ptr->mvd_8x8[0][0] & 0x3fff) << 0);	// [13:0]:	mvd_l0_0_x
		FPRINTF_VLC (g_vlc_para_fp, "%07x\n", temp1);
		FPRINTF_VLC (g_vlc_para_fp, "%07x\n", temp1);
		FPRINTF_VLC (g_vlc_para_fp, "%07x\n", temp1);
		FPRINTF_VLC (g_vlc_para_fp, "%07x\n", temp1);
		temp1 = 0;

		temp1 |= ((mb_info_ptr->mvd_8x8[1][1] & 0x1fff) << 14);	// [26:14]: mvd_l0_1_y
		temp1 |= ((mb_info_ptr->mvd_8x8[1][0] & 0x3fff) << 0);	// [13:0]:	mvd_l0_1_x
		FPRINTF_VLC (g_vlc_para_fp, "%07x\n", temp1);
		FPRINTF_VLC (g_vlc_para_fp, "%07x\n", temp1);
		FPRINTF_VLC (g_vlc_para_fp, "%07x\n", temp1);
		FPRINTF_VLC (g_vlc_para_fp, "%07x\n", temp1);
		temp1 = 0;

		temp1 |= ((mb_info_ptr->mvd_8x8[2][1] & 0x1fff) << 14);	// [26:14]: mvd_l0_2_y
		temp1 |= ((mb_info_ptr->mvd_8x8[2][0] & 0x3fff) << 0);	// [13:0]:	mvd_l0_2_x
		FPRINTF_VLC (g_vlc_para_fp, "%07x\n", temp1);
		FPRINTF_VLC (g_vlc_para_fp, "%07x\n", temp1);
		FPRINTF_VLC (g_vlc_para_fp, "%07x\n", temp1);
		FPRINTF_VLC (g_vlc_para_fp, "%07x\n", temp1);
		temp1 = 0;

		temp1 |= ((mb_info_ptr->mvd_8x8[3][1] & 0x1fff) << 14);	// [26:14]: mvd_l0_3_y
		temp1 |= ((mb_info_ptr->mvd_8x8[3][0] & 0x3fff) << 0);	// [13:0]:	mvd_l0_3_x
		FPRINTF_VLC (g_vlc_para_fp, "%07x\n", temp1);
		FPRINTF_VLC (g_vlc_para_fp, "%07x\n", temp1);
		FPRINTF_VLC (g_vlc_para_fp, "%07x\n", temp1);
		FPRINTF_VLC (g_vlc_para_fp, "%07x\n", temp1);
		temp1 = 0;

		for(i=0; i<29; i++)
			FPRINTF_VLC (g_vlc_para_fp, "%07x\n", 0);
#else
		// g_mca_para_fp
		temp1 |= (((img_ptr->sh.i_type==SLICE_TYPE_P)?1:0) << 2); // [3:2]: slice_type, 0=I_SLICE, 1=P_SLICE, 2=B_SLICE
		FPRINTF_PPA (g_mca_para_fp, "%07x", temp1);
		FPRINTF_PPA (g_mca_para_fp, "%07x\n", 0);	// Ref_idx_l0 & sub_mbmode are fixed
		
		temp1 = 0;
		temp1 |= ((mb_info_ptr->mvd_8x8[0][1] & 0x1fff) << 14);	// [26:14]: mvd_l0_0_y
		temp1 |= ((mb_info_ptr->mvd_8x8[0][0] & 0x3fff) << 0);	// [13:0]:	mvd_l0_0_x
		FPRINTF_PPA (g_mca_para_fp, "%07x", temp1);
		FPRINTF_PPA (g_mca_para_fp, "%07x\n", 0);	// reserved
		FPRINTF_PPA (g_mca_para_fp, "%07x", temp1);
		FPRINTF_PPA (g_mca_para_fp, "%07x\n", temp1);

		temp2 = 0;
		temp2 |= ((mb_info_ptr->mvd_8x8[1][1] & 0x1fff) << 14);	// [26:14]: mvd_l0_1_y
		temp2 |= ((mb_info_ptr->mvd_8x8[1][0] & 0x3fff) << 0);	// [13:0]:	mvd_l0_1_x
		FPRINTF_PPA (g_mca_para_fp, "%07x", temp2);
		FPRINTF_PPA (g_mca_para_fp, "%07x\n", temp1);
		FPRINTF_PPA (g_mca_para_fp, "%07x", temp2);
		FPRINTF_PPA (g_mca_para_fp, "%07x\n", temp2);

		temp1 = 0;
		temp1 |= ((mb_info_ptr->mvd_8x8[2][1] & 0x1fff) << 14);	// [26:14]: mvd_l0_2_y
		temp1 |= ((mb_info_ptr->mvd_8x8[2][0] & 0x3fff) << 0);	// [13:0]:	mvd_l0_2_x
		FPRINTF_PPA (g_mca_para_fp, "%07x", temp1);
		FPRINTF_PPA (g_mca_para_fp, "%07x\n", temp2);
		FPRINTF_PPA (g_mca_para_fp, "%07x", temp1);
		FPRINTF_PPA (g_mca_para_fp, "%07x\n", temp1);

		temp2 = 0;
		temp2 |= ((mb_info_ptr->mvd_8x8[3][1] & 0x1fff) << 14);	// [26:14]: mvd_l0_3_y
		temp2 |= ((mb_info_ptr->mvd_8x8[3][0] & 0x3fff) << 0);	// [13:0]:	mvd_l0_3_x
		FPRINTF_PPA (g_mca_para_fp, "%07x", temp2);
		FPRINTF_PPA (g_mca_para_fp, "%07x\n", temp1);
		FPRINTF_PPA (g_mca_para_fp, "%07x", temp2);
		FPRINTF_PPA (g_mca_para_fp, "%07x\n", temp2);
		FPRINTF_PPA (g_mca_para_fp, "%07x", 0);
		FPRINTF_PPA (g_mca_para_fp, "%07x\n", temp2);

		for(i=0; i<14; i++)
			FPRINTF_PPA (g_mca_para_fp, "%014x\n", 0);
#endif
	}
	else
	{
		//FPRINTF_PPA (g_vlc_para_fp, "%07x\n", temp);	// Ref_idx_l1 is fixed
		temp1 |= (((img_ptr->sh.i_type==SLICE_TYPE_P)?1:0) << 2); // [3:2]: slice_type, 0=I_SLICE, 1=P_SLICE, 2=B_SLICE
#ifdef VLC_TV_SPLIT
		FPRINTF_VLC (g_vlc_para_fp, "%07x\n", 0);	// Ref_idx_l0 & sub_mbmode are fixed
		FPRINTF_VLC (g_vlc_para_fp, "%07x\n", temp1);
		for(i=0; i<46; i++)
			FPRINTF_VLC (g_vlc_para_fp, "%07x\n", 0);
#else
		// g_mca_para_fp
		FPRINTF_PPA (g_mca_para_fp, "%07x", temp1);
		FPRINTF_PPA (g_mca_para_fp, "%07x\n", 0);	// Ref_idx_l0 & sub_mbmode are fixed
		for(i=0; i<23; i++)
			FPRINTF_PPA (g_mca_para_fp, "%014x\n", 0);
#endif
	}
}


void PrintfPPALineBuf(ENC_IMAGE_PARAMS_T *img_ptr, ENC_MB_MODE_T *mb_info_ptr, ENC_MB_CACHE_T *mb_cache_ptr)
{
	uint32 temp=0;

	FPRINTF_PPA (g_ppal_buf_fp, "//frame_cnt=%d, mb_x=%d, mb_y=%d\n", g_nFrame_enc, g_mode_dcs_ptr->mb_x, g_mode_dcs_ptr->mb_y);
	temp |= (mb_info_ptr->slice_nr << 0);	// [8:0]:	slice_nr
	temp |= ((mb_info_ptr->type<=I_16x16) << 9);	// [9]:		is_intra
	temp |= ((mb_info_ptr->type==P_SKIP) << 10);	// [10]:	is_skipped
	// [11]: transform_size_8x8_flag
	temp |= ((mb_cache_ptr->nnz[36]!=0) << 12);	// [15:12]:	CBP_BLK of bottom 4 blocks
	temp |= ((mb_cache_ptr->nnz[37]!=0) << 13);
	temp |= ((mb_cache_ptr->nnz[38]!=0) << 14);
	temp |= ((mb_cache_ptr->nnz[39]!=0) << 15);
	temp |= (mb_info_ptr->qp << 16);	// [21:16]:	QP
	temp |= (mb_info_ptr->type << 22);	// [24:22]:	mb_mode
	FPRINTF_PPA (g_ppal_buf_fp, "%08x\n", temp);

	temp = 0;
	if (mb_info_ptr->type==I_16x16)
	{
		temp |= mb_info_ptr->i_intra16x16_pred_mode;	// Intra_pred_mode0
		temp |= ((mb_info_ptr->type==I_4x4) << 30);		// Partition_mode[30], 1:4x4 or 8x8
		temp |= ((mb_info_ptr->type<=I_16x16) << 31);	// [31]:	is_intra
		FPRINTF_PPA (g_ppal_buf_fp, "%08x\n", temp);

		temp = 0;
		temp |= mb_info_ptr->i_intra16x16_pred_mode;	// Intra_pred_mode0
		FPRINTF_PPA (g_ppal_buf_fp, "%08x\n", temp);

		temp = 0;
		FPRINTF_PPA (g_ppal_buf_fp, "%08x\n", temp);
		FPRINTF_PPA (g_ppal_buf_fp, "%08x\n", temp);
		FPRINTF_PPA (g_ppal_buf_fp, "%08x\n", temp);
		FPRINTF_PPA (g_ppal_buf_fp, "%08x\n", temp);
	}
	else if (mb_info_ptr->type==I_4x4)
	{
		temp |= ((mb_info_ptr->intra4x4_pred_mode[12] & 0xf) << 0);	// [3:0]:	intra_block_mode_12
		temp |= ((mb_info_ptr->intra4x4_pred_mode[13] & 0xf) << 4);	// [7:4]:	intra_block_mode_13
		temp |= ((mb_info_ptr->intra4x4_pred_mode[14] & 0xf) << 8);	// [11:8]:	intra_block_mode_14
		temp |= ((mb_info_ptr->intra4x4_pred_mode[15] & 0xf) << 12);// [15:12]:	intra_block_mode_15
		// [29:16]: reserved
		temp |= ((mb_info_ptr->type==I_4x4) << 30);		// Partition_mode[30], 1:4x4 or 8x8
		temp |= ((mb_info_ptr->type<=I_16x16) << 31);	// [31]:	is_intra
		FPRINTF_PPA (g_ppal_buf_fp, "%08x\n", temp);

		temp = 0;
		temp |= ((mb_info_ptr->intra4x4_pred_mode[12] & 0x1f) << 0);	// [4:0]:	intra_block_mode_12
		temp |= ((mb_info_ptr->intra4x4_pred_mode[13] & 0x1f) << 5);	// [9:5]:	intra_block_mode_13
		temp |= ((mb_info_ptr->intra4x4_pred_mode[14] & 0x1f) << 10);	// [14:10]:	intra_block_mode_14
		temp |= ((mb_info_ptr->intra4x4_pred_mode[15] & 0x1f) << 15);	// [19:15]:	intra_block_mode_15
		FPRINTF_PPA (g_ppal_buf_fp, "%08x\n", temp);

		temp = 0;
		FPRINTF_PPA (g_ppal_buf_fp, "%08x\n", temp);
		FPRINTF_PPA (g_ppal_buf_fp, "%08x\n", temp);
		FPRINTF_PPA (g_ppal_buf_fp, "%08x\n", temp);
		FPRINTF_PPA (g_ppal_buf_fp, "%08x\n", temp);
	}
	else
	{
		temp |= ((g_mode_dcs_ptr->blk_mv[2].y & 0x3f) << 0);	// [5:0]: mv_l0_2_y
		temp |= ((g_mode_dcs_ptr->blk_mv[2].x & 0x7f) << 6);	// [12:6]: mv_l0_2_x
		temp |= ((g_mode_dcs_ptr->blk_mv[3].y & 0x3f) << 13);	// [18:13]: mv_l0_3_y
		temp |= ((g_mode_dcs_ptr->blk_mv[3].x & 0x7f) << 19);	// [25:19]: mv_l0_3_x
		temp |= ((mb_info_ptr->type==P_8x8) << 30);		// Partition_mode[30], 1:4x4 or 8x8
		temp |= ((mb_info_ptr->type<=I_16x16) << 31);	// [31]:	is_intra
		FPRINTF_PPA (g_ppal_buf_fp, "%08x", temp);

		temp = 0;
		temp |= ((mb_cache_ptr->ref[28] & 0x1f) << 0);	// [4:0]:	ref_idx_2_l0
		temp |= ((mb_cache_ptr->ref[30] & 0x1f) << 5);	// [9:5]:	ref_idx_3_l0
		FPRINTF_PPA (g_ppal_buf_fp, "%08x\n", temp);

		temp = 0;
		temp |= ((mb_cache_ptr->mv[36][1] & 0xfff) << 0);	// [11:0]:	mv_l0_12_y
		temp |= ((mb_cache_ptr->mv[36][0] & 0x3fff) << 12);	// [25:12]:	mv_l0_12_x
		//temp |= ((mb_cache_ptr->ref[28] & 0x3f) << 26);		// [31:26]:	ref_idx_2_l0
		temp |= ((mb_info_ptr->addr_idx & 0x3f) << 26);		// [31:26]:	addr_idx_2_l0
		FPRINTF_PPA (g_ppal_buf_fp, "%08x\n", temp);
		temp = 0;
		temp |= ((mb_cache_ptr->mv[37][1] & 0xfff) << 0);	// [11:0]:	mv_l0_13_y
		temp |= ((mb_cache_ptr->mv[37][0] & 0x3fff) << 12);	// [25:12]:	mv_l0_13_x
		FPRINTF_PPA (g_ppal_buf_fp, "%08x\n", temp);
		temp = 0;
		temp |= ((mb_cache_ptr->mv[38][1] & 0xfff) << 0);	// [11:0]:	mv_l0_14_y
		temp |= ((mb_cache_ptr->mv[38][0] & 0x3fff) << 12);	// [25:12]:	mv_l0_14_x
		//temp |= ((mb_cache_ptr->ref[30] & 0x3f) << 26);		// [31:26]:	ref_idx_3_l0
		temp |= ((mb_info_ptr->addr_idx & 0x3f) << 26);		// [31:26]:	addr_idx_3_l0
		FPRINTF_PPA (g_ppal_buf_fp, "%08x\n", temp);
		temp = 0;
		temp |= ((mb_cache_ptr->mv[39][1] & 0xfff) << 0);	// [11:0]:	mv_l0_15_y
		temp |= ((mb_cache_ptr->mv[39][0] & 0x3fff) << 12);	// [25:12]:	mv_l0_15_x
		FPRINTF_PPA (g_ppal_buf_fp, "%08x\n", temp);
	}
}


void PrintfDCTParaBuf(ENC_IMAGE_PARAMS_T *img_ptr, ENC_MB_MODE_T *mb_info_ptr, ENC_MB_CACHE_T *mb_cache_ptr, short dct_buf[])
{
	uint32 temp=0;

	uint32 qp_per,qp_rem,qp_per_c,qp_rem_c;
//	uint32 qbits;
	uint32 f;
	qp_per = g_qpPerRem_tbl[mb_info_ptr->qp][0];
	qp_rem = g_qpPerRem_tbl[mb_info_ptr->qp][1];
	qp_per_c = g_qpPerRem_tbl[mb_info_ptr->qp_c][0];
	qp_rem_c = g_qpPerRem_tbl[mb_info_ptr->qp_c][1];
	
	//FPRINTF_PPA (g_dct_para_fp, "//dct_mode=%d vsp_standard=%d scale_enable=%d\n", 0, 4, 0);
	FPRINTF_PPA (g_dct_para_fp, "//frame_cnt=%d, mb_x=%d, mb_y=%d\n", g_nFrame_enc, g_mode_dcs_ptr->mb_x, g_mode_dcs_ptr->mb_y);
	temp |= (img_ptr->mb_y << 0);		// [6:0]: cur_mb_y
	temp |= (img_ptr->mb_x << 7);		// [13:7]:cur_mb_x
	temp |= ((mb_info_ptr->type==I_16x16) << 14);	// [14]:	need_y_hadama
//	temp |= ((0) << 15);							// [15]:	dct_size
	temp |= ((mb_info_ptr->type<=I_16x16) << 16);	// [16]:	is_intra
	temp |= ((mb_info_ptr->type==P_SKIP) << 17);	// [17]:	is_skipped
//	temp |= ((0) << 18);							// [18]:	is_ipcm	
	FPRINTF_PPA (g_dct_para_fp, "%08x\n", temp);

	temp = 0;
	temp |= (qp_rem << 0);		// [2:0]:	Qp_rem_Y
	temp |= (qp_per << 3);		// [6:3]:	Qp_per_Y
	temp |= (qp_rem_c << 7);	// [9:7]:	Qp_rem_U
	temp |= (qp_per_c << 10);	// [13:10]:	Qp_per_U
	temp |= (qp_rem_c << 14);	// [16:14]:	Qp_rem_V
	temp |= (qp_per_c << 17);	// [20:17]:	Qp_per_V
	FPRINTF_PPA (g_dct_para_fp, "%08x\n", temp);

	FPRINTF_PPA (g_dct_para_fp, "%08x\n", 0);	// cbp_blk[25:0], don't need, set to 0

	//qbits = 15 + qp_per;
	//(1<<qbits) /(3)
	f = g_qpPerF_tbl[qp_per];	// give intra, when inter, HW can ">>1"
	FPRINTF_PPA (g_dct_para_fp, "%08x\n", f);	// [21:0]: f of Y intra

	//qbits = 15 + qp_per_c;
	//(1<<qbits) /(3)
	f = g_qpPerF_tbl[qp_per_c];	// give intra, when inter, HW can ">>1"
	FPRINTF_PPA (g_dct_para_fp, "%08x\n", f);	// [21:0]: f of UV intra

	for(f=0; f<5; f++)
		FPRINTF_PPA (g_dct_para_fp, "%08x\n", 0);


	// for cbp_in.txt
	/*FPRINTF_PPA (g_dct_cbp_fp, "//frame_cnt=%d, mb_x=%d, mb_y=%d\n", g_nFrame_enc, g_mode_dcs_ptr->mb_x, g_mode_dcs_ptr->mb_y);
	temp = 0;
	for(f=0; f<16; f++)
		temp |= ((mb_cache_ptr->nnz[x264_scan8[g_blk_rec_ord_tbl[f]]]!=0)<<f);		// CBP for each Y 4x4
	for(f=16; f<24; f++)
		temp |= ((mb_cache_ptr->nnz[x264_scan8[f]]!=0)<<f);		// CBP for each CbCr 4x4
	for(f=0; f<8; f++)
		temp |= ((dct_buf[400+f]!=0) << 24);	// nnz of Chroma DC
	temp |= (mb_info_ptr->i_cbp_chroma==2) << 25;	// nnz of Chroma AC
	//temp = ((mb_info_ptr->i_cbp_chroma & 0x3) << 4) | (mb_info_ptr->i_cbp_luma & 0xf);
	FPRINTF_PPA (g_dct_cbp_fp, "cbp_blk = %x\n", temp);*/

	// for cbp_in.txt
	FPRINTF_PPA (g_dct_cbp_fp, "//frame_cnt=%d, mb_x=%d, mb_y=%d\n", g_nFrame_enc, g_mode_dcs_ptr->mb_x, g_mode_dcs_ptr->mb_y);
	temp = 0;
	for(f=0; f<24; f++)
		temp |= ((mb_cache_ptr->nnz[x264_scan8[f]]!=0)<<f);		// CBP for each YCbCr 4x4
	for(f=0; f<8; f++)
		temp |= ((dct_buf[400+f]!=0) << 24);	// nnz of Chroma DC
	temp |= (mb_info_ptr->i_cbp_chroma==2) << 25;	// nnz of Chroma AC
	//temp = ((mb_info_ptr->i_cbp_chroma & 0x3) << 4) | (mb_info_ptr->i_cbp_luma & 0xf);
	FPRINTF_PPA (g_dct_cbp_fp, "cbp_blk = %x\n", temp);
}


void PrintfDBKParaBuf(ENC_IMAGE_PARAMS_T *img_ptr, int *slice_info)
{
	uint32 temp=0;
	
	FPRINTF_PPA (g_dbk_para_fp, "//frame_cnt=%d, mb_x=%d, mb_y=%d\n", g_nFrame_enc, g_mode_dcs_ptr->mb_x, g_mode_dcs_ptr->mb_y);

	temp |= (img_ptr->mb_y << 0);		// [6:0]: cur_mb_y
	temp |= (img_ptr->mb_x << 8);		// [14:8]:cur_mb_x
	temp |= ((((img_ptr->curr_mb_nr+1) == img_ptr->frame_size_in_mbs)||img_ptr->slice_end)<<31);	// [31]:is_last_mb
	FPRINTF_PPA (g_dbk_para_fp, "%08x\n", temp);
	temp = 0;

	if(img_ptr->sh.i_disable_deblocking_filter_idc)
	{
		FPRINTF_PPA (g_dbk_para_fp, "%08x\n", 0);
		FPRINTF_PPA (g_dbk_para_fp, "%08x\n", 0);
		FPRINTF_PPA (g_dbk_para_fp, "%08x\n", 0);
		FPRINTF_PPA (g_dbk_para_fp, "%08x\n", 0);
	}
	else
	{
		FPRINTF_PPA (g_dbk_para_fp, "%08x\n", g_dbk_reg_ptr->HDBK_BS_H0);
		FPRINTF_PPA (g_dbk_para_fp, "%08x\n", g_dbk_reg_ptr->HDBK_BS_H1);
		FPRINTF_PPA (g_dbk_para_fp, "%08x\n", g_dbk_reg_ptr->HDBK_BS_V0);
		FPRINTF_PPA (g_dbk_para_fp, "%08x\n", g_dbk_reg_ptr->HDBK_BS_V1);
	}
		
	temp |= (slice_info[110]&0x3f) << 0;	// [5:0]:	qp_cur
	temp |= (slice_info[111]&0x3f) << 6;	// [11:6]:	qp_left
	temp |= (slice_info[112]&0x3f) << 12;	// [17:12]:	qp_top
	FPRINTF_PPA (g_dbk_para_fp, "%08x\n", temp);
	temp = 0;

	temp |= (slice_info[116]) << 0;	// [0]:	Is_i4x4
	FPRINTF_PPA (g_dbk_para_fp, "%08x\n", temp);
	FPRINTF_PPA (g_dbk_para_fp, "%08x\n", 0);
}

