/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 8101 $ $Date: 2008-11-19 08:16:11 +0800 (Wed, 19 Nov 2008) $
 *
 * Description: OSAL unit test for memory features. Not much to do here
 * since we're a pass thru at the OSAL level...
 *
 */

#include "osal_ut.h"

 /*
 * ========= do_test_mem() ========
 * Gen unit test vectors for each OSAL memory function
 *
 * Return:
 *   UT_PASS: success.
 *   UT_FAIL: fail.
 */
UT_Return do_test_mem(
    void)
{
    char          **allocBuf, **allocBuf2;
    register int    i;

    OSAL_logMsg("Memory Unit Test Starting...\n");

    /* reset this before every test */
    osal_utErrorsFound = 0;

    allocBuf = (char **) OSAL_memAlloc((int) 1024, OSAL_MEM_ARG_STATIC_ALLOC);
    if ((char **) NULL == allocBuf) {
        prError("Unexpected OSAL_memAalloc() failure.\n");
    }

    if (OSAL_SUCCESS != OSAL_memFree((void *) allocBuf,
            OSAL_MEM_ARG_STATIC_ALLOC)) {
        prError("Unexpected OSAL_memFree() failure.\n");
    }

    /*dynamic memory alloc*/
    allocBuf = (char **) OSAL_memAlloc((int) 64,
            (int) OSAL_MEM_ARG_DYNAMIC_ALLOC);
    if ((char **) NULL == allocBuf) {
        prError("Unexpected OSAL_memAalloc() with dynamic failure.\n");
    }
         OSAL_logMsg("%s %d\n", __FILE__, __LINE__);
     /*dynamic memory alloc*/
    allocBuf2 = (char **) OSAL_memAlloc((int) 1024,
            (int) OSAL_MEM_ARG_DYNAMIC_ALLOC);
    if ((char **) NULL == allocBuf2) {
        prError("Unexpected OSAL_memAalloc() with dynamic failure.\n");
    }
    OSAL_memSet((void *)allocBuf, 7, 64);

    OSAL_memMove((void *)allocBuf2, (void *)allocBuf, 64);

    if (OSAL_SUCCESS != OSAL_memFree((void *) allocBuf2,
                OSAL_MEM_ARG_DYNAMIC_ALLOC)) {
        prError("Unexpected OSAL_memFree() with dynamic failure.\n");
    }

    if (OSAL_SUCCESS != OSAL_memFree((void *) allocBuf,
                OSAL_MEM_ARG_DYNAMIC_ALLOC)) {
        prError("Unexpected OSAL_memFree() with dynamic failure.\n");
    }

    allocBuf = (char **) OSAL_memCalloc((int) 1024, (int) sizeof(char *),
            (int) 0);

    if (NULL == allocBuf) {
        prError("Unexpected OSAL_memCalloc() failure.\n");
    }

    for (i=0;i<1024;i++) {
        allocBuf[i] = (char *) OSAL_memCalloc((int) 1024, (int) sizeof(char *),
                (int) 0);
        if (NULL == allocBuf[i]) {
            prError1("Unexpected OSAL_memAlloc() failure at alloc %d.\n", i+1);
            break;
        }
    }

    if (0 != OSAL_memCmp(allocBuf[0], allocBuf[1], 1024)) {
        prError("Unexpected OSAL_memCmp() failure.\n");
    }

    OSAL_memSet(allocBuf[0], 3, 512);

    OSAL_memCpy(allocBuf[1], allocBuf[0], 1024);

    if (0 != OSAL_memCmp(allocBuf[0], allocBuf[1], 1024)) {
        prError("Unexpected OSAL_memSet(), OSAL_memCpy(), or OSAL_memCmp() "
                "failure.\n");
    }

    for (i=0;i<1024;i++) {
        OSAL_memFree(allocBuf[i], (int) 0);
    }

    if (OSAL_SUCCESS != OSAL_memFree((void *) allocBuf,0)) {
        prError("Unexpected OSAL_memFree() failure.\n");
    }

    if (0 == osal_utErrorsFound) {
        OSAL_logMsg("Memory Unit Test Completed Successfully.\n");
        return (UT_PASS);
    }
    else {
        OSAL_logMsg("Memory Unit Test Completed with %d errors.\n",
            osal_utErrorsFound);
        return (UT_FAIL);
    }
}
