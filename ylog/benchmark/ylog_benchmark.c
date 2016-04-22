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
#include <sys/types.h>
#include <sys/stat.h>
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
#define ylog_printf(l, msg...) if (debug_level >= l) printf(msg)
#define ylog_debug(msg...) ylog_printf(LOG_DEBUG, "ylog<debug> "msg)
#define ylog_info(msg...) ylog_printf(LOG_INFO, "ylog<info> "msg)
#define ylog_warn(msg...) ylog_printf(LOG_WARN, "ylog<warn> "msg)
#define ylog_critical(msg...) ylog_printf(LOG_CRITICAL, "ylog<critical> "msg)
#define ylog_error(msg...) ylog_printf(LOG_ERROR, "ylog<error> "msg)

#ifdef ANDROID
#include <cutils/sockets.h>
int connect_socket_local_server(char *name) {
    int fd = socket_local_client(name, ANDROID_SOCKET_NAMESPACE_ABSTRACT, SOCK_STREAM);
    if (fd < 0) {
        ylog_error("%s open %s failed: %s\n", __func__, name, strerror(errno));
        return -1;
    }
    return fd;
}
#else
int connect_socket_local_server(char *name) {
    struct sockaddr_un address;
    int fd;
    int namelen;
    /* init unix domain socket */
    fd = socket(PF_LOCAL, SOCK_STREAM, 0);
    if (fd < 0) {
        ylog_error("%s open %s failed: %s\n", __func__, name, strerror(errno));
        return -1;
    }

    namelen = strlen(name);
    /* Test with length +1 for the *initial* '\0'. */
    if ((namelen + 1) > (int)sizeof(address.sun_path)) {
        ylog_critical("%s %s length is too long\n", __func__, name);
        close(fd);
        return -1;
    }
    /* Linux-style non-filesystem Unix Domain Sockets */
    memset(&address, 0, sizeof(address));
    address.sun_family = PF_LOCAL;
    strcpy(&address.sun_path[1], name); /* local abstract socket server */

    if (connect(fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        ylog_error("%s connect %s failed: %s\n", __func__, name, strerror(errno));
        close(fd);
        return -1;
    }

    return fd;
}
#endif

static time_t diff_ts_millisecond(struct timespec *b, struct timespec *a) {
    /**
     * b -- before
     * a -- after
     */
    return (a->tv_sec - b->tv_sec) * 1000 + (a->tv_nsec - b->tv_nsec) / 1000000;
}

static int get_monotime(struct timespec *ts) {
    if (clock_gettime(CLOCK_MONOTONIC, ts) == -1) {
        ylog_error("Could not get monotonic time: %s\n", strerror(errno));
        return -1;
    }
    return 0;
}

static float ylog_get_unit_size_float_with_speed(unsigned long long size, char *unit, time_t millisecond) {
    float ret;
    /* GB/s */
    if (size >= 1024*1024*1024) {
        ret = (1000 * (float)size / (1024*1024*1024)) / millisecond;
        if (ret > 1) {
            *unit = 'G';
            return ret;
        }
    }
    /* MB/s */
    if (size >= 1024*1024) {
        ret = (1000 * (float)size / (1024*1024)) / millisecond;
        if (ret > 1) {
            *unit = 'M';
            return ret;
        }
    }
    /* KB/s */
    if (size >= 1024) {
        ret = (1000 * (float)size / (1024)) / millisecond;
        if (ret > 1) {
            *unit = 'K';
            return ret;
        }
    }
    /* B/s */
    *unit = 'B';
    return (1000 * (float)size) / millisecond;
}

static int fd_write(char *buf, int count, int fd) {
    int c;
    int retries = 2;
    int written = 0;

    if (fd < 0) {
        ylog_critical("write fd does not open\n");
        return 0;
    }

    do {
        c = write(fd, buf, count);
        if (c > 0) {
            written += c;
            if (count == c) {
                break;
            } else {
                buf += c;
                count -= c;
            }
        } else {
            ylog_error("write failed %d: %s\n", c, strerror(errno));
        }
        usleep(10*1000);
    } while (--retries);

    if (retries == 0) {
        ylog_critical("write failed: retries all\n");
    }

    return written;
}

static void socket_send_file(char *file, int buf_size, int socket) {
    if (access(file, F_OK) == 0) {
        int fd = open(file, O_RDONLY);
        char *buf;
        int len;
        long long count = 0;
        if (fd < 0) {
            ylog_error("open %s failed: %s\n", file, strerror(errno));
            return;
        }
        if (buf_size == 0)
            buf_size = 4096;
        buf = malloc(buf_size);
        if (buf == NULL) {
            ylog_critical("Can't malloc %d\n", buf_size);
            close(fd);
            return;
        }
        ylog_info("write file %s with size %d\n", file, buf_size);
        do {
            len = read(fd, buf, buf_size);
            if (len > 0) {
                if (fd_write(buf, len, socket) != len) {
                    ylog_critical("should write %d, but not it\n", len);
                    break;
                }
                count += len;
            }
        } while (len > 0);
        if (len == 0) {
            ylog_info("file %s sending %lld done\n", file, count);
        } else {
            ylog_error("open %s failed: %s\n", file, strerror(errno));
        }
        free(buf);
        close(fd);
    } else {
        ylog_critical("file %s does not exist\n", file);
        return;
    }
}

static void usage(void) {
    printf(
"ylog_benchmark [-s socket-name] [-d string data to send] [-f flie.bin to send]\n"
"               [-b buffer size for each sending with -f]\n"
);
}

int main(int argc, char *argv[]) {
    UNUSED(argc);
    UNUSED(argv);
    int ylog;
    char buf[8192];
    char data[8192];
    char *p, *pbase, *pmax = buf + sizeof(buf);
    int i;
    char arg;
    unsigned long seqt, seq = 0;
    int seq_hex_len = sizeof(seq) * 2;
    int str_len;
    unsigned long delta_speed_size = 0;
    struct timespec ts, ts2;
    time_t delta_speed_millisecond;
    float delta_speed_float;
    char delta_speed_unit;
    char cindex[16] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};
    int send_data = 0;
    int send_data_len;
    int buf_size = 4096;
    int send_file = 0;

    snprintf(buf, sizeof buf, "ylog");
    while ((arg = getopt(argc, argv, "s:d:f:b:h")) != (char)-1) {
        switch (arg) {
        case 's': snprintf(buf, sizeof buf, "%s", optarg); break;
        case 'd': send_data_len = snprintf(data, sizeof data, "%s", optarg); send_data = 1; break;
        case 'f': snprintf(data, sizeof data, "%s", optarg); send_file = 1; break;
        case 'b': buf_size = atoi(optarg); break;
        case 'h': usage(); exit(0); break;
        default: break;
        }
    }

    ylog_info("connect to socket %s\n", buf);
    ylog = connect_socket_local_server(buf);
    if (ylog < 0)
        return -1;

    if (send_data) {
        if (fd_write(data, send_data_len, ylog) == send_data_len)
            ylog_info("all %d data done\n", send_data_len);
        return 0;
    }

    if (send_file) {
        socket_send_file(data, buf_size, ylog);
        return 0;
    }

    p = pbase = buf;
    p += snprintf(p, pmax - p,
                    "00000000000000000000000000000000000000000000000000"\
                    "11111111111111111111111111111111111111111111111111"\
                    "22222222222222222222222222222222222222222222222222"\
                    "33333333333333333333333333333333333333333333333333"\
                    "44444444444444444444444444444444444444444444444444"\
                    "55555555555555555555555555555555555555555555555555"\
                    "66666666666666666666666666666666666666666666666666"\
                    "77777777777777777777777777777777777777777777777777"\
                    "88888888888888888888888888888888888888888888888888"\
                    "99999999999999999999999999999999999999999999999999"\
                    "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"\
                    "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb"\
                    "cccccccccccccccccccccccccccccccccccccccccccccccccc"\
                    "dddddddddddddddddddddddddddddddddddddddddddddddddd"\
                    "eeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee"\
                    "ffffffffffffffffffffffffffffffffffffffffffffffffff"\
                    "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"\
                    "BBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB"\
                    "CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC"\
                    "DDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDD"\
                    "EEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEE"\
                    "FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF"\
                    "GGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGG"\
                    "HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH"\
                    "IIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIII"\
                    "JJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJJ"\
                    "KKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKK"\
                    "LLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLL"\
                    "MMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMM"\
                    "NNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNN"\
                    "ylob_benchmark for ylog socket open -- 0x");
    p[seq_hex_len] = '\n';
    p[seq_hex_len+1] = 0;
    str_len = p - pbase + seq_hex_len + 1;

    get_monotime(&ts);

    for (;;) {
        seqt = seq++;
        for (i = seq_hex_len -1 ; i >= 0; i--) {
            p[i] = cindex[seqt & 0xf];
            seqt >>= 4;
        }
        write(ylog, buf, str_len);
        delta_speed_size += str_len;
        if (delta_speed_size >= 20*1024*1024) {
            get_monotime(&ts2);
            delta_speed_millisecond = diff_ts_millisecond(&ts, &ts2);
            delta_speed_float = ylog_get_unit_size_float_with_speed(delta_speed_size,
                            &delta_speed_unit, delta_speed_millisecond);
            ts = ts2;
            delta_speed_size = 0;
            printf("ylog socket write speed %.2f%c/s\n", delta_speed_float, delta_speed_unit);
        }
    }

    return 0;
}
