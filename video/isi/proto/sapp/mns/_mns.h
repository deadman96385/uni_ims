/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 30369 $ $Date: 2014-12-11 19:09:13 +0800 (Thu, 11 Dec 2014) $
 */

#ifndef __MNS_H_
#define __MNS_H_

vint _MNS_initOutboundSession(
    MNS_ServiceObj  *service_ptr,
    MNS_SessionObj  *mns_ptr);

vint _MNS_initInboundSession(
    MNS_ServiceObj  *service_ptr,
    MNS_SessionObj  *mns_ptr,
    tSession        *rmtSess_ptr);

void _MNS_callHold(
    MNS_SessionObj  *mns_ptr);

void _MNS_callResume(
    MNS_SessionObj  *mns_ptr);

vint _MNS_addMedia(
    MNS_SessionObj *mns_ptr,
    ISIP_Message   *m_ptr);

vint _MNS_getMedia(
    MNS_SessionObj *mns_ptr,
    tMedia  *media_ptr,
    vint     mediaIndex);

vint _MNS_setMedia(
    MNS_SessionObj *mns_ptr,
    tMedia  *media_ptr,
    vint     mediaIndex);

vint _MNS_disableMedia(
    MNS_SessionObj *mns_ptr,
    vint mediaIndex);

void _MNS_sipLoadSipCoderString(
    MNS_ServiceObj   *s_ptr,
    vint             *prate_ptr,
    char             *target_ptr);

void _MNS_setSessionPtr(
    MNS_SessionObj  *mns_ptr);

void _MNS_clearSessionPtr(
    MNS_SessionObj  *mns_ptr);

vint _MNS_loadDefaultSession(
    MNS_ServiceObj *service_ptr,
    MNS_SessionObj *mns_ptr);

void _MNS_setSessionInactive(
    MNS_SessionObj  *mns_ptr);

void _MNS_setSessionActive(
    MNS_SessionObj *mns_ptr);

void _MNS_updateIsNewMediaAdded(
    MNS_SessionObj  *mnsOld_ptr,
    MNS_SessionObj  *mnsNew_ptr);

void _MNS_updateDirection(
    MNS_SessionObj *mns_ptr);

#endif
