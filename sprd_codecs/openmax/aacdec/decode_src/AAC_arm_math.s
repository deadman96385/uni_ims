@// AAC-LC tns_ar_filter
@// author: reed.zhang
@// input: SBR synthesis filter bank processing
@//     r0: output sample pointer
@// r1: the input overlap
@// r2: the tablr for output data
@// r3: the table for overlapping  
@// purpose:
	

                @ ***** END LICENSE BLOCK *****  

	 .text  @AREA	|.text|, CODE, READONLY
	
	 .arm  
s_l         .req   r3
s_h         .req   r4                                
                @//int32_t AAC_ARM_LongByLongAsm(
                @//                              int32_t   d0
                @//                              int32_t   d1
                @//                              int16_t   bitw)@
                

                @@ // r0, input data
                @@ // r1, intput data
                @@ // r2, bit widht
AAC_ARM_LongByLongAsm:   @   FUNCTION
                @ // save the value for the following calculation
                .global  AAC_ARM_LongByLongAsm
                stmfd   sp!, {r4, r14}
                
                SMULL    s_l,  s_h,    r0,  r1
                mov      s_l,  s_l,  lsr r2
                rsb      r0,   r2,  #32
                mov      s_h,   s_h,  lsl  r0
                orr      r0, s_h,   s_l
                ldmfd   sp!, {r4, pc}
                @ENDFUNC
                               

ld_ptr    .req r0             
@@///////
@//int32_t AAC_GetBitsAsm(bitfile *ld_ptr, uint32_t n )                
AAC_GetBits:   @   FUNCTION
        .global	AAC_GetBits	
        stmfd	sp!, {r14}      
        ldrh   r2, [ld_ptr, #4]   @@@// bits_left + bytes_used
        ldr    r14, [ld_ptr]        
        cmp    r1,  r2    
        ldr    r3, [r14]

    @IF AAC_DEC_ASM_LITTLE_ENDIAN = 1  
        EOR    r12,  r3, r3,  ROR #16
        MOV  r12,  r12,    LSR #8
        BIC     r12,  r12, #0xFF00
        EOR    r3,  r12, r3, ROR #8
    @ENDIF
        @@//    bits_lest >= getbits
        suble  r1,  r2,   r1
        strleh r1, [ld_ptr, #4]
        rsble  r2,  r2,   #32
        movle  r3,  r3,   lsl r2
        addle  r1,  r1,    r2
        movle  r0,  r3,    lsr  r1
        ble    AAC_GetBitsEXIT
        @@//    bits_lest < getbits
        sub  r1,  r1,    r2   @@// getbits - bits_left
        rsb  r2,  r2,    #32    @@// r2 -> 32 - r2
        mov  r3,  r3,    lsl   r2
        sub  r2,  r2,    r1
        mov  r3,  r3,    lsr r2
        
        ldr   r2,  [ld_ptr, #8]
        ldrh  r12, [ld_ptr, #6]      @@// bytes used        
        add    r14,  r14,  #4
        add    r12,  r12, #4       
        cmp   r2,  r12
        strh   r12, [ld_ptr, #6]
        BLT   AAC_GetBitsEXIT

        ldr  r2, [r14]    @@// next word
        rsb  r1, r1,   #32
        str  r14, [ld_ptr]
        strh  r1, [ld_ptr, #4]      @@// bits_left
    @IF AAC_DEC_ASM_LITTLE_ENDIAN = 1        
        EOR      r12,  r2, r2,  ROR #16
        MOV    r12,  r12,    LSR #8
        BIC       r12,  r12, #0xFF00
        EOR     r2,  r12, r2, ROR #8        
    @ENDIF
        orr        r0,  r3,  r2 ,  lsr  r1 
AAC_GetBitsEXIT:        
        ldmfd	sp!, { pc} 	
        @ENDFUNC 
        
        
        
        
        
ld_ptr    .req r0             
@@///////
@//int32_t AAC_ShowBitsAsm(bitfile *ld_ptr, uint32_t n )                
@// uint8_t AAC_HuffmanBinaryQuadCb3Asm(bitfile *ld_ptr, int16_t *sp_ptr, int32_t *hcb3_ptr, int16 len)    @// p1: 31:16,  p2: 15:0
AAC_ShowBitsAsm:   @   FUNCTION
        .global	AAC_ShowBitsAsm	
                stmfd	sp!, {r14}      
        ldrh   r2, [ld_ptr, #4]   @@@// bits_left + bytes_used
        ldr    r14, [ld_ptr]        
        cmp    r1,  r2    
        ldr    r3, [r14]

    @IF AAC_DEC_ASM_LITTLE_ENDIAN = 1  
        EOR    r12,  r3, r3,  ROR #16
        MOV  r12,  r12,    LSR #8
        BIC     r12,  r12, #0xFF00
        EOR    r3,  r12, r3, ROR #8
    @ENDIF
        @@//    bits_lest >= getbits
        suble  r1,  r2,   r1
        rsble  r2,  r2,   #32
        movle  r3,  r3,   lsl r2
        addle  r1,  r1,    r2
        movle  r0,  r3,    lsr  r1
        ble    AAC_ShowBitsAsmEXIT
        @@//    bits_lest < getbits
        sub  r1,  r1,    r2   @@// getbits - bits_left
        rsb  r2,  r2,    #32    @@// r2 -> 32 - r2
        mov  r3,  r3,    lsl   r2
        sub  r2,  r2,    r1
        mov  r3,  r3,    lsr r2

        ldr  r2, [r14, #4]    @@// next word
        rsb  r1, r1,   #32

    @IF 1 @AAC_DEC_ASM_LITTLE_ENDIAN = 1        
        EOR      r12,  r2, r2,  ROR #16
        MOV    r12,  r12,    LSR #8
        BIC       r12,  r12, #0xFF00
        EOR     r2,  r12, r2, ROR #8        
    @ENDIF
        orr        r0,  r3,  r2 ,  lsr  r1 
AAC_ShowBitsAsmEXIT:        
        ldmfd	sp!, { pc} 	
        @ENDFUNC 
        
        
        
@@/************************************************/  
@@// input/output             
cov_x_buf_ptr		.req	r0
cov_aac_buf_ptr	.req  r1
@@///////////////
cov_count	                .req	r2
cov_x0_re_ptr	        .req 	r3
cov_x0_im_ptr	        .req 	r4
cov_x1_re_ptr	        .req 	r5
cov_x1_im_ptr	        .req 	r6
cov_x0_im_neg        .req	r14


cov_c01_re_lo	        .req 	r7
cov_c01_re_hi	        .req	r8
cov_c01_im_lo	        .req  r9
cov_c01_im_hi	        .req 	r10
cov_c11_re_lo	        .req 	r11
cov_c11_re_hi	        .req	r12

cov_c12_re_lo	        .req 	r7
cov_c12_re_hi	        .req	r8
cov_c12_im_lo	        .req  r9
cov_c12_im_hi	        .req 	r10
cov_c22_re_lo	        .req 	r11
cov_c22_re_hi	        .req	r12

@// void AAC_SBRGenConvCalculate(int *cov_x_buf_ptr, int *cov_aac_buf_ptr)
AAC_SBRGenConvCalculate:   @		FUNCTION
	.global	AAC_SBRGenConvCalculate
	stmfd	sp!, {r4-r11, r14}

	ldr	        cov_x0_re_ptr, [cov_x_buf_ptr], #4*(1)
	ldr	        cov_x0_im_ptr, [cov_x_buf_ptr], #4*(2*64-1)
	ldr	        cov_x1_re_ptr, [cov_x_buf_ptr], #4*(1)
	ldr	        cov_x1_im_ptr, [cov_x_buf_ptr], #4*(2*64-1)
	rsb	        cov_x0_im_neg, cov_x0_im_ptr, #0

	smull	        cov_c12_re_lo, cov_c12_re_hi, cov_x1_re_ptr, cov_x0_re_ptr
	smlal	        cov_c12_re_lo, cov_c12_re_hi, cov_x1_im_ptr, cov_x0_im_ptr
	smull	        cov_c12_im_lo, cov_c12_im_hi, cov_x0_re_ptr, cov_x1_im_ptr
	smlal	        cov_c12_im_lo, cov_c12_im_hi, cov_x0_im_neg, cov_x1_re_ptr
	smull	        cov_c22_re_lo, cov_c22_re_hi, cov_x0_re_ptr, cov_x0_re_ptr
	smlal	        cov_c22_re_lo, cov_c22_re_hi, cov_x0_im_ptr, cov_x0_im_ptr

	add	        cov_count, cov_aac_buf_ptr, #(4*6)
	stmia        	cov_count, {cov_c12_re_lo-cov_c22_re_hi}
	@@/////////////////////////////////////////////////////////////
	ldr	        cov_x0_re_ptr, [cov_x_buf_ptr], #4*(1)
	ldr	        cov_x0_im_ptr, [cov_x_buf_ptr], #4*(2*64-1)
	rsb           cov_x0_im_neg, cov_x1_im_ptr, #0

	smull	        cov_c01_re_lo, cov_c01_re_hi, cov_x0_re_ptr, cov_x1_re_ptr
	smlal	        cov_c01_re_lo, cov_c01_re_hi, cov_x0_im_ptr, cov_x1_im_ptr
	smull	        cov_c01_im_lo, cov_c01_im_hi, cov_x1_re_ptr, cov_x0_im_ptr
	smlal	        cov_c01_im_lo, cov_c01_im_hi, cov_x0_im_neg, cov_x0_re_ptr
	smull	        cov_c11_re_lo, cov_c11_re_hi, cov_x1_re_ptr, cov_x1_re_ptr
	smlal	        cov_c11_re_lo, cov_c11_re_hi, cov_x1_im_ptr, cov_x1_im_ptr
        
        @@/////////////////////////////////////////////////////////////       
	ldr	        cov_x1_re_ptr, [cov_x_buf_ptr], #4*(1)
	ldr	        cov_x1_im_ptr, [cov_x_buf_ptr], #4*(2*64-1)
	rsb           cov_x0_im_neg, cov_x0_im_ptr, #0

	smlal	        cov_c01_re_lo, cov_c01_re_hi, cov_x1_re_ptr, cov_x0_re_ptr
	smlal	        cov_c01_re_lo, cov_c01_re_hi, cov_x1_im_ptr, cov_x0_im_ptr
	smlal	        cov_c01_im_lo, cov_c01_im_hi, cov_x0_re_ptr, cov_x1_im_ptr
	smlal	        cov_c01_im_lo, cov_c01_im_hi, cov_x0_im_neg, cov_x1_re_ptr
	smlal	        cov_c11_re_lo, cov_c11_re_hi, cov_x0_re_ptr, cov_x0_re_ptr
	smlal	        cov_c11_re_lo, cov_c11_re_hi, cov_x0_im_ptr, cov_x0_im_ptr
	
	
	mov		cov_count, #(16*2 + 4)
AAC_SBR_GenCV1_Loop_Start:
        @@/////////////////////////////////////////////////////////////
	ldr	        cov_x0_re_ptr, [cov_x_buf_ptr], #4*(1)
	ldr	        cov_x0_im_ptr, [cov_x_buf_ptr], #4*(2*64-1)
	rsb           cov_x0_im_neg, cov_x1_im_ptr, #0

	smlal	        cov_c01_re_lo, cov_c01_re_hi, cov_x0_re_ptr, cov_x1_re_ptr
	smlal	        cov_c01_re_lo, cov_c01_re_hi, cov_x0_im_ptr, cov_x1_im_ptr
	smlal	        cov_c01_im_lo, cov_c01_im_hi, cov_x1_re_ptr, cov_x0_im_ptr
	smlal	        cov_c01_im_lo, cov_c01_im_hi, cov_x0_im_neg, cov_x0_re_ptr
	smlal	        cov_c11_re_lo, cov_c11_re_hi, cov_x1_re_ptr, cov_x1_re_ptr
	smlal	        cov_c11_re_lo, cov_c11_re_hi, cov_x1_im_ptr, cov_x1_im_ptr
        
        @@/////////////////////////////////////////////////////////////       
	ldr	        cov_x1_re_ptr, [cov_x_buf_ptr], #4*(1)
	ldr	        cov_x1_im_ptr, [cov_x_buf_ptr], #4*(2*64-1)
	rsb           cov_x0_im_neg, cov_x0_im_ptr, #0

	smlal	        cov_c01_re_lo, cov_c01_re_hi, cov_x1_re_ptr, cov_x0_re_ptr
	smlal	        cov_c01_re_lo, cov_c01_re_hi, cov_x1_im_ptr, cov_x0_im_ptr
	smlal	        cov_c01_im_lo, cov_c01_im_hi, cov_x0_re_ptr, cov_x1_im_ptr
	smlal	        cov_c01_im_lo, cov_c01_im_hi, cov_x0_im_neg, cov_x1_re_ptr
	smlal	        cov_c11_re_lo, cov_c11_re_hi, cov_x0_re_ptr, cov_x0_re_ptr
	smlal	        cov_c11_re_lo, cov_c11_re_hi, cov_x0_im_ptr, cov_x0_im_ptr


	
	subs	        cov_count, cov_count, #2
	BGT		AAC_SBR_GenCV1_Loop_Start
	
	
	stmia	        cov_aac_buf_ptr, {cov_c01_re_lo-cov_c11_re_hi}
	ldr		cov_x_buf_ptr, [cov_aac_buf_ptr, #4*(6)]           @// load cov_c12_re_lo into temp buf
	ldr		cov_count,  [cov_aac_buf_ptr, #4*(7)]                 @//load cov_c12_re_hi into temp buf
	rsb		cov_x0_re_ptr, cov_x0_re_ptr, #0
	adds	        cov_c12_re_lo, cov_x_buf_ptr, cov_c01_re_lo    @//cov_c12_re_lo and cov_c01_re_lo are same reg
	adc		cov_c12_re_hi, cov_count,  cov_c01_re_hi	    @// cov_c12_re_hi and cov_c01_re_hi are same reg
	smlal	        cov_c12_re_lo, cov_c12_re_hi, cov_x1_re_ptr, cov_x0_re_ptr
	smlal	        cov_c12_re_lo, cov_c12_re_hi, cov_x1_im_ptr, cov_x0_im_neg

	ldr	        cov_x_buf_ptr, [cov_aac_buf_ptr, #4*(8)]           @// load cov_c12_im_lo into temp buf
	ldr            cov_count,  [cov_aac_buf_ptr, #4*(9)]                @// load cov_c12_im_hi into temp buf
	adds	        cov_c12_im_lo, cov_x_buf_ptr, cov_c01_im_lo  @// cov_c12_im_lo and cov_c01_im_lo are same reg
	adc		cov_c12_im_hi, cov_count,  cov_c01_im_hi        @// cov_c12_im_hi and cov_c01_im_hi are same reg
	smlal	        cov_c12_im_lo, cov_c12_im_hi, cov_x0_re_ptr, cov_x1_im_ptr
	smlal	        cov_c12_im_lo, cov_c12_im_hi, cov_x0_im_ptr, cov_x1_re_ptr
	
	ldr           cov_x_buf_ptr, [cov_aac_buf_ptr, #4*(10)]        @//load cov_c22_re_lo into temp buf
	ldr           cov_count,  [cov_aac_buf_ptr, #4*(11)]             @//load cov_c22_re_hi into temp buf
	adds	       cov_c22_re_lo, cov_x_buf_ptr, cov_c11_re_lo  @// cov_c22_re_lo and cov_c11_re_lo are same reg
	adc	       cov_c22_re_hi, cov_count,  cov_c11_re_hi        @// cov_c22_re_hi and cov_c11_re_hi are same reg
	rsb          cov_x_buf_ptr, cov_x0_re_ptr, #0
	smlal       cov_c22_re_lo, cov_c22_re_hi, cov_x0_re_ptr, cov_x_buf_ptr
	rsb          cov_count,  cov_x0_im_ptr, #0
	smlal       cov_c22_re_lo, cov_c22_re_hi, cov_x0_im_ptr, cov_count
	
	add		cov_aac_buf_ptr, cov_aac_buf_ptr, #(4*6)
	stmia	cov_aac_buf_ptr, {cov_c12_re_lo-cov_c22_re_hi}

	ldmfd	sp!, {r4-r11, pc}
	@ENDFUNC

@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
cov_c02_re_lo		.req 	r7
cov_c02_re_hi		.req	r8
cov_c02_im_lo		.req  r9
cov_c02_im_hi		.req 	r10
cov_x2_re_ptr		.req 	r11
cov_x2_im_ptr		.req 	r12

@ void AAC_SBRGenConvCalculate1(int *cov_x_buf_ptr, int *cov_aac_buf_ptr)

AAC_SBRGenConvCalculate1:   @		FUNCTION
	.global	AAC_SBRGenConvCalculate1
	stmfd	sp!, {r4-r11, r14}
	
	ldr		cov_x0_re_ptr, [cov_x_buf_ptr], #4*(1)
	ldr		cov_x0_im_ptr, [cov_x_buf_ptr], #4*(2*64-1)
	ldr		cov_x1_re_ptr, [cov_x_buf_ptr], #4*(1)
	ldr		cov_x1_im_ptr, [cov_x_buf_ptr], #4*(2*64-1)		
	@@/* 1 */
	ldr		cov_x2_re_ptr, [cov_x_buf_ptr], #4*(1)
	ldr		cov_x2_im_ptr, [cov_x_buf_ptr], #4*(2*64-1)
	rsb		cov_x0_im_neg, cov_x0_im_ptr, #0
	smull	cov_c02_re_lo, cov_c02_re_hi, cov_x2_re_ptr, cov_x0_re_ptr
	smlal	cov_c02_re_lo, cov_c02_re_hi, cov_x2_im_ptr, cov_x0_im_ptr
	smull	cov_c02_im_lo, cov_c02_im_hi, cov_x0_re_ptr, cov_x2_im_ptr
	smlal	cov_c02_im_lo, cov_c02_im_hi, cov_x0_im_neg, cov_x2_re_ptr	

	@@/* 2 */
	ldr		cov_x0_re_ptr, [cov_x_buf_ptr], #4*(1)
	ldr		cov_x0_im_ptr, [cov_x_buf_ptr], #4*(2*64-1)
	rsb		cov_x0_im_neg, cov_x1_im_ptr, #0
	smlal	cov_c02_re_lo, cov_c02_re_hi, cov_x0_re_ptr, cov_x1_re_ptr
	smlal	cov_c02_re_lo, cov_c02_re_hi, cov_x0_im_ptr, cov_x1_im_ptr
	smlal	cov_c02_im_lo, cov_c02_im_hi, cov_x1_re_ptr, cov_x0_im_ptr
	smlal	cov_c02_im_lo, cov_c02_im_hi, cov_x0_im_neg, cov_x0_re_ptr	
	
	mov		cov_count, #(16*2 + 4)
AAC_SBRGen_CV2_Loop_Start:
	@@/* 3 */
	ldr		cov_x1_re_ptr, [cov_x_buf_ptr], #4*(1)
	ldr		cov_x1_im_ptr, [cov_x_buf_ptr], #4*(2*64-1)
	rsb		cov_x0_im_neg, cov_x2_im_ptr, #0
	smlal	cov_c02_re_lo, cov_c02_re_hi, cov_x1_re_ptr, cov_x2_re_ptr
	smlal	cov_c02_re_lo, cov_c02_re_hi, cov_x1_im_ptr, cov_x2_im_ptr
	smlal	cov_c02_im_lo, cov_c02_im_hi, cov_x2_re_ptr, cov_x1_im_ptr
	smlal	cov_c02_im_lo, cov_c02_im_hi, cov_x0_im_neg, cov_x1_re_ptr	
	@@/* 1 */
	ldr		cov_x2_re_ptr, [cov_x_buf_ptr], #4*(1)
	ldr		cov_x2_im_ptr, [cov_x_buf_ptr], #4*(2*64-1)
	rsb		cov_x0_im_neg, cov_x0_im_ptr, #0
	smlal	cov_c02_re_lo, cov_c02_re_hi, cov_x2_re_ptr, cov_x0_re_ptr
	smlal	cov_c02_re_lo, cov_c02_re_hi, cov_x2_im_ptr, cov_x0_im_ptr
	smlal	cov_c02_im_lo, cov_c02_im_hi, cov_x0_re_ptr, cov_x2_im_ptr
	smlal	cov_c02_im_lo, cov_c02_im_hi, cov_x0_im_neg, cov_x2_re_ptr
	@@/* 2 */
	ldr		cov_x0_re_ptr, [cov_x_buf_ptr], #4*(1)
	ldr		cov_x0_im_ptr, [cov_x_buf_ptr], #4*(2*64-1)
	rsb		cov_x0_im_neg, cov_x1_im_ptr, #0
	smlal	cov_c02_re_lo, cov_c02_re_hi, cov_x0_re_ptr, cov_x1_re_ptr
	smlal	cov_c02_re_lo, cov_c02_re_hi, cov_x0_im_ptr, cov_x1_im_ptr
	smlal	cov_c02_im_lo, cov_c02_im_hi, cov_x1_re_ptr, cov_x0_im_ptr
	smlal	cov_c02_im_lo, cov_c02_im_hi, cov_x0_im_neg, cov_x0_re_ptr
	
	subs	cov_count, cov_count, #3
	BGT		AAC_SBRGen_CV2_Loop_Start
	stmia	cov_aac_buf_ptr, {cov_c02_re_lo-cov_c02_im_hi}

	ldmfd	sp!, {r4-r11, pc}
	@ENDFUNC

    

@// clac the energe 
@// bs_interpol_freq1
XBuf_ptr	         .req 	 r0
tE_curr_ptr        .req	 r1
M                      .req  r2
i_div                  .req 	 r3
E_P02RELO     .req  r4
E_P02REHI       .req  r5
T_XBuf_ptr        .req  r6
E_re                   .req  r7
E_im                   .req  r8
ii                         .req   r9
ii_bak                 .req   r10

@// void AAC_SBRAdj_CalcInterpolFreq1Enrg(int32_t *XBuf_ptr, int32_t *tE_curr_ptr, int16_t M, int16_t i_div)
@//   see comments in sbrhfadj.c
AAC_SBRAdj_CalcInterpolFreq1Enrg:   @		FUNCTION
    .global	AAC_SBRAdj_CalcInterpolFreq1Enrg
    stmfd	sp!, {r4-r11, r14}	
    and      ii,    M, #0xFF
    mov      M,     M, lsr #16
    sub      ii,    ii, #1
    mov     ii_bak,    ii

AAC_SBRAdj_CalcInterpolFreq1Enrg_M_Loop_Start:
    mov     E_P02RELO,  #0
    mov     E_P02REHI,  #0
    mov     T_XBuf_ptr, XBuf_ptr    
    add     XBuf_ptr,   XBuf_ptr, #8    
    mov     ii,         ii_bak    
AAC_SBRAdj_CalcInterpolFreq1Enrg_II_Loop_Start:
    ldr		E_re, [T_XBuf_ptr], #4
    ldr		E_im, [T_XBuf_ptr], #508
    smlal	E_P02RELO, E_P02REHI, E_re, E_re
    smlal	E_P02RELO, E_P02REHI, E_im, E_im

    ldr		E_re, [T_XBuf_ptr], #4
    ldr		E_im, [T_XBuf_ptr], #508
    smlal	E_P02RELO, E_P02REHI, E_re, E_re
    smlal	E_P02RELO, E_P02REHI, E_im, E_im
    @// internal loop contrl
    subs	ii, ii, #2
    bge	AAC_SBRAdj_CalcInterpolFreq1Enrg_II_Loop_Start

    mul     E_P02REHI, i_div, E_P02REHI
    mov   E_P02RELO,  E_P02RELO, lsr #16
        
    mul     E_P02RELO,  i_div, E_P02RELO
    mov    E_P02RELO,  E_P02RELO, lsr #16
    
    add     E_P02REHI,  E_P02REHI, E_P02RELO
    
    str     E_P02REHI, [tE_curr_ptr], #4
    @// external loop contrl
    subs	M, M, #1
    BGT		AAC_SBRAdj_CalcInterpolFreq1Enrg_M_Loop_Start
     
    ldmfd	sp!, {r4-r11, pc}
    @ENDFUNC
	
	
	
@// clac the energe 
@ bs_interpol_freq0
XBuf_ptr		.req 	r0
tE_curr_ptr		.req	r1
M	  	        .req  r2
i_div           .req 	r3
E_P02RELO       .req  r4
E_P02REHI       .req  r5
T_XBuf_ptr      .req  r6
E_re            .req  r7
E_im            .req  r8
ii              .req  r9
ii_bak          .req  r10
XBuf_ptr_bak    .req  r11

@ void AAC_SBRAdj_CalcInterpolFreq0Enrg(int32_t *XBuf_ptr, int32_t *tE_curr_ptr, int16_t M, int16_t i_div)
@   see comments in sbrhfadj.c

AAC_SBRAdj_CalcInterpolFreq0Enrg:   @		FUNCTION
	.global	AAC_SBRAdj_CalcInterpolFreq0Enrg
	stmfd	sp!, {r4-r11, r14}
	mov     XBuf_ptr_bak,   XBuf_ptr
	
    and      ii,    M, #0xFF
    mov      M,     M, lsr #16
    
    sub      ii,    ii, #1
    rsb      XBuf_ptr_bak, ii, #63
    mov      XBuf_ptr_bak, XBuf_ptr_bak, lsl #3
    
    mov      ii_bak,    ii
     
    mov      E_P02RELO, #0
    mov      E_P02REHI, #0
AAC_SBRAdj_CalcInterpolFreq0Enrg_M_Loop_Start:
    
AAC_SBRAdj_CalcInterpolFreq0Enrg_Internal_Loop_Start:
    
    ldmia		XBuf_ptr!, {E_re,E_im}
    
    smlal	E_P02RELO, E_P02REHI, E_re, E_re
	smlal	E_P02RELO, E_P02REHI, E_im, E_im
    
    subs   ii,   ii,  #1
    bge    AAC_SBRAdj_CalcInterpolFreq0Enrg_Internal_Loop_Start
    @// external loop contrl
    mov    ii,  ii_bak
    add     XBuf_ptr, XBuf_ptr, XBuf_ptr_bak
	subs	M, M, #1
	BGT		AAC_SBRAdj_CalcInterpolFreq0Enrg_M_Loop_Start

	mul     E_P02REHI, i_div, E_P02REHI
    mov     E_P02RELO,  E_P02RELO, lsr #16
        
    mul     E_P02RELO,  i_div, E_P02RELO
    mov     E_P02RELO,  E_P02RELO, lsr #16
    
    add     E_P02REHI,  E_P02REHI, E_P02RELO
    
AAC_SBRAdj_CalcInterpolFreq0Enrg_Save_Loop_Start:
    str     E_P02REHI,   [tE_curr_ptr], #4
    subs   ii_bak,   ii_bak,  #1
    bge    AAC_SBRAdj_CalcInterpolFreq0Enrg_Save_Loop_Start
    
    
	ldmfd	sp!, {r4-r11, pc}
	@ENDFUNC	
	
	
	

        
         
@@/////////////////////////////////////////////////
@/************************************************/
@/*     PS model    */         

X_ptr		.req 	r0
y_curr_ptr  .req	r0
ff0	  	    .req  r1
ff1         .req 	r2
ff2         .req 	r3
ff3         .req 	r4
ff4         .req 	r5
ff5         .req 	r6
ff6         .req 	r7
ff7         .req 	r8
ff8         .req 	r9




@ void AAC_PS_DCT3_4_unscaledAsm(int32_t *X_ptr, int32_t *y_curr_ptr)
@   see comments in ps_decorrelate.c
AAC_PS_DCT3_4_unscaledAsm:   @   FUNCTION
        .global	AAC_PS_DCT3_4_unscaledAsm	
        stmfd	sp!, {r1-r11, r14}
        
        ldr r8, =AAC_PS_DCT_iii_IV_TAB
        
        @@@@@@@@@@@@@@@@@@@@@@@@@
        ldr  r10,   [X_ptr, #8]  @@// x[2]
        ldr  r9,   [r8], #4           
        mov  r10,  r10,  lsl #1

        @@// calc f0
        smull     r12,   ff0,   r10,  r9
        ldr   r10,  [X_ptr]
        sub   ff1,   r10,  ff0
        add   ff2,   r10,  ff0
        
        ldr   r9,   [X_ptr, #4]    @@// x[1]
        ldr   r10,  [X_ptr, #12]   @@// x[3]
        ldr   r11,   [r8], #4            
        add   ff3,   r9,  r10
        mov   r9,    r9,  lsl #3
        
    
        smull r12,   ff4,   r9,  r11
        ldr   r11,   [r8], #4
        mov   ff3,   ff3,  lsl #1
        ldr   r8,   [r8]
        mov   r10,  r10,  lsl #1
        smull r12,   ff5,   ff3,  r11      
        
        smull r12,   ff6,   r10,  r8
        
        add   ff7,   ff4, ff5
        sub   ff8,   ff6, ff5
        
        add   ff0,   ff2,   ff8
        str   ff0,   [y_curr_ptr]
        add   ff0,   ff1,   ff7
        str   ff0,   [y_curr_ptr, #4]
        
        sub   ff0,   ff1,   ff7
        str   ff0,   [y_curr_ptr, #8]
        
        sub   ff0,   ff2,   ff8
        str   ff0,   [y_curr_ptr, #12]
        
        @@@@@@@@@@@@@@@@@@@@@@@@@
	    ldmfd	sp!, {r1-r11, pc}
AAC_PS_DCT_iii_IV_TAB:
				.word   0x5A82799A
				.word   0x29CF5D23
				.word   0x89be50c3
				.word   0xbaba1611    
	    
    	@ENDFUNC	
    	
    
    	


    

    	@@//void  AAC_SBR_HfGenerateAsm(AAC_SBR_INFO_T *sbr_ptr,
    	@@//                                                      aac_complex              *Xlow,
    	@//                                                       uint8                             ch,
    	@@//                                                      int32                           *alpha_ptr)
gen_sbr_ptr                   .req    r0
bwArray_ptr                  .req    r9
gen_k                            .req    r12
gen_p                            .req    r11
gen_low_ptr                  .req    r11
gen_high_ptr                  .req   r12
table_map_k_to_g_ptr  .req   r2
gen_alpha_ptr               .req   r3



gt_X20                    .req r0
gt_X21                    .req r1
gt_X10                    .req r2
gt_X11                    .req r3



AAC_SBR_HfGenerateAsm:   @   FUNCTION
        .global	AAC_SBR_HfGenerateAsm	
        stmfd	sp!, {r4-r11, r14}
        sub           sp,     sp,    #16*8
    	@@////////////////////////////////////////
        add         r5,          gen_sbr_ptr,   #20  @@// t_E
    	
        ldrb          r14,       [gen_sbr_ptr,  #3]
        ldrb          gen_k,   [gen_sbr_ptr,  #0]
    	
        add           bwArray_ptr,    gen_sbr_ptr,   #0xF70
        add           gen_sbr_ptr,   bwArray_ptr,    #2*5*4
        add           r6,                  gen_sbr_ptr,    #80  @@// L_E
        cmp          r2,   #0
        addne       r5,   r5,  #6      
        addne       r6,   r6,  #1       
        addne       bwArray_ptr,  bwArray_ptr,   #5*4
        str             bwArray_ptr,   [sp,  #0]    @@//  bwArray_ptr base addr(r3)
        cmp          r14,  #0
        BLE         AAC_SBR_HfGenerateAsm_Exit        
        ldrb          r6,         [r6]                           @@// sbr_ptr->L_E[ch]
        add          table_map_k_to_g_ptr,      gen_sbr_ptr,   #16
        str            table_map_k_to_g_ptr,                  [sp,   #2*4]     @@// table_map_k_to_g(r2)
        ldrb          gen_p,    [gen_sbr_ptr], #1
        
        ldrb          r6,      [r5,   r6]   @//   last
        ldrb          r5,      [r5]          @@//  first        
           
        @//str            gen_p,            [sp,   #5*4]     @@// p      
          
        add          r1,          r1,     r5,   lsl    #9  @@// *64*2*4   base addr
        subs          r10,        r6,      r5      @@// lasr - first
        BLE         AAC_SBR_HfGenerateAsm_Exit        
        str           r10,                 [sp,   #6*4]@@// lasr - first
        str           r1,                   [sp,   #7*4]@@// base freq addr
        
        str           gen_alpha_ptr,   [sp,  #14*4]        @@//gen_alpha_ptr
SBR_GEN_PatchesLOOP:
    	str             r14,   [sp,  #1*4]    @@//  noPatches
    	str            gen_sbr_ptr,   [sp,   #4*4]     @@// gen_sbr_ptr
    	str            gen_k,            [sp,   #3*4]     @@// k     
    	@/////////////////////////////////////////////////////////////////////
    	ldrb           r14,             [gen_sbr_ptr,  #7]  @@// patchNoSubbands = sbr_ptr->patchNoSubbands[i]@
    	add           table_map_k_to_g_ptr,      table_map_k_to_g_ptr,    gen_k
    	add           gen_alpha_ptr,                   gen_alpha_ptr,     gen_p,     lsl   #4
    	add           gen_high_ptr,       r1,   gen_k,   lsl  #3
    	add           gen_low_ptr,       r1,    gen_p,   lsl  #3
    	ldrb           r4,     [table_map_k_to_g_ptr],  #1
    	cmp          r14,   #0
    	BLE         AAC_SBR_HfGenerateAsm_Exit        
    	@@/* r4, r5, r6, r7, r8,  gen_sbr_ptr are free */
SBR_GEN_patchNoSubbandsLOOP:
    	ldr            r4,  [bwArray_ptr,    r4, lsl #2]
    	add          gen_alpha_ptr,  gen_alpha_ptr,   #4*4
               str          gen_low_ptr,  [sp,  #12*4]
    	str          gen_high_ptr,  [sp,  #13*4]
    	@@//////////////////////////////////////////////////
    	cmp         r4,    #0
    	BEQ        SBR_GEN_patchNoSubbandsLOOPEQBRANCH
    	SMULL  r7,   r8,   r4,  r4
    	ldr           r5,        [gen_alpha_ptr,   #-4*4]
    	ldr           r6,        [gen_alpha_ptr,   #-3*4]    	
    	mov         r8,       r8,  lsl   #1               @@//     r6 -> i_bw2    	
    	SMULL  gen_sbr_ptr,  r5,   r4,     r5
    	SMULL  gen_sbr_ptr,  r6,   r4,     r6    	
    	mov        r5,  r5,    lsl  #1  @@// i_a[0]  -> r5
    	mov        r6,  r6,    lsl  #1  @@// i_a[1]  -> r6
    	str          r1,  [sp,  #10*4]
    	ldr           r4,        [gen_alpha_ptr,   #-2*4]
    	ldr           r7,        [gen_alpha_ptr,   #-1*4]    	
    	
    	SMULL  gen_sbr_ptr,  r1,   r4,     r8
    	SMULL  gen_sbr_ptr,  r8,   r7,     r8  
    	
    	mov        r7,  r1,    lsl  #1  @@// i_a[2]  -> r7
    	mov        r8,  r8,    lsl  #1  @@// i_a[3]  -> r8  	
    	
    	str          gen_alpha_ptr,  [sp,  #8*4]
    	str          r14,  [sp,  #9*4]
    	
    	str          table_map_k_to_g_ptr,  [sp,  #11*4]
    	@@/* r4,  r0, r1,  r2, r3,  r9,  r14,  (r10 used), are free */
    	
    	ldr     gt_X20, [gen_low_ptr, #-1024]
    	ldr     gt_X21, [gen_low_ptr, #-1020]    	
    	ldr     gt_X10, [gen_low_ptr, #-512]
    	ldr     gt_X11, [gen_low_ptr, #-508]
    	
SBR_GEN_FILTER_LOOP:
        @@/* 1 */
    	@/////////////////////////////    	
    	smull   r4, r9,  r5,  gt_X10
        smlal   r4, r9,  r7,  gt_X20        
        smull   r4, r14,   r6,  gt_X11
        smlal   r4, r14,  r8,  gt_X21        
        
        sub     r9,  r9, r14                
        smull   r4,  r14, r5,   gt_X11
        smlal   r4,  r14, r6,   gt_X10
        smlal   r4,  r14, r7,   gt_X21        
        smlal   r4,  r14, r8,   gt_X20       
                   
        @//mov     gt_X20,  gt_X10
        @//mov     gt_X21,  gt_X11        
        
        ldr     gt_X20,  [gen_low_ptr], #4
        ldr     gt_X21,  [gen_low_ptr], #508        
        
        add     r9,        gt_X20, r9, lsl #3
        add     r14,      gt_X21, r14,    lsl #3        
        
        str     r9,        [gen_high_ptr], #4
        str     r14,      [gen_high_ptr], #508     	
        
        
        @@/* 2 */
        @/////////////////////////////    	
    	smull   r4, r9,  r5,  gt_X20
        smlal   r4, r9,  r7,  gt_X10        
        smull   r4, r14,   r6,  gt_X21
        smlal   r4, r14,  r8,  gt_X11        
        subs     r10, r10,  #2
        sub     r9,  r9, r14                
        smull   r4,  r14, r5,   gt_X21
        smlal   r4,  r14, r6,   gt_X20
        smlal   r4,  r14, r7,   gt_X11        
        smlal   r4,  r14, r8,   gt_X10       
                   
        @//mov     gt_X20,  gt_X10
        @//mov     gt_X21,  gt_X11        
        
        ldr     gt_X10,  [gen_low_ptr], #4
        ldr     gt_X11,  [gen_low_ptr], #508        
        
        add     r9,        gt_X10, r9, lsl #3
        add     r14,      gt_X11, r14,    lsl #3        
        
        str     r9,        [gen_high_ptr], #4
        str     r14,      [gen_high_ptr], #508     
        
        
        
        
    	@/////////////////////////////    	
    	BGT    SBR_GEN_FILTER_LOOP    	
    	ldr          gen_alpha_ptr,  [sp,  #8*4]
    	ldr          bwArray_ptr,   [sp,  #0]    @@//  bwArray_ptr base addr(r3)
    	ldr          r14,  [sp,  #9*4]
    	ldr          r1,  [sp,  #10*4]
    	ldr          table_map_k_to_g_ptr,  [sp,  #11*4]
    	
    	B         SBR_GEN_patchNoSubbandsLOOPEQ_NEXT_B
SBR_GEN_patchNoSubbandsLOOPEQBRANCH:
    	ldr         r4,    [gen_low_ptr], #4
    	ldr         r5,    [gen_low_ptr],    #127*4    	
    	
    	str         r4,    [gen_high_ptr], #4
    	str         r5,    [gen_high_ptr],    #127*4  	    	
    	
    	ldr         r4,    [gen_low_ptr], #4
    	ldr         r5,    [gen_low_ptr],    #127*4    	
    	
    	str         r4,    [gen_high_ptr], #4
    	str         r5,    [gen_high_ptr],    #127*4  
    	
    	
    	subs       r10,  r10,   #2
    	BGT      SBR_GEN_patchNoSubbandsLOOPEQBRANCH   	
    	
SBR_GEN_patchNoSubbandsLOOPEQ_NEXT_B:
    	@@////////////////////////
    	ldr          gen_low_ptr,  [sp,  #12*4]
    	ldr          gen_high_ptr,  [sp,  #13*4]
    	ldrb           r4,     [table_map_k_to_g_ptr],  #1   
    	
    	add         	gen_low_ptr,  gen_low_ptr,  #8
    	add          gen_high_ptr, gen_high_ptr,  #8
    	
    	ldr           r10,                 [sp,   #6*4]@@// lasr - first
        
    	
    	subs         r14,           r14,  #1
    	BGT        SBR_GEN_patchNoSubbandsLOOP
    	@/////////////////////////////////////////////////////////////////////    	
    	ldr            gen_sbr_ptr,   [sp,   #4*4]     @@// gen_sbr_ptr  	    	
    	ldr            table_map_k_to_g_ptr,                  [sp,   #2*4]     @@// table_map_k_to_g(r2)
    	ldr            gen_alpha_ptr,   [sp,  #14*4]        @@//gen_alpha_ptr
    	@/////////////////////////////////////////////////////////////////////
    	ldr          r14,   [sp,  #1*4]    @@//  noPatches
    	ldrb        r6,            [gen_sbr_ptr, #7]
    	ldr          gen_k,            [sp,   #3*4]     @@// k     
    	ldrb        gen_p,            [gen_sbr_ptr],   #1
    	ldr          r1,                   [sp,   #7*4]@@// base freq addr
    	add        gen_k,  gen_k,   r6    	
    	
    	subs       r14,  r14,  #1
    	BGT           SBR_GEN_PatchesLOOP
    	
    	
    	@@///////////////////////////////////////
AAC_SBR_HfGenerateAsm_Exit:
    	add           sp,     sp,    #16*8
        ldmfd	sp!, {r4-r11, pc} 
    	@ENDFUNC   
    	
    	
    	
    	
    	
    	
ML_data0   .req    r3    	
    		  	  
L_data1    .req    r4    	  
L_data2    .req    r5
L_data3    .req    r6
L_data4    .req    r7
    	  
R_data1    .req    r8    	  
R_data2    .req    r9
R_data3    .req    r10
R_data4    .req    r11
    	  
@ void AAC_MSAsm(int32_t *l_spec_ptr, int32_t *r_spec_ptr, int16_t offset_start, int16_t offset_end)
@   see comments in ps_decorrelate.c
AAC_MSAsm:   @   FUNCTION
        .global	AAC_MSAsm	
        stmfd	sp!, {r4-r11, r14}
    	add       r0,   r0,  r2,  lsl #2
    	add       r1,   r1,  r2,  lsl #2
    	
    	sub       r2,   r3,  r2
AAC_MSAsm_LOOP:
        
        ldmia     r0,  {L_data1-L_data4}
        ldmia     r1,  {R_data1-R_data4}
        
        add       ML_data0,  L_data1, R_data1
        sub       R_data1,   L_data1, R_data1
        
        add       L_data1,  L_data2, R_data2
        sub       R_data2,   L_data2, R_data2
        
    	add       L_data2,  L_data3, R_data3
        sub       R_data3,   L_data3, R_data3
        
        add       L_data3,  L_data4, R_data4
        sub       R_data4,   L_data4, R_data4
          
        stmia     r0!,  {ML_data0-L_data3}
        stmia     r1!,  {R_data1-R_data4}
        
        
    	subs      r2, r2,  #4
    	BGT       AAC_MSAsm_LOOP
    	
    	  
    	ldmfd	sp!, {r4-r11, pc} 	
    	
    	@ENDFUNC   	  	  
    	    
    	  
    	  
    	  
    	  
    	  
	  
    	  
 
    	  
    	
    	
    	        @/////////////////////////////////
    	        @//int32 AAC_DEC_ARM_Log2IntAsm(
                @//                             int32 val,
                @//                             int16 *table_ptr)@
AAC_DEC_ARM_Log2IntAsm_frac       .req r3
AAC_DEC_ARM_Log2IntAsm_index      .req r4
AAC_DEC_ARM_Log2IntAsm_indexFrac  .req r14
AAC_DEC_ARM_Log2IntAsm_exp        .req r2
AAC_DEC_ARM_Log2IntAsm_x1         .req r12

                @// r0, val
AAC_DEC_ARM_Log2IntAsm:   @   FUNCTION
                @ // save the value for the following calculation
                .global  AAC_DEC_ARM_Log2IntAsm
                stmfd   sp!, {r4, r14}
                cmp     r0,  #0              
                movle   r0,   #16384
                rsble   r0,   r0,  #0
                ble     AAC_DEC_ARM_Log2IntAsmExit
                
                clz     AAC_DEC_ARM_Log2IntAsm_exp,    r0    @@// 
                rsbs    AAC_DEC_ARM_Log2IntAsm_exp,  AAC_DEC_ARM_Log2IntAsm_exp, #17 @@// 
                movge   AAC_DEC_ARM_Log2IntAsm_frac,  r0, asr AAC_DEC_ARM_Log2IntAsm_exp
                rsblt   AAC_DEC_ARM_Log2IntAsm_x1,  AAC_DEC_ARM_Log2IntAsm_exp,  #0
                movlt   AAC_DEC_ARM_Log2IntAsm_frac,  r0, lsl AAC_DEC_ARM_Log2IntAsm_x1   @@// r3 -> frac
                
                mov     AAC_DEC_ARM_Log2IntAsm_index, AAC_DEC_ARM_Log2IntAsm_frac, asr #8      
                and     AAC_DEC_ARM_Log2IntAsm_index, AAC_DEC_ARM_Log2IntAsm_index, #0x3F
                add     r1,  r1, AAC_DEC_ARM_Log2IntAsm_index, lsl #1
                ldrsh   AAC_DEC_ARM_Log2IntAsm_x1, [r1], #2     
                ldrsh   r0, [r1]
                and     AAC_DEC_ARM_Log2IntAsm_indexFrac, AAC_DEC_ARM_Log2IntAsm_frac, #0xFF
                sub     r0, r0, AAC_DEC_ARM_Log2IntAsm_x1
                mul     r0, AAC_DEC_ARM_Log2IntAsm_indexFrac, r0
                add     AAC_DEC_ARM_Log2IntAsm_x1, AAC_DEC_ARM_Log2IntAsm_x1, r0, asr #8
                
                add     AAC_DEC_ARM_Log2IntAsm_exp, AAC_DEC_ARM_Log2IntAsm_exp, #14
                mov     AAC_DEC_ARM_Log2IntAsm_exp, AAC_DEC_ARM_Log2IntAsm_exp, lsl #14
                add     r0,     AAC_DEC_ARM_Log2IntAsm_x1, AAC_DEC_ARM_Log2IntAsm_exp             
                           
AAC_DEC_ARM_Log2IntAsmExit:
                ldmfd   sp!, {r4, pc}
                @ENDFUNC 
    	
    	
    	
    	
    	
    	        @/////////////////////////////////
    	        @//int32 AAC_DEC_ARM_Pow2IntAsm(
                @//                             int32 val,
                @//                             int16 *table_ptr)@
AAC_DEC_ARM_Pow2IntAsm_frac       .req r3
AAC_DEC_ARM_Pow2IntAsm_index      .req r4
AAC_DEC_ARM_Pow2IntAsm_indexFrac  .req r5
AAC_DEC_ARM_Pow2IntAsm_whole      .req r6
AAC_DEC_ARM_Pow2IntAsm_x1         .req r2
AAC_DEC_ARM_Pow2IntAsm_x2         .req r12
AAC_DEC_ARM_Pow2IntAsm_rest       .req r14
                @// r0, val
                @// r1, table_ptr
AAC_DEC_ARM_Pow2IntAsm:   @   FUNCTION
                @ // save the value for the following calculation
                .global  AAC_DEC_ARM_Pow2IntAsm
                stmfd   sp!, {r4-r6, r14}
                cmp     r0,  #0     
                moveq   r0,  #1           
                beq     AAC_DEC_ARM_Pow2IntAsmExit                
                mov     AAC_DEC_ARM_Pow2IntAsm_whole,  r0,  asr #14
                sub     AAC_DEC_ARM_Pow2IntAsm_rest,   r0,  AAC_DEC_ARM_Pow2IntAsm_whole, lsl #14                
                mov     AAC_DEC_ARM_Pow2IntAsm_index,AAC_DEC_ARM_Pow2IntAsm_rest, asr  #8                
                and     AAC_DEC_ARM_Pow2IntAsm_indexFrac, AAC_DEC_ARM_Pow2IntAsm_rest, #0xFF                
                cmp     AAC_DEC_ARM_Pow2IntAsm_whole, #0
                movle   r0,  #0
                ble     AAC_DEC_ARM_Pow2IntAsmExit                
                and     AAC_DEC_ARM_Pow2IntAsm_index,  AAC_DEC_ARM_Pow2IntAsm_index, #0xFF
                add     r1,   r1,  AAC_DEC_ARM_Pow2IntAsm_index, lsl #1
                ldrsh   AAC_DEC_ARM_Pow2IntAsm_x1, [r1], #2
                ldrsh   AAC_DEC_ARM_Pow2IntAsm_x2, [r1]                
                sub     AAC_DEC_ARM_Pow2IntAsm_x2,  AAC_DEC_ARM_Pow2IntAsm_x2,  AAC_DEC_ARM_Pow2IntAsm_x1                
                mul     AAC_DEC_ARM_Pow2IntAsm_indexFrac, AAC_DEC_ARM_Pow2IntAsm_x2, AAC_DEC_ARM_Pow2IntAsm_indexFrac
                add     AAC_DEC_ARM_Pow2IntAsm_x1, AAC_DEC_ARM_Pow2IntAsm_x1, AAC_DEC_ARM_Pow2IntAsm_indexFrac, asr #8
                subs    AAC_DEC_ARM_Pow2IntAsm_whole, AAC_DEC_ARM_Pow2IntAsm_whole, #14
                movge   r0,  AAC_DEC_ARM_Pow2IntAsm_x1, lsl AAC_DEC_ARM_Pow2IntAsm_whole
                BGE     AAC_DEC_ARM_Pow2IntAsmExit
                rsblt   AAC_DEC_ARM_Pow2IntAsm_whole,  AAC_DEC_ARM_Pow2IntAsm_whole,  #0
                movlts   r0,  AAC_DEC_ARM_Pow2IntAsm_x1, asr AAC_DEC_ARM_Pow2IntAsm_whole                           
                ADC     r0,   r0,  #0
AAC_DEC_ARM_Pow2IntAsmExit:
                ldmfd   sp!, {r4-r6, pc}
                @ENDFUNC 
    	
    	        @/////////////////////////////////
    	        @//int32 AAC_DEC_ARM_Pow2IntOut14bitAsm(
                @//                             int32 val,
                @//                             int16 *table_ptr)@
AAC_DEC_ARM_Pow2IntOut14bitAsm_frac       .req r3
AAC_DEC_ARM_Pow2IntOut14bitAsm_index      .req r4
AAC_DEC_ARM_Pow2IntOut14bitAsm_indexFrac  .req r5
AAC_DEC_ARM_Pow2IntOut14bitAsm_whole      .req r6
AAC_DEC_ARM_Pow2IntOut14bitAsm_x1         .req r2
AAC_DEC_ARM_Pow2IntOut14bitAsm_x2         .req r12
AAC_DEC_ARM_Pow2IntOut14bitAsm_rest       .req r14
                @// r0, val
                @// r1, table_ptr
AAC_DEC_ARM_Pow2IntOut14bitAsm:   @   FUNCTION
                @ // save the value for the following calculation
                .global  AAC_DEC_ARM_Pow2IntOut14bitAsm
                stmfd   sp!, {r4-r6, r14}
                cmp     r0,  #0     
                moveq   r0,  #16384           
                beq     AAC_DEC_ARM_Pow2IntOut14bitAsmExit                
                mov     AAC_DEC_ARM_Pow2IntOut14bitAsm_whole,  r0,  asr #14
                sub     AAC_DEC_ARM_Pow2IntOut14bitAsm_rest,   r0,  AAC_DEC_ARM_Pow2IntOut14bitAsm_whole, lsl #14                
                mov     AAC_DEC_ARM_Pow2IntOut14bitAsm_index,AAC_DEC_ARM_Pow2IntOut14bitAsm_rest, asr  #8                
                and     AAC_DEC_ARM_Pow2IntOut14bitAsm_index,  AAC_DEC_ARM_Pow2IntOut14bitAsm_index, #0xFF
                add     r1,   r1,  AAC_DEC_ARM_Pow2IntOut14bitAsm_index, lsl #1
                ldrsh   AAC_DEC_ARM_Pow2IntOut14bitAsm_x1, [r1], #2
                ldrsh   AAC_DEC_ARM_Pow2IntOut14bitAsm_x2, [r1]
                and     AAC_DEC_ARM_Pow2IntOut14bitAsm_indexFrac, AAC_DEC_ARM_Pow2IntOut14bitAsm_rest, #0xFF                
                sub     AAC_DEC_ARM_Pow2IntOut14bitAsm_x2,  AAC_DEC_ARM_Pow2IntOut14bitAsm_x2,  AAC_DEC_ARM_Pow2IntOut14bitAsm_x1
                mul     AAC_DEC_ARM_Pow2IntOut14bitAsm_indexFrac, AAC_DEC_ARM_Pow2IntOut14bitAsm_x2, AAC_DEC_ARM_Pow2IntOut14bitAsm_indexFrac
                add     AAC_DEC_ARM_Pow2IntOut14bitAsm_x1, AAC_DEC_ARM_Pow2IntOut14bitAsm_x1, AAC_DEC_ARM_Pow2IntOut14bitAsm_indexFrac, asr #8
                cmp     AAC_DEC_ARM_Pow2IntOut14bitAsm_whole, #0
                mov     r0,  AAC_DEC_ARM_Pow2IntOut14bitAsm_x1, lsl AAC_DEC_ARM_Pow2IntOut14bitAsm_whole
                bge     AAC_DEC_ARM_Pow2IntOut14bitAsmExit
                rsb     AAC_DEC_ARM_Pow2IntOut14bitAsm_whole, AAC_DEC_ARM_Pow2IntOut14bitAsm_whole, #0
                cmp     AAC_DEC_ARM_Pow2IntOut14bitAsm_whole, #31
                movgt   AAC_DEC_ARM_Pow2IntOut14bitAsm_whole, #31
                mov     r0,  AAC_DEC_ARM_Pow2IntOut14bitAsm_x1, asr AAC_DEC_ARM_Pow2IntOut14bitAsm_whole                
AAC_DEC_ARM_Pow2IntOut14bitAsmExit:
                ldmfd   sp!, {r4-r6, pc}
                @ENDFUNC     	
    	
                
                
                
                .extern   SBR_ADJ_Noise_tab
                .extern   SBR_DEC_phi_re_im
                @// int32 AAC_DecHfAssemblyFilterAsm(int32                                idx_noise_sine_ch_l,                                @@// [31:16]: noise,   [15:12]: sine,      [11:8]: ch,    [7:4]:  l,    [3:0]:  noise
                @//                                                            int32                               *XBuf_ptr, 
                @//                                                            int32                               *G_lim_boost_ptr,
                @//                                                            AAC_SBR_INFO_T     *sbr_ptr)@
@//   see comments in sbrhfadj.c
idx_noise_sine_ch_l   .req    r0
adj_XBuf_ptr            .req    r1
G_lim_boost_ptr       .req    r2
sbr_ptr                      .req    r3
adj_rev                     .req   r12
idx_noise                  .req   r11
idx_sine                    .req   r10

AAC_DecHfAssemblyFilterAsm:   @		FUNCTION
        .global	AAC_DecHfAssemblyFilterAsm
        stmfd	sp!, {r4-r11, r14}	
        sub           sp,      sp,  #8*4
        @@//////////////////////////////////////
        ldrb          r12,   [sbr_ptr,  #0]  @// kx
        ldrb          r9,     [sbr_ptr,  #1]  @// M
	mov         r4,     idx_noise_sine_ch_l,   lsr  #8    @@// ch-> ch
	
	mov         r6,   r3
	ands        r4,     r4,   #0xF
	str              r4,                      [sp,  #6*4]            @@// store ch
	addne        r6,    r6,   #6
	mov           r5,    idx_noise_sine_ch_l,  lsr    #4
	and            r5,    r5,   #0xF	@@//   l	
	mov          r7,   #49*4
	mla          G_lim_boost_ptr,      r5,  r7,  G_lim_boost_ptr
	add            r6,    r6,   r5
	ldrb           r5,   [r6,  #20]
	ldrb           r14,   [r6,  #21]	
	add           adj_XBuf_ptr,   adj_XBuf_ptr,    r12,   lsl   #3
	add           adj_XBuf_ptr,   adj_XBuf_ptr,    #2*64*2*4
	add           adj_XBuf_ptr,   adj_XBuf_ptr,    r5,  lsl  #9	
	subs            r14,   r14,   r5
	ble              AAC_DecHfAssemblyFilterAsmExit
	str              r14,                      [sp,  #5*4]            @@// sbr_ptr->t_E[ch][l+1] - sbr_ptr->t_E[ch][l]
	ands           r12, r12,   #0x1
	moveq        adj_rev,   #1
	moveq        adj_rev,   #-1                                   @@//  rev = (((sbr_ptr->kx) & 1) ? -1 : 1)@	
	
	and            r5,  idx_noise_sine_ch_l,   #0xf
	add           r14,       r5,  r14,    lsl   #16
	sub           r14,       r14,   #32768
	
	mov            idx_noise,     idx_noise_sine_ch_l,   lsr   #16

	mov            idx_sine,     idx_noise_sine_ch_l,   lsr   #12
        and             idx_sine,   idx_sine,   #0xF
        str              sbr_ptr,                 [sp,  #3*4]
	str              G_lim_boost_ptr,  [sp,  #0]                @@// store G_lim_boost_ptr
	str              adj_rev,                [sp,   #2*4]           @@// rev = (((sbr_ptr->kx) & 1) ? -1 : 1)@
	str              r9,                        [sp,   #4*4]           @@// M
	
	ldr             r0,    = SBR_ADJ_Noise_tab   @@// noise table
	
AAC_DecHfAssemblyFilterAsm_EXTERNAL_LOOP:
        @//for (i = sbr_ptr->t_E[ch][l]@ i < t_E; i++)
        ldr             r3,    = SBR_DEC_phi_re_im             @@// SBR_phi_re_im table
        str              adj_XBuf_ptr,    [sp,  #1*4]               @@// store     adj_XBuf_ptr
        ldr             adj_rev,   [sp,   #2*4]
        ldr     r3,      [r3,  idx_sine,  lsl  #2]
        @@//////////
        @@/* r4, r5, r6, r7, r8 are free*/        
AAC_DecHfAssemblyFilterAsm_INTERNAL_LOOP:
        ldr   r4,   [G_lim_boost_ptr,  #5*2*49*4]  @@// S_M_boost        
        ldr   r5,   [G_lim_boost_ptr],  #4                @@// G_filt        
        rsb         adj_rev,  adj_rev,   #0
        add        idx_noise,  idx_noise,   #1
        mov       idx_noise,   idx_noise,  lsl  #23
        mov       idx_noise,   idx_noise,  lsr  #23
                
        movs      r4,    r4,    lsl    #12
        andeqs   r6,    r14,   #0xF
        beq       AAC_DecHfAssemblyFilterAsm_INTERNAL_LOOP_NEXT_BRANCK
        @@/*if (S_M_boost != 0 || no_noise)*/
        ldr                r6,   [adj_XBuf_ptr]
        SMULWB   r8,   r4,     r3  @@// im   sum1
        SMULWT   r7,   r4,     r3  @@// rel   sum0
        SMULL       r4,      r6,   r5,       r6
        mul               r8,    adj_rev,   r8     
        add              r7,   r7,   r4,  lsr     #16
        add              r7,   r7,   r6,  lsl     #16
        ldr               r6,    [adj_XBuf_ptr,  #4]
        str               r7,    [adj_XBuf_ptr],  #4
        
        SMULL       r4,      r6,   r5,       r6
        subs          r9,   r9,  #1
        add              r8,   r8,   r4,  lsr     #16
        add              r8,   r8,   r6,  lsl     #16
        str                r8,    [adj_XBuf_ptr],  #4             
        
        BGT         AAC_DecHfAssemblyFilterAsm_INTERNAL_LOOP     
        B              AAC_DecHfAssemblyFilterAsm_EXTERNAL_LOOP_BRK
AAC_DecHfAssemblyFilterAsm_INTERNAL_LOOP_NEXT_BRANCK:
        @@/*else*/
        ldr   r4,   [G_lim_boost_ptr,  #5*49*4-4]      @@// Q_filt
        ldr   r7,   [r0,  idx_noise,  lsl   #2]
        mov r4,  r4,   asr  #3     @@// Q_filt  >>= 3;
        ldr               r8,   [adj_XBuf_ptr]
        SMULWT  r6,     r4,     r7  @@// sum0
        SMULWB  r7,     r4,     r7  @@// sum1        
        SMULL       r4,      r8,   r5,       r8
        subs             r9,   r9,  #1
        add              r6,   r6,   r8,  lsl     #16
        ldr               r8,    [adj_XBuf_ptr,  #4]
        add              r6,   r6,   r4,  lsr     #16                      
        SMULL       r4,      r8,   r5,       r8
        str                r6,   [adj_XBuf_ptr],  #4
        add              r7,   r7,   r4,  lsr     #16
        add              r7,   r7,   r8,  lsl     #16
        str                r7,    [adj_XBuf_ptr],  #4                 
        BGT         AAC_DecHfAssemblyFilterAsm_INTERNAL_LOOP       
AAC_DecHfAssemblyFilterAsm_EXTERNAL_LOOP_BRK:
        @@//
        ldr              r9,                         [sp,   #4*4]           @@// M
        ldr              adj_XBuf_ptr,              [sp,  #1*4]    @@// store     adj_XBuf_ptr
        ldr              G_lim_boost_ptr,  [sp,  #0]   @@// store G_lim_boost_ptr
        add            idx_sine,  idx_sine,   #1
        and            idx_sine,  idx_sine,  #3
        subs     r14,  r14,    #65536
        add            adj_XBuf_ptr,    adj_XBuf_ptr,    #64*8        
        BGT    AAC_DecHfAssemblyFilterAsm_EXTERNAL_LOOP
        @@//////////////////////////////////////
AAC_DecHfAssemblyFilterAsmExit:
        mov         r0,      idx_noise,            lsl  #16
        add          r0,      r0,  idx_sine,       lsl   #12
        @@/////////////////////
        ldr              sbr_ptr,                 [sp,  #3*4]
        ldr              G_lim_boost_ptr,  [sp,  #0]                @@// store G_lim_boost_ptr
        ldr              r14,                      [sp,  #5*4]            @@// sbr_ptr->t_E[ch][l+1] - sbr_ptr->t_E[ch][l]
        ldr              r11,                      [sp,  #6*4]            @@// ch
        
        add           r10,       sbr_ptr,    #32
        cmp          r11,  #0        
        addne       sbr_ptr,   sbr_ptr,    #2
        ldrh           r9,    [sbr_ptr,  #12]     @@// GQ_ringbuf_index[ch]
        addne       r10,       r10,       #5*49*4
        
        add          r8,      r9,          r14
        
SBR_ADJ_COPY_LOOP_CRTL0:
        subs        r8,            r8,      #5
        BGE      SBR_ADJ_COPY_LOOP_CRTL0
        add        r8,           r8,      #5
        strh        r8,    [sbr_ptr,  #12]     @@// GQ_ringbuf_index[ch]
        
        ldr          r12,                         [sp,   #4*4]           @@// M
        cmp         r14,  #5
        movgt      r14,    #5    
        mov        r8,   #49*4       
        rsb         sbr_ptr,     r12,   #49
SBR_ADJ_COPY_LOOP_CRTL:
        mla         r7,      r9,    r8,  r7               
SBR_ADJ_COPY_LOOP_INTER:
        ldr          r5,         [G_lim_boost_ptr,   #5*49*4]
        ldr          r4,         [G_lim_boost_ptr],  #4
                
        str         r5,          [r10,   #2*5*49*4]
        str         r4,          [r10],  #4
        subs         r12,  r12,  #1
        BGT       SBR_ADJ_COPY_LOOP_INTER
        
        ldr          r12,                         [sp,   #4*4]           @@// M
        add           r9,      r9,  #1
        cmp          r9,     #5
        movge      r9,  #0   
        sub          G_lim_boost_ptr,     G_lim_boost_ptr,  r12,  lsl  #2
        add          r10,    r10,  sbr_ptr,  lsl  #2
        subs          r14,    r14,    #1
        BGT       SBR_ADJ_COPY_LOOP_CRTL

        
        @@////////////////////
        add          sp,      sp,  #8*4
        ldmfd   sp!, {r4-r11, pc}
        @ENDFUNC     	
        
        .extern   AAC_SBR_h_smooth
                @// int32 AAC_DecHfAssemblySmoothFilterAsm(int32    *G_temp_prev_ptr,
                @//                                                                        int32      *ptr,
                @//                                                                        int32      ri,
                @//                                                                        int32     *X_ptr)@
@//   see comments in sbrhfadj.c
ADJ_SMOOTH_tab_ptr     .req    r7
ADJ_G_filt                           .req    r4
ADJ_Q_filt                           .req    r5
ADJ_ri                                  .req   r2
ADJ_rev                               .req   r8
ADJ_tmp_ptr                        .req   r1
S_M                                     .req   r0
AAC_DecHfAssemblySmoothFilterAsm:   @		FUNCTION
        .global	AAC_DecHfAssemblySmoothFilterAsm
        stmfd	sp!, {r4- r8, r14}	
        ldr                     ADJ_SMOOTH_tab_ptr,  =AAC_SBR_h_smooth
        ldr                     r12,   [r0,  #2*5*49*4]   @@//   Q
        movs                 ADJ_rev,                  ADJ_ri,   asr   #31
        rsblt                  ADJ_ri,   r2,   #0
        movge              ADJ_rev,  #1
       
        add                   ADJ_SMOOTH_tab_ptr,    ADJ_SMOOTH_tab_ptr,     ADJ_ri,  lsr  #13
        ldr                     r6,     [ADJ_SMOOTH_tab_ptr],  #4                 @@// tab        
        ldr                     r14,   [r0],  #49*4                 @@//   G
        
        SMULWT        r5,     r12,  r6     @@// Q_filter
        SMULWT        r4,     r14,  r6     @@//G_filter
        ldr                     r12,   [r0,  #2*5*49*4]   @@//   Q
        ldr                     r14,   [r0],  #49*4           @@//   G
        
        SMLAWB        r5,     r12,  r6,  r5     @@// Q_filter
        SMLAWB        r4,     r14,  r6,  r4     @@// G_filter
        
        ldr                     r6,     [ADJ_SMOOTH_tab_ptr],  #4                 @@// tab        
        ldr                     r12,   [r0,  #2*5*49*4]   @@//   Q
        ldr                     r14,   [r0],  #49*4          @@//   G
        
        SMLAWT        r5,     r12,  r6,  r5     @@// Q_filter
        SMLAWT        r4,     r14,  r6,  r4     @@// G_filter
        
        ldr                     r12,   [r0,  #2*5*49*4]   @@//   Q
        ldr                     r14,   [r0],  #49*4           @@//   G        
        SMLAWB        r5,     r12,  r6,  r5     @@// Q_filter
        SMLAWB        r4,     r14,  r6,  r4     @@// G_filter
        
        ldr                     r6,     [ADJ_SMOOTH_tab_ptr],  #4                 @@// tab        
        ldr                     r12,   [r0,  #2*5*49*4]   @@//   Q
        ldr                     r14,   [r0],  #49*4           @@//   G
        ldr                     S_M,    [ADJ_tmp_ptr, #4]
        SMLAWT        r5,     r12,  r6,  r5     @@// Q_filter
        SMLAWT        r4,     r14,  r6,  r4     @@// G_filter
        

        movs                 S_M,    S_M,    lsl    #12 
        andeqs              ADJ_ri,   ADJ_ri,   #0xFF
        
        beq                 AAC_DecHfAssemblySmoothFilterAsm_BRANCH
        @@/*if (S_M_boost != 0 || no_noise)*/
        ldr                r2,   [ADJ_tmp_ptr,  #0]
        ldr                r6,   [r3]
        SMULWB   r14,  S_M,     r2  @@// im   sum1
        SMULWT   r12,   S_M,    r2  @@// rel   sum0
        SMULL       r0,      r6,   ADJ_G_filt,       r6
        mul               r14,    ADJ_rev,   r14     
        add              r12,   r12,   r0,  lsr     #16
        ldr               r0,    [r3,  #4]
        add              r12,   r12,   r6,  lsl     #16             
        SMULL       r0,      r6,   ADJ_G_filt,       r0
        str                r12,    [r3],  #4        
        add              r14,   r14,   r0,  lsr     #16
        add              r14,   r14,   r6,  lsl     #16
        str                r14,    [r3],  #4               
        ldmfd   sp!, {r4- r8,  pc}
        @ENDFUNC     	
AAC_DecHfAssemblySmoothFilterAsm_BRANCH:
        ldr               ADJ_tmp_ptr,   [ADJ_tmp_ptr,  #2*4]
        ldr               r0,       [r3]
        mov            ADJ_Q_filt,  ADJ_Q_filt,   asr  #3
        SMULWT  r12,     ADJ_Q_filt,     ADJ_tmp_ptr  @@// sum0
        SMULWB  r14,     ADJ_Q_filt,     ADJ_tmp_ptr  @@// sum1        
        
        SMULL       r6,      r0,   ADJ_G_filt,       r0
        ldr               ADJ_Q_filt,    [r3,  #4]
        add              r12,   r12,   r0,  lsl     #16        
        add              r12,   r12,   r6,  lsr     #16                              
        SMULL       r6,      r0,   ADJ_G_filt,       ADJ_Q_filt
        str                r12,   [r3],  #4
        add              r14,   r14,   r6,  lsr     #16
        add              r14,   r14,   r0,  lsl     #16
        str                r14,    [r3],  #4   
        ldmfd   sp!, {r4- r8,  pc}
        @ENDFUNC     	
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
    
    	                
        .end@END
        