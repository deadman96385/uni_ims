
@//this asm file is for vld

			.text
@arm_streamBfr_update
@ input:
@	r0: stream
@ refill the stream buffer from bitstream

@/************register map begin**************/
stream_f0	.req			r0
rdbfr_f0	.req			r1
bs_ptr_f0	.req			r2
rdptr_f0	.req			r3
bs_left_f0	.req			r4

i_f0		.req			r14
@/************register map end**************/
arm_streamBfr_update:	@FUNCTION					
			.global	arm_streamBfr_update

			push	{r0 - r4, r14}

			add	rdbfr_f0, stream_f0, #0x14
			ldr	bs_ptr_f0, [stream_f0, #0x218]
			str	rdbfr_f0, [stream_f0, #0x10]

			mov	i_f0, #8
copy_64byte_f0:
			vld1.8	{d0, d1, d2, d3}, [bs_ptr_f0]!	@//8*4 = 32byte
			vld1.8	{d4, d5, d6, d7}, [bs_ptr_f0]!	@//8*4 = 32byte
			vst1.8	{d0, d1, d2, d3}, [rdbfr_f0]!
			vst1.8	{d4, d5, d6, d7}, [rdbfr_f0]!

			subs	i_f0, i_f0, #1
			bne	copy_64byte_f0

			@//update bs_left
			ldr	bs_left_f0, [stream_f0, #(4+129+3)*4]
			str	bs_ptr_f0, [stream_f0, #0x218]
			sub	bs_left_f0, bs_left_f0, #512
			str	bs_left_f0, [stream_f0, #(4+129+3)*4]

			pop	{r0 - r4, pc}
			@ENDFUNC
@/*
@int32 Mp4Dec_VlcDecIntraTCOEF(DEC_VOP_MODE_T *vop_mode_ptr, int16 *iCoefQ, int32 iCoefStart,char *pNonCoeffPos)
@	input:
@		r0: vop_mode_ptr
@		r1: iCoefQ
@		r2: iCoefStart
@		r3: pNoncoeffPos
@
@	description: vld intra block of mpeg4
@*/

@/************register map begin**************/
pNonCoeffPos_f1		.req	r12

tab_f1			.req	r0
run_f1			.req	r0
level_f1		.req	r1
sign_f1			.req	r2
tmp_f1			.req	r3
pZigzag_f1		.req	r4
last_f1			.req	r5	@//will used in whole process
code_f1			.req	r6
flushBits_f1		.req	r14
index_f1		.req	r6
i_f1			.req	r8

stream_f1		.req	r9
vop_mode_ptr_f1		.req	r10
iDCTCoeff_f1		.req	r11

nonCoeffNum_f1		.req	r7
@/************register map end**************/

.equ 	ERRORFLAG_OFFSET,	0x25
.equ 	DCT3DTAB3_OFFSET,	0xF0
.equ 	DCT3DTAB4_OFFSET,	0xF4
.equ 	DCT3DTAB5_OFFSET,	0xF8
.equ 	INTRA_MAX_LEVEL,	0x104
.equ 	INTRA_MAX_RUN,		0x108

Mp4Dec_VlcDecIntraTCOEF:	@FUNCTION
		.global	Mp4Dec_VlcDecIntraTCOEF

		push	{r4 - r12, r14}
	
		@/*initial*/
		mov	vop_mode_ptr_f1, r0
		mov	iDCTCoeff_f1, r1
		mov	i_f1, r2
		mov	pNonCoeffPos_f1, r3
		ldr	stream_f1, [vop_mode_ptr_f1, #0xc8]
		ldr	pZigzag_f1, [vop_mode_ptr_f1, #0x64]
		mov	last_f1, #0
		mov	nonCoeffNum_f1, #0

VLD_LOOP_f1:
		@//show next 32 bits in bitstream and store in reg (code)
		ldmia	stream_f1, {r0 - r2}	@//r0: bufa, r1: bufb, r2: bitsLeft
		rsb	r6, r2, #32
		mov	code_f1, r0, lsl r6
		orr	code_f1, code_f1, r1, lsr r2

		@//choose huffman table
		mov	tmp_f1, code_f1, lsr #20
		cmp	tmp_f1, #512
		bcc	DCT3TAB1_0_f1
		ldr	tab_f1, [vop_mode_ptr_f1, #DCT3DTAB3_OFFSET]
		mov	tmp_f1, tmp_f1, lsr #5
		add	tab_f1, tab_f1, tmp_f1, lsl #2
		sub	tab_f1, tab_f1, #64
		b	STEP1_f1

DCT3TAB1_0_f1:
		cmp	tmp_f1, #128
		bcc	DCT3TAB2_0_f1
		ldr	tab_f1, [vop_mode_ptr_f1, #DCT3DTAB4_OFFSET]
		mov	tmp_f1, tmp_f1, lsr #2
		add	tab_f1, tab_f1, tmp_f1, lsl #2
		sub	tab_f1, tab_f1, #128
		b	STEP1_f1

DCT3TAB2_0_f1:
		cmp	tmp_f1, #8
		bcc	ERROR_f1
		ldr	tab_f1, [vop_mode_ptr_f1, #DCT3DTAB5_OFFSET]
		add	tab_f1, tab_f1, tmp_f1, lsl #2
		sub	tab_f1, tab_f1, #32
		b	STEP1_f1

ERROR_f1:
		mov	tmp_f1, #1
		strb	tmp_f1, [vop_mode_ptr_f1, #ERRORFLAG_OFFSET]
		b	VLD_END_f1


		@//update code and recoder flush bits
		@ /******************************************/
STEP1_f1:
		ldrh	flushBits_f1, [tab_f1, #2]	@//flush_bits: tab->len
		ldrh	tmp_f1, [tab_f1, #0]		@//tmp: tab->code

		mov	code_f1, code_f1, lsl flushBits_f1
		
	@	mov	r0, #0x1bff
	@	cmp	tmp_f1, r0
		sub	r0, tmp_f1, #0x1b00
		subs	r0, r0, #0xff
		bne	NOT_ESCAPE_f1

		@//if escape mode, enter escape vld mode
ESCAPE_1_f1:
		movs	level_f1, code_f1, lsr #31
		mov	code_f1, code_f1, lsl #1
		add	flushBits_f1, flushBits_f1, #1
		bne	ESCAPE_2_f1

		mov	tmp_f1, code_f1, lsr #20
		cmp	tmp_f1, #512
		bcc	DCT3TAB1_1_f1
		ldr	tab_f1, [vop_mode_ptr_f1, #DCT3DTAB3_OFFSET]
		mov	tmp_f1, tmp_f1, lsr #5
		add	tab_f1, tab_f1, tmp_f1, lsl #2
		sub	tab_f1, tab_f1, #64
		b	UPDATE_ESCAP1_f1

DCT3TAB1_1_f1:
		cmp	tmp_f1, #128
		bcc	DCT3TAB2_1_f1
		ldr	tab_f1, [vop_mode_ptr_f1, #DCT3DTAB4_OFFSET]
		mov	tmp_f1, tmp_f1, lsr #2
		add	tab_f1, tab_f1, tmp_f1, lsl #2
		sub	tab_f1, tab_f1, #128
		b	UPDATE_ESCAP1_f1

DCT3TAB2_1_f1:
		cmp	tmp_f1, #8
		blt	ERROR_f1
		ldr	tab_f1, [vop_mode_ptr_f1, #DCT3DTAB5_OFFSET]
		add	tab_f1, tab_f1, tmp_f1, lsl #2
		sub	tab_f1, tab_f1, #32
	@//	b	UPDATE_ESCAP1_f1

UPDATE_ESCAP1_f1:
		ldrh	tmp_f1, [tab_f1, #2]		@//tab->len
		add	flushBits_f1, flushBits_f1, tmp_f1
		mov	code_f1, code_f1, lsl tmp_f1
		
		ldrh	tmp_f1, [tab_f1, #0]		@//tab->code
		mov	run_f1, tmp_f1, lsr #8
		and	run_f1, run_f1, #63
		and	level_f1, tmp_f1, #0xff
		mov	last_f1, tmp_f1, lsr #14
		mov	sign_f1, code_f1, lsr #31
		add	flushBits_f1, flushBits_f1, #1
		ldr	r6, [vop_mode_ptr_f1, #INTRA_MAX_LEVEL]
		add	tmp_f1, run_f1, last_f1, lsl #6
		ldrb	tmp_f1, [r6, tmp_f1]
		add	level_f1, level_f1, tmp_f1

		b	VLD_ONE_COEFF_END_f1


		@/*****************SECOND ESCAPE MODE*******************************/
ESCAPE_2_f1:
		movs	run_f1, code_f1, lsr #31
		mov	code_f1, code_f1, lsl #1
		add	flushBits_f1, flushBits_f1, #1

		bne	ESCAPE_3_f1

		mov	tmp_f1, code_f1, lsr #20
		cmp	tmp_f1, #512
		bcc	DCT3TAB1_2_f1
		ldr	tab_f1, [vop_mode_ptr_f1, #DCT3DTAB3_OFFSET]
		mov	tmp_f1, tmp_f1, lsr #5
		add	tab_f1, tab_f1, tmp_f1, lsl #2
		sub	tab_f1, tab_f1, #64
		b	UPDATE_ESCAP2_f1

DCT3TAB1_2_f1:
		cmp	tmp_f1, #128
		bcc	DCT3TAB2_2_f1
		ldr	tab_f1, [vop_mode_ptr_f1, #DCT3DTAB4_OFFSET]
		mov	tmp_f1, tmp_f1, lsr #2
		add	tab_f1, tab_f1, tmp_f1, lsl #2
		sub	tab_f1, tab_f1, #128
		b	UPDATE_ESCAP2_f1

DCT3TAB2_2_f1:
		cmp	tmp_f1, #8
		bcc	ERROR_f1
		ldr	tab_f1, [vop_mode_ptr_f1, #DCT3DTAB5_OFFSET]
		add	tab_f1, tab_f1, tmp_f1, lsl #2
		sub	tab_f1, tab_f1, #32
	@//	b	UPDATE_ESCAP2_f1

UPDATE_ESCAP2_f1:
		ldrh	tmp_f1, [tab_f1, #2]	@//tab->len
		add	flushBits_f1, flushBits_f1, tmp_f1
		mov	code_f1, code_f1, lsl tmp_f1

		ldrh	tmp_f1, [tab_f1, #0]	@//tab->code
		mov	run_f1, tmp_f1, lsr #8
		and	run_f1, run_f1, #63
		and	level_f1, tmp_f1, #0xff
		mov	last_f1, tmp_f1, lsr #14
		mov	sign_f1, code_f1, lsr #31
		add	flushBits_f1, flushBits_f1, #1

		ldr	r6, [vop_mode_ptr_f1, #INTRA_MAX_RUN]
		add	tmp_f1, level_f1, last_f1, lsl #5
		ldrb	tmp_f1, [r6, tmp_f1]
		add	run_f1, run_f1, tmp_f1
		add	run_f1, run_f1, #1
	
		b	VLD_ONE_COEFF_END_f1

		@/*************THIRD ESCAPE MODE*****************/
ESCAPE_3_f1:
		mov	code_f1, code_f1, lsr #11
		add	flushBits_f1, flushBits_f1, #21
		mov	last_f1, code_f1, lsr #20
		mov	run_f1, code_f1, lsr #14
		and	run_f1, run_f1, #0x3f
		mov	level_f1, code_f1, lsl #19
		mov	level_f1, level_f1, lsr #20

		mov	sign_f1, #0
		cmp	level_f1, #2048
		movhs	sign_f1, #1
		rsbhs	level_f1, level_f1, #4096

		b	VLD_ONE_COEFF_END_f1

		@//not escape mode, get run, level, sign and last
NOT_ESCAPE_f1:
		mov	run_f1, tmp_f1, lsr #8
		and	run_f1, run_f1, #63
	
		and	level_f1, tmp_f1, #0xff
		mov	last_f1, tmp_f1, lsr #14
		mov	sign_f1, code_f1, lsr #31
		add	flushBits_f1, flushBits_f1, #1

		@//vld one coefficient end
VLD_ONE_COEFF_END_f1:
		add	i_f1, i_f1, run_f1

		@//check error
		cmp	i_f1, #64
		bhs	ERROR_f1

		ldrb	index_f1, [pZigzag_f1, i_f1]	@//zigzag index

		cmp	sign_f1, #1
		rsbeq	level_f1, level_f1, #0

		strb	index_f1, [pNonCoeffPos_f1, nonCoeffNum_f1]
		add	r6, iDCTCoeff_f1, index_f1, lsl #1
		strh	level_f1, [r6]

		@//flush the used bits
		;/*******************for little endian*/
		ldr	r0, [stream_f1, #0xc]	@//bitcnt
		ldr	r6, [stream_f1, #8]	@//bitsLeft
		add	r0, r0, flushBits_f1
		str	r0, [stream_f1, #0xc]
		cmp	flushBits_f1, r6
		sublo	r6, r6, flushBits_f1
		blo	FLUSH_END_f1
		@FLUSH_NEXTWORD_f1
		ldr	r0, [stream_f1, #4]	@//bufb
		add	r6, r6, #32
		sub	r6, r6, flushBits_f1
		str	r0, [stream_f1, #0]	@//bufa = bufb

		ldr	r0, [stream_f1, #0x10]	@//rdptr
		add	tmp_f1, stream_f1, #0x214	@//rdbfr+512byte
		cmp	r0, tmp_f1
		movhs	r0, r9
		blhs	arm_streamBfr_update



		ldr	r0, [stream_f1, #0x10]	@//rdptr
		ldrb	r1, [r0], #1
		ldrb	r2, [r0], #1
		ldrb	r14, [r0], #1
		add	r1, r2, r1, lsl #8
		ldrb	r2, [r0], #1
		add	r1, r14, r1, lsl #8
		add	r1, r2, r1, lsl #8
		str	r1, [stream_f1, #0x4]	@//bufb
		str	r0, [stream_f1, #0x10]	@//rdptr++
FLUSH_END_f1:
		str	r6, [stream_f1, #0x8]	@//bitleft

		add	i_f1, i_f1, #1
		add	nonCoeffNum_f1, nonCoeffNum_f1, #1

		@//if last is zero, jump to VLD_LOOP and decoding next coefficient
		cmp	last_f1, #1
		bne	VLD_LOOP_f1

VLD_END_f1:
		mov	r0, nonCoeffNum_f1
		pop	{r4 - r12, pc}
		@ENDFUNC


@/*
@void Mp4Dec_VlcDecInterTCOEF_Mpeg(DEC_VOP_MODE_T *vop_mode_ptr, int16 *iDCTCoef, int32 iQP, DEC_BS_T * pBitstrm)
@	input:
@		r0: vop_mode_ptr
@		r1: iDCTCoef
@		r2: iQP
@		r3: pBitstrm
@
@	description: vld inter block of mpeg4
@*/

@/************register map begin**************/
tab_f2			.req	r0
run_f2			.req	r0
level_f2		.req	r1
iquant_f2		.req	r1
sign_f2			.req	r14
tmp_f2			.req	r3

iQP_f2			.req	r2

pZigzag_f2		.req	r4
piQuantizerMatrix_f2	.req	r5
qadd_f2			.req	r5
code_f2			.req	r6
flushBits_f2		.req	r7
index_f2		.req	r6
i_f2			.req	r8

stream_f2		.req	r9
vop_mode_ptr_f2		.req	r10
iDCTCoeff_f2		.req	r11

last_f2			.req	r12	@//will used in whole process	

fQuantizer_f2		.req	r14
iSum_f2			.req	r0
bCoefQAllZero_f2	.req	r3
@/************register map end**************/

.equ	DCT3DTAB0_OFFSET,	0xE4
.equ	DCT3DTAB1_OFFSET,	0xE8
.equ	DCT3DTAB2_OFFSET,	0xEC
.equ	INTER_MAX_LEVEL,	0xFC
.equ	INTER_MAX_RUN,		0x100

.equ	Q_H263,	0

Mp4Dec_VlcDecInterTCOEF_Mpeg:		@FUNCTION
		.global	Mp4Dec_VlcDecInterTCOEF_Mpeg

		push	{r4 - r12, r14}
		sub	sp, sp, #8
	
		@/*initial the vld parameter*/
		mov	vop_mode_ptr_f2, r0
		mov	iDCTCoeff_f2, r1
		mov	stream_f2, r3
		mov	i_f2, #0
		mov	last_f2, #0
		mov	iSum_f2, #0
		str	iSum_f2, [sp, #0]
		ldrb	fQuantizer_f2, [vop_mode_ptr_f2, #0x22]
		ldr	pZigzag_f2, [vop_mode_ptr_f2, #0x58]	@//standard_zigzag
		cmp	fQuantizer_f2, #Q_H263
		subeq	r14, iQP_f2, #1
		orreq	qadd_f2, r14, #1
		ldrne	piQuantizerMatrix_f2, [vop_mode_ptr_f2, #0x74]

		@//begin vld loop, one loop for one coeff		
VLD_LOOP_f2:
		@//show next 32 bits in bitstream and store in reg (code)
		ldmia	stream_f2, {r0, r1, r3}	@//r0: bufa, r1: bufb, r3: bitsLeft
		rsb	r6, r3, #32
		mov	code_f2, r0, lsl r6
		orr	code_f2, code_f2, r1, lsr r3

		@//choose huffman table
		mov	tmp_f2, code_f2, lsr #20
		cmp	tmp_f2, #512
		bcc	DCT3TAB1_0_f2
		ldr	tab_f2, [vop_mode_ptr_f2, #DCT3DTAB0_OFFSET]
		mov	tmp_f2, tmp_f2, lsr #5
		add	tab_f2, tab_f2, tmp_f2, lsl #2
		sub	tab_f2, tab_f2, #64
		b	STEP1_f2

DCT3TAB1_0_f2:
		cmp	tmp_f2, #128
		bcc	DCT3TAB2_0_f2
		ldr	tab_f2, [vop_mode_ptr_f2, #DCT3DTAB1_OFFSET]
		mov	tmp_f2, tmp_f2, lsr #2
		add	tab_f2, tab_f2, tmp_f2, lsl #2
		sub	tab_f2, tab_f2, #128
		b	STEP1_f2

DCT3TAB2_0_f2:
		cmp	tmp_f2, #8
		bcc	ERROR_f2
		ldr	tab_f2, [vop_mode_ptr_f2, #DCT3DTAB2_OFFSET]
		add	tab_f2, tab_f2, tmp_f2, lsl #2
		sub	tab_f2, tab_f2, #32
		b	STEP1_f2

ERROR_f2:
		mov	tmp_f2, #1
		strb	tmp_f2, [vop_mode_ptr_f2, #ERRORFLAG_OFFSET]
		b	VLD_FINAL_f2


		@//update code and recoder flush bits
		@ /******************************************/
STEP1_f2:
		ldrh	flushBits_f2, [tab_f2, #2]	@//flush_bits: tab->len
		ldrh	tmp_f2, [tab_f2, #0]		@//tmp: tab->code

		mov	code_f2, code_f2, lsl flushBits_f2
		
		@mov	r0, #0x1bff
		@cmp	tmp_f2, r0
		sub	r0, tmp_f2, #0x1b00
		subs	r0, r0, #0xff
		bne	NOT_ESCAPE_f2

		@//if escape mode, enter escape vld mode
ESCAPE_1_f2:
		movs	level_f2, code_f2, lsr #31
		mov	code_f2, code_f2, lsl #1
		add	flushBits_f2, flushBits_f2, #1
		bne	ESCAPE_2_f2

		mov	tmp_f2, code_f2, lsr #20
		cmp	tmp_f2, #512
		bcc	DCT3TAB1_1_f2
		ldr	tab_f2, [vop_mode_ptr_f2, #DCT3DTAB0_OFFSET]
		mov	tmp_f2, tmp_f2, lsr #5
		add	tab_f2, tab_f2, tmp_f2, lsl #2
		sub	tab_f2, tab_f2, #64
		b	UPDATE_ESCAP1_f2

DCT3TAB1_1_f2:
		cmp	tmp_f2, #128
		bcc	DCT3TAB2_1_f2
		ldr	tab_f2, [vop_mode_ptr_f2, #DCT3DTAB1_OFFSET]
		mov	tmp_f2, tmp_f2, lsr #2
		add	tab_f2, tab_f2, tmp_f2, lsl #2
		sub	tab_f2, tab_f2, #128
		b	UPDATE_ESCAP1_f2

DCT3TAB2_1_f2:

		cmp	tmp_f2, #8
		blt	ERROR_f2
		ldr	tab_f2, [vop_mode_ptr_f2, #DCT3DTAB2_OFFSET]
		add	tab_f2, tab_f2, tmp_f2, lsl #2
		sub	tab_f2, tab_f2, #32
	@//	b	UPDATE_ESCAP1_f2

UPDATE_ESCAP1_f2:
		ldrh	tmp_f2, [tab_f2, #2]		@//tab->len
		add	flushBits_f2, flushBits_f2, tmp_f2
		mov	code_f2, code_f2, lsl tmp_f2
		
		ldrh	tmp_f2, [tab_f2, #0]		@//tab->code
		mov	run_f2, tmp_f2, lsr #4
		and	run_f2, run_f2, #0xff
		and	level_f2, tmp_f2, #0xf
		mov	last_f2, tmp_f2, lsr #12
		mov	sign_f2, code_f2, lsr #31
		add	flushBits_f2, flushBits_f2, #1
		ldr	r6, [vop_mode_ptr_f2, #INTER_MAX_LEVEL]
		add	tmp_f2, run_f2, last_f2, lsl #6
		ldrb	tmp_f2, [r6, tmp_f2]
		add	level_f2, level_f2, tmp_f2

		b	VLD_ONE_COEFF_END_f2


		@/*****************SECOND ESCAPE MODE*******************************/
ESCAPE_2_f2:
		movs	run_f2, code_f2, lsr #31
		mov	code_f2, code_f2, lsl #1
		add	flushBits_f2, flushBits_f2, #1

		bne	ESCAPE_3_f2

		mov	tmp_f2, code_f2, lsr #20
		cmp	tmp_f2, #512
		bcc	DCT3TAB1_2_f2
		ldr	tab_f2, [vop_mode_ptr_f2, #DCT3DTAB0_OFFSET]
		mov	tmp_f2, tmp_f2, lsr #5
		add	tab_f2, tab_f2, tmp_f2, lsl #2
		sub	tab_f2, tab_f2, #64
		b	UPDATE_ESCAP2_f2

DCT3TAB1_2_f2:
		cmp	tmp_f2, #128
		bcc	DCT3TAB2_2_f2
		ldr	tab_f2, [vop_mode_ptr_f2, #DCT3DTAB1_OFFSET]
		mov	tmp_f2, tmp_f2, lsr #2
		add	tab_f2, tab_f2, tmp_f2, lsl #2
		sub	tab_f2, tab_f2, #128
		b	UPDATE_ESCAP2_f2

DCT3TAB2_2_f2:
		cmp	tmp_f2, #8
		bcc	ERROR_f2
		ldr	tab_f2, [vop_mode_ptr_f2, #DCT3DTAB2_OFFSET]
		add	tab_f2, tab_f2, tmp_f2, lsl #2
		sub	tab_f2, tab_f2, #32
	@//	b	UPDATE_ESCAP2_f2

UPDATE_ESCAP2_f2:
		ldrh	tmp_f2, [tab_f2, #2]	@//tab->len
		add	flushBits_f2, flushBits_f2, tmp_f2
		mov	code_f2, code_f2, lsl tmp_f2

		ldrh	tmp_f2, [tab_f2, #0]	@//tab->code
		mov	run_f2, tmp_f2, lsr #4
		and	run_f2, run_f2, #0xff
		and	level_f2, tmp_f2, #0xf
		mov	last_f2, tmp_f2, lsr #12
		mov	sign_f2, code_f2, lsr #31
		add	flushBits_f2, flushBits_f2, #1

		ldr	r6, [vop_mode_ptr_f2, #INTER_MAX_RUN]
		add	tmp_f2, level_f2, last_f2, lsl #4
		ldrb	tmp_f2, [r6, tmp_f2]
		add	run_f2, run_f2, tmp_f2
		add	run_f2, run_f2, #1
	
		b	VLD_ONE_COEFF_END_f2

		@/*************THIRD ESCAPE MODE*****************/
ESCAPE_3_f2:
		mov	code_f2, code_f2, lsr #11
		add	flushBits_f2, flushBits_f2, #21
		mov	last_f2, code_f2, lsr #20
		mov	run_f2, code_f2, lsr #14
		and	run_f2, run_f2, #0x3f
		mov	level_f2, code_f2, lsl #19
		mov	level_f2, level_f2, lsr #20

		mov	sign_f2, #0
		cmp	level_f2, #2048
		movhs	sign_f2, #1
		rsbhs	level_f2, level_f2, #4096

		b	VLD_ONE_COEFF_END_f2

		@//not escape mode, get run, level, sign and last
NOT_ESCAPE_f2:
		mov	run_f2, tmp_f2, lsr #4
		and	run_f2, run_f2, #0xff
	
		and	level_f2, tmp_f2, #0xf
		mov	last_f2, tmp_f2, lsr #12
		mov	sign_f2, code_f2, lsr #31
		add	flushBits_f2, flushBits_f2, #1

		@//vld one coefficient end
VLD_ONE_COEFF_END_f2:
		add	i_f2, i_f2, run_f2

		@//check error
		cmp	i_f2, #64
		bhs	ERROR_f2

		@//iq and store the coefficient
		add	level_f2, level_f2, level_f2
		ldrb	r6, [vop_mode_ptr_f2, #0x22]
		cmp	r6, #Q_H263
		ldrb	index_f2, [pZigzag_f2, i_f2]	@//zigzag index
		mlaeq	iquant_f2, level_f2, iQP_f2, qadd_f2
		beq	STORE_COEF_f2
		@//Q_MPEG4
		add	level_f2, level_f2, #1
		ldrb	r3, [piQuantizerMatrix_f2, index_f2]
		mul	iquant_f2, level_f2, iQP_f2
		mul	iquant_f2, iquant_f2, r3
		asr	iquant_f2, iquant_f2, #4
		ldmia	sp, {iSum_f2, bCoefQAllZero_f2}
		eor	iSum_f2, iSum_f2, iquant_f2
		mov	bCoefQAllZero_f2, #0
		stmia	sp, {iSum_f2, bCoefQAllZero_f2}

STORE_COEF_f2:
		cmp	sign_f2, #1
		rsbeq	iquant_f2, iquant_f2, #0

		add	r6, iDCTCoeff_f2, index_f2, lsl #1
		strh	iquant_f2, [r6]

		@//flush the used bits
		;/*******************for little endian*/
		ldr	r0, [stream_f2, #0xc]	@//bitcnt
		ldr	r6, [stream_f2, #8]	@//bitsLeft
		add	r0, r0, flushBits_f2
		str	r0, [stream_f2, #0xc]
		cmp	flushBits_f2, r6
		sublo	r6, r6, flushBits_f2
		blo	FLUSH_END_f2
		@FLUSH_NEXTWORD_f2
		ldr	r0, [stream_f2, #4]	@//bufb
		add	r6, r6, #32
		sub	r6, r6, flushBits_f2
		str	r0, [stream_f2, #0]	@//bufa = bufb

		ldr	r0, [stream_f2, #0x10]	@//rdptr
		add	tmp_f2, stream_f2, #0x214	@//rdbfr+512byte
		cmp	r0, tmp_f2
		movhs	r0, r9
		blhs	arm_streamBfr_update
		ldr	r0, [stream_f2, #0x10]	@//rdptr
		ldrb	r1, [r0], #1
		ldrb	r3, [r0], #1
		ldrb	r14, [r0], #1
		add	r1, r3, r1, lsl #8
		ldrb	r3, [r0], #1
		add	r1, r14, r1, lsl #8
		add	r1, r3, r1, lsl #8
		str	r1, [stream_f2, #0x4]	@//bufb
		str	r0, [stream_f2, #0x10]	@//rdptr++
FLUSH_END_f2:
		str	r6, [stream_f2, #0x8]	@//bitleft

		add	i_f2, i_f2, #1

		@//if last is zero, jump to VLD_LOOP and decoding next coefficient
		cmp	last_f2, #1
		bne	VLD_LOOP_f2

VLD_END_f2:
		ldrb	fQuantizer_f2, [vop_mode_ptr_f2, #0x22]
		cmp	fQuantizer_f2, #Q_H263
		beq	VLD_FINAL_f2
		ldmia	sp, {iSum_f2, bCoefQAllZero_f2}
		cmp	bCoefQAllZero_f2, #0
		bne	VLD_FINAL_f2
		ands		iSum_f2, #0x1
		bne	VLD_FINAL_f2
		ldrsh	r14, [iDCTCoeff_f2, #0x7e]
		eor	r14, r14, #0x1
		strh	r14, [iDCTCoeff_f2, #0x7e]
VLD_FINAL_f2:
		add	sp, sp, #8
		pop	{r4 - r12, pc}
		@ENDFUNC

		.end

