/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004-2012 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 19918 $ $Date: 2013-02-26 15:29:12 +0800 (Tue, 26 Feb 2013) $
 *
 */

#ifndef __ISI_CLIENT_RPC_H_
#define __ISI_CLIENT_RPC_H_

#ifdef __cplusplus
extern "C" {
#endif

/*
 * In Seattle architecture, ISI client/server are the in the same processor,
 * use OSAL message queue to write/read data.
 */
OSAL_Status _ISI_clientRpcReadIsi(
    ISI_ClientObj *isi_ptr,
    void          *buf_ptr,
    vint           size,
    vint           timeout);

OSAL_Status _ISI_clientRpcWriteIsi(
    ISI_ClientObj *isi_ptr,
    void          *buf_ptr,
    vint           size);

OSAL_Status _ISI_clientRpcReadIsiEvt(
    ISI_ClientObj *isi_ptr,
    void          *buf_ptr,
    vint           size,
    vint           timeout);

OSAL_Status _ISI_clientRpcWriteIsiEvt(
    ISI_ClientObj *isi_ptr,
    void          *buf_ptr,
    vint           size);
#endif
