@// AAC-LC tns_ar_filter
@// author: reed.zhang
@// input: SBR synthesis filter bank processing
@//     r0: output sample pointer
@// r1: the input overlap
@// r2: the tablr for output data
@// r3: the table for overlapping  
@// purpose:

@// define the register name 
spectrum_ptr      .req      r0
lpc_ptr           .req      r1
state_ptr         .req      r2

para              .req      r3

l_d0              .req      r4
l_d1              .req      r5


sum_l             .req      r7
sum_h             .req      r6
tmp_state_ptr     .req      r8
state_index       .req      r9
tmp_lpc_ptr       .req      r10

data0             .req      r11
data1             .req      r12

order             .req      r14
                .text  @AREA    PROGRAM, CODE, READONLY                         
                .arm@CODE32                  
                @//AACTnsArFilterAsm(
                @//                       int32_t   *spectrum_ptr
                @//                       int32_t   *lpc_ptr
                @//                       int32_t   *state_ptr
                @//                       int32_t    para
                @//)    
                @//  para: [0:15], size, para:[16:23], order,  para:[24:31], inc
                

AACTnsArFilterAsm:@ FUNCTION
                .global  AACTnsArFilterAsm
                @ // save the value for the following calculation
               stmfd   sp!, {r4 - r12, r14}          
               mov       state_index,  #0
SIZE_LOOP:
               add       tmp_state_ptr,  state_ptr,   state_index,  lsl  #2
               add       tmp_lpc_ptr,  lpc_ptr,   #4
               mov       sum_l,  #0
               mov       sum_h,  #0
               mov        order,  para, lsr #16 
               ands        order,  order,  #30               
               beq     ORDER_LOOP_NEXT
               @@//////////////////
ORDER_LOOP:
               @@//////////////////
               ldmia        tmp_state_ptr!, {data0, data1}
               ldmia        tmp_lpc_ptr!,   {l_d0, l_d1}
               SMLAL     sum_l,  sum_h,    data0,  l_d0               
               SMLAL     sum_l,  sum_h,    data1,  l_d1               
               @@//////////////////
               subs  order, order, #2
               BGT   ORDER_LOOP
ORDER_LOOP_NEXT:
               mov        order,  para, lsr #16     
               ands       data0,  order,  #1
               ldrne       data0,     [tmp_state_ptr],  #4
               ldrne       data1,     [tmp_lpc_ptr],    #4
               SMLALNE     sum_l,  sum_h,    data0,  data1                     
               @@//////////////////               
               ldr      sum_l,  [spectrum_ptr]
               and       order,  order,  #0xFF
               sub    sum_l,   sum_l,   sum_h,  lsl   #4
               @@////////////////// 
               cmp      state_index,    #1
               ADDLO    state_index,    order,   #-1
               ADDHS    state_index,    state_index, #-1                 
               add       tmp_state_ptr,  state_ptr,   state_index,  lsl  #2
               str       sum_l,   [tmp_state_ptr]
               str       sum_l,   [tmp_state_ptr, order,  lsl  #2]
               str       sum_l,   [spectrum_ptr], para,  asr #22    
               @@//loop control
               sub   para, para,  #1
               movs   data0,  para,   lsl #16
               BGT   SIZE_LOOP
               @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@                       
               ldmfd   sp!, {r4 - r12, pc}                                
               @ENDFUNC
               
               
               
               
               @/////////////////////////////////////////////////
               @//void AAC_DEC_TnsAnalyzeFilterAsm(
               @//                                int32   *spectrum_ptr,
               @//                                int32   *lpc_ptr,
               @//                                int32   *state_ptr,
               @//                                int32    para)@
               @//    
               @//  para: [0:15], size, para:[16:23], inc,  para:[24:31], order
                

AAC_DEC_TnsAnalyzeFilterAsm:@  FUNCTION
               .global  AAC_DEC_TnsAnalyzeFilterAsm
                @ // save the value for the following calculation
               stmfd   sp!, {r4 - r12, r14}    
      
               mov       state_index,  #0
LTP_SIZE_LOOP:
               add       tmp_state_ptr,  state_ptr,   state_index,  lsl  #2
               add       tmp_lpc_ptr,  lpc_ptr,   #4
               mov       sum_l,  #0
               mov       sum_h,  #0
               mov       order,  para, lsr #16 
               ands      order,  order,  #30
               
               beq     LTP_TNS_ORDER_LOOP_NEXT
               @@//////////////////
LTP_ORDER_LOOP:        
                      
               @@//////////////////
               ldmia     tmp_state_ptr!, {data0, data1}
               ldmia     tmp_lpc_ptr!,   {l_d0, l_d1}
               SMLAL     sum_l,  sum_h,    data0,  l_d0               
               SMLAL     sum_l,  sum_h,    data1,  l_d1
               
               @@//////////////////
               subs  order, order, #2
               BGT   LTP_ORDER_LOOP
LTP_TNS_ORDER_LOOP_NEXT:                              
               @//ands        order,  r14,  #0x1
               mov        order,  para, lsr #16     
               ands       order,  order,  #1

               ldrne       data0,     [tmp_state_ptr],  #4
               ldrne       data1,     [tmp_lpc_ptr],    #4
               SMLALNE     sum_l,  sum_h,    data0,  data1               
               
               @@//////////////////               
               mov      sum_l,  sum_l,  lsr #28               
               mov      sum_h,  sum_h,  lsl  #4
               orr      sum_l,  sum_h,   sum_l                              
               ldr      sum_h,  [spectrum_ptr]
               
               
               add      sum_l,   sum_l,   sum_h                         
               
               @@//////////////////                    
               mov       order,  para, lsr #16     
               and       order,  order,  #0xFF
               
               cmp      state_index,    #1
               ADDLO    state_index,    order,   #-1
               ADDHS    state_index,    state_index, #-1  
               
               add       tmp_state_ptr,  state_ptr,   state_index,  lsl  #2
               str       sum_h,   [tmp_state_ptr]
               str       sum_h,   [tmp_state_ptr, order,  lsl  #2]
               str       sum_l,   [spectrum_ptr], para,  asr #22    
               @@//loop control
               sub   para, para,  #1
               movs   data0,  para,   lsl #16
               BGT   LTP_SIZE_LOOP
               @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@                       
               ldmfd   sp!, {r4 - r12, pc}                                
               @ENDFUNC
               
               
               
               @//int32 AAC_DEC_SinglePtReQuanAsm(
                @//                              int32                       spec,
                @//                              int32                       scf,
                @//                              int32                       sgn
                @//                              )@
iq_table_ptr   .req  r14
q_spec          .req   r0
                .extern AAC_iq_table
                .extern AAC_iq_power_TAB_F_n
AAC_DEC_SinglePtReQuanAsm:@   FUNCTION
                @ // save the value for the following calculation
                .global  AAC_DEC_SinglePtReQuanAsm                
                stmfd   sp!, {r14}
                        ldr     iq_table_ptr,  =AAC_iq_table                    
    	        @@//   4/3 de-quantization
    	        cmp     q_spec,  #256
    	        ldrlt      q_spec,  [iq_table_ptr,  q_spec, lsl #2]    	        
    	        movlt    q_spec,  q_spec,  lsl #2
    	        blt     AAC_DEC_SinglePtReQuanAsm_SCALE

    	        cmp     q_spec,  #2048
    	        bge     AAC_DEC_SinglePtReQuanAsmGE2048
    	        @@/* < 2048 */
    	        and      r12,  q_spec,  #0x7
    	        mov     r3,    q_spec,  lsr  #3
    	        add     r3,     iq_table_ptr,  r3,  lsl   #2
                        ldr       r14,   [r3],  #4
                        ldr       r3,   [r3]
   	        
    	        cmp     r12,    #4
    	        ble     AAC_DEC_SinglePtReQuanAsmLT2048LE4
    	        sub     r12,    r12,  #8   @@// r12: t - 8
    	        sub     r14,    r3,  r14   @@// x2 - x1
    	        mov     r14,    r14,  lsl   #3
    	        add     r14,    r14,  #181
    	        sub     r14,    r14,  q_spec,   lsr #4  @@// r14: x2 - x1 - fx
    	        mul     r12,    r12,  r14
    	        add     q_spec,    r14,  r3, lsl  #6 	            	        
    	        B       AAC_DEC_SinglePtReQuanAsm_SCALE
AAC_DEC_SinglePtReQuanAsmLT2048LE4:
    	        sub     r3,    r3,  r14   @@// x2 - x1
    	        mov     r3,    r3,  lsl   #3
    	        sub     r3,    r3,  #181
    	        add     r3,    r3,  q_spec,   lsr #4  @@// r14: x2 - x1 - fx
    	        mul     r3,    r12,  r3
    	        add     q_spec,    r3,  r14, lsl  #6    	       	        
     
    	        B       AAC_DEC_SinglePtReQuanAsm_SCALE
AAC_DEC_SinglePtReQuanAsmGE2048:
    	        @@/* >= 2048*/
    	        and     r12,    q_spec,  #0x3F
    	        mov     r3,    q_spec,  lsr  #6
    	        add     r3,    iq_table_ptr,  r3,  lsl   #2
    	        ldr       r14,   [r3],  #4
                        ldr       r3,   [r3]
   	        
    	        cmp     r12,    #32
    	        ble    AAC_DEC_SinglePtReQuanAsmGE2048LE32
    	        sub     r12,    r12,  #64   @@// r12: t - 64
    	        sub     r14,    r3,  r14   @@// x2 - x1
    	        mov     r14,    r14,  lsl   #4
    	        add     r14,    r14,  #420
    	        sub     r14,    r14,  q_spec,   lsr #5  @@// r14: x2 - x1 - fx
    	        mul     r14,    r12,  r14
    	        add     q_spec,    r14,  r3, lsl  #10       	        
	        
    	        B       AAC_DEC_SinglePtReQuanAsm_SCALE
AAC_DEC_SinglePtReQuanAsmGE2048LE32:
    	        sub     r3,    r3,  r14   @@// x2 - x1
    	        mov     r3,    r3,  lsl   #4
    	        sub     r3,    r3,  #420
    	        add     r3,    r3,  q_spec,   lsr #5  @@// r14: x2 - x1 - fx
    	        mul     r3,    r12,  r3
    	        add     q_spec,    r3,  r14, lsl  #10       	        
	            	        
AAC_DEC_SinglePtReQuanAsm_SCALE:
    	        @@/* scale */   
                        ldr        r14,   =AAC_iq_power_TAB_F_n
                        mul      q_spec,  r2,   q_spec
                        and      r2,      r1,       #0x3
                        ldr        r2,      [r14,  r2,  lsl #2]
                        mov     r1,   r1,   lsr   #2
                        smull    r3,    q_spec,    r2,  q_spec
                        subs     r1,   r1,    #9
                        rsblt     r1,   r1,  #0
                        movlt    r0,    r0,   asr   r1
                        movge  r0,    r0,   lsl   r1
                
                ldmfd   sp!, {pc}
                @ENDFUNC
               
               
               
               .end@END
               
               
               
               
