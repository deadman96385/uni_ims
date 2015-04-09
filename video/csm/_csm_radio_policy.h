/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2012 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 23805 $ $Date: 2013-12-30 15:01:12 +0800 (Mon, 30 Dec 2013) $
 */

#ifndef _CSM_RADIO_POLICY_H_
#define _CSM_RADIO_POLICY_H_

#include <osal.h>
#include <csm_event.h>
#include "_csm_service.h"

/* 
 * CSM Radio Policy Manager package public methods 
 */
vint CSM_rpmInit(
    CSM_ServiceMngr *serviceMngr_ptr);

vint CSM_rpmProcessEvent(
    CSM_InputRadio *radioEvt_ptr);

vint CSM_rpmShutdown(
    void);
/* 
 * CSM RPM private methods
 */

#endif //_CSM_RADIO_POLICY_H_
