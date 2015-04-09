/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 30298 $ $Date: 2014-12-09 19:05:49 +0800 (Tue, 09 Dec 2014) $
 */

#ifndef _SAPP_XML_H_
#define _SAPP_XML_H_

typedef enum {
    SAPP_XML_REG_STATUS_ON               = 0,
    SAPP_XML_REG_STATUS_TERMINATED       = 1,
    SAPP_XML_REG_STATUS_REJECTED         = 2,
    SAPP_XML_REG_STATUS_ACTIVE_SHORTENED = 3,
    SAPP_XML_REG_STATUS_DEACTIVATED      = 4,
} SAPP_xmlRegStatus;

typedef enum {
    SAPP_XML_ACTION_NONE,
    SAPP_XML_ACTION_EMERGENCY_REG,
    SAPP_XML_ACTION_INITIAL_REG
} SAPP_3gppImsAction;

extern const char _SAPP_XML_ATTR_ENTITY[];
extern const char _SAPP_XML_TAG_ENDPOINT[];

extern const char _SAPP_XML_SHOW_AVAIL[];
extern const char _SAPP_XML_SHOW_UNAVAIL[];
extern const char _SAPP_XML_SHOW_DND[];
extern const char _SAPP_XML_SHOW_CHAT[];
extern const char _SAPP_XML_SHOW_AWAY[];
extern const char _SAPP_XML_SHOW_XAWAY[];
extern const char _SAPP_XML_SHOW_OTP[];

extern const char _SAPP_XML_STATUS_CONNECTED[];
extern const char _SAPP_XML_STATUS_DISCONNECTED[];
extern const char _SAPP_XML_STATUS_ONHOLD[];
extern const char _SAPP_XML_STATUS_MUTEDVIAFOCUS[];
extern const char _SAPP_XML_STATUS_PENDING[];
extern const char _SAPP_XML_STATUS_ALERTING[];
extern const char _SAPP_XML_STATUS_DIALING[];
extern const char _SAPP_XML_STATUS_DISCONNECTING[];

vint SAPP_xmlDecodeRegEventDoc(
    char              *aor_ptr,
    char              *contact_ptr,
    char              *doc_ptr,
    vint               docLen,
    SAPP_xmlRegStatus *regStatus_ptr,
    vint              *numDev_ptr,
    vint              *expires_ptr);

void SAPP_xmlEncodeRegEventDoc(
    const char *aor_ptr,
    const vint  status,
    char       *doc_ptr,
    vint        maxDocLen);

vint SAPP_xmlDecodeConferenceInfoDoc(
    ISI_Id           isiCallId,
    SAPP_ServiceObj *service_ptr,
    const char      *from_ptr,
    char            *contentType_ptr,
    char            *doc_ptr,
    vint             docLen,
    SAPP_Event      *evt_ptr);

vint _SAPP_xmlDecodeIsiCapabilitiesDoc(
    char               *doc_ptr,
    vint                docLen,
    uint32             *capsBitmap_ptr);

extern vint SAPP_xmlDecodeIsiCapabilitiesHelper(
    ezxml_t       xml_ptr,
    uint32       *capsBitmap_ptr);

extern void _SAPP_xmlEncodeCapabilitiesIsiDoc(
    const uint32  capsBitmap,
    const char   *status_ptr,
    char         *doc_ptr,
    vint          maxDocLen);

void SAPP_xmlEncodePresenceDoc(
    const char *from_ptr,
    const char *show_ptr,
    const char *status_ptr,
    const char *priority_ptr,
    char       *doc_ptr,
    vint        maxDocLen);

vint _SAPP_xmlDecode3gppImsDoc(
    char                *doc_ptr,
    vint                 docLen,
    char               **reason_ptr,
    SAPP_3gppImsAction  *xmlAciton);

vint _SAPP_xmlDecode3gppCwiHelper(
    char         *doc_ptr,
    vint          docLen,
    OSAL_Boolean *cwi_ptr);

vint _SAPP_xmlEncodeRcRlsDoc(
    char    *participants_ptr,
    char    *doc_ptr,
    vint     maxDocLen);

#endif
