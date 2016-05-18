/**
 * Copyright (C) 2016 Spreadtrum Communications Inc.
 */

#include <cutils/sched_policy.h>
#include <private/android_filesystem_config.h>
#include <utils/threads.h>
#include <sys/prctl.h>
#include <sys/capability.h>
#include <cutils/properties.h>

#define DROP_PRIVS_UID AID_SYSTEM
#define DROP_PRIVS_GID AID_SYSTEM

#define YLOG_ROOT_FOLDER "/data"
#define YLOG_JOURNAL_FILE "/data/ylog/ylog_journal_file"
#define YLOG_CONFIG_FILE "/data/ylog/ylog.conf"
#define YLOG_FILTER_PATH "/system/lib/"

#define ANR_OR_TOMSTONE_UNIFILE_MODE
#define ANR_OR_TOMSTONE_UNIFILE_THREAD_COPY_MODE
#define ANR_FAST_COPY_MODE
#define ANR_FAST_COPY_MODE_ANR_FILE "/data/anr/traces.txt"
#define ANR_FAST_COPY_MODE_TMP "/data/ylog/traces.txt.ylog.tmp"
#define ANR_FAST_COPY_MODE_YLOG "/data/ylog/traces.txt.ylog"

struct os_status {
    int sdcard_online;
    char ylog_root_path_latest[512];
    char historical_folder_root_last[512];
    char *file_atch[5];
    char *file_atloop[5];
    int file_atch_num;
    int file_atloop_num;
    int anr_fast;
    int fd_kmsg;
    struct ylog *snapshot;
#ifdef ANR_FAST_COPY_MODE
    int anr_fast_copy_count;
#endif
} oss = {
    .file_atch_num = 5,
    .file_atloop_num = 5,
}, *pos = &oss;

#include "modem.c"
#include "snapshot-android.c"

static int ylog_write_header_sgm_cpu_memory_header(struct ylog *y) {
#define YLOG_SGM_CPU_MEMORY_HEADER "second,cpu-cpu_percent[0],cpu-iowtime[1],cpu-cpu_frequency[2],cpu-null[3],cpu-null[4],cpu-null[5],cpu0-cpu_percent[0],cpu0-iowtime[1],cpu0-cpu_frequency[2],cpu0-null[3],cpu0-null[4],cpu0-null[5],cpu1-cpu_percent[0],cpu1-iowtime[1],cpu1-cpu_frequency[2],cpu1-null[3],cpu1-null[4],cpu1-null[5],cpu2-cpu_percent[0],cpu2-iowtime[1],cpu2-cpu_frequency[2],cpu2-null[3],cpu2-null[4],cpu2-null[5],cpu3-cpu_percent[0],cpu3-iowtime[1],cpu3-cpu_frequency[2],cpu3-null[3],cpu3-null[4],cpu3-null[5],cpu4-cpu_percent[0],cpu4-iowtime[1],cpu4-cpu_frequency[2],cpu4-null[3],cpu4-null[4],cpu4-null[5],cpu5-cpu_percent[0],cpu5-iowtime[1],cpu5-cpu_frequency[2],cpu5-null[3],cpu5-null[4],cpu5-null[5],cpu6-cpu_percent[0],cpu6-iowtime[1],cpu6-cpu_frequency[2],cpu6-null[3],cpu6-null[4],cpu6-null[5],cpu7-cpu_percent[0],cpu7-iowtime[1],cpu7-cpu_frequency[2],cpu7-null[3],cpu7-null[4],cpu7-null[5],irqs,ctxt,processes,procs_running,procs_blocked,totalram,freeram,cached,Reserve01,Reserve02,Reserve03,Reserve04,Reserve05,Reserve06\n"
    return y->ydst->write(y->id_token, y->id_token_len,
            YLOG_SGM_CPU_MEMORY_HEADER, strlen(YLOG_SGM_CPU_MEMORY_HEADER), y->ydst);
}

#ifdef HAVE_YLOG_INFO
static int ylog_read_info_hook(char *buf, int count, FILE *fp, int fd, struct ylog *y) {
    UNUSED(fp);
    FILE *wfp;
    char tmp[PATH_MAX];
    char *cmd_list[] = {
        "ls -l /dev/block/platform/*/by-name/",
        "ls -l /dev/",
        "cat /default.prop",
        "getprop",
        NULL
    };

    wfp = popen("ls /*.rc", "r");
    if (wfp) {
        char *last;
        do {
            if (fgets(buf, count, wfp) == NULL)
                break;
            snprintf(tmp, sizeof tmp, "cat %s", strtok_r(buf, "\n", &last));
            pcmd(tmp, &fd, y->write_handler, y, "ylog_info", -1, -1, NULL);
        } while (1);
        pclose(wfp);
    }

    pcmds(cmd_list, &fd, y->write_handler, y, "ylog_info", -1, NULL);

    return 0;
}
#endif

static int ylog_read_ylog_debug_hook(char *buf, int count, FILE *fp, int fd, struct ylog *y) {
    UNUSED(buf);
    UNUSED(count);
    UNUSED(fp);
    char *cmd_list[] = {
        "logcat -S",
        "/system/bin/ylog_cli ylog",
        "/system/bin/ylog_cli speed",
        "/system/bin/ylog_cli space",
        "getprop ylog.monkey",
        "getprop ylog.killed",
        NULL
    };
    pcmds(cmd_list, &fd, y->write_handler, y, "ylog_debug", 1000, NULL);
    return 0;
}

static int ylog_read_sys_info(char *buf, int count, FILE *fp, int fd, struct ylog *y) {
    UNUSED(buf);
    UNUSED(count);
    UNUSED(fp);
    UNUSED(fd);
    int cnt = 0;
    char *cmd_list[] = {
        "free",
        "vmstat",
        "df",
        "cat /proc/buddyinfo",
        "cat /proc/meminfo",
        "cat /proc/slabinfo",
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
        "cat /proc/interrupts",
        NULL
    };
    pcmds(cmd_list, &cnt, y->write_handler, y, "sys_info", 1000, NULL);
    return 0;
}

static int ylog_read_snapshot(char *buf, int count, FILE *fp, int fd, struct ylog *y) {
    UNUSED(buf);
    UNUSED(count);
    UNUSED(fp);
    UNUSED(fd);
    UNUSED(y);
    return 0;
}

/**
 * system/core/logd/main.cpp
 */
static int drop_privs(char *description) {
    struct sched_param param;
    int pid = getpid();
    int tid = gettid();
    memset(&param, 0, sizeof(param));

#if 0
    if (set_sched_policy(0, SP_BACKGROUND) < 0) {
        ylog_error("ylog drop_privs pid=%d, tid=%d for %s failed set_sched_policy: %s\n",
                pid, tid, description, strerror(errno));
        return -1;
    }

    if (sched_setscheduler((pid_t) 0, SCHED_BATCH, &param) < 0) {
        ylog_error("ylog drop_privs pid=%d, tid=%d for %s failed sched_setscheduler: %s\n",
                pid, tid, description, strerror(errno));
        return -1;
    }

    if (setpriority(PRIO_PROCESS, 0, ANDROID_PRIORITY_BACKGROUND) < 0) {
        ylog_error("ylog drop_privs pid=%d, tid=%d for %s failed setpriority: %s\n",
                pid, tid, description, strerror(errno));
        return -1;
    }
#endif

    if (prctl(PR_SET_KEEPCAPS, 1) < 0) {
        ylog_error("ylog drop_privs pid=%d, tid=%d for %s failed prctl: %s\n",
                pid, tid, description, strerror(errno));
        return -1;
    }

    /**
     * we get this groups[] from system/core/adb/adb_main.cpp
     */
    /* add extra groups:
    ** AID_ADB to access the USB driver
    ** AID_LOG to read system logs (adb logcat)
    ** AID_INPUT to diagnose input issues (getevent)
    ** AID_INET to diagnose network issues (ping)
    ** AID_NET_BT and AID_NET_BT_ADMIN to diagnose bluetooth (hcidump)
    ** AID_SDCARD_R to allow reading from the SD card
    ** AID_SDCARD_RW to allow writing to the SD card
    ** AID_NET_BW_STATS to read out qtaguid statistics
    */
    gid_t groups[] = { AID_ADB, AID_LOG, AID_INPUT, AID_INET, AID_NET_BT,
                       AID_NET_BT_ADMIN, AID_SDCARD_R, AID_SDCARD_RW,
                       AID_NET_BW_STATS };
    if (setgroups(sizeof(groups)/sizeof(groups[0]), groups) != 0) {
        ylog_error("ylog drop_privs pid=%d, tid=%d for %s failed setgroups: %s\n",
                pid, tid, description, strerror(errno));
        return -1;
    }

    if (setgid(DROP_PRIVS_GID) != 0) {
        ylog_error("ylog drop_privs pid=%d, tid=%d for %s failed setgid: %s\n",
                pid, tid, description, strerror(errno));
        return -1;
    }

    if (setuid(DROP_PRIVS_UID) != 0) {
        ylog_error("ylog drop_privs pid=%d, tid=%d for %s failed setuid: %s\n",
                pid, tid, description, strerror(errno));
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
        ylog_error("ylog drop_privs pid=%d, tid=%d for %s failed capset: %s\n",
                pid, tid, description, strerror(errno));
        return -1;
    }

    ylog_debug("ylog drop_privs pid=%d, tid=%d for %s success\n", pid, tid, description);

    return 0;
}

static int check_sdcard_mounted_default(char *sdcard, int count) {
    char str[4096];
    char *token, *last;
    const char *mountPath = "/mnt/media_rw/";
    FILE *fp = fopen("/proc/mounts", "r");
    /**
     * # cat /proc/mounts | grep vfat
     * /dev/block/vold/public:179,129 /mnt/media_rw/0B07-10F1 vfat rw,dirsync,nosuid,nodev,noexec,relatime,uid=1023,gid=1023,fmask=0007,dmask=0007,allow_utime=0020,codepage=437,iocharset=iso8859-1,shortname=mixed,utf8,errors=remount-ro 0 0
     */
    sdcard[0] = 0;

    if(fp != NULL) {
        while (fgets(str, sizeof(str), fp)) {
            token = strtok_r(str, " ", &last);
            if (token != NULL) {
                token = strtok_r(NULL, " ", &last);
                if (strstr(token, mountPath)) {
                    snprintf(sdcard, count, "/storage/%s", token + strlen(mountPath));
                    break;
                }
            }
        }
        fclose(fp);
    }

    if (!access(sdcard, F_OK))
        return 1;

    return 0;
}

static int os_check_sdcard_online(char *sdcard_path, int count) {
    char sdcard_status[PROPERTY_VALUE_MAX];

    property_get("vold.sdcard0.state", sdcard_status, "");
    if (sdcard_status[0]) {
        if (!strncmp(sdcard_status, "mounted", 7)) {
            if (sdcard_path)
                property_get("vold.sdcard0.path", sdcard_path, "/storage/sdcard0");
            return 1;
        }
        return 0;
    }

    if (sdcard_path == NULL) {
        sdcard_path = sdcard_status;
        count = sizeof(sdcard_status);
    }

    return check_sdcard_mounted_default(sdcard_path, count);
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

    if (os_check_sdcard_online(sdcard_path, sizeof(sdcard_path))) {
        keep_historical_folder_numbers = c->keep_historical_folder_numbers;
        historical_folder_root = pos->historical_folder_root_last;
        pos->sdcard_online = 1;
    } else {
        strcpy(sdcard_path, YLOG_ROOT_FOLDER);
        keep_historical_folder_numbers = c->keep_historical_folder_numbers;
        historical_folder_root = pos->historical_folder_root_last;
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
            ylog_root_folder_delete(path, historical_folder_root_tmp, c->keep_historical_folder_numbers, c->keep_historical_folder_numbers);

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
static int ydst_fwrite_kernel(char *buf, int count, int fd, char *desc) {
    int ret = 0;
    if (count) {
        ret = fd_write(buf, count, fd, desc);
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
    SEND(fd, buf, snprintf(buf, buf_size, "%s = %s\n", property, pp), MSG_NOSIGNAL);
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
        if (len == fd_write(buf, len, pos->fd_kmsg, "cmd_print2kernel"))
            ret = 0;
    }
    if (ret)
        SEND(fd, buf, snprintf(buf, buf_size, "%s\n", "failed"), MSG_NOSIGNAL);
    return 0;
}

static int cmd_anr(struct command *cmd, char *buf, int buf_size, int fd, int index, struct ylog_poll *yp) {
    UNUSED(cmd);
    UNUSED(buf);
    UNUSED(buf_size);
    UNUSED(fd);
    UNUSED(index);
    UNUSED(yp);
    pcmd("cp /data/ylog/ylog_journal_file /data/anr/traces.txt", NULL, NULL, NULL, "cmd_anr", -1, -1, NULL);
    return 0;
}

static int cmd_tombstone(struct command *cmd, char *buf, int buf_size, int fd, int index, struct ylog_poll *yp) {
    UNUSED(cmd);
    UNUSED(buf);
    UNUSED(buf_size);
    UNUSED(fd);
    UNUSED(index);
    UNUSED(yp);
    pcmd("cp /data/ylog/ylog_journal_file /data/tombstones/tombstone_90", NULL, NULL, NULL, "cmd_tombstone", -1, -1, NULL);
    pcmd("cp /data/ylog/ylog_journal_file /data/tombstones/tombstone_91", NULL, NULL, NULL, "cmd_tombstone", -1, -1, NULL);
    pcmd("cp /data/ylog/ylog_journal_file /data/tombstones/tombstone_92", NULL, NULL, NULL, "cmd_tombstone", -1, -1, NULL);
    pcmd("cp /data/ylog/ylog_journal_file /data/tombstones/tombstone_93", NULL, NULL, NULL, "cmd_tombstone", -1, -1, NULL);
    return 0;
}

static int cmd_monkey(struct command *cmd, char *buf, int buf_size, int fd, int index, struct ylog_poll *yp) {
    UNUSED(cmd);
    UNUSED(index);
    UNUSED(yp);
    char *last;
    char *value = NULL;
    const char *property = "ylog.monkey";
    char *level;
    char pp[PROPERTY_VALUE_MAX];
    strtok_r(buf, " ", &last);
    value = strtok_r(NULL, " ", &last);
    if (value) {
        strcpy(pp, value);
        property_set(property, pp);
    }
    property_get(property, pp, "");
    SEND(fd, buf, snprintf(buf, buf_size, "%s = %s\n", property, pp), MSG_NOSIGNAL);
    return 0;
}

static struct command os_commands[] = {
    {"test", "test from android", cmd_test, NULL},
    {"\n", NULL, os_cmd_help, (void*)os_commands},
    {"rootdir", "get the log disk root dir", cmd_rootdir, NULL},
    {"cpath_last", "get the last_ylog path", cmd_cpath_last, NULL},
    {"setprop", "set property, ex. ylog_cli setprop persist.ylog.enabled 1", cmd_setprop, NULL},
    {"at", "send AT command to cp side, ex. ylog_cli at AT+ARMLOG=1 or ylog_cli at AT+CGMR", cmd_at, NULL},
    {"print2android", "write data to android system log", cmd_print2android, NULL},
    {"print2kernel ", "write data to kernel log", cmd_print2kernel, NULL},
    {"anr", "trigger anr action for auto test", cmd_anr, NULL},
    {"tombstone", "trigger tombstone action for auto test", cmd_tombstone, NULL},
    {"monkey", "mark the status of monkey, ex. ylog_cli monkey 1 or ylog_cli monkey 0", cmd_monkey, NULL},
    {"atloop", "send AT loop command to cp side, ex. ylog_cli atloop AT+SPREF=\\\"AUTODLOADER\\\" for ResearchDownload auto pac flash", cmd_atloop, NULL},
    {"pac", "Trigger phone to download mode for ResearchDownload auto pac flash", cmd_pac, NULL},
    {NULL, NULL, NULL, NULL}
};

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
    if (state == YLOG_STOP || state == YLOG_RUN)
        ylog_debug("ylog %s setprop %s %s\n", y->name, buf, value);
    property_set(buf, value);
}

static void pthread_create_hook(struct ylog *y, void *args, const char *fmt, ...) {
    UNUSED(args);
    va_list ap;
    char buf[4096];
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    if (y == NULL || ((y->mode & YLOG_READ_ROOT_USER) != YLOG_READ_ROOT_USER))
        drop_privs(buf);
    else {
        if (setgid(DROP_PRIVS_GID) != 0)
            ylog_error("ylog_ready failed setgid %d: %s\n", DROP_PRIVS_GID, strerror(errno));
        else
            ylog_debug("ylog %s setgid to %d success\n", buf, DROP_PRIVS_GID);
    }
}

static void ready_go(void) {
    int i;
    pid_t ctid;
    struct ylog *yi;
    struct ylog_event_thread *e;
    char buf[4096];
    int count = sizeof buf;
    struct ylog *y = pos->snapshot;
    struct ydst *ydst = y->ydst;
    int fd;

    snprintf(buf, count, "%s/%s/phone.info", ydst->root_folder, ydst->file);
    fd = open(buf, O_RDWR | O_CREAT | O_APPEND, 0664);
    if (fd < 0) {
        ylog_error("open %s failed: %s\n", buf, strerror(errno));
        return;
    }

    for_each_ylog(i, yi, NULL) {
        if (yi->name == NULL)
            continue;
        ctid = yi->ydst->cache ? yi->ydst->cache->tid : -1;
        write(fd, buf, snprintf(buf, count,
                    "[ylog] %s pid=%d, tid=%d, cache tid=%d\n", yi->name, yi->pid, yi->tid, ctid));
    }

    for_each_event_thread_start(e)
    write(fd, buf, snprintf(buf, count, "[event] %s pid=%d, tid=%d\n", e->yewait.name, e->pid, e->tid));
    for_each_event_thread_end();

    close(fd);

    ylog_info("ylog ready to go\n");
}

static int os_inotify_handler_anr(struct ylog_inotify_cell *pcell, int timeout, struct ylog *y) {
    static int cnt = 1;
    char *cmd_list[] = {
        "cat /data/anr/traces.txt",
        NULL
    };

    ylog_debug("os_inotify_handler_anr is called for '%s %s' %dms %s now\n",
                pcell->pathname ? pcell->pathname:"", pcell->filename, pcell->timeout, timeout ? "timeout":"normal");

    if (pos->anr_fast) {
        char buf[4];
        int fd = open("/data/anr/traces.txt", O_RDONLY);
        if (fd < 0) {
            ylog_error("open %s fail.%s", "/data/anr/traces.txt", strerror(errno));
            return -1;
        }
        LSEEK(fd, -3, SEEK_END);
        buf[0] = 0;
        buf[3] = 0;
        if ((read(fd, buf, 3) != 3) || (strcmp(buf, "EOF"))) {
            ylog_error("read %s fail.%s", "/data/anr/traces.txt", strerror(errno));
            close(fd);
            return -1;
        }
        close(fd);
    }

    pcmds(cmd_list, &cnt, y->write_handler, y, "ylog_traces", 1000, NULL);

    return -1;
}

static int os_inotify_handler_anr_nowrap(struct ylog_inotify_cell *pcell, int timeout, struct ylog *y);
static int os_inotify_handler_anr_delete_create(struct ylog_inotify_cell *pcell, int timeout, struct ylog *y) {
    UNUSED(timeout);
    UNUSED(y);
    unsigned long event_mask = pcell->event_mask;
    if (event_mask & IN_DELETE) {
        ylog_info("Wrote stack traces to /data/anr/traces.txt, os_inotify_handler_anr is deleted\n");
#ifdef ANR_FAST_COPY_MODE
        if (pcell->status & YLOG_INOTIFY_WAITING_TIMEOUT) {
            /* copy from ylog_inotfiy_file_handler_timeout to skip this timeout process because of unlink action */
            pcell->status &= ~YLOG_INOTIFY_WAITING_TIMEOUT;
            os_inotify_handler_anr_nowrap(pcell, 0, y);
            pcell->step = 0;
        }
#endif
    }
    if (event_mask & IN_CREATE) {
        ylog_info("Wrote stack traces to /data/anr/traces.txt, os_inotify_handler_anr is created\n");
#ifdef ANR_FAST_COPY_MODE
        pos->anr_fast_copy_count = 0;
#endif
    }
    event_mask &= ~(IN_DELETE | IN_CREATE);
#ifdef ANR_FAST_COPY_MODE
    if (event_mask && timeout == 0) {
        /* IN_MODIFY */
        int fd_from = open(ANR_FAST_COPY_MODE_ANR_FILE, O_RDONLY);
        if (fd_from < 0) {
            ylog_error("open %s fail.%s", ANR_FAST_COPY_MODE_ANR_FILE, strerror(errno));
        } else {
            static int warning = 0;
            int fd_to = open(ANR_FAST_COPY_MODE_TMP, O_RDWR | O_CREAT | O_TRUNC, 0664);
            if (fd_to >= 0) {
                struct stat stat_dst0, stat_dst;
                /**
                 * Todo...
                 * to avoid anr data lost, we do not add timeout gap filter,
                 * ex. let copy_file action happens every 200ms
                 */
                copy_file(fd_to, fd_from, 10 * 1024 * 1024, ANR_FAST_COPY_MODE_TMP);
                pos->anr_fast_copy_count++;
                close(fd_to);
                if (access(ANR_FAST_COPY_MODE_YLOG, F_OK) == 0) {
                    if (stat(ANR_FAST_COPY_MODE_TMP, &stat_dst0) || stat(ANR_FAST_COPY_MODE_YLOG, &stat_dst))
                        ylog_error("stat %s, %s failed: %s\n", ANR_FAST_COPY_MODE_TMP,
                                ANR_FAST_COPY_MODE_YLOG, strerror(errno));
                    else {
                        if (stat_dst0.st_size > stat_dst.st_size) {
                            warning = 0;
                            rename(ANR_FAST_COPY_MODE_TMP, ANR_FAST_COPY_MODE_YLOG);
                        } else {
                            if (stat_dst0.st_size < stat_dst.st_size && warning == 0) {
                                warning = 1;
                                ylog_critical("os_inotify_handler_anr_delete_create does not delete, why?\n");
                            }
                        }
                    }
                } else {
                    warning = 0;
                    rename(ANR_FAST_COPY_MODE_TMP, ANR_FAST_COPY_MODE_YLOG);
                }
            } else {
                warning = 0;
                ylog_error("open %s fail.%s", ANR_FAST_COPY_MODE_TMP, strerror(errno));
            }
            close(fd_from);
        }
    }
#endif
    return event_mask ? 1:0;
}

#ifdef ANR_OR_TOMSTONE_UNIFILE_THREAD_COPY_MODE
static void *ylog_pthread_handler_anr_unifile_thread_copy_bottom_half(void *arg) {
    struct pcmds_print2file_args *ppa = arg;
    if (ppa->fd_dup_flag != PCMDS_PRINT2FILE_FD_DUP || ppa->fd < 0)
        return NULL;
    char *cmd_list[] = {
        "logcat -d",
        "dmesg",
        NULL
    };
    int cur_cnt = 1;
    ppa->cmds = cmd_list;
    ppa->file = NULL;
    ppa->cnt = &cur_cnt;
    ppa->fd_dup_flag = 0; /* assigne 0, then pcmds_print2fd will close(fd); */
    pcmds_print2fd(ppa);
    if (ppa->malloced)
        free(ppa);
    return NULL;
}
#endif

/**
 * because anr process should as soon as possible,
 * we assume when anr happens, cpath will not change
 * pcmd_print2file calls pcmd to copy file to the current root folder
 * because of pcmd may call logcat -d and some other time costing commands,
 * so we can't use PCMDS_YLOG_CALL_LOCK(y); in the whole function
 * it may happen following thing:
 * 1. /data/ylog/traces is writing
 * 2. sdcard ready
 * 3. move whole /data/ylog to sdcard (because lacking of PCMDS_YLOG_CALL_LOCK)
 * 4. still write /data/ylog/traces
 * (we can use a.mutex->locked() struct pcmds_print2file_args .locked++; a.mutex->unlock()  in anr and tombstones function)
 * if .locked > 1, then ....
 */
static void pcmds_ylog_anr_nowrap_callback(struct ylog *y, char *buf, int count, struct ylog_inotify_cell *pcell) {
    static int cnt = 1;
    char timeBuf[32];
    int len, cur_cnt;
    PCMDS_YLOG_CALL_LOCK(y);
    /**
     * if cnt is less than 20, although quta has reached,
     * we also need to continue save it, because some other ylog
     * fill full the shared ydst, but here does not even have 20 counts
     */
    if (ydst->size < ydst->max_size_now || cnt < 20) {
        char timeBuf0[32];
        char timeBuf1[32];
        cur_cnt = cnt;
        ylog_get_format_time_year(timeBuf);

        snprintf(buf, count, "%s/%s/%04d.%s.anr", ydst->root_folder, ydst->file, cur_cnt, timeBuf);
#ifdef ANR_OR_TOMSTONE_UNIFILE_MODE
        char *cmd_list[] = {
#ifdef ANR_FAST_COPY_MODE
            "cat "ANR_FAST_COPY_MODE_YLOG,
#else
            "cat /data/anr/traces.txt",
#endif
#ifndef ANR_OR_TOMSTONE_UNIFILE_THREAD_COPY_MODE
            "logcat -d",
            "dmesg",
#endif
            NULL
        };
        cur_cnt = 0;
        struct pcmds_print2file_args ppa = {
            .cmds = cmd_list,
            .file = buf,
            .flags = -1,
            .cnt = &cur_cnt,
            .w = NULL,
            .y = y,
            .prefix = "ylog.anr",
            .millisecond = 1000,
            .max_size = 8*1024*1024,
#ifdef ANR_OR_TOMSTONE_UNIFILE_THREAD_COPY_MODE
            .fd_dup_flag = PCMDS_PRINT2FILE_FD_DUP,
#endif
            .locked = PCMDS_PRINT2FILE_LOCKED,
        };
        ppa.fd = -1;
        pcmds_print2file(&ppa);
#ifdef ANR_OR_TOMSTONE_UNIFILE_THREAD_COPY_MODE
        struct pcmds_print2file_args *tppa = malloc(sizeof(struct pcmds_print2file_args));
        if (tppa) {
            *tppa = ppa;
            tppa->malloced = 1;
            tppa->locked = 0;
            if (ylog_pthread_create(ylog_pthread_handler_anr_unifile_thread_copy_bottom_half, tppa)) {
                tppa->locked = PCMDS_PRINT2FILE_LOCKED;
                ylog_pthread_handler_anr_unifile_thread_copy_bottom_half(tppa);
            }
        } else {
            ylog_error("malloc %s fail.%s", "pcmds_print2file_args anr", strerror(errno));
            ylog_pthread_handler_anr_unifile_thread_copy_bottom_half(&ppa);
        }
#endif
#else
        struct pcmds_print2file_args ppa = {
#ifdef ANR_FAST_COPY_MODE
            .cmd = "cat "ANR_FAST_COPY_MODE_YLOG,
#else
            .cmd = "cat /data/anr/traces.txt",
#endif
            .file = buf,
            .flags = -1,
            .cnt = &cur_cnt,
            .w = NULL,
            .y = y,
            .prefix = "ylog.anr",
            .millisecond = 1000,
            .max_size = -1,
            .locked = PCMDS_PRINT2FILE_LOCKED,
        };
        pcmd_print2file(&ppa);

        cur_cnt = cnt;
        snprintf(buf, count, "%s/%s/%04d.%s.anr.kmsg", ydst->root_folder, ydst->file, cur_cnt, timeBuf);
        ppa.cmd = "dmesg";
        ppa.max_size = 3*1024*1024;
        pcmd_print2file(&ppa);

        cur_cnt = cnt;
        snprintf(buf, count, "%s/%s/%04d.%s.anr.logcat", ydst->root_folder, ydst->file, cur_cnt, timeBuf);
        ppa.cmd = "logcat -d";
        ppa.max_size = 5*1024*1024;
        pcmd_print2file(&ppa);
#endif
        cnt++;
        ylog_tv2format_time(timeBuf0, &pcell->tvf); /* after traces.txt copied, format following info for more faster */
        ylog_tv2format_time(timeBuf1, &pcell->tv);
        ylog_info("ylog traces create %s, %d, %s~ %s, anr_fast_copy_count=%d\n",
                buf, pcell->step, timeBuf0, timeBuf1
#ifdef ANR_FAST_COPY_MODE
                ,pos->anr_fast_copy_count
#else
                ,0
#endif
                );
#ifdef ANR_FAST_COPY_MODE
        unlink(ANR_FAST_COPY_MODE_TMP);
        unlink(ANR_FAST_COPY_MODE_YLOG);
        pos->anr_fast_copy_count = 0;
#endif
    } else {
        ylog_critical("ylog %s is forced stop, cnt=%d, because ydst %s has "
                "reached max_size %lld/%lld in pcmds_ylog_anr_nowrap_callback\n",
                y->name, cnt, ydst->file, ydst->size, ydst->max_size_now);
    }
    PCMDS_YLOG_CALL_UNLOCK();
}

static int os_inotify_handler_anr_nowrap(struct ylog_inotify_cell *pcell, int timeout, struct ylog *y) {
    static struct ylog *sys_info = NULL;
    char buf[PATH_MAX];
    ylog_info("os_inotify_handler_anr_nowrap is called for '%s %s' %dms %s now\n",
                pcell->pathname ? pcell->pathname:"", pcell->filename, pcell->timeout, timeout ? "timeout":"normal");
    pcmds_ylog_anr_nowrap_callback(y, buf, sizeof buf, pcell);
    if (sys_info == NULL)
        sys_info = ylog_get_by_name("sys_info");
    if (sys_info)
        sys_info->thread_restart(sys_info, 0); /* Trigger sys_info to capture again */
    return -1;
}

static int os_inotify_handler_tombstone(struct ylog_inotify_cell *pcell, int timeout, struct ylog *y) {
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

    ylog_debug("os_inotify_handler_tombstone is called for '%s' %dms %s now\n",
                pcell->pathname ? pcell->pathname:"", pcell->timeout, timeout ? "timeout":"normal");

    for (i = 0; i < file->num; i++) {
        a = file->files_array + i * file->len;
        if (a[0]) {
            if (cmd_start_idx < ((int)ARRAY_LEN(cmd_list) - 1))
                cmd_list[cmd_start_idx++] = a;
            ylog_debug("os_inotify_handler_tombstone -> %s\n", a);
        }
    }
    cmd_list[cmd_start_idx] = NULL;

    pcmds(cmd_list, &cnt, y->write_handler, y, "ylog_tombstones", 1000, NULL);

    for (i = 0; i < file->num; i++) {
        a = file->files_array + i * file->len;
        a[0] = 0;
    }

    return -1;
}

#ifdef ANR_OR_TOMSTONE_UNIFILE_THREAD_COPY_MODE
static void *ylog_pthread_handler_tombstone_unifile_thread_copy_bottom_half(void *arg) {
    struct pcmds_print2file_args *ppa = arg;
    if (ppa->fd_dup_flag != PCMDS_PRINT2FILE_FD_DUP || ppa->fd < 0)
        return NULL;
    char *cmd_list[] = {
        "logcat -d",
        "dmesg",
        NULL
    };
    int cur_cnt = 1;
    ppa->cmds = cmd_list;
    ppa->file = NULL;
    ppa->cnt = &cur_cnt;
    ppa->fd_dup_flag = 0; /* assigne 0, then pcmds_print2fd will close(fd); */
    pcmds_print2fd(ppa);
    if (ppa->malloced)
        free(ppa);
    return NULL;
}
#endif

static void pcmds_ylog_tombstone_nowrap_callback(struct ylog *y, char *buf, int count, struct ylog_inotify_cell *pcell) {
    static int cnt = 1;
    char timeBuf[32];
    int len, cur_cnt;
    int i;
    char *a;
    char **cmd;
    char *cmd_list[30] = {
        "cat /data/tombstones/*",
        NULL
    };
    int cmd_start_idx = 0;
    struct ylog_inotify_cell_args *pcella = pcell->args;
    struct ylog_inotify_files *file = &pcella->file;

    ylog_get_format_time_year(timeBuf);

    for (i = 0; i < file->num; i++) {
        a = file->files_array + i * file->len;
        if (a[0]) {
            if (cmd_start_idx < ((int)ARRAY_LEN(cmd_list) - 1))
                cmd_list[cmd_start_idx++] = a;
            ylog_debug("os_inotify_handler_tombstone -> %s\n", a);
        }
    }
    cmd_list[cmd_start_idx] = NULL;

    /**
     * if cnt is less than 20, although quta has reached,
     * we also need to continue save it, because some other ylog
     * fill full the shared ydst, but here does not even have 20 counts
     */
    PCMDS_YLOG_CALL_LOCK(y);
    if (ydst->size < ydst->max_size_now || cnt < 20) {
        char timeBuf0[32];
        char timeBuf1[32];
        ylog_tv2format_time(timeBuf0, &pcell->tvf);
        ylog_tv2format_time(timeBuf1, &pcell->tv);
        snprintf(buf, count, "%s/%s/%04d.%s.tombstone", ydst->root_folder, ydst->file, cnt, timeBuf);
        ylog_info("ylog traces create %s, %d, %s~ %s\n", buf, pcell->step, timeBuf0, timeBuf1);
        struct pcmds_print2file_args ppa = {
            .cmds = NULL,
            .file = buf,
            .flags = -1,
            .cnt = &cur_cnt,
            .w = NULL,
            .y = y,
            .prefix = "ylog.tombstone",
            .millisecond = 1000,
            .max_size = 8*1024*1024,
#ifdef ANR_OR_TOMSTONE_UNIFILE_THREAD_COPY_MODE
            .fd_dup_flag = PCMDS_PRINT2FILE_FD_DUP,
#endif
            .locked = PCMDS_PRINT2FILE_LOCKED,
        };
        for (cmd = cmd_list; *cmd; cmd++) {
            cur_cnt = cnt;
            snprintf(buf, count, "%s/%s/%04d.%s.tombstone", ydst->root_folder, ydst->file, cur_cnt, timeBuf);
#ifdef ANR_OR_TOMSTONE_UNIFILE_MODE
            char *cmd_list_one[] = {
                *cmd,
#ifndef ANR_OR_TOMSTONE_UNIFILE_THREAD_COPY_MODE
                "logcat -d",
                "dmesg",
#endif
                NULL
            };
            cur_cnt = 0;
            ppa.cmds = cmd_list_one;
            ppa.fd = -1;
            pcmds_print2file(&ppa);
#ifdef ANR_OR_TOMSTONE_UNIFILE_THREAD_COPY_MODE
            struct pcmds_print2file_args *tppa = malloc(sizeof(struct pcmds_print2file_args));
            if (tppa) {
                *tppa = ppa;
                tppa->malloced = 1;
                tppa->locked = 0;
                if (ylog_pthread_create(ylog_pthread_handler_tombstone_unifile_thread_copy_bottom_half, tppa)) {
                    tppa->locked = PCMDS_PRINT2FILE_LOCKED;
                    ylog_pthread_handler_tombstone_unifile_thread_copy_bottom_half(&ppa);
                }
            } else {
                ylog_error("malloc %s fail.%s", "pcmds_print2file_args tombstone", strerror(errno));
                ylog_pthread_handler_tombstone_unifile_thread_copy_bottom_half(&ppa);
            }
#endif
#else
            ppa.cmd = *cmd;
            ppa.max_size = -1;
            pcmd_print2file(&ppa);

            cur_cnt = cnt;
            snprintf(buf, count, "%s/%s/%04d.%s.tombstone.kmsg", ydst->root_folder, ydst->file, cur_cnt, timeBuf);
            ppa.cmd = "dmesg";
            ppa.max_size = 3*1024*1024;
            pcmd_print2file(&ppa);

            cur_cnt = cnt;
            snprintf(buf, count, "%s/%s/%04d.%s.tombstone.logcat", ydst->root_folder, ydst->file, cur_cnt, timeBuf);
            ppa.cmd = "logcat -d";
            ppa.max_size = 5*1024*1024;
            pcmd_print2file(&ppa);
#endif
            cnt++;
        }
    } else {
        ylog_critical("ylog %s is forced stop, cnt=%d, because ydst %s has "
                "reached max_size %lld/%lld in pcmds_ylog_tombstone_nowrap_callback\n",
                y->name, cnt, ydst->file, ydst->size, ydst->max_size_now);
    }
    PCMDS_YLOG_CALL_UNLOCK();

    for (i = 0; i < file->num; i++) {
        a = file->files_array + i * file->len;
        a[0] = 0;
    }
}

static int os_inotify_handler_tombstone_nowrap(struct ylog_inotify_cell *pcell, int timeout, struct ylog *y) {
    char buf[PATH_MAX];
    ylog_info("os_inotify_handler_tombstone_nowrap is called for '%s' %dms %s now\n",
                pcell->pathname ? pcell->pathname:"", pcell->timeout, timeout ? "timeout":"normal");
    pcmds_ylog_tombstone_nowrap_callback(y, buf, sizeof buf, pcell);
    return -1;
}

static void ylog_ready(void) {
    struct ylog *y;
    char yk[PROPERTY_VALUE_MAX];
    int count;
    struct context *c = global_context;
    struct ydst_root *root = global_ydst_root;

    property_get("ylog.killed", yk, "-1");
    count = atoi(yk);
    snprintf(yk, sizeof yk, "%d", count + 1);
    property_set("ylog.killed", yk);

    if (root->quota_new && root->quota_now != root->quota_new)
        ydst_root_quota(NULL, root->quota_new); /* all ylog threads are ready to run */

    c->command_loop_ready = 1; /* mark it to work command_loop(); */

    if (setgid(DROP_PRIVS_GID) != 0)
        ylog_error("ylog_ready failed setgid %d: %s\n", DROP_PRIVS_GID, strerror(errno));
    ylog_snapshot_startup(count == -1 ? 1:0); /* count == -1 meas powering on the phone just */

#ifdef HAVE_YLOG_INFO
    y = ylog_get_by_name("info");
    if (y)
        ylog_trigger_and_wait_for_finish(y);
#endif

    pos->fd_kmsg = open("/dev/kmsg", O_WRONLY);
    if (pos->fd_kmsg < 0)
        ylog_error("open %s fail. %s", "/dev/kmsg", strerror(errno));

    mkdirs("/data/anr/"); /* For ylog traces.txt */
    mkdirs("/data/tombstones/"); /* For ylog tombstone_00 ~ tombstone_09 */

    ylog_trigger_all(global_ylog); /* mark ylog status from ready to run */

    ylog_os_event_timer_create("android", pos->sdcard_online ? 5*1000:1*1000, event_timer_handler, (void*)-1);

    os_hooks.pthread_create_hook(NULL, NULL, "main ylog_ready");
}

static struct ylog_keyword os_ylog_keyword[] = {
    {"loglevel", load_loglevel},
    {"keep_historical_folder_numbers", load_keep_historical_folder_numbers},
    {"ylog", load_ylog},
    {NULL, NULL},
};

static struct context os_context[M_MODE_NUM] = {
    [M_USER] = {
        .config_file = "1.xml",
        .filter_plugin_path = YLOG_FILTER_PATH,
        .journal_file = YLOG_JOURNAL_FILE,
        .ylog_config_file = YLOG_CONFIG_FILE,
        .model = C_MINI_LOG,
        .loglevel = LOG_WARN,
        .keep_historical_folder_numbers_default = 5,
        .ylog_keyword = os_ylog_keyword,
        .ylog_snapshot_list = os_ylog_snapshot_list,
    },
    [M_USER_DEBUG] = {
        .config_file = "2.xml",
        .filter_plugin_path = YLOG_FILTER_PATH,
        .journal_file = YLOG_JOURNAL_FILE,
        .ylog_config_file = YLOG_CONFIG_FILE,
        .model = C_FULL_LOG,
        .loglevel = LOG_INFO,
        .keep_historical_folder_numbers_default = 5,
        .ylog_keyword = os_ylog_keyword,
        .ylog_snapshot_list = os_ylog_snapshot_list,
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
    /* ylog_cli quota 1024 */
    struct ynode os_ynode[] = {
        /* kernel/ */ {
           .ylog = {
                {
                    .name = "kernel",
                    .type = FILE_NORMAL,
                    .file = "/proc/kmsg",
                    .mode = YLOG_READ_MODE_BLOCK | YLOG_READ_LEN_MIGHT_ZERO | YLOG_READ_MODE_BLOCK_RESTART_ALWAYS,
                    .restart_period = 1000,
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
                .num = 8,
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
                .num = 8,
                .timeout = 1000,
                .debuglevel = CACHELINE_DEBUG_CRITICAL,
            },
        },
        /* tcpdump/ */ {
            .ylog = {
                {
                    .name = "tcpdump",
                    .type = FILE_POPEN,
                    .file = "tcpdump -i any -p -s 0 -C 60 -U -w -",
                    .mode = YLOG_READ_MODE_BLOCK | YLOG_READ_MODE_BINARY | YLOG_READ_ROOT_USER,
                    .restart_period = 2000,
                    .status = YLOG_DISABLED,
                    .raw_data = 1,
                },
            },
            .ydst = {
                .file = "tcpdump/",
                .file_name = "tcpdump.cap",
                .max_segment = 3,
                .max_segment_size = 50*1024*1024,
                .nowrap = 1,
                .write_data2cache_first = 1,
                .ytag = 1,
            },
            .cache = {
                .size = 512 * 1024,
                .num = 40,
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
                    .mode = YLOG_READ_MODE_BLOCK | YLOG_READ_MODE_BINARY | YLOG_READ_ROOT_USER,
                    .restart_period = 2000,
                    .raw_data = 1,
                },
            },
            .ydst = {
                .file = "hcidump/",
                .file_name = "hcidump.log",
                .max_segment = 4,
                .max_segment_size = 50*1024*1024,
                .nowrap = 1,
                .write_data2cache_first = 1,
                .ytag = 1,
            },
            .cache = {
                .size = 512 * 1024,
                .num = 40,
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
                            .mask = IN_MODIFY | IN_DELETE | IN_CREATE,
                            .type = YLOG_INOTIFY_TYPE_MASK_SUBSET_BIT,
                            /* .timeout = 100, */
                            .handler_prev = os_inotify_handler_anr_delete_create,
                            .handler = os_inotify_handler_anr,
                        },
                        .cells[1] = { /* kill -6 pid of com.xxx */
                            /* this folder must make sure no one will delete tombstones folder */
                            .pathname = "/data/tombstones/",
                            .mask = IN_MODIFY,
                            .type = YLOG_INOTIFY_TYPE_MASK_SUBSET_BIT,
                            .timeout = 1000,
                            .handler = os_inotify_handler_tombstone,
                            .args = &tombstone, /* struct ylog_inotify_cell_args */
                        },
                    },
                },
            },
            .ydst = {
                .file = "traces/",
                .file_name = "traces.log",
                .max_segment = 2,
                .max_segment_size = 50*1024*1024,
                .nowrap = 1,
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
                },
            },
            .ydst = {
                .file = "sys_info/",
                .file_name = "sys_info.log",
                .max_segment = 5,
                .max_segment_size = 50*1024*1024,
            },
        },
        /* ftrace/ */ {
            .ylog = {
                {
                    .name = "ftrace",
                    .type = FILE_NORMAL,
                    .file = "/d/tracing/trace_pipe",
                    .mode = YLOG_READ_MODE_BLOCK,
                    .restart_period = 0,
                    .status = YLOG_DISABLED,
                },
            },
            .ydst = {
                .file = "ftrace/",
                .file_name = "ftrace.log",
                .max_segment = 2,
                .max_segment_size = 10*1024*1024,
            },
            .cache = {
                .size = 512 * 1024,
                .num = 30,
                .timeout = 1000,
                .debuglevel = CACHELINE_DEBUG_CRITICAL,
            },
        },
        /* sgm/cpu_memory/ */ {
            .ylog = {
                {
                    .name = "sgm.cpu_memory",
                    .type = FILE_POPEN,
                    /**
                     * 1000ms per sample will create 200 Bytes data per second,
                     * if we use default 100ms, 1 second will have 10 samples,
                     * the sgm.cpu_memory cpu usage will be 4.6%, it is so huge
                     * so we drop the sample freq to 1 sample per second,
                     * and remove the cache because of the very little throughput
                     */
                    .file = "sgm.cpu_memory -t 1000 -z",
                    .mode = YLOG_READ_MODE_BLOCK | YLOG_READ_LEN_MIGHT_ZERO | YLOG_READ_MODE_BLOCK_RESTART_ALWAYS,
                    .restart_period = 2000,
                    .write_header = ylog_write_header_sgm_cpu_memory_header,
                },
            },
            .ydst = {
                .file = "sgm/cpu_memory/",
                .file_name = "sgm.cpu_memory.log",
                .max_segment = 2,
                .max_segment_size = 35*1024*1024,
            },
            /*
            .cache = {
                .size = 15 * 1024,
                .num = 2,
                .timeout = 5000,
                .debuglevel = CACHELINE_DEBUG_CRITICAL,
            },
            */
        },
        /* snapshot/xxxx */ {
            .ylog = {
                {
                    .name = "snapshot",
                    .type = FILE_NORMAL,
                    .file = "/dev/null",
                    .restart_period = -1,
                    .fread = ylog_read_snapshot,
                },
            },
            .ydst = {
                .file = "snapshot/",
                .file_name = "snapshot.log",
                .max_segment = 2,
                .max_segment_size = 50*1024*1024,
            },
        },
    };

    umask(0);
    mode = M_USER_DEBUG;
    *c = &os_context[mode];

    property_get("persist.ylog.enabled", buf, "1");
    if (buf[0] == '0') {
        print2journal_file_string_with_uptime("persist.ylog.enabled is 0, stop ylog");
        system("stop ylog");
        sleep(2);
        exit(0);
    }

    ylog_warn("ylog start\n");

#ifdef ANR_FAST_COPY_MODE
    unlink(ANR_FAST_COPY_MODE_TMP);
    unlink(ANR_FAST_COPY_MODE_YLOG);
#endif

    ynode_insert_all(os_ynode, (int)ARRAY_LEN(os_ynode));
    other_processor_ylog_insert(); /* in modem.c */
    os_parse_config();

    hook->ylog_read_ylog_debug_hook = ylog_read_ylog_debug_hook;
#ifdef HAVE_YLOG_INFO
    hook->ylog_read_info_hook = ylog_read_info_hook;
#endif
    hook->process_command_hook = process_command_hook;
    hook->cmd_ylog_hook = cmd_ylog_hook;
    hook->ylog_status_hook = ylog_status_hook;
    hook->pthread_create_hook = pthread_create_hook;
    hook->ready_go = ready_go;

    parse_config(global_context->ylog_config_file);

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

    property_get("ylog.killed", buf, "-1");
    if (atoi(buf) == -1)
        print2journal_file_string_with_uptime("power on");

    pos->snapshot = ylog_get_by_name("snapshot");
    y = ylog_get_by_name("traces");
    if (y) {
        if (pos->anr_fast)
            y->yinotify.cells[0].timeout = 50;
        else
            y->yinotify.cells[0].timeout = 2000;
        if (y->ydst->nowrap) {
            y->yinotify.cells[0].handler = os_inotify_handler_anr_nowrap;
            y->yinotify.cells[1].handler = os_inotify_handler_tombstone_nowrap;
        } else {
            y->yinotify.cells[0].handler = os_inotify_handler_anr;
            y->yinotify.cells[1].handler = os_inotify_handler_tombstone;
        }
    }
}
