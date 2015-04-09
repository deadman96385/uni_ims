/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 24709 $ $Date: 2014-02-20 17:27:12 +0800 (Thu, 20 Feb 2014) $
 */
#include "isi.h"
#include "_isi_port.h"
#include "_isi_list.h"

/* 
 *****************************************************************************
 * ================ISIL_initEntry()===================
 *
 * This function init the Entry
 *
 * RETURNS: 
 * 
 ******************************************************************************
 */
void ISIL_initEntry(
    ISIL_ListEntry *pEntry)
{
    if (NULL == pEntry) {
        OSAL_logMsg("ISIL_initEntry: NULL entry");
        return;
    }
#ifdef ISI_LIST_DEBUG
    pEntry->inUse = 0;
#endif
    pEntry->pNext = NULL;
    pEntry->pPrev = NULL;
}

/* 
 *****************************************************************************
 * ================ISIL_initList()===================
 *
 * This function init the DLList
 *
 * RETURNS: 
 * 
 ******************************************************************************
 */
void ISIL_initList(
    ISIL_List    *pDLL)
{
     if (NULL == pDLL) {
         OSAL_logMsg("ISIL_initList: NULL DLL");
         return;
     }
    pDLL->cnt = 0;
    pDLL->pHead = NULL;
    pDLL->pTail = NULL;
    return;
}

/* 
 *****************************************************************************
 * ================ISIL_dequeue()===================
 *
 * This function pops the head element off the DLL
 *
 * RETURNS: 
 *         A void pointer to the head element if something exists.
 *         Or, void if the list is empty
 ******************************************************************************
 */
ISIL_ListEntry * ISIL_dequeue(ISIL_List *pDLL)
{
    ISIL_ListEntry *pEntry;

    if (!pDLL) {
        OSAL_logMsg("ISIL_dequeue: NULL DLList");
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
#ifdef ISI_LIST_DEBUG
    pEntry->inUse = 0;
#endif
    pDLL->cnt--;
    return pEntry;
}

/* 
 *****************************************************************************
 * ================ISIL_enqueue()===================
 *
 * This function enqueues an element of 'void' type into the linked list
 *
 * RETURNS: 
 *        ISI_RETURN_OK if successful 
 *        ISI_RETURN_FAILED if DLL pointer is null
 ******************************************************************************
 */
int ISIL_enqueue(
    ISIL_List *pDLL, 
    ISIL_ListEntry *pEntry)
{
    if (!pDLL || !pEntry) {
        OSAL_logMsg("ISIL_enqueue: bad param pDLL:%x pEntry:%x",
                (int)pDLL, (int)pEntry);
        return (ISI_RETURN_FAILED);
    }
    
#ifdef ISI_LIST_DEBUG
    if (pEntry->inUse == 1) {
          OSAL_logMsg(
            "%s: %s (%X) - enqueuing an element that's already in use inUse:0x%X",
            __FUNCTION__, pDLL->listName, (int)pDLL, pEntry->inUse);
        return (ISI_RETURN_FAILED);
    }

    /* set the data to inuse */
    pEntry->inUse = 1;
#endif

    if (!pDLL->pHead) {
        /* then list is empty */
        pDLL->pHead = pDLL->pTail = pEntry;
        pDLL->cnt++;
        return (ISI_RETURN_OK);
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
    return (ISI_RETURN_OK);
}

