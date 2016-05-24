/**
 * Copyright (C) 2016 Spreadtrum Communications Inc.
 */

#include <cutils/sockets.h>

#define SOCKET_UDP_DGRAM_TYPE

#ifdef SOCKET_UDP_DGRAM_TYPE
#define YLOGD_SOCKET_FILE "/dev/socket/ylog/ylogdw"
#else
#define YLOGD_SOCKET_FILE "ylogdw"
#endif

/**
 * Steps:
 * 1. open file system/core/liblog/logd_write.c
 * 2. append following code
 * 2.a. #include "libylog.c"
 * 2.b. WRITE_TO_YLOGD(); in __write_to_log_daemon
 */
#define WRITE_TO_YLOGD() \
    ylogd_header_t yh; \
    yh.id = log_id; \
    yh.uid = last_uid; \
    yh.pid = last_pid; \
    yh.tid = header.tid; \
    yh.realtime = header.realtime; \
    write_to_ylogd(vec, nr, &yh);

typedef struct  __attribute__((__packed__)) {
    uint16_t version;
    typeof_log_id_t id;
    uint16_t uid;
    uint16_t pid;
    uint16_t tid;
    log_time realtime;
    uint16_t msg_offset;
    uint16_t msg_len;
} ylogd_header_t;

static int ylogd_fd = -1;

// #define YLOGD_DEBUG_SELF

#ifdef YLOGD_DEBUG_SELF
#define ___ylogd_printf___ write2file
static pthread_mutex_t ylog_write2file_lock = PTHREAD_MUTEX_INITIALIZER;
static int write2file(char *buf, int len) {
    static int fd = -1;
    pthread_mutex_lock(&ylog_write2file_lock);
    if (fd < 0) {
        char name[128];
        pid_t pid = getpid();
        snprintf(name, sizeof name, "/data/local/tmp/%d", pid);
        unlink(name);
        fd = open(name, O_RDWR | O_CREAT | O_APPEND, 0666);
    }
    if (fd >= 0) {
        write(fd, buf, len);
    }
    pthread_mutex_unlock(&ylog_write2file_lock);
    return 0;
}

static int ylog_tv2format_time(char *timeBuf, struct timeval *tv) {
    struct tm tm;
    struct tm* ptm;
    time_t t;
    int len;

    t = tv->tv_sec; //time(NULL);
    //#if defined(HAVE_LOCALTIME_R)
    ptm = localtime_r(&t, &tm);
    //#else
    //    ptm = localtime(&t);
    //#endif
    /* strftime(timeBuf, sizeof(timeBuf), "%Y-%m-%d %H:%M:%S", ptm); */
    len = strftime(timeBuf, 32, "[%m-%d %H:%M:%S", ptm);
    if (1/* usec_time_output */) {
        /* 01-01 22:14:36.000   826   826 D SignalClusterView_dual: */
        len += snprintf(timeBuf + len, 32 - len,
                ".%03ld] ", tv->tv_usec / 1000);
    } else {
        /* 01-01 22:14:36.000   826   826 D SignalClusterView_dual: */
        len += snprintf(timeBuf + len, 32 - len,
                ".%03ld] ", tv->tv_usec / 1000000);
    }
    return len;
}

static int ylog_get_format_time(char *buf) {
    char *timeBuf = buf;
    struct timeval tv;
    //char timeBuf[32];/* good margin, 23+nul for msec, 26+nul for usec */
    /* From system/core/liblog/logprint.c */
    /*
     * Get the current date/time in pretty form
     *
     * It's often useful when examining a log with "less" to jump to
     * a specific point in the file by searching for the date/time stamp.
     * For this reason it's very annoying to have regexp meta characters
     * in the time stamp.  Don't use forward slashes, parenthesis,
     * brackets, asterisks, or other special chars here.
     */
    gettimeofday(&tv, NULL);
    return ylog_tv2format_time(timeBuf, &tv);
}

static int ylogd_printf_format(int level, const char *fmt, ...) {
    if (100 >= level) {
        int len;
        va_list ap;
        char buf[4096];
        len = ylog_get_format_time(buf);
        va_start(ap, fmt);
        len += vsnprintf(buf + len, sizeof(buf) - len, fmt, ap);
        va_end(ap);
        ___ylogd_printf___(buf, len);
    }
    return 0;
}
#define ylogd_printf(level, fmt...) ylogd_printf_format(level, fmt)
#define ylogd_debug(msg...) ylogd_printf(5, "ylogd<debug> "msg)
#else
#define ylogd_debug(msg...) (void*)(0)
#endif

static int __write_to_ylog_initialize(void) {
    int i, ret = 0;
#ifdef SOCKET_UDP_DGRAM_TYPE
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
            ylogd_fd = i;
        }
    }
#else
    i = socket_local_client(YLOGD_SOCKET_FILE, ANDROID_SOCKET_NAMESPACE_ABSTRACT, SOCK_STREAM);
    if (i < 0) {
        ret = -errno;
    } else if (TEMP_FAILURE_RETRY(fcntl(i, F_SETFL, O_NONBLOCK)) < 0) {
        ret = -errno;
        close(i);
    } else {
        ylogd_fd = i;
    }
#endif
    if (ret < 0)
        ylogd_debug("%s failed %d -> %s\n", __func__, ret, strerror(-ret));
    else
        ylogd_debug("%s success\n", __func__);
    return ret;
}

static int write_to_ylogd(struct iovec *vec, size_t nr, ylogd_header_t *yh) {
    int ret, i, cnt;

    if (ylogd_fd < 0) {
#ifdef SOCKET_UDP_DGRAM_TYPE
#if 0
        /**
         * 1. YLOGD_SOCKET_FILE will be created when ylogd started, and deleted when ylogd stopped
         * 2. access /dev/ ramfs should be faster than socket + fcntl + connnect
         *
         * we use access_socket_fcnt_connect_close_test to test this thing
         *
         * 1. in x80, access is always faster than socket+fcntl+connect
         * 2. in arm, access is unstable, but more time is slower than socket+fcntl+connect
         *
         * Conclusion: in arm still keep using orignal method socket+fcntl+connect
         */
        if (access(YLOGD_SOCKET_FILE, W_OK))
            return -EBADF;
#endif
#endif
        lock();
        if (ylogd_fd < 0)
            ret = __write_to_ylog_initialize();
        unlock();
        if (ylogd_fd < 0) {
            return -EBADF;
        }
    }

    struct iovec newVec[nr + 1];
    newVec[0].iov_base   = (unsigned char *)yh;
    newVec[0].iov_len    = yh->msg_offset = sizeof(*yh);
    cnt = 1;
    for (i = 0; i < (int)nr; i++, cnt++) {
        newVec[i+1] = vec[i];
        yh->msg_offset += vec[i].iov_len;
    }
    yh->msg_offset -= vec[nr-1].iov_len;
    yh->msg_len = vec[nr-1].iov_len;

    if (yh->id == LOG_ID_MAIN ||
        yh->id == LOG_ID_SYSTEM ||
        yh->id == LOG_ID_RADIO) {
        char *msg = vec[2].iov_base;
        ylogd_debug("id=%d %s\n", yh->id, msg);
    }

    /*
     * The write below could be lost, but will never block, because of fcntl(i, F_SETFL, O_NONBLOCK) above
     *
     * ENOTCONN or EPIPE occurs if ylogd dies.
     * EAGAIN occurs if ylogd is overloaded.
     * EBADF occurs if ylogd dies and many threads are calling writev but ylogd_fd is assigned -1 by one thread
     */
    ret = TEMP_FAILURE_RETRY(writev(ylogd_fd, newVec, cnt));
    if (ret < 0) {
        ret = -errno;
        if (1/* ret == -ENOTCONN || ret == -EPIPE */) {
            lock();
            if (ylogd_fd >= 0) {
                close(ylogd_fd);
                ylogd_fd = -1;
                /* we don't think ylogd can recover so fast, lost this log ruthless */
            }
            unlock();
        }
        ylogd_debug("%s failed %d -> %s\n", __func__, ret, strerror(-ret));
    }
    return ret;
}
