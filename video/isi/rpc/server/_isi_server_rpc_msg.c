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
#include <isip.h>
#include <isi_rpc.h>
#include <isi_xdr.h>
#include "_isi_server.h"

/*
 * ======== _ISI_serverRpcInit() ========
 *
 * Initialize ISI server rpc for message queue.
 *
 * Return:
 *  OSAL_FAIL     Failed.
 *  OSAL_SUCCESS  Done.
 */
OSAL_Status _ISI_serverRpcInit(
    ISI_ServerObj *isi_ptr)
{
    /* Create msg Q for writing ISI RPC to ISI Client */
    if (NULL == (isi_ptr->isiRpcWriteQ = OSAL_msgQCreate(
            ISI_RPC_UP_QUEUE_NAME,
            OSAL_MODULE_ISI_RPC_SERVER, OSAL_MODULE_ISI_RPC_CLIENT,
            OSAL_DATA_STRUCT_ISI_DATA,
            ISI_RPC_MSGQ_LEN,
            ISI_DATA_SIZE, 0))) {
        OSAL_logMsg("%s:%d Could not create %s queue.\n", __FUNCTION__,
                __LINE__, ISI_RPC_UP_QUEUE_NAME);
        return (OSAL_FAIL);
    }
    /* Create msg Q for reading  ISI RPC to ISI Client */
    if (NULL == (isi_ptr->isiRpcReadQ = OSAL_msgQCreate(
            ISI_RPC_DOWN_QUEUE_NAME,
            OSAL_MODULE_ISI_RPC_CLIENT, OSAL_MODULE_ISI_RPC_SERVER,
            OSAL_DATA_STRUCT_ISI_DATA,
            ISI_RPC_MSGQ_LEN,
            ISI_DATA_SIZE, 0))) {
        OSAL_logMsg("%s:%d Could not create %s queue.\n", __FUNCTION__,
                __LINE__, ISI_RPC_DOWN_QUEUE_NAME);
        return (OSAL_FAIL);
    }
    /* Create msg Q for writing ISI EVT RPC to ISI Client */
    if (NULL == (isi_ptr->isiEvtRpcWriteQ = OSAL_msgQCreate(
            ISI_EVT_RPC_UP_QUEUE_NAME,
            OSAL_MODULE_ISI_RPC_SERVER, OSAL_MODULE_ISI_RPC_CLIENT,
            OSAL_DATA_STRUCT_ISI_DATA,
            ISI_RPC_MSGQ_LEN,
            ISI_DATA_SIZE, 0))) {
        OSAL_logMsg("%s:%d Could not create %s queue.\n", __FUNCTION__,
                __LINE__, ISI_EVT_RPC_UP_QUEUE_NAME);
        return (OSAL_FAIL);
    }
    /* Create msg Q for reading ISI EVT RPC to ISI Client */
    if (NULL == (isi_ptr->isiEvtRpcReadQ = OSAL_msgQCreate(
            ISI_EVT_RPC_DOWN_QUEUE_NAME,
            OSAL_MODULE_ISI_RPC_CLIENT, OSAL_MODULE_ISI_RPC_SERVER,
            OSAL_DATA_STRUCT_ISI_DATA,
            ISI_RPC_MSGQ_LEN,
            ISI_DATA_SIZE, 0))) {
        OSAL_logMsg("%s:%d Could not create %s queue.\n", __FUNCTION__,
                __LINE__, ISI_EVT_RPC_DOWN_QUEUE_NAME);
        return (OSAL_FAIL);
    }

    return (OSAL_SUCCESS);
}

/*
 * ======== _ISI_serverRpcShutdown() ========
 *
 * Shutdown ISI server rpc for message queue.
 *
 * Return:
 *  OSAL_FAIL     Failed.
 *  OSAL_SUCCESS  Done.
 */
OSAL_Status _ISI_serverRpcShutdown(
    ISI_ServerObj *isi_ptr)
{
    if (NULL != isi_ptr->isiRpcWriteQ) {
        OSAL_msgQDelete(isi_ptr->isiRpcWriteQ);
    }
    if (NULL != isi_ptr->isiRpcReadQ) {
        OSAL_msgQDelete(isi_ptr->isiRpcReadQ);
    }
    if (NULL != isi_ptr->isiEvtRpcWriteQ) {
        OSAL_msgQDelete(isi_ptr->isiEvtRpcWriteQ);
    }
    if (NULL != isi_ptr->isiEvtRpcReadQ) {
        OSAL_msgQDelete(isi_ptr->isiEvtRpcReadQ);
    }

    return (OSAL_SUCCESS);
}

/*
 * ======== _ISI_serverRpcReadIsi() ========
 *
 * Read ISI rpc from message queue.
 *
 * Return:
 *  OSAL_FAIL     Read failed.
 *  OSAL_SUCCESS    Read done.
 */
OSAL_Status _ISI_serverRpcReadIsi(
    ISI_ServerObj *isi_ptr,
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
 * ======== _ISI_serverRpcWriteIsi() ========
 *
 * Write ISI rpc to message queue.
 *
 * Return:
 *  OSAL_FAIL     Write failed.
 *  OSAL_SUCCESS    Write done.
 */
OSAL_Status _ISI_serverRpcWriteIsi(
    ISI_ServerObj *isi_ptr,
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
 * ======== _ISI_serverRpcReadIsiEvt() ========
 *
 * Read ISI evt rpc from message queue.
 *
 * Return:
 *  OSAL_FAIL     Read failed.
 *  OSAL_SUCCESS    Read done.
 */
OSAL_Status _ISI_serverRpcReadIsiEvt(
    ISI_ServerObj *isi_ptr,
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
 * ======== _ISI_serverRpcWriteIsiEvt() ========
 *
 * Write ISI evt rpc to message queue.
 *
 * Return:
 *  OSAL_FAIL     Write failed.
 *  OSAL_SUCCESS    Write done.
 */
OSAL_Status _ISI_serverRpcWriteIsiEvt(
    ISI_ServerObj *isi_ptr,
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
