/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 19840 $ $Date: 2013-02-14 15:06:18 -0800 (Thu, 14 Feb 2013) $
 */

#ifndef _SAPP_USSD_H_
#define _SAPP_USSD_H_

vint SAPP_isiUssdCmd(
    ISIP_Message     *cmd_ptr, 
    SAPP_SipObj      *sip_ptr, 
    SAPP_Event       *evt_ptr);

vint SAPP_sipUssdEvent(
    SAPP_SipObj     *sip_ptr,
    SAPP_ServiceObj *service_ptr,
    tSipHandle       hUa,
    tSipHandle       hDialog,
    tUaEvtType       event,
    tUaAppEvent     *uaEvt_ptr,
    SAPP_Event      *evt_ptr);

#endif
