/***************************************************************************************
** File Name:      dcam_module_common.c                                    			   *
** DATE:           12/06/2008                                                		   *
** Copyright:      2008 Spreatrum, Incoporated. All Rights Reserved.         		   *
** Description:                                                                        *
****************************************************************************************

****************************************************************************************
**                         Edit History                                       		   *
**-------------------------------------------------------------------------------------*
** DATE                    DESCRIPTION                               				   *
** 12/06/2008     	       Create.													   *
****************************************************************************************/


/**------------------------------------------------------------------------------------*
 **                         Dependencies           			                           *
 **------------------------------------------------------------------------------------*/ 
#include "sci_types.h"
#include "sc6600l_dcam_common.h" 
#include "sc6600l_isp_reg.h"
 
 
 /**------------------------------------------------------------------------------------*
 **                         Compiler Flag                               		       *
 **------------------------------------------------------------------------------------*/
#ifdef   __cplusplus
    extern   "C" 
    {
#endif

typedef void    *SCI_SEMAPHORE_PTR;

static DCAM_MODULE_IRQ_FUNC s_dcam_irq[DCAM_IRQ_NUM];

static SCI_SEMAPHORE_PTR s_dcam_sm = NULL;

//test being
#define TEST_MODE   1
//test end

/****************************************************************************************/
// Description: dcam module irq handle
// Global resource dependence: 
// Author: Jianping.wang
// Note:
/*****************************************************************************************/
void DCAMMODULE_Irq(void)
{
	int32 i = 0;
	DCAM_INT_STS_U  *int_sts_ptr = (DCAM_INT_STS_U*)DCAM_INT_STS;
	DCAM_INT_CLR_U  *int_clr_ptr = (DCAM_INT_CLR_U*)DCAM_INT_CLR;
	
	uint32 irq_line,irq_state;

	irq_line = DCAM_INT_MASK_VALUE & int_sts_ptr->dwValue;
	irq_state = irq_line;
	
	int_clr_ptr->dwValue |= (DCAM_INT_MASK_VALUE & irq_state);
	
	//SCI_TraceLow ("DCAMMODULE_Irq irq_state is 0x%x \r",irq_state); 
	
	for(i=0 ; i<DCAM_IRQ_NUM ; i++)
	{
		if(irq_line & 1)
		{
			if(s_dcam_irq[i])
			{
				s_dcam_irq[i]();
			}
		}
		irq_line >>= 1;
		
		if(!irq_line)
		{
			break;
		}
	}
}
/****************************************************************************************/
// Description: register dcam module irq handle func
// Global resource dependence: 
// Author: Jianping.wang
// Note:
/*****************************************************************************************/
int DCAMMODULE_RegisterIntFunc(DCAM_MODULE_INT_ID_E irq_num, DCAM_MODULE_IRQ_FUNC func)
{
#ifndef TEST_MODE
	SCI_ASSERT( PNULL != func );
	SCI_ASSERT( DCAM_IRQ_NUM > irq_num);
#endif

	s_dcam_irq[irq_num] = func;
	
	return 0;
}
/****************************************************************************************/
// Description: open interrupt
// Global resource dependence: 
// Author: Jianping.wang
// Note:
/*****************************************************************************************/
void DCAMMODULE_OpenInt(DCAM_MODULE_INT_ID_E int_id)
{
	DCAM_INT_MASK_U  *reg_ptr = (DCAM_INT_MASK_U*)DCAM_INT_MASK;
#ifndef TEST_MODE	
	SCI_ASSERT(int_id<ISP_INT_ID_MAX);
#endif
	
	reg_ptr->dwValue |= (1<<int_id);	
}
/****************************************************************************************/
// Description: close interrupt
// Global resource dependence: 
// Author: Jianping.wang
// Note:
/*****************************************************************************************/
void DCAMMODULE_CloseInt(DCAM_MODULE_INT_ID_E int_id)
{
	DCAM_INT_CLR_U  *reg_ptr = (DCAM_INT_CLR_U*)DCAM_INT_CLR;
#ifndef TEST_MODE	
	SCI_ASSERT(int_id<ISP_INT_ID_MAX);
#endif
	
	reg_ptr->dwValue &= ~(1<<int_id);
}
/****************************************************************************************/
// Description: clear interrupt
// Global resource dependence: 
// Author: Tim.zhu
// Note:
/*****************************************************************************************/
void DCAMMODULE_ClearInt(DCAM_MODULE_INT_ID_E int_id)
{
	DCAM_INT_MASK_U  *reg_ptr = (DCAM_INT_MASK_U*)DCAM_INT_MASK;
#ifndef TEST_MODE	
	SCI_ASSERT(int_id<ISP_INT_ID_MAX);
#endif
	
	reg_ptr->dwValue |= (1<<int_id);
}
/****************************************************************************************/
// Description: Initialize DCAM module common
// Global resource dependence: 
// Author: Jianping.wang
// Note:
/*****************************************************************************************/
void DCAMMODULE_Init(void)
{
	int32 i = 0;
#ifndef TEST_MODE	
	if(s_dcam_sm == NULL)	
	    s_dcam_sm = SCI_CreateSemaphore("DCAM MODULE SEMAPHORE", 1);
#endif	    
	    
	for( i=0 ; i<DCAM_IRQ_NUM ; i++)
	{
		s_dcam_irq[i] = NULL;
	}
	
	//Register irq to OS .
//    DRV_RegHandler(?, DCAMMODULE_Irq);
    
	//enable irq interface
	//TB_REG_OR(SC6800_IRQ_ENABLE_ADDR,(1<<SC6800_LCDC_IRQ_BIT));????
}
/*****************************************************************************/
//  Description:dcam Get Semaphore
//	Global resource dependence:
//  Author: Jianping.wang
//	Note:
//		input:
//			sm    -  semaphore
//		output:
//			none
//		return:
//			none
/*****************************************************************************/
PUBLIC void DCAMMODULE_GetSemaphore(void)
{
#ifndef TEST_MODE
	SCI_ASSERT( PNULL != s_dcam_sm );
	
	if(SCI_InThreadContext())
	{
		if( SCI_ERROR == SCI_GetSemaphore(s_dcam_sm, SCI_WAIT_FOREVER) )
		{
			SCI_TRACE_LOW("DCAM:Get Semaphore Fail!");
		}
		SCI_TRACE_LOW("---DCAM Get Semaphor OK!-----");
	}
	else
	{
		SCI_TRACE_LOW("---DCAM Get Semaphor None-----");
	}
#endif
}//end of DCAMMODULE_GetSemaphore
/*****************************************************************************/
//  Description:dcam Put Semaphor
//	Global resource dependence:
//  Author: Jianping.wang
//	Note:
//		input:
//			sm    -  semaphore
//		output:
//			none
//		return:
//			none
/*****************************************************************************/
PUBLIC void DCAMMODULE_PutSemaphore(void)
{
#ifndef TEST_MODE	
	SCI_ASSERT( PNULL != s_dcam_sm );	
	
	if(SCI_InThreadContext())
	{
		uint32 ret;

		ret = SCI_PutSemaphore( s_dcam_sm );	
	    SCI_ASSERT( ret == SCI_SUCCESS );
	    SCI_TRACE_LOW("---DCAM Put Semaphor OK!-----");
	}
	else
	{
		SCI_TRACE_LOW("---DCAM Put Semaphor None-----");
	}
#endif	
}//end of DCAMMODULE_PutSemaphore



/**---------------------------------------------------------------------------------------------*
 **                         Compiler Flag                             					        *
 **---------------------------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif


//end of dcam_module_common.c
