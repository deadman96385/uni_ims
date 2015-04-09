/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 28728 $ $Date: 2014-09-05 17:01:31 +0800 (Fri, 05 Sep 2014) $
 */

#include "sip_sip.h"
#include "sip_port.h"
#include "sip_mem.h"
#include "sip_list.h"
#include "sip_hdrflds.h"
#include "sip_debug.h"
#include "sip_port.h"
#include "sip_mem_pool.h"
#include "sip_list.h"

void DLLIST_InitEntry(
    tDLListEntry *pEntry)
{
    if (NULL == pEntry) {
        SIP_DebugLog(SIP_DB_GENERAL_LVL_1, "DLLIST_InitEntry: NULL entry", 0, 0, 0);
        return;
    }
#ifdef SIP_LIST_DEBUG
    pEntry->inUse = 0;
#endif
    pEntry->pNext = NULL;
    pEntry->pPrev = NULL;
}

void DLLIST_InitList(
    tDLList    *pDLL)
{
     if (NULL == pDLL) {
         SIP_DebugLog(SIP_DB_GENERAL_LVL_1, "DLLIST_InitList: NULL DLL", 0, 0, 0);
         return;
     }
    pDLL->cnt = 0;
    pDLL->pHead = NULL;
    pDLL->pTail = NULL;
    return;
}

/* 
 *****************************************************************************
 * ================DLLIST_Dequeue()===================
 *
 * This function pops the head element off the DLL
 *
 * RETURNS: 
 *         A void pointer to the head element if something exists.
 *         Or, void if the list is empty
 ******************************************************************************
 */
tDLListEntry * DLLIST_Dequeue(tDLList *pDLL)
{
    tDLListEntry *pEntry;

    if (!pDLL) {
        SIP_DebugLog(SIP_DB_GENERAL_LVL_1, "DLLIST_Dequeue: NULL DLList", 0, 0, 0);
        return (NULL);
    }

    pEntry = pDLL->pHead;
    if (!pEntry) {
        return pEntry;
    }

    /* This also cleans up the head */
    if (pDLL->isBackwards) {
        pDLL->pHead = pDLL->pHead->pPrev;
        if (pDLL->pHead)
            pDLL->pHead->pNext = NULL;
    }
    else {
        pDLL->pHead = pDLL->pHead->pNext;
        if (pDLL->pHead)
            pDLL->pHead->pPrev = NULL;
    }

    if (pDLL->pTail == pEntry) /* then update the tail too */
        pDLL->pTail = pDLL->pHead;

    /* clean up the entry before you return, it's safer */
    pEntry->pNext = NULL;
    pEntry->pPrev = NULL;

    /* clear the 'inUse' var */
#ifdef SIP_LIST_DEBUG
    pEntry->inUse = 0;
#endif
    pDLL->cnt--;
    return pEntry;
}


/* 
 *****************************************************************************
 * ================DLLIST_Enqueue()===================
 *
 * This function enqueues an element of 'void' type into the linked list
 *
 * RETURNS: 
 *        SIP_OK if successful 
 *        SIP_FAILED if DLL pointer is null
 ******************************************************************************
 */
int DLLIST_Enqueue(
    tDLList *pDLL, 
    tDLListEntry *pEntry)
{
    if (!pDLL || !pEntry) {
        SIP_DebugLog(SIP_DB_GENERAL_LVL_1, "DLLIST_Enqueue: bad param pDLL:%x pEntry:%x", (int)pDLL, (int)pEntry, 0);
        return (SIP_BADPARM);
    }
    
#ifdef SIP_LIST_DEBUG
    if (pEntry->inUse == SIP_VALID_DATA) {
        SIP_DebugLog(SIP_DB_GENERAL_LVL_1, "DLLIST_Enqueue: %s (%X) - enqueuing an element that's already in use inUse:0x%X",
            (int)pDLL->listName, (int)pDLL, pEntry->inUse);
        return (SIP_BADPARM);
    }

    /* set the data to inuse */
    pEntry->inUse = SIP_VALID_DATA;
#endif

    if (!pDLL->pHead) {
        /* then list is empty */
        pDLL->pHead = pDLL->pTail = pEntry;
        pDLL->cnt++;
        return (SIP_OK);
    }
    
    if (!pDLL->isBackwards) {
        pDLL->pTail->pNext = pEntry;
        pEntry->pPrev = pDLL->pTail;
    }
    else {
        pDLL->pTail->pPrev = pEntry;
        pEntry->pNext = pDLL->pTail;
    }
    pDLL->pTail = pEntry;
    pDLL->cnt++;
    return (SIP_OK);
}

/* 
 *****************************************************************************
 * ================DLLIST_Reverse()===================
 *
 * This function sets a flag to indictate to further calls to the DLL
 * interface are operating on the list in reverse (backwards) order
 *
 * RETURNS: Nothing
 *****************************************************************************
 */
vint DLLIST_Reverse(tDLList *pDLL)
{
    tDLListEntry *pEntry;

    if (!pDLL) {
        SIP_DebugLog(SIP_DB_GENERAL_LVL_1, "DLLIST_Reverse: NULL pDLL", 0, 0, 0);
        return (SIP_BADPARM);
    }

    /* Flip the is backwards ? switch */
    pDLL->isBackwards = (!(pDLL->isBackwards));
    /* Swap the head and tail pointers */
    pEntry = pDLL->pHead;
    pDLL->pHead = pDLL->pTail;
    pDLL->pTail = pEntry;
    return (SIP_OK);
}

/* 
 *****************************************************************************
 * ================DLLIST_GetNext()===================
 *
 * This function gets the next element in the DLL.
 * If the Address of target pointer (pPtr) is null
 * then the head element is returned.  Otherwise the next 
 * element to pPtr is returned.  Please Note that this function is aware
 * of whether or not the DLL is being operated on in a backwards manor.
 *
 * RETURNS: 
 *        0 = List is empty or invalid.
 *        1 = The target element (pPtr) is the last element.
 *        2 = More elements exist in the DLL.
 ******************************************************************************
 */
int DLLIST_GetNext(
    const tDLList *pDLL, 
    tDLListEntry **ppPtr)
{
    int value;
    tDLListEntry *pEntry;
    
    if (!pDLL || !ppPtr) {
        SIP_DebugLog(SIP_DB_GENERAL_LVL_1, "DLLIST_GetNext: %s (%X) BADPARM ppPtr:%x", (int)pDLL->listName, (int)pDLL, (int)ppPtr);
        return (0);
    }

    pEntry = *ppPtr;
    if (pEntry == NULL)
        pEntry = pDLL->pHead;
    else {

#ifdef SIP_LIST_DEBUG
        /* verify that the data we received is valid */
        if (pEntry->inUse != SIP_VALID_DATA) {
            SIP_DebugLog(SIP_DB_GENERAL_LVL_1, "DLLIST_GetNext: %s (%X) - Invalid data inUse:0x%X", (int)pDLL->listName, (int)pDLL, pEntry->inUse);
            return (0);
        }
#endif

        if (pDLL->isBackwards)
            pEntry = pEntry->pPrev;
        else
            pEntry = pEntry->pNext;
    }
    
    if (pEntry == NULL)
        value = 0;
    else {
        if (pEntry == pDLL->pTail)
            value = 1;
        else
            value = 2;
    }
    *ppPtr = pEntry;
    return value;
}

/* 
 *****************************************************************************
 * ================DLLIST_Remove()===================
 *
 * This function removes an element from the DLL
 *********************************************************************
 * WARNING: NOTE: IS DOES NOT FREE THE OBJECT (MEMORY). 
 *          MEANING...The Caller is responsible for 
 *          handling the object and it's allocation (or deallocation)
 *********************************************************************
 *
 * RETURNS: 
 *        SIP_OK: Item was removed
 *        SIP_BADPARM: one of the parameters was invalid or the list entry 
 *                     is currently being used
 ******************************************************************************
 */
vint DLLIST_Remove(
    tDLList *pDLL, 
    tDLListEntry *pEntry)
{
    if (!pDLL || !pEntry) {
        SIP_DebugLog(SIP_DB_GENERAL_LVL_1, "DLLIST_Remove: %s (%X) Error BADPARM pEntry:%x", (int)pDLL->listName, (int)pDLL, (int)pEntry);
        return (SIP_BADPARM);
    }

#ifdef SIP_LIST_DEBUG
    if (pEntry->inUse != SIP_VALID_DATA) {
        /* the element must not be part of a list */
        SIP_DebugLog(SIP_DB_GENERAL_LVL_1, "DLLIST_Remove: %s (%X) - removing an invalid element inUse:0x%X", (int)pDLL->listName, (int)pDLL, pEntry->inUse);
        return (SIP_BADPARM);
    }
#endif
    
    if (pEntry == pDLL->pHead) {
        DLLIST_Dequeue(pDLL);
    }
    else if (pEntry ==  pDLL->pTail) {
        DLLIST_Reverse(pDLL);
        DLLIST_Dequeue(pDLL);
        DLLIST_Reverse(pDLL);
    }
    else {
       pEntry->pPrev->pNext = pEntry->pNext;
       pEntry->pNext->pPrev = pEntry->pPrev;
       pEntry->pNext = NULL;
       pEntry->pPrev = NULL;

#ifdef SIP_LIST_DEBUG
       pEntry->inUse = 0;
#endif
       pDLL->cnt--;
    }
    return (SIP_OK);
}

/* 
 *****************************************************************************
 * ================DLLIST_IsEmpty()===================
 *
 * This function checks if there are any more elements in the DLL
 *
 * RETURNS: 
 *        TRUE: List is NOT Empty
 *        FALSE: List is in fact empty.
 ******************************************************************************
 */
vint DLLIST_IsEmpty(const tDLList *pDLL)
{
    if (!pDLL) {
        SIP_DebugLog(SIP_DB_GENERAL_LVL_1, "DLLIST_IsEmpty: pDLL is NULL", 0, 0, 0);
        return (TRUE);
    }
    
    if (pDLL->pHead)
        return FALSE;
    else
        return TRUE;
}

/* 
 *****************************************************************************
 * ================DLLIST_Empty()===================
 *
 * This function empties out a DLL and "frees" any elements in the list to
 * memory pool
 *
 * type - The tSipObjType to free to memory pool.
 *        eSIP_OBJECT_NONE means it's not managed by memory pool, free it to
 *        system memory.
 *
 * RETURNS: 
 *        Nothing
 *       
 ******************************************************************************
 */
vint DLLIST_Empty(
    tDLList *pDLL,
    vint     type)
{
    tDLListEntry *pEntry;
    
    if (!pDLL) {
        SIP_DebugLog(SIP_DB_GENERAL_LVL_1, "DLLIST_Empty: pDLL is NULL", 0, 0, 0);
        return (SIP_BADPARM);
    }
    
    pEntry = pDLL->pHead;

    while (pEntry) {
        tDLListEntry *pDump = pEntry;
#ifdef SIP_LIST_DEBUG
        if (pEntry->inUse != SIP_VALID_DATA) {
            /* we stumbled across an invalid element */
            SIP_DebugLog(SIP_DB_GENERAL_LVL_1,
                    "DLLIST_Empty: %s (%X) - Bad element in the list inUse:0x%X",
                    (int)pDLL->listName, (int)pDLL, pEntry->inUse);
            return (SIP_BADPARM);
        }
#endif
       
        if (pDLL->isBackwards)
            pEntry = pEntry->pPrev;
        else
            pEntry = pEntry->pNext;

#ifdef SIP_LIST_DEBUG
        /* clear the inUse bit before we free */
        pDump->inUse = 0;
#endif
        pDLL->cnt--;
        if (eSIP_OBJECT_NONE != type) {
            SIP_memPoolFree((tSipObjType)type, (tDLListEntry *)pDump);
        }
        else {
            SIP_free(pDump);
        }
    }
    pDLL->pHead = NULL;
    pDLL->pTail = NULL;
    return (SIP_OK);
}

/* 
 *****************************************************************************
 * ================DLLIST_Copy()===================
 *
 * This function copies a DLL to another DLL, including allocating new 
 * elements off the heap
 *
 *
 * RETURNS: 
 *        SIP_OK: List was successfully copied.
 *        SIP_FAILED: Could not copy list probably due to memory 
 *                    availablility problems.
 *        SIP_NOT_SUPPORTED:  The list could not be copies because
 *                            the element types are not supported and could
 *                            not be allocated from the memory heap.
 ******************************************************************************
 */
vint DLLIST_Copy(
    tDLList     *pFrom, 
    tDLList     *pTo, 
    tDLList_Type listType)
{
    tDLListEntry   *pEntry;
    int             status;
    vint            type = eSIP_OBJECT_NONE; /* tSipObjType */

    type = eSIP_OBJECT_NONE; // set default value

    if (!pFrom || !pTo) {
        SIP_DebugLog(SIP_DB_GENERAL_LVL_1, "DLLIST_Copy: BADPARM pFrom:%x pTo:%x", (int)pFrom, (int)pTo, 0);
        return (SIP_BADPARM);
    }
    pEntry = NULL;
    while (0 != (status = DLLIST_GetNext(pFrom, &pEntry))) {
#ifdef SIP_LIST_DEBUG
        if (pEntry->inUse != SIP_VALID_DATA) {
            /* found a bad element in the list we are copying */
            SIP_DebugLog(SIP_DB_GENERAL_LVL_1, "DLLIST_Copy: pFrom %s (%X) - invalid data", (int)pFrom->listName, (int)pFrom, 0);
            goto copydllcleanup;
        }
#endif

        /* Now build the copy object and insert into target list 'pTo'
         * remember to clear the inUse value before attempting to 'enqueue'
         */
        switch (listType) {
        case eDLLIST_VIA_HF:
            {
            tViaHFE               *pVia;

            type = eSIP_OBJECT_VIA_HF;
            if (NULL == (pVia = (tViaHFE *)SIP_memPoolAlloc(eSIP_OBJECT_VIA_HF)))
                goto copydllcleanup;
            
            (*pVia) = (*((tViaHFE*)pEntry));
            DLLIST_InitEntry(&pVia->dll);
            if (DLLIST_Enqueue(pTo, &pVia->dll) != SIP_OK) {
                SIP_memPoolFree(eSIP_OBJECT_VIA_HF, (tDLListEntry *)pVia);
                goto copydllcleanup;
            }
            }
            break;

        case eDLLIST_REC_ROUTE_HF:
        case eDLLIST_ROUTE_HF:
        case eDLLIST_SERVICE_ROUTE_HF:
            {
            tRouteHFE             *pRoute;

            type = eSIP_OBJECT_ROUTE_HF;
            if (NULL == (pRoute = (tRouteHFE *)SIP_memPoolAlloc(eSIP_OBJECT_ROUTE_HF)))
                goto copydllcleanup;
            
            (*pRoute) = (*((tRouteHFE*)pEntry));
            DLLIST_InitEntry(&pRoute->dll);
            if (DLLIST_Enqueue(pTo, &pRoute->dll) != SIP_OK) {
                SIP_memPoolFree(eSIP_OBJECT_ROUTE_HF, (tDLListEntry *)pRoute);
                goto copydllcleanup;
            }
            }
            break;

        case eDLLIST_WWW_AUTH_HF:
        case eDLLIST_PROXY_AUTH_HF:
            {
            tAuthorizationHFE     *pAuth;

            type = eSIP_OBJECT_AUTH_HF;
            if (NULL == (pAuth = (tAuthorizationHFE *)SIP_memPoolAlloc(eSIP_OBJECT_AUTH_HF)))
                goto copydllcleanup;
            
            (*pAuth) = (*((tAuthorizationHFE*)pEntry));
            DLLIST_InitEntry(&pAuth->dll);
            if (DLLIST_Enqueue(pTo, &pAuth->dll) != SIP_OK) {
                SIP_memPoolFree(eSIP_OBJECT_AUTH_HF, (tDLListEntry *)pAuth);
                goto copydllcleanup;
            }
            }
            break;

        case eDLLIST_CONTACT_HF:
            {
            tContactHFE           *pContact;

            type = eSIP_OBJECT_CONTACT_HF;
            if (NULL == (pContact = (tContactHFE *)SIP_memPoolAlloc(eSIP_OBJECT_CONTACT_HF)))
                goto copydllcleanup;
            
            (*pContact) = (*((tContactHFE*)pEntry));
            DLLIST_InitEntry(&pContact->dll);
            if (DLLIST_Enqueue(pTo, &pContact->dll) != SIP_OK) {
                SIP_memPoolFree(eSIP_OBJECT_CONTACT_HF, (tDLListEntry *)pContact);
                goto copydllcleanup;
            }
            }
            break;

        case eDLLIST_AUTH_CREDENTIALS:
            {
            tAUTH_Cred  *pCred;

            type = eSIP_OBJECT_AUTH_CRED;
            if (NULL == (pCred = (tAUTH_Cred *)SIP_memPoolAlloc(eSIP_OBJECT_AUTH_CRED)))
                goto copydllcleanup;

            (*pCred) = (*((tAUTH_Cred*)pEntry));
            DLLIST_InitEntry(&pCred->dll);
            if (DLLIST_Enqueue(pTo, &pCred->dll) != SIP_OK) {
                SIP_memPoolFree(eSIP_OBJECT_AUTH_CRED, (tDLListEntry *)pCred);
                goto copydllcleanup;
            }
            }
            break;

        default:
            SIP_DebugLog(SIP_DB_GENERAL_LVL_1, "DLLIST_Copy: Unknown list type : %d", listType, 0, 0);
            return (SIP_NOT_SUPPORTED);
        }
    }

    if (status == 0) {
        pTo->cnt = pFrom->cnt;
        return (SIP_OK);
    }

copydllcleanup:
    /* Something went wrong */
    DLLIST_Empty(pTo, type);
    return (SIP_FAILED);
}



