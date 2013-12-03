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
	.extern  log2_tab
	.extern  pow2_tab
	.extern  AAC_ps_Q_fract_allpass_subQmf20
	 @ //void AAC_DECORR_CALCEnergCoefAsm(AAC_PS_INFO_T *ps_ptr,
	 @//                                                                        aac_complex   *X_left_ptr,
	 @//                                                                        aac_complex   *X_hybrid_peft_ptr)
map_group2bk_ptr       .req     r3
group_border_ptr       .req     r4
energ_ptr              .req     r12
X_hybrid_peft_ptr      .req      r2
coef_ptr               .req     r12
X_right_ptr            .req     r7
frac       .req r3
index      .req r4
indexFrac  .req r5
exp        .req r6
x1         .req r7
x2         .req r8
rest       .req r9


delay_SubQmf_ptr       .req    r12
delay_SubQmf_ser_ptr   .req   r11
AAC_ps_Q_fract_allpass_subQmf20_ptr   .req    r10
delay_Qmf_nr_allpass_bandsLT22_ptr    .req    r11
delay_Qmf_ser_ptr                     .req    r8

AAC_DECORR_ReconstructAsm:   @FUNCTION
        .global	AAC_DECORR_ReconstructAsm	
        stmfd    sp!, {r4-r11, r14}
        sub       sp,  sp,  #9*4
        stmia    sp,   {r0,r1}   @@// store ps_ptr address -> sp[0] and X_left_ptr address -> sp[1]       
        str        X_hybrid_peft_ptr,   [sp,  #2*4] 
        @@////////////////////////////////
        @@/* step 1: calc the energe and the coeff */
        ldr        map_group2bk_ptr, [r0,  #24]
        ldr        group_border_ptr,  [r0, #28]
        mov      r14,   #10
        add      energ_ptr,    X_hybrid_peft_ptr, #256*4
        
        mov    r5,  #0
        mov    r6,  #0        
        stmia   energ_ptr!,  { r5-r6}
        stmia   energ_ptr!,  { r5-r6}
        stmia   energ_ptr!,  { r5-r6}
        stmia   energ_ptr!,  { r5-r6}
        stmia   energ_ptr!,  { r5-r6}
        sub     energ_ptr,  energ_ptr,  #10*4 
        
AAC_DECORR_CALCEnergCoefAsm_LOOP1:
        ldrh    r6,    [group_border_ptr],  #2      @@// sb
        ldrh    r5,    [map_group2bk_ptr],  #2    @@// bk
        
        add     r6,    X_hybrid_peft_ptr,    r6,  lsl   #3
        ldmia  r6,  {r6-r7}         
        SMULL    r8,  r9,  r6,  r6
        ldr             r6,    [energ_ptr, r5,  lsl  #2]  @ @// energe 
        SMLAL    r8,  r9,  r7,  r7
        
        subs          r14,  r14,  #1
        add           r6,    r6,  r8,  lsr   #20
        add           r6,    r6,  r9,  lsl   #12
        str             r6,    [energ_ptr, r5,  lsl  #2]           
        BGT     AAC_DECORR_CALCEnergCoefAsm_LOOP1
        
        @/////////////////////////////////////////////////////////
        
        add        energ_ptr,  energ_ptr,   #8*4
        add        X_hybrid_peft_ptr,  r1,   #3*8
        ldrh         r1,      [r0,  #4]
        mov       r14,   #11                        
AAC_DECORR_CALCEnergCoefAsm_LOOP2:
        ldrh       r7,   [group_border_ptr,  #2]       @@// sb+1  
        ldrh       r6,    [group_border_ptr],  #2      @@// sb
        cmp       r7,    r1
        movgt     r7,    r1
        subs      r6,  r7, r6
        BLE        AAC_DECORR_CALCEnerg_SKIP
        mov      r7,  #0
        mov      r10,  #0
AAC_DECORR_CALCEnergCoefAsm_LOOP2_INTER:
        ldmia         X_hybrid_peft_ptr!,  {r8, r9}
        SMLAL    r7,  r10,  r8,  r8
        SMLAL    r7,  r10,  r9,  r9         
        subs      r6,  r6,  #1
        BGT     AAC_DECORR_CALCEnergCoefAsm_LOOP2_INTER
        
        mov          r6,    r7,  lsr   #20
        add           r6,    r6,  r10,  lsl   #12
        str             r6,    [energ_ptr]  ,  #4
        subs          r14,  r14,  #1
        BGT     AAC_DECORR_CALCEnergCoefAsm_LOOP2
        @@@/////////////////////////////////////////////////////////
        subs         r1,    r1,   #35
        BLE        AAC_DECORR_CALCEnerg_SKIP
        
        mov      r7,  #0
        mov      r10,  #0
AAC_DECORR_CALCEnergCoefAsm_LOOP3:
        ldmia         X_hybrid_peft_ptr!,  {r8, r9}
        SMLAL    r7,  r10,  r8,  r8
        SMLAL    r7,  r10,  r9,  r9          
        subs      r1,  r1,  #1
        BGT     AAC_DECORR_CALCEnergCoefAsm_LOOP3
        mov          r6,    r7,  lsr   #20
        add           r6,    r6,  r10,  lsl   #12
        str             r6,    [energ_ptr] ,  #-19*4        
AAC_DECORR_CALCEnerg_SKIP:
        ldr        energ_ptr,   [sp,  #2*4]
        @@/* calc the coef */        
        mov        r14,   #20
        add         r11,    r0,   #32
        ldr         r1,   = log2_tab
        ldr         r2,   = pow2_tab
       add        energ_ptr,  energ_ptr,   #256*4
        @@/* r14, r12 are used and others are rest */
AAC_DECORR_CALCCOEF_LOOP:
        ldr          r7,        [r11]                @@// P_PeakDecayNrg = ps_ptr->P_PeakDecayNrg[bk]@
        ldr          r10,      [r11,  #40*4]   @@// P_SmoothPeakDecayDiffNrg = ps_ptr->P_SmoothPeakDecayDiffNrg_prev[bk]@
        ldr          r9,       [energ_ptr], #4  @@// P     = tP_eng_ptr[bk]@
        
        ldr      r6, = 17428
        mov      r8,  r7,  asr  #1
        SMLAWB   r8,     r7,   r6,  r8         @@// P_PeakDecayNrg = AAC_MULSHIFT32(P_PeakDecayNrg << 1, 1644818582)@
        ldr       r7,       [r11,  #20*4]     @@// nrg  = ps_ptr->P_prev[bk]@
        cmp       r8,       r9
        movlt     r8,       r9
        
        sub       r6,      r8,     r9
        mov      r10,    r10,   asr   #1
        add       r10,   r10,   r10,  asr  #1
        add       r10,   r10,   r6,    asr  #2  @@// P_SmoothPeakDecayDiffNrg =   (P_SmoothPeakDecayDiffNrg >> 1) + (P_SmoothPeakDecayDiffNrg >> 2) + ((P_PeakDecayNrg - P) >> 2)@
        
        mov       r7,    r7,   asr   #1
        add       r7,   r7,   r7,  asr  #1
        add       r0,   r7,   r9,  asr  #2   @@// nrg = (nrg >> 1) + (nrg >> 2) + (P >> 2)@
        
        str        r0,       [r11,  #20*4]     @@// ps_ptr->P_prev[bk]   = nrg@
        str        r10,      [r11,  #40*4]     @@// ps_ptr->P_SmoothPeakDecayDiffNrg_prev[bk] = P_SmoothPeakDecayDiffNrg@
        str        r8,        [r11],  #4           @@// ps_ptr->P_PeakDecayNrg[bk]   = P_PeakDecayNrg@
        
        add      r10,     r10,  r10,  asr  #1
        cmp     r10,     r0
        @@// if (P <= nrg)
        BGT    AAC_DECORR_CALCCOEF_LOOP_NEXT_BRANCH
        mov     r0,  #16384
        str        r0,  [energ_ptr,  #(32*4-4)]
        subs      r14,  r14,  #1
        BGT     AAC_DECORR_CALCCOEF_LOOP  
        B          AAC_DECORR_CALCEnergCoefAsmExit
AAC_DECORR_CALCCOEF_LOOP_NEXT_BRANCH:
        @@// if (P > nrg)
        
        @@// log2(nrg)                     
                clz       exp,    r0    @@// 
                rsbs    exp,  exp, #17 @@// 
                movge   frac,  r0, asr exp
                rsblt   r9,  exp,  #0
                movlt   frac,  r0, lsl r9   @@// r3 -> frac                
                mov     index, frac, asr #8
                and     indexFrac, frac, #0xFF                
                and     index, index, #0x3F
                add     index,  r1, index, lsl #1
                ldrsh   x1, [index], #2     
                ldrsh   x2, [index]
                add     exp, exp, #14
                sub     x2, x2, x1
                mul     x2, indexFrac, x2
                add     x1, x1, x2, asr #8
                add     r0,     x1, exp , lsl #14
                           
         @@// log2(P)                     
                clz       exp,    r10    @@// 
                rsbs    exp,  exp, #17 @@// 
                movge   frac,  r10, asr exp
                rsblt   r9,  exp,  #0
                movlt   frac,  r10, lsl r9   @@// r3 -> frac                
                mov     index, frac, asr #8
                and     indexFrac, frac, #0xFF                
                and     index, index, #0x3F
                add     index,  r1, index, lsl #1
                ldrsh   x1, [index], #2     
                ldrsh   x2, [index]
                add     exp, exp, #14
                sub     x2, x2, x1
                mul     x2, indexFrac, x2
                add     x1, x1, x2, asr #8
                add     r10,     x1, exp , lsl #14
    	sub    r0,     r0,  r10
     @@//    (AAC_FixedPowerInt(t))@
                mov     exp,  r0,  asr #14
                sub     rest,   r0,  exp, lsl #14                
                mov     index,rest, asr  #8                
                and     indexFrac, rest, #0xFF
                and     index,  index, #0xFF
                add     index,   r2,  index, lsl #1
                ldrsh   x1, [index], #2
                ldrsh   x2, [index]
               cmp     exp, #0
                sub     x2,  x2,  x1                
                mul     indexFrac, x2, indexFrac
                add     x1, x1, indexFrac, asr #8                
                mov     r0,  x1, lsl exp
                bge     AAC_DECORR_CALCCOEF_Pow2IntOut14bitAsmExit
                rsb       exp, exp, #0
                cmp     exp, #31
                movgt   exp, #31
                mov     r0,  x1, asr exp                 
AAC_DECORR_CALCCOEF_Pow2IntOut14bitAsmExit:
        str        r0,  [energ_ptr,  #(32*4-4)]	        
        subs      r14,  r14,  #1
        BGT     AAC_DECORR_CALCCOEF_LOOP
AAC_DECORR_CALCEnergCoefAsmExit:

@//delay_SubQmf_ptr       .req    r12
@//delay_SubQmf_ser_ptr .req   r11
@//AAC_ps_Q_fract_allpass_subQmf20_ptr   .req    r10
@//map_group2bk_ptr       .req     r3
@//group_border_ptr         .req     r4
@//X_hybrid_peft_ptr        .req      r2

        @@/* step 2:  hybrid data reconstruct */
        @@/* energ_ptr -> r12 */
@//        sub       X_hybrid_peft_ptr,    energ_ptr,   #(20+128+64*2)*4  @@// r2
        ldr        X_hybrid_peft_ptr,   [sp,  #2*4]
        ldr        r0,   [sp,  #0]
        mov      r14,   #0x98000
        ldrb      r5,  [r0]  @@// saved_delay
        ldrb      r6,  [r0, #1]  @@// saved_delay
        ldrb      r7,  [r0, #2]  @@// saved_delay
        ldrb      r8,  [r0, #3]  @@// saved_delay
        
        ldr        map_group2bk_ptr, [r0,  #24]
        ldr        group_border_ptr,   [r0, #28]
        
        add      r14,  r14,   r6,  lsl   #8
        add      r7,  r7,   #5
        add      r14,  r14,   r7,  lsl   #4        
        add      r8,  r8,   #5*2
        add      r14,  r14,   r8
        
        add      delay_SubQmf_ptr,           r0,   #272
        add      delay_SubQmf_ser_ptr,    r0,   #272+192
        cmp     r5,  #0
        addne  delay_SubQmf_ptr,  delay_SubQmf_ptr,  #12*2*4
        
        ldrh      r9,  [group_border_ptr],  #2  @@// sb 
        ldr        AAC_ps_Q_fract_allpass_subQmf20_ptr,  =AAC_ps_Q_fract_allpass_subQmf20
        @@/* r14, r12,  r11, r10,  r4, r3, r2 are used */
        @@/* r0, r1, r5, r6, r7, r8, r9 are free*/
AAC_DECORR_ReconstructAsmStep1LOOP:
        add     r7,   X_hybrid_peft_ptr,   r9,  lsl   #3
        ldmia   r7,   {r0-r1}  @@// inputLeft
        add     r7,   delay_SubQmf_ptr,    r9,  lsl   #3
        ldmia   r7,   {r5-r6}  @@// tmp0  ->  r5, r6
        stmia   r7,   {r0-r1}  @@// inputLeft        
        @@//ComplexMult(&RE(tmp), &IM(tmp), RE(tmp0), IM(tmp0), RE(Phi_Fract), IM(Phi_Fract))@			
        ldr       r7,  [AAC_ps_Q_fract_allpass_subQmf20_ptr],#4  @@// Phi_Fract -> r7
        mov     r5,  r5,  lsl  #1
        mov     r6,  r6,  lsl  #1
        SMULWT    r0,   r5,   r7
        rsb                r5,   r5,   #0
        SMULWB    r1,   r5,   r7
        SMLAWB    r0,   r6,   r7,  r0
        SMLAWT    r1,   r6,   r7,  r1
        @@/* R0-> r0, r1*/
        @@/* r5, r6, r7, r8 are free */
        @@/* m= 0*/
        mov     r8,   r14,  lsr   #8
        and      r8,    r8,   #7
        add      r8,    delay_SubQmf_ser_ptr,   r8,  lsl  #7
        add      r8,    r8,   r9,  lsl  #3           
        ldr        r6,   [r8]
        ldr        r7,   [AAC_ps_Q_fract_allpass_subQmf20_ptr],  #4
        ldr        r2,   [r8, #4]
        mov     r6,  r6,  lsl  #1
        mov     r2,  r2,  lsl  #1
        SMULWT    r5,     r6,     r7
        rsb                r6,     r6,   #0
        SMULWB    r6,     r6,     r7        
        SMLAWB    r5,   r2,   r7,  r5
        SMLAWT    r6,   r2,   r7,  r6
        @@/*tmp -> r5, r6*/
        ldr                r7,    =-21346
        mov             r2,  r0,  lsl  #1
        SMLAWB  r5,    r2,  r7,   r5
        mov             r2,  r1,  lsl  #1
        SMLAWB  r6,    r2,  r7,   r6
        
        rsb              r7,  r7,  #0
        mov             r2,  r5,  lsl  #1
        SMLAWB  r0,    r2,  r7,   r0
        mov             r2,  r6,  lsl  #1
        SMLAWB  r1,    r2,  r7,   r1
        @@/* tmp2 -> r0, r1*/
        stmia          r8,  {r0-r1}
        @@/* R0 -> r5, r6*/
        @@/* r0, r1, r7, r8 are free */
        @@/* m= 1*/
        mov     r8,   r14,  lsr   #4
        and      r8,    r8,   #0xF
        add      r8,    delay_SubQmf_ser_ptr,   r8,  lsl  #7
        add      r8,    r8,   r9,  lsl  #3           
        ldr        r1,   [r8]
        ldr        r7,   [AAC_ps_Q_fract_allpass_subQmf20_ptr],  #4
        ldr        r2,   [r8, #4]
        mov     r1,  r1,  lsl  #1
        mov     r2,  r2,  lsl  #1
        SMULWT    r0,     r1,     r7
        rsb                r1,     r1,   #0
        SMULWB    r1,     r1,     r7        
        SMLAWB    r0,   r2,   r7,  r0
        SMLAWT    r1,   r2,   r7,  r1
        @@/*tmp -> r0, r1*/
        ldr                r7,    =-18505
        mov             r2,  r5,  lsl  #1
        SMLAWB  r0,    r2,  r7,   r0
        mov             r2,  r6,  lsl  #1
        SMLAWB  r1,    r2,  r7,   r1
        
        rsb              r7,  r7,  #0
        mov             r2,  r0,  lsl  #1
        SMLAWB  r5,    r2,  r7,   r5
        mov             r2,  r1,  lsl  #1
        SMLAWB  r6,    r2,  r7,   r6
        @@/* tmp2 -> r5, r6*/
        stmia          r8,  {r5-r6}
        @@/* R0-> r0, r1*/
        @@/* r5, r6, r7, r8 are free */
        @@/* m= 2*/
        and      r8,    r14,   #0xF
        add      r8,    delay_SubQmf_ser_ptr,   r8,  lsl  #7
        add      r8,    r8,   r9,  lsl  #3           
        ldr        r6,   [r8]
        ldr        r7,   [AAC_ps_Q_fract_allpass_subQmf20_ptr],  #4
        ldr        r2,   [r8, #4]
        mov     r6,  r6,  lsl  #1
        mov     r2,  r2,  lsl  #1
        SMULWT    r5,     r6,     r7
        rsb                r6,     r6,   #0
        SMULWB    r6,     r6,     r7        
        SMLAWB    r5,   r2,   r7,  r5
        SMLAWT    r6,   r2,   r7,  r6
        @@/*tmp -> r5, r6*/
        ldr                r7,    =-32083
        ldr        X_hybrid_peft_ptr,   [sp,  #2*4]
        SMLAWB  r5,    r0,  r7,   r5
        SMLAWB  r6,    r1,  r7,   r6        
        rsb              r7,  r7,  #0
        SMLAWB  r0,    r5,  r7,   r0
        SMLAWB  r1,    r6,  r7,   r1
        @@/* tmp2 -> r0, r1*/
        ldrh      r7,    [map_group2bk_ptr],  #2
        stmia    r8,  {r0-r1}
        @@/* R0-> r5, r6*/
        @@///////////////////////////////////////////////////
        add      r0,     X_hybrid_peft_ptr,   r7,  lsl   #2
        ldr        r0,     [r0,  #288*4]
        mov     r5,  r5,  lsl  #2
        mov     r6,  r6,  lsl  #2        
        SMULWB  r5,   r5,   r0
        SMULWB  r6,   r6,   r0
        add     r9,     X_hybrid_peft_ptr,  r9,  lsl  #3
        @//stmia   r9,    {r5-r6}      
        str        r5,   [r9,  #64*4]
        str        r6,   [r9,  #65*4]
        
        ldrh      r9,  [group_border_ptr],  #2  @@// sb 
        subs     r14,  r14,  #65536
        BGT    AAC_DECORR_ReconstructAsmStep1LOOP
        
        @@/* Step 3 */
@//delay_Qmf_nr_allpass_bandsLT22_ptr       .req    r1
@//delay_Qmf_ser_ptr                                     .req    r3
        @@//stmia    sp,   {r0,r1}   @@// store ps_ptr address -> sp[0] and X_left_ptr address -> sp[1]   
        ldr      r0,   [sp,  #0]
        ldr      r1,   [sp,  #1*4]
        ldrb    r6,  [r0]   @@// ps_ptr->saved_delay        
        add    X_right_ptr,    X_hybrid_peft_ptr,   #(128+6)*4
        add    energ_ptr, X_hybrid_peft_ptr,  #(288*4+8*4)
        add    X_hybrid_peft_ptr,   r1,  #3*8
        add    delay_Qmf_nr_allpass_bandsLT22_ptr,  r0,   #2384
        add    delay_Qmf_ser_ptr,   delay_Qmf_nr_allpass_bandsLT22_ptr,  #368
        add    delay_Qmf_nr_allpass_bandsLT22_ptr,  delay_Qmf_nr_allpass_bandsLT22_ptr,   #(3*8)
        cmp   r6,   #0
        addne  delay_Qmf_nr_allpass_bandsLT22_ptr,  delay_Qmf_nr_allpass_bandsLT22_ptr,  #(23*8)
        
        add   r6,    r6,  #1
        cmp  r6,  #2
        movge  r6,  #0
        strb    r6,  [r0]   @@// ps_ptr->saved_delay        
        
        add   r14,  r14,  #(10*65536)
        
@///map_group2bk_ptr       .req     r3
@//group_border_ptr         .req     r4
@//energ_ptr                      .req     r12
@//X_hybrid_peft_ptr        .req      r2

@//delay_SubQmf_ptr       .req    r12
@//delay_SubQmf_ser_ptr .req   r11
@//AAC_ps_Q_fract_allpass_subQmf20_ptr   .req    r10
@//delay_Qmf_nr_allpass_bandsLT22_ptr       .req    r11
@//delay_Qmf_ser_ptr                                     .req    r8
        
        @@/* r14, r12,  r11, r10,  r4, r3, r2 are used */
        @@/* r0, r1, r5, r6, r9 are free*/
        @@// coef_ptr->             r12, 
        @//                                r14,    
        @//X_hybrid_peft_ptr->r2,   
        @//delay_Qmf_nr_allpass_bandsLT22_ptr: r11,   
        @//r10, 
        @// r3,  
        @// r4, 
        @//delay_Qmf_ser_ptr       r8
        @//  X_right_ptr                 r7
AAC_DECORR_ReconstructAsmStep2LOOP:
        str     group_border_ptr,  [sp,   #4*4]
        ldrh    group_border_ptr,  [group_border_ptr]   @// border sb        
        @@/* r0, r1, r5, r6 are free,  r9 = sb*/
AAC_DECORR_ReconstructAsmStep2LOOP_INTER:
        @@///////////////////////////////////////////////////////
        rsbs                  r1,   r9,   #3
        ldr                    r5,    =1638*2
        mov                 r0,     #32768*2
        ldr                   r6,    = 0x53624849
        MLALT          r0,    r1,    r5,  r0  @@// g_DecaySlope       
        ldr                  r5,  =16041
        SMULWT      r1,     r0,        r6
        SMULWB      r6,     r0,        r6
        SMULWB      r0,     r0,        r5        
        add                r6,  r6, r1,   lsl   #16
        str                  r6,   [sp,  #6*4]       @////////////////////////////
        str                  r0,   [sp,  #7*4]       @////////////////////////////
        
        ldmia     X_hybrid_peft_ptr!,    {r5,  r6}
        ldr         r0,      [delay_Qmf_nr_allpass_bandsLT22_ptr]
        str         r5,      [delay_Qmf_nr_allpass_bandsLT22_ptr],   #4
        ldr        r1,       [delay_Qmf_nr_allpass_bandsLT22_ptr]
        str        r6,       [delay_Qmf_nr_allpass_bandsLT22_ptr],   #4        
        ldr        r5,      [AAC_ps_Q_fract_allpass_subQmf20_ptr],  #4  @@// Phi_Fract
        
        @@//ComplexMult(&RE(tmp), &IM(tmp), RE(tmp0), IM(tmp0), RE(Phi_Fract), IM(Phi_Fract))@			
        mov     r0,  r0,  lsl  #1
        mov     r1,  r1,  lsl  #1
        SMULWT    r6,   r0,   r5
        rsb                r0,   r0,   #0
        SMULWB    r0,   r0,   r5
        SMLAWB    r6,   r1,   r5,  r6
        SMLAWT    r0,   r1,   r5,  r0
        
        str      delay_Qmf_nr_allpass_bandsLT22_ptr,[sp,      #3*4]
        str      X_hybrid_peft_ptr,[sp,      #5*4]        
        @@/* r1, r5,  r11,    X_hybrid_peft_ptr are free */
        @@/* R0 -> r6, r0 */
        @@/* r1,  r5,  r3, r11,    X_hybrid_peft_ptr  are free*/
        @/* m = 0 */
        mov     r1,     r14,  lsr   #8
        and      r1,     r1,   #7
        add      r3,    delay_Qmf_ser_ptr,   r1,  lsl  #8
        add      r3,    r3,   r9,  lsl  #3           
        ldmia    r3,    { X_hybrid_peft_ptr,  r11 }
        ldr        r5,    [AAC_ps_Q_fract_allpass_subQmf20_ptr],  #4  @@// Q_Fract_allpass
        @@//ComplexMult(&RE(tmp), &IM(tmp), RE(tmp0), IM(tmp0), RE(Q_Fract_allpass), IM(Q_Fract_allpass))@
        mov    X_hybrid_peft_ptr,  X_hybrid_peft_ptr,   lsl  #1
        mov    r11,  r11,   lsl  #1        
        SMULWT    r1,   X_hybrid_peft_ptr,   r5
        rsb                X_hybrid_peft_ptr,   X_hybrid_peft_ptr,   #0
        SMULWB    X_hybrid_peft_ptr,   X_hybrid_peft_ptr,   r5
        SMLAWB    r1,   r11,   r5,  r1
        SMLAWT    X_hybrid_peft_ptr,   r11,   r5,  X_hybrid_peft_ptr
        @@// tmp -> r1,  X_hybrid_peft_ptr
        ldr              r5,    [sp,  #6*4]
        mvn            r11,  r6,  lsl #1
        SMLAWT r1,    r11,   r5,  r1
        mvn            r11,  r0,  lsl  #1
        SMLAWT X_hybrid_peft_ptr,    r11,   r5,  X_hybrid_peft_ptr
        
        mov            r11,  r1,  lsl #1
        SMLAWT r6,    r11,   r5,  r6
        mov            r11,  X_hybrid_peft_ptr,  lsl  #1
        SMLAWT r0,    r11,   r5,  r0        
        str         r6,   [r3], #4
        str         r0,   [r3], #4
        
        @@//////////////////
        @/* m = 1*/
        @/* R0 -> r1,  X_hybrid_peft_ptr */
        @/* r5,  r6,  r0,  r3,  r11 are free */
        mov     r0,     r14,  lsr   #4
        and      r0,     r0,   #0xF
        add      r3,    delay_Qmf_ser_ptr,   r0,  lsl  #8
        add      r3,    r3,   r9,  lsl  #3           
        ldmia    r3,    { r0,  r11 }
        ldr        r5,    [AAC_ps_Q_fract_allpass_subQmf20_ptr],  #4  @@// Q_Fract_allpass
        @@//ComplexMult(&RE(tmp), &IM(tmp), RE(tmp0), IM(tmp0), RE(Q_Fract_allpass), IM(Q_Fract_allpass))@
        mov    r0,  r0,   lsl  #1
        mov    r11,  r11,   lsl  #1        
        SMULWT    r6,   r0,   r5
        rsb                r0,   r0,   #0
        SMULWB    r0,   r0,   r5
        SMLAWB    r6,   r11,   r5,  r6
        SMLAWT    r0,   r11,   r5,  r0
        @@// tmp -> r6,  r0
        ldr               r5,    [sp,  #6*4]
        mvn             r11,  r1,  lsl #1
        SMLAWB  r6,    r11,   r5,  r6
        mvn             r11,  X_hybrid_peft_ptr,  lsl  #1
        SMLAWB  r0,    r11,   r5,  r0
        @@// tmp -> r6,  r0
        mov            r11,  r6,  lsl #1
        SMLAWB r1,    r11,   r5,  r1
        mov            r11,  r0,  lsl  #1
        SMLAWB  X_hybrid_peft_ptr,    r11,   r5,  X_hybrid_peft_ptr        
        str         r1,   [r3], #4
        str         X_hybrid_peft_ptr,   [r3], #4
        
        @@//////////////////
        @/* m = 2*/
        @/* R0 -> r6,  r0 */
        @/* r5,  X_hybrid_peft_ptr,  r1,  r3,  r11 are free */
        and      r1,     r14,   #0xF
        add      r3,    delay_Qmf_ser_ptr,   r1,  lsl  #8
        add      r3,    r3,   r9,  lsl  #3           
        ldmia    r3,    { X_hybrid_peft_ptr,  r11 }
        ldr        r5,    [AAC_ps_Q_fract_allpass_subQmf20_ptr],  #4  @@// Q_Fract_allpass
        @@//ComplexMult(&RE(tmp), &IM(tmp), RE(tmp0), IM(tmp0), RE(Q_Fract_allpass), IM(Q_Fract_allpass))@
        mov    X_hybrid_peft_ptr,  X_hybrid_peft_ptr,   lsl  #1
        mov    r11,  r11,   lsl  #1        
        SMULWT    r1,   X_hybrid_peft_ptr,   r5
        rsb                X_hybrid_peft_ptr,   X_hybrid_peft_ptr,   #0
        SMULWB    X_hybrid_peft_ptr,   X_hybrid_peft_ptr,   r5
        SMLAWB    r1,   r11,   r5,  r1
        SMLAWT    X_hybrid_peft_ptr,   r11,   r5,  X_hybrid_peft_ptr
        @@// tmp -> r1,  X_hybrid_peft_ptr
        ldr              r5,    [sp,  #7*4]
        mvn            r11,  r6,  lsl #1
        SMLAWB r1,    r11,   r5,  r1
        mvn            r11,  r0,  lsl  #1
        SMLAWB X_hybrid_peft_ptr,    r11,   r5,  X_hybrid_peft_ptr
         @@// tmp -> r1,  X_hybrid_peft_ptr
        mov            r11,  r1,  lsl #1
        SMLAWB r6,    r11,   r5,  r6
        mov            r11,  X_hybrid_peft_ptr,  lsl  #1
        SMLAWB r0,    r11,   r5,  r0        
        str         r6,   [r3], #4
        str         r0,   [r3], #4        
        @@///////////////////////////////
         @@// R0 -> r1,  X_hybrid_peft_ptr
        ldr      r11, [coef_ptr]
        mov    r1,   r1,   lsl  #2
        mov    X_hybrid_peft_ptr,   X_hybrid_peft_ptr,   lsl  #2
        
        SMULWB   r5,  X_hybrid_peft_ptr,   r11
        ldr      X_hybrid_peft_ptr,[sp,      #5*4] 
        SMULWB   r1,  r1,   r11
        ldr      delay_Qmf_nr_allpass_bandsLT22_ptr,[sp,      #3*4]
        
        stmia   X_right_ptr!,  {r1,r5}
        
        
        add       r9,  r9,   #1
        cmp      r9,   group_border_ptr
        BLT     AAC_DECORR_ReconstructAsmStep2LOOP_INTER        
        @@///////////////////////////////////////////////////////
        ldr       group_border_ptr,  [sp,   #4*4]
        add     coef_ptr,  coef_ptr,  #4
        subs     r14,  r14,  #65536
        ldrh      r9,  [group_border_ptr],  #2  @@// sb         
        BGT    AAC_DECORR_ReconstructAsmStep2LOOP
        
                
        @@/* sb: 23--34*/
        @@// coef_ptr(r12),   X_hybrid_peft_ptr(r2),   X_right_ptr(r7)
        ldr       r0,   [sp,  #0]
        ldrh      r14,     [r0,  #4]
        cmp      r14,   #35
        movge    r14,   #35
        subs    r14,   r14,  #23
        BLE   AAC_DECORR_ReconstructAsmStep3LOOPSKIP
        ldrh    r5,   [r0,  #6]    @@// delay_D
        add    delay_Qmf_nr_allpass_bandsLT22_ptr,   r0,  #6400 @@//6592
        add    delay_Qmf_nr_allpass_bandsLT22_ptr,   delay_Qmf_nr_allpass_bandsLT22_ptr,   #(6592-6400)
        mov   r6,    #12*8
        mla    delay_Qmf_nr_allpass_bandsLT22_ptr,    r5,   r6,  delay_Qmf_nr_allpass_bandsLT22_ptr
        add       r5,   r5,  #1
        cmp      r5,  #14
        movge  r5,  #0
       strh       r5,   [r0,  #6]    @@// delay_D
       ldr        r3,     [coef_ptr],  #4
       
AAC_DECORR_ReconstructAsmStep3LOOP:
        ldmia   X_hybrid_peft_ptr!,  {r5, r6}
        ldmia   delay_Qmf_nr_allpass_bandsLT22_ptr,  {r9,  r10}   @@// tmp
        stmia   delay_Qmf_nr_allpass_bandsLT22_ptr!,  {r5, r6}
        mov    r9,    r9,    lsl  #2
        mov    r10,  r10,  lsl  #2        
        SMULWB   r9,    r9,   r3
        SMULWB   r10,    r10,   r3
        stmia   X_right_ptr!,  {r9, r10}       
        subs     r14,  r14,  #1
        BGT    AAC_DECORR_ReconstructAsmStep3LOOP
AAC_DECORR_ReconstructAsmStep3LOOPSKIP:
        @@/* sb: 35--max*/
        ldrh      r5,     [r0,  #4]
        add    delay_Qmf_nr_allpass_bandsLT22_ptr,   r0,  #7936
        ldr        r3,     [coef_ptr],  #4
        subs     r14,   r5,   #35
        BLE    AAC_DECORR_ReconstructAsmStep4LOOPSKIP
AAC_DECORR_ReconstructAsmStep4LOOP:
        
        ldmia   X_hybrid_peft_ptr!,  {r5, r6}
        ldmia   delay_Qmf_nr_allpass_bandsLT22_ptr,  {r9,  r10}   @@// tmp
        stmia   delay_Qmf_nr_allpass_bandsLT22_ptr!,  {r5, r6}
        mov    r9,    r9,    lsl  #2
        mov    r10,  r10,  lsl  #2        
        SMULWB   r9,    r9,   r3
        SMULWB   r10,    r10,   r3
        stmia   X_right_ptr!,  {r9, r10}       
        
        
        subs      r14,  r14,  #1        
        BGT     AAC_DECORR_ReconstructAsmStep4LOOP        
AAC_DECORR_ReconstructAsmStep4LOOPSKIP:
        ldrb      r6,  [r0, #1]  @@// saved_delay
        ldrb      r7,  [r0, #2]  @@// saved_delay
        ldrb      r8,  [r0, #3]  @@// saved_delay
       @@////////////////        
        add       sp,  sp,  #9*4
        add     r6,  r6,  #1
        add     r7,  r7,  #1
        add     r8,  r8,  #1
        
        cmp     r6,  #3
        movge  r6,  #0        
        cmp     r7,  #4
        movge  r7,  #0        
        cmp     r8,  #5
        movge  r8,  #0
        
        strb      r6,  [r0, #1]  @@// saved_delay
        strb      r7,  [r0, #2]  @@// saved_delay
        strb      r8,  [r0, #3]  @@// saved_delay
        
        ldmfd    sp!, {r4-r11, pc} 	    	
       @@ENDFUNC   




@ //void AAC_PsPhaseMixAsm(AAC_PS_INFO_T *ps_ptr,
@//                                              aac_complex   *X_left_ptr,
@//                                              aac_complex   *X_hybrid_left_ptr)@
ps_ptr                                       .req  r0
X_left_ptr                                 .req  r1
X_hybrid_left_ptr                      .req  r2
phase_group_border_ptr           .req  r4
H11_ptr                                    .req  r5

AAC_PsPhaseMixAsm:@   FUNCTION
        .global	AAC_PsPhaseMixAsm	
        stmfd    sp!, {r4-r11, r14}
        sub       sp,    sp,  #4
        str        r0,    [sp,  #0]
        ldr         phase_group_border_ptr,    [r0,  #28]
        mov      r14,  #10
        sub       H11_ptr,      X_hybrid_left_ptr,   #600*4
        @@/*step 1*/
AAC_PsPhaseMixAsmSTEP1_LOOP:
        @@////////////////////////////     
        ldr       r6,      [H11_ptr,  #0*4]      @@// H11  -> r6
        ldr       r7,      [H11_ptr,  #50*4]    @@// H12 -> r7
        ldr       r8,      [H11_ptr,  #100*4]  @@// H21  -> r8
        ldr       r9,      [H11_ptr,  #150*4]  @@// H22 -> r9
        ldr       r10,    [H11_ptr,  #300*4] 
        ldr       r11,    [H11_ptr,  #350*4]  
        
        add      r8,    r8,   r10
        add      r9,    r9,   r11
        str        r8,    [H11_ptr,  #100*4]
        str       r9,      [H11_ptr,  #150*4]        
        movs     r8,   r8,  asr  #16
        ADC    r8,    r8,  #0
        movs    r9,     r9,  asr  #16
        ADC    r9,    r9,  #0        
        
        ldr       r10,    [H11_ptr,  #200*4]
        ldr       r11,    [H11_ptr,  #250*4]
        add      r6,   r6,   r10
        add      r7,   r7,   r11
        str       r7,      [H11_ptr,  #50*4]
        str       r6,      [H11_ptr],  #4
        
        ldrh     r3,    [phase_group_border_ptr],  #2   @@// sb
        movs     r6,   r6,  asr  #16
        ADC    r6,    r6,  #0
        movs     r7,   r7,  asr  #16
        ADC    r7,    r7,  #0
        
        @@/*r3,  r10, r11, r12 are free */
        add     r3,    X_hybrid_left_ptr,   r3,   lsl  #3
        ldr       r10,    [r3,  #0]        @@// X_hybrid_left[sb][0]
        ldr       r11,    [r3,  #64*4]   @@// X_hybrid_right[sb][0]
        mov    r10,  r10,  lsl   #4
        mov    r11,  r11,  lsl   #4
        
        SMULWB   r12,   r10,   r6
        SMLAWB   r12,    r11,   r8, r12
        
        SMULWB   r10,    r10,  r7
        SMLAWB   r10,    r11,  r9,  r10
        str       r12,    [r3,  #0]        @@// X_hybrid_left[sb][0]
        str       r10,    [r3,  #64*4]   @@// X_hybrid_right[sb][0]
        @@///////////////////////////////////
        ldr       r10,    [r3,  #4]        @@// X_hybrid_left[sb][0]
        ldr       r11,    [r3,  #65*4]   @@// X_hybrid_right[sb][0]
        mov    r10,  r10,  lsl   #4
        mov    r11,  r11,  lsl   #4
        
        SMULWB   r12,   r10,   r6
        SMLAWB   r12,    r11,   r8, r12
        
        SMULWB   r10,    r10,  r7
        SMLAWB   r10,    r11,  r9,  r10
        str       r12,    [r3,  #4]        @@// X_hybrid_left[sb][0]
        str       r10,    [r3,  #65*4]   @@// X_hybrid_right[sb][0]
                
        @@////////////////////////////
        subs    r14,  r14,  #1
        BGT   AAC_PsPhaseMixAsmSTEP1_LOOP
        
        @@/*step 2*/
        ldrh       r14,  [r0, #4]
        add      X_left_ptr,  X_left_ptr,  #3*8
        add      X_hybrid_left_ptr,  X_hybrid_left_ptr,   #(128+6)*4 

        add     r14,   r14,  #(10<< 16)
AAC_PsPhaseMixAsmSTEP2_LOOP:
        @@////////////////////////////
        ldr       r6,      [H11_ptr,  #0*4]      @@// H11  -> r6
        ldr       r7,      [H11_ptr,  #50*4]    @@// H12 -> r7
        ldr       r8,      [H11_ptr,  #100*4]  @@// H21  -> r8
        ldr       r9,      [H11_ptr,  #150*4]  @@// H22 -> r9
        ldr       r10,    [H11_ptr,  #300*4] 
        ldr       r11,    [H11_ptr,  #350*4]  
        
        add      r8,    r8,   r10
        add      r9,    r9,   r11
        str        r8,    [H11_ptr,  #100*4]
        str         r9,      [H11_ptr,  #150*4]        
        movs     r8,   r8,  asr  #16
        ADC    r8,    r8,  #0
        movs      r9,     r9,  asr  #16
        ADC    r9,    r9,  #0        
        
        ldr       r10,    [H11_ptr,  #200*4]
        ldr       r11,    [H11_ptr,  #250*4]
        add      r6,   r6,   r10
        add      r7,   r7,   r11
        str       r7,      [H11_ptr,  #50*4]
        str       r6,      [H11_ptr],  #4
        
        ldrh     r3,    [phase_group_border_ptr],  #2   @@// sb
        ldrh     r0,    [phase_group_border_ptr]   @@// sb+1
        movs     r6,   r6,  asr  #16
        ADC    r6,    r6,  #0
        movs     r7,   r7,  asr  #16
        ADC    r7,    r7,  #0
        and       r10,   r14,  #0xFF
        cmp      r0,   r10
        movgt   r0,  r10
        @@////////////////////////////
        @@/*r3,  r10, r11, r12 are free */
AAC_PsPhaseMixAsmSTEP2_LOOP_INTER:
        ldr      r10,       [X_left_ptr]
        ldr      r11,       [X_hybrid_left_ptr]
        mov    r10,  r10,  lsl   #4
        mov    r11,  r11,  lsl   #4
        
        SMULWB   r12,   r10,   r6
        SMLAWB   r12,    r11,   r8, r12
        
        SMULWB   r10,    r10,  r7
        SMLAWB   r10,    r11,  r9,  r10
        str      r12,       [X_left_ptr], #4
        str      r10,       [X_hybrid_left_ptr], #4
        @@//////////////////////////////////////////////////////
        ldr      r10,       [X_left_ptr]
        ldr      r11,       [X_hybrid_left_ptr]
        mov    r10,  r10,  lsl   #4
        mov    r11,  r11,  lsl   #4
        
        SMULWB   r12,   r10,   r6
        SMLAWB   r12,    r11,   r8, r12
        
        SMULWB   r10,    r10,  r7
        SMLAWB   r10,    r11,  r9,  r10
        str      r12,       [X_left_ptr], #4
        str      r10,       [X_hybrid_left_ptr], #4
        @@////////////////////////////
        add       r3,   r3,  #1
        cmp      r3,   r0
        BLT     AAC_PsPhaseMixAsmSTEP2_LOOP_INTER
        @@////////////////////////////
        subs     r14,  r14,  #(1<<16)
        BGT    AAC_PsPhaseMixAsmSTEP2_LOOP
        @@/* sb = 35: max */ 
        ldr        r0,    [sp,  #0]
        add       sp,    sp,  #4
        ldrh       r0,  [r0, #4]
        @@////////////////////////////
        ldr       r6,      [H11_ptr,  #0*4]      @@// H11  -> r6
        ldr       r7,      [H11_ptr,  #50*4]    @@// H12 -> r7
        ldr       r8,      [H11_ptr,  #100*4]  @@// H21  -> r8
        ldr       r9,      [H11_ptr,  #150*4]  @@// H22 -> r9
        ldr       r10,    [H11_ptr,  #300*4] 
        ldr       r11,    [H11_ptr,  #350*4]  
        
        add      r8,    r8,   r10
        add      r9,    r9,   r11
        str        r8,    [H11_ptr,  #100*4]
        str         r9,      [H11_ptr,  #150*4]        
        movs     r8,   r8,  asr  #16
        ADC    r8,    r8,  #0
        movs      r9,     r9,  asr  #16
        ADC    r9,    r9,  #0        
        
        ldr       r10,    [H11_ptr,  #200*4]
        ldr       r11,    [H11_ptr,  #250*4]
        add      r6,   r6,   r10
        add      r7,   r7,   r11
        str       r7,      [H11_ptr,  #50*4]
        str       r6,      [H11_ptr],  #4
        movs     r6,   r6,  asr  #16
        ADC    r6,    r6,  #0
        movs     r7,   r7,  asr  #16
        ADC    r7,    r7,  #0
        @@////////////////////////////      
                
        subs     r0,  r0,  #35
        BLE     AAC_PsPhaseMixAsmSTEP3_LOOP_SKIP        
AAC_PsPhaseMixAsmSTEP3_LOOP:
        ldr      r10,       [X_left_ptr]
        ldr      r11,       [X_hybrid_left_ptr]
        mov    r10,  r10,  lsl   #4
        mov    r11,  r11,  lsl   #4
        
        SMULWB   r12,   r10,   r6
        SMLAWB   r12,    r11,   r8, r12
        
        SMULWB   r10,    r10,  r7
        SMLAWB   r10,    r11,  r9,  r10
        str      r12,       [X_left_ptr], #4
        str      r10,       [X_hybrid_left_ptr], #4
        @@//////////////////////////////////////////////////////
        ldr      r10,       [X_left_ptr]
        ldr      r11,       [X_hybrid_left_ptr]
        mov    r10,  r10,  lsl   #4
        mov    r11,  r11,  lsl   #4
        
        SMULWB   r12,   r10,   r6
        SMLAWB   r12,    r11,   r8, r12
        
        SMULWB   r10,    r10,  r7
        SMLAWB   r10,    r11,  r9,  r10
        str      r12,       [X_left_ptr], #4
        str      r10,       [X_hybrid_left_ptr], #4
        @@////////////////////////////
        
        subs      r0,   r0,  #1
        BGT     AAC_PsPhaseMixAsmSTEP3_LOOP

AAC_PsPhaseMixAsmSTEP3_LOOP_SKIP:
        @@////////////////////////////        
        ldmfd    sp!, {r4-r11, pc} 	    	
        @ENDFUNC   




    @@// void HybridSynthesisAsm(aac_complex    *X_left, 
    @@//                                         aac_complex    *X_hybrid_left)@
X_hybrid_left    .req  r1
HybridSynthesisAsm:@   FUNCTION
    .global	HybridSynthesisAsm	    
    stmfd    sp!, {r4-r5, r14}
    
    @/* 0 */
    ldmia    X_hybrid_left!,  {r4,  r5}       @@// t00 -> r4,   t01-> r5
    ldr        r2,    [X_hybrid_left,  #62*4]     @@// t10 -> r2
    ldr        r3,    [X_hybrid_left,  #63*4]     @@// t11 -> r3
    
    @/* 1 */
    ldmia    X_hybrid_left!,  {r12,  r14}           
    add      r4,    r4,   r12
    add      r5,    r5,   r14    
    ldr        r12,    [X_hybrid_left,  #62*4]
    ldr        r14,    [X_hybrid_left,  #63*4]    
    add      r2,    r2,   r12
    add      r3,    r3,   r14

    @/* 2 */
    ldmia    X_hybrid_left!,  {r12,  r14}           
    add      r4,    r4,   r12
    add      r5,    r5,   r14    
    ldr        r12,    [X_hybrid_left,  #62*4]
    ldr        r14,    [X_hybrid_left,  #63*4]    
    add      r2,    r2,   r12
    add      r3,    r3,   r14

   @/* 3 */
    ldmia    X_hybrid_left!,  {r12,  r14}           
    add      r4,    r4,   r12
    add      r5,    r5,   r14    
    ldr        r12,    [X_hybrid_left,  #62*4]
    ldr        r14,    [X_hybrid_left,  #63*4]    
    add      r2,    r2,   r12
    add      r3,    r3,   r14

    @/* 4 */
    ldmia    X_hybrid_left!,  {r12,  r14}           
    add      r4,    r4,   r12
    add      r5,    r5,   r14    
    ldr        r12,    [X_hybrid_left,  #62*4]
    ldr        r14,    [X_hybrid_left,  #63*4]    
    add      r2,    r2,   r12
    add      r3,    r3,   r14

    @/* 5 */
    ldmia    X_hybrid_left!,  {r12,  r14}           
    add      r4,    r4,   r12
    add      r5,    r5,   r14    
    ldr        r12,    [X_hybrid_left,  #62*4]
    ldr        r14,    [X_hybrid_left,  #63*4]    
    add      r2,    r2,   r12
    add      r3,    r3,   r14

   @/* 6 */
    ldmia    X_hybrid_left!,  {r12,  r14}           
    add      r4,    r4,   r12
    add      r5,    r5,   r14    
    ldr        r12,    [X_hybrid_left,  #62*4]
    ldr        r14,    [X_hybrid_left,  #63*4]    
    add      r2,    r2,   r12
    add      r3,    r3,   r14

    @/* 7 */
    ldmia    X_hybrid_left!,  {r12,  r14}           
    add      r4,    r4,   r12
    add      r5,    r5,   r14    
    ldr        r12,    [X_hybrid_left,  #62*4]
    ldr        r14,    [X_hybrid_left,  #63*4]    
    add      r2,    r2,   r12
    add      r3,    r3,   r14
    
    stmia    r0!,   {r4, r5}
    str        r2,     [X_hybrid_left,   #112*4]
    str        r3,     [X_hybrid_left,   #113*4]
    
    @@/////////////////////////////////////////
    @/* 0 */
    ldmia    X_hybrid_left!,  {r4,  r5}       @@// t00 -> r4,   t01-> r5
    ldr        r2,    [X_hybrid_left,  #62*4]     @@// t10 -> r2
    ldr        r3,    [X_hybrid_left,  #63*4]     @@// t11 -> r3
    
    @/* 1 */
    ldmia    X_hybrid_left!,  {r12,  r14}           
    add      r4,    r4,   r12
    add      r5,    r5,   r14    
    ldr        r12,    [X_hybrid_left,  #62*4]
    ldr        r14,    [X_hybrid_left,  #63*4]    
    add      r2,    r2,   r12
    add      r3,    r3,   r14
    
    stmia    r0!,   {r4, r5}
    str        r2,     [X_hybrid_left,   #110*4]
    str        r3,     [X_hybrid_left,   #111*4]
    
    
    @@/////////////////////////////////////////
    @/* 0 */
    ldmia    X_hybrid_left!,  {r4,  r5}       @@// t00 -> r4,   t01-> r5
    ldr        r2,    [X_hybrid_left,  #62*4]     @@// t10 -> r2
    ldr        r3,    [X_hybrid_left,  #63*4]     @@// t11 -> r3
    
    @/* 1 */
    ldmia    X_hybrid_left!,  {r12,  r14}           
    add      r4,    r4,   r12
    add      r5,    r5,   r14    
    ldr        r12,    [X_hybrid_left,  #62*4]
    ldr        r14,    [X_hybrid_left,  #63*4]    
    add      r2,    r2,   r12
    add      r3,    r3,   r14
    
    stmia    r0!,   {r4, r5}
    str        r2,     [X_hybrid_left,   #108*4]
    str        r3,     [X_hybrid_left,   #109*4]
    
    

    ldmfd    sp!, {r4-r5, pc} 	    	
    @ENDFUNC   



@//void AAC_PS_HybridAnalysisAsm(aac_complex   *overlap_ptr, 
@//                                                       aac_complex    * X_ptr,             // S16.3 the input data and store the output left channel data 
@//                                                       aac_complex    * X_hybrid_ptr, // S16.3 the input data and store the output hybrid channel data
@//                                                       int16  slot)
ps_overlap_ptr      .req  r0
ps_X_ptr               .req  r1
ps_X_hybrid_ptr   .req  r2
slot                        .req  r3


AAC_PS_HybridAnalysisAsm:@   FUNCTION
    .global	AAC_PS_HybridAnalysisAsm	    
    stmfd    sp!, {r4-r11, r14}
    @////////////////////////////////////
    add        ps_overlap_ptr,  ps_overlap_ptr,   slot,  lsl   #3
    ldmia      ps_X_ptr,     {r4-r9}    
    str         r4,   [ps_overlap_ptr,  #24*4]
    str         r5,   [ps_overlap_ptr,  #25*4]    
    str         r6,   [ps_overlap_ptr,  #(24*4 + 88*4)]
    str         r7,   [ps_overlap_ptr,  #(25*4 + 88*4)]    
    str         r8,   [ps_overlap_ptr,  #(24*4 + 88*2*4)]
    str         r9,   [ps_overlap_ptr,  #(25*4 + 88*2*4)]
    
    @@/* r0,  r2 are used */
    @@/* first do 8 point */
    @@/* r3, r4, r5 ,r6, r7, r8, r9, r10*/    
    ldr       r11,     =0x17483c62  @@// [2], [5]
    ldr       r12,     =0xba02534    @@// [1], [3]
    ldr       r14,     =0x3d2329d    @@// [0], [4]
    
    ldr       r7,  [ps_overlap_ptr,  #21*4]   @@// QMF_IM(buffer[10+i]
    ldr       r1,  [ps_overlap_ptr,  #5*4]     @@// QMF_IM(buffer[2+i])
    ldr       r3,  [ps_overlap_ptr,  #12*4]   @@// QMF_RE(buffer[6+i]    
    sub      r7,  r7,  r1
    SMULWT   r7,      r7,      r11      @@// * filter[2]
    
    ldr       r4,     [ps_overlap_ptr,  #10*4]    @@// QMF_RE(buffer[5+i]
    ldr       r1,     [ps_overlap_ptr,  #14*4]    @@// QMF_RE(buffer[7+i]
    ldr       r8,     [ps_overlap_ptr,  #23*4]    @@// QMF_IM(buffer[11+i]
    @@///////////////
    add     r3,        r7,     r3,   asr  #2                   @@//input_re1[0] + input_im1[3]@
    sub     r7,        r3,     r7,    lsl   #1    @@//input_re1[0] - input_im1[3]@
    
    add     r4,       r4,     r1
    ldr       r1,      [ps_overlap_ptr,  #3*4]      @@// QMF_IM(buffer[1+i])
    SMULWB   r4,  r4,   r11      @@//  input_re1[1]
    sub      r8,    r8,    r1              @@// QMF_IM(buffer[11+i]) - QMF_IM(buffer[1+i])
    
    ldr       r9,       [ps_overlap_ptr,  #19*4]      @@// QMF_IM(buffer[9+i])
    ldr       r10,     [ps_overlap_ptr,  #7*4]        @@// QMF_IM(buffer[3+i])
    
    SMULWT    r8,    r8,    r12
    sub      r9,   r9,  r10              @@// QMF_IM(buffer[9+i]) -  QMF_IM(buffer[3+i])
    
    
    SMLAWB    r8,    r9,   r12,  r8         @@// input_im1[2]
    
    ldr       r5,       [ps_overlap_ptr,  #0*4]      @@// QMF_RE(buffer[0+i])
    ldr       r6,       [ps_overlap_ptr,  #24*4]    @@// QMF_RE(buffer[12+i])
    @@///////////////
    add     r4,        r4,     r8                   @@//input_re1[1] + input_im1[2]@
    sub     r8,        r4,     r8,    lsl   #1     @@//input_re1[1] - input_im1[2]@
    
    add    r5,         r5,     r6    
    ldr       r6,       [ps_overlap_ptr,  #8*4]      @@// QMF_RE(buffer[4+i])
    ldr       r1,       [ps_overlap_ptr,  #16*4]    @@// QMF_RE(buffer[8+i])
    SMULWT     r5,    r5,  r14
    add    r6,  r6,  r1
    rsb     r5,  r5,   #0
    SMLAWB    r5,    r6,   r14,     r5             @@//  input_re1[2]
    
    ldr       r9,       [ps_overlap_ptr,  #25*4]      @@// QMF_IM(buffer[12+i])
    ldr       r10,     [ps_overlap_ptr,  #1*4]    @@// QMF_IM(buffer[0+i])    
    ldr       r1,       [ps_overlap_ptr,  #17*4]    @@// QMF_IM(buffer[8+i])    
    sub     r9,    r9,   r10
    ldr       r10,     [ps_overlap_ptr,  #9*4]    @@// QMF_IM(buffer[4+i])    
    SMULWT    r9,     r9,  r14
    sub     r1,   r1,   r10
    SMLAWB   r9,     r1,  r14,    r9            @@//   input_im1[1]
    ldr       r6,       [ps_overlap_ptr,  #2*4]      @@// QMF_RE(buffer[1+i])
    @@///////////////
    add     r5,        r5,     r9                   @@//input_re1[2] + input_im1[1]@
    sub     r9,        r5,     r9,    lsl   #1     @@//input_re1[2] - input_im1[1]@    
    
    ldr       r1,       [ps_overlap_ptr,  #22*4]    @@// QMF_RE(buffer[11+i])
    
    ldr       r10,     [ps_overlap_ptr,  #6*4]    @@// QMF_RE(buffer[3+i])
    add     r6,    r6,     r1
    ldr       r1,       [ps_overlap_ptr,  #18*4]    @@// QMF_RE(buffer[9+i])    
    SMULWT      r6,   r6,    r12
    add     r1,        r10,     r1
    
    ldr       r10,     [ps_overlap_ptr,  #15*4]    @@// QMF_IM(buffer[7+i])    
    rsb      r6,        r6,    #0
    SMLAWB      r6,     r1,    r12,   r6            @@//  input_re1[3]    
    ldr      r1,         [ps_overlap_ptr,  #11*4]    @@// QMF_IM(buffer[5+i])   
    @@/**/    
    ldr     r12,  = 0x5a82539f    @@// 
    sub    r10,      r10,    r1
    SMULWB    r10,     r10,  r11    @@// input_im1[0]
    ldr    r14,  = 0xc4dfbaba
    @@/**/        
    @@///////////////
    add     r6,        r6,     r10                   @@//input_re1[3] + input_im1[0]@
    sub     r10,      r6,     r10,    lsl   #1     @@//input_re1[3] - input_im1[0]@    
    @@//   DCT3_4_unscaled
    @@/*x[0]->r3,  x[1]->r4,  x[2]->r5,  x[3]->r6*/
    @@/* r1 is free */
    SMULWT     r5,   r5,   r12        @@// f0 = AAC_MULSHIFT32_OPT(1518500250, x[2])@
    add                r1,    r4,  r6          @@// f3 = (x[1] + x[3])@
    SMULWT     r1,    r1, r14         @@// f5 = AAC_MULSHIFT32_OPT(-992008095,f3)@
    SMULWB     r4,    r4,    r12     @@// f4 = AAC_MULSHIFT32_OPT(701455651*2,  x[1])@
    SMULWB     r6,    r6,    r14     @@// f6 = AAC_MULSHIFT32_OPT(-1162209775,  x[3])@
    rsb                  r3,        r5,    r3,    asr    #1   @@// f1 = (x[0]>>1) - f0@
    add                 r5,        r3,    r5,    lsl     #1   @@// f2 = (x[0]>>1) + f0@            
    add                 r4,        r1,     r4,    lsl   #1  @@//f7 = (f4 << 1) + f5@
    sub                 r6,        r6,      r1                  @@//f8 = f6 - f5@    
    add                r5,     r5,    r6                     @@// y[0]
    sub                r6,     r5,    r6,  lsl  #1         @@// y[3]    
    add                r3,     r3,    r4                    @@// y[1]
    sub                r4,     r3,    r4,  lsl  #1         @// y[2]    
    str                 r5,     [ps_X_hybrid_ptr]
    @//str                 r4,     [ps_X_hybrid_ptr,  #4*4]
    @//str                 r6,     [ps_X_hybrid_ptr,  #8*4]
    str                 r3,     [ps_X_hybrid_ptr,  #12*4]
    
    @@/*x[0]->r7,  x[1]->r8,  x[2]->r9,  x[3]->r10*/
    @@/* r1 is free */
    SMULWT     r9,   r9,   r12        @@// f0 = AAC_MULSHIFT32_OPT(1518500250, x[2])@
    add                r1,    r8,  r10          @@// f3 = (x[1] + x[3])@
    SMULWT     r1,    r1, r14         @@// f5 = AAC_MULSHIFT32_OPT(-992008095,f3)@
    SMULWB     r8,    r8,    r12     @@// f4 = AAC_MULSHIFT32_OPT(701455651*2,  x[1])@
    SMULWB     r10,    r10,    r14     @@// f6 = AAC_MULSHIFT32_OPT(-1162209775,  x[3])@
    rsb                  r7,        r9,    r7,    asr    #1   @@// f1 = (x[0]>>1) - f0@
    add                 r9,        r7,    r9,    lsl     #1   @@// f2 = (x[0]>>1) + f0@            
    add                 r8,        r1,     r8,    lsl   #1  @@//f7 = (f4 << 1) + f5@
    sub                 r10,        r10,      r1                  @@//f8 = f6 - f5@    
    add                r9,     r9,    r10                     @@// y[0]
    sub                r10,     r9,    r10,  lsl  #1         @@// y[3]    
    add                r7,     r7,    r8                    @@// y[1]
    sub                r8,     r7,    r8,  lsl  #1         @// y[2]    
    str                 r7,     [ps_X_hybrid_ptr,  #2*4]
    @//str                 r10,     [ps_X_hybrid_ptr,  #6*4]
    @//str                 r8,     [ps_X_hybrid_ptr,  #10*4]
    str                 r9,     [ps_X_hybrid_ptr,  #14*4]
    @@///////////////////////////
    add     r10,   r10,    r6
    add     r4,     r4,      r8
    mov    r8,   #0
    str       r4,     [ps_X_hybrid_ptr,  #4*4]
    str       r10,     [ps_X_hybrid_ptr,  #6*4]    
    str                 r8,     [ps_X_hybrid_ptr,  #8*4]
    str                 r8,     [ps_X_hybrid_ptr,  #10*4]
    @@/*************************step 6***************************/    
    @@/* r3, r4, r5 ,r6, r7, r8, r9, r10*/    
    ldr       r12,     =0xba02534    @@// [1], [3]
    ldr       r14,     =0x3d2329d    @@// [0], [4]
    
    ldr       r7,  [ps_overlap_ptr,  #20*4]   @@// QMF_RE(buffer[10+i]
    ldr       r1,  [ps_overlap_ptr,  #4*4]     @@// QMF_RE(buffer[2+i])
    ldr       r3,  [ps_overlap_ptr,  #13*4]   @@// QMF_IM(buffer[6+i]    
    sub      r7,  r7,  r1
    SMULWT   r7,      r7,      r11      @@// * filter[2]
    
    ldr       r4,     [ps_overlap_ptr,  #11*4]    @@// QMF_IM(buffer[5+i]
    ldr       r1,     [ps_overlap_ptr,  #15*4]    @@// QMF_IM(buffer[7+i]
    ldr       r8,     [ps_overlap_ptr,  #22*4]    @@// QMF_RE(buffer[11+i]
    @@///////////////
    add     r3,        r7,     r3,  asr  #2                   @@//input_re1[0] + input_im1[3]@
    sub     r7,        r3,     r7,    lsl   #1    @@//input_re1[0] - input_im1[3]@
    
    add     r4,       r4,     r1
    ldr       r1,      [ps_overlap_ptr,  #2*4]      @@// QMF_RE(buffer[1+i])
    SMULWB   r4,  r4,   r11      @@//  input_re1[1]
    sub      r8,    r8,    r1              @@// QMF_IM(buffer[11+i]) - QMF_IM(buffer[1+i])
    
    ldr       r9,       [ps_overlap_ptr,  #18*4]      @@// QMF_RE(buffer[9+i])
    ldr       r10,     [ps_overlap_ptr,  #6*4]        @@// QMF_RE(buffer[3+i])
    SMULWT    r8,    r8,    r12
    sub      r9,   r9,  r10              @@// QMF_IM(buffer[9+i]) -  QMF_IM(buffer[3+i])
    
    
    SMLAWB    r8,    r9,   r12,  r8         @@// input_im1[2]
    
    ldr       r5,       [ps_overlap_ptr,  #1*4]      @@// QMF_IM(buffer[0+i])
    ldr       r6,       [ps_overlap_ptr,  #25*4]    @@// QMF_IM(buffer[12+i])
    @@///////////////
    add     r4,        r4,     r8                   @@//input_re1[1] + input_im1[2]@
    sub     r8,        r4,     r8,    lsl   #1     @@//input_re1[1] - input_im1[2]@
    
    add    r5,         r5,     r6    
    ldr       r6,       [ps_overlap_ptr,  #9*4]      @@// QMF_IM(buffer[4+i])
    ldr       r1,       [ps_overlap_ptr,  #17*4]    @@// QMF_IM(buffer[8+i])
    SMULWT     r5,    r5,  r14
    add    r6,  r6,  r1
    rsb     r5,  r5,   #0
    SMLAWB    r5,    r6,   r14,     r5             @@//  input_re1[2]
    
    ldr       r9,       [ps_overlap_ptr,  #24*4]      @@// QMF_RE(buffer[12+i])
    ldr       r10,     [ps_overlap_ptr,  #0*4]    @@// QMF_RE(buffer[0+i])    
    ldr       r1,       [ps_overlap_ptr,  #16*4]    @@// QMF_RE(buffer[8+i])    
    sub     r9,    r9,   r10
    ldr       r10,     [ps_overlap_ptr,  #8*4]    @@// QMF_RE(buffer[4+i])    
    SMULWT    r9,     r9,  r14
    sub     r1,   r1,   r10
    SMLAWB   r9,     r1,  r14,    r9            @@//   input_im1[1]
    ldr       r6,       [ps_overlap_ptr,  #3*4]      @@// QMF_IM(buffer[1+i])
    @@///////////////
    add     r5,        r5,     r9                   @@//input_re1[2] + input_im1[1]@
    sub     r9,        r5,     r9,    lsl   #1     @@//input_re1[2] - input_im1[1]@    
    
    ldr       r1,       [ps_overlap_ptr,  #23*4]    @@// QMF_IM(buffer[11+i])
    
    ldr       r10,     [ps_overlap_ptr,  #7*4]    @@// QMF_IM(buffer[3+i])
    add     r6,    r6,     r1
    ldr       r1,       [ps_overlap_ptr,  #19*4]    @@// QMF_IM(buffer[9+i])    
    SMULWT      r6,   r6,    r12
    add     r1,        r10,     r1
    
    ldr       r10,     [ps_overlap_ptr,  #14*4]    @@// QMF_RE(buffer[7+i])    
    rsb      r6,        r6,    #0
    SMLAWB      r6,     r1,    r12,   r6            @@//  input_re1[3]    
    ldr      r1,         [ps_overlap_ptr,  #10*4]    @@// QMF_RE(buffer[5+i])   
    @@/**/    
    ldr     r12,  = 0x5a82539f    @@// 
    sub    r10,      r10,    r1
    SMULWB    r10,     r10,  r11    @@// input_im1[0]
    ldr    r14,  = 0xc4dfbaba
    @@/**/        
    @@///////////////
    add     r6,        r6,     r10                   @@//input_re1[3] + input_im1[0]@
    sub     r10,      r6,     r10,    lsl   #1     @@//input_re1[3] - input_im1[0]@       
    
    
    @@//   DCT3_4_unscaled
    @@/*x[0]->r3,  x[1]->r4,  x[2]->r5,  x[3]->r6*/
    @@/* r1 is free */
    SMULWT     r5,   r5,   r12        @@// f0 = AAC_MULSHIFT32_OPT(1518500250, x[2])@
    add                r1,    r4,  r6          @@// f3 = (x[1] + x[3])@
    SMULWT     r1,    r1, r14         @@// f5 = AAC_MULSHIFT32_OPT(-992008095,f3)@
    SMULWB     r4,    r4,    r12     @@// f4 = AAC_MULSHIFT32_OPT(701455651*2,  x[1])@
    SMULWB     r6,    r6,    r14     @@// f6 = AAC_MULSHIFT32_OPT(-1162209775,  x[3])@
    rsb                  r3,        r5,    r3,    asr    #1   @@// f1 = (x[0]>>1) - f0@
    add                 r5,        r3,    r5,    lsl     #1   @@// f2 = (x[0]>>1) + f0@            
    add                 r4,        r1,     r4,    lsl   #1  @@//f7 = (f4 << 1) + f5@
    sub                 r6,        r6,      r1                  @@//f8 = f6 - f5@    
    add                r5,     r5,    r6                     @@// y[0]
    sub                r6,     r5,    r6,  lsl  #1         @@// y[3]    
    add                r3,     r3,    r4                    @@// y[1]
    sub                r4,     r3,    r4,  lsl  #1         @// y[2]    
    str                 r3,     [ps_X_hybrid_ptr, #3*4]
    @//str                 r6,     [ps_X_hybrid_ptr,  #7*4]
    @//str                 r4,     [ps_X_hybrid_ptr,  #11*4]
    str                 r5,     [ps_X_hybrid_ptr,  #15*4]
    
    @@/*x[0]->r7,  x[1]->r8,  x[2]->r9,  x[3]->r10*/
    @@/* r1 is free */
    SMULWT     r9,   r9,   r12        @@// f0 = AAC_MULSHIFT32_OPT(1518500250, x[2])@
    add                r1,    r8,  r10          @@// f3 = (x[1] + x[3])@
    SMULWT     r1,    r1, r14         @@// f5 = AAC_MULSHIFT32_OPT(-992008095,f3)@
    SMULWB     r8,    r8,    r12     @@// f4 = AAC_MULSHIFT32_OPT(701455651*2,  x[1])@
    SMULWB     r10,    r10,    r14     @@// f6 = AAC_MULSHIFT32_OPT(-1162209775,  x[3])@
    rsb                  r7,        r9,    r7,    asr    #1   @@// f1 = (x[0]>>1) - f0@
    add                 r9,        r7,    r9,    lsl     #1   @@// f2 = (x[0]>>1) + f0@            
    add                 r8,        r1,     r8,    lsl   #1  @@//f7 = (f4 << 1) + f5@
    sub                 r10,        r10,      r1                  @@//f8 = f6 - f5@    
    add                r9,     r9,    r10                     @@// y[0]
    sub                r10,     r9,    r10,  lsl  #1         @@// y[3]    
    add                r7,     r7,    r8                    @@// y[1]
    sub                r8,     r7,    r8,  lsl  #1         @// y[2]    
    str                 r9,     [ps_X_hybrid_ptr,  #1*4]
    @//str                 r8,     [ps_X_hybrid_ptr,  #5*4]
    @//str                 r10,     [ps_X_hybrid_ptr,  #9*4]
    str                 r7,     [ps_X_hybrid_ptr,  #13*4]
    
    add              r6,  r6,   r10
    add              r8,  r8,   r4
    mov             r4,   #0
    str                 r6,     [ps_X_hybrid_ptr,  #7*4]
    str                 r8,     [ps_X_hybrid_ptr,  #5*4]
    str                 r4,     [ps_X_hybrid_ptr,  #9*4]
    str                 r4,     [ps_X_hybrid_ptr,  #11*4]
    
    @@/* second do 2 point */
    ldr       r12,   =0x4dded54
    ldr       r14,   =0x4e54
    ldr        r3,   [ps_overlap_ptr,   #(  3*4+88*4)]
    ldr        r4,   [ps_overlap_ptr,   #(23*4+88*4)]
    
    ldr        r5,   [ps_overlap_ptr,   #(  7*4+88*4)]
    ldr        r6,   [ps_overlap_ptr,   #(19*4+88*4)]
    
    ldr        r7,   [ps_overlap_ptr,   #(11*4+88*4)]
    ldr        r8,   [ps_overlap_ptr,   #(15*4+88*4)]
    
    add      r3,  r3,  r4
    add      r4,  r5,  r6
    add      r5,  r7,  r8
    
    SMULWT    r1,      r3,    r12
    SMLAWB    r1,      r4,    r12,   r1
    SMLAWB    r1,      r5,    r14,   r1
    
    ldr        r3,   [ps_overlap_ptr,   #(  2*4+88*4)]
    ldr        r4,   [ps_overlap_ptr,   #(22*4+88*4)]
    
    ldr        r5,   [ps_overlap_ptr,   #(  6*4+88*4)]
    ldr        r6,   [ps_overlap_ptr,   #(18*4+88*4)]
    
    ldr        r7,   [ps_overlap_ptr,   #(10*4+88*4)]
    ldr        r8,   [ps_overlap_ptr,   #(14*4+88*4)]
    
    add      r3,  r3,  r4
    add      r4,  r5,  r6
    add      r5,  r7,  r8   
    
    SMULWT    r3,      r3,    r12
    SMLAWB    r3,      r4,    r12,   r3
    SMLAWB    r3,      r5,    r14,   r3
    
    ldr                 r9,      [ps_overlap_ptr,   #(12*4+88*4)]  @@// r6
    ldr                 r10,    [ps_overlap_ptr,   #(13*4+88*4)]  @@// i6
    @@// i1 -> r1,    r1 ->  r3    
    add         r9,      r3,    r9,  asr    #1             @@// (int32_t)(r1>> 0) + r6@
    sub          r3,     r9,    r3,  lsl  #1 @@//  r6 - ((int32_t)(r1>> 0))@
        
    add         r10,      r1,    r10,   asr  #1            @@//  (int32_t)(i1>> 0) + i6@
    sub          r1,       r10,    r1,  lsl  #1@@//  i6 - ((int32_t)(i1>> 0))@
    
    str           r9,     [ps_X_hybrid_ptr,  #(0*4 + 16*4)]           
    str           r10,   [ps_X_hybrid_ptr,  #(1*4 + 16*4)]           
    str           r3,     [ps_X_hybrid_ptr,  #(2*4 + 16*4)]           
    str           r1,     [ps_X_hybrid_ptr,  #(3*4 + 16*4)]           
    
    
    @@/* Third do 2 point */
    ldr       r12,   =0x4dded54
    ldr       r14,   =0x4e54
    ldr        r3,   [ps_overlap_ptr,   #(  3*4+88*4*2)]
    ldr        r4,   [ps_overlap_ptr,   #(23*4+88*4*2)]
    
    ldr        r5,   [ps_overlap_ptr,   #(  7*4+88*4*2)]
    ldr        r6,   [ps_overlap_ptr,   #(19*4+88*4*2)]
    
    ldr        r7,   [ps_overlap_ptr,   #(11*4+88*4*2)]
    ldr        r8,   [ps_overlap_ptr,   #(15*4+88*4*2)]
    
    add      r3,  r3,  r4
    add      r4,  r5,  r6
    add      r5,  r7,  r8
    
    SMULWT    r1,      r3,    r12
    SMLAWB    r1,      r4,    r12,   r1
    SMLAWB    r1,      r5,    r14,   r1
    
    ldr        r3,   [ps_overlap_ptr,   #(  2*4+88*4*2)]
    ldr        r4,   [ps_overlap_ptr,   #(22*4+88*4*2)]
    
    ldr        r5,   [ps_overlap_ptr,   #(  6*4+88*4*2)]
    ldr        r6,   [ps_overlap_ptr,   #(18*4+88*4*2)]
    
    ldr        r7,   [ps_overlap_ptr,   #(10*4+88*4*2)]
    ldr        r8,   [ps_overlap_ptr,   #(14*4+88*4*2)]
    
    add      r3,  r3,  r4
    add      r4,  r5,  r6
    add      r5,  r7,  r8   
    
    SMULWT    r3,      r3,    r12
    SMLAWB    r3,      r4,    r12,   r3
    SMLAWB    r3,      r5,    r14,   r3
    
    ldr                 r9,      [ps_overlap_ptr,   #(12*4+88*4*2)]  @@// r6
    ldr                 r10,    [ps_overlap_ptr,   #(13*4+88*4*2)]  @@// i6
    @@// i1 -> r1,    r1 ->  r3    
    add         r9,      r3,    r9,  asr  #1             @@// (int32_t)(r1>> 0) + r6@
    sub          r3,     r9,    r3,  lsl  #1 @@//  r6 - ((int32_t)(r1>> 0))@
        
    add         r10,      r1,    r10,  asr  #1            @@//  (int32_t)(i1>> 0) + i6@
    sub          r1,       r10,    r1,  lsl  #1@@//  i6 - ((int32_t)(i1>> 0))@
    
    str           r9,     [ps_X_hybrid_ptr,  #(0*4 + 20*4)]           
    str           r10,   [ps_X_hybrid_ptr,  #(1*4 + 20*4)]           
    str           r3,     [ps_X_hybrid_ptr,  #(2*4 + 20*4)]           
    str           r1,     [ps_X_hybrid_ptr,  #(3*4 + 20*4)]           
    
    
    @@/////////////////////////////////
    ldmfd    sp!, {r4-r11, pc}
    @ENDFUNC   
    
    
    
    
    
    
    
    
    
    .end@END
        