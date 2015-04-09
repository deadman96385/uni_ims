
/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004-2012 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 19918 $ $Date: 2013-02-26 15:29:12 +0800 (Tue, 26 Feb 2013) $
 *
 */

#include <osal.h>
#include <isi.h>
#include <isi_rpc.h>
#include <isi_xdr.h>
#include "_isi_client.h"

/*
 * ======== _ISI_clientInit() ========
 *
 * Initilize ISI client.
 *
 * Return:
 *  ISI_RETURN_FAILED  Failed to initialize ISI client.
 *  ISI_RETURN_OK      Initialized successfully.
 */
ISI_Return _ISI_clientInit(
    ISI_ClientObj **pIsi_ptr)
{
    /* Allocate global object. */
    if (NULL == (*pIsi_ptr = OSAL_memCalloc(1,
            sizeof(ISI_ClientObj), 0))) {
        OSAL_logMsg("%s:%d Failed to allocate ISI_Client_GlobalObj.\n",
                __FUNCTION__, __LINE__);
        return (ISI_RETURN_FAILED);
    }
    /* Clean the memory. */
    OSAL_memSet(*pIsi_ptr, 0, sizeof(ISI_ClientObj));

    /* Create isi rpc write message queue. */
    if (0 == ((*pIsi_ptr)->isiRpcWriteQ = OSAL_msgQCreate(
            ISI_RPC_DOWN_QUEUE_NAME,
            OSAL_MODULE_ISI_RPC_CLIENT, OSAL_MODULE_ISI_RPC_SERVER,
            OSAL_DATA_STRUCT_ISI_DATA,
            ISI_RPC_MSGQ_LEN, ISI_DATA_SIZE, 0))) {
        OSAL_logMsg("%s:%d Failed to create queue:%s\n", __FUNCTION__, __LINE__,
                ISI_RPC_DOWN_QUEUE_NAME);
        return (ISI_RETURN_FAILED);
    }

    /* Create isi rpc read message queue. */
    if (0 == ((*pIsi_ptr)->isiRpcReadQ = OSAL_msgQCreate(
            ISI_RPC_UP_QUEUE_NAME,
            OSAL_MODULE_ISI_RPC_SERVER, OSAL_MODULE_ISI_RPC_CLIENT,
            OSAL_DATA_STRUCT_ISI_DATA,
            ISI_RPC_MSGQ_LEN, ISI_DATA_SIZE, 0))) {
        OSAL_logMsg("%s:%d Failed to create queue:%s\n", __FUNCTION__, __LINE__,
                ISI_RPC_UP_QUEUE_NAME);
        return (ISI_RETURN_FAILED);
    }

    /* Create isi evt rpc write message queue. */
    if (0 == ((*pIsi_ptr)->isiEvtRpcWriteQ = OSAL_msgQCreate(
            ISI_EVT_RPC_DOWN_QUEUE_NAME,
            OSAL_MODULE_ISI_RPC_CLIENT, OSAL_MODULE_ISI_RPC_SERVER,
            OSAL_DATA_STRUCT_ISI_DATA,
            ISI_RPC_MSGQ_LEN, ISI_DATA_SIZE, 0))) {
        OSAL_logMsg("%s:%d Failed to create queue:%s\n", __FUNCTION__, __LINE__,
                ISI_EVT_RPC_DOWN_QUEUE_NAME);
        return (ISI_RETURN_FAILED);
    }

    /* Create isi evt rpc write message queue. */
    if (0 == ((*pIsi_ptr)->isiEvtRpcReadQ = OSAL_msgQCreate(
            ISI_EVT_RPC_UP_QUEUE_NAME,
            OSAL_MODULE_ISI_RPC_SERVER, OSAL_MODULE_ISI_RPC_CLIENT,
            OSAL_DATA_STRUCT_ISI_DATA,
            ISI_RPC_MSGQ_LEN, ISI_DATA_SIZE, 0))) {
        OSAL_logMsg("%s:%d Failed to create queue:%s\n", __FUNCTION__, __LINE__,
                ISI_EVT_RPC_UP_QUEUE_NAME);
        return (ISI_RETURN_FAILED);
    }

    return (ISI_RETURN_OK);
}

/*
 * ======== _ISI_clientShutdown() ========
 *
 * Shutdown ISI client.
 *
 * Return:
 *   ISI_RETURN_OK Always return ok.
 */
ISI_Return _ISI_clientShutdown(
    ISI_ClientObj *isi_ptr)
{
    /* Delete message queues. */
    if (NULL != isi_ptr->isiRpcReadQ) {
        OSAL_msgQDelete(isi_ptr->isiRpcReadQ);
    }
    if (NULL != isi_ptr->isiRpcWriteQ) {
        OSAL_msgQDelete(isi_ptr->isiRpcWriteQ);
    }
    if (NULL != isi_ptr->isiEvtRpcReadQ) {
        OSAL_msgQDelete(isi_ptr->isiEvtRpcReadQ);
    }
    if (NULL != isi_ptr->isiEvtRpcWriteQ) {
        OSAL_msgQDelete(isi_ptr->isiEvtRpcWriteQ);
    }

    /* Free memory of global object. */
    if (NULL != isi_ptr) {
        OSAL_memFree(isi_ptr, 0);
    }
    isi_ptr = NULL;

    return (ISI_RETURN_OK);
}

/*
 * ======== _ISI_clientRpcReadIsi() ========
 *
 * Read ISI rpc from message queue.
 *
 * Return:
 *  OSAL_FAIL     Read failed.
 *  OSAL_SUCCESS    Read done.
 */
OSAL_Status _ISI_clientRpcReadIsi(
    ISI_ClientObj *isi_ptr,
    void          *buf_ptr,
    vint           size,
    vint           timeout)
{
    OSAL_Boolean isTimeout;

    if (NULL == isi_ptr) {
        return (OSAL_FAIL);
    }

    /* Read message queue. */
    if (0 >= OSAL_msgQRecv(isi_ptr->isiRpcReadQ, buf_ptr, size,
            timeout, &isTimeout)) {
        OSAL_logMsg("%s:%d Failed to read isi rpc queue.\n",
                __FUNCTION__, __LINE__);
        return (OSAL_FAIL);
    }
    return (OSAL_SUCCESS);
}

/*
 * ======== _ISI_clientRpcWriteIsi() ========
 *
 * Write ISI rpc to message queue.
 *
 * Return:
 *  OSAL_FAIL     Write failed.
 *  OSAL_SUCCESS    Write done.
 */
OSAL_Status _ISI_clientRpcWriteIsi(
    ISI_ClientObj *isi_ptr,
    void          *buf_ptr,
    vint           size)
{
    if (NULL == isi_ptr) {
        return (OSAL_FAIL);
    }

    /* Write queue. */
    if (OSAL_SUCCESS != OSAL_msgQSend(isi_ptr->isiRpcWriteQ,
            (void *)buf_ptr, size, OSAL_NO_WAIT, NULL)) {
        OSAL_logMsg("%s:%d Failed to write isi rpc queue.\n",
                __FUNCTION__, __LINE__);
        return (OSAL_FAIL);
    }

    return (OSAL_SUCCESS);
}

/*
 * ======== _ISI_clientRpcReadIsiEvt() ========
 *
 * Read ISI evt rpc from message queue.
 *
 * Return:
 *  OSAL_FAIL     Read failed.
 *  OSAL_SUCCESS    Read done.
 */
OSAL_Status _ISI_clientRpcReadIsiEvt(
    ISI_ClientObj *isi_ptr,
    void          *buf_ptr,
    vint           size,
    vint           timeout)
{
    OSAL_Boolean isTimeout;

    if (NULL == isi_ptr) {
        return (OSAL_FAIL);
    }

    /* Read message queue. */
    if (0 >= OSAL_msgQRecv(isi_ptr->isiEvtRpcReadQ, buf_ptr, size,
            timeout, &isTimeout)) {
        OSAL_logMsg("%s:%d Failed to read isi evt rpc queue.\n",
                __FUNCTION__, __LINE__);
        return (OSAL_FAIL);
    }
    return (OSAL_SUCCESS);
}

/*
 * ======== _ISI_clientRpcWriteIsiEvt() ========
 *
 * Write ISI evt rpc to message queue.
 *
 * Return:
 *  OSAL_FAIL     Write failed.
 *  OSAL_SUCCESS    Write done.
 */
OSAL_Status _ISI_clientRpcWriteIsiEvt(
    ISI_ClientObj *isi_ptr,
    void          *buf_ptr,
    vint           size)
{
    if (NULL == isi_ptr) {
        return (OSAL_FAIL);
    }

    /* Write queue. */
    if (OSAL_SUCCESS != OSAL_msgQSend(isi_ptr->isiEvtRpcWriteQ,
            (void *)buf_ptr, size, OSAL_NO_WAIT, NULL)) {
        OSAL_logMsg("%s:%d Failed to write isi rpc queue.\n",
                __FUNCTION__, __LINE__);
        return (OSAL_FAIL);
    }

    return (OSAL_SUCCESS);
}
