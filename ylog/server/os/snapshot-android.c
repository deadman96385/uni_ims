/**
 * Copyright (C) 2016 Spreadtrum Communications Inc.
 */

struct ylog_snapshot_args {
    union {
        int ret;
        char *p;
    } u;
    int argc;
    char *argv[16];
    char data[4096];
    int offset;
};

static void ylog_snapshot___case___log(struct ylog_snapshot_args *args);
static void ylog_snapshot_startup(long poweron) {
    struct ylog_snapshot_args args;
    char buf[4096];
    int count = sizeof buf;
    struct ylog *y = pos->snapshot;
    char *file = "/data/ylog/last_kmsg";
    char *file2 = "/data/ylog/last_kmsg.current";

    ylog_snapshot___case___log(&args);

    PCMDS_YLOG_CALL_LOCK(y);
    /**
     * copy last_kmsg to ydst snapshot/ folder
     */
    if (poweron) { /* first power on, move it to file2,
                      file2 will be the current last_kmsg,
                      file1 will be used for waiting next future last_kmsg */
        long fd;
        unlink(file2);
        if (access(file, F_OK) == 0) {
            struct stat st;
            if (stat(file, &st) == 0) {
                if (st.st_size) {
                    snprintf(buf, count, "cp -r %s %s", file, file2);
                    pcmd(buf, NULL, NULL, NULL, "ylog_snapshot_startup", 2000, -1);
                }
            } else {
                ylog_error("fstat %s failed: %s\n", file, strerror(errno));
            }
        }
        /**
         * Because of SELinux policy, if bootloader create /data/ylog/last_kmsg directly
         * SELinux info will lose, so /data/ylog/last_kmsg should be created by android process, so in here
         * ylog will create this file, bootloader should check /data/ylog/last_kmsg first
         * if /data/ylog/last_kmsg exits then overwrite this file, otherwise bootloader give a warning
         * but should not create /data/ylog/last_kmsg in bootloader
         */
        mkdirs_with_file(file);
        // unlink(file);
        fd = open(file, O_RDWR | O_CREAT | O_TRUNC, 0664);
        if (fd < 0) {
            ylog_critical("%s %s Failed to open %s\n", __func__, file, strerror(errno));
        } else {
            close(fd);
        }
    }
    pcmds_ylog_copy_file(file2, "last_kmsg", buf, count, y);

    /**
     * copy ylog_journal_file to snapshot/ folder
     */
#ifndef HAVE_YLOG_JOURNAL
    pcmds_ylog_copy_file(global_context->journal_file, NULL, buf, count, y);
#endif
    pcmds_ylog_copy_file(YLOG_CONFIG, NULL, buf, count, y);
    PCMDS_YLOG_CALL_UNLOCK();
}

static void ylog_snapshot___case___log(struct ylog_snapshot_args *args) {
    char timeBuf[32];
    struct ylog *y = pos->snapshot;
    struct ydst *ydst = y->ydst;

    ylog_get_format_time_year(timeBuf);
    pcmd_snapshot("dmesg", y, 1000, 3*1024*1024,
            "%s/%slog/%s/kmsg", ydst->root_folder, ydst->file, timeBuf);
    pcmd_snapshot("logcat -d", y, 1000, 5*1024*1024,
            "%s/%slog/%s/logcat", ydst->root_folder, ydst->file, timeBuf);
    snprintf(args->data, sizeof args->data, "%s/%slog/%s/", ydst->root_folder, ydst->file, timeBuf);
}

static void ylog_snapshot___case___mtp(struct ylog_snapshot_args *args) {
    struct ylog *y = pos->snapshot;
    struct ydst *ydst = y->ydst;
    char *mtp_path = "ylog";

    if (args->argc)
        mtp_path = args->argv[0];
    snprintf(args->data, sizeof args->data,
            "am broadcast -a android.intent.action.MEDIA_SCANNER_SCAN_DIR -d file:///%s/%s",
            pos->ylog_root_path_latest, mtp_path);
    pcmd_snapshot(args->data, y, 2*60*1000, -1, "/dev/null");
    snprintf(args->data, sizeof args->data, "%s/%s", pos->ylog_root_path_latest, mtp_path);
}

static void ylog_snapshot___case___screen(struct ylog_snapshot_args *args) {
    struct ylog *y = pos->snapshot;
    struct ydst *ydst = y->ydst;

    snprintf(args->data, sizeof args->data, "mkdir -p %s/%sscreen", ydst->root_folder, ydst->file);
    if (access(args->data + 9, F_OK))
        pcmd(args->data, NULL, NULL, y, "ylog_snapshot___case___screen", 10*1000, -1);
    if (args->argc == 0) {
        char timeBuf[32];
        ylog_get_format_time_year(timeBuf);
        snprintf(args->data, sizeof args->data,
                "screencap -p %s/%sscreen/%s.png",
                ydst->root_folder, ydst->file, timeBuf);
    } else
        snprintf(args->data, sizeof args->data,
                "screencap -p %s", args->argv[0]);

    pcmd_snapshot_exec(args->data, y, 20*1000, -1);
    args->offset = 13; /* length of "screencap -p " */
}

static struct ylog_snapshot_list_s {
    char *name;
    char *usage;
    void (*f)(struct ylog_snapshot_args *args);
} ylog_snapshot_list[] = {
    { "log", "snapshot current android & kernel log, ex. ylog_cli snapshot log", ylog_snapshot___case___log },
    { "mtp", "snapshot current sdcard contents for mtp, ex. ylog_cli snapshot mtp", ylog_snapshot___case___mtp },
    { "screen", "snapshot current screen, ex. ylog_cli snapshot screen", ylog_snapshot___case___screen },
};
