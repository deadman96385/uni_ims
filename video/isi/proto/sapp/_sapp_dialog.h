/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 30028 $ $Date: 2014-11-21 19:05:32 +0800 (Fri, 21 Nov 2014) $
 */

#ifndef _SAPP_DIALOG_H_
#define _SAPP_DIALOG_H_

void SAPP_sipCallEvent(
    SAPP_SipObj     *sip_ptr, 
    SAPP_ServiceObj *service_ptr, 
    tSipHandle       hUa, 
    tSipHandle       hDialog,
    tUaEvtType       event, 
    tUaAppEvent     *uaEvt_ptr,
    SAPP_Event      *evt_ptr);

void SAPP_isiCallCmd(
    ISIP_Message     *cmd_ptr, 
    SAPP_SipObj      *sip_ptr, 
    SAPP_Event       *evt_ptr);

void SAPP_sipHandoff(
    SAPP_ServiceObj *service_ptr,
    SAPP_Event      *evt_ptr);

void SAPP_sipCallVdxIsiEvt(
    ISI_Id              serviceId,
    vint                protocolId,
    ISI_Id              callId,
    char               *target_ptr,
    ISIP_Message       *isi_ptr);

void SAPP_sipCallIsiEvt(
    ISI_Id              serviceId,
    vint                protocolId,
    ISI_Id              callId,
    ISIP_CallReason     reason,
    ISIP_Status         status,
    ISIP_Message       *isi_ptr);

vint _SAPP_sipUnhold(
    SAPP_ServiceObj *service_ptr,
    SAPP_CallObj    *call_ptr,
    ISIP_Message     *cmd_ptr); 

vint _SAPP_sipHold(
    SAPP_ServiceObj *service_ptr,
    SAPP_CallObj    *call_ptr,
    ISIP_Message     *cmd_ptr); 

vint _SAPP_sipReinvite(
    SAPP_ServiceObj *service_ptr,
    SAPP_CallObj    *call_ptr,
    ISIP_Message    *cmd_ptr);

void _SAPP_sipForwardCall(
    SAPP_ServiceObj *service_ptr,
    tSipHandle       dialogId,
    char            *to_ptr);

vint _SAPP_sipMakeCall(
    SAPP_ServiceObj *service_ptr,
    SAPP_CallObj    *call_ptr,
    char            *to_ptr,
    tSession        *sess_ptr,
    char            *pDisplayName,
    uint32          *srvccStatus);

vint _SAPP_sipCallInitiateOutbound(
    SAPP_SipObj     *sip_ptr,
    SAPP_ServiceObj *service_ptr,
    SAPP_CallObj    *call_ptr,
    ISIP_Message    *cmd_ptr,
    SAPP_Event      *evt_ptr);

vint _SAPP_sipCallInitiateInbound(
    SAPP_SipObj     *sip_ptr,
    SAPP_ServiceObj *service_ptr,
    SAPP_CallObj    *call_ptr,
    tSipHandle       dialogId,
    tUaAppEvent     *uaEvt_ptr,
    SAPP_Event      *evt_ptr);

void _SAPP_getErrorReasonDesc(
    tUaAppEvent *uaEvt_ptr,
    ISIP_Call   *isi_ptr);

void  _SAPP_populateIsiEvtSession(
    MNS_ServiceObj  *service_ptr,
    MNS_SessionObj  *mns_ptr,
    ISIP_Call       *isi_ptr);

void SAPP_sipTerminateAllCalls(
    SAPP_ServiceObj *service_ptr,
    SAPP_Event      *evt_ptr);

void SAPP_sipDestroyCall(
    SAPP_CallObj *call_ptr);

vint SAPP_sipCallInitiateOutbound(
    SAPP_SipObj     *sip_ptr,
    SAPP_ServiceObj *service_ptr,
    SAPP_CallObj    *call_ptr,
    ISIP_Message    *cmd_ptr,
    SAPP_Event      *evt_ptr);

vint _SAPP_sipHungUp(
    SAPP_ServiceObj    *service_ptr,
    tSipHandle          hDialog,
    char               *reasonDesc_ptr);

#endif
