/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 30365 $ $Date: 2014-12-11 18:28:14 +0800 (Thu, 11 Dec 2014) $
 */

#ifndef _SAPP_REG_H_
#define _SAPP_REG_H_

void SAPP_regInit(
    SAPP_RegObj     *reg_ptr,
    uint32           reRegTimeoutSecs,
    uint32           natRefreshRateSecs,
    uint32           regRetryBaseTime,
    uint32           regRetryMaxTime,   
    vint             useRegEvt,
    char            *preconfigRoute_ptr,
    SAPP_ServiceObj *service_ptr,
    OSAL_MsgQId      tmrEvtQ);

void SAPP_regDestroy(
    SAPP_RegObj     *reg_ptr);

void SAPP_regStart(
    SAPP_RegObj      *reg_ptr,
    SAPP_ServiceObj  *service_ptr,
    SAPP_Event       *evt_ptr,
    SAPP_SipObj      *sip_ptr);

void SAPP_regStop(
    SAPP_RegObj      *reg_ptr,
    SAPP_ServiceObj  *service_ptr,
    SAPP_Event       *evt_ptr,
    SAPP_SipObj      *sip_ptr);

void SAPP_regRestart(
    SAPP_RegObj      *reg_ptr,
    SAPP_ServiceObj  *service_ptr,
    SAPP_Event       *evt_ptr,
    SAPP_SipObj      *sip_ptr);

void SAPP_regReset(
    SAPP_RegObj      *reg_ptr,
    SAPP_ServiceObj  *service_ptr,
    SAPP_Event       *evt_ptr,
    SAPP_SipObj      *sip_ptr);

void SAPP_regNoNet(
    SAPP_RegObj      *reg_ptr,
    SAPP_ServiceObj  *service_ptr,
    SAPP_Event       *evt_ptr,
    SAPP_SipObj      *sip_ptr);

void SAPP_regYesNet(
    SAPP_RegObj      *reg_ptr,
    SAPP_ServiceObj  *service_ptr,
    SAPP_Event       *evt_ptr,
    SAPP_SipObj      *sip_ptr);

void SAPP_regReReg(
    SAPP_RegObj      *reg_ptr,
    SAPP_ServiceObj  *service_ptr,
    SAPP_Event       *evt_ptr,
    SAPP_SipObj      *sip_ptr);

vint SAPP_regEvent(
    SAPP_RegObj     *reg_ptr,
    SAPP_ServiceObj *service_ptr,
    tUaAppEvent     *sipEvt_ptr,
    SAPP_Event      *evt_ptr,
    vint            *stateChanged_ptr,
    SAPP_SipObj     *sip_ptr);

vint SAPP_regTransportInit(
    SAPP_ServiceObj  *service_ptr,
    SAPP_SipObj      *sip_ptr);

vint SAPP_regTransportClean(
    SAPP_ServiceObj  *service_ptr,
    SAPP_SipObj      *sip_ptr);

vint _SAPP_regStartRetryTmr(
    SAPP_ServiceObj  *service_ptr,
    tUaAppEvent      *sipEvt_ptr);

void _SAPP_regAdvancePcscf(
    SAPP_ServiceObj  *service_ptr,
    SAPP_SipObj      *sip_ptr);

vint _SAPP_regServiceUnavailable(
    SAPP_RegObj     *reg_ptr,
    SAPP_ServiceObj *service_ptr,
    tUaAppEvent     *uaEvt_ptr);

vint _SAPP_regSubscribe(
    SAPP_RegObj     *reg_ptr,
    SAPP_ServiceObj *service_ptr);
#endif
