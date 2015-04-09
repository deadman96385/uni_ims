/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 29993 $ $Date: 2014-11-21 12:03:21 +0800 (Fri, 21 Nov 2014) $
 *
 */

#include <curl/curl.h>
#include <ctype.h>
#include <osal.h>
#include "http.h"

#ifdef HTTP_DBG_LOG
#define _HTTP_DBG    OSAL_logMsg
#else
#define _HTTP_DBG(x, ...)
#endif

#define _HTTP_AKA_AUTH_AUTS_SZ 14
/*
 * Follows HTTP private data. Dont read/write.
 */
typedef struct {
    CURL           *id_ptr;
    void           *rbuf_ptr;
    int             rbufSize;
    int             rindex;
} _HTTP_Priv;

/* 
 * ======== HTTP_alloc() ========
 * Public routine for allocating the module resource
 *
 * Returns: 
 *      OSAL_SUCCESS: function exits normally.
 *      OSAL_FAIL: in case of error
 */
 vint HTTP_alloc()
{
    return (OSAL_SUCCESS);
}

/* 
 * ======== HTTP_start() ========
 *
 * Public routine for starting the module tasks/actions
 *
 * Returns:
 *      OSAL_SUCCESS: function exits normally.
 *      OSAL_FAIL: in case of error
 */
vint HTTP_start()
{
    return (OSAL_SUCCESS);
}

/*
 * ======== HTTP_destroy() ========
 * Destroy the module
 *
 * Return Values:
 *      OSAL_SUCCESS: function exits normally.
 *      OSAL_FAIL: in case of error
 *
 */
vint HTTP_destroy(void)
{
    return (OSAL_SUCCESS);
}
/* 
 * ======== HTTP_applyAuthInfo() ========
 * 
 * This function setup the user/pass to the underlining http layer
 *
 * Returns: 
 *   OSAL_SUCCESS or OSAL_FAIL.
 */
OSAL_Status HTTP_applyAuthInfo(
    HTTP_Obj *obj_ptr,
    char     *akaAuts_ptr)
{
    _HTTP_Priv  *priv_ptr = (_HTTP_Priv *)obj_ptr->priv_ptr;
    CURL        *curl_ptr = priv_ptr->id_ptr;
    char        *userpwd;
    size_t       userpwdSz;
    int          curlAuth;
    char        *pass_ptr;
    char        *b64buf;
    size_t       autsb64Len;

    if (    NULL != obj_ptr->password_ptr && 
            0 != *(obj_ptr->password_ptr) &&
            NULL != obj_ptr->username_ptr && 
            0 != *(obj_ptr->username_ptr)) {
        userpwdSz = OSAL_strlen(obj_ptr->username_ptr) +
                OSAL_strlen(obj_ptr->password_ptr) + 4; /* 4 for ':' and end margin */
        userpwd = OSAL_memAlloc(userpwdSz, OSAL_MEM_ARG_DYNAMIC_ALLOC);
        OSAL_snprintf(userpwd, userpwdSz, "%s:%s",
                obj_ptr->username_ptr,
                obj_ptr->password_ptr);
        _HTTP_DBG("HTTP_applyAuthInfo %s\n",userpwd);
        if (CURLE_OK != curl_easy_setopt(curl_ptr, CURLOPT_USERPWD, userpwd)) {
            /* free our buffer */
            OSAL_memFree(userpwd, OSAL_MEM_ARG_DYNAMIC_ALLOC);
            return (OSAL_FAIL);
        }

        /* libcurl copy away the userpwd, we could free it now */
        OSAL_memFree(userpwd, OSAL_MEM_ARG_DYNAMIC_ALLOC);
    }
    else if ((NULL != akaAuts_ptr) &&
            (NULL != obj_ptr->username_ptr) &&
            (0 != *(obj_ptr->username_ptr))) {
        /*
         * only patched libcurl support GBA AKA extension
         * also expect empty password in this case, rfc 3310
         * our extended format, with the fact ',' not allowed in NAI/URI :
         * username,auts-b64:passwd
         * the patched libcurl will extract the auts-b64 by scanning ',' in username
         */
        b64buf = OSAL_memAlloc(_HTTP_AKA_AUTH_AUTS_SZ*2, OSAL_MEM_ARG_DYNAMIC_ALLOC);
        autsb64Len = OSAL_cryptoB64Encode(akaAuts_ptr, b64buf, _HTTP_AKA_AUTH_AUTS_SZ);
        /* hack the trailing new line. we dont' need that for auts attribute */
        if ('\n' == b64buf[autsb64Len-1]) {
            b64buf[--autsb64Len] = '\0';
        }
        _HTTP_DBG("autsb64Len %d akaAuts_ptr ::%s::\n",autsb64Len, b64buf);
        pass_ptr = obj_ptr->password_ptr;
        if (pass_ptr==NULL) pass_ptr = "";
        userpwdSz = OSAL_strlen(obj_ptr->username_ptr) +
                OSAL_strlen(pass_ptr) +
                autsb64Len + 4; /* 4 for ':' and ',' and end margin */
        userpwd = OSAL_memAlloc(userpwdSz, OSAL_MEM_ARG_DYNAMIC_ALLOC);
        OSAL_snprintf(userpwd, userpwdSz, "%s,%s:%s",
                obj_ptr->username_ptr,
                b64buf,
                pass_ptr);
        _HTTP_DBG("HTTP_applyAuthInfo with userpwd %s\n",userpwd);
        if (CURLE_OK != curl_easy_setopt(curl_ptr, CURLOPT_USERPWD, userpwd)) {
            /* free our buffer */
            OSAL_memFree(userpwd, OSAL_MEM_ARG_DYNAMIC_ALLOC);
            OSAL_memFree(b64buf, OSAL_MEM_ARG_DYNAMIC_ALLOC);
            return (OSAL_FAIL);
        }
        /* libcurl copy away the userpwd, we could free it now */
        OSAL_memFree(userpwd, OSAL_MEM_ARG_DYNAMIC_ALLOC);
        OSAL_memFree(b64buf, OSAL_MEM_ARG_DYNAMIC_ALLOC);
    }

    switch (obj_ptr->authProtocol) {
        case HTTP_AUTH_ANY:
            curlAuth = CURLAUTH_ANY;
            break;
        case HTTP_AUTH_DIGEST:
            curlAuth = CURLAUTH_DIGEST;
            break;
        case HTTP_AUTH_GBA_DIGEST:
            curlAuth = CURLAUTH_DIGEST;
            break;
        default:
            curlAuth = CURLAUTH_ANY;
            break;
    }
    if (CURLE_OK != curl_easy_setopt(curl_ptr,
            CURLOPT_HTTPAUTH,
            curlAuth)) {
        curl_easy_cleanup(curl_ptr);
        return (OSAL_FAIL);
    }
    return (OSAL_SUCCESS);
}

/* 
 * ======== _HTTP_headerf() ========
 * 
 * This function is a curl callback for reading a header data.
 * Calls HTTP application callback.
 *
 * Returns: 
 *   data octets read by application.
 */
static size_t _HTTP_headerf(
     void   *buf_ptr,
     size_t size,
     size_t elem,
     void   *data_ptr)
{
    HTTP_Obj *obj_ptr = (HTTP_Obj *)data_ptr;

    
    /* parse the last http status line */
    if (OSAL_strncmp((char *)(buf_ptr), "HTTP", 4) == 0) {
        int len = size*elem;
        len = (len>=HTTP_HEADER_LINE_LENGTH) ? (HTTP_HEADER_LINE_LENGTH-1) : len;
        OSAL_memCpy(obj_ptr->lastHttpStatusLine, 
                    (char *)(buf_ptr),
                    len);
        obj_ptr->lastHttpStatusLine[len]= '\0';
    }
    
    /* parse the authentication line */
    if (OSAL_strncmp((char *)(buf_ptr), "WWW-Authenticate", 16) == 0) {
        if (obj_ptr->authHandler != NULL) {
            OSAL_Boolean updateUserPass;
            updateUserPass = (obj_ptr->authHandler)(obj_ptr, 
                                                    buf_ptr,
                                                    size*elem);
            if (updateUserPass) {
                HTTP_applyAuthInfo(obj_ptr, NULL);
            }
        }
    }
    
    /* parse Content-Type */
    if (OSAL_strncmp((char *)(buf_ptr), "Content-Type", 12) == 0) {
        int len = size*elem;
        len = (len>=HTTP_HEADER_LINE_LENGTH) ? (HTTP_HEADER_LINE_LENGTH-1) : len;
        OSAL_memCpy(obj_ptr->lastContentType, 
                    (char *)(buf_ptr),
                    len);
        obj_ptr->lastContentType[len]= '\0';
        /* app could use HTTP_copyHeaderValue to extract the value */
    }
    
    // TBD: other customized header parsing needs?

    return (size*elem);
}

/* 
 * ======== _HTTP_writef() ========
 * 
 * This function is a curl callback for reading body data.
 * Calls HTTP application callback.
 *
 * Returns: 
 *   data octets read by application.
 */
static size_t _HTTP_writef(
     char   *buf_ptr,
     size_t size,
     size_t elem,
     void   *data_ptr)
{
    HTTP_Obj *httpObj_ptr = (HTTP_Obj *)data_ptr;
    size_t realsize = size * elem;
    void *newbuf_ptr;

    if ( (httpObj_ptr->bufs.bodyBufSz - httpObj_ptr->bufs.bodyBufIndex) <=
            (int)realsize ) {
        newbuf_ptr = realloc(httpObj_ptr->bufs.body_ptr,
                                httpObj_ptr->bufs.bodyBufSz + realsize + 1);
        if(newbuf_ptr == NULL) {
            /* out of memory! */
            OSAL_logMsg("http: not enough memory (realloc returned NULL)\n");
            httpObj_ptr->bufs.failed = 1;
            free(httpObj_ptr->bufs.body_ptr);
            httpObj_ptr->bufs.body_ptr = NULL;
            httpObj_ptr->bufs.bodyBufSz = 0;
            return 0;
        }

        httpObj_ptr->bufs.body_ptr = newbuf_ptr;
        httpObj_ptr->bufs.bodyBufSz = httpObj_ptr->bufs.bodyBufSz + realsize + 1;
        _HTTP_DBG("writef realloc: %d\n", httpObj_ptr->bufs.bodyBufSz);
    }
    OSAL_memCpy(&(httpObj_ptr->bufs.body_ptr[httpObj_ptr->bufs.bodyBufIndex]),
                buf_ptr, 
                realsize);
    httpObj_ptr->bufs.bodyBufIndex += realsize;
    httpObj_ptr->bufs.body_ptr[httpObj_ptr->bufs.bodyBufIndex] = 0;

    _HTTP_DBG("writef http body realsize: %d\n", realsize);
    return realsize;
}

/* 
 * ======== _HTTP_readf() ========
 * 
 * This function is a curl callback for writing body data.
 *
 * Returns: 
 *   n = number of octets written
 *   0 = done.
 */
static size_t _HTTP_readf(
     void *buf_ptr,
     size_t   size,
     size_t   elem,
     void *data_ptr)
{
    HTTP_Obj      *obj_ptr = (HTTP_Obj *)data_ptr;
    _HTTP_Priv    *priv_ptr = (_HTTP_Priv *)obj_ptr->priv_ptr;
    curl_off_t     nread;
    
    register int  *rindex_ptr = &priv_ptr->rindex;
    register int   bufsz = priv_ptr->rbufSize;
    register char *dat_ptr = (char *)priv_ptr->rbuf_ptr;

    nread = bufsz - (*rindex_ptr);
    if (nread > (int)(size * elem)) {
        nread = size*elem;
    }
    if (0 != nread) {
        OSAL_memCpy(buf_ptr, &dat_ptr[*rindex_ptr], nread);
        *rindex_ptr += nread;
    }

    _HTTP_DBG("readf http body nread: %d\n", (int)nread);
    return (nread);
}

/* 
 * ======== _HTTP_controlf() ========
 * 
 * This function is a curl callback for control of writing body data.
 *
 * Returns: 
 *   0 on success.
 */
static curlioerr _HTTP_controlf(
     CURL *curl_ptr,
     int   cmd,
     void *data_ptr)
{
    HTTP_Obj   *obj_ptr = (HTTP_Obj *)data_ptr;
    _HTTP_Priv *priv_ptr = (_HTTP_Priv *)obj_ptr->priv_ptr;
    
    if (CURLIOCMD_RESTARTREAD == cmd) {
        priv_ptr->rindex = 0;
    }
    return (0);
}

/* 
 * ======== _HTTP_prepareCurlHeaders() ========
 * 
 * This function setup the custome headers to the underlining http layer
 *
 * Returns: 
 *   OSAL_SUCCESS or OSAL_FAIL.
 */
struct curl_slist * _HTTP_prepareCurlHeaders(
    HTTP_Obj *obj_ptr)
{
    _HTTP_Priv          *priv_ptr = (_HTTP_Priv *)obj_ptr->priv_ptr;
    CURL                *curl_ptr = priv_ptr->id_ptr;
    struct curl_slist   *headers_ptr = NULL;
    
    if (0 != obj_ptr->customHeadersCount) {
        int i = 0;
        while (i<obj_ptr->customHeadersCount) {
            headers_ptr = curl_slist_append(headers_ptr,
                    obj_ptr->customHeaders[i]);
            i++;
        }
        _HTTP_DBG("custom headers: n,ptr %d,%x\n",
                obj_ptr->customHeadersCount,
                (unsigned int)headers_ptr);
        /* pass list of custom made headers */
        if (CURLE_OK != curl_easy_setopt(curl_ptr,
                CURLOPT_HTTPHEADER,
                headers_ptr)) {
            curl_slist_free_all(headers_ptr);
            return (NULL);
        }
    }
    return headers_ptr;
}

/* 
 * ======== _HTTP_discardCurlHeaders() ========
 * 
 * This function free up the customer headers for the underlining http layer
 *
 * Returns: 
 *   OSAL_SUCCESS or OSAL_FAIL.
 */
static void _HTTP_discardCurlHeaders(
    HTTP_Obj *obj_ptr,
    struct curl_slist *headers_ptr)
{
    _HTTP_Priv *priv_ptr = (_HTTP_Priv *)obj_ptr->priv_ptr;
    CURL       *curl_ptr = priv_ptr->id_ptr;
    if (NULL != headers_ptr) {
        /* clear old one */
        curl_easy_setopt(curl_ptr,
                CURLOPT_HTTPHEADER,
                NULL);
        curl_slist_free_all(headers_ptr);
    }
}   

/* 
 * ======== HTTP_freshBuffer() ========
 * 
 * This function is called to refresh http buffer for next transaction
 * Must be called before next new http request.
 *
 * Returns: 
 *   OSAL_SUCCESS or OSAL_FAIL.
 */
 void HTTP_freshBuffer(
    HTTP_Obj *obj_ptr)
{
    obj_ptr->bufs.bodyBufIndex = 0;
    if (NULL != obj_ptr->bufs.body_ptr) {
        obj_ptr->bufs.body_ptr[0] = 0;
    }
    obj_ptr->lastHttpStatusLine[0] = '\0';
    obj_ptr->lastContentType[0] = '\0';
}

/* 
 * ======== HTTP_setup() ========
 * 
 * This function is called to setup http connection parameters.
 * Must be called before any http request.
 *
 * Returns: 
 *   OSAL_SUCCESS or OSAL_FAIL.
 */
 OSAL_Status HTTP_setup(
    HTTP_Obj *obj_ptr)
{
    _HTTP_Priv *priv_ptr;
    
     /*
     * get CURL handle.
     */
    CURL *curl_ptr = curl_easy_init();
    if (NULL == curl_ptr) {
        return (OSAL_FAIL);
    }
    priv_ptr = (_HTTP_Priv *)OSAL_memAlloc(sizeof(_HTTP_Priv),
            OSAL_MEM_ARG_DYNAMIC_ALLOC); 
    priv_ptr->id_ptr = curl_ptr;
    obj_ptr->priv_ptr = (void *)priv_ptr;
    
    /*
     * Set options.
     */

    /*
     * Verify authenticity of remote peer certificate?
     * Verify if the host name matches host in the certificate?
     */
    if (0 != obj_ptr->certificate[0]) {
        if (CURLE_OK != curl_easy_setopt(curl_ptr,
                CURLOPT_SSL_VERIFYPEER, 
                1)) {
            curl_easy_cleanup(curl_ptr);
            return (OSAL_FAIL);
        }
        if (CURLE_OK != curl_easy_setopt(curl_ptr,
                CURLOPT_SSL_VERIFYHOST, 
                1)) {
            curl_easy_cleanup(curl_ptr);
            return (OSAL_FAIL);
        }
        if (CURLE_OK != curl_easy_setopt(curl_ptr,
                CURLOPT_CAPATH,
                obj_ptr->certificate)) {
            curl_easy_cleanup(curl_ptr);
            return (OSAL_FAIL);
        }
    }
    else {
        if (CURLE_OK != curl_easy_setopt(curl_ptr,
                CURLOPT_SSL_VERIFYPEER, 
                0)) {
            curl_easy_cleanup(curl_ptr);
            return (OSAL_FAIL);
        }
        if (CURLE_OK != curl_easy_setopt(curl_ptr,
                CURLOPT_SSL_VERIFYHOST, 
                0)) {
            curl_easy_cleanup(curl_ptr);
            return (OSAL_FAIL);
        }
    }

    /* 
     * app provided buffer or we allocated for them
     * but app is always responsible to OSAL_free() it after use
     * because we don't know when it is done
     */
    if (obj_ptr->bufs.body_ptr == NULL) {
        obj_ptr->bufs.body_ptr = OSAL_memAlloc(1, OSAL_MEM_ARG_DYNAMIC_ALLOC);
        obj_ptr->bufs.bodyBufSz = 0;
        obj_ptr->bufs.bodyBufIndex = 0;
    }
    
    /*
     * Authentication setup
     */

    HTTP_applyAuthInfo(obj_ptr, NULL);
    
    curl_easy_setopt(curl_ptr, CURLOPT_COOKIEFILE, ""); /* just to start the cookie engine */

    /* Set user agent */
    if (CURLE_OK != curl_easy_setopt(curl_ptr,
            CURLOPT_USERAGENT,
            "libcurl/7.18")) {
        return (OSAL_FAIL);
    }

    /*
     * If redirected, follow location and auth if required again?
     */
    if (OSAL_TRUE == obj_ptr->followLocation) {
        if (CURLE_OK != curl_easy_setopt(curl_ptr,
                CURLOPT_FOLLOWLOCATION, 
                1)) {
            curl_easy_cleanup(curl_ptr);
            return (OSAL_FAIL);
        }
        if (CURLE_OK != curl_easy_setopt(curl_ptr,
                CURLOPT_UNRESTRICTED_AUTH, 
                1)) {
            curl_easy_cleanup(curl_ptr);
            return (OSAL_FAIL);
        }
    }
        
    /*
     * set callbacks.
     */
    if (CURLE_OK != curl_easy_setopt(curl_ptr,
            CURLOPT_WRITEFUNCTION,
            _HTTP_writef)) {
        return (OSAL_FAIL);
    }
    if (CURLE_OK != curl_easy_setopt(curl_ptr,
            CURLOPT_WRITEDATA,
            obj_ptr)) {
        return (OSAL_FAIL);
    }
    if (CURLE_OK != curl_easy_setopt(curl_ptr,
            CURLOPT_HEADERFUNCTION,
            _HTTP_headerf)) {
        return (OSAL_FAIL);
    }
    if (CURLE_OK != curl_easy_setopt(curl_ptr,
            CURLOPT_HEADERDATA,
            obj_ptr)) {
        return (OSAL_FAIL);
    }
    if (CURLE_OK != curl_easy_setopt(curl_ptr,
            CURLOPT_READFUNCTION,
            _HTTP_readf)) {
        return (OSAL_FAIL);
    }
    if (CURLE_OK != curl_easy_setopt(curl_ptr,
            CURLOPT_READDATA,
            obj_ptr)) {
        return (OSAL_FAIL);
    }
    if (CURLE_OK != curl_easy_setopt(curl_ptr,
            CURLOPT_IOCTLFUNCTION,
            _HTTP_controlf)) {
        return (OSAL_FAIL);
    }
    if (CURLE_OK != curl_easy_setopt(curl_ptr,
            CURLOPT_IOCTLDATA,
            obj_ptr)) {
        return (OSAL_FAIL);
    }
    /*
     * Timeout
     */
    if (obj_ptr->timeoutsec > 0) {
        if (CURLE_OK != curl_easy_setopt(curl_ptr,
                CURLOPT_TIMEOUT,
                obj_ptr->timeoutsec)) {
            return (OSAL_FAIL);
        }
    }
    /*
     * Debug
     */
#ifdef HTTP_DBG_LOG
    curl_easy_setopt(curl_ptr, CURLOPT_VERBOSE, 1);
#else
    curl_easy_setopt(curl_ptr, CURLOPT_VERBOSE, 0);
#endif

#if defined(PROVIDER_CMCC)
    {
        char hostAddrString[48];

        /* Convert to host byte order string */
        OSAL_netAddressToString(hostAddrString, &obj_ptr->infcAddress);
        curl_easy_setopt(curl_ptr, CURLOPT_INTERFACE, hostAddrString);
    }
#endif

    return (OSAL_SUCCESS);
}

/* 
 * ======== HTTP_delete() ========
 * 
 * Sends HTTP DELETE request.
 *
 * Returns: 
 *   OSAL_SUCCESS or OSAL_FAIL.
 */
OSAL_Status HTTP_delete(
    HTTP_Obj *obj_ptr,
    char     *url_ptr)
{
    _HTTP_Priv *priv_ptr = (_HTTP_Priv *)obj_ptr->priv_ptr;
    CURL       *curl_ptr = priv_ptr->id_ptr;
    CURLcode    ret;
    struct curl_slist *headers_ptr = NULL;
    int                i = 0;

    if (NULL == curl_ptr) {
        return (OSAL_FAIL);
    }
    
    /*
     * URL
     * Start URL with http:// or https://
     * For example 
     *  http://d2fs1.hq.d2tech.com
     *  https://d2fs1.hq.d2tech.com
     */
    if (CURLE_OK != curl_easy_setopt(curl_ptr,
            CURLOPT_URL,
            url_ptr)) {
        return (OSAL_FAIL);
    }
    
    /*
     * Set options
     */
    if (CURLE_OK != curl_easy_setopt(curl_ptr,
            CURLOPT_HTTPHEADER,
            NULL)) {
        return (OSAL_FAIL);
    }
    
    if (CURLE_OK != curl_easy_setopt(curl_ptr,
            CURLOPT_CUSTOMREQUEST,
            "DELETE")) {
        return (OSAL_FAIL);
    }

    headers_ptr = _HTTP_prepareCurlHeaders(obj_ptr);

    /*
     * Go
     */
    if (0 != (ret = curl_easy_perform(curl_ptr))) {
        if ((NULL != headers_ptr) && i) {
            curl_slist_free_all(headers_ptr);
        }
        return (OSAL_FAIL);
    }

    _HTTP_discardCurlHeaders(obj_ptr, headers_ptr);
    
    return (OSAL_TRUE);
}

/* 
 * ======== HTTP_get() ========
 * 
 * Sends HTTP GET request.
 *
 * Returns: 
 *   OSAL_SUCCESS or OSAL_FAIL.
 */
OSAL_Status HTTP_get(
    HTTP_Obj *obj_ptr,
    char     *url_ptr)
{
    _HTTP_Priv *priv_ptr = (_HTTP_Priv *)obj_ptr->priv_ptr;
    CURL       *curl_ptr = priv_ptr->id_ptr;
    CURLcode    ret;
    struct curl_slist *headers_ptr = NULL;
    int                i = 0;

    if (NULL == curl_ptr) {
        return (OSAL_FAIL);
    }
    
    /*
     * URL
     * Start URL with http:// or https://
     * For example 
     *  http://d2fs1.hq.d2tech.com
     *  https://d2fs1.hq.d2tech.com
     */
    if (CURLE_OK != curl_easy_setopt(curl_ptr,
            CURLOPT_URL,
            url_ptr)) {
        return (OSAL_FAIL);
    }
    
    /*
     * Set options
     */
    if (CURLE_OK != curl_easy_setopt(curl_ptr,
            CURLOPT_HTTPHEADER,
            NULL)) {
        return (OSAL_FAIL);
    }
    
    if (CURLE_OK != curl_easy_setopt(curl_ptr,
            CURLOPT_CUSTOMREQUEST,
            NULL)) {
        return (OSAL_FAIL);
    }

    if (CURLE_OK != curl_easy_setopt(curl_ptr,
            CURLOPT_HTTPGET,
            1)) {
        return (OSAL_FAIL);
    }

    headers_ptr = _HTTP_prepareCurlHeaders(obj_ptr);
   
    /*
     * Go
     */
    if (0 != (ret = curl_easy_perform(curl_ptr))) {
        if ((NULL != headers_ptr) && i) {
            curl_slist_free_all(headers_ptr);
        }
        return (OSAL_FAIL);
    }

    _HTTP_discardCurlHeaders(obj_ptr, headers_ptr);

    return (OSAL_TRUE);
}

/* 
 * ======== HTTP_put() ========
 * 
 * Sends HTTP PUT request.
 *
 * Returns: 
 *   OSAL_SUCCESS or OSAL_FAIL.
 */
OSAL_Status HTTP_put(
    HTTP_Obj *obj_ptr,
    void     *buf_ptr,
    int       bufSz,
    char     *url_ptr)
{
    _HTTP_Priv        *priv_ptr = obj_ptr->priv_ptr;
    CURL              *curl_ptr = priv_ptr->id_ptr;
    struct curl_slist *headers_ptr = NULL;
    CURLcode           ret;
    int                i = 0;

    if (NULL == curl_ptr) {
        return (OSAL_FAIL);
    }

    if (NULL == buf_ptr) {
        return (OSAL_FAIL);
    }
    /*
     * Set buffer and its size, will use in read function from curl.
     */
    priv_ptr->rbufSize = bufSz;
    priv_ptr->rbuf_ptr = buf_ptr;
    priv_ptr->rindex = 0;

    /*
     * URL
     * Start URL with http:// or https://
     * For example 
     *  http://d2fs1.hq.d2tech.com
     *  https://d2fs1.hq.d2tech.com
     */
    if (CURLE_OK != curl_easy_setopt(curl_ptr,
            CURLOPT_URL,
            url_ptr)) {
        return (OSAL_FAIL);
    }

    /*
     * Set options
     */
    if (CURLE_OK != curl_easy_setopt(curl_ptr,
            CURLOPT_HTTPHEADER,
            NULL)) {
        return (OSAL_FAIL);
    }
    
    if (CURLE_OK != curl_easy_setopt(curl_ptr,
            CURLOPT_CUSTOMREQUEST,
            NULL)) {
        return (OSAL_FAIL);
    }
    
    if (CURLE_OK != curl_easy_setopt(curl_ptr,
            CURLOPT_UPLOAD,
            1)) {
        return (OSAL_FAIL);
    }
    
    if (CURLE_OK != curl_easy_setopt(curl_ptr,
            CURLOPT_PUT,
            1)) {
        return (OSAL_FAIL);
    }
   
    if (CURLE_OK != curl_easy_setopt(curl_ptr,
            CURLOPT_INFILESIZE,
            bufSz)) {
        return (OSAL_FAIL);
    }

    headers_ptr = _HTTP_prepareCurlHeaders(obj_ptr);

    /*
     * Go
     */
    if (0 != (ret = curl_easy_perform(curl_ptr))) {
        if ((NULL != headers_ptr) && i) {
            curl_slist_free_all(headers_ptr);
        }
        return (OSAL_FAIL);
    }

    _HTTP_discardCurlHeaders(obj_ptr, headers_ptr);

    return (OSAL_TRUE);
}

/* 
 * ======== HTTP_cleanup() ========
 * 
 * Deallocates all resources allocated by HTTP_setup().
 *
 * Returns: 
 *   OSAL_SUCCESS or OSAL_FAIL.
 */
OSAL_Status HTTP_cleanup(
    HTTP_Obj *obj_ptr)
{
    _HTTP_Priv *priv_ptr = (_HTTP_Priv *)obj_ptr->priv_ptr;
    CURL       *curl_ptr = priv_ptr->id_ptr;

    if (NULL != curl_ptr) {
        curl_easy_cleanup(curl_ptr);
    }
    
    OSAL_memFree(priv_ptr, OSAL_MEM_ARG_DYNAMIC_ALLOC);
    
    return (OSAL_SUCCESS);
}

/*
 * ======== HTTP_copyHeaderValue() ========
 *
 * Strip off leading and trailing whitespace from the value in the
 * given HTTP header line and copy to the supplied buffer.
 * Set buffer to an empty string if the header value
 * consists entirely of whitespace.
 * assumption: destBuf_ptr is big enough to store the value string
 * code base is from libcurl
 *
 * Returns: 
 *   OSAL_SUCCESS or OSAL_FAIL.
 */
OSAL_Status  HTTP_copyHeaderValue(
    const char  *hl_ptr,
    char        *destBuf_ptr)
{
    const char *start;
    const char *end;
    size_t len;

    /* Find the end of the header name */
    while (*hl_ptr && (*hl_ptr != ':')) {
        ++hl_ptr;
    }
    if(*hl_ptr) {
        /* Skip over colon */
        ++hl_ptr;
    }

    /* Find the first non-space letter */
    start = hl_ptr;
    while (*start && isspace(*start)) {
        start++;
    }
    
    /* data is in the host encoding so
     use '\r' and '\n' instead of 0x0d and 0x0a */
    end = OSAL_strchr(start, '\r');
    if(!end) {
        end = OSAL_strchr(start, '\n');
    }
    if(!end) {
        end = OSAL_strchr(start, '\0');
    }
    if(!end) {
        return OSAL_FAIL;
    }

    /* skip all trailing space letters */
    while((end > start) && isspace(*end)) {
        end--;
    }
    
    /* get length of the type */
    len = end-start+1;

    OSAL_memCpy(destBuf_ptr, start, len);
    destBuf_ptr[len] = 0; /* zero terminate */

    return OSAL_SUCCESS;
}

