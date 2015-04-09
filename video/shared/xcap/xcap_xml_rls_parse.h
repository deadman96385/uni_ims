/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 12486 $ $Date: 2010-07-08 06:10:49 +0800 (Thu, 08 Jul 2010) $
 *
 */

/** \file
 * \brief RLS services document parsing.
 *  
 * This is a (not so complete) RLS services document parsing implementation as
 * specified in RFC 4826.
 * Application uses this API to parse XML documents, or modify
 * existing XML documents.
 * Remains to be implemented:\n
 * - resource lists embedded in service\n
 *
 * Example RLS services XML document looks like:
 * \verbatim
 * <rls-services xmlns="urn:ietf:params:xml:ns:rls-services"
 *     xmlns:rl="urn:ietf:params:xml:ns:resource-lists"
 *     xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance">
 *     <service uri="sip:mybuddies@example.com">
 *         <packages>
 *             <package>presence</package>
 *         </packages>
 *         <resource-list>
 *             http://xcap.example.com/resource-lists/users/
 *             sip:joe@example.com/index/~~/
 *             /resource-lists/list%5b@name=%22l1%22%5d
 *         </resource-list>
 *     </service>
 * </rls-services>
 * \endverbatim
 */

#ifndef _XCAP_XML_RLS_PARSE_H_
#define _XCAP_XML_RLS_PARSE_H_

/*
 * Function prototypes.
 */

/** \fn XCAP_Xml XCAP_xmlRlsParseDocument(
 *          char *doc_ptr,
 *          int   docLen);
 * \brief Parse a RLS services document.
 *
 * This function completely parses a RLS services document in memory and
 * assigns a handle to it. Document is parsed from a text string containing that
 * document.\n
 *
 * @param doc_ptr A NULL terminated string containing the XML document to be
 * parsed. The document must be a RLS services document.
 * @param docLen Length of the document placed in buffer pointed by doc_ptr.
 * @return A handle to a RLS services document. This handle is
 * used later for getting elements and attributes from the document.\n
 * The handle will be freed using
 * #XCAP_xmlHelperDeleteHandle when the document is not needed.\n
 * If handle is NULL, document parsing failed.
 */
XCAP_Xml XCAP_xmlRlsParseDocument(
     char *doc_ptr,
     int   docLen);

/** \fn XCAP_Xml XCAP_xmlRlsParseGetService(
 *          XCAP_Xml   doc,
 *          int        serviceNumber,
 *          char     **uri_ptr);
 * \brief Parse a service from a RLS services document.
 *
 * This function gets a service from a parsed XML RLS services
 * document, at a specified index, parsed by call to
 * #XCAP_xmlRlsParseDocument.
 * For example following call will get first service from example document:
 * \verbatim
 * doc = XCAP_xmlRlsParseDocument();
 * XCAP_xmlRlsParseGetService(doc, 0, &uri_ptr);
 * \endverbatim
 * The service will look like:
 * \verbatim
 *     <service uri="sip:mybuddies@example.com">
 *         <packages>
 *             <package>presence</package>
 *         </packages>
 *         <resource-list>
 *             http://xcap.example.com/resource-lists/users/
 *             sip:joe@example.com/index/~~/
 *             /resource-lists/list%5b@name=%22l1%22%5d
 *         </resource-list>
 *     </service>
 * \endverbatim
 * And pointer\n
 * uri_ptr will point to NULL terminated string "sip:mybuddies@example.com"\n
 *
 * @param doc Handle to XML document returned by call to 
 * #XCAP_xmlRlsParseDocument.
 * @param serviceNumber Index of service. User can iterate on index to get 
 * all services in the document, till function returns NULL.
 * @param uri_ptr A pointer to a pointer, to be pointed to a NULL
 * terminated string containing URI of service.
 * @return A handle to a service. This handle is
 * used later for getting elements and attributes from the service.\n
 * If handle is NULL, no service was found at the specified index.
 */
XCAP_Xml XCAP_xmlRlsParseGetService(
    XCAP_Xml   doc,
    int        serviceNumber,
    char     **uri_ptr);

/** \fn XCAP_Xml XCAP_xmlRlsParseGetPackagesFromService(
 *          XCAP_Xml service);
 * \brief Parse packages from a service in RLS services document.
 *
 * This function gets packages element from a parsed XML
 * RLS services document parsed by call to
 * #XCAP_xmlRlsParseDocument, in a service obtained by call to 
 * #XCAP_xmlRlsParseGetService.
 * For example continuing from example of #XCAP_xmlRlsParseGetService, following
 * call will get packages from example document:
 * \verbatim
 * XCAP_xmlRlsParseGetPackagesFromService(service);
 * \endverbatim
 * The packages will look like:
 * \verbatim
 *         <packages>
 *             <package>presence</package>
 *         </packages>
 * \endverbatim
 *
 * @param service Handle to service returned by call to 
 * #XCAP_xmlRlsParseGetService.
 * @return A handle to packages in the service. This handle is
 * used later for getting elements and attributes from the packages.\n
 * If handle is NULL, no packages element was found in the service.
 */
XCAP_Xml XCAP_xmlRlsParseGetPackagesFromService(
    XCAP_Xml service);

/** \fn XCAP_Xml XCAP_xmlRlsParseGetPackageFromServicePackages(
 *          XCAP_Xml   packages,
 *          char     **packageName_ptr);
 * \brief Parse package from packages in a service in RLS services document.
 *
 * This function gets package element from a parsed XML
 * RLS services document parsed by call to
 * #XCAP_xmlRlsParseDocument, in a service obtained by call to 
 * #XCAP_xmlRlsParseGetService, in packages obtained by call to
 * #XCAP_xmlRlsParseGetPackagesFromService.
 * For example continuing from example of
 * #XCAP_xmlRlsParseGetPackagesFromService, following
 * call will get package from example document:
 * \verbatim
 * XCAP_xmlRlsParseGetPackageFromServicePackages(packages);
 * \endverbatim
 * The package will look like:
 * \verbatim
 *             <package>presence</package>
 * \endverbatim
 * And pointer\n
 * packageName_ptr will point to a NULL terminated string "presence".
 *
 * @param packages Handle to packages returned by call to 
 * #XCAP_xmlRlsParseGetPackagesFromService.
 * @param packageName_ptr is a pointer to a pointer, to be pointed to a NULL
 * terminated string containing value of the package.
 * @return A handle to package in the packages. This handle is
 * used later for getting elements and attributes from the package.\n
 * If handle is NULL, no packages element was found in the package.
 */
XCAP_Xml XCAP_xmlRlsParseGetPackageFromServicePackages(
    XCAP_Xml   packages,
    char     **packageName_ptr);

/** \fn XCAP_Xml XCAP_xmlRlsParseGetResourceListElementFromService(
 *          XCAP_Xml   service,
 *          char     **uri_ptr);
 * \brief Parse resource-list element from a service in RLS services document.
 *
 * This function gets resource-list element from a parsed XML
 * RLS services document parsed by call to
 * #XCAP_xmlRlsParseDocument, in a service obtained by call to 
 * #XCAP_xmlRlsParseGetService.
 * For example continuing from example of
 * #XCAP_xmlRlsParseGetService, following call will get resource-list from
 * example document:
 * \verbatim
 * XCAP_xmlRlsParseGetResourceListElementFromService(service);
 * \endverbatim
 * The element will look like (URI folding for clarity only):
 * \verbatim
 *         <resource-list>
 *             http://xcap.example.com/resource-lists/users/
 *             sip:joe@example.com/index/~~/
 *             /resource-lists/list%5b@name=%22l1%22%5d
 *         </resource-list>
 * \endverbatim
 * And pointer\n
 * uri_ptr will point to a NULL terminated string
 * "http://xcap.example.com/resource-lists/users/"
 * "sip:joe@example.com/index/~~/"
 * "/resource-lists/list%5b@name=%22l1%22%5d"
 *
 * @param service Handle to service returned by call to 
 * #XCAP_xmlRlsParseGetService.
 * @param uri_ptr is a pointer to a pointer, to be pointed to a NULL terminated
 * string containing URI value of the resource-list element.
 * @return A handle to resource-list element in the service. This handle is
 * used later for getting elements and attributes from the element.\n
 * If handle is NULL, no resource-list element was found in the service.
 */
XCAP_Xml XCAP_xmlRlsParseGetResourceListElementFromService(
    XCAP_Xml   service,
    char     **uri_ptr);

#endif
