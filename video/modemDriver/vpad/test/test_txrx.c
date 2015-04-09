/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004-2010 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Revision$ $Date$
 */
#include <osal.h>
#include <osal_net.h>
#include <vpr_comm.h>
#include <vpad_vpmd.h>

#define VPAD_TEST_TASK_NAME         "vpad_test_task"
#define VPAD_TEST_MSGQ_DEPTH        (2)
#define VPAD_TEST_TASK_STACK_BYTES  (4096)

#define VPAD_ECHO_TASK_NAME         "vpad_echo_task"
#define VPAD_ECHO_MSGQ_DEPTH        (2)
#define VPAD_ECHO_TASK_STACK_BYTES  (4096)

#define VPAD_TEST_DEFAULT_XBATCH    (8)
#define VPAD_TEST_DEFAULT_XGAP      (33)
#define VPAD_TEST_DEFAULT_XTOTAL    (120)
#define VPAD_TEST_DEFAULT_YGAP      (0)
#define VPAD_TEST_DEFAULT_ECHO      (OSAL_FALSE)

#define D2_VPORT_REVISION ""

/* Struct of VPAD_TestObj */
typedef struct {
    OSAL_TaskId     vpadTestTaskId;
    OSAL_TaskId     vpadEchoTaskId;
    vint            xBatch; /* e.g. 8 frames per batch */
    vint            xGap; /* e.g. 33ms for 30fps */
    vint            xTotal; /* total number of messages to stress test, 2400 for 10sec */
    vint            yGap; /* small gap between send */
    OSAL_Boolean    echo; /* echo mode */
    VPR_Comm        commVpad; /* For vpad testing use */
    struct {
        int vpadVideoStreamFd;
        int vpadVideoCmdEvtFd;
        int vpadVoiceStreamFd;
        int vpadSipFd;
        int vpadIsipFd;
        int vpadCsmEvtFd;
    } fd;
} VPAD_TestObj;


static VPAD_TestObj _VPAD_testObj;
static VPAD_TestObj *_VPAD_testObj_ptr = &_VPAD_testObj;


/*
 * ======== _VPAD_testEchoTask() ========
 *
 * This task is to wait on all vpad's fds, receive data from VPMD
 * and echo the data back to VPMD.
 *
 * Returns:
 * None
 */
static OSAL_TaskReturn _VPAD_testEchoTask(
    OSAL_TaskArg arg_ptr)
{
    OSAL_SelectSet   readSet;
    OSAL_Boolean     boolean;
    VPAD_TestObj    *vpadTestObj_ptr;
    VPR_Comm        *comm_ptr;

    vpadTestObj_ptr = (VPAD_TestObj*) arg_ptr;
    comm_ptr = &vpadTestObj_ptr->commVpad;

__VPAD_TASK_LOOP:
    /*
     * Group fd set.
     */
    OSAL_selectSetInit(&readSet);

    /* Add vpad video stream fd for select */
    OSAL_selectAddId(&vpadTestObj_ptr->fd.vpadVideoStreamFd, &readSet);
    /* Add vpad video cmd evt fd for select */
    OSAL_selectAddId(&vpadTestObj_ptr->fd.vpadVideoCmdEvtFd, &readSet);
    /* Add vpad voice stream fd for select */
    OSAL_selectAddId(&vpadTestObj_ptr->fd.vpadVoiceStreamFd, &readSet);
    /* Add vpad sip fd for select */
    OSAL_selectAddId(&vpadTestObj_ptr->fd.vpadSipFd, &readSet);
    /* Add vpad isip fd for select */
    OSAL_selectAddId(&vpadTestObj_ptr->fd.vpadIsipFd, &readSet);
    /* Add vpad csm event fd for select */
    OSAL_selectAddId(&vpadTestObj_ptr->fd.vpadCsmEvtFd, &readSet);

    /* Wait forever */
    if (OSAL_FAIL == OSAL_select(&readSet, NULL, NULL, NULL)) {
        OSAL_logMsg("%s:%d Failed to seletec fd for echo test.\n",
             __FUNCTION__, __LINE__);
        return (OSAL_FAIL);
    }

    /* See if it's command from video stream */
    OSAL_selectIsIdSet(&vpadTestObj_ptr->fd.vpadVideoStreamFd, &readSet, &boolean);
    if (OSAL_TRUE == boolean) {
        /* Read VPR command from vpad */
        if (OSAL_SUCCESS == VPAD_readVideoStream(comm_ptr, sizeof(VPR_Comm),
                OSAL_WAIT_FOREVER)) {
            /* Got VPR_Comm, echo it */
            VPAD_writeVideoStream(comm_ptr, sizeof(VPR_Comm));
        }
    }
    /* See if it's command from video stream */
    OSAL_selectIsIdSet(&vpadTestObj_ptr->fd.vpadVideoCmdEvtFd, &readSet, &boolean);
    if (OSAL_TRUE == boolean) {
        /* Read VPR command from vpad */
        if (OSAL_SUCCESS == VPAD_readVideoCmdEvt(comm_ptr, sizeof(VPR_Comm),
                OSAL_WAIT_FOREVER)) {
            /* Got VPR_Comm, echo it */
            VPAD_writeVideoCmdEvt(comm_ptr, sizeof(VPR_Comm));
        }
    }
    /* See if it's command from video stream */
    OSAL_selectIsIdSet(&vpadTestObj_ptr->fd.vpadVoiceStreamFd, &readSet, &boolean);
    if (OSAL_TRUE == boolean) {
        /* Read VPR command from vpad */
        if (OSAL_SUCCESS == VPAD_readVoiceStream(comm_ptr, sizeof(VPR_Comm),
                OSAL_WAIT_FOREVER)) {
            /* Got VPR_Comm, echo it */
            VPAD_writeVoiceStream(comm_ptr, sizeof(VPR_Comm));
        }
    }
    /* See if it's sip packet from sr */
    OSAL_selectIsIdSet(&vpadTestObj_ptr->fd.vpadSipFd, &readSet, &boolean);
    if (OSAL_TRUE == boolean) {
        /* Read VPR command from vpad */
        if (OSAL_SUCCESS == VPAD_readSip(comm_ptr, sizeof(VPR_Comm),
                OSAL_WAIT_FOREVER)) {
            /* Got VPR_Comm, echo it */
            VPAD_writeSip(comm_ptr, sizeof(VPR_Comm));
        }
    }
    /* See if it's isip message from sr */
    OSAL_selectIsIdSet(&vpadTestObj_ptr->fd.vpadIsipFd, &readSet, &boolean);
    if (OSAL_TRUE == boolean) {
        /* Read VPR command from vpad */
        if (OSAL_SUCCESS == VPAD_readIsip(comm_ptr, sizeof(VPR_Comm),
                OSAL_WAIT_FOREVER)) {
            /* Got VPR_Comm, echo it */
            VPAD_writeIsip(comm_ptr, sizeof(VPR_Comm));
        }
    }
    goto __VPAD_TASK_LOOP;

    return (0);
}


/*
 * ======== _VPAD_testEchoInit() ========
 *
 * Private function to initialize vpad echo test.
 *
 * Returns:
 * OSAL_SUCCESS: echo initialized successfully.
 * OSAL_FAIL: Error in echo initialization.
 */
OSAL_Status _VPAD_testEchoInit(
    VPAD_TestObj *vpadTestObj_ptr)
{
    OSAL_taskDelay(1000);
    while (OSAL_FALSE == VPAD_isReady()) {
        OSAL_logMsg("%s:%d wait 1 sec for vpad ready\n",
            __FUNCTION__, __LINE__);
        OSAL_taskDelay(1000);
    }
    
    /* Get voice stream device fd from VPAD */
    if (0 == (vpadTestObj_ptr->fd.vpadVoiceStreamFd =
            VPAD_getVoiceStreamReadFd())) {
        OSAL_logMsg("%s:%d Fail to get voice stream read fd from vpad.\n",
             __FUNCTION__, __LINE__);
        return (OSAL_FAIL);
    }

    /* Get video event device fd from VPAD */
    if (0 == (vpadTestObj_ptr->fd.vpadVideoCmdEvtFd =
            VPAD_getVideoCmdEvtReadFd())) {
        OSAL_logMsg("%s:%d Fail to get video event read fd from vpad.\n",
             __FUNCTION__, __LINE__);
        return (OSAL_FAIL);
    }

    /* Get video stream device fd from VPAD */
    if (0 == (vpadTestObj_ptr->fd.vpadVideoStreamFd =
            VPAD_getVideoStreamReadFd())) {
        OSAL_logMsg("%s:%d Fail to get video stream read fd from vpad.\n",
             __FUNCTION__, __LINE__);
        return (OSAL_FAIL);
    }

    /* Get sip device fd from VPAD */
    if (0 == (vpadTestObj_ptr->fd.vpadSipFd =
            VPAD_getSipReadFd())) {
        OSAL_logMsg("%s:%d Fail to get sip read fd from vpad.\n",
             __FUNCTION__, __LINE__);
        return (OSAL_FAIL);
    }

    /* Get isip device fd from VPAD */
    if (0 == (vpadTestObj_ptr->fd.vpadIsipFd =
            VPAD_getIsipReadFd())) {
        OSAL_logMsg("%s:%d Fail to get isip read fd from vpad.\n",
             __FUNCTION__, __LINE__);
        return (OSAL_FAIL);
    }

    /* Create VPR vpmd task */
    if (0 == (vpadTestObj_ptr->vpadEchoTaskId = OSAL_taskCreate(
            VPAD_ECHO_TASK_NAME,
            OSAL_TASK_PRIO_DEC20,
            VPAD_ECHO_TASK_STACK_BYTES,
            _VPAD_testEchoTask,
            (void *)vpadTestObj_ptr))) {
        OSAL_logMsg("%s:%d Failed creating %s daemon\n",
                __FUNCTION__, __LINE__, VPAD_TEST_TASK_NAME);
        return (OSAL_FAIL);
    }

    return (OSAL_SUCCESS);
}


/*
 * ======== _VPAD_testStructDump() ========
 *
 * This task is to write on all VPAD/VPMD major struct size
 *
 * Returns:
 *  OSAL_SUCCESS: success
 *  OSAL_FAIL: fail
 */
static OSAL_Status _VPAD_testStructDump(
    VPAD_TestObj    *vpadTestObj_ptr)
{
    OSAL_logMsg("VPR_Comm size=%d\n",sizeof(VPR_Comm));
    OSAL_logMsg("_VTSP_CmdMsg size=%d\n",sizeof(_VTSP_CmdMsg));
    OSAL_logMsg("_VTSP_RtcpCmdMsg size=%d\n",sizeof(_VTSP_RtcpCmdMsg));
    OSAL_logMsg("_VTSP_RtcpEventMsg size=%d\n",sizeof(_VTSP_RtcpEventMsg));
    OSAL_logMsg("VTSP_EventMsg size=%d\n",sizeof(VTSP_EventMsg));
    OSAL_logMsg("ISIP_Message size=%d\n",sizeof(ISIP_Message));
    OSAL_logMsg("VPR_Net size=%d\n",sizeof(VPR_Net));
    OSAL_logMsg("CSM_InputEvent size=%d\n",sizeof(CSM_InputEvent));
    OSAL_logMsg("VPR_NetworkMode size=%d\n",sizeof(VPR_NetworkMode));

    OSAL_logMsg("ISIP_System size=%d\n",sizeof(ISIP_System));
    OSAL_logMsg("ISIP_Service size=%d\n",sizeof(ISIP_Service));
    OSAL_logMsg("ISIP_Call size=%d\n",sizeof(ISIP_Call));
    OSAL_logMsg("ISIP_Text size=%d\n",sizeof(ISIP_Text));
    OSAL_logMsg("ISIP_Media size=%d\n",sizeof(ISIP_Media));
    OSAL_logMsg("ISIP_TelEvent size=%d\n",sizeof(ISIP_TelEvent));
    OSAL_logMsg("ISIP_Presence size=%d\n",sizeof(ISIP_Presence));
    OSAL_logMsg("ISIP_Chat size=%d\n",sizeof(ISIP_Chat));
    OSAL_logMsg("ISIP_File size=%d\n",sizeof(ISIP_File));
    OSAL_logMsg("ISIP_Diagnostic size=%d\n",sizeof(ISIP_Diagnostic));
    OSAL_logMsg("ISIP_Ussd size=%d\n",sizeof(ISIP_Ussd));

    return (OSAL_SUCCESS);
}

/*
 * ======== _VPAD_testTxRxDispatch() ========
 *
 * This task is to write different vpad message and check echo back
 *
 * Returns:
 *  OSAL_SUCCESS: success
 *  OSAL_FAIL: fail
 */
static OSAL_Status _VPAD_testTxRxDispatch(
    VPAD_TestObj    *vpadTestObj_ptr)
{
    VPR_Comm    *comm_ptr;
    OSAL_Status result;
    vint        dataSeed = 0x55;

    OSAL_logMsg("%s:%d starting VPAD tx/rx testing...\n",
             __FUNCTION__, __LINE__);

    comm_ptr = &vpadTestObj_ptr->commVpad;

    /* checking tx/rx for video stream */
    OSAL_memSet(comm_ptr, dataSeed++, sizeof(VPR_Comm));
    result = VPAD_writeVideoStream(comm_ptr, sizeof(VPR_Comm));
    OSAL_logMsg("%s:%d VPAD_writeVideoStream result=%d\n",
            __FUNCTION__, __LINE__, result);

    /* Read back from vpad */
    if (OSAL_SUCCESS == VPAD_readVideoStream(comm_ptr, sizeof(VPR_Comm),
            OSAL_WAIT_FOREVER)) {
        /* Got VPR_Comm echo */
        OSAL_logMsg("%s:%d VPAD tx/rx ok VideoStream.\n",
                __FUNCTION__, __LINE__);
    }
    OSAL_taskDelay(100);

    /* checking tx/rx for video cmd stream */
    OSAL_memSet(comm_ptr, dataSeed++, sizeof(VPR_Comm));
    result = VPAD_writeVideoCmdEvt(comm_ptr, sizeof(VPR_Comm));
    OSAL_logMsg("%s:%d VPAD_writeVideoCmdEvt result=%d\n",
            __FUNCTION__, __LINE__, result);
    /* Read back from vpad */
    if (OSAL_SUCCESS == VPAD_readVideoCmdEvt(comm_ptr, sizeof(VPR_Comm),
            OSAL_WAIT_FOREVER)) {
        /* Got VPR_Comm echo */
        OSAL_logMsg("%s:%d VPAD tx/rx ok VideoCmdEvt.\n",
                __FUNCTION__, __LINE__);
    }
    OSAL_taskDelay(100);

    /* checking tx/rx for voice stream */
    OSAL_memSet(comm_ptr, dataSeed++, sizeof(VPR_Comm));
    result = VPAD_writeVoiceStream(comm_ptr, sizeof(VPR_Comm));
    OSAL_logMsg("%s:%d VPAD_writeVoiceStream result=%d\n",
            __FUNCTION__, __LINE__, result);
    /* Read back from vpad */
    if (OSAL_SUCCESS == VPAD_readVoiceStream(comm_ptr, sizeof(VPR_Comm),
            OSAL_WAIT_FOREVER)) {
        /* Got VPR_Comm echo */
        OSAL_logMsg("%s:%d VPAD tx/rx ok VoiceStream.\n",
                __FUNCTION__, __LINE__);
    }
    OSAL_taskDelay(100);

    /* checking tx/rx for sip packet */
    OSAL_memSet(comm_ptr, dataSeed++, sizeof(VPR_Comm));
    result = VPAD_writeSip(comm_ptr, sizeof(VPR_Comm));
    OSAL_logMsg("%s:%d VPAD_writeSip result=%d\n",
            __FUNCTION__, __LINE__, result);
    /* Read back from vpad */
    if (OSAL_SUCCESS == VPAD_readSip(comm_ptr, sizeof(VPR_Comm),
            OSAL_WAIT_FOREVER)) {
        /* Got VPR_Comm echo */
        OSAL_logMsg("%s:%d VPAD tx/rx ok Sip.\n",
                __FUNCTION__, __LINE__);
    }
    OSAL_taskDelay(100);

    /* checking tx/rx for isip message */
    OSAL_memSet(comm_ptr, dataSeed++, sizeof(VPR_Comm));
    result = VPAD_writeIsip(comm_ptr, sizeof(VPR_Comm));
    OSAL_logMsg("%s:%d VPAD_writeIsip result=%d\n",
            __FUNCTION__, __LINE__, result);
    /* Read back from vpad */
    if (OSAL_SUCCESS == VPAD_readIsip(comm_ptr, sizeof(VPR_Comm),
            OSAL_WAIT_FOREVER)) {
        /* Got VPR_Comm echo */
        OSAL_logMsg("%s:%d VPAD tx/rx ok Isip.\n",
                __FUNCTION__, __LINE__);
    }
    OSAL_taskDelay(100);

    /* no need checking tx for csm event, as we don't have echo back csm event */

    return (OSAL_SUCCESS);
}

/*
 * ======== _VPAD_testTxRxStress() ========
 *
 * This task is to write stressing stream to vpmd
 *
 * Returns:
 *  OSAL_SUCCESS: success
 *  OSAL_FAIL: fail
 */
static OSAL_Status _VPAD_testTxRxStress(
    VPAD_TestObj    *vpadTestObj_ptr)
{
    VPR_Comm    *comm_ptr;
    OSAL_Status result;
    vint        dataSeed = 0x55;
    vint        count, txOkCount, rxOkCount, batch;
    OSAL_TimeVal beginTime, endTime, durationTime;

    OSAL_logMsg("%s:%d stressing VPAD video stream tx/rx testing...\n",
         __FUNCTION__, __LINE__);

    comm_ptr = &vpadTestObj_ptr->commVpad;
    count = txOkCount = rxOkCount = 0;
    OSAL_timeGetTimeOfDay(&beginTime);

    while (count < vpadTestObj_ptr->xTotal) {
        for (batch=0; batch<vpadTestObj_ptr->xBatch; batch++) {
            /* checking tx/rx for video stream */
            OSAL_memSet(comm_ptr, dataSeed++, sizeof(VPR_Comm));
            result = VPAD_writeVideoStream(comm_ptr, sizeof(VPR_Comm));
            if (OSAL_SUCCESS == result) {
                txOkCount += 1;
                OSAL_logMsg("%s:%d txOkCount=%d\n", __FUNCTION__, __LINE__, txOkCount);
            }
            else  {
                OSAL_logMsg("%s:%d fail VPAD_writeVideoStream\n", __FUNCTION__, __LINE__);
            }
            count += 1;
            if (0 != vpadTestObj_ptr->yGap) {
                OSAL_taskDelay(vpadTestObj_ptr->yGap);
            }
        }

        for (batch=0; batch<vpadTestObj_ptr->xBatch; batch++) {
            /* Read back from vpad */
            if (OSAL_SUCCESS == VPAD_readVideoStream(comm_ptr, sizeof(VPR_Comm),
                    OSAL_WAIT_FOREVER)) {
                rxOkCount += 1;
                OSAL_logMsg("%s:%d rxOkCount=%d\n", __FUNCTION__, __LINE__, rxOkCount);
            }
            else  {
                OSAL_logMsg("%s:%d fail VPAD_readVideoStream\n", __FUNCTION__, __LINE__);
            }
        }
        if (0 != vpadTestObj_ptr->xGap) {
            OSAL_taskDelay(vpadTestObj_ptr->xGap);
        }
    }

    OSAL_timeGetTimeOfDay(&endTime);
    if (endTime.usec >= beginTime.usec) {
        durationTime.usec = endTime.usec - beginTime.usec;
        durationTime.sec = endTime.sec - beginTime.sec;
    } else {
        durationTime.usec = 1000*1000 + endTime.usec - beginTime.usec;
        durationTime.sec = endTime.sec - beginTime.sec - 1;
    }

    OSAL_logMsg("Testing Setup :\n");
    OSAL_logMsg(" --xBatch=%d\n", vpadTestObj_ptr->xBatch);
    OSAL_logMsg(" --xGap=%d\n", vpadTestObj_ptr->xGap);
    OSAL_logMsg(" --xTotal=%d\n", vpadTestObj_ptr->xTotal);
    OSAL_logMsg(" --yGap=%d\n", vpadTestObj_ptr->yGap);

    OSAL_logMsg("Stress Testing Results :\n");
    OSAL_logMsg(" total=%d\n", count);
    OSAL_logMsg(" txOkCount=%d\n", txOkCount);
    OSAL_logMsg(" rxOkCount=%d\n", rxOkCount);
    OSAL_logMsg(" duration sec=%d msec=%d\n", durationTime.sec, durationTime.usec/1000);

    /* 
     * for RTOS, this special csm event would trigger termination of vpmd echo task
     * for linux style os, echo task will just sunk the event message
     */
    OSAL_memSet(comm_ptr, dataSeed++, sizeof(VPR_Comm));
    result = VPAD_writeCsmEvt(comm_ptr, sizeof(VPR_Comm));
    OSAL_logMsg("%s:%d terminating VPAD_writeCsmEvt result=%d\n",
            __FUNCTION__, __LINE__, result);

    return (OSAL_SUCCESS);
}

/*
 * ======== _VPAD_testTxRxTask() ========
 *
 * This task is to write on all VPAD's fds,
 * and receive the echoed data.
 *
 * Returns:
 * OSAL_TaskReturn
 */
static OSAL_TaskReturn _VPAD_testTxRxTask(
    OSAL_TaskArg arg)
{
    VPAD_TestObj *vpadTestObj_ptr = (VPAD_TestObj*)arg;

    OSAL_logMsg("%s:%d check for vpad ready\n",
            __FUNCTION__, __LINE__);
    while (OSAL_FALSE == VPAD_isReady()) {
        OSAL_logMsg("%s:%d wait 1 sec for vpad ready\n",
            __FUNCTION__, __LINE__);
        OSAL_taskDelay(1000);
    }

    /* run each test */
    _VPAD_testStructDump(vpadTestObj_ptr);
    _VPAD_testTxRxDispatch(vpadTestObj_ptr);
    _VPAD_testTxRxStress(vpadTestObj_ptr);

    OSAL_logMsg("%s:%d all tests are done, press control-c to exit testing\n",
            __FUNCTION__, __LINE__);
            
    return (0);
}

static void _VPAD_testShowUsage(void)
{
    OSAL_logMsg("Testing VPAD/VPMD comm\n");
    OSAL_logMsg("Usage   : vpad_test [Option]\n");
    OSAL_logMsg("Options :\n");
    OSAL_logMsg(" --xBatch=n       number of video stream pkts in a batch:%d\n",
            VPAD_TEST_DEFAULT_XBATCH);
    OSAL_logMsg(" --xGap=n         number of ms between batch (taskDelay):%d\n",
            VPAD_TEST_DEFAULT_XGAP);
    OSAL_logMsg(" --xTotal=n       total number of msgs to test:%d\n",
            VPAD_TEST_DEFAULT_XTOTAL);
    OSAL_logMsg(" --yGap=n         number of ms between tx (taskDelay):%d\n",
            VPAD_TEST_DEFAULT_YGAP);
    OSAL_logMsg(" --echo           run in echo mode:%d\n",
            VPAD_TEST_DEFAULT_ECHO);
    OSAL_logMsg(" --help           print this help.\n");
    OSAL_logMsg("\n");
    return;
}

/*
 * ======== main() ========
 * VPAD test Entry point.
 * Start with OSAL conventions.
 *
 * Returns:
 *  -1 : Failed
 *   0 : OK
 */
OSAL_ENTRY
{
    int     OptionIndex;

    if (OSAL_SUCCESS != OSAL_condApplicationExitRegister()) {
        return (-1);
    }

    _VPAD_testObj_ptr->xBatch = VPAD_TEST_DEFAULT_XBATCH;
    _VPAD_testObj_ptr->xGap = VPAD_TEST_DEFAULT_XGAP;
    _VPAD_testObj_ptr->xTotal = VPAD_TEST_DEFAULT_XTOTAL;
    _VPAD_testObj_ptr->yGap = VPAD_TEST_DEFAULT_YGAP;
    _VPAD_testObj_ptr->echo = VPAD_TEST_DEFAULT_ECHO;

    OptionIndex = 0;
    if (argc > 1) {
        // parsing options
        while (OptionIndex < (argc-1)) {
            if (OSAL_strncmp(argv_ptr[OptionIndex+1], "--xBatch=", 9) == 0) {
                _VPAD_testObj_ptr->xBatch = OSAL_atoi(&argv_ptr[OptionIndex+1][9]);
            }
            if (OSAL_strncmp(argv_ptr[OptionIndex+1], "--xGap=", 7) == 0) {
                _VPAD_testObj_ptr->xGap = OSAL_atoi(&argv_ptr[OptionIndex+1][7]);
            }
            if (OSAL_strncmp(argv_ptr[OptionIndex+1], "--xTotal=", 9) == 0) {
                _VPAD_testObj_ptr->xTotal = OSAL_atoi(&argv_ptr[OptionIndex+1][9]);
            }
            if (OSAL_strncmp(argv_ptr[OptionIndex+1], "--yGap=", 7) == 0) {
                _VPAD_testObj_ptr->yGap = OSAL_atoi(&argv_ptr[OptionIndex+1][7]);
            }
            if (OSAL_strncmp(argv_ptr[OptionIndex+1], "--echo", 6) == 0) {
                _VPAD_testObj_ptr->echo = OSAL_TRUE;
            }
            if ((OSAL_strcmp(argv_ptr[OptionIndex+1], "--help") == 0) ||
              (OSAL_strcmp(argv_ptr[OptionIndex+1], "/?") == 0)) {
                _VPAD_testShowUsage();
                return 0;
            }
            OptionIndex++;
        }
    }
    OSAL_logMsg("Testing Setup :\n");
    OSAL_logMsg(" --xBatch=%d\n", _VPAD_testObj_ptr->xBatch);
    OSAL_logMsg(" --xGap=%d\n", _VPAD_testObj_ptr->xGap);
    OSAL_logMsg(" --xTotal=%d\n", _VPAD_testObj_ptr->xTotal);
    OSAL_logMsg(" --yGap=%d\n", _VPAD_testObj_ptr->yGap);
    OSAL_logMsg(" --echo=%d\n", _VPAD_testObj_ptr->echo);


    VPAD_init();

    if (OSAL_TRUE == _VPAD_testObj_ptr->echo) {
        /*
         * Initialize and run echo server.
         */
        if (OSAL_SUCCESS != _VPAD_testEchoInit(_VPAD_testObj_ptr)) {
            OSAL_condApplicationExitUnregister();
            OSAL_logMsg("%s:%d Failed creating vpad echo task\n",
                    __FUNCTION__, __LINE__);
            return (OSAL_FAIL);
        }
    } else {
        /* Create VPAD testing task */
        if (0 == (_VPAD_testObj_ptr->vpadTestTaskId = OSAL_taskCreate(
                VPAD_TEST_TASK_NAME,
                OSAL_TASK_PRIO_NRT,
                VPAD_TEST_TASK_STACK_BYTES,
                _VPAD_testTxRxTask,
                (void *)_VPAD_testObj_ptr))) {
            OSAL_logMsg("%s:%d Failed creating %s vpad testing task\n",
                    __FUNCTION__, __LINE__, VPAD_TEST_TASK_NAME);
            return (OSAL_FAIL);
        }
    }

    OSAL_condApplicationExitWaitForCondition();
    OSAL_condApplicationExitUnregister();

    if (OSAL_TRUE == _VPAD_testObj_ptr->echo) {
        OSAL_taskDelete(_VPAD_testObj_ptr->vpadEchoTaskId);
    } else {
        if (0 != _VPAD_testObj_ptr->vpadTestTaskId) {
            OSAL_taskDelete(_VPAD_testObj_ptr->vpadTestTaskId);
        }
    }

    VPAD_destroy();

    OSAL_logMsg("vpad test finished.\n");

    return (0);
}
OSAL_EXIT
