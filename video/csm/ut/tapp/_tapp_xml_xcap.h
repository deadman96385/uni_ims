/*
 * THIS IS AN UNPUBLISHED WORK CONVETINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIEVETRY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev$ $Date$
 *
 */
 
/* TAPP XCAP xml action parsing functions */

#ifndef __TAPP_XML_XCAP_H_
#define __TAPP_XML_XCAP_H_

TAPP_Return TAPP_xmlParseXcapCmd(
    ezxml_t     xml_ptr,
    TAPP_mockXcapCmd    *mockXcapCmd_ptr);

TAPP_Return TAPP_xmlParseXcapEvt(
    ezxml_t     xml_ptr,
    TAPP_mockXcapEvt    *mockXcapEvt_ptr);

#endif