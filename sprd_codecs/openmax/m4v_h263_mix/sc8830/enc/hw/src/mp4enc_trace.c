#include "video_common.h"
#include "mp4enc_mode.h"
#include "mp4enc_global.h"

#if _CMODEL_
#include "common_global.h"
#endif

#if defined(SIM_IN_WIN)
void FormatPrint32bitNum(uint32 num, FILE *fp)
{
	uint8 bit8;
	uint32 i;

//	fprintf(fp, "0x");

	for(i = 0; i < 4; i++)
	{
		bit8 = (num >> (24 - 8 * i))&0xff ; 

		if(bit8 < 0x10)
		{
			fprintf(fp, "0");
		}
		fprintf(fp, "%x", bit8);
	}

	fprintf(fp, "\n");
}
#endif // SIM_IN_WIN   


void Mp4Enc_Trace_MBC_result_one_macroblock(ENC_VOP_MODE_T *pVop_mode, ENC_MB_MODE_T *pMb_mode)
{
#if defined(SIM_IN_WIN)
	if(g_vector_enable_flag&VECTOR_ENABLE_MBC)
	{
		uint8 *ppxlcRecMBY, *ppxlcRecMBU, *ppxlcRecMBV;
		uint8 iMbIndX = pVop_mode->mb_x;
		uint8 iMbIndY = pVop_mode->mb_y;
		int32 dstWidth = pVop_mode->FrameWidth;	
		int32 ii, jj;
		uint8 *pLine;
		static uint32 mbc_trace_line = 0;

		ppxlcRecMBY = pVop_mode->pYUVRecFrame->imgY + (iMbIndY * MB_SIZE) * dstWidth + (iMbIndX * MB_SIZE) ;
		ppxlcRecMBU = pVop_mode->pYUVRecFrame->imgU + (iMbIndY * BLOCK_SIZE) * (dstWidth>>1) + (iMbIndX * BLOCK_SIZE);
		ppxlcRecMBV = pVop_mode->pYUVRecFrame->imgU + (iMbIndY * BLOCK_SIZE) * (dstWidth>>1) + (iMbIndX * BLOCK_SIZE) + 1;

		fprintf(g_fp_mbc_tv, "//frame_cnt=%d, mb_x=%d, mb_y=%d\n", g_nFrame_enc, pVop_mode->mb_x, pVop_mode->mb_y);
		
		for (ii = 0; ii < MB_SIZE; ii++)
		{
			pLine = ppxlcRecMBY + ii * dstWidth;
			/*for(jj = 0; jj < 4; jj++)
			{
				FomatPrint4Pix(pLine, g_fp_mbc_tv);
				pLine += 4;
				mbc_trace_line++;
			}*/	
			for(jj=7;jj>=0;jj--)
			{
				fprintf(g_fp_mbc_tv, "%02x", pLine[jj]);
			}
			fprintf(g_fp_mbc_tv, "\n", pLine[jj]);
			for(jj=15;jj>=8;jj--)
			{
				fprintf(g_fp_mbc_tv, "%02x", pLine[jj]);
			}
			fprintf(g_fp_mbc_tv, "\n", pLine[jj]);
			pLine += 16;
		}

		//u
		for (ii = 0; ii < BLOCK_SIZE; ii++)
		{
			pLine = ppxlcRecMBU + ii * (dstWidth/2);
			/*for(jj = 0; jj < 2; jj++)
			{
				
				FomatPrint4Pix(pLine, g_fp_mbc_tv);
				pLine += 4;
				mbc_trace_line++;
			}*/
			for(jj=7;jj>=0;jj--)
			{
				fprintf(g_fp_mbc_tv, "%02x", pLine[jj*2]);
			}
			fprintf(g_fp_mbc_tv, "\n", pLine[jj]);
			pLine += 8*2;
		}

		//V
		for (ii = 0; ii < BLOCK_SIZE; ii++)
		{
			pLine = ppxlcRecMBV + ii * (dstWidth/2);
			/*for(jj = 0; jj < 2; jj++)
			{
				FomatPrint4Pix(pLine, g_fp_mbc_tv);
				pLine += 4;
				mbc_trace_line++;
			}*/
			for(jj=7;jj>=0;jj--)
			{
				fprintf(g_fp_mbc_tv, "%02x", pLine[jj*2]);
			}
			fprintf(g_fp_mbc_tv, "\n", pLine[jj]);
			pLine += 8*2;
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

void Mp4Enc_Trace_MEA_result_one_macroblock(ENC_VOP_MODE_T *pVop_mode)
{
#if defined(SIM_IN_WIN)
	if(g_vector_enable_flag&VECTOR_ENABLE_MBC)
	{
		printf_blkChar_MB (g_fp_trace_mea_src_mb, pVop_mode->pMBCache->pSrcMb);
		printf_blkChar_MB (g_fp_trace_mea_ref_mb, pVop_mode->pMBCache->pRefMb);
	}
#endif // SIM_IN_WIN   
}

#if defined(SIM_IN_WIN)
void printf_MB (FILE * pfOutMB, uint8 * pMBY, uint8 pMBUV[2][64])
{
	int i;
	uint32 *pIntdata;
	
	/*MBY*/
	pIntdata = (uint32 *)pMBY;
	for (i = 0; i < 16; i++)
	{
		FPRINTF (pfOutMB, "%08x\n", *pIntdata++);
		FPRINTF (pfOutMB, "%08x\n", *pIntdata++);
		FPRINTF (pfOutMB, "%08x\n", *pIntdata++);
		FPRINTF (pfOutMB, "%08x\n", *pIntdata++);
	}
	
	/*MBU*/
	pIntdata = (uint32 *)pMBUV[0];
	for (i = 0; i < 8; i++)
	{
		FPRINTF (pfOutMB, "%08x\n", *pIntdata++);
		FPRINTF (pfOutMB, "%08x\n", *pIntdata++);
	}
	
	/*MBV*/
	pIntdata = (uint32 *)pMBUV[1];
	for (i = 0; i < 8; i++)
	{
		FPRINTF (pfOutMB, "%08x\n", *pIntdata++);
		FPRINTF (pfOutMB, "%08x\n", *pIntdata++);
	}
}
#endif

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
