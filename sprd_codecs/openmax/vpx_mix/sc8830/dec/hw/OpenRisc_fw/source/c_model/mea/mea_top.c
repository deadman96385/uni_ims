/******************************************************************************
 ** File Name:    mea_top.c                                                   *
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

uint32 * g_mea_src_frame_y;
uint32 * g_mea_src_frame_u;
uint32 * g_mea_src_frame_v;

uint32 * g_mea_ref_frame_y;
uint32 * g_mea_ref_frame_u;
uint32 * g_mea_ref_frame_v;

uint32 g_mea_frame_width;	//word
uint32 g_mea_frame_height; 

uint8 g_SrcMB[6][64];
uint8 g_RefMB[6][64];
int16 g_ErrMB[6][64];
uint8 g_FinalRefMB[6][64];

#ifdef SIM_IN_WIN
//FILE *g_fp_trace_mea_sad;
FILE *g_pPureSadFile;
FILE *g_pFinalResultFile;
FILE *g_p12x9UVBlockFile;
FILE *g_pPredMVFile;
#endif //SIM_IN_WIN

int32 g_search_point_cnt;

PUBLIC void init_mea(void)
{

	/*init motion compensation function*/
	g_dec_mc_16x16[0] = Mp4_mc_xyfull_16x16;
	g_dec_mc_16x16[1] = Mp4_mc_xfullyhalf_16x16;
	g_dec_mc_16x16[2] = Mp4_mc_xhalfyfull_16x16;
	g_dec_mc_16x16[3] = Mp4_mc_xyhalf_16x16;

	g_dec_mc_8x8[0] = Mp4_mc_xyfull_8x8;
	g_dec_mc_8x8[1] = Mp4_mc_xfullyhalf_8x8;
	g_dec_mc_8x8[2] = Mp4_mc_xhalfyfull_8x8;
	g_dec_mc_8x8[3] = Mp4_mc_xyhalf_8x8;

	g_dec_mc_8x8_me[0] = Mp4_mc_xyfull_8x8_me;
	g_dec_mc_8x8_me[1] = Mp4_mc_xfullyhalf_8x8_me;
	g_dec_mc_8x8_me[2] = Mp4_mc_xhalfyfull_8x8_me;
	g_dec_mc_8x8_me[3] = Mp4_mc_xyhalf_8x8_me;

	g_search_point_cnt = 0;
}

void mea_module(void)
{
	int32 mb_pos_x;
	int32 mb_pos_y;
	int32 meEna;
	int32 pre_filter_en;
	int32 pre_filter_thres;
	int32 mea_cfg0, mea_cfg1, mea_cfg2;
	int32 mea_result;
	int32 mb_mode = INTRA;
	int32 standard;
	
	standard = (g_glb_reg_ptr->VSP_CFG0 >> 8) & 0xf;

	g_mea_frame_width = ((g_glb_reg_ptr->VSP_CFG1 >> 0) & 0x1ff); //word
	g_mea_frame_height = ((g_glb_reg_ptr->VSP_CFG1 >> 12) & 0x1ff);

	if(VSP_JPEG != standard)
	{
		g_mea_frame_width  *= 4; //word
		g_mea_frame_height *= MB_SIZE;
	}else
	{
		int32 mcu_format = ((g_glb_reg_ptr->VSP_CFG1 >> 24) & 0x7);

		switch(mcu_format)
		{
		case JPEG_FW_YUV420:
			g_mea_frame_width  *= 4; //word
			g_mea_frame_height *= MB_SIZE;
			break;
		case JPEG_FW_YUV422:
			g_mea_frame_width  *= 4; //word
			g_mea_frame_height *= BLOCK_SIZE;
			break;
		default:
			PRINTF("error format!");
		}
	}

	mea_cfg0 = g_mea_reg_ptr->MEA_CFG0;
	mea_cfg1 = g_mea_reg_ptr->MEA_CFG1;
	mea_cfg2 = g_mea_reg_ptr->MEA_CFG2;

	pre_filter_en = (mea_cfg0 >> 24) & 0x01;
	pre_filter_thres = (mea_cfg0 >> 16) & 0xff;
	meEna	 = (mea_cfg1 >> 0) & 0x01;
	mb_pos_x = (mea_cfg2 >> 0) & 0xff;
	mb_pos_y = (mea_cfg2 >> 16) & 0xff;

#if defined(MPEG4_ENC)
	g_mea_src_frame_y = (uint32 *)g_enc_vop_mode_ptr->pYUVSrcFrame->imgY;
	g_mea_src_frame_u = (uint32 *)g_enc_vop_mode_ptr->pYUVSrcFrame->imgU;
	g_mea_src_frame_v = (uint32 *)g_enc_vop_mode_ptr->pYUVSrcFrame->imgV;

	g_mea_ref_frame_y = (uint32 *)g_enc_vop_mode_ptr->pYUVRefFrame->imgY;
	g_mea_ref_frame_u = (uint32 *)g_enc_vop_mode_ptr->pYUVRefFrame->imgU;
	g_mea_ref_frame_v = (uint32 *)g_enc_vop_mode_ptr->pYUVRefFrame->imgV;
	
	if (g_mea_reg_ptr->REF_CFG>>31) //fetch enable
	{
		trace_mea_fetch_one_macroblock(&g_mea_fetch);
	}
#elif defined(JPEG_ENC)
{
	JPEG_CODEC_T *JpegCodec = Get_JPEGEncCodec();

	g_mea_src_frame_y = (uint32 *)JpegCodec->YUV_Info_0.y_data_ptr;
	g_mea_src_frame_u = (uint32 *)JpegCodec->YUV_Info_0.u_data_ptr;
	g_mea_src_frame_v = (uint32 *)JpegCodec->YUV_Info_0.v_data_ptr;

	if(JpegCodec->mea_bfr1_valid)
	{
		g_mea_src_frame_y = (uint32 *)JpegCodec->YUV_Info_1.y_data_ptr;
		g_mea_src_frame_u = (uint32 *)JpegCodec->YUV_Info_1.u_data_ptr;
		g_mea_src_frame_v = (uint32 *)JpegCodec->YUV_Info_1.v_data_ptr;
	}
}

#endif //defined(MPEG4_ENC)

	if(pre_filter_en)
	{
		pre_filter_mb(mb_pos_x, mb_pos_y, pre_filter_thres);
	}
	print_src_mb(mb_pos_x, mb_pos_y);
	
	if(meEna) //inter mb
	{
		SoftMotionEstimation(mb_pos_x, mb_pos_y);
		mea_result = (g_mea_reg_ptr->MEA_CTL >> MEA_RESULT_BIT & 0x03);
		mb_mode = (mea_result == 0)?INTRA:((mea_result == 1)?INTER:INTER4V);
	}

	if(VSP_JPEG ==standard)
	{
		int32 mcu_num_x = ((g_glb_reg_ptr->VSP_CFG1 >> 0) & 0xff);

		memset(vsp_mea_out_bfr, 0, MEA_OUT_BFR_SIZE*sizeof(uint32));	
		fetch_src_mb_to_dctiobfr_jpeg(mb_pos_x, mb_pos_y); //souce mb
		print_src_mb(mb_pos_x, mb_pos_y); //after prefilter

		mb_pos_x++;
				
		if(mb_pos_x == mcu_num_x)
		{
			mb_pos_x = 0;
			mb_pos_y++;
		}

		g_mea_reg_ptr->MEA_CFG2 = (((mb_pos_y& 0x1ff)<<16) | (mb_pos_x & 0x1ff));
	}else
	{
		if(INTRA == mb_mode)
		{
			memset(vsp_mea_out_bfr, 0, MEA_OUT_BFR_SIZE*sizeof(uint32));	
			fetch_src_mb_to_dctiobfr(mb_pos_x, mb_pos_y); //souce mb
		}else //inter mb
		{	
			output_ref_mb();
			fetch_err_mb_to_dctiobfr();
		}
	}

//	if((mb_pos_x == 10) && (mb_pos_y == 8))
//	{
//		FILE *fp = fopen("d:\\prefilted.yuv", "wb");
//		fwrite(g_mea_src_frame_y, 1, g_mea_frame_width*g_mea_frame_height*4, fp);
//		fwrite(g_mea_src_frame_u, 1, g_mea_frame_width*g_mea_frame_height, fp);
//		fwrite(g_mea_src_frame_v, 1, g_mea_frame_width*g_mea_frame_height, fp);
//		fclose(fp);
//	}

}

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End 
