/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2012 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 29644 $ $Date: 2014-11-03 19:15:40 +0800 (Mon, 03 Nov 2014) $
 */

#ifndef _CSM_ISI_H_
#define _CSM_ISI_H_

#include <osal.h>
#include <csm_event.h>
#include <isi.h>
#include <isi_errors.h>
#include "_csm_event.h"
#include "fsm.h"

/* Task Constants */
#define CSM_ISI_EVENT_THREAD_NAME ("isi")
#define CSM_ISI_TASK_STACK_SZ     (8192)

/* Service Constants */
#define CSM_ISI_NUM_SERVICES      (4)

#define CSM_ISI_PROTOCOL_SIP                (1)
#define CSM_ISI_PROTOCOL_GSM                (3)
#define CSM_ISI_PROTOCOL_SIP_RCS            (6)
#define CSM_ISI_PROTOCOL_VOLTE              (7)
#define CSM_ISI_PROTOCOL_SIP_NAME           ("sip")
#define CSM_ISI_PROTOCOL_SIP_RCS_NAME       ("sip-rcs")
#define CSM_ISI_PROTOCOL_GSM_NAME           ("gsm")
#define CSM_ISI_PROTOCOL_VOLTE_NAME         ("lte")

#define CSM_ISI_NETWORK_INFC_NAME_4G        "lte"
#define CSM_ISI_NETWORK_INFC_NAME_WIFI      "wifi"
#define CSM_ISI_EMPTY_STRING                ""
#define CSM_ISI_IPV4_ZERO_STR               "0.0.0.0"

#define CSM_ISI_ALIAS_URI_STR               ("ALIAS")
/*
 * Define protocol response reason code.
 */
#define CSM_ISI_REASONCODE_REQUEST_TIMED_OUT    (ISI_REQUEST_TIMED_OUT)
#define CSM_ISI_REASONCODE_SIP_FORBIDDEN        (403)

/*
 * Define IMS service protocol id and name in the modem.
 * For CSM running in modem processor under vPort 4G/4G+ architecture, it will
 * be CSM_ISI_PROTOCOL_SIP.
 * For CSM running in application under Seattle architecture, it will be
 * CSM_ISI_PROTOCOL_VOLTE.
 */
#ifdef VPORT_4G_PLUS_APROC
#define CSM_ISI_PROTOCOL_MODEM_IMS           (CSM_ISI_PROTOCOL_VOLTE)
#define CSM_ISI_PROTOCOL_MODEM_IMS_NAME      (CSM_ISI_PROTOCOL_VOLTE_NAME)
#else
#define CSM_ISI_PROTOCOL_MODEM_IMS           (CSM_ISI_PROTOCOL_SIP)
#define CSM_ISI_PROTOCOL_MODEM_IMS_NAME      (CSM_ISI_PROTOCOL_SIP_NAME)
#endif

/*
 * CSM ISI Service object.
 *
 * Contains all relevant information about a given service including the FSM
 * which processes service level registratino events.
 */
typedef struct {
    ISI_Id serviceId;
    vint  isEmergency;
    vint  isRegistered;
    vint  isMasterSip; /* To indicate if this service is master sip service */
    OSAL_Boolean isInitialized;
    CSM_TransportProto rtMedia;
    vint  protocol;
    char *protoName;
    char  username[ISI_ADDRESS_STRING_SZ + 1];
    char  password[ISI_ADDRESS_STRING_SZ + 1];
    char  realm[ISI_ADDRESS_STRING_SZ + 1];
    char  proxy[ISI_ADDRESS_STRING_SZ + 1];
    char  stun[ISI_ADDRESS_STRING_SZ + 1];
    char  relay[ISI_ADDRESS_STRING_SZ + 1];
    char  xcap[ISI_ADDRESS_STRING_SZ + 1];
    char  chat[ISI_ADDRESS_STRING_SZ + 1];
    char  audioconf[ISI_ADDRESS_STRING_SZ + 1];
    char  videoconf[ISI_ADDRESS_STRING_SZ + 1];
    char  outProxy[ISI_LONG_ADDRESS_STRING_SZ + 1];
    char  uri[ISI_ADDRESS_STRING_SZ + 1];
    char  chatConfUri[ISI_ADDRESS_STRING_SZ + 1];
    FSM_Context  fsm;    
} CSM_IsiService;

/* 
 * ISI Manager Package structures
 */
typedef struct {
    CSM_IsiService      service[CSM_ISI_NUM_SERVICES];
    OSAL_TaskId         task;
    OSAL_MsgQId         eventQ;
    CSM_PrivateInputEvt csmInputEvent; /* Used when the ISI thread
                                        * writes input events to CSM. */
} CSM_IsiMngr;

/* 
 * CSM ISI Manager package public methods 
 */
vint CSM_isiAllocate(
    CSM_IsiMngr *isiMngr_ptr,
    OSAL_MsgQId  qId);

vint CSM_isiStart(
    CSM_IsiMngr *isiMngr_ptr);

vint CSM_isiInit(
    CSM_IsiMngr *isiMngr_ptr,
    OSAL_MsgQId  qId);

vint CSM_isiShutdown(
    CSM_IsiMngr *isiMngr_ptr);

vint CSM_isiSendEvent(
    CSM_IsiMngr         *isiMngr_ptr,
    CSM_PrivateInputEvt *event_ptr);

void _CSM_isiServiceTypeEventHandler(
    CSM_IsiMngr *isiMngr_ptr,
    ISI_Id       serviceId,
    ISI_Event    event,
    char        *desc_ptr);

void _CSM_isiCallTypeEventHandler(
    CSM_IsiMngr    *isiMngr_ptr,
    ISI_Id          isiServiceId,
    ISI_Id          isiCallId,
    ISI_Event       event,
    const char     *desc_ptr);

void _CSM_isiCallPresenceTypeEventHandler(
    CSM_IsiMngr    *isiMngr_ptr,
    ISI_Id          isiServiceId,
    ISI_Id          isiCallId,
    ISI_Event       event,
    const char     *desc_ptr);

void _CSM_isiSmsTypeEventHandler(
    CSM_IsiMngr    *isiMngr_ptr,
    ISI_Id          isiServiceId,
    ISI_Id          isiCallId,
    ISI_Event       event,
    const char     *desc_ptr);

/* 
 * CSM ISI Manager private methods
 */
OSAL_INLINE void _CSM_isiProcessEvent(
    CSM_IsiMngr *isiMngr_ptr,
    ISI_Id       serviceId,
    ISI_Id       id,
    ISI_IdType   idType,
    ISI_Event    event,
    char        *desc_ptr);

CSM_IsiService* CSM_isiGetServiceViaId(
    CSM_IsiMngr *isiMngr_ptr,
    ISI_Id       serviceId);

CSM_IsiService* CSM_isiGetServiceViaProtocol(
    CSM_IsiMngr *isiMngr_ptr,
    vint         protocol,
    vint         isEmergency);

const char* CSM_isiProtoNameViaServiceId(
    CSM_IsiMngr *isiMngr_ptr,
    ISI_Id       serviceId);

CSM_IsiService* CSM_isiNormalizeOutboundAddress(
    CSM_IsiMngr         *isiMngr_ptr,
    char const          *address_ptr,
    char                *out_ptr,
    vint                 maxOutLen,
    RPM_FeatureType      callType);

void CSM_isiNormalizeInboundAddress(
    CSM_IsiService      *service_ptr,
    char const          *address_ptr,
    char                *outAddress_ptr,
    vint                 maxOutAddressLen,
    CSM_CallAddressType *outAddressType_ptr,
    char                *outAlpha_ptr,
    vint                 maxOutAlphaLen);

CSM_IsiService* CSM_isiGetMasterSipService(
    CSM_IsiMngr *isiMngr_ptr);

OSAL_Boolean CSM_isiGetServiceIsActive(
    CSM_IsiService *service_ptr);

vint CSM_isiAllocate(
    CSM_IsiMngr *isiMngr_ptr,
    OSAL_MsgQId  qId);

vint CSM_isiStart(
     CSM_IsiMngr *isiMngr_ptr);

vint CSM_isiDestroy(
    CSM_IsiMngr *isiMngr_ptr);

#endif //_CSM_ISI_H_
