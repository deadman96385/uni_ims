/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 12486 $ $Date: 2010-07-08 06:10:49 +0800 (Thu, 08 Jul 2010) $
 *
 */

#ifndef _XCAP_API_H_
#define _XCAP_API_H_

/** \mainpage vPort Module XCAP: Application Programming Interface
 * \author Zubair Ali Khan
 *
 * \section intro_sec Introduction
 * This is a client only embedded implementation of XCAP protocol specified in
 * http://www.ietf.org/rfc/rfc4825.txt .
 * Many IP communication deployments that provide VoIP, Presence, 
 * IM or any combination of these services find it advantageous to store 
 * per-user information on network servers.  This relieves endpoint clients 
 * from the responsibility of managing and backing up user provisioning 
 * information and personal data.  It also provides users with a same or 
 * similar experience with the network services regardless of what device 
 * they are using.
 * \image html Intro.png
 * For example, a user could have a mobile handset when they are in the car 
 * and then use a soft client running on a PC when they are in the office; 
 * however, the user's experience on both device types is very similar 
 * (or the same) as both of these devices share the user's information located 
 * on the network that provides the services.\n
 * There are many examples of user information used in IP voice, 
 * instant message and presence applications that are stored on 
 * network servers.  It is common that user information is stored as 
 * XML documents.  This is especially true in Presence applications.  
 * Resource lists (contact lists), Presence rules (authorization rules for 
 * allowing/denying others the ability to see your presence) and Presence 
 * lists (List of other people who will see your presence) are all types of 
 * information that can be stored on network servers as XML documents. 
 * XCAP allows a client to read, write, and modify per-user data stored in 
 * XML format on an XCAP server. \n
 * Although XCAP is considered a protocol in it's own right that actually 
 * isn't completely accurate.  XCAP defines a URI format that can be used to 
 * manipulate XML document on the server via HTTP, so XCAP implements HTTP.\n
 * - XCAP is defined in \n
 *   RFC 4825 http://www.ietf.org/rfc/rfc4825.txt \n
 * - XCAP presence / contact list manipulations are defined in\n 
 *   RFC 4826 http://www.ietf.org/rfc/rfc4826.txt \n
 *   RFC 5025 http://www.ietf.org/rfc/rfc5025.txt \n
 *   RFC 4745 http://www.ietf.org/rfc/rfc4745.txt \n
 *
 * \section install_sec Installation
 * XCAP under vPort build process will be built as a relocatable
 * library, and can be linked statically with other vPort programs using it.
 * There is no need to place XCAP library on your target system as it is linked
 * statically.
 * XCAP links with following libraries (which may also link with OS libraries):
 * - ezxml (XML) - which links with libc dynamically
 * - osal (OS independant) - which links with libc, libssl dynamically
 * - http (transport) - which links with libcurl, libssl dynamically
 * - libc for GNU regular expressions dynamically (_xcap_port.c)
 *
 * \subsection step1 Step 1: Preparing the Target Device
 * Target device must have all librariees required to run OSAL on it. OSAL
 * typically requires a C library, an SSL library, and an OS threading library.
 * Once OSAL is up and running, proceed to next step.
 * \subsection step2 Step 2: Installing libcurl
 * Install libcurl to the target device. You may need to build libcurl by
 * downloading its source from http://curl.haxx.se/libcurl . Once libcurl is
 * built, place its library on your target device where the linker loader can
 * load it dynamically. On some platforms static linking is required, in which
 * case libcurl need not be on the target device.
 * \subsection step3 Step 3: Installing libcurl in Build Tools
 * Prepare your tools for building with libcurl. This involves installing
 * libcurl headers an libraries to your build tools (tools may be cross
 * compiling).
 *
 * \page dev_page Developing with XCAP Library
 * In a SIP SIMPLE enabled presence capable network, XCAP will work very 
 * closely with the SIP stack.  SIP signaling is used to communicate to SIP 
 * servers the endpoints desire to receive another's presence state and to 
 * report this endpoint's presence state.  It is also used to communicate 
 * to a SIP server that this endpoint wishes to be notified of changes 
 * or updates to per-user information that is stored on the XCAP server.
 * Then when changes occur, to say an endpoints “Presence Rules”,
 * SIP signaling can notify the endpoint that there is a change.  
 * Then, at that point, the endpoint uses XCAP to retrieve the XML
 * information that changed on the XCAP server.
 * \image html Dev.png
 * SAPP SIP applicaiton in vPort works with XCAP protocol for managing documents
 * on the XCAP sevrer.
 *
 * - Application includes file xcap_api.h\n
 *   This file exposes all public API of XCAP
 * - To prepare and manipulate XML documents, elements, and attributes,
 *   see xcap_xml_parules.h, xcap_xml_rls.h, and xcap_xml_reslist.h
 * - To parse XML documents, elements, and attributes, see 
 *   xcap_xml_parules_parse.h, xcap_xml_rls_parse.h, and 
 *   xcap_xml_reslist_parse.h
 * - To generate XML documents, and free them, see xcap_xml_helper.h
 * - To send and receive XML documents, elements, and attributes, see xcap.h
 * - To parse HTML HTTP headers, see xcap_http_parse.h
 * - For misc. helper functions e.g. URI manipulations, see xcap_helper.h
 * - Text strings are defined in xcap_resources.h, and is included
 *   automatically with xcap_api.h. Strings used in public API are explained
 *   with fucntion calls that use them.
 *
 * \page limits_page Limitations
 *
 * \page example1_page C Code Example1 - Documents
 * This example demostrates how to delete, make, put, and fetch XML documents in
 * their entirety. Fetched document is parsed in this example.
 * You can copy this code then build and run it.
 * \code
#include <xcap_api.h>
#include <osal.h>
#include <stdio.h>

// Main function
int main()
{
    char                 *str_ptr;
    char                 *str1_ptr;
    char                  touri[256];
    XCAP_Xml              xml;
    XCAP_Xml              subs1;
    XCAP_Xml              arule;
    XCAP_Xml              conds;
    XCAP_Xml              acts;
    XCAP_Xml              xforms;
    XCAP_Xml              aid;
    XCAP_Xml              axid;
    XCAP_Obj              obj;
    XCAP_Cmd              cmd;
    XCAP_Evt              evt;
    XCAP_HttpParseHdrObj  hdrobj;

    //Init XCAP protocol.
    //All XCAP transactions will timeout after 10 seconds:
    XCAP_init(&obj, 10);
    OSAL_taskDelay(1000);

    //
    // Delete a doc.
    //
    
    // Make a URI
    XCAP_helperMakeUri(
            "2233358499",
            "enyb73",
            "https://xcap.sipthor.net/xcap-root",
            XCAP_PRES_RULES,
            XCAP_USERS,
            "2233358499@sip2sip.info",
            NULL,
            NULL,
            touri,
            sizeof(touri) - 1);

    // Setup a command
    cmd.op = XCAP_OPERATION_DELETE;
    cmd.opType = XCAP_OPERATION_TYPE_DOCUMENT;
    cmd.uri_ptr = touri;
    cmd.auid_ptr = XCAP_PRES_RULES;

    // Now send it
    XCAP_sendCmd(&obj, &cmd);

    // Wait for response
    XCAP_getEvt(&obj, &evt, -1);

    // Assume there was no error, parse the HTTP header
    // ! Do check evt.error in your code, skipped here
    XCAP_httpParseHeader(evt.hdr_ptr, strlen(evt.hdr_ptr),
            &hdrobj);

    // Print header info.
    OSAL_logMsg("event error code=%d\n", evt.error);
    OSAL_logMsg("HTTP final code=%d\n", hdrobj.finalCode);
    OSAL_logMsg("HTTP Content-Length=%d\n",hdrobj.contentLength);
    OSAL_logMsg("HTTP Content-Type=%s\n",hdrobj.contentType);
    OSAL_logMsg("HTTP ETag=%s\n", hdrobj.etag);
    OSAL_logMsg("HTTP body=\n%s\n", evt.body_ptr);
    // This is absolutely necessary
    XCAP_disposeEvt(&evt);
    
    //
    // Now put a doc
    //
   
    // Make a presence authorization rules document
    xml = XCAP_xmlParulesCreateDocument();
    // add a rule to it
    arule = XCAP_xmlParulesAddRuleToDocument(xml, "pres_whitelist");
    // add conditions, actions, and transformations to it
    conds = XCAP_xmlParulesAddElementToRule(arule, XCAP_CR_CONDITIONS);
    acts = XCAP_xmlParulesAddElementToRule(arule, XCAP_CR_ACTIONS);
    xforms = XCAP_xmlParulesAddElementToRule(arule, XCAP_CR_TRANSFORMATIONS);
    // add identity to conditions
    aid = XCAP_xmlParulesAddIdentityToConditions(conds);
    // add one to identity
    axid = XCAP_xmlParulesAddOneToIdentity(aid, "sip:2233358499@sip2sip.info");
    // add subscription handling rule to actions
    subs1 = XCAP_xmlParulesAddSubscriptionHandlingToActions(acts, XCAP_ALLOW);
    // generate document in string
    str_ptr = XCAP_xmlHelperMakeDocument(xml, OSAL_TRUE);
    OSAL_logMsg("xml is:\n%s\n", str_ptr);
   
    // Setup a command
    cmd.op = XCAP_OPERATION_CREATE_REPLACE;
    cmd.opType = XCAP_OPERATION_TYPE_DOCUMENT;
    cmd.src_ptr = str_ptr;
    cmd.srcSz = strlen(str_ptr);

    // Send it
    XCAP_sendCmd(&obj, &cmd);

    // Wait for response
    XCAP_getEvt(&obj, &evt, -1);

    // Assume there was no error, parse the HTTP header
    // ! Do check evt.error in your code, skipped here
    XCAP_httpParseHeader(evt.hdr_ptr, strlen(evt.hdr_ptr),
            &hdrobj);

    // Print header info.
    OSAL_logMsg("event error code=%d\n", evt.error);
    OSAL_logMsg("HTTP final code=%d\n", hdrobj.finalCode);
    OSAL_logMsg("HTTP Content-Length=%d\n",hdrobj.contentLength);
    OSAL_logMsg("HTTP Content-Type=%s\n",hdrobj.contentType);
    OSAL_logMsg("HTTP ETag=%s\n", hdrobj.etag);
    OSAL_logMsg("HTTP body=\n%s\n", evt.body_ptr);
    
    // These are absolutely necessary
    XCAP_xmlHelperFreeDocument(&str_ptr);
    XCAP_xmlHelperDeleteHandle(&xml);
   
    //
    // now fetch back for verification
    //
    
    cmd.op = XCAP_OPERATION_FETCH;
    cmd.opType = XCAP_OPERATION_TYPE_DOCUMENT;
    XCAP_sendCmd(&obj, &cmd);
    XCAP_getEvt(&obj, &evt, -1);
    XCAP_httpParseHeader(evt.hdr_ptr, strlen(evt.hdr_ptr),
            &hdrobj);
    OSAL_logMsg("event err code=%d\n",evt.error);
    OSAL_logMsg("finalcode=%d\n", hdrobj.finalCode);
    OSAL_logMsg("contentlen=%d\n",hdrobj.contentLength);
    OSAL_logMsg("contentype=%s\n\0",hdrobj.contentType);
    OSAL_logMsg("etag=%s\n\0", hdrobj.etag);
    OSAL_logMsg("body=\n%s\n\0", evt.body_ptr);

    //
    // Now parse it
    //

    xml = XCAP_xmlParulesParseDocument(evt.body_ptr, strlen(evt.body_ptr));
    // get rule #1
    arule = XCAP_xmlParulesParseGetRule(xml, 0, &str1_ptr);
    str_ptr = XCAP_xmlHelperMakeDocument(arule, OSAL_FALSE);
    OSAL_logMsg("rule text is %s\nrule is:\n%s\n", str1_ptr, str_ptr);
    XCAP_xmlHelperFreeDocument(&str_ptr);
    // get conditions
    conds = XCAP_xmlParulesParseGetElementFromRule(arule, XCAP_CR_CONDITIONS);
    str_ptr = XCAP_xmlHelperMakeDocument(conds, OSAL_FALSE);
    OSAL_logMsg("condition is:\n%s\n", str_ptr);
    XCAP_xmlHelperFreeDocument(&str_ptr);
    // get identity #1
    aid = XCAP_xmlParulesParseGetIdentityFromConditions(conds, 0); 
    str_ptr = XCAP_xmlHelperMakeDocument(aid, OSAL_FALSE);
    OSAL_logMsg("condition id is:\n%s\n", str_ptr);
    XCAP_xmlHelperFreeDocument(&str_ptr);
    // get one #1
    axid = XCAP_xmlParulesParseGetOneFromIdentity(aid, 0, &str1_ptr);
    OSAL_logMsg("identity id is:\n%s\n", str1_ptr);
    // get actions
    acts = XCAP_xmlParulesParseGetElementFromRule(arule, XCAP_CR_ACTIONS);
    // get subscription handling
    subs1 = XCAP_xmlParulesParseGetSubscriptionHandlingFromActions(acts,
            &str1_ptr);
    OSAL_logMsg("subs handling is:\n%s\n", str1_ptr);
    XCAP_xmlHelperFreeDocument(&str_ptr);
    // dispose event and delete handle
    XCAP_disposeEvt(&evt);
    XCAP_xmlHelperDeleteHandle(&xml);

    // like always
    XCAP_shutdown(&obj);
    return (0);
}
 * \endcode
 *  
 * \page example2_page C Code Example2 - Elements 
 * This example demostrates how to delete, make, put, and fetch XML elements.
 * You can copy this code then build and run it.
 * \code
#include <xcap_api.h>
#include <osal.h>
#include <stdio.h>

int main()
{
    XCAP_Xml              xml;
    XCAP_Xml              alist;
    XCAP_Xml              blist;
    XCAP_Xml              aentry;
    XCAP_Xml              bentry;
    int                   len;
    char                 *str;
    XCAP_Obj              obj;
    XCAP_Cmd              cmd;
    XCAP_HttpParseHdrObj  hdrobj;
    XCAP_Evt              evt;
    char                  touri[256];

    //Init XCAP protocol.
    //All XCAP transactions will timeout after 10 seconds:
    XCAP_init(&obj, 10);
    OSAL_taskDelay(100);

    //
    // Make a resource list doc
    // Note order is important
    //
    
    xml = XCAP_xmlReslistCreateDocument();
    alist = XCAP_xmlReslistAddListToDocument(xml, "Friends", "Close friends");
    blist = XCAP_xmlReslistAddListToDocument(xml, "Business", "D2 Employees");
    aentry = XCAP_xmlReslistAddEntryToList(alist,
            "sip:2233356539@sip2sip.info", "2233356539");
    bentry = XCAP_xmlReslistAddEntryToList(blist,
            "sip:zkhan@d2tech.com", "Zubair Khan");
    str = XCAP_xmlHelperMakeDocument(xml, OSAL_TRUE);
    OSAL_logMsg("xml is:\n%s\n",str);
    
    //
    // Put it.
    //
    len = XCAP_helperMakeUri(
            "2233358499",
            "enyb73",
            "https://xcap.sipthor.net/xcap-root",
            XCAP_RESOURCE_LISTS,
            XCAP_USERS,
            "2233358499@sip2sip.info",
            NULL,
            NULL,
            touri,
            sizeof(touri) - 1);
    str = XCAP_xmlHelperMakeDocument(xml, OSAL_TRUE);
    OSAL_logMsg("xml is:\n%s\n", str);
   
    // Setup a command
    cmd.op = XCAP_OPERATION_CREATE_REPLACE;
    cmd.opType = XCAP_OPERATION_TYPE_DOCUMENT;
    cmd.uri_ptr = touri;
    cmd.auid_ptr = XCAP_RESOURCE_LISTS;
    cmd.src_ptr = str;
    cmd.srcSz = strlen(str);

    // Now send it
    XCAP_sendCmd(&obj, &cmd);

    // Wait for response
    XCAP_getEvt(&obj, &evt, -1);

    // Assume there was no error, parse the HTTP header
    // ! Do check evt.error in your code, skipped here
    XCAP_httpParseHeader(evt.hdr_ptr, strlen(evt.hdr_ptr),
            &hdrobj);

    // Print header info.
    OSAL_logMsg("event error code=%d\n", evt.error);
    OSAL_logMsg("HTTP final code=%d\n", hdrobj.finalCode);
    OSAL_logMsg("HTTP Content-Length=%d\n",hdrobj.contentLength);
    OSAL_logMsg("HTTP Content-Type=%s\n",hdrobj.contentType);
    OSAL_logMsg("HTTP ETag=%s\n", hdrobj.etag);
    OSAL_logMsg("HTTP body=\n%s\n", evt.body_ptr);
    // This is absolutely necessary
    XCAP_disposeEvt(&evt);
   
    XCAP_xmlHelperFreeDocument(&str);
    XCAP_xmlHelperDeleteHandle(&xml);

    //
    // Now repalce an element
    //
    len = XCAP_helperMakeUri(
            "2233358499",
            "enyb73",
            "https://xcap.sipthor.net/xcap-root",
            XCAP_RESOURCE_LISTS,
            XCAP_USERS,
            "2233358499@sip2sip.info",
            NULL,
            "resource-lists/list[@name=\"Friends\"]/entry"
            "[@uri=\"sip:2233356539@sip2sip.info\"]/display-name",
            touri,
            sizeof(touri) - 1);

    str = "<display-name>some name</display-name>";
    
    // Setup a command
    cmd.op = XCAP_OPERATION_CREATE_REPLACE;
    cmd.opType = XCAP_OPERATION_TYPE_ELEMENT;
    cmd.uri_ptr = touri;
    cmd.auid_ptr = XCAP_RESOURCE_LISTS;
    cmd.src_ptr = str;
    cmd.srcSz = strlen(str);
    
    // Now send it
    XCAP_sendCmd(&obj, &cmd);

    // Wait for response
    XCAP_getEvt(&obj, &evt, -1);

    // Assume there was no error, parse the HTTP header
    // ! Do check evt.error in your code, skipped here
    XCAP_httpParseHeader(evt.hdr_ptr, strlen(evt.hdr_ptr),
            &hdrobj);

    // Print header info.
    OSAL_logMsg("event error code=%d\n", evt.error);
    OSAL_logMsg("HTTP final code=%d\n", hdrobj.finalCode);
    OSAL_logMsg("HTTP Content-Length=%d\n",hdrobj.contentLength);
    OSAL_logMsg("HTTP Content-Type=%s\n",hdrobj.contentType);
    OSAL_logMsg("HTTP ETag=%s\n", hdrobj.etag);
    OSAL_logMsg("HTTP body=\n%s\n", evt.body_ptr);
    // This is absolutely necessary
    XCAP_disposeEvt(&evt);

    //
    // Now get the element back
    //
    
    // Setup a command
    cmd.op = XCAP_OPERATION_FETCH;
    cmd.opType = XCAP_OPERATION_TYPE_ELEMENT;
    cmd.uri_ptr = touri;
    cmd.auid_ptr = XCAP_RESOURCE_LISTS;
    cmd.src_ptr = NULL;
    cmd.srcSz = 0;
    
    // Now send it
    XCAP_sendCmd(&obj, &cmd);

    // Wait for response
    XCAP_getEvt(&obj, &evt, -1);

    // Assume there was no error, parse the HTTP header
    // ! Do check evt.error in your code, skipped here
    XCAP_httpParseHeader(evt.hdr_ptr, strlen(evt.hdr_ptr),
            &hdrobj);

    // Print header info.
    OSAL_logMsg("event error code=%d\n", evt.error);
    OSAL_logMsg("HTTP final code=%d\n", hdrobj.finalCode);
    OSAL_logMsg("HTTP Content-Length=%d\n",hdrobj.contentLength);
    OSAL_logMsg("HTTP Content-Type=%s\n",hdrobj.contentType);
    OSAL_logMsg("HTTP ETag=%s\n", hdrobj.etag);
    OSAL_logMsg("HTTP body=\n%s\n", evt.body_ptr);
    // This is absolutely necessary
    XCAP_disposeEvt(&evt);

    //
    // Now delete the element
    //
    
    // Setup a command
    cmd.op = XCAP_OPERATION_DELETE;
    cmd.opType = XCAP_OPERATION_TYPE_ELEMENT;
    cmd.uri_ptr = touri;
    cmd.auid_ptr = XCAP_RESOURCE_LISTS;
    cmd.src_ptr = NULL;
    cmd.srcSz = 0;
    
    // Now send it
    XCAP_sendCmd(&obj, &cmd);

    // Wait for response
    XCAP_getEvt(&obj, &evt, -1);

    // Assume there was no error, parse the HTTP header
    // ! Do check evt.error in your code, skipped here
    XCAP_httpParseHeader(evt.hdr_ptr, strlen(evt.hdr_ptr),
            &hdrobj);

    // Print header info.
    OSAL_logMsg("event error code=%d\n", evt.error);
    OSAL_logMsg("HTTP final code=%d\n", hdrobj.finalCode);
    OSAL_logMsg("HTTP Content-Length=%d\n",hdrobj.contentLength);
    OSAL_logMsg("HTTP Content-Type=%s\n",hdrobj.contentType);
    OSAL_logMsg("HTTP ETag=%s\n", hdrobj.etag);
    OSAL_logMsg("HTTP body=\n%s\n", evt.body_ptr);
    // This is absolutely necessary
    XCAP_disposeEvt(&evt);

    //
    // Now try to get the element, for error, since it got deleted
    //
    
    // Setup a command
    cmd.op = XCAP_OPERATION_FETCH;
    cmd.opType = XCAP_OPERATION_TYPE_ELEMENT;
    cmd.uri_ptr = touri;
    cmd.auid_ptr = XCAP_RESOURCE_LISTS;
    cmd.src_ptr = NULL;
    cmd.srcSz = 0;
    
    // Now send it
    XCAP_sendCmd(&obj, &cmd);

    // Wait for response
    XCAP_getEvt(&obj, &evt, -1);

    // Assume there was no error, parse the HTTP header
    // ! Do check evt.error in your code, skipped here
    XCAP_httpParseHeader(evt.hdr_ptr, strlen(evt.hdr_ptr),
            &hdrobj);

    // Print header info.
    OSAL_logMsg("event error code=%d\n", evt.error);
    OSAL_logMsg("HTTP final code=%d\n", hdrobj.finalCode);
    OSAL_logMsg("HTTP Content-Length=%d\n",hdrobj.contentLength);
    OSAL_logMsg("HTTP Content-Type=%s\n",hdrobj.contentType);
    OSAL_logMsg("HTTP ETag=%s\n", hdrobj.etag);
    OSAL_logMsg("HTTP body=\n%s\n", evt.body_ptr);
    // This is absolutely necessary
    XCAP_disposeEvt(&evt);


    // forever stuck
    XCAP_getEvt(&obj, &evt, -1);

    // like always
    XCAP_shutdown(&obj);


    return (0);
}
 * \endcode
 * \page example3_page C Code Example3 - Attributes 
 * This example demostrates how to make, put, and fetch XML attributes.
 * You can copy this code then build and run it.
 * \code

#include <xcap_api.h>
#include <osal.h>
#include <stdio.h>

int main()
{
    XCAP_Xml              xml;
    XCAP_Xml              aservice;
    XCAP_Xml              rlist;
    XCAP_Xml              apack;
    XCAP_Xml              apacks1;
    int                   len;
    char                 *str;
    XCAP_Obj              obj;
    XCAP_Cmd              cmd;
    XCAP_HttpParseHdrObj  hdrobj;
    XCAP_Evt              evt;
    char                  touri[256];
    char                  uri[256];

    //Init XCAP protocol.
    //All XCAP transactions will timeout after 10 seconds:
    XCAP_init(&obj, 10);
    OSAL_taskDelay(100);

    //
    // Make a RLS doc
    // Note order is important
    //
    xml = XCAP_xmlRlsCreateDocument();
    aservice = XCAP_xmlRlsAddServiceToDocument(xml, "sip:myclose@sip2sip.info");
    len = XCAP_helperMakeUri(
            "2233358499",
            "enyb73",
            "https://xcap.sipthor.net/xcap-root",
            XCAP_RESOURCE_LISTS,
            XCAP_USERS,
            "2233358499@sip2sip.info",
            NULL,
            "resource-lists/list[@name=\"Friends\"]",
            uri,
            sizeof(uri) - 1);
    rlist = XCAP_xmlRlsAddResourceListElementToService(aservice, uri);
    apack = XCAP_xmlRlsAddPackagesToService(aservice);
    apacks1 = XCAP_xmlRlsAddPackageToServicePackages(apack, "presence");
    str = XCAP_xmlHelperMakeDocument(xml, OSAL_TRUE);
    
    //
    // Put it.
    //

    len = XCAP_helperMakeUri(
            "2233358499",
            "enyb73",
            "https://xcap.sipthor.net/xcap-root",
            XCAP_RLS_SERVICES,
            XCAP_USERS,
            "2233358499@sip2sip.info",
            NULL,
            NULL,
            touri,
            sizeof(touri) - 1);
    str = XCAP_xmlHelperMakeDocument(xml, OSAL_TRUE);
    OSAL_logMsg("xml is:\n%s\n", str);
   
    // Setup a command
    cmd.op = XCAP_OPERATION_CREATE_REPLACE;
    cmd.opType = XCAP_OPERATION_TYPE_DOCUMENT;
    cmd.uri_ptr = touri;
    cmd.auid_ptr = XCAP_RLS_SERVICES;
    cmd.src_ptr = str;
    cmd.srcSz = strlen(str);

    // Now send it
    XCAP_sendCmd(&obj, &cmd);

    // Wait for response
    XCAP_getEvt(&obj, &evt, -1);

    // Assume there was no error, parse the HTTP header
    // ! Do check evt.error in your code, skipped here
    XCAP_httpParseHeader(evt.hdr_ptr, strlen(evt.hdr_ptr),
            &hdrobj);

    // Print header info.
    OSAL_logMsg("event error code=%d\n", evt.error);
    OSAL_logMsg("HTTP final code=%d\n", hdrobj.finalCode);
    OSAL_logMsg("HTTP Content-Length=%d\n",hdrobj.contentLength);
    OSAL_logMsg("HTTP Content-Type=%s\n",hdrobj.contentType);
    OSAL_logMsg("HTTP ETag=%s\n", hdrobj.etag);
    OSAL_logMsg("HTTP body=\n%s\n", evt.body_ptr);
    // This is absolutely necessary
    XCAP_disposeEvt(&evt);
   
    XCAP_xmlHelperFreeDocument(&str);
    XCAP_xmlHelperDeleteHandle(&xml);

    //
    // Now repalce an attribute 
    //

    len = XCAP_helperMakeUri(
            "2233358499",
            "enyb73",
            "https://xcap.sipthor.net/xcap-root",
            XCAP_RLS_SERVICES,
            XCAP_USERS,
            "2233358499@sip2sip.info",
            NULL,
            "rls-services/service[@uri=\"sip:myclose@sip2sip.info\"]/@uri",
            touri,
            sizeof(touri) - 1);

    str = "sip:zubair@sip2sip.info";
    
    // Setup a command
    cmd.op = XCAP_OPERATION_CREATE_REPLACE;
    cmd.opType = XCAP_OPERATION_TYPE_ATTRIBUTE;
    cmd.uri_ptr = touri;
    cmd.auid_ptr = XCAP_RLS_SERVICES;
    cmd.src_ptr = str;
    cmd.srcSz = strlen(str);
    
    // Now send it
    XCAP_sendCmd(&obj, &cmd);

    // Wait for response
    XCAP_getEvt(&obj, &evt, -1);

    // Assume there was no error, parse the HTTP header
    // ! Do check evt.error in your code, skipped here
    XCAP_httpParseHeader(evt.hdr_ptr, strlen(evt.hdr_ptr),
            &hdrobj);

    // Print header info.
    OSAL_logMsg("event error code=%d\n", evt.error);
    OSAL_logMsg("HTTP final code=%d\n", hdrobj.finalCode);
    OSAL_logMsg("HTTP Content-Length=%d\n",hdrobj.contentLength);
    OSAL_logMsg("HTTP Content-Type=%s\n",hdrobj.contentType);
    OSAL_logMsg("HTTP ETag=%s\n", hdrobj.etag);
    OSAL_logMsg("HTTP body=\n%s\n", evt.body_ptr);
    // This is absolutely necessary
    XCAP_disposeEvt(&evt);

    //
    // Now fetch the attribute, note URI has changed after put
    //
    
    len = XCAP_helperMakeUri(
            "2233358499",
            "enyb73",
            "https://xcap.sipthor.net/xcap-root",
            XCAP_RLS_SERVICES,
            XCAP_USERS,
            "2233358499@sip2sip.info",
            NULL,
            "rls-services/service[@uri=\"sip:zubair@sip2sip.info\"]/@uri",
            touri,
            sizeof(touri) - 1);
    
    // Setup a command
    cmd.op = XCAP_OPERATION_FETCH;
    cmd.opType = XCAP_OPERATION_TYPE_ATTRIBUTE;
    cmd.uri_ptr = touri;
    cmd.auid_ptr = XCAP_RLS_SERVICES;
    cmd.src_ptr = NULL;
    cmd.srcSz = 0;
    
    // Now send it
    XCAP_sendCmd(&obj, &cmd);

    // Wait for response
    XCAP_getEvt(&obj, &evt, -1);

    // Assume there was no error, parse the HTTP header
    // ! Do check evt.error in your code, skipped here
    XCAP_httpParseHeader(evt.hdr_ptr, strlen(evt.hdr_ptr),
            &hdrobj);

    // Print header info.
    OSAL_logMsg("event error code=%d\n", evt.error);
    OSAL_logMsg("HTTP final code=%d\n", hdrobj.finalCode);
    OSAL_logMsg("HTTP Content-Length=%d\n",hdrobj.contentLength);
    OSAL_logMsg("HTTP Content-Type=%s\n",hdrobj.contentType);
    OSAL_logMsg("HTTP ETag=%s\n", hdrobj.etag);
    OSAL_logMsg("HTTP body=\n%s\n", evt.body_ptr);
    // This is absolutely necessary
    XCAP_disposeEvt(&evt);

    //
    // Now try to get the attribute, to verify 
    //
    
    // Setup a command
    cmd.op = XCAP_OPERATION_FETCH;
    cmd.opType = XCAP_OPERATION_TYPE_ATTRIBUTE;
    cmd.uri_ptr = touri;
    cmd.auid_ptr = XCAP_RLS_SERVICES;
    cmd.src_ptr = NULL;
    cmd.srcSz = 0;
    
    // Now send it
    XCAP_sendCmd(&obj, &cmd);

    // Wait for response
    XCAP_getEvt(&obj, &evt, -1);

    // Assume there was no error, parse the HTTP header
    // ! Do check evt.error in your code, skipped here
    XCAP_httpParseHeader(evt.hdr_ptr, strlen(evt.hdr_ptr),
            &hdrobj);

    // Print header info.
    OSAL_logMsg("event error code=%d\n", evt.error);
    OSAL_logMsg("HTTP final code=%d\n", hdrobj.finalCode);
    OSAL_logMsg("HTTP Content-Length=%d\n",hdrobj.contentLength);
    OSAL_logMsg("HTTP Content-Type=%s\n",hdrobj.contentType);
    OSAL_logMsg("HTTP ETag=%s\n", hdrobj.etag);
    OSAL_logMsg("HTTP body=\n%s\n", evt.body_ptr);
    // This is absolutely necessary
    XCAP_disposeEvt(&evt);


    // forever stuck
    XCAP_getEvt(&obj, &evt, -1);

    // like always
    XCAP_shutdown(&obj);


    return (0);
}
 * \endcode
 */


/** \file
 * \brief Include this file for all XCAP public API.
 */

#include "xcap_helper.h"
#include "xcap_xml_helper.h"
#include "xcap_xml_reslist.h"
#include "xcap_xml_reslist_parse.h"
#include "xcap_xml_rls.h"
#include "xcap_xml_rls_parse.h"
#include "xcap_xml_parules.h"
#include "xcap_xml_parules_parse.h"
#include "xcap_resources.h"
#include "xcap_http_parse.h"
#include "xcap.h"

#endif
