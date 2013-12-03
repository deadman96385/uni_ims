                

                @ ***** END LICENSE BLOCK *****  

	@ ***** END LICENSE BLOCK *****  

	.text@AREA	|.text|, CODE, READONLY
	

        @@/* little endian mode */
                                
                @//uint32 MP3_DEC_32BitReadAsm(MP3_DEC_BIT_POOL_T *bitptr, 
                @//                         int32 len)@
@//  unsigned short cache@
@//  unsigned short left@
@//  unsigned long const *word_ptr@
@//  unsigned char const *byte@                


                @@ // r0, bitptr
                @@ // r1, len

                .extern  MP3_DEC_BitmaskTable
                .extern  MP3_DEC_iq_table
                .extern  MP3_DEC_root_table
                .extern  mp3_huff_pair_table
                .extern  MP3_DEC_huff_tabA
                .extern  MP3_DEC_huff_tabB
                
                @ // save the value for the following calculation                    
                
                
        @//int32 MP3_DEC_ReQuantizeAsm(int16 value, 
                @//                            int16 exp)@
                
                @//  r0: value
                @//  r1: exp
                
Q_exp         .req   r1
Q_t           .req   r2                
Q_frac        .req   r2





Q_x1          .req   r0
Q_x2          .req   r3
Q_table_ptr   .req   r3

Q_value       .req   r4

Q_fx          .req   r14

MP3_DEC_ReQuantizeInternalAsm:@   FUNCTION
                
                .global  MP3_DEC_ReQuantizeInternalAsm                
                stmfd  sp!, {r0-r2, r14}
                @@/////////////////////
                ldr     Q_table_ptr, =MP3_DEC_iq_table       
                mov     Q_exp, r8, asr #16                       
                @@///////////////////////                
                cmp     Q_value,  #256
                addlt     Q_table_ptr, Q_table_ptr, Q_value, lsl #2
                ldrlt     Q_value, [Q_table_ptr]
                blt       MP3_DEC_ReQuantizeInternalAsmQ_BRANCH_EXIT
MP3_DEC_ReQuantizeInternalAsmQ_NEXT_BRANCH0:
                @@////'
                cmp     Q_value,  #2048
                BGE     MP3_DEC_ReQuantizeInternalAsmQ_NEXT_BRANCH1
                @@// >= 256
                mov     Q_t,  Q_value, lsr  #3
                add     Q_table_ptr,  Q_table_ptr, Q_t, lsl #2
                ldmia   Q_table_ptr, {Q_x1, Q_x2}
                and     Q_t,  Q_value, #7
                rsb     Q_fx,   Q_value, #2048
                add     Q_fx,   Q_fx, #844     @@// 2892 - value
                mov     Q_fx,  Q_fx, lsr #7
                
                cmp     Q_t,  #4
                ble     MP3_DEC_ReQuantizeInternalAsmQ_NEXT_BRANCH0_branch
                @@// > 4
                sub     Q_x1,   Q_x2, Q_x1  @@// x2 - x1
                add     Q_x1,   Q_fx, Q_x1
                rsb     Q_t, Q_t, #8
                mul     Q_x1,  Q_t,  Q_x1
                sub     Q_x2,  Q_x2, Q_x1, asr #3
                mov     Q_value,  Q_x2, asl #4                
                
                B       MP3_DEC_ReQuantizeInternalAsmQ_BRANCH_EXIT
MP3_DEC_ReQuantizeInternalAsmQ_NEXT_BRANCH0_branch:
                @// <= 4
                sub     Q_x2,   Q_x2, Q_x1  @@// x2 - x1
                sub     Q_x2,   Q_x2, Q_fx
                mul     Q_x2,  Q_t,  Q_x2
                add     Q_x2,  Q_x1, Q_x2, asr #3
                mov     Q_value,  Q_x2, asl #4 
                
                B       MP3_DEC_ReQuantizeInternalAsmQ_BRANCH_EXIT        
                
MP3_DEC_ReQuantizeInternalAsmQ_NEXT_BRANCH1:
                @@// >= 2048
                
                mov     Q_t,  Q_value, lsr  #6
                add     Q_table_ptr,  Q_table_ptr, Q_t, lsl #2
                ldmia   Q_table_ptr, {Q_x1, Q_x2}
                and     Q_t,  Q_value, #63
                rsb     Q_fx,   Q_value, #13440   @@// 13440 - value
                mov     Q_fx,  Q_fx, lsr #9
                
                cmp     Q_t,  #32
                ble     MP3_DEC_ReQuantizeInternalAsmQ_NEXT_BRANCH1_branch
                @@// > 4
                sub     Q_x1,   Q_x2, Q_x1  @@// x2 - x1
                add     Q_x1,   Q_fx, Q_x1
                rsb     Q_t, Q_t, #64
                mul     Q_x1,  Q_t,  Q_x1
                sub     Q_x2,  Q_x2, Q_x1, asr #6
                mov     Q_value,  Q_x2, asl #8                
                
                B       MP3_DEC_ReQuantizeInternalAsmQ_BRANCH_EXIT
MP3_DEC_ReQuantizeInternalAsmQ_NEXT_BRANCH1_branch:                
                @// <= 4
                sub     Q_x2,   Q_x2, Q_x1  @@// x2 - x1
                sub     Q_x2,   Q_x2, Q_fx
                mul     Q_x2,  Q_t,  Q_x2
                add     Q_x2,  Q_x1, Q_x2, asr #6
                mov     Q_value,  Q_x2, asl #8 
                
MP3_DEC_ReQuantizeInternalAsmQ_BRANCH_EXIT:                
                @@////
                cmp     Q_exp,  #-196
                movle   Q_value,  #0
                ble     MP3_DEC_ReQuantizeInternalAsmMP3_DEC_ReQuantizeAsm_EXIT                
                
                cmp     Q_exp, #0
                andge   Q_frac, Q_exp, #3
                movge   Q_exp,  Q_exp, lsr #2
                bge     MP3_DEC_ReQuantizeInternalAsmQ_exp_exit       
                
                rsb     Q_exp,  Q_exp, #0
                and     Q_frac, Q_exp, #3
                mov     Q_exp,  Q_exp, lsr #2
                
                rsb     Q_exp,  Q_exp, #0
                rsb     Q_frac, Q_frac, #0
MP3_DEC_ReQuantizeInternalAsmQ_exp_exit:  
                
                adds     Q_exp,  Q_exp,  #17
                movges   Q_value, Q_value, asl Q_exp
                rsblt    Q_exp, Q_exp, #0 
                movlts   Q_value, Q_value, asr Q_exp
                @//beq      MP3_DEC_ReQuantizeInternalAsmMP3_DEC_ReQuantizeAsm_EXIT
                
                cmpne      Q_frac,  #0
                @//cmpne   Q_value, #0
                beq     MP3_DEC_ReQuantizeInternalAsmMP3_DEC_ReQuantizeAsm_EXIT
                
                ldr     Q_table_ptr, =MP3_DEC_root_table
                add     Q_frac, Q_frac, #3
                add     Q_table_ptr, Q_table_ptr, Q_frac, lsl #2
                
                ldr     Q_table_ptr, [Q_table_ptr]                
                smull	Q_frac, Q_exp, Q_value, Q_table_ptr
       		    movs	Q_frac,Q_frac,lsr#28
		        adc		Q_value, Q_frac, Q_exp, lsl#4                            
                
                
                @@///////////////////////
MP3_DEC_ReQuantizeInternalAsmMP3_DEC_ReQuantizeAsm_EXIT:                                
                @@/////////////////////
                ldmfd  sp!, {r0-r2, pc}                                
                
                @ENDFUNC
                                
                
                
                
                @//int32 MP3_DEC_HuffmanParsingAsm(struct mp3_bitptr *peek_ptr, 
                @//                                int32             *tst_data_ptr
                @//                                int32             *xr_ptr,)@
                
                @//  r0: peek_ptr
                @//  r1: xr_ptr
                @//  r2: tst_data_ptr
                
                
                @@// tst_data_ptr[0] = bits_left
                @@// tst_data_ptr[1] = bitcache
                @@// tst_data_ptr[2] = cachesz
                
                @@// tst_data_ptr[3] = sfbwidth_ptr @@// 0
                @@// tst_data_ptr[4] = exp_ptr      @@// 1
                @@// tst_data_ptr[5] = channel_ptr  @@// 2
                @@// tst_data_ptr[6] = big_values   @@// 3
                @@// tst_data_ptr[7] : exp          @@// 4
                @@// tst_data_ptr[8] : reqhits      @@// 5
                
peek_ptr             .req  r0                
xr_ptr               .req  r2 
tst_data_ptr         .req  r1

data0_left           .req  r3
H_clumpsz            .req  r3
data1_ptr            .req  r4

table_ptr            .req  r6
rcount_region        .req  r7  @@// 31:16, rcount, 15:0, region
exp_linbits          .req  r8
H_bits_left          .req  r9
H_bitcache           .req  r10
H_cachesz            .req  r11

sfboundary_ptr       .req  r12
big_values           .req  r14
MP3_DEC_HuffmanParsingAsm:@   FUNCTION
                @ // save the value for the following calculation
                .global  MP3_DEC_HuffmanParsingAsm
                stmfd   sp!, {r4-r12, r14}
                
                
                ldrh    big_values, [tst_data_ptr, #24]
                
                ldmia   tst_data_ptr!, {H_bits_left-H_cachesz}
                add    sfboundary_ptr, xr_ptr, #572*4
                str     sfboundary_ptr, [tst_data_ptr, #16]
                
                
                
                
                mov     rcount_region,  #0
                mov     exp_linbits,  #0
                
                
                mov     sfboundary_ptr,  xr_ptr
                
                
                
                cmp     big_values, #0
                ble     MP3_DEC_HuffmanParsingAsm_EXIT
                
                @@//while (big_values--) 
MP3_DEC_HUFF_BIG_VALUE_LOOP:
                @///////////
                cmp     xr_ptr, sfboundary_ptr
                bne     sfboundary_ptr_EXIT
                @@//  if (xrptr == sfboundary_ptr) 
                ldmia   tst_data_ptr, {r4-r5}
                ldrb    r3,   [r4], #1  @@// get sfb_width
                ldr     r14,  [r5], #4  @@// get exp
                stmia   tst_data_ptr, {r4,r5} @//store   sfbwidth_ptr, exp_ptr
                add     sfboundary_ptr, sfboundary_ptr, r3, lsl #2  @@// sfboundary_ptr += *sfbwidth_ptr++@	
                mov     r3,  exp_linbits, asr #16
                cmp     r3,   r14
                andne   r5,   exp_linbits, #0xFF
                movne   exp_linbits, r14, asl #16
                orrne   exp_linbits, exp_linbits, r5
                movne   r14, #0
                strne   r14,  [tst_data_ptr, #20]
                
                subs    rcount_region, rcount_region, #65536     
                bge     rcount_exit
                ldr     table_ptr,  [tst_data_ptr, #8]
                add     rcount_region, rcount_region, #65536
                and     rcount_region, rcount_region, #0xff
                add     r3,  table_ptr,   rcount_region
                ldrb     r4,  [r3]
                ldrb     r5,  [r3, #3]
                ldr     table_ptr, =mp3_huff_pair_table
                add     rcount_region, rcount_region, r4,  asl #16
                add     r4, table_ptr, r5, lsl #3
                ldr     table_ptr, [r4]
                ldr     r5, [r4, #4]
                add     rcount_region,  rcount_region,  #1
                mov     exp_linbits,  exp_linbits, asr #16
                orr     exp_linbits, r5, exp_linbits, asl #16
                
rcount_exit:                
                @@//  quit the boundary
sfboundary_ptr_EXIT:                
                @/////////
                @@/* read the stream */
                cmp  H_cachesz, #21                   
                bge   cachesz0_EXIT
                
                @@/* if (cachesz < 21) */
                @@/* register: r3, r4, r5, r14*/
                rsb   H_cachesz, H_cachesz, #31                
                @@/**/
                ldmia   peek_ptr,  {data0_left, data1_ptr}
                mov     H_bitcache, H_bitcache, lsl H_cachesz
                
                ldr     r5,  [data1_ptr]       
                
                sub   H_bits_left, H_bits_left, H_cachesz
                         
                cmp     H_cachesz,   data0_left
                @@// little->big endian                
                EOR     r14,  r5, r5, ROR #16
                MOV     r14,  r14, LSR #8
                BIC     r14,  r14, #0xFF00
                EOR     r5, r14, r5, ROR #8                 
                bgt     MP3_DEC_WordRead1Asm_BRANCH
                @//if (len <= bitptr->left)
                rsb     r14,   data0_left, #32
                mov     r5,    r5,  lsl  r14                
                sub     data0_left,  data0_left,  H_cachesz
                str     data0_left,   [r0]                
                rsb     r14,   H_cachesz,      #32     
                orr     H_bitcache,     H_bitcache,    r5,  lsr  r14                
                
                mov     H_cachesz, #31
                b      cachesz0_EXIT
MP3_DEC_WordRead1Asm_BRANCH:                
                @// else                
                sub     H_cachesz,  H_cachesz,  data0_left
                rsb     data0_left, data0_left, #32
                mov     r5,   r5,   lsl data0_left                
                sub     data0_left,  data0_left,  H_cachesz
                orr     H_bitcache,  H_bitcache,  r5,  lsr  data0_left
                                
                ldr     r5, [data1_ptr, #4]                
                add     data1_ptr,  data1_ptr, #4
                rsb     data0_left,   H_cachesz,  #32
                stmia   peek_ptr,  {data0_left, data1_ptr}                
                @@// little->big endian                
                EOR     r14,  r5, r5, ROR #16
                MOV     r14,  r14, LSR #8                               
                BIC     r14,  r14, #0xFF00
                EOR     r5, r14, r5, ROR #8                
                orr     H_bitcache,  H_bitcache,  r5,  lsr   data0_left
                
                mov     H_cachesz, #31
cachesz0_EXIT:                
                @/* huffman parsing */
                @/* register: r3, r4, r5, r14*/
                rsb    H_clumpsz, H_cachesz, #32
                mov    r4,  H_bitcache, lsl H_clumpsz  
                
                
                mov    r4,  r4,  lsr #28
                add    r4,  table_ptr, r4, lsl #1                
                ldrh   r4, [r4]                
                mov    H_clumpsz, #4
                
                @@// little endian
                @@//<----------------- 
                @@//0000 0001 0000 0110

                tst  r4, #1
                bne  HUFF_DEC_EXIT
HUFF_DEC_LOOP:                
                sub   H_cachesz, H_cachesz, H_clumpsz
                @//cachesz -= clumpsz@				
                @//clumpsz  = pair->ptr.bits@
                @//pair     = &table[pair->ptr.offset + MASK(bitcache, cachesz, clumpsz)]@
                mov   H_clumpsz, r4, lsr #1
                and   H_clumpsz, H_clumpsz, #0x7
                mov   r4,  r4, lsr #4
                
                add   r4, table_ptr, r4, lsl #1                
                
                rsb    r5, H_cachesz, #32
                mov    r14,  H_bitcache, lsl r5
                rsb    r5, H_clumpsz, #32
                mov    r14,  r14, lsr r5
                add    r4,  r4, r14, lsl #1                 
                ldrh   r4, [r4]     
                   
                tst    r4, #1
                beq    HUFF_DEC_LOOP
HUFF_DEC_EXIT:                
                
                @//cachesz -= pair->value.hlen@
                @//valuex   = pair->value.x@	
                @//valuey   = pair->value.y@
                mov   r5,   r4,  lsr #1
                and   H_clumpsz, r5, #0x7
                sub   H_cachesz, H_cachesz, H_clumpsz
                
                
                
                
                
                
                ands   r3, exp_linbits, #0xFF
                @@/* if (linbits)  */
                bne    HUFF_ESC_BRANCH                
                @@/* no esc word */
                mov    r5,   r5, lsr #3
                ands   r4,   r5, #0xF
                streq  r4,   [xr_ptr], #4
                beq    huff_NO_ESC_Another
                
                ldr    r3,   [tst_data_ptr, #20]
                mov    r14, #1
                ands   r14,  r3, r14, lsl r4
                addne  r3,  tst_data_ptr, r4, lsl #2
                ldrne  r4,  [r3, #24]
                bne    huff_NO_ESC_THIS_FREQ_BRANCH 
                mov    r14, #1               
                orr    r3,  r3, r14, lsl r4
                str    r3,  [tst_data_ptr, #20]
                @@// IQ
                BL     MP3_DEC_ReQuantizeInternalAsm   
                                
                and    r3,  r5, #0xF
                add    r3,  tst_data_ptr, r3, lsl #2
                str    r4,  [r3, #24]
                             
huff_NO_ESC_THIS_FREQ_BRANCH:                               
                sub    H_cachesz, H_cachesz, #1
                mov    r3, #1
                tst    H_bitcache, r3,  lsl H_cachesz
                rsbne  r4,  r4, #0                
                str    r4,   [xr_ptr], #4                
huff_NO_ESC_Another:
                movs   r4,   r5, lsr #4
                streq  r4,   [xr_ptr], #4
                beq    HUFF_NEXT_PAIR_B
                
                ldr    r3,   [tst_data_ptr, #20]
                mov    r14, #1
                ands   r14,  r3, r14, lsl r4
                addne  r3,  tst_data_ptr, r4, lsl #2
                ldrne  r4,  [r3, #24]
                bne    huff_NO_ESC_THIS_ANOTHER_BRANCH  
                mov    r14, #1              
                orr    r3,  r3, r14, lsl r4
                str    r3,  [tst_data_ptr, #20]
                
                @@// IQ
                BL     MP3_DEC_ReQuantizeInternalAsm                    
                mov    r3,   r5, lsr #4
                add    r3,  tst_data_ptr, r3, lsl #2
                str    r4,  [r3, #24]            
huff_NO_ESC_THIS_ANOTHER_BRANCH:                
                sub    H_cachesz, H_cachesz, #1
                mov    r3, #1
                tst    H_bitcache, r3,  lsl H_cachesz
                
                rsbne  r4,  r4, #0                
                str    r4,   [xr_ptr], #4       
                
                
                
                
                
                
                
                B    HUFF_NEXT_PAIR_B
HUFF_ESC_BRANCH:
                @@/* esc word processing */
                mov    r5,   r5, lsr #3
                ands   r4,   r5, #0xF
                streq  r4,   [xr_ptr], #4
                beq    huff_ESC_Another
                cmp    r4,  #15
                bne    huff_ESCMODE_1_14                
                add    r3, r3, #2
                
                cmp    H_cachesz, r3
                bge    huff_ESCMODE_15_WORD_READ_SKIP
                @@/* read stream */
                stmfd   sp!, {r4, r5}
                ldmia   peek_ptr,  {data0_left, data1_ptr}
                
                rsb     H_cachesz,  H_cachesz, #31
                
                sub   H_bits_left, H_bits_left, H_cachesz
                
                mov     H_bitcache, H_bitcache, lsl H_cachesz
                
                ldr     r5,  [data1_ptr]                
                cmp     H_cachesz,   data0_left
                @@// little->big endian                
                EOR     r14,  r5, r5, ROR #16
                MOV     r14,  r14, LSR #8
                BIC     r14,  r14, #0xFF00
                EOR     r5, r14, r5, ROR #8                 
                bgt     MP3_DEC_WordRead1Asm_BRANCH0
                @//if (len <= bitptr->left)
                rsb     r14,   data0_left, #32
                mov     r5,    r5,  lsl  r14                
                sub     data0_left,  data0_left,  H_cachesz
                str     data0_left,   [r0]                
                rsb     r14,   H_cachesz,      #32     
                orr     H_bitcache,     H_bitcache,    r5,  lsr  r14                
                b      cachesz0_EXIT0
MP3_DEC_WordRead1Asm_BRANCH0:                
                @// else                
                sub     H_cachesz,  H_cachesz,  data0_left
                rsb     data0_left, data0_left, #32
                mov     r5,   r5,   lsl data0_left
                sub     data0_left,  data0_left, H_cachesz
                orr     H_bitcache, H_bitcache, r5, lsr data0_left
                
                ldr     r5, [data1_ptr, #4]                
                add     data1_ptr,  data1_ptr, #4
                rsb     data0_left,   H_cachesz,  #32
                stmia   peek_ptr,  {data0_left, data1_ptr}                
                @@// little->big endian                
                EOR     r14,  r5, r5, ROR #16
                MOV     r14,  r14, LSR #8                               
                BIC     r14,  r14, #0xFF00
                EOR     r5, r14, r5, ROR #8                
                orr     H_bitcache,  H_bitcache,  r5,  lsr   data0_left

                
cachesz0_EXIT0:        
                mov     H_cachesz, #31
                ldmfd   sp!, {r4, r5}
                
huff_ESCMODE_15_WORD_READ_SKIP:                
                rsb     r14,  H_cachesz, #32
                mov     r3,  H_bitcache, lsl r14                
                and     r14, exp_linbits, #0xFF
                rsb     r14,  r14, #32
                add     r4,  r4, r3, lsr r14      
                
                and     r3, exp_linbits, #0xFF          
                sub     H_cachesz, H_cachesz, r3
                
                BL       MP3_DEC_ReQuantizeInternalAsm
                sub    H_cachesz, H_cachesz, #1
                mov    r3, #1
                tst    H_bitcache, r3,  lsl H_cachesz
                rsbne   r4, r4, #0
                str     r4,   [xr_ptr], #4  
                B       huff_ESC_Another
huff_ESCMODE_1_14:                
                
                ldr    r3,   [tst_data_ptr, #20]
                mov    r14, #1
                ands   r14,  r3, r14, lsl r4
                addne  r3,  tst_data_ptr, r4, lsl #2
                ldrne  r4,  [r3, #24]
                bne    huff_ESC_THIS_FREQ_BRANCH    
                mov    r14, #1            
                orr    r3,  r3, r14, lsl r4
                str    r3,  [tst_data_ptr, #20]
                @@// IQ
                BL     MP3_DEC_ReQuantizeInternalAsm     
                
                and    r3,  r5, #0xF
                add    r3,  tst_data_ptr, r3, lsl #2
                str    r4,  [r3, #24]           
huff_ESC_THIS_FREQ_BRANCH:                               
                sub    H_cachesz, H_cachesz, #1
                mov    r3, #1
                tst    H_bitcache, r3,  lsl H_cachesz
                rsbne  r4,  r4, #0                
                str    r4,   [xr_ptr], #4 
                
huff_ESC_Another:                
                @/* another point */
                movs   r4,   r5, lsr #4                
                streq  r4,   [xr_ptr], #4
                beq    HUFF_NEXT_PAIR_B
                cmp    r4,  #15
                bne    huff_ESCANOTHERPOINT_1_14
                ands   r3, exp_linbits, #0xFF
                add    r3, r3, #1               
             
                
                cmp    H_cachesz, r3
                bge    huff_ESCANOTHERPOINT_15_WORD_READ_SKIP
                @@/* read stream */
                stmfd   sp!, {r4, r5}
                ldmia   peek_ptr,  {data0_left, data1_ptr}
                
                rsb     H_cachesz,  H_cachesz, #31
                
                sub   H_bits_left, H_bits_left, H_cachesz
                
                mov     H_bitcache, H_bitcache, lsl H_cachesz
                
                ldr     r5,  [data1_ptr]                
                cmp     H_cachesz,   data0_left
                @@// little->big endian                
                EOR     r14,  r5, r5, ROR #16
                MOV     r14,  r14, LSR #8
                BIC     r14,  r14, #0xFF00
                EOR     r5, r14, r5, ROR #8                 
                bgt     MP3_DEC_WordRead1Asm_BRANCH1
                @//if (len <= bitptr->left)
                rsb     r14,   data0_left, #32
                mov     r5,    r5,  lsl  r14                
                sub     data0_left,  data0_left,  H_cachesz
                str     data0_left,   [r0]                
                rsb     r14,   H_cachesz,      #32     
                orr     H_bitcache,     H_bitcache,    r5,  lsr  r14                
                b      cachesz0_EXIT1
MP3_DEC_WordRead1Asm_BRANCH1:               
                @// else                
                sub     H_cachesz,  H_cachesz,  data0_left
                rsb     data0_left, data0_left, #32
                mov     r5,   r5,   lsl data0_left
                sub     data0_left,  data0_left, H_cachesz
                orr     H_bitcache, H_bitcache, r5, lsr data0_left
                
                ldr     r5, [data1_ptr, #4]                
                add     data1_ptr,  data1_ptr, #4
                rsb     data0_left,   H_cachesz,  #32
                stmia   peek_ptr,  {data0_left, data1_ptr}                
                @@// little->big endian                
                EOR     r14,  r5, r5, ROR #16
                MOV     r14,  r14, LSR #8                               
                BIC     r14,  r14, #0xFF00
                EOR     r5, r14, r5, ROR #8                
                orr     H_bitcache,  H_bitcache,  r5,  lsr   data0_left
                
cachesz0_EXIT1:       
                mov     H_cachesz, #31
                ldmfd   sp!, {r4, r5}
huff_ESCANOTHERPOINT_15_WORD_READ_SKIP:
                and     r3, exp_linbits, #0xFF
                rsb     r14,  H_cachesz, #32
                mov     r5,  H_bitcache, lsl r14
                rsb     r14,  r3, #32
                add     r4,  r4, r5, lsr r14                
                sub     H_cachesz, H_cachesz, r3
                
                BL      MP3_DEC_ReQuantizeInternalAsm
                sub    H_cachesz, H_cachesz, #1
                mov    r3, #1
                tst    H_bitcache, r3,  lsl H_cachesz
                rsbne   r4, r4, #0
                str     r4,   [xr_ptr], #4  
                B       HUFF_NEXT_PAIR_B
huff_ESCANOTHERPOINT_1_14:
                ldr    r3,   [tst_data_ptr, #20]
                mov    r14, #1
                ands   r14,  r3, r14, lsl r4
                addne  r3,  tst_data_ptr, r4, lsl #2
                ldrne  r4,  [r3, #24]
                bne    huff_ESC_ANOTHER_FREQ_BRANCH 
                mov    r14, #1               
                orr    r3,  r3, r14, lsl r4
                str    r3,  [tst_data_ptr, #20]
                @@// IQ
                BL     MP3_DEC_ReQuantizeInternalAsm    
                mov    r3,  r5, lsr #4
                add    r3,  tst_data_ptr, r3, lsl #2
                str    r4,  [r3, #24]
                            
huff_ESC_ANOTHER_FREQ_BRANCH:                               
                sub    H_cachesz, H_cachesz, #1
                mov    r3, #1
                tst    H_bitcache, r3,  lsl H_cachesz
                rsbne  r4,  r4, #0                
                str    r4,   [xr_ptr], #4 
HUFF_NEXT_PAIR_B:
                
                @@// load tst_data_ptr
                @@//   
                ldr   big_values, [tst_data_ptr, #12]
                subs  big_values, big_values, #1                
                str   big_values, [tst_data_ptr, #12]
                
                BGT   MP3_DEC_HUFF_BIG_VALUE_LOOP                 
MP3_DEC_HuffmanParsingAsm_EXIT:
                
@//peek_ptr             .req  r0                
@//xr_ptr               .req  r2 
@//tst_data_ptr         .req  r1

@//data0_left           .req  r3
@//H_clumpsz            .req  r3
@//data1_ptr            .req  r4

@//table_ptr            .req  r6
@//rcount_region        .req  r7  @@// 31:16, rcount, 15:0, region
@//exp_linbits          .req  r8
@//H_bits_left          .req  r9
@//H_bitcache           .req  r10
@//H_cachesz            .req  r11                
                ldr     r3,   [tst_data_ptr, #4*4]
                @//add     r3,   r3,   #4*4
                
                cmp     r3,   xr_ptr
                @//moveq   r0,   #576
                blt     MP3_DEC_HuffmanParsingCount1Asm_EXIT1 @@// 574 or 576
                
                
                @@// count 1 parsing
                add     r3, H_bits_left, H_cachesz
		cmp	r3, #-31				@@// loose a little bit of error handling
                					  	@@// if (cachesz + bits_left <= 0)->MP3_DEC_HuffmanParsingCount1Asm_EXIT
                movlt   r0, #0x2380000
                blt     MP3_DEC_HuffmanParsingZEROAsm_EXIT
                
		cmp	r3, #0
		ble     MP3_DEC_HuffmanParsingCount1Asm_EXIT
                
                
                
                
                
                ldr     table_ptr,  [tst_data_ptr, #8] 
                                
                mov     r4, #1
                BL MP3_DEC_ReQuantizeInternalAsm             
                
                ldrh    r5,  [table_ptr, #8]
                ands    r5,  r5, #1
                ldreq   table_ptr, =MP3_DEC_huff_tabA
                ldrne   table_ptr, =MP3_DEC_huff_tabB
MP3_DEC_HuffmanParsingCount1Asm_LOOP:                
                @@/////////////////////////////////
                @@/* read stream */
                cmp      H_cachesz, #10
                BGE      MP3_DEC_HuffmanParsingCount1_ReadStream_SKIP
                
                @@/* if (cachesz < 10) */
                @@/* register: r3, r5, r14*/
                rsb   H_cachesz, H_cachesz, #31                
                @@/**/
                ldmia   peek_ptr,  {data0_left, r7}
                mov     H_bitcache, H_bitcache, lsl H_cachesz
                
                ldr     r5,  [r7]       
                
                sub   H_bits_left, H_bits_left, H_cachesz
                         
                cmp     H_cachesz,   data0_left
                @@// little->big endian                
                EOR     r14,  r5, r5, ROR #16
                MOV     r14,  r14, LSR #8
                BIC     r14,  r14, #0xFF00
                EOR     r5, r14, r5, ROR #8                 
                bgt     MP3_DEC_Count1_WordRead1Asm_BRANCH
                @//if (len <= bitptr->left)
                rsb     r14,   data0_left, #32
                mov     r5,    r5,  lsl  r14                
                sub     data0_left,  data0_left,  H_cachesz
                str     data0_left,   [r0]                
                rsb     r14,   H_cachesz,      #32     
                orr     H_bitcache,     H_bitcache,    r5,  lsr  r14                
                
                mov     H_cachesz, #31
                b      MP3_DEC_HuffmanParsingCount1_ReadStream_SKIP
MP3_DEC_Count1_WordRead1Asm_BRANCH:                
                @// else                
                sub     H_cachesz,  H_cachesz,  data0_left
                rsb     data0_left, data0_left, #32
                mov     r5,   r5,   lsl data0_left                
                sub     data0_left,  data0_left,  H_cachesz
                orr     H_bitcache,  H_bitcache,  r5,  lsr  data0_left
                                
                ldr     r5, [r7, #4]                
                add     r7,  r7, #4
                rsb     data0_left,   H_cachesz,  #32
                stmia   peek_ptr,  {data0_left, r7}                
                @@// little->big endian                
                EOR     r14,  r5, r5, ROR #16
                MOV     r14,  r14, LSR #8                               
                BIC     r14,  r14, #0xFF00
                EOR     r5, r14, r5, ROR #8                
                orr     H_bitcache,  H_bitcache,  r5,  lsr   data0_left
                
                mov     H_cachesz, #31
                
MP3_DEC_HuffmanParsingCount1_ReadStream_SKIP:                
                @@/* huff dec */
                rsb    H_clumpsz, H_cachesz, #32
                mov    r7,  H_bitcache, lsl H_clumpsz                
                
                mov    r7,  r7,  lsr #28
                add    r7,  table_ptr, r7, lsl #1                
                ldrh   r7, [r7]                
                mov    H_clumpsz, #4                
                @@// little endian
                @@//<----------------- 
                @@//0000 0001 0000 0110
                tst  r7, #1
                bne  Count1_HUFF_DEC_LOOP
               
                sub   H_cachesz, H_cachesz, H_clumpsz
                @//cachesz -= clumpsz@				
                @//clumpsz  = pair->ptr.bits@
                @//pair     = &table[pair->ptr.offset + MASK(bitcache, cachesz, clumpsz)]@
                mov   H_clumpsz, r7, lsr #1
                and   H_clumpsz, H_clumpsz, #0x7
                mov   r7,  r7, lsr #4
                add   r7, table_ptr, r7, lsl #1                                
                rsb    r5, H_cachesz, #32
                mov    r14,  H_bitcache, lsl r5
                rsb    r5, H_clumpsz, #32
                mov    r14,  r14, lsr r5
                add    r7,  r7, r14, lsl #1                 
                ldrh   r7, [r7] 
Count1_HUFF_DEC_LOOP:   

                @//cachesz -= pair->value.hlen@
                mov   r7,   r7,  lsr #1
                and   H_clumpsz, r7, #0x7
                sub   H_cachesz, H_cachesz, H_clumpsz
                
                
                cmp   xr_ptr, sfboundary_ptr
                bne   Count1_Boundary_EXIT0
                ldmia tst_data_ptr, {r3, r5}
                ldrb  r14, [r3], #1                
                add   sfboundary_ptr, sfboundary_ptr, r14, lsl #2
                ldr   r14, [r5], #4
                stmia tst_data_ptr, {r3, r5}
                mov   exp_linbits, exp_linbits, asr #16
                cmp   r14, exp_linbits, asr #16
                movne exp_linbits, r14, asl #16   
                beq   Count1_Boundary_EXIT0
                mov   r4, #1
                BL MP3_DEC_ReQuantizeInternalAsm                
Count1_Boundary_EXIT0:
                @////////////////////////
                mov   r7,   r7,  lsr #3
                ands  r3,   r7, #0x1
                beq   Count1_v_skip
                sub   H_cachesz, H_cachesz, #1
                tst   H_bitcache, r3, lsl H_cachesz
                rsbne r3, r4, #0
                moveq r3, r4                
Count1_v_skip:    
                mov   r7,   r7,  lsr #1
                ands  r5,   r7, #0x1
                beq   Count1_w_skip
                sub   H_cachesz, H_cachesz, #1
                tst   H_bitcache, r5, lsl H_cachesz
                rsbne r5, r4, #0
                moveq r5, r4                
Count1_w_skip:   
                stmia  xr_ptr!, {r3, r5}
                @/////////////////////////
                
                cmp   xr_ptr, sfboundary_ptr
                bne   Count1_Boundary_EXIT1
                ldmia tst_data_ptr, {r3, r5}
                ldrb  r14, [r3], #1                
                add   sfboundary_ptr, sfboundary_ptr, r14, lsl #2
                ldr   r14, [r5], #4
                stmia tst_data_ptr, {r3, r5}
                cmp   r14, exp_linbits, asr #16
                movne exp_linbits, r14, asl #16     
                beq   Count1_Boundary_EXIT1
                mov   r4, #1
                BL MP3_DEC_ReQuantizeInternalAsm              
Count1_Boundary_EXIT1:
                @////////////////////////
                mov   r7,   r7,  lsr #1
                ands  r3,   r7, #0x1
                beq   Count1_x_skip
                sub   H_cachesz, H_cachesz, #1
                tst   H_bitcache, r3, lsl H_cachesz
                rsbne r3, r4, #0
                moveq r3, r4                
Count1_x_skip:    
                movs   r5,   r7,  lsr #1
                @//ands  r5,   r7, #0x1
                beq   Count1_y_skip
                sub   H_cachesz, H_cachesz, #1
                tst   H_bitcache, r5, lsl H_cachesz
                rsbne r5, r4, #0
                moveq r5, r4                
Count1_y_skip:   
                ldr   r7, [tst_data_ptr, #16] 
                @//cmp   xr_ptr, r7
                @//bge   MP3_DEC_HuffmanParsingCount1Asm_EXIT
                
                stmia  xr_ptr!, {r3, r5}  
                               
                @@/////////////////////////////////
                adds    r3, H_bits_left, H_cachesz          
                ble     MP3_DEC_HuffmanParsingCount1Asm_EXIT        
                cmpgt   r7, xr_ptr
                bge     MP3_DEC_HuffmanParsingCount1Asm_LOOP
                
                
                                
MP3_DEC_HuffmanParsingCount1Asm_EXIT:             
                cmp     r3,  #0
                sublt   xr_ptr,  xr_ptr, #16
                
                
                
                     
MP3_DEC_HuffmanParsingCount1Asm_EXIT1:
                @@////////////////////////////
                @@/* zero */                
                ldr    sfboundary_ptr, [tst_data_ptr, #16]
                mov    r6, #0                
                add    sfboundary_ptr, sfboundary_ptr, #4*4
                
                
                sub    r0,   xr_ptr,  sfboundary_ptr
                mov    r0,   r0,  asr #2
                add    r0,   r0,  #576               
                
                
                cmp    xr_ptr, sfboundary_ptr
                bge    MP3_DEC_HuffmanParsingZEROAsm_EXIT                
                
                mov    r7, #0
   
                
                
                
MP3_DEC_HuffmanParsingZEROAsm_LOOP:
                stmia  xr_ptr!, {r6-r7}
                cmp    xr_ptr, sfboundary_ptr
                blt    MP3_DEC_HuffmanParsingZEROAsm_LOOP                
MP3_DEC_HuffmanParsingZEROAsm_EXIT:  

                ldmfd   sp!, {r4-r12, pc}
                @ENDFUNC               
   
                
                

                
                
                
                
                
                
                @//void MP3_DEC_OverlapZ0Asm(mp3_fixed_t overlap[/*18*/9],
                @//                                mp3_fixed_t sample[18][32], unsigned int sb))@
                @//  r0: overlap
                @//  r1: sample
                @//  r2: sb
overlap               .req      r0
sample_pos_ptr  .req      r1
mp3_isb                      .req      r2
window_l_ptr     .req   r3
win0                   .req   r4
win1                   .req   r5
t0                       .req   r6
t4                       .req   r7
t1                       .req   r8
t2                       .req   r9
t3                       .req   r10
z                        .req   r12
sample_neg_ptr  .req   r11
              .extern   window_l
MP3_DEC_OverlapZ0Asm:@   FUNCTION
                @ // save the value for the following calculation
                .global  MP3_DEC_OverlapZ0Asm
                stmfd   sp!, {r4-r12, r14}
                mov     r14,  #8
                ldr       window_l_ptr,  =window_l
                mov    z,  #0
                mov    t3,    mp3_isb
                add    sample_pos_ptr,   sample_pos_ptr,    mp3_isb, lsl   #2                
                add     sample_neg_ptr,   sample_pos_ptr,   #(16*32*4)              
MP3_DEC_OverlapZ0Asm_LOOP:
      ldr     win0, [window_l_ptr], #4
			ldr     win1, [window_l_ptr], #4
			ldr     t0, [overlap]										
			ldr     t4, [overlap, #4]
			str     z, [overlap], #4
			str     z,  [overlap], #4
			smulwb  t1, t0, win0		
			mov     t1, t1, lsl#1
			smulwt  t2, t0, win0
			mov     t2, t2, lsl#1		
			str     t1, [sample_pos_ptr],  #(32*4)			
			tst		t3, #1
			rsbne	t2, t2,#0					
			str     t2, [sample_neg_ptr,  #(32*4)] 
			smulwb  t1, t4, win1		
	   	mov     t1, t1, lsl#1
			smulwt  t2, t4, win1
			mov     t2, t2, lsl#1					
			rsbne	t1, t1,#0x0					
			str     t1, [sample_pos_ptr],  #(32*4)	
			str     t2, [sample_neg_ptr] ,  #(-32*4*2)
      subs        r14,  r14,  #2
      BGT       MP3_DEC_OverlapZ0Asm_LOOP
      ldr         win0, [window_l_ptr]					
		  ldr         t0, [overlap]
		  str         z,  [overlap]
		  smulwb  t1, t0, win0
		  mov       t1, t1, lsl#1
		  smulwt   t2, t0, win0
		  mov       t2, t2, lsl#1					
		  str          t1,  [sample_pos_ptr]				
		  tst	      t3, #1
		  rsbne     t2, t2,#0					
		  str          t2, [sample_neg_ptr,  #(32*4)] 
      ldmfd   sp!, {r4-r12, pc}
      @ENDFUNC 


                @//mp3_fixed_t MP3_DecMulLongAsm(mp3_fixed_t x0, mp3_fixed_t y0)                
                @//  r0: x
                @//  r1: y
x0               .req      r0
y0               .req      r1
lo                .req      r14
hi                .req      r12
result          .req      r0
MP3_DecMulLongAsm:@   FUNCTION
                @ // save the value for the following calculation
       .global  MP3_DecMulLongAsm
       stmfd   sp!, {r14}
       smull	    lo,hi,x0,y0
       movs    lo,lo,lsr#28
       adc      result, lo, hi, lsl#4
       ldmfd   sp!, {pc}
       @ENDFUNC 
                
                
                .end@END
