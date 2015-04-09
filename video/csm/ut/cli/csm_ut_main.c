/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2006 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 20269 $ $Date: 2013-03-29 02:06:02 -0700 (Fri, 29 Mar 2013) $
 */

#include <osal.h>
#include <csm_event.h>
#include "_csm_ut.h"

#define D2_VPORT_REVISION D2_Release_CSM_UT_CLI

extern char const D2_Release_CSM_UT_CLI[];

#ifdef OSAL_VXWORKS
int csm_ut_main(int argc, char *argv_ptr[])
#else
OSAL_ENTRY
#endif
{
    OSAL_logMsg("CSM unit test start!!\n"); 
    CSM_UT_menu();
    OSAL_logMsg("Bye\n");
    return (0);
}
#ifndef OSAL_VXWORKS
OSAL_EXIT
#endif
