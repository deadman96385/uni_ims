/******************************************************************************
 ** File Name:    vlc_top.c                                                   *
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

int		g_vlc_status;
uint16 g_enc_dc_pred_y;
uint16 g_enc_dc_pred_u; 
uint16 g_enc_dc_pred_v;

void mvlc_ctr ()
{
	//regist file
	int mb_x;
	int cbp;
	int left_avail;
	int top_avail;
	int tl_avail;
	int mb_type;
	int is_short_header;
	int qp;

	int blk_id;

	int16	dc_coeff_q;
	int16	dc_coeff_iq;
	int		dc_scaler;

	int		vlc_dbuf_addr;
	int		vlc_dbuf_rdata;

	int		dir_pred;
	int16	dc_pred;
	int16	dc_pred_q;
	int16	dc_diff_q;

	int		vlc_bsm_wdata;
	int		vlc_bsm_length;
	int		blk_cbp;

	int		start_position;

	int		is_intra;

	int32	vlc_ctrl = g_vlc_reg_ptr->VLC_CTRL;

	g_vlc_status = 1;

	mb_x		= (vlc_ctrl >> 24) & 0xff;
	qp			= (vlc_ctrl >> 16) & 0x3f;
	left_avail	= (vlc_ctrl >> 11) & 0x01;
	top_avail	= (vlc_ctrl >> 10) & 0x01;
	tl_avail	= (vlc_ctrl >> 9) & 0x01;
	mb_type		= (vlc_ctrl >> 8) & 0x01;
	cbp			= (vlc_ctrl >> 0) & 0x3f;

	is_short_header = (((g_glb_reg_ptr->VSP_CFG0 >> 8) & 0x7) == 0) ? 1 : 0;

	is_intra = (mb_type == 0) ? 1 : 0;

	for (blk_id = 0; blk_id < 6; blk_id++)
	{
		/*dc prediction for mpeg4 intra mb*/
		if (is_intra)
		{
			vlc_dbuf_addr  = 32*blk_id;
			vlc_dbuf_rdata = vsp_dct_io_1[vlc_dbuf_addr];

			dc_coeff_q     = (uint16)((vlc_dbuf_rdata << 16) >> 16);
		}

		FPRINTF (g_fp_trace_vlc, "nblock: %d\n", blk_id);

		if (is_intra && !is_short_header)
		{
			dc_scaler = GetDCScaler (qp, blk_id);       //current block qp

			dc_coeff_iq = dc_coeff_q * dc_scaler;

			if (intra_blk_cnt == 277)
				printf ("");

			dc_prediction		(
						mb_x,
						blk_id,
						left_avail,
						top_avail,
						tl_avail,
						dc_coeff_iq,
						
						&dir_pred,
						&dc_pred
						);

			dc_pred_q	= (dc_scaler == 0) ? 0
				: (dc_scaler == 1) ? dc_pred 
				: (dc_scaler == 2) ? ((dc_pred + 1) >> 1)
				: (int)(dc_pred + (dc_scaler>>1)) * vlc_GetDivInverse (dc_scaler) >> 17;
			
			dc_diff_q = dc_coeff_q - dc_pred_q;	

			PrintDCPred (dir_pred, dc_pred);
	//		fprintf (g_fp_trace_vlc, "dc_coeff: %d, dc_pred: %d\n", dc_coeff_q, dc_pred_q);
		}



		/*dc encoding*/
		if (is_intra)
		{
			if (is_short_header)
			{
				if (dc_coeff_q == 128)
					dc_coeff_q = 255;

				vlc_bsm_wdata = dc_coeff_q;
				vlc_bsm_length = 8;

				write_nbits (vlc_bsm_wdata, vlc_bsm_length, 1);
			}
			else
			{
				mpeg4_dc_enc (dc_diff_q, blk_id, &vlc_bsm_wdata, &vlc_bsm_length);
			}

		}

		/*vlc for ac coeff for coded block*/
		blk_cbp = (cbp >> (5 - blk_id)) & 1;
		if (blk_cbp)
		{
			start_position = (is_intra == 1) ? 1 : 0;

			rlc_blk (
							blk_id, 
							MPEG4, 
							start_position, 

							is_intra, 
							is_short_header
						);
		}
	}

	g_vlc_status = 0;

//	fclose (g_fp_trace_vlc);
}

void jvlc_ctr()
{
	int32 blk_num = g_block_num_in_one_mcu;
	int32 blk_idx;

	jpeg_bsmr.destuffing.destuffing_eb = 1;

	for(blk_idx = 0; blk_idx < blk_num; blk_idx++)
	{
		rlc_blk (blk_idx, JPEG, 0, 0, 0);
	}
}

void vlc_module()
{
	int32 standard		= (g_glb_reg_ptr->VSP_CFG0 >> 8) & 0x7;

	if(VSP_JPEG != standard)
	{

	}else
	{
		jvlc_ctr();
	}
}

void clear_vlc()
{
	int32 bit_left = jpeg_bsmr.bit_left;
	int32 stuff_bit = bit_left - ((bit_left>>3)<<3);
	
	write_nbits (((1<<stuff_bit)-1), stuff_bit, 1);
}

void init_vlc_module()
{
	g_enc_dc_pred_y = 0;
	g_enc_dc_pred_u = 0; 
	g_enc_dc_pred_v = 0;
}
/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End 
