/**
 * Copyright (C) 2016 Spreadtrum Communications Inc.
 */

#define LOG_TAG "LOGD_TEST"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <cutils/log.h>
#include <unistd.h>
#include <errno.h>

//#define KERNEL_TEST
#ifdef KERNEL_TEST
#define ALOGD printf
#define ALOGE printf
#endif

#define UNUSED(x) (void)(x) /* avoid compiler warning */

static volatile int g_delay_time = 100000;
static volatile int g_total_lines = 0;
static volatile int define_line_num = 0;
static time_t diff_ts_millisecond(struct timespec *b, struct timespec *a) {

    /*

     * b -- before
     * a -- after
     */
    return (a->tv_sec - b->tv_sec) * 1000 + (a->tv_nsec - b->tv_nsec) / 1000000;
}

static int get_monotime(struct timespec *ts) {
       if (clock_gettime(CLOCK_MONOTONIC, ts) == -1) {
               printf("Could not get monotonic time: %s\n", strerror(errno));
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

static int parse_cmdline(int argc,char *argv[])
{
    int oc;
    if (argc == 1) {
        printf("please type in a option\n");
        return 0;
    }
    if (strrchr(argv[1],'-')) {
        while((oc = getopt(argc,argv,"t:l:")) != -1){
            switch(oc){
                case 't':
                g_delay_time = strtoul(optarg,NULL,0);
                    break;
				case 'l':
					define_line_num = 1;
					g_total_lines = strtoul(optarg, NULL, 0);
                case '?':
                    printf("no this argv\n");
                    break;
                case ':':
                    printf("Miss option argv\n");
            }
        }
        if (optind > argc) {
            printf("Expected argument after options\n");
            exit(EXIT_FAILURE);
        }
    }
    return 0;
}

int main(int argc, char *argv[])
{
    UNUSED(argc);
    UNUSED(argv);
    int ylog;
    char buf[8192];
    char *p, *pbase, *pmax = buf + sizeof(buf);
    int i;
    unsigned long seqt, seq = 0;
    int seq_hex_len = sizeof(seq) * 2;
    int str_len;
    unsigned long delta_speed_size = 0;
    struct timespec ts, ts2, ts3, ts4;
    time_t delta_speed_millisecond;
    float delta_speed_float;
    unsigned long log_count = 0;
    float log_Hz;
    char delta_speed_unit;
    char cindex[16] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};

    p = pbase = buf;
/*                    "00000000000000000000000000000000000000000000000000"\
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
                    "DDDDDDDDDDDDDDDDDDDDDDDD"\
*/
    p += snprintf(p, pmax - p,
                    "test_benchmark for logd socket open -- 0x");
    p[seq_hex_len] = '\n';
    p[seq_hex_len+1] = 0;
    str_len = p - pbase + seq_hex_len + 1;

    get_monotime(&ts);
    get_monotime(&ts3);
    parse_cmdline(argc,argv);
    printf("str_len = %d\n",str_len);
    for (;;) {
        seqt = seq++;
        for (i = seq_hex_len -1 ; i >= 0; i--) {
            p[i] = cindex[seqt & 0xf];
            seqt >>= 4;
        }
		if (define_line_num) {
			if(!g_total_lines--) break;
		}
        ALOGE(buf);

        log_count ++;
        delta_speed_size += str_len;
        if (delta_speed_size >= 20*1024*1024) {
            get_monotime(&ts2);
            delta_speed_millisecond = diff_ts_millisecond(&ts, &ts2);

            delta_speed_float = ylog_get_unit_size_float_with_speed(delta_speed_size,
                            &delta_speed_unit, delta_speed_millisecond);
            ts = ts2;
            delta_speed_size = 0;
            log_Hz = 1000*(float)log_count / delta_speed_millisecond;
            log_count = 0;
            printf("ylog socket write speed %.2f%c/s log_Hz %.2f\n", delta_speed_float, delta_speed_unit,log_Hz);
        }
        usleep(g_delay_time);
        if(g_delay_time != 0) {
            get_monotime(&ts4);
            if (diff_ts_millisecond(&ts3, &ts4) > 5000) {
                g_delay_time -= g_delay_time*0.05;
                ts3 = ts4;
                printf("g_delay_time = %d\n",g_delay_time);
                if (g_delay_time <= 0 )
                    g_delay_time = 0;
            }
        }

    }

    return 0;
}
