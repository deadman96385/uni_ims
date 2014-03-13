.code 32	@CODE32
.section .text	@AREA ||.text||, CODE, READONLY
			
@; r0: short *PM
@; r1: short *pAdapt
@; r2: int nDirection
@; r3: int nOrder

@; assume little-endian
@MONKEY_CNNFilter_Adapt_ASM PROC
MONKEY_CNNFilter_Adapt_ASM:
			STMFD 		sp!,{r4-r12,lr}

			MOV			r3,r3,ASR #4
			CMP			r2,#0
			BEQ			MONKEY_CNNFilter_Adapt_ASM_End
			BGT			MONKEY_CNNFilter_Adapt_ASM_B1

MONKEY_CNNFilter_Adapt_ASM_Loop1:
			@;;; A
			LDRSH		r5,[r0],#2
			LDRSH		r6,[r0],#2
			LDRSH		r7,[r0],#2
			LDRSH		r8,[r0],#-6
			LDRSH		r9,[r1],#2
			LDRSH		r10,[r1],#2
			LDRSH		r11,[r1],#2
			LDRSH		r12,[r1],#2
			
			ADD			r5,r5,r9
			ADD			r6,r6,r10
			ADD			r7,r7,r11
			ADD			r8,r8,r12
			
			STRH		r5,[r0],#2
			STRH		r6,[r0],#2
			STRH		r7,[r0],#2
			STRH		r8,[r0],#2
			
			@;;; B
			LDRSH		r5,[r0],#2
			LDRSH		r6,[r0],#2
			LDRSH		r7,[r0],#2
			LDRSH		r8,[r0],#-6
			LDRSH		r9,[r1],#2
			LDRSH		r10,[r1],#2
			LDRSH		r11,[r1],#2
			LDRSH		r12,[r1],#2
			
			ADD			r5,r5,r9
			ADD			r6,r6,r10
			ADD			r7,r7,r11
			ADD			r8,r8,r12
			
			STRH		r5,[r0],#2
			STRH		r6,[r0],#2
			STRH		r7,[r0],#2
			STRH		r8,[r0],#2
			
			@;;; C
			LDRSH		r5,[r0],#2
			LDRSH		r6,[r0],#2
			LDRSH		r7,[r0],#2
			LDRSH		r8,[r0],#-6
			LDRSH		r9,[r1],#2
			LDRSH		r10,[r1],#2
			LDRSH		r11,[r1],#2
			LDRSH		r12,[r1],#2
			
			ADD			r5,r5,r9
			ADD			r6,r6,r10
			ADD			r7,r7,r11
			ADD			r8,r8,r12
			
			STRH		r5,[r0],#2
			STRH		r6,[r0],#2
			STRH		r7,[r0],#2
			STRH		r8,[r0],#2
			
			@;;; D
			LDRSH		r5,[r0],#2
			LDRSH		r6,[r0],#2
			LDRSH		r7,[r0],#2
			LDRSH		r8,[r0],#-6
			LDRSH		r9,[r1],#2
			LDRSH		r10,[r1],#2
			LDRSH		r11,[r1],#2
			LDRSH		r12,[r1],#2
			
			ADD			r5,r5,r9
			ADD			r6,r6,r10
			ADD			r7,r7,r11
			ADD			r8,r8,r12
			
			STRH		r5,[r0],#2
			STRH		r6,[r0],#2
			STRH		r7,[r0],#2
			STRH		r8,[r0],#2
			
			SUBS		r3,r3,#1
			BGT			MONKEY_CNNFilter_Adapt_ASM_Loop1
			
			B				MONKEY_CNNFilter_Adapt_ASM_End
			
MONKEY_CNNFilter_Adapt_ASM_B1:

MONKEY_CNNFilter_Adapt_ASM_Loop2:
			@;;; A
			LDRSH		r5,[r0],#2
			LDRSH		r6,[r0],#2
			LDRSH		r7,[r0],#2
			LDRSH		r8,[r0],#-6
			LDRSH		r9,[r1],#2
			LDRSH		r10,[r1],#2
			LDRSH		r11,[r1],#2
			LDRSH		r12,[r1],#2
			
			SUB			r5,r5,r9
			SUB			r6,r6,r10
			SUB			r7,r7,r11
			SUB			r8,r8,r12
			
			STRH		r5,[r0],#2
			STRH		r6,[r0],#2
			STRH		r7,[r0],#2
			STRH		r8,[r0],#2
			
			@;;; B
			LDRSH		r5,[r0],#2
			LDRSH		r6,[r0],#2
			LDRSH		r7,[r0],#2
			LDRSH		r8,[r0],#-6
			LDRSH		r9,[r1],#2
			LDRSH		r10,[r1],#2
			LDRSH		r11,[r1],#2
			LDRSH		r12,[r1],#2
			
			SUB			r5,r5,r9
			SUB			r6,r6,r10
			SUB			r7,r7,r11
			SUB			r8,r8,r12
			
			STRH		r5,[r0],#2
			STRH		r6,[r0],#2
			STRH		r7,[r0],#2
			STRH		r8,[r0],#2
			
			@;;; C
			LDRSH		r5,[r0],#2
			LDRSH		r6,[r0],#2
			LDRSH		r7,[r0],#2
			LDRSH		r8,[r0],#-6
			LDRSH		r9,[r1],#2
			LDRSH		r10,[r1],#2
			LDRSH		r11,[r1],#2
			LDRSH		r12,[r1],#2
			
			SUB			r5,r5,r9
			SUB			r6,r6,r10
			SUB			r7,r7,r11
			SUB			r8,r8,r12
			
			STRH		r5,[r0],#2
			STRH		r6,[r0],#2
			STRH		r7,[r0],#2
			STRH		r8,[r0],#2
			
			@;;; D
			LDRSH		r5,[r0],#2
			LDRSH		r6,[r0],#2
			LDRSH		r7,[r0],#2
			LDRSH		r8,[r0],#-6
			LDRSH		r9,[r1],#2
			LDRSH		r10,[r1],#2
			LDRSH		r11,[r1],#2
			LDRSH		r12,[r1],#2
			
			SUB			r5,r5,r9
			SUB			r6,r6,r10
			SUB			r7,r7,r11
			SUB			r8,r8,r12
			
			STRH		r5,[r0],#2
			STRH		r6,[r0],#2
			STRH		r7,[r0],#2
			STRH		r8,[r0],#2
			
			SUBS		r3,r3,#1
			BGT			MONKEY_CNNFilter_Adapt_ASM_Loop2
			
MONKEY_CNNFilter_Adapt_ASM_End:		
	
			LDMFD		sp!,{r4-r12,pc}
			
			@ENDP
			
			
@; r0: short *pA
@; r1: short *pB
@; r2: int nOrder

@; assume little-endian
@MONKEY_CNNFilter_CalcDotProduct_ASM PROC
MONKEY_CNNFilter_CalcDotProduct_ASM:
			STMFD 	sp!,{r4-r12,lr}

			MOV			lr,#0
			MOV			r2,r2,ASR #4
			ANDS		r3,r1,#0x03
			BNE			MONKEY_CNNFilter_CalcDotProduct_ASM_B1

MONKEY_CNNFilter_CalcDotProduct_ASM_Loop1:
			
			@;;; A
			LDR			r4,[r0],#4
			LDR			r5,[r1],#4
			LDR			r6,[r0],#4
			LDR			r7,[r1],#4			
			SMLABB	lr,r4,r5,lr
			SMLATT	lr,r4,r5,lr
			SMLABB	lr,r6,r7,lr
			SMLATT	lr,r6,r7,lr
			
			@;;; B
			LDR			r4,[r0],#4
			LDR			r5,[r1],#4
			LDR			r6,[r0],#4
			LDR			r7,[r1],#4			
			SMLABB	lr,r4,r5,lr
			SMLATT	lr,r4,r5,lr
			SMLABB	lr,r6,r7,lr
			SMLATT	lr,r6,r7,lr
			
			@;;; C
			LDR			r4,[r0],#4
			LDR			r5,[r1],#4
			LDR			r6,[r0],#4
			LDR			r7,[r1],#4			
			SMLABB	lr,r4,r5,lr
			SMLATT	lr,r4,r5,lr
			SMLABB	lr,r6,r7,lr
			SMLATT	lr,r6,r7,lr
			
			@;;; D
			LDR			r4,[r0],#4
			LDR			r5,[r1],#4
			LDR			r6,[r0],#4
			LDR			r7,[r1],#4			
			SMLABB	lr,r4,r5,lr
			SMLATT	lr,r4,r5,lr
			SMLABB	lr,r6,r7,lr
			SMLATT	lr,r6,r7,lr
			
			SUBS		r2,r2,#1
			BGT			MONKEY_CNNFilter_CalcDotProduct_ASM_Loop1
			
			B			MONKEY_CNNFilter_CalcDotProduct_ASM_End
			
MONKEY_CNNFilter_CalcDotProduct_ASM_B1:

			ADD			r1,r1,#-2
			LDR			r7,[r1],#4
MONKEY_CNNFilter_CalcDotProduct_ASM_Loop2:
			
			@;;; A
			LDR			r4,[r0],#4
			LDR			r5,[r1],#4
			SMLABT	lr,r4,r7,lr
			LDR			r6,[r0],#4
			LDR			r7,[r1],#4
			SMLATB	lr,r4,r5,lr
			SMLABT	lr,r6,r5,lr
			SMLATB	lr,r6,r7,lr
			
			@;;; B
			LDR			r4,[r0],#4
			LDR			r5,[r1],#4
			SMLABT	lr,r4,r7,lr
			LDR			r6,[r0],#4
			LDR			r7,[r1],#4
			SMLATB	lr,r4,r5,lr
			SMLABT	lr,r6,r5,lr
			SMLATB	lr,r6,r7,lr
			
			@;;; C
			LDR			r4,[r0],#4
			LDR			r5,[r1],#4
			SMLABT	lr,r4,r7,lr
			LDR			r6,[r0],#4
			LDR			r7,[r1],#4
			SMLATB	lr,r4,r5,lr
			SMLABT	lr,r6,r5,lr
			SMLATB	lr,r6,r7,lr
			
			@;;; D
			LDR			r4,[r0],#4
			LDR			r5,[r1],#4
			SMLABT	lr,r4,r7,lr
			LDR			r6,[r0],#4
			LDR			r7,[r1],#4
			SMLATB	lr,r4,r5,lr
			SMLABT	lr,r6,r5,lr
			SMLATB	lr,r6,r7,lr
			
			SUBS		r2,r2,#1
			BGT			MONKEY_CNNFilter_CalcDotProduct_ASM_Loop2
	
MONKEY_CNNFilter_CalcDotProduct_ASM_End:
			MOV			r0,lr
			LDMFD		sp!,{r4-r12,pc}
			
			@ENDP


			@EXPORT	MONKEY_CNNFilter_Adapt_ASM
			@EXPORT	MONKEY_CNNFilter_CalcDotProduct_ASM
.global MONKEY_CNNFilter_Adapt_ASM
.global MONKEY_CNNFilter_CalcDotProduct_ASM
			@END
