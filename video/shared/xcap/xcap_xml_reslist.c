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
 * ======== XCAP_xmlReslistCreateDocument() ========
 */
XCAP_Xml XCAP_xmlReslistCreateDocument(
    void)
{
    ezxml_t xml_ptr;
    
    /*
     * Create a new resource list
     */
    if (NULL == (xml_ptr = ezxml_new(XCAP_RESOURCE_LISTS))) {
        return (NULL);
    }
    
    /*
     * Set XML namespaces
     */
    if (NULL == ezxml_set_attr(xml_ptr,
            XCAP_XMLNS,
            XCAP_URN_IETF_PARAMS_XML_NS_RESOURCE_LISTS)) {
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
 * ======== XCAP_xmlReslistAddListToDocument() ========
 */
XCAP_Xml XCAP_xmlReslistAddListToDocument(
    XCAP_Xml  doc,
    char     *listName_ptr,
    char     *listDisplayName_ptr)
{
    ezxml_t xml_ptr;
    ezxml_t child_ptr;
    ezxml_t childDn_ptr;

    if ((NULL == listName_ptr) || (NULL == listDisplayName_ptr) ||
            (NULL == doc)) {
        return (NULL);
    }

    xml_ptr = (ezxml_t)doc;

    /*
     * Add a new list to the XML doc
     */
    if (NULL == (child_ptr = ezxml_add_child(xml_ptr,
                XCAP_LIST,
                0))) {
        return (NULL);
    }

    /*
     * Set its name
     */
    if (NULL == ezxml_set_attr(child_ptr,
            XCAP_NAME,
            listName_ptr)) {
        ezxml_free(child_ptr);
        return (NULL);
    }
    
    /*
     * Set its display name
     */
    if (NULL == (childDn_ptr = ezxml_add_child(child_ptr,
            XCAP_DISPLAY_NAME,
            0))) {
        ezxml_free(child_ptr);
        return (NULL);
    }
    if (NULL == ezxml_set_txt(childDn_ptr, listDisplayName_ptr)) {
        ezxml_free(childDn_ptr);
        ezxml_free(child_ptr);
        return (NULL);
    }

    return ((XCAP_Xml)child_ptr);
}

/* 
 * ======== XCAP_xmlReslistAddEntryToList() ========
 */
XCAP_Xml XCAP_xmlReslistAddEntryToList(
    XCAP_Xml  list,
    char     *uri_ptr,
    char     *displayName_ptr)
{
    ezxml_t xml_ptr;
    ezxml_t child_ptr;
    ezxml_t childDn_ptr;

    if ((NULL == uri_ptr) || (NULL == displayName_ptr) || (NULL == list)) {
        return (NULL);
    }

    xml_ptr = (ezxml_t)list;

    /*
     * Add entry
     */
    if (NULL == (child_ptr = ezxml_add_child(xml_ptr,
                XCAP_ENTRY,
                0))) {
        return (NULL);
    }

    /*
     * Set URI
     */
    if (NULL == ezxml_set_attr(child_ptr, XCAP_URI, uri_ptr)) {
        ezxml_free(child_ptr);
        return (NULL);
    }
    
    /*
     * Set its display name
     */
    if (NULL == (childDn_ptr = ezxml_add_child(child_ptr,
            XCAP_DISPLAY_NAME,
            0))) {
        ezxml_free(child_ptr);
        return (NULL);
    }
    if (NULL == ezxml_set_txt(childDn_ptr, displayName_ptr)) {
        ezxml_free(childDn_ptr);
        ezxml_free(child_ptr);
        return (NULL);
    }

    return ((XCAP_Xml)child_ptr);
}
