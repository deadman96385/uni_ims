
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
#include <vpad_vpmd.h>
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
    return (ISI_RETURN_OK);
}

/*
 * ======== _ISI_clientRpcReadIsi() ========
 *
 * Read ISI rpc through VPAD API.
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
    return (VPAD_readIsiRpc(buf_ptr, size, timeout));
}

/*
 * ======== _ISI_clientRpcWriteIsi() ========
 *
 * Write ISI rpc through VPAD API.
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
    return (VPAD_writeIsiRpc(buf_ptr, size));
}

/*
 * ======== _ISI_clientRpcReadIsiEvt() ========
 *
 * Read ISI evt rpc through VPAD API.
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
    return (VPAD_readIsiEvtRpc(buf_ptr, size, timeout));
}

/*
 * ======== _ISI_clientRpcWriteIsiEvt() ========
 *
 * Write ISI evt rpc through VPAD API.
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
    return (VPAD_writeIsiEvtRpc(buf_ptr, size));
}
