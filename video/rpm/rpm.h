/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2012 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 5312 $ $Date: 2008-02-18 19:05:41 -0600 (Mon, 18 Feb 2008) $
 */

#ifndef RPM_H_
#define RPM_H_

#include <osal.h>

/*
 * Public RPM Constants
 */
#define RPM_ADDRESS_STR_LEN        (128)

typedef enum {
    RPM_RETURN_OK = 0,
    RPM_RETURN_ERROR
} RPM_Return;

typedef enum {
    RPM_EVENT_TYPE_IP_CHANGE = 0,
    RPM_EVENT_TYPE_EMERGENCY_CALL,
    RPM_EVENT_TYPE_SRVCC_START,
    RPM_EVENT_TYPE_SRVCC_FAIL,
    RPM_EVENT_TYPE_SERVICE_STATE_CHANGE,
    RPM_EVENT_TYPE_SERVICE_CHANGE, /* Service specific configurations change */
} RPM_EventType;

typedef enum {
    RPM_FEATURE_TYPE_CALL_NORMAL = 0,
    RPM_FEATURE_TYPE_CALL_EMERGENCY, /* No domain Specified emergency call */
    RPM_FEATURE_TYPE_CALL_IMS_EMERGENCY, /* Domain specified emergency call */
    RPM_FEATURE_TYPE_CALL_CS_EMERGENCY, /* Domain specified emergency call */
    RPM_FEATURE_TYPE_CALL_SPECIAL, // for examample USSD
    RPM_FEATURE_TYPE_SMS,
    RPM_FEATURE_TYPE_IMS_SERVICE,
    RPM_FEATURE_TYPE_IMS_EMERGENCY_SERVICE,
    RPM_FEATURE_TYPE_RCS,
    RPM_FEATURE_TYPE_SUPSRV
} RPM_FeatureType;

typedef enum {
    RPM_RADIO_TYPE_NONE = 0,
    RPM_RADIO_TYPE_CS,
    RPM_RADIO_TYPE_LTE,
    RPM_RADIO_TYPE_LTE_EMERGENCY,
    RPM_RADIO_TYPE_WIFI,
    RPM_RADIO_TYPE_LTE_SS
} RPM_RadioType;

typedef enum {
    RPM_SERVICE_STATE_INACTIVE = 0,
    RPM_SERVICE_STATE_ACTIVE
} RPM_ServiceState;

typedef enum {
    RPM_URL_FORMAT_TEL_URI = 0,
    RPM_URL_FORMAT_SIP_URI
} RPM_UrlFmtVal;

typedef struct {
    RPM_UrlFmtVal natUrlFmt;
    RPM_UrlFmtVal intUrlFmt;
} RPM_UrlFmt;

/*
 * Public RPM Structure definitions
 */
typedef struct {
    RPM_EventType    type;
    RPM_RadioType    radioType;
    union {
        OSAL_NetAddress  ipAddress;
        RPM_ServiceState serviceState;
        struct {
            vint         isEmergencyFailoverToCs;
            vint         isEmergencyRegRequired;
        } config;
    } u;
} RPM_Event;

#if 0
typedef struct {
    OSAL_NetAddress lteIpAddr;
    OSAL_NetAddress wifiIpAddr;
    RPM_RadioType   radioType;
} RPM_RadioInterface;
#endif

typedef struct {
    OSAL_NetAddress ipAddr;
    RPM_RadioType   radioType;
} RPM_RadioInterface;

typedef struct {
    char               normallizedAddress[RPM_ADDRESS_STR_LEN];
    RPM_RadioInterface radioInfc;
    RPM_FeatureType    callFeatureType;
} RPM_Address;

/*
 * Public function pointer definitions
 */
typedef void (*RPM_notifyImsRadioChange)(RPM_RadioInterface *, RPM_RadioType);
typedef void (*RPM_notifySrvccChange)(RPM_EventType);
typedef void (*RPM_notifyEmerRegChange)(vint);
typedef void (*RPM_notifySupsrvChange)(RPM_RadioInterface *, RPM_RadioType);


/*
 * Public functions
 */
RPM_Return RPM_init(
    void);

RPM_Return RPM_processEvent(
    RPM_Event *event_ptr);

void RPM_shutdown(
    void);

void RPM_registerImsRadioChangeCallback(
    RPM_notifyImsRadioChange imsCb);

void RPM_registerSupsrvRadioChangeCallback(
    RPM_notifySupsrvChange supsrvCb);

void RPM_registerSrvccChangeCallback(
    RPM_notifySrvccChange srvccCb);

void RPM_eRegisterRequiredChangeCallback(
    RPM_notifyEmerRegChange emerRegCb);

void RPM_setServiceState(
    RPM_RadioType    serviceType, 
    RPM_ServiceState serviceState);

RPM_Return RPM_normalizeInboundAddress(
    RPM_RadioType  type,
    const char    *domain_ptr,
    const char    *address_ptr,
    char          *out_ptr,
    vint           maxOutLen,
    char          *display_ptr,
    vint           maxDisplayLen); 


RPM_Return RPM_normalizeOutboundAddress(
    const char      *domain_ptr,
    const char     *address_ptr,
    char           *out_ptr,
    vint            maxOutLen,
    RPM_FeatureType type);

RPM_Return RPM_getActiveRadio(
    RPM_FeatureType type, 
    RPM_RadioInterface *radioInfc);

RPM_Return RPM_getAvailableRadio(
    RPM_FeatureType type,
    RPM_RadioInterface *radioInfc_ptr);

OSAL_Boolean RPM_isEmergencyFailoverToCs(
    void);

OSAL_Boolean RPM_isEmergencyRegRequired(
    void);

void RPM_setUrlFmt(
    RPM_UrlFmtVal intUrlFmt,
    RPM_UrlFmtVal natUrlFmt);

RPM_Return RPM_getNameFromAddress(
    const char *address_ptr,
    char       *out_ptr,
    vint        maxOutSize);

#endif // RPM_H_
