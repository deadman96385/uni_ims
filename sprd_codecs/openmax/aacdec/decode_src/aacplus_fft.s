@//fixed point DCT-IV                                                                                     
@//input: 64 sample dct coefficient                                                                       
@	  r0: input/output sample pointer                                                                       
@ purpose:                                                                                                
                                                                                                          
                                                                                                          
i			      .req		  r14                                                                                            
tab         .req      r12                                                                                    
img_ptr     .req      r12                                                                                    
rel_ptr     .req      r0                                                                                    
tmp_ptr     .req      r0                                                                                    
                                                                                                          
                                                                                                          
                                                                                                          
			.text@AREA	PROGRAM, CODE, READONLY				                                                                
			.arm@CODE32				                                                                                      
			.global	asm_DCT_IV                                                                                  
asm_DCT_IV:
		stmfd 	sp!, {r4 - r7, r14}                                                                           
		ldr tab, =DCT_IV_TAB                                                                                  
                                                                                                          
        @@/* 1 */                                                                                         
		@//add tmp_ptr, r0, #0                                                                                
		ldr r1, [tmp_ptr, #0x0]  @//point1_real = Real[i]                                                     
		ldr r2, [tmp_ptr, #0x40] @//point2_real = Real[i+16]                                                  
		ldr r3, [tmp_ptr, #0x80] @//point1_imag = Real[i+32]                                                  
		ldr r4, [tmp_ptr, #0xc0] @//point2_imag = Real[i+48]		                                              
		                                                                                                      
		add r5, r1, r2@ //Real[i]    += point2_real		                                                        
		add r6, r4, r3@ //Real[i+32] += point2_imag                                                           
		                                                                                                      
		str r5, [tmp_ptr, #0x0]                                                                               
		str r6, [tmp_ptr, #0x80]                                                                              
		                                                                                                      
		sub r1, r1, r2@ //point1_real -= point2_real@                                                         
		sub r3, r3, r4@ //point1_imag -= point2_imag@                                                         
		@ //Real[i+16] = (d0) >> 14@                                                                          
        @ //Real[i+48] = (d1) >> 14@                                                                      
		str r1, [tmp_ptr, #0x40]		                                                                          
		str r3, [tmp_ptr, #0xc0]		                                                                          
		ldr r1, [tmp_ptr, #0x20] @//point1_real = Real[i+8]                                                   
		ldr r2, [tmp_ptr, #0x60] @//point2_real = Real[i+24]                                                  
		ldr r3, [tmp_ptr, #0xa0] @//point1_imag = Real[i+40]                                                  
		ldr r4, [tmp_ptr, #0xe0] @//point2_imag = Real[i+56]		                                              
		@ //Real[i+8]    += point2_real                                                                       
		add r5, r1, r2                                                                                        
		str r5, [tmp_ptr, #0x20]                                                                              
		@ //Real[i+40] += point2_imag                                                                         
		add r5, r4, r3                                                                                        
		str r5, [tmp_ptr, #0xa0]		                                                                          
        sub r1, r2, r1@ //point1_real -= point2_real@                                                     
        sub r3, r3, r4@ //point1_imag -= point2_imag@				                                              
		str r3, [tmp_ptr, #0x60]                                                                              
		str r1, [tmp_ptr, #0xe0]                                                                              
		                                                                                                      
		@@/* 2 not use r11 */                                                                                 
		@//add tmp_ptr, r0, #4                                                                                
		ldr r1, [tmp_ptr, #0x0 +4]  @//point1_real = Real[i]                                                  
		ldr r2, [tmp_ptr, #0x40+4] @//point2_real = Real[i+16]                                                
		ldr r3, [tmp_ptr, #0x80+4] @//point1_imag = Real[i+32]                                                
		ldr r4, [tmp_ptr, #0xc0+4] @//point2_imag = Real[i+48]                                                
				                                                                                                  
		add r5, r1, r2@ //Real[i]    += point2_real		                                                        
		add r6, r4, r3@ //Real[i+32] += point2_imag                                                           
		                                                                                                      
		str r5, [tmp_ptr, #0x0+4]                                                                             
		str r6, [tmp_ptr, #0x80+4]		                                                                        
                                                                                                          
		sub r1, r1, r2 @// r1: point1_real -= point2_real@                                                    
		sub r3, r3, r4 @// r3: point1_imag -= point2_imag@                                                    
		ldr  r14, [tab], #4                                                                                   
		                                                                                                      
		@ //d0  = -point1_imag * w_imag@		                                                                  
		@ //d0 +=  point1_real * w_real@                                                                      
		add      r2,    r1,    r3,  asr  #1                                                                   
		sub      r2,    r2,    r1,  asr  #1 @@// d0, (point1_real + (point1_imag>>1) - (point1_real>>1))      
		sub      r4,    r2,    r1           @@// d1, ((point1_imag>>1) - (point1_real>>1))                    
		                                                                                                      
		SMLAWT   r4,    r3,   r14, r4                                                                         
		rsb      r3,    r3,  #0                                                                               
		SMLAWB   r2,    r3,   r14, r2		                                                                      
		SMLAWB   r4,    r1,   r14, r4                                                                         
		SMLAWT   r2,    r1,   r14, r2		                                                                      
		                                                                                                      
		str r4, [tmp_ptr, #0xc0+4]@ //Real[i+48] = (d1) >> 14@		                                            
		str r2, [tmp_ptr, #0x40+4]@ //Real[i+16] = (d0) >> 14@                                                
		                                                                                                      
		ldr r1, [tmp_ptr, #0x20+4] @//point1_real = Real[i+8]                                                 
		ldr r2, [tmp_ptr, #0x60+4] @//point2_real = Real[i+24]                                                
		ldr r3, [tmp_ptr, #0xa0+4] @//point1_imag = Real[i+40]                                                
		ldr r4, [tmp_ptr, #0xe0+4] @//point2_imag = Real[i+56]                                                
		                                                                                                      
		@ //Real[i+8]    += point2_real                                                                       
		add r5, r1, r2                                                                                        
		str r5, [tmp_ptr, #0x20+4]                                                                            
		@ //Real[i+40] += point2_imag                                                                         
		add r5, r4, r3                                                                                        
		str r5, [tmp_ptr, #0xa0+4]	                                                                          
                                                                                                          
        sub r1, r1, r2@ //point1_real -= point2_real@                                                     
        sub r3, r3, r4	@ //point1_imag -= point2_imag@                                                   
	    add      r2,    r1,    r3,  asr  #1                                                                 
		sub      r2,    r2,    r1,  asr  #1 @@// d1, (point1_real + (point1_imag>>1) - (point1_real>>1))      
		sub      r4,    r2,    r1           @@// d0, ((point1_imag>>1) - (point1_real>>1))                    
		SMLAWT   r2,    r1,   r14,  r2                                                                        
	    SMLAWT   r4,    r3,   r14,  r4                                                                      
	    rsb      r2,  r2,  #0	                                                                              
	    SMLAWB   r2,    r3,   r14,  r2	                                                                    
	    SMLAWB   r4,    r1,   r14,  r4                                                                      
		str r4, [tmp_ptr, #0x60+4]                                                                            
		str r2, [tmp_ptr, #0xe0+4]			                                                                      
		                                                                                                      
		                                                                                                      
		@@/* 3 */                                                                                             
		@//add tmp_ptr, r0, #8                                                                                
		ldr r1, [tmp_ptr, #0x0+8]  @//point1_real = Real[i]                                                   
		ldr r2, [tmp_ptr, #0x40+8] @//point2_real = Real[i+16]                                                
		ldr r3, [tmp_ptr, #0x80+8] @//point1_imag = Real[i+32]                                                
		ldr r4, [tmp_ptr, #0xc0+8] @//point2_imag = Real[i+48]                                                
				                                                                                                  
		add r5, r1, r2@ //Real[i]    += point2_real		                                                        
		add r6, r4, r3@ //Real[i+32] += point2_imag                                                           
		                                                                                                      
		str r5, [tmp_ptr, #0x0+8]                                                                             
		str r6, [tmp_ptr, #0x80+8]		                                                                        
                                                                                                          
		sub r1, r1, r2 @// r1: point1_real -= point2_real@                                                    
		sub r3, r3, r4 @// r3: point1_imag -= point2_imag@                                                    
		ldr  r14, [tab], #4                                                                                   
		                                                                                                      
		@ //d0  = -point1_imag * w_imag@		                                                                  
		@ //d0 +=  point1_real * w_real@                                                                      
		add      r2,    r1,    r3,  asr  #1                                                                   
		sub      r2,    r2,    r1,  asr  #1 @@// d0, (point1_real + (point1_imag>>1) - (point1_real>>1))      
		sub      r4,    r2,    r1           @@// d1, ((point1_imag>>1) - (point1_real>>1))                    
		                                                                                                      
		SMLAWT   r4,    r3,   r14, r4                                                                         
		rsb      r3,    r3,  #0                                                                               
		SMLAWB   r2,    r3,   r14, r2		                                                                      
		SMLAWB   r4,    r1,   r14, r4                                                                         
		SMLAWT   r2,    r1,   r14, r2		                                                                      
		                                                                                                      
		str r4, [tmp_ptr, #0xc0+8]@ //Real[i+48] = (d1) >> 14@		                                            
		str r2, [tmp_ptr, #0x40+8]@ //Real[i+16] = (d0) >> 14@                                                
		                                                                                                      
		ldr r1, [tmp_ptr, #0x20+8] @//point1_real = Real[i+8]                                                 
		ldr r2, [tmp_ptr, #0x60+8] @//point2_real = Real[i+24]                                                
		ldr r3, [tmp_ptr, #0xa0+8] @//point1_imag = Real[i+40]                                                
		ldr r4, [tmp_ptr, #0xe0+8] @//point2_imag = Real[i+56]                                                
		                                                                                                      
		@ //Real[i+8]    += point2_real                                                                       
		add r5, r1, r2                                                                                        
		str r5, [tmp_ptr, #0x20+8]                                                                            
		@ //Real[i+40] += point2_imag                                                                         
		add r5, r4, r3                                                                                        
		str r5, [tmp_ptr, #0xa0+8]	                                                                          
                                                                                                          
        sub r1, r1, r2@ //point1_real -= point2_real@                                                     
        sub r3, r3, r4	@ //point1_imag -= point2_imag@                                                   
	    add      r2,    r1,    r3,  asr  #1                                                                 
		sub      r2,    r2,    r1,  asr  #1 @@// d1, (point1_real + (point1_imag>>1) - (point1_real>>1))      
		sub      r4,    r2,    r1           @@// d0, ((point1_imag>>1) - (point1_real>>1))                    
		SMLAWT   r2,    r1,   r14,  r2                                                                        
	    SMLAWT   r4,    r3,   r14,  r4                                                                      
	    rsb      r2,  r2,  #0	                                                                              
	    SMLAWB   r2,    r3,   r14,  r2	                                                                    
	    SMLAWB   r4,    r1,   r14,  r4                                                                      
		str r4, [tmp_ptr, #0x60+8]                                                                            
		str r2, [tmp_ptr, #0xe0+8]			                                                                      
		                                                                                                      
		@@/* 4 */                                                                                             
		@//add tmp_ptr, r0, #12                                                                               
		ldr r1, [tmp_ptr, #0x0+12]  @//point1_real = Real[i]                                                  
		ldr r2, [tmp_ptr, #0x40+12] @//point2_real = Real[i+16]                                               
		ldr r3, [tmp_ptr, #0x80+12] @//point1_imag = Real[i+32]                                               
		ldr r4, [tmp_ptr, #0xc0+12] @//point2_imag = Real[i+48]                                               
				                                                                                                  
		add r5, r1, r2@ //Real[i]    += point2_real		                                                        
		add r6, r4, r3@ //Real[i+32] += point2_imag                                                           
		                                                                                                      
		str r5, [tmp_ptr, #0x0+12]                                                                            
		str r6, [tmp_ptr, #0x80+12]		                                                                        
                                                                                                          
		sub r1, r1, r2 @// r1: point1_real -= point2_real@                                                    
		sub r3, r3, r4 @// r3: point1_imag -= point2_imag@                                                    
		ldr  r14, [tab], #4                                                                                   
		                                                                                                      
		@ //d0  = -point1_imag * w_imag@		                                                                  
		@ //d0 +=  point1_real * w_real@                                                                      
		add      r2,    r1,    r3,  asr  #1                                                                   
		sub      r2,    r2,    r1,  asr  #1 @@// d0, (point1_real + (point1_imag>>1) - (point1_real>>1))      
		sub      r4,    r2,    r1           @@// d1, ((point1_imag>>1) - (point1_real>>1))                    
		                                                                                                      
		SMLAWT   r4,    r3,   r14, r4                                                                         
		rsb      r3,    r3,  #0                                                                               
		SMLAWB   r2,    r3,   r14, r2		                                                                      
		SMLAWB   r4,    r1,   r14, r4                                                                         
		SMLAWT   r2,    r1,   r14, r2		                                                                      
		                                                                                                      
		str r4, [tmp_ptr, #0xc0+12]@ //Real[i+48] = (d1) >> 14@		                                            
		str r2, [tmp_ptr, #0x40+12]@ //Real[i+16] = (d0) >> 14@                                               
		                                                                                                      
		ldr r1, [tmp_ptr, #0x20+12] @//point1_real = Real[i+8]                                                
		ldr r2, [tmp_ptr, #0x60+12] @//point2_real = Real[i+24]                                               
		ldr r3, [tmp_ptr, #0xa0+12] @//point1_imag = Real[i+40]                                               
		ldr r4, [tmp_ptr, #0xe0+12] @//point2_imag = Real[i+56]                                               
		                                                                                                      
		@ //Real[i+8]    += point2_real                                                                       
		add r5, r1, r2                                                                                        
		str r5, [tmp_ptr, #0x20+12]                                                                           
		@ //Real[i+40] += point2_imag                                                                         
		add r5, r4, r3                                                                                        
		str r5, [tmp_ptr, #0xa0+12]	                                                                          
                                                                                                          
        sub r1, r1, r2@ //point1_real -= point2_real@                                                     
        sub r3, r3, r4	@ //point1_imag -= point2_imag@                                                   
	    add      r2,    r1,    r3,  asr  #1                                                                 
		sub      r2,    r2,    r1,  asr  #1 @@// d1, (point1_real + (point1_imag>>1) - (point1_real>>1))      
		sub      r4,    r2,    r1           @@// d0, ((point1_imag>>1) - (point1_real>>1))                    
		SMLAWT   r2,    r1,   r14,  r2                                                                        
	    SMLAWT   r4,    r3,   r14,  r4                                                                      
	    rsb      r2,  r2,  #0	                                                                              
	    SMLAWB   r2,    r3,   r14,  r2	                                                                    
	    SMLAWB   r4,    r1,   r14,  r4                                                                      
		str r4, [tmp_ptr, #0x60+12]                                                                           
		str r2, [tmp_ptr, #0xe0+12]				                                                                    
		                                                                                                      
		@@/* 5 */                                                                                             
		@//add tmp_ptr, r0, #16                                                                               
		ldr r1, [tmp_ptr, #0x0+16]  @//point1_real = Real[i]                                                  
		ldr r2, [tmp_ptr, #0x40+16] @//point2_real = Real[i+16]                                               
		ldr r3, [tmp_ptr, #0x80+16] @//point1_imag = Real[i+32]                                               
		ldr r4, [tmp_ptr, #0xc0+16] @//point2_imag = Real[i+48]                                               
				                                                                                                  
		add r5, r1, r2@ //Real[i]    += point2_real		                                                        
		add r6, r4, r3@ //Real[i+32] += point2_imag                                                           
		                                                                                                      
		str r5, [tmp_ptr, #0x0+16]                                                                            
		str r6, [tmp_ptr, #0x80+16]		                                                                        
                                                                                                          
		sub r1, r1, r2 @// r1: point1_real -= point2_real@                                                    
		sub r3, r3, r4 @// r3: point1_imag -= point2_imag@                                                    
		ldr  r14, [tab], #4                                                                                   
		                                                                                                      
		@ //d0  = -point1_imag * w_imag@		                                                                  
		@ //d0 +=  point1_real * w_real@                                                                      
		add      r2,    r1,    r3,  asr  #1                                                                   
		sub      r2,    r2,    r1,  asr  #1 @@// d0, (point1_real + (point1_imag>>1) - (point1_real>>1))      
		sub      r4,    r2,    r1           @@// d1, ((point1_imag>>1) - (point1_real>>1))                    
		                                                                                                      
		SMLAWT   r4,    r3,   r14, r4                                                                         
		rsb      r3,    r3,  #0                                                                               
		SMLAWB   r2,    r3,   r14, r2		                                                                      
		SMLAWB   r4,    r1,   r14, r4                                                                         
		SMLAWT   r2,    r1,   r14, r2		                                                                      
		                                                                                                      
		str r4, [tmp_ptr, #0xc0+16]@ //Real[i+48] = (d1) >> 14@		                                            
		str r2, [tmp_ptr, #0x40+16]@ //Real[i+16] = (d0) >> 14@                                               
		                                                                                                      
		ldr r1, [tmp_ptr, #0x20+16] @//point1_real = Real[i+8]                                                
		ldr r2, [tmp_ptr, #0x60+16] @//point2_real = Real[i+24]                                               
		ldr r3, [tmp_ptr, #0xa0+16] @//point1_imag = Real[i+40]                                               
		ldr r4, [tmp_ptr, #0xe0+16] @//point2_imag = Real[i+56]                                               
		                                                                                                      
		@ //Real[i+8]    += point2_real                                                                       
		add r5, r1, r2                                                                                        
		str r5, [tmp_ptr, #0x20+16]                                                                           
		@ //Real[i+40] += point2_imag                                                                         
		add r5, r4, r3                                                                                        
		str r5, [tmp_ptr, #0xa0+16]	                                                                          
                                                                                                          
        sub r1, r1, r2@ //point1_real -= point2_real@                                                     
        sub r3, r3, r4	@ //point1_imag -= point2_imag@                                                   
	    add      r2,    r1,    r3,  asr  #1                                                                 
		sub      r2,    r2,    r1,  asr  #1 @@// d1, (point1_real + (point1_imag>>1) - (point1_real>>1))      
		sub      r4,    r2,    r1           @@// d0, ((point1_imag>>1) - (point1_real>>1))                    
		SMLAWT   r2,    r1,   r14,  r2                                                                        
	    SMLAWT   r4,    r3,   r14,  r4                                                                      
	    rsb      r2,  r2,  #0	                                                                              
	    SMLAWB   r2,    r3,   r14,  r2	                                                                    
	    SMLAWB   r4,    r1,   r14,  r4                                                                      
		str r4, [tmp_ptr, #0x60+16]                                                                           
		str r2, [tmp_ptr, #0xe0+16]			                                                                      
		                                                                                                      
		@@/* 6 */                                                                                             
		@//add tmp_ptr, r0, #20                                                                               
		ldr r1, [tmp_ptr, #0x0+20]  @//point1_real = Real[i]                                                  
		ldr r2, [tmp_ptr, #0x40+20] @//point2_real = Real[i+16]                                               
		ldr r3, [tmp_ptr, #0x80+20] @//point1_imag = Real[i+32]                                               
		ldr r4, [tmp_ptr, #0xc0+20] @//point2_imag = Real[i+48]                                               
				                                                                                                  
		add r5, r1, r2@ //Real[i]    += point2_real		                                                        
		add r6, r4, r3@ //Real[i+32] += point2_imag                                                           
		                                                                                                      
		str r5, [tmp_ptr, #0x0+20]                                                                            
		str r6, [tmp_ptr, #0x80+20]		                                                                        
                                                                                                          
		sub r1, r1, r2 @// r1: point1_real -= point2_real@                                                    
		sub r3, r3, r4 @// r3: point1_imag -= point2_imag@                                                    
		ldr  r14, [tab], #4                                                                                   
		                                                                                                      
		@ //d0  = -point1_imag * w_imag@		                                                                  
		@ //d0 +=  point1_real * w_real@                                                                      
		add      r2,    r1,    r3,  asr  #1                                                                   
		sub      r2,    r2,    r1,  asr  #1 @@// d0, (point1_real + (point1_imag>>1) - (point1_real>>1))      
		sub      r4,    r2,    r1           @@// d1, ((point1_imag>>1) - (point1_real>>1))                    
		                                                                                                      
		SMLAWT   r4,    r3,   r14, r4                                                                         
		rsb      r3,    r3,  #0                                                                               
		SMLAWB   r2,    r3,   r14, r2		                                                                      
		SMLAWB   r4,    r1,   r14, r4                                                                         
		SMLAWT   r2,    r1,   r14, r2		                                                                      
		                                                                                                      
		str r4, [tmp_ptr, #0xc0+20]@ //Real[i+48] = (d1) >> 14@		                                            
		str r2, [tmp_ptr, #0x40+20]@ //Real[i+16] = (d0) >> 14@                                               
		                                                                                                      
		ldr r1, [tmp_ptr, #0x20+20] @//point1_real = Real[i+8]                                                
		ldr r2, [tmp_ptr, #0x60+20] @//point2_real = Real[i+24]                                               
		ldr r3, [tmp_ptr, #0xa0+20] @//point1_imag = Real[i+40]                                               
		ldr r4, [tmp_ptr, #0xe0+20] @//point2_imag = Real[i+56]                                               
		                                                                                                      
		@ //Real[i+8]    += point2_real                                                                       
		add r5, r1, r2                                                                                        
		str r5, [tmp_ptr, #0x20+20]                                                                           
		@ //Real[i+40] += point2_imag                                                                         
		add r5, r4, r3                                                                                        
		str r5, [tmp_ptr, #0xa0+20]	                                                                          
                                                                                                          
        sub r1, r1, r2@ //point1_real -= point2_real@                                                     
        sub r3, r3, r4	@ //point1_imag -= point2_imag@                                                   
	    add      r2,    r1,    r3,  asr  #1                                                                 
		sub      r2,    r2,    r1,  asr  #1 @@// d1, (point1_real + (point1_imag>>1) - (point1_real>>1))      
		sub      r4,    r2,    r1           @@// d0, ((point1_imag>>1) - (point1_real>>1))                    
		SMLAWT   r2,    r1,   r14,  r2                                                                        
	    SMLAWT   r4,    r3,   r14,  r4                                                                      
	    rsb      r2,  r2,  #0	                                                                              
	    SMLAWB   r2,    r3,   r14,  r2	                                                                    
	    SMLAWB   r4,    r1,   r14,  r4                                                                      
		str r4, [tmp_ptr, #0x60+20]                                                                           
		str r2, [tmp_ptr, #0xe0+20]				                                                                    
		                                                                                                      
		@@/* 7 */                                                                                             
		@//add tmp_ptr, r0, #24                                                                               
		ldr r1, [tmp_ptr, #0x0+24]  @//point1_real = Real[i]                                                  
		ldr r2, [tmp_ptr, #0x40+24] @//point2_real = Real[i+16]                                               
		ldr r3, [tmp_ptr, #0x80+24] @//point1_imag = Real[i+32]                                               
		ldr r4, [tmp_ptr, #0xc0+24] @//point2_imag = Real[i+48]                                               
				                                                                                                  
		add r5, r1, r2@ //Real[i]    += point2_real		                                                        
		add r6, r4, r3@ //Real[i+32] += point2_imag                                                           
		                                                                                                      
		str r5, [tmp_ptr, #0x0+24]                                                                            
		str r6, [tmp_ptr, #0x80+24]		                                                                        
                                                                                                          
		sub r1, r1, r2 @// r1: point1_real -= point2_real@                                                    
		sub r3, r3, r4 @// r3: point1_imag -= point2_imag@                                                    
		ldr  r14, [tab], #4                                                                                   
		                                                                                                      
		@ //d0  = -point1_imag * w_imag@		                                                                  
		@ //d0 +=  point1_real * w_real@                                                                      
		add      r2,    r1,    r3,  asr  #1                                                                   
		sub      r2,    r2,    r1,  asr  #1 @@// d0, (point1_real + (point1_imag>>1) - (point1_real>>1))      
		sub      r4,    r2,    r1           @@// d1, ((point1_imag>>1) - (point1_real>>1))                    
		                                                                                                      
		SMLAWT   r4,    r3,   r14, r4                                                                         
		rsb      r3,    r3,  #0                                                                               
		SMLAWB   r2,    r3,   r14, r2		                                                                      
		SMLAWB   r4,    r1,   r14, r4                                                                         
		SMLAWT   r2,    r1,   r14, r2		                                                                      
		                                                                                                      
		str r4, [tmp_ptr, #0xc0+24]@ //Real[i+48] = (d1) >> 14@		                                            
		str r2, [tmp_ptr, #0x40+24]@ //Real[i+16] = (d0) >> 14@                                               
		                                                                                                      
		ldr r1, [tmp_ptr, #0x20+24] @//point1_real = Real[i+8]                                                
		ldr r2, [tmp_ptr, #0x60+24] @//point2_real = Real[i+24]                                               
		ldr r3, [tmp_ptr, #0xa0+24] @//point1_imag = Real[i+40]                                               
		ldr r4, [tmp_ptr, #0xe0+24] @//point2_imag = Real[i+56]                                               
		                                                                                                      
		@ //Real[i+8]    += point2_real                                                                       
		add r5, r1, r2                                                                                        
		str r5, [tmp_ptr, #0x20+24]                                                                           
		@ //Real[i+40] += point2_imag                                                                         
		add r5, r4, r3                                                                                        
		str r5, [tmp_ptr, #0xa0+24]	                                                                          
                                                                                                          
        sub r1, r1, r2@ //point1_real -= point2_real@                                                     
        sub r3, r3, r4	@ //point1_imag -= point2_imag@                                                   
	    add      r2,    r1,    r3,  asr  #1                                                                 
		sub      r2,    r2,    r1,  asr  #1 @@// d1, (point1_real + (point1_imag>>1) - (point1_real>>1))      
		sub      r4,    r2,    r1           @@// d0, ((point1_imag>>1) - (point1_real>>1))                    
		SMLAWT   r2,    r1,   r14,  r2                                                                        
	    SMLAWT   r4,    r3,   r14,  r4                                                                      
	    rsb      r2,  r2,  #0	                                                                              
	    SMLAWB   r2,    r3,   r14,  r2	                                                                    
	    SMLAWB   r4,    r1,   r14,  r4                                                                      
		str r4, [tmp_ptr, #0x60+24]                                                                           
		str r2, [tmp_ptr, #0xe0+24]			                                                                      
		                                                                                                      
		@@/* 8 */                                                                                             
		@//add tmp_ptr, r0, #28                                                                               
		ldr r1, [tmp_ptr, #0x0+28]  @//point1_real = Real[i]                                                  
		ldr r2, [tmp_ptr, #0x40+28] @//point2_real = Real[i+16]                                               
		ldr r3, [tmp_ptr, #0x80+28] @//point1_imag = Real[i+32]                                               
		ldr r4, [tmp_ptr, #0xc0+28] @//point2_imag = Real[i+48]                                               
				                                                                                                  
		add r5, r1, r2@ //Real[i]    += point2_real		                                                        
		add r6, r4, r3@ //Real[i+32] += point2_imag                                                           
		                                                                                                      
		str r5, [tmp_ptr, #0x0+28]                                                                            
		str r6, [tmp_ptr, #0x80+28]		                                                                        
                                                                                                          
		sub r1, r1, r2 @// r1: point1_real -= point2_real@                                                    
		sub r3, r3, r4 @// r3: point1_imag -= point2_imag@                                                    
		ldr  r14, [tab], #4                                                                                   
		                                                                                                      
		@ //d0  = -point1_imag * w_imag@		                                                                  
		@ //d0 +=  point1_real * w_real@                                                                      
		add      r2,    r1,    r3,  asr  #1                                                                   
		sub      r2,    r2,    r1,  asr  #1 @@// d0, (point1_real + (point1_imag>>1) - (point1_real>>1))      
		sub      r4,    r2,    r1           @@// d1, ((point1_imag>>1) - (point1_real>>1))                    
		                                                                                                      
		SMLAWT   r4,    r3,   r14, r4                                                                         
		rsb      r3,    r3,  #0                                                                               
		SMLAWB   r2,    r3,   r14, r2		                                                                      
		SMLAWB   r4,    r1,   r14, r4                                                                         
		SMLAWT   r2,    r1,   r14, r2		                                                                      
		                                                                                                      
		str r4, [tmp_ptr, #0xc0+28]@ //Real[i+48] = (d1) >> 14@		                                            
		str r2, [tmp_ptr, #0x40+28]@ //Real[i+16] = (d0) >> 14@                                               
		                                                                                                      
		ldr r1, [tmp_ptr, #0x20+28] @//point1_real = Real[i+8]                                                
		ldr r2, [tmp_ptr, #0x60+28] @//point2_real = Real[i+24]                                               
		ldr r3, [tmp_ptr, #0xa0+28] @//point1_imag = Real[i+40]                                               
		ldr r4, [tmp_ptr, #0xe0+28] @//point2_imag = Real[i+56]                                               
		                                                                                                      
		@ //Real[i+8]    += point2_real                                                                       
		add r5, r1, r2                                                                                        
		str r5, [tmp_ptr, #0x20+28]                                                                           
		@ //Real[i+40] += point2_imag                                                                         
		add r5, r4, r3                                                                                        
		str r5, [tmp_ptr, #0xa0+28]	                                                                          
                                                                                                          
        sub r1, r1, r2@ //point1_real -= point2_real@                                                     
        sub r3, r3, r4	@ //point1_imag -= point2_imag@                                                   
	    add      r2,    r1,    r3,  asr  #1                                                                 
		sub      r2,    r2,    r1,  asr  #1 @@// d1, (point1_real + (point1_imag>>1) - (point1_real>>1))      
		sub      r4,    r2,    r1           @@// d0, ((point1_imag>>1) - (point1_real>>1))                    
		SMLAWT   r2,    r1,   r14,  r2                                                                        
	    SMLAWT   r4,    r3,   r14,  r4                                                                      
	    rsb      r2,  r2,  #0	                                                                              
	    SMLAWB   r2,    r3,   r14,  r2	                                                                    
	    SMLAWB   r4,    r1,   r14,  r4                                                                      
		str r4, [tmp_ptr, #0x60+28]                                                                           
		str r2, [tmp_ptr, #0xe0+28]		                                                                        
                                                                                                          
		                                                                                                      
		                                                                                                      
		@ //step 2                                                                                            
        @@/////1                                                                                          
        ldr r1, [tmp_ptr, #0x0]  @//point1_real = Real[i]                                                 
		ldr r2, [tmp_ptr, #0x80] @//point1_imag = Real[i+32]                                                  
		ldr r3, [tmp_ptr, #0x20] @//point2_real = Real[i+8]                                                   
		ldr r4, [tmp_ptr, #0xa0] @//point2_imag = Real[i+40] 		                                              
                                                                                                          
		sub r6, r1, r3@ //point1_real -= point2_real@                                                         
		sub r7, r2, r4@ //point1_imag -= point2_imag@		                                                      
                                                                                                          
		add r1, r1, r3@ //Real[i]    += point2_real@                                                          
		str r1, [tmp_ptr, #0x0]                                                                               
		add r2, r2, r4@ //Real[i+32] += point2_imag@                                                          
		str r2, [tmp_ptr, #0x80]		                                                                          
				                                                                                                  
		str r6, [tmp_ptr, #0x20] @ //Real[i+8]  = (d0) >> 14@		                                              
	    str r7, [tmp_ptr, #0xa0] @ //Real[i+40] = (d1) >> 14@	                                              
	    	                                                                                                  
		ldr r1, [tmp_ptr, #0x40] @//point1_real = Real[i+16]                                                  
		ldr r2, [tmp_ptr, #0xc0] @//point1_imag = Real[i+48]                                                  
		ldr r3, [tmp_ptr, #0x60] @//point2_real = Real[i+24]                                                  
		ldr r4, [tmp_ptr, #0xe0] @//point2_imag = Real[i+56] 		                                              
                                                                                                          
		sub r6, r1, r3@ //point1_real -= point2_real@                                                         
		sub r7, r2, r4@ //point1_imag -= point2_imag@                                                         
		                                                                                                      
		@ //Real[i+16] += point2_real@                                                                        
        @ //Real[i+48] += point2_imag@                                                                    
		add r1, r1, r3                                                                                        
		str r1, [tmp_ptr, #0x40]                                                                              
		add r2, r2, r4                                                                                        
		str r2, [tmp_ptr, #0xc0]				                                                                      
		str r6, [tmp_ptr, #0x60] @ //Real[i+24] = (d0) >> 14@	                                                
        str r7, [tmp_ptr, #0xe0] @ //Real[i+56] = (d1) >> 14@                                             
                                                                                                          
                                                                                                          
        @@/////2                                                                                          
        ldr r1, [tmp_ptr, #0x0+4]  @//point1_real = Real[i]                                               
		ldr r2, [tmp_ptr, #0x80+4] @//point1_imag = Real[i+32]                                                
		ldr r3, [tmp_ptr, #0x20+4] @//point2_real = Real[i+8]                                                 
		ldr r4, [tmp_ptr, #0xa0+4] @//point2_imag = Real[i+40] 		                                            
                                                                                                          
		sub r6, r1, r3@ //point1_real -= point2_real@                                                         
		sub r7, r2, r4@ //point1_imag -= point2_imag@                                                         
		add r1, r1, r3                                                                                        
		str r1, [tmp_ptr, #0x0+4] @ //Real[i]    += point2_real@                                              
		add r2, r2, r4                                                                                        
		str r2, [tmp_ptr, #0x80+4]@ //Real[i+32] += point2_imag@                                              
		                                                                                                      
		ldr r14,   [tab], #4		                                                                              
		add     r2,   r6,  r7,  asr  #1                                                                       
		sub     r2,   r2,  r6,  asr  #1 @@// (point1_real + (point1_imag>>1) - (point1_real>>1))  -->> d0@    
		sub     r4,   r2,  r6           @@// ((point1_imag>>1) - (point1_real>>1)) -->> d1@                   
		                                                                                                      
		SMLAWT  r4,   r7,  r14,  r4                                                                           
		rsb     r7,   r7,  #0                                                                                 
		SMLAWB  r2,   r7,  r14,  r2                                                                           
		SMLAWB  r4,   r6,  r14,  r4                                                                           
		SMLAWT  r2,   r6,  r14,  r2		                                                                        
		str r4, [tmp_ptr, #0xa0+4] @ //Real[i+40] = (d1) >> 14@	                                              
		str r2, [tmp_ptr, #0x20+4] @ //Real[i+8]  = (d0) >> 14@                                               
        	                                                                                                
		ldr r1, [tmp_ptr, #0x40+4] @//point1_real = Real[i+16]                                                
		ldr r2, [tmp_ptr, #0xc0+4] @//point1_imag = Real[i+48]                                                
		ldr r3, [tmp_ptr, #0x60+4] @//point2_real = Real[i+24]                                                
		ldr r4, [tmp_ptr, #0xe0+4] @//point2_imag = Real[i+56] 		                                            
                                                                                                          
		sub r6, r1, r3@ //point1_real -= point2_real@                                                         
		sub r7, r2, r4@ //point1_imag -= point2_imag@                                                         
		add r1, r1, r3                                                                                        
		str r1, [tmp_ptr, #0x40+4]@ //Real[i+16] += point2_real@                                              
		add r2, r2, r4                                                                                        
		str r2, [tmp_ptr, #0xc0+4]@ //Real[i+48] += point2_imag@                                              
		                                                                                                      
		add     r2,   r6,  r7,  asr  #1                                                                       
		sub     r2,   r2,  r6,  asr  #1 @@// (point1_real + (point1_imag>>1) - (point1_real>>1))  -->> d0@    
		sub     r4,   r2,  r6           @@// ((point1_imag>>1) - (point1_real>>1)) -->> d1@                   
		SMLAWT  r4,   r7,  r14,  r4                                                                           
		rsb     r7,   r7,  #0                                                                                 
		SMLAWB  r2,   r7,  r14,  r2                                                                           
		SMLAWB  r4,   r6,  r14,  r4                                                                           
		SMLAWT  r2,   r6,  r14,  r2			                                                                      
		str     r4, [tmp_ptr, #0xe0+4] @ //Real[i+56] = (d1) >> 14@                                           
		str     r2, [tmp_ptr, #0x60+4] @ //Real[i+24] = (d0) >> 14@                                           
                                                                                                          
        @@/////3                                                                                          
		ldr r1, [tmp_ptr, #0x0+8]  @//point1_real = Real[i]                                                   
		ldr r2, [tmp_ptr, #0x80+8] @//point1_imag = Real[i+32]                                                
		ldr r3, [tmp_ptr, #0x20+8] @//point2_real = Real[i+8]                                                 
		ldr r4, [tmp_ptr, #0xa0+8] @//point2_imag = Real[i+40] 		                                            
                                                                                                          
		sub r6, r1, r3@ //point1_real -= point2_real@                                                         
		sub r7, r2, r4@ //point1_imag -= point2_imag@                                                         
		add r1, r1, r3                                                                                        
		str r1, [tmp_ptr, #0x0+8] @ //Real[i]    += point2_real@                                              
		add r2, r2, r4                                                                                        
		str r2, [tmp_ptr, #0x80+8]@ //Real[i+32] += point2_imag@                                              
		                                                                                                      
		ldr r14,   [tab], #4		                                                                              
		add     r2,   r6,  r7,  asr  #1                                                                       
		sub     r2,   r2,  r6,  asr  #1 @@// (point1_real + (point1_imag>>1) - (point1_real>>1))  -->> d0@    
		sub     r4,   r2,  r6           @@// ((point1_imag>>1) - (point1_real>>1)) -->> d1@                   
		                                                                                                      
		SMLAWT  r4,   r7,  r14,  r4                                                                           
		rsb     r7,   r7,  #0                                                                                 
		SMLAWB  r2,   r7,  r14,  r2                                                                           
		SMLAWB  r4,   r6,  r14,  r4                                                                           
		SMLAWT  r2,   r6,  r14,  r2		                                                                        
		str r4, [tmp_ptr, #0xa0+8] @ //Real[i+40] = (d1) >> 14@	                                              
		str r2, [tmp_ptr, #0x20+8] @ //Real[i+8]  = (d0) >> 14@                                               
        	                                                                                                
		ldr r1, [tmp_ptr, #0x40+8] @//point1_real = Real[i+16]                                                
		ldr r2, [tmp_ptr, #0xc0+8] @//point1_imag = Real[i+48]                                                
		ldr r3, [tmp_ptr, #0x60+8] @//point2_real = Real[i+24]                                                
		ldr r4, [tmp_ptr, #0xe0+8] @//point2_imag = Real[i+56] 		                                            
                                                                                                          
		sub r6, r1, r3@ //point1_real -= point2_real@                                                         
		sub r7, r2, r4@ //point1_imag -= point2_imag@                                                         
		add r1, r1, r3                                                                                        
		str r1, [tmp_ptr, #0x40+8]@ //Real[i+16] += point2_real@                                              
		add r2, r2, r4                                                                                        
		str r2, [tmp_ptr, #0xc0+8]@ //Real[i+48] += point2_imag@                                              
		                                                                                                      
		add     r2,   r6,  r7,  asr  #1                                                                       
		sub     r2,   r2,  r6,  asr  #1 @@// (point1_real + (point1_imag>>1) - (point1_real>>1))  -->> d0@    
		sub     r4,   r2,  r6           @@// ((point1_imag>>1) - (point1_real>>1)) -->> d1@                   
		SMLAWT  r4,   r7,  r14,  r4                                                                           
		rsb     r7,   r7,  #0                                                                                 
		SMLAWB  r2,   r7,  r14,  r2                                                                           
		SMLAWB  r4,   r6,  r14,  r4                                                                           
		SMLAWT  r2,   r6,  r14,  r2			                                                                      
		str     r4, [tmp_ptr, #0xe0+8] @ //Real[i+56] = (d1) >> 14@                                           
		str     r2, [tmp_ptr, #0x60+8] @ //Real[i+24] = (d0) >> 14@                                           
                                                                                                          
        @@/////4                                                                                          
		ldr r1, [tmp_ptr, #0x0+12]  @//point1_real = Real[i]                                                  
		ldr r2, [tmp_ptr, #0x80+12] @//point1_imag = Real[i+32]                                               
		ldr r3, [tmp_ptr, #0x20+12] @//point2_real = Real[i+8]                                                
		ldr r4, [tmp_ptr, #0xa0+12] @//point2_imag = Real[i+40] 		                                          
                                                                                                          
		sub r6, r1, r3@ //point1_real -= point2_real@                                                         
		sub r7, r2, r4@ //point1_imag -= point2_imag@                                                         
		add r1, r1, r3                                                                                        
		str r1, [tmp_ptr, #0x0+12] @ //Real[i]    += point2_real@                                             
		add r2, r2, r4                                                                                        
		str r2, [tmp_ptr, #0x80+12]@ //Real[i+32] += point2_imag@                                             
		                                                                                                      
		ldr r14,   [tab], #-2*4		                                                                            
		add     r2,   r6,  r7,  asr  #1                                                                       
		sub     r2,   r2,  r6,  asr  #1 @@// (point1_real + (point1_imag>>1) - (point1_real>>1))  -->> d0@    
		sub     r4,   r2,  r6           @@// ((point1_imag>>1) - (point1_real>>1)) -->> d1@                   
		                                                                                                      
		SMLAWT  r4,   r7,  r14,  r4                                                                           
		rsb     r7,   r7,  #0                                                                                 
		SMLAWB  r2,   r7,  r14,  r2                                                                           
		SMLAWB  r4,   r6,  r14,  r4                                                                           
		SMLAWT  r2,   r6,  r14,  r2		                                                                        
		str r4, [tmp_ptr, #0xa0+12] @ //Real[i+40] = (d1) >> 14@	                                            
		str r2, [tmp_ptr, #0x20+12] @ //Real[i+8]  = (d0) >> 14@                                              
        	                                                                                                
		ldr r1, [tmp_ptr, #0x40+12] @//point1_real = Real[i+16]                                               
		ldr r2, [tmp_ptr, #0xc0+12] @//point1_imag = Real[i+48]                                               
		ldr r3, [tmp_ptr, #0x60+12] @//point2_real = Real[i+24]                                               
		ldr r4, [tmp_ptr, #0xe0+12] @//point2_imag = Real[i+56] 		                                          
                                                                                                          
		sub r6, r1, r3@ //point1_real -= point2_real@                                                         
		sub r7, r2, r4@ //point1_imag -= point2_imag@                                                         
		add r1, r1, r3                                                                                        
		str r1, [tmp_ptr, #0x40+12]@ //Real[i+16] += point2_real@                                             
		add r2, r2, r4                                                                                        
		str r2, [tmp_ptr, #0xc0+12]@ //Real[i+48] += point2_imag@                                             
		                                                                                                      
		add     r2,   r6,  r7,  asr  #1                                                                       
		sub     r2,   r2,  r6,  asr  #1 @@// (point1_real + (point1_imag>>1) - (point1_real>>1))  -->> d0@    
		sub     r4,   r2,  r6           @@// ((point1_imag>>1) - (point1_real>>1)) -->> d1@                   
		SMLAWT  r4,   r7,  r14,  r4                                                                           
		rsb     r7,   r7,  #0                                                                                 
		SMLAWB  r2,   r7,  r14,  r2                                                                           
		SMLAWB  r4,   r6,  r14,  r4                                                                           
		SMLAWT  r2,   r6,  r14,  r2			                                                                      
		str     r4, [tmp_ptr, #0xe0+12] @ //Real[i+56] = (d1) >> 14@                                          
		str     r2, [tmp_ptr, #0x60+12] @ //Real[i+24] = (d0) >> 14@		                                      
		                                                                                                      
                                                                                                          
        @@/////5		                                                                                      
        ldr r1, [tmp_ptr, #0x0+16]  @//point1_real = Real[i]                                              
		ldr r2, [tmp_ptr, #0x80+16] @//point1_imag = Real[i+32]                                               
		ldr r3, [tmp_ptr, #0x20+16] @//point2_real = Real[i+8]                                                
		ldr r4, [tmp_ptr, #0xa0+16] @//point2_imag = Real[i+40] 		                                          
                                                                                                          
		sub r6, r3, r1@ //point1_real -= point2_real@                                                         
		sub r7, r2, r4@ //point1_imag -= point2_imag@                                                         
		add r1, r1, r3                                                                                        
		str r1, [tmp_ptr, #0x0+16]@ //Real[i]    += point2_real@                                              
		add r2, r2, r4                                                                                        
		str r2, [tmp_ptr, #0x80+16]@ //Real[i+32] += point2_imag@                                             
				                                                                                                  
		str r7, [tmp_ptr, #0x20+16] @ //Real[i+8]  = (d0) >> 14@		                                          
        str r6, [tmp_ptr, #0xa0+16] @ //Real[i+40] = (d1) >> 14@	                                        
        	                                                                                                
		ldr r1, [tmp_ptr, #0x40+16] @//point1_real = Real[i+16]                                               
		ldr r2, [tmp_ptr, #0xc0+16] @//point1_imag = Real[i+48]                                               
		ldr r3, [tmp_ptr, #0x60+16] @//point2_real = Real[i+24]                                               
		ldr r4, [tmp_ptr, #0xe0+16] @//point2_imag = Real[i+56] 		                                          
                                                                                                          
		sub r6, r3, r1@ //point1_real -= point2_real@                                                         
		sub r7, r2, r4@ //point1_imag -= point2_imag@                                                         
		                                                                                                      
		add r1, r1, r3                                                                                        
		str r1, [tmp_ptr, #0x40+16]@ //Real[i+16] += point2_real@                                             
		add r2, r2, r4                                                                                        
		str r2, [tmp_ptr, #0xc0+16]@ //Real[i+48] += point2_imag@			                                        
		str r7, [tmp_ptr, #0x60+16] @ //Real[i+24] = (d0) >> 14@                                              
        str r6, [tmp_ptr, #0xe0+16] @ //Real[i+56] = (d1) >> 14@                                          
                                                                                                          
        @@/////6                                                                                          
        ldr r1, [tmp_ptr, #0x0+20]  @//point1_real = Real[i]                                              
		ldr r2, [tmp_ptr, #0x80+20] @//point1_imag = Real[i+32]                                               
		ldr r3, [tmp_ptr, #0x20+20] @//point2_real = Real[i+8]                                                
		ldr r4, [tmp_ptr, #0xa0+20] @//point2_imag = Real[i+40] 		                                          
                                                                                                          
		sub r6, r1, r3@ //point1_real -= point2_real@                                                         
		sub r7, r2, r4@ //point1_imag -= point2_imag@	                                                        
                                                                                                          
		add r1, r1, r3                                                                                        
		str r1, [tmp_ptr, #0x0+20]  @ //Real[i]    += point2_real@                                            
		add r2, r2, r4                                                                                        
		str r2, [tmp_ptr, #0x80+20]	@ //Real[i+32] += point2_imag@		                                        
		ldr r14,   [tab], #4                                                                                  
		                                                                                                      
		add  r4,    r6,    r7,   asr  #1                                                                      
		sub  r4,    r4,    r6,   asr  #1   @@// point1_real + (point1_imag>>1) - (point1_real>>1)  d1 -->> d1 
		sub  r2,    r4,    r6               @@// (point1_imag>>1) - (point1_real>>1)   --->> d0               
		                                                                                                      
		SMLAWT  r4,  r6,   r14,   r4                                                                          
		SMLAWB  r2,  r6,   r14,   r2                                                                          
		rsb     r4,  r4,  #0                                                                                  
		SMLAWB  r4,  r7,   r14,   r4                                                                          
		SMLAWT  r2,  r7,   r14,   r2                                                                          
		                                                                                                      
		str r4, [tmp_ptr, #0xa0+20] @ //Real[i+40] = (d1) >> 14@		                                          
		str r2, [tmp_ptr, #0x20+20] @ //Real[i+8]  = (d0) >> 14@			                                        
                                                                                                          
                                                                                                          
		ldr r1, [tmp_ptr, #0x40+20] @//point1_real = Real[i+16]                                               
		ldr r2, [tmp_ptr, #0xc0+20] @//point1_imag = Real[i+48]                                               
		ldr r3, [tmp_ptr, #0x60+20] @//point2_real = Real[i+24]                                               
		ldr r4, [tmp_ptr, #0xe0+20] @//point2_imag = Real[i+56] 		                                          
                                                                                                          
		sub r6, r1, r3@ //point1_real -= point2_real@                                                         
		sub r7, r2, r4@ //point1_imag -= point2_imag@	                                                        
                                                                                                          
		add r1, r1, r3                                                                                        
		str r1, [tmp_ptr, #0x40+20]@ //Real[i+16] += point2_real@                                             
		add r2, r2, r4                                                                                        
		str r2, [tmp_ptr, #0xc0+20]@ //Real[i+48] += point2_imag@                                             
		                                                                                                      
		add  r4,    r6,    r7,   asr  #1                                                                      
		sub  r4,    r4,    r6,   asr  #1    @@// point1_real + (point1_imag>>1) - (point1_real>>1)  d1 -->> d1
		sub  r2,    r4,    r6               @@// (point1_imag>>1) - (point1_real>>1)   --->> d0               
		                                                                                                      
		SMLAWT  r4,  r6,   r14,   r4                                                                          
		SMLAWB  r2,  r6,   r14,   r2                                                                          
		rsb     r4,  r4,  #0                                                                                  
		SMLAWB  r4,  r7,   r14,   r4                                                                          
		SMLAWT  r2,  r7,   r14,   r2                                                                          
		str     r4, [tmp_ptr, #0xe0+20] @ //Real[i+56] = (d1) >> 14@                                          
		str     r2, [tmp_ptr, #0x60+20] @ //Real[i+24] = (d0) >> 14@                                          
                                                                                                          
        @@/////7                                                                                          
		ldr r1, [tmp_ptr, #0x0+24]  @//point1_real = Real[i]                                                  
		ldr r2, [tmp_ptr, #0x80+24] @//point1_imag = Real[i+32]                                               
		ldr r3, [tmp_ptr, #0x20+24] @//point2_real = Real[i+8]                                                
		ldr r4, [tmp_ptr, #0xa0+24] @//point2_imag = Real[i+40] 		                                          
                                                                                                          
		sub r6, r1, r3@ //point1_real -= point2_real@                                                         
		sub r7, r2, r4@ //point1_imag -= point2_imag@	                                                        
                                                                                                          
		add r1, r1, r3                                                                                        
		str r1, [tmp_ptr, #0x0+24]  @ //Real[i]    += point2_real@                                            
		add r2, r2, r4                                                                                        
		str r2, [tmp_ptr, #0x80+24]	@ //Real[i+32] += point2_imag@		                                        
		ldr r14,   [tab], #4                                                                                  
		                                                                                                      
		add  r4,    r6,    r7,   asr  #1                                                                      
		sub  r4,    r4,    r6,   asr  #1   @@// point1_real + (point1_imag>>1) - (point1_real>>1)  d1 -->> d1 
		sub  r2,    r4,    r6               @@// (point1_imag>>1) - (point1_real>>1)   --->> d0               
		                                                                                                      
		SMLAWT  r4,  r6,   r14,   r4                                                                          
		SMLAWB  r2,  r6,   r14,   r2                                                                          
		rsb     r4,  r4,  #0                                                                                  
		SMLAWB  r4,  r7,   r14,   r4                                                                          
		SMLAWT  r2,  r7,   r14,   r2                                                                          
		                                                                                                      
		str r4, [tmp_ptr, #0xa0+24] @ //Real[i+40] = (d1) >> 14@		                                          
		str r2, [tmp_ptr, #0x20+24] @ //Real[i+8]  = (d0) >> 14@			                                        
                                                                                                          
                                                                                                          
		ldr r1, [tmp_ptr, #0x40+24] @//point1_real = Real[i+16]                                               
		ldr r2, [tmp_ptr, #0xc0+24] @//point1_imag = Real[i+48]                                               
		ldr r3, [tmp_ptr, #0x60+24] @//point2_real = Real[i+24]                                               
		ldr r4, [tmp_ptr, #0xe0+24] @//point2_imag = Real[i+56] 		                                          
                                                                                                          
		sub r6, r1, r3@ //point1_real -= point2_real@                                                         
		sub r7, r2, r4@ //point1_imag -= point2_imag@	                                                        
                                                                                                          
		add r1, r1, r3                                                                                        
		str r1, [tmp_ptr, #0x40+24]@ //Real[i+16] += point2_real@                                             
		add r2, r2, r4                                                                                        
		str r2, [tmp_ptr, #0xc0+24]@ //Real[i+48] += point2_imag@                                             
		                                                                                                      
		add  r4,    r6,    r7,   asr  #1                                                                      
		sub  r4,    r4,    r6,   asr  #1    @@// point1_real + (point1_imag>>1) - (point1_real>>1)  d1 -->> d1
		sub  r2,    r4,    r6               @@// (point1_imag>>1) - (point1_real>>1)   --->> d0               
		                                                                                                      
		SMLAWT  r4,  r6,   r14,   r4                                                                          
		SMLAWB  r2,  r6,   r14,   r2                                                                          
		rsb     r4,  r4,  #0                                                                                  
		SMLAWB  r4,  r7,   r14,   r4                                                                          
		SMLAWT  r2,  r7,   r14,   r2                                                                          
		str     r4, [tmp_ptr, #0xe0+24] @ //Real[i+56] = (d1) >> 14@                                          
		str     r2, [tmp_ptr, #0x60+24] @ //Real[i+24] = (d0) >> 14@                                          
		                                                                                                      
        @@/////8                                                                                          
		ldr r1, [tmp_ptr, #0x0+28]  @//point1_real = Real[i]                                                  
		ldr r2, [tmp_ptr, #0x80+28] @//point1_imag = Real[i+32]                                               
		ldr r3, [tmp_ptr, #0x20+28] @//point2_real = Real[i+8]                                                
		ldr r4, [tmp_ptr, #0xa0+28] @//point2_imag = Real[i+40] 		                                          
                                                                                                          
		sub r6, r1, r3@ //point1_real -= point2_real@                                                         
		sub r7, r2, r4@ //point1_imag -= point2_imag@	                                                        
                                                                                                          
		add r1, r1, r3                                                                                        
		str r1, [tmp_ptr, #0x0+28]  @ //Real[i]    += point2_real@                                            
		add r2, r2, r4                                                                                        
		str r2, [tmp_ptr, #0x80+28]	@ //Real[i+32] += point2_imag@		                                        
		ldr r14,   [tab], #4                                                                                  
		                                                                                                      
		add  r4,    r6,    r7,   asr  #1                                                                      
		sub  r4,    r4,    r6,   asr  #1   @@// point1_real + (point1_imag>>1) - (point1_real>>1)  d1 -->> d1 
		sub  r2,    r4,    r6               @@// (point1_imag>>1) - (point1_real>>1)   --->> d0               
		                                                                                                      
		SMLAWT  r4,  r6,   r14,   r4                                                                          
		SMLAWB  r2,  r6,   r14,   r2                                                                          
		rsb     r4,  r4,  #0                                                                                  
		SMLAWB  r4,  r7,   r14,   r4                                                                          
		SMLAWT  r2,  r7,   r14,   r2                                                                          
		                                                                                                      
		str r4, [tmp_ptr, #0xa0+28] @ //Real[i+40] = (d1) >> 14@		                                          
		str r2, [tmp_ptr, #0x20+28] @ //Real[i+8]  = (d0) >> 14@			                                        
                                                                                                          
                                                                                                          
		ldr r1, [tmp_ptr, #0x40+28] @//point1_real = Real[i+16]                                               
		ldr r2, [tmp_ptr, #0xc0+28] @//point1_imag = Real[i+48]                                               
		ldr r3, [tmp_ptr, #0x60+28] @//point2_real = Real[i+24]                                               
		ldr r4, [tmp_ptr, #0xe0+28] @//point2_imag = Real[i+56] 		                                          
                                                                                                          
		sub r6, r1, r3@ //point1_real -= point2_real@                                                         
		sub r7, r2, r4@ //point1_imag -= point2_imag@	                                                        
                                                                                                          
		add r1, r1, r3                                                                                        
		str r1, [tmp_ptr, #0x40+28]@ //Real[i+16] += point2_real@                                             
		add r2, r2, r4                                                                                        
		str r2, [tmp_ptr, #0xc0+28]@ //Real[i+48] += point2_imag@                                             
		                                                                                                      
		add  r4,    r6,    r7,   asr  #1                                                                      
		sub  r4,    r4,    r6,   asr  #1    @@// point1_real + (point1_imag>>1) - (point1_real>>1)  d1 -->> d1
		sub  r2,    r4,    r6               @@// (point1_imag>>1) - (point1_real>>1)   --->> d0               
		                                                                                                      
		SMLAWT  r4,  r6,   r14,   r4                                                                          
		SMLAWB  r2,  r6,   r14,   r2                                                                          
		rsb     r4,  r4,  #0                                                                                  
		SMLAWB  r4,  r7,   r14,   r4                                                                          
		SMLAWT  r2,  r7,   r14,   r2                                                                          
		str     r4, [tmp_ptr, #0xe0+28] @ //Real[i+56] = (d1) >> 14@                                          
		str     r2, [tmp_ptr, #0x60+28] @ //Real[i+24] = (d0) >> 14@                                          
		                                                                                                      
		                                                                                                      
		                                                                                                      
		                                                                                                      
		                                                                                                      
		                                                                                                      
		                                                                                                      
		                                                                                                      
		                                                                                                      
		                                                                                                      
		                                                                                                      
		                                                                                                      
		                                                                                                      
		@ // step 3                                                                                           
		ldr r14,   =-19195                                                                                    
		                                                                                                      
		@@/* 1 */	                                                                                            
		ldr r1, [tmp_ptr, #0x0 ]  @ //point1_real = Real[i]@                                                  
        ldr r2, [tmp_ptr, #0x80] @ //point1_imag = Real[i+32]@                                            
        ldr r3, [tmp_ptr, #0x10] @ //point2_real = Real[i+4]@                                             
        ldr r4, [tmp_ptr, #0x90] @ //point2_imag = Real[i+36]@                                            
                                                                                                          
        add r5, r1, r3                                                                                    
        str r5, [tmp_ptr, #0x0]@ //Real[i]    += point2_real@                                             
        add r5, r2, r4                                                                                    
        str r5, [tmp_ptr, #0x80]@ //Real[i+32] += point2_imag@                                            
                                                                                                          
        sub r1, r1, r3                                                                                    
        sub r2, r2, r4                                                                                    
        str r1, [tmp_ptr, #0x10]@ //Real[i+4]  = point1_real - point2_real@                               
        str r2, [tmp_ptr, #0x90]@ //Real[i+36] = point1_imag - point2_imag@                               
                                                                                                          
        ldr r1, [tmp_ptr, #0x4] @ //point1_real = Real[i+1]@                                              
        ldr r2, [tmp_ptr, #0x84]@ //point1_imag = Real[i+33]@		                                          
        ldr r3, [tmp_ptr, #0x14]@ //point2_real = Real[i+5]@                                              
        ldr r4, [tmp_ptr, #0x94]@ //point2_imag = Real[i+37]@                                             
                                                                                                          
        add r5, r1, r3                                                                                    
        str r5, [tmp_ptr, #0x4 ] @ //Real[i+1]  += point2_real@                                           
        add r5, r2, r4                                                                                    
        str r5, [tmp_ptr, #0x84]@ //Real[i+33] += point2_imag@	                                          
                                                                                                          
		sub r1, r1, r3@ //point1_real -= point2_real@                                                         
		sub r2, r2, r4@ //point1_imag -= point2_imag@		                                                      
		add r5, r1, r2                                                                                        
		sub r2, r2, r1		                                                                                    
		SMLAWB    r5,   r5,  r14,  r5                                                                         
		SMLAWB    r2,   r2,  r14,  r2				                                                                  
		str r5, [tmp_ptr, #0x14]				                                                                      
		str r2, [tmp_ptr, #0x94]                                                                              
		                                                                                                      
		@@/* 2 */	                                                                                            
		ldr r1, [tmp_ptr, #0x0 +2*4]  @ //point1_real = Real[i]@                                              
        ldr r2, [tmp_ptr, #0x80+2*4] @ //point1_imag = Real[i+32]@                                        
        ldr r3, [tmp_ptr, #0x10+2*4] @ //point2_real = Real[i+4]@                                         
        ldr r4, [tmp_ptr, #0x90+2*4] @ //point2_imag = Real[i+36]@                                        
                                                                                                          
        add r5, r1, r3                                                                                    
        str r5, [tmp_ptr, #0x0+2*4]@ //Real[i]    += point2_real@                                         
        add r5, r2, r4                                                                                    
        str r5, [tmp_ptr, #0x80+2*4]@ //Real[i+32] += point2_imag@                                        
                                                                                                          
        sub r1, r3, r1                                                                                    
        sub r2, r2, r4                                                                                    
        str r2, [tmp_ptr, #0x10+2*4]@ //Real[i+4]  = point1_real - point2_real@                           
        str r1, [tmp_ptr, #0x90+2*4]@ //Real[i+36] = point1_imag - point2_imag@                           
                                                                                                          
        ldr r1, [tmp_ptr, #0x4+2*4] @ //point1_real = Real[i+1]@                                          
        ldr r2, [tmp_ptr, #0x84+2*4]@ //point1_imag = Real[i+33]@		                                      
        ldr r3, [tmp_ptr, #0x14+2*4]@ //point2_real = Real[i+5]@                                          
        ldr r4, [tmp_ptr, #0x94+2*4]@ //point2_imag = Real[i+37]@                                         
                                                                                                          
        add r5, r1, r3                                                                                    
        str r5, [tmp_ptr, #0x4 +2*4] @ //Real[i+1]  += point2_real@                                       
        add r5, r2, r4                                                                                    
        str r5, [tmp_ptr, #0x84+2*4]@ //Real[i+33] += point2_imag@	                                      
                                                                                                          
		sub r1, r1, r3@ //point1_real -= point2_real@                                                         
		sub r2, r2, r4@ //point1_imag -= point2_imag@		                                                      
		add r5, r1, r2                                                                                        
		sub r2, r2, r1                                                                                        
		rsb       r5,   r5,   #0		                                                                          
		SMLAWB    r5,   r5,  r14,  r5                                                                         
		SMLAWB    r2,   r2,  r14,  r2	                                                                        
		str r5, [tmp_ptr, #0x94+2*4]			                                                                    
		str r2, [tmp_ptr, #0x14+2*4]				                                                                  
		                                                                                                      
		                                                                                                      
		                                                                                                      
		                                                                                                      
		                                                                                                      
		                                                                                                      
		                                                                                                      
		                                                                                                      
		                                                                                                      
		@@/* 3 */	                                                                                            
		ldr r1, [tmp_ptr, #0x0 +8*4]  @ //point1_real = Real[i]@                                              
        ldr r2, [tmp_ptr, #0x80+8*4] @ //point1_imag = Real[i+32]@                                        
        ldr r3, [tmp_ptr, #0x10+8*4] @ //point2_real = Real[i+4]@                                         
        ldr r4, [tmp_ptr, #0x90+8*4] @ //point2_imag = Real[i+36]@                                        
                                                                                                          
        add r5, r1, r3                                                                                    
        str r5, [tmp_ptr, #0x0+8*4]@ //Real[i]    += point2_real@                                         
        add r5, r2, r4                                                                                    
        str r5, [tmp_ptr, #0x80+8*4]@ //Real[i+32] += point2_imag@                                        
                                                                                                          
        sub r1, r1, r3                                                                                    
        sub r2, r2, r4                                                                                    
        str r1, [tmp_ptr, #0x10+8*4]@ //Real[i+4]  = point1_real - point2_real@                           
        str r2, [tmp_ptr, #0x90+8*4]@ //Real[i+36] = point1_imag - point2_imag@                           
                                                                                                          
        ldr r1, [tmp_ptr, #0x4+8*4] @ //point1_real = Real[i+1]@                                          
        ldr r2, [tmp_ptr, #0x84+8*4]@ //point1_imag = Real[i+33]@		                                      
        ldr r3, [tmp_ptr, #0x14+8*4]@ //point2_real = Real[i+5]@                                          
        ldr r4, [tmp_ptr, #0x94+8*4]@ //point2_imag = Real[i+37]@                                         
                                                                                                          
        add r5, r1, r3                                                                                    
        str r5, [tmp_ptr, #0x4 +8*4] @ //Real[i+1]  += point2_real@                                       
        add r5, r2, r4                                                                                    
        str r5, [tmp_ptr, #0x84+8*4]@ //Real[i+33] += point2_imag@	                                      
                                                                                                          
		sub r1, r1, r3@ //point1_real -= point2_real@                                                         
		sub r2, r2, r4@ //point1_imag -= point2_imag@		                                                      
		add r5, r1, r2                                                                                        
		sub r2, r2, r1		                                                                                    
		SMLAWB    r5,   r5,  r14,  r5                                                                         
		SMLAWB    r2,   r2,  r14,  r2				                                                                  
		str r5, [tmp_ptr, #0x14+8*4]				                                                                  
		str r2, [tmp_ptr, #0x94+8*4]                                                                          
		                                                                                                      
		@@/* 4 */	                                                                                            
		ldr r1, [tmp_ptr, #0x0 +2*4+8*4]  @ //point1_real = Real[i]@                                          
        ldr r2, [tmp_ptr, #0x80+2*4+8*4] @ //point1_imag = Real[i+32]@                                    
        ldr r3, [tmp_ptr, #0x10+2*4+8*4] @ //point2_real = Real[i+4]@                                     
        ldr r4, [tmp_ptr, #0x90+2*4+8*4] @ //point2_imag = Real[i+36]@                                    
                                                                                                          
        add r5, r1, r3                                                                                    
        str r5, [tmp_ptr, #0x0+2*4+8*4]@ //Real[i]    += point2_real@                                     
        add r5, r2, r4                                                                                    
        str r5, [tmp_ptr, #0x80+2*4+8*4]@ //Real[i+32] += point2_imag@                                    
                                                                                                          
        sub r1, r3, r1                                                                                    
        sub r2, r2, r4                                                                                    
        str r2, [tmp_ptr, #0x10+2*4+8*4]@ //Real[i+4]  = point1_real - point2_real@                       
        str r1, [tmp_ptr, #0x90+2*4+8*4]@ //Real[i+36] = point1_imag - point2_imag@                       
                                                                                                          
        ldr r1, [tmp_ptr, #0x4+2*4+8*4] @ //point1_real = Real[i+1]@                                      
        ldr r2, [tmp_ptr, #0x84+2*4+8*4]@ //point1_imag = Real[i+33]@		                                  
        ldr r3, [tmp_ptr, #0x14+2*4+8*4]@ //point2_real = Real[i+5]@                                      
        ldr r4, [tmp_ptr, #0x94+2*4+8*4]@ //point2_imag = Real[i+37]@                                     
                                                                                                          
        add r5, r1, r3                                                                                    
        str r5, [tmp_ptr, #0x4 +2*4+8*4] @ //Real[i+1]  += point2_real@                                   
        add r5, r2, r4                                                                                    
        str r5, [tmp_ptr, #0x84+2*4+8*4]@ //Real[i+33] += point2_imag@	                                  
                                                                                                          
		sub r1, r1, r3@ //point1_real -= point2_real@                                                         
		sub r2, r2, r4@ //point1_imag -= point2_imag@		                                                      
		add r5, r1, r2                                                                                        
		sub r2, r2, r1                                                                                        
		rsb       r5,   r5,   #0		                                                                          
		SMLAWB    r5,   r5,  r14,  r5                                                                         
		SMLAWB    r2,   r2,  r14,  r2	                                                                        
		str r5, [tmp_ptr, #0x94+2*4+8*4]			                                                                
		str r2, [tmp_ptr, #0x14+2*4+8*4]				                                                              
		                                                                                                      
		                                                                                                      
		                                                                                                      
		                                                                                                      
		                                                                                                      
		                                                                                                      
		                                                                                                      
		                                                                                                      
		@@/* 5 */	                                                                                            
		ldr r1, [tmp_ptr, #0x0 +16*4]  @ //point1_real = Real[i]@                                             
        ldr r2, [tmp_ptr, #0x80+16*4] @ //point1_imag = Real[i+32]@                                       
        ldr r3, [tmp_ptr, #0x10+16*4] @ //point2_real = Real[i+4]@                                        
        ldr r4, [tmp_ptr, #0x90+16*4] @ //point2_imag = Real[i+36]@                                       
                                                                                                          
        add r5, r1, r3                                                                                    
        str r5, [tmp_ptr, #0x0+16*4]@ //Real[i]    += point2_real@                                        
        add r5, r2, r4                                                                                    
        str r5, [tmp_ptr, #0x80+16*4]@ //Real[i+32] += point2_imag@                                       
                                                                                                          
        sub r1, r1, r3                                                                                    
        sub r2, r2, r4                                                                                    
        str r1, [tmp_ptr, #0x10+16*4]@ //Real[i+4]  = point1_real - point2_real@                          
        str r2, [tmp_ptr, #0x90+16*4]@ //Real[i+36] = point1_imag - point2_imag@                          
                                                                                                          
        ldr r1, [tmp_ptr, #0x4+16*4] @ //point1_real = Real[i+1]@                                         
        ldr r2, [tmp_ptr, #0x84+16*4]@ //point1_imag = Real[i+33]@		                                    
        ldr r3, [tmp_ptr, #0x14+16*4]@ //point2_real = Real[i+5]@                                         
        ldr r4, [tmp_ptr, #0x94+16*4]@ //point2_imag = Real[i+37]@                                        
                                                                                                          
        add r5, r1, r3                                                                                    
        str r5, [tmp_ptr, #0x4 +16*4] @ //Real[i+1]  += point2_real@                                      
        add r5, r2, r4                                                                                    
        str r5, [tmp_ptr, #0x84+16*4]@ //Real[i+33] += point2_imag@	                                      
                                                                                                          
		sub r1, r1, r3@ //point1_real -= point2_real@                                                         
		sub r2, r2, r4@ //point1_imag -= point2_imag@		                                                      
		add r5, r1, r2                                                                                        
		sub r2, r2, r1		                                                                                    
		SMLAWB    r5,   r5,  r14,  r5                                                                         
		SMLAWB    r2,   r2,  r14,  r2				                                                                  
		str r5, [tmp_ptr, #0x14+16*4]				                                                                  
		str r2, [tmp_ptr, #0x94+16*4]                                                                         
		                                                                                                      
		@@/* 6 */	                                                                                            
		ldr r1, [tmp_ptr, #0x0 +2*4+16*4]  @ //point1_real = Real[i]@                                         
        ldr r2, [tmp_ptr, #0x80+2*4+16*4] @ //point1_imag = Real[i+32]@                                   
        ldr r3, [tmp_ptr, #0x10+2*4+16*4] @ //point2_real = Real[i+4]@                                    
        ldr r4, [tmp_ptr, #0x90+2*4+16*4] @ //point2_imag = Real[i+36]@                                   
                                                                                                          
        add r5, r1, r3                                                                                    
        str r5, [tmp_ptr, #0x0+2*4+16*4]@ //Real[i]    += point2_real@                                    
        add r5, r2, r4                                                                                    
        str r5, [tmp_ptr, #0x80+2*4+16*4]@ //Real[i+32] += point2_imag@                                   
                                                                                                          
        sub r1, r3, r1                                                                                    
        sub r2, r2, r4                                                                                    
        str r2, [tmp_ptr, #0x10+2*4+16*4]@ //Real[i+4]  = point1_real - point2_real@                      
        str r1, [tmp_ptr, #0x90+2*4+16*4]@ //Real[i+36] = point1_imag - point2_imag@                      
                                                                                                          
        ldr r1, [tmp_ptr, #0x4+2*4+16*4] @ //point1_real = Real[i+1]@                                     
        ldr r2, [tmp_ptr, #0x84+2*4+16*4]@ //point1_imag = Real[i+33]@		                                
        ldr r3, [tmp_ptr, #0x14+2*4+16*4]@ //point2_real = Real[i+5]@                                     
        ldr r4, [tmp_ptr, #0x94+2*4+16*4]@ //point2_imag = Real[i+37]@                                    
                                                                                                          
        add r5, r1, r3                                                                                    
        str r5, [tmp_ptr, #0x4 +2*4+16*4] @ //Real[i+1]  += point2_real@                                  
        add r5, r2, r4                                                                                    
        str r5, [tmp_ptr, #0x84+2*4+16*4]@ //Real[i+33] += point2_imag@	                                  
                                                                                                          
		sub r1, r1, r3@ //point1_real -= point2_real@                                                         
		sub r2, r2, r4@ //point1_imag -= point2_imag@		                                                      
		add r5, r1, r2                                                                                        
		sub r2, r2, r1                                                                                        
		rsb       r5,   r5,   #0		                                                                          
		SMLAWB    r5,   r5,  r14,  r5                                                                         
		SMLAWB    r2,   r2,  r14,  r2		                                                                      
		str r5, [tmp_ptr, #0x94+2*4+16*4]		                                                                  
		str r2, [tmp_ptr, #0x14+2*4+16*4]				                                                              
		                                                                                                      
		                                                                                                      
		                                                                                                      
                                                                                                          
                                                                                                          
                                                                                                          
                                                                                                          
                                                                                                          
                                                                                                          
        @@/* 7 */	                                                                                        
		ldr r1, [tmp_ptr, #0x0 +24*4]  @ //point1_real = Real[i]@                                             
        ldr r2, [tmp_ptr, #0x80+24*4] @ //point1_imag = Real[i+32]@                                       
        ldr r3, [tmp_ptr, #0x10+24*4] @ //point2_real = Real[i+4]@                                        
        ldr r4, [tmp_ptr, #0x90+24*4] @ //point2_imag = Real[i+36]@                                       
                                                                                                          
        add r5, r1, r3                                                                                    
        str r5, [tmp_ptr, #0x0+24*4]@ //Real[i]    += point2_real@                                        
        add r5, r2, r4                                                                                    
        str r5, [tmp_ptr, #0x80+24*4]@ //Real[i+32] += point2_imag@                                       
                                                                                                          
        sub r1, r1, r3                                                                                    
        sub r2, r2, r4                                                                                    
        str r1, [tmp_ptr, #0x10+24*4]@ //Real[i+4]  = point1_real - point2_real@                          
        str r2, [tmp_ptr, #0x90+24*4]@ //Real[i+36] = point1_imag - point2_imag@                          
                                                                                                          
        ldr r1, [tmp_ptr, #0x4+24*4] @ //point1_real = Real[i+1]@                                         
        ldr r2, [tmp_ptr, #0x84+24*4]@ //point1_imag = Real[i+33]@		                                    
        ldr r3, [tmp_ptr, #0x14+24*4]@ //point2_real = Real[i+5]@                                         
        ldr r4, [tmp_ptr, #0x94+24*4]@ //point2_imag = Real[i+37]@                                        
                                                                                                          
        add r5, r1, r3                                                                                    
        str r5, [tmp_ptr, #0x4 +24*4] @ //Real[i+1]  += point2_real@                                      
        add r5, r2, r4                                                                                    
        str r5, [tmp_ptr, #0x84+24*4]@ //Real[i+33] += point2_imag@	                                      
                                                                                                          
		sub r1, r1, r3@ //point1_real -= point2_real@                                                         
		sub r2, r2, r4@ //point1_imag -= point2_imag@		                                                      
		add r5, r1, r2                                                                                        
		sub r2, r2, r1		                                                                                    
		SMLAWB    r5,   r5,  r14,  r5                                                                         
		SMLAWB    r2,   r2,  r14,  r2				                                                                  
		str r5, [tmp_ptr, #0x14+24*4]				                                                                  
		str r2, [tmp_ptr, #0x94+24*4]                                                                         
		                                                                                                      
		@@/* 8 */	                                                                                            
		ldr r1, [tmp_ptr, #0x0 +2*4+24*4]  @ //point1_real = Real[i]@                                         
        ldr r2, [tmp_ptr, #0x80+2*4+24*4] @ //point1_imag = Real[i+32]@                                   
        ldr r3, [tmp_ptr, #0x10+2*4+24*4] @ //point2_real = Real[i+4]@                                    
        ldr r4, [tmp_ptr, #0x90+2*4+24*4] @ //point2_imag = Real[i+36]@                                   
                                                                                                          
        add r5, r1, r3                                                                                    
        str r5, [tmp_ptr, #0x0+2*4+24*4]@ //Real[i]    += point2_real@                                    
        add r5, r2, r4                                                                                    
        str r5, [tmp_ptr, #0x80+2*4+24*4]@ //Real[i+32] += point2_imag@                                   
                                                                                                          
        sub r1, r3, r1                                                                                    
        sub r2, r2, r4                                                                                    
        str r2, [tmp_ptr, #0x10+2*4+24*4]@ //Real[i+4]  = point1_real - point2_real@                      
        str r1, [tmp_ptr, #0x90+2*4+24*4]@ //Real[i+36] = point1_imag - point2_imag@                      
                                                                                                          
        ldr r1, [tmp_ptr, #0x4+2*4+24*4] @ //point1_real = Real[i+1]@                                     
        ldr r2, [tmp_ptr, #0x84+2*4+24*4]@ //point1_imag = Real[i+33]@		                                
        ldr r3, [tmp_ptr, #0x14+2*4+24*4]@ //point2_real = Real[i+5]@                                     
        ldr r4, [tmp_ptr, #0x94+2*4+24*4]@ //point2_imag = Real[i+37]@                                    
                                                                                                          
        add r5, r1, r3                                                                                    
        str r5, [tmp_ptr, #0x4 +2*4+24*4] @ //Real[i+1]  += point2_real@                                  
        add r5, r2, r4                                                                                    
        str r5, [tmp_ptr, #0x84+2*4+24*4]@ //Real[i+33] += point2_imag@	                                  
                                                                                                          
		sub r1, r1, r3@ //point1_real -= point2_real@                                                         
		sub r2, r2, r4@ //point1_imag -= point2_imag@                                                         
				                                                                                                  
		add r5, r1, r2                                                                                        
		sub r2, r2, r1                                                                                        
		rsb       r5,   r5,   #0		                                                                          
		SMLAWB    r5,   r5,  r14,  r5                                                                         
		SMLAWB    r2,   r2,  r14,  r2	                                                                        
		str r5, [tmp_ptr, #0x94+2*4+24*4]			                                                                
		str r2, [tmp_ptr, #0x14+2*4+24*4]				                                                              
		                                                                                                      
		                                                                                                      
		                                                                                                      
		                                                                                                      
                                                                                                          
                                                                                                          
                                                                                                          
                                                                                                          
                                                                                                          
                                                                                                          
        @ // step 4                                                                                       
		add img_ptr, rel_ptr, #128                                                                            
                                                                                                          
        @@@// 1                                                                                           
		ldmia  rel_ptr, {r1-r4}                                                                               
		add    r5, r1, r3  @// r0                                                                             
		sub    r6, r1, r3  @// r2                                                                             
		                                                                                                      
		add    r7, r2, r4  @// r1                                                                             
		sub    r14, r4, r2  @// r7                                                                             
		                                                                                                      
		add    r1, r5, r7                                                                                     
		sub    r2, r5, r7                                                                                     
		stmia  rel_ptr!, {r1-r2}		                                                                          
		ldmia img_ptr, {r1-r4}                                                                                
		                                                                                                      
		add   r5, r1, r3 @ //r0 = d0 + d1@  // i+32                                                           
	    sub   r7, r1, r3 @ //r6 = d0 - d1@  // i+34		                                                      
        add   r1, r2, r4 @ //r1 = d2 + d3@  // i+33		                                                    
        sub   r2, r2, r4 @ //r3 = d2 - d3@  // i+3                                                        
                                                                                                          
        add   r3, r6, r2                                                                                  
        sub   r4, r6, r2                                                                                  
        stmia rel_ptr!, {r3-r4}                                                                           
		                                                                                                      
		add   r3, r5, r1                                                                                      
		sub   r4, r5, r1                                                                                      
		add   r5, r7, r14                                                                                      
		sub   r6, r7, r14		                                                                                  
		stmia img_ptr!, {r3-r6}                                                                               
                                                                                                          
        @@@// 2                                                                                           
		ldmia  rel_ptr, {r1-r4}                                                                               
		add    r5, r1, r3  @// r0                                                                             
		sub    r6, r1, r3  @// r2                                                                             
		                                                                                                      
		add    r7, r2, r4  @// r1                                                                             
		sub    r14, r4, r2  @// r7                                                                             
		                                                                                                      
		add    r1, r5, r7                                                                                     
		sub    r2, r5, r7                                                                                     
		stmia  rel_ptr!, {r1-r2}		                                                                          
		ldmia img_ptr, {r1-r4}                                                                                
		                                                                                                      
		add   r5, r1, r3 @ //r0 = d0 + d1@  // i+32                                                           
	    sub   r7, r1, r3 @ //r6 = d0 - d1@  // i+34		                                                      
        add   r1, r2, r4 @ //r1 = d2 + d3@  // i+33		                                                    
        sub   r2, r2, r4 @ //r3 = d2 - d3@  // i+3                                                        
                                                                                                          
        add   r3, r6, r2                                                                                  
        sub   r4, r6, r2                                                                                  
        stmia rel_ptr!, {r3-r4}                                                                           
		                                                                                                      
		add   r3, r5, r1                                                                                      
		sub   r4, r5, r1                                                                                      
		add   r5, r7, r14                                                                                      
		sub   r6, r7, r14		                                                                                  
		stmia img_ptr!, {r3-r6}                                                                               
                                                                                                          
        @@@// 3                                                                                           
		ldmia  rel_ptr, {r1-r4}                                                                               
		add    r5, r1, r3  @// r0                                                                             
		sub    r6, r1, r3  @// r2                                                                             
		                                                                                                      
		add    r7, r2, r4  @// r1                                                                             
		sub    r14, r4, r2  @// r7                                                                             
		                                                                                                      
		add    r1, r5, r7                                                                                     
		sub    r2, r5, r7                                                                                     
		stmia  rel_ptr!, {r1-r2}		                                                                          
		ldmia img_ptr, {r1-r4}                                                                                
		                                                                                                      
		add   r5, r1, r3 @ //r0 = d0 + d1@  // i+32                                                           
	    sub   r7, r1, r3 @ //r6 = d0 - d1@  // i+34		                                                      
        add   r1, r2, r4 @ //r1 = d2 + d3@  // i+33		                                                    
        sub   r2, r2, r4 @ //r3 = d2 - d3@  // i+3                                                        
                                                                                                          
        add   r3, r6, r2                                                                                  
        sub   r4, r6, r2                                                                                  
        stmia rel_ptr!, {r3-r4}                                                                           
		                                                                                                      
		add   r3, r5, r1                                                                                      
		sub   r4, r5, r1                                                                                      
		add   r5, r7, r14                                                                                      
		sub   r6, r7, r14		                                                                                  
		stmia img_ptr!, {r3-r6}                                                                               
                                                                                                          
        @@@// 4                                                                                           
		ldmia  rel_ptr, {r1-r4}                                                                               
		add    r5, r1, r3  @// r0                                                                             
		sub    r6, r1, r3  @// r2                                                                             
		                                                                                                      
		add    r7, r2, r4  @// r1                                                                             
		sub    r14, r4, r2  @// r7                                                                             
		                                                                                                      
		add    r1, r5, r7                                                                                     
		sub    r2, r5, r7                                                                                     
		stmia  rel_ptr!, {r1-r2}		                                                                          
		ldmia img_ptr, {r1-r4}                                                                                
		                                                                                                      
		add   r5, r1, r3 @ //r0 = d0 + d1@  // i+32                                                           
	    sub   r7, r1, r3 @ //r6 = d0 - d1@  // i+34		                                                      
        add   r1, r2, r4 @ //r1 = d2 + d3@  // i+33		                                                    
        sub   r2, r2, r4 @ //r3 = d2 - d3@  // i+3                                                        
                                                                                                          
        add   r3, r6, r2                                                                                  
        sub   r4, r6, r2                                                                                  
        stmia rel_ptr!, {r3-r4}                                                                           
		                                                                                                      
		add   r3, r5, r1                                                                                      
		sub   r4, r5, r1                                                                                      
		add   r5, r7, r14                                                                                      
		sub   r6, r7, r14		                                                                                  
		stmia img_ptr!, {r3-r6}                                                                               
                                                                                                          
        @@@// 5                                                                                           
		ldmia  rel_ptr, {r1-r4}                                                                               
		add    r5, r1, r3  @// r0                                                                             
		sub    r6, r1, r3  @// r2                                                                             
		                                                                                                      
		add    r7, r2, r4  @// r1                                                                             
		sub    r14, r4, r2  @// r7                                                                             
		                                                                                                      
		add    r1, r5, r7                                                                                     
		sub    r2, r5, r7                                                                                     
		stmia  rel_ptr!, {r1-r2}		                                                                          
		ldmia img_ptr, {r1-r4}                                                                                
		                                                                                                      
		add   r5, r1, r3 @ //r0 = d0 + d1@  // i+32                                                           
	    sub   r7, r1, r3 @ //r6 = d0 - d1@  // i+34		                                                      
        add   r1, r2, r4 @ //r1 = d2 + d3@  // i+33		                                                    
        sub   r2, r2, r4 @ //r3 = d2 - d3@  // i+3                                                        
                                                                                                          
        add   r3, r6, r2                                                                                  
        sub   r4, r6, r2                                                                                  
        stmia rel_ptr!, {r3-r4}                                                                           
		                                                                                                      
		add   r3, r5, r1                                                                                      
		sub   r4, r5, r1                                                                                      
		add   r5, r7, r14                                                                                      
		sub   r6, r7, r14		                                                                                  
		stmia img_ptr!, {r3-r6}                                                                               
                                                                                                          
        @@@// 6                                                                                           
		ldmia  rel_ptr, {r1-r4}                                                                               
		add    r5, r1, r3  @// r0                                                                             
		sub    r6, r1, r3  @// r2                                                                             
		                                                                                                      
		add    r7, r2, r4  @// r1                                                                             
		sub    r14, r4, r2  @// r7                                                                             
		                                                                                                      
		add    r1, r5, r7                                                                                     
		sub    r2, r5, r7                                                                                     
		stmia  rel_ptr!, {r1-r2}		                                                                          
		ldmia img_ptr, {r1-r4}                                                                                
		                                                                                                      
		add   r5, r1, r3 @ //r0 = d0 + d1@  // i+32                                                           
	    sub   r7, r1, r3 @ //r6 = d0 - d1@  // i+34		                                                      
        add   r1, r2, r4 @ //r1 = d2 + d3@  // i+33		                                                    
        sub   r2, r2, r4 @ //r3 = d2 - d3@  // i+3                                                        
                                                                                                          
        add   r3, r6, r2                                                                                  
        sub   r4, r6, r2                                                                                  
        stmia rel_ptr!, {r3-r4}                                                                           
		                                                                                                      
		add   r3, r5, r1                                                                                      
		sub   r4, r5, r1                                                                                      
		add   r5, r7, r14                                                                                      
		sub   r6, r7, r14		                                                                                  
		stmia img_ptr!, {r3-r6}                                                                               
                                                                                                          
        @@@// 7                                                                                           
		ldmia  rel_ptr, {r1-r4}                                                                               
		add    r5, r1, r3  @// r0                                                                             
		sub    r6, r1, r3  @// r2                                                                             
		                                                                                                      
		add    r7, r2, r4  @// r1                                                                             
		sub    r14, r4, r2  @// r7                                                                             
		                                                                                                      
		add    r1, r5, r7                                                                                     
		sub    r2, r5, r7                                                                                     
		stmia  rel_ptr!, {r1-r2}		                                                                          
		ldmia img_ptr, {r1-r4}                                                                                
		                                                                                                      
		add   r5, r1, r3 @ //r0 = d0 + d1@  // i+32                                                           
	    sub   r7, r1, r3 @ //r6 = d0 - d1@  // i+34		                                                      
        add   r1, r2, r4 @ //r1 = d2 + d3@  // i+33		                                                    
        sub   r2, r2, r4 @ //r3 = d2 - d3@  // i+3                                                        
                                                                                                          
        add   r3, r6, r2                                                                                  
        sub   r4, r6, r2                                                                                  
        stmia rel_ptr!, {r3-r4}                                                                           
		                                                                                                      
		add   r3, r5, r1                                                                                      
		sub   r4, r5, r1                                                                                      
		add   r5, r7, r14                                                                                      
		sub   r6, r7, r14		                                                                                  
		stmia img_ptr!, {r3-r6}                                                                               
                                                                                                          
        @@@// 8                                                                                           
		ldmia  rel_ptr, {r1-r4}                                                                               
		add    r5, r1, r3  @// r0                                                                             
		sub    r6, r1, r3  @// r2                                                                             
		                                                                                                      
		add    r7, r2, r4  @// r1                                                                             
		sub    r14, r4, r2  @// r7                                                                             
		                                                                                                      
		add    r1, r5, r7                                                                                     
		sub    r2, r5, r7                                                                                     
		stmia  rel_ptr!, {r1-r2}		                                                                          
		ldmia img_ptr, {r1-r4}                                                                                
		                                                                                                      
		add   r5, r1, r3 @ //r0 = d0 + d1@  // i+32                                                           
	    sub   r7, r1, r3 @ //r6 = d0 - d1@  // i+34		                                                      
        add   r1, r2, r4 @ //r1 = d2 + d3@  // i+33		                                                    
        sub   r2, r2, r4 @ //r3 = d2 - d3@  // i+3                                                        
                                                                                                          
        add   r3, r6, r2                                                                                  
        sub   r4, r6, r2                                                                                  
        stmia rel_ptr!, {r3-r4}                                                                           
		                                                                                                      
		add   r3, r5, r1                                                                                      
		sub   r4, r5, r1                                                                                      
		add   r5, r7, r14                                                                                      
		sub   r6, r7, r14		                                                                                  
		stmia img_ptr!, {r3-r6}                                                                               
                                                                                                          
                                                                                                          
		ldmfd	sp!, {r4 - r7, pc}                                                                              
                                                                                                          
	    @AREA SBR_SYNTHESIS_FILTER_TABLE_D, DATA, READONLY                                                   
DCT_IV_TAB:   	                                                                                            
				.word  0x7b154e0f                                                                                   
                .word  0x6c831e08                                                                           
                .word  0x54dbf1c6                                                                           
                .word  0x3505cafb                                                                           
                .word  0x0e3aab25                                                                           
                .word  0xe1f8937d                                                                           
                .word  0xb1f184eb                                                                           
                .word   0x6c831e08                                                                          
                .word   0x3505cafb                                                                          
                .word   0xe1f8937d                                                                          
                                                                                                          
                                                                                                          
                                                                                                          
                                                                                                          
		                                                                                                      
		.end@END                                                                                                   
		                                                                                                      