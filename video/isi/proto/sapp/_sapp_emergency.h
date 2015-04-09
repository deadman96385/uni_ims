/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 17122 $ $Date: 2012-04-26 18:27:14 +0800 (Thu, 26 Apr 2012) $
 */

#ifndef _SAPP_EMG_H_
#define _SAPP_EMG_H_

vint SAPP_emgcyInit(
    SAPP_EmgcyObj *emgcy_ptr,
    OSAL_MsgQId    tmrEvtQ);

void SAPP_emgcyDestroy(
    SAPP_EmgcyObj *emgcy_ptr);

void SAPP_emgcyCallInitiate(
    SAPP_ServiceObj  *service_ptr,
    SAPP_SipObj      *sip_ptr,
    ISIP_Message     *cmd_ptr,
    SAPP_CallObj     *call_ptr);

void SAPP_emgcyCallTerminate(
    SAPP_ServiceObj  *service_ptr,
    SAPP_SipObj      *sip_ptr,
    ISIP_Message     *cmd_ptr,
    SAPP_CallObj     *call_ptr);

void SAPP_emgcyRegComplete(
    SAPP_ServiceObj  *service_ptr,
    SAPP_SipObj      *sip_ptr,
    ISIP_Message     *cmd_ptr,
    vint              expires);

void SAPP_emgcyRegFailed(
    SAPP_ServiceObj  *service_ptr,
    SAPP_SipObj      *sip_ptr,
    ISIP_Message     *cmd_ptr);

void SAPP_emgcyRegExpired(
    SAPP_ServiceObj  *service_ptr,
    SAPP_SipObj      *sip_ptr,
    ISIP_Message     *cmd_ptr);

#endif
