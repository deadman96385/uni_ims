/**
 * Copyright (C) 2016 Spreadtrum Communications Inc.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/un.h>
#ifdef ANDROID
#include <cutils/sockets.h>
#else
#include <sys/socket.h>
#endif
#include <time.h>

#define UNUSED(x) (void)(x) /* avoid compiler warning */

#define YLOGD_SOCKET_FILE "/dev/socket/ylog/ylogdw"

/* Used to retry syscalls that can return EINTR. */
#define TEMP_FAILURE_RETRY(exp) ({         \
    __typeof__(exp) _rc;                   \
    do {                                   \
        _rc = (exp);                       \
    } while (_rc == -1 && errno == EINTR); \
    _rc; })

static int __write_to_ylog_initialize(void) {
    int i, ret = 0;
    i = TEMP_FAILURE_RETRY(socket(PF_UNIX, SOCK_DGRAM | SOCK_CLOEXEC, 0));
    if (i < 0) {
        ret = -errno;
    } else if (TEMP_FAILURE_RETRY(fcntl(i, F_SETFL, O_NONBLOCK)) < 0) {
        ret = -errno;
        close(i);
    } else {
        struct sockaddr_un un;
        memset(&un, 0, sizeof(struct sockaddr_un));
        un.sun_family = AF_UNIX;
        strcpy(un.sun_path, YLOGD_SOCKET_FILE);

        if (TEMP_FAILURE_RETRY(connect(i, (struct sockaddr *)&un,
                        sizeof(struct sockaddr_un))) < 0) {
            ret = -errno;
            close(i);
        } else {
            close(i);
        }
    }
    return 0;
}

static int __write_to_ylog_initialize2(void) {
    int i, ret = 0;
    i = TEMP_FAILURE_RETRY(socket(PF_UNIX, SOCK_DGRAM | SOCK_CLOEXEC, 0));
    if (i < 0) {
        ret = -errno;
    } else if (TEMP_FAILURE_RETRY(fcntl(i, F_SETFL, O_NONBLOCK)) < 0) {
        ret = -errno;
        close(i);
    } else {
        static struct sockaddr_un un;
        if (un.sun_family != AF_UNIX) {
            memset(&un, 0, sizeof(struct sockaddr_un));
            un.sun_family = AF_UNIX;
            strcpy(un.sun_path, YLOGD_SOCKET_FILE);
        }
        if (TEMP_FAILURE_RETRY(connect(i, (struct sockaddr *)&un,
                        sizeof(struct sockaddr_un))) < 0) {
            ret = -errno;
            close(i);
            // printf("11111\n");
        } else {
            close(i);
        }
    }
    return 0;
}

static int get_boottime(struct timespec *ts) {
    struct timespec elapsed_timespec;
    if (clock_gettime(CLOCK_BOOTTIME, &elapsed_timespec) == -1) {
        if (ts)
            memset(ts, sizeof *ts, 0);
        return -1;
    }
    if (ts)
        *ts = elapsed_timespec;
    return 0;
}

static time_t diff_ts_nanosecond(struct timespec *b, struct timespec *a) {
    /**
     * b -- before
     * a -- after
     */
    return (a->tv_sec - b->tv_sec) * 1000000000 + (a->tv_nsec - b->tv_nsec);
}

static time_t test_func(char *desc, void (*func)(void *private), void *private, long count) {
    int i;
    struct timespec ts = {0}, ts2 = {0};
    time_t delta;

    get_boottime(&ts);
    for (i = 0; i < count; i++)
        func(private);
    get_boottime(&ts2);
    delta = diff_ts_nanosecond(&ts, &ts2);
    if (desc)
        printf("%ldns for %s\n", delta, desc);
    return delta;
}

static void test_case_____write_to_ylog_initialize(void *private) {
    UNUSED(private);
    if (__write_to_ylog_initialize() != 0)
        printf("error %s\n", strerror(errno));
}

static void test_case_____write_to_ylog_initialize2(void *private) {
    UNUSED(private);
    if (__write_to_ylog_initialize2() != 0)
        printf("error %s\n", strerror(errno));
}

static void test_case_____access(void *private) {
    UNUSED(private);
    if (access(YLOGD_SOCKET_FILE, W_OK) == 0)
        printf("error %s\n", strerror(errno));
}

int main(int argc, char *argv[]) {
    UNUSED(argc);
    UNUSED(argv);
    int ret, i;
    struct timespec ts = {0}, ts2 = {0};
    time_t delta1, delta2, delta_us;
    int repate_count;

#ifdef ANDROID
    repate_count = 10;
#else
    repate_count = 10;//100000;
#endif
    delta1 = test_func("socket_fcntl_connect_close", test_case_____write_to_ylog_initialize, NULL, repate_count);
    delta2 = test_func("access", test_case_____access, NULL, repate_count);
    delta_us = (delta1-delta2)/1000;
    printf("delta=%ld.%06lds\n", delta_us / 1000000, delta_us % 1000000);

#ifdef ANDROID
    repate_count = 100;
#else
    repate_count = 10;//100000;
#endif
    delta1 = test_func("socket_fcntl_connect_close", test_case_____write_to_ylog_initialize, NULL, repate_count);
    delta2 = test_func("socket_fcntl_connect_close2", test_case_____write_to_ylog_initialize2, NULL, repate_count);
    delta_us = (delta1-delta2)/1000;
    printf("delta=%ld.%06lds\n", delta_us / 1000000, delta_us % 1000000);

    return 0;
}
