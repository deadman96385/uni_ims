/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 29904 $ $Date: 2014-11-19 02:54:52 +0800 (Wed, 19 Nov 2014) $
 *
 */

#ifndef __XCAP_XACT_H_
#define __XCAP_XACT_H_

#include <http.h>

#define _XCAP_XACT_MAX_BODY_SZ       (0x1F4000)
#define _XCAP_XACT_MAX_HEADER_SZ     1024

/*
 * Transaction object.
 */
typedef struct {
    char         *uri_ptr;     /* URI to connect to */
    char         *x3gpp_ptr;   /* The value to use for the X-3GPP-Intended-Identity*/
    char         *username_ptr;  /* The value to use for the username when authenticating */
    char         *password_ptr;  /* The value to use for the password when authenticating */
    char         *userAgent_ptr; /* The value to use for the useragent header field */
    char         *src_ptr;     /* Poplulate with string to send (if) */
    int           srcSz;       /* size of above string */
    char         *hdrs_ptr[4]; /* For extra headers */
    char         *ifnm_ptr;    /* Set to If-(None)-Match for such inclusion */ 
    char         *etag_ptr;    /* Set to ETag etc. with ifnm_ptr != NULL */
    int           timeoutsec;  /* timeout in seconds */
    HTTP_Obj      http;        /* Underlying HTTP layer object */
    OSAL_NetAddress *infcAddr_ptr; /* the supsrv radio to do the xcap transactin */
    struct {
        char *hdr_ptr;
        int   hdrBufSz;
        int   hdrBufIndex;
        char *body_ptr;
        int   bodyBufSz;
        int   bodyBufIndex;
        int   failed;
    } reply;
    char  scratch[128 + 1];
    char  scratch2[128 + 1];
} _XCAP_XactObj;

/*
 * Function prototypes.
 */

int _XCAP_xactCreateOrReplaceElement(
    _XCAP_XactObj *xact_ptr);

int _XCAP_xactCreateOrReplaceAttribute(
    _XCAP_XactObj *xact_ptr);

int _XCAP_xactCreateOrReplaceDocument(
    _XCAP_XactObj *xact_ptr,
    char         *auid_ptr);

int _XCAP_xactDelete(
    _XCAP_XactObj *xact_ptr);

int _XCAP_xactFetch(
    _XCAP_XactObj *xact_ptr);

int _XCAP_xactInit(
    _XCAP_XactObj *xact_ptr,
    int           timeoutsec);

int _XCAP_xactShutdown(
    _XCAP_XactObj *xact_ptr);

#endif
