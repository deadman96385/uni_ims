@// AAC-LC stream parsing
@// author: reed.zhang
@// input: SBR synthesis filter bank processing
@//     r0: output sample pointer
@// r1: the input overlap
@// r2: the tablr for output data
@// r3: the table for overlapping  
@// purpose:
	
                @ ***** END LICENSE BLOCK *****  

	.text@AREA	|.text|, CODE, READONLY
	.arm
IQ_out_ptr          .req    r0
ld_ptr              .req    r1
stream_ptr          .req    r1
scalefac_ptr        .req    r2
sect_sfb_offset_ptr .req    r3
AAC_SFB_COUNT       .req    r14
codeword            .req    r12
bit_info            .req    r11    @@// 31:16,  total bit consumed,   15:8:  bit_left,   7:0,  exp
huf_table_ptr       .req    r10



bit_consumed        .req    r4     @@// bit consumed
sfb_frac            .req    r5
iq_table_ptr        .req    r3	 
                               
                                .extern  AAC_iq_power_TAB_F_n
                .extern  AAC_iq_table    
                .extern  AAC_iq_table16
                .extern  AAC_DEC_HuffCb11

                @//int32 AAC_DEC_HuffmanCB11Asm(
                @//                              int32                   *out_ptr,
                @//                              AAC_BIT_FIFO_FORMAT_T   *ld_ptr,
                @//                              int32                   *scalefac_ptr,
                @//                              int32                   *sect_sfb_offset_ptr
                @//                              )@
AAC_DEC_HuffmanCB11Asm:@   FUNCTION
                @ // save the value for the following calculation
                .global  AAC_DEC_HuffmanCB11Asm                
                stmfd   sp!, {r4-r12, r14}                   
                stmfd    sp!, {ld_ptr}                
                ldsh    r6,  [ld_ptr, #4]  
                mov     bit_info,   #0
                ldr     stream_ptr, [ld_ptr]
                cmp     r6,  #0
                addeq   stream_ptr,  stream_ptr,  #4
                moveq   r6,  #32                
                
                ldr     AAC_SFB_COUNT, [IQ_out_ptr]
                ldr     codeword, [stream_ptr], #4       
                rsbs    r7, r6, #32 
    @IF AAC_DEC_ASM_LITTLE_ENDIAN = 1      
        @@/* little endian */  
        EOR      r5,  codeword, codeword,  ROR #16
        MOV    r5,  r5,    LSR #8
        BIC       r5,  r5, #0xFF00
        EOR     codeword,  r5, codeword, ROR #8        
    @ENDIF    
                ldr     r4, [stream_ptr]
                mov     codeword, codeword, lsl r7
    @IF AAC_DEC_ASM_LITTLE_ENDIAN = 1      
        @@/* little endian */  
        EOR      r5,  r4, r4,  ROR #16
        MOV    r5,  r5,    LSR #8
        BIC       r5,  r5, #0xFF00
        EOR     r4,  r5, r4, ROR #8        
    @ENDIF
                rsbs    r7, r7, #32
                orr     codeword, codeword, r4, lsr  r7
                orr     bit_info, bit_info,  r7,  lsl #8          
                
                 ldr     huf_table_ptr, =AAC_DEC_HuffCb11                                  
                mov     bit_consumed,  #0
                @/* the register r4, r5, r6, r7,r8, r9, r10 are free */
AAC_DEC_HuffmanCB11Asm_SFB_COUNT_LOOP:                
                @@///////////////////////////////////////////
                ldrsh    r7,  [sect_sfb_offset_ptr], #2
                ldrsh    r5,  [sect_sfb_offset_ptr]                            
                ldrsh    r6,  [scalefac_ptr], #2                                
                stmfd    sp!, {r14, r2, r3}                
                sub      r14, r5,  r7                
                mov      r5,   r6,  asr  #2
                and      r8,   r6,  #0x3  
                ldr      r6, =AAC_iq_power_TAB_F_n
                subs     r5,   r5,  #32
                mov      bit_info,  bit_info,  lsr #8
                mov      bit_info, bit_info, lsl #8
                orrle    bit_info, bit_info, #0x80
                rsble    r5,  r5,  #0
                add      bit_info,  bit_info,  r5
                
                add      sfb_frac,  r6,   r8, lsl #2
                ldr      sfb_frac, [sfb_frac]
                
                
                @/* the register r6, r7, r8, r9 are free */
AAC_DEC_HuffmanCB11Asm_SFB_WIDTH_LOOP:                
                @/* huffman parsing */                
                mov     r6,  codeword,  lsl   bit_consumed
                mov     r6,  r6, lsr  #24   @@// first get 8 bit
                add     r9,  huf_table_ptr,   r6,  lsl  #1
                ldrsh   r8,  [r9]                
                add     bit_consumed,  bit_consumed,   #1
    @IF AAC_DEC_ASM_LITTLE_ENDIAN = 1      
                @@/* little endian */  
                tst     r8,    #0x1
    @ELSE
    @            tst     r8,    #0x8000
    @ENDIF
                bne     CB11_HUFFMAN_PARSING
                @@//////////////////////////////                
                add     bit_consumed,  bit_consumed,  #7     
    @IF AAC_DEC_ASM_LITTLE_ENDIAN = 1      
                @@/* little endian */  
                and     r7,   r8,   #0xF
                mov    r7,  r7,  lsr  #1

                mov     r8,   r8,  lsr  #4
                add     r9,   huf_table_ptr,  r8, lsl  #1
    @ELSE
    @            mov     r7,   r8,  lsr  #12
    @            mov     r8,   r8,  lsl  #20
    @            add     r9,   huf_table_ptr,  r8, lsr  #19   
    @ENDIF
                mov     r6,  codeword,  lsl   bit_consumed
                rsb     r7,  r7,  #32
                mov     r8,  r6, lsr   r7                
                add     r9,  r9,  r8, lsl  #1                
                ldrsh   r8,  [r9]                 
                add     bit_consumed,  bit_consumed,   #1
CB11_HUFFMAN_PARSING: 

    @IF AAC_DEC_ASM_LITTLE_ENDIAN = 1      
                @@/* little endian */  
                and     r9,  r8,    #0xF
                add     bit_consumed,  bit_consumed,   r9,  lsr  #1  
                mov     r8,  r8,   lsr   #4
    @ELSE
    @            mov     r9,  r8,   lsl   #17
    @            add     bit_consumed,  bit_consumed,   r9,  lsr  #29   
    @            mov     r9,  r9,   lsl   #3
    @ENDIF                
                @// r7 record the sign                        
                @@/* get X and Y sign when the data is not zero!*/
                mov     r7,  codeword,  lsl  bit_consumed                          
                mov     r7,  r7,  lsr #30   @@// record the sign
                
    @IF AAC_DEC_ASM_LITTLE_ENDIAN = 1      
                @@/* little endian */  
                movs    r9,  r8,   lsr   #6     @@// get x, r9                       
    @ELSE
    @            movs    r9,  r9,   lsr   #26     @@// get x, r9                       
    @ENDIF
                addne   bit_consumed, bit_consumed,  #1  
                moveq   r7,  r7,  lsr #1   @// update the y sign 
                    
                ands    r8,  r8,   #0x3F         @@// get y: r8  bin(11 1111)                
                addne   bit_consumed, bit_consumed,  #1                
                @@/////////////////////////////////////////////////////
                @@////////////////////////////////                
                @@/* x sample */
                cmp     r9,    #16   @@// x is esp word?
                blt     CB11_XESP_WORD_EXIT                
                
                @@////////////////////////////////                
                cmp   bit_consumed,  #11
                ble   CB11_X_PRE_BIT_EXIT
                @@/* read bit start */
                add   bit_info,  bit_info,  bit_consumed,  lsl  #16                
                mov   codeword,  codeword,  lsl bit_consumed                
                mov   r2, bit_info,  lsr   #8
                and   r2,  r2,  #0xFF     @@// get the bit_left
                sub   bit_info,  bit_info,  r2, lsl #8                
                cmp   bit_consumed,  r2
                bgt   CB11_X_PRE_BIT_BRANCH
                @///////////////////////////
                ldr   r3, [stream_ptr]
                rsb   r2,  r2,  #32
    @IF AAC_DEC_ASM_LITTLE_ENDIAN = 1      
        @@/* little endian */  
        EOR      r6,  r3, r3,  ROR #16
        MOV    r6,  r6,    LSR #8
        BIC       r6,  r6, #0xFF00
        EOR     r3,  r6, r3, ROR #8        
    @ENDIF   
                mov   r3,  r3,   lsl   r2
                rsb   bit_consumed,  bit_consumed,  #32
                orr   codeword, codeword, r3,  lsr   bit_consumed                
                sub   r2,   bit_consumed,  r2
                orr   bit_info,  bit_info, r2,  lsl  #8  
                mov   bit_consumed,  #0              
                b     CB11_X_PRE_BIT_EXIT
CB11_X_PRE_BIT_BRANCH:     
                ldr   r3, [stream_ptr], #4
                sub   bit_consumed,  bit_consumed,  r2           
    @IF AAC_DEC_ASM_LITTLE_ENDIAN = 1      
        @@/* little endian */  
        EOR      r6,  r3, r3,  ROR #16
        MOV    r6,  r6,    LSR #8
        BIC       r6,  r6, #0xFF00
        EOR     r3,  r6, r3, ROR #8        
    @ENDIF    
                rsb   r2,  r2,  #32
                mov   r3,  r3,  lsl r2
                mov   r3,  r3,  lsr r2                               
                orr   codeword, codeword, r3,   lsl  bit_consumed
                ldr   r3,  [stream_ptr]
                rsb   r2,  bit_consumed,  #32
    @IF AAC_DEC_ASM_LITTLE_ENDIAN = 1      
        @@/* little endian */  
        EOR      r6,  r3, r3,  ROR #16
        MOV    r6,  r6,    LSR #8
        BIC       r6,  r6, #0xFF00
        EOR     r3,  r6, r3, ROR #8        
    @ENDIF   
                orr   bit_info,  bit_info,  r2,  lsl #8
                orr   codeword, codeword, r3,  lsr  r2         
                mov   bit_consumed,  #0                       
CB11_X_PRE_BIT_EXIT:                
                @@/* read bit end */
                @///////////////////////////
                @/* esp word dec */
                mov   r2,  codeword,  lsl bit_consumed
                rsbs  r2,  r2,  #0
                moveq r2,  #-1
                clz   r2,  r2                
                add   r2,  r2,  #1
                add   bit_consumed, bit_consumed, r2
                add   r2,  r2,  #3                                
                mov   r6, codeword,  lsl bit_consumed
                rsb   r3,  r2,  #32
                mov   r6, r6,  lsr r3
                add   bit_consumed, bit_consumed, r2
                mov   r3,  #1
                orr   r9,  r6,  r3,  lsl r2
                
                
CB11_XESP_WORD_EXIT: 
                
                
                @@/* y sample */
                cmp     r8,    #16   @@// y is esp word?
                blt     CB11_YESP_WORD_EXIT                                
                
                cmp   bit_consumed,  #11
                ble   CB11_Y_PRE_BIT_EXIT
                @@/* read bit */
                add   bit_info,  bit_info,  bit_consumed,  lsl  #16                
                mov   codeword,  codeword,  lsl bit_consumed             
                

                mov   r2, bit_info,  lsr   #8
                and   r2,  r2,  #0xFF     @@// get the bit_left
                sub   bit_info,  bit_info,  r2, lsl #8
                cmp   bit_consumed,  r2
                bgt   CB11_Y_PRE_BIT_BRANCH
                @///////////////////////////
                ldr   r3, [stream_ptr]
                rsb   r2,  r2,  #32
    @IF AAC_DEC_ASM_LITTLE_ENDIAN = 1      
        @@/* little endian */  
        EOR      r6,  r3, r3,  ROR #16
        MOV    r6,  r6,    LSR #8
        BIC       r6,  r6, #0xFF00
        EOR     r3,  r6, r3, ROR #8        
    @ENDIF   
                mov   r3,  r3,   lsl   r2
                rsb   bit_consumed,  bit_consumed,  #32
                orr   codeword, codeword, r3,  lsr   bit_consumed                
                sub   r2,   bit_consumed,  r2
                orr   bit_info,  bit_info, r2,  lsl  #8      
                mov   bit_consumed,  #0          
                b     CB11_Y_PRE_BIT_EXIT
CB11_Y_PRE_BIT_BRANCH:     
                ldr   r3, [stream_ptr], #4
                sub   bit_consumed,  bit_consumed,  r2           
    @IF AAC_DEC_ASM_LITTLE_ENDIAN = 1      
        @@/* little endian */  
        EOR      r6,  r3, r3,  ROR #16
        MOV    r6,  r6,    LSR #8
        BIC       r6,  r6, #0xFF00
        EOR     r3,  r6, r3, ROR #8        
    @ENDIF   
                rsb   r2,  r2,  #32
                mov   r3,  r3,  lsl r2
                mov   r3,  r3,  lsr r2                               
                orr   codeword, codeword, r3,   lsl  bit_consumed
                ldr   r3,  [stream_ptr]
                rsb   r2,  bit_consumed,  #32
    @IF AAC_DEC_ASM_LITTLE_ENDIAN = 1      
        @@/* little endian */  
        EOR      r6,  r3, r3,  ROR #16
        MOV    r6,  r6,    LSR #8
        BIC       r6,  r6, #0xFF00
        EOR     r3,  r6, r3, ROR #8        
    @ENDIF   
                orr   bit_info,  bit_info,  r2,  lsl #8
                orr   codeword, codeword, r3,  lsr  r2  
                
                mov   bit_consumed,  #0
CB11_Y_PRE_BIT_EXIT:                
                @///////////////////////////                
                mov   r2, codeword, lsl  bit_consumed
                rsbs  r2,  r2,  #0
                moveq r2,  #-1
                clz   r2,  r2
                add   r2,  r2,  #1
                add   bit_consumed, bit_consumed, r2
                add   r2,  r2,  #3                
                mov   r6, codeword,  lsl bit_consumed
                rsb   r3,  r2,  #32
                mov   r6, r6,  lsr r3
                add   bit_consumed, bit_consumed, r2
                mov   r3,  #1
                orr   r8,  r6,  r3,  lsl r2
                
CB11_YESP_WORD_EXIT:                
                @//////////////////////////////////////////////                
                @@////////////////////////////////                 
                @@@/* IQ--left channel */
                @@//  r2, r3, r6, r9, r7    	                                            
    	        movs    r9,   r9     	                      
    	        streq   r9,   [IQ_out_ptr], #4                              
    	        beq     long_width_LOOP_SECOND_POINT    	                  
    	                                                                    
    	        ldr     iq_table_ptr,  =AAC_iq_table                        
    	        @@//   4/3 de-quantization                                  
    	        cmp     r9,  #256                                           
    	        addlt   r9,  iq_table_ptr,  r9, lsl #2                      
    	        ldrlt   r9,  [r9]     
    	        and     r6,  bit_info,  #0x7F                                      
    	        movlt   r9,  r9,  lsl #2                                    
    	        blt     long_width_LOOP_FIRST_POINT_SCALE                   
    	                                                                    
    	        cmp     r9,  #2048                                          
    	        bge     long_width_LOOP_FIRST_POINTGE2048                   
    	        @@/* < 2048 */                                              
    	        and     r6,    r9,  #0x7                                    
    	        mov     r2,    r9,  lsr  #3                                 
    	        add     r3,    iq_table_ptr,  r2,  lsl   #2                 
    	        ldmia   r3, {r2, r3}   @// r2: x1, r3: x2                   
   	                                                                      
    	        cmp     r6,    #4                                           
    	        ble     long_width_LOOP_FIRST_POINTLT2048LE4                
    	        sub     r6,    r6,  #8   @@// r6: t - 8                     
    	        sub     r2,    r3,  r2   @@// x2 - x1                       
    	        mov     r2,    r2,  lsl   #3                                
    	        add     r2,    r2,  #181                                    
    	        sub     r2,    r2,  r9,   lsr #4  @@// r2: x2 - x1 - fx     
    	        mul     r2,    r6,  r2                                      
    	        add     r9,    r2,  r3, lsl  #6
    	        and   r6,  bit_info,  #0x7F    	            	          
    	        B       long_width_LOOP_FIRST_POINT_SCALE                   
long_width_LOOP_FIRST_POINTLT2048LE4:                                      
    	        sub     r3,    r3,  r2   @@// x2 - x1                       
    	        mov     r3,    r3,  lsl   #3                                
    	        sub     r3,    r3,  #181                                    
    	        add     r3,    r3,  r9,   lsr #4  @@// r2: x2 - x1 - fx     
    	        mul     r3,    r6,  r3                                      
    	        add     r9,    r3,  r2, lsl  #6    
    	        
    	        and   r6,  bit_info,  #0x7F	                        
    	        B       long_width_LOOP_FIRST_POINT_SCALE                   
long_width_LOOP_FIRST_POINTGE2048:                                         
    	        @@/* >= 2048*/                                              
    	        and     r6,    r9,  #0x3F                                   
    	        mov     r2,    r9,  lsr  #6                                 
    	        add     r3,    iq_table_ptr,  r2,  lsl   #2                 
    	        ldmia   r3, {r2, r3}   @// r2: x1, r3: x2                   
   	                                                                      
    	        cmp     r6,    #32                                          
    	        ble     long_width_LOOP_FIRST_POINTGE2048LE32               
    	        sub     r6,    r6,  #64   @@// r6: t - 64                   
    	        sub     r2,    r3,  r2   @@// x2 - x1                       
    	        mov     r2,    r2,  lsl   #4                                
    	        add     r2,    r2,  #420                                    
    	        sub     r2,    r2,  r9,   lsr #5  @@// r2: x2 - x1 - fx     
    	        mul     r2,    r6,  r2                                      
    	        add     r9,    r2,  r3, lsl  #10   	      
    	        
    	        and   r6,  bit_info,  #0x7F      	          
    	        B       long_width_LOOP_FIRST_POINT_SCALE                   
long_width_LOOP_FIRST_POINTGE2048LE32:                                     
    	        sub     r3,    r3,  r2   @@// x2 - x1                       
    	        mov     r3,    r3,  lsl   #4                                
    	        sub     r3,    r3,  #420                                    
    	        add     r3,    r3,  r9,   lsr #5  @@// r2: x2 - x1 - fx     
    	        mul     r3,    r6,  r3                                      
    	        add     r9,    r3,  r2, lsl  #10    
    	        
    	        and   r6,  bit_info,  #0x7F	            	        
long_width_LOOP_FIRST_POINT_SCALE:                                         
    	        @@/* scale */                                               
    	        subs   r3,  r7,  #2
                rsbge  r9,  r9,  #0                                          
    	        smull  r9,  r3,  sfb_frac,  r9

    	        tst   bit_info,  #0x80
    	        @//and   r6,  bit_info,  #0x7F
    	        moveq r3,    r3,  lsl   r6
    	        movne r3,    r3,  asr   r6
    	            	        
    	        str     r3,    [IQ_out_ptr], #4                         
long_width_LOOP_SECOND_POINT:   
                 	

                @@@/* IQ--right channel */
                @@//  r6, r2, r3, r8, r10
    	        movs    r8,   r8
    	        streq   r8,   [IQ_out_ptr], #4
    	        beq     long_width_LOOP_NEXT_STEP
    	        ldr     iq_table_ptr,  =AAC_iq_table                    
    	        @@//   4/3 de-quantization
    	        cmp     r8,  #256
    	        addlt   r8,  iq_table_ptr,  r8, lsl #2
    	        ldrlt   r8,  [r8]
    	        and     r9,  bit_info,  #0x7F
    	        movlt   r8,  r8,  lsl #2
    	        blt     long_width_LOOP_SECOND_POINT_SCALE
    	        
    	        cmp     r8,  #2048
    	        bge     long_width_LOOP_SECOND_POINTGE2048
    	        @@/* < 2048 */
    	        and     r6,    r8,  #0x7
    	        mov     r2,    r8,  lsr  #3
    	        add     r3,    iq_table_ptr,  r2,  lsl   #2
    	        ldmia   r3, {r2, r3}   @// r2: x1, r3: x2
   	        
    	        cmp     r6,    #4
    	        ble     long_width_LOOP_SECOND_POINTLT2048LE4
    	        sub     r6,    r6,  #8   @@// r6: t - 8
    	        sub     r2,    r3,  r2   @@// x2 - x1
    	        mov     r2,    r2,  lsl   #3
    	        add     r2,    r2,  #181
    	        sub     r2,    r2,  r8,   lsr #4  @@// r2: x2 - x1 - fx
    	        mul     r2,    r6,  r2
    	        add     r8,    r2,  r3, lsl  #6   
    	        
    	        and     r9,  bit_info,  #0x7F 	            	        
    	        B       long_width_LOOP_SECOND_POINT_SCALE
long_width_LOOP_SECOND_POINTLT2048LE4:
    	        sub     r3,    r3,  r2   @@// x2 - x1
    	        mov     r3,    r3,  lsl   #3
    	        sub     r3,    r3,  #181
    	        add     r3,    r3,  r8,   lsr #4  @@// r2: x2 - x1 - fx
    	        mul     r3,    r6,  r3
    	        add     r8,    r3,  r2, lsl  #6    	   
    	        
    	        and     r9,  bit_info,  #0x7F      
    	        B       long_width_LOOP_SECOND_POINT_SCALE
long_width_LOOP_SECOND_POINTGE2048:
    	        @@/* >= 2048*/
    	        and     r6,    r8,  #0x3F
    	        mov     r2,    r8,  lsr  #6
    	        add     r3,    iq_table_ptr,  r2,  lsl   #2
    	        ldmia   r3, {r2, r3}   @// r2: x1, r3: x2
   	        
    	        cmp     r6,    #32
    	        ble     long_width_LOOP_SECOND_POINTGE2048LE32
    	        sub     r6,    r6,  #64   @@// r6: t - 64
    	        sub     r2,    r3,  r2   @@// x2 - x1
    	        mov     r2,    r2,  lsl   #4
    	        add     r2,    r2,  #420
    	        sub     r2,    r2,  r8,   lsr #5  @@// r2: x2 - x1 - fx
    	        mul     r2,    r6,  r2
    	        add     r8,    r2,  r3, lsl  #10   	          
    	        
    	        and     r9,  bit_info,  #0x7F   	        
    	        B       long_width_LOOP_SECOND_POINT_SCALE
long_width_LOOP_SECOND_POINTGE2048LE32:
    	        sub     r3,    r3,  r2   @@// x2 - x1
    	        mov     r3,    r3,  lsl   #4
    	        sub     r3,    r3,  #420
    	        add     r3,    r3,  r8,   lsr #5  @@// r2: x2 - x1 - fx
    	        mul     r3,    r6,  r3
    	        add     r8,    r3,  r2, lsl  #10   
    	        
    	        and     r9,  bit_info,  #0x7F  	            	        
long_width_LOOP_SECOND_POINT_SCALE:
    	        @@/* scale */   
    	        ands    r7,  r7,  #0x1
    	        rsbne   r8,  r8,  #0 	            	        
    	        smull   r8,    r3,    sfb_frac,  r8
    	        
    	        tst   bit_info,  #0x80
    	        @//and   r9,  bit_info,  #0x7F
    	        moveq r3,    r3,  lsl   r9
    	        movne r3,    r3,  asr   r9
    	        
    	        
    	          	        
    	        str     r3,    [IQ_out_ptr], #4  
long_width_LOOP_NEXT_STEP:
                
                @//////////////////////////////////////////////
                @/* read bit */                
                cmp   bit_consumed,  #18
                ble   CB11_LOOP_PRE_BIT_EXIT
                @@/* read bit */
                add   bit_info,  bit_info,  bit_consumed,  lsl  #16                
                mov   codeword,  codeword,  lsl bit_consumed

                mov   r2, bit_info,  lsr   #8
                and   r2,  r2,  #0xFF     @@// get the bit_left
                sub   bit_info,  bit_info,  r2, lsl #8
                cmp   bit_consumed,  r2
                bgt   CB11_LOOP_PRE_BIT_BRANCH
                @///////////////////////////
                ldr   r3, [stream_ptr]
                rsb   r2,  r2,  #32    
    @IF AAC_DEC_ASM_LITTLE_ENDIAN = 1      
        @@/* little endian */  
        EOR      r6,  r3, r3,  ROR #16
        MOV    r6,  r6,    LSR #8
        BIC       r6,  r6, #0xFF00
        EOR     r3,  r6, r3, ROR #8        
    @ENDIF   
                mov   r3,  r3,   lsl   r2
                rsb   bit_consumed,  bit_consumed,  #32
                orr   codeword, codeword, r3,  lsr   bit_consumed                
                sub   r2,   bit_consumed,  r2
                orr   bit_info,  bit_info, r2,  lsl  #8  
                mov   bit_consumed,  #0              
                b     CB11_LOOP_PRE_BIT_EXIT
CB11_LOOP_PRE_BIT_BRANCH:     
                ldr   r3, [stream_ptr], #4
                sub   bit_consumed,  bit_consumed,  r2        
    @IF AAC_DEC_ASM_LITTLE_ENDIAN = 1      
        @@/* little endian */  
        EOR      r6,  r3, r3,  ROR #16
        MOV    r6,  r6,    LSR #8
        BIC       r6,  r6, #0xFF00
        EOR     r3,  r6, r3, ROR #8        
    @ENDIF   
   
                rsb   r2,  r2,  #32
                mov   r3,  r3,  lsl r2
                mov   r3,  r3,  lsr r2                               
                orr   codeword, codeword, r3,   lsl  bit_consumed
                ldr   r3,  [stream_ptr]
                rsb   r2,  bit_consumed,  #32
    @IF AAC_DEC_ASM_LITTLE_ENDIAN = 1      
        @@/* little endian */  
        EOR      r6,  r3, r3,  ROR #16
        MOV    r6,  r6,    LSR #8
        BIC       r6,  r6, #0xFF00
        EOR     r3,  r6, r3, ROR #8        
    @ENDIF   
                orr   bit_info,  bit_info,  r2,  lsl #8
                orr   codeword, codeword, r3,  lsr  r2   
                mov   bit_consumed,  #0                              
CB11_LOOP_PRE_BIT_EXIT:                
                @///////////////////////////     
                                
                
                @@///////////////////////////////////////////  
                subs   r14, r14, #2
                BGT    AAC_DEC_HuffmanCB11Asm_SFB_WIDTH_LOOP           
                
                
                
                
                ldmfd   sp!, {r14, r2, r3}
                
                @@///////////////////////////////////////////  
                subs   AAC_SFB_COUNT, AAC_SFB_COUNT, #1
                BGT    AAC_DEC_HuffmanCB11Asm_SFB_COUNT_LOOP
                @@///////////////////////////////////////////
                
                ldmfd   sp!, {ld_ptr}
                
                
                @@/* bit buffer update */
                add   bit_consumed, bit_consumed, bit_info, lsr #16
                ldrsh r7, [ld_ptr, #4]   @@// left bit
                ldrsh r9, [ld_ptr, #6]   @@// byte used
                
                cmp    bit_consumed, r7
                suble  r7,  r7, bit_consumed
                strleh r7, [ld_ptr, #4]
                ble   AAC_DEC_HuffmanCB11Asm_EXIT
                @/**/
                ldr    r10,   [ld_ptr]                
                sub   bit_consumed,  bit_consumed,  r7
                
                mov   r8,  bit_consumed,  lsr #5
                add   r8,  r8,  #1
                add   r10,  r10,  r8,    lsl #2                
                and   r7,  bit_consumed,  #0x1F
                rsb   r7,  r7,  #32
                
                add   r9,  r9,  r8,    lsl #2
                str   r10, [ld_ptr]
                strh  r7, [ld_ptr, #4]
                strh  r9, [ld_ptr, #6]
                
                
AAC_DEC_HuffmanCB11Asm_EXIT:                
                ldmfd   sp!, {r4-r12, pc}
                @@ENDFUNC
                
                
                
                
                
                
                @//int32 AAC_DEC_HuffmanCB9Asm(
                @//                              int32                   *out_ptr,
                @//                              AAC_BIT_FIFO_FORMAT_T   *ld_ptr,
                @//                              int32                   *scalefac_ptr,
                @//                              int32                   *sect_sfb_offset_ptr
                @//                              )@
                

tmp_iq_table_ptr   .req  r2
tmp_iq_exp         .req  r7

AAC_DEC_HuffmanCB9Asm:@   FUNCTION
                @ // save the value for the following calculation
                .global  AAC_DEC_HuffmanCB9Asm
                .extern  AAC_DEC_HuffCb9
                
                stmfd   sp!, {r4-r12, r14}                               
                
                stmfd    sp!, {ld_ptr}                 
                ldsh    r6,  [ld_ptr, #4]  
                mov     bit_info,   #0
                ldr     stream_ptr, [ld_ptr]
                cmp     r6,  #0
                addeq   stream_ptr,  stream_ptr,  #4
                moveq   r6,  #32                
                ldr     AAC_SFB_COUNT, [IQ_out_ptr]
                ldr     codeword, [stream_ptr], #4               
                rsbs    r7, r6, #32
    @IF AAC_DEC_ASM_LITTLE_ENDIAN = 1      
        @@/* little endian */  
        EOR      r5,  codeword, codeword,  ROR #16
        MOV    r5,  r5,    LSR #8
        BIC       r5,  r5, #0xFF00
        EOR     codeword,  r5, codeword, ROR #8        
    @ENDIF 

                ldr     r4, [stream_ptr]
                mov     codeword, codeword, lsl r7             

    @IF AAC_DEC_ASM_LITTLE_ENDIAN = 1      
        @@/* little endian */  
        EOR      r5,  r4, r4,  ROR #16
        MOV    r5,  r5,    LSR #8
        BIC       r5,  r5, #0xFF00
        EOR     r4,  r5, r4, ROR #8        
    @ENDIF 
                rsbs    r7, r7, #32
                orr     codeword, codeword, r4, lsr  r7
                orr     bit_info, bit_info,  r7,  lsl #8  
                              
                ldr     huf_table_ptr, [IQ_out_ptr, #4]
                                 
                mov     bit_consumed,  #0
                @/* the register r4, r5, r6, r7,r8, r9, r10 are free */
                
                
AAC_DEC_HuffmanCB9Asm_SFB_COUNT_LOOP:                
                @@///////////////////////////////////////////
                ldrsh    r8,  [sect_sfb_offset_ptr], #2
                ldrsh    r5,  [sect_sfb_offset_ptr]                            
                ldrsh    r6,  [scalefac_ptr], #2
                                
                stmfd    sp!, {r14, r2, r3}                
                sub      r14, r5,  r8                
                mov      r5,   r6,  asr  #2
                and      r8,   r6,  #0x3  
                ldr      r6, =AAC_iq_power_TAB_F_n
                sub      tmp_iq_exp,   r5,  #32
                
                add      sfb_frac,  r6,   r8, lsl #2
                ldr      sfb_frac, [sfb_frac]     
                
                ldr     tmp_iq_table_ptr,  =AAC_iq_table16                   
                           
                @/* the register r6, r7, r8, r9 are free */                
AAC_DEC_HuffmanCB9Asm_SFB_WIDTH_LOOP:
                @/* huffman parsing */                
                mov     r3,  codeword,  lsl   bit_consumed
                mov     r3,  r3, lsr  #28   @@// first get 4 bit
                add     r9,  huf_table_ptr,   r3,  lsl  #1
                ldrsh   r8,  [r9]       
                mov     r9,  #4    
    @IF AAC_DEC_ASM_LITTLE_ENDIAN = 1      
                @@/* little endian */
                tst     r8,    #0x1
    @ELSE     
    @            tst     r8,    #0x8000
    @ENDIF
                bne     CB9_HUFFMAN_PARSING                
CB9_HUFFMAN_BIT_DEC_LOOP:               
                @@//////////////////////////////                
                add     bit_consumed,  bit_consumed,  r9
    @IF AAC_DEC_ASM_LITTLE_ENDIAN = 1      
                @@/* little endian */
                and     r9,  r8,   #0xF    @@// pre-read bit count
                mov    r9,  r9,   lsr  #1
                mov     r8,  r8, lsr  #4
                add     r8,  huf_table_ptr,  r8,lsl #1       
         
                rsb     r6,  r9,   #32
                mov     r3,  codeword,   lsl  bit_consumed
                mov     r3,  r3,   lsr r6
                add     r8,  r8,   r3,  lsl #1                          
                ldrsh   r8,  [r8]                
                tst     r8,    #0x1
     @ELSE
     @           mov     r9,  r8, lsr  #12    @@// pre-read bit count
     @           mov     r8,  r8, lsl  #20
     @           add     r8,  huf_table_ptr,  r8,lsr #19                
     @           rsb     r6,  r9,   #32
     @           mov     r3,  codeword,   lsl  bit_consumed
     @           mov     r3,  r3,   lsr r6
     @           add     r8,  r8,   r3,  lsl #1                          
     @           ldrsh   r8,  [r8]                
     @           tst     r8,    #0x8000
     @ENDIF
                beq     CB9_HUFFMAN_BIT_DEC_LOOP 
CB9_HUFFMAN_PARSING:

     @IF AAC_DEC_ASM_LITTLE_ENDIAN = 1      
                @@/* little endian */
                and     r9,  r8,   #0xF
                add     bit_consumed,  bit_consumed,   r9,  lsr  #1                     
                movs    r9,  r8,   lsr   #10    @@// get x, r9       
    @ELSE
    @            mov     r9,  r8,   lsl   #17
    @            add     bit_consumed,  bit_consumed,   r9,  lsr  #29
    @            mov     r9,  r9,   lsl   #3                
    @            movs    r9,  r9,   lsr   #26    @@// get x, r9       
    @ENDIF         
                @//////////////////////////////////////////////                 
                @@@/* IQ--left channel */
                @@//  r2, r3, r6, r9, r7                                        
    	        streq   r9,   [IQ_out_ptr], #4                              
    	        beq     CB9_long_width_LOOP_SECOND_POINT                  
    	        @@//   4/3 de-quantization    	                                                
    	        add   r9,  tmp_iq_table_ptr,  r9, lsl #2
    	        ldr   r9,  [r9]    	        
    	        mov     r6,  codeword,  lsl  bit_consumed
                add     bit_consumed, bit_consumed,  #1
                movs    r6,  r6,  asr #31   @@// get the sign    	        
    	        @@/* scale */    	        
                rsbne  r9,  r9,  #0                                                    
    	        smull  r9,  r3,  sfb_frac,  r9    	        
    	        @//mov     r6,  bit_info,  lsl #24
    	        @//movs    r9,  r6,  asr #24                              
    	        movs    r9,  tmp_iq_exp
    	        movge   r3,    r3,  lsl   r9                       
    	        rsblt   r9,    r9,  #0                             
    	        movlt   r3,    r3,  asr   r9    	            	                                                                    
    	        str     r3,    [IQ_out_ptr], #4
    	        
CB9_long_width_LOOP_SECOND_POINT:
     @IF AAC_DEC_ASM_LITTLE_ENDIAN = 1      
                @@/* little endian */
                mov    r8,  r8,  lsr #4
                ands    r8,  r8,   #0x3F         @@// get y: r8
     @ELSE
     @           ands    r8,  r8,   #0x3F         @@// get y: r8
     @ENDIF
                @@@/* IQ--right channel */
                @@//  r6, r2, r3, r8, r7    	        
    	        streq   r8,   [IQ_out_ptr], #4
    	        beq     CB9_long_width_LOOP_NEXT_STEP    	        
    	        @@//   4/3 de-quantization    	        
    	        add   r8,  tmp_iq_table_ptr,  r8, lsl #2
    	        ldr   r8,  [r8]    	           
    	        mov   r6,  codeword,  lsl  bit_consumed
                add   bit_consumed, bit_consumed,  #1                
                movs  r6, r6,  asr #31   @@// get the sign     	        
    	        @@/* scale */    	        
    	        rsbne   r8,  r8,  #0    	               	        
    	        smull   r8,    r3,    sfb_frac,  r8    	        
    	        @//mov     r9,  bit_info,  lsl #24
    	        @//movs    r9,  r9,  asr #24
    	        movs    r9,  tmp_iq_exp       
    	        movge   r3,    r3,  lsl   r9
    	        rsblt   r8,    r9,  #0
    	        movlt   r3,    r3,  asr   r8    	        
    	        str     r3,    [IQ_out_ptr], #4  
CB9_long_width_LOOP_NEXT_STEP:                
                @@/* IQ end */
                
                
                @@/////////////////////////////////////////////                
                @//////////////////////////////////////////////
                @/* read bit */                
                cmp   bit_consumed,  #14
                ble   CB9_LOOP_PRE_BIT_EXIT
                @@/* read bit */
                add   bit_info,  bit_info,  bit_consumed,  lsl  #16                
                mov   codeword,  codeword,  lsl bit_consumed

                mov   r8, bit_info,  lsr   #8
                and   r8,  r8,  #0xFF     @@// get the bit_left
                sub   bit_info,  bit_info,  r8, lsl #8
                cmp   bit_consumed,  r8
                bgt   CB9_LOOP_PRE_BIT_BRANCH
                @///////////////////////////
                ldr   r3, [stream_ptr]
                rsb   r8,  r8,  #32
    @IF AAC_DEC_ASM_LITTLE_ENDIAN = 1      
        @@/* little endian */  
        EOR      r6,  r3, r3,  ROR #16
        MOV    r6,  r6,    LSR #8
        BIC       r6,  r6, #0xFF00
        EOR     r3,  r6, r3, ROR #8        
    @ENDIF 
                mov   r3,  r3,   lsl   r8
                rsb   bit_consumed,  bit_consumed,  #32
                orr   codeword, codeword, r3,  lsr   bit_consumed                
                sub   r8,   bit_consumed,  r8
                orr   bit_info,  bit_info, r8,  lsl  #8  
                mov   bit_consumed,  #0              
                b     CB9_LOOP_PRE_BIT_EXIT
CB9_LOOP_PRE_BIT_BRANCH:     
                ldr   r3, [stream_ptr], #4
                sub   bit_consumed,  bit_consumed,  r8
    @IF AAC_DEC_ASM_LITTLE_ENDIAN = 1      
        @@/* little endian */  
        EOR      r6,  r3, r3,  ROR #16
        MOV    r6,  r6,    LSR #8
        BIC       r6,  r6, #0xFF00
        EOR     r3,  r6, r3, ROR #8        
    @ENDIF 
                rsb   r8,  r8,  #32
                mov   r3,  r3,  lsl r8
                mov   r3,  r3,  lsr r8                               
                orr   codeword, codeword, r3,   lsl  bit_consumed
                ldr   r3,  [stream_ptr]
                rsb   r8,  bit_consumed,  #32
    @IF AAC_DEC_ASM_LITTLE_ENDIAN = 1      
        @@/* little endian */  
        EOR      r6,  r3, r3,  ROR #16
        MOV    r6,  r6,    LSR #8
        BIC       r6,  r6, #0xFF00
        EOR     r3,  r6, r3, ROR #8        
    @ENDIF 
                orr   bit_info,  bit_info,  r8,  lsl #8
                orr   codeword, codeword, r3,  lsr  r8   
                mov   bit_consumed,  #0                              
CB9_LOOP_PRE_BIT_EXIT:                
                @///////////////////////////     
                                
                
                @@///////////////////////////////////////////  
                subs   r14, r14, #2
                BGT    AAC_DEC_HuffmanCB9Asm_SFB_WIDTH_LOOP           
                
                
                
                
                ldmfd   sp!, {r14, r2, r3}
                
                @@///////////////////////////////////////////  
                subs   AAC_SFB_COUNT, AAC_SFB_COUNT, #1
                BGT    AAC_DEC_HuffmanCB9Asm_SFB_COUNT_LOOP
                @@///////////////////////////////////////////
                
                
                
                
                
                
                
                
                
                
                
                
                
                
                
                @@/* end */                
                ldmfd   sp!, {ld_ptr}                
                @@/* bit buffer update */
                add   bit_consumed, bit_consumed, bit_info, lsr #16
                ldrsh r7, [ld_ptr, #4]   @@// left bit
                ldrsh r9, [ld_ptr, #6]   @@// byte used                
                cmp    bit_consumed, r7
                suble  r7,  r7, bit_consumed
                strleh r7, [ld_ptr, #4]
                ble   AAC_DEC_HuffmanCB9Asm_EXIT
                @/**/
                ldr    r10,   [ld_ptr]                
                sub   bit_consumed,  bit_consumed,  r7                
                mov   r8,  bit_consumed,  lsr #5
                add   r8,  r8,  #1
                add   r10,  r10,  r8,    lsl #2                
                and   r7,  bit_consumed,  #0x1F
                rsb   r7,  r7,  #32                
                add   r9,  r9,  r8,    lsl #2
                str   r10, [ld_ptr]
                strh  r7, [ld_ptr, #4]
                strh  r9, [ld_ptr, #6]                
AAC_DEC_HuffmanCB9Asm_EXIT:                
                ldmfd   sp!, {r4-r12, pc}
                @ENDFUNC
                
                
                
                
                
                               
                
                
                
                
                @//int32 AAC_DEC_HuffmanCB34Asm(
                @//                              int32                   *out_ptr,
                @//                              AAC_BIT_FIFO_FORMAT_T   *ld_ptr,
                @//                              int32                   *scalefac_ptr,
                @//                              int32                   *sect_sfb_offset_ptr
                @//                              )@
                

tmp_value1_iq   .req  r7
tmp_value2_iq   .req  r2

AAC_DEC_HuffmanCB34Asm:@   FUNCTION
                @ // save the value for the following calculation
                .global  AAC_DEC_HuffmanCB34Asm                
                
                stmfd   sp!, {r4-r12, r14}                               
                
                stmfd    sp!, {ld_ptr}                 
                ldsh    r6,  [ld_ptr, #4]  
                mov     bit_info,   #0
                ldr     stream_ptr, [ld_ptr]
                cmp     r6,  #0
                addeq   stream_ptr,  stream_ptr,  #4
                moveq   r6,  #32                
                ldr     AAC_SFB_COUNT, [IQ_out_ptr]
                ldr     codeword, [stream_ptr], #4               
                rsbs    r7, r6, #32
    @IF AAC_DEC_ASM_LITTLE_ENDIAN = 1      
        @@/* little endian */  
        EOR      r5,  codeword, codeword,  ROR #16
        MOV    r5,  r5,    LSR #8
        BIC       r5,  r5, #0xFF00
        EOR     codeword,  r5, codeword, ROR #8        
    @ENDIF 

                ldr     r4, [stream_ptr]
                mov     codeword, codeword, lsl r7    
    @IF AAC_DEC_ASM_LITTLE_ENDIAN = 1      
        @@/* little endian */  
        EOR      r5,  r4, r4,  ROR #16
        MOV    r5,  r5,    LSR #8
        BIC       r5,  r5, #0xFF00
        EOR     r4,  r5, r4, ROR #8        
    @ENDIF             
                rsbs    r7, r7, #32
                orr     codeword, codeword, r4, lsr  r7
                orr     bit_info, bit_info,  r7,  lsl #8                
                
                ldr     huf_table_ptr, [IQ_out_ptr, #4]    
                             
                mov     bit_consumed,  #0
                @/* the register r4, r5, r6, r7,r8, r9, r10 are free */
                
                
AAC_DEC_HuffmanCB4Asm_SFB_COUNT_LOOP:                
                @@///////////////////////////////////////////
                ldrsh    r8,  [sect_sfb_offset_ptr], #2
                ldrsh    r5,  [sect_sfb_offset_ptr]                            
                ldrsh    r6,  [scalefac_ptr], #2
                                
                stmfd    sp!, {r14, r2, r3}                
                sub      r14, r5,  r8                
                mov      r9,   r6,  asr  #2
                and      r8,   r6,  #0x3  
                ldr      r6, =AAC_iq_power_TAB_F_n              
                
                add      sfb_frac,  r6,   r8, lsl #2
                ldr      sfb_frac, [sfb_frac]     
                
                mov      tmp_value1_iq,  #2048*4                
                smull    r8, tmp_value1_iq, sfb_frac, tmp_value1_iq 
                mov      tmp_value2_iq,  #20480
                add      tmp_value2_iq,  tmp_value2_iq,  #164
                smull    r8, tmp_value2_iq, sfb_frac, tmp_value2_iq 
                
                subs     r9,   r9,  #32  @@// r9->exp
                movge    tmp_value1_iq,  tmp_value1_iq,  lsl  r9
                movge    tmp_value2_iq,  tmp_value2_iq,  lsl  r9
                                
                rsblt    r9,  r9,  #0
                movlt    tmp_value1_iq,  tmp_value1_iq,  asr  r9
                movlt    tmp_value2_iq,  tmp_value2_iq,  asr  r9
                           
                @/* the register r6, r7, r8, r9 are free */                
AAC_DEC_HuffmanCB4Asm_SFB_WIDTH_LOOP:
                @/* huffman parsing */                
                mov     r3,  codeword,  lsl   bit_consumed
                mov     r3,  r3, lsr  #27   @@// first get 5 bit
                add     r9,  huf_table_ptr,   r3,  lsl  #1
                ldrsh   r8,  [r9]       
                mov     r9,  #5        
    @IF AAC_DEC_ASM_LITTLE_ENDIAN = 1      
                 @@/* little endian */  
                tst     r8,    #0x1
    @ELSE
    @            tst     r8,    #0x8000
    @ENDIF
                bne     CB4_HUFFMAN_PARSING                
CB4_HUFFMAN_BIT_DEC_LOOP:                
                @@//////////////////////////////                
                add     bit_consumed,  bit_consumed,  r9
    @IF AAC_DEC_ASM_LITTLE_ENDIAN = 1      
                 @@/* little endian */  
                and      r9,  r8,   #0xF    @@// pre-read bit count
                mov     r9,  r9,  lsr  #1
                mov     r8,  r8, lsr  #4
                add     r8,  huf_table_ptr,  r8,lsl #1
                rsb     r6,  r9,   #32
                mov     r3,  codeword,   lsl  bit_consumed
                mov     r3,  r3,   lsr r6
                add     r8,  r8,   r3,  lsl #1                          
                ldrsh   r8,  [r8]                
                tst     r8,    #0x1
    @ELSE
    @            mov     r9,  r8, lsr  #12    @@// pre-read bit count
    @            mov     r8,  r8, lsl  #20
    @            add     r8,  huf_table_ptr,  r8,lsr #19                
    @            rsb     r6,  r9,   #32
    @            mov     r3,  codeword,   lsl  bit_consumed
    @            mov     r3,  r3,   lsr r6
    @            add     r8,  r8,   r3,  lsl #1                          
    @            ldrsh   r8,  [r8]                
    @            tst     r8,    #0x8000
    @ENDIF

                beq     CB4_HUFFMAN_BIT_DEC_LOOP 
CB4_HUFFMAN_PARSING:                

    @IF AAC_DEC_ASM_LITTLE_ENDIAN = 1      
                 @@/* little endian */  
                and     r9,  r8,    #0xF
                add     bit_consumed,  bit_consumed,   r9,  lsr  #1
                mov    r8,   r8,    lsr  #4
    @ELSE
    @            mov     r9,  r8,   lsl   #17
    @            add     bit_consumed,  bit_consumed,   r9,  lsr  #29       
    @ENDIF          
                ands    r9,  r8,   #0xc0    @@// get x, r9  b(1100 0000)
                @//////////////////////////////////////////////                 
                @@@/* IQ--x channel */                                       
    	        streq   r9,   [IQ_out_ptr], #4                              
    	        beq     CB4_long_width_LOOP_SECOND_POINT                  
    	        @@//   4/3 de-quantization    	                                                
    	        subs    r9,  r9,  #0x40
    	        moveq   r9,  tmp_value1_iq
    	        movne   r9,  tmp_value2_iq
    	        
    	        mov     r6,  codeword,  lsl  bit_consumed
                add     bit_consumed, bit_consumed,  #1
                movs    r6,  r6,  asr #31   @@// get the sign  	        
                rsbne   r9,  r9,  #0   	                   	                                                                    
    	        str     r9,    [IQ_out_ptr], #4
    	        
CB4_long_width_LOOP_SECOND_POINT:
                ands    r9,  r8,   #0x30    @@// get y, r9  b(11 0000)
                @//////////////////////////////////////////////                 
                @@@/* IQ--y channel */                                       
    	        streq   r9,   [IQ_out_ptr], #4                              
    	        beq     CB4_long_width_LOOP_THIRD_POINT                  
    	        @@//   4/3 de-quantization    	                                                
    	        subs    r9,  r9,  #0x10
    	        moveq   r9,  tmp_value1_iq
    	        movne   r9,  tmp_value2_iq
    	        
    	        mov     r6,  codeword,  lsl  bit_consumed
                add     bit_consumed, bit_consumed,  #1
                movs    r6,  r6,  asr #31   @@// get the sign  	        
                rsbne   r9,  r9,  #0   	                   	                                                                    
    	        str     r9,    [IQ_out_ptr], #4
CB4_long_width_LOOP_THIRD_POINT:
                ands    r9,  r8,   #0xc    @@// get w, r9  b(1100)
                @//////////////////////////////////////////////                 
                @@@/* IQ--w channel */                                       
    	        streq   r9,   [IQ_out_ptr], #4                              
    	        beq     CB4_long_width_LOOP_FOURTH_POINT                  
    	        @@//   4/3 de-quantization    	                                                
    	        subs    r9,  r9,  #0x4
    	        moveq   r9,  tmp_value1_iq
    	        movne   r9,  tmp_value2_iq
    	        
    	        mov     r6,  codeword,  lsl  bit_consumed
                add     bit_consumed, bit_consumed,  #1
                movs    r6,  r6,  asr #31   @@// get the sign  	        
                rsbne   r9,  r9,  #0   	                   	                                                                    
    	        str     r9,    [IQ_out_ptr], #4
CB4_long_width_LOOP_FOURTH_POINT:
    	        ands    r9,  r8,   #0x3    @@// get v, r9  b(11)
                @//////////////////////////////////////////////                 
                @@@/* IQ--v channel */                                       
    	        streq   r9,   [IQ_out_ptr], #4                              
    	        beq     CB4_long_width_LOOP_NEXT_STEP                  
    	        @@//   4/3 de-quantization    	                                                
    	        subs    r9,  r9,  #0x1
    	        moveq   r9,  tmp_value1_iq
    	        movne   r9,  tmp_value2_iq
    	        
    	        mov     r6,  codeword,  lsl  bit_consumed
                add     bit_consumed, bit_consumed,  #1
                movs    r6,  r6,  asr #31   @@// get the sign  	        
                rsbne   r9,  r9,  #0   	                   	                                                                    
    	        str     r9,    [IQ_out_ptr], #4
    	        
CB4_long_width_LOOP_NEXT_STEP:                
                @@/* IQ end */                
                
                @@/////////////////////////////////////////////                
                @//////////////////////////////////////////////
                @/* read bit */                
                cmp   bit_consumed,  #12
                ble   CB4_LOOP_PRE_BIT_EXIT
                @@/* read bit */
                add   bit_info,  bit_info,  bit_consumed,  lsl  #16                
                mov   codeword,  codeword,  lsl bit_consumed

                mov   r8, bit_info,  lsr   #8
                and   r8,  r8,  #0xFF     @@// get the bit_left
                sub   bit_info,  bit_info,  r8, lsl #8
                cmp   bit_consumed,  r8
                bgt   CB4_LOOP_PRE_BIT_BRANCH
                @///////////////////////////
                ldr   r3, [stream_ptr]
                rsb   r8,  r8,  #32
    @IF AAC_DEC_ASM_LITTLE_ENDIAN = 1      
        @@/* little endian */  
        EOR      r6,  r3, r3,  ROR #16
        MOV    r6,  r6,    LSR #8
        BIC       r6,  r6, #0xFF00
        EOR     r3,  r6, r3, ROR #8        
    @ENDIF      

                mov   r3,  r3,   lsl   r8
                rsb   bit_consumed,  bit_consumed,  #32
                orr   codeword, codeword, r3,  lsr   bit_consumed                
                sub   r8,   bit_consumed,  r8
                orr   bit_info,  bit_info, r8,  lsl  #8  
                mov   bit_consumed,  #0              
                b     CB4_LOOP_PRE_BIT_EXIT
CB4_LOOP_PRE_BIT_BRANCH:     
                ldr   r3, [stream_ptr], #4
                sub   bit_consumed,  bit_consumed,  r8
    @IF AAC_DEC_ASM_LITTLE_ENDIAN = 1      
        @@/* little endian */  
        EOR      r6,  r3, r3,  ROR #16
        MOV    r6,  r6,    LSR #8
        BIC       r6,  r6, #0xFF00
        EOR     r3,  r6, r3, ROR #8        
    @ENDIF       
                rsb   r8,  r8,  #32
                mov   r3,  r3,  lsl r8
                mov   r3,  r3,  lsr r8                               
                orr   codeword, codeword, r3,   lsl  bit_consumed
                ldr   r3,  [stream_ptr]
                rsb   r8,  bit_consumed,  #32
    @IF AAC_DEC_ASM_LITTLE_ENDIAN = 1      
        @@/* little endian */  
        EOR      r6,  r3, r3,  ROR #16
        MOV    r6,  r6,    LSR #8
        BIC       r6,  r6, #0xFF00
        EOR     r3,  r6, r3, ROR #8        
    @ENDIF     
                orr   bit_info,  bit_info,  r8,  lsl #8
                orr   codeword, codeword, r3,  lsr  r8   
                mov   bit_consumed,  #0                              
CB4_LOOP_PRE_BIT_EXIT:                
                @///////////////////////////     
                                
                
                @@///////////////////////////////////////////  
                subs   r14, r14, #4
                BGT    AAC_DEC_HuffmanCB4Asm_SFB_WIDTH_LOOP           
                
                
                
                
                ldmfd   sp!, {r14, r2, r3}
                
                @@///////////////////////////////////////////  
                subs   AAC_SFB_COUNT, AAC_SFB_COUNT, #1
                BGT    AAC_DEC_HuffmanCB4Asm_SFB_COUNT_LOOP
                @@///////////////////////////////////////////
                
                @@/* end */                
                ldmfd   sp!, {ld_ptr}                
                @@/* bit buffer update */
                add   bit_consumed, bit_consumed, bit_info, lsr #16
                ldrsh r7, [ld_ptr, #4]   @@// left bit
                ldrsh r9, [ld_ptr, #6]   @@// byte used                
                cmp    bit_consumed, r7
                suble  r7,  r7, bit_consumed
                strleh r7, [ld_ptr, #4]
                ble   AAC_DEC_HuffmanCB4Asm_EXIT
                @/**/
                ldr    r10,   [ld_ptr]                
                sub   bit_consumed,  bit_consumed,  r7                
                mov   r8,  bit_consumed,  lsr #5
                add   r8,  r8,  #1
                add   r10,  r10,  r8,    lsl #2                
                and   r7,  bit_consumed,  #0x1F
                rsb   r7,  r7,  #32                
                add   r9,  r9,  r8,    lsl #2
                str   r10, [ld_ptr]
                strh  r7, [ld_ptr, #4]
                strh  r9, [ld_ptr, #6]                
AAC_DEC_HuffmanCB4Asm_EXIT:                
                ldmfd   sp!, {r4-r12, pc}
                @ENDFUNC
                
                
                
                
                
                
                
                
                
                
                
                
                
                
                
                
                @//int32 AAC_DEC_HuffmanCB1and2Asm(
                @//                              int32                   *out_ptr,
                @//                              AAC_BIT_FIFO_FORMAT_T   *ld_ptr,
                @//                              int32                   *scalefac_ptr,
                @//                              int32                   *sect_sfb_offset_ptr
                @//                              )@
                

tmp_value1_iq     .req  r7
tmp_value_neg1_iq .req  r2

AAC_DEC_HuffmanCB1and2Asm:@   FUNCTION
                @ // save the value for the following calculation
                .global  AAC_DEC_HuffmanCB1and2Asm                
                
                stmfd   sp!, {r4-r12, r14}                               
                
                stmfd    sp!, {ld_ptr}                 
                ldsh    r6,  [ld_ptr, #4]  
                mov     bit_info,   #0
                ldr     stream_ptr, [ld_ptr]
                cmp     r6,  #0
                addeq   stream_ptr,  stream_ptr,  #4
                moveq   r6,  #32                
                ldr     AAC_SFB_COUNT, [IQ_out_ptr]
                ldr     codeword, [stream_ptr], #4               
                rsbs    r7, r6, #32
    @IF AAC_DEC_ASM_LITTLE_ENDIAN = 1      
        @@/* little endian */  
        EOR      r5,  codeword, codeword,  ROR #16
        MOV    r5,  r5,    LSR #8
        BIC       r5,  r5, #0xFF00
        EOR     codeword,  r5, codeword, ROR #8        
    @ENDIF 
                ldr     r4, [stream_ptr]
                mov     codeword, codeword, lsl r7          
    @IF AAC_DEC_ASM_LITTLE_ENDIAN = 1      
        @@/* little endian */  
        EOR      r5,  r4, r4,  ROR #16
        MOV    r5,  r5,    LSR #8
        BIC       r5,  r5, #0xFF00
        EOR     r4,  r5, r4, ROR #8        
    @ENDIF       
                rsbs    r7, r7, #32
                orr     codeword, codeword, r4, lsr  r7
                orr     bit_info, bit_info,  r7,  lsl #8                
                
                ldr     huf_table_ptr, [IQ_out_ptr, #4]    
                             
                mov     bit_consumed,  #0
                @/* the register r4, r5, r6, r7,r8, r9, r10 are free */
                
                
AAC_DEC_HuffmanCB1and2Asm_SFB_COUNT_LOOP:                
                @@///////////////////////////////////////////
                ldrsh    r8,  [sect_sfb_offset_ptr], #2
                ldrsh    r5,  [sect_sfb_offset_ptr]                            
                ldrsh    r6,  [scalefac_ptr], #2
                                
                stmfd    sp!, {r14, r2, r3}                
                sub      r14, r5,  r8                
                mov      r9,   r6,  asr  #2
                and      r8,   r6,  #0x3  
                ldr      r6, =AAC_iq_power_TAB_F_n              
                
                add      sfb_frac,  r6,   r8, lsl #2
                ldr      sfb_frac, [sfb_frac]     
                
                mov      tmp_value1_iq,  #2048*4                
                smull    r8, tmp_value1_iq, sfb_frac, tmp_value1_iq                
                
                subs     r9,   r9,  #32  @@// r9->exp
                movge    tmp_value1_iq,  tmp_value1_iq,  lsl  r9              
                                
                rsblt    r9,  r9,  #0
                movlt    tmp_value1_iq,  tmp_value1_iq,  asr  r9
               
                rsb      tmp_value_neg1_iq,  tmp_value1_iq,  #0
                @/* the register r6, r7, r8, r9 are free */                
AAC_DEC_HuffmanCB1and2Asm_SFB_WIDTH_LOOP:
                @/* huffman parsing */                
                mov     r3,  codeword,  lsl   bit_consumed
                mov     r3,  r3, lsr  #25   @@// first get 7 bit
                add     r9,  huf_table_ptr,   r3,  lsl  #1
                ldrsh   r8,  [r9]
    @IF AAC_DEC_ASM_LITTLE_ENDIAN = 1      
                @@/* little endian */  
                tst     r8,    #0x1
    @ELSE
    @            tst     r8,    #0x8000
    @ENDIF
                bne     CB1and2_HUFFMAN_PARSING                
                @@//////////////////////////////                
                add     bit_consumed,  bit_consumed,  #7
    @IF AAC_DEC_ASM_LITTLE_ENDIAN = 1      
                @@/* little endian */  
                and     r9,  r8,  #0xF    @@// pre-read bit count
                mov    r9,  r9, lsr  #1
                mov     r8,  r8, lsr  #4
                add     r8,  huf_table_ptr,  r8,lsl #1        
     @ELSE
     @           mov     r9,  r8, lsr  #12    @@// pre-read bit count
     @           mov     r8,  r8, lsl  #20
     @           add     r8,  huf_table_ptr,  r8,lsr #19        
     @ENDIF
                rsb     r6,  r9,   #32
                mov     r3,  codeword,   lsl  bit_consumed
                mov     r3,  r3,   lsr r6
                add     r8,  r8,   r3,  lsl #1                          
                ldrsh   r8,  [r8]                
                
CB1and2_HUFFMAN_PARSING:      
    @IF AAC_DEC_ASM_LITTLE_ENDIAN = 1      
                @@/* little endian */  
                and     r9,  r8,    #0xF
                add     bit_consumed,  bit_consumed,   r9,  lsr  #1
                mov    r8,  r8,  lsr  #4
    @ELSE          
    @            mov     r9,  r8,   lsl   #17
    @            add     bit_consumed,  bit_consumed,   r9,  lsr  #29
    @ENDIF
                
                and     r9,  r8,   #0xc0    @@// get x, r9  b(1100 0000)  0xc0
                subs    r9,  r9,   #0x40    @@// get x, r9  b(0100 0000)  0x40
                @//////////////////////////////////////////////                 
                @@@/* IQ--x channel */                                       
    	        streq   r9,   [IQ_out_ptr], #4	                                                
    	        strgt   tmp_value1_iq,     [IQ_out_ptr], #4
    	        strlt   tmp_value_neg1_iq, [IQ_out_ptr], #4
    	        

                and     r9,  r8,   #0x30    @@// get y, r9  b(11 0000)
                subs    r9,  r9,   #0x10    @@// get y, r9  b(01 0000)
                @//////////////////////////////////////////////                 
                @@@/* IQ--y channel */                                       
    	        streq   r9,   [IQ_out_ptr], #4                                                
    	        strgt   tmp_value1_iq,     [IQ_out_ptr], #4
    	        strlt   tmp_value_neg1_iq, [IQ_out_ptr], #4
    	        

                and     r9,  r8,   #0xc    @@// get w, r9  b(1100)
                subs    r9,  r9,   #0x4
                @//////////////////////////////////////////////                 
                @@@/* IQ--w channel */                                       
    	        streq   r9,   [IQ_out_ptr], #4                                                
    	        strgt   tmp_value1_iq,     [IQ_out_ptr], #4
    	        strlt   tmp_value_neg1_iq, [IQ_out_ptr], #4
    	        
    	        and     r9,  r8,   #0x3    @@// get v, r9  b(1100)
    	        subs    r9,  r9,   #0x1
                @//////////////////////////////////////////////                 
                @@@/* IQ--v channel */                                       
    	        streq   r9,   [IQ_out_ptr], #4
    	        strgt   tmp_value1_iq,     [IQ_out_ptr], #4
    	        strlt   tmp_value_neg1_iq, [IQ_out_ptr], #4
    	        
                @@/* IQ end */                
                
                @@/////////////////////////////////////////////                
                @//////////////////////////////////////////////
                @/* read bit */                
                cmp   bit_consumed,  #21
                ble   CB1and2_LOOP_PRE_BIT_EXIT
                @@/* read bit */
                add   bit_info,  bit_info,  bit_consumed,  lsl  #16                
                mov   codeword,  codeword,  lsl bit_consumed

                mov   r8, bit_info,  lsr   #8
                and   r8,  r8,  #0xFF     @@// get the bit_left
                sub   bit_info,  bit_info,  r8, lsl #8
                cmp   bit_consumed,  r8
                bgt   CB1and2_LOOP_PRE_BIT_BRANCH
                @///////////////////////////
                ldr   r3, [stream_ptr]
                rsb   r8,  r8,  #32
    @IF AAC_DEC_ASM_LITTLE_ENDIAN = 1      
        @@/* little endian */  
        EOR      r9,  r3, r3,  ROR #16
        MOV    r9,  r9,    LSR #8
        BIC       r9,  r9, #0xFF00
        EOR     r3,  r9, r3, ROR #8        
    @ENDIF     
                mov   r3,  r3,   lsl   r8
                rsb   bit_consumed,  bit_consumed,  #32
                orr   codeword, codeword, r3,  lsr   bit_consumed                
                sub   r8,   bit_consumed,  r8
                orr   bit_info,  bit_info, r8,  lsl  #8  
                mov   bit_consumed,  #0              
                b     CB1and2_LOOP_PRE_BIT_EXIT
CB1and2_LOOP_PRE_BIT_BRANCH:     
                ldr   r3, [stream_ptr], #4
                sub   bit_consumed,  bit_consumed,  r8
    @IF AAC_DEC_ASM_LITTLE_ENDIAN = 1      
        @@/* little endian */  
        EOR      r9,  r3, r3,  ROR #16
        MOV    r9,  r9,    LSR #8
        BIC       r9,  r9, #0xFF00
        EOR     r3,  r9, r3, ROR #8        
    @ENDIF     
                rsb   r8,  r8,  #32
                mov   r3,  r3,  lsl r8
                mov   r3,  r3,  lsr r8                               
                orr   codeword, codeword, r3,   lsl  bit_consumed
                ldr   r3,  [stream_ptr]
                rsb   r8,  bit_consumed,  #32
    @IF AAC_DEC_ASM_LITTLE_ENDIAN = 1      
        @@/* little endian */  
        EOR      r9,  r3, r3,  ROR #16
        MOV    r9,  r9,    LSR #8
        BIC       r9,  r9, #0xFF00
        EOR     r3,  r9, r3, ROR #8        
    @ENDIF     
                orr   bit_info,  bit_info,  r8,  lsl #8
                orr   codeword, codeword, r3,  lsr  r8   
                mov   bit_consumed,  #0                              
CB1and2_LOOP_PRE_BIT_EXIT:                
                @///////////////////////////     
                                
                
                @@///////////////////////////////////////////  
                subs   r14, r14, #4
                BGT    AAC_DEC_HuffmanCB1and2Asm_SFB_WIDTH_LOOP  
                
                
                ldmfd   sp!, {r14, r2, r3}                
                @@///////////////////////////////////////////  
                subs   AAC_SFB_COUNT, AAC_SFB_COUNT, #1
                BGT    AAC_DEC_HuffmanCB1and2Asm_SFB_COUNT_LOOP
                @@///////////////////////////////////////////
                
                @@/* end */                
                ldmfd   sp!, {ld_ptr}                
                @@/* bit buffer update */
                add   bit_consumed, bit_consumed, bit_info, lsr #16
                ldrsh r7, [ld_ptr, #4]   @@// left bit
                ldrsh r9, [ld_ptr, #6]   @@// byte used                
                cmp    bit_consumed, r7
                suble  r7,  r7, bit_consumed
                strleh r7, [ld_ptr, #4]
                ble   AAC_DEC_HuffmanCB1and2Asm_EXIT
                @/**/
                ldr    r10,   [ld_ptr]                
                sub   bit_consumed,  bit_consumed,  r7                
                mov   r8,  bit_consumed,  lsr #5
                add   r8,  r8,  #1
                add   r10,  r10,  r8,    lsl #2                
                and   r7,  bit_consumed,  #0x1F
                rsb   r7,  r7,  #32                
                add   r9,  r9,  r8,    lsl #2
                str   r10, [ld_ptr]
                strh  r7, [ld_ptr, #4]
                strh  r9, [ld_ptr, #6]                
AAC_DEC_HuffmanCB1and2Asm_EXIT:                
                ldmfd   sp!, {r4-r12, pc}
                @ENDFUNC
                
                @//int32 AAC_DEC_HuffmanCB8and10Asm(
                @//                              int32                   *out_ptr,
                @//                              AAC_BIT_FIFO_FORMAT_T   *ld_ptr,
                @//                              int32                   *scalefac_ptr,
                @//                              int32                   *sect_sfb_offset_ptr
                @//                              )@
                

tmp_iq_table_ptr   .req  r2
tmp_iq_exp         .req  r7

AAC_DEC_HuffmanCB8and10Asm:@   FUNCTION
                @ // save the value for the following calculation
                .global  AAC_DEC_HuffmanCB8and10Asm
                
                stmfd   sp!, {r4-r12, r14}                               
                
                stmfd    sp!, {ld_ptr}                 
                ldsh    r6,  [ld_ptr, #4]  
                mov     bit_info,   #0
                ldr     stream_ptr, [ld_ptr]
                cmp     r6,  #0
                addeq   stream_ptr,  stream_ptr,  #4
                moveq   r6,  #32                
                ldr     AAC_SFB_COUNT, [IQ_out_ptr]
                ldr     codeword, [stream_ptr], #4               
                rsbs    r7, r6, #32
    @IF AAC_DEC_ASM_LITTLE_ENDIAN = 1      
        @@/* little endian */  
        EOR      r5,  codeword, codeword,  ROR #16
        MOV    r5,  r5,    LSR #8
        BIC       r5,  r5, #0xFF00
        EOR     codeword,  r5, codeword, ROR #8        
    @ENDIF 
                ldr     r4, [stream_ptr]
                mov     codeword, codeword, lsl r7       
    @IF AAC_DEC_ASM_LITTLE_ENDIAN = 1      
        @@/* little endian */  
        EOR      r5,  r4, r4,  ROR #16
        MOV    r5,  r5,    LSR #8
        BIC       r5,  r5, #0xFF00
        EOR     r4,  r5, r4, ROR #8        
    @ENDIF          
                rsbs    r7, r7, #32
                orr     codeword, codeword, r4, lsr  r7
                orr     bit_info, bit_info,  r7,  lsl #8  
                              
                ldr     huf_table_ptr, [IQ_out_ptr, #4]
                                 
                mov     bit_consumed,  #0
                @/* the register r4, r5, r6, r7,r8, r9, r10 are free */
                
                
AAC_DEC_HuffmanCB8and10Asm_SFB_COUNT_LOOP:                
                @@///////////////////////////////////////////
                ldrsh    r8,  [sect_sfb_offset_ptr], #2
                ldrsh    r5,  [sect_sfb_offset_ptr]                            
                ldrsh    r6,  [scalefac_ptr], #2
                                
                stmfd    sp!, {r14, r2, r3}                
                sub      r14, r5,  r8                
                mov      r5,   r6,  asr  #2
                and      r8,   r6,  #0x3  
                ldr      r6, =AAC_iq_power_TAB_F_n
                sub      tmp_iq_exp,   r5,  #32
                                
                add      sfb_frac,  r6,   r8, lsl #2
                ldr      sfb_frac, [sfb_frac]     
                
                ldr     tmp_iq_table_ptr,  =AAC_iq_table16   
                
                           
                @/* the register r6, r7, r8, r9 are free */                
AAC_DEC_HuffmanCB8and10Asm_SFB_WIDTH_LOOP:
                @/* huffman parsing */                
                mov     r3,  codeword,  lsl   bit_consumed
                mov     r3,  r3, lsr  #26   @@// first get 6 bit
                add     r9,  huf_table_ptr,   r3,  lsl  #1
                ldrsh   r8,  [r9] 
    @IF AAC_DEC_ASM_LITTLE_ENDIAN = 1      
                @@/* little endian */  
                tst     r8,    #0x1
    @ELSE
    @            tst     r8,    #0x8000
    @ENDIF
                bne     CB8and10_HUFFMAN_PARSING
                @@//////////////////////////////                
                add     bit_consumed,  bit_consumed,  #6
    @IF AAC_DEC_ASM_LITTLE_ENDIAN = 1      
                @@/* little endian */  
               and     r9,  r8,   #0xF    @@// pre-read bit count
               mov    r9,   r9,    lsr  #1
                mov     r8,  r8, lsr  #4
                add     r8,  huf_table_ptr,  r8,lsl #1
     @ELSE
     @           mov     r9,  r8, lsr  #12    @@// pre-read bit count
     @           mov     r8,  r8, lsl  #20
     @           add     r8,  huf_table_ptr,  r8,lsr #19           
     @ENDIF
                rsb     r6,  r9,   #32
                mov     r3,  codeword,   lsl  bit_consumed
                mov     r3,  r3,   lsr r6
                add     r8,  r8,   r3,  lsl #1                          
                ldrsh   r8,  [r8] 
CB8and10_HUFFMAN_PARSING:          
    @IF AAC_DEC_ASM_LITTLE_ENDIAN = 1      
                @@/* little endian */        
                and      r9,  r8,    #0xF
                add     bit_consumed,  bit_consumed,   r9,  lsr  #1                
                mov     r8,   r8,  lsr  #4
                movs    r9,  r8,   lsr   #6    @@// get x, r9                
    @ELSE
    @            mov     r9,  r8,   lsl   #17
    @            add     bit_consumed,  bit_consumed,   r9,  lsr  #29
    @            mov     r9,  r9,   lsl   #3                
    @            movs    r9,  r9,   lsr   #26    @@// get x, r9                
    @ENDIF
                @//////////////////////////////////////////////                 
                @@@/* IQ--left channel */
                @@//  r2, r3, r6, r9, r7                                        
    	        streq   r9,   [IQ_out_ptr], #4                              
    	        beq     CB8and10_long_width_LOOP_SECOND_POINT                  
    	        @@//   4/3 de-quantization    	                                                
    	        add   r9,  tmp_iq_table_ptr,  r9, lsl #2
    	        ldr   r9,  [r9]    	        
    	        mov     r6,  codeword,  lsl  bit_consumed
                add     bit_consumed, bit_consumed,  #1
                movs    r6,  r6,  asr #31   @@// get the sign    	        
    	        @@/* scale */    	        
                rsbne  r9,  r9,  #0                                                    
    	        smull  r9,  r3,  sfb_frac,  r9    	        
    	        @//mov     r6,  bit_info,  lsl #24
    	        @//movs    r9,  r6,  asr #24                              
    	        movs    r9,  tmp_iq_exp
    	        movge   r3,    r3,  lsl   r9                       
    	        rsblt   r9,    r9,  #0                             
    	        movlt   r3,    r3,  asr   r9    	            	                                                                    
    	        str     r3,    [IQ_out_ptr], #4
    	        
CB8and10_long_width_LOOP_SECOND_POINT:
                ands    r8,  r8,   #0x3F         @@// get y: r8
                @@@/* IQ--right channel */
                @@//  r6, r2, r3, r8, r7    	        
    	        streq   r8,   [IQ_out_ptr], #4
    	        beq     CB8and10_long_width_LOOP_NEXT_STEP    	        
    	        @@//   4/3 de-quantization    	        
    	        add   r8,  tmp_iq_table_ptr,  r8, lsl #2
    	        ldr   r8,  [r8]    	           
    	        mov   r6,  codeword,  lsl  bit_consumed
                add   bit_consumed, bit_consumed,  #1                
                movs  r6, r6,  asr #31   @@// get the sign     	        
    	        @@/* scale */    	        
    	        rsbne   r8,  r8,  #0    	               	        
    	        smull   r8,    r3,    sfb_frac,  r8    	        
    	        @//mov     r9,  bit_info,  lsl #24
    	        @//movs    r9,  r9,  asr #24
    	        movs    r9,  tmp_iq_exp       
    	        movge   r3,    r3,  lsl   r9
    	        rsblt   r8,    r9,  #0
    	        movlt   r3,    r3,  asr   r8    	        
    	        str     r3,    [IQ_out_ptr], #4  
CB8and10_long_width_LOOP_NEXT_STEP:                
                @@/* IQ end */                
                
                @@/////////////////////////////////////////////                
                @//////////////////////////////////////////////
                @/* read bit */                
                cmp   bit_consumed,  #18
                ble   CB8and10_LOOP_PRE_BIT_EXIT
                @@/* read bit */
                add   bit_info,  bit_info,  bit_consumed,  lsl  #16                
                mov   codeword,  codeword,  lsl bit_consumed

                mov   r8, bit_info,  lsr   #8
                and   r8,  r8,  #0xFF     @@// get the bit_left
                sub   bit_info,  bit_info,  r8, lsl #8
                cmp   bit_consumed,  r8
                bgt   CB8and10_LOOP_PRE_BIT_BRANCH
                @///////////////////////////
                ldr   r3, [stream_ptr]
                rsb   r8,  r8,  #32
    @IF AAC_DEC_ASM_LITTLE_ENDIAN = 1      
        @@/* little endian */  
        EOR      r6,  r3, r3,  ROR #16
        MOV    r6,  r6,    LSR #8
        BIC       r6,  r6, #0xFF00
        EOR     r3,  r6, r3, ROR #8        
    @ENDIF 
                mov   r3,  r3,   lsl   r8
                rsb   bit_consumed,  bit_consumed,  #32
                orr   codeword, codeword, r3,  lsr   bit_consumed                
                sub   r8,   bit_consumed,  r8
                orr   bit_info,  bit_info, r8,  lsl  #8  
                mov   bit_consumed,  #0              
                b     CB8and10_LOOP_PRE_BIT_EXIT
CB8and10_LOOP_PRE_BIT_BRANCH:     
                ldr   r3, [stream_ptr], #4
                sub   bit_consumed,  bit_consumed,  r8
    @IF AAC_DEC_ASM_LITTLE_ENDIAN = 1      
        @@/* little endian */  
        EOR      r6,  r3, r3,  ROR #16
        MOV    r6,  r6,    LSR #8
        BIC       r6,  r6, #0xFF00
        EOR     r3,  r6, r3, ROR #8        
    @ENDIF 
                rsb   r8,  r8,  #32
                mov   r3,  r3,  lsl r8
                mov   r3,  r3,  lsr r8                               
                orr   codeword, codeword, r3,   lsl  bit_consumed
                ldr   r3,  [stream_ptr]
                rsb   r8,  bit_consumed,  #32
    @IF AAC_DEC_ASM_LITTLE_ENDIAN = 1      
        @@/* little endian */  
        EOR      r6,  r3, r3,  ROR #16
        MOV    r6,  r6,    LSR #8
        BIC       r6,  r6, #0xFF00
        EOR     r3,  r6, r3, ROR #8        
    @ENDIF 
                orr   bit_info,  bit_info,  r8,  lsl #8
                orr   codeword, codeword, r3,  lsr  r8   
                mov   bit_consumed,  #0                              
CB8and10_LOOP_PRE_BIT_EXIT:                
                @@///////////////////////////////////////////  
                subs   r14, r14, #2
                BGT    AAC_DEC_HuffmanCB8and10Asm_SFB_WIDTH_LOOP           
                
                
                
                
                ldmfd   sp!, {r14, r2, r3}
                
                @@///////////////////////////////////////////  
                subs   AAC_SFB_COUNT, AAC_SFB_COUNT, #1
                BGT    AAC_DEC_HuffmanCB8and10Asm_SFB_COUNT_LOOP
                @@///////////////////////////////////////////               
                
                
                
                @@/* end */                
                ldmfd   sp!, {ld_ptr}                
                @@/* bit buffer update */
                add   bit_consumed, bit_consumed, bit_info, lsr #16
                ldrsh r7, [ld_ptr, #4]   @@// left bit
                ldrsh r9, [ld_ptr, #6]   @@// byte used                
                cmp    bit_consumed, r7
                suble  r7,  r7, bit_consumed
                strleh r7, [ld_ptr, #4]
                ble   AAC_DEC_HuffmanCB8and10Asm_EXIT
                @/**/
                ldr    r10,   [ld_ptr]                
                sub   bit_consumed,  bit_consumed,  r7                
                mov   r8,  bit_consumed,  lsr #5
                add   r8,  r8,  #1
                add   r10,  r10,  r8,    lsl #2                
                and   r7,  bit_consumed,  #0x1F
                rsb   r7,  r7,  #32                
                add   r9,  r9,  r8,    lsl #2
                str   r10, [ld_ptr]
                strh  r7, [ld_ptr, #4]
                strh  r9, [ld_ptr, #6]                
AAC_DEC_HuffmanCB8and10Asm_EXIT:                
                ldmfd   sp!, {r4-r12, pc}
                @ENDFUNC
                    	                
        .end@END
        