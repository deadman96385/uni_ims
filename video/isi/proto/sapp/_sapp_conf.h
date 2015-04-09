/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 29964 $ $Date: 2014-11-20 13:26:23 +0800 (Thu, 20 Nov 2014) $
 */

#ifndef _SAPP_CONF_H_
#define _SAPP_CONF_H_

vint SAPP_conferenceSubscribe(
    tSipHandle      *hDialog_ptr,
    SAPP_ServiceObj *service_ptr,
    char            *to_ptr,
    char            *from_ptr,
    char            *acceptContact_ptr);

vint SAPP_conferenceUnsubscribe(
    tSipHandle      *hDialog_ptr,
    SAPP_ServiceObj *service_ptr,
    char            *to_ptr,
    char            *from_ptr,
    char            *acceptContact_ptr);

vint SAPP_conferenceSubscribeEvent(
    SAPP_ServiceObj *service_ptr,
    tUaAppEvent     *sipEvt_ptr,
    SAPP_Event      *evt_ptr,
    SAPP_SipObj     *sip_ptr);

#endif
