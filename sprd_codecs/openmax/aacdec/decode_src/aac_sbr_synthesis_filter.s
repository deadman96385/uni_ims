@// SBR synthesis filter bank
@// author: reed.zhang
@// input: SBR synthesis filter bank processing
@//     r0: output sample pointer
@// r1: the input data
@// purpose:

@// define the register name 
k           .req      r14
tab0        .req      r10
tab1        .req      r11
tab2        .req      r12
sum1        .req      r9
sum2        .req      r6
data0       .req      r3
data1       .req      r4
data2       .req      r10
rel_ptr     .req      r7
img_ptr     .req      r8
tab_ptr     .req      r2

out_rel_ptr .req      r0
out_img_ptr .req      r5
in_data_ptr  .req     r1



                .text@AREA    PROGRAM, CODE, READONLY                         
                .arm@CODE32                          
                .global  asm_sbr_synthesis_filter
asm_sbr_synthesis_filter:
                stmfd   sp!, {r4 - r11, r14}
                
                ldr tab_ptr, =SBR_SYNTHESIS_FILTER_TABLE
                ldr    k, =0x00007FFF                 
                add       img_ptr,  r1,   #512*4
                @@///////////////////////                
                ldr    data0,    [in_data_ptr,   #192*4]
                ldr    data1,    [img_ptr,   #(1216-512)*4]
                ldr  tab0,  =0x5601B3
                mov   sum1,   #512
                add    data0,  data0,  data1
                SMLAWT  sum1,   data0,  tab0,  sum1
                
                ldr    data0,    [in_data_ptr,   #256*4]
                ldr    data1,    [img_ptr,   #(1024-512)*4]
                ldr  tab1,  =0x9012E3A
                sub    data0,  data0,  data1
                SMLAWB  sum1,   data0,  tab0,  sum1                
                
                ldr    data0,    [in_data_ptr,   #448*4]
                ldr    data1,    [img_ptr,   #(960-512)*4]
                ldr    sum2,    [img_ptr,   #(768-512)*4]
                add    data0,  data0,  data1
                SMLAWT  sum1,   data0,  tab1,  sum1
                
                ldr    data0,    [in_data_ptr,   #512*4]
                
                ldr   tab0,    =0x6D47FFEA
                sub    data0,  data0,  sum2             
                ldr    data1,    [in_data_ptr,   #704*4]                
                SMLAWB  sum1,   data0,  tab1,  sum1
                SMLAWT  sum1,   data1,  tab0,  sum1             
                 
                ldr    data0,    [in_data_ptr,   #32*4]
                ldr    data1,    [img_ptr,   #(1216+32-512)*4]
                
                mov    sum1,    sum1, asr#10
                mov    tab2,   sum1, asr#15
                teq      tab2, sum1, asr #31
                eorne  sum1, k, sum1, asr #31
                strh     sum1,   [out_rel_ptr],  #2
                                
                mov   sum2,   #512
                add    data0,  data0,  data1
                SMLAWB  sum2,   data0,  tab0,  sum2
                
                ldr    data0,    [in_data_ptr,   #(192+32)*4]
                ldr    data1,    [img_ptr,   #(1024+32-512)*4]
                ldr    tab0,     =0xAD0780
                add    data0,  data0,  data1
                SMLAWT  sum2,   data0,  tab0,  sum2
                
                ldr    data0,    [in_data_ptr,   #(256+32)*4]
                ldr    data1,    [img_ptr,   #(960+32-512)*4]
                ldr    tab1,     =0xF88759E3
                add    data0,  data0,  data1
                SMLAWB  sum2,   data0,  tab0,  sum2
                
                ldr    data0,    [in_data_ptr,   #(448+32)*4]
                ldr    data1,    [img_ptr,   #(768+32-512)*4]
                ldr    data2,    [img_ptr,   #32*4]
                add    data0,  data0,  data1
                ldr     data1,   [img_ptr,   #(704+32-512)*4]
                     
                SMLAWT  sum2,   data0,  tab1,  sum2
                add   data1,    data1,   data2           
                SMLAWB  sum2,   data1,  tab1,  sum2
                
                add    rel_ptr,   in_data_ptr,  #4
                add    img_ptr,   in_data_ptr,  #1280*4
                sub    img_ptr,   img_ptr,   #4
                
                mov   r1,  #31                
                mov    sum2,    sum2, asr#10
                mov    tab2,   sum2, asr#15
                teq     tab2,   sum2, asr #31
                eorne  sum2, k, sum2, asr #31
                
                add     out_img_ptr,  out_rel_ptr,  #62*2
                strh     sum2,   [out_rel_ptr, #31*2]
                
                
                
                ldr   tab2,    =4860
FILTER_LOOP1:
        @// the function body
        @//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@        
                mov  sum1, #1024
                mov  sum2, #1024
                ldmia  tab_ptr!, {tab0-tab1}                 
                ldr    data0, [rel_ptr], #0x300
                ldr    data1, [rel_ptr], #0x100                
                @ // s1
                smlawt sum1, data0, tab0, sum1
                ldr    data0, [img_ptr], #-0x300                
                smlawb sum1, data1, tab0, sum1
                ldr    data1, [img_ptr], #-0x100                
                smlawt sum2, data0, tab0, sum2
                ldr    data0, [rel_ptr], #0x300                
                smlawb sum2, data1, tab0, sum2
                ldr    data1, [rel_ptr], #0x100
                ldr  tab0, [tab_ptr], #4
        	@ // s2
                smlawt sum1, data0, tab1, sum1
                ldr    data0, [img_ptr], #-0x300                
                smlawb sum1, data1, tab1, sum1
                ldr    data1, [img_ptr], #-0x100                
                smlawt sum2, data0, tab1, sum2
                ldr    data0, [rel_ptr], #0x300                
                smlawb sum2, data1, tab1, sum2
                ldr    data1, [rel_ptr], #0x100                
                @ // s3
                add      sum1,  sum1,   data0,  asr  #1
                smlawt sum1, data0, tab0, sum1
                ldr    data0, [img_ptr], #-0x300
                add      sum1,  sum1,   data1,  asr  #1
                smlawb sum1, data1, tab0, sum1
                ldr    data1, [img_ptr], #-0x100
                add      sum2,  sum2,   data0,  asr  #1
                smlawt sum2, data0, tab0, sum2
                ldr    data0, [rel_ptr], #0x300
                add      sum2,  sum2,   data1,  asr  #1
                smlawb sum2, data1, tab0, sum2
                ldr    data1, [rel_ptr], #0x100                
                @ // s4
                ldmia  tab_ptr!, {tab0-tab1} 
                smlawt sum1, data0, tab0, sum1
                ldr    data0, [img_ptr], #-0x300                
                smlawb sum1, data1, tab0, sum1
                ldr    data1, [img_ptr], #-0x100                
                smlawt sum2, data0, tab0, sum2
                ldr    data0, [rel_ptr], #0x300
                smlawb sum2, data1, tab0, sum2
                ldr    data1, [rel_ptr], -tab2                
                @ // s5
                smlawt sum1, data0, tab1, sum1
                ldr    data0, [img_ptr], #-0x300                
                smlawb sum1, data1, tab1, sum1
                ldr    data1, [img_ptr], tab2                
                smlawt sum2, data0, tab1, sum2                          
                smlawb sum2, data1, tab1, sum2                
                @ // saturation processing       
                mov    sum1, sum1, asr#11
                mov    data1, sum1, asr#15
                teq     data1, sum1, asr #31
                eorne  sum1, k, sum1, asr #31
                strh     sum1, [out_rel_ptr], #0x2
                
                mov    sum2, sum2, asr#11       
                mov    data1, sum2, asr#15
                teq      data1, sum2, asr #31
                eorne  sum2, k, sum2, asr #31
                @ // strore the result          
                strh   sum2, [out_img_ptr], #-0x2
                @//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
                @// the loop control
                subs   r1,  r1,  #1
                BGT FILTER_LOOP1
                
                ldmfd   sp!, {r4 - r11, pc} 
                
                @AREA SBR_SYNTHESIS_FILTER_TABLE_D, DATA, READONLY
SBR_SYNTHESIS_FILTER_TABLE:

     .word  0xffdc00b7,  0x03af11a8,  0xdf545a84,  0xa6671253,  0xfce100a1 
     .word  0xffdb00c1,  0x03fa1145,  0xe2365a65,  0xa940129b,  0xfd260097 
     .word  0xffe000cc,  0x044910d6,  0xe51a5a31,  0xac1512da,  0xfd69008d 
     .word  0xffe000d7,  0x0498105f,  0xe7ff59e8,  0xaee41310,  0xfda90082 
     .word  0xffe000e2,  0x04e90fdd,  0xeae6598b,  0xb1b0133e,  0xfde80078 
     .word  0xffdf00ec,  0x053c0f50,  0xedcd5919,  0xb4761363,  0xfe24006f 
     .word  0xffde00f6,  0x05910eb9,  0xf0b55892,  0xb7371380,  0xfe5e0065 
     .word  0xffdc0101,  0x05e80e17,  0xf39c57f8,  0xb9f11396,  0xfe95005b 
     .word  0xffdb010c,  0x06400d6b,  0xf6835749,  0xbca513a4,  0xfecb0052 
     .word  0xffda0115,  0x069a0cb2,  0xf9685685,  0xbf5213ab,  0xfefe004a 
     .word  0xffd8011f,  0x06f60bf0,  0xfc4b55ae,  0xc1f913aa,  0xff2f0041 
     .word  0xffd70128,  0x07530b21,  0xff2c54c3,  0xc49713a3,  0xff5d0038 
     .word  0xffd50131,  0x07b10a47,  0x020b53c5,  0xc72d1395,  0xff8a0031 
     .word  0xffd4013a,  0x08100961,  0x04e552b3,  0xc9bc1382,  0xffb40029 
     .word  0xffd30142,  0x08710870,  0x07bd518e,  0xcc421368,  0xffdc0021 
     .word  0xffd1014a,  0x08d30772,  0x0a905056,  0xcebf1348,  0x0002001a 
     .word  0xffd00151,  0x09350668,  0x0d5d4f0c,  0xd1331323,  0x00260013 
     .word  0xffcf0158,  0x09980553,  0x10264daf,  0xd39d12f8,  0x0047000d 
     .word  0xffcf015e,  0x09fc0431,  0x12e84c40,  0xd5fe12c8,  0x00670007 
     .word  0xffce0163,  0x0a600303,  0x15a44abf,  0xd8541294,  0x00850001 
     .word  0xffcd0168,  0x0ac501c8,  0x1859492d,  0xdaa1125b,  0x00a1fffc 
     .word  0xffcd016c,  0x0b2a0082,  0x1b074789,  0xdce3121e,  0x00bafff7 
     .word  0xffcd016f,  0x0b8fff2e,  0x1dac45d5,  0xdf1b11dc,  0x00d2fff2 
     .word  0xffcd0170,  0x0bf4fdce,  0x204a4410,  0xe1471197,  0x00e8ffed 
     .word  0xffcd0172,  0x0c58fc62,  0x22de423b,  0xe369114f,  0x00fcffe9 
     .word  0xffcd0172,  0x0cbcfae9,  0x25694056,  0xe57f1102,  0x010effe5 
     .word  0xffce0171,  0x0d1ff964,  0x27e93e62,  0xe78a10b3,  0x011fffe2 
     .word  0xffcf016e,  0x0d82f7d2,  0x2a5f3c5f,  0xe9891060,  0x012effde 
     .word  0xffd0016b,  0x0de3f633,  0x2ccb3a4d,  0xeb7c100c,  0x013bffdc 
     .word  0xffd10167,  0x0e43f489,  0x2f2a382e,  0xed640fb4,  0x0147ffd9 
     .word  0xffd30161,  0x0ea2f2d2,  0x317e3600,  0xef3f0f5b,  0x0151ffd6 

                
                
                .end@END
                
                
                
                
                
                
                
                
                
                
                
                
                
                