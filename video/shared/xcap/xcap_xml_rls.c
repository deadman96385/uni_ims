/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 12486 $ $Date: 2010-07-08 06:10:49 +0800 (Thu, 08 Jul 2010) $
 *
 */

#include <ezxml.h>
#include "xcap_resources.h"
#include "xcap_xml_helper.h"

/* 
 * ======== XCAP_xmlRlsCreateDocument() ========
 */
XCAP_Xml XCAP_xmlRlsCreateDocument(
    void)
{
    ezxml_t xml_ptr;
    
    /*
     * Create a new RLS services
     */
    if (NULL == (xml_ptr = ezxml_new(XCAP_RLS_SERVICES))) {
        return (NULL);
    }
    
    /*
     * Set XML namespaces
     */
    if (NULL == ezxml_set_attr(xml_ptr,
            XCAP_XMLNS,
            XCAP_URN_IETF_PARAMS_XML_NS_RLS_SERVICES)) {
        ezxml_free(xml_ptr);
        return (NULL);
    }
    if (NULL == ezxml_set_attr(xml_ptr,
            XCAP_XMLNS_XSI,
            XCAP_HTTP_WWW_W3_ORG_2001_XMLSCHEMA_INSTANCE)) {
        ezxml_free(xml_ptr);
        return (NULL);
    }

    /*
     * Return handle for future additions to this doc
     */
    return ((XCAP_Xml)xml_ptr);
}

/* 
 * ======== XCAP_xmlRlsAddServiceToDocument() ========
 */
XCAP_Xml XCAP_xmlRlsAddServiceToDocument(
    XCAP_Xml  doc,
    char     *uri_ptr)
{
    ezxml_t xml_ptr;
    ezxml_t child_ptr;

    if ((NULL == uri_ptr) || (NULL == doc)) {
        return (NULL);
    }

    xml_ptr = (ezxml_t)doc;

    /*
     * Add a new service to the XML doc
     */
    if (NULL == (child_ptr = ezxml_add_child(xml_ptr,
                XCAP_SERVICE,
                0))) {
        return (NULL);
    }

    /*
     * Set its URI
     */
    if (NULL == ezxml_set_attr(child_ptr,
            XCAP_URI,
            uri_ptr)) {
        ezxml_free(child_ptr);
        return (NULL);
    }
    
    return ((XCAP_Xml)child_ptr);
}

/* 
 * ======== XCAP_xmlRlsAddPackagesToService() ========
 */
XCAP_Xml XCAP_xmlRlsAddPackagesToService(
    XCAP_Xml service)
{
    ezxml_t xml_ptr;
    ezxml_t child_ptr;

    if (NULL == service) {
        return (NULL);
    }

    xml_ptr = (ezxml_t)service;

    /*
     * Add packages
     */
    if (NULL == (child_ptr = ezxml_add_child(xml_ptr,
                XCAP_PACKAGES,
                0))) {
        return (NULL);
    }

    return ((XCAP_Xml)child_ptr);
}

/* 
 * ======== XCAP_xmlRlsAddPackageToRlsServicePackages() ========
 */
XCAP_Xml XCAP_xmlRlsAddPackageToServicePackages(
    XCAP_Xml  packages,
    char     *packageName_ptr)
{
    ezxml_t xml_ptr;
    ezxml_t child_ptr;

    if ((NULL == packages) || (NULL == packageName_ptr)) {
        return (NULL);
    }
    
    xml_ptr = (ezxml_t)packages;

    /*
     * Add package
     */
    if (NULL == (child_ptr = ezxml_add_child(xml_ptr,
                XCAP_PACKAGE,
                0))) {
        return (NULL);
    }
    if (NULL == ezxml_set_txt(child_ptr,
            packageName_ptr)) {
        ezxml_free(child_ptr);
        return (NULL);
    }
    
    return ((XCAP_Xml)child_ptr);
}

/* 
 * ======== XCAP_xmlRlsAddResourceListElementToService() ========
 */
XCAP_Xml XCAP_xmlRlsAddResourceListElementToService(
    XCAP_Xml  service,
    char     *uri_ptr)
{
    ezxml_t xml_ptr;
    ezxml_t child_ptr;

    if ((NULL == service) || (NULL == uri_ptr)) {
        return (NULL);
    }
    
    xml_ptr = (ezxml_t)service;

    /*
     * Add package
     */
    if (NULL == (child_ptr = ezxml_add_child(xml_ptr,
                XCAP_RESOURCE_LIST,
                0))) {
        return (NULL);
    }
    if (NULL == ezxml_set_txt(child_ptr,
            uri_ptr)) {
        ezxml_free(child_ptr);
        return (NULL);
    }
    
    return ((XCAP_Xml)child_ptr);
}
