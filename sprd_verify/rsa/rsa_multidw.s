;/*multiplication for rsa*/
;/*void rsa_multidw(unsigned int * pt, unsigned int *ps, unsigned int m)*/
;//input: r0: [output] unsigned int *pt: pointer to unsigned int t[65].
;//		  r1: [input] unsigned int *ps:  pointer to unsigned int s[32].
;//		  r2: [input] unsigned int m:    pointer to unsigned int m.
;//function: // *pt += *ps * m.
;//t[65] adds the multiplication result of 32 bit integer s and 1024bit long integer s.
;//return: none

;//RN-->.req, num-->rnum

input_pt	.req		r0
input_ps	.req		r1
input_m	        .req		r2

uml_rm	        .req		r2

ps	        .req		r3
pt       	.req		r4
loop_index	.req		r5
uml_low 	.req		r6
uml_high	.req		r7
uml_rs	    	.req		r8

data_tmp	.req		r9

data_tmp1	.req		r11
loop_index1	.req		r12

;/*void rsa_multidw(unsigned int * pt, unsigned int *ps, unsigned int m)*/
		;//AREA	PROGRAM, CODE, READONLY
	.text  @AREA    |.text|, CODE, READONLY
		;//CODE32
	.arm@CODE32
		
		;//EXPORT	rsa_multidw
	.global	rsa_multidw
		
rsa_multidw:
		stmfd	sp!, {r4 - r9, r11, r12, r14}
		
		;//init for loop1
		mov		loop_index,  #16
		mov		loop_index1, #2
		mov     ps,input_ps
		mov     pt,input_pt
		
		;//clear clc
		msr     CPSR_f, #0
		
loop16:
        ;//do 64bit multiplication
        ldr     uml_rs,[ps],#8
        ldmia   pt,{data_tmp,data_tmp1}
        umull   uml_low,uml_high,uml_rm,uml_rs
        
        ;//add uml_high to pt and pt+4
        adcs    data_tmp,data_tmp,uml_low
        adcs    data_tmp1,data_tmp1,uml_high
        stmia   pt!,{data_tmp,data_tmp1}
        
        ;//sub	    loop_index
        sub	    loop_index, loop_index, #1
		teq     loop_index, #0
		bne		loop16
		
		;//add the carry bit to pt after loop16
		ldr     data_tmp,[pt]
		
		;//init for another loop
		mov		loop_index, #16
        
        adcs    data_tmp,data_tmp,#0
        str     data_tmp,[pt], #-124
        
        sub     ps, ps, #124
		
		;//clear clc
		;//msr     CPSR_f, #0
		
        sub	    loop_index1, loop_index1, #1
        teq     loop_index1, #0
		bne		loop16
		
		ldmfd	sp!, {r4 - r9, r11, r12, pc}
        
	.end@END
