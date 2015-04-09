/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev$ $Date$
 *
 */

#include <stdio.h>

#if !(defined(OSAL_VXWORKS) || defined(OSAL_THREADX))
#include <sys/stat.h>
#endif

#include <osal.h>
#include "http.h"

#define D2_VPORT_REVISION D2_Release_HTTP_TEST
extern char const D2_Release_HTTP_TEST[];

HTTP_Obj      httpObj;

typedef struct {
    char         *uri_ptr;     /* URI to connect to */
    char         *x3gpp_ptr;   /* The value to use for the X-3GPP-Intended-Identity */
    char          username[32];  /* The value to use for the username when authenticating */
    char          password[32];  /* The value to use for the password when authenticating */
    char          auts[16];
    HTTP_AuthProtocol authProtocol;
    char          fileNameBuffer[256]; /* read file for put testing */
    char         *src_ptr;     /* Poplulate with string to send (if) */
    int           srcSz;       /* size of above string */
    char         *hdrs_ptr[4]; /* For extra headers */
    char         *ifnm_ptr;    /* Set to If-(None)-Match for such inclusion */
    char         *etag_ptr;    /* Set to ETag etc. with ifnm_ptr != NULL */
    int           timeoutsec;  /* timeout in seconds */
    char          scratch[128 + 1];
    char          scratch2[128 + 1];
} MyAppObj;

MyAppObj myAppObj;

#define _MY_MAX_BODY_SZ       (4096)
#define _MY_MAX_HEADER_SZ     1024

OSAL_Boolean myAuthHandler(
    HTTP_Obj *httpObj_ptr,
    char     *authHeader,
    int      size)
{
    /* auth */
    OSAL_logMsg("got authenticate header:\n%s\n",authHeader);
    httpObj_ptr->authProtocol = HTTP_AUTH_DIGEST;
    httpObj_ptr->username_ptr = &myAppObj.username[0];
    httpObj_ptr->password_ptr = &myAppObj.password[0];

    if ('\0' != myAppObj.auts[0]) {
        HTTP_applyAuthInfo(&httpObj, myAppObj.auts);
        return OSAL_FALSE;
    }
    else {
        return OSAL_TRUE;
    }
}

static int myAppInit(MyAppObj *myAppObj_ptr)
{
    myAppObj_ptr->uri_ptr = OSAL_memAlloc(_MY_MAX_HEADER_SZ, 0);
    /* setup the user info */
    myAppObj_ptr->username[0] = '\0';
    myAppObj_ptr->password[0] = '\0';
    myAppObj_ptr->auts[0] = '\0';
    myAppObj.authProtocol = HTTP_AUTH_ANY;
    myAppObj_ptr->x3gpp_ptr = "http_test";
    // myAppObj_ptr->etag_ptr = "12345678";
    // myAppObj_ptr->ifnm_ptr = XCAP_IF_NONE_MATCH;
    /*
     * A doc location (can be NULL for get and delete ops)
     */
    myAppObj_ptr->src_ptr = NULL;
    myAppObj_ptr->srcSz = 0;
    
    return 0;
}

int myFetch(
    MyAppObj *myAppObj_ptr,
    HTTP_Obj *myHttpObj_ptr)
{
    OSAL_Status  status;

    if (NULL == myAppObj_ptr) {
        return (0);
    }
    if (NULL == myAppObj_ptr->uri_ptr) {
        return (0);
    }

    status = HTTP_get(myHttpObj_ptr, myAppObj_ptr->uri_ptr);
    if (OSAL_FAIL == status) {
        OSAL_logMsg("http get failed");
        return (0);
    }
    OSAL_logMsg("got header status:\n%s\n",myHttpObj_ptr->lastHttpStatusLine);
    OSAL_logMsg("body size/index:%d/%d\n", 
                myHttpObj_ptr->bufs.bodyBufSz, 
                myHttpObj_ptr->bufs.bodyBufIndex);
    OSAL_logMsg("got body:\n%s\n",myHttpObj_ptr->bufs.body_ptr);
    
    // if HTTP/1.1 401 Unauthorized, try again
    if (OSAL_strncmp(&myHttpObj_ptr->lastHttpStatusLine[9], "401 ", 4) == 0) {
        status = HTTP_get(myHttpObj_ptr, myAppObj_ptr->uri_ptr);
        OSAL_logMsg("after auth, got header status:\n%s\n",myHttpObj_ptr->lastHttpStatusLine);
        OSAL_logMsg("body size/index:%d/%d\n", 
                myHttpObj_ptr->bufs.bodyBufSz, 
                myHttpObj_ptr->bufs.bodyBufIndex);
        OSAL_logMsg("got body:\n%s\n",myHttpObj_ptr->bufs.body_ptr);
    }
    
    OSAL_memFree(myHttpObj_ptr->bufs.body_ptr, OSAL_MEM_ARG_DYNAMIC_ALLOC);
    return (1);
}

int myPut(
    MyAppObj *myAppObj_ptr,
    HTTP_Obj *myHttpObj_ptr)
{
    OSAL_Status  status;
    size_t fsize,rsize;
    void *fbuf = NULL;
#if !(defined(OSAL_VXWORKS) || defined(OSAL_THREADX))
    FILE *file_src;
    struct stat file_info;
#endif

    if (NULL == myAppObj_ptr) {
        return (0);
    }
    if (NULL == myAppObj_ptr->uri_ptr) {
        return (0);
    }

#if defined(OSAL_VXWORKS) || defined(OSAL_THREADX)

    /* no file io, do only buffer preparation */
    /* XXX */
#else
    /* get the file size of the local file */
    if(stat(myAppObj_ptr->fileNameBuffer, &file_info)) {
        OSAL_logMsg("Couldnt open '%s': %s\n", myAppObj_ptr->fileNameBuffer, strerror(errno));
        return 1;
    }
    fsize = file_info.st_size;
    OSAL_logMsg("Local file size: %d bytes.\n", fsize);

    /* get a FILE * of the same file */
    file_src = fopen(myAppObj_ptr->fileNameBuffer, "rb");
    fbuf = malloc(fsize);
    rsize = fread(fbuf, 1, fsize, file_src);
#endif

    status = HTTP_put(myHttpObj_ptr, fbuf, rsize, myAppObj_ptr->uri_ptr);
    if (OSAL_FAIL == status) {
        OSAL_logMsg("http get failed");
        return (0);
    }
    OSAL_logMsg("last header status:\n%s\n",myHttpObj_ptr->lastHttpStatusLine);
    // if HTTP/1.1 401 Unauthorized, try again
    if (OSAL_strncmp(&myHttpObj_ptr->lastHttpStatusLine[9], "401 ", 4) == 0) {
        status = HTTP_put(myHttpObj_ptr, fbuf, rsize, myAppObj_ptr->uri_ptr);
        OSAL_logMsg("after re-auth, last header status:\n%s\n",myHttpObj_ptr->lastHttpStatusLine);
    }
    if (fbuf) {
        free(fbuf);
    }
    return (1);
}

void http_init(HTTP_Obj *myHttpObj_ptr)
{
    myHttpObj_ptr->followLocation = OSAL_TRUE;
    myHttpObj_ptr->timeoutsec = 0;
    OSAL_strncpy(myHttpObj_ptr->certificate,
            "/etc/ssl/certs/ca-certificates.crt",
            sizeof(myHttpObj_ptr->certificate));

    /*
     * Extra headers.
     */
    myHttpObj_ptr->customHeadersCount = 1;
    myHttpObj_ptr->customHeaders[0] = "X-3GPP-Intended-Identity: 3gpp-test";

    /* auth */
    myHttpObj_ptr->username_ptr = 0;
    myHttpObj_ptr->password_ptr = 0;
    
//     myHttpObj_ptr->username_ptr = &myAppObj.username[0];
//     myHttpObj_ptr->password_ptr = &myAppObj.password[0];
    
    myHttpObj_ptr->authHandler = myAuthHandler;
    myHttpObj_ptr->authProtocol = HTTP_AUTH_DIGEST;

    /*
     * This is where replies from server will be stored.
     */
    myHttpObj_ptr->arg_ptr = &myAppObj;
    if (OSAL_FAIL == HTTP_setup(&httpObj)) {
        return;
    }
}

void showUsage(void)
{
  OSAL_logMsg("Testing HTTP wrapper library\n");
  OSAL_logMsg("Usage   : http_test [Option] URL\n");
  OSAL_logMsg("Options :\n");
  OSAL_logMsg(" --put=filename        put file to the URL.\n");
  OSAL_logMsg(" --digest              force use digest authentication.\n");
  OSAL_logMsg(" --user=username       username for digest auth.\n");
  OSAL_logMsg(" --pass=password       password for digest auth.\n");
  OSAL_logMsg(" --auts=autsbinary     auts 14 bytes.\n");
  OSAL_logMsg(" --localip=xx.xx.xx.xx src ip to bind to\n");
  OSAL_logMsg(" --help                print this help.\n");
  OSAL_logMsg("\n");
  return;
}

#if defined(OSAL_VXWORKS) || defined(OSAL_THREADX)
int http_testMain(int argc, char *argv_ptr[])
#else
OSAL_ENTRY
#endif
{
    int     OptionIndex;
    OSAL_Boolean  isPutReq = OSAL_FALSE;

    myAppInit(&myAppObj);
    
    OptionIndex = 0;
    if (argc > 1) {
    
        // parsing options
        while (OptionIndex < (argc-1)) {
            if (strncmp(argv_ptr[OptionIndex], "--put=", 6) == 0) {
                OSAL_snprintf(myAppObj.fileNameBuffer, 256, "%s", &argv_ptr[OptionIndex][6]);
                isPutReq = OSAL_TRUE;
            }
            if (strncmp(argv_ptr[OptionIndex], "--user=", 7) == 0) {
                OSAL_snprintf(myAppObj.username, 32, "%s", &argv_ptr[OptionIndex][7]);
            }
            if (strncmp(argv_ptr[OptionIndex], "--pass=", 7) == 0) {
                OSAL_snprintf(myAppObj.password, 32, "%s", &argv_ptr[OptionIndex][7]);
            }
            if (strncmp(argv_ptr[OptionIndex], "--auts=", 7) == 0) {
                OSAL_snprintf(myAppObj.auts, 32, "%s", &argv_ptr[OptionIndex][7]);
            }
            if (strcmp(argv_ptr[OptionIndex], "--digest") == 0) {
                myAppObj.authProtocol = HTTP_AUTH_DIGEST;
            }
            if ((strcmp(argv_ptr[OptionIndex], "--help") == 0) ||
                    (strcmp(argv_ptr[OptionIndex], "/?") == 0)) {
                showUsage();
                return 0;
            }
            if (strncmp(argv_ptr[OptionIndex], "--localip=", 10) == 0) {
                OSAL_netStringToAddress(&argv_ptr[OptionIndex][10], &httpObj.infcAddress);
            }
            OptionIndex++;
        }
        
        // parsing url
        // HTTP_setURL(&httpObj, argv_ptr[argc-1]);
        OSAL_strcpy(myAppObj.uri_ptr, argv_ptr[argc-1]);
    }

    http_init(&httpObj);
    
    if ('\0' != myAppObj.auts[0]) {
        HTTP_applyAuthInfo(&httpObj, myAppObj.auts);
    }
    // do it
    if (isPutReq) {
        myPut(&myAppObj, &httpObj);
    } 
    else {
        myFetch(&myAppObj, &httpObj);
    }
    
    // done with it
    HTTP_cleanup(&httpObj);
    return 0;
}
#if (!defined(OSAL_VXWORKS)) && (!defined(OSAL_THREADX))
OSAL_EXIT
#endif
