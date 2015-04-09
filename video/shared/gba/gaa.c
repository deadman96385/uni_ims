/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev$ $Date$
 *
 */

#include <osal.h>
#include "http.h"
#include "gba.h"
#include "gaa.h"

#ifndef GAA_DEBUG
#define GAA_dbgPrintf(x, ...)
#else
#define GAA_dbgPrintf OSAL_logMsg
#endif

#define GAA_BOOTSTRAPPING_PREFIX "3GPP-bootstrapping"
#define GAA_REALM_HF_ARG_STR     "realm"

/*
 * ======== _GAA_AuthDetectionHandler() ========
 *
 * This function is a http callback for auth response to do detection
 * To implement 24.109 section 5.2.4 spec
 *
 * Returns:
 *   flag for updating update auth user/pass or not.
 */
static OSAL_Boolean _GAA_AuthDetectionHandler(
        HTTP_Obj *httpObj_ptr,
        char     *authHeader,
        int      size)
{
    char         *realm_ptr;
    vint          realmLen;
    GAA_NafSession *nafSession_ptr;

    /* get auth type */
    nafSession_ptr = (GAA_NafSession *)httpObj_ptr->arg_ptr;
    HTTP_getParameterValue(authHeader, GAA_REALM_HF_ARG_STR, &realm_ptr, &realmLen);
    if (0 == OSAL_strncmp(realm_ptr,
            GAA_BOOTSTRAPPING_PREFIX,
            OSAL_strlen(GAA_BOOTSTRAPPING_PREFIX))
            ) {
        /* GBA bootstrapping required */
        GAA_dbgPrintf("GAA detected 3gpp gba prefix\n");
        nafSession_ptr->autoDetectedAuthType = GBA_NET_APP_AUTHTYPE_GAA_DIGEST;
    }
    else {
        /* default auth type without the GBA hint */
        nafSession_ptr->autoDetectedAuthType = GBA_NET_APP_AUTHTYPE_DIGEST;
        /* for testing with ericsson server, comment out following when not needed */
        GAA_dbgPrintf("GAA use default ericsson GAA DIGEST\n");
        nafSession_ptr->autoDetectedAuthType = GBA_NET_APP_AUTHTYPE_GAA_DIGEST_UNKNOWN;
    }

    return OSAL_FALSE;
}


/*
 * ======== _GAA_initHttp() ========
 *
 * Prepare a http context to initiate the bsf bootstraping
 *
 * Returns:
 *  void
 *
 */
void _GAA_initHttp(
    GAA_NafSession  *nafSession_ptr)
{
    HTTP_Obj        *httpObj_ptr = &nafSession_ptr->httpObj;
    GBA_NetAppObj   *netAppObj_ptr = nafSession_ptr->netAppObj_ptr;
    GBA_NafContext  *nafContext_ptr = nafSession_ptr->nafContext_ptr;

    httpObj_ptr->followLocation = OSAL_TRUE;
    httpObj_ptr->timeoutsec = 0;
    OSAL_strncpy(httpObj_ptr->certificate,
            "/etc/ssl/certs/ca-certificates.crt",
            sizeof(httpObj_ptr->certificate));

    /*
     * Extra headers.
     */
    httpObj_ptr->customHeadersCount = 2;
    httpObj_ptr->customHeaders[0] = "User-Agent: libcurl/7.18 d2tech 3gpp-gba ";
    httpObj_ptr->customHeaders[1] = 
            "X-EricssonLabs-APIKEY: OCf5heNqCFqGg6kfRbPlTh5MRbymRcmA9qW3D4gO";

    /* auth */
    switch (netAppObj_ptr->appAuthType) {
        case GBA_NET_APP_AUTHTYPE_GAA_DIGEST:
            httpObj_ptr->username_ptr = nafContext_ptr->btid;
            httpObj_ptr->password_ptr = nafContext_ptr->ksNaf;
            httpObj_ptr->authHandler = NULL;
            httpObj_ptr->authProtocol = HTTP_AUTH_DIGEST;
            break;
        case GBA_NET_APP_AUTHTYPE_GAA_DIGEST_UNKNOWN:
            httpObj_ptr->username_ptr = nafContext_ptr->btid;
            httpObj_ptr->password_ptr = nafContext_ptr->ksNaf;
            httpObj_ptr->authHandler = NULL;
            httpObj_ptr->authProtocol = HTTP_AUTH_DIGEST;
            break;
        case GBA_NET_APP_AUTHTYPE_AUTO:
            httpObj_ptr->username_ptr = NULL;
            httpObj_ptr->password_ptr = NULL;
            httpObj_ptr->authHandler = _GAA_AuthDetectionHandler;
            httpObj_ptr->authProtocol = HTTP_AUTH_ANY;
            break;
        case GBA_NET_APP_AUTHTYPE_DIGEST:
            httpObj_ptr->username_ptr = netAppObj_ptr->appAuthName;
            httpObj_ptr->password_ptr = netAppObj_ptr->appAuthSecret;
            httpObj_ptr->authHandler = NULL;
            httpObj_ptr->authProtocol = HTTP_AUTH_DIGEST;
            break;
    }

    httpObj_ptr->bufs.body_ptr = OSAL_memAlloc(4*1024, OSAL_MEM_ARG_DYNAMIC_ALLOC);
    httpObj_ptr->bufs.bodyBufSz = 4*1024;
    httpObj_ptr->bufs.bodyBufIndex = 0;

    OSAL_netAddrCpy(&httpObj_ptr->infcAddress, nafSession_ptr->infcAddr_ptr);

    httpObj_ptr->arg_ptr = nafSession_ptr;
    if (OSAL_FAIL == HTTP_setup(httpObj_ptr)) {
        return;
    }
}

void _GAA_updateHttpAuth(
    GAA_NafSession  *nafSession_ptr)
{
    HTTP_Obj        *httpObj_ptr = &nafSession_ptr->httpObj;
    GBA_NetAppObj   *netAppObj_ptr = nafSession_ptr->netAppObj_ptr;
    GBA_NafContext  *nafContext_ptr = nafSession_ptr->nafContext_ptr;

    /* auth */
    switch (netAppObj_ptr->appAuthType) {
        case GBA_NET_APP_AUTHTYPE_GAA_DIGEST:
            httpObj_ptr->username_ptr = nafContext_ptr->btid;
            httpObj_ptr->password_ptr = nafContext_ptr->ksNaf;
            httpObj_ptr->authHandler = NULL;
            httpObj_ptr->authProtocol = HTTP_AUTH_DIGEST;
            break;
        case GBA_NET_APP_AUTHTYPE_GAA_DIGEST_UNKNOWN:
            httpObj_ptr->username_ptr = nafContext_ptr->btid;
            httpObj_ptr->password_ptr = nafContext_ptr->ksNaf;
            httpObj_ptr->authHandler = NULL;
            httpObj_ptr->authProtocol = HTTP_AUTH_DIGEST;
            break;
        case GBA_NET_APP_AUTHTYPE_AUTO:
            /* weird case here */
            GAA_dbgPrintf("GAA still do auto because no auth header\n");
            httpObj_ptr->username_ptr = NULL;
            httpObj_ptr->password_ptr = NULL;
            httpObj_ptr->authHandler = _GAA_AuthDetectionHandler;
            httpObj_ptr->authProtocol = HTTP_AUTH_ANY;
            break;
        case GBA_NET_APP_AUTHTYPE_DIGEST:
            httpObj_ptr->username_ptr = netAppObj_ptr->appAuthName;
            httpObj_ptr->password_ptr = netAppObj_ptr->appAuthSecret;
            httpObj_ptr->authHandler = NULL;
            httpObj_ptr->authProtocol = HTTP_AUTH_DIGEST;
            break;
    }
    HTTP_applyAuthInfo(httpObj_ptr, NULL);
}
/* 
 * ======== GAA_newNafSession() ========
 * 
 * This function is to init a new GAA Naf session object.
 * Returns: 
 *   GAA_NafSession object pointer, null if not provisioned
 */
GAA_NafSession *GAA_newNafSession(
    char *appName)
{
    // get the context from gba,
    // create http obj using the context
    // in future may use SIP memory pool style allocation
    GBA_NetAppObj   *netAppObj_ptr;
    GAA_NafSession  *nafSession_ptr;
    GBA_NafContext  *nafContext_ptr;

    netAppObj_ptr = GBA_getNafProvision(appName);
    if (NULL == netAppObj_ptr) {
        return (NULL);
    }
    nafSession_ptr = OSAL_memAlloc(sizeof(GAA_NafSession), OSAL_MEM_ARG_STATIC_ALLOC);
    if (NULL == nafSession_ptr) {
        return (NULL);
    }
    OSAL_strncpy(nafSession_ptr->nafAppName, appName, GAA_STRING_SZ);
    nafSession_ptr->netAppObj_ptr = netAppObj_ptr;

    nafContext_ptr = NULL;
    switch (netAppObj_ptr->appAuthType) {
    case GBA_NET_APP_AUTHTYPE_GAA_DIGEST:
    case GBA_NET_APP_AUTHTYPE_GAA_DIGEST_UNKNOWN:
        nafContext_ptr = GBA_getNafContext(appName);
        if (NULL == nafContext_ptr) {
            GBA_bootstrape();
            nafContext_ptr = GBA_getNafContext(appName);
        }
        break;
    case GBA_NET_APP_AUTHTYPE_AUTO:
    case GBA_NET_APP_AUTHTYPE_DIGEST:
        break;
    }
    nafSession_ptr->nafContext_ptr = nafContext_ptr;
    nafSession_ptr->autoDetectedAuthType = GBA_NET_APP_AUTHTYPE_AUTO;

    OSAL_netAddrClear(nafSession_ptr->infcAddr_ptr);

    return (nafSession_ptr);
}

/* 
 * ======== GAA_freeNafSession() ========
 * 
 * This function is to free the GAA Naf session object.
 * Returns: 
 *   OSAL_SUCCESS or OSAL_FAIL.
 */
OSAL_Status GAA_freeNafSession(GAA_NafSession *nafSession_ptr)
{
    HTTP_Obj *httpObj_ptr = &nafSession_ptr->httpObj;

    free(httpObj_ptr->bufs.body_ptr);
    httpObj_ptr->bufs.body_ptr = NULL;
    httpObj_ptr->bufs.bodyBufSz =
            httpObj_ptr->bufs.bodyBufIndex = 0;

    OSAL_memFree(nafSession_ptr, OSAL_MEM_ARG_STATIC_ALLOC);
    return (OSAL_SUCCESS);

}

/*
 * 5.2.4 Bootstrapping required indication
 * NAF shall indicate to the UE that bootstrapped security association is
 * required by sending an HTTP response with code 401 "Unauthorized" and
 * include the WWW-Authenticate header into the response. In particular,
 * the "realm" attribute shall contain a prefix "3GPP-bootstrapping@" or
 * "3GPP-bootstrapping-uicc@" or both, and this shall trigger UE to run
 * bootstrapping procedure over Ub interface.
 */
/*
 * 5.2.5 Bootstrapping renegotiation indication
 * The NAF shall indicate to the UE that the existing bootstrapped security
 * association used in the last HTTP request sent by the UE has expired and
 * that a new bootstrapped security association is required by sending an
 * HTTP response described in subclause 5.2.3. When the UE receives the
 * 401 "Unauthorized" HTTP response to the HTTP request that was protected
 * using the existing bootstrapped security association, this shall trigger
 * the UE to run bootstrapping procedure over Ub interface.
 */
/* 
 * ======== GAA_detectAuthType() ========
 * 
 * This function is to detect NAF GAA auth type
 * Returns: 
 *   GAA_AuthType: the auth type.
 */
OSAL_Status GAA_detectAuthType(GAA_NafSession *nafSession_ptr)
{
    GBA_NetAppObj   *netAppObj_ptr = nafSession_ptr->netAppObj_ptr;

    // to be called by GAA_startNafSession?
    switch (netAppObj_ptr->appAuthType) {
        case GBA_NET_APP_AUTHTYPE_GAA_DIGEST:
        case GBA_NET_APP_AUTHTYPE_GAA_DIGEST_UNKNOWN:
            /* 24.109 section 5.2.5 */
            GBA_bootstrape();
            nafSession_ptr->nafContext_ptr =
                    GBA_getNafContext(netAppObj_ptr->appName);
            break;
        case GBA_NET_APP_AUTHTYPE_AUTO:
            switch (nafSession_ptr->autoDetectedAuthType) {
                case GBA_NET_APP_AUTHTYPE_GAA_DIGEST:
                case GBA_NET_APP_AUTHTYPE_GAA_DIGEST_UNKNOWN:
                    /* 24.109 section 5.2.4 */
                    GBA_bootstrape();
                    nafSession_ptr->nafContext_ptr =
                            GBA_getNafContext(netAppObj_ptr->appName);
                    /* no need to auto detect next time */
                    netAppObj_ptr->appAuthType = GBA_NET_APP_AUTHTYPE_GAA_DIGEST_UNKNOWN;
                    break;
                case GBA_NET_APP_AUTHTYPE_DIGEST:
                    /* no need to auto detect next time */
                    netAppObj_ptr->appAuthType = GBA_NET_APP_AUTHTYPE_DIGEST;
                    break;
                default:
                    /* weird case */
                    break;
            }
            break;
        case GBA_NET_APP_AUTHTYPE_DIGEST:
            /* just do the auth setup */
            break;
    }
    /* update http obj accordingly */
    /* this is what original designed GAA_setupHttpAuth() would do */
    _GAA_updateHttpAuth(nafSession_ptr);
    return (OSAL_SUCCESS);

}


/* 
 * ======== GAA_startNafSession() ========
 * 
 * This function is to start the GAA Naf session.
 * This will trigger a new http request and exchange the digest
 * challenge/response. Then GAA cache the nonce for future use.
 *
 * This function also implemented the 24.109 5.2.4 and 5.2.5 bootstrapping
 * requirement.
 *
 * Returns: 
 *   OSAL_SUCCESS or OSAL_FAIL.
 */
OSAL_Status GAA_startNafSession(GAA_NafSession *nafSession_ptr)
{
    HTTP_Obj        *httpObj_ptr = &nafSession_ptr->httpObj;
    GBA_NetAppObj   *netAppObj_ptr = nafSession_ptr->netAppObj_ptr;
    OSAL_Status  status;

    _GAA_initHttp(nafSession_ptr);

    // try the http connection based on the context
    // auth handler will check the auth setting
    // reissue connect based on the auth setting
    status = HTTP_get(httpObj_ptr, netAppObj_ptr->appUri);
    if (OSAL_FAIL == status) {
        GAA_dbgPrintf("http get app url failed\n");
        return (OSAL_FAIL);
    }
    GAA_dbgPrintf("got header status:\n%s\n",httpObj_ptr->lastHttpStatusLine);
    // if HTTP/1.1 401 Unauthorized, try again
    if (OSAL_strncmp(&httpObj_ptr->lastHttpStatusLine[9], "401 ", 4) == 0) {
        GAA_detectAuthType(nafSession_ptr);
        /* reset buffer and do again based on auto detected auth type */
        httpObj_ptr->bufs.bodyBufIndex = 0;
        status = HTTP_get(httpObj_ptr, netAppObj_ptr->appUri);
    }

    GAA_dbgPrintf("got header status:\n%s\n",httpObj_ptr->lastHttpStatusLine);
    GAA_dbgPrintf("body size/index:%d/%d\n",
            httpObj_ptr->bufs.bodyBufSz,
            httpObj_ptr->bufs.bodyBufIndex);
    GAA_dbgPrintf("got body:\n%s\n",httpObj_ptr->bufs.body_ptr);

    return (OSAL_SUCCESS);
}


/* 
 * ======== GAA_stopNafSession() ========
 * 
 * This function is to stop the GAA Naf session.
 * Returns: 
 *   OSAL_SUCCESS or OSAL_FAIL.
 */
OSAL_Status GAA_stopNafSession(GAA_NafSession *nafSession_ptr)
{
    /* house keeping */
    HTTP_Obj *httpObj_ptr = &nafSession_ptr->httpObj;

    HTTP_cleanup(httpObj_ptr);

    /* call GAA_freeNafSession() to free memory */
    return (OSAL_SUCCESS);
}

