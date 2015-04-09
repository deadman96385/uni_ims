/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 24709 $ $Date: 2014-02-20 17:27:12 +0800 (Thu, 20 Feb 2014) $
 */

#ifndef _SIP_LIST_H_
#define _SIP_LIST_H_

/* Assumes sip_sip.h is always included first */
#include "sip_port.h"

/*
 * Define this if you are concerned that users are not using
 * the interfaces in this module correctly. Defining it
 * will perform checks against element inside DLL's and
 * report things like trying install the same element twice
 * or if there is an element in a list that doesn't belong.
 */
/* #define SIP_LIST_DEBUG (1) */

typedef enum eDLList_Type
{
    eDLLIST_VIA_HF,
    eDLLIST_REC_ROUTE_HF,
    eDLLIST_ROUTE_HF,
    eDLLIST_WWW_AUTH_HF,
    eDLLIST_PROXY_AUTH_HF,
    eDLLIST_CONTACT_HF,
    eDLLIST_AUTH_CREDENTIALS,
    eDLLIST_SERVICE_ROUTE_HF,
}tDLList_Type;

typedef struct sDLListEntry
{
    struct sDLListEntry *pNext;
    struct sDLListEntry *pPrev;
#ifdef SIP_LIST_DEBUG
    uint32    inUse;
#endif
}tDLListEntry;

typedef struct sDLList
{
    tDLListEntry *pHead;
    tDLListEntry *pTail;
    vint          isBackwards;
    tSipMutex     lock;
    char         *listName;
    int           cnt;
}tDLList;

void DLLIST_InitEntry(tDLListEntry *pEntry);

void DLLIST_InitList(
    tDLList    *pDLL);

tDLListEntry * DLLIST_Dequeue(tDLList *pDLL);

int DLLIST_Enqueue(
    tDLList *pDLL, 
    tDLListEntry *pEntry);

vint DLLIST_Reverse(tDLList *pDLL);

int DLLIST_GetNext(
        const tDLList *pDLL, 
    tDLListEntry **ppPtr);

vint DLLIST_Remove(
    tDLList *pDLL, 
    tDLListEntry *pEntry);

vint DLLIST_IsEmpty(const tDLList *pDLL);

vint DLLIST_Empty(
    tDLList *pDLL,
    vint     type);

vint DLLIST_Copy(
    tDLList     *pFrom, 
    tDLList     *pTo, 
    tDLList_Type listType);

#endif
