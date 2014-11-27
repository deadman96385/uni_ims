@//this asm file is for outloop deblocking
@//void arm_Mp4Dec_Deblock_Process_Block(DBK_PATA_T * dbk_para, uint8 * above_edge_ptr, uint8 * below_edge_ptr, uint8 * dst_block_ptr)
@/****************register map begin*******************/
dbk_para		.req			r0
above_edge_ptr		.req			r1
below_edge_ptr		.req			r2
dst_block_ptr		.req			r3
line_width		.req			r4
bs			.req			r5
alpha			.req	 		r6
beta			.req			r7
clip			.req			r8
plane_idx		.req			r9
is_luma			.req			r9
direction		.req			r10
clip_1			.req			r11
@/***************register map end**********************/




				.text
				.arm


@//////////////////////////////////////////////////////////////////////////////

arm_Mp4Dec_Deblock_Process_Block:	@FUNCTION
				.global arm_Mp4Dec_Deblock_Process_Block

				push		{r4 - r11, r14}

				ldrsh		line_width, [dbk_para, #0]
				ldrb		bs, [dbk_para, #4]

				@// load 8x8 pixel to d0 - d7
				vld1.8 		{d0}, [above_edge_ptr],line_width
				vld1.8 		{d4}, [below_edge_ptr],line_width
				vld1.8 		{d1}, [above_edge_ptr],line_width
				vld1.8 		{d5}, [below_edge_ptr],line_width
				vld1.8 		{d2}, [above_edge_ptr],line_width
				vld1.8 		{d6}, [below_edge_ptr],line_width
				vld1.8 		{d3}, [above_edge_ptr],line_width
				vld1.8 		{d7}, [below_edge_ptr],line_width


				@// filtering start
				cmp		bs, #0
				beq		Trans_array
			
				ldrsh		plane_idx, [dbk_para, #8]
				ldrb		alpha, [dbk_para, #5]
				ldrb		beta, [dbk_para, #6]
				ldrb		clip, [dbk_para, #7]
				cmp		plane_idx, #0
				moveq		is_luma, #0xff
				movne		is_luma, #0

				vdup.8		d8, alpha
				vdup.8		d9, beta
				vdup.8		d10, clip
				vdup.8		d11, is_luma

				vabd.u8		d12, d3, d4 	@// abs_p0_q0
				vabd.u8		d13, d2, d3	@// abs_p1_p0
				vabd.u8		d14, d5, d4	@// abs_q1_q0
				vabd.u8		d15, d1, d3	@// ap	
				vabd.u8		d16, d6, d4	@// aq

				vcgt.u8		d17, d8, d12	@// (abs_p0_q0 < alpha)
				vcgt.u8		d18, d9, d13	@// (abs_p1_p0 < beta)
				vcgt.u8		d19, d9, d14	@// (abs_q1_q0 < beta)
				vand		d17, d17, d18
				vand		d17, d17, d19	@// filt_ena
				vcgt.u8		d18, d9,  d15	@// is_ap_lt_b
				vcgt.u8		d19, d9,  d16	@// is_aq_lt_b

				cmp		bs, #4
				bne		Weak_filter
Strong_filter:
				lsr		alpha, alpha, #2
				add		alpha, alpha, #2
				vdup.8		d8, alpha
				vcgt.u8		d16, d8, d12	@// small_gap

				vaddl.u8	q10, d3, d4	@// sum_p0_q0
				vaddw.u8	q11, q10,d2	@// sum_p0_q0_p1
				vaddw.u8	q12, q10,d5	@// sum_p0_q0_q1
				vaddl.u8	q13, d2, d5	@// sum_p1_q1
				vaddl.u8	q14, d2, d3	@// sum_p1_p0
				vaddl.u8	q15, d5, d4 	@// sum_q1_q0

				vshl.i16	q6, q11, #1
				vaddw.u8	q6, q6, d1
				vaddw.u8	q6, q6, d5
				vqrshrun.s16	d8, q6, #3	@// p0_sf0= (sum_p0_q0_p1*2 + p2 + q1 +4) >>3@
				
				vaddw.u8	q6,q11, d1
				vqrshrun.s16	d9, q6, #2	@// p1_sf=(sum_p0_q0_p1 + p2 +2) >>2@
				
				vaddl.u8	q6, d0, d1
				vshl.i16	q6, q6, #1
				vadd.u16	q6, q6, q11
				vaddw.u8	q6, q6, d1
				vqrshrun.s16	d14,q6, #3	@// p2_sf=(sum_p0_q0_p1 + (p3+p2)*2 + p2 +4) >>3@

				vadd.u16	q6, q13,q14
				vqrshrun.s16	d15,q6, #2	@// p0_sf1=(sum_p1_q1 + sum_p1_p0+2) >>2@
			
				vshl.i16	q6, q12, #1
				vaddw.u8	q6, q6, d2
				vaddw.u8	q6, q6, d6
				vqrshrun.s16	d20,q6, #3	@// q0_sf0=(sum_p0_q0_q1*2 + p1 + q2 +4) >>3@
				vaddw.u8	q6, q12, d6
				vqrshrun.s16	d21, q6, #2	@// q1_sf=(sum_p0_q0_q1+q2 +2) >>2@	
				vaddl.u8	q6, d6, d7
				vshl.i16	q6, q6, #1
				vadd.u16	q6, q6, q12
				vaddw.u8	q6, q6, d6
				vqrshrun.s16	d22,q6, #3	@// q2_sf=(sum_p0_q0_q1 + (q3 +q2)*2 + q2 +4) >>3@

				vadd.u16	q6, q13, q15
				vqrshrun.s16	d23, q6, #2	@// q0_sf1=(sum_p1_q1+ sum_q1_q0+2) >>2@
				
				vand 		d16, d16, d11
				vand		d18, d18, d16	@// is_ap_lt_b && small_gap && is_luma
				vand 		d19, d19, d16	@// is_aq_lt_b && small_gap && is_luma
				

				vand		d26, d18, d17
				vmov		d27, d26
				vbsl		d26, d9, d2	@// p1sf or p1
				vbsl		d27, d14, d1	@// p2sf or p2
				vmov		d2, d26
				vmov		d1, d27

				vmov		d26, d18
				vbsl		d26, d8, d15	@// p0sf
				vmov		d27, d17
				vbsl		d27, d26, d3	@// p0sf or p0
				vmov		d3, d27

				vand		d26, d19, d17
				vmov		d27, d26
				vbsl		d26, d21, d5	@// q1sf or q1
				vbsl		d27, d22, d6	@// q2sf or q2
				vmov		d5, d26
				vmov		d6, d27

				vbsl		d19, d20, d23	@// q0sf
				vbsl		d17, d19, d4	@// q0sf or q0
				vmov		d4, d17
				b 		Trans_array

Weak_filter:
				mov		clip_1, #1
				vdup.8		d22, clip_1
				vadd.u8		d23, d10, d22	@//clip + 1
		
				vand 		d24, d22, d18
				vand		d25, d22, d19
				vadd.u8		d24, d24, d10
				vadd.u8		d24, d24, d25	@// clip + is_ap_lt_b + is_aq_lt_b
				vmov		d22, d11
				vbsl		d22, d24, d23	@// clip_c0
		
				vrhadd.u8	d20, d3, d4	@// (sum_p0_q0 + 1)/2
			
				vsubl.u8	q4, d4, d3	@// sub_q0_p0
				vsubl.u8	q6, d2, d5	@// sub_p1_q1

				vshl.i16	q4, q4, #2
				vadd.i16	q4, q4, q6
				vrshr.s16	q4, q4, #3	@// delta_0_tmp = (sub_q0_p0*4 + sub_p1_q1 +4)>>3@
		
				vshll.u8	q6, d2, #1
				vaddl.u8	q7, d20, d1
				vhsub.s16	q6, q7, q6	@// delta_p1_tmp = (p2 + (sum_p0_q0 +1)/2 - p1*2) >> 1@
	
				vshll.u8	q7, d5, #1
				vaddl.u8	q10, d20, d6
				vhsub.s16	q7, q10, q7	@// delta_q1_tmp = (q2 + (sum_p0_q0 +1)/2 - q1*2) >> 1@

				vmovl.u8	q11, d22
				vneg.s16	q12, q11
				vcgt.s16	q13, q4, q11
				vbsl		q13, q11, q4
				vcgt.s16	q10, q12, q4
				vbsl		q10, q12, q13	@// delta_0 = IClip(-clip_c0, clip_c0, delta_0_tmp)@

				vmovl.u8	q11, d10
				vneg.s16	q12, q11
				vcgt.s16	q13, q6, q11
				vbsl		q13, q11, q6
				vcgt.s16	q14, q12, q6
				vbsl		q14, q12, q13	@// delta_p1 = IClip(-clip, clip, delta_p1_tmp)@
	
				vcgt.s16	q13, q7, q11
				vbsl		q13, q11, q7
				vcgt.s16	q15, q12, q7
				vbsl		q15, q12, q13	@// delta_q1 = IClip(-clip, clip, delta_q1_tmp)@

				vmovl.u8	q11, d3
				vadd.s16	q11, q11, q10
				vqmovun.s16	d12, q11	@// p0_wf = IClip(0,255, p0+delta_0)@

				vmovl.u8	q11, d4
				vsub.s16	q11, q11, q10
				vqmovun.s16	d13, q11	@// q0_wf = IClip(0,255, q0-delta_0)@
				vmovl.u8	q11, d2
				vadd.s16	q11, q11, q14
				vqmovun.s16	d14, q11	@// p1_wf = p1 +delta_p1@
		
				vmovl.u8	q11, d5
				vadd.s16	q11, q11, q15
				vqmovun.s16	d15, q11	@// q1_wf = q1 +delta_q1@

				vmov		d16, d17
				vbsl		d16, d12, d3
				vmov		d3, d16		@// p0
		
				vmov		d16, d17
				vbsl		d16, d13, d4
				vmov		d4, d16		@// q0

				vand		d18, d18, d17
				vand		d18, d18, d11
				vbsl		d18, d14, d2
				vmov		d2, d18		@// p1

				vand		d19, d19, d17
				vand		d19, d19, d11
				vbsl		d19, d15, d5
				vmov		d5, d19		@// q1

Trans_array:
				@// filtering end
				ldrsh		direction, [dbk_para, #0xa]
				ldrsh		line_width, [dbk_para, #2]
				cmp		direction, #0
				bne		Trans_ver

Trans_hor:
				@// transpose while hor filter
				vtrn.8		d0, d1
				vtrn.8		d2, d3
				vtrn.8		d4, d5
				vtrn.8		d6, d7

				vtrn.16		q0, q1
				vtrn.16		q2, q3

				vtrn.32		q0, q2
				vtrn.32		q1, q3

				b		Store_pixel

Trans_ver:			
				@// transpose while ver filter
				vtrn.8		d1, d0
				vtrn.8		d3, d2
				vtrn.8		d5, d4
				vtrn.8		d7, d6
                                                      
				vtrn.16		q1, q0
				vtrn.16		q3, q2
                                                      
				vtrn.32		q2, q0
				vtrn.32		q3, q1


Store_pixel:
				@// store
				vst1.8		{d7}, [dst_block_ptr], line_width
				vst1.8		{d6}, [dst_block_ptr], line_width
				vst1.8		{d5}, [dst_block_ptr], line_width
				vst1.8		{d4}, [dst_block_ptr], line_width
				vst1.8		{d3}, [dst_block_ptr], line_width
				vst1.8		{d2}, [dst_block_ptr], line_width
				vst1.8		{d1}, [dst_block_ptr], line_width
				vst1.8		{d0}, [dst_block_ptr], line_width


				pop		{r4 - r11, pc}
				@ENDFUNC

				.end
