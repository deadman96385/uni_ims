@// AAC-LC synthesis filter bank
@// author: reed.zhang
@// input: SBR synthesis filter bank processing
@//	r0: input ovlap data and ouput overlap data
@// r1: pcm filtler
@// r2: overlap filter
@// r3: input data
@// purpose:

@// define the register name 
s_k			     .req	     r14
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
		.text@AREA	PROGRAM, CODE, READONLY				
		.arm@CODE32			
		@//short_block_only_window(
		@//                       int32_t   *overlap_data
		@//                       int16_t   *pcm_filter
		@//                       int16_t   *overlap_filter
		@//                       int16_t   *input_data
		@//)	
		
		
		@@ // r0, the output data
		@@ // the input data fixed-point is S16.0
		@@ // and the output data fixed-point is S16.0
		@@ // r1, the data for overlaping
		@@ // r2, the table for window
		@@ // r3, the table for overlapping
short_block_only_window: @FUNCTION
		.global	short_block_only_window
		
		stmfd 	sp!, {r4 - r12, r14}
		
		mov  s_k, #0
		
		
		

@@s_pcm_tab    .req      r1
@@s_lap_tab    .req      r2
@@s_in_data    .req      r3
@@s_out_put    .req      r4		
@@s_lap        .req      r0		
		ldr  s_pcm_tab, [r0, #4]
		ldr  s_lap_tab, [r0, #8]
		ldr  s_in_data, [r0, #12]
		ldr  s_out_put, [r0, #16]
		
		ldr  s_lap, [r0]
		
		ldr  satutation, =0x00007FFF
PCM_FIRST:
		@@ // LOOP
		ldmia s_lap!, {sum11, sum22}
		@//mov   sum11, sum11, asr #15
		@//mov   sum22, sum22, asr #15
		
		@//mov    ddd0,  sum11, asr #15
        @//teq    ddd0,  sum11, asr #31
        @//EORNE  sum11, satutation, sum11, asr #31		

		@//mov    ddd0,  sum22, asr #15
        @//teq    ddd0,  sum22, asr #31
        @//EORNE  sum22, satutation, sum22, asr #31        
        
        str   sum11, [s_out_put], #4
        str   sum22, [s_out_put], #4	
		
		@@ // control
		add    s_k, s_k, #2
		cmp    s_k, #448
		bcc    PCM_FIRST
		@@ // END OF PCM_FIRST
		mov    s_k, #0
		
PCM_SECOND:
		@@ // LOOP
		ldmia   s_lap!, {sum11, sum22}
		ldr     ddd0, [s_in_data], #4
		ldr     tab_val0, [s_pcm_tab], #4
		
		SMLAWT  sum11, ddd0, tab_val0, sum11
		ldr     ddd0, [s_in_data], #4

                  str   sum11, [s_out_put], #4
		SMLAWB  sum22, ddd0, tab_val0, sum22
		
		@//mov   sum11, sum11, asr #15
		@//mov   sum22, sum22, asr #15
		@//mov    ddd0,  sum11, asr #15

        @//teq    ddd0,  sum11, asr #31
        @//EORNE  sum11, satutation, sum11, asr #31		
		@//mov    ddd0,  sum22, asr #15
        
        @//teq    ddd0,  sum22, asr #31
        @//EORNE  sum22, satutation, sum22, asr #31
        str   sum22, [s_out_put], #4
		@@ // control
		add    s_k, s_k, #2
		cmp    s_k, #128
		bcc    PCM_SECOND		
		@@ // END OF PCM_SECOND
		
		mov    s_k, #0
		add    s_pcm_tab, s_lap_tab, #252
		add    in_data_ptr2, s_in_data, #512
PCM_THIRD:
		@@ // LOOP 
		ldr    ddd0, [s_in_data], #4
		ldr    ddd1, [in_data_ptr2], #4
		ldmia  s_lap!, {sum11, sum22}
		ldr    tab_val0, [s_pcm_tab], #-4
		ldr    tab_val1, [s_lap_tab], #4
		
		SMLAWB sum11, ddd0, tab_val0, sum11
		ldr    ddd0, [s_in_data], #4
		
		SMLAWT sum22, ddd0, tab_val0, sum22
		SMLAWT sum11, ddd1, tab_val1, sum11		
		
		ldr    ddd1, [in_data_ptr2], #4
		SMLAWB sum22, ddd1, tab_val1, sum22
		
		@//mov   sum11, sum11, asr #15
		@//mov   sum22, sum22, asr #15
		@//mov    ddd0,  sum11, asr #15

        @//teq    ddd0,  sum11, asr #31
        @//EORNE  sum11, satutation, sum11, asr #31		
		@//mov    ddd0,  sum22, asr #15
        
        @//teq    ddd0,  sum22, asr #31
        @//EORNE  sum22, satutation, sum22, asr #31
        str   sum11, [s_out_put], #4
        str   sum22, [s_out_put], #4
		@@ // control
		add    s_k, s_k, #2
		cmp    s_k, #128
		bcc    PCM_THIRD		
		@@ // END OF PCM_THIRD
		
		mov    s_k, #0
		add    s_in_data, s_in_data, #1024
		sub    s_lap_tab, s_lap_tab, #4
		add    s_pcm_tab, s_pcm_tab, #4
PCM_FOURTH:
		@@ // LOOP
		ldr    ddd0, [s_in_data], #4
		ldr    ddd1, [in_data_ptr2], #4
		ldmia  s_lap!, {sum11, sum22}
		ldr    tab_val1, [s_lap_tab], #-4
		ldr    tab_val0, [s_pcm_tab], #4
		
		SMLAWT sum11, ddd0, tab_val0, sum11
		ldr    ddd0, [s_in_data], #4
		
		SMLAWB sum22, ddd0, tab_val0, sum22
		SMLAWB sum11, ddd1, tab_val1, sum11		
		
		ldr    ddd1, [in_data_ptr2], #4
		SMLAWT sum22, ddd1, tab_val1, sum22
		
		@//mov   sum11, sum11, asr #15
		@//mov   sum22, sum22, asr #15
		@//mov    ddd0,  sum11, asr #15

        @//teq    ddd0,  sum11, asr #31
        @//EORNE  sum11, satutation, sum11, asr #31		
		@//mov    ddd0,  sum22, asr #15
        
        @//teq    ddd0,  sum22, asr #31
        @//EORNE  sum22, satutation, sum22, asr #31
        str   sum11, [s_out_put], #4
        str   sum22, [s_out_put], #4
		@@ // control
		add    s_k, s_k, #2
		cmp    s_k, #128
		bcc    PCM_FOURTH		
		@@ // END OF PCM_FOURTH		
				
		mov    s_k, #0
		add    s_lap_tab, s_lap_tab, #4
		sub    s_pcm_tab, s_pcm_tab, #4
		add    in_data_ptr2, in_data_ptr2, #1024
PCM_FIFTH:
		@@ // LOOP
		ldr    ddd0, [s_in_data], #4
		ldr    ddd1, [in_data_ptr2], #4
		ldmia  s_lap!, {sum11, sum22}
		ldr    tab_val0, [s_pcm_tab], #-4
		ldr    tab_val1, [s_lap_tab], #4
		
		SMLAWB sum11, ddd0, tab_val0, sum11
		ldr    ddd0, [s_in_data], #4
		
		SMLAWT sum22, ddd0, tab_val0, sum22
		SMLAWT sum11, ddd1, tab_val1, sum11		
		
		ldr    ddd1, [in_data_ptr2], #4
		SMLAWB sum22, ddd1, tab_val1, sum22
		
		@//mov   sum11, sum11, asr #15
		@//mov   sum22, sum22, asr #15
		@//mov    ddd0,  sum11, asr #15

        @//teq    ddd0,  sum11, asr #31
        @//EORNE  sum11, satutation, sum11, asr #31		
		@//mov    ddd0,  sum22, asr #15
        
        @//teq    ddd0,  sum22, asr #31
        @//EORNE  sum22, satutation, sum22, asr #31
        str   sum11, [s_out_put], #4
        str   sum22, [s_out_put], #4
		
		@@ // control
		add    s_k, s_k, #2
		cmp    s_k, #128
		bcc    PCM_FIFTH		
		@@ // END OF PCM_FIFTH	
		
		
		mov    s_k, #0
		@add    in_data_ptr2, in_data_ptr2, #256
		add    s_in_data, s_in_data, #1024
		
		sub    s_lap_tab, s_lap_tab, #4
		add    s_pcm_tab, s_pcm_tab, #4
PCM_SIXTH:
		@@ // LOOP 
		ldr    ddd1, [s_in_data], #4
		ldr    ddd0, [in_data_ptr2], #4
		ldmia  s_lap!, {sum11, sum22}
		ldr    tab_val0, [s_lap_tab], #-4
		ldr    tab_val1, [s_pcm_tab], #4
		
		SMLAWB sum11, ddd0, tab_val0, sum11
		ldr    ddd0, [in_data_ptr2], #4
		
		SMLAWT sum22, ddd0, tab_val0, sum22
		SMLAWT sum11, ddd1, tab_val1, sum11		
		
		ldr    ddd1, [s_in_data], #4
		SMLAWB sum22, ddd1, tab_val1, sum22
		
		@//mov   sum11, sum11, asr #15
		@//mov   sum22, sum22, asr #15
		@//mov    ddd0,  sum11, asr #15

        @//teq    ddd0,  sum11, asr #31
        @//EORNE  sum11, satutation, sum11, asr #31		
		@//mov    ddd0,  sum22, asr #15
        
        @//teq    ddd0,  sum22, asr #31
        @//EORNE  sum22, satutation, sum22, asr #31
        str   sum11, [s_out_put], #4
        str   sum22, [s_out_put], #4
        
		@@ // control
		add    s_k, s_k, #2
		cmp    s_k, #64
		bcc    PCM_SIXTH		
		@@ // END OF PCM_SIXTH	
		
		mov    s_k, #64
		@add    s_lap_tab, s_lap_tab, #4  @@// point to table start pos 
		@sub    s_pcm_tab, s_pcm_tab, #4  @@// point to table end pos
		sub    s_in_data, s_in_data, #512
		add    in_data_ptr2, s_in_data, #512
		
		sub    s_lap, s_lap, #4096
OVERLAP_FIRST:
		@@ // LOOP
		ldr    ddd0, [s_in_data], #4
		ldr    ddd1, [in_data_ptr2], #4
		ldr    tab_val0, [s_lap_tab], #-4
		ldr    tab_val1, [s_pcm_tab], #4
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
		bcc    OVERLAP_FIRST		
		@@ // END OF OVERLAP_FIRST
		
		mov    s_k, #128
		add    s_in_data, s_in_data, #512
		add    in_data_ptr2, s_in_data, #512
		add    tmp_ptr1,     in_data_ptr2, #512
		add    tmp_ptr2,     tmp_ptr1, #512
		
		add    s_lap_tab, s_lap_tab, #4
		sub    s_pcm_tab, s_pcm_tab, #4
OVERLAP_SECOND:
		ldr    tab_val0, [s_pcm_tab], #-4
		ldr    tab_val1, [s_lap_tab], #4
		
		@@// the first point
		ldr    ddd0, [s_in_data], #4
		ldr    ddd1, [in_data_ptr2], #4	
		
		@//mov    sum11, #0
		@//mov    sum22, #0
		
		SMULWB sum11, ddd0, tab_val0
		ldr    ddd0, [s_in_data], #4
		
		SMULWT sum22, ddd0, tab_val0
		SMLAWT sum11, ddd1, tab_val1, sum11		
		ldr    ddd1, [in_data_ptr2], #4	
		SMLAWB sum22, ddd1, tab_val1, sum22@/////////////
		stmia  s_lap!, {sum11, sum22}
		
		@@// the second point
		ldr    ddd0, [tmp_ptr1], #4
		ldr    ddd1, [tmp_ptr2], #4	
		
		@//mov    sum11, #0
		@//mov    sum22, #0
		
		SMULWB sum11, ddd0, tab_val0
		ldr    ddd0, [tmp_ptr1], #4
		
		SMULWT sum22, ddd0, tab_val0
		SMLAWT sum11, ddd1, tab_val1, sum11		
		ldr    ddd1, [tmp_ptr2], #4	
		SMLAWB sum22, ddd1, tab_val1, sum22
		str    sum11, [s_lap, #504]
		str    sum22, [s_lap, #508]       @@//////////////////
		
		@@// the third point
		ldr    ddd0, [tmp_ptr1, #1016]
		ldr    ddd1, [tmp_ptr2, #1016]
		
		@//mov    sum11, #0
		@//mov    sum22, #0
		
		SMULWB sum11, ddd0, tab_val0
		ldr    ddd0, [tmp_ptr1, #1020]
		
		SMULWT sum22, ddd0, tab_val0
		SMLAWT sum11, ddd1, tab_val1, sum11		
		ldr    ddd1, [tmp_ptr2, #1020]
		SMLAWB sum22, ddd1, tab_val1, sum22
		str    sum11, [s_lap, #1016]
		str    sum22, [s_lap, #1020]
		@@// the fourth point
		ldr    ddd0, [tmp_ptr1, #2040] @@///////////
		
		@//mov    sum11, #0
		@//mov    sum22, #0
		SMULWB sum11, ddd0, tab_val0
		ldr    ddd0, [tmp_ptr1, #2044]
		SMULWT sum22, ddd0, tab_val0
		
		str    sum11, [s_lap, #1528]
		str    sum22, [s_lap, #1532]
		@@ // control
		subs    s_k, s_k, #2		
		BNE    OVERLAP_SECOND		
		@@ // END OF OVERLAP_SECOND
		
		add   s_lap, s_lap, #1536
		mov   s_k, #448
		mov   sum11, #0
		mov   sum22, #0
		mov   ddd0, #0
		mov   ddd1, #0
OVERLAP_END:
		@@ // LOOP		
		
		stmia s_lap!, {sum11, sum22, ddd0, ddd1}
		@@ // control
		subs   s_k, s_k, #4		
		BNE   OVERLAP_END
		
		@ // load the following value for quit the function
		ldmfd	sp!, {r4 - r12, pc}
				
		@ENDFUNC
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		.end@END