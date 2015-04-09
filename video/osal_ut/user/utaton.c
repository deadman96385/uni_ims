/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004-2007 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 8688 $ $Date: 2009-01-10 09:34:42 +0800 (Sat, 10 Jan 2009) $
 *
 */
#include "osal_ut.h"

/*
 * ========= do_test_sem() ========
 * Gen unit test vectors for each OSAL sem function
 *
 * Return:
 *  UT_PASS: Exit normally.
 */
UT_Return do_test_aton(
    void)
{
#if 0
    OSAL_NetAddress  addrNbo;
    char             buf1[100];
    char             buf2[100];
    unsigned int    *byte_ptr;
    char            *a1_ptr;

    struct sockaddr_in sockaddrIn1;

    /* Test the "aton" */
    OSAL_logMsg("\n\n=== Testing 'aton'\n");

    /* Start with network address 1.2.3.4 */
    sOSAL_logMsg(buf1, "1.2.3.4");
    OSAL_netStringToAddress(buf1, &addrNbo);
    byte_ptr = (void *)&addrNbo.ipv4;
    /* addrNbo now should have network byte order of "1.2.3.4" */
    OSAL_logMsg("OSAL: '%s' = %x = %d.%d.%d.%d\n", buf1, addrNbo.ipv4,
          byte_ptr[0],
          byte_ptr[1],
          byte_ptr[2],
          byte_ptr[3]);

    /* Test using aton and socket interface */
    memset((void *)&sockaddrIn1, 0, sizeof(sockaddrIn1));
    sockaddrIn1.sin_family = AF_INET;         // host byte order
    inet_aton(buf1, &(sockaddrIn1.sin_addr));
    byte_ptr = (void *)&(sockaddrIn1.sin_addr);
    OSAL_logMsg("SOCKET.h: '%s' = %x = %d.%d.%d.%d\n",
            buf1, *(unsigned int *) &(sockaddrIn1.sin_addr),
            byte_ptr[0],
            byte_ptr[1],
            byte_ptr[2],
            byte_ptr[3]);
    /* Test using inet_addr and socket interface */
    memset((void *)&sockaddrIn1, 0, sizeof(sockaddrIn1));
    sockaddrIn1.sin_addr.s_addr = inet_addr(buf1);
    OSAL_logMsg("SOCKET.h: '%s' = %x = %d.%d.%d.%d\n",
            buf1, *(unsigned int *) &(sockaddrIn1.sin_addr),
            byte_ptr[0],
            byte_ptr[1],
            byte_ptr[2],
            byte_ptr[3]);
    OSAL_logMsg("\n--> Verify above results are the same.\n\n");


    /* Test the "ntoa" */
    OSAL_logMsg("\n\n=== Testing 'ntoa'\n");

    /* Convert previous result back to string */
    OSAL_netAddressToString(buf2, &addrNbo);
    OSAL_logMsg("OSAL: %x = '%s'\n", addrNbo.ipv4, buf2);

    /* Test using inet_ntoa and socket interface */
    a1_ptr = inet_ntoa(sockaddrIn1.sin_addr);
    OSAL_logMsg("SOCKET.h: %x = '%s'\n", *(unsigned int *) &(sockaddrIn1.sin_addr),
            a1_ptr);
    OSAL_logMsg("\n--> Verify above results are the same.\n\n");
#endif
    return (UT_PASS);
}

