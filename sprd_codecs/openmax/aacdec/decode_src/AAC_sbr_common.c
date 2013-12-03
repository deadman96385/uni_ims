/*************************************************************************
** File Name:      sbr_common.h                                          *
** Author:         Reed zhang                                            *
** Date:           24/01/2006                                            *
** Copyright:      2001 Spreatrum, Incoporated. All Rights Reserved.     *
** Description:    This file defines the common function and variation   *
**                 for SBR dec.                                          *
**                        Edit History                                   *
** ----------------------------------------------------------------------*
** DATE           NAME             DESCRIPTION                           *
** 24/01/2006     Reed zhang       Create.                               *
**************************************************************************/
#include "AAC_sbr_common.h"
/************************************************************************/
/* this function is used to implement fixed-point division              */
/* the return value is the result by division,                          */
/************************************************************************/
extern const int16 pow2_tab[];
extern const int16 log2_tab[];

//////////////////////////////////////////////////////////////////////////
/* perform the square operation a^0.5 */
int32 AAC_DEC_Sqrt(int32 a)
{
    int32 x, y;
    if (0 == a)
    {
        return 0;
    }
    x = AAC_DEC_ARM_Log2IntAsm(a, (int16*)log2_tab);
    y = AAC_DEC_ARM_Pow2IntAsm(x/2 + 7 * (1 << 14), (int16*)pow2_tab);

    
    return y;
}

/* perform the normalize operation (1 << nbit)/b */
int32 AAC_DEC_Normalize(int16 nbit, int32 b)
{
    int32 x, y;
    if (0 >= nbit)
    {
        return 0;
    }
    if (0 == b)
    {
        return 0;
    }    
    y = AAC_DEC_ARM_Log2IntAsm(b, (int16*)log2_tab);
    x = ((int32)nbit << 14) - y;
    y = AAC_DEC_ARM_Pow2IntAsm(x, (int16*)pow2_tab);
    return y;
}

int32 AAC_DEC_MultiplyShiftR32(int32 x, int32 y)
{
/* rules for smull RdLo, RdHi, Rm, Rs:
*   RdHi != Rm
*   RdLo != Rm
*   RdHi != RdLo
	*/
    int32 zlow;
/*@jgdu
    __asm {
        smull zlow,y,x,y
	}""
*/	
    __asm__("smull %0,%1,%2,%3":"=&r"(zlow),"=r"(y):"r"(x),"1"(y):"cc");
    return y;
}