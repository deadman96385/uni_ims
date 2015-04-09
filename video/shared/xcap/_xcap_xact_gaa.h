/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev$ $Date$
 *
 */

#ifndef __XCAP_XACT_GAA_H_
#define __XCAP_XACT_GAA_H_

#include "osal.h"
#include <http.h>
#include "gba.h"
#include "gaa.h"

#define _XCAP_XACT_MAX_BODY_SZ       (0x1F4000)
#define _XCAP_XACT_MAX_HEADER_SZ     1024

#define XCAP_APP_NAME "xcap"

/*
 * Transaction object.
 */
typedef struct {
    GAA_NafSession *nafSession_ptr; /* gaa session for gba/gaa security */
    OSAL_Boolean    isSessionStarted;
    char           *uri_ptr;     /* URI to connect to */
    char           *x3gpp_ptr;   /* The value to use for the X-3GPP-Intended-Identity*/
    char           *userAgent_ptr; /* The value to use for the useragent header field */
    char           *src_ptr;     /* Poplulate with string to send (if) */
    int             srcSz;       /* size of above string */
    char           *hdrs_ptr[4]; /* For extra headers */
    char           *ifnm_ptr;    /* Set to If-(None)-Match for such inclusion */
    char           *etag_ptr;    /* Set to ETag etc. with ifnm_ptr != NULL */
    int             timeoutsec;  /* timeout in seconds */
    OSAL_NetAddress *infcAddr_ptr; /* the supsrv radio to do the xcap/gaa transactin */
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
} _XCAP_XactGaaObj;

/*
 * Function prototypes.
 */

int _XCAP_xactGaaCreateOrReplaceElement(
    _XCAP_XactGaaObj *xactGaa_ptr);

int _XCAP_xactGaaCreateOrReplaceAttribute(
    _XCAP_XactGaaObj *xactGaa_ptr);

int _XCAP_xactGaaCreateOrReplaceDocument(
    _XCAP_XactGaaObj *xactGaa_ptr,
    char         *auid_ptr);

int _XCAP_xactGaaDelete(
    _XCAP_XactGaaObj *xactGaa_ptr);

int _XCAP_xactGaaFetch(
    _XCAP_XactGaaObj *xactGaa_ptr);

int _XCAP_xactGaaInit(
    _XCAP_XactGaaObj *xactGaa_ptr,
    int           timeoutsec);

int _XCAP_xactGaaShutdown(
    _XCAP_XactGaaObj *xactGaa_ptr);

#endif
