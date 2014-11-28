;/*sub for rsa*/
;/*int sub(unsigned int *pt, unsigned int * ps)*/
;// *pt -= *ps; andput result on pt-128, length of pt is 33 dword
;// the carry of last sub is in eax
;// return: 1; if *pt > *ps

input_pt	.req		r0
output	    	.req		r0

input_ps	.req		r1

data_pt	    	.req		r2
data_ps     	.req		r3

loop_index  	.req		r4

data_pt1	.req		r5
data_ps1    	.req		r6

data_pt2	.req		r7
data_ps2    	.req		r8

data_pt3	.req		r9
data_ps3    	.req		r12

;/*int sub(unsigned int *pt, unsigned int * ps)*/
		;//AREA PROGRAM, CODE, READONLY
        .text  @AREA    |.text|, CODE, READONLY
                ;//CODE32
        .arm@CODE32
	
		;//EXPORT	rsa_sub
	.global	rsa_sub
	
rsa_sub:
		stmfd	sp!, {r4 - r9, r11, r12, r14}
		
		;//init for loop8
		mov		loop_index, #8
		
		;//clear clc for sub
		msr     CPSR_f, #0x20000000
		
loop8:
        ;//load data
        ;//ldr     data_pt,[input_pt],#4
        ldmia   input_pt!,{data_pt,data_pt1,data_pt2,data_pt3}
        
        ;//ldr     data_ps,[input_ps],#4
        ldmia   input_ps!,{data_ps,data_ps1,data_ps2,data_ps3}
        
        sub     input_pt,input_pt,#144

        sbcs    data_pt,data_pt,data_ps
        sbcs    data_pt1,data_pt1,data_ps1
        sbcs    data_pt2,data_pt2,data_ps2
        sbcs    data_pt3,data_pt3,data_ps3
        
        ;//str     data_pt,[input_pt,#-132]
        stmia   input_pt,{data_pt,data_pt1,data_pt2,data_pt3}
        add     input_pt,input_pt,#144
        
 		sub	    loop_index,loop_index, #1
		teq     loop_index,#0
		bne		loop8
		
		;//if *pt > *ps; return: 1; 
        ldr     data_pt,[input_pt]
        sbc     output,data_pt,#0
        
		ldmfd	sp!, {r4 - r9, r11, r12, pc}
        
	.end@END
