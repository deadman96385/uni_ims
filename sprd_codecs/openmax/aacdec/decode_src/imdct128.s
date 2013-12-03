@// AAC-LC synthesis filter bank
@// author: reed.zhang
@// input: SBR synthesis filter bank processing
@//	r0: output sample pointer
@// r1: the input data
@// purpose:

@// define the register name 
in_ptr0     .req      r0  @@// input buffer
tmp_ptr     .req      r1

in_ptr1     .req      r2
c20         .req      r3
t20         .req      r4
t10         .req      r5
t21         .req      r6
t11         .req      r7
t30         .req      r8
t41         .req      r9
t31         .req      r10
t40         .req      r11

tab_ptr0    .req      r12
k		      	.req   		r14

ch1_ptr1    .req      r2
tar4_ptr    .req      r12
c21         .req      r12



IFFT_step4_in_ptr0        .req  r0
IFFT_step4_in_ptr16       .req  r2
tt10                      .req  r9
tt11                      .req  r10
tt20                      .req  r3
tt21                      .req  r4
tt30                      .req  r5
tt31                      .req  r8
tt40                      .req  r6
tt41                      .req  r7


out_ptr00                 .req  r1
out_ptr32                 .req  r2
out_ptr64                 .req  r3
out_ptr96                 .req  r4

		.text@AREA	PROGRAM, CODE, READONLY				
		.arm@CODE32				
		.global	AAC_DEC_Imdct128Asm
		@@ // r0, the in/output data
		@@ // the input data fixed-point is S18.13
		@@ // and the output data fixed-point is S23.9
		@@ // r1, the tmp buffer for calculating IMDCT
AAC_DEC_Imdct128Asm:
		@ // save the value for the following calculation
		stmfd 	sp!, {r2 - r12, r14}	
		
		@@// the input data fixed point is S19.13
		@@// and the after this model the data precision is S22.10
		mov     k, #16
		add     in_ptr1, in_ptr0, #256
		ldr     tab_ptr0, =FIRST_STEP_TAB
FIRST_STEP:
		@@// LOOP CONTENT
		ldmia   in_ptr0, {t30, t41}
		ldmia   in_ptr1, {t31, t40}		
		add     t20, t30, t31
		sub     t10, t30, t31		
		add     t21, t41, t40
		sub     t11, t41, t40		
		ldr     t31, [in_ptr0, #128]
		ldr     t40, [in_ptr1, #128]		
		ldr     c20, [in_ptr1, #132]		
		add     t30, t31, t40
		sub     t41, t31, t40		
		ldr     t40, [in_ptr0, #132]
		add     t31, c20, t40
		sub     t40, c20, t40		
		add     c20, t10, t40
		sub     t10, t10, t40 @@// t10 -> c40
		
		add     t40, t11, t41 @@// t40 -> c21
		sub     t11, t11, t41 @@// t11 -> c41
		
		add     t41, t20, t30
		sub     t20, t20, t30 @@// t20 -> c30
		
		sub     t30, t21, t31 @@// t30 -> c31
		add     t31, t21, t31 
		
        mov     t41,    t41,   lsl #4 @@//
        mov     t31,    t31,   lsl #4 @@//
        
		stmia   in_ptr0!, {t41, t31}
		@@/////////
		ldr     t21, [tab_ptr0], #4		
		mov     t40,  t40,  lsl #5
		mov     c20,  c20,  lsl #5
		SMULWT  t41, t40, t21
		SMULWT  t31, c20, t21		
		rsb     t40, t40, #0
		SMLAWB  t41, c20, t21, t41
		SMLAWB  t31, t40, t21, t31			
		str     t41, [in_ptr0, #124]
		str     t31, [in_ptr0, #120]		
		@@/////////
		ldmia   tab_ptr0!, {t21, t40}
		
		mov     t20,  t20,  lsl #5
		mov     t30,  t30,  lsl #5
		SMULWT  t31, t30, t21
		SMULWT  t41, t20, t21
		rsb     t30, t30, #0
		SMLAWB  t31, t20, t21, t31
		SMLAWB  t41, t30, t21, t41
		stmia   in_ptr1!, {t41, t31}		
		mov     t11,  t11,  lsl #5
		mov     t10,  t10,  lsl #5
		SMULWT  t41, t11, t40
		SMULWT  t31, t10, t40		
		rsb     t11, t11, #0
		SMLAWB  t41, t10, t40, t41
		SMLAWB  t31, t11, t40, t31
		str     t41, [in_ptr1, #124]
		str     t31, [in_ptr1, #120]				
		@@// control
		subs    k, k, #1		
		BGT     FIRST_STEP
		
		
		@@// END OF FIRST_STEP
		sub in_ptr0,in_ptr0, #128 @@// return to the start postion
		
		        ldr     r12,   =0x764230FC  @@// 30274, 12540		                        
                ldr     r11,  =-19195                
                add     IFFT_step4_in_ptr16,  IFFT_step4_in_ptr0,  #16*4

                mov     r14,  #4
AAC_DEC_ARM_ShortIfftStep4AsmLOOP:
                @@//////////////////////////////////////////////////
                @/* the free register is: r3, r4, r5, r6, r7, r8, r9, r10*/@
                @@@///////////////////////////////////////
                @@@/* first */
                ldmia   IFFT_step4_in_ptr0,  {r3, r4}
                ldmia   IFFT_step4_in_ptr16, {r9, r10}
                ldr     r5,  [IFFT_step4_in_ptr0, #8*4]
                ldr     r6,  [IFFT_step4_in_ptr0, #9*4]                
                ldr     r7,  [IFFT_step4_in_ptr16, #8*4]
                ldr     r8,  [IFFT_step4_in_ptr16, #9*4]                
                
                add     r5,  r5,  r7           @//  t30: r5
                sub     r7,  r5,  r7, lsl #1   @//  t41: r7
                
                add     r8,  r8,  r6           @//  t31: r8
                sub     r6,  r8,  r6, lsl #1   @//  t40: r6
                
                add     r3,  r3,  r9           @//  t20: r3
                sub     r9,  r3,  r9, lsl #1   @//  t10: r9
                
                add     r4,  r4,  r10          @//  t21: r4
                sub     r10, r4,  r10,lsl #1   @//  t11: r10                
                
                add     r9,    tt10,   tt40           @@// r9: c20
                sub     tt40,  tt10,   tt40,  lsl #1                 
                add     r10,  tt11,   tt41            @@// r10: c21
                sub     tt41, tt11,   tt41,  lsl #1                  

                str     r9,   [IFFT_step4_in_ptr0, #8*4]
                str     r10,  [IFFT_step4_in_ptr0, #9*4]                
                
                str     tt40,   [IFFT_step4_in_ptr16, #8*4]
                str     tt41,   [IFFT_step4_in_ptr16, #9*4]                
                
                add     tt20,  tt20,  tt30            @@// sum1, r3
                sub     tt30,  tt20,  tt30, lsl #1
                
                add     tt21,  tt21,  tt31            @//  sum2. r4
                sub     tt31,  tt21,  tt31, lsl #1
                
                stmia   IFFT_step4_in_ptr0!, {tt20,   tt21}                
                stmia   IFFT_step4_in_ptr16!,  {r5,  r8}                
                @@@///////////////////////////////////////
                @@@/* second */
                ldmia   IFFT_step4_in_ptr0,  {r3, r4}
                ldmia   IFFT_step4_in_ptr16, {r9, r10}
                ldr     r5,  [IFFT_step4_in_ptr0, #8*4]
                ldr     r6,  [IFFT_step4_in_ptr0, #9*4]                
                ldr     r7,  [IFFT_step4_in_ptr16, #8*4]
                ldr     r8,  [IFFT_step4_in_ptr16, #9*4]                
                add     r5,  r5,  r7           @//  t30: r5
                sub     r7,  r5,  r7, lsl #1   @//  t41: r7                
                add     r8,  r8,  r6           @//  t31: r8
                sub     r6,  r8,  r6, lsl #1   @//  t40: r6                
                add     r3,  r3,  r9           @//  t20: r3
                sub     r9,  r3,  r9, lsl #1   @//  t10: r9                
                add     r4,  r4,  r10          @//  t21: r4
                sub     r10, r4,  r10,lsl #1   @//  t11: r10                  
                add     r9,    tt10,   tt40           @@// c20: r9
                sub     tt40,  tt10,   tt40, lsl #1   @@// t40: r6
                add     r10,  tt11,   tt41            @@// c21: r10
                sub     tt41, tt11,   tt41,  lsl #1   @@// t41: r7
                
                add     tt20,  tt20,  tt30            @@// sum1, r3
                sub     tt30,  tt20,  tt30, lsl #1    @@// t30:  r5
                
                add     tt21,  tt21,  tt31            @@// sum2. r4
                sub     tt31,  tt21,  tt31, lsl #1    @@// t31:  r8
                
                stmia   IFFT_step4_in_ptr0!, {tt20,   tt21}                 
                add     tt30, tt30,  tt31
                sub     r4,   tt30,  tt31, lsl #1                
                SMLAWB  tt30, tt30,  r11,  tt30
                SMLAWB  r4,   r4,    r11,  r4                
                mov     r10,r10, lsl #1   @@// d0
                stmia   IFFT_step4_in_ptr16!, {r4, r5}           
                mov     r9, r9,  lsl #1   @@// d1
                @//buufferFly(c21<<1, c20<<1, 1984016128 >> TEST_BIT_SHIFT, 821806464 >> TEST_BIT_SHIFT, &tmp2, &tmp1)@
                SMULWT  r8,  r10, r12    @// d0 * sin
                SMULWT  r4,  r9,  r12    @// d1 * sin 
                rsb     r10, r10,  #0                
                SMLAWB  r4,  r10,  r12, r4  @// -d0 * cos
                SMLAWB  r8,  r9,   r12, r8  @//  d1 * cos                                
                mov     r7,  r7,  lsl #1   @@// d0
                str     r4,  [IFFT_step4_in_ptr0, #6*4]
                str     r8,  [IFFT_step4_in_ptr0, #7*4]
                mov     r6,  r6,  lsl #1    @@// d1
                SMULWB  r8,  r7, r12    @// d0 * sin
                SMULWB  r4,  r6, r12    @// d1 * sin 
                rsb     r7, r7,  #0                
                SMLAWT  r4,  r7,   r12, r4  @// -d0 * cos
                SMLAWT  r8,  r6,   r12, r8  @//  d1 * cos                
                str     r4,  [IFFT_step4_in_ptr16, #6*4]
                str     r8,  [IFFT_step4_in_ptr16, #7*4]
                @@@/* third */
                ldmia   IFFT_step4_in_ptr0,  {r3, r4}
                ldmia   IFFT_step4_in_ptr16, {r9, r10}
                ldr     r5,  [IFFT_step4_in_ptr0, #8*4]
                ldr     r6,  [IFFT_step4_in_ptr0, #9*4]                
                ldr     r7,  [IFFT_step4_in_ptr16, #8*4]
                ldr     r8,  [IFFT_step4_in_ptr16, #9*4]                
                add     r5,  r5,  r7           @//  t30: r5
                sub     r7,  r5,  r7, lsl #1   @//  t41: r7                
                add     r8,  r8,  r6           @//  t31: r8
                sub     r6,  r8,  r6, lsl #1   @//  t40: r6                
                add     r3,  r3,  r9           @//  t20: r3
                sub     r9,  r3,  r9, lsl #1   @//  t10: r9                
                add     r4,  r4,  r10          @//  t21: r4
                sub     r10, r4,  r10,lsl #1   @//  t11: r10                 
                add     r9,    tt10,   tt40           @@// r9: c20
                sub     tt40,  tt10,   tt40,  lsl #1                 
                add     r10,  tt11,   tt41            @@// r10: c21
                sub     tt41, tt11,   tt41,  lsl #1                         
                add     tt20,  tt20,  tt30            @@// sum1, r3
                sub     tt30,  tt20,  tt30, lsl #1
                add     tt21,  tt21,  tt31            @//  sum2. r4
                sub     tt31,  tt21,  tt31, lsl #1
                stmia   IFFT_step4_in_ptr0!, {tt20,   tt21}                
                rsb     r8,   r8,  #0
                str     r8,    [IFFT_step4_in_ptr16], #4
                str     r5,    [IFFT_step4_in_ptr16], #4                
                add     r3,    r9,  r10
                sub     r4,    r9,  r10                
                SMLAWB  r3,    r3,  r11,  r3
                SMLAWB  r4,    r4,  r11,  r4                
                str     r4,    [IFFT_step4_in_ptr0, #6*4]
                str     r3,    [IFFT_step4_in_ptr0, #7*4]                
                sub     r3,    r6,  r7
                sub     r4,    r3,  r6, lsl #1                
                SMLAWB  r3,    r3,  r11,  r3
                SMLAWB  r4,    r4,  r11,  r4                
                str     r4,    [IFFT_step4_in_ptr16, #6*4]
                str     r3,    [IFFT_step4_in_ptr16, #7*4]                
                @@@///////////////////////////////////////
                @@@/* fourth */
                ldmia   IFFT_step4_in_ptr0,  {r3, r4}
                ldmia   IFFT_step4_in_ptr16, {r9, r10}
                ldr     r5,  [IFFT_step4_in_ptr0, #8*4]
                ldr     r6,  [IFFT_step4_in_ptr0, #9*4]                
                ldr     r7,  [IFFT_step4_in_ptr16, #8*4]
                ldr     r8,  [IFFT_step4_in_ptr16, #9*4]                
                add     r5,  r5,  r7           @//  t30: r5
                sub     r7,  r5,  r7, lsl #1   @//  t41: r7                
                add     r8,  r8,  r6           @//  t31: r8
                sub     r6,  r8,  r6, lsl #1   @//  t40: r6                
                add     r3,  r3,  r9           @//  t20: r3
                sub     r9,  r3,  r9, lsl #1   @//  t10: r9                
                add     r4,  r4,  r10          @//  t21: r4
                sub     r10, r4,  r10,lsl #1   @//  t11: r10                 
                add     r9,    tt10,   tt40           @@// c20: r9
                sub     tt40,  tt10,   tt40, lsl #1   @@// t40: r6
                add     r10,  tt11,   tt41            @@// c21: r10
                sub     tt41, tt11,   tt41,  lsl #1   @@// t41: r7                
                add     tt20,  tt20,  tt30            @@// sum1, r3
                sub     tt30,  tt20,  tt30, lsl #1    @@// t30:  r5                
                add     tt21,  tt21,  tt31            @@// sum2. r4
                sub     tt31,  tt21,  tt31, lsl #1    @@// t31:  r8                
                stmia   IFFT_step4_in_ptr0!, {tt20,   tt21}                       
                sub     tt31,   tt30,  tt31
                sub     tt30,   tt31,  tt30,  lsl #1                
                SMLAWB  tt30,   tt30,  r11,  tt30
                SMLAWB  tt31,   tt31,  r11,  tt31                
                mov     r10,r10, lsl #1   @@// d0
                stmia   IFFT_step4_in_ptr16!, {tt30,   tt31}           
                @//str     tt31,  [IFFT_step4_in_ptr16], #4
                @//str     tt30,  [IFFT_step4_in_ptr16], #4
                mov     r9, r9,  lsl #1   @@// d1
                @//buufferFly(c21<<1, c20<<1, 1984016128 >> TEST_BIT_SHIFT, 821806464 >> TEST_BIT_SHIFT, &tmp2, &tmp1)@
                SMULWB  r8,  r10, r12    @// d0 * sin
                SMULWB  r4,  r9,  r12    @// d1 * sin 
                rsb     r10, r10,  #0                
                SMLAWT  r4,  r10,  r12, r4  @// -d0 * cos
                SMLAWT  r8,  r9,   r12, r8  @//  d1 * cos                
                mvn     r7,  r7,  lsl #1   @@// d0                
                str     r4,  [IFFT_step4_in_ptr0, #6*4]
                str     r8,  [IFFT_step4_in_ptr0, #7*4]                
                mvn     r6,  r6,  lsl #1   @@// d1
                SMULWT  r8,  r7, r12    @// d0 * sin
                SMULWT  r4,  r6, r12    @// d1 * sin 
                rsb     r7, r7,  #0                
                SMLAWB  r4,  r7,   r12, r4  @// -d0 * cos
                SMLAWB  r8,  r6,   r12, r8  @//  d1 * cos                
                str     r4,  [IFFT_step4_in_ptr16, #6*4]
                str     r8,  [IFFT_step4_in_ptr16, #7*4]                
                add   IFFT_step4_in_ptr0, IFFT_step4_in_ptr0, #24*4 
                add   IFFT_step4_in_ptr16, IFFT_step4_in_ptr16, #24*4
                @@//////////////////////////////////////////////////
                subs   r14,  r14,  #1
                BGT    AAC_DEC_ARM_ShortIfftStep4AsmLOOP
		
		
		
		
		
		
		sub in_ptr0,in_ptr0, #128*4 @@// return to the start postion
		add out_ptr32,  out_ptr00,   #32*4
		add out_ptr64,  out_ptr32,   #32*4
		add out_ptr96,  out_ptr64,   #32*4
		
		mov   r14,  #4
AAC_DEC_SHORT_STEP3LOOP:
        @@////////////////////////////
        @@/*1*/
        ldmia  		in_ptr0, {r5-r12}
        @//RE(cc[0]): r5. IM(cc[0]): r6
        @//RE(cc[1]): r7. IM(cc[1]): r8
        @//RE(cc[2]): r9. IM(cc[2]): r10
        @//RE(cc[3]): r11.IM(cc[3]): r12        
		add  r7,   r7,   r11           @@// r7:  t30
		sub  r11,  r7,   r11, lsl #1   @@// r11: t41
		
		add  r12,  r12,   r8           @@// r12: t31
		sub  r8,   r12,   r8, lsl #1   @@// r8:  t40
		
		add  r5 ,  r5,    r9           @@// r5:  t20
		sub  r9,   r5,    r9, lsl #1   @@// r9:  t10
		
		add  r6 ,  r6,    r10          @@// r6:  t21
		sub  r10,  r6,    r10, lsl #1  @@// r10: t11
		@////////////////////////////
		
		add  r5,  r5, r7               @@// RE(ch[k])
		sub  r7,  r5, r7, lsl #1       @@// RE(ch[k+2*16])
		
		add  r9,  r9, r8               @@// RE(ch[k+16])
		sub  r8,  r9, r8,  lsl #1      @@// RE(ch[k+3*16])
		
		add  r6,  r6, r12              @@// IM(ch[k]) 
		sub  r12, r6, r12, lsl #1      @@// IM(ch[k+2*16])
		
		add  r10,  r10, r11            @@// IM(ch[k+16])
		sub  r11,  r10, r11,  lsl #1   @@// IM(ch[k+3*16])
				
		stmia out_ptr00!, {r5,r6}	
		stmia out_ptr32!, {r9,r10}		
		stmia out_ptr64!, {r7,r12}		
		stmia out_ptr96!, {r8,r11}
		
		add   in_ptr0, in_ptr0, #32*4
		
		@@/*1*/
        ldmia  		in_ptr0, {r5-r12}
        @//RE(cc[0]): r5. IM(cc[0]): r6
        @//RE(cc[1]): r7. IM(cc[1]): r8
        @//RE(cc[2]): r9. IM(cc[2]): r10
        @//RE(cc[3]): r11.IM(cc[3]): r12        
		add  r7,   r7,   r11           @@// r7:  t30
		sub  r11,  r7,   r11, lsl #1   @@// r11: t41
		
		add  r12,  r12,   r8           @@// r12: t31
		sub  r8,   r12,   r8, lsl #1   @@// r8:  t40
		
		add  r5 ,  r5,    r9           @@// r5:  t20
		sub  r9,   r5,    r9, lsl #1   @@// r9:  t10
		
		add  r6 ,  r6,    r10          @@// r6:  t21
		sub  r10,  r6,    r10, lsl #1  @@// r10: t11
		@////////////////////////////
		
		add  r5,  r5, r7               @@// RE(ch[k])
		sub  r7,  r5, r7, lsl #1       @@// RE(ch[k+2*16])
		
		add  r9,  r9, r8               @@// RE(ch[k+16])
		sub  r8,  r9, r8,  lsl #1      @@// RE(ch[k+3*16])
		
		add  r6,  r6, r12              @@// IM(ch[k]) 
		sub  r12, r6, r12, lsl #1      @@// IM(ch[k+2*16])
		
		add  r10,  r10, r11            @@// IM(ch[k+16])
		sub  r11,  r10, r11,  lsl #1   @@// IM(ch[k+3*16])
				
		stmia out_ptr00!, {r5,r6}	
		stmia out_ptr32!, {r9,r10}		
		stmia out_ptr64!, {r7,r12}		
		stmia out_ptr96!, {r8,r11}
		
		add   in_ptr0, in_ptr0, #32*4
		
		
		@@/*1*/
        ldmia  		in_ptr0, {r5-r12}
        @//RE(cc[0]): r5. IM(cc[0]): r6
        @//RE(cc[1]): r7. IM(cc[1]): r8
        @//RE(cc[2]): r9. IM(cc[2]): r10
        @//RE(cc[3]): r11.IM(cc[3]): r12        
		add  r7,   r7,   r11           @@// r7:  t30
		sub  r11,  r7,   r11, lsl #1   @@// r11: t41
		
		add  r12,  r12,   r8           @@// r12: t31
		sub  r8,   r12,   r8, lsl #1   @@// r8:  t40
		
		add  r5 ,  r5,    r9           @@// r5:  t20
		sub  r9,   r5,    r9, lsl #1   @@// r9:  t10
		
		add  r6 ,  r6,    r10          @@// r6:  t21
		sub  r10,  r6,    r10, lsl #1  @@// r10: t11
		@////////////////////////////
		
		add  r5,  r5, r7               @@// RE(ch[k])
		sub  r7,  r5, r7, lsl #1       @@// RE(ch[k+2*16])
		
		add  r9,  r9, r8               @@// RE(ch[k+16])
		sub  r8,  r9, r8,  lsl #1      @@// RE(ch[k+3*16])
		
		add  r6,  r6, r12              @@// IM(ch[k]) 
		sub  r12, r6, r12, lsl #1      @@// IM(ch[k+2*16])
		
		add  r10,  r10, r11            @@// IM(ch[k+16])
		sub  r11,  r10, r11,  lsl #1   @@// IM(ch[k+3*16])
				
		stmia out_ptr00!, {r5,r6}	
		stmia out_ptr32!, {r9,r10}		
		stmia out_ptr64!, {r7,r12}		
		stmia out_ptr96!, {r8,r11}
		
		add   in_ptr0, in_ptr0, #32*4
		
		@@/*1*/
        ldmia  		in_ptr0, {r5-r12}
        @//RE(cc[0]): r5. IM(cc[0]): r6
        @//RE(cc[1]): r7. IM(cc[1]): r8
        @//RE(cc[2]): r9. IM(cc[2]): r10
        @//RE(cc[3]): r11.IM(cc[3]): r12        
		add  r7,   r7,   r11           @@// r7:  t30
		sub  r11,  r7,   r11, lsl #1   @@// r11: t41
		
		add  r12,  r12,   r8           @@// r12: t31
		sub  r8,   r12,   r8, lsl #1   @@// r8:  t40
		
		add  r5 ,  r5,    r9           @@// r5:  t20
		sub  r9,   r5,    r9, lsl #1   @@// r9:  t10
		
		add  r6 ,  r6,    r10          @@// r6:  t21
		sub  r10,  r6,    r10, lsl #1  @@// r10: t11
		@////////////////////////////
		
		add  r5,  r5, r7               @@// RE(ch[k])
		sub  r7,  r5, r7, lsl #1       @@// RE(ch[k+2*16])
		
		add  r9,  r9, r8               @@// RE(ch[k+16])
		sub  r8,  r9, r8,  lsl #1      @@// RE(ch[k+3*16])
		
		add  r6,  r6, r12              @@// IM(ch[k]) 
		sub  r12, r6, r12, lsl #1      @@// IM(ch[k+2*16])
		
		add  r10,  r10, r11            @@// IM(ch[k+16])
		sub  r11,  r10, r11,  lsl #1   @@// IM(ch[k+3*16])
				
	
		stmia out_ptr00!, {r5,r6}
		stmia out_ptr32!, {r9,r10}		
		stmia out_ptr64!, {r7,r12}		
		stmia out_ptr96!, {r8,r11}
		
		sub   in_ptr0, in_ptr0, #88*4
		
		
		
		subs  r14,  r14,  #1		
		BGT  AAC_DEC_SHORT_STEP3LOOP
		
		
		
		
		
		
		
		
		
		@@// END OF SECOND_STEP
		

		@ // load the following value for quit the function
		ldmfd	sp!, {r2 - r12, pc} 

FIRST_STEP_TAB:
      .word   0x7fff0000, 0x7fff0000, 0x7fff0000    
      .word   0x7f620c8c, 0x7d8a18f9, 0x7a7d2528    
      .word   0x7d8a18f9, 0x764230fc, 0x6a6e471d    
      .word   0x7a7d2528, 0x6a6e471d, 0x513462f2    
      .word   0x764230fc, 0x5a825a82, 0x30fc7642    
      .word   0x70e33c57, 0x471d6a6e, 0x0c8c7f62    
      .word   0x6a6e471d, 0x30fc7642, 0xe7077d8a    
      .word   0x62f25134, 0x18f97d8a, 0xc3a970e3    
      .word   0x5a825a82, 0x00007fff, 0xa57e5a82    
      .word   0x513462f2, 0xe7077d8a, 0x8f1d3c57    
      .word   0x471d6a6e, 0xcf047642, 0x827618f9    
      .word   0x3c5770e3, 0xb8e36a6e, 0x809ef374    
      .word   0x30fc7642, 0xa57e5a82, 0x89becf04    
      .word   0x25287a7d, 0x9592471d, 0x9d0eaecc    
      .word   0x18f97d8a, 0x89be30fc, 0xb8e39592    
      .word   0x0c8c7f62, 0x827618f9, 0xdad88583    


					
		.end