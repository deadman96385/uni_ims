/******************************************************************************
 ** File Name:    h264dec_slice.c                                             *
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
PUBLIC void H264Dec_decode_one_slice_I_sw (DEC_IMAGE_PARAMS_T *img_ptr)
{
	int32 end_of_slice = FALSE;
	DEC_MB_CACHE_T *mb_cache_ptr = g_mb_cache_ptr;
	DEC_MB_INFO_T *curr_mb_info_ptr = img_ptr->mb_info + img_ptr->curr_mb_nr;
	while (!end_of_slice)
	{
		H264Dec_start_macroblock (img_ptr, curr_mb_info_ptr, mb_cache_ptr);
		H264Dec_read_one_macroblock_ISlice_sw (img_ptr, curr_mb_info_ptr, mb_cache_ptr);
		end_of_slice = H264Dec_exit_macroblock (img_ptr, curr_mb_info_ptr, mb_cache_ptr);
		curr_mb_info_ptr++;
	}

	H264Dec_exit_slice (img_ptr);

	return;
}

PUBLIC void H264Dec_decode_one_slice_P_sw (DEC_IMAGE_PARAMS_T *img_ptr)
{
	int32 end_of_slice = FALSE;
	DEC_MB_CACHE_T *mb_cache_ptr = g_mb_cache_ptr;
	DEC_MB_INFO_T *curr_mb_info_ptr = img_ptr->mb_info + img_ptr->curr_mb_nr;

	img_ptr->cod_counter = -1;

	set_ref_pic_num(img_ptr);

	while (!end_of_slice)
	{
		H264Dec_start_macroblock (img_ptr, curr_mb_info_ptr, mb_cache_ptr);
		H264Dec_read_one_macroblock_PSlice_sw (img_ptr, curr_mb_info_ptr, mb_cache_ptr);
		if (img_ptr->error_flag)
		{
			return;
		}
		end_of_slice = H264Dec_exit_macroblock (img_ptr, curr_mb_info_ptr, mb_cache_ptr);
		curr_mb_info_ptr++;
	}

	H264Dec_exit_slice (img_ptr);

	return;
}

PUBLIC void H264Dec_decode_one_slice_B_sw (DEC_IMAGE_PARAMS_T *img_ptr)
{
	int32 end_of_slice = FALSE;
	DEC_MB_CACHE_T *mb_cache_ptr = g_mb_cache_ptr;
	DEC_MB_INFO_T *curr_mb_info_ptr = img_ptr->mb_info + img_ptr->curr_mb_nr;

	img_ptr->cod_counter = -1;

	set_ref_pic_num(img_ptr);

	while (!end_of_slice)
	{
		H264Dec_start_macroblock (img_ptr, curr_mb_info_ptr, mb_cache_ptr);
		H264Dec_read_one_macroblock_BSlice_sw (img_ptr, curr_mb_info_ptr, mb_cache_ptr);
		end_of_slice = H264Dec_exit_macroblock (img_ptr, curr_mb_info_ptr, mb_cache_ptr);
		
		curr_mb_info_ptr++; 
	}

	H264Dec_exit_slice (img_ptr);

	return;
}

/*extend 24 pixel*/
#ifdef WIN32
void H264Dec_extent_frame (DEC_IMAGE_PARAMS_T *img_ptr, DEC_STORABLE_PICTURE_T * dec_picture)
{
	int32 i;
	int32 height, offset, extendWidth;
	uint8 *pSrc1, *pSrc2, *pDst1, *pDst2;
	uint8 **Frame = dec_picture->imgYUV;
#ifdef _NEON_OPT_	
	uint8x8_t vec64;
	uint8x16_t vec128;
#endif	

//#ifndef _ASM_HOR_EXTENSION_
	int32 width;

	height      = img_ptr->height;
	width       = img_ptr->width;
	extendWidth = img_ptr->ext_width;

	pSrc1 = Frame[0] + img_ptr->start_in_frameY;
	pDst1 = pSrc1 - Y_EXTEND_SIZE;
	pSrc2 = pSrc1 + width - 1;
	pDst2 = pSrc2 + 1;

	/*horizontal repeat Y*/
	for (i = 0; i < height; i++)
	{
	#ifndef _NEON_OPT_
		int32 intValue = ((int32)(*pSrc1) << 24) | ((int32)(*pSrc1) << 16) | ((int32)(*pSrc1) << 8) | (int32)(*pSrc1);
		int32 *pIntDst = (int32 *)pDst1;
		pIntDst[0] = intValue;
		pIntDst[1] = intValue;
		pIntDst[2] = intValue;
		pIntDst[3] = intValue;
		pIntDst[4] = intValue;
		pIntDst[5] = intValue;

		intValue = ((int32)(*pSrc2) << 24) | ((int32)(*pSrc2) << 16) | ((int32)(*pSrc2) << 8) | (int32)(*pSrc2);
		pIntDst = (int32 *)pDst2;
		pIntDst[0] = intValue;
		pIntDst[1] = intValue;
		pIntDst[2] = intValue;
		pIntDst[3] = intValue;
		pIntDst[4] = intValue;
		pIntDst[5] = intValue;

	#else
		//left
		vec64 = vld1_lane_u8(pSrc1, vec64, 0);
		vec128 = vdupq_lane_u8(vec64, 0);
		vst1q_u8(pDst1, vec128);

		vec64 = vget_low_u8(vec128);
		vst1_u8(pDst1+16, vec64);

		//right
		vec64 = vld1_lane_u8(pSrc2, vec64, 0);
		vec128 = vdupq_lane_u8(vec64, 0);
		vst1q_u8(pDst2, vec128);

		vec64 = vget_low_u8(vec128);
		vst1_u8(pDst2+16, vec64);		
	#endif

		pSrc1 += extendWidth;
		pDst1 += extendWidth;
		pSrc2 += extendWidth;
		pDst2 += extendWidth;
	}

	/*horizontal repeat U*/
	extendWidth = extendWidth>>1;
	pSrc1       = Frame [1] + img_ptr->start_in_frameUV;
	pDst1       = pSrc1 - UV_EXTEND_SIZE;
	pSrc2 = pSrc1 + width / 2 - 1;
	pDst2 = pSrc2 + 1;
	height = height / 2;
	for(i = 0; i < height; i++)
	{
	#if 1//ndef _NEON_OPT_
		int32 intValue = ((int32)(*pSrc1) << 24) | ((int32)(*pSrc1) << 16) | ((int32)(*pSrc1) << 8) | (int32)(*pSrc1);
		int32 * pIntDst = (int32 *)pDst1;
		pIntDst[0] = intValue;
		pIntDst[1] = intValue;
		pIntDst[2] = intValue;
		
		intValue = ((int32)(*pSrc2) << 24) | ((int32)(*pSrc2) << 16) | ((int32)(*pSrc2) << 8) | (int32)(*pSrc2);
		pIntDst = (int32 *)pDst2;
		pIntDst[0] = intValue;
		pIntDst[1] = intValue;
		pIntDst[2] = intValue;
	#else
		//left
		vec64 = vld1_lane_u8(pSrc1, vec64, 0);
		vec128 = vdupq_lane_u8(vec64, 0);
		vst1q_u8(pDst1, vec128);

		//right
		vec64 = vld1_lane_u8(pSrc2, vec64, 0);
		vec128 = vdupq_lane_u8(vec64, 0);
		vst1q_u8(pDst2, vec128);
	#endif

		pSrc1 += extendWidth;
		pDst1 += extendWidth;
		pSrc2 += extendWidth;
		pDst2 += extendWidth;
	}
	/*horizontal repeat V*/
	pSrc1 = Frame [2] + img_ptr->start_in_frameUV;
	pDst1 = pSrc1 - UV_EXTEND_SIZE;
	pSrc2 = pSrc1 + width / 2 - 1;
	pDst2 = pSrc2 + 1;
	for(i = 0; i < height; i++)
	{
	#if 1//ndef _NEON_OPT_
		int32 intValue = ((int32)(*pSrc1) << 24) | ((int32)(*pSrc1) << 16) | ((int32)(*pSrc1) << 8) | (int32)(*pSrc1);
		int32 * pIntDst = (int32 *)pDst1;
		pIntDst[0] = intValue;
		pIntDst[1] = intValue;	
		pIntDst[2] = intValue;	
		
		intValue = ((int32)(*pSrc2) << 24) | ((int32)(*pSrc2) << 16) | ((int32)(*pSrc2) << 8) | (int32)(*pSrc2);
		pIntDst = (int32 *)pDst2;
		pIntDst[0] = intValue;
		pIntDst[1] = intValue;
		pIntDst[2] = intValue;
	#else
		//left
		vec64 = vld1_lane_u8(pSrc1, vec64, 0);
		vec128 = vdupq_lane_u8(vec64, 0);
		vst1q_u8(pDst1, vec128);

		//right
		vec64 = vld1_lane_u8(pSrc2, vec64, 0);
		vec128 = vdupq_lane_u8(vec64, 0);
		vst1q_u8(pDst2, vec128);
	#endif
		pSrc1 += extendWidth;
		pDst1 += extendWidth;
		pSrc2 += extendWidth;
		pDst2 += extendWidth;	
	}
//#else
//	asm_horExtendYUV_h264(Frame, img_ptr->ext_width, img_ptr->height);
//#endif //_ASM_HOR_EXTENSION_

	/*copy first row and last row*/
	/*vertical repeat Y*/
	height = img_ptr->height;
	extendWidth  = img_ptr->ext_width;
	offset = extendWidth * Y_EXTEND_SIZE;
	pSrc1  = Frame[0] + offset;
	pDst1  = Frame[0];
	pSrc2  = pSrc1 + extendWidth * (height - 1);
	pDst2  = pSrc2 + extendWidth;

	for(i = 0; i < Y_EXTEND_SIZE; i++)
	{
		memcpy(pDst1, pSrc1, extendWidth);
		memcpy(pDst2, pSrc2, extendWidth);
		pDst1 += extendWidth;
		pDst2 += extendWidth;
	}
 #if 1   
 /*vertical repeat U*/
    height = height / 2;
	extendWidth  = extendWidth / 2;
	offset = extendWidth * UV_EXTEND_SIZE;
	pSrc1  = Frame[1] + offset;
	pDst1  = Frame[1];
	pSrc2  = pSrc1 + extendWidth * (height - 1);
	pDst2  = pSrc2 + extendWidth;

	for(i = 0; i < UV_EXTEND_SIZE; i++)
	{
		memcpy(pDst1, pSrc1, extendWidth);
		memcpy(pDst2, pSrc2, extendWidth);
		pDst1 += extendWidth;
		pDst2 += extendWidth;
	}

	/*vertical repeat V*/
	pSrc1  = Frame[2] + offset;
	pDst1  = Frame[2];
	pSrc2  = pSrc1 + extendWidth * (height - 1);
	pDst2  = pSrc2 + extendWidth;
	
	for(i = 0; i < UV_EXTEND_SIZE; i++)
	{
		memcpy(pDst1, pSrc1, extendWidth);
		memcpy(pDst2, pSrc2, extendWidth);
		pDst1 += extendWidth;
		pDst2 += extendWidth;
	}	
#endif	

}
#endif

#ifndef YUV_THREE_PLANE
PUBLIC void H264Dec_write_disp_frame (DEC_IMAGE_PARAMS_T *img_ptr, DEC_STORABLE_PICTURE_T * dec_picture)
{
	int32 height      = img_ptr->height;
	int32 width       = img_ptr->width;
	int32 extendWidth = img_ptr->ext_width;
	int32 offset;
	uint8 *pSrc_y, *pSrc_u, *pSrc_v, *pDst;
	int32 row, col;

	//y
	pSrc_y = dec_picture->imgYUV[0] + img_ptr->start_in_frameY;
	pDst = dec_picture->imgY;
	for (row = 0; row < height; row++)
	{
	#if 1//ndef _NEON_OPT_
		memcpy(pDst, pSrc_y, width);
	#else
		uint8x8x4_t v8x8x4;
		for (col = 0; col < width; col += 32)
		{
			v8x8x4 = vld4_u8(pSrc_y + col);
			vst4_u8(pDst + col, v8x8x4);
		}
	#endif
		pDst += width;
		pSrc_y += extendWidth;
	}

	//u and v
	height >>= 1;
	width >>= 1;
	extendWidth >>= 1;
	offset = img_ptr->start_in_frameUV;

	pSrc_u = dec_picture->imgYUV[1] + offset;
	pSrc_v = dec_picture->imgYUV[2] + offset;
	pDst = dec_picture->imgU;
#ifndef _NEON_OPT_ 
	for (row = 0; row < height; row++)
	{
		for (col = 0; col < width; col++)
		{
			pDst[col*2] = pSrc_u[col];
			pDst[col*2+1] = pSrc_v[col];
		}

		pDst += width*2;
		pSrc_u += extendWidth;
		pSrc_v += extendWidth;
	}
#else
{
	uint8x8_t u_vec64, v_vec64;
	uint16x8_t u_vec128, v_vec128;
	uint16x8_t uv_vec128;
	
	for (row = 0; row < height; row++)
	{
		for (col = 0; col < width; col+=8)
		{
			u_vec64 = vld1_u8(pSrc_u);
			v_vec64 = vld1_u8(pSrc_v);

			u_vec128 = vmovl_u8(u_vec64);
			v_vec128 = vmovl_u8(v_vec64);

			uv_vec128 = vsliq_n_u16(u_vec128, v_vec128, 8);
			vst1q_u16((uint16*)pDst, uv_vec128);

			pSrc_u += 8;
			pSrc_v += 8;
			pDst += 16;
		}
		pSrc_u += (UV_EXTEND_SIZE*2);
		pSrc_v += (UV_EXTEND_SIZE*2);
	}
}	
#endif
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
