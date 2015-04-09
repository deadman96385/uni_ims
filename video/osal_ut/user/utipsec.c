/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev$ $Date$
 */

 /* this should be all the user needs */
#include "osal_ut.h"

/*
 * Definition
 */
#define IPSEC_UT_ADDR_IPV4  "127.0.0.1"
#define IPSEC_UT_ADDR_IPV6  "::1"
#define IPSEC_UT_TX_PORT    5070
#define IPSEC_UT_RX_PORT    5071

typedef struct {
    struct {
        OSAL_IpsecSa sa;
        OSAL_IpsecSp sp;
    } ipsecOut;
    struct {
        OSAL_IpsecSa sa;
        OSAL_IpsecSp sp;
    } ipsecIn;
} IPSEC_Ut_Obj;

IPSEC_Ut_Obj *ipsec_ut_ptr = NULL;
static int ipsec_ut_recvTask_completed;
char    ipsec_ut_sendBuf[1025];

/*
 * ======== _OSALUT_ipsecHexCharToInt() ========
 *
 * Private function to convert hex character to int
 *
 * Returns:
 *    0: Failed to convert.
 *    Non-zero: converted int.
 */
static int _OSALUT_ipsecHexCharToInt(
    char c)
{
    if (c >= '0' && c <= '9') {
        return (c - '0');
    }
    if (c >= 'A' && c <= 'F') {
        return (c - 'A' + 10);
    }
    if (c >= 'a' && c <= 'f') {
        return (c - 'a' + 10);
    }
    return 0;
}

/*
 * ======== _OSALUT_ipsecHexStringToBytes() ========
 *
 * Private function to convert hex string to bytes
 *
 * Returns:
 *    Converted bytes size.
 */
int _OSALUT_ipsecHexStringToBytes(
    char *in_ptr,
    char *out_ptr)
{
    int sz, i;
    char a,b;

    sz = OSAL_strlen(in_ptr);

    for (i = 0 ; i < sz ; i += 2) {
        a = _OSALUT_ipsecHexCharToInt(in_ptr[i]) << 4;
        b = _OSALUT_ipsecHexCharToInt(in_ptr[i + 1]);
        out_ptr[i / 2] = a | b;
    }
    return (i / 2);
}

/*
 * ======== _OSALUT_ipsecInitTx() ========
 *
 * initialize SA and SP of Tx
 *
 * Return:
 *  UT_PASS: Exit normally.
 */
static UT_Return _OSALUT_ipsecInitTx(
    char   *txAddr,
    char   *rxAddr,
    uint16  txPort,
    uint16  rxPort)
{
    OSAL_IpsecSa   *outSA_ptr;
    OSAL_IpsecSp   *outSP_ptr;

    outSA_ptr = &ipsec_ut_ptr->ipsecOut.sa;
    outSP_ptr = &ipsec_ut_ptr->ipsecOut.sp;

    /*
     * Configure Tx
     */
    OSAL_netStringToAddress((int8 *)txAddr, &outSA_ptr->srcAddr);
    outSA_ptr->srcAddr.port = OSAL_netHtons(txPort);
    OSAL_netStringToAddress((int8 *)rxAddr, &outSA_ptr->dstAddr);
    outSA_ptr->dstAddr.port = OSAL_netHtons(rxPort);
    outSA_ptr->protocol     = OSAL_IPSEC_PROTOCOL_ESP;
    outSA_ptr->mode         = OSAL_IPSEC_SP_MODE_TRANSPORT;
    outSA_ptr->spi          = 0x00001234;
    outSA_ptr->reqId        = 0;
    outSA_ptr->algAh        = OSAL_IPSEC_AUTH_ALG_HMAC_SHA1_96;
    outSA_ptr->algEsp       = OSAL_IPSEC_ENC_ALG_DES_EDE3_CBC;
    _OSALUT_ipsecHexStringToBytes("b48408f4655000f588a1a22cc14697d1a4d259cd",
            outSA_ptr->keyAh);
    _OSALUT_ipsecHexStringToBytes(
            "84cc855d6892207565811df4edd6bff5cf53af9106b72461",
            outSA_ptr->keyEsp);

    /*
     * Tx addr is as inSP_ptr->srcAddr
     * Tx port is as inSP_ptr->srcAddr.port
     * Rx addr is as inSP_ptr->dstAddr
     * Rx port is as inSP_ptr->dstAddr.port
     * SP dir is OUT.
     */
    OSAL_netStringToAddress((int8 *)txAddr, &outSP_ptr->srcAddr);
    outSP_ptr->srcAddr.port = OSAL_netHtons(txPort);
    OSAL_netStringToAddress((int8 *)rxAddr, &outSP_ptr->dstAddr);
    outSP_ptr->dstAddr.port = OSAL_netHtons(rxPort);
    outSP_ptr->protocol     = OSAL_IPSEC_PROTOCOL_ESP;
    outSP_ptr->dir          = OSAL_IPSEC_DIR_OUT;
    outSP_ptr->transport    = OSAL_IPSEC_TRANSPORT_ANY;
    outSP_ptr->mode         = OSAL_IPSEC_SP_MODE_TRANSPORT;
    outSP_ptr->level        = OSAL_IPSEC_SP_LEVEL_REQUIRE;
    outSP_ptr->reqId        = 0;

    return (UT_PASS);
}

/*
 * ======== _OSALUT_ipsecInitRx() ========
 *
 * initialize SA and SP of Rx
 *
 * Return:
 *  UT_PASS: Exit normally.
 */
static UT_Return _OSALUT_ipsecInitRx(
    char   *txAddr,
    char   *rxAddr,
    uint16  txPort,
    uint16  rxPort)
{
    OSAL_IpsecSa   *inSA_ptr;
    OSAL_IpsecSp   *inSP_ptr;

    inSA_ptr  = &ipsec_ut_ptr->ipsecIn.sa;
    inSP_ptr  = &ipsec_ut_ptr->ipsecIn.sp;

    /*
     * Configure Rx
     */
    OSAL_netStringToAddress((int8 *)rxAddr, &inSA_ptr->srcAddr);
    inSA_ptr->srcAddr.port = OSAL_netHtons(rxPort);
    OSAL_netStringToAddress((int8 *)txAddr, &inSA_ptr->dstAddr);
    inSA_ptr->dstAddr.port = OSAL_netHtons(txPort);
    inSA_ptr->protocol     = OSAL_IPSEC_PROTOCOL_ESP;
    inSA_ptr->mode         = OSAL_IPSEC_SP_MODE_TRANSPORT;
    inSA_ptr->spi          = 0x00005678;
    inSA_ptr->reqId        = 0;
    inSA_ptr->algAh        = OSAL_IPSEC_AUTH_ALG_HMAC_SHA1_96;
    inSA_ptr->algEsp       = OSAL_IPSEC_ENC_ALG_DES_EDE3_CBC;
    _OSALUT_ipsecHexStringToBytes("b48408f4655000f588a1a22cc14697d1a4d259ca",
            inSA_ptr->keyAh);
    _OSALUT_ipsecHexStringToBytes(
            "84cc855d6892207565811df4edd6bff5cf53af9106b72461",
            inSA_ptr->keyEsp);

    /*
     * Tx addr is as inSP_ptr->srcAddr
     * Tx port is as inSP_ptr->srcAddr.port
     * Rx addr is as inSP_ptr->dstAddr
     * Rx port is as inSP_ptr->dstAddr.port
     * SP dir is IN
     */
    OSAL_netStringToAddress((int8 *)txAddr, &inSP_ptr->srcAddr);
    inSP_ptr->srcAddr.port = OSAL_netHtons(txPort);
    OSAL_netStringToAddress((int8 *)rxAddr, &inSP_ptr->srcAddr);
    inSP_ptr->dstAddr.port = OSAL_netHtons(rxPort);
    inSP_ptr->protocol     = OSAL_IPSEC_PROTOCOL_ESP;
    inSP_ptr->dir          = OSAL_IPSEC_DIR_IN;
    inSP_ptr->transport    = OSAL_IPSEC_TRANSPORT_ANY;
    inSP_ptr->mode         = OSAL_IPSEC_SP_MODE_TRANSPORT;
    inSP_ptr->level        = OSAL_IPSEC_SP_LEVEL_REQUIRE;
    inSP_ptr->reqId        = 0;

    return (UT_PASS);
}

/*
 * ======== _OSALUT_ipsecSend() ========
 *
 * Send data through IPSec
 *
 * Return:
 *  UT_PASS: Exit normally.
 *  UT_FAIL: Invalid arguments.
 */
static UT_Return _OSALUT_ipsecSend(
    void)
{
    int            size;
    OSAL_NetSockId socketFd;

    /* Create Tx socket */
    if (OSAL_SUCCESS != OSAL_netSocket(&socketFd,
            ipsec_ut_ptr->ipsecOut.sa.srcAddr.type)) {
        OSAL_logMsg("Cannot create Tx socket.\n");
        return (UT_FAIL);
    }

    /* Bind Tx socket */
    if (OSAL_SUCCESS != OSAL_netBindSocket(&socketFd,
            &ipsec_ut_ptr->ipsecOut.sa.srcAddr)) {
        OSAL_netCloseSocket(&socketFd);
        OSAL_logMsg("Cannot bind Tx socket.\n");
        return (UT_FAIL);
    }

    /* Send data */
    OSAL_logMsg("Sending message...\n");
    size = OSAL_strlen(ipsec_ut_sendBuf);
    if (OSAL_SUCCESS != OSAL_netSocketSendTo(&socketFd,
            ipsec_ut_sendBuf, &size, &ipsec_ut_ptr->ipsecOut.sa.dstAddr)) {
        OSAL_logMsg("Send failed.\n");
        return (UT_FAIL);
    }

    /* close Tx socket */
    OSAL_netCloseSocket(&socketFd);

    return (UT_PASS);
}

/*
 * ======== _OSALUT_ipsecRecv() ========
 *
 * recv data through IPSec
 *
 * Return:
 *  UT_PASS: Exit normally.
 *  UT_FAIL: Invalid arguments.
 */
static UT_Return _OSALUT_ipsecRecv(
    void)
{
    int            size;
    OSAL_NetSockId socketFd;
    char    buf[1025];

    /* Create Rx socket */
    if (OSAL_SUCCESS != OSAL_netSocket(&socketFd,
            ipsec_ut_ptr->ipsecIn.sa.srcAddr.type)) {
        OSAL_logMsg("Cannot create Rx socket.\n");
        return (UT_FAIL);
    }

    /* Bind Rx socket */
    if (OSAL_SUCCESS != OSAL_netBindSocket(&socketFd,
            &ipsec_ut_ptr->ipsecIn.sa.srcAddr)) {
        OSAL_netCloseSocket(&socketFd);
        OSAL_logMsg("Cannot bind Rx socket.\n");
        return (UT_FAIL);
    }

    size = sizeof(buf);
    if (OSAL_SUCCESS != OSAL_netSocketReceiveFrom(&socketFd,
            (void *)buf, &size, &ipsec_ut_ptr->ipsecIn.sa.dstAddr)) {
        OSAL_logMsg("Cannot receive data.\n");
        return (UT_FAIL);
    }
    buf[size] = 0;

    /* Compare receive msg with send msg */
    if(0 != OSAL_memCmp(buf, ipsec_ut_sendBuf, strlen(ipsec_ut_sendBuf))) {
        OSAL_logMsg("Unexpected receive msg.\n");
    }
    OSAL_logMsg("%s %d: recv msg=%s\n", __func__, __LINE__, buf);

    /* close Rx socket */
    OSAL_netCloseSocket(&socketFd);

    ipsec_ut_recvTask_completed = OSAL_SUCCESS;
    return (UT_PASS);
}

/*
 * ======== _OSALUT_ipsecTranceivTest() ========
 *
 * Function to create SA for msg trassmission. After receiving completely,
 * delete SA.
 *
 * Return:
 *  UT_PASS: Exit normally.
 *  UT_FAIL: Invalid arguments.
 */
static UT_Return _OSALUT_ipsecTranceivTest(
    void)
{
    OSAL_TaskId vtRecvId, vtSendId;

    ipsec_ut_recvTask_completed = OSAL_FAIL;

    /* Create SA of Rx */
    if (OSAL_SUCCESS != OSAL_ipsecCreateSA(&ipsec_ut_ptr->ipsecIn.sa,
            &ipsec_ut_ptr->ipsecIn.sp)) {
        OSAL_logMsg("Unexpected OSAL_ipsecCreateSA() In failure.\n");
        return (UT_FAIL);
    }    

    /* Create SA of Tx */
    if (OSAL_SUCCESS != OSAL_ipsecCreateSA(&ipsec_ut_ptr->ipsecOut.sa,
            &ipsec_ut_ptr->ipsecOut.sp)) {
        OSAL_logMsg("Unexpected OSAL_ipsecCreateSA() Out failure.\n");
        return (UT_FAIL);
    }


    /* Create a task to receive msg */
    vtRecvId = OSAL_taskCreate("ipsecUtRecv", OSAL_TASK_PRIO_NRT,
            (uint32) 8000, (OSAL_TaskPtr)_OSALUT_ipsecRecv, NULL);
        if ((OSAL_TaskId) NULL == vtRecvId) {
            OSAL_logMsg("Unexpected OSAL_taskCreate() recv task faulure - expected "
                    "non-NULL \n");
            return (UT_FAIL);
    }
    /* Wait for recv socket ready. */
    OSAL_taskDelay(100);

    vtSendId = OSAL_taskCreate("ipsecUtSend", OSAL_TASK_PRIO_NRT,
            (uint32) 8000, (OSAL_TaskPtr)_OSALUT_ipsecSend, NULL);
        if ((OSAL_TaskId) NULL == vtSendId) {
            OSAL_logMsg("Unexpected OSAL_taskCreate() recv task faulure - expected "
                    "non-NULL \n");
            return (UT_FAIL);
    }

    /* Wait for msg receiving completely. */
    OSAL_taskDelay(500);

    /* Delete Task */
    OSAL_taskDelete(vtRecvId);
    OSAL_taskDelete(vtSendId);

    /* Delete SA of Tx */
    if (OSAL_SUCCESS != OSAL_ipsecDeleteSA(&ipsec_ut_ptr->ipsecOut.sa,
            &ipsec_ut_ptr->ipsecOut.sp)) {
        OSAL_logMsg("Unexpected OSAL_ipsecDeleteSA() Out failure.\n");
        return (UT_FAIL);
    }
    /* Delete SA of Rx */
    if (OSAL_SUCCESS != OSAL_ipsecDeleteSA(&ipsec_ut_ptr->ipsecIn.sa,
            &ipsec_ut_ptr->ipsecIn.sp)) {
        OSAL_logMsg("Unexpected OSAL_ipsecDeleteSA() In failure.\n");
        return (UT_FAIL);
    }

    if (ipsec_ut_recvTask_completed  != OSAL_SUCCESS){
        return (UT_FAIL);
    }

    return (UT_PASS);
}

 /*
 * ========= do_test_ipsec() ========
 * Gen unit test vectors for each OSAL ipsec function
 *
 * Return:
 *  UT_PASS: Exit normally.
 *  UT_FAIL: Invalid arguments.
 */
UT_Return do_test_ipsec(
    void)
{
    OSAL_logMsg("IPsec Unit Test Starting...\n");
    /* reset this before every test */
    osal_utErrorsFound = 0;

    /*
     * Clear and init the global object
     */
    ipsec_ut_ptr = OSAL_memCalloc(1, sizeof(IPSEC_Ut_Obj), 0);
    OSAL_memSet(ipsec_ut_ptr, 0, sizeof(IPSEC_Ut_Obj));

    /* Init IPv4 */
    _OSALUT_ipsecInitTx(IPSEC_UT_ADDR_IPV4, IPSEC_UT_ADDR_IPV4,
            IPSEC_UT_TX_PORT, IPSEC_UT_RX_PORT);
    _OSALUT_ipsecInitRx(IPSEC_UT_ADDR_IPV4, IPSEC_UT_ADDR_IPV4,
            IPSEC_UT_TX_PORT, IPSEC_UT_RX_PORT);
    /*
     * IPsec test 1: (IPV4)
     * Default setting: ESP with DES_EDE3_CBC algrithom.
     * Trasnmission test with default setting.
     */
    OSAL_logMsg("IPsec test 1\n");
    memcpy(ipsec_ut_sendBuf, "IPsec unit test 1", strlen("IPsec unit test 1"));
    if (UT_PASS != _OSALUT_ipsecTranceivTest()) {
        prError("IPsec test 1 failure \n");
    }
    /*
     * IPsec test 2:(IPV4)
     * Change ESP alorithom as AES_CBC to do transmissiono test.
     */
    OSAL_logMsg("IPsec test 2\n");
    memcpy(ipsec_ut_sendBuf, "IPsec unit test 2", strlen("IPsec unit test 2"));
    ipsec_ut_ptr->ipsecOut.sa.algEsp = OSAL_IPSEC_ENC_ALG_AES_CBC;
    ipsec_ut_ptr->ipsecIn.sa.algEsp = OSAL_IPSEC_ENC_ALG_AES_CBC;
    if (UT_PASS!= _OSALUT_ipsecTranceivTest()) {
        prError("IPsec test 2 failure \n");
    }

    /*
     * IPsec test 3:(IPV4)
     * Change protocal as AH with SHA1 alorithom to do transmissiono test.
     */
    OSAL_logMsg("IPsec test 3\n");
    memcpy(ipsec_ut_sendBuf, "IPsec unit test 3", strlen("IPsec unit test 3"));
    ipsec_ut_ptr->ipsecOut.sa.protocol = OSAL_IPSEC_PROTOCOL_AH;
    ipsec_ut_ptr->ipsecOut.sp.protocol = OSAL_IPSEC_PROTOCOL_AH;
    ipsec_ut_ptr->ipsecIn.sa.protocol = OSAL_IPSEC_PROTOCOL_AH;
    ipsec_ut_ptr->ipsecIn.sp.protocol = OSAL_IPSEC_PROTOCOL_AH;
    if (UT_PASS!= _OSALUT_ipsecTranceivTest()) {
        prError("IPsec test 3 failure \n");
    }

    /*
     * IPsec test 4:(IPV4)
     * Change AH alorithom as MD5 to do transmissiono test.
     */
    OSAL_logMsg("IPsec test 4\n");
    memcpy(ipsec_ut_sendBuf, "IPsec unit test 4", strlen("IPsec unit test 4"));
    ipsec_ut_ptr->ipsecOut.sa.algAh = OSAL_IPSEC_AUTH_ALG_HMAC_MD5_96;
    ipsec_ut_ptr->ipsecIn.sa.algAh = OSAL_IPSEC_AUTH_ALG_HMAC_MD5_96;
    if (UT_PASS!= _OSALUT_ipsecTranceivTest()) {
        prError("IPsec test 4 failure \n");
    }

    /* Init IPv6 */
    _OSALUT_ipsecInitTx(IPSEC_UT_ADDR_IPV6, IPSEC_UT_ADDR_IPV6,
            IPSEC_UT_TX_PORT, IPSEC_UT_RX_PORT);
    _OSALUT_ipsecInitRx(IPSEC_UT_ADDR_IPV6, IPSEC_UT_ADDR_IPV6,
            IPSEC_UT_TX_PORT, IPSEC_UT_RX_PORT);
    /*
     * IPsec test 5: (IPV6)
     * Default setting: ESP with DES_EDE3_CBC algrithom.
     * Trasnmission test with default setting.
     */
    OSAL_logMsg("IPsec test 5\n");
    memcpy(ipsec_ut_sendBuf, "IPsec unit test 5", strlen("IPsec unit test 5"));
    if (UT_PASS != _OSALUT_ipsecTranceivTest()) {
        prError("IPsec test 5 failure \n");
    }

    /*
     * IPsec test 6:(IPV6)
     * Change ESP alorithom as AES_CBC to do transmissiono test.
     */
    OSAL_logMsg("IPsec test 6\n");
    memcpy(ipsec_ut_sendBuf, "IPsec unit test 6", strlen("IPsec unit test 6"));
    ipsec_ut_ptr->ipsecOut.sa.algEsp = OSAL_IPSEC_ENC_ALG_AES_CBC;
    ipsec_ut_ptr->ipsecIn.sa.algEsp = OSAL_IPSEC_ENC_ALG_AES_CBC;
    if (UT_PASS!= _OSALUT_ipsecTranceivTest()) {
        prError("IPsec test 6 failure \n");
    }

    /*
     * IPsec test 7:(IPV6)
     * Change protocal as AH with SHA1 alorithom to do transmissiono test.
     */
    OSAL_logMsg("IPsec test 7\n");
    memcpy(ipsec_ut_sendBuf, "IPsec unit test 7", strlen("IPsec unit test 7"));
    ipsec_ut_ptr->ipsecOut.sa.protocol = OSAL_IPSEC_PROTOCOL_AH;
    ipsec_ut_ptr->ipsecOut.sp.protocol = OSAL_IPSEC_PROTOCOL_AH;
    ipsec_ut_ptr->ipsecIn.sa.protocol = OSAL_IPSEC_PROTOCOL_AH;
    ipsec_ut_ptr->ipsecIn.sp.protocol = OSAL_IPSEC_PROTOCOL_AH;
    if (UT_PASS!= _OSALUT_ipsecTranceivTest()) {
        prError("IPsec test 7 failure \n");
    }

    /*
     * IPsec test 8:(IPV6)
     * Change AH alorithom as MD5 to do transmissiono test.
     */
    OSAL_logMsg("IPsec test 8\n");
    memcpy(ipsec_ut_sendBuf, "IPsec unit test 8", strlen("IPsec unit test 8"));
    ipsec_ut_ptr->ipsecOut.sa.algAh = OSAL_IPSEC_AUTH_ALG_HMAC_MD5_96;
    ipsec_ut_ptr->ipsecIn.sa.algAh = OSAL_IPSEC_AUTH_ALG_HMAC_MD5_96;
    if (UT_PASS!= _OSALUT_ipsecTranceivTest()) {
        prError("IPsec test 8 failure \n");
    }

    /*
    * IPsec test 9:
    * Test fail case to create SA with NULL pointer.
    */
    OSAL_logMsg("IPsec test 9\n");
    if (OSAL_FAIL != OSAL_ipsecCreateSA(NULL, NULL)) {
        prError("Unexpected OSAL_ipsecCreateSA() return.\n");
    }

    /*
    * IPsec test 10:
    * Test fail case to delete SA with NULL pointer.
    */
    OSAL_logMsg("IPsec test 10\n");
    /* Create SA of Rx */
    if (OSAL_SUCCESS != OSAL_ipsecCreateSA(&ipsec_ut_ptr->ipsecIn.sa,
            &ipsec_ut_ptr->ipsecIn.sp)) {
        OSAL_logMsg("Unexpected OSAL_ipsecCreateSA() In failure.\n");
        return (UT_FAIL);
    }  
    /* Create SA of Tx */
    if (OSAL_SUCCESS != OSAL_ipsecCreateSA(&ipsec_ut_ptr->ipsecOut.sa,
            &ipsec_ut_ptr->ipsecOut.sp)) {
        prError("Unexpected OSAL_ipsecCreateSA() Out failure.\n");
    }
    /* Delte SA with NULL pointer */
    if (OSAL_FAIL != OSAL_ipsecDeleteSA(NULL, NULL)) {
        prError("Unexpected OSAL_ipsecDeleteSA() return.\n");
    }
    /* Delte SA of Tx */
    if (OSAL_SUCCESS != OSAL_ipsecDeleteSA(&ipsec_ut_ptr->ipsecOut.sa,
            &ipsec_ut_ptr->ipsecOut.sp)) {
        prError("Unexpected OSAL_ipsecDeleteSA() Out failure.\n");
        return (UT_FAIL);
    }

    /* free global object */
    OSAL_memFree(ipsec_ut_ptr, 0);
    ipsec_ut_ptr = NULL;

    if (0 == osal_utErrorsFound) {
        OSAL_logMsg("IPsec Unit Test Completed Successfully.\n");
        return (UT_PASS);
    }
    else {
        OSAL_logMsg("IPsec Unit Test Completed with %d errors.\n",
                osal_utErrorsFound);
        return (UT_FAIL);
    }
}
