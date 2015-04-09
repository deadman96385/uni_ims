/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 11815 $ $Date: 2010-04-16 06:06:18 +0800 (Fri, 16 Apr 2010) $
 *
 *
 * Description: OSAL unit test for timer-related features.
 *
 */

#include "osal_ut.h"

static OSAL_Boolean       ut_tmr1_completed;
static int                ut_tmr1_invocations;
static OSAL_TmrId         vtId1;

 /*
 * ========= ut_tmr1() ========
 * A generic timer task that is kicked off during testing.
 *
 * Params:
 *  OSAL_Boolean - OSAL_TRUE if this is a 1-shot. OSAL_FALSE if
 *              periodic (in which case set the ut_tmr1_completed flag as true
 *              at IRQ context after the 5th invocation
 * Return:
 *  None.
 */
static void ut_tmr1(
    OSAL_Boolean oneShot)
{
    ut_tmr1_invocations++;
    if (oneShot == OSAL_TRUE) {
        ut_tmr1_completed = OSAL_TRUE;
    }
    else /* oneShot == FALSE */ if (ut_tmr1_invocations == 5) {
        ut_tmr1_completed = OSAL_TRUE;
    }
}

 /*
 * ========= do_test_timer() ========
 * Gen unit test vectors for each OSAL timer function.
 *
 * Return:
 *  UT_PASS: Exit normally.
 *  UT_FAIL: Invalid arguments.
 */
UT_Return do_test_timer(
    void)
{
    OSAL_Status           status;

    OSAL_logMsg("Timer Unit Test Starting...\n");


    /* Reset this before every test */
    osal_utErrorsFound = 0;



    OSAL_logMsg("Timer test 1\n");

    /* Tmr Self-Init Verification... */

    /* On entry, the OSAL tmr package may have been inited if all previous
     * tests were run up to this point. Ensure by creating and deleting a
     * timer
     */
    vtId1 = OSAL_tmrCreate();
    if (NULL == vtId1) {
        prError("OSAL_tmrCreate() didn't succeed as expected.\n");
        osal_utErrorsFound++;
        goto _OSAL_UT_TMR_TEST_2;
    }

    status = OSAL_tmrDelete(vtId1);
    if (OSAL_SUCCESS != status) {
        prError("OSAL_tmrDelete() didn't succeed as expected.\n");
        osal_utErrorsFound++;
        goto _OSAL_UT_TMR_TEST_2;
    }

    /* good to go let's test basic timers... */

    /* Tmr Start Verification...  */
_OSAL_UT_TMR_TEST_2:
    OSAL_logMsg("Timer test 2\n");
    /* At this point, we have no timers so we need to allocate new ones... */
    /* create a good timer... */
    vtId1 = OSAL_tmrCreate();
    if (NULL == vtId1) {
        prError("OSAL_tmrCreate() didn't succeed as expected.\n");
        osal_utErrorsFound++;
        goto _OSAL_UT_TMR_TEST_3;
    }


    /* Test Timer Start with invalid msTimeout */
    status = OSAL_tmrStart(vtId1, (OSAL_TmrPtr) ut_tmr1, (void *) OSAL_TRUE,
            OSAL_NO_WAIT);
    if (OSAL_FAIL != status) {
        prError("OSAL_tmrStart() didn't OSAL_FAIL as expected.\n");
        osal_utErrorsFound++;
        goto _OSAL_UT_TMR_TEST_3;
    }

    status = OSAL_tmrStart(vtId1, (OSAL_TmrPtr)ut_tmr1, (void *) OSAL_TRUE,
            OSAL_WAIT_FOREVER);
    if (OSAL_FAIL != status) {
        prError("OSAL_tmrStart() didn't OSAL_FAIL as expected.\n");
        osal_utErrorsFound++;
        goto _OSAL_UT_TMR_TEST_3;
    }


    /* Test Timer Start at IRQ level... we'll do this generically via a
     * support routine that runs at IRQ level...
     * this should work fine...
     */
    ut_tmr1_completed = OSAL_FALSE;
    ut_tmr1_invocations = 0;

_OSAL_UT_TMR_TEST_3:
    OSAL_logMsg("Timer test 3\n");

    status = OSAL_tmrStart(vtId1, (OSAL_TmrPtr) ut_tmr1, (void *) OSAL_TRUE,
            (uint32) 3000 /*3 secs*/);
    if (OSAL_SUCCESS != status) {
        prError1("Unexpected OSAL_tmrStart() retval - expected success from "
                "IRQ, got 0x%08x\n", (unsigned) status);
        osal_utErrorsFound++;
        goto _OSAL_UT_TMR_TEST_4;
    }

    status = OSAL_tmrStop(vtId1);
    if (OSAL_SUCCESS != status) {
        prError1("Unexpected OSAL_tmrStop() retval - expected success from "
                "IRQ, got 0x%08x\n", (unsigned) status);
        osal_utErrorsFound++;
        goto _OSAL_UT_TMR_TEST_4;
    }

_OSAL_UT_TMR_TEST_4:
    OSAL_taskDelay(4000); /* delay 4 seconds */
    OSAL_logMsg("Timer test 4\n");
    /* verify if timer fired erroneously */
    if (OSAL_TRUE == ut_tmr1_completed) {
        prError("OSAL_tmrStop() didn't succeed as expected - timer actually "
                "fired.\n");
        osal_utErrorsFound++;
        goto _OSAL_UT_TMR_TEST_5;
    }
    if (0 != ut_tmr1_invocations) {
        prError1("OSAL_tmrStop() didn't succeed as expected - timer actually "
                "fired %d times.\n", ut_tmr1_invocations);
        osal_utErrorsFound++;
        goto _OSAL_UT_TMR_TEST_5;
    }


    /* Start a timer - expect good start here */

    ut_tmr1_completed = OSAL_FALSE;
    ut_tmr1_invocations = 0;

    status = OSAL_tmrStart(vtId1, (OSAL_TmrPtr)ut_tmr1, (void *) OSAL_TRUE,
            10);

    /* delay for 1 second then verify our timer was run only once */
    OSAL_taskDelay(1000);
    if (OSAL_TRUE != ut_tmr1_completed) {
        prError("Unexpectedly, the timer routine ut_tmr1() did not "
                "complete.\n");
        osal_utErrorsFound++;
        goto _OSAL_UT_TMR_TEST_5;
    }
    if (1 != ut_tmr1_invocations) {
        prError("Unexpectedly, the timer routine ut_tmr1() did not exec "
                "exactly 1 time.\n");
        osal_utErrorsFound++;
        goto _OSAL_UT_TMR_TEST_5;
    }

    status = OSAL_tmrStop(vtId1);
    if (OSAL_SUCCESS != status) {
        prError1("Unexpected OSAL_tmrStop() retval - expected success, got "
            "0x%08x\n", (unsigned) status);
        osal_utErrorsFound++;
        goto _OSAL_UT_TMR_TEST_5;
    }

_OSAL_UT_TMR_TEST_5:
    /* Periodic Tmr Start Verification...  */
    OSAL_logMsg("Timer test 5\n");
    /* Test Periodic Timer Start with invalid msTimeout */
    status = OSAL_tmrPeriodicStart(vtId1, (OSAL_TmrPtr)ut_tmr1,
            (void *) OSAL_FALSE, OSAL_NO_WAIT);
    if (OSAL_FAIL != status) {
        prError("OSAL_tmrPeriodicStart() didn't OSAL_FAIL as expected.\n");
        osal_utErrorsFound++;
        goto _OSAL_UT_TMR_TEST_6;
    }

    status = OSAL_tmrPeriodicStart(vtId1, (OSAL_TmrPtr)ut_tmr1,
            (void *) OSAL_FALSE, OSAL_WAIT_FOREVER);
    if (OSAL_FAIL != status) {
        prError("OSAL_tmrPeriodicStart() didn't OSAL_FAIL as expected.\n");
        osal_utErrorsFound++;
        goto _OSAL_UT_TMR_TEST_6;
    }

_OSAL_UT_TMR_TEST_6:
    OSAL_logMsg("Timer test 6\n");

    /* Test Periodic Timer Start
     * this should work fine...
     */
    status =  OSAL_tmrPeriodicStart(vtId1, (OSAL_TmrPtr)ut_tmr1, OSAL_FALSE,
            5000);
    if (OSAL_SUCCESS != status) {
        prError1("Unexpected OSAL_tmrPeriodicStart() retval - expected "
                "success, got 0x%08x\n", (unsigned) status);
        osal_utErrorsFound++;
        goto _OSAL_UT_TMR_TEST_7;
    }

    status = OSAL_tmrStop(vtId1);
    if (OSAL_SUCCESS != status) {
        prError1("Unexpected OSAL_tmrStop() retval - expected success, got "
                "0x%08x\n", (unsigned) status);
        osal_utErrorsFound++;
        goto _OSAL_UT_TMR_TEST_7;
    }


_OSAL_UT_TMR_TEST_7:
    OSAL_logMsg("Timer test 7\n");
    /* check deadlock prevention */

    status = OSAL_tmrPeriodicStart(vtId1, (OSAL_TmrPtr)ut_tmr1,
            (void *) OSAL_FALSE, (uint32) 10000);
    if (OSAL_SUCCESS != status) {
        prError("OSAL_tmrPeriodicStart() didn't succeed as expected.\n");
        osal_utErrorsFound++;
        goto _OSAL_UT_TMR_TEST_8;
    }

    status = OSAL_tmrStop(vtId1);
    if (OSAL_SUCCESS != status) {
        prError("OSAL_tmrStop() didn't succeed as expected.\n");
        osal_utErrorsFound++;
        goto _OSAL_UT_TMR_TEST_8;
    }

_OSAL_UT_TMR_TEST_8:
    /* Start a timer - expect good start here */
    OSAL_logMsg("Timer test 8\n");

    ut_tmr1_completed = OSAL_FALSE;
    ut_tmr1_invocations = 0;

    /* The arg OSAL_FALSE tells the timer routine this is not a one-shot
     * and the ut_tmr1_completed flag will be set after 5 invocations
     * that occur 1000 ms apart... (try this at 10ms and the limits of a
     * non-rt kernel are pathetically exposed
     */
    status = OSAL_tmrPeriodicStart(vtId1, (OSAL_TmrPtr)ut_tmr1,
            (void *) OSAL_FALSE, (uint32) 1000);
    if (OSAL_SUCCESS != status) {
        prError("OSAL_tmrPeriodicStart() didn't succeed as expected.\n");
        osal_utErrorsFound++;
        goto _OSAL_UT_TMR_TEST_9;
    }

    /* delay for 7.5 seconds then verify our timer was run only 7 times */
    OSAL_taskDelay(7500);
    if (OSAL_TRUE != ut_tmr1_completed) {
        prError("Unexpectedly, the timer routine ut_tmr1() did not "
                "complete.\n");
        osal_utErrorsFound++;
        goto _OSAL_UT_TMR_TEST_9;
    }
    status = OSAL_tmrStop(vtId1);
    if (OSAL_SUCCESS != status) {
        prError1("Unexpected OSAL_tmrStop() retval - expected success, got "
                "0x%08x\n", (unsigned) status);
        osal_utErrorsFound++;
        goto _OSAL_UT_TMR_TEST_9;
    }
    if (7 != ut_tmr1_invocations) {
        prError("Unexpectedly, the timer routine ut_tmr1() did not exec "
                "exactly 7 times.\n");
        osal_utErrorsFound++;
        goto _OSAL_UT_TMR_TEST_9;
    }

    /* OSAL_tmrPeriodicStart() testing complete... we have one
     * timer created (vtId1) but not running
     */


_OSAL_UT_TMR_TEST_9:
   /* Timer Stop Verification...  */

    OSAL_logMsg("Timer test 9\n");
    /* Bad timer ID */
    status = OSAL_tmrStop(NULL);
    if (OSAL_FAIL != status) {
        prError("OSAL_tmrStop() didn't fail as expected.\n");
        osal_utErrorsFound++;
        goto _OSAL_UT_TMR_TEST_10;
    }

    /* Start it to test normal task stop */
    status = OSAL_tmrStart(vtId1, (OSAL_TmrPtr)ut_tmr1, (void *) OSAL_TRUE,
            5000);
    if (OSAL_SUCCESS != status) {
        prError("OSAL_tmrStop() didn't fail as expected.\n");
        osal_utErrorsFound++;
        goto _OSAL_UT_TMR_TEST_10;
    }
    status = OSAL_tmrStop(vtId1);
    if (OSAL_SUCCESS != status) {
        prError("OSAL_tmrStop() didn't succeed as expected.\n");
        osal_utErrorsFound++;
        goto _OSAL_UT_TMR_TEST_10;
    }

    /* Restart it to test normal irq-level stop */
    status = OSAL_tmrStart(vtId1, (OSAL_TmrPtr)ut_tmr1, (void *) OSAL_TRUE,
            5000);
    if (OSAL_SUCCESS != status) {
        prError("OSAL_tmrStop() didn't fail as expected.\n");
        osal_utErrorsFound++;
        goto _OSAL_UT_TMR_TEST_10;
    }

    status = OSAL_tmrStop(vtId1);

    if (OSAL_SUCCESS != status) {
        prError1("Unexpected OSAL_tmrStop() retval - expected OSAL_SUCCESS, "
                "got 0x%08x\n", (unsigned) status);
    }


_OSAL_UT_TMR_TEST_10:
    OSAL_logMsg("Timer test 10\n");
    /* Check Mutex protection... */
    status = OSAL_tmrStart(vtId1, (OSAL_TmrPtr)ut_tmr1, (void *) OSAL_TRUE,
            5000);

    status = OSAL_tmrStop(vtId1);
    if (OSAL_SUCCESS != status) {
        prError("OSAL_tmrStop() didn't succeed as expected.\n");
        osal_utErrorsFound++;
        goto _OSAL_UT_TMR_TEST_11;
    }


    /* Start it to test normal task refresh - one-shot tmr w/2sec trigger */
    ut_tmr1_completed = OSAL_FALSE;
    ut_tmr1_invocations = 0;

    status = OSAL_tmrStart(vtId1, (OSAL_TmrPtr)ut_tmr1, (void *) OSAL_TRUE,
            2000);
    if (OSAL_SUCCESS != status) {
        prError("OSAL_tmrStart() didn't succeed as expected.\n");
        osal_utErrorsFound++;
        goto _OSAL_UT_TMR_TEST_11;
    }
    OSAL_taskDelay(1000);

    if ((OSAL_TRUE == ut_tmr1_completed) || ut_tmr1_invocations) {
        prError("OSAL_tmrRefresh() didn't perform as expected - the timer "
                "triggered.\n");
        osal_utErrorsFound++;
        goto _OSAL_UT_TMR_TEST_11;
    }

    /* now delay for 1 second to verify it has gone off */
    OSAL_taskDelay(1000);

    if ((OSAL_TRUE != ut_tmr1_completed) && (1 != ut_tmr1_invocations)) {
        prError("OSAL_tmrRefresh() didn't perform as expected - \n");
        prError("    the timer still has not triggered.\n");
        osal_utErrorsFound++;
        goto _OSAL_UT_TMR_TEST_11;
    }

    /* now delay for 3 second to verify it has not gone off further */
    OSAL_taskDelay(3000);

    if ((OSAL_TRUE != ut_tmr1_completed) && (1 != ut_tmr1_invocations)) {
        prError("OSAL_tmrRefresh() didn't perform as expected - \n");
        prError("    the timer still has not triggered.\n");
        osal_utErrorsFound++;
        goto _OSAL_UT_TMR_TEST_11;
    }

_OSAL_UT_TMR_TEST_11:
    /* 
    * Timer test 11:
    * Test OSAL_tmrInterrupt() for wake up a timer.
    */
    OSAL_logMsg("Timer test 11\n");
    status = OSAL_tmrStart(vtId1, (OSAL_TmrPtr)ut_tmr1, (void *) OSAL_TRUE,
            000);
    /* delay 0.5 second and then wakeup the timer */
    OSAL_taskDelay(500);
    OSAL_tmrInterrupt(vtId1);
    /* verify our timer was run only once */
    if (OSAL_TRUE != ut_tmr1_completed) {
        prError("Unexpectedly, the timer routine ut_tmr1() did not "
                "complete.\n");
        osal_utErrorsFound++;
        goto _OSAL_UT_TMR_TEST_12;
    }
    if (1 != ut_tmr1_invocations) {
        prError("Unexpectedly, the timer routine ut_tmr1() did not exec "
                "exactly 1 time.\n");
        osal_utErrorsFound++;
        goto _OSAL_UT_TMR_TEST_12;
    }
    status = OSAL_tmrStop(vtId1);
    if (OSAL_SUCCESS != status) {
        prError1("Unexpected OSAL_tmrStop() retval - expected success, got "
            "0x%08x\n", (unsigned) status);
        osal_utErrorsFound++;
        goto _OSAL_UT_TMR_TEST_12;
    }    

_OSAL_UT_TMR_TEST_12:
    OSAL_logMsg("Timer test 12\n");
    /* Delete the timer */
    status = OSAL_tmrDelete(vtId1);
    if (OSAL_SUCCESS != status) {
        prError("OSAL_tmrDelete() didn't succeed as expected.\n");
        osal_utErrorsFound++;
    }

    if (0 == osal_utErrorsFound) {
        OSAL_logMsg("Timer Unit Test Completed Successfully.\n");
        return (UT_PASS);
    }
    else {
        OSAL_logMsg("Timer Unit Test Completed with %d errors.\n",
                osal_utErrorsFound);
        return (UT_FAIL);
    }
}
