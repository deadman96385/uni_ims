/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 7102 $ $Date: 2008-07-23 16:03:18 -0500 (Wed, 23 Jul 2008) $
 *
 */

/*
 * To generate C parser simply do: 
 * lex http.l
 * This will generate C outfile as named below.
 */

%option noyywrap
%option case-insensitive
%option nounput
%option noinput
%option nounistd
%option outfile="xcap_http_parse.c"

%{

/*
 * These are for C
 */

#include "xcap_http_parse.h"

/*
 * Parser states
 */
typedef enum {
    HTTP_NONE = 0,
    HTTP,
    CONTENT_LEN,
    CONTENT_TYPE,
    ETAG
} STATE;

static STATE state;
static XCAP_HttpParseHdrObj *localObj_ptr = NULL;

%}

%%

[\t ] ;

\n { state = HTTP_NONE; }

^HTTP\/1.1 { state = HTTP; }

^Content-Length: { state = CONTENT_LEN; }

^Content-Type: { state = CONTENT_TYPE; }

^ETag: { state = ETAG; }

[0-9]* { 
    if (HTTP == state) {
        localObj_ptr->finalCode = atoi(yytext);
    }
    else if (CONTENT_LEN == state) {
        localObj_ptr->contentLength = atoi(yytext); 
    } 
    else {
        state = HTTP_NONE;
    }
}

[a-zA-Z0-9+/-]* {
    if (CONTENT_TYPE == state) {
        strncpy(localObj_ptr->contentType, yytext,
                sizeof(localObj_ptr->contentType) - 1); 
    }
    else {
        state = HTTP_NONE; 
    }
}

\"[a-fA-F0-9]*\" {
    if (ETAG == state) {
        int len = strlen(yytext);
        strncpy(localObj_ptr->etag, yytext + 1,
                sizeof(localObj_ptr->etag) - 1);
        len = strlen(localObj_ptr->etag);
        localObj_ptr->etag[len] = 0;
    }
    else {
        state = HTTP_NONE;
    }
}

. { state = HTTP_NONE; }


%%

/* 
 * ======== XCAP_httpParseHeader () ========
 */
int XCAP_httpParseHeader(
    char                 *buf_ptr,
    int                   bufSz,
    XCAP_HttpParseHdrObj *obj_ptr)
{
    if ((NULL == buf_ptr) || (bufSz < 0) || (NULL == obj_ptr)) {
        return (0);
    }

    /*
     * Init for parsing.
     */
    localObj_ptr = obj_ptr;
    obj_ptr->contentLength = -1;
    obj_ptr->finalCode = -1;
    obj_ptr->contentType[0] = 0;
    obj_ptr->etag[0] = 0;
    state = HTTP_NONE;
    
    /*
     * parse
     */
    yy_scan_buffer(buf_ptr, bufSz);
    yylex();

    return (1);
}
