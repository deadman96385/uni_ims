/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2006-2012 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * +D2Tech+ Rev: 3238  Date: 2008-10-30 12:15:07 -0700 (Thu, 30 Oct 2008) 
 * +D2Tech+ Release Version: PENT0_A_1_1-basicop_1_3
 */
/*___________________________________________________________________________
 |                                                                           |
 |   Constants and Globals                                                   |
 |                                                                           |
 | $Id $
 |___________________________________________________________________________|
*/

#include <typedefs.h>

extern Flag Overflow;
extern Flag Carry;

#define MAX_32          (Word32)0x7fffffffL
#define MIN_32          (Word32)0x80000000L
#define MAX_16 (Word16)+32767    /* 0x7fff */
#define MIN_16 (Word16)-32768    /* 0x8000 */

#define GAMRNB_ROUND(x) ((EXTRACT_H((ADD((int32)(x),(int32)0x00008000L)))))
#define GAMRWB_ROUND(x) ((EXTRACT_H((ADD((Word32)(x),(Word32)0x00008000L)))))

#define L_ABS(x)        (((x) == MIN_32) ? MAX_32 : ((x) < 0 ? -(x) : (x)))
#define ABS_S(x)        (((x) == MIN_16) ? MAX_16 : ((x) < 0 ? -(x) : (x)))

#define L_SHR(x, y)     (((y) < 0) ? (((y) < -32) ? -32 : L_shl((x),-(y))) : (((y) > 31) ? (((x) < 0L) ? -1 : 0) : (((x) < 0) ? (~((~(x)) >> (y))) : ((x) >> (y)))))

#define L_SHL(x, y)     (((y) < 0) ? (SHR((x),(-y))) : SHL((x),(y)))
#define SHR_R(x, y)     (((y) > 15) ? 0 : (((y) > 0) ? ((((x) & ((Word16) 1 << ((y) - 1))) != 0) ? (SHR((x),(y)) + 1) : (SHR((x),(y)))) : (SHL((x),(y))) )) //
#define SHR_R2(x, y)    ( (((x) & ((Word16) 1 << ((y) - 1))) != 0) ? (SHR((x),(y)) + 1) : (SHR((x),(y))) )

#define L_MULT(x, y)    (((x) * (y)) << 1) 
#define SUB(x, y)       ((x) - (y)) 
#define ADD(x, y)       ((x) + (y)) 
#define MULT(x, y)      (((x) * (y)) >> 15)
#define SHL(x, y)       ((x) << (y)) 
#define SHR(x, y)       ((x) >> (y)) 
#define EXTRACT_H(x)    ((Word16)((x) >> 16))
#define EXTRACT_L(x)    ((Word16)(x))
#define L_DEPOSIT_L(x)  ((Word32)(x))
#define L_DEPOSIT_H(x)  (((Word32)(x)) << 16)
#define NEGATE(x)       (((x) == MIN_16) ? MAX_16 : -(x))
#define L_NEGATE(x)     (((x) == MIN_32) ? MAX_32 : -(x))
#define MULT_R(x ,y)    (((((Word32)(x)) * (y)) + 0x4000) >> 15)

#define L_EXTRACT(L_var, hi, lo) {*hi = (Word16)(L_var >> 16); \
                                *lo = (Word16)(((Word16)(L_var >> 1)) & 0x7fff);}

#define L_COMP(hi, lo)  (L_MAC((L_DEPOSIT_H (hi)), (lo), 1))

#define L_MSU(z, x, y)  ((z) - L_MULT((x), (y)))
#define MSU(z, x, y)    ((z) - ((x) * (y)))                                   
#define L_MAC(z, x, y)  (L_MULT((x), (y)) + (z))
#define MSU_R(z, x, y)  (GAMRNB_ROUND(L_MULT((x), (y)) - (z)))
#define MAC(z, x, y)    (((x) * (y)) + (z))                                   
#define MAC_R(z, x, y)  (GAMRNB_ROUND(L_MULT((x), (y)) + (z)))
/*___________________________________________________________________________
 |                                                                           |
 |   Prototypes for basic arithmetic operators                               |
 |___________________________________________________________________________|
*/

Word16 add (Word16 var1, Word16 var2);    /* Short add,           1   */
Word16 sub (Word16 var1, Word16 var2);    /* Short sub,           1   */
Word16 abs_s (Word16 var1);               /* Short abs,           1   */
Word16 shl (Word16 var1, Word16 var2);    /* Short shift left,    1   */
Word16 shr (Word16 var1, Word16 var2);    /* Short shift right,   1   */
Word16 mult (Word16 var1, Word16 var2);   /* Short mult,          1   */
Word32 L_mult (Word16 var1, Word16 var2); /* Long mult,           1   */
Word16 negate (Word16 var1);              /* Short negate,        1   */
Word16 extract_h (Word32 L_var1);         /* Extract high,        1   */
Word16 extract_l (Word32 L_var1);         /* Extract low,         1   */
Word16 GAMRNB_round (Word32 L_var1);             /* Round,               1   */
inline Word16 gamrwb_round (Word32 L_var1);      /* Round,               1   */
Word32 L_mac (Word32 L_var3, Word16 var1, Word16 var2);   /* Mac,  1  */
Word32 L_msu (Word32 L_var3, Word16 var1, Word16 var2);   /* Msu,  1  */
Word32 L_macNs (Word32 L_var3, Word16 var1, Word16 var2); /* Mac without
                                                             sat, 1   */
Word32 L_msuNs (Word32 L_var3, Word16 var1, Word16 var2); /* Msu without
                                                             sat, 1   */
Word32 L_add (Word32 L_var1, Word32 L_var2);    /* Long add,        2 */
Word32 L_sub (Word32 L_var1, Word32 L_var2);    /* Long sub,        2 */
Word32 L_add_c (Word32 L_var1, Word32 L_var2);  /* Long add with c, 2 */
Word32 L_sub_c (Word32 L_var1, Word32 L_var2);  /* Long sub with c, 2 */
Word32 L_negate (Word32 L_var1);                /* Long negate,     2 */
Word16 mult_r (Word16 var1, Word16 var2);       /* Mult with round, 2 */
Word32 L_shl (Word32 L_var1, Word16 var2);      /* Fast Long shift left */
Word32 L_shl2 (Word32 L_var1, Word16 var2);     /* Long shift left, 2 */
Word32 L_shr (Word32 L_var1, Word16 var2);      /* Long shift right, 2*/
Word16 shr_r (Word16 var1, Word16 var2);        /* Shift right with
                                                   round, 2           */
Word16 mac_r (Word32 L_var3, Word16 var1, Word16 var2); /* Mac with
                                                           rounding,2 */
Word16 msu_r (Word32 L_var3, Word16 var1, Word16 var2); /* Msu with
                                                           rounding,2 */
Word32 L_deposit_h (Word16 var1);        /* 16 bit var1 -> MSB,     2 */
Word32 L_deposit_l (Word16 var1);        /* 16 bit var1 -> LSB,     2 */

Word32 L_shr_r (Word32 L_var1, Word16 var2); /* Long shift right with
                                                round,  3             */
Word32 L_abs (Word32 L_var1);            /* Long abs,              3  */
Word32 L_sat (Word32 L_var1);            /* Long saturation,       4  */
Word16 norm_s (Word16 var1);             /* Short norm,           15  */
Word16 div_s (Word16 var1, Word16 var2); /* Short division,       18  */
Word16 norm_l (Word32 L_var1);           /* Long norm,            30  */   

