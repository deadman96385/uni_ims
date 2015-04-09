/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 29950 $ $Date: 2014-11-20 09:42:47 +0800 (Thu, 20 Nov 2014) $
 */

#include "sip_sip.h"
#include "sip_sdp_msg.h"
#include "sip_port.h"
#include "sip_mem.h"
#include "sip_mem_pool.h"
#include "sip_timers.h"
#include "sip_session.h"
#include "sip_dialog.h"
#include "sip_xact.h"
#include "sip_dbase_sys.h"
#include "sip_dbase_endpt.h"
#include "sip_xport.h"
#include "../xport/_sip_descr.h"
#include "../xport/_sip_drvr.h"
#include "sip_ua.h"
#include "../ua/_sip_helpers.h"

/* Static memroy pool list */
#ifndef SIP_DEBUG_LOG
static tSipMemPool _sipMemPool[] = {
    {eSIP_OBJECT_TRANS, sizeof(tTrans), SIP_MEM_POOL_SZ_TRANS,
        {NULL, NULL, 0, SIP_PORT_LOCK_INIT, "XactMemPool", 0}},
    {eSIP_OBJECT_TIMER, sizeof(tSipTimerEntry), SIP_MEM_POOL_SZ_TIMER,
        {NULL, NULL, 0, SIP_PORT_LOCK_INIT, "TimerMemPool", 0}},
    {eSIP_OBJECT_SIP_INT_MSG, sizeof(tSipIntMsg), SIP_MEM_POOL_SZ_SIP_INT_MSG,
        {NULL, NULL, 0, SIP_PORT_LOCK_INIT, "SipIntMsgMemPool", 0}},
    {eSIP_OBJECT_SDP_MSG, sizeof(tSdpMsg), SIP_MEM_POOL_SZ_SDP_MSG,
        {NULL, NULL, 0, SIP_PORT_LOCK_INIT, "SdpMsgMemPool", 0}},
    {eSIP_OBJECT_SDP_CONN_INFO, sizeof(tSdpConnInfo), SIP_MEM_POOL_SZ_SDP_CONN_INFO,
        {NULL, NULL, 0, SIP_PORT_LOCK_INIT, "SdpConnInfoPool", 0}},
    {eSIP_OBJECT_SDP_MEDIA, sizeof(tSdpMedia), SIP_MEM_POOL_SZ_SDP_MEDIA,
        {NULL, NULL, 0, SIP_PORT_LOCK_INIT, "SdpMediaPool", 0}},
    {eSIP_OBJECT_ATTRIBUTE, sizeof(tAttribute), SIP_MEM_POOL_SZ_ATTRIBUTE,
        {NULL, NULL, 0, SIP_PORT_LOCK_INIT, "SdpAttrPool", 0}},
    {eSIP_OBJECT_SIP_TEXT, sizeof(tSipText), SIP_MEM_POOL_SZ_SIP_TEXT,
        {NULL, NULL, 0, SIP_PORT_LOCK_INIT, "SipTextMemPool", 0}},
    {eSIP_OBJECT_SIP_MSG_BODY, sizeof(tSipMsgBody), SIP_MEM_POOL_SZ_SIP_MSG_BODY,
        {NULL, NULL, 0, SIP_PORT_LOCK_INIT, "SipMsgBodyMemPool", 0}},
    {eSIP_OBJECT_HF_LIST, sizeof(tHdrFldList), SIP_MEM_POOL_SZ_HF_LIST,
        {NULL, NULL, 0, SIP_PORT_LOCK_INIT, "HFListMemPool", 0}},
    {eSIP_OBJECT_VIA_HF, sizeof(tViaHFE), SIP_MEM_POOL_SZ_VIA_HF,
        {NULL, NULL, 0, SIP_PORT_LOCK_INIT, "ViaHFMemPool", 0}},
    {eSIP_OBJECT_CONTACT_HF, sizeof(tContactHFE), SIP_MEM_POOL_SZ_CONTACT_HF,
        {NULL, NULL, 0, SIP_PORT_LOCK_INIT, "ContactHFMemPool", 0}},
    {eSIP_OBJECT_AUTH_HF, sizeof(tAuthorizationHFE), SIP_MEM_POOL_SZ_AUTH_HF,
        {NULL, NULL, 0, SIP_PORT_LOCK_INIT, "AuthHFMemPool", 0}},
    {eSIP_OBJECT_ROUTE_HF, sizeof(tRouteHFE), SIP_MEM_POOL_SZ_ROUTE_HF,
        {NULL, NULL, 0, SIP_PORT_LOCK_INIT, "RouteHFMemPool", 0}},
    {eSIP_OBJECT_AUTH_CRED, sizeof(tAUTH_Cred), SIP_MEM_POOL_SZ_AUTH_CRED,
        {NULL, NULL, 0, SIP_PORT_LOCK_INIT, "AuthCredMemPool", 0}},
    {eSIP_OBJECT_DIALOG_EVENT, sizeof(tDialogEvent), SIP_MEM_POOL_SZ_DIALOG_EVENT,
        {NULL, NULL, 0, SIP_PORT_LOCK_INIT, "DialogEventMemPool", 0}},
    {eSIP_OBJECT_UA_TRANS, sizeof(tUaTrans), SIP_MEM_POOL_SZ_UA_TRANS,
        {NULL, NULL, 0, SIP_PORT_LOCK_INIT, "UATransMemPool", 0}},
    {eSIP_OBJECT_LAYER_4_BUF, sizeof(tLayer4Buffer), SIP_MEM_POOL_SZ_LAYER_4_BUF,
        {NULL, NULL, 0, SIP_PORT_LOCK_INIT, "Layer4BufMemPool", 0}},
    {eSIP_OBJECT_TRANSPORT, sizeof(tTransport), SIP_MEM_POOL_SZ_TRANSPORT,
        {NULL, NULL, 0, SIP_PORT_LOCK_INIT, "TransportMemPool", 0}},
    {eSIP_OBJECT_URI, sizeof(tUri), SIP_MEM_POOL_SZ_URI,
        {NULL, NULL, 0, SIP_PORT_LOCK_INIT, "UriMemPool", 0}},
    {eSIP_OBJECT_UA, sizeof(tUa), SIP_MEM_POOL_SZ_UA,
        {NULL, NULL, 0, SIP_PORT_LOCK_INIT, "UaMemPool", 0}},
};
#else
static tSipMemPool _sipMemPool[] = {
    {eSIP_OBJECT_TRANS, sizeof(tTrans), SIP_MEM_POOL_SZ_TRANS,
        {NULL, NULL, 0, SIP_PORT_LOCK_INIT, "XactMemPool", 0}, 0, 0},
    {eSIP_OBJECT_TIMER, sizeof(tSipTimerEntry), SIP_MEM_POOL_SZ_TIMER,
        {NULL, NULL, 0, SIP_PORT_LOCK_INIT, "TimerMemPool", 0}, 0, 0},
    {eSIP_OBJECT_SIP_INT_MSG, sizeof(tSipIntMsg), SIP_MEM_POOL_SZ_SIP_INT_MSG,
        {NULL, NULL, 0, SIP_PORT_LOCK_INIT, "SipIntMsgMemPool", 0}, 0, 0},
    {eSIP_OBJECT_SDP_MSG, sizeof(tSdpMsg), SIP_MEM_POOL_SZ_SDP_MSG,
        {NULL, NULL, 0, SIP_PORT_LOCK_INIT, "SdpMsgMemPool", 0}, 0, 0},
    {eSIP_OBJECT_SDP_CONN_INFO, sizeof(tSdpConnInfo), SIP_MEM_POOL_SZ_SDP_CONN_INFO,
        {NULL, NULL, 0, SIP_PORT_LOCK_INIT, "SdpConnInfoPool", 0}, 0, 0},
    {eSIP_OBJECT_SDP_MEDIA, sizeof(tSdpMedia), SIP_MEM_POOL_SZ_SDP_MEDIA,
        {NULL, NULL, 0, SIP_PORT_LOCK_INIT, "SdpMediaPool", 0}, 0, 0},
    {eSIP_OBJECT_ATTRIBUTE, sizeof(tAttribute), SIP_MEM_POOL_SZ_ATTRIBUTE,
        {NULL, NULL, 0, SIP_PORT_LOCK_INIT, "SdpAttrPool", 0}, 0, 0},
    {eSIP_OBJECT_SIP_TEXT, sizeof(tSipText), SIP_MEM_POOL_SZ_SIP_TEXT,
        {NULL, NULL, 0, SIP_PORT_LOCK_INIT, "SipTextMemPool", 0}, 0, 0},
    {eSIP_OBJECT_SIP_MSG_BODY, sizeof(tSipMsgBody), SIP_MEM_POOL_SZ_SIP_MSG_BODY,
        {NULL, NULL, 0, SIP_PORT_LOCK_INIT, "SipMsgBodyMemPool", 0}, 0, 0},
    {eSIP_OBJECT_HF_LIST, sizeof(tHdrFldList), SIP_MEM_POOL_SZ_HF_LIST,
        {NULL, NULL, 0, SIP_PORT_LOCK_INIT, "HFListMemPool", 0}, 0, 0},
    {eSIP_OBJECT_VIA_HF, sizeof(tViaHFE), SIP_MEM_POOL_SZ_VIA_HF,
        {NULL, NULL, 0, SIP_PORT_LOCK_INIT, "ViaHFMemPool", 0}, 0, 0},
    {eSIP_OBJECT_CONTACT_HF, sizeof(tContactHFE), SIP_MEM_POOL_SZ_CONTACT_HF,
        {NULL, NULL, 0, SIP_PORT_LOCK_INIT, "ContactHFMemPool", 0}, 0, 0},
    {eSIP_OBJECT_AUTH_HF, sizeof(tAuthorizationHFE), SIP_MEM_POOL_SZ_AUTH_HF,
        {NULL, NULL, 0, SIP_PORT_LOCK_INIT, "AuthHFMemPool", 0}, 0, 0},
    {eSIP_OBJECT_ROUTE_HF, sizeof(tRouteHFE), SIP_MEM_POOL_SZ_ROUTE_HF,
        {NULL, NULL, 0, SIP_PORT_LOCK_INIT, "RouteHFMemPool", 0}, 0, 0},
    {eSIP_OBJECT_AUTH_CRED, sizeof(tAUTH_Cred), SIP_MEM_POOL_SZ_AUTH_CRED,
        {NULL, NULL, 0, SIP_PORT_LOCK_INIT, "AuthCredMemPool", 0}, 0, 0},
    {eSIP_OBJECT_DIALOG_EVENT, sizeof(tDialogEvent), SIP_MEM_POOL_SZ_DIALOG_EVENT,
        {NULL, NULL, 0, SIP_PORT_LOCK_INIT, "DialogEventMemPool", 0}, 0, 0},
    {eSIP_OBJECT_UA_TRANS, sizeof(tUaTrans), SIP_MEM_POOL_SZ_UA_TRANS,
        {NULL, NULL, 0, SIP_PORT_LOCK_INIT, "UATransMemPool", 0}, 0, 0},
    {eSIP_OBJECT_LAYER_4_BUF, sizeof(tLayer4Buffer), SIP_MEM_POOL_SZ_LAYER_4_BUF,
        {NULL, NULL, 0, SIP_PORT_LOCK_INIT, "Layer4BufMemPool", 0}, 0, 0},
    {eSIP_OBJECT_TRANSPORT, sizeof(tTransport), SIP_MEM_POOL_SZ_TRANSPORT,
        {NULL, NULL, 0, SIP_PORT_LOCK_INIT, "TransportMemPool", 0}, 0, 0},
    {eSIP_OBJECT_URI, sizeof(tUri), SIP_MEM_POOL_SZ_URI,
        {NULL, NULL, 0, SIP_PORT_LOCK_INIT, "UriMemPool", 0}, 0, 0},
    {eSIP_OBJECT_UA, sizeof(tUa), SIP_MEM_POOL_SZ_UA,
        {NULL, NULL, 0, SIP_PORT_LOCK_INIT, "UaMemPool", 0}, 0, 0},
};
#endif

/*
 * ======== SIP_memPoolInit() ========
 *
 * This function is to allocate whole required memory for sip stack.
 *
 * Returns:
 *   SIP_OK      Initilized successfully.
 *   SIP_NO_MEM  Failed to initilized memory pool.
 */
vint SIP_memPoolInit(
    void)
{
#ifdef SIP_DYNAMIC_MEMORY_ALLOC
    return (SIP_OK);
#else

    vint          poolIndex;
    vint          poolSize;
    vint          objSize;
    vint          size;
    tDLListEntry *pEntry;
    tDLList      *pList;
#ifdef SIP_DEBUG_LOG
    uvint         totalSize = 0;
#endif

    SIP_DebugLog(SIP_DB_MEMORY_LVL_2, "SIP_memPoolInit\n", 0, 0, 0);

    /* Allocate memory for SIP objects pool list */
    for (poolIndex = eSIP_OBJECT_FIRST; poolIndex < eSIP_OBJECT_LAST; poolIndex++) {
        pList = &_sipMemPool[poolIndex].poolList;
        objSize  = _sipMemPool[poolIndex].objSize;
        poolSize = _sipMemPool[poolIndex].poolSize;
        SIP_DebugLog(SIP_DB_MEMORY_LVL_2,
                "SIP_memPoolInit %s objSize:%d poolSize:%d\n",
                (int)pList->listName, objSize, poolSize);
        DLLIST_InitList(pList);
        SIP_MutexInit(&pList->lock);
        /* Allocate object entry and enqueue to pool list */
        for (size = 0; size < poolSize; size++) {
            /* Allocate memory from heap */
            pEntry = (tDLListEntry *) SIP_malloc(objSize);
            if (NULL == pEntry) {
                SIP_DebugLog(SIP_DB_MEMORY_LVL_1,
                        "SIP_memPoolInit failed!!! %s\n",
                        (int)pList->listName, 0, 0);
                return (SIP_NO_MEM);
            }
            /* Clean up memory */
            OSAL_memSet(pEntry, 0, objSize);
            /* Create OSAL timer for each tSipTimerEntry */
            if (eSIP_OBJECT_TIMER == poolIndex) {
                if (0 == (((tSipTimerEntry *)pEntry)->tmrId =
                            OSAL_tmrCreate())) {
                    SIP_DebugLog(SIP_DB_MEMORY_LVL_1,
                            "SIP_memPoolInit %s OSAL timer create failed!!!\n",
                            (int)pList->listName, 0, 0);
                    return (SIP_NO_MEM);
                }
                if (0 == (((tSipTimerEntry *)pEntry)->retryTmrId =
                        OSAL_tmrCreate())) {
                    SIP_DebugLog(SIP_DB_MEMORY_LVL_1,
                            "SIP_memPoolInit %s OSAL timer create failed!!!\n",
                            (int)pList->listName, 0, 0);
                    return (SIP_NO_MEM);
                }
            }
            DLLIST_InitEntry(pEntry);
            SIP_Lock(pList->lock);
            DLLIST_Enqueue(pList, pEntry);
            SIP_Unlock(pList->lock);
        }
#ifdef SIP_DEBUG_LOG
        totalSize += poolSize * objSize;
        _sipMemPool[poolIndex].curSize = _sipMemPool[poolIndex].minSize =
                poolSize;
#endif
    }

#ifdef SIP_DEBUG_LOG
    SIP_DebugLog(SIP_DB_MEMORY_LVL_2,
            "SIP_memPoolInit Total pre-allocated memory size:%d\n",
            (int)totalSize, 0, 0);
#endif
    
    return (SIP_OK);
#endif
}

/*
 * ======== SIP_memPoolAlloc() ========
 *
 * This function is to allocate a SIP object(tDLListEntry) from the pool list.
 *
 * objType:      The tSipObjType to allocate.
 *
 * Returns:
 *   NULL        No free entry available.
 *   Otherwise   A pointer to the entry
 */
void *SIP_memPoolAlloc(
    tSipObjType objType)
{
#ifdef SIP_DYNAMIC_MEMORY_ALLOC
    void                *entry_ptr;
    if (NULL == (entry_ptr = OSAL_memAlloc(_sipMemPool[objType].objSize, 0))) {
        return (NULL);    
    }
    OSAL_memSet(entry_ptr, 0, _sipMemPool[objType].objSize);

    if (eSIP_OBJECT_TIMER == objType){
        if (NULL == (((tSipTimerEntry *)entry_ptr)->tmrId = OSAL_tmrCreate())) {
            OSAL_memFree(entry_ptr, 0);
            return (NULL);
        }
        if (NULL == (((tSipTimerEntry *)entry_ptr)->retryTmrId =
                OSAL_tmrCreate())) {
            OSAL_memFree(entry_ptr, 0);
            return (NULL);
        }
        return (void *)(entry_ptr);
    }
    
    return (entry_ptr);
#else
    tDLListEntry *pEntry;
    tDLList      *pList;

    pList = &_sipMemPool[objType].poolList;

    SIP_Lock(pList->lock);
    pEntry = DLLIST_Dequeue(pList);
    SIP_Unlock(pList->lock);

    if (NULL == pEntry) {
#ifdef SIP_DEBUG_LOG
        /* Keep track of the max ever attempted to be allocated */
        _sipMemPool[objType].minSize--;

        SIP_DebugLog(SIP_DB_MEMORY_LVL_3,
                "SIP_memPoolAlloc: No free entry from %s  minSize:%d curSize:%d\n",
                (int)pList->listName,
                (int)_sipMemPool[objType].minSize, (int)_sipMemPool[objType].curSize);
#endif

        SIP_DebugLog(SIP_DB_MEMORY_LVL_1,
                "SIP_memPoolAlloc: No free entry from %s\n",
                (int)pList->listName, 0, 0);
    }
#ifdef SIP_DEBUG_LOG
    else {
        _sipMemPool[objType].curSize--;
        /* keep track of the minimum size the pool ever gets to */
        if (_sipMemPool[objType].minSize > _sipMemPool[objType].curSize) {
            _sipMemPool[objType].minSize = _sipMemPool[objType].curSize;
        } 
        SIP_DebugLog(SIP_DB_MEMORY_LVL_3,
                "SIP_memPoolAlloc: %s  %p  curSize:%d\n", (int)pList->listName,
                (int)pEntry, (int)_sipMemPool[objType].curSize);

    }
#endif
    return (pEntry);
#endif
}

/*
 * ======== SIP_memPoolFree() ========
 *
 * This function is to free a SIP object(tDLListEntry) to the memory pool list.
 *
 * objType:      The tSipObjType to allocate.
 * pEntry        A pointer to the entry to free.
 *
 * Returns:
 *     None.
 */
void SIP_memPoolFree(
    tSipObjType   objType,
    tDLListEntry *pEntry)
{
#ifdef SIP_DYNAMIC_MEMORY_ALLOC
    if (eSIP_OBJECT_TIMER == objType){
        if (((tSipTimerEntry *)pEntry)->tmrId) {
            OSAL_tmrDelete(((tSipTimerEntry *)pEntry)->tmrId);
        }
        if (((tSipTimerEntry *)pEntry)->retryTmrId) {
            OSAL_tmrDelete(((tSipTimerEntry *)pEntry)->retryTmrId);
        }
    }
    OSAL_memFree(pEntry, 0);
    return;
#else
    tDLList        *pList;
    vint            objSize;
    tSipTimerEntry *pTmrEntry;

    pList = &_sipMemPool[objType].poolList;
    objSize  = _sipMemPool[objType].objSize;

    SIP_DebugLog(SIP_DB_MEMORY_LVL_2,
            "SIP_memPoolFree %s %p\n", (int)pList->listName, (int)pEntry, 0);

    SIP_Lock(pList->lock);
    /* Clean up memory */
    if (eSIP_OBJECT_TIMER == objType) {
          /* Clean up content except tmrId */
          pTmrEntry = (tSipTimerEntry *)pEntry;
          pTmrEntry->id = 0;
          pTmrEntry->pfCB = 0;
          pTmrEntry->pArg = 0;
          pTmrEntry->hContext = 0;
    }
    else {
        /* Clean up memory */
        OSAL_memSet(pEntry, 0, objSize);
    }
    DLLIST_InitEntry(pEntry);
    DLLIST_Enqueue(pList, pEntry);
    SIP_Unlock(pList->lock);
#ifdef SIP_DEBUG_LOG
    _sipMemPool[objType].curSize++;
#endif
#endif
}

/*
 * ======== SIP_memPoolDestroy() ========
 *
 * This function is to free all memory pool list to heap.
 *
 * Returns:
 *     None.
 */
void SIP_memPoolDestroy(
    void)
{
#ifdef SIP_DYNAMIC_MEMORY_ALLOC
    return;
#else
    vint     poolIndex;
    tDLList *pList;
    tDLListEntry *pEntry;
    vint          size;
#ifdef SIP_DEBUG_LOG
    vint          poolSize;
#endif

    SIP_DebugLog(SIP_DB_MEMORY_LVL_2, "SIP_memPoolDestroy\n", 0, 0, 0);
    for (poolIndex = eSIP_OBJECT_FIRST; poolIndex < eSIP_OBJECT_LAST;
            poolIndex++) {
        pList = &_sipMemPool[poolIndex].poolList;
#ifdef SIP_DEBUG_LOG
        poolSize = _sipMemPool[poolIndex].poolSize;
#endif
        size = 0;

        SIP_Lock(pList->lock);
        /* Free all entries in the list */
        while ((NULL != (pEntry = DLLIST_Dequeue(pList)))) {
            /* Free OSAL timer for each tSipTimerEntry */
            if (eSIP_OBJECT_TIMER == poolIndex) {
                if (((tSipTimerEntry *)pEntry)->tmrId) {
                    OSAL_tmrDelete(((tSipTimerEntry *)pEntry)->tmrId);
                }
                if (((tSipTimerEntry *)pEntry)->retryTmrId) {
                    OSAL_tmrDelete(((tSipTimerEntry *)pEntry)->retryTmrId);
                }
            }
            /* Free memory to heap */
            SIP_free(pEntry);
            size++;
        }
        SIP_Unlock(pList->lock);

        SIP_DebugLog(SIP_DB_MEMORY_LVL_2,
                "SIP_memPoolDestroy %s original poolSize:%d free size:%d\n",
        (int)pList->listName, poolSize, size);
#ifdef SIP_DEBUG_LOG
        if (0 <= _sipMemPool[poolIndex].minSize) {
            SIP_DebugLog(SIP_DB_MEMORY_LVL_2,
                    "SIP_memPoolDestroy %s max usage:%d\n",
                    (int)pList->listName,
                    (poolSize - _sipMemPool[poolIndex].minSize), 0);
        }
        else {
            SIP_DebugLog(SIP_DB_MEMORY_LVL_2,
                    "SIP_memPoolDestroy %s max usage:%d, miss count:%d\n",
                    (int)pList->listName,
                    poolSize, -_sipMemPool[poolIndex].minSize);
        }
#endif
        /* Destroy mutex */
        SIP_MutexDestroy(pList->lock);
    }

    return;
#endif
}
