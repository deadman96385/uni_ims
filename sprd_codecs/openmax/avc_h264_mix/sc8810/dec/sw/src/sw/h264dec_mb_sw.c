/******************************************************************************
 ** File Name:    h264dec_mb.c			                                      *
 ** Author:       Xiaowei.Luo                                                 *
 ** DATE:         03/29/2010                                                  *
 ** Copyright:    2010 Spreatrum, Incoporated. All Rights Reserved.           *
 ** Description:                                                              *
 *****************************************************************************/
/******************************************************************************
 **                   Edit    History                                         *
 **---------------------------------------------------------------------------* 
 ** DATE          NAME            DESCRIPTION                                 * 
 ** 03/29/2010    Xiaowei.Luo     Create.                                     *
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

/**----------------------------------------------------------------------------*
**                           Function Prototype                               **
**----------------------------------------------------------------------------*/
#ifdef WIN32
void put_mb2Frame (uint8 *mb_pred, uint8 *mb_addr[3], int32 pitch)
{
	int32 uv;
	int32 i;
	uint8 * pFrame;
	int32 * pIntMB;
	
	/*put Y*/
	pFrame = mb_addr[0];
	pIntMB = (int32 *)mb_pred;
	
	for (i = 0; i < MB_SIZE; i++)
	{
		int32 * pIntFrame = (int32 *)pFrame;
		
		*pIntFrame++ = *pIntMB++;
		*pIntFrame++ = *pIntMB++;
		*pIntFrame++ = *pIntMB++;
		*pIntFrame++ = *pIntMB++;		
		
		pFrame += pitch;
	}
	
	pitch >>= 1;
	for (uv = 0; uv < 2; uv++)
	{
		pFrame = mb_addr[uv+1];
		for (i = 0; i < 8; i++)
		{
			int32 * pIntFrame = (int32 *)pFrame;
			*pIntFrame++ = *pIntMB++;
			*pIntFrame++ = *pIntMB++;
			pFrame += pitch;
		}
	}
}
#endif

LOCAL void H264Dec_read_and_derive_ipred_modes_sw(DEC_IMAGE_PARAMS_T *img_ptr, DEC_MB_INFO_T *mb_info_ptr, DEC_MB_CACHE_T *mb_cache_ptr)
{
	//get 16 block4x4 intra prediction mode, luma
	if (mb_info_ptr->mb_type == I4MB_264)
	{
		int32 blk4x4StrIdx, blk4x4Idx, blk8x8Idx;
		int32 pred_mode;
		int32 up_ipred_mode;
		int32 left_ipred_mode;
		int32 most_probable_ipred_mode;
		int8 *i4_pred_mode_ref_ptr = mb_cache_ptr->i4x4pred_mode_cache;
		const uint8 *blk_order_map_tbl_ptr = g_blk_order_map_tbl;
		DEC_BS_T *bitstrm = img_ptr->bitstrm_ptr;
		int32 val;

		int32 i, j;

		//First decode transform_size_8x8_flag. 
		if (g_active_pps_ptr->transform_8x8_mode_flag)
		{
			if (img_ptr->is_cabac)
			{
				mb_info_ptr->transform_size_8x8_flag = decode_cabac_mb_transform_size(img_ptr, mb_info_ptr, mb_cache_ptr);
			}else
			{
				mb_info_ptr->transform_size_8x8_flag = READ_BITS1(bitstrm);
			}
		}

		if (!mb_info_ptr->transform_size_8x8_flag)
		{
			//Intra 4x4
			for (blk4x4Idx = 0, i = 0; i < 8; i++)
			{	
				for (j = 1; j >= 0; j--)
				{
					if (!img_ptr->is_cabac)
					{
						val = BITSTREAMSHOWBITS(bitstrm, 4);
						if(val >= 8)	//b'1xxx, use_most_probable_mode_flag = 1!, noted by xwluo@20100618
						{
							pred_mode = -1;
							READ_BITS1(bitstrm);
						}else
						{
							pred_mode = val & 0x7; //remaining_mode_selector
							READ_FLC(bitstrm, 4);
						}
					}else
					{
						pred_mode = decode_cabac_mb_intra4x4_pred_mode (img_ptr);
					}
					
					blk4x4StrIdx = blk_order_map_tbl_ptr[blk4x4Idx];
					left_ipred_mode = i4_pred_mode_ref_ptr[blk4x4StrIdx-1];
					up_ipred_mode = i4_pred_mode_ref_ptr[blk4x4StrIdx-CTX_CACHE_WIDTH];
					most_probable_ipred_mode = mmin(left_ipred_mode, up_ipred_mode);

					if (most_probable_ipred_mode < 0)
					{
						most_probable_ipred_mode = DC_PRED;
					}

					pred_mode = ((pred_mode == -1) ? most_probable_ipred_mode : (pred_mode + (pred_mode >= most_probable_ipred_mode)));
						
					i4_pred_mode_ref_ptr[blk4x4StrIdx] = pred_mode;
					blk4x4Idx++;
				}
			}	
		}else
		{
			//Intra 8x8
			for (blk8x8Idx = 0; blk8x8Idx < 4; blk8x8Idx++)
			{
				if (!img_ptr->is_cabac)
				{
					val = BITSTREAMSHOWBITS (bitstrm, 4);
					if (val >= 8)	//b'1xxx, use_most_probable_mode_flag = 1!
					{
						pred_mode = -1;
						READ_BITS1(bitstrm);
					}else
					{
						pred_mode = val & 0x7;	//remaining_mode_selector
						READ_FLC(bitstrm, 4);
					}
				}else
				{
					pred_mode = decode_cabac_mb_intra4x4_pred_mode (img_ptr);
				}

				blk4x4Idx = (blk8x8Idx << 2);
				blk4x4StrIdx = blk_order_map_tbl_ptr[blk4x4Idx];
				left_ipred_mode = i4_pred_mode_ref_ptr[blk4x4StrIdx-1];
				up_ipred_mode = i4_pred_mode_ref_ptr[blk4x4StrIdx - CTX_CACHE_WIDTH];
				most_probable_ipred_mode = mmin(left_ipred_mode, up_ipred_mode);

				if (most_probable_ipred_mode < 0)
				{
					most_probable_ipred_mode = DC_PRED;
				}
				pred_mode = ((pred_mode == -1) ? most_probable_ipred_mode : (pred_mode + (pred_mode >= most_probable_ipred_mode)));

				i4_pred_mode_ref_ptr[blk4x4StrIdx] = 
				i4_pred_mode_ref_ptr[blk4x4StrIdx + 1] = 	
				i4_pred_mode_ref_ptr[blk4x4StrIdx + CTX_CACHE_WIDTH] = 
				i4_pred_mode_ref_ptr[blk4x4StrIdx + CTX_CACHE_WIDTH + 1] = pred_mode;
			}
		}
	}else
	{
		int8 *i4x4_pred_mode_ref_ptr = mb_cache_ptr->i4x4pred_mode_cache;

		((int32*)i4x4_pred_mode_ref_ptr)[4] = 
		((int32*)i4x4_pred_mode_ref_ptr)[7] = 
		((int32*)i4x4_pred_mode_ref_ptr)[10] = 
		((int32*)i4x4_pred_mode_ref_ptr)[13] = 0x02020202;	
	}

	if (!img_ptr->is_cabac)
	{
		DEC_BS_T *bitstrm = img_ptr->bitstrm_ptr;

		//get chroma intra prediction mode
		mb_info_ptr->c_ipred_mode = READ_UE_V(bitstrm);
	}else
	{
		mb_info_ptr->c_ipred_mode = decode_cabac_mb_chroma_pre_mode(img_ptr, mb_info_ptr, mb_cache_ptr);
	}

	return;
}

/************************************************************************/
/* decode I_PCM data                                                    */
/************************************************************************/
LOCAL void H264Dec_decode_IPCM_MB_sw(DEC_IMAGE_PARAMS_T *img_ptr, DEC_MB_INFO_T *mb_info_ptr, DEC_MB_CACHE_T *mb_cache_ptr, DEC_BS_T * bitstrm)
{
	int32 value;
	int32 i,j;
	int32 bit_offset;
	int32 comp;
	int32 *i4x4pred_mode_cache = (int32 *)(mb_cache_ptr->i4x4pred_mode_cache);
		
	i4x4pred_mode_cache[4] = i4x4pred_mode_cache[7] =
	i4x4pred_mode_cache[10] = i4x4pred_mode_cache[13] = 0x02020202;	

	bit_offset  = (bitstrm->bitsLeft & 0x7);
	if(bit_offset)
	{
		READ_FLC(bitstrm, bit_offset);
	}

	if (img_ptr->cabac.low & 0x1)
	{
		REVERSE_BITS(bitstrm, 8);
	}

	if (img_ptr->cabac.low & 0x1ff)
	{
		REVERSE_BITS(bitstrm, 8);
	}

	//Y, U, V
	for (comp = 0; comp < 3; comp++)
	{
		int32 blk_size = (comp == 0) ? MB_SIZE : MB_CHROMA_SIZE;
		int32 pitch = (comp == 0) ? img_ptr->ext_width : (img_ptr->ext_width>>1);
		uint8 *I_PCM_value = mb_cache_ptr->mb_addr[comp];

		for (j = 0; j < blk_size; j++)
		{
			for (i = 0; i < blk_size; i+=4)
			{
				value = READ_FLC(bitstrm, 32);

				I_PCM_value[i] 	 = (value>>24) & 0xff;
				I_PCM_value[i+1] = (value>>16) & 0xff;
				I_PCM_value[i+2] = (value>>8) & 0xff;
				I_PCM_value[i+3] = (value>>0) & 0xff;
			}
			I_PCM_value += pitch;	
		}
	}

	H264Dec_set_IPCM_nnz(mb_cache_ptr);

	mb_info_ptr->qp = 0;
	mb_info_ptr->cbp = 0x3f;
	mb_info_ptr->skip_flag = 1;
        
	//for CABAC decoding of Dquant
	if (g_active_pps_ptr->entropy_coding_mode_flag)
	{
		last_dquant = 0;
		mb_info_ptr->dc_coded_flag = 7;
		ff_init_cabac_decoder(img_ptr);	//arideco_start_decoding(img_ptr);
	}
}

LOCAL void FilterPixel_I8x8 (DEC_IMAGE_PARAMS_T *img_ptr, DEC_MB_INFO_T *mb_info_ptr,DEC_MB_CACHE_T *mb_cache_ptr, int blkidx_8x8, uint8 *pRec, int32 pitch)
{
	int32 i;
	uint8 * ptop;
	uint8 * pLeft;
	uint8 * pDst;
	uint8  topleftpix;
	uint8  toppix[16];
	uint8  leftpix[8];
	int8    availA ;
	int8    availB ; 
	int8    availC ;
	int8    availD ;	


	if(blkidx_8x8 == 0)
	{
		availA = mb_cache_ptr->mb_avail_a;
		availB = mb_cache_ptr->mb_avail_b; 
		availD = mb_cache_ptr->mb_avail_d;	
		if(img_ptr->constrained_intra_pred_flag)
		{
			availA &= (mb_info_ptr -1)->is_intra;
			availB &= (img_ptr->abv_mb_info)->is_intra;
			availD &= (img_ptr->abv_mb_info -1)->is_intra;
		}
		availC = availB;
	}
	else if(blkidx_8x8 == 1)
	{
		availB = mb_cache_ptr->mb_avail_b; 
		availC = mb_cache_ptr->mb_avail_c;
		if(img_ptr->constrained_intra_pred_flag)
		{
			availB &= (img_ptr->abv_mb_info)->is_intra;
			availC &= (img_ptr->abv_mb_info +1)->is_intra;
		}	
		availA = 1;
		availD = availB;		
	}
	else if(blkidx_8x8 == 2)
	{
		availA = mb_cache_ptr->mb_avail_a;
		if(img_ptr->constrained_intra_pred_flag)
		{
			availA &= (mb_info_ptr -1)->is_intra;
		}	
		availD = availA;
		availB = 1;
		availC = 1;
	}
	else
	{
		availA = 1;
		availB = 1;
		availC = 0;
		availD = 1;
	}

	ptop = pRec - pitch;
	topleftpix = ptop[-1];
	pLeft = pRec -1;

	//store availx to mb_cache which can be used directly by intra prediction process.
	mb_cache_ptr->mb_avail_a_8x8_ipred = availA;
	mb_cache_ptr->mb_avail_b_8x8_ipred = availB;
	mb_cache_ptr->mb_avail_c_8x8_ipred = availC;
	mb_cache_ptr->mb_avail_d_8x8_ipred = availD;

	//load pixel vaule to array toppix and leftpix.
	
	((uint32 *)toppix)[0] = ((uint32 *)ptop)[0];
	((uint32 *)toppix)[1] = ((uint32 *)ptop)[1];
	if(availC)
	{
		((uint32 *)toppix)[2] = ((uint32 *)ptop)[2];
		((uint32 *)toppix)[3] = ((uint32 *)ptop)[3];
	}
	else
	{
		uint32 tmp_pix = ptop[7] * 0x01010101;
		((uint32 *)toppix)[2]  = ((uint32 *)toppix)[3] = tmp_pix;
	}

	for(i= 0; i<8; i++)
	{
		leftpix[i] = pLeft[0];
		pLeft += pitch;
	}
		
	/*filter top border pixel*/
	
	pDst = mb_cache_ptr->topFilteredPix;
	
	//p[-1,-1]
	if(!availA && availB)
		 pDst[0] = (3*topleftpix + toppix[0] +2) >>2;
	else if(availA && !availB)
		 pDst[0] = (3*topleftpix+ leftpix[0] +2) >>2;
	else if (!availA &&  !availB)
		pDst[0] =topleftpix;
	else
		pDst[0] = (leftpix[0] + 2*topleftpix + toppix[0] +2) >>2;

	//p[0,-1]
	if(availD)
		pDst[1] = (topleftpix + 2*toppix[0] + toppix[1] +2) >>2;
	else
		pDst[1] = (3*toppix[0] +  toppix[1]+2) >>2;	
	
	//p[x,-1] x=1...14
	for(i = 1; i <15; i++)
		pDst[i+1] =  (toppix[i-1] + 2*toppix[i] + toppix[i+1] +2)>>2;		

	//p[15, -1]
	pDst[16] =  (toppix[14] + 3* toppix[15]+2) >>2;


	/*filter left border pixel*/
	// leftPredPix_Y[8..15] to store left filtered pixel. leftPredPix_Y[7] to duplicate topleft pixel
	pDst = mb_cache_ptr->leftPredPix_Y + 8;

	//p[-1,-1]
	pDst[-1] = mb_cache_ptr->topFilteredPix[0];
	
	//p[-1,0]
	if(availD)
		pDst[0] = (topleftpix + 2*leftpix[0] + leftpix[1] +2) >>2;
	else
		pDst[0] =  (3*leftpix[0] +  leftpix[1]+2) >>2;

	//p[-1,y] y=1...6
	for(i=1;i<7;i++)
		pDst[i] =  (leftpix[i-1] + 2*leftpix[i] + leftpix[i+1] +2)>>2;	

	//p[-1,7]
	pDst[7] = ( leftpix[6] + 3*leftpix[7] +2) >>2;	

}

/*blk4x4Nr order
___ ___ ___ ___
| 0 | 1 | 4 | 5 |
|___|___|___|___|
| 2 | 3 | 6 | 7 |
|___|___|___|___|
| 8 | 9 | 12| 13|
|___|___|___|___|
| 10| 11| 14| 15|
|___|___|___|___| 

*/

LOCAL void H264Dec_decode_intra_mb(DEC_IMAGE_PARAMS_T *img_ptr, DEC_MB_INFO_T *mb_info_ptr, DEC_MB_CACHE_T *mb_cache_ptr)
{
	int32 uv;
	int32 blkStrOdr;
	int32 blk4x4Nr, blk8x8Nr;
	int32 blkScanIdx;
	int32 mb_type = mb_info_ptr->mb_type;
	int16 *coff;
	uint8 *pred, *rec;
	uint8 *mb_pred, *mb_rec;
	const uint8 * pMbCacheAddr = g_mbcache_addr_map_tbl;
	int32 MbPos_x = img_ptr->mb_x;
	int32 MbPos_y = img_ptr->mb_y;
	const uint8 * pBlkOrderMap = g_blk_order_map_tbl;
	const uint8 * pDecToScnOdrMap = g_dec_order_to_scan_order_map_tbl;
	int32 ext_width = img_ptr->ext_width;
	int32 uvCBPMsk, uvCBP;

	coff = mb_cache_ptr->coff_Y;
	mb_pred = mb_cache_ptr->pred_cache[0].pred_Y;
	mb_rec = mb_cache_ptr->mb_addr[0];

	if (mb_type == I16MB)
	{
		int32 i16mode = mb_cache_ptr->i16mode;
#if defined(CTS_PROTECT)
		i16mode = IClip(0, 3, i16mode);
#endif

		g_intraPred16x16[i16mode](img_ptr, mb_info_ptr,mb_cache_ptr, mb_pred, mb_rec, ext_width);

		for (blk4x4Nr = 0; blk4x4Nr < 16; blk4x4Nr++)
		{
//			if (blk4x4Nr == 4)
//				PRINTF (" ");
			pred = mb_pred + pMbCacheAddr [blk4x4Nr];
			rec = mb_rec + mb_cache_ptr->blk4x4_offset[blk4x4Nr];
			itrans_4x4 (coff, pred, MB_SIZE, rec, ext_width);
			coff += 16;
		}
	}
	else if (!mb_info_ptr->transform_size_8x8_flag)	//intra 4x4
	{
		int8 * pI4PredMode = mb_cache_ptr->i4x4pred_mode_cache;
		int8 *pNnzRef = mb_cache_ptr->nnz_cache;
		Intra4x4Pred * intraPred4x4 = g_intraPred4x4;
		int32 intraPredMode;
		
		for (blk4x4Nr = 0; blk4x4Nr < 16; blk4x4Nr++)
		{
			blkStrOdr = pBlkOrderMap[blk4x4Nr];
			intraPredMode = pI4PredMode [blkStrOdr];
			
			rec = mb_rec + mb_cache_ptr->blk4x4_offset[blk4x4Nr];
			pred = mb_pred + pMbCacheAddr [blk4x4Nr];
			blkScanIdx = pDecToScnOdrMap [blk4x4Nr];

#if defined(CTS_PROTECT)
			intraPredMode = IClip(0, 8, intraPredMode);
#endif
			intraPred4x4[intraPredMode] (img_ptr, mb_info_ptr, mb_cache_ptr, pred, blkScanIdx, rec, ext_width);

			if (pNnzRef[blkStrOdr])
			{
				itrans_4x4 (coff, pred, MB_SIZE, rec, ext_width);
			}else
			{
				((uint32 *)rec)[0] = ((uint32 *)pred)[0];	rec += ext_width; pred += MB_SIZE;
				((uint32 *)rec)[0] = ((uint32 *)pred)[0];	rec += ext_width; pred += MB_SIZE;
				((uint32 *)rec)[0] = ((uint32 *)pred)[0];	rec += ext_width; pred += MB_SIZE;
				((uint32 *)rec)[0] = ((uint32 *)pred)[0];	//rec += ext_width; pred += MB_SIZE;
			}
			
			coff += 16;	
		
		}	
	}else	//Intra 8x8
	{
		int8 * pI4PredMode = mb_cache_ptr->i4x4pred_mode_cache;
		int8 *pNnzRef = mb_cache_ptr->nnz_cache;
		int32 intraPredMode;

		for (blk8x8Nr = 0; blk8x8Nr < 4; blk8x8Nr++)
		{
			blk4x4Nr = blk8x8Nr << 2;
			blkStrOdr = pBlkOrderMap[blk4x4Nr];
			intraPredMode = pI4PredMode[blkStrOdr];
			
			pred = mb_pred + pMbCacheAddr[blk4x4Nr];
			rec = mb_rec + mb_cache_ptr->blk4x4_offset[blk4x4Nr];
			FilterPixel_I8x8 (img_ptr, mb_info_ptr, mb_cache_ptr, blk8x8Nr, rec, ext_width);

			//intraPred8x8 function
			g_intraPred8x8[intraPredMode] (mb_cache_ptr, pred, blk8x8Nr);

			if (pNnzRef[blkStrOdr]||pNnzRef[blkStrOdr+1]||pNnzRef[blkStrOdr+12]||pNnzRef[blkStrOdr+13])
			{
				itrans_8x8 (coff, pred, MB_SIZE, rec, ext_width);
			}else
			{
				//copy 8x8
#ifdef WIN32
				((uint32 *)rec)[0] = ((uint32 *)pred)[0];	((uint32 *)rec)[1] = ((uint32 *)pred)[1];	rec += ext_width; pred += MB_SIZE;
				((uint32 *)rec)[0] = ((uint32 *)pred)[0];	((uint32 *)rec)[1] = ((uint32 *)pred)[1];	rec += ext_width; pred += MB_SIZE;
				((uint32 *)rec)[0] = ((uint32 *)pred)[0];	((uint32 *)rec)[1] = ((uint32 *)pred)[1];	rec += ext_width; pred += MB_SIZE;
				((uint32 *)rec)[0] = ((uint32 *)pred)[0];	((uint32 *)rec)[1] = ((uint32 *)pred)[1];	rec += ext_width; pred += MB_SIZE;
				((uint32 *)rec)[0] = ((uint32 *)pred)[0];	((uint32 *)rec)[1] = ((uint32 *)pred)[1];	rec += ext_width; pred += MB_SIZE;
				((uint32 *)rec)[0] = ((uint32 *)pred)[0];	((uint32 *)rec)[1] = ((uint32 *)pred)[1];	rec += ext_width; pred += MB_SIZE;
				((uint32 *)rec)[0] = ((uint32 *)pred)[0];	((uint32 *)rec)[1] = ((uint32 *)pred)[1];	rec += ext_width; pred += MB_SIZE;
				((uint32 *)rec)[0] = ((uint32 *)pred)[0];	((uint32 *)rec)[1] = ((uint32 *)pred)[1];	//rec += ext_width; pred += MB_SIZE;
#else
				copyPredblk_8x8 (pred, rec, ext_width);
#endif
			}

			coff += 64;
		}
	}	

	uvCBPMsk = 1;
	uvCBP = mb_cache_ptr->cbp_uv;
//	pMbCacheAddr = g_MBUVCacheAddrMap;
	ext_width >>= 1;
		
	for (uv = 0; uv < 2; uv++)
	{	
		int predMode = mb_info_ptr->c_ipred_mode;	

#if defined(CTS_PROTECT)
		predMode = IClip(0, 3, predMode);
#endif
		coff = mb_cache_ptr->coff_UV [uv];
		mb_pred = mb_cache_ptr->pred_cache[0].pred_UV [uv]; 
		mb_rec = mb_cache_ptr->mb_addr[1+uv];
		
//		ptopLeftPix = mb_cache_ptr->mb_addr[uv+1] - ext_width - 1;
		g_intraChromaPred[predMode] (img_ptr, mb_info_ptr, mb_cache_ptr,mb_pred, mb_rec, ext_width);	

		for (blk4x4Nr = 16; blk4x4Nr < 20; blk4x4Nr++)
		{
			pred = mb_pred + pMbCacheAddr [blk4x4Nr];
			rec = mb_rec + mb_cache_ptr->blk4x4_offset[blk4x4Nr];
			if (uvCBP & uvCBPMsk)
			{
				itrans_4x4 (coff, pred, MB_CHROMA_SIZE, rec, ext_width);
			}else
			{
				((uint32 *)rec)[0] = ((uint32 *)pred)[0];	rec += ext_width; pred += MB_CHROMA_SIZE;
				((uint32 *)rec)[0] = ((uint32 *)pred)[0];	rec += ext_width; pred += MB_CHROMA_SIZE;
				((uint32 *)rec)[0] = ((uint32 *)pred)[0];	rec += ext_width; pred += MB_CHROMA_SIZE;
				((uint32 *)rec)[0] = ((uint32 *)pred)[0];	//rec += ext_width; pred += MB_CHROMA_SIZE;
			}

			uvCBPMsk = uvCBPMsk << 1;			
			coff += 16;
		}
	}
}

LOCAL void H264Dec_decode_inter_mb (DEC_IMAGE_PARAMS_T *img_ptr, DEC_MB_INFO_T *mb_info_ptr, DEC_MB_CACHE_T *mb_cache_ptr)
{
	int32 uv;
	int32 blk4x4Nr, blk8x8Nr;
	int32 blkStrIdx;
	int16 * coff;
	uint8 * mb_pred, * mb_rec;
	uint8 * pred, *rec;
	const uint8 * pBlkOdrMap = g_blk_order_map_tbl;
	const uint8 * pMbCacheAddr = g_mbcache_addr_map_tbl;
	int32 ext_width = img_ptr->ext_width;
	int uvCBPMsk, uvCBP;

	/*luminance*/
	coff = mb_cache_ptr->coff_Y;
	mb_pred = mb_cache_ptr->pred_cache[0].pred_Y;
	mb_rec = mb_cache_ptr->mb_addr[0];

	if (!mb_info_ptr->transform_size_8x8_flag)
	{
		// 4x4 idct
		for (blk4x4Nr = 0; blk4x4Nr < 16; blk4x4Nr++)
		{
			blkStrIdx = pBlkOdrMap [blk4x4Nr];

			pred = mb_pred + pMbCacheAddr [blk4x4Nr];
			rec = mb_rec + mb_cache_ptr->blk4x4_offset[blk4x4Nr];
			if (mb_cache_ptr->nnz_cache [blkStrIdx])
			{
				itrans_4x4 (coff, pred, MB_SIZE, rec, ext_width);
			}else
			{
				((uint32 *)rec)[0] = ((uint32 *)pred)[0];	rec += ext_width; pred += MB_SIZE;
				((uint32 *)rec)[0] = ((uint32 *)pred)[0];	rec += ext_width; pred += MB_SIZE;
				((uint32 *)rec)[0] = ((uint32 *)pred)[0];	rec += ext_width; pred += MB_SIZE;
				((uint32 *)rec)[0] = ((uint32 *)pred)[0];	//rec += ext_width; pred += MB_SIZE;
			}

			coff += 16;		
		}
	}else
	{
		int8 *pNnzRef = mb_cache_ptr->nnz_cache;
		//8x8 idct
		for (blk8x8Nr = 0; blk8x8Nr < 4; blk8x8Nr++)
		{
			blk4x4Nr = blk8x8Nr << 2;
			blkStrIdx = pBlkOdrMap[blk4x4Nr];

			pred = mb_pred+ pMbCacheAddr[blk4x4Nr];
			rec = mb_rec + mb_cache_ptr->blk4x4_offset[blk4x4Nr];

			if (pNnzRef[blkStrIdx]||pNnzRef[blkStrIdx+1]||pNnzRef[blkStrIdx+12]||pNnzRef[blkStrIdx+13])
			{
				itrans_8x8 (coff, pred, MB_SIZE, rec, ext_width);
			}else
			{
				//copy 8x8
			#ifdef WIN32	
				((uint32 *)rec)[0] = ((uint32 *)pred)[0];	((uint32 *)rec)[1] = ((uint32 *)pred)[1];	rec += ext_width; pred += MB_SIZE;
				((uint32 *)rec)[0] = ((uint32 *)pred)[0];	((uint32 *)rec)[1] = ((uint32 *)pred)[1];	rec += ext_width; pred += MB_SIZE;
				((uint32 *)rec)[0] = ((uint32 *)pred)[0];	((uint32 *)rec)[1] = ((uint32 *)pred)[1];	rec += ext_width; pred += MB_SIZE;
				((uint32 *)rec)[0] = ((uint32 *)pred)[0];	((uint32 *)rec)[1] = ((uint32 *)pred)[1];	rec += ext_width; pred += MB_SIZE;
				((uint32 *)rec)[0] = ((uint32 *)pred)[0];	((uint32 *)rec)[1] = ((uint32 *)pred)[1];	rec += ext_width; pred += MB_SIZE;
				((uint32 *)rec)[0] = ((uint32 *)pred)[0];	((uint32 *)rec)[1] = ((uint32 *)pred)[1];	rec += ext_width; pred += MB_SIZE;
				((uint32 *)rec)[0] = ((uint32 *)pred)[0];	((uint32 *)rec)[1] = ((uint32 *)pred)[1];	rec += ext_width; pred += MB_SIZE;
				((uint32 *)rec)[0] = ((uint32 *)pred)[0];	((uint32 *)rec)[1] = ((uint32 *)pred)[1];	//rec += ext_width; pred += MB_SIZE;
			#else
				copyPredblk_8x8 (pred, rec, ext_width);
			#endif
			}

			coff += 64;
		}
	}

	/*chroma*/
//	mb_cache_ptrAddr = g_MBUVCacheAddrMap;
	ext_width >>= 1;
	uvCBP = mb_cache_ptr->cbp_uv;
	uvCBPMsk = 1;
	for (uv = 0; uv < 2; uv++)
	{
		coff = mb_cache_ptr->coff_UV[uv];
		mb_pred = mb_cache_ptr->pred_cache[0].pred_UV [uv];
		mb_rec = mb_cache_ptr->mb_addr[1+uv];
		
		for (blk4x4Nr = 16; blk4x4Nr < 20; blk4x4Nr++)
		{			
			pred = mb_pred + pMbCacheAddr [blk4x4Nr];
			rec = mb_rec + mb_cache_ptr->blk4x4_offset[blk4x4Nr];
			if (uvCBP & uvCBPMsk)
			{
				itrans_4x4 (coff, pred, MB_CHROMA_SIZE, rec, ext_width);
			}else
			{
				((uint32 *)rec)[0] = ((uint32 *)pred)[0];	rec += ext_width; pred += MB_CHROMA_SIZE;
				((uint32 *)rec)[0] = ((uint32 *)pred)[0];	rec += ext_width; pred += MB_CHROMA_SIZE;
				((uint32 *)rec)[0] = ((uint32 *)pred)[0];	rec += ext_width; pred += MB_CHROMA_SIZE;
				((uint32 *)rec)[0] = ((uint32 *)pred)[0];	//rec += ext_width; pred += MB_CHROMA_SIZE;
			}

			uvCBPMsk = uvCBPMsk << 1;	
			coff += 16;
		}
	}
}

LOCAL void H264Dec_read_intraMB_context_sw (DEC_IMAGE_PARAMS_T *img_ptr, DEC_MB_INFO_T *mb_info_ptr, DEC_MB_CACHE_T *mb_cache_ptr)
{
	H264Dec_fill_intraMB_context (img_ptr, mb_info_ptr, mb_cache_ptr);
	
	if (mb_info_ptr->mb_type != IPCM)
	{
		H264Dec_read_and_derive_ipred_modes_sw (img_ptr, mb_info_ptr, mb_cache_ptr);
		H264Dec_read_CBPandDeltaQp (img_ptr, mb_info_ptr, mb_cache_ptr);
		sw_vld_mb(mb_info_ptr, mb_cache_ptr);
		H264Dec_decode_intra_mb (img_ptr, mb_info_ptr, mb_cache_ptr);
	}else
	{
		H264Dec_decode_IPCM_MB_sw(img_ptr, mb_info_ptr, mb_cache_ptr, img_ptr->bitstrm_ptr);		
	}
}

PUBLIC void H264Dec_read_one_macroblock_ISlice_sw (DEC_IMAGE_PARAMS_T *img_ptr, DEC_MB_INFO_T *mb_info_ptr, DEC_MB_CACHE_T *mb_cache_ptr)
{
	mb_info_ptr->is_intra = TRUE;
	mb_info_ptr->mb_type = read_mb_type (img_ptr, mb_info_ptr, mb_cache_ptr);

	H264Dec_interpret_mb_mode_I (mb_info_ptr, mb_cache_ptr);	
	H264Dec_read_intraMB_context_sw(img_ptr, mb_info_ptr, mb_cache_ptr);

	return;
}

PUBLIC void H264Dec_read_one_macroblock_PSlice_sw (DEC_IMAGE_PARAMS_T *img_ptr, DEC_MB_INFO_T *mb_info_ptr, DEC_MB_CACHE_T *mb_cache_ptr)
{
	mb_info_ptr->mb_type = read_mb_type (img_ptr, mb_info_ptr, mb_cache_ptr);
	H264Dec_interpret_mb_mode_P(img_ptr, mb_info_ptr, mb_cache_ptr);

	if (mb_info_ptr->is_intra)
	{
		H264Dec_read_intraMB_context_sw(img_ptr, mb_info_ptr, mb_cache_ptr);
	}else
	{
		mb_cache_ptr->read_ref_id_flag[0] =( (img_ptr->ref_count[0] > 1) && (!mb_cache_ptr->all_zero_ref));
		mb_cache_ptr->read_ref_id_flag[1] = ((img_ptr->ref_count[1] > 1) && (!mb_cache_ptr->all_zero_ref));

		fill_cache (img_ptr, mb_info_ptr, mb_cache_ptr);

		if (!mb_cache_ptr->is_skipped)
		{
			H264Dec_read_motionAndRefId (img_ptr, mb_info_ptr, mb_cache_ptr);
			H264Dec_read_CBPandDeltaQp (img_ptr, mb_info_ptr, mb_cache_ptr);
			sw_vld_mb (mb_info_ptr, mb_cache_ptr);
		}
		
		H264Dec_mv_prediction_sw (img_ptr, mb_info_ptr, mb_cache_ptr);
		
		if (!mb_cache_ptr->is_skipped)
		{
			H264Dec_decode_inter_mb (img_ptr, mb_info_ptr, mb_cache_ptr);
		}else
		{
			put_mb2Frame (mb_cache_ptr->pred_cache[0].pred_Y, mb_cache_ptr->mb_addr, img_ptr->ext_width);
		}
	}
	
	return;
}

PUBLIC void H264Dec_read_one_macroblock_BSlice_sw (DEC_IMAGE_PARAMS_T *img_ptr, DEC_MB_INFO_T *mb_info_ptr, DEC_MB_CACHE_T *mb_cache_ptr)
{
	mb_info_ptr->mb_type = read_mb_type (img_ptr, mb_info_ptr, mb_cache_ptr);
	H264Dec_interpret_mb_mode_B(img_ptr, mb_info_ptr, mb_cache_ptr);

	if (mb_info_ptr->is_intra)
	{
		H264Dec_read_intraMB_context_sw(img_ptr, mb_info_ptr, mb_cache_ptr);
	}else
	{
#if _H264_PROTECT_ & _LEVEL_LOW_
		if (g_list[0][0] == NULL)
		{
			img_ptr->error_flag |= ER_REF_FRM_ID;
			img_ptr->return_pos1 |= (1<<1);
			return;
		}
#endif		

		mb_cache_ptr->read_ref_id_flag[0] = (img_ptr->ref_count[0] > 1);
		mb_cache_ptr->read_ref_id_flag[1] = (img_ptr->ref_count[1] > 1);

		fill_cache (img_ptr, mb_info_ptr, mb_cache_ptr);
		
		if (!mb_cache_ptr->is_skipped)
		{
#if 0
			if (IS_INTERMV(mb_info_ptr))
#else
			if (mb_info_ptr->mb_type != 0)
#endif
			{
				H264Dec_read_motionAndRefId (img_ptr, mb_info_ptr, mb_cache_ptr);
			}
			H264Dec_read_CBPandDeltaQp (img_ptr, mb_info_ptr, mb_cache_ptr);
			sw_vld_mb(mb_info_ptr, mb_cache_ptr);
		}
		
		if ( mb_cache_ptr->is_direct)
		{
			direct_mv (img_ptr, mb_info_ptr, mb_cache_ptr);
		}
		
		H264Dec_mv_prediction_sw (img_ptr, mb_info_ptr, mb_cache_ptr);
		
		if (!mb_cache_ptr->is_skipped)
		{
			H264Dec_decode_inter_mb (img_ptr, mb_info_ptr, mb_cache_ptr);
		}else
		{
			put_mb2Frame (mb_cache_ptr->pred_cache[0].pred_Y, mb_cache_ptr->mb_addr, img_ptr->ext_width);
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
