/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev$ $Date$
 *
 */

#ifndef _GAA_H_
#define _GAA_H_

#include <osal.h>
#include "http.h"

#define GAA_FQDN_STRING_SZ         (255)
#define GAA_STRING_SZ              (128)

typedef struct {
    char            nafAppName[GAA_STRING_SZ];
    HTTP_Obj        httpObj;
    GBA_NafContext  *nafContext_ptr;
    GBA_NetAppObj   *netAppObj_ptr;
    GBA_AuthType    autoDetectedAuthType;
    OSAL_NetAddress *infcAddr_ptr; /* the radio to do the gaa transactin */
} GAA_NafSession;

/*
 * Function prototypes.
 */
GAA_NafSession *GAA_newNafSession(
    char *appName);

OSAL_Status GAA_freeNafSession(
    GAA_NafSession *nafSession_ptr);
    
OSAL_Status GAA_detectAuthType(
    GAA_NafSession *nafSession_ptr);

OSAL_Status GAA_setupHttpAuth(
    GAA_NafSession *nafSession_ptr);

OSAL_Status GAA_startNafSession(
    GAA_NafSession *nafSession_ptr);

OSAL_Status GAA_stopNafSession(
    GAA_NafSession *nafSession_ptr);

#endif
