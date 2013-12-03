@// AAC-LC filter bank, long-block end
@// author: reed.zhang
@// input: SBR synthesis filter bank processing
@//	r0: output sample pointer
@// r1: the input overlap
@// r2: the tablr for output data
@// r3: the table for overlapping  
@// purpose:

@// define the register name 
k			       .req		   r14
over_lap     .req      r0
table_pcm    .req      r1
table_lap    .req      r2
indata_ptr   .req      r3
output       .req      r4

sum11        .req      r5
sum22        .req      r6
sum33        .req      r7
sum44        .req      r8
ddd0         .req      r9

ddd1         .req      r10
tab_val1     .req      r11
overlap_ptr1 .req      r12
@@@@@@@@@@@
@@//PCM_LOOP1
output_ptr1  .req      r11


		.text@AREA	PROGRAM, CODE, READONLY				
		.arm@CODE32			
		@//int32 AAC_DEC_SBR_ARM_StopLongBlockProcessingAsm(
                @//                              int32   *in_data_ptr,
                @//                              int32   *overlap_ptr,
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


AAC_DEC_SBR_ARM_StopLongBlockProcessingAsm:   @FUNCTION
                @ // save the value for the following calculation
                .global  AAC_DEC_SBR_ARM_StopLongBlockProcessingAsm
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
                
                add pcm_out_ptr1024, pcm_out_ptr0, #1024*4
                sub pcm_out_ptr1024, pcm_out_ptr1024, #4      


                mov   r14,  #224      
AAC_DEC_SBR_ARM_StopLongBlockProcessingAsm_PCM_LOOP1:
                @@///////////////////////////////////////////////////
                @@/* r8, r9, r10, r11 */     
                @@// do pcm 0                
                ldr   r11, [overlap_ptr0], #4 @@// load the overlap 0                       
                ldr   r8, [in_data_ptr512], #2*4  @@// get 513, 515, .....                
                
                str   r11,  [pcm_out_ptr0], #4              
                @@// do pcm 1023
                ldr    r11, [overlap_ptr1024], #-4 @@// load the overlap 1023                
                rsb    r8,   r8,   #0

                add    r11,  r11, r8, asr #1               
                                            
                str   r11,  [pcm_out_ptr1024], #-4    
                @@///////////////////////////////////////                
                ldr    r11, [overlap_ptr0], #4 @@// load the overlap 1
                ldr    r8, [in_data_ptr0], #-2*4  @@// get 510, 508, .....                       
                ldr    r10, [overlap_ptr1024], #-4 @@// load the overlap 1022           
                str   r11,  [pcm_out_ptr0], #4               
                @@// do pcm 1022                
                add    r8,  r10,   r8,  asr #1
                str   r8,  [pcm_out_ptr1024], #-4    
                @@/////////////////
               
                subs   r14,  r14,  #1
                BGT    AAC_DEC_SBR_ARM_StopLongBlockProcessingAsm_PCM_LOOP1                       
                                
                mov   r14,  #32               
AAC_DEC_SBR_ARM_StopLongBlockProcessingAsm_PCM_LOOP:
                @@///////////////////////////////////////////////////
                @@/* r8, r9, r10, r11 */                
                ldr   r8, [in_data_ptr512], #2*4  @@// get 513, 515, .....
                @@// do pcm 0                
                ldr   r11, [overlap_ptr0], #4 @@// load the overlap 0
                ldr   r9, [win_ptr0], #4      @@// load the 0 win coef
     
                SMLAWT r11,  r8,  r9,  r11     @@// window and get the r10             
                
                ldr   r10, [win_ptr1024], #-4  @@// load the 1023 win coef 
                str   r11,  [pcm_out_ptr0], #4               
                @@// do pcm 1023
                ldr    r11, [overlap_ptr1024], #-4 @@// load the overlap 1023
                rsb    r8,   r8,   #0

                SMLAWB r11,  r8,  r10, r11         @@// window and get the r10          
                                  
                str   r11,  [pcm_out_ptr1024], #-4    
                @@///////////////////////////////////////
                @@// do pcm 1
                
                ldr    r11, [overlap_ptr0], #4 @@// load the overlap 1
                ldr    r8, [in_data_ptr0], #-2*4  @@// get 510, 508, .....                       

                rsb    r8,   r8,  #0
                SMLAWB r11,  r8,   r9,  r11     @@// window and get the r10                                
                @//?????ldr   r9, [win_ptr1024], #-4  @@// load the 1022 win coef                  

                str   r11,  [pcm_out_ptr0], #4                
                @@// do pcm 1022
                ldr    r11, [overlap_ptr1024], #-4 @@// load the overlap 1022                
                rsb    r8,   r8,  #0

                SMLAWT r11,  r8,  r10, r11         @@// window and get the r10 
                

                str   r11,  [pcm_out_ptr1024], #-4    
                @@/////////////////
                @@///////////////////////////////////////////////////
                subs   r14,  r14,  #1
                BGT    AAC_DEC_SBR_ARM_StopLongBlockProcessingAsm_PCM_LOOP
                ldmfd   sp!, {r6}
                
                mov   r14,  #256 
                
                add     win_ptr1024,   r6,  #512*4
                sub     win_ptr0,      win_ptr1024,  #4
                
                add     in_data_ptr0,  in_data_ptr0, #3*4
                sub     in_data_ptr512, in_data_ptr512, #3*4
                               
AAC_DEC_SBR_ARM_StopLongBlockProcessingAsm_OVLERLAP_LOOP:
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
                
                BGT    AAC_DEC_SBR_ARM_StopLongBlockProcessingAsm_OVLERLAP_LOOP                
                
                ldmfd   sp!, {r4-r12, pc}
                @ENDFUNC
                
                
                
                
                
		
		
		
		
		
k			       .req	   	 r14
over_lap     .req      r0
@//table_pcm .req      r1
table_lap    .req      r2
indata_ptr   .req      r3
@//output    .req      r4

sum11        .req      r5
sum22        .req      r6
sum33        .req      r7
sum44        .req      r8
ddd0         .req      r9
ddd1         .req      r10
tab_val1     .req      r11
overlap_ptr1 .req      r12
@@@@@@@@@@@
@@//PCM_LOOP1
output_ptr1  .req      r11		
		
		
		@@/* for AAC-LC & LTP output */		
		
		@//void AAC_DEC_LongBlockStopWindowLC_LtpAsm()	
		
		
		
		
		@@ // r0, the output data
		@@ // the input data fixed-point is S16.0
		@@ // and the output data fixed-point is S16.0
		@@ // r1, the data for overlaping
		@@ // r2, the table for window
		@@ // r3, the table for overlapping
AAC_DEC_LongBlockStopWindowLC_LtpAsm:  @FUNCTION
        .global	AAC_DEC_LongBlockStopWindowLC_LtpAsm
		@ // save the value for the following calculation
		stmfd 	sp!, {r4 - r12, r14}	

		ldr     r4, [r0, #4]
		ldr     table_lap, [r0, #8]
		ldr     indata_ptr, [r0, #12]
		
		ldr     over_lap, [r0]		
		
		mov     k,#448
		add     indata_ptr, indata_ptr, #2304
		add     overlap_ptr1, over_lap, #2304
		add     output_ptr1, r1, #576*2
		
		ldr     sum33, =0x00007FFF
LC_PCM_LOOP1:
		@@ // LOOP
		@@///////////////////////////////////////
        ldmia  over_lap!, {sum11-sum22}
        add    sum11, sum11, #64		
		mov    sum11, sum11, asr  #7	
			
		mov    r9,  sum11, asr #15
        teq    r9,  sum11, asr #31
        EORNE  sum11, sum33, sum11, asr #31	
        add    sum22, sum22, #64		
		mov    sum22, sum22, asr  #7		
		mov    r9,  sum22, asr #15
        teq    r9,  sum22, asr #31
        EORNE  sum22, sum33, sum22, asr #31
        strh   sum11,  [r1], #2
		strh   sum22,  [r1], #2		

		@@/////////////////////////////
		ldmia  overlap_ptr1!, {sum11-sum22}
		ldmia   indata_ptr!, {ddd0, ddd1}
		add     sum11, sum11, ddd0, asr #1
		add     sum22, sum22, ddd1, asr #1			
		add    sum11, sum11, #64		
		mov    sum11, sum11, asr  #7		
		mov    r9,  sum11, asr #15
        teq    r9,  sum11, asr #31
        EORNE  sum11, sum33, sum11, asr #31	
        add    sum22, sum22, #64		
		mov    sum22, sum22, asr  #7		
		mov    r9,  sum22, asr #15
        teq    r9,  sum22, asr #31
        EORNE  sum22, sum33, sum22, asr #31	  
        strh   sum11,  [output_ptr1], #2
		strh   sum22,  [output_ptr1], #2	
		
		@@/////////////////////////////////////
        ldmia  over_lap!, {sum11-sum22}
        add    sum11, sum11, #64		
		mov    sum11, sum11, asr  #7		
		mov    r9,  sum11, asr #15
        teq    r9,  sum11, asr #31
        EORNE  sum11, sum33, sum11, asr #31	
        add    sum22, sum22, #64		
		mov    sum22, sum22, asr  #7		
		mov    r9,  sum22, asr #15
        teq    r9,  sum22, asr #31
        EORNE  sum22, sum33, sum22, asr #31
        strh   sum11,  [r1], #2
		strh   sum22,  [r1], #2		

		@@/////////////////////////////
		ldmia  overlap_ptr1!, {sum11-sum22}
		ldmia   indata_ptr!, {ddd0, ddd1}
		add     sum11, sum11, ddd0, asr #1
		add     sum22, sum22, ddd1, asr #1			
		add    sum11, sum11, #64		
		mov    sum11, sum11, asr  #7		
		mov    r9,  sum11, asr #15
        teq    r9,  sum11, asr #31
        EORNE  sum11, sum33, sum11, asr #31	
        add    sum22, sum22, #64		
		mov    sum22, sum22, asr  #7		
		mov    r9,  sum22, asr #15
        teq    r9,  sum22, asr #31
        EORNE  sum22, sum33, sum22, asr #31	  
        strh   sum11,  [output_ptr1], #2
		strh   sum22,  [output_ptr1], #2
				
		@@ //control
		subs k, k, #4
		bne  LC_PCM_LOOP1
		
		
		@@ // END of PCM_LOOP1
@sum11        .req      r5
@sum22        .req      r6
@sum33        .req      r7
@sum44        .req      r8
@ddd0         .req      r9
@ddd1         .req      r10
@tab_val1     .req      r11
@overlap_ptr1 .req      r12		
		
		sub     indata_ptr, indata_ptr, #2304
		mov     k,#128
LC_PCM_LOOP2:
		ldmia   indata_ptr!, {ddd0, ddd1}
		ldmia   over_lap!, {sum11 - sum22}
		ldmia   r4!, {tab_val1, overlap_ptr1}
		
		SMLAWT  sum11, ddd0, tab_val1, sum11
		SMLAWB  sum22, ddd1, tab_val1, sum22
		
		
		add    sum11, sum11, #64		
		mov    sum11, sum11, asr  #7		
		mov    r9,  sum11, asr #15
        teq    r9,  sum11, asr #31
        EORNE  sum11, sum33, sum11, asr #31	
        add    sum22, sum22, #64		
		mov    sum22, sum22, asr  #7		
		mov    r9,  sum22, asr #15
        teq    r9,  sum22, asr #31
        EORNE  sum22, sum33, sum22, asr #31
        strh   sum11,  [r1], #2
		strh   sum22,  [r1], #2		
		
		
		
		@@///////////////////////////////		
		ldmia   indata_ptr!, {ddd0, ddd1}
		ldmia   over_lap!, {sum11 - sum22}
		
		SMLAWT  sum11, ddd0, overlap_ptr1, sum11
		SMLAWB  sum22, ddd1, overlap_ptr1, sum22
		
		
		add    sum11, sum11, #64		
		mov    sum11, sum11, asr  #7		
		mov    r9,  sum11, asr #15
        teq    r9,  sum11, asr #31
        EORNE  sum11, sum33, sum11, asr #31	
        add    sum22, sum22, #64		
		mov    sum22, sum22, asr  #7		
		mov    r9,  sum22, asr #15
        teq    r9,  sum22, asr #31
        EORNE  sum22, sum33, sum22, asr #31
        strh   sum11,  [r1], #2
		strh   sum22,  [r1], #2	
		@@ //control
		subs    k, k, #4		
		bne    LC_PCM_LOOP2
@sum11        .req      r5
@sum22        .req      r6
@sum33        .req      r7
@sum44        .req      r8
@ddd0         .req      r9
@ddd1         .req      r10
@tab_val1     .req      r11
@overlap_ptr1 .req      r12			
		
		
		@@ // end of PCM_LOOP2
		add     indata_ptr, indata_ptr, #1792
		mov     k, #1024
		sub     over_lap, over_lap, #2304
LC_PCM_LOOP3:
		@@ // LOOP
		ldr    overlap_ptr1, [table_lap], #-4
		ldr    tab_val1, [table_lap], #-4		
		ldmia  indata_ptr!, {ddd0, ddd1}

		SMULWB sum11, ddd0, overlap_ptr1
		SMULWT sum22, ddd1, overlap_ptr1
		ldmia  indata_ptr!, {ddd0, ddd1}

		SMULWB sum33, ddd0, tab_val1
		SMULWT sum44, ddd1, tab_val1
		stmia  over_lap!, {sum11-sum44}
		ldr    overlap_ptr1, [table_lap], #-4
		ldr    tab_val1, [table_lap], #-4		
		ldmia  indata_ptr!, {ddd0, ddd1}
		

		SMULWB sum11, ddd0, overlap_ptr1
		SMULWT sum22, ddd1, overlap_ptr1
		ldmia  indata_ptr!, {ddd0, ddd1}

		SMULWB sum33, ddd0, tab_val1
		SMULWT sum44, ddd1, tab_val1
		
		stmia  over_lap!, {sum11-sum44}
		@@ //control
		subs k, k, #8		
		bne LC_PCM_LOOP3
				
		@ // load the following value for quit the function
		ldmfd	sp!, {r4 - r12, pc}
				
		@ENDFUNC
		
		
		
		
		
		.end@END