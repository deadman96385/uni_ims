#include "rdbk_global.h"
#include "sc8810_video_header.h"
//#include "rvdec_global.h"
#include "common_global.h"


/*for deblocking*/
const char clip_table[3][32] =
{
	/*   0         5        10        15        20        25        30 */
	{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
	{0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,2,2,2,2,3,3,3,3,4,5,5},
	{0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,2,2,2,2,3,3,3,4,4,5,5,5,7,8,9},
};

// const char clip_table[3][32] =
// {
// 	/*   0         5        10        15        20        25        30 */
// 	{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
// 	{0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,2,2,2,2,3,3,3,3,4,5,5},
// 	{0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,2,2,2,2,3,3,3,4,4,5,5,5,7,8,9},
// };
 const int32 alpha_tab[32] = 
 {
 	128,128,128,128,128,128,128,128,128,128,
 		122, 96, 75, 59, 47, 37, 29, 23, 18, 15,
 		13, 11,  10, 9,  8,  7,  6,  5,  4, 3,
 		2,  1,
 };
 
 const uint8 beta_tab[32] =
 {
 	0, 0, 0, 0, 0, 0, 0, 0, 3, 3,
 		3, 4, 4, 4, 6, 6, 6, 7, 8, 8,
 		9, 9,10,10,11,11,12,13,14,15,
 		16,17,
 };

#ifndef REAL_DEC
typedef enum {
    MBTYPE_INTRA,			/* 0 */
    MBTYPE_INTRA_16x16,		/* 1 */
    MBTYPE_INTER,			/* 2 */
    MBTYPE_INTER_4V,		/* 3 */
    MBTYPE_FORWARD,			/* 4 */
    MBTYPE_BACKWARD,		/* 5 */
    MBTYPE_SKIPPED,			/* 6 */
    MBTYPE_DIRECT,			/* 7 */
    MBTYPE_INTER_16x8V,		/* 8 */
    MBTYPE_INTER_8x16V,		/* 9  */
    MBTYPE_BIDIR,			/* 10 */
	MBTYPE_INTER_16x16,		/* 11 */
    NUMBER_OF_MBTYPES		/* 12 */
} MB_Type_E;
#endif

uint8 getMBStrength(uint32 mbType) 
{
	if(mbType == MBTYPE_INTRA || mbType == MBTYPE_INTRA_16x16 || mbType == MBTYPE_INTER_16x16) 
	{
		return 2;
	}
	else 
	{		
		return 1;
	}
}

int deblockRefDiff (uint32 mbType, uint32 adjMbType) 
{	
	switch(mbType) 
	{
	case MBTYPE_FORWARD: /* 5 */
		if(adjMbType != MBTYPE_FORWARD)
			return TRUE;
		break;
		
	case MBTYPE_BACKWARD: /* 6 */
		if(adjMbType != MBTYPE_BACKWARD)
			return TRUE;
		break;
		
	case MBTYPE_SKIPPED:		/* 7				 */
	case MBTYPE_DIRECT:		    /* 8		 */
	case MBTYPE_BIDIR:
		if(adjMbType == MBTYPE_FORWARD || adjMbType == MBTYPE_BACKWARD) 
			return TRUE;
		break;
		
	default:
		break;
	}
	
	return FALSE;
}

void CalBlkPara_RDBK_rv9(int blk_x, int blk_y, int blk_comp, int second_phase,//input blk position 
					int proc_edge[2], int edge_clip_left[2]/*or above*/, int edge_clip_right[2]/*or below*/,
				int *alpha, int *beta, int *beta2)
				//output 
				// a. boolean, whether or not to filter the edge 
				// b. boolean, weak or strong filter (if edge is filtered)
				// c. filter clip level on each side of edge 

				/*blk order in this function is 
				 0  1  2  3  4	5			  TL0  TL1  T0  T1  T2  T3
				 6  7  8  9  10 11			  L0   L1   0   1   2   3
				 12 13 14 15 16 17 represent  L2   L3   4   5   6   7
				 18 19 20 21 22 23			  L4   L5   8   9   10  11
				 24 25 26 27 28 29            L6   L7   12  13  14  15 */
{
	uint8 bOnLeftEdge;
	uint8 bOnTopEdge;
	uint8 bOnBottomEdge;
	uint8 bOnRightEdge;
	uint8 beTureBPic;
	uint8 beSmallPic;

	uint8	edge1_need_filt;
	uint8	edge2_need_filt;
	uint8	edge3_need_filt;
	uint8	edge4_need_filt;

	uint8 mbtype;
	uint8 mbtype_left;
	uint8 mbtype_above;
	uint8 mbtype_topleft;

	uint32 Full_CBP;
	uint32 Full_CBP_left;
	uint32 Full_CBP_above;
	uint32 Full_CBP_topleft;

	uint32 cbp;
	uint32 cbp_left;
	uint32 cbp_above;
	uint32 cbp_topleft;
	
	uint16 mvd;
	uint16 mvd_left;
	uint16 mvd_above;
	uint16 mvd_topleft;

	uint8  uQP;
	uint8  uQP_current;
	uint8  uQP_above;
	uint8  uQP_left;

	uint32 cbpc[2];
	uint32 cbpc_left[2];
	uint32 cbpc_above[2];
	uint32 cbpc_topleft[2];

	uint32 expandMbCbp;
	uint32 expandMbMvd;
	uint32 expandMbbnzs;

	uint32 expandMbCbp_c[2];


	uint8  blk_cbp_current;
	uint8  blk_cbp_above;
	uint8  blk_cbp_left;
	uint8  blk_cbp_below;

	uint8  blk_mvd_current;
	uint8  blk_mvd_below;

	uint8  blk_bnzs_current;
	uint8  blk_bnzs_above;
	uint8  blk_bnzs_left;
	uint8  blk_bnzs_below;

	uint8  bFilterBlks;

	uint8  blk_id;


	uint8	edge1_clip_strenth_right;
	uint8	edge1_clip_strenth_left;
	uint8	edge3_clip_strenth_right;
	uint8	edge3_clip_strenth_left;
	uint8	edge24_clip_strenth_right;
	uint8	edge24_clip_strenth_left;

	uint8	edge1_cbp_right;
	uint8	edge1_cbp_left;
	uint8	edge1_mvd_right;
	uint8	edge1_bnzs_right;
	uint8	edge1_bnzs_left;	

	uint8	edge3_cbp_right;
	uint8	edge3_cbp_left;
	uint8	edge3_mvd_right;
	uint8	edge3_bnzs_right;
	uint8	edge3_bnzs_left;	

	uint8	edge24_cbp_right;
	uint8	edge24_cbp_left;
	uint8	edge24_mvd_right;
	uint8	edge24_bnzs_right;
	uint8	edge24_bnzs_left;

	uint8	b_topleft_above_diff;
	uint8	b_topleft_left_diff;
	uint8	b_left_current_diff;
	uint8	b_above_current_diff;



	uint8	b_edge1_filtered;
	uint8	b_edge3_filtered;
	uint8	b_edge24_filtered;

	uint8	start_x;
	uint8	end_x;
	uint8	start_y;
	uint8	end_y;

	uint8	b_edge1_frame_edge;
	uint8	b_edge3_frame_edge;
	uint8	b_edge24_frame_edge;

	uint8	b_edge1_MB_edge;
	uint8	b_edge3_MB_edge;
	uint8	b_edge24_MB_edge;

	uint8	b_edge1_ref_diff_inB;
	uint8	b_edge3_ref_diff_inB;
	uint8	b_edge24_ref_diff_inB;

	uint8	strenth_current;
	uint8	strenth_left;
	uint8	strenth_above;
	uint8	strenth_topleft;

	uint8	strenth_edge1_left;
	uint8	strenth_edge1_right;
	uint8	strenth_edge3_left;
	uint8	strenth_edge3_right;
	uint8	strenth_edge24_left;
	uint8	strenth_edge24_right;

	uint8	b_edge1_strength2_MB_edge;
	uint8	b_edge1_filter_strong;
	uint8	b_edge3_strength2_MB_edge;
	uint8	b_edge3_filter_strong;
	uint8	b_edge24_strength2_MB_edge;
	uint8	b_edge24_filter_strong;
	/*get para from register*/
	// Add QP_left;
#if RDBK_OWN_REG
	mbtype			= (g_dbk_reg_ptr->rdbk_cfg0 >>24) & 0xf;
	Full_CBP		= (g_dbk_reg_ptr->rdbk_cfg0) & 0xffffff;
	
	mbtype_left		= (g_dbk_reg_ptr->rdbk_cfg1 >>24) & 0xf;
	Full_CBP_left	= (g_dbk_reg_ptr->rdbk_cfg1) & 0xffffff;

	mbtype_above	= (g_dbk_reg_ptr->rdbk_cfg2 >>24) & 0xf;
	Full_CBP_above	= (g_dbk_reg_ptr->rdbk_cfg2) & 0xffffff;
	
	mbtype_topleft	= (g_dbk_reg_ptr->rdbk_cfg3 >>24) & 0xf;	
	Full_CBP_topleft= (g_dbk_reg_ptr->rdbk_cfg3) & 0xffffff;

	mvd			= (g_dbk_reg_ptr->rdbk_cfg4 >> 16) & 0xffff;
	mvd_left	= (g_dbk_reg_ptr->rdbk_cfg4) & 0xffff;

	mvd_above	= (g_dbk_reg_ptr->rdbk_cfg5 >> 16) & 0xffff;
	mvd_topleft	= (g_dbk_reg_ptr->rdbk_cfg5) & 0xffff;

	beSmallPic	= (g_dbk_reg_ptr->rdbk_cfg6 >>26) & 0x1;
	beTureBPic	= (g_dbk_reg_ptr->rdbk_cfg6 >>25) & 0x1;
	bOnRightEdge= (g_dbk_reg_ptr->rdbk_cfg6 >>24) & 0x1;
	bOnLeftEdge = (g_dbk_reg_ptr->rdbk_cfg6 >>23) & 0x1;
	bOnBottomEdge= (g_dbk_reg_ptr->rdbk_cfg6>>22) & 0x1;
	bOnTopEdge	= (g_dbk_reg_ptr->rdbk_cfg6 >>21) & 0x1;
	uQP_left	= (g_dbk_reg_ptr->rdbk_cfg6 >>16)& 0x1f;
	uQP_above	= (g_dbk_reg_ptr->rdbk_cfg6 >>8) & 0x1f;
	uQP_current	= (g_dbk_reg_ptr->rdbk_cfg6 >>0) & 0x1f;
#else
// 		g_dbk_reg_ptr->HDBK_MB_INFO	= (pDecoder->uMbPosX <<24) | (uQP_above <<16) | (uQP_left <<8) | (uQP);
// 		g_dbk_reg_ptr->HDBK_BS_H0	= cbp;
// 		g_dbk_reg_ptr->HDBK_BS_H1	= cbp_left;
// 		g_dbk_reg_ptr->HDBK_BS_V0	= cbp_above;
// 		g_dbk_reg_ptr->HDBK_BS_V1	= uTopLeftCBP;
// 		g_dbk_reg_ptr->RDBK_MVD0	= (mvd <<16) | (mvd_left);
// 		g_dbk_reg_ptr->RDBK_MVD1	= (mvd_above <<16) | (mvd_top_left);
// 		g_dbk_reg_ptr->RDBK_PARS	= (mbtype_top_left <<28) | (mbtype_above <<24) 
// 									| (mbtype_left <<20) | (mbtype <<16) 
// 									| (beSmallPic << 10) | (beTurePic <<9)
// 									| (bOnRightEdge << 3)| (bOnLeftEdge <<2)
// 									| (bOnBottomEdge <<1)| (bOnTopEdge);

	mbtype			= (g_dbk_reg_ptr->RDBK_PARS >>16) & 0xf;
	Full_CBP		= (g_dbk_reg_ptr->HDBK_BS_H0) & 0xffffff;
	
	mbtype_left		= (g_dbk_reg_ptr->RDBK_PARS >>20) & 0xf;
	Full_CBP_left	= (g_dbk_reg_ptr->HDBK_BS_H1) & 0xffffff;

	mbtype_above	= (g_dbk_reg_ptr->RDBK_PARS >>24) & 0xf;
	Full_CBP_above	= (g_dbk_reg_ptr->HDBK_BS_V0) & 0xffffff;
	
	mbtype_topleft	= (g_dbk_reg_ptr->RDBK_PARS >>28) & 0xf;	
	Full_CBP_topleft= (g_dbk_reg_ptr->HDBK_BS_V1) & 0xffffff;

	mvd			= (g_dbk_reg_ptr->RDBK_MVD0 >> 16) & 0xffff;
	mvd_left	= (g_dbk_reg_ptr->RDBK_MVD0) & 0xffff;

	mvd_above	= (g_dbk_reg_ptr->RDBK_MVD1 >> 16) & 0xffff;
	mvd_topleft	= (g_dbk_reg_ptr->RDBK_MVD1) & 0xffff;

	beSmallPic	= (g_dbk_reg_ptr->RDBK_PARS >>10) & 0x1;
	beTureBPic	= (g_dbk_reg_ptr->RDBK_PARS >>9) & 0x1;
	bOnRightEdge= (g_dbk_reg_ptr->RDBK_PARS >>3) & 0x1;
	bOnLeftEdge = (g_dbk_reg_ptr->RDBK_PARS >>2) & 0x1;
	bOnBottomEdge= (g_dbk_reg_ptr->RDBK_PARS>>1) & 0x1;
	bOnTopEdge	= (g_dbk_reg_ptr->RDBK_PARS) & 0x1;
	uQP_left	= (g_dbk_reg_ptr->HDBK_MB_INFO >>8)& 0x1f;
	uQP_above	= (g_dbk_reg_ptr->HDBK_MB_INFO >>16) & 0x1f;
	uQP_current	= (g_dbk_reg_ptr->HDBK_MB_INFO) & 0x1f;
#endif


	cbp			= Full_CBP & 0xffff;
	cbp_left	= Full_CBP_left & 0xffff;
	cbp_above	= Full_CBP_above & 0xffff;
	cbp_topleft = Full_CBP_topleft & 0xffff;

	cbpc[0]		= (Full_CBP >>16) & 0xf;
	cbpc[1]		= (Full_CBP >>20) & 0xf;
	cbpc_left[0]= (Full_CBP_left >>16) & 0xf;
	cbpc_left[1]= (Full_CBP_left >>20) & 0xf;
	cbpc_above[0]	= (Full_CBP_above >>16) & 0xf;
	cbpc_above[1]	= (Full_CBP_above >>20) & 0xf;
	cbpc_topleft[0] = (Full_CBP_topleft >>16) & 0xf;
	cbpc_topleft[1] = (Full_CBP_topleft >>20) & 0xf;


	/*new ExpandMB CBP[29:0] =
	15 14 13 12 L7 L6 11 10 9 8 L5 L4 7 6 5 4 L3 L2 3 2 1 0 L1 L0 T3 T2 T1 T0 TL1 TL0

	where L7 = 15L; L6 = 14L; L5 = 11L; L4 = 10L; L3 = 7L; L2 = 6L; L1 = 3L; L0 = 2L;
	T3 = 15T; T2 = 14T; T1 = 13T; T0 = 12T;
	TL1 = 15TL; TL0 = 14TL;*/
	expandMbCbp = (((cbp >>12) & 0xf) <<26) | (((cbp_left >>14) &0x3) <<24)
				 | (((cbp >>8) & 0xf) <<20) | (((cbp_left >>10) &0x3) <<18)
				 | (((cbp >>4) & 0xf) <<14) | (((cbp_left >> 6) &0x3) <<12)
				 | (((cbp >>0) & 0xf) <<8)  | (((cbp_left >> 2) &0x3) <<6)
				 | (((cbp_above >>12) & 0xf) <<2) | (((cbp_topleft >>14) &0x3) <<0);

	/*new ExpandMB CBPc[11:0] = 

	3 2 L3 L2 1 0 L1 L0 T1 T0 TL1 TL0
	*/
	expandMbCbp_c[0] = (((cbpc[0] >>2) & 0x3) <<10) | (((cbpc_left[0] >>2) & 0x3) <<8)
				 	 | (((cbpc[0] >>0) & 0x3) <<6) | (((cbpc_left[0] >>0) & 0x3) <<4)
					 | (((cbpc_above[0] >>2) & 0x3) <<2) | (((cbpc_topleft[0] >>2) & 0x3) <<0);

	expandMbCbp_c[1] = (((cbpc[1] >>2) & 0x3) <<10) | (((cbpc_left[1] >>2) & 0x3) <<8)
				 	 | (((cbpc[1] >>0) & 0x3) <<6) | (((cbpc_left[1] >>0) & 0x3) <<4)
					 | (((cbpc_above[1] >>2) & 0x3) <<2) | (((cbpc_topleft[1] >>2) & 0x3) <<0);

	/*new ExpandMB Mvd[24:0] =
	15 14 13 12 L7 L6 11 10 9 8 L5 L4 7 6 5 4 L3 L2 3 2 1 0 L1 L0 T3 T2 T1 T0 TL1 TL0
	*/
	expandMbMvd = (((mvd >>12) & 0xf) <<26) | (((mvd_left >>14) &0x3) <<24)
				 | (((mvd >>8) & 0xf) <<20) | (((mvd_left >>10) &0x3) <<18)
				 | (((mvd >>4) & 0xf) <<14) | (((mvd_left >> 6) &0x3) <<12)
				 | (((mvd >>0) & 0xf) <<8)  | (((mvd_left >> 2) &0x3) <<6)
				 | (((mvd_above >>12) & 0xf) <<2) | (((mvd_topleft >>14) &0x3) <<0);

	/*new Expandbnzs = expandMbCbp | expandMbMvd*/
	expandMbbnzs = expandMbCbp | expandMbMvd;


	/* select para to calculate para of filtering current edge of blk*/
	if (blk_comp == 0)
	{
		blk_id = blk_x + blk_y * 6;

		blk_cbp_current = ((expandMbCbp >> blk_id) & 0x1);
		blk_cbp_above	= (blk_y == 0)? 0: ((expandMbCbp >> (blk_id -6)) & 0x1);
		blk_cbp_left	= (blk_x == 0)? 0: ((expandMbCbp >> (blk_id -1)) & 0x1);
		blk_cbp_below	= (blk_y == 4)? 0: ((expandMbCbp >> (blk_id +6)) & 0x1);

		blk_mvd_current	= ((expandMbMvd >> blk_id) & 0x1);
		blk_mvd_below	= (blk_y == 4)? 0: ((expandMbMvd >> (blk_id +6)) & 0x1);

		blk_bnzs_current= ((expandMbbnzs >> blk_id) & 0x1);
		blk_bnzs_above	= (blk_y == 0)? 0: ((expandMbbnzs >> (blk_id -6)) & 0x1);
		blk_bnzs_left	= (blk_x == 0)? 0: ((expandMbbnzs >> (blk_id -1)) & 0x1);
		blk_bnzs_below	= (blk_y == 4)? 0: ((expandMbbnzs >> (blk_id +6)) & 0x1);

	}
	else // Chroma
	{
		blk_id = blk_x + blk_y * 4;

		if (blk_comp == 1)
		{
			blk_cbp_current = ((expandMbCbp_c[0] >> blk_id) & 0x1);
			blk_cbp_above	= (blk_y == 0)? 0: ((expandMbCbp_c[0] >> (blk_id -4)) & 0x1);
			blk_cbp_left	= (blk_x == 0)? 0: ((expandMbCbp_c[0] >> (blk_id -1)) & 0x1);
			blk_cbp_below	= (blk_y == 2)? 0: ((expandMbCbp_c[0] >> (blk_id +4)) & 0x1);
		} 
		else
		{
			blk_cbp_current = ((expandMbCbp_c[1] >> blk_id) & 0x1);
			blk_cbp_above	= (blk_y == 0)? 0: ((expandMbCbp_c[1] >> (blk_id -4)) & 0x1);
			blk_cbp_left	= (blk_x == 0)? 0: ((expandMbCbp_c[1] >> (blk_id -1)) & 0x1);
			blk_cbp_below	= (blk_y == 2)? 0: ((expandMbCbp_c[1] >> (blk_id +4)) & 0x1);
		}

		blk_mvd_current	= 0;
		blk_mvd_below	= 0;

		blk_bnzs_current= blk_cbp_current;
		blk_bnzs_above	= blk_cbp_above;
		blk_bnzs_left	= blk_cbp_left;
		blk_bnzs_below	= blk_cbp_below;


	}

	/************************************************************************/
	/* NEED TO BE MODEIFIED !!!!!!								            */
	/************************************************************************/




	// edge1 
	edge1_cbp_right		= blk_cbp_below;
	edge1_cbp_left		= blk_cbp_current;

	edge1_mvd_right		= blk_mvd_below;

	edge1_bnzs_right	= blk_bnzs_below;
	edge1_bnzs_left		= blk_bnzs_current;

	// edge3
	edge3_cbp_right		= blk_cbp_current;
	edge3_cbp_left		= blk_cbp_above;

	edge3_mvd_right		= blk_mvd_current;

	edge3_bnzs_right	= blk_bnzs_current;
	edge3_bnzs_left		= blk_bnzs_above;

	//edge 2 or 4

	edge24_cbp_right	= blk_cbp_current;
	edge24_cbp_left		= blk_cbp_left;

	edge24_mvd_right	= blk_mvd_current;

	edge24_bnzs_right	= blk_bnzs_current;
	edge24_bnzs_left	= blk_bnzs_left;






	/* For normal MB, 16 blks are filtered: 
	T0 T1 T2 T3 from above MB, L1 L3 L5 from Left MB and 0 1 2 4 5 6 8 9 10 from current MB.
	If MB is on top edge of frame, T0 T1 T2 T3 are not filtered.
	If MB is on left edge of frame, L1 L3 L5 are not filtered.
	If MB is on right edge of frame, 3 7 11 are filtered.
	If MB is on bottom edge of frame, L7 12 13 14 of current MB is filtered
	If MB is on left bottom of  frame, L7 is not filtered
	If MB is on right bottom of  frame, 15 is filtered.*/


	if (blk_comp == 0)
	{
		start_x = bOnLeftEdge ? 2 : 1;
		end_x	= bOnRightEdge? 5 : 4;
		start_y = 1;
		end_y	= bOnBottomEdge? 4 : 3;

		bFilterBlks = (blk_x >= start_x && blk_x <= end_x && blk_y >= start_y && blk_y <=end_y)
					|| (!bOnTopEdge && blk_x >=2 && blk_x<=5 && blk_y ==0);
	} 
	else
	{
		start_x = bOnLeftEdge ? 2 : 1;
		end_x	= bOnRightEdge? 3 : 2;
		start_y = 1;
		end_y	= bOnBottomEdge? 2 : 1;

		bFilterBlks = (blk_x >= start_x && blk_x <= end_x && blk_y >= start_y && blk_y <=end_y)
					|| (!bOnTopEdge && blk_x >=2 && blk_x<=3 && blk_y ==0);		
	}

	/* Edge boolean determination */
	/* -------------------------- */
	/* a. whether or not to filter the edge (bit=1 indicates filter edge) */
	/* */
	/*	NO, unless: */
	/*   i. cbp bit for block on either side set */
	/*   ii. mvd bit for block below or to right of edge set */
	/*   iii. when B frame, MB edge, reference frames differ for adjacent MB */
	/*  also,  picture edges are never filtered */


	b_topleft_above_diff	= deblockRefDiff(mbtype_above,mbtype_topleft);
	b_topleft_left_diff		= deblockRefDiff(mbtype_left,mbtype_topleft);
	b_left_current_diff		= deblockRefDiff(mbtype, mbtype_left);
	b_above_current_diff	= deblockRefDiff(mbtype, mbtype_above);


	// whether edge 1 is filtered
	b_edge1_frame_edge	= bOnBottomEdge && ((blk_comp == 0 && blk_y== 4) || (blk_comp != 0 && blk_y== 2));
	
	b_edge1_MB_edge		= blk_y == 0 ? 1: 0;

	b_edge1_ref_diff_inB= beTureBPic && b_edge1_MB_edge && b_above_current_diff;

	b_edge1_filtered	= bFilterBlks && !b_edge1_frame_edge &&  (edge1_cbp_right || edge1_cbp_left || edge1_mvd_right || b_edge1_ref_diff_inB);


	// whether edge3 is filtered
	b_edge3_frame_edge	= bOnTopEdge && (blk_y == 1);
	
	b_edge3_MB_edge		= blk_y == 1 ? 1: 0;

	b_edge3_ref_diff_inB= beTureBPic && b_edge3_MB_edge && ((blk_x == 1)? b_topleft_left_diff : b_above_current_diff);

	b_edge3_filtered	= bFilterBlks && !b_edge3_frame_edge &&  (edge3_cbp_right || edge3_cbp_left || edge3_mvd_right || b_edge3_ref_diff_inB);

	// whether edge 2 or 4 is filtered
	b_edge24_frame_edge	= bOnLeftEdge && (blk_x == 2);
	
	b_edge24_MB_edge	= blk_x == 2? 1: 0;

	b_edge24_ref_diff_inB= beTureBPic && b_edge24_MB_edge && ((blk_y == 0) ? b_topleft_above_diff : b_left_current_diff);

	b_edge24_filtered	= bFilterBlks && !b_edge24_frame_edge &&  (edge24_cbp_right || edge24_cbp_left || edge24_mvd_right || b_edge24_ref_diff_inB);



	/* b. strong or weak filter (bit=1 indicates strong) */
	/* */
	/*	WEAK, unless: */
	/*   i. MB edge and this or adjacent MB filter strength = 2 */
	/*   ii. B frame, MB edge, reference frames differ for adjacent MB */

	strenth_current		 = getMBStrength(mbtype);
	strenth_left		 = getMBStrength(mbtype_left);
	strenth_above		 = getMBStrength(mbtype_above);
	strenth_topleft		 = getMBStrength(mbtype_topleft);



	// edge 1  strenth

	strenth_edge1_left		= blk_y == 0 ? strenth_above : blk_x == 1 ? strenth_left : strenth_current;
	strenth_edge1_right		= blk_x == 1 ? strenth_left : strenth_current;

	b_edge1_strength2_MB_edge = ((strenth_edge1_left == 2) || (strenth_edge1_right == 2)) && b_edge1_MB_edge;
	
	b_edge1_filter_strong	= b_edge1_strength2_MB_edge || b_edge1_ref_diff_inB;

	// edge 3 strenth
	strenth_edge3_left		= blk_x == 1 ? strenth_topleft :strenth_above;
	strenth_edge3_right		= blk_x == 1 ? strenth_left :strenth_current;

	b_edge3_strength2_MB_edge = ((strenth_edge3_left == 2) || (strenth_edge3_right == 2)) && b_edge3_MB_edge;
	
	b_edge3_filter_strong	= b_edge3_strength2_MB_edge || b_edge3_ref_diff_inB;

	// edge 2 or 4 strenth

	strenth_edge24_left		= (blk_y == 0 && blk_x <3 ) ? strenth_topleft : 
								blk_y == 0 ? strenth_above : blk_x <3 ? strenth_left : strenth_current;

	strenth_edge24_right	= blk_y == 0 ? strenth_above : blk_x <2 ? strenth_left : strenth_current;

	b_edge24_strength2_MB_edge = ((strenth_edge24_left == 2) || (strenth_edge24_right == 2)) && b_edge24_MB_edge;
	
	b_edge24_filter_strong	= b_edge24_strength2_MB_edge || b_edge24_ref_diff_inB;




	/* Clip levels are determined by finding the clip levels for the current */
	/* and adjacent MBs (dependent on MB filter strength), then adjusting on a block basis */
	/* dependent upon block location, coded coeffs, and in B frames adjusted */
	/* when adjacent MBs use different reference frames. */



	edge1_clip_strenth_right = b_edge1_ref_diff_inB ? 2: edge1_bnzs_right ? strenth_edge1_right : 0;
	edge1_clip_strenth_left =  edge1_bnzs_left ? strenth_edge1_left : 0;

	edge3_clip_strenth_right = b_edge3_ref_diff_inB ? 2: edge3_bnzs_right ? strenth_edge3_right : 0;
	edge3_clip_strenth_left =  edge3_bnzs_left ? strenth_edge3_left : 0;

	edge24_clip_strenth_right = b_edge24_ref_diff_inB ? 2: edge24_bnzs_right ? strenth_edge24_right : 0;
	edge24_clip_strenth_left =  edge24_bnzs_left ? strenth_edge24_left : 0;

	/*if blk_y == 0, uses QP of above MB to calculate alpha, beta, beta2
	  if blk_x == 1 ,uses QP of left MB,
	  else uses current QP of current MB*/
	uQP					= (blk_y == 0)? uQP_above : (blk_x == 1)? uQP_left: uQP_current;



	if (!second_phase)
	{
		edge1_need_filt = b_edge1_filtered && !b_edge1_filter_strong;
		edge2_need_filt = (blk_y == 1) ?0: (b_edge24_filtered && !b_edge24_filter_strong);
		edge3_need_filt = 0;
		edge4_need_filt = (blk_y == 1) ?0: (b_edge24_filtered && b_edge24_filter_strong);
	}
	else
	{
		edge1_need_filt = 0;
		edge2_need_filt = b_edge24_filtered && !b_edge24_filter_strong;
		edge3_need_filt = b_edge3_filtered	&& b_edge3_filter_strong;
		edge4_need_filt = b_edge24_filtered && b_edge24_filter_strong;
	}


	/* Output parameters*/
	if (!second_phase)
	{
		proc_edge[0] = edge1_need_filt ? 1 : 0;
		proc_edge[1] = edge2_need_filt ? 2 : edge4_need_filt ? 4 : 0;
	}
	else
	{
		proc_edge[0] = (edge3_need_filt && edge4_need_filt) ? 3 : edge2_need_filt ? 2 : 0;
		proc_edge[1] = (edge3_need_filt && edge4_need_filt) ? 4 : edge3_need_filt ? 3 : edge4_need_filt? 4: 0;
	}

	edge_clip_right[0] = proc_edge[0] == 1 ? clip_table[edge1_clip_strenth_right][uQP] :
	proc_edge[0] == 3 ? clip_table[edge3_clip_strenth_right][uQP] : clip_table[edge24_clip_strenth_right][uQP];

	edge_clip_left[0] = proc_edge[0] == 1 ? clip_table[edge1_clip_strenth_left][uQP] :
	proc_edge[0] == 3 ? clip_table[edge3_clip_strenth_left][uQP] : clip_table[edge24_clip_strenth_left][uQP];
	
	edge_clip_right[1] = proc_edge[1] == 1 ? clip_table[edge1_clip_strenth_right][uQP] :
	proc_edge[1] == 3 ? clip_table[edge3_clip_strenth_right][uQP] : clip_table[edge24_clip_strenth_right][uQP];

	edge_clip_left[1] = proc_edge[1] == 1 ? clip_table[edge1_clip_strenth_left][uQP] :
	proc_edge[1] == 3 ? clip_table[edge3_clip_strenth_left][uQP] : clip_table[edge24_clip_strenth_left][uQP];
	

	* alpha				= alpha_tab[uQP];
	* beta				= beta_tab[uQP];
	* beta2				= (beSmallPic && blk_comp==0)? 4*beta_tab[uQP] : 3*beta_tab[uQP];

	
}


void CalFilterType_rv9(uint8 left_arr[12],uint8 right_arr[12],int bStrong,
				   int beta, int beta2,
				   int *Al,int *Ar,int *filter_type)
{
	/*left_arr[12] = L32 L31 L30 L22 L21 L20 L12 L11 L10 L02 L01 L00
	  right_arr[12]= R32 R31 R30 R22 R21 R20 R12 R11 R10 R02 R01 R00
	which organized as 
	L02 L01 L00 | R00 R01 R02
	L12 L11 L10 | R10 R11 R12
	L22 L21 L20 | R20 R21 R22
	L32 L31 L30 | R30 R31 R32*/

	int		deltaL[4], deltaR[4];
	int		deltaL2[4],deltaR2[4];
	int		sum_deltaL, sum_deltaR;	
	int		sum_deltaL2,sum_deltaR2;
	int		b3SmoothLeft, b3SmoothRight;
	int		Al_out,Ar_out;
	int		filter_type_out;

	// 01 - 00
	deltaL[0] = left_arr[1] - left_arr[0];
	deltaL[1] = left_arr[4] - left_arr[3];
	deltaL[2] = left_arr[7] - left_arr[6];
	deltaL[3] = left_arr[10] - left_arr[9];

	deltaR[0] = right_arr[1] - right_arr[0];
	deltaR[1] = right_arr[4] - right_arr[3];
	deltaR[2] = right_arr[7] - right_arr[6];
	deltaR[3] = right_arr[10] - right_arr[9];
	
	//01 - 02
	deltaL2[0] = left_arr[1] - left_arr[2];
	deltaL2[1] = left_arr[4] - left_arr[5];
	deltaL2[2] = left_arr[7] - left_arr[8];
	deltaL2[3] = left_arr[10] - left_arr[11];

	deltaR2[0] = right_arr[1] - right_arr[2];
	deltaR2[1] = right_arr[4] - right_arr[5];
	deltaR2[2] = right_arr[7] - right_arr[8];
	deltaR2[3] = right_arr[10] - right_arr[11];
	
	// sum of delta and delta2
	sum_deltaL = deltaL[0] + deltaL[1] + deltaL[2] + deltaL[3];
	
	sum_deltaR = deltaR[0] + deltaR[1] + deltaR[2] + deltaR[3];

	sum_deltaL2 = deltaL2[0] + deltaL2[1] + deltaL2[2] + deltaL2[3];
	
	sum_deltaR2 = deltaR2[0] + deltaR2[1] + deltaR2[2] + deltaR2[3];

	// Al and Ar
	Al_out = (abs(sum_deltaL) >= (beta <<2)) ? 1 : 3;
	Ar_out = (abs(sum_deltaR) >= (beta <<2)) ? 1 : 3; 

	b3SmoothLeft = bStrong && (Al_out == 3) && (abs(sum_deltaL2) < beta2);
	b3SmoothRight= bStrong && (Ar_out == 3) && (abs(sum_deltaR2) < beta2);

	filter_type_out = (b3SmoothLeft && b3SmoothRight) ? 3: //3:strong filter
					  (Al_out >1 && Ar_out >1) ? 2:		   //2:normal filter
					  (Al_out >1 || Ar_out >1) ? 1: 0;	   //1:weak filter, 0: none	
					
	// output
	*Al = Al_out;
	*Ar = Ar_out;
	*filter_type = filter_type_out;

}

//CalBlkPara_RDBK_SW Calculate edge order of each block and clip values of edges.
void CalBlkPara_RDBK_SW_rv9(int blk_x, int blk_y, int blk_comp, int second_phase,//input blk position 
					int proc_edge[2], int edge_clip_left[2]/*or above*/, int edge_clip_right[2]/*or below*/,
				int *alpha, int *beta, int *beta2)
				//output 
				// a. boolean, whether or not to filter the edge 
				// b. boolean, weak or strong filter (if edge is filtered)
				// c. filter clip level on each side of edge 

				/*blk order in this function is 
				 0  1  2  3  4	5			  TL0  TL1  T0  T1  T2  T3
				 6  7  8  9  10 11			  L0   L1   0   1   2   3
				 12 13 14 15 16 17 represent  L2   L3   4   5   6   7
				 18 19 20 21 22 23			  L4   L5   8   9   10  11
				 24 25 26 27 28 29            L6   L7   12  13  14  15 */
{
	uint8 bOnLeftEdge;
	uint8 bOnTopEdge;
	uint8 bOnBottomEdge;
	uint8 bOnRightEdge;
	uint8 beTureBPic;
	uint8 beSmallPic;

	uint8	edge1_need_filt;
	uint8	edge2_need_filt;
	uint8	edge3_need_filt;
	uint8	edge4_need_filt;

	uint8 mbtype;
	uint8 mbtype_left;
	uint8 mbtype_above;
	uint8 mbtype_topleft;

	uint32 Full_CBP;
	uint32 Full_CBP_left;
	uint32 Full_CBP_above;
	uint32 Full_CBP_topleft;

	uint32 cbp;
	uint32 cbp_left;
	uint32 cbp_above;
	uint32 cbp_topleft;
	
	uint16 mvd;
	uint16 mvd_left;
	uint16 mvd_above;
	uint16 mvd_topleft;

	uint8  uQP;
	uint8  uQP_current;
	uint8  uQP_above;
	uint8  uQP_left;

	uint32 cbpc[2];
	uint32 cbpc_left[2];
	uint32 cbpc_above[2];
	uint32 cbpc_topleft[2];

	uint32 expandMbCbp;
	uint32 expandMbMvd;
	uint32 expandMbbnzs;

	uint32 expandMbCbp_c[2];


	uint8  blk_cbp_current;
	uint8  blk_cbp_above;
	uint8  blk_cbp_left;
	uint8  blk_cbp_below;

	uint8  blk_mvd_current;
	uint8  blk_mvd_below;

	uint8  blk_bnzs_current;
	uint8  blk_bnzs_above;
	uint8  blk_bnzs_left;
	uint8  blk_bnzs_below;

	uint8  bFilterBlks;

	uint8  blk_id;


	uint8	edge1_clip_strenth_right;
	uint8	edge1_clip_strenth_left;
	uint8	edge3_clip_strenth_right;
	uint8	edge3_clip_strenth_left;
	uint8	edge24_clip_strenth_right;
	uint8	edge24_clip_strenth_left;

	uint8	edge1_cbp_right;
	uint8	edge1_cbp_left;
	uint8	edge1_mvd_right;
	uint8	edge1_bnzs_right;
	uint8	edge1_bnzs_left;	

	uint8	edge3_cbp_right;
	uint8	edge3_cbp_left;
	uint8	edge3_mvd_right;
	uint8	edge3_bnzs_right;
	uint8	edge3_bnzs_left;	

	uint8	edge24_cbp_right;
	uint8	edge24_cbp_left;
	uint8	edge24_mvd_right;
	uint8	edge24_bnzs_right;
	uint8	edge24_bnzs_left;

	uint8	b_topleft_above_diff;
	uint8	b_topleft_left_diff;
	uint8	b_left_current_diff;
	uint8	b_above_current_diff;



	uint8	b_edge1_filtered;
	uint8	b_edge3_filtered;
	uint8	b_edge24_filtered;

	uint8	start_x;
	uint8	end_x;
	uint8	start_y;
	uint8	end_y;

	uint8	b_edge1_frame_edge;
	uint8	b_edge3_frame_edge;
	uint8	b_edge24_frame_edge;

	uint8	b_edge1_MB_edge;
	uint8	b_edge3_MB_edge;
	uint8	b_edge24_MB_edge;

	uint8	b_edge1_ref_diff_inB;
	uint8	b_edge3_ref_diff_inB;
	uint8	b_edge24_ref_diff_inB;

	uint8	strenth_current;
	uint8	strenth_left;
	uint8	strenth_above;
	uint8	strenth_topleft;

	uint8	strenth_edge1_left;
	uint8	strenth_edge1_right;
	uint8	strenth_edge3_left;
	uint8	strenth_edge3_right;
	uint8	strenth_edge24_left;
	uint8	strenth_edge24_right;

	uint8	b_edge1_strength2_MB_edge;
	uint8	b_edge1_filter_strong;
	uint8	b_edge3_strength2_MB_edge;
	uint8	b_edge3_filter_strong;
	uint8	b_edge24_strength2_MB_edge;
	uint8	b_edge24_filter_strong;
	/*get para from register*/
	// Add QP_left;

	mbtype			= (g_dbk_reg_ptr->rdbk_cfg0 >>24) & 0xf;
	Full_CBP		= (g_dbk_reg_ptr->rdbk_cfg0) & 0xffffff;
	
	mbtype_left		= (g_dbk_reg_ptr->rdbk_cfg1 >>24) & 0xf;
	Full_CBP_left	= (g_dbk_reg_ptr->rdbk_cfg1) & 0xffffff;

	mbtype_above	= (g_dbk_reg_ptr->rdbk_cfg2 >>24) & 0xf;
	Full_CBP_above	= (g_dbk_reg_ptr->rdbk_cfg2) & 0xffffff;
	
	mbtype_topleft	= (g_dbk_reg_ptr->rdbk_cfg3 >>24) & 0xf;	
	Full_CBP_topleft= (g_dbk_reg_ptr->rdbk_cfg3) & 0xffffff;

	mvd			= (g_dbk_reg_ptr->rdbk_cfg4 >> 16) & 0xffff;
	mvd_left	= (g_dbk_reg_ptr->rdbk_cfg4) & 0xffff;

	mvd_above	= (g_dbk_reg_ptr->rdbk_cfg5 >> 16) & 0xffff;
	mvd_topleft	= (g_dbk_reg_ptr->rdbk_cfg5) & 0xffff;

	beSmallPic	= (g_dbk_reg_ptr->rdbk_cfg6 >>26) & 0x1;
	beTureBPic	= (g_dbk_reg_ptr->rdbk_cfg6 >>25) & 0x1;
	bOnRightEdge= (g_dbk_reg_ptr->rdbk_cfg6 >>24) & 0x1;
	bOnLeftEdge = (g_dbk_reg_ptr->rdbk_cfg6 >>23) & 0x1;
	bOnBottomEdge= (g_dbk_reg_ptr->rdbk_cfg6>>22) & 0x1;
	bOnTopEdge	= (g_dbk_reg_ptr->rdbk_cfg6 >>21) & 0x1;
	uQP_left	= (g_dbk_reg_ptr->rdbk_cfg6 >>16)& 0x1f;
	uQP_above	= (g_dbk_reg_ptr->rdbk_cfg6 >>8) & 0x1f;
	uQP_current	= (g_dbk_reg_ptr->rdbk_cfg6 >>0) & 0x1f;



	cbp			= Full_CBP & 0xffff;
	cbp_left	= Full_CBP_left & 0xffff;
	cbp_above	= Full_CBP_above & 0xffff;
	cbp_topleft = Full_CBP_topleft & 0xffff;

	cbpc[0]		= (Full_CBP >>16) & 0xf;
	cbpc[1]		= (Full_CBP >>20) & 0xf;
	cbpc_left[0]= (Full_CBP_left >>16) & 0xf;
	cbpc_left[1]= (Full_CBP_left >>20) & 0xf;
	cbpc_above[0]	= (Full_CBP_above >>16) & 0xf;
	cbpc_above[1]	= (Full_CBP_above >>20) & 0xf;
	cbpc_topleft[0] = (Full_CBP_topleft >>16) & 0xf;
	cbpc_topleft[1] = (Full_CBP_topleft >>20) & 0xf;


	/*new ExpandMB CBP[29:0] =
	15 14 13 12 L7 L6 11 10 9 8 L5 L4 7 6 5 4 L3 L2 3 2 1 0 L1 L0 T3 T2 T1 T0 TL1 TL0

	where L7 = 15L; L6 = 14L; L5 = 11L; L4 = 10L; L3 = 7L; L2 = 6L; L1 = 3L; L0 = 2L;
	T3 = 15T; T2 = 14T; T1 = 13T; T0 = 12T;
	TL1 = 15TL; TL0 = 14TL;*/
	expandMbCbp = (((cbp >>12) & 0xf) <<26) | (((cbp_left >>14) &0x3) <<24)
				 | (((cbp >>8) & 0xf) <<20) | (((cbp_left >>10) &0x3) <<18)
				 | (((cbp >>4) & 0xf) <<14) | (((cbp_left >> 6) &0x3) <<12)
				 | (((cbp >>0) & 0xf) <<8)  | (((cbp_left >> 2) &0x3) <<6)
				 | (((cbp_above >>12) & 0xf) <<2) | (((cbp_topleft >>14) &0x3) <<0);

	/*new ExpandMB CBPc[11:0] = 

	3 2 L3 L2 1 0 L1 L0 T1 T0 TL1 TL0
	*/
	expandMbCbp_c[0] = (((cbpc[0] >>2) & 0x3) <<10) | (((cbpc_left[0] >>2) & 0x3) <<8)
				 	 | (((cbpc[0] >>0) & 0x3) <<6) | (((cbpc_left[0] >>0) & 0x3) <<4)
					 | (((cbpc_above[0] >>2) & 0x3) <<2) | (((cbpc_topleft[0] >>2) & 0x3) <<0);

	expandMbCbp_c[1] = (((cbpc[1] >>2) & 0x3) <<10) | (((cbpc_left[1] >>2) & 0x3) <<8)
				 	 | (((cbpc[1] >>0) & 0x3) <<6) | (((cbpc_left[1] >>0) & 0x3) <<4)
					 | (((cbpc_above[1] >>2) & 0x3) <<2) | (((cbpc_topleft[1] >>2) & 0x3) <<0);

	/*new ExpandMB Mvd[24:0] =
	15 14 13 12 L7 L6 11 10 9 8 L5 L4 7 6 5 4 L3 L2 3 2 1 0 L1 L0 T3 T2 T1 T0 TL1 TL0
	*/
	expandMbMvd = (((mvd >>12) & 0xf) <<26) | (((mvd_left >>14) &0x3) <<24)
				 | (((mvd >>8) & 0xf) <<20) | (((mvd_left >>10) &0x3) <<18)
				 | (((mvd >>4) & 0xf) <<14) | (((mvd_left >> 6) &0x3) <<12)
				 | (((mvd >>0) & 0xf) <<8)  | (((mvd_left >> 2) &0x3) <<6)
				 | (((mvd_above >>12) & 0xf) <<2) | (((mvd_topleft >>14) &0x3) <<0);

	/*new Expandbnzs = expandMbCbp | expandMbMvd*/
	expandMbbnzs = expandMbCbp | expandMbMvd;

	b_topleft_above_diff	= deblockRefDiff(mbtype_above,mbtype_topleft);
	b_topleft_left_diff		= deblockRefDiff(mbtype_left,mbtype_topleft);
	b_left_current_diff		= deblockRefDiff(mbtype, mbtype_left);
	b_above_current_diff	= deblockRefDiff(mbtype, mbtype_above);


		strenth_current		 = getMBStrength(mbtype);
	strenth_left		 = getMBStrength(mbtype_left);
	strenth_above		 = getMBStrength(mbtype_above);
	strenth_topleft		 = getMBStrength(mbtype_topleft);



	/* select para to calculate para of filtering current edge of blk*/
	if (blk_comp == 0)
	{
		blk_id = blk_x + blk_y * 6;

		blk_cbp_current = ((expandMbCbp >> blk_id) & 0x1);
		blk_cbp_above	= (blk_y == 0)? 0: ((expandMbCbp >> (blk_id -6)) & 0x1);
		blk_cbp_left	= (blk_x == 0)? 0: ((expandMbCbp >> (blk_id -1)) & 0x1);
		blk_cbp_below	= (blk_y == 4)? 0: ((expandMbCbp >> (blk_id +6)) & 0x1);

		blk_mvd_current	= ((expandMbMvd >> blk_id) & 0x1);
		blk_mvd_below	= (blk_y == 4)? 0: ((expandMbMvd >> (blk_id +6)) & 0x1);

		blk_bnzs_current= ((expandMbbnzs >> blk_id) & 0x1);
		blk_bnzs_above	= (blk_y == 0)? 0: ((expandMbbnzs >> (blk_id -6)) & 0x1);
		blk_bnzs_left	= (blk_x == 0)? 0: ((expandMbbnzs >> (blk_id -1)) & 0x1);
		blk_bnzs_below	= (blk_y == 4)? 0: ((expandMbbnzs >> (blk_id +6)) & 0x1);

	}
	else // Chroma
	{
		blk_id = blk_x + blk_y * 4;

		if (blk_comp == 1)
		{
			blk_cbp_current = ((expandMbCbp_c[0] >> blk_id) & 0x1);
			blk_cbp_above	= (blk_y == 0)? 0: ((expandMbCbp_c[0] >> (blk_id -4)) & 0x1);
			blk_cbp_left	= (blk_x == 0)? 0: ((expandMbCbp_c[0] >> (blk_id -1)) & 0x1);
			blk_cbp_below	= (blk_y == 2)? 0: ((expandMbCbp_c[0] >> (blk_id +4)) & 0x1);
		} 
		else
		{
			blk_cbp_current = ((expandMbCbp_c[1] >> blk_id) & 0x1);
			blk_cbp_above	= (blk_y == 0)? 0: ((expandMbCbp_c[1] >> (blk_id -4)) & 0x1);
			blk_cbp_left	= (blk_x == 0)? 0: ((expandMbCbp_c[1] >> (blk_id -1)) & 0x1);
			blk_cbp_below	= (blk_y == 2)? 0: ((expandMbCbp_c[1] >> (blk_id +4)) & 0x1);
		}

		blk_mvd_current	= 0;
		blk_mvd_below	= 0;

		blk_bnzs_current= blk_cbp_current;
		blk_bnzs_above	= blk_cbp_above;
		blk_bnzs_left	= blk_cbp_left;
		blk_bnzs_below	= blk_cbp_below;


	}

	/************************************************************************/
	/* NEED TO BE MODEIFIED !!!!!!								            */
	/************************************************************************/




	// edge1 
	edge1_cbp_right		= blk_cbp_below;
	edge1_cbp_left		= blk_cbp_current;

	edge1_mvd_right		= blk_mvd_below;

	edge1_bnzs_right	= blk_bnzs_below;
	edge1_bnzs_left		= blk_bnzs_current;

	// edge3
	edge3_cbp_right		= blk_cbp_current;
	edge3_cbp_left		= blk_cbp_above;

	edge3_mvd_right		= blk_mvd_current;

	edge3_bnzs_right	= blk_bnzs_current;
	edge3_bnzs_left		= blk_bnzs_above;

	//edge 2 or 4

	edge24_cbp_right	= blk_cbp_current;
	edge24_cbp_left		= blk_cbp_left;

	edge24_mvd_right	= blk_mvd_current;

	edge24_bnzs_right	= blk_bnzs_current;
	edge24_bnzs_left	= blk_bnzs_left;






	/* For normal MB, 16 blks are filtered: 
	T0 T1 T2 T3 from above MB, L1 L3 L5 from Left MB and 0 1 2 4 5 6 8 9 10 from current MB.
	If MB is on top edge of frame, T0 T1 T2 T3 are not filtered.
	If MB is on left edge of frame, L1 L3 L5 are not filtered.
	If MB is on right edge of frame, 3 7 11 are filtered.
	If MB is on bottom edge of frame, L7 12 13 14 of current MB is filtered
	If MB is on left bottom of  frame, L7 is not filtered
	If MB is on right bottom of  frame, 15 is filtered.*/


	if (blk_comp == 0)
	{
		start_x = bOnLeftEdge ? 2 : 1;
		end_x	= bOnRightEdge? 5 : 4;
		start_y = 1;
		end_y	= bOnBottomEdge? 4 : 3;

		bFilterBlks = (blk_x >= start_x && blk_x <= end_x && blk_y >= start_y && blk_y <=end_y)
					|| (!bOnTopEdge && blk_x >=2 && blk_x<=5 && blk_y ==0);
	} 
	else
	{
		start_x = bOnLeftEdge ? 2 : 1;
		end_x	= bOnRightEdge? 3 : 2;
		start_y = 1;
		end_y	= bOnBottomEdge? 2 : 1;

		bFilterBlks = (blk_x >= start_x && blk_x <= end_x && blk_y >= start_y && blk_y <=end_y)
					|| (!bOnTopEdge && blk_x >=2 && blk_x<=3 && blk_y ==0);		
	}

	/* Edge boolean determination */
	/* -------------------------- */
	/* a. whether or not to filter the edge (bit=1 indicates filter edge) */
	/* */
	/*	NO, unless: */
	/*   i. cbp bit for block on either side set */
	/*   ii. mvd bit for block below or to right of edge set */
	/*   iii. when B frame, MB edge, reference frames differ for adjacent MB */
	/*  also,  picture edges are never filtered */





	// whether edge 1 is filtered
	b_edge1_frame_edge	= bOnBottomEdge && ((blk_comp == 0 && blk_y== 4) || (blk_comp != 0 && blk_y== 2));
	
	b_edge1_MB_edge		= blk_y == 0 ? 1: 0;

	b_edge1_ref_diff_inB= beTureBPic && b_edge1_MB_edge && b_above_current_diff;

	b_edge1_filtered	= bFilterBlks && !b_edge1_frame_edge &&  (edge1_cbp_right || edge1_cbp_left || edge1_mvd_right || b_edge1_ref_diff_inB);


	// whether edge3 is filtered
	b_edge3_frame_edge	= bOnTopEdge && (blk_y == 1);
	
	b_edge3_MB_edge		= blk_y == 1 ? 1: 0;

	b_edge3_ref_diff_inB= beTureBPic && b_edge3_MB_edge && ((blk_x == 1)? b_topleft_left_diff : b_above_current_diff);

	b_edge3_filtered	= bFilterBlks && !b_edge3_frame_edge &&  (edge3_cbp_right || edge3_cbp_left || edge3_mvd_right || b_edge3_ref_diff_inB);

	// whether edge 2 or 4 is filtered
	b_edge24_frame_edge	= bOnLeftEdge && (blk_x == 2);
	
	b_edge24_MB_edge	= blk_x == 2? 1: 0;

	b_edge24_ref_diff_inB= beTureBPic && b_edge24_MB_edge && ((blk_y == 0) ? b_topleft_above_diff : b_left_current_diff);

	b_edge24_filtered	= bFilterBlks && !b_edge24_frame_edge &&  (edge24_cbp_right || edge24_cbp_left || edge24_mvd_right || b_edge24_ref_diff_inB);



	/* b. strong or weak filter (bit=1 indicates strong) */
	/* */
	/*	WEAK, unless: */
	/*   i. MB edge and this or adjacent MB filter strength = 2 */
	/*   ii. B frame, MB edge, reference frames differ for adjacent MB */


	// edge 1  strenth

	strenth_edge1_left		= blk_y == 0 ? strenth_above : blk_x == 1 ? strenth_left : strenth_current;
	strenth_edge1_right		= blk_x == 1 ? strenth_left : strenth_current;

	b_edge1_strength2_MB_edge = ((strenth_edge1_left == 2) || (strenth_edge1_right == 2)) && b_edge1_MB_edge;
	
	b_edge1_filter_strong	= b_edge1_strength2_MB_edge || b_edge1_ref_diff_inB;

	// edge 3 strenth
	strenth_edge3_left		= blk_x == 1 ? strenth_topleft :strenth_above;
	strenth_edge3_right		= blk_x == 1 ? strenth_left :strenth_current;

	b_edge3_strength2_MB_edge = ((strenth_edge3_left == 2) || (strenth_edge3_right == 2)) && b_edge3_MB_edge;
	
	b_edge3_filter_strong	= b_edge3_strength2_MB_edge || b_edge3_ref_diff_inB;

	// edge 2 or 4 strenth

	strenth_edge24_left		= (blk_y == 0 && blk_x <3 ) ? strenth_topleft : 
								blk_y == 0 ? strenth_above : blk_x <3 ? strenth_left : strenth_current;

	strenth_edge24_right	= blk_y == 0 ? strenth_above : blk_x <2 ? strenth_left : strenth_current;

	b_edge24_strength2_MB_edge = ((strenth_edge24_left == 2) || (strenth_edge24_right == 2)) && b_edge24_MB_edge;
	
	b_edge24_filter_strong	= b_edge24_strength2_MB_edge || b_edge24_ref_diff_inB;




	/* Clip levels are determined by finding the clip levels for the current */
	/* and adjacent MBs (dependent on MB filter strength), then adjusting on a block basis */
	/* dependent upon block location, coded coeffs, and in B frames adjusted */
	/* when adjacent MBs use different reference frames. */



	edge1_clip_strenth_right = b_edge1_ref_diff_inB ? 2: edge1_bnzs_right ? strenth_edge1_right : 0;
	edge1_clip_strenth_left =  edge1_bnzs_left ? strenth_edge1_left : 0;

	edge3_clip_strenth_right = b_edge3_ref_diff_inB ? 2: edge3_bnzs_right ? strenth_edge3_right : 0;
	edge3_clip_strenth_left =  edge3_bnzs_left ? strenth_edge3_left : 0;

	edge24_clip_strenth_right = b_edge24_ref_diff_inB ? 2: edge24_bnzs_right ? strenth_edge24_right : 0;
	edge24_clip_strenth_left =  edge24_bnzs_left ? strenth_edge24_left : 0;

	/*if blk_y == 0, uses QP of above MB to calculate alpha, beta, beta2
	  if blk_x == 1 ,uses QP of left MB,
	  else uses current QP of current MB*/
	uQP					= (blk_y == 0)? uQP_above : (blk_x == 1)? uQP_left: uQP_current;



	if (!second_phase)
	{
		edge1_need_filt = b_edge1_filtered && !b_edge1_filter_strong;
		edge2_need_filt = (blk_y == 1) ?0: (b_edge24_filtered && !b_edge24_filter_strong);
		edge3_need_filt = 0;
		edge4_need_filt = (blk_y == 1) ?0: (b_edge24_filtered && b_edge24_filter_strong);
	}
	else
	{
		edge1_need_filt = 0;
		edge2_need_filt = b_edge24_filtered && !b_edge24_filter_strong;
		edge3_need_filt = b_edge3_filtered	&& b_edge3_filter_strong;
		edge4_need_filt = b_edge24_filtered && b_edge24_filter_strong;
	}


	/* Output parameters*/
	if (!second_phase)
	{
		proc_edge[0] = edge1_need_filt ? 1 : 0;
		proc_edge[1] = edge2_need_filt ? 2 : edge4_need_filt ? 4 : 0;
	}
	else
	{
		proc_edge[0] = (edge3_need_filt && edge4_need_filt) ? 3 : edge2_need_filt ? 2 : 0;
		proc_edge[1] = (edge3_need_filt && edge4_need_filt) ? 4 : edge3_need_filt ? 3 : edge4_need_filt? 4: 0;
	}

	edge_clip_right[0] = proc_edge[0] == 1 ? clip_table[edge1_clip_strenth_right][uQP] :
	proc_edge[0] == 3 ? clip_table[edge3_clip_strenth_right][uQP] : clip_table[edge24_clip_strenth_right][uQP];

	edge_clip_left[0] = proc_edge[0] == 1 ? clip_table[edge1_clip_strenth_left][uQP] :
	proc_edge[0] == 3 ? clip_table[edge3_clip_strenth_left][uQP] : clip_table[edge24_clip_strenth_left][uQP];
	
	edge_clip_right[1] = proc_edge[1] == 1 ? clip_table[edge1_clip_strenth_right][uQP] :
	proc_edge[1] == 3 ? clip_table[edge3_clip_strenth_right][uQP] : clip_table[edge24_clip_strenth_right][uQP];

	edge_clip_left[1] = proc_edge[1] == 1 ? clip_table[edge1_clip_strenth_left][uQP] :
	proc_edge[1] == 3 ? clip_table[edge3_clip_strenth_left][uQP] : clip_table[edge24_clip_strenth_left][uQP];
	

	* alpha				= alpha_tab[uQP];
	* beta				= beta_tab[uQP];
	* beta2				= (beSmallPic && blk_comp==0)? 4*beta_tab[uQP] : 3*beta_tab[uQP];

	
}