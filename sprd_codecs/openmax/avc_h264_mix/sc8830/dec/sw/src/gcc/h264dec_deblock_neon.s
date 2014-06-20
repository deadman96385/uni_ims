@/************register map**************/
slice_ptr       .req		r0
pix             .req		r1
dbk_para        .req		r2
bS              .req		r3

stride          .req		r4
alpha           .req		r5
beta            .req		r6
index           .req		r7
pix_adr         .req		r8

qp              .req		r8
alpha_tbl       .req		r9
beta_tbl        .req		r10
tc0_tbl         .req		r11

dir             .req		r8
tc0             .req		r8
bs_0            .req		r9
bs_1            .req		r10
bs_2		.req		r12
bs_3		.req		r14




.text
.arm

@//static void filter_mb_edgec(DEC_SLICE_T *curr_slice_ptr, uint8 *pix, DEBLK_PARAS_T * dbk_para, int bS[4])
filter_mb_edgec_neon:
@FUNCTION
.global	filter_mb_edgec_neon
.extern	alpha_table
.extern	beta_table
.extern	tc0_table

push 	{r4 - r12, r14}

ldr    	alpha_tbl, = alpha_table
                     ldr    	beta_tbl, = beta_table

                             ldrb  	r8, [dbk_para, #3]
                             add	r8, r8, #1
                             ldrb 	qp, [dbk_para, r8]

                             ldr	stride, [dbk_para, #4]
                             mov	stride, stride, lsr #1

                             ldrsb	index, [slice_ptr, #7]
                             add  	index, qp, index
                             cmp  	index, #0
                             movlt	index, #0
                             cmp  	index, #51
                             movgt	index, #51
                             ldr    	beta, [beta_tbl, index, lsl #2]         @//beta

                             ldrsb	index, [slice_ptr, #6]
                             add  	index, qp, index
                             cmp  	index, #0
                             movlt	index, #0
                             cmp  	index, #51
                             movgt	index, #51
                             ldr    	alpha, [alpha_tbl, index, lsl #2]	    @//alpha

                             ldr	bs_0, [bS]
                             ldr	bs_1, [bS, #4]
                             ldr	bs_2, [bS, #8]
                             ldr	bs_3, [bS, #12]
                             add	r0, bs_0, bs_1
                             add	r0, r0, bs_2
                             add	r0, r0, bs_3
                             cmp	r0, #0
                             beq	End_C

                             @//load pixes 8x8
                             mov   	pix_adr, pix
                             vld1.8	{d0}, [pix_adr], stride
                             vld1.8	{d1}, [pix_adr], stride
                             vld1.8	{d2}, [pix_adr], stride
                             vld1.8	{d3}, [pix_adr], stride
                             vld1.8	{d4}, [pix_adr], stride
                             vld1.8	{d5}, [pix_adr], stride
                             vld1.8	{d6}, [pix_adr], stride
                             vld1.8	{d7}, [pix_adr], stride

                             ldr    	dir, [dbk_para, #8]
                             cmp 	dir, #0
                             bne  	Filter_C

                             vtrn.8	d0, d1
                             vtrn.8	d2, d3
                             vtrn.8	d4, d5
                             vtrn.8	d6, d7

                             vtrn.16	q0, q1
                             vtrn.16	q2, q3

                             vtrn.32	q0, q2
                             vtrn.32	q1, q3

                             @//(d0, d1, d2, d3, d4, d5, d6, d7)
                             @//stand for
                             @//(p3, p2, p1, p0, q0, q1, q2, q3)

                             Filter_C:

                             vdup.8		d8, alpha
                             vdup.8		d9, beta

                             cmp   		bs_0, #0
                             vmoveq.16	d8[0], bs_0
                             cmp   		bs_1, #0
                             vmoveq.16	d8[1], bs_1
                             cmp   		bs_2, #0
                             vmoveq.16	d8[2], bs_2
                             cmp   		bs_3, #0
                             vmoveq.16	d8[3], bs_3

                             Cal_Cond_C:

                             vabd.u8		d12, d3, d4 	@// abs_p0_q0
                             vabd.u8		d13, d2, d3	@// abs_p1_p0
                             vabd.u8		d14, d5, d4	@// abs_q1_q0

                             vcgt.u8		d17, d8, d12	@// (abs_p0_q0 < alpha)
                             vcgt.u8		d18, d9, d13	@// (abs_p1_p0 < beta)
                             vcgt.u8		d19, d9, d14	@// (abs_q1_q0 < beta)

                             vand		d17, d17, d18
                             vand		d17, d17, d19	@// filt_ena

                             cmp		bs_0, #4
                             bne		Weak_filter_C

                             Strong_filter_C:

                             vaddl.u8	q10, d3, d5	@// sum_p0_q1
                             vaddl.u8	q11, d2, d2	@// sum_p1_p1
                             vaddl.u8	q12, d2, d4	@// sum_p1_q0
                             vaddl.u8	q13, d5, d5 	@// sum_q1_q1

                             vadd.u16	q4, q10, q11
                             vadd.u16	q5, q12, q13

                             vrshrn.u16	d12, q4, #2	@// p0_sf=(2*p1 + p0 + q1 + 2) >> 2
                             vrshrn.u16	d13, q5, #2	@// q0_sf=(2*q1 + q0 + p1 + 2) >> 2

                             vmov.u8		d16, d17
                             vbsl		d16, d12, d3
                             vbsl		d17, d13, d4
                             vmov		d3, d16
                             vmov		d4, d17

                             b 		Trans_array_C

                             Weak_filter_C:

                             mov   		r0, #3
                             ldr    		tc0_tbl, = tc0_table

                                     cmp  		bs_0, #0
                                     subgt		bs_0, bs_0, #1
                                     mlagt		tc0, index, r0, bs_0
                                     ldrgt		tc0, [tc0_tbl, tc0, lsl #2]
                                     addgt		tc0, tc0, #1
                                     moveq		tc0, #0
                                     vmov.u8		d8[0], tc0
                                     vmov.u8		d8[1], tc0

                                     cmp  		bs_1, #0
                                     subgt		bs_1, bs_1, #1
                                     mlagt		tc0, index, r0, bs_1
                                     ldrgt		tc0, [tc0_tbl, tc0, lsl #2]
                                     addgt		tc0, tc0, #1
                                     moveq		tc0, #0
                                     vmov.u8		d8[2], tc0
                                     vmov.u8		d8[3], tc0

                                     cmp  		bs_2, #0
                                     subgt		bs_2, bs_2, #1
                                     mlagt		tc0, index, r0, bs_2
                                     ldrgt		tc0, [tc0_tbl, tc0, lsl #2]
                                     addgt		tc0, tc0, #1
                                     moveq		tc0, #0
                                     vmov.u8		d8[4], tc0
                                     vmov.u8		d8[5], tc0

                                     cmp  		bs_3, #0
                                     subgt		bs_3, bs_3, #1
                                     mlagt		tc0, index, r0, bs_3
                                     ldrgt		tc0, [tc0_tbl, tc0, lsl #2]
                                     addgt		tc0, tc0, #1
                                     moveq		tc0, #0
                                     vmov.u8		d8[6], tc0
                                     vmov.u8		d8[7], tc0

                                     vmov.u8		d10, #0
                                     vsub.u8		d9, d10, d8		@//-tc0

                                     vsubl.u8  	q5, d4, d3		@// sub_q0_p0
                                     vsubl.u8  	q6, d2, d5		@// sub_p1_q1

                                     vshl.s16   	q5, q5, #2		@//((q0 - p0) << 2 )
                                     vadd.s16  	q7, q5, q6
                                     vrshrn.s16 	d10, q7, #3		@//(((q0 - p0) << 2) + (p1 -q1) + 4) >> 3
                                     vmin.s8		d10, d10, d8
                                     vmax.s8		d10, d10, d9		@//i_delta

                                     vmov.u8		d8, #0
                                     vmov.u8		d9, #255		@//????????????????
                                     vadd.s8		d12, d3, d10		@//p0 + i_delta
                                     vsub.s8		d13, d4, d10		@//q0 - i_delta

                                     vmin.u8		d12, d12, d9
                                     vmax.u8		d12, d12, d8		@//p0'
                                     vmin.u8		d13, d13, d9
                                     vmax.u8		d13, d13, d8		@//q0'

                                     vmov		d16, d17
                                     vbsl		d16, d12, d3
                                     vbsl		d17, d13, d4
                                     vmov		d3, d16
                                     vmov		d4, d17

                                     Trans_array_C:
                                     @// filtering end
                                     ldr   	dir, [dbk_para, #8]
                                     cmp   	dir, #0
                                     bne   	Store_pixel_C

                                     Trans_hor_C:
                                     @// transpose while hor filter
                                     vtrn.8		d0, d1
                                     vtrn.8		d2, d3
                                     vtrn.8		d4, d5
                                     vtrn.8		d6, d7

                                     vtrn.16		q0, q1
                                     vtrn.16		q2, q3

                                     vtrn.32		q0, q2
                                     vtrn.32		q1, q3

                                     b		Store_pixel_C

                                     Trans_ver_C:
                                     @// transpose while ver filter
                                     vtrn.8		d1, d0
                                     vtrn.8		d3, d2
                                     vtrn.8		d5, d4
                                     vtrn.8		d7, d6

                                     vtrn.16		q1, q0
                                     vtrn.16		q3, q2

                                     vtrn.32		q2, q0
                                     vtrn.32		q3, q1


                                     Store_pixel_C:

                                     @//store pixes 8x8
                                     @//sub		pix_adr, pix, #4
                                     mov   	pix_adr, pix
                                     vst1.8	{d0}, [pix_adr], stride
                                     vst1.8	{d1}, [pix_adr], stride
                                     vst1.8	{d2}, [pix_adr], stride
                                     vst1.8	{d3}, [pix_adr], stride
                                     vst1.8	{d4}, [pix_adr], stride
                                     vst1.8	{d5}, [pix_adr], stride
                                     vst1.8	{d6}, [pix_adr], stride
                                     vst1.8	{d7}, [pix_adr], stride

                                     End_C:
                                     pop		{r4 - r12, pc}

                                     @ENDFUNC

                                     @//static void filter_mb_edgev(DEC_SLICE_T *curr_slice_ptr, uint8 *pix, DEBLK_PARAS_T * dbk_para, int bS[2])
                                     filter_mb_edge_neon:	@FUNCTION
                                     .global	filter_mb_edge_neon
                                     .extern	alpha_table
                                     .extern	beta_table
                                     .extern	tc0_table

                                     push 	{r4 - r11, r14}

                                     ldr    	alpha_tbl, = alpha_table
                                             ldr    	beta_tbl, = beta_table
                                                     ldrb  	qp, [dbk_para]
                                                     ldr    	stride, [dbk_para, #4]

                                                     ldrsb	index, [slice_ptr, #7]
                                                     add  	index, qp, index
                                                     cmp  	index, #0
                                                     movlt	index, #0
                                                     cmp  	index, #51
                                                     movgt	index, #51
                                                     ldr    	beta, [beta_tbl, index, lsl #2]         @//beta

                                                     ldrsb	index, [slice_ptr, #6]
                                                     add  	index, qp, index
                                                     cmp  	index, #0
                                                     movlt	index, #0
                                                     cmp  	index, #51
                                                     movgt	index, #51
                                                     ldr    	alpha, [alpha_tbl, index, lsl #2]	    @//alpha

                                                     ldr	bs_0, [bS]
                                                     ldr	bs_1, [bS, #4]
                                                     add	r0, bs_0, bs_1
                                                     cmp	r0, #0
                                                     beq	End

                                                     @//load pixes 8x8
                                                     mov   	pix_adr, pix
                                                     vld1.8	{d0}, [pix_adr], stride
                                                     vld1.8	{d1}, [pix_adr], stride
                                                     vld1.8	{d2}, [pix_adr], stride
                                                     vld1.8	{d3}, [pix_adr], stride
                                                     vld1.8	{d4}, [pix_adr], stride
                                                     vld1.8	{d5}, [pix_adr], stride
                                                     vld1.8	{d6}, [pix_adr], stride
                                                     vld1.8	{d7}, [pix_adr], stride

                                                     ldr    	dir, [dbk_para, #8]
                                                     cmp 	dir, #0
                                                     bne  	Filter

                                                     vtrn.8	d0, d1
                                                     vtrn.8	d2, d3
                                                     vtrn.8	d4, d5
                                                     vtrn.8	d6, d7

                                                     vtrn.16	q0, q1
                                                     vtrn.16	q2, q3

                                                     vtrn.32	q0, q2
                                                     vtrn.32	q1, q3

                                                     @//(d0, d1, d2, d3, d4, d5, d6, d7)
                                                     @//stand for
                                                     @//(p3, p2, p1, p0, q0, q1, q2, q3)

                                                     Filter:

                                                     vdup.8		d8, alpha
                                                     vdup.8		d9, beta

                                                     mov   		r0, #0
                                                     vdup.8		d10, r0
                                                     cmp   		bs_0, #0
                                                     beq		bs0_alpha_0
                                                     cmp   		bs_1, #0
                                                     beq		bs1_alpha_0

                                                     b       	Cal_Cond

                                                     bs0_alpha_0:
                                                     vtrn.32		d10, d8
                                                     b       	Cal_Cond

                                                     bs1_alpha_0:
                                                     vtrn.32		d8, d10
                                                     b      		Cal_Cond

                                                     Cal_Cond:

                                                     vabd.u8		d12, d3, d4 	@// abs_p0_q0
                                                     vabd.u8		d13, d2, d3	@// abs_p1_p0
                                                     vabd.u8		d14, d5, d4	@// abs_q1_q0
                                                     vabd.u8		d15, d1, d3	@// abs_p2_p0
                                                     vabd.u8		d16, d6, d4	@// abs_q2_q0

                                                     vcgt.u8		d17, d8, d12	@// (abs_p0_q0 < alpha)
                                                     vcgt.u8		d18, d9, d13	@// (abs_p1_p0 < beta)
                                                     vcgt.u8		d19, d9, d14	@// (abs_q1_q0 < beta)

                                                     vand		d17, d17, d18
                                                     vand		d17, d17, d19	@// filt_ena
                                                     vcgt.u8		d18, d9,  d15	@// is_ap_lt_b
                                                     vcgt.u8		d19, d9,  d16	@// is_aq_lt_b

                                                     cmp		bs_0, #4
                                                     bne		Weak_filter

                                                     Strong_filter:

                                                     lsr    		r0, alpha, #2
                                                     add   		r0, r0, #2
                                                     vdup.8		d8, r0
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

                                                     @//vand 	d16, d16, d11
                                                     vand		d18, d18, d16	@// is_ap_lt_b && small_gap && is_luma
                                                     vand 		d19, d19, d16	@// is_aq_lt_b && small_gap && is_luma

                                                     vand		d26, d18, d17
                                                     vmov		d27, d26
                                                     vbsl		d26, d9, d2		@// p1sf or p1
                                                     vbsl		d27, d14, d1		@// p2sf or p2
                                                     vmov		d2, d26
                                                     vmov		d1, d27

                                                     vmov		d26, d18
                                                     vbsl		d26, d8, d15		@// p0sf
                                                     vmov		d27, d17
                                                     vbsl		d27, d26, d3		@// p0sf or p0
                                                     vmov		d3, d27

                                                     vand		d26, d19, d17
                                                     vmov		d27, d26
                                                     vbsl		d26, d21, d5		@// q1sf or q1
                                                     vbsl		d27, d22, d6		@// q2sf or q2
                                                     vmov		d5, d26
                                                     vmov		d6, d27

                                                     vbsl		d19, d20, d23	@// q0sf
                                                     vbsl		d17, d19, d4		@// q0sf or q0
                                                     vmov		d4, d17
                                                     b 		Trans_array

                                                     Weak_filter:

                                                     mov   		r0, #3
                                                     ldr    		tc0_tbl, = tc0_table

                                                             cmp  		bs_0, #0
                                                             subgt		bs_0, bs_0, #1
                                                             mlagt		tc0, index, r0, bs_0
                                                             ldrgt			tc0, [tc0_tbl, tc0, lsl #2]
                                                             moveq		tc0, #0
                                                             vdup.8		d8, tc0

                                                             cmp  		bs_1, #0
                                                             subgt		bs_1, bs_1, #1
                                                             mlagt		tc0, index, r0, bs_1
                                                             ldrgt		tc0, [tc0_tbl, tc0, lsl #2]
                                                             moveq		tc0, #0
                                                             vdup.8		d9, tc0

                                                             vtrn.32		d8, d9			@//tc0

                                                             vmov.u8		d20, #0
                                                             vsub.u8		d9, d20, d8		@//-tc0

                                                             vadd.s8   	d21, d19, d18
                                                             vsub.s8		d10, d8, d21		@//tc
                                                             vsub.u8		d11, d20, d10	@//-tc

                                                             vsubl.u8  	q6, d4, d3		@// sub_q0_p0
                                                             vsubl.u8  	q7, d2, d5		@// sub_p1_q1
                                                             vrhadd.u8	d16, d3, d4		@// (sum_p0_q0 + 1)/2

                                                             vshll.u8   	q12, d2, #1		@//( p1 << 1 )
                                                             vaddl.u8  	q14, d16, d1
                                                             vsub.u16 	q14, q14, q12
                                                             vshrn.s16 	d20, q14, #1		@//( p2 + ( ( p0 + q0 + 1 ) >> 1 ) - ( p1 << 1 ) ) >> 1
                                                             vmin.s8		d20, d20, d8
                                                             vmax.s8		d20, d20, d9
                                                             vadd.s8		d20, d20, d2		@//p1'

                                                             vshll.u8   	q14, d5, #1		@//( q1 << 1 )
                                                             vaddl.u8  	q15, d16, d6
                                                             vsub.u16 	q15, q15, q14
                                                             vshrn.s16 	d21, q15, #1		@//( q2 + ( ( p0 + q0 + 1 ) >> 1 ) - ( q1 << 1 ) ) >> 1
                                                             vmin.s8		d21, d21, d8
                                                             vmax.s8		d21, d21, d9
                                                             vadd.s8		d21, d21, d5		@//q1'   ????????????????????

                                                             vshl.s16	q14, q6, #2		@//((q0 - p0 ) << 2)
                                                             vadd.s16	q14, q14, q7
                                                             vrshrn.s16	d24, q14, #3
                                                             vmin.s8		d24, d24, d10
                                                             vmax.s8		d24, d24, d11	@//i_delta

                                                             vmov.u8		d8, #0
                                                             vmov.u8		d9, #255		@//????????????????
                                                             vadd.s8		d22, d3, d24		@//p0 + i_delta
                                                             vsub.s8		d23, d4, d24		@//q0 - i_delta

                                                             vmin.u8		d22, d22, d9
                                                             vmax.u8		d22, d22, d8		@//p0'
                                                             vmin.u8		d23, d23, d9
                                                             vmax.u8		d23, d23, d8		@//q0'

                                                             vand		d10, d17, d18
                                                             vbsl		d10, d20, d2
                                                             vmov		d2, d10			@//p1

                                                             vand		d10, d17, d19
                                                             vbsl		d10, d21, d5
                                                             vmov		d5, d10			@//q1

                                                             vmov		d10, d17
                                                             vbsl		d10, d22, d3
                                                             vmov		d3, d10			@//p0

                                                             vbsl		d17, d23, d4
                                                             vmov		d4, d17			@//q0

                                                             Trans_array:
                                                             @// filtering end
                                                             ldr   	dir, [dbk_para, #8]
                                                             cmp   	dir, #0
                                                             bne   	Store_pixel

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

                                                             @//store pixes 8x8
                                                             @//sub		pix_adr, pix, #4
                                                             mov   	pix_adr, pix
                                                             vst1.8	{d0}, [pix_adr], stride
                                                             vst1.8	{d1}, [pix_adr], stride
                                                             vst1.8	{d2}, [pix_adr], stride
                                                             vst1.8	{d3}, [pix_adr], stride
                                                             vst1.8	{d4}, [pix_adr], stride
                                                             vst1.8	{d5}, [pix_adr], stride
                                                             vst1.8	{d6}, [pix_adr], stride
                                                             vst1.8	{d7}, [pix_adr], stride

                                                             End:
                                                             pop		{r4 - r11, pc}

                                                             @ENDFUNC
                                                             .end
