/******************************************************************************
 ** File Name:    sc6600l_dcam_common.h                                       *
 ** Author:       Jianping.Wang                                               *
 ** DATE:         12/06/2008                                                  *
 ** Copyright:    2006 Spreatrum, Incoporated. All Rights Reserved.           *
 ** Description:                                                              *
 ******************************************************************************/
/******************************************************************************
 **                   Edit    History                                         *
 **---------------------------------------------------------------------------* 
 ** DATE          NAME            DESCRIPTION                                 * 
 ** 12/06/2008    Jianping.Wang   Create.                                     *
 ******************************************************************************/
#ifndef _SC6600L_DCAM_COMMON_H_
#define _SC6600L_DCAM_COMMON_H_
/*----------------------------------------------------------------------------*
 **                          Dependencies                                     *
 **---------------------------------------------------------------------------*/
 //#include "os_api.h"

/**---------------------------------------------------------------------------*
 **                          Compiler Flag                                    *
 **---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    extern   "C" 
    {
#endif
/**---------------------------------------------------------------------------*
**                               Micro Define                                **
**----------------------------------------------------------------------------*/
#define DCAM_IRQ_NUM        16
/**---------------------------------------------------------------------------*
**                               Data Prototype                              **
**----------------------------------------------------------------------------*/

//interrupt handle function declare
typedef void (*DCAM_MODULE_IRQ_FUNC)(void);




typedef enum
{
	SENSOR_SOF_ID = 0,
	SENSOR_EOF_ID,
	CAP_SOF_ID,
	CAP_EOF_ID,
	ISP_TX_DONE_ID,
	SC_DONE_ID,
	ISP_BUFFER_OVF_ID,
	VSP_BSM_DONE_ID,
	VSP_VLC_DONE_ID,
	VSP_MBIO_DOEN_ID,
	VSP_CMD_TIME_OUT_ID,
	JPEG_BUF_OVF_ID,	
	VSP_TIMEOUT_ID,
	VSP_VLD_ERROR_ID,
	VSP_MEA_DONE_ID,
	VSP_CMD_DONE_ID,
	ISP_INT_ID_MAX
}DCAM_MODULE_INT_ID_E;



//typedef enum
//{
//	SYS_STATE_ASSERT = 0,		    // Assert state
//	SYS_STATE_NORMAL    ,		    // Normal Run 
//	SYS_STATE_INT       		    // Interrupt state 
//}SYS_STATE_E;
/****************************************************************************************/
// Description: Initialize DCAM module common
// Global resource dependence: 
// Author: Jianping.wang
// Note:
/*****************************************************************************************/
void DCAMMODULE_Init(void);
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
PUBLIC void DCAMMODULE_GetSemaphore(void);
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
PUBLIC void DCAMMODULE_PutSemaphore(void);
/****************************************************************************************/
// Description: register dcam module irq handle func
// Global resource dependence: 
// Author: Jianping.wang
// Note:
/*****************************************************************************************/
int DCAMMODULE_RegisterIntFunc(DCAM_MODULE_INT_ID_E irq_num, DCAM_MODULE_IRQ_FUNC func);


/**----------------------------------------------------------------------------*
**                         Compiler Flag                                      **
**----------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
#endif
// End 