/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004-2010 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Revision$ $Date$
 */

#ifndef _VPAD_MUX_H_
#define _VPAD_MUX_H_

#ifndef VPAD_MUX_DEBUG
#define VPAD_muxDbgPrintf(fmt, args...)
#else
#define VPAD_muxDbgPrintf(fmt, args...) \
         OSAL_logMsg("[%s:%d] " fmt, __FUNCTION__, __LINE__, ## args)
#endif

#define VPAD_FIFO_MSG_COUNT            (12)

#define VPAD_WRITE_TASK_NAME           "vpad_mux_write_task"
#define VPAD_READ_TASK_NAME            "vpad_mux_read_task"
#define VPAD_TASK_STACK_BYTES          (1024 * 4)

/* XXX This recoery delay should be reconsider with modem reset issue. */
#define VPAD_MUX_ERROR_RECOVERY_DELAY  (1000)
#define VPAD_MUX_STAT_TIMER            (4)

#if (D2_LITTLE_ENDIAN && VPMD_BIG_ENDIAN) || \
        (D2_BIG_ENDIAN && VPMD_LITTLE_ENDIAN)
/*
 * The endian type is different between application processor
 * and modem processor, so need to swap data endian type.
 */
#define _VPAD_MUX_SWAP_VPR_COMM_ENDIAN(a, b) \
        _VPAD_muxSwapVprCommEndian(a, b);
#else
#define _VPAD_MUX_SWAP_VPR_COMM_ENDIAN(a, b)

#endif

#ifdef VPAD_MUX_STATS

#define _VPAD_MUX_STATS_UPDATE(q_ptr, size) \
    _VPAD_muxStatsUpdate(q_ptr, size)

#define _VPAD_MUX_COLLECT_VIDEO_COUNTERS(vpr_ptr) \
    _VPAD_muxCollectVideoCounters(vpr_ptr)

#define _VPAD_MUX_STATS_REPORT() _VPAD_muxStatsReport()

#define _VPAD_MUX_DUMP_ONE_STAT(MODEM_TERM) \
        OSAL_logMsg("%36s \t %d \t %d\n", \
                    (MODEM_TERM).name, \
                    (MODEM_TERM).pktCount, \
                    (MODEM_TERM).szCount);

typedef struct {
    vint vtspCmdCount;
    vint vtspRtcpCmdCount;
    vint vtspRtcpEventMsgCount;
    vint vtspEvtCount;
    vint vprNetCount;
} VPAD_muxVideoQCounters;
#else

#define _VPAD_MUX_STATS_UPDATE(q_ptr, size)
#define _VPAD_MUX_STATS_REPORT() 
#define _VPAD_MUX_COLLECT_VIDEO_COUNTERS(vpr_ptr)

#endif

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
    } inFifo;  /* write data to device, the data is from VPAD */
    struct {
        MODEM_Terminal  videoCmdEvtQ;
        MODEM_Terminal  videoStreamQ;
        MODEM_Terminal  voiceStreamQ;
        MODEM_Terminal  isiRpcQ;
        MODEM_Terminal  isiEvtRpcQ;
        MODEM_Terminal  sipQ;
        MODEM_Terminal  isipQ;
        MODEM_Terminal  csmEvtQ;
    } outFifo;   /* read data from device, VPAD will read the data */
    VPR_Comm        vprCommBuffer;
    VPR_Comm        writeTaskBuf;
    VPR_Comm        readTaskBuf;
    OSAL_TaskId     writeTaskId;
    OSAL_TaskId     readTaskId;
    OSAL_Boolean    isVpmdReady; /* Indicate if VPMD is ready. */
#ifdef VPAD_MUX_STATS
    OSAL_TimeVal    lastStatTimeStamp;
    VPAD_muxVideoQCounters videoCounters;
#endif
} VPAD_muxObj;

#endif // _VPAD_MUX_H_
