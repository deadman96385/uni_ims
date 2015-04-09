 
        .arm

        .text @MP3_PROM, CODE, READONLY
                  
        
@--------------------------------------------------------------------
@   Function:
@       void MP3_DEC_ImdctWinOverlap(mp3_fixed_t X[18], mp3_fixed_t overlap[32][9],
@							   mp3_fixed_t sample[18][32], int sb_limit)
@
@   Description: Perform frequency line alias reduction.
@
@   Register usage:
@       r0 = X
@       r1 = overlap
@			  r2 = sample
@       r3 = sblimit
@--------------------------------------------------------------------				

overlap .req  r0
sample  .req  r3

MP3_DEC_ImdctWinOverlap:
        STMFD    sp!,{r1, r2,r4-r12,lr}
                
        LDR      r9, c00_1
        MOV      r12, #00
        
START_LOOP:
        CMP      r12, r3
        BGE      OUT_LOOP        
        TST			 r12, #1
        SUB      sp, sp, #60
        STR      r12, [sp, #56]  
        
        LDR			 r1, [r0, #24]   @X[6]
        LDR			 r2, [r0, #20]   @X[5]
        LDR			 r4, [r0, #44]   @X[11]
        LDR			 r5, [r0, #48]   @X[12]
        LDR			 r6, [r0, #68]   @X[17]
        LDR			 r7, [r0], #8   @X[0]        
				LDR      r8, c2a_2b
                
        SUB			 r1, r1, r2          @tt2
        ADD      r2, r1, r2, lsl#1   
        SMLAWB   r2, r2, r9, r2      @tt4
        
        ADD      r4, r4, r5          @tt3
        SUB      r5, r4, r5, lsl#1
        SMLAWB   r5, r5, r9, r5      @tt5
        
        ADD			 r10, r6, r1          @tt6
        SUB      r6, r6, r1, asr#1    
        SUB      r1, r4, r7           @tt7
        ADD      r7, r7, r4, asr#1    
        ADD      r6, r6, r5           @tt0
        SUB      r5, r6, r5, lsl#1    @tt5
        SUB      r7, r2, r7           @tt1
        SUB      r2, r7, r2, lsl#1    @tt4
        
        SMLAWT   r4, r10, r8, r10
        SMLAWB   r10, r10, r8, r1
        SMULWB   r11,  r1, r8        
        SMLAWT   r1, r1, r8, r10     @tt3 - x_tmp[3]     
        SUB      r4, r4, r11         @tt2 - x_tmp[2]
        LDR      r11, c3a_3b   
        STR      r4, [sp, #0]
        STR      r1, [sp, #4]
        
        SMLAWB   r10, r6, r11, r6
        SMLAWT   r6, r6, r11, r2
        SMULWT   r12, r2, r11     
        SMLAWB   r2, r2, r11, r6     @tt6 - x_tmp[8]
        LDR      r11, c4a_4b
        SUB      r6, r12, r10        @tt7 - x_tmp[9]    
        STR      r2, [sp, #16]      
        STR      r6, [sp, #20]
        
        SMLAWT   r10, r5, r11, r7        
        SMULWB   r12, r7, r11
        SMULWB   r14, r5, r11
        SUB      r10, r10, r12, asr#4     @tt6  - x_tmp[14]
        RSB      r5,  r5,  r14, asr#4      
        SMLAWT   r5,  r7, r11, r5         @tt7  - x_tmp[15]        
        STR      r10, [sp, #32]      
        STR      r5, [sp, #36]
        
        
        @ x_tmp[3]- r1  x_tmp[2]-r4  r0, r3
        @ r9-c00_1  r8-c2a_2b  r11-c4a_4b
        LDR	    r2, [r0], #28     @tt2
        LDR     r5, [r0], #24    @tt4
        LDR     r6, [r0], #-28    @tt3
        LDR     r7, [r0], #24    @tt5
        LDR     r1, [r0], #-44    @tt0      
        LDR     r4, [r0], #-8    @tt1  
        
        SUB     r2, r5, r2         @tt2
        SUB     r5, r2, r5, lsl#1
        SMLAWB  r5, r5, r9, r5     @tt4
        
        SUB     r7, r7, r6
        ADD     r6, r7, r6, lsl#1   @tt3
        SMLAWB  r7, r7, r9, r7      @tt5
        
        ADD    r10, r1, r2          @tt6
        SUB    r1, r1, r2, asr#1    @tt0
        SUB    r2, r4, r6           @tt7
        ADD    r4, r4, r6, asr#1    @tt1      
        
        SMLAWT   r12, r10, r8, r10
        SMLAWB   r10, r10, r8, r2
        SMULWB   r14, r2, r8
        SMLAWT   r2, r2, r8, r10        @tt3  - x_tmp[5]
        SUB      r10, r12, r14          @tt2  - x_tmp[4]
         
        @r0, r3, r2, r10
        @tt4-r5  tt5-r7  tt0-r1  tt1- r4
        @ r9-c00_1, r8-c2a_2b, r11-c4a_4b
        ADD   r1, r1, r7          @tt0
        SUB   r7, r1, r7, lsl#1   @tt5
        ADD   r4, r4, r5        @tt1
        SUB   r5, r4, r5, lsl#1  @tt4
        
        SMLAWT   r6, r1, r11, r5 
        SMULWB   r12, r1, r11
        SMULWB   r14, r5, r11
        RSB      r1, r1, r12, asr#4
        SUB      r6, r6, r14, asr#4        @tt2-x_tmp[10]
        SMLAWT   r5, r5, r11, r1           @tt3-x_tmp[11]
        LDR      r11, c6a_6b
        STR      r6, [sp, #24]
        STR      r5, [sp, #28]
        
        @r0, r3, r2, r10
        @tt5-r7   tt1- r4
        @ r9-c00_1, r8-c2a_2b, r11-c4a_4b
        @free: r1, r5, r6, r12, r14
        SMULWT   r1, r7, r11
        SMULWB   r5, r7, r11
        RSB      r7, r7, r1, asr#9
        RSB      r1, r4, r5, asr#3
        SMULWB   r5, r4, r11
        SMULWT   r6, r4, r11
        SUB      r7, r7, r5, asr#3        @tt2 - x_tmp[16]
        ADD      r1, r1, r6, asr#9        @tt3 - x_tmp[17]
        STR      r7, [sp, #40]
        STR      r1, [sp, #44]
        
         @r0, r3, r2, r10        
        @ r9-c00_1, r8-c2a_2b 
        @free: r1, r4, r5, r6, r7, r11 r12, r14
        LDR    r1,  [r0], #36     @X[1]
        LDR    r4,  [r0], #24     @X[10]
        LDR    r5,  [r0], #-36    @X[16]
        LDR    r6,  [r0], #24     @X[7]
        LDR    r7,  [r0], #-36    @X[13]
        LDR    r11, [r0],#56      @X[4]

        
        SUB    r4, r1, r4         @tt2
        SUB    r1, r4, r1, lsl#1  @tt4
        SUB    r5, r5, r6         @tt5
        ADD    r6, r5, r6, lsl#1  @tt3
        
        SUB    r12, r4, r7         @tt6
        ADD    r7, r7, r4, asr#1   @tt0
        SUB    r4, r11, r6         @tt7
        ADD    r11, r11, r6, asr#1 @tt1
        
        SMLAWT  r14, r12, r8, r12
        SMLAWB  r12, r12, r8, r4
        SMULWB  r6,  r4, r8        
        SMLAWT  r4, r4, r8, r12        @x_tmp[1]
        SUB     r6, r14, r6            @x_tmp[0]
        
        STR     r0,[sp, #48]
        STR     r3,[sp, #52]
        
         @r0, r3, r2-x_tmp[5], r10-x_tmp[4]        
        @ r9-c00_1, r8-c2a_2b 
        @ x_tmp[0]-r6  x_tmp[1] -r4
        @tt0 - r7  tt1 - r11  tt4 - r1  rr5-r5
        @free:  r12, r14        
       SMLAWT   r14, r11, r8, r11
       SMLAWB   r11, r11, r8, r7
       SMULWB   r12, r7, r8
       SMLAWT   r7, r7, r8, r11    @tt7
       SUB      r8, r14, r12       @tt6
       STR      r8, [sp, #8]
       STR      r7, [sp, #12]
        
        @r0, r3, r2-x_tmp[5], r10-x_tmp[4]        
        @ r9-c00_1
        @ x_tmp[0]-r6  x_tmp[1] -r4
        @tt6 - r7  tt7 - r8  tt4 - r1  tt5-r5
        @free:  r11, r12, r14 
        LDR     r0, [sp, #60]          @overlap      
                              
        LDR    r11, [sp], #4  @x_tmp[2]
        LDR    r12, [sp], #4          @x_tmp[3]  
        
        @ x_tmp[0]-r6   x_tmp[1] -r4
        @ x_tmp[2]-r11  x_tmp[3] -r12
        @ x_tmp[4]-r10  x_tmp[5] -r2        
        ADD  r11, r11, r10           @tt02
        SUB  r10, r11, r10, lsl#1
        SMLAWB r10, r10, r9, r10      @tt04
        ADD  r12, r12, r2             @tt03
        SUB  r2, r12, r2, lsl#1
        SMLAWB r2, r2, r9, r2         @tt05
        
        @ r7, r8, r14, r0, r3
        LDR     r3, window_l_4_13
        
        ADD     r7, r6, r11       @tt06
        ADD     r8,  r4, r12      @tt07
        
        LDR     r14, [overlap, #16]                  @t0-overlap[4]
        STR     r8,  [overlap, #16]                 
        
        @ r8, 
        SMLAWB  r8, r14, r3, r14
        SMLAWT  r14, r14, r3, r7
        SMLAWB  r14, r7, r3, r14             @sample [   13][sb]
        SMULWT  r7, r7, r3
        RSBNE   r14, r14, #0             @sample [   13][sb]
        LDR     r3, [sp, #56]        @sample        
        SUB     r8, r8, r7                   @sample [   4][sb]
        STR     r8,  [sample, #512]        
        STR     r14, [sample, #1664]
        
        @r0, r3    r7, r8, r14
        @tt04 - r10  tt03-r12  tt02 - r11  tt05 - r2
        @tt00 - r6  tt01 - r4
        ADD			r7, r12, r10, lsl#1   @tt06
        RSB     r12, r12, r10, lsl#1  @tt07
        ADD     r10, r11, r2, lsl#1   @@tt04
        RSB     r2, r11, r2, lsl#1    @tt05
        ADD     r2, r6, r2, asr#1     @tt05
        RSB     r7, r4, r7, asr#1     @tt06
        RSB     r10, r6, r10, asr#1   @tt04
        ADD     r12, r4, r12, asr#1   @tt07
        
        @r0, r3   r4,  r6,  r8, r11, r14
        @tt04 - r10  tt07 - r12
        @tt05 - r2  tt06 - r7
        @tt4 - r1  tt5-r5
        LDR      r4, [overlap, #4]           @t0
        STR      r7, [overlap, #4]           @overlap[1]
        
        LDR       r7, window_l_1_16
        
        LDR      r6, [overlap, #28]          @t0
        STR       r10, [overlap, #28]        @overlap[7]
        
        SMLAWB    r8, r4, r7, r4
        SMLAWT    r4, r4, r7, r2
        SMULWT    r11, r2, r7
        SMLAWB    r4, r2, r7, r4           @sample [16][sb]        
        SUB       r8, r8, r11              @sample [1][sb]
        RSBNE     r8, r8,#0                 @sample [1][sb]        
        LDR       r7, window_l_7_10
        STR       r8, [sample, #128] 
        STR       r4, [sample, #2048]
        
		    SMLAWB    r4, r6, r7, r6
		    SMLAWT    r8, r6, r7, r6
		    SMLAWT    r11, r12, r7, r12
		    SMLAWB    r14, r12, r7, r12
		    ADD       r4, r4, r11
		    SUB       r8, r8, r14             @sample[10][sb]
		    RSBNE     r4, r4, #0              @sample[7][sb]
		      
		    
		    STR       r4, [sample, #896]
		    STR       r8, [sample, #1280]
		    
		    LDR       r8, c1a_1b
		    @r0, r3   
		    @r2, r4, r6, r7, r8, r10, r11, r12, r14
        @tt4 - r1  tt5-r5
        LDR     r6, [sp], #4            @tt6
        LDR     r7, [sp], #4           @tt7
        
        SMLAWT   r2, r1, r8, r1
        SMLAWB   r1, r1, r8, r5
        SMULWB   r4, r5, r8
        SMLAWT   r1, r5, r8, r1    @tt3
        SUB      r2, r2, r4        @tt2
        
        SUB     r7, r1, r7         @tt7
        SUB     r1, r7, r1, lsl#1  @tt3 
        ADD     r6, r6, r2         @tt6
        SUB     r2, r6, r2, lsl#1  @tt2
        
        @x_tmp[6] - r7  x_tmp[7] - r2
        @x_tmp[12] - r1  x_tmp[13] - r6
        @r0, r3   
		    @r4, r5, r8, r10, r11, r12, r14
		    LDR      r4,   [sp], #4            @x_tmp[8] - tt2
		    LDR      r5,   [sp], #4           @x_tmp[9] - tt3
		    LDR      r8,   [sp], #4           @x_tmp[10]- tt4
		    LDR      r10,  [sp], #4            @x_tmp[11]- tt5
		    
		    ADD      r4, r4, r8          @tt2
		    SUB      r8, r4, r8, lsl#1   
		    ADD      r5, r5, r10         @tt3
		    SUB      r10, r5, r10, lsl#1  
		    SMLAWB   r8, r8, r9, r8      @tt6
		    SMLAWB   r10, r10, r9, r10   @tt7
		    
		    ADD     r11, r7, r4      @tt4
		    ADD     r12, r2, r5      @tt5 
		    
		    LDR     r9, window_l_3_14
		    
		    LDR     r14, [overlap, #12]    @t0 - overlap[3]
		    STR     r11, [overlap, #12]    
		    					
        @x_tmp[12] - r1  x_tmp[13] - r6
        @r0-overlap, r3-sample
        @r4, r5, r8, r10, r2, r7
		    @r12 -tt5, r14 - t0
		    @r11	    
		    		    
		    SMLAWB  r11, r14, r9, r14
		    SMLAWT  r14, r14, r9, r12		    
		    SMLAWB  r14, r12, r9, r14          @sample[14][sb]
		    SMULWT  r12, r12, r9
		    STR		  r14, [sample, #1792]  
		    SUB     r11, r11, r12         
		    RSBNE   r11, r11, #0               @sample[3][sb]
		    STR     r11, [sample, #384]
		    
        @x_tmp[12] - r1  x_tmp[13] - r6
        @r0-overlap, r3-sample
        @r4-tt2, r5-tt3, r8-tt6, r10-tt7, r2-tt1, r7-tt0		    
		    @r11, r9, r12, r14	    
		    ADD    r9, r5, r8, lsl#1      @tt4
		    RSB    r5, r5, r8, lsl#1      @tt5
		    ADD    r8, r4, r10, lsl#1     @tt6
		    RSB    r4, r4, r10, lsl#1     @tt7
		    ADD    r4, r7, r4, asr#1      @tt7
		    RSB    r9, r2, r9, asr#1      @tt4
		    RSB    r8, r7, r8, asr#1      @tt6
		    ADD    r5, r2, r5, asr#1      @tt5
		    
		    @x_tmp[12] - r1  x_tmp[13] - r6
        @r0-overlap, r3-sample
        @r8-tt6, r4-tt7, r9-tt4, r5-tt5		    
		    @r2, r7, r10, r11, r12, r14	    
		    LDR    r2, [overlap, #32]      @t0 -overlap[8]
		    STR    r9, [overlap, #32]      
		    
		    LDR    r12, window_l_8_9
		    
		    LDR    r7, [overlap, #8]      @t0 -overlap[2] 
		    STR    r8, [overlap, #8] 
		    
		    @x_tmp[12] - r1  x_tmp[13] - r6
        @r0-overlap, r3-sample
        @r4-tt7,  r5-tt5		r2   r7
		    @r8, r9, r10, r11, r12, r14	   
		    SMLAWB   r8, r2, r12, r2
		    SMLAWT   r2, r2, r12, r2
		    SMLAWT   r9, r4, r12, r4
		    SMLAWB   r4, r4, r12, r4
		    ADD      r8, r8, r9          @sample[8][sb]
		    SUB      r2, r2, r4   
		    RSBNE    r2, r2, #0x0        @sample[9][sb]
		    LDR      r12, window_l_2_15		   
		    STR      r8, [sample, #1024]
		    STR      r2, [sample, #1152]
		    
		    SMLAWB   r8, r7, r12, r7
		    SMLAWT   r7, r7, r12, r5
		    SMULWT   r2, r5, r12
		    SMLAWB   r7, r5, r12, r7      @sample[15][sb]
		    SUB      r8, r8, r2            @sample[2][sb]
		    RSBNE    r7, r7, #0x0
		    STR      r8, [sample, #256]
		    STR      r7, [sample, #1920]
		    		    		    
		    @x_tmp[12] - r1  x_tmp[13] - r6
        @r0-overlap, r3-sample
        @r2, r4, r5, r7, r8, r9, r10, r11, r12, r14	  
        LDR      r4,   [sp], #4               @x_tmp[14] - tt2
        LDR      r5,   [sp], #4               @x_tmp[15] - tt3
        LDR      r7,   [sp], #4               @x_tmp[16] - tt4
        LDR      r8,   [sp], #4               @x_tmp[17] - tt5
        LDR      r9, c00_1
		    
		    ADD      r4, r4, r7        @tt2 
		    SUB      r7, r4, r7, lsl#1  
		    ADD      r5, r5, r8         @tt3
		    SUB      r8, r5, r8, lsl#1 
		    SMLAWB   r7, r7, r9, r7     @tt6
		    SMLAWB   r8, r8, r9, r8     @tt7
		    
		    ADD      r2,   r1, r4
		    ADD      r10,  r6, r5
		    
		    LDR      r12, window_l_6_11
		    
		    LDR      r11, [overlap, #24]        @t0
		    STR      r2,  [overlap, #24]
		    
		    SMLAWB   r2, r11, r12, r11
		    SMLAWT   r11, r11, r12, r11
		    SMLAWT   r14, r10, r12, r10
		    SMLAWB   r10, r10, r12, r10
		    ADD      r2, r2, r14               @sample[6][sb]
		    SUB      r11, r11, r10
		    RSBNE    r11, r11, #0x0            @sample[11][sb]
		    STR      r2,  [sample, #768]
		    STR      r11, [sample, #1408]
		    
		    ADD     r2, r5, r7, lsl#1     @tt4
		    RSB     r5, r5, r7, lsl#1     @tt5
		    ADD     r7, r4, r8, lsl#1     @tt6
		    RSB     r4, r4, r8, lsl#1     @tt7
		    
		    ADD     r4, r1, r4, asr#1     @tt7
		    RSB     r2, r6, r2, asr#1     @tt4
		    SUB     r7, r1, r7, asr#1     @tt6
		    ADD     r5, r6, r5, asr#1     @tt5
		    
		    LDR     r1, [overlap]           @t0-overlap[0]
		    STR     r2, [overlap], #20
		    
		    LDR     r12, window_l_0_17
		    
		    LDR     r6, [overlap]            @t0-overlap[5]
		   	STR     r5,  [overlap], #16
		   	
		   	SMLAWB   r2, r1, r12, r1
		   	SMLAWB   r5, r4, r12, r4
		   	SMULWT   r4, r4, r12
		   	SMULWT   r1, r1, r12
		   	ADD      r2, r2, r4           @sample[0][sb]
		   	SUB      r1, r1, r5  
		   	RSBNE    r1, r1, #0           @sample[17][sb]
		   	LDR      r12, window_l_5_12
		   	STR      r1, [sample, #2176]
		   	STR      r2, [sample], #640
		   	
		   	SMLAWB   r1, r6, r12, r6
		   	SMLAWB   r2, r7, r12, r7
		   	SMULWT   r7, r7, r12
		   	SMULWT   r6, r6, r12
		   	ADD      r1, r1, r7
		   	SUB      r2, r6, r2
		   	RSBNE    r1, r1, #0x1
		   	STR      r1, [sample], #896           @sample[5][sb]
		   	STR      r2, [sample], #-1532           @sample[12][sb]
		   	
		   	STR    r0,  [sp, #12]
		   	STR    r3,  [sp, #16] 
		   	LDR    r0,  [sp], #4
		   	LDR    r3,	[sp], #4	   
        LDR    r12, [sp], #4        
        ADD    r12, r12, #1
        B			 START_LOOP
                
OUT_LOOP:  
				ADD			 sp, sp, #0x8
        LDMFD    sp!,{r4-r12,pc}
        @ENDP        
        
@--------------------------------------------------------------------
@   Function:
@       void MP3_DEC_AliasImdctWinOverlap(mp3_fixed_t X[18], mp3_fixed_t overlap[32][9],
@							   mp3_fixed_t sample[18][32], unsigned int sb_limit)
@
@   Description: Perform frequency line alias reduction.
@
@   Register usage:
@       r0 = X
@       r1 = overlap
@			  r2 = sample
@       r3 = sblimit
@--------------------------------------------------------------------				
        
x        .req r0
overlap  .req r0
sample   .req r3

                
MP3_DEC_AliasImdctWinOverlap:
        STMFD    sp!,{r1, r4-r12,lr}
        
        ADD      r2, r2, r3, lsl#2   @sample
        STR      r2, [sp, #-4]!
        
        LDR      r12, c00_1       
        CMP      r3,#0
        BLE      LOOP_END
LOOP_START:
@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
@@@@*******************************@@@@@
@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
@@@ r0 - x, r3 - sb 
				LDR      r1, [x, #4] 
				LDR      r2, [x, #-8]
				LDR      r4, [x, #40]
				ORRS     r5, r1, r2
				LDRNE      r5,cs_ca_1
				BEQ      ZERO_X1				
				SMLAWT   r6, r2, r5, r2
				SMLAWT   r7, r1, r5, r1
				SMLAWB   r6, r1, r5, r6
				SMULWB   r1, r2, r5
				STR      r6, [x, #-8]
				SUB      r1, r7, r1        @tt4
ZERO_X1:
			  SUB      r4, r1, r4          @tt2
			  SUB      r1, r4, r1, lsl#1   @tt4
			  
			  LDR      r2, [x, #-32]
			  LDR      r5, [x, #28]
			  LDR      r6, [x, #64]
			  ORRS     r7, r2, r5
			  BEQ      ZERO_X7
			  MOV      r7, #0xf2
			  SMLAWB   r8, r5, r7, r2
			  SMULWB   r2, r2, r7
			  STR      r8, [x, #-32]
			  SUB			 r5, r5, r2			  
			  SUB      r6, r6, r5         @tt5
ZERO_X7: 				
 				ADD      r5, r6, r5, lsl#1  @tt3
 				
 				LDR	     r2, [x, #52]
 				SUB      r7, r4, r2          @tt6
 				ADD      r2, r2, r4, asr#1   @tt0
 				
 				LDR      r4, [x, #-20]
 				LDR      r8, [x, #16]
 				ORRS     r9, r4, r8
 				LDRNE      r9, cs_ca_4
 				BEQ      ZERO_X4 				
 				SMLAWT   r10, r4, r9, r4
 				SMLAWT   r11, r8, r9, r8
 				SMLAWB   r10, r8, r9, r10
 				SMULWB   r8, r4, r9
 				STR      r10, [x, #-20]
 				SUB			 r8, r11, r8		
ZERO_X4:
 				LDR      r10, c2a_2b
 				SUB      r9, r8, r5           @tt7
 				ADD      r8, r8, r5, asr#1    @tt1
 				
 				@tt6 - r7  tt7 - r9 				
 				 SMLAWT  r4, r7, r10, r7 
 				 SMLAWB  r7, r7, r10, r9
 				 SMULWB  r5, r9, r10
 				 SMLAWT  r7, r9, r10, r7   @x_tmp[1]
 				 SUB     r4, r4, r5        @x_tmp[0]
 				 STR     r4, [sp,#-48]!
 				 STR     r7, [sp, #4]
 				 
 				 @tt0 - r2  tt1 - r8 				
 				 SMLAWT  r4, r8, r10, r8
 				 SMLAWB  r8, r8, r10, r2
 				 SMULWB  r5, r2, r10
 				 SMLAWT  r9, r2, r10, r8    @tt7
 				 LDR     r11, c1a_1b
 				 SUB     r4, r4, r5         @tt6
 				 
 				 @tt4 - r1  tt5 - r6 				 
 				 SMLAWT   r7, r1, r11, r1  
 				 SMLAWB   r1, r1, r11, r6
 				 SMULWB   r8, r6, r11
 				 SMLAWT   r1, r6, r11, r1     @tt3  
 				 SUB      r7, r7, r8          @tt2
 				 
 				 @tt3 - r1  tt7 - r9 				
 				 @tt2 - r7  tt6 - r4 				
 				 SUB      r9, r1, r9         @tt7
 				 SUB      r1, r9, r1, lsl#1  @tt3
 				 ADD      r4, r4, r7         @tt6
 				 SUB      r7, r4, r7, lsl#1  @tt2
 				 
 				 STR      r9, [sp, #8]
 				 STR      r7, [sp, #12]
 				 STR      r1, [sp, #24]
 				 STR		  r4, [sp, #28]
@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
@@@@*******************************@@@@@
@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
			  LDR			 r1, [x, #-28]   @a
			  LDR      r2, [x, #24]    @b tt2
			  ORRS     r4, r1, r2
			  LDRNE      r4 ,cs_ca_6		
			  BEQ			 ZERO_X6			  	  
			  SMLAWT   r5,r1,r4,r1   
        SMLAWT   r6,r2,r4,r2   @hi
        SMLAWB   r5,r2,r4,r5   @lo0
        SMULWB   r2,r1,r4   @hi1
        STR      r5, [r0, #-28]
        SUB      r2, r6, r2        
ZERO_X6:
				LDR     r1, [x, #-24]  @a
				LDR     r4, [x, #20]   @b
				ORRS    r5, r1, r4
				LDRNE     r6 ,cs_ca_5
				BEQ		  ZERO_X5        				
				SMLAWT  r7, r1, r6, r1
				SMLAWT  r8, r4, r6, r4
				SMLAWB  r7, r4, r6, r7
				SMULWB  r4, r1, r6
				STR     r7, [r0, #-24]
				SUB     r4, r8, r4  
				SUB     r2, r2, r4    @tt2				
ZERO_X5:
 				
 				ADD     r4, r2, r4, lsl#1
 				SMLAWB  r4, r4, r12, r4    @tt4
 				
 				@ r0 - x, r2 - tt2  r3 - sb r4 - tt4  
 				LDR     r1, [x, #44]
 				LDR     r5, [x, #48]
 				ADD     r1, r1, r5             @tt3
 				SUB     r5, r1, r5, lsl#1
 				SMLAWB  r5, r5, r12, r5        @tt5
 				
 				LDR     r6, [x, #68]
 				ADD     r7, r6, r2             @tt6
 				SUB     r6, r6, r2, asr#1      @tt0
 				
 				@tt0  - r6   
 				@tt6  - r7
 				@tt3  - r1  tt4 - r4  rr5 - r5
 				@r2 r8, r9, r11, r12, r14
 				
 				LDR     r2, [x, #-4]       @a
 				LDR     r8, [x], #-72        @b
 				ORRS    r9, r2, r8
 				LDRNE     r9  ,cs_ca_0
 				BEQ     ZERO_X0 				
 				SMLAWT  r11, r2, r9, r2 				
 				SMLAWB  r14, r8, r9, r8
 				SMLAWT  r8, r8, r9, r8
 				ADD     r11, r11, r14
 				SMLAWB  r2, r2, r9, r2
 				STR     r11, [x, #-4+72]
 				SUB     r8, r8, r2 				
ZERO_X0: 		
 				SUB     r9, r1, r8        @tt7
 				ADD     r8, r8, r1, asr#1 @tt1
  				
 				
 				@tt6 - r7   tt7 - r9
 				@tt0 - r6   tt4 - r4 				
 				SMLAWB   r11, r7, r10, r9
 				SMLAWT   r7, r7, r10, r7 				
 				SMLAWT   r11, r9, r10, r11 			@ x_tmp[3]
 				SMULWB   r9, r9, r10 				 				 				
 				SUB      r7, r7, r9        @ x_tmp[2]
 				LDR      r9, c3a_3b
 				
 				ADD     r6, r6, r5         @tt0
 				SUB     r5, r6, r5, lsl#1  @tt5
 				SUB     r8, r4, r8         @tt1
 				SUB     r4, r8, r4, lsl#1  @tt4
 				
 				
 				SMLAWB   r1, r6, r9, r6
 				SMLAWT   r6, r6, r9, r4
 				SMULWT   r14, r4, r9
 				SMLAWB   r6, r4, r9, r6      @tt6  - x_tmp[8]
 				LDR      r9, c4a_4b
 				SUB      r1, r14, r1       @tt7 - x_tmp[9]
 				STR      r6, [sp, #16] 
 				STR      r1, [sp, #20]
 				
 				@tt1 - r8   tt5 - r5
 				@r7 - x_tmp[2] r11 - x_tmp[2]
 				@r10 - c2a_2b  r9 - c4a_4b r12-c00_1
 				SMLAWT    r1, r5, r9, r8
 				SMULWB    r2, r5, r9
 				SMULWB    r4, r8, r9
 				RSB       r5, r5, r2, asr#4
 				SMLAWT    r5, r8, r9, r5            @tt7 - x_tmp[15] 				
 				SUB       r1, r1, r4, asr#4         @tt6 - x_tmp[14]
 				STR       r1, [sp, #32]
 				STR       r5, [sp, #36] 
 				 				
@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
@@@@*******************************@@@@@
@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@				
 				@r7 - x_tmp[2] r11 - x_tmp[2]
 				@r10 - c2a_2b  r9 - c4a_4b
 				@r1, r2, r4, r5, r6, r8, r14
 				LDR      r1, [x, #-12+72]
 				LDR      r2, [x, #8+72]
 				LDR      r4, [x, #36+72]    @@tt4
 				ORRS     r5, r1, r2
 				LDRNE      r5, cs_ca_2
 				BEQ      ZERO_X2 				
 				SMLAWT   r6, r1, r5, r1
 				SMLAWT   r8, r2, r5, r2
 				SMLAWB   r6, r2, r5, r6
 				SMULWB   r2, r1, r5
 				STR      r6, [x, #-12+72]
 				SUB      r2, r8, r2     @tt2
ZERO_X2: 
				SUB      r2, r4, r2        @tt2
				SUB      r4, r2, r4, lsl#1				
				LDR      r1, [x, #56+72]
				SMLAWB   r4, r4, r12, r4   @tt4 
				
				ADD      r5, r1, r2          @tt6				
								
 				LDR      r8, [x, #-16+72]
 				LDR      r6, [x, #12+72]
 				
 				SUB      r1, r1, r2, asr#1   @tt0
 				
 				@@tt0 - r1   tt6 - r5
 				@tt4 - r4
 				
 				ORRS     r2, r8, r6
 				LDRNE    r2, cs_ca_3
 				BEQ      ZERO_X3 				
 				SMLAWT   r14, r8, r2, r8
 				SMULWB   r8, r8, r2 				
 				SMLAWB   r14, r6, r2, r14
 				SMLAWT   r6, r6, r2, r6
 				STR      r14, [x, #-16+72]
 				SUB      r6, r6, r8     @tt1
ZERO_X3:  		
 				@@tt0 - r1   tt6 - r5
 				@tt4 - r4    tt1 - r6 						
 				LDR      r2, [x, #60+72]
 				LDR      r8, [x, #32+72] 				
 				SUB      r8, r8, r2         
 				ADD      r2, r8, r2, lsl#1  @tt3
 				SMLAWB   r8, r8, r12, r8    @tt5
 				
 				@r7 - x_tmp[2] r11 - x_tmp[2]
 				@r10 - c2a_2b  r9 - c4a_4b r12-c00_1 				
 				@@tt0 - r1   tt6 - r5
 				@tt4 - r4    tt1 - r6 
 				@tt3 - r2  tt5 - r8
 				SUB      r14, r6, r2        @tt7
 				ADD      r6, r6, r2, asr#1  @tt1
 				
 				@tt6 - r5   tt7 - r14
 				@tt0 - r1   tt4 - r4 
 				@tt1 - r6   tt5 - r8 				
 				SMLAWB    r2, r5, r10, r14
 				SMLAWT    r5, r5, r10, r5
 				SMLAWT    r2, r14, r10, r2   @x_tmp[5]
 				SMULWB    r14,r14, r10    
 				
 				ADD     r1, r1, r8          @tt0
 				SUB     r8, r1, r8, lsl#1  @tt5
 				ADD     r6, r6, r4
 				SUB     r4, r6, r4, lsl#1
 				
 				SUB     r5, r5, r14        @x_tmp[4]
 				
 				
 			  @x_tmp[4] - r5   x_tmp[5] - r2
 				@tt0 - r1   tt4 - r4 
 				@tt1 - r6   tt5 - r8 		
 				@r7 - x_tmp[2] r11 - x_tmp[2]
 				@r9 - c4a_4b r12-c00_1 		
 				@r10, r14
 				SMULWB   r14, r1, r9
 				SMLAWT   r10, r1, r9, r4
 				RSB      r1, r1, r14, asr#4
 				SMLAWT   r1, r4, r9, r1            @tt3  -- x_tmp[11]
 				SMULWB   r4, r4, r9
 				LDR      r9, c6a_6b	
 				SUB      r4, r10, r4, asr#4         @tt2 - x_tmp[10]
 				
 		 	  @x_tmp[4] - r5   x_tmp[5] - r2 				
 				@tt1 - r6   tt5 - r8 		
 				@x_tmp[2] - r7   x_tmp[2] - r11
 				@x_tmp[10] - r4   x_tmp[11] - r1
 				@r9 - c6a_6b r12-c00_1 				
 				@ r10, r14
 				SMULWB   r14, r6, r9
 				SMULWT   r10, r6, r9
 				ADD      r14, r8, r14, asr#3
 				RSB      r6, r6, r10, asr#9
 				SMULWB   r10, r8, r9
 				SMULWT   r8, r8, r9
 				ADD      r10, r6, r10, asr#3        @tt3 - x_tmp[17] 				
 				RSB      r6, r14, r8, asr#9     @tt2 - x_tmp[16]



 				
@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@    
@@@@*******************************@@@@@@@@@*******************************@@@@@@@@@*******************************@@@@@    
@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@						 				  
 				@x_tmp[2] - r7   x_tmp[3] - r11
 				@x_tmp[4] - r5   x_tmp[5] - r2 				 				 				
 				@x_tmp[10] - r4   x_tmp[11] - r1 		
 				@x_tmp[16] - r6   x_tmp[17] - r10 		
 						
 				@r12-c00_1 			
 				@r0, r3, r8, r9, r14
 				TST    r3, #1
 				STR    r0, [sp, #40] 
 				STR    r3, [sp, #44] 
 				
 				LDR    r0, [sp, #52]         @overlap
 				 				
 				LDR    r8, [sp], #4      @x_tmp[0]-tt00
 				LDR    r9, [sp], #4      @x_tmp[1]-tt01
 				ADD    r7, r7, r5         @tt02
 				SUB    r5, r7, r5, lsl#1
 				ADD    r11, r11, r2       @tt03
 				SUB    r2, r11, r2, lsl#1
 				SMLAWB r5, r5, r12, r5    @tt04 
 				SMLAWB r2, r2, r12, r2    @tt05

 				ADD    r3, r9, r11          @tt07
 				ADD    r14, r11, r5, lsl#1  @tt2
 				RSB    r5, r11, r5, lsl#1   @tt3
 				RSB    r11, r9, r14, asr#1  @tt2
 				ADD    r5, r9, r5, asr#1    @tt3
 				
 				ADD    r9, r8, r7
 				ADD    r14, r7, r2, lsl#1   @tt04
 				RSB    r2, r7, r2, lsl#1    @tt05
 				ADD    r2, r8, r2, asr#1    @tt05
 				RSB    r7, r8, r14, asr#1   @tt04
 				
 				@tt06 - r9  tt07 - r3
 				@tt05 - r2  tt2  - r11
 				@tt3  - r5  tt04 - r7 
 				@x_tmp[10] - r4   x_tmp[11] - r1 		
 				@x_tmp[16] - r6   x_tmp[17] - r10 		
 				@r8, r14		
 				 				
 				LDR    r8, [overlap, #16]  @t0 -overlap[4]
 				LDR    r14, window_l_4_13
 				STR    r3, [overlap, #16]
 				
 	@	sample [   4][sb] =  -mp3_f_mul_win_1_t(/*X[13]*/tt06, window_l_4_13) + mp3_f_mul_win_1(t0, window_l_4_13) + t0@
	@	t2 = mp3_f_mul_win_1(/*X[13]*/tt06, window_l_4_13)+tt06 + mp3_f_mul_win_1_t(t0, window_l_4_13)@						
	@	sample [17-4][sb] = (sb&1)?-t2 : t2@
	
				SMLAWT	 r3, r8, r14, r9
 				SMLAWB   r8, r8, r14, r8
 				SMLAWB   r3, r9, r14, r3      @t2
 				SMULWT   r9, r9, r14
 				RSBNE    r3, r3, #0   				  
 				MOV      r14, r3 				
				LDR      r3, [sp, #40]             @sample
 				SUB      r8, r8, r9
 				STR      r14, [sample, #1664]              @sample[13][sb]
 				STR      r8, [sample, #512]              @sample[ 4][sb]
 				 				
 				@tt05 - r2  tt2  - r11
 				@tt3  - r5  tt04 - r7 
 				@x_tmp[10] - r4   x_tmp[11] - r1 		
 				@x_tmp[16] - r6   x_tmp[17] - r10 		
 				@r8, r9, r14
 				LDR      r14, window_l_1_16
 				LDR      r8, [overlap, #4]
 				STR      r11,[overlap, #4]
 				
		   @t1 = mp3_f_mul_win_1(t0, window_l_1_16)@
		   @t2 = mp3_f_mul_win_1_t(t0, window_l_1_16)@
		   @/*sample [   1][sb]*/t1 =  -mp3_f_mul_win_1_t(/*X[10]*/tt05, window_l_1_16) + t1 + t0@
		   @sample [1][sb] = (sb&1)?-t1 : t1@
		   @sample [17-1][sb] = mp3_f_mul_win_1(/*X[10]*/tt05, window_l_1_16) + tt05 + t2@		
		   SMLAWT   r9, r8, r14, r2
		   SMLAWB	 r8, r8, r14, r8
		   SMLAWB   r9, r2, r14, r9       @sample[16][sb]
		   SMULWT   r2, r2, r14
		   STR     r9, [sample, #2048]
		   SUB     r8, r8, r2
		   RSBNE   r8, r8, #0        @sample[1][sb]
		   STR     r8, [sample,#128 ]
				 		
 				@tt3  - r5  tt04 - r7 
 				@x_tmp[10] - r4   x_tmp[11] - r1 		
 				@x_tmp[16] - r6   x_tmp[17] - r10 		
 				@r8, r9, r2, r11, r14
 				LDR     r14, window_l_7_10
 				LDR     r8, [overlap, #28]
 				STR     r7, [overlap, #28]
 				
 				SMLAWB  r9, r8, r14, r8
 				SMLAWT  r8, r8, r14, r8
 				SMLAWT  r7, r5, r14, r5
 				SMLAWB  r5, r5, r14, r5
 				ADD     r9, r9, r7
 				RSBNE   r9, r9, #0
 				STR     r9, [sample,#896 ]       @sample[7][sb]
 				SUB     r8, r8, r5
 				STR     r8, [sample,#1280 ]       @sample[10][sb]
 				
@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
@@@@*******************************@@@@@
@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
 				@x_tmp[6] - r8   x_tmp[7] - r9	 		
 				@x_tmp[8] - r7   x_tmp[9] - r11		
 				@x_tmp[10] - r4   x_tmp[11] - r1 		
 				@x_tmp[16] - r6   x_tmp[17] - r10 		
 				@c00_1 - r12  overlap-r0 sample- r3
 				@r2, r5,  r14 				
 				
 				LDR    r8, [sp], #4       @x_tmp[6]-tt0
 				LDR    r9, [sp], #4       @x_tmp[7]-tt1
 				LDR    r7, [sp], #4       @x_tmp[8]-tt2 				
 				LDR    r11, [sp], #4      @x_tmp[9]-tt3
 				
 				ADD    r7, r7, r4         @tt2
 				SUB    r4, r7, r4, lsl#1
 				ADD    r11, r11, r1       @tt3
 				SUB    r1, r11, r1, lsl#1
 				SMLAWB r4, r4, r12, r4    @tt4 
 				SMLAWB r1, r1, r12, r1    @tt5

 				ADD    r2, r9, r11          @tt7
 				ADD    r14, r11, r4, lsl#1  @tt04
 				RSB    r5, r11, r4, lsl#1   @tt05
 				RSB    r11, r9, r14, asr#1  @tt04
 				ADD    r5,  r9, r5, asr#1   @tt05
 				
 				ADD    r9, r8, r7
 				ADD    r14, r7, r1, lsl#1   @tt06
 				RSB    r1, r7, r1, lsl#1    @tt07
 				ADD    r1, r8, r1, asr#1    @tt07
 				RSB    r7, r8, r14, asr#1   @tt06
 				
 				@tt6 - r9  tt7 - r2
 				@tt07 - r1  tt04  - r11
 				@tt05  - r5  tt06 - r7  			
 				@x_tmp[16] - r6   x_tmp[17] - r10 		
 				@r4, r8, r14		
 				 				
 				LDR    r8, [overlap, #12]  @t0 -overlap[3]
 				LDR    r14, window_l_3_14
 				STR    r9, [overlap, #12]
 				
@	t1 = mp3_f_mul_win_1(t0, window_l_3_14)@
@		t2 = mp3_f_mul_win_1_t(t0, window_l_3_14)@
@		/*sample [   3][sb]*/t1 =  -mp3_f_mul_win_1_t(/*X[3 + 9]*/tt7, window_l_3_14) + t1 + t0@
@		sample [3][sb] = (sb&1)?-t1 : t1@
@		sample [17-3][sb] = mp3_f_mul_win_1(/*X[3 + 9]*/tt7, window_l_3_14) + tt7 + t2@	
	
				SMLAWT	 r4, r8, r14, r2
 				SMLAWB   r8, r8, r14, r8
 				SMLAWB   r4, r2, r14, r4      @t2
 				SMULWT   r2, r2, r14 					   				
 				SUB      r8, r8, r2  
 				RSBNE    r8, r8, #0
 				STR      r4, [sample,#1792 ]              @sample[14][sb]
 				STR      r8, [sample,#384 ]              @sample[ 3][sb]
 				 				
				@tt07 - r1  tt04  - r11
 				@tt05  - r5  tt06 - r7  			
 				@x_tmp[16] - r6   x_tmp[17] - r10 		
 				@r2, r4, r8, r9,  r14		
 				LDR      r14, window_l_8_9
 				LDR      r8, [overlap, #32]
 				STR      r11,[overlap, #32]
 				
@		t1 = mp3_f_mul_win_1(t0, window_l_8_9) + t0@
@		t2 = mp3_f_mul_win_1_t(t0, window_l_8_9) + t0@
@		sample [   8][sb] =  (mp3_f_mul_win_1_t(/*X[8 + 9]*/tt07, window_l_8_9) + tt07 + t1)@
@		/*sample [17-8][sb]*/t2 = -mp3_f_mul_win_1(/*X[8 + 9]*/tt07, window_l_8_9) - tt07 + t2@						
@		sample [17-8][sb] = (sb&1)?-t2 : t2@
				SMLAWB   r2, r8, r14, r8				
				SMLAWT   r8, r8, r14, r8
				SMLAWT   r4, r1, r14, r1
				SMLAWB   r1, r1, r14, r1
				ADD      r2, r2, r4
				SUB      r1, r8, r1
				RSBNE    r1, r1, #0
				STR      r2, [sample, #1024]              @sample[8][sb]
				STR      r1, [sample, #1152]               @sample[9][sb]


		
				@tt05  - r5  tt06 - r7  			
 				@x_tmp[16] - r6   x_tmp[17] - r10 		
 				@r1, r2, r4, r8, r9, r11  r14				 				
 				LDR     r14, window_l_2_15
 				LDR     r8, [overlap, #8]
 				STR     r7, [overlap, #8]
 		
 		@t1 = mp3_f_mul_win_1(t0, window_l_2_15)@
		@t2 = mp3_f_mul_win_1_t(t0, window_l_2_15)@
		@sample [   2][sb] =  -mp3_f_mul_win_1_t(/*X[2 + 9]*/tt05, window_l_2_15) + t1 + t0@
		@/*sample [17-2][sb]*/t2 = mp3_f_mul_win_1(/*X[2 + 9]*/tt05, window_l_2_15) + tt05 +t2@						
		@sample [17-2][sb] = (sb&1)?-t2 : t2@ 				
 				SMLAWB  r9, r8, r14, r8
 				SMLAWT  r8, r8, r14, r5
 				SMULWT  r1, r5, r14
 				SMLAWB  r8, r5, r14, r8
 				SUB     r9, r9, r1
 				RSBNE   r8, r8, #0
 				STR     r9,  [sample, #256]               @sample[2][sb]
 				STR     r8,  [sample, #1920]               @sample[15][sb]
 				
 				
@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
@@@@*******************************@@@@@
@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
 				@x_tmp[12] - r8   x_tmp[13] - r9	 		
 				@x_tmp[14] - r7   x_tmp[15] - r11	
 				@x_tmp[16] - r6   x_tmp[17] - r10 		
 				@c00_1 - r12  overlap-r0 sample- r3 				
 				
 				LDR    r8,  [sp], #4      @x_tmp[12]-tt0
 				LDR    r9,  [sp], #4     @x_tmp[13]-tt1
 				LDR    r7,  [sp], #4       @x_tmp[14]-tt2 				
 				LDR    r11, [sp], #4      @x_tmp[15]-tt3
 				
 				ADD    r7, r7, r6         @tt2
 				SUB    r4, r7, r6, lsl#1
 				ADD    r11, r11, r10       @tt3
 				SUB    r1, r11, r10, lsl#1
 				SMLAWB r4, r4, r12, r4    @tt4 
 				SMLAWB r1, r1, r12, r1    @tt5

 				ADD    r2, r9, r11          @tt7
 				ADD    r14, r11, r4, lsl#1  @tt04
 				RSB    r5, r11, r4, lsl#1   @tt05
 				RSB    r11, r9, r14, asr#1  @tt04
 				ADD    r5,  r9, r5, asr#1   @tt05
 				
 				ADD    r9, r8, r7
 				ADD    r14, r7, r1, lsl#1   @tt06
 				RSB    r1, r7, r1, lsl#1    @tt07
 				ADD    r1, r8, r1, asr#1    @tt07
 				SUB    r7, r8, r14, asr#1   @tt06
 				
 				@tt6 - r9  tt7 - r2
 				@tt07 - r1  tt04  - r11
 				@tt05  - r5  tt06 - r7  			 				
 				@r4, r8, r14, r6, r10
 				 				
 				LDR    r8, [overlap, #24]  @t0 -overlap[6]
 				LDR    r14, window_l_6_11
 				STR    r9, [overlap, #24]
 				
		@t1 = mp3_f_mul_win_1(t0, window_l_6_11) + t0@
		@t2 = mp3_f_mul_win_1_t(t0, window_l_6_11) + t0@
		@sample [   6][sb] =  (mp3_f_mul_win_1_t(tt7, window_l_6_11) + tt7 + t1)@
		@/*sample [17-6][sb]*/t2 = -mp3_f_mul_win_1(tt7, window_l_6_11) - tt7 + t2@						
		@sample [17-6][sb] = (sb&1)?-t2 : t2@
				SMLAWB  r4, r8, r14, r8
				SMLAWT  r8, r8, r14, r8
				SMLAWT  r6, r2, r14, r2
				SMLAWB  r2, r2, r14, r2
				ADD     r4, r4, r6
				SUB     r8, r8, r2
				RSBNE   r8, r8, #0		
				STR      r4, [sample, #768 ]              @sample[6][sb]
 				STR      r8, [sample, #1408 ]              @sample[11][sb]
 				 				
				@tt07 - r1  tt04  - r11
 				@tt05  - r5  tt06 - r7  			
 				@x_tmp[16] - r6   x_tmp[17] - r10 		
 				@r2, r4, r8, r9,  r14		
 				LDR      r14, window_l_0_17
 				LDR      r8, [overlap]
 				STR      r11,[overlap], #20
 				
		@t1 = mp3_f_mul_win_1(t0, window_l_0_17)@
		@t2 = mp3_f_mul_win_1_t(t0, window_l_0_17)@
		@sample [   0][sb] =  mp3_f_mul_win_1_t(/*X[0 + 9]*/tt07, window_l_0_17) + t1 + t0@
		@/*sample [17-0][sb]*/t2 = -mp3_f_mul_win_1(/*X[0 + 9]*/tt07, window_l_0_17) - tt07 + t2@						
		@sample [17-0][sb] = (sb&1)?-t2 : t2@
				SMLAWB    r2, r8, r14, r8
				SMLAWB    r4, r1, r14, r1
				SMULWT    r1, r1, r14
				SMULWT    r8, r8, r14
				ADD       r2, r2, r1
				SUB       r8, r8, r4
				RSBNE     r8, r8, #0	
				STR       r8,  [sample, #2176 ]              @sample[17][sb]
				STR       r2,  [sample], #640             @sample[0][sb]
				

		
				@tt05  - r5  tt06 - r7  			
 				@x_tmp[16] - r6   x_tmp[17] - r10 		
 				@r1, r2, r4, r8, r9, r11  r14				 				
 				LDR     r14, window_l_5_12
 				LDR     r8, [overlap] 
 				STR     r5, [overlap], #-56
 		
		@t1 = mp3_f_mul_win_1(t0, window_l_5_12)@
		@t2 = mp3_f_mul_win_1_t(t0, window_l_5_12)@
		@/*sample [   5][sb]*/ t1 =  mp3_f_mul_win_1_t(tt06, window_l_5_12) + t1 + t0@
		@sample [5][sb] = (sb&1)?-t1 : t1@
		@sample [17-5][sb] = -mp3_f_mul_win_1(tt06, window_l_5_12) - tt06 + t2@		
				SMLAWB  r9, r8, r14, r8
				SMLAWB  r1, r7, r14, r7
				SMULWT  r7, r7, r14
				SMULWT  r8, r8, r14
				ADD     r9, r9, r7
				RSBNE   r9, r9, #0
				SUB     r8, r8, r1	
				STR     r9, [sample], #896               @sample[5][sb]
 				STR     r8, [sample], #-1540               @sample[12][sb]
 				
 				
 				STR     r0, [sp, #12]
 				STR     r3, [sp, #8]
 				
 				LDR     r0, [sp], #4
 				LDR     r3, [sp], #4
      
        SUBS     r3, r3, #1
        BGT      LOOP_START
LOOP_END:
        LDR      r2, [sp], #4
        LDR      r1, [sp], #4  
        @ADD      sp, sp, #8
        LDMFD    sp!,{r4-r12,lr}                        
        MOV      r3,#1
        B        MP3_DEC_ImdctWinOverlap
              
cs_ca_6:
        .word      0xfff903a3
cs_ca_5:
        .word      0xffc90a7d
cs_ca_0:
        .word      0xdb8583b6
cs_ca_2:
        .word      0xf31b503a
cs_ca_3:
        .word      0xfbbb2e92
cs_ca_1:
        .word      0xe1ba78c3
cs_ca_4:
        .word      0xfeda1836
c1a_1b:
        .word      0xccd4ab29
c2a_2b:
        .word      0xec839e09
c00_1:
        .word      0xffffddb4
c3a_3b:
        .word      0x7635e314
c4a_4b:
        .word      0xc8976117
c6a_6b:
        .word      0x7cc05955
window_l_4_13:
        .word      0x61f8ec83
window_l_1_16:
        .word      0x216afdcf
window_l_7_10:
        .word      0x9bd8cb19
window_l_3_14:
        .word      0x4cfbf427
window_l_8_9:
        .word      0xacf3bcbe
window_l_2_15:
        .word      0x3769f9ef
window_l_6_11:
        .word      0x898cd7e9
window_l_0_17:
        .word      0x0b2bffc2
window_l_5_12:
        .word      0x7635e313        
        @ENDP        
        

@--------------------------------------------------------------------
@   Function:
@       void MP3_DEC_ImdctWinOverlap1(mp3_fixed_t *x,,  mp3_fixed_t sample[18][32]
@               unsigned int sb, unsigned int sblimit, mp3_fixed_t Overlap[32][9])
@
@   Description: Perform frequency line alias reduction.
@
@   Register usage:
@       r0 = X
@       r1 = overlap
@			  r2 = sb
@       r3 = sblimit
@       sp = sample
@--------------------------------------------------------------------				

overlap .req  r0
sample  .req  r3

MP3_DEC_ImdctWinOverlap1:
        STMFD    sp!,{r4-r12,lr}        				
        ADD      r1, r1, r2, lsl#2   @@// sample
        ldr      r4,  [sp, #10*4]     @@// overlap
        sub      sp,   sp,   #8
        str      r4,   [sp,  #0]     @@// overlap
	    str      r1,   [sp,  #4]     @@// sample
				              
        LDR      r9, c00_1
        MOV      r12, r2
        
START_LOOP_1:
        CMP      r12, r3
        BGE      OUT_LOOP_1      
        TST			 r12, #1
        SUB      sp, sp, #60
        STR      r12, [sp, #56]  
        
        LDR			 r1, [r0, #24]   @X[6]
        LDR			 r2, [r0, #20]   @X[5]
        LDR			 r4, [r0, #44]   @X[11]
        LDR			 r5, [r0, #48]   @X[12]
        LDR			 r6, [r0, #68]   @X[17]
        LDR			 r7, [r0], #8   @X[0]        
				LDR      r8, c2a_2b
                
        SUB			 r1, r1, r2          @tt2
        ADD      r2, r1, r2, lsl#1   
        SMLAWB   r2, r2, r9, r2      @tt4
        
        ADD      r4, r4, r5          @tt3
        SUB      r5, r4, r5, lsl#1
        SMLAWB   r5, r5, r9, r5      @tt5
        
        ADD			 r10, r6, r1          @tt6
        SUB      r6, r6, r1, asr#1    
        SUB      r1, r4, r7           @tt7
        ADD      r7, r7, r4, asr#1    
        ADD      r6, r6, r5           @tt0
        SUB      r5, r6, r5, lsl#1    @tt5
        SUB      r7, r2, r7           @tt1
        SUB      r2, r7, r2, lsl#1    @tt4
        
        SMLAWT   r4, r10, r8, r10
        SMLAWB   r10, r10, r8, r1
        SMULWB   r11,  r1, r8        
        SMLAWT   r1, r1, r8, r10     @tt3 - x_tmp[3]     
        SUB      r4, r4, r11         @tt2 - x_tmp[2]
        LDR      r11, c3a_3b   
        STR      r4, [sp, #0]
        STR      r1, [sp, #4]
        
        SMLAWB   r10, r6, r11, r6
        SMLAWT   r6, r6, r11, r2
        SMULWT   r12, r2, r11     
        SMLAWB   r2, r2, r11, r6     @tt6 - x_tmp[8]
        LDR      r11, c4a_4b
        SUB      r6, r12, r10        @tt7 - x_tmp[9]    
        STR      r2, [sp, #16]      
        STR      r6, [sp, #20]
        
        SMLAWT   r10, r5, r11, r7        
        SMULWB   r12, r7, r11
        SMULWB   r14, r5, r11
        SUB      r10, r10, r12, asr#4     @tt6  - x_tmp[14]
        RSB      r5,  r5,  r14, asr#4      
        SMLAWT   r5,  r7, r11, r5         @tt7  - x_tmp[15]        
        STR      r10, [sp, #32]      
        STR      r5, [sp, #36]
        
        
        @ x_tmp[3]- r1  x_tmp[2]-r4  r0, r3
        @ r9-c00_1  r8-c2a_2b  r11-c4a_4b
        LDR	    r2, [r0], #28     @tt2
        LDR     r5, [r0], #24    @tt4
        LDR     r6, [r0], #-28    @tt3
        LDR     r7, [r0], #24    @tt5
        LDR     r1, [r0], #-44    @tt0      
        LDR     r4, [r0], #-8    @tt1  
        
        SUB     r2, r5, r2         @tt2
        SUB     r5, r2, r5, lsl#1
        SMLAWB  r5, r5, r9, r5     @tt4
        
        SUB     r7, r7, r6
        ADD     r6, r7, r6, lsl#1   @tt3
        SMLAWB  r7, r7, r9, r7      @tt5
        
        ADD    r10, r1, r2          @tt6
        SUB    r1, r1, r2, asr#1    @tt0
        SUB    r2, r4, r6           @tt7
        ADD    r4, r4, r6, asr#1    @tt1      
        
        SMLAWT   r12, r10, r8, r10
        SMLAWB   r10, r10, r8, r2
        SMULWB   r14, r2, r8
        SMLAWT   r2, r2, r8, r10        @tt3  - x_tmp[5]
        SUB      r10, r12, r14          @tt2  - x_tmp[4]
         
        @r0, r3, r2, r10
        @tt4-r5  tt5-r7  tt0-r1  tt1- r4
        @ r9-c00_1, r8-c2a_2b, r11-c4a_4b
        ADD   r1, r1, r7          @tt0
        SUB   r7, r1, r7, lsl#1   @tt5
        ADD   r4, r4, r5        @tt1
        SUB   r5, r4, r5, lsl#1  @tt4
        
        SMLAWT   r6, r1, r11, r5 
        SMULWB   r12, r1, r11
        SMULWB   r14, r5, r11
        RSB      r1, r1, r12, asr#4
        SUB      r6, r6, r14, asr#4        @tt2-x_tmp[10]
        SMLAWT   r5, r5, r11, r1           @tt3-x_tmp[11]
        LDR      r11, c6a_6b
        STR      r6, [sp, #24]
        STR      r5, [sp, #28]
        
        @r0, r3, r2, r10
        @tt5-r7   tt1- r4
        @ r9-c00_1, r8-c2a_2b, r11-c4a_4b
        @free: r1, r5, r6, r12, r14
        SMULWT   r1, r7, r11
        SMULWB   r5, r7, r11
        RSB      r7, r7, r1, asr#9
        RSB      r1, r4, r5, asr#3
        SMULWB   r5, r4, r11
        SMULWT   r6, r4, r11
        SUB      r7, r7, r5, asr#3        @tt2 - x_tmp[16]
        ADD      r1, r1, r6, asr#9        @tt3 - x_tmp[17]
        STR      r7, [sp, #40]
        STR      r1, [sp, #44]
        
         @r0, r3, r2, r10        
        @ r9-c00_1, r8-c2a_2b 
        @free: r1, r4, r5, r6, r7, r11 r12, r14
        LDR    r1,  [r0], #36     @X[1]
        LDR    r4,  [r0], #24     @X[10]
        LDR    r5,  [r0], #-36    @X[16]
        LDR    r6,  [r0], #24     @X[7]
        LDR    r7,  [r0], #-36    @X[13]
        LDR    r11, [r0],#56      @X[4]

        
        SUB    r4, r1, r4         @tt2
        SUB    r1, r4, r1, lsl#1  @tt4
        SUB    r5, r5, r6         @tt5
        ADD    r6, r5, r6, lsl#1  @tt3
        
        SUB    r12, r4, r7         @tt6
        ADD    r7, r7, r4, asr#1   @tt0
        SUB    r4, r11, r6         @tt7
        ADD    r11, r11, r6, asr#1 @tt1
        
        SMLAWT  r14, r12, r8, r12
        SMLAWB  r12, r12, r8, r4
        SMULWB  r6,  r4, r8        
        SMLAWT  r4, r4, r8, r12        @x_tmp[1]
        SUB     r6, r14, r6            @x_tmp[0]
        
        STR     r0,[sp, #48]
        STR     r3,[sp, #52]
        
         @r0, r3, r2-x_tmp[5], r10-x_tmp[4]        
        @ r9-c00_1, r8-c2a_2b 
        @ x_tmp[0]-r6  x_tmp[1] -r4
        @tt0 - r7  tt1 - r11  tt4 - r1  rr5-r5
        @free:  r12, r14        
       SMLAWT   r14, r11, r8, r11
       SMLAWB   r11, r11, r8, r7
       SMULWB   r12, r7, r8
       SMLAWT   r7, r7, r8, r11    @tt7
       SUB      r8, r14, r12       @tt6
       STR      r8, [sp, #8]
       STR      r7, [sp, #12]
        
        @r0, r3, r2-x_tmp[5], r10-x_tmp[4]        
        @ r9-c00_1
        @ x_tmp[0]-r6  x_tmp[1] -r4
        @tt6 - r7  tt7 - r8  tt4 - r1  tt5-r5
        @free:  r11, r12, r14 
        LDR     r0, [sp, #60]          @overlap      
        @LDR     r0, [sp, #100]          @overlap      
                              
        LDR    r11, [sp], #4  @x_tmp[2]
        LDR    r12, [sp], #4          @x_tmp[3]  
        
        @ x_tmp[0]-r6   x_tmp[1] -r4
        @ x_tmp[2]-r11  x_tmp[3] -r12
        @ x_tmp[4]-r10  x_tmp[5] -r2        
        ADD  r11, r11, r10           @tt02
        SUB  r10, r11, r10, lsl#1
        SMLAWB r10, r10, r9, r10      @tt04
        ADD  r12, r12, r2             @tt03
        SUB  r2, r12, r2, lsl#1
        SMLAWB r2, r2, r9, r2         @tt05
        
        @ r7, r8, r14, r0, r3
        @LDR     r3, window_s_4_13
        @LDR     r3, [sp, #92]        @sample                  
        LDR     r3, [sp, #56]        @sample                  
                
        ADD     r7, r6, r11       @tt06
        ADD     r8,  r4, r12      @tt07
        
        LDR     r14, [overlap, #16]                  @t0-overlap[4]@sample [   4][sb]
        STR     r8,  [overlap, #16]                        
        
        @ r8,                 
        RSBNE   r7, r7, #0       @sample [   13][sb]                
        STR     r14,  [sample, #512]        
        STR     r7, [sample, #1664]
        
        @r0, r3    r7, r8, r14
        @tt04 - r10  tt03-r12  tt02 - r11  tt05 - r2
        @tt00 - r6  tt01 - r4
        ADD			r7, r12, r10, lsl#1   @tt06
        RSB     r12, r12, r10, lsl#1  @tt07
        ADD     r10, r11, r2, lsl#1   @@tt04
        RSB     r2, r11, r2, lsl#1    @tt05
        ADD     r2, r6, r2, asr#1     @tt05
        RSB     r7, r4, r7, asr#1     @tt06
        RSB     r10, r6, r10, asr#1   @tt04
        ADD     r12, r4, r12, asr#1   @tt07
        
        @r0, r3   r4,  r6,  r8, r11, r14
        @tt04 - r10  tt07 - r12
        @tt05 - r2  tt06 - r7
        @tt4 - r1  tt5-r5
        LDR      r4, [overlap, #4]           @t0
        STR      r7, [overlap, #4]           @overlap[1]
        
        LDR       r7, window_s_7_10
        
        LDR      r6, [overlap, #28]          @t0
        STR       r10, [overlap, #28]        @overlap[7]
        
        RSBNE     r4, r4, #0              @sample [1][sb]                        
        STR       r4, [sample, #128] 
        STR       r2, [sample, #2048]      @sample [16][sb]        
        
		    SMLAWB    r4, r6, r7, r6
		    SMULWT    r8, r6, r7
		    SMLAWT    r4, r12, r7, r4		    
		    SMLAWB    r14, r12, r7, r12		    
		    SUB       r8, r8, r14             @sample[10][sb]
		    RSBNE     r4, r4, #0              @sample[7][sb]      
		    STR       r4, [sample, #896]
		    STR       r8, [sample, #1280]
		    
		    LDR       r8, c1a_1b
		    @r0, r3   
		    @r2, r4, r6, r7, r8, r10, r11, r12, r14
        @tt4 - r1  tt5-r5
        LDR     r6, [sp], #4            @tt6
        LDR     r7, [sp], #4           @tt7
        
        SMLAWT   r2, r1, r8, r1
        SMLAWB   r1, r1, r8, r5
        SMULWB   r4, r5, r8
        SMLAWT   r1, r5, r8, r1    @tt3
        SUB      r2, r2, r4        @tt2
        
        SUB     r7, r1, r7         @tt7
        SUB     r1, r7, r1, lsl#1  @tt3 
        ADD     r6, r6, r2         @tt6
        SUB     r2, r6, r2, lsl#1  @tt2
        
        @x_tmp[6] - r7  x_tmp[7] - r2
        @x_tmp[12] - r1  x_tmp[13] - r6
        @r0, r3   
		    @r4, r5, r8, r10, r11, r12, r14
		    LDR      r4,   [sp], #4            @x_tmp[8] - tt2
		    LDR      r5,   [sp], #4           @x_tmp[9] - tt3
		    LDR      r8,   [sp], #4           @x_tmp[10]- tt4
		    LDR      r10,  [sp], #4            @x_tmp[11]- tt5
		    
		    ADD      r4, r4, r8          @tt2
		    SUB      r8, r4, r8, lsl#1   
		    ADD      r5, r5, r10         @tt3
		    SUB      r10, r5, r10, lsl#1  
		    SMLAWB   r8, r8, r9, r8      @tt6
		    SMLAWB   r10, r10, r9, r10   @tt7
		    
		    ADD     r11, r7, r4      @tt4
		    ADD     r12, r2, r5      @tt5 
		    
		    @LDR     r9, window_s_3_14
		    
		    LDR     r14, [overlap, #12]    @t0 - overlap[3]
		    STR     r11, [overlap, #12]    
		    					
        @x_tmp[12] - r1  x_tmp[13] - r6
        @r0-overlap, r3-sample
        @r4, r5, r8, r10, r2, r7
		    @r12 -tt5, r14 - t0
		    @r11	    
		    RSBNE    r14, r14, #0		           @sample[3][sb] 		    
		    STR		  r12, [sample, #1792]           @sample[14][sb]		    
		    STR     r14, [sample, #384]
		    
        @x_tmp[12] - r1  x_tmp[13] - r6
        @r0-overlap, r3-sample
        @r4-tt2, r5-tt3, r8-tt6, r10-tt7, r2-tt1, r7-tt0		    
		    @r11, r9, r12, r14	    
		    ADD    r9, r5, r8, lsl#1      @tt4
		    RSB    r5, r5, r8, lsl#1      @tt5
		    ADD    r8, r4, r10, lsl#1     @tt6
		    RSB    r4, r4, r10, lsl#1     @tt7
		    ADD    r4, r7, r4, asr#1      @tt7
		    RSB    r9, r2, r9, asr#1      @tt4
		    RSB    r8, r7, r8, asr#1      @tt6
		    ADD    r5, r2, r5, asr#1      @tt5
		    
		    @x_tmp[12] - r1  x_tmp[13] - r6
        @r0-overlap, r3-sample
        @r8-tt6, r4-tt7, r9-tt4, r5-tt5		    
		    @r2, r7, r10, r11, r12, r14	    
		    LDR    r2, [overlap, #32]      @t0 -overlap[8]
		    STR    r9, [overlap, #32]      
		    
		    LDR    r12, window_s_8_9
		    
		    LDR    r7, [overlap, #8]      @t0 -overlap[2] @sample[2][sb]
		    STR    r8, [overlap, #8] 
		    
		    @x_tmp[12] - r1  x_tmp[13] - r6
        @r0-overlap, r3-sample
        @r4-tt7,  r5-tt5		r2   r7
		    @r8, r9, r10, r11, r12, r14	   
		    SMLAWB   r8, r2, r12, r2
		    SMLAWT   r2, r2, r12, r2
		    SMLAWT   r9, r4, r12, r4
		    SMLAWB   r4, r4, r12, r4
		    ADD      r8, r8, r9          @sample[8][sb]
		    SUB      r2, r2, r4   
		    RSBNE    r2, r2, #0x0        @sample[9][sb]
		    @LDR      r12, window_s_2_15		   
		    STR      r8, [sample, #1024]
		    STR      r2, [sample, #1152]
		    		    
		    RSBNE    r5, r5, #0x0    @sample[15][sb]
		    STR      r7, [sample, #256]
		    STR      r5, [sample, #1920]
		    		    		    
		    @x_tmp[12] - r1  x_tmp[13] - r6
        @r0-overlap, r3-sample
        @r2, r4, r5, r7, r8, r9, r10, r11, r12, r14	  
        LDR      r4,   [sp], #4               @x_tmp[14] - tt2
        LDR      r5,   [sp], #4               @x_tmp[15] - tt3
        LDR      r7,   [sp], #4               @x_tmp[16] - tt4
        LDR      r8,   [sp], #4               @x_tmp[17] - tt5
        LDR      r9, c00_1
		    
		    ADD      r4, r4, r7        @tt2 
		    SUB      r7, r4, r7, lsl#1  
		    ADD      r5, r5, r8         @tt3
		    SUB      r8, r5, r8, lsl#1 
		    SMLAWB   r7, r7, r9, r7     @tt6
		    SMLAWB   r8, r8, r9, r8     @tt7
		    
		    ADD      r2,   r1, r4
		    ADD      r10,  r6, r5
		    
		    LDR      r12, window_s_6_11
		    
		    LDR      r11, [overlap, #24]        @t0
		    STR      r2,  [overlap, #24]
		    
		    SMLAWB   r2, r11, r12, r11
		    SMULWT   r11, r11, r12
		    SMLAWT   r2, r10, r12, r2         @sample[6][sb]
		    SMLAWB   r10, r10, r12, r10		   
		    SUB      r11, r11, r10
		    RSBNE    r11, r11, #0x0            @sample[11][sb]
		    STR      r2,  [sample, #768]
		    STR      r11, [sample, #1408]
		    
		    ADD     r2, r5, r7, lsl#1     @tt4
		    RSB     r5, r5, r7, lsl#1     @tt5
		    ADD     r7, r4, r8, lsl#1     @tt6
		    RSB     r4, r4, r8, lsl#1     @tt7
		    
		    ADD     r4, r1, r4, asr#1     @tt7
		    RSB     r2, r6, r2, asr#1     @tt4
		    SUB     r7, r1, r7, asr#1     @tt6
		    ADD     r5, r6, r5, asr#1     @tt5
		    
		    LDR     r1, [overlap]           @t0-overlap[0] @sample[0][sb]
		    STR     r2, [overlap], #20
		        
		    
		    LDR     r6, [overlap]            @t0-overlap[5]
		   	STR     r5,  [overlap], #16
		   			   	
		   	RSBEQ    r4, r4, #0           @sample[17][sb]		   			   	
		   	STR      r4, [sample, #2176]
		   	STR      r1, [sample], #640
		   			   	
		   	RSBNE    r6, r6, #0x0
		   	RSB      r7, r7, #0
		   	STR      r6, [sample], #896           @sample[5][sb]
		   	STR      r7, [sample], #-1532           @sample[12][sb]
		   	
		   	STR    r0,  [sp, #12]   @@// overlap
		   	STR    r3,  [sp, #16]   @@// sample
		   	LDR    r0,  [sp], #4
		   	LDR    r3,	[sp], #4	   
        LDR    r12, [sp], #4        
        ADD    r12, r12, #1
        B			 START_LOOP_1
                
OUT_LOOP_1:  
				ADD			 sp, sp, #0x8
        LDMFD    sp!,{r4-r12,pc}
window_s_4_13:
        .word      0x0
window_s_1_16:
        .word      0x0
window_s_7_10:
        .word      0x61f8ec83
window_s_3_14:
        .word      0x0
window_s_8_9:
        .word      0x9bd8cb19
window_s_2_15:
        .word      0x0
window_s_6_11:
        .word      0x216afdcf
window_s_0_17:
        .word      0x0
window_s_5_12:
        .word      0x0
        @ENDP
 
@--------------------------------------------------------------------
@   Function:
@       void MP3_DEC_AliasReduce(int32 xr[576], int lines)
@
@   Description: Perform frequency line alias reduction.
@
@   Register usage:
@       r0 = xr
@       r1 = lines
@--------------------------------------------------------------------		
		
MP3_DEC_AliasReduce:
        STMFD    sp!,{r4-r8,r12,lr}
        ADD      r7,r0,r1,LSL #2    @bound                           
        ADD      r0,r0,#0x48       @xr[18]
        CMP      r0,r7
        LDMCSFD  sp!,{r4-r8,r12,pc}
        LDR      r8,=cs_ca
        ADD      lr, r8, #32   @cs_ca[8]
        
        @xr[i] - r0 xr[-1-i] - r1  bound- r7  cs_ca_r8 cs_ca[8]-lr 
L1_10564:
        SUB			 r1, r0, #4       @xr[17]
        
        LDR			 r2, [r1],#-4      @a xr[-1-i]
				LDR			 r3, [r0],#4       @b xr[ i]                     
        LDR      r6,[r8], #4  @ cs_ca 
        ORRS     r4,r2,r3        
        BEQ      L1_10620        
              
        SMLAWT   r4,r2,r6, r2  @lo
        SMLAWT   r5,r3,r6, r3  @hi
        RSB      r2,r2,#0  @-a
        SMLAWB   r4,r3,r6,r4  @lo        
        SMLAWB   r5,r2,r6,r5  @hi
        ADD      r4,r4,r3  @lo        
        ADD      r5,r5,r2  @hi        
        STR      r4,[r1, #4]
        STR      r5,[r0, #-4]     
                
L1_10620:
        LDR			 r2, [r1],#-4       @a xr[-1-i]
				LDR			 r3, [r0],#4       @b xr[ i]      
				LDR      r6,[r8], #4  @ cs_ca 
        ORRS     r4,r2,r3        
        BEQ      L1_10680      
        
        SMLAWT   r4,r2,r6,r2   @lo
        SMLAWT   r5,r3,r6,r3   @hi
        SMLAWB   r4,r3,r6,r4   @lo1
        SMULWB   r2,r2,r6   @hi1
        STR      r4,[r1,#4]
        RSB      r2,r2,r5
        STR      r2,[r0,#-4]        
L1_10680:
        CMP      r8, lr
        BLT      L1_10620  
        
        ADD      r0, r0, #40     
        SUB		 r8, r8, #32  
        CMP      r0,r7
        BCC      L1_10564        
        LDMFD    sp!,{r4-r8,r12,pc}
        @ENDP				
      

        .global MP3_DEC_ImdctWinOverlap1          
        .global MP3_DEC_ImdctWinOverlap
        .global MP3_DEC_AliasImdctWinOverlap
        .global  MP3_DEC_AliasReduce   
        
        .extern  cs_ca  
       
                
        
        .end@END