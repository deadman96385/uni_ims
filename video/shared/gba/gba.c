/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev$ $Date$
 *
 */

#include "osal.h"
#include "http.h"
#include "gba.h"
#include "_gba.h"
#include "auth_b64.h"
#include "ezxml.h"
#include <osal_crypto.h>


/* Used for authorization */
#define _GBA_USERNAME_ARG_STR_SIZE        (64)
#define _GBA_REALM_ARG_STR_SIZE           (64)
#define _GBA_PASSWORD_ARG_STR_SIZE        (64)
#define _GBA_NONCE_ARG_STR_SIZE           (128)
#define _GBA_CNONCE_ARG_STR_SIZE          (128)
#define _GBA_NONCE_CNT_ARG_STR_SIZE       (12)
#define _GBA_OPAQUE_ARG_STR_SIZE          (128)
#define _GBA_RESPONSE_ARG_STR_SIZE        (128)
#define _GBA_CNONCE_RAND_MIN_LEN          (8)
#define _GBA_CNONCE_RAND_MAX_LEN          (15)
#define _GBA_BOUNDRY_MAX_LEN              (32)

#ifndef GBA_DEBUG
#define GBA_dbgPrintf(x, ...)
#else
#define GBA_dbgPrintf OSAL_logMsg
#endif


#define _GBA_UA_SEC_ID_LEN      (5)
/* special one for the ericsson server */
static char _GBA_UaSecProto3gppUnknown[_GBA_UA_SEC_ID_LEN] = { 1, 0, 0, 0, 0 };

/* 33.220 H.3 protocol ids*/
/*
static uint8 _GBA_UaSecProtoUnknown[_GBA_UA_SEC_ID_LEN] = { 0, 0, 0, 0, 0 }; 
static uint8 _GBA_UaSecProto_33_221[_GBA_UA_SEC_ID_LEN] = { 1, 0, 0, 0, 0 };
static uint8 _GBA_UaSecProto_33_246[_GBA_UA_SEC_ID_LEN] = { 1, 0, 0, 0, 1 };
static uint8 _GBA_UaSecProto_24_109[_GBA_UA_SEC_ID_LEN] = { 1, 0, 0, 0, 2 };
*/
#define _GBA_SERVER_COOKIE_HF_STR "Cookie2: $Version=1"
/*
 * GBA is a singleton object. Only one instance would interact with BSF.
 * GBA module assumed single thread access, bootstrape() is not thread-safe
 */
static GBA_GlobalObj _GBA_globalObj;

/*
 * The GBA AKA flow is based on the HTTP/SIP AKA flow
 * Assumed libcurl underneath patched to support
 * 1. algo "AKAv1-MD5" is considered as CURLDIGESTALGO_MD5
 * 2. qop="auth-int" support by using empty entity-body ""
 */

/*
 * ======== _GBA_DecodeNonce() ========
 * This function is to decode nonce string to RAND and AUTN
 *
 * nonce_ptr  : nonce string (not necessary null terminated)
 * nonceLen   : nonce string length
 * rand_ptr   : Pointer to rand string
 * randLen    : Length of rand string
 * autn_ptr   : pointer to autn string
 * autnLen    : Length of autn string
 *
 * Returns:
 *  GBA_OK : Decode nonce successfully
 *  GBA_ERR: Failed to decode nonce.
 */
vint _GBA_DecodeNonce(
    char  *nonce_ptr,
    vint   nonceLen,
    char *rand_ptr,
    vint   randLen,
    char *autn_ptr,
    vint   autnLen)
{
    vint    len;
    uint8   nonce[_GBA_NONCE_ARG_STR_SIZE + 1];

    /* Zero the nonce */
    OSAL_memSet(nonce, 0, sizeof(nonce));

    if (0 == (len = OSAL_cryptoB64Decode(nonce_ptr, nonceLen, (char *)nonce))) {
        /* Failed to decode the nonce */
        return (GBA_ERR);
    }

    /* Verify the length of the decoded nonce */
    if (len < GBA_AUTH_AKA_RANDLEN + GBA_AUTH_AKA_AUTNLEN) {
        return (GBA_ERR);
    }

    if ((GBA_AUTH_AKA_RANDLEN != randLen) ||
            (GBA_AUTH_AKA_AUTNLEN != autnLen)) {
        return (GBA_ERR);
    }

    /* Get RAND and AUTN */
    OSAL_memCpy(rand_ptr, nonce, GBA_AUTH_AKA_RANDLEN);
    OSAL_memCpy(autn_ptr, nonce + GBA_AUTH_AKA_RANDLEN, GBA_AUTH_AKA_AUTNLEN);

    GBA_dbgPrintf("%s %d: rand\n",
            __FILE__, __LINE__ );
    HTTP_hexDump(rand_ptr, GBA_AUTH_AKA_RANDLEN);
    GBA_dbgPrintf("\n%s %d: autn\n",
            __FILE__, __LINE__ );
    HTTP_hexDump(autn_ptr, GBA_AUTH_AKA_AUTNLEN);

    return (GBA_OK);
}

/* 
 * ======== _GBA_authHandler() ========
 * 
 * This function is a http callback for GBA auth response.
 * Parse and decode nonce for AKA challenge/response
 *
 * Returns: 
 *   data octets read by application.
 */
static OSAL_Boolean _GBA_authHandler(
    HTTP_Obj *httpObj_ptr,
    char     *authHeader,
    int      size)
{
    char         *nonce_ptr;
    vint          nonceLen;
    GBA_Event    *evt_ptr;
    GBA_Obj      *gbaObj_ptr;

    /* get auth type
     * assumed this is digest auth,
     * since this is what http lib will check and call this handler
     */
    gbaObj_ptr = (GBA_Obj *)httpObj_ptr->arg_ptr;
    evt_ptr = &gbaObj_ptr->gbaEventScratch;
    /* do gba aka auth */
    HTTP_getParameterValue(authHeader, GBA_NONCE_HF_ARG_STR, &nonce_ptr, &nonceLen);

    if (GBA_OK != _GBA_DecodeNonce(nonce_ptr,
            nonceLen,
            gbaObj_ptr->akaRand,
            sizeof(gbaObj_ptr->akaRand),
            gbaObj_ptr->akaAutn,
            sizeof(gbaObj_ptr->akaAutn))) {
        /* let the power fall */
        GBA_dbgPrintf("%s %d: AKA response decode nonce failed\n",
                __FILE__, __LINE__);
    }
    else {
        /* Decode nonce successfully, Send event to CSM */
        evt_ptr->type = GBA_EVENT_AKA_CHALLENGE;
        evt_ptr->u.akaChallenge.rand_ptr = gbaObj_ptr->akaRand;
        evt_ptr->u.akaChallenge.autn_ptr = gbaObj_ptr->akaAutn;
        /* only send event after the body reading is done. */
    }

    return OSAL_FALSE;
}


/* 
 * ======== GBA_init() ========
 * 
 * This function is to init GBA object.
 * Returns: 
 *   GBA_OK or GBA_ERR.
 */
vint GBA_init()
{

    /*  Init and clear the global object that keep gba context */
    OSAL_memSet(&_GBA_globalObj, 0, sizeof(_GBA_globalObj));
    
    /* Create command queue */
    _GBA_globalObj.gbaObj.cmdq = OSAL_msgQCreate(GBA_COMMAND_QUEUE_NAME,
            OSAL_MODULE_GBAM, OSAL_MODULE_GBAM, OSAL_DATA_STRUCT_GBA_Command,
            GBA_MSGQ_LEN,
            sizeof(GBA_Command),
            0);
    if (NULL == _GBA_globalObj.gbaObj.cmdq) {
        return (-1);
    }

    /* Create event queue */
    _GBA_globalObj.gbaObj.evtq = OSAL_msgQCreate(GBA_EVENT_QUEUE_NAME,
            OSAL_MODULE_GBAM, OSAL_MODULE_GBAM, OSAL_DATA_STRUCT_GBA_Event,
            GBA_MSGQ_LEN,
            sizeof(GBA_Event),
            0);
    if (NULL == _GBA_globalObj.gbaObj.evtq) {
        OSAL_msgQDelete(_GBA_globalObj.gbaObj.cmdq);
        return (-1);
    }

    /* Create task */
    _GBA_globalObj.gbaObj.tid = OSAL_taskCreate(GBA_TASK_NAME,
            OSAL_TASK_PRIO_NRT,
            GBA_TASK_STACK_BYTES,
            _GBA_task,
            (void *)&_GBA_globalObj.gbaObj);
    if (NULL == _GBA_globalObj.gbaObj.tid) {
        return (0);
    }

    /*
     * fill globals using the setup function
     * usually when ISIM is ready
     */
    return (GBA_OK);
}

/* 
 * ======== GBA_setup() ========
 * 
 * This function is to init GBA context.
 * the context included necessary info to start bootstraping
 * and control/provide NAF gaa auth
 *
 * Returns: 
 *   GBA_OK or GBA_ERR.
 */
vint GBA_setup(
    char *bsf_ptr,
    char *impi)
{
    /* fill global using these parameters */
    int isSsl;
    
    /* parse bsf_ptr into fqdn and url */
    OSAL_strncpy(_GBA_globalObj.gbaObj.bsfUrl, bsf_ptr, GBA_FQDN_STRING_SZ);
    OSAL_strncpy(_GBA_globalObj.gbaObj.impi, impi, GBA_STRING_SZ);

    HTTP_urlToHostPortPath(_GBA_globalObj.gbaObj.bsfUrl,
            _GBA_globalObj.gbaObj.bsfHost,
            &_GBA_globalObj.gbaObj.bsfPort,
            _GBA_globalObj.gbaObj.bsfPath,
            &isSsl);

    _GBA_globalObj.gbaObj.netAppCount = 0;
    OSAL_memSet(_GBA_globalObj.gbaObj.netAppInfos, 0,
            sizeof(GBA_NetAppObj)*GBA_NET_APP_MAX_COUNT);

    return (GBA_OK);
}

/* 
 * ======== GBA_shutdown() ========
 * 
 * Deallocates all resources allocated by GBA_init().
 *
 * Returns: 
 *   GBA_OK or GBA_ERR.
 */
vint GBA_shutdown()
{
    GBA_Command gbaCommand;

    gbaCommand.type = GBA_COMMAND_EXIT;
    GBA_sendCommand(&gbaCommand);

    OSAL_semDelete(_GBA_globalObj.gbaObj.taskLock);

    return (GBA_OK);
}

/*
 * ======== GBA_registerNetApp() ========
 *
 * Save the net app info for future Naf sessions
 *
 * Returns:
 *   GBA_OK or GBA_ERR.
 */
vint GBA_registerNetApp(
    GBA_NetAppObj   *netAppObj_ptr)
{
    /* quick hack way now. later using DLL */
    GBA_Obj *obj_ptr = &_GBA_globalObj.gbaObj;
    if (obj_ptr->netAppCount < GBA_NET_APP_MAX_COUNT) {
        OSAL_memCpy(&obj_ptr->netAppInfos[obj_ptr->netAppCount++],
                netAppObj_ptr,
                sizeof(GBA_NetAppObj));
    } else {
        return (GBA_ERR);
    }
    return (GBA_OK);
}

/*
 * ======== GBA_unRegisterNetApp() ========
 *
 * Remove the net app from the stored list
 *
 * Returns:
 *   GBA_OK or GBA_ERR.
 */
vint GBA_unRegisterNetApp(
    char   *netAppName)
{
    /* quick hack way now. later using DLL */
    GBA_Obj *obj_ptr = &_GBA_globalObj.gbaObj;
    int idx;

    for (idx=0; idx < obj_ptr->netAppCount; idx++) {
        if (0 == OSAL_strcmp(obj_ptr->netAppInfos[idx].appName, netAppName)){
            break;
        }
    }
    if (idx < obj_ptr->netAppCount) {
        OSAL_memCpy(&obj_ptr->netAppInfos[idx],
                &obj_ptr->netAppInfos[idx+1],
                sizeof(GBA_NetAppObj)*(obj_ptr->netAppCount-idx));
        OSAL_memCpy(&obj_ptr->netAppContext[idx],
                &obj_ptr->netAppContext[idx+1],
                sizeof(GBA_NafContext)*(obj_ptr->netAppCount-idx));
        obj_ptr->netAppCount -= 1;
        return (GBA_OK);
    } else {
        return (GBA_ERR);
    }
}

/*
 * ======== GBA_bootstrape() ========
 *
 * Bootstrapping with BSF. Blocked until the process is done.
 * Should be called outside the GBA task or it will block itself.
 *
 * Returns:
 *   GBA_OK or GBA_ERR.
 */
vint GBA_bootstrape(
    void)
{
    OSAL_Status  status;

    /*
     * 1. prepare http session to bsf with customized header
     * 2. auth handler get the aka challenge and send to csm
     * 3. blocking wait for return first
     * 4. csm aka response back and setup next http request with resp/ik/ck
     * 5. get the bsf xml
     * 6. parse the xml and store the btid and life time
     */
    _GBA_initHttp(&_GBA_globalObj.gbaObj.httpObj);

    GBA_dbgPrintf("try to connect to :%s\n", _GBA_globalObj.gbaObj.bsfUrl);

    status = HTTP_get(&_GBA_globalObj.gbaObj.httpObj, _GBA_globalObj.gbaObj.bsfUrl);
    if (OSAL_FAIL == status) {
        GBA_dbgPrintf("http get bsf failed\n");
        return (0);
    }
    GBA_dbgPrintf("got header status:\n%s\n",
            _GBA_globalObj.gbaObj.httpObj.lastHttpStatusLine);
    GBA_dbgPrintf("body size/index:%d/%d\n",
            _GBA_globalObj.gbaObj.httpObj.bufs.bodyBufSz,
            _GBA_globalObj.gbaObj.httpObj.bufs.bodyBufIndex);
    GBA_dbgPrintf("got body:\n%s\n",_GBA_globalObj.gbaObj.httpObj.bufs.body_ptr);

    /*
     * if HTTP/1.1 401 Unauthorized, try again when received the AKA response
     * steps 4,5,6 would be done in _GBA_bootstrapeDone()
     */
    if (0 == OSAL_strncmp(&_GBA_globalObj.gbaObj.httpObj.lastHttpStatusLine[9],
            "401 ", 4)) {
        /*
         * lastHttpStatusLine is 401, then continue the boot strape by doing aka.
         * _GBA_authHandler() prepared the gbaEventScratch
         */
        GBA_sendEvent(&_GBA_globalObj.gbaObj.gbaEventScratch);

        OSAL_semAcquire(_GBA_globalObj.gbaObj.taskLock, OSAL_WAIT_FOREVER);
    }
    return (GBA_OK);
}

/*
 * Sample BSF xml file to parse
 * <?xml version="1.0" encoding="UTF-8"?>
<BootstrappingInfo xmlns="uri:3gpp-gba">
    <btid>x1XRv9GU/jU1U5DrkYljrQ==@bsf.labs.ericsson.net</btid>
    <lifetime>2013-04-22T13:10:25.559Z</lifetime>
</BootstrappingInfo>
 */
/*
 * ======== _GBA_parseBootstrappingInfo() ========
 *
 * parse the returned BootstrappingInfo from BSF.
 * xml schema defined in 24.109 Annex C
 *
 * Returns:
 *   GBA_OK or GBA_ERR.
 */
vint _GBA_parseBootstrappingInfo(
    char *doc_ptr,
    size_t docLen)
{
    ezxml_t  xml_ptr, node_ptr;
    char    *str_ptr;

    xml_ptr = NULL;
    if (NULL != doc_ptr) {
        xml_ptr = ezxml_parse_str(doc_ptr, docLen);
    }
    if (NULL == xml_ptr) {
        return (GBA_ERR);
    }

    /*  Now verify that it meets 3gpp 24.109. */
    str_ptr = (char *)ezxml_attr(xml_ptr,
            (const char *)GBA_XMLNS_ATTR);
    if (NULL != str_ptr) {
        if (0 != OSAL_strcmp(GBA_XMLNS_URI, str_ptr)) {
            ezxml_remove(xml_ptr);
            return (GBA_ERR);
        }
    }
    else {
        ezxml_remove(xml_ptr);
        return (GBA_ERR);
    }

    if (NULL == (node_ptr = ezxml_child(xml_ptr, GBA_XML_BTID))) {
        return (GBA_ERR);
    }
    if (NULL == (str_ptr = ezxml_txt(node_ptr))) {
        return (GBA_ERR);
    }
    OSAL_strncpy(_GBA_globalObj.gbaObj.btid, str_ptr, GBA_STRING_SZ);

    if (NULL == (node_ptr = ezxml_child(xml_ptr, GBA_XML_LIFETIME))) {
        return (GBA_ERR);
    }
    if (NULL == (str_ptr = ezxml_txt(node_ptr))) {
        return (GBA_ERR);
    }
    OSAL_strncpy(_GBA_globalObj.gbaObj.lifeTime, str_ptr, GBA_STRING_SZ);

    return (GBA_OK);
}


/*
 * ======== _GBA_bootstrapeAkaFailure() ========
 *
 * need to retry the bootstraping after the aka failure
 *
 * Returns:
 *   GBA_OK or GBA_ERR.
 */
vint _GBA_bootstrapeAkaFailure(
    GBA_Command *cmd_ptr)
{
    GBA_Obj        *obj_ptr = &_GBA_globalObj.gbaObj;
    HTTP_Obj       *httpObj_ptr = &obj_ptr->httpObj;
    OSAL_Status     status;
    char           b64str[GBA_AKA_AUTH_RESP_SZ*2+1];

    /*
     * trigger next http get bsf with correct digest
     * calculate response from aka response and convert from bytes to hex string
     */

    httpObj_ptr->customHeadersCount = 3;

    httpObj_ptr->customHeaders[2] = _GBA_SERVER_COOKIE_HF_STR;

    // let libcurl do the digest math for us
    httpObj_ptr->authProtocol = HTTP_AUTH_DIGEST;
    httpObj_ptr->username_ptr = obj_ptr->impi;

    /*
     * auts
      A string carrying a base64 encoded AKA AUTS parameter.  This
      directive is used to re-synchronize the server side SQN.  If the
      directive is present, the client doesn't use any password when
      calculating its credentials.  Instead, the client MUST calculate
      its credentials using an empty password (password of "").
     */
    httpObj_ptr->password_ptr = "";
    GBA_dbgPrintf("\ngot AKA response, will auth with u:p=%s:%s\n",
            httpObj_ptr->username_ptr,
            httpObj_ptr->password_ptr);

    /* base64 convert cmd_ptr->u.akaResponse.auts */
    OSAL_cryptoB64Encode(cmd_ptr->u.akaResponse.auts, b64str, GBA_AKA_AUTH_AUTS_SZ);
    HTTP_applyAuthInfo(httpObj_ptr, b64str);

    /* reset buffer */
    httpObj_ptr->bufs.bodyBufIndex = 0;

    status = HTTP_get(httpObj_ptr, obj_ptr->bsfUrl);

    if (OSAL_FAIL == status) {
        GBA_dbgPrintf("http get bsf resync failed\n");

        // let gba  know the failure.
        _GBA_globalObj.gbaObj.bootstrapped = OSAL_FALSE;
        OSAL_semGive(_GBA_globalObj.gbaObj.taskLock);
        return (GBA_ERR);
    }
    GBA_dbgPrintf("got header status:\n%s\n",
            _GBA_globalObj.gbaObj.httpObj.lastHttpStatusLine);
    GBA_dbgPrintf("body size/index:%d/%d\n",
            _GBA_globalObj.gbaObj.httpObj.bufs.bodyBufSz,
            _GBA_globalObj.gbaObj.httpObj.bufs.bodyBufIndex);
    GBA_dbgPrintf("got body:\n%s\n",_GBA_globalObj.gbaObj.httpObj.bufs.body_ptr);

    /*
     * if HTTP/1.1 401 Unauthorized, try again when received the AKA response
     * steps 4,5,6 would be done in _GBA_bootstrapeDone()
     */
    if (0 == OSAL_strncmp(&_GBA_globalObj.gbaObj.httpObj.lastHttpStatusLine[9],
            "401 ", 4)) {
        /*
         * lastHttpStatusLine is 401, then continue the bootstrape by doing aka.
         * _GBA_authHandler() prepared the gbaEventScratch already.
         */
        GBA_sendEvent(&_GBA_globalObj.gbaObj.gbaEventScratch);
        /*
         * Use next aka response event to complete the process,
         * hopefully to call _GBA_bootstrapeDone
         */
        return (GBA_OK);
    }

    /*
     * other status codes are considered unexpected and error,
     * let gba the know the failure.
     */
    _GBA_globalObj.gbaObj.bootstrapped = OSAL_FALSE;
    OSAL_semGive(_GBA_globalObj.gbaObj.taskLock);
    return (GBA_ERR);
}

/*
 * ======== _GBA_bootstrapeDone() ========
 *
 * complete the bootstraping
 *
 * Returns:
 *   GBA_OK or GBA_ERR.
 */
vint _GBA_bootstrapeDone(
    GBA_Command *cmd_ptr)
{
    GBA_Obj        *obj_ptr = &_GBA_globalObj.gbaObj;
    HTTP_Obj       *httpObj_ptr = &obj_ptr->httpObj;
    OSAL_Status     status;

    /* 
     * trigger next http get bsf with correct digest
     * calculate response from aka response and convert from bytes to hex string
     */

    httpObj_ptr->customHeadersCount = 3;

    httpObj_ptr->customHeaders[2] = _GBA_SERVER_COOKIE_HF_STR;

    // let libcurl do the digest math for us
    httpObj_ptr->authProtocol = HTTP_AUTH_DIGEST;
    httpObj_ptr->username_ptr = obj_ptr->impi;
    httpObj_ptr->password_ptr = cmd_ptr->u.akaResponse.resp;
    GBA_dbgPrintf("\ngot AKA response, will auth with u:p=%s:%s\n",
            httpObj_ptr->username_ptr,
            httpObj_ptr->password_ptr);

    HTTP_applyAuthInfo(httpObj_ptr, NULL);

    OSAL_memCpy(obj_ptr->akaAuthCk, cmd_ptr->u.akaResponse.ck, GBA_AKA_AUTH_CK_SZ);
    OSAL_memCpy(obj_ptr->akaAuthIk, cmd_ptr->u.akaResponse.ik, GBA_AKA_AUTH_IK_SZ);
    GBA_dbgPrintf("\nck\n");
    HTTP_hexDump(obj_ptr->akaAuthCk, GBA_AKA_AUTH_CK_SZ);
    GBA_dbgPrintf("\nik\n");
    HTTP_hexDump(obj_ptr->akaAuthIk, GBA_AKA_AUTH_IK_SZ);

    /* reset buffer */
    httpObj_ptr->bufs.bodyBufIndex = 0;

    status = HTTP_get(httpObj_ptr, obj_ptr->bsfUrl);
    
    if (OSAL_FAIL == status) {
        GBA_dbgPrintf("http get bsf resync failed\n");

        // let gba  know the failure.
        _GBA_globalObj.gbaObj.bootstrapped = OSAL_FALSE;
        OSAL_semGive(_GBA_globalObj.gbaObj.taskLock);
        return (GBA_ERR);
    }
    
    _GBA_parseBootstrappingInfo(httpObj_ptr->bufs.body_ptr, 
            httpObj_ptr->bufs.bodyBufIndex);
    GBA_dbgPrintf("parsed BootstrappingInfo:\nbtid=%s\nlifetime=%s\n",
            obj_ptr->btid, obj_ptr->lifeTime);
    HTTP_cleanup(httpObj_ptr);

    OSAL_memFree(httpObj_ptr->bufs.body_ptr, OSAL_MEM_ARG_DYNAMIC_ALLOC);
    httpObj_ptr->bufs.bodyBufSz =
            httpObj_ptr->bufs.bodyBufIndex = 0;

    // let gba the know the success.
    _GBA_globalObj.gbaObj.bootstrapped = OSAL_TRUE;
    OSAL_semGive(_GBA_globalObj.gbaObj.taskLock);

    return (GBA_OK);
}

/* 
 * ======== GBA_getNafProvision() ========
 *
 * This function returns a pointer to the app's provisioned info.
 * 
 * Returns: 
 *  A pointer to the GBA_NetAppObj
 *  
 */
GBA_NetAppObj *GBA_getNafProvision(
    char *netAppName)
{
    /* quick hack way now. later using DLL */
    GBA_Obj *obj_ptr = &_GBA_globalObj.gbaObj;
    int idx;

    for (idx=0; idx < obj_ptr->netAppCount; idx++) {
        if (0 == OSAL_strcmp(obj_ptr->netAppInfos[idx].appName, netAppName)){
            break;
        }
    }
    if (idx < obj_ptr->netAppCount) {
        return (&obj_ptr->netAppInfos[idx]);
    } else {
        return (NULL);
    }
}

uint16 _GBA_swapUint16( uint16 val )
{
    return (val << 8) | (val >> 8 );
}

/*
 * ======== GBA_calcKsNaf() ========
 *
 * Prepare the net app Naf sessions by calculating the NafKs
 *
 * Returns:
 *   GBA_OK or GBA_ERR.
 */
vint GBA_calcKsNaf(void *ks,
        void *rand,
        char *impi,
        char *naf_id,
        char *uaSecProtocol,
        char *result_ptr)
{
    /* quick hack way now. later using DLL */
    OSAL_CryptoCtxId ctxId;
    size_t          mdSize;
    uint16          twoBytes;


    /* 3gpp 33.220: Ks_NAF = KDF (Ks, "gba-me", RAND, IMPI, NAF_Id) */
    ctxId = OSAL_cryptoAllocCtx();
    OSAL_cryptoHmacInit(ctxId, ks,
            GBA_AKA_AUTH_CK_SZ+GBA_AKA_AUTH_IK_SZ,
            OSAL_CRYPTO_MD_ALGO_SHA256);

    OSAL_cryptoHmacUpdate(ctxId, "\001", 1);

    twoBytes = 6;
    OSAL_cryptoHmacUpdate(ctxId, "gba-me", twoBytes);
    twoBytes = _GBA_swapUint16(twoBytes);
    OSAL_cryptoHmacUpdate(ctxId, &twoBytes, 2);

    twoBytes = GBA_AUTH_AKA_RANDLEN;
    OSAL_cryptoHmacUpdate(ctxId, rand, twoBytes);
    twoBytes = _GBA_swapUint16(twoBytes);
    OSAL_cryptoHmacUpdate(ctxId, &twoBytes, 2);

    twoBytes = OSAL_strlen(impi);
    OSAL_cryptoHmacUpdate(ctxId, impi, twoBytes);
    twoBytes = _GBA_swapUint16(twoBytes);
    OSAL_cryptoHmacUpdate(ctxId, &twoBytes, 2);

    twoBytes = OSAL_strlen(naf_id);
    OSAL_cryptoHmacUpdate(ctxId, naf_id, twoBytes);
    OSAL_cryptoHmacUpdate(ctxId, uaSecProtocol, _GBA_UA_SEC_ID_LEN);
    twoBytes += _GBA_UA_SEC_ID_LEN;
    twoBytes = _GBA_swapUint16(twoBytes);
    OSAL_cryptoHmacUpdate(ctxId, &twoBytes, 2);

    OSAL_cryptoHmacFinal(ctxId, (unsigned char *)result_ptr, &mdSize);
    result_ptr[mdSize] = '\0';
    OSAL_cryptoFreeCtx(ctxId);

    return (GBA_OK);
}


/*
 * ======== GBA_prepareNetApp() ========
 *
 * Prepare the net app Naf sessions by calculating the NafKs
 *
 * Returns:
 *   GBA_OK or GBA_ERR.
 */
vint GBA_prepareNetApp(
    GBA_NetAppObj   *netAppInfo_ptr,
    GBA_NafContext  *netAppContext_ptr)
{
    /* quick hack way now. later using DLL */
    GBA_Obj *obj_ptr = &_GBA_globalObj.gbaObj;
    char            nafHost[GBA_FQDN_STRING_SZ];
    int             nafPort;
    char            nafPath[GBA_STRING_SZ];
    int             isSsl;
    char            ks[GBA_AKA_AUTH_CK_SZ+GBA_AKA_AUTH_IK_SZ+1];
    char           *uaSecProtocol;

    /* 3gpp 33.220: Ks_NAF = KDF (Ks, "gba-me", RAND, IMPI, NAF_Id) */

    /* derive Ks from ck|ik */
    OSAL_memCpy(ks, obj_ptr->akaAuthCk, GBA_AKA_AUTH_CK_SZ);
    OSAL_memCpy(&ks[GBA_AKA_AUTH_CK_SZ], obj_ptr->akaAuthIk, GBA_AKA_AUTH_IK_SZ);
    ks[GBA_AKA_AUTH_CK_SZ+GBA_AKA_AUTH_IK_SZ]=0;

    GBA_dbgPrintf("GBA_prepareNetApp: ks=\n");
    HTTP_hexDump(ks,GBA_AKA_AUTH_CK_SZ+GBA_AKA_AUTH_IK_SZ);
    GBA_dbgPrintf("\n");

    /* derive naf_id from app Uri */
    HTTP_urlToHostPortPath(netAppInfo_ptr->appUri,
                nafHost,
                &nafPort,
                nafPath,
                &isSsl);

    /* xxx switch (netAppInfo_ptr->appAuthType) */
    uaSecProtocol = _GBA_UaSecProto3gppUnknown;
    GBA_calcKsNaf(ks,
            obj_ptr->akaRand,
            obj_ptr->impi,
            nafHost,
            uaSecProtocol,
            netAppContext_ptr->ksNaf);

    OSAL_strncpy(netAppContext_ptr->btid, obj_ptr->btid, GBA_STRING_SZ);
    GBA_dbgPrintf("GBA_prepareNetApp: ksNaf=\n");
    HTTP_hexDump(netAppContext_ptr->ksNaf, GBA_KS_NAF_LEN);
    GBA_dbgPrintf("\n");

    return (GBA_OK);
}



/* 
 * ======== GBA_getNafContext() ========
 *
 * This function returns a pointer to the app's GBA_NafContext.
 * 
 * Returns: 
 *  A pointer to the GBA_NafContext
 *  
 */
GBA_NafContext *GBA_getNafContext(
    char *netAppName)
{
    GBA_Obj *obj_ptr = &_GBA_globalObj.gbaObj;
    int idx;

    if (OSAL_FALSE == obj_ptr->bootstrapped) {
        return NULL;
    }
    for (idx=0; idx < obj_ptr->netAppCount; idx++) {
        if (0 == OSAL_strcmp(obj_ptr->netAppInfos[idx].appName, netAppName)){
            break;
        }
    }
    if (idx < obj_ptr->netAppCount) {
        if (0 == obj_ptr->netAppContext[idx].btid[0]) {
            if (GBA_OK != GBA_prepareNetApp(&obj_ptr->netAppInfos[idx],
                    &obj_ptr->netAppContext[idx])) {
                return NULL;
            }
        }
        return (&obj_ptr->netAppContext[idx]);
    }
    return (NULL);
}


/*
 * ======== GBA_getCommand() ========
 *
 * For GBA to get command from other module. Example usage is CSM to send AKA response.
 *
 * 1 : One command received.
 * 0 : Error or no command.
 */
vint GBA_getCommand(
     GBA_Command *cmd_ptr,
     int       msTimeout)
{
    if (NULL == cmd_ptr) {
        return (0);
    }

    if (sizeof(GBA_Event) != OSAL_msgQRecv(_GBA_globalObj.gbaObj.cmdq,
            (char *)cmd_ptr,
            sizeof(GBA_Command),
            msTimeout < 0 ? OSAL_WAIT_FOREVER : msTimeout,
            NULL)) {
        return (0);
    }
    GBA_dbgPrintf("%s:%d", __FILE__, __LINE__);
    return (1);
}

/*
 * ======== GBA_sendCommand() ========
 *
 * This function sends command to GBA from other module such as CSM.
 * GBA could use GBA_getCommand() to read such command
 *
 * Returns:
 * 1 : One command sent.
 * 0 : Error or no command.
 */

int GBA_sendCommand(
     GBA_Command *cmd_ptr)
{
    if (NULL == cmd_ptr) {
        return (0);
    }

    if (GBA_OK == OSAL_msgQSend(_GBA_globalObj.gbaObj.cmdq,
            (char *)cmd_ptr,
            sizeof(GBA_Command),
            OSAL_WAIT_FOREVER,
            NULL)) {
        GBA_dbgPrintf("%s:%d", __FILE__, __LINE__);
        return (1);
    }
    GBA_dbgPrintf("%s:%d", __FILE__, __LINE__);
    return (0);
}

/*
 * ======== GBA_getEvent() ========
 *
 * For other module to get event from GBA. Example usage is for CSM to get AKA challenge.
 *
 * 1 : One event received.
 * 0 : Error or no event.
 */
vint GBA_getEvent(
     GBA_Event *evt_ptr,
     int       msTimeout)
{
    vint evtSize;

    if (NULL == evt_ptr) {
        return (0);
    }

    if (sizeof(GBA_Event) != (evtSize=OSAL_msgQRecv(_GBA_globalObj.gbaObj.evtq,
            (char *)evt_ptr,
            sizeof(GBA_Event),
            msTimeout < 0 ? OSAL_WAIT_FOREVER : msTimeout,
            NULL))) {
        GBA_dbgPrintf("%s:%d OSAL_msgQRecv error got size:%d\n", 
                __FILE__, __LINE__, evtSize);
        return (0);
    }
    GBA_dbgPrintf("%s:%d OSAL_msgQRecv ok got size:%d\n", __FILE__, __LINE__, evtSize);
    return (1);
}

/*
 * ======== GBA_sendEvent() ========
 *
 * This function sends event from GBA to other module such as CSM.
 * Other module could use GBA_getEvt() to read such event
 *
 * Returns:
 * 1 : Event sent.
 * 0 : Error.
 */
vint GBA_sendEvent(
        GBA_Event *evt_ptr)
{
    if (OSAL_SUCCESS != OSAL_msgQSend(_GBA_globalObj.gbaObj.evtq,
            (char *)evt_ptr,
            sizeof(GBA_Event),
            OSAL_WAIT_FOREVER,
            NULL)) {
        GBA_dbgPrintf("%s:%d ERROR msgQ send FAILED qId=%d\n", __FUNCTION__,
                __LINE__, (int)_GBA_globalObj.gbaObj.evtq);
        return (0);
    }
    return (1);
}

/*
 * ======== _GBA_task() ========
 *
 * This is one and only task of GBA. It receives sequentially, commands from
 * application, processes these commands and trigger other http actions in the 
 * gba task context.
 *
 * Returns:
 *  Never returns.
 *
 */
OSAL_TaskReturn _GBA_task(
    OSAL_TaskArg arg_ptr)
{
    GBA_Obj        *obj_ptr = (GBA_Obj *)arg_ptr;
    GBA_Command     cmd;
    vint            len;

    if (NULL == (_GBA_globalObj.gbaObj.taskLock = OSAL_semMutexCreate())) {
        return (GBA_ERR); 
    }

_GBA_TASK_LOOP:
    /* Receive a valid command. */
    len = OSAL_msgQRecv(obj_ptr->cmdq,
            (char *)&cmd,
            sizeof(cmd),
            OSAL_WAIT_FOREVER,
            NULL);

    /* Check if exit command. */
    if (obj_ptr->exit) {
        goto _GBA_TASK_EXIT;
    }

    /* validate */
    if (len != sizeof(cmd)) {
        OSAL_taskDelay(100);
        goto _GBA_TASK_LOOP;
    }

    /*
     * Process cmd.
     */
    switch (cmd.type) {
        case GBA_COMMAND_AKA_RESPONSE_SUCCESS:
            _GBA_bootstrapeDone(&cmd);
            break;
        case GBA_COMMAND_AKA_RESPONSE_NETWORK_FAILURE:
            // let gba  know the failure.
            _GBA_globalObj.gbaObj.bootstrapped = OSAL_FALSE;
            OSAL_semGive(_GBA_globalObj.gbaObj.taskLock);
            break;
        case GBA_COMMAND_AKA_RESPONSE_SYNC_FAILURE:
            _GBA_bootstrapeAkaFailure(&cmd);
            break;
        case GBA_COMMAND_EXIT:
            obj_ptr->exit = OSAL_TRUE;
            goto _GBA_TASK_EXIT;
            break;
        default:
            break;
    }
    goto _GBA_TASK_LOOP;

_GBA_TASK_EXIT:

    /*
     * Task exit, better than killing it.
     */
    OSAL_msgQDelete(obj_ptr->cmdq);
    OSAL_msgQDelete(obj_ptr->evtq);
    GBA_dbgPrintf("%s:%d", __FILE__, __LINE__);

    return (0);
}

/*
 * ======== _GBA_initHttp() ========
 *
 * Prepare a http context to initiate the bsf bootstraping
 *
 * Returns:
 *  void
 *
 */
void _GBA_initHttp(
    HTTP_Obj *httpObj_ptr)
{
    GBA_Obj        *obj_ptr;

    obj_ptr = &_GBA_globalObj.gbaObj;

    httpObj_ptr->followLocation = OSAL_TRUE;
    httpObj_ptr->timeoutsec = 0;
    OSAL_strncpy(httpObj_ptr->certificate,
            "/etc/ssl/certs/ca-certificates.crt",
            sizeof(httpObj_ptr->certificate));

    /*
     * Extra headers.
     * xxx: 3gpp-gba-tmpi product token not supported
     */
    httpObj_ptr->customHeadersCount = 3;
    httpObj_ptr->customHeaders[0] = "User-Agent: libcurl/7.18 d2tech 3gpp-gba-tmpi";
    httpObj_ptr->customHeaders[1] = 
            "X-EricssonLabs-APIKEY: OCf5heNqCFqGg6kfRbPlTh5MRbymRcmA9qW3D4gO";

    OSAL_snprintf(obj_ptr->authHeaderScratch, sizeof(obj_ptr->authHeaderScratch) - 1,
            "Authorization: Digest username=\"%s\", \
            realm=\"%s\", uri=\"%s\", response=\"\" ",
            obj_ptr->impi, obj_ptr->bsfHost, obj_ptr->bsfPath);
    httpObj_ptr->customHeaders[2] = obj_ptr->authHeaderScratch;

    /* auth */
    httpObj_ptr->username_ptr = NULL;
    httpObj_ptr->password_ptr = NULL;

    httpObj_ptr->authHandler = _GBA_authHandler;
    httpObj_ptr->authProtocol = HTTP_AUTH_ANY;

    httpObj_ptr->bufs.body_ptr = OSAL_memAlloc(4*1024, OSAL_MEM_ARG_DYNAMIC_ALLOC);
    httpObj_ptr->bufs.bodyBufSz = 4*1024;
    httpObj_ptr->bufs.bodyBufIndex = 0;

    OSAL_netAddrCpy(&httpObj_ptr->infcAddress, &obj_ptr->infcAddress);

    /*
     * This is where replies from server will be stored.
     */
    httpObj_ptr->arg_ptr = obj_ptr;
    
    if (OSAL_FAIL == HTTP_setup(httpObj_ptr)) {
        return;
    }
}

/*
 * ======== GBA_processIpChange() ========
 *
 * Handel new or lost IP addres for the GBA session
 *
 * Returns:
 *   GBA_OK or GBA_ERR.
 *
 */
vint GBA_processIpChange(
    OSAL_NetAddress *ipAddr_ptr)
{

    /* 
     * GBA session could be valid until time out, even IP changed
     * if we want agressive policy, we could clean GBA session whenever ip changed
     * _GBA_globalObj.gbaObj.bootstrapped = OSAL_FALSE; 
     */

    OSAL_netAddrCpy(&_GBA_globalObj.gbaObj.infcAddress, ipAddr_ptr);
    return (GBA_OK);
}

