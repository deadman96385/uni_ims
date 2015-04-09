/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004-2010 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev$ $Date$
 *
 */

#ifndef _GBAM_H_
#define _GBAM_H_

#include "gba.h"
#include "_csm_event.h"

#define GBAM_TASK_NAME   "GBAM"
#define GBAM_TASK_SIZE   (1024)

typedef enum {
    GBAM_AKA_ISI_REQ = 1,
    GBAM_AKA_GBA_REQ = 2
} GBAM_AkaArbitor;

/*
 * This is the GBAM global object.
 */
typedef struct {
    OSAL_TaskId     tId;
    OSAL_MsgQId     csmIsimQ;    // Output from CSM
    OSAL_MsgQId     csmEventInQ; // Input to CSM
    OSAL_SemId      akaLock;
    GBAM_AkaArbitor lastAkaTarget;
    GBA_Command     gbaCommand;
} GBAM_GlobalObj;



/*
 * CSM GBA message task public methods
 */
vint GBAM_init();
vint GBAM_shutdown();

vint GBAM_akaArbitorSetTarget(
        GBAM_AkaArbitor akaRequester);

GBAM_AkaArbitor GBAM_akaArbitorGetTarget(void);

void GBAM_eventsTask(
    void *arg_ptr);

vint GBAM_sendAkaAuthResp(
   CSM_ServiceEvt *serviceEvt_ptr);

#endif
