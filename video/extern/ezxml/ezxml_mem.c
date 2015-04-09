/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 28698 $ $Date: 2014-09-04 11:38:05 +0800 (Thu, 04 Sep 2014) $
 */

#include <osal.h>
#include "ezxml_mem.h"

static uint8 _EZXML_memPoolInitFlag = 0;

/* Static memroy pool list */
#ifndef EZXML_DEBUG_LOG
static EZXML_MemPool _ezMemPool[] = {
    {EZXML_MEM_TYPE_8, 8, EZXML_MEM_POOL_SZ_8,
        {NULL, NULL, 0, EZXML_PORT_LOCK_INIT, "EZXML_MemPool_8", 0}},
    {EZXML_MEM_TYPE_16, 16, EZXML_MEM_POOL_SZ_16,
        {NULL, NULL, 0, EZXML_PORT_LOCK_INIT, "EZXML_MemPool_16", 0}},
    {EZXML_MEM_TYPE_32, 32, EZXML_MEM_POOL_SZ_32,
        {NULL, NULL, 0, EZXML_PORT_LOCK_INIT, "EZXML_MemPool_32", 0}},
    {EZXML_MEM_TYPE_64, 64, EZXML_MEM_POOL_SZ_64,
        {NULL, NULL, 0, EZXML_PORT_LOCK_INIT, "EZXML_MemPool_64", 0}},
    {EZXML_MEM_TYPE_256, 256, EZXML_MEM_POOL_SZ_256,
        {NULL, NULL, 0, EZXML_PORT_LOCK_INIT, "EZXML_MemPool_256", 0}},
    {EZXML_MEM_TYPE_512, 512, EZXML_MEM_POOL_SZ_512,
        {NULL, NULL, 0, EZXML_PORT_LOCK_INIT, "EZXML_MemPool_512", 0}},
    {EZXML_MEM_TYPE_2048, 2048, EZXML_MEM_POOL_SZ_2048,
        {NULL, NULL, 0, EZXML_PORT_LOCK_INIT, "EZXML_MemPool_2048", 0}},
    {EZXML_MEM_TYPE_4096, 4096, EZXML_MEM_POOL_SZ_4096,
        {NULL, NULL, 0, EZXML_PORT_LOCK_INIT, "EZXML_MemPool_4096", 0}},
    {EZXML_MEM_TYPE_8192, 8192, EZXML_MEM_POOL_SZ_8192,
        {NULL, NULL, 0, EZXML_PORT_LOCK_INIT, "EZXML_MemPool_8192", 0}},
    {EZXML_MEM_TYPE_65536, 65536, EZXML_MEM_POOL_SZ_65536,
        {NULL, NULL, 0, EZXML_PORT_LOCK_INIT, "EZXML_MemPool_65536", 0}},
   };
#else
static EZXML_MemPool _ezMemPool[] = {
    {EZXML_MEM_TYPE_8, 8, EZXML_MEM_POOL_SZ_8,
        {NULL, NULL, 0, EZXML_PORT_LOCK_INIT, "EZXML_MemPool_8", 0}, 0, 0},
    {EZXML_MEM_TYPE_16, 16, EZXML_MEM_POOL_SZ_16,
        {NULL, NULL, 0, EZXML_PORT_LOCK_INIT, "EZXML_MemPool_16", 0}, 0, 0},
    {EZXML_MEM_TYPE_32, 32, EZXML_MEM_POOL_SZ_32,
        {NULL, NULL, 0, EZXML_PORT_LOCK_INIT, "EZXML_MemPool_32", 0}, 0, 0},
    {EZXML_MEM_TYPE_64, 64, EZXML_MEM_POOL_SZ_64,
        {NULL, NULL, 0, EZXML_PORT_LOCK_INIT, "EZXML_MemPool_64", 0}, 0, 0},
    {EZXML_MEM_TYPE_256, 256, EZXML_MEM_POOL_SZ_256,
        {NULL, NULL, 0, EZXML_PORT_LOCK_INIT, "EZXML_MemPool_256", 0}, 0, 0},
    {EZXML_MEM_TYPE_512, 512, EZXML_MEM_POOL_SZ_512,
        {NULL, NULL, 0, EZXML_PORT_LOCK_INIT, "EZXML_MemPool_512", 0}, 0, 0},
    {EZXML_MEM_TYPE_2048, 2048, EZXML_MEM_POOL_SZ_2048,
        {NULL, NULL, 0, EZXML_PORT_LOCK_INIT, "EZXML_MemPool_2048", 0}, 0, 0},
    {EZXML_MEM_TYPE_4096, 4096, EZXML_MEM_POOL_SZ_4096,
        {NULL, NULL, 0, EZXML_PORT_LOCK_INIT, "EZXML_MemPool_4096", 0}, 0, 0},
    {EZXML_MEM_TYPE_8192, 8192, EZXML_MEM_POOL_SZ_8192,
        {NULL, NULL, 0, EZXML_PORT_LOCK_INIT, "EZXML_MemPool_8192", 0}, 0, 0},
    {EZXML_MEM_TYPE_65536, 65536, EZXML_MEM_POOL_SZ_65536,
        {NULL, NULL, 0, EZXML_PORT_LOCK_INIT, "EZXML_MemPool_65536", 0}, 0, 0},
   };
#endif

/*
 * ======== EZXML_semMutexCreate() ========
 *
 * Create a mutex semaphore.
 *
 * Returns
 *  1 or 0
 */
int EZXML_semMutexCreate(EZXML_SemId *lock_ptr)
{
    *lock_ptr = OSAL_semMutexCreate();
    if (NULL == *lock_ptr) {
        return 0;
    }
    return 1;
}

/*
 * ======== EZXML_semDelete() ========
 *
 * Delete a mutex or a semaphore.
 *
 * Returns
 *  1 or 0
 */
int EZXML_semDelete(EZXML_SemId lock)
{
    if (OSAL_SUCCESS != OSAL_semDelete(lock)) {
        return 0;
    }
    return 1;
}

/*
 * ======== EZXML_semAcquire() ========
 *
 * Acquire a mutex or a semaphore.
 *
 * Returns
 *  0 (timeout, error) or 1 (aquired)
 */
int EZXML_semAcquire(EZXML_SemId lock)
{
    if (OSAL_SUCCESS != OSAL_semAcquire(lock,
            OSAL_WAIT_FOREVER)) {
        return 0;
    }
    return 1;
}
/*
 * ======== EZXML_semGive() ========
 *
 * Give a mutex or a semaphore.
 *
 * Returns
 *  1 or 0
 */
int EZXML_semGive(EZXML_SemId lock)
{
    if (OSAL_SUCCESS != OSAL_semGive(lock)) {
        return 0;
    }
    return 1;
}

/*
 * ======== EZXML_init() ========
 *
 * This function is to allocate whole required memory for ezxml stack.
 *
 * Returns:
 *     0 : Initilized successfully.
 *   -1 : Failed to initilized memory pool.
 */
vint EZXML_init(
    void)
{
#ifdef EZXML_DYNAMIC_MEMORY_ALLOC
    return;
#endif
    vint             poolIndex;
    vint             poolSize;
    vint             objSize;
    vint             size;
    EZXML_ListEntry *pEntry;
    EZXML_List      *pList;
#ifdef EZXML_DEBUG_LOG
    uvint            totalSize = 0;
#endif
    if (_EZXML_memPoolInitFlag) {
         return (0);
    }
    OSAL_logMsg("%s %d %s\n", __FILE__, __LINE__, __FUNCTION__);

    /* Allocate memory for EZXML objects pool list */
    for (poolIndex = EZXML_MEM_TYPE_FIRST; poolIndex < EZXML_MEM_TYPE_LAST;
            poolIndex++) {
        pList = &_ezMemPool[poolIndex].ezPoolList;
        objSize  = _ezMemPool[poolIndex].ezObjSize;
        poolSize = _ezMemPool[poolIndex].ezPoolSize;
        OSAL_logMsg("EZXML_init %s ezxmlPoolSize:%d\n",
                pList->listName, poolSize);
        EZXML_initList(pList);
        EZXML_semMutexCreate(&pList->lock);
        /* Allocate object entry and enqueue to pool list */
        for (size = 0; size < poolSize; size++) {
            /* Allocate memory from heap */
            pEntry = (EZXML_ListEntry *) OSAL_memAlloc(
                    objSize + sizeof(pEntry->u.e.objType),
                    OSAL_MEM_ARG_STATIC_ALLOC);
            if (NULL == pEntry) {
                OSAL_logMsg("EZXML_init failed!!! %s\n",
                        pList->listName);
                return (-1);
            }
            EZXML_initEntry(pEntry);
            EZXML_semAcquire(pList->lock);
            EZXML_enqueue(pList, pEntry);
            EZXML_semGive(pList->lock);
            
        }
        
#ifdef EZXML_DEBUG_LOG
        totalSize += poolSize * objSize;
        _ezMemPool[poolIndex].ezMemCurSz =
                _ezMemPool[poolIndex].ezMemMinSz = poolSize;
#endif
    }

#ifdef EZXML_DEBUG_LOG
    OSAL_logMsg("EZXML_init Total pre-allocated memory size:%d\n",
            (int)totalSize);
#endif
    _EZXML_memPoolInitFlag = 1;
    return (0);
}

/*
 * ======== EZXML_memPoolAlloc() ========
 *
 * This function is to allocate  EZXML (EZXML_ListEntry) from the pool list.
 *
 * objType:      The EZXML_memType to allocate.
 *
 * Returns:
 *   NULL        No free entry available.
 *   Otherwise   A pointer to the entry
 */
void* EZXML_memPoolAlloc(
    EZXML_memType objType)
{
    EZXML_ListEntry *pEntry;
    EZXML_List      *pList;

START_DEQUEUE:
    pList = &_ezMemPool[objType].ezPoolList;
    EZXML_semAcquire(pList->lock);
    pEntry = EZXML_dequeue(pList);
    EZXML_semGive(pList->lock);

    if (NULL == pEntry) {
        
#ifdef EZXML_DEBUG_LOG
        /* Keep track of the max ever attempted to be allocated */
        _ezMemPool[objType].ezMemMinSz--;

         OSAL_logMsg("%s: No free entry from %s ezMemMinSz:%d ezMemCurSz:%d\n",
                __FUNCTION__,
                pList->listName,
                _ezMemPool[objType].ezMemMinSz,
                _ezMemPool[objType].ezMemCurSz);
#endif
        /* Look for the next level memory type */
        if (EZXML_MEM_TYPE_LAST != objType) {
            objType++;
            goto START_DEQUEUE;
        }
        else {
            OSAL_logMsg("%s: No free entry from %s !!!!!!!!!!\n",
                    __FUNCTION__,
                    pList->listName);
            return (NULL);
        }
    }
#ifdef EZXML_DEBUG_LOG
    else {
        _ezMemPool[objType].ezMemCurSz--;
        /* keep track of the minimum size the pool ever gets to */
        if (_ezMemPool[objType].ezMemMinSz >
                _ezMemPool[objType].ezMemCurSz) {
            _ezMemPool[objType].ezMemMinSz = 
                    _ezMemPool[objType].ezMemCurSz;
        } 
        OSAL_logMsg("%s: %s  %p  ezMemCurSz:%d\n",
                __FUNCTION__,
                pList->listName,
                pEntry, _ezMemPool[objType].ezMemCurSz);
    }
#endif
    pEntry->u.e.objType = objType;
    return (void *)(&pEntry->u.e.data);
}

/*
 * ======== EZXML_memPoolFree() ========
 *
 * This function is to free a EZXML object(EZXML_ListEntry) to the memory pool list.
 *
 * objType:      The EZXML_memType to allocate.
 * pEntry        A pointer to the entry to free.
 *
 * Returns:
 *     None.
 */
void EZXML_memPoolFree(
    void *mem_ptr)
{
    EZXML_List      *pList;
    vint             objSize;
    EZXML_memType    objType;
    EZXML_ListEntry *pEntry = NULL;

    if (!mem_ptr) {
        return;
    }

    pEntry = (EZXML_ListEntry *)((int)mem_ptr - sizeof(pEntry->u.e.objType));
    objType = (EZXML_memType)pEntry->u.e.objType;
       
    if (EZXML_MEM_TYPE_LAST < objType) {
        return;
    }
    pList = &_ezMemPool[objType].ezPoolList;
    objSize  = _ezMemPool[objType].ezObjSize;

    EZXML_semAcquire(pList->lock);
    
    OSAL_memSet(pEntry , 0, objSize + sizeof(pEntry->u.e.objType));
    
    EZXML_initEntry(pEntry);
    EZXML_enqueue(pList, pEntry);
    EZXML_semGive(pList->lock);
#ifdef EZXML_DEBUG_LOG
    _ezMemPool[objType].ezMemCurSz++;
#endif
}

/*
 * ======== EZXML_memPoolDestroy() ========
 *
 * This function is to free all memory pool list to heap.
 *
 * Returns:
 *     None.
 */
void EZXML_destroy(
    void)
{
#ifdef EZXML_DYNAMIC_MEMORY_ALLOC
    return;
#endif
    vint             poolIndex;
    EZXML_List      *pList;
    EZXML_ListEntry *pEntry;
    vint             size;
#ifdef EZXML_DEBUG_LOG
    vint             poolSize;
#endif
    if (!_EZXML_memPoolInitFlag) {
        return;
    }
    OSAL_logMsg( "%s\n", __FUNCTION__);
    for (poolIndex = EZXML_MEM_TYPE_FIRST; poolIndex < EZXML_MEM_TYPE_LAST;
            poolIndex++) {
        pList = &_ezMemPool[poolIndex].ezPoolList;
#ifdef EZXML_DEBUG_LOG
        poolSize = _ezMemPool[poolIndex].ezPoolSize;
#endif
        size = 0;

        EZXML_semAcquire(pList->lock);
        /* Free all entries in the list */
        while ((NULL != (pEntry = EZXML_dequeue(pList)))) {
            /* Free memory to heap */
            OSAL_memFree(pEntry, OSAL_MEM_ARG_STATIC_ALLOC);
            size++;
        }
        EZXML_semGive(pList->lock);

#ifdef EZXML_DEBUG_LOG
        OSAL_logMsg("%s %s original poolSize:%d free size:%d\n",
                __FUNCTION__, pList->listName, poolSize, size);
        if (0 <= _ezMemPool[poolIndex].ezMemMinSz) {
            OSAL_logMsg("%s %s max usage:%d\n",
                     __FUNCTION__,
                    pList->listName,
                    (poolSize - _ezMemPool[poolIndex].ezMemMinSz));
        }
        else {
            OSAL_logMsg("%s %s max usage:%d, miss count:%d\n",
                    __FUNCTION__,
                    pList->listName,
                    poolSize, -_ezMemPool[poolIndex].ezMemMinSz);
        }
#endif
        /* Destroy mutex */
        EZXML_semDelete(pList->lock);
    }
    _EZXML_memPoolInitFlag = 0;
    return;
}

/*
 * ======== EZXML_memAlloc() ========
 *
 * This function is to allocate  memory .
 *
 * Returns:
 *   NULL        No free entry available.
 *   Otherwise   A pointer.
 */
void* EZXML_memAlloc(
    vint size,
    vint memArg)
{

#ifdef EZXML_DYNAMIC_MEMORY_ALLOC
    return (void *)OSAL_memAlloc(size, memArg);
#else
    EZXML_memType objType;
    int poolIndex;
    /* Get objType form Memory pool definition */
    objType = EZXML_MEM_TYPE_LAST;
    for (poolIndex = EZXML_MEM_TYPE_FIRST; poolIndex < EZXML_MEM_TYPE_LAST;
        poolIndex ++) {
        if (_ezMemPool[poolIndex].ezObjSize >= size) {
            objType = _ezMemPool[poolIndex].ezMemType;
            break;
        }
    }
    if (EZXML_MEM_TYPE_LAST == objType) {
        OSAL_logMsg("%s: size %d memory alloc failed, no objType!\n",
                __FUNCTION__, size);
        return (NULL);
    }
    return (void *)EZXML_memPoolAlloc(objType);
#endif
}

/*
 * ======== EZXML_memFree() ========
 *
 * This function is to freememory.
 * mem_ptr        A pointer to the entry to free.
 *
 * Returns:
 *     None.
 */
void EZXML_memFree(
    void *mem_ptr,
    vint memArg)
{
#ifdef EZXML_DYNAMIC_MEMORY_ALLOC
    OSAL_memFree(mem_ptr, memArg);
#else
     EZXML_memPoolFree(mem_ptr);
#endif
}
/*
 * ======== EZXML_memPoolReAlloc() ========
 *
 * This function is to reallocate  EZXML (EZXML_ListEntry) from the pool list.
 *
 *
 * Returns:
 *   NULL        No free entry available.
 *   Otherwise   A pointer to the entry
 */
void* EZXML_memPoolReAlloc(
    void  *mem_ptr,
    vint   size)
{
    EZXML_memType   objType;
    void           *new_mem_ptr;
    vint            oriSize;
    EZXML_ListEntry *pEntry = NULL;

    /* to get the real pEntry pointer which is including objType. */
    pEntry = (EZXML_ListEntry *)((int)mem_ptr - sizeof(pEntry->u.e.objType));
    objType = (EZXML_memType)pEntry->u.e.objType;
    
    oriSize = _ezMemPool[objType].ezObjSize;
    if (oriSize >= size) {
        /* size is enough, need not to realloc. */
        return (mem_ptr);
    }
    new_mem_ptr = EZXML_memAlloc(size, 0);
    /* Copy the data to new pointer. */
    OSAL_memCpy(new_mem_ptr, mem_ptr, oriSize);
    EZXML_memPoolFree((EZXML_ListEntry *)mem_ptr);
    return (new_mem_ptr);
}

/*
 * ======== EZXML_memReAlloc() ========
 *
 * This function is to reallocate memory.

 * Returns:
 *   NULL        No free entry available.
 *   Otherwise   A pointer to the entry
 */
void* EZXML_memReAlloc(
    void  *mem_ptr,
    vint   size,
    vint   memArg)
{
#ifdef EZXML_DYNAMIC_MEMORY_ALLOC
    return OSAL_memReAlloc(mem_ptr, size, memArg);
#else
    if (NULL == mem_ptr) {
        /* do not need to realloc, alloc it and return. */
        return (EZXML_memAlloc(size, memArg));
    }
    return (EZXML_memPoolReAlloc(mem_ptr, size));
#endif
}

/*
 * ======== EZXML_memLogReset() ========
 *
 * This function is to reset memory max usage for DEBUG log.

 * Returns:
 *   NONE
 */
 void EZXML_memLogReset(void)
{
#ifdef EZXML_DEBUG_LOG
    vint          poolSize = 0;
    vint     poolIndex = 0;
    EZXML_List *pList;
    
    poolSize = _ezMemPool[poolIndex].ezPoolSize;
    
    for (poolIndex = EZXML_MEM_TYPE_FIRST; poolIndex < EZXML_MEM_TYPE_LAST;
            poolIndex++) {
        pList = &_ezMemPool[poolIndex].ezPoolList;
        if (0 <= _ezMemPool[poolIndex].ezMemMinSz) {
            OSAL_logMsg("%s %s max usage:%d\n",
                    __FUNCTION__,
                    pList->listName,
                    (poolSize - _ezMemPool[poolIndex].ezMemMinSz));
        }
        else {
            OSAL_logMsg("%s %s max usage:%d, miss count:%d\n",
                    __FUNCTION__,
                    pList->listName,
                    poolSize, -_ezMemPool[poolIndex].ezMemMinSz);
        }
        _ezMemPool[poolIndex].ezMemMinSz =
                _ezMemPool[poolIndex].ezMemCurSz =
                _ezMemPool[poolIndex].ezPoolSize;
    }
    OSAL_logMsg("%s %d %s\n", __FILE__, __LINE__, __FUNCTION__);  
#else
    return;
#endif
}
/*
 * ======== EZXML_strdup() ========
 *
 * This function is to implement system call strdup().

 * Returns:
 *   NULL        No free entry available.
 *   Otherwise   A pointer to the entry
 */
char* EZXML_strdup(const char *s)  
{  
    size_t  len = OSAL_strlen(s) + 1;  
    void *new_ptr = EZXML_memAlloc(len, 0);  
    if (new_ptr == NULL) {
        return NULL;
    }
    OSAL_memCpy(new_ptr, s, len); 
    return (char *)(new_ptr);
}
