/**
 * Copyright (C) 2016 Spreadtrum Communications Inc.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/un.h>
#include <cutils/sockets.h>
#include <sys/time.h>
#include <time.h>
#include <poll.h>
#include <log/log.h>
#include <log/log_read.h>
#include <log/logprint.h>
#include <sys/stat.h>
#include <sys/types.h>

#define SOCKET_UDP_DGRAM_TYPE

#ifdef SOCKET_UDP_DGRAM_TYPE
#define ANDROID_RESERVED_SOCKET_PREFIX "/dev/socket/ylog"
#define YLOGD_SOCKET_FILE ANDROID_RESERVED_SOCKET_PREFIX"/ylogdw"
#else
#define YLOGD_SOCKET_FILE "ylogdw"
#endif

#define UNUSED(x) (void)(x) /* avoid compiler warning */

enum loglevel {
    LOG_ERROR,
    LOG_CRITICAL,
    LOG_WARN,
    LOG_INFO,
    LOG_DEBUG,
};
int debug_level = LOG_DEBUG;

struct __attribute__((__packed__)) ylogd_header_t {
    uint16_t version;
    typeof_log_id_t id;
    uint16_t uid;
    uint16_t pid;
    uint16_t tid;
    log_time realtime;
    uint16_t msg_offset;
    uint16_t msg_len;
};

static char ylogd_tag[][2] = {"Y0"/*main*/, "Y2"/*radio*/, "Y3"/*events*/, "Y1"/*system*/, "Y4"/*crash*/};

#define MS_PER_NSEC 1000000
#define d_printf(l, msg...) if (debug_level >= l) printf(msg)
#define d_debug(msg...) d_printf(LOG_DEBUG, "Y1ylogd<debug> "msg)
#define d_info(msg...) d_printf(LOG_INFO, "Y1ylogd<info> "msg)
#define d_warn(msg...) d_printf(LOG_WARN, "Y1ylogd<warn> "msg)
#define d_critical(msg...) d_printf(LOG_CRITICAL, "Y1ylogd<critical> "msg)
#define d_error(msg...) d_printf(LOG_ERROR, "Y1ylogd<error> "msg)

#ifdef SOCKET_UDP_DGRAM_TYPE
static int create_socket_local_server(char *socket_name) {
    int on = 1;
    int sock = android_get_control_socket(socket_name);
    if (sock < 0) {
        d_error("android_get_control_socket %s failed: %s\n", socket_name, strerror(errno));
        sock = socket_local_server(socket_name,
                socket_name[0] == '/' ? ANDROID_SOCKET_NAMESPACE_FILESYSTEM : ANDROID_SOCKET_NAMESPACE_RESERVED,
                SOCK_DGRAM); /* DGRAM no need listen */
        if (setsockopt(sock, SOL_SOCKET, SO_PASSCRED, &on, sizeof(on)) < 0) {
            return -1;
        }
        if (sock < 0) {
            d_error("socket_local_server %s failed: %s\n", socket_name, strerror(errno));
        }
    }
    return sock;
}
#else
#include <cutils/sockets.h>
static int create_socket_local_server(char *socket_name) {
    int fd = socket_local_server(socket_name, ANDROID_SOCKET_NAMESPACE_ABSTRACT, SOCK_STREAM); /* listen embedded */
    if (fd < 0) {
        d_error("socket_local_server %s failed: %s\n", socket_name, strerror(errno));
        return -1;
    }
    return fd;
}
#endif

static int accept_client(int fd) {
    struct sockaddr addr;
    socklen_t addrlen = sizeof addr;
    return accept(fd, &addr, &addrlen);
}

static int fd_socket_server;

/* Used to retry syscalls that can return EINTR. */
#define TEMP_FAILURE_RETRY(exp) ({         \
    __typeof__(exp) _rc;                   \
    do {                                   \
        _rc = (exp);                       \
    } while (_rc == -1 && errno == EINTR); \
    _rc; })

static struct pollfd gpfd[1024];
static int pfd_max_size = 1024;
static int gpfd_max;

static int insert_fd(int fd) {
    int i;

    if (TEMP_FAILURE_RETRY(fcntl(fd, F_SETFL, O_NONBLOCK)) < 0) {
        close(fd);
        return -2;
    }

    if (gpfd_max < pfd_max_size) {
        gpfd[gpfd_max].fd = fd;
        gpfd[gpfd_max].events = POLLIN;
        gpfd_max++;
        d_debug("new client : %d\n", gpfd_max);
        return gpfd_max;
    }

    return -1;
}

static int erase_fd(int fd) {
    int i, ret = -1;
    for (i = 0; i < gpfd_max; i++) {
        if (gpfd[i].fd == fd) {
            gpfd_max -= 1;
            if (i != gpfd_max)
                gpfd[i] = gpfd[gpfd_max];
            gpfd[gpfd_max].fd = 0;
            gpfd[gpfd_max].events = 0;
            d_debug("close client : %d\n", gpfd_max);
            ret = 0;
            break;
        }
    }
    close(fd);
    return ret;
}

static char filterPriToChar(android_LogPriority pri) {
    switch (pri) {
        case ANDROID_LOG_VERBOSE:       return 'V';
        case ANDROID_LOG_DEBUG:         return 'D';
        case ANDROID_LOG_INFO:          return 'I';
        case ANDROID_LOG_WARN:          return 'W';
        case ANDROID_LOG_ERROR:         return 'E';
        case ANDROID_LOG_FATAL:         return 'F';
        case ANDROID_LOG_SILENT:        return 'S';
        case ANDROID_LOG_DEFAULT:
        case ANDROID_LOG_UNKNOWN:
        default:                        return '?';
    }
}

/*
 * got information from this function in liblog:
 * android_log_formatLogLine(p_format, defaultBuffer,
 *		            sizeof(defaultBuffer), entry, &totalLen);
 */
static char *ylogd_log_format_logline(int fd, AndroidLogEntry *entry, int id) {
    char priChar;
    time_t now;
    unsigned long nsec;
    struct tm tmBuf;
    struct tm* ptm;
    char timeBuf[64];
    size_t len;
    char prefixBuf[128];
    struct iovec vec[4];

    now = entry->tv_sec;
    nsec = entry->tv_nsec;

    priChar = filterPriToChar(entry->priority);
    if (now < 0) {
        nsec = NS_PER_SEC - nsec;
    }
    ptm = localtime_r(&now, &tmBuf);
    strftime(timeBuf, sizeof(timeBuf),
            "%m-%d %H:%M:%S",
            ptm);

    len = snprintf(prefixBuf, sizeof(prefixBuf),
            ".%03ld %5d %5d %c %-8s: ",
            nsec / MS_PER_NSEC, entry->pid, entry->tid, priChar, entry->tag);

    vec[0].iov_base = ylogd_tag[id];
    vec[0].iov_len = 2;
    vec[1].iov_base = timeBuf;
    vec[1].iov_len = 14;
    vec[2].iov_base = prefixBuf;
    vec[2].iov_len = len;
    vec[3].iov_base = (void *)entry->message;
    vec[3].iov_len = entry->messageLen;
    writev(fd, vec, 4);
    return 0;
}

static void parse_log(char *buf, int len, struct ucred *cred) {
    struct ylogd_header_t *yh;
    char *prio;
    char *tag;
    char *msg;
    int tlen;
    AndroidLogEntry entry;

    yh = (struct ylogd_header_t *)buf;

    tlen = yh->msg_offset + yh->msg_len;

    if (yh->msg_len < 1 || tlen > len) {
        d_critical("wrong packet size, msg_offset=%d, msg_len=%d, len=%d\n",
                yh->msg_offset, yh->msg_len, len);
        return;
    }
    if (tlen != len)
        d_warn("packet size not match : %d/%d\n", tlen, len);

    if (yh->id == LOG_ID_MAIN ||
            yh->id == LOG_ID_SYSTEM ||
            yh->id == LOG_ID_RADIO) {
        prio = buf + sizeof(struct ylogd_header_t);
        tag = prio + 1;
        msg = buf + yh->msg_offset;

        entry.tv_sec = yh->realtime.tv_sec;
        entry.tv_nsec = yh->realtime.tv_nsec;
        entry.priority = *(buf+sizeof(struct ylogd_header_t));
        entry.uid = cred->uid;
        entry.pid = cred->pid;
        entry.tid = yh->tid;
        entry.tag = tag;
        entry.messageLen = yh->msg_len - 1;
        entry.message = msg;
        if (msg[entry.messageLen - 1] != '\n') {
            msg[entry.messageLen++] = '\n';
            msg[entry.messageLen] = '\0';
        }
        ylogd_log_format_logline(STDOUT_FILENO, &entry, yh->id);
    }
}

static void process_client_data(int fd, char *buf, int buf_size) {
    int ret;
    struct ucred *cred = NULL;
    struct cmsghdr *cmsg = NULL;
    struct iovec iov = { buf, buf_size };
    char control[CMSG_SPACE(sizeof(struct ucred))] __aligned(4);
    struct msghdr hdr = {
        NULL,
        0,
        &iov,
        1,
        control,
        sizeof(control),
        0,
    };
    ret = recvmsg(fd, &hdr, 0);

    cmsg = CMSG_FIRSTHDR(&hdr);
    while (cmsg != NULL) {
        if (cmsg->cmsg_level == SOL_SOCKET
            && cmsg->cmsg_type  == SCM_CREDENTIALS) {
            cred = (struct ucred *)CMSG_DATA(cmsg);
            break;
        }
        cmsg = CMSG_NXTHDR(&hdr, cmsg);
    }
    if (cred != NULL && ret > 0)
        parse_log(buf, ret, cred);
    else
        erase_fd(fd);
}

static int check_fd(char *buf, int buf_size) {
    int i;
    for (i = 0; i < gpfd_max; i++) {
        if (gpfd[i].revents) {
            int fd = gpfd[i].fd;
#ifndef SOCKET_UDP_DGRAM_TYPE
            if (fd == fd_socket_server) {
                int fd_client = accept_client(fd_socket_server);
                if (fd_client >= 0)
                    insert_fd(fd_client);
            } else
#endif
            {
                process_client_data(fd, buf, buf_size);
            }
        }
    }
    return 0;
}

int main(int argc, char *argv[]) {
    UNUSED(argc);
    UNUSED(argv);
    char buf[32*1024];

#ifdef SOCKET_UDP_DGRAM_TYPE
    if (access(ANDROID_RESERVED_SOCKET_PREFIX, X_OK) && \
        mkdir(ANDROID_RESERVED_SOCKET_PREFIX, 0755))
        d_error("mkdir error %s\n", strerror(errno));
#endif

    umask(0555);
    if ((fd_socket_server = create_socket_local_server(YLOGD_SOCKET_FILE)) < 0)
        return 0;
    /* umask(0); */

    insert_fd(fd_socket_server);

    for (;;) {
        if (poll(gpfd, gpfd_max, -1) < 0) {
            d_error("poll error %d -> %s\n", errno, strerror(errno));
            continue;
        }
        check_fd(buf, sizeof buf);
    }

    return 0;
}
