/**
 * Copyright (C) 2016 Spreadtrum Communications Inc.
 */

#define YLOG_ROOT_FOLDER "data/ylog/ylog"
#define YLOG_CONFIG        "data/ylog/ylog.conf"

static int ylog_read_info_hook(char *buf, int count, FILE *fp, int fd, struct ylog *y) {
    char *cmd_list[] = {
        "cat /proc/interrupts",
        NULL
    };
    pcmds(cmd_list, &fd, y->write_handler, y, "ylog_info");
    return 0;
    if (0) { /* avoid compiler warning */
        buf = buf;
        count = count;
        fp = fp;
    }
}

static int ylog_read_ylog_debug_hook(char *buf, int count, FILE *fp, int fd, struct ylog *y) {
    char *cmd_list[] = {
        "./ylog_cli ylog",
        "./ylog_cli speed",
        "./ylog_cli space",
        "echo 'ubuntu test'",
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

static int ydst_fwrite_kernel(char *buf, int count, int fd) {
    int ret = 0;
    if (count) {
        ret = fd_write(buf, count, fd);
        kernel_notify(buf, count);
    }
    return ret;
}

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

static struct command os_commands[] = {
    {"test", "test from ubuntu", cmd_test, NULL},
    {"\n", NULL, os_cmd_help, (void*)os_commands},
    {NULL, NULL, NULL, NULL}
};

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
    {"loglevel", NULL},
    {"ylog", load_ylog},
    {NULL, NULL},
};

static void ylog_update_config2(char *key, char *value) {
    char *argv[2];
    argv[0] = key;
    argv[1] = value;
    ylog_update_config(YLOG_CONFIG, 2, argv, 1);
}

static int os_inotify_handler(struct ylog_inotify_cell *pcell, int timeout, struct ylog *y) {
    ylog_info("os_inotify_handler is called for '%s %s' %dms %s now\n",
                pcell->pathname ? pcell->pathname:"", pcell->filename, pcell->timeout, timeout ? "timeout":"normal");
    y = y;
    return -1;
}

static void ylog_ready(void) {
    ylog_info("ylog_ready for ubuntu\n");
    ylog_trigger_all(global_ylog);
}

enum os_ydst_type {
    OS_YDST_TYPE_KERNEL,
    OS_YDST_TYPE_UBUNTU,
    OS_YDST_TYPE_MAX
};

static struct cacheline os_cacheline[OS_YDST_TYPE_UBUNTU+1] = {
    [OS_YDST_TYPE_KERNEL] = {
        .size = 512 * 1024,
        .num = 2,
        .timeout = 1000, /* ms */
        .debuglevel = CACHELINE_DEBUG_INFO | CACHELINE_DEBUG_CRITICAL,
    },
    [OS_YDST_TYPE_UBUNTU] = {
        .size = 512 * 1024,
        .num = 10,
        .timeout = 1000, /* ms */
        .debuglevel = CACHELINE_DEBUG_INFO | CACHELINE_DEBUG_CRITICAL,
    },
};

static struct context os_context[M_MODE_NUM] = {
    [M_USER] = {
        .config_file = "1.xml",
        .journal_file = "ylog_journal_file",
        .model = C_MINI_LOG,
        .loglevel = LOG_WARN,
        .keep_historical_folder_numbers = 2,
    },
    [M_USER_DEBUG] = {
        .config_file = "2.xml",
        .journal_file = "ylog_journal_file",
        .model = C_FULL_LOG,
        .loglevel = LOG_DEBUG,
        .pre_fill_zero_to_possession_storage_spaces = 0,
        .historical_folder_root = "data/ylog/last_ylog",
        .keep_historical_folder_numbers = 2,
    },
};

static int os_check_sdcard_online(void) {
    return 0;
}

static void os_env_prepare(void) {

}

static void os_init(struct ylog *ylog, struct ydst *ydst, struct ydst_root *root,
        struct context **c, struct os_hooks *hook) {
    enum mode_types mode;
    /* Assume max size is 1G */
    struct ydst os_ydst[OS_YDST_TYPE_MAX + 1] = { /* local variable */
        [OS_YDST_TYPE_KERNEL] = {
            .file = "kernel/",
            .max_segment = 10,
            .max_segment_size = 10*1024*1024,
            .cache = &os_cacheline[OS_YDST_TYPE_KERNEL],
            .fwrite = ydst_fwrite_kernel,
        },
        [OS_YDST_TYPE_UBUNTU] = {
            .file = "ubuntu/",
            .max_segment = 30,
            .max_segment_size = 1024*1024,
            .cache = &os_cacheline[OS_YDST_TYPE_UBUNTU],
        },
        { .file = NULL, }, /* .file = NULL to mark the end of the array */
    };
    struct ylog os_ylog[] = { /* local variable */
        {
            .name = "kernel",
            .type = FILE_NORMAL,
            .file = "/proc/kmsg",
            .ydst = &ydst[OS_YDST_TYPE_KERNEL+OS_YDST_TYPE_BASE],
            .mode = YLOG_READ_MODE_BLOCK | YLOG_READ_LEN_MIGHT_ZERO,
            .restart_period = 0,
            .fp_array = NULL,
            .timestamp = 1,
        },
        {
            .name = "test",
            .type = FILE_POPEN,
            .file = "ls -l /",
            .ydst = &ydst[OS_YDST_TYPE_UBUNTU+OS_YDST_TYPE_BASE],
            .restart_period = 200,
            .fp_array = NULL,
            .timestamp = 1,
            .id_token = "U0",
            .id_token_len = 2,
            .id_token_filename = "test",
        },
        {
            .name = "test1",
            .type = FILE_POPEN,
            .file = "ls -l /tmp/",
            .ydst = &ydst[OS_YDST_TYPE_UBUNTU+OS_YDST_TYPE_BASE],
            .restart_period = 200,
            .fp_array = NULL,
            .timestamp = 1,
            .id_token = "U1",
            .id_token_len = 2,
            .id_token_filename = "test1",
        },
        {
            .name = "test2",
            .type = FILE_POPEN,
            .file = "ls -l /dev/",
            .ydst = &ydst[OS_YDST_TYPE_UBUNTU+OS_YDST_TYPE_BASE],
            .restart_period = 200,
            .fp_array = NULL,
            .timestamp = 1,
            .id_token = "U2",
            .id_token_len = 2,
            .id_token_filename = "test2",
        },
        {
            .name = "test3",
            .type = FILE_POPEN,
            .file = "ls -l /dev/",
            .restart_period = 200,
            .fp_array = NULL,
            .timestamp = 1,
        },
        {
            .name = "inotify1",
            .type = FILE_INOTIFY,
            .file = "inotify",
            .yinotify = {
                .cells[0] = {
                    .filename = "Android.mk",
                    .mask = IN_ACCESS,
                    .type = YLOG_INOTIFY_TYPE_MASK_EQUAL,
                    .timeout = 1000,
                    .handler = os_inotify_handler,
                },
                .cells[1] = {
                    .pathname = "data/",
                    .filename = "hello.c",
                    .mask = IN_ACCESS,
                    .type = YLOG_INOTIFY_TYPE_MASK_EQUAL,
                    .timeout = 1000,
                    .handler = os_inotify_handler,
                },
            },
            .fp_array = NULL,
            .timestamp = 1,
        },
        { .name = NULL, }, /* .name = NULL to mark the end of the array */
    };

    mode = M_USER_DEBUG;
    *c = &os_context[mode];

    ydst_insert_all(os_ydst);
    ylog_insert_all(os_ylog);

    hook->ylog_read_ylog_debug_hook = ylog_read_ylog_debug_hook;
    hook->ylog_read_info_hook = ylog_read_info_hook;
    hook->process_command_hook = process_command_hook;
    hook->cmd_ylog_hook = cmd_ylog_hook;

    parse_config(YLOG_CONFIG);

    root->root = strdup(YLOG_ROOT_FOLDER); /* Remember to call free */
    if (0) { /* avoid compiler warning */
        ylog = ylog;
    }
}
