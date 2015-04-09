/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004-2012 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 19918 $ $Date: 2013-02-26 15:29:12 +0800 (Tue, 26 Feb 2013) $
 *
 */

#ifndef __ISI_SERVER_RPC_H_
#define __ISI_SERVER_RPC_H_

#ifdef __cplusplus
extern "C" {
#endif

OSAL_Status _ISI_serverRpcInit(
    ISI_ServerObj *isi_ptr);

OSAL_Status _ISI_serverRpcShutdown(
    ISI_ServerObj *isi_ptr);

OSAL_Status _ISI_serverRpcReadIsi(
    ISI_ServerObj *isi_ptr,
    void          *buf_ptr,
    vint           size,
    vint           timeout);

OSAL_Status _ISI_serverRpcWriteIsi(
    ISI_ServerObj *isi_ptr,
    void          *buf_ptr,
    vint           size);

OSAL_Status _ISI_serverRpcReadIsiEvt(
    ISI_ServerObj *isi_ptr,
    void          *buf_ptr,
    vint           size,
    vint           timeout);

OSAL_Status _ISI_serverRpcWriteIsiEvt(
    ISI_ServerObj *isi_ptr,
    void          *buf_ptr,
    vint           size);

#ifdef __cplusplus
}
#endif


#endif /* __ISI_SERVER_RPC_H_ */
