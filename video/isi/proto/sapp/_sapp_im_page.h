/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 29964 $ $Date: 2014-11-20 13:26:23 +0800 (Thu, 20 Nov 2014) $
 */

#ifndef _SAPP_IM_PAGE_H_
#define _SAPP_IM_PAGE_H_

//#define SAPP_IM_PAGE_USE_CPIM (1)

void SAPP_imPageInit(
    SAPP_ImObj       *im_ptr);

void SAPP_isiImPageCmd(
    ISIP_Message    *cmd_ptr, 
    SAPP_SipObj     *sip_ptr,
    ISIP_Message    *isi_ptr);

vint SAPP_imEvent(
    SAPP_ServiceObj *service_ptr,
    tUaAppEvent     *sipEvt_ptr,
    SAPP_Event      *evt_ptr,
    SAPP_SipObj     *sip_ptr);

vint SAPP_imParseMultipartMessage(
    SAPP_ServiceObj *service_ptr,
    char            *contentType_ptr,
    char            *payload_ptr,
    vint             payloadLen,
    SAPP_Event      *evt_ptr);

#endif
