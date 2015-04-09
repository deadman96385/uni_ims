/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 20842 $ $Date: 2013-05-28 15:20:10 +0800 (Tue, 28 May 2013) $
 */

#ifndef __MNS_PRECONDITION_H_
#define __MNS_PRECONDITION_H_

vint _MNS_preconditionInit(
    MNS_SessionObj *mns_ptr);

vint _MNS_preconditionUpdate(
    MNS_SessionObj *mns_ptr,
    tPrecondition  *lclPrec_ptr,
    tPrecondition  *rmtPrec_ptr);

OSAL_Boolean _MNS_preconditionIsMet(
    tPrecondition   *p_ptr,
    tPrecStatusType  statusType);

void _MNS_preconditionSetLocalRsrcStatus(
    MNS_SessionObj *mns_ptr);

void  _MNS_preconditionUpdateMet(
    tPrecondition   *p_ptr);

#endif
