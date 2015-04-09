/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 28424 $ $Date: 2014-08-22 12:07:37 +0800 (Fri, 22 Aug 2014) $
 *
 */

#include "_isi_port.h"
#include "_isi_mem.h"

int ISI_semMutexCreate(ISI_SemId *lock_ptr)
{
    *lock_ptr = OSAL_semMutexCreate();
    if (NULL == *lock_ptr) {
        return 0;
    }
    return 1;
}

int ISI_semDelete(ISI_SemId lock)
{
    if (OSAL_SUCCESS != OSAL_semDelete(lock)) {
        return 0;
    }
    return 1;
}

int ISI_semAcquire(ISI_SemId lock)
{
    if (OSAL_SUCCESS != OSAL_semAcquire(lock,
            OSAL_WAIT_FOREVER)) {
        return 0;
    }
    return 1;
}

int ISI_semGive(ISI_SemId lock)
{
    if (OSAL_SUCCESS != OSAL_semGive(lock)) {
        return 0;
    }
    return 1;
}
void* ISI_alloc(vint size, ISI_objType objType)
{
#ifdef ISI_DYNAMIC_MEMORY_ALLOC
        return OSAL_memAlloc(size, OSAL_MEM_ARG_DYNAMIC_ALLOC);
#else
        return ISIMP_memPoolAlloc(objType);
#endif
}

void ISI_free(void *mem_ptr, ISI_objType objType)
{
#ifdef ISI_DYNAMIC_MEMORY_ALLOC
    OSAL_memFree(mem_ptr, OSAL_MEM_ARG_DYNAMIC_ALLOC);
#else
    ISIMP_memPoolFree(objType, (ISIL_ListEntry *)mem_ptr);
#endif
}
