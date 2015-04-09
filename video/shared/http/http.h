/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 29993 $ $Date: 2014-11-21 12:03:21 +0800 (Fri, 21 Nov 2014) $
 *
 */

#ifndef _HTTP_H_
#define _HTTP_H_

#include <osal.h>

/*
 * refactored http lib to use synchronous API style and allow only
 * one active http request per httpObj
 * entire entity body is collected or sent before the request return.
 * no need to write header or body call back anymore
 * no more header callback, so a last header status line is added in httpObj
 *
 * however, auth handler is added to allow GBA authentication
 * usage of the auth handler:
 * 1. without auth handler: app should setup the user/pass before hand.
 *      No dynamic detection and dynamic user/pass.
 * 2. with auth handler: app could leave the user/pass as nil first
 *      Once the auth handler is invoked, app could do dynamic user/pass fill up
 *      After the fill up, app will get last request result with http 401 error,
 *      App should redo the request again with the same http object.
 *      underlining protocol will remember the nonce/.../ to do the right auth.
 *
 * Memory Management Rules
 * The entity buffer is dynamically allocated. App should free it after consuming it.
 * The customer headers are just pointers to string. App handle them by itself.
 *
 * Internally, some xact actions/fields are moved in the http lib
 * Sample usage could be found in the test folder
 */

#define HTTP_HEADER_LINE_LENGTH 128
#define HTTP_MAX_CUSTOM_HEADERS 8

typedef enum {
    HTTP_AUTH_ANY = 0,
    HTTP_AUTH_DIGEST = 1,
    HTTP_AUTH_GBA_DIGEST = 2,
} HTTP_AuthProtocol;

struct HTTP_Obj_s;
typedef struct HTTP_Obj_s HTTP_Obj;
/*
 * handler function prototypes
 */
typedef OSAL_Boolean (*HTTP_AuthHandler) (
    HTTP_Obj *httpObj_ptr,
    char     *authHeader,
    int      size);

/*
 * Setting for a transaction.
 */
struct HTTP_Obj_s {
    /* SSL/TLS
     * Certificate path, set to "" to not verify certificate.
     * Set to certificates file location to verify certificate.
     * For example "/etc/ssl/certs/ca-certificates.crt"
     */
    char               certificate[256];
    /*
     * Set this to follow links and recursively auth while doing so.
     */
    OSAL_Boolean       followLocation;
    int                timeoutsec; /* Time out for every transaction */

    HTTP_AuthProtocol  authProtocol; /* specify the auth protocol this context to use */
    HTTP_AuthHandler   authHandler; /* setup user/pass according to the auth protocol */
    char              *username_ptr; /* digest auth username */
    char              *password_ptr; /* digest auth password */

    /* custom headers */
    char               lastHttpStatusLine[HTTP_HEADER_LINE_LENGTH];
    char               lastContentType[HTTP_HEADER_LINE_LENGTH];
    int                customHeadersCount;
    char              *customHeaders[HTTP_MAX_CUSTOM_HEADERS];

    struct {
        char *body_ptr;
        int   bodyBufSz;
        int   bodyBufIndex;
        int   failed;
    } bufs;
    void              *arg_ptr; /* per app argument for used in handler */
    void              *priv_ptr; /* per underlining http lib opaque data */
    OSAL_NetAddress   infcAddress; /* radio interface ip address for http access */
};


/*
 * Function prototypes.
 * HEAD, POST, OPTIONS, TRACE, CONNECT not implemented.
 */

void HTTP_freshBuffer(
    HTTP_Obj *obj_ptr);

OSAL_Status HTTP_setup(
    HTTP_Obj *obj_ptr);

OSAL_Status HTTP_delete(
    HTTP_Obj *obj_ptr,
    char     *url_ptr);

OSAL_Status HTTP_get(
    HTTP_Obj *obj_ptr,
    char     *url_ptr);

OSAL_Status HTTP_put(
    HTTP_Obj *obj_ptr,
    void     *buf_ptr,
    int       bufSz,
    char     *url_ptr);

OSAL_Status HTTP_cleanup(
    HTTP_Obj *obj_ptr);

OSAL_Status  HTTP_copyHeaderValue(
    const char *hl_ptr,
    char *destBuf_ptr);

OSAL_Status HTTP_applyAuthInfo(
    HTTP_Obj *obj_ptr,
    char     *akaAuts_ptr);

/* utilities for parsing common http headers */
OSAL_Status HTTP_getParameterValue(
    char         *headerLine_ptr,
    const char   *pName_ptr,
    char        **value_ptr,
    vint         *valueLen_ptr);

void HTTP_hexDump(
    const void *buf,
    int len);

OSAL_Status HTTP_urlToHostPortPath(
    const char *url,
    char *host,
    vint *port,
    char *path,
    vint *ssl);

OSAL_Status HTTP_getKeyValuePair(
    char  *str,
    char **keyStr_ptr,
    vint  *keyLen_ptr,
    char **valueStr_ptr,
    vint  *valueLen_ptr,
    char **endStr_ptr /* starting of next key pair */
    );

OSAL_Status HTTP_decodeKeyValuePair(
    char    *keyStr,
    vint    keyLen,
    char    *valueStr,
    vint    valueLen,
    char    *token,
    char    *buffer,
    vint    bufferLen
    );

vint HTTP_allocate(void);
vint HTTP_start(void);
vint HTTP_destroy(void);

#endif
