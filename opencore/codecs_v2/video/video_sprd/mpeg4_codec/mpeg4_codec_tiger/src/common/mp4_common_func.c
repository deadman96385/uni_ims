/******************************************************************************
 ** File Name:    mp4_common_func.c                                           *
 ** Author:       Xiaowei Luo                                                 *
 ** DATE:         10/13/2009                                                  *
 ** Copyright:    2009 Spreatrum, Incoporated. All Rights Reserved.           *
 ** Description:                                                              *
 *****************************************************************************/
/******************************************************************************
 **                   Edit    History                                         *
 **---------------------------------------------------------------------------* 
 ** DATE          NAME            DESCRIPTION                                 * 
 ** 10/13/2009    Xiaowei Luo     Create.                                     *
 *****************************************************************************/
/*----------------------------------------------------------------------------*
**                        Dependencies                                        *
**---------------------------------------------------------------------------*/
#include "tiger_video_header.h"
/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    extern   "C" 
    {
#endif

#define PIX_SORT(_a, _b) {if (_a > _b) PIX_SWAP(_a, _b);}
#define PIX_SWAP(_a, _b) {int32 temp = _a; _a = _b; _b = temp;}

PUBLIC int32 Mp4_GetMedianofThree(int32 a0, int32 a1, int32 a2)
{
	PIX_SORT(a0,a1);
	PIX_SORT(a1,a2);
	PIX_SORT(a0,a1);

	return a1;
}

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End 
