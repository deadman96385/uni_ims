/********************************************************************************
**  File Name: 	mp3_fixed.h										                *
**  Author:		Tan Li      									                *
**  Date:		17/01/2011                                                      *
**  Copyright:	2011 Spreadtrum, Incorporated. All Rights Reserved.		        *
**  Description:  The compiling macro and fixed-point macro definition 	        *
*********************************************************************************
*********************************************************************************
**  Edit History											                    *
**------------------------------------------------------------------------------*
**  DATE			NAME			DESCRIPTION				                    *
**  17/01/2011		Tan li    		Create. 				                    *
*********************************************************************************/


# ifndef LIBMP3_FIXED_H
# define LIBMP3_FIXED_H

#define __ARM_PLATFORM__
//#define INTERLEAVE_PCM

#ifdef WIN32
#undef __ARM_PLATFORM__
#endif

#ifdef __ARM_PLATFORM__
#define  __ASM_OPT__
#ifdef __ASO__
#define ASO_SYNTHFULL
#define ASO_DCT32
#define ASO_ALIAS
#define ASO_DCTIV
#define ASO_WIN_OVERLAP
#define ASO_HUFFMAN
#define ASO_IQ
#define ASO_IMDCT_WIN
#define ASO_SYNTHFILTER
#endif
#endif

#include "t_types.h"




typedef   signed int mp3_fixed_t;




typedef mp3_fixed_t mp3_sample_t;

# ifndef inline
# define inline __inline
# endif

#ifndef NULL
#define NULL    ((void *)0)
#endif

#ifndef PNULL
#define PNULL    ((void *)0)
#endif
/*
 * Fixed-point format: 0xABBBBBBB
 * A == whole part      (sign + 3 bits)
 * B == fractional part (28 bits)
 *
 * Values are signed two's complement, so the effective range is:
 * 0x80000000 to 0x7fffffff
 *       -8.0 to +7.9999999962747097015380859375
 *
 * The smallest representable value is:
 * 0x00000001 == 0.0000000037252902984619140625 (i.e. about 3.725e-9)
 *
 * 28 bits of fractional accuracy represent about
 * 8.6 digits of decimal accuracy.
 *
 * Fixed-point numbers can be added or subtracted as normal
 * integers, but multiplication requires shifting the 64-bit result
 * from 56 fractional bits back to 28 (and rounding.)
 *
 * Changing the definition of MP3_F_FRACBITS is only partially
 * supported, and must be done with care.
 */

# define MP3_F_FRACBITS		28

#  define MP3_F(x)		((mp3_fixed_t) (x##L))


# define MP3_F_ONE		MP3_F(0x10000000)

















#define mp3_f_mul   MP3_DecMulLongAsm

#if defined(__ASM_OPT__)
/*
 * This ARM V4 version is as accurate as FPM_64BIT but much faster. The
 * least significant bit is properly rounded at no CPU cycle cost!
 */
 static  mp3_fixed_t mp3_f_mul_fixed16_inline(mp3_fixed_t x, mp3_fixed_t y)
 {	 
	 register int lo; 	 
#if 0
	 __asm 
	 { 
       		smulwb	lo,x,y						
	 }
#else
        __asm__("smulwb %0, %1, %2" : "=&r"(lo) : "r"(x), "r"(y):"cc" );
#endif
	 return lo;	 
 }

 static  mp3_fixed_t mp3_f_mla_fixed16_inline(mp3_fixed_t x, mp3_fixed_t y, mp3_fixed_t z)
 {	 
	 register int lo; 	 
#if 0
	 __asm 
	 { 
       		smlawb	lo,x,y,z						
	 }
#else
        __asm__("smlawb  %0,%1,%2,%3":"+&r"(lo),"+r"(x):"r"(y),"r"(z):"cc");
#endif
	 return lo;	 
 }
  

 static  mp3_fixed_t mp3_f_mul_fixed16_t_inline(mp3_fixed_t x, mp3_fixed_t y)
 {	 
	 register int lo; 	 
#if 0
	 __asm 
	 { 
       		smulwt	lo,x,y						
	 }
#else
        __asm__("smulwt %0, %1, %2" : "=&r"(lo) : "r"(x), "r"(y):"cc" );
#endif
	 return lo;	 
 }

 static  mp3_fixed_t mp3_f_mla_fixed16_t_inline(mp3_fixed_t x, mp3_fixed_t y, mp3_fixed_t z)
 {	 
	 register int lo; 	 
#if 0
	 __asm 
	 { 
       		smlawt	lo,x,y, z						
	 }
#else
        __asm__("smlawt  %0,%1,%2,%3":"+&r"(lo),"+r"(x):"r"(y),"r"(z):"cc");
#endif
	 return lo;	 
 }






#endif









#define BSWAP(a) \
 ((a) = ( ((a)&0xff)<<24) | (((a)&0xff00)<<8) | (((a)>>8)&0xff00) | (((a)>>24)&0xff))
#endif


















































































































































