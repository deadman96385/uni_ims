
@//this asm file is for luma16xN motion compensation

@/************register map begin**************/
pRefFrame	.req			r0
pRecMB		.req			r1
width		.req			r2
dstWidth		.req			r3

i			.req			r4
@/************register map end**************/
					
			.text					
			.arm

@//void mc_xyfull_16x16 (uint8 *pRefFrame, uint8 *pRecMB, int32 width, int32 dstWidth)
mc_xyfull_16x16:	@FUNCTION
			.global		mc_xyfull_16x16
				
			push			{r4, r14}

			mov			i, #16
mc_xyfull_16x16_VER_LOOP:
			vld1.8		{d0, d1}, [pRefFrame], width	
			vld1.8		{d2, d3}, [pRefFrame], width	
			vld1.8		{d4, d5}, [pRefFrame], width	
			vld1.8		{d6, d7}, [pRefFrame], width	
			
			vst1.8		{d0, d1}, [pRecMB, :128], dstWidth
			vst1.8		{d2, d3}, [pRecMB, :128], dstWidth
			vst1.8		{d4, d5}, [pRecMB, :128], dstWidth
			vst1.8		{d6, d7}, [pRecMB, :128], dstWidth

			subs			i, i, #4
			bne			mc_xyfull_16x16_VER_LOOP

			pop			{r4,  pc}
			@ENDFUNC

@//void mc_xfullyhalf_16x16_rnd0 (uint8 *pRefFrame, uint8 *pRecMB, int32 width, int32 dstWidth)
mc_xfullyhalf_16x16_rnd0:	@FUNCTION
			.global		mc_xfullyhalf_16x16_rnd0
				
			push			{r4, r14}
				
			vld1.8		{d0, d1}, [pRefFrame], width
			mov			i, #16
mc_xfullyhalf_16x16_rnd0_VER_LOOP:
			vld1.8		{d2, d3}, [pRefFrame], width
			vrhadd.u8 	q9, q1, q0					@//(a+b+1)/2

			vld1.8		{d0, d1}, [pRefFrame], width
			vst1.8		{d18, d19}, [pRecMB, :128], dstWidth
			vrhadd.u8	q10, q0, q1					@//(a+b+1)/2

			vld1.8		{d2, d3}, [pRefFrame], width	
			vst1.8		{d20, d21}, [pRecMB, :128], dstWidth
			vrhadd.u8	q9, q1, q0					@//(a+b+1)/2

			vld1.8		{d0, d1}, [pRefFrame], width
			vst1.8		{d18, d19}, [pRecMB, :128], dstWidth
			vrhadd.u8	q10, q0, q1					@//(a+b+1)/2

			subs			i, i, #4	
			vst1.8		{d20, d21}, [pRecMB, :128], dstWidth

			bne			mc_xfullyhalf_16x16_rnd0_VER_LOOP

			pop		{r4, pc}
			@ENDFUNC

@//void mc_xfullyhalf_16x16_rnd1 (uint8 *pRefFrame, uint8 *pRecMB, int32 width, int32 dstWidth)
mc_xfullyhalf_16x16_rnd1:	@FUNCTION
			.global		mc_xfullyhalf_16x16_rnd1
				
			push			{r4, r14}
				
			vld1.8		{d0, d1}, [pRefFrame], width
			mov			i, #16
mc_xfullyhalf_16x16_rnd1_VER_LOOP:
			vld1.8		{d2, d3}, [pRefFrame], width
			vhadd.u8 	q9, q1, q0					@//(a+b+1)/2

			vld1.8		{d0, d1}, [pRefFrame], width
			vst1.8		{d18, d19}, [pRecMB, :128], dstWidth
			vhadd.u8		q10, q0, q1					@//(a+b+1)/2

			vld1.8		{d2, d3}, [pRefFrame], width	
			vst1.8		{d20, d21}, [pRecMB, :128], dstWidth
			vhadd.u8		q9, q1, q0					@//(a+b+1)/2

			vld1.8		{d0, d1}, [pRefFrame], width
			vst1.8		{d18, d19}, [pRecMB, :128], dstWidth
			vhadd.u8		q10, q0, q1					@//(a+b+1)/2

			subs			i, i, #4	
			vst1.8		{d20, d21}, [pRecMB, :128], dstWidth
			
			bne			mc_xfullyhalf_16x16_rnd1_VER_LOOP

			pop			{r4, pc}
			@ENDFUNC			

@//void mc_xhalfyfull_16x16_rnd0 (uint8 *pRefFrame, uint8 *pRecMB, int32 width, int32 dstWidth)
mc_xhalfyfull_16x16_rnd0:	@FUNCTION
			.global		mc_xhalfyfull_16x16_rnd0
				
			push			{r4, r14}
				
			mov			i, #16
mc_xhalfyfull_16x16_rnd0_VER_LOOP:
			add 			r14, pRefFrame, #16
			vld1.8		{d0, d1}, [pRefFrame], width	
			vld1.8 		{d2},[r14]
			vext.8		d4, d0, d1, #1					
			add 			r14, pRefFrame, #16
			vext.8		d5, d1, d2, #1					
			vld1.8 		{d8},[r14]		
			vrhadd.u8	q1, q0, q2						@//(a+b+1)/2
			vld1.8		{d6, d7}, [pRefFrame], width	
			vst1.8		{d2, d3}, [pRecMB, :128], dstWidth
			vext.8		d10, d6, d7, #1
			vext.8		d11, d7, d8, #1
			vrhadd.u8	q4, q3, q5						@//(a+b+1)/2
			subs			i, i, #2	
			vst1.8		{d8, d9}, [pRecMB, :128], dstWidth
			
			bne			mc_xhalfyfull_16x16_rnd0_VER_LOOP

			pop			{r4, pc}
			@ENDFUNC

@//void mc_xfullyhalf_16x16_rnd1 (uint8 *pRefFrame, uint8 *pRecMB, int32 width, int32 dstWidth)
mc_xhalfyfull_16x16_rnd1:	@FUNCTION
			.global		mc_xhalfyfull_16x16_rnd1
				
			push			{r4, r14}
				
			mov			i, #16
mc_xhalfyfull_16x16_rnd1_VER_LOOP:
			add 			r14, pRefFrame, #16
			vld1.8		{d0, d1}, [pRefFrame], width	
			vld1.8 		{d2},[r14]
			vext.8		d4, d0, d1, #1					
			add 			r14, pRefFrame, #16
			vext.8		d5, d1, d2, #1					
			vld1.8 		{d8},[r14]		
			vhadd.u8		q1, q0, q2						@//(a+b+1)/2
			vld1.8		{d6, d7}, [pRefFrame], width	
			vst1.8		{d2, d3}, [pRecMB, :128], dstWidth
			vext.8		d10, d6, d7, #1
			vext.8		d11, d7, d8, #1
			vhadd.u8		q4, q3, q5						@//(a+b+1)/2
			subs			i, i, #2	
			vst1.8		{d8, d9}, [pRecMB, :128], dstWidth

			bne 			mc_xhalfyfull_16x16_rnd1_VER_LOOP

			pop			{r4, pc}
			@ENDFUNC

@//void mc_xyhalf_16x16_rnd0 (uint8 *pRefFrame, uint8 *pRecMB, int32 width, int32 dstWidth)
mc_xyhalf_16x16_rnd0:	@ FUNCTION
			.global 	mc_xyhalf_16x16_rnd0

			push 		{r4,r14}

			add 			r14, pRefFrame , #16
			vld1.8 		{d0,d1},[pRefFrame],width @//line0
			vld1.8 		{d2},[r14]

			mov 			i, #16
mc_xyhalf_16x16_rnd0_VER_LOOP:
			vext.8 		d4, d0, d1, #1
        		vext.8 		d5, d1, d2, #1
			vaddl.u8 		q4,d0,d4
			vaddl.u8 		q5,d1,d5
			add 			r14, pRefFrame , #16
			vld1.8 		{d20,d21},[pRefFrame],width @//line1
			vld1.8 		{d22},[r14]
			vaddw.u8 	q4, q4,d20
			vaddw.u8 	q5, q5, d21
			vmov 		q0, q10
			vmov 		d2, d22
			vext.8 		d4, d0, d1, #1
        		vext.8 		d5, d1, d2, #1
			vext.8 		d24, d20, d21, #1
			vext.8 		d25, d21, d22, #1
			vaddw.u8 	q4,q4,d24
			vaddw.u8 	q5,q5,d25
			vqrshrn.u16 	d30, q4,#2
			vqrshrn.u16 	d31, q5,#2

			@//next line
			vaddl.u8 		q4,d0,d4
			vaddl.u8 		q5,d1,d5
			vst1.8 		{d30,d31},[pRecMB, :128],dstWidth @//save the first line
			add 			r14, pRefFrame , #16
			vld1.8 		{d22},[r14]
			vld1.8 		{d20,d21},[pRefFrame],width @//line2
			vmov 		d2, d22
			vaddw.u8 	q4, q4,d20
			vmov 		q0, q10
			vaddw.u8 	q5, q5, d21
			vext.8 		d24, d20, d21, #1
        		vext.8 		d25, d21, d22, #1
			vaddw.u8 	q4,q4,d24
			vaddw.u8 	q5,q5,d25
			vqrshrn.u16 	d30, q4,#2
			vqrshrn.u16 	d31, q5,#2

			subs 		i, i, #2
			vst1.8 		{d30,d31},[pRecMB, :128],dstWidth @//save the second line
			bne mc_xyhalf_16x16_rnd0_VER_LOOP
	
        		pop 			{r4,	pc} 
			@ENDFUNC
				
@//void mc_xyhalf_16x16_rnd1 (uint8 *pRefFrame, uint8 *pRecMB, int32 width, int32 dstWidth)
mc_xyhalf_16x16_rnd1:	@FUNCTION
			.global		mc_xyhalf_16x16_rnd1
				
			push			{r4, r14}

			mov 			r14,#1
			vdup.u8 		d6,r14
			
			add 			r14, pRefFrame , #16
			vld1.8 		{d0,d1},[pRefFrame],width @//line0
			vld1.8 		{d2},[r14]

			mov 			i, #16
mc_xyhalf_16x16_rnd1_VER_LOOP:
			vext.8 		d4, d0, d1, #1
        		vext.8 		d5, d1, d2, #1

			vaddl.u8 		q4,d0,d4
			vaddl.u8 		q5,d1,d5

			add 			r14, pRefFrame , #16

			vld1.8 		{d22},[r14]
			vld1.8 		{d20,d21},[pRefFrame],width @//line0

			vmov 		d2, d22
			vaddw.u8 	q4, q4,d20

			vmov 		q0, q10
			vaddw.u8 	q5, q5, d21

	
			vext.8 		d4, d0, d1, #1
        		vext.8 		d5, d1, d2, #1

			vext.8 		d24, d20, d21, #1
			vext.8 		d25, d21, d22, #1

			vaddw.u8 	q4,q4,d24
			vaddw.u8 	q5,q5,d25

			vaddw.u8 	q4,q4,d6
			vqshrn.u16 	d30, q4,#2
			vaddw.u8 	q5,q5,d6
			vqshrn.u16 	d31, q5,#2

			@//next line
			vaddl.u8 		q4,d0,d4
			vaddl.u8 		q5,d1,d5

			vst1.8 		{d30,d31},[pRecMB, :128],dstWidth @//save the first line

			add 			r14, pRefFrame , #16

			vld1.8 		{d22},[r14]
			vld1.8 		{d20,d21},[pRefFrame],width @//line1

			vmov 		d2, d22
			vaddw.u8 	q4, q4,d20

			vmov 		q0, q10
			vaddw.u8 	q5, q5, d21
	
			vext.8 		d24, d20, d21, #1
        		vext.8 		d25, d21, d22, #1
	
			vaddw.u8 	q4,q4,d24
			vaddw.u8 	q5,q5,d25
	
			vaddw.u8 	q4,q4,d6
			vqshrn.u16 	d30, q4,#2
			vaddw.u8 	q5,q5,d6
			vqshrn.u16 	d31, q5,#2

			subs 		i, i, #2
			vst1.8 		{d30,d31},[pRecMB, :128],dstWidth @//save the second line
			bne 			mc_xyhalf_16x16_rnd1_VER_LOOP
	
        		pop 			{r4,pc} 
			@ENDFUNC	
				
@//for 8x8 luma
@//void mc_xyfull_8x8 (uint8 *pRefFrame, uint8 *pRecMB, int32 width, int32 dstWidth)
mc_xyfull_8x8:	@FUNCTION
			.global		mc_xyfull_8x8
				
			push			{r4, r14}

			mov			i, #8
mc_xyfull_8x8_VER_LOOP:
			vld1.8		{d0}, [pRefFrame], width	
			vld1.8		{d1}, [pRefFrame], width	
			vld1.8		{d2}, [pRefFrame], width	
			vld1.8		{d3}, [pRefFrame], width	
			
			vst1.8		{d0}, [pRecMB, :64], dstWidth
			vst1.8		{d1}, [pRecMB, :64], dstWidth
			vst1.8		{d2}, [pRecMB, :64], dstWidth
			vst1.8		{d3}, [pRecMB, :64], dstWidth

			subs			i, i, #4
			bne			mc_xyfull_8x8_VER_LOOP

			pop			{r4,  pc}
			@ENDFUNC

@//void mc_xfullyhalf_8x8_rnd0 (uint8 *pRefFrame, uint8 *pRecMB, int32 width, int32 dstWidth)
mc_xfullyhalf_8x8_rnd0:	@FUNCTION
			.global		mc_xfullyhalf_8x8_rnd0
				
			push			{r4, r14}
				
			vld1.8		{d0}, [pRefFrame], width	@// 0
			mov			i, #8
mc_xfullyhalf_8x8_rnd0_VER_LOOP:
			vld1.8		{d1}, [pRefFrame], width	@ // 1     
			vrhadd.u8	d9,  d1, d0					@//(a+b+1)/2
						
			vld1.8		{d0}, [pRefFrame], width	@// 2
			vst1.8		{d9}, [pRecMB, :64], dstWidth
			vrhadd.u8 	d10, d0, d1					@//(a+b+1)/2	

			vld1.8		{d1}, [pRefFrame], width	@// 3
			vst1.8		{d10}, [pRecMB, :64], dstWidth
			vrhadd.u8	d9, d1, d0					@//(a+b+1)/2

			vld1.8		{d0}, [pRefFrame], width	@// 4
			vst1.8		{d9}, [pRecMB, :64], dstWidth
			vrhadd.u8	d10, d0, d1					@//(a+b+1)/2

			subs			i, i, #4
			vst1.8		{d10}, [pRecMB, :64], dstWidth

			bne			mc_xfullyhalf_8x8_rnd0_VER_LOOP

			pop			{r4, pc}
			@ENDFUNC

@//void mc_xfullyhalf_8x8_rnd1 (uint8 *pRefFrame, uint8 *pRecMB, int32 width, int32 dstWidth)
mc_xfullyhalf_8x8_rnd1:	@FUNCTION
			.global		mc_xfullyhalf_8x8_rnd1
				
			push			{r4, r14}
				
			vld1.8		{d0}, [pRefFrame], width	@// 0
			mov			i, #8
mc_xfullyhalf_8x8_rnd1_VER_LOOP:
			vld1.8		{d1}, [pRefFrame], width	@ // 1     
			vhadd.u8	d9,  d1, d0						@//(a+b+1)/2
						
			vld1.8		{d0}, [pRefFrame], width	@// 2
			vst1.8		{d9}, [pRecMB, :64], dstWidth
			vhadd.u8 	d10, d0, d1					@//(a+b+1)/2	

			vld1.8		{d1}, [pRefFrame], width	@// 3
			vst1.8		{d10}, [pRecMB, :64], dstWidth
			vhadd.u8		d9, d1, d0					@//(a+b+1)/2

			vld1.8		{d0}, [pRefFrame], width	@// 4
			vst1.8		{d9}, [pRecMB, :64], dstWidth
			vhadd.u8		d10, d0, d1						@//(a+b+1)/2

			subs			i, i, #4
			vst1.8		{d10}, [pRecMB, :64], dstWidth

			bne			mc_xfullyhalf_8x8_rnd1_VER_LOOP
			
			pop			{r4, pc}
			@ENDFUNC
				
@//void mc_xfullyhalf_8x8_rnd0 (uint8 *pRefFrame, uint8 *pRecMB, int32 width, int32 dstWidth)
mc_xhalfyfull_8x8_rnd0:	@FUNCTION
			.global		mc_xhalfyfull_8x8_rnd0
				
			push			{r4, r14}
				
			mov			i, #8
mc_xhalfyfull_8x8_rnd0_VER_LOOP:
			vld1.8		{d0,d1},[pRefFrame], width		@// 0
			vld1.8		{d10,d11},[pRefFrame], width	@// 1
			vext.8		d2, d0, d1, #1					
			vext.8		d12, d10, d11, #1				
						
			vrhadd.u8	d3, d0, d2					@//(a+b+1)/2
			vrhadd.u8	d13, d10, d12				@//(a+b+1)/2
			vst1.8		{d3}, [pRecMB, :64], dstWidth
			vst1.8		{d13}, [pRecMB, :64], dstWidth

			subs			i, i, #2	
			bne			mc_xhalfyfull_8x8_rnd0_VER_LOOP

			pop		{r4, pc}
			@ENDFUNC

@//void mc_xfullyhalf_8x8_rnd1 (uint8 *pRefFrame, uint8 *pRecMB, int32 width, int32 dstWidth)
mc_xhalfyfull_8x8_rnd1:	@FUNCTION
			.global		mc_xhalfyfull_8x8_rnd1
				
			push			{r4, r14}
				
			mov			i, #8
mc_xhalfyfull_8x8_rnd1_VER_LOOP:
			vld1.8		{d0,d1},[pRefFrame], width		@// 0
			vld1.8		{d10,d11},[pRefFrame], width	@// 1
			vext.8		d2, d0, d1, #1				
			vext.8		d12, d10, d11, #1				
						
			vhadd.u8		d3, d0, d2					@//(a+b+1)/2
			vhadd.u8		d13, d10, d12				@//(a+b+1)/2
			vst1.8		{d3}, [pRecMB, :64], dstWidth
			vst1.8		{d13}, [pRecMB, :64], dstWidth

			subs			i, i, #2	
			bne			mc_xhalfyfull_8x8_rnd1_VER_LOOP

			pop		{r4, pc}
			@ENDFUNC

@//void mc_xyhalf_8x8_rnd0 (uint8 *pRefFrame, uint8 *pRecMB, int32 width, int32 dstWidth)
mc_xyhalf_8x8_rnd0:	@FUNCTION
			.global		mc_xyhalf_8x8_rnd0
				
			push			{r4, r14}

			vld1.8		{d0,d1}, [pRefFrame], width	@//v128[0]
			mov			i, #8
			vext.8		d4, d0, d1, #1				@//v128[1]
			
mc_xyhalf_8x8_rnd0_VER_LOOP:			
			vld1.8		{d2,d3}, [pRefFrame], width	@//v128[0]
			vaddl.u8		q4, d0, d4
			vext.8		d5, d2, d3, #1					@//v128[1]
			vaddw.u8	q4, q4, d2
			vmov		d0, d2
			vaddw.u8	q4, q4, d5
			vmov		d4,d5
			vqrshrn.u16	d10, q4, #2		
			subs			i, i, #1			
			vst1.8		{d10}, [pRecMB, :64], dstWidth		
	
			bne			mc_xyhalf_8x8_rnd0_VER_LOOP

			pop			{r4, pc}
			@ENDFUNC
			
@//void mc_xyhalf_8x8_rnd1 (uint8 *pRefFrame, uint8 *pRecMB, int32 width, int32 dstWidth)
mc_xyhalf_8x8_rnd1:	@FUNCTION
			.global		mc_xyhalf_8x8_rnd1
				
			push			{r4 - r6, r14}
      
      			mov 			r14, #1
      			vdup.u8  		d6,r14
      
			vld1.8		{d0,d1}, [pRefFrame], width		@// 0
			mov			i, #8
			vext.8		d4, d0, d1, #1					

mc_xyhalf_8x8_rnd1_VER_LOOP:			
			vld1.8		{d2,d3}, [pRefFrame], width		@// 1
			vaddl.u8		q4, d0, d4
			vext.8		d5, d2, d3, #1					
			vaddw.u8	q4, q4, d2
			vmov		d0, d2
			vaddw.u8	q4, q4, d5
			vmov		d4,d5
			vaddw.u8  	q4, q4, d6			
			vqshrn.u16	d10, q4, #2					@ (a+b+c+d+1)/4
			subs			i, i, #1	
			vst1.8		{d10}, [pRecMB, :64], dstWidth
			
			bne			mc_xyhalf_8x8_rnd1_VER_LOOP

			pop		{r4 - r6, pc}
			@ENDFUNC

@//void write_display_frame(DEC_VOP_MODE_T *vop_mode_ptr,DEC_FRM_BFR *pDecFrame);
vop_mode_ptr		.req			r0
pDecFrame		.req			r1

FrameWidth		.req			r2		
FrameHeight		.req			r3

iStartFrameY		.req			r4
iStartFrameUV		.req			r4

src				.req			r5
dst 				.req			r6

src_tmp			.req			r7
dst_tmp			.req			r8
src_u			.req			r7
src_v			.req			r8

row				.req			r9
col				.req			r10

FrameExtendWidth	.req			r11

write_display_frame1:	@FUNCTION
			.global 		write_display_frame1

			push			{r4 - r11, r14}

			@// Y
			ldrsh			FrameWidth, [vop_mode_ptr, #0xa]
			ldrsh			FrameHeight, [vop_mode_ptr, #0xc]
			ldrsh			FrameExtendWidth, [vop_mode_ptr, #0xe]
			
			ldr			src, [pDecFrame, #0]
			ldrsh			iStartFrameY, [vop_mode_ptr, #0x12]
			ldr			dst, [pDecFrame, #0xc]
			add			src, src, iStartFrameY
			
			mov			row, FrameHeight
Y_loop_row:
			mov			src_tmp, src
			pld			[src_tmp]	
			mov			dst_tmp, dst
			
			mov			col, FrameWidth	@//32 pix
Y_loop_col:
			vld4.u8		{d0, d1, d2, d3}, [src_tmp, :128]!
			subs			col, col, #32
			vst4.u8		{d0, d1, d2, d3}, [dst_tmp, :128]!
			pld			[src_tmp]	
			bhi			Y_loop_col

			add			src, src, FrameExtendWidth
			add			dst, dst, FrameWidth
			subs			row, row, #1
			bne			Y_loop_row

			@//U, V
			ldrsh			iStartFrameUV, [vop_mode_ptr, #0x14]
			ldr			src_u, [pDecFrame, #4]
			ldr			src_v, [pDecFrame, #8]
			ldr			dst, [pDecFrame, #0x10]
			add			src_u, src_u, iStartFrameUV
			add			src_v, src_v, iStartFrameUV

			pld			[src_u, src_v]
			mov			row, FrameHeight, asr #1
C_loop_row:
			mov			col, FrameWidth, asr #1
C_loop_col:				
			vld1.8		{d0}, [src_u, :64]!
			vld1.8		{d1}, [src_v, :64]!

			subs			col, col, #8
			vst2.8		{d0, d1}, [dst, :128]!
			bne			C_loop_col

			add			src_u, src_u, #16
			add			src_v, src_v, #16

			pld			[src_u, src_v]

			subs			row, row, #1
			bne			C_loop_row
			
			pop			{r4 - r11, pc}
			@ENDFUNC
			
			.end
				

