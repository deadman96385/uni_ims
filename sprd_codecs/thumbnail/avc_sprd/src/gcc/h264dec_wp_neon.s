
@//this asm file is for weighting prediction
			.text
					
			.arm

ref_idx_ptr0		.req		r0
src_ptr0			.req		r1
src_ptr1			.req		r12
shift				.req		r2
mem_ptr			.req		r3
b8map_ptr		.req		r3
b8pdir_ptr		.req 		r5
g_wp_offset_ptr	.req		r6
ROUND			.req		r12
width			.req		r4

@//void H264Dec_wp_y (int8 *ref_idx_ptr0, uint8 *src_ptr0, int32 shift, DEC_WP_MEM_INFO_T *mem_ptr)
H264Dec_wp_y:	@FUNCTION
			.global	H264Dec_wp_y
			
			push			{r4 -r11, r14}
			sub			sp, sp, #8*4
			@///////////////////////////
			ldmia		r3, {r3 - r8}
			stmia		sp, {r3 - r8}

			mov			ROUND, #1
			mov			ROUND, ROUND, lsl shift
			vdup.32		q6, ROUND
			add			shift, shift, #1
			rsb			shift, #0		@//-shift
			vdup.32		q7, shift
			mov			width, #16

			mov			r14, #4
y_row_loop:	
			str 			r14, [sp, #6*4]	@//str row
			mov			r14, #4
y_col_loop:
			@//r7, r8, r9, r10, r11, r12 can be used
			ldrb			r7, [b8map_ptr], #1		@//b8 = *b8map_ptr++;//b8map[blk4x4_id]
			ldr			b8pdir_ptr, [sp, #2*4]
			ldr			g_wp_offset_ptr, [sp, #3*4]
			ldrb			r7, [b8pdir_ptr, r7]		@//pdir = b8pdir_ptr[b8]
			ldrb			r9, [ref_idx_ptr0, #60]		@//r9: mc_ref_idx1 = ref_idx_ptr0[60]
			ldrb			r8, [ref_idx_ptr0], #1		@//r8: mc_ref_idx0 = *ref_idx_ptr0++
			subs			r7, r7, #1				@//
			ble			y_col_loop_brk0
			@//bi-dir
			add			r8, r8, r8, lsl #1
			add			r7, g_wp_offset_ptr, r8, lsl #1
			ldrsh		r7, [r7]					@//f7->offset[0]

			add			r9, r9, r9, lsl #1
			add			r10, g_wp_offset_ptr, r9, lsl #1
			ldrsh		r10, [r10, #0x60]			@//r10->offset[1]

			add			r7, r10, r7
			ldr			r10, [sp, #(4*4)]
			add			r7, r7, #1
			mov			r7, r7, asr #1		@//offset_val = ((offset[0] + offset[1]+1)>>1
			vdup.32		q8, r7			@//offset_val
			add			r10, r10, r8, lsl #5
			add			r10, r10, r9, lsl #1
			ldrsh		r8, [r10]
			add			r10, r10, #0x600
			ldrsh		r9, [r10]
			vdup.16		d28, r8		@//w0
			vdup.16		d29, r9		@//w1

			mov			r11, #4
y_i_int_loop:
			vld1.32		d9[0], [src_ptr0]			@//fw
			add			src_ptr1, src_ptr0, #384
			vmovl.u8		q0, d9
			vld1.32		d10[0], [src_ptr1]			@//bw
			vmull.s16		q2, d0, d28	@//fw*w0
			vmovl.u8		q1, d10
			vadd.i32		q2, q2, q6	@//+rnd
			vmlal.s16		q2, d2, d29	@//+bw*w1
			vshl.s32		q0, q2, q7	@//>>shift
			vadd.s32		q0, q0, q8	@//+offset_val
			vqmovun.s32	d2, q0
			vqmovn.u16	d0, q1
			subs			r11, r11, #1
			vst1.32		d0[0], [src_ptr0], width

			bgt			y_i_int_loop
			sub			src_ptr0, src_ptr0, #60
			@//col loop control
			subs			r14, r14, #1
			bgt			y_col_loop
			b			y_col_loop_exit
y_col_loop_brk0:
			beq			y_col_loop_brk1
			@//list 0
			add			r8, r8, r8, lsl #1
			add			r7, g_wp_offset_ptr, r8, lsl #1
			ldr			r10, [sp, #(5*4)]	@//g_wp_offset[0]
			ldrsh		r7, [r7]			@//r7->offset[0]
			vdup.32		q8, r7			@//offset_val
			add			r10, r10, r8, lsl #1
			ldrsh		r8, [r10]
			mov			r11, #4
			mov			r8, r8, lsl #1		@//w
			vdup.16		d28, r8			@//w0
y_list0_int_loop:
			vld1.32		d9[0], [src_ptr0]	@//fw
			subs			r11, r11, #1
			vmovl.u8		q0, d9
			vmull.s16		q2, d0, d28		@//fw*w0
			vadd.i32		q2, q2, q6		@//+rnd
			vshl.s32		q0, q2, q7		@//>>shift
			vadd.s32		q0, q0, q8		@//+offset_val
			vqmovun.s32	d2, q0
			vqmovn.u16	d0, q1
			vst1.32		d0[0], [src_ptr0], width

			bgt			y_list0_int_loop
			sub			src_ptr0, src_ptr0, #60

			@//col loop control
			subs			r14, r14, #1
			bgt			y_col_loop
			b			y_col_loop_exit
y_col_loop_brk1:
			@//list 1
			add			r8, r9, r9, lsl #1
			add			r7, g_wp_offset_ptr, r8, lsl #1
			ldr			r10, [sp, #(5*4)]	@//g_wp_offset[0]
			ldrsh		r7, [r7, #0x60]		@//r7->offset[0]
			vdup.32		q8, r7				@//offset_val
			add			r10, r10, r8, lsl #1
			ldrsh		r8, [r10, #0x60]
			mov			r11, #4
			mov			r8, r8, lsl #1
			vdup.16		d28, r8				@//w0
y_list1_int_loop:
			add			src_ptr1, src_ptr0, #384
			vld1.32		d9[0], [src_ptr1]		@//fw
			subs			r11, r11, #1
			vmovl.u8		q0, d9
			vmull.s16		q2, d0, d28			@//fw*w0
			vadd.i32		q2, q2, q6			@//+rnd
			vshl.s32		q0, q2, q7			@//>>shift
			vadd.s32		q0, q0, q8			@//+offset_val
			vqmovun.s32	d2, q0
			vqmovn.u16	d0, q1
			vst1.32		d0[0], [src_ptr0], width

			bgt			y_list1_int_loop
			sub			src_ptr0, src_ptr0, #60

			@//col loop control
			subs			r14, r14, #1
			bgt			y_col_loop
y_col_loop_exit:
			@////////////////////////
			ldr			r14, [sp, #6*4]		@//ldr row
			add			src_ptr0, src_ptr0, #48
			add			ref_idx_ptr0, ref_idx_ptr0, #8
			subs			r14, r14, #1
			bgt			y_row_loop
				
			@////////////////////////
			add			sp, sp, #8*4		
			pop 			{r4 - r11, pc}
			@ENDFUNC


@//void H264Dec_wp_c (int8 *ref_idx_ptr0, uint8 *src_ptr0, int32 shift, DEC_WP_MEM_INFO_T *mem_ptr)
H264Dec_wp_c:	@FUNCTION
			.global		H264Dec_wp_c
			
			push			{r4 -r11, r14}
			sub			sp, sp, #8*4
			@///////////////////////////
			ldmia		r3, {r3 - r8}
			stmia		sp, {r3 - r8}

			mov			ROUND, #1
			mov			ROUND, ROUND, lsl shift
			vdup.32		q6, ROUND
			add			shift, shift, #1
			rsb			shift, #0		@//-shift
			vdup.32		q7, shift
			mov			width, #8

			mov			r14, #4
c_row_loop:	
			str 			r14, [sp, #6*4]	@//str row
			mov			r14, #4
c_col_loop:
			@//r7, r8, r9, r10, r11, r12 can be used
			ldrb			r7, [b8map_ptr], #1		@//b8 = *b8map_ptr++;//b8map[blk4x4_id]
			ldr			b8pdir_ptr, [sp, #2*4]
			ldr			g_wp_offset_ptr, [sp, #3*4]
			ldrb			r7, [b8pdir_ptr, r7]		@//pdir = b8pdir_ptr[b8]
			ldrb			r9, [ref_idx_ptr0, #60]		@//r9: mc_ref_idx1 = ref_idx_ptr0[60]
			ldrb			r8, [ref_idx_ptr0], #1		@//r8: mc_ref_idx0 = *ref_idx_ptr0++
			subs			r7, r7, #1				@//
			ble			c_col_loop_brk0
			@//bi-dir
			add			r8, r8, r8, lsl #1
			add			r7, g_wp_offset_ptr, r8, lsl #1
			ldrsh		r7, [r7]					@//f7->offset[0]

			add			r9, r9, r9, lsl #1
			add			r10, g_wp_offset_ptr, r9, lsl #1
			ldrsh		r10, [r10, #0x60]			@//r10->offset[1]

			add			r7, r10, r7
			ldr			r10, [sp, #(4*4)]
			add			r7, r7, #1
			mov			r7, r7, asr #1		@//offset_val = ((offset[0] + offset[1]+1)>>1
			vdup.32		q8, r7			@//offset_val
			add			r10, r10, r8, lsl #5
			add			r10, r10, r9, lsl #1
			ldrsh		r8, [r10]
			add			r10, r10, #0x600
			ldrsh		r9, [r10]
			vdup.16		d28, r8		@//w0
			vdup.16		d29, r9		@//w1

			mov			r11, #2
c_i_int_loop:
			vld1.16		d9[0], [src_ptr0]			@//fw
			add			src_ptr1, src_ptr0, #384
			vmovl.u8		q0, d9
			vld1.16		d10[0], [src_ptr1]		@//bw
			vmull.s16		q2, d0, d28	@//fw*w0
			vmovl.u8		q1, d10
			vadd.i32		q2, q2, q6	@//+rnd
			vmlal.s16		q2, d2, d29	@//+bw*w1
			vshl.s32		q0, q2, q7	@//>>shift
			vadd.s32		q0, q0, q8	@//+offset_val
			vqmovun.s32	d2, q0
			vqmovn.u16	d0, q1
			subs			r11, r11, #1
			vst1.16		d0[0], [src_ptr0], width

			bgt			c_i_int_loop
			sub			src_ptr0, src_ptr0, #(2*8-2)
			@//col loop control
			subs			r14, r14, #1
			bgt			c_col_loop
			b			c_col_loop_exit
c_col_loop_brk0:
			beq			c_col_loop_brk1
			@//list 0
			add			r8, r8, r8, lsl #1
			add			r7, g_wp_offset_ptr, r8, lsl #1
			ldr			r10, [sp, #(5*4)]	@//g_wp_offset[0]
			ldrsh		r7, [r7]			@//r7->offset[0]
			vdup.32		q8, r7			@//offset_val
			add			r10, r10, r8, lsl #1
			ldrsh		r8, [r10]
			mov			r11, #2
			mov			r8, r8, lsl #1		@//w
			vdup.16		d28, r8			@//w0
c_list0_int_loop:
			vld1.16		d9[0], [src_ptr0]	@//fw
			subs			r11, r11, #1
			vmovl.u8		q0, d9
			vmull.s16		q2, d0, d28		@//fw*w0
			vadd.i32		q2, q2, q6		@//+rnd
			vshl.s32		q0, q2, q7		@//>>shift
			vadd.s32		q0, q0, q8		@//+offset_val
			vqmovun.s32	d2, q0
			vqmovn.u16	d0, q1
			vst1.16		d0[0], [src_ptr0], width

			bgt			c_list0_int_loop
			sub			src_ptr0, src_ptr0, #(2*8-2)

			@//col loop control
			subs			r14, r14, #1
			bgt			c_col_loop
			b			c_col_loop_exit
c_col_loop_brk1:
			@//list 1
			add			r8, r9, r9, lsl #1
			add			r7, g_wp_offset_ptr, r8, lsl #1
			ldr			r10, [sp, #(5*4)]	@//g_wp_offset[0]
			ldrsh		r7, [r7, #0x60]		@//r7->offset[0]
			vdup.32		q8, r7				@//offset_val
			add			r10, r10, r8, lsl #1
			ldrsh		r8, [r10, #0x60]
			mov			r11, #2
			mov			r8, r8, lsl #1
			vdup.16		d28, r8				@//w0
c_list1_int_loop:
			add			src_ptr1, src_ptr0, #384
			vld1.16		d9[0], [src_ptr1]		@//fw
			subs			r11, r11, #1
			vmovl.u8		q0, d9
			vmull.s16		q2, d0, d28			@//fw*w0
			vadd.i32		q2, q2, q6			@//+rnd
			vshl.s32		q0, q2, q7			@//>>shift
			vadd.s32		q0, q0, q8			@//+offset_val
			vqmovun.s32	d2, q0
			vqmovn.u16	d0, q1
			vst1.16		d0[0], [src_ptr0], width

			bgt			c_list1_int_loop
			sub			src_ptr0, src_ptr0, #(2*8-2)

			@//col loop control
			subs			r14, r14, #1
			bgt			c_col_loop
c_col_loop_exit:
			@////////////////////////
			ldr			r14, [sp, #6*4]		@//ldr row
			add			src_ptr0, src_ptr0, #8
			add			ref_idx_ptr0, ref_idx_ptr0, #8
			subs			r14, r14, #1
			bgt			c_row_loop
				
			@////////////////////////
			add			sp, sp, #8*4		
			pop 			{r4 - r11, pc}
			@ENDFUNC			

			.end
				
