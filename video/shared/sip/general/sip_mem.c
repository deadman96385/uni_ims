/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 28728 $ $Date: 2014-09-05 17:01:31 +0800 (Fri, 05 Sep 2014) $
 */

#include "sip_list.h"
#include "sip_sip.h"
#include "sip_sdp_msg.h"
#include "sip_port.h"
#include "sip_mem.h"
#include "sip_mem_pool.h"

/* 
 ******************************************************************************
 * ================SIP_malloc()===================
 *
 * This function is a wrapper function for allocating memory off heap.
 * Memory is allocted and zeroed out.
 *
 * size = The number of contiguous bytes to allocate.
 *
 * RETURNS: 
 *        A pointer to the memory block.  NULL, if the undelying system call 
 *        failed.
 *         
 ******************************************************************************
 */
void *SIP_malloc(
    int size)
{
    void *pMem;

    pMem = (void *)SIP_MEMALLOC(size);
    SIP_DebugLog(SIP_DB_MEMORY_LVL_3, "SIP_malloc: %d bytes @ %X",
            (int)size, (int)pMem, 0);
    
    if (NULL != pMem) {
        OSAL_memSet(pMem, 0, size);
    }
    
    return (pMem);
}


/* 
 ******************************************************************************
 * ================SIP_free()===================
 *
 * This function is a wrapper function for freeing memory back to the heap.
 * 
 * pMem = A pointer to the memory block.
 *
 * RETURNS: 
 *        Nothing.
 *         
 ******************************************************************************
 */
void SIP_free(
    void *pMem)
{
    SIP_DebugLog(SIP_DB_MEMORY_LVL_3, "SIP_free: %X", (int)pMem, 0, 0);
    SIP_MEMFREE(pMem);
}

/* 
 ******************************************************************************
 * ================SIP_allocMsg()===================
 *
 * This function allocates an tSipIntMsg object (internal sip message).
 * 
 * RETURNS: 
 *        A pointer to the object.
 *         
 ******************************************************************************
 */
tSipIntMsg* SIP_allocMsg(
    void)
{
    return ((tSipIntMsg*)SIP_memPoolAlloc(eSIP_OBJECT_SIP_INT_MSG));
}

/* 
 ******************************************************************************
 * ================SIP_freeMsg()===================
 *
 * This function allocates frees a tSipIntMsg object back to the heap.
 * This function automatically handles freeing nested objects that also came off 
 * the heap.  This includes header fields that are strings and 
 * Double Linked Lists
 * 
 * RETURNS: 
 *        Nothing
 *         
 ******************************************************************************
 */
void SIP_freeMsg(
    tSipIntMsg *pMsg)
{
    if (pMsg->pReasonPhrase) {
        SIP_memPoolFree(eSIP_OBJECT_SIP_TEXT,
                (tDLListEntry *)pMsg->pReasonPhrase);
        pMsg->pReasonPhrase = NULL;
    }
    
    if (pMsg->pMsgBody) {
        SIP_memPoolFree(eSIP_OBJECT_SIP_MSG_BODY, (tDLListEntry *)pMsg->pMsgBody);
        pMsg->pMsgBody = NULL;
    }

    /* free all the header fields */
    HF_DeleteAll(&pMsg->pHFList);
    
    /* Free all linked list members */
    DLLIST_Empty(&pMsg->ViaList, eSIP_OBJECT_VIA_HF);
    DLLIST_Empty(&pMsg->RecRouteList, eSIP_OBJECT_ROUTE_HF);
    DLLIST_Empty(&pMsg->RouteList, eSIP_OBJECT_ROUTE_HF);
    DLLIST_Empty(&pMsg->AuthorizationList, eSIP_OBJECT_AUTH_HF);
    DLLIST_Empty(&pMsg->ContactList, eSIP_OBJECT_CONTACT_HF);
    DLLIST_Empty(&pMsg->ServiceRouteList, eSIP_OBJECT_ROUTE_HF);

    if (pMsg->pSessDescr) {
        SDP_DeallocMsg(pMsg->pSessDescr);
        pMsg->pSessDescr = NULL;
    }

    /* finally the message itself */
    SIP_memPoolFree(eSIP_OBJECT_SIP_INT_MSG, (tDLListEntry *)pMsg);
    return;
}

/* 
 ******************************************************************************
 * ================SIP_copyMsg()===================
 *
 * This function copies a tSipIntMsg.  First a new tSipIntMsg object is 
 * allocated off the heap and then all internal objects and members are copied
 * from the from the source to the new target 
 *
 * pMsg = A Pointer to the message to be copied (this is the source)
 *
 * RETURNS: 
 *        NULL: Function failed due to memory allocation problems.
 * tSipIntMsg*: A pointer to the new message.
 *         
 ******************************************************************************
 */
tSipIntMsg *SIP_copyMsg(
    tSipIntMsg *pMsg)
{
    tSipIntMsg *pNewMsg;
    
    if (NULL == (pNewMsg =
            ((tSipIntMsg*)SIP_memPoolAlloc(eSIP_OBJECT_SIP_INT_MSG)))) {
        return (pNewMsg);
    }

    /* copy the first part of the message */
    pNewMsg->msgType    = pMsg->msgType;
    pNewMsg->method     = pMsg->method;
    pNewMsg->code       = pMsg->code;
    pNewMsg->requestUri = pMsg->requestUri;
    pNewMsg->isCompactForm = pMsg->isCompactForm;

    /* copy the reason phrase if it exists */
    if (pMsg->pReasonPhrase) {
        SIP_copySipText(pMsg->pReasonPhrase, &pNewMsg->pReasonPhrase);
    }
    
    /* copy a text message if one exists */
    if (pMsg->pMsgBody)
        SIP_copySipMsgBody(pMsg->pMsgBody, &pNewMsg->pMsgBody);

    /* copy string from old into newly allocated NewMsg */
    if (SIP_OK != HF_CopyAll(&pNewMsg->pHFList, pMsg->pHFList, NULL)) {
        goto SIP_MEM_DESTROY_DUE_TO_ERROR;
    }
    
    /* now copy the call id */
    OSAL_strcpy(pNewMsg->szCallId, pMsg->szCallId);

    /* now copy the 'Via' linked lists */
    if (SIP_OK != DLLIST_Copy(&pMsg->ViaList, &pNewMsg->ViaList, eDLLIST_VIA_HF)) {
        goto SIP_MEM_DESTROY_DUE_TO_ERROR;
    }

    /* now copy the 'Contact' linked lists */
    if (SIP_OK != DLLIST_Copy(&pMsg->ContactList, &pNewMsg->ContactList,
                eDLLIST_CONTACT_HF)) {
        goto SIP_MEM_DESTROY_DUE_TO_ERROR;
    }

    /* now the route list */
    if (SIP_OK != DLLIST_Copy(&pMsg->RouteList, &pNewMsg->RouteList,
                eDLLIST_ROUTE_HF)) {
        goto SIP_MEM_DESTROY_DUE_TO_ERROR;
    }
    
    /* now copy the record-route linked lists */
    if (SIP_OK != DLLIST_Copy(&pMsg->RecRouteList, &pNewMsg->RecRouteList,
                eDLLIST_REC_ROUTE_HF)) {
        goto SIP_MEM_DESTROY_DUE_TO_ERROR;
    }
    
    /* now copy the service-route linked lists */
    if (SIP_OK != DLLIST_Copy(&pMsg->ServiceRouteList,
            &pNewMsg->ServiceRouteList, eDLLIST_SERVICE_ROUTE_HF)) {
        goto SIP_MEM_DESTROY_DUE_TO_ERROR;
    }

    /* now the authorization list */
    if (SIP_OK != DLLIST_Copy(&pMsg->AuthorizationList,
                &pNewMsg->AuthorizationList, eDLLIST_WWW_AUTH_HF)) {
        goto SIP_MEM_DESTROY_DUE_TO_ERROR;
    }

    /* now do the rest */

    /* bit maps */
    pNewMsg->x = pMsg->x;

    /* now the uint32's, and enums */
    pNewMsg->ContentLength  = pMsg->ContentLength;
    pNewMsg->Expires        = pMsg->Expires;
    pNewMsg->ContentType    = pMsg->ContentType;
    pNewMsg->RSeq           = pMsg->RSeq;
    
    /* now the others */
    pNewMsg->From   = pMsg->From;
    pNewMsg->To     = pMsg->To;
    pNewMsg->CSeq   = pMsg->CSeq;
    pNewMsg->RAck   = pMsg->RAck;
    
    /* the rest of these are optional */ 
    pNewMsg->Event        = pMsg->Event;
    pNewMsg->ReferTo      = pMsg->ReferTo;
    pNewMsg->SubState     = pMsg->SubState;
    pNewMsg->Replaces     = pMsg->Replaces;
    pNewMsg->SessionTimer = pMsg->SessionTimer;
    pNewMsg->MinSE        = pMsg->MinSE;
    
    /* now the SDP information */
    if (pMsg->pSessDescr) {
        if (NULL == (pNewMsg->pSessDescr = SDP_CopyMsg(pMsg->pSessDescr))) {
            goto SIP_MEM_DESTROY_DUE_TO_ERROR;
        }
    }
    pNewMsg->parmPresenceMask = pMsg->parmPresenceMask;
    pNewMsg->sipfragCode      = pMsg->sipfragCode; 
    return (pNewMsg);

SIP_MEM_DESTROY_DUE_TO_ERROR:
    SIP_DebugLog(SIP_DB_MEMORY_LVL_1, "SIP_copyMsg: failed !\n", 0, 0, 0);
    SIP_memPoolFree(eSIP_OBJECT_SIP_INT_MSG, (tDLListEntry *)pNewMsg);
    pNewMsg = NULL;
    return (pNewMsg);
}

/* 
 ******************************************************************************
 * ================SIP_allocString()===================
 *
 * This function allocates a string.  
 *
 * size = The length of the string
 *
 * RETURNS: 
 *        NULL: Function failed due to memory allocation problems.
 *       char*: A pointer to the new string.
 *         
 ******************************************************************************
 */
char *SIP_allocString(
    uint32 size)
{
    return ((char *)SIP_malloc(size + 1));
}

/* 
 ******************************************************************************
 * ================SIP_copyString()===================
 *
 * This function copies a string to a new block of memory allocated 
 * off the heap.  
 *
 * pSrc = Teh NULL terminated string to copy
 *
 * ppDest = An address to a pointer looking at a string.  If this function is
 *          Successful then the new address of the memory just allocated will 
 *          be populated here.
 *
 * RETURNS: 
 *       SIP_OK: Function successful.
 *       SIP_BADPARM: The pSrc was NULL
 *       SIP_NO_MEM: could not allocate the new memory off the heap
 *         
 ******************************************************************************
 */
vint SIP_copyString(
    char *pSrc,
    char **ppDest)
{
    char *pDest;

    if (NULL == pSrc) {
        SIP_DebugLog(SIP_DB_MEMORY_LVL_1,
                "SIP_copyString: src is NULL!\n", 0, 0, 0);
        SIP_TaskExit();
        return (SIP_BADPARM);
    }

    pDest = SIP_allocString(OSAL_strlen(pSrc));
    if (NULL == pDest) {
        SIP_DebugLog(SIP_DB_MEMORY_LVL_1,
                "SIP_copyString: pDest is NULL!\n", 0, 0, 0);
        SIP_TaskExit();
        return (SIP_NO_MEM);
    }

    OSAL_strcpy(pDest, pSrc);
    *ppDest = pDest;
    return (SIP_OK);
}

/* 
 ******************************************************************************
 * ================ SIP_copyStringToSipText()===================
 *
 * This function copies a string to msg of tSipMsgBody. If the pointer to the tTxtMsg
 * is NULL, it allocates a new tSipMsgBody from memory pool.
 *
 * pSrc = Teh NULL terminated string to copy
 *
 * ppDest = An address to a pointer looking at a tSipMsgBody.
 *
 * RETURNS: 
 *       SIP_OK: Function successful.
 *       SIP_BADPARM: The pSrc was NULL
 *       SIP_NO_MEM: could not allocate the new memory off the memory pool
 *         
 ******************************************************************************
 */
vint SIP_copyStringToSipText(
    char     *pSrc,
    tSipText **ppDest)
{
    tSipText *pDest;

    if (NULL == pSrc) {
        SIP_DebugLog(SIP_DB_MEMORY_LVL_1,
                "SIP_copyStringToSipText: src is NULL!\n", 0, 0, 0);
        SIP_TaskExit();
        return (SIP_BADPARM);
    }

    if (NULL == *ppDest) {
        pDest = (tSipText *)SIP_memPoolAlloc(eSIP_OBJECT_SIP_TEXT);
        if (NULL == pDest) {
            SIP_DebugLog(SIP_DB_MEMORY_LVL_1,
                    "SIP_copyStringToSipText: pDest is NULL!\n", 0, 0, 0);
            SIP_TaskExit();
            return (SIP_NO_MEM);
        }
    }
    else {
        pDest = *ppDest;
    }

    OSAL_strcpy(pDest->msg, pSrc);
    *ppDest = pDest;
    return (SIP_OK);
}

/* 
 ******************************************************************************
 * ================SIP_copySipText()===================
 *
 * This function copies a tSipText. If destination tSipText is NULL, a new block of
 * tSipText will be allocated from memory pool
 * off the heap.  
 *
 * pSrc = The tSipMsgBody to copy
 *
 * ppDest = An address to a pointer of tSipText
 *
 * RETURNS: 
 *       SIP_OK: Function successful.
 *       SIP_BADPARM: The pSrc was NULL
 *       SIP_NO_MEM: could not allocate the new memory off the heap
 *         
 ******************************************************************************
 */
vint SIP_copySipText(
    tSipText  *pSrc,
    tSipText **ppDest)
{
    tSipText *pDest;

    if (NULL == pSrc) {
        SIP_DebugLog(SIP_DB_MEMORY_LVL_1,
                "SIP_copySipText: src is NULL!\n", 0, 0, 0);
        SIP_TaskExit();
        return (SIP_BADPARM);
    }

    if (NULL == *ppDest) {
        pDest = (tSipText *)SIP_memPoolAlloc(eSIP_OBJECT_SIP_TEXT);
        if (NULL == pDest) {
            SIP_DebugLog(SIP_DB_MEMORY_LVL_1,
                    "SIP_copySipText: pDest is NULL!\n", 0, 0, 0);
            SIP_TaskExit();
            return (SIP_NO_MEM);
        }
    }
    else {
        pDest = *ppDest;
    }

    OSAL_strcpy(pDest->msg, pSrc->msg);
    *ppDest = pDest;
    return (SIP_OK);
}

/* 
 ******************************************************************************
 * ================SIP_copySipMsgBody()===================
 *
 * This function copies a tSipMsgBody. If destination tTxtMsg is NULL, a new block of
 * tSipMsgBody will be allocated from memory pool
 * off the heap.  
 *
 * pSrc = The tSipMsgBody to copy
 *
 * ppDest = An address to a pointer of tSipMsgBody.
 *
 * RETURNS: 
 *       SIP_OK: Function successful.
 *       SIP_BADPARM: The pSrc was NULL
 *       SIP_NO_MEM: could not allocate the new memory off the heap
 *         
 ******************************************************************************_ENC_StringHelper
 */
vint SIP_copySipMsgBody(
    tSipMsgBody *pSrc,
    tSipMsgBody **ppDest)
{
    tSipMsgBody *pDest;

    if (NULL == pSrc) {
        SIP_DebugLog(SIP_DB_MEMORY_LVL_1,
                "SIP_copySipMsgBody: src is NULL!\n", 0, 0, 0);
        SIP_TaskExit();
        return (SIP_BADPARM);
    }

    if (NULL == *ppDest) {
        pDest = (tSipMsgBody *)SIP_memPoolAlloc(eSIP_OBJECT_SIP_MSG_BODY);
        if (NULL == pDest) {
            SIP_DebugLog(SIP_DB_MEMORY_LVL_1,
                    "SIP_copySipMsgBody: pDest is NULL!\n", 0, 0, 0);
            SIP_TaskExit();
            return (SIP_NO_MEM);
        }
    }
    else {
        pDest = *ppDest;
    }

    OSAL_strcpy(pDest->msg, pSrc->msg);
    *ppDest = pDest;
    return (SIP_OK);
}

