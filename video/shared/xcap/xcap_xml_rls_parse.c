/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 13573 $ $Date: 2010-12-01 03:58:51 +0800 (Wed, 01 Dec 2010) $
 *
 */

#include <ezxml.h>
#include <osal.h>
#include "xcap_resources.h"
#include "xcap_xml_helper.h"

/* 
 * ======== XCAP_xmlRlsParseDocument() ========
 */
XCAP_Xml XCAP_xmlRlsParseDocument(
     char *doc_ptr,
     int   docLen)
{
    ezxml_t  xml_ptr;
    char    *str_ptr;

    xml_ptr = NULL;
    if (NULL != doc_ptr) {
        xml_ptr = ezxml_parse_str(doc_ptr,
                docLen);
    }
    if (NULL == xml_ptr) {
        return (NULL);
    }

    /*
     * Now verify that it meets RFC 4826. Namespace.
     */

    str_ptr = (char *)ezxml_attr(xml_ptr,
            (const char *)XCAP_XMLNS);
    if (NULL != str_ptr) {
        if (0 != OSAL_strcmp(XCAP_URN_IETF_PARAMS_XML_NS_RLS_SERVICES,
                str_ptr)) {
            ezxml_remove(xml_ptr);
            return (NULL);
        }
    }
    else {
        ezxml_remove(xml_ptr);
        return (NULL);
    }

#if 0 /* This is not needed really */
    str_ptr = (char *)ezxml_attr(xml_ptr,
            (const char *)XCAP_XMLNS_XSI);
    if (NULL != str_ptr) {
        if (0 != OSAL_strcmp(XCAP_HTTP_WWW_W3_ORG_2001_XMLSCHEMA_INSTANCE,
                str_ptr)) {
            ezxml_remove(xml_ptr);
            return (NULL);
        }
    }
    else {
        ezxml_remove(xml_ptr);
        return (NULL);
    }
#endif
    return ((XCAP_Xml)xml_ptr);
}

/* 
 * ======== XCAP_xmlRlsParseGetService() ========
 */
XCAP_Xml XCAP_xmlRlsParseGetService(
    XCAP_Xml   doc,
    int        serviceNumber,
    char     **uri_ptr)
{
    ezxml_t ret_ptr;

    if (NULL == doc) {
        return (NULL);
    }

    /*
     * Get a service
     */
    ret_ptr = ezxml_get((ezxml_t)doc,
            XCAP_SERVICE,
            serviceNumber,
            NULL);

    /*
     * Also return its URI
     */
    if ((NULL != ret_ptr) && (NULL != uri_ptr)) {
        *uri_ptr = (char *)ezxml_attr(ret_ptr,
                (const char *)XCAP_URI);
    }
    
    return ((XCAP_Xml)ret_ptr);
}

/* 
 * ======== XCAP_xmlRlsParseGetPackagesFromService() ========
 */
XCAP_Xml XCAP_xmlRlsParseGetPackagesFromService(
    XCAP_Xml service)
{
    if (NULL == service) {
        return (NULL);
    }

    /*
     * Get packages
     */
    return((XCAP_Xml)ezxml_get((ezxml_t)service,
            XCAP_PACKAGES,
            0,
            NULL));
}

/* 
 * ======== XCAP_xmlRlsParseGetPackageFromServicePackages() ========
 */
XCAP_Xml XCAP_xmlRlsParseGetPackageFromServicePackages(
    XCAP_Xml   packages,
    char     **packageName_ptr)
{
    ezxml_t ret_ptr;

    if (NULL == packages) {
        return (NULL);
    }

    /*
     * Get packages
     */
    ret_ptr = ezxml_get((ezxml_t)packages,
            XCAP_PACKAGE,
            0,
            NULL);
    if (NULL == ret_ptr) {
        return (NULL);
    }

    /*
     * Also get its value.
     */
    if (NULL != packageName_ptr) {
        *packageName_ptr = (char *)ezxml_txt(ret_ptr);
    }

    return ((XCAP_Xml)ret_ptr);
}

/* 
 * ======== XCAP_xmlRlsParseGetResourceListElementFromService() ========
 */
XCAP_Xml XCAP_xmlRlsParseGetResourceListElementFromService(
    XCAP_Xml   service,
    char     **uri_ptr)
{
    ezxml_t ret_ptr;

    if (NULL == service) {
        return (NULL);
    }

    /*
     * Get packages
     */
    ret_ptr = ezxml_get((ezxml_t)service,
            XCAP_RESOURCE_LIST,
            0,
            NULL);
    if (NULL == ret_ptr) {
        return (NULL);
    }

    /*
     * Also get its value.
     */
    if (NULL != uri_ptr) {
        *uri_ptr = (char *)ezxml_txt(ret_ptr);
    }

    return ((XCAP_Xml)ret_ptr);
}

