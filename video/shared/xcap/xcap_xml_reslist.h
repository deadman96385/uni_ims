/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 12486 $ $Date: 2010-07-08 06:10:49 +0800 (Thu, 08 Jul 2010) $
 *
 */

/** \file
 * \brief Resource lists document preparation and manipulation.
 *  
 * This is an (not so complete) implementation of RFC 4826.
 * Application uses this API to make XML documents from scratch, or modify
 * existing XML documents. Once a document is finalized, it can be converted to
 * XML text using call to #XCAP_xmlHelperMakeDocument.
 * Remains to be implemented:\n
 * - entry-ref not supported
 */

#ifndef _XCAP_XML_RESLIST_H_
#define _XCAP_XML_RESLIST_H_

/*
 * Function prototypes.
 */

/** \fn XCAP_Xml XCAP_xmlReslistCreateDocument(
 *          void)
 * \brief Create a resource lists document.
 *
 * This function is the starting point for creating a resource lists document
 * in memory.\n
 * XML document after this call succeeds looks like:\n
 * \verbatim
 * <resource-lists xmlns="urn:ietf:params:xml:ns:resource-lists"
 *     xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance">
 * </resource-lists>
 * \endverbatim
 *
 * @return A handle to a resource lists document. This handle is
 * used later for adding elements and attributes to the document.\n
 * At the return of the function, the resource lists document is empty and
 * contains no list. This handle will be freed using
 * #XCAP_xmlHelperDeleteHandle when the document is not needed.
 * If handle is NULL, document creation failed.
 */
XCAP_Xml XCAP_xmlReslistCreateDocument(
    void);

/** \fn XCAP_Xml XCAP_xmlReslistAddListToDocument(
 *          XCAP_Xml  doc,
 *          char     *listName_ptr,
 *          char     *listDisplayName_ptr);
 * \brief Create a list in resource lists document.
 *
 * This function adds a list in a resource lists document with a given name and
 * display name.
 * For example continuing from example of #XCAP_xmlReslistCreateDocument, 
 * XML document after this call succeeds looks like:\n
 * \verbatim
 * <resource-lists xmlns="urn:ietf:params:xml:ns:resource-lists"
 *     xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance">
 *     <list name="friends">
 *         <display-name>Close Friends</display-name>
 *     </list>
 * </resource-lists>
 * \endverbatim
 *
 * @param doc Handle of XML document returned by call to 
 * #XCAP_xmlReslistCreateDocument.
 * @param listName_ptr A NULL terminated string containing name of list to be
 * created. In the above example, listName_ptr is set as 
 * listName_ptr = "friends".
 * @param listDisplayName_ptr A NULL terminated string containing display name
 * of list to be created. In the above example, listDisplayName_ptr is set as 
 * listDisplayName_ptr = "Close Friends".
 * @return A Handle to the list created, or NULL pointer indicating an error.
 */
XCAP_Xml XCAP_xmlReslistAddListToDocument(
    XCAP_Xml  doc,
    char     *listName_ptr,
    char     *listDisplayName_ptr);

/** \fn XCAP_Xml XCAP_xmlReslistAddEntryToList(
 *          XCAP_Xml  list,
 *          char     *uri_ptr,
 *          char     *displayName_ptr);
 * \brief Create an entry in a list in resource lists document.
 *
 * This function adds an entry in a list in resource lists document with a 
 * given URI and display name.
 * For example continuing from example of #XCAP_xmlReslistAddListToDocument, 
 * XML document after this call succeeds looks like:\n
 * \verbatim
 * <resource-lists xmlns="urn:ietf:params:xml:ns:resource-lists"
 *     xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance">
 *     <list name="friends">
 *         <display-name>Close Friends</display-name>
 *             <entry uri="sip:joe@example.com">
 *                  <display-name>Joe Smith</display-name>
 *             </entry>
 *     </list>
 * </resource-lists>
 * \endverbatim
 *
 * @param list Handle of an XML list returned by call to 
 * #XCAP_xmlReslistAddListToDocument.
 * @param uri_ptr A NULL terminated string containing URI of entry to be
 * created. In the above example, uri_ptr is set as 
 * uri_ptr = "sip:joe@example.com".
 * @param displayName_ptr A NULL terminated string containing display name
 * of entry to be created. In the above example, displayName_ptr is set as 
 * displayName_ptr = "Joe Smith".
 * @return A Handle to the entry created, or NULL pointer indicating an error.
 */
XCAP_Xml XCAP_xmlReslistAddEntryToList(
    XCAP_Xml  list,
    char     *uri_ptr,
    char     *displayName_ptr);
     
#endif
