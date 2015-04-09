/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 26184 $ $Date: 2014-05-14 10:44:55 +0800 (Wed, 14 May 2014) $
 *
 */

/*
 * Define all strings including debug strings here.
 * - For auto optimizations by compiler
 * - For Unicode auto translation
 * - For reuse and reference
 */

char XCAP_CONTENT_TYPE_APPLICATION_XCAP_EL_XML[] = 
        "Content-Type:application/xcap-el+xml";
char XCAP_CONTENT_TYPE_APPLICATION_XCAP_ATT_XML[] = 
        "Content-Type:application/xcap-att+xml";
char XCAP_CONTENT_TYPE_APPLICATION_RLS_SERVICE_XML[] = 
        "Content-Type:application/rls-services+xml";
char XCAP_CONTENT_TYPE_APPLICATION_PRES_RULES_XML[] = 
        "Content-Type:application/pres-rules+xml";
char XCAP_CONTENT_TYPE_APPLICATION_RESOURCE_LISTS_XML[] = 
        "Content-Type:application/resource-lists+xml";
char XCAP_CONTENT_TYPE_APPLICATION_ETSI_SERVICE_XML[] =
        "Content-Type:application/simservs+xml";
char XCAP_RLS_SERVICES[] =
        "rls-services";
char XCAP_PRES_RULES[] =
        "pres-rules";
char XCAP_ETSI_SIMSERVS_AUID[] =
        "xcap/simservs.ngn.etsi.org";
char XCAP_SIMSERVS_AUID[] =
        "xcap/simservs.ngn.etsi.org";
char XCAP_SIMSERVS_DOC[] =
        "simservs.xml";
char XCAP_SIMSERVS_CD_DOC[] =
        "simservs.xml/~~/simservs/communication-diversion";
char XCAP_SIMSERVS_OIR_DOC[] =
        "simservs.xml/~~/simservs/originating-identity-presentation-restriction";
char XCAP_SIMSERVS_CBIC_DOC[] =
        "simservs.xml/~~/simservs/incoming-communication-barring";
char XCAP_SIMSERVS_CBOG_DOC[] =
        "simservs.xml/~~/simservs/outgoing-communication-barring";
char XCAP_SIMSERVS_CW_DOC[] =
        "simservs.xml/~~/simservs/communication-waiting";
char XCAP_SIMSERVS_TIR_DOC[] =
        "simservs.xml/~~/simservs/terminating-identity-presentation-restriction";
//char XCAP_RESOURCE_LISTS[] =
//      "resource-lists";
// This change is for YTL only.  This should be configurable
// in the future.
char XCAP_RESOURCE_LISTS[] =
      "network-address-book";
char XCAP_XML_HDR[] =
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>";
char XCAP_XMLNS[] =
        "xmlns";
char XCAP_XMLNS_PR[] =
        "xmlns:pr";
char XCAP_XMLNS_CR[] =
        "xmlns:cr";
char XCAP_XMLNS_XSI[] =
        "xmlns:xsi";
char XCAP_URN_IETF_PARAMS_XML_NS_PRES_RULES[] =
        "urn:ietf:params:xml:ns:pres-rules";
char XCAP_URN_IETF_PARAMS_XML_NS_RLS_SERVICES[] =
        "urn:ietf:params:xml:ns:rls-services";
char XCAP_URN_IETF_PARAMS_XML_NS_RESOURCE_LISTS[] =
        "urn:ietf:params:xml:ns:resource-lists";
char XCAP_URN_IETF_PARAMS_XML_NS_COMMON_POLICY[] =
        "urn:ietf:params:xml:ns:common-policy";
char XCAP_HTTP_WWW_W3_ORG_2001_XMLSCHEMA_INSTANCE[] = 
        "http://www.w3.org/2001/XMLSchema-instance";
char XCAP_CR_RULESET[] =
        "cr:ruleset";
char XCAP_CR_RULE[] =
        "cr:rule";
char XCAP_SERVICE[] =
        "service";
char XCAP_PACKAGES[] =
        "packages";
char XCAP_PACKAGE[] =
        "package";
char XCAP_RESOURCE_LIST[] =
        "resource-list";
char XCAP_ID[] =
        "id";
char XCAP_LIST[] =
        "list";
char XCAP_NAME[] =
        "name";
char XCAP_URI[] =
        "uri";
char XCAP_ENTRY[] =
        "entry";
char XCAP_DISPLAY_NAME[] =
        "display-name";
char XCAP_CR_CONDITIONS[] =
        "cr:conditions";
char XCAP_CR_ACTIONS[] =
        "cr:actions";
char XCAP_CR_TRANSFORMATIONS[] =
        "cr:transformations";
char XCAP_CR_IDENTITY[] =
        "cr:identity";
char XCAP_CR_ONE[] =
        "cr:one";
char XCAP_PR_SUB_HANDLING[] =
        "pr:sub-handling";
char XCAP_IF_MATCH[] =
        "If-Match: ";
char XCAP_IF_NONE_MATCH[] =
        "If-None-Match: ";
char XCAP_USERS[] =
        "users";
char XCAP_GLOBAL[] =
        "global";
char XCAP_INDEX_XML[] =
        "index";
char XCAP_ALLOW[] =
        "allow";
char XCAP_BLOCK[] =
        "block";
char XCAP_POLITE_BLOCK[] =
        "polite-block";
char XCAP_CONFIRM[] =
        "confirm";
#ifdef VPORT_4G_PLUS_APROC
char XCAP_CMD_Q_NAME[] =
        "aproc-xcap-cmdq";
char XCAP_EVT_Q_NAME[] =
        "aproc-xcap-evtq";
char XCAP_TASK_NAME[] =
        "aproc-xcapt";
#else
char XCAP_CMD_Q_NAME[] =
        "xcap-cmdq";
char XCAP_EVT_Q_NAME[] =
        "xcap-evtq";
char XCAP_TASK_NAME[] =
        "xcapt";
#endif
char XCAP_SSL_CERTIFICATE_LOCATION[] =
        "/etc/ssl/certs/ca-certificates.crt";
char XCAP_HTTPS[] = 
        "https://";
char XCAP_HTTP[] = 
        "http://";
char XCAP_X3GPP_INTENDED_ID[] =
        "X-3GPP-Intended-Identity";
