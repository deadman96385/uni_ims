@// AAC-LC synthesis filter bank
@// author: reed.zhang
@// input: SBR synthesis filter bank processing
@//	r0: output sample pointer
@// r1: the input overlap
@// r2: the tablr for output data
@// r3: the table for overlapping  
@// purpose:

@// define the register name 
long_start_k    	      .req		  r14
long_start_overlap      .req      r0
long_start_table_p      .req      r1
long_start_overlap_tab  .req      r2
long_start_in_data_ptr  .req      r3

long_start_out_put      .req      r4

long_start_sum11        .req      r5
long_start_sum22        .req      r6
long_start_sum33        .req      r7
long_start_sum44        .req      r8
long_start_tab_val0     .req      r9
long_start_ddd0         .req      r11
long_start_ddd1         .req      r12
long_start_tab_va20     .req      r10
@@/// overlap
long_start_in_data_ptr2 .req      r7
@//OVERLAP2
tab_val                 .req      r4
@//satutation           .req      r12

		.text@AREA	PROGRAM, CODE, READONLY				
		.arm@CODE32			
		@//int32 AAC_DEC_SBR_ARM_StartLongBlockProcessingAsm(
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


AAC_DEC_SBR_ARM_StartLongBlockProcessingAsm:   @FUNCTION
                @ // save the value for the following calculation
                .global  AAC_DEC_SBR_ARM_StartLongBlockProcessingAsm
                stmfd   sp!, {r4-r12, r14}
                ldmia   r3, {win_ptr0, r6}
                stmfd   sp!, {r6}        
                add in_data_ptr512, in_data_ptr512, #512*4
                add in_data_ptr512, in_data_ptr512,  #4
                sub in_data_ptr0, in_data_ptr512, #3*4
               
                add win_ptr1024, win_ptr0, #1024*4
                sub win_ptr1024, win_ptr1024, #4
                
                add overlap_ptr1024, overlap_ptr0, #1024*4
                sub overlap_ptr1024, overlap_ptr1024, #4                
                add pcm_out_ptr1024, pcm_out_ptr0, #1024*4
                sub pcm_out_ptr1024, pcm_out_ptr1024, #4              
                
                mov   r14,  #256                
AAC_DEC_SBR_ARM_StartLongBlockProcessingAsm_PCM_LOOP:
                @@///////////////////////////////////////////////////
                @@/* r8, r9, r10, r11 */
                ldr   r8, [in_data_ptr512], #2*4  @@// get 513, 515, .....
                
                @@// do pcm 0                
                ldr   r11, [overlap_ptr0], #4 @@// load the overlap 0
                ldr   r9, [win_ptr0], #4      @@// load the 0 win coef
        
                SMLAWT r11,  r8,   r9, r11    @@// window and get the r10   

                ldr   r9, [win_ptr1024], #-4  @@// load the 1023 win coef                

                str   r11,  [pcm_out_ptr0], #4           
                @@// do pcm 1023                
                ldr    r11, [overlap_ptr1024], #-4 @@// load the overlap 1023
                rsb    r8,   r8,  #0
                SMLAWT r11,  r8,   r9,  r11         @@// window and get the r10                                          
                ldr   r8, [in_data_ptr0], #-2*4  @@// get 510, 508, .....      
                str   r11,  [pcm_out_ptr1024], #-4    
                @@///////////////////////////////////////
                @@// do pcm 1
                ldr    r11, [overlap_ptr0], #4 @@// load the overlap 1
                ldr    r9, [win_ptr0], #4      @@// load the 1 win coef
                rsb    r8,  r8,    #0              
                SMLAWT r11,  r8,   r9,  r11     @@// window and get the r10                
                
                ldr   r9, [win_ptr1024], #-4  @@// load the 1022 win coef                  
                str   r11,  [pcm_out_ptr0], #4
                
                ldr   r10, [overlap_ptr1024], #-4 @@// load the overlap 1022              
                @@// do pcm 1022
                rsb    r8,   r8,  #0
                SMULWT r11,  r8,   r9         @@// window and get the r10 
                add    r11,  r11,  r10
                str   r11,  [pcm_out_ptr1024], #-4                                  
                @@/////////////////
                @@///////////////////////////////////////////////////
                subs   r14,  r14,  #1
                BGT    AAC_DEC_SBR_ARM_StartLongBlockProcessingAsm_PCM_LOOP                
                
                @@/* overlap */
                ldmfd   sp!, {r6}                
                mov   r14,  #32                 
                add     win_ptr1024,   r6,  #64*2
                sub     win_ptr0,      win_ptr1024,  #4
                
                add     in_data_ptr0,  in_data_ptr0, #3*4
                sub     in_data_ptr512, in_data_ptr512, #3*4
                               
AAC_DEC_SBR_ARM_StartLongBlockProcessingAsm_OVLERLAP_LOOP:
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
                SMULWB r9,  r2,  r8
                SMULWT  r10, r2,  r12
                
                str   r9,  [overlap_ptr0], #4  
                str   r10, [overlap_ptr1024], #-4
                
                SMULWT r9,  r5,   r8
                SMULWB r10, r5,   r12              
                
                str   r9,  [overlap_ptr0], #4  
                str   r10, [overlap_ptr1024], #-4
                subs   r14,  r14,  #2
                @@///////////////////////////////////////////////////
                
                BGT    AAC_DEC_SBR_ARM_StartLongBlockProcessingAsm_OVLERLAP_LOOP                
                
                
                mov   r14,  #224 
                mov   r11,  #0
                mov   r12,  #0
                mov   r9 ,  #0
                mov   r10,  #0
AAC_DEC_SBR_ARM_StartLongBlockProcessingAsm_OVLERLAP_LOOP1:
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
                
                BGT    AAC_DEC_SBR_ARM_StartLongBlockProcessingAsm_OVLERLAP_LOOP1                   
                
                
                
                ldmfd   sp!, {r4-r12, pc}
                @ENDFUNC
                
                
                
                
                
                
                
                
                
		
		
@// define the register name 
long_start_k    	      .req		  r14
long_start_overlap      .req      r0
@//long_start_table_p   .req      r1
long_start_overlap_tab  .req      r2
long_start_in_data_ptr  .req      r3

@//long_start_out_put   .req      r4

long_start_sum11        .req      r5
long_start_sum22        .req      r6
long_start_sum33        .req      r7
long_start_sum44        .req      r8
long_start_tab_val0     .req      r9
long_start_ddd0         .req      r11
long_start_ddd1         .req      r12
long_start_tab_va20     .req      r10
@@/// overlap
long_start_in_data_ptr2 .req      r7
@//OVERLAP2
tab_val                 .req      r4
@//satutation           .req      r12
		
		@// void AAC_DEC_LongStartBlockWindowLCLtpAsm()@
		
		
	
		
AAC_DEC_LongStartBlockWindowLCLtpAsm:        @FUNCTION 
		.global	AAC_DEC_LongStartBlockWindowLCLtpAsm
		stmfd 	sp!, {r4 - r12, r14}	
		mov     long_start_k, #1024	
		
        ldr     long_start_sum44,  =0x00007FFF
		
		ldr     r4,     [r0, #4]
		ldr     long_start_overlap_tab, [r0, #8]   @@//
		ldr     long_start_in_data_ptr, [r0, #12]
		
		ldr     long_start_overlap,     [r0]		
		
		
		
LC_PCM_LOOP_LONG_START:
		@@ // loop 
		ldmia  r4!, {long_start_tab_val0, long_start_tab_va20}		
		ldmia  long_start_in_data_ptr!, {long_start_ddd0-long_start_ddd1}
		ldmia  long_start_overlap!, {long_start_sum11 - long_start_sum33}
		SMLAWT long_start_sum11, long_start_ddd0, long_start_tab_val0, long_start_sum11
		SMLAWB long_start_sum22, long_start_ddd1, long_start_tab_val0, long_start_sum22
        ldmia  long_start_in_data_ptr!, {long_start_tab_val0, long_start_ddd0}
		SMLAWT long_start_sum33, long_start_tab_val0, long_start_tab_va20, long_start_sum33
		
		add    long_start_sum11, long_start_sum11, #64
		mov    long_start_sum11, long_start_sum11, asr  #7
		mov    r9,  long_start_sum11, asr #15
        teq    r9,  long_start_sum11, asr #31
        EORNE  long_start_sum11, long_start_sum44, long_start_sum11, asr #31	
        strh   long_start_sum11, [r1], #2
        
		add    long_start_sum22, long_start_sum22, #64
		add    long_start_sum33, long_start_sum33, #64		
		
		ldr  long_start_sum11, [long_start_overlap], #4
		
		mov    long_start_sum22, long_start_sum22, asr  #7
		mov    long_start_sum33, long_start_sum33, asr  #7
		
		SMLAWB long_start_sum11, long_start_ddd0, long_start_tab_va20, long_start_sum11
		
		
		
		mov    r9,  long_start_sum22, asr #15
        teq    r9,  long_start_sum22, asr #31
        EORNE  long_start_sum22, long_start_sum44, long_start_sum22, asr #31	
        
        
        mov    r9,  long_start_sum33, asr #15
        teq    r9,  long_start_sum33, asr #31
        EORNE  long_start_sum33, long_start_sum44, long_start_sum33, asr #31	
		

		strh   long_start_sum22, [r1], #2
		strh   long_start_sum33, [r1], #2
		
		
		add    long_start_sum11, long_start_sum11, #64
		mov    long_start_sum11, long_start_sum11, asr  #7
		
		mov    r9,  long_start_sum11, asr #15
        teq    r9,  long_start_sum11, asr #31
        EORNE  long_start_sum11, long_start_sum44, long_start_sum11, asr #31	
        strh   long_start_sum11, [r1], #2 	
		
        
        
        

		ldmia  r4!, {long_start_tab_val0, long_start_tab_va20}
		ldr    long_start_ddd0, [long_start_in_data_ptr], #4
		
		ldmia  long_start_overlap!, {long_start_sum11 - long_start_sum33}
		
		SMLAWT long_start_sum11, long_start_ddd0, long_start_tab_val0, long_start_sum11
		ldr    long_start_ddd0, [long_start_in_data_ptr], #4
		
		SMLAWB long_start_sum22, long_start_ddd0, long_start_tab_val0, long_start_sum22
        ldmia  long_start_in_data_ptr!, {long_start_tab_val0, long_start_ddd0}
		
		SMLAWT long_start_sum33, long_start_tab_val0, long_start_tab_va20, long_start_sum33
		
		
		add    long_start_sum11, long_start_sum11, #64
		mov    long_start_sum11, long_start_sum11, asr  #7
		mov    r9,  long_start_sum11, asr #15
        teq    r9,  long_start_sum11, asr #31
        EORNE  long_start_sum11, long_start_sum44, long_start_sum11, asr #31	
        strh   long_start_sum11, [r1], #2
        
        add    long_start_sum22, long_start_sum22, #64
		add    long_start_sum33, long_start_sum33, #64
		
		ldr  long_start_sum11, [long_start_overlap], #4
		
		mov    long_start_sum22, long_start_sum22, asr  #7
		mov    long_start_sum33, long_start_sum33, asr  #7
		
		SMLAWB long_start_sum11, long_start_ddd0, long_start_tab_va20, long_start_sum11
		
        
        mov    r9,  long_start_sum22, asr #15
        teq    r9,  long_start_sum22, asr #31
        EORNE  long_start_sum22, long_start_sum44, long_start_sum22, asr #31	
        
        
        mov    r9,  long_start_sum33, asr #15
        teq    r9,  long_start_sum33, asr #31
        EORNE  long_start_sum33, long_start_sum44, long_start_sum33, asr #31			

		strh   long_start_sum22, [r1], #2
		strh   long_start_sum33, [r1], #2


        add    long_start_sum11, long_start_sum11, #64
		mov    long_start_sum11, long_start_sum11, asr  #7

        mov    r9,  long_start_sum11, asr #15
        teq    r9,  long_start_sum11, asr #31
        EORNE  long_start_sum11, long_start_sum44, long_start_sum11, asr #31	
        strh   long_start_sum11, [r1], #2 	
        
        
		@@ // control 
		subs long_start_k, long_start_k, #8
		bne LC_PCM_LOOP_LONG_START
		
		
		
		
		
		@@// calculate the overlap data
		sub long_start_overlap, long_start_overlap, #4096
		mov long_start_k, #448
		add long_start_in_data_ptr2, long_start_overlap, #2304   
		
       	mov     long_start_ddd0, #0 
       	mov     long_start_ddd1, #0
       	mov     long_start_tab_val0, #0 
       	mov     long_start_tab_va20, #0
LC_OVERLAP1:
		@@ // loop
        ldmia  long_start_in_data_ptr!, {long_start_sum11-long_start_sum22}	
       	mov    long_start_sum11,  long_start_sum11, asr #1       	
       	mov    long_start_sum22,  long_start_sum22, asr #1      	
       	stmia long_start_overlap!, {long_start_sum11-long_start_sum22}
       	ldmia  long_start_in_data_ptr!, {long_start_sum11-long_start_sum22}	
       	mov     long_start_sum11, long_start_sum11, asr #1
       	
       	mov     long_start_sum22,  long_start_sum22, asr #1
       	
       	stmia long_start_overlap!, {long_start_sum11-long_start_sum22}
        stmia long_start_in_data_ptr2!, {long_start_tab_val0-long_start_ddd1}
       	
       	ldmia  long_start_in_data_ptr!, {long_start_sum11-long_start_sum22}	
       	mov    long_start_sum11,  long_start_sum11, asr #1       	
       	mov    long_start_sum22,  long_start_sum22, asr #1      	
       	stmia long_start_overlap!, {long_start_sum11-long_start_sum22}
       	ldmia  long_start_in_data_ptr!, {long_start_sum11-long_start_sum22}	
       	mov    long_start_sum11,  long_start_sum11, asr #1       	
       	mov    long_start_sum22,  long_start_sum22, asr #1
       	stmia long_start_overlap!, {long_start_sum11-long_start_sum22}
        stmia long_start_in_data_ptr2!, {long_start_tab_val0-long_start_ddd1}
       	
       	
       	
       	@@ // control	
		subs long_start_k, long_start_k, #8		
		bne LC_OVERLAP1
@long_start_tab_val0     .req      r9
		
@long_start_ddd0         .req      r11
@long_start_ddd1         .req      r12
@long_start_tab_va20     .req      r10		
		mov long_start_k, #128
LC_OVERLAP2:
		ldr  tab_val, [long_start_overlap_tab], #-4
		ldmia long_start_in_data_ptr!, {long_start_tab_val0-long_start_ddd1}

       	SMULWB  long_start_sum11, long_start_tab_val0, tab_val
       	SMULWT  long_start_sum22, long_start_tab_va20, tab_val
       	ldr  r4, [long_start_overlap_tab], #-4
       	SMULWB  long_start_sum33, long_start_ddd0, r4
       	SMULWT  long_start_sum44, long_start_ddd1, r4
		stmia   long_start_overlap!, {long_start_sum11-long_start_sum44}
		ldr  tab_val, [long_start_overlap_tab], #-4
		ldmia long_start_in_data_ptr!, {long_start_tab_val0-long_start_ddd1}		
       	
       	SMULWB  long_start_sum11, long_start_tab_val0, tab_val   	
       	SMULWT  long_start_sum22, long_start_tab_va20, tab_val
       	ldr  r4, [long_start_overlap_tab], #-4
       	SMULWB  long_start_sum33, long_start_ddd0, r4
       	SMULWT  long_start_sum44, long_start_ddd1, r4
		stmia   long_start_overlap!, {long_start_sum11-long_start_sum44}
		
		@@ // control	
		subs long_start_k, long_start_k, #8
		bne LC_OVERLAP2
		@ // load the following value for quit the function
		ldmfd	sp!, {r4 - r12, pc}
				
		@ENDFUNC
		
		.end@END