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
 * ======== XCAP_xmlParulesParseDocument() ========
 */
XCAP_Xml XCAP_xmlParulesParseDocument(
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
     * Now verify that it meets RFC 5025. Namespace.
     */

    str_ptr = (char *)ezxml_attr(xml_ptr,
            (const char *)XCAP_XMLNS);
    if (NULL != str_ptr) {
        if (0 != OSAL_strcmp(XCAP_URN_IETF_PARAMS_XML_NS_PRES_RULES,
                str_ptr)) {
            ezxml_remove(xml_ptr);
            return (NULL);
        }
    }
    else {
        ezxml_remove(xml_ptr);
        return (NULL);
    }
    
    str_ptr = (char *)ezxml_attr(xml_ptr,
            (const char *)XCAP_XMLNS_PR);
    if (NULL != str_ptr) {
        if (0 != OSAL_strcmp(XCAP_URN_IETF_PARAMS_XML_NS_PRES_RULES,
                str_ptr)) {
            ezxml_remove(xml_ptr);
            return (NULL);
        }
    }
    else {
        ezxml_remove(xml_ptr);
        return (NULL);
    }
    
    str_ptr = (char *)ezxml_attr(xml_ptr,
            (const char *)XCAP_XMLNS_CR);
    if (NULL != str_ptr) {
        if (0 != OSAL_strcmp(XCAP_URN_IETF_PARAMS_XML_NS_COMMON_POLICY,
                str_ptr)) {
            ezxml_remove(xml_ptr);
            return (NULL);
        }
    }
    else {
        ezxml_remove(xml_ptr);
        return (NULL);
    }

    return ((XCAP_Xml)xml_ptr);
}

/* 
 * ======== XCAP_xmlParulesParseGetRule() ========
 */
XCAP_Xml XCAP_xmlParulesParseGetRule(
    XCAP_Xml   doc,
    int        ruleNumber,
    char     **id_ptr)
{
    ezxml_t ret_ptr;

    if (NULL == doc) {
        return (NULL);
    }

    /*
     * Get a service
     */
    ret_ptr = ezxml_get((ezxml_t)doc,
            XCAP_CR_RULE,
            ruleNumber,
            NULL);

    /*
     * Also return its URI
     */
    if ((NULL != ret_ptr) && (NULL != id_ptr)) {
        *id_ptr = (char *)ezxml_attr(ret_ptr,
                (const char *)XCAP_ID);
    }
    
    return ((XCAP_Xml)ret_ptr);
}

/* 
 * ======== XCAP_xmlParulesParseGetElementFromRule() ========
 */
XCAP_Xml XCAP_xmlParulesParseGetElementFromRule(
    XCAP_Xml  rule,
    char     *name_ptr)
{
    if (NULL == rule) {
        return (NULL);
    }

    /*
     * Limit what can be parsed.
     */
    if ((XCAP_CR_CONDITIONS != name_ptr) &&
            (XCAP_CR_ACTIONS != name_ptr) &&
            (XCAP_CR_TRANSFORMATIONS != name_ptr)) {
        return (NULL);
    }

    /*
     * Get packages
     */
    return((XCAP_Xml)ezxml_get((ezxml_t)rule,
            name_ptr,
            0,
            NULL));
}

/* 
 * ======== XCAP_xmlParulesParseGetIdentityFromConditions() ========
 */
XCAP_Xml XCAP_xmlParulesParseGetIdentityFromConditions(
    XCAP_Xml conditions,
    int      identityNumber)
{
    if (NULL == conditions) {
        return (NULL);
    }

    /*
     * Get identity 
     */
    return((XCAP_Xml)ezxml_get((ezxml_t)conditions,
            XCAP_CR_IDENTITY,
            identityNumber,
            NULL));
}

/* 
 * ======== XCAP_xmlParulesParseGetOneFromIdentity() ========
 */
XCAP_Xml XCAP_xmlParulesParseGetOneFromIdentity(
    XCAP_Xml   identity,
    int        oneNumber,
    char     **id_ptr)
{
    ezxml_t ret_ptr;

    if (NULL == identity) {
        return (NULL);
    }

    /*
     * Get a one 
     */
    ret_ptr = ezxml_get((ezxml_t)identity,
            XCAP_CR_ONE,
            oneNumber,
            NULL);

    /*
     * Also return its ID
     */
    if ((NULL != ret_ptr) && (NULL != id_ptr)) {
        *id_ptr = (char *)ezxml_attr(ret_ptr,
                (const char *)XCAP_ID);
    }
    
    return ((XCAP_Xml)ret_ptr);
}

/* 
 * ======== XCAP_xmlParulesParseGetSubscriptionHandlingFromActions() ========
 */
XCAP_Xml XCAP_xmlParulesParseGetSubscriptionHandlingFromActions(
    XCAP_Xml   actions,
    char     **subs_ptr)
{
    ezxml_t  ret_ptr;
    char    *s_ptr;

    if (NULL == actions) {
        return (NULL);
    }

    /*
     * Get sub handling 
     */
    ret_ptr = ezxml_get((ezxml_t)actions,
            XCAP_PR_SUB_HANDLING,
            0,
            NULL);

    /*
     * Also return its value, but only allow specified states 
     */
    if ((NULL != ret_ptr) && (NULL != subs_ptr)) {
        s_ptr = (char *)ezxml_txt(ret_ptr);
        if (NULL == s_ptr) {
            *subs_ptr = NULL;
        }
        else if (0 == OSAL_strcmp(s_ptr, XCAP_ALLOW)) {
            *subs_ptr = XCAP_ALLOW;
        }
        else if (0 == OSAL_strcmp(s_ptr, XCAP_BLOCK)) {
            *subs_ptr = XCAP_BLOCK;
        }
        else if (0 == OSAL_strcmp(s_ptr, XCAP_CONFIRM)) {
            *subs_ptr = XCAP_CONFIRM;
        }
        else if (0 == OSAL_strcmp(s_ptr, XCAP_POLITE_BLOCK)) {
            *subs_ptr = XCAP_POLITE_BLOCK;
        }
        else {
            *subs_ptr = NULL;
        }
    }
    
    return ((XCAP_Xml)ret_ptr);
}
