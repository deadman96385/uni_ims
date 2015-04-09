/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 14437 $ $Date: 2011-04-10 23:38:48 +0800 (Sun, 10 Apr 2011) $
 */

#ifndef _SAPP_TE_H_
#define _SAPP_TE_H_

void SAPP_teInit(
    SAPP_TelEvt   *tel_ptr);

void SAPP_isiTeCmd(
    ISIP_Message        *cmd_ptr, 
    SAPP_SipObj         *sip_ptr,
    SAPP_Event          *evt_ptr);

vint SAPP_teEvent(
    SAPP_TelEvt     *tel_ptr,
    SAPP_ServiceObj *service_ptr,
    tUaAppEvent     *sipEvt_ptr,
    SAPP_Event      *evt_ptr);

#endif
