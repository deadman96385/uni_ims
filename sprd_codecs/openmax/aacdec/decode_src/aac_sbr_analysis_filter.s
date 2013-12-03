@// SBR analysis filter bank
@// author: reed.zhang
@// input: SBR analysis filter bank processing
@// r0: output sample pointer
@// r1: the raw data
@// r2: the input data
@// purpose:

@// define the register name 
k              .req      r14
ana_tab_ptr    .req      r12
tab1           .req      r11
tab0           .req      r10
tmp2           .req      r9
t0             .req      r7
data0          .req      r5
data1          .req      r6
t_x_ptr        .req      r2
input_ptr      .req      r1
out_rel_ptr    .req      r0
out_img_ptr    .req      r3
x_re           .req      r4
x_im           .req      r8

                .text@AREA    PROGRAM, CODE, READONLY                         
                .arm
                .global  asm_sbr_analysis_filter
asm_sbr_analysis_filter:
          stmfd   sp!, {r4 - r11, r14}
          ldr ana_tab_ptr, =ANALYSIS_FILTRS               
          add out_img_ptr, out_rel_ptr, #0x80  @ // out the in_real[64-n]                
           @//mov   x_re, #0
           @//mov   x_im, #0     
           ldr     data0,  [t_x_ptr, #32*4]           
           ldr     data1,  [t_x_ptr, #96*4]
           ldr     t0,       [t_x_ptr, #160*4]
           ldr     k,        [t_x_ptr, #224*4]           
           sub   data1,   data1,   t0
           sub   data0,   data0,   k
           ldr     tab1,  =0x1B32e3a
           mov   tab0,  #86           
           SMULWT   x_re,    data0,   tab1 
           SMLAWB   x_re,    data1,   tab1,  x_re
           ldr     data0,   [t_x_ptr,  #0]
           ldr     data1,   [t_x_ptr,  #64*4]
           ldr     t0,        [t_x_ptr,  #128*4]
           ldr     k,         [t_x_ptr,  #192*4]
           ldr     tmp2,   [t_x_ptr,  #256*4]
                      
           ldr     tab1,    =0x9016D47  @@// 2305,    27975
           add   data0,  data0,   tmp2
           SMULWB    x_im,   data0,   tab0
           add   data1,   data1,   k           
           SMLAWT    x_im,   data1,   tab1,  x_im
           ldr       k,   =0xC917FFF
           SMLAWB    x_im,   t0,   tab1,  x_im           
           @@//////////////////////////////////////////////////////
           SMULWB    tab0,    x_re,     k
           rsb       x_re,  x_re,  #0
           SMULWT    tab1,     x_re,     k           
           mov             tab0,    tab0,   lsl  #3
           SMLAWT    tab0,    x_im,    k,   tab0
           mov             x_im,    x_im,   lsl  #3
           SMLAWB    tab1,    x_im,    k,   tab1
           str     tab0,    [out_rel_ptr],  #4
           str     tab1,    [out_img_ptr],  #4          
           
           mov     k,  #31 
           sub      t_x_ptr,  t_x_ptr,  #4          
FILTER_LOOP2:
            @// the function body
            @//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@      
            ldr   t0, [input_ptr], #0x4
            ldmia ana_tab_ptr!, {tab0-tab1}
            mov   t0,  t0,   asr  #2
            str   t0, [t_x_ptr], #-0x4                                    
            smulwt x_im, t0, tab0   @ // acc 1
            ldr    data0, [t_x_ptr, #0x104]
            ldr    data1, [t_x_ptr, #0x204]
            smlawb x_im, data0, tab0, x_im  @ // acc 2
            ldr    data0, [t_x_ptr, #0x304]
            add   x_im,   x_im,  data1,  asr  #1
            smlawt x_im, data1, tab1, x_im  @ // acc 3      
            ldr    data1, [t_x_ptr, #0x404]
            smlawb x_im, data0, tab1, x_im  @ // acc 4
            
            ldmia ana_tab_ptr!, {tab0-tab1}
            ldr    data0, [t_x_ptr, #0x84]
            smlawt x_im, data1, tab0, x_im  @ // acc 5
            ldr    data1, [t_x_ptr, #0x184]
            
            smulwb x_re, data0, tab0   @ // acc 1
            ldr    data0, [t_x_ptr, #0x284]            
            smlawt x_re, data1, tab1, x_re  @ // acc 2
            ldr    data1, [t_x_ptr, #0x384]   
            add      x_re,  x_re,   data0,  asr  #1         
            smlawb x_re, data0, tab1, x_re  @ // acc 3
            ldmia ana_tab_ptr!, {tab0-tab1}
            ldr    data0, [t_x_ptr, #0x484]
            smlawt x_re, data1, tab0, x_re  @ // acc 4
            smlawb x_re, data0, tab0, x_re  @ // acc 5     
            subs  k, k, #1       
            @@////////////////////////////////////////////////////            
            sub    t0,      x_im,   x_re
            add   tmp2,  x_im,   x_re
            mov             x_re,    x_re,   lsl  #1
            mov             x_im,    x_im,   lsl  #1
            SMLAWT    tmp2,    x_re,     tab1,   tmp2
            rsb                x_re,    x_re,  #0
            SMLAWB   t0,         x_re,     tab1,     t0
            SMLAWB  tmp2,    x_im,     tab1,     tmp2
            SMLAWT   t0,        x_im,     tab1,     t0        
            str tmp2, [out_img_ptr], #0x4
            str t0, [out_rel_ptr], #0x4                     
            @//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
            @// the loop control
            
            BGT  FILTER_LOOP2
                
                ldr tmp2, [input_ptr]
                mov   tmp2,  tmp2,   asr  #2
                str tmp2, [t_x_ptr]
                ldmfd   sp!, {r4 - r11, pc} 
                
                @AREA SBR_SYNTHESIS_FILTER_TABLE_D, DATA, READONLY
ANALYSIS_FILTRS:
           .word    0x0097129b,    0x5a651145,    0x00c1fd26,    0xa940e236,    0x03faffdb,    0x8fb37f85
           .word    0x00821310,    0x59e8105f,    0x00d7fda9,    0xaee4e7ff,    0x0498ffe0,    0x9c387e71
           .word    0x006f1363,    0x59190f50,    0x00ecfe24,    0xb476edcd,    0x053cffdf,    0xa8ab7cc0
           .word    0x005b1396,    0x57f80e17,    0x0101fe95,    0xb9f1f39c,    0x05e8ffdc,    0xb5057a73
           .word    0x004a13ab,    0x56850cb2,    0x0115fefe,    0xbf52f968,    0x069affda,    0xc13f778c
           .word    0x003813a3,    0x54c30b21,    0x0128ff5d,    0xc497ff2c,    0x0753ffd7,    0xcd50740c
           .word    0x00291382,    0x52b30961,    0x013affb4,    0xc9bc04e5,    0x0810ffd4,    0xd9326ff5
           .word    0x001a1348,    0x50560772,    0x014a0002,    0xcebf0a90,    0x08d3ffd1,    0xe4dd6b4b
           .word    0x000d12f8,    0x4daf0553,    0x01580047,    0xd39d1026,    0x0998ffcf,    0xf0496610
           .word    0x00011294,    0x4abf0303,    0x01630085,    0xd85415a4,    0x0a60ffce,    0xfb706046
           .word    0xfff7121e,    0x47890082,    0x016c00ba,    0xdce31b07,    0x0b2affcd,    0x064c59f2
           .word    0xffed1197,    0x4410fdce,    0x017000e8,    0xe147204a,    0x0bf4ffcd,    0x10d45318
           .word    0xffe51102,    0x4056fae9,    0x0172010e,    0xe57f2569,    0x0cbcffcd,    0x1b034bbc
           .word    0xffde1060,    0x3c5ff7d2,    0x016e012e,    0xe9892a5f,    0x0d82ffcf,    0x24d243e2
           .word    0xffd90fb4,    0x382ef489,    0x01670147,    0xed642f2a,    0x0e43ffd1,    0x2e3c3b8f
           .word    0xffd40f00,    0x33c6f10f,    0x01590159,    0xf10f33c6,    0x0f00ffd4,    0x373a32c9
           .word    0xffd10e43,    0x2f2aed64,    0x01470167,    0xf489382e,    0x0fb4ffd9,    0x3fc72994
           .word    0xffcf0d82,    0x2a5fe989,    0x012e016e,    0xf7d23c5f,    0x1060ffde,    0x47de1ff7
           .word    0xffcd0cbc,    0x2569e57f,    0x010e0172,    0xfae94056,    0x1102ffe5,    0x4f7a15f7
           .word    0xffcd0bf4,    0x204ae147,    0x00e80170,    0xfdce4410,    0x1197ffed,    0x56960b9a
           .word    0xffcd0b2a,    0x1b07dce3,    0x00ba016c,    0x00824789,    0x121efff7,    0x5d2d00e8
           .word    0xffce0a60,    0x15a4d854,    0x00850163,    0x03034abf,    0x12940001,    0x633cf5e6
           .word    0xffcf0998,    0x1026d39d,    0x00470158,    0x05534daf,    0x12f8000d,    0x68bfea9b
           .word    0xffd108d3,    0x0a90cebf,    0x0002014a,    0x07725056,    0x1348001a,    0x6db3df0f
           .word    0xffd40810,    0x04e5c9bc,    0xffb4013a,    0x096152b3,    0x13820029,    0x7213d348
           .word    0xffd70753,    0xff2cc497,    0xff5d0128,    0x0b2154c3,    0x13a30038,    0x75dfc74d
           .word    0xffda069a,    0xf968bf52,    0xfefe0115,    0x0cb25685,    0x13ab004a,    0x7913bb27
           .word    0xffdc05e8,    0xf39cb9f1,    0xfe950101,    0x0e1757f8,    0x1396005b,    0x7badaedc
           .word    0xffdf053c,    0xedcdb476,    0xfe2400ec,    0x0f505919,    0x1363006f,    0x7daca274
           .word    0xffe00498,    0xe7ffaee4,    0xfda900d7,    0x105f59e8,    0x13100082,    0x7f0e95f7
           .word    0xffdb03fa,    0xe236a940,    0xfd2600c1,    0x11455a65,    0x129b0097,    0x7fd4896c
               .end@ END