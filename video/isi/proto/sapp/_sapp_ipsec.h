/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 25167 $ $Date: 2014-03-17 16:53:00 +0800 (Mon, 17 Mar 2014) $
 */

#ifndef _SAPP_IPSEC_H_
#define _SAPP_IPSEC_H_

#define _SAPP_IPSEC_IK_SZ     (16)
#define _SAPP_IPSEC_CK_SZ     (16)
#define _SAPP_IPSEC_CK_SUB_SZ (8)

#define _SAPP_IPSEC_DEFAULT_PROTOCOL OSAL_IPSEC_PROTOCOL_ESP
#define _SAPP_IPSEC_DEFAULT_AUTH_ALG OSAL_IPSEC_AUTH_ALG_HMAC_SHA1_96
#define _SAPP_IPSEC_DEFAULT_ENC_ALG  OSAL_IPSEC_ENC_ALG_DES_EDE3_CBC
#define _SAPP_IPSEC_DEFAULT_MODE     OSAL_IPSEC_SP_MODE_TRANSPORT

extern const char *_SAPP_ipsecAlgStrings[OSAL_IPSEC_AUTH_ALG_LAST + 1];

extern const char *_SAPP_ipsecProtStrings[OSAL_IPSEC_PROTOCOL_LAST + 1];

extern const char *_SAPP_ipsecModStrings[OSAL_IPSEC_SP_MODE_LAST + 1];

extern const char *_SAPP_ipsecEAlgStrings[OSAL_IPSEC_ENC_ALG_LAST + 1];

vint _SAPP_ipsecSetLocalAddress(
    SAPP_ServiceObj *service_ptr,
    OSAL_NetAddress *addr_ptr);

vint _SAPP_ipsecParseSecurityServer(
    SAPP_ServiceObj *service_ptr);

vint _SAPP_ipsecInit(
    SAPP_ServiceObj *service_ptr,
    SAPP_SipObj     *sip_ptr);

vint _SAPP_ipsecSetKey(
    SAPP_ServiceObj *sip_ptr,
    const uint8     *ik,
    vint             ikLen,
    const uint8     *ck,
    vint             ckLen);

vint _SAPP_ipsecCreateSAs(
    SAPP_ServiceObj *service_ptr,
    SAPP_Event      *evt_ptr);

vint _SAPP_ipsecClearSAs(
    SAPP_ServiceObj *service_ptr,
    SAPP_Event      *evt_ptr);

vint _SAPP_ipsecSetProtectedPort(
    SAPP_ServiceObj *service_ptr,
    SAPP_SipObj     *sip_ptr);

vint _SAPP_ipsecSetDefaultPort(
    SAPP_ServiceObj *service_ptr,
    SAPP_SipObj     *sip_ptr);

vint _SAPP_ipsecAddSecurityVerify(
    SAPP_ServiceObj *service_ptr,
    char            *hdrFlds_ptr[],
    vint            *numHdrFlds);

void _SAPP_ipsecAddSecurityClient(
    SAPP_ServiceObj *service_ptr,
    char            *target_ptr,
    vint             targetLen);

void _SAPP_ipsecAddSecAgreeRequire(
    SAPP_ServiceObj *service_ptr,
    char            *hdrFlds_ptr[],
    vint            *numHdrFlds);

vint _SAPP_ipsecCalculateAKAResp(
    SAPP_ServiceObj *service_ptr,
    uint8           *rand_ptr,
    uint8           *autn_ptr,
    SAPP_SipObj     *sip_ptr);

vint _SAPP_ipsecClean(
    SAPP_ServiceObj *service_ptr,
    SAPP_SipObj     *sip_ptr);

vint _SAPP_ipsecResetPort(
    SAPP_ServiceObj *service_ptr,
    SAPP_SipObj     *sip_ptr,
    uint16           portUC,
    uint16           portUS);

#endif
