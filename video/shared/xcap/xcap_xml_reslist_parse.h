/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 12486 $ $Date: 2010-07-08 06:10:49 +0800 (Thu, 08 Jul 2010) $
 *
 */

/** \file
 * \brief Resource lists document parsing.
 *  
 * This is a (not so complete) resource list document parsing implementation as
 * specified in RFC 4826.
 * Application uses this API to parse XML documents, or modify
 * existing XML documents.
 * Remains to be implemented:\n
 * - entry-ref not supported
 *
 * Example resource lists XML document looks like:
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
 */

#ifndef _XCAP_XML_RESLIST_PARSE_H_
#define _XCAP_XML_RESLIST_PARSE_H_

/*
 * Function prototypes.
 */

/** \fn XCAP_Xml XCAP_xmlReslistParseDocument(
 *          char *doc_ptr,
 *          int   docLen);
 * \brief Parse a resource lists document.
 *
 * This function completely parses a resource lists document in memory and
 * assigns a handle to it. Document is parsed from a text string containing that
 * document.\n
 *
 * @param doc_ptr A NULL terminated string containing the XML document to be
 * parsed. The document must be a resource lists document.
 * @param docLen Length of the document placed in buffer pointed by doc_ptr.
 * @return A handle to a resource lists document. This handle is
 * used later for getting elements and attributes from the document.\n
 * The handle will be freed using
 * #XCAP_xmlHelperDeleteHandle when the document is not needed.\n
 * If handle is NULL, document parsing failed.
 */
XCAP_Xml XCAP_xmlReslistParseDocument(
     char *doc_ptr,
     int   docLen);

/** \fn XCAP_Xml XCAP_xmlReslistParseGetList(
 *          XCAP_Xml   doc,
 *          int        listNumber,
 *          char     **listName_ptr,
 *          char     **listDisplayName_ptr);
 * \brief Parse a list from a resource list document.
 *
 * This function gets a list from a parsed XML resource lists
 * document, at a specified index, parsed by call to
 * #XCAP_xmlReslistParseDocument.
 * For example following call will get first list from example document:
 * \verbatim
 * doc = XCAP_xmlReslistParseDocument();
 * XCAP_xmlReslistParseGetList(doc, 0, &name_ptr, &dname_ptr);
 * \endverbatim
 * The list will look like:
 * \verbatim
 *     <list name="friends">
 *         <display-name>Close Friends</display-name>
 *             <entry uri="sip:joe@example.com">
 *                  <display-name>Joe Smith</display-name>
 *             </entry>
 *     </list>
 * \endverbatim
 * And pointers\n
 * name_ptr will point to NULL terminated string "friends"\n
 * dname_ptr will point to NULL terminated string "Close Friends"\n
 *
 * @param doc Handle to XML document returned by call to 
 * #XCAP_xmlReslistParseDocument.
 * @param listNumber Index of list. User can interate on index to get all lists
 * in the document, till function returns NULL.
 * @param listName_ptr A pointer to a pointer, to be pointed to name of list.
 * @param listDisplayName_ptr A pointer to a pointer,
 * to be pointed to display-name of list.
 * @return A handle to a resource list. This handle is
 * used later for getting elements and attributes from the list.\n
 * If handle is NULL, no list was found at the specified index.
 */
XCAP_Xml XCAP_xmlReslistParseGetList(
    XCAP_Xml   doc,
    int        listNumber,
    char     **listName_ptr,
    char     **listDisplayName_ptr);

/** \fn XCAP_Xml XCAP_xmlReslistParseGetEntry(
 *          XCAP_Xml   list,
 *          int        entryNumber,
 *          char     **uri_ptr,
 *          char     **displayName_ptr);
 * \brief Parse a list from a resource list document.
 *
 * This function gets an entry from a parsed XML resource lists
 * document, from a list, at a specified index, parsed by call to
 * #XCAP_xmlReslistParseDocument in a list obtained by call to 
 * #XCAP_xmlReslistParseGetList.
 * For example contiuning from example of #XCAP_xmlReslistParseGetList, 
 * following call will get first entry from example document:
 * \verbatim
 * XCAP_xmlReslistParseGetEntry(list, 0, &uri_ptr, &dname_ptr);
 * \endverbatim
 * The entry will look like:
 * \verbatim
 *             <entry uri="sip:joe@example.com">
 *                  <display-name>Joe Smith</display-name>
 *             </entry>
 * \endverbatim
 * And pointers\n
 * uri_ptr will point to NULL terminated string "sip:joe@example.com"\n
 * dname_ptr will point to NULL terminated string "Joe Smith"\n
 *
 * @param list Handle to list returned by call to 
 * #XCAP_xmlReslistParseGetList.
 * @param entryNumber Index of entry. User can interate on index to get all 
 * entries in the list, till function returns NULL.
 * @param uri_ptr A pointer to a pointer, to be pointed to uri of entry.
 * @param displayName_ptr A pointer to a pointer, to be pointed to 
 * display-name of entry.
 * @return A handle to an entry. This handle is
 * used later for getting elements and attributes from the entry.\n
 * If handle is NULL, no entry was found at the specified index.
 */
XCAP_Xml XCAP_xmlReslistParseGetEntry(
    XCAP_Xml   list,
    int        entryNumber,
    char     **uri_ptr,
    char     **displayName_ptr);

#endif
