/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2012 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 27294 $ $Date: 2014-07-07 13:38:14 +0800 (Mon, 07 Jul 2014) $
 */

#ifndef _CSM_ISI_SERVICE_H_
#define _CSM_ISI_SERVICE_H_

#include <isi.h>
#include "_csm_service.h"

/* 
 * CSM ISI Call private methods
 */
void _CSM_isiServiceTypeEventHandler(
    CSM_IsiMngr *isiMngr_ptr,
    ISI_Id       serviceId,
    ISI_Event    evt,
    char        *desc_ptr);

vint _CSM_isiServiceDestroy(
    CSM_ServiceMngr *serviceMngr_ptr,
    vint             protocol);

vint _CSM_isiServiceCreate(
    CSM_ServiceMngr *serviceMngr_ptr,
    vint             protocol,
    CSM_OutputEvent *csmOutput_ptr);

vint _CSM_isiServiceSetup(
    CSM_ServiceMngr *serviceMngr_ptr,
    ISI_Id           serviceId);

vint _CSM_isiServiceActivate(
    CSM_ServiceMngr *serviceMngr_ptr,
    ISI_Id           serviceId);

vint _CSM_isiServiceDeactivate(
    CSM_ServiceMngr *serviceMngr_ptr,
    ISI_Id           serviceId);

vint _CSM_isiServiceProcessFsmEvent(
    CSM_ServiceMngr *serviceMngr_ptr,
    ISI_Id           serviceId,
    vint             reason,
    const char      *reason_ptr);

void _CSM_isiServiceProcessIpChange(
    CSM_ServiceMngr *serviceMngr_ptr,
    RPM_RadioType    radioType,
    RPM_RadioInterface *radioInfc_ptr);

void _CSM_isiServiceProcessEmerRegChange(
    CSM_ServiceMngr     *serviceMngr_ptr);

void _CSM_isiServiceSendRetry(
    CSM_ServiceMngr *serviceMngr_ptr,
    ISI_Id           serviceId);

vint _CSM_isiServiceSendAkaChallenge(
    CSM_ServiceMngr *serviceMngr_ptr,
    ISI_Id           serviceId,
    CSM_OutputEvent *csmOutput_ptr);

vint _CSM_isiServiceSetAkaResponse(
    CSM_ServiceMngr *serviceMngr_ptr,
    ISI_Id           serviceId);

vint CSM_isiServiceSetPorts(
    CSM_ServiceMngr *serviceMngr_ptr,
    CSM_IsiService  *isiSrvc_ptr);

vint CSM_isiServiceSetIpsec(
    CSM_ServiceMngr *serviceMngr_ptr,
    CSM_IsiService  *isiSrvc_ptr);

vint CSM_isiServiceSetImeiUri(
    CSM_ServiceMngr *serviceMngr_ptr,
    CSM_IsiService  *isiSrvc_ptr);

vint CSM_isiServiceSetInstanceId(
    CSM_ServiceMngr *serviceMngr_ptr,
    CSM_IsiService  *isiSrvc_ptr);

vint CSM_isiServiceUpdateCgi(
    CSM_ServiceMngr *serviceMngr_ptr,
    CSM_IsiService  *isiSrvc_ptr);

vint _CSM_isiServiceActivateSlaveSip(
    CSM_ServiceMngr *serviceMngr_ptr,
    vint             protocolMasterSip);

vint _CSM_isiServiceDeactivateSlaveSip(
    CSM_ServiceMngr *serviceMngr_ptr,
    vint             protocolMasterSip);

vint _CSM_isiServiceSetFeature(
    CSM_IsiService *service_ptr);

vint _CSM_isiServiceSendIpsecEvent(
    ISI_Id            serviceId,
    vint              reason,
    CSM_OutputEvent  *csmOutput_ptr);

vint _CSM_isiServiceGetProvisioningData(
    CSM_ServiceMngr *serviceMngr_ptr,
    ISI_Id           serviceId);

vint CSM_isiServiceSetRcsProvisioningData(
    CSM_ServiceMngr *serviceMngr_ptr,
    CSM_IsiService  *isiSrvc_ptr);

vint CSM_isiServiceSetMediaSessionType(
    CSM_ServiceMngr    *serviceMngr_ptr);

void CSM_isiServiceSetMasterNetworkMode(
    vint    mode);

OSAL_Boolean CSM_isiServiceIsActive(
    CSM_IsiService *isiSrvc_ptr);

#endif //_CSM_ISI_SERVICE_H_
