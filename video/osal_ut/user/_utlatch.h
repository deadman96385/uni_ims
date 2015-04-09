/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL  
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE  
 * APPLIES: "COPYRIGHT 2004-2007 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"  
 *  
 * $D2Tech$ $Rev: 28444 $ $Date: 2014-08-25 07:51:30 +0800 (Mon, 25 Aug 2014) $
 * 
 */

#ifndef __OSAL_UT_LATCH_H__
#define __OSAL_UT_LATCH_H__

typedef struct {
    OSAL_SemId semId;
    uint8      count;
} OSALUT_Latch;

OSAL_Status OSALUT_latchInit(
    OSALUT_Latch *l_ptr,
    uint8         keyNum);

OSAL_Status OSALUT_latchUnlock(
    OSALUT_Latch *l_ptr);

OSAL_Status OSALUT_latchWait(
    OSALUT_Latch *l_ptr);
    
OSAL_Status OSALUT_latchDestroy(
    OSALUT_Latch *l_ptr);
    
#endif

