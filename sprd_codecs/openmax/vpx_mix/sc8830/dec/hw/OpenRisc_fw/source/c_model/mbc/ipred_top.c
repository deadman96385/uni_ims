#include <memory.h>
#include "sci_types.h"
#include "video_common.h"
#include "ipred_global.h"
#include "common_global.h"
#include "buffer_global.h"
#include "mbc_global.h"

IntraLuma4x4Pred s_intraPredLuma4x4[9];
IntraLuma8x8Pred s_intraPredLuma8x8[9];//weihu
IntraLuma16x16Pred s_intraPredLuma16x16[4];
IntraChroma8x8Pred s_intraPredChroma8x8[4];

Intra4x4Pred g_intraPred4x4[9];
Intra16x16Pred g_intraPredLuma16x16[4];
Intra16x16Pred g_intraPredChroma8x8[4];

void InitIntra4x4LumaPredFunction()
{
	IntraLuma4x4Pred *intraPredLuma4x4 = s_intraPredLuma4x4;

	intraPredLuma4x4[0] = intra_pred_luma4x4_VERT_PRED;
	intraPredLuma4x4[1] = intra_pred_luma4x4_HOR_PRED;
	intraPredLuma4x4[2] = intra_pred_luma4x4_DC_PRED;
	intraPredLuma4x4[3] = intra_pred_luma4x4_DIAG_DOWN_LEFT_PRED;
	intraPredLuma4x4[4] = intra_pred_luma4x4_DIAG_DOWN_RIGHT_PRED;
	intraPredLuma4x4[5] = intra_pred_luma4x4_VERT_RIGHT_PRED;
	intraPredLuma4x4[6] = intra_pred_luma4x4_HOR_DOWN_PRED;
	intraPredLuma4x4[7] = intra_pred_luma4x4_VERT_LEFT_PRED;
	intraPredLuma4x4[8] = intra_pred_luma4x4_HOR_UP_PRED;
}

void InitIntra8x8LumaPredFunction()//weihu
{
	IntraLuma8x8Pred *intraPredLuma8x8 = s_intraPredLuma8x8;
	
	intraPredLuma8x8[0] = intra_pred_luma8x8_VERT_PRED;
	intraPredLuma8x8[1] = intra_pred_luma8x8_HOR_PRED;
	intraPredLuma8x8[2] = intra_pred_luma8x8_DC_PRED;
	intraPredLuma8x8[3] = intra_pred_luma8x8_DIAG_DOWN_LEFT_PRED;
	intraPredLuma8x8[4] = intra_pred_luma8x8_DIAG_DOWN_RIGHT_PRED;
	intraPredLuma8x8[5] = intra_pred_luma8x8_VERT_RIGHT_PRED;
	intraPredLuma8x8[6] = intra_pred_luma8x8_HOR_DOWN_PRED;
	intraPredLuma8x8[7] = intra_pred_luma8x8_VERT_LEFT_PRED;
	intraPredLuma8x8[8] = intra_pred_luma8x8_HOR_UP_PRED;
}

void InitIntra16x16LumaPredFunction()
{
	IntraLuma16x16Pred *intraPredLuma16x16 = s_intraPredLuma16x16;

	intraPredLuma16x16[0] = intra_pred_luma16x16_VERT_PRED;
	intraPredLuma16x16[1] = intra_pred_luma16x16_HOR_PRED;
	intraPredLuma16x16[2] = intra_pred_luma16x16_DC_PRED;
	intraPredLuma16x16[3] = intra_pred_luma16x16_PLANE_PRED;
}

void InitIntraChromaPredFunction()
{
// 	IntraLuma16x16Pred *intraPredLuma16x16 = s_intraPredLuma16x16;

	s_intraPredChroma8x8[0] = intra_pred_chroma8x8_DC_PRED;
	s_intraPredChroma8x8[1] = intra_pred_chroma8x8_HOR_PRED;
	s_intraPredChroma8x8[2] = intra_pred_chroma8x8_VERT_PRED;
	s_intraPredChroma8x8[3] = intra_pred_chroma8x8_PLANE_PRED;
}

void RvDec_InitIntra4x4PredFunction()
{
	Intra4x4Pred *intraPred4x4 = g_intraPred4x4;
	
	intraPred4x4[0] = rv_intra_pred_4x4_DC_PRED;
	intraPred4x4[1] = rv_intra_pred_4x4_VERT_PRED;
	intraPred4x4[2] = rv_intra_pred_4x4_HOR_PRED;
	intraPred4x4[3] = rv_intra_pred_4x4_DIAG_DOWN_RIGHT_PRED;
	intraPred4x4[4] = rv_intra_pred_4x4_DIAG_DOWN_LEFT_PRED;
	intraPred4x4[5] = rv_intra_pred_4x4_VERT_RIGHT_PRED;
	intraPred4x4[6] = rv_intra_pred_4x4_VERT_LEFT_PRED;
	intraPred4x4[7] = rv_intra_pred_4x4_HOR_UP_PRED;
	intraPred4x4[8] = rv_intra_pred_4x4_HOR_DOWN_PRED;
}

void RvDec_InitIntra16x16PredFunction()
{
	Intra16x16Pred *intraPredLuma16x16 = g_intraPredLuma16x16;
	Intra16x16Pred *intraPredChroma8x8 = g_intraPredChroma8x8;

	
	intraPredLuma16x16[0] = rv_intra_pred_Luma16x16_DC_PRED;
	intraPredLuma16x16[1] = rv_intra_pred_Luma16x16_VERT_PRED;
	intraPredLuma16x16[2] = rv_intra_pred_Luma16x16_HORZ_PRED;
	intraPredLuma16x16[3] = rv_intra_pred_Luma16x16_PLANAR_PRED;
	
	intraPredChroma8x8[0] = rv_intra_pred_CHROMA8x8_DC_PRED;
	intraPredChroma8x8[1] = rv_intra_pred_CHROMA8x8_VERT_PRED;
	intraPredChroma8x8[2] = rv_intra_pred_CHROMA8x8_HORZ_PRED;
	intraPredChroma8x8[3] = rv_intra_pred_CHROMA8x8_DC_PRED;
}


uint8 s_MBCacheAddrMap[16] = 
{
        16 *0+0, 16 *0+4, 16 *0+8, 16 *0+12,
		16 *4+0, 16 *4+4,  16 *4+8, 16 *4+12, 
		16 *8+0, 16 *8+4, 16 *8+8, 16 *8+12,
		16 *12+0, 16 *12+4,  16 *12+8, 16 *12 + 12, 
};

uint8 s_MBUVCacheAddrMap[4] = 
{
        8 *0+0, 8 *0+4, 
		8 *4+0, 8 *4+4,
};

uint8 g_MBUVCacheAddrMap[4] = 
{
       0,				4, 
	MBC_C_SIZE*4, MBC_C_SIZE*4+4
};

extern void foo();

void AddPredAndResidual (uint8 *pred, int16 *coeff, int32 width)
{
	int32 i, j;
	uint8 *dst;
	int16 *src;
	int32 m, n;

	src = coeff;
	dst = pred;

	for (i = 0; i < 4; i++)
	{
		for (j = 0; j < 4; j++)
		{
			n = src[j];	
			m = pred[j];
			dst[j] = IClip(0, 255, m + n);
		}

		src += 4;
		pred += width;
		dst  += width; 		
	}
}

void init_ipred ()
{
#if defined(H264_DEC)
	InitIntra4x4LumaPredFunction();
	InitIntra8x8LumaPredFunction();//weihu
	InitIntra16x16LumaPredFunction();
	InitIntraChromaPredFunction();
#elif defined(REAL_DEC)
	RvDec_InitIntra4x4PredFunction();
	RvDec_InitIntra16x16PredFunction();
#else
#endif
}

void ipred_module ()
{
#if defined(H264_DEC)
	#include "h264dec_global.h"

	int32 mb_type = (g_mbc_reg_ptr->MBC_CMD0>>28)&0x3;
	int32 mb_x = ((g_glb_reg_ptr->VSP_CTRL0>>0) & 0x7f);//weihu //0x3f
	int32 mb_y = ((g_glb_reg_ptr->VSP_CTRL0>>8) & 0x7f);
	int32 mb_num_x	= (g_glb_reg_ptr->VSP_CFG1 >> 0) & 0xff;
	int32 mb_num_y	= (g_glb_reg_ptr->VSP_CFG1 >> 12) & 0xff;
	int32 frame_width = mb_num_x*MB_SIZE;
	uint8* rec_frame_y_ptr = g_dec_picture_ptr->imgY;
	uint8* rec_frame_uv_ptr = g_dec_picture_ptr->imgU; //uv interleaved
	uint8* mbc_bfr_ptr = (uint8 *)vsp_mbc_out_bfr;
	int16 *coeff = (int16 *)vsp_dct_io_0;

if(g_trace_enable_flag&TRACE_ENABLE_IPRED)	
{
	FPRINTF(g_fp_trace_ipred, "Current MB ipred_mode, mb_x = %d, mb_y = %d, No.%d frame, %d (0: I,  1:P)\n", mb_x, mb_y, g_nFrame_dec_h264, 0);
    FPRINTF(g_ipred_log_fp, "Current MB ipred_mode, mb_x = %d, mb_y = %d, No.%d frame, %d (0: I,  1:P)\n", mb_x, mb_y, g_nFrame_dec_h264, 0);
	FPRINTF(g_ipred_log_fp, "current MB type = %d[ INTRA16X16 = 0, INTRA4X4 = 1]\n", mb_type);
}
	if ((mb_x == 9) && (mb_y == 2) && (g_nFrame_dec_h264 == 0))
	{
		printf("");
	}

	if (mb_type == 2) //I_PCM
	{
		int32 i, j, k;
		int32 blkIdxInMB;
		uint8 * pMbc;
		int16 *p4x4blk;
		uint32 mbc_cmd0 = g_mbc_reg_ptr->MBC_CMD0;
		int16 * pBlkCff = (int16 *)vsp_dct_io_0;
		uint8 * pMbcBfr = (uint8 *)vsp_mbc_out_bfr;
		int32 uv;

		//luma
		//blkIdxInMB in follwing order
		//0,   1,   2,   3,
		//4,   5,   6,   7,
		//8,   9,  10,  11,
		//12, 13,  14,  15
		for (blkIdxInMB = 0; blkIdxInMB < 16; blkIdxInMB++)
		{
			i = blkIdxInMB>>2;
			j = blkIdxInMB&0x3;

			pMbc = pMbcBfr + 26*4 + i * 4 * MBC_Y_SIZE + j * 4;
			p4x4blk = pBlkCff + g_blk_rec_ord_tbl[blkIdxInMB] * 16;

			//add residual coef
			for (k = 0; k < 4; k++)
			{
				pMbc[0] = (uint8)p4x4blk[k*4+0];
				pMbc[1] = (uint8)p4x4blk[k*4+1];
				pMbc[2] = (uint8)p4x4blk[k*4+2];
				pMbc[3] = (uint8)p4x4blk[k*4+3];

				pMbc += MBC_Y_SIZE;
			}		
		}

		//chroma
		for (blkIdxInMB = 16; blkIdxInMB < 24; blkIdxInMB++)
		{
			if (blkIdxInMB < 20)
			{
				//16, 17
				//18, 19
				uv = 0;

				i = (blkIdxInMB - 16)/2;
				j = (blkIdxInMB - 16)%2;

				pMbc = pMbcBfr + MBC_U_OFFSET+18*4 + i*4*MBC_C_SIZE+j*4;
			}else
			{
				//20, 21,
				//22, 23
				
				uv = 1;

				i = (blkIdxInMB - 20)/2;
				j = (blkIdxInMB - 20)%2;

				pMbc = pMbcBfr + MBC_V_OFFSET+18*4 + i*4*MBC_C_SIZE+j*4;
			}

			p4x4blk = pBlkCff + blkIdxInMB * 16;

			//add residual coef
			for (k = 0; k < 4; k++)
			{
				pMbc[0] = (uint8)p4x4blk[k*4+0];
				pMbc[1] = (uint8)p4x4blk[k*4+1];
				pMbc[2] = (uint8)p4x4blk[k*4+2];
				pMbc[3] = (uint8)p4x4blk[k*4+3];

				pMbc += MBC_C_SIZE;
			}		
		}			

	}else //intra prediction
	{			
		int32 blkIdxInMB;
		int32 blk_coded;
 		uint8 *topBorder_luma, *topBorder_chroma[2];
		int32 i, j;
	// 	uint8 pred_y[16*16], pred_uv[2][8*8];
		int32 ipredmode_chroma = (g_mbc_reg_ptr->MBC_CMD3 & 0x3);
		uint8 tmp_pred_Y[16 *16]; //for trace
        uint8 tmp_pred_UV[2][8 *8];
		uint8 *pred;

		topBorder_luma = g_image_ptr->ipred_top_line_buffer + mb_x * MB_SIZE;
		topBorder_chroma[0] = g_image_ptr->ipred_top_line_buffer + mb_num_x * MB_SIZE + mb_x * MB_CHROMA_SIZE;
		topBorder_chroma[1] = g_image_ptr->ipred_top_line_buffer + mb_num_x * (MB_SIZE+MB_CHROMA_SIZE) + mb_x * MB_CHROMA_SIZE;

	if(g_trace_enable_flag&TRACE_ENABLE_IPRED)	
	{
		if (mb_type == 1) //i4x4
		{
			for (i = 0; i < 4; i++)
			{
				for (j = 0; j < 4; j++)
				{
					if ((i*4+j) <8)
					{
						FPRINTF(g_fp_trace_ipred, "%2d ", (g_mbc_reg_ptr->MBC_CMD1>>((7-(i*4+j))*4))&0xf);
						FPRINTF(g_ipred_log_fp, "%2d ", (g_mbc_reg_ptr->MBC_CMD1>>((7-(i*4+j))*4))&0xf);
					}else
					{
						FPRINTF(g_fp_trace_ipred, "%2d ", (g_mbc_reg_ptr->MBC_CMD2>>((15-(i*4+j))*4))&0xf);
						FPRINTF(g_ipred_log_fp, "%2d ", (g_mbc_reg_ptr->MBC_CMD2>>((15-(i*4+j))*4))&0xf);
					}
				}
				FPRINTF(g_fp_trace_ipred, "\n");
				FPRINTF(g_ipred_log_fp, "\n");
			}
			FPRINTF(g_fp_trace_ipred, "\n");
		}else if (mb_type == 0) //i16x16
		{
			FPRINTF(g_fp_trace_ipred, "i16x16 pred mode:%d\n",(g_mbc_reg_ptr->MBC_CMD1 & 0xf));
			FPRINTF(g_ipred_log_fp, "i16x16 pred mode:%d\n",(g_mbc_reg_ptr->MBC_CMD1 & 0xf));
		}else //ipcm
		{
			FPRINTF(g_fp_trace_ipred, "ipcm\n");
			FPRINTF(g_ipred_log_fp, "ipcm\n");
		}

		FPRINTF(g_ipred_log_fp, "chroma_ipred_mode: %d\n", ipredmode_chroma);
		FPRINTF(g_ipred_log_fp, "\n");
	}

		//load current mb's bordery data
			memcpy(luma_top+1, topBorder_luma, 16+8);//weihu 4->8
		luma_top[0] = top_left[0];

		if (mb_x == 0)
		{
			memset (luma_left, 0, 16);
			memset (chroma_left[0], 0, 8);
			memset (chroma_left[1], 0, 8);
		}

		if (mb_type == 0) //i16x16
		{
			int32 i16mode = (g_mbc_reg_ptr->MBC_CMD1 & 0xf);
			
			if(g_trace_enable_flag&TRACE_ENABLE_IPRED)	
			{
				FPRINTF(g_ipred_log_fp, "16x16 pred mode\n");

				FPRINTF(g_ipred_log_fp, "LUMA:\n");
			}
			pred = mbc_bfr_ptr + 26*4;
			s_intraPredLuma16x16[i16mode](pred, luma_top, luma_left);
			//memcpy(tmp_pred_Y, pred_y, 16 *16);
			for(i = 0; i < 16; i++)
			{
				for (j = 0; j < 16; j++)
				{
					tmp_pred_Y[16*i+j] = pred[24*i+j];
				}
			}
			Trace_after_intra_prediction_luma16x6 (tmp_pred_Y);
			for (blkIdxInMB = 0; blkIdxInMB < 16; blkIdxInMB++)
			{
				i = blkIdxInMB>>2;
				j = blkIdxInMB&0x3;

				pred = mbc_bfr_ptr + 26*4 + i * 4 * MBC_Y_SIZE + j * 4;

				if (1)
				{
					//add residual coef
					int16 *p4x4blk = coeff + g_blk_rec_ord_tbl[blkIdxInMB] * 16;
					AddPredAndResidual (pred, p4x4blk, MBC_Y_SIZE);
				}
			}
		}else if (mb_type == 1) //i4x4
		{
			IntraLuma4x4Pred *intraPredLuma4x4 = s_intraPredLuma4x4;
			int32 intrapred4x4mode;

		     if(g_trace_enable_flag&TRACE_ENABLE_IPRED)	
			 {
			     fprintf(g_ipred_log_fp, "LUMA:\n");
			 }

			//blkIdxInMB is in the following order
			//0,   1,   2,   3
			//4,   5,   6,   7
			//8,   9,  10,  11
			//12, 13,  14,  15
			for (blkIdxInMB = 0; blkIdxInMB < 16; blkIdxInMB++)
			{
				i = blkIdxInMB>>2;
				j = blkIdxInMB&0x3;

				if ((mb_x == 2) && (mb_y == 0) && (i == 2) && (j == 2))
				{
					printf("");
				}

				if(g_trace_enable_flag&TRACE_ENABLE_IPRED)	
				{
					fprintf(g_ipred_log_fp, "sub 4x4 block num = %d\n", blkIdxInMB);
				}

				if (blkIdxInMB <8)
				{
					intrapred4x4mode = (g_mbc_reg_ptr->MBC_CMD1>>((7-blkIdxInMB)*4))&0xf;
				}else
				{
					intrapred4x4mode = (g_mbc_reg_ptr->MBC_CMD2>>((15-blkIdxInMB)*4))&0xf;
				}
				
				pred = mbc_bfr_ptr + 26*4 + i * 4 * MBC_Y_SIZE + j * 4;//pred_y + i * 4 * MB_SIZE + j * 4;
				intraPredLuma4x4[intrapred4x4mode] (blkIdxInMB, pred, luma_top, luma_left);

				{
                    int ii, jj;
                    uint8 *tmp_pred = tmp_pred_Y + s_MBCacheAddrMap[blkIdxInMB];

                    for (ii = 0; ii < 4; ii++)
                    {
                        for (jj = 0; jj < 4; jj++)
                        {
                            tmp_pred[ii *MB_SIZE + jj] = pred[ii *MBC_Y_SIZE + jj];
                        }
                    }
                
					Trace_after_intra_prediction_Blk4x4(tmp_pred, MB_SIZE);
				}

				blk_coded = (g_mbc_reg_ptr->MBC_CMD0 >> (/*g_blk_rec_ord_tbl[*/blkIdxInMB/*]*/)) & 0x01;

				if (blk_coded)
				{
					//add residual coef
					int16 *p4x4blk = coeff + g_blk_rec_ord_tbl[blkIdxInMB] * 16;
					AddPredAndResidual (pred, p4x4blk, MBC_Y_SIZE);
				}
			}			
		}else if (mb_type == 4) //i8x8//weihu
		{
			IntraLuma8x8Pred *intraPredLuma8x8 = s_intraPredLuma8x8;
			int32 intrapred8x8mode;
			uint8 top[17], left[9];
			int k;
            uint8 *l_p;
			int avail_left,avail_top,avail_topleft,avail_topright;
			
			avail_top = (g_mbc_reg_ptr->MBC_CMD0 >> 25) & 0x01; //mbAvailB
			avail_left = (g_mbc_reg_ptr->MBC_CMD0 >> 26) & 0x01; //mbAvailA
			avail_topleft = (g_mbc_reg_ptr->MBC_CMD0 >> 27) & 0x01; //mbAvailD
			avail_topright = (g_mbc_reg_ptr->MBC_CMD0 >> 24) & 0x01; //mbAvailC
				
			//blkIdxInMB is in the following order
			//0,   1, 
			//2,   3,  
			for (blkIdxInMB = 0; blkIdxInMB < 4; blkIdxInMB++)
			{
				i = blkIdxInMB>>1;
				j = blkIdxInMB&0x1;
				
				if (blkIdxInMB <2)
				{
					intrapred8x8mode = (g_mbc_reg_ptr->MBC_CMD1>>((1-blkIdxInMB)*16))&0xf;
				}else
				{
					intrapred8x8mode = (g_mbc_reg_ptr->MBC_CMD2>>((3-blkIdxInMB)*16))&0xf;
				}
				
				pred = mbc_bfr_ptr + 26*4 + i * 8 * MBC_Y_SIZE + j * 8;//pred_y + i * 4 * MB_SIZE + j * 4;
				
                if (blkIdxInMB==0) 
				{
					for(k=0;k<17;k++)
						top[k]=luma_top[k];
					for(k=0;k<8;k++)
						left[k]=luma_left[k];					
                }
				else if(blkIdxInMB==1) 
				{
					for(k=0;k<9;k++)
						top[k]=luma_top[k+8];
					if(avail_topright)
					{
						for(k=9;k<17;k++)
							top[k]=luma_top[k+8];
					}
					else
					{
						for(k=9;k<17;k++)
							top[k]=luma_top[16];
					}
					for(k=0;k<8;k++)
					{
						l_p = pred - 1 + k * MBC_Y_SIZE;
						left[k]=*l_p;
					}					
                }
				else if(blkIdxInMB==2) 
				{
					for(k=0;k<8;k++)
						left[k]=luma_left[k+8];
					l_p	=pred - 1-MBC_Y_SIZE;	
					for(k=0;k<17;k++)
						top[k]=l_p[k];
				}
				else
				{
					for(k=0;k<8;k++)
					{
						l_p = pred - 1 + k * MBC_Y_SIZE;
						left[k]=*l_p;
					}
					l_p = pred - 1 - MBC_Y_SIZE;
					for(k=0;k<9;k++)
						top[k]=l_p[k];
					for(k=9;k<17;k++)
						top[k]=l_p[8];					

				}


				intraPredLuma8x8[intrapred8x8mode] (blkIdxInMB, pred, luma_top, luma_left);
				
				{
                    int ii, jj;
                    //uint8 *tmp_pred = tmp_pred_Y + s_MBCacheAddrMap[blkIdxInMB];
					uint8 *tmp_pred = tmp_pred_Y + s_MBCacheAddrMap[i*8+j*2];//weihu
					
                    for (ii = 0; ii < 8; ii++)
                    {
                        for (jj = 0; jj < 8; jj++)
                        {
                            tmp_pred[ii *MB_SIZE + jj] = pred[ii *MBC_Y_SIZE + jj];
                        }
                    }
					
					Trace_after_intra_prediction_Blk8x8(tmp_pred, MB_SIZE);
				}
				
				blk_coded = (g_mbc_reg_ptr->MBC_CMD0 >> (/*g_blk_rec_ord_tbl[*/blkIdxInMB/*]*/)) & 0x01;
				
				if (blk_coded)
				{
					//add residual coef
					int16 *p4x4blk = coeff + g_blk_rec_ord_tbl[blkIdxInMB] * 16;
					AddPredAndResidual (pred, p4x4blk, MBC_Y_SIZE);
				}
			}			
		}//ipred 8x8






		//chroma
		if(g_trace_enable_flag&TRACE_ENABLE_IPRED)	
		{
			fprintf(g_ipred_log_fp, "CHROMA:\n");
		}
	//	for (uv = 0; uv < 2; uv++)
		{
			//u
			memcpy (chroma_top[0]+1, topBorder_chroma[0], 12);
			*(chroma_top[0]) =  top_left[1];


			pred = mbc_bfr_ptr + MBC_U_OFFSET+18*4;
			s_intraPredChroma8x8[ipredmode_chroma](pred, chroma_top[0], chroma_left[0]);

			//memcpy(tmp_pred_UV[0], pred_uv[0], 8*8);
			for (i = 0; i < 8; i++)
			{
				for (j = 0; j < 8; j++)
				{
					tmp_pred_UV[0][i*8+j] = pred[i*MBC_C_SIZE+j];
				}
			}
			Trace_after_intra_prediction_chroma8x8 (tmp_pred_UV[0]);

		//	for(uv = 0; uv < 2; uv++)
			{
				for (blkIdxInMB = 0; blkIdxInMB < 4; blkIdxInMB++)
				{
					blk_coded = (g_mbc_reg_ptr->MBC_CMD0 >> (/*g_blk_rec_ord_tbl[*/16+(0*4)+blkIdxInMB/*]*/)) & 0x01;
					if (blk_coded)
					{
						//add residual coef
						int16 *p4x4blk = coeff + 16 * 16 + ((0*4)+blkIdxInMB) * 16;
						AddPredAndResidual (pred+g_MBUVCacheAddrMap[blkIdxInMB], p4x4blk, MBC_C_SIZE);
					}
				}
			}			
		}

		{
			//v
			memcpy (chroma_top[1]+1, topBorder_chroma[1], 12);
			*(chroma_top[1]) =  top_left[2];


			pred = mbc_bfr_ptr + MBC_V_OFFSET+18*4;
			s_intraPredChroma8x8[ipredmode_chroma](pred, chroma_top[1], chroma_left[1]);

			//memcpy(tmp_pred_UV[1], pred_uv[1], 8*8);
			for (i = 0; i < 8; i++)
			{
				for (j = 0; j < 8; j++)
				{
					tmp_pred_UV[1][i*8+j] = pred[i*MBC_C_SIZE+j];
				}
			}
			Trace_after_intra_prediction_chroma8x8 (tmp_pred_UV[1]);

			//for(uv = 0; uv < 2; uv++)
			{
				for (blkIdxInMB = 0; blkIdxInMB < 4; blkIdxInMB++)
				{
					blk_coded = (g_mbc_reg_ptr->MBC_CMD0 >> (/*g_blk_rec_ord_tbl[*/16+(1*4)+blkIdxInMB/*]*/)) & 0x01;
					if (blk_coded)
					{
						//add residual coef
						int16 *p4x4blk = coeff + 16 * 16 + ((1*4)+blkIdxInMB) * 16;
						AddPredAndResidual (pred+g_MBUVCacheAddrMap[blkIdxInMB], p4x4blk, MBC_C_SIZE);
					}
				}
			}			
		}
		
		//only ipred
		trace_ipred (tmp_pred_Y, tmp_pred_UV[0], tmp_pred_UV[1]);

		//ipred+residual
		pred = mbc_bfr_ptr + 26*4; //y
		for (i = 0; i < 16; i++)
        {
			for (j = 0; j < 16; j++)
			{
				tmp_pred_Y[i*MB_SIZE+j] = pred[i*MBC_Y_SIZE+j];
			}
		}
		pred = mbc_bfr_ptr + MBC_U_OFFSET +18*4; //u
		for (i = 0; i < 8; i++)
        {
			for (j = 0; j < 8; j++)
			{
				tmp_pred_UV[0][i*MB_CHROMA_SIZE+j] = pred[i*MBC_C_SIZE+j];
			}
		}
		pred = mbc_bfr_ptr + MBC_V_OFFSET +18*4; //v
		for (i = 0; i < 8; i++)
        {
			for (j = 0; j < 8; j++)
			{
				tmp_pred_UV[1][i*MB_CHROMA_SIZE+j] = pred[i*MBC_C_SIZE+j];
			}
		}                
	 	trace_ipred (tmp_pred_Y, tmp_pred_UV[0], tmp_pred_UV[1]);
	}
#endif
#if defined(REAL_DEC)// REAL_DEC
#include "rvdec_global.h"
	int32 mb_type = (g_mbc_reg_ptr->MBC_CMD0>>28)&0x3;
	int32 mb_x = ((g_glb_reg_ptr->VSP_CTRL0>>0) & 0x7f);//weihu //0x3f
	int32 mb_y = ((g_glb_reg_ptr->VSP_CTRL0>>8) & 0x7f);
	int32 mb_num_x	= (g_glb_reg_ptr->VSP_CFG1 >> 0) & 0xff;
	int32 mb_num_y	= (g_glb_reg_ptr->VSP_CFG1 >> 12) & 0xff;
	int32 frame_width = mb_num_x*MB_SIZE;
	uint8* mbc_bfr_ptr = (uint8 *)vsp_mbc_out_bfr;
	int16 *coeff = (int16 *)vsp_dct_io_0;

// if(g_trace_enable_flag&TRACE_ENABLE_IPRED)	
// {
// 	fprintf(g_fp_trace_ipred, "Current MB ipred_mode, mb_x = %d, mb_y = %d, No.%d frame, %d (0: I,  1:P)\n", mb_x, mb_y, g_nFrame_dec_h264, 0);
//     fprintf(g_ipred_log_fp, "Current MB ipred_mode, mb_x = %d, mb_y = %d, No.%d frame, %d (0: I,  1:P)\n", mb_x, mb_y, g_nFrame_dec_h264, 0);
// 	fprintf(g_ipred_log_fp, "current MB type = %d[ INTRA16X16 = 0, INTRA4X4 = 1]\n", mb_type);
// }
// 	if ((mb_x == 9) && (mb_y == 2) && (g_nFrame_dec_h264 == 0))
// 	{
// 		printf("");
// 	}

	 //intra prediction
	{			
		int32 blkIdxInMB;
		int32 blk_coded;
 		uint8 *topBorder_luma, *topBorder_chroma[2];
		int32 i, j;
	// 	uint8 pred_y[16*16], pred_uv[2][8*8];
		int32 ipredmode_chroma ;
		uint8 tmp_pred_Y[16 *16]; //for trace
        uint8 tmp_pred_UV[2][8 *8];
		uint8 *pred;
		Intra16x16Pred *intraPredLuma16x16 = g_intraPredLuma16x16;
		Intra16x16Pred *intraPredChroma8x8 = g_intraPredChroma8x8;
		Intra4x4Pred *intraPred4x4 = g_intraPred4x4;
		topBorder_luma = g_rv_decoder_ptr->ipred_top_line_buffer + mb_x * MB_SIZE;
		topBorder_chroma[0] = g_rv_decoder_ptr->ipred_top_line_buffer + mb_num_x * MB_SIZE + mb_x * MB_CHROMA_SIZE;
		topBorder_chroma[1] = g_rv_decoder_ptr->ipred_top_line_buffer + mb_num_x * (MB_SIZE+MB_CHROMA_SIZE) + mb_x * MB_CHROMA_SIZE;

	if(g_trace_enable_flag&TRACE_ENABLE_IPRED)	
	{
		if (mb_type == 1) //i4x4
		{
			for (i = 0; i < 4; i++)
			{
				for (j = 0; j < 4; j++)
				{
					if ((i*4+j) <8)
					{
						FPRINTF(g_fp_trace_ipred, "%2d ", (g_mbc_reg_ptr->MBC_CMD1>>((7-(i*4+j))*4))&0xf);
						FPRINTF(g_ipred_log_fp, "%2d ", (g_mbc_reg_ptr->MBC_CMD1>>((7-(i*4+j))*4))&0xf);
					}else
					{
						FPRINTF(g_fp_trace_ipred, "%2d ", (g_mbc_reg_ptr->MBC_CMD2>>((15-(i*4+j))*4))&0xf);
						FPRINTF(g_ipred_log_fp, "%2d ", (g_mbc_reg_ptr->MBC_CMD2>>((15-(i*4+j))*4))&0xf);
					}
				}
				FPRINTF(g_fp_trace_ipred, "\n");
				FPRINTF(g_ipred_log_fp, "\n");
			}
			FPRINTF(g_fp_trace_ipred, "\n");
		}else if (mb_type == 0) //i16x16
		{
			FPRINTF(g_fp_trace_ipred, "i16x16 pred mode:%d\n",(g_mbc_reg_ptr->MBC_CMD1 & 0xf));
			FPRINTF(g_ipred_log_fp, "i16x16 pred mode:%d\n",(g_mbc_reg_ptr->MBC_CMD1 & 0xf));
		}else //ipcm
		{
			FPRINTF(g_fp_trace_ipred, "ipcm\n");
			FPRINTF(g_ipred_log_fp, "ipcm\n");
		}

		FPRINTF(g_ipred_log_fp, "chroma_ipred_mode: %d\n", ipredmode_chroma);
		FPRINTF(g_ipred_log_fp, "\n");
	}

		//load current mb's bordery data
		memcpy(luma_top+1, topBorder_luma, 16+4);
		luma_top[0] = top_left[0];

		if (mb_x == 0)
		{
			memset (luma_left, 0, 16);
			memset (chroma_left[0], 0, 8);
			memset (chroma_left[1], 0, 8);
		}

		if (mb_type == 0) //i16x16
		{
			int32 i16mode = (g_mbc_reg_ptr->MBC_CMD2 & 0xf);
			
			if(g_trace_enable_flag&TRACE_ENABLE_IPRED)	
			{
				FPRINTF(g_ipred_log_fp, "16x16 pred mode\n");

				FPRINTF(g_ipred_log_fp, "LUMA:\n");
			}
			pred = mbc_bfr_ptr + MBC_Y_DATA_OFFSET;
			intraPredLuma16x16[i16mode]((luma_top + 1), luma_left, luma_top[0],pred);
			//memcpy(tmp_pred_Y, pred_y, 16 *16);
			for(i = 0; i < 16; i++)
			{
				for (j = 0; j < 16; j++)
				{
					tmp_pred_Y[16*i+j] = pred[MBC_Y_SIZE*i+j];
				}
			}
			Trace_after_intra_prediction_luma16x6 (tmp_pred_Y);
			for (blkIdxInMB = 0; blkIdxInMB < 16; blkIdxInMB++)
			{
				int32 blk_coded = (g_mbc_reg_ptr->MBC_CMD0 >> (blkIdxInMB)) & 0x01;
				i = blkIdxInMB>>2;
				j = blkIdxInMB&0x3;

				pred = mbc_bfr_ptr + 26*4 + i * 4 * MBC_Y_SIZE + j * 4;

		//		if (1)
				if (blk_coded)
				{
					//add residual coef
					int16 *p4x4blk = coeff + g_blk_rec_ord_tbl[blkIdxInMB] * 16;
					AddPredAndResidual (pred, p4x4blk, MBC_Y_SIZE);
				}
			}

			//chroma
			if(g_trace_enable_flag&TRACE_ENABLE_IPRED)	
			{
				fprintf(g_ipred_log_fp, "CHROMA:\n");
			}

			//u
			memcpy (chroma_top[0]+1, topBorder_chroma[0], 12);
			*(chroma_top[0]) =  top_left[1];

			
			pred = mbc_bfr_ptr + MBC_U_OFFSET+MBC_C_DATA_OFFSET;
			intraPredChroma8x8[i16mode]((chroma_top[0]+1), chroma_left[0], chroma_top[0][0], pred);
		
			//memcpy(tmp_pred_UV[0], pred_uv[0], 8*8);
			for (i = 0; i < 8; i++)
			{
				for (j = 0; j < 8; j++)
				{
					tmp_pred_UV[0][i*8+j] = pred[i*MBC_C_SIZE+j];
				}
			}
			Trace_after_intra_prediction_chroma8x8 (tmp_pred_UV[0]);

		//	for(uv = 0; uv < 2; uv++)
			{
				for (blkIdxInMB = 0; blkIdxInMB < 4; blkIdxInMB++)
				{
					blk_coded = (g_mbc_reg_ptr->MBC_CMD0 >> (/*g_blk_rec_ord_tbl[*/16+(0*4)+blkIdxInMB/*]*/)) & 0x01;
					if (blk_coded)
					{
						//add residual coef
						int16 *p4x4blk = coeff + 16 * 16 + ((0*4)+blkIdxInMB) * 16;
						AddPredAndResidual (pred+g_MBUVCacheAddrMap[blkIdxInMB], p4x4blk, MBC_C_SIZE);
					}
				}
			}			

			//v
			memcpy (chroma_top[1]+1, topBorder_chroma[1], 12);
			*(chroma_top[1]) =  top_left[2];

			pred = mbc_bfr_ptr + MBC_V_OFFSET+MBC_C_DATA_OFFSET;
			intraPredChroma8x8[i16mode]((chroma_top[1]+1), chroma_left[1], chroma_top[1][0], pred);

			//memcpy(tmp_pred_UV[1], pred_uv[1], 8*8);
			for (i = 0; i < 8; i++)
			{
				for (j = 0; j < 8; j++)
				{
					tmp_pred_UV[1][i*8+j] = pred[i*MBC_C_SIZE+j];
				}
			}
			Trace_after_intra_prediction_chroma8x8 (tmp_pred_UV[1]);

			//for(uv = 0; uv < 2; uv++)
			{
				for (blkIdxInMB = 0; blkIdxInMB < 4; blkIdxInMB++)
				{
					blk_coded = (g_mbc_reg_ptr->MBC_CMD0 >> (/*g_blk_rec_ord_tbl[*/16+(1*4)+blkIdxInMB/*]*/)) & 0x01;
					if (blk_coded)
					{
						//add residual coef
						int16 *p4x4blk = coeff + 16 * 16 + ((1*4)+blkIdxInMB) * 16;
						AddPredAndResidual (pred+g_MBUVCacheAddrMap[blkIdxInMB], p4x4blk, MBC_C_SIZE);
					}
				}
			}			
		}else if (mb_type == 1) //i4x4
		{
			int32 intrapred4x4mode;

			if(g_trace_enable_flag&TRACE_ENABLE_IPRED)	
			{
				fprintf(g_ipred_log_fp, "LUMA:\n");
			}

			//blkIdxInMB is in the following order
			//0,   1,   2,   3
			//4,   5,   6,   7
			//8,   9,  10,  11
			//12, 13,  14,  15
			for (blkIdxInMB = 0; blkIdxInMB < 16; blkIdxInMB++)
			{
				i = blkIdxInMB>>2;
				j = blkIdxInMB&0x3;

				if ((mb_x == 2) && (mb_y == 0) && (i == 2) && (j == 2))
				{
					printf("");
				}

				if(g_trace_enable_flag&TRACE_ENABLE_IPRED)	
				{
					fprintf(g_ipred_log_fp, "sub 4x4 block num = %d\n", blkIdxInMB);
				}

				if (blkIdxInMB <8)
				{
					intrapred4x4mode = (g_mbc_reg_ptr->MBC_CMD1>>((7-blkIdxInMB)*4))&0xf;
				}else
				{
					intrapred4x4mode = (g_mbc_reg_ptr->MBC_CMD2>>((15-blkIdxInMB)*4))&0xf;
				}
				
				pred = mbc_bfr_ptr + MBC_Y_DATA_OFFSET + i * 4 * MBC_Y_SIZE + j * 4;//pred_y + i * 4 * MB_SIZE + j * 4;
				
				get_luma_nei_pixels(pred, (luma_top+1), luma_left, luma_top[0], blkIdxInMB);

				intraPred4x4[intrapred4x4mode](pred, MBC_Y_SIZE);
				
				{
                    int ii, jj;
                    uint8 *tmp_pred = tmp_pred_Y + s_MBCacheAddrMap[blkIdxInMB];

                    for (ii = 0; ii < 4; ii++)
                    {
                        for (jj = 0; jj < 4; jj++)
                        {
                            tmp_pred[ii *MB_SIZE + jj] = pred[ii *MBC_Y_SIZE + jj];
                        }
                    }
                
					Trace_after_intra_prediction_Blk4x4(tmp_pred, MB_SIZE);
				}

				blk_coded = (g_mbc_reg_ptr->MBC_CMD0 >> (/*g_blk_rec_ord_tbl[*/blkIdxInMB/*]*/)) & 0x01;

				if (blk_coded)
				{
					//add residual coef
					int16 *p4x4blk = coeff + g_blk_rec_ord_tbl[blkIdxInMB] * 16;
					AddPredAndResidual (pred, p4x4blk, MBC_Y_SIZE);
				}
			}
			
			//chroma
			if(g_trace_enable_flag&TRACE_ENABLE_IPRED)	
			{
				fprintf(g_ipred_log_fp, "CHROMA:\n");
			}

			//u
			memcpy (chroma_top[0]+1, topBorder_chroma[0], 12);
			*(chroma_top[0]) =  top_left[1];

			for (blkIdxInMB = 0; blkIdxInMB < 4; blkIdxInMB++)
			{
				if (blkIdxInMB == 0)
				{
					intrapred4x4mode = (g_mbc_reg_ptr->MBC_CMD1>>28)&0xf;
				}
				else if (blkIdxInMB == 1)
				{
					intrapred4x4mode = (g_mbc_reg_ptr->MBC_CMD1>>20)&0xf;
				}
				else if (blkIdxInMB == 2)
				{
					intrapred4x4mode = (g_mbc_reg_ptr->MBC_CMD2>>28)&0xf;
				}
				else
				{
					intrapred4x4mode = (g_mbc_reg_ptr->MBC_CMD2>>20)&0xf;
				}

				i = blkIdxInMB>>1;
				j = blkIdxInMB&0x1;

				pred = mbc_bfr_ptr + MBC_U_OFFSET + MBC_C_DATA_OFFSET + i * 4 * MBC_C_SIZE + j * 4;//pred_y + i * 4 * MB_SIZE + j * 4;

				get_chroma_nei_pixels(pred, (chroma_top[0]+1), chroma_left[0], chroma_top[0][0], blkIdxInMB);
				
				intraPred4x4[intrapred4x4mode](pred,MBC_C_SIZE);

				{
                    int ii, jj;
                    uint8 *tmp_pred = tmp_pred_UV[0] + s_MBUVCacheAddrMap[blkIdxInMB];

                    for (ii = 0; ii < 4; ii++)
                    {
                        for (jj = 0; jj < 4; jj++)
                        {
                            tmp_pred[ii *MB_CHROMA_SIZE + jj] = pred[ii *MBC_C_SIZE + jj];
                        }
                    }
                
					Trace_after_intra_prediction_Blk4x4(tmp_pred, MB_CHROMA_SIZE);
				}
							
				blk_coded = (g_mbc_reg_ptr->MBC_CMD0 >> (/*g_blk_rec_ord_tbl[*/16+(0*4)+blkIdxInMB/*]*/)) & 0x01;
				if (blk_coded)
				{
					//add residual coef
					int16 *p4x4blk = coeff + 16 * 16 + ((0*4)+blkIdxInMB) * 16;
					AddPredAndResidual (pred/*+g_MBUVCacheAddrMap[blkIdxInMB]*/, p4x4blk, MBC_C_SIZE);
				}
			}

			//v
			memcpy (chroma_top[1]+1, topBorder_chroma[1], 12);
			*(chroma_top[1]) =  top_left[2];

			for (blkIdxInMB = 0; blkIdxInMB < 4; blkIdxInMB++)
			{
				if (blkIdxInMB == 0)
				{
					intrapred4x4mode = (g_mbc_reg_ptr->MBC_CMD1>>28)&0xf;
				}
				else if (blkIdxInMB == 1)
				{
					intrapred4x4mode = (g_mbc_reg_ptr->MBC_CMD1>>20)&0xf;
				}
				else if (blkIdxInMB == 2)
				{
					intrapred4x4mode = (g_mbc_reg_ptr->MBC_CMD2>>28)&0xf;
				}
				else
				{
					intrapred4x4mode = (g_mbc_reg_ptr->MBC_CMD2>>20)&0xf;
				}

				i = blkIdxInMB>>1;
				j = blkIdxInMB&0x1;

				pred = mbc_bfr_ptr + MBC_V_OFFSET + MBC_C_DATA_OFFSET + i * 4 * MBC_C_SIZE + j * 4;//pred_y + i * 4 * MB_SIZE + j * 4;

				get_chroma_nei_pixels(pred, (chroma_top[1]+1), chroma_left[1], chroma_top[1][0], blkIdxInMB);
				
				intraPred4x4[intrapred4x4mode](pred,MBC_C_SIZE);

				{
                    int ii, jj;
                    uint8 *tmp_pred = tmp_pred_UV[1] + s_MBUVCacheAddrMap[blkIdxInMB];

                    for (ii = 0; ii < 4; ii++)
                    {
                        for (jj = 0; jj < 4; jj++)
                        {
                            tmp_pred[ii *MB_CHROMA_SIZE + jj] = pred[ii *MBC_C_SIZE + jj];
                        }
                    }
                
					Trace_after_intra_prediction_Blk4x4(tmp_pred, MB_CHROMA_SIZE);
				}
							
				blk_coded = (g_mbc_reg_ptr->MBC_CMD0 >> (/*g_blk_rec_ord_tbl[*/16+(1*4)+blkIdxInMB/*]*/)) & 0x01;
				if (blk_coded)
				{
					//add residual coef
					int16 *p4x4blk = coeff + 16 * 16 + ((1*4)+blkIdxInMB) * 16;
					AddPredAndResidual (pred/*+g_MBUVCacheAddrMap[blkIdxInMB]*/, p4x4blk, MBC_C_SIZE);
				}
			}		
		}
	
		//only ipred
		trace_ipred (tmp_pred_Y, tmp_pred_UV[0], tmp_pred_UV[1]);

		//ipred+residual
		pred = mbc_bfr_ptr + 21*4; //y
		for (i = 0; i < 16; i++)
        {
			for (j = 0; j < 16; j++)
			{
				tmp_pred_Y[i*MB_SIZE+j] = pred[i*MBC_Y_SIZE+j];
			}
		}
		pred = mbc_bfr_ptr + MBC_U_OFFSET +13*4; //u
		for (i = 0; i < 8; i++)
        {
			for (j = 0; j < 8; j++)
			{
				tmp_pred_UV[0][i*MB_CHROMA_SIZE+j] = pred[i*MBC_C_SIZE+j];
			}
		}
		pred = mbc_bfr_ptr + MBC_V_OFFSET +13*4; //v
		for (i = 0; i < 8; i++)
        {
			for (j = 0; j < 8; j++)
			{
				tmp_pred_UV[1][i*MB_CHROMA_SIZE+j] = pred[i*MBC_C_SIZE+j];
			}
		}                
	 	trace_ipred (tmp_pred_Y, tmp_pred_UV[0], tmp_pred_UV[1]);
	}
#endif

	return;
}