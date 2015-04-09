/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev$ $Date$
 *
 */

#include <osal.h>
#include <http.h>
#include "xcap_resources.h"
#include "_xcap_dbg.h"
#include "gba.h"
#include "gaa.h"
#include "_xcap_xact_gaa.h"

/*
 * ======== _XCAP_xactGaaGetHttpResult() ========
 *
 * This function setup xcap pointers to http incoming data buffer for the app (XCAP).
 * All XCAP/HTTP parsing is done here.
 *
 * Returns:
 *  0 failed, 1 success.
 *
 */
static int _XCAP_xactGaaGetHttpResult(_XCAP_XactGaaObj *xactGaa_ptr) {
    GAA_NafSession *nafSession_ptr;
    HTTP_Obj *httpObj_ptr;

    if (NULL == xactGaa_ptr) {
        return (0);
    }
    nafSession_ptr = xactGaa_ptr->nafSession_ptr;
    httpObj_ptr = &(nafSession_ptr->httpObj);

    xactGaa_ptr->reply.hdr_ptr = httpObj_ptr->lastHttpStatusLine;
    xactGaa_ptr->reply.hdrBufSz = OSAL_strlen(
            httpObj_ptr->lastHttpStatusLine);
    xactGaa_ptr->reply.body_ptr = httpObj_ptr->bufs.body_ptr;
    xactGaa_ptr->reply.bodyBufSz = httpObj_ptr->bufs.bodyBufSz;
    xactGaa_ptr->reply.failed = httpObj_ptr->bufs.failed;
    return 1;
}

/*
 * ======== _XCAP_xactGaaCreateOrReplaceElement() ========
 *
 * This function creates/replaces an XML element.
 * (RFC 4825 Sec 7).
 * This is a blocking function.
 *
 * Returns:
 *
 */
int _XCAP_xactGaaCreateOrReplaceElement(_XCAP_XactGaaObj *xactGaa_ptr) {
    OSAL_Status status;
    vint cnt = 0;
    GAA_NafSession *nafSession_ptr;
    HTTP_Obj *httpObj_ptr;

    if (NULL == xactGaa_ptr) {
        return (0);
    }
    nafSession_ptr = xactGaa_ptr->nafSession_ptr;
    httpObj_ptr = &(nafSession_ptr->httpObj);

    if ((NULL == xactGaa_ptr->uri_ptr) || (NULL == xactGaa_ptr->src_ptr)) {
        return (0);
    }

    /*
     * Fresh transaction.
     */
    HTTP_freshBuffer(httpObj_ptr);

    /*
     * Extra headers.
     */
    cnt = httpObj_ptr->customHeadersCount;
    httpObj_ptr->customHeaders[cnt++] =
            XCAP_CONTENT_TYPE_APPLICATION_XCAP_EL_XML;

    /*
     * If-(None)-Match header add.
     */
    if (((XCAP_IF_MATCH == xactGaa_ptr->ifnm_ptr)
            || (XCAP_IF_NONE_MATCH == xactGaa_ptr->ifnm_ptr))
            && (NULL != xactGaa_ptr->etag_ptr)) {
        OSAL_snprintf(xactGaa_ptr->scratch, sizeof(xactGaa_ptr->scratch) - 1,
                "%s\"%s\"", xactGaa_ptr->ifnm_ptr, xactGaa_ptr->etag_ptr);
        httpObj_ptr->customHeaders[cnt++] = xactGaa_ptr->scratch;
    }

    /*
     * Extra headers.
     */
    if (NULL != xactGaa_ptr->x3gpp_ptr && 0 != *xactGaa_ptr->x3gpp_ptr) {
        OSAL_snprintf(xactGaa_ptr->scratch2, sizeof(xactGaa_ptr->scratch2) - 1,
                "%s: %s", XCAP_X3GPP_INTENDED_ID, xactGaa_ptr->x3gpp_ptr);
        httpObj_ptr->customHeaders[cnt++] = xactGaa_ptr->scratch2;
    }

    httpObj_ptr->customHeadersCount = cnt;

    /*
     * PUT it.
     * HTTP
     *  setup
     *  put
     *  cleanup
     */

    status = HTTP_put(httpObj_ptr, xactGaa_ptr->src_ptr,
            xactGaa_ptr->srcSz, xactGaa_ptr->uri_ptr);
    if (OSAL_FAIL == status) {
        return (0);
    }

    _XCAP_xactGaaGetHttpResult(xactGaa_ptr);
    return (1);
}

/*
 * ======== _XCAP_xactGaaCreateOrReplaceAttribute() ========
 *
 * This function creates/replaces an XML attribute.
 * (RFC 4825 Sec 7).
 * This is a blocking function.
 *
 * Returns:
 *
 */
int _XCAP_xactGaaCreateOrReplaceAttribute(_XCAP_XactGaaObj *xactGaa_ptr) {
    OSAL_Status status;
    vint cnt = 0;
    GAA_NafSession *nafSession_ptr;
    HTTP_Obj *httpObj_ptr;

    if (NULL == xactGaa_ptr) {
        return (0);
    }
    nafSession_ptr = xactGaa_ptr->nafSession_ptr;
    httpObj_ptr = &(nafSession_ptr->httpObj);

    if ((NULL == xactGaa_ptr->uri_ptr) || (NULL == xactGaa_ptr->src_ptr)) {
        return (0);
    }

    /*
     * Fresh transaction.
     */
    HTTP_freshBuffer(httpObj_ptr);

    /*
     * Make HTTP request.
     */
    httpObj_ptr->customHeaders[cnt++] =
            XCAP_CONTENT_TYPE_APPLICATION_XCAP_ATT_XML;

    /*
     * If-(None)-Match header add.
     */
    if (((XCAP_IF_MATCH == xactGaa_ptr->ifnm_ptr)
            || (XCAP_IF_NONE_MATCH == xactGaa_ptr->ifnm_ptr))
            && (NULL != xactGaa_ptr->etag_ptr)) {
        OSAL_snprintf(xactGaa_ptr->scratch, sizeof(xactGaa_ptr->scratch) - 1,
                "%s\"%s\"", xactGaa_ptr->ifnm_ptr, xactGaa_ptr->etag_ptr);
        httpObj_ptr->customHeaders[cnt++] = xactGaa_ptr->scratch;
    }

    /*
     * Extra headers.
     */
    if (NULL != xactGaa_ptr->x3gpp_ptr && 0 != *xactGaa_ptr->x3gpp_ptr) {
        OSAL_snprintf(xactGaa_ptr->scratch2, sizeof(xactGaa_ptr->scratch2) - 1,
                "%s: %s", XCAP_X3GPP_INTENDED_ID, xactGaa_ptr->x3gpp_ptr);
        httpObj_ptr->customHeaders[cnt++] = xactGaa_ptr->scratch2;
    }

    httpObj_ptr->customHeadersCount = cnt;

    /*
     * PUT it.
     * HTTP
     *  setup
     *  put
     *  cleanup
     */

    status = HTTP_put(httpObj_ptr, xactGaa_ptr->src_ptr,
            xactGaa_ptr->srcSz, xactGaa_ptr->uri_ptr);
    if (OSAL_FAIL == status) {
        return (0);
    }
    _XCAP_xactGaaGetHttpResult(xactGaa_ptr);
    return (1);
}

/*
 * ======== _XCAP_xactGaaCreateOrReplaceDocument() ========
 *
 * This function creates/replaces an XML doc.
 * (RFC 4825 Sec 7).
 * This is a blocking function.
 *
 * Returns:
 *
 */
int _XCAP_xactGaaCreateOrReplaceDocument(_XCAP_XactGaaObj *xactGaa_ptr,
        char *auid_ptr) {
    OSAL_Status status;
    vint cnt = 0;
    GAA_NafSession *nafSession_ptr;
    HTTP_Obj *httpObj_ptr;

    if (NULL == xactGaa_ptr) {
        return (0);
    }
    nafSession_ptr = xactGaa_ptr->nafSession_ptr;
    httpObj_ptr = &(nafSession_ptr->httpObj);

    if ((NULL == xactGaa_ptr->uri_ptr) || (NULL == xactGaa_ptr->src_ptr)) {
        return (0);
    }

    /*
     * Fresh transaction.
     */
    HTTP_freshBuffer(httpObj_ptr);

    /*
     * Make HTTP request.
     */
    if (auid_ptr == XCAP_RLS_SERVICES) {
        httpObj_ptr->customHeaders[cnt++] =
                XCAP_CONTENT_TYPE_APPLICATION_RLS_SERVICE_XML;
    } else if (auid_ptr == XCAP_PRES_RULES) {
        httpObj_ptr->customHeaders[cnt++] =
                XCAP_CONTENT_TYPE_APPLICATION_PRES_RULES_XML;
    } else if (auid_ptr == XCAP_RESOURCE_LISTS) {
        httpObj_ptr->customHeaders[cnt++] =
                XCAP_CONTENT_TYPE_APPLICATION_RESOURCE_LISTS_XML;
    } else if (auid_ptr == XCAP_SIMSERVS_AUID) {
        httpObj_ptr->customHeaders[cnt++] =
                XCAP_CONTENT_TYPE_APPLICATION_ETSI_SERVICE_XML;
    } else {
        return (0);
    }

    /*
     * If-(None)-Match header add.
     */
    if (((XCAP_IF_MATCH == xactGaa_ptr->ifnm_ptr)
            || (XCAP_IF_NONE_MATCH == xactGaa_ptr->ifnm_ptr))
            && (NULL != xactGaa_ptr->etag_ptr)) {
        OSAL_snprintf(xactGaa_ptr->scratch, sizeof(xactGaa_ptr->scratch) - 1,
                "%s\"%s\"", xactGaa_ptr->ifnm_ptr, xactGaa_ptr->etag_ptr);
        httpObj_ptr->customHeaders[cnt++] = xactGaa_ptr->scratch;
    }

    /*
     * Extra headers.
     */
    if (NULL != xactGaa_ptr->x3gpp_ptr && 0 != *xactGaa_ptr->x3gpp_ptr) {
        OSAL_snprintf(xactGaa_ptr->scratch2, sizeof(xactGaa_ptr->scratch2) - 1,
                "%s: %s", XCAP_X3GPP_INTENDED_ID, xactGaa_ptr->x3gpp_ptr);
        httpObj_ptr->customHeaders[cnt++] = xactGaa_ptr->scratch2;
    }

    httpObj_ptr->customHeadersCount = cnt;

    /*
     * PUT it.
     * HTTP
     *  setup
     *  put
     *  cleanup
     */

    status = HTTP_put(httpObj_ptr, xactGaa_ptr->src_ptr,
            xactGaa_ptr->srcSz, xactGaa_ptr->uri_ptr);
    if (OSAL_FAIL == status) {
        return (0);
    }
    _XCAP_xactGaaGetHttpResult(xactGaa_ptr);
    return (1);
}

/*
 * ======== _XCAP_xactGaaDelete() ========
 *
 * This function deletes an XML doc/element/attribute.
 * (RFC 4825 Sec 7).
 * This is a blocking function.
 *
 * Returns:
 *
 */
int _XCAP_xactGaaDelete(_XCAP_XactGaaObj *xactGaa_ptr) {
    OSAL_Status status;
    vint cnt = 0;
    GAA_NafSession *nafSession_ptr;
    HTTP_Obj *httpObj_ptr;

    if (NULL == xactGaa_ptr) {
        return (0);
    }
    nafSession_ptr = xactGaa_ptr->nafSession_ptr;
    httpObj_ptr = &(nafSession_ptr->httpObj);

    if (NULL == xactGaa_ptr->uri_ptr) {
        return (0);
    }

    /*
     * Fresh transaction.
     */
    HTTP_freshBuffer(httpObj_ptr);

    /*
     * Extra headers.
     */
    if (NULL != xactGaa_ptr->x3gpp_ptr && 0 != *xactGaa_ptr->x3gpp_ptr) {
        OSAL_snprintf(xactGaa_ptr->scratch, sizeof(xactGaa_ptr->scratch) - 1,
                "%s: %s", XCAP_X3GPP_INTENDED_ID, xactGaa_ptr->x3gpp_ptr);
        httpObj_ptr->customHeaders[cnt++] = xactGaa_ptr->scratch;
    }

    httpObj_ptr->customHeadersCount = cnt;

    /*
     * Delete it.
     * HTTP
     *  setup
     *  delete
     *  cleanup
     */

    status = HTTP_delete(httpObj_ptr, xactGaa_ptr->uri_ptr);
    if (OSAL_FAIL == status) {
        return (0);
    }
    _XCAP_xactGaaGetHttpResult(xactGaa_ptr);
    return (1);
}

/*
 * ======== _XCAP_xactGaaFetch() ========
 *
 * This function fetches an XML doc/element/attribute.
 * (RFC 4825 Sec 7).
 * This is a blocking function. Data is received in the http callback function.
 *
 * Returns:
 *
 */
int _XCAP_xactGaaFetch(_XCAP_XactGaaObj *xactGaa_ptr) {
    OSAL_Status status;
    vint cnt = 0;
    GAA_NafSession *nafSession_ptr;
    HTTP_Obj *httpObj_ptr;

    if (NULL == xactGaa_ptr) {
        return (0);
    }
    nafSession_ptr = xactGaa_ptr->nafSession_ptr;
    httpObj_ptr = &(nafSession_ptr->httpObj);

    if (NULL == xactGaa_ptr->uri_ptr) {
        return (0);
    }

    /*
     * Fresh transaction.
     */
    HTTP_freshBuffer(httpObj_ptr);

    /*
     * Extra headers.
     */
    if (NULL != xactGaa_ptr->x3gpp_ptr && 0 != *xactGaa_ptr->x3gpp_ptr) {
        OSAL_snprintf(xactGaa_ptr->scratch, sizeof(xactGaa_ptr->scratch) - 1,
                "%s: %s", XCAP_X3GPP_INTENDED_ID, xactGaa_ptr->x3gpp_ptr);
        httpObj_ptr->customHeaders[cnt++] = xactGaa_ptr->scratch;
    }

    httpObj_ptr->customHeadersCount = cnt;

    /*
     * Fetch it.
     * HTTP
     *  setup
     *  get
     *  cleanup
     */

    status = HTTP_get(httpObj_ptr, xactGaa_ptr->uri_ptr);
    if (OSAL_FAIL == status) {
        return (0);
    }

    _XCAP_xactGaaGetHttpResult(xactGaa_ptr);
    return (1);
}

/*
 * ======== _XCAP_xactGaaInit() ========
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
int _XCAP_xactGaaInit(_XCAP_XactGaaObj *xactGaa_ptr, int timeoutsec) {
    GAA_NafSession *nafSession_ptr;

    if (NULL == xactGaa_ptr) {
        return (0);
    }
    nafSession_ptr = xactGaa_ptr->nafSession_ptr;

    /* every transaction will start from current supsrv radio infc */
    nafSession_ptr->infcAddr_ptr = xactGaa_ptr->infcAddr_ptr;

    if (OSAL_FALSE == xactGaa_ptr->isSessionStarted) {
        GAA_startNafSession(nafSession_ptr);
        xactGaa_ptr->isSessionStarted = OSAL_TRUE;
    }
    return (1);
}

/*
 * ======== _XCAP_xactGaaShutdown() ========
 *
 * Shuts the transaction layer previously inited by _XCAP_xactGaaInit() call.
 * Warning: Two successive inits on same object will cause memory leak.
 * Shutdown before init again on same object.
 *
 * Returns:
 *  0: failed
 *  1: success
 *
 */
int _XCAP_xactGaaShutdown(_XCAP_XactGaaObj *xactGaa_ptr) {
    GAA_NafSession *nafSession_ptr;

    if (NULL == xactGaa_ptr) {
        return (0);
    }
    nafSession_ptr = xactGaa_ptr->nafSession_ptr;

    if (NULL == xactGaa_ptr) {
        return (0);
    }

    if (OSAL_FALSE == xactGaa_ptr->isSessionStarted) {
        GAA_stopNafSession(nafSession_ptr);
        xactGaa_ptr->isSessionStarted = OSAL_TRUE;
    }

    return (1);
}
