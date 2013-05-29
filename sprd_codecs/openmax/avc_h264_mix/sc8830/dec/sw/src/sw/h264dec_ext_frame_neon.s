
@//this asm file is for extend reference frame
			.text
					
			.arm

@//void H264Dec_extent_frame (DEC_IMAGE_PARAMS_T *img_ptr, DEC_STORABLE_PICTURE_T * dec_picture)

img_ptr			.req	r0
dec_picture		.req		r1

width			.req		r2
width_c			.req		r3
uv				.req		r4
ext_width		.req		r5
ext_width_0		.req		r6
ext_width_c		.req		r7
ext_width_c_0	.req		r8

pSrc1			.req		r9
pSrc2			.req		r10
pDst1			.req		r11
pDst2			.req		r12

i				.req 		r14

.equ	Y_EXTEND_SIZE,		24
.equ	UV_EXTEND_SIZE,		12

H264Dec_extent_frame:	@FUNCTION
			.global	H264Dec_extent_frame
			push	{r4 -r12, r14}

			ldrh	width, [img_ptr, #0x24]
			mov		width_c, width, lsr #1
			add		ext_width_0, width, #Y_EXTEND_SIZE
			add		ext_width, ext_width_0, #Y_EXTEND_SIZE
			mov		ext_width_c, ext_width, lsr #1
			sub		ext_width_c_0, ext_width_c, #12

		@//horizontal repeat
			@//y
			ldrh	r14, [img_ptr, #0x2c]	@//img_ptr->start_in_frameY
			ldr		pSrc1, [dec_picture, #0]
			add		pSrc1, pSrc1, r14
			sub		pDst1, pSrc1, #Y_EXTEND_SIZE
			add		pDst2, pSrc1, width
			sub		pSrc2, pDst2, #1

			ldrh	i, [img_ptr, #0x26]	@//height
loop_y_hor:
			vld1.8	{d0[0]}, [pSrc1],ext_width
			vld1.8	{d2[0]}, [pSrc2],ext_width

			vdup.8	q0, d0[0]
			vdup.8	q1, d2[0]

			vst1.8	{d0, d1}, [pDst1]!	@//left
			vst1.8	d0, [pDst1]!

			vst1.8	{d2, d3}, [pDst2]!	@//right
			vst1.8	d2, [pDst2]!

			add		pDst1, pDst1, ext_width_0
			add		pDst2, pDst2, ext_width_0

			subs	i, i, #1
			bne		loop_y_hor

			@//uv
			mov		uv, #2
			ldr 	pSrc1, [dec_picture, #4]	@//u	
loop_uv_0:			
			ldrh	r14, [img_ptr, #0x2e]		@//img_ptr->start_in_frameUV
			add		pSrc1, pSrc1, r14
			sub		pDst1, pSrc1, #UV_EXTEND_SIZE
			add		pDst2, pSrc1, width_c
			ldrh	i, [img_ptr, #0x26] @//height
			sub		pSrc2, pDst2, #1

			mov		i, i, lsr #1
loop_uv_hor:
			vld1.8	{d0[0]}, [pSrc1], ext_width_c
			vld1.8	{d2[0]}, [pSrc2], ext_width_c

			vdup.8	d0, d0[0]
			vdup.8	d2, d2[0]

			vst1.8	{d0}, [pDst1]!	@//left
			vst1.32	d0[0], [pDst1]!

			vst1.8	{d2}, [pDst2]!	@//right
			vst1.32	d2[0], [pDst2]!

			add		pDst1, pDst1, ext_width_c_0
			add		pDst2, pDst2, ext_width_c_0

			subs	i, i, #1
			bne		loop_uv_hor
			
			ldr		pSrc1, [dec_picture, #8]	@//v

			subs	uv, uv, #1
			bne		loop_uv_0

		@//vertical repeat
			@//y
			ldr		pDst1, [dec_picture, #0]
			ldrh	r14, [img_ptr, #0x26] @//height
			add		pSrc1, pDst1, ext_width, lsl #4
			add		pSrc1, pSrc1, ext_width, lsl #3
			mla		pDst2, ext_width, r14, pSrc1 
			sub		pSrc2, pDst2, ext_width			

			mov		r6, ext_width, lsl #4
			add		r6, r6, ext_width, lsl #3
			sub		r6, r6, #16
			
			mov		r8, ext_width
loop_y_ver:
			vld1.8	{d0, d1}, [pSrc1]!
			vld1.8	{d2, d3}, [pSrc2]!

			mov		i, #Y_EXTEND_SIZE
loop_y_0:
			vst1.8	{d0, d1}, [pDst1], ext_width
			vst1.8	{d0, d1}, [pDst1], ext_width
			vst1.8	{d0, d1}, [pDst1], ext_width
			
			vst1.8	{d2, d3}, [pDst2], ext_width
			vst1.8	{d2, d3}, [pDst2], ext_width
			vst1.8	{d2, d3}, [pDst2], ext_width

			subs	i, i, #3
			bne		loop_y_0

			sub		pDst1, r6
			sub		pDst2, r6

			subs	r8, r8, #16
			bne		loop_y_ver

			@//uv
			mov		uv, #2
			ldr 	pDst1, [dec_picture, #4]	@//u
loop_uv_1:
			ldrh	r14, [img_ptr, #0x26] @//height
			add		pSrc1, pDst1, ext_width_c, lsl #3
			add		pSrc1, pSrc1, ext_width_c, lsl #2
			mov 	r14, r14, lsr #1
			mla		pDst2, ext_width_c, r14, pSrc1
			sub		pSrc2, pDst2, ext_width_c		

			mov 	r6, ext_width_c, lsl #3
			add 	r6, r6, ext_width_c, lsl #2
			sub 	r6, r6, #8
						
			mov 	r8, ext_width_c
loop_uv_ver_0:
			vld1.8	{d0}, [pSrc1]!
			vld1.8	{d1}, [pSrc2]!
			
			mov 	i, #UV_EXTEND_SIZE
loop_uv_ver_1:
			vst1.8	{d0}, [pDst1], ext_width_c
			vst1.8	{d0}, [pDst1], ext_width_c
			vst1.8	{d0}, [pDst1], ext_width_c
						
			vst1.8	{d1}, [pDst2], ext_width_c
			vst1.8	{d1}, [pDst2], ext_width_c
			vst1.8	{d1}, [pDst2], ext_width_c
			
			subs	i, i, #3
			bne 	loop_uv_ver_1
			
			sub 	pDst1, r6
			sub 	pDst2, r6
			
			subs	r8, r8, #8
			bne 	loop_uv_ver_0

			ldr 	pDst1, [dec_picture, #8]	@//v
			subs	uv, uv, #1
			bne		loop_uv_1

			pop 	{r4 - r12, pc}
			@ENDFUNC
			
			.end
				
