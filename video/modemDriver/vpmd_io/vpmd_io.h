/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004-2010 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Revision$ $Date$
 */

#ifndef _VPMD_IO_H_
#define _VPMD_IO_H_

#include <vpr_comm.h>
#define VPMD_IO_MESSAGE_SIZE_MAX             (sizeof(VPR_Comm))

#ifndef VPMD_IO_DEBUG
#define VPMD_ioDbgPrintf(fmt, args...)
#else
#define VPMD_ioDbgPrintf(fmt, args...) \
         OSAL_logMsg("[%s:%d] " fmt, __FUNCTION__, __LINE__, ## args)
#endif

typedef OSAL_Status (*VPMD_IoDataHandler)(
    void   *buf_ptr,
    vint    size);

typedef OSAL_Status (*VPMD_IoReadyHandler)(
    vint    status);

OSAL_Status VPMD_ioInit(
    VPMD_IoReadyHandler ioReadyHandler,
    VPMD_IoDataHandler  ioReadHandler);

void VPMD_ioDestroy(
        void);

/* VPMD interfaces for Video command/events write/read to/from VPAD */
OSAL_Status VPMD_ioWriteDevice(
    void   *buf_ptr,
    vint   *size_ptr);

#endif // _VPMD_IO_H_
