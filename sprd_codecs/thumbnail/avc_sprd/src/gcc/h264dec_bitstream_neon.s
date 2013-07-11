@//this asm file is for bitstream operation

			.text
					
			.arm

@//			.extern	g_image_ptr
@//typedef struct bitstream
@//{
@//	uint32 bitcnt;
@//	uint32 bitsLeft;	//left bits in the word pointed by rdptr
@//	uint32 *rdptr;
@//	uint32 bitcnt_before_vld;
@//}DEC_BS_T;

.equ	error_flag_POSITION, 16
.equ	ER_BS_UE,			1
.equ	ER_BS_SE,			2

@//int32 READ_UE_V(DEC_BS_T *stream)
bitcnt		.req	r1
bitsLeft		.req	r2
str_ptr		.req	r3
code			.req	r4

READ_UE_V:	@FUNCTION
		.global	READ_UE_V
		
		push		{r4, r14}
		@//pre_read 32 bit
		ldr		str_ptr, [r0, #(2*4)]
		ldr		bitsLeft, [r0, #(1*4)]

		@//pre-read 32 bit
		ldr		code, [str_ptr]
		rsb		r14, bitsLeft, #32
		ldr		bitcnt, [str_ptr, #4]

		mov		code, code, lsl r14
		orrs		code, code, bitcnt, lsr bitsLeft
                beq         read_ue_v_skip

		clz		r14, code	@//calc leading zero
		cmp		r14, #16
		bgt		read_ue_v_skip
		mov		r12, r14, lsl #1
		ldr		bitcnt, [r0]
		add		r12, r12, #1
		add		bitcnt, bitcnt, r12
		str		bitcnt, [r0]
		subs		bitsLeft, bitsLeft, r12
		addle	bitsLeft, bitsLeft, #32
		addle	str_ptr, str_ptr, #4
		rsb		r12, r12, #32
		str		bitsLeft, [r0, #(1*4)]
		str		str_ptr, [r0, #(2*4)]
		mov		r0, code, lsr	r12
		mov		r12, #1
		mov		r12, r12, lsl r14
		sub		r12, r12, #1	@//(1<<n)-1
		and		r0, r0, r12
		add		r0, r0, r12
		b		read_ue_v_exit
read_ue_v_skip:
		ldr		bitcnt, [r0, #error_flag_POSITION]
		orr		bitcnt, bitcnt, #ER_BS_UE
		strb		bitcnt, [r0, #error_flag_POSITION]
		mov		r0, #0
read_ue_v_exit:		
		pop		{r4, pc}
		@ENDFUNC

@//int32 READ_SE_V (DEC_BS_T *stream)
bitcnt		.req	r1
bitsLeft		.req	r2
str_ptr		.req	r3
code			.req	r4
READ_SE_V: 	@FUNCTION
		.global	READ_SE_V 
		
		push		{r4, r14}
		@//pre_read 32 bit
		ldr		str_ptr, [r0, #(2*4)]
		ldr		bitsLeft, [r0, #(1*4)]

		@//pre_read 32 bit
		ldr		code, [str_ptr]
		rsb		r14, bitsLeft, #32
		ldr		bitcnt, [str_ptr, #4]

		mov		code, code, lsl r14
		orrs		code, code, bitcnt, lsr bitsLeft
		beq          read_se_v_skip

		clz		r14, code	@//calc leading zero
		cmp		r14, #16
		bgt		read_se_v_skip
		mov		r12, r14, lsl #1
		ldr		bitcnt, [r0]
		add		r12, r12, #1
		add		bitcnt, bitcnt, r12
		str		bitcnt, [r0]
		subs		bitsLeft, bitsLeft, r12
		addle	bitsLeft, bitsLeft, #32
		addle	str_ptr, str_ptr, #4
		rsb		r12, r12, #32
		str		bitsLeft, [r0, #(1*4)]
		str		str_ptr, [r0, #(2*4)]
		mov		r0, code, lsr r12
		mov		r12, #1
		mov		r12, r12, lsl r14
		sub		r12, r12, #1	@//(1<<n)-1
		and		r0, r0, r12
		add		r4, r0, r12

		mov		r0, r4, lsr #1
		ands		r14, r4, #0x1
		addne	r0, r0, #1
		rsbeq	r0, r0, #0
		b 		read_se_v_exit
read_se_v_skip:
		ldr		bitcnt, [r0, #error_flag_POSITION]	
		orr		bitcnt, bitcnt, #ER_BS_SE
		str		bitcnt, [r0, #error_flag_POSITION]
		mov		r0, #0
read_se_v_exit:		
		pop		{r4, pc}
		@ENDFUNC


@//int32 READ_BITS1 (DEC_BS_T *stream)
READ_BITS1:	@FUNCTION
		.global	READ_BITS1
		
		push	{r14}
		@//pre_read 32 bit
		ldr		r1, [r0, #(2*4)]	@//stream address
		ldmia	r0, {r2, r3}
		ldr		r14, [r1]	@//ldr stream
		add		r2, r2, #1
		subs		r3, r3, #1
		mov		r14, r14, lsr r3
		moveq	r3, #32
		addeq	r1, r1, #4
		stmia	r0!, {r2, r3}
		str		r1, [r0]
		and		r0, r14, #0x1
		pop		{pc}
		@ENDFUNC

@//int32 READ_NBITS(DEC_BS_T *stream)
bit			.req	r1
bit_cnt		.req	r12
bitsLeft		.req	r2
str_ptr		.req	r3
cw			.req	r14

READ_BITS:	@FUNCTION
		.global	READ_BITS
		
		push	{r14}
		@//pre_read 32 bit
		ldr		str_ptr, [r0, #(2*4)]
		ldr		bitsLeft, [r0, #(1*4)]
		ldr		cw, [str_ptr]
		rsb		bit_cnt, bitsLeft, #32
		mov		cw, cw, lsl bit_cnt
		ldr		bit_cnt, [str_ptr, #4]

		add		str_ptr, str_ptr, #4
		
		orr		cw, cw, bit_cnt, lsr	bitsLeft
		ldr		bit_cnt, [r0, #0]
		subs		bitsLeft, bitsLeft, bit
		add		bit_cnt, bit_cnt, bit
		str		bit_cnt, [r0, #0]

		addle	bitsLeft, bitsLeft, #32
		strle		str_ptr, [r0, #(2*4)]
		str		bitsLeft, [r0, #4]

		rsb		bit, bit, #32
		mov		r0, cw, lsr bit
		
		pop		{pc}
		@ENDFUNC

@//int32 get_unit (H264DecContext *img_ptr, uint8 *pInStream, int32 frm_bs_len, int32 *slice_unit_len)
img_ptr				.req		r0
pInStream			.req 		r1		
frm_bs_len			.req		r2
slice_unit_len_ptr		.req 		r3
dec_len				.req 		r12
nalu_ptr				.req 		r12
data 				.req 		r4
len					.req 		r5
bfr					.req 		r6
zero_num			.req 		r7
stuffing_num			.req 		r8
startCode_len			.req 		r9

cw_data				.req 		r10
byte_rest				.req 		r11

get_unit:		@FUNCTION
		.global	get_unit
		@.extern	g_need_back_last_word
		@.extern g_back_last_word

		push		{r4 - r11, r14}
		mov		bfr, pInStream
@//		ldr		nalu_ptr, [slice_unit_len_ptr]
		mov		len, #1
		mov		startCode_len, #0
get_unit_loop0:
		ldrb		data, [pInStream], #1
		mov		stuffing_num, #0
		mov		byte_rest, #4
		cmp		data, #0
		addeq	len, len, #1
		beq		get_unit_loop0

		sub		dec_len, frm_bs_len, len
		mov		bfr, bfr, lsr #2
		mov		bfr, bfr, lsl #2

		ldr		r14, [img_ptr, #84]	@//nalu_ptr
		str 		bfr, [r14]	
		mov		zero_num, #0

get_unit_loop1:
		ldrb		data, [pInStream], #1

		cmp		zero_num, #2
		bge		get_unit_loop1_brk0
		add		zero_num, zero_num, #1
		subs		byte_rest, byte_rest, #1
		movge	cw_data, cw_data, lsl #8
		orrge	cw_data, cw_data, data
		moveq	byte_rest, #4
		streq	cw_data, [bfr], #4

		cmp		data, #0
		movne	zero_num, #0
		subs		dec_len, dec_len, #1
		bgt		get_unit_loop1
		b		get_unit_loop1_end
get_unit_loop1_brk0:
		cmpeq	data, #0x3
		bne		get_unit_loop1_brk1
		moveq	zero_num, #0
		addeq	stuffing_num, stuffing_num, #1
		subs		dec_len, dec_len, #1
		bgt		get_unit_loop1
		b 		get_unit_loop1_end
get_unit_loop1_brk1:
		subs		byte_rest, byte_rest, #1
		movge	cw_data, cw_data, lsl #8
		orrge	cw_data, cw_data, data
		moveq	byte_rest, #4
		streq	cw_data, [bfr], #4

		subs		data, data, #1
		movgt	zero_num, #0
		addlt	zero_num, zero_num, #1
		bne		get_unit_loop1_brk2
		add		startCode_len, zero_num, #1
		b 		get_unit_loop1_end
get_unit_loop1_brk2:
		subs		dec_len, dec_len, #1
		bgt		get_unit_loop1
get_unit_loop1_end:
		sub		r14, pInStream, startCode_len	@//startcode_len
		bic		r14, r14, #3
	@//	ldr		r7, =g_need_back_last_word	
		cmp		bfr, r14
		movne		r14, #0
		moveq		r14, #1
		str		r14, [img_ptr, #64]	@//g_need_back_last_word
		bne		not_need_backup_last_word
		ldr		r14, [bfr]
	@//	ldr		r7, =g_back_last_word
		str		r14, [img_ptr, #68]	@//g_back_last_word

not_need_backup_last_word:
		mov		byte_rest, byte_rest, lsl #3
		mov		r14, cw_data, lsl byte_rest
		str		r14, [bfr], #4

		cmp		dec_len, #0
		moveq	dec_len, #1
		sub		dec_len, frm_bs_len, dec_len
		add		dec_len, dec_len, #1
		sub		len, dec_len, len

		sub		r14, dec_len, startCode_len
		str		r14, [slice_unit_len_ptr]

		sub		r14, len, startCode_len
		sub		r14, r14, stuffing_num

remove_tailing_zero:
		cmp		cw_data, #0
		beq		str_nalu_len
		ands		r7, cw_data, #0xff
		bne		str_nalu_len
		sub		r14, r14, #1
		mov		cw_data, cw_data, asr #8
		b		remove_tailing_zero
str_nalu_len:		
		ldr		r7, [img_ptr, #84]	@//nalu_ptr
		str		r14, [r7, #4]

		cmp		dec_len, frm_bs_len
		movge	r0, #1
		movlt	r0, #0

		pop		{r4 - r11, pc}
		@ENDFUNC


		.end
