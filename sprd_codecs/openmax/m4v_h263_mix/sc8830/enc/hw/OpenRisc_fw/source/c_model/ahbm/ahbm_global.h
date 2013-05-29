/******************************************************************************
 ** File Name:      ahbm_global.h	                                          *
 ** Author:         Xiaowei Luo                                               *
 ** DATE:           11/20/2007                                                *
 ** Copyright:      2007 Spreatrum, Incoporated. All Rights Reserved.         *
 ** Description:    VSP bsm Driver for video codec.	  						  *
 *****************************************************************************/
/******************************************************************************
 **                   Edit    History                                         *
 **---------------------------------------------------------------------------* 
 ** DATE          NAME            DESCRIPTION                                 * 
 ** 11/20/2007    Xiaowei Luo     Create.                                     *
 *****************************************************************************/
#ifndef _AHBM_GLOBAL_H_
#define _AHBM_GLOBAL_H_

/*----------------------------------------------------------------------------*
**                        Dependencies                                        *
**---------------------------------------------------------------------------*/
#include "video_common.h"
//#include "mp4dec_mode.h"
//#include "vsp_global_defines.h"
/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    extern   "C" 
    {
#endif

extern VSP_AHBM_REG_T * g_ahbm_reg_ptr;

/**----------------------------------------------------------------------------*
**                           Function Prototype                               **
**----------------------------------------------------------------------------*/
PUBLIC  void VSP_InitAhbmCtrlRegAddr(void);
PUBLIC  void VSP_DelAhbmCtrlRegAddr(void);
PUBLIC void VSP_Write_Ahbm_Reg(uint32 RegAddrOffset, int32 value, char *pstring);
PUBLIC int32 VSP_Read_Ahbm_Reg(uint32 RegAddrOffset, char *pstring);

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End 
#endif  //_AHBM_GLOBAL_H_