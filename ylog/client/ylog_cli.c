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
#include <poll.h>

enum loglevel {
    LOG_ERROR,
    LOG_CRITICAL,
    LOG_WARN,
    LOG_INFO,
    LOG_DEBUG,
};
int debug_level = LOG_INFO;
#ifdef ANDROID
#define LOG_TAG "YLOG_CLI"
#include "cutils/log.h"
#include "cutils/properties.h"
#define ___ylog_printf_trace___ SLOGW
#define ___ylog_printf___ printf
#else
#define ___ylog_printf_trace___(x...)
#define ___ylog_printf___ printf
#endif
#define cli_trace(msg...)  ___ylog_printf_trace___(msg)
#define cli_printf(msg...) ___ylog_printf___(msg)
#define cli_printf_debug(l, msg...) if (debug_level >= l) cli_printf(msg)
#define cli_debug(msg...) cli_printf_debug(LOG_DEBUG, "ylog<debug> "msg)
#define cli_info(msg...) cli_printf_debug(LOG_INFO, "ylog<info> "msg)
#define cli_warn(msg...) cli_printf_debug(LOG_WARN, "ylog<warn> "msg)
#define cli_critical(msg...) cli_printf_debug(LOG_CRITICAL, "ylog<critical> "msg)
#define cli_error(msg...) { cli_printf_debug(LOG_ERROR, "ylog<error> "msg); cli_trace("ylog<error> "msg); }
#define ARRAY_LEN(A) (sizeof(A)/sizeof((A)[0]))

int connect_socket_local_server(char *name) {
    struct sockaddr_un address;
    int fd;
    int namelen;
    /* init unix domain socket */
    fd = socket(PF_LOCAL, SOCK_STREAM, 0);
    if (fd < 0) {
        cli_error("%s open %s failed: %s\n", __func__, name, strerror(errno));
        return -1;
    }

    namelen = strlen(name);
    /* Test with length +1 for the *initial* '\0'. */
    if ((namelen + 1) > (int)sizeof(address.sun_path)) {
        cli_critical("%s %s length is too long\n", __func__, name);
        close(fd);
        return -1;
    }
    /* Linux-style non-filesystem Unix Domain Sockets */
    memset(&address, 0, sizeof(address));
    address.sun_family = PF_LOCAL;
    strcpy(&address.sun_path[1], name); /* local abstract socket server */

    if (connect(fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        cli_error("%s connect %s failed: %s\n", __func__, name, strerror(errno));
        return -1;
    }

    return fd;
}

int main(int argc, char *argv[]) {
    int ylog;
    int i = 0;
    char buf[4096];
    int buf_size = sizeof buf;
    int ret;
    char *p, *pmax;
    struct pollfd pfd[2];
    int forced_exit = 0;

#ifdef ANDROID
    i = 0;
    do {
        char prop0[PROPERTY_VALUE_MAX];
        char prop1[PROPERTY_VALUE_MAX];
        property_get("init.svc.ylog", prop0, "stopped");
        property_get("persist.ylog.enabled", prop1, "0");
        if ((i++ < 25) && ( \
            (strcmp(prop0, "running") == 0) || \
            (strcmp(prop1, "1") == 0))) {
            ylog = connect_socket_local_server("ylog_cli");
            if (ylog < 0) {
                cli_trace("waiting for ylog service ready -> msleep(200)");
                usleep(200*1000);
            } else
                break;
        } else {
            cli_trace("ylog service does not run now");
            return -1;
        }
    } while (1);
#else
    ylog = connect_socket_local_server("ylog_cli");
    if (ylog < 0)
        return -1;
#endif

    p = buf;
    pmax = buf + buf_size;
    for (i = 1; i < argc; i++) {
        p += snprintf(p, pmax - p, "%s ", argv[i]);
    }
    p += snprintf(p, pmax - p, "\n");
    cli_debug("cli -> server :%s", buf);

    write(ylog, buf, strlen(buf));
    cli_trace("%s", buf);

    pfd[0].fd = ylog;
    pfd[0].events = POLLIN;

    for (;;) {
        if (poll(pfd, ARRAY_LEN(pfd), -1) <= 0) {
            cli_error("poll");
            continue;
        }
        if (pfd[0].revents) {
            ret = read(pfd[0].fd, buf, buf_size);
            if (ret > 0) {
                if (ret >= 20 && strncmp(&buf[ret-20], "____cli____exit____\n", 20) == 0) {
                    ret -= 20;
                    buf[ret] = 0;
                    forced_exit = 1;
                }
                write(STDOUT_FILENO, buf, ret);
                if (forced_exit) {
                    cli_debug("cli is disconnected by ylog\n");
                    exit(0);
                }
            } else if (ret == 0) {
                cli_debug("server closed\n");
                exit(0);
            } else {
                cli_error("cli read failed: %s\n", strerror(errno));
                exit(0);
            }
        }
    }

    return 0;
}
