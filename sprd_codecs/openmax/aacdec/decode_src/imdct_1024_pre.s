@// AAC-LC synthesis filter bank
@// author: reed.zhang
@// input: SBR synthesis filter bank processing
@//	r0: output sample pointer
@// r1: the input data
@// purpose:

@// define the register name 
in_ptr      .req      r0
tmp_ptr     .req      r1
tab_ptr     .req      r2


s_ptr2      .req      r11
t_ptr1      .req      r4
sin_cos0    .req      r5
sin_cos1    .req      r6
sin_cos2    .req      r7
sin_cos3	  .req  		r8
t0          .req      r10
t1          .req      r14
ss1         .req      r9
ss2         .req      r12
con_val     .req      r3
		.text@AREA	PROGRAM, CODE, READONLY				
		.arm@CODE32				
		.global	asm_imdct1024_pre
		@@ // r0, the in/output data
		@@ // the input data fixed-point is S18.14
		@@ // and the output data fixed-point is S16.0
		@@ // r1, the tmp buffer for calculating IMDCT
asm_imdct1024_pre:
		@ // save the value for the following calculation
		stmfd 	sp!, {r2 - r12, r14}
		
		@ // PRE_TWINDDLE	
		@ // after this model, the data save in tmp_ptr buffer (S19.13)
		
		cmp   con_val, #128
		addlo s_ptr2, in_ptr, #512
		addhs s_ptr2, in_ptr, #4096	
		
		
		sub s_ptr2, s_ptr2,  #4	
		
		mov t_ptr1, tmp_ptr
		
		

PRE_TWINDDLE:
		@ // function body
		@ //@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@		
		@//ldr   sin_cos0, [tab_ptr], #4
		ldmia    tab_ptr!, {sin_cos0-sin_cos3}
		@@/// 0
		ldr   t0, [in_ptr], #8
		ldr   t1, [s_ptr2], #-8
		@ // cal first point		
		SMULWT ss2, t0, sin_cos0
		SMULWT ss1, t1, sin_cos0
		rsb    t0, t0, #0
		SMLAWB ss1, t0, sin_cos0, ss1
		SMLAWB ss2, t1, sin_cos0, ss2
		@@/// 1
		ldr   t0, [in_ptr], #8
		ldr   t1, [s_ptr2], #-8
		stmia     t_ptr1!, {ss1, ss2}
		@ // cal first point		
		SMULWT ss2, t0, sin_cos1
		SMULWT ss1, t1, sin_cos1
		rsb    t0, t0, #0
		SMLAWB ss1, t0, sin_cos1, ss1
		SMLAWB ss2, t1, sin_cos1, ss2
		@@/// 2
		ldr   t0, [in_ptr], #8
		ldr   t1, [s_ptr2], #-8
		stmia     t_ptr1!, {ss1, ss2}
		@ // cal first point		
		SMULWT ss2, t0, sin_cos2
		SMULWT ss1, t1, sin_cos2
		rsb    t0, t0, #0
		SMLAWB ss1, t0, sin_cos2, ss1
		SMLAWB ss2, t1, sin_cos2, ss2
		@@/// 3
		ldr   t0, [in_ptr], #8
		ldr   t1, [s_ptr2], #-8
		stmia     t_ptr1!, {ss1, ss2}
		@ // cal first point		
		SMULWT ss2, t0, sin_cos3
		SMULWT ss1, t1, sin_cos3
		
		rsb    t0, t0, #0
		SMLAWB ss1, t0, sin_cos3, ss1
		SMLAWB ss2, t1, sin_cos3, ss2
				
		stmia     t_ptr1!, {ss1, ss2}
		subs con_val, con_val, #4	
		
		BGT  PRE_TWINDDLE
		@@ // end of PRE_TWINDDLE

		@ // load the following value for quit the function
		ldmfd	sp!, {r2 - r12, pc}
				
		.end@END