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
  ** 06/26/2012   Leon Li             Modify.                                                                                      *
 *****************************************************************************/
/*----------------------------------------------------------------------------*
**                        Dependencies                                        *
**---------------------------------------------------------------------------*/
#include "tiger_video_header.h"
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

#if 0
	int32 mb_x, mb_y;
	DEC_MB_CACHE_T *mb_cache_ptr = g_mb_cache_ptr;
	DEC_MB_INFO_T *curr_mb_info_ptr;
	uint32 cmd;

#if _CMODEL_
	uint8 *tmp_imgY = g_list0[0]->imgY;
	uint8 *tmp_imgU = g_list0[0]->imgU;
	g_list0[0]->imgY = g_dec_picture_ptr->imgY;
	g_list0[0]->imgU = g_dec_picture_ptr->imgU;
#endif
		
	H264Dec_VSPInit ();

	VSP_WRITE_REG(VSP_DCAM_REG_BASE+DCAM_INT_MASK_OFF, (1<<15)|(1<<10), "DCAM_INT_RAW: enable CMD DONE INT bit");

//	cmd = VSP_READ_REG(VSP_AHBM_REG_BASE+AHBM_ENDAIN_SEL_OFFSET, "Read out Endian Info") & 0xf;
	cmd = (img_ptr->width*img_ptr->height/4); //word unit
	VSP_WRITE_REG(VSP_AXIM_REG_BASE+AXIM_UV_OFFSET_OFF, cmd, "configure uv offset");
	VSP_WRITE_REG(VSP_DCAM_REG_BASE+DCAM_SRC_SIZE_OFF, ((img_ptr->height << 16) | img_ptr->width), "configure frame width");
open_vsp_iram();
	g_cmd_data_ptr = g_cmd_data_base;
	g_cmd_info_ptr = g_cmd_info_base;
#if _CMODEL_
	VSP_WRITE_REG(VSP_MEMO10_ADDR+20*4, ((uint32)CMD_CONTROL_INFO_ADDR)>>2, "config CMD control info buffer address");
	VSP_WRITE_REG(VSP_MEMO10_ADDR+21*4, ((uint32)CMD_CONTROL_DATA_ADDR)>>2, "config CMD control data buffer address");
#else
	VSP_WRITE_REG(VSP_MEMO10_ADDR+20*4,MAPaddr((uint32)g_cmd_info_base), "config CMD control info buffer address");
	VSP_WRITE_REG(VSP_MEMO10_ADDR+21*4, MAPaddr((uint32)g_cmd_data_base), "config CMD control data buffer address");
#endif		
	close_vsp_iram();

	//now, can enable cmd-exe
  //  cmd = VSP_READ_REG(VSP_DCAM_REG_BASE+DCAM_CFG_OFF, "DCAM_CFG: readout DCAM CFG");
//	cmd |= (1<<8);
//	VSP_WRITE_REG(VSP_DCAM_REG_BASE+DCAM_CFG_OFF, cmd, "DCAM_CFG: enable cmd-exe");

	/*init vsp command*/
	cmd = (1<<19)|((g_pps_ptr->entropy_coding_mode_flag & 0x01)<<18) |(1<<17) |(0<<16) |(0<<15) | (0<<14) |(1<<12) | (H264<<8) | (1<<7) | (1<<6) | (1<<3) | (1<<2) | (1<<1) | (1<<0);
	VSP_WRITE_REG_CQM(VSP_GLB_REG_BASE+GLB_CFG0_OFF, cmd, "GLB_CFG0: global config0");
	VSP_WRITE_REG_CQM(VSP_GLB_REG_BASE+VSP_TIME_OUT_VAL, TIME_OUT_CLK, "GLB_TIMEOUT_VALUE: timeout value");
			
	/*frame size, YUV format, max_X, max_Y*/
	cmd = (JPEG_FW_YUV420 << 24) | (img_ptr->frame_height_in_mbs << 12) | img_ptr->frame_width_in_mbs;
	VSP_WRITE_REG_CQM(VSP_GLB_REG_BASE+GLB_CFG1_OFF, cmd, "GLB_CFG1: configure max_y and max_X");			
	VSP_WRITE_CMD_INFO((VSP_GLB << MID_SHIFT_BIT) | (3<<24) | (GLB_CFG1_WOFF<<16) | (VSP_TIME_OUT_VAL_WOFF<<8) | (GLB_CFG0_WOFF));

	//config current rec frame as mca ref frame
	VSP_WRITE_REG_CQM(VSP_MEMO10_ADDR+ 0*4, MAPaddr(g_dec_picture_ptr->imgYAddr), "configure recontructed picture");
	VSP_WRITE_REG_CQM(VSP_MEMO10_ADDR+ 4*4, MAPaddr(g_dec_picture_ptr->imgYAddr), "configure reference picture");
	VSP_WRITE_CMD_INFO((VSP_RAM10 << MID_SHIFT_BIT) | (2<<24) | (4<<8)| 0);

	//clear fmo flag
	VSP_WRITE_REG_CQM(VSP_DBK_REG_BASE+DBK_CFG_OFF, 6, "configure dbk free run mode and enable filter");
	VSP_WRITE_REG_CQM(VSP_DBK_REG_BASE+DBK_VDB_BUF_ST_OFF, 1, "configure dbk ping-pang buffer0 enable");
	VSP_WRITE_REG_CQM(VSP_DBK_REG_BASE+DBK_CTR1_OFF, 1, "configure dbk control flag");
	VSP_WRITE_CMD_INFO((VSP_DBK << MID_SHIFT_BIT)| (3<<24) |(DBK_CTR1_WOFF<<16) | (DBK_VDB_BUF_ST_WOFF<<8) | (DBK_CFG_WOFF));

	for (mb_y = 0; mb_y < img_ptr->frame_height_in_mbs; mb_y++)
	{
		for (mb_x = 0; mb_x < img_ptr->frame_width_in_mbs; mb_x++)
		{
			//config mca using zero mv
			int32 ref_blk_end = TRUE;
			int32 ref_bir_blk = FALSE;
			int32 ref_cmd_type = 0;
			int32 ref_bw_frame_id = 0;
			int32 ref_fw_frame_id = 0;//use the first reference frame
			int32 ref_blk_id = 0;
			int32 ref_blk_size = MC_BLKSIZE_16x16;
			int32 mc_blk_info;

			if ((mb_x == 5) && (mb_y == 0) && (g_nFrame_dec_h264 == 1))
			{
				PRINTF("");
			}

			img_ptr->curr_mb_nr = mb_y*img_ptr->frame_width_in_mbs+mb_x;
			img_ptr->mb_x = mb_x;
			img_ptr->mb_y = mb_y;

			curr_mb_info_ptr = img_ptr->mb_info + img_ptr->curr_mb_nr;

			{
				int32 stride;
				int32 offset;

				stride = (img_ptr->frame_width_in_mbs<<2);
				offset = (img_ptr->mb_y<<2)*stride + (img_ptr->mb_x<<2);
				mb_cache_ptr->curr_mv_ptr[0] = g_dec_picture_ptr->mv[0] + offset*2;
				mb_cache_ptr->curr_mv_ptr[1] = g_dec_picture_ptr->mv[1] + offset*2;

				memcpy(&mb_cache_ptr->mv_cache[0][0], &mb_cache_ptr->curr_mv_ptr[0][(-stride-1)*2],  sizeof(int16)*(6*2));
				memcpy(&mb_cache_ptr->mv_cache[0][12], &mb_cache_ptr->curr_mv_ptr[0][-2],  sizeof(int16)*(6*2));
				memcpy(&mb_cache_ptr->mv_cache[0][24], &mb_cache_ptr->curr_mv_ptr[0][(-1+stride)*2],  sizeof(int16)*(6*2));
				memcpy(&mb_cache_ptr->mv_cache[0][36], &mb_cache_ptr->curr_mv_ptr[0][(-1+2*stride)*2],  sizeof(int16)*(6*2));
				memcpy(&mb_cache_ptr->mv_cache[0][48], &mb_cache_ptr->curr_mv_ptr[0][(-1+3*stride)*2],  sizeof(int16)*(6*2));

				memcpy(&mb_cache_ptr->mv_cache[1][0], &mb_cache_ptr->curr_mv_ptr[1][(-stride-1)*2],  sizeof(int16)*(6*2));
				memcpy(&mb_cache_ptr->mv_cache[1][12], &mb_cache_ptr->curr_mv_ptr[1][-2],  sizeof(int16)*(6*2));
				memcpy(&mb_cache_ptr->mv_cache[1][24], &mb_cache_ptr->curr_mv_ptr[1][(-1+stride)*2],  sizeof(int16)*(6*2));
				memcpy(&mb_cache_ptr->mv_cache[1][36], &mb_cache_ptr->curr_mv_ptr[1][(-1+2*stride)*2],  sizeof(int16)*(6*2));
				memcpy(&mb_cache_ptr->mv_cache[1][48], &mb_cache_ptr->curr_mv_ptr[1][(-1+3*stride)*2],  sizeof(int16)*(6*2));							
			}
						
			H264Dec_get_mb_avail(img_ptr, curr_mb_info_ptr, mb_cache_ptr);
			
			VSP_WRITE_REG_CQM (VSP_GLB_REG_BASE+GLB_CTRL0_OFF, (mb_y << 8) | (mb_x << 0), "vsp global reg: configure current MB position");
			VSP_WRITE_CMD_INFO((VSP_GLB << MID_SHIFT_BIT) | (1<<24) | (GLB_CTRL0_WOFF));
			
			mc_blk_info = (ref_blk_end << 27) | (ref_bir_blk << 26) | (ref_cmd_type << 24) | (ref_bw_frame_id << 20)|
				(ref_fw_frame_id << 16) | (ref_blk_id << 8) | (ref_blk_size & 0xff);
			VSP_WRITE_REG_CQM(VSP_MCA_REG_BASE+MCA_BLK_CBUF_OFF, mc_blk_info, "configure MCA command buffer, block info");
			VSP_WRITE_REG_CQM(VSP_MCA_REG_BASE+MCA_MV_CBUF_OFF, 0, "configure MCA command buffer, block mv info");
			VSP_WRITE_REG_CQM(VSP_MCA_REG_BASE+MCA_CFG_OFF, 1, "MCA_CFG: start MCA");
			VSP_WRITE_CMD_INFO((VSP_MCA << MID_SHIFT_BIT) | (3<<24) | (MCA_CFG_WOFF<<16)|(MCA_MV_CBUF_WOFF<<8)|(MCA_BLK_CBUF_WOFF));

		#if _CMODEL_
			mca_module();
		#endif

			//mbc
			VSP_WRITE_REG_CQM(VSP_MBC_REG_BASE+MBC_CMD0_OFF, (MBC_INTER_MB<<30) |	(0 & 0xffffff), "configure mb information, cbp = 0.");
			VSP_WRITE_CMD_INFO((VSP_MBC << MID_SHIFT_BIT) | (1<<24) |(MBC_CMD0_WOFF));
			
			//dbk
			H264Dec_BS_and_Para (img_ptr, curr_mb_info_ptr, mb_cache_ptr);

			H264Dec_mb_level_sync (img_ptr);
		}
	}

#if _CMODEL_
	g_list0[0]->imgY = tmp_imgY;
	g_list0[0]->imgU = tmp_imgU;
#endif

	VSP_READ_REG_POLL_CQM(VSP_DBK_REG_BASE+DBK_DEBUG_OFF, 16, 1, 1, "DBK_CTR1: polling dbk frame idle");
	VSP_WRITE_CMD_INFO((VSP_DBK << MID_SHIFT_BIT) | (1<<24) |((1<<7)|DBK_DEBUG_WOFF));

	VSP_READ_REG_POLL_CQM(VSP_GLB_REG_BASE+VSP_DBG_OFF, 6, 1, 1, "AHBM_STS: polling AHB idle status");
	VSP_WRITE_REG_CQM(VSP_GLB_REG_BASE+VSP_TST_OFF, g_nFrame_dec_h264, "VSP_TST: configure frame_cnt to debug register for end of picture");
	VSP_WRITE_CMD_INFO((VSP_GLB << MID_SHIFT_BIT) | (2<<24) | (VSP_TST_WOFF<<8)|((1<<7)|VSP_DBG_WOFF));
		
	VSP_WRITE_REG_CQM(VSP_GLB_REG_BASE+VSP_TST_OFF, 0x12345678, "VSP_TST: finished one frame");
	VSP_WRITE_CMD_INFO(0x12345678);
		
	//now, can enable cmd-exe
	cmd = VSP_READ_REG(VSP_DCAM_REG_BASE+DCAM_CFG_OFF, "DCAM_CFG: readout DCAM CFG");
	//cmd |= (1<<8)|(1<<9);
	cmd = cmd &(~0x10);
	cmd = cmd &(~0x200);
	cmd |= (1<<8);
	VSP_WRITE_REG(VSP_DCAM_REG_BASE+DCAM_CFG_OFF, cmd, "DCAM_CFG: enable cmd-exe");

#if	_CMODEL_
	VSP_READ_REG_POLL(VSP_DCAM_REG_BASE+DCAM_INT_STS_OFF, (1<<15), (1<<15), TIME_OUT_CLK, "DCAM_INT_STS_OFF: polling CMD DONE initerupt");
#else
	do{
	}while(!g_cmd_done_init);

	g_cmd_done_init = 0;		
#endif	
#endif //#if 0
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
