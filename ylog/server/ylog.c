/**
 * Copyright (C) 2016 Spreadtrum Communications Inc.
 */

#include "ylog.h"
#include "popen2.c"
#include "common.c"
#include "cacheline.c"
#include "socket.c"
#include "filter.c"
#include "inotify.c"
#include "file.c"
#include "parser.c"
#include "command.c"
#ifdef ANDROID
#include "os/android.c"
#else
#include "os/ubuntu.c"
#endif
#include "verify.c"

struct os_hooks os_hooks;
static int fd_command_server;

#include <signal.h>
static sigset_t signals_handled;
//static pthread_mutex_t mutex_sig = PTHREAD_MUTEX_INITIALIZER;
static void *ylog_sighandler_thread_default(void *arg) {
    int *processing_signal = arg;
    ylog_info("ylog_sighandler_thread_default starts to run\n");
    if (global_context->ignore_signal_process == 0/**processing_signal == SIGPWR*/) {
        ylog_all_thread_exit();
        CLOSE(fd_command_server);
        print2journal_file("ylog.stop with signal %d, %s, sdcard is %s", *processing_signal, strsignal(*processing_signal), os_check_sdcard_online(NULL, 0) ? "online" : "offline");
        #if 0
        exit(0); /* sometime it will pending there long time, don't know why */
        #else
        kill(getpid(), SIGKILL);
        #endif
    }
    ylog_info("ylog_sighandler_thread_default finshes running\n");
    *processing_signal = -1;
    return NULL;
}

static void ylog_terminate_signal_handler(int sig) {
    pthread_t ptid;
    static int processing_signal = -1;

    //pthread_mutex_lock(&mutex_sig); /* signal process can't re-entrant, no need to protect */
    if (processing_signal >= 0) {
        ylog_info("ylog_terminate_signal_handler is processing %s, ignore %s\n",
                strsignal(processing_signal), strsignal(sig));
        //pthread_mutex_unlock(&mutex_sig);
        return;
    }

    processing_signal = sig;
    //pthread_mutex_unlock(&mutex_sig);

    ylog_info("ylog_terminate_signal_handler get signal %s\n", strsignal(processing_signal));

    /**
     * For the signal mechanism, the signal can happen at any time, when any thread is running
     * then that means it can interrupt the code at any place,
     * even worse after you call pthread_mutex_lock to get the mutex, the running code will be
     * stopped and return to userspace signal_handler to process this signal until
     * the signal_handler return to kernel then to the running code again
     *
     * as for the effect for the running code, when the signal handler return
     * it does not go on the sleeping, but will be woken up, like signal SIGINT
     *
     * hook_signals();
     * printf("sleep 10000\n");
     * sleep(10000);
     * while (1) {
     *         printf("xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\n");
     *         sleep(1);
     * }
     *
     * if you press ctrl+c, then sleep(10000); will be interrupted, signal_handler will be called
     * when signal_handler return, sleep(10000); will be skiped, the next line while (1) will be executed.
     * the sleeping thread will become awake, so signal will change some code behaviour, but not fatal,
     * anyway, we need to return this signal handler ASAP, to let the userspace code go on normal work
     * then, a new processing signal thread is created here by luther 2016.01.29
     */
    pthread_create(&ptid, NULL, ylog_sighandler_thread_default, &processing_signal);
}

/*
 * setup_signals - initialize signal handling.
 */
static void hook_signals(void) {
    struct sigaction sa;
    /*
     * Compute mask of all interesting signals and install signal handlers
     * for each.  Only one signal handler may be active at a time.  Therefore,
     * all other signals should be masked when any handler is executing.
     */
    sigemptyset(&signals_handled);

#define SIGNAL(s, handler) do { \
        sigaddset(&signals_handled, s); \
        sa.sa_handler = handler; \
        if (sigaction(s, &sa, NULL) < 0) \
        {}/* ylog_critical("Couldn't establish signal handler (%d): %m", s); */ \
    } while (0)

    sa.sa_mask = signals_handled;
    sa.sa_flags = 0;
    SIGNAL(SIGINT, ylog_terminate_signal_handler);    /* #define SIGINT 2 */
    SIGNAL(SIGTERM, ylog_terminate_signal_handler);    /* #define SIGTERM 15 */
#if 1
    SIGNAL(SIGQUIT, ylog_terminate_signal_handler); /* #define SIGQUIT 3 */
    SIGNAL(SIGILL, ylog_terminate_signal_handler); /* #define SIGILL 4 */
    SIGNAL(SIGABRT, ylog_terminate_signal_handler); /* #define SIGABRT 6 */
    SIGNAL(SIGKILL, ylog_terminate_signal_handler);    /* Kill */ /* we can't capture SIGKILL signal */
    /* we remove this then kill -11 can get tombstone under /data/tombstones/ */
    // SIGNAL(SIGSEGV, ylog_terminate_signal_handler); /* #define SIGSEGV 11 */
    SIGNAL(SIGPIPE, ylog_terminate_signal_handler); /* #define SIGPIPE 13 */
    SIGNAL(SIGPWR, ylog_terminate_signal_handler);    /* #define SIGPWR 30 */
#endif
}

#ifdef HAVE_YLOG_INFO
static int ylog_read_info(char *buf, int count, FILE *fp, int fd, struct ylog *y) {
    static int cnt = 0;
    int i;
    struct ylog *yi;
    struct ydst *yd;
    struct ylog_event_thread *e;
    pid_t ctid;
    ylog_write_handler w = y->write_handler;
    char *cmd_list[] = {
        "cat /proc/cmdline",
        "cat /proc/version",
        "cat /proc/meminfo",
        "cat /proc/mounts",
        "cat /proc/partitions",
        "cat /proc/diskstats",
        "cat /proc/modules",
        "cat /proc/cpuinfo",
        "ls -l /",
        NULL
    };

    if (fd == yp_fd(YLOG_POLL_INDEX_PIPE, &y->yp))
        return y->read(buf, count, fp, fd, y);

    pcmds(cmd_list, &cnt, w, y, "ylog_info", -1, NULL);

    if (os_hooks.ylog_read_info_hook)
        os_hooks.ylog_read_info_hook(buf, count, fp, cnt, y);

    for_each_ylog(i, yi, NULL) {
        if (yi->name == NULL)
            continue;
        ctid = yi->ydst->cache ? yi->ydst->cache->tid : -1;
        w(buf, snprintf(buf, count, "[ylog] %s pid=%d, tid=%d, cache tid=%d\n", yi->name, yi->pid, yi->tid, ctid), y, NULL);
    }

    for_each_event_thread_start(e)
    w(buf, snprintf(buf, count, "[event] %s pid=%d, tid=%d\n", e->yewait.name, e->pid, e->tid), y, NULL);
    for_each_event_thread_end();

    return 0; /* INT_MAX; */
}
#endif

#ifdef HAVE_YLOG_JOURNAL
static int ylog_read_journal(char *buf, int count, FILE *fp, int fd, struct ylog *y) {
    ylog_write_handler w = y->write_handler;
    if (fd == yp_fd(YLOG_POLL_INDEX_PIPE, &y->yp))
        return y->read(buf, count, fp, fd, y);
    do {
        count = get_journal_file(buf, count);
        if (count <= 0)
            break;
        w(buf, count, y, NULL);
    } while (1);
    return 0; /* INT_MAX; */
}
#endif

static int ylog_read_ylog_debug(char *buf, int count, FILE *fp, int fd, struct ylog *y) {
    static int cnt = 0;
    ylog_write_handler w = y->write_handler;
    float transfered_size_float, max_size_float;
    char max_unit, unit;
    struct timespec ts;
    struct tm delta_tm;
    time_t delta_t;
    struct ydst *yd;
    char *p, *pmax = buf + count;
    int i;
    int ret;
    char *cmd_list[] = {
        "uptime",
        NULL
    };

    if (fd == yp_fd(YLOG_POLL_INDEX_PIPE, &y->yp))
        return y->read(buf, count, fp, fd, y);

    get_boottime(&ts);
    delta_t = ts.tv_sec - global_context->ts.tv_sec;
    gmtime_r(&delta_t, &delta_tm); /* UTC, don't support keep running more than 1 year by luther */
    w(buf, snprintf(buf, count,
"[ ylog has runned %02d day %02d:%02d:%02d ]\n",
    delta_tm.tm_yday,
    delta_tm.tm_hour,
    delta_tm.tm_min,
    delta_tm.tm_sec), y, NULL);

    pcmds(cmd_list, &cnt, y->write_handler, y, "ylog_debug", -1, NULL);

    if (os_hooks.ylog_read_ylog_debug_hook)
        os_hooks.ylog_read_ylog_debug_hook(buf, count, fp, cnt, y);

    return 0; /* INT_MAX; */
}

static struct context context_default = {
    .model = C_FULL_LOG,
    .loglevel = LOG_DEBUG,
};

static struct ydst_root ydst_root_default = {
    .root = NULL,
};

struct cacheline cacheline_default = {
    .size = 512 * 1024,
    .num = 16,
    .timeout = 1000, /* ms */
    .debuglevel = CACHELINE_DEBUG_INFO | CACHELINE_DEBUG_CRITICAL | CACHELINE_DEBUG_WCLIDX_WRAP,
};

struct cacheline cacheline_socket_open = {
    .size = 512 * 1024,
    .num = 16,
    .timeout = 1000, /* ms */
    .debuglevel = CACHELINE_DEBUG_INFO | CACHELINE_DEBUG_CRITICAL | CACHELINE_DEBUG_WCLIDX_WRAP,
};

/* Assume max size is 1G */
/* ylog_cli quota 1024 */
static struct ydst ydst_default[YDST_MAX+1] = {
    [YDST_TYPE_DEFAULT] = {
        .file = "default/default.", /* by default .file == NULL, then stdout will be used */
        .file_name = "default.log",
        .max_segment = 2,
        .max_segment_size = 2*1024*1024,
        .cache = &cacheline_default,
    },
    [YDST_TYPE_SOCKET] = {
        .file = "socket/open/",
        .file_name = "socket.log",
        .max_segment = 2,
        .max_segment_size = YDST_TYPE_SOCKET_DEFAULT_SIZE,
        .cache = &cacheline_socket_open,
        .write_data2cache_first = 1,
        /* .nowrap = 1, */
    },
    [YDST_TYPE_YLOG_DEBUG] = {
        .file = "ylog_debug",
        .file_name = "ylog_debug.log",
        .max_segment = 1,
        .max_segment_size = 30*1024*1024,
    },
#ifdef HAVE_YLOG_INFO
    [YDST_TYPE_INFO] = {
        .file = "info",
        .file_name = "info.log",
        .max_segment = 1,
        .max_segment_size = 20*1024*1024,
    },
#endif
#ifdef HAVE_YLOG_JOURNAL
    [YDST_TYPE_JOURNAL] = {
        .file = "ylog_journal_file",
        .file_name = "ylog_journal_file.log",
        .max_segment = 1,
        .max_segment_size = 20*1024*1024,
    },
#endif
};

static struct ylog ylog_default[YLOG_MAX+1] = {
    {
        .name = "benchmark_socket",
        .type = FILE_POPEN,
        .file = "ylog_benchmark_socket_server",
        .ydst = &ydst_default[YDST_TYPE_DEFAULT],
        .mode = YLOG_READ_MODE_BLOCK | YLOG_READ_MODE_BINARY | YLOG_READ_LEN_MIGHT_ZERO,
        .raw_data = 1,
    },
    {
        .name = "socket",
        .type = FILE_SOCKET_LOCAL_SERVER,
        .file = "ylog_socket",
        .ydst = &ydst_default[YDST_TYPE_SOCKET],
        .mode = YLOG_READ_MODE_BLOCK | YLOG_READ_MODE_BINARY | YLOG_READ_LEN_MIGHT_ZERO,
        .raw_data = 1,
    },
    {
        .name = "ylog_debug",
        .type = FILE_NORMAL,
        .file = "/dev/null",
        .ydst = &ydst_default[YDST_TYPE_YLOG_DEBUG],
        .restart_period = 1000 * 60 * 20,
        .fread = ylog_read_ylog_debug,
    },
#ifdef HAVE_YLOG_INFO
    {
        .name = "info",
        .type = FILE_NORMAL,
        .file = "/dev/null",
        .ydst = &ydst_default[YDST_TYPE_INFO],
        .fread = ylog_read_info,
    },
#endif
#ifdef HAVE_YLOG_JOURNAL
    {
        .name = "journal",
        .type = FILE_NORMAL,
        .file = "/dev/null",
        .ydst = &ydst_default[YDST_TYPE_JOURNAL],
        .restart_period = 1000 * 60,
        .fread = ylog_read_journal,
    },
#endif
};

static int speed_statistics_event_timer_handler(void *arg, long tick, struct ylog_event_cond_wait *yewait) {
    UNUSED(arg);
    UNUSED(tick);
    UNUSED(yewait);
    static unsigned long long prev_size = 0;
    static struct timespec prev_ts;
    static struct timeval prev_tv;
    static unsigned long long prev_size_ylog[YLOG_MAX];
    static unsigned long long prev_size_ydst[YDST_MAX];
    struct timespec cur_ts;
    struct timeval cur_tv;
    int i;
    struct ylog *y;
    struct ydst *yd;
    unsigned long long cur_size = ydst_all_transfered_size();
    struct ydst_root *root = global_ydst_root;
    int delta_millisecond;
    unsigned long delta_size;
    int wrap = 0;
    if (prev_size > cur_size) { /* wrap up should happen, maybe ylog_cli ryloga or maybe really overrun */
        prev_size = cur_size;
        wrap = 1;
    }
    gettimeofday(&cur_tv, NULL);
    get_boottime(&cur_ts);
    delta_millisecond = diff_ts_millisecond(&prev_ts, &cur_ts);
    for_each_ylog(i, y, NULL) {
        if (y->name == NULL)
            continue;
        if (wrap)
            prev_size_ylog[i] = y->size;
        if (prev_size)
            insert_new_speed(y->size - prev_size_ylog[i], delta_millisecond,
                    &prev_tv, &cur_tv, &cur_ts, y->speed, YLOG_SPEED_NUM);
        prev_size_ylog[i] = y->size;
    }
    for_each_ydst(i, yd, NULL) {
        if (yd->file == NULL || yd->refs <= 0)
            continue;
        if (wrap)
            prev_size_ydst[i] = yd->size;
        if (prev_size)
            insert_new_speed(yd->size - prev_size_ydst[i], delta_millisecond,
                    &prev_tv, &cur_tv, &cur_ts, yd->speed, YDST_SPEED_NUM);
        prev_size_ydst[i] = yd->size;
    }
    if (prev_size) {
        float cur_speed_float;
        char cur_speed_unit;
        delta_size = cur_size - prev_size;
        if (global_context->loglevel >= LOG_DEBUG)
            cur_speed_float = ylog_get_unit_size_float_with_speed(delta_size, &cur_speed_unit, delta_millisecond);
        else {
            cur_speed_float = 0;
            cur_speed_unit = 'B';
        }
        if (insert_new_speed(delta_size, delta_millisecond, &prev_tv, &cur_tv, &cur_ts, root->speed, YDST_ROOT_SPEED_NUM) == 0)
            ylog_debug("ydst write speed %.2f%c/s new max\n", cur_speed_float, cur_speed_unit);
        ylog_debug("ydst write speed %.2f%c/s\n", cur_speed_float, cur_speed_unit);
    }
    prev_size = cur_size;
    prev_ts = cur_ts;
    prev_tv = cur_tv;
    return 0;
}

static void pthread_create_hook_default(struct ylog *y, void *args, const char *fmt, ...) {
    UNUSED(y);
    UNUSED(args);
    UNUSED(fmt);
}

static void *ylog_command_loop(void *arg) {
    UNUSED(arg);
    os_hooks.pthread_create_hook(NULL, NULL, "ylog_command_loop");
    command_loop(fd_command_server);
    return NULL;
}

struct context *global_context = &context_default;
struct ydst *global_ydst = ydst_default;
struct ylog *global_ylog = ylog_default;
struct ydst_root *global_ydst_root = &ydst_root_default;

int main(int argc, char *argv[]) {
    UNUSED(argc);
    UNUSED(argv);
    pthread_t ptid;
    os_init(global_ydst_root, &global_context, &os_hooks);
    if (os_hooks.pthread_create_hook == NULL)
        os_hooks.pthread_create_hook = pthread_create_hook_default;
    os_env_prepare();
    if (create_socket_local_server(&fd_command_server, "ylog_cli")) {
        print2journal_file_string_with_uptime("ylog.start failed, ylog_cli socket create failed!");
        return -1; /* To avoid run ylog twice */
    }
    print2journal_file_string_with_uptime("ylog_cli socket done");
    ylog_init(global_ydst_root, global_context);
    hook_signals();
    pthread_create(&ptid, NULL, ylog_command_loop, NULL);
    ylog_ready();
    ylog_os_event_timer_create("speed", 1000, speed_statistics_event_timer_handler, (void*)-1);
    ylog_verify();
    if (os_hooks.ready_go)
        os_hooks.ready_go();
    print2journal_file_string_with_uptime("ylog.start success");
    pthread_join(ptid, NULL);
    return 0;
}
