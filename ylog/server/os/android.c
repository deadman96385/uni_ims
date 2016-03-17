/**
 * Copyright (C) 2016 Spreadtrum Communications Inc.
 */

#include <cutils/sched_policy.h>
#include <private/android_filesystem_config.h>
#include <utils/threads.h>
#include <sys/prctl.h>
#include <sys/capability.h>
#include <cutils/properties.h>

#define YLOG_ROOT_FOLDER "/data"
#define YLOG_JOURNAL_FILE "/data/ylog/ylog_journal_file"
#define YLOG_CONFIG        "/data/ylog/ylog.conf"
#define YLOG_FILTER_PATH "/system/lib/"

struct os_status {
    int sdcard_online;
    char ylog_root_path_latest[512];
    char historical_folder_root_last[512];
    int anr_fast;
} oss, *pos = &oss;

struct os_config {
#define KEEP_HISTORICAL_FOLDER_NUMBERS_DEFUALT 5
    int keep_historical_folder_numbers;
} oc, *poc = &oc;

static int ylog_read_info_hook(char *buf, int count, FILE *fp, int fd, struct ylog *y) {
    FILE *wfp;
    char tmp[PATH_MAX];
    char *cmd_list[] = {
        "ls -l /dev/block/platform/*/by-name/",
        "ls -l /dev/",
        "cat /default.prop",
        "getprop",
        "cat /data/ylog/ylog.conf",
        NULL
    };

    wfp = popen("ls /*.rc", "r");
    do {
        if (fgets(buf, count, wfp) == NULL)
            break;
        snprintf(tmp, sizeof tmp, "cat %s", strtok(buf, "\n"));
        pcmd(tmp, &fd, y->write_handler, y, "ylog_info");
    } while (1);
    pclose(wfp);

    pcmds(cmd_list, &fd, y->write_handler, y, "ylog_info");

    return 0;
    if (0) { /* avoid compiler warning */
        fp = fp;
    }
}

static int ylog_read_ylog_debug_hook(char *buf, int count, FILE *fp, int fd, struct ylog *y) {
    char *cmd_list[] = {
        "/system/bin/ylog_cli ylog",
        "/system/bin/ylog_cli speed",
        "/system/bin/ylog_cli space",
        "getprop ylog.killed",
        NULL
    };
    pcmds(cmd_list, &fd, y->write_handler, y, "ylog_debug");
    return 0;
    if (0) { /* avoid compiler warning */
        buf = buf;
        count = count;
        fp = fp;
    }
}

static int ylog_read_sys_info(char *buf, int count, FILE *fp, int fd, struct ylog *y) {
    int cnt = 0;
    char *cmd_list[] = {
        "cat /proc/slabinfo",
        "cat /proc/buddyinfo",
        "cat /proc/zoneinfo",
        "cat /proc/vmstat",
        "cat /proc/vmallocinfo",
        "cat /proc/pagetypeinfo",
        "cat /sys/module/lowmemorykiller/parameters/adj",
        "cat /sys/module/lowmemorykiller/parameters/minfree",
        "cat /proc/wakelocks",
        "cat /d/wakeup_sources",
        "cat /sys/class/backlight/sprd_backlight/brightness",
        "cat /sys/kernel/debug/binder/failed_transaction_log",
        "cat /sys/kernel/debug/binder/transaction_log",
        "cat /sys/kernel/debug/binder/transactions",
        "cat /sys/kernel/debug/binder/stats",
        "cat /sys/kernel/debug/binder/state",
        "cat /sys/kernel/debug/sprd_debug/cpu/cpu_usage",
        NULL
    };
    pcmds(cmd_list, &cnt, y->write_handler, y, "sys_info");
    return 0;
    if (0) { /* avoid compiler warning */
        buf = buf;
        count = count;
        fp = fp;
        fd = fd;
    }
}

static int ylog_read_sys_info_manual(char *buf, int count, FILE *fp, int fd, struct ylog *y) {
    int cnt = 0;
    char *cmd_list[] = {
        "busybox netstat -ap",
        "ps -t",
        NULL
    };
    ylog_read_sys_info(buf, count, fp, fd, y);
    pcmds(cmd_list, &cnt, y->write_handler, y, "sys_info_manual");
    return 0;
}

/**
 * system/core/logd/main.cpp
 */
static int drop_privs() {
    struct sched_param param;
    memset(&param, 0, sizeof(param));

    if (set_sched_policy(0, SP_BACKGROUND) < 0) {
        return -1;
    }

    if (sched_setscheduler((pid_t) 0, SCHED_BATCH, &param) < 0) {
        return -1;
    }

    if (setpriority(PRIO_PROCESS, 0, ANDROID_PRIORITY_BACKGROUND) < 0) {
        return -1;
    }

    if (prctl(PR_SET_KEEPCAPS, 1) < 0) {
        return -1;
    }

    if (setgroups(0, NULL) == -1) {
        return -1;
    }

    if (setgid(AID_SYSTEM) != 0) {
        return -1;
    }

    if (setuid(AID_SYSTEM) != 0) {
        return -1;
    }

    struct __user_cap_header_struct capheader;
    struct __user_cap_data_struct capdata[2];
    memset(&capheader, 0, sizeof(capheader));
    memset(&capdata, 0, sizeof(capdata));
    capheader.version = _LINUX_CAPABILITY_VERSION_3;
    capheader.pid = 0;

    capdata[CAP_TO_INDEX(CAP_SYSLOG)].permitted = CAP_TO_MASK(CAP_SYSLOG);
    capdata[CAP_TO_INDEX(CAP_AUDIT_CONTROL)].permitted |= CAP_TO_MASK(CAP_AUDIT_CONTROL);

    capdata[0].effective = capdata[0].permitted;
    capdata[1].effective = capdata[1].permitted;
    capdata[0].inheritable = 0;
    capdata[1].inheritable = 0;

    if (capset(&capheader, &capdata[0]) < 0) {
        return -1;
    }

    return 0;
}

static int os_check_sdcard_online(void) {
    char sdcard_status[PROPERTY_VALUE_MAX];
    property_get("vold.sdcard0.state", sdcard_status, "unmounted");
    return !strncmp(sdcard_status, "mounted", 7);
}

static int os_search_root_path(char *path, int len) {
    struct ydst_root *root = global_ydst_root;
    struct context *c = global_context;
    char historical_folder_root_tmp[512];
    int changed = 0;
    /* Should detect sdcard mount status, then fill the path */
    char sdcard_path[PATH_MAX];
    int keep_historical_folder_numbers = 0;
    char *historical_folder_root = NULL;
    unsigned long long quota = 0;

    if (os_check_sdcard_online()) {
        property_get("vold.sdcard0.path", sdcard_path, "/storage/sdcard0");
        keep_historical_folder_numbers = poc->keep_historical_folder_numbers;
        historical_folder_root = pos->historical_folder_root_last;
        pos->sdcard_online = 1;
    } else {
        strcpy(sdcard_path, YLOG_ROOT_FOLDER);
        keep_historical_folder_numbers = 0;
        historical_folder_root = NULL;
        quota = 200 * 1024 * 1024;
        if (pos->sdcard_online) {
            ylog_critical("sdcard is removed, exit ylog forcely\n");
            kill(getpid(), SIGTERM);
            while (1) { sleep(1); }
        }
    }

    if (strcmp(sdcard_path, pos->ylog_root_path_latest)) {
        strcpy(pos->ylog_root_path_latest, sdcard_path);
        snprintf(path, len, "%s/%s/%s", pos->ylog_root_path_latest, "ylog", "ylog");
        snprintf(historical_folder_root_tmp, sizeof historical_folder_root_tmp,
                "%s/%s/%s", pos->ylog_root_path_latest, "ylog", "last_ylog");
        if (historical_folder_root)
            strcpy(historical_folder_root, historical_folder_root_tmp);
        else
            rm_all(historical_folder_root_tmp);
        if (quota == 0) {
            /**
             * calculate the new root folder usable space
             * you can use following function to free the last_ylog log
             * then we will have more space can be used if the new root folder space
             * is in short supply very very.
             *
             * ylog_root_folder_delete(path, historical_folder_root_tmp, 1, 2);
             */
            // ylog_root_folder_delete(path, historical_folder_root_tmp, 1, 2);
            quota = calculate_path_disk_available(sdcard_path) * 0.9;
        }
        c->keep_historical_folder_numbers = keep_historical_folder_numbers;
        c->historical_folder_root = historical_folder_root;
        root->quota_new = quota;

        if (quota < 50 * 1024 * 1024)
            ylog_root_folder_delete(path, historical_folder_root_tmp, poc->keep_historical_folder_numbers, poc->keep_historical_folder_numbers);

        changed = 1;
    }

    return changed;
}

static char *os_detect_root_folder(char *root) {
    char path[PATH_MAX];
    os_search_root_path(path, sizeof(path));
    if (root)
        strcpy(root, path);
    else
        return strdup(path); /* Remember to call free */
    return root;
}

static int event_timer_handler(void *arg, long tick, struct ylog_event_cond_wait *yewait) {
    char log_path[PATH_MAX];

    if (os_search_root_path(log_path, sizeof(log_path))) {
        struct ydst_root *root = global_ydst_root;
        if (pos->sdcard_online)
            yewait->period = 5*1000;
        else
            yewait->period = 1*1000;
        ylog_info("new root = %s, tick=%ld, yewait->peroid=%dms\n", log_path, tick, yewait->period);
        if (root->quota_new != root->quota_now) {
            float fsize1, fsize2;
            char unit1, unit2;
            fsize1 = ylog_get_unit_size_float(root->quota_now ? root->quota_now : root->max_size, &unit1);
            fsize2 = ylog_get_unit_size_float(root->quota_new, &unit2);
            ylog_info("new quota from %.2f%c to %.2f%c\n", fsize1, unit1, fsize2, unit2);
            if (root->quota_new < root->quota_now) {
                ydst_root_quota(NULL, root->quota_new); /* shrink first */
                ydst_root_new(NULL, log_path);
            } else {
                ydst_root_new(NULL, log_path); /* move first */
                ydst_root_quota(NULL, root->quota_new);
            }
        } else {
            ydst_root_new(NULL, log_path);
        }
    }

    return 0;
    if (0) { /* avoid compiler warning */
        arg = arg;
    }
}

#define KERNEL_NOTIFY_MODE  0
#if KERNEL_NOTIFY_MODE == 1
static int ydst_fwrite_kernel(char *buf, int count, int fd) {
    int ret = 0;
    if (count) {
        ret = fd_write(buf, count, fd);
        kernel_notify(buf, count);
    }
    return ret;
}
#else
static int ydst_write_kernel(char *id_token, int id_token_len,
        char *buf, int count, struct ydst *ydst) {
    int ret = 0;
    if (id_token_len) {
        ret += ydst_write_default(id_token, id_token_len, ydst);
        kernel_notify(id_token, id_token_len);
    }
    if (count) {
        ret += ydst_write_default(buf, count, ydst);
        kernel_notify(buf, count);
    }
    return ret;
}
#endif

static int cmd_test(struct command *cmd, char *buf, int buf_size, int fd, int index, struct ylog_poll *yp) {
    buf[ylog_get_format_time(buf)] = '\n';
    send(fd, buf, strlen(buf), MSG_NOSIGNAL);
    return 0;
    if (0) { /* avoid compiler warning */
        cmd = cmd;
        buf_size = buf_size;
        index = index;
        yp = yp;
    }
}

static int cmd_rootdir(struct command *cmd, char *buf, int buf_size, int fd, int index, struct ylog_poll *yp) {
    send(fd, buf, snprintf(buf, buf_size, "%s\n", pos->ylog_root_path_latest), MSG_NOSIGNAL);
    return 0;
    if (0) { /* avoid compiler warning */
        cmd = cmd;
        index = index;
        yp = yp;
    }
}

static int cmd_cpath_last(struct command *cmd, char *buf, int buf_size, int fd, int index, struct ylog_poll *yp) {
    send(fd, buf, snprintf(buf, buf_size, "%s\n", pos->historical_folder_root_last), MSG_NOSIGNAL);
    return 0;
    if (0) { /* avoid compiler warning */
        cmd = cmd;
        index = index;
        yp = yp;
    }
}

static void ylog_update_config2(char *key, char *value);
static int cmd_history_n(struct command *cmd, char *buf, int buf_size, int fd, int index, struct ylog_poll *yp) {
    char *last;
    char *value = NULL;
    char *level;
    struct context *c = global_context;
    strtok_r(buf, " ", &last);
    value = strtok_r(NULL, " ", &last);
    if (value) {
        int history_n = strtol(value, NULL, 0);
        if (history_n == 0)
            history_n = KEEP_HISTORICAL_FOLDER_NUMBERS_DEFUALT;
        c->keep_historical_folder_numbers = poc->keep_historical_folder_numbers = history_n;
        snprintf(buf, buf_size, "%d", poc->keep_historical_folder_numbers);
        ylog_update_config2("keep_historical_folder_numbers", buf);
    }
    send(fd, buf, snprintf(buf, buf_size, "%d\n", c->keep_historical_folder_numbers), MSG_NOSIGNAL);
    return 0;
    if (0) { /* avoid compiler warning */
        cmd = cmd;
        index = index;
        yp = yp;
    }
}

static int cmd_setprop(struct command *cmd, char *buf, int buf_size, int fd, int index, struct ylog_poll *yp) {
    char *last;
    char *property = NULL;
    char *value = NULL;
    char *level;
    char pp[PROPERTY_VALUE_MAX];
    strtok_r(buf, " ", &last);
    property = strtok_r(NULL, " ", &last);
    value = strtok_r(NULL, " ", &last);
    if (value && property) {
        strcpy(pp, value);
        property_set(property, pp);
    }
    property_get(property, pp, "");
    send(fd, buf, snprintf(buf, buf_size, "%s %s\n", property, pp), MSG_NOSIGNAL);
    return 0;
    if (0) { /* avoid compiler warning */
        cmd = cmd;
        index = index;
        yp = yp;
    }
}

static struct command os_commands[] = {
    {"test", "test from android", cmd_test, NULL},
    {"\n", NULL, os_cmd_help, (void*)os_commands},
    {"rootdir", "get the log disk root dir", cmd_rootdir, NULL},
    {"cpath_last", "get the last_ylog path", cmd_cpath_last, NULL},
    {"history_n", "set keep_historical_folder_numbers", cmd_history_n, NULL},
    {"setprop", "set property, ex. ylog_cli setprop persist.ylog.enabled 1", cmd_setprop, NULL},
    {NULL, NULL, NULL, NULL}
};

static void load_keep_historical_folder_numbers(struct ylog_keyword *kw, int nargs, char **args) {
    int history_n = strtol(args[1], NULL, 0);
    if (history_n == 0)
        history_n = KEEP_HISTORICAL_FOLDER_NUMBERS_DEFUALT;
    poc->keep_historical_folder_numbers = history_n;
    if (0) { /* avoid compiler warning */
        kw = kw;
        nargs = nargs;
    }
}

static void cmd_ylog_hook(int nargs, char **args) {
    /**
     * args 0    1       2    3
     * 1. ylog enabled kernel 0
     * 2.
     */
    ylog_update_config(YLOG_CONFIG, nargs, args, nargs - 1);
}

static void load_ylog(struct ylog_keyword *kw, int nargs, char **args) {
    /**
     * args 0    1       2    3
     * 1. ylog enabled kernel 0
     * 2.
     */
    struct ylog *y;
    int v;
    char *key = (nargs > 1 ? args[1] : NULL);
    char *value = (nargs > 2 ? args[2] : NULL);
    char *svalue1 = (nargs > 3 ? args[3] : NULL);
    if (strcmp(key, "enabled") == 0) {
        if (value && svalue1) {
            y = ylog_get_by_name(value);
            if (y) {
                v = !!atoi(svalue1);
                if (v == 0) {
                    y->status |= YLOG_DISABLED_FORCED_RUNNING | YLOG_DISABLED;
                } else {
                    y->status &= ~YLOG_DISABLED;
                }
                ylog_info("ylog <%s> is %s forcely by ylog.conf\n",
                        y->name, (y->status & YLOG_DISABLED) ? "disabled":"enabled");
            } else {
                ylog_critical("%s: can't find ylog %s\n", __func__, value);
            }
        } else {
            ylog_critical("%s: value=%s, svalue1=%s\n", __func__, value, svalue1);
        }
    }
    if (0) { /* avoid compiler warning */
        kw = kw;
    }
}

static struct ylog_keyword ylog_keyword[] = {
    {"keep_historical_folder_numbers", load_keep_historical_folder_numbers},
    {"ylog", load_ylog},
    {NULL, NULL},
};

static void ylog_update_config2(char *key, char *value) {
    char *argv[2];
    argv[0] = key;
    argv[1] = value;
    ylog_update_config(YLOG_CONFIG, 2, argv, 1);
}

static void ylog_status_hook(enum ylog_thread_state state, struct ylog *y) {
    char *value;
    char buf[PROPERTY_VALUE_MAX];
    snprintf(buf, sizeof buf, "ylog.svc.%s", y->name);
    switch (state) {
    case YLOG_STOP: value = "stopped"; break;
    case YLOG_RUN: value = "running"; break;
    case YLOG_NOP: value = "waiting"; break;
    default: value = "unknown"; break;
    }
    ylog_info("ylog %s setprop %s %s\n", y->name, buf, value);
    property_set(buf, value);
}

static int os_inotify_handler_anr(struct ylog_inotify_cell *pcell, int timeout, struct ylog *y) {
    ylog_info("os_inotify_handler_anr is called for '%s %s' %dms %s now\n",
                pcell->pathname ? pcell->pathname:"", pcell->filename, pcell->timeout, timeout ? "timeout":"normal");
    static int cnt = 1;
    char *cmd_list[] = {
        "cat /data/anr/traces.txt",
        NULL
    };

    if (pos->anr_fast) {
        char buf[4];
        int fd = open("/data/anr/traces.txt", O_RDONLY);
        if (fd < 0) {
            ylog_info("open %s fail.%s", "/data/anr/traces.txt", strerror(errno));
            return 0;
        }
        lseek(fd, -3, SEEK_END);
        buf[0] = 0;
        buf[3] = 0;
        read(fd, buf, 3);
        if (strcmp(buf, "EOF"))
            return -1;
    }

    pcmds(cmd_list, &cnt, y->write_handler, y, "ylog_traces");

    return -1;
}

static int os_inotify_handler_crash(struct ylog_inotify_cell *pcell, int timeout, struct ylog *y) {
    ylog_info("os_inotify_handler_crash is called for '%s' %dms %s now\n",
                pcell->pathname ? pcell->pathname:"", pcell->timeout, timeout ? "timeout":"normal");
    static int cnt = 1;
    struct ylog_inotify_cell_args *pcella = pcell->args;
    struct ylog_inotify_files *file = &pcella->file;
    int i;
    char *a;
    char *cmd_list[30] = {
        "cat /data/tombstones/*",
        NULL
    };
    int cmd_start_idx = 0;

    for (i = 0; i < file->num; i++) {
        a = file->files_array + i * file->len;
        if (a[0]) {
            if (cmd_start_idx < (int)ARRAY_LEN(cmd_list))
                cmd_list[cmd_start_idx++] = a;
            ylog_info("os_inotify_handler_crash -> %s\n", a);
        }
    }

    pcmds(cmd_list, &cnt, y->write_handler, y, "ylog_tombstones");

    for (i = 0; i < file->num; i++) {
        a = file->files_array + i * file->len;
        a[0] = 0;
    }

    return -1;
}
static void ylog_ready(void) {
    struct ylog *y;
    char yk[PROPERTY_VALUE_MAX];
    int count;
    struct ydst_root *root = global_ydst_root;
    ylog_info("ylog_ready for android\n");

    property_get("ylog.killed", yk, "-1");
    count = atoi(yk);
    snprintf(yk, sizeof yk, "%d", count + 1);
    property_set("ylog.killed", yk);

    if (root->quota_new && root->quota_now != root->quota_new)
        ydst_root_quota(NULL, root->quota_new); /* all ylog threads are ready to run */

    y = ylog_get_by_name("info");
    if (y)
        ylog_trigger_and_wait_for_finish(y);

    if (drop_privs() < 0)
        ylog_error("ylog drop_privs failed: %s\n", strerror(errno));
    else
        ylog_critical("ylog drop_privs success\n");

    mkdirs("/data/anr/"); /* For ylog traces.txt */
    mkdirs("/data/tombstones/"); /* For ylog tombstone_00 ~ tombstone_09 */

    ylog_trigger_all(global_ylog); /* mark ylog status from ready to run */

    ylog_os_event_timer_create("android", pos->sdcard_online ? 5*1000:1*1000, event_timer_handler, NULL);
}

enum os_ydst_type {
    OS_YDST_TYPE_ANDROID,
    OS_YDST_TYPE_KERNEL,
    OS_YDST_TYPE_TRACER,
    OS_YDST_TYPE_TCPDUMP,
    OS_YDST_TYPE_HCIDUMP,
    OS_YDST_TYPE_TRACES,
    OS_YDST_TYPE_SYS_INFO,
    OS_YDST_TYPE_MAX
};

static struct cacheline os_cacheline[OS_YDST_TYPE_MAX] = {
    [OS_YDST_TYPE_ANDROID] = {
        .size = 512 * 1024,
        .num = 4,
        .timeout = 1000, /* ms */
        .debuglevel = CACHELINE_DEBUG_CRITICAL,
    },
    [OS_YDST_TYPE_KERNEL] = {
        .size = 512 * 1024,
        .num = 2,
        .timeout = 1000, /* ms */
        .debuglevel = CACHELINE_DEBUG_CRITICAL,
    },
    [OS_YDST_TYPE_TRACER] = {
        .size = 512 * 1024,
        .num = 2,
        .timeout = 1000, /* ms */
        .debuglevel = CACHELINE_DEBUG_CRITICAL,
    },
    [OS_YDST_TYPE_TCPDUMP] = {
        .size = 512 * 1024,
        .num = 2,
        .timeout = 1000, /* ms */
        .debuglevel = CACHELINE_DEBUG_CRITICAL,
    },
    [OS_YDST_TYPE_HCIDUMP] = {
        .size = 512 * 1024,
        .num = 2,
        .timeout = 1000, /* ms */
        .debuglevel = CACHELINE_DEBUG_CRITICAL,
    },
};

static struct context os_context[M_MODE_NUM] = {
    [M_USER] = {
        .config_file = "1.xml",
        .filter_so_path = YLOG_FILTER_PATH,
        .journal_file = YLOG_JOURNAL_FILE,
        .model = C_MINI_LOG,
        .loglevel = LOG_WARN,
    },
    [M_USER_DEBUG] = {
        .config_file = "2.xml",
        .filter_so_path = YLOG_FILTER_PATH,
        .journal_file = YLOG_JOURNAL_FILE,
        .model = C_FULL_LOG,
        .loglevel = LOG_INFO,
    },
};

static void os_parse_config(void) {
#if 0
    /* parse xml file */
    struct ylog *e;
    struct ylog y_xml_e = {
        .name = "xml",
        .type = FILE_POPEN,
        .file = "cat /sys/kernel/debug/sprd_debug/cpu/cpu_usage",
        .restart_period = 20*1000,
        .fp_array = NULL,
        .timestamp = 1,
    };
    e = &y_xml_e;

    if (ylog_insert(e) < 0) {
        ylog_info("Failed to insert %s, no space left\n", e->name);
    }
#endif
}

static void os_env_prepare(void) {
    /**
     * after popen2 is called, if stop ylog or kill -9 ylog
     * then logcat will become the zombie process
     * so you need to use kill -2 ylog to let ylog kill popen2 process by himself
     * if you did not do that, here will have a unabashed thinking,
     * all zombie process created by ylog popen2 will be killed in here by luther 2016.01.29
     */

}

static void os_init(struct ylog *ylog, struct ydst *ydst, struct ydst_root *root,
        struct context **c, struct os_hooks *hook) {
    struct ylog *y;
    int i;
    char *value;
    char buf[PROPERTY_VALUE_MAX];
    static char tombstone_files[10][128];
    static struct ylog_inotify_cell_args tombstone = {
        .type = YLOG_INOTIFY_CELL_TYPE_STORE_FILES,
        .name = "tombstone",
        .prefix = "cat /data/tombstones/",
        .suffix = NULL,
        .file = {
            .num = ARRAY_LEN(tombstone_files),
            .len = ARRAY_LEN(tombstone_files[0]),
            .files_array = (char*)tombstone_files,
        },
    };
    enum mode_types mode;
    /* Assume max size is 1G */
    struct ydst os_ydst[OS_YDST_TYPE_MAX + 1] = {
        [OS_YDST_TYPE_ANDROID] = {
            .file = "android/",
            .file_name = "android.log",
            .max_segment = 30,
            .max_segment_size = 50*1024*1024,
            .cache = &os_cacheline[OS_YDST_TYPE_ANDROID],
        },
        [OS_YDST_TYPE_KERNEL] = {
            .file = "kernel/",
            .file_name = "kernel.log",
            .max_segment = 10,
            .max_segment_size = 50*1024*1024,
            .cache = &os_cacheline[OS_YDST_TYPE_KERNEL],
#if KERNEL_NOTIFY_MODE == 1
            .fwrite = ydst_fwrite_kernel,
#else
            .write = ydst_write_kernel,
#endif
        },
        [OS_YDST_TYPE_TRACER] = {
            .file = "tracer/",
            .file_name = "tracer.log",
            .max_segment = 3,
            .max_segment_size = 10*1024*1024,
            .cache = &os_cacheline[OS_YDST_TYPE_TRACER],
        },
        [OS_YDST_TYPE_TCPDUMP] = {
            .file = "tcpdump/",
            .file_name = "tcpdump.log",
            .max_segment = 6,
            .max_segment_size = 20*1024*1024,
            .cache = &os_cacheline[OS_YDST_TYPE_TCPDUMP],
            .write_data2cache_first = 1,
        },
        [OS_YDST_TYPE_HCIDUMP] = {
            .file = "hcidump/",
            .file_name = "hcidump.log",
            .max_segment = 10,
            .max_segment_size = 20*1024*1024,
            .cache = &os_cacheline[OS_YDST_TYPE_HCIDUMP],
            .nowrap = 1,
            .ytag = 1,
            .write_data2cache_first = 1,
        },
        [OS_YDST_TYPE_TRACES] = {
            .file = "traces/",
            .file_name = "traces.log",
            .max_segment = 2,
            .max_segment_size = 20*1024*1024,
        },
        [OS_YDST_TYPE_SYS_INFO] = {
            .file = "sys_info/",
            .file_name = "sys_info.log",
            .max_segment = 5,
            .max_segment_size = 50*1024*1024,
        },
        { .file = NULL, }, /* .file = NULL to mark the end of the array */
    };
    struct ylog os_ylog[] = {
        {
            .name = "kernel",
            .type = FILE_NORMAL,
            .file = "/proc/kmsg",
            .ydst = &ydst[OS_YDST_TYPE_KERNEL+OS_YDST_TYPE_BASE],
            .mode = YLOG_READ_MODE_BLOCK | YLOG_READ_LEN_MIGHT_ZERO | YLOG_READ_MODE_BLOCK_RESTART_ALWAYS,
            .restart_period = 300,
            .fp_array = NULL,
            .timestamp = 1,
        },
        {
            .name = "tracer",
            .type = FILE_NORMAL,
            .file = "/d/tracing/trace_pipe",
            .ydst = &ydst[OS_YDST_TYPE_TRACER+OS_YDST_TYPE_BASE],
            .mode = YLOG_READ_MODE_BLOCK,
            .restart_period = 0,
            .fp_array = NULL,
        },
        {
            .name = "android_main",
            .type = FILE_POPEN,
            .file = "logcat -v threadtime -b main",
            .ydst = &ydst[OS_YDST_TYPE_ANDROID+OS_YDST_TYPE_BASE],
            .mode = YLOG_READ_MODE_BLOCK | YLOG_READ_LEN_MIGHT_ZERO | YLOG_READ_MODE_BLOCK_RESTART_ALWAYS,
            .restart_period = 2000,
            .fp_array = NULL,
            .id_token = "A0",
            .id_token_len = 2,
            .id_token_filename = "main.log",
        },
        {
            .name = "android_system",
            .type = FILE_POPEN,
            .file = "logcat -v threadtime -b system",
            .ydst = &ydst[OS_YDST_TYPE_ANDROID+OS_YDST_TYPE_BASE],
            .mode = YLOG_READ_MODE_BLOCK | YLOG_READ_LEN_MIGHT_ZERO | YLOG_READ_MODE_BLOCK_RESTART_ALWAYS,
            .restart_period = 2000,
            .fp_array = NULL,
            .id_token = "A1",
            .id_token_len = 2,
            .id_token_filename = "system.log",
        },
        {
            .name = "android_radio",
            .type = FILE_POPEN,
            .file = "logcat -v threadtime -b radio",
            .ydst = &ydst[OS_YDST_TYPE_ANDROID+OS_YDST_TYPE_BASE],
            .mode = YLOG_READ_MODE_BLOCK | YLOG_READ_LEN_MIGHT_ZERO | YLOG_READ_MODE_BLOCK_RESTART_ALWAYS,
            .restart_period = 2000,
            .fp_array = NULL,
            .id_token = "A2",
            .id_token_len = 2,
            .id_token_filename = "radio.log",
        },
        {
            .name = "android_events",
            .type = FILE_POPEN,
            .file = "logcat -v threadtime -b events",
            .ydst = &ydst[OS_YDST_TYPE_ANDROID+OS_YDST_TYPE_BASE],
            .mode = YLOG_READ_MODE_BLOCK | YLOG_READ_LEN_MIGHT_ZERO | YLOG_READ_MODE_BLOCK_RESTART_ALWAYS,
            .restart_period = 2000,
            .fp_array = NULL,
            .id_token = "A3",
            .id_token_len = 2,
            .id_token_filename = "events.log",
        },
        {
            .name = "android_crash",
            .type = FILE_POPEN,
            .file = "logcat -v threadtime -b crash",
            .ydst = &ydst[OS_YDST_TYPE_ANDROID+OS_YDST_TYPE_BASE],
            .mode = YLOG_READ_MODE_BLOCK | YLOG_READ_LEN_MIGHT_ZERO | YLOG_READ_MODE_BLOCK_RESTART_ALWAYS,
            .restart_period = 2000,
            .fp_array = NULL,
            .id_token = "A4",
            .id_token_len = 2,
            .id_token_filename = "crash.log",
        },
        {
            .name = "tcpdump",
            .type = FILE_POPEN,
            .file = "tcpdump -i any -p",
            .ydst = &ydst[OS_YDST_TYPE_TCPDUMP+OS_YDST_TYPE_BASE],
            .mode = YLOG_READ_MODE_BLOCK | YLOG_READ_LEN_MIGHT_ZERO | YLOG_READ_MODE_BLOCK_RESTART_ALWAYS,
            .restart_period = 2000,
            .status = YLOG_DISABLED,
            .fp_array = NULL,
        },
        {
            .name = "hcidump",
            .type = FILE_POPEN,
            .file = "hcidump",
            .ydst = &ydst[OS_YDST_TYPE_HCIDUMP+OS_YDST_TYPE_BASE],
            .mode = YLOG_READ_MODE_BLOCK | YLOG_READ_MODE_BINARY,
            .status = YLOG_DISABLED,
            .raw_data = 1,
        },
        {
            .name = "traces",
            .type = FILE_INOTIFY,
            .file = "inotify",
            .ydst = &ydst[OS_YDST_TYPE_TRACES+OS_YDST_TYPE_BASE],
            .yinotify = {
                .cells[0] = {
                    .pathname = "/data/anr/", /* this folder must make sure no one will delete anr folder */
                    .filename = "traces.txt",
                    .mask = IN_MODIFY,
                    .type = YLOG_INOTIFY_TYPE_MASK_SUBSET_BIT,
                    /* .timeout = 100, */
                    .handler = os_inotify_handler_anr,
                },
                .cells[1] = { /* kill -6 pid of com.xxx */
                    .pathname = "/data/tombstones/", /* this folder must make sure no one will delete tombstones folder */
                    .mask = IN_MODIFY,
                    .type = YLOG_INOTIFY_TYPE_MASK_SUBSET_BIT,
                    .timeout = 100,
                    .handler = os_inotify_handler_crash,
                    .args = &tombstone, /* struct ylog_inotify_cell_args */
                },
                #if 0
                .cells[0] = {
                    .filename = "/data/anr/traces.txt",
                    .mask = IN_MODIFY,
                    .type = YLOG_INOTIFY_TYPE_MASK_SUBSET_BIT,
                    .timeout = 100,
                    .handler = os_inotify_handler_anr,
                },
                #endif
                #if 0
                .cells[1] = {
                    .pathname = "/data/",
                    .filename = "hello.c",
                    .mask = IN_ACCESS,
                    .type = YLOG_INOTIFY_TYPE_MASK_EQUAL,
                    .timeout = 100,
                    .handler = os_inotify_handler_test,
                },
                #endif
            },
            .fp_array = NULL,
        },
        {
            .name = "sys_info",
            .type = FILE_NORMAL,
            .file = "/dev/null",
            .ydst = &ydst[OS_YDST_TYPE_SYS_INFO+OS_YDST_TYPE_BASE],
            .restart_period = 1000 * 60 * 2,
            .fread = ylog_read_sys_info,
            .fp_array = NULL,
        },
        {
            .name = "sys_info_manual",
            .type = FILE_NORMAL,
            .file = "/dev/null",
            .ydst = &ydst[OS_YDST_TYPE_SYS_INFO+OS_YDST_TYPE_BASE],
            .fread = ylog_read_sys_info_manual,
            .status = YLOG_DISABLED,
            .fp_array = NULL,
        },
        { .name = NULL, }, /* .name = NULL to mark the end of the array */
    };

    property_get("persist.ylog.enabled", buf, "1");
    if (buf[0] == '0') {
        system("stop ylog");
        sleep(60);
        exit(0);
    }

    umask(0);
    mode = M_USER_DEBUG;
    *c = &os_context[mode];

    ydst_insert_all(os_ydst);
    ylog_insert_all(os_ylog);
    os_parse_config();

    hook->ylog_read_ylog_debug_hook = ylog_read_ylog_debug_hook;
    hook->ylog_read_info_hook = ylog_read_info_hook;
    hook->process_command_hook = process_command_hook;
    hook->cmd_ylog_hook = cmd_ylog_hook;
    hook->ylog_status_hook = ylog_status_hook;

    poc->keep_historical_folder_numbers = KEEP_HISTORICAL_FOLDER_NUMBERS_DEFUALT;
    parse_config(YLOG_CONFIG);

    for_each_ylog(i, y, ylog) {
        enum ylog_thread_state state;
        if (y->name == NULL)
            continue;
        if (y->status & YLOG_DISABLED_MASK)
            state = YLOG_STOP;
        else
            state = YLOG_NOP;
        hook->ylog_status_hook(state, y);
    }

    root->root = os_detect_root_folder(NULL); /**
                                                * root->quota_new is 0 now if quota = root->max_size;
                                                * is used when sdcard is ready when ylog starts in os_search_root_path
                                                */
    property_get("ylog.anr.flag", buf, "0");
    pos->anr_fast = strtol(buf, NULL, 0);

    y = ylog_get_by_name("traces");
    if (pos->anr_fast)
        y->yinotify.cells[0].timeout = 50;
    else
        y->yinotify.cells[0].timeout = 2000;

    if (0) { /* avoid compiler warning */
        ylog = ylog;
    }
}
