/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2012 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 29388 $ $Date: 2014-10-17 10:09:39 +0800 (Fri, 17 Oct 2014) $
 */

#ifndef _CSM_SERVICE_H_
#define _CSM_SERVICE_H_

#include <osal.h>
#include <csm_event.h>
#include <ezxml.h>
#include <rpm.h>
#include "_csm_event.h"
#include "_csm_isi.h"

#ifdef INCLUDE_SUPSRV
#include "_csm_supsrv.h"
#endif

#define CSM_SERVICE_XML_DOC_PATH_NAME_SIZE  (128)
#define CSM_SERVICE_XML_DOC_NAME            ("csm.xml")
#define CSM_SERVICE_STR_SZ                  (128)
#define CSM_SERVICE_SIP_SCHEME              ("sip:")

typedef enum {
    CSM_URL_FORMAT_TEL_URI = 0,
    CSM_URL_FORMAT_SIP_URI
} CSM_UrlFmt;

typedef struct {
    CSM_UrlFmt natUrlFmt;
    CSM_UrlFmt intUrlFmt;
} CSM_UrlFmtSettings;

/* Tranport portocol settings strcut */
typedef struct {
    CSM_TransportProto psSignalling;
    CSM_TransportProto wifiSignalling;
    CSM_TransportProto psMedia;
    CSM_TransportProto wifiMedia;
    CSM_TransportProto psRTMedia;
    CSM_TransportProto wifiRTMedia;
} CSM_TransportProtoSettings;

/* 
 * Top level class of the Account Package 
 */
typedef struct {
    char         username[CSM_SERVICE_STR_SZ + 1];
    char         password[CSM_SERVICE_STR_SZ + 1];
    char         authname[CSM_SERVICE_STR_SZ + 1];
    char         realm[CSM_SERVICE_STR_SZ + 1];
    char         proxy[CSM_SERVICE_STR_SZ + 1];
    char         stun[CSM_SERVICE_STR_SZ + 1];
    char         relay[CSM_SERVICE_STR_SZ + 1];
    char         xcap[CSM_SERVICE_STR_SZ + 1];
    char         chat[CSM_SERVICE_STR_SZ + 1];
    char         audioconf[CSM_SERVICE_STR_SZ + 1];
    char         videoconf[CSM_SERVICE_STR_SZ + 1];
                 /*
                  * Accecpt multiple P-CSCF addresses up to 5.
                  * Seperate muliple P-CSCF address by ','.
                  * Example: 
                  *     pcscf1.home1.net, pcscf2.home1.net, pcscf3.home1.net
                  */
    char         obProxy[CSM_EVENT_LONG_STRING_SZ + 1];
    char         eObProxy[CSM_EVENT_LONG_STRING_SZ + 1]; /* Emergency PCSCF addr */
    char         uri[CSM_SERVICE_STR_SZ + 1];
    char         instanceId[CSM_INSTANCE_STRING_SZ + 1];
    char         chatConfUri[CSM_SERVICE_STR_SZ + 1];
    struct {
        vint     result;
        char     response[CSM_AKA_RESP_STRING_SZ];
        vint     resLength;
        char     auts[CSM_AKA_AUTS_STRING_SZ];
        char     ik[CSM_AKA_IK_STRING_SZ];
        char     ck[CSM_AKA_CK_STRING_SZ];
    } aka;
    struct {
        vint     protectedPort;           /* Protected port */
        vint     protectedPortPoolSz;     /* Protected port pool size */
        vint     spi;
        vint     spiPoolSz;
    } ipsec;
    char         imeiUri[CSM_SERVICE_STR_SZ + 1];
    vint         sipPort;
    vint         audioRtpPort;
    vint         audioPoolSize;
    vint         videoRtpPort;
    vint         videoPoolSize;
                 /* Indicate if service is provisioned from ISIM */
    OSAL_Boolean isReady;
                 /* Indicate if RCS provisioning is enabled or not. */
    OSAL_Boolean isRcsProvisioningEnabled;
                 /* Indicate if RCS data provisioned or not. */
    OSAL_Boolean isRcsDataProvisioned;
                 /* RCS provisioning data */
    char         rcsProvisioningData[ISI_PROVISIONING_DATA_STRING_SZ];
    struct {
        vint     type;
        char     id[CSM_CGI_STRING_SZ + 1];
    } cgi;
    CSM_IsiMngr *isiMngr_ptr;
    CSM_UrlFmtSettings urlFmt;
    CSM_TransportProtoSettings transportProto;
                 /* Indicate if IMS is enabled or not. */
    OSAL_Boolean isImsEnabled;
    OSAL_NetAddress    regIpAddress;
} CSM_ServiceMngr;

/* 
 * CSM Service Manager package public methods
 */
vint CSM_serviceInit(
    CSM_ServiceMngr    *serviceMngr_ptr,
    CSM_IsiMngr        *isiMngr_ptr,
#ifdef INCLUDE_SUPSRV
    SUPSRV_Mngr        *supSrvManager_ptr,
#endif
    void               *cfg_ptr);

vint CSM_serviceProcessEvent(
    CSM_ServiceMngr *serviceMngr_ptr,
    CSM_ServiceEvt  *serviceEvt_ptr,
    CSM_OutputEvent *csmOutput_ptr);

vint CSM_serviceShutdown(
    CSM_ServiceMngr *serviceMngr_ptr);

void CSM_serviceOnImsRadioChange(
    RPM_RadioInterface *radioInfc_ptr,
    RPM_RadioType       radioType);

void CSM_serviceEmerRegRequiredChange(
    vint    isEmergencyRegRequired);

void CSM_serviceConvertToInternalEvt(
    CSM_InputEvtType    type,
    void               *inputServiceEvt_ptr,
    CSM_ServiceEvt     *csmServiceEvt_ptr);

/* 
 * CSM Service Manager private methods
 */
OSAL_Boolean _CSM_serviceIsReady(
    CSM_ServiceMngr *serviceMngr_ptr,
    CSM_IsiService  *isiService_ptr);

#endif //_CSM_SERVICE_H_
