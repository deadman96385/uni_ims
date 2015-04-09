/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004-2012 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 19918 $ $Date: 2013-02-26 15:29:12 +0800 (Tue, 26 Feb 2013) $
 *
 */

#ifndef __ISI_CLIENT_H_
#define __ISI_CLIENT_H_

#ifdef __cplusplus
extern "C" {
#endif

#define _ISI_CLIENT_VERIFY_INIT           \
        if (NULL == _ISI_ClientObj_ptr) { \
            return (OSAL_FAIL);         \
        }

/* ISI Client object. */
typedef struct {
    OSAL_MsgQId isiRpcWriteQ;
    OSAL_MsgQId isiRpcReadQ;
    OSAL_MsgQId isiEvtRpcWriteQ;
    OSAL_MsgQId isiEvtRpcReadQ;
} ISI_ClientObj;

ISI_Return _ISI_clientInit(ISI_ClientObj **_ISI_ClientObj_ptr);
ISI_Return _ISI_clientShutdown(ISI_ClientObj *_ISI_ClientObj_ptr);

#endif
