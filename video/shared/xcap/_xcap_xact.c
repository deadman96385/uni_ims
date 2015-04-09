/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 29904 $ $Date: 2014-11-19 02:54:52 +0800 (Wed, 19 Nov 2014) $
 *
 */

#include <osal.h>
#include <http.h>
#include "xcap_resources.h"
#include "_xcap_dbg.h"
#include "_xcap_xact.h"

/* 
 * ======== _XCAP_xactGetHttpResult() ========
 * 
 * This function setup xcap pointers to http incoming data buffer for the app (XCAP).
 * All XCAP/HTTP parsing is done here.
 *
 * Returns:
 *  0 failed, 1 success.
 *
 */
static int _XCAP_xactGetHttpResult(_XCAP_XactObj *xact_ptr)
{
    xact_ptr->reply.hdr_ptr = xact_ptr->http.lastHttpStatusLine;
    xact_ptr->reply.hdrBufSz = OSAL_strlen(xact_ptr->http.lastHttpStatusLine);
    xact_ptr->reply.body_ptr = xact_ptr->http.bufs.body_ptr;
    xact_ptr->reply.bodyBufSz = xact_ptr->http.bufs.bodyBufSz;
    xact_ptr->reply.failed = xact_ptr->http.bufs.failed;
    return 1;
}


/* 
 * ======== _XCAP_xactAuthHandler() ========
 * 
 * Based on the .
 * (RFC 4825 Sec 7).
 * This is a blocking function.
 *
 * Returns: 
 *
 */
OSAL_Boolean _XCAP_xactAuthHandler(
    HTTP_Obj *httpObj_ptr,
    char     *authHeader,
    int      size)
{
    _XCAP_XactObj *xact_ptr = (_XCAP_XactObj *)httpObj_ptr->arg_ptr;
    /* auth */
    _XCAP_DBG(__FILE__, __LINE__);
    /* xxx : check authHeader on DIGEST and REALM value */
    httpObj_ptr->username_ptr = xact_ptr->username_ptr;
    httpObj_ptr->password_ptr = xact_ptr->password_ptr;
    return OSAL_TRUE;
}

/* 
 * ======== _XCAP_xactCreateOrReplaceElement() ========
 * 
 * This function creates/replaces an XML element.
 * (RFC 4825 Sec 7).
 * This is a blocking function.
 *
 * Returns: 
 *
 */
int _XCAP_xactCreateOrReplaceElement(
    _XCAP_XactObj *xact_ptr)
{
    OSAL_Status  status;
    vint cnt = 0;

    if (NULL == xact_ptr) {
        return (0);
    }
    if ((NULL == xact_ptr->uri_ptr) || (NULL == xact_ptr->src_ptr)) {
        return (0);
    }

    /*
     * Fresh transaction.
     */
    HTTP_freshBuffer(&xact_ptr->http);

    /*
     * Extra headers.
     */
    xact_ptr->http.customHeaders[cnt++] = XCAP_CONTENT_TYPE_APPLICATION_XCAP_EL_XML;
       
    /*
     * If-(None)-Match header add.
     */
    if (((XCAP_IF_MATCH == xact_ptr->ifnm_ptr) || 
            (XCAP_IF_NONE_MATCH == xact_ptr->ifnm_ptr)) &&
            (NULL != xact_ptr->etag_ptr)) {
        OSAL_snprintf(xact_ptr->scratch,
                sizeof(xact_ptr->scratch) - 1,
                "%s\"%s\"",
                xact_ptr->ifnm_ptr,
                xact_ptr->etag_ptr);
        xact_ptr->http.customHeaders[cnt++]  = xact_ptr->scratch;
    }
    
    /*
     * Extra headers.
     */
    if (NULL != xact_ptr->x3gpp_ptr && 0 != *xact_ptr->x3gpp_ptr) {
        OSAL_snprintf(xact_ptr->scratch2, sizeof(xact_ptr->scratch2) - 1,
                "%s: %s", XCAP_X3GPP_INTENDED_ID, xact_ptr->x3gpp_ptr);
        xact_ptr->http.customHeaders[cnt++] = xact_ptr->scratch2;
    }

    xact_ptr->http.customHeadersCount = cnt;

    /*
     * PUT it.
     * HTTP
     *  setup
     *  put
     *  cleanup
     */

    status = HTTP_put(&xact_ptr->http,
        xact_ptr->src_ptr,
        xact_ptr->srcSz,
        xact_ptr->uri_ptr);
    if (OSAL_FAIL == status) {
        return (0);
    }

    _XCAP_xactGetHttpResult(xact_ptr);
    return (1);
}

/* 
 * ======== _XCAP_xactCreateOrReplaceAttribute() ========
 * 
 * This function creates/replaces an XML attribute.
 * (RFC 4825 Sec 7).
 * This is a blocking function.
 *
 * Returns: 
 *
 */
int _XCAP_xactCreateOrReplaceAttribute(
    _XCAP_XactObj *xact_ptr)
{
    OSAL_Status  status;
    vint cnt = 0;

    if (NULL == xact_ptr) {
        return (0);
    }
    if ((NULL == xact_ptr->uri_ptr) || (NULL == xact_ptr->src_ptr)) {
        return (0);
    }

    /*
     * Fresh transaction.
     */
    HTTP_freshBuffer(&xact_ptr->http);

    /*
     * Make HTTP request.
     */
    xact_ptr->http.customHeaders[cnt++] = XCAP_CONTENT_TYPE_APPLICATION_XCAP_ATT_XML;
       
    /*
     * If-(None)-Match header add.
     */
    if (((XCAP_IF_MATCH == xact_ptr->ifnm_ptr) || 
            (XCAP_IF_NONE_MATCH == xact_ptr->ifnm_ptr)) &&
            (NULL != xact_ptr->etag_ptr)) {
        OSAL_snprintf(xact_ptr->scratch,
                sizeof(xact_ptr->scratch) - 1,
                "%s\"%s\"",
                xact_ptr->ifnm_ptr,
                xact_ptr->etag_ptr);
        xact_ptr->http.customHeaders[cnt++] = xact_ptr->scratch;
    }
    
    /*
     * Extra headers.
     */
    if (NULL != xact_ptr->x3gpp_ptr && 0 != *xact_ptr->x3gpp_ptr) {
        OSAL_snprintf(xact_ptr->scratch2, sizeof(xact_ptr->scratch2) - 1,
                "%s: %s", XCAP_X3GPP_INTENDED_ID, xact_ptr->x3gpp_ptr);
        xact_ptr->http.customHeaders[cnt++] = xact_ptr->scratch2;
    }

    xact_ptr->http.customHeadersCount = cnt;

    /*
     * PUT it.
     * HTTP
     *  setup
     *  put
     *  cleanup
     */
    
    status = HTTP_put(&xact_ptr->http,
        xact_ptr->src_ptr,
        xact_ptr->srcSz,
        xact_ptr->uri_ptr);
    if (OSAL_FAIL == status) {
        return (0);
    }
    _XCAP_xactGetHttpResult(xact_ptr);
    return (1);
}

/* 
 * ======== _XCAP_xactCreateOrReplaceDocument() ========
 * 
 * This function creates/replaces an XML doc.
 * (RFC 4825 Sec 7).
 * This is a blocking function.
 *
 * Returns: 
 *
 */
int _XCAP_xactCreateOrReplaceDocument(
    _XCAP_XactObj *xact_ptr,
    char         *auid_ptr)
{
    OSAL_Status  status;
    vint cnt = 0;

    if ((NULL == xact_ptr) || (NULL == auid_ptr)) {
        return (0);
    }
    if ((NULL == xact_ptr->uri_ptr) || (NULL == xact_ptr->src_ptr)) {
        return (0);
    }

    /*
     * Fresh transaction.
     */
    HTTP_freshBuffer(&xact_ptr->http);

    /*
     * Make HTTP request.
     */
    if (auid_ptr == XCAP_RLS_SERVICES) {
        xact_ptr->http.customHeaders[cnt++] =
                XCAP_CONTENT_TYPE_APPLICATION_RLS_SERVICE_XML;
    }
    else if (auid_ptr == XCAP_PRES_RULES) {
        xact_ptr->http.customHeaders[cnt++] =
                XCAP_CONTENT_TYPE_APPLICATION_PRES_RULES_XML;
    }
    else if (auid_ptr == XCAP_RESOURCE_LISTS) {
        xact_ptr->http.customHeaders[cnt++] =
                XCAP_CONTENT_TYPE_APPLICATION_RESOURCE_LISTS_XML;
    }
    else if (auid_ptr == XCAP_SIMSERVS_AUID) {
        xact_ptr->http.customHeaders[cnt++] =
                XCAP_CONTENT_TYPE_APPLICATION_ETSI_SERVICE_XML;
   }
    else {
        return (0);
    }
   
    /*
     * If-(None)-Match header add.
     */
    if (((XCAP_IF_MATCH == xact_ptr->ifnm_ptr) || 
            (XCAP_IF_NONE_MATCH == xact_ptr->ifnm_ptr)) &&
            (NULL != xact_ptr->etag_ptr)) {
        OSAL_snprintf(xact_ptr->scratch,
                sizeof(xact_ptr->scratch) - 1,
                "%s\"%s\"",
                xact_ptr->ifnm_ptr,
                xact_ptr->etag_ptr);
        xact_ptr->http.customHeaders[cnt++] = xact_ptr->scratch;
    }
    
    /*
     * Extra headers.
     */
    if (NULL != xact_ptr->x3gpp_ptr && 0 != *xact_ptr->x3gpp_ptr) {
        OSAL_snprintf(xact_ptr->scratch2, sizeof(xact_ptr->scratch2) - 1,
                "%s: %s", XCAP_X3GPP_INTENDED_ID, xact_ptr->x3gpp_ptr);
        xact_ptr->http.customHeaders[cnt++] = xact_ptr->scratch2;
    }

    xact_ptr->http.customHeadersCount = cnt;

    /*
     * PUT it.
     * HTTP
     *  setup
     *  put
     *  cleanup
     */

    status = HTTP_put(&xact_ptr->http,
        xact_ptr->src_ptr,
        xact_ptr->srcSz,
        xact_ptr->uri_ptr);
    if (OSAL_FAIL == status) {
        return (0);
    }
    _XCAP_xactGetHttpResult(xact_ptr);
    return (1);
}

/* 
 * ======== _XCAP_xactDelete() ========
 * 
 * This function deletes an XML doc/element/attribute.
 * (RFC 4825 Sec 7).
 * This is a blocking function.
 *
 * Returns: 
 *
 */
int _XCAP_xactDelete(
    _XCAP_XactObj *xact_ptr)
{
    OSAL_Status  status;
    vint cnt = 0;

    if (NULL == xact_ptr) {
        return (0);
    }
    if (NULL == xact_ptr->uri_ptr) {
        return (0);
    }

    /*
     * Fresh transaction.
     */
    HTTP_freshBuffer(&xact_ptr->http);

    /*
     * Extra headers.
     */
    if (NULL != xact_ptr->x3gpp_ptr && 0 != *xact_ptr->x3gpp_ptr) {
        OSAL_snprintf(xact_ptr->scratch, sizeof(xact_ptr->scratch) - 1,
                "%s: %s", XCAP_X3GPP_INTENDED_ID, xact_ptr->x3gpp_ptr);
        xact_ptr->http.customHeaders[cnt++] = xact_ptr->scratch;
    }

    xact_ptr->http.customHeadersCount = cnt;

    /*
     * Delete it.
     * HTTP
     *  setup
     *  delete
     *  cleanup
     */
    
    status = HTTP_delete(&xact_ptr->http, xact_ptr->uri_ptr);
    if (OSAL_FAIL == status) {
        return (0);
    }
    _XCAP_xactGetHttpResult(xact_ptr);
    return (1);
}

/* 
 * ======== _XCAP_xactFetch() ========
 * 
 * This function fetches an XML doc/element/attribute.
 * (RFC 4825 Sec 7).
 * This is a blocking function. Data is received in the http callback function.
 *
 * Returns: 
 *
 */
int _XCAP_xactFetch(
    _XCAP_XactObj *xact_ptr)
{
    OSAL_Status  status;
    vint cnt = 0;

    if (NULL == xact_ptr) {
        return (0);
    }
    if (NULL == xact_ptr->uri_ptr) {
        return (0);
    }

    /*
     * Fresh transaction.
     */
    HTTP_freshBuffer(&xact_ptr->http);
    
    /*
     * Extra headers.
     */
    if (NULL != xact_ptr->x3gpp_ptr && 0 != *xact_ptr->x3gpp_ptr) {
        OSAL_snprintf(xact_ptr->scratch, sizeof(xact_ptr->scratch) - 1,
                "%s: %s", XCAP_X3GPP_INTENDED_ID, xact_ptr->x3gpp_ptr);
        xact_ptr->http.customHeaders[cnt++] = xact_ptr->scratch;
    }
    
    xact_ptr->http.customHeadersCount = cnt;
    
    /*
     * Fetch it.
     * HTTP
     *  setup
     *  get
     *  cleanup
     */

    status = HTTP_get(&xact_ptr->http, xact_ptr->uri_ptr);
    if (OSAL_FAIL == status) {
        return (0);
    }
    
    _XCAP_xactGetHttpResult(xact_ptr);
    return (1);
}

/*
 * ======== _XCAP_xactInit() ========
 * 
 * Init the transaction layer with transport aka http.
 * This is required for optimization of HTTP transport.
 * It should only be called when new HTTP auth and server certification
 * verification is required. Once connection is established with server, it
 * should stay on as long as required.
 * User must learn properly when to use this function.
 * Warning: Two successive inits on same object will cause memory leak.
 * Shutdown before init again on same object.
 *
 * Returns:
 *  0: failed
 *  1: success
 *
 */
int _XCAP_xactInit(
    _XCAP_XactObj *xact_ptr,
    int           timeoutsec)
{
    if (NULL == xact_ptr) {
        return (0);
    }

    OSAL_memSet(&xact_ptr->http,
            0,
            sizeof(HTTP_Obj));

    /*
     * Others.
     * XXX: Get all these from a config file.
     */
    xact_ptr->http.followLocation = OSAL_TRUE;
    xact_ptr->http.timeoutsec = timeoutsec < 0 ? 0 : timeoutsec;
    OSAL_strncpy(xact_ptr->http.certificate,
            XCAP_SSL_CERTIFICATE_LOCATION,
            sizeof(xact_ptr->http.certificate));

    xact_ptr->http.customHeadersCount = 0;
    xact_ptr->http.username_ptr = 0;
    xact_ptr->http.password_ptr = 0;
    xact_ptr->http.authHandler = _XCAP_xactAuthHandler;
    xact_ptr->http.authProtocol = HTTP_AUTH_ANY;
    
    /* every transaction will start from current supsrv radio infc */
    OSAL_netAddrCpy(&xact_ptr->http.infcAddress, xact_ptr->infcAddr_ptr);
    
    /*
     * This is where replies from server will be stored.
     */
    xact_ptr->http.arg_ptr = xact_ptr;
    if (OSAL_FAIL == HTTP_setup(&xact_ptr->http)) {
        return (0);
    }
    return (1);
}

/*
 * ======== _XCAP_xactShutdown() ========
 * 
 * Shuts the transaction layer previously inited by _XCAP_xactInit() call.
 * Warning: Two successive inits on same object will cause memory leak.
 * Shutdown before init again on same object.
 *
 * Returns: 
 *  0: failed
 *  1: success
 *
 */
int _XCAP_xactShutdown(
    _XCAP_XactObj *xact_ptr)
{
    if (NULL == xact_ptr) {
        return (0);
    }
    HTTP_cleanup(&xact_ptr->http);

    return (1);
}
