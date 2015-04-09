/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004-2007 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 20269 $ $Date: 2013-03-29 02:06:02 -0700 (Fri, 29 Mar 2013) $
 *
 */

#ifndef _CSM_UT_MENU_H_
#define _CSM_UT_MENU_H_

/* xml configuration files folder path */
#ifndef CSM_CONFIG_DEFAULT_PATH
#define CSM_UT_CONFIG_DEFAULT_PATH "//system//bin"
#else
#define CSM_UT_CONFIG_DEFAULT_PATH CSM_CONFIG_DEFAULT_PATH
#endif

typedef enum {
    UT_UNKNOWN = -2,
    UT_PASS = 1,
    UT_FAIL = -1
} UT_Return;

typedef UT_Return (CSM_UT_TestFunc)(vint arg);

typedef struct {
    unsigned char    cmd[20];
    CSM_UT_TestFunc *func_ptr;
    unsigned char    desc[60];
} CSM_UT_TestTableItem;

typedef struct {
    OSAL_MsgQId csmInputEvtQ;
    OSAL_MsgQId csmOutputEvtQ;
    OSAL_TaskId csmEvtTaskId;
} CSM_UT_GlobalObj;

void CSMUT_ipEventSend(
    char   *addr_ptr,
    int     isEmergency);
    
UT_Return CSM_UT_writeCsmEvent(
    const CSM_InputEvent* csmEvt_ptr);

OSAL_TaskReturn CSM_UT_menu(
    void);

#endif
