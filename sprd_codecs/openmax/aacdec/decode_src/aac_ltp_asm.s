@/*************************************************************************
@** File Name:      aac_ltp.c                                             *
@** Author:         Reed zhang                                            *
@** Date:           09/11/2010                                            *
@** Copyright:      2001 Spreatrum, Incoporated. All Rights Reserved.     *
@** Description:                                                          *
@**      This file is used to do LTP mode.                                *
@**                        Edit History                                   *
@** ----------------------------------------------------------------------*
@** DATE           NAME             DESCRIPTION                           *
@** 09/11/2010     Reed zhang       Create.                               *
@**************************************************************************/
	@INCLUDE aac_asm_config.s
                @ ***** .end LICENSE BLOCK *****  

	.text  @AREA	|.text|, CODE, READONLY
	.arm
	 
                               
                @//int32_t AAC_LTP_ARMFilterBankLongAsm(
                @//                              int16   *in_rec_ptr,
                @//                              int32   *out_est_ptr,
                @//                              int16   *win_pre_ptr,
                @//                              int16   *win_cur_ptr)@
                

                @@ // r0, input  data
                @@ // r1, output data
                @@ // r2, pre win
                @@ // r3, cur win
in_rec_ptr       .req  r0                
out_est_ptr      .req  r1                
win_pre_ptr      .req  r2                
win_cur_ptr      .req  r3  
ltp_coef         .req  r12  
in_rec_ptr1024   .req  r4
out_est_ptr1024  .req  r5
AAC_LTP_ARMFilterBankLongAsm:@   FUNCTION
                @ // save the value for the following calculation
                .global  AAC_LTP_ARMFilterBankLongAsm
                stmfd   sp!, {r4-r12, r14}
                ldr     ltp_coef, [r1]                
                
                mov     r14, #1024
                add     win_cur_ptr, win_cur_ptr, #1024*2
                sub     win_cur_ptr, win_cur_ptr, #4
                add     in_rec_ptr1024, in_rec_ptr, #1024*2
                add     out_est_ptr1024, out_est_ptr, #1024*4
                
AAC_LTP_ARMFilterBankLongAsm_LOOP00:
                @@//////////////
                ldmia   in_rec_ptr!,     {r8, r9}
                ldmia   win_pre_ptr!,    {r10, r11}
    @IF AAC_DEC_ASM_LITTLE_ENDIAN = 1      
                @@/* little endian */
                SMULBB  r6,  r8, ltp_coef
                SMULTB  r7,  r8, ltp_coef       
    @ELSE
    @            SMULTB  r6,  r8, ltp_coef
    @            SMULBB  r7,  r8, ltp_coef        
    @ENDIF
                        
                SMULWT  r6,   r6,  r10
                SMULWB  r8,   r7,  r10    @@// r6, r8
                
    @IF AAC_DEC_ASM_LITTLE_ENDIAN = 1      
                @@/* little endian */
                SMULBB  r7,   r9, ltp_coef
                SMULTB  r10,  r9, ltp_coef
     @ELSE
     @           SMULTB  r7,   r9, ltp_coef
     @           SMULBB  r10,  r9, ltp_coef
     @ENDIF
                SMULWT  r9,  r7,  r11                
                SMULWB  r10, r10,   r11                
                stmia   out_est_ptr!, {r6, r8, r9, r10}                                          

                subs    r14, r14, #4
                BGT     AAC_LTP_ARMFilterBankLongAsm_LOOP00
                
                @@///////////////////////////////////////
                movs     r14,   ltp_coef, lsr #16
                moveq   r14,   #1024                    
                beq      AAC_LTP_ARMFilterBankLongAsm_CLEAR
                beq       AAC_LTP_ARMFilterBankLongAsm_BRANCH01
AAC_LTP_ARMFilterBankLongAsm_LOOP01:
                @@//////////////////////////////////
                ldr   r8, [in_rec_ptr1024], #4
                ldr   r10,[win_cur_ptr], #-4
    @IF AAC_DEC_ASM_LITTLE_ENDIAN = 1      
                @@/* little endian */
                SMULBB  r6,  r8, ltp_coef
                SMULTB  r7,  r8, ltp_coef       
                SMULWB  r6,   r6,  r10
                SMULWT  r8,   r7,  r10    @@// r6, r8
    @ELSE
    @            SMULTB  r6,  r8, ltp_coef
    @            SMULBB  r7,  r8, ltp_coef        
    @            SMULWB  r6,   r6,  r10
    @            SMULWT  r8,   r7,  r10    @@// r6, r8
    @ENDIF     
                stmia   out_est_ptr!, {r6, r8}          
                subs    r14, r14, #2
                BGT     AAC_LTP_ARMFilterBankLongAsm_LOOP01             
AAC_LTP_ARMFilterBankLongAsm_BRANCH01:
                mov       r14,   ltp_coef, lsr #16                
                ands      r8,     r14,  #0x1
                subne    out_est_ptr,  out_est_ptr,  #4
                
                

AAC_LTP_ARMFilterBankLongAsm_BRANCH00:
                @@/////////////////////////////////////////////
                mov       r14,   ltp_coef, lsr #16
                rsbs       r14,   r14, #1024
                beq       AAC_LTP_ARMFilterBankLongAsm_BRANCH0


AAC_LTP_ARMFilterBankLongAsm_CLEAR:
                mov     r8,    #0
AAC_LTP_ARMFilterBankLongAsm_CLEAR_LOOP:
                str   r8, [out_est_ptr], #4
                subs    r14, r14, #1
                BGT     AAC_LTP_ARMFilterBankLongAsm_CLEAR_LOOP


AAC_LTP_ARMFilterBankLongAsm_BRANCH0:
                @@//////////////                
                ldmfd   sp!, {r4-r12, pc}
                @ENDFUNC
                   	
    	        @@/* long start block window processing */
                @//int32 AAC_LTP_ARMFilterBankLongStartAsm(
                @//                              int16   *in_rec_ptr,
                @//                              int32   *out_est_ptr,
                @//                              int16   *win_pre_ptr,
                @//                              int16   *win_cur_ptr)@
                

                @@ // r0, input  data
                @@ // r1, output data
                @@ // r2, pre win
                @@ // r3, cur win
longstar_in_rec_ptr       .req  r0                
longstar_out_est_ptr      .req  r1                
longstar_win_pre_ptr      .req  r2                
longstar_win_cur_ptr      .req  r3  
longstar_ltp_coef         .req  r12  
longstar_out_est_ptr1024  .req  r4
AAC_LTP_ARMFilterBankLongStartAsm:@   FUNCTION
                @ // save the value for the following calculation
                .global  AAC_LTP_ARMFilterBankLongStartAsm
                stmfd   sp!, {r4-r12, r14}                                
                ldr     longstar_ltp_coef, [r1]
                                
                add     win_cur_ptr, win_cur_ptr, #128*2
                sub     win_cur_ptr, win_cur_ptr, #4
                
                mov     r14,  #1024
AAC_LTP_ARMFilterBankLongStartAsm_LOOP00:
                @@//////////////////
                ldmia   longstar_in_rec_ptr!,     {r8, r9}
                ldmia   longstar_win_pre_ptr!,    {r10, r11}
                
    @IF AAC_DEC_ASM_LITTLE_ENDIAN = 1      
                @@/* little endian */
                SMULBB  r6,  r8, ltp_coef
                SMULTB  r7,  r8, ltp_coef       
    @ELSE
    @            SMULTB  r6,  r8, ltp_coef
    @            SMULBB  r7,  r8, ltp_coef        
    @ENDIF
                        
                SMULWT  r6,   r6,  r10
                SMULWB  r8,   r7,  r10    @@// r6, r8
                
    @IF AAC_DEC_ASM_LITTLE_ENDIAN = 1      
                @@/* little endian */
                SMULBB  r7,   r9, ltp_coef
                SMULTB  r10,  r9, ltp_coef
    @ ELSE
    @            SMULTB  r7,   r9, ltp_coef
    @            SMULBB  r10,  r9, ltp_coef
    @ ENDIF
                SMULWT  r9,  r7,  r11                
                SMULWB  r10, r10,   r11     

                stmia   longstar_out_est_ptr!, {r6, r8, r9, r10}
                subs    r14, r14, #4
                BGT     AAC_LTP_ARMFilterBankLongStartAsm_LOOP00    
                
                mov     r14,   longstar_ltp_coef, lsr  #16
                cmp     r14,   #448
                bge     AAC_LTP_ARMFilterBankLongStartAsm_BRANCHGE448
                @@@///////////////////////////////////////////
                @@/* < 448*/
AAC_LTP_ARMFilterBankLongStartAsm_BRANCHGE448_S1LOOP:
               ldr    r8,  [longstar_in_rec_ptr],  #4
               subs     r14,  r14,   #2
    @IF AAC_DEC_ASM_LITTLE_ENDIAN = 1      
                @@/* little endian */
                SMULBB  r6,  r8, ltp_coef
                SMULTB  r7,  r8, ltp_coef       
    @ELSE
    @            SMULTB  r6,  r8, ltp_coef
    @            SMULBB  r7,  r8, ltp_coef        
    @ENDIF
                 mov      r6,  r6,  asr  #1
                 mov      r7,  r7,  asr  #1
                 stmia    longstar_out_est_ptr!,  {r6, r7}
                 BGT    AAC_LTP_ARMFilterBankLongStartAsm_BRANCHGE448_S1LOOP

                 mov     r14,   longstar_ltp_coef, lsr  #16
                 ands    r8,    r14,    #0x1
                 subne  longstar_out_est_ptr,  longstar_out_est_ptr,  #4
                 mov    r8,     #0
                 rsb      r14,  r14,     #1024
AAC_LTP_ARMFilterBankLongStartAsm_BRANCHGE448_S2LOOP:
                 str       r8,   [longstar_out_est_ptr],  #4
                 subs     r14,  r14,   #1
                 BGT    AAC_LTP_ARMFilterBankLongStartAsm_BRANCHGE448_S2LOOP
                @@@///////////////////////////////////////////
                b      AAC_LTP_ARMFilterBankLongStartAsm_BRANCHEXIT
AAC_LTP_ARMFilterBankLongStartAsm_BRANCHGE448:
                cmp    r14,   #576
                bge     AAC_LTP_ARMFilterBankLongStartAsm_BRANCHGE576
                @@@/////////////////////////////////////////
                @@/* 448 =< r14 < 576*/
               mov     r9,  #448
AAC_LTP_ARMFilterBankLongStartAsm_BRANCHGE576_S1LOOP:
               ldr    r8,  [longstar_in_rec_ptr],  #4
               subs     r9,  r9,   #2
    @IF AAC_DEC_ASM_LITTLE_ENDIAN = 1      
                @@/* little endian */
                SMULBB  r6,  r8, ltp_coef
                SMULTB  r7,  r8, ltp_coef       
    @ELSE
    @            SMULTB  r6,  r8, ltp_coef
    @            SMULBB  r7,  r8, ltp_coef        
    @ENDIF
                 mov      r6,  r6,  asr  #1
                 mov      r7,  r7,  asr  #1
                 stmia    longstar_out_est_ptr!,  {r6, r7}
                 BGT    AAC_LTP_ARMFilterBankLongStartAsm_BRANCHGE576_S1LOOP
                 
                 sub     r9,  r14,   #448
AAC_LTP_ARMFilterBankLongStartAsm_BRANCHGE576_S2LOOP:
                 ldr    r8,  [longstar_in_rec_ptr],  #4
                 ldr    r10,  [win_cur_ptr],  #-4                 
    @IF AAC_DEC_ASM_LITTLE_ENDIAN = 1      
                @@/* little endian */
                SMULBB  r6,  r8, ltp_coef
                SMULTB  r7,  r8, ltp_coef       
                SMULWB  r6,   r6,  r10
                SMULWT  r7,   r7,  r10    @@// r6, r7
    @ELSE
    @            SMULTB  r6,  r8, ltp_coef
    @            SMULBB  r7,  r8, ltp_coef    
    @            SMULWB  r6,   r6,  r10
    @            SMULWT  r7,   r7,  r10    @@// r6, r7    
    @ENDIF
                 stmia    longstar_out_est_ptr!,  {r6, r7}
                 subs      r9,  r9,  #2 
                 BGT    AAC_LTP_ARMFilterBankLongStartAsm_BRANCHGE576_S2LOOP
                 ands     r12,  r14,   #1
                 subne     longstar_out_est_ptr, longstar_out_est_ptr,  #4
                 mov     r8,  #0
                 rsb      r14,  r14,    #1024
AAC_LTP_ARMFilterBankLongStartAsm_BRANCHGE576_S3LOOP: 
                 str       r8,   [longstar_out_est_ptr],  #4
                 subs     r14,  r14,   #1
                 BGT    AAC_LTP_ARMFilterBankLongStartAsm_BRANCHGE576_S3LOOP

                @@@//////////////////////////////////////////
                b      AAC_LTP_ARMFilterBankLongStartAsm_BRANCHEXIT
AAC_LTP_ARMFilterBankLongStartAsm_BRANCHGE576:
                @@/* r14 >= 576*/
               mov     r9,  #448
AAC_LTP_ARMFilterBankLongStartAsm_BRANCHGE1024_S1LOOP:
               ldr    r8,  [longstar_in_rec_ptr],  #4
               subs     r9,  r9,   #2
    @IF AAC_DEC_ASM_LITTLE_ENDIAN = 1      
                @@/* little endian */
                SMULBB  r6,  r8, ltp_coef
                SMULTB  r7,  r8, ltp_coef       
    @ELSE
    @            SMULTB  r6,  r8, ltp_coef
    @            SMULBB  r7,  r8, ltp_coef        
    @ENDIF
                 mov      r6,  r6,  asr  #1
                 mov      r7,  r7,  asr  #1
                 stmia    longstar_out_est_ptr!,  {r6, r7}
                 BGT    AAC_LTP_ARMFilterBankLongStartAsm_BRANCHGE1024_S1LOOP
                 
                 mov     r9,   #128
AAC_LTP_ARMFilterBankLongStartAsm_BRANCHGE1024_S2LOOP:
                 ldr    r8,  [longstar_in_rec_ptr],  #4
                 ldr    r10,  [win_cur_ptr],  #-4                 
    @IF AAC_DEC_ASM_LITTLE_ENDIAN = 1      
                @@/* little endian */
                SMULBB  r6,  r8, ltp_coef
                SMULTB  r7,  r8, ltp_coef       
                SMULWB  r6,   r6,  r10
                SMULWT  r7,   r7,  r10    @@// r6, r7
    @ELSE
    @            SMULTB  r6,  r8, ltp_coef
    @            SMULBB  r7,  r8, ltp_coef    
    @            SMULWB  r6,   r6,  r10
    @            SMULWT  r7,   r7,  r10    @@// r6, r7    
    @ENDIF
                
                 stmia    longstar_out_est_ptr!,  {r6, r7}
                 subs      r9,  r9,  #2 
                 BGT    AAC_LTP_ARMFilterBankLongStartAsm_BRANCHGE1024_S2LOOP
                 mov      r14,  #448
                 mov      r8,  #0
AAC_LTP_ARMFilterBankLongStartAsm_BRANCHGE1024_S3LOOP: 
                 str       r8,   [longstar_out_est_ptr],  #4
                 subs     r14,  r14,   #1
                 BGT    AAC_LTP_ARMFilterBankLongStartAsm_BRANCHGE1024_S3LOOP                
                @@@//////////////////////////////////////////
AAC_LTP_ARMFilterBankLongStartAsm_BRANCHEXIT:
                    	        
    	        ldmfd   sp!, {r4-r12, pc}
                @ENDFUNC
                
                
                
                @@/* long stop block window processing */
                @//int32 AAC_LTP_ARMFilterBankLongStopAsm(
                @//                              int16   *in_rec_ptr,
                @//                              int32   *out_est_ptr,
                @//                              int16   *win_pre_ptr,
                @//                              int16   *win_cur_ptr)@
                

                @@ // r0, input  data
                @@ // r1, output data
                @@ // r2, pre win
                @@ // r3, cur win
longstop_in_rec_ptr       .req  r0                
longstop_out_est_ptr      .req  r1                
longstop_win_pre_ptr      .req  r2                
longstop_win_cur_ptr      .req  r3  
longstop_ltp_coef         .req  r12  
longstop_out_est_ptr576   .req  r4
AAC_LTP_ARMFilterBankLongStopAsm:   @FUNCTION
                @ // save the value for the following calculation
                .global  AAC_LTP_ARMFilterBankLongStopAsm
                stmfd   sp!, {r4-r12, r14}                                
                ldr     longstop_ltp_coef, [r1]                
                add     longstop_win_cur_ptr, longstop_win_cur_ptr, #1024*2
                sub     longstop_win_cur_ptr, longstop_win_cur_ptr, #4
    	        mov     r14,  #448
    	        add     longstop_out_est_ptr576,  longstop_out_est_ptr,  #576*4
    	        add     longstop_in_rec_ptr,      longstop_in_rec_ptr,   #576*2
    	        
                @//////////////////////////////////////////////////
                mov     r10,  #0
                mov     r11,  #0
AAC_LTP_ARMFilterBankLongStopAsmLOOP00:  	        
    	        ldmia   longstop_in_rec_ptr!,     {r8, r9}
    @IF AAC_DEC_ASM_LITTLE_ENDIAN = 1      
                @@/* little endian */
                SMULBB  r6,  r8, longstop_ltp_coef
                SMULTB  r7,  r8, longstop_ltp_coef                      
                SMULBB  r8,  r9, longstop_ltp_coef
                SMULTB  r9,  r9, longstop_ltp_coef     
    @ELSE
    @            SMULTB  r6,  r8, longstop_ltp_coef
    @            SMULBB  r7,  r8, longstop_ltp_coef                      
    @            SMULTB  r8,  r9, longstop_ltp_coef
    @            SMULBB  r9,  r9, longstop_ltp_coef
    @ENDIF

                
                mov     r6,  r6,  asr #1
                mov     r7,  r7,  asr #1
                mov     r8,  r8,  asr #1
                mov     r9,  r9,  asr #1                
                stmia   longstop_out_est_ptr576!, {r6-r9}
                stmia   longstop_out_est_ptr!, {r10-r11}
                stmia   longstop_out_est_ptr!, {r10-r11}
                    	        
    	        subs   r14,  r14,  #4
    	        BGT    AAC_LTP_ARMFilterBankLongStopAsmLOOP00
    	        
    	        
    	        mov     r14,  #128
    	        sub     longstop_in_rec_ptr, longstop_in_rec_ptr,  #576*2
AAC_LTP_ARMFilterBankLongStopAsmLOOP01:   	        
    	        ldmia   longstar_in_rec_ptr!,     {r8, r9}
    @IF AAC_DEC_ASM_LITTLE_ENDIAN = 1      
                @@/* little endian */
                SMULBB  r6,  r8, longstop_ltp_coef
                SMULTB  r7,  r8, longstop_ltp_coef                      
                SMULBB  r8,  r9, longstop_ltp_coef
                SMULTB  r9,  r9, longstop_ltp_coef     
    @ELSE
    @            SMULTB  r6,  r8, longstop_ltp_coef
    @            SMULBB  r7,  r8, longstop_ltp_coef                 
    @            SMULTB  r8,  r9, longstop_ltp_coef
    @            SMULBB  r9,  r9, longstop_ltp_coef
    @ ENDIF
                ldmia   longstop_win_pre_ptr!,    {r10, r11}                
                SMULWT  r6,  r6,  r10
                SMULWB  r7,  r7,  r10                 
                SMULWT  r8, r8,  r11                
                SMULWB  r9, r9,  r11                 
                stmia   longstop_out_est_ptr!, {r6-r9}    	        
    	        subs   r14,  r14,  #4
    	        BGT    AAC_LTP_ARMFilterBankLongStopAsmLOOP01    	        
    	        
    	        
    	        movs     r14,  longstop_ltp_coef,  asr #16
    	        moveq    r14,   #1024
    	        beq      AAC_LTP_ARMFilterBankLongStopAsmLOOP02_BRANCH
    	        
    	        add     longstop_in_rec_ptr, longstop_in_rec_ptr,  #448*2
AAC_LTP_ARMFilterBankLongStopAsmLOOP02:
    	        
    	        ldr   r8, [longstop_in_rec_ptr], #4
                ldr   r10,[longstop_win_cur_ptr], #-4              

    @IF AAC_DEC_ASM_LITTLE_ENDIAN = 1      
                @@/* little endian */
                SMULBB  r6,  r8, ltp_coef
                SMULTB  r7,  r8, ltp_coef       
                SMULWB  r6,   r6,  r10
                SMULWT  r7,   r7,  r10    @@// r6, r7
    @ELSE
    @            SMULTB  r6,  r8, ltp_coef
    @            SMULBB  r7,  r8, ltp_coef    
    @            SMULWB  r6,   r6,  r10
    @            SMULWT  r7,   r7,  r10    @@// r6, r7    
    @ENDIF
    	        stmia    longstop_out_est_ptr576!,   {r6, r7}
    	        subs   r14,  r14,  #2
    	        BGT    AAC_LTP_ARMFilterBankLongStopAsmLOOP02
    	        
    	        mov     r14,  longstop_ltp_coef,  asr #16
                ands     r12,     r14,  #1
                subne   longstop_out_est_ptr576,  longstop_out_est_ptr576,  #4
                rsb       r14,  r14,   #1024
AAC_LTP_ARMFilterBankLongStopAsmLOOP02_BRANCH:
    	        @@/* clear */    	        
    	        mov     r10,  #0
AAC_LTP_ARMFilterBankLongStopAsmLOOP03:
    	        str     r10,  [longstop_out_est_ptr576], #4    	        
    	        subs   r14,  r14,  #1
    	        BGT    AAC_LTP_ARMFilterBankLongStopAsmLOOP03    	       
    	        
    	        @//////////////////////////////////////////////////
AAC_LTP_ARMFilterBankLongStopAsm_BRANCH0EXIT:
    	
    	        ldmfd   sp!, {r4-r12, pc}
                @ENDFUNC
    	
    	
    	
    	
    	
    	        @//void AAC_LTP_DEC_FFTPreProcessAsm(int32 *in_data_ptr,
                @//           int32 *out_data_ptr,
                @//           int16 *table_ptr)@
FFT_pre_ptr0        .req  r0
FFT_pre_ptr1        .req  r3
FFT_pre_ptr2        .req  r4
FFT_pre_ptr3        .req  r5
FFT_pre_out_ptr     .req  r1
FFT_pre_out_ptr512  .req  r6
FFT_pre_tab_ptr     .req  r2


AAC_LTP_DEC_FFTPreProcessAsm:@   FUNCTION
                .global  AAC_LTP_DEC_FFTPreProcessAsm
                stmfd   sp!, {r4-r12, r14}
                
                mov     r14,  #256
                add     FFT_pre_ptr2, FFT_pre_ptr0, #512*4 @@// 512
                sub     FFT_pre_ptr3, FFT_pre_ptr2, #1*4   @@// 510
                
                add     FFT_pre_ptr0, FFT_pre_ptr2, #1024*4@@// 1536
                sub     FFT_pre_ptr1, FFT_pre_ptr0, #1*4   @@// 1534
                
                add     FFT_pre_out_ptr512, FFT_pre_out_ptr, #1024*4
                sub     FFT_pre_out_ptr512,  FFT_pre_out_ptr512,  #1*4
AAC_LTP_DEC_FFTPreProcessAsm_LOOP:                
                @@//////////////
                ldmda   FFT_pre_ptr1!, {r7, r8}
                ldmda   FFT_pre_ptr3!, {r9, r10}                
                ldr     r11, [FFT_pre_ptr0], #4
                ldr     r12, [FFT_pre_ptr2], #4
                
                add     r11,  r11, r8   @// re -> r11
                ldr     r8,   [FFT_pre_tab_ptr], #4
                sub     r12,  r12, r10  @// im -> r12
                
                SMULWT  r10,  r11,  r8
                SMLAWB  r10,  r12,  r8, r10
                
                rsb     r11,  r11,  #0
                SMULWB  r11,  r11,  r8
                
                SMLAWT  r11,  r12,  r8, r11
                
                
                stmia   FFT_pre_out_ptr!, {r10, r11}
                
                @@/////////////////////////////
                ldr     r11, [FFT_pre_ptr0], #4
                ldr     r12, [FFT_pre_ptr2], #4
                
                add     r11,  r11, r7  @// im -> r11
                ldr     r8,   [FFT_pre_tab_ptr], #4
                sub     r12,  r12, r9  @// re -> r12
                
                SMULWT  r9,  r12,  r8
                SMLAWB  r9,  r11,  r8, r9
                
                
                SMULWT  r10,  r11,  r8
                rsb     r12,  r12,  #0
                SMLAWB  r10,  r12,  r8, r10
                                
                stmda   FFT_pre_out_ptr512!, {r9, r10}
                
                
                @@//////////////                
                subs    r14, r14, #1
                BGT     AAC_LTP_DEC_FFTPreProcessAsm_LOOP
                
                ldmfd   sp!, {r4-r12, pc}
                @ENDFUNC    	
    	
    	
    	        @//void AAC_LTP_DEC_FFTPostProcessAsm(int32 *in_data_ptr,
                @//                                   int16 *table_ptr)@
FFT_post_ptr0         .req  r0
FFT_post_ptr1022      .req  r2
FFT_post_tab_ptr      .req  r1


AAC_LTP_DEC_FFTPostProcessAsm:@   FUNCTION
                .global  AAC_LTP_DEC_FFTPostProcessAsm
                stmfd   sp!, {r4-r12, r14}
                
                mov     r14,  #256
                add     FFT_post_ptr1022, FFT_post_ptr0,    #1024*4
                sub     FFT_post_ptr1022, FFT_post_ptr1022, #1*4
AAC_LTP_DEC_FFTPostProcessAsm_LOOP:                
                @@//////////////
                ldmia   FFT_post_ptr0,    {r8-r9}
                ldmda   FFT_post_ptr1022, {r10-r11}                
                ldmia   FFT_post_tab_ptr!,{r4-r5}
                
                SMULWT  r3, r8,  r4
                rsb     r8, r8,  #0
                SMULWB  r8, r8,  r4
                
                SMLAWB  r3, r9,  r4, r3  @@// r3: 0                              
                SMLAWT  r8, r9,  r4, r8  @@// r8: 1023
                
                rsb     r3,  r3,  #0
                @//////////////////
                SMULWT  r4, r10,  r5
                rsb     r10,  r10,  #0
                SMULWB  r7, r10,  r5
                
                SMLAWB  r4,  r11,  r5, r4  @@// r4: 1                
                SMLAWT  r7,  r11,  r5, r7  @@// r7: 1022
                rsb     r4,  r4, #0
                
                stmia   FFT_post_ptr0!,    {r3, r7}
                stmda   FFT_post_ptr1022!, {r4, r8}
                @@//////////////                
                subs    r14, r14, #1
                BGT     AAC_LTP_DEC_FFTPostProcessAsm_LOOP
                
                ldmfd   sp!, {r4-r12, pc}
                @ENDFUNC
                
                
                @//void AAC_LTP_DEC_FFTStep1Asm(int32 *in_data_ptr,
                @//                                   int16 *table_ptr)@
FFT_step1_ptr0         .req  r0
FFT_step1_tab_ptr      .req  r1
FFT_step1_ptr1         .req  r2
FFT_step1_ptr128_0     .req  r3
FFT_step1_ptr128_1     .req  r4


AAC_LTP_DEC_FFTStep1Asm:@   FUNCTION
                .global  AAC_LTP_DEC_FFTStep1Asm
                stmfd   sp!, {r4-r12, r14}
                
                
                add     FFT_step1_ptr1, FFT_step1_ptr0, #512*4
                add     FFT_step1_ptr128_0, FFT_step1_ptr0, #256*4
                add     FFT_step1_ptr128_1, FFT_step1_ptr1, #256*4
                       
                @@//////////////////////////////
                ldmia   FFT_step1_ptr0,   {r5,r6}
                ldmia   FFT_step1_ptr1,   {r7,r8}
                
                add     r9,   r5, r7   @@// re1
                sub     r11,  r5, r7   @@// re2
                
                add     r10,  r6, r8   @@// im1
                sub     r12,  r6, r8   @@// im2
                
                mov     r9,    r9,   asr   #1
                mov     r10,   r10,  asr   #1
                mov     r11,   r11,  asr   #1
                mov     r12,   r12,  asr   #1
                
                stmia   FFT_step1_ptr0!, {r9, r10}
                stmia   FFT_step1_ptr1!, {r11, r12}
                
                @@//////////////////////////////
                ldmia   FFT_step1_ptr128_0,   {r5,r6}
                ldmia   FFT_step1_ptr128_1,   {r7,r8}
                
                add     r9,   r5, r7   @@// re1
                sub     r12,  r5, r7   @@// re2
                
                add     r10,  r6, r8   @@// im1
                sub     r11,  r6, r8   @@// im2
                
                rsb     r12,   r12,  #0
                mov     r9,    r9,   asr   #1
                mov     r10,   r10,  asr   #1
                mov     r11,   r11,  asr   #1
                mov     r12,   r12,  asr   #1
                
                stmia   FFT_step1_ptr128_0!, {r9, r10}
                stmia   FFT_step1_ptr128_1!, {r11, r12}                 
                
                mov     r14,  #64         
AAC_LTP_DEC_FFTStep1Asm_LOOP1:
                @@/////////////////
                @@//////////////////////////////
                ldmia   FFT_step1_ptr0,   {r5,r6}
                ldmia   FFT_step1_ptr1,   {r7,r8}
                
                ldr     r12,   [FFT_step1_tab_ptr], #4
                
                add     r9,   r5, r7   @@// re1
                sub     r11,  r5, r7   @@// re2
                
                add     r10,  r6, r8   @@// im1
                sub     r8,   r6, r8   @@// im2
                
                mov     r9,    r9,   asr   #1
                mov     r10,   r10,  asr   #1
                stmia   FFT_step1_ptr0!, {r9, r10}
                
                SMULWT  r5,    r11, r12
                SMULWT  r6,    r8,  r12
                SMLAWB  r5,    r8,  r12,  r5
                rsb     r11,   r11, #0
                SMLAWB  r6,    r11,  r12,  r6              
                stmia   FFT_step1_ptr1!, {r5, r6}
                
                @@//////////////////////////////
                ldmia   FFT_step1_ptr128_0,   {r5,r6}
                ldmia   FFT_step1_ptr128_1,   {r7,r8}
                
                add     r9,  r5, r7   @@// re1
                sub     r7,  r5, r7   @@// re2     @// x1
                
                add     r10,  r6, r8   @@// im1
                sub     r11,  r6, r8   @@// im2    @// x2
                
                mov     r9,    r9,   asr   #1
                mov     r10,   r10,  asr   #1                
                stmia   FFT_step1_ptr128_0!, {r9, r10}
                
                SMULWT  r6,    r7,     r12
                SMULWT  r5,    r11,    r12
                rsb     r7,    r7,     #0
                SMLAWB  r5,    r7,     r12,  r5
                SMLAWB  r6,    r11,    r12,  r6
                rsb     r6,    r6,  #0                
                stmia   FFT_step1_ptr128_1!, {r5, r6}  
                
                @@/////////////////
                subs   r14, r14, #1
                BGT    AAC_LTP_DEC_FFTStep1Asm_LOOP1                   
                
                @@//////////////////////////////////
                mov     r14,  #63   
                sub     FFT_step1_tab_ptr,   FFT_step1_tab_ptr,  #8      
AAC_LTP_DEC_FFTStep1Asm_LOOP2:
                @@/////////////////
                @@//////////////////////////////
                ldmia   FFT_step1_ptr0,   {r5,r6}
                ldmia   FFT_step1_ptr1,   {r7,r8}
                
                ldr     r12,   [FFT_step1_tab_ptr], #-4
                
                add     r9,   r5, r7   @@// re1
                sub     r11,  r5, r7   @@// re2
                
                add     r10,  r6, r8   @@// im1
                sub     r8,   r6, r8   @@// im2
                
                mov     r9,    r9,   asr   #1
                mov     r10,   r10,  asr   #1
                stmia   FFT_step1_ptr0!, {r9, r10}
                
                SMULWB  r5,    r11, r12
                SMULWB  r6,    r8,  r12
                SMLAWT  r5,    r8,  r12,  r5
                rsb     r11,   r11, #0
                SMLAWT  r6,    r11,  r12,  r6              
                stmia   FFT_step1_ptr1!, {r5, r6}
                
                @@//////////////////////////////
                ldmia   FFT_step1_ptr128_0,   {r5,r6}
                ldmia   FFT_step1_ptr128_1,   {r7,r8}
                
                add     r9,  r5, r7   @@// re1
                sub     r7,  r5, r7   @@// re2     @// x1
                
                add     r10,  r6, r8   @@// im1
                sub     r11,  r6, r8   @@// im2    @// x2
                
                mov     r9,    r9,   asr   #1
                mov     r10,   r10,  asr   #1                
                stmia   FFT_step1_ptr128_0!, {r9, r10}
                
                SMULWB  r6,    r7,     r12
                SMULWB  r5,    r11,    r12
                rsb     r7,    r7,     #0
                SMLAWT  r5,    r7,     r12,  r5
                SMLAWT  r6,    r11,    r12,  r6
                rsb     r6,    r6,  #0                
                stmia   FFT_step1_ptr128_1!, {r5, r6}  
                
                @@/////////////////
                subs   r14, r14, #1
                BGT    AAC_LTP_DEC_FFTStep1Asm_LOOP2                  
                
                
                ldmfd   sp!, {r4-r12, pc}
                @ENDFUNC
                
                
                
                @//void AAC_LTP_DEC_FFTStep2Asm(int32 *in_data_ptr,
                @//                             int16 *table_ptr)@
FFT_step2_in_ptr0         .req  r0
FFT_step2_in_ptr1         .req  r2
FFT_step2_in_ptr2         .req  r3
FFT_step2_in_ptr3         .req  r4
FFT_step2_tab_ptr         .req  r1




AAC_LTP_DEC_FFTStep2Asm:@   FUNCTION
                .global  AAC_LTP_DEC_FFTStep2Asm
                stmfd   sp!, {r4-r12, r14}
                
                add FFT_step2_in_ptr1, FFT_step2_in_ptr0, #128*4
                add FFT_step2_in_ptr2, FFT_step2_in_ptr1, #128*4                               
                add FFT_step2_in_ptr3, FFT_step2_in_ptr2, #128*4                
          
                
                ldmia  FFT_step2_in_ptr0, {r5, r6}
                ldmia  FFT_step2_in_ptr2, {r7, r8}
                
                add    r9,   r5,  r7  @@// RE(t2)--r9
                sub    r10,  r5,  r7  @@// RE(t1)--r10
                
                ldr    r5,   [FFT_step2_in_ptr1]
                ldr    r7,   [FFT_step2_in_ptr3]                
                
                add    r11,  r6,  r8  @@// IM(t2)--r11
                sub    r12,  r6,  r8  @@// IM(t1)--r12
                
                add    r6,   r5,  r7  @@// RE(t3)--r6
                sub    r8,   r5,  r7  @@// IM(t4)--r8

                
                add    r5,   r9,  r6  @@// RE(t2) + RE(t3)
                mov    r5,   r5,  asr #1
                str    r5,   [FFT_step2_in_ptr0], #4
                sub    r9,   r9,  r6  @@// RE(c3) =  RE(t2) - RE(t3) -- r9(RE(c3))
                
                ldr    r5,   [FFT_step2_in_ptr1, #4]
                ldr    r7,   [FFT_step2_in_ptr3, #4] 
                
                add    r7,   r5, r7            @@// IM(t3) -- r7
                sub    r5,   r7, r5,  lsl #1   @@// RE(t4) -- r5
                
                
                add    r11,  r11, r7           @@// IM(t2) + IM(t3)
                sub    r7,   r11, r7,  lsl #1  @@// IM(t2) - IM(t3) -- r7(IM(c3))
                mov    r11,  r11,  asr  #1
                str    r11,   [FFT_step2_in_ptr0], #4 
                @@// r6 and r11 are free, and the other reg, t1(r10, r12), t4:(r5, r8), c3:(r9, r7)
                
                add    r10,   r10,  r5
                sub    r5,    r10,   r5, lsl #1    @@// RE(c4): r10,  RE(c2): r5
                
                add    r12,   r12,  r8
                sub    r8,    r12,  r8, lsl #1     @@// IM(c4): r12,  IM(c2): r8
                @@// r6 and r11 are free, and the other reg, c4(r10, r12), c2:(r5, r8), c3:(r9, r7)
                mov   r5,  r5,  asr #1
                mov   r8,  r8,  asr #1                
                mov   r10, r10, asr #1
                mov   r12, r12, asr #1
                stmia  FFT_step2_in_ptr1!, {r5,  r8}
                stmia  FFT_step2_in_ptr3!, {r10, r12}
                
                mov   r9,  r9,  asr #1
                mov   r10, r7,  asr #1             
                
                
                stmia  FFT_step2_in_ptr2!, {r9,  r10}
                
                
                mov    r14,  #32
AAC_LTP_DEC_FFTStep2Asm_LOOP0:                
                @///////////
                ldmia  FFT_step2_in_ptr0, {r5, r6}
                ldmia  FFT_step2_in_ptr2, {r7, r8}
                
                add    r9,   r5,  r7  @@// RE(t2)--r9
                sub    r10,  r5,  r7  @@// RE(t1)--r10
                
                ldr    r5,   [FFT_step2_in_ptr1]
                ldr    r7,   [FFT_step2_in_ptr3]                
                
                add    r11,  r6,  r8  @@// IM(t2)--r11
                sub    r12,  r6,  r8  @@// IM(t1)--r12
                
                add    r6,   r5,  r7  @@// RE(t3)--r6
                sub    r8,   r5,  r7  @@// IM(t4)--r8

                
                add    r5,   r9,  r6  @@// RE(t2) + RE(t3)
                mov    r5,   r5,  asr #1
                str    r5,   [FFT_step2_in_ptr0], #4
                sub    r9,   r9,  r6  @@// RE(c3) =  RE(t2) - RE(t3) -- r9(RE(c3))
                
                ldr    r5,   [FFT_step2_in_ptr1, #4]
                ldr    r7,   [FFT_step2_in_ptr3, #4] 
                
                add    r7,   r5, r7            @@// IM(t3) -- r7
                sub    r5,   r7, r5,  lsl #1   @@// RE(t4) -- r5
                
                
                add    r11,  r11, r7           @@// IM(t2) + IM(t3)
                sub    r7,   r11, r7,  lsl #1  @@// IM(t2) - IM(t3) -- r7(IM(c3))
                mov    r11,  r11,  asr  #1
                str    r11,   [FFT_step2_in_ptr0], #4 
                @@// r6 and r11 are free, and the other reg, t1(r10, r12), t4:(r5, r8), c3:(r9, r7)
                
                add    r10,   r10,  r5
                sub    r6,    r10,   r5, lsl #1    @@// RE(c4): r10,  RE(c2): r6
                ldr    r11,   [FFT_step2_tab_ptr], #4
                add    r12,   r12,  r8
                sub    r8,    r12,  r8, lsl #1     @@// IM(c4): r12,  IM(c2): r8
                @@// r5 and r11 are free, and the other reg, c4(r10, r12), c2:(r6, r8), c3:(r9, r7)
                
                @@// C2
                SMULWT  r5,  r6, r11
                rsb     r6,  r6, #0
                SMULWB  r6,  r6, r11
                
                SMLAWB  r5,  r8, r11, r5
                SMLAWT  r6,  r8, r11, r6
                
                ldr    r11,   [FFT_step2_tab_ptr], #4                         
                stmia  FFT_step2_in_ptr1!, {r5,  r6}
                
                
                @// C3
                SMULWT  r8,  r9, r11
                rsb     r9,  r9, #0
                SMULWB  r9,  r9, r11
                SMLAWB  r8,  r7, r11, r8
                SMLAWT  r9,  r7, r11, r9
                ldr    r11,   [FFT_step2_tab_ptr], #4  
                stmia  FFT_step2_in_ptr2!, {r8,  r9} 
                
                
                @@// C4
                SMULWT  r6,  r10, r11
                rsb     r10, r10, #0
                SMULWB  r7,  r10, r11
                SMLAWB  r6,  r12, r11, r6
                SMLAWT  r7,  r12, r11, r7                        
                stmia  FFT_step2_in_ptr3!, {r6,  r7}                
                @///////////
                subs   r14, r14, #1
                BGT    AAC_LTP_DEC_FFTStep2Asm_LOOP0
                
                
                
                
                sub    FFT_step2_tab_ptr, FFT_step2_tab_ptr, #4*4
                mov    r14,  #31
AAC_LTP_DEC_FFTStep2Asm_LOOP1:                
                @///////////
                ldmia  FFT_step2_in_ptr0, {r5, r6}
                ldmia  FFT_step2_in_ptr2, {r7, r8}
                
                add    r9,   r5,  r7  @@// RE(t2)--r9
                sub    r10,  r5,  r7  @@// RE(t1)--r10
                
                ldr    r5,   [FFT_step2_in_ptr1]
                ldr    r7,   [FFT_step2_in_ptr3]                
                
                add    r11,  r6,  r8  @@// IM(t2)--r11
                sub    r12,  r6,  r8  @@// IM(t1)--r12
                
                add    r6,   r5,  r7  @@// RE(t3)--r6
                sub    r8,   r5,  r7  @@// IM(t4)--r8

                
                add    r5,   r9,  r6  @@// RE(t2) + RE(t3)
                mov    r5,   r5,  asr #1
                str    r5,   [FFT_step2_in_ptr0], #4
                sub    r9,   r9,  r6  @@// RE(c3) =  RE(t2) - RE(t3) -- r9(RE(c3))
                
                ldr    r5,   [FFT_step2_in_ptr1, #4]
                ldr    r7,   [FFT_step2_in_ptr3, #4] 
                
                add    r7,   r5, r7            @@// IM(t3) -- r7
                sub    r5,   r7, r5,  lsl #1   @@// RE(t4) -- r5
                
                
                add    r11,  r11, r7           @@// IM(t2) + IM(t3)
                sub    r7,   r11, r7,  lsl #1  @@// IM(t2) - IM(t3) -- r7(IM(c3))
                mov    r11,  r11,  asr  #1
                str    r11,   [FFT_step2_in_ptr0], #4 
                @@// r6 and r11 are free, and the other reg, t1(r10, r12), t4:(r5, r8), c3:(r9, r7)
                
                add    r10,   r10,  r5
                sub    r5,    r10,   r5, lsl #1    @@// RE(c4): r10,  RE(c2): r5
                ldr    r11,   [FFT_step2_tab_ptr], #-4
                add    r12,   r12,  r8
                sub    r8,    r12,  r8, lsl #1     @@// IM(c4): r12,  IM(c2): r8
                @@// r6 and r11 are free, and the other reg, c4(r10, r12), c2:(r5, r8), c3:(r9, r7)
                
                @////////////////////c4
                SMULWB  r6,  r10, r11
                rsb     r10, r10, #0
                SMULWT  r10,  r10, r11
                SMLAWT  r6,  r12, r11, r6
                SMLAWB  r10,  r12, r11, r10    
                ldr    r11,   [FFT_step2_tab_ptr], #-4                     
                rsb    r6,  r6, #0
                rsb    r10,  r10, #0
                stmia  FFT_step2_in_ptr3!, {r6,  r10}   
                
                @///////////////////c3
                SMULWB  r12,  r9, r11
                rsb     r9,   r9,  #0                
                SMULWT  r9,  r9, r11
                
                SMLAWT  r12,  r7, r11, r12
                SMLAWB  r9,   r7, r11, r9
                rsb     r12,  r12,  #0
                ldr    r11,   [FFT_step2_tab_ptr], #-4  
                stmia  FFT_step2_in_ptr2!, {r9,  r12} 
                
                
                @///////////////////c2
                SMULWB  r6,  r5, r11
                rsb     r5,  r5, #0
                SMULWT  r12,  r5, r11
                SMLAWT  r6,  r8, r11, r6
                SMLAWB  r12,  r8, r11, r12
                                        
                stmia  FFT_step2_in_ptr1!, {r6,  r12} 
                             
                @///////////
                subs   r14, r14, #1
                BGT    AAC_LTP_DEC_FFTStep2Asm_LOOP1                
                
                @//////////////////////////
                add FFT_step2_in_ptr0, FFT_step2_in_ptr0, #384*4
                add FFT_step2_in_ptr1, FFT_step2_in_ptr1, #384*4
                add FFT_step2_in_ptr2, FFT_step2_in_ptr2, #384*4                               
                add FFT_step2_in_ptr3, FFT_step2_in_ptr3, #384*4 
                
                add   FFT_step2_tab_ptr,  FFT_step2_tab_ptr,  #4
                
                
                ldmia  FFT_step2_in_ptr0, {r5, r6}
                ldmia  FFT_step2_in_ptr2, {r7, r8}
                
                add    r9,   r5,  r7  @@// RE(t2)--r9
                sub    r10,  r5,  r7  @@// RE(t1)--r10
                
                ldr    r5,   [FFT_step2_in_ptr1]
                ldr    r7,   [FFT_step2_in_ptr3]                
                
                add    r11,  r6,  r8  @@// IM(t2)--r11
                sub    r12,  r6,  r8  @@// IM(t1)--r12
                
                add    r6,   r5,  r7  @@// RE(t3)--r6
                sub    r8,   r5,  r7  @@// IM(t4)--r8

                
                add    r5,   r9,  r6  @@// RE(t2) + RE(t3)
                mov    r5,   r5,  asr #1
                str    r5,   [FFT_step2_in_ptr0], #4
                sub    r9,   r9,  r6  @@// RE(c3) =  RE(t2) - RE(t3) -- r9(RE(c3))
                
                ldr    r5,   [FFT_step2_in_ptr1, #4]
                ldr    r7,   [FFT_step2_in_ptr3, #4] 
                
                add    r7,   r5, r7            @@// IM(t3) -- r7
                sub    r5,   r7, r5,  lsl #1   @@// RE(t4) -- r5
                
                
                add    r11,  r11, r7           @@// IM(t2) + IM(t3)
                sub    r7,   r11, r7,  lsl #1  @@// IM(t2) - IM(t3) -- r7(IM(c3))
                mov    r11,  r11,  asr  #1
                str    r11,   [FFT_step2_in_ptr0], #4 
                @@// r6 and r11 are free, and the other reg, t1(r10, r12), t4:(r5, r8), c3:(r9, r7)
                
                add    r10,   r10,  r5
                sub    r5,    r10,   r5, lsl #1    @@// RE(c4): r10,  RE(c2): r5
                
                add    r12,   r12,  r8
                sub    r8,    r12,  r8, lsl #1     @@// IM(c4): r12,  IM(c2): r8
                @@// r6 and r11 are free, and the other reg, c4(r10, r12), c2:(r5, r8), c3:(r9, r7)
                mov   r5,  r5,  asr #1
                mov   r8,  r8,  asr #1                
                mov   r10, r10, asr #1
                mov   r12, r12, asr #1
                stmia  FFT_step2_in_ptr1!, {r5,  r8}
                stmia  FFT_step2_in_ptr3!, {r10, r12}
                
                mov   r9,  r9,  asr #1
                mov   r10, r7,  asr #1             
                
                
                stmia  FFT_step2_in_ptr2!, {r9,  r10}
                
                
                mov    r14,  #32
AAC_LTP_DEC_FFTStep2Asm_LOOP2:
                @///////////
                ldmia  FFT_step2_in_ptr0, {r5, r6}
                ldmia  FFT_step2_in_ptr2, {r7, r8}
                
                add    r9,   r5,  r7  @@// RE(t2)--r9
                sub    r10,  r5,  r7  @@// RE(t1)--r10
                
                ldr    r5,   [FFT_step2_in_ptr1]
                ldr    r7,   [FFT_step2_in_ptr3]                
                
                add    r11,  r6,  r8  @@// IM(t2)--r11
                sub    r12,  r6,  r8  @@// IM(t1)--r12
                
                add    r6,   r5,  r7  @@// RE(t3)--r6
                sub    r8,   r5,  r7  @@// IM(t4)--r8

                
                add    r5,   r9,  r6  @@// RE(t2) + RE(t3)
                mov    r5,   r5,  asr #1
                str    r5,   [FFT_step2_in_ptr0], #4
                sub    r9,   r9,  r6  @@// RE(c3) =  RE(t2) - RE(t3) -- r9(RE(c3))
                
                ldr    r5,   [FFT_step2_in_ptr1, #4]
                ldr    r7,   [FFT_step2_in_ptr3, #4] 
                
                add    r7,   r5, r7            @@// IM(t3) -- r7
                sub    r5,   r7, r5,  lsl #1   @@// RE(t4) -- r5
                
                
                add    r11,  r11, r7           @@// IM(t2) + IM(t3)
                sub    r7,   r11, r7,  lsl #1  @@// IM(t2) - IM(t3) -- r7(IM(c3))
                mov    r11,  r11,  asr  #1
                str    r11,   [FFT_step2_in_ptr0], #4 
                @@// r6 and r11 are free, and the other reg, t1(r10, r12), t4:(r5, r8), c3:(r9, r7)
                
                add    r10,   r10,  r5
                sub    r6,    r10,   r5, lsl #1    @@// RE(c4): r10,  RE(c2): r5
                ldr    r11,   [FFT_step2_tab_ptr], #4 
                add    r12,   r12,  r8
                sub    r8,    r12,  r8, lsl #1     @@// IM(c4): r12,  IM(c2): r8
                @@// r5 and r11 are free, and the other reg, c4(r10, r12), c2:(r6, r8), c3:(r9, r7)
                @@// c2
                SMULWT  r5,  r6, r11
                rsb     r6,  r6, #0
                SMULWB  r6,  r6, r11
                SMLAWB  r5,  r8, r11, r5
                SMLAWT  r6,  r8, r11, r6       
                
                ldr    r11,   [FFT_step2_tab_ptr], #4                         
                stmia  FFT_step2_in_ptr1!, {r5,  r6}                
                
                @@// c3
                SMULWT  r8,  r9, r11
                rsb     r9,  r9, #0
                SMULWB  r9,  r9, r11
                SMLAWB  r8,  r7, r11, r8
                SMLAWT  r9,  r7, r11, r9
                ldr    r11,   [FFT_step2_tab_ptr], #4  
                stmia  FFT_step2_in_ptr2!, {r8,  r9} 
                
                @@// c4
                SMULWT  r6,  r10, r11
                rsb     r10, r10, #0
                SMULWB  r7,  r10, r11
                SMLAWB  r6,  r12, r11, r6
                SMLAWT  r7,  r12, r11, r7                        
                stmia  FFT_step2_in_ptr3!, {r6,  r7}                
                @///////////
                subs   r14, r14, #1
                BGT    AAC_LTP_DEC_FFTStep2Asm_LOOP2
                
                
                
                
                sub    FFT_step2_tab_ptr, FFT_step2_tab_ptr, #4*4
                mov    r14,  #31
AAC_LTP_DEC_FFTStep2Asm_LOOP3:               
                @///////////
                ldmia  FFT_step2_in_ptr0, {r5, r6}
                ldmia  FFT_step2_in_ptr2, {r7, r8}
                
                add    r9,   r5,  r7  @@// RE(t2)--r9
                sub    r10,  r5,  r7  @@// RE(t1)--r10
                
                ldr    r5,   [FFT_step2_in_ptr1]
                ldr    r7,   [FFT_step2_in_ptr3]                
                
                add    r11,  r6,  r8  @@// IM(t2)--r11
                sub    r12,  r6,  r8  @@// IM(t1)--r12
                
                add    r6,   r5,  r7  @@// RE(t3)--r6
                sub    r8,   r5,  r7  @@// IM(t4)--r8

                
                add    r5,   r9,  r6  @@// RE(t2) + RE(t3)
                mov    r5,   r5,  asr #1
                str    r5,   [FFT_step2_in_ptr0], #4
                sub    r9,   r9,  r6  @@// RE(c3) =  RE(t2) - RE(t3) -- r9(RE(c3))
                
                ldr    r5,   [FFT_step2_in_ptr1, #4]
                ldr    r7,   [FFT_step2_in_ptr3, #4] 
                
                add    r7,   r5, r7            @@// IM(t3) -- r7
                sub    r5,   r7, r5,  lsl #1   @@// RE(t4) -- r5
                
                
                add    r11,  r11, r7           @@// IM(t2) + IM(t3)
                sub    r7,   r11, r7,  lsl #1  @@// IM(t2) - IM(t3) -- r7(IM(c3))
                mov    r11,  r11,  asr  #1
                str    r11,   [FFT_step2_in_ptr0], #4 
                @@// r6 and r11 are free, and the other reg, t1(r10, r12), t4:(r5, r8), c3:(r9, r7)
                
                add    r10,   r10,  r5
                sub    r5,    r10,   r5, lsl #1    @@// RE(c4): r10,  RE(c2): r5
                ldr    r11,   [FFT_step2_tab_ptr], #-4 
                add    r12,   r12,  r8
                sub    r8,    r12,  r8, lsl #1     @@// IM(c4): r12,  IM(c2): r8
                @@// r6 and r11 are free, and the other reg, c4(r10, r12), c2:(r5, r8), c3:(r9, r7)
                @////////////////////c4
                SMULWB  r6,  r10, r11
                rsb     r10, r10, #0
                SMULWT  r10,  r10, r11
                SMLAWT  r6,  r12, r11, r6
                SMLAWB  r10,  r12, r11, r10    
                ldr    r11,   [FFT_step2_tab_ptr], #-4                     
                rsb    r6,  r6, #0
                rsb    r10,  r10, #0
                stmia  FFT_step2_in_ptr3!, {r6,  r10}   
                
                @///////////////////c3
                SMULWB  r12,  r9, r11
                rsb     r9,   r9,  #0                
                SMULWT  r9,  r9, r11
                
                SMLAWT  r12,  r7, r11, r12
                SMLAWB  r9,   r7, r11, r9
                rsb     r12,  r12,  #0
                ldr    r11,   [FFT_step2_tab_ptr], #-4  
                stmia  FFT_step2_in_ptr2!, {r9,  r12} 
                
                
                @///////////////////c2
                SMULWB  r6,  r5, r11
                rsb     r5,  r5, #0
                SMULWT  r9,  r5, r11
                SMLAWT  r6,  r8, r11, r6
                SMLAWB  r9,  r8, r11, r9
                                        
                stmia  FFT_step2_in_ptr1!, {r6,  r9} 
                             
                @///////////
                subs   r14, r14, #1
                BGT    AAC_LTP_DEC_FFTStep2Asm_LOOP3            
                
                
                
                
                
                
                ldmfd   sp!, {r4-r12, pc}
                @ENDFUNC
                
                
                
                
                @//void AAC_LTP_DEC_FFTStep3Asm(int32 *in_data_ptr,
                @//                             int16 *table_ptr)@
FFT_step3_in_ptr0         .req  r0
FFT_step3_in_ptr1         .req  r2
FFT_step3_in_ptr2         .req  r3
FFT_step3_in_ptr3         .req  r4
FFT_step3_tab_ptr         .req  r1




AAC_LTP_DEC_FFTStep3Asm:@   FUNCTION
                .global  AAC_LTP_DEC_FFTStep3Asm
                stmfd   sp!, {r4-r12, r14}             
                
                
                add     FFT_step3_in_ptr1,  FFT_step3_in_ptr0,  #32*4
                add     FFT_step3_in_ptr2,  FFT_step3_in_ptr1,  #32*4
                add     FFT_step3_in_ptr3,  FFT_step3_in_ptr2,  #32*4
                mov     r14,  #8
AAC_LTP_DEC_FFTStep3Asm_EXT_LOOP:                
                stmfd   sp!, {r14}
                @////////////////////////////////////////
                mov     r14,  #16                
AAC_LTP_DEC_FFTStep3Asm_INT_LOOP:                
                @@////////////
                ldmia  FFT_step3_in_ptr0, {r5, r6}
                ldmia  FFT_step3_in_ptr2, {r7, r8}
                
                add    r9,   r5,  r7  @@// RE(t2)--r9
                sub    r10,  r5,  r7  @@// RE(t1)--r10
                
                ldr    r5,   [FFT_step3_in_ptr1]
                ldr    r7,   [FFT_step3_in_ptr3]                
                
                add    r11,  r6,  r8  @@// IM(t2)--r11
                sub    r12,  r6,  r8  @@// IM(t1)--r12
                
                add    r6,   r5,  r7  @@// RE(t3)--r6
                sub    r8,   r5,  r7  @@// IM(t4)--r8

                
                add    r5,   r9,  r6  @@// RE(t2) + RE(t3)
                mov    r5,   r5,  asr #1
                str    r5,   [FFT_step3_in_ptr0], #4
                sub    r9,   r9,  r6  @@// RE(c3) =  RE(t2) - RE(t3) -- r9(RE(c3))
                
                ldr    r5,   [FFT_step3_in_ptr1, #4]
                ldr    r7,   [FFT_step3_in_ptr3, #4] 
                
                add    r7,   r5, r7            @@// IM(t3) -- r7
                sub    r5,   r7, r5,  lsl #1   @@// RE(t4) -- r5
                
                
                add    r11,  r11, r7           @@// IM(t2) + IM(t3)
                sub    r7,   r11, r7,  lsl #1  @@// IM(t2) - IM(t3) -- r7(IM(c3))
                mov    r11,  r11,  asr  #1
                str    r11,   [FFT_step3_in_ptr0], #4 
                @@// r6 and r11 are free, and the other reg, t1(r10, r12), t4:(r5, r8), c3:(r9, r7)
                
                add    r10,   r10,  r5
                sub    r6,    r10,   r5, lsl #1    @@// RE(c4): r10,  RE(c2): r5
                ldr    r11,   [FFT_step3_tab_ptr], #4 
                add    r12,   r12,  r8
                sub    r8,    r12,  r8, lsl #1     @@// IM(c4): r12,  IM(c2): r8
                @@// r5 and r11 are free, and the other reg, c4(r10, r12), c2:(r6, r8), c3:(r9, r7)
                
                @@// C2
                SMULWT  r5,  r6, r11
                rsb     r6,  r6, #0
                SMULWB  r6,  r6, r11
                SMLAWB  r5,  r8, r11, r5
                SMLAWT  r6,  r8, r11, r6       
                
                ldr    r11,   [FFT_step3_tab_ptr], #4                         
                stmia  FFT_step3_in_ptr1!, {r5,  r6}
                
                @@// C3
                SMULWT  r8,  r9, r11
                rsb     r9,  r9, #0
                SMULWB  r9,  r9, r11
                SMLAWB  r8,  r7, r11, r8
                SMLAWT  r9,  r7, r11, r9
                ldr    r11,   [FFT_step3_tab_ptr], #4  
                stmia  FFT_step3_in_ptr2!, {r8,  r9} 
                
                @@// C4
                SMULWT  r6,  r10, r11
                rsb     r10, r10, #0
                SMULWB  r7,  r10, r11
                SMLAWB  r6,  r12, r11, r6
                SMLAWT  r7,  r12, r11, r7                        
                stmia  FFT_step3_in_ptr3!, {r6,  r7}                
                
                @@////////////
                subs    r14,  r14, #1
                BGT     AAC_LTP_DEC_FFTStep3Asm_INT_LOOP
                @//////////////////////////////////////// 
                sub     FFT_step3_tab_ptr,  FFT_step3_tab_ptr,  #96*2
                
                add     FFT_step3_in_ptr0,  FFT_step3_in_ptr0,  #96*4
                add     FFT_step3_in_ptr1,  FFT_step3_in_ptr1,  #96*4                
                add     FFT_step3_in_ptr2,  FFT_step3_in_ptr2,  #96*4
                add     FFT_step3_in_ptr3,  FFT_step3_in_ptr3,  #96*4
                ldmfd   sp!, {r14}
                subs    r14,  r14, #1
                BGT     AAC_LTP_DEC_FFTStep3Asm_EXT_LOOP
                
                
                
                ldmfd   sp!, {r4-r12, pc}
                @ENDFUNC
                
                
                @//void AAC_LTP_DEC_FFTStep4Asm(int32 *in_data_ptr)@
FFT_step4_in_ptr0         .req  r0
FFT_step4_in_ptr1         .req  r2
FFT_step4_in_ptr2         .req  r3
FFT_step4_in_ptr3         .req  r4


AAC_LTP_DEC_FFTStep4Asm:@   FUNCTION
                .global  AAC_LTP_DEC_FFTStep4Asm
                stmfd   sp!, {r4-r12, r14}
                
                ldr     r1,   =0x764230fc
                
                add     FFT_step4_in_ptr1,  FFT_step4_in_ptr0,  #8*4
                add     FFT_step4_in_ptr2,  FFT_step4_in_ptr1,  #8*4
                add     FFT_step4_in_ptr3,  FFT_step4_in_ptr2,  #8*4
                mov    r14,  #32
AAC_LTP_DEC_FFTStep4Asm_LOOP:                
                @///////////////////////////////
                @// 1
                ldmia  FFT_step4_in_ptr0, {r5, r6}
                ldmia  FFT_step4_in_ptr2, {r7, r8}
                
                add    r9,   r5,  r7  @@// RE(t2)--r9
                sub    r10,  r5,  r7  @@// RE(t1)--r10
                
                ldr    r5,   [FFT_step4_in_ptr1]
                ldr    r7,   [FFT_step4_in_ptr3]                
                
                add    r11,  r6,  r8  @@// IM(t2)--r11
                sub    r12,  r6,  r8  @@// IM(t1)--r12
                
                add    r6,   r5,  r7  @@// RE(t3)--r6
                sub    r8,   r5,  r7  @@// IM(t4)--r8

                
                add    r5,   r9,  r6  @@// RE(t2) + RE(t3)
                mov    r5,   r5,  asr #1
                str    r5,   [FFT_step4_in_ptr0], #4
                sub    r9,   r9,  r6  @@// RE(c3) =  RE(t2) - RE(t3) -- r9(RE(c3))
                
                ldr    r5,   [FFT_step4_in_ptr1, #4]
                ldr    r7,   [FFT_step4_in_ptr3, #4] 
                
                add    r7,   r5, r7            @@// IM(t3) -- r7
                sub    r5,   r7, r5,  lsl #1   @@// RE(t4) -- r5
                
                
                add    r11,  r11, r7           @@// IM(t2) + IM(t3)
                sub    r7,   r11, r7,  lsl #1  @@// IM(t2) - IM(t3) -- r7(IM(c3))
                mov    r11,  r11,  asr  #1
                str    r11,   [FFT_step4_in_ptr0], #4 
                @@// r6 and r11 are free, and the other reg, t1(r10, r12), t4:(r5, r8), c3:(r9, r7)
                
                add    r10,   r10,  r5
                sub    r5,    r10,   r5, lsl #1    @@// RE(c4): r10,  RE(c2): r5
                add    r12,   r12,  r8
                sub    r8,    r12,  r8, lsl #1     @@// IM(c4): r12,  IM(c2): r8
                @@// r5 and r11 are free, and the other reg, c4(r10, r12), c2:(r5, r8), c3:(r9, r7)
                
                mov   r5,  r5,  asr #1
                mov   r8,  r8,  asr #1                
                mov   r10, r10, asr #1
                mov   r12, r12, asr #1
                stmia  FFT_step4_in_ptr1!, {r5,  r8}
                stmia  FFT_step4_in_ptr3!, {r10, r12}                
                mov   r9,  r9,  asr #1
                mov   r10, r7,  asr #1                
                stmia  FFT_step4_in_ptr2!, {r9,  r10}
                
                
                @// 2
                ldmia  FFT_step4_in_ptr0, {r5, r6}
                ldmia  FFT_step4_in_ptr2, {r7, r8}
                
                add    r9,   r5,  r7  @@// RE(t2)--r9
                sub    r10,  r5,  r7  @@// RE(t1)--r10
                
                ldr    r5,   [FFT_step4_in_ptr1]
                ldr    r7,   [FFT_step4_in_ptr3]                
                
                add    r11,  r6,  r8  @@// IM(t2)--r11
                sub    r12,  r6,  r8  @@// IM(t1)--r12
                
                add    r6,   r5,  r7  @@// RE(t3)--r6
                sub    r8,   r5,  r7  @@// IM(t4)--r8

                
                add    r5,   r9,  r6  @@// RE(t2) + RE(t3)
                mov    r5,   r5,  asr #1
                str    r5,   [FFT_step4_in_ptr0], #4
                sub    r9,   r9,  r6  @@// RE(c3) =  RE(t2) - RE(t3) -- r9(RE(c3))
                
                ldr    r5,   [FFT_step4_in_ptr1, #4]
                ldr    r7,   [FFT_step4_in_ptr3, #4] 
                
                add    r7,   r5, r7            @@// IM(t3) -- r7
                sub    r5,   r7, r5,  lsl #1   @@// RE(t4) -- r5
                
                
                add    r11,  r11, r7           @@// IM(t2) + IM(t3)
                sub    r7,   r11, r7,  lsl #1  @@// IM(t2) - IM(t3) -- r7(IM(c3))
                mov    r11,  r11,  asr  #1
                str    r11,   [FFT_step4_in_ptr0], #4 
                @@// r6 and r11 are free, and the other reg, t1(r10, r12), t4:(r5, r8), c3:(r9, r7)
                
                add    r10,   r10,  r5
                sub    r6,    r10,   r5, lsl #1    @@// RE(c4): r10,  RE(c2): r6

                add    r12,   r12,  r8
                sub    r8,    r12,  r8, lsl #1     @@// IM(c4): r12,  IM(c2): r8
                @@// r5 and r11 are free, and the other reg, c4(r10, r12), c2:(r6, r8), c3:(r9, r7)
                
                @@// C2
                SMULWT  r5,  r6, r1
                rsb     r6,  r6, #0
                SMULWB  r6,  r6, r1                
                SMLAWB  r5,  r8, r1, r5
                SMLAWT  r6,  r8, r1, r6                             
                stmia  FFT_step4_in_ptr1!, {r5,  r6}
                
                
                @// C3
                ldr    r11, =23170
                add    r8,  r7,  r9
                sub    r9,  r7,  r9                
                SMULWB  r8,  r8, r11                
                SMULWB  r9,  r9, r11                
                stmia  FFT_step4_in_ptr2!, {r8,  r9}                 
                
                @@// C4
                SMULWB  r6,  r10, r1
                rsb     r10, r10, #0
                SMULWT  r7,  r10, r1
                SMLAWT  r6,  r12, r1, r6
                SMLAWB  r7,  r12, r1, r7                        
                stmia  FFT_step4_in_ptr3!, {r6,  r7}        
                
                
                @// 3
                ldmia  FFT_step4_in_ptr0, {r5, r6}
                ldmia  FFT_step4_in_ptr2, {r7, r8}
                
                add    r9,   r5,  r7  @@// RE(t2)--r9
                sub    r10,  r5,  r7  @@// RE(t1)--r10
                
                ldr    r5,   [FFT_step4_in_ptr1]
                ldr    r7,   [FFT_step4_in_ptr3]                
                
                add    r11,  r6,  r8  @@// IM(t2)--r11
                sub    r12,  r6,  r8  @@// IM(t1)--r12
                
                add    r6,   r5,  r7  @@// RE(t3)--r6
                sub    r8,   r5,  r7  @@// IM(t4)--r8

                
                add    r5,   r9,  r6  @@// RE(t2) + RE(t3)
                mov    r5,   r5,  asr #1
                str    r5,   [FFT_step4_in_ptr0], #4
                sub    r9,   r9,  r6  @@// RE(c3) =  RE(t2) - RE(t3) -- r9(RE(c3))
                
                ldr    r5,   [FFT_step4_in_ptr1, #4]
                ldr    r7,   [FFT_step4_in_ptr3, #4] 
                
                add    r7,   r5, r7            @@// IM(t3) -- r7
                sub    r5,   r7, r5,  lsl #1   @@// RE(t4) -- r5
                
                
                add    r11,  r11, r7           @@// IM(t2) + IM(t3)
                sub    r7,   r11, r7,  lsl #1  @@// IM(t2) - IM(t3) -- r7(IM(c3))
                mov    r11,  r11,  asr  #1
                str    r11,   [FFT_step4_in_ptr0], #4 
                @@// r6 and r11 are free, and the other reg, t1(r10, r12), t4:(r5, r8), c3:(r9, r7)
                
                add    r10,   r10,  r5
                sub    r6,    r10,   r5, lsl #1    @@// RE(c4): r10,  RE(c2): r6
                add    r12,   r12,  r8
                sub    r8,    r12,  r8, lsl #1     @@// IM(c4): r12,  IM(c2): r8
                @@// r5 and r11 are free, and the other reg, c4(r10, r12), c2:(r6, r8), c3:(r9, r7)
                
                @@// C2              
                ldr    r11, =23170
                add    r5,  r6, r8
                sub    r6,  r8, r6                
                
                SMULWB  r5,  r5, r11                
                SMULWB  r6,  r6, r11                                
                stmia  FFT_step4_in_ptr1!, {r5,  r6}
                
                
                @// C3
                mov    r8,  r7,  asr #1
                rsb    r9,  r9, #0
                mov    r9,  r9,  asr #1              
                stmia  FFT_step4_in_ptr2!, {r8,  r9}                 
                
                @@// C4
                rsb     r11,  r11,  #0
                add     r7,   r10,  r12
                sub     r6,   r10,  r12
                SMULWB  r6,  r6, r11
                SMULWB  r7,  r7, r11
                stmia  FFT_step4_in_ptr3!, {r6,  r7}  
                
                
                @// 4
                ldmia  FFT_step4_in_ptr0, {r5, r6}
                ldmia  FFT_step4_in_ptr2, {r7, r8}
                
                add    r9,   r5,  r7  @@// RE(t2)--r9
                sub    r10,  r5,  r7  @@// RE(t1)--r10
                
                ldr    r5,   [FFT_step4_in_ptr1]
                ldr    r7,   [FFT_step4_in_ptr3]                
                
                add    r11,  r6,  r8  @@// IM(t2)--r11
                sub    r12,  r6,  r8  @@// IM(t1)--r12
                
                add    r6,   r5,  r7  @@// RE(t3)--r6
                sub    r8,   r5,  r7  @@// IM(t4)--r8

                
                add    r5,   r9,  r6  @@// RE(t2) + RE(t3)
                mov    r5,   r5,  asr #1
                str    r5,   [FFT_step4_in_ptr0], #4
                sub    r9,   r9,  r6  @@// RE(c3) =  RE(t2) - RE(t3) -- r9(RE(c3))
                
                ldr    r5,   [FFT_step4_in_ptr1, #4]
                ldr    r7,   [FFT_step4_in_ptr3, #4] 
                
                add    r7,   r5, r7            @@// IM(t3) -- r7
                sub    r5,   r7, r5,  lsl #1   @@// RE(t4) -- r5
                
                
                add    r11,  r11, r7           @@// IM(t2) + IM(t3)
                sub    r7,   r11, r7,  lsl #1  @@// IM(t2) - IM(t3) -- r7(IM(c3))
                mov    r11,  r11,  asr  #1
                str    r11,   [FFT_step4_in_ptr0], #4 
                @@// r6 and r11 are free, and the other reg, t1(r10, r12), t4:(r5, r8), c3:(r9, r7)
                
                add    r10,   r10,  r5
                sub    r6,    r10,   r5, lsl #1    @@// RE(c4): r10,  RE(c2): r6

                add    r12,   r12,  r8
                sub    r8,    r12,  r8, lsl #1     @@// IM(c4): r12,  IM(c2): r8
                @@// r5 and r11 are free, and the other reg, c4(r10, r12), c2:(r6, r8), c3:(r9, r7)
                
                @@// C2
                SMULWB  r5,  r6, r1
                rsb     r6,  r6, #0
                SMULWT  r6,  r6, r1                
                SMLAWT  r5,  r8, r1, r5
                SMLAWB  r6,  r8, r1, r6                             
                stmia  FFT_step4_in_ptr1!, {r5,  r6}
                
                
                @// C3
                ldr    r11, =-23170
                add    r8,  r7,  r9
                sub    r7,  r9,  r7
                SMULWB  r8,  r8, r11
                SMULWB  r7,  r7, r11
                stmia  FFT_step4_in_ptr2!, {r7,  r8}
                
                @@// C4
                SMULWT  r6,  r10, r1
                rsb     r10, r10, #0
                SMULWB  r7,  r10, r1
                SMLAWB  r6,  r12, r1, r6
                SMLAWT  r7,  r12, r1, r7
                rsb     r6,  r6,  #0
                rsb     r7,  r7,  #0
                stmia  FFT_step4_in_ptr3!, {r6,  r7}
                
                @/////////////////////////////// 
                add     FFT_step4_in_ptr0,  FFT_step4_in_ptr0,  #24*4
                add     FFT_step4_in_ptr1,  FFT_step4_in_ptr1,  #24*4                
                add     FFT_step4_in_ptr2,  FFT_step4_in_ptr2,  #24*4
                add     FFT_step4_in_ptr3,  FFT_step4_in_ptr3,  #24*4                               
                subs    r14, r14,  #1
                BGT     AAC_LTP_DEC_FFTStep4Asm_LOOP
                
                ldmfd   sp!, {r4-r12, pc}
                @ENDFUNC
                
                
                @//void AAC_LTP_DEC_FFTStep5Asm(int32 *in_data_ptr,
                @//                             int32 *out_data_ptr)@
FFT_step5_in_ptr0         .req  r0
FFT_step5_in_ptr1         .req  r2

FFT_step5_out_ptr0        .req  r1
FFT_step5_out_ptr256      .req  r3
FFT_step5_out_ptr512      .req  r4
FFT_step5_out_ptr768      .req  r5


AAC_LTP_DEC_FFTStep5Asm:@   FUNCTION
                .global  AAC_LTP_DEC_FFTStep5Asm
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
                str   r8,   [FFT_step5_out_ptr512], #4 
                ldr   r6,   [FFT_step5_in_ptr0], #121*4               
                
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
                                
                stmia FFT_step5_out_ptr256!, {r6, r7}
                stmia FFT_step5_out_ptr768!, {r9, r10}
                
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
                str   r8,   [FFT_step5_out_ptr512], #4 
                ldr   r6,   [FFT_step5_in_ptr1], #121*4               
                
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
                                
                stmia FFT_step5_out_ptr256!, {r6, r7}
                stmia FFT_step5_out_ptr768!, {r9, r10}
                                
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
                str   r8,   [FFT_step5_out_ptr512], #4 
                ldr   r6,   [FFT_step5_in_ptr0], #121*4               
                
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
                                
                stmia FFT_step5_out_ptr256!, {r6, r7}
                stmia FFT_step5_out_ptr768!, {r9, r10}
                
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
                str   r8,   [FFT_step5_out_ptr512], #4 
                ldr   r6,   [FFT_step5_in_ptr1], #121*4               
                
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
                                
                stmia FFT_step5_out_ptr256!, {r6, r7}
                stmia FFT_step5_out_ptr768!, {r9, r10}
                                
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
                str   r8,   [FFT_step5_out_ptr512], #4 
                ldr   r6,   [FFT_step5_in_ptr0], #121*4               
                
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
                                
                stmia FFT_step5_out_ptr256!, {r6, r7}
                stmia FFT_step5_out_ptr768!, {r9, r10}
                
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
                str   r8,   [FFT_step5_out_ptr512], #4 
                ldr   r6,   [FFT_step5_in_ptr1], #121*4               
                
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
                                
                stmia FFT_step5_out_ptr256!, {r6, r7}
                stmia FFT_step5_out_ptr768!, {r9, r10}
                                
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
                str   r8,   [FFT_step5_out_ptr512], #4 
                ldr   r6,   [FFT_step5_in_ptr0], #121*4               
                
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
                                
                stmia FFT_step5_out_ptr256!, {r6, r7}
                stmia FFT_step5_out_ptr768!, {r9, r10}
                
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
                str   r8,   [FFT_step5_out_ptr512], #4 
                ldr   r6,   [FFT_step5_in_ptr1], #121*4               
                
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
                                
                stmia FFT_step5_out_ptr256!, {r6, r7}
                stmia FFT_step5_out_ptr768!, {r9, r10}
                                
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
                
                
                @/////////////////////////////////////////////////////
    	        @@/* LTP synthesis model */
    	        @@//void AAC_LTP_DEC_SynthesisAsm(int32 *spec_ptr,
                @@//                              int32 *ltp_est_ptr,
                @@//                              int16 *swb_offset_ptr,
                @@//                              AAC_DEC_LTP_INFO_T *ltp_info_ptr)@
ltp_syn_spec_ptr       .req  r0
ltp_syn_est_ptr        .req  r1
ltp_syn_swb_offset_ptr .req  r2
ltp_syn_info_ptr       .req  r3
ltp_syn_low            .req  r4

ltp_syn_high           .req  r5
        

AAC_LTP_DEC_SynthesisAsm:@   FUNCTION
                .global  AAC_LTP_DEC_SynthesisAsm
                stmfd   sp!, {r4-r12, r14}
    	        
    	        ldrsh   r14, [ltp_syn_info_ptr], #4
    	        ldrsh   ltp_syn_low,  [ltp_syn_swb_offset_ptr], #2
    	        
AAC_LTP_DEC_SynthesisAsm_EXT_LOOP:
    	        @@////////////////////
    	        ldrsh   ltp_syn_high,  [ltp_syn_swb_offset_ptr], #2
    	        sub     r12, ltp_syn_high, ltp_syn_low
    	        mov     ltp_syn_low,  ltp_syn_high
    	        
    	        ldrb    r11,  [ltp_syn_info_ptr], #1
    	        cmp     r11,   #0
    	        beq     AAC_LTP_DEC_SynthesisAsmBRANCH
    	        
    	        cmp    r12,   #1024
    	        movge  r0,    #0x1200
    	        bge    AAC_LTP_DEC_SynthesisAsmEXIT
    	        @@///////////////////////////////////
AAC_LTP_DEC_SynthesisAsm_INT_LOOP:
    	        ldmia   ltp_syn_spec_ptr, {r5-r8}
    	        ldmia   ltp_syn_est_ptr!, {r9-r11}
    	        
    	        add     r5,   r5,   r9,  asr #2
    	        add     r6,   r6,   r10, asr #2
    	        ldr     r9,   [ltp_syn_est_ptr], #4
    	        
    	        add     r7,   r7,   r11, asr #2
    	        add     r8,   r8,   r9,  asr #2
    	        
    	        stmia   ltp_syn_spec_ptr!, {r5-r8}
    	        
    	        subs    r12,  r12,  #4
    	        BGT     AAC_LTP_DEC_SynthesisAsm_INT_LOOP
    	        
    	        @@///////////////////////////////////
    	        b     AAC_LTP_DEC_SynthesisAsmNEXTLOOP
AAC_LTP_DEC_SynthesisAsmBRANCH:
                add    ltp_syn_spec_ptr,  ltp_syn_spec_ptr,  r12, lsl  #2
                add    ltp_syn_est_ptr,   ltp_syn_est_ptr,   r12, lsl  #2
AAC_LTP_DEC_SynthesisAsmNEXTLOOP:
                
    	        @@////////////////////
    	        subs    r14,  r14,  #1
    	        BGT     AAC_LTP_DEC_SynthesisAsm_EXT_LOOP
    	        
    	        movge  r0,    #0
AAC_LTP_DEC_SynthesisAsmEXIT:
    	        
    	        ldmfd   sp!, {r4-r12, pc}
                @ENDFUNC
                
                
                
                
                
                
                
                
                
                @/////////////////////////////////////////////////////
    	        @@/* LTP synthesis model */
    	        @@//void AAC_LTP_RecSampleAsm(int32 *overlap_ptr,
                @@//                          int16 *last0_pcm_ptr,
                @@//                          int16 *last1_pcm_ptr,
                @@//                          int16 *rec_out_ptr)@
overlap_ptr     .req  r0                
last0_pcm_ptr   .req  r1         
last1_pcm_ptr   .req  r2
rec_out_ptr     .req  r3       
AAC_LTP_RecSampleAsm:@   FUNCTION
                .global  AAC_LTP_RecSampleAsm
                stmfd   sp!, {r4-r12, r14}
                
                
                ldrsh   r14,    [rec_out_ptr]
                ldr    r11, =0x00007FFF
                rsb     r14,    r14,   #2048
                cmp     r14,    #1024
                
                bge      AAC_LTP_RecSampleAsm_BRANCH
                @@/* < 1024 */
                
                add     last1_pcm_ptr,   last1_pcm_ptr,   r14,  lsl #1
                rsb     r12,   r14,  #1024   
                
                @@///////////////////////////////
                add     r9,   rec_out_ptr, r12, lsl #1
                mov     r10,  #1024
AAC_LTP_RecSampleAsm_LOOP01:
                ldrsh   r4, [last0_pcm_ptr], #2
                ldrsh   r5, [last0_pcm_ptr], #2
                ldrsh   r6, [last0_pcm_ptr], #2
                ldrsh   r7, [last0_pcm_ptr], #2
                strh   r4, [r9], #2
                strh   r5, [r9], #2
                strh   r6, [r9], #2
                strh   r7, [r9], #2                
                subs    r10,  r10,  #4
                BGT     AAC_LTP_RecSampleAsm_LOOP01   
                @@///////////////////////////////
                ands    r4,   r12,  #1                
                bne     AAC_LTP_RecSampleAsm_LOOP00_NEXT
                @@/* = 0 */                             
AAC_LTP_RecSampleAsm_LOOP00:               
                ldrsh  r4, [last1_pcm_ptr], #2
                ldrsh  r5, [last1_pcm_ptr], #2
                strh   r4, [rec_out_ptr], #2
                strh   r5, [rec_out_ptr], #2
                subs    r12,  r12,  #2
                BGT     AAC_LTP_RecSampleAsm_LOOP00               
           
                add    rec_out_ptr,  rec_out_ptr,  #1024*2
AAC_LTP_RecSampleAsm_LOOP02:
                ldmia  overlap_ptr!, {r4-r5}
                add    r4,    r4,  #2048
                mov    r4,  r4,  asr #12	                
                mov    r9,  r4, asr #15
                teq    r9,  r4, asr #31
                EORNE  r4, r11, r4, asr #31	                
                
                add    r5,    r5,  #2048
                mov    r5,  r5,  asr #12	                
                mov    r9,  r5, asr #15
                teq    r9,  r5, asr #31
                EORNE  r5, r11, r5, asr #31	                
                                
                strh   r4, [rec_out_ptr], #2
                strh   r5, [rec_out_ptr], #2
                
                subs    r14,  r14,  #2
                BGT     AAC_LTP_RecSampleAsm_LOOP02      
                
                @@////////////////////////////////////          
                b       AAC_LTP_RecSampleAsm_BRANCHEXIT
                @@/////////////
AAC_LTP_RecSampleAsm_LOOP00_NEXT:                
                @@/* != 0*/
                
                ldrsh  r4, [last1_pcm_ptr], #2
                strh   r4, [rec_out_ptr], #2
                subs    r12,  r12,  #1
                beq     AAC_LTP_RecSampleAsm_LOOP000_SKIP
AAC_LTP_RecSampleAsm_LOOP000:
                ldrsh  r4, [last1_pcm_ptr], #2
                ldrsh  r5, [last1_pcm_ptr], #2
                strh   r4, [rec_out_ptr], #2
                strh   r5, [rec_out_ptr], #2                                
                subs    r12,  r12,  #2
                BGT     AAC_LTP_RecSampleAsm_LOOP000
           
AAC_LTP_RecSampleAsm_LOOP000_SKIP:  

         
           
                add    rec_out_ptr,  rec_out_ptr,  #1024*2

                ldr    r4, [overlap_ptr], #4                
                add    r4,    r4,  #2048
                mov    r4,  r4,  asr #12	                
                mov    r9,  r4, asr #15
                teq    r9,  r4, asr #31
                EORNE  r4, r11, r4, asr #31	                
                strh   r4, [rec_out_ptr], #2
                subs   r14,  r14,  #1
                beq    AAC_LTP_RecSampleAsm_BRANCHEXIT
                
AAC_LTP_RecSampleAsm_LOOP002:
                
                
                ldmia  overlap_ptr!, {r4-r5}
                add    r4,    r4,  #2048
                mov    r4,  r4,  asr #12                
                mov    r9,  r4, asr #15
                teq    r9,  r4, asr #31
                EORNE  r4, r11, r4, asr #31	  
                
                
                
                add    r5,    r5,  #2048
                mov    r5,    r5,  asr #12	                
                mov    r9,    r5, asr #15
                teq    r9,    r5, asr #31
                EORNE  r5, r11, r5, asr #31	
                
                             
                strh   r4, [rec_out_ptr], #2
                
                strh   r5, [rec_out_ptr], #2                
                
                subs    r14,  r14,  #2
                BGT     AAC_LTP_RecSampleAsm_LOOP002                
                
                
                
                
                
                
                
                
                
                
                
                
                
                @@////////////////////////////////////          
                b       AAC_LTP_RecSampleAsm_BRANCHEXIT
                @@/////////////
AAC_LTP_RecSampleAsm_BRANCH:                
                @@/* >= 1024 */
                sub    r14,  r14,   #1024
                
                add    last0_pcm_ptr, last0_pcm_ptr, r14,  lsl  #1
                rsb     r12,   r14,  #1024
                
                
                @@/**/
                mov    r10,  r12,   asr #1                
                and    r10,  r10,  #0x1FC
                
                add    r4,   rec_out_ptr,  r10, lsl #2
                add    r4,   r4,  #512*4
                
                rsb    r10,  r10,  #512
                @@/* clear */
                mov    r5,   #0
                mov    r6,   #0
                mov    r7,   #0
                mov    r8,   #0
AAC_LTP_RecSampleAsm_LOOP12:           
                stmia  r4!, {r5-r8}
                subs    r10,  r10,  #4
                BGT     AAC_LTP_RecSampleAsm_LOOP12                    
                
                cmp     r12,  #0
                beq     AAC_LTP_RecSampleAsm_LOOP10_SKIP
                
                ands    r10,   r12,  #0x3
                rsbne   r10,   r10,   #4
                        
AAC_LTP_RecSampleAsm_LOOP10:
                ldrsh   r4, [last0_pcm_ptr], #2
                ldrsh   r5, [last0_pcm_ptr], #2
                
                ldrsh   r6, [last0_pcm_ptr], #2
                ldrsh   r7, [last0_pcm_ptr], #2
                
                strh    r4, [rec_out_ptr], #2
                strh    r5, [rec_out_ptr], #2
                
                strh    r6, [rec_out_ptr], #2
                strh    r7, [rec_out_ptr], #2
                
                subs    r12,  r12,  #4
                BGT     AAC_LTP_RecSampleAsm_LOOP10                
                sub     rec_out_ptr, rec_out_ptr, r10, lsl #1
                
AAC_LTP_RecSampleAsm_LOOP10_SKIP:
                
                mov    r12,  #1024
AAC_LTP_RecSampleAsm_LOOP11:
                ldmia  overlap_ptr!, {r4-r7}
                add    r4,    r4,  #2048
                add    r5,    r5,  #2048
                add    r6,    r6,  #2048
                add    r7,    r7,  #2048
                
                mov    r4,  r4,  asr #12	
                mov    r5,  r5,  asr #12	
                mov    r6,  r6,  asr #12	
                mov    r7,  r7,  asr #12	
                
                mov    r9,  r4, asr #15
                teq    r9,  r4, asr #31
                EORNE  r4, r11, r4, asr #31	
                
                
                mov    r9,  r5, asr #15
                teq    r9,  r5, asr #31
                EORNE  r5, r11, r5, asr #31	
                
                
                mov    r9,  r6, asr #15
                teq    r9,  r6, asr #31
                EORNE  r6, r11, r6, asr #31	
                
                
                mov    r9,  r6, asr #15
                teq    r9,  r6, asr #31
                EORNE  r6, r11, r6, asr #31	              
                
                
                
                strh   r4, [rec_out_ptr], #2
                strh   r5, [rec_out_ptr], #2
                strh   r6, [rec_out_ptr], #2
                strh   r7, [rec_out_ptr], #2
                
                subs    r12,  r12,  #4
                BGT     AAC_LTP_RecSampleAsm_LOOP11                
                
                @@/////////////
AAC_LTP_RecSampleAsm_BRANCHEXIT:
                
                
                
                                
                ldmfd   sp!, {r4-r12, pc}
                @ENDFUNC
                
                
                
        .end@END
        
        