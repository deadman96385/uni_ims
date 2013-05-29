#include "sc8810_video_header.h"

uint32 readMB_skip_flagInfo_CABAC_High()
{
	LINE_BUF_T *MBLineBuf;
	int32 a, b;
	MotionInfoContexts *ctx = g_image_ptr->curr_slice_ptr->mot_ctx;	//接口可能需要重新定义
	int32 act_ctx;
	int32 act_sym;
	int mb_x		= currMBBuf->mb_x;
	int slice_type	= currMBBuf->slice_type;
	int lmb_avail	= currMBBuf->lmb_avail;
	int tmb_avail	= currMBBuf->tmb_avail;

	MBLineBuf= vldLineBuf + mb_x;

	if (!tmb_avail)
	{
		b = 0;
	}
	else
	{
		b = (MBLineBuf->mb_skip_flag == 1) ? 0 : 1;
	}

	if (!lmb_avail)
	{
		a = 0;
	}
	else
	{
		a = (leftMBBuf->mb_skip_flag == 1) ? 0 : 1;
	}

	if (slice_type == B_SLICE)
	{
		act_ctx = 7 + a + b;
		act_sym = biari_decode_symbol(g_image_ptr, &ctx->mb_type_contexts[2][act_ctx]);//接口可能需要重新定义
		
	}
	else
	{
		act_ctx = a + b;
		act_sym = biari_decode_symbol(g_image_ptr, &ctx->mb_type_contexts[1][act_ctx]);
	}

	return act_sym;
}

uint32 readMB_tran8x8_flagInfo_CABAC_High()
{
	LINE_BUF_T *MBLineBuf;
	int32 a, b;
	MotionInfoContexts *ctx = g_image_ptr->curr_slice_ptr->mot_ctx;	//接口可能需要重新定义
	int32 act_ctx;
	int mb_x		= currMBBuf->mb_x;
	int lmb_avail	= currMBBuf->lmb_avail;
	int tmb_avail	= currMBBuf->tmb_avail;

	MBLineBuf= vldLineBuf + mb_x;

	if (!tmb_avail)
	{
		b = 0;
	}
	else
	{
		b = (MBLineBuf->transform_size_8x8_flag&1) ? 1 : 0;
	}

	if (!lmb_avail)
	{
		a = 0;
	}
	else
	{
		a = (leftMBBuf->transform_size_8x8_flag&1) ? 1 : 0;
	}

	act_ctx = a + b;

	currMBBuf->transform_size_8x8_flag = biari_decode_symbol(g_image_ptr, &ctx->transform_size_contexts[act_ctx]);

	return currMBBuf->transform_size_8x8_flag;
}		

uint32 readMB_typeInfo_CABAC_High (void)//模式的值需要改成和标准一样的，现在是用的JM里头的值
{
	int slice_type	= currMBBuf->slice_type;
	MotionInfoContexts *ctx = g_image_ptr->curr_slice_ptr->mot_ctx;
	int32 a, b;
	int32 act_ctx;
	int32 act_sym;
	LINE_BUF_T *MBLineBuf;
	int mb_x = currMBBuf->mb_x;
	int lmb_avail	= currMBBuf->lmb_avail;
	int tmb_avail	= currMBBuf->tmb_avail;
	int32 mode_sym;

	MBLineBuf= vldLineBuf + mb_x;

	if (slice_type == I_SLICE)	//Intra frame
	{
		if (!tmb_avail)
		{
			b = 0;
		}else
		{
			b = ((MBLineBuf->mb_type != 0) ? 1 : 0);
		}

		if (!lmb_avail)
		{
			a = 0;
		}else
		{
			a = ((leftMBBuf->mb_type != 0) ? 1 : 0);
		}

		act_ctx = a + b;
		act_sym = biari_decode_symbol(g_image_ptr, ctx->mb_type_contexts[0] + act_ctx);
		// store context

		if (act_sym == 0) ///4x4 intra
		{
			currMBBuf->mb_type = act_sym;
		}else //16x16 intra
		{
			mode_sym = biari_decode_final (g_image_ptr);
			if (mode_sym == 1)
			{
				currMBBuf->mb_type = 25;
			}else
			{
				act_sym = 1;
				act_ctx = 4;
				mode_sym = biari_decode_symbol(g_image_ptr, ctx->mb_type_contexts[0]+act_ctx); //decoding of AC/no AC
				act_sym += mode_sym*12;
				act_ctx = 5;
				//decoding of cbp: 0, 1, 2
				mode_sym = biari_decode_symbol(g_image_ptr, ctx->mb_type_contexts[0]+act_ctx);
				if (mode_sym != 0)
				{
					act_ctx = 6;
					mode_sym = biari_decode_symbol(g_image_ptr, ctx->mb_type_contexts[0]+act_ctx);
					act_sym += 4;
					if (mode_sym != 0)
					{
						act_sym += 4;
					}
				}

				//decoding of I pred-mode: 0, 1, 2, 3
				act_ctx = 7;
				mode_sym = biari_decode_symbol(g_image_ptr, ctx->mb_type_contexts[0]+act_ctx);
				act_sym += mode_sym*2;
				act_ctx = 8;
				mode_sym = biari_decode_symbol(g_image_ptr, ctx->mb_type_contexts[0]+act_ctx);
				act_sym += mode_sym;
				currMBBuf->mb_type = act_sym;
			}
		}		
	}else
	{
		if (slice_type == B_SLICE)
		{
			if (!tmb_avail)
			{
				b = 0;
			}else
			{
				b = ((MBLineBuf->mb_type != 0 && MBLineBuf->mb_skip_flag != 1) ? 1 : 0);
			}

			if (!lmb_avail)
			{
				a = 0;
			}else
			{
				a = ((leftMBBuf->mb_type != 0 && leftMBBuf->mb_skip_flag != 1) ? 1 : 0);
			}

			act_ctx = a + b;

			if (biari_decode_symbol(g_image_ptr, &ctx->mb_type_contexts[2][act_ctx]))
			{
				if (biari_decode_symbol(g_image_ptr, &ctx->mb_type_contexts[2][4]))
				{
					if (biari_decode_symbol(g_image_ptr, &ctx->mb_type_contexts[2][5]))
					{
						act_sym = 12;

						if (biari_decode_symbol(g_image_ptr, &ctx->mb_type_contexts[2][6]))	act_sym +=8;
						if (biari_decode_symbol(g_image_ptr, &ctx->mb_type_contexts[2][6]))	act_sym +=4;
						if (biari_decode_symbol(g_image_ptr, &ctx->mb_type_contexts[2][6]))	act_sym +=2;

						if (act_sym == 24)	act_sym = 11;
						else if (act_sym == 26)	act_sym = 22;
						else
						{
							if (act_sym == 22)	act_sym = 23;
							if (biari_decode_symbol(g_image_ptr, &ctx->mb_type_contexts[2][6])) act_sym += 1;
						}
					}else
					{
						act_sym = 3;

						if (biari_decode_symbol(g_image_ptr, &ctx->mb_type_contexts[2][6]))	act_sym +=4;
						if (biari_decode_symbol(g_image_ptr, &ctx->mb_type_contexts[2][6]))	act_sym +=2;
						if (biari_decode_symbol(g_image_ptr, &ctx->mb_type_contexts[2][6]))	act_sym +=1;
					}
				}else
				{
					if (biari_decode_symbol(g_image_ptr, &ctx->mb_type_contexts[2][6]))	act_sym =2;
					else	act_sym = 1;
				}
			}else
			{
				act_sym = 0;
			}
		}else //p frame
		{
			if (biari_decode_symbol(g_image_ptr, &ctx->mb_type_contexts[1][4]))
			{
				if (biari_decode_symbol(g_image_ptr, &ctx->mb_type_contexts[1][7]))	act_sym = 6;
				else	act_sym = 5;
			}else
			{
				if (biari_decode_symbol(g_image_ptr, &ctx->mb_type_contexts[1][5]))
				{
					if (biari_decode_symbol(g_image_ptr, &ctx->mb_type_contexts[1][7]))	act_sym = 1;
					else act_sym = 2;
				}else
				{
					if (biari_decode_symbol(g_image_ptr, &ctx->mb_type_contexts[1][6]))	act_sym = 3;
					else	act_sym = 0;
				}
			}
		}

		if (act_sym <= 5 || (((g_image_ptr->type == B_SLICE)?1:0) && act_sym <= 23))
		{
			currMBBuf->mb_type = act_sym;
		}else// additional info for 16x16 Intra-mode
		{
			mode_sym = biari_decode_final(g_image_ptr);

			if (mode_sym == 1)
			{
				if (slice_type == B_SLICE)	//B frame
				{
					currMBBuf->mb_type = 48;
				}else	//P frame
				{
					currMBBuf->mb_type = 30;
				}
			}else
			{
				act_ctx = 8;
				mode_sym = biari_decode_symbol(g_image_ptr, ctx->mb_type_contexts[1] + act_ctx); //decoding of AC/no AC
				act_sym += mode_sym*12;

				//decoding of cbp: 0, 1, 2
				act_ctx = 9;
				mode_sym = biari_decode_symbol(g_image_ptr, ctx->mb_type_contexts[1] + act_ctx);
				if (mode_sym != 0)
				{
					act_sym += 4;
					mode_sym = biari_decode_symbol(g_image_ptr, ctx->mb_type_contexts[1] + act_ctx);
					if (mode_sym != 0)
					{
						act_sym += 4;
					}
				}

				// decoding of I pred-mode: 0,1,2,3
				act_ctx = 10;
				mode_sym = biari_decode_symbol(g_image_ptr, ctx->mb_type_contexts[1] + act_ctx);
				act_sym += mode_sym*2;
				mode_sym = biari_decode_symbol(g_image_ptr, ctx->mb_type_contexts[1] + act_ctx);
				act_sym += mode_sym;
				currMBBuf->mb_type = act_sym;
			}
		}
	}
	
	return currMBBuf->mb_type;
}

uint32 readB8_typeInfo_CABAC_High (void)
{
	DEC_SLICE_T *curr_slice_ptr = g_image_ptr->curr_slice_ptr;
	MotionInfoContexts *ctx = curr_slice_ptr->mot_ctx;
	int32 bframe=(g_image_ptr->type==B_SLICE);
	int32 b8_type;

	if (!bframe)
	{
		if (biari_decode_symbol(g_image_ptr, &ctx->b8_type_contexts[0][1]))
		{
			b8_type = 0;
		}else
		{
			if (biari_decode_symbol(g_image_ptr, &ctx->b8_type_contexts[0][3]))
			{
				if (biari_decode_symbol(g_image_ptr, &ctx->b8_type_contexts[0][4])) b8_type = 2;
				else	b8_type = 3;
			}else
			{
				b8_type = 1;
			}
		}
	}else //B frame
	{
		if (biari_decode_symbol(g_image_ptr, &ctx->b8_type_contexts[1][0]))
		{
			if (biari_decode_symbol(g_image_ptr, &ctx->b8_type_contexts[1][1]))
			{
				if (biari_decode_symbol(g_image_ptr, &ctx->b8_type_contexts[1][2]))
				{
					if (biari_decode_symbol(g_image_ptr, &ctx->b8_type_contexts[1][3]))
					{
						b8_type = 10;
						if (biari_decode_symbol(g_image_ptr, &ctx->b8_type_contexts[1][3]))	b8_type++;
					}else
					{
						b8_type = 6;
						if (biari_decode_symbol(g_image_ptr, &ctx->b8_type_contexts[1][3]))	b8_type += 2;
						if (biari_decode_symbol(g_image_ptr, &ctx->b8_type_contexts[1][3]))	b8_type++;
					}
				}else
				{
					b8_type = 2;
					if (biari_decode_symbol(g_image_ptr, &ctx->b8_type_contexts[1][3]))	b8_type += 2;
					if (biari_decode_symbol(g_image_ptr, &ctx->b8_type_contexts[1][3]))	b8_type+=1;
				}
			}else
			{
				if (biari_decode_symbol(g_image_ptr, &ctx->b8_type_contexts[1][3]))	b8_type = 1;
				else	b8_type = 0;
			}
			b8_type++;
		}else
		{
			b8_type = 0;
		}
	}

	return b8_type;
}

int32 readCBP_CABAC_High ()
{
	DEC_SLICE_T *curr_slice_ptr = g_image_ptr->curr_slice_ptr;
	TextureInfoContexts *ctx = curr_slice_ptr->tex_ctx;
	int32 mb_x, mb_y;
	int32 a, b;
	int32 curr_cbp_ctx, curr_cbp_idx;
	int32 cbp = 0;
	int32 cbp_bit;
	int32 msk;
	LINE_BUF_T *MBLineBuf;
	int mb_h = currMBBuf->mb_x;
	int lmb_avail	= currMBBuf->lmb_avail;
	int tmb_avail	= currMBBuf->tmb_avail;
	int slice_type	= currMBBuf->slice_type;
	int mb_type;

	MBLineBuf= vldLineBuf + mb_h;

	//  coding of luma part (bit by bit)
	for (mb_y = 0; mb_y < 4; mb_y += 2)
	{
		for (mb_x = 0; mb_x < 4; mb_x += 2)
		{
			if (mb_y == 0)
			{
				if (!tmb_avail)
				{
					b = 0;
				}else
				{
					mb_type = MBLineBuf->mb_type;
					if (IS_I_PCM)
					{
						b = 0;
					}else
					{
						b = ((((MBLineBuf->cbp) & (1 << (2 + mb_x/2))) == 0)? 1: 0);
					}					
				}
			}else
			{
				b = (((cbp & (1<<(mb_x/2))) == 0) ? 1: 0);
			}

			if (mb_x == 0)
			{
				if (lmb_avail)
				{
					mb_type = leftMBBuf->mb_type;
					if (IS_I_PCM)
					{
						a = 0;
					}else
					{
						a = (((leftMBBuf->cbp & (1<<(mb_y+1))) == 0)? 1: 0);
					}
				}else
				{
					a = 0;
				}
			}else
			{
				a = (((cbp & (1<<mb_y)) == 0) ? 1 : 0);
			}
		
			curr_cbp_ctx = a + 2*b;
			msk = (1<<(mb_y+mb_x/2));
			cbp_bit = biari_decode_symbol(g_image_ptr, ctx->cbp_contexts[0]+curr_cbp_ctx);
			if (cbp_bit) cbp += msk;
		}
	}

	mb_type = currMBBuf->mb_type;
	if (IS_I_16x16 || IS_I_NxN || IS_I_PCM )
	{
		curr_cbp_idx = 0;
	}else
	{
		curr_cbp_idx = 1;
	}

	 // coding of chroma part
	 // CABAC decoding for BinIdx 0
	 b = 0;
	 if (tmb_avail)
	 {
		mb_type = MBLineBuf->mb_type;
	 	if (IS_I_PCM)
	 	{
	 		b = 1;
	 	}else
	 	{
	 		b = ((MBLineBuf->cbp) > 15)?1: 0;
	 	}
	 }

	 a = 0;
	 if (lmb_avail)
	 {
		mb_type = leftMBBuf->mb_type;
		if (IS_I_PCM)
		{
			a = 1;
		}else
		{
			a = ((leftMBBuf->cbp) > 15)?1:0;
		}
	 }

	 curr_cbp_ctx = a + 2*b;
	 cbp_bit = biari_decode_symbol(g_image_ptr,  ctx->cbp_contexts[1]+curr_cbp_ctx);
	 
	 // CABAC decoding for BinIdx 1 
	 if (cbp_bit) // set the chroma bits
	 {
	 	b = 0;
	 	if (tmb_avail)
	 	{
			mb_type = MBLineBuf->mb_type;
		 	if (IS_I_PCM)
		 	{
		 		b = 1;
		 	}else
		 	{
		 		if ((MBLineBuf->cbp) > 15)
		 		{
		 			b = (( (MBLineBuf->cbp >> 4) == 2) ? 1 : 0);
		 		}
		 	}
		 }

	 	a = 0;
	 	if (lmb_avail)
		 {
			mb_type = leftMBBuf->mb_type;
			if (IS_I_PCM)
			{
				a = 1;
			}else
			{
				if (leftMBBuf->cbp > 15)
				{
          			a = (( (leftMBBuf->cbp >> 4) == 2) ? 1 : 0);
				}
			}
		 }

	 	curr_cbp_ctx = a + 2*b;
	 	cbp_bit = biari_decode_symbol(g_image_ptr, ctx->cbp_contexts[2] + curr_cbp_ctx);
	 	cbp += (cbp_bit == 1) ? 32:16;
	 }

	if (!cbp)
	{
		currMBBuf->cbp = 0;
		currMBBuf->delta_qp = 0;
	}

	return cbp;
}

int32 readDquant_CABAC_High ()
{
  	DEC_SLICE_T *curr_slice_ptr = g_image_ptr->curr_slice_ptr;
	MotionInfoContexts *ctx = curr_slice_ptr->mot_ctx;
	int32 act_ctx;
	int32 act_sym;
	int32 dquant;

	act_ctx = ((leftMBBuf->delta_qp != 0) ? 1 : 0);

	act_sym = biari_decode_symbol(g_image_ptr, ctx->delta_qp_contexts + act_ctx);
	if (act_sym != 0)
	{
		act_ctx = 2;
		act_sym = unary_bin_decode(g_image_ptr, ctx->delta_qp_contexts + act_ctx, 1);
		act_sym++;
	}

	dquant = (act_sym+1)/2;
	if ((act_sym & 0x01) == 0)	// lsb is signed bit
	{
		dquant = -dquant;
	}

	currMBBuf->delta_qp = dquant;

	return dquant;
}

uint32 readPrev_intra_pred_mode_flag_CABAC_High(void)
{
	DEC_SLICE_T *curr_slice_ptr = g_image_ptr->curr_slice_ptr;
	TextureInfoContexts *ctx = curr_slice_ptr->tex_ctx;

  	// use_most_probable_mode
  	return biari_decode_symbol(g_image_ptr, ctx->ipr_contexts);
}

uint32 readRem_intra_pred_mode_CABAC_High()
{
	DEC_SLICE_T *curr_slice_ptr = g_image_ptr->curr_slice_ptr;
	DEC_PPS_T	*active_pps_ptr = g_active_pps_ptr;
	TextureInfoContexts *ctx = curr_slice_ptr->tex_ctx;
	int32 pred;

	pred  = 0;
	pred |= (biari_decode_symbol(g_image_ptr, ctx->ipr_contexts+1)     );
	pred |= (biari_decode_symbol(g_image_ptr, ctx->ipr_contexts+1) << 1);
	pred |= (biari_decode_symbol(g_image_ptr, ctx->ipr_contexts+1) << 2);

	return pred;
}

int32 readCIPredMode_CABAC_High ()
{
	DEC_SLICE_T *curr_slice_ptr = g_image_ptr->curr_slice_ptr;
	DEC_PPS_T	*active_pps_ptr = g_active_pps_ptr;
	TextureInfoContexts *ctx = curr_slice_ptr->tex_ctx;
	int32 act_ctx, a, b;
	int act_sym;
	LINE_BUF_T *MBLineBuf;
	int mb_h = currMBBuf->mb_x;
	int lmb_avail	= currMBBuf->lmb_avail;
	int tmb_avail	= currMBBuf->tmb_avail;
	int mb_type;
	int slice_type	= currMBBuf->slice_type;

	MBLineBuf= vldLineBuf + mb_h;

	if (!tmb_avail)
	{
		b = 0;
	}else
	{
		mb_type = MBLineBuf->mb_type;
		if (IS_I_PCM)
		{
			b = 0;
		}else
		{
			b = ((MBLineBuf->c_ipred_mode != 0) ? 1 : 0);
		}
	}

	if (!lmb_avail)
	{
		a = 0;
	}else
	{
		mb_type = leftMBBuf->mb_type;
		if (IS_I_PCM)
		{
			a = 0;
		}else
		{
			a = ((leftMBBuf->c_ipred_mode != 0) ? 1 : 0);
		}
	}

	act_ctx = a+b;

	act_sym = biari_decode_symbol(g_image_ptr, ctx->cipr_contexts + act_ctx );

	if (act_sym != 0)
	{
		act_sym = unary_bin_max_decode (g_image_ptr, ctx->cipr_contexts+3, 0, 2)+1;
	}

	return act_sym;
	
}

int32 readRefFrame_CABAC_High (int32 blk_id, int32 list)
{
	DEC_SLICE_T *curr_slice_ptr = g_image_ptr->curr_slice_ptr;
	MotionInfoContexts *ctx = curr_slice_ptr->mot_ctx;

	int32 addctx = 0;
	int32 a, b;
	int32 act_ctx;
	int32 act_sym;
	LINE_BUF_T *MBLineBuf;
	int mb_h = currMBBuf->mb_x;

	MBLineBuf= vldLineBuf + mb_h;

	switch(blk_id)
	{
	case 0:
		if (!currMBBuf->lmb_avail)
		{
			a = 0;
		}
		else
		{
			int8 ref_idx = ((leftMBBuf->ref[list] >> 8) & 0xff);
			a = (ref_idx > 0) ? 1 : 0;
		}
		
		if (!currMBBuf->tmb_avail)
		{
			b = 0;
		}
		else
		{
			int8 ref_idx = ((MBLineBuf->ref[list] >> 0) & 0xff);
			b = (ref_idx > 0) ? 1 : 0;
		}

		break;
	case 2:
		{
			int8 ref_idx = ((currMBBuf->ref[list] >> 0) & 0xff);
			a = (ref_idx > 0) ? 1 : 0;
		}
		
		if (!currMBBuf->tmb_avail)
		{
			b = 0;
		}
		else
		{
			int8 ref_idx = ((MBLineBuf->ref[list] >> 8) & 0xff);
			b = (ref_idx > 0) ? 1 : 0;
		}

		break;
	case 8:
		if (!currMBBuf->lmb_avail)
		{
			a = 0;
		}
		else
		{
			int8 ref_idx = ((leftMBBuf->ref[list] >> 24) & 0xff);
			a = (ref_idx > 0) ? 1 : 0;
		}
		
		{
			int8 ref_idx = ((currMBBuf->ref[list] >> 0) & 0xff);
			b = (ref_idx > 0) ? 1 : 0;
		}
		break;
	case 10:
	
		{
			int8 ref_idx = ((currMBBuf->ref[list] >> 16) & 0xff);
			a = (ref_idx > 0) ? 1 : 0;
		}
		
		{
			int8 ref_idx = ((currMBBuf->ref[list] >> 8) & 0xff);
			b = (ref_idx > 0) ? 1 : 0;
		}
		break;
	}

	act_ctx = a + 2*b;
	g_image_ptr->context = act_ctx;	// store context

	act_sym = biari_decode_symbol(g_image_ptr, ctx->ref_no_contexts[addctx]+act_ctx);

	if (act_sym != 0)
	{
		act_ctx = 4;
		act_sym = unary_bin_decode(g_image_ptr, ctx->ref_no_contexts[addctx]+act_ctx, 1);
		act_sym++;
	}

	return act_sym;
}

int32 readMVD_CABAC_High (int32 block4x4Idx, int32 list, int32 compIdx)
{
	DEC_SLICE_T *curr_slice_ptr = g_image_ptr->curr_slice_ptr;
	MotionInfoContexts *ctx = curr_slice_ptr->mot_ctx;
	int16 a, b;
	int32 act_ctx;
 	int32 mvd;
	int32 mv_sign;
	int32 mv_local_err;
	LINE_BUF_T *MBLineBuf;
	
	MBLineBuf= vldLineBuf + currMBBuf->mb_x;

	if (block4x4Idx == 0)
	{
		if (currMBBuf->lmb_avail)
		{
			a = (compIdx == 0) ? (leftMBBuf->mvd[3][list] & 0xffff) : ((leftMBBuf->mvd[3][list] >> 16) &0xffff);
		}
		else
			a = 0;
		if (currMBBuf->tmb_avail)
		{
			b = (compIdx == 0) ? (MBLineBuf->mvd[0][list] & 0xffff) : ((MBLineBuf->mvd[0][list] >> 16) &0xffff);
		}
		else
			b = 0;
	}
	else if (block4x4Idx < 4)
	{
//		a = currMBBuf->mvd[block4x4Idx - 1][list];
//		b = MBLineBuf->mvd[block4x4Idx][list];

		a = (compIdx == 0) ? (currMBBuf->mvd[block4x4Idx - 1][list] & 0xffff) : ((currMBBuf->mvd[block4x4Idx - 1][list] >> 16) &0xffff);
		if (currMBBuf->tmb_avail)
		{
			b = (compIdx == 0) ? (MBLineBuf->mvd[block4x4Idx][list] & 0xffff) : ((MBLineBuf->mvd[block4x4Idx][list] >> 16) &0xffff);
		}
		else
			b = 0;
	}
	else if (block4x4Idx % 4 == 0)
	{
//		a = leftMBBuf->mvd[block4x4Idx + 3][list];
//		b = currMBBuf->mvd[block4x4Idx - 4][list];
		if (currMBBuf->lmb_avail)
		{
			a = (compIdx == 0) ? (leftMBBuf->mvd[block4x4Idx + 3][list] & 0xffff) : ((leftMBBuf->mvd[block4x4Idx + 3][list] >> 16) &0xffff);
		}
		else
			a = 0;

		b = (compIdx == 0) ? (currMBBuf->mvd[block4x4Idx - 4][list] & 0xffff) : ((currMBBuf->mvd[block4x4Idx - 4][list] >> 16) &0xffff);
	}
	else
	{
//		a = currMBBuf->mvd[block4x4Idx - 1][list];
//		b = currMBBuf->mvd[block4x4Idx - 4][list];
		a = (compIdx == 0) ? (currMBBuf->mvd[block4x4Idx - 1][list] & 0xffff) : ((currMBBuf->mvd[block4x4Idx - 1][list] >> 16) &0xffff);
		b = (compIdx == 0) ? (currMBBuf->mvd[block4x4Idx - 4][list] & 0xffff) : ((currMBBuf->mvd[block4x4Idx - 4][list] >> 16) &0xffff);
	}

//	if (compIdx == 0)
//	{
//		a = (a & 0xffff);
//		b = (b & 0xffff);
//	}
//	else
//	{
//		a = ((a >> 16) & 0xffff);
//		b = ((b >> 16) & 0xffff);
//	}

	mv_local_err = ABS(a)+ABS(b);
	if (mv_local_err < 3) act_ctx = 5*compIdx;
	else
	{
		if (mv_local_err > 32)	act_ctx = 5*compIdx+3;
		else	act_ctx = 5*compIdx+2;
	}

	mvd = biari_decode_symbol(g_image_ptr, &ctx->mv_res_contexts[0][act_ctx]);

	if (mvd != 0)
	{
		act_ctx = 5*compIdx;
		mvd = unary_exp_golomb_mv_decode(g_image_ptr, ctx->mv_res_contexts[1]+act_ctx, 3);
		mvd++;
		mv_sign = biari_decode_symbol_eq_prob(g_image_ptr);

		if (mv_sign)	mvd = -mvd;
	}

	return mvd;
}