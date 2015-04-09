/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004-2010 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev$ $Date$
 *
 */

#include <http.h>
#include "osal.h"
#include "gba.h"
#include "iemu.h"
#include "milenage.h"

/*
 * This module simulate the ISIM authentication features.
 * It interacts with GBA similar to the intended CSM interface, via cmd/evt queues.
 */


/* Global object. */
static IEMU_GlobalObj _IEMU_globalObj;

uint8   k[IEMU_AUTH_KEY_SIZE + 1];
uint8   op[IEMU_AUTH_OP_SIZE];
uint8   amf[IEMU_AUTH_AMF_SIZE + 1];
uint8   ck[IEMU_AUTH_AKA_CKLEN + 1];
uint8   ik[IEMU_AUTH_AKA_IKLEN + 1];
uint8   ak[IEMU_AUTH_AKA_AKLEN + 1];
uint8   sqn[IEMU_AUTH_AKA_SQNLEN + 1];
uint8   xmac[IEMU_AUTH_AKA_MACLEN + 1];
uint8   res[IEMU_AUTH_AKA_RESLEN + 1];

void
_IEMU_hexdump(buf, len)
    const void *buf;
    int len;
{
    int i;

    for (i = 0; i < len; i++) {
        if (i != 0 && i % 32 == 0) printf("\n");
        if (i % 4 == 0) OSAL_logMsg(" ");
        OSAL_logMsg("%02x", ((const unsigned char *)buf)[i]);
    }
#if 0
    if (i % 32 != 0) OSAL_logMsg("\n");
#endif

    return;
}

/*
 * ======== _IEMU_calculateAKAResp() ========
 * This function calculates the response for AKA challenge.
 *
 * Returns:
 */
vint _IEMU_calculateAKAResp(
    uint8           *rand_ptr,
    uint8           *autn_ptr,
    IEMU_Obj        *obj_ptr)
{
    vint    i;
    uint8  *sqn_ptr;
    //uint8  *mac_ptr;
    char   hex2d[3] = {0, 0, 0};

    sqn_ptr = autn_ptr;
    // mac_ptr = autn_ptr + IEMU_AUTH_AKA_SQNLEN + IEMU_AUTH_AMF_SIZE;

    /* Copy k. op, and amf */
    OSAL_memSet(k, 0, sizeof(k));
    /* simple and slow way to convert hex string into bytes */
    for(i = 0; i < IEMU_AUTH_KEY_SIZE; i++) {
        hex2d[0] = obj_ptr->simki[i*2];
        hex2d[1] = obj_ptr->simki[i*2+1];
        k[i] = OSAL_htoi(hex2d);
    }
    OSAL_logMsg("%s %d: simki =%s\n",
                __FILE__, __LINE__ , obj_ptr->simki);
    OSAL_logMsg("simki byte buffer:\n");
    _IEMU_hexdump(k, IEMU_AUTH_KEY_SIZE);

    /* Optional 'op' and 'amf' code, leave them empty */
    OSAL_memSet(op, 0, sizeof(op));
    OSAL_memSet(amf, 0, sizeof(amf));

    /* Given key K and random challenge RAND, compute response RES,
     * confidentiality key CK, integrity key IK and anonymity key AK.
     */
    f2345(k, rand_ptr, res, ck, ik, ak, op);

    /* Compute sequence number SQN */
    for (i = 0 ; i < IEMU_AUTH_AKA_SQNLEN; ++i) {
        sqn[i] = (char) (sqn_ptr[i] ^ ak[i]);
    }

    /* Verify MAC in the challenge */
    f1(k, rand_ptr, sqn, amf, xmac, op);

//    if (0 != OSAL_memCmp(mac_ptr, xmac, IEMU_AUTH_AKA_MACLEN)) {
//      obj_ptr->akaAuthResp[0] = '\0';
//        return (IEMU_ERR);
//    }

    /* xxx : adding auts test for sync error */

    OSAL_memCpy(obj_ptr->akaAuthResp,
            res,
            IEMU_AUTH_AKA_RESLEN);

    OSAL_memCpy(obj_ptr->akaAuthCk,
            ck,
            IEMU_AKA_AUTH_CK_SZ);

    OSAL_memCpy(obj_ptr->akaAuthIk,
            ik,
            IEMU_AKA_AUTH_IK_SZ);


    OSAL_logMsg("\n%s %d: rand\n",
            __FILE__, __LINE__ );
    _IEMU_hexdump(rand_ptr, IEMU_AUTH_AKA_RANDLEN);
    OSAL_logMsg("\n%s %d: autn\n",
            __FILE__, __LINE__ );
    _IEMU_hexdump(autn_ptr, IEMU_AUTH_AKA_AUTNLEN);
    OSAL_logMsg("\n%s %d: res\n",
            __FILE__, __LINE__ );
    _IEMU_hexdump(res, IEMU_AUTH_AKA_RESLEN);
    OSAL_logMsg("\n%s %d: \n",
                __FILE__, __LINE__ );
    return (IEMU_OK);
}

/*
 * ======== _IEMU_processEvent() ========
 *
 * This function is process the event from GBA.
 *
 * Returns:
 *     CSM_OK: Event processed successfully.
 *     CSM_ERR: Event processed failed
 */
vint _IEMU_processEvent(
    GBA_Event  *evt_ptr)
{
    GBA_Command *cmd_ptr = &_IEMU_globalObj.iemuObj.gbaCommand;

    switch (evt_ptr->type) {
        case GBA_EVENT_AKA_CHALLENGE:
            OSAL_logMsg("%s %d: Got AKA Challenge\n",
                                    __FILE__, __LINE__);
            if (IEMU_OK == _IEMU_calculateAKAResp(
                    (uint8 *)evt_ptr->u.akaChallenge.rand_ptr,
                    (uint8 *)evt_ptr->u.akaChallenge.autn_ptr,
                    &_IEMU_globalObj.iemuObj) ) {

                cmd_ptr->type = GBA_COMMAND_AKA_RESPONSE_SUCCESS;
                OSAL_logMsg("%s:%d calculated aka resp\n", __func__, __LINE__);
                HTTP_hexDump(cmd_ptr->u.akaResponse.resp,
                        IEMU_AKA_AUTH_RESP_SZ);
                OSAL_logMsg("\n%s:%d calculated aka resp\n", __func__, __LINE__);

                OSAL_memCpy(cmd_ptr->u.akaResponse.resp,
                        _IEMU_globalObj.iemuObj.akaAuthResp,
                        IEMU_AKA_AUTH_RESP_SZ);
                cmd_ptr->u.akaResponse.respLength = IEMU_AKA_AUTH_RESP_SZ;

                OSAL_memCpy(cmd_ptr->u.akaResponse.ck,
                        _IEMU_globalObj.iemuObj.akaAuthCk,
                        GBA_AKA_AUTH_CK_SZ);

                OSAL_memCpy(cmd_ptr->u.akaResponse.ik,
                        _IEMU_globalObj.iemuObj.akaAuthIk,
                        GBA_AKA_AUTH_IK_SZ);

                GBA_sendCommand(&_IEMU_globalObj.iemuObj.gbaCommand);
            } else {
                OSAL_logMsg("%s:%d failed calculated aka resp\n", __func__, __LINE__);
                cmd_ptr->type = GBA_COMMAND_AKA_RESPONSE_NETWORK_FAILURE;
                GBA_sendCommand(&_IEMU_globalObj.iemuObj.gbaCommand);
            }
            break;
        default:
            OSAL_logMsg("%s:%d ERROR bad event type\n", __func__, __LINE__);
            break;
    }

    return (IEMU_OK);
}

/*
 * ======== _IEMU_task() ========
 *
 * This function is the entry point for a thread that handles events from GBA
 *
 * Returns:
 *   Nothing and never.
 */
OSAL_TaskReturn _IEMU_task(
    OSAL_TaskArg taskArg)
{
    GBA_Event event;
    uint32          msTimeout;

    msTimeout = -1;

    /* Loop forever to receive event from CSM and process it */
    while (1) {
        if (GBA_getEvent(&event, msTimeout)) {
            _IEMU_processEvent(&event);
        }
        else {
            OSAL_taskDelay(100);
        }
    }

    OSAL_logMsg("IEMU_csmEventTask exit.\n");

    return(0);
}

/*
 * ======== IEMU_credSetup() ========
 *
 * Function to setup the credential.
 *
 * Returns:
 *      IEMU_OK: function exits normally.
 *      IEMU_ERR: in case of error
 */
vint IEMU_credSetup(
    char *impi_ptr,
    char *simki_ptr)
{
    OSAL_strncpy(_IEMU_globalObj.iemuObj.impi, impi_ptr, IEMU_STRING_SZ);
    OSAL_strncpy(_IEMU_globalObj.iemuObj.simki, simki_ptr, IEMU_STRING_SZ);
    return (IEMU_OK);
}

/*
 * ======== IEMU_init() ========
 *
 * Public routine for initializing the ISIM application
 *
 * Returns:
 *      IEMU_OK: function exits normally.
 *      IEMU_ERR: in case of error
 */
vint IEMU_init(
     void)
{
    OSAL_memSet(&_IEMU_globalObj, 0, sizeof(_IEMU_globalObj));

    /* Create  thread */
    if (0 == (_IEMU_globalObj.iemuObj.taskId = OSAL_taskCreate(
            IEMU_TASK_NAME,
            OSAL_TASK_PRIO_NRT,
            IEMU_TASK_STACK_SZ,
            (void *)_IEMU_task,
            (void *)&_IEMU_globalObj.iemuObj))) {
        OSAL_logMsg("Error starting IEMU thread\n");
        return (IEMU_ERR);
    }


    return (IEMU_OK);
}

/*
 * ======== IEMU_shutdown() ========
 *
 * Public routine for shutting down the module
 *
 * Returns:
 *      IEMU_OK: function exits normally.
 *      IEMU_ERR: in case of error
 */
vint IEMU_shutdown(
    void)
{
    return (IEMU_OK);
}


