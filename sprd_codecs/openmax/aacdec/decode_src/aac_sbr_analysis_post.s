@// SBR synthesis dct pre processing
@// author: reed.zhang
@// input: SBR synthesis filter bank processing
@// r0: output sample pointer
@// r1: the input data
@// r2: the kx value
@// purpose:

@// define the register name 


out_ptr            .req    r0
in_real_ptr        .req   r1
kx                 .req   r2
tab_ptr            .req    r3
bit_rev_tab_ptr    .req    r4
k                  .req    r14
TAB                .req     r7
sum0               .req     r5
sum1               .req     r6
                .text@ AREA    PROGRAM, CODE, READONLY                         
                .arm@CODE32                          
                .global  asm_sbr_analysis_post
asm_sbr_analysis_post:
                stmfd   sp!, {r4 - r10, r14}
                ldr     tab_ptr, =ANLAYSIS_POST_TABLE
                ldr     bit_rev_tab_ptr, =BIT_POS_TAB
                                               
                mov     k,    kx,    lsr  #1
                ands    r12,   kx,    #0x1
                addne  k,   k,  #1
                
                mov    r10,   #0
FILTER_LOOP1:
                @//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
                @//the function body
                ldrb     r8,    [bit_rev_tab_ptr],  #1
                ldrb     r12,    [bit_rev_tab_ptr],  #1
                ldr       TAB,  [tab_ptr],  #4                
                add     r8,  in_real_ptr,   r8,  lsl  #2
                ldr       r9,    [r8,  #32*4]     @@// im
                ldr       r8,    [r8]                  @@// re           
                 
                @//mov    r9,  r9,  asr  #1
                @//mov    r8,  r8,  asr  #1
                mov     sum1,    r9,  asr   #1
                sub      sum1,    sum1,   r8,   asr  #1    @@// ((x_im>>1) - (x_re>>1))@
                add     sum0,    sum1,      r8           @@// (x_re + (x_im>>1) - (x_re>>1))@    
                
                            
                SMLAWT   sum0,    r9,    TAB,  sum0
                SMLAWB   sum1,    r9,    TAB,  sum1                
                SMLAWB  sum0,     r8,     TAB,  sum0
                rsb     r8,  r8,  #0
                SMLAWT  sum1,     r8,     TAB,  sum1                
                mov     sum0,  sum0,  asr  #5
                mov     sum1,  sum1,  asr  #5
                stmia   out_ptr!, {sum0-sum1}
                @@//////////////////////////////////////
                ldr       TAB,  [tab_ptr],  #4                
                add     r9,  in_real_ptr,   r12,  lsl  #2               
                ldr       r8,    [r9]                  @@// re        
                ldr       r9,    [r9,  #32*4]     @@// im        
                
                @//mov    r8,  r8,  asr  #1 
                @//mov    r9,  r9,  asr  #1                
                mov     sum0,    r8,  asr   #1
                sub      sum0,    sum0,   r9,   asr  #1    @@//((x_re>>1) - (x_im>>1))
                add     sum1,    sum0,      r9           @@// (x_im + (x_re>>1) - (x_im>>1))@
                              
                SMLAWT   sum1,    r9,    TAB,  sum1
                rsb     r9,  r9,  #0
                SMLAWB   sum0,    r9,    TAB,  sum0
                SMLAWB  sum1,     r8,     TAB,  sum1
                SMLAWT  sum0,     r8,     TAB,  sum0
                sub      sum1,  r10,     sum1,  asr  #5
                mov     sum0,  sum0,  asr  #5
                
                stmia   out_ptr!, {sum0-sum1}       
                
                subs  k,  k,  #1
                BGT  FILTER_LOOP1            
                         
                rsbs     kx,    kx,  #32
                BLE     FILTER_LOOP2_SKIP
                ands    r12,   kx,    #0x1
                subne    out_ptr, out_ptr,  #8      

                mov     r9, #0  
FILTER_LOOP2:
                stmia   out_ptr!, {r9-r10}
                @//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
                @// the loop control
                subs  kx, kx, #1               
                BGT  FILTER_LOOP2
FILTER_LOOP2_SKIP:
                ldmfd   sp!, {r4 - r10, pc} 
                
                @AREA SBR_SYNTHESIS_FILTER_TABLE_D, DATA, READONLY
                
ANLAYSIS_POST_TABLE:
                                .word    0x80007fff, 0x7fb18c90
                                .word    0x8c907fb1, 0x7ec49918
                                .word    0x99187ec4, 0x7d3ba590
                                .word    0xa5907d3b, 0x7b15b1f1
                                .word    0xb1f17b15, 0x7854be34
                                .word    0xbe347854, 0x74faca50
                                .word    0xca5074fa, 0x7109d63e
                                .word    0xd63e7109, 0x6c83e1f8
                                .word    0xe1f86c83, 0x676ced74
                                .word    0xed74676c, 0x61c6f8ad
                                .word    0xf8ad61c6, 0x5b94039c
                                .word    0x039c5b94, 0x54db0e3a
                                .word    0x0e3a54db, 0x4d9f1880
                                .word    0x18804d9f, 0x45e42268
                                .word    0x226845e4, 0x3daf2beb
                                .word    0x2beb3daf, 0x35053505

                
        
BIT_POS_TAB:
                                .byte     0,     31
                                .byte    16,     15
                                .byte     8,     23
                                .byte    24,      7
                                .byte     4,     27
                                .byte    20,     11
                                .byte    12,     19
                                .byte    28,      3
                                .byte     2,     29
                                .byte    18,     13
                                .byte    10,     21
                                .byte    26,      5
                                .byte     6,     25
                                .byte    22,      9
                                .byte    14,     17
                                .byte    30,      1

 
                                
                .end
                
                
                
                
                
                
                
                
                
                
                
                
                
                