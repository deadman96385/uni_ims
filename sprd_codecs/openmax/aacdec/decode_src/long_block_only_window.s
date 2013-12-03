@// AAC-LC synthesis filter bank
@// author: reed.zhang
@// input: SBR synthesis filter bank processing
@//	r0: output sample pointer
@// r1: the input overlap
@// r2: the tablr for output data
@// r3: the table for overlapping  
@// purpose:

@// define the register name 
k			       .req		   r14
overlap      .req      r0
table_p      .req      r1
overlap_tab  .req      r2
in_data_ptr  .req      r3


out_put      .req      r4

sum11        .req      r5
sum22        .req      r6
tab_val0     .req      r7
in_data_ptr2 .req      r8
ddd0         .req      r9
ddd1         .req      r10
tab_val1     .req      r11

@//satutation   .req   r12
		@ ***** END LICENSE BLOCK *****  

	.text@AREA	|.text|, CODE, READONLY	
	.arm	
		
		
		@//int32 AAC_DEC_SBR_ARM_OnlyLongBlockProcessingAsm(
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


AAC_DEC_SBR_ARM_OnlyLongBlockProcessingAsm:   @FUNCTION
                @ // save the value for the following calculation
                .global  AAC_DEC_SBR_ARM_OnlyLongBlockProcessingAsm
                stmfd   sp!, {r4-r12, r14}       
                ldmia   r3, {win_ptr0, r6}                
                stmfd   sp!, {r6}                
                
                add in_data_ptr512, in_data_ptr512,  #512*4
                add in_data_ptr512, in_data_ptr512,  #4
                sub in_data_ptr0, in_data_ptr512,    #3*4
               
                add win_ptr1024, win_ptr0,           #1024*4
                sub win_ptr1024, win_ptr1024,        #4
                
                add overlap_ptr1024, overlap_ptr0,   #1024*4
                sub overlap_ptr1024, overlap_ptr1024,#4
                
                add pcm_out_ptr1024, pcm_out_ptr0,   #1024*4
                sub pcm_out_ptr1024, pcm_out_ptr1024,#4              
                
                mov   r14,  #256                
AAC_DEC_SBR_ARM_OnlyLongBlockProcessingAsm_PCM_LOOP:
                @@///////////////////////////////////////////////////
                @@/* r8, r9, r10, r11 */
                ldr   r8, [in_data_ptr512], #2*4  @@// get 513, 515, .....
                
                @@// do pcm 0
                ldr   r9, [win_ptr0], #4      @@// load the 0 win coef
                ldr   r11, [overlap_ptr0], #4 @@// load the overlap 0
                SMULL r9,  r10,   r8,  r9     @@// window and get the r10                

                ldr   r9, [win_ptr1024], #-4  @@// load the 1023 win coef                
                add   r11,   r10,   r11       @@// add overlap
                         
                str   r11,  [pcm_out_ptr0], #4
                @@// do pcm 1023
                SMULL r9,  r10,   r8,  r9         @@// window and get the r10
                ldr   r11, [overlap_ptr1024], #-4 @@// load the overlap 1023                
                ldr   r8, [in_data_ptr0], #-2*4  @@// get 510, 508, .....                
                sub   r11,  r11,  r10                
                str   r11,  [pcm_out_ptr1024], #-4 
                @@///////////////////////////////////////
                @@// do pcm 1
                ldr   r9, [win_ptr0], #4      @@// load the 1 win coef
                ldr   r11, [overlap_ptr0], #4 @@// load the overlap 1
                SMULL r9,  r10,   r8,  r9     @@// window and get the r10                

                ldr   r9, [win_ptr1024], #-4  @@// load the 1022 win coef                
                sub   r11,   r11,   r10       @@// add overlap
                str   r11,  [pcm_out_ptr0], #4                
                @@// do pcm 1022
                SMULL r9,  r10,   r8,  r9         @@// window and get the r10                 
                ldr   r11, [overlap_ptr1024], #-4 @@// load the overlap 1022
                subs   r14,  r14,  #1
                add   r11,  r11,  r10                
                str   r11,  [pcm_out_ptr1024], #-4                   

                @@///////////////////////////////////////////////////
                
                BGT    AAC_DEC_SBR_ARM_OnlyLongBlockProcessingAsm_PCM_LOOP
                
                
                
                ldmfd   sp!, {r6}
                
                mov   r14,  #256 
                
                add     win_ptr1024,   r6,  #512*4
                sub     win_ptr0,      win_ptr1024,  #4
                
                add     in_data_ptr0,  in_data_ptr0, #3*4
                sub     in_data_ptr512, in_data_ptr512, #3*4
                               
AAC_DEC_SBR_ARM_OnlyLongBlockProcessingAsm_OVLERLAP_LOOP:
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
                
                BGT    AAC_DEC_SBR_ARM_OnlyLongBlockProcessingAsm_OVLERLAP_LOOP                
                
                ldmfd   sp!, {r4-r12, pc}
                @ENDFUNC
                	
		
		
		
		
		
		@//void AAC_DEC_OnlyLongBlockWindowLCLtpAsm()

k			       .req	   	 r14
overlap      .req      r0
LC_PCM_OUT   .req      r1

@//table_p   .req      r1
overlap_tab  .req      r2
in_data_ptr  .req      r3

sum11        .req      r5
sum22        .req      r6
tab_val0     .req      r7
in_data_ptr2 .req      r8
ddd0         .req      r9
ddd1         .req      r10
tab_val1     .req      r11

		
AAC_DEC_OnlyLongBlockWindowLCLtpAsm:  @FUNCTION

        .global	AAC_DEC_OnlyLongBlockWindowLCLtpAsm
		@ // save the value for the following calculation
		stmfd 	sp!, {r4 - r12, r14}
		
		ldr     r4, [r0, #4]
		ldr     overlap_tab, [r0, #8]
		ldr     in_data_ptr, [r0, #12]

		ldr     overlap, [r0]
        ldr     r12, =0x00007FFF
        		
		mov     k, #1024
		add     in_data_ptr2, in_data_ptr, #4096
		
AAC_DEC_OnlyLongBlockWindowLCLtpAsm_LOOP:
		ldr    tab_val0, [r4], #4
		ldmia  overlap, {sum11,sum22}
		
		ldmia  in_data_ptr!, {ddd0, ddd1}		
		ldr    tab_val1, [overlap_tab], #-4
		SMLAWT sum11, ddd0, tab_val0, sum11  @// output the first point		
		SMLAWB sum22, ddd1, tab_val0, sum22  @// output the second point
		
		add    sum11,  sum11,  #64
		add    sum22,  sum22,  #64
		
		mov    sum11,  sum11, asr  #7
		mov    sum22,  sum22, asr  #7


        mov    r10,  sum11, asr #15
        teq    r10,  sum11, asr #31
        EORNE  sum11, r12, sum11, asr #31	
        
        mov    r10,  sum22, asr #15
        teq    r10,  sum22, asr #31
        EORNE  sum22, r12, sum22, asr #31	
        
        ldmia   in_data_ptr2!, {ddd0, ddd1}
		strh   sum11,  [LC_PCM_OUT], #2
		strh   sum22,  [LC_PCM_OUT], #2
		
		
		SMULWB tab_val0, ddd0, tab_val1     @ // output first overlap data
		SMULWT ddd0,     ddd1, tab_val1     @ // output second overlap data
		stmia  overlap!,    {tab_val0, ddd0}  
		
        @@/////////////////////////////////
        ldr    tab_val0, [r4], #4
		ldmia  overlap, {sum11,sum22}
		
		ldmia  in_data_ptr!, {ddd0, ddd1}		
		ldr    tab_val1, [overlap_tab], #-4
		SMLAWT sum11, ddd0, tab_val0, sum11  @// output the first point		
		SMLAWB sum22, ddd1, tab_val0, sum22  @// output the second point
		
		add    sum11,  sum11,  #64
		add    sum22,  sum22,  #64		
		mov    sum11,  sum11, asr  #7
		mov    sum22,  sum22, asr  #7
        mov    r10,  sum11, asr #15
        teq    r10,  sum11, asr #31
        EORNE  sum11, r12, sum11, asr #31	
        
        mov    r10,  sum22, asr #15
        teq    r10,  sum22, asr #31
        EORNE  sum22, r12, sum22, asr #31	
        
        ldmia   in_data_ptr2!, {ddd0, ddd1}
		strh   sum11,  [LC_PCM_OUT], #2
		strh   sum22,  [LC_PCM_OUT], #2
		
		
		SMULWB tab_val0, ddd0, tab_val1     @ // output first overlap data
		SMULWT ddd0,     ddd1, tab_val1     @ // output second overlap data
		stmia  overlap!,    {tab_val0, ddd0}     
		@// the loop control
		subs k, k, #4
		bne AAC_DEC_OnlyLongBlockWindowLCLtpAsm_LOOP		
		
		@ // load the following value for quit the function
		ldmfd	sp!, {r4 - r12, pc}
				
		@ENDFUNC
		
		
		
		.end@END