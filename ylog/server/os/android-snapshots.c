/**
 * Copyright (C) 2016 Spreadtrum Communications Inc.
 */

// enum SNAPSHOTS_TYPES {
//     SNAPSHOTS_STARTUP,
// };

struct ylog_snapshots_args {
    union {
        int ret;
        char *p;
    } u;
    char result[4096];
};

static void ylog_snapshots___case___startup(struct ylog_snapshots_args *args) {
    UNUSED(args);
    char timeBuf[32];
    struct ylog *y = pos->snapshots;
    struct ydst *ydst = y->ydst;

    ylog_get_format_time_year(timeBuf);
    pcmd_snapshots("logcat -d", y, 1000, 3*1024*1024,
            "%s/%s/startup/%s/logcat", ydst->root_folder, ydst->file, timeBuf);
    pcmd_snapshots("dmesg", y, 1000, 3*1024*1024,
            "%s/%s/startup/%s/kmsg", ydst->root_folder, ydst->file, timeBuf);
    if (args)
        snprintf(args->result, sizeof args->result, "%s/%s/startup/%s/", ydst->root_folder, ydst->file, timeBuf);
}

static struct ylog_snapshots_list_s {
    char *name;
    void (*f)(struct ylog_snapshots_args *args);
} ylog_snapshots_list[] = {
    { "startup", ylog_snapshots___case___startup },
};
