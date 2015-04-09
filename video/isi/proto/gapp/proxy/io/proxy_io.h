/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2007 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 29631 $ $Date: 2014-11-03 15:42:57 +0800 (Mon, 03 Nov 2014) $
 */

#ifndef _PROXY_IO_H_
#define _PROXY_IO_H_

OSAL_Status PRXY_ioInit(
    const char *proxyName_ptr);

void PRXY_ioGetEventQueue(
    OSAL_MsgQId *queue_ptr);

void PRXY_ioDestroy(
    void);

vint PRXY_ioWrite(
    char    *buf_ptr,
    vint     size);

#endif
