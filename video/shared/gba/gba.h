/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev$ $Date$
 *
 */

#ifndef _GBA_H_
#define _GBA_H_

#include "osal.h"

#define GBA_FQDN_STRING_SZ          (255)
#define GBA_STRING_SZ               (128)
#define GBA_SMALL_STRING_SZ         (16)

/* Return codes. */
#define GBA_OK                     (0)
#define GBA_ERR                    (-1)

/* Definitions for AKA authentication */
#define GBA_AKA_AUTH_RESP_SZ      (16)
#define GBA_AKA_AUTH_AUTS_SZ      (14)
#define GBA_AKA_AUTH_CK_SZ        (16)
#define GBA_AKA_AUTH_IK_SZ        (16)

#define GBA_KS_NAF_LEN            (32)


typedef enum {
    GBA_NET_APP_AUTHTYPE_AUTO = 0, /* only auto detect the first 2 types */
    GBA_NET_APP_AUTHTYPE_DIGEST = 1,
    GBA_NET_APP_AUTHTYPE_GAA_DIGEST = 2,
    GBA_NET_APP_AUTHTYPE_GAA_DIGEST_UNKNOWN = 3
    /* future supported Ua security protocol identifiers to be added here */
}  GBA_AuthType;

typedef struct {
    char            appName[GBA_STRING_SZ];
    char            appUri[GBA_FQDN_STRING_SZ]; /* e.g. xcap root uri */
    GBA_AuthType    appAuthType;
    char            appAuthName[GBA_STRING_SZ];
    char            appAuthSecret[GBA_STRING_SZ];
} GBA_NetAppObj;

typedef struct {
    char btid[GBA_STRING_SZ]; /* Bootstrapping Transaction Identifier */
    char ksNaf[GBA_KS_NAF_LEN+1]; /* Ks_Naf for this app */
    vint expiryTime; /* expiry time of the base key */
} GBA_NafContext;

typedef enum {
    GBA_EVENT_NONE = 0,
    GBA_EVENT_AKA_CHALLENGE
} GBA_EventType;

typedef enum {
    GBA_COMMAND_NONE = 0,
    GBA_COMMAND_EXIT,
    GBA_COMMAND_AKA_RESPONSE_SUCCESS,
    GBA_COMMAND_AKA_RESPONSE_NETWORK_FAILURE,
    GBA_COMMAND_AKA_RESPONSE_SYNC_FAILURE,
} GBA_CommandType;

typedef struct {
    GBA_EventType type;
    union {
        struct {
            char   *rand_ptr;
            char   *autn_ptr;
        } akaChallenge;
    } u;
} GBA_Event;

typedef struct {
    GBA_CommandType type;
    union {
        struct {
            int    respLength;
            char   resp[GBA_AKA_AUTH_RESP_SZ];
            char   auts[GBA_AKA_AUTH_AUTS_SZ];
            char   ck[GBA_AKA_AUTH_CK_SZ];
            char   ik[GBA_AKA_AUTH_IK_SZ];
        } akaResponse;
    } u;
} GBA_Command;

/*
 * Function prototypes.
 */
vint GBA_init();

vint GBA_setup(
    char                *bsf_ptr,
    char                *impi);

vint GBA_registerNetApp(
    GBA_NetAppObj   *netAppObj_ptr);

vint GBA_unRegisterNetApp(
    char   *netAppName);


vint GBA_shutdown(
    void);

vint GBA_bootstrape(
    void);

GBA_NetAppObj *GBA_getNafProvision(
    char *netAppName);

GBA_NafContext *GBA_getNafContext(
    char *netAppName);

vint GBA_sendEvent(
        GBA_Event *evt_ptr);

vint GBA_getEvent(
     GBA_Event *evt_ptr,
     int       msTimeout);

vint GBA_sendCommand(
     GBA_Command *cmd_ptr);

vint GBA_getCommand(
     GBA_Command *cmd_ptr,
     int       msTimeout);

vint GBA_calcKsNaf(void *ks,
        void *rand,
        char *impi,
        char *naf_id,
        char *uaSecProtocol,
        char *result_ptr);

vint GBA_processIpChange(
    OSAL_NetAddress *ipAddr_ptr);

#endif
