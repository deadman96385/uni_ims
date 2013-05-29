/******************************************************************************
 ** File Name:    vld_top.c	    										  *
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

#include "sc6800x_video_header.h"
/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    extern   "C" 
    {
#endif

int16 g_pre_dc_value[3];				/*store pre block value for DPCM coding*/
		
int32 InPutRstMarker(void)
{
	uint8 ret = 0; 

	flush_read(0xFFFFFFFF); 
	ret = (uint8)read_nbits(8);
	ret = (uint8)read_nbits(8); 
#if defined(JPEG_DEC)
	if((ret < M_RST0) || (ret > M_RST7))
	{
		return 0xff;//JPEG_FAILED;
	}
#endif
	return 0x0; //JPEG_SUCCESS;
}

PUBLIC void vld_module()
{
	int32 standard		= (g_glb_reg_ptr->VSP_CFG0 >> 8) & 0x7;

	if(VSP_JPEG != standard)
	{
		int mb_type;
		int cbp;
		int rotation_ena;
		int is_rvlc;

		g_vld_reg_ptr->VLD_CTL &= 0xFFFFFFFE; ///clear "START" flag.

		/*register file*/
		is_rvlc			= (g_vld_reg_ptr->MPEG4_CFG0 >> 25) & 1;
		mb_type			= (g_vld_reg_ptr->MPEG4_CFG0 >> 24) & 1;
		cbp				= (g_vld_reg_ptr->MPEG4_CFG0 >> 8) & 0x3f;	
		rotation_ena	= (g_glb_reg_ptr->VSP_CFG0 >> 16) & 1;

		memset (vsp_dct_io_1, 0, 216*4);
#if defined(MPEG4_DEC)
// 		VLD_FPRINTF(g_pfRunLevel_mpeg4dec, "frame: %d, Y: %d, X: %d\n", g_nFrame_dec, (g_glb_reg_ptr->VSP_CTRL0>>8) & 0x3f, g_glb_reg_ptr->VSP_CTRL0 & 0x3f);
#endif
		if (mb_type == VSP_INTER)
		{
			VLD_FPRINTF(g_pfRunLevel_mpeg4dec, "inter mb\n");
		
			mpeg4dec_vld (
							standard,
							is_rvlc,
							mb_type,
							cbp, 
							0,					//start_position, not used 
							VSP_STD_ZIGZAG,	
							0,					//blk_id, not used
							rotation_ena
				);

		}
		else
		{
			VLD_FPRINTF(g_pfRunLevel_mpeg4dec, "intra mb\n");
		
			mpeg4dec_acdc_pred (
							standard,
							is_rvlc,
							mb_type,
							cbp,
							rotation_ena	
				);
		}
	}else
	{
	#if defined(JPEG_DEC)
		int32 block_id = 0, ci = 0;
		HUFF_TBL_T *dc_tbl = NULL, *ac_tbl = NULL;
		const uint8 *quant = NULL;
		int16 *pBlk_asic = NULL;
		JPEG_CODEC_T *jpeg_fw_codec = Get_JPEGDecCodec();

		for(block_id = 0; block_id < g_block_num_in_one_mcu; block_id++)
		{
			ci = g_blocks_membership[block_id];
			dc_tbl = &(jpeg_fw_codec->dc_huff_tbl[jpeg_fw_codec->tbl_map[ci].dc_huff_tbl_id]);
			ac_tbl = &(jpeg_fw_codec->ac_huff_tbl[jpeg_fw_codec->tbl_map[ci].ac_huff_tbl_id]);
			quant = jpeg_fw_codec->quant_tbl[jpeg_fw_codec->tbl_map[ci].quant_tbl_id];
		

			pBlk_asic =  (int16*)(vsp_dct_io_1 + 32*block_id);

			/*decode one block*/
			jpeg_vld_Block(pBlk_asic, dc_tbl, ac_tbl, quant, block_id, g_pre_dc_value[ci], (int8)(ci>0));

			/*update pre_dc_val*/
			pBlk_asic[0] += g_pre_dc_value[ci];
			g_pre_dc_value[ci] = pBlk_asic[0];
		}

		g_vld_reg_ptr->JPEG_DC_Y = g_pre_dc_value[0]&0x7ff;
		g_vld_reg_ptr->JPEG_DC_UV = ((g_pre_dc_value[2]&0x7ff)<<16) | (g_pre_dc_value[1]&0x7ff);
	#endif
	}
}

void init_vld()
{
	g_pre_dc_value[0] = 0;
	g_pre_dc_value[1] = 0;
	g_pre_dc_value[2] = 0;
}
/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End 