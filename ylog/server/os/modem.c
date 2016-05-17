/**
 * Copyright (C) 2016 Spreadtrum Communications Inc.
 */

struct other_processor_property_cell {
    const char *property_name;
    const char *property_value_default;
};

struct other_processor_node {
    struct other_processor_property_cell log;
    struct other_processor_property_cell diag;
    struct other_processor_property_cell atch; /* File is for sending raw AT command string */
    struct other_processor_property_cell atloop; /* File is for sending auto download at swith command AT AUTODLOADER */
};

struct other_processor_log_privates {
    char *file_diag;
    char *file_atch;
    char *file_atloop;
};

struct other_processor_log {
    const char *enabled;
    struct other_processor_node pnode;
    struct ynode ynode;
    struct other_processor_log_privates privates;
};

static int send_at_file_atloop(int idx, char *buf, int count, char *retbuf, int *retcount_max) {
    int fd, result = 0, ret = 0;
    char *file_atloop;
    if (idx >= pos->file_atloop_num) {
        return 1;
    }
    file_atloop = pos->file_atloop[idx];
    fd = open(file_atloop, O_RDWR);
    if (fd < 0) {
        ylog_info("open %s fail.%s", file_atloop, strerror(errno));
        return 1;
    }
    if (count != fd_write(buf, count, fd, "send_at_file_atloop"))
        result = 1;
    ylog_debug("send to %s with %s\n", file_atloop, buf);
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
            if (fcntl_read_nonblock(fd, "send_at_file_atloop") == 0) {
                ret = read(fd, retbuf, ret_max);
                if (ret > 0)
                    fcntl_read_block(fd, "send_at_file_atloop");
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
    CLOSE(fd);
    return result;
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
    if (count != fd_write(buf, count, fd, "send_at_file_atch"))
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
    CLOSE(fd);
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
    /* skip "at " first 3 bytes */
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

static int cmd_atloop(struct command *cmd, char *buf, int buf_size, int fd, int index, struct ylog_poll *yp) {
    UNUSED(cmd);
    UNUSED(index);
    UNUSED(yp);
    int len = strlen(buf);
    int idx = 0;
    int ret_size = buf_size;
    if (len == 7) {
        SEND(fd, buf, snprintf(buf, buf_size, "%s\n", pos->file_atch[idx]), MSG_NOSIGNAL);
        return 0;
    }
    strcat(buf, "\r");
    len++;
    /* skip "atloop " first 7 bytes */
    if (send_at_file_atloop(idx, buf + 7, len - 7, NULL, NULL) == 0) {
        len = snprintf(buf, buf_size, "OK\n");
    } else {
        len = snprintf(buf, buf_size, "Failed\n");
    }
    if (len)
        SEND(fd, buf, len, MSG_NOSIGNAL);
    return 0;
}

static int cmd_pac(struct command *cmd, char *buf, int buf_size, int fd, int index, struct ylog_poll *yp) {
    snprintf(buf, buf_size, "atloop AT+SPREF=\"AUTODLOADER\"");
    return cmd_atloop(cmd, buf, buf_size, fd, index, yp);
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

static void insert_file_atloop(char *file_atloop, int mode) {
    UNUSED(mode);
    int i;
    for (i = 0; i < pos->file_atloop_num; i++) {
        if (pos->file_atloop[i] == NULL) {
            pos->file_atloop[i] = strdup(file_atloop);
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
                .atloop = {"ro.modem.w.loop", "/dev/stty_w0"},
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
                    .num = 80,
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
                .atloop = {"ro.modem.t.loop", "/dev/stty_t0"},
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
                    .num = 80,
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
                .atloop = {"ro.modem.l.loop", "/dev/stty_lte0"},
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
                    .num = 80,
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
                .atloop = {"ro.modem.lf.loop", "/dev/stty_lte0"},
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
                    .num = 80,
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
                .atloop = {"ro.modem.tl.loop", "/dev/stty_lte0"},
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
                    .num = 80,
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
                    .num = 80,
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
                    .num = 80,
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
                    .num = 80,
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

                snprintf(mprop, sizeof mprop, "ro.modem.%s.loop", modem_type);
                property_get(mprop, prop, "");
                if (prop[0] && opl->pnode.atloop.property_name == NULL)
                    opl->pnode.atloop.property_name = strdup(prop);
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
        if (opl->pnode.atloop.property_name) {
            property_get(opl->pnode.atloop.property_name, prop, opl->pnode.atloop.property_value_default);
            if (prop[0]) {
                strcat(prop, "0");
#if 1
                ylog_info("%s has bug, property value is %s, it should be %s\n",
                        opl->pnode.atloop.property_name, prop, opl->pnode.atloop.property_value_default);
                strcpy(prop, opl->pnode.atloop.property_value_default);
#endif
                privates->file_atloop = strdup(prop);
                if (ylog->mode & YLOG_GROUP_MODEM)
                    insert_file_atloop(privates->file_atloop, YLOG_GROUP_MODEM);
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
