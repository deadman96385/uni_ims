/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 27867 $ $Date: 2014-08-04 17:15:29 +0800 (Mon, 04 Aug 2014) $
 */

#ifndef __SETTINGS_PARM_H_
#define __SETTINGS_PARM_H_

/* Private settings APIs */
char* _SETTINGS_getParmValue(
    ezxml_t     xml_ptr,
    const char *tag_ptr,
    const char *parmName_ptr);

char* _SETTINGS_getNestedParmValue(
    ezxml_t     xml_ptr,
    const char *parentTag_ptr,
    const char *childTag_ptr,
    const char *parmName_ptr);

char* _SETTINGS_getTagAttribute(
    ezxml_t     xml_ptr,
    const char *tag_ptr,
    const char *attr_ptr);

char* _SETTINGS_get2NestedParmValue(
    ezxml_t     xml_ptr,
    const char *parentTag_ptr,
    const char *childOneTag_ptr,
    const char *childTwoTag_ptr,
    const char *parmName_ptr);

char* _SETTINGS_get2NestedTagAttribute(
    ezxml_t     xml_ptr,
    const char *parentTag_ptr,
    const char *childOneTag_ptr,
    const char *childTwoTag_ptr,
    const char *attr_ptr);

#endif
