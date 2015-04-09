/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 20679 $ $Date: 2013-05-15 18:20:29 +0800 (Wed, 15 May 2013) $
 */
#ifndef _SAPP_RADIO_UPDATE_H_
#define _SAPP_RADIO_UPDATE_H_

void SAPP_radioInterfaceUpdate(
    SAPP_SipObj         *sip_ptr,
    SAPP_ServiceObj     *service_ptr,
    SAPP_Event          *evt_ptr,
    OSAL_NetAddress      address,
    char                *infcName);

void SAPP_radioUpdateCall(
    SAPP_SipObj         *sip_ptr,
    SAPP_ServiceObj     *service_ptr,
    SAPP_Event          *evt_ptr);
#endif
