/* 
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL  
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE  
 * APPLIES: "COPYRIGHT 2004-2006 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"  
 *  
 * $D2Tech$ $Rev: 30359 $ $Date: 2014-12-11 16:39:45 +0800 (Thu, 11 Dec 2014) $
 */

/*
 * This file is included by D2 portable code which make use of the D2 portable
 * Operating System Abstraction Layer (OSAL) API
 * 
 * OSAL implements a portable abstraction layer for each RTOS that D2 portable 
 * code uses for system services.
 */

#ifndef __OSAL_PLATFORM_H__
#define __OSAL_PLATFORM_H__

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <sci_types.h>
#include <socket_api.h>
#include <osa_interface.h>
#include <dal_time.h>
#include <ims_stack_file_code.h>

#ifdef OSAL_NET_ENABLE_SSL
#include <ssl_api.h>
#endif
#ifdef OSAL_IPSEC_ENABLE
#include <ipsec_api.h>
#endif

/*
 * Exactly one of the following must be defined
 * OSAL_VXWORKS 
 * OSAL_LKM  (Linux LKM)
 * OSAL_PTHREADS  
 * OSAL_NUCLEUS  
 * OSAL_THREADX
 */

#define OSAL_IPC_FOLDER       "/pipe/"

/*
 * OS specific type defines
 */
typedef int         OSAL_Fid;
typedef int         OSAL_SelectId;
typedef int         OSAL_NetSockId;
typedef int         OSAL_FileId;
typedef sci_fd_set  OSAL_SelectSet;

#ifdef OSAL_NET_ENABLE_SSL
/*
 * Certification for SSL
 */
typedef struct {
    SSL_CERTINFO_T     *certInfo;
} OSAL_NetSslCert;

typedef struct {
    SSL_HANDLE      *ssl_ptr;
    OSAL_NetSslCert *cert_ptr;  /* Pointer to OSAL_NetSslCert */
    OSAL_NetSockId  sockId;
    OSAL_FileId     fid;
    uint16          port;
    char           *sslFifo_ptr;
    char            ipStr[46];
} OSAL_NetSslId;
#else
typedef struct {
    int *x509_ptr;
    int *pKey_ptr;
} OSAL_NetSslCert;

typedef struct {
    int *ssl_ptr;
    int *ctx_ptr;
    OSAL_NetSslCert *cert_ptr;
    int            (*certValidateCB)(int, void *);
} OSAL_NetSslId;
#endif

typedef enum {
    OSAL_TASK_PRIO_VTSPR = OSA_PRIORITY_NINE,
    OSAL_TASK_PRIO_DEC20 = OSA_PRIORITY_ELEVEN, /* for task ut. */
    OSAL_TASK_PRIO_NIC   = OSA_PRIORITY_THIRTEEN,
    OSAL_TASK_PRIO_NRT   = OSA_LOWEST_PRIORITY,
} OSAL_TaskPrio;

typedef enum {
    OSAL_SEMB_UNAVAILABLE = 0,
    OSAL_SEMB_AVAILABLE   = 1
} OSAL_SemBState;

#define OSAL_SCHED_FIFO_PRIO_MAX    (255)
#define OSAL_SCHED_FIFO_PRIO_MIN    (0)

#define OSAL_INLINE           
#define OSAL_STACK_SZ_LARGE   (1024 * 4)
#define OSAL_STACK_SZ_SMALL   (1000)

#define OSAL_NET_INADDR_ANY        (0)
#define OSAL_NET_IN6ADDR_ANY       (0)
#define OSAL_NET_INADDR_LOOPBACK   ((unsigned long int) 0x7f000001)

#define OSAL_NET_SOCK_INVALID_ID    (TCPIP_SOCKET_INVALID)

#define OSAL_ENTRY\
    int main(int argc, char *argv_ptr[])\
    {\
        OSAL_logMsg(\
                "\n\n"\
                "    ====================================== \n"\
                "                                           \n"\
                "               D2 Technologies             \n"\
                "         IP Communication Products         \n"\
                "                www.d2tech.com             \n\n"\
                "    Starting %s ...                        \n\n",\
                argv_ptr[0]);
#define OSAL_EXIT\
    }


#endif /* __OSAL_PLATFORM_H__ */
