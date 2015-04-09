/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2013 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Revision$ $Date$
 */
#include <osal.h>
#include <osal_net.h>
#include <vpr_comm.h>
#include <vpad_vpmd.h>

#define VPMD_TEST_TASK_NAME         "vpmd_test_task"
#define VPMD_TEST_MSGQ_DEPTH        (2)
#define VPMD_TEST_TASK_STACK_BYTES  (4096)

#define VPMD_ECHO_TASK_NAME         "vpmd_echo_task"
#define VPMD_ECHO_MSGQ_DEPTH        (2)
#define VPMD_ECHO_TASK_STACK_BYTES  (4096)

#define VPMD_CHECK_TASK_NAME         "vpmd_check_task"
#define VPMD_CHECK_MSGQ_DEPTH        (2)
#define VPMD_CHECK_TASK_STACK_BYTES  (4096)

#define VPMD_TEST_DEFAULT_XBATCH    (8)
#define VPMD_TEST_DEFAULT_XGAP      (33)
#define VPMD_TEST_DEFAULT_XTOTAL    (1200)
#define VPMD_TEST_DEFAULT_YGAP      (0)
#define VPMD_TEST_DEFAULT_ECHO      (OSAL_FALSE)

#define D2_VPORT_REVISION ""

typedef struct {
    int videoStreamCounter;
    int videoCmdEvtCounter;
    int voiceStreamCounter;
    int sipCounter;
    int isipCounter;
    int csmEvtCounter;
} VPMD_TestCounter;
    
/* Struct of VPMD_TestObj */
typedef struct {
    OSAL_TaskId     vpmdTestTaskId;
    OSAL_TaskId     vpmdEchoTaskId;
    OSAL_TaskId     vpmdCheckTxRxTaskId;
    vint            xBatch; /* e.g. 8 frames per batch */
    vint            xGap; /* e.g. 33ms for 30fps */
    vint            xTotal; /* total number of messages to stress test, 2400 for 10sec */
    vint            yGap; /* small gap between send */
    OSAL_Boolean    echo; /* echo mode */
    VPR_Comm        commVpmd; /* For vpmd testing use */
    VPR_Comm        commRxVpmd; /* For vpmd testing use */
    unsigned char   dataSeed;
    unsigned char   expectDataSeed;
    struct {
        int vpmdVideoStreamFd;
        int vpmdVideoCmdEvtFd;
        int vpmdVoiceStreamFd;
        int vpmdSipFd;
        int vpmdIsipFd;
        int vpmdCsmEvtFd;
    } fd;
    VPMD_TestCounter txCounters;
    VPMD_TestCounter rxCounters;
} VPMD_TestObj;


static VPMD_TestObj _VPMD_testObj;
static VPMD_TestObj *_VPMD_testObj_ptr = &_VPMD_testObj;

#ifndef VPMD_IO_DEBUG
#define VPMD_muxDbgPrintf(fmt, args...)
#else
#define VPMD_muxDbgPrintf(fmt, args...) \
         OSAL_logMsg("[%s:%d] " fmt, __FUNCTION__, __LINE__, ## args)
#endif

#ifdef OSAL_THREADX
static OSAL_SemId VPMD_testEndSem = NULL;
static OSAL_SemId VPMD_testStreamDoneSem = NULL;
#endif
/*
 * ======== _VPMD_testEchoTask() ========
 *
 * This task is to wait on all VPMD's fds, receive data from VPMD
 * and echo the data back to VPAD.
 *
 * Returns:
 * None
 */
static OSAL_TaskReturn _VPMD_testEchoTask(
    OSAL_TaskArg arg_ptr)
{
    OSAL_SelectSet   readSet;
    OSAL_Boolean     boolean;
    VPMD_TestObj    *vpmdTestObj_ptr;
    VPR_Comm        *comm_ptr;
    OSAL_Boolean       isTimeout;
    OSAL_SelectTimeval tv;

    /* Wait 10 second. */
    tv.sec = 0;
    tv.usec = 10000;

    vpmdTestObj_ptr = (VPMD_TestObj*) arg_ptr;
    comm_ptr = &vpmdTestObj_ptr->commVpmd;

    while (OSAL_FALSE == VPMD_isReady()) {
        OSAL_logMsg("%s:%d wait 1 sec for vpmd ready\n",
            __FUNCTION__, __LINE__);
        OSAL_taskDelay(1000);
    }

__VPMD_TASK_LOOP:
    /*
     * Group fd set.
     */
    OSAL_selectSetInit(&readSet);

    /* Add vpmd video stream fd for select */
    OSAL_selectAddId(&vpmdTestObj_ptr->fd.vpmdVideoStreamFd, &readSet);
    /* Add vpmd video cmd evt fd for select */
    OSAL_selectAddId(&vpmdTestObj_ptr->fd.vpmdVideoCmdEvtFd, &readSet);
    /* Add vpmd voice stream fd for select */
    OSAL_selectAddId(&vpmdTestObj_ptr->fd.vpmdVoiceStreamFd, &readSet);
    /* Add vpmd sip fd for select */
    OSAL_selectAddId(&vpmdTestObj_ptr->fd.vpmdSipFd, &readSet);
    /* Add vpmd isip fd for select */
    OSAL_selectAddId(&vpmdTestObj_ptr->fd.vpmdIsipFd, &readSet);
    /* Add vpmd csm event fd for select */
    OSAL_selectAddId(&vpmdTestObj_ptr->fd.vpmdCsmEvtFd, &readSet);

    /* Wait forever */
    if (OSAL_FAIL == OSAL_select(&readSet, NULL, &tv, &isTimeout)) {
        OSAL_logMsg("%s:%d time out to seletec fd for echo test.\n",
             __FUNCTION__, __LINE__);
        goto __VPMD_TASK_LOOP;
    }

    /* See if it's command from video stream */
    OSAL_selectIsIdSet(&vpmdTestObj_ptr->fd.vpmdVideoStreamFd, &readSet, &boolean);
    if (OSAL_TRUE == boolean) {
        /* Read VPR command from vpad */
        if (OSAL_SUCCESS == VPMD_readVideoStream(comm_ptr, sizeof(VPR_Comm),
                OSAL_WAIT_FOREVER)) {
            /* Got VPR_Comm, echo it */
            VPMD_writeVideoStream(comm_ptr, sizeof(VPR_Comm));
        }
    }
    /* See if it's command from video stream */
    OSAL_selectIsIdSet(&vpmdTestObj_ptr->fd.vpmdVideoCmdEvtFd, &readSet, &boolean);
    if (OSAL_TRUE == boolean) {
        /* Read VPR command from vpad */
        if (OSAL_SUCCESS == VPMD_readVideoCmdEvt(comm_ptr, sizeof(VPR_Comm),
                OSAL_WAIT_FOREVER)) {
            /* Got VPR_Comm, echo it */
            VPMD_writeVideoCmdEvt(comm_ptr, sizeof(VPR_Comm));
        }
    }
    /* See if it's command from video stream */
    OSAL_selectIsIdSet(&vpmdTestObj_ptr->fd.vpmdVoiceStreamFd, &readSet, &boolean);
    if (OSAL_TRUE == boolean) {
        /* Read VPR command from vpad */
        if (OSAL_SUCCESS == VPMD_readVoiceStream(comm_ptr, sizeof(VPR_Comm),
                OSAL_WAIT_FOREVER)) {
            /* Got VPR_Comm, echo it */
            VPMD_writeVoiceStream(comm_ptr, sizeof(VPR_Comm));
        }
    }
    /* See if it's sip packet from sr */
    OSAL_selectIsIdSet(&vpmdTestObj_ptr->fd.vpmdSipFd, &readSet, &boolean);
    if (OSAL_TRUE == boolean) {
        /* Read VPR command from vpad */
        if (OSAL_SUCCESS == VPMD_readSip(comm_ptr, sizeof(VPR_Comm),
                OSAL_WAIT_FOREVER)) {
            /* Got VPR_Comm, echo it */
            VPMD_writeSip(comm_ptr, sizeof(VPR_Comm));
        }
    }
    /* See if it's isip message from sr */
    OSAL_selectIsIdSet(&vpmdTestObj_ptr->fd.vpmdIsipFd, &readSet, &boolean);
    if (OSAL_TRUE == boolean) {
        /* Read VPR command from vpad */
        if (OSAL_SUCCESS == VPMD_readIsip(comm_ptr, sizeof(VPR_Comm),
                OSAL_WAIT_FOREVER)) {
            /* Got VPR_Comm, echo it */
            VPMD_writeIsip(comm_ptr, sizeof(VPR_Comm));
        }
    }
    /* See if it's csm event from rir */
    OSAL_selectIsIdSet(&vpmdTestObj_ptr->fd.vpmdCsmEvtFd, &readSet, &boolean);
    if (OSAL_TRUE == boolean) {
        /* Read VPR command from vpad */
        if (OSAL_SUCCESS == VPMD_readCsmEvt(comm_ptr, sizeof(VPR_Comm),
                OSAL_WAIT_FOREVER)) {
            /* Got csm_event, log it since we dont' have csm write to vpad */
            OSAL_logMsg("Got csm event from RIR\n");
            // VPMD_writeCsmEvt(comm_ptr, sizeof(VPR_Comm));
#ifdef OSAL_THREADX
            /* sem give for end of echo test. */
            OSAL_semGive(VPMD_testEndSem);
#endif
        }
    }
    goto __VPMD_TASK_LOOP;

    return (0);
}


/*
 * ======== VPMD_testInit() ========
 *
 * Private function to initialize VPMD test.
 *
 * Returns:
 * OSAL_SUCCESS: echo initialized successfully.
 * OSAL_FAIL: Error in echo initialization.
 */
OSAL_Status VPMD_testInit(
    VPMD_TestObj *vpmdTestObj_ptr)
{
    VPMD_init();

#ifdef OSAL_THREADX
    if (NULL == VPMD_testStreamDoneSem) {
        VPMD_testStreamDoneSem = OSAL_semBinaryCreate(OSAL_SEMB_UNAVAILABLE);
    }
#endif

    /* Get voice stream device fd from VPAD */
    if (0 == (vpmdTestObj_ptr->fd.vpmdVoiceStreamFd =
            VPMD_getVoiceStreamReadFd())) {
        OSAL_logMsg("%s:%d Fail to get voice stream read fd from VPMD.\n",
             __FUNCTION__, __LINE__);
        return (OSAL_FAIL);
    }

    /* Get video event device fd from VPAD */
    if (0 == (vpmdTestObj_ptr->fd.vpmdVideoCmdEvtFd =
            VPMD_getVideoCmdEvtReadFd())) {
        OSAL_logMsg("%s:%d Fail to get video event read fd from VPMD.\n",
             __FUNCTION__, __LINE__);
        return (OSAL_FAIL);
    }

    /* Get video stream device fd from VPAD */
    if (0 == (vpmdTestObj_ptr->fd.vpmdVideoStreamFd =
            VPMD_getVideoStreamReadFd())) {
        OSAL_logMsg("%s:%d Fail to get video stream read fd from VPMD.\n",
             __FUNCTION__, __LINE__);
        return (OSAL_FAIL);
    }

    /* Get sip device fd from VPAD */
    if (0 == (vpmdTestObj_ptr->fd.vpmdSipFd =
            VPMD_getSipReadFd())) {
        OSAL_logMsg("%s:%d Fail to get sip read fd from VPMD.\n",
             __FUNCTION__, __LINE__);
        return (OSAL_FAIL);
    }

    /* Get isip device fd from VPAD */
    if (0 == (vpmdTestObj_ptr->fd.vpmdIsipFd =
            VPMD_getIsipReadFd())) {
        OSAL_logMsg("%s:%d Fail to get isip read fd from VPMD.\n",
             __FUNCTION__, __LINE__);
        return (OSAL_FAIL);
    }

    /* Get video stream device fd from VPAD */
    if (0 == (vpmdTestObj_ptr->fd.vpmdCsmEvtFd =
            VPMD_getCsmEvtReadFd())) {
        OSAL_logMsg("%s:%d Fail to get csm event read fd from VPMD.\n",
             __FUNCTION__, __LINE__);
        return (OSAL_FAIL);
    }

    return (OSAL_SUCCESS);
}

/*
 * ======== VPMD_testEchoShutdown() ========
 *
 * Private function to shutdown VPR.
 *
 * Returns:
 */
void VPMD_testEchoShutdown(
    VPMD_TestObj *vpmdTestObj_ptr)
{
    VPMD_destroy();
}

/*
 * ======== _VPMD_testTxRxStress() ========
 *
 * This task is to write stressing stream to vpmd
 *
 * Returns:
 *  OSAL_SUCCESS: success
 *  OSAL_FAIL: fail
 */
static OSAL_Status _VPMD_testTxRxStress(
    VPMD_TestObj    *vpmdTestObj_ptr)
{
    VPR_Comm    *comm_ptr;
    OSAL_Status result;
    vint        count, txOkCount, rxOkCount, batch;
    OSAL_TimeVal beginTime, endTime, durationTime;

    OSAL_logMsg("%s:%d stressing VPAD video stream tx/rx testing...\n",
         __FUNCTION__, __LINE__);

    OSAL_logMsg("Testing Setup :\n");
    OSAL_logMsg(" --xBatch=%d\n", vpmdTestObj_ptr->xBatch);
    OSAL_logMsg(" --xGap=%d\n", vpmdTestObj_ptr->xGap);
    OSAL_logMsg(" --xTotal=%d\n", vpmdTestObj_ptr->xTotal);
    OSAL_logMsg(" --yGap=%d\n", vpmdTestObj_ptr->yGap);
    
    comm_ptr = &vpmdTestObj_ptr->commVpmd;
    count = txOkCount = rxOkCount = vpmdTestObj_ptr->rxCounters.videoStreamCounter = 0;
    OSAL_timeGetTimeOfDay(&beginTime);

    while (count < vpmdTestObj_ptr->xTotal) {
        for (batch=0; batch<vpmdTestObj_ptr->xBatch; batch++) {
            /* checking tx/rx for video stream */
            OSAL_memSet(comm_ptr, (int)vpmdTestObj_ptr->dataSeed++, sizeof(VPR_Comm));
            result = VPMD_writeVideoStream(comm_ptr, sizeof(VPR_Comm));
            if (OSAL_SUCCESS == result) {
                txOkCount += 1;
            };
            count += 1;
            if (0 != vpmdTestObj_ptr->yGap) {
                OSAL_taskDelay(vpmdTestObj_ptr->yGap);
            }
        }

        if (0 != vpmdTestObj_ptr->xGap) {
            OSAL_taskDelay(vpmdTestObj_ptr->xGap);
        }
    }

    rxOkCount = vpmdTestObj_ptr->rxCounters.videoStreamCounter;
    
    OSAL_timeGetTimeOfDay(&endTime);
    if (endTime.usec >= beginTime.usec) {
        durationTime.usec = endTime.usec - beginTime.usec;
        durationTime.sec = endTime.sec - beginTime.sec;
    } else {
        durationTime.usec = 1000*1000 + endTime.usec - beginTime.usec;
        durationTime.sec = endTime.sec - beginTime.sec - 1;
    }

    OSAL_logMsg("right after tx, rxOkCount=%d, to wait next exchange \n", rxOkCount);
    
    /* checking tx/rx for video stream */
    OSAL_memSet(comm_ptr, vpmdTestObj_ptr->dataSeed, sizeof(VPR_Comm));
    result = VPMD_writeVoiceStream(comm_ptr, sizeof(VPR_Comm));
    OSAL_logMsg("%s:%d VPMD_writeVoiceStream result=%d\n",
            __FUNCTION__, __LINE__, result);

#ifdef OSAL_THREADX
    OSAL_semAcquire(VPMD_testStreamDoneSem, OSAL_WAIT_FOREVER);
#endif

    rxOkCount = vpmdTestObj_ptr->rxCounters.videoStreamCounter;

    OSAL_logMsg("Testing Setup :\n");
    OSAL_logMsg(" --xBatch=%d\n", vpmdTestObj_ptr->xBatch);
    OSAL_logMsg(" --xGap=%d\n", vpmdTestObj_ptr->xGap);
    OSAL_logMsg(" --xTotal=%d\n", vpmdTestObj_ptr->xTotal);
    OSAL_logMsg(" --yGap=%d\n", vpmdTestObj_ptr->yGap);

    OSAL_logMsg("Stress Testing Results :\n");
    OSAL_logMsg(" total=%d\n", count);
    OSAL_logMsg(" txOkCount=%d\n", txOkCount);
    OSAL_logMsg(" rxOkCount=%d\n", rxOkCount);
    OSAL_logMsg(" duration sec=%d msec=%d\n", durationTime.sec, durationTime.usec/1000);

    OSAL_logMsg("%s:%d seed info expect 0x%0x, last 0x%0x\n",
            __FUNCTION__, __LINE__,
            vpmdTestObj_ptr->expectDataSeed,
            vpmdTestObj_ptr->dataSeed);

    if (txOkCount == rxOkCount) {
        OSAL_logMsg("Stress Testing OK\n");
        return (OSAL_SUCCESS);
    } else {
        OSAL_logMsg("Stress Testing FAILED\n");
        return (OSAL_FAIL);
    }
}

/*
 * ======== _VPMD_testTxRxTask() ========
 *
 * This task is to write on all VPMD's fds,
 * and receive the echoed data.
 *
 * Returns:
 * OSAL_TaskReturn
 */
static OSAL_TaskReturn _VPMD_testTxRxTask(
    OSAL_TaskArg arg)
{
    VPMD_TestObj *vpmdTestObj_ptr = (VPMD_TestObj*)arg;

    OSAL_logMsg("%s:%d check for vpmd ready\n",
            __FUNCTION__, __LINE__);
    while (OSAL_FALSE == VPMD_isReady()) {
        OSAL_logMsg("%s:%d wait 1 sec for vpmd ready\n",
            __FUNCTION__, __LINE__);
        OSAL_taskDelay(1000);
    }

    _VPMD_testTxRxStress(vpmdTestObj_ptr);

#ifdef OSAL_THREADX
    OSAL_taskDelay(1000);
    /* sem give for end of test. */
    OSAL_semGive(VPMD_testEndSem);
#endif
    OSAL_logMsg("%s:%d all tests are done, press control-c to exit testing\n",
            __FUNCTION__, __LINE__);

    return (0);
}


/*
 * ======== _VPMD_testCheckTxRxTask() ========
 *
 * This task is to wait on all VPMD's fds, receive data from VPMD
 * and update the tx/rx counters
 *
 * Returns:
 * None
 */
static OSAL_TaskReturn _VPMD_testCheckTxRxTask(
    OSAL_TaskArg arg_ptr)
{
    OSAL_SelectSet   readSet;
    OSAL_Boolean     boolean;
    VPMD_TestObj    *vpmdTestObj_ptr;
    VPR_Comm        *comm_ptr;
    unsigned char   *seedPtr;

    vpmdTestObj_ptr = (VPMD_TestObj*) arg_ptr;
    comm_ptr = &vpmdTestObj_ptr->commRxVpmd;
    seedPtr = (unsigned char *)comm_ptr;

    while (OSAL_FALSE == VPMD_isReady()) {
        OSAL_logMsg("%s:%d wait 1 sec for vpmd ready\n",
            __FUNCTION__, __LINE__);
        OSAL_taskDelay(1000);
    }
    
__VPMD_TASK_LOOP:
    /*
     * Group fd set.
     */
    OSAL_selectSetInit(&readSet);

    /* Add vpmd video stream fd for select */
    OSAL_selectAddId(&vpmdTestObj_ptr->fd.vpmdVideoStreamFd, &readSet);
    /* Add vpmd video cmd evt fd for select */
    OSAL_selectAddId(&vpmdTestObj_ptr->fd.vpmdVideoCmdEvtFd, &readSet);
    /* Add vpmd voice stream fd for select */
    OSAL_selectAddId(&vpmdTestObj_ptr->fd.vpmdVoiceStreamFd, &readSet);
    /* Add vpmd sip fd for select */
    OSAL_selectAddId(&vpmdTestObj_ptr->fd.vpmdSipFd, &readSet);
    /* Add vpmd isip fd for select */
    OSAL_selectAddId(&vpmdTestObj_ptr->fd.vpmdIsipFd, &readSet);
    /* Add vpmd csm event fd for select */
    OSAL_selectAddId(&vpmdTestObj_ptr->fd.vpmdCsmEvtFd, &readSet);

    /* Wait forever */
    if (OSAL_FAIL == OSAL_select(&readSet, NULL, NULL, NULL)) {
        OSAL_logMsg("%s:%d Failed to seletec fd for echo test.\n",
             __FUNCTION__, __LINE__);
        return (OSAL_FAIL);
    }

    /* See if it's  from video stream */
    OSAL_selectIsIdSet(&vpmdTestObj_ptr->fd.vpmdVideoStreamFd, &readSet, &boolean);
    if (OSAL_TRUE == boolean) {
        /* Read VPR command from vpad */
        if (OSAL_SUCCESS == VPMD_readVideoStream(comm_ptr, sizeof(VPR_Comm),
                OSAL_WAIT_FOREVER)) {
            if (vpmdTestObj_ptr->expectDataSeed == (*seedPtr)) {
                vpmdTestObj_ptr->expectDataSeed += 1;
            } else {
                 OSAL_logMsg("%s:%d mismached seeed in vpmd pkt: expect%0x, got%0x\n",
                    __FUNCTION__, __LINE__,
                    vpmdTestObj_ptr->expectDataSeed,
                    (*seedPtr));
                vpmdTestObj_ptr->expectDataSeed = (*seedPtr)+1;
            }
            vpmdTestObj_ptr->rxCounters.videoStreamCounter += 1;
            VPMD_muxDbgPrintf("video Stream Counter=%d\n",
                    vpmdTestObj_ptr->rxCounters.videoStreamCounter);
        }
    }
    /* See if it's command from video stream */
    OSAL_selectIsIdSet(&vpmdTestObj_ptr->fd.vpmdVideoCmdEvtFd, &readSet, &boolean);
    if (OSAL_TRUE == boolean) {
        /* Read VPR command from vpad */
        if (OSAL_SUCCESS == VPMD_readVideoCmdEvt(comm_ptr, sizeof(VPR_Comm),
                OSAL_WAIT_FOREVER)) {
            /* Got VPR_Comm, echo it */
            vpmdTestObj_ptr->rxCounters.videoCmdEvtCounter += 1;
        }
    }
    /* See if it's from voice stream */
    OSAL_selectIsIdSet(&vpmdTestObj_ptr->fd.vpmdVoiceStreamFd, &readSet, &boolean);
    if (OSAL_TRUE == boolean) {
        /* Read VPR command from vpad */
        if (OSAL_SUCCESS == VPMD_readVoiceStream(comm_ptr, sizeof(VPR_Comm),
                OSAL_WAIT_FOREVER)) {
            /* Got VPR_Comm, echo it */
            vpmdTestObj_ptr->rxCounters.voiceStreamCounter += 1;
            VPMD_muxDbgPrintf("voice stream Counter=%d\n", 
                    vpmdTestObj_ptr->rxCounters.voiceStreamCounter);
#ifdef OSAL_THREADX
            OSAL_semGive(VPMD_testStreamDoneSem);
#endif
        }
    }
    /* See if it's sip packet from sip */
    OSAL_selectIsIdSet(&vpmdTestObj_ptr->fd.vpmdSipFd, &readSet, &boolean);
    if (OSAL_TRUE == boolean) {
        /* Read VPR command from vpad */
        if (OSAL_SUCCESS == VPMD_readSip(comm_ptr, sizeof(VPR_Comm),
                OSAL_WAIT_FOREVER)) {
            /* Got VPR_Comm, echo it */
            vpmdTestObj_ptr->rxCounters.sipCounter += 1;
        }
    }
    /* See if it's isip message from isip */
    OSAL_selectIsIdSet(&vpmdTestObj_ptr->fd.vpmdIsipFd, &readSet, &boolean);
    if (OSAL_TRUE == boolean) {
        /* Read VPR command from vpad */
        if (OSAL_SUCCESS == VPMD_readIsip(comm_ptr, sizeof(VPR_Comm),
                OSAL_WAIT_FOREVER)) {
            /* Got VPR_Comm, echo it */
            vpmdTestObj_ptr->rxCounters.isipCounter += 1;
        }
    }
    /* See if it's csm event from rir */
    OSAL_selectIsIdSet(&vpmdTestObj_ptr->fd.vpmdCsmEvtFd, &readSet, &boolean);
    if (OSAL_TRUE == boolean) {
        /* Read VPR command from vpad */
        if (OSAL_SUCCESS == VPMD_readCsmEvt(comm_ptr, sizeof(VPR_Comm),
                OSAL_WAIT_FOREVER)) {
            /* Got csm_event, log it since we dont' have csm write to vpad */
            OSAL_logMsg("Got csm event from RIR\n");
            vpmdTestObj_ptr->rxCounters.csmEvtCounter += 1;
        }
    }
    goto __VPMD_TASK_LOOP;

    return (0);
}

/*
 * ======== VPMD_testEcho() ========
 *
 * This function is main echo testing entry point
 *
 * Returns:
 * OSAL_TaskReturn
 */
int VPMD_testEcho()
{
    /* Register the routine to call when the process is being terminated */
    if (OSAL_SUCCESS != OSAL_condApplicationExitRegister()) {
        return (-1);
    }

    /* Initialize vpmd and ready for listening. */
    if (OSAL_SUCCESS != VPMD_testInit(_VPMD_testObj_ptr)) {
        OSAL_logMsg("%s:%d Failed creating VPMD_testInit\n",
                    __FUNCTION__, __LINE__);
        OSAL_condApplicationExitUnregister();
        return (-1);
    }

    /* Create VPR vpmd echo task */
    if (0 == (_VPMD_testObj_ptr->vpmdEchoTaskId = OSAL_taskCreate(
            VPMD_ECHO_TASK_NAME,
            OSAL_TASK_PRIO_NRT,
            VPMD_ECHO_TASK_STACK_BYTES,
            _VPMD_testEchoTask,
            (void *)_VPMD_testObj_ptr))) {
        OSAL_logMsg("%s:%d Failed creating %s daemon\n",
                __FUNCTION__, __LINE__, VPMD_ECHO_TASK_NAME);
        return (OSAL_FAIL);
    }

/* Block this process until it's time to terminate */
#ifdef OSAL_THREADX
    OSAL_semAcquire(VPMD_testEndSem, OSAL_WAIT_FOREVER);
#endif
    OSAL_condApplicationExitWaitForCondition();
    OSAL_condApplicationExitUnregister();

    /* Executable has exited shutdown */
    OSAL_taskDelete(_VPMD_testObj_ptr->vpmdEchoTaskId);
    VPMD_destroy();
    OSAL_logMsg("%s:%d Funtion Exit!\n", __FUNCTION__, __LINE__);

    return (0);
}

/*
 * ======== VPMD_testStress() ========
 *
 * This function is main stress testing entry point
 *
 * Returns:
 * OSAL_TaskReturn
 */
int VPMD_testStress(
    int xBatch,
    int xGap,
    int xTotal,
    int yGap)
{
    _VPMD_testObj_ptr->xBatch = xBatch;
    _VPMD_testObj_ptr->xGap = xGap;
    _VPMD_testObj_ptr->xTotal = xTotal;
    _VPMD_testObj_ptr->yGap = yGap;
                
    /* Register the routine to call when the process is being terminated */
    if (OSAL_SUCCESS != OSAL_condApplicationExitRegister()) {
        return (-1);
    }

    /* Initialize vpmd and ready for listening. */
    if (OSAL_SUCCESS != VPMD_testInit(_VPMD_testObj_ptr)) {
        OSAL_logMsg("%s:%d Failed creating VPMD_testInit\n",
                    __FUNCTION__, __LINE__);
        OSAL_condApplicationExitUnregister();
        return (-1);
    }
    
    /* Run test tasks */
    /* Create VPAD testing task */
    if (0 == (_VPMD_testObj_ptr->vpmdTestTaskId = OSAL_taskCreate(
            VPMD_TEST_TASK_NAME,
            OSAL_TASK_PRIO_DEC20,
            VPMD_TEST_TASK_STACK_BYTES,
            _VPMD_testTxRxTask,
            (void *)_VPMD_testObj_ptr))) {
        OSAL_logMsg("%s:%d Failed creating %s vpmd testing task\n",
                __FUNCTION__, __LINE__, VPMD_TEST_TASK_NAME);
        return (OSAL_FAIL);
    }

    /* Create VPR vpmd checking tx/rx task */
    if (0 == (_VPMD_testObj_ptr->vpmdCheckTxRxTaskId = OSAL_taskCreate(
            VPMD_CHECK_TASK_NAME,
            OSAL_TASK_PRIO_DEC20,
            VPMD_CHECK_TASK_STACK_BYTES,
            _VPMD_testCheckTxRxTask,
            (void *)_VPMD_testObj_ptr))) {
        OSAL_logMsg("%s:%d Failed creating %s daemon\n",
                __FUNCTION__, __LINE__, VPMD_CHECK_TASK_NAME);
        return (OSAL_FAIL);
    }
    
    /* Block this process until it's time to terminate */
#ifdef OSAL_THREADX
    OSAL_semAcquire(VPMD_testEndSem, OSAL_WAIT_FOREVER);
#endif
    OSAL_condApplicationExitWaitForCondition();
    OSAL_condApplicationExitUnregister();

    /* Executable has exited shutdown */
    OSAL_taskDelete(_VPMD_testObj_ptr->vpmdTestTaskId);
    OSAL_taskDelete(_VPMD_testObj_ptr->vpmdCheckTxRxTaskId);
    
    VPMD_destroy();
    
    return (0);
}

static void _VPMD_testShowUsage(void)
{
    OSAL_logMsg("Testing VPAD/VPMD comm\n");
    OSAL_logMsg("Usage   : vpmd_test [Option]\n");
    OSAL_logMsg("Options :\n");
    OSAL_logMsg(" --xBatch=n       number of video stream pkts in a batch:%d\n",
            VPMD_TEST_DEFAULT_XBATCH);
    OSAL_logMsg(" --xGap=n         number of ms between batch (taskDelay):%d\n",
            VPMD_TEST_DEFAULT_XGAP);
    OSAL_logMsg(" --xTotal=n       total number of msgs to test:%d\n",
            VPMD_TEST_DEFAULT_XTOTAL);
    OSAL_logMsg(" --yGap=n         number of ms between tx (taskDelay):%d\n",
            VPMD_TEST_DEFAULT_YGAP);
    OSAL_logMsg(" --echo           run in echo mode:%d\n",
            VPMD_TEST_DEFAULT_ECHO);
    OSAL_logMsg(" --help           print this help.\n");
    OSAL_logMsg("\n");
    return;
}

/*
 * ======== test vpmd echo server ========
 *
 */
#ifdef OSAL_VXWORKS
int VPMD_testMain(int argc, char *argv_ptr[])
#elif defined(OSAL_THREADX)
int VPMD_testMain(int argc, int argv)
#else
OSAL_ENTRY
#endif
{
    int     OptionIndex;
#if defined(OSAL_THREADX)    
    char  **argv_ptr;

    VPMD_muxDbgPrintf("vpmd received argv_ptr address=0x%08x\n", argv); 
    argv_ptr = (char **)argv;
#endif

    _VPMD_testObj_ptr->xBatch = VPMD_TEST_DEFAULT_XBATCH;
    _VPMD_testObj_ptr->xGap = VPMD_TEST_DEFAULT_XGAP;
    _VPMD_testObj_ptr->xTotal = VPMD_TEST_DEFAULT_XTOTAL;
    _VPMD_testObj_ptr->yGap = VPMD_TEST_DEFAULT_YGAP;
    _VPMD_testObj_ptr->echo = VPMD_TEST_DEFAULT_ECHO;

    OptionIndex = 0;
    if (argc > 1) {
        // parsing options
        while (OptionIndex < (argc-1)) {
            if (OSAL_strncmp(argv_ptr[OptionIndex+1], "--xBatch=", 9) == 0) {
                _VPMD_testObj_ptr->xBatch = OSAL_atoi(&argv_ptr[OptionIndex+1][9]);
            }
            if (OSAL_strncmp(argv_ptr[OptionIndex+1], "--xGap=", 7) == 0) {
                _VPMD_testObj_ptr->xGap = OSAL_atoi(&argv_ptr[OptionIndex+1][7]);
            }
            if (OSAL_strncmp(argv_ptr[OptionIndex+1], "--xTotal=", 9) == 0) {
                _VPMD_testObj_ptr->xTotal = OSAL_atoi(&argv_ptr[OptionIndex+1][9]);
            }
            if (OSAL_strncmp(argv_ptr[OptionIndex+1], "--yGap=", 7) == 0) {
                _VPMD_testObj_ptr->yGap = OSAL_atoi(&argv_ptr[OptionIndex+1][7]);
            }
            if (OSAL_strncmp(argv_ptr[OptionIndex+1], "--echo", 6) == 0) {
                _VPMD_testObj_ptr->echo = OSAL_TRUE;
            }
            if ((OSAL_strcmp(argv_ptr[OptionIndex+1], "--help") == 0) ||
              (OSAL_strcmp(argv_ptr[OptionIndex+1], "/?") == 0)) {
                _VPMD_testShowUsage();
                return 0;
            }
            OptionIndex++;
        }
    }
    OSAL_logMsg("Testing Setup :\n");
    OSAL_logMsg(" --xBatch=%d\n", _VPMD_testObj_ptr->xBatch);
    OSAL_logMsg(" --xGap=%d\n", _VPMD_testObj_ptr->xGap);
    OSAL_logMsg(" --xTotal=%d\n", _VPMD_testObj_ptr->xTotal);
    OSAL_logMsg(" --yGap=%d\n", _VPMD_testObj_ptr->yGap);
    OSAL_logMsg(" --echo=%d\n", _VPMD_testObj_ptr->echo);

#ifdef OSAL_THREADX
    if (NULL == VPMD_testEndSem) {
        VPMD_testEndSem = OSAL_semBinaryCreate(OSAL_SEMB_UNAVAILABLE);
    }
#endif

    if (OSAL_TRUE == _VPMD_testObj_ptr->echo) {
        VPMD_testEcho();
    } else {
        VPMD_testStress(
            _VPMD_testObj_ptr->xBatch,
            _VPMD_testObj_ptr->xGap,
            _VPMD_testObj_ptr->xTotal,
            _VPMD_testObj_ptr->yGap);
    }

    return (0);
}
#if !defined(OSAL_VXWORKS) && !defined(OSAL_THREADX)
OSAL_EXIT
#endif
