/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 8919 $ $Date: 2009-02-14 07:19:11 +0800 (Sat, 14 Feb 2009) $
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

#define MAX_MQ          10

static OSAL_MsgQId           mqId[MAX_MQ];
static OSAL_TaskId           rcvrId[MAX_MQ];
static OSAL_TaskId           sndrId[MAX_MQ];

static uint64                send_timeouts[MAX_MQ],
                             send_attempts[MAX_MQ],
                             recv_timeouts[MAX_MQ],
                             recv_badMsg[MAX_MQ],
                             recv_errors[MAX_MQ],
                             recv_attempts[MAX_MQ];

 /*
 * ========= _OSAL_testSend() ========
 * Funtion to send msg.
 *
 * Return:
 *  None
 */
static void _OSAL_testSend(
    uint32 taskArg)
{
    int    instance;
    uint32 msgBuf=0;
    OSAL_Boolean timeout;

    instance = taskArg;

    send_attempts[instance]=0;
    send_timeouts[instance]=0;
    while(1) {
        send_attempts[instance]++;
        if (OSAL_FAIL == OSAL_msgQSend(mqId[instance], (char *) &msgBuf,
                sizeof(msgBuf), 10, &timeout)) {
            if (OSAL_TRUE == timeout) {
                prError("send timeout\n");
                send_timeouts[instance]++;
            }
            else {
                OSAL_logMsg("send error\n");
            }
            continue;
        }
        msgBuf++;
    }
}

 /*
 * ========= _OSAL_testRcvr() ========
 * Funtion to recv msg.
 *
 * Return:
 *  None
 */
static void _OSAL_testRcvr(
    uint32 taskArg)
{
    uint32 msgBuf;
    uint32 expected = 0;
    uint32 numBytes;
    int    instance;
    OSAL_Boolean timeout;

    instance = taskArg;

    recv_timeouts[instance] = 0;
    recv_badMsg[instance] = 0;
    recv_errors[instance] = 0;
    recv_attempts[instance] = 0;
    while(1) {
        recv_attempts[instance]++;
        numBytes = OSAL_msgQRecv(mqId[instance],(char *)
                &msgBuf,sizeof(msgBuf),10, &timeout);
        if (numBytes != sizeof(msgBuf)) {
            if (OSAL_TRUE == timeout) {
                recv_timeouts[instance]++;
            }
            else {
                prError2("recv error, got %u, size %d\n", msgBuf, numBytes);
                recv_errors[instance]++;
            }

            continue;
        }
        if (msgBuf != expected) {
            OSAL_logMsg("badMsg: %u instead of %u on instance: %d\n", msgBuf,
                    expected, instance);
            recv_badMsg[instance]++;
        }

        expected++;
    }
}

 /*
 * ========= do_test_stresstask() ========
 * Gen unit test vectors for each OSAL task function.
 *
 * Return:
 *  UT_PASS: Exit normally.
 *  UT_FAIL: Invalid arguments.
 */
UT_Return do_test_stresstask(
    void)
{
    int                             instance;
    char                            taskName[20];
    int                             i;
    uint64                          st, sa, rt, ra, rb, re;
    OSAL_Status                     status;
    char                            qname[7];


    uint32       taskArg;

    OSAL_logMsg("Task Stress Test Starting...\n");

    osal_utErrorsFound = 0;
    for (instance=0;instance<MAX_MQ;instance++) {
        OSAL_snprintf(qname, 8, "mqId%d", instance);
        mqId[instance] = OSAL_msgQCreate(qname,
                OSAL_MODULE_OSAL_UT, OSAL_MODULE_OSAL_UT, OSAL_DATA_STRUCT_int,
                (int32) sizeof(uint32), (int32) 4, (uint32) 0);
        if (NULL == mqId[instance]) {
            prError1("Unexpected OSAL_msgQCreate(4,4) retval - got NULL "
                    "(inst %d)\n", instance);
            return (UT_FAIL);
        }
    }

    /* create the receivers first... */
    for (instance=0;instance<MAX_MQ;instance++) {
        OSAL_snprintf(taskName, sizeof(taskName), "tRecvr%d", instance);

        taskArg = instance;

        rcvrId[instance] = OSAL_taskCreate(taskName, OSAL_TASK_PRIO_NRT,
                (uint32) 8000, (OSAL_TaskPtr) _OSAL_testRcvr, (void *) taskArg);

        if ((OSAL_TaskId) NULL == rcvrId[instance]) {
            prError1("OSAL_taskCreate() failed on receiver %d\n", instance);
            return (UT_FAIL);
        }
    }

    /* now the senders... */
    for (instance=0;instance<MAX_MQ;instance++) {
        OSAL_snprintf(taskName, sizeof(taskName), "tSendr%d", instance);

        taskArg = instance;

        sndrId[instance] = OSAL_taskCreate(taskName, OSAL_TASK_PRIO_NRT,
                (uint32) 8000, (OSAL_TaskPtr)_OSAL_testSend, (void *) taskArg);

        if ((OSAL_TaskId) NULL == sndrId[instance]) {
            prError1("OSAL_taskCreate() failed on sender %d\n", instance);
            return (UT_FAIL);
        }
    }

    /* wait for 10 seconds then exit */
    OSAL_taskDelay(10000);


    /*
     *We should have blocked and the task should have run because
     * it's priority is greater than ours...
     */
    st= sa= rt= ra= rb= re= 0;
    for (i=0;i<MAX_MQ;i++) {
        st+= send_timeouts[i];
        sa+= send_attempts[i];
        rt+= recv_timeouts[i];
        ra+= recv_attempts[i];
        rb+= recv_badMsg[i];
        re+= recv_errors[i];
    }
    OSAL_logMsg("%llu send timeouts out of %llu send attempts\n",
            st, sa);
    OSAL_logMsg("%llu recv timeouts out of %llu recv attempts (%llu bad mesgs/%llu "
            "errors)\n", rt, ra, rb, re);


    if (0 == osal_utErrorsFound) {
        OSAL_logMsg("Task Stress Test Completed Successfully.\n");
    }
    else {
        OSAL_logMsg("Task Stress Test Completed with %d errors.\n",
                osal_utErrorsFound);
    }

    /*
     * Finally, delete the created receive and send tasks
     * Otherwise the console hangs, when main program exits
     */
    for (instance = 0; instance < MAX_MQ; instance++) {
        status = OSAL_taskDelete(rcvrId[instance]);
        if (OSAL_SUCCESS != status) {
            prError2("OSAL_taskDelete(rcvrId[%d]) not OSAL_SUCCESS; got "
                    "0x%08x\n", instance, (unsigned) status);
        }
    }
    for (instance = 0; instance < MAX_MQ; instance++) {
        status = OSAL_taskDelete(sndrId[instance]);
        if (OSAL_SUCCESS != status) {
            prError2("OSAL_taskDelete(sndrId[%d]) not OSAL_SUCCESS; got "
                    "0x%08x\n", instance, (unsigned) status);
        }
    }

    if (osal_utErrorsFound) {
        return (UT_FAIL);
    }
    else {
        return (UT_PASS);
    }
}
