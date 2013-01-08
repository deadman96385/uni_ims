
@//this asm file is for ipred
			.text
					
			.arm

@//void copyLeftPredPixel (DEC_MB_CACHE_T *mb_cache_ptr,DEC_MB_INFO_T *mb_info_ptr)
copyLeftPredPixel:	@FUNCTION
			.global	copyLeftPredPixel
			push	{r4 - r6, r14}

			ldr	r14, =pred_offset_neon
			ldmia	r14, {r3 - r6}

			ldrb	r14, [r1, #8]
			cmp		r14, #0
			bne		trans8x8_mode
		@//y	
			add		r14, r0, r3
			mov		r2, #0x10
			vld1.8	{d0[0]}, [r14], r2
			vld1.8	{d0[1]}, [r14], r2
			vld1.8	{d0[2]}, [r14], r2
			vld1.8	{d0[3]}, [r14], r2

			vld1.8	{d0[4]}, [r14], r2
			vld1.8	{d0[5]}, [r14], r2
			vld1.8	{d0[6]}, [r14], r2
			vld1.8	{d0[7]}, [r14], r2

			vld1.8	{d1[0]}, [r14], r2
			vld1.8	{d1[1]}, [r14], r2
			vld1.8	{d1[2]}, [r14], r2
			vld1.8	{d1[3]}, [r14], r2

			vld1.8	{d1[4]}, [r14], r2
			vld1.8	{d1[5]}, [r14], r2
			vld1.8	{d1[6]}, [r14], r2
			vld1.8	{d1[7]}, [r14], r2

			add		r14, r0, #0xe70
			vst1.8	{d0, d1}, [r14]			
			b	uv
trans8x8_mode:			
			ldrb	 r14,[r0,#0xbef]
			strb	 r14,[r0,#0xe92]
uv:
			mov 	r2, #0x8
		@//u
			add 	r14, r0, r4
			vld1.8	{d0[0]}, [r14], r2
			vld1.8	{d0[1]}, [r14], r2
			vld1.8	{d0[2]}, [r14], r2
			vld1.8	{d0[3]}, [r14], r2
			
			vld1.8	{d0[4]}, [r14], r2
			vld1.8	{d0[5]}, [r14], r2
			vld1.8	{d0[6]}, [r14], r2
			vld1.8	{d0[7]}, [r14], r2
			add 	r14, r0, #0xe80
			vst1.8	{d0},	[r14]
			
		@//v
			add 	r14, r0, r5
			vld1.8	{d0[0]}, [r14], r2
			vld1.8	{d0[1]}, [r14], r2
			vld1.8	{d0[2]}, [r14], r2
			vld1.8	{d0[3]}, [r14], r2
			
			vld1.8	{d0[4]}, [r14], r2
			vld1.8	{d0[5]}, [r14], r2
			vld1.8	{d0[6]}, [r14], r2
			vld1.8	{d0[7]}, [r14], r2
			add 	r14, r0, r6
			vst1.8	{d0},	[r14]			
			
			pop 	{r4 - r6, pc}
			@ENDFUNC
	
			.align	4
pred_offset_neon:	
			.word	0xb7f, 0xc77, 0xcb7, 0xe88
			
			.end
				
