/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 29964 $ $Date: 2014-11-20 13:26:23 +0800 (Thu, 20 Nov 2014) $
 */

#ifndef _SAPP_MWI_H_
#define _SAPP_MWI_H_

vint SAPP_mwiSubscribe(
    SAPP_MwiObj     *mwi_ptr,
    SAPP_ServiceObj *service_ptr);

vint SAPP_mwiUnsubscribe(
    SAPP_MwiObj     *mwi_ptr,
    SAPP_ServiceObj *service_ptr);

void SAPP_mwiInit(
    SAPP_MwiObj     *mwi_ptr,
    uint32           reMwiTimeoutSecs,
    vint             useMwiEvt,
    SAPP_ServiceObj *service_ptr);

vint SAPP_mwiEvent(
    SAPP_ServiceObj *service_ptr,
    tUaAppEvent     *sipEvt_ptr,
    SAPP_Event      *evt_ptr,
    SAPP_SipObj     *sip_ptr);

vint SAPP_mwiParseTextMessage(
    SAPP_ServiceObj *service_ptr,
    char            *contentType_ptr,
    char            *payload_ptr,
    vint             payloadLen,
    ISIP_Message    *isi_ptr);

#endif
