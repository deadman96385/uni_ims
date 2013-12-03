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
                               


tmp_iq_table_ptr   .req  r2
tmp_iq_exp         .req  r7
                
                
                .extern  AAC_iq_table16
                .extern  AAC_iq_power_TAB_F_n
                
                @//int32 AAC_DEC_HuffmanCB5Asm(
                @//                              int32                   *out_ptr,
                @//                              AAC_BIT_FIFO_FORMAT_T   *ld_ptr,
                @//                              int32                   *scalefac_ptr,
                @//                              int32                   *sect_sfb_offset_ptr
                @//                              )@
                

tmp_iq_table_ptr   .req  r2
tmp_iq_exp         .req  r7

AAC_DEC_HuffmanCB5Asm:@   FUNCTION
                @ // save the value for the following calculation
                .global  AAC_DEC_HuffmanCB5Asm
                
                stmfd   sp!, {r4-r11, r14}                               
                
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
                
                
AAC_DEC_HuffmanCB5Asm_SFB_COUNT_LOOP:                
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
AAC_DEC_HuffmanCB5Asm_SFB_WIDTH_LOOP:
                @/* huffman parsing */ 
                @@/* check the first bit */               
                mov     r3,  codeword,  lsl   bit_consumed
                add     bit_consumed,  bit_consumed,  #1
                movs    r3,  r3,  lsr   #31
                streq   r3,  [IQ_out_ptr], #4 
                streq   r3,  [IQ_out_ptr], #4 
                beq     CB5_long_width_LOOP_NEXT_STEP  
                       
                
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
                bne     CB5_HUFFMAN_PARSING                
CB5_HUFFMAN_PARSING_LOOP:
                @@//////////////////////////////                
                add     bit_consumed,  bit_consumed,  r9
    @IF AAC_DEC_ASM_LITTLE_ENDIAN = 1      
                @@/* little endian */
                and      r9,  r8,   #0xF    @@// pre-read bit count
                mov     r9,  r9,   lsr  #1
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
    @ ENDIF
                beq     CB5_HUFFMAN_PARSING_LOOP  
CB5_HUFFMAN_PARSING:     
    @IF AAC_DEC_ASM_LITTLE_ENDIAN = 1      
                @@/* little endian */
                and     r9,  r8,    #0xF
                add     bit_consumed,  bit_consumed,   r9,  lsr  #1
                mov     r8,  r8,   lsr   #4
                mov     r9,  r8,   lsr   #6    @@// get x, r9                
    @ELSE           
    @            mov     r9,  r8,   lsl   #17
    @            add     bit_consumed,  bit_consumed,   r9,  lsr  #29
    @            mov     r9,  r9,   lsl   #3                
    @            mov     r9,  r9,   lsr   #26    @@// get x, r9                
    @ENDIF
                @//////////////////////////////////////////////                 
                @@@/* IQ--left channel */
                @@//  r2, r3, r6, r9, r7      
                subs    r9,   r9,  #4                                  
    	        streq   r9,   [IQ_out_ptr], #4                              
    	        beq     CB5_long_width_LOOP_SECOND_POINT                  
    	        @@//   4/3 de-quantization 
    	        rsblt   r9,   r9,  #0
    	        add     r9,  tmp_iq_table_ptr,  r9, lsl #2
    	        ldr     r9,  [r9]    	            	           	        
    	        @@/* scale */    	        
                rsblt   r9,  r9,  #0                                                    
    	        smull   r9,  r3,  sfb_frac,  r9
    	            	        
    	        movs    r9,  tmp_iq_exp
    	        movge   r3,    r3,  lsl   r9                       
    	        rsblt   r9,    r9,  #0                             
    	        movlt   r3,    r3,  asr   r9    	            	                                                                    
    	        str     r3,    [IQ_out_ptr], #4
    	        
CB5_long_width_LOOP_SECOND_POINT:
                and     r8,  r8,   #0x3F         @@// get y: r8
                subs    r8,  r8,   #4
                @@@/* IQ--right channel */
                @@//  r6, r2, r3, r8, r7    	        
    	        streq   r8,   [IQ_out_ptr], #4
    	        beq     CB5_long_width_LOOP_NEXT_STEP    	        
    	        @@//   4/3 de-quantization    	        
    	        rsblt   r8,  r8,  #0
    	        add     r8,  tmp_iq_table_ptr,  r8, lsl #2
    	        ldr     r8,  [r8]    	           
    	            	        
    	        @@/* scale */    	        
    	        rsblt   r8,  r8,  #0    	               	        
    	        smull   r8,    r3,    sfb_frac,  r8    	        
    	        
    	        movs    r9,  tmp_iq_exp       
    	        movge   r3,    r3,  lsl   r9
    	        rsblt   r8,    r9,  #0
    	        movlt   r3,    r3,  asr   r8    	        
    	        str     r3,    [IQ_out_ptr], #4  
CB5_long_width_LOOP_NEXT_STEP:
                @@/* IQ end */                
                
                @@/////////////////////////////////////////////                
                @//////////////////////////////////////////////
                @/* read bit */                
                cmp   bit_consumed,  #19
                ble   CB5_LOOP_PRE_BIT_EXIT
                @@/* read bit */
                add   bit_info,  bit_info,  bit_consumed,  lsl  #16                
                mov   codeword,  codeword,  lsl bit_consumed

                mov   r8, bit_info,  lsr   #8
                and   r8,  r8,  #0xFF     @@// get the bit_left
                sub   bit_info,  bit_info,  r8, lsl #8
                cmp   bit_consumed,  r8
                bgt   CB5_LOOP_PRE_BIT_BRANCH
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
                b     CB5_LOOP_PRE_BIT_EXIT
CB5_LOOP_PRE_BIT_BRANCH:
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
CB5_LOOP_PRE_BIT_EXIT:                
                @///////////////////////////     
                                
                
                @@///////////////////////////////////////////  
                subs   r14, r14, #2
                BGT    AAC_DEC_HuffmanCB5Asm_SFB_WIDTH_LOOP           
                
                
                
                
                ldmfd   sp!, {r14, r2, r3}
                
                @@///////////////////////////////////////////  
                subs   AAC_SFB_COUNT, AAC_SFB_COUNT, #1
                BGT    AAC_DEC_HuffmanCB5Asm_SFB_COUNT_LOOP
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
                ble   AAC_DEC_HuffmanCB5Asm_EXIT
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
AAC_DEC_HuffmanCB5Asm_EXIT:                
                ldmfd   sp!, {r4-r11, pc}
                @ENDFUNC
                
                
            
            
                

ld_ptr                      .req    r1
stream_ptr                  .req    r1
scalefac_ptr                .req    r2
sfb_cb_ptr                  .req    r14
AAC_num_window_groups       .req    r3
codeword                    .req    r12
bit_info                    .req    r11    @@// 31:16,  total bit consumed,   15:8:  bit_left,   7:0,  exp
huf_table_ptr               .req    r10
bit_consumed                .req    r4     @@// bit consumed                
                
max_sfb                     .req    r9 


scale_factor                .req    r7
noise_energy                .req    r6
is_position_noise_flag      .req    r0    @@// 31:16, is_position,   15:0, noise_flag

                
                @//int32 AAC_DEC_ScaleFactorHuffAsm(
                @//                              AAC_ICS_STREAM_T        *ics_ptr,
                @//                              AAC_BIT_FIFO_FORMAT_T   *ld_ptr
                @//                              )@
AAC_DEC_ScaleFactorHuffAsm:@   FUNCTION
                @ // save the value for the following calculation
                .global  AAC_DEC_ScaleFactorHuffAsm
                .extern  AAC_DEC_ScaleFactorHuff
                stmfd   sp!, {r4-r11, r14}  
                
                ldrb    max_sfb,               [r0, #20]
                subs     max_sfb, max_sfb,  #1                
                movlt    r0,  #0 
                BLT      AAC_DEC_ScaleFactorHuffAsm_EXIT                               
                add     max_sfb, max_sfb,   max_sfb,   lsl #8   
                
                           
                stmfd    sp!, {ld_ptr}                 
                ldsh    r6,  [ld_ptr, #4]  
                mov     bit_info,   #0
                ldr     stream_ptr, [ld_ptr]
                cmp     r6,  #0
                addeq   stream_ptr,  stream_ptr,  #4
                moveq   r6,  #32                
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
                                                            
                ldr     huf_table_ptr, =AAC_DEC_ScaleFactorHuff
                
                mov     bit_consumed,  #0
                
                ldr     scalefac_ptr,          [r0, #12]
                ldrb    AAC_num_window_groups, [r0, #17]
                
                ldrb    scale_factor,          [r0, #23]
                add     sfb_cb_ptr        ,     r0,  #144
                
                
                sub     noise_energy, scale_factor, #90
                mov     is_position_noise_flag,  #1
                @/* the register r5, r8 are free */
                
                
AAC_DEC_HuffmanSCFAsm_num_window_groups_LOOP:                
                @@///////////////////////////////////////////                                
                stmfd    sp!, {scalefac_ptr, AAC_num_window_groups, sfb_cb_ptr} 
                                           
                @/* the register r3, r5, r8 are free */            
AAC_DEC_HuffmanSCFAsm_MAX_SFB_LOOP:
                @@//////////////////////////////////////////////
                ldrb    r5,  [sfb_cb_ptr],  #1
                
                cmp     r5,  #0
                @/* case ZERO_HCB */
                streqh  r5,  [scalefac_ptr], #2
                beq     SCF_LOOP_PRE_BIT_EXIT
                @//////////////////////////////
                cmp     r5,   #12
                bgt     AAC_DEC_HuffmanSCFAsmNOISE_HCB
                @/* default, cb <= 12*/
                @@/*huffman dec */
                @@// first, pre-read 1 bit
                mov     r3,  codeword,  lsl   bit_consumed
                add     bit_consumed,  bit_consumed,  #1
                movs    r3,  r3,  lsr   #31
                streqh  scale_factor,   [scalefac_ptr], #2
                beq     SCF_long_width_LOOP_NEXT_STEP
                @/* huffman parsing */                
                mov     r3,  codeword,  lsl   bit_consumed                    
                mov     r3,  r3, lsr  #27   @@// first get 5 bit              
                add     r5,  huf_table_ptr,   r3,  lsl  #1                    
                ldrsh   r8,  [r5]                                             
                mov     r5,  #5      
    @IF AAC_DEC_ASM_LITTLE_ENDIAN = 1         
                @@/* little endian */                                        
                tst     r8,    #0x1
    @ELSE
    @            tst     r8,    #0x8000
    @ENDIF
                bne     SCF_HUFFMAN_DEC_CBLESS13_EXIT                                   
SCF_HUFFMAN_DEC_CBLESS13_LOOP:                                                      
                @@//////////////////////////////                              
                add     bit_consumed,  bit_consumed,  r5                                                       
    @IF AAC_DEC_ASM_LITTLE_ENDIAN = 1      
               @@/* little endian */
                and     r5,  r8, #0xF    @@// pre-read bit count          
                mov    r5,  r5,  lsr #1
                mov     r8,  r8, lsr  #4
                add     r8,  huf_table_ptr,  r8,lsl #1    
                   
                rsb      r5,  r5,   #32                                        
                mov     r3,  codeword,   lsl  bit_consumed                    
                mov     r3,  r3,   lsr r5                                     
                add     r8,  r8,   r3,  lsl #1                                
                ldrsh   r8,  [r8]    
                rsb     r5,  r5,   #32              
                                 
                tst     r8,    #0x1
    @ELSE
    @            mov     r5,  r8, lsr  #12    @@// pre-read bit count          
    @            mov     r8,  r8, lsl  #20                                     
    @            add     r8,  huf_table_ptr,  r8,lsr #19                       
    @            rsb     r5,  r5,   #32                                        
    @            mov     r3,  codeword,   lsl  bit_consumed                    
    @            mov     r3,  r3,   lsr r5                                     
    @            add     r8,  r8,   r3,  lsl #1                                
    @            ldrsh   r8,  [r8]    
    @            rsb     r5,  r5,   #32      
    @            tst     r8,    #0x8000
    @ENDIF                                                      
                beq     SCF_HUFFMAN_DEC_CBLESS13_LOOP                              
SCF_HUFFMAN_DEC_CBLESS13_EXIT:           

      @IF AAC_DEC_ASM_LITTLE_ENDIAN = 1          
                @@/* little endian */                                      
                and     r5,  r8,   #0xF
                add     bit_consumed,  bit_consumed,   r5,  lsr  #1                 
                add     scale_factor,  scale_factor,  r8,  lsr   #4 
                sub      scale_factor,  scale_factor,  #60        
      @ELSE
      @          mov     r5,  r8,   lsl   #17                                  
      @          add     bit_consumed,  bit_consumed,   r5,  lsr  #29 
      @          and     r8,  r8,  #0xFF
      @          sub     r8,  r8,  #60
      @          add     scale_factor,  scale_factor,  r8                
      @ENDIF                
                
                tst     scale_factor, #0xFF00                
                BNE     SCF_HUFFMAN_ERROR_EXIT                         
                strh    scale_factor,  [scalefac_ptr], #2                  
                @/////////////////////////////
                B       SCF_long_width_LOOP_NEXT_STEP
AAC_DEC_HuffmanSCFAsmNOISE_HCB:                
                cmp     r5,   #13
                BGT     AAC_DEC_HuffmanSCFAsmINTENSITY_HCB
                @/////////////////////////////
                @/* NOISE_HCB:  noise books */
                ands   r5,   is_position_noise_flag,  #0x1
                beq    AAC_DEC_HuffmanSCFAsmNOISE_HCB_BRANCH
                @@/* noise_pcm_flag == 1 */ 
                sub   is_position_noise_flag,  is_position_noise_flag,  r5  @@// noise_pcm_flag = 0@
                mov     r8,  codeword,  lsl   bit_consumed                    
                mov     r8,  r8,  lsr  #23   @@// first get 9 bit 
                add      bit_consumed,  bit_consumed,   #9
                sub     r8,  r8,  #256
                
                B      AAC_DEC_HuffmanSCFAsmNOISE_HCB_BRANCHEXIT
AAC_DEC_HuffmanSCFAsmNOISE_HCB_BRANCH:                
                @@/* noise_pcm_flag == 0 */                
                @@/*huffman dec */
                @@// first, pre-read 1 bit
                mov     r8,  codeword,  lsl   bit_consumed
                add     bit_consumed,  bit_consumed,  #1
                movs    r8,  r8,  lsr   #31                
                beq     AAC_DEC_HuffmanSCFAsmNOISE_HCB_BRANCHEXIT
                @/* huffman parsing */                
                mov     r3,  codeword,  lsl   bit_consumed                    
                mov     r3,  r3, lsr  #27   @@// first get 5 bit              
                add     r5,  huf_table_ptr,   r3,  lsl  #1                    
                ldrsh   r8,  [r5]                                             
                mov     r5,  #5 
    @IF AAC_DEC_ASM_LITTLE_ENDIAN = 1         
                @@/* little endian */                                        
                tst     r8,    #0x1
    @ELSE
    @            tst     r8,    #0x8000        
    @ENDIF                                
                bne     SCF_HUFFMAN_DEC_NOISE_HCB_EXIT                                   
SCF_HUFFMAN_DEC_NOISE_HCB_LOOP:                                                      
                @@//////////////////////////////                              
                add     bit_consumed,  bit_consumed,  r5                     
    @IF AAC_DEC_ASM_LITTLE_ENDIAN = 1         
                @@/* little endian */                                        
                and     r5,  r8,  #0xF    @@// pre-read bit count          
                mov    r5,  r5,  lsr #1
                mov     r8,  r8, lsr  #4
                add     r8,  huf_table_ptr,  r8,lsl  #1
                rsb     r5,  r5,   #32                                        
                mov     r3,  codeword,   lsl  bit_consumed                    
                mov     r3,  r3,   lsr r5                                     
                add     r8,  r8,   r3,  lsl #1                                
                ldrsh   r8,  [r8]    
                rsb     r5,  r5,   #32                                           
                tst     r8,    #0x1
    @ELSE
    @            mov     r5,  r8, lsr  #12    @@// pre-read bit count          
    @            mov     r8,  r8, lsl  #20                                     
    @            add     r8,  huf_table_ptr,  r8,lsr #19                       
    @            rsb     r5,  r5,   #32                                        
    @            mov     r3,  codeword,   lsl  bit_consumed                    
    @            mov     r3,  r3,   lsr r5                                     
    @            add     r8,  r8,   r3,  lsl #1                                
    @            ldrsh   r8,  [r8]    
    @            rsb     r5,  r5,   #32                                           
    @            tst     r8,    #0x8000                                                       
    @ENDIF
                beq     SCF_HUFFMAN_DEC_NOISE_HCB_LOOP                              
SCF_HUFFMAN_DEC_NOISE_HCB_EXIT:        
    @IF AAC_DEC_ASM_LITTLE_ENDIAN = 1         
                @@/* little endian */       
                and     r5,  r8,    #0xF
                add     bit_consumed,  bit_consumed,   r5,  lsr  #1
                mov     r8,  r8,   lsr  #0x4
    @ELSE                                                  
    @            mov     r5,  r8,   lsl   #17                                  
    @            add     bit_consumed,  bit_consumed,   r5,  lsr  #29     
    @            and     r8,  r8,   #0xFF
    @ENDIF
                sub     r8,  r8,  #60
                                
                
                
                
AAC_DEC_HuffmanSCFAsmNOISE_HCB_BRANCHEXIT:                
                @@//////////////////
                add     noise_energy,  noise_energy,  r8
                strh    noise_energy,  [scalefac_ptr], #2   
                                
                @/////////////////////////////
                B       SCF_long_width_LOOP_NEXT_STEP
AAC_DEC_HuffmanSCFAsmINTENSITY_HCB:        
                @/////////////////////////////
                @/* INTENSITY_HCB, INTENSITY_HCB2:  intensity books */
                
                
                @@// first, pre-read 1 bit
                mov     r8,  codeword,  lsl   bit_consumed
                add     bit_consumed,  bit_consumed,  #1
                movs    r8,  r8,  lsr   #31                
                beq     SCF_HUFFMAN_DEC_INTENSITY_HCB_PROCESS                
                @/* huffman parsing */                
                mov     r3,  codeword,  lsl   bit_consumed                    
                mov     r3,  r3, lsr  #27   @@// first get 5 bit              
                add     r5,  huf_table_ptr,   r3,  lsl  #1                    
                ldrsh   r8,  [r5]                                             
                mov     r5,  #5               
    @IF AAC_DEC_ASM_LITTLE_ENDIAN = 1         
                @@/* little endian */      
                tst     r8,    #0x1
    @ELSE                               
    @            tst     r8,    #0x8000                                        
    @ENDIF
                bne     SCF_HUFFMAN_DEC_INTENSITY_HCB_EXIT                                   
SCF_HUFFMAN_DEC_INTENSITY_HCB_LOOP:                                                      
                @@//////////////////////////////        
    @IF AAC_DEC_ASM_LITTLE_ENDIAN = 1         
                @@/* little endian */
                add     bit_consumed,  bit_consumed,  r5                     
                and      r5,  r8,   #0xF    @@// pre-read bit count          
                mov     r5,  r5,  lsr  #1
                mov     r8,  r8, lsr  #4
                add     r8,  huf_table_ptr,  r8,lsl #1
                rsb     r5,  r5,   #32                                        
                mov     r3,  codeword,   lsl  bit_consumed                    
                mov     r3,  r3,   lsr r5                                     
                add     r8,  r8,   r3,  lsl #1                                
                ldrsh   r8,  [r8]    
                rsb     r5,  r5,   #32                                           
                tst     r8,    #0x1
    @ELSE                      
    @            add     bit_consumed,  bit_consumed,  r5                     
    @            mov     r5,  r8, lsr  #12    @@// pre-read bit count          
    @            mov     r8,  r8, lsl  #20                                     
    @            add     r8,  huf_table_ptr,  r8,lsr #19                       
    @            rsb     r5,  r5,   #32                                        
    @            mov     r3,  codeword,   lsl  bit_consumed                    
    @            mov     r3,  r3,   lsr r5                                     
    @            add     r8,  r8,   r3,  lsl #1                                
    @            ldrsh   r8,  [r8]    
    @            rsb     r5,  r5,   #32                                           
    @            tst     r8,    #0x8000                                                       
    @ ENDIF
                beq     SCF_HUFFMAN_DEC_INTENSITY_HCB_LOOP                              
SCF_HUFFMAN_DEC_INTENSITY_HCB_EXIT:     
    @IF AAC_DEC_ASM_LITTLE_ENDIAN = 1         
                @@/* little endian */
                and     r5,  r8,    #0xF
                add     bit_consumed,  bit_consumed,   r5,  lsr  #1
                mov     r8,  r8,   lsr #4
    @ELSE                                                      
    @            mov     r5,  r8,   lsl   #17                                  
    @            add     bit_consumed,  bit_consumed,   r5,  lsr  #29 
    @            and     r8,  r8,   #0xFF
    @ENDIF
                sub     r8,  r8,  #60
SCF_HUFFMAN_DEC_INTENSITY_HCB_PROCESS:                
                
                add     is_position_noise_flag, is_position_noise_flag, r8, lsl #16
                mov     r8,  is_position_noise_flag,  asr #16
                strh    r8,  [scalefac_ptr], #2                     
                
                
                
                @/////////////////////////////
                @@////////////////////////////////////////////// 
SCF_long_width_LOOP_NEXT_STEP:                
                @@/////////////////////////////////////////////                
                @//////////////////////////////////////////////
                @/* read bit */                
                cmp   bit_consumed,  #13
                ble   SCF_LOOP_PRE_BIT_EXIT
                @@/* read bit */
                add   bit_info,  bit_info,  bit_consumed,  lsl  #16                
                mov   codeword,  codeword,  lsl bit_consumed

                mov   r8, bit_info,  lsr   #8
                and   r8,  r8,  #0xFF     @@// get the bit_left
                sub   bit_info,  bit_info,  r8, lsl #8
                cmp   bit_consumed,  r8
                bgt   SCF_LOOP_PRE_BIT_BRANCH
                @///////////////////////////
                ldr   r3, [stream_ptr]
                rsb   r8,  r8,  #32

    @IF AAC_DEC_ASM_LITTLE_ENDIAN = 1       
        @@/* little endian */   
        EOR      r5,  r3, r3,  ROR #16
        MOV    r5,  r5,    LSR #8
        BIC       r5,  r5, #0xFF00
        EOR     r3,  r5, r3, ROR #8        
    @ENDIF

                mov   r3,  r3,   lsl   r8
                rsb   bit_consumed,  bit_consumed,  #32
                orr   codeword, codeword, r3,  lsr   bit_consumed                
                sub   r8,   bit_consumed,  r8
                orr   bit_info,  bit_info, r8,  lsl  #8  
                mov   bit_consumed,  #0              
                b     SCF_LOOP_PRE_BIT_EXIT
SCF_LOOP_PRE_BIT_BRANCH:     
                ldr   r3, [stream_ptr], #4
                sub   bit_consumed,  bit_consumed,  r8
    @IF AAC_DEC_ASM_LITTLE_ENDIAN = 1      
        @@/* little endian */  
        EOR      r5,  r3, r3,  ROR #16
        MOV    r5,  r5,    LSR #8
        BIC       r5,  r5, #0xFF00
        EOR     r3,  r5, r3, ROR #8        
    @ENDIF
                rsb   r8,  r8,  #32
                mov   r3,  r3,  lsl r8
                mov   r3,  r3,  lsr r8                               
                orr   codeword, codeword, r3,   lsl  bit_consumed
                ldr   r3,  [stream_ptr]
                rsb   r8,  bit_consumed,  #32
    @IF AAC_DEC_ASM_LITTLE_ENDIAN = 1        
        @@/* little endian */  
        EOR      r5,  r3, r3,  ROR #16
        MOV    r5,  r5,    LSR #8
        BIC       r5,  r5, #0xFF00
        EOR     r3,  r5, r3, ROR #8        
    @ENDIF
                orr   bit_info,  bit_info,  r8,  lsl #8
                orr   codeword, codeword, r3,  lsr  r8   
                mov   bit_consumed,  #0     
                                         
SCF_LOOP_PRE_BIT_EXIT:   

             
                @@///////////////////////////////////////////  
                subs   max_sfb, max_sfb, #256
                BGT    AAC_DEC_HuffmanSCFAsm_MAX_SFB_LOOP           
                
                add     max_sfb, max_sfb, #256
                add     max_sfb, max_sfb,   max_sfb,   lsl #8                 
                ldmfd   sp!, {scalefac_ptr, AAC_num_window_groups, sfb_cb_ptr}
                add     scalefac_ptr,  scalefac_ptr,  #51*2
                add     sfb_cb_ptr,    sfb_cb_ptr,  #51                
                @@///////////////////////////////////////////  
                subs   AAC_num_window_groups, AAC_num_window_groups, #1
                BGT    AAC_DEC_HuffmanSCFAsm_num_window_groups_LOOP
                @@///////////////////////////////////////////      
                
                mov     r0,  #0    @@// right
                B       SCF_HUFFMAN_ERROR_EXIT_JUMP
SCF_HUFFMAN_ERROR_EXIT:                
                ldmfd   sp!, {scalefac_ptr, AAC_num_window_groups, sfb_cb_ptr}
                mov     r0,  #4    @@// error eixt
SCF_HUFFMAN_ERROR_EXIT_JUMP:
                
                
                @@/* end */                
                ldmfd   sp!, {ld_ptr}                
                @@/* bit buffer update */
                add   bit_consumed, bit_consumed, bit_info, lsr #16
                ldrsh r7, [ld_ptr, #4]   @@// left bit
                ldrsh r9, [ld_ptr, #6]   @@// byte used                
                cmp    bit_consumed, r7
                suble  r7,  r7, bit_consumed
                strleh r7, [ld_ptr, #4]
                ble   AAC_DEC_ScaleFactorHuffAsm_EXIT
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
AAC_DEC_ScaleFactorHuffAsm_EXIT:                
                ldmfd   sp!, {r4-r11, pc}
                @ENDFUNC
                
                
                
   
                
                @//int32 AAC_DEC_HuffmanCB6Asm(
                @//                              int32                   *out_ptr,
                @//                              AAC_BIT_FIFO_FORMAT_T   *ld_ptr,
                @//                              int32                   *scalefac_ptr,
                @//                              int32                   *sect_sfb_offset_ptr
                @//                              )@
                

tmp_iq_table_ptr   .req  r2
tmp_iq_exp         .req  r7

AAC_DEC_HuffmanCB6Asm:@   FUNCTION
                @ // save the value for the following calculation
                .global  AAC_DEC_HuffmanCB6Asm
                
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
                
                
AAC_DEC_HuffmanCB6Asm_SFB_COUNT_LOOP:                
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
AAC_DEC_HuffmanCB6Asm_SFB_WIDTH_LOOP:
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
                bne     CB6_HUFFMAN_PARSING                
            
                @@//////////////////////////////                
                add     bit_consumed,  bit_consumed,  #6
    @IF AAC_DEC_ASM_LITTLE_ENDIAN = 1      
                @@/* little endian */
                and     r9,  r8,   #0xF    @@// pre-read bit count
                mov    r9,  r9,  lsr  #1
                mov     r8,  r8, lsr  #4
                add     r8,  huf_table_ptr,  r8,lsl #1
    @ELSE
    @            mov     r9,  r8, lsr  #12    @@// pre-read bit count
    @            mov     r8,  r8, lsl  #20
    @            add     r8,  huf_table_ptr,  r8,lsr #19                
    @ENDIF
                rsb     r6,  r9,   #32
                mov     r3,  codeword,   lsl  bit_consumed
                mov     r3,  r3,   lsr r6
                add     r8,  r8,   r3,  lsl #1                          
                ldrsh   r8,  [r8]                

CB6_HUFFMAN_PARSING:  
    @IF AAC_DEC_ASM_LITTLE_ENDIAN = 1      
                @@/* little endian */
                and     r9,  r8,     #0xF
                add     bit_consumed,  bit_consumed,   r9,  lsr  #1
                mov     r8,    r8,  lsr  #4                
                mov     r9,  r8,   lsr   #6    @@// get x, r9   
    @ELSE             
    @            mov     r9,  r8,   lsl   #17
    @            add     bit_consumed,  bit_consumed,   r9,  lsr  #29
    @            mov     r9,  r9,   lsl   #3                
    @            mov     r9,  r9,   lsr   #26    @@// get x, r9                
    @ENDIF
                @//////////////////////////////////////////////                 
                @@@/* IQ--left channel */
                @@//  r2, r3, r6, r9, r7      
                subs    r9,   r9,  #4                                  
    	        streq   r9,   [IQ_out_ptr], #4                              
    	        beq     CB6_long_width_LOOP_SECOND_POINT                  
    	        @@//   4/3 de-quantization 
    	        rsblt   r9,   r9,  #0
    	        add     r9,  tmp_iq_table_ptr,  r9, lsl #2
    	        ldr     r9,  [r9]    	            	           	        
    	        @@/* scale */    	        
                rsblt   r9,  r9,  #0                                                    
    	        smull   r9,  r3,  sfb_frac,  r9
    	            	        
    	        movs    r9,  tmp_iq_exp
    	        movge   r3,    r3,  lsl   r9                       
    	        rsblt   r9,    r9,  #0                             
    	        movlt   r3,    r3,  asr   r9    	            	                                                                    
    	        str     r3,    [IQ_out_ptr], #4
    	        
CB6_long_width_LOOP_SECOND_POINT:
                and     r8,  r8,   #0x3F         @@// get y: r8
                subs    r8,  r8,   #4
                @@@/* IQ--right channel */
                @@//  r6, r2, r3, r8, r7    	        
    	        streq   r8,   [IQ_out_ptr], #4
    	        beq     CB6_long_width_LOOP_NEXT_STEP    	        
    	        @@//   4/3 de-quantization    	        
    	        rsblt   r8,  r8,  #0
    	        add     r8,  tmp_iq_table_ptr,  r8, lsl #2
    	        ldr     r8,  [r8]    	           
    	            	        
    	        @@/* scale */    	        
    	        rsblt   r8,  r8,  #0    	               	        
    	        smull   r8,    r3,    sfb_frac,  r8    	        
    	        
    	        movs    r9,  tmp_iq_exp       
    	        movge   r3,    r3,  lsl   r9
    	        rsblt   r8,    r9,  #0
    	        movlt   r3,    r3,  asr   r8    	        
    	        str     r3,    [IQ_out_ptr], #4  
CB6_long_width_LOOP_NEXT_STEP:
                @@/* IQ end */                
                
                @@/////////////////////////////////////////////                
                @//////////////////////////////////////////////
                @/* read bit */                
                cmp   bit_consumed,  #21
                ble   CB6_LOOP_PRE_BIT_EXIT
                @@/* read bit */
                add   bit_info,  bit_info,  bit_consumed,  lsl  #16                
                mov   codeword,  codeword,  lsl bit_consumed

                mov   r8, bit_info,  lsr   #8
                and   r8,  r8,  #0xFF     @@// get the bit_left
                sub   bit_info,  bit_info,  r8, lsl #8
                cmp   bit_consumed,  r8
                bgt   CB6_LOOP_PRE_BIT_BRANCH
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
                b     CB6_LOOP_PRE_BIT_EXIT
CB6_LOOP_PRE_BIT_BRANCH:     
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
CB6_LOOP_PRE_BIT_EXIT:
                @///////////////////////////     
                                
                
                @@///////////////////////////////////////////  
                subs   r14, r14, #2
                BGT    AAC_DEC_HuffmanCB6Asm_SFB_WIDTH_LOOP           
                
                
                
                
                ldmfd   sp!, {r14, r2, r3}
                
                @@///////////////////////////////////////////  
                subs   AAC_SFB_COUNT, AAC_SFB_COUNT, #1
                BGT    AAC_DEC_HuffmanCB6Asm_SFB_COUNT_LOOP
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
                ble   AAC_DEC_HuffmanCB6Asm_EXIT
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
AAC_DEC_HuffmanCB6Asm_EXIT:                
                ldmfd   sp!, {r4-r12, pc}
                @ENDFUNC
                
                
                
                
                
                
                
    	                
        .end@END
        