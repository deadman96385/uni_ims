/**
 * Copyright (C) 2016 Spreadtrum Communications Inc.
 */

#define YLOG_ROOT_FOLDER "data/ylog/ylog"
#define YLOG_JOURNAL_FILE "ylog_journal_file"
#define YLOG_CONFIG_FILE "data/ylog/ylog.conf"

static int ylog_read_info_hook(char *buf, int count, FILE *fp, int fd, struct ylog *y) {
    UNUSED(buf);
    UNUSED(count);
    UNUSED(fp);
    char *cmd_list[] = {
        "cat /proc/interrupts",
        NULL
    };
    pcmds(cmd_list, &fd, y->write_handler, y, "ylog_info", -1, NULL);
    return 0;
}

static int ylog_read_ylog_debug_hook(char *buf, int count, FILE *fp, int fd, struct ylog *y) {
    UNUSED(buf);
    UNUSED(count);
    UNUSED(fp);
    char *cmd_list[] = {
        "./ylog_cli ylog",
        "./ylog_cli speed",
        "./ylog_cli space",
        "echo 'ubuntu test'",
        NULL
    };
    pcmds(cmd_list, &fd, y->write_handler, y, "ylog_debug", 1000, NULL);
    return 0;
}

static int ydst_fwrite_kernel(char *buf, int count, int fd, char *desc) {
    int ret = 0;
    if (count) {
        ret = fd_write(buf, count, fd, desc);
        kernel_notify(buf, count);
    }
    return ret;
}

static int cmd_test(struct command *cmd, char *buf, int buf_size, int fd, int index, struct ylog_poll *yp) {
    UNUSED(cmd);
    UNUSED(buf_size);
    UNUSED(index);
    UNUSED(yp);
    buf[ylog_get_format_time(buf)] = '\n';
    SEND(fd, buf, strlen(buf), MSG_NOSIGNAL);
    return 0;
}

static struct command os_commands[] = {
    {"test", "test from ubuntu", cmd_test, NULL},
    {"\n", NULL, os_cmd_help, (void*)os_commands},
    {NULL, NULL, NULL, NULL}
};

static int os_inotify_handler(struct ylog_inotify_cell *pcell, int timeout, struct ylog *y) {
    ylog_info("os_inotify_handler is called for '%s %s' %dms %s now\n",
                pcell->pathname ? pcell->pathname:"", pcell->filename, pcell->timeout, timeout ? "timeout":"normal");
    y = y;
    return -1;
}

static void ylog_ready(void) {
    struct context *c = global_context;
    ylog_info("ylog_ready for ubuntu\n");
    c->command_loop_ready = 1; /* mark it to work command_loop(); */
    ylog_trigger_all(global_ylog);
}

static struct ylog_keyword os_ylog_keyword[] = {
    {"loglevel", load_loglevel},
    {"keep_historical_folder_numbers", load_keep_historical_folder_numbers},
    {"ylog", load_ylog},
    {NULL, NULL},
};

static struct ylog_snapshot_list_s os_ylog_snapshot_list[] = {
    {"xxxxxxxxx", "2222222", NULL},
};

static struct context os_context[M_MODE_NUM] = {
    [M_USER] = {
        .config_file = "1.xml",
        .journal_file = YLOG_JOURNAL_FILE,
        .ylog_config_file = YLOG_CONFIG_FILE,
        .model = C_MINI_LOG,
        .loglevel = LOG_WARN,
        .keep_historical_folder_numbers = 2,
        .keep_historical_folder_numbers_default = 5,
        .ylog_keyword = os_ylog_keyword,
        .ylog_snapshot_list = os_ylog_snapshot_list,
    },
    [M_USER_DEBUG] = {
        .config_file = "2.xml",
        .journal_file = YLOG_JOURNAL_FILE,
        .ylog_config_file = YLOG_CONFIG_FILE,
        .model = C_FULL_LOG,
        .loglevel = LOG_DEBUG,
        .pre_fill_zero_to_possession_storage_spaces = 0,
        .historical_folder_root = "data/ylog/last_ylog",
        .keep_historical_folder_numbers = 2,
        .keep_historical_folder_numbers_default = 5,
        .ylog_keyword = os_ylog_keyword,
        .ylog_snapshot_list = os_ylog_snapshot_list,
    },
};

static int os_check_sdcard_online(char *sdcard_path, int count) {
    UNUSED(sdcard_path);
    UNUSED(count);
    return 0;
}

static void os_env_prepare(void) {

}

static void os_init(struct ydst_root *root, struct context **c, struct os_hooks *hook) {
    enum mode_types mode;
    /* Assume max size is 1G */
    struct ynode os_ynode[] = {
        /* kernel/ */ {
           .ylog = {
                {
                    .name = "kernel",
                    .type = FILE_NORMAL,
                    .file = "/proc/kmsg",
                    .mode = YLOG_READ_MODE_BLOCK | YLOG_READ_LEN_MIGHT_ZERO,
                    .restart_period = 0,
                    .fp_array = NULL,
                    .timestamp = 1,
                },
            },
            .ydst = {
                .file = "kernel/",
                .max_segment = 10,
                .max_segment_size = 10*1024*1024,
                .fwrite = ydst_fwrite_kernel,
            },
            .cache = {
                .size = 512 * 1024,
                .num = 2,
                .timeout = 1000,
                .debuglevel = CACHELINE_DEBUG_INFO | CACHELINE_DEBUG_CRITICAL,
            },
        },
        /* ubuntu/ */ {
            .ylog = {
                {
                    .name = "test",
                    .type = FILE_POPEN,
                    .file = "ls -l /",
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
            },
            .ydst = {
                .file = "ubuntu/",
                .max_segment = 30,
                .max_segment_size = 1024*1024,
            },
            .cache = {
                .size = 512 * 1024,
                .num = 10,
                .timeout = 1000,
                .debuglevel = CACHELINE_DEBUG_INFO | CACHELINE_DEBUG_CRITICAL,
            },
        },
        /* default/ */ {
            .ylog = {
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
            },
        },
    };

    mode = M_USER_DEBUG;
    *c = &os_context[mode];

    ynode_insert_all(os_ynode, (int)ARRAY_LEN(os_ynode));

    hook->ylog_read_ylog_debug_hook = ylog_read_ylog_debug_hook;
    hook->ylog_read_info_hook = ylog_read_info_hook;
    hook->process_command_hook = process_command_hook;
    hook->cmd_ylog_hook = cmd_ylog_hook;

    parse_config(global_context->ylog_config_file);

    root->root = strdup(YLOG_ROOT_FOLDER); /* Remember to call free */
}
