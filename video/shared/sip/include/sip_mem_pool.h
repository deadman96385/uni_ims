/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 24709 $ $Date: 2014-02-20 17:27:12 +0800 (Thu, 20 Feb 2014) $
 */
#ifndef _SIP_MEM_POOL_H_
#define _SIP_MEM_POOL_H_

/* Max number of UA's at any given time */
#define SIP_MEM_POOL_SZ_UA              (2)

/*
 * The Maximum number of active dialogs at an given time.
 *
 * Normal UA have...
 * 2 for calls (voice and video)
 * 1 for possible conference (a.k.a merging)
 * 1 for subscription to call conference event package
 * 1 for subscription to registration event package
 * 1 for subscription to dialog event package
 * 20 for subscription to IM conference event package
 * 20 for IM's and file transfers
 * 10 for misc subscriptions
 *
 * Emergency UA's have...
 * 2 for calls
 */
#ifdef VPORT_4G_PLUS_APROC
/* For SAPP running in application processor(with IM) */
#define SIP_DIALOGS_MAX_ACTIVE          (58)
#else
/* For SAPP running in modem processor */
#define SIP_DIALOGS_MAX_ACTIVE          (8)
#endif

#define SIP_MEM_POOL_SZ_TRANS           ((SIP_DIALOGS_MAX_ACTIVE * 5) + 8)
#define SIP_MEM_POOL_SZ_TIMER           ((SIP_MEM_POOL_SZ_TRANS * 3) + 34)
#define SIP_MEM_POOL_SZ_SIP_INT_MSG     ((SIP_MEM_POOL_SZ_TRANS) + (SIP_DIALOGS_MAX_ACTIVE * 3) + 2)
#define SIP_MEM_POOL_SZ_SDP_MSG         (SIP_MEM_POOL_SZ_SIP_INT_MSG / 2)
#define SIP_MEM_POOL_SZ_SDP_CONN_INFO   (SIP_MEM_POOL_SZ_SDP_MSG)
#define SIP_MEM_POOL_SZ_SDP_MEDIA       (SIP_MEM_POOL_SZ_SDP_MSG * 2)

#define SIP_MEM_POOL_SZ_ATTRIBUTE       (SIP_MEM_POOL_SZ_SDP_MEDIA * 4)

#define SIP_MEM_POOL_SZ_SIP_TEXT        (SIP_MEM_POOL_SZ_SIP_INT_MSG)
#define SIP_MEM_POOL_SZ_SIP_MSG_BODY    (SIP_MEM_POOL_SZ_SIP_INT_MSG / 2)
/* Unknown HF */
#define SIP_MEM_POOL_SZ_HF_LIST         ((SIP_MEM_POOL_SZ_SIP_INT_MSG * 10) + (SIP_DIALOGS_MAX_ACTIVE * 10))
#define SIP_MEM_POOL_SZ_VIA_HF          ((SIP_MEM_POOL_SZ_SIP_INT_MSG * 4) + (SIP_DIALOGS_MAX_ACTIVE * 8))
#define SIP_MEM_POOL_SZ_CONTACT_HF      ((SIP_MEM_POOL_SZ_SIP_INT_MSG * 4) + (SIP_DIALOGS_MAX_ACTIVE * 4))
#define SIP_MEM_POOL_SZ_AUTH_HF         (SIP_MEM_POOL_SZ_SIP_INT_MSG * 4)
#define SIP_MEM_POOL_SZ_ROUTE_HF        ((SIP_MEM_POOL_SZ_SIP_INT_MSG * 4) + (SIP_DIALOGS_MAX_ACTIVE * 4))
/* Auth credentials */
#define SIP_MEM_POOL_SZ_AUTH_CRED       (SIP_MAX_NUM_AUTH_CRED * SIP_MEM_POOL_SZ_UA)
/*
 * XXX For subscribe request.
 * Conference, Reg event, watcher list, RLS service * 2.
 */
#define SIP_MEM_POOL_SZ_DIALOG_EVENT    (5)
/* XXX For Notify and Message */
#define SIP_MEM_POOL_SZ_UA_TRANS        (2)
/*
 * One for send, one for tcp receive
 * If ipsec is used, need another two plus one for new socket
 * after tcp accepted
 */
#define SIP_MEM_POOL_SZ_LAYER_4_BUF     (5)
/* XXX Each transaction uses one transport? */
#define SIP_MEM_POOL_SZ_TRANSPORT       (SIP_MEM_POOL_SZ_TRANS)
/* Only for UA epdb, reg, proxy, obProxy and wmProxy */
#define SIP_MEM_POOL_SZ_URI             (4 * SIP_MEM_POOL_SZ_UA)

typedef enum eSipObjType
{
    eSIP_OBJECT_FIRST,
    eSIP_OBJECT_TRANS = eSIP_OBJECT_FIRST,
    eSIP_OBJECT_TIMER,
    eSIP_OBJECT_SIP_INT_MSG,
    eSIP_OBJECT_SDP_MSG,
    eSIP_OBJECT_SDP_CONN_INFO,
    eSIP_OBJECT_SDP_MEDIA,
    eSIP_OBJECT_ATTRIBUTE,
    eSIP_OBJECT_SIP_TEXT,
    eSIP_OBJECT_SIP_MSG_BODY,
    eSIP_OBJECT_HF_LIST,
    eSIP_OBJECT_VIA_HF,
    eSIP_OBJECT_CONTACT_HF,
    eSIP_OBJECT_AUTH_HF,
    eSIP_OBJECT_ROUTE_HF,
    eSIP_OBJECT_AUTH_CRED,
    eSIP_OBJECT_DIALOG_EVENT,
    eSIP_OBJECT_UA_TRANS,
    eSIP_OBJECT_LAYER_4_BUF,
    eSIP_OBJECT_TRANSPORT,
    eSIP_OBJECT_URI,
    eSIP_OBJECT_UA,
    eSIP_OBJECT_NONE,
    eSIP_OBJECT_LAST = eSIP_OBJECT_NONE
}tSipObjType;

typedef struct eSipMemPool
{
    tSipObjType type;
    vint        objSize;
    vint        poolSize;
    tDLList     poolList;
#ifdef SIP_DEBUG_LOG
    vint        minSize; /*
                          * Record for pool usage.
                          * It's minimum remaining pool size,
                          * minus means miss count
                          */
    vint        curSize; /* Current pool size */
#endif
}tSipMemPool;

vint SIP_memPoolInit(
    void);

void *SIP_memPoolAlloc(
    tSipObjType objType);

void SIP_memPoolFree(
    tSipObjType   objType,
    tDLListEntry *pEntry);


void SIP_memPoolDestroy(
    void);

#endif
