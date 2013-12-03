@ // SBR synthesis dct-post processing
@ // author: reed.zhang
@ // input: SBR synthesis filter bank processing
@ // r0: output data
@ // r1: input data
@ // purpose:

@ // define the register name 
out_ptr0    .req      r0
in_ptr      .req      r1

k           .req      r14
rev_ptr     .req      r12
tab_ptr     .req      r8

x_re0       .req      r2
x_im0       .req      r3
sum1        .req      r4
sum3        .req      r6
sum2        .req      r5
sum4        .req      r7

out_ptr1    .req      r9
tab_val     .req      r10

                .text@AREA    PROGRAM, CODE, READONLY                         
                .arm@CODE32                          
                .global  asm_sbr_synthesis_dct_post
asm_sbr_synthesis_dct_post:
                stmfd   sp!, {r4 - r11, r14}
                mov     k,  #32
                ldr     tab_ptr, =POST_DCT_TAB
                ldr     rev_ptr, =BIT_POS_TAB                
                add   out_ptr1, out_ptr0, #127*4 @@//
LOOP_POST:
                @// the function body
                ldrsh    sum4,  [rev_ptr],    #2
                ldr      tab_val,   [tab_ptr],  #4                
                add     sum2,   in_ptr,  sum4,  lsl  #2                
                ldr       x_re0,   [sum2]
                ldr       x_im0,   [sum2,  #32*4]                
                mov    sum1,     x_re0,    asr  #1
                add    sum1,     sum1,    x_im0, asr  #1  @@//((x_im0>>1) + (x_re0>>1)))@
                sub     sum3,     sum1,   x_re0                @@//((x_im1>>1) - (x_re1>>1)))@                
                SMLAWT   sum1,   x_im0,    tab_val,   sum1
                SMLAWB   sum3,   x_im0,    tab_val,   sum3                
                SMLAWB   sum1,   x_re0,    tab_val,   sum1
                rsb               x_re0,  x_re0,  #0
                SMLAWT   sum3,   x_re0,    tab_val,   sum3
                
                 ldr       x_re0,   [sum2,  #64*4]
                 ldr       x_im0,   [sum2,  #96*4]                
                mov    sum2,     x_re0,    asr  #1
                add    sum2,     sum2,    x_im0, asr  #1  @@//((x_im0>>1) + (x_re0>>1)))@
                sub     sum4,     sum2,   x_re0                @@//((x_im1>>1) - (x_re1>>1)))@                
                SMLAWT   sum2,   x_im0,    tab_val,   sum2
                SMLAWB   sum4,   x_im0,    tab_val,   sum4                
                SMLAWB   sum2,   x_re0,    tab_val,   sum2
                rsb               x_re0,  x_re0,  #0
                SMLAWT   sum4,   x_re0,    tab_val,   sum4                
                sub      x_re0,    sum2,  sum1
                sub      x_im0,    sum4,  sum3
                str       x_re0,     [out_ptr0],   #2*4
                str       x_im0,     [out_ptr0,  #64*4-2*4]                
                add      x_re0,    sum2,  sum1
                add      x_im0,    sum4,  sum3
                str       x_re0,     [out_ptr1],   #-2*4
                str       x_im0,     [out_ptr1,  #(-64+2)*4]
                
                
                @// the function body
                ldrsh    sum4,  [rev_ptr],    #2
                ldr      tab_val,   [tab_ptr],  #4                
                add     sum2,   in_ptr,  sum4,  lsl  #2                
                ldr       x_re0,   [sum2]
                ldr       x_im0,   [sum2,  #32*4]                
                mov    sum1,     x_re0,    asr  #1
                add    sum1,     sum1,    x_im0, asr  #1  @@//((x_im0>>1) + (x_re0>>1)))@
                sub     sum3,     sum1,   x_re0                @@//((x_im1>>1) - (x_re1>>1)))@                
                SMLAWT   sum1,   x_im0,    tab_val,   sum1
                SMLAWB   sum3,   x_im0,    tab_val,   sum3                
                SMLAWB   sum1,   x_re0,    tab_val,   sum1
                rsb               x_re0,  x_re0,  #0
                SMLAWT   sum3,   x_re0,    tab_val,   sum3
                
                 ldr       x_re0,   [sum2,  #64*4]
                 ldr       x_im0,   [sum2,  #96*4]                
                mov    sum2,     x_re0,    asr  #1
                add    sum2,     sum2,    x_im0, asr  #1  @@//((x_im0>>1) + (x_re0>>1)))@
                sub     sum4,     sum2,   x_re0                @@//((x_im1>>1) - (x_re1>>1)))@                
                SMLAWT   sum2,   x_im0,    tab_val,   sum2
                SMLAWB   sum4,   x_im0,    tab_val,   sum4                
                SMLAWB   sum2,   x_re0,    tab_val,   sum2
                rsb               x_re0,  x_re0,  #0
                SMLAWT   sum4,   x_re0,    tab_val,   sum4                
                sub      x_re0,    sum2,  sum1
                sub      x_im0,    sum4,  sum3
                str       x_re0,     [out_ptr0],   #2*4
                str       x_im0,     [out_ptr0,  #64*4-2*4]                
                add      x_re0,    sum2,  sum1
                add      x_im0,    sum4,  sum3
                str       x_re0,     [out_ptr1],   #-2*4
                str       x_im0,     [out_ptr1,  #(-64+2)*4]
                
                
                
                @// the function body
                ldrsh    sum4,  [rev_ptr],    #2
                ldr      tab_val,   [tab_ptr],  #4                
                add     sum2,   in_ptr,  sum4,  lsl  #2                
                ldr       x_re0,   [sum2]
                ldr       x_im0,   [sum2,  #32*4]                
                mov    sum1,     x_re0,    asr  #1
                add    sum1,     sum1,    x_im0, asr  #1  @@//((x_im0>>1) + (x_re0>>1)))@
                sub     sum3,     sum1,   x_re0                @@//((x_im1>>1) - (x_re1>>1)))@                
                SMLAWT   sum1,   x_im0,    tab_val,   sum1
                SMLAWB   sum3,   x_im0,    tab_val,   sum3                
                SMLAWB   sum1,   x_re0,    tab_val,   sum1
                rsb               x_re0,  x_re0,  #0
                SMLAWT   sum3,   x_re0,    tab_val,   sum3
                
                 ldr       x_re0,   [sum2,  #64*4]
                 ldr       x_im0,   [sum2,  #96*4]                
                mov    sum2,     x_re0,    asr  #1
                add    sum2,     sum2,    x_im0, asr  #1  @@//((x_im0>>1) + (x_re0>>1)))@
                sub     sum4,     sum2,   x_re0                @@//((x_im1>>1) - (x_re1>>1)))@                
                SMLAWT   sum2,   x_im0,    tab_val,   sum2
                SMLAWB   sum4,   x_im0,    tab_val,   sum4                
                SMLAWB   sum2,   x_re0,    tab_val,   sum2
                rsb               x_re0,  x_re0,  #0
                SMLAWT   sum4,   x_re0,    tab_val,   sum4                
                sub      x_re0,    sum2,  sum1
                sub      x_im0,    sum4,  sum3
                str       x_re0,     [out_ptr0],   #2*4
                str       x_im0,     [out_ptr0,  #64*4-2*4]                
                add      x_re0,    sum2,  sum1
                add      x_im0,    sum4,  sum3
                str       x_re0,     [out_ptr1],   #-2*4
                str       x_im0,     [out_ptr1,  #(-64+2)*4]
                
                
                
                @// the function body
                ldrsh    sum4,  [rev_ptr],    #2
                ldr      tab_val,   [tab_ptr],  #4                
                add     sum2,   in_ptr,  sum4,  lsl  #2                
                ldr       x_re0,   [sum2]
                ldr       x_im0,   [sum2,  #32*4]                
                mov    sum1,     x_re0,    asr  #1
                add    sum1,     sum1,    x_im0, asr  #1  @@//((x_im0>>1) + (x_re0>>1)))@
                sub     sum3,     sum1,   x_re0                @@//((x_im1>>1) - (x_re1>>1)))@                
                SMLAWT   sum1,   x_im0,    tab_val,   sum1
                SMLAWB   sum3,   x_im0,    tab_val,   sum3                
                SMLAWB   sum1,   x_re0,    tab_val,   sum1
                rsb               x_re0,  x_re0,  #0
                SMLAWT   sum3,   x_re0,    tab_val,   sum3
                
                 ldr       x_re0,   [sum2,  #64*4]
                 ldr       x_im0,   [sum2,  #96*4]                
                mov    sum2,     x_re0,    asr  #1
                add    sum2,     sum2,    x_im0, asr  #1  @@//((x_im0>>1) + (x_re0>>1)))@
                sub     sum4,     sum2,   x_re0                @@//((x_im1>>1) - (x_re1>>1)))@                
                SMLAWT   sum2,   x_im0,    tab_val,   sum2
                SMLAWB   sum4,   x_im0,    tab_val,   sum4                
                SMLAWB   sum2,   x_re0,    tab_val,   sum2
                rsb               x_re0,  x_re0,  #0
                SMLAWT   sum4,   x_re0,    tab_val,   sum4                
                sub      x_re0,    sum2,  sum1
                sub      x_im0,    sum4,  sum3
                str       x_re0,     [out_ptr0],   #2*4
                str       x_im0,     [out_ptr0,  #64*4-2*4]                
                add      x_re0,    sum2,  sum1
                add      x_im0,    sum4,  sum3
                str       x_re0,     [out_ptr1],   #-2*4
                str       x_im0,     [out_ptr1,  #(-64+2)*4]
                @//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
                @// the loop .
                subs  k, k, #4
                BGT LOOP_POST
                
                ldmfd   sp!, {r4 - r11, pc}
                
                @AREA SBR_SYNTHESIS_FILTER_TABLE_D, DATA, READONLY

POST_DCT_TAB:
                .word       0x80007fff
                .word       0x8c907fb1
                .word       0x99187ec4
                .word       0xa5907d3b
                .word       0xb1f17b15
                .word       0xbe347854
                .word       0xca5074fa
                .word       0xd63e7109
                .word       0xe1f86c83
                .word       0xed74676c
                .word       0xf8ad61c6
                .word       0x039c5b94
                .word       0x0e3a54db
                .word       0x18804d9f
                .word       0x226845e4
                .word       0x2beb3daf
                .word       0x35053505
                .word       0x3daf2beb
                .word       0x45e42268
                .word       0x4d9f1880
                .word       0x54db0e3a
                .word       0x5b94039c
                .word       0x61c6f8ad
                .word       0x676ced74
                .word       0x6c83e1f8
                .word       0x7109d63e
                .word       0x74faca50
                .word       0x7854be34
                .word       0x7b15b1f1
                .word       0x7d3ba590
                .word       0x7ec49918
                .word       0x7fb18c90
 

BIT_POS_TAB:
		.hword   0 
                                .hword   16
                                .hword   8 
                                .hword   24
                                .hword   4 
                                .hword   20
                                .hword   12
                                .hword   28
                                .hword   2 
                                .hword   18
                                .hword   10
                                .hword   26
                                .hword   6 
                                .hword   22
                                .hword   14
                                .hword   30
                                .hword   1 
                                .hword   17
                                .hword   9 
                                .hword   25
                                .hword   5 
                                .hword   21
                                .hword   13
                                .hword   29
                                .hword   3 
                                .hword   19
                                .hword   11
                                .hword   27
                                .hword   7 
                                .hword   23
                                .hword   15
                                .hword   31




                .end@END