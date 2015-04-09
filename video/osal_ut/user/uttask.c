/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIA
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 11815 $ $Date: 2010-04-16 06:06:18 +0800 (Fri, 16 Apr 2010) $
 *
 *
 * Name:        Voice Operating System (OSAL) Unit Test
 *
 * File:        uttask.c
 *
 * Description: OSAL unit test for task-related features.
 *
 * Author:      bc
 */

#include "osal_ut.h"

static OSAL_Boolean _OSAL_test_task_completed;
static int _OSAL_test_prio_result[2] = {0};

#define OSALUT_PRIO_TEST_NUMS   (0X1fffffff)

#ifdef OSAL_PTHREADS
#define TASK_SELF_ID ((unsigned) pthread_self())
#else
#define TASK_SELF_ID 0
#endif

/*
 * ======== _OSALUT_taskTest() ========
 * We're going to be suspended/resumed which will defer our
 * expected delay so we wont check for delay sensitivity
 *
 * Returns:
 *   UT_PASS: Exit normally.
 */
static UT_Return _OSALUT_taskTest(
    uint32 delay_makeSafe)
{
    uint16      delay = delay_makeSafe >> 16;
    uint16      makeSafe = delay_makeSafe & 0xffff;
    OSAL_Status status;

    OSAL_logMsg("_OSALUT_taskTest (0x%08x) entered. delay=%d, makesafe=%d\n",
        TASK_SELF_ID, delay, makeSafe);

    if (delay) {
        /* delay is specified in HZ - convert to millisecs */
        OSAL_logMsg("_OSAL_testTask(0x%08x) delaying\n",
            TASK_SELF_ID);
        status = OSAL_taskDelay(delay*1000);
        if (status != OSAL_SUCCESS) {
            prError("Unexpected delay trunc\n");
        }
        OSAL_logMsg("_OSAL_testTask(0x%08x) resumed\n",
            TASK_SELF_ID);
    }

    _OSAL_test_task_completed = OSAL_TRUE;
    return (UT_PASS);
}

/*
 * ======== _OSALUT_taskTest2() ========
 * same as 1 except we're going to be suspended/resumed which will defer our
 * expected delay so we wont check for delay sensitivity
 *
 * Returns:
 *   UT_PASS: Exit normally.
 */
static UT_Return _OSALUT_taskTest2(
    uint32 delay_makeSafe)
{
    uint16  delay = delay_makeSafe >> 16;
    uint16  makeSafe = delay_makeSafe & 0xffff;

    OSAL_logMsg("_OSALUT_taskTest2() entered. delay = %d, makesafe = %d\n",
        delay, makeSafe);

    if (delay) {
        /* delay is specified in HZ - convert to millisecs */
        OSAL_logMsg("_OSAL_testTask2(0x%08x) delaying\n",
            TASK_SELF_ID);
        OSAL_taskDelay(delay*1000);
        OSAL_logMsg("_OSAL_testTask2(0x%08x) resumed\n",
            TASK_SELF_ID);
    }

    _OSAL_test_task_completed = OSAL_TRUE;
    return (UT_PASS);
}

 /*
 * ========= _OSALUT_taskTest3() ========
 * Function to set the priority test result
 *
 * Return:
 *  UT_PASS: Exit normally.
 *  UT_FAIL: Invalid arguments.
 */
static UT_Return _OSALUT_taskTest3(
    void)
{
    uint32 i = 0;
    uint32 j = 0;

    OSAL_logMsg("_OSALUT_taskTest3() entered. \n");
    for (i = 0 ; i < OSALUT_PRIO_TEST_NUMS ; i++) {
        j++;
    }
    OSAL_taskDelay(1); 
    j = 0;
    for (i = 0 ; i < OSALUT_PRIO_TEST_NUMS ; i++) {
        j++;
    } 

    for (i = 0 ; i < 2 ; i++) {
        if(_OSAL_test_prio_result[i] == 0) {
            _OSAL_test_prio_result[i] = 3;
            break;
        }
    }
    OSAL_logMsg("_OSALUT_taskTest3 finish\n");
    return (UT_PASS);
}

 /*
 * ========= _OSALUT_taskTest4() ========
 * Function to set the priority test result
 *
 * Return:
 *  UT_PASS: Exit normally.
 *  UT_FAIL: Invalid arguments.
 */
static UT_Return _OSALUT_taskTest4(
    void)
{
    uint32 i = 0;
    uint32 j = 0;

    OSAL_logMsg("_OSALUT_taskTest4() entered. \n");
    for (i = 0 ; i < OSALUT_PRIO_TEST_NUMS ; i++) {
        j++;
    }
    OSAL_taskDelay(1);
    j = 0;
    for (i = 0 ; i < OSALUT_PRIO_TEST_NUMS ; i++) {
        j++;
    }
    for (i = 0 ; i < 2 ; i++) {
        if(_OSAL_test_prio_result[i] == 0) {
            _OSAL_test_prio_result[i] = 4;
            break;
        }
    }
    OSAL_logMsg("_OSALUT_taskTest4 finish\n");

    return (UT_PASS);
}

 /*
 * ========= _OSALUT_taskPrioTest() ========
 * Function to test task priority
 *
 * Return:
 *  UT_PASS: Exit normally.
 *  UT_FAIL: Invalid arguments.
 */
static UT_Return _OSALUT_taskPrioTest(
    int priority1,
    int priority2)
{
    OSAL_TaskId     tId1;
    OSAL_TaskId     tId2;

    /* reset test result */
    OSAL_memSet(_OSAL_test_prio_result, 0, sizeof(_OSAL_test_prio_result));
    tId1 = OSAL_taskCreate("tVosTestTask3", 
            (OSAL_TaskPrio)priority1, 
            (uint32) 16384, 
            (OSAL_TaskPtr)_OSALUT_taskTest3, NULL);
    if ((OSAL_TaskId) NULL == tId1) {
        OSAL_logMsg("Unexpected OSAL_taskCreate() retval - expected non-NULL from "
                "IRQ, got NULL\n");
        return (UT_PASS);
    }   
    tId2 = OSAL_taskCreate("tVosTestTask4", 
            (OSAL_TaskPrio)priority2, 
            (uint32) 16384, 
            (OSAL_TaskPtr)_OSALUT_taskTest4, NULL);
    if ((OSAL_TaskId) NULL == tId2) {
        OSAL_logMsg("Unexpected OSAL_taskCreate() retval - expected non-NULL from "
                "IRQ, got NULL\n");
        return (UT_PASS);
    }
    OSAL_taskDelay(8000);

    return (UT_PASS);
}

/*
 * ======== do_test_task() ========
 * Gen unit test vectors for each OSAL function
 *
 * Return:
 *  UT_PASS: Exit normally.
 *  UT_FAIL: Invalid arguments.
 */
UT_Return do_test_task(
    void)
{
    char           *tName = "tVosTestTask";
    OSAL_Status     status;
    OSAL_TaskId     vtId;
    OSAL_Boolean    makesafe;
    OSAL_logMsg("Task Unit Test Starting...\n");
    /* Reset this before every test */
    osal_utErrorsFound = 0;

    /* task Creation Error Processing... */
    /* NULL name */
    OSAL_logMsg("Task test 1 \n");
    vtId = OSAL_taskCreate(NULL, OSAL_TASK_PRIO_NRT, (uint32) 16384,
            (OSAL_TaskPtr)_OSALUT_taskTest, NULL);
    if ((OSAL_TaskId) NULL != vtId) {
        prError1("Unexpected OSAL_taskCreate() retval - expected NULL, "
                "got 0x%08x\n", (unsigned) vtId);
    }

    /* NULL entryPt */
    OSAL_logMsg("Task test 2\n");
    vtId = OSAL_taskCreate(tName, OSAL_TASK_PRIO_NRT, (uint32) 8000, NULL,
            NULL);
    if ((OSAL_TaskId) NULL != vtId) {
        prError1("Unexpected OSAL_taskCreate() retval - expected NULL, "
                "got 0x%08x\n", (unsigned) vtId);
    }
    
#if !defined(OSAL_LKM) && !defined(OSAL_PTHREADS)
    /* Too much stack => TOO_BIG */
    vtId = OSAL_taskCreate("tVosTestTask", OSAL_TASK_PRIO_NRT,
            (uint32) 0x80000000, (OSAL_TaskPtr)_OSALUT_taskTest, (uint32) NULL);
    if ((OSAL_TaskId) NULL != vtId) {
        prError1("Unexpected OSAL_taskCreate() retval - expected NULL "
                "from IRQ, got 0x%08x\n", (unsigned) vtId);
    }
#endif

    /* task Deletion Error Processing... */
    /* Bad tId */
    OSAL_logMsg("Task test 3\n");
    status = OSAL_taskDelete((OSAL_TaskId) NULL);
    if (OSAL_FAIL != status) {
        prError1("Unexpected OSAL_taskDelete() retval - expected OSAL_FAIL, "
                "got 0x%08x\n", (unsigned) status);
    }

    /*
     * Create a good task at a very high priority , make it stick for a second
     * and make it delete safe...
     */
    _OSAL_test_task_completed = OSAL_FALSE;
    makesafe = OSAL_TRUE;
    OSAL_logMsg("Task test 4\n");
    vtId = OSAL_taskCreate("tVosTestTask", OSAL_TASK_PRIO_NRT, (uint32) 16384,
            (OSAL_TaskPtr)_OSALUT_taskTest, (void *) ((2 << 16) | makesafe));
    if ((OSAL_TaskId) NULL == vtId) {
        prError("Unexpected OSAL_taskCreate() retval - expected non-NULL "
                "from IRQ, got NULL\n");
    }

    /*
     * wait for 3 secs to make sure task completes and that the task
     * actually completed
     */
    OSAL_logMsg("Task test 5\n");
    OSAL_logMsg("do_test_task() -delaying 3secs\n");
    status = OSAL_taskDelay(3000); /* 3secs worth of millisecs */
    OSAL_logMsg("do_test_task() -resumed\n");
    if (status != OSAL_SUCCESS) {
        prError("Unexpected delay trunc\n");
    }
    if (OSAL_TRUE != _OSAL_test_task_completed) {
        prError("Expected test task to have completed, it has not.\n");
    }

    /*
     *Create a good task at a very high priority , make it delay for 10 secs
     * verify we can suspend it
     */
    OSAL_logMsg("Task test 6\n");
    _OSAL_test_task_completed = OSAL_FALSE;
    makesafe = OSAL_FALSE;
    vtId = OSAL_taskCreate("tTestTask2", OSAL_TASK_PRIO_NRT, (uint32) 16384,
            (OSAL_TaskPtr)_OSALUT_taskTest2, (void *) ((10000 << 16) |makesafe));
    if ((OSAL_TaskId) NULL == vtId) {
        prError("Unexpected OSAL_taskCreate() retval - expected non-NULL "
                "from IRQ, got NULL\n");
    }

    /* Delay to give thread a chance to get started */
    OSAL_taskDelay(1000);

    status = OSAL_taskDelete(vtId);
    if (OSAL_SUCCESS != status) {
        prError1("Unexpected OSAL_taskDelete() retval - expected OSAL_SUCCESS, "
                "got 0x%08x\n", (unsigned) status);
    }

    /*
     *Task test 7:
     * Setting task 3 with priority 1 and task 4 with priority 90.
     * Expect task 4 complete first because task 4 has higher priority.
     */
    OSAL_logMsg("Task test 7\n");
    if (UT_PASS == _OSALUT_taskPrioTest(OSAL_TASK_PRIO_DEC20, OSAL_TASK_PRIO_NRT)) {
        if (_OSAL_test_prio_result[0] != 3) {
            prError("_OSALUT_taskTest4 didn't complete first as expected. \n");
        }
    }
    
    /*
     *Task test 8:
     * Setting task 3 with priority 90 and task 4 with priority 1.
     * Expect task 3 complete first because task 3 has higher priority.
     */
    OSAL_logMsg("Task test 8\n");
    if (UT_PASS == _OSALUT_taskPrioTest(OSAL_TASK_PRIO_NRT, OSAL_TASK_PRIO_DEC20)) {
        if (_OSAL_test_prio_result[0] != 4) {
            prError("_OSALUT_taskTest3 didn't complete first as expected. \n");
        }
    } 

    /*
     *Task test 9:
     * Setting task 3 with priority 0 and task 4 with priority 90.
     * Expect task 4 complete first because task 4 has higher priority.
     */
    OSAL_logMsg("Task test 9\n");
    if (UT_PASS == _OSALUT_taskPrioTest(OSAL_TASK_PRIO_VTSPR, OSAL_TASK_PRIO_DEC20)) {
        if (_OSAL_test_prio_result[0] != 3) {
            prError("_OSALUT_taskTest4 didn't complete first as expected. \n");
        }
    } 

    /*
     *Task test 10:
     * Setting task 3 with priority 1 and task 4 with priority 0.
     * Expect task 3 complete first because task 3 has higher priority.
     */
    OSAL_logMsg("Task test 10\n");
    if (UT_PASS == _OSALUT_taskPrioTest(OSAL_TASK_PRIO_DEC20, OSAL_TASK_PRIO_VTSPR)) {
        if (_OSAL_test_prio_result[0] != 4) {
            prError("_OSALUT_taskTest4 didn't complete first as expected. \n");
        }
    }     

    if (0 == osal_utErrorsFound) {
        OSAL_logMsg("Task Unit Test Completed Successfully.\n");
        return (UT_PASS);
    }
    else {
        OSAL_logMsg("Task Unit Test Completed with %d errors.\n",
                osal_utErrorsFound);
        return (UT_FAIL);
    }
}

