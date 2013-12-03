@// SBR synthesis dct pre processing
@// author: reed.zhang
@// input: SBR synthesis filter bank processing
@//     r0: output sample pointer
@// r1: the input data
@// purpose:

@// define the register name 
k           .req      r14
tab_ptr     .req      r2
x_re0       .req      r3
x_im0       .req      r4
tab0        .req      r5
ptr127      .req      r6
sum0        .req      r7
sum1        .req      r12



        


                .text@AREA    PROGRAM, CODE, READONLY                         
                .arm@CODE32                          
                .global  asm_sbr_synthesis_dct_pre
asm_sbr_synthesis_dct_pre:
                stmfd   sp!, {r4 - r7, r14}                
                mov k, #32             
                ldr    tab_ptr, =DCT_PRE_TABLE
                add  ptr127,  r1, #0x1fc   @@//  127                
FILTER_LOOP1:
            @// the function body
            @//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
            ldr      tab0, [tab_ptr],  #4
            ldr      x_im0,  [ptr127,  #-4]
            ldr      x_re0,   [r1],  #4*4
            mov    sum1,   x_im0,  asr  #1
            sub     sum1,  sum1,   x_re0,   asr  #1  @@// (-(x_re0>>1) + (x_im0>>1))) >> 0@
            add     sum0,  sum1,   x_re0                @@// ( x_re0 - (x_re0>>1) + (x_im0>>1))) >> 0@            
            SMLAWT   sum0,   x_im0,   tab0,   sum0
            SMLAWB   sum1,   x_im0,   tab0,   sum1            
            SMLAWB   sum0,   x_re0,   tab0,   sum0
            rsb               x_re0,   x_re0,  #0
            SMLAWT   sum1,   x_re0,   tab0,   sum1            
            str                sum0,  [r0],  #4
            str                sum1,  [r0,  #32*4-4] 
            
            @@//////////////////////////////////////////////
            ldr      x_im0,   [r1,  #-3*4]
            ldr      x_re0,  [ptr127],  #-4*4
                        
            mov    sum1,   x_im0,  asr  #1
            sub     sum1,  sum1,   x_re0,   asr  #1  @@// (-(x_re0>>1) + (x_im0>>1))) >> 0@
            add     sum0,  sum1,   x_re0                @@// ( x_re0 - (x_re0>>1) + (x_im0>>1))) >> 0@            
            SMLAWT   sum0,   x_im0,   tab0,   sum0
            SMLAWB   sum1,   x_im0,   tab0,   sum1            
            SMLAWB   sum0,   x_re0,   tab0,   sum0
            rsb               x_re0,   x_re0,  #0
            SMLAWT   sum1,   x_re0,   tab0,   sum1            
            str                sum0,  [r0,  #64*4-4]
            str                sum1,  [r0,  #96*4-4]
            
            @//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
            ldr      tab0, [tab_ptr],  #4
            ldr      x_im0,  [ptr127,  #-4]
            ldr      x_re0,   [r1],  #4*4
            mov    sum1,   x_im0,  asr  #1
            sub     sum1,  sum1,   x_re0,   asr  #1  @@// (-(x_re0>>1) + (x_im0>>1))) >> 0@
            add     sum0,  sum1,   x_re0                @@// ( x_re0 - (x_re0>>1) + (x_im0>>1))) >> 0@            
            SMLAWT   sum0,   x_im0,   tab0,   sum0
            SMLAWB   sum1,   x_im0,   tab0,   sum1            
            SMLAWB   sum0,   x_re0,   tab0,   sum0
            rsb               x_re0,   x_re0,  #0
            SMLAWT   sum1,   x_re0,   tab0,   sum1            
            str                sum0,  [r0],  #4
            str                sum1,  [r0,  #32*4-4] 
            
            @@//////////////////////////////////////////////
            ldr      x_im0,   [r1,  #-3*4]
            ldr      x_re0,  [ptr127],  #-4*4
                        
            mov    sum1,   x_im0,  asr  #1
            sub     sum1,  sum1,   x_re0,   asr  #1  @@// (-(x_re0>>1) + (x_im0>>1))) >> 0@
            add     sum0,  sum1,   x_re0                @@// ( x_re0 - (x_re0>>1) + (x_im0>>1))) >> 0@            
            SMLAWT   sum0,   x_im0,   tab0,   sum0
            SMLAWB   sum1,   x_im0,   tab0,   sum1            
            SMLAWB   sum0,   x_re0,   tab0,   sum0
            rsb               x_re0,   x_re0,  #0
            SMLAWT   sum1,   x_re0,   tab0,   sum1            
            str                sum0,  [r0,  #64*4-4]
            str                sum1,  [r0,  #96*4-4]
            
            @// the loop control
            subs k, k, #2
            BGT FILTER_LOOP1
        
            ldmfd   sp!, {r4 - r7, pc} 
                
                
        @AREA SBR_SYNTHESIS_FILTER_TABLE_D, DATA, READONLY         
DCT_PRE_TABLE:
                .word    0x83247fff
                .word    0x8fb37f85
                .word    0x9c387e71
                .word    0xa8ab7cc0
                .word    0xb5057a73
                .word    0xc13f778c
                .word    0xcd50740c
                .word    0xd9326ff5
                .word    0xe4dd6b4b
                .word    0xf0496610
                .word    0xfb706046
                .word    0x064c59f2
                .word    0x10d45318
                .word    0x1b034bbc
                .word    0x24d243e2
                .word    0x2e3c3b8f
                .word    0x373a32c9
                .word    0x3fc72994
                .word    0x47de1ff7
                .word    0x4f7a15f7
                .word    0x56960b9a
                .word    0x5d2d00e8
                .word    0x633cf5e6
                .word    0x68bfea9b
                .word    0x6db3df0f
                .word    0x7213d348
                .word    0x75dfc74d
                .word    0x7913bb27
                .word    0x7badaedc
                .word    0x7daca274
                .word    0x7f0e95f7
                .word    0x7fd4896c


                .end@END