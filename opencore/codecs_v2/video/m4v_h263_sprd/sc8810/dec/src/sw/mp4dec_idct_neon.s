
@//this asm file is for fixed point IDCT

@ W1 2841 /* 2048*sqrt(2)*cos(1*pi/16) */
@ W2 2676 /* 2048*sqrt(2)*cos(2*pi/16) */
@ W3 2408 /* 2048*sqrt(2)*cos(3*pi/16) */
@ W5 1609 /* 2048*sqrt(2)*cos(5*pi/16) */
@ W6 1108 /* 2048*sqrt(2)*cos(6*pi/16) */
@ W7 565  /* 2048*sqrt(2)*cos(7*pi/16) */

@/************register map begin**************/
pSrc				.req		r0

pDst				.req		r1
dst_width		.req		r2

pRef			.req		r3
ref_width			.req		r7

pDst0			.req		r4
pDst1			.req 		r14

pRef0			.req 		r5
pRef1			.req 		r6

t1				.req		r12
@/************register map end**************/

.equ		BLOCK_SIZE,	8

.equ		ROW_SHIFT,	11
.equ		COL_SHIFT,	20

#define 	W1 	2841 	@/* 2048*sqrt(2)*cos(1*pi/16) */
#define		W2 	2676 	@/* 2048*sqrt(2)*cos(2*pi/16) */
#define		W3 	2408 	@/* 2048*sqrt(2)*cos(3*pi/16) */
#define		W4	2048 	@/* 2048*sqrt(2)*cos(4*pi/16) */	
#define		W5 	1609 	@/* 2048*sqrt(2)*cos(5*pi/16) */
#define		W6 	1108 	@/* 2048*sqrt(2)*cos(6*pi/16) */
#define		W7	565  	@/* 2048*sqrt(2)*cos(7*pi/16) */
#define		W4c	((1<<(COL_SHIFT-1))/W4)

@#define 	w1	d0[0]
@#define	w2	d0[1]
@#define	w3	d0[2]
@#define	w4	d0[3]
@#define	w5	d1[0]
@#define	w6	d1[1]
@#define	w7	d1[2]
@#define	w4c	d1[3]


				.text
				.arm

				.macro	LOAD_ALL_COEFFICIENT	@//64
				vld1.16	{d2, d3, d4, d5}, [pSrc]!		@//x0, x4
				vld1.16	{d6, d7, d8, d9}, [pSrc]!		@//x3, x7
				vld1.16	{d10, d11, d12, d13}, [pSrc]!		@//x1, x6
				vld1.16	{d14, d15, d16, d17}, [pSrc]!		@//x2, x5
				sub		pSrc, pSrc, #(64*2)
				.endm

				.macro	FIRST_STAGE_X4_X5_X6_X7
				@//_x4 = W7*x5+W1*x4
				@vmull.s16		q9, d4, w1
				vmull.s16		q9, d4, d0[0]
				vmull.s16		q10, d5, d0[0]
				vmlal.s16		q9, d16, d1[2]
				vmlal.s16		q10, d17, d1[2]

				@//x5 = W7*x4-W1*x5
				vmull.s16		q13, d4, d1[2]
				vmull.s16		q14, d5, d1[2]
				vmlsl.s16		q13, d16, d0[0]
				vmlsl.s16		q14, d17, d0[0]

				@//_x6 = W3*x7+W5*x6
				vmull.s16		q2, d8, d0[2]
				vmull.s16		q8, d9, d0[2]
				vmlal.s16		q2, d12, d1[0]
				vmlal.s16		q8, d13, d1[0]

				@//x7 = W3*x6-W5*x7
				vmull.s16		q12, d12, d0[2]
				vmull.s16		q15, d13, d0[2]
				vmlsl.s16		q12, d8, d1[0]
				vmlsl.s16		q15, d9, d1[0]
				.endm

				.macro	FIRST_STAGE_X4_X5_X6_X7_RND_SHIFT	@//(x+4)>>3
				vrshr.s32		q9, q9, #3		@//_x4
				vrshr.s32		q10, q10, #3
				vrshr.s32		q13, q13, #3		@//x5
				vrshr.s32		q14, q14, #3
				vrshr.s32		q2, q2, #3		@//_x6
				vrshr.s32		q8, q8, #3
				vrshr.s32		q12, q12, #3	@//x7
				vrshr.s32		q15, q15, #3	
				.endm

				.macro	SECOND_STAGE_X2		@//_x2 = W6*x3 - W2*x2
				vmull.s16		q4, d6, d1[1]
				vmull.s16		q11, d7, d1[1]
				vmlsl.s16		q4, d14, d0[1]
				vmlsl.s16		q11, d15, d0[1]
				.endm

				.macro	SECOND_STAGE_X3		@// x3 = W6*x2 + W2*x3
				vmull.s16		q4, d14, d1[1]
				vmull.s16		q11, d15, d1[1]
				vmlal.s16		q4, d6, d0[1]
				vmlal.s16		q11, d7, d0[1]
				.endm

				.macro	SECOND_STAGE_REMAIN	inc, shift
				@//x0 = (blk[8*0]<<shift) + inc;
				mov			t1, \inc
				vdup.32		q4, t1
				vshll.s16		q11, d2, \shift
				vshll.s16		q7, d3, \shift
				vadd.s32		q11, q11, q4
				vadd.s32		q7, q7, q4

				@//x1 = blk[8*4]<<shift
				vshll.s16		q1, d10, \shift
				vshll.s16		q3, d11, \shift

				@//x8 = x0 + x1
				vadd.s32		q4, q11, q1
				vadd.s32		q5, q7, q3

				@//x0 = x0 - x1
				vsub.s32		q11, q11, q1
				vsub.s32		q7, q7, q3

				@//x1 = _x4 + _x6
				vadd.s32		q1, q9, q2
				vadd.s32		q3, q10, q8

				@//x4 = _x4 - _x6
				vsub.s32		q9, q9, q2
				vsub.s32		q10, q10, q8

				@//x6 = x5 + x7
				vst1.32		{d2, d3}, [pSrc]!	@//str x1, offset = 0x40
				vst1.32		{d6, d7}, [pSrc]!
				vadd.s32		q2, q13, q12
				vadd.s32		q8, q14, q15

				@//x5 = x5 - x7
				vsub.s32		q13, q13, q12
				vsub.s32		q14, q14, q15
				vst1.32		{d4, d5}, [pSrc]!	@//str x6, offset = 0x60
				vst1.32		{d16, d17}, [pSrc]!
				.endm

				.macro	THIRD_STAGE
				sub			pSrc, pSrc, #0x80		@//offset = 0
				vld1.32		{d2, d3}, [pSrc]!		@//load x2, offset = 0x0
				vld1.32		{d6, d7}, [pSrc]!
				vld1.32		{d4, d5}, [pSrc]!		@//load x3, offset = 0x20
				vld1.32		{d16, d17}, [pSrc]!

				@//x7 = x8 + x3
				vadd.s32		q12, q4, q2
				vadd.s32		q15, q5, q8

				@//x8 = x8 - x3
				vsub.s32		q4, q4, q2
				vsub.s32		q5, q5, q8

				@//x3 = x0 +_x2
				vadd.s32		q2, q11, q1
				vadd.s32		q8, q7, q3

				@//x0 = x0 - _x2
				vsub.s32		q11, q11, q1
				vsub.s32		q7, q7, q3

				@//x2 = (181*(x4+x5)+128)>>8
				mov			t1, #181
				vdup.32		q6, t1
				vadd.s32		q1, q9, q13
				vadd.s32		q3, q10, q14
				vmul.s32		q1, q1, q6
				vmul.s32		q3, q3, q6
				vrshr.s32		q1, q1, #8		@//x2
				vrshr.s32		q3, q3, #8

				@//x4 = (181*(x4-x5)+128)>>8
				vsub.s32		q9, q9, q13
				vsub.s32		q10, q10, q14
				vmul.s32		q9, q9, q6
				vmul.s32		q10, q10, q6
				vrshr.s32		q9, q9, #8		@//x4
				vrshr.s32		q10, q10, #8
				.endm

				.macro	STORE_TWO_LINE_HOR_C	q_0, q_1, q_2, q_3, offset0, offset1
				vadd.s32		q0, \q_1, \q_0
				vadd.s32		q6, \q_3, \q_2
				vshrn.s32	d0, q0, #8
				vshrn.s32	d1, q6, #8
				add			t1, pSrc, \offset0	
				vst1.16		{d0, d1}, [t1]

				vsub.s32		q0, \q_1, \q_0
				vsub.s32		q6, \q_3, \q_2
				vshrn.s32	d0, q0, #8
				vshrn.s32	d1, q6, #8
				add			t1, pSrc, \offset1	
				vst1.16		{d0, d1}, [t1]
				.endm

				.macro	STORE_TWO_LINE_VER_C	q_0, q_1, q_2, q_3, addr0, addr1
				vadd.s32		q0, \q_1, \q_0
				vadd.s32		q6, \q_3, \q_2
				vqshrun.s32	d0, q0, #14
				vqshrun.s32	d1, q6, #14
				vqmovn.u16	d12, q0
				vsub.s32		q0, \q_1, \q_0		@//remove buble
				vst1.8		{d12}, [\addr0]		@//line

				vsub.s32		q6, \q_3, \q_2
				vqshrun.s32	d0, q0, #14
				vqshrun.s32	d1, q6, #14
				vqmovn.u16	d12, q0
				vst1.8		{d12}, [\addr1]		@//line
				.endm

				.macro	STORE_TWO_LINE_VER_I	q_0, q_1, q_2, q_3, addr0, addr1, ref_addr0, ref_addr1
				vadd.s32		q0, \q_1, \q_0
				vadd.s32		q6, \q_3, \q_2
				vshrn.s32	d0, q0, #14
				vshrn.s32	d1, q6, #14
				vld1.u8		{d12}, [\ref_addr0]
				vaddw.u8	q0, q0, d12
				vqmovun.s16	d12, q0
				vsub.s32		q0, \q_1, \q_0	@//remove buble
				vst1.8		{d12}, [\addr0]	@//line

				vsub.s32		q6, \q_3, \q_2
				vshrn.s32	d0, q0, #14
				vshrn.s32	d1, q6, #14
				vld1.u8		{d12}, [\ref_addr1]
				vaddw.u8	q0, q0, d12
				vqmovun.s16	d12, q0
				vst1.8		{d12}, [\addr1]	@//line
				.endm

				.macro	TRANS4X4BLK	d_0, d_1, d_2, d_3
				vtrn.16		\d_0, \d_1
				vtrn.16		\d_2, \d_3
				vtrn.32		\d_0, \d_2
				vtrn.32		\d_1, \d_3
				.endm

@///////////////////////////////////////////////
				.macro	HORIZONTAL_TRANS

				ldr			t1, =idct_coeff_neon
				vld1.8		{d0, d1}, [t1]

			@//load all 64 coefficient
				LOAD_ALL_COEFFICIENT

			@//first stage
				FIRST_STAGE_X4_X5_X6_X7

			@//second stage
				SECOND_STAGE_X2
				vst1.16		{d8, d9}, [pSrc]!	@//str x2,offset = 0
				vst1.16		{d22, d23}, [pSrc]!

				SECOND_STAGE_X3
				vst1.16		{d8, d9}, [pSrc]!	@//str x3,offset = 0x20
				vst1.16		{d22, d23}, [pSrc]!

				SECOND_STAGE_REMAIN	#0x80, #11

			@//third stage
				THIRD_STAGE

			@//fourth stage
				@//str line1, (x3+x2)>>8
				vadd.s32		q0, q2, q1
				vadd.s32		q6, q8, q3
				vshrn.s32	d0, q0, #8
				vshrn.s32	d1, q6, #8
				add			t1, pSrc, #(8*2-0x40)	@//offset = 0x10
				vst1.16		{d0, d1}, [t1]

				@//str line 6, (x3-x2)>>8
				vsub.s32		q0, q2, q1
				vsub.s32		q6, q8, q3
				vshrn.s32	d0, q0, #8
				vshrn.s32	d1, q6, #8

				@//must before store line6 to avoid to be covered
				vld1.32		{d2, d3}, [pSrc]!	@//load x1, offset = 0x40
				vld1.32		{d6, d7}, [pSrc]!
				vld1.32		{d4, d5}, [pSrc]!	@//load x6, offset = 0x60
				vld1.32		{d16, d17}, [pSrc]!

				add			t1, pSrc, #(6*8*2-0x80)	@//offset = 0x60
				vst1.16		{d0, d1}, [t1]

				@//str line2, (x0+x4)>>8
				@//str line5, (x0-x4)>>8
				STORE_TWO_LINE_HOR_C	q9, q11, q10, q7, #(2*8*2-0x80), #(5*8*2-0x80)

				@//str line0, (x7+x1)>>8
				@//str line7, (x7-x1)>>8
				STORE_TWO_LINE_HOR_C	q1, q12, q3, q15, #(0*8*2-0x80), #(7*8*2-0x80)

				@//str line3, (x8+x6)>>8
				@//str line4, (x8-x6)>>8
				STORE_TWO_LINE_HOR_C	q2, q4, q8, q5, #(3*8*2-0x80), #(4*8*2-0x80)				

			@//move the pointer to the start address of input block
				sub		pSrc, pSrc, #(64*2)
				.endm
@/////////////////////////////////////////////////////////////////////////////////////////////////

@//fixed point IDCT
@//input: 8x8 block dct coefficient
@//	r0: input block's pointer
@//	r1: point to the stored address in construction frame
@//	r2: output data width
@//	input data width is 8

arm_IDCTcFix:	@FUNCTION
				.global arm_IDCTcFix

				push		{r4, r12, r14}

@//horizontal transform
				HORIZONTAL_TRANS

@//vertical transform
				ldr		t1, =idct_coeff_neon
				vld1.8	{d0, d1}, [t1]

			@//load all 64 coefficient
				LOAD_ALL_COEFFICIENT

				TRANS4X4BLK	d2, d4, d6, d8
				TRANS4X4BLK	d3, d5, d7, d9
				TRANS4X4BLK	d10, d12, d14, d16				
				TRANS4X4BLK	d11, d13, d15, d17

			@//swp block4x4 2, 3
				vswp	d3, d10
				vswp	d5, d12
				vswp	d7, d14
				vswp	d9, d16

			@//first stage
				FIRST_STAGE_X4_X5_X6_X7
				FIRST_STAGE_X4_X5_X6_X7_RND_SHIFT
				
			@//second stage
				SECOND_STAGE_X2
				vrshr.s32		q4, q4, #3		@//ronding and shift
				vrshr.s32		q11, q11, #3
				vst1.32		{d8, d9}, [pSrc]!	@//str x2
				vst1.32		{d22, d23}, [pSrc]!

				SECOND_STAGE_X3
				vrshr.s32		q4, q4, #3		@//ronding and shift
				vrshr.s32		q11, q11, #3
				vst1.32		{d8, d9}, [pSrc]!	@//str x3
				vst1.32		{d22, d23}, [pSrc]!

				SECOND_STAGE_REMAIN	#0x2000, #8

			@//third stage
				THIRD_STAGE

			@//fourth stage
				@//str line1, clip[(x3+x2)>>14]
				add		pDst0, pDst, dst_width
				@//str line6, clip[(x3-x2)>>14]
				add		pDst1, pDst, dst_width, lsl #2
				add		pDst1, pDst1, dst_width, lsl #1
				STORE_TWO_LINE_VER_C	q1, q2, q3, q8, pDst0, pDst1

				@//str line2, clip[(x0+x4)>>14]
				add		pDst0, pDst, dst_width, lsl #1
				@//str line5, clip[(x0-x4)>>14]
				add		pDst1, pDst, dst_width, lsl #2
				add		pDst1, pDst1, dst_width
				STORE_TWO_LINE_VER_C	q9, q11, q10, q7, pDst0, pDst1

				@//
				vld1.32	{d2, d3}, [pSrc]!	@//load x1
				vld1.32	{d6, d7}, [pSrc]!
				vld1.32	{d4, d5}, [pSrc]!	@//load x6
				vld1.32	{d16, d17}, [pSrc]!

				@//str line0, clip[(x7+x1)>>14]
				@//str line7, clip[(x7-x1)>>14]
				add		pDst1, pDst, dst_width, lsl #3
				sub		pDst1, pDst1, dst_width
				STORE_TWO_LINE_VER_C	q1, q12, q3, q15, pDst, pDst1

				@//str line3, clip[(x8+x6)>>14]
				add		pDst0, pDst, dst_width, lsl #2
				sub		pDst0, pDst0, dst_width
				@//str line4, clip[(x8-x6)>>14]
				add		pDst1, pDst, dst_width, lsl #2
				STORE_TWO_LINE_VER_C	q2, q4, q8, q5, pDst0, pDst1

				pop		{r4, r12, pc}
				@ENDFUNC


@/////////////////////////////////////////////////////////////////////////////////////////////////
@//input: 8x8 block dct coefficient
@//	r0: input block's pointer
@//	r1: point to the stored address in construction frame
@//	r2: output data width
@//	r3: point to the corresponding address in reference frame

arm_IDCTiFix:	@FUNCTION
				.global arm_IDCTiFix

				push		{r4 - r7, r12, r14}

				ldr		ref_width, [sp, #24]

@//horizontal transform
				HORIZONTAL_TRANS

@//vertical transform
				ldr		t1, =idct_coeff_neon
				vld1.8	{d0, d1}, [t1]

			@//load all 64 coefficient
				LOAD_ALL_COEFFICIENT

				TRANS4X4BLK	d2, d4, d6, d8
				TRANS4X4BLK	d3, d5, d7, d9
				TRANS4X4BLK	d10, d12, d14, d16				
				TRANS4X4BLK	d11, d13, d15, d17

			@//swp block4x4 2, 3
				vswp	d3, d10
				vswp	d5, d12
				vswp	d7, d14
				vswp	d9, d16

			@//first stage
				FIRST_STAGE_X4_X5_X6_X7
				FIRST_STAGE_X4_X5_X6_X7_RND_SHIFT
				
			@//second stage
				SECOND_STAGE_X2
				vrshr.s32		q4, q4, #3		@//rounding and shift
				vrshr.s32		q11, q11, #3
				vst1.32		{d8, d9}, [pSrc]!	@//str x2
				vst1.32		{d22, d23}, [pSrc]!

				SECOND_STAGE_X3
				vrshr.s32		q4, q4, #3		@//rounding and shift
				vrshr.s32		q11, q11, #3
				vst1.32		{d8, d9}, [pSrc]!	@//str x3
				vst1.32		{d22, d23}, [pSrc]!

				SECOND_STAGE_REMAIN	#0x2000, #8

			@//third stage
				THIRD_STAGE

			@//fourth stage
				@//str line1, clip[(x3+x2)>>14]
				add		pDst0, pDst, dst_width
				add		pRef0, pRef, ref_width
				@//str line6, clip[(x3-x2)>>14]
				add		pDst1, pDst, dst_width, lsl #2
				add		pDst1, pDst1, dst_width, lsl #1
				add		pRef1, pRef, ref_width, lsl #2
				add		pRef1, pRef1, ref_width, lsl #1
				STORE_TWO_LINE_VER_I	q1, q2, q3, q8, pDst0, pDst1, pRef0, pRef1

				@//str line2, clip[(x0+x4)>>14]
				add		pDst0, pDst, dst_width, lsl #1
				add		pRef0, pRef, ref_width, lsl #1
				@//str line5, clip[(x0-x4)>>14]
				add		pDst1, pDst, dst_width, lsl #2
				add		pDst1, pDst1, dst_width
				add		pRef1, pRef, ref_width, lsl #2
				add		pRef1, pRef1, ref_width
				STORE_TWO_LINE_VER_I	q9, q11, q10, q7, pDst0, pDst1, pRef0, pRef1

				@//
				vld1.32	{d2, d3}, [pSrc]!	@//load x1
				vld1.32	{d6, d7}, [pSrc]!
				vld1.32	{d4, d5}, [pSrc]!	@//load x6
				vld1.32	{d16, d17}, [pSrc]!

				@//str line0, clip[(x7+x1)>>14]
				@//str line7, clip[(x7-x1)>>14]
				add		pDst1, pDst, dst_width, lsl #3
				sub		pDst1, pDst1, dst_width
				add		pRef1, pRef, ref_width, lsl #3
				sub		pRef1, pRef1, ref_width
				STORE_TWO_LINE_VER_I	q1, q12, q3, q15, pDst, pDst1, pRef, pRef1

				@//str line3, clip[(x8+x6)>>14]
				add		pDst0, pDst, dst_width, lsl #2
				sub		pDst0, pDst0, dst_width
				add		pRef0, pRef, ref_width, lsl #2
				sub		pRef0, pRef0, ref_width
				@//str line4, clip[(x8-x6)>>14]
				add		pDst1, pDst, dst_width, lsl #2
				add		pRef1, pRef, ref_width, lsl #2
				STORE_TWO_LINE_VER_I	q2, q4, q8, q5, pDst0, pDst1, pRef0, pRef1

				pop		{r4 - r7, r12, pc}
				@ENDFUNC
					
				@AREA	DATA1, DATA, READONLY
				.align	4
idct_coeff_neon:	
				@.short	W1, W2, W3, W4, W5, W6, W7, W4c
				.short	2841, 2676, 2408, 2048, 1609, 1108, 565, 1

				.end

