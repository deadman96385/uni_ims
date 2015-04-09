/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 24709 $ $Date: 2014-02-20 17:27:12 +0800 (Thu, 20 Feb 2014) $
 */

#include "_ezxml_list.h"


/* 
 *****************************************************************************
 * ================EZXML_initEntry()===================
 *
 * This function init the Entry
 *
 * RETURNS: 
 * 
 ******************************************************************************
 */
void EZXML_initEntry(
    EZXML_ListEntry *pEntry)
{
    if (NULL == pEntry) {
        OSAL_logMsg("EZXML_initEntry: NULL entry");
        return;
    }
#ifdef EZXML_DEBUG_LOG
    pEntry->inUse = 0;
#endif
    pEntry->u.p.pNext = NULL;
    pEntry->u.p.pPrev = NULL;
    pEntry->u.e.data = NULL;
    pEntry->u.e.objType = 0; 
}

/* 
 *****************************************************************************
 * ================EZXML_initList()===================
 *
 * This function init the DLList
 *
 * RETURNS: 
 * 
 ******************************************************************************
 */
void EZXML_initList(
    EZXML_List    *pDLL)
{
     if (NULL == pDLL) {
         OSAL_logMsg("EZXML_initList: NULL DLL");
         return;
     }
    pDLL->cnt = 0;
    pDLL->pHead = NULL;
    pDLL->pTail = NULL;
    return;
}

/* 
 *****************************************************************************
 * ================EZXML_dequeue()===================
 *
 * This function pops the head element off the DLL
 *
 * RETURNS: 
 *         A void pointer to the head element if something exists.
 *         Or, void if the list is empty
 ******************************************************************************
 */
EZXML_ListEntry * EZXML_dequeue(EZXML_List *pDLL)
{
    EZXML_ListEntry *pEntry;

    if (!pDLL) {
        OSAL_logMsg("EZXML_dequeue: NULL DLList");
        return (NULL);
    }

    pEntry = pDLL->pHead;
    if (!pEntry) {
        return pEntry;
    }

    /* This also cleans up the head */
    if (pDLL->isBackwards) {
        pDLL->pHead = pDLL->pHead->u.p.pPrev;
        if (pDLL->pHead)
            pDLL->pHead->u.p.pNext = NULL;
    }
    else {
        pDLL->pHead = pDLL->pHead->u.p.pNext;
        if (pDLL->pHead)
            pDLL->pHead->u.p.pPrev = NULL;
    }

    if (pDLL->pTail == pEntry) /* then update the tail too */
        pDLL->pTail = pDLL->pHead;

    /* clean up the entry before you return, it's safer */
    pEntry->u.p.pNext = NULL;
    pEntry->u.p.pPrev = NULL;
    pEntry->u.e.data  = NULL;

    /* clear the 'inUse' var */
#ifdef EZXML_DEBUG_LOG
    pEntry->inUse = 0;
#endif
    pDLL->cnt--;
    return pEntry;
}

/* 
 *****************************************************************************
 * ================EZXML_enqueue()===================
 *
 * This function enqueues an element of 'void' type into the linked list
 *
 * RETURNS: 
 *        ISI_RETURN_OK if successful 
 *        ISI_RETURN_FAILED if DLL pointer is null
 ******************************************************************************
 */
int EZXML_enqueue(
    EZXML_List *pDLL, 
    EZXML_ListEntry *pEntry)
{
    if (!pDLL || !pEntry) {
        OSAL_logMsg("EZXML_enqueue: bad param pDLL:%x pEntry:%x",
                (int)pDLL, (int)pEntry);
        return (-1);
    }
    
#ifdef EZXML_DEBUG_LOG
    if (pEntry->inUse == 1) {
          OSAL_logMsg(
                "%s: %s (%X) - enqueuing an element that's already in use inUse:0x%X",
                __FUNCTION__, pDLL->listName, (int)pDLL, pEntry->inUse);
        return (-1);
    }

    /* set the data to inuse */
    pEntry->inUse = 1;
#endif

    if (!pDLL->pHead) {
        /* then list is empty */
        pDLL->pHead = pDLL->pTail = pEntry;
        pDLL->cnt++;
        return (0);
    }
    
    if (!pDLL->isBackwards) {
        pDLL->pTail->u.p.pNext = pEntry;
        pEntry->u.p.pPrev = pDLL->pTail;
    }
    else {
        pDLL->pTail->u.p.pPrev = pEntry;
        pEntry->u.p.pNext = pDLL->pTail;
    }
    pDLL->pTail = pEntry;
    pDLL->cnt++;
    return (0);
}

