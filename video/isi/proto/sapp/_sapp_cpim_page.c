/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 14082 $ $Date: 2011-02-25 18:30:07 -0600 (Fri, 25 Feb 2011) $
 */
#include <osal_types.h>
#include <osal.h>
#include <osal_msg.h>

#include <ezxml.h>

#include <sip_sip.h>
#include <sip_xport.h>
#include <sip_ua.h>
#include <sip_app.h>
#include <sip_debug.h>

#include "isi.h"
#include "isip.h"

#include "mns.h"

#ifdef INCLUDE_SIMPLE
#include "xcap.h"
#include "_simple_types.h"
#endif

#include "_sapp.h"
#include "_sapp_parse_helper.h"
#include "_sapp_dialog.h"
#include "_sapp_cpim_page.h"
#include "_sapp_im_page.h"

#ifdef INCLUDE_SIMPLE
#include "_simple.h"
#include "simple/_simple_types.h"
#endif

static const char _SAPP_IM_NS_HF[]                              = "NS:";
static const char _SAPP_IM_MSG_ID_HF[]                          = "imdn.Message-ID:";
static const char _SAPP_IM_DISP_NOTI_HF[]                       = "imdn.Disposition-Notification:";

static const char _SAPP_IM_NS_HF_VALUE[]                        = "imdn <urn:ietf:params:imdn>";
static const char _SAPP_IM_CONTENT_TYPE_HF_VALUE_TEXT[]         = "text/plain;charset=\"UTF-8\"";
static const char _SAPP_IM_CONTENT_TYPE_HF_VALUE_IMDN[]         = "message/imdn+xml";
static const char _SAPP_IM_CONTENT_TYPE_HF_VALUE_COMPOSING[]    = "application/im-iscomposing+xml";
static const char _SAPP_IM_CONTENT_DISP_HF_VALUE[]              = "notification";
static const char _SAPP_IM_CONTENT_DISP_MESSAGE_VALUE[]         = "Message";

static const char _SAPP_IM_XML_HEADER[]                         = "<?xml version='1.0' encoding='UTF-8'?>";
static const char _SAPP_IM_XML_TAG_IMDN[]                       = "imdn";
static const char _SAPP_IM_XML_ATTR_XMLNS[]                     = "xmlns";
static const char _SAPP_IM_XML_ATTR_XMLNS_VALUE[]               = "urn:ietf:params:xml:ns:imdn";
static const char _SAPP_IM_XML_TAG_MSG_ID[]                     = "message-id";
static const char _SAPP_IM_XML_TAG_DATE_TIME[]                  = "datetime";
static const char _SAPP_IM_XML_TAG_RECIPIENT_URI[]              = "recipient-uri";
static const char _SAPP_IM_XML_TAG_ORG_RECIPIENT_URI[]          = "original-recipient-uri";
static const char _SAPP_IM_XML_TAG_DISPLAY_NOTI[]               = "display-notification";
static const char _SAPP_IM_XML_TAG_STATUS[]                     = "status";
static const char _SAPP_IM_XML_TAG_SUBJECT[]                    = "subject";
static const char _SAPP_IM_XML_TAG_DELIVERY_NOTI[]              = "delivery-notification";
static const char _SAPP_IM_XML_TAG_PROCESSING_NOTI[]            = "processing-notification";

#define _SAPP_CPIM_IS_DELIVERY_REPORT(x) (x &  \
        (ISI_MSG_RPT_DELIVERY_SUCCESS | ISI_MSG_RPT_DELIVERY_FAILED | \
        ISI_MSG_RPT_DELIVERY_FORBIDDEN | ISI_MSG_RPT_DELIVERY_ERROR))

#define _SAPP_CPIM_IS_DISPLAY_REPORT(x) (x &  \
        (ISI_MSG_RPT_DISPLAY_SUCCESS | ISI_MSG_RPT_DISPLAY_ERROR | \
        ISI_MSG_RPT_DISPLAY_FORBIDDEN))

#define _SAPP_CPIM_IS_PROCESSING_REPORT(x) (x &  \
        (ISI_MSG_RPT_PROCESSING_SUCCESS | ISI_MSG_RPT_PROCESSING_ERROR | \
        ISI_MSG_RPT_PROCESSING_FORBIDDEN | ISI_MSG_RPT_PROCESSING_STORED))

static SAPP_IntExt _SAPP_ReportRequest[12] = {
    { ISI_MSG_RPT_NONE,                 ""                  },
    { ISI_MSG_RPT_DELIVERY_SUCCESS,     "positive-delivery" },
    { ISI_MSG_RPT_DELIVERY_FAILED,      "negative-delivery" },
    { ISI_MSG_RPT_DELIVERY_FORBIDDEN,   "negative-delivery" },
    { ISI_MSG_RPT_DELIVERY_ERROR,       "negative-delivery" },
    { ISI_MSG_RPT_DISPLAY_SUCCESS,      "display"           },
    { ISI_MSG_RPT_DISPLAY_ERROR,        "display"           },
    { ISI_MSG_RPT_DISPLAY_FORBIDDEN,    "display"           },
    { ISI_MSG_RPT_PROCESSING_SUCCESS,   "processing"        },
    { ISI_MSG_RPT_PROCESSING_ERROR,     "processing"        },
    { ISI_MSG_RPT_PROCESSING_FORBIDDEN, "processing"        },
    { ISI_MSG_RPT_PROCESSING_STORED,    "processing"        },
};

static const char* _SAPP_cpimGetReportRequestInt2Ext(ISI_MessageReport report)
{
    vint x;
    vint size = sizeof(_SAPP_ReportRequest) / sizeof(SAPP_IntExt);
    for (x = 0 ; x < size ; x++) {
        if (report == (ISI_MessageReport)_SAPP_ReportRequest[x].internal) {
            return _SAPP_ReportRequest[x].ext_ptr;
        }
    }
    return _SAPP_ReportRequest[0].ext_ptr;
}

static vint _SAPP_cpimGetReportRequestExt2Int(const char *report_ptr, vint reportSize)
{
    vint report = 0;
    vint x;
    vint size = sizeof(_SAPP_ReportRequest) / sizeof(SAPP_IntExt);
    for (x = 1 ; x < size ; x++) {
        if (0 != OSAL_strncasescan(report_ptr, reportSize, _SAPP_ReportRequest[x].ext_ptr)) {
            report |= _SAPP_ReportRequest[x].internal;
        }
    }
    return (report);
}

static SAPP_IntExt _SAPP_DeliveryReport[6] = {
    { ISI_MSG_RPT_NONE,                 ""              },
    { ISI_MSG_RPT_DELIVERY_SUCCESS,     "delivered"     },
    { ISI_MSG_RPT_DELIVERY_FAILED,      "failed"        },
    { ISI_MSG_RPT_DELIVERY_FORBIDDEN,   "forbidden"     },
    { ISI_MSG_RPT_DELIVERY_ERROR,       "error"         },
    { ISI_MSG_RPT_DELIVERY_FAILED,      "undeliverable" },
};

static SAPP_IntExt _SAPP_DisplayReport[4] = {
    { ISI_MSG_RPT_NONE,                 ""            },
    { ISI_MSG_RPT_DISPLAY_SUCCESS,      "displayed"   },
    { ISI_MSG_RPT_DISPLAY_ERROR,        "error"       },
    { ISI_MSG_RPT_DISPLAY_FORBIDDEN,    "forbidden"   },
};

static SAPP_IntExt _SAPP_ProcessingReport[5] = {
    { ISI_MSG_RPT_NONE,                 ""            },
    { ISI_MSG_RPT_PROCESSING_SUCCESS,   "processed"   },
    { ISI_MSG_RPT_PROCESSING_ERROR,     "error"       },
    { ISI_MSG_RPT_PROCESSING_FORBIDDEN, "forbidden"   },
    { ISI_MSG_RPT_PROCESSING_STORED,    "stored"      },
};

static const char* _SAPP_cpimGetDeliveryReportInt2Ext(ISI_MessageReport r)
{
    vint x;
    vint size = sizeof(_SAPP_DeliveryReport) / sizeof(SAPP_IntExt);
    for (x = 0 ; x < size ; x++) {
        if (r == (ISI_MessageReport)_SAPP_DeliveryReport[x].internal) {
            return _SAPP_DeliveryReport[x].ext_ptr;
        }
    }
    return NULL;
}

static const char* _SAPP_cpimGetDisplayReportInt2Ext(ISI_MessageReport r)
{
    vint x;
    vint size = sizeof(_SAPP_DisplayReport) / sizeof(SAPP_IntExt);
    for (x = 0 ; x < size ; x++) {
        if (r == (ISI_MessageReport)_SAPP_DisplayReport[x].internal) {
            return _SAPP_DisplayReport[x].ext_ptr;
        }
    }
    return NULL;
}

#if 0
static const char* _SAPP_cpimGetProcessingReportInt2Ext(ISI_MessageReport r)
{
    vint x;
    vint size = sizeof(_SAPP_ProcessingReport) / sizeof(SAPP_IntExt);
    for (x = 0 ; x < size ; x++) {
        if (r == _SAPP_ProcessingReport[x].internal) {
            return _SAPP_ProcessingReport[x].ext_ptr;
        }
    }
    return NULL;
}
#endif

static vint _SAPP_cpimGetDeliveryReportExt2Int(const char *report_ptr)
{
    vint x;
    vint size = sizeof(_SAPP_DeliveryReport) / sizeof(SAPP_IntExt);
    for (x = 0 ; x < size ; x++) {
        if (0 == OSAL_strcasecmp(report_ptr, _SAPP_DeliveryReport[x].ext_ptr)) {
            return _SAPP_DeliveryReport[x].internal;
        }
    }
    return _SAPP_DeliveryReport[0].internal;
}

static vint _SAPP_cpimGetDisplayReportExt2Int(const char *report_ptr)
{
    vint x;
    vint size = sizeof(_SAPP_DisplayReport) / sizeof(SAPP_IntExt);
    for (x = 0 ; x < size ; x++) {
        if (0 == OSAL_strcasecmp(report_ptr, _SAPP_DisplayReport[x].ext_ptr)) {
            return _SAPP_DisplayReport[x].internal;
        }
    }
    return _SAPP_DisplayReport[0].internal;
}

static vint _SAPP_cpimGetProcessingReportExt2Int(const char *report_ptr)
{
    vint x;
    vint size = sizeof(_SAPP_ProcessingReport) / sizeof(SAPP_IntExt);
    for (x = 0 ; x < size ; x++) {
        if (0 == OSAL_strcasecmp(report_ptr, _SAPP_ProcessingReport[x].ext_ptr)) {
            return _SAPP_ProcessingReport[x].internal;
        }
    }
    return _SAPP_ProcessingReport[0].internal;
}

static void _SAPP_generateRandomString(char *target_ptr, vint length) {
    static char _randomNumberCharTable[]=
            "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz.-";
    vint  x;
    uint8 c;
    OSAL_randomGetOctets(target_ptr, length);
    /* Now convert each byte to a character */
    for (x = 0 ; x <  length ; x++) {
        c = (uint8) target_ptr[x];
        c &= 0x3F; // Make the value 63 or less
        // Rewrite the value with a ascii char
        target_ptr[x] = _randomNumberCharTable[c];
    }
    /* NULL Terminate */
    target_ptr[x] = 0;
}

static vint _SAPP_cpimDispNotiValue(
    ISI_MessageReport report,
    char             *target_ptr,
    vint              maxTargetLen)
{
    char   *dispNoti_ptr;
    vint    bytes;
    uint16  r;

    dispNoti_ptr = target_ptr;

    if (report & ISI_MSG_RPT_DELIVERY_SUCCESS) {
        bytes = OSAL_snprintf(dispNoti_ptr, maxTargetLen,
                "%s,",  _SAPP_cpimGetReportRequestInt2Ext(ISI_MSG_RPT_DELIVERY_SUCCESS));
        if (bytes > maxTargetLen) {
            return (maxTargetLen);
        }
        dispNoti_ptr += bytes;
        maxTargetLen -= bytes;
    }

    r = (ISI_MSG_RPT_DELIVERY_FAILED | 
            ISI_MSG_RPT_DELIVERY_FORBIDDEN | ISI_MSG_RPT_DELIVERY_ERROR);
    if (r & report) {
        bytes = OSAL_snprintf(dispNoti_ptr, maxTargetLen,
                "%s,", _SAPP_cpimGetReportRequestInt2Ext(ISI_MSG_RPT_DELIVERY_ERROR));
        if (bytes > maxTargetLen) {
            return (maxTargetLen);
        }
        dispNoti_ptr += bytes;
        maxTargetLen -= bytes;
    }

    if (_SAPP_CPIM_IS_DISPLAY_REPORT(report)) {
        bytes = OSAL_snprintf(dispNoti_ptr, maxTargetLen,
                "%s,",  _SAPP_cpimGetReportRequestInt2Ext(ISI_MSG_RPT_DISPLAY_SUCCESS));
        if (bytes > maxTargetLen) {
            return (maxTargetLen);
        }
        dispNoti_ptr += bytes;
        maxTargetLen -= bytes;
    }

    if (_SAPP_CPIM_IS_PROCESSING_REPORT(report)) {
        bytes = OSAL_snprintf(dispNoti_ptr, maxTargetLen,
                "%s,", _SAPP_cpimGetReportRequestInt2Ext(ISI_MSG_RPT_PROCESSING_SUCCESS));
        if (bytes > maxTargetLen) {
            return (maxTargetLen);
        }
        dispNoti_ptr += bytes;
        maxTargetLen -= bytes;
    }

    if (dispNoti_ptr != target_ptr) {
        /* Then we write something, get rid of the last ',' */
        dispNoti_ptr--;
        *dispNoti_ptr = 0;
    }
    /* Return the number of bytes written */
    return (dispNoti_ptr - target_ptr);
}

/*
 * ======== _SAPP_cpimIsiEvt() ========
 *
 * This function is used by various other functions to populate a ISI event
 * for "im" (instant messaging) related events. These events will be passed
 * from SAPP to the ISI module.
 *
 * Returns:
 *   Nothing.
 */
static void _SAPP_cpimIsiEvt(
    ISI_Id              serviceId,
    vint                protocolId,
    ISIP_TextReason     reason,
    ISIP_Message       *isi_ptr)
{
    isi_ptr->id = 0;
    isi_ptr->code = ISIP_CODE_MESSAGE;
    isi_ptr->protocol = protocolId;
    isi_ptr->msg.message.reason = reason;
    isi_ptr->msg.message.serviceId = serviceId;
    isi_ptr->msg.message.chatId = 0;
return;
}

vint SAPP_cpimEncodeIsComposing(
    const char       *to_ptr,
    const char       *from_ptr,
    char            **target_ptr,
    vint             *targetLen_ptr)
{
    char scratch[128];
    vint scratchSize = 127;
    if (SAPP_OK != SAPP_parseAddAddressHf(SAPP_FROM_HF, from_ptr,
            target_ptr, targetLen_ptr)) {
        return (SAPP_ERR);
    }

    if (SAPP_OK != SAPP_parseAddAddressHf(SAPP_TO_HF, to_ptr,
            target_ptr, targetLen_ptr)) {
        return (SAPP_ERR);
    }

    /* Set the date and time per ISO 8601. */
    if (OSAL_SUCCESS == OSAL_timeGetISO8601((uint8*)scratch, &scratchSize)) {
        if (SAPP_OK != SAPP_parseAddHf(SAPP_DATETIME_HF, scratch,
                target_ptr, targetLen_ptr)) {
            return (SAPP_ERR);
        }
    }
    /* 
     * Add Eol before content-type: application/im-iscomposing+xml. 
     * This Eol is for 3rd party hotfix client to well parse the isComposing event.
     */
    if (SAPP_OK != SAPP_parseAddEol(target_ptr, targetLen_ptr)) {
        return (SAPP_ERR);    
    }
        
    if (SAPP_OK != SAPP_parseAddHf(SAPP_CONTENT_TYPE_HF,
        _SAPP_IM_CONTENT_TYPE_HF_VALUE_COMPOSING, target_ptr, targetLen_ptr)) {
        return (SAPP_ERR);
    }

    /* Add Eol after content-type: application/im-iscomposing+xml. */
    if (SAPP_OK != SAPP_parseAddEol(target_ptr, targetLen_ptr)) {
        return (SAPP_ERR);    
    }
    return (SAPP_OK);
}

vint SAPP_cpimEncodeIm(
    const char       *to_ptr,
    const char       *from_ptr,
    ISI_MessageReport report,
    const char       *messageId_ptr,
    const char       *msg_ptr,
    char            **target_ptr,
    vint             *targetLen_ptr)
{
    char scratch[128];
    vint scratchSize = 127;
    
    if (SAPP_OK != SAPP_parseAddAddressHf(SAPP_FROM_HF, from_ptr,
            target_ptr, targetLen_ptr)) {
        return (SAPP_ERR);
    }

    if (SAPP_OK != SAPP_parseAddAddressHf(SAPP_TO_HF, to_ptr,
            target_ptr, targetLen_ptr)) {
        return (SAPP_ERR);
    }

    if (SAPP_OK != SAPP_parseAddHf(SAPP_CONTENT_DISP_HF,
            _SAPP_IM_CONTENT_DISP_MESSAGE_VALUE, target_ptr, targetLen_ptr)) {
        return (SAPP_ERR);
    }

    /* Set the date and time per ISO 8601. */
    if (OSAL_SUCCESS == OSAL_timeGetISO8601((uint8*)scratch, &scratchSize)) {
        if (SAPP_OK != SAPP_parseAddHf(SAPP_DATETIME_HF, scratch,
                target_ptr, targetLen_ptr)) {
            return (SAPP_ERR);
        }
    }

    if (ISI_MSG_RPT_NONE != report &&
            NULL != messageId_ptr && 0 != *messageId_ptr) {
        if (SAPP_OK != SAPP_parseAddHf(_SAPP_IM_NS_HF, _SAPP_IM_NS_HF_VALUE,
                target_ptr, targetLen_ptr)) {
            return (SAPP_ERR);
        }
        if (SAPP_OK != SAPP_parseAddHf(_SAPP_IM_MSG_ID_HF, messageId_ptr,
                target_ptr, targetLen_ptr)) {
            return (SAPP_ERR);
        }
    }

    if (0 != _SAPP_cpimDispNotiValue(report, scratch, 127)) {
        if (SAPP_OK != SAPP_parseAddHf(_SAPP_IM_DISP_NOTI_HF,
                scratch, target_ptr, targetLen_ptr)) {
            return (SAPP_ERR);
        }
    }
    
    /* let's add the encapsulated CPIM object, add new line then Content-Type. */
    if (SAPP_OK != SAPP_parseAddEol(target_ptr, targetLen_ptr)) {
        return (SAPP_ERR);
    }
    
    if (SAPP_OK != SAPP_parseAddHf(SAPP_CONTENT_TYPE_HF,
            _SAPP_IM_CONTENT_TYPE_HF_VALUE_TEXT, target_ptr, targetLen_ptr)) {
       return (SAPP_ERR);
    }

    OSAL_snprintf(scratch, 127, "%d", OSAL_strlen(msg_ptr));
    if (SAPP_OK != SAPP_parseAddHf(SAPP_CONTENT_LENGTH_HF, scratch,
            target_ptr, targetLen_ptr)) {
        return (SAPP_ERR);
    }

    /* Finally, let's add the message itself */
    if (SAPP_OK != SAPP_parseAddEol(target_ptr, targetLen_ptr)) {
        return (SAPP_ERR);
    }

    if (SAPP_OK != SAPP_parseAddPayload(msg_ptr, target_ptr, targetLen_ptr, 0)) {
        return (SAPP_ERR);
    }
    return (SAPP_OK);
}

static vint _SAPP_cpimXmlNotification(
    const char        *from_ptr,
    ISI_MessageReport  report,
    const char        *messageId_ptr,
    const char        *datetime_ptr,
    char             **out_ptr,
    vint              *outLen_ptr)
{
    ezxml_t            xml_ptr;
    ezxml_t            stat_ptr;
    ezxml_t            child_ptr;
    char              *str_ptr;
    char              *buff_ptr;
    vint               bytes;
    vint               size;

    xml_ptr = ezxml_new(_SAPP_IM_XML_TAG_IMDN);
    /* The 'name space' MUST exist */
    ezxml_set_attr(xml_ptr, _SAPP_IM_XML_ATTR_XMLNS, _SAPP_IM_XML_ATTR_XMLNS_VALUE);

    /* Add 'message-id' */
    child_ptr = ezxml_add_child(xml_ptr, _SAPP_IM_XML_TAG_MSG_ID, 0);
    ezxml_set_attr(child_ptr, _SAPP_IM_XML_ATTR_XMLNS, _SAPP_IM_XML_ATTR_XMLNS_VALUE);
    ezxml_set_txt(child_ptr, messageId_ptr);
    
    /* Add 'datetime' */
    child_ptr = ezxml_add_child(xml_ptr, _SAPP_IM_XML_TAG_DATE_TIME, 0);
    ezxml_set_attr(child_ptr, _SAPP_IM_XML_ATTR_XMLNS, _SAPP_IM_XML_ATTR_XMLNS_VALUE);
    ezxml_set_txt(child_ptr, datetime_ptr);
    
    str_ptr = NULL;
    if (_SAPP_CPIM_IS_DELIVERY_REPORT(report)) {
        /* Then it's a delivery report */
        child_ptr = ezxml_add_child(xml_ptr, _SAPP_IM_XML_TAG_DELIVERY_NOTI, 0);
        ezxml_set_attr(child_ptr, _SAPP_IM_XML_ATTR_XMLNS, _SAPP_IM_XML_ATTR_XMLNS_VALUE);
        str_ptr = (char*)_SAPP_cpimGetDeliveryReportInt2Ext(report);
    }
    else if (_SAPP_CPIM_IS_DISPLAY_REPORT(report)) {
        /* Then it's a display report */
        child_ptr = ezxml_add_child(xml_ptr, _SAPP_IM_XML_TAG_DISPLAY_NOTI, 0);
        ezxml_set_attr(child_ptr, _SAPP_IM_XML_ATTR_XMLNS, _SAPP_IM_XML_ATTR_XMLNS_VALUE);
        str_ptr = (char*)_SAPP_cpimGetDisplayReportInt2Ext(report);
    }
    else {
        /* Then we can't populate this report */
        ezxml_free(xml_ptr);
        return (SAPP_ERR);
    }

    if (NULL == str_ptr) {
        /* Then we can't populate this report */
        ezxml_free(xml_ptr);
        return (SAPP_ERR);
    }
    stat_ptr = ezxml_add_child(child_ptr, _SAPP_IM_XML_TAG_STATUS, 0);
    ezxml_add_child(stat_ptr, str_ptr, 0);

    if (NULL == (str_ptr = ezxml_toxml(xml_ptr))) {
        ezxml_free(xml_ptr);
        return (SAPP_ERR);
    }
    ezxml_free(xml_ptr);

    /* Let's prepend the XML doc header and the XML document contents. */
    size = OSAL_strlen(str_ptr) + sizeof(_SAPP_IM_XML_HEADER) +
            sizeof(SAPP_END_OF_LINE) + 1;
    /* Get some space for this */
    if (NULL == (buff_ptr = OSAL_memAlloc(size, OSAL_MEM_ARG_DYNAMIC_ALLOC))) {
        OSAL_memFree(str_ptr, OSAL_MEM_ARG_DYNAMIC_ALLOC);
        return (SAPP_ERR);
    }

    bytes = OSAL_snprintf(buff_ptr, size, "%s%s%s",
            _SAPP_IM_XML_HEADER, SAPP_END_OF_LINE, str_ptr);
    OSAL_memFree(str_ptr, OSAL_MEM_ARG_DYNAMIC_ALLOC);
    if (bytes > size) {
        /* This should never happen, but let's protect anyway */
        bytes = size;
    }
    *outLen_ptr = bytes;
    *out_ptr = buff_ptr;
    return (SAPP_OK);
}

vint SAPP_cpimEncodeImNotification(
    const char       *to_ptr,
    const char       *from_ptr,
    ISI_MessageReport report,
    const char       *messageId_ptr,
    char             *target_ptr,
    vint              maxTargetLen)
{
    char scratch[128];
    vint scratchSize = 127;
    vint bytes;
    char *doc_ptr;
    vint  docLen;
    char *start_ptr = target_ptr;
        
#ifdef CPIM_PAGE_USE_ANONYMITY
    if (SAPP_OK != SAPP_parseAddAddressHf(SAPP_FROM_HF, SAPP_ANONYMOUS_URI,
            &target_ptr, &maxTargetLen)) {
        return (0);
    }
    
    if (SAPP_OK != SAPP_parseAddAddressHf(SAPP_TO_HF, SAPP_ANONYMOUS_URI,
            &target_ptr, &maxTargetLen)) {
        return (0);
    }
#else
    if (SAPP_OK != SAPP_parseAddAddressHf(SAPP_FROM_HF, from_ptr,
            &target_ptr, &maxTargetLen)) {
        return (0);
    }

    if (SAPP_OK != SAPP_parseAddAddressHf(SAPP_TO_HF, to_ptr,
            &target_ptr, &maxTargetLen)) {
        return (0);
    }
#endif

    if (SAPP_OK != SAPP_parseAddHf(_SAPP_IM_NS_HF, _SAPP_IM_NS_HF_VALUE,
            &target_ptr, &maxTargetLen)) {
        return (0);
    }

    /*
     *  The message ID here can be anything.  The message ID of the message
     * we are notifing on will be in the XML document
     */
    _SAPP_generateRandomString(scratch, 16);
    if (SAPP_OK != SAPP_parseAddHf(_SAPP_IM_MSG_ID_HF, scratch,
            &target_ptr, &maxTargetLen)) {
        return (0);
    }

    /* Set the date and time per ISO 8601. */
    if (OSAL_SUCCESS != OSAL_timeGetISO8601((uint8*)scratch, &scratchSize)) {
        scratch[0] = 0;
    }
    else {
        if (SAPP_OK != SAPP_parseAddHf(SAPP_DATETIME_HF,
                scratch, &target_ptr, &maxTargetLen)) {
            return (0);
        }
    }
    
    /* let's add the encapsulated CPIM object, add new line */
    if (SAPP_OK != SAPP_parseAddEol(&target_ptr, &maxTargetLen)) { 
        return (SAPP_ERR);
    }

    if (SAPP_OK != SAPP_parseAddHf(SAPP_CONTENT_DISP_HF,
            _SAPP_IM_CONTENT_DISP_HF_VALUE, &target_ptr, &maxTargetLen)) {
        return (0);
    }

    if (SAPP_OK != SAPP_parseAddHf(SAPP_CONTENT_TYPE_HF,
            _SAPP_IM_CONTENT_TYPE_HF_VALUE_IMDN, &target_ptr, &maxTargetLen)) {
        return (0);
    }

    /* The scratch value here is the Date and Time. */
    if (SAPP_OK != _SAPP_cpimXmlNotification(from_ptr, report,
            messageId_ptr, scratch, &doc_ptr, &docLen)) {
        return (0);
    }
    
    /* Add the content-length */
    OSAL_snprintf(scratch, 127, "%d", docLen);
    if (SAPP_OK != SAPP_parseAddHf(SAPP_CONTENT_LENGTH_HF, scratch,
            &target_ptr, &maxTargetLen)) {
        OSAL_memFree(doc_ptr, 0);
        return (0);
    }
    /* Add the xml document */
    bytes = OSAL_snprintf(target_ptr, maxTargetLen,
        "%s%s", SAPP_END_OF_LINE, doc_ptr);
    if (bytes > maxTargetLen) {
        /* Then things were truncated, don't allow it */
        OSAL_memFree(doc_ptr, 0);
        return (0);
    }
    OSAL_memFree(doc_ptr, 0);
    target_ptr += bytes;
    return (target_ptr - start_ptr);
}


/*
 * ======== SAPP_imCpimDecodeTextPlain() ========
 * This routine will process the body of a CPIM message when the content-tpye
 * is text/plain.
 *
 * Returns:
 *  Nothing.
 */
void SAPP_cpimDecodeTextPlain(
    const char      *payload_ptr,
    vint             payloadLen,
    ISIP_Text       *isi_ptr)
{
    char   *end_ptr;
    vint    size;

    /* Get text message */
    if (NULL != (end_ptr = OSAL_strscan(payload_ptr, SAPP_END_OF_LINE))) {
        /* Let's figure out how much to copy. */
        size = end_ptr - payload_ptr;
    }
    else {
        size = payloadLen;
    }
    /* Let's copy, this will protect against overflow. */
    SAPP_parseCopy(isi_ptr->message, sizeof(isi_ptr->message), payload_ptr, size);
    return;
}

static vint _SAPP_cpimDecodeImdnDoc(
    char        *doc_ptr,
    vint         docLen,
    ISIP_Text   *isi_ptr)
{
    ezxml_t     child_ptr;
    ezxml_t      xml_ptr;
    char        *value_ptr;

    if (NULL == (xml_ptr = ezxml_parse_str(doc_ptr,  docLen))) {
        return (SAPP_ERR);
    }

    /* Check for the mandatory 'imdn' root tag */
    if (NULL == (value_ptr = ezxml_name(xml_ptr))) {
        ezxml_free(xml_ptr);
        return (SAPP_ERR);
    }
    if (0 != OSAL_strncmp(value_ptr, _SAPP_IM_XML_TAG_IMDN,
            sizeof(_SAPP_IM_XML_TAG_IMDN - 1))) {
        ezxml_free(xml_ptr);
        return (SAPP_ERR);
    }

    /* Let's get the notification type, if we don't understand it then ignore it */
    if (NULL != (child_ptr = ezxml_child(xml_ptr, _SAPP_IM_XML_TAG_DELIVERY_NOTI))) {
        /* Let's get the 'status' under the notifiction type and the first tag under 'status' */
        child_ptr = ezxml_child(child_ptr, _SAPP_IM_XML_TAG_STATUS);
        if (NULL == child_ptr || NULL == child_ptr->child) {
            ezxml_free(xml_ptr);
            return (SAPP_ERR);
        }
        isi_ptr->report = 
            (ISI_MessageReport)_SAPP_cpimGetDeliveryReportExt2Int(child_ptr->child->name);
    }
    else if (NULL != (child_ptr = ezxml_child(xml_ptr, _SAPP_IM_XML_TAG_DISPLAY_NOTI))) {
        /* Let's get the 'status' under the notifiction type and the first tag under 'status' */
        child_ptr = ezxml_child(child_ptr, _SAPP_IM_XML_TAG_STATUS);
        if (NULL == child_ptr || NULL == child_ptr->child) {
            ezxml_free(xml_ptr);
            return (SAPP_ERR);
        }
        isi_ptr->report = 
            (ISI_MessageReport)_SAPP_cpimGetDisplayReportExt2Int(child_ptr->child->name);
    }
    else if (NULL != (child_ptr = ezxml_child(xml_ptr, _SAPP_IM_XML_TAG_PROCESSING_NOTI))) {
        /* Let's get the 'status' under the notifiction type and the first tag under 'status' */
        child_ptr = ezxml_child(child_ptr, _SAPP_IM_XML_TAG_STATUS);
        if (NULL == child_ptr || NULL == child_ptr->child) {
            ezxml_free(xml_ptr);
            return (SAPP_ERR);
        }
        isi_ptr->report = 
            (ISI_MessageReport)_SAPP_cpimGetProcessingReportExt2Int(child_ptr->child->name);
    }
    else {
        ezxml_free(xml_ptr);
        return (SAPP_ERR);
    }

    /* Let's get the mandatory 'message-id' */
    if (NULL == (child_ptr = ezxml_child(xml_ptr, _SAPP_IM_XML_TAG_MSG_ID))) {
        ezxml_free(xml_ptr);
        return (SAPP_ERR);
    }
    if (NULL == (value_ptr = ezxml_txt(child_ptr))) {
        ezxml_free(xml_ptr);
        return (SAPP_ERR);
    }
    /* Let's copy the message-id field */
    OSAL_strncpy(isi_ptr->messageId, value_ptr, ISI_ID_STRING_SZ);

    /* Let's get the mandatory 'datetime' tag if it exists */
    if (NULL == (child_ptr = ezxml_child(xml_ptr, _SAPP_IM_XML_TAG_DATE_TIME))) {
        ezxml_free(xml_ptr);
        return (SAPP_ERR);
    }
    if (NULL == (value_ptr = ezxml_txt(child_ptr))) {
        ezxml_free(xml_ptr);
        return (SAPP_ERR);
    }
    OSAL_strncpy(isi_ptr->dateTime, value_ptr, ISI_DATE_TIME_STRING_SZ);

    /* Let's get the optional 'recipient-uri' tag if it exists */
    if (NULL != (child_ptr = ezxml_child(xml_ptr, _SAPP_IM_XML_TAG_RECIPIENT_URI))) {
        if (NULL != (value_ptr = ezxml_txt(child_ptr))) {
            OSAL_strncpy(isi_ptr->from, value_ptr, ISI_ADDRESS_STRING_SZ);
        }
    }

    /*
     * Check for 'original-recipient-uri' tag if it exists. If this exists
     * then this information is better (more accurate) than the 'recipient-uri'
     * field. So we will end up rewriting the 'from' value if there was a
     * 'recipient-uri' field.
     */
    if (NULL != (child_ptr = ezxml_child(xml_ptr, _SAPP_IM_XML_TAG_ORG_RECIPIENT_URI))) {
        if (NULL != (value_ptr = ezxml_txt(child_ptr))) {
            OSAL_strncpy(isi_ptr->from, value_ptr, ISI_ADDRESS_STRING_SZ);
        }
    }
    /* Let's get the optional 'subject' tag if it exists */
    if (NULL != (child_ptr = ezxml_child(xml_ptr, _SAPP_IM_XML_TAG_SUBJECT))) {
        if (NULL != (value_ptr = ezxml_txt(child_ptr))) {
            OSAL_strncpy(isi_ptr->subject, value_ptr, ISI_SUBJECT_STRING_SZ);
        }
    }
    ezxml_free(xml_ptr);
    return (SAPP_OK);
}

static vint _SAPP_cpimGetDocument(
    const char    *payload_ptr,
    const vint     payloadSize,
    char         **doc_ptr,
    vint          *docLen_ptr)
{
    char          *document_ptr;
    char          *start_ptr;
    char          *end_ptr;
    vint           size;
    vint           left;
        
    left = payloadSize;
    
    /* Documet start from Content-Type */
    if (NULL == (document_ptr = OSAL_strncasescan(payload_ptr, left, SAPP_CONTENT_TYPE_HF))) {
        /* Nothing there */
        return (SAPP_ERR);
    }

    /* Get the start and stop of the part of the payload to process */
    if (NULL == (start_ptr = OSAL_strnscan(document_ptr, left, SAPP_END_OF_DOC))) {
        /* Nothing there */
        return (SAPP_ERR);
    }
    /* Advance off the \r\n\r\n */
    start_ptr += 4;
     /* update the payload size */
    left -= (start_ptr - payload_ptr);

    /* Get the end of the doc */
    if (NULL == (end_ptr = OSAL_strnscan(start_ptr, left, SAPP_END_OF_DOC))) {
        /* Set the payload pointer to the end */
        size = left;
        payload_ptr = (start_ptr + left);
        left = 0;
    }
    else {
        payload_ptr = end_ptr + 4;
        size = (end_ptr - start_ptr);
        left -= (payload_ptr - start_ptr);
    }
    if (0 == size) {
        return (SAPP_ERR);
    }
    *doc_ptr = start_ptr;
    *docLen_ptr = size;
    return (SAPP_OK);
}

#if 0
/*
 * YTL: After YTL then we no longer have to check if there's a "friend request".
 */
static vint _SAPP_cpimDecodeFriend(
    char          *payload_ptr,
    vint           payloadSize,
    ISIP_Message  *isi_ptr)
{
    vint            size;
    char           *value_ptr;
    ISIP_PresReason presReason;

    if (SAPP_OK != _SAPP_cpimGetDocument(payload_ptr, payloadSize, &value_ptr, &size)) {
        return (SAPP_ERR);
    }

    if (SAPP_OK != SIMPL_friendParsePayload(NULL, value_ptr, size, &presReason)) {
        return (SAPP_ERR);
    }
    /* Othereise it's a friend request */

    /* Let's get the 'from' field */
    if (SAPP_OK == SAPP_parsePayloadValue(payload_ptr, SAPP_FROM_HF, &value_ptr, &size)) {
        SAPP_parseStripDelimters(&value_ptr, &size, '<', '>');
        /* Let's copy, this will protect against overflow. */
        SAPP_parseCopy(isi_ptr->msg.presence.from,
                sizeof(isi_ptr->msg.presence.from), value_ptr, size);
    }
    /* Let's get the 'to' field */
    if (SAPP_OK == SAPP_parsePayloadValue(payload_ptr, SAPP_TO_HF, &value_ptr, &size)) {
        SAPP_parseStripDelimters(&value_ptr, &size, '<', '>');
        /* Let's copy, this will protect against overflow. */
        SAPP_parseCopy(isi_ptr->msg.presence.to,
                sizeof(isi_ptr->msg.presence.to), value_ptr, size);

    }
    isi_ptr->msg.presence.reason = presReason;
    return (SAPP_OK);
}
#endif

static vint _SAPP_cpimDecodeIm(
    SAPP_ServiceObj *service_ptr,
    char            *contentType_ptr,
    vint             contentTypeSize,
    char            *payload_ptr,
    vint             payloadSize,
    ISIP_Text       *text_ptr)
{
    vint           size;
    char          *value_ptr;

    if (SAPP_CONTENT_TEXT_PLAIN != SAPP_parseIntFileType(
            contentType_ptr, contentTypeSize)) {
        /* Then it's not for us to process */
        return (SAPP_ERR);
    }
#ifdef INCLUDE_SIMPLE
#if 0 /* This needs to be removed with the rest of friend request stuff when the time comes. */
    if (SAPP_OK == _SAPP_cpimDecodeFriend(payload_ptr, payloadSize, isi_ptr)) {
        /* Then we are a YTL 'friend request' */
        /* Let's populate the rest of the ISI message for a friend request*/
        SIMPL_isiEvt(0, service_ptr->isiServiceId,
                isi_ptr->msg.presence.reason, isi_ptr->msg.presence.to, isi_ptr->msg.presence.from, isi_ptr);
        return (SAPP_OK);
    }
#endif
#endif
    
    /* Let's get the 'from' field */
    if (SAPP_OK == SAPP_parsePayloadValue(payload_ptr, SAPP_FROM_HF, &value_ptr, &size)) {
        SAPP_parseStripDelimters(&value_ptr, &size, '<', '>');
        /* Check if the from is anonymous, if so then let's get a display name if there is one. */
        if (0 == OSAL_strncasecmp(SAPP_ANONYMOUS_URI, value_ptr, size)) {
            /* It's anonymous, let's get a display name. */
            SAPP_parseStripDelimters(&value_ptr, &size, '"', '"');
        }
        /* Whatever we have at this point, let's use it as the from. */

        /* Let's copy, this will protect against overflow. */
        SAPP_parseCopy(text_ptr->from, sizeof(text_ptr->from), value_ptr, size);
    }
    /* Let's get the 'to' field */
    if (SAPP_OK == SAPP_parsePayloadValue(payload_ptr, SAPP_TO_HF, &value_ptr, &size)) {
        SAPP_parseStripDelimters(&value_ptr, &size, '<', '>');
        /* Let's copy, this will protect against overflow. */
        SAPP_parseCopy(text_ptr->to, sizeof(text_ptr->to), value_ptr, size);
    }
    
    /* Let's look for any date and time info */
    if (SAPP_OK == SAPP_parsePayloadValue(payload_ptr, SAPP_DATETIME_HF,
            &value_ptr, &size)) {
        SAPP_parseCopy(text_ptr->dateTime, sizeof(text_ptr->dateTime), value_ptr, size);
    }

    /* Get any desired reports */
    if (SAPP_OK == SAPP_parsePayloadValue(payload_ptr, _SAPP_IM_DISP_NOTI_HF,
            &value_ptr, &size)) {
        text_ptr->report = 
            (ISI_MessageReport)_SAPP_cpimGetReportRequestExt2Int(value_ptr, size);
    }

    /* Get Message ID */
    if (SAPP_OK == SAPP_parsePayloadValue(payload_ptr, _SAPP_IM_MSG_ID_HF,
            &value_ptr, &size)) {
        SAPP_parseCopy(text_ptr->messageId, sizeof(text_ptr->messageId), value_ptr, size);
    }

    if (SAPP_OK == _SAPP_cpimGetDocument(payload_ptr, payloadSize, &value_ptr, &size)) {
        SAPP_cpimDecodeTextPlain(value_ptr, size, text_ptr);
        /* Let's populate the rest of the ISI message */
        //_SAPP_cpimIsiEvt(service_ptr->isiServiceId,
        //        ISIP_TEXT_REASON_NEW, isi_ptr);
    }
    return (SAPP_OK);
}

static vint _SAPP_cpimDecodeImdn(
    SAPP_ServiceObj *service_ptr,
    char            *contentType_ptr,
    vint             contentTypeSize,
    char            *payload_ptr,
    vint             payloadSize,
    ISIP_Message    *isi_ptr)
{
    vint           size;
    char          *value_ptr;

    value_ptr = NULL;

    /* Check for 'message/imdn+xml' which means it's a notification */
    if (NULL == OSAL_strncasescan(contentType_ptr, contentTypeSize,
            _SAPP_IM_CONTENT_TYPE_HF_VALUE_IMDN)) {
        /* Then it isn't an IMDN */
        return (SAPP_ERR);
    }
    /*
     * Let's also check the Content-Disposition to confirm it's a notification
     * if it's exist, if no Content-Disp exists then assume it's a notification
     */
    if (SAPP_OK == SAPP_parsePayloadValue(payload_ptr, SAPP_CONTENT_DISP_HF,
            &value_ptr, &size)) {
       if (NULL == OSAL_strncasescan(value_ptr, size,
               _SAPP_IM_CONTENT_DISP_HF_VALUE)) {
            /* Then we don't know what this is so let's ignore */
            return (SAPP_OK);
        }
    }

    /* Let's get the 'from' field */
    if (SAPP_OK == SAPP_parsePayloadValue(payload_ptr, SAPP_FROM_HF, &value_ptr, &size)) {
        SAPP_parseStripDelimters(&value_ptr, &size, '<', '>');
        SAPP_parseCopy(isi_ptr->msg.message.from,
                sizeof(isi_ptr->msg.message.from), value_ptr, size);
    }
    /* Let's get the 'to' field */
    if (SAPP_OK == SAPP_parsePayloadValue(payload_ptr, SAPP_TO_HF, &value_ptr, &size)) {
        SAPP_parseStripDelimters(&value_ptr, &size, '<', '>');
        SAPP_parseCopy(isi_ptr->msg.message.to, sizeof(isi_ptr->msg.message.to),
                value_ptr, size);
    }

    /* Let's look for any date and time info */
    if (SAPP_OK == SAPP_parsePayloadValue(payload_ptr, SAPP_DATETIME_HF,
            &value_ptr, &size)) {
        SAPP_parseCopy(isi_ptr->msg.message.dateTime,
                sizeof(isi_ptr->msg.message.dateTime), value_ptr, size);
    }

    if (SAPP_OK == _SAPP_cpimGetDocument(payload_ptr, payloadSize, &value_ptr, &size)) {
        if (SAPP_OK == _SAPP_cpimDecodeImdnDoc(value_ptr, size, &isi_ptr->msg.message)) {
            /* Let's populate the rest of the ISI message */
            _SAPP_cpimIsiEvt(service_ptr->isiServiceId, service_ptr->protocolId,
                    ISIP_TEXT_REASON_REPORT, isi_ptr);
        }
    }
    return (SAPP_OK);
}

static vint _SAPP_cpimDecodeImdnMultiPart(
    SAPP_ServiceObj *service_ptr,
    char            *contentType_ptr,
    vint             contentTypeSize,
    char            *payload_ptr,
    vint             payloadSize,
    ISIP_Message    *isi_ptr)
{
    /* Check for 'message/imdn+xml' which means it's a notification */
    if (NULL == OSAL_strncasescan(contentType_ptr, contentTypeSize,
            _SAPP_IM_CONTENT_TYPE_HF_VALUE_IMDN)) {
        /* Then it isn't an IMDN */
        return (SAPP_ERR);
    }

    if (SAPP_OK == _SAPP_cpimDecodeImdnDoc(payload_ptr, payloadSize,
            &isi_ptr->msg.message)) {
        /* Let's populate the rest of the ISI message */
        _SAPP_cpimIsiEvt(service_ptr->isiServiceId, service_ptr->protocolId,
                ISIP_TEXT_REASON_REPORT, isi_ptr);
    }
    return (SAPP_OK);
}

static vint _SAPP_cpimDecodeMulipartMixed(
    SAPP_ServiceObj   *service_ptr,
    char              *contentType_ptr,
    vint               contentTypeSize,
    char              *payload_ptr,
    vint               payloadLen,
    SAPP_Event        *evt_ptr)
{
    char         boundary_ary[SAPP_STRING_SZ];
    char        *start_ptr;
    char        *end_ptr;
    char        *hf_ptr;
    vint         size;
    vint         hfLen;
    vint         len;
    vint         skipBytes;
    OSAL_Boolean notDone;

    if (SAPP_CONTENT_MULTI_PART != SAPP_parseIntFileType(contentType_ptr,
            contentTypeSize)) {
        /* Then this doesn't belong to us */
        return (SAPP_ERR);
    }

    /* Advance to the next payload */
    SAPP_parseAdvance(&payload_ptr, &payloadLen, SAPP_END_OF_DOC);
    
    /* Get the 'boundary=' value in the content-type */
    if (0 == (len = SAPP_parseBoundry(contentType_ptr, contentTypeSize,
            boundary_ary, SAPP_STRING_SZ))) {
        /* No boundry! let's bail */
        return (SAPP_OK);
    }
    
    /* Find all Content-Types and process */
    while (SAPP_OK == SAPP_parsePayloadValue(payload_ptr,
            SAPP_CONTENT_TYPE_HF, &hf_ptr, &hfLen)) {

        /* Get the start and stop of the part of the payload to process */
        if (NULL == (start_ptr = OSAL_strnscan(payload_ptr, payloadLen,
                SAPP_END_OF_DOC))) {
            break;
        }
        /* Advance off the \r\n\r\n */
        start_ptr += 4;
         /* update the payload size */
        payloadLen -= (start_ptr - payload_ptr);

        notDone = OSAL_TRUE;
        skipBytes = 0;

        while (OSAL_TRUE == notDone) {
            if (NULL == (end_ptr = OSAL_strnscan(start_ptr + skipBytes,
                    payloadLen - skipBytes, boundary_ary))) {
                /* Set the payload pointer to the end */
                payload_ptr = (start_ptr + payloadLen);
                size = payloadLen;
                payloadLen = 0;
                notDone = OSAL_FALSE;
            }
            else if (0 == OSAL_strncmp(end_ptr - 4,"\r\n--",4)) {
                payload_ptr = end_ptr + len;
                size = (end_ptr - start_ptr) - 4;
                payloadLen -= (payload_ptr - start_ptr);
                notDone = OSAL_FALSE;
            }
            else if (0 == OSAL_strncmp(end_ptr - 2,"\r\n",2)) {
                payload_ptr = end_ptr + len;
                size = (end_ptr - start_ptr) - 2;
                payloadLen -= (payload_ptr - start_ptr);
                notDone = OSAL_FALSE;
            }
            else {
                skipBytes = (end_ptr - start_ptr) + len;
                notDone = OSAL_TRUE;
            }
        }

        if (SAPP_OK == _SAPP_cpimDecodeImdnMultiPart(service_ptr,
                hf_ptr, hfLen, start_ptr, size, &evt_ptr->isiMsg)) {
            SAPP_sendEvent(evt_ptr);
            /* Since we just sent an event to isi, let's clear the code */
            OSAL_memSet(evt_ptr, 0, sizeof(ISIP_Message));
            continue;
        }
        /* ADD MORE HERE IF NEED BE */
    }
    return (SAPP_OK);
}

static vint _SAPP_cpimDecodeIsComposing(
    SAPP_ServiceObj *service_ptr,
    char            *contentType_ptr,
    vint             contentTypeSize,
    char            *payload_ptr,
    vint             payloadSize,
    ISIP_Message    *isi_ptr)
{
    vint           size;
    char          *value_ptr;

    value_ptr = NULL;

    /* Check for 'application/im-iscomposing+xml' which means it's an isComposing event */
    if (NULL == OSAL_strncasescan(contentType_ptr, contentTypeSize,
            _SAPP_IM_CONTENT_TYPE_HF_VALUE_COMPOSING)) {
        /* Then it isn't an IMDN */
        return (SAPP_ERR);
    }
    /* Let's get the 'from' field */
    if (SAPP_OK == SAPP_parsePayloadValue(payload_ptr, SAPP_FROM_HF, &value_ptr, &size)) {
        SAPP_parseStripDelimters(&value_ptr, &size, '<', '>');
        SAPP_parseCopy(isi_ptr->msg.message.from,
                sizeof(isi_ptr->msg.message.from), value_ptr, size);
    }
    /* Let's get the 'to' field */
    if (SAPP_OK == SAPP_parsePayloadValue(payload_ptr, SAPP_TO_HF, &value_ptr, &size)) {
        SAPP_parseStripDelimters(&value_ptr, &size, '<', '>');
        SAPP_parseCopy(isi_ptr->msg.message.to, sizeof(isi_ptr->msg.message.to),
                value_ptr, size);
    }
    /* Let's look for any date and time info */
    if (SAPP_OK == SAPP_parsePayloadValue(payload_ptr, SAPP_DATETIME_HF,
            &value_ptr, &size)) {
        SAPP_parseCopy(isi_ptr->msg.message.dateTime,
                sizeof(isi_ptr->msg.message.dateTime), value_ptr, size);
    }
    return (SAPP_OK);
}

vint SAPP_cpimDecode(
    SAPP_ServiceObj *service_ptr,
    char            *payload_ptr,
    vint             payloadSize,
    SAPP_Event      *evt_ptr)
{
    vint  size;
    char *value_ptr;

    /*
     * Find the 'Content-Types' and process accordingly.  If it's a notification
     * then process like a notification, otherwise treat like a regular IM.
     */
    if (SAPP_OK != SAPP_parsePayloadValue(payload_ptr, SAPP_CONTENT_TYPE_HF,
            &value_ptr, &size)) {
        /* Then there's no content type...!!?? Let's ignore */
        return (SAPP_ERR);
    }

    if (SAPP_OK == _SAPP_cpimDecodeIm(service_ptr,
            value_ptr, size, payload_ptr, payloadSize, &evt_ptr->isiMsg.msg.message)) {
        // Populate the rest of the message.
        _SAPP_cpimIsiEvt(service_ptr->isiServiceId, service_ptr->protocolId,
                        ISIP_TEXT_REASON_NEW, &evt_ptr->isiMsg);
        /* Then it's been handled */
        return (SAPP_OK);
    }
    else if (SAPP_OK == _SAPP_cpimDecodeImdn(service_ptr, value_ptr, size,
            payload_ptr, payloadSize, &evt_ptr->isiMsg)) {
        /* Then it's been handled */
        return (SAPP_OK);
    }
    else if (SAPP_OK == _SAPP_cpimDecodeMulipartMixed(service_ptr,
            value_ptr, size, payload_ptr, payloadSize, evt_ptr)) {
        /* Then it's been handled */
        return (SAPP_OK);
    }
    else if (SAPP_OK == _SAPP_cpimDecodeIsComposing(service_ptr, value_ptr, size,
            payload_ptr, payloadSize, &evt_ptr->isiMsg)) {
        /* IsComposing event.*/
        return (SAPP_OK);
    }
    /* Add any more types here if need be */
    OSAL_logMsg("%s:%d unhandle cpim type\n", __FUNCTION__, __LINE__);
    return (SAPP_ERR);
}

vint SAPP_cpimDecodeMessageOnly(
    SAPP_ServiceObj *service_ptr,
    char            *payload_ptr,
    vint             payloadSize,
    ISIP_Text       *text_ptr)
{
    vint  size;
    char *value_ptr;

    /*
     * Find the 'Content-Types' and process accordingly.  If it's a notification
     * then process like a notification, otherwise treat like a regular IM.
     */
    if (SAPP_OK != SAPP_parsePayloadValue(payload_ptr, SAPP_CONTENT_TYPE_HF,
            &value_ptr, &size)) {
        /* Then there's no content type...!!?? Let's ignore */
        return (SAPP_ERR);
    }

    if (SAPP_OK == _SAPP_cpimDecodeIm(service_ptr,
            value_ptr, size, payload_ptr, payloadSize, text_ptr)) {
        /* Then it's been handled */
        return (SAPP_OK);
    }
    /* Add any more types here if need be */
    return (SAPP_ERR);
}

