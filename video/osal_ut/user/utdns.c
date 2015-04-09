/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 8688 $ $Date: 2009-01-10 09:34:42 +0800 (Sat, 10 Jan 2009) $
 */

#include "osal_ut.h"

/*
 * ========= do_test_sem() ========
 * Gen unit test vectors for each OSAL dns function
 *
 * Return:
 *  UT_PASS: Exit normally.
 */
UT_Return do_test_dns(
    void)
{
    int8            ans[512] = "";
    OSAL_NetAddress addr;

    /*
     * Make query, DNS A records.
     * Timeout is 2 seconds.
     * 4 retries on timeout.
     */
    addr.type = OSAL_NET_SOCK_UDP;
    if (OSAL_SUCCESS == OSAL_netGetHostByName("www.google.com",
            ans, sizeof(ans), &addr)) {
        OSAL_logMsg("www.google.com is resolved to:%s\n", ans);
    }
    else {
        return (UT_FAIL);
    }
    /*
     * Make query, DNS SRV records.
     * Timeout is 2 seconds.
     * 4 retries on timeout.
     */
#ifndef OSAL_RESOLVE_A_ONLY
    if (OSAL_SUCCESS == OSAL_netResolve("_xmpp-client._tcp.gmail.com",
            ans, sizeof(ans), 2, 4, OSAL_NET_RESOLVE_SRV)) {
        OSAL_logMsg("_xmpp-client._tcp.gmail.com has SRV entries:\n%s\n", ans);
    }
    else {
        return (UT_FAIL);
    }
#endif

    /* Delay for waiting osal_ut main function to pthred_cond_wait() */
    OSAL_taskDelay(1000);
    return (UT_PASS);
}
