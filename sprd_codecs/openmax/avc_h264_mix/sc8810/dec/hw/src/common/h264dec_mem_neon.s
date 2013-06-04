
@//this asm file is for memory operation

			.text
					
			.arm

@//memset(dec_picture_ptr->ref_idx_ptr[0], -1, (img_ptr->frame_size_in_mbs << 4) * sizeof(char));
ref_idx_ptr0	.req 	r0
ref_idx_ptr1	.req 	r1
frm_size_in_mbs	.req	r2

memset_refIdx_negOne:	@FUNCTION
			.global	memset_refIdx_negOne
			push	{r14}

			mov		r3, #-1
			vdup.8	q0, r3

loop_f0:
			vst1.8	{d0, d1}, [ref_idx_ptr0, :128]!
			vst1.8	{d0, d1}, [ref_idx_ptr1, :128]!

			subs	r2, r2, #1
			bne		loop_f0

			pop		{pc}
			@ENDFUNC


@//memset(g_sps_ptr, 0, sizeof(DEC_SPS_T)-(6*16+2*64+4));
@//memset(g_sps_ptr->ScalingList4x4,16,6*16);
@//memset(g_sps_ptr->ScalingList8x8,16,2*64);
sps_ptr		.req		r0

memset_sps:	@FUNCTION
			.global	memset_sps

			push	{r14}

			mov		r1, #0
			vdup.8	q0, r1
			
			mov		r2, #16
			vdup.8	q1, r2

			add		r14, sps_ptr, #0x420	@//g_sps_ptr->ScalingList4x4
			add		r14, r14, #0x4

			@// memset sps
			mov		r3, #16		@// 16*64 = 1024 byte
loop_0_f1:
			vst1.8	{d0, d1}, [sps_ptr]!
			vst1.8	{d0, d1}, [sps_ptr]!
			vst1.8	{d0, d1}, [sps_ptr]!
			vst1.8	{d0, d1}, [sps_ptr]!

			subs	r3, r3, #1
			bne		loop_0_f1

			vst1.8	{d0, d1}, [sps_ptr]!	@//0x424 - 1024 = 36 byte, 
			vst1.8	{d0, d1}, [sps_ptr]!
			str 	r1, [sps_ptr]	
			
			@//memset scalinglist, 16*6byte + 2*64 byte = 16*14 byte
			mov		r3, #2
loop_1_f1:				
			vst1.8	{d2, d3}, [r14]!
			vst1.8	{d2, d3}, [r14]!
			vst1.8	{d2, d3}, [r14]!
			vst1.8	{d2, d3}, [r14]!
			vst1.8	{d2, d3}, [r14]!
			vst1.8	{d2, d3}, [r14]!
			vst1.8	{d2, d3}, [r14]!
			vst1.8	{d2, d3}, [r14]!

			subs	r3, r3, #1
			bne		loop_1_f1	

			pop		{pc}
			@ENDFUNC


@//memset (DC, 0, 16*sizeof(int16));
DC		.req		r0

memset_8words_zero:	@FUNCTION
			.global	memset_8words_zero

			push	{r14}

			mov		r14, #0
			mov		r1,	 #0
			mov		r2, #0
			mov		r3, #0

			stm		DC!,{r1, r2, r3, r14} 
			stm		DC!,{r1, r2, r3, r14} 

			pop		{pc}
			@ENDFUNC

@//memset(coeff, 0, N*32*sizeof(int16));
COEFF_f3	.req		r0
N_f3		.req		r1

clear_Nx16words:	@FUNCTION 
			.global	clear_Nx16words

			push	{r14}

			mov 	r14, #0
			vdup.8	q0, r14

loop_f3:			
			vst1.16	{d0, d1}, [COEFF_f3]!
			vst1.16	{d0, d1}, [COEFF_f3]!
			vst1.16	{d0, d1}, [COEFF_f3]!
			vst1.16	{d0, d1}, [COEFF_f3]!

			subs	r1, r1, #1
			bne		loop_f3

			pop		{pc}
			@ENDFUNC	

@//copy 8x8 pred to reconstruction frame
@//copyPredblk_8x8 (pred, rec, ext_width)
pred_f4			.req 		r0
rec_f4 			.req		r1
rec_width_f4	.req		r2

copyPredblk_8x8:
			.global	copyPredblk_8x8
			push	{r14}

			mov		r3,	#0x10

			vld1.8	{d0}, [pred_f4, :64], r3
			vld1.8	{d1}, [pred_f4, :64], r3
			vst1.8	{d0}, [rec_f4, :64], rec_width_f4
			vst1.8	{d1}, [rec_f4, :64], rec_width_f4

			vld1.8	{d0}, [pred_f4, :64], r3
			vld1.8	{d1}, [pred_f4, :64], r3
			vst1.8	{d0}, [rec_f4, :64], rec_width_f4
			vst1.8	{d1}, [rec_f4, :64], rec_width_f4

			vld1.8	{d0}, [pred_f4, :64], r3
			vld1.8	{d1}, [pred_f4, :64], r3
			vst1.8	{d0}, [rec_f4, :64], rec_width_f4
			vst1.8	{d1}, [rec_f4, :64], rec_width_f4

			vld1.8	{d0}, [pred_f4, :64], r3
			vld1.8	{d1}, [pred_f4, :64], r3
			vst1.8	{d0}, [rec_f4, :64], rec_width_f4
			vst1.8	{d1}, [rec_f4, :64], rec_width_f4

			pop		{pc}
			@ENDFUNC


@//void put_mb2Frame (uint8 *mb_pred, uint8 *mb_addr[3], int32 pitch)		
mb_pred_f5 		.req		r0
mb_addr_f5		.req		r1
pitch_f5		.req		r2

rec_f5			.req		r3
i_f5			.req		r14
			
put_mb2Frame:
			.global	put_mb2Frame
			push	{r4, r14}

			@//y
			ldr		rec_f5,	[mb_addr_f5, #0]
			mov		r4, #0x10
			
			mov		i_f5, #0x10
y_loop_f5:
			vld1.8	{d0, d1}, [mb_pred_f5], r4
			vld1.8	{d2, d3}, [mb_pred_f5], r4
			vst1.8	{d0, d1}, [rec_f5], pitch_f5
			vst1.8	{d2, d3}, [rec_f5], pitch_f5
			
			vld1.8	{d0, d1}, [mb_pred_f5], r4
			vld1.8	{d2, d3}, [mb_pred_f5], r4
			vst1.8	{d0, d1}, [rec_f5], pitch_f5
			vst1.8	{d2, d3}, [rec_f5], pitch_f5

			subs	i_f5, i_f5, #4
			bne		y_loop_f5

			@//u
			ldr		rec_f5,	[mb_addr_f5, #4]
			mov		r4, #0x8
			mov		pitch_f5, pitch_f5, lsr	#1
			
			mov		i_f5, #0x8
u_loop_f5:
			vld1.8	{d0}, [mb_pred_f5], r4
			vld1.8	{d1}, [mb_pred_f5], r4
			vst1.8	{d0}, [rec_f5], pitch_f5
			vst1.8	{d1}, [rec_f5], pitch_f5
			
			vld1.8	{d0}, [mb_pred_f5], r4
			vld1.8	{d1}, [mb_pred_f5], r4
			vst1.8	{d0}, [rec_f5], pitch_f5
			vst1.8	{d1}, [rec_f5], pitch_f5

			subs	i_f5, i_f5, #4
			bne		u_loop_f5

			@//v
			ldr 	rec_f5, [mb_addr_f5, #8]
						
			mov 	i_f5, #0x8
v_loop_f5:
			vld1.8	{d0}, [mb_pred_f5], r4
			vld1.8	{d1}, [mb_pred_f5], r4
			vst1.8	{d0}, [rec_f5], pitch_f5
			vst1.8	{d1}, [rec_f5], pitch_f5
						
			vld1.8	{d0}, [mb_pred_f5], r4
			vld1.8	{d1}, [mb_pred_f5], r4
			vst1.8	{d0}, [rec_f5], pitch_f5
			vst1.8	{d1}, [rec_f5], pitch_f5
			
			subs	i_f5, i_f5, #4
			bne 	v_loop_f5

			pop 	{r4, pc}
			@ENDFUNC	
			
			.end
				

