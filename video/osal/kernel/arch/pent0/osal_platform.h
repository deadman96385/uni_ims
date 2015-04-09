/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004-2009 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 30289 $ $Date: 2014-12-09 15:50:00 +0800 (Tue, 09 Dec 2014) $
 */

#ifndef _OSAL_PLATFORM_H_
#define _OSAL_PLATFORM_H_

//unsigned extern const char D2_Release_VPORT[];

/*
 * Exactly one of the following must be defined
 */

#if ( \
        defined(OSAL_PTHREADS) \
        != 1)

#if defined(OSAL_PTHREADS)
#warning "OSAL_PTHREADS is defined, ..."
#endif

#error "Define proper OSAL abstraction, and choose only one"

#endif

/*
 * Set up definitions and includes
 * depending on the OSAL define
 * to minimize build rules.
 */

#define OSAL_IPC_FOLDER       "/var/tmp/osal/"
#define OSAL_INLINE           inline
#define OSAL_STACK_SZ_LARGE   (8192)
#define OSAL_STACK_SZ_SMALL   (1000)

/* Define message struct id. */
#define OSAL_MSG_STRUCT_CSMOUTPUTEVT    (1)

#if defined(OSAL_PTHREADS)

#include <pthread.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <time.h>
#include <semaphore.h>
#include <sched.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <arpa/nameser.h>
#include <netdb.h>
#ifdef OSAL_NET_ENABLE_SSL
#include <openssl/ssl.h>
#include <openssl/x509v3.h>
#endif
#include <linux/ipsec.h>
#include <stdarg.h>

/*
 * OS specific type defines
 */
typedef int         OSAL_Fid;
typedef int         OSAL_SelectId;
typedef int         OSAL_NetSockId;
typedef int         OSAL_FileId;
typedef fd_set      OSAL_SelectSet;

#ifdef OSAL_NET_ENABLE_SSL
/*
 * Certification for SSL
 */
typedef struct {
    X509     *x509_ptr;
    EVP_PKEY *pKey_ptr;     /* Private key*/
} OSAL_NetSslCert;

typedef struct {
    SSL             *ssl_ptr;
    SSL_CTX         *ctx_ptr;
    OSAL_NetSslCert *cert_ptr;
    int            (*certValidateCB)(int, X509_STORE_CTX *);
} OSAL_NetSslId;

#else
typedef struct {
    int *x509_ptr;
    int *pKey_ptr;     /* Private key*/
} OSAL_NetSslCert;

typedef struct {
    int *ssl_ptr;
    int *ctx_ptr;
    OSAL_NetSslCert *cert_ptr;
    int            (*certValidateCB)(int, void *);
} OSAL_NetSslId;

#endif

#define OSAL_NET_INADDR_ANY        (INADDR_ANY)
#define OSAL_NET_IN6ADDR_ANY       (IN6ADDR_ANY_INIT)
#define OSAL_NET_INADDR_LOOPBACK   (INADDR_LOOPBACK)

#define OSAL_NET_SOCK_INVALID_ID    (-1)

typedef enum {
    OSAL_TASK_PRIO_VTSPR = 90,
    OSAL_TASK_PRIO_ENC20 = 85,
    OSAL_TASK_PRIO_ENC30 = 80,
    OSAL_TASK_PRIO_DEC20 = 79,
    OSAL_TASK_PRIO_VENC  = 60,
    OSAL_TASK_PRIO_VDEC  = 61,
    OSAL_TASK_PRIO_NIC   = 0,
    OSAL_TASK_PRIO_NRT   = 0
} OSAL_TaskPrio;

typedef enum {
    OSAL_SEMB_UNAVAILABLE = 0,
    OSAL_SEMB_AVAILABLE   = 1
} OSAL_SemBState;

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
                "    %s                                     \n\n"\
                "    Starting %s ...                        \n\n",\
                D2_VPORT_REVISION,\
                argv_ptr[0]);
#define OSAL_EXIT\
    }

#endif /* OSAL_PTHREADS */

#endif /* _OSAL_PLATFORM_H_ */
