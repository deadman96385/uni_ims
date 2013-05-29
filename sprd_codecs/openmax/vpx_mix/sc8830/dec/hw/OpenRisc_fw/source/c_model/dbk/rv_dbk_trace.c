#include <stdio.h>
#include "rdbk_global.h"
#include "rv_dbk_trace.h"
//#include "rvdec_global.h"
#include <stdlib.h>
#include "common_global.h"

FILE	*fp_rdbk_bf_frame;
FILE	*fp_rdbk_cmd_file;
FILE	*fp_rdbk_af_frame;
FILE	*fp_rdbk_trace_file;
FILE	*fp_rdbk_mb_trace;

FILE *fp_rdbk_config_trace;
FILE *fp_rdbk_input_data_trace;
FILE *fp_rdbk_blk_para_trace;
FILE *fp_rdbk_filter_para_trace;
FILE *fp_rdbk_filter_data_trace;
FILE *fp_rdbk_output_data_trace;

void RDbkTraceInit()
{
	fp_rdbk_bf_frame = fopen("..\\..\\test_vectors\\dbk\\rdbk_bf.dat","w");
	fp_rdbk_af_frame = fopen("..\\..\\test_vectors\\dbk\\rdbk_af.dat","w");
	fp_rdbk_cmd_file = fopen("..\\..\\test_vectors\\dbk\\rdbk_cmd.dat","w");
	fp_rdbk_trace_file = fopen("..\\..\\test_vectors\\dbk\\rdbk_trace.txt","w");
	fp_rdbk_mb_trace= fopen("..\\..\\test_vectors\\dbk\\c_rdbk_mb_trace.txt","w");
// 	g_rdbk_reg_ptr = (VSP_RDBK_REG_T  *)malloc(sizeof(VSP_RDBK_REG_T));
// 	g_dbk_reg_ptr  = (VSP_DBK_REG_T	  *)malloc(sizeof(VSP_DBK_REG_T));

	fp_rdbk_config_trace = fopen(".//trace//config.txt","w");
	fp_rdbk_input_data_trace = fopen(".//trace//input_data.txt","w");
	fp_rdbk_blk_para_trace = fopen(".//trace//blk_para.txt","w");
	fp_rdbk_filter_para_trace = fopen(".//trace//filter_para.txt","w");
	fp_rdbk_filter_data_trace = fopen(".//trace//filter_data.txt","w");
	fp_rdbk_output_data_trace = fopen(".//trace//output_data.txt","w");
}

void PrintFrmBfDBK()
{
	
}

void PrintDBKCmd()
{
	
}

void PrintMBTrace(int width, uint8 * y_ptr, uint8 * u_ptr, uint8 *v_ptr,int mb_num_x,int mb_num_y, int g_nFrame)
{
	int j,i;
	int mb_j,mb_i;
	int mb_cnt;
	uint8 p0,p1,p2,p3;
	uint32 * frm_ptr;
	int pic_width;

	fprintf(fp_rdbk_mb_trace,"Frame num %d : \n",g_nFrame);

	pic_width =  width>>2;
	
	for (mb_j = 0 ; mb_j <mb_num_y; mb_j ++ )
	{
		for (mb_i = 0; mb_i <mb_num_x; mb_i++)
		{
			mb_cnt = mb_j * mb_num_x + mb_i;
			fprintf(fp_rdbk_mb_trace,"MB num %d : \n",mb_cnt);

			frm_ptr = ((uint32 *)y_ptr) +  MB_SIZE*mb_j*pic_width + 4 * mb_i;

			for(j=0;j<16;j++)
			{
				for (i=0;i<4;i++)
				{
					if (j == 0 && i == 0)
					{
						fprintf(fp_rdbk_mb_trace,"Y : \n");
					}

					p0 = ((frm_ptr[i]) >>24) & 0xff;
					p1 = ((frm_ptr[i]) >>16) & 0xff;
					p2 = ((frm_ptr[i]) >>8) & 0xff;
					p3 = ((frm_ptr[i]) >>0) & 0xff;
					fprintf(fp_rdbk_mb_trace, "%3x, %3x, %3x, %3x, |",p3,p2,p1,p0);
				}				
				fprintf(fp_rdbk_mb_trace,"\n");	
				if (j%4 == 3)
					fprintf(fp_rdbk_mb_trace,"+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
			
				frm_ptr += pic_width;
			}
#if 1
			frm_ptr = ((uint32 *)u_ptr) + (MB_SIZE/2)*mb_j*(pic_width/2) + 2 * mb_i;

			for (j=0;j<8;j++)
			{
				for (i=0;i<2;i++)
				{
					if(j == 0 && i ==0)
						fprintf(fp_rdbk_mb_trace,"U : \n");

					p0 = ((frm_ptr[i]) >>24) & 0xff;
					p1 = ((frm_ptr[i]) >>16) & 0xff;
					p2 = ((frm_ptr[i]) >>8) & 0xff;
					p3 = ((frm_ptr[i]) >>0) & 0xff;
					fprintf(fp_rdbk_mb_trace, "%3x, %3x, %3x, %3x, |",p3,p2,p1,p0);
				}
				fprintf(fp_rdbk_mb_trace,"\n");	
				
				if (j%4 == 3)
					fprintf(fp_rdbk_mb_trace,"+++++++++++++++++++++++++++++++++++++++++++\n");
			
				frm_ptr += (pic_width/2);
			}

			frm_ptr = ((uint32 *)v_ptr) + (MB_SIZE/2)*mb_j*(pic_width/2) + 2 * mb_i;

			for (j=0;j<8;j++)
			{
				for (i=0;i<2;i++)
				{
					if(j == 0 && i ==0)
						fprintf(fp_rdbk_mb_trace,"V : \n");

					p0 = ((frm_ptr[i]) >>24) & 0xff;
					p1 = ((frm_ptr[i]) >>16) & 0xff;
					p2 = ((frm_ptr[i]) >>8) & 0xff;
					p3 = ((frm_ptr[i]) >>0) & 0xff;
					fprintf(fp_rdbk_mb_trace, "%3x, %3x, %3x, %3x, |",p3,p2,p1,p0);
				}
				fprintf(fp_rdbk_mb_trace,"\n");	

				if (j%4 == 3)
					fprintf(fp_rdbk_mb_trace,"+++++++++++++++++++++++++++++++++++++++++++\n");
			
				frm_ptr += (pic_width/2);
			}
#endif
		}
	}

}


#if 0
void PrintMBBeforeDBK(RV_DECODER_T * pDecoder, uint8 * y_ptr,uint8 *u_ptr,uint8 *v_ptr)
{
	uint32 * pTest;
	int	mb_y;
	int mb_x;
	uint32 * pMBYUV[3];
	uint32 * pGobPix[3];
	int i;
	/************************************************************************/
	/* write  MB before dbk filter                                   */
	/************************************************************************/
	pGobPix[0] = (uint32 *)y_ptr;
	pGobPix[1] = (uint32 *)u_ptr;
	pGobPix[2] = (uint32 *)v_ptr;

	if(/*g_nFrame == 0*/1)
	{
		
		for (mb_y = 0; mb_y < pDecoder->uMbNumY; mb_y++)
		{
			pMBYUV [0] = pGobPix [0];
			pMBYUV [1] = pGobPix [1];
			pMBYUV [2] = pGobPix [2];
			for (mb_x = 0; mb_x < pDecoder->uMbNumX; mb_x++)
			{
				pTest = pMBYUV [0];
				for (i=0;i<16;i++)
				{
					fprintf(fp_rdbk_bf_frame,"%08x\n",pTest[0]);
					fprintf(fp_rdbk_bf_frame,"%08x\n",pTest[1]);
					fprintf(fp_rdbk_bf_frame,"%08x\n",pTest[2]);
					fprintf(fp_rdbk_bf_frame,"%08x\n",pTest[3]);
					pTest += (pDecoder->uFrameWidth>>2);
				}

				pTest = pMBYUV[1];
				for (i=0;i<8;i++)
				{
					fprintf(fp_rdbk_bf_frame,"%08x\n",pTest[0]);
					fprintf(fp_rdbk_bf_frame,"%08x\n",pTest[1]);
					pTest += (pDecoder->uFrameWidth >>3);
				}

				pTest = pMBYUV[2];
				for (i=0;i<8;i++)
				{
					fprintf(fp_rdbk_bf_frame,"%08x\n",pTest[0]);
					fprintf(fp_rdbk_bf_frame,"%08x\n",pTest[1]);
					pTest += (pDecoder->uFrameWidth >>3);
				}
				pMBYUV [0] += (MB_SIZE>>2);
				pMBYUV [1] += (BLOCK_SIZE>>2);
				pMBYUV [2] += (BLOCK_SIZE>>2);
			}
			pGobPix [0] += (pDecoder->uFrameWidth>>2) * (MB_SIZE);
			pGobPix [1] += ((pDecoder->uFrameWidth>>1)>>2) * (BLOCK_SIZE);
			pGobPix [2] += ((pDecoder->uFrameWidth>>1)>>2) * (BLOCK_SIZE);
		}

	}
}

void Rv8_DBK_FW_Command(RV_DECODER_T * pDecoder, MB_MODE_T * pmbmd)
{
	int		QP;
	int		strength;

	int32	CBP_topleft;
	int32	CBP_top;
	int32	CBP_left;
	int32	CBP_current;

	int		bOnleftedge;
	int		bOnrightedge;
	int		bOntopedge;

	int		mb_num_x;

	int32	Y_ver_expand;
	int32	Y_ver_tmp0;
	int32	Y_ver_tmp1;
	int32	Y_ver_bs;

	int32	U_ver_expand;
	int32	U_ver_tmp0;
	int32	U_ver_tmp1;
	int32	U_ver_bs;

	int32	V_ver_expand;
	int32	V_ver_tmp0;
	int32	V_ver_tmp1;
	int32	V_ver_bs;

	int32	Y_hor_tmp0;
	int32	Y_hor_tmp1;
	int32	Y_hor_bs;

	int32	U_hor_tmp0;
	int32	U_hor_tmp1;
	int32	U_hor_bs;

	int32	V_hor_tmp0;
	int32	V_hor_tmp1;
	int32	V_hor_bs;

	int		b_Intra_pic;
	BOOLEAN	bOnTopEdge;
	BOOLEAN	bOnLeftEdge;
	BOOLEAN	bOnBottomEdge;
	BOOLEAN	bOnRightEdge;	

	uint8 g_strength [32] = {0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,2,2,2,2,2,3,3,3,3,4,4,4,4,5,5,5};

	U_ver_bs	= 0;
	V_ver_bs	= 0;
	U_hor_bs	= 0;
	V_hor_bs	= 0;

	QP			= pmbmd->uQp;
	strength	= g_strength[QP];

	mb_num_x	= pDecoder->uMbNumX;

	bOnleftedge	= (pDecoder->uMbPosX == 0) ? 1 :0 ;
	bOnrightedge= (pDecoder->uMbPosX == mb_num_x - 1) ? 1 :0 ;
	bOntopedge	= (pDecoder->uMbPosY == 0) ? 1 :0 ;	

	CBP_topleft = (!bOnleftedge && ! bOntopedge) ? ((pmbmd- mb_num_x - 1)->uCbpcoef) : 0;
	CBP_top		= (!bOntopedge)? ((pmbmd - mb_num_x)->uCbpcoef) : 0;
	CBP_left	= (!bOnleftedge)? ((pmbmd - 1)->uCbpcoef) : 0;
	CBP_current = pmbmd->uCbpcoef;
#if RV8_PARA_FW		
	Y_ver_expand= (((CBP_current >> 12) & 0xf) << 21) | (((CBP_left >> 15) & 0x1) << 20)
			    | (((CBP_current >> 8 ) & 0xf) << 16) | (((CBP_left >> 11) & 0x1) << 15)
				| (((CBP_current >> 4 ) & 0xf) << 11) | (((CBP_left >> 7 ) & 0x1) << 10)
				| (((CBP_current >> 0 ) & 0xf) << 6 ) | (((CBP_left >> 3 ) & 0x1) << 5)
				| (((CBP_top	 >> 12) & 0xf) << 1 ) | (((CBP_topleft >> 15 ) & 0x1) << 0);
	Y_ver_tmp0	= Y_ver_expand >> 5;
	Y_ver_tmp1	= Y_ver_expand & 0xfffff;
	Y_ver_bs	= Y_ver_tmp0 | Y_ver_tmp1;


	Y_hor_tmp0	= Y_ver_tmp0;
	Y_hor_tmp1	= Y_ver_tmp0 << 1;
	Y_hor_bs	= (Y_hor_tmp0 | Y_hor_tmp1) & 0x1EF7BDE;
	

	if (pDecoder->uPicCodeType == INTRAPIC)
	{
		U_ver_expand= (((CBP_current >> 18) & 0x3) << 7) | (((CBP_left >> 19) & 0x1) << 6)
					| (((CBP_current >> 16) & 0x3) << 4) | (((CBP_left >> 17) & 0x1) << 3)
					| (((CBP_top	 >> 18) & 0x3) << 1) | (((CBP_topleft >> 19 ) & 0x1) << 0);
		
		V_ver_expand= (((CBP_current >> 22) & 0x3) << 7) | (((CBP_left >> 23) & 0x1) << 6)
					| (((CBP_current >> 20) & 0x3) << 4) | (((CBP_left >> 21) & 0x1) << 3)
					| (((CBP_top	 >> 22) & 0x3) << 1) | (((CBP_topleft >> 23 ) & 0x1) << 0);
	
		U_ver_tmp0	= U_ver_expand >> 3;
		U_ver_tmp1	= U_ver_expand & 0x3f;
		U_ver_bs	= U_ver_tmp0 | U_ver_tmp1;

		V_ver_tmp0	= V_ver_expand >> 3;
		V_ver_tmp1	= V_ver_expand & 0x3f;
		V_ver_bs	= V_ver_tmp0 | V_ver_tmp1;

		U_hor_tmp0	= U_ver_expand;
		U_hor_tmp1	= U_ver_expand << 1;
		U_hor_bs	= (U_hor_tmp0 | U_hor_tmp1) & 0x1b0;

		V_hor_tmp0	= V_ver_expand;
		V_hor_tmp1	= V_ver_expand << 1;
		V_hor_bs	= (V_hor_tmp0 | V_hor_tmp1) & 0x1b0;
	}	
	
	if (!bOnrightedge)
	{
		Y_ver_bs &= 0x7bdef;
		U_ver_bs &= 0x1b;
		V_ver_bs &= 0x1b;
	}
	if (bOntopedge)
	{
		Y_ver_bs &= 0xFFFE0;
		U_ver_bs &= 0x38;
		V_ver_bs &= 0x38;
	}
	if (bOnleftedge)
	{
		Y_hor_bs &= 0xE739C;
		U_hor_bs &= 0x120;
		V_hor_bs &= 0x120;
	}

		/* Y_ver_bs[19:0] = {L3 v15 v14 v13 v12,
						 L2 v11 v10 v9  v8,
						 L1 v7  v6  v5  v4,
						 L0 v3  v2  v1  v0};
	   Y_hor_bs[19:0] = {h15 h11 h7 h3  0,
						 h14 h10 h6 h2  0,
						 h13 h9  h5 h1  0,
						 h12 h8  h4 h0  0};
	*/



	g_dbk_reg_ptr->rdbk_cfg0	= strength;
	g_dbk_reg_ptr->rdbk_cfg1	= Y_ver_bs;
	g_dbk_reg_ptr->rdbk_cfg2	= U_ver_bs | (V_ver_bs << 8);
	g_dbk_reg_ptr->rdbk_cfg3	= Y_hor_bs;
	g_dbk_reg_ptr->rdbk_cfg4	= U_hor_bs | (V_hor_bs << 16);

# else

#if RDBK_OWN_REG
// 	uint32	rdbk_cfg0;						//[8]		bIntraPic
											//[2:0]		MB strength
// 
// 	uint32	rdbk_cfg1;						//[23:0]	cbp_topleft
// 
// 	uint32	rdbk_cfg2;						//[23:0]	cbp_top

// 
// 	uint32	rdbk_cfg3;						//[23:0]	cbp_left
// 
// 	uint32	rdbk_cfg4;						//[23:0]	cbp_current


//	g_rdbk_reg_ptr->rdbk_cfg0	= ((pDecoder->uPicCodeType == INTRAPIC)<< 8) | strength;
//	g_rdbk_reg_ptr->rdbk_cfg1	= CBP_topleft;
//	g_rdbk_reg_ptr->rdbk_cfg2	= CBP_top;
//	g_rdbk_reg_ptr->rdbk_cfg3	= CBP_left;
//	g_rdbk_reg_ptr->rdbk_cfg4	= CBP_current;
//
//		fprintf(fp_rdbk_cmd_file, "mb_x = %d, mb_y = %d\n",pDecoder->uMbPosX,pDecoder->uMbPosY);
//		fprintf(fp_rdbk_cmd_file, "1,0x20000000,%08x\n",g_rdbk_reg_ptr->rdbk_cfg0);
//		fprintf(fp_rdbk_cmd_file, "1,0x20000004,%08x\n",g_rdbk_reg_ptr->rdbk_cfg1);
//		fprintf(fp_rdbk_cmd_file, "1,0x20000008,%08x\n",g_rdbk_reg_ptr->rdbk_cfg2);
//		fprintf(fp_rdbk_cmd_file, "1,0x2000000c,%08x\n",g_rdbk_reg_ptr->rdbk_cfg3);
//		fprintf(fp_rdbk_cmd_file, "1,0x20000010,%08x\n",g_rdbk_reg_ptr->rdbk_cfg4);
//
//	g_rdbk_reg_ptr->rdbk_cfg7	= (pDecoder->uMbNumX <<16) | (pDecoder->uMbPosY <<8) | (pDecoder->uMbPosX);
#else


	b_Intra_pic	= (pDecoder->uPicCodeType == INTRAPIC);
	bOnRightEdge= (pDecoder->uMbPosX == (pDecoder->uMbNumX -1));
	bOnLeftEdge = (pDecoder->uMbPosX == 0);
	bOnTopEdge	= (pDecoder->uMbPosY == 0);
	bOnBottomEdge=(pDecoder->uMbPosY == (pDecoder->uMbNumY -1));

	g_dbk_reg_ptr->HDBK_MB_INFO = strength;
	g_dbk_reg_ptr->HDBK_BS_H0	= CBP_current;
	g_dbk_reg_ptr->HDBK_BS_H1	= CBP_left;
	g_dbk_reg_ptr->HDBK_BS_V0	= CBP_top;
	g_dbk_reg_ptr->HDBK_BS_V1	= CBP_topleft;
	g_dbk_reg_ptr->RDBK_PARS	=	(b_Intra_pic <<8)
									| (bOnRightEdge << 3)| (bOnLeftEdge <<2)
									| (bOnBottomEdge <<1)| (bOnTopEdge);
#endif

#endif
}


void Rv9_DBK_FW_Command(RV_DECODER_T * pDecoder, MB_MODE_T * pmbmd,uint32 *pmvd)
{
	uint8 * pV, *pH;
	uint8 *pVc[2], *pHc[2];
	int uPitch;
	uint32 uTQPa, uTQPl;

	/*for current MB*/
	uint32 cbp;		/* luma */
	uint32 cbpc[2];	/* chroma, U=0,V=1 */
	uint32 mbtype;
	uint32 mvd;
	uint32 s;				/* strength (index to clip_table) */
	uint32 mbclip;			/* clip_table[s][QP] */
	uint32 mbclip0;
	uint32 mbclip2;		/* for B frame ref diff case */
	uint32 bhfilter;		/* bit-packed boolean, filter H edge (1=yes) */
	uint32 bhfilterc[2];	/* bit-packed boolean, filter H edge (1=yes) */
	uint32 bvfilter;		/* bit-packed boolean, filter V edge (1=yes) */
	uint32 bvfilterc[2];	/* bit-packed boolean, filter V edge (1=yes) */
	uint32 hfilter;		/* bit-packed H edge filter (0=weak,1=strong) */
	uint32 hfilterchroma[2];	/* bit-packed H edge filter (0=weak,1=strong) */
	uint32 vfilter;		/* bit-packed V edge filter (0=weak,1=strong) */
	uint32 vfilterchroma[2];	/* bit-packed V edge filter (0=weak,1=strong) */
	
	/*for adjacent MB*/
	uint32 cbp_left;
	uint32 cbpc_left[2];
	uint32 cbp_above;
	uint32 cbpc_above[2];
	uint32 cbp_below;
	uint32 cbpc_below[2];
	uint32 mvd_below;
	uint32 mvd_above;
	uint32 mvd_left;
	uint32 mbtype_left;
	uint32 mbtype_above;
	uint32 mbtype_below;
	uint32 s_left;
	uint32 s_above;
	uint32 s_below;
	uint32 mbclip_left;
	uint32 mbclip_above;
	uint32 mbclip_below;
	uint32 bnzs_left;
	uint32 bnzs_above;

	uint32 bnzs;			/* bit-packed block boolean, use MB strength (1=yes, 0=use 0 strength) */
	BOOLEAN bOnTopEdge;
	BOOLEAN bOnLeftEdge;
	BOOLEAN bOnBottomEdge;
	BOOLEAN bOnRightEdge;
	BOOLEAN bAboveRefDiff = FALSE;
	BOOLEAN bLeftRefDiff = FALSE;
	BOOLEAN bBelowRefDiff = FALSE;

	/* for adjacent blocks, formed by combining this MB and adjacent MB bits */
	uint32 cbph_adj;
	uint32 cbpv_adj;

	uint8 beta2;	
	uint8 beta2chroma;

	uint32 uQP;

	uint32 uBlock;
	uint32 uBlockRow;
	/* edge bit masks for current block */
	uint32 uEdgeBit;		/* for left and upper edges */
	uint32 uLowEdgeBit;	/* for lower edge */
	uint32 uBlockClip;		/* clip level current block */
	uint32 uBlockClipBelow;
	uint32 uBlockClipAbove;
	uint32 uBlockClipLeft;
	uint32 uBlockClipHVal;	/* H clip level current block */
	uint32 uBlockClipVVal;	/* V clip level current block */
	uint32 uCIx;
	uint32 i;
	uint32 *pTest;
	uint8  *pTest_af;

	int32  beTurePic;
	int32  beSmallPic;
	int32  uTopLeftCBP;
	int32  mbtype_top_left;
	int32  mvd_top_left;
	int32  uQP_above;
	int32  uQP_left;
	
	mbtype = pmbmd->uMbType;
	cbp = pmbmd->uCbpcoef;
	mvd = (pDecoder->uPicCodeType == INTRAPIC) ? 0 : *pmvd;
	
	/*get mb postion information, and get it's neighbor MB's mb type and other information*/	
	if (pDecoder->uMbPosX > 0)//left boundary
	{
		bOnLeftEdge = FALSE;
		mbtype_left = (pmbmd-1)->uMbType;
		cbp_left = (pmbmd-1)->uCbpcoef;
		mvd_left = *(pmvd-1);
		uQP_left = (pmbmd-1)->uQp;
	}
	else
	{
		bOnLeftEdge = TRUE;
		mbtype_left = mbtype;
		cbp_left = 0;
		mvd_left = 0;
		uQP_left = pmbmd->uQp;
	}

	if (pDecoder->uMbPosY > 0) //top boundary
	{
		bOnTopEdge = FALSE;
		mbtype_above = (pmbmd-pDecoder->uMbNumX)->uMbType;
		cbp_above = (pmbmd-pDecoder->uMbNumX)->uCbpcoef;
		mvd_above = *(pmvd-pDecoder->uMbNumX);
		uQP_above = (pmbmd-pDecoder->uMbNumX)->uQp;
	}
	else
	{
		bOnTopEdge = TRUE;
		mbtype_above = mbtype;
		cbp_above = 0;
		mvd_above = 0;
		uQP_above = pmbmd->uQp;
	}

	if (pDecoder->uMbPosY < pDecoder->uMbNumY-1)	//bottom boundary
	{
		bOnBottomEdge = FALSE;
// 		mbtype_below = (pmbmd + pDecoder->uMbNumX)->uMbType;
// 		cbp_below = (pmbmd + pDecoder->uMbNumX)->uCbpcoef;
// 		mvd_below = *(pmvd + pDecoder->uMbNumX);
	}
	else
	{
		bOnBottomEdge = TRUE;
// 		mbtype_below = mbtype;
// 		cbp_below = 0;
// 		mvd_below = 0;
	}

	if (pDecoder->uMbPosX > 0 && pDecoder->uMbPosY >0)
	{
		uTopLeftCBP = ((pmbmd-pDecoder->uMbNumX-1)->uCbpcoef) ;
		mbtype_top_left = (pmbmd-pDecoder->uMbNumX-1)->uMbType;
		mvd_top_left = ((*(pmvd -pDecoder->uMbNumX-1)) ) ;
	} 
	else
	{
		uTopLeftCBP = 0;
		mbtype_top_left = mbtype;
		mvd_top_left = 0;
	}

	bOnRightEdge = ((pDecoder->uMbNumX-1) ==pDecoder->uMbPosX) ? 1: 0;

	beTurePic = (pDecoder->uPicCodeType == TRUEBPIC) ? 1: 0;
	beSmallPic= ((pDecoder->uFrameWidth*pDecoder->uFrameHeight) <= (176*144))? 1: 0;
	uQP = pmbmd->uQp;
	/************************************************************************/
	/* write rv_dbk_reg				                                        */
	/************************************************************************/
#if RDBK_OWN_REG
	if(/*g_nFrame == 0*/1)
	{
		g_dbk_reg_ptr->rdbk_cfg0 = (mbtype <<24) | cbp;
		g_dbk_reg_ptr->rdbk_cfg1 = (mbtype_left <<24)| cbp_left;
		g_dbk_reg_ptr->rdbk_cfg2 = (mbtype_above <<24)| cbp_above;
		g_dbk_reg_ptr->rdbk_cfg3 = (mbtype_top_left <<24) | uTopLeftCBP;
		g_dbk_reg_ptr->rdbk_cfg4 = (mvd <<16) | mvd_left;
		g_dbk_reg_ptr->rdbk_cfg5 = (mvd_above <<16) | mvd_top_left;
		g_dbk_reg_ptr->rdbk_cfg6 = (beSmallPic <<26)
									| (beTurePic <<25)
									| (bOnRightEdge <<24)
									| (bOnLeftEdge <<23)
									| (bOnBottomEdge <<22)
									| (bOnTopEdge <<21)
									| (uQP_left << 16)
									| (uQP_above <<8) |(uQP);

// 		g_dbk_reg_ptr->rdbk_cfg7 = (pDecoder->uMbPosY <<8)| (pDecoder->uMbPosX );

//		fprintf(fp_rdbk_cmd_file, "1,0x20000000,%08x\n",g_rdbk_reg_ptr->rdbk_cfg0);
//		fprintf(fp_rdbk_cmd_file, "1,0x20000004,%08x\n",g_rdbk_reg_ptr->rdbk_cfg1);
//		fprintf(fp_rdbk_cmd_file, "1,0x20000008,%08x\n",g_rdbk_reg_ptr->rdbk_cfg2);
//		fprintf(fp_rdbk_cmd_file, "1,0x2000000c,%08x\n",g_rdbk_reg_ptr->rdbk_cfg3);
//		fprintf(fp_rdbk_cmd_file, "1,0x20000010,%08x\n",g_rdbk_reg_ptr->rdbk_cfg4);
//		fprintf(fp_rdbk_cmd_file, "1,0x20000014,%08x\n",g_rdbk_reg_ptr->rdbk_cfg5);	
//		fprintf(fp_rdbk_cmd_file, "1,0x20000018,%08x\n",g_rdbk_reg_ptr->rdbk_cfg6);
//		fprintf(fp_rdbk_cmd_file, "1,0x2000001c,%08x\n",g_rdbk_reg_ptr->rdbk_cfg7);
		

	}
#else 
/************************************************************************/
/* 	volatile uint32	HDBK_MB_INFO;		//[29:24]:	mb_x
										// Real 9 & H264
										//[21:16]:	qp_top;
										//[13:8] :	qp_left;
										//[5:0]  :	qp_cur;
										
										// Real 8:
										//[2:0]	 :   MB_strength
										

	volatile uint32	HDBK_BS_H0;			// H264:
										//[31:28]:	bs_h7
										//....
										//[15:12]:	bs_h3
										//[11:8]:	bs_h2
										//[7:4]:	bs_h1
										//[3:0]:	bs_h0;

										// Real 8 9:
										//[23:0]	cbp
								
	volatile uint32	HDBK_BS_H1;			// H264:
										//[31:28]:	bs_h15
										//....
										//[15:12]:	bs_h11
										//[11:8]:	bs_h10
										//[7:4]:	bs_h9
										//[3:0]:	bs_h8;

										// Real 8 9:
										//[23:0]	cbp_left

	volatile uint32	HDBK_BS_V0;			// H264:
										//[31:28]:	bs_v7
										//....
										//[15:12]:	bs_v3
										//[11:8]:	bs_v2
										//[7:4]:	bs_v1
										//[3:0]:	bs_v0;

										// Real 8 9:
										////[23:0]	cbp_above

	volatile uint32	HDBK_BS_V1;			// H264
										//[31:28]:	bs_v15
										//....
										//[15:12]:	bs_v11
										//[11:8]:	bs_v10
										//[7:4]:	bs_v9
										//[3:0]:	bs_v8;

										// Real 8 9: 
										//[23:0]	cbp_topleft
	
	volatile uint32	HDBK_PARS;			// Only for H264
										//[20:16]:	chroma_qp_index_offset [-12, 12]
										//12~8:		alpha_offset	[-12, 12]
										//4~0:		beta_offset	[-12, 12]

	volatile uint32	RDBK_MVD0;			// Only for Real 9:
										//[31:16]	mvd
										//[15:0]	mvd_left

	volatile uint32	RDBK_MVD1;			// Only for Real 9:
										//[31:16]	mvd_above
										//[15:0]	mvd_topleft

	volatile uint32	RDBK_PARS;			// Real 9:
										//[31:28]	mb_type_topleft
										//[27:24]	mb_type_above
										//[23:20]	mb_type_left
										//[19:16]	mb_type
										//[10]      beSmallPic
										//[9]		beTureBPic
										// Real 8:
										//[8]		bIntraPic
										// Real 8 & 9:
										//[3]		bOnRightEdge
										//[2]		bOnLeftEdge
										//[1]		bOnBottomEdge
										//[0]		bOnTopEdge

	volatile uint32 HDBK_CFG_FINISH;	//0: write 1 to indicate configure finished                                                                     */
/************************************************************************/
	if(/*g_nFrame == 0*/1)
	{
		g_dbk_reg_ptr->HDBK_MB_INFO	= (pDecoder->uMbPosX <<24) | (uQP_above <<16) | (uQP_left <<8) | (uQP);
		g_dbk_reg_ptr->HDBK_BS_H0	= cbp;
		g_dbk_reg_ptr->HDBK_BS_H1	= cbp_left;
		g_dbk_reg_ptr->HDBK_BS_V0	= cbp_above;
		g_dbk_reg_ptr->HDBK_BS_V1	= uTopLeftCBP;
		g_dbk_reg_ptr->RDBK_MVD0	= (mvd <<16) | (mvd_left);
		g_dbk_reg_ptr->RDBK_MVD1	= (mvd_above <<16) | (mvd_top_left);
		g_dbk_reg_ptr->RDBK_PARS	= (mbtype_top_left <<28) | (mbtype_above <<24) 
									| (mbtype_left <<20) | (mbtype <<16) 
									| (beSmallPic << 10) | (beTurePic <<9)
									| (bOnRightEdge << 3)| (bOnLeftEdge <<2)
									| (bOnBottomEdge <<1)| (bOnTopEdge);



// 		fprintf(fp_rdbk_cmd_file, "1,0x20000000,%08x\n",g_rdbk_reg_ptr->rdbk_cfg0);
// 		fprintf(fp_rdbk_cmd_file, "1,0x20000004,%08x\n",g_rdbk_reg_ptr->rdbk_cfg1);
// 		fprintf(fp_rdbk_cmd_file, "1,0x20000008,%08x\n",g_rdbk_reg_ptr->rdbk_cfg2);
// 		fprintf(fp_rdbk_cmd_file, "1,0x2000000c,%08x\n",g_rdbk_reg_ptr->rdbk_cfg3);
// 		fprintf(fp_rdbk_cmd_file, "1,0x20000010,%08x\n",g_rdbk_reg_ptr->rdbk_cfg4);
// 		fprintf(fp_rdbk_cmd_file, "1,0x20000014,%08x\n",g_rdbk_reg_ptr->rdbk_cfg5);	
// 		fprintf(fp_rdbk_cmd_file, "1,0x20000018,%08x\n",g_rdbk_reg_ptr->rdbk_cfg6);
// 		fprintf(fp_rdbk_cmd_file, "1,0x2000001c,%08x\n",g_rdbk_reg_ptr->rdbk_cfg7);
		

	}
#endif
}
#endif

void DBK_malloc_buf()
{
	vsp_rdbk_lbuf = (uint32 *)malloc(sizeof(uint32)*RDBK_LBUF_SIZE);
	vsp_rdbk_obuf = (uint32 *)malloc(sizeof(uint32)*RDBK_OBUF_SIZE);
	vsp_rdbk_tbuf = (uint32 *)malloc(sizeof(uint32)*RDBK_TBUF_SIZE);

	vsp_ripred_lbuf=(uint32 *)malloc(sizeof(uint32)*RDBK_LBUF_SIZE/4); // Only 1 line. 1/4 size of dbk line buffer.
}
void Print_input_data(int mb_num)
{
	int	addr;
	int	i;
	int j;
	uint32 val;

	RDBK_TRACE(fp_rdbk_input_data_trace,"MB_num = %d\n",mb_num);
	
	
	/************************************************************************/
	/* Write CURRENT MB from   RDBK_OBUF   to  file                          */
	/************************************************************************/
	// Read Y 
	addr = 26;//6*4+2
	for (i=0;i<16;i++)
	{
		for (j=0;j<4;j++)
		{
		 	val = vsp_rdbk_obuf[addr];
			addr ++;
			RDBK_TRACE(fp_rdbk_input_data_trace,"%08x\n",val);
		}
		addr += 2;
	}

	// Read U
	addr = 138;//120+4*4+2
	for (i=0;i<8;i++)
	{
		for (j=0;j<2;j++)
		{
		 	val = vsp_rdbk_obuf[addr];
			addr ++;
			RDBK_TRACE(fp_rdbk_input_data_trace,"%08x\n",val);
		}
		addr += 2;
	}	

	// Read V
	addr = 186;//168+4*4+2
	for (i=0;i<8;i++)
	{
		for (j=0;j<2;j++)
		{
		 	val = vsp_rdbk_obuf[addr];
			addr ++;
			RDBK_TRACE(fp_rdbk_input_data_trace,"%08x\n",val);
		}
		addr += 2;
	}	

}

void Print_config(int mb_num)
{
	RDBK_TRACE(fp_rdbk_config_trace,"MB_num = %d\n",mb_num);
	//add MB_num info in all trace file 
	RDBK_TRACE(fp_rdbk_blk_para_trace,"MB_num = %d\n",mb_num);
	RDBK_TRACE(fp_rdbk_filter_data_trace,"MB_num = %d\n",mb_num);
	RDBK_TRACE(fp_rdbk_filter_para_trace,"MB_num = %d\n",mb_num);

	RDBK_TRACE(fp_rdbk_config_trace,"rdbk_cfg0 = %08x\n",g_dbk_reg_ptr->rdbk_cfg0);
	RDBK_TRACE(fp_rdbk_config_trace,"rdbk_cfg1 = %08x\n",g_dbk_reg_ptr->rdbk_cfg1);
	RDBK_TRACE(fp_rdbk_config_trace,"rdbk_cfg2 = %08x\n",g_dbk_reg_ptr->rdbk_cfg2);
	RDBK_TRACE(fp_rdbk_config_trace,"rdbk_cfg3 = %08x\n",g_dbk_reg_ptr->rdbk_cfg3);
	RDBK_TRACE(fp_rdbk_config_trace,"rdbk_cfg4 = %08x\n",g_dbk_reg_ptr->rdbk_cfg4);
	RDBK_TRACE(fp_rdbk_config_trace,"rdbk_cfg5 = %08x\n",g_dbk_reg_ptr->rdbk_cfg5);
	RDBK_TRACE(fp_rdbk_config_trace,"rdbk_cfg6 = %08x\n",g_dbk_reg_ptr->rdbk_cfg6);
	RDBK_TRACE(fp_rdbk_config_trace,"rdbk_cfg7 = %08x\n",g_glb_reg_ptr->VSP_CTRL0);
}

void Print_output_data(int mb_num,int last_row,int last_col)
{
	int	addr;
	int	i;
	int j;
	uint32 val;
	int	b_valid;
	int	mb_x	= (g_glb_reg_ptr->VSP_CTRL0) & 0xff;
	int mb_y = (g_glb_reg_ptr->VSP_CTRL0 >> 8) & 0xff;

	RDBK_TRACE(fp_rdbk_output_data_trace,"MB_num = %d\n",mb_num);
	
	//Y
	for (i=0;i<20;i++)
	{
		for (j=0;j<6;j++)
		{
			if ((mb_x == 0 && j <2) || (mb_y == 0 && i < 4))
			{ // invalid data.
				val = 0;
				b_valid = 0;
			} 
			else if (i<4)
			{// Write 0st line from Tbuf to file
				addr = 0 + i*6 + j;
				val = vsp_rdbk_tbuf[addr];
			}
			else if (i<16)
			{// Write 1,2,3 line from Obuf to file
				addr = 0 + i*6 + j;
				val = vsp_rdbk_obuf[addr];
			} 
			else
			{
				if ((!last_row && j<5) || (!last_row &&last_col && j == 5))
				{// write from lbuf to file
					addr = (i-16) * Y_LINE_BUF_WIDTH + mb_x*4 - 2 + j;
					val = vsp_rdbk_lbuf[addr];
				} 
				else
				{// write from Obuf to file
					addr = 0 + i*6 + j;
					val = vsp_rdbk_obuf[addr];
				}
			}

			RDBK_TRACE(fp_rdbk_output_data_trace,"%08x\n",val);
			
		}
	}
	// U
	for (i=0;i<12;i++)
	{
		for (j=0;j<4;j++)
		{
			if ((mb_x == 0 && j <2) || (mb_y == 0 && i < 4))
			{ // invalid data.
				val = 0;
				b_valid = 0;
			} 
			else if (i<4)
			{// Write 0st line from Tbuf to file
				addr = U_OFFSET_TBUF_C + i*4 + j;
				val = vsp_rdbk_tbuf[addr];
			}
			else if (i<8)
			{// Write 1 line from Obuf to file
				addr = U_OFFSET_OBUF_C + i*4 + j;
				val = vsp_rdbk_obuf[addr];
			} 
			else
			{
				if ((!last_row && j<3)|| (!last_row && last_col && j == 3))
				{// write from lbuf to file
					addr = U_OFFSET_LINE_BUF + (i-8) * UV_LINE_BUF_WIDTH + mb_x*2 - 2 + j;
					val = vsp_rdbk_lbuf[addr];
				} 
				else
				{// write from Obuf to file
					addr = U_OFFSET_OBUF_C + i*4 + j;
					val = vsp_rdbk_obuf[addr];
				}
			}

			RDBK_TRACE(fp_rdbk_output_data_trace,"%08x\n",val);
			
		}
	}
	// V
	for (i=0;i<12;i++)
	{
		for (j=0;j<4;j++)
		{
			if ((mb_x == 0 && j <2) || (mb_y == 0 && i < 4))
			{ // invalid data.
				val = 0;
				b_valid = 0;
			} 
			else if (i<4)
			{// Write 0st line from Tbuf to file
				addr = V_OFFSET_TBUF_C + i*4 + j;
				val = vsp_rdbk_tbuf[addr];
			}
			else if (i<8)
			{// Write 1 line from Obuf to file
				addr = V_OFFSET_OBUF_C + i*4 + j;
				val = vsp_rdbk_obuf[addr];
			} 
			else
			{
				if ((!last_row && j<3) || (!last_row &&last_col && j == 3))
				{// write from lbuf to file
					addr = V_OFFSET_LINE_BUF + (i-8) * UV_LINE_BUF_WIDTH + mb_x*2 - 2 + j;
					val = vsp_rdbk_lbuf[addr];
				} 
				else
				{// write from Obuf to file
					addr = V_OFFSET_OBUF_C + i*4 + j;
					val = vsp_rdbk_obuf[addr];
				}
			}

			RDBK_TRACE(fp_rdbk_output_data_trace,"%08x\n",val);
			
		}
	}

}

