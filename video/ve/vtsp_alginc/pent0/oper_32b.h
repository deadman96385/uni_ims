/* Double precision operations */
/* $Id$ */

#include <typedefs.h>

inline void L_Extract (Word32 L_32, Word16 *hi, Word16 *lo);
inline Word32 L_Comp (Word16 hi, Word16 lo);
inline Word32 Mpy_32 (Word16 hi1, Word16 lo1, Word16 hi2, Word16 lo2);
inline Word32 Mpy_32_16 (Word16 hi, Word16 lo, Word16 n);
inline Word32 Div_32 (Word32 L_num, Word16 denom_hi, Word16 denom_lo);
