/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev$ $Date$
 *
 */

#include <osal.h>
#include <isi.h>
#include <isip.h>
#include <csm_event.h>
#include <xcap.h>
#include <ezxml.h>
#include "_tapp.h"
#include "_tapp_report.h"
#include "_tapp_xml.h"
#include "_tapp_xml_xcap.h"


/* XXX split ISIP from _tapp_xml.c */
extern TAPP_EnumObj _TAPP_DataTypeTable[];

TAPP_EnumObj _TAPP_XcapCmdTable[] = {
    {XCAP_OPERATION_NONE,           "XCAP_OPERATION_NONE"},
    {XCAP_OPERATION_DELETE,         "XCAP_OPERATION_DELETE"},
    {XCAP_OPERATION_CREATE_REPLACE, "XCAP_OPERATION_CREATE_REPLACE"},
    {XCAP_OPERATION_FETCH,          "XCAP_OPERATION_FETCH"},
    {XCAP_OPERATION_INVALID,        "XCAP_OPERATION_INVALID"},
};

TAPP_EnumObj _TAPP_XcapOpTypeTable[] = {
    {XCAP_OPERATION_TYPE_NONE,      "XCAP_OPERATION_TYPE_NONE"},
    {XCAP_OPERATION_TYPE_DOCUMENT,  "XCAP_OPERATION_TYPE_DOCUMENT"},
    {XCAP_OPERATION_TYPE_ELEMENT,   "XCAP_OPERATION_TYPE_ELEMENT"},
    {XCAP_OPERATION_TYPE_ATTRIBUTE, "XCAP_OPERATION_TYPE_ATTRIBUTE"},
    {XCAP_OPERATION_TYPE_INVALID,   "XCAP_OPERATION_TYPE_INVALID"},
};

TAPP_EnumObj _TAPP_XcapEtagCondTable[] = {
    {XCAP_CONDITION_NONE,           "XCAP_CONDITION_NONE"},
    {XCAP_CONDITION_IF_MATCH,       "XCAP_CONDITION_IF_MATCH"},
    {XCAP_CONDITION_IF_NONE_MATCH,  "XCAP_CONDITION_IF_NONE_MATCH"},
    {XCAP_CONDITION_INVALID,        "XCAP_CONDITION_INVALID"},
};

TAPP_EnumObj _TAPP_XcapEvtErrTable[] = {
    {XCAP_EVT_ERR_NONE,             "XCAP_EVT_ERR_NONE"},
    {XCAP_EVT_ERR_NOMEM,            "XCAP_EVT_ERR_NOMEM"},
    {XCAP_EVT_ERR_NET,              "XCAP_EVT_ERR_NET"},
    {XCAP_EVT_ERR_HTTP,             "XCAP_EVT_ERR_HTTP"},
    {XCAP_EVT_ERR_LAST,             "XCAP_EVT_ERR_LAST"},
};

#define TAPP_XML_OPERATION_TAG      "operation"
#define TAPP_XML_OPTYPE_TAG         "optype"
#define TAPP_XML_CONDITION_TAG      "condition"
#define TAPP_XML_URI_TAG            "uri"
#define TAPP_XML_USERNAME_TAG       "username"
#define TAPP_XML_PASSWORD_TAG       "password"
#define TAPP_XML_X3GPP_TAG          "x3gpp"
#define TAPP_XML_AUID_TAG           "auid"
#define TAPP_XML_ETAG_TAG           "etag"
#define TAPP_XML_ERROR_TAG          "error"
#define TAPP_XML_HEADER_TAG         "header"
#define TAPP_XML_BODY_TAG           "body"

                        
/* XXX : This should be generic function used by all XAPPs */
extern TAPP_Return _TAPP_getIsipValueByTag(
    ezxml_t         xmlisip_ptr,
    void           *res_ptr,
    TAPP_DataType   type,
    char           *tag_ptr,
    TAPP_EnumObj   *enumString_ptr,
    int             enumSize,
    int             enumDefault);


/* sample xcap validate action, where xcap expecting csm to send command
<action type="validate xcap">
    <xcap>
        <command>
            <operation type="enum">XCAP_OPERATION_FETCH</operation>
            <optype type="enum">XCAP_OPERATION_TYPE_DOCUMENT</type>
            <condition type="enum">XCAP_CONDITION_NONE</condition>
            <uri type="string">https://xcap.example.com:/resource-lists/users/sip:joebloggs@example.com/index</uri>
            <username type="string">joebloggs</username>
            <password type="string">cleartext</password>
            <x3gpp type="string">d2tech</x3gpp>
            <auid type="string">d2tech</auid>
            <etag type="string">d2tech</etag>
        </command>
    </xcap>
</action>
*/

/*
 * ======== TAPP_xmlParseXcapCmd() ========
 *
 * function to parse xcap cmd tag
 *
 * Returns:
 *  TAPP_PASS: xcap tag parsed
 *  TAPP_FAIL: Failed to parse xcap tag
 */
TAPP_Return TAPP_xmlParseXcapCmd(
    ezxml_t     xml_ptr,
    TAPP_mockXcapCmd    *mockXcapCmd_ptr)
{
    if (TAPP_PASS != _TAPP_getIsipValueByTag(xml_ptr,&mockXcapCmd_ptr->op,
            TAPP_DATA_TYPE_ENUM, TAPP_XML_OPERATION_TAG,
            _TAPP_XcapCmdTable, XCAP_OPERATION_INVALID,
            XCAP_OPERATION_INVALID)) {
        return (TAPP_FAIL);
    }
    if (TAPP_PASS != _TAPP_getIsipValueByTag(xml_ptr,&mockXcapCmd_ptr->opType,
            TAPP_DATA_TYPE_ENUM, TAPP_XML_OPTYPE_TAG,
            _TAPP_XcapOpTypeTable, XCAP_OPERATION_TYPE_INVALID,
            XCAP_OPERATION_TYPE_INVALID)) {
        return (TAPP_FAIL);
    }
    if (TAPP_PASS != _TAPP_getIsipValueByTag(xml_ptr,&mockXcapCmd_ptr->cond,
            TAPP_DATA_TYPE_ENUM, TAPP_XML_CONDITION_TAG,
            _TAPP_XcapEtagCondTable, XCAP_CONDITION_INVALID,
            XCAP_CONDITION_NONE)) {
        return (TAPP_FAIL);
    }
    if (TAPP_PASS != _TAPP_getIsipValueByTag(xml_ptr,
            mockXcapCmd_ptr->uri,
            TAPP_DATA_TYPE_STRING,
            TAPP_XML_URI_TAG, NULL, 0, 0)) {
        return (TAPP_FAIL);
    }
    if (TAPP_PASS != _TAPP_getIsipValueByTag(xml_ptr,
            mockXcapCmd_ptr->username,
            TAPP_DATA_TYPE_STRING,
            TAPP_XML_USERNAME_TAG, NULL, 0, 0)) {
        return (TAPP_FAIL);
    }
    if (TAPP_PASS != _TAPP_getIsipValueByTag(xml_ptr,
            mockXcapCmd_ptr->password,
            TAPP_DATA_TYPE_STRING,
            TAPP_XML_PASSWORD_TAG, NULL, 0, 0)) {
        return (TAPP_FAIL);
    }
    if (TAPP_PASS != _TAPP_getIsipValueByTag(xml_ptr,
            mockXcapCmd_ptr->x3gpp,
            TAPP_DATA_TYPE_STRING,
            TAPP_XML_X3GPP_TAG, NULL, 0, 0)) {
        return (TAPP_FAIL);
    }
    if (TAPP_PASS != _TAPP_getIsipValueByTag(xml_ptr,
            mockXcapCmd_ptr->auid,
            TAPP_DATA_TYPE_STRING,
            TAPP_XML_AUID_TAG, NULL, 0, 0)) {
        return (TAPP_FAIL);
    }
    if (TAPP_PASS != _TAPP_getIsipValueByTag(xml_ptr,
            mockXcapCmd_ptr->etag,
            TAPP_DATA_TYPE_STRING,
            TAPP_XML_ETAG_TAG, NULL, 0, 0)) {
        return (TAPP_FAIL);
    }
    if (TAPP_PASS != _TAPP_getIsipValueByTag(xml_ptr,
            mockXcapCmd_ptr->body,
            TAPP_DATA_TYPE_STRING,
            TAPP_XML_BODY_TAG, NULL, 0, 0)) {
        return (TAPP_FAIL);
    }
    return (TAPP_PASS);
}


/* sample xcap issue action, where xcap return event to csm for further processing
<action type="issue xcap">
    <xcap>
        <event>
            <error type="enum">XCAP_EVT_ERR_NONE</error>
            <header type="string">http headers</header>
            <body type="string">xml body</body>
        </event>
    </xcap>
</action>
*/

/*
 * ======== TAPP_xmlParseXcapEvt() ========
 *
 * function to parse xcap evt tag
 *
 * Returns:
 *  TAPP_PASS: xcap evt tag parsed
 *  TAPP_FAIL: Failed to parse xcap evt tag
 */
TAPP_Return TAPP_xmlParseXcapEvt(
    ezxml_t     xml_ptr,
    TAPP_mockXcapEvt    *mockXcapEvt_ptr)
{
    if (TAPP_PASS != _TAPP_getIsipValueByTag(xml_ptr,&mockXcapEvt_ptr->error,
            TAPP_DATA_TYPE_ENUM, TAPP_XML_ERROR_TAG,
            _TAPP_XcapEvtErrTable, XCAP_EVT_ERR_LAST,
            XCAP_EVT_ERR_NONE)) {
        return (TAPP_FAIL);
    }
    if (TAPP_PASS != _TAPP_getIsipValueByTag(xml_ptr,
            mockXcapEvt_ptr->hdr,
            TAPP_DATA_TYPE_STRING,
            TAPP_XML_HEADER_TAG, NULL, 0, 0)) {
        return (TAPP_FAIL);
    }
    if (TAPP_PASS != _TAPP_getIsipValueByTag(xml_ptr,
            mockXcapEvt_ptr->body,
            TAPP_DATA_TYPE_STRING,
            TAPP_XML_BODY_TAG, NULL, 0, 0)) {
        return (TAPP_FAIL);
    }
    return (TAPP_PASS);
}

