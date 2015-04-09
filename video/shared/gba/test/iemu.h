/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004-2010 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev$ $Date$
 *
 */

#ifndef _IEMU_H_
#define _IEMU_H_

/* Definitions for AKA authentication */
#define IEMU_AKA_AUTH_RESP_SZ      (8)
#define IEMU_AKA_AUTH_CK_SZ        (16)
#define IEMU_AKA_AUTH_IK_SZ        (16)

/* Definitions needed for Mutual Authenication (AKA version 1 & 2). */
#define IEMU_AUTH_AKA_RANDLEN    (16)
#define IEMU_AUTH_AKA_AUTNLEN    (16)
#define IEMU_AUTH_AKA_SQNLEN     (6)
#define IEMU_AUTH_AKA_MACLEN     (8)
#define IEMU_AUTH_AKA_RESLEN     (8)
#define IEMU_AUTH_AKA_AKLEN      (6)
#define IEMU_AUTH_AKA_CKLEN      (16)
#define IEMU_AUTH_AKA_IKLEN      (16)

#define IEMU_AUTH_KEY_SIZE (16)
#define IEMU_AUTH_OP_SIZE (16)
#define IEMU_AUTH_AMF_SIZE (2)

#define IEMU_TASK_NAME        ("iemuTask")
#define IEMU_TASK_STACK_SZ    (1000)
#define IEMU_STRING_SZ       (64)

#define IEMU_OK (0)
#define IEMU_ERR (-1)

typedef struct {
    OSAL_TaskId taskId;
    GBA_Event   gbaEvent;
    GBA_Command gbaCommand;
    char        impi[IEMU_STRING_SZ];
    char        simki[IEMU_STRING_SZ]; /* should be 32 hex chars for 16 bytes */
    char        operatorId[IEMU_AUTH_OP_SIZE];
    char        akaAuthResp[IEMU_AKA_AUTH_RESP_SZ];
    char        akaAuthCk[IEMU_AKA_AUTH_CK_SZ];
    char        akaAuthIk[IEMU_AKA_AUTH_IK_SZ];
} IEMU_Obj;

/*
 * This is the ISIM global object.
 */
typedef struct {
    IEMU_Obj iemuObj;
} IEMU_GlobalObj;

/*
 * ISIM Public functions.
 */
vint IEMU_init(
    void);

vint IEMU_shutdown(
    void);

vint IEMU_credSetup(
    char *impi_ptr,
    char *simki_ptr);


#endif
