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
#include <vpad_vpmd.h>
#include "_isi_server.h"

/*
 * ======== _ISI_serverRpcInit() ========
 *
 * Initialize ISI server rpc for VPMD.
 *
 * Return:
 *  OSAL_SUCCESS  Done.
 */
OSAL_Status _ISI_serverRpcInit(
    ISI_ServerObj *isi_ptr)
{
    /* Always return OSAL_SUCCESS. */
    return (OSAL_SUCCESS);
}

/*
 * ======== _ISI_serverRpcShutdown() ========
 *
 * Shutdown ISI server rpc for VPMD.
 *
 * Return:
 *  OSAL_SUCCESS  Done.
 */
OSAL_Status _ISI_serverRpcShutdown(
    ISI_ServerObj *isi_ptr)
{
    /* Always return OSAL_SUCCESS. */
    return (OSAL_SUCCESS);
}

/*
 * ======== _ISI_serverRpcReadIsi() ========
 *
 * Read ISI rpc through VPMD API.
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
    return (VPMD_readIsiRpc(buf_ptr, size, timeout));
}

/*
 * ======== _ISI_serverRpcWriteIsi() ========
 *
 * Write ISI rpc through VPMD API.
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
    return (VPMD_writeIsiRpc(buf_ptr, size));
}

/*
 * ======== _ISI_serverRpcReadIsiEvt() ========
 *
 * Read ISI evt rpc through VPMD API.
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
    return (VPMD_readIsiEvtRpc(buf_ptr, size, timeout));
}

/*
 * ======== _ISI_serverRpcWriteIsiEvt() ========
 *
 * Write ISI evt rpc through VPMD API.
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
    return (VPMD_writeIsiEvtRpc(buf_ptr, size));
}
