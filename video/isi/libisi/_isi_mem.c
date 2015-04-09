/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 24709 $ $Date: 2014-02-20 17:27:12 +0800 (Thu, 20 Feb 2014) $
 */

#include <osal.h>
#include "_isi_mem.h"
#include "isi.h"
#include "isip.h"
#include "_isi_db.h"
/* Static memroy pool list */
#ifndef ISI_MEM_DEBUG_LOG
static ISIMP_MemPool _isiMemPool[] = {
    {ISI_OBJECT_ISIP_MSG, sizeof(ISIP_Message), ISI_MEM_POOL_SZ_ISIP_MSG,
        {NULL, NULL, 0, ISI_PORT_LOCK_INIT, "ISIP_MessagePool", 0}},
    {ISI_OBJECT_ACCOUNT, sizeof(ISID_Account), ISI_MEM_POOL_SZ_ACCOUNT,
        {NULL, NULL, 0, ISI_PORT_LOCK_INIT, "ISID_AccountPool", 0}},
    {ISI_OBJECT_TEXT_ID, sizeof(ISID_TextId), ISI_MEM_POOL_SZ_TEXT_ID,
        {NULL, NULL, 0, ISI_PORT_LOCK_INIT, "ISID_TextIdPool", 0}},
    {ISI_OBJECT_FILE_ID, sizeof(ISID_FileId), ISI_MEM_POOL_SZ_FILE_ID,
        {NULL, NULL, 0, ISI_PORT_LOCK_INIT, "ISID_FileIdPool", 0}},
    {ISI_OBJECT_EVT_ID, sizeof(ISID_EvtId), ISI_MEM_POOL_SZ_EVT_ID,
        {NULL, NULL, 0, ISI_PORT_LOCK_INIT, "ISID_EvtIdPool", 0}},
    {ISI_OBJECT_PRES_ID, sizeof(ISID_PresId), ISI_MEM_POOL_SZ_PRES_ID,
        {NULL, NULL, 0, ISI_PORT_LOCK_INIT, "ISID_PresIdPool", 0}},
    {ISI_OBJECT_CONF_ID, sizeof(ISID_ConfId), ISI_MEM_POOL_SZ_CONF_ID,
        {NULL, NULL, 0, ISI_PORT_LOCK_INIT, "ISID_ConfIdPool", 0}},
    {ISI_OBJECT_SERVICE_ID, sizeof(ISID_ServiceId), ISI_MEM_POOL_SZ_SERVICE_ID,
        {NULL, NULL, 0, ISI_PORT_LOCK_INIT, "ISID_ServiceIdPool", 0}},
    {ISI_OBJECT_CALL_ID, sizeof(ISID_CallId), ISI_MEM_POOL_SZ_CALL_ID,
        {NULL, NULL, 0, ISI_PORT_LOCK_INIT, "ISID_CallIdPool", 0}},
    {ISI_OBJECT_GCHAT_ID, sizeof(ISID_GChatId), ISI_MEM_POOL_SZ_GCHAT_ID,
        {NULL, NULL, 0, ISI_PORT_LOCK_INIT, "ISID_GChatIdPool", 0}},
    {ISI_OBJECT_USSD_ID, sizeof(ISID_UssdId), ISI_MEM_POOL_SZ_USSD_ID,
        {NULL, NULL, 0, ISI_PORT_LOCK_INIT, "ISID_UssdIdPool", 0}},
   };
#else
static ISIMP_MemPool _isiMemPool[] = {
    {ISI_OBJECT_ISIP_MSG, sizeof(ISIP_Message), ISI_MEM_POOL_SZ_ISIP_MSG,
        {NULL, NULL, 0, ISI_PORT_LOCK_INIT, "ISIP_MessagePool", 0}, 0, 0},
    {ISI_OBJECT_ACCOUNT, sizeof(ISID_Account), ISI_MEM_POOL_SZ_ACCOUNT,
        {NULL, NULL, 0, ISI_PORT_LOCK_INIT, "ISID_AccountPool", 0}, 0, 0},
    {ISI_OBJECT_TEXT_ID, sizeof(ISID_TextId), ISI_MEM_POOL_SZ_TEXT_ID,
        {NULL, NULL, 0, ISI_PORT_LOCK_INIT, "ISID_TextIdPool", 0}, 0, 0},
    {ISI_OBJECT_FILE_ID, sizeof(ISID_FileId), ISI_MEM_POOL_SZ_FILE_ID,
        {NULL, NULL, 0, ISI_PORT_LOCK_INIT, "ISID_FileIdPool", 0}, 0, 0},
    {ISI_OBJECT_EVT_ID, sizeof(ISID_EvtId), ISI_MEM_POOL_SZ_EVT_ID,
        {NULL, NULL, 0, ISI_PORT_LOCK_INIT, "ISID_EvtIdPool", 0}, 0, 0},
    {ISI_OBJECT_PRES_ID, sizeof(ISID_PresId), ISI_MEM_POOL_SZ_PRES_ID,
        {NULL, NULL, 0, ISI_PORT_LOCK_INIT, "ISID_PresIdPool", 0}, 0, 0},
    {ISI_OBJECT_CONF_ID, sizeof(ISID_ConfId), ISI_MEM_POOL_SZ_CONF_ID,
        {NULL, NULL, 0, ISI_PORT_LOCK_INIT, "ISID_ConfIdPool", 0}, 0, 0},
    {ISI_OBJECT_SERVICE_ID, sizeof(ISID_ServiceId), ISI_MEM_POOL_SZ_SERVICE_ID,
        {NULL, NULL, 0, ISI_PORT_LOCK_INIT, "ISID_ServiceIdPool", 0}, 0, 0},
    {ISI_OBJECT_CALL_ID, sizeof(ISID_CallId), ISI_MEM_POOL_SZ_CALL_ID,
        {NULL, NULL, 0, ISI_PORT_LOCK_INIT, "ISID_CallIdPool", 0}, 0, 0},
    {ISI_OBJECT_GCHAT_ID, sizeof(ISID_GChatId), ISI_MEM_POOL_SZ_GCHAT_ID,
        {NULL, NULL, 0, ISI_PORT_LOCK_INIT, "ISID_GChatIdPool", 0}, 0, 0},
    {ISI_OBJECT_USSD_ID, sizeof(ISID_UssdId), ISI_MEM_POOL_SZ_USSD_ID,
        {NULL, NULL, 0, ISI_PORT_LOCK_INIT, "ISID_UssdIdPool", 0}, 0, 0},
   };

#endif
/*
 * ======== ISIMP_memPoolInit() ========
 *
 * This function is to allocate whole required memory for ISI stack.
 *
 * Returns:
 *   ISI_RETURN_OK          Initilized successfully.
 *   ISI_RETURN_NO_MEM  Failed to initilized memory pool.
 */
vint ISIMP_memPoolInit(
    void)
{
    vint             poolIndex;
    vint             poolSize;
    vint             objSize;
    vint             size;
    ISIL_ListEntry *pEntry;
    ISIL_List      *pList;
#ifdef ISI_MEM_DEBUG_LOG
    uvint            totalSize = 0;
#endif

    OSAL_logMsg("%s %d ISIMP_memPoolInit\n", __FILE__, __LINE__);

    /* Allocate memory for ISI objects pool list */
    for (poolIndex = ISI_OBJECT_FIRST; poolIndex < ISI_OBJECT_LAST;
            poolIndex++) {
        pList = &_isiMemPool[poolIndex].isiPoolList;
        objSize  = _isiMemPool[poolIndex].isiObjSize;
        poolSize = _isiMemPool[poolIndex].isiPoolSize;
        OSAL_logMsg("ISIMP_memPoolInit %s isiObjSize:%d isiPoolSize:%d\n",
                pList->listName, objSize, poolSize);
        ISIL_initList(pList);
        ISI_semMutexCreate(&pList->lock);
        /* Allocate object entry and enqueue to pool list */
        for (size = 0; size < poolSize; size++) {
            /* Allocate memory from heap */
            pEntry = (ISIL_ListEntry *) OSAL_memAlloc(objSize,
                OSAL_MEM_ARG_STATIC_ALLOC);
            if (NULL == pEntry) {
                OSAL_logMsg("ISIMP_memPoolInit failed!!! %s\n",
                        pList->listName);
                return (ISI_RETURN_NO_MEM);
            }
            ISIL_initEntry(pEntry);
            ISI_semAcquire(pList->lock);
            ISIL_enqueue(pList, pEntry);
            ISI_semGive(pList->lock);
        }
#ifdef ISI_MEM_DEBUG_LOG
        totalSize += poolSize * objSize;
        _isiMemPool[poolIndex].isiMemCurSz =
                _isiMemPool[poolIndex].isiMemMinSz =
                poolSize;
#endif
    }

#ifdef ISI_MEM_DEBUG_LOG
    OSAL_logMsg("ISIMP_memPoolInit Total pre-allocated memory size:%d\n",
            (int)totalSize);
#endif
    
    return (ISI_RETURN_OK);
}

/*
 * ======== ISIMP_memPoolAlloc() ========
 *
 * This function is to allocate a ISI object(ISIL_ListEntry) from the pool list.
 *
 * objType:      The ISI_objType to allocate.
 *
 * Returns:
 *   NULL        No free entry available.
 *   Otherwise   A pointer to the entry
 */
void *ISIMP_memPoolAlloc(
    ISI_objType objType)
{
    ISIL_ListEntry *pEntry;
    ISIL_List      *pList;

    /* Check the objType is valid. */
    if (objType >= ISI_OBJECT_LAST) {
         OSAL_logMsg("ISIMP_memPoolAlloc: objType is %d\n", (int)objType);
       return NULL;
    }

    pList = &_isiMemPool[objType].isiPoolList;

    ISI_semAcquire(pList->lock);
    pEntry = ISIL_dequeue(pList);
    ISI_semGive(pList->lock);

    if (NULL == pEntry) {
#ifdef ISI_MEM_DEBUG_LOG
        /* Keep track of the max ever attempted to be allocated */
        _isiMemPool[objType].isiMemMinSz--;

         OSAL_logMsg("%s: No free entry from %s isiMemMinSz:%d isiMemCurSz:%d\n",
                __FUNCTION__,
                pList->listName,
                _isiMemPool[objType].isiMemMinSz,
                _isiMemPool[objType].isiMemCurSz);
#endif

        OSAL_logMsg("ISIMP_memPoolAlloc: No free entry from %s\n",
                pList->listName);
    }
#ifdef ISI_MEM_DEBUG_LOG
    else {
        _isiMemPool[objType].isiMemCurSz--;
        /* keep track of the minimum size the pool ever gets to */
        if (_isiMemPool[objType].isiMemMinSz >
                _isiMemPool[objType].isiMemCurSz) {
            _isiMemPool[objType].isiMemMinSz = 
                    _isiMemPool[objType].isiMemCurSz;
        } 
        OSAL_logMsg("ISIMP_memPoolAlloc: %s  %p  isiMemCurSz:%d\n",
                pList->listName,
                pEntry, _isiMemPool[objType].isiMemCurSz);

    }
#endif
    return (pEntry);
}

/*
 * ======== ISIMP_memPoolFree() ========
 *
 * This function is to free a ISI object(isiDLListEntry) to the memory pool list.
 *
 * objType:      The ISI_objType to allocate.
 * pEntry        A pointer to the entry to free.
 *
 * Returns:
 *     None.
 */
void ISIMP_memPoolFree(
    ISI_objType   objType,
    ISIL_ListEntry *pEntry)
{
    ISIL_List        *pList;
    vint            objSize;

    pList = &_isiMemPool[objType].isiPoolList;
    objSize  = _isiMemPool[objType].isiObjSize;

    ISI_semAcquire(pList->lock);
    
    OSAL_memSet(pEntry, 0, objSize);
    
    ISIL_initEntry(pEntry);
    ISIL_enqueue(pList, pEntry);
    ISI_semGive(pList->lock);
#ifdef ISI_MEM_DEBUG_LOG
    _isiMemPool[objType].isiMemCurSz++;
#endif
}

/*
 * ======== ISIMP_memPoolDestroy() ========
 *
 * This function is to free all memory pool list to heap.
 *
 * Returns:
 *     None.
 */
void ISIMP_memPoolDestroy(
    void)
{
    vint     poolIndex;
    ISIL_List *pList;
    ISIL_ListEntry *pEntry;
    vint          size;
#ifdef ISI_MEM_DEBUG_LOG
    vint          poolSize;
#endif

    OSAL_logMsg( "ISIMP_memPoolDestroy\n");
    for (poolIndex = ISI_OBJECT_FIRST; poolIndex < ISI_OBJECT_LAST;
            poolIndex++) {
        pList = &_isiMemPool[poolIndex].isiPoolList;
#ifdef ISI_MEM_DEBUG_LOG
        poolSize = _isiMemPool[poolIndex].isiPoolSize;
#endif
        size = 0;

        ISI_semAcquire(pList->lock);
        /* Free all entries in the list */
        while ((NULL != (pEntry = ISIL_dequeue(pList)))) {
            /* Free memory to heap */
            OSAL_memFree(pEntry, OSAL_MEM_ARG_STATIC_ALLOC);
            size++;
        }
        ISI_semGive(pList->lock);

#ifdef ISI_MEM_DEBUG_LOG
        OSAL_logMsg("ISIMP_memPoolDestroy %s original poolSize:%d free size:%d\n",
        pList->listName, poolSize, size);
        if (0 <= _isiMemPool[poolIndex].isiMemMinSz) {
            OSAL_logMsg("ISIMP_memPoolDestroy %s max usage:%d\n",
                    pList->listName,
                    (poolSize - _isiMemPool[poolIndex].isiMemMinSz));
        }
        else {
            OSAL_logMsg("ISIMP_memPoolDestroy %s max usage:%d, miss count:%d\n",
                    pList->listName,
                    poolSize, -_isiMemPool[poolIndex].isiMemMinSz);
        }
#endif
        /* Destroy mutex */
        ISI_semDelete(pList->lock);
    }

    return;
}
