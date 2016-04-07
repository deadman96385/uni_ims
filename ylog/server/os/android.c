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

struct other_processor_property_cell {
    const char *property_name;
    const char *property_value_default;
};

struct other_processor_node {
    struct other_processor_property_cell log;
    struct other_processor_property_cell diag;
    struct other_processor_property_cell atch; /* File is for sending raw AT command string */
};

struct other_processor_log_privates {
    char *file_diag;
    char *file_atch;
};

struct other_processor_log {
    const char *enabled;
    struct other_processor_node pnode;
    struct ynode ynode;
    struct other_processor_log_privates privates;
};

struct os_status {
    int sdcard_online;
    char ylog_root_path_latest[512];
    char historical_folder_root_last[512];
    char *file_atch[5];
    int file_atch_num;
    int anr_fast;
    int fd_kmsg;
} oss = {
    .file_atch_num = 5,
}, *pos = &oss;

struct os_config {
#define KEEP_HISTORICAL_FOLDER_NUMBERS_DEFUALT 5
    int keep_historical_folder_numbers;
} oc, *poc = &oc;

static int ylog_read_info_hook(char *buf, int count, FILE *fp, int fd, struct ylog *y) {
    UNUSED(fp);
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
    if (wfp) {
        do {
            if (fgets(buf, count, wfp) == NULL)
                break;
            snprintf(tmp, sizeof tmp, "cat %s", strtok(buf, "\n"));
            pcmd(tmp, &fd, y->write_handler, y, "ylog_info");
        } while (1);
        pclose(wfp);
    }

    pcmds(cmd_list, &fd, y->write_handler, y, "ylog_info");

    return 0;
}

static int ylog_read_ylog_debug_hook(char *buf, int count, FILE *fp, int fd, struct ylog *y) {
    UNUSED(buf);
    UNUSED(count);
    UNUSED(fp);
    char *cmd_list[] = {
        "/system/bin/ylog_cli ylog",
        "/system/bin/ylog_cli speed",
        "/system/bin/ylog_cli space",
        "getprop ylog.killed",
        NULL
    };
    pcmds(cmd_list, &fd, y->write_handler, y, "ylog_debug");
    return 0;
}

static int ylog_read_sys_info(char *buf, int count, FILE *fp, int fd, struct ylog *y) {
    UNUSED(buf);
    UNUSED(count);
    UNUSED(fp);
    UNUSED(fd);
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
    UNUSED(arg);
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
    UNUSED(cmd);
    UNUSED(buf_size);
    UNUSED(index);
    UNUSED(yp);
    buf[ylog_get_format_time(buf)] = '\n';
    SEND(fd, buf, strlen(buf), MSG_NOSIGNAL);
    return 0;
}

static int cmd_rootdir(struct command *cmd, char *buf, int buf_size, int fd, int index, struct ylog_poll *yp) {
    UNUSED(cmd);
    UNUSED(index);
    UNUSED(yp);
    SEND(fd, buf, snprintf(buf, buf_size, "%s\n", pos->ylog_root_path_latest), MSG_NOSIGNAL);
    return 0;
}

static int cmd_cpath_last(struct command *cmd, char *buf, int buf_size, int fd, int index, struct ylog_poll *yp) {
    UNUSED(cmd);
    UNUSED(index);
    UNUSED(yp);
    SEND(fd, buf, snprintf(buf, buf_size, "%s\n", pos->historical_folder_root_last), MSG_NOSIGNAL);
    return 0;
}

static void ylog_update_config2(char *key, char *value);
static int cmd_history_n(struct command *cmd, char *buf, int buf_size, int fd, int index, struct ylog_poll *yp) {
    UNUSED(cmd);
    UNUSED(index);
    UNUSED(yp);
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
    SEND(fd, buf, snprintf(buf, buf_size, "%d\n", c->keep_historical_folder_numbers), MSG_NOSIGNAL);
    return 0;
}

static int cmd_setprop(struct command *cmd, char *buf, int buf_size, int fd, int index, struct ylog_poll *yp) {
    UNUSED(cmd);
    UNUSED(index);
    UNUSED(yp);
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
    SEND(fd, buf, snprintf(buf, buf_size, "%s %s\n", property, pp), MSG_NOSIGNAL);
    return 0;
}

static int send_at_file_atch(int idx, char *buf, int count, char *retbuf, int *retcount_max) {
    int fd, result = 0, ret = 0;
    char *file_atch;
    if (idx >= pos->file_atch_num) {
        return 1;
    }
    file_atch = pos->file_atch[idx];
    fd = open(file_atch, O_RDWR);
    if (fd < 0) {
        ylog_info("open %s fail.%s", file_atch, strerror(errno));
        return 1;
    }
    if (count != fd_write(buf, count, fd))
        result = 1;
    ylog_debug("send to %s with %s\n", file_atch, buf);
    if (retbuf && retcount_max) {
        struct pollfd pfd[1];
        int ret_max = *retcount_max;
        *retcount_max = 0;
        pfd[0].fd = fd;
        pfd[0].events = POLLIN;
        do {
            ret = poll(pfd, 1, 1000);
            if (ret <= 0) {
                ylog_critical("xxxxxxxxxxxxxxxxxxxxxx ylog cp is failed, %s\n",
                        ret == 0 ? "timeout":strerror(errno));
                result = ret ? 1:0;
                break;
            }
            if (fcntl_read_nonblock(fd, "send_at_file_atch") == 0) {
                ret = read(fd, retbuf, ret_max);
                if (ret > 0)
                    fcntl_read_block(fd, "send_at_file_atch");
            } else
                ret = 0;

            if (ret >=0) {
                *retcount_max += ret;
            } else {
                result = 0/*1*/;
                ylog_critical("xxxxxxxxxxxxxxxxxxxxxx ylog read is failed, %s\n", strerror(errno));
            }
        } while (0/*ret > 0*/);
    }
    close(fd);
    return result;
}

static int cmd_at(struct command *cmd, char *buf, int buf_size, int fd, int index, struct ylog_poll *yp) {
    UNUSED(cmd);
    UNUSED(index);
    UNUSED(yp);
    int len = strlen(buf);
    int idx = 0;
    int ret_size = buf_size;
    if (len == 3) {
        SEND(fd, buf, snprintf(buf, buf_size, "%s\n", pos->file_atch[idx]), MSG_NOSIGNAL);
        return 0;
    }
    strcat(buf, "\r");
    len++;
    if (send_at_file_atch(idx, buf + 3, len - 3, buf, &ret_size) == 0) {
        if (ret_size <= 0) {
            len = 0;
            if (ret_size == 0) {
                len = snprintf(buf, buf_size, "Try to stop engpc:\n"
                        "getprop | grep init.svc.engpc | cut -d '.' -f 3 | cut -d ']' -f 0\n"
                        "or\n"
                        "stop engpcclientt; stop engpcclientlte; "
                        "stop engpcclientw; stop engpcclienttl; stop engpcclientlf\n");
            }
        } else
            len = ret_size;
    } else {
        len = snprintf(buf, buf_size, "Failed\n");
    }
    if (len)
        SEND(fd, buf, len, MSG_NOSIGNAL);
    return 0;
}

static int cmd_print2android(struct command *cmd, char *buf, int buf_size, int fd, int index, struct ylog_poll *yp) {
    UNUSED(cmd);
    UNUSED(buf_size);
    UNUSED(fd);
    UNUSED(index);
    UNUSED(yp);
    ylog_warn("%s\n", buf);
    return 0;
}

static int cmd_print2kernel(struct command *cmd, char *buf, int buf_size, int fd, int index, struct ylog_poll *yp) {
    UNUSED(cmd);
    UNUSED(index);
    UNUSED(yp);
    int ret = 1, len;
    if (pos->fd_kmsg > 0) {
        len = strlen(buf);
        buf[len++] = '\n';
        buf[len] = '\0';
        if (len == fd_write(buf, len, pos->fd_kmsg))
            ret = 0;
    }
    if (ret)
        SEND(fd, buf, snprintf(buf, buf_size, "%s\n", "failed"), MSG_NOSIGNAL);
    return 0;
}

static struct command os_commands[] = {
    {"test", "test from android", cmd_test, NULL},
    {"\n", NULL, os_cmd_help, (void*)os_commands},
    {"rootdir", "get the log disk root dir", cmd_rootdir, NULL},
    {"cpath_last", "get the last_ylog path", cmd_cpath_last, NULL},
    {"history_n", "set keep_historical_folder_numbers", cmd_history_n, NULL},
    {"setprop", "set property, ex. ylog_cli setprop persist.ylog.enabled 1", cmd_setprop, NULL},
    {"at", "send AT command to cp side, ex. ylog_cli at AT+ARMLOG=1 or ylog_cli at AT+CGMR", cmd_at, NULL},
    {"print2android", "write data to android system log", cmd_print2android, NULL},
    {"print2kernel ", "write data to kernel log", cmd_print2kernel, NULL},
    {NULL, NULL, NULL, NULL}
};

static void load_loglevel(struct ylog_keyword *kw, int nargs, char **args) {
    UNUSED(kw);
    UNUSED(nargs);
    struct context *c = global_context;
    int loglevel = strtol(args[1], NULL, 0);
    if (loglevel < 0 || loglevel >= LOG_LEVEL_MAX)
        loglevel = LOG_DEBUG;
    c->loglevel = loglevel;
}

static void load_keep_historical_folder_numbers(struct ylog_keyword *kw, int nargs, char **args) {
    UNUSED(kw);
    UNUSED(nargs);
    int history_n = strtol(args[1], NULL, 0);
    if (history_n == 0)
        history_n = KEEP_HISTORICAL_FOLDER_NUMBERS_DEFUALT;
    poc->keep_historical_folder_numbers = history_n;
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
    UNUSED(kw);
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
    if (key && strcmp(key, "enabled") == 0) {
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
}

static struct ylog_keyword ylog_keyword[] = {
    {"loglevel", load_loglevel},
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
        LSEEK(fd, -3, SEEK_END);
        buf[0] = 0;
        buf[3] = 0;
        if (read(fd, buf, 3) <= 0)
            return -1;
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

    pos->fd_kmsg = open("/dev/kmsg", O_WRONLY);
    if (pos->fd_kmsg < 0)
        ylog_info("open %s fail. %s", "/dev/kmsg", strerror(errno));

#if 1
    if (drop_privs() < 0)
        ylog_error("ylog drop_privs failed: %s\n", strerror(errno));
    else
        ylog_critical("ylog drop_privs success\n");
#endif

    mkdirs("/data/anr/"); /* For ylog traces.txt */
    mkdirs("/data/tombstones/"); /* For ylog tombstone_00 ~ tombstone_09 */

    ylog_trigger_all(global_ylog); /* mark ylog status from ready to run */

    ylog_os_event_timer_create("android", pos->sdcard_online ? 5*1000:1*1000, event_timer_handler, NULL);
}

static struct context os_context[M_MODE_NUM] = {
    [M_USER] = {
        .config_file = "1.xml",
        .filter_plugin_path = YLOG_FILTER_PATH,
        .journal_file = YLOG_JOURNAL_FILE,
        .model = C_MINI_LOG,
        .loglevel = LOG_WARN,
    },
    [M_USER_DEBUG] = {
        .config_file = "2.xml",
        .filter_plugin_path = YLOG_FILTER_PATH,
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

static void insert_file_atch(char *file_atch, int mode) {
    UNUSED(mode);
    int i;
    for (i = 0; i < pos->file_atch_num; i++) {
        if (pos->file_atch[i] == NULL) {
            pos->file_atch[i] = strdup(file_atch);
        }
    }
}

static void other_processor_ylog_insert(void) {
    int i, flag;
    struct ylog *ylog;
    struct ydst *ydst;
    struct other_processor_log *opl;
    char prop[PROPERTY_VALUE_MAX];
    struct other_processor_log_privates *privates;
    char buf[4096];
    struct other_processor_log opls[] = {
        /* cp/wcdma/ */ {
            .enabled = "persist.modem.w.enable",
            .pnode = {
                .log  = {"ro.modem.w.log", "/dev/slog_w"},
                .diag = {"ro.modem.w.diag", "/dev/slog_w"},
                .atch = {"ro.modem.w.tty", "/dev/stty_w31"},
            },
            .ynode = {
                .ylog = {
                    {
                        .name = "cp_wcdma",
                        .type = FILE_NORMAL,
                        .file = NULL,
                        .mode = YLOG_READ_MODE_BLOCK | YLOG_READ_MODE_BINARY |
                            YLOG_READ_LEN_MIGHT_ZERO | YLOG_READ_MODE_BLOCK_RESTART_ALWAYS |
                            YLOG_GROUP_MODEM,
                        .restart_period = 3000,
                        .status = YLOG_DISABLED,
                        .raw_data = 1,
                    },
                },
                .ydst = {
                    .file = "cp/wcdma/",
                    .file_name = "wcdma.log",
                    .max_segment = 10,
                    .max_segment_size = 200*1024*1024,
                    .write_data2cache_first = 1,
                },
                .cache = {
                    .size = 512 * 1024,
                    .num = 8,
                    .timeout = 1000,
                    .debuglevel = CACHELINE_DEBUG_CRITICAL,
                },
            },
        },
        /* cp/td-scdma/ */ {
            .enabled = "persist.modem.t.enable",
            .pnode = {
                .log  = {"ro.modem.t.log", "/dev/slog_gge"},
                .diag = {"ro.modem.t.diag", "/dev/slog_gge"},
                .atch = {"ro.modem.t.tty", "/dev/stty_t31"},
            },
            .ynode = {
                .ylog = {
                    {
                        .name = "cp_td-scdma",
                        .type = FILE_NORMAL,
                        .file = NULL,
                        .mode = YLOG_READ_MODE_BLOCK | YLOG_READ_MODE_BINARY |
                            YLOG_READ_LEN_MIGHT_ZERO | YLOG_READ_MODE_BLOCK_RESTART_ALWAYS |
                            YLOG_GROUP_MODEM,
                        .restart_period = 3000,
                        .status = YLOG_DISABLED,
                        .raw_data = 1,
                    },
                },
                .ydst = {
                    .file = "cp/td-scdma/",
                    .file_name = "td-scdma.log",
                    .max_segment = 10,
                    .max_segment_size = 200*1024*1024,
                    .write_data2cache_first = 1,
                },
                .cache = {
                    .size = 512 * 1024,
                    .num = 8,
                    .timeout = 1000,
                    .debuglevel = CACHELINE_DEBUG_CRITICAL,
                },
            },
        },
        /* cp/5mode/ */ {
            .enabled = "persist.modem.l.enable",
            .pnode = {
                .log  = {"ro.modem.l.log", "/dev/slog_lte"},
                .diag = {"ro.modem.l.diag", "/dev/sdiag_lte"},
                .atch = {"ro.modem.l.tty", "/dev/stty_lte31"},
            },
            .ynode = {
                .ylog = {
                    {
                        .name = "cp_5mode",
                        .type = FILE_NORMAL,
                        .file = NULL,
                        .mode = YLOG_READ_MODE_BLOCK | YLOG_READ_MODE_BINARY |
                            YLOG_READ_LEN_MIGHT_ZERO | YLOG_READ_MODE_BLOCK_RESTART_ALWAYS |
                            YLOG_GROUP_MODEM,
                        .restart_period = 3000,
                        .status = YLOG_DISABLED,
                        .raw_data = 1,
                    },
                },
                .ydst = {
                    .file = "cp/5mode/",
                    .file_name = "5mode.log",
                    .max_segment = 10,
                    .max_segment_size = 200*1024*1024,
                    .write_data2cache_first = 1,
                },
                .cache = {
                    .size = 512 * 1024,
                    .num = 8,
                    .timeout = 1000,
                    .debuglevel = CACHELINE_DEBUG_CRITICAL,
                },
            },
        },
        /* cp/4mode/ */ {
            .enabled = "persist.modem.lf.enable",
            .pnode = {
                .log  = {"ro.modem.lf.log", "/dev/slog_lte"},
                .diag = {"ro.modem.lf.diag", "/dev/sdiag_lte"},
                .atch = {"ro.modem.lf.tty", "/dev/stty_lte31"},
            },
            .ynode = {
                .ylog = {
                    {
                        .name = "cp_4mode",
                        .type = FILE_NORMAL,
                        .file = NULL,
                        .mode = YLOG_READ_MODE_BLOCK | YLOG_READ_MODE_BINARY |
                            YLOG_READ_LEN_MIGHT_ZERO | YLOG_READ_MODE_BLOCK_RESTART_ALWAYS |
                            YLOG_GROUP_MODEM,
                        .restart_period = 3000,
                        .status = YLOG_DISABLED,
                        .raw_data = 1,
                    },
                },
                .ydst = {
                    .file = "cp/4mode/",
                    .file_name = "4mode.log",
                    .max_segment = 10,
                    .max_segment_size = 200*1024*1024,
                    .write_data2cache_first = 1,
                },
                .cache = {
                    .size = 512 * 1024,
                    .num = 8,
                    .timeout = 1000,
                    .debuglevel = CACHELINE_DEBUG_CRITICAL,
                },
            },
        },
        /* cp/3mode/ */ {
            .enabled = "persist.modem.tl.enable",
            .pnode = {
                .log  = {"ro.modem.tl.log", "/dev/slog_lte"},
                .diag = {"ro.modem.tl.diag", "/dev/sdiag_lte"},
                .atch = {"ro.modem.tl.tty", "/dev/stty_lte31"},
            },
            .ynode = {
                .ylog = {
                    {
                        .name = "cp_3mode",
                        .type = FILE_NORMAL,
                        .file = NULL,
                        .mode = YLOG_READ_MODE_BLOCK | YLOG_READ_MODE_BINARY |
                            YLOG_READ_LEN_MIGHT_ZERO | YLOG_READ_MODE_BLOCK_RESTART_ALWAYS |
                            YLOG_GROUP_MODEM,
                        .restart_period = 3000,
                        .status = YLOG_DISABLED,
                        .raw_data = 1,
                    },
                },
                .ydst = {
                    .file = "cp/3mode/",
                    .file_name = "3mode.log",
                    .max_segment = 10,
                    .max_segment_size = 200*1024*1024,
                    .write_data2cache_first = 1,
                },
                .cache = {
                    .size = 512 * 1024,
                    .num = 8,
                    .timeout = 1000,
                    .debuglevel = CACHELINE_DEBUG_CRITICAL,
                },
            },
        },
        /* cp/wcn/ */ {
            .enabled = "ro.modem.wcn.enable",
            .pnode = {
                .log  = {"ro.modem.wcn.log", "/dev/slog_wcn"},
                .diag = {"ro.modem.wcn.diag", "/dev/slog_wcn"},
            },
            .ynode = {
                .ylog = {
                    {
                        .name = "cp_wcn",
                        .type = FILE_NORMAL,
                        .file = NULL,
                        .mode = YLOG_READ_MODE_BLOCK | YLOG_READ_MODE_BINARY |
                            YLOG_READ_LEN_MIGHT_ZERO | YLOG_READ_MODE_BLOCK_RESTART_ALWAYS,
                        .restart_period = 3000,
                        .status = YLOG_DISABLED,
                        .raw_data = 1,
                    },
                },
                .ydst = {
                    .file = "cp/wcn/",
                    .file_name = "wcn.log",
                    .max_segment = 2,
                    .max_segment_size = 100*1024*1024,
                },
                .cache = {
                    .size = 512 * 1024,
                    .num = 2,
                    .timeout = 1000,
                    .debuglevel = CACHELINE_DEBUG_CRITICAL,
                },
            },
        },
        /* cp/gnss/ */ {
            .enabled = "ro.modem.gnss.enable",
            .pnode = {
                .log  = {"ro.modem.gnss.log", "/dev/slog_gnss"},
                .diag = {"ro.modem.gnss.diag", "/dev/slog_gnss"},
            },
            .ynode = {
                .ylog = {
                    {
                        .name = "cp_gnss",
                        .type = FILE_NORMAL,
                        .file = NULL,
                        .mode = YLOG_READ_MODE_BLOCK | YLOG_READ_MODE_BINARY |
                            YLOG_READ_LEN_MIGHT_ZERO | YLOG_READ_MODE_BLOCK_RESTART_ALWAYS,
                        .restart_period = 3000,
                        .status = YLOG_DISABLED,
                        .raw_data = 1,
                    },
                },
                .ydst = {
                    .file = "cp/gnss/",
                    .file_name = "gnss.log",
                    .max_segment = 2,
                    .max_segment_size = 100*1024*1024,
                },
                .cache = {
                    .size = 512 * 1024,
                    .num = 4,
                    .timeout = 1000,
                    .debuglevel = CACHELINE_DEBUG_CRITICAL,
                },
            },
        },
        /* cp/agdsp/ */ {
            .enabled = "ro.modem.agdsp.enable",
            .pnode = {
                .log  = {"ro.modem.agdsp.log", "/dev/audio_dsp_log"},
                .diag = {"ro.modem.agdsp.diag", "/dev/audio_dsp_log"},
            },
            .ynode = {
                .ylog = {
                    {
                        .name = "cp_agdsp",
                        .type = FILE_NORMAL,
                        .file = NULL,
                        .mode = YLOG_READ_MODE_BLOCK | YLOG_READ_MODE_BINARY |
                            YLOG_READ_LEN_MIGHT_ZERO | YLOG_READ_MODE_BLOCK_RESTART_ALWAYS,
                        .restart_period = 3000,
                        .status = YLOG_DISABLED,
                        .raw_data = 1,
                    },
                },
                .ydst = {
                    .file = "cp/agdsp/",
                    .file_name = "agdsp.log",
                    .max_segment = 2,
                    .max_segment_size = 100*1024*1024,
                    .write_data2cache_first = 1,
                },
                .cache = {
                    .size = 512 * 1024,
                    .num = 8,
                    .timeout = 1000,
                    .debuglevel = CACHELINE_DEBUG_CRITICAL,
                },
            },
        },
    };

    for (i = 0; i < (int)ARRAY_LEN(opls); i++) {
        static int first_time_print = 1;
        opl = &opls[i];
        ylog = &opl->ynode.ylog[0];
        ydst = &opl->ynode.ydst;
#if 1
        if ((ylog->mode & YLOG_GROUP_MODEM) && first_time_print) {
            first_time_print = 0;
            property_get("ro.radio.modemtype", prop, "");
            if (prop[0]) {
                char modem_type[64];
                char mprop[512];
                strncpy(modem_type, prop, sizeof modem_type);

                snprintf(mprop, sizeof mprop, "persist.modem.%s.enable", modem_type);
                property_get(mprop, prop, "");
                if (prop[0] && opl->enabled == NULL)
                    opl->enabled = strdup(prop);
                else
                    ylog_info("%s = %s\n", mprop, prop);

                snprintf(mprop, sizeof mprop, "ro.modem.%s.log", modem_type);
                property_get(mprop, prop, "");
                if (prop[0] && opl->pnode.log.property_name == NULL)
                    opl->pnode.log.property_name = strdup(prop);
                else
                    ylog_info("%s = %s\n", mprop, prop);

                snprintf(mprop, sizeof mprop, "ro.modem.%s.diag", modem_type);
                property_get(mprop, prop, "");
                if (prop[0] && opl->pnode.diag.property_name == NULL)
                    opl->pnode.diag.property_name = strdup(prop);
                else
                    ylog_info("%s = %s\n", mprop, prop);

                snprintf(mprop, sizeof mprop, "ro.modem.%s.tty", modem_type);
                property_get(mprop, prop, "");
                if (prop[0] && opl->pnode.atch.property_name == NULL)
                    opl->pnode.atch.property_name = strdup(prop);
                else
                    ylog_info("%s = %s\n", mprop, prop);
            }
        }
#endif
        if (opl->enabled) {
            property_get(opl->enabled, prop, "");
            if (strcmp(prop, "1"))
                continue;
        }
        flag = 0;
#if 1
        ydst->max_segment = 2;
        ydst->max_segment_size = 10 * 1024 * 1024;
#endif
        privates = &opl->privates;
        if (opl->pnode.log.property_name) {
            property_get(opl->pnode.log.property_name, prop, opl->pnode.log.property_value_default);
            if (prop[0] && ylog->file == NULL) {
                ylog->file = strdup(prop);
                flag |= 0x01;
            }
        }
        if (opl->pnode.diag.property_name) {
            property_get(opl->pnode.diag.property_name, prop, opl->pnode.diag.property_value_default);
            if (prop[0] && ylog->file == NULL) {
                /**
                 * diag path will be used both for log reading
                 */
                ylog->file = strdup(prop);
                flag |= 0x01;
            }
        }
        if (opl->pnode.atch.property_name) {
            property_get(opl->pnode.atch.property_name, prop, opl->pnode.atch.property_value_default);
            if (prop[0]) {
                strcat(prop, "31");
                privates->file_atch = strdup(prop);
                if (ylog->mode & YLOG_GROUP_MODEM)
                    insert_file_atch(privates->file_atch, YLOG_GROUP_MODEM);
                ylog->privates = calloc(sizeof(struct other_processor_log_privates), 1);
                *(struct other_processor_log_privates*)ylog->privates = *privates;
                flag |= 0x02;
            }
        }
        if (ylog->file) {
            if (ynode_insert(&opl->ynode)) {
                if (flag & 0x01)
                    free(ylog->file);
                if (flag & 0x02) {
                    free(ylog->privates);
                    free(privates->file_atch);
                }
            }
        }
    }
}

static void os_init(struct ydst_root *root, struct context **c, struct os_hooks *hook) {
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
    struct ynode os_ynode[] = {
        /* kernel/ */ {
           .ylog = {
                {
                    .name = "kernel",
                    .type = FILE_NORMAL,
                    .file = "/proc/kmsg",
                    .mode = YLOG_READ_MODE_BLOCK | YLOG_READ_LEN_MIGHT_ZERO | YLOG_READ_MODE_BLOCK_RESTART_ALWAYS,
                    .restart_period = 1000,
                    .fp_array = NULL,
                    .timestamp = 1,
                },
            },
            .ydst = {
                .file = "kernel/",
                .file_name = "kernel.log",
                .max_segment = 10,
                .max_segment_size = 50*1024*1024,
#if KERNEL_NOTIFY_MODE == 1
                .fwrite = ydst_fwrite_kernel,
#else
                .write = ydst_write_kernel,
#endif
            },
            .cache = {
                .size = 512 * 1024,
                .num = 2,
                .timeout = 1000,
                .debuglevel = CACHELINE_DEBUG_CRITICAL,
            },
        },
        /* android/ */ {
            .ylog = {
                {
                    .name = "android_main",
                    .type = FILE_POPEN,
                    .file = "logcat -v threadtime -b main",
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
                    .mode = YLOG_READ_MODE_BLOCK | YLOG_READ_LEN_MIGHT_ZERO | YLOG_READ_MODE_BLOCK_RESTART_ALWAYS,
                    .restart_period = 2000,
                    .fp_array = NULL,
                    .id_token = "A4",
                    .id_token_len = 2,
                    .id_token_filename = "crash.log",
                },
            },
            .ydst = {
                .file = "android/",
                .file_name = "android.log",
                .max_segment = 30,
                .max_segment_size = 50*1024*1024,
            },
            .cache = {
                .size = 512 * 1024,
                .num = 4,
                .timeout = 1000,
                .debuglevel = CACHELINE_DEBUG_CRITICAL,
            },
        },
        /* tcpdump/ */ {
            .ylog = {
                {
                    .name = "tcpdump",
                    .type = FILE_POPEN,
                    .file = "tcpdump -i any -p",
                    .mode = YLOG_READ_MODE_BLOCK | YLOG_READ_LEN_MIGHT_ZERO | YLOG_READ_MODE_BLOCK_RESTART_ALWAYS,
                    .restart_period = 2000,
                    .status = YLOG_DISABLED,
                    .fp_array = NULL,
                },
            },
            .ydst = {
                .file = "tcpdump/",
                .file_name = "tcpdump.log",
                .max_segment = 3,
                .max_segment_size = 50*1024*1024,
                .write_data2cache_first = 1,
            },
            .cache = {
                .size = 512 * 1024,
                .num = 2,
                .timeout = 1000,
                .debuglevel = CACHELINE_DEBUG_CRITICAL,
            },

        },
        /* hcidump/ */ {
            .ylog = {
                {
                    .name = "hcidump",
                    .type = FILE_POPEN,
                    .file = "hcidump",
                    .mode = YLOG_READ_MODE_BLOCK | YLOG_READ_MODE_BINARY,
                    .status = YLOG_DISABLED,
                    .raw_data = 1,
                },
            },
            .ydst = {
                .file = "hcidump/",
                .file_name = "hcidump.log",
                .max_segment = 4,
                .max_segment_size = 50*1024*1024,
                .nowrap = 1,
                .ytag = 1,
                .write_data2cache_first = 1,
            },
            .cache = {
                .size = 512 * 1024,
                .num = 2,
                .timeout = 1000,
                .debuglevel = CACHELINE_DEBUG_CRITICAL,
            },
        },
        /* traces/ */ {
            .ylog = {
                {
                    .name = "traces",
                    .type = FILE_INOTIFY,
                    .file = "inotify",
                    .yinotify = {
                        .cells[0] = {
                            /* this folder must make sure no one will delete anr folder */
                            .pathname = "/data/anr/",
                            .filename = "traces.txt",
                            .mask = IN_MODIFY,
                            .type = YLOG_INOTIFY_TYPE_MASK_SUBSET_BIT,
                            /* .timeout = 100, */
                            .handler = os_inotify_handler_anr,
                        },
                        .cells[1] = { /* kill -6 pid of com.xxx */
                            /* this folder must make sure no one will delete tombstones folder */
                            .pathname = "/data/tombstones/",
                            .mask = IN_MODIFY,
                            .type = YLOG_INOTIFY_TYPE_MASK_SUBSET_BIT,
                            .timeout = 1000,
                            .handler = os_inotify_handler_crash,
                            .args = &tombstone, /* struct ylog_inotify_cell_args */
                        },
                    },
                    .fp_array = NULL,
                },
            },
            .ydst = {
                .file = "traces/",
                .file_name = "traces.log",
                .max_segment = 2,
                .max_segment_size = 20*1024*1024,
            },
        },
        /* sys_info/ */ {
            .ylog = {
                {
                    .name = "sys_info",
                    .type = FILE_NORMAL,
                    .file = "/dev/null",
                    .restart_period = 1000 * 60 * 2,
                    .fread = ylog_read_sys_info,
                    .fp_array = NULL,
                },
                {
                    .name = "sys_info_manual",
                    .type = FILE_NORMAL,
                    .file = "/dev/null",
                    .fread = ylog_read_sys_info_manual,
                    .status = YLOG_DISABLED,
                    .fp_array = NULL,
                },
            },
            .ydst = {
                .file = "sys_info/",
                .file_name = "sys_info.log",
                .max_segment = 5,
                .max_segment_size = 50*1024*1024,
            },
        },
        /* tracer/ */ {
            .ylog = {
                {
                    .name = "tracer",
                    .type = FILE_NORMAL,
                    .file = "/d/tracing/trace_pipe",
                    .mode = YLOG_READ_MODE_BLOCK,
                    .restart_period = 0,
                    .fp_array = NULL,
                },
            },
            .ydst = {
                .file = "tracer/",
                .file_name = "tracer.log",
                .max_segment = 3,
                .max_segment_size = 10*1024*1024,
            },
            .cache = {
                .size = 512 * 1024,
                .num = 2,
                .timeout = 1000,
                .debuglevel = CACHELINE_DEBUG_CRITICAL,
            },
        },
    };

    property_get("persist.ylog.enabled", buf, "1");
    if (buf[0] == '0') {
        system("stop ylog");
        sleep(2);
        exit(0);
    }

    umask(0);
    mode = M_USER_DEBUG;
    *c = &os_context[mode];

    ynode_insert_all(os_ynode, (int)ARRAY_LEN(os_ynode));
    other_processor_ylog_insert();
    os_parse_config();

    hook->ylog_read_ylog_debug_hook = ylog_read_ylog_debug_hook;
    hook->ylog_read_info_hook = ylog_read_info_hook;
    hook->process_command_hook = process_command_hook;
    hook->cmd_ylog_hook = cmd_ylog_hook;
    hook->ylog_status_hook = ylog_status_hook;

    poc->keep_historical_folder_numbers = KEEP_HISTORICAL_FOLDER_NUMBERS_DEFUALT;
    parse_config(YLOG_CONFIG);

    for_each_ylog(i, y, NULL) {
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
    if (y) {
        if (pos->anr_fast)
            y->yinotify.cells[0].timeout = 50;
        else
            y->yinotify.cells[0].timeout = 2000;
    }
}
