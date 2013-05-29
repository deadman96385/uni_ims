
@//this asm file is for luma4xN motion compensation

@/************register map begin**************/
pRefFrame	.req			r0
pPred		.req			r1
width		.req			r2
N			.req			r3

i			.req			r3
ii			.req			r7

@//coef20		.req			r14
@//coef5		.req			r14

offset		.req			r4
pPred_inc		.req			r5
pPred_inc_1	.req			r8

dx			.req			r4
dy			.req			r4
	
pRefVerFilter	.req		r5
pRefHorFilter	.req		r5

pPredTemp		.req		r6	@//pPred
pHalfPix			.req		r4
@/************register map begin**************/

			.text					
			.arm

@//void MC_luma4xN_dx0dy0 (uint8 * pRefFrame, uint8 * pPred, int32 width, int32 N)
MC_luma4xN_dx0dy0:	@FUNCTION
			.global	MC_luma4xN_dx0dy0
				
			push		{r14}

			mov		r14, #0x10
				
MC_luma4xN_dx0dy0_VER_LOOP:
			vld1.8	{d0}, [pRefFrame], width	
			vld1.8	{d1}, [pRefFrame], width
			
			vst1.32	d0[0],	[pPred, :32], r14	@//store 4 pix, line0
			vld1.8	{d2}, [pRefFrame], width
			
			vst1.32	d1[0],	[pPred, :32], r14	@//store 4 pix, line1
			vld1.8	{d3}, [pRefFrame], width
			
			vst1.32	d2[0],	[pPred, :32], r14	@//store 4 pix, line2
			subs		i, i, #4
			vst1.32	d3[0],	[pPred, :32], r14	@//store 4 pix, line3

			bne		MC_luma4xN_dx0dy0_VER_LOOP

			pop		{ pc}
			@ENDFUNC

@//void MC_luma4xN_dxMdy0 (uint8 * pRefFrame, uint8 * pPred, int32 width, int32 N)
MC_luma4xN_dxMdy0:	@FUNCTION
			.extern	g_refPosx
			.global	MC_luma4xN_dxMdy0
				
			push		{r4 - r5, r14}
				
			ldr		r14, =g_refPosx
			ldr		r14, [r14, #0]
			and		r14, r14, #3
			cmp		r14, #1
			movne	offset, #3
			moveq	offset, #2

			ldr		r14, =MC_coefficient_neon
			mov		r5, #0x10
			vld1.16	{d4}, [r14]
			
			sub		pRefFrame, pRefFrame, #2
			
LUMA4XN_dxM_VER_LOOP:

			add		r14, pRefFrame, #8
			vld1.8	{d6}, [r14]					@//v64_next
			add		r14, pRefFrame, offset
			vld1.8	{d0}, [pRefFrame], width	@//v64[0]
			vld1.8	{d30}, [r14]

			vext.8	d25, d0, d6, #5		@//v64[5]
			vaddl.u8	q4, d0, d25			@//v05

			vext.8	d2, d0, d6, #2		@//v64[2]
			vext.8	d3, d0, d6, #3		@//v64[3]
			vaddl.u8	q5, d2, d3			@//v23

			vmla.i16	q4, q5, d4[0]			@//halfpix

			vext.8	d1, d0, d6, #1		@//v64[1]
			vext.8	d24, d0, d6, #4		@//v64[4]
			vaddl.u8	q5, d1, d24			@//v14

			vmls.i16	q4, q5, d4[1]			@//halfpix

			vqrshrun.s16	d0, q4, #5		@//v64

			vrhadd.u8	d0, d0, d30		@//(a+b+1)/2

			subs		i, i, #1
			vst1.32	d0[0], [pPred, :32], r5		@//store 4 pix			
			bne		LUMA4XN_dxM_VER_LOOP

			pop		{r4 - r5, pc}

			@ENDFUNC

@//only for dx=2, dy=0
@//void MC_luma4xN_yfull (uint8 * pRefFrame, uint8 * pPred, int32 width, int32 N)
MC_luma4xN_yfull:	@FUNCTION
			.global	MC_luma4xN_yfull
				
			push		{r4, r14}
				
			ldr		r14, =MC_coefficient_neon
			mov		r4, #0x10
			vld1.16	{d4}, [r14]
			
			sub		pRefFrame, pRefFrame, #2
			
LUMA4XN_dx2_VER_LOOP:
			add		r14, pRefFrame, #8
			vld1.8	{d6}, [r14]					@//v64_next
			vld1.8	{d0}, [pRefFrame], width	@//v64[0]

			vext.8	d25, d0, d6, #5		@//v64[5]
			vaddl.u8	q4, d0, d25			@//v05

			vext.8	d2, d0, d6, #2		@//v64[2]
			vext.8	d3, d0, d6, #3		@//v64[3]
			vaddl.u8	q5, d2, d3			@//v23

			vmla.i16	q4, q5, d4[0]			@//halfpix

			vext.8	d1, d0, d6, #1		@//v64[1]
			vext.8	d24, d0, d6, #4		@//v64[4]
			vaddl.u8	q5, d1, d24			@//v14

			vmls.i16	q4, q5, d4[1]			@//halfpix

			vqrshrun.s16	d0, q4, #5		@//v64
			
			subs		i, i, #1
			vst1.32	d0[0], [pPred, :32], r4		@//store 4 pix			
			bne		LUMA4XN_dx2_VER_LOOP

			pop	{r4, pc}

			@ENDFUNC			

@//void MC_luma4xN_dx0dyM (uint8 * pRefFrame, uint8 * pPred, int32 width, int32 N)
MC_luma4xN_dx0dyM:	@FUNCTION

			.extern	g_refPosy
			.global	MC_luma4xN_dx0dyM
				
			push		{r4 - r5, r14}
				
			ldr		r14, =g_refPosy
			ldr		r14, [r14, #0]
			and		r14, r14, #3
			cmp		r14, #1
			moveq	offset, width, lsl #2
			addne	offset, width, width, lsl #1

			ldr		r14, =MC_coefficient_neon
			mov		r5, #0x10
			vld1.16	{d4}, [r14]

			sub		pRefFrame, pRefFrame, width, lsl #1
			
			vld1.8	{d0}, 	[pRefFrame], width		@//v64[0]
			vld1.8	{d1}, 	[pRefFrame], width		@//v64[1]
			vld1.8	{d2}, 	[pRefFrame], width		@//v64[2]
			vld1.8	{d3}, 	[pRefFrame], width		@//v64[3]
			vld1.8	{d24}, 	[pRefFrame], width		@//v64[4]
			vld1.8	{d25}, 	[pRefFrame], width		@//v64[5]

LUMA4XN_dyM_VER_LOOP:
			sub		r14, pRefFrame, offset
			vld1.8	{d30}, [r14]
			
			vaddl.u8	q4, d0, d25			@//v05
			vaddl.u8	q5, d2, d3			@//v23

			vmla.i16	q4, q5, d4[0]			@//halfpix

			vaddl.u8	q5, d1, d24			@//v14

			vmls.i16	q4, q5, d4[1]			@//halfpix

			vqrshrun.s16	d0, q4, #5		@//v64

			vrhadd.u8	d0, d0, d30		@//(a+b+1)/2
			
			vst1.32	d0[0], [pPred, :32], r5		@//store 4 pix			

			@//swp
			vmov	d0, d1
			vmov	d1, d2
			vmov	d2, d3
			vmov	d3, d24
			vmov	d24, d25
			vld1.8	{d25},  [pRefFrame]			@//v64[5]
			add		pRefFrame, pRefFrame, width
				
			subs		i, i, #1
			bne		LUMA4XN_dyM_VER_LOOP

			pop	{r4 - r5, pc}
			@ENDFUNC

@//void MC_luma4xN_xyqpix (uint8 * pRefFrame, uint8 * pPred, int32 width, int32 N)
MC_luma4xN_xyqpix:	@FUNCTION
			.extern	g_refPosx
			.global	MC_luma4xN_xyqpix
				
			push		{r4 - r8, r14}
				
			ldr		r14, =g_refPosx
			ldr		r14, [r14, #0]
			and		dx, r14, #3
			add		pRefVerFilter, pRefFrame, dx, lsr #1
			sub		pRefVerFilter, pRefVerFilter, width, lsl #1

			ldr		r14, =MC_coefficient_neon
			mov		r8, #0x10
			vld1.16	{d4}, [r14]

			vld1.8	{d0}, 	[pRefVerFilter], width		@//v64[0]
			vld1.8	{d1}, 	[pRefVerFilter], width		@//v64[1]
			vld1.8	{d2}, 	[pRefVerFilter], width		@//v64[2]
			vld1.8	{d3}, 	[pRefVerFilter], width		@//v64[3]
			vld1.8	{d24}, 	[pRefVerFilter], width		@//v64[4]
			vld1.8	{d25}, 	[pRefVerFilter], width		@//v64[5]

			mov		pPredTemp, pPred
			mov		ii, N
MC_luma4xN_xyqpix_LOOP_0:
			vaddl.u8	q4, d0, d25			@//v05
			vaddl.u8	q5, d2, d3			@//v23

			vmla.i16	q4, q5, d4[0]			@//halfpix

			vaddl.u8	q5, d1, d24			@//v14

			vmls.i16	q4, q5, d4[1]			@//halfpix

			vqrshrun.s16	d0, q4, #5		@//v64
			
			vst1.32	d0[0], [pPredTemp], r8		@//store 4 pix			

			@//swp
			vmov	d0, d1
			vmov	d1, d2
			vmov	d2, d3
			vmov	d3, d24
			vmov	d24, d25
			vld1.8	{d25},  [pRefVerFilter], width	@//v64[5]
			
			subs		ii, ii, #1
			bne		MC_luma4xN_xyqpix_LOOP_0

			cmp		dx, #0
			beq		MC_luma4xN_xyqpix_END

			ldr		r14, =g_refPosy
			ldr		r14, [r14, #0]
			and		dy, r14, #3
			mov		dy, dy, asr #1
			mla		pRefHorFilter, width, dy, pRefFrame
			sub		pRefHorFilter, pRefHorFilter, #2

MC_luma4xN_xyqpix_LOOP_1:
			add		r14, pRefHorFilter, #8
			vld1.8	{d6}, [r14]			@//v64_next
			vld1.8	{d0}, [pRefHorFilter], width		@//v64[0]
			vld1.32	{d30[0]}, [pPred, :32]

			vext.8	d15, d0, d6, #5		@//v64[5]
			vaddl.u8	q4, d0, d15			@//v05

			vext.8	d2, d0, d6, #2		@//v64[2]
			vext.8	d3, d0, d6, #3		@//v64[3]
			vaddl.u8	q5, d2, d3			@//v23

			vmla.i16	q4, q5, d4[0]			@//halfpix

			vext.8	d1, d0, d6, #1		@//v64[1]
			vext.8	d14, d0, d6, #4		@//v64[4]
			vaddl.u8	q5, d1, d14			@//v14

			vmls.i16	q4, q5, d4[1]			@//halfpix

			vqrshrun.s16	d0, q4, #5		@//v64

			vrhadd.u8	d0, d0, d30		@//(a+b+1)/2

			subs		i, i, #1
			vst1.32	d0[0], [pPred, :32], r8		@//store 4 pix			

			bne		MC_luma4xN_xyqpix_LOOP_1

MC_luma4xN_xyqpix_END:
			pop		{r4 - r8, pc}
			@ENDFUNC					

@//void MC_luma4xN_xhalf (uint8 * pRefFrame, uint8 * pPred, int32 width, int32 N)
MC_luma4xN_xhalf:	@FUNCTION
			.extern	g_halfPixTemp
			.global	MC_luma4xN_xhalf
							
			push		{r4 - r8, r14}

			@//	ldr 	pHalfPix, =g_halfPixTemp
			ldr 	r4, =g_halfPixTemp
			ldr	pHalfPix, [r4, #0]

			ldr		r14, =MC_coefficient_neon
			mov		r5, #0x10
			vld1.16	{d4}, [r14]
			
@//sub func: void luma9x5pN_interpolation_hor (uint8 * pRefFrame, int16 * pHalfPix, int32 width, int32 N)							
			sub		pRefFrame, pRefFrame, width, lsl #1
			sub 		pRefFrame, pRefFrame, #2

			add		ii, N, #5
luma9x5pN_interpolation_hor_LOOP:
			add		r14, pRefFrame, #8
			vld1.8	{d6}, [r14]					@//v64_next
			vld1.8	{d0}, [pRefFrame], width	@//v64[0]

			vext.8	d25, d0, d6, #5		@//v64[5]
			vaddl.u8	q4, d0, d25			@//v05

			vext.8	d2, d0, d6, #2		@//v64[2]
			vext.8	d3, d0, d6, #3		@//v64[3]
			vaddl.u8	q5, d2, d3			@//v23

			vmla.i16	q4, q5, d4[0]		@//halfpix

			vext.8	d1, d0, d6, #1		@//v64[1]
			vext.8	d24, d0, d6, #4		@//v64[4]
			vaddl.u8	q5, d1, d24			@//v14

			vmls.i16	q4, q5, d4[1]		@//halfpix
			
			vst1.8	{d8}, [pHalfPix]!
			
			subs		ii, ii, #1
			bne 		luma9x5pN_interpolation_hor_LOOP

@//sub func: luma4x5pN_interpolation_ver (int16 * pHalfPix, uint8 * pPred, int32 N)
			@//	ldr 	pHalfPix, =g_halfPixTemp
			ldr 	r4, =g_halfPixTemp
			ldr	pHalfPix, [r4, #0]

			mov		pPredTemp, pPred
			mov		width, #8

			vld1.16	{d0}, 	[pHalfPix, :64], width		@//v64[0]
			vld1.16	{d1}, 	[pHalfPix, :64], width		@//v64[1]
			vld1.16	{d2}, 	[pHalfPix, :64], width		@//v64[2]
			vld1.16	{d3}, 	[pHalfPix, :64], width		@//v64[3]
			vld1.16	{d24}, 	[pHalfPix, :64], width		@//v64[4]
			vld1.16	{d25},	[pHalfPix, :64], width		@//v64[5]

			mov		ii, N
luma4x5pN_interpolation_ver_loop_i:				
			vadd.i16	d10, d0, d25 	@//v05
			vadd.i16	d8, d2, d3 		@//v23
				
			vmull.s16	q7, d8, d4[0]			

			vaddw.s16	q7, q7, d10	@//halfpix_l8
				
			vadd.i16	d9, d1, d24		@//v14_l8
				
			vmlsl.s16	q7, d9, d4[1]	@//halfpix_l8
				
			vqrshrun.s32	d12, q7, #10 	@//v64_low
			
			vqmovn.u16	d0, q6
				
			vst1.32	d0[0], [pPredTemp, :32], r5	@//store 4 pix

			@//swp
			vmov	d0, d1
			vmov	d1, d2
			vmov	d2, d3
			vmov	d3, d24
			vmov	d24, d25
			vld1.8	{d25},  [pHalfPix, :64], width			@//v64[5]
							
			subs		ii, ii, #1
			bne 		luma4x5pN_interpolation_ver_loop_i
			
			ldr 		r14, =g_refPosy
			ldr 		r14, [r14, #0]
			and 		r14, r14, #3
			cmp 		r14, #2
			beq		MC_luma4xN_xhalf_end

			@//	ldr 	pHalfPix, =g_halfPixTemp
			ldr 	r4, =g_halfPixTemp
			ldr	pHalfPix, [r4, #0]

			mov		r14, r14, asr #1
			add		r14, r14, #2
			add		pHalfPix, pHalfPix, r14, lsl #3

@//sub func: void bilinear_filter4xN_short (int16 * phalfPix, uint8 * pPred, int32 width, int32 N)
@// width = 4
bilinear_filter4xN_short_w4_loop:
			vld1.32	{d0[0]}, [pPred, :32]

			vld1.8	{d2, d3}, [pHalfPix], width	@//8 hpix
			vqrshrun.s16 d1, q1, #5

			vrhadd.u8	d0, d0, d1
			
			subs		i, i, #1
			vst1.32	d0[0], [pPred, :32], r5		@//store 4 pix			

			bne		bilinear_filter4xN_short_w4_loop

MC_luma4xN_xhalf_end:
			pop {r4 - r8, pc}
			@ENDFUNC	

@//void MC_luma4xN_yhalf (uint8 * pRefFrame, uint8 * pPred, int32 width, int32 N)
MC_luma4xN_yhalf:	@FUNCTION
			.extern	g_halfPixTemp
			.global	MC_luma4xN_yhalf
							
			push		{r4 - r9, r14}

			@//	ldr 	pHalfPix, =g_halfPixTemp
			ldr 	r4, =g_halfPixTemp
			ldr	pHalfPix, [r4, #0]

			ldr		r14, =MC_coefficient_neon
			mov		r5, #0x10
			vld1.16	{d4}, [r14]
@//sub func: luma9xN_interpolation_ver (uint8 * pRefFrame, int16 * pHalfPix, int32 width, int32 N)						
			sub		pRefFrame, pRefFrame, width, lsl #1
			sub		pRefFrame, pRefFrame, #2

			vld1.8	{d0, d1}, 	[pRefFrame], width		@//v128[0]
			vld1.8	{d2, d3},  	[pRefFrame], width		@//v128[1]
			vld1.8	{d24, d25},	[pRefFrame], width		@//v128[2]
			vld1.8	{d6, d7},  	[pRefFrame], width		@//v128[3]
			vld1.8	{d8, d9},  	[pRefFrame], width		@//v128[4]
			vld1.8	{d10, d11},  [pRefFrame], width		@//v128[5]		

			mov		ii, N
luma9xN_interpolation_ver_loop_i:
			vaddl.u8	q7, d0, d10			@//v05_l8
			vaddl.u8	q8, d1, d11			@//v05_h8
				
			vaddl.u8	q9, d24, d6			@//v23_l8
			vaddl.u8	q10, d25, d7			@//v23_h8

			vmla.i16	q7, q9, d4[0]			@//halfpix_l8
			vmla.i16	q8, q10, d4[0]		@//halfpix_h8

			vaddl.u8	q9, d2, d8			@//v14_l8
			vaddl.u8	q10, d3, d9			@//v14_h8

			vmls.i16	q7, q9, d4[1]		@//halfpix_l8
			vmls.i16	q8, q10, d4[1]		@//halfpix_h8

			vst1.8	{d14, d15}, [pHalfPix, :128]!
			vst1.8	{d16, d17}, [pHalfPix, :128]!
			
			@//swp
			vmov	q0, q1
			vmov	q1, q12
			vmov	q12, q3
			vmov	q3, q4
			vmov	q4, q5
			vld1.8	{d10, d11},  [pRefFrame], width	@//v128[5]
				
			subs		ii, ii, #1
			bne 		luma9xN_interpolation_ver_loop_i

@//sub func: luma9xN_interpolation_hor (int16 * pHalfPix, uint8 * pPred, int32 N)
			@//	ldr 	pHalfPix, =g_halfPixTemp
			ldr 	r4, =g_halfPixTemp
			ldr	pHalfPix, [r4, #0]

			mov		pPredTemp, pPred
			mov		width, #32
			mov		pPred_inc_1,	#16
			
			mov		ii, N
luma9xN_interpolation_hor_loop_i:			
			vld1.16	{d0, d1}, [pHalfPix]!		@//v128[0]
			vld1.16	{d12, d13}, [pHalfPix]!	@//v128_next

			vext.8	q5, q0, q6, #10			@//v128[5]
			vadd.i16	q5, q0, q5				@//v05

			vext.8	q12, q0, q6, #4			@//v128[2]
			vext.8	q3, q0, q6, #6			@//v128[3]
				
			vadd.i16	q9, q12, q3				@//v23
	
			vmull.s16	q7, d18, d4[0]
			vmull.s16	q8, d19, d4[0]

			vaddw.s16	q7, q7, d10			@//halfpix_l8
			vaddw.s16	q8, q8, d11			@//halfpix_h8
				
			vext.8	q1, q0, q6, #2			@//v128[1]
			vext.8	q4, q0, q6, #8			@//v128[4]
			vadd.i16	q9, q1, q4				@//v14
				
			vmlsl.s16	q7, d18, d4[1]			@//halfpix_l8

			vqrshrun.s32	d12, q7, #10 			@//v64_low
			
			vqmovn.u16	d0, q6
				
			vst1.32	d0[0], [pPredTemp, :32], r5		@//store 4 pix			
										
			subs		ii, ii, #1
			bne		luma9xN_interpolation_hor_loop_i
			
			ldr 		r14, =g_refPosx
			ldr 		r14, [r14, #0]
			and 		r14, r14, #3

			@//	ldr 	pHalfPix, =g_halfPixTemp
			ldr 	r4, =g_halfPixTemp
			ldr	pHalfPix, [r4, #0]

			mov		r14, r14, asr #1
			add		r14, r14, #2
			add		pHalfPix, pHalfPix, r14, lsl #1

@//sub func: void bilinear_filter4xN_short (int16 * phalfPix, uint8 * pPred, int32 width, int32 N)
@// width = 16
bilinear_filter4xN_short_w16_loop:
			vld1.32	{d0[0]}, [pPred, :32]

			vld1.8	{d2, d3}, [pHalfPix], width	@//8 hpix
			vqrshrun.s16 d1, q1, #5

			vrhadd.u8	d0, d0, d1
			
			subs		i, i, #1
			vst1.32	d0[0], [pPred, :32], r5		@//store 4 pix			
			bne		bilinear_filter4xN_short_w16_loop

MC_luma4xN_yhalf_end:
			pop 		{r4 - r9, pc}
			@ENDFUNC

@//void PC_MC_chroma2xN (uint8 ** pRefFrame, uint8 ** pPredUV, int32 width_c, int32 N)
pRefFrame	.req 		r0
pPredUV 		.req 		r1
width_c		.req 		r2
N			.req 		r3

dx1			.req 		r4
dy1			.req 		r5
dx2			.req 		r4
dy2			.req 		r6

offset_c		.req 		r7
uv			.req		r4

pPred_c 		.req		r5
pRef		.req		r6

iii			.req		r9

PC_MC_chroma2xN:	@FUNCTION
			.extern	g_refPosx
			.extern	g_refPosy
			.global	PC_MC_chroma2xN

			push		{r4 - r9, r14}

			ldr		r14, =g_refPosx
			ldr		r14, [r14, #0]
			and		dx1, r14, #7
			add		offset_c, r14, #0		

			ldr		r14, =g_refPosy
			ldr		r14, [r14, #0]
			and		dy1, r14, #7
			rsb		dy2, dy1, #8

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

			mov		r8, #8

			mov		uv, #0
PC_MC_chroma2xN_comp_loop:
			ldr		pPred_c, [pPredUV, uv, lsl #2]
			add		r14, uv, #1
			ldr 		pRef, [pRefFrame, r14, lsl #2]
			add		pRef, pRef, offset_c

			vld1.8	{d6, d7}, [pRef], width_c
			vld1.8	{d0, d1}, [pRef], width_c

			mov		iii, N
PC_MC_chroma2xN_ver_loop:
			vext.8	d16, d6, d7, #1			@//v8x8_t[1]

			vmull.u8	q3, d6, d26
			vmlal.u8	q3, d16, d24

			vext.8	d16, d0, d1, #1			@//v8x8_b[1]
			vmlal.u8	q3, d0, d25
			vmlal.u8 	q3, d16, d23

			vqrshrn.u16	d6, q3, #6

			
		@//	vmov.u8	r14, d6[1]
		@//	vmov.u8	r8,  d6[0]
		@//	add		r8, r8, r14, lsl #8
		@//	strh 		r8, [pPred_c], #8
			vst1.16	d6[0], [pPred_c, :16], r8	@//store 2 pix

			@//update for next line
			vmov	q3, q0
			vld1.8	{d0, d1}, [pRef], width_c

			subs		iii, iii, #1
			bne		PC_MC_chroma2xN_ver_loop

			add		uv, uv, #1
			cmp		uv, #2
			blt		PC_MC_chroma2xN_comp_loop		
			
PC_MC_chroma2xN_end:
			pop 		{r4 - r9, pc}
			@ENDFUNC	

			@AREA	DATA1, DATA, READONLY
			.align	4
MC_coefficient_neon:
			.short	20, 5
			
			.end
				

