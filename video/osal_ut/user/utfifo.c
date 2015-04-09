/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev$ $Date$
 */

 /* this should be all the user needs */
#include "osal_ut.h"
#include "_utlatch.h"

#define OSALUT_FILE_TEST_BUF_SZ         32
#define OSALUT_FILE_TEST_LARGE_BUF_SZ   4000 
#define OSALUT_FIFO_TEST_FIFO_NAME      OSAL_IPC_FOLDER"fifo-test"
#define OSALUT_FIFO_TEST_FIFO_NAME2     OSAL_IPC_FOLDER"fifo-test2"
#define OSALUT_FIFO_TEST_FIFO_MSG_SZ    16
#define OSALUT_FIFO_TEST_MAX_MSG_SZ     1024

static char largeWriteBuff[OSALUT_FILE_TEST_LARGE_BUF_SZ] = {0xFF};
static char largeReadBuff[OSALUT_FILE_TEST_LARGE_BUF_SZ] = {0x00};

static int OSALUT_writeTaskStatus = 0;
static int OSALUT_write2TaskStatus = 0;
static int OSALUT_write3TaskStatus = 0;
static int OSALUT_write4TaskStatus = 0;
static int OSALUT_readTaskStatus  = 0;
static int OSALUT_read2TaskStatus  = 0;
static int OSALUT_read3TaskStatus  = 0;

/* Global latch */
static OSALUT_Latch latch;

/*
 * Fifo write task.
 */
static UT_Return OSALUT_FifoWriteTask(
    void)
{
    OSAL_FileId        fid;
    vint               index;
    vint               size;
    char               writeBuff[OSALUT_FILE_TEST_BUF_SZ];
    OSAL_SelectSet     writeSet;
    OSAL_Boolean       isSet;
    OSAL_Boolean       isTimeout;
    OSAL_SelectTimeval tv;

    /* Wait one second. */
    tv.sec = 0;
    tv.usec = 1000;

    if (OSAL_SUCCESS != OSAL_fileOpen(&fid, OSALUT_FIFO_TEST_FIFO_NAME,
            OSAL_FILE_O_RDWR, 0)) {
        prError("Failed to OSAL_fileOpen() with OSAL_FILE_O_RDWR. \n");
        OSALUT_latchUnlock(&latch);
        return (UT_FAIL);
    }

    size = OSALUT_FILE_TEST_BUF_SZ;
    for (index = 0; index < OSALUT_FIFO_TEST_MAX_MSG_SZ; index++) {
        OSAL_selectSetInit(&writeSet);
        OSAL_selectAddId(&fid, &writeSet);

        if (OSAL_FAIL == OSAL_select(NULL, &writeSet, &tv, &isTimeout)) {
            prError("Failed to select write fd.\n");
            OSALUT_latchUnlock(&latch);
            return (UT_FAIL);
        }

        if (isTimeout) {
            OSAL_logMsg("Write timeout\n");
            index--;
            /* Re-loop. */
            continue;
        } 

        if (OSAL_FAIL == OSAL_selectIsIdSet(&fid, &writeSet, &isSet)) {
            prError("OSAL_selectIsIdSet() failed\n");
            OSALUT_latchUnlock(&latch);
            return (UT_FAIL);
        }

        if (!isSet) {
            prError("Write fd is not set!!!\n");
            OSALUT_latchUnlock(&latch);
            return (UT_FAIL);
        }
        if (OSAL_SUCCESS != OSAL_fileWrite(&fid, writeBuff, &size)) {
            prError("Failed to OSAL_fileWrite(). \n");
            OSALUT_latchUnlock(&latch);
            return (UT_FAIL);
        }
        //OSAL_logMsg("Wrote:%d\n", index);
        /*
         * XXX There will be write error after write bunch of data
         * w/o a short delay.
         */
        OSAL_taskDelay(1);
    }

    if (OSAL_SUCCESS != OSAL_fileClose(&fid)) {
        prError("Failed to OSAL_fileClose().\n");
        OSALUT_latchUnlock(&latch);
        return (UT_FAIL);
    }
    OSAL_logMsg("OSALUT_FifoWriteTask DONE!!!!\n");
    OSALUT_writeTaskStatus = 1;

    OSALUT_latchUnlock(&latch);
    return (UT_PASS);
}

/*
 * Fifo write2 task.
 */
static UT_Return OSALUT_FifoWrite2Task(
    void)
{
    OSAL_FileId        fid;
    vint               index;
    vint               size;
    char               writeBuff[OSALUT_FILE_TEST_BUF_SZ];
    OSAL_SelectSet     writeSet;
    OSAL_Boolean       isSet;
    OSAL_Boolean       isTimeout;
    OSAL_SelectTimeval tv;

    /* Wait one second. */
    tv.sec = 0;
    tv.usec = 1000;

    if (OSAL_SUCCESS != OSAL_fileOpen(&fid, OSALUT_FIFO_TEST_FIFO_NAME,
            OSAL_FILE_O_RDWR, 0)) {
        prError("Failed to OSAL_fileOpen() with OSAL_FILE_O_RDWR. \n");
        OSALUT_latchUnlock(&latch);
        return (UT_FAIL);
    }

    size = OSALUT_FILE_TEST_BUF_SZ;
    for (index = 0; index < OSALUT_FIFO_TEST_MAX_MSG_SZ; index++) {
        OSAL_selectSetInit(&writeSet);
        OSAL_selectAddId(&fid, &writeSet);

        if (OSAL_FAIL == OSAL_select(NULL, &writeSet, &tv, &isTimeout)) {
            prError("Failed to select write fd.\n");
            OSALUT_latchUnlock(&latch);
            return (UT_FAIL);
        }

        if (isTimeout) {
            OSAL_logMsg("Write timeout\n");
            index--;
            /* Re-loop. */
            continue;
        } 

        if (OSAL_FAIL == OSAL_selectIsIdSet(&fid, &writeSet, &isSet)) {
            prError("OSAL_selectIsIdSet() failed\n");
            OSALUT_latchUnlock(&latch);
            return (UT_FAIL);
        }

        if (!isSet) {
            prError("Write fd is not set!!!\n");
            OSALUT_latchUnlock(&latch);
            return (UT_FAIL);
        }
        if (OSAL_SUCCESS != OSAL_fileWrite(&fid, writeBuff, &size)) {
            prError("Failed to OSAL_fileWrite(). \n");
            OSALUT_latchUnlock(&latch);
            return (UT_FAIL);
        }
        //OSAL_logMsg("Wrote2:%d\n", index);
        /*
         * XXX There will be write error after write bunch of data
         * w/o a short delay.
         */
        OSAL_taskDelay(1);
    }

    if (OSAL_SUCCESS != OSAL_fileClose(&fid)) {
        prError("Failed to OSAL_fileClose().\n");
        OSALUT_latchUnlock(&latch);
        return (UT_FAIL);
    }
    OSAL_logMsg("OSALUT_FifoWriteTask DONE!!!!\n");
    OSALUT_write2TaskStatus = 1;

    OSALUT_latchUnlock(&latch);
    return (UT_PASS);
}

/*
 * Fifo write3 task.
 */
static UT_Return OSALUT_FifoWrite3Task(
    void)
{
    OSAL_FileId        fid;
    vint               index;
    vint               size;
    char               writeBuff[OSALUT_FILE_TEST_BUF_SZ];
    OSAL_SelectSet     writeSet;
    OSAL_Boolean       isSet;
    OSAL_Boolean       isTimeout;
    OSAL_SelectTimeval tv;

    /* Wait one second. */
    tv.sec = 0;
    tv.usec = 1000;

    if (OSAL_SUCCESS != OSAL_fileOpen(&fid, OSALUT_FIFO_TEST_FIFO_NAME2,
            OSAL_FILE_O_RDWR, 0)) {
        prError("Failed to OSAL_fileOpen() with OSAL_FILE_O_RDWR. \n");
        OSALUT_latchUnlock(&latch);
        return (UT_FAIL);
    }

    size = OSALUT_FILE_TEST_BUF_SZ;
    for (index = 0; index < OSALUT_FIFO_TEST_MAX_MSG_SZ; index++) {
        OSAL_selectSetInit(&writeSet);
        OSAL_selectAddId(&fid, &writeSet);

        if (OSAL_FAIL == OSAL_select(NULL, &writeSet, &tv, &isTimeout)) {
            prError("Failed to select write fd.\n");
            OSALUT_latchUnlock(&latch);
            return (UT_FAIL);
        }

        if (isTimeout) {
            OSAL_logMsg("Write timeout\n");
            index--;
            /* Re-loop. */
            continue;
        } 

        if (OSAL_FAIL == OSAL_selectIsIdSet(&fid, &writeSet, &isSet)) {
            prError("OSAL_selectIsIdSet() failed\n");
            OSALUT_latchUnlock(&latch);
            return (UT_FAIL);
        }

        if (!isSet) {
            prError("Write fd is not set!!!\n");
            OSALUT_latchUnlock(&latch);
            return (UT_FAIL);
        }
        if (OSAL_SUCCESS != OSAL_fileWrite(&fid, writeBuff, &size)) {
            prError("Failed to OSAL_fileWrite(). \n");
            OSALUT_latchUnlock(&latch);
            return (UT_FAIL);
        }
        //OSAL_logMsg("Wrote3:%d\n", index);
        /*
         * XXX There will be write error after write bunch of data
         * w/o a short delay.
         */
        OSAL_taskDelay(1);
    }

    if (OSAL_SUCCESS != OSAL_fileClose(&fid)) {
        prError("Failed to OSAL_fileClose().\n");
        OSALUT_latchUnlock(&latch);
        return (UT_FAIL);
    }
    OSAL_logMsg("OSALUT_FifoWriteTask DONE!!!!\n");
    OSALUT_write3TaskStatus = 1;

    OSALUT_latchUnlock(&latch);
    return (UT_PASS);
}

/*
 * Fifo write 4 task for large message
 */
static UT_Return OSALUT_FifoWrite4Task(
    void)
{
    OSAL_FileId        fid;
    vint               index;
    vint               size;
    OSAL_SelectSet     writeSet;
    OSAL_Boolean       isSet;
    OSAL_Boolean       isTimeout;
    OSAL_SelectTimeval tv;

    /* Wait one second. */
    tv.sec = 0;
    tv.usec = 1000;

    if (OSAL_SUCCESS != OSAL_fileOpen(&fid, OSALUT_FIFO_TEST_FIFO_NAME,
            OSAL_FILE_O_RDWR, 0)) {
        prError("Failed to OSAL_fileOpen() with OSAL_FILE_O_RDWR. \n");
        OSALUT_latchUnlock(&latch);
        return (UT_FAIL);
    }

    size = OSALUT_FILE_TEST_LARGE_BUF_SZ;
    for (index = 0; index < OSALUT_FIFO_TEST_MAX_MSG_SZ; index++) {
        OSAL_selectSetInit(&writeSet);
        OSAL_selectAddId(&fid, &writeSet);

        if (OSAL_FAIL == OSAL_select(NULL, &writeSet, &tv, &isTimeout)) {
            prError("Failed to select write fd.\n");
            OSALUT_latchUnlock(&latch);
            return (UT_FAIL);
        }

        if (isTimeout) {
            OSAL_logMsg("Write timeout\n");
            index--;
            /* Re-loop. */
            continue;
        } 

        if (OSAL_FAIL == OSAL_selectIsIdSet(&fid, &writeSet, &isSet)) {
            prError("OSAL_selectIsIdSet() failed\n");
            OSALUT_latchUnlock(&latch);
            return (UT_FAIL);
        }

        if (!isSet) {
            prError("Write fd is not set!!!\n");
            OSALUT_latchUnlock(&latch);
            return (UT_FAIL);
        }
        if (OSAL_SUCCESS != OSAL_fileWrite(&fid, largeWriteBuff, &size)) {
            prError("Failed to OSAL_fileWrite(). \n");
            OSALUT_latchUnlock(&latch);
            return (UT_FAIL);
        }
        //OSAL_logMsg("Wrote:%d size:%d\n", index, size);
        /*
         * XXX There will be write error after write bunch of data
         * w/o a short delay.
         */
        OSAL_taskDelay(1);
    }

    if (OSAL_SUCCESS != OSAL_fileClose(&fid)) {
        prError("Failed to OSAL_fileClose().\n");
        OSALUT_latchUnlock(&latch);
        return (UT_FAIL);
    }
    OSAL_logMsg("OSALUT_FifoWriteTask DONE!!!!\n");
    OSALUT_write4TaskStatus = 1;

    OSALUT_latchUnlock(&latch);
    return (UT_PASS);
}

/*
 * Fifo read task.
 */
static UT_Return OSALUT_FifoReadTask(
    void)
{
    OSAL_FileId        fid;
    vint               size;
    vint               index;
    char               readBuff[OSALUT_FILE_TEST_BUF_SZ];
    OSAL_SelectSet     readSet;
    OSAL_Boolean       isSet;
    OSAL_Boolean       isTimeout;
    OSAL_SelectTimeval tv;

    /* Wait one second. */
    tv.sec = 0;
    tv.usec = 1000;
    index = 0;
    if (OSAL_SUCCESS != OSAL_fileOpen(&fid, OSALUT_FIFO_TEST_FIFO_NAME,
            OSAL_FILE_O_RDWR, 0)) {
        prError("Failed to OSAL_fileOpen() with OSAL_FILE_O_RDWR. \n");
        OSALUT_latchUnlock(&latch);
        return (UT_FAIL);
    }

_OSALUT_FIFO_READ_LOOP:
    OSAL_selectSetInit(&readSet);
    OSAL_selectAddId(&fid, &readSet);

    if (OSAL_FAIL == OSAL_select(&readSet, NULL, &tv, &isTimeout)) {
        prError("Failed to select read fd.\n");
        OSALUT_latchUnlock(&latch);
        return (UT_FAIL);
    }

    if (isTimeout) {
        OSAL_logMsg("Read timeout\n");
        goto _OSALUT_FIFO_READ_LOOP;
    } 

    if (OSAL_FAIL == OSAL_selectIsIdSet(&fid, &readSet, &isSet)) {
        prError("OSAL_selectIsIdSet() failed\n");
        OSALUT_latchUnlock(&latch);
        return (UT_FAIL);
    }

    if (!isSet) {
        prError("Read fd is not set!!!\n");
        OSALUT_latchUnlock(&latch);
        return (UT_FAIL);
    }

    size = OSALUT_FILE_TEST_BUF_SZ;
    if (OSAL_SUCCESS != OSAL_fileRead(&fid, readBuff, &size)) {
        prError("Failed to OSAL_fileRead(). \n");
        OSALUT_latchUnlock(&latch);
        return (UT_FAIL);
    }

    index++;
//    OSAL_logMsg("read %d\n", index);
    if (index >= OSALUT_FIFO_TEST_MAX_MSG_SZ) {
        goto _OSALUT_FIFO_EXIT_READ_LOOP;
    }
    goto _OSALUT_FIFO_READ_LOOP;

_OSALUT_FIFO_EXIT_READ_LOOP:
    if (OSAL_SUCCESS != OSAL_fileClose(&fid)) {
        prError("Failed to OSAL_fileClose().\n");
        return (UT_FAIL);
    }
    OSAL_logMsg("OSALUT_FifoReadTask DONE!!!!\n");
    OSALUT_readTaskStatus = 1;
    OSALUT_latchUnlock(&latch);
    return (UT_PASS);
}

/*
 * Fifo read2 task.
 */
static UT_Return OSALUT_FifoRead2Task(
    void)
{
    OSAL_FileId        fid;
    OSAL_FileId        fid2;
    vint               size;
    vint               index;
    char               readBuff[OSALUT_FILE_TEST_BUF_SZ];
    OSAL_SelectSet     readSet;
    OSAL_Boolean       isSet;
    OSAL_Boolean       isTimeout;
    OSAL_SelectTimeval tv;

    /* Wait one second. */
    tv.sec = 0;
    tv.usec = 1000;
    index = 0;
    if (OSAL_SUCCESS != OSAL_fileOpen(&fid, OSALUT_FIFO_TEST_FIFO_NAME,
            OSAL_FILE_O_RDWR, 0)) {
        prError("Failed to OSAL_fileOpen() with OSAL_FILE_O_RDWR. \n");
        OSALUT_latchUnlock(&latch);
        return (UT_FAIL);
    }
    if (OSAL_SUCCESS != OSAL_fileOpen(&fid2, OSALUT_FIFO_TEST_FIFO_NAME2,
            OSAL_FILE_O_RDWR, 0)) {
        prError("Failed to OSAL_fileOpen() with OSAL_FILE_O_RDWR. \n");
        OSALUT_latchUnlock(&latch);
        return (UT_FAIL);
    }

_OSALUT_FIFO_READ_LOOP:
    OSAL_selectSetInit(&readSet);
    OSAL_selectAddId(&fid, &readSet);
    OSAL_selectAddId(&fid2, &readSet);

    if (OSAL_FAIL == OSAL_select(&readSet, NULL, &tv, &isTimeout)) {
        prError("Failed to select read fd.\n");
        OSALUT_latchUnlock(&latch);
        return (UT_FAIL);
    }

    if (isTimeout) {
        OSAL_logMsg("Read timeout\n");
        goto _OSALUT_FIFO_READ_LOOP;
    } 

    /* Check first fid. */
    if (OSAL_FAIL == OSAL_selectIsIdSet(&fid, &readSet, &isSet)) {
        prError("OSAL_selectIsIdSet() failed\n");
        OSALUT_latchUnlock(&latch);
        return (UT_FAIL);
    }

    if (isSet) {
        size = OSALUT_FILE_TEST_BUF_SZ;
        if (OSAL_SUCCESS != OSAL_fileRead(&fid, readBuff, &size)) {
            prError("Failed to OSAL_fileRead(). \n");
            OSALUT_latchUnlock(&latch);
            return (UT_FAIL);
        }
        index++;
        //OSAL_logMsg("read %d\n", index);
    }

    /* Check second fid. */
    if (OSAL_FAIL == OSAL_selectIsIdSet(&fid2, &readSet, &isSet)) {
        prError("OSAL_selectIsIdSet() failed\n");
        OSALUT_latchUnlock(&latch);
        return (UT_FAIL);
    }

    if (isSet) {
        size = OSALUT_FILE_TEST_BUF_SZ;
        if (OSAL_SUCCESS != OSAL_fileRead(&fid2, readBuff, &size)) {
            prError("Failed to OSAL_fileRead(). \n");
            OSALUT_latchUnlock(&latch);
            return (UT_FAIL);
        }
        index++;
        //OSAL_logMsg("read2 %d\n", index);
    }

    if (index >= (2 * OSALUT_FIFO_TEST_MAX_MSG_SZ)) {
        goto _OSALUT_FIFO_EXIT_READ_LOOP;
    }
    goto _OSALUT_FIFO_READ_LOOP;

_OSALUT_FIFO_EXIT_READ_LOOP:
    if (OSAL_SUCCESS != OSAL_fileClose(&fid)) {
        prError("Failed to OSAL_fileClose().\n");
        return (UT_FAIL);
    }
    if (OSAL_SUCCESS != OSAL_fileClose(&fid2)) {
        prError("Failed to OSAL_fileClose().\n");
        return (UT_FAIL);
    }
    OSAL_logMsg("OSALUT_FifoRead2Task DONE!!!!\n");
    OSALUT_read2TaskStatus = 1;
    OSALUT_latchUnlock(&latch);
    return (UT_PASS);
}

/*
 * Fifo read3 task for large message
 */
static UT_Return OSALUT_FifoRead3Task(
    void)
{
    OSAL_FileId        fid;
    vint               size;
    vint               index;
    //char               readBuff[OSALUT_FILE_TEST_LARGE_BUF_SZ];
    OSAL_SelectSet     readSet;
    OSAL_Boolean       isSet;
    OSAL_Boolean       isTimeout;
    OSAL_SelectTimeval tv;

    /* Wait one second. */
    tv.sec = 0;
    tv.usec = 1000;
    index = 0;
    if (OSAL_SUCCESS != OSAL_fileOpen(&fid, OSALUT_FIFO_TEST_FIFO_NAME,
            OSAL_FILE_O_RDWR, 0)) {
        prError("Failed to OSAL_fileOpen() with OSAL_FILE_O_RDWR. \n");
        OSALUT_latchUnlock(&latch);
        return (UT_FAIL);
    }

_OSALUT_FIFO_READ_LOOP:
    OSAL_selectSetInit(&readSet);
    OSAL_selectAddId(&fid, &readSet);

    if (OSAL_FAIL == OSAL_select(&readSet, NULL, &tv, &isTimeout)) {
        prError("Failed to select read fd.\n");
        OSALUT_latchUnlock(&latch);
        return (UT_FAIL);
    }

    if (isTimeout) {
        OSAL_logMsg("Read timeout\n");
        goto _OSALUT_FIFO_READ_LOOP;
    } 

    if (OSAL_FAIL == OSAL_selectIsIdSet(&fid, &readSet, &isSet)) {
        prError("OSAL_selectIsIdSet() failed\n");
        OSALUT_latchUnlock(&latch);
        return (UT_FAIL);
    }

    if (!isSet) {
        prError("Read fd is not set!!!\n");
        OSALUT_latchUnlock(&latch);
        return (UT_FAIL);
    }
    
    size = OSALUT_FILE_TEST_LARGE_BUF_SZ;
    if (OSAL_SUCCESS != OSAL_fileRead(&fid, largeReadBuff, &size)) {
        prError("Failed to OSAL_fileRead(). \n");
        OSALUT_latchUnlock(&latch);
        return (UT_FAIL);
    }
    index++;
    //OSAL_logMsg("read %d size:%d\n", index, size);
    if (index >= OSALUT_FIFO_TEST_MAX_MSG_SZ) {
        goto _OSALUT_FIFO_EXIT_READ_LOOP;
    }
    goto _OSALUT_FIFO_READ_LOOP;

_OSALUT_FIFO_EXIT_READ_LOOP:
    if (OSAL_SUCCESS != OSAL_fileClose(&fid)) {
        prError("Failed to OSAL_fileClose().\n");
        return (UT_FAIL);
    }
    OSAL_logMsg("OSALUT_FifoRead3Task DONE!!!!\n");
    OSALUT_read3TaskStatus = 1;
    OSALUT_latchUnlock(&latch);
    return (UT_PASS);
}

/*
 * ========= do_test_fifo() ========
 * Gen unit test vectors for each OSAL fifo function
 *
 * Return:
 *  UT_PASS: Exit normally.
 *  UT_FAIL: Invalid arguments.
 */
UT_Return do_test_fifo(
    void)
{
    OSAL_TaskId writeTaskId, readTaskId;
    OSAL_TaskId write2TaskId, write3TaskId, read2TaskId;

    /*
     * FIFO test 1:
     * FIFO create test. NULL FIFO name, the result should be failed.
     */
    OSAL_logMsg("FIFO test 1\n");
    if (OSAL_FAIL != OSAL_fileFifoCreate(NULL, OSALUT_FIFO_TEST_FIFO_MSG_SZ,
            OSALUT_FILE_TEST_BUF_SZ)) {
        prError("Unexpected OSAL_fileFifoCreate() retval - expected FAIL.\n");
    }

    /*
     * FIFO test 2:
     * FIFO create test successfully.
     */
    OSAL_logMsg("FIFO test 2\n");
    if (OSAL_SUCCESS != OSAL_fileFifoCreate(OSALUT_FIFO_TEST_FIFO_NAME,
            OSALUT_FIFO_TEST_FIFO_MSG_SZ, OSALUT_FILE_TEST_BUF_SZ)) {
        prError("Unexpected OSAL_fileFifoCreate() retval - expected SUCCESS.\n");
    }

    /*
     * FIFO test 3:
     * FIFO delete test. NULL FIFO name, the result should be failed.
     */
    OSAL_logMsg("FIFO test 3\n");
    if (OSAL_FAIL != OSAL_fileFifoDelete(NULL)) {
        prError("Unexpected OSAL_fileFifoDelete() retval - expected FAIL.\n");
    }

    /*
     * FIFO test 4:
     * FIFO delete test successfully.
     */
    OSAL_logMsg("FIFO test 4\n");
    if (OSAL_SUCCESS != OSAL_fileFifoDelete(OSALUT_FIFO_TEST_FIFO_NAME)) {
        prError("Unexpected OSAL_fileFifoDelete() retval - expected SUCCESS.\n");
    }
    /*
     * FIFO test 5:
     * FIFO read/write
     */
    OSAL_logMsg("FIFO test 5\n");
    OSALUT_latchInit(&latch, 2);

    if (OSAL_SUCCESS != OSAL_fileFifoCreate(OSALUT_FIFO_TEST_FIFO_NAME,
            OSALUT_FIFO_TEST_FIFO_MSG_SZ, OSALUT_FILE_TEST_BUF_SZ)) {
        prError("Unexpected OSAL_fileFifoCreate() retval - expected SUCCESS.\n");
    }
    readTaskId = OSAL_taskCreate("readTask", OSAL_TASK_PRIO_NIC,
            (uint32)1024*4, (OSAL_TaskPtr)OSALUT_FifoReadTask, NULL);

    writeTaskId = OSAL_taskCreate("writeTask", OSAL_TASK_PRIO_NIC,
            (uint32)1024*4, (OSAL_TaskPtr)OSALUT_FifoWriteTask, NULL);

    /* Wait for all tasks are done. */
    OSALUT_latchWait(&latch);
    OSALUT_latchDestroy(&latch);

    OSAL_taskDelete(writeTaskId);
    OSAL_taskDelete(readTaskId);

    if ((1 != OSALUT_readTaskStatus) || (1 != OSALUT_writeTaskStatus)) {
        prError("Unexpected FIFO read/write task result.\n");
    }

    if (OSAL_SUCCESS != OSAL_fileFifoDelete(OSALUT_FIFO_TEST_FIFO_NAME)) {
        prError("Unexpected OSAL_fileFifoDelete() retval - expected SUCCESS.\n");
    }

    /*
     * FIFO test 6:
     * Multiple FIFO read/write
     */
    OSAL_logMsg("FIFO test 6\n");
    OSALUT_latchInit(&latch, 3);

    if (OSAL_SUCCESS != OSAL_fileFifoCreate(OSALUT_FIFO_TEST_FIFO_NAME,
            OSALUT_FIFO_TEST_FIFO_MSG_SZ, OSALUT_FILE_TEST_BUF_SZ)) {
        prError("Unexpected OSAL_fileFifoCreate() retval - expected SUCCESS.\n");
    }
    if (OSAL_SUCCESS != OSAL_fileFifoCreate(OSALUT_FIFO_TEST_FIFO_NAME2,
            OSALUT_FIFO_TEST_FIFO_MSG_SZ, OSALUT_FILE_TEST_BUF_SZ)) {
        prError("Unexpected OSAL_fileFifoCreate() retval - expected SUCCESS.\n");
    }
    read2TaskId = OSAL_taskCreate("read2Task", OSAL_TASK_PRIO_NIC,
            (uint32)1024*4, (OSAL_TaskPtr)OSALUT_FifoRead2Task, NULL);

    write2TaskId = OSAL_taskCreate("write2Task", OSAL_TASK_PRIO_NIC,
            (uint32)1024*4, (OSAL_TaskPtr)OSALUT_FifoWrite2Task, NULL);

    write3TaskId = OSAL_taskCreate("write3Task", OSAL_TASK_PRIO_NIC,
            (uint32)1024*4, (OSAL_TaskPtr)OSALUT_FifoWrite3Task, NULL);

    /* Wait for all tasks are done. */
    OSALUT_latchWait(&latch);
    OSALUT_latchDestroy(&latch);

    OSAL_taskDelete(write2TaskId);
    OSAL_taskDelete(write3TaskId);
    OSAL_taskDelete(read2TaskId);

    if ((1 != OSALUT_read2TaskStatus) || (1 != OSALUT_write2TaskStatus) ||
            (1 != OSALUT_write3TaskStatus)) {
        prError("Unexpected FIFO read/write task result.\n");
    }

    if (OSAL_SUCCESS != OSAL_fileFifoDelete(OSALUT_FIFO_TEST_FIFO_NAME)) {
        prError("Unexpected OSAL_fileFifoDelete() retval - expected SUCCESS.\n");
    }

    if (OSAL_SUCCESS != OSAL_fileFifoDelete(OSALUT_FIFO_TEST_FIFO_NAME2)) {
        prError("Unexpected OSAL_fileFifoDelete() retval - expected SUCCESS.\n");
    }

    /*
     * FIFO test 6:
     * FIFO read/write large message.
     */
    OSAL_logMsg("FIFO test 6\n");
    OSALUT_latchInit(&latch, 2);

    if (OSAL_SUCCESS != OSAL_fileFifoCreate(OSALUT_FIFO_TEST_FIFO_NAME,
            OSALUT_FIFO_TEST_FIFO_MSG_SZ, OSALUT_FILE_TEST_LARGE_BUF_SZ)) {
        prError("Unexpected OSAL_fileFifoCreate() retval - expected SUCCESS.\n");
    }
    readTaskId = OSAL_taskCreate("read3Task", OSAL_TASK_PRIO_NIC,
            (uint32)1024*4, (OSAL_TaskPtr)OSALUT_FifoRead3Task, NULL);

    writeTaskId = OSAL_taskCreate("write4Task", OSAL_TASK_PRIO_NIC,
            (uint32)1024*4, (OSAL_TaskPtr)OSALUT_FifoWrite4Task, NULL);

    /* Wait for all tasks are done. */
    OSALUT_latchWait(&latch);
    OSALUT_latchDestroy(&latch);

    OSAL_taskDelete(writeTaskId);
    OSAL_taskDelete(readTaskId);

    if ((1 != OSALUT_read3TaskStatus) || (1 != OSALUT_write4TaskStatus)) {
        prError("Unexpected FIFO read/write task result.\n");
    }

    if (OSAL_SUCCESS != OSAL_fileFifoDelete(OSALUT_FIFO_TEST_FIFO_NAME)) {
        prError("Unexpected OSAL_fileFifoDelete() retval - expected SUCCESS.\n");
    }

    /* Done. */
    if (0 == osal_utErrorsFound) {
        OSAL_logMsg("Fifo Unit Test Completed Successfully.\n");
        return (UT_PASS);
    }
    else {
        OSAL_logMsg("Fifo Unit Test Completed with %d errors.\n",
                osal_utErrorsFound);
        return (UT_FAIL);
    }
}

