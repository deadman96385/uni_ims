
@//this asm file is for luma16xN motion compensation

@/************register map begin**************/
img_ptr		.req			r0
pRefFrame	.req			r1
pPred		.req			r2
N			.req			r3
width		.req			r10

i			.req			r3
ii			.req			r7
j			.req			r5

offset		.req			r4
pPred_inc		.req			r5
pPred_inc_1	.req			r8

dx			.req			r4
dy			.req			r4
				
pRefVerFilter	.req		r5
pRefHorFilter	.req		r5
			
pPredTemp	.req			r6	@//pPred
pHalfPix		.req			r4

pHalfPixTmp 	.req			r6
pRefTmp 		.req			r9

pHalfPixTmp_inc 	.req		r8
pHalfPix_inc		.req		r8
@/************register map end**************/
					
			.text					
			.arm

@//void MC_luma16xN_dx0dy0 (H264DecContext *img_ptr, uint8 * pRefFrame, uint8 * pPred, int32 N)
MC_luma16xN_dx0dy0:	@FUNCTION
			.global	MC_luma16xN_dx0dy0
				
			push		{r10, r14}

			ldrh		width, [img_ptr, #20]
				
MC_luma16xN_dx0dy0_VER_LOOP:
			vld1.8	{d0, d1}, [pRefFrame], width	
			vld1.8	{d2, d3}, [pRefFrame], width	
			vst1.8	{d0, d1}, [pPred, :128]!
			vld1.8	{d4, d5}, [pRefFrame], width	
			vst1.8	{d2, d3}, [pPred, :128]!
			vld1.8	{d6, d7}, [pRefFrame], width				
			vst1.8	{d4, d5}, [pPred, :128]!
			vst1.8	{d6, d7}, [pPred, :128]!

			subs		i, i, #4
			bne		MC_luma16xN_dx0dy0_VER_LOOP

			pop		{r10, pc}
			@ENDFUNC

@//void MC_luma16xN_dxMdy0 (H264DecContext *img_ptr, uint8 * pRefFrame, uint8 * pPred, int32 N)
MC_luma16xN_dxMdy0:	@FUNCTION
			@.extern	g_refPosx
			.global	MC_luma16xN_dxMdy0
				
			push	{r4, r10, r14}
				
			ldr		r14, [img_ptr, #52]	@//g_refPosx
			ldrh		width, [img_ptr, #20]
			and		r14, r14, #3
			cmp		r14, #1
			movne	offset, #3
			moveq	offset, #2

			ldr		r14, =MC_coefficient_neon
			sub		pRefFrame, pRefFrame, #2
			vld1.16	{d4}, [r14]

LUMA16XN_dxM_VER_LOOP:
			add 		r14, pRefFrame, offset
			vld1.8		{d30, d31}, [r14]
			
			add			r14, pRefFrame, #16
			vld1.8		{d12, d13}, [r14]				@//v128_next
			vld1.8		{d0, d1}, [pRefFrame], width	@//v128[0]

			vext.8		q5, q0, q6, #5			@//v128[5]
			vaddl.u8	q7, d0, d10			@//v05_l8
			vaddl.u8	q8, d1, d11			@//v05_h8

			vext.8		q12, q0, q6, #2			@//v128[2]
			vext.8		q3, q0, q6, #3			@//v128[3]
				
			vaddl.u8	q9, d24, d6			@//v23_l8
			vaddl.u8	q10, d25, d7		@//v23_h8

			vmla.i16	q7, q9, d4[0]		@//halfpix_l8
			vmla.i16	q8, q10, d4[0]		@//halfpix_h8

			vext.8		q1, q0, q6, #1			@//v128[1]
			vext.8		q4, q0, q6, #4			@//v128[4]
			vaddl.u8	q9, d2, d8			@//v14_l8
			vaddl.u8	q10, d3, d9			@//v14_h8

			vmls.i16	q7, q9, d4[1]		@//halfpix_l8
			vmls.i16	q8, q10, d4[1]		@//halfpix_h8

			vqrshrun.s16	d0, q7, #5		@//v64_low
			vqrshrun.s16	d1, q8, #5		@//v64_high
			
			vrhadd.u8	q0, q0, q15			@//(a+b+1)/2
			subs	i, i, #1
			vst1.8		{d0, d1}, [pPred, :128]!

			bne		LUMA16XN_dxM_VER_LOOP

			pop		{r4, r10, pc}
			@ENDFUNC
				
@//only for dx=2, dy=0
@//void MC_luma16xN_yfull (H264DecContext *img_ptr, uint8 * pRefFrame, uint8 * pPred, int32 N)
MC_luma16xN_yfull:	@FUNCTION
			.global	MC_luma16xN_yfull
				
			push		{r4, r10, r14}
				
			ldr		r14, =MC_coefficient_neon
			sub		pRefFrame, pRefFrame, #2
			vld1.16	{d4}, [r14]
			ldrh		width, [img_ptr, #20]		
			
LUMA16XN_dx2_VER_LOOP:
			add 	r14, pRefFrame, #16
			vld1.8	{d12, d13}, [r14]				@//v128_next
			vld1.8	{d0, d1}, [pRefFrame], width	@//v128[0]

			vext.8		q5, q0, q6, #5		@//v128[5]
			vext.8		q12, q0, q6, #2		@//v128[2]
			vaddl.u8	q7, d0, d10			@//v05_l8
			vext.8		q3, q0, q6, #3		@//v128[3]
			vaddl.u8	q8, d1, d11			@//v05_h8

			vaddl.u8	q9, d24, d6			@//v23_l8
			vaddl.u8	q10, d25, d7		@//v23_h8

			vext.8		q1, q0, q6, #1		@//v128[1]
			vmla.i16	q7, q9, d4[0]		@//halfpix_l8
			vext.8		q4, q0, q6, #4		@//v128[4]
			vmla.i16	q8, q10, d4[0]		@//halfpix_h8

			vaddl.u8	q9, d2, d8			@//v14_l8
			vaddl.u8	q10, d3, d9			@//v14_h8

			vmls.i16	q7, q9, d4[1]		@//halfpix_l8
			vmls.i16	q8, q10, d4[1]		@//halfpix_h8

			vqrshrun.s16	d0, q7, #5		@//v64_low
			vqrshrun.s16	d1, q8, #5		@//v64_high

			subs		i, i, #1
			vst1.8	{d0, d1}, [pPred, :128]!

			bne		LUMA16XN_dx2_VER_LOOP

			pop		{r4, r10, pc}
			@ENDFUNC	

@//void MC_luma16xN_dx0dyM (H264DecContext *img_ptr, uint8 * pRefFrame, uint8 * pPred, int32 N)
MC_luma16xN_dx0dyM:	@FUNCTION
			@.extern	g_refPosy
			.global	MC_luma16xN_dx0dyM
				
			push		{r4, r10, r14}
				
			ldr		r14, [img_ptr, #56]	@//g_refPosy
			ldrh		width, [img_ptr, #20]
			and		r14, r14, #3
			cmp		r14, #1
			moveq	offset, width, lsl #2
			addne	offset, width, width, lsl #1

			ldr		r14, =MC_coefficient_neon
			sub		pRefFrame, pRefFrame, width, lsl #1
			vld1.16	{d4}, [r14]

			vld1.8	{d0, d1}, 	[pRefFrame], width		@//v128[0]
			vld1.8	{d2, d3},  	[pRefFrame], width		@//v128[1]
			vld1.8	{d24, d25},	[pRefFrame], width		@//v128[2]
			vld1.8	{d6, d7},  	[pRefFrame], width		@//v128[3]
			vld1.8	{d8, d9},  	[pRefFrame], width		@//v128[4]
			vld1.8	{d10, d11}, [pRefFrame], width		@//v128[5]		
			
LUMA16XN_dyM_VER_LOOP:
			sub 		r14, pRefFrame, offset
			vld1.8		{d30, d31}, [r14]
			
			vaddl.u8	q7, d0, d10			@//v05_l8
			vaddl.u8	q8, d1, d11			@//v05_h8
				
			vaddl.u8	q9, d24, d6			@//v23_l8
			vaddl.u8	q10, d25, d7		@//v23_h8

			vmla.i16	q7, q9, d4[0]		@//halfpix_l8
			vmla.i16	q8, q10, d4[0]		@//halfpix_h8

			vaddl.u8	q9, d2, d8			@//v14_l8
			vaddl.u8	q10, d3, d9			@//v14_h8

			vmls.i16	q7, q9, d4[1]		@//halfpix_l8
			vmls.i16	q8, q10, d4[1]		@//halfpix_h8

			vqrshrun.s16	d0, q7, #5		@//v64_low
			vqrshrun.s16	d1, q8, #5		@//v64_high

			vrhadd.u8	q0, q0, q15			@//(a+b+1)/2
			subs		i, i, #1
			vst1.8		{d0, d1}, [pPred, :128]!

			@//swp
			vmov	q0, q1
			vmov	q1, q12
			vmov	q12, q3
			vmov	q3, q4
			vmov	q4, q5
			vld1.8	{d10, d11},  [pRefFrame], width		@//v128[5]
			
			bne		LUMA16XN_dyM_VER_LOOP

			pop		{r4, r10, pc}
			@ENDFUNC

@//void MC_luma16xN_xyqpix (H264DecContext *img_ptr, uint8 * pRefFrame, uint8 * pPred, int32 N)
MC_luma16xN_xyqpix:	@FUNCTION
			@.extern	g_refPosx
			.global	MC_luma16xN_xyqpix
				
			push		{r4 - r7, r10, r14}
				
			ldr		r14, [img_ptr, #52]	@//g_refPosx
			ldrh		width, [img_ptr, #20]
			and		dx, r14, #3
			add		pRefVerFilter, pRefFrame, dx, lsr #1

			ldr		r14, =MC_coefficient_neon
			sub		pRefVerFilter, pRefVerFilter, width, lsl #1
			vld1.16	{d4}, [r14]

			vld1.8	{d0, d1}, 	[pRefVerFilter], width		@//v128[0]
			vld1.8	{d2, d3},  	[pRefVerFilter], width		@//v128[1]
			vld1.8	{d24, d25}, [pRefVerFilter], width		@//v128[2]
			vld1.8	{d6, d7},  	[pRefVerFilter], width		@//v128[3]
			vld1.8	{d8, d9},  	[pRefVerFilter], width		@//v128[4]
			vld1.8	{d10, d11}, [pRefVerFilter], width		@//v128[5]		

			mov		pPredTemp, pPred
			mov		ii, N
MC_luma16xN_xyqpix_LOOP_0:
			vaddl.u8	q7, d0, d10			@//v05_l8
			vaddl.u8	q8, d1, d11			@//v05_h8
				
			vaddl.u8	q9, d24, d6			@//v23_l8
			vaddl.u8	q10, d25, d7		@//v23_h8

			vmla.i16	q7, q9, d4[0]		@//halfpix_l8
			vmla.i16	q8, q10, d4[0]		@//halfpix_h8

			vaddl.u8	q9, d2, d8			@//v14_l8
			vaddl.u8	q10, d3, d9			@//v14_h8

			vmls.i16	q7, q9, d4[1]		@//halfpix_l8
			vmls.i16	q8, q10, d4[1]		@//halfpix_h8

			vqrshrun.s16	d0, q7, #5		@//v64_low
			vqrshrun.s16	d1, q8, #5		@//v64_high
			subs		ii, ii, #1
			vst1.8		{d0, d1}, [pPredTemp, :128]!

			@//swp
			vmov	q0, q1
			vmov	q1, q12
			vmov	q12, q3
			vmov	q3, q4
			vmov	q4, q5
			vld1.8	{d10, d11},  [pRefVerFilter], width	@//v128[5]
			
			bne		MC_luma16xN_xyqpix_LOOP_0

			cmp		dx, #0
			beq		MC_luma16xN_xyqpix_END

			ldr		r14, [img_ptr, #56]	@//g_refPosy
			and		dy, r14, #3
			mov		dy, dy, asr #1
			mla		pRefHorFilter, width, dy, pRefFrame
			sub		pRefHorFilter, pRefHorFilter, #2

MC_luma16xN_xyqpix_LOOP_1:
			add 		r14, pRefHorFilter, #16
			vld1.8	{d12, d13}, [r14]					@//v128_next
			vld1.8	{d0, d1}, [pRefHorFilter], width	@//v128[0]
			vld1.8	{d30, d31}, [pPred]		@//pred16

			vext.8	q5, q0, q6, #5			@//v128[5]
			vext.8	q12, q0, q6, #2			@//v128[2]
			vaddl.u8	q7, d0, d10			@//v05_l8
			vext.8	q3, q0, q6, #3			@//v128[3]
			vaddl.u8	q8, d1, d11			@//v05_h8

			vaddl.u8	q9, d24, d6			@//v23_l8
			vaddl.u8	q10, d25, d7		@//v23_h8

			vext.8	q1, q0, q6, #1			@//v128[1]
			vmla.i16	q7, q9, d4[0]		@//halfpix_l8
			vext.8	q4, q0, q6, #4			@//v128[4]
			vmla.i16	q8, q10, d4[0]		@//halfpix_h8

			vaddl.u8	q9, d2, d8			@//v14_l8
			vaddl.u8	q10, d3, d9			@//v14_h8

			vmls.i16	q7, q9, d4[1]		@//halfpix_l8
			vmls.i16	q8, q10, d4[1]		@//halfpix_h8

			vqrshrun.s16	d0, q7, #5		@//v64_low
			vqrshrun.s16	d1, q8, #5		@//v64_high

			vrhadd.u8	q0, q0, q15			@//(a+b+1)/2
			subs		i, i, #1
			vst1.8		{d0, d1}, [pPred, :128]!

			bne		MC_luma16xN_xyqpix_LOOP_1

MC_luma16xN_xyqpix_END:
			pop		{r4 - r7, r10, pc}
			@ENDFUNC

@//void MC_luma16xN_xhalf (H264DecContext *img_ptr, uint8 * pRefFrame, uint8 * pPred, int32 N)
MC_luma16xN_xhalf:	@FUNCTION
			@.extern	g_halfPixTemp
			.global	MC_luma16xN_xhalf
							
			push	{r4 - r8, r10, r14}

			ldrh		width, [img_ptr, #20]
			ldr		pHalfPix, [img_ptr, #60]	@//g_halfPixTemp

			ldr		r14, =MC_coefficient_neon
			sub		pRefFrame, pRefFrame, width, lsl #1
			vld1.16	{d4}, [r14]
			
@//sub func: void luma21x5pN_interpolation_hor (uint8 * pRefFrame, int16 * pHalfPix, int32 width, int32 N)							
			sub 		pRefFrame, pRefFrame, #2

			add		ii, N, #5
luma21x5pN_interpolation_hor_LOOP:
			add 		r14, pRefFrame, #16
			vld1.8	{d12, d13}, [r14]				@//v128_next
			vld1.8	{d0, d1}, [pRefFrame], width 	@//v128[0]
			
			vext.8	q5, q0, q6, #5			@//v128[5]
			vext.8	q12, q0, q6, #2 		@//v128[2]
			vaddl.u8	q7, d0, d10 		@//v05_l8
			vext.8	q3, q0, q6, #3			@//v128[3]
			vaddl.u8	q8, d1, d11 		@//v05_h8
			
			vaddl.u8	q9, d24, d6 		@//v23_l8
			vaddl.u8	q10, d25, d7		@//v23_h8
			
			vext.8	q1, q0, q6, #1			@//v128[1]
			vmla.i16	q7, q9, d4[0]		@//halfpix_l8
			vext.8	q4, q0, q6, #4			@//v128[4]
			vmla.i16	q8, q10, d4[0]		@//halfpix_h8
			
			vaddl.u8	q9, d2, d8			@//v14_l8
			vaddl.u8	q10, d3, d9 		@//v14_h8
			
			vmls.i16	q7, q9, d4[1]		@//halfpix_l8
			vmls.i16	q8, q10, d4[1]		@//halfpix_h8
			
			vst1.8	{d14, d15}, [pHalfPix, :128]!
			vst1.8	{d16, d17}, [pHalfPix, :128]!
			
			subs		ii, ii, #1
			bne 		luma21x5pN_interpolation_hor_LOOP

@//sub func: luma16x5pN_interpolation_ver (int16 * pHalfPix, uint8 * pPred, int32 N)
			ldr	pHalfPix, [img_ptr, #60]	@//g_halfPixTemp

			mov		pPredTemp, pPred
			mov		width, #32
			mov		pPred_inc_1,	#16
			
			mov		j, #2
luma16x5pN_interpolation_ver_loop_j:			
			vld1.16	{d0, d1}, 	[pHalfPix], width	@//v128[0]
			vld1.16	{d2, d3},  	[pHalfPix], width	@//v128[1]
			vld1.16	{d24, d25}, [pHalfPix], width 	@//v128[2]
			vld1.16	{d6, d7},  	[pHalfPix], width	@//v128[3]
			vld1.16	{d8, d9},  	[pHalfPix], width	@//v128[4]
			vld1.16	{d10, d11}, [pHalfPix], width 	@//v128[5]		

			mov		ii, N
luma16x5pN_interpolation_ver_loop_i:
			vadd.i16	q8, q12, q3 		@//v23
			vadd.i16	q10, q0, q5 		@//v05
				
			vmull.s16	q7, d16, d4[0]			
			vmull.s16	q8, d17, d4[0]		

			vaddw.s16	q7, q7, d20			@//halfpix_l8
			vadd.i16	q9, q1, q4			@//v14_l8
			vaddw.s16	q8, q8, d21			@//halfpix_h8
				
			vmlsl.s16	q7, d18, d4[1]		@//halfpix_l8
			vmlsl.s16	q8, d19, d4[1]		@//halfpix_h8
				
			vqrshrun.s32	d12, q7, #10 	@//v64_low
			vqrshrun.s32	d13, q8, #10 	@//v64_high
			
			vqmovn.u16	d0, q6			
			vst1.8		{d0}, [pPredTemp, :64], pPred_inc_1
				
			@//swp
			vmov	q0, q1
			vmov	q1, q12
			vmov	q12, q3
			vmov	q3, q4
			vmov	q4, q5
			vld1.8	{d10, d11},  [pHalfPix], width	@//v128[5]
							
			subs		ii, ii, #1
			bne 		luma16x5pN_interpolation_ver_loop_i

			@//update for next 8 pix
			add		pPredTemp, pPredTemp, #8
			sub		pPredTemp, pPredTemp, N, lsl #4
			sub		pHalfPix, pHalfPix, #(32*6-16)
			sub		pHalfPix, pHalfPix, N, lsl #5

			subs		j, j, #1
			bne		luma16x5pN_interpolation_ver_loop_j
			
			ldr		r14, [img_ptr, #56]	@//g_refPosy
			and 		r14, r14, #3
			cmp 		r14, #2
			beq		MC_luma16xN_xhalf_end

			ldr		pHalfPix, [img_ptr, #60]	@//g_halfPixTemp

			mov		r14, r14, asr #1
			add		r14, r14, #2
			add		pHalfPix, pHalfPix, r14, lsl #5

@//sub func: void bilinear_filter16xN_short (int16 * phalfPix, uint8 * pPred, int32 width, int32 N)
@// width = 16
bilinear_filter16xN_short_w16_loop:
			vld1.8	{d0, d1}, [pPred]

			vld1.8	{d2, d3}, [pHalfPix]!@//the first 8 hpix
			vld1.8	{d4, d5}, [pHalfPix]!	@//the second 8 hpix
			vqrshrun.s16 d2, q1, #5
			vqrshrun.s16 d3, q2, #5

			vrhadd.u8	q0, q0, q1
			vst1.8		{d0, d1}, [pPred, :128]!

			subs		i, i, #1
			bne		bilinear_filter16xN_short_w16_loop

MC_luma16xN_xhalf_end:
			pop {r4 - r8, r10, pc}
			@ENDFUNC

@//void MC_luma16xN_yhalf (H264DecContext *img_ptr, uint8 * pRefFrame, uint8 * pPred, int32 N)
MC_luma16xN_yhalf:	@FUNCTION
			@.extern	g_halfPixTemp
			.global	MC_luma16xN_yhalf
							
			push		{r4 - r10, r14}
			
			ldrh		width, [img_ptr, #20]
			ldr		pHalfPix, [img_ptr, #60]	@//g_halfPixTemp

			ldr		r14, =MC_coefficient_neon
			sub		pRefFrame, pRefFrame, width, lsl #1
			vld1.16	{d4}, [r14]
			
@//sub func: luma21xN_interpolation_ver (uint8 * pRefFrame, int16 * pHalfPix, int32 width, int32 N)						
			sub		pRefFrame, pRefFrame, #2

			mov		pHalfPixTmp_inc, #(24*2)

			mov		j, #2
luma21xN_interpolation_ver_loop_j:
			add		pRefTmp, 	pRefFrame, j, lsl #3
			add		pHalfPixTmp, pHalfPix, j, lsl #4

			vld1.8	{d0}, [pRefTmp], width		@//v64[0]
			vld1.8	{d1}, [pRefTmp], width		@//v64[1]
			vld1.8	{d2}, [pRefTmp], width		@//v64[2]
			vld1.8	{d3}, [pRefTmp], width		@//v64[3]
			vld1.8	{d24}, [pRefTmp], width		@//v64[4]
			vld1.8	{d25}, [pRefTmp], width		@//v64[5]

			mov		ii, N
luma21xN_interpolation_ver_loop_i:
			vaddl.u8	q4, d0, d25			@//v05
			vaddl.u8	q5, d2, d3			@//v23

			vmla.i16	q4, q5, d4[0]		@//halfpix

			vaddl.u8	q5, d1, d24			@//v14

			vmls.i16	q4, q5, d4[1]		@//halfpix

			vst1.8	{d8, d9}, [pHalfPixTmp, :128], pHalfPixTmp_inc

			@//swp
			vmov	d0, d1
			vmov	d1, d2
			vmov	d2, d3
			vmov	d3, d24
			vmov	d24, d25
			vld1.8	{d25},  [pRefTmp], width		@//v64[5]
				
			subs		ii, ii, #1
			bne 		luma21xN_interpolation_ver_loop_i

			subs		j, j, #1
			bpl		luma21xN_interpolation_ver_loop_j

@//sub func: luma21xN_interpolation_hor (int16 * pHalfPix, uint8 * pPred, int32 N)
			ldr		pHalfPix, [img_ptr, #60]	@//g_halfPixTemp

			mov		pPredTemp, pPred
			mov		pHalfPix_inc, #(8*2)
			
			mov		ii, N
luma21xN_interpolation_hor_loop_i:			

			mov		j, #2
luma21xN_interpolation_hor_loop_j:
			vld1.16	{d0, d1}, [pHalfPix], pHalfPix_inc	@//v128[0]
			vld1.16	{d12, d13}, [pHalfPix]			@//v128_next

			vext.8	q12, q0, q6, #4			@//v128[2]
			vext.8	q3, q0, q6, #6			@//v128[3]
			vext.8	q5, q0, q6, #10			@//v128[5]
			vadd.i16	q9, q12, q3			@//v23
			vadd.i16	q5, q0, q5			@//v05

			vmull.s16	q7, d18, d4[0]
			vmull.s16	q8, d19, d4[0]

			vaddw.s16	q7, q7, d10		@//halfpix_l8
			vaddw.s16	q8, q8, d11		@//halfpix_h8
				
			vext.8	q1, q0, q6, #2			@//v128[1]
			vext.8	q4, q0, q6, #8			@//v128[4]
			vadd.i16	q9, q1, q4			@//v14
				
			vmlsl.s16	q7, d18, d4[1]			@//halfpix_l8
			vmlsl.s16	q8, d19, d4[1]		@//halfpix_h8
				
			vqrshrun.s32	d12, q7, #10 	@//v64_low
			vqrshrun.s32	d13, q8, #10 	@//v64_high
			
			vqmovn.u16		d0, q6			
			subs	j, j, #1
			vst1.8		{d0}, [pPredTemp, :64]!
							
			bne 	luma21xN_interpolation_hor_loop_j

			@//update for next line
			add		pHalfPix, pHalfPix, pHalfPix_inc

			subs	ii, ii, #1
			bne		luma21xN_interpolation_hor_loop_i

			ldr		r14, [img_ptr, #52]	@//g_refPosx
			and 	r14, r14, #3

			@//	ldr 	pHalfPix, =g_halfPixTemp
			ldr		pHalfPix, [img_ptr, #60]	@//g_halfPixTemp

			mov		r14, r14, asr #1
			add		r14, r14, #2
			add		pHalfPix, pHalfPix, r14, lsl #1

@//sub func: void bilinear_filter16xN_short (int16 * phalfPix, uint8 * pPred, int32 width, int32 N)
@// width = 24
bilinear_filter16xN_short_w24_loop:
			vld1.8	{d0, d1}, [pPred]

			vld1.8	{d2, d3}, [pHalfPix]!	@//the first 8 hpix
			vld1.8	{d4, d5}, [pHalfPix]!	@//the second 8 hpix
			vqrshrun.s16 d2, q1, #5
			vqrshrun.s16 d3, q2, #5

			subs	i, i, #1
			vrhadd.u8	q0, q0, q1
			add		pHalfPix, pHalfPix, pHalfPix_inc	@//update for next line
			vst1.8		{d0, d1}, [pPred, :128]!

			bne		bilinear_filter16xN_short_w24_loop

MC_luma16xN_yhalf_end:
			pop {r4 - r10, pc}
			@ENDFUNC			

@//void PC_MC_chroma8xN (H264DecContext *img_ptr, uint8 ** pRefFrame, uint8 ** pPredUV, int32 N)
img_ptr		.req		r0
pRefFrame	.req 		r1
pPredUV 		.req 		r2
N			.req 		r3
width_c		.req 		r0


dx1			.req		r4
dy1			.req 		r5
dx2			.req 		r4
dy2			.req 		r6

offset_c		.req 		r7
uv			.req		r4

pPred_c 		.req		r5
pRef		.req		r6

iii			.req		r8

PC_MC_chroma8xN:	@FUNCTION
			@.extern	g_refPosx
			@.extern	g_refPosy
			.global	PC_MC_chroma8xN

			push		{r4 - r8, r14}

			ldr		r14, [img_ptr, #52]	@//g_refPosx
			and		dx1, r14, #7
			add		offset_c, r14, #0		

			ldr		r14, [img_ptr, #56]	@//g_refPosy
			and		dy1, r14, #7
			rsb		dy2, dy1, #8

			ldrh		width_c, [img_ptr, #20]
			mov		width_c, width_c, lsr #1

			mov		r14, r14, asr #3
			mul		r14, width_c, r14
			add		offset_c, r14, offset_c, asr #3
			
			mul		r14, dx1, dy1
			vdup.8	d23, r14		@//coef8x8_dr

			mul		r14, dx1, dy2
			vdup.8	d24, r14		@//coef8x8_ur

			rsb		dx2, dx1,#8
			mul		r14, dx2, dy1
			vdup.8	d25, r14	@//coef8x8_dl

			mul		r14, dx2, dy2
			vdup.8	d26, r14		@//coef8x8_ul

			mov		uv, #0
PC_MC_chroma8xN_comp_loop:
			ldr		pPred_c, [pPredUV, uv, lsl #2]
			add		r14, uv, #1
			ldr 		pRef, [pRefFrame, r14, lsl #2]
			add		pRef, pRef, offset_c

			vld1.8	{d6, d7}, [pRef], width_c
			vld1.8	{d0, d1}, [pRef], width_c

			mov		iii, N
PC_MC_chroma8xN_ver_loop:
			vext.8	d16, d6, d7, #1			@//v8x8_t[1]

			vmull.u8	q3, d6, d26
			vmlal.u8	q3, d16, d24

			vext.8	d16, d0, d1, #1			@//v8x8_b[1]
			vmlal.u8	q3, d0, d25
			vmlal.u8 	q3, d16, d23

			vqrshrn.u16	d6, q3, #6

			@//store 8 pix
			vst1.8		{d6}, [pPred_c, :64]!

			@//update for next line
			vmov	q3, q0
			vld1.8	{d0, d1}, [pRef], width_c

			subs		iii, iii, #1
			bne		PC_MC_chroma8xN_ver_loop

			add		uv, uv, #1
			cmp		uv, #2
			blt		PC_MC_chroma8xN_comp_loop		
			
PC_MC_chroma8xN_end:
			pop 		{r4 - r8, pc}
			@ENDFUNC	

			@AREA	DATA1, DATA, READONLY
			.align	4
MC_coefficient_neon:
			.short	20, 5
			
			.end
				

