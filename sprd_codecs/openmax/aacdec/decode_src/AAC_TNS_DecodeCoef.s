@// AAC-LC tns_decode_coef
@// author: reed.zhang
@// input: SBR synthesis filter bank processing
@//     r0: output sample pointer
@// r1: the input overlap
@// r2: the tablr for output data
@// r3: the table for overlapping  
@// purpose:

@// define the register name 
a_ptr             .req      r0
tm_buf_ptr        .req      r1
order0             .req     r2
m                 .req      r3
t_b_ptr           .req      r4
b_ptr             .req      r5
m_bak             .req      r6

data_0            .req      r7
data_1            .req      r10

t_tm_buf_ptr      .req      r8
t_a_ptr           .req      r9

m_l               .req      r12
m_h               .req      r11

t_a_ptr2          .req      r14
                .text @AREA    PROGRAM, CODE, READONLY                         
                .arm@CODE32                  
                @//AACTnsTnsDecodeCoefAsm(
                @//                       int32_t   *a_ptr
                @//                       int32_t   *tm_buf_ptr
                @//                       int8_t     order)@   
                .global  AACTnsTnsDecodeCoefAsm

AACTnsTnsDecodeCoefAsm:
                @ // save the value for the following calculation
               stmfd   sp!, {r4 - r12, r14}    
               @@/////////////////
               add     b_ptr,   tm_buf_ptr,  #84
               
               ldr     data_0, =268435456               
               str     data_0,    [a_ptr], #4
               ldr     data_0,       [tm_buf_ptr]
               mov     m_bak,        #1
               str     data_0,       [a_ptr], #-4
               mov     t_a_ptr,      a_ptr
               
ORDER_LOOP0:
               mov    m,   m_bak
               add    t_tm_buf_ptr, tm_buf_ptr, m,  lsl  #2
               add    t_a_ptr,      a_ptr,        m,  lsl  #2
               ldr   data_0,   [t_tm_buf_ptr]
               
               add    t_a_ptr2,    a_ptr,  #4
               add    t_b_ptr,     b_ptr,  #4
M_LOOP0:               
               ldr   data_1,   [t_a_ptr], #-4
               mov    m_l,     #0
               mov    m_h,     #0
               SMLAL     m_l,  m_h,    data_0,  data_1           
               mov      m_l,  m_l,  lsr #28               
               mov      m_h,  m_h,  lsl  #4
               orr      m_l,  m_h,   m_l
               
               ldr      m_h,  [t_a_ptr2],  #4
               add      m_l,   m_l,   m_h
               str      m_l,  [t_b_ptr], #4              
                        
               @@ //control
               subs   m,   m,  #1               
               BGT   M_LOOP0
               
               @@//
               add    t_a_ptr2,    a_ptr,  #4
               add    t_b_ptr,     b_ptr,  #4
               mov    m,   m_bak
M_LOOP1:               
               ldr    data_1,   [t_b_ptr], #4
               str    data_1,   [t_a_ptr2], #4   
               
               @@ //control
               subs   m,   m,  #1               
               BGT   M_LOOP1
               @@//
               
               str   data_0,   [t_a_ptr2]
               
               @@//loop control
               @@ //control
               add   m_bak,   m_bak,  #1
               subs  order0, order0, #1
               BGT   ORDER_LOOP0
               @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@                       
               ldmfd   sp!, {r4 - r12, pc}                                
               .end@END