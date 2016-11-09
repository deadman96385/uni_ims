/**
 * Copyright (C) 2016 Spreadtrum Communications Inc.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <time.h>
#include <poll.h>
#include <unistd.h>
#include <fcntl.h>

#define UNUSED(x) (void)(x) /* avoid compiler warning */

enum loglevel {
    LOG_ERROR,
    LOG_CRITICAL,
    LOG_WARN,
    LOG_INFO,
    LOG_DEBUG,
};
int debug_level = LOG_DEBUG;
#define d_printf(l, msg...) if (debug_level >= l) printf(msg)
#define d_debug(msg...) d_printf(LOG_DEBUG, "ylog<debug> "msg)
#define d_info(msg...) d_printf(LOG_INFO, "ylog<info> "msg)
#define d_warn(msg...) d_printf(LOG_WARN, "ylog<warn> "msg)
#define d_critical(msg...) d_printf(LOG_CRITICAL, "ylog<critical> "msg)
#define d_error(msg...) d_printf(LOG_ERROR, "ylog<error> "msg)

#ifdef ANDROID
// #define SOCKET_UDP_DGRAM_TYPE
#include <cutils/sockets.h>
#ifdef SOCKET_UDP_DGRAM_TYPE
static int create_socket_local_server(int *fd, char *socket_name) {
    int sock = android_get_control_socket(socket_name);
    if (sock < 0) {
        d_error("android_get_control_socket %s failed: %s\n", socket_name, strerror(errno));
        sock = socket_local_server(socket_name, ANDROID_SOCKET_NAMESPACE_RESERVED, SOCK_DGRAM); /* DGRAM no need listen */
        if (sock < 0) {
            d_error("socket_local_server %s failed: %s\n", socket_name, strerror(errno));
        }
    }
    *fd = sock;
    return sock < 0 ? -1 : 0;
}
#else
static int create_socket_local_server(int *fd, char *file) {
    *fd = socket_local_server(file, ANDROID_SOCKET_NAMESPACE_ABSTRACT, SOCK_STREAM);
    if (*fd < 0) {
        d_error("open %s failed: %s\n", file, strerror(errno));
        return -1;
    }
    return 0;
}
#endif
#else
#define CLOSE(fd) ({\
    int ll_ret = -1; \
    if (fd < 0) { \
        d_critical("BUG close fd is %d %s\n", fd, __func__); \
    } else {\
        if (fd == 0) \
            d_critical("BUG: close fd is %d %s\n", fd, __func__); \
        ll_ret = close(fd); \
        if (ll_ret < 0) \
            d_error("close %s %s\n", __func__, strerror(errno)); \
    } \
    ll_ret; \
})

static int create_socket_local_server(int *fd, char *file) {
    struct sockaddr_un address;
    int namelen;
    int ffd;

    /* init unix domain socket */
    ffd = *fd = socket(PF_LOCAL, SOCK_STREAM, 0);
    if (*fd < 0) {
        d_error("open %s failed: %s\n", file, strerror(errno));
        return -1;
    }

    namelen = strlen(file);
    /* Test with length +1 for the *initial* '\0'. */
    if ((namelen + 1) > (int)sizeof(address.sun_path)) {
        d_critical("%s length is too long\n", file);
        CLOSE(ffd);
        return -1;
    }

    /* Linux-style non-filesystem Unix Domain Sockets */
    memset(&address, 0, sizeof(address));
    address.sun_family = PF_LOCAL;
    strcpy(&address.sun_path[1], file); /* local abstract socket server */

    if (bind(*fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        d_error("bind %s failed: %s\n", file, strerror(errno));
        CLOSE(ffd);
        return -1;
    }

    if (listen(*fd, 3) < 0) {
        d_error("listen %s failed: %s\n", file, strerror(errno));
        CLOSE(ffd);
        return -1;
    }

    return 0;
}
#endif

static int accept_client(int fd) {
    struct sockaddr addr;
    socklen_t addrlen = sizeof addr;
    return accept(fd, &addr, &addrlen);
}

static int fd_socket_server;

static void usage(void) {
    printf(
"ylog_benchmark_socket_server [-s socket-name]\n"
);
}

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

static void process_client_data(int fd, char *buf, int buf_size) {
    int ret;
    ret = read(fd, buf, buf_size);
    if (ret > 0)
        write(STDOUT_FILENO, buf, ret);
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
    char arg;
    int fd_client;
    int len;
    char buf[32*1024];

    snprintf(buf, sizeof buf, "ylog_benchmark_socket_server");
    while ((arg = getopt(argc, argv, "s:h")) != (char)-1) {
        switch (arg) {
            case 's': snprintf(buf, sizeof buf, "%s", optarg); break;
            case 'h': usage(); exit(0); break;
            default: break;
        }
    }

    if (create_socket_local_server(&fd_socket_server, buf) != 0)
        return 0;

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
