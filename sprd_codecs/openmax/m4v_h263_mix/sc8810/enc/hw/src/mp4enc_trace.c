#include "video_common.h"
#include "mp4enc_mode.h"
#include "mp4enc_global.h"

#if _CMODEL_
#include "common_global.h"
#endif

#if defined(SIM_IN_WIN)



void Mp4Enc_Trace_MBC_result_one_macroblock(ENC_VOP_MODE_T *vop_mode_ptr, ENC_MB_MODE_T *mb_mode_ptr)
{
#if defined(SIM_IN_WIN)
	if(g_vector_enable_flag&VECTOR_ENABLE_MBC)
	{
		uint8 *ppxlcRecMBY, *ppxlcRecMBU, *ppxlcRecMBV;
		uint8 iMbIndX = vop_mode_ptr->mb_x;
		uint8 iMbIndY = vop_mode_ptr->mb_y;
		int32 dstWidth = vop_mode_ptr->FrameWidth;	
		int32 ii, jj;
		uint8 *pLine;
		static uint32 mbc_trace_line = 0;

		ppxlcRecMBY = vop_mode_ptr->pYUVRecFrame->imgY + (iMbIndY * MB_SIZE) * dstWidth + (iMbIndX * MB_SIZE) ;
		ppxlcRecMBU = vop_mode_ptr->pYUVRecFrame->imgU + (iMbIndY * BLOCK_SIZE) * (dstWidth>>1) + (iMbIndX * BLOCK_SIZE);
		ppxlcRecMBV = vop_mode_ptr->pYUVRecFrame->imgU + (iMbIndY * BLOCK_SIZE) * (dstWidth>>1) + (iMbIndX * BLOCK_SIZE);
		
		for (ii = 0; ii < MB_SIZE; ii++)
		{
			pLine = ppxlcRecMBY + ii * dstWidth;
			for(jj = 0; jj < 4; jj++)
			{
				FomatPrint4Pix(pLine, g_fp_mbc_tv);
				pLine += 4;
				mbc_trace_line++;
			}			
		}

		//u
		for (ii = 0; ii < BLOCK_SIZE; ii++)
		{
			pLine = ppxlcRecMBU + ii * (dstWidth/2);
			for(jj = 0; jj < 2; jj++)
			{
				
				FomatPrint4Pix(pLine, g_fp_mbc_tv);
				pLine += 4;
				mbc_trace_line++;
			}			
		}

		//V
		for (ii = 0; ii < BLOCK_SIZE; ii++)
		{
			pLine = ppxlcRecMBV + ii * (dstWidth/2);
			for(jj = 0; jj < 2; jj++)
			{
				FomatPrint4Pix(pLine, g_fp_mbc_tv);
				pLine += 4;
				mbc_trace_line++;
			}			
		}
	}
#endif // SIM_IN_WIN   
}

#if defined(SIM_IN_WIN)
void printf_blkChar_MB (FILE * pFile, uint8 * pblkMB [6])
{
	int i, j;
	uint8 * pBlk;

	for (i = 0; i < 6; i++)
	{
		pBlk = pblkMB [i];

		for (j = 0; j < 8; j++)
		{
			FPRINTF (pFile, "%02x %02x %02x %02x %02x %02x %02x %02x\n",
				pBlk [0], pBlk [1], pBlk [2], pBlk [3], pBlk [4], pBlk [5], pBlk [6], pBlk [7]);
			
			pBlk += 8;
		}

		FPRINTF (pFile, "\n");
	}
}
#endif // SIM_IN_WIN   

void Mp4Enc_Trace_MEA_result_one_macroblock(ENC_VOP_MODE_T *vop_mode_ptr)
{
#if defined(SIM_IN_WIN)
	if(g_vector_enable_flag&VECTOR_ENABLE_MBC)
	{
		printf_blkChar_MB (g_fp_trace_mea_src_mb, vop_mode_ptr->pMBCache->pSrcMb);
		printf_blkChar_MB (g_fp_trace_mea_ref_mb, vop_mode_ptr->pMBCache->pRefMb);
	}
#endif // SIM_IN_WIN   
}


void Mp4Enc_Trace_OneBlkIDctCoef_forASICCompare(int16 *pBlk)
{
#if defined(SIM_IN_WIN)
	if(g_vector_enable_flag&VECTOR_ENABLE_DCT)
	{
		int ii,jj;
		uint32 tmp;
					
		for(ii = 0; ii < BLOCK_SIZE; ii++)
		{
			for(jj = 0; jj < (BLOCK_SIZE/2); jj++)
			{
				int16 tmp1;
				int16 tmp2;

				
				tmp1 = Clip3(-256,255,pBlk[ii * BLOCK_SIZE + jj * 2 + 1]);
				tmp2 = Clip3(-256,255,pBlk[ii * BLOCK_SIZE + jj * 2 +0]);

				tmp = (((tmp1 & 0xffff)<<16)|(tmp2 & 0xffff));
				FormatPrint32bitNum(tmp, g_fp_idct_tv);
			}
		}
	}
#endif // SIM_IN_WIN   

}

#endif // SIM_IN_WIN   

