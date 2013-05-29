
@//this asm file is for isqt

			.text
					
			.arm

			
@//void itrans_lumaDC (int16 * DCCoeff, int16 * pCoeffIq, int32 qp)
@/************register map begin**************/
DCCoeff		.req			r0
pCoeffIq		.req			r1
qp		.req			r2

img_ptr		.req			r3
quantizer	.req			r3

@/************register map end**************/
			.macro	TRANS4X4_DC_HOR
			vadd.s16	d14, d0, d2	@//n0 = m0 + m2
			vsub.s16	d15, d0, d2	@//n1 = m0 - m2
			vsub.s16	d16, d1, d3	@//n2 = m1 - m3
			vadd.s16	d17, d1, d3	@//n3 = m1 + m3

			vadd.s16	d0, d14, d17	@//n0+n3
			vadd.s16	d1, d15, d16	@//n1+n2
			vsub.s16	d2,	d15, d16	@//n1-n2
			vsub.s16	d3, d14, d17	@//n0-n3
			.endm
			
			.macro	TRANS4X4_DC_VER
			vadd.s16	d14, d0, d2	@//n0 = m0 + m2
			vsub.s16	d15, d0, d2	@//n1 = m0 - m2
			vsub.s16	d16, d1, d3	@//n2 = m1 - m3
			vadd.s16	d17, d1, d3	@//n3 = m1 + m3

			vaddl.s16	q0, d14, d17	@//n0+n3
			vaddl.s16	q1, d15, d16	@//n1+n2
			vsubl.s16	q12,	d15, d16	@//n1-n2
			vsubl.s16	q3, d14, d17	@//n0-n3
			.endm
			
itrans_lumaDC:	@FUNCTION
			.extern	g_image_ptr
			.global	itrans_lumaDC
				
			push		{r14}

			ldr		r14, =g_image_ptr
			ldr		img_ptr, [r14, #0]
			add		r2, img_ptr, qp, lsl #6
			ldr		quantizer,[r2, #0x388]
			vmov.32		d4[0], quantizer

			vld4.s16	{d0, d1, d2, d3}, [DCCoeff]	@//load all 16 DC
				
			TRANS4X4_DC_HOR		@/*horizontal inverse transform*/

			vtrn.16		d0, d1	@//transpose
			vtrn.16		d2, d3
			vtrn.32		q0, q1
			
			TRANS4X4_DC_VER		@/*vertical inverse transform*/

		@//	vst1.s16	{d0, d1, d2, d3}, [DCCoeff_f0]	@//store
			
			vmul.s32	q4, q0, d4[0]
			vmul.s32	q5, q1, d4[0]
			vmul.s32	q6, q12, d4[0]
			vmul.s32	q7, q3, d4[0]

			vrshrn.s32	d0, q4, #8
			vrshrn.s32	d1, q5, #8
			vrshrn.s32	d2, q6, #8
			vrshrn.s32	d3, q7, #8

			vtrn.32		d0, d1
			vtrn.32		d2, d3

			mov			r14, #(16*2)
			vst1.16		d0[0], [pCoeffIq], r14
			vst1.16		d0[1], [pCoeffIq], r14
			vst1.16		d0[2], [pCoeffIq], r14
			vst1.16		d0[3], [pCoeffIq], r14

			vst1.16		d1[0], [pCoeffIq], r14
			vst1.16		d1[1], [pCoeffIq], r14
			vst1.16		d1[2], [pCoeffIq], r14
			vst1.16		d1[3], [pCoeffIq], r14

			vst1.16		d2[0], [pCoeffIq], r14
			vst1.16		d2[1], [pCoeffIq], r14
			vst1.16		d2[2], [pCoeffIq], r14
			vst1.16		d2[3], [pCoeffIq], r14

			vst1.16		d3[0], [pCoeffIq], r14
			vst1.16		d3[1], [pCoeffIq], r14
			vst1.16		d3[2], [pCoeffIq], r14
			vst1.16		d3[3], [pCoeffIq], r14

			pop		{ pc}
			@ENDFUNC

@//void itrans_4x4 (int16 *coff, uint8 *pred, int32 width_p, uint8 *rec, int32 width_r)
@/************register map begin**************/
coeff_f1		.req		r0
pred_f1		.req		r1
width_f1		.req		r2
rec_f1		.req		r3
witdh_r_f1	.req		r4
@/************register map end**************/

			.macro	TRANS4X4
			vadd.s16	d4, d0, d2	@//n0 = m0 + m2
			vsub.s16	d5, d0, d2	@//n1 = m0 - m2
			vneg.s16	d6, d3		@//n2 = (m1>>1) - m3
			vsra.s16	d6, d1, #1
			vmov.s16	d7, d1		@//n3 = m1 + (m3 >> 1)
			vsra.s16	d7, d3, #1	

			vadd.s16	d0, d4, d7	@//n0+n3
			vadd.s16	d1, d5, d6	@//n1+n2
			vsub.s16	d2, d5, d6	@//n1-n2
			vsub.s16	d3, d4, d7	@//n0-n3
			.endm

itrans_4x4:		@FUNCTION
			.global	itrans_4x4

			push			{r4, r14}
			ldr			witdh_r_f1, [sp, #8]

			vld4.s16	{d0, d1, d2, d3}, [coeff_f1]	@//load all 16 coeff

			TRANS4X4	;/*horizontal inverse transform*/
			
			vtrn.16		d0, d1		@//transpose
			vtrn.16		d2, d3
			vtrn.32		q0, q1
			
			TRANS4X4	@/*vertical inverse transform*/

			vrshr.s16	d4, d0, #6	@//(x+32)>>6
			vrshr.s16	d5, d1, #6
			vrshr.s16	d6, d2, #6
			vrshr.s16	d7, d3, #6

			mov		r14, pred_f1		@//load 8x4 pred
			vld1.8	d0, [r14], width_f1
			vld1.8	d1, [r14], width_f1
			vld1.8	d2, [r14], width_f1
			vld1.8	d3, [r14], width_f1

			vtrn.32		d0, d1			@//transpos, removd invalid data
			vtrn.32 	d2, d3

			vaddw.u8	q5, q2, d0		@/pred+coeff
			vaddw.u8	q6, q3, d2

			vqmovun.s16	d0, q5			@// int16->uint8, with q	
			vqmovun.s16	d2, q6

			vst1.32		d0[0], [rec_f1], witdh_r_f1	@//save final data
			vst1.32		d0[1], [rec_f1], witdh_r_f1

			vst1.32		d2[0], [rec_f1], witdh_r_f1
			vst1.32		d2[1], [rec_f1], witdh_r_f1

			pop			{r4, pc}

			@ENDFUNC

@//void itrans_8x8 (int16 *coff, uint8 *pred, int32 width_p, uint8 *rec, int32 width_r)
@/************register map begin**************/
coeff_f2		.req		r0
pred_f2		.req		r1
width_p_f2	.req		r2
rec_f2		.req		r3
width_r_f2	.req		r4
@/************register map end**************/

			.macro	LOAD_ALL_COEFFICIENT	@//64
			vld1.16		{d0, d1, d2, d3}, [coeff_f2]!		@//d0, d1
			vld1.16		{d4, d5, d6, d7}, [coeff_f2]!		@//d2, d3
			vld1.16		{d8, d9, d10, d11}, [coeff_f2]!		@//d4, d5
			vld1.16		{d12, d13, d14, d15}, [coeff_f2]!	@//x6, x7
			sub			coeff_f2, coeff_f2, #(64*2)
			.endm

			.macro	TRANS4X4BLK	d_0, d_1, d_2, d_3
			vtrn.16		\d_0, \d_1
			vtrn.16		\d_2, \d_3
			vtrn.32		\d_0, \d_2
			vtrn.32		\d_1, \d_3
			.endm

			.macro	TRANS8X8	c0, c1, c2, c3, c4, c5, c6, c7, c8, c9, c10, c11, c12, c13, c14, c15 
			vadd.s16		\c8, \c0, \c4	@//e0 = d0 + d4;
			vsub.s16		\c10, \c0, \c4	@//e2 = d0 - d4;
			vsub.s16		\c0, \c5, \c3	@//e1 = -d3 + d5 - d7 -(d7>>1);
			vshr.s16		\c4, \c7, #1
			vsub.s16		\c0, \c0, \c7
			vsub.s16		\c9, \c0, \c4
			vadd.s16		\c0, \c1, \c7	@//e3 = d1 + d7 -d3 - (d3>>1);
			vshr.s16		\c4, \c3, #1
			vsub.s16		\c0, \c0, \c3
			vsub.s16		\c11, \c0, \c4
			vshr.s16		\c0, \c2, #1	@//e4 = (d2>>1) - d6;
			vsub.s16		\c12, \c0, \c6
			vshr.s16		\c0, \c6, #1	@//e6 = d2 + (d6>>1);
			vadd.s16		\c14, \c0, \c2			
			vsub.s16		\c0, \c7, \c1	@//e5 = -d1 + d7 + d5 + (d5>>1);	
			vshr.s16		\c4, \c5, #1
			vadd.s16		\c0, \c0, \c5
			vadd.s16		\c13, \c0, \c4
			vadd.s16		\c0, \c3, \c5	@//e7 = d3 + d5 + d1 + (d1>>1);
			vshr.s16		\c4, \c1, #1
			vadd.s16		\c0, \c0, \c1
			vadd.s16		\c15, \c0, \c4

			vadd.s16		\c0, \c8, \c14	@//f0 = e0 + e6;
			vsub.s16		\c6, \c8, \c14	@//f6 = e0 - e6;
			vadd.s16		\c2, \c10, \c12 @//f2 = e2 + e4;
			vsub.s16		\c4, \c10, \c12	@//f4 = e2 - e4;
			vshr.s16		\c8, \c15, #2	@//f1 = e1 + (e7>>2);
			vadd.s16		\c1, \c9, \c8
			vshr.s16		\c8, \c9, #2	@//f7 = e7 -(e1>>2);
			vsub.s16		\c7, \c15, \c8
			vshr.s16		\c8, \c13, #2	@//f3 = e3 + (e5>>2);
			vadd.s16		\c3, \c11, \c8
			vshr.s16		\c8, \c11, #2	@//f5 = (e3>>2) - e5;
			vsub.s16		\c5, \c8, \c13	

			vadd.s16		\c8, \c0, \c7	@//tmp[0] = f0 + f7;
			vsub.s16		\c15, \c0, \c7	@//tmp[7] = f0 - f7;
			vadd.s16		\c9, \c2, \c5	@//tmp[1] = f2 + f5;
			vsub.s16		\c14, \c2, \c5	@//tmp[6] = f2 - f5;
			vadd.s16		\c10, \c4, \c3	@//tmp[2] = f4 + f3;
			vsub.s16		\c13, \c4, \c3	@//tmp[5] = f4 - f3;
			vadd.s16		\c11, \c6, \c1	@//tmp[3] = f6 + f1;
			vsub.s16		\c12, \c6, \c1	@//tmp[4] = f6 - f1;

			.endm

			.macro	STORE_ONE_LINE	c0
			vld1.8		{d16}, [pred_f2], width_p_f2
			vrshr.s16		q14, \c0, #6	@//(x+32)>>6
			vaddw.u8		q15, q14, d16	@//pred+coeff
			vqmovun.s16	d16, q15		@// int16->uint8, with q
			vst1.8		{d16}, [rec_f2], width_r_f2
			.endm

itrans_8x8:		@FUNCTION
			.global	itrans_8x8

			push		{r4, r14}
			ldr		witdh_r_f1, [sp, #8]

			LOAD_ALL_COEFFICIENT
			
		@/*horizontal inverse transform*/
			TRANS8X8	q0, q1, q2, q3, q4, q5, q6, q7, q8, q9, q10, q11, q12, q13, q14, q15

			TRANS4X4BLK	d16, d18, d20, d22
			TRANS4X4BLK	d17, d19, d21, d23
			TRANS4X4BLK	d24, d26, d28, d30				
			TRANS4X4BLK	d25, d27, d29, d31

			@//swp block4x4 2, 3
			vswp	d17, d24
			vswp	d19, d26
			vswp	d21, d28
			vswp	d23, d30
			
		@/*vertical inverse transform*/
			TRANS8X8	q8, q9, q10, q11, q12, q13, q14, q15, q0, q1, q2, q3, q4, q5, q6, q7 

		@//store
			STORE_ONE_LINE	q0
			STORE_ONE_LINE	q1
			STORE_ONE_LINE	q2
			STORE_ONE_LINE	q3
			STORE_ONE_LINE	q4
			STORE_ONE_LINE	q5
			STORE_ONE_LINE	q6
			STORE_ONE_LINE	q7			

			pop		{r4, pc}

			@ENDFUNC	
			
			.end
				
