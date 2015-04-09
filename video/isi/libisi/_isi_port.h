/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 27660 $ $Date: 2014-07-21 13:26:30 +0800 (Mon, 21 Jul 2014) $
 *
 */

#ifndef __ISI_PORT_H__
#define __ISI_PORT_H__

#include <osal.h>

typedef OSAL_SemId ISI_SemId;

typedef enum {
    ISI_OBJECT_FIRST,
    ISI_OBJECT_ISIP_MSG = ISI_OBJECT_FIRST,
    ISI_OBJECT_ACCOUNT,
    ISI_OBJECT_TEXT_ID,
    ISI_OBJECT_FILE_ID,
    ISI_OBJECT_EVT_ID,
    ISI_OBJECT_PRES_ID,
    ISI_OBJECT_CONF_ID,
    ISI_OBJECT_SERVICE_ID,
    ISI_OBJECT_CALL_ID,
    ISI_OBJECT_GCHAT_ID,
    ISI_OBJECT_USSD_ID,
    ISI_OBJECT_NONE,
    ISI_OBJECT_LAST = ISI_OBJECT_NONE
}ISI_objType;

int  ISI_semMutexCreate(ISI_SemId *lock_ptr);

int  ISI_semDelete(ISI_SemId lock);

int  ISI_semAcquire(ISI_SemId lock);

int  ISI_semGive(ISI_SemId lock);

void* ISI_alloc(vint size, ISI_objType objType);

void  ISI_free(void *mem_ptr, ISI_objType objType);

#endif
