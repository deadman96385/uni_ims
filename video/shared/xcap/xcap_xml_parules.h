/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 12486 $ $Date: 2010-07-08 06:10:49 +0800 (Thu, 08 Jul 2010) $
 *
 */
/** \file
 * \brief Presence authorization rules document preparation and manipulation.
 *  
 * This is an (not so complete) implementation of RFC 5025.
 * Application uses this API to make XML documents from scratch, or modify
 * existing XML documents. Once a document is finalized, it can be converted to
 * XML text using call to #XCAP_xmlHelperMakeDocument.
 * Remains to be implemented:\n
 * - actions not fully supported: identity and sphere\n
 * - transformation not fully supported: no attributes implemented\n
 * - conditions not fully supported: many except id type not supported\n
 */

#ifndef _XCAP_XML_PARULES_H_
#define _XCAP_XML_PARULES_H_

/*
 * Function prototypes.
 */

/** \fn XCAP_Xml XCAP_xmlParulesCreateDocument(
 *          void)
 * \brief Create a presence authorization rules document.
 *
 * This function is the starting point for creating a presence authorization 
 * rules document in memory.\n
 * XML document after this call succeeds looks like:\n
 * \verbatim
 * <cr:ruleset xmlns="urn:ietf:params:xml:ns:pres-rules"
 *     xmlns:pr="urn:ietf:params:xml:ns:pres-rules"
 *     xmlns:cr="urn:ietf:params:xml:ns:common-policy">
 * </cr:ruleset>
 * \endverbatim
 *
 * @return A handle to a presence authorization rules document. This handle is
 * used later for adding elements and attributes to the document.\n
 * At the return of the function, the rules document is empty and contains no 
 * rule. This handle will be freed using #XCAP_xmlHelperDeleteHandle when the
 * document is not needed. If handle is NULL, document creation failed.
 */
XCAP_Xml XCAP_xmlParulesCreateDocument(
    void);

/** \fn XCAP_Xml XCAP_xmlParulesAddRuleToDocument(
 *          XCAP_Xml  doc,
 *          char     *id_ptr)
 * \brief Create a rule in presence authorization rules document.
 *
 * This function adds a rule in a presence autorization rules document with a
 * given id.
 * For example continuing from example of #XCAP_xmlParulesCreateDocument, 
 * XML document after this call succeeds looks like:\n
 * \verbatim
 * <cr:ruleset xmlns="urn:ietf:params:xml:ns:pres-rules"
 *     xmlns:pr="urn:ietf:params:xml:ns:pres-rules"
 *     xmlns:cr="urn:ietf:params:xml:ns:common-policy">
 *     <cr:rule id="someid">
 *     </cr:rule>
 * </cr:ruleset>
 * \endverbatim
 *
 * @param doc Handle of XML doument returned by call to 
 * #XCAP_xmlParulesCreateDocument.
 * @param id_ptr A NULL terminated string containing id of the rule to be
 * created. In the above example, id_ptr is set as id_ptr = "someid".
 * @return A Handle to the rule created, or NULL pointer indicating an error.
 */
XCAP_Xml XCAP_xmlParulesAddRuleToDocument(
    XCAP_Xml  doc,
    char     *id_ptr);

/** \fn XCAP_Xml XCAP_xmlParulesAddElementToRule(
 *          XCAP_Xml  rule,
 *          char     *name_ptr)
 * \brief Creates one of conditions, actions, or transformations element in
 * a presence autorization rule.
 *
 * This function adds elements namely conditions, actions, or transformations
 * in a presence autorization rule previosuly created by call to 
 * #XCAP_xmlParulesAddRuleToDocument.\n
 * For example continuing from example of #XCAP_xmlParulesAddRuleToDocument 
 * (the order of calls must be followed for correct document generation),
 * follwoing calls - if successful add conditions, actions, and transforamtions
 * to the rule:\n
 * \verbatim
 * XCAP_xmlParulesAddElementToRule(rule, XCAP_CR_CONDITIONS);
 * XCAP_xmlParulesAddElementToRule(rule, XCAP_CR_ACTIONS);
 * XCAP_xmlParulesAddElementToRule(rule, XCAP_CR_TRANSFORMATIONS);
 * \endverbatim
 * XML document after above calls succeed looks like:\n
 * \verbatim
 * <cr:ruleset xmlns="urn:ietf:params:xml:ns:pres-rules"
 *     xmlns:pr="urn:ietf:params:xml:ns:pres-rules"
 *     xmlns:cr="urn:ietf:params:xml:ns:common-policy">
 *     <cr:rule id="someid">
 *         <cr:conditions>
 *         </cr:conditions>
 *         <cr:actions>
 *         </cr:actions>
 *         <cr:transformations>
 *         </cr:transformations>
 *     </cr:rule>
 * </cr:ruleset>
 * \endverbatim
 *
 * @param rule Handle returned by call to 
 * #XCAP_xmlParulesAddRuleToDocument.
 * @param name_ptr Must be set to either #XCAP_CR_CONDITIONS, #XCAP_CR_ACTIONS,
 * or #XCAP_CR_TRANSFORMATIONS.
 * @return A Handle to the element created, or NULL pointer indicating an error.
 */
XCAP_Xml XCAP_xmlParulesAddElementToRule(
    XCAP_Xml  rule,
    char     *name_ptr);

/** \fn XCAP_Xml XCAP_xmlParulesAddIdentityToConditions(
 *          XCAP_Xml conditions)
 * \brief Adds an identity in conditions under a rule.
 *
 * This function adds identity to conditions under a rule. Identity is explained
 * in RFC 4745.
 * For example continuing from example of #XCAP_xmlParulesAddElementToRule,
 * following call, if successful, will add an identity:
 * \verbatim
 * XCAP_xmlParulesAddIdentityToConditions(conds);
 * \endverbatim
 * XML document after above call succeeds looks like:\n
 * \verbatim
 * <cr:ruleset xmlns="urn:ietf:params:xml:ns:pres-rules"
 *     xmlns:pr="urn:ietf:params:xml:ns:pres-rules"
 *     xmlns:cr="urn:ietf:params:xml:ns:common-policy">
 *     <cr:rule id="someid">
 *         <cr:conditions>
 *             <cr:identity>
 *             </cr:identity>
 *         </cr:conditions>
 *         <cr:actions>
 *         </cr:actions>
 *         <cr:transformations>
 *         </cr:transformations>
 *     </cr:rule>
 * </cr:ruleset>
 * \endverbatim
 *
 * @param conditions Handle returned by call to 
 * #XCAP_xmlParulesAddElementToRule with id_ptr = #XCAP_CR_CONDITIONS.
 * @return A Handle to the identity created, or NULL pointer indicating an
 * error.
 */
XCAP_Xml XCAP_xmlParulesAddIdentityToConditions(
    XCAP_Xml conditions);

/** \fn XCAP_Xml XCAP_xmlParulesAddOneToIdentity(
 *          XCAP_Xml  identity,
 *          char     *id_ptr)
 * \brief Adds a one id to identity in conditions under a rule.
 *
 * This function adds a one id to identity to conditions under a rule.
 * Identity is explained in RFC 4745.
 * For example continuing from example of
 * #XCAP_xmlParulesAddIdentityToConditions, following call, if successful,
 * will add a one id to identity:
 * \verbatim
 * XCAP_xmlParulesAddOneToIdentity(identity, "sip:user@example.com");
 * \endverbatim
 * XML document after above call succeeds looks like:\n
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
 *         </cr:actions>
 *         <cr:transformations>
 *         </cr:transformations>
 *     </cr:rule>
 * </cr:ruleset>
 * \endverbatim
 *
 * @param identity Handle returned by call to 
 * #XCAP_xmlParulesAddIdentityToConditions.
 * @param id_ptr Set to required id as in RFC 5025.
 * @return handle to the one id created, or NULL pointer indicating an
 * error.
 */
XCAP_Xml XCAP_xmlParulesAddOneToIdentity(
    XCAP_Xml identity,
    char    *id_ptr);

/** \fn XCAP_Xml XCAP_xmlParulesAddSubscriptionHandlingToActions(
 *          XCAP_Xml  actions,
 *          char     *subs_ptr)
 * \brief Adds a sub-handling in actions under a rule.
 *
 * This function adds a sub-handling and its value in actions under a rule.
 * Only selective values documented in RFC 5025 are allowed.
 * For example continuing from example of
 * #XCAP_xmlParulesAddOneToIdentity, following call, if successful,
 * will add a sub-handling to actions with value allow:
 * \verbatim
 * XCAP_xmlParulesAddSubscriptionHandlingToActions(actions, XCAP_ALLOW);
 * \endverbatim
 * XML document after above call succeeds looks like:\n
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
 *
 * @param actions Handle returned by call to 
 * #XCAP_xmlParulesAddElementToRule with id_ptr = #XCAP_CR_ACTIONS.
 * @param subs_ptr Set to #XCAP_ALLOW, #XCAP_BLOCK, #XCAP_POLITE_BLOCK,
 * or #XCAP_CONFIRM
 * @return A Handle to the sub-handling element created,
 * or NULL pointer indicating an error.
 */
XCAP_Xml XCAP_xmlParulesAddSubscriptionHandlingToActions(
    XCAP_Xml  actions,
    char     *subs_ptr);

#endif
