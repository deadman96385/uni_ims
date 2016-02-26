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

enum loglevel {
    LOG_ERROR,
    LOG_CRITICAL,
    LOG_WARN,
    LOG_INFO,
    LOG_DEBUG,
};
int debug_level = LOG_DEBUG;
#define ylog_printf(l, msg...) if (debug_level >= l) printf(msg)
#define ylog_debug(msg...) ylog_printf(LOG_DEBUG, "ylog<debug> "msg)
#define ylog_info(msg...) ylog_printf(LOG_INFO, "ylog<info> "msg)
#define ylog_warn(msg...) ylog_printf(LOG_WARN, "ylog<warn> "msg)
#define ylog_critical(msg...) ylog_printf(LOG_CRITICAL, "ylog<critical> "msg)
#define ylog_error(msg...) ylog_printf(LOG_ERROR, "ylog<error> "msg)

static int create_socket_local_server(int *fd, char *file) {
    struct sockaddr_un address;
    int namelen;

    /* init unix domain socket */
    *fd = socket(PF_LOCAL, SOCK_STREAM, 0);
    if (*fd < 0) {
        ylog_error("open %s failed: %s\n", file, strerror(errno));
        return -1;
    }

    namelen = strlen(file);
    /* Test with length +1 for the *initial* '\0'. */
    if ((namelen + 1) > (int)sizeof(address.sun_path)) {
        ylog_critical("%s length is too long\n", file);
        close(*fd);
        return -1;
    }

    /* Linux-style non-filesystem Unix Domain Sockets */
    memset(&address, 0, sizeof(address));
    address.sun_family = PF_LOCAL;
    strcpy(&address.sun_path[1], file); /* local abstract socket server */

    if (bind(*fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        ylog_error("bind %s failed: %s\n", file, strerror(errno));
        close(*fd);
        return -1;
    }

    if (listen(*fd, 3) < 0) {
        ylog_error("listen %s failed: %s\n", file, strerror(errno));
        close(*fd);
        return -1;
    }

    return 0;
}

static int accept_client(int fd) {
    struct sockaddr addr;
    socklen_t addrlen = sizeof addr;
    return accept(fd, &addr, &addrlen);
}

int fd_socket_server;

static void usage(void) {
    printf(
"ylog_benchmark_socket_server [-s socket-name]\n"
);
}

int main(int argc, char *argv[]) {
    char buf[64 * 1024];
    char arg;
    int fd_client;
    int len;

    snprintf(buf, sizeof buf, "ylog_benchmark_socket_server");
    while ((arg = getopt(argc, argv, "s:h")) != (char)-1) {
        switch (arg) {
            case 's': snprintf(buf, sizeof buf, "%s", optarg); break;
            case 'h': usage(); exit(0); break;
            default: break;
        }
    }

    create_socket_local_server(&fd_socket_server, buf);

    for (;;) {
        if ((fd_client = accept_client(fd_socket_server)) > 0) {
            do {
                len = read(fd_client, buf, sizeof buf);
                if (len > 0)
                    write(STDOUT_FILENO, buf, len);
            } while (len > 0);
            close(fd_client);
        }
    }

    return 0;
}
