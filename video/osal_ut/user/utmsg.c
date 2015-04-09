/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 11815 $ $Date: 2010-04-16 06:06:18 +0800 (Fri, 16 Apr 2010) $
 *
 */

#include "osal_ut.h"

#define     OSAL_UT_MSG_Q_NAME  "testq"


/*
 * ======== _OSALUT_msgGrpQSend() ========
 * This task is to test group queue send to different task.
 *
 * Returns:
 *   UT_PASS: Exit normally.
 */
static UT_Return _OSALUT_msgGrpQSend(
    void)
{
    OSAL_MsgQId     vmId;
    unsigned char   msgBuf[8];
    vint            idx;

    vmId = OSAL_msgQCreate(OSAL_UT_MSG_Q_NAME,
            OSAL_MODULE_OSAL_UT, OSAL_MODULE_OSAL_UT, OSAL_DATA_STRUCT_int,
            20, (int32) sizeof(msgBuf), (int32)0);
    if (NULL == vmId) {
        prError1("Unexpected OSAL_msgQCreate() retval - expected NULL, "
                "got 0x%08x\n", (unsigned) vmId);
        return (UT_FAIL);
    }

    /* Add 100 messages to the queue */
    for (idx = 0; idx < 20; idx++) {
        msgBuf[0] = (char) idx;
        if (OSAL_FAIL == OSAL_msgQSend(vmId, msgBuf, sizeof(msgBuf),
                OSAL_NO_WAIT, NULL)) {
            prError("Unexpected OSAL_msgQSend() Failure.\n");
            return (UT_FAIL);
        }
        OSAL_taskDelay(1000);
    }
    return (UT_PASS);
}

/*
 * ======== _OSALUT_msgGrpQRecv() ========
 * This task is to test group queue receiving from different task.
 *
 * Returns:
 *   UT_PASS: Exit normally.
 */
static UT_Return _OSALUT_msgGrpQRecv(
    void)
{
    OSAL_MsgQId     vmId, whichVmq;
    OSAL_MsgQGrpId  vgId;
    unsigned char   msgBuf[8];
    vint            numBytes, idx = 0;

    vmId = OSAL_msgQCreate(OSAL_UT_MSG_Q_NAME,
            OSAL_MODULE_OSAL_UT, OSAL_MODULE_OSAL_UT, OSAL_DATA_STRUCT_int,
            20, (int32) sizeof(msgBuf), (int32)0);
    if (NULL == vmId) {
        prError1("Unexpected OSAL_msgQCreate() retval - expected NULL, "
                "got 0x%08x\n", (unsigned) vmId);
        return (UT_FAIL);
    }

    if (OSAL_SUCCESS != OSAL_msgQGrpCreate(&vgId)) {
        prError("Unexpected OSAL_msgQGrpCreate() retval - expected non-NULL, "
                "got NULL\n");
        return (UT_FAIL);
    }

    if (OSAL_SUCCESS != OSAL_msgQGrpAddQ(&vgId, vmId)) {
        prError("Unexpected OSAL_msgQGrpAddQ() retval - expected "
                "OSAL_SUCCESS\n");
        return (UT_FAIL);
    }

    while (20 > idx) {
        numBytes = OSAL_msgQGrpRecv(&vgId, msgBuf, sizeof(msgBuf),
                OSAL_WAIT_FOREVER, &whichVmq, NULL);
        if (numBytes != sizeof(msgBuf)) {
            prError2("Unexpected OSAL_msgQGrpRecv() failure - expected %d "
                    "bytes, rcvd %d.\n", (int) sizeof(msgBuf), (int)numBytes);
            return (UT_FAIL);
        }
        if (msgBuf[0] != (unsigned char) idx) {
            prError2("Unexpected OSAL_msgQGrpRecv() failure - expected buffer "
                    "%d, rcvd %d.\n", idx, (int) msgBuf[0]);
            return (UT_FAIL);
        }
        if (whichVmq != vmId) {
            prError("Reception on Unexpected Queue.\n");
            return (UT_FAIL);
        }
        idx++;
    }
    /* Group queue delete */
    if (OSAL_SUCCESS != OSAL_msgQGrpDelete(&vgId)) {
        prError("Unexpected OSAL_msgQGrpDelete() retval\n");
    }
    /* Message queue delete */
    if (OSAL_SUCCESS != OSAL_msgQDelete(vmId)) {
        prError("Unexpected OSAL_msgQDelete() retval\n");
    }
    return (UT_PASS);
}


/*
 * ========= do_test_msg() ========
 * Gen unit test vectors for each OSAL msg functionn
 *
 * Return:
 *   UT_PASS: success.
 *   UT_FAIL: fail.
 */
UT_Return do_test_msg(
    void)
{
    OSAL_MsgQId     vmId1, vmId2, vmId3, whichVmq;
    OSAL_MsgQGrpId  vgId1;
    OSAL_Status     status;
    int             numBytes;
    unsigned char   msgBuf[8];
    register int    i;
    OSAL_TaskId     vtId1, vtId2;

    OSAL_logMsg("Message Unit Test Starting...\n");


    /* Reset this before every test */
    osal_utErrorsFound = 0;


    /* good to go let's test basic messaging... */
    /* MsgQ Creation    Verification...  */


    /* NULL name */
    vmId1 = OSAL_msgQCreate(NULL,
            OSAL_MODULE_OSAL_UT, OSAL_MODULE_OSAL_UT, OSAL_DATA_STRUCT_int,
            0 , (int32) 64, (int32)0);
    if (NULL != vmId1) {
        prError1("Unexpected OSAL_msgQCreate() retval - expected NULL, "
                "got 0x%08x\n", (unsigned) vmId1);
    }


    /* Exceed Memory capacity */
    vmId1 = OSAL_msgQCreate("testq1",
            OSAL_MODULE_OSAL_UT, OSAL_MODULE_OSAL_UT, OSAL_DATA_STRUCT_int,
            0, (int32) 0x20000000, (int32)0);
    if (NULL != vmId1) {
        prError1("Unexpected OSAL_msgQCreate() retval - expected NULL, "
                "got 0x%08x\n", (unsigned) vmId1);
    }


    /* First good msg Queue Creation... */
    /* 8kB Queue */
    vmId1 = OSAL_msgQCreate("testq1",
            OSAL_MODULE_OSAL_UT, OSAL_MODULE_OSAL_UT, OSAL_DATA_STRUCT_int,
            1, (int32) sizeof(msgBuf), (int32)0);
    if (NULL == vmId1) {
        prError("Unexpected OSAL_msgQCreate() retval - expected non-NULL, "
                "got NULL\n");
    }
    /* Verification of msgQCreate() complete... */


    /* MsgQ Deletion  Verification...    */
    /* NULL msgId */
    status = OSAL_msgQDelete(NULL);
    if (OSAL_FAIL != status) {
        prError1("Unexpected OSAL_msgQDelete() retval - expected OSAL_FAIL, "
                "got 0x%08x\n", (unsigned) status);
    }

    /* check deadlock prevention - in OSAL, Mutex is recursively taken
     * successfully, therefore, have another task take the mutex, delay for
     * a second, and we continue verifying the elapsed time of 3 second...
     */
#if 0
    vtId = OSAL_taskCreate("tVosTestMutex",(uint32) -19, (uint32) 8000,
            (void *)_OSAL_testMsgMutex, (uint32) NULL);
    if ((OSAL_TaskId)NULL == vtId) {
        prError("Unexpected OSAL_taskCreate() retval - expected "
                "non-NULL, got NULL\n");
    }
#endif

    status = OSAL_msgQDelete(vmId1);
    if (OSAL_SUCCESS != status) {
        prError1("Unexpected OSAL_msgQDelete() retval - expected OSAL_SUCCESS, "
                "got 0x%08x\n", (unsigned) status);
    }



    /* Create a q, 8 byte entries each and 100 entries */
    vmId1 = OSAL_msgQCreate("testq1",
            OSAL_MODULE_OSAL_UT, OSAL_MODULE_OSAL_UT, OSAL_DATA_STRUCT_int,
            100, (int32) sizeof(msgBuf), (uint32) 0);
    if (NULL == vmId1) {
        prError("Unexpected OSAL_msgQCreate() retval - expected non-NULL, "
                "got NULL\n");
    }




    /* Add 42 messages to the queue */
    for (i=0;i<42;i++) {
        msgBuf[0] = (char) i;
        if (OSAL_FAIL == OSAL_msgQSend(vmId1, msgBuf, sizeof(msgBuf),
                OSAL_NO_WAIT, NULL)) {
            prError("Unexpected OSAL_msgQSend() Failure.\n");
        }
    }

    /*
     *We've already sent 42 messages successfully on vmId1 in the last set
     * of tests. Check error handling...
     */

    /* NULL msgId */
    status = OSAL_msgQSend(NULL, msgBuf, sizeof(msgBuf), OSAL_NO_WAIT, NULL);
    if (OSAL_FAIL != status) {
        prError1("Unexpected OSAL_msgQSend() retval - expected OSAL_FAIL, "
                "got 0x%08x\n", (unsigned) status);
    }


    /* Set up msg buf #43 to id that this is 42 */
    msgBuf[0] = (char) 42;

    status = OSAL_msgQSend(vmId1, msgBuf, sizeof(msgBuf), OSAL_NO_WAIT, NULL);

    /*
     *We now have 43 msgs on the queue, fill it up then test blocking send
     * with timeout
     */
    for (i=43;i<100;i++) {
        msgBuf[0] = (unsigned char) i;
        if (OSAL_FAIL == OSAL_msgQSend(vmId1, msgBuf, sizeof(msgBuf),
                OSAL_NO_WAIT, NULL)) {
            prError("Unexpected OSAL_msgQSend() Failure.\n");
        }
    }
    /* Msg Q Send Test complete... - vmId1 is now full with 100 8-byte msgs */


    /* MsgQ Recv Msgs Verification... */
    /* Bad qId */
    numBytes = OSAL_msgQRecv(NULL, msgBuf, sizeof(msgBuf), OSAL_NO_WAIT, NULL);
    if (-1 != numBytes) {
        prError1("Unexpected OSAL_msgQRecv() retval - expected -1, "
                "got 0x%08x\n", (unsigned) numBytes);
    }

    /* check that we can still recv on vmId1 even though its grouped. */
    numBytes = OSAL_msgQRecv(vmId1,msgBuf,sizeof(msgBuf),OSAL_NO_WAIT, NULL);
    if (8 != numBytes) {
        prError1("Unexpected OSAL_msgQRecv() retval - expected 8, "
                "got 0x%08x\n", (unsigned) numBytes);
    }
    else /* make sure we recvd msg #1 */ {
        if (msgBuf[0] != (unsigned char) 0) {
            prError1("Unexpected OSAL_msgQRecv() failure - expected buffer 0, "
                    "rcvd %d.\n", (int) msgBuf[0]);
        }
    }

    /* Receive 99 good messages and verify order */
    for (i=1;i<100;i++) {
        numBytes = OSAL_msgQRecv(vmId1, msgBuf, sizeof(msgBuf), OSAL_NO_WAIT,
                NULL);
        if (numBytes != sizeof(msgBuf)) {
            prError2("Unexpected OSAL_msgQRecv() failure - expected %d bytes, "
                    "rcvd %d.\n", (int) sizeof(msgBuf), (int) numBytes);
        }
        if (msgBuf[0] != (unsigned char) i) {
            prError2("Unexpected OSAL_msgQRecv() failure - expected buffer %d, "
                    "rcvd %d.\n", i, (int) msgBuf[0]);
        }
    }

    /* vmId1 now empty, verify Pend with timeout... */
    numBytes = OSAL_msgQRecv(vmId1, msgBuf, sizeof(msgBuf), 1000 /* 1 sec */,
            NULL);
    if (-1 != numBytes) {
        prError1("Unexpected OSAL_msgQRecv() success (returned %d bytes).\n",
            numBytes);
    }


    /* Msg Q recv Test complete... - vmId1 is now empty with 0 of 100
     * 8-byte msgs
     */

    /* MsgQ Group Create Verification... */

    /* A Good group create... */

    if (OSAL_SUCCESS != OSAL_msgQGrpCreate(&vgId1)) {
        prError("Unexpected OSAL_msgQGrpCreate() retval - expected non-NULL, "
                "got NULL\n");
    }
    if (OSAL_SUCCESS != OSAL_msgQGrpDelete(&vgId1)) {
        prError("Unexpected OSAL_msgQGrpDelete() retval\n");
    }


    /*
     *Msg Q Group Create Test complete... - vmId1 is now empty with 0 of 100
     * 8-byte msgs , vgId1 has no groups added
     */

    /*
     *Msg Q Group Delete Test complete... - vmId1 is now empty with 0 of 100
     * 8-byte msgs , vgId1 is de-allocated
     */

    /* MsgQ Group Add Verification... */

    /* Create a good group */
    if (OSAL_SUCCESS != OSAL_msgQGrpCreate(&vgId1)) {
        prError("Unexpected OSAL_msgQGrpCreate() retval - expected non-NULL, "
                "got NULL\n");
    }


    /* Verify bad group id */
    status = OSAL_msgQGrpAddQ(NULL, vmId1);
    if (OSAL_FAIL != status) {
        prError1("Unexpected OSAL_msgQGrpAddQ() retval - expected FAIL, "
                "got %d.\n", (unsigned) status);
    }

    /* A Good group addQ... */
    status = OSAL_msgQGrpAddQ(&vgId1, vmId1);
    if (OSAL_FAIL == status) {
        prError("Unexpected OSAL_msgQGrpAddQ() retval - expected "
                "OSAL_SUCCESS\n");
    }






    /* Msg Q Group Addq Test complete... - vmId1 is now empty with 0 of 100
     * 8-byte msgs , vgId1 is allocated and vmId1 is placed on vgId1
     */
    /* MsgQ Group Remove Verification... */

    /* Verify bad group id */
    numBytes = OSAL_msgQGrpRecv(NULL, msgBuf, (int) sizeof(msgBuf),
            OSAL_NO_WAIT, &whichVmq, NULL);
    if (-1 != numBytes) {
        prError1("Unexpected OSAL_msgQGrpRecv() retval - expected -1, "
                "got %d.\n", (unsigned) numBytes);
    }

    /* Verify no msgQueue entries */
    numBytes = OSAL_msgQGrpRecv(&vgId1, msgBuf, (int) sizeof(msgBuf),
            OSAL_NO_WAIT, &whichVmq, NULL);
    if (-1 != numBytes) {
        prError1("Unexpected OSAL_msgQGrpRecv() retval - expected -1, "
                "got %d.\n", (unsigned) numBytes);
    }

    /* Add vmId1 back into the vgId1 group */
    status = OSAL_msgQGrpAddQ(&vgId1, vmId1);
    if (OSAL_FAIL == status) {
        prError("Unexpected OSAL_msgQGrpAddQ() retval - expected "
                "OSAL_SUCCESS\n");
    }


    /* Verify TIMEOUT since vmId1 has no messages in it */
    numBytes = OSAL_msgQGrpRecv(&vgId1, msgBuf, (int) sizeof(msgBuf),
            OSAL_NO_WAIT, &whichVmq, NULL);
    if (-1 != numBytes) {
        prError1("Unexpected OSAL_msgQGrpRecv() retval - expected -1, "
                "got %d.\n", (unsigned) numBytes);
    }

    numBytes = OSAL_msgQGrpRecv(&vgId1, msgBuf, (int) sizeof(msgBuf), 1000,
        &whichVmq, NULL);
    if (-1 != numBytes) {
        prError1("Unexpected OSAL_msgQGrpRecv() retval - expected -1, "
                "got %d.\n", (unsigned) numBytes);
    }

    /*
     *Error processing complete - check functional testing. For this, we need
     * a few more msg queues...
     */

    /* Create 2 more qs with 100 entries, 8 bytes each */
    vmId2 = OSAL_msgQCreate("testq2",
            OSAL_MODULE_OSAL_UT, OSAL_MODULE_OSAL_UT, OSAL_DATA_STRUCT_int,
            100, (int32) sizeof(msgBuf), (uint32) 0);
    if (NULL == vmId2) {
        prError("Unexpected OSAL_msgQCreate() retval - expected non NULL, "
                "got NULL.\n");
    }

    vmId3 = OSAL_msgQCreate("testq3",
            OSAL_MODULE_OSAL_UT, OSAL_MODULE_OSAL_UT, OSAL_DATA_STRUCT_int,
            100, (int32) sizeof(msgBuf), (uint32) 0);
    if (NULL == vmId3) {
        prError("Unexpected OSAL_msgQCreate() retval - expected non NULL, "
                "got NULL.\n");
    }


    /* add vmId2 and 3 into vgId1 */
    status = OSAL_msgQGrpAddQ(&vgId1, vmId2);
    if (OSAL_FAIL == status) {
        prError("Unexpected OSAL_msgQGrpAddQ() retval - expected "
                "OSAL_SUCCESS\n");
    }
    status = OSAL_msgQGrpAddQ(&vgId1, vmId3);
    if (OSAL_FAIL == status) {
        prError("Unexpected OSAL_msgQGrpAddQ() retval - expected "
                "OSAL_SUCCESS\n");
    }

    /*
     *vgId1 is a PRIORITY based group with vmId1 as highest priority,
     * followed by vmId2 and lastly vmId3. Add 10 msgs to each group numbered
     * 1-10 in vmId1, 11-20 in vmId2, and 21-30 in vmId3. Then do 30
     * non-blocking msg group receives and verify that 30 msgs are received in
     * order.
     */
    for (i=0;i<30;i++) {
        msgBuf[0] = (unsigned char) i;
        if (i<10) {
            if (OSAL_FAIL == OSAL_msgQSend(vmId1,msgBuf,sizeof(msgBuf),
                    OSAL_NO_WAIT, NULL)) {
                prError("Unexpected OSAL_msgQSend() Failure.\n");
            }
        }
        else if (i<20) {
            if (OSAL_FAIL == OSAL_msgQSend(vmId2,msgBuf,sizeof(msgBuf),
                    OSAL_NO_WAIT, NULL)) {
                prError("Unexpected OSAL_msgQSend() Failure.\n");
            }
        }
        else {
            if (OSAL_FAIL == OSAL_msgQSend(vmId3,msgBuf,sizeof(msgBuf),
                    OSAL_NO_WAIT, NULL)) {
                prError("Unexpected OSAL_msgQSend() Failure.\n");
            }
        }
    }

    /* Receive 30 good messages from the group and verify order */
    for (i=0;i<30;i++) {
        numBytes = OSAL_msgQGrpRecv(&vgId1, msgBuf, sizeof(msgBuf),
                OSAL_NO_WAIT, &whichVmq, NULL);
        if (numBytes != sizeof(msgBuf)) {
            prError2("Unexpected OSAL_msgQGrpRecv() failure - expected %d "
                    "bytes, rcvd %d.\n", (int) sizeof(msgBuf), (int)numBytes);
        }
        if (msgBuf[0] != (unsigned char) i) {
            prError2("Unexpected OSAL_msgQGrpRecv() failure - expected buffer "
                    "%d, rcvd %d.\n", i, (int) msgBuf[0]);
        }
        if (i<10) {
            if (whichVmq != vmId1) {
                prError("Reception on Unexpected Queue.\n");
            }
        }
        else if (i<20) {
            if (whichVmq != vmId2) {
                prError("Reception on Unexpected Queue.\n");
            }
        }
        else {
            if (whichVmq != vmId3) {
                prError("Reception on Unexpected Queue.\n");
            }
        }
    }
    /* Group queue delete */
    if (OSAL_SUCCESS != OSAL_msgQGrpDelete(&vgId1)) {
        prError("Unexpected OSAL_msgQGrpDelete() retval\n");
    }
    /* Message queue delete */
    if (OSAL_SUCCESS != OSAL_msgQDelete(vmId1)) {
        prError("Unexpected OSAL_msgQDelete() retval\n");
    }
    if (OSAL_SUCCESS != OSAL_msgQDelete(vmId2)) {
        prError("Unexpected OSAL_msgQDelete() retval\n");
    }
    if (OSAL_SUCCESS != OSAL_msgQDelete(vmId3)) {
        prError("Unexpected OSAL_msgQDelete() retval\n");
    }


    /* Create 2 task to send/receive group queue */
    vtId1 = OSAL_taskCreate("groupQueueSend", OSAL_TASK_PRIO_NRT, (uint32) 8000,
            (OSAL_TaskPtr)_OSALUT_msgGrpQSend, NULL);
    if (NULL == vtId1) {
        prError("Unexpected OSAL_taskCreate() retval - expected non-NULL "
                "from IRQ, got NULL\n");
    }
    vtId2 = OSAL_taskCreate("groupQueueRecv", OSAL_TASK_PRIO_NRT, (uint32) 8000,
            (OSAL_TaskPtr)_OSALUT_msgGrpQRecv, NULL);
    if (NULL == vtId2) {
        prError("Unexpected OSAL_taskCreate() retval - expected non-NULL "
                "from IRQ, got NULL\n");
    }

    /* Delay to give thread a chance to get started */
    OSAL_taskDelay(60000);

    if (OSAL_SUCCESS != OSAL_taskDelete(vtId1)) {
        prError1("Unexpected OSAL_taskDelete() retval - expected OSAL_SUCCESS, "
                "got 0x%08x\n", (unsigned) status);
    }
    if (OSAL_SUCCESS != OSAL_taskDelete(vtId2)) {
        prError1("Unexpected OSAL_taskDelete() retval - expected OSAL_SUCCESS, "
                "got 0x%08x\n", (unsigned) status);
    }


    /* MsgQ Group NumMsgs Verification... */

    if (0 == osal_utErrorsFound) {
        OSAL_logMsg("MsgQ Unit Test Completed Successfully.\n");
        return (UT_PASS);
    }
    else {
        OSAL_logMsg("MsgQ Unit Test Completed with %d errors.\n",
                osal_utErrorsFound);
        return (UT_FAIL);
    }
}
