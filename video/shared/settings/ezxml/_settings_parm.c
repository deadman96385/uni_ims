/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 27754 $ $Date: 2014-07-29 14:27:42 +0800 (Tue, 29 Jul 2014) $
 */

#include <osal.h>
#include <osal_string.h>
#include "_settings.h"

/*
 * ======== _SETTINGS_getParmValue() ========
 *
 * This function retrieves the value of a parm tag with name attribute name_ptr
 * which under tag_ptr in the XML doc.
 * Example:
 * <sip>
 *     <parm name="Timer_T1" value="2000"/>
 * </sip>
 * Given tag_ptr as SETTINGS_TAG_SIP and parmName_ptr as SETTINGS_PARM_TIMER_T1
 * and this functions retrieves value "2000" and returns
 * the pointer to the value string.
 *
 * Returns:
 *  A pointer to a string containing the parm value.
 *  NULL if the value  could not be found or if there is no value.
 *
 */
char* _SETTINGS_getParmValue(
    ezxml_t     xml_ptr,
    const char *tag_ptr,
    const char *parmName_ptr)
{
    ezxml_t   child_ptr;
    char     *value_ptr;

    SETTINGS_dbgPrintf("tag:%s parmName:%s\n", tag_ptr, parmName_ptr);
    /* Get the tag */
    if (NULL == (child_ptr = ezxml_child(xml_ptr, tag_ptr))) {
        /* Cannot find the tag */
        return (NULL);
    }

    /* Then we have the tag then find parm */
    child_ptr = ezxml_child(child_ptr, SETTINGS_TAG_PARM);
    while (NULL != child_ptr) {
        /* Get and compare the type */
        if ((NULL != (value_ptr = (char*)ezxml_attr(child_ptr,
                SETTINGS_ATTR_NAME))) &&
                (0 == OSAL_strcmp(value_ptr, parmName_ptr))) {
            /* Found the type */
            break;
        }
        /* parm doesn't match, try next. */
        child_ptr = ezxml_next(child_ptr);
    }

    if (NULL == child_ptr) {
        return (NULL);
    }

    SETTINGS_dbgPrintf("value:%s\n",
            (char*)ezxml_attr(child_ptr, SETTINGS_ATTR_VALUE));

    /* We got the parm with the name, return the value. */
    return (char*)ezxml_attr(child_ptr, SETTINGS_ATTR_VALUE);
}

/*
 * ======== _SETTINGS_get2NestedParmValue() ========
 *
 * This function retrieves the value of a parm tag with name attribute
 * parmName_ptr which under two depth nested tags parentTag_ptr and
 * childOneTag_ptr and childTwoTag_ptr.
 * in the XML doc.
 * Example:
 * <protocol>
 *     <sip>
 *         <capabilities>
 *             <parm name="ip voice call" value="1"/>
 *         </capabilities>
 *     </sip>
 * </protocol>
 * Given parentTag_ptr as SETTING_TAG_PROTOCOL and and childOneTag_ptr as
 * SETTINGS_TAG_SIP, childTwoTag_ptr as SETTINGS_TAG_CAPABILITIES,
 * and parmName_ptr as SETTING_PARM_IP_VOICE_CALL and this functions
 * retrieves value "1" and returns the pointer to the value string.
 *
 * Returns:
 *  A pointer to a string containing the parm value.
 *  NULL if the value  could not be found or if there is no value.
 *
 */
char* _SETTINGS_get2NestedParmValue(
    ezxml_t     xml_ptr,
    const char *parentTag_ptr,
    const char *childOneTag_ptr,
    const char *childTwoTag_ptr,
    const char *parmName_ptr)
{
    ezxml_t   child_ptr;
    char     *value_ptr;

    SETTINGS_dbgPrintf("parentTag:%s childOneTag:%s childTwoTag:%s, "
            "parmName:%s\n", parentTag_ptr, childOneTag_ptr, childTwoTag_ptr,
            parmName_ptr);

    /* Get the parent tag */
    if (NULL == (child_ptr = ezxml_child(xml_ptr, parentTag_ptr))) {
        /* Cannot find the tag */
        return (NULL);
    }

    /* Got the parent tag, find first child tag */
    if (NULL == (child_ptr = ezxml_child(child_ptr, childOneTag_ptr))) {
        /* Cannot find the tag */
        return (NULL);
    }

    /* Got the first child tag, find second child tag */
    if (NULL == (child_ptr = ezxml_child(child_ptr, childTwoTag_ptr))) {
        /* Cannot find the tag */
        return (NULL);
    }

    /* Then we have the child tag then find parm */
    child_ptr = ezxml_child(child_ptr, SETTINGS_TAG_PARM);
    while (NULL != child_ptr) {
        /* Get and compare the type */
        if ((NULL != (value_ptr = (char*)ezxml_attr(child_ptr,
                SETTINGS_ATTR_NAME))) &&
                (0 == OSAL_strcmp(value_ptr, parmName_ptr))) {
            /* Found the type */
            break;
        }
        /* parm doesn't match, try next. */
        child_ptr = ezxml_next(child_ptr);
    }

    if (NULL == child_ptr) {
        return (NULL);
    }

    SETTINGS_dbgPrintf("value:%s\n",
            (char*)ezxml_attr(child_ptr, SETTINGS_ATTR_VALUE));

    /* We got the parm with the name, return the value. */
    return (char*)ezxml_attr(child_ptr, SETTINGS_ATTR_VALUE);
}

/*
 * ======== _SETTINGS_getNestedParmValue() ========
 *
 * This function retrieves the value of a parm tag with name attribute
 * parmName_ptr which under a nested tags parentTag_ptr and childTag_ptr
 * in the XML doc.
 * Example:
 * <sip>
 *     <capabilities>
 *         <parm name="ip voice call" value="1"/>
 *     </capabilities>
 * </sip
 * Given parentTag_ptr as SETTING_TAG_SIP and and childTag_ptr as
 * SETTINGS_TAG_CAPABILITIES and parmName_ptr as SETTINGS_PARM_Q_VALUE
 *  and this functions retrieves value "0.5" and returns
 * the pointer to the value string.
 *
 * Returns:
 *  A pointer to a string containing the parm value.
 *  NULL if the value  could not be found or if there is no value.
 *
 */
char* _SETTINGS_getNestedParmValue(
    ezxml_t     xml_ptr,
    const char *parentTag_ptr,
    const char *childTag_ptr,
    const char *parmName_ptr)
{
    ezxml_t   child_ptr;
    char     *value_ptr;

    SETTINGS_dbgPrintf("parentTag:%s childTag:%s parmName:%s\n",
            parentTag_ptr, childTag_ptr, parmName_ptr);

    /* Get the tag */
    if (NULL == (child_ptr = ezxml_child(xml_ptr, parentTag_ptr))) {
        /* Cannot find the tag */
        return (NULL);
    }

    /* Got the parent tag, find child tag */
    if (NULL == (child_ptr = ezxml_child(child_ptr, childTag_ptr))) {
        /* Cannot find the tag */
        return (NULL);
    }

    /* Then we have the child tag then find parm */
    child_ptr = ezxml_child(child_ptr, SETTINGS_TAG_PARM);
    while (NULL != child_ptr) {
        /* Get and compare the type */
        if ((NULL != (value_ptr = (char*)ezxml_attr(child_ptr,
                SETTINGS_ATTR_NAME))) &&
                (0 == OSAL_strcmp(value_ptr, parmName_ptr))) {
            /* Found the type */
            break;
        }
        /* parm doesn't match, try next. */
        child_ptr = ezxml_next(child_ptr);
    }

    if (NULL == child_ptr) {
        return (NULL);
    }

    SETTINGS_dbgPrintf("value:%s\n",
            (char*)ezxml_attr(child_ptr, SETTINGS_ATTR_VALUE));

    /* We got the parm with the name, return the value. */
    return (char*)ezxml_attr(child_ptr, SETTINGS_ATTR_VALUE);
}

/*
 * ======== _SETTINGS_getTagAttribute() ========
 *
 * This function retrieves the value of a attribute of a tag.
 * Example:
 * <protocol id="1">
 * Given parentTag_ptr as SETTINGS_TAG_PROTOCOL and attr_ptr as
 * SETTINGS_ATTR_ID and this routine retrieves value "1" and returns
 * the pointer to the value string.
 *
 * Returns:
 *  A pointer to a string containing the attribute value.
 *  NULL if the value  could not be found or if there is no value.
 *
 */
char* _SETTINGS_getTagAttribute(
    ezxml_t     xml_ptr,
    const char *tag_ptr,
    const char *attr_ptr)
{
    ezxml_t   child_ptr;

    SETTINGS_dbgPrintf("tag:%s attr:%s\n",
            tag_ptr, attr_ptr);

    /* Get the tag */
    if (NULL == (child_ptr = ezxml_child(xml_ptr, tag_ptr))) {
        /* Cannot find the tag */
        return (NULL);
    }

    SETTINGS_dbgPrintf("value:%s\n",
            (char*)ezxml_attr(child_ptr, attr_ptr));

    /* Then we have the tag then return the attribute value */
    return (char*)ezxml_attr(child_ptr, attr_ptr);
}

/*
 * ======== _SETTINGS_get2NestedTagAttribute() ========
 *
 * This function retrieves the value of a attribute of two depth
 * nested tag.
 * Example:
 * <protocol>
 *     <sip>
 *         <exchang-capabilities common-stack="1"/>
 *     </sip>
 * </protocol>
 * Given parentTag_ptr as SETTINGS_TAG_PROTOCOL, childOneTag_ptr as
 * SETTINGS_TAG_SIP, childTwoTag_ptr as SETTINGS_TAG_EX_CAPABILITIES
 * and attr_ptr as SETTINGS_ATTR_COMMON_STACK and this routine retrieves
 * value "1" and returns the pointer to the value string.
 *
 * Returns:
 *  A pointer to a string containing the attribute value.
 *  NULL if the value  could not be found or if there is no value.
 *
 */
char* _SETTINGS_get2NestedTagAttribute(
    ezxml_t     xml_ptr,
    const char *parentTag_ptr,
    const char *childOneTag_ptr,
    const char *childTwoTag_ptr,
    const char *attr_ptr)
{
    ezxml_t   child_ptr;

    SETTINGS_dbgPrintf("parentTag:%s childOneTag:%s childTwoTag:%s, "
            "attr:%s\n", parentTag_ptr, childOneTag_ptr, childTwoTag_ptr,
            attr_ptr);

    /* Get the tag */
    if (NULL == (child_ptr = ezxml_child(xml_ptr, parentTag_ptr))) {
        /* Cannot find the tag */
        return (NULL);
    }

    /* Get the first child tag */
    if (NULL == (child_ptr = ezxml_child(child_ptr, childOneTag_ptr))) {
        /* Cannot find the tag */
        return (NULL);
    }

    /* Get the second child tag */
    if (NULL == (child_ptr = ezxml_child(child_ptr, childTwoTag_ptr))) {
        /* Cannot find the tag */
        return (NULL);
    }

    SETTINGS_dbgPrintf("value:%s\n",
            (char*)ezxml_attr(child_ptr, attr_ptr));

    /* Then we have the tag then return the attribute value */
    return (char*)ezxml_attr(child_ptr, attr_ptr);
}

