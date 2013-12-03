@// AAC-LC tns_ar_filter
@// author: reed.zhang
@// input: SBR synthesis filter bank processing
@//     r0: output sample pointer
@// r1: the input overlap
@// r2: the tablr for output data
@// r3: the table for overlapping  
@// purpose:
	
                @ ***** END LICENSE BLOCK *****  

	.text  @AREA	|.text|, CODE, READONLY
	
	.arm
                               
                @//int32 AAC_DEC_ARM_LongImdctPreAsm(
                @//                              int32   *in_out_ptr
                @//                              int32   *table_ptr
                @//                              )@
                

                @@ // r0, in_out_ptr
                @@ // r1, table_ptr
in_out_ptr    .req  r0
table_ptr     .req  r1
in_out_ptr511 .req  r2

AAC_DEC_ARM_LongImdctPreAsm:@   FUNCTION
                @ // save the value for the following calculation
                .global  AAC_DEC_ARM_LongImdctPreAsm
                stmfd   sp!, {r4-r12, r14}
                
                mov     r14,  #256
                add     in_out_ptr511, in_out_ptr, #1024*4
                sub     in_out_ptr511, in_out_ptr511, #4
                
AAC_DEC_ARM_LongImdctPreAsm_LOOP:
                @@/////////////////////////////////////
                ldmia   in_out_ptr,    {r3-r4}   @@// d0:  d1
                ldmda   in_out_ptr511, {r5-r6}   @@// d1:  d0
                ldmia   table_ptr!, {r7-r8}      @@// t0, t1
                @@// r9, r10, r11, r12
                @@/* first step */                
                sub      r9,   r6,   r3,  asr #1   @@// d1 + (d0>>1)
                add      r10,  r3,   r6,  asr #1   @@// d0 + (d1>>1)
                
                SMLAWT   r10,   r3,   r7,  r10                             
                rsb      r3,    r3,  #0
                SMLAWB   r9,    r3,   r7,  r9
                
                SMLAWT   r9,    r6,   r7,  r9
                SMLAWB   r10,   r6,   r7,  r10                
                
                @@/* second step */                
                rsb      r11,   r5,   r4,  asr #1   @@// d1 + (d0>>1)
                add      r12,   r4,   r5,  asr #1   @@// d0 + (d1>>1)
                
                SMLAWT   r12,   r5,   r8,  r12                             
                rsb      r5,    r5,  #0
                SMLAWB   r11,    r5,   r8,  r11
                
                SMLAWT   r11,    r4,   r8,  r11
                SMLAWB   r12,   r4,   r8,  r12    
                
                stmia    in_out_ptr!,    {r9-r10}
                stmda    in_out_ptr511!, {r11, r12}
                
                
                @@//////
                ldmia   in_out_ptr,    {r3-r4}   @@// d0:  d1
                ldmda   in_out_ptr511, {r5-r6}   @@// d1:  d0
                ldmia   table_ptr!, {r7-r8}      @@// t0, t1
                @@// r9, r10, r11, r12
                @@/* first step */                
                sub      r9,   r6,   r3,  asr #1   @@// d1 + (d0>>1)
                add      r10,  r3,   r6,  asr #1   @@// d0 + (d1>>1)
                
                SMLAWT   r10,   r3,   r7,  r10                             
                rsb      r3,    r3,  #0
                SMLAWB   r9,    r3,   r7,  r9
                
                SMLAWT   r9,    r6,   r7,  r9
                SMLAWB   r10,   r6,   r7,  r10                
                
                @@/* second step */                
                rsb      r11,   r5,   r4,  asr #1   @@// d1 + (d0>>1)
                add      r12,   r4,   r5,  asr #1   @@// d0 + (d1>>1)
                
                SMLAWT   r12,   r5,   r8,  r12                             
                rsb      r5,    r5,  #0
                SMLAWB   r11,    r5,   r8,  r11
                
                SMLAWT   r11,    r4,   r8,  r11
                SMLAWB   r12,   r4,   r8,  r12    
                
                stmia    in_out_ptr!, {r9-r10}
                stmda    in_out_ptr511!, {r11, r12}
                
                
                @@/////////////////////////////////////
                ldmia   in_out_ptr,    {r3-r4}   @@// d0:  d1
                ldmda   in_out_ptr511, {r5-r6}   @@// d1:  d0
                ldmia   table_ptr!, {r7-r8}      @@// t0, t1
                @@// r9, r10, r11, r12
                @@/* first step */                
                sub      r9,   r6,   r3,  asr #1   @@// d1 + (d0>>1)
                add      r10,  r3,   r6,  asr #1   @@// d0 + (d1>>1)
                
                SMLAWT   r10,   r3,   r7,  r10                             
                rsb      r3,    r3,  #0
                SMLAWB   r9,    r3,   r7,  r9
                
                SMLAWT   r9,    r6,   r7,  r9
                SMLAWB   r10,   r6,   r7,  r10                
                
                @@/* second step */                
                rsb      r11,   r5,   r4,  asr #1   @@// d1 + (d0>>1)
                add      r12,   r4,   r5,  asr #1   @@// d0 + (d1>>1)
                
                SMLAWT   r12,   r5,   r8,  r12                             
                rsb      r5,    r5,  #0
                SMLAWB   r11,    r5,   r8,  r11
                
                SMLAWT   r11,    r4,   r8,  r11
                SMLAWB   r12,   r4,   r8,  r12    
                
                stmia    in_out_ptr!, {r9-r10}
                stmda    in_out_ptr511!, {r11, r12}
                
                
                @@//////
                ldmia   in_out_ptr,    {r3-r4}   @@// d0:  d1
                ldmda   in_out_ptr511, {r5-r6}   @@// d1:  d0
                ldmia   table_ptr!, {r7-r8}      @@// t0, t1
                @@// r9, r10, r11, r12
                @@/* first step */                
                sub      r9,   r6,   r3,  asr #1   @@// d1 + (d0>>1)
                add      r10,  r3,   r6,  asr #1   @@// d0 + (d1>>1)
                
                SMLAWT   r10,   r3,   r7,  r10                             
                rsb      r3,    r3,  #0
                SMLAWB   r9,    r3,   r7,  r9
                
                SMLAWT   r9,    r6,   r7,  r9
                SMLAWB   r10,   r6,   r7,  r10                
                
                @@/* second step */                
                rsb      r11,   r5,   r4,  asr #1   @@// d1 + (d0>>1)
                add      r12,   r4,   r5,  asr #1   @@// d0 + (d1>>1)
                
                SMLAWT   r12,   r5,   r8,  r12                             
                rsb      r5,    r5,  #0
                SMLAWB   r11,    r5,   r8,  r11
                
                SMLAWT   r11,    r4,   r8,  r11
                SMLAWB   r12,   r4,   r8,  r12    
                
                stmia    in_out_ptr!, {r9-r10}
                stmda    in_out_ptr511!, {r11, r12}
                @@/////////////////////////////////////
                subs    r14,  r14, #4
                BGT     AAC_DEC_ARM_LongImdctPreAsm_LOOP
                
                
                ldmfd   sp!, {r4-r12, pc}
                @ENDFUNC
                
                
                
   	
    	        @//int32 AAC_DEC_ARM_LongImdctPostAsm(
                @//                              int32   *in_out_ptr
                @//                              int32   *table_ptr
                @//                              )@
                

                @@ // r0, in_out_ptr
                @@ // r1, table_ptr
in_out_ptr    .req  r0
table_ptr     .req  r1
in_out_ptr511 .req  r2

AAC_DEC_ARM_LongImdctPostAsm:@   FUNCTION
                @ // save the value for the following calculation
                .global  AAC_DEC_ARM_LongImdctPostAsm
                stmfd   sp!, {r4-r12, r14}
                
                mov     r14,  #256
                add     in_out_ptr511, in_out_ptr, #1024*4
                sub     in_out_ptr511, in_out_ptr511, #4
                
AAC_DEC_ARM_LongImdctPostAsm_LOOP:
                @@/////////////////////////////////////
                ldmia   in_out_ptr,    {r3-r4}   @@// d0:  d1
                ldmda   in_out_ptr511, {r5-r6}   @@// d1:  d0
                ldmia   table_ptr!, {r7-r8}      @@// t0, t1
                @@// r9, r10, r11, r12
                @@/* first step */                
                sub      r9,   r3,   r4,  asr #1   @@// d1 + (d0>>1)
                add      r10,  r4,   r3,  asr #1   @@// d0 + (d1>>1)
                
                SMLAWT   r10,   r4,   r7,  r10                             
                rsb      r4,    r4,  #0
                SMLAWB   r9,    r4,   r7,  r9
                
                SMLAWT   r9,    r3,   r7,  r9
                SMLAWB   r10,   r3,   r7,  r10                
                
                @@/* second step */                
                rsb      r11,   r6,   r5,  asr #1   @@// d1 + (d0>>1)
                add      r12,   r5,   r6,  asr #1   @@// d0 + (d1>>1)
                
                SMLAWT   r12,   r6,   r8,  r12                             
                rsb      r6,    r6,  #0
                SMLAWB   r11,   r6,   r8,  r11
                
                SMLAWT   r11,   r5,   r8,  r11
                SMLAWB   r12,   r5,   r8,  r12    
                
                stmia    in_out_ptr!, {r9-r10}
                stmda    in_out_ptr511!, {r11, r12}
                
                
                @@//////
                ldmia   in_out_ptr,    {r3-r4}   @@// d0:  d1
                ldmda   in_out_ptr511, {r5-r6}   @@// d1:  d0
                ldmia   table_ptr!, {r7-r8}      @@// t0, t1
                @@// r9, r10, r11, r12
                @@/* first step */                
                sub      r9,   r3,   r4,  asr #1   @@// d1 + (d0>>1)
                add      r10,  r4,   r3,  asr #1   @@// d0 + (d1>>1)
                
                SMLAWT   r10,   r4,   r7,  r10                             
                rsb      r4,    r4,  #0
                SMLAWB   r9,    r4,   r7,  r9
                
                SMLAWT   r9,    r3,   r7,  r9
                SMLAWB   r10,   r3,   r7,  r10                
                
                @@/* second step */                
                rsb      r11,   r6,   r5,  asr #1   @@// d1 + (d0>>1)
                add      r12,   r5,   r6,  asr #1   @@// d0 + (d1>>1)
                
                SMLAWT   r12,   r6,   r8,  r12                             
                rsb      r6,    r6,  #0
                SMLAWB   r11,   r6,   r8,  r11
                
                SMLAWT   r11,   r5,   r8,  r11
                SMLAWB   r12,   r5,   r8,  r12    
                
                stmia    in_out_ptr!, {r9-r10}
                stmda    in_out_ptr511!, {r11, r12}
                
                
                @@/////////////////////////////////////
                ldmia   in_out_ptr,    {r3-r4}   @@// d0:  d1
                ldmda   in_out_ptr511, {r5-r6}   @@// d1:  d0
                ldmia   table_ptr!, {r7-r8}      @@// t0, t1
                @@// r9, r10, r11, r12
                @@/* first step */                
                sub      r9,   r3,   r4,  asr #1   @@// d1 + (d0>>1)
                add      r10,  r4,   r3,  asr #1   @@// d0 + (d1>>1)
                
                SMLAWT   r10,   r4,   r7,  r10                             
                rsb      r4,    r4,  #0
                SMLAWB   r9,    r4,   r7,  r9
                
                SMLAWT   r9,    r3,   r7,  r9
                SMLAWB   r10,   r3,   r7,  r10                
                
                @@/* second step */                
                rsb      r11,   r6,   r5,  asr #1   @@// d1 + (d0>>1)
                add      r12,   r5,   r6,  asr #1   @@// d0 + (d1>>1)
                
                SMLAWT   r12,   r6,   r8,  r12                             
                rsb      r6,    r6,  #0
                SMLAWB   r11,   r6,   r8,  r11
                
                SMLAWT   r11,   r5,   r8,  r11
                SMLAWB   r12,   r5,   r8,  r12    
                
                stmia    in_out_ptr!, {r9-r10}
                stmda    in_out_ptr511!, {r11, r12}
                
                
                @@//////
                ldmia   in_out_ptr,    {r3-r4}   @@// d0:  d1
                ldmda   in_out_ptr511, {r5-r6}   @@// d1:  d0
                ldmia   table_ptr!, {r7-r8}      @@// t0, t1
                @@// r9, r10, r11, r12
                @@/* first step */                
                sub      r9,   r3,   r4,  asr #1   @@// d1 + (d0>>1)
                add      r10,  r4,   r3,  asr #1   @@// d0 + (d1>>1)
                
                SMLAWT   r10,   r4,   r7,  r10                             
                rsb      r4,    r4,  #0
                SMLAWB   r9,    r4,   r7,  r9
                
                SMLAWT   r9,    r3,   r7,  r9
                SMLAWB   r10,   r3,   r7,  r10                
                
                @@/* second step */                
                rsb      r11,   r6,   r5,  asr #1   @@// d1 + (d0>>1)
                add      r12,   r5,   r6,  asr #1   @@// d0 + (d1>>1)
                
                SMLAWT   r12,   r6,   r8,  r12                             
                rsb      r6,    r6,  #0
                SMLAWB   r11,   r6,   r8,  r11
                
                SMLAWT   r11,   r5,   r8,  r11
                SMLAWB   r12,   r5,   r8,  r12    
                
                stmia    in_out_ptr!, {r9-r10}
                stmda    in_out_ptr511!, {r11, r12}
                @@/////////////////////////////////////
                subs    r14,  r14, #4
                BGT     AAC_DEC_ARM_LongImdctPostAsm_LOOP
                
                
                ldmfd   sp!, {r4-r12, pc}
                @ENDFUNC    
                
                @//int32 AAC_DEC_ARM_LongIfftStep1Asm(
                @//                              int32   *in_out_ptr
                @//                              int32   *table_ptr
                @//                              )@
                

                @@ // r0, in_out_ptr
                @@ // r1, table_ptr
s1_in_out_ptr0    .req  r0
s1_table_ptr      .req  r1
s1_in_out_ptr512  .req  r2

                 
AAC_DEC_ARM_LongIfftStep1Asm:@   FUNCTION
                @ // save the value for the following calculation
                .global  AAC_DEC_ARM_LongIfftStep1Asm
                stmfd   sp!, {r4-r12, r14}
                
                add     s1_in_out_ptr512, s1_in_out_ptr0, #512*4
                @@//////////////////////////////////////////////
                
                ldmia   s1_in_out_ptr0,   {r3,r4}  @@// cc10, cc11
                ldmia   s1_in_out_ptr512, {r5,r6}  @@// cc20, cc21
                
                add     r7, r3,  r5   @@// sum1
                sub     r3, r3,  r5   @@// t20
                
                add     r8, r4,  r6   @@// sum2
                sub     r4, r4,  r6   @@// t21
                
                stmia   s1_in_out_ptr0!,   {r7,r8}
                stmia   s1_in_out_ptr512!, {r3,r4}
                
                @@// step 1
                mov   r14,  #63
AAC_DEC_ARM_LongIfftStep1Asm_LOOP1:
                @@////////////////////////////////////
                ldmia   s1_in_out_ptr0,   {r3,r4}          @@// cc10, cc11
                ldmia   s1_in_out_ptr512, {r5,r6, r7, r8}  @@// cc20, cc21                
                ldmia   s1_table_ptr!, {r9, r12}   @@// table_val                
                add     r10, r3,  r5   @@// sum1
                sub     r3,  r3,  r5   @@// t20--r3, d1                
                add     r11, r4,  r6   @@// sum2
                sub     r4,  r4,  r6   @@// t21--r4, d0
                stmia   s1_in_out_ptr0!,   {r10,r11}                
                @@///////////////////////////////
                sub     r10,   r3,  r4, asr #1  @@// tmp0
                add     r11,   r4,  r3, asr #1  @@// tmp1                
                SMLAWT  r11,   r4,  r9,  r11
                rsb     r4,    r4, #0
                SMLAWB  r10,   r4,  r9,  r10                
                SMLAWB  r11,   r3,  r9,  r11
                SMLAWT  r10,   r3,  r9,  r10            
                                    
                ldmia   s1_in_out_ptr0,    {r3,r4}          @@// cc10, cc11      
                stmia   s1_in_out_ptr512!, {r10,r11}
                          
                add     r5,  r3,  r7   @@// sum1
                sub     r3,  r3,  r7   @@// t20--r3, d1                
                add     r6, r4,  r8   @@// sum2             
                
                sub     r4,  r4,  r8   @@// t21--r4, d0
                stmia   s1_in_out_ptr0!,   {r5,r6}
                @@///////////////////////////////
                sub     r10,   r3,  r4, asr #1  @@// tmp0
                add     r11,   r4,  r3, asr #1  @@// tmp1                
                SMLAWT  r11,   r4,  r12,  r11
                rsb     r4,    r4, #0
                SMLAWB  r10,   r4,  r12,  r10                
                SMLAWB  r11,   r3,  r12,  r11
                SMLAWT  r10,   r3,  r12,  r10
                
                
                ldmia   s1_in_out_ptr0,   {r3,r4}  @@// cc10, cc11
                stmia   s1_in_out_ptr512!, {r10,r11}
                
                ldmia   s1_in_out_ptr512, {r5,r6}  @@// cc20, cc21                
                ldr     r9,   [s1_table_ptr], #4   @@// table_val 
                                
                add     r10, r3,  r5   @@// sum1
                sub     r3,  r3,  r5   @@// t20--r3, d1                
                add     r11, r4,  r6   @@// sum2
                sub     r4,  r4,  r6   @@// t21--r4, d0
                stmia   s1_in_out_ptr0!,   {r10,r11}                
                @@///////////////////////////////
                sub     r10,   r3,  r4, asr #1  @@// tmp0
                add     r11,   r4,  r3, asr #1  @@// tmp1                
                SMLAWT  r11,   r4,  r9,  r11
                rsb     r4,    r4, #0
                SMLAWB  r10,   r4,  r9,  r10                
                SMLAWB  r11,   r3,  r9,  r11
                SMLAWT  r10,   r3,  r9,  r10   
    	        
    	        subs r14, r14, #3
    	        stmia   s1_in_out_ptr512!, {r10,r11}       	        
    	        @@////////////////////////////////////    	        
    	        BGT  AAC_DEC_ARM_LongIfftStep1Asm_LOOP1  
    	        @@// step 2
    	        ldmia   s1_in_out_ptr0,   {r3,r4}  @@// cc10, cc11
                ldmia   s1_in_out_ptr512, {r5,r6}  @@// cc20, cc21                
                add     r7, r3,  r5   @@// sum1
                sub     r3, r3,  r5   @@// t20
                
                add     r8, r4,  r6   @@// sum2
                sub     r4, r4,  r6   @@// t21                
                stmia   s1_in_out_ptr0!,   {r7,r8}                
                ldr     r12,   =-19195
                add     r6,   r3,  r4
                sub     r5,   r3,  r4
                
                SMLAWB  r5,   r5,   r12,  r5
                SMLAWB  r6,   r6,   r12,  r6
                sub     s1_table_ptr,  s1_table_ptr,  #4
                mov     r14,  #63
                stmia   s1_in_out_ptr512!, {r5,r6}                
                
AAC_DEC_ARM_LongIfftStep1Asm_LOOP2:
                ldmia   s1_in_out_ptr0,   {r3,r4}          @@// cc10, cc11
                ldmia   s1_in_out_ptr512, {r5,r6, r7, r8}  @@// cc20, cc21                
                ldmda   s1_table_ptr!, {r9, r12}   @@// table_val                
                add     r10, r3,  r5   @@// sum1
                sub     r3,  r3,  r5   @@// t20--r3, d1                
                add     r11, r4,  r6   @@// sum2
                sub     r4,  r4,  r6   @@// t21--r4, d0
                stmia   s1_in_out_ptr0!,   {r10,r11}                
                @@///////////////////////////////
                rsb     r10,   r4,  r3, asr #1  @@// tmp0
                add     r11,   r3,  r4, asr #1  @@// tmp1
                                
                SMLAWB  r11,   r4,  r12,  r11
                rsb     r4,    r4, #0
                SMLAWT  r10,   r4,  r12,  r10                
                SMLAWT  r11,   r3,  r12,  r11
                SMLAWB  r10,   r3,  r12,  r10            
                                    
                ldmia   s1_in_out_ptr0,    {r3,r4}          @@// cc10, cc11      
                stmia   s1_in_out_ptr512!, {r10,r11}
                          
                add     r5,  r3,  r7   @@// sum1
                sub     r3,  r3,  r7   @@// t20--r3, d1                
                add     r6, r4,  r8   @@// sum2             
                
                sub     r4,  r4,  r8   @@// t21--r4, d0
                stmia   s1_in_out_ptr0!,   {r5,r6}
                @@///////////////////////////////
                rsb     r10,   r4,  r3, asr #1  @@// tmp0
                add     r11,   r3,  r4, asr #1  @@// tmp1                
                SMLAWB  r11,   r4,  r9,  r11
                rsb     r4,    r4, #0
                SMLAWT  r10,   r4,  r9,  r10                
                SMLAWT  r11,   r3,  r9,  r11
                SMLAWB  r10,   r3,  r9,  r10               
                
                ldmia   s1_in_out_ptr0,   {r3,r4}  @@// cc10, cc11
                stmia   s1_in_out_ptr512!, {r10,r11}
                
                ldmia   s1_in_out_ptr512, {r5,r6}  @@// cc20, cc21                
                ldr     r9,   [s1_table_ptr], #-4   @@// table_val 
                
                add     r10, r3,  r5   @@// sum1
                sub     r3,  r3,  r5   @@// t20--r3, d1                
                add     r11, r4,  r6   @@// sum2
                sub     r4,  r4,  r6   @@// t21--r4, d0
                stmia   s1_in_out_ptr0!,   {r10,r11}                
                @@///////////////////////////////
                rsb     r10,   r4,  r3, asr #1  @@// tmp0
                add     r11,   r3,  r4, asr #1  @@// tmp1                
                SMLAWB  r11,   r4,  r9,  r11
                rsb     r4,    r4, #0
                SMLAWT  r10,   r4,  r9,  r10                
                SMLAWT  r11,   r3,  r9,  r11
                SMLAWB  r10,   r3,  r9,  r10   
    	        
    	        subs r14, r14, #3
    	        stmia   s1_in_out_ptr512!, {r10,r11}       	                
    	        @@////////////////////////////////////    	        
    	        BGT  AAC_DEC_ARM_LongIfftStep1Asm_LOOP2
    	        
    	        @/* step 3*/    	        
    	        ldmia   s1_in_out_ptr0,   {r3,r4}  @@// cc10, cc11
                ldmia   s1_in_out_ptr512, {r5,r6}  @@// cc20, cc21                
                add     r7, r3,  r5   @@// sum1
                sub     r9, r3,  r5   @@// t20                
                add     r8, r4,  r6   @@// sum2
                sub     r4, r6,  r4   @@// t21
                
                stmia   s1_in_out_ptr0!,   {r7,r8}  
                stmia   s1_in_out_ptr512!, {r4,r9}
    	        
    	        mov     r14,  #63
    	        add     s1_table_ptr,  s1_table_ptr,  #4
AAC_DEC_ARM_LongIfftStep1Asm_LOOP3:
                ldmia   s1_in_out_ptr0,   {r3,r4}          @@// cc10, cc11
                ldmia   s1_in_out_ptr512, {r5,r6, r7, r8}  @@// cc20, cc21                
                ldmia   s1_table_ptr!, {r9, r12}   @@// table_val                
                add     r10, r3,  r5   @@// sum1
                sub     r3,  r3,  r5   @@// t20--r3, d1                
                add     r11, r4,  r6   @@// sum2
                sub     r4,  r6,  r4   @@// t21--r4, -d0(rsb)
                stmia   s1_in_out_ptr0!,   {r10,r11}    
    	        sub     r5,  r4,   r3,  asr #1 @// tmp0
    	        add     r6,  r3,   r4,  asr #1 @// tmp1
    	        
    	        SMLAWT  r6,  r3,  r9, r6
    	        rsb     r3,  r3,  #0
    	        SMLAWB  r5,  r3,  r9, r5    	        
    	        SMLAWB  r6,  r4,  r9, r6    	        
    	        SMLAWT  r5,  r4,  r9, r5
    	        
    	        ldmia   s1_in_out_ptr0,    {r3, r4} 
    	        stmia   s1_in_out_ptr512!, {r5, r6}
    	        
    	        add     r10, r3,  r7   @@// sum1
                sub     r3,  r3,  r7   @@// t20--r3, d1                
                add     r11, r4,  r8   @@// sum2
                sub     r4,  r8,  r4   @@// t21--r4, -d0(rsb)
                stmia   s1_in_out_ptr0!,   {r10,r11}    
                
    	        sub     r5,  r4,   r3,  asr #1 @// tmp0
    	        add     r6,  r3,   r4,  asr #1 @// tmp1
    	        
    	        SMLAWT  r6,  r3,  r12, r6
    	        rsb     r3,  r3,  #0
    	        SMLAWB  r5,  r3,  r12, r5    	        
    	        SMLAWB  r6,  r4,  r12, r6    	        
    	        SMLAWT  r5,  r4,  r12, r5
    	        
    	        ldr     r9,   [s1_table_ptr], #4   @@// table_val 
    	        stmia   s1_in_out_ptr512!, {r5, r6}
    	        
    	        ldmia   s1_in_out_ptr0,   {r3,r4}          @@// cc10, cc11
                ldmia   s1_in_out_ptr512, {r5,r6}
    	        
    	        add     r10, r3,  r5   @@// sum1
                sub     r3,  r3,  r5   @@// t20--r3, d1                
                add     r11, r4,  r6   @@// sum2
                sub     r4,  r6,  r4   @@// t21--r4, -d0(rsb)
                stmia   s1_in_out_ptr0!,   {r10,r11}    
    	        sub     r5,  r4,   r3,  asr #1 @// tmp0
    	        add     r6,  r3,   r4,  asr #1 @// tmp1
    	        
    	        SMLAWT  r6,  r3,  r9, r6
    	        rsb     r3,  r3,  #0
    	        SMLAWB  r5,  r3,  r9, r5    	        
    	        SMLAWB  r6,  r4,  r9, r6    	        
    	        SMLAWT  r5,  r4,  r9, r5
    	        
    	        subs r14, r14, #3
    	        stmia   s1_in_out_ptr512!, {r5, r6}
    	        
    	        @@////////////////////////////////////    	        
    	        BGT  AAC_DEC_ARM_LongIfftStep1Asm_LOOP3
    	        
    	        @@/* step 4 */
    	        
    	        ldmia   s1_in_out_ptr0,   {r3,r4}  @@// cc10, cc11
                ldmia   s1_in_out_ptr512, {r5,r6}  @@// cc20, cc21
                
                add     r7, r3,  r5   @@// sum1
                sub     r3, r5,  r3   @@// -t20
                
                add     r8, r4,  r6   @@// sum2
                sub     r4, r6,  r4   @@// -t21
                
                stmia   s1_in_out_ptr0!,   {r7,r8}
                
                ldr     r12,   =-19195
                add     r5,   r3,  r4
                sub     r6,   r4,  r3
                
                SMLAWB  r6,   r6,   r12,  r6
                SMLAWB  r5,   r5,   r12,  r5
                sub     s1_table_ptr,  s1_table_ptr,  #4
                mov     r14,  #63
                stmia   s1_in_out_ptr512!, {r5,r6}                
AAC_DEC_ARM_LongIfftStep1Asm_LOOP4:
                
                
                ldmia   s1_in_out_ptr0,   {r3,r4}          @@// cc10, cc11
                ldmia   s1_in_out_ptr512, {r5,r6, r7, r8}  @@// cc20, cc21                
                ldmda   s1_table_ptr!, {r9, r12}   @@// table_val                
                add     r10, r3,  r5   @@// sum1
                sub     r3,  r3,  r5   @@// t20--r3, d1                
                add     r11, r4,  r6   @@// sum2
                sub     r4,  r6,  r4   @@// t21--r4, -d0(rsb)
                stmia   s1_in_out_ptr0!,   {r10,r11}    
    	        rsb     r5,  r3,   r4,  asr #1 @// tmp0
    	        add     r6,  r4,   r3,  asr #1 @// tmp1
    	        
    	        SMLAWB  r6,  r3,  r12, r6
    	        rsb     r3,  r3,  #0
    	        SMLAWT  r5,  r3,  r12, r5    	        
    	        SMLAWT  r6,  r4,  r12, r6    	        
    	        SMLAWB  r5,  r4,  r12, r5
    	        
    	        ldmia   s1_in_out_ptr0,    {r3, r4} 
    	        stmia   s1_in_out_ptr512!, {r5, r6}
    	        
    	        add     r10, r3,  r7   @@// sum1
                sub     r3,  r3,  r7   @@// t20--r3, d1                
                add     r11, r4,  r8   @@// sum2
                sub     r4,  r8,  r4   @@// t21--r4, -d0(rsb)
                stmia   s1_in_out_ptr0!,   {r10,r11}    
                
    	        rsb     r5,  r3,   r4,  asr #1 @// tmp0
    	        add     r6,  r4,   r3,  asr #1 @// tmp1
    	        
    	        SMLAWB  r6,  r3,  r9, r6
    	        rsb     r3,  r3,  #0
    	        SMLAWT  r5,  r3,  r9, r5    	        
    	        SMLAWT  r6,  r4,  r9, r6    	        
    	        SMLAWB  r5,  r4,  r9, r5
    	        
    	        ldr     r9,   [s1_table_ptr], #-4   @@// table_val 
    	        stmia   s1_in_out_ptr512!, {r5, r6}
    	        
    	        ldmia   s1_in_out_ptr0,   {r3,r4}          @@// cc10, cc11
                ldmia   s1_in_out_ptr512, {r5,r6}
    	        
    	        add     r10, r3,  r5   @@// sum1
                sub     r3,  r3,  r5   @@// t20--r3, d1                
                add     r11, r4,  r6   @@// sum2
                sub     r4,  r6,  r4   @@// t21--r4, -d0(rsb)
                stmia   s1_in_out_ptr0!,   {r10,r11}    
    	        rsb     r5,  r3,   r4,  asr #1 @// tmp0
    	        add     r6,  r4,   r3,  asr #1 @// tmp1
    	        
    	        SMLAWB  r6,  r3,  r9, r6
    	        rsb     r3,  r3,  #0
    	        SMLAWT  r5,  r3,  r9, r5    	        
    	        SMLAWT  r6,  r4,  r9, r6    	        
    	        SMLAWB  r5,  r4,  r9, r5
    	        
    	        subs r14, r14, #3
    	        stmia   s1_in_out_ptr512!, {r5, r6}
                
                    	        
    	        BGT  AAC_DEC_ARM_LongIfftStep1Asm_LOOP4
    	        
    	        
    	        ldmfd   sp!, {r4-r12, pc}
                @ENDFUNC
                
                
                
                
                @//int32 AAC_DEC_ARM_LongIfftStep2Asm(
                @//                              int32   *in_out_ptr,
                @//                              int32   *table_ptr
                @//                              )@

s2_in_out_ptr0    .req  r0
s2_table_ptr      .req  r1
s2_in_out_ptr128  .req  r2
s2_in_out_ptr256  .req  r3
s2_in_out_ptr384  .req  r4

                 
AAC_DEC_ARM_LongIfftStep2Asm:@   FUNCTION
                @ // save the value for the following calculation
                .global  AAC_DEC_ARM_LongIfftStep2Asm
                stmfd   sp!, {r4-r12, r14}
                add     s2_in_out_ptr128,  s2_in_out_ptr0,    #128*4
                add     s2_in_out_ptr256,  s2_in_out_ptr128,  #128*4
                add     s2_in_out_ptr384,  s2_in_out_ptr256,  #128*4
                
                mov     r14,  #2
AAC_DEC_ARM_LongIfftStep2AsmEXTERNAL_LOOP:            
                stmfd   sp!, {r14}
                @@////////////////////////////////////////
                ldmia  s2_in_out_ptr0, {r5, r6}
                ldmia  s2_in_out_ptr256, {r7, r8}                
                add    r9,   r5,  r7  @@// RE(t20)--r9
                sub    r10,  r5,  r7  @@// RE(t10)--r10                
                ldr    r5,   [s2_in_out_ptr128]
                ldr    r7,   [s2_in_out_ptr384]                
                add    r11,  r6,  r8  @@// IM(t21)--r11
                sub    r12,  r6,  r8  @@// IM(t11)--r12                
                add    r6,   r5,  r7  @@// RE(t30)--r6
                sub    r8,   r5,  r7  @@// IM(t41)--r8                
                add    r5,   r9,  r6  @@// RE(t20) + RE(t30)     
                str    r5,   [s2_in_out_ptr0], #4     
                
                ldr    r5,   [s2_in_out_ptr128, #4]
                ldr    r7,   [s2_in_out_ptr384, #4]                 
                sub    r9,   r9,  r6  @@// RE(c3) =  RE(t20) - RE(t30) -- r9(RE(t30))                
                     
                add    r7,   r5, r7            @@// IM(t31) -- r7
                sub    r5,   r7, r5,  lsl #1   @@// RE(t40) -- r5                
                add    r11,  r11, r7           @@// IM(t21) + IM(t31)
                sub    r7,   r11, r7,  lsl #1  @@// IM(t21) - IM(t31) -- r7(IM(t31))
                str    r11,   [s2_in_out_ptr0], #4
                add    r10,   r10,  r5
                sub    r5,    r10,   r5, lsl #1    @@// RE(c20): r10,  RE(t40): r5                
                add    r12,   r12,  r8
                sub    r8,    r12,  r8, lsl #1     @@// IM(c21): r12,  IM(t41): r8
                @@// r6 and r11 are free, and the other reg, c2(r10, r12), t4:(r5, r8), t3:(r9, r7)
                stmia  s2_in_out_ptr128!, {r10, r12}
                str    r9, [s2_in_out_ptr256], #4
                stmia  s2_in_out_ptr384!, {r5,  r8}            
                str    r7, [s2_in_out_ptr256], #4               
                
                @/* step 1*/
                mov    r14,  #31
AAC_DEC_ARM_LongIfftStep2Asm_LOOP1:
                @@////////////////////////////////////////
                ldmia  s2_in_out_ptr0, {r5, r6}
                ldmia  s2_in_out_ptr256, {r7, r8}                
                add    r9,   r5,  r7  @@// RE(t20)--r9
                sub    r10,  r5,  r7  @@// RE(t10)--r10                
                ldr    r5,   [s2_in_out_ptr128]
                ldr    r7,   [s2_in_out_ptr384]                 
                add    r11,  r6,  r8  @@// IM(t21)--r11
                sub    r12,  r6,  r8  @@// IM(t11)--r12                
                add    r6,   r5,  r7  @@// RE(t30)--r6
                sub    r8,   r5,  r7  @@// IM(t41)--r8                
                add    r5,   r9,  r6  @@// RE(t20) + RE(t30)     
                str    r5,   [s2_in_out_ptr0], #4  
                ldr    r7,   [s2_in_out_ptr384, #4]  
                ldr    r5,   [s2_in_out_ptr128, #4]
                            
                sub    r9,   r9,  r6  @@// RE(c3) =  RE(t20) - RE(t30) -- r9(RE(t30))
                
                
                            
                add    r7,   r5, r7            @@// IM(t31) -- r7
                sub    r5,   r7, r5,  lsl #1   @@// RE(t40) -- r5                
                add    r11,  r11, r7           @@// IM(t21) + IM(t31)
                sub    r7,   r11, r7,  lsl #1  @@// IM(t21) - IM(t31) -- r7(IM(t31))
                str    r11,   [s2_in_out_ptr0], #4                       
                add    r10,   r10,  r5
                sub    r5,    r10,   r5, lsl #1    @@// RE(c20): r10,  RE(t40): r5                
                add    r12,   r12,  r8
                sub    r8,    r12,  r8, lsl #1     @@// IM(c21): r12,  IM(t41): r8
                @@// r6 and r11 are free, and the other reg, c2(r10, r12), t4:(r5, r8), t3:(r9, r7)
                @@// 1
                ldr    r11,   [s2_table_ptr], #4
                add    r6,    r12,  r10,  asr #1
                
                SMLAWT r6,    r12,  r11,  r6                
                SMLAWB r6,    r10,  r11,  r6                
                str    r6,    [s2_in_out_ptr128, #4]
                
                sub    r6,    r10,  r12,  asr #1                              
                SMLAWT r6,    r10,  r11,  r6      
                rsb    r12,   r12,  #0  
                SMLAWB r6,    r12,  r11,  r6        
                str    r6,    [s2_in_out_ptr128], #8
                @@//  
                ldr    r11,   [s2_table_ptr], #4   @@// d0: r7, d1: r9
                sub    r10,   r9,  r7,  asr #1
                sub    r10,   r10, r9,  asr #1 @@// (d1 - d0) >> 1
                add    r12,   r10, r7          @@// (d0 + d1) >> 1                
                SMLAWT r12,    r7,  r11,  r12
                rsb    r7,     r7,  #0
                SMLAWB r10,    r7,  r11,   r10                
                SMLAWB r12,    r9,  r11,  r12
                SMLAWT r10,    r9,  r11,   r10 
                mov    r5,  r5,  lsl #1               
                stmia  s2_in_out_ptr256!, {r10, r12}
                mov    r8,  r8,  lsl #1
                ldmia  s2_table_ptr!, {r11,  r12}
                
                SMULL  r9, r10,  r8,  r11
                rsb    r8, r8,   #0
                SMULL  r6, r7,   r8,  r12                
                SMLAL  r9, r10,  r5,  r12
                SMLAL  r6, r7,   r5,  r11                  
                
                
                @@////////////////////////////////////////
                subs   r14,  r14,  #1   
                stmia  s2_in_out_ptr384!, {r7, r10}
                
                BGT    AAC_DEC_ARM_LongIfftStep2Asm_LOOP1
                
                
                @// step 2
                ldmia  s2_in_out_ptr0, {r5, r6}
                ldmia  s2_in_out_ptr256, {r7, r8}                
                add    r9,   r5,  r7  @@// RE(t20)--r9
                sub    r10,  r5,  r7  @@// RE(t10)--r10                
                ldr    r5,   [s2_in_out_ptr128]
                ldr    r7,   [s2_in_out_ptr384]                
                add    r11,  r6,  r8  @@// IM(t21)--r11
                sub    r12,  r6,  r8  @@// IM(t11)--r12                
                add    r6,   r5,  r7  @@// RE(t30)--r6
                sub    r8,   r5,  r7  @@// IM(t41)--r8                
                add    r5,   r9,  r6  @@// RE(t20) + RE(t30)     
                str    r5,   [s2_in_out_ptr0], #4       
                ldr    r7,   [s2_in_out_ptr384, #4]    
                ldr    r5,   [s2_in_out_ptr128, #4]
                sub    r9,   r9,  r6  @@// RE(c3) =  RE(t20) - RE(t30) -- r9(RE(t30))                
                
                 
                               
                add    r7,   r5, r7            @@// IM(t31) -- r7
                sub    r5,   r7, r5,  lsl #1   @@// RE(t40) -- r5                
                add    r11,  r11, r7           @@// IM(t21) + IM(t31)
                rsb    r7,   r11, r7,  lsl #1  @@// IM(t21) - IM(t31) -- r7(IM(t31))
                str    r11,   [s2_in_out_ptr0], #4
                add    r10,   r10,  r5
                sub    r5,    r10,   r5, lsl #1    @@// RE(c20): r10,  RE(t40): r5                
                add    r12,   r12,  r8
                sub    r8,    r12,  r8, lsl #1     @@// IM(c21): r12,  IM(t41): r8
                @@// r6 and r11 are free, and the other reg, c2(r10, r12), t4:(r5, r8), t3:(r9, r7)
                ldr    r11,   =-19195
                sub    r6,    r10,  r12
                SMLAWB r6,    r6,   r11, r6
                
                str    r6,    [s2_in_out_ptr128], #4
                str    r7,    [s2_in_out_ptr256], #4
                add    r6,    r10,  r12
                SMLAWB r6,    r6,   r11, r6                
                str    r9,    [s2_in_out_ptr256], #4
                str    r6,    [s2_in_out_ptr128], #4                
                @////
                rsb    r8,    r8,  #0 @@// d0 = -d0
                sub    r6,    r8,  r5
                add    r7,    r5,  r8
                SMLAWB r6,    r6,   r11, r6
                SMLAWB r7,    r7,   r11, r7
                
                stmia  s2_in_out_ptr384!, {r6-r7}
                
                mov    r14,  #31
AAC_DEC_ARM_LongIfftStep2Asm_LOOP2:
                @@////////////////////////////////////////
                ldmia  s2_in_out_ptr0, {r5, r6}
                ldmia  s2_in_out_ptr256, {r7, r8}                
                add    r9,   r5,  r7  @@// RE(t20)--r9
                sub    r10,  r5,  r7  @@// RE(t10)--r10                
                ldr    r5,   [s2_in_out_ptr128]
                ldr    r7,   [s2_in_out_ptr384]                 
                add    r11,  r6,  r8  @@// IM(t21)--r11
                sub    r12,  r6,  r8  @@// IM(t11)--r12                
                add    r6,   r5,  r7  @@// RE(t30)--r6
                sub    r8,   r5,  r7  @@// IM(t41)--r8                
                add    r5,   r9,  r6  @@// RE(t20) + RE(t30)     
                str    r5,   [s2_in_out_ptr0], #4       
                ldr    r7,   [s2_in_out_ptr384, #4]      
                ldr    r5,   [s2_in_out_ptr128, #4]    
                sub    r9,   r9,  r6  @@// RE(c3) =  RE(t20) - RE(t30) -- r9(RE(t30))
                
                
                           
                add    r7,   r5, r7            @@// IM(t31) -- r7
                sub    r5,   r7, r5,  lsl #1   @@// RE(t40) -- r5                
                add    r11,  r11, r7           @@// IM(t21) + IM(t31)
                rsb    r7,   r11, r7,  lsl #1  @@// IM(t21) - IM(t31) -- r7(IM(t31))
                str    r11,   [s2_in_out_ptr0], #4                       
                add    r10,   r10,  r5
                sub    r5,    r10,   r5, lsl #1    @@// RE(c20): r10,  RE(t40): r5                
                add    r12,   r12,  r8
                sub    r8,    r12,  r8, lsl #1     @@// IM(c21): r12,  IM(t41): r8
                @@// r6 and r11 are free, and the other reg, c2(r10, r12), t4:(r5, r8), t3:(r9, r7)
                @@// 1
                ldr    r11,   [s2_table_ptr], #4
                add    r6,    r10,  r12,  asr #1  @@// d0: r12,   d1: r10  ((d0>>1)+d1)
                
                SMLAWT r6,    r12,  r11,  r6                
                SMLAWB r6,    r10,  r11,  r6                
                str    r6,    [s2_in_out_ptr128, #4]
                
                rsb    r6,    r12,  r10,  asr #1  
                rsb    r12,   r12,  #0  
                SMLAWB r6,    r12,  r11,  r6                
                SMLAWT r6,    r10,  r11,  r6          
                str    r6,    [s2_in_out_ptr128], #8
                @@//  2         d0: r7,   d1: r9  ((d0>>1)+d1)
                ldr    r11,   [s2_table_ptr], #4
                           
                add    r12,    r9,   r7,   asr #1  @//(d1+(d0>>1))
                sub    r12,    r12,  r9,   asr #1 
                sub    r10,    r12,  r9
                
                
                
                SMLAWT r12,    r7,  r11,  r12                
                SMLAWB r10,    r7,  r11,   r10                               
                SMLAWB r12,    r9,  r11,  r12
                rsb    r9,     r9,  #0
                SMLAWT r10,    r9,  r11,   r10 
                mov    r5,  r5,  lsl #1               
                stmia  s2_in_out_ptr256!, {r10, r12}
                mov    r8,  r8,  lsl #1
                ldmia  s2_table_ptr!, {r11,  r12}
                
                @@//  3
                SMULL  r9, r10,  r8,  r11
                rsb    r8, r8,   #0
                SMULL  r6, r7,   r8,  r12                
                SMLAL  r9, r10,  r5,  r12
                SMLAL  r6, r7,   r5,  r11                  
                
                
                @@////////////////////////////////////////
                
                subs   r14,  r14,  #1   
                stmia  s2_in_out_ptr384!, {r7, r10}
                BGT    AAC_DEC_ARM_LongIfftStep2Asm_LOOP2
                
                add    s2_in_out_ptr0,   s2_in_out_ptr0,   #384*4
                add    s2_in_out_ptr128, s2_in_out_ptr128, #384*4
                add    s2_in_out_ptr256, s2_in_out_ptr256, #384*4
                add    s2_in_out_ptr384, s2_in_out_ptr384, #384*4
                sub    s2_table_ptr,     s2_table_ptr,  #248*4
                
                ldmfd   sp!, {r14}
                subs    r14,  r14,  #1
                BGT     AAC_DEC_ARM_LongIfftStep2AsmEXTERNAL_LOOP
                
                                
                ldmfd   sp!, {r4-r12, pc}
                @ENDFUNC
                
                
                
                
                @//int32 AAC_DEC_ARM_LongIfftStep3Asm(
                @//                              int32   *in_out_ptr,
                @//                              int32   *table_ptr
                @//                              )@

s3_in_out_ptr     .req   r0
s3_table_ptr      .req   r1
s3_in_out_ptr64   .req   r2

                 
AAC_DEC_ARM_LongIfftStep3Asm:@   FUNCTION
                @ // save the value for the following calculation
                .global  AAC_DEC_ARM_LongIfftStep3Asm
                stmfd   sp!, {r4-r12, r14}
                
                add     s3_in_out_ptr64, s3_in_out_ptr, #64 * 4
                
                mov     r14,  #8
AAC_DEC_ARM_LongIfftStep3Asm_EXTERNAL_LOOP:
                stmfd   sp!, {r14}
                @///////////////////////////////////////////////////
                ldmia   s3_in_out_ptr,   {r3, r4}
                ldmia   s3_in_out_ptr64, {r5, r6}                
                ldr     r7,  [s3_in_out_ptr, #32*4]
                ldr     r8,  [s3_in_out_ptr, #33*4]                                
                ldr     r9,  [s3_in_out_ptr64, #32*4]
                ldr     r10, [s3_in_out_ptr64, #33*4]                                
                add     r7,  r7,  r9         @@// r7: t30, -----RE(cc_ptr[16])+RE(cc_ptr[3*16])
                sub     r9,  r7,  r9, lsl #1 @@// r9: t41, -----RE(cc_ptr[16])-RE(cc_ptr[3*16])                
                add     r3,  r3,  r5         @@// r3: t20, -----RE(cc_ptr[0])+RE(cc_ptr[2*16])
                sub     r5,  r3,  r5, lsl #1 @@// r5: t10, -----RE(cc_ptr[0])-RE(cc_ptr[2*16])                
                add     r3,  r3,  r7         @@// r3: (t20) + (t30)@
                sub     r7,  r3,  r7, lsl #1 @@// r7: t30   = (t20) - (t30)@
                mov   r3,  r3,  asr #1
                str     r3,  [s3_in_out_ptr], #4                
                add     r10,  r10, r8          @// r10: t31-----IM(cc_ptr[3*16])+ IM(cc_ptr[16])@
                sub     r8,  r10, r8, lsl #1   @// r8:  t40-----IM(cc_ptr[3*16])- IM(cc_ptr[16])@                
                add     r4,  r4, r6           @//  r4:  t21-----IM(cc_ptr[0]) + IM(cc_ptr[2*16])@
                sub     r6,  r4, r6, lsl #1   @//  r6:  t11-----IM(cc_ptr[0]) - IM(cc_ptr[2*16])@                
                add     r4,   r4, r10          @// 
                sub     r10,  r4, r10,  lsl #1 @// r10, t31 :
                mov   r4,  r4,  asr #1
                str     r4,  [s3_in_out_ptr], #4                
                @@/* t3:(r7, r10) */
                add    r5,   r5,   r8          @// r5: c20
                sub    r8,   r5,   r8,  lsl #1 @// r8: t40                
                add    r6,   r6,   r9          @// r6: c21
                sub    r9,   r6,   r9,  lsl #1 @// r9: t41
                @@/* t3:(r7, r10), c2:(r5, r6), t4:(r8, r9) */
                @@/* the free reg is: r3, r4, r11, r12 :*/
                mov   r5,  r5,  asr #1
                mov   r6,  r6,  asr #1
                mov   r7,  r7,  asr #1
                mov   r8,  r8,  asr #1
                mov   r9,  r9,  asr #1
                mov   r10, r10,  asr #1
                
                str   r5, [s3_in_out_ptr, #(32-2)*4]
                str   r6, [s3_in_out_ptr, #(32-1)*4]                
                stmia s3_in_out_ptr64!, {r7, r10}                
                str   r8, [s3_in_out_ptr64, #(32-2)*4]
                str   r9, [s3_in_out_ptr64, #(32-1)*4]
                
                mov   r14,  #7                
AAC_DEC_ARM_LongIfftStep3Asm_INTERLOOP0:
                @/////////////////////////////////////
                @/* step 1 */
                ldmia   s3_in_out_ptr,   {r3, r4}
                ldmia   s3_in_out_ptr64, {r5, r6}                
                ldr     r7,  [s3_in_out_ptr, #32*4]
                ldr     r8,  [s3_in_out_ptr, #33*4]                                
                ldr     r9,  [s3_in_out_ptr64, #32*4]
                ldr     r10, [s3_in_out_ptr64, #33*4]                                
                add     r7,  r7,  r9         @@// r7: t30, -----RE(cc_ptr[16])+RE(cc_ptr[3*16])
                sub     r9,  r7,  r9, lsl #1 @@// r9: t41, -----RE(cc_ptr[16])-RE(cc_ptr[3*16])                
                add     r3,  r3,  r5         @@// r3: t20, -----RE(cc_ptr[0])+RE(cc_ptr[2*16])
                sub     r5,  r3,  r5, lsl #1 @@// r5: t10, -----RE(cc_ptr[0])-RE(cc_ptr[2*16])                
                add     r3,  r3,  r7         @@// r3: (t20) + (t30)@
                sub     r7,  r3,  r7, lsl #1 @@// r7: t30   = (t20) - (t30)@
                mov   r3,  r3,  asr #1
                str     r3,  [s3_in_out_ptr], #4                
                add     r10,  r10, r8          @// r10: t31-----IM(cc_ptr[3*16])+ IM(cc_ptr[16])@
                sub     r8,  r10, r8, lsl #1   @// r8:  t40-----IM(cc_ptr[3*16])- IM(cc_ptr[16])@                
                add     r4,  r4, r6           @//  r4:  t21-----IM(cc_ptr[0]) + IM(cc_ptr[2*16])@
                sub     r6,  r4, r6, lsl #1   @//  r6:  t11-----IM(cc_ptr[0]) - IM(cc_ptr[2*16])@                
                add     r4,   r4, r10          @// 
                sub     r10,  r4, r10,  lsl #1 @// r10, t31 :
                mov   r4,  r4,  asr #1
                str     r4,  [s3_in_out_ptr], #4                
                @@/* t3:(r7, r10) */
                add    r5,   r5,   r8          @// r5: c20
                sub    r8,   r5,   r8,  lsl #1 @// r8: t40                
                add    r6,   r6,   r9          @// r6: c21
                sub    r9,   r6,   r9,  lsl #1 @// r9: t41
                @@/* t3:(r7, r10), c2:(r5, r6), t4:(r8, r9) */
                @@/* the free reg is: r3, r4, r11, r12 :*/
                @@// 1
                ldmia  s3_table_ptr!, {r11,  r12}                
                SMULL  r3,   r4,  r6,  r11
                SMLAL  r3,   r4,  r5,  r12   
                rsb    r6,   r6,  #0             
                str    r4, [s3_in_out_ptr, #(32-1)*4]                              
                SMULL  r3,   r4,  r6,  r12
                SMLAL  r3,   r4,  r5,  r11 
                ldmia  s3_table_ptr!, {r11,  r12}                    
                str    r4, [s3_in_out_ptr, #(32-2)*4] 
                @@// 2                           
                SMULL  r3,   r6,  r10,  r11
                SMLAL  r3,   r6,  r7,  r12   
                rsb    r10,  r10,  #0
                SMULL  r4,   r5,  r10,  r12
                SMLAL  r4,   r5,  r7,  r11                
                ldmia  s3_table_ptr!, {r11,  r12} 
                stmia  s3_in_out_ptr64!, {r5,r6}
                @// 3
                SMULL  r3,   r5,  r9,  r11
                SMLAL  r3,   r5,  r8,  r12   
                rsb    r9,  r9,  #0
                SMULL  r4,   r6,  r9,  r12
                SMLAL  r4,   r6,  r8,  r11
                
                str   r6, [s3_in_out_ptr64, #(32-2)*4]
                subs   r14,  r14,  #1
                str   r5, [s3_in_out_ptr64, #(32-1)*4]
                
                BGT    AAC_DEC_ARM_LongIfftStep3Asm_INTERLOOP0
                @////////////////////////////////////////////
                @/* step 2 */
                ldmia   s3_in_out_ptr,   {r3, r4}
                ldmia   s3_in_out_ptr64, {r5, r6}                
                ldr     r7,  [s3_in_out_ptr, #32*4]
                ldr     r8,  [s3_in_out_ptr, #33*4]                                
                ldr     r9,  [s3_in_out_ptr64, #32*4]
                ldr     r10, [s3_in_out_ptr64, #33*4]                                
                add     r7,  r7,  r9         @@// r7: t30, -----RE(cc_ptr[16])+RE(cc_ptr[3*16])
                sub     r9,  r7,  r9, lsl #1 @@// r9: t41, -----RE(cc_ptr[16])-RE(cc_ptr[3*16])                
                add     r3,  r3,  r5         @@// r3: t20, -----RE(cc_ptr[0])+RE(cc_ptr[2*16])
                sub     r5,  r3,  r5, lsl #1 @@// r5: t10, -----RE(cc_ptr[0])-RE(cc_ptr[2*16])                
                add     r3,  r3,  r7         @@// r3: (t20) + (t30)@
                sub     r7,  r3,  r7, lsl #1 @@// r7: t30   = (t20) - (t30)@
                mov   r3,  r3,  asr #1
                str     r3,  [s3_in_out_ptr], #4                
                add     r10,  r10, r8          @// r10: t31-----IM(cc_ptr[3*16])+ IM(cc_ptr[16])@
                sub     r8,  r10, r8, lsl #1   @// r8:  t40-----IM(cc_ptr[3*16])- IM(cc_ptr[16])@                
                add     r4,  r4, r6           @//  r4:  t21-----IM(cc_ptr[0]) + IM(cc_ptr[2*16])@
                sub     r6,  r4, r6, lsl #1   @//  r6:  t11-----IM(cc_ptr[0]) - IM(cc_ptr[2*16])@                
                add     r4,   r4, r10          @// 
                sub     r10,  r4, r10,  lsl #1 @// r10, t31 :
                mov   r4,  r4,  asr #1
                str     r4,  [s3_in_out_ptr], #4                
                @@/* t3:(r7, r10) */
                add    r5,   r5,   r8          @// r5: c20
                sub    r8,   r5,   r8,  lsl #1 @// r8: t40                
                add    r6,   r6,   r9          @// r6: c21
                sub    r9,   r6,   r9,  lsl #1 @// r9: t41
                @@/* t3:(r7, r10), c2:(r5, r6), t4:(r8, r9) */
                @@/* the free reg is: r3, r4, r11, r12 :*/
                mov   r7,  r7,  asr #1
                @@//mov   r10, r10,  asr #1
                rsb   r3,  r10,  r10,  asr #1
                stmia s3_in_out_ptr64!, {r3, r7}   
                ldr    r3,   =-19195
                sub    r11,   r5,  r6
                add    r12,   r5,  r6
                SMLAWB r11,   r11,  r3, r11
                SMLAWB r12,   r12,  r3, r12           
                mov   r11,  r11,  asr #1
                mov   r12,  r12,  asr #1              
                str   r11, [s3_in_out_ptr, #(32-2)*4]
                str   r12, [s3_in_out_ptr, #(32-1)*4] 
                sub   r5,   r8,  r9
                sub   r4,   r5,  r8,  lsl  #1
                SMLAWB r4,   r4,  r3, r4
                SMLAWB r5,   r5,  r3, r5     
                mov   r4,  r4,  asr #1
                mov   r5,  r5,  asr #1       
                str   r4, [s3_in_out_ptr64, #(32-2)*4]
                str   r5, [s3_in_out_ptr64, #(32-1)*4]                
                mov   r14,  #7                
                
AAC_DEC_ARM_LongIfftStep3Asm_INTERLOOP1:
                @/////////////////////////////////////
                @/* step 1 */
                ldmia   s3_in_out_ptr,   {r3, r4}
                ldmia   s3_in_out_ptr64, {r5, r6}                
                ldr     r7,  [s3_in_out_ptr, #32*4]
                ldr     r8,  [s3_in_out_ptr, #33*4]                                
                ldr     r9,  [s3_in_out_ptr64, #32*4]
                ldr     r10, [s3_in_out_ptr64, #33*4]                                
                add     r7,  r7,  r9         @@// r7: t30, -----RE(cc_ptr[16])+RE(cc_ptr[3*16])
                sub     r9,  r7,  r9, lsl #1 @@// r9: t41, -----RE(cc_ptr[16])-RE(cc_ptr[3*16])                
                add     r3,  r3,  r5         @@// r3: t20, -----RE(cc_ptr[0])+RE(cc_ptr[2*16])
                sub     r5,  r3,  r5, lsl #1 @@// r5: t10, -----RE(cc_ptr[0])-RE(cc_ptr[2*16])                
                add     r3,  r3,  r7         @@// r3: (t20) + (t30)@
                sub     r7,  r3,  r7, lsl #1 @@// r7: t30   = (t20) - (t30)@
                mov   r3,  r3,  asr #1
                str     r3,  [s3_in_out_ptr], #4                
                add     r10,  r10, r8          @// r10: t31-----IM(cc_ptr[3*16])+ IM(cc_ptr[16])@
                sub     r8,  r10, r8, lsl #1   @// r8:  t40-----IM(cc_ptr[3*16])- IM(cc_ptr[16])@                
                add     r4,  r4, r6           @//  r4:  t21-----IM(cc_ptr[0]) + IM(cc_ptr[2*16])@
                sub     r6,  r4, r6, lsl #1   @//  r6:  t11-----IM(cc_ptr[0]) - IM(cc_ptr[2*16])@                
                add     r4,   r4, r10          @// 
                sub     r10,  r4, r10,  lsl #1 @// r10, t31 :
                mov   r4,  r4,  asr #1
                str     r4,  [s3_in_out_ptr], #4                
                @@/* t3:(r7, r10) */
                add    r5,   r5,   r8          @// r5: c20
                sub    r8,   r5,   r8,  lsl #1 @// r8: t40                
                add    r6,   r6,   r9          @// r6: c21
                sub    r9,   r6,   r9,  lsl #1 @// r9: t41
                @@/* t3:(r7, r10), c2:(r5, r6), t4:(r8, r9) */
                @@/* the free reg is: r3, r4, r11, r12 :*/
                @@// 1
                ldmia  s3_table_ptr!, {r11,  r12}                
                SMULL  r3,   r4,  r6,  r11
                SMLAL  r3,   r4,  r5,  r12   
                rsb    r6,   r6,  #0             
                str    r4, [s3_in_out_ptr, #(32-1)*4]                              
                SMULL  r3,   r4,  r6,  r12
                SMLAL  r3,   r4,  r5,  r11 
                ldmia  s3_table_ptr!, {r11,  r12}                    
                str    r4, [s3_in_out_ptr, #(32-2)*4] 
                @@// 2                           
                SMULL  r3,   r6,  r10,  r11
                SMLAL  r3,   r6,  r7,  r12   
                rsb    r10,  r10,  #0
                SMULL  r4,   r5,  r10,  r12
                SMLAL  r4,   r5,  r7,  r11                
                ldmia  s3_table_ptr!, {r11,  r12} 
                stmia  s3_in_out_ptr64!, {r5,r6}
                @// 3
                SMULL  r3,   r5,  r9,  r11
                SMLAL  r3,   r5,  r8,  r12   
                rsb    r9,  r9,  #0
                SMULL  r4,   r6,  r9,  r12
                SMLAL  r4,   r6,  r8,  r11
                
                str   r6, [s3_in_out_ptr64, #(32-2)*4]
                subs   r14,  r14,  #1
                str   r5, [s3_in_out_ptr64, #(32-1)*4]
                
                BGT    AAC_DEC_ARM_LongIfftStep3Asm_INTERLOOP1
                
                @///////////////////////////////////////////////////
                ldmfd  sp!,  {r14}
                sub    s3_table_ptr,  s3_table_ptr, #84*4                
                add    s3_in_out_ptr,    s3_in_out_ptr,    #96*4
                add    s3_in_out_ptr64,  s3_in_out_ptr64,  #96*4                
                
                subs   r14,  r14,  #1
                BGT    AAC_DEC_ARM_LongIfftStep3Asm_EXTERNAL_LOOP
                
                
                
                @@//////////////////////////////////////////////////
                ldmfd   sp!, {r4-r12, pc}
                @ENDFUNC
                
                
                
                
                
                @//int32 AAC_DEC_ARM_LongIfftStep4Asm(
                @//                              int32   *in_out_ptr
                @//                              )@


IFFT_step4_in_ptr0          .req  r0
IFFT_step4_in_ptr16         .req  r2

tt10                      .req  r9
tt11                      .req  r10
tt20                      .req  r3
tt21                      .req  r4
tt30                      .req  r5
tt31                      .req  r8
tt40                      .req  r6
tt41                      .req  r7
                 
AAC_DEC_ARM_LongIfftStep4Asm:@   FUNCTION
                @ // save the value for the following calculation
                .global  AAC_DEC_ARM_LongIfftStep4Asm
                stmfd   sp!, {r4-r12, r14}
                
                
                ldr     r1,   =1984016128
                ldr     r12,  =821806400
                ldr     r11,  =-19195                
                add     IFFT_step4_in_ptr16,  IFFT_step4_in_ptr0,  #16*4

                mov     r14,  #32
AAC_DEC_ARM_LongIfftStep4AsmLOOP:
                @@//////////////////////////////////////////////////
                @/* the free register is: r3, r4, r5, r6, r7, r8, r9, r10*/@
                @@@///////////////////////////////////////
                @@@/* first */
                ldmia   IFFT_step4_in_ptr0,  {r3, r4}
                ldmia   IFFT_step4_in_ptr16, {r9, r10}
                ldr     r5,  [IFFT_step4_in_ptr0, #8*4]
                ldr     r6,  [IFFT_step4_in_ptr0, #9*4]                
                ldr     r7,  [IFFT_step4_in_ptr16, #8*4]
                ldr     r8,  [IFFT_step4_in_ptr16, #9*4]                
                
                add     r5,  r5,  r7           @//  t30: r5
                sub     r7,  r5,  r7, lsl #1   @//  t41: r7
                
                add     r8,  r8,  r6           @//  t31: r8
                sub     r6,  r8,  r6, lsl #1   @//  t40: r6
                
                add     r3,  r3,  r9           @//  t20: r3
                sub     r9,  r3,  r9, lsl #1   @//  t10: r9
                
                add     r4,  r4,  r10          @//  t21: r4
                sub     r10, r4,  r10,lsl #1   @//  t11: r10                
                
                add     r9,    tt10,   tt40           @@// r9: c20
                sub     tt40,  tt10,   tt40,  lsl #1                 
                add     r10,  tt11,   tt41            @@// r10: c21
                sub     tt41, tt11,   tt41,  lsl #1                  

                str     r9,   [IFFT_step4_in_ptr0, #8*4]
                str     r10,  [IFFT_step4_in_ptr0, #9*4]                
                
                str     tt40,   [IFFT_step4_in_ptr16, #8*4]
                str     tt41,   [IFFT_step4_in_ptr16, #9*4]                
                
                add     tt20,  tt20,  tt30            @@// sum1, r3
                sub     tt30,  tt20,  tt30, lsl #1
                
                add     tt21,  tt21,  tt31            @//  sum2. r4
                sub     tt31,  tt21,  tt31, lsl #1
                
                stmia   IFFT_step4_in_ptr0!, {tt20,   tt21}                
                stmia   IFFT_step4_in_ptr16!,  {r5,  r8}
                
                @@@///////////////////////////////////////
                @@@/* second */
                ldmia   IFFT_step4_in_ptr0,  {r3, r4}
                ldmia   IFFT_step4_in_ptr16, {r9, r10}
                ldr     r5,  [IFFT_step4_in_ptr0, #8*4]
                ldr     r6,  [IFFT_step4_in_ptr0, #9*4]                
                ldr     r7,  [IFFT_step4_in_ptr16, #8*4]
                ldr     r8,  [IFFT_step4_in_ptr16, #9*4]                
                
                add     r5,  r5,  r7           @//  t30: r5
                sub     r7,  r5,  r7, lsl #1   @//  t41: r7
                
                add     r8,  r8,  r6           @//  t31: r8
                sub     r6,  r8,  r6, lsl #1   @//  t40: r6
                
                add     r3,  r3,  r9           @//  t20: r3
                sub     r9,  r3,  r9, lsl #1   @//  t10: r9
                
                add     r4,  r4,  r10          @//  t21: r4
                sub     r10, r4,  r10,lsl #1   @//  t11: r10                
                
                add     r9,    tt10,   tt40           @@// c20: r9
                sub     tt40,  tt10,   tt40, lsl #1   @@// t40: r6
                add     r10,  tt11,   tt41            @@// c21: r10
                sub     tt41, tt11,   tt41,  lsl #1   @@// t41: r7
                
                add     tt20,  tt20,  tt30            @@// sum1, r3
                sub     tt30,  tt20,  tt30, lsl #1    @@// t30:  r5
                
                add     tt21,  tt21,  tt31            @@// sum2. r4
                sub     tt31,  tt21,  tt31, lsl #1    @@// t31:  r8
                
                stmia   IFFT_step4_in_ptr0!, {tt20,   tt21}             
                
                add     tt30,   tt30,  tt31
                sub     r4,   tt30,  tt31, lsl #1                
                SMLAWB  tt30,   tt30,  r11,  tt30
                SMLAWB  r4,   r4,  r11,  r4                
                mov     r10,r10, lsl #1   @@// d0
                stmia   IFFT_step4_in_ptr16!, {r4, r5}           
                mov     r9, r9,  lsl #1   @@// d1
                @//buufferFly(c21<<1, c20<<1, 1984016128 >> TEST_BIT_SHIFT, 821806464 >> TEST_BIT_SHIFT, &tmp2, &tmp1)@
                SMULL   r5,  r8,  r10, r1    @// d0 * sin
                SMULL   r3,  r4,  r9,  r1    @// d1 * sin 
                rsb     r10, r10,  #0                
                SMLAL   r3,  r4,  r10,  r12  @// -d0 * cos
                SMLAL   r5,  r8,  r9,   r12  @//  d1 * cos
                
                mov     r7,  r7,  lsl #1   @@// d0
                
                str     r4,  [IFFT_step4_in_ptr0, #6*4]
                str     r8,  [IFFT_step4_in_ptr0, #7*4]
                mov     r6,  r6,  lsl #1    @@// d1
                SMULL   r5,  r8,  r7, r12    @// d0 * sin
                SMULL   r3,  r4,  r6, r12    @// d1 * sin 
                rsb     r7, r7,  #0                
                SMLAL   r3,  r4,  r7,   r1  @// -d0 * cos
                SMLAL   r5,  r8,  r6,   r1  @//  d1 * cos
                
                str     r4,  [IFFT_step4_in_ptr16, #6*4]
                str     r8,  [IFFT_step4_in_ptr16, #7*4]

                @@@/* third */
                ldmia   IFFT_step4_in_ptr0,  {r3, r4}
                ldmia   IFFT_step4_in_ptr16, {r9, r10}
                ldr     r5,  [IFFT_step4_in_ptr0, #8*4]
                ldr     r6,  [IFFT_step4_in_ptr0, #9*4]                
                ldr     r7,  [IFFT_step4_in_ptr16, #8*4]
                ldr     r8,  [IFFT_step4_in_ptr16, #9*4]                
                
                add     r5,  r5,  r7           @//  t30: r5
                sub     r7,  r5,  r7, lsl #1   @//  t41: r7
                
                add     r8,  r8,  r6           @//  t31: r8
                sub     r6,  r8,  r6, lsl #1   @//  t40: r6
                
                add     r3,  r3,  r9           @//  t20: r3
                sub     r9,  r3,  r9, lsl #1   @//  t10: r9
                
                add     r4,  r4,  r10          @//  t21: r4
                sub     r10, r4,  r10,lsl #1   @//  t11: r10                
                
                add     r9,    tt10,   tt40           @@// r9: c20
                sub     tt40,  tt10,   tt40,  lsl #1                 
                add     r10,  tt11,   tt41            @@// r10: c21
                sub     tt41, tt11,   tt41,  lsl #1                              
                
                
                add     tt20,  tt20,  tt30            @@// sum1, r3
                sub     tt30,  tt20,  tt30, lsl #1
                
                add     tt21,  tt21,  tt31            @//  sum2. r4
                sub     tt31,  tt21,  tt31, lsl #1
                
                stmia   IFFT_step4_in_ptr0!, {tt20,   tt21}                
                rsb     r8,   r8,  #0
                str     r8,    [IFFT_step4_in_ptr16], #4
                str     r5,    [IFFT_step4_in_ptr16], #4
                
                add     r3,    r9,  r10
                sub     r4,    r9,  r10
                
                SMLAWB  r3,    r3,  r11,  r3
                SMLAWB  r4,    r4,  r11,  r4
                
                str     r4,    [IFFT_step4_in_ptr0, #6*4]
                str     r3,    [IFFT_step4_in_ptr0, #7*4]
                
                sub     r3,    r6,  r7
                sub     r4,    r3,  r6, lsl #1
                
                SMLAWB  r3,    r3,  r11,  r3
                SMLAWB  r4,    r4,  r11,  r4
                
                str     r4,    [IFFT_step4_in_ptr16, #6*4]
                str     r3,    [IFFT_step4_in_ptr16, #7*4]
                
                
                
                
                @@@///////////////////////////////////////
                @@@/* fourth */
                ldmia   IFFT_step4_in_ptr0,  {r3, r4}
                ldmia   IFFT_step4_in_ptr16, {r9, r10}
                ldr     r5,  [IFFT_step4_in_ptr0, #8*4]
                ldr     r6,  [IFFT_step4_in_ptr0, #9*4]                
                ldr     r7,  [IFFT_step4_in_ptr16, #8*4]
                ldr     r8,  [IFFT_step4_in_ptr16, #9*4]                
                
                add     r5,  r5,  r7           @//  t30: r5
                sub     r7,  r5,  r7, lsl #1   @//  t41: r7
                
                add     r8,  r8,  r6           @//  t31: r8
                sub     r6,  r8,  r6, lsl #1   @//  t40: r6
                
                add     r3,  r3,  r9           @//  t20: r3
                sub     r9,  r3,  r9, lsl #1   @//  t10: r9
                
                add     r4,  r4,  r10          @//  t21: r4
                sub     r10, r4,  r10,lsl #1   @//  t11: r10                
                
                add     r9,    tt10,   tt40           @@// c20: r9
                sub     tt40,  tt10,   tt40, lsl #1   @@// t40: r6
                add     r10,  tt11,   tt41            @@// c21: r10
                sub     tt41, tt11,   tt41,  lsl #1   @@// t41: r7
                
                add     tt20,  tt20,  tt30            @@// sum1, r3
                sub     tt30,  tt20,  tt30, lsl #1    @@// t30:  r5
                
                add     tt21,  tt21,  tt31            @@// sum2. r4
                sub     tt31,  tt21,  tt31, lsl #1    @@// t31:  r8
                
                stmia   IFFT_step4_in_ptr0!, {tt20,   tt21}             
                
                
                sub     tt31,   tt30,  tt31
                sub     tt30,   tt31,  tt30,  lsl #1
                
                SMLAWB  tt30,   tt30,  r11,  tt30
                SMLAWB  tt31,   tt31,  r11,  tt31                
                mov     r10,r10, lsl #1   @@// d0
                stmia   IFFT_step4_in_ptr16!, {tt30,   tt31}           
                @//str     tt31,  [IFFT_step4_in_ptr16], #4
                @//str     tt30,  [IFFT_step4_in_ptr16], #4
                mov     r9, r9,  lsl #1   @@// d1
                @//buufferFly(c21<<1, c20<<1, 1984016128 >> TEST_BIT_SHIFT, 821806464 >> TEST_BIT_SHIFT, &tmp2, &tmp1)@
                SMULL   r5,  r8,  r10, r12    @// d0 * sin
                SMULL   r3,  r4,  r9,  r12    @// d1 * sin 
                rsb     r10, r10,  #0                
                SMLAL   r3,  r4,  r10,  r1  @// -d0 * cos
                SMLAL   r5,  r8,  r9,   r1  @//  d1 * cos
                
                mvn     r7,  r7,  lsl #1   @@// d0
                
                str     r4,  [IFFT_step4_in_ptr0, #6*4]
                str     r8,  [IFFT_step4_in_ptr0, #7*4]
                mvn     r6,  r6,  lsl #1   @@// d1
                SMULL   r5,  r8,  r7, r1    @// d0 * sin
                SMULL   r3,  r4,  r6, r1    @// d1 * sin 
                rsb     r7, r7,  #0                
                SMLAL   r3,  r4,  r7,   r12  @// -d0 * cos
                SMLAL   r5,  r8,  r6,   r12  @//  d1 * cos
                
                str     r4,  [IFFT_step4_in_ptr16, #6*4]
                str     r8,  [IFFT_step4_in_ptr16, #7*4]

                
                
                
                
                add   IFFT_step4_in_ptr0, IFFT_step4_in_ptr0, #24*4 
                add   IFFT_step4_in_ptr16, IFFT_step4_in_ptr16, #24*4 
                
                
                
                @@//////////////////////////////////////////////////
                subs   r14,  r14,  #1
                BGT    AAC_DEC_ARM_LongIfftStep4AsmLOOP
                
                
                @@//////////////////////////////////////////////////
                ldmfd   sp!, {r4-r12, pc}
                @ENDFUNC
                
                
                @//void AAC_LTP_DEC_IFFTStep5Asm(int32 *in_data_ptr,
                @//                             int32 *out_data_ptr)@
FFT_step5_in_ptr0         .req  r0
FFT_step5_in_ptr1         .req  r2

FFT_step5_out_ptr0        .req  r1
FFT_step5_out_ptr256      .req  r3
FFT_step5_out_ptr512      .req  r4
FFT_step5_out_ptr768      .req  r5


AAC_LTP_DEC_IFFTStep5Asm:@   FUNCTION
                .global  AAC_LTP_DEC_IFFTStep5Asm
                stmfd   sp!, {r4-r12, r14}
                add   FFT_step5_in_ptr1,  FFT_step5_in_ptr0,      #512*4
                add   FFT_step5_out_ptr256, FFT_step5_out_ptr0,   #256*4
                add   FFT_step5_out_ptr512, FFT_step5_out_ptr256, #256*4
                add   FFT_step5_out_ptr768, FFT_step5_out_ptr512, #256*4
                
                mov   r14, #0x34    
AAC_LTP_DEC_FFTStep5Asm_EXT_LOOP0:
AAC_LTP_DEC_FFTStep5Asm_LOOP0:
                @@////////////////////////////////////////////////////////////////
                ldmia   FFT_step5_in_ptr0!, {r6-r12} 
                @@// 0: r6, 1: r7, 2: r8, 3: r9, 4: r10, 5: r11, 6: r12,                
                add    r6,    r6,  r10
                sub    r10,   r6,  r10,  lsl  #1    @@// RE(t2): r6.    RE(t1): r10
                
                add    r7,    r7,  r11
                sub    r11,   r7,  r11,  lsl  #1    @@// IM(t2): r7.    IM(t1): r11
                
                add    r8,    r8,  r12
                sub    r12,   r8,  r12,  lsl  #1    @@// RE(t3): r8.    IM(t4): r12
                
                @@//RE(t2) + RE(t3)@
                @@//RE(t2) - RE(t3)@
                add   r6,   r6,  r8
                sub   r8,   r6,  r8,  lsl #1
                str   r6,   [FFT_step5_out_ptr0], #4 
                ldr   r6,   [FFT_step5_in_ptr0], #121*4               
                str   r8,   [FFT_step5_out_ptr512], #4 
                
                
                add   r6,   r6,  r9
                sub   r9,   r6,  r9,  lsl  #1       @@// IM(t3): r6.    RE(t4): r9
                
                @//out_data_ptr[2*k+1]       = IM(t2) + IM(t3)@
                @//out_data_ptr[2*k+2*256+1] = IM(t2) - IM(t3)@
                add   r7,   r7,  r6
                sub   r6,   r7,  r6,  lsl #1
                str   r7,   [FFT_step5_out_ptr0], #4 
                str   r6,   [FFT_step5_out_ptr512], #4                 
                @//out_data_ptr[2*k+256]     = RE(t1) - RE(t4)@
                @//out_data_ptr[2*k+3*256]   = RE(t1) + RE(t4)@
                sub   r6,   r10,   r9
                add   r9,   r10,   r9                
                @//out_data_ptr[2*k+256+1]   = IM(t1) - IM(t4)@
                @//out_data_ptr[2*k+3*256+1] = IM(t1) + IM(t4)@
                sub   r7,   r11,   r12
                add   r10,  r11,   r12
                                
                stmia FFT_step5_out_ptr256!, {r9, r10}
                stmia FFT_step5_out_ptr768!, {r6, r7}
                
                @@////////////////////////////////////////////////////////////////
                ldmia   FFT_step5_in_ptr1!, {r6-r12} 
                @@// 0: r6, 1: r7, 2: r8, 3: r9, 4: r10, 5: r11, 6: r12,                
                add    r6,    r6,  r10
                sub    r10,   r6,  r10,  lsl  #1    @@// RE(t2): r6.    RE(t1): r10
                
                add    r7,    r7,  r11
                sub    r11,   r7,  r11,  lsl  #1    @@// IM(t2): r7.    IM(t1): r11
                
                add    r8,    r8,  r12
                sub    r12,   r8,  r12,  lsl  #1    @@// RE(t3): r8.    IM(t4): r12
                
                @@//RE(t2) + RE(t3)@
                @@//RE(t2) - RE(t3)@
                add   r6,   r6,  r8
                sub   r8,   r6,  r8,  lsl #1
                str   r6,   [FFT_step5_out_ptr0], #4 
                ldr   r6,   [FFT_step5_in_ptr1], #121*4           
                str   r8,   [FFT_step5_out_ptr512], #4 
                    
                
                add   r6,   r6,  r9
                sub   r9,   r6,  r9,  lsl  #1       @@// IM(t3): r6.    RE(t4): r9
                
                @//out_data_ptr[2*k+1]       = IM(t2) + IM(t3)@
                @//out_data_ptr[2*k+2*256+1] = IM(t2) - IM(t3)@
                add   r7,   r7,  r6
                sub   r6,   r7,  r6,  lsl #1
                str   r7,   [FFT_step5_out_ptr0], #4 
                str   r6,   [FFT_step5_out_ptr512], #4                 
                @//out_data_ptr[2*k+256]     = RE(t1) - RE(t4)@
                @//out_data_ptr[2*k+3*256]   = RE(t1) + RE(t4)@
                sub   r6,   r10,   r9
                add   r9,   r10,   r9                
                @//out_data_ptr[2*k+256+1]   = IM(t1) - IM(t4)@
                @//out_data_ptr[2*k+3*256+1] = IM(t1) + IM(t4)@
                sub   r7,   r11,   r12
                add   r10,  r11,   r12
                                
                stmia FFT_step5_out_ptr256!, {r9, r10}
                stmia FFT_step5_out_ptr768!, {r6, r7}
                                
                sub    r14,  r14,  #1
                ands   r11,  r14,  #0xf
                BNE    AAC_LTP_DEC_FFTStep5Asm_LOOP0
                @@/////////////////////////////////////
                add     r14, r14, #4
                sub     FFT_step5_in_ptr0, FFT_step5_in_ptr0,  #480*4
                sub     FFT_step5_in_ptr1, FFT_step5_in_ptr1,  #480*4
                
                subs    r14, r14,  #0x10                
                BGT     AAC_LTP_DEC_FFTStep5Asm_EXT_LOOP0
                
                
                
                
                
                
                
                sub     FFT_step5_in_ptr0, FFT_step5_in_ptr0,  #120*4
                sub     FFT_step5_in_ptr1, FFT_step5_in_ptr1,  #120*4
                @///////////////////////                
                mov   r14, #0x34    
AAC_LTP_DEC_FFTStep5Asm_EXT_LOOP1:
AAC_LTP_DEC_FFTStep5Asm_LOOP1:
                @@////////////////////////////////////////////////////////////////
                ldmia   FFT_step5_in_ptr0!, {r6-r12} 
                @@// 0: r6, 1: r7, 2: r8, 3: r9, 4: r10, 5: r11, 6: r12,                
                add    r6,    r6,  r10
                sub    r10,   r6,  r10,  lsl  #1    @@// RE(t2): r6.    RE(t1): r10
                
                add    r7,    r7,  r11
                sub    r11,   r7,  r11,  lsl  #1    @@// IM(t2): r7.    IM(t1): r11
                
                add    r8,    r8,  r12
                sub    r12,   r8,  r12,  lsl  #1    @@// RE(t3): r8.    IM(t4): r12
                
                @@//RE(t2) + RE(t3)@
                @@//RE(t2) - RE(t3)@
                add   r6,   r6,  r8
                sub   r8,   r6,  r8,  lsl #1
                str   r6,   [FFT_step5_out_ptr0], #4 
                ldr   r6,   [FFT_step5_in_ptr0], #121*4               
                str   r8,   [FFT_step5_out_ptr512], #4 
                
                
                add   r6,   r6,  r9
                sub   r9,   r6,  r9,  lsl  #1       @@// IM(t3): r6.    RE(t4): r9
                
                @//out_data_ptr[2*k+1]       = IM(t2) + IM(t3)@
                @//out_data_ptr[2*k+2*256+1] = IM(t2) - IM(t3)@
                add   r7,   r7,  r6
                sub   r6,   r7,  r6,  lsl #1
                str   r7,   [FFT_step5_out_ptr0], #4 
                str   r6,   [FFT_step5_out_ptr512], #4                 
                @//out_data_ptr[2*k+256]     = RE(t1) - RE(t4)@
                @//out_data_ptr[2*k+3*256]   = RE(t1) + RE(t4)@
                sub   r6,   r10,   r9
                add   r9,   r10,   r9                
                @//out_data_ptr[2*k+256+1]   = IM(t1) - IM(t4)@
                @//out_data_ptr[2*k+3*256+1] = IM(t1) + IM(t4)@
                sub   r7,   r11,   r12
                add   r10,  r11,   r12
                                
                stmia FFT_step5_out_ptr256!, {r9, r10}
                stmia FFT_step5_out_ptr768!, {r6, r7}
                
                @@////////////////////////////////////////////////////////////////
                ldmia   FFT_step5_in_ptr1!, {r6-r12} 
                @@// 0: r6, 1: r7, 2: r8, 3: r9, 4: r10, 5: r11, 6: r12,                
                add    r6,    r6,  r10
                sub    r10,   r6,  r10,  lsl  #1    @@// RE(t2): r6.    RE(t1): r10
                
                add    r7,    r7,  r11
                sub    r11,   r7,  r11,  lsl  #1    @@// IM(t2): r7.    IM(t1): r11
                
                add    r8,    r8,  r12
                sub    r12,   r8,  r12,  lsl  #1    @@// RE(t3): r8.    IM(t4): r12
                
                @@//RE(t2) + RE(t3)@
                @@//RE(t2) - RE(t3)@
                add   r6,   r6,  r8
                sub   r8,   r6,  r8,  lsl #1
                str   r6,   [FFT_step5_out_ptr0], #4 
                ldr   r6,   [FFT_step5_in_ptr1], #121*4              
                str   r8,   [FFT_step5_out_ptr512], #4 
                
                 
                
                add   r6,   r6,  r9
                sub   r9,   r6,  r9,  lsl  #1       @@// IM(t3): r6.    RE(t4): r9
                
                @//out_data_ptr[2*k+1]       = IM(t2) + IM(t3)@
                @//out_data_ptr[2*k+2*256+1] = IM(t2) - IM(t3)@
                add   r7,   r7,  r6
                sub   r6,   r7,  r6,  lsl #1
                str   r7,   [FFT_step5_out_ptr0], #4 
                str   r6,   [FFT_step5_out_ptr512], #4                 
                @//out_data_ptr[2*k+256]     = RE(t1) - RE(t4)@
                @//out_data_ptr[2*k+3*256]   = RE(t1) + RE(t4)@
                sub   r6,   r10,   r9
                add   r9,   r10,   r9                
                @//out_data_ptr[2*k+256+1]   = IM(t1) - IM(t4)@
                @//out_data_ptr[2*k+3*256+1] = IM(t1) + IM(t4)@
                sub   r7,   r11,   r12
                add   r10,  r11,   r12
                                
                stmia FFT_step5_out_ptr256!, {r9, r10}
                stmia FFT_step5_out_ptr768!, {r6, r7}
                                
                sub    r14,  r14,  #1
                ands   r11,  r14,  #0xf
                BNE    AAC_LTP_DEC_FFTStep5Asm_LOOP1
                @@/////////////////////////////////////
                add     r14, r14, #4
                sub     FFT_step5_in_ptr0, FFT_step5_in_ptr0,  #480*4
                sub     FFT_step5_in_ptr1, FFT_step5_in_ptr1,  #480*4
                
                subs    r14, r14,  #0x10                
                BGT     AAC_LTP_DEC_FFTStep5Asm_EXT_LOOP1    
                
                
                
                
                
                sub     FFT_step5_in_ptr0, FFT_step5_in_ptr0,  #120*4
                sub     FFT_step5_in_ptr1, FFT_step5_in_ptr1,  #120*4
                @///////////////////////                
                mov   r14, #0x34    
AAC_LTP_DEC_FFTStep5Asm_EXT_LOOP2:
AAC_LTP_DEC_FFTStep5Asm_LOOP2:
                @@////////////////////////////////////////////////////////////////
                ldmia   FFT_step5_in_ptr0!, {r6-r12} 
                @@// 0: r6, 1: r7, 2: r8, 3: r9, 4: r10, 5: r11, 6: r12,                
                add    r6,    r6,  r10
                sub    r10,   r6,  r10,  lsl  #1    @@// RE(t2): r6.    RE(t1): r10
                
                add    r7,    r7,  r11
                sub    r11,   r7,  r11,  lsl  #1    @@// IM(t2): r7.    IM(t1): r11
                
                add    r8,    r8,  r12
                sub    r12,   r8,  r12,  lsl  #1    @@// RE(t3): r8.    IM(t4): r12
                
                @@//RE(t2) + RE(t3)@
                @@//RE(t2) - RE(t3)@
                add   r6,   r6,  r8
                sub   r8,   r6,  r8,  lsl #1
                str   r6,   [FFT_step5_out_ptr0], #4 
                ldr   r6,   [FFT_step5_in_ptr0], #121*4               
                str   r8,   [FFT_step5_out_ptr512], #4 
                
                
                
                add   r6,   r6,  r9
                sub   r9,   r6,  r9,  lsl  #1       @@// IM(t3): r6.    RE(t4): r9
                
                @//out_data_ptr[2*k+1]       = IM(t2) + IM(t3)@
                @//out_data_ptr[2*k+2*256+1] = IM(t2) - IM(t3)@
                add   r7,   r7,  r6
                sub   r6,   r7,  r6,  lsl #1
                str   r7,   [FFT_step5_out_ptr0], #4 
                str   r6,   [FFT_step5_out_ptr512], #4                 
                @//out_data_ptr[2*k+256]     = RE(t1) - RE(t4)@
                @//out_data_ptr[2*k+3*256]   = RE(t1) + RE(t4)@
                sub   r6,   r10,   r9
                add   r9,   r10,   r9                
                @//out_data_ptr[2*k+256+1]   = IM(t1) - IM(t4)@
                @//out_data_ptr[2*k+3*256+1] = IM(t1) + IM(t4)@
                sub   r7,   r11,   r12
                add   r10,  r11,   r12
                                
                stmia FFT_step5_out_ptr256!, {r9, r10}
                stmia FFT_step5_out_ptr768!, {r6, r7}
                
                @@////////////////////////////////////////////////////////////////
                ldmia   FFT_step5_in_ptr1!, {r6-r12} 
                @@// 0: r6, 1: r7, 2: r8, 3: r9, 4: r10, 5: r11, 6: r12,                
                add    r6,    r6,  r10
                sub    r10,   r6,  r10,  lsl  #1    @@// RE(t2): r6.    RE(t1): r10
                
                add    r7,    r7,  r11
                sub    r11,   r7,  r11,  lsl  #1    @@// IM(t2): r7.    IM(t1): r11
                
                add    r8,    r8,  r12
                sub    r12,   r8,  r12,  lsl  #1    @@// RE(t3): r8.    IM(t4): r12
                
                @@//RE(t2) + RE(t3)@
                @@//RE(t2) - RE(t3)@
                add   r6,   r6,  r8
                sub   r8,   r6,  r8,  lsl #1
                str   r6,   [FFT_step5_out_ptr0], #4 
                ldr   r6,   [FFT_step5_in_ptr1], #121*4            
                str   r8,   [FFT_step5_out_ptr512], #4 
                   
                
                add   r6,   r6,  r9
                sub   r9,   r6,  r9,  lsl  #1       @@// IM(t3): r6.    RE(t4): r9
                
                @//out_data_ptr[2*k+1]       = IM(t2) + IM(t3)@
                @//out_data_ptr[2*k+2*256+1] = IM(t2) - IM(t3)@
                add   r7,   r7,  r6
                sub   r6,   r7,  r6,  lsl #1
                str   r7,   [FFT_step5_out_ptr0], #4 
                str   r6,   [FFT_step5_out_ptr512], #4                 
                @//out_data_ptr[2*k+256]     = RE(t1) - RE(t4)@
                @//out_data_ptr[2*k+3*256]   = RE(t1) + RE(t4)@
                sub   r6,   r10,   r9
                add   r9,   r10,   r9                
                @//out_data_ptr[2*k+256+1]   = IM(t1) - IM(t4)@
                @//out_data_ptr[2*k+3*256+1] = IM(t1) + IM(t4)@
                sub   r7,   r11,   r12
                add   r10,  r11,   r12
                                
                stmia FFT_step5_out_ptr256!, {r9, r10}
                stmia FFT_step5_out_ptr768!, {r6, r7}
                                
                sub    r14,  r14,  #1
                ands   r11,  r14,  #0xf
                BNE    AAC_LTP_DEC_FFTStep5Asm_LOOP2
                @@/////////////////////////////////////
                add     r14, r14, #4
                sub     FFT_step5_in_ptr0, FFT_step5_in_ptr0,  #480*4
                sub     FFT_step5_in_ptr1, FFT_step5_in_ptr1,  #480*4
                
                subs    r14, r14,  #0x10                
                BGT     AAC_LTP_DEC_FFTStep5Asm_EXT_LOOP2                                

                
                
                
                sub     FFT_step5_in_ptr0, FFT_step5_in_ptr0,  #120*4
                sub     FFT_step5_in_ptr1, FFT_step5_in_ptr1,  #120*4
                @///////////////////////                
                mov   r14, #0x34    
AAC_LTP_DEC_FFTStep5Asm_EXT_LOOP3:
AAC_LTP_DEC_FFTStep5Asm_LOOP3:
                @@////////////////////////////////////////////////////////////////
                ldmia   FFT_step5_in_ptr0!, {r6-r12} 
                @@// 0: r6, 1: r7, 2: r8, 3: r9, 4: r10, 5: r11, 6: r12,                
                add    r6,    r6,  r10
                sub    r10,   r6,  r10,  lsl  #1    @@// RE(t2): r6.    RE(t1): r10
                
                add    r7,    r7,  r11
                sub    r11,   r7,  r11,  lsl  #1    @@// IM(t2): r7.    IM(t1): r11
                
                add    r8,    r8,  r12
                sub    r12,   r8,  r12,  lsl  #1    @@// RE(t3): r8.    IM(t4): r12
                
                @@//RE(t2) + RE(t3)@
                @@//RE(t2) - RE(t3)@
                add   r6,   r6,  r8
                sub   r8,   r6,  r8,  lsl #1
                str   r6,   [FFT_step5_out_ptr0], #4 
                ldr   r6,   [FFT_step5_in_ptr0], #121*4               
                str   r8,   [FFT_step5_out_ptr512], #4 
                
                
                add   r6,   r6,  r9
                sub   r9,   r6,  r9,  lsl  #1       @@// IM(t3): r6.    RE(t4): r9
                
                @//out_data_ptr[2*k+1]       = IM(t2) + IM(t3)@
                @//out_data_ptr[2*k+2*256+1] = IM(t2) - IM(t3)@
                add   r7,   r7,  r6
                sub   r6,   r7,  r6,  lsl #1
                str   r7,   [FFT_step5_out_ptr0], #4 
                str   r6,   [FFT_step5_out_ptr512], #4                 
                @//out_data_ptr[2*k+256]     = RE(t1) - RE(t4)@
                @//out_data_ptr[2*k+3*256]   = RE(t1) + RE(t4)@
                sub   r6,   r10,   r9
                add   r9,   r10,   r9                
                @//out_data_ptr[2*k+256+1]   = IM(t1) - IM(t4)@
                @//out_data_ptr[2*k+3*256+1] = IM(t1) + IM(t4)@
                sub   r7,   r11,   r12
                add   r10,  r11,   r12
                                
                stmia FFT_step5_out_ptr256!, {r9, r10}
                stmia FFT_step5_out_ptr768!, {r6, r7}
                
                @@////////////////////////////////////////////////////////////////
                ldmia   FFT_step5_in_ptr1!, {r6-r12} 
                @@// 0: r6, 1: r7, 2: r8, 3: r9, 4: r10, 5: r11, 6: r12,                
                add    r6,    r6,  r10
                sub    r10,   r6,  r10,  lsl  #1    @@// RE(t2): r6.    RE(t1): r10
                
                add    r7,    r7,  r11
                sub    r11,   r7,  r11,  lsl  #1    @@// IM(t2): r7.    IM(t1): r11
                
                add    r8,    r8,  r12
                sub    r12,   r8,  r12,  lsl  #1    @@// RE(t3): r8.    IM(t4): r12
                
                @@//RE(t2) + RE(t3)@
                @@//RE(t2) - RE(t3)@
                add   r6,   r6,  r8
                sub   r8,   r6,  r8,  lsl #1
                str   r6,   [FFT_step5_out_ptr0], #4 
                ldr   r6,   [FFT_step5_in_ptr1], #121*4               
                str   r8,   [FFT_step5_out_ptr512], #4 
                
                
                add   r6,   r6,  r9
                sub   r9,   r6,  r9,  lsl  #1       @@// IM(t3): r6.    RE(t4): r9
                
                @//out_data_ptr[2*k+1]       = IM(t2) + IM(t3)@
                @//out_data_ptr[2*k+2*256+1] = IM(t2) - IM(t3)@
                add   r7,   r7,  r6
                sub   r6,   r7,  r6,  lsl #1
                str   r7,   [FFT_step5_out_ptr0], #4 
                str   r6,   [FFT_step5_out_ptr512], #4                 
                @//out_data_ptr[2*k+256]     = RE(t1) - RE(t4)@
                @//out_data_ptr[2*k+3*256]   = RE(t1) + RE(t4)@
                sub   r6,   r10,   r9
                add   r9,   r10,   r9                
                @//out_data_ptr[2*k+256+1]   = IM(t1) - IM(t4)@
                @//out_data_ptr[2*k+3*256+1] = IM(t1) + IM(t4)@
                sub   r7,   r11,   r12
                add   r10,  r11,   r12
                                
                stmia FFT_step5_out_ptr256!, {r9, r10}
                stmia FFT_step5_out_ptr768!, {r6, r7}
                                
                sub    r14,  r14,  #1
                ands   r11,  r14,  #0xf
                BNE    AAC_LTP_DEC_FFTStep5Asm_LOOP3
                @@/////////////////////////////////////
                add     r14, r14, #4
                sub     FFT_step5_in_ptr0, FFT_step5_in_ptr0,  #480*4
                sub     FFT_step5_in_ptr1, FFT_step5_in_ptr1,  #480*4
                
                subs    r14, r14,  #0x10                
                BGT     AAC_LTP_DEC_FFTStep5Asm_EXT_LOOP3      
                                            
                ldmfd   sp!, {r4-r12, pc}
                @ENDFUNC
                
                
                
                
                
                @//void AAC_DEC_Imdct128PostReorderAsm(int32 *in_data_ptr,
                @//                                    int32 *out_data_ptr,
                @//                                    int32 *table_ptr)@

in_data64_ptr   .req  r0
in_data62_ptr   .req  r3
out_data0_ptr   .req  r1
out_data255_ptr .req  r4
out_data128_ptr .req  r5
out_data127_ptr .req  r6
table64_ptr     .req  r2
table63_ptr     .req  r7

                
AAC_DEC_Imdct128PostReorderAsm:@   FUNCTION
                .global  AAC_DEC_Imdct128PostReorderAsm
                stmfd   sp!, {r4-r12, r14}
                mov    r14,  #32                
                add    in_data64_ptr,  in_data64_ptr,  #64*4
                sub    in_data62_ptr,  in_data64_ptr,  #1*4
                
                add    out_data255_ptr,  out_data0_ptr,   #255*4
                add    out_data128_ptr,  out_data0_ptr,   #128*4
                sub    out_data127_ptr,  out_data128_ptr,   #1*4
                add    table64_ptr, table64_ptr, #64*2
                sub    table63_ptr, table64_ptr,  #2*2                     
AAC_DEC_Imdct128PostReorderAsmLOOP:
                @//////////////////////////////////////                
                ldmia  in_data64_ptr!, {r8,r9}
                ldr    r10,   [table64_ptr], #4
                @//mov    r8,  r8,  lsl #1
                @//mov    r9,  r9,  lsl #1                
                SMULWT r12,  r9,  r10   @@// d0 * t0
                SMULWT r11,  r8,  r10   @@// d1 * t0
                rsb    r9,   r9,  #0
                SMLAWB r11,  r9,  r10,  r11
                SMLAWB r12,  r8,  r10,  r12                
                str    r11,  [out_data255_ptr], #-4
                str    r11,  [out_data128_ptr], #4
                str    r12,  [out_data0_ptr]  , #4
                rsb    r12,  r12,  #0
                str    r12,  [out_data127_ptr], #-4 
                @@////////////////////////////////
                ldmda  in_data62_ptr!, {r8,r9}
                ldr    r10,   [table63_ptr], #-4
                @//mov    r8,  r8,  lsl #1
                @//mov    r9,  r9,  lsl #1                
                SMULWT r12,  r9,  r10   @@// d0 * t0
                SMULWT r11,  r8,  r10   @@// d1 * t0
                rsb    r9,   r9,  #0
                SMLAWB r11,  r9,  r10,  r11
                SMLAWB r12,  r8,  r10,  r12                
                str    r11,  [out_data127_ptr], #-4
                rsb    r11,  r11,  #0
                str    r11,  [out_data0_ptr]   , #4  
                rsb    r12,  r12,  #0
                str    r12,  [out_data255_ptr], #-4
                str    r12,  [out_data128_ptr], #4     
                
                    
                @//////////////////////////////////////
                subs  r14, r14,  #1
                BGT   AAC_DEC_Imdct128PostReorderAsmLOOP
                
                
                ldmfd   sp!, {r4-r12, pc}
                @ENDFUNC
                
                
                
                
                
                
                
                
                
    	                
        .end@END
        