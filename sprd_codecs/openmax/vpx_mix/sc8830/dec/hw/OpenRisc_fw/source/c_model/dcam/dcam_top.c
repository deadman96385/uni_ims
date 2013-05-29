/******************************************************************************
 ** File Name:    dcam_top.c		                                          *
 ** Author:       Xiaowei Luo                                                 *
 ** DATE:         11/19/2008                                                  *
 ** Copyright:    2006 Spreatrum, Incoporated. All Rights Reserved.           *
 ** Description:   c model of bsmr module in mpeg4 decoder                    *
 *****************************************************************************/
/******************************************************************************
 **                   Edit    History                                         *
 **---------------------------------------------------------------------------* 
 ** DATE          NAME            DESCRIPTION                                 * 
 ** 12/14/2006    Xiaowei Luo     Create.                                     *
 *****************************************************************************/
/*----------------------------------------------------------------------------*
**                        Dependencies                                        *
**---------------------------------------------------------------------------*/
#include "sc8810_video_header.h"
/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    extern   "C" 
    {
#endif


/*****************************************************************************
 **	Name : 			VSP_InitDcamCtrlRegAddr
 ** Description:	Initial dcam ctrl register address.
 ** Author:			Binggo Zhou
 **	Note:			
 *****************************************************************************/
PUBLIC  void VSP_InitDcamCtrlRegAddr(void)
{
	g_dcam_reg_ptr = (VSP_DCAM_REG_T *)malloc(sizeof(VSP_DCAM_REG_T));
	memset(g_dcam_reg_ptr, 0, sizeof(VSP_DCAM_REG_T));
}

/*****************************************************************************
 **	Name : 			VSP_DelDcamCtrlRegAddr
 ** Description:	Delete dcam ctrl register address.
 ** Author:			Binggo Zhou
 **	Note:			
 *****************************************************************************/
PUBLIC  void VSP_DelDcamCtrlRegAddr(void)
{
	free(g_dcam_reg_ptr); g_dcam_reg_ptr = PNULL;
}

/*****************************************************************************
 **	Name : 			VSP_Write_Dcam_Reg
 ** Description:	write register.
 ** Author:			Xiaowei Luo
 **	Note:			
 *****************************************************************************/
PUBLIC void VSP_Write_Dcam_Reg(uint32 RegAddrOffset, int32 value, char *pstring)
{
	int32 *pAddr;

	pAddr = (int32 *)(((uint8 *)g_dcam_reg_ptr) + RegAddrOffset);
	*pAddr = value;

#if _FW_TEST_VECTOR_
	OUTPUT_FW_CMD_VECTOR(1, VSP_DCAM_REG_BASE + RegAddrOffset, value, pstring);
#endif //_FW_TEST_VECTOR_
}

/*****************************************************************************
 **	Name : 			VSP_Read_Dcam_Reg
 ** Description:	read register.
 ** Author:			Xiaowei Luo
 **	Note:			
 *****************************************************************************/
PUBLIC int32 VSP_Read_Dcam_Reg(uint32 RegAddrOffset, char *pstring)
{
	int32 value;
	int32 *pAddr;

	pAddr = (int32 *)(((uint8 *)g_dcam_reg_ptr) + RegAddrOffset);
	
	value = *pAddr;
	
#if _FW_TEST_VECTOR_
	OUTPUT_FW_CMD_VECTOR(0, VSP_DCAM_REG_BASE + RegAddrOffset, value, pstring);
#endif //_FW_TEST_VECTOR_

	return value;
}
/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End 

