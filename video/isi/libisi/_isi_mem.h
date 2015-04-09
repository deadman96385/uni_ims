/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 24709 $ $Date: 2014-02-20 17:27:12 +0800 (Thu, 20 Feb 2014) $
 */
#ifndef _ISI_MEM_H_
#define _ISI_MEM_H_
#include "_isi_list.h"
#include "_isi_port.h"

#define ISI_PORT_LOCK_INIT              NULL

/*  Maximum needed ISI message at the same time could not be over 5. */
#define ISI_MEM_POOL_SZ_ISIP_MSG        (5)
#ifdef INCLUDE_GAPP
/* Memory pool size for GAPP included. */
/* Current services: SAPP, emergency, ASAPP and GAPP. */
#define ISI_MEM_POOL_SZ_SERVICE_ID      (4)
/* Maximum concurrent call */
#define ISI_MEM_POOL_SZ_CALL_ID         (6)
#else
/* Memory pool size for GAPP excluded. */
/* Current services: SAPP, emergency and ASAPP. */
#define ISI_MEM_POOL_SZ_SERVICE_ID      (3)
/*
 * Sip conference call occupies one, plus original two calls,
 * maximum could be 3.
 */
#define ISI_MEM_POOL_SZ_CALL_ID         (3)
#endif

#ifdef INCLUDE_4G_PLUS_RCS
/* Memory pool size for RCS enabled. */
/* Maximum concurrent message transactions. */
#define ISI_MEM_POOL_SZ_TEXT_ID         (20)
/* Maximum concurrent file transfer. */
#define ISI_MEM_POOL_SZ_FILE_ID         (20) 
/* Presence, If does not support RCS , could be decreased. */
#define ISI_MEM_POOL_SZ_PRES_ID         (20)
/* Maximum concurrent chat sessions. */
#define ISI_MEM_POOL_SZ_GCHAT_ID        (20)
#else
/* Memory pool size for RCS disabled. */
/* Maximum concurrent message transactions. */
#define ISI_MEM_POOL_SZ_TEXT_ID         (5)
/* Maximum concurrent file transfer. */
#define ISI_MEM_POOL_SZ_FILE_ID         (0)
/* Presence, If does not support RCS , could be decreased. */
#define ISI_MEM_POOL_SZ_PRES_ID         (ISI_MEM_POOL_SZ_CALL_ID)
/* Maximum concurrent chat sessions. */
#define ISI_MEM_POOL_SZ_GCHAT_ID        (0)
#endif

/*
 * DTMF event. The tel event is sequential,
 * but could be sent and received at the same time.
 */
#define ISI_MEM_POOL_SZ_EVT_ID          (5)
/* CS conference call. */
#define ISI_MEM_POOL_SZ_CONF_ID         (2)
/* Maximum concurrent USSD */
#define ISI_MEM_POOL_SZ_USSD_ID         (1) 
/* One account per service. */
#define ISI_MEM_POOL_SZ_ACCOUNT         (ISI_MEM_POOL_SZ_SERVICE_ID) 

typedef struct {
    ISI_objType isiObjType;
    vint        isiObjSize;
    vint        isiPoolSize;
    ISIL_List   isiPoolList;
#ifdef ISI_MEM_DEBUG_LOG
    vint        isiMemMinSz; 
                /*
                 * Record for pool usage. 
                 * It's minimum remaining pool size,  minus means miss count
                 */
    vint        isiMemCurSz; /* Current pool size */
#endif
}ISIMP_MemPool;

vint ISIMP_memPoolInit(
    void);

void *ISIMP_memPoolAlloc(
    ISI_objType objType);

void ISIMP_memPoolFree(
    ISI_objType   objType,
    ISIL_ListEntry *pEntry);

void ISIMP_memPoolDestroy(
    void);

#endif
