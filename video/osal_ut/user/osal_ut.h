/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL  
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE  
 * APPLIES: "COPYRIGHT 2004-2007 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"  
 *  
 * $D2Tech$ $Rev: 11815 $ $Date: 2010-04-16 06:06:18 +0800 (Fri, 16 Apr 2010) $
 * 
 */

#ifndef __OSAL_UT_H__
#define __OSAL_UT_H__

#include "osal.h"

typedef enum {
    UT_FAIL = 0,
    UT_PASS = 1
} UT_Return;

extern int osal_utErrorsFound;

/* Macros and constants... */
#define prError(msg) \
   do { OSAL_logMsg("%s:%d - ",__FILE__,(unsigned)__LINE__); \
        OSAL_logMsg(msg); \
        osal_utErrorsFound++; } while (0)

#define prError1(msg, arg) \
   do { OSAL_logMsg("%s:%d - ",__FILE__,(unsigned)__LINE__); \
        OSAL_logMsg(msg,arg); \
        osal_utErrorsFound++; } while (0)

#define prError2(msg, arg1, arg2) \
   do { OSAL_logMsg("%s:%d - ",__FILE__,(unsigned)__LINE__); \
        OSAL_logMsg(msg,arg1,arg2); \
        osal_utErrorsFound++; } while (0)

#define prError3(msg, arg1, arg2, arg3) \
   do { OSAL_logMsg("%s:%d - ",__FILE__,(unsigned)__LINE__); \
        OSAL_logMsg(msg,arg1,arg2,arg3); \
        osal_utErrorsFound++; } while (0)

#define prError4(msg, arg1, arg2, arg3, arg4) \
   do { OSAL_logMsg("%s:%d - ",__FILE__,(unsigned)__LINE__); \
        OSAL_logMsg(msg,arg1,arg2,arg3,arg4); \
        osal_utErrorsFound++; } while (0)

UT_Return do_test_mem(void);
UT_Return do_test_task(void);
UT_Return do_test_stresstask(void);
UT_Return do_test_msg(void);
UT_Return do_test_timer(void);
UT_Return do_test_sem(void);
UT_Return do_test_dns(void);
UT_Return do_test_aton(void);
UT_Return do_test_ipsec(void);
UT_Return do_test_crypto(void);
UT_Return do_test_file(void);
UT_Return do_test_fifo(void);
UT_Return do_test_net(void);
UT_Return do_test_ssl(void);

#endif /* __OSAL_UT_H__ */

