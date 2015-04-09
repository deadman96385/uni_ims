/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev$ $Date$
 *
 */

#ifndef __GBA_H_
#define __GBA_H_

#include "osal.h"
#include "gba.h"

#define GBA_AUTH_HEADER_STRING_SZ   (1024)


/* Queue and IPC definitions */
#define GBA_MSGQ_LEN                (8)
#define GBA_TASK_NAME               "GBA"
#define GBA_TASK_STACK_BYTES        (1000)
#define GBA_COMMAND_QUEUE_NAME      "gba.command.q"
#define GBA_EVENT_QUEUE_NAME        "gba.event.q"
#define GBA_NET_APP_MAX_COUNT       4

/* Definitions needed for Mutual Authenication (AKA version 1 & 2). */
#define GBA_AUTH_AKA_RANDLEN    (16)
#define GBA_AUTH_AKA_AUTNLEN    (16)
#define GBA_AUTH_AKA_SQNLEN     (6)
#define GBA_AUTH_AKA_MACLEN     (8)
#define GBA_AUTH_AKA_RESLEN     (8)
#define GBA_AUTH_AKA_AKLEN      (6)
#define GBA_AUTH_AKA_CKLEN      (16)
#define GBA_AUTH_AKA_IKLEN      (16)

/* Used for authorization */
#define GBA_USERNAME_ARG_STR_SIZE        (64)
#define GBA_REALM_ARG_STR_SIZE           (64)
#define GBA_PASSWORD_ARG_STR_SIZE        (64)
#define GBA_NONCE_ARG_STR_SIZE           (128)
#define GBA_CNONCE_ARG_STR_SIZE          (128)
#define GBA_NONCE_CNT_ARG_STR_SIZE       (12)
#define GBA_OPAQUE_ARG_STR_SIZE          (128)
#define GBA_RESPONSE_ARG_STR_SIZE        (128)
#define GBA_CNONCE_RAND_MIN_LEN          (8)
#define GBA_CNONCE_RAND_MAX_LEN          (15)
#define GBA_BOUNDRY_MAX_LEN              (32)

#define GBA_DIGEST_HF_ARG_STR           "Digest"
#define GBA_BASIC_HF_ARG_STR            "Basic"
#define GBA_DOMAIN_HF_ARG_STR           "domain"
#define GBA_USERNAME_HF_ARG_STR         "username"
#define GBA_REALM_HF_ARG_STR            "realm"
#define GBA_NONCE_HF_ARG_STR            "nonce"
#define GBA_QOP_HF_ARG_STR              "qop"
#define GBA_NC_HF_ARG_STR               "nc"
#define GBA_RESPONSE_HF_ARG_STR         "response"
#define GBA_ALGORITHM_HF_ARG_STR        "algorithm"
#define GBA_CNONCE_HF_ARG_STR           "cnonce"
#define GBA_OPAQUE_HF_ARG_STR           "opaque"
#define GBA_STALE_HF_ARG_STR            "stale"
#define GBA_URI_HF_ARG_STR              "uri"
#define GBA_AUTH_ALG_MD5_STR            "MD5"
#define GBA_AUTH_ALG_AKAV1_MD5_STR      "AKAv1-MD5"
#define GBA_AUTH_ALG_AKAV2_MD5_STR      "AKAv2-MD5"
#define GBA_QOP_AUTH_STR                "auth"
#define GBA_QOP_AUTH_INT_STR            "auth-int"

#define GBA_XMLNS_ATTR                  "xmlns"
#define GBA_XMLNS_URI                   "uri:3gpp-gba"
#define GBA_XML_BTID                    "btid"
#define GBA_XML_LIFETIME                "lifetime"


typedef enum
{
    GBA_AUTH_ALG_NONE,
    GBA_AUTH_ALG_MD5,
    GBA_AUTH_ALG_AKAV1_MD5,
    GBA_AUTH_ALG_AKAV2_MD5,
} _GBA_AuthAlg;

typedef enum
{
    GBA_AUTH_QOP_NONE,
    GBA_AUTH_QOP_AUTH,
    GBA_AUTH_QOP_AUTH_INT,
} _GBA_AuthQop;

typedef struct
{
    GBA_AuthType    type;
    char            szRealm[GBA_REALM_ARG_STR_SIZE];
    char            domain[GBA_STRING_SZ];
    char            nonce[GBA_NONCE_ARG_STR_SIZE];
    char            opaque[GBA_OPAQUE_ARG_STR_SIZE];
    char            username[GBA_USERNAME_ARG_STR_SIZE];
    char            uri[GBA_STRING_SZ];
    char            cnonce[GBA_CNONCE_ARG_STR_SIZE];
    char            nc[GBA_NONCE_CNT_ARG_STR_SIZE];
    char            response[GBA_RESPONSE_ARG_STR_SIZE];
    vint            stale;
    _GBA_AuthAlg    alg;
    _GBA_AuthQop    qop;
} _GBA_AuthParams;



typedef struct {
    OSAL_TaskId     tid;            /* OSAL task ID for GBA task */
    OSAL_MsgQId     cmdq;           /* Command queue ID */
    OSAL_MsgQId     evtq;           /* Event queue ID */
    OSAL_SemId      taskLock;       /* Semaphore ID */
    OSAL_Boolean    exit;           /* Condition for exiting the task */
    OSAL_Boolean    bootstrapped;   /* contacted bsf already? */
    HTTP_Obj        httpObj;        /* http lib obj doing the AKA auth */
    OSAL_NetAddress infcAddress;    /* radion infc for http access */
    char            bsfUrl[GBA_FQDN_STRING_SZ];
    char            bsfHost[GBA_FQDN_STRING_SZ];
    int             bsfPort;
    char            bsfPath[GBA_STRING_SZ];
    char            impi[GBA_STRING_SZ];
    char            btid[GBA_STRING_SZ];
    char            lifeTime[GBA_STRING_SZ];
    vint            netAppCount;
    GBA_NetAppObj   netAppInfos[GBA_NET_APP_MAX_COUNT];
    GBA_NafContext  netAppContext[GBA_NET_APP_MAX_COUNT];
    char            akaRand[GBA_AUTH_AKA_RANDLEN];
    char            akaAutn[GBA_AUTH_AKA_AUTNLEN];
    uint8           akaAuthResp[GBA_AKA_AUTH_RESP_SZ + 1];
    uint8           akaAuthCk[GBA_AKA_AUTH_CK_SZ + 1];
    uint8           akaAuthIk[GBA_AKA_AUTH_IK_SZ + 1];
    char            authHeaderScratch[GBA_AUTH_HEADER_STRING_SZ];
    GBA_Event       gbaEventScratch;
    _GBA_AuthParams authParams;
} GBA_Obj;

typedef struct {
    GBA_Obj gbaObj;
} GBA_GlobalObj;

/*
 * module private function prototypes.
 */
OSAL_TaskReturn _GBA_task(
    OSAL_TaskArg arg_ptr);

void _GBA_initHttp(
    HTTP_Obj *httpObj_ptr);

vint _GBA_urlToHostPortPath(
    const char *url,
    char *host,
    vint *port,
    char *path,
    vint *ssl);

vint _GBA_bootstrapeDone(
    GBA_Command *cmd_ptr);

void _GBA_hexdump(
    const void *buf,
    int len);

#endif
