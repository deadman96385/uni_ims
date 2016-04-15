/**
 *  Copyright (C) 2015 Spreadtrum Communications Inc.
 */

#include <ctype.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>
#include <sys/sysinfo.h>
#include <sys/wait.h>
#include <signal.h>
#include <unistd.h>
#include "common.h"

static int is_cpu_basic_quit = 0;
#define MAX_LINE 256
#define get_index_old(x) (x % 2)
#define get_index_new(x) ((x + 1) % 2)
// char line[MAX_LINE];
static int per_pid = 0;
static struct log_output *output = NULL;
static pid_t pid_table[10];
static unsigned long interval = 100;
static char title[2000];

#define PAGE_SIZE_B ( sysconf(_SC_PAGESIZE) )
#define ONE_K 1024L
#define PAGE_SIZE_KB ( PAGE_SIZE / ONE_K )

// struct sysinfo {
//     __kernel_long_t uptime;        /* Seconds since boot */
//     __kernel_ulong_t loads[3];    /* 1, 5, and 15 minute load averages */
//     __kernel_ulong_t totalram;    /* Total usable main memory size */
//     __kernel_ulong_t freeram;    /* Available memory size */
//     __kernel_ulong_t sharedram;    /* Amount of shared memory */
//     __kernel_ulong_t bufferram;    /* Memory used by buffers */
//     __kernel_ulong_t totalswap;    /* Total swap space size */
//     __kernel_ulong_t freeswap;    /* swap space still available */
//     __u16 procs;               /* Number of current processes */
//     __u16 pad;               /* Explicit padding for m68k */
//     __kernel_ulong_t totalhigh;    /* Total high memory size */
//     __kernel_ulong_t freehigh;    /* Available high memory size */
//     __u32 mem_unit;            /* Memory unit size in bytes */
//     char _f[20-2*sizeof(__kernel_ulong_t)-sizeof(__u32)];    /* Padding: libc5 uses this.. */
// };
#define G_unit_steps 10
static unsigned mem_unit;
struct per_pid_info {
    unsigned long pid_runtime;
    int current_cpu;
    unsigned long utime;
    unsigned long stime;
    unsigned long cutime;
    unsigned long cstime;
    unsigned long rss;
    unsigned long cpu_nr;
};
static struct per_pid_info pid_info[2];
static unsigned long pid_info_index = 0;

static int update_pid_info(struct per_pid_info *info) {
    int i;
    unsigned long totaltime = 0;
    FILE *p = NULL;
    char ppath[20];
    int rss = 0;
    int cpu = 0;
    char buffer[256];
    int cpus = -1;

    //now is only one process in the pid_table
    for (i = 0; pid_table[i] != 0; i++) {
        sprintf(ppath, "/proc/%d/stat", pid_table[i]);
        p = fopen(ppath, "r");
        if (!p) {
            printf("open error!!\n");
            return -1;
        }
        fscanf(p, "%*d %*s %*c"
            "%*d %*d %*d %*d %*d %*d"
            "%*u %*u %*u %*u"
            "%lu %lu %ld %ld" //utime, stime, cutime, cstime
            "%*d %*d %*d %*d %*u %*u %ld" //rss is not use
            "%*u %*u %*u %*u %*u %*u %*u %*u %*u %*u %*u %*u %*u %*d"
            "%d",
            &info->utime, &info->stime, &info->cutime, &info->cstime, &info->rss, &info->current_cpu);
        fclose(p);
        info->pid_runtime = info->utime + info->stime + info->cutime + info->cstime;

        sprintf(ppath, "/proc/%d/statm", pid_table[i]);
        p = fopen(ppath, "r");
        if (!p) {
            printf("open error!!\n");
            return -1;
        }
        fscanf(p, "%*d %lu", &info->rss);
        fclose(p);
    }

    p = fopen("/proc/stat", "r");
    if (p == NULL) {
        return -1;
    }

    do {
        cpus++;
        fgets(buffer, 255, p);
    } while (strstr(buffer, "cpu") == buffer);
    info->cpu_nr = ((cpus - 1 > 1) ? (cpus - 1) : 1);
    fclose(p);

    return 0;
}

static unsigned long long scale(unsigned long d) {
    return ((unsigned long long)d * mem_unit) >> G_unit_steps;
}

static unsigned long parse_cached_kb(void) {
    char buf[60]; /* actual lines we expect are ~30 chars or less */
    FILE *fp;
    unsigned long cached = 0;

    fp = fopen("/proc/meminfo", "r");
    while (fgets(buf, sizeof(buf), fp) != NULL) {
        if (sscanf(buf, "Cached: %lu %*s\n", &cached) == 1)
            break;
    }
    fclose(fp);

    return cached;
}

static int log_proc_meminfo(int flag, void *private, char *p) {
    struct sysinfo info;
    FILE *f;
    unsigned long cached;
    unsigned long pid_totalram;

    UNUSED(private);

    if (flag & PRINT_BODY) {
        sysinfo(&info);
        mem_unit = (info.mem_unit ? info.mem_unit : 1);
        /* Extract cached from /proc/meminfo and convert to mem_units */
        cached = ((unsigned long long) parse_cached_kb() * 1024) / mem_unit;
        pid_totalram = (pid_info[get_index_new(pid_info_index)].rss * PAGE_SIZE_B) / mem_unit;
        if (per_pid) {
            printf("%lld,%lld,%lld,%d,%d,%d,%d,%d,%d",\
                scale(info.totalram), scale(info.totalram - pid_totalram), scale(cached),\
                0,0,0,0,0,0);
        } else {
            printf("%lld,%lld,%lld,%d,%d,%d,%d,%d,%d",\
                scale(info.totalram), scale(info.freeram + info.bufferram + cached), scale(cached),\
                0,0,0,0,0,0);
        }
#if 0
        f = fopen("/proc/meminfo", "r");
        if (!f) return -1;

        while (fgets(line, MAX_LINE, f)) {
            sscanf(line, "MemFree: %ld kB", &s->mem_free);
            sscanf(line, "AnonPages: %ld kB", &s->mem_anon);
            sscanf(line, "Mapped: %ld kB", &s->mem_mapped);
            sscanf(line, "Slab: %ld kB", &s->mem_slab);
        }

        fclose(f);
#endif
        pid_info_index++;
     } else if (flag & PRINT_HEAD) {
        printf("%s", p);
    }
    return 0;
}

#define NR_CPUS 8
#define INFO_PER_CPU(x) \
#x"-""cpu_percent[0],"\
#x"-""iowtime[1],"\
#x"-""cpu_frequency[2],"\
#x"-""null[3],"\
#x"-""null[4],"\
#x"-""null[5],"

/**
 * cat /proc/stat format like following
 * cpu  2496129 6327 625117 5820708 179425 6 13693 0 0 0
 * cpu0 588086 738 179834 4337466 72440 5 9110 0 0 0
 * cpu1 645972 1102 155888 505196 26358 0 1360 0 0 0
 * cpu2 582835 1990 153046 502275 27062 1 1898 0 0 0
 * cpu3 679234 2496 136348 475769 53563 0 1323 0 0 0
 * intr 68560822 23 90927 0 0 0 0 0 0 1 435 0 0 816292 0 0 0 401 0 0 0 0 0 0 73 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 1197402 27 1806978 4386979 148 1279050 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0
 * ctxt 338253570
 * btime 1434924134
 * processes 1412409
 * procs_running 1
 * procs_blocked 0
 * softirq 52558883 117501 15437007 75367 12272775 1807034 3 346095 13061443 55769 9385889
 */
struct cpu_info {
    int cpu;
    unsigned long runtime;
    unsigned long utime, ntime, stime, itime;
    unsigned long iowtime, irqtime, sirqtime;
};
static struct cpu_info cpu_info[2][33];
static unsigned long cpu_info_index = 0;

static int log_cpu__print_cpu_info(FILE *f, struct cpu_info *info_old, struct cpu_info *info, int cpu_num) {
    int i;
    char cpu[6] = { 0 };
    int num = -1;
    int cpus;

    memset(info, 0, sizeof *info);

    for (i = 0; i < 5; i++)
        cpu[i] = fgetc(f);
    if (cpu[0] == 'c' && cpu[1] == 'p' && cpu[2] == 'u') {
        if (cpu[3] >= '0' && cpu[3] <='9' )
            num = strtol(&cpu[3], NULL, 0);
        else
            num = 0xff; /* all cpu */
        if (num == cpu_num) {
            fscanf(f, "%lu %lu %lu %lu %lu %lu %lu\n", &info->utime, &info->ntime, &info->stime,
                &info->itime, &info->iowtime, &info->irqtime, &info->sirqtime);
            info->runtime = info->utime + info->ntime + info->stime + info->itime +
                        info->iowtime + info->irqtime + info->sirqtime;
            while (fgetc(f) != '\n'); // read until meets '\n', nextline
        } else {
            *info = *info_old;
            info->runtime = 0;
            for (i = 4; i >= 0; i--)
                ungetc(cpu[i], f);
        }
    } else {
        for (i = 4; i >= 0; i--)
            ungetc(cpu[i], f);
    }
    {
    int iowtime_percent;
    int cpu_percent;
    if (info_old->runtime) {
        if (per_pid) {
            if (cpu_num == pid_info[get_index_new(pid_info_index)].current_cpu) {
                cpu_percent = 100.0 * (pid_info[get_index_new(pid_info_index)].pid_runtime - pid_info[get_index_old(pid_info_index)].pid_runtime) / (cpu_info[get_index_new(cpu_info_index)][0].runtime - cpu_info[get_index_old(cpu_info_index)][0].runtime);
            } else if (cpu_num == 0xff) {
                cpu_percent = 100 - (100 * (info->itime - info_old->itime)) / (info->runtime - info_old->runtime);
            } else {
                cpu_percent = 0;
            }
        } else {
            cpu_percent = 100 - (100 * (info->itime - info_old->itime)) / (info->runtime - info_old->runtime);
        }
        iowtime_percent = 100 * (info->iowtime - info_old->iowtime) / (info->runtime - info_old->runtime);
    } else {
        cpu_percent = iowtime_percent = 0;
    }
    printf("%d,%d",
            cpu_percent, iowtime_percent
          );
    }
    return num;
}

/**
 * cat /proc/stat
 */
static int log_proc_stat(int flag, void *private, char *p) {
    FILE *f;
    int  fd = -1;

    static unsigned int irqs_index = 0;
    static unsigned long irqs[2];
    static unsigned int ctxts_index = 0;
    static unsigned long ctxts[2];
    unsigned long irq, ctxt, value;
    int cpu;
    char cpu_freq[20] = {0};
    char freq_cmd[60];
    unsigned long freq = 0;
    int ret;

    UNUSED(private);

#if 0
static int read_stat(struct state *s) {
    FILE *f;

    f = fopen("/proc/stat", "r");
    if (!f) return errno;

    while (fgets(line, MAX_LINE, f)) {
        if (!strncmp(line, "cpu ", 4)) {
            sscanf(line, "cpu  %ld %ld %ld %ld %ld %ld %ld",
                &s->cpu_us, &s->cpu_ni, &s->cpu_sy, &s->cpu_id, &s->cpu_wa,
                &s->cpu_ir, &s->cpu_si);
        }
        sscanf(line, "intr %ld", &s->sys_in);
        sscanf(line, "ctxt %ld", &s->sys_cs);
        sscanf(line, "procs_running %ld", &s->procs_r);
        sscanf(line, "procs_blocked %ld", &s->procs_b);
    }

    fclose(f);

    return 0;
}
#endif

    if (flag & PRINT_BODY) {
        f = fopen("/proc/stat", "r");
        if (!f) return -1;

        if (per_pid) {
            ret = update_pid_info(&pid_info[get_index_new(pid_info_index)]);
            if (ret < 0) return -1;
        }
        for (cpu = 0; cpu < (NR_CPUS + 1); cpu++) {
            if (cpu) printf(",");
            log_cpu__print_cpu_info(f, &cpu_info[get_index_old(cpu_info_index)][cpu], &cpu_info[get_index_new(cpu_info_index)][cpu], cpu ? cpu - 1 : 0xff);

            if (cpu) {
                memset(cpu_freq, 0, sizeof(cpu_freq));
                memset(freq_cmd, 0, sizeof(freq_cmd));
                sprintf(freq_cmd, "/sys/devices/system/cpu/cpu%d/cpufreq/scaling_cur_freq", cpu - 1);
                fd = open(freq_cmd, O_RDONLY);
                if (fd<0) {
                    freq = 0;
                } else {
                    ret = read(fd, &cpu_freq, sizeof(cpu_freq));
                    if (ret < 0) {
                        freq = 0;
                    } else {
                        freq = strtoul(cpu_freq, NULL, 10);
                    }
                    close(fd);
                }
            }
            printf(",%ld,%d,%d,%d",
                freq, 0, 0, 0
            );
        }
        ++cpu_info_index;

        fscanf(f, "intr %ld", &irqs[get_index_new(irqs_index)]);
        if (get_index_old(irqs_index)) {
            irq = irqs[get_index_new(irqs_index)] - irqs[get_index_old(irqs_index)];
        } else {
            irq = 0;
        }
        ++irqs_index;
        printf(",%ld", irq);

        while (fgetc(f) != '\n'); // read until meets '\n', nextline

        fscanf(f, "ctxt %ld", &ctxts[get_index_new(ctxts_index)]);
        if (get_index_old(ctxts_index)) {
            ctxt = ctxts[get_index_new(ctxts_index)] - ctxts[get_index_old(ctxts_index)];
        } else {
            ctxt = 0;
        }
        ++ctxts_index;
        printf(",%ld", ctxt);

        while (fgetc(f) != '\n'); // read until meets '\n', nextline
        while (fgetc(f) != '\n'); // read until meets '\n', nextline

        fscanf(f, "processes %ld", &value);
        printf(",%ld", value);

        while (fgetc(f) != '\n'); // read until meets '\n', nextline

        fscanf(f, "procs_running %ld", &value);
        printf(",%ld", value);

        while (fgetc(f) != '\n'); // read until meets '\n', nextline

        fscanf(f, "procs_blocked %ld", &value);
        printf(",%ld", value);

        fclose(f);
    } else if (flag & PRINT_HEAD) {
        printf("%s", p);
    }
    return 0;
}

/**
 * Header contents
second,cpu-cpu_percent[0],cpu-iowtime[1],cpu-cpu_frequency[2],cpu-null[3],cpu-null[4],cpu-null[5],cpu0-cpu_percent[0],cpu0-iowtime[1],cpu0-cpu_frequency[2],cpu0-null[3],cpu0-null[4],cpu0-null[5],cpu1-cpu_percent[0],cpu1-iowtime[1],cpu1-cpu_frequency[2],cpu1-null[3],cpu1-null[4],cpu1-null[5],cpu2-cpu_percent[0],cpu2-iowtime[1],cpu2-cpu_frequency[2],cpu2-null[3],cpu2-null[4],cpu2-null[5],cpu3-cpu_percent[0],cpu3-iowtime[1],cpu3-cpu_frequency[2],cpu3-null[3],cpu3-null[4],cpu3-null[5],cpu4-cpu_percent[0],cpu4-iowtime[1],cpu4-cpu_frequency[2],cpu4-null[3],cpu4-null[4],cpu4-null[5],cpu5-cpu_percent[0],cpu5-iowtime[1],cpu5-cpu_frequency[2],cpu5-null[3],cpu5-null[4],cpu5-null[5],cpu6-cpu_percent[0],cpu6-iowtime[1],cpu6-cpu_frequency[2],cpu6-null[3],cpu6-null[4],cpu6-null[5],cpu7-cpu_percent[0],cpu7-iowtime[1],cpu7-cpu_frequency[2],cpu7-null[3],cpu7-null[4],cpu7-null[5],irqs,ctxt,processes,procs_running,procs_blocked,totalram,freeram,cached,Reserve01,Reserve02,Reserve03,Reserve04,Reserve05,Reserve06
*/
static struct log_output basic_log_output[] = {
    {
        .print_log = log_proc_stat,
        .p = "second,"\
            INFO_PER_CPU(cpu) \
            INFO_PER_CPU(cpu0) \
            INFO_PER_CPU(cpu1) \
            INFO_PER_CPU(cpu2) \
            INFO_PER_CPU(cpu3) \
            INFO_PER_CPU(cpu4) \
            INFO_PER_CPU(cpu5) \
            INFO_PER_CPU(cpu6) \
            INFO_PER_CPU(cpu7) \
            "irqs,ctxt,processes,procs_running,procs_blocked"
            ,
    }, {
        .print_log = log_proc_meminfo,
        .p = "totalram,freeram,cached,Reserve01,Reserve02,Reserve03,Reserve04,Reserve05,Reserve06"
    }, {
        .print_log = NULL,
    }
};

static void cpu_sig_handler(int sig_num) {
    (int)sig_num;
    is_cpu_basic_quit = 1;
}

static void cpu_sig_killchld(int sig_num) {
    pid_t chld;
    (int)sig_num;

    chld = wait(NULL);
    is_cpu_basic_quit = 1;
}

static int parse_cmdline(int argc, char **argv) {
    int c;
    int i;
    pid_t pid;
    int ret = 0;
    FILE *p = NULL;

    output = basic_log_output;
    while ((c = getopt(argc, argv, "p:c:t:hfz")) != -1) {
        switch (c) {
            case 'p':
                pid_table[0] = strtol(optarg, NULL, 10);
                per_pid = 1;
                break;
            case 'c':
                pid = fork();
                if (pid == 0) {
                    close(1);
                    close(2);
                    strtok(optarg, " ");
                    ret = execlp(optarg, optarg, NULL);
                    if (ret == -1) {
                        printf("command exec error!!\n");
                        return -1;
                    }
                } else {
                    pid_table[0] = pid;
                    per_pid = 1;
                }
                break;
            case 't':
                interval = strtoul(optarg, NULL, 10);
                break;
            case 'h':
                output = NULL;
                printf(
                    "\nAll parameters are optional:\n"
                    "-p process_id    \n"
                    "-c task_name    \n"
                    "-h this help page\n"
                    "If there is not parameters, sgm will show the whole system info.\n\n"
                );
                ret = -1;
                break;
            case 'f':
                tag_on = true;
                strtok(argv[0], "6");
                sprintf(title, "%s %s", strchr(argv[0], 's'), "second,"\
                            INFO_PER_CPU(cpu) \
                            INFO_PER_CPU(cpu0) \
                            INFO_PER_CPU(cpu1) \
                            INFO_PER_CPU(cpu2) \
                            INFO_PER_CPU(cpu3) \
                            INFO_PER_CPU(cpu4) \
                            INFO_PER_CPU(cpu5) \
                            INFO_PER_CPU(cpu6) \
                            INFO_PER_CPU(cpu7) \
                            "irqs,ctxt,processes,procs_running,procs_blocked");
                basic_log_output[0].p = title;
                basic_log_output[0].private = strchr(argv[0], 's');
                break;
            case 'z':
                no_header = true;
                break;
        }
    }
    return ret;
}

int main(int argc, char *argv[]) {
    int ret;
    int FD;
    struct tm *cur;
    struct timespec tp;
    struct timeval tv;
    char cur_time[33];

    ret = parse_cmdline(argc, argv);
    if (ret < 0)
        return -1;
    setbuf(stdout, NULL);
    setbuf(stderr, NULL);

    if (no_header == false)
        logs_print(0, PRINT_HEAD, output);
    signal(SIGTERM, cpu_sig_handler);
    signal(SIGCHLD, cpu_sig_killchld);

    while (!is_cpu_basic_quit) {
        gettimeofday(&tv, NULL);
        cur = localtime(&tv.tv_sec);
        clock_gettime(CLOCK_MONOTONIC_RAW, &tp);
        sprintf(cur_time, "%d-%d %d:%d:%d.%03ld<%5lu.%06lu>", cur->tm_mon + 1, cur->tm_mday, cur->tm_hour, cur->tm_min, cur->tm_sec, tv.tv_usec / 1000, tp.tv_sec, tp.tv_nsec / 1000);

        logs_print(cur_time, PRINT_BODY, output);
        msleep(interval);
    }
    return 0;
}
