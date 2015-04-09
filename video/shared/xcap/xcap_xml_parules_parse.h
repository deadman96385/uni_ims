/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 12486 $ $Date: 2010-07-08 06:10:49 +0800 (Thu, 08 Jul 2010) $
 *
 */

/** \file
 * \brief Presence authorization rules document parsing.
 *  
 * This is a (not so complete) presence authorization rules document parsing
 * implementation as specified in RFC 5025.
 * Application uses this API to parse XML documents, or modify
 * existing XML documents.
 * Remains to be implemented:\n
 * - actions not fully supported: identity and sphere\n
 * - transformation not fully supported: no attributes implemented\n
 * - conditions not fully supported: many except id type not supported\n
 * - resource lists embedded in service\n
 *
 * Example presence authorization rules XML document looks like:
 * \verbatim
 * <cr:ruleset xmlns="urn:ietf:params:xml:ns:pres-rules"
 *     xmlns:pr="urn:ietf:params:xml:ns:pres-rules"
 *     xmlns:cr="urn:ietf:params:xml:ns:common-policy">
 *     <cr:rule id="someid">
 *         <cr:conditions>
 *             <cr:identity>
 *                 <cr:one id="sip:user@example.com">
 *                 </cr:one>
 *             </cr:identity>
 *         </cr:conditions>
 *         <cr:actions>
 *             <pr:sub-handling>allow</pr:sub-handling>
 *         </cr:actions>
 *         <cr:transformations>
 *         </cr:transformations>
 *     </cr:rule>
 * </cr:ruleset>
 * \endverbatim
 */

#ifndef _XCAP_XML_PARULES_PARSE_H_
#define _XCAP_XML_PARULES_PARSE_H_

/*
 * Function prototypes.
 */

/** \fn XCAP_Xml XCAP_xmlParulesParseDocument(
 *          char *doc_ptr,
 *          int   docLen);
 * \brief Parse a presence authorization rules document.
 *
 * This function completely parses a presence authorization rules document 
 * in memory and assigns a handle to it.
 * Document is parsed from a text string containing that document.\n
 *
 * @param doc_ptr A NULL terminated string containing the XML document to be
 * parsed. The document must be a presence authorization rules document.
 * @param docLen Length of the document placed in buffer pointed by doc_ptr.
 * @return A handle to a presence authorization rules document. This handle is
 * used later for getting elements and attributes from the document.\n
 * The handle will be freed using
 * #XCAP_xmlHelperDeleteHandle when the document is not needed.\n
 * If handle is NULL, document parsing failed.
 */
XCAP_Xml XCAP_xmlParulesParseDocument(
     char *doc_ptr,
     int   docLen);

/** \fn XCAP_Xml XCAP_xmlParulesParseGetRule(
 *          XCAP_Xml   doc,
 *          int        ruleNumber,
 *          char     **id_ptr);
 * \brief Parse a rule from a prsence authorization rules document.
 *
 * This function gets a rule from a parsed XML presence authorization rules
 * document, at a specified index, parsed by call to
 * #XCAP_xmlParulesParseDocument.
 * For example following call will get first rule from example document:
 * \verbatim
 * rule = XCAP_xmlParulesParseDocument();
 * XCAP_xmlParulesParseGetRule(doc, 0, &id_ptr);
 * \endverbatim
 * The rule will look like:
 * \verbatim
 *     <cr:rule id="someid">
 *         <cr:conditions>
 *             <cr:identity>
 *                 <cr:one id="sip:user@example.com">
 *                 </cr:one>
 *             </cr:identity>
 *         </cr:conditions>
 *         <cr:actions>
 *             <pr:sub-handling>allow</pr:sub-handling>
 *         </cr:actions>
 *         <cr:transformations>
 *         </cr:transformations>
 *     </cr:rule>
 * \endverbatim
 * And pointer\n
 * id_ptr will point to NULL terminated string "someid"\n
 *
 * @param doc Handle to XML document returned by call to 
 * #XCAP_xmlParulesParseDocument.
 * @param ruleNumber Index of rule. User can iterate on index to get 
 * all rules in the document, till function returns NULL.
 * @param id_ptr A pointer to a pointer, to be pointed to a NULL
 * terminated string containing id of rule.
 * @return A handle to a rule. This handle is
 * used later for getting elements and attributes from the rule.\n
 * If handle is NULL, no rule was found at the specified index.
 */
XCAP_Xml XCAP_xmlParulesParseGetRule(
    XCAP_Xml   doc,
    int        ruleNumber,
    char     **id_ptr);

/** \fn XCAP_Xml XCAP_xmlParulesParseGetElementFromRule(
 *          XCAP_Xml  rule,
 *          char     *name_ptr);
 * \brief Parse one of conditions, actions, or transformations element from a 
 * prsence authorization rules document.
 *
 * This function gets conditions, actions, or transformations element from a
 * rule obtained by call to #XCAP_xmlParulesParseGetRule in a parsed XML
 * presence authorization rules document parsed by call to
 * #XCAP_xmlParulesParseDocument.
 * For example continuing from example of #XCAP_xmlParulesParseGetRule, 
 * following calls will get conditions, actions, and transformations
 * from first rule in example document:
 * \verbatim
 * XCAP_xmlParulesParseGetElementFromRule(rule, XCAP_CR_CONDITIONS);
 * XCAP_xmlParulesParseGetElementFromRule(rule, XCAP_CR_ACTIONS);
 * XCAP_xmlParulesParseGetElementFromRule(rule, XCAP_CR_TRANFORMATIONS);
 * \endverbatim
 * Corresponding elements obtained will look like:
 * \verbatim
 * -- first call
 *         <cr:conditions>
 *             <cr:identity>
 *                 <cr:one id="sip:user@example.com">
 *                 </cr:one>
 *             </cr:identity>
 *         </cr:conditions>
 * -- second call
 *         <cr:actions>
 *             <pr:sub-handling>allow</pr:sub-handling>
 *         </cr:actions>
 * -- third call
 *         <cr:transformations>
 *         </cr:transformations>
 * \endverbatim
 *
 * @param rule Handle to rule returned by call to 
 * #XCAP_xmlParulesParseGetRule.
 * @param name_ptr Must be set to either #XCAP_CR_CONDITIONS, #XCAP_CR_ACTIONS,
 * or #XCAP_CR_TRANSFORMATIONS.
 * @return A handle to an element obtained. This handle is
 * used later for getting elements and attributes from the element.\n
 * If handle is NULL, no element was found with specified name.
 */
XCAP_Xml XCAP_xmlParulesParseGetElementFromRule(
    XCAP_Xml  rule,
    char     *name_ptr);

/** \fn XCAP_Xml XCAP_xmlParulesParseGetIdentityFromConditions(
 *          XCAP_Xml conditions,
 *          int      identityNumber);
 * \brief Parse identity from conditions in a rule in presence authorization
 * document.
 *
 * This function gets an indentity from conditions obtained by call to 
 * #XCAP_xmlParulesParseGetElementFromRule, in a rule obtained by call to
 * #XCAP_xmlParulesParseGetRule, in a presence authorization rules document
 * parsed by call to #XCAP_xmlParulesParseDocument.
 * For example continuing from example of 
 * #XCAP_xmlParulesParseGetElementFromRule, following call will get first 
 * identity from conditions in first rule in example document:
 * \verbatim
 * XCAP_xmlParulesParseGetIdentityFromConditions(conditions, 0);
 * \endverbatim
 * Identity obtained will look like:
 * \verbatim
 *             <cr:identity>
 *                 <cr:one id="sip:user@example.com">
 *                 </cr:one>
 *             </cr:identity>
 * \endverbatim
 *
 * @param conditions Handle to conditions returned by call to 
 * #XCAP_xmlParulesParseGetElementFromRule.
 * @param identityNumber Index of identity. User can iterate on index to get all
 * identities in conditions, till function returns NULL.
 * @return A handle to an identity  obtained. This handle is
 * used later for getting elements and attributes from the identity.\n
 * If handle is NULL, no identity was found at the specified index.
 */
XCAP_Xml XCAP_xmlParulesParseGetIdentityFromConditions(
    XCAP_Xml conditions,
    int      identityNumber);

/** \fn XCAP_Xml XCAP_xmlParulesParseGetOneFromIdentity(
 *          XCAP_Xml   identity,
 *          int        oneNumber,
 *          char     **id_ptr);
 * \brief Parse a one id from a presence authorization rules document.
 *
 * This function gets a one id from an indentity obtained by call to
 * #XCAP_xmlParulesParseGetIdentityFromConditions, in conditions obtained by 
 * call to #XCAP_xmlParulesParseGetElementFromRule, in a rule obtained by call
 * to #XCAP_xmlParulesParseGetRule, in a presence authorization rules document
 * parsed by call to #XCAP_xmlParulesParseDocument.
 * For example continuing from example of
 * #XCAP_xmlParulesParseGetIdentityFromConditions,
 * following call will get first one id in example document:
 * This function gets a one id from an identity obtained by call to 
 * #XCAP_xmlParulesParseGetIdentityFromConditions, in conditions obtained by
 * call to parsed XML presence rules authorization
 * document, at a specified index, parsed by call to
 * #XCAP_xmlParulesParseDocument.
 * For example following call will get first one id from identity, in 
 * conditions, in a rule, in example document:
 * \verbatim
 * XCAP_xmlParulesParseGetOneFromIdentity(identity, 0, &id_ptr);
 * \endverbatim
 * The one id will look like:
 * \verbatim
 *                 <cr:one id="sip:user@example.com">
 * \endverbatim
 * And pointer\n
 * id_ptr will point to NULL terminated string "sip:user@example.com"\n
 *
 * @param identity Handle to identity returned by call to
 * #XCAP_xmlParulesParseGetIdentityFromConditions.
 * @param oneNumber Index of one id. User can iterate on index to get 
 * all one ids in the document, till function returns NULL.
 * @param id_ptr A pointer to a pointer, to be pointed to a NULL
 * terminated string containing one id.
 * @return A handle to a one id. This handle is
 * used later for getting elements and attributes from the one id.\n
 * If handle is NULL, no one id was found at the specified index.
 */
XCAP_Xml XCAP_xmlParulesParseGetOneFromIdentity(
    XCAP_Xml   identity,
    int        oneNumber,
    char     **id_ptr);

/** \fn XCAP_Xml XCAP_xmlParulesParseGetSubscriptionHandlingFromActions(
 *          XCAP_Xml   actions,
 *          char     **subs_ptr);
 * \brief Parse sub-handling from a prsence authorization rules document.
 *
 * This function gets sub-hadling element from actions obtained by call to
 * #XCAP_xmlParulesParseGetElementFromRule, in a rule obtained 
 * by call to #XCAP_xmlParulesParseGetRule, in a parsed XML
 * presence authorization rules document parsed by call to
 * #XCAP_xmlParulesParseDocument.
 * For example continuing from example of 
 * #XCAP_xmlParulesParseGetElementFromRule, following call will get 
 * sub-handling and its value from actions in first rule in example document:
 * \verbatim
 * XCAP_xmlParulesParseGetSubscriptionHandlingFromActions(actions, &subs_ptr);
 * \endverbatim
 * Corresponding sub-handling element obtained will look like:
 * \verbatim
 *             <pr:sub-handling>allow</pr:sub-handling>
 * \endverbatim
 * And pointer\n
 * subs_ptr will point to #XCAP_ALLOW, #XCAP_BLOCK, #XCAP_CONFIRM, or
 * #XCAP_POLITE_BLOCK \n
 *
 * @param actions Handle to actions returned by call to
 * #XCAP_xmlParulesParseGetElementFromRule.
 * @param subs_ptr A pointer to a pointer, to be pointed to #XCAP_CR_CONDITIONS,
 * #XCAP_CR_ACTIONS, or #XCAP_CR_TRANSFORMATIONS.
 * @return A handle to an element sub-handling. This handle is
 * used later for getting elements and attributes from the element.\n
 * If handle is NULL, no sub-handling element was found in specified actions.
 */
XCAP_Xml XCAP_xmlParulesParseGetSubscriptionHandlingFromActions(
    XCAP_Xml   actions,
    char     **subs_ptr);

#endif
