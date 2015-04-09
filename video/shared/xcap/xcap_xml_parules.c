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
 * ======== XCAP_xmlParulesCreateDocument() ========
 */
XCAP_Xml XCAP_xmlParulesCreateDocument(
    void)
{
    ezxml_t xml_ptr;
    
    /*
     * Create a new ruleset
     */
    if (NULL == (xml_ptr = ezxml_new(XCAP_CR_RULESET))) {
        return (NULL);
    }
    
    /*
     * Set XML namespaces
     */
    if (NULL == ezxml_set_attr(xml_ptr,
            XCAP_XMLNS,
            XCAP_URN_IETF_PARAMS_XML_NS_PRES_RULES)) {
        ezxml_free(xml_ptr);
        return (NULL);
    }
    if (NULL == ezxml_set_attr(xml_ptr,
            XCAP_XMLNS_PR,
            XCAP_URN_IETF_PARAMS_XML_NS_PRES_RULES)) {
        ezxml_free(xml_ptr);
        return (NULL);
    }
    if (NULL == ezxml_set_attr(xml_ptr,
            XCAP_XMLNS_CR,
            XCAP_URN_IETF_PARAMS_XML_NS_COMMON_POLICY)) {
        ezxml_free(xml_ptr);
        return (NULL);
    }

    /*
     * Return handle for future additions to this doc
     */
    return ((XCAP_Xml)xml_ptr);
}

/* 
 * ======== XCAP_xmlParulesAddRuleToDocument() ========
 */
XCAP_Xml XCAP_xmlParulesAddRuleToDocument(
    XCAP_Xml  doc,
    char     *id_ptr)
{
    ezxml_t xml_ptr;
    ezxml_t child_ptr;

    if ((NULL == id_ptr) || (NULL == doc)) {
        return (NULL);
    }

    xml_ptr = (ezxml_t)doc;

    /*
     * Add a new rule to the XML doc
     */
    if (NULL == (child_ptr = ezxml_add_child(xml_ptr,
                XCAP_CR_RULE,
                0))) {
        return (NULL);
    }

    /*
     * Set its URI
     */
    if (NULL == ezxml_set_attr(child_ptr,
            XCAP_ID,
            id_ptr)) {
        ezxml_free(child_ptr);
        return (NULL);
    }
    
    return ((XCAP_Xml)child_ptr);
}


/* 
 * ======== XCAP_xmlParulesAddElementToRule() ========
 */
XCAP_Xml XCAP_xmlParulesAddElementToRule(
    XCAP_Xml  rule,
    char     *name_ptr)
{
    ezxml_t xml_ptr;
    ezxml_t child_ptr;

    if ((NULL == name_ptr) || (NULL == rule)) {
        return (NULL);
    }

    xml_ptr = (ezxml_t)rule;

    /*
     * Add a new rule to the XML doc
     */
    if ((name_ptr != XCAP_CR_CONDITIONS)
            && (name_ptr != XCAP_CR_ACTIONS)
            && (name_ptr != XCAP_CR_TRANSFORMATIONS)) {
        return (NULL);
    }
    if (NULL == (child_ptr = ezxml_add_child(xml_ptr,
                name_ptr,
                0))) {
        return (NULL);
    }

    return ((XCAP_Xml)child_ptr);
}

/* 
 * ======== XCAP_xmlParulesAddIdentityToConditions() ========
 */
XCAP_Xml XCAP_xmlParulesAddIdentityToConditions(
    XCAP_Xml conditions)
{
    ezxml_t xml_ptr;
    ezxml_t child_ptr;

    if (NULL == conditions) {
        return (NULL);
    }

    xml_ptr = (ezxml_t)conditions;

    /*
     * Add a new identity
     */
    if (NULL == (child_ptr = ezxml_add_child(xml_ptr,
                XCAP_CR_IDENTITY,
                0))) {
        return (NULL);
    }

    return ((XCAP_Xml)child_ptr);
}

/* 
 * ======== XCAP_xmlParulesAddOneToIdentity() ========
 */
XCAP_Xml XCAP_xmlParulesAddOneToIdentity(
    XCAP_Xml  identity,
    char     *id_ptr)
{
    ezxml_t xml_ptr;
    ezxml_t child_ptr;

    if ((NULL == id_ptr) || (NULL == identity)) {
        return (NULL);
    }

    xml_ptr = (ezxml_t)identity;

    /*
     * Add a new identity
     */
    if (NULL == (child_ptr = ezxml_add_child(xml_ptr,
                XCAP_CR_ONE,
                0))) {
        return (NULL);
    }

    if (NULL == ezxml_set_attr(child_ptr,
            XCAP_ID,
            id_ptr)) {
        ezxml_free(child_ptr);
        return (NULL);
    }
    
    return ((XCAP_Xml)child_ptr);
}

/* 
 * ======== XCAP_xmlParulesAddSubscriptionHandlingToActions() ========
 */
XCAP_Xml XCAP_xmlParulesAddSubscriptionHandlingToActions(
    XCAP_Xml  actions,
    char     *subs_ptr)
{
    ezxml_t xml_ptr;
    ezxml_t child_ptr;

    if (NULL == actions) {
        return (NULL);
    }

    if ((XCAP_ALLOW != subs_ptr) && (XCAP_BLOCK != subs_ptr) &&
            (XCAP_POLITE_BLOCK != subs_ptr) && (XCAP_CONFIRM != subs_ptr)) {
        return (NULL);
    }

    xml_ptr = (ezxml_t)actions;

    /*
     * Add a new identity
     */
    if (NULL == (child_ptr = ezxml_add_child(xml_ptr,
                XCAP_PR_SUB_HANDLING,
                0))) {
        return (NULL);
    }

    /*
     * Set its URI
     */
    if (NULL == ezxml_set_txt(child_ptr,
            subs_ptr)) {
        ezxml_free(child_ptr);
        return (NULL);
    }
    
    return ((XCAP_Xml)child_ptr);
}
