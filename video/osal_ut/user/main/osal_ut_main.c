/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 8919 $ $Date: 2009-02-14 07:19:11 +0800 (Sat, 14 Feb 2009) $
 *
 */

/* this should be all the user needs */
#include "osal_ut.h"

int osal_utErrorsFound;

pthread_cond_t     shutdown_cond; /* for signalling shutdown */
pthread_mutex_t    shutdown_mutex;

/*
 * ========= OSAL_utShutdown() ========
 * Function to shutdown osal user ut.
 *
 * Return:
 *  None.
 */
static void OSAL_utShutdown(
    void)
{
    pthread_mutex_lock(&shutdown_mutex);
    pthread_cond_broadcast(&shutdown_cond);
    pthread_mutex_unlock(&shutdown_mutex);
}

/*
 * ======== OSAL_utmain() ========
 * Gen unit test vectors for each OSAL function
 *
 * Params:   regime - hex indicator to perform unit test for that regime
 *
 * Returns:
 *   None.
 */
static void OSAL_utmain(
    unsigned regime)
{
    int   passed;
    int   tests;
    int   i;
    char *failLog[128];

    passed = 0;
    tests = 0;

    if (regime & 0x01) {
        if (UT_PASS == do_test_mem()) {
            passed++;
        }
        else {
            failLog[tests-passed] = "Memory unit test failed\n";
        }
        tests++;
    }
    if (regime & 0x02) {
        if (UT_PASS == do_test_task()) {
            passed++;
        }
        else {
            failLog[tests-passed] = "Task unit test failed\n";
        }
        tests++;
    }
    if (regime & 0x04) {
        if (UT_PASS == do_test_msg()) {
            passed++;
        }
        else {
            failLog[tests-passed] = "Message unit test failed\n";
        }
        tests++;
    }
    if (regime & 0x010) {
        if (UT_PASS == do_test_timer()) {
            passed++;
        }
        else {
            failLog[tests-passed] = "Timer unit test failed\n";
        }
        tests++;
    }
    if (regime & 0x020) {
        if (UT_PASS == do_test_sem()) {
            passed++;
        }
        else {
            failLog[tests-passed] = "Semaphore unit test failed\n";
        }
        tests++;
    }
    if (regime & 0x080) {
        if (UT_PASS == do_test_stresstask()) {
            passed++;
        }
        else {
            failLog[tests-passed] = "Task Stress unit test failed\n";
        }
        tests++;
    }
    if (regime & 0x0100) {
        if (UT_PASS == do_test_dns()) {
            passed++;
        }
        else {
            failLog[tests-passed] = "DNS unit test failed\n";
        }
        tests++;
    }
    if (regime & 0x0200) {
        if (UT_PASS == do_test_aton()) {
            passed++;
        }
        else {
            failLog[tests-passed] = "ATON/NTOA unit test failed\n";
        }
        tests++;
    }
    if (regime & 0x0400) {
        if (UT_PASS == do_test_ipsec()) {
            passed++;
        }
        else {
            failLog[tests-passed] = "IPsec unit test failed\n";
        }
        tests++;
    }
    if (regime & 0x0800) {
        if (UT_PASS == do_test_crypto()) {
            passed++;
        }
        else {
            failLog[tests-passed] = "Crypto unit test failed\n";
        }
        tests++;
    }
    if (regime & 0x1000) {
        if (UT_PASS == do_test_file()) {
            passed++;
        }
        else {
            failLog[tests-passed] = "File unit test failed\n";
        }
        tests++;
    }
    if (regime & 0x2000) {
        if (UT_PASS == do_test_fifo()) {
            passed++;
        }
        else {
            failLog[tests-passed] = "Fifo unit test failed\n";
        }
        tests++;
    }
    if (regime & 0x4000) {
        if (UT_PASS == do_test_net()) {
            passed++;
        }
        else {
            failLog[tests-passed] = "Net unit test failed\n";
        }
        tests++;
    }    
    if (regime & 0x8000) {
        if (UT_PASS == do_test_ssl()) {
            passed++;
        }
        else {
            failLog[tests-passed] = "SSL unit test failed\n";
        }
        tests++;
    }
    OSAL_logMsg("All unit tests completed.\n");
    OSAL_logMsg("%d/%d tests passed\n",passed,tests);

    for (i = 0; i < (tests-passed); i++) {
        OSAL_logMsg("%s\n", failLog[i]);
    }

    OSAL_utShutdown();
}

/*
 * ========= usage() ========
 * Function to print the usage of osal user ut.
 *
 * Return:
 *    None.
 */
void usage(
    char *uname)
{
    OSAL_logMsg("usage: %s regime\n", uname);
    OSAL_logMsg("where regime is a bitwise OR of the following:\n");
    OSAL_logMsg("   0x01 - run mem tests\n");
    OSAL_logMsg("   0x02 - run task tests\n");
    OSAL_logMsg("   0x04 - run msg tests\n");
    OSAL_logMsg("   0x10 - run timer tests\n");
    OSAL_logMsg("   0x20 - run sem tests\n");
    OSAL_logMsg("   0x40 - run kernel-user IPC tests\n");
    OSAL_logMsg("   0x80 - run task stress tests\n");
    OSAL_logMsg("   0x100 - run dns test\n");
    OSAL_logMsg("   0x200 - run aton/ntoa tests\n");
    OSAL_logMsg("   0x400 - run ipsec tests\n");
    OSAL_logMsg("   0x800 - run crypto tests\n");
    OSAL_logMsg("   0x1000 - run file tests\n");
    OSAL_logMsg("   0x2000 - run fifo tests\n");
    OSAL_logMsg("   0x4000 - run net tests\n");
    OSAL_logMsg("   0x8000 - run ssl tests\n\n");
}

/*
 * ========= main() ========
 * Main function of osal user ut.
 *
 * Return:
 *  0: Exit normally.
 * -1: Invalid arguments.
 */
int main(
    int    argc,
    char **argv)
{
    unsigned                        regime;
    uid_t                           uid;


    if ((argc<2)||(argc>3)) {
        usage(argv[0]);
        return (-1);
    }

    uid = getuid();
    if (uid) {
        OSAL_logMsg("WARNING: in order to set task priorities lower than the unit "
                "test, the\n");
        OSAL_logMsg("         user should be logged in as root. Currently, the uid "
                "is %d.\n",uid);
    }

    regime = (unsigned) atoi(argv[1]);

    (void) pthread_cond_init(&shutdown_cond, NULL);
    (void) pthread_mutex_init(&shutdown_mutex, NULL);

    OSAL_taskCreate("osalUtmain", OSAL_TASK_PRIO_NRT, (uint32) 8000,
            (void *) OSAL_utmain, (void *) regime);

    /* Wait for OSAL_utmain to finish */
    pthread_cond_wait(&shutdown_cond, &shutdown_mutex);

    /* Quit the unit test and return to the console */
    OSAL_logMsg("         Unit test finished, exiting to console...\n");

    return (0);
}
