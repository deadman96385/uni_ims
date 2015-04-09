/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2013 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 20468 $ $Date: 2013-04-22 15:58:21 -0700 (Mon, 22 Apr 2013) $
 */

#ifndef __VHW_H_
#define __VHW_H_
#include <osal.h>
#include <jvhw.h>

/* Generally used definitions */
#define _VHW_STRING_SZ           (128)

/*
 * The maximum amount of time to wait for the command queue to accept new
 * commands that are passed to the JVHW task.
 */
#define _VHW_JVHW_TIMEOUT_MS     (1000)

/*
 * This structure is used to hold the names and identifiers of queues that
 * are used to communicate with the JVHW task.
 */
typedef struct {
    struct {
        OSAL_MsgQId cmdIpcId;
        char cmdIpcName[_VHW_STRING_SZ];
    } cmdQueue;
    struct {
        OSAL_MsgQId rxQueueId;
        char rxIpcName[_VHW_STRING_SZ];
    } rxQueue;
    struct {
        OSAL_MsgQId txQueueId;
        char txIpcName[_VHW_STRING_SZ];
    } txQueue;
} _VHW_Object;
#endif
