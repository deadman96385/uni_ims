/******************************************************************************
 ** File Name:    h264dec_deblock.c                                           *
 ** Author:       Xiaowei Luo                                                 *
 ** DATE:         01/23/2007                                                  *
 ** Copyright:    2006 Spreatrum, Incoporated. All Rights Reserved.           *
 ** Description:                                                              *
 *****************************************************************************/
/******************************************************************************
 **                   Edit    History                                         *
 **---------------------------------------------------------------------------* 
 ** DATE          NAME            DESCRIPTION                                 * 
 ** 01/23/2007    Xiaowei Luo     Create.                                     *
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

LOCAL void H264Dec_Config_DBK (DEC_IMAGE_PARAMS_T *img_ptr, DEC_MB_INFO_T *mb_info_ptr, DEC_MB_CACHE_T *mb_cache_ptr)
{
	int32 mb_x = img_ptr->mb_x;
	int32 mb_y = img_ptr->mb_y;
	int32 top_mb_qp = 0;
	int32 left_mb_qp = 0;
	uint32 cmd;
	uint32 *BS = (uint32 *)(mb_cache_ptr->BS);

	if (mb_y > 0)
	{
		top_mb_qp = img_ptr->abv_mb_info->qp;
	}

	if (mb_x > 0)
	{
		left_mb_qp = (mb_info_ptr - 1)->qp;
	}

	//polling dbk ready
	VSP_READ_REG_POLL_CQM(VSP_DBK_REG_BASE+HDBK_CFG_FINISH_OFF, 0, 1, 0,"dbk: polling dbk cfg finish flag = 0");

	cmd = ((mb_x << 24) | (top_mb_qp << 16) | (left_mb_qp << 8) | (mb_info_ptr->qp << 0));
	VSP_WRITE_REG_CQM (VSP_DBK_REG_BASE+HDBK_MB_INFO_OFF, cmd, "configure mb information");

	cmd = (((img_ptr->chroma_qp_offset & 0x1f) << 16) | ((img_ptr->curr_slice_ptr->LFAlphaC0Offset & 0x1f) << 8) | ((img_ptr->curr_slice_ptr->LFBetaOffset & 0x1f) << 0));
	VSP_WRITE_REG_CQM (VSP_DBK_REG_BASE+HDBK_PARS_OFF, cmd, "configure dbk parameter");

#if 0
	BS[0] = BS[1] = BS[2] = BS[3] = 0x0;
#endif	

	VSP_WRITE_REG_CQM(VSP_DBK_REG_BASE+HDBK_BS_H0_OFF, BS[0], "configure bs h0");
	VSP_WRITE_REG_CQM(VSP_DBK_REG_BASE+HDBK_BS_H1_OFF, BS[1], "configure bs h1");
	VSP_WRITE_REG_CQM(VSP_DBK_REG_BASE+HDBK_BS_V0_OFF, BS[2], "configure bs v0");
	VSP_WRITE_REG_CQM(VSP_DBK_REG_BASE+HDBK_BS_V1_OFF, BS[3], "configure bs v1");
	VSP_WRITE_REG_CQM(VSP_DBK_REG_BASE+HDBK_CFG_FINISH_OFF, 1, "config finished, start dbk");

	VSP_WRITE_CMD_INFO((VSP_DBK << CQM_SHIFT_BIT)| (8<<24) |(HDBK_PARS_WOFF<<16) | (HDBK_MB_INFO_WOFF<<8) | ((1<<7)|HDBK_CFG_FINISH_WOFF));
	VSP_WRITE_CMD_INFO((HDBK_BS_V1_WOFF<<24) |(HDBK_BS_V0_WOFF<<16) | (HDBK_BS_H1_WOFF<<8) | HDBK_BS_H0_WOFF);
	VSP_WRITE_CMD_INFO(HDBK_CFG_FINISH_WOFF);

	return;
}

LOCAL void H264Dec_BS_Para_intraMB (DEC_IMAGE_PARAMS_T *img_ptr, DEC_MB_INFO_T *mb_info_ptr, DEC_MB_CACHE_T *mb_cache_ptr)
{
	int32 leftEdgeFilterFlag = mb_cache_ptr->left_edge_filter_flag;
	int32 topEdgeFilterFlag = mb_cache_ptr->top_edge_filter_flag;

	if (leftEdgeFilterFlag)
	{
		mb_cache_ptr->BS[0] = 0x4444;
	}else
	{
		mb_cache_ptr->BS[0] = 0x0000;
	}

	mb_cache_ptr->BS[1] = 
	mb_cache_ptr->BS[2] = 
	mb_cache_ptr->BS[3] = 0x3333;

	if (topEdgeFilterFlag)
	{
		mb_cache_ptr->BS[4] = 0x4444;
	}else
	{
		mb_cache_ptr->BS[4] = 0x0000;
	}

	mb_cache_ptr->BS[5] =
	mb_cache_ptr->BS[6] =
	mb_cache_ptr->BS[7] = 0x3333;

	return;
}

LOCAL void BS_and_Para_interMB_hor_BSLICE (DEC_IMAGE_PARAMS_T *img_ptr, DEC_MB_INFO_T *mb_info_ptr, DEC_MB_CACHE_T *mb_cache_ptr)
{	
	int32 start_edge = 0, edge, edgePart;
	int16 *BS = &(mb_cache_ptr->BS[0]);

	if (mb_cache_ptr->left_edge_filter_flag)
	{
		if( (mb_info_ptr - 1)->is_intra)
		{
			start_edge = 1;
			BS[0] = 0x4444;
		}
	}else
	{
		start_edge = 1;
		BS[0] = 0x0000;
	}

//	if (!mb_cache_ptr->is_skipped || mb_cache_ptr->is_direct)
	{
		int32 ref_p0, ref_p1, ref_q0, ref_q1;
		uint32 tmp_bs;
		int8  *ref_idx_cache0 = mb_cache_ptr->ref_idx_cache[0] + CTX_CACHE_WIDTH_PLUS4;
		int8  *ref_idx_cache1 = mb_cache_ptr->ref_idx_cache[1] + CTX_CACHE_WIDTH_PLUS4;
		int16 *mv_cache0 = mb_cache_ptr->mv_cache[0] + CTX_CACHE_WIDTH_PLUS4_X2;
		int16 *mv_cache1 = mb_cache_ptr->mv_cache[1] + CTX_CACHE_WIDTH_PLUS4_X2;
		int32 *ref_pic_id_cache0 = mb_cache_ptr->ref_pic_id_cache[0] + CTX_CACHE_WIDTH_PLUS4;
		int32 *ref_pic_id_cache1 = mb_cache_ptr->ref_pic_id_cache[1] + CTX_CACHE_WIDTH_PLUS4;
		int8 *nnz_cache = mb_cache_ptr->nnz_cache + CTX_CACHE_WIDTH_PLUS4;
	
		if (start_edge == 1)
		{
			mv_cache0 += (1*2);
			mv_cache1 += (1*2);
			ref_pic_id_cache0 ++;
			ref_pic_id_cache1 ++;
			ref_idx_cache0 ++;
			ref_idx_cache1 ++;
			nnz_cache++;
		}
		
		for (edge = start_edge; edge < 4; edge++)
		{
			BS[edge] = 0; 
			for (edgePart = 0; edgePart < 4; edgePart++)
			{
				if (nnz_cache[-1] || nnz_cache[0])
				{
					tmp_bs = 2;
				}else
				{
					ref_p0 = ref_idx_cache0[-1]< 0 ? INVALID_REF_ID : ref_pic_id_cache0[-1];
					ref_q0 = ref_idx_cache0[0]  < 0 ? INVALID_REF_ID : ref_pic_id_cache0[0];
					ref_p1 = ref_idx_cache1[-1]< 0 ? INVALID_REF_ID : ref_pic_id_cache1[-1];
					ref_q1 = ref_idx_cache1[0]  < 0 ? INVALID_REF_ID : ref_pic_id_cache1[0];

					if ( ((ref_p0==ref_q0) && (ref_p1==ref_q1)) || ((ref_p0==ref_q1) && (ref_p1==ref_q0)))
					{ 					
						// L0 and L1 reference pictures of p0 are different; q0 as well
						if (ref_p0 != ref_p1)
						{
							 // compare MV for the same reference picture
							 if (ref_p0 == ref_q0)
						  	{
								if (ref_p0 == INVALID_REF_ID)
								{
									  tmp_bs = ((ABS(mv_cache1[-2] - mv_cache1[0]) >= 4) || (ABS(mv_cache1[-1] - mv_cache1[1]) >= 4));
								}else if (ref_p1 == INVALID_REF_ID)
								{
									  tmp_bs = ((ABS(mv_cache0[-2] - mv_cache0[0]) >= 4) || (ABS(mv_cache0[-1] - mv_cache0[1]) >= 4));
								}else
								{
									  tmp_bs = ((ABS(mv_cache0[-2] - mv_cache0[0]) >= 4) || (ABS(mv_cache0[-1] - mv_cache0[1]) >= 4) ||
										(ABS(mv_cache1[-2] - mv_cache1[0]) >= 4) || (ABS(mv_cache1[-1] - mv_cache1[1]) >= 4));
								}
							} else
						  	{
								tmp_bs =   (	(ABS(mv_cache0[-2] - mv_cache1[0]) >= 4) ||(ABS(mv_cache0[-1] - mv_cache1[1]) >= 4) ||
							  				(ABS(mv_cache1[-2] - mv_cache0[0]) >= 4) ||(ABS(mv_cache1[-1] - mv_cache0[1]) >= 4));
						  	}
						}else
						{ // L0 and L1 reference pictures of p0 are the same; q0 as well
						  	tmp_bs =  (	((ABS( mv_cache0[-2] - mv_cache0[0]) >= 4) ||(ABS( mv_cache0[-1] - mv_cache0[1]) >= 4 ) ||
								  		 (ABS( mv_cache1[-2] - mv_cache1[0]) >= 4) ||(ABS( mv_cache1[-1] - mv_cache1[1]) >= 4)) &&
										((ABS( mv_cache0[-2] - mv_cache1[0]) >= 4) ||(ABS( mv_cache0[-1] - mv_cache1[1]) >= 4) ||
										 (ABS( mv_cache1[-2] - mv_cache0[0]) >= 4) ||(ABS( mv_cache1[-1] - mv_cache0[1]) >= 4)));
						}
					}else
					{
						tmp_bs = 1;
					}     
				}
				
				mv_cache0 += 2*CTX_CACHE_WIDTH;
				mv_cache1 += 2*CTX_CACHE_WIDTH;
				ref_idx_cache0 += CTX_CACHE_WIDTH;
				ref_idx_cache1 += CTX_CACHE_WIDTH;
				ref_pic_id_cache0 += CTX_CACHE_WIDTH;
				ref_pic_id_cache1 += CTX_CACHE_WIDTH;
				nnz_cache += CTX_CACHE_WIDTH;

				BS[edge] |= (tmp_bs << (4*edgePart));
			}

			mv_cache0 += 2*(-4*CTX_CACHE_WIDTH+1);
			mv_cache1 += 2*(-4*CTX_CACHE_WIDTH+1);
			ref_idx_cache0 += (-4*CTX_CACHE_WIDTH+1);
			ref_idx_cache1 += (-4*CTX_CACHE_WIDTH+1);
			ref_pic_id_cache0 += (-4*CTX_CACHE_WIDTH+1);
			ref_pic_id_cache1 += (-4*CTX_CACHE_WIDTH+1);
			nnz_cache += (-4*CTX_CACHE_WIDTH+1);
		}
	}		

	return;
}

LOCAL void BS_and_Para_interMB_ver_BSLICE (DEC_IMAGE_PARAMS_T *img_ptr, DEC_MB_INFO_T *mb_info_ptr, DEC_MB_CACHE_T *mb_cache_ptr)
{
	int32 start_edge = 0, edge, edgePart;
	int16 *BS = &(mb_cache_ptr->BS[4]);

	if (mb_cache_ptr->top_edge_filter_flag)
	{
		if( img_ptr->abv_mb_info->is_intra)
		{
			start_edge = 1;
			BS[0] = 0x4444;
		}
	}else
	{
		start_edge = 1;
		BS[0] = 0x0000;
	}

//	if (!mb_cache_ptr->is_skipped || mb_cache_ptr->is_direct)
	{
		int32 ref_p0, ref_p1, ref_q0, ref_q1;
		uint32 tmp_bs;
		int8  *ref_idx_cache0 = mb_cache_ptr->ref_idx_cache[0] + CTX_CACHE_WIDTH_PLUS4;
		int8  *ref_idx_cache1 = mb_cache_ptr->ref_idx_cache[1] + CTX_CACHE_WIDTH_PLUS4;
		int16 *mv_cache0 = mb_cache_ptr->mv_cache[0] + CTX_CACHE_WIDTH_PLUS4_X2;
		int16 *mv_cache1 = mb_cache_ptr->mv_cache[1] + CTX_CACHE_WIDTH_PLUS4_X2;
		int32 *ref_pic_id_cache0 = mb_cache_ptr->ref_pic_id_cache[0] + CTX_CACHE_WIDTH_PLUS4;
		int32 *ref_pic_id_cache1 = mb_cache_ptr->ref_pic_id_cache[1] + CTX_CACHE_WIDTH_PLUS4;
		int8 *nnz_cache = mb_cache_ptr->nnz_cache + CTX_CACHE_WIDTH_PLUS4;
		
		if (start_edge == 1)
		{
			mv_cache0 += CTX_CACHE_WIDTH_X2;
			mv_cache1 += CTX_CACHE_WIDTH_X2;
			ref_pic_id_cache0 += CTX_CACHE_WIDTH;
			ref_pic_id_cache1 += CTX_CACHE_WIDTH;
			ref_idx_cache0 += CTX_CACHE_WIDTH;
			ref_idx_cache1 += CTX_CACHE_WIDTH;
			nnz_cache += CTX_CACHE_WIDTH;
		}
		
		for (edge = start_edge; edge < 4; edge++)
		{
			BS[edge] = 0; 
			for (edgePart = 0; edgePart < 4; edgePart++)
			{
				if (nnz_cache[-CTX_CACHE_WIDTH] || nnz_cache[0])
				{
					tmp_bs = 2;
				}else
				{
					ref_p0 = ref_idx_cache0[-CTX_CACHE_WIDTH]< 0 ? INVALID_REF_ID : ref_pic_id_cache0[-CTX_CACHE_WIDTH];
					ref_p1 = ref_idx_cache1[-CTX_CACHE_WIDTH]< 0 ? INVALID_REF_ID : ref_pic_id_cache1[-CTX_CACHE_WIDTH];
					ref_q0 = ref_idx_cache0[0]< 0 ? INVALID_REF_ID : ref_pic_id_cache0[0];
					ref_q1 = ref_idx_cache1[0]< 0 ? INVALID_REF_ID : ref_pic_id_cache1[0];

					if ( ((ref_p0==ref_q0) && (ref_p1==ref_q1)) || ((ref_p0==ref_q1) && (ref_p1==ref_q0)))
					{
						// L0 and L1 reference pictures of p0 are different; q0 as well
						if (ref_p0 != ref_p1)
						{
							// compare MV for the same reference picture
							  if (ref_p0 == ref_q0)
						  	{
								if (ref_p0 == INVALID_REF_ID)
								{
									 tmp_bs = ((ABS(mv_cache1[-CTX_CACHE_WIDTH_X2] - mv_cache1[0]) >= 4) ||
									 		   (ABS(mv_cache1[-CTX_CACHE_WIDTH_X2 + 1] - mv_cache1[1]) >= 4));
								}else if (ref_p1 == INVALID_REF_ID)
								{
									tmp_bs = ((ABS( mv_cache0[-CTX_CACHE_WIDTH_X2] - mv_cache0[0]) >= 4) ||
											  (ABS(mv_cache0[-CTX_CACHE_WIDTH_X2 + 1] - mv_cache0[1]) >= 4));
								}else
								{
							  		tmp_bs = ((ABS( mv_cache0[-CTX_CACHE_WIDTH_X2] - mv_cache0[0]) >= 4) ||
											  (ABS(mv_cache0[-CTX_CACHE_WIDTH_X2 + 1] - mv_cache0[1]) >= 4) ||
											  (ABS(mv_cache1[-CTX_CACHE_WIDTH_X2] - mv_cache1[0]) >= 4) ||
											  (ABS(mv_cache1[-CTX_CACHE_WIDTH_X2 + 1] - mv_cache1[1]) >= 4));
								}
						  	} else
						  	{
								tmp_bs =   ( 	(ABS( mv_cache0[-CTX_CACHE_WIDTH_X2] - mv_cache1[0]) >= 4) ||(ABS(mv_cache0[-CTX_CACHE_WIDTH_X2 + 1] - mv_cache1[1]) >= 4) ||
				        						(ABS(mv_cache1[-CTX_CACHE_WIDTH_X2] - mv_cache0[0]) >= 4) || (ABS(mv_cache1[-CTX_CACHE_WIDTH_X2 + 1] - mv_cache0[1]) >= 4));
						  	}
						}else
						{ // L0 and L1 reference pictures of p0 are the same; q0 as well
						  	tmp_bs =  ((	(ABS(  mv_cache0[-CTX_CACHE_WIDTH_X2] - mv_cache0[0]) >= 4) ||(ABS( mv_cache0[-CTX_CACHE_WIDTH_X2 + 1] - mv_cache0[1]) >= 4 ) ||
										(ABS( mv_cache1[-CTX_CACHE_WIDTH_X2] - mv_cache1[0]) >= 4) ||(ABS( mv_cache1[-CTX_CACHE_WIDTH_X2 + 1] - mv_cache1[1]) >= 4)) &&
									(	(ABS(  mv_cache0[-CTX_CACHE_WIDTH_X2] - mv_cache1[0]) >= 4) ||(ABS( mv_cache0[-CTX_CACHE_WIDTH_X2 + 1] - mv_cache1[1]) >= 4) ||
										(ABS( mv_cache1[-CTX_CACHE_WIDTH_X2] - mv_cache0[0]) >= 4) ||(ABS( mv_cache1[-CTX_CACHE_WIDTH_X2 + 1] - mv_cache0[1]) >= 4)));
						}
					}else
					{
						tmp_bs = 1;
					}     
				}
				BS[edge] |= (tmp_bs << (4*edgePart));
				
				mv_cache0 += 2*1;
				mv_cache1 += 2*1;
				ref_idx_cache0 ++;
				ref_idx_cache1 ++;
				ref_pic_id_cache0 ++;
				ref_pic_id_cache1 ++;
				nnz_cache++;
			}	
			
			mv_cache0 += (CTX_CACHE_WIDTH - 4)*2;
			mv_cache1 +=  (CTX_CACHE_WIDTH - 4)*2;
			ref_idx_cache0 +=  (CTX_CACHE_WIDTH - 4);
			ref_idx_cache1 +=  (CTX_CACHE_WIDTH - 4);
			ref_pic_id_cache0 +=  (CTX_CACHE_WIDTH - 4);
			ref_pic_id_cache1 +=  (CTX_CACHE_WIDTH - 4);
			nnz_cache += (CTX_CACHE_WIDTH - 4);
		}
	}	

	return;
}


LOCAL void BS_and_Para_interMB_hor_PSLICE(DEC_IMAGE_PARAMS_T *img_ptr, DEC_MB_INFO_T *mb_info_ptr, DEC_MB_CACHE_T *mb_cache_ptr)
{	
	int32 start_edge = 0, edge, edgePart;
	int16 *BS = &(mb_cache_ptr->BS[0]);

	BS[0] = 0x0000;
	if (mb_cache_ptr->left_edge_filter_flag)
	{
		if( (mb_info_ptr - 1)->is_intra)
		{
			start_edge = 1;
			BS[0] = 0x4444;
		}
	}else
	{
		start_edge = 1;	
	}

//	if (!mb_cache_ptr->is_skipped)
	{
		uint32 tmp_bs;
		int8  *ref_idx_cache = mb_cache_ptr->ref_idx_cache[0] + CTX_CACHE_WIDTH_PLUS4;
 		int16 *mv_cache = mb_cache_ptr->mv_cache[0] + CTX_CACHE_WIDTH_PLUS4_X2;
 		int8 *nnz_cache = mb_cache_ptr->nnz_cache + CTX_CACHE_WIDTH_PLUS4;
	
		if (start_edge == 1)
		{
			mv_cache += (1*2);
 			ref_idx_cache ++;
 			nnz_cache++;
		}
		
		for (edge = start_edge; edge < 4; edge++)
		{
			BS[edge] = 0; 
			if (edge && mb_cache_ptr->is_skipped) continue;
			
			for (edgePart = 0; edgePart < 4; edgePart++)
			{
				if (nnz_cache[-1] || nnz_cache[0])
				{
					tmp_bs = 2;
				}else
				{
					tmp_bs = ((ref_idx_cache[-1] != ref_idx_cache[0]) ||((ABS(mv_cache[-2] - mv_cache[0]) >= 4) ||(ABS(mv_cache[-1] - mv_cache[1]) >=4)));
 				}
				
				mv_cache += 2*CTX_CACHE_WIDTH;
 				ref_idx_cache += CTX_CACHE_WIDTH;
 				nnz_cache += CTX_CACHE_WIDTH;

				BS[edge] |= (tmp_bs << (4*edgePart));
			}

			mv_cache += 2*(-4*CTX_CACHE_WIDTH+1);
 			ref_idx_cache += (-4*CTX_CACHE_WIDTH+1);
 			nnz_cache += (-4*CTX_CACHE_WIDTH+1);
		}
	}		

	return;
}

LOCAL void BS_and_Para_interMB_ver_PSLICE(DEC_IMAGE_PARAMS_T *img_ptr, DEC_MB_INFO_T *mb_info_ptr, DEC_MB_CACHE_T *mb_cache_ptr)
{
	int32 start_edge = 0, edge, edgePart;
	int16 *BS = &(mb_cache_ptr->BS[4]);

	BS[0] = 0x0000;
	if (mb_cache_ptr->top_edge_filter_flag)
	{
		if( img_ptr->abv_mb_info->is_intra)
		{
			start_edge = 1;
			BS[0] = 0x4444;
		}
	}else
	{
		start_edge = 1;		
	}

//	if (!mb_cache_ptr->is_skipped)
	{
		uint32 tmp_bs;
		int8  *ref_idx_cache = mb_cache_ptr->ref_idx_cache[0] + CTX_CACHE_WIDTH_PLUS4;
		int16 *mv_cache = mb_cache_ptr->mv_cache[0] + CTX_CACHE_WIDTH_PLUS4_X2;
		int8 *nnz_cache = mb_cache_ptr->nnz_cache + CTX_CACHE_WIDTH_PLUS4;
		
		if (start_edge == 1)
		{
			mv_cache += CTX_CACHE_WIDTH_X2;
			ref_idx_cache += CTX_CACHE_WIDTH;
			nnz_cache += CTX_CACHE_WIDTH;
		}
		
		for (edge = start_edge; edge < 4; edge++)
		{
			BS[edge] = 0; 
			if (edge && mb_cache_ptr->is_skipped)	continue;
			
			for (edgePart = 0; edgePart < 4; edgePart++)
			{
				if (nnz_cache[-CTX_CACHE_WIDTH] || nnz_cache[0])
				{
					tmp_bs = 2;
				}else
				{
					tmp_bs = (ref_idx_cache[-CTX_CACHE_WIDTH] != ref_idx_cache[0]) ||
						((ABS( mv_cache[-CTX_CACHE_WIDTH_X2] - mv_cache[0]) >= 4) ||
						(ABS(mv_cache[-CTX_CACHE_WIDTH_X2 + 1] - mv_cache[1]) >= 4));
				}
				BS[edge] |= (tmp_bs << (4*edgePart));
				
				mv_cache += 2*1;
				ref_idx_cache ++;
				nnz_cache++;
			}	
			
			mv_cache += (CTX_CACHE_WIDTH - 4)*2;
			ref_idx_cache +=  (CTX_CACHE_WIDTH - 4);
			nnz_cache += (CTX_CACHE_WIDTH - 4);
		}
	}	

	return;
}

PUBLIC void H264Dec_BS_and_Para (DEC_IMAGE_PARAMS_T *img_ptr, DEC_MB_INFO_T *mb_info_ptr, DEC_MB_CACHE_T *mb_cache_ptr)
{
	if (img_ptr->curr_slice_ptr->LFDisableIdc == 1) //not filtered
	{
		uint32 *BS = (uint32 *)(mb_cache_ptr->BS);	
		BS[0] = BS[1] = BS[2] = BS[3] = 0x0;
	}else
	{
#if 0
		if (img_ptr->curr_slice_ptr->LFDisableIdc == 2)
		{
			mb_cache_ptr->top_edge_filter_flag = mb_cache_ptr->mb_avail_b;
			mb_cache_ptr->left_edge_filter_flag = mb_cache_ptr->mb_avail_a;
		}else
		{
			mb_cache_ptr->top_edge_filter_flag = ((img_ptr->mb_y > 0) ? 1 : 0);
			mb_cache_ptr->left_edge_filter_flag = ((img_ptr->mb_x > 0) ? 1 : 0);
		}

		if (mb_info_ptr->is_intra)
		{
			H264Dec_BS_Para_intraMB (img_ptr, mb_info_ptr, mb_cache_ptr);
		}else
		{	
			BS_and_Para_interMB_hor (img_ptr, mb_info_ptr, mb_cache_ptr);
			BS_and_Para_interMB_ver (img_ptr, mb_info_ptr, mb_cache_ptr);		
		}
#else
		uint32 *BS = (uint32 *)(mb_cache_ptr->BS);	
		BS[0] = BS[2] = 0x33334444;
		BS[2] = BS[3] = 0x33333333;
#endif
	}

	H264Dec_Config_DBK (img_ptr, mb_info_ptr, mb_cache_ptr);

	return;
}

PUBLIC void H264Dec_register_deblock_func(DEC_IMAGE_PARAMS_T *img_ptr)
{
	if (img_ptr->type == P_SLICE)
	{
		BS_and_Para_interMB_hor = BS_and_Para_interMB_hor_PSLICE;		
		BS_and_Para_interMB_ver = BS_and_Para_interMB_ver_PSLICE;
	}else
	{	
		BS_and_Para_interMB_hor = BS_and_Para_interMB_hor_BSLICE;		
		BS_and_Para_interMB_ver = BS_and_Para_interMB_ver_BSLICE;
	}

	return;
}


//for FMO mode
PUBLIC void H264Dec_deblock_one_frame (DEC_IMAGE_PARAMS_T *img_ptr)
{
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
