/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004-2010 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Revision: 1381 $ $Date: 2006-12-05 14:44:53 -0800 (Tue, 05 Dec 2006) $
 */
#ifndef __VPR_DAEMON_H__
#define __VPR_DAEMON_H__

void _VPR_daemonStop(VPR_Obj *VPR_Obj_ptr);

OSAL_Status _VPR_daemonGo(VPR_Obj *VPR_Obj_ptr);

OSAL_Status _VPR_startIsipTask(
    VPR_Obj *VPR_Obj_ptr);
OSAL_Status _VPR_startVideoTask(
    VPR_Obj *VPR_Obj_ptr);
OSAL_Status _VPR_startVpmdTask(
    VPR_Obj *VPR_Obj_ptr);
OSAL_Status _VPR_startKernelTask(
    VPR_Obj *VPR_Obj_ptr);


#endif
