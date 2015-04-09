/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2012 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 30357 $ $Date: 2014-12-11 16:11:02 +0800 (Thu, 11 Dec 2014) $
 */
#ifndef _CSM_SUPSRV_H_
#define _CSM_SUPSRV_H_

#include <xcap.h>
#include <supsrv.h>
#include <rpm.h>

#include <csm_event.h>
#include "_csm_event.h"


/* 
 * CSM SUPSRV Manager package public methods 
 */
vint CSM_supSrvShutdown(
    SUPSRV_Mngr *supSrvMngr_ptr);

vint CSM_supSrvProcessEvent(
    SUPSRV_Mngr     *supSrvMngr_ptr,
    CSM_InputSupSrv *csmSupsrv_ptr,
    CSM_OutputEvent *csmOutput_ptr);

vint CSM_xcapProcessEvent(
    SUPSRV_Mngr     *supSrvMngr_ptr,
    SUPSRV_XcapEvt  *supSrvEvt_ptr,
    CSM_OutputEvent *csmOutput_ptr);

vint CSM_supSrvInit(
    SUPSRV_Mngr *supSrvMngr_ptr,
    OSAL_MsgQId  qId);

void CSM_supSrvOnRadioChange(
    RPM_RadioInterface *radioInfc_ptr,
    RPM_RadioType       radioType);

void CSM_supSrvProvisioning(
    char *rootUri,
    char *authName,
    char *authSecret);

void CSM_supSrvSetXcapUri(
    char *uri);

void CSM_supSrvSetImpu(
    char *impu);

#endif //_CSM_SUPSRV_H_
