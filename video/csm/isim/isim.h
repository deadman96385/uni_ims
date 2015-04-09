/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004-2010 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 28625 $ $Date: 2014-09-01 15:12:45 +0800 (Mon, 01 Sep 2014) $
 *
 */
#ifndef __ISIM_H__
#define __ISIM_H__

#include "csm_event.h"

#define ISIM_CSM_EVENT_TASK_NAME        ("isimCsmEventTask")
#define ISIM_CSM_EVENT_TASK_STACK_SZ    (8192)

#define ISIM_AUTH_AKA_OP_SZ             (16)
#define ISIM_AUTH_AKA_AMF_SZ            (2)
#define ISIM_AUTH_AKA_KI_SZ             (16)
#define ISIM_AUTH_AKA_OPC_SZ            (16)
#define ISIM_AUTH_AKA_RESP_SZ           (1024)
#define ISIM_AUTH_AKA_RANDLEN           (16)
#define ISIM_AUTH_AKA_AUTNLEN           (16)
#define ISIM_AUTH_AKA_SQNLEN            (6)
#define ISIM_AUTH_AKA_MACLEN            (8)
#define ISIM_AUTH_AKA_RESLEN            (8)
#define ISIM_AUTH_AKA_AKLEN             (6)
#define ISIM_AUTH_AKA_CKLEN             (16)
#define ISIM_AUTH_AKA_IKLEN             (16)

/* Return codes. */
#define ISIM_OK                     (0)
#define ISIM_ERR                    (-1)

/* xml configuration files folder path */
#ifndef ISIM_CONFIG_DEFAULT_PATH
#define ISIM_CONFIG_DEFAULT_PATH "//system//bin"
#endif

#ifndef ISIM_DEBUG
#define ISIM_dbgPrintf(fmt, args...)
#else
#define ISIM_dbgPrintf(fmt, args...) \
         OSAL_logMsg("[%s:%d] " fmt"\n", __FUNCTION__, __LINE__, ## args)
#endif

/*
 * This is the ISIM global object.
 */
typedef struct {
    OSAL_MsgQId csmIsimQ;    // Output from CSM
    OSAL_MsgQId csmEventInQ; // Input to CSM
    OSAL_TaskId csmEventTaskId;
    char        ki[ISIM_AUTH_AKA_KI_SZ*2 + 1];
    char        op[ISIM_AUTH_AKA_OP_SZ*2 + 1];
    char        opc[ISIM_AUTH_AKA_OPC_SZ*2 + 1];
    CSM_OutputEvent csmOutputEvent; /* to read CSM outputs into. */
    CSM_InputEvent  csmInputEvent; /* to construct CSM inputs to write to CSM */
} ISIM_GlobalObj;

/*
 * ISIM Public functions.
 */
vint ISIM_init(
    void);

vint ISIM_shutdown(
    void);

vint ISIM_allocate(
     void);

vint ISIM_start(
     void);

vint ISIM_destroy(
    void);

#endif
