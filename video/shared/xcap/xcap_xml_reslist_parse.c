/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 12486 $ $Date: 2010-07-08 06:10:49 +0800 (Thu, 08 Jul 2010) $
 *
 */

#include <ezxml.h>
#include <osal.h>
#include "xcap_resources.h"
#include "xcap_xml_helper.h"

/* 
 * ======== _XCAP_xmlReslistParseDisplayName() ========
 * 
 * This function gets display name tag.
 *
 * Returns: 
 *   A string pointing to display name, or NULL for not found.
 */
static char *_XCAP_xmlReslistParseDisplayName(
    XCAP_Xml xml) 
{
    ezxml_t dn_ptr;
    
    if (NULL == xml) {
        return (NULL);
    }

    dn_ptr = ezxml_get((ezxml_t)xml,
            XCAP_DISPLAY_NAME,
            0,
            NULL);
    if (NULL != dn_ptr) {
        return((char *)ezxml_txt(dn_ptr));
    }
    else {
        return(NULL);
    }
}

/* 
 * ======== XCAP_xmlReslistParseDocument() ========
 */
XCAP_Xml XCAP_xmlReslistParseDocument(
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
        if (0 != OSAL_strcmp(XCAP_URN_IETF_PARAMS_XML_NS_RESOURCE_LISTS,
                str_ptr)) {
            ezxml_remove(xml_ptr);
            return (NULL);
        }
    }
    else {
        ezxml_remove(xml_ptr);
        return (NULL);
    }
    
    /* 
     * Enable if needed, specification says it's required but no presence
     * agent vendors require it.
     */
#if 0
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
 * ======== XCAP_xmlReslistParseGetList() ========
 */
XCAP_Xml XCAP_xmlReslistParseGetList(
    XCAP_Xml   doc,
    int        listNumber,
    char     **listName_ptr,
    char     **listDisplayName_ptr)
{
    ezxml_t ret_ptr;

    if (NULL == doc) {
        return (NULL);
    }

    /*
     * Get a list
     */
    ret_ptr = ezxml_get((ezxml_t)doc,
            XCAP_LIST,
            listNumber,
            NULL);

    /*
     * Also return its name
     */
    if ((NULL != ret_ptr) && (NULL != listName_ptr)) {
        *listName_ptr = (char *)ezxml_attr(ret_ptr,
                (const char *)XCAP_NAME);
    }
    
    /*
     * And dsiplay name
     */
    if ((NULL != ret_ptr) && (NULL != listDisplayName_ptr)) {
        *listDisplayName_ptr = _XCAP_xmlReslistParseDisplayName(ret_ptr);
    }

    return ((XCAP_Xml)ret_ptr);
}

/* 
 * ======== XCAP_xmlReslistParseGetEntry() ========
 */
XCAP_Xml XCAP_xmlReslistParseGetEntry(
    XCAP_Xml   list,
    int        entryNumber,
    char     **uri_ptr,
    char     **displayName_ptr)
{
    ezxml_t ret_ptr;

    if (NULL == list) {
        return (NULL);
    }

    /*
     * Get a entry
     */
    ret_ptr = ezxml_get((ezxml_t)list,
            XCAP_ENTRY,
            entryNumber,
            NULL);

    /*
     * Also return its name
     */
    if ((NULL != ret_ptr) && (NULL != uri_ptr)) {
        *uri_ptr = (char *)ezxml_attr(ret_ptr,
                (const char *)XCAP_URI);
    }

    /*
     * And dsiplay name
     */
    if ((NULL != ret_ptr) && (NULL != displayName_ptr)) {
        *displayName_ptr = _XCAP_xmlReslistParseDisplayName(ret_ptr);
    }

    return ((XCAP_Xml)ret_ptr);
}
