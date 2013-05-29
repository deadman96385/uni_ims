/******************************************************************************
 ** File Name:    mea_trace.c                                                 *
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

void init_mea_trace()
{
if(g_trace_enable_flag&TRACE_ENABLE_MEA)
{
	g_pPureSadFile = fopen("..\\..\\trace\\PureSad.txt", "w");
	g_pFinalResultFile = fopen("..\\..\\trace\\FinalResult.txt", "w");
	g_p12x9UVBlockFile = fopen("..\\..\\trace\\trace_mea_UV12x9Block.txt", "w");
	g_pPredMVFile = fopen("..\\..\\trace\\PredMV.txt","r");
}
}

void print_src_mb(int32 mb_x, int32 mb_y)
{
if(g_trace_enable_flag&TRACE_ENABLE_MEA)
{
	int32 i, j;
	uint32 *pIntSrcY = g_mea_src_frame_y;
	uint8 *pSrcU = (uint8*)g_mea_src_frame_u;
	uint8 *pSrcV = (uint8*)g_mea_src_frame_v;
	int32 offset;
	uint32 *p_y;
	FILE *test_vector_fp = g_fp_mea_src_mb_tv;
	int32 frame_width = g_mea_frame_width << 2; //byte
	int32 frame_height = g_mea_frame_height;
	int32 frame_width_c = frame_width >> 1;
	int32 vid_standard	= (g_glb_reg_ptr->VSP_CFG0 >> 8) & 0xf;
	int32 uv_interleaved	= ((g_ahbm_reg_ptr->AHBM_BURST_GAP >> 8) & 0x01);
	int32 data_format = (g_glb_reg_ptr->VSP_CFG1>>24)&0x7;
	int32 c_pix_pitch;
	int32 four_pix;
	int8 *p_u, *p_v;

	if (!uv_interleaved) //three plane
	{
		c_pix_pitch = 1;
	}else //two plane
	{
		c_pix_pitch = 2;
		pSrcV = pSrcU+1;
	}
	
	if(JPEG_FW_YUV420 == data_format) 
	{
		offset = (frame_width * (mb_y*16) + (mb_x * 16))>>2; //word
		p_y = pIntSrcY + offset;
		for(i = 0; i < 16; i++)
		{
			for(j = 0; j < 4; j++ )
			{
				fprintf_oneWord_hex(test_vector_fp, *p_y++);
			}
			p_y += (frame_width - 16)/4;
		}
	}else //422
	{
		offset = (frame_width * (mb_y*8) + (mb_x * 16))>>2; //word
		p_y = pIntSrcY + offset;
		for(i = 0; i < 8; i++)
		{
			for(j = 0; j < 4; j++ )
			{
				fprintf_oneWord_hex(test_vector_fp, *p_y++);
			}
			p_y += (frame_width - 16)/4;
		}
	}

	//u
	offset = (frame_width_c * (mb_y*8) + (mb_x * 8))*c_pix_pitch; //byte
		
	p_u = pSrcU+offset;
	for(i = 0; i < 8; i++)
	{
		four_pix = ((p_u[3*c_pix_pitch]&0xff)<<24)|((p_u[2*c_pix_pitch]&0xff)<<16)|
				   ((p_u[1*c_pix_pitch]&0xff)<<8)|((p_u[0]&0xff)<<0);
		fprintf_oneWord_hex(test_vector_fp, four_pix);

		four_pix = ((p_u[7*c_pix_pitch]&0xff)<<24)|((p_u[6*c_pix_pitch]&0xff)<<16)|
			       ((p_u[5*c_pix_pitch]&0xff)<<8)|((p_u[4*c_pix_pitch]&0xff)<<0);
		fprintf_oneWord_hex(test_vector_fp, four_pix);	
						
		p_u += frame_width_c*c_pix_pitch;
	}

	//v
	p_v = pSrcV+offset;
	for(i = 0; i < 8; i++)
	{
		four_pix = ((p_v[3*c_pix_pitch]&0xff)<<24)|((p_v[2*c_pix_pitch]&0xff)<<16)|
			       ((p_v[1*c_pix_pitch]&0xff)<<8)|((p_v[0]&0xff)<<0);
		fprintf_oneWord_hex(test_vector_fp, four_pix);

		four_pix = ((p_v[7*c_pix_pitch]&0xff)<<24)|((p_v[6*c_pix_pitch]&0xff)<<16)|
			       ((p_v[5*c_pix_pitch]&0xff)<<8)|((p_v[4*c_pix_pitch]&0xff)<<0);
		fprintf_oneWord_hex(test_vector_fp, four_pix);	
						
		p_v += frame_width_c*c_pix_pitch;
	}
}
}

//422		
void Dump_Src_frame_Data_422(uint8 *input_yuv_bfr_ptr, int32 frame_width, int32 frame_height, int32 uv_interleaved)
{
if(g_trace_enable_flag&TRACE_ENABLE_MEA)
{
	int mb_num_x = (frame_width >> 4);
	int mb_num_y = (frame_height >> 3);
	uint32 *pIntSrcY = (uint32 *)input_yuv_bfr_ptr;
	uint32 *pIntSrcU = pIntSrcY + (frame_width*frame_height>>2);
	uint32 *pIntSrcV = pIntSrcU + (frame_width*frame_height>>3);
	int i,j;

	for(i = 0; i < frame_height; i++)
	{
		for(j = 0; j < (frame_width>>3); j++)
		{
			fprintf_oneWord_hex(g_fp_mea_src_frm_y_tv, *pIntSrcY++);
			fprintf_oneWord_hex(g_fp_mea_src_frm_y_tv, *pIntSrcY++);

			fprintf_oneWord_hex(g_fp_mea_src_frm_u_tv, *pIntSrcU++);
			if (uv_interleaved) //two plane
			{
				fprintf_oneWord_hex(g_fp_mea_src_frm_u_tv, *pIntSrcU++);
			}else //three plane
			{
				fprintf_oneWord_hex(g_fp_mea_src_frm_v_tv, *pIntSrcV++);
		}
	}
}
}
}

//420
void Dump_Src_frame_Data_420(uint8 *input_yuv_bfr_ptr, int32 frame_width, int32 frame_height, int32 uv_interleaved)
{
if(g_trace_enable_flag&TRACE_ENABLE_MEA)
{
	int mb_num_x = (frame_width >> 4);
	int mb_num_y = (frame_height >> 4);
	uint32 *pIntSrcY = (uint32 *)input_yuv_bfr_ptr;
	uint32 *pIntSrcU = pIntSrcY + (frame_width*frame_height>>2);
	uint32 *pIntSrcV = pIntSrcU + (frame_width*frame_height>>4);
	int i,j;

	for(i = 0; i < frame_height; i++)
	{
		for(j = 0; j < (frame_width>>2); j++)
		{
			fprintf_oneWord_hex(g_fp_mea_src_frm_y_tv, *pIntSrcY++);
		}
	}

	for(i = 0; i < frame_height>>1; i++)
	{
		for(j = 0; j < (frame_width>>3); j++)
		{
			fprintf_oneWord_hex(g_fp_mea_src_frm_u_tv, *pIntSrcU++);
			if (!uv_interleaved) //three plane
			{
				fprintf_oneWord_hex(g_fp_mea_src_frm_v_tv, *pIntSrcV++);
			}else //two plane
			{
				fprintf_oneWord_hex(g_fp_mea_src_frm_u_tv, *pIntSrcU++);
			}
		}
	}

//	{
//		FILE *fp = fopen("d:\\src.yuv", "wb");
//		fwrite(input_yuv_bfr_ptr, 1, frame_width*frame_height*3/2, fp);
//		fclose(fp);
//	}
}
}

#if defined(MPEG4_ENC)
/*printf fetched reference Y data for verification*/
void trace_mea_fetch_one_macroblock(MEA_FETCH_REF *pMea_fetch)
{
if(g_trace_enable_flag&TRACE_ENABLE_MEA)
{
	int32 frame_width = g_mea_frame_width<<2; //byte
	int32 frame_height = g_mea_frame_height;
	uint8 * pRefChar;
	uint32 * pRefInt;
	int i, j;
	uint32 fourPix;
	int start_Y = pMea_fetch->start_Y;
	int start_X = pMea_fetch->start_X;
	int fetch_XWidth = pMea_fetch->fetch_XWidth; 
	int fetch_YHeigth = pMea_fetch->fetch_YHeigth;

	pRefChar = g_enc_vop_mode_ptr->pYUVRefFrame->imgY + start_Y*frame_width + start_X;

	for (j = 0; j < fetch_YHeigth; j++)
	{
		pRefInt = (uint32 *)pRefChar;
		for (i = 0; i < fetch_XWidth/4; i++)
		{
			fourPix = pRefInt [i];
			fprintf (g_fp_mea_tv, "%08x\n", fourPix);
		}

		pRefChar += frame_width;
	}
}
}
#endif
/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End 
