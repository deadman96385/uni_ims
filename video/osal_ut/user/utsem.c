/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 8919 $ $Date: 2009-02-14 07:19:11 +0800 (Sat, 14 Feb 2009) $
 *
 * Name:        Voice Operating System (OSAL)
 *
 * File:        utsem.c
 *
 * Description: Semaphore Unit Test
 *
 * Author:      bc
 */

#include "osal_ut.h"

/*
 *Test task to pend on a mutex sem forever, which will be deleted
 * by another task. Verify that we re-awake with an EINTR retcode
 */
OSAL_Status        testTaskStatus = OSAL_SUCCESS;
OSAL_Boolean       testTaskComplete = OSAL_FALSE;

/*
 * ========= semTestTask1() ========
 * Function ptr of sem task to do sem test
 *
 * Return:
 *    None.
 */
static void semTestTask1(
    OSAL_SemId s)
{
    OSAL_TimeVal                currentTime;

    OSAL_timeGetTimeOfDay(&currentTime);
    OSAL_logMsg("semTestTask1(0x%08x) invoked at %ds,%dus\n",
        (unsigned) s, (int) currentTime.sec, (int) currentTime.usec);

    /* take the global sem with wait forever */
    testTaskStatus = OSAL_semAcquire(s ,OSAL_WAIT_FOREVER);

    OSAL_timeGetTimeOfDay(&currentTime);
    OSAL_logMsg("semTestTask1: semAcquire(0x%08x) returns at %ds,%dus\n",
        (unsigned) s, (int) currentTime.sec, (int) currentTime.usec);

    testTaskComplete = OSAL_TRUE;
}

/*
 * ========= do_test_sem() ========
 * Gen unit test vectors for each OSAL sem function
 *
 * Return:
 *  UT_PASS: Exit normally.
 *  UT_FAIL: fail.
 */
UT_Return do_test_sem(
    void)
{
    OSAL_SemId                  cId, mId;
    OSAL_Status                 status;
    int                         bogusVal, i;
    OSAL_TaskId                 ttId;
    OSAL_TimeVal                currentTime;


    OSAL_timeGetTimeOfDay(&currentTime);
    OSAL_logMsg("Semaphore Unit Test Starting at %ds,%dus...\n",
        (int) currentTime.sec, (int) currentTime.usec);


    /* Reset this before every test */
    osal_utErrorsFound = 0;



    /* CSem Creation Verification...  */
    OSAL_timeGetTimeOfDay(&currentTime);
    OSAL_logMsg("2: CSem Unit Test at %ds,%dus...\n", (int) currentTime.sec,
            (int) currentTime.usec);
    /* check invalid init state  */
    bogusVal = -42;
    cId = OSAL_semCountCreate(bogusVal);
    if (NULL != cId) {
        prError1("Unexpected OSAL_semCountCreate() retval - expected NULL, "
                "got 0x%08x\n", (unsigned) cId);
    }


    /* Create a counting sem with 2 counts */
    cId = OSAL_semCountCreate(2);
    if (NULL == cId) {
        prError("Unexpected OSAL_semCountCreate() retval - expected "
                "non-NULL\n");
    }
    /* CSem creation test complete... */


    /* MSem Creation Verification...  */
    OSAL_timeGetTimeOfDay(&currentTime);
    OSAL_logMsg("3: MSem Unit Test at %ds,%dus...\n", (int) currentTime.sec,
            (int) currentTime.usec);

    /* Create a mutex sem */
    mId = OSAL_semMutexCreate();
    if (NULL == mId) {
        prError("Unexpected OSAL_semMutexCreate() retval - expected "
                "non-NULL\n");
    }
    /* MSem creation test complete... */


    /* Sem Deletion Verification...  */
    {
        OSAL_SemId       s;
        OSAL_timeGetTimeOfDay(&currentTime);
        OSAL_logMsg("4: CSem Delete Unit Test at %ds,%dus...\n",
                (int) currentTime.sec, (int) currentTime.usec);
        s = OSAL_semCountCreate(5);
        if (NULL == s) {
            prError("Unexpected OSAL_semMutexCreate() retval - expected "
                    "non-NULL\n");
        }
        status = OSAL_semDelete(s);
        if (OSAL_SUCCESS != status) {
            prError1("Unexpected OSAL_semDelete() retval - success expected, "
                    "actual: %d\n", status);
        }
        OSAL_timeGetTimeOfDay(&currentTime);
        OSAL_logMsg("5: MSem Acquire Unit Test at %ds,%dus...\n",
                (int) currentTime.sec, (int) currentTime.usec);
        s = OSAL_semMutexCreate();
        if (NULL == s) {
            prError("Unexpected OSAL_semMutexCreate() retval - expected "
                    "non-NULL\n");
        }
        status = OSAL_semAcquire(s, OSAL_NO_WAIT);
        if (OSAL_SUCCESS != status) {
            prError("Unexpected OSAL_semAcquire() retval - expected \n");
        }
        else {
            /*
             *we own it
             * spawn test task to try to take s and wait for it's notification
             */
            ttId = OSAL_taskCreate("tTestTask", OSAL_TASK_PRIO_NRT,
                    (uint32) 8000, (OSAL_TaskPtr) semTestTask1, s);
            if ((OSAL_TaskId) NULL == ttId) {
                prError("Unexpected OSAL_taskCreate() retval - expected "
                        "non-NULL, got NULL\n");
            }

            OSAL_timeGetTimeOfDay(&currentTime);
            OSAL_logMsg("6: MSem Give Unit Test at %ds,%dus...\n",
                    (int) currentTime.sec, (int) currentTime.usec);
            status = OSAL_semGive(s);
            if (OSAL_SUCCESS != status) {
                prError1("Unexpected OSAL_semDelete() retval - "
                        "success expected, actual: %d\n", status);
            }

            if (OSAL_FAIL == testTaskStatus) {
                prError("testTask Unexpectedly FAILed\n");
            }

        }
    }
    /* Sem delete test complete... */



    /* Sem Acquire/Give Verification...  */
    OSAL_timeGetTimeOfDay(&currentTime);
    OSAL_logMsg("8: MSem Aquire/Give Test at %ds,%dus...\n",
            (int) currentTime.sec, (int) currentTime.usec);

    status = OSAL_semAcquire(NULL, OSAL_NO_WAIT);
    if (OSAL_FAIL != status) {
        prError1("Unexpected OSAL_semAcquire() retval - expected OSAL_FAIL, "
                "got 0x%08x\n", (unsigned) status);
    }

    status = OSAL_semAcquire(mId, OSAL_NO_WAIT);
    if (OSAL_SUCCESS != status) {
        prError1("Unexpected OSAL_semAcquire() retval - expected OSAL_SUCCESS, "
                "got 0x%08x\n", (unsigned) status);
    }

    /* Give mId and retry - we should succeed right away */
    status = OSAL_semGive(mId);

    OSAL_timeGetTimeOfDay(&currentTime);
    OSAL_logMsg("9: MSem Timeout Acquire Unit Test at %ds,%dus...\n",
            (int) currentTime.sec, (int) currentTime.usec);
    status = OSAL_semAcquire(mId, 1000);
    if (OSAL_SUCCESS != status) {
        prError1("Unexpected OSAL_semAcquire() retval - expected OSAL_SUCCESS, "
                "got %d\n", (unsigned) status);
    }

    OSAL_semGive(mId);

    OSAL_timeGetTimeOfDay(&currentTime);
    OSAL_logMsg("9-2: MSem Timeout Acquire Unit Test Finished at %ds,%dus...\n",
            (int) currentTime.sec, (int) currentTime.usec);
   /* Sem Acquire test complete... */


    /* Sem Acquire stress test */
    OSAL_timeGetTimeOfDay(&currentTime);
    OSAL_logMsg("11: MSem Acquire Stress Test at %ds,%dus...\n",
            (int) currentTime.sec, (int) currentTime.usec);

    for (i = 0; i < 1000000; i++) {
        status = OSAL_semAcquire(mId, OSAL_NO_WAIT);
        if (OSAL_SUCCESS != status) {
            prError1("Unexpected OSAL_semGetValue() retval - expected "
                    "OSAL_SUCCESS, got 0x%08x\n", (unsigned) status);
        }

        OSAL_semGive(mId);
    }

    OSAL_timeGetTimeOfDay(&currentTime);
    OSAL_logMsg("12: CSem Acquire Stress Test at %ds,%dus...\n",
            (int) currentTime.sec, (int) currentTime.usec);

    for (i = 0; i < 1000000; i++) {
        status = OSAL_semAcquire(cId, OSAL_NO_WAIT);
        if (OSAL_SUCCESS != status) {
            prError1("Unexpected OSAL_semGetValue() retval - expected "
                    "OSAL_SUCCESS, got 0x%08x\n", (unsigned) status);
        }

        status = OSAL_semAcquire(cId, OSAL_NO_WAIT);
        if (OSAL_SUCCESS != status) {
            prError1("Unexpected OSAL_semGetValue() retval - expected "
                    "OSAL_SUCCESS, got 0x%08x\n", (unsigned) status);
        }

        OSAL_semGive(cId);
        OSAL_semGive(cId);
    }

    /* cleanup */
    OSAL_timeGetTimeOfDay(&currentTime);
    OSAL_logMsg("13: Cleanup Unit Test at %ds,%dus...\n", (int) currentTime.sec,
            (int) currentTime.usec);
    OSAL_semDelete(cId);
    OSAL_semDelete(mId);
    /* Sem GetValue test complete... */

    if (0 == osal_utErrorsFound) {
        OSAL_logMsg("Semaphore Unit Test Completed Successfully.\n");
        return (UT_PASS);
    }
    else {
        OSAL_logMsg("Semaphore Unit Test Completed with %d errors.\n",
                osal_utErrorsFound);
        return (UT_FAIL);
    }
}
