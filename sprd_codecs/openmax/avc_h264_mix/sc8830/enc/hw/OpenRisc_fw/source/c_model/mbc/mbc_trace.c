/******************************************************************************
 ** File Name:	  mbc_trace.c		                                          *
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

void printf_mbc_mb (FILE * pfMBC)
{
if(g_vector_enable_flag&VECTOR_ENABLE_MBC) 
{
	int i;
	uint32 * pBlk;

	/*Y*/
	pBlk = vsp_mbc_out_bfr+(MBC_Y_DATA_OFFSET/4); //word
	for (i = 0; i < 16; i++)
	{
		fprintf_oneWord_hex (pfMBC, pBlk[1]);
		fprintf_oneWord_hex (pfMBC, pBlk[0]);		
		fprintf (pfMBC, "\n");//weihu
		fprintf_oneWord_hex (pfMBC, pBlk[3]);
		fprintf_oneWord_hex (pfMBC, pBlk[2]);		
        fprintf (pfMBC, "\n");//weihu
		pBlk += (MBC_Y_SIZE/4);
	}

	/*U*/
	pBlk = vsp_mbc_out_bfr+(MBC_U_OFFSET/4)+(MBC_C_DATA_OFFSET/4); //word
	for (i = 0; i < 8; i++)
	{
		fprintf_oneWord_hex (pfMBC, pBlk[1]);
		fprintf_oneWord_hex (pfMBC, pBlk[0]);
		
        fprintf (pfMBC, "\n");//weihu
		pBlk += (MBC_C_SIZE/4);
	}

	/*V*/
	pBlk = vsp_mbc_out_bfr+(MBC_V_OFFSET/4)+(MBC_C_DATA_OFFSET/4); //word
	for (i = 0; i < 8; i++)
	{
		
		fprintf_oneWord_hex (pfMBC, pBlk[1]);
		fprintf_oneWord_hex (pfMBC, pBlk[0]);
		fprintf (pfMBC, "\n");//weihu

		pBlk += (MBC_C_SIZE/4);
	}
}
}

void Dump_dec_frame_Data(uint8 *frm_y_ptr, uint8 *frm_u_ptr, uint8 *frm_v_ptr, int32 frame_width, int32 frame_height, int32 out_mcu_format)
{
	int32 i, j;
	int32 *word_ptr;
	int32 frame_height_uv, frame_width_uv;
	int32 uv_interleaved	= ((g_ahbm_reg_ptr->AHBM_BURST_GAP >> 8) & 0x01);

	if(out_mcu_format == JPEG_FW_YUV420)
	{
		frame_width_uv = frame_width >> 1;
		frame_height_uv = frame_height >> 1;
	}else if(out_mcu_format == JPEG_FW_YUV422)
	{
		frame_width_uv = frame_width >> 1;
		frame_height_uv = frame_height;
	}else if(out_mcu_format == JPEG_FW_YUV400)
	{
		frame_width_uv = 0;
		frame_height_uv = 0;
	}else
	{
		printf("error format!");
	}

	word_ptr = (int32 *)frm_y_ptr;
	for(j = 0; j < frame_height; j++)
	{
		for(i = 0; i < (frame_width>>2); i++)
		{
			fprintf_oneWord_hex(g_fp_rec_frm_tv, *word_ptr++);
		}
	}

	if (uv_interleaved) //two plane
	{
		word_ptr = (int32 *)frm_u_ptr;
		for(j = 0; j < frame_height_uv; j++)
		{
			for(i = 0; i < (frame_width_uv>>2); i++)
			{
				fprintf_oneWord_hex(g_fp_rec_frm_tv, *word_ptr++);
				fprintf_oneWord_hex(g_fp_rec_frm_tv, *word_ptr++);
			}
		}
	}else //three
	{
		word_ptr = (int32 *)frm_u_ptr;
		for(j = 0; j < frame_height_uv; j++)
		{
			for(i = 0; i < (frame_width_uv>>2); i++)
			{
				fprintf_oneWord_hex(g_fp_rec_frm_tv, *word_ptr++);
			}
		}

		word_ptr = (int32 *)frm_v_ptr;
		for(j = 0; j < frame_height_uv; j++)
		{
			for(i = 0; i < (frame_width_uv>>2); i++)
			{
				fprintf_oneWord_hex(g_fp_rec_frm_tv, *word_ptr++);
			}
		}
	}

	return;
}

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End 
