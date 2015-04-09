/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004-2010 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Revision$ $Date$
 */

#ifndef _VPMD_MUX_H_
#define _VPMD_MUX_H_

#ifndef VPMD_MUX_DEBUG
#define VPMD_muxDbgPrintf(fmt, args...)
#else
#define VPMD_muxDbgPrintf(fmt, args...) \
         OSAL_logMsg("[%s:%d] " fmt, __FUNCTION__, __LINE__, ## args)
#endif

#define VPMD_FIFO_MSG_COUNT         (12)

#define VPMD_WRITE_TASK_NAME        "VPMD_mux_write_task"
#define VPMD_READ_TASK_NAME         "VPMD_mux_read_task"
#define VPMD_TASK_STACK_BYTES       (1024 * 4)

typedef struct {
    struct {
        MODEM_Terminal  videoCmdEvtQ;
        MODEM_Terminal  videoStreamQ;
        MODEM_Terminal  voiceStreamQ;
        MODEM_Terminal  isiRpcQ;
        MODEM_Terminal  isiEvtRpcQ;
        MODEM_Terminal  sipQ;
        MODEM_Terminal  isipQ;
        MODEM_Terminal  csmEvtQ;
    } outFifo;   /* output FIFO, VPMD will read these FIFO */
    VPR_Comm        vprCommBuffer;
    VPR_Comm        readTaskBuf;
    OSAL_TaskId     readTaskId;
    OSAL_Boolean    isVpadReady;  /* Is VPAD ready. */
} VPMD_muxObj;


OSAL_Status VPMD_muxInit(
    void);

void VPMD_muxDestroy(
    void);

OSAL_Boolean VPMD_muxIsReady(
    void);

#endif // _VPMD_MUX_H_
