@// author: reed.zhang

	

                @ ***** END LICENSE BLOCK *****  

	.text  @AREA	|.text|, CODE, READONLY
	.arm
                @//int32 AAC_DEC_ARM_OnlyLongBlockProcessingAsm(
                @//                              int32   *in_data_ptr,
                @//                              int32   *overlap_ptr
                @//                              int16   *pcm_out_ptr,  
                @//                              int32   *table_addr_ptr              
                @//                              )@
                

                @@ // r0, in_out_ptr
                @@ // r1, table_ptr
in_data_ptr512  .req  r0
overlap_ptr0    .req  r1
pcm_out_ptr0    .req  r2
table_addr_ptr0 .req  r3

overlap_ptr1024 .req  r4
pcm_out_ptr1024 .req  r5
win_ptr1024     .req  r6
in_data_ptr0    .req  r7
win_ptr0        .req  r3


AAC_DEC_ARM_OnlyLongBlockProcessingAsm: @   FUNCTION
                @ // save the value for the following calculation
                .global  AAC_DEC_ARM_OnlyLongBlockProcessingAsm
                stmfd   sp!, {r4-r12, r14}
                ldr     r12, =0x00007FFF                
                ldmia   r3, {win_ptr0, r6}                
                stmfd   sp!, {r6}                
                
                add in_data_ptr512, in_data_ptr512,  #512*4
                add in_data_ptr512, in_data_ptr512,  #4
                sub in_data_ptr0, in_data_ptr512,    #3*4
               
                add win_ptr1024, win_ptr0,           #1024*4
                sub win_ptr1024, win_ptr1024,        #4
                
                add overlap_ptr1024, overlap_ptr0,   #1024*4
                sub overlap_ptr1024, overlap_ptr1024,#4
                
                add pcm_out_ptr1024, pcm_out_ptr0,   #1024*2
                sub pcm_out_ptr1024, pcm_out_ptr1024,#2               
                
                mov   r14,  #256                
AAC_DEC_ARM_OnlyLongBlockProcessingAsm_PCM_LOOP:
                @@///////////////////////////////////////////////////
                @@/* r8, r9, r10, r11 */
                ldr   r8, [in_data_ptr512], #2*4  @@// get 513, 515, .....
                
                @@// do pcm 0
                ldr   r9, [win_ptr0], #4      @@// load the 0 win coef
                ldr   r11, [overlap_ptr0], #4 @@// load the overlap 0
                SMULL r9,  r10,   r8,  r9     @@// window and get the r10                
                add   r11,   r11,   #2048       
                ldr   r9, [win_ptr1024], #-4  @@// load the 1023 win coef                
                add   r11,   r10,   r11       @@// add overlap
                         
                mov   r11,  r11, asr  #12     @@// right shift 12 bit
                                
                mov    r10,  r11, asr #15
                teq    r10,  r11, asr #31
                EORNE  r11,  r12, r11, asr #31	
                strh   r11,  [pcm_out_ptr0], #2                
                @@// do pcm 1023
                SMULL r9,  r10,   r8,  r9         @@// window and get the r10
                ldr   r11, [overlap_ptr1024], #-4 @@// load the overlap 1023
                
                ldr   r8, [in_data_ptr0], #-2*4  @@// get 510, 508, .....
                
                sub   r11,  r11,  r10
                add   r11,  r11,  #2048
                mov   r11,  r11,  asr  #12     @@// right shift 12 bit
                mov    r10,  r11, asr #15
                teq    r10,  r11, asr #31
                EORNE  r11,  r12, r11, asr #31	
                strh   r11,  [pcm_out_ptr1024], #-2    
                @@///////////////////////////////////////
                @@// do pcm 1
                ldr   r9, [win_ptr0], #4      @@// load the 1 win coef
                ldr   r11, [overlap_ptr0], #4 @@// load the overlap 1
                SMULL r9,  r10,   r8,  r9     @@// window and get the r10                
                add   r11,   r11,   #2048       
                ldr   r9, [win_ptr1024], #-4  @@// load the 1022 win coef                
                sub   r11,   r11,   r10       @@// add overlap
                         
                mov   r11,  r11, asr  #12     @@// right shift 12 bit
                                
                mov    r10,  r11, asr #15
                teq    r10,  r11, asr #31
                EORNE  r11,  r12, r11, asr #31	
                strh   r11,  [pcm_out_ptr0], #2                
                @@// do pcm 1022
                SMULL r9,  r10,   r8,  r9         @@// window and get the r10 
                
                ldr   r11, [overlap_ptr1024], #-4 @@// load the overlap 1022
                
                add   r10,  r10,  #2048
                add   r11,  r11,  r10
                
                mov   r11,  r11, asr  #12     @@// right shift 12 bit
                mov    r10,  r11, asr #15
                teq    r10,  r11, asr #31
                EORNE  r11,  r12, r11, asr #31	
                strh   r11,  [pcm_out_ptr1024], #-2                    
                
                
                @@//////////////////////////////////////////////////////////
                @@/* r8, r9, r10, r11 */
                ldr   r8, [in_data_ptr512], #2*4  @@// get 513, 515, .....                
                @@// do pcm 0
                ldr   r9, [win_ptr0], #4      @@// load the 0 win coef
                ldr   r11, [overlap_ptr0], #4 @@// load the overlap 0
                SMULL r9,  r10,   r8,  r9     @@// window and get the r10                
                add   r11,   r11,   #2048       
                ldr   r9, [win_ptr1024], #-4  @@// load the 1023 win coef                
                add   r11,   r10,   r11       @@// add overlap
                         
                mov   r11,  r11, asr  #12     @@// right shift 12 bit
                                
                mov    r10,  r11, asr #15
                teq    r10,  r11, asr #31
                EORNE  r11,  r12, r11, asr #31	
                strh   r11,  [pcm_out_ptr0], #2                
                @@// do pcm 1023
                SMULL r9,  r10,   r8,  r9         @@// window and get the r10          
                ldr   r11, [overlap_ptr1024], #-4 @@// load the overlap 1023
                
                ldr   r8, [in_data_ptr0], #-2*4  @@// get 510, 508, .....
                
                sub   r11,  r11,  r10
                add   r11,  r11,  #2048
                mov   r11,  r11, asr  #12     @@// right shift 12 bit
                mov    r10,  r11, asr #15
                teq    r10,  r11, asr #31
                EORNE  r11,  r12, r11, asr #31	
                strh   r11,  [pcm_out_ptr1024], #-2    
                @@///////////////////////////////////////
                @@// do pcm 1
                ldr   r9, [win_ptr0], #4      @@// load the 1 win coef
                ldr   r11, [overlap_ptr0], #4 @@// load the overlap 1
                SMULL r9,  r10,   r8,  r9     @@// window and get the r10                
                add   r11,   r11,   #2048       
                ldr   r9, [win_ptr1024], #-4  @@// load the 1022 win coef                
                sub   r11,   r11,   r10       @@// add overlap
                         
                mov   r11,  r11, asr  #12     @@// right shift 12 bit
                                
                mov    r10,  r11, asr #15
                teq    r10,  r11, asr #31
                EORNE  r11,  r12, r11, asr #31	
                strh   r11,  [pcm_out_ptr0], #2                
                @@// do pcm 1022
                SMULL r9,  r10,   r8,  r9         @@// window and get the r10 
                
                ldr   r11, [overlap_ptr1024], #-4 @@// load the overlap 1022
                
                add   r10,  r10,  #2048
                add   r11,  r11,  r10
                
                mov   r11,  r11, asr  #12     @@// right shift 12 bit
                mov    r10,  r11, asr #15
                teq    r10,  r11, asr #31
                EORNE  r11,  r12, r11, asr #31	
                strh   r11,  [pcm_out_ptr1024], #-2
                                
                @@/////////////////
                @@///////////////////////////////////////////////////
                subs   r14,  r14,  #2
                BGT    AAC_DEC_ARM_OnlyLongBlockProcessingAsm_PCM_LOOP
                
                
                
                ldmfd   sp!, {r6}
                
                mov   r14,  #256 
                
                add     win_ptr1024,   r6,  #512*4
                sub     win_ptr0,      win_ptr1024,  #4
                
                add     in_data_ptr0,  in_data_ptr0, #3*4
                sub     in_data_ptr512, in_data_ptr512, #3*4
                               
AAC_DEC_ARM_OnlyLongBlockProcessingAsm_OVLERLAP_LOOP:
                @@///////////////////////////////////////////////////
                @@/* r2, r5,  r8, r9, r10, r11, r12 */
                ldr  r2,  [in_data_ptr0],   #2*4    @@// load  1, 3, 5,7 
                ldr  r5,  [in_data_ptr512], #-2*4   @@// load  1022, 1020, 1018, 1016 
                ldr  r8,  [win_ptr0], #-4
                rsb  r2,  r2,  #0  @@// 
                
                SMULL r8,  r9,  r2,  r8
                ldr   r12, [win_ptr1024], #4
                str   r9,  [overlap_ptr0], #4
                
                SMULL r12,  r10,  r2,  r12
                ldr   r8, [win_ptr0], #-4       @@// 
                str   r10,  [overlap_ptr1024], #-4
                
                SMULL r8,  r12,  r5,   r8
                ldr   r2,  [win_ptr1024], #4
                str   r12, [overlap_ptr0], #4                
                SMULL r2,  r8,  r5,   r2               
                
                
                @@/* r2, r5,  r8, r9, r10, r11, r12 */
                ldr  r10,  [in_data_ptr0],   #2*4    @@// load  1, 3, 5,7 
                
                str   r8,  [overlap_ptr1024], #-4
                
                ldr  r5,  [in_data_ptr512], #-2*4   @@// load  1022, 1020, 1018, 1016 
                ldr  r8,  [win_ptr0], #-4
                rsb  r10,  r10,  #0  @@// 
                
                SMULL r8,  r9,  r10,  r8
                ldr   r12, [win_ptr1024], #4
                str   r9,  [overlap_ptr0], #4
                SMULL r12,  r11,  r10,  r12
                ldr   r8, [win_ptr0], #-4       @@// 
                str   r11,  [overlap_ptr1024], #-4
                
                SMULL r8,  r12,  r5,   r8
                ldr   r10,  [win_ptr1024], #4
                str   r12, [overlap_ptr0], #4                
                SMULL r10,  r8,  r5,   r10                
                
                
                @@/* r2, r5,  r8, r9, r10, r11, r12 */
                ldr  r2,  [in_data_ptr0],   #2*4    @@// load  1, 3, 5,7 
                str   r8,  [overlap_ptr1024], #-4
                
                ldr  r5,  [in_data_ptr512], #-2*4   @@// load  1022, 1020, 1018, 1016 
                ldr  r8,  [win_ptr0], #-4
                rsb  r2,  r2,  #0  @@// 
                
                SMULL r8,  r9,  r2,  r8
                ldr   r12, [win_ptr1024], #4
                str   r9,  [overlap_ptr0], #4
                SMULL r12,  r9,  r2,  r12
                ldr   r8, [win_ptr0], #-4       @@// 
                str   r9,  [overlap_ptr1024], #-4
                
                SMULL r8,  r12,  r5,   r8
                ldr   r2,  [win_ptr1024], #4
                str   r12, [overlap_ptr0], #4                
                SMULL r2,  r8,  r5,   r2
                                
                @@/* r2, r5,  r8, r9, r10, r11, r12 */
                ldr  r10,  [in_data_ptr0],   #2*4    @@// load  1, 3, 5,7
                str   r8,  [overlap_ptr1024], #-4
                 
                ldr  r5,  [in_data_ptr512], #-2*4   @@// load  1022, 1020, 1018, 1016 
                ldr  r8,  [win_ptr0], #-4
                rsb  r10,  r10,  #0  @@// 
                
                SMULL r8,  r9,  r10,  r8
                ldr   r12, [win_ptr1024], #4
                str   r9,  [overlap_ptr0], #4
                SMULL r12,  r11,  r10,  r12
                ldr   r8, [win_ptr0], #-4       @@// 
                str   r11,  [overlap_ptr1024], #-4
                
                SMULL r8,  r12,  r5,   r8
                ldr   r10,  [win_ptr1024], #4
                str   r12, [overlap_ptr0], #4                
                SMULL r10,  r8,  r5,   r10
                
                subs   r14,  r14,  #4
                str   r8,  [overlap_ptr1024], #-4
                @@///////////////////////////////////////////////////
                
                BGT    AAC_DEC_ARM_OnlyLongBlockProcessingAsm_OVLERLAP_LOOP                
                
                ldmfd   sp!, {r4-r12, pc}
                @ENDFUNC
                @//int32 AAC_DEC_ARM_StartLongBlockProcessingAsm(
                @//                              int32   *in_data_ptr,
                @//                              int32   *overlap_ptr
                @//                              int16   *pcm_out_ptr,  
                @//                              int32   *table_addr_ptr              
                @//                              )@
                

                @@ // r0, in_out_ptr
                @@ // r1, table_ptr
in_data_ptr512     .req  r0
overlap_ptr0    .req  r1
pcm_out_ptr0    .req  r2
table_addr_ptr0 .req  r3

overlap_ptr1024 .req  r4
pcm_out_ptr1024 .req  r5
win_ptr1024     .req  r6
in_data_ptr0    .req  r7
win_ptr0        .req  r3


AAC_DEC_ARM_StartLongBlockProcessingAsm:@   FUNCTION
                @ // save the value for the following calculation
                .global  AAC_DEC_ARM_StartLongBlockProcessingAsm
                stmfd   sp!, {r4-r12, r14}
                ldr     r12, =0x00007FFF
                
                ldmia   r3, {win_ptr0, r6}
                
                stmfd   sp!, {r6}
                
                
                add in_data_ptr512, in_data_ptr512, #512*4
                add in_data_ptr512, in_data_ptr512,  #4
                sub in_data_ptr0, in_data_ptr512, #3*4
               
                add win_ptr1024, win_ptr0, #1024*4
                sub win_ptr1024, win_ptr1024, #4
                
                add overlap_ptr1024, overlap_ptr0, #1024*4
                sub overlap_ptr1024, overlap_ptr1024, #4
                
                add pcm_out_ptr1024, pcm_out_ptr0, #1024*2
                sub pcm_out_ptr1024, pcm_out_ptr1024, #2
                
                
                mov   r14,  #256                
AAC_DEC_ARM_StartLongBlockProcessingAsm_PCM_LOOP:
                @@///////////////////////////////////////////////////
                @@/* r8, r9, r10, r11 */
                ldr   r8, [in_data_ptr512], #2*4  @@// get 513, 515, .....
                
                @@// do pcm 0                
                ldr   r11, [overlap_ptr0], #4 @@// load the overlap 0
                ldr   r9, [win_ptr0], #4      @@// load the 0 win coef
                add   r11,  r11,  #2048                
                SMLAWT r11,  r8,   r9, r11    @@// window and get the r10   

                ldr   r9, [win_ptr1024], #-4  @@// load the 1023 win coef                
                         
                mov   r11,  r11, asr  #12     @@// right shift 12 bit                               
                mov    r10,  r11, asr #15
                teq    r10,  r11, asr #31
                EORNE  r11,  r12, r11, asr #31	
                strh   r11,  [pcm_out_ptr0], #2                
                @@// do pcm 1023
                
                ldr    r11, [overlap_ptr1024], #-4 @@// load the overlap 1023
                rsb    r8,   r8,  #0
                add    r11,  r11,  #2048
                SMLAWT r11,  r8,   r9,  r11         @@// window and get the r10          
                                
                ldr   r8, [in_data_ptr0], #-2*4  @@// get 510, 508, .....                
                mov   r11,  r11, asr  #12     @@// right shift 12 bit
                mov    r10,  r11, asr #15
                teq    r10,  r11, asr #31
                EORNE  r11,  r12, r11, asr #31	
                strh   r11,  [pcm_out_ptr1024], #-2    
                @@///////////////////////////////////////
                @@// do pcm 1
                ldr    r11, [overlap_ptr0], #4 @@// load the overlap 1
                ldr    r9, [win_ptr0], #4      @@// load the 1 win coef
                add    r11,  r11,  #2048  
                rsb    r8,  r8,    #0              
                SMLAWT r11,  r8,   r9,  r11     @@// window and get the r10                
                
                ldr   r9, [win_ptr1024], #-4  @@// load the 1022 win coef                                
                         
                mov   r11,  r11, asr  #12     @@// right shift 12 bit                                
                mov    r10,  r11, asr #15
                teq    r10,  r11, asr #31
                EORNE  r11,  r12, r11, asr #31	
                strh   r11,  [pcm_out_ptr0], #2  
                
                ldr   r10, [overlap_ptr1024], #-4 @@// load the overlap 1022              
                @@// do pcm 1022
                mov    r11,  #2048
                rsb    r8,   r8,  #0
                SMLAWT r11,  r8,   r9,  r11         @@// window and get the r10 
                add    r11,  r11,  r10
                
                mov   r11,  r11, asr  #12     @@// right shift 12 bit
                mov    r10,  r11, asr #15
                teq    r10,  r11, asr #31
                EORNE  r11,  r12, r11, asr #31	
                strh   r11,  [pcm_out_ptr1024], #-2  
                                
                @@/////////////////
                @@///////////////////////////////////////////////////
                subs   r14,  r14,  #1
                BGT    AAC_DEC_ARM_StartLongBlockProcessingAsm_PCM_LOOP
                
                
                @@/* overlap */
                ldmfd   sp!, {r6}
                
                mov   r14,  #32 
                
                add     win_ptr1024,   r6,  #64*2
                sub     win_ptr0,      win_ptr1024,  #4
                
                add     in_data_ptr0,  in_data_ptr0, #3*4
                sub     in_data_ptr512, in_data_ptr512, #3*4
                               
AAC_DEC_ARM_StartLongBlockProcessingAsm_OVLERLAP_LOOP:
                @@///////////////////////////////////////////////////
                @@/* r2, r5,  r8, r9, r10, r11, r12 */
                ldr  r2,  [in_data_ptr0],   #2*4    @@// load  1, 3, 5,7 
                ldr  r5,  [in_data_ptr512], #-2*4   @@// load  1022, 1020, 1018, 1016 
                ldr  r8,  [win_ptr0], #-4
                ldr  r12, [win_ptr1024], #4
                rsb  r2,  r2,  #0  @@//                                 
                SMULWB  r9,  r2,  r8
                SMULWT  r10, r2,  r12
                
                str   r9,  [overlap_ptr0], #4  
                str   r10, [overlap_ptr1024], #-4
                
                SMULWT r9,  r5,   r8
                SMULWB r10, r5,   r12              
                
                str   r9,  [overlap_ptr0], #4  
                str   r10, [overlap_ptr1024], #-4
                @@/* r2, r5,  r8, r9, r10, r11, r12 */
                ldr  r2,  [in_data_ptr0],   #2*4    @@// load  1, 3, 5,7 
                ldr  r5,  [in_data_ptr512], #-2*4   @@// load  1022, 1020, 1018, 1016 
                ldr  r8,  [win_ptr0], #-4
                ldr  r12, [win_ptr1024], #4
                rsb  r2,  r2,  #0  @@//                                 
                SMULWB  r9,  r2,  r8
                SMULWT  r10, r2,  r12
                
                str   r9,  [overlap_ptr0], #4  
                str   r10, [overlap_ptr1024], #-4
                
                SMULWT r9,  r5,   r8
                SMULWB r10, r5,   r12              
                
                str   r9,  [overlap_ptr0], #4  
                str   r10, [overlap_ptr1024], #-4
                subs   r14,  r14,  #2
                @@///////////////////////////////////////////////////
                
                BGT    AAC_DEC_ARM_StartLongBlockProcessingAsm_OVLERLAP_LOOP                
                
                
                mov   r14,  #224 
                mov   r11,  #0
                mov   r12,  #0
                mov   r9 ,  #0
                mov   r10,  #0
AAC_DEC_ARM_StartLongBlockProcessingAsm_OVLERLAP_LOOP1:
                @@///////////////////////////////////////////////////
                @@/* r2, r5,  r8, r9, r10, r11, r12 */
                ldr  r2,  [in_data_ptr0],   #2*4    @@// load  1, 3, 5,7 
                ldr  r5,  [in_data_ptr512], #-2*4   @@// load  1022, 1020, 1018, 1016                                 
                rsb  r2,  r2,  r2, asr #1                
                mov  r5,  r5,  asr #1                                
                str   r2, [overlap_ptr1024], #-4                
                str   r5, [overlap_ptr1024], #-4                
                @@/* r2, r5,  r8, r9, r10, r11, r12 */
                ldr  r2,  [in_data_ptr0],   #2*4    @@// load  1, 3, 5,7 
                ldr  r5,  [in_data_ptr512], #-2*4   @@// load  1022, 1020, 1018, 1016                                 
                rsb  r2,  r2,  r2, asr #1                
                mov  r5,  r5,  asr #1                                
                str   r2, [overlap_ptr1024], #-4                
                str   r5, [overlap_ptr1024], #-4                
                stmia overlap_ptr0!, {r9-r12}
                
                
                
                @@///////////////////////////////////////////////////
                @@/* r2, r5,  r8, r9, r10, r11, r12 */
                ldr  r2,  [in_data_ptr0],   #2*4    @@// load  1, 3, 5,7 
                ldr  r5,  [in_data_ptr512], #-2*4   @@// load  1022, 1020, 1018, 1016                                 
                rsb  r2,  r2,  r2, asr #1                
                mov  r5,  r5,  asr #1                                
                str   r2, [overlap_ptr1024], #-4                
                str   r5, [overlap_ptr1024], #-4                
                @@/* r2, r5,  r8, r9, r10, r11, r12 */
                ldr  r2,  [in_data_ptr0],   #2*4    @@// load  1, 3, 5,7 
                ldr  r5,  [in_data_ptr512], #-2*4   @@// load  1022, 1020, 1018, 1016                                 
                rsb  r2,  r2,  r2, asr #1                
                mov  r5,  r5,  asr #1                                
                str   r2, [overlap_ptr1024], #-4                
                str   r5, [overlap_ptr1024], #-4                
                stmia overlap_ptr0!, {r9-r12}
                subs   r14,  r14,  #4
                @@///////////////////////////////////////////////////
                
                BGT    AAC_DEC_ARM_StartLongBlockProcessingAsm_OVLERLAP_LOOP1                   
                
                
                
                ldmfd   sp!, {r4-r12, pc}
                @ENDFUNC
                
                
                
                @//int32 AAC_DEC_ARM_StopLongBlockProcessingAsm(
                @//                              int32   *in_data_ptr,
                @//                              int32   *overlap_ptr,
                @//                              int16   *pcm_out_ptr,  
                @//                              int32   *table_addr_ptr              
                @//                              )@
                

                @@ // r0, in_out_ptr
                @@ // r1, table_ptr
in_data_ptr512     .req  r0
overlap_ptr0    .req  r1
pcm_out_ptr0    .req  r2
table_addr_ptr0 .req  r3

overlap_ptr1024 .req  r4
pcm_out_ptr1024 .req  r5
win_ptr1024     .req  r6
in_data_ptr0    .req  r7
win_ptr0        .req  r3


AAC_DEC_ARM_StopLongBlockProcessingAsm:@   FUNCTION
                @ // save the value for the following calculation
                .global  AAC_DEC_ARM_StopLongBlockProcessingAsm
                stmfd   sp!, {r4-r12, r14}
                ldr     r12, =0x00007FFF
                
                ldmia   r3, {win_ptr0, r6}
                
                stmfd   sp!, {r6}
                
                
                add in_data_ptr512, in_data_ptr512, #512*4
                add in_data_ptr512, in_data_ptr512,  #4
                sub in_data_ptr0, in_data_ptr512, #3*4
               
                add win_ptr1024, win_ptr0,    #128*2
                sub win_ptr1024, win_ptr1024, #4
                
                add overlap_ptr1024, overlap_ptr0, #1024*4
                sub overlap_ptr1024, overlap_ptr1024, #4
                
                add pcm_out_ptr1024, pcm_out_ptr0, #1024*2
                sub pcm_out_ptr1024, pcm_out_ptr1024, #2                
                
                mov   r14,  #224      
AAC_DEC_ARM_StopLongBlockProcessingAsm_PCM_LOOP1:
                @@///////////////////////////////////////////////////
                @@/* r8, r9, r10, r11 */     
                @@// do pcm 0                
                ldr   r11, [overlap_ptr0], #4 @@// load the overlap 0                       
                ldr   r8, [in_data_ptr512], #2*4  @@// get 513, 515, .....                
                add   r11,   r11,  #2048                         
                mov    r11,  r11, asr #12     @@// right shift 12 bit                                
                mov    r10,  r11, asr #15
                teq    r10,  r11, asr #31
                EORNE  r11,  r12, r11, asr #31	
                strh   r11,  [pcm_out_ptr0], #2                
                @@// do pcm 1023
                ldr    r11, [overlap_ptr1024], #-4 @@// load the overlap 1023                
                rsb    r8,   r8,   #0
                add    r11,  r11,  #2048
                add    r11,  r11, r8, asr #1               
                mov    r11,  r11, asr  #12     @@// right shift 12 bit
                mov    r8,  r11, asr #15
                teq    r8,  r11, asr #31
                EORNE  r11,  r12, r11, asr #31	                                   
                strh   r11,  [pcm_out_ptr1024], #-2    
                @@///////////////////////////////////////                
                ldr    r11, [overlap_ptr0], #4 @@// load the overlap 1
                ldr    r8, [in_data_ptr0], #-2*4  @@// get 510, 508, .....                       
                add    r11, r11,  #2048
                                         
                mov    r11,  r11, asr  #12     @@// right shift 12 bit                                
                mov    r9,  r11, asr #15
                teq    r9,  r11, asr #31
                EORNE  r11,  r12, r11, asr #31	
                ldr    r10, [overlap_ptr1024], #-4 @@// load the overlap 1022           
                strh   r11,  [pcm_out_ptr0], #2                
                @@// do pcm 1022                
                add    r8,  r10,   r8,  asr #1
                add    r8,  r8, #2048 
                mov    r8,   r8, asr  #12     @@// right shift 12 bit
                mov    r10,  r8, asr #15
                teq    r10,  r8, asr #31
                EORNE  r8,  r12, r8, asr #31	
                strh   r8,  [pcm_out_ptr1024], #-2    
                @@/////////////////
                
                @@/* r8, r9, r10, r11 */     
                @@// do pcm 0                
                ldr   r11, [overlap_ptr0], #4 @@// load the overlap 0                       
                ldr   r8, [in_data_ptr512], #2*4  @@// get 513, 515, .....                
                add   r11,   r11,  #2048                         
                mov    r11,  r11, asr #12     @@// right shift 12 bit                                
                mov    r10,  r11, asr #15
                teq    r10,  r11, asr #31
                EORNE  r11,  r12, r11, asr #31	
                strh   r11,  [pcm_out_ptr0], #2                
                @@// do pcm 1023
                ldr    r11, [overlap_ptr1024], #-4 @@// load the overlap 1023                
                rsb    r8,   r8,   #0
                add    r11,  r11,  #2048
                add    r11,  r11, r8, asr #1               
                mov    r11,  r11, asr  #12     @@// right shift 12 bit
                mov    r8,  r11, asr #15
                teq    r8,  r11, asr #31
                EORNE  r11,  r12, r11, asr #31	                                   
                strh   r11,  [pcm_out_ptr1024], #-2    
                @@///////////////////////////////////////                
                ldr    r11, [overlap_ptr0], #4 @@// load the overlap 1
                ldr    r8, [in_data_ptr0], #-2*4  @@// get 510, 508, .....                       
                add    r11, r11,  #2048
                                         
                mov    r11,  r11, asr  #12     @@// right shift 12 bit                                
                mov    r9,  r11, asr #15
                teq    r9,  r11, asr #31
                EORNE  r11,  r12, r11, asr #31	
                ldr    r10, [overlap_ptr1024], #-4 @@// load the overlap 1022           
                strh   r11,  [pcm_out_ptr0], #2                
                @@// do pcm 1022                
                add    r8,  r10,   r8,  asr #1
                add    r8,  r8, #2048 
                mov    r8,   r8, asr  #12     @@// right shift 12 bit
                mov    r10,  r8, asr #15
                teq    r10,  r8, asr #31
                EORNE  r8,  r12, r8, asr #31	
                strh   r8,  [pcm_out_ptr1024], #-2    
                @@///////////////////////////////////////////////////
                subs   r14,  r14,  #2
                BGT    AAC_DEC_ARM_StopLongBlockProcessingAsm_PCM_LOOP1                       
                
                
                
                
                
                
                
                
                
                mov   r14,  #32               
AAC_DEC_ARM_StopLongBlockProcessingAsm_PCM_LOOP:
                @@///////////////////////////////////////////////////
                @@/* r8, r9, r10, r11 */                
                ldr   r8, [in_data_ptr512], #2*4  @@// get 513, 515, .....
                @@// do pcm 0                
                ldr   r11, [overlap_ptr0], #4 @@// load the overlap 0
                ldr   r9, [win_ptr0], #4      @@// load the 0 win coef
                add   r11,   r11,  #2048                
                SMLAWT r11,  r8,  r9,  r11     @@// window and get the r10                      
                         
                mov    r11,  r11, asr #12     @@// right shift 12 bit                                
                mov    r10,  r11, asr #15
                teq    r10,  r11, asr #31
                EORNE  r11,  r12, r11, asr #31	                
                ldr   r10, [win_ptr1024], #-4  @@// load the 1023 win coef 
                strh   r11,  [pcm_out_ptr0], #2                
                @@// do pcm 1023
                ldr    r11, [overlap_ptr1024], #-4 @@// load the overlap 1023
                rsb    r8,   r8,   #0
                add    r11,  r11,  #2048
                SMLAWB r11,  r8,  r10, r11         @@// window and get the r10          
               
                mov    r11,  r11, asr  #12     @@// right shift 12 bit
                mov    r8,  r11, asr #15
                teq    r8,  r11, asr #31
                EORNE  r11,  r12, r11, asr #31	                                   
                strh   r11,  [pcm_out_ptr1024], #-2    
                @@///////////////////////////////////////
                @@// do pcm 1
                @//?????ldr   r9, [win_ptr0], #4      @@// load the 1 win coef
                
                
                ldr    r11, [overlap_ptr0], #4 @@// load the overlap 1
                ldr    r8, [in_data_ptr0], #-2*4  @@// get 510, 508, .....                       
                add    r11, r11,  #2048
                rsb    r8,   r8,  #0
                SMLAWB r11,  r8,   r9,  r11     @@// window and get the r10                                
                @//?????ldr   r9, [win_ptr1024], #-4  @@// load the 1022 win coef                  
                         
                mov    r11,  r11, asr  #12     @@// right shift 12 bit                                
                mov    r9,  r11, asr #15
                teq    r9,  r11, asr #31
                EORNE  r11,  r12, r11, asr #31	
                strh   r11,  [pcm_out_ptr0], #2                
                @@// do pcm 1022
                ldr    r11, [overlap_ptr1024], #-4 @@// load the overlap 1022                
                rsb    r8,   r8,  #0
                add    r11,  r11,  #2048
                SMLAWT r11,  r8,  r10, r11         @@// window and get the r10 
                
                mov   r11,  r11, asr  #12     @@// right shift 12 bit
                mov    r10,  r11, asr #15
                teq    r10,  r11, asr #31
                EORNE  r11,  r12, r11, asr #31	
                strh   r11,  [pcm_out_ptr1024], #-2    
                @@/////////////////
                @@///////////////////////////////////////////////////
                subs   r14,  r14,  #1
                BGT    AAC_DEC_ARM_StopLongBlockProcessingAsm_PCM_LOOP
                ldmfd   sp!, {r6}
                
                mov   r14,  #256 
                
                add     win_ptr1024,   r6,  #512*4
                sub     win_ptr0,      win_ptr1024,  #4
                
                add     in_data_ptr0,  in_data_ptr0, #3*4
                sub     in_data_ptr512, in_data_ptr512, #3*4
                               
AAC_DEC_ARM_StopLongBlockProcessingAsm_OVLERLAP_LOOP:
                @@///////////////////////////////////////////////////
                @@/* r2, r5,  r8, r9, r10, r11, r12 */
                ldr  r2,  [in_data_ptr0],   #2*4    @@// load  1, 3, 5,7 
                ldr  r5,  [in_data_ptr512], #-2*4   @@// load  1022, 1020, 1018, 1016 
                ldr  r8,  [win_ptr0], #-4
                rsb  r2,  r2,  #0  @@//                 
                SMULWT r8,  r2,  r8
                ldr    r12, [win_ptr1024], #4
                str    r8,  [overlap_ptr0], #4                
                SMULWT r10,  r2,  r12
                ldr    r8, [win_ptr0], #-4       @@// 
                str    r10,  [overlap_ptr1024], #-4                
                SMULWT r12,  r5,   r8
                ldr    r2,  [win_ptr1024], #4
                str    r12, [overlap_ptr0], #4                
                SMULWT r8,  r5,   r2                
                subs   r14,  r14,  #1
                str   r8,  [overlap_ptr1024], #-4
                @@///////////////////////////////////////////////////
                
                BGT    AAC_DEC_ARM_StopLongBlockProcessingAsm_OVLERLAP_LOOP                
                
                ldmfd   sp!, {r4-r12, pc}
                @ENDFUNC
                
                
                
                
                @@////////////////////////////////////////////////
		@@/* for AAC-LC & LTP pcm output */
		
		@@// void AAC_DEC_ShortBlockOnlyWindowLCLtpAsm()	
		
		
		@@ // r0, the output data
		@@ // the input data fixed-point is S16.0
		@@ // and the output data fixed-point is S16.0
		@@ // r1, the data for overlaping
		@@ // r2, the table for window
		@@ // r3, the table for overlapping
s_k			 .req	   r14
s_lap        .req      r0
s_pcm_tab    .req      r1
s_lap_tab    .req      r2
s_in_data    .req      r3
s_out_put    .req      r4

sum11        .req      r5
sum22        .req      r6
tab_val0     .req      r7
tab_val1     .req      r8
ddd0         .req      r9
ddd1         .req      r10
in_data_ptr2 .req      r11

satutation   .req      r12

tmp_ptr1     .req      r4
tmp_ptr2     .req      r12
		
AAC_DEC_ShortBlockOnlyWindowLCLtpAsm:@ FUNCTION
		.global	AAC_DEC_ShortBlockOnlyWindowLCLtpAsm
		
		stmfd 	sp!, {r4 - r12, r14}		
		mov  s_k, #0
		
		
		

@@s_pcm_tab    .req      1
@@s_lap_tab    .req      2
@@s_in_data    .req      3
@@s_out_put    .req      4		
@@s_lap        .req      0		
		ldr  r4, [r0, #4]
		ldr  s_lap_tab, [r0, #8]
		ldr  s_in_data, [r0, #12]
		
		
		ldr  s_lap, [r0]
		
		ldr  satutation, =0x00007FFF
		
		mov   s_k,  #448
LC_PCM_FIRST:		
		@@ // LOOP
		ldmia s_lap!, {sum11, sum22}           
               
        add    sum11, sum11, #2048		
		mov    sum11, sum11, asr  #12				
		mov    r9,  sum11, asr #15
        teq    r9,  sum11, asr #31
        EORNE  sum11, satutation, sum11, asr #31	
        add    sum22, sum22, #2048		
		mov    sum22, sum22, asr  #12		
		mov    r9,  sum22, asr #15
        teq    r9,  sum22, asr #31
        EORNE  sum22, satutation, sum22, asr #31        
        strh   sum11, [r1], #2
        strh   sum22, [r1], #2
        
        
        @@//////////////////
        ldmia s_lap!, {sum11, sum22}           
               
        add    sum11, sum11, #2048		
		mov    sum11, sum11, asr  #12				
		mov    r9,  sum11, asr #15
        teq    r9,  sum11, asr #31
        EORNE  sum11, satutation, sum11, asr #31	
        add    sum22, sum22, #2048		
		mov    sum22, sum22, asr  #12		
		mov    r9,  sum22, asr #15
        teq    r9,  sum22, asr #31
        EORNE  sum22, satutation, sum22, asr #31        
        strh   sum11, [r1], #2
        strh   sum22, [r1], #2
        
		
		@@ // control
		subs   s_k,  s_k,  #4
		BNE    LC_PCM_FIRST
		@@ // END OF PCM_FIRST
		
		mov    s_k, #128		
LC_PCM_SECOND:		
		@@ // LOOP
		ldr     tab_val0, [r4], #4
		ldmia   s_lap!, {sum11, sum22}
		ldmia   s_in_data!, {ddd0-ddd1}	
		
		SMLAWT  sum11, ddd0, tab_val0, sum11
		SMLAWB  sum22, ddd1, tab_val0, sum22
		

        add    sum11, sum11, #2048		
		mov    sum11, sum11, asr  #12				
		mov    r9,  sum11, asr #15
        teq    r9,  sum11, asr #31
        EORNE  sum11, satutation, sum11, asr #31	
        add    sum22, sum22, #2048		
		mov    sum22, sum22, asr  #12		
		mov    r9,  sum22, asr #15
        teq    r9,  sum22, asr #31
        EORNE  sum22, satutation, sum22, asr #31        
        strh   sum11, [r1], #2
        strh   sum22, [r1], #2
        
        @@////////////////////
        ldr     tab_val0, [r4], #4
		ldmia   s_lap!, {sum11, sum22}
		ldmia   s_in_data!, {ddd0-ddd1}	
		
		SMLAWT  sum11, ddd0, tab_val0, sum11
		SMLAWB  sum22, ddd1, tab_val0, sum22
		

        add    sum11, sum11, #2048		
		mov    sum11, sum11, asr  #12				
		mov    r9,  sum11, asr #15
        teq    r9,  sum11, asr #31
        EORNE  sum11, satutation, sum11, asr #31	
        add    sum22, sum22, #2048		
		mov    sum22, sum22, asr  #12		
		mov    r9,  sum22, asr #15
        teq    r9,  sum22, asr #31
        EORNE  sum22, satutation, sum22, asr #31        
        strh   sum11, [r1], #2
        strh   sum22, [r1], #2
        
		@@ // control
		subs    s_k, s_k, #4	
		BNE     LC_PCM_SECOND		
		@@ // END OF PCM_SECOND
		
		mov    s_k, #128
		add    r4, s_lap_tab, #252
		add    in_data_ptr2, s_in_data, #512
LC_PCM_THIRD:
		@@ // LOOP 
		ldr    ddd0, [s_in_data], #4
		ldr    ddd1, [in_data_ptr2], #4
		ldmia  s_lap!, {sum11, sum22}
		ldr    tab_val0, [r4], #-4
		ldr    tab_val1, [s_lap_tab], #4
		
		SMLAWB sum11, ddd0, tab_val0, sum11
		ldr    ddd0, [s_in_data], #4
		
		SMLAWT sum22, ddd0, tab_val0, sum22
		SMLAWT sum11, ddd1, tab_val1, sum11		
		
		ldr    ddd1, [in_data_ptr2], #4
		SMLAWB sum22, ddd1, tab_val1, sum22
				
        add    sum11, sum11, #2048		
		mov    sum11, sum11, asr  #12				
		mov    r9,  sum11, asr #15
        teq    r9,  sum11, asr #31
        EORNE  sum11, satutation, sum11, asr #31	
        add    sum22, sum22, #2048		
		mov    sum22, sum22, asr  #12		
		mov    r9,  sum22, asr #15
        teq    r9,  sum22, asr #31
        EORNE  sum22, satutation, sum22, asr #31        
        strh   sum11, [r1], #2
        strh   sum22, [r1], #2
        
		@@ // control
		subs    s_k, s_k, #2		
		BNE    LC_PCM_THIRD		
		@@ // END OF PCM_THIRD
		
		mov    s_k, #128
		add    s_in_data, s_in_data, #1024
		sub    s_lap_tab, s_lap_tab, #4
		add    r4, r4, #4
LC_PCM_FOURTH:
		@@ // LOOP
		ldr    ddd0, [s_in_data], #4
		ldr    ddd1, [in_data_ptr2], #4
		ldmia  s_lap!, {sum11, sum22}
		ldr    tab_val1, [s_lap_tab], #-4
		ldr    tab_val0, [r4], #4
		
		SMLAWT sum11, ddd0, tab_val0, sum11
		ldr    ddd0, [s_in_data], #4
		
		SMLAWB sum22, ddd0, tab_val0, sum22
		SMLAWB sum11, ddd1, tab_val1, sum11		
		
		ldr    ddd1, [in_data_ptr2], #4
		SMLAWT sum22, ddd1, tab_val1, sum22
		
        add    sum11, sum11, #2048		
		mov    sum11, sum11, asr  #12				
		mov    r9,  sum11, asr #15
        teq    r9,  sum11, asr #31
        EORNE  sum11, satutation, sum11, asr #31	
        add    sum22, sum22, #2048		
		mov    sum22, sum22, asr  #12		
		mov    r9,  sum22, asr #15
        teq    r9,  sum22, asr #31
        EORNE  sum22, satutation, sum22, asr #31        
        strh   sum11, [r1], #2
        strh   sum22, [r1], #2
        
		@@ // control
		subs    s_k, s_k, #2		
		BNE    LC_PCM_FOURTH		
		@@ // END OF PCM_FOURTH		
				
		mov    s_k, #128
		add    s_lap_tab, s_lap_tab, #4
		sub    r4, r4, #4
		add    in_data_ptr2, in_data_ptr2, #1024
LC_PCM_FIFTH:		
		@@ // LOOP
		ldr    ddd0, [s_in_data], #4
		ldr    ddd1, [in_data_ptr2], #4
		ldmia  s_lap!, {sum11, sum22}
		ldr    tab_val0, [r4], #-4
		ldr    tab_val1, [s_lap_tab], #4
		
		SMLAWB sum11, ddd0, tab_val0, sum11
		ldr    ddd0, [s_in_data], #4
		
		SMLAWT sum22, ddd0, tab_val0, sum22
		SMLAWT sum11, ddd1, tab_val1, sum11		
		
		ldr    ddd1, [in_data_ptr2], #4
		SMLAWB sum22, ddd1, tab_val1, sum22
		
        add    sum11, sum11, #2048		
		mov    sum11, sum11, asr  #12				
		mov    r9,  sum11, asr #15
        teq    r9,  sum11, asr #31
        EORNE  sum11, satutation, sum11, asr #31	
        add    sum22, sum22, #2048		
		mov    sum22, sum22, asr  #12		
		mov    r9,  sum22, asr #15
        teq    r9,  sum22, asr #31
        EORNE  sum22, satutation, sum22, asr #31        
        strh   sum11, [r1], #2
        strh   sum22, [r1], #2
		
		@@ // control
		subs    s_k, s_k, #2
		
		BNE    LC_PCM_FIFTH		
		@@ // END OF PCM_FIFTH	
		
		
		mov    s_k, #64
		@add    in_data_ptr2, in_data_ptr2, #256
		add    s_in_data, s_in_data, #1024
		
		sub    s_lap_tab, s_lap_tab, #4
		add    r4, r4, #4
LC_PCM_SIXTH:		
		@@ // LOOP 
		ldr    ddd1, [s_in_data], #4
		ldr    ddd0, [in_data_ptr2], #4
		ldmia  s_lap!, {sum11, sum22}
		ldr    tab_val0, [s_lap_tab], #-4
		ldr    tab_val1, [r4], #4
		
		SMLAWB sum11, ddd0, tab_val0, sum11
		ldr    ddd0, [in_data_ptr2], #4
		
		SMLAWT sum22, ddd0, tab_val0, sum22
		SMLAWT sum11, ddd1, tab_val1, sum11		
		
		ldr    ddd1, [s_in_data], #4
		SMLAWB sum22, ddd1, tab_val1, sum22
		

        add    sum11, sum11, #2048		
		mov    sum11, sum11, asr  #12				
		mov    r9,  sum11, asr #15
        teq    r9,  sum11, asr #31
        EORNE  sum11, satutation, sum11, asr #31	
        add    sum22, sum22, #2048		
		mov    sum22, sum22, asr  #12		
		mov    r9,  sum22, asr #15
        teq    r9,  sum22, asr #31
        EORNE  sum22, satutation, sum22, asr #31        
        strh   sum11, [r1], #2
        strh   sum22, [r1], #2
        
		@@ // control
		subs    s_k, s_k, #2
		BNE    LC_PCM_SIXTH		
		
		
		@@ // END OF PCM_SIXTH		
		mov    s_k, #64

		sub    s_in_data, s_in_data, #512
		add    in_data_ptr2, s_in_data, #512
		
		sub    s_lap, s_lap, #4096
LC_OVERLAP_FIRST:
		@@ // LOOP
		ldr    ddd0, [s_in_data], #4
		ldr    ddd1, [in_data_ptr2], #4
		ldr    tab_val0, [s_lap_tab], #-4
		ldr    tab_val1, [r4], #4
		mov    sum11, #0
		mov    sum22, #0
		
		SMLAWB sum11, ddd0, tab_val0, sum11
		ldr    ddd0, [s_in_data], #4
		
		SMLAWT sum22, ddd0, tab_val0, sum22
		SMLAWT sum11, ddd1, tab_val1, sum11		
		ldr    ddd1, [in_data_ptr2], #4
		SMLAWB sum22, ddd1, tab_val1, sum22 
		
		stmia  s_lap!, {sum11, sum22}
		@@ // control
		add    s_k, s_k, #2
		cmp    s_k, #128
		bcc    LC_OVERLAP_FIRST		
		@@ // END OF OVERLAP_FIRST
		
		mov    s_k, #0
		add    s_in_data, s_in_data, #512
		add    in_data_ptr2, s_in_data, #512
		add    r1,     in_data_ptr2, #512
		add    tmp_ptr2,     r1, #512
		
		add    s_lap_tab, s_lap_tab, #4
		sub    r4, r4, #4
LC_OVERLAP_SECOND:		
		ldr    tab_val0, [r4], #-4
		ldr    tab_val1, [s_lap_tab], #4
		
		@@// the first point
		ldr    ddd0, [s_in_data], #4
		ldr    ddd1, [in_data_ptr2], #4	
		
		mov    sum11, #0
		mov    sum22, #0
		
		SMLAWB sum11, ddd0, tab_val0, sum11
		ldr    ddd0, [s_in_data], #4
		
		SMLAWT sum22, ddd0, tab_val0, sum22
		SMLAWT sum11, ddd1, tab_val1, sum11		
		ldr    ddd1, [in_data_ptr2], #4	
		SMLAWB sum22, ddd1, tab_val1, sum22 @/////////////
		stmia  s_lap!, {sum11, sum22}
		
		@@// the second point
		ldr    ddd0, [r1], #4
		ldr    ddd1, [tmp_ptr2], #4	
		
		mov    sum11, #0
		mov    sum22, #0
		
		SMLAWB sum11, ddd0, tab_val0, sum11
		ldr    ddd0, [r1], #4
		
		SMLAWT sum22, ddd0, tab_val0, sum22
		SMLAWT sum11, ddd1, tab_val1, sum11		
		ldr    ddd1, [tmp_ptr2], #4	
		SMLAWB sum22, ddd1, tab_val1, sum22
		str    sum11, [s_lap, #504]
		str    sum22, [s_lap, #508]       @@//////////////////
		
		@@// the third point
		ldr    ddd0, [r1, #1016]
		ldr    ddd1, [tmp_ptr2, #1016]
		
		mov    sum11, #0
		mov    sum22, #0
		
		SMLAWB sum11, ddd0, tab_val0, sum11
		ldr    ddd0, [r1, #1020]
		
		SMLAWT sum22, ddd0, tab_val0, sum22
		SMLAWT sum11, ddd1, tab_val1, sum11		
		ldr    ddd1, [tmp_ptr2, #1020]
		SMLAWB sum22, ddd1, tab_val1, sum22
		str    sum11, [s_lap, #1016]
		str    sum22, [s_lap, #1020]
		@@// the fourth point
		ldr    ddd0, [r1, #2040] @@///////////
		
		mov    sum11, #0
		mov    sum22, #0
		SMLAWB sum11, ddd0, tab_val0, sum11
		ldr    ddd0, [r1, #2044]
		SMLAWT sum22, ddd0, tab_val0, sum22
		
		str    sum11, [s_lap, #1528]
		str    sum22, [s_lap, #1532]
		@@ // control
		add    s_k, s_k, #2
		cmp    s_k, #128
		bcc    LC_OVERLAP_SECOND		
		@@ // END OF OVERLAP_SECOND
		
		add   s_lap, s_lap, #1536
		mov   s_k, #0
		mov   sum11, #0
		mov   sum22, #0
		mov   ddd0, #0
		mov   ddd1, #0
LC_OVERLAP_END:		
		@@ // LOOP		
		
		stmia s_lap!, {sum11, sum22, ddd0, ddd1}
		@@ // control
		add   s_k, s_k, #4
		cmp   s_k, #448
		bcc   LC_OVERLAP_END
                
                
                
        ldmfd   sp!, {r4-r12, pc}
        @ENDFUNC        
                
                
                
                
                
                
                
                
                
                
                
                
                
                .end@END