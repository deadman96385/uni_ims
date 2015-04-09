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
 * ======== XCAP_xmlHelperMakeDocument() ========
 */
char *XCAP_xmlHelperMakeDocument(
    XCAP_Xml     xml,
    OSAL_Boolean hasHdr)
{
    char *str_ptr;
    char *ret_ptr;
    int   len;
    int   sz;

    if (NULL == xml) {
        return (NULL);
    }

    /*
     * Find length of string to return.
     */
    str_ptr = ezxml_toxml((ezxml_t)xml);
    if (NULL == str_ptr) {
        return (NULL);
    }
    if (OSAL_FALSE == hasHdr) {
        return (str_ptr);
    }
    len = OSAL_strlen(str_ptr);
    len += OSAL_strlen(XCAP_XML_HDR);
    len += 1;

    ret_ptr = NULL;
    ret_ptr = OSAL_memAlloc(len + 1,
            0);

    if (NULL != ret_ptr) {
        sz = OSAL_snprintf(ret_ptr,
                len,
                "%s%s",
                XCAP_XML_HDR,
                str_ptr);
        ret_ptr[sz] = 0;
    }

    OSAL_memFree(str_ptr,
            0);
    return (ret_ptr);
}

/* 
 * ======== XCAP_xmlHelperFreeDocument() ========
 */
void XCAP_xmlHelperFreeDocument(
    char **doc_ptr)
{
    if (NULL == doc_ptr) {
        return;
    }
    if (NULL == *doc_ptr) {
        return;
    }

    OSAL_memFree(*doc_ptr,
            0);

    /*
     * Make sure no repeat free, and segfault.
     */
    *doc_ptr = NULL;
}

/* 
 * ======== XCAP_xmlHelperDeleteHandle() ========
 */
void XCAP_xmlHelperDeleteHandle(
    XCAP_Xml *xml_ptr)
{
    if (NULL == xml_ptr) {
        return;
    }
    if (NULL == *xml_ptr) {
        return;
    }

    ezxml_remove((ezxml_t)*xml_ptr);

    /*
     * Make sure no repeat free, and segfault.
     */
    *xml_ptr = NULL;
}
