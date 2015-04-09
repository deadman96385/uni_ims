/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 12486 $ $Date: 2010-07-08 06:10:49 +0800 (Thu, 08 Jul 2010) $
 *
 */

/** \file
 * \brief RLS services document preparation and manipulation.
 *  
 * This is an (not so complete) implementation of RFC 4826.
 * Application uses this API to make XML documents from scratch, or modify
 * existing XML documents. Once a document is finalized, it can be converted to
 * XML text using call to #XCAP_xmlHelperMakeDocument.
 * Remains to be implemented:\n
 * - resource lists embedded in service\n
 */

#ifndef _XCAP_XML_RLS_H_
#define _XCAP_XML_RLS_H_

/*
 * Function prototypes.
 */

/** \fn XCAP_Xml XCAP_xmlRlsCreateDocument(
 *          void)
 * \brief Create a RLS services document.
 *
 * This function is the starting point for creating a RLS services document in 
 * memory.\n
 * XML document after this call succeeds looks like:\n
 * \verbatim
 * <rls-services xmlns="urn:ietf:params:xml:ns:rls-services"
 *     xmlns:rl="urn:ietf:params:xml:ns:resource-lists"
 *     xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance">
 * </rls-services>
 * \endverbatim
 *
 * @return A handle to a RLS services document. This handle is
 * used later for adding elements and attributes to the document.\n
 * At the return of the function, the RLS document is empty and contains no 
 * service. This handle will be freed using #XCAP_xmlHelperDeleteHandle when the
 * document is not needed. If handle is NULL, document creation failed.
 */
XCAP_Xml XCAP_xmlRlsCreateDocument(
    void);

/** \fn XCAP_Xml XCAP_xmlRlsAddServiceToDocument(
 *          XCAP_Xml  doc,
 *          char     *uri_ptr)
 * \brief Create a service in RLS services document.
 *
 * This function adds a service in a RLS services document with a given uri.
 * For example continuing from example of #XCAP_xmlRlsCreateDocument, 
 * XML document after this call succeeds looks like:\n
 * \verbatim
 * <rls-services xmlns="urn:ietf:params:xml:ns:rls-services"
 *     xmlns:rl="urn:ietf:params:xml:ns:resource-lists"
 *     xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance">
 *     <service uri="sip:mybuddies@example.com">
 *     </service>
 * </rls-services>
 * \endverbatim
 *
 * @param doc Handle of XML doument returned by call to 
 * #XCAP_xmlRlsCreateDocument.
 * @param uri_ptr A NULL terminated string containing URI of the service to be
 * created. In the above example, uri_ptr is set as 
 * uri_ptr = "sip:mybuddies@example.com".
 * @return A Handle to the service created, or NULL pointer indicating an error.
 */
XCAP_Xml XCAP_xmlRlsAddServiceToDocument(
    XCAP_Xml  doc,
    char     *uri_ptr);

/** \fn XCAP_Xml XCAP_xmlRlsAddPackagesToService(
 *          XCAP_Xml service)
 * \brief Adds packages to a service.
 *
 * This function adds packages to a service. At least one package must be added
 * to packages by calling #XCAP_xmlRlsAddPackageToServicePackages after this
 * call succeeds.
 *
 * For example continuing from example of #XCAP_xmlRlsAddServiceToDocument,
 * following call, if successful, will add packages to the service:
 * \verbatim
 * XCAP_xmlRlsAddPackagesToService(service);
 * \endverbatim
 * XML document after above call succeeds looks like:\n
 * \verbatim
 * <rls-services xmlns="urn:ietf:params:xml:ns:rls-services"
 *     xmlns:rl="urn:ietf:params:xml:ns:resource-lists"
 *     xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance">
 *     <service uri="sip:mybuddies@example.com">
 *         <packages>
 *         </packages>
 *     </service>
 * </rls-services>
 * \endverbatim
 *
 * @param service Handle returned by call to 
 * #XCAP_xmlRlsAddServiceToDocument.
 * @return A Handle to the packages created, or NULL pointer indicating an
 * error.
 */
XCAP_Xml XCAP_xmlRlsAddPackagesToService(
    XCAP_Xml service);

/** \fn XCAP_Xml XCAP_xmlRlsAddPackageToServicePackages(
 *          XCAP_Xml  packages,
 *          char     *packageName_ptr)
 * \brief Adds a package to packages in a service.
 *
 * This function adds a package to packages in a service and sets its value.
 * For example continuing from example of
 * #XCAP_xmlRlsAddPackagesToService, following call, if successful,
 * will add a package named presence to packages:
 * \verbatim
 * XCAP_xmlRlsAddPackageToServicePackages(packages, "presence");
 * \endverbatim
 * XML document after above call succeeds looks like:\n
 * \verbatim
 * <rls-services xmlns="urn:ietf:params:xml:ns:rls-services"
 *     xmlns:rl="urn:ietf:params:xml:ns:resource-lists"
 *     xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance">
 *     <service uri="sip:mybuddies@example.com">
 *         <packages>
 *             <package>presence</package>
 *         </packages>
 *     </service>
 * </rls-services>
 * \endverbatim
 *
 * @param packages Handle returned by call to 
 * #XCAP_xmlRlsAddPackagesToService.
 * @param packageName_ptr Set to required package name.
 * @return handle to the package created, or NULL pointer indicating an error.
 */
XCAP_Xml XCAP_xmlRlsAddPackageToServicePackages(
    XCAP_Xml  packages,
    char     *packageName_ptr);

/** \fn XCAP_Xml XCAP_xmlRlsAddResourceListElementToService(
 *          XCAP_Xml  service,
 *          char     *uri_ptr)
 * \brief Adds a resource-list element in a service.
 *
 * This function adds a an element resource-list in a service, and sets its
 * value specified by uri_ptr.
 * For example continuing from example of
 * #XCAP_xmlRlsAddPackageToServicePackages, following call, if successful,
 * will add a resource-list element to a service, then sets its value to URI
 * pointed by uri_ptr:
 * \verbatim
 * XCAP_xmlRlsAddResourceListElementToService(service, uri_ptr);
 * \endverbatim
 * XML document after above call succeeds looks like (URI folding for clarity
 * only):\n
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
 *
 * @param service Handle returned by call to 
 * #XCAP_xmlRlsAddServiceToDocument.
 * @param uri_ptr Set to required URI value to be set. Make this URI using
 * helper URI generation call #XCAP_helperMakeUri.
 * @return handle to the resource-list created, or NULL pointer indicating an
 * error.
 */
XCAP_Xml XCAP_xmlRlsAddResourceListElementToService(
    XCAP_Xml  service,
    char     *uri_ptr);

#endif
