

        .arm@CODE32

        .text@AREA MP3_PROM, CODE, READONLY
        
@--------------------------------------------------------------------
@   Function:
@      void MP3_DEC_SynthFilter(mp3_fixed_t const (*sbsample)[36][32],int32 ns,
@          short *pcm1,mp3_fixed_t (*filter)[2][2][16][16],  unsigned int phase)
@
@   Description: Perform frequency line alias reduction.
@
@   Register usage:
@       r0 = sbsample
@       r1 = ns
@			  r2 = pcm1
@       r3 = filter
@       sp.. phase
@--------------------------------------------------------------------				



fe   			.req  r0
fx   			.req  r1
fo   			.req  r1
ptr  					.req  r12
ptr_bound 		.req  r4
pcm1 			.req  r7
pcm2      		.req  r11
regd_const 		.req  r3
lo0 				.req  r8
lo1 				.req  r9
lo2 				.req  r10

MP3_DEC_SynthFilter:
        STMFD    sp!,{r4-r12,lr}
        LDR      r4,[sp,#40]    @phase                    
        MOV      r9, r3
        MOV      r10, r0
        MOV      r8, r1
        MOV      r7, r2
        
        SUB      r1, r4, #1          @phase-1
        MOV      r1, r1, LSL #28
        MOV      r1,r1, LSR #29      @t1 = ((phase-1)>>1)&0x7
        
        ANDS     r0,r4,#1        @phase&1        								
        ADDNE    r9,r9,#1024     @fdct32_lo = &(*filter)[0][phase & 1][0][0]@	
          
LOOP_START:
				MOV      r3,r4,LSR #1         @t0 = phase>>1
        ADD      r5,r9,r3,LSL #2      @fe = &fdct32_lo[t0]
          
				ANDS     r0, r4, #1
				ADDEQ    r9, r9, #1024
				SUBNE    r9, r9, #1024
        ADD      r6,r9,r1,LSL #2    @ fx   
        
        STMFD    sp!,{r4, r8, r9, r10}
        STMFD    sp!,{r5, r6, r7}
                
        @ADD      r1, r5,#0xbc0     @fe+47
        @MOV      r0, r10                    
        @BL       dct32
            
@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
@@@************************************************************************@
@@@************************************************************************@
@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
            
        @STR      r1, [sp, #-4]!
        SUB      sp,sp,#96      
        
        @@@@@@@@@@@@*******************:::::::::::::::
        @@@@@@@@@@@@*******************:::::::::::::::
        @@@@@@@@@@@@*******************:::::::::::::::
        LDR			 r1, [r10, #28]
        LDR			 r2, [r10, #96]
        LDR			 r3, [r10, #32]
        LDR			 r4, [r10, #92]
        LDR      r5, c_1_15
        
        ADD			 r1, r1, r2            @t2
        SUB      r2, r1, r2, lsl#1
        SMLAWT   r2, r2, r5, r2        @t18
        ADD      r3, r3, r4            @t3
        SUB      r4, r3, r4, lsl#1
        SMLAWB   r4, r4, r5, r4        @t19
        
        LDR      r5, c_1_1 
        ADD      r2, r2, r4           @t42
        SUB      r4, r2, r4, lsl#1
        SMULWB   r4, r4, r5           @t60
        ADD      r1, r1, r3           @t34
        SUB      r3, r1, r3, lsl#1
        SMULWB   r3, r3, r5           @t51            
        
        LDR			 r6, [r10, #0]
        LDR			 r7, [r10, #124]
        LDR			 r8, [r10, #60]
        LDR			 r9, [r10, #64]      
                
        ADD      r8, r8, r9          @t1
        SUB      r9, r8, r9, lsl#1
        SMULWT   r9, r9, r5          @t17
        LDR      r5, c_1_0                
        ADD      r6,  r6,  r7        @t0
        SUB      r7,  r6,  r7, lsl#1
        SMULL    r14, r0, r7, r5
        LDR      r5, c_1_2        
        MOV			 r7, r0, lsl#1       @t16

				              
        ADD      r7, r7, r9, asr#3   @t41
        SUB      r9, r7, r9, asr#2
        SMULL    r14, r0, r9, r5        
        ADD      r6, r6, r8          @t33
        SUB      r8, r6, r8, lsl#1
        SMULL    r14, r11, r8, r5
        MOV      r9, r0, lsl#1      @t59
        MOV      r8, r11, lsl#1      @t50
        
        @t34-r1 t42-r2 t51-r3  t60-r4
        @t33-r6 t41-r7 t50-r8  t59-r9
             
        LDR      r5, c_1_4
        ADD      r14, r6, r1           @t69
        SUB      r1, r14, r1, lsl#1
        SMULWT   r0, r1, r5
        ADD      r11, r7, r2,  asr#1   @t73
        ADD      r1, r0, r1, asr#1   @t89        
        SUB      r2, r7, r2, asr#1
        SMULWT   r0, r2, r5
        ADD      r8, r8, r3, asr#2    @t78
        ADD      r2, r0, r2, asr#1   @t94        
        SUB      r3, r8, r3, asr#1
        SMULWT   r0, r3, r5
        ADD      r9, r9, r4, asr#3     @t83
        ADD      r3, r0, r3, asr#1    @t100        
        SUB      r4, r9, r4, asr#2
        SMULWT   r0, r4, r5
        STR     r1, [sp, #4]    @t89
        ADD      r4, r0, r4, asr#1    @t106 
        
        @t89-r1 t94-r2 t100-r3  t106-r4
        @t69-r6 t73-r11 t78-r8   t83-r9        
        STR     r2, [sp, #64]    @t94
        STR     r3, [sp, #28]    @t100
        STR     r4, [sp, #76]    @t106
        STR     r11, [sp, #40]    @t73
        STR     r8, [sp, #16]    @t78
        STR     r9, [sp, #52]    @t83
        
        @@@@@@@@@@@@*******************:::::::::::::::
        @@@@@@@@@@@@*******************:::::::::::::::        
        LDR			 r1, [r10, #16]
        LDR			 r2, [r10, #108]
        LDR			 r3, [r10, #44]
        LDR			 r4, [r10, #80]       
        
        LDR      r11, c_1_11
        ADD      r3, r3, r4            @t7
        SUB      r4, r3, r4, lsl#1
        SMULWT   r4, r4, r11           @t23
        ADD			 r1, r1, r2            @t6
        SUB      r2, r1, r2, lsl#1
        SMLAWB   r0, r2, r5, r4       
        ADD      r1, r1, r3            @t36
        SUB      r3, r1, r3, lsl#1
        SMLAWB   r3, r3, r11, r3        @t53                    
        ADD      r2, r0, r2, asr#1    @t44 t22                
        SUB      r4, r2, r4, lsl#1
        SMLAWB   r4, r4, r11,r4         @t62
                
        LDR			 r6, [r10, #12]
        LDR			 r7, [r10, #112]
        LDR			 r8, [r10, #48]
        LDR			 r9, [r10, #76]      
        LDR      r5, c_1_9
                
        ADD      r8, r8, r9          @t5
        SUB      r9, r8, r9, lsl#1
        SMULWB   r9, r9, r5          @t21               
        ADD      r6,  r6,  r7        @t4
        SUB      r7,  r6,  r7, lsl#1
        SMLAWT   r0, r7, r5, r9
        LDR      r5, c_1_5        
        ADD			 r7, r0, r7, asr#1       @t43 t20
				          
        SUB      r9, r7, r9, lsl#1
        SMULWB   r0, r9, r5  
        ADD      r6, r6, r8          @t35        
        SUB      r8, r6, r8, lsl#1
        SMULWB   r11, r8, r5        
        ADD      r9, r0, r9, asr#1  @t61  
        ADD      r8, r11, r8, asr#1  @t52
                    
        @t36-r1 t44-r2 t53-r3  t62-r4
        @t35-r6 t43-r7 t52-r8  t61-r9             
        ADD      r12, r6, r1           @t70
        SUB      r1, r6, r1
        SMULWT   r1, r1, r5          @t90
        ADD      r7, r7, r2          @t74
        SUB      r2, r7, r2, lsl#1
        SMULWT   r2, r2, r5           @t95
        ADD      r0, r8, r3, asr#1    @t79
        SUB      r3, r8, r3, asr#1
        SMULWT   r3, r3, r5            @t101
        ADD      r11, r9, r4, asr#1     @t84
        SUB      r4, r9, r4, asr#1
        SMULWT   r4, r4, r5				    @t107             
        
        
        @t90-r1 t95-r2 t101-r3  t107-r4
        @t70-r12 t74-r7 t79-r8   t84-r9
        STR     r1, [sp, #8]    @t90
        STR     r2, [sp, #68]   @t95
        STR     r3, [sp, #32]   @t101
        STR     r4, [sp, #80]   @t107
        STR     r7, [sp, #44]   @t74
        STR     r0, [sp, #20]   @t79
        STR     r11, [sp, #56]   @t84
        
        @@@@@@@@@@@@*******************:::::::::::::::
        @@@@@@@@@@@@*******************:::::::::::::::
        @@@@@@@@@@@@*******************:::::::::::::::        
        LDR			 r1, [r10, #4]
        LDR			 r2, [r10, #120]
        LDR			 r3, [r10, #56]
        LDR			 r4, [r10, #68]
        LDR      r5, c_1_3
        
        ADD			 r1, r1, r2            @t8
        SUB      r2, r1, r2, lsl#1
        SMULWT   r0, r2, r5
        ADD      r3, r3, r4            @t9
        ADD      r2, r0, r2, asr#1    @t24        
        SUB      r4, r3, r4, lsl#1
        SMULWB   r4, r4, r5            @t25
        
        LDR      r5, c_1_7 
        ADD      r2, r2, r4, asr#1    @t45
        SUB      r4, r2, r4
        SMULWT   r0, r4, r5           
        ADD      r1, r1, r3           @t37
        ADD      r4, r0, r4, asr#1   @t63        
        SUB      r3, r1, r3, lsl#1
        SMULWT   r0, r3, r5       
        LDR			 r6, [r10, #24]
        ADD      r3, r0, r3, asr#1   @t54            
                
        LDR			 r7, [r10, #100]
        LDR			 r8, [r10, #36]
        LDR			 r9, [r10, #88]      
                        
        ADD      r6,  r6,  r7        @t10
        SUB      r7,  r6,  r7, lsl#1
        SMULWB   r0, r7, r5
        LDR      r5, c_1_8              
        ADD      r7, r0, r7, asr#1  @t26           
        ADD      r8, r8, r9          @t11
        SUB      r9, r8, r9, lsl#1
        SMULWB   r0, r9, r5                  
        ADD      r6, r6, r8          @t38
        SUB      r8, r6, r8, lsl#1
        SMULWT   r8, r8, r5          @t55        
        ADD      r9, r9, r0, asr#1  @t27        				              
        ADD      r7, r7, r9, asr#1   @t46
        SUB      r9, r7, r9
        SMULWT   r9, r9, r5          @t64
        
                
        @t37-r1 t45-r2 t54-r3  t63-r4
        @t38-r6 t46-r7 t55-r8  t64-r9
             
        LDR      r5, c_1_14
        ADD      r0, r6, r1           @t71
        SUB      r6, r0, r6, lsl#1
        SMULWB   r11, r6, r5
        ADD      r2, r7, r2           @t75
        ADD      r6, r11, r6, asr#1   @t91        
        SUB      r7, r2, r7, lsl#1
        SMULWB   r11, r7, r5
        ADD      r3, r8, r3           @t80
        ADD      r7, r11, r7, asr#1   @t96      
        SUB      r8, r3, r8, lsl#1
        SMULWB   r11, r8, r5
        ADD      r4, r9, r4           @t85
        ADD      r8, r11, r8, asr#1    @t102        
        SUB      r9, r4, r9, lsl#1
        SMULWB   r11, r9, r5
        STR     r6, [sp, #0]    @t91
        ADD      r9, r11, r9, asr#1    @t108 
        
        @t71-r0 t75-r2 t80 -r3  t85-r4
        @t91-r6  t96-r7 t102-r8  t108-r9        
        STR     r7, [sp, #60]   @t96
        STR     r8, [sp, #24]   @t102
        STR     r9, [sp, #72]   @t108
        STR     r2, [sp, #36]   @t75
        STR     r3, [sp, #12]   @t80
        STR     r4, [sp, #48]   @t85
        
        @@@@@@@@@@@@*******************:::::::::::::::
        @@@@@@@@@@@@*******************:::::::::::::::
        LDR			 r1, [r10, #20]
        LDR			 r2, [r10, #104]
        LDR			 r3, [r10, #40]
        LDR			 r4, [r10, #84]       
              
				ADD      r3, r3, r4            @t15
        SUB      r4, r3, r4, lsl#1
        SMULWT   r11, r4, r5           
        LDR			 r5, c_1_13        
        ADD      r4, r4, r11, asr#4    @t31        
        ADD			 r1, r1, r2            @t14
        SUB      r2, r1, r2, lsl#1
        SMULWB   r11, r2, r5   
        ADD      r1, r1, r3            @t40
        SUB      r3, r1, r3, lsl#1                
        ADD      r2, r11, r2, asr#1    @t30                          
        ADD      r2, r2, r4, asr#1    @t48       
        SUB      r4, r2, r4        
        SMULWT   r4, r4, r5            @t66        
        SMULWT   r3, r3, r5            @t57            
                
        LDR			 r6, [r10, #8]
        LDR			 r7, [r10, #116]
        LDR			 r8, [r10, #52]
        LDR			 r9, [r10, #72]      
        LDR      r5, c_1_6
        
        
        ADD      r6,  r6,  r7          @t12
        SUB      r7,  r6,  r7, lsl#1
        SMULWT   r11, r7, r5
        ADD      r8, r8, r9            @t13        
        ADD			 r7, r11, r7, asr#1    @t28      
        SUB      r9, r8, r9, lsl#1
        SMULWB   r9, r9, r5            @t29               
        
        LDR      r5, c_1_12
        ADD      r7, r7, r9, asr#1		@t47	          
        SUB      r9, r7, r9
        SMLAWT   r11, r9, r5, r4  
        ADD      r6, r6, r8          @t39        
        ADD      r9, r11, r9, asr#1  @t86        
        SUB      r8, r6, r8, lsl#1
        SMLAWT   r11, r8, r5, r3                
        ADD      r6, r6, r1           @t72
        ADD      r8, r11, r8, asr#1  @t81          
          
                    
        @t40-r1 t48-r2 t57-r3  t66-r4
        @t39-r6 t47-r7 t81-r8  t86-r9                          
        SUB      r1, r6, r1, lsl#1
        SMULWB   r11,  r1, r5       
        ADD      r7,  r7, r2           @t76
        ADD      r1,  r1, r11, asr#2   @t92        
        SUB      r2,  r7, r2, lsl#1
        SMULWB   r11,  r2, r5      
        SUB      r3, r8, r3, lsl#1
        ADD      r2, r2, r11, asr#2     @t97                
        SMULWB   r11, r3, r5       
        SUB      r4, r9, r4, lsl#1
        ADD      r3, r3, r11, asr#2     @t103                
        SMULWB   r11, r4, r5
        LDR	 		r5,  [sp, #96]   @lo[0][slot]	
        ADD      r5, r5, #0xbc0 
        ADD      r4, r4, r11, asr#2     @t109             
                
        @t92-r1 t97-r2 t103-r3  t109-r4
        @t72-r6 t76-r7 t81-r8   t86-r9
                
        @@@@@@@@@@@@*******************:::::::::::::::
        @@@@@@@@@@@@*******************:::::::::::::::
        @@@@@@@@@@@@*******************:::::::::::::::        
         ADD   r10, r0, r6         @t114
        
        @t72-r6 t71-r11 t70-r12   t69-r14
        LDR     r11, c_1_10        
        SUB     r6, r10, r6, lsl#1        
        SMULWB  r6, r6, r11         @t142
        
        ADD     r14, r14, r12         @t113
        SUB			r12, r14, r12, lsl#1   @t67-t70
        
        ADD     r14, r14, r10          
        STR			r14, [r5], #32		@hi[15][slot]       
        STR     r14, [r5], #-512     
        
        SUB			r14, r14, r10, lsl#1   @t113-t114                                              
        SMLAWT  r0, r12, r11, r6              			
        LDR     r10, c_1_16
        ADD     r12, r0, r12, asr#1   @t143                
        STR			r12, [r5], #-32		@hi[7][slot]        
                
        SMLAWB  r14, r14, r10,r14 
        STR     r12, [r5], #-2496
        MOV     r14, r14, asr#1
        STR			r14, [r5], #32				@lo[0][slot]
                
        SUB     r12, r12, r6, lsl#1
        SMULWB  r12, r12, r10
        STR     r14, [r5], #512
        SUB     r12, r12, r6, lsl#1
        STR			r12, [r5], #-32					@lo[8][slot]
        STR     r12, [r5], #2240
        
        
        @t92-r1 t97-r2 t103-r3  t109-r4
        @t76-r7 t81-r8   t86-r9        
        LDMIA  sp!,{r6, r12, r14} @t91, t89, t90        
                
        ADD    r6, r6, r1, asr#1  @t126
        SUB    r1, r6, r1        
        SMULWB r1, r1, r11        @t158 **
                
        ADD    r12, r12, r14, asr#1 @t125            
        SUB    r14, r12, r14        @t89-(t90>>1)
                
        ADD    r12, r12, r6          @t93
        STR		 r12, [r5], #32		   @hi[11][slot]
        STR    r12, [r5], #-512
        
        SUB    r6, r12, r6, lsl#1     @t125-t126        
        SMLAWT r0, r14, r11, r1
        SMLAWB r6, r6, r10, r6        @t125 **        
        ADD    r14, r0, r14, asr#1   @t159   **     
        RSB    r12, r12, r14, lsl#1  @t127   **
        STR		 r12, [r5], #-32	  	@hi[3][slot]
        STR    r12, [r5], #-1984
        
        SUB    r6, r6, r12          @t160
        STR		 r6, [r5],#32   @lo[4][slot]        
        
        SUB    r14, r14, r1, lsl#1
        SMULWB r14, r14, r10
        STR    r6, [r5],#512 
        SUB    r14, r14, r1, lsl#1   
        RSB    r6, r6, r14, lsl#1    @t157
        STR		 r6, [r5],#-32 		  	@lo[12][slot]
        STR    r6, [r5],#2112
        
        @t97-r2 t103-r3  t109-r4
        @t76-r7 t81-r8   t86-r9
        LDR     r0, [sp],#4
        LDMIA   sp!, {r1, r6} @t80, t78, t79
                
        ADD     r0, r0, r8        @t119
        SUB     r8, r0, r8, lsl#1
        SMULWB  r8, r8, r11       @t149
        ADD     r1, r1, r6        @t118
        SUB     r6, r1, r6, lsl#1 
        SMLAWT  r12, r6, r11, r8
        ADD     r1, r1, r0          @t58 **
        ADD     r6, r12, r6, asr#1  @t150**        
        STR			r1, [r5], #32		  	@hi[13][slot]
        STR     r1, [r5], #-256
        SUB     r0, r1, r0, lsl#1        
        SUB     r12, r6, r8, lsl#1
        SMULWB  r12, r12, r10
        SMLAWB  r0, r0, r10, r0     @t118 **
        SUB     r8, r12, r8, lsl#1  @t148**
        
        @t97-r2 t103-r3  t109-r4
        @t76-r7 t148-r8   t86-r9        
        LDR     r12, [sp], #4       @t102         
        LDMIA   sp!, {r10, r14}
        
        ADD     r12, r12, r3, asr#1  @t133
        SUB     r3, r12, r3 @, asr#1                
        ADD     r10, r10, r14, asr#1 @t132
        SUB     r14, r10, r14 @, asr#1       
        
                
        ADD     r10, r10, r12       @t104**
        RSB     r1, r1, r10, lsl#1  @t82
        STR			r1, [r5], #-32		  	@hi[9][slot]
        STR     r1, [r5], #-256
            
        
        RSB     r1, r1, r6, lsl#1   @t105**
        STR			r1, [r5], #32	  	  @hi[5][slot]      
           
        
        SMULWB  r3, r3, r11          @t167        
        SMLAWT  r6, r14, r11, r3          
        STR     r1, [r5], #-256   
        ADD     r14, r6, r14, asr#1    @t168
        SUB     r12, r10, r12, lsl#1  @t32-t133        
        RSB     r10, r10, r14, lsl#1   @t134 **
        RSB     r1, r1, r10, lsl#1     @t120 **
        STR			r1, [r5], #-32  	  @hi[1][slot]
        STR     r1, [r5], #-1984
        
        SUB    r0, r0, r1           @t135
        STR		 r0, [r5], #32	  	  @lo[2][slot]
        STR    r0, [r5], #256
        
        LDR     r6, c_1_16        
        SUB     r14, r14, r3, lsl#1        
        SMULWB  r14, r14, r6
        SMLAWB  r12, r12, r6, r12                 @t132
        SUB     r3, r14, r3, lsl#1                  @t166    
                
        SUB     r12, r12, r10           @t169
        RSB     r3, r12, r3, lsl#1      @t166
        RSB     r12, r0, r12, lsl#1     @t151
        STR			r12, [r5], #-32  	  @lo[6][slot]
        STR     r12, [r5], #256
        
        RSB     r12, r12, r8, lsl#1     @t170 
        STR			r12, [r5], #32  	  @lo[10][slot]
        STR     r12, [r5], #256
        
        RSB     r3, r12, r3, lsl#1
        STR			r3, [r5], #-32  	  @lo[14][slot]
        STR     r3, [r5], #2048
            
        @t97-r2 t109-r4
        @t76-r7 t86-r9
        LDR    r0, [sp], #4
        LDMIA  sp!, {r1, r3}  @ t75, t73, t74
        
        ADD    r0, r0, r7       @t116
        SUB    r7, r0, r7, lsl#1
        SMULWB r7, r7, r11      @t145
        ADD    r1, r1, r3         @t115
        SUB    r3, r1, r3, lsl#1
        SMLAWT r8, r3, r11, r7 
        ADD    r1, r1, r0         @t32 **
        ADD    r3, r8, r3, asr#1  @t146**        
        STR		 r1, [r5], #32	  	  @hi[14][slot]
        STR    r1, [r5], #-128
        SUB     r0, r1, r0, lsl#1           
        SUB     r8, r3, r7, lsl#1
        SMULWB  r8, r8, r6
        SMLAWB  r0, r0, r6, r0     @t116 **    
        SUB     r7, r8, r7, lsl#1     @t145 **
        
        @t97-r2 t109-r4
        @t152-r7 t86-r9
        LDR    r8, [sp], #4
        LDMIA  sp!, {r10, r12} @t85, t83, t84
        
        ADD    r8, r8, r9     @t122
        SUB    r9, r8, r9, lsl#1   
        SMULWB r9, r9, r11     @t153     
        ADD    r10, r10, r12    @t121
        SUB    r12, r10, r12, lsl#1        
        SMLAWT r14, r12, r11, r9
        ADD    r10, r10, r8  @t67  **
        ADD    r12, r14, r12, asr#1 @t154**        
        RSB    r1, r1, r10, lsl#1 @t49 **
        STR		 r1, [r5], #-32	  	  @hi[12][slot]        
        SUB     r8, r10, r8, lsl#1
        SMLAWB  r8, r8, r6, r8      @t121 **        
        SUB     r14, r12, r9, lsl#1
        SMULWB  r14, r14, r6 
        STR    r1, [r5], #-128
        SUB     r9, r14, r9, lsl#1  @t152**
        
        STR    r0,[sp, #24]          @t116
        STR    r7,[sp, #28]          @t145
        STR    r9,[sp, #32]          @t152
             
        @t97-r2 t109-r4       
        LDR    r0, [sp], #4
        LDMIA  sp!, {r7, r9}    @t96, t94, t95
        
        ADD      r0, r0, r2, asr#1   @t129
        SUB      r2, r0, r2 
        SMULWB   r2, r2, r11         @t162        
        ADD      r7, r7, r9, asr#1   @t128
        SUB      r9, r7, r9 
        SMLAWT   r14, r9, r11, r2                
        ADD      r9, r14, r9, asr#1  @t163        
        ADD      r14, r7, r0
        RSB      r1, r1, r14, lsl#1   @t68**
        STR			 r1, [r5], #32	  	  @hi[10][slot]
        STR      r1, [r5], #-128
        RSB       r14, r14, r9, lsl#1   @t130**
        SUB       r0, r7, r0
        SUB       r7, r7, r9
        MOV       r7, r7, lsl#1     
        SUB       r9, r9, r2, lsl#1
        SMULWB    r9, r9, r6
        SMLAWB    r0, r0, r6, r7       @t164**
        SUB       r9, r9, r2, lsl#1           
        RSB       r9, r0, r9, lsl#1    @t161**        
        
        @t109-r4   
        LDR      r2, [sp], #4
        LDMIA    sp!, {r6, r7}  @ t108, t106, t107
        
        ADD     r2, r2, r4, asr#1    @t137
        SUB     r4, r2, r4                 
        ADD     r6, r6, r7, asr#1    @t136
        SUB     r7, r6, r7             
        
        ADD     r6, r6, r2    @t110**
        SUB     r2, r6, r2, lsl#1    @t136-t137
        RSB     r10, r10, r6, lsl#1    @t87
        RSB     r1, r1, r10, lsl#1     @t77**
        STR			r1, [r5], #-32	  	  @hi[8][slot]
        STR     r1, [r5], #-128   
        RSB    r12, r10, r12, lsl#1   @t111**
        RSB    r3, r1, r3, lsl#1      @t88
        STR		 r3, [r5], #32	  	  @hi[6][slot]
        STR		 r3, [r5], #-128
        RSB    r3, r3, r12, lsl#1    @t99
        STR		 r3, [r5], #-32		  	  @hi[4][slot]
        STR		 r3, [r5], #-128
        RSB    r14, r3, r14, lsl#1   @t112
        STR		 r14, [r5], #32		  	  @hi[2][slot]
        
                        
        @r10, r1, r3        
        SMULWB  r4, r4, r11          @t172
        SMLAWT  r10, r7, r11, r4
        STR		 r14, [r5], #-128
        ADD     r7, r10, r7, asr#1    @t173**
        
        @r10, r11, r1, r3
        RSB    r6, r6, r7, lsl#1     @t138**
        RSB    r12, r12, r6, lsl#1   @t123**
        SUB    r8, r8, r12           @t139**
        RSB    r14, r14, r12, lsl#1  @t117
        STR		 r14, [r5], #-32	  	  @hi[0][slot]
        STR		 r14, [r5], #-1984
        
        @r10, r11, r1, r3, r12
        LDR    r10, [sp], #4    @t116
        SUB    r10, r10, r14    @t124
        STR		 r10, [r5], #32	  @lo[1][slot]
        STR		 r10, [r5], #128
        
        @r10, r11, r1, r3, r12, r14
        RSB    r10, r10, r8, lsl#1      @t131**
        STR		 r10, [r5], #-32	  	  @lo[3][slot]
        STR		 r10, [r5], #128
        
        RSB    r10, r10, r0, lsl#1      @t140
        STR		 r10, [r5], #32			  	  @lo[5][slot] 
        STR		 r10, [r5], #128
        
         @r0, r10, r11, r1, r3, r12, r14
        LDR   r11, c_1_16
        LDR   r0, [sp], #4    @t145
        LDR   r1, [sp], #4   @t152
        
        MOV    r4, r4, lsl#1
        SUB    r7, r4, r7        
        SMLAWB r2, r2, r11, r2     @t136
        SMLAWB r4, r7, r11, r4     @t171
        SUB    r2, r2, r6          @t174
        ADD    r4, r2, r4, lsl#1   @t171
        RSB    r8, r8, r2, lsl#1   @t155
        RSB    r10, r10, r8, lsl#1  @t147
        STR		 r10, [r5], #-32			  	  @lo[7][slot] 
        STR		 r10, [r5], #128
        
        RSB    r1, r8, r1, lsl#1    @t175
        ADD    r4, r1, r4, lsl#1    @t171
        RSB    r0, r10, r0,lsl#1    @t156
        STR		 r0, [r5], #32			  	  @lo[9][slot] 
        STR		 r0, [r5], #128
        
        RSB    r0, r0, r1, lsl#1    @t165
        STR		 r0, [r5], #-32			  	  @lo[11][slot] 
        STR		 r0, [r5], #128
        
        RSB    r0, r0, r9, lsl#1      @t176
        STR		 r0, [r5], #32			  	  @lo[13][slot] 
        STR		 r0, [r5], #128
        
        ADD    r4, r0, r4, lsl#1
        RSB    r4, r4, #0
        STR		 r4, [r5], #-32			  	  @lo[15][slot] 
        STR		 r4, [r5]
        
@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
@@@************************************************************************@
@@@************************************************************************@
@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
        
        LDMFD    sp!, {r0, r1, r7}                                                           
        @BL       synth_full     
        
        		 @LDR      pcm1,[sp,#16]     @pcm1    		              
     LDR      ptr,L1_1088   @dptr             
				
		 LDR      r5,[fe,#16]!    @fe[4]         				 
		 LDR      r6,[ptr,#12]      @D[3]
		 LDR      r3,[fx,#16] 		 @fx[4]			     
		     
		 SMLAWB   r5, r5, r6, r5    @lo1
		 LDR      lr,[fx,#20]      @fx[5]
		 SMLAWT   r5, r3, r6, r5
		 SMLAWT   r8, lr, r6, r3    @lo0		     
		     
		@fe - r0  fx- r1  **pcm-r2 pcm1-r3  d - r4
		@lo0 - r8  lo1 - r5
         LDR		  r3,  [ptr],#4       @d[0]
         LDR      r6,  [fx], #4   @fx[0] 
         LDR      r9,  [fx], #4   @fx[1]
         LDR      r10, [fe, #-12]   @fe[1]
         LDR      r11, [fe, #12]   @fe[7]
         
         SMLAWT   r8, r6, r3, r8
         LDR      r6, [fx], #20   @fx[2]
         SMLAWB   r5, r10, r3, r5
         LDR      r10, [fe, #-8]  @fe[2]
         SMLAWT   r5, r9, r3, r5
         LDR      r9, [fx], #-16   @fx[7]
         SMLAWB   r5, r11, r3, r5
         LDR      r3, [ptr], #4     @d[1]
         LDR      r11, [fe, #8]  @fe[6]
         
         SMLAWT   r5, r6, r3, r5
         LDR      r6, [fx], #12   @fx[3]
         SMLAWB   r5, r10, r3, r5         
         SMLAWT   r8, r9, r3, r8         
         SMLAWB   r5, r11, r3, r5         
         LDR      r3, [ptr], #20         @d[2]  
         LDR      r10, [fe, #-4]  @fe[3]         
                  
         SMLAWT   r5,  r6, r3, r5         
         LDR      r11, [fe, #4]  @fe[5]   
         SMLAWB   r5,  r10, r3, r5
         LDR      r9, [fx], #1980   @fx[6] -->fo[5]
         SMLAWB   r5,  r11, r3, r5  
         SMLAWT   r8,  r9, r3, r8
         ADD      r5,  r5, lr                                      
		 SUB      r8,r5,r8                  
		                                        		                                        
		 QADD    r8,r8,r8                   
		 MOV     regd_const,#0x8000                
		                                        
		 QADD    r8,r8,r8                   
		 ADD	 pcm2, pcm1, #62               
		                                        
		 QDADD   r8,r3,r8                  
		 ADD	 ptr_bound, ptr, #448@#64                
		                                        
		 MOV      r8,r8,ASR #16             
		 STRH     r8,[pcm1], #2
           
       
        @fe-r0 fo-r1 ptr-r12  pcm1-r7 pcm2-r11 ptr_bound-r4
        @0x8000-r3 
        @r5, r6, r8, r9, r10, lr
        @@@@@@@@@@@@@@@@@@@@@@
        @@******************@@
        @@@@@@@@@@@@@@@@@@@@@@
        @fe[4]  fo[5]   ptr[7]     

@|LOOP1.632|:                
        LDR      r6,[ptr,#0x20]!    @ptr++  ptr[7]             
        LDR      r8,[fo,#0x40]!       @fo[5]
        LDR      r5,[fe,#0x40]!    @fe++, fe[4]
        
        SMLAWB   lo1, r8, r6, r8   @lo1
        SMLAWB   lo2, r5, r6, r5  @lo2
        SUB      lo1, lo1, r5        
        SMLAWT   lo1, r5, r6, lo1        
        LDR      r5, [fo, #-4]       @fo[4]     
        ADD      lo2, lo2, r8
        SMLAWT   lo0, r8, r6, r5  @lo0
        LDR      r6, [ptr, #-4]   @ptr[6]        
        LDR      lr, [fe, #4]      @fe[5]
        
        
        @lo0 - r8  lo1 - r9  lo2 - r10
        SMLAWB   lo0, r5, r6, lo0        
        SMLAWT   lo1, r5, r6, lo1
        LDR      r5, [fe, #-16]    @fe[0]
        SMLAWB   lo1, lr, r6, lo1        
        SMLAWT   lo2, lr, r6, lo2
        LDR      r6,  [ptr, #-28]   @ptr[0]
        ADD      lo1, lo1, lr
        LDR      lr, [fo, #-16]    @fo[1]
       
        SMLAWT   lo2,  r5, r6, lo2
        SMLAWB   lo1,  r5, r6, lo1
        LDR      r5, [fo, #-20]     @fo[0]
        SMLAWB   lo0, lr, r6, lo0
        SMLAWT   lo1, lr, r6, lo1
        LDR      r6, [ptr, #-24]
        LDR      lr, [fe, #-12]    @fe[1]
        
        SMLAWT   lo0, r5, r6, lo0        
        SMLAWB   lo1, r5, r6, lo1
        LDR      r5, [fo, #-12]   @fo[2]
        SMLAWB   lo2, lr, r6, lo2
        SMLAWT   lo1, lr, r6, lo1
        LDR      r6, [ptr, #-20]
        LDR      lr, [fe, #12]    @fe[7]
        
        SMLAWB   lo0, r5, r6, lo0        
        SMLAWT   lo1, r5, r6, lo1
        LDR      r5, [fo, #8]     @fo[7]
        SMLAWT   lo2, lr, r6, lo2
        SMLAWB   lo1, lr, r6, lo1
        LDR      r6, [ptr, #-16]
        LDR      lr, [fe, #-8]    @fe[2]
        
        SMLAWT   lo0, r5, r6, lo0        
        SMLAWB   lo1, r5, r6, lo1
        LDR      r5, [fo, #-8]     @fo[3]
        SMLAWB   lo2, lr, r6, lo2
        SMLAWT   lo1, lr, r6, lo1
        LDR      r6, [ptr, #-12]
        LDR      lr, [fe, #8]    @fe[6]
        
        SMLAWB   lo0, r5, r6, lo0        
        SMLAWT   lo1, r5, r6, lo1
        LDR      r5, [fo, #4]     @fo[6]
        SMLAWT   lo2, lr, r6, lo2
        SMLAWB   lo1, lr, r6, lo1
        LDR      r6, [ptr, #-8]
        LDR      lr, [fe, #-4]    @fe[3]
        
        SMLAWT   lo0, r5, r6, lo0        
        SMLAWB   lo1, r5, r6, lo1        
        SMLAWB   lo2, lr, r6, lo2
        SMLAWT   lo1, lr, r6, lo1
        
        SUB      lo0, lo2, lo0     
                   
        QADD    lo0,lo0,lo0
        QADD    lo1,lo1,lo1             
        
        QADD    lo0,lo0,lo0
        QADD    lo1,lo1,lo1
        
                
        QDADD    lo0,r3,lo0        
        QDADD    lo1,r3,lo1 		@***      
        
        
        MOV      lo0,lo0,ASR #16
        MOV      lo1,lo1,ASR #16            
                     
        
        STRH     lo0,[pcm1],#2  @r3   @pcm1
        STRH     lo1,[pcm2],#-2   @-r3      @pcm2                                 
        
        LDR      r6,[ptr,#0x20]!    @ptr++  ptr[7]             
        LDR      r8,[fo,#0x40]!       @fo[5]
        LDR      r5,[fe,#0x40]!    @fe++, fe[4]
        
        SMLAWB   lo1, r8, r6, r8   @lo1
        SMLAWB   lo2, r5, r6, r5  @lo2
        SUB      lo1, lo1, r5        
        SMLAWT   lo1, r5, r6, lo1        
        LDR      r5, [fo, #-4]       @fo[4]     
        ADD      lo2, lo2, r8
        SMLAWT   lo0, r8, r6, r5  @lo0
        LDR      r6, [ptr, #-4]   @ptr[6]        
        LDR      lr, [fe, #4]      @fe[5]
        
        
        @lo0 - r8  lo1 - r9  lo2 - r10
        SMLAWB   lo0, r5, r6, lo0        
        SMLAWT   lo1, r5, r6, lo1
        LDR      r5, [fe, #-16]    @fe[0]
        SMLAWB   lo1, lr, r6, lo1        
        SMLAWT   lo2, lr, r6, lo2
        LDR      r6,  [ptr, #-28]   @ptr[0]
        ADD      lo1, lo1, lr
        LDR      lr, [fo, #-16]    @fo[1]
       
        SMLAWT   lo2,  r5, r6, lo2
        SMLAWB   lo1,  r5, r6, lo1
        LDR      r5, [fo, #-20]     @fo[0]
        SMLAWB   lo0, lr, r6, lo0
        SMLAWT   lo1, lr, r6, lo1
        LDR      r6, [ptr, #-24]
        LDR      lr, [fe, #-12]    @fe[1]
        
        SMLAWT   lo0, r5, r6, lo0        
        SMLAWB   lo1, r5, r6, lo1
        LDR      r5, [fo, #-12]   @fo[2]
        SMLAWB   lo2, lr, r6, lo2
        SMLAWT   lo1, lr, r6, lo1
        LDR      r6, [ptr, #-20]
        LDR      lr, [fe, #12]    @fe[7]
        
        SMLAWB   lo0, r5, r6, lo0        
        SMLAWT   lo1, r5, r6, lo1
        LDR      r5, [fo, #8]     @fo[7]
        SMLAWT   lo2, lr, r6, lo2
        SMLAWB   lo1, lr, r6, lo1
        LDR      r6, [ptr, #-16]
        LDR      lr, [fe, #-8]    @fe[2]
        
        SMLAWT   lo0, r5, r6, lo0        
        SMLAWB   lo1, r5, r6, lo1
        LDR      r5, [fo, #-8]     @fo[3]
        SMLAWB   lo2, lr, r6, lo2
        SMLAWT   lo1, lr, r6, lo1
        LDR      r6, [ptr, #-12]
        LDR      lr, [fe, #8]    @fe[6]
        
        SMLAWB   lo0, r5, r6, lo0        
        SMLAWT   lo1, r5, r6, lo1
        LDR      r5, [fo, #4]     @fo[6]
        SMLAWT   lo2, lr, r6, lo2
        SMLAWB   lo1, lr, r6, lo1
        LDR      r6, [ptr, #-8]
        LDR      lr, [fe, #-4]    @fe[3]
        
        SMLAWT   lo0, r5, r6, lo0        
        SMLAWB   lo1, r5, r6, lo1        
        SMLAWB   lo2, lr, r6, lo2
        SMLAWT   lo1, lr, r6, lo1
        
        SUB      lo0, lo2, lo0     
                   
        QADD    lo0,lo0,lo0
        QADD    lo1,lo1,lo1             
        
        QADD    lo0,lo0,lo0
        QADD    lo1,lo1,lo1
        
                
        QDADD    lo0,r3,lo0        
        QDADD    lo1,r3,lo1 		@***      
        
        
        MOV      lo0,lo0,ASR #16
        MOV      lo1,lo1,ASR #16            
                     
        
        STRH     lo0,[pcm1],#2  @r3   @pcm1
        STRH     lo1,[pcm2],#-2   @-r3      @pcm2   
        
        @CMP			 ptr, r4  
        @BLT      |LOOP1.632|     
        
        @@@@@@@@@@@@@@@@@@@@@@
        @@******************@@
        @@@@@@@@@@@@@@@@@@@@@@
           
        
        @ADD			 r4, r4, #384@#416
L1_632:        
        LDR      r6,[ptr,#0x20]!    @ptr++  ptr[7]             
        LDR      r8,[fo,#0x40]!       @fo[5]
        LDR      r5,[fe,#0x40]!    @fe++, fe[4]
        
        SMLAWB   lo1, r8, r6, r8   @lo1
        SMLAWB   lo2, r5, r6, r5  @lo2
        SUB      lo1, lo1, r5
        ADD      lo2, lo2, r8
        SMLAWT   lo1, r5, r6, lo1        
        LDR      r5, [fo, #-4]       @fo[4]    
        SMULWT   lo0, r8, r6      @lo0        
        LDR      r6, [ptr, #-4]   @ptr[6]        
        LDR      lr, [fe, #4]      @fe[5]
        
        @lo0 - r8  lo1 - r9  lo2 - r10
        SMLAWB   lo0, r5, r6, lo0        
        SMLAWT   lo1, r5, r6, lo1
        LDR      r5, [fe, #-16]    @fe[0]
        SMLAWB   lo1, lr, r6, lo1        
        SMLAWT   lo2, lr, r6, lo2
        LDR      r6, [ptr, #-28]   @ptr[0]        
        LDR      lr, [fo, #-16]    @fo[1]
       
        SMLAWT   lo2,  r5, r6, lo2
        SMLAWB   lo1,  r5, r6, lo1
        LDR      r5, [fo, #-20]     @fo[0]
        SMLAWB   lo0, lr, r6, lo0
        SMLAWT   lo1, lr, r6, lo1
        LDR      r6, [ptr, #-24]
        LDR      lr, [fe, #-12]    @fe[1]
        
        SMLAWT   lo0, r5, r6, lo0        
        SMLAWB   lo1, r5, r6, lo1
        LDR      r5, [fo, #-12]   @fo[2]
        SMLAWB   lo2, lr, r6, lo2
        SMLAWT   lo1, lr, r6, lo1
        LDR      r6, [ptr, #-20]
        LDR      lr, [fe, #12]    @fe[7]
        
        SMLAWB   lo0, r5, r6, lo0        
        SMLAWT   lo1, r5, r6, lo1
        LDR      r5, [fo, #8]     @fo[7]
        SMLAWT   lo2, lr, r6, lo2
        SMLAWB   lo1, lr, r6, lo1
        LDR      r6, [ptr, #-16]
        LDR      lr, [fe, #-8]    @fe[2]
        
        SMLAWT   lo0, r5, r6, lo0        
        SMLAWB   lo1, r5, r6, lo1
        LDR      r5, [fo, #-8]     @fo[3]
        SMLAWB   lo2, lr, r6, lo2
        SMLAWT   lo1, lr, r6, lo1
        LDR      r6, [ptr, #-12]
        LDR      lr, [fe, #8]    @fe[6]
        
        SMLAWB   lo0, r5, r6, lo0        
        SMLAWT   lo1, r5, r6, lo1
        LDR      r5, [fo, #4]     @fo[6]
        SMLAWT   lo2, lr, r6, lo2
        SMLAWB   lo1, lr, r6, lo1
        LDR      r6, [ptr, #-8]
        LDR      lr, [fe, #-4]    @fe[3]
        
        SMLAWT   lo0, r5, r6, lo0        
        SMLAWB   lo1, r5, r6, lo1        
        SMLAWB   lo2, lr, r6, lo2
        SMLAWT   lo1, lr, r6, lo1
        
        SUB      lo0, lo2, lo0     
                   
        QADD    lo0,lo0,lo0
        QADD    lo1,lo1,lo1             
        
        QADD    lo0,lo0,lo0
        QADD    lo1,lo1,lo1
        
                
        QDADD    lo0,r3,lo0        
        QDADD    lo1,r3,lo1 		@***      
        
        
        MOV      lo0,lo0,ASR #16
        MOV      lo1,lo1,ASR #16            
                     
        
        STRH     lo0,[pcm1],#2  @r3   @pcm1
        STRH     lo1,[pcm2],#-2   @-r3      @pcm2                                 
        
        
                LDR      r6,[ptr,#0x20]!    @ptr++  ptr[7]             
        LDR      r8,[fo,#0x40]!       @fo[5]
         LDR      r5,[fe,#0x40]!    @fe++, fe[4]
        
        SMLAWB   lo1, r8, r6, r8   @lo1
        SMLAWB   lo2, r5, r6, r5  @lo2
        SUB      lo1, lo1, r5
        ADD      lo2, lo2, r8
        SMLAWT   lo1, r5, r6, lo1        
        LDR      r5, [fo, #-4]       @fo[4]    
        SMULWT   lo0, r8, r6      @lo0        
        LDR      r6, [ptr, #-4]   @ptr[6]        
        LDR      lr, [fe, #4]      @fe[5]
        
        @lo0 - r8  lo1 - r9  lo2 - r10
        SMLAWB   lo0, r5, r6, lo0        
        SMLAWT   lo1, r5, r6, lo1
        LDR      r5, [fe, #-16]    @fe[0]
        SMLAWB   lo1, lr, r6, lo1        
        SMLAWT   lo2, lr, r6, lo2
        LDR      r6, [ptr, #-28]   @ptr[0]        
        LDR      lr, [fo, #-16]    @fo[1]
       
        SMLAWT   lo2,  r5, r6, lo2
        SMLAWB   lo1,  r5, r6, lo1
        LDR      r5, [fo, #-20]     @fo[0]
        SMLAWB   lo0, lr, r6, lo0
        SMLAWT   lo1, lr, r6, lo1
        LDR      r6, [ptr, #-24]
        LDR      lr, [fe, #-12]    @fe[1]
        
        SMLAWT   lo0, r5, r6, lo0        
        SMLAWB   lo1, r5, r6, lo1
        LDR      r5, [fo, #-12]   @fo[2]
        SMLAWB   lo2, lr, r6, lo2
        SMLAWT   lo1, lr, r6, lo1
        LDR      r6, [ptr, #-20]
        LDR      lr, [fe, #12]    @fe[7]
        
        SMLAWB   lo0, r5, r6, lo0        
        SMLAWT   lo1, r5, r6, lo1
        LDR      r5, [fo, #8]     @fo[7]
        SMLAWT   lo2, lr, r6, lo2
        SMLAWB   lo1, lr, r6, lo1
        LDR      r6, [ptr, #-16]
        LDR      lr, [fe, #-8]    @fe[2]
        
        SMLAWT   lo0, r5, r6, lo0        
        SMLAWB   lo1, r5, r6, lo1
        LDR      r5, [fo, #-8]     @fo[3]
        SMLAWB   lo2, lr, r6, lo2
        SMLAWT   lo1, lr, r6, lo1
        LDR      r6, [ptr, #-12]
        LDR      lr, [fe, #8]    @fe[6]
        
        SMLAWB   lo0, r5, r6, lo0        
        SMLAWT   lo1, r5, r6, lo1
        LDR      r5, [fo, #4]     @fo[6]
        SMLAWT   lo2, lr, r6, lo2
        SMLAWB   lo1, lr, r6, lo1
        LDR      r6, [ptr, #-8]
        LDR      lr, [fe, #-4]    @fe[3]
        
        SMLAWT   lo0, r5, r6, lo0        
        SMLAWB   lo1, r5, r6, lo1        
        SMLAWB   lo2, lr, r6, lo2
        SMLAWT   lo1, lr, r6, lo1
        
        SUB      lo0, lo2, lo0     
                   
        QADD    lo0,lo0,lo0
        QADD    lo1,lo1,lo1             
        
        QADD    lo0,lo0,lo0
        QADD    lo1,lo1,lo1
        
                
        QDADD    lo0,r3,lo0        
        QDADD    lo1,r3,lo1 		@***      
        
        
        MOV      lo0,lo0,ASR #16
        MOV      lo1,lo1,ASR #16            
                     
        
        STRH     lo0,[pcm1],#2  @r3   @pcm1
        STRH     lo1,[pcm2],#-2   @-r3      @pcm2      
        
        CMP			 ptr, r4                     
        BLT      L1_632
        
        
        LDR      r6,[ptr,#0x20]!    @ptr++  ptr[7]             
        LDR      r8,[fo,#0x40]!       @fo[5]
        LDR      r5,[fe,#0x40]!    @fe++, fe[4]
        
        SMLAWB   lo1, r8, r6, r8   @lo1
        SMLAWB   lo2, r5, r6, r5  @lo2
        SUB      lo1, lo1, r5
        ADD      lo2, lo2, r8
        SMLAWT   lo1, r5, r6, lo1        
        LDR      r5, [fo, #-4]       @fo[4]    
        SMULWT   lo0, r8, r6      @lo0        
        LDR      r6, [ptr, #-4]   @ptr[6]        
        LDR      lr, [fe, #4]      @fe[5]
        
        @lo0 - r8  lo1 - r9  lo2 - r10
        SMLAWB   lo0, r5, r6, lo0        
        SMLAWT   lo1, r5, r6, lo1
        LDR      r5, [fe, #-16]    @fe[0]
        SMLAWB   lo1, lr, r6, lo1        
        SMLAWT   lo2, lr, r6, lo2
        LDR      r6, [ptr, #-28]   @ptr[0]        
        LDR      lr, [fo, #-16]    @fo[1]
       
        SMLAWT   lo2,  r5, r6, lo2
        SMLAWB   lo1,  r5, r6, lo1
        LDR      r5, [fo, #-20]     @fo[0]
        SMLAWB   lo0, lr, r6, lo0
        SMLAWT   lo1, lr, r6, lo1
        LDR      r6, [ptr, #-24]
        LDR      lr, [fe, #-12]    @fe[1]
        
        SMLAWT   lo0, r5, r6, lo0        
        SMLAWB   lo1, r5, r6, lo1
        LDR      r5, [fo, #-12]   @fo[2]
        SMLAWB   lo2, lr, r6, lo2
        SMLAWT   lo1, lr, r6, lo1
        LDR      r6, [ptr, #-20]
        LDR      lr, [fe, #12]    @fe[7]
        
        SMLAWB   lo0, r5, r6, lo0        
        SMLAWT   lo1, r5, r6, lo1
        LDR      r5, [fo, #8]     @fo[7]
        SMLAWT   lo2, lr, r6, lo2
        SMLAWB   lo1, lr, r6, lo1
        LDR      r6, [ptr, #-16]
        LDR      lr, [fe, #-8]    @fe[2]
        
        SMLAWT   lo0, r5, r6, lo0        
        SMLAWB   lo1, r5, r6, lo1
        LDR      r5, [fo, #-8]     @fo[3]
        SMLAWB   lo2, lr, r6, lo2
        SMLAWT   lo1, lr, r6, lo1
        LDR      r6, [ptr, #-12]
        LDR      lr, [fe, #8]    @fe[6]
        
        SMLAWB   lo0, r5, r6, lo0        
        SMLAWT   lo1, r5, r6, lo1
        LDR      r5, [fo, #4]     @fo[6]
        SMLAWT   lo2, lr, r6, lo2
        SMLAWB   lo1, lr, r6, lo1
        LDR      r6, [ptr, #-8]
        LDR      lr, [fe, #-4]    @fe[3]
        
        SMLAWT   lo0, r5, r6, lo0        
        SMLAWB   lo1, r5, r6, lo1        
        SMLAWB   lo2, lr, r6, lo2
        SMLAWT   lo1, lr, r6, lo1
        
        SUB      lo0, lo2, lo0     
                   
        QADD    lo0,lo0,lo0
        QADD    lo1,lo1,lo1             
        
        QADD    lo0,lo0,lo0
        QADD    lo1,lo1,lo1
        
                
        QDADD    lo0,r3,lo0        
        QDADD    lo1,r3,lo1 		@***      
        
        
        MOV      lo0,lo0,ASR #16
        MOV      lo1,lo1,ASR #16            
                     
        
        STRH     lo0,[pcm1],#2  @r3   @pcm1
        STRH     lo1,[pcm2],#-2   @-r3      @pcm2 
        
        
        @@@@@@@@@@@@@@@@@@@@@@
        @@******************@@
        @@@@@@@@@@@@@@@@@@@@@@        
        LDR      r0,[fo,#0x40]!    @fo[5]
        LDR      lr,[ptr,#-480]!   @ptr[3        
        LDR      r5,[fo,#-4]       @fo[4]
        
        SMLAWT    lo0, r0, lr, r0
        LDR       r0, [fo, #-20]    @fo[0]
        SMLAWB    lo0, r5, lr, lo0
        LDR       lr, [ptr, #-12]    @ptr[0]
        LDR       r5, [fo, #-16]     @fo[1]
        
        SMLAWT    lo0, r0, lr, lo0
        LDR       r0, [fo, #8]    @fo[7]
        SMLAWB    lo0, r5, lr, lo0
        LDR       lr, [ptr, #-8]    @ptr[1]
        LDR       r5, [fo, #-12]     @fo[2]
        
        
        SMLAWT    lo0, r0, lr, lo0
        LDR       r0, [fo, #4]    @fo[6]
        SMLAWB    lo0, r5, lr, lo0
        LDR       lr, [ptr, #-4]    @ptr[2]
        LDR       r5, [fo, #-8]     @fo[3]
        
        
        SMLAWT    lo0, r0, lr, lo0        
        LDR		  r4, [sp], #4        
				@ADD       r4,r7,#32 @r3,LSL #4 @pcm1 + nch*16     
        SMLAWB    lo0, r5, lr, lo0            
        @STR      r4,[sp,#16]     @*pcm = pcm1+nch*16
        LDR		 r9, [sp, #4]        
             
        QADD    lo0,lo0,lo0
        LDR		  r10, [sp, #8]        
                         
        QADD    lo0,lo0,lo0        
        ADD      r10, r10,#128        
        
        QDADD   lo0,r3,lo0@***              
        MOV      r1, r4, lsr#1      @t1 = ((phase-1)>>1)&0x7    
            
        MOV      lo0,lo0,ASR #16        
        STRH     lo0,[r7,#0]       
        
        ADD      r7,r7,#32 @r3,LSL #4 @pcm1 + nch*16             
        LDR      r8, [sp], #12 
            
       @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
        @@@************************************************************************@
        @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@               
        ADD      r4, r4, #1
        AND      r4, r4, #0xf        @(phase+1)%16     
        SUBS     r8, r8, #1       
        BGT      LOOP_START
      
        @ADD      sp, sp, #4
        LDMFD    sp!,{r4-r12,pc}
c_1_15:
        .word      0x7b5e57d7
c_1_1:
        .word      0x647e645f
c_1_0:
        .word      0x7fd8878e
c_1_2:
        .word      0x7f62368f
c_1_4:
        .word      0x7b15676c
c_1_11:
        .word      0x6d7444cf
c_1_9:
        .word      0x7109563e
c_1_5:
        .word      0x63e345e4
c_1_3:
        .word      0x7d3b4b20
c_1_7:
        .word      0x74fa4d9f
c_1_8:
        .word      0x4a5061ff
c_1_14:
        .word      0x738854db
c_1_13:
        .word      0x78ad5b94
c_1_6:
        .word      0x78547c68
c_1_12:
        .word      0x61c671cf
c_1_10:
        .word      0x6c8361f8
c_1_16:
        .word      0x00006a0a
L1_1088:
        .word      constdata_1        
        @ENDP
        
        .text@AREA ||.constdata||, DATA, READONLY, ALIGN=2

constdata_1:
        .word      0xffe300d5
        .word      0xfe3507f5
        .word      0xebdf19ae
        .word      0x6d8f251e
        .word      0x0068fffb
        .word      0x061f0092
        .word      0x25ffffd3
        .word      0xfa13d909
        .word      0xffff001a
        .word      0xffe100d0
        .word      0x00da0191
        .word      0xfdf9080f
        .word      0x07d012b4
        .word      0xea731bde
        .word      0x17478b38
        .word      0x665824f0
        .word      0xffff0018
        .word      0xffdd00ca
        .word      0x00de015b
        .word      0xfdbb0820
        .word      0x07a01149
        .word      0xe9091dd8
        .word      0x14a883ff
        .word      0x5f282468
        .word      0xffff0015
        .word      0xffda00c4
        .word      0x00e10126
        .word      0xfd7b0827
        .word      0x07650fdf
        .word      0xe7a31f9c
        .word      0x11d17ccb
        .word      0x58022386
        .word      0xffff0013
        .word      0xffd700be
        .word      0x00e300f4
        .word      0xfd390825
        .word      0x071e0e79
        .word      0xe643212c
        .word      0x0ec075a0
        .word      0x50eb2249
        .word      0xffff0011
        .word      0xffd300b7
        .word      0x00e400c5
        .word      0xfcf5081b
        .word      0x06cb0d17
        .word      0xe4e92288
        .word      0x0b776e81
        .word      0x49e720b4
        .word      0xffff0010
        .word      0xffcf00b0
        .word      0x00e40099
        .word      0xfcb00809
        .word      0x066c0bbc
        .word      0xe39923b3
        .word      0x07f56772
        .word      0x42fa1ec7
        .word      0xfffe000e
        .word      0xffcb00a9
        .word      0x00e3006f
        .word      0xfc6907f0
        .word      0x05ff0a67
        .word      0xe25324ad
        .word      0x043a6076
        .word      0x3c271c83
        .word      0xfffe000d
        .word      0xffc600a1
        .word      0x00e00048
        .word      0xfc2107d1
        .word      0x0586091a
        .word      0xe11a2578
        .word      0x00465991
        .word      0x357319e9
        .word      0xfffe000b
        .word      0xffc1009a
        .word      0x00dd0024
        .word      0xfbd807aa
        .word      0x050007d6
        .word      0xdfef2616
        .word      0xfc1a52c5
        .word      0x2ee216fc
        .word      0xfffe000a
        .word      0xffbc0093
        .word      0x00d70002
        .word      0xfb8f077f
        .word      0x046b069c
        .word      0xded52687
        .word      0xf7b64c16
        .word      0x287613be
        .word      0xfffd0009
        .word      0xffb7008b
        .word      0x00d0ffe3
        .word      0xfb46074e
        .word      0x03ca056c
        .word      0xddcd26cf
        .word      0xf31c4587
        .word      0x2236102f
        .word      0xfffd0008
        .word      0xffb10084
        .word      0x00c8ffc7
        .word      0xfafd0719
        .word      0x031a0447
        .word      0xdcda26ee
        .word      0xee4b3f1b
        .word      0x1c230c54
        .word      0xfffc0007
        .word      0xffab007d
        .word      0x00bdffad
        .word      0xfab406df
        .word      0x025d032e
        .word      0xdbfd26e7
        .word      0xe94638d4
        .word      0x1642082d
        .word      0xfffc0007
        .word      0xffa50075
        .word      0x00b1ff96
        .word      0xfa6c06a2
        .word      0x01920221
        .word      0xdb3826bc
        .word      0xe40e32b4
        .word      0x109703be
        .word      0xfffb0006
        .word      0xff9f006f
        .word      0x00a3ff81
        .word      0xfa260662
        .word      0x00b90120
        .word      0xda8f266e
        .word      0xdea42cbf
        .word      0x0b24ff0a
                
		@IMPORT dct32
		@IMPORT synth_full        
        .global  MP3_DEC_SynthFilter
        
        .end@END