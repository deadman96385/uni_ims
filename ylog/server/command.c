/**
 * Copyright (C) 2016 Spreadtrum Communications Inc.
 */

int cmd_command_disabled = 0;
static pthread_mutex_t mutex_command = PTHREAD_MUTEX_INITIALIZER;
static struct command commands[];
static struct ylog_argument kernel_index[4];
static int kernel_index_ready = 0;
static int command_pipe[2];

static int send_command_reclaim_garbage(void) {
    if (write(command_pipe[1], "R", 1) != 1) {
        ylog_critical("send_command_reclaim_garbage write pipe failed\n");
        return 1;
    }
    return 0;
}

static int mark_it_can_be_free(int index, struct ylog_poll *yp) {
    pthread_mutex_lock(&mutex_command); /* protect yp->flags[index] */
    yp->flags[index] |= YLOG_POLL_FLAG_CAN_BE_FREE_NOW; /* a little like RCU */
    pthread_mutex_unlock(&mutex_command);
    send_command_reclaim_garbage();
    return 0;
}

int cmd_command_enable(int enable) {
    int before = cmd_command_disabled;
    cmd_command_disabled = enable;
    return before;
}

void kernel_notify(char *buf, int count) {
    int i;
    int index;
    if (kernel_index_ready) {
        for (i = 0; i < (int)ARRAY_LEN(kernel_index); i++) {
            index = kernel_index[i].index;
            if (index >= 0) {
                struct ylog_poll *yp = kernel_index[i].yp;
                if (SEND(yp_fd(index, yp), buf, count, MSG_NOSIGNAL) < 0) {
                    kernel_index[i].index = -1; /* release it, no need */
                    mark_it_can_be_free(index, yp); /* remote client should be closed, mark i will not use the fd */
                }
            }
        }
    }
}

static void command_client_try_free(int *online, int closed, int index, struct ylog_poll *yp) {
    int free = 1;
    pthread_mutex_lock(&mutex_command); /* protect yp->flags[index] */
    if (yp->flags[index] & YLOG_POLL_FLAG_FREE_LATER) {
        /**
         * if we free here right now, and a new connect comes to server,
         * then new fd maybe placed into the same index
         * but the index user does not know this change, so we need to get the confirm from user
         * a little like RCU
         * but if the user close first, send() socket will return -1,
         * and will call mark_it_can_be_free at that time
         */
        if ((yp->flags[index] & YLOG_POLL_FLAG_CAN_BE_FREE_NOW) == 0) {
            free = 0;
            if (yp->flags[index] & YLOG_POLL_FLAG_COMMAND_ONLINE) {
                if (yp->args[index]) {
                    pclose2(yp->args[index]); /* maybe command is running, ex. ylog_cli -c top */
                    yp->args[index] = NULL; /* avoid pclose2 again, because next time, the fd maybe will be used by others */
                }
            }
            if (yp->flags[index] & YLOG_POLL_FLAG_THREAD)
                yp->flags[index] |= YLOG_POLL_FLAG_THREAD_STOP;
        }
    }
    if (free) {
        *online = *online - 1;
        if (yp->flags[index] & YLOG_POLL_FLAG_COMMAND_ONLINE) {
            if (yp->args[index])
                pclose2(yp->args[index]);
        }
        yp->flags[index] = 0;
        yp->args[index] = NULL;
        yp_free(index, yp);
        if (closed) {
            ylog_debug("server command one client index %d is closed, online left %d\n", index, *online);
        } else {
            ylog_error("server command read failed: %s\n", strerror(errno));
        }
    } else {
        yp_clr(index, yp); /* don't receive revents */
        ylog_debug("client index %d free is delayed to the future\n", index);
    }
    pthread_mutex_unlock(&mutex_command);
}

static int process_command_list(char *buf, int buf_size, int fd, int index,
        struct ylog_poll *yp, struct command *lcommands) {
    int keep_open = 0;
    struct command *cmd;
    char *b = buf;
    char *bmax = buf + strlen(buf);
    for (; b < bmax; b++) {
        if (*b == '\t' ||
            *b == ' ' ||
            *b == '\0')
               break;
    }
    for (cmd = lcommands; cmd->name; cmd++) {
        if (strncmp(buf, cmd->name, b - buf) == 0) {
            keep_open |= cmd->handler(cmd, buf, buf_size, fd, index, yp);
            break;
        }
    }
    return keep_open;
}

static int process_command(char *buf, int buf_size, int fd, int index, struct ylog_poll *yp) {
    int keep_open = 0;
    char buf2[8192];
    char *last;

    ylog_debug("command is: %s", buf);

    if (buf[0] != '\n')
        strtok_r(buf, "\n", &last); /* we must remove \n if not help \n*/
    strncpy(buf2, buf, sizeof buf2);

    keep_open |= process_command_list(buf, buf_size, fd, index, yp, commands);

    if (os_hooks.process_command_hook) {
        keep_open |= os_hooks.process_command_hook(buf2, sizeof(buf2), fd, index, yp);
    }

    if (keep_open == 0) {
        /**
         * I don't know why in x86, ylog_cli ylog start and ylog_cli ylog stop can work well
         * but in arm, ylog_cli ylog stop and ylog_cli ylog start, then ylog_cli will pending there
         * so add this string to exit ylog_cli by luther 2016.01.19
         */
        SEND(fd, "____cli____exit____\n", 20, MSG_NOSIGNAL);
    }

    return keep_open;
}

static struct command os_commands[];
static int process_command_hook(char *buf, int buf_size, int fd, int index, struct ylog_poll *yp) {
    return process_command_list(buf, buf_size, fd, index, yp, os_commands);
}

static void command_loop(int fd_server) {
    int fd_client;
    struct ylog_poll ypc, *yp;
    int i, ret, fd_server_index, fd_pipe;
    char buf[8192];
    int online = 0;
    struct context *c = global_context;

    //while (create_socket_local_server(&fd_server, "ylog_cli"))
    //    sleep(1);

    yp = &ypc;
    memset(yp, 0, sizeof(*yp));

    for (i = 0; i < YLOG_POLL_INDEX_MAX; i++)
        yp_invalid(i, yp, NULL);

    for (i = 0; i < (int)ARRAY_LEN(kernel_index); i++)
        kernel_index[i].index = -1;
    kernel_index_ready = 1;

    ret = fd_server_index = fd_pipe = -1;
    do {
        if (fd_server_index < 0)
            fd_server_index = yp_insert(NULL, fd_server, YLOG_POLL_INDEX_MAX, yp, "r");

        if (ret < 0) {
            ret = pipe(command_pipe);
            if (ret)
                ylog_error("create pipe %s failed: %s\n", "command_loop", strerror(errno));
        }

        if (fd_pipe < 0)
            fd_pipe = yp_insert(NULL, command_pipe[0], YLOG_POLL_INDEX_MAX, yp, "r");

        if (ret == 0 && fd_server_index >= 0 && fd_pipe >= 0)
            break;

        ylog_critical("command_loop start failed, try again, fd_server_index=%d, fd_pipe=%d, ret=%d\n",
                fd_server_index, fd_pipe, ret);
        sleep(1);
    } while (1);

    for (;;) {
        if (poll(yp->pfd, YLOG_POLL_INDEX_MAX, -1) < 0) {
            ylog_error("poll failed: %s\n", strerror(errno));
            sleep(1);
            continue;
        }
        if (c->command_loop_ready == 0) {
            ylog_critical("command_loop is not ready now, waiting\n");
            usleep(100*1000);
            continue;
        }
        if (yp_isset(fd_pipe, yp)) {
            if (read(yp_fd(fd_pipe, yp), buf, 1) <= 0) {
                buf[0] = 0;
                ylog_error("read fd_pipe failed: %s\n", strerror(errno));
            }
            switch (buf[0]) {
            case 'R':
                for (i = 0 ; i < YLOG_POLL_INDEX_MAX; i++) {
                    if (yp->flags[i] & YLOG_POLL_FLAG_CAN_BE_FREE_NOW) {
                        command_client_try_free(&online, 1, i, yp);
                    }
                }
                break;
            default: ylog_warn("command_loop get the wrong pipe data:0x%02x ->%c\n", buf[0], buf[0]); break;
            }
        } else if (yp_isset(fd_server_index, yp)) {
            if ((fd_client = accept_client(fd_server)) < 0) {
                ylog_error("server command accept failed: %s\n", strerror(errno));
                sleep(1);
                continue;
            }
            if (yp_insert(NULL, fd_client, YLOG_POLL_INDEX_MAX, yp, "r") < 0) {
                ylog_critical("server command online clients have reached maximum %d\n", YLOG_POLL_INDEX_MAX - 1);
                CLOSE(fd_client);
                continue;
            }
            online++;
            ylog_debug("server command online %d\n", online);
        } else {
            for (i = 0 ; i < YLOG_POLL_INDEX_MAX; i++) {
                if (i != fd_server_index && yp_isset(i, yp)) {
                    FILE *fp = yp_fp(i, yp);
                    int fd = yp_fd(i, yp);
                    ret = ylog_read_default_line(buf, sizeof(buf), fp, fd, NULL);
                    if (ret > 0)
                        ret = process_command(buf, sizeof(buf), fd, i, yp);
                    if (ret <= 0)
                        command_client_try_free(&online, ret == 0, i, yp);
                }
            }
        }
    }
}

static int insert_ylog_argument(int index, struct ylog_poll *yp, struct ylog_argument *ya, int max) {
    int i;
    int inserted = 0;
    for (i = 0; i < max; i++) {
        if (ya[i].index < 0) {
            ya[i].index = index;
            ya[i].yp = yp;
            inserted = 1;
            break;
        }
    }
    return inserted;
}

static void *command_thread_handler(void *arg) {
    struct ylog_argument *ya = (struct ylog_argument *)arg;
    char buf[4096];
    char *p = buf;
    int ret;
    int index = ya->index;
    struct ylog_poll *yp = ya->yp;
    int fd = yp_fd(index, yp);
    int buf_size = sizeof buf;
    FILE *wfp = yp->args[index];

    ya->flags = 0; /* *arg can be free now */

    if (wfp) {
        int wfd = fileno(wfp);
        do {
            ret = read(wfd, p, buf_size);
            if (ret > 0) {
                if (SEND(fd, p, ret, MSG_NOSIGNAL) < 0) /* remote client has quited */
                    break;
            }
        } while (ret > 0);
        /* pclose2(wfp); */ /* we will call this in command_client_try_free */
    } else {
        SEND(fd, buf + buf_size/2, snprintf(buf + buf_size/2, buf_size/2,
                        "%s failed: %s\n", buf, strerror(errno)), MSG_NOSIGNAL);
    }

    mark_it_can_be_free(index, yp);

    return NULL;
}

static int cmd_command(struct command *cmd, char *buf, int buf_size, int fd, int index, struct ylog_poll *yp) {
    struct ylog_argument ya;
    pthread_t ptid;

    if (cmd_command_disabled == 0) {
        if (strlen(buf) <= strlen(cmd->name)) {
            strcat(buf, " length is wrong\n");
            SEND(fd, buf, strlen(buf), MSG_NOSIGNAL);
            return 0;
        }
        ya.args = &buf[3];
        ya.index = index;
        ya.yp = yp;
        ya.flags = 1;
        yp->flags[index] |= YLOG_POLL_FLAG_FREE_LATER | YLOG_POLL_FLAG_COMMAND_ONLINE;
        yp->args[index] = popen2(ya.args, "r");
        if (pthread_create(&ptid, NULL, command_thread_handler, &ya) == 0) {
            int wait_count = 0;
            while (ya.flags) {
                usleep(10*1000); /* wait until thread start */
                if (wait_count++ > 100 * 30) /* wait 30s */
                    break;
            }
        }
        if (ya.flags) {
            pclose2(yp->args[index]);
            yp->flags[index] &= ~(YLOG_POLL_FLAG_FREE_LATER | YLOG_POLL_FLAG_COMMAND_ONLINE);
            ylog_critical("cmd_command failed to pthread_create for %s\n", ya.args);
            return 0;
        }
        return 1; /* 1 keep this client opened */
    } else {
        SEND(fd, buf, snprintf(buf, buf_size, "this version does not support this command\n"), MSG_NOSIGNAL);
        return 0;
    }
}

static int cmd_kernel(struct command *cmd, char *buf, int buf_size, int fd, int index, struct ylog_poll *yp) {
    UNUSED(cmd);
    yp->flags[index] |= YLOG_POLL_FLAG_FREE_LATER;
    if (insert_ylog_argument(index, yp, kernel_index, ARRAY_LEN(kernel_index)) == 0) {
        yp->flags[index] &= ~YLOG_POLL_FLAG_FREE_LATER;
        SEND(fd, buf, snprintf(buf, buf_size, "kernel reading online client number has reached max %d, reject you!\n", \
                    (int)ARRAY_LEN(kernel_index)), MSG_NOSIGNAL);
        ylog_debug("%s", buf);
        return 0; /* 0 close this client after return 0 */
    }
    return 1; /* 1 keep this client opened */
}

static int cmd_help(struct command *cmd, char *buf, int buf_size, int fd, int index, struct ylog_poll *yp) {
    UNUSED(index);
    UNUSED(yp);
    struct command *cmds = (struct command *)cmd->args;
    SEND(fd, buf, snprintf(buf, buf_size, "=== [ ylog server supported commands ] ===\n"), MSG_NOSIGNAL);
    for (cmd = cmds; cmd->name; cmd++) {
        if (cmd->name[0] == '\n')
            continue;
        SEND(fd, buf, snprintf(buf, buf_size, "%-10s -- %s\n", cmd->name, cmd->help ? cmd->help : ""), MSG_NOSIGNAL);
    }
    return 0;
}

static int os_cmd_help(struct command *cmd, char *buf, int buf_size, int fd, int index, struct ylog_poll *yp) {
    UNUSED(index);
    UNUSED(yp);
    struct command *cmds = (struct command *)cmd->args;
    for (cmd = cmds; cmd->name; cmd++) {
        if (cmd->name[0] == '\n')
            continue;
        SEND(fd, buf, snprintf(buf, buf_size, "%-10s -- %s\n", cmd->name, cmd->help ? cmd->help : ""), MSG_NOSIGNAL);
    }
    return 0;
}

static int cmd_loglevel(struct command *cmd, char *buf, int buf_size, int fd, int index, struct ylog_poll *yp) {
    UNUSED(cmd);
    UNUSED(index);
    UNUSED(yp);
    char *last;
    char *value = NULL;
    char *level;
    strtok_r(buf, " ", &last);
    value = strtok_r(NULL, " ", &last);
    if (value) {
        int loglevel = strtol(value, NULL, 0);
        if (loglevel >= LOG_LEVEL_MAX)
            loglevel = LOG_LEVEL_MAX - 1;
        global_context->loglevel = loglevel;
        snprintf(buf, buf_size, "%d", loglevel);
        ylog_update_config2("loglevel", buf);
    }
    switch (global_context->loglevel) {
    case LOG_ERROR: level = "0:error"; break;
    case LOG_CRITICAL: level = "1:critical"; break;
    case LOG_WARN: level = "2:warn"; break;
    case LOG_INFO: level = "3:info"; break;
    case LOG_DEBUG: level = "4:debug"; break;
    default: level = "wrong"; break;
    }
    SEND(fd, buf, snprintf(buf, buf_size, "%s\n", level), MSG_NOSIGNAL);
    return 0;
}

static int cmd_speed(struct command *cmd, char *buf, int buf_size, int fd, int index, struct ylog_poll *yp) {
    UNUSED(cmd);
    UNUSED(index);
    UNUSED(yp);
    struct ydst_root *root = global_ydst_root;
    unsigned long long asizes = ydst_all_transfered_size();
    int i, j;
    float fsize1, fsize2, avg_speed = 0;
    char unit1, unit2, avg_speed_unit = 'B';
    struct timespec ts;
    time_t delta_speed_millisecond, delta_t;
    struct tm delta_tm;
    int y_sort_num, ydst_sort_num;
    struct ylog *y, *y_sort[YLOG_MAX]; /* */
    struct ydst *yd, *ydst_sort[YDST_MAX];
    char *suffix;
    get_boottime(&ts);
    delta_speed_millisecond = diff_ts_millisecond(&global_context->ts, &ts);
    delta_t = delta_speed_millisecond / 1000;
    if (asizes == 0)
        asizes = 1;
    // SEND(fd, buf, snprintf(buf, buf_size, "\n"), MSG_NOSIGNAL);
    y_sort_num = 0;
    for_each_ylog(i, y, NULL) {
        if (y->name == NULL)
            continue;
        y_sort[y_sort_num++] = y;
    }
    for (i = 0; i < y_sort_num; i++) {
        for (j = i; j < y_sort_num; j++) {
            if (y_sort[i]->size < y_sort[j]->size) {
                y = y_sort[i];
                y_sort[i] = y_sort[j];
                y_sort[j] = y;
            }
        }
    }
    ydst_sort_num = 0;
    for_each_ydst(i, yd, NULL) {
        if (yd->file == NULL || yd->refs <= 0)
            continue;
        ydst_sort[ydst_sort_num++] = yd;
    }
    for (i = 0; i < ydst_sort_num; i++) {
        for (j = i; j < ydst_sort_num; j++) {
            if (ydst_sort[i]->size < ydst_sort[j]->size) {
                yd = ydst_sort[i];
                ydst_sort[i] = ydst_sort[j];
                ydst_sort[j] = yd;
            }
        }
    }

    suffix = "\n";
    for (i = 0; i < y_sort_num; i++) {
        float percent = 0;
        y = y_sort[i];
        fsize1 = ylog_get_unit_size_float(y->size, &unit1);
        if (y->size)
            percent = (float)(100 * y->size) / asizes;
        SEND(fd, buf, snprintf(buf, buf_size, "[ylog] %-20s -> %5.2f%% %.2f%c\n",
                    y->name, percent, fsize1, unit1), MSG_NOSIGNAL);
        if (i == (y_sort_num - 1))
            suffix = NULL;
        send_speed(fd, buf, buf_size, y_sort[i]->speed, YLOG_SPEED_NUM, NULL, suffix?suffix:NULL);
    }
    SEND(fd, buf, snprintf(buf, buf_size,
                "----------------------------------------------------------------------------\n"), MSG_NOSIGNAL);

    suffix = "\n";
    for (i = 0; i < ydst_sort_num; i++) {
        float percent = 0;
        yd = ydst_sort[i];
        if (yd->size)
            percent = (float)(100 * yd->size) / asizes;
        fsize1 = ylog_get_unit_size_float(yd->size, &unit1);
        SEND(fd, buf, snprintf(buf, buf_size, "[ydst] %-20s -> %5.2f%% %.2f%c\n",
                    yd->file, percent, fsize1, unit1), MSG_NOSIGNAL);
        if (i == (ydst_sort_num - 1))
            suffix = NULL;
        send_speed(fd, buf, buf_size, ydst_sort[i]->speed, YDST_SPEED_NUM, NULL, suffix?suffix:NULL);
    }
    SEND(fd, buf, snprintf(buf, buf_size,
                "----------------------------------------------------------------------------\n"), MSG_NOSIGNAL);

    for (i = 0; i < y_sort_num; i++) {
        float percent = 0;
        y = y_sort[i];
        fsize1 = ylog_get_unit_size_float(y->size, &unit1);
        if (y->size)
            percent = (float)(100 * y->size) / asizes;
        SEND(fd, buf, snprintf(buf, buf_size, "[ylog] %-20s -> %5.2f%% %.2f%c\n",
                    y->name, percent, fsize1, unit1), MSG_NOSIGNAL);
    }
    SEND(fd, buf, snprintf(buf, buf_size, "\n"), MSG_NOSIGNAL);

    for (i = 0; i < ydst_sort_num; i++) {
        float percent = 0;
        yd = ydst_sort[i];
        if (yd->size)
            percent = (float)(100 * yd->size) / asizes;
        fsize1 = ylog_get_unit_size_float(yd->size, &unit1);
        fsize2 = ylog_get_unit_size_float(yd->max_size_now, &unit2);
        SEND(fd, buf, snprintf(buf, buf_size, "[ydst] %-20s -> %5.2f%% %.2f%c/%.2f%c\n",
                    yd->file, percent, fsize1, unit1, fsize2, unit2), MSG_NOSIGNAL);
    }

    fsize1 = ylog_get_unit_size_float(asizes, &unit1);
    if (delta_speed_millisecond)
        avg_speed = ylog_get_unit_size_float_with_speed(asizes, &avg_speed_unit, delta_speed_millisecond);
    gmtime_r(&delta_t, &delta_tm); /* UTC, don't support keep running more than 1 year by luther */
    SEND(fd, buf, snprintf(buf, buf_size, "\nTransfered %.2f%c Has run %02d day %02d:%02d:%02d avg_speed=%.2f%c/s\n",
                fsize1, unit1,
                delta_tm.tm_yday, delta_tm.tm_hour,
                delta_tm.tm_min, delta_tm.tm_sec,
                avg_speed, avg_speed_unit), MSG_NOSIGNAL);
    send_speed(fd, buf, buf_size, root->speed, YDST_ROOT_SPEED_NUM, NULL, NULL);
    return 0;
}

static int cmd_time(struct command *cmd, char *buf, int buf_size, int fd, int index, struct ylog_poll *yp) {
    UNUSED(cmd);
    UNUSED(index);
    UNUSED(yp);
    struct timespec ts;
    time_t delta_speed_millisecond, delta_t;
    struct tm delta_tm;
    char timeBuf[32];
    get_boottime(&ts);
    delta_speed_millisecond = diff_ts_millisecond(&global_context->ts, &ts);
    delta_t = delta_speed_millisecond / 1000;
    gmtime_r(&delta_t, &delta_tm); /* UTC, don't support keep running more than 1 year by luther */
    ylog_get_format_time(timeBuf);
    SEND(fd, buf, snprintf(buf, buf_size,
                "\n"
                "%02d day %02d:%02d:%02d       -- runned\n"
                "%s -- start time\n"
                "%s -- current time\n"
                "\n",
                delta_tm.tm_yday, delta_tm.tm_hour,
                delta_tm.tm_min, delta_tm.tm_sec,
                global_context->timeBuf, timeBuf), MSG_NOSIGNAL);
    return 0;
}

static int cmd_flush(struct command *cmd, char *buf, int buf_size, int fd, int index, struct ylog_poll *yp) {
    UNUSED(cmd);
    UNUSED(buf);
    UNUSED(buf_size);
    UNUSED(fd);
    UNUSED(index);
    UNUSED(yp);
    struct ylog *y = NULL;
    int i;
    for_each_ylog(i, y, NULL) {
        if (y->name == NULL)
            continue;
        y->thread_flush(y, 1);
    }
    return 0;
}

static int cmd_ylog(struct command *cmd, char *buf, int buf_size, int fd, int index, struct ylog_poll *yp) {
    UNUSED(cmd);
    UNUSED(index);
    UNUSED(yp);
    struct ylog *y = NULL;
    int i;
    int nargs;
    char *args[20];
    float fsize1, fsize2;
    char unit1, unit2;
    struct ydst_root *root = global_ydst_root;
    int show_info = 0;
    char *last;
    char ylog_name[128];
    char *name = NULL;
    char *key;
    char *value = NULL;
    char *svalue1 = NULL;
    char *svalue2 = NULL;
    char *space_string = "    ";//"\t";
    char path[PATH_MAX];
    struct timespec ts;
    struct tm delta_tm;
    time_t delta_t;
    int all = 0;
    int action_start_stop = 0;
    int action_timestamp = 0;
    int action_bypass = 0;
    int action_cacheline_bypass = 0;
    int action_cacheline_timeout = 0;
    int action_cacheline_debuglevel = 0;
    int action_ydst_max_segment = 0;
    int action_ydst_max_segment_size = 0;
    int action_ydst_segment_size = 0;
    int action_ydst_quota = 0;

    strtok_r(buf, " ", &last);
    name = strtok_r(NULL, " ", &last);
    key = strtok_r(NULL, " ", &last);
    value = strtok_r(NULL, " ", &last);
    svalue1 = strtok_r(NULL, " ", &last);
    svalue2 = strtok_r(NULL, " ", &last);

    if (name) {
        strcpy(ylog_name, name);
        name = ylog_name;
        /* 1. get ylog name */
        if (strcmp(name, "all")) {
            all = 0;
            y = ylog_get_by_name(name);
            if (y == NULL) {
                SEND(fd, buf, snprintf(buf, buf_size, "%s does not exist\n", name), MSG_NOSIGNAL);
                return 0;
            }
        } else {
            all = 1;
            name = NULL;
            y = NULL;
        }
        /* 2. check the key and value */
        if (key) {
            if (strcmp(key, "start") == 0) {
                action_start_stop = 1;
                value = "1";
            } else if (strcmp(key, "stop") == 0) {
                action_start_stop = 1;
                value = "0";
            } else if (strcmp(key, "timestamp") == 0) {
                if (value)
                    action_timestamp = 1;
            } else if (strcmp(key, "bypass") == 0) {
                if (value)
                    action_bypass = 1;
            } else if (strcmp(key, "cache") == 0) {
                if (value == NULL || svalue1 == NULL || y == NULL || y->ydst->cache == NULL) {
                    SEND(fd, buf, snprintf(buf, buf_size, "wrong format\n"), MSG_NOSIGNAL);
                    return 0;
                }
                if (strcmp(value, "bypass") == 0)
                    action_cacheline_bypass = 1;
                if (strcmp(value, "timeout") == 0)
                    action_cacheline_timeout = 1;
                if (strcmp(value, "debuglevel") == 0)
                    action_cacheline_debuglevel = 1;
            } else if (strcmp(key, "ydst") == 0) {
                if (value == NULL || svalue1 == NULL) {
                    SEND(fd, buf, snprintf(buf, buf_size, "wrong format\n"), MSG_NOSIGNAL);
                    return 0;
                }
                if (strcmp(value, "max_segment") == 0)
                    action_ydst_max_segment = 1;
                if (strcmp(value, "max_segment_size") == 0)
                    action_ydst_max_segment_size = 1;
                if (strcmp(value, "segment_size") == 0)
                    action_ydst_segment_size = 1;
                if (strcmp(value, "quota") == 0)
                    action_ydst_quota = 1;
            } else if (strcmp(key, "get") == 0) {
                if (y) {
                    if (value) {
                        if (strcmp(value, "started") == 0)
                            SEND(fd, buf, snprintf(buf, buf_size, "%d\n", !(y->status & YLOG_DISABLED_MASK)), MSG_NOSIGNAL);
                    }
                }
                return 0;
            }
        }
        show_info = 1;
    }

    if (action_ydst_quota) {
        ydst_requota_percent(atoi(svalue1), y, NULL);
    }
    if (action_ydst_segment_size) {
        int max_segment = strtol(svalue1, NULL, 0);
        unsigned long long max_segment_size = strtoll(svalue2, NULL, 0) * 1024 * 1024;
        ydst_quota(max_segment_size, max_segment, y, NULL);
    }
    if (action_ydst_max_segment_size) {
        unsigned long long max_segment_size = strtoll(svalue1, NULL, 0) * 1024 * 1024;
        ydst_quota(max_segment_size, 0, y, NULL);
    }
    if (action_ydst_max_segment) {
        int max_segment = strtol(svalue1, NULL, 0);
        ydst_quota(0, max_segment, y, NULL);
    }
    if (action_cacheline_debuglevel) {
        int debuglevel = strtol(svalue1, NULL, 0);
        y->ydst->cache->debuglevel = debuglevel;
    }
    if (action_cacheline_timeout) {
        int timeout = strtol(svalue1, NULL, 0);
        if (timeout)
            y->ydst->cache->timeout = timeout;
    }
    if (action_cacheline_bypass) {
        int en = strcmp(svalue1, "0");
        // ylog_cli ylog kernel cache bypass 1
        y->ydst->cache->bypass = !!en;
    }
    if (action_bypass) {
        int en = strcmp(value, "0");
        y->bypass = !!en;
    }
    if (action_timestamp) {
        int en = strcmp(value, "0");
        y->timestamp = !!en;
    }
    if (action_start_stop) {
        int en = strcmp(value, "0");
        ylog_thread_run action = NULL;
        if (all == 0) {
            pthread_mutex_lock(&mutex);
            if (en) {
                y->status &= ~YLOG_DISABLED_MASK;
                action = y->thread_run;
            } else {
                y->status &= ~YLOG_DISABLED_FORCED_RUNNING;
                y->status |= YLOG_DISABLED_FORCED;
                action = y->thread_stop;
            }
            args[0] = "ylog";
            args[1] = "enabled";
            args[2] = y->name;
            args[3] = en ? "1" : "0";
            nargs = 4;
            if (os_hooks.cmd_ylog_hook)
                os_hooks.cmd_ylog_hook(nargs, args);
            pthread_mutex_unlock(&mutex);
            if (action)
                action(y, 1);
            print2journal_file("ylog %s %s", y->name, en ? "start":"stop");
            SEND(fd, buf, snprintf(buf, buf_size, "[ %s ] = %s\n",
                        y->name, (y->status & YLOG_DISABLED_MASK) ? "stopped" : "running"), MSG_NOSIGNAL);
        } else {
            for_each_ylog(i, y, NULL) {
                if (y->name == NULL)
                    continue;
                action = NULL;
                pthread_mutex_lock(&mutex);
                if (en) {
                    if (y->status & YLOG_DISABLED_FORCED_RUNNING) {
                        y->status &= ~YLOG_DISABLED_MASK;
                        action = y->thread_run;
                    }
                } else {
                    y->status |= YLOG_DISABLED_FORCED;
                    action = y->thread_stop;
                }
                args[0] = "ylog";
                args[1] = "enabled";
                args[2] = y->name;
                args[3] = en ? "1" : "0";
                nargs = 4;
                if (os_hooks.cmd_ylog_hook)
                    os_hooks.cmd_ylog_hook(nargs, args);
                pthread_mutex_unlock(&mutex);
                ylog_info("ylog %s %s enter...\n", en ? "start":"stop", y->name);
                if (action)
                    action(y, 1);
                ylog_info("ylog %s %s done.\n", en ? "start":"stop", y->name);
            }
            if (en)
                ylog_event_thread_notify_all_run_type();
            else
                ylog_event_thread_notify_all_stop_type();
            print2journal_file("ylog all %s", en ? "start":"stop");
            SEND(fd, buf, snprintf(buf, buf_size, "[ %s ] = %s\n",
                        "all", (!en) ? "stopped" : "running"), MSG_NOSIGNAL);
        }
        return 0;
    }

    get_boottime(&ts);
    delta_t = ts.tv_sec - global_context->ts.tv_sec;
    gmtime_r(&delta_t, &delta_tm); /* UTC, don't support keep running more than 1 year by luther */
    fsize1 = ylog_get_unit_size_float(root->quota_now ? root->quota_now : root->max_size, &unit1);
    SEND(fd, buf, snprintf(buf, buf_size, "--------------------------------------------------------------------------\n"\
                "root = %s quota = %.2f%c, running %02d day %02d:%02d:%02d\n"\
                "--------------------------------------------------------------------------\n"
                , global_ydst_root->root, fsize1, unit1,
                delta_tm.tm_yday,
                delta_tm.tm_hour,
                delta_tm.tm_min,
                delta_tm.tm_sec), MSG_NOSIGNAL);

    if (show_info) {
        #if 0
        Output format like below
        root =
        [ socket ] = {
            .file =
            .restart_period =
            .timestamp =
            .ydst = {
                .file =
                .max_segment =
                .max_segment_size =
                .cache = {
                    .size =
                    .num =
                    .timeout =
                }
            }
        }
        #endif
        for_each_ylog(i, y, NULL) {
            if (y->name == NULL)
                continue;
            if (name && strcmp(name, y->name))
                continue;
            SEND(fd, buf, snprintf(buf, buf_size, "[ %s ] = %s {\n", y->name, (y->status & YLOG_DISABLED_MASK) ? "stopped" : "running"), MSG_NOSIGNAL);
            SEND(fd, buf, snprintf(buf, buf_size, "%s.loglevel = %d\n", space_string, global_context->loglevel), MSG_NOSIGNAL);
            SEND(fd, buf, snprintf(buf, buf_size, "%s.file = %s\n", space_string, y->file), MSG_NOSIGNAL);
            SEND(fd, buf, snprintf(buf, buf_size, "%s.restart_period = %d\n", space_string, y->restart_period), MSG_NOSIGNAL);
            SEND(fd, buf, snprintf(buf, buf_size, "%s.timestamp = %d\n", space_string, y->timestamp), MSG_NOSIGNAL);
            SEND(fd, buf, snprintf(buf, buf_size, "%s.bypass = %d\n", space_string, y->bypass), MSG_NOSIGNAL);
            SEND(fd, buf, snprintf(buf, buf_size, "%s.mode = %d\n", space_string, y->mode), MSG_NOSIGNAL);
            fsize1 = ylog_get_unit_size_float(y->ydst->size, &unit1);
            fsize2 = ylog_get_unit_size_float(y->size, &unit2);
            SEND(fd, buf, snprintf(buf, buf_size, "%s.ydst = %.2f%c/%.2f%c {\n", space_string, fsize2, unit2, fsize1, unit1), MSG_NOSIGNAL);
            yds_new_segment_file_name(path, sizeof path, 0, y->ydst);
            SEND(fd, buf, snprintf(buf, buf_size, "%s%s.file = %s\n", space_string, space_string, path), MSG_NOSIGNAL);
            SEND(fd, buf, snprintf(buf, buf_size, "%s%s.max_segment  = %d\n", space_string, space_string, y->ydst->max_segment_now), MSG_NOSIGNAL);
            fsize1 = ylog_get_unit_size_float(y->ydst->max_segment_size_now, &unit1);
            SEND(fd, buf, snprintf(buf, buf_size, "%s%s.max_segment_size  = %.2f%c\n", space_string, space_string, fsize1, unit1), MSG_NOSIGNAL);
            if (y->ydst->cache) {
                SEND(fd, buf, snprintf(buf, buf_size, "%s%s.cache= {\n", space_string, space_string), MSG_NOSIGNAL);
                fsize1 = ylog_get_unit_size_float(y->ydst->cache->size, &unit1);
                SEND(fd, buf, snprintf(buf, buf_size, "%s%s%s.size = %.2f%c\n",
                            space_string, space_string, space_string, fsize1, unit1), MSG_NOSIGNAL);
                SEND(fd, buf, snprintf(buf, buf_size, "%s%s%s.num = %d\n",
                            space_string, space_string, space_string, y->ydst->cache->num), MSG_NOSIGNAL);
                SEND(fd, buf, snprintf(buf, buf_size, "%s%s%s.bypass = %d\n",
                            space_string, space_string, space_string, y->ydst->cache->bypass), MSG_NOSIGNAL);
                SEND(fd, buf, snprintf(buf, buf_size, "%s%s%s.timeout = %dms\n",
                            space_string, space_string, space_string, y->ydst->cache->timeout), MSG_NOSIGNAL);
                SEND(fd, buf, snprintf(buf, buf_size, "%s%s%s.debuglevel = 0x%02x\n",
                            space_string, space_string, space_string, y->ydst->cache->debuglevel), MSG_NOSIGNAL);
                SEND(fd, buf, snprintf(buf, buf_size, "%s%s%s.wclidx_max = %d\n",
                            space_string, space_string, space_string, y->ydst->cache->wclidx_max), MSG_NOSIGNAL);
                SEND(fd, buf, snprintf(buf, buf_size, "%s%s}\n", space_string, space_string), MSG_NOSIGNAL);
            }
            SEND(fd, buf, snprintf(buf, buf_size, "%s}\n", space_string), MSG_NOSIGNAL);
            SEND(fd, buf, snprintf(buf, buf_size, "}\n"), MSG_NOSIGNAL);
        }
    } else {
        char *status;
        unsigned long long quota_now = root->quota_now ? root->quota_now : root->max_size;
        for_each_ylog(i, y, NULL) {
            if (y->name == NULL)
                continue;
            // ylog -> running -> ydst -> cacheline
            if (y->status & YLOG_DISABLED)
                status = "stop";
            if (y->status & YLOG_DISABLED_FORCED)
                status = "stop-f";
            if (y->status & YLOG_DISABLED_FORCED_RUNNING)
                status = "stop-f-r";
            if ((y->status & YLOG_DISABLED_MASK) == 0)
                status ="running";
            fsize1 = ylog_get_unit_size_float(y->ydst->max_segment_size_now, &unit1);
            fsize2 = ylog_get_unit_size_float(y->ydst->max_size_now, &unit2);
            yds_new_segment_file_name(path, sizeof path, 0, y->ydst);
            SEND(fd, buf, snprintf(buf, buf_size, "%-20s -> %-8s -> %s (%dx%.2f%c/%.2f%c,%5.2f%%)",
                        y->name, status, path + strlen(y->ydst->root_folder) + 1, y->ydst->max_segment_now,
                        fsize1, unit1, fsize2, unit2, (100 * y->ydst->max_size_now) / (float)quota_now), MSG_NOSIGNAL);
            if (y->ydst->cache) {
                fsize1 = ylog_get_unit_size_float(y->ydst->cache->size, &unit1);
                SEND(fd, buf, snprintf(buf, buf_size, " -> cache.%s(%dx%.2f%c/%d,%d)",
                            y->ydst->cache->name, y->ydst->cache->num,
                            fsize1, unit1, y->ydst->cache->wclidx, y->ydst->cache->wclidx_max), MSG_NOSIGNAL);
            }
            fsize1 = ylog_get_unit_size_float(y->size, &unit1);
            fsize2 = ylog_get_unit_size_float(y->ydst->size, &unit2);
            SEND(fd, buf, snprintf(buf, buf_size, " [%.2f%c/%.2f%c]",
                            fsize1, unit1, fsize2, unit2), MSG_NOSIGNAL);
            SEND(fd, "\n", 1, MSG_NOSIGNAL);
        }
    }
    return 0;
}

static int cmd_cpath(struct command *cmd, char *buf, int buf_size, int fd, int index, struct ylog_poll *yp) {
    UNUSED(cmd);
    UNUSED(index);
    UNUSED(yp);
    char *root_new, *last;
    char path[PATH_MAX];
    struct ydst_root *root = global_ydst_root;
    strtok_r(buf, " ", &last);
    root_new = strtok_r(NULL, " ", &last);
    if (root_new) {
        snprintf(path, sizeof path, "%s/ylog", root_new); /* we must append /ylog to avoid wrong rm -rf */
        ydst_root_new(NULL, path);
    }
    SEND(fd, buf, snprintf(buf, buf_size, "%s\n", root->root), MSG_NOSIGNAL);
    return 0;
}

static int cmd_quota(struct command *cmd, char *buf, int buf_size, int fd, int index, struct ylog_poll *yp) {
    UNUSED(cmd);
    UNUSED(index);
    UNUSED(yp);
    float fsize1;
    char unit1;
    char *quotas, *last;
    struct ydst_root *root = global_ydst_root;
    strtok_r(buf, " ", &last);
    quotas = strtok_r(NULL, " ", &last);
    if (quotas) {
        unsigned long long quota;
        quota = strtol(quotas, NULL, 0);
        if (quota == 0) {
            SEND(fd, buf, snprintf(buf, buf_size, "%s is not a number\n", quotas), MSG_NOSIGNAL);
            return 0;
        }
        quota *= 1024 * 1024;
        ydst_root_quota(NULL, quota);
    }
    fsize1 = ylog_get_unit_size_float(root->quota_now ? root->quota_now : root->max_size, &unit1);
    SEND(fd, buf, snprintf(buf, buf_size, "%.2f%c\n", fsize1, unit1), MSG_NOSIGNAL);
    return 0;
}

static void rm_root_others_last(void) {
    char *root_path_others[] = {
        "SYSDUMP",
    };
    char *root_path_others_abs_path[] = {
        "/data/ylog/last_ylog",
    };
    char basename_root[PATH_MAX];
    char *dirname;
    struct ydst_root *root = global_ydst_root;
    int i;
    strcpy(basename_root, root->root);
    dirname = dirname2(basename_root);
    for (i = 0; i < (int)ARRAY_LEN(root_path_others); i++)
        do_cmd(NULL, 0, 1, "rm -rf %s/%s", dirname, root_path_others[i]);
    for (i = 0; i < (int)ARRAY_LEN(root_path_others_abs_path); i++)
        do_cmd(NULL, 0, 1, "rm -rf %s", root_path_others_abs_path[i]);
}

static void rm_root_others_live(void) {
    int i;
    char *root_path_others_abs_path[] = {
        "/data/ylog/ylog",
    };
    for (i = 0; i < (int)ARRAY_LEN(root_path_others_abs_path); i++)
        do_cmd(NULL, 0, 1, "rm -rf %s", root_path_others_abs_path[i]);
}

static int cmd_clear_ylog(struct command *cmd, char *buf, int buf_size, int fd, int index, struct ylog_poll *yp) {
    UNUSED(index);
    UNUSED(yp);
    long mode = (long)cmd->args;
    struct context *c = global_context;
    struct ydst_root *root = global_ydst_root;
    struct ylog *y = NULL;
    int i;

    if (mode & (YLOG_CLEAR_LAST_YLOG | YLOG_CLEAR_ALL_QUIT)) {
        if (c->historical_folder_root) {
            rm_all(c->historical_folder_root);
            print2journal_file("clear last_ylog %s", c->historical_folder_root);
        }
        rm_root_others_last();
    }

    if (mode & YLOG_CLEAR_ALL_QUIT) {
        ylog_all_thread_exit();
        usleep(300 * 1000); /* wait 300ms for ylog thread exit itself */
        ylog_root_folder_delete(root->root, c->historical_folder_root, 0, 0);
        print2journal_file("clear all ylog and reboot %s", root->root);
        rm_root_others_live();
        kill(getpid(), SIGKILL);
    }

    if (mode & (YLOG_CLEAR_CURRENT_RUNNING | YLOG_CLEAR_ALL_QUIT)) {
        for_each_ylog(i, y, NULL) {
            if (y->name == NULL)
                continue;
            y->thread_reset(y, 1);
        }
        #ifdef HAVE_YLOG_JOURNAL
        reset_journal_file();
        #endif
        print2journal_file("clear all ylog %s", root->root);
    }

    SEND(fd, buf, snprintf(buf, buf_size, "done\n"), MSG_NOSIGNAL);

    return 0;
}

static int cmd_space(struct command *cmd, char *buf, int buf_size, int fd, int index, struct ylog_poll *yp) {
    UNUSED(cmd);
    UNUSED(index);
    UNUSED(yp);
    struct ydst_root *root = global_ydst_root;
    struct context *c = global_context;
    char *root_path = root->root;
    char *historical_folder_root = c->historical_folder_root;
    int ret;
    char unit;
    float fsize;
    unsigned long long freespace;
    fsize = ylog_get_unit_size_float(root->quota_now ? root->quota_now : root->max_size, &unit);
    SEND(fd, buf, snprintf(buf, buf_size, "%.2f%c\t(quota)\n", fsize, unit), MSG_NOSIGNAL);
#if 0
    ret = do_cmd(buf, buf_size, 1, "du -sh %s", root_path);
    if (historical_folder_root)
        ret += do_cmd(buf + ret, buf_size - ret, 1, "du -sh %s", historical_folder_root);
#else
    freespace = calculate_path_disk_available(root_path);
    fsize = ylog_get_unit_size_float(freespace, &unit);
    ret = snprintf(buf, buf_size, "%s -> %.2f%c (freespace)\n", root_path, fsize, unit);
    SEND(fd, buf, ret, MSG_NOSIGNAL);

    if (historical_folder_root) {
        ret = do_cmd(buf, buf_size, 1, "du -sh %s", historical_folder_root);
        if (ret <= 0)
            ret = snprintf(buf, buf_size, "Failed to call du -sh %s", historical_folder_root);
        SEND(fd, buf, ret, MSG_NOSIGNAL);
    }

    ret = do_cmd(buf, buf_size, 1, "du -sh %s", root_path);
    if (ret <= 0)
        ret = snprintf(buf, buf_size, "Failed to call du -sh %s", root_path);
    SEND(fd, buf, ret, MSG_NOSIGNAL);
#endif
    return 0;
}

static int cmd_freespace(struct command *cmd, char *buf, int buf_size, int fd, int index, struct ylog_poll *yp) {
    UNUSED(cmd);
    UNUSED(index);
    UNUSED(yp);
    struct ydst_root *root = global_ydst_root;
    char *root_path = root->root;
    unsigned long long freespace = calculate_path_disk_available(root_path);
    char unit;
    float fsize = ylog_get_unit_size_float(freespace, &unit);
    SEND(fd, buf, snprintf(buf, buf_size, "%s -> %.2f%c\n", root_path, fsize, unit), MSG_NOSIGNAL);
    return 0;
}

static int cmd_isignal(struct command *cmd, char *buf, int buf_size, int fd, int index, struct ylog_poll *yp) {
    UNUSED(cmd);
    UNUSED(index);
    UNUSED(yp);
    int *ignore_signal_process = &global_context->ignore_signal_process;
    char *last;
    char *signal;
    strtok_r(buf, " ", &last);
    signal = strtok_r(NULL, " ", &last);
    if (signal)
        *ignore_signal_process = !!strtol(signal, NULL, 0);
    SEND(fd, buf, snprintf(buf, buf_size, "signal %s\n",
                *ignore_signal_process ? "is ignored":"will be processed"), MSG_NOSIGNAL);
    return 0;
}

static void *cmd_benchmark_thread_handler(void *arg) {
    UNUSED(arg);
    struct ylog_argument *ya = (struct ylog_argument *)arg;
    char buf[2048];
    char buf_const[64];
    int index = ya->index;
    struct ylog_poll *yp = ya->yp;
    int fd = yp_fd(index, yp);
    int buf_size = sizeof buf;
    unsigned long seqt, seq = 0;
    struct ylog *y;
    char *name = "socket";
    unsigned long delta_speed_size = 0;
    struct timespec ts, ts2;
    time_t delta_speed_millisecond;
    float delta_speed_float;
    char delta_speed_unit;
    char path[PATH_MAX];
    int without_timestamp = 1;
    int timestamp = 0;
    char *p, *pbase, *pmax;
    int seq_hex_len = sizeof(seq) * 2;
    int str_len;
    int i;
    char cindex[16] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};

    if (ya->args && strcmp(ya->args, "2") == 0)
        without_timestamp = 0;
    ya->flags = 0; /* *arg can be free now */

    y = ylog_get_by_name(name);
    if (y) {
        static pthread_mutex_t benchmark_mutex_command = PTHREAD_MUTEX_INITIALIZER;
        static volatile int socket_ydst_resized = 0;
        yds_new_segment_file_name(path, sizeof path, 0, y->ydst);
        get_boottime(&ts);
        pthread_mutex_lock(&benchmark_mutex_command);
        if (++socket_ydst_resized == 1) {
            timestamp = y->timestamp;
            if (without_timestamp)
                y->timestamp = 0;
            else
                y->timestamp = 1;
            ydst_refs_inc(y);
            ylog_warn("ylog socket's ydst size is default value, change to 200M per segment\n");
            y->ydst->max_segment_size = y->ydst->max_segment_size_now = 200 * 1024 * 1024;
#if 0
            snprintf(buf, buf_size, "ylog_cli ylog socket ydst max_segment_size %d", 200);
            pcmd(buf, NULL, NULL, y, NULL);
#endif
        }
        pthread_mutex_unlock(&benchmark_mutex_command);
        p = pbase = buf_const;
        pmax = p + sizeof(buf_const);
        p += snprintf(p, pmax - p, "ylog_cli %s seq=0x", name);
        p[seq_hex_len] = '\n';
        p[seq_hex_len+1] = 0;
        str_len = p - pbase + seq_hex_len + 1;
        while ((yp->flags[index] & YLOG_POLL_FLAG_THREAD_STOP) == 0) {
#if 1
                seqt = seq++;
                for (i = seq_hex_len -1 ; i >= 0; i--) {
                    p[i] = cindex[seqt & 0xf];
                    seqt >>= 4;
                }
                delta_speed_size += y->write_handler(buf_const, str_len, y, NULL);
#else
                /**
                 * snprintf will cost so many time, let the ylog_cli benchmark2 very slow
                 * so you need to take care of the android bionic libraries by luther 2016.01.19
                 */
                delta_speed_size += y->write_handler(buf, snprintf(buf, buf_size,
                            "cmd_benchmark_thread_handler write data seq = %lld\n", seq++), y, NULL);
#endif
            if (delta_speed_size >= 20*1024*1024) {
                get_boottime(&ts2);
                delta_speed_millisecond = diff_ts_millisecond(&ts, &ts2);
                delta_speed_float = ylog_get_unit_size_float_with_speed(delta_speed_size,
                                &delta_speed_unit, delta_speed_millisecond);
                ts = ts2;
                delta_speed_size = 0;
                if (SEND(fd, buf, snprintf(buf, buf_size, "cmd_benchmark -> %s speed %.2f%c/s\n", path, delta_speed_float, delta_speed_unit), MSG_NOSIGNAL) < 0)
                    break;
                delta_speed_size += y->write_handler(buf, strlen(buf), y, NULL);
            }
        }
        pthread_mutex_lock(&benchmark_mutex_command);
        if (--socket_ydst_resized == 0) {
            ylog_warn("ylog socket's ydst size changes back to default value %dM\n",
                    YDST_TYPE_SOCKET_DEFAULT_SIZE >> 20);
            y->ydst->max_segment_size = y->ydst->max_segment_size_now = YDST_TYPE_SOCKET_DEFAULT_SIZE;
#if 0
            snprintf(buf, buf_size, "ylog_cli ylog socket ydst max_segment_size %d", YDST_TYPE_SOCKET_DEFAULT_SIZE >> 20);
            pcmd(buf, NULL, NULL, y, NULL);
#endif
            ydst_refs_dec(y);
            y->timestamp = timestamp;
        }
        pthread_mutex_unlock(&benchmark_mutex_command);
    } else {
        SEND(fd, buf, snprintf(buf, buf_size, "cmd_benchmark can't find ylog named: %s\n", name), MSG_NOSIGNAL);
    }
    mark_it_can_be_free(index, yp);
    return NULL;
}

static int cmd_thread(int index, struct ylog_poll *yp, ylog_thread_handler thread_handler, void *args) {
    pthread_t ptid;
    struct ylog_argument ya;

    ya.args = args;
    ya.index = index;
    ya.yp = yp;
    ya.flags = 1;

    yp->flags[index] |= YLOG_POLL_FLAG_FREE_LATER | YLOG_POLL_FLAG_THREAD;
    if (pthread_create(&ptid, NULL, thread_handler, &ya) == 0) {
        int wait_count = 0;
        while (ya.flags) {
            usleep(10*1000); /* wait until thread start */
            if (wait_count++ > 100 * 5) /* wait 5s */
                break;
        }
    }
    if (ya.flags) {
        yp->flags[index] &= ~(YLOG_POLL_FLAG_FREE_LATER | YLOG_POLL_FLAG_THREAD);
        ylog_critical("cmd_thread failed to pthread_create for %s\n", ya.args);
        return 0;
    }

    return 1; /* 1 keep this client opened */
}

static int cmd_benchmark(struct command *cmd, char *buf, int buf_size, int fd, int index, struct ylog_poll *yp) {
    UNUSED(buf);
    UNUSED(buf_size);
    UNUSED(fd);
    return cmd_thread(index, yp, cmd_benchmark_thread_handler, cmd->args);
}

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
            history_n = c->keep_historical_folder_numbers_default;
        c->keep_historical_folder_numbers = history_n;
        snprintf(buf, buf_size, "%d", c->keep_historical_folder_numbers);
        ylog_update_config2("keep_historical_folder_numbers", buf);
    }
    SEND(fd, buf, snprintf(buf, buf_size, "%d\n", c->keep_historical_folder_numbers), MSG_NOSIGNAL);
    return 0;
}

static int cmd_exit(struct command *cmd, char *buf, int buf_size, int fd, int index, struct ylog_poll *yp) {
    UNUSED(cmd);
    UNUSED(index);
    UNUSED(yp);
    ylog_all_thread_exit();
    print2journal_file("exit command : %s", buf);
    SEND(fd, buf, snprintf(buf, buf_size, "exit done\n"), MSG_NOSIGNAL);
    kill(getpid(), SIGKILL);
    return 0;
}

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
        history_n = global_context->keep_historical_folder_numbers_default;
    global_context->keep_historical_folder_numbers = history_n;
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

static void *cmd_snapshot_thread_handler(void *arg) {
    struct ylog_argument *ya = (struct ylog_argument *)arg;
    int index = ya->index;
    struct ylog_poll *yp = ya->yp;
    int fd = yp_fd(index, yp);
    char *cmd = ya->args;

    char buf[2048];
    int buf_size = sizeof buf;
    char *last;
    char *snp;
    int i;
    struct ylog_snapshot_list_s *sl;
    struct ylog_snapshot_list_s *ylog_snapshot_list = global_context->ylog_snapshot_list;

    ya->flags = 0; /* cmd_thread must call this in here, *arg can be free now */

    strtok_r(cmd, " ", &last);
    snp = strtok_r(NULL, " ", &last);

    if (snp) {
        struct ylog_snapshot_args sargs;
        char *result = "Failed";
        char *arg;
        sargs.argc = 0;
        do {
            arg = strtok_r(NULL, " ", &last);
            if (arg == NULL)
                break;
            sargs.argv[sargs.argc++] = arg;
            if (sargs.argc >= (int)ARRAY_LEN(sargs.argv))
                break;
        } while (1);
        for (i = 0; ylog_snapshot_list && ylog_snapshot_list[i].name != NULL; i++) {
            sl = &ylog_snapshot_list[i];
            if (strcmp(sl->name, snp) == 0) {
                sargs.data[0] = 0;
                sargs.offset = 0;
                if (sl->f)
                    sl->f(&sargs);
                if (sargs.data[sargs.offset] != 0)
                    result = sargs.data + sargs.offset;
                else
                    result = "OK";
                break;
            }
        }
        SEND(fd, buf, snprintf(buf, buf_size, "%s %s\n", snp, result), MSG_NOSIGNAL);
    } else {
        for (i = 0; ylog_snapshot_list && ylog_snapshot_list[i].name != NULL; i++) {
            sl = &ylog_snapshot_list[i];
            SEND(fd, buf, snprintf(buf, buf_size, "%-10s -- %s\n", sl->name, sl->usage), MSG_NOSIGNAL);
        }
    }

    free(cmd); /* from char *buf_dup = strdup(buf); */
    mark_it_can_be_free(index, yp); /* cmd_thread must call this in here */
    return NULL;
}

static int cmd_snapshot(struct command *cmd, char *buf, int buf_size, int fd, int index, struct ylog_poll *yp) {
    UNUSED(cmd);
    UNUSED(buf_size);
    UNUSED(fd);
    char *buf_dup = strdup(buf);

    if (buf_dup) {
        if (cmd_thread(index, yp, cmd_snapshot_thread_handler, buf_dup) == 0) {
            free(buf_dup);
            return 0;
        }
    } else {
        ylog_error("%s strdup %s failed: %s\n", __func__, buf, strerror(errno));
        return 0;
    }

    return 1;
}

static void cmd_ylog_hook(int nargs, char **args) {
    /**
     * args 0    1       2    3
     * 1. ylog enabled kernel 0
     * 2.
     */
    ylog_update_config(global_context->ylog_config_file, nargs, args, nargs - 1);
}

static struct command commands[] = {
    {"kernel", "read kernel log", cmd_kernel, NULL},
    {"-c", "execute shell command, ex. ylog_cli -c ls / or ylog_cli -c top", cmd_command, NULL},
    {"\n", NULL, cmd_help, (void*)commands},
    {"flush", "flush all the data back to disk", cmd_flush, NULL},
    {"loglevel", "0:error, 1:critical, 2:warn, 3:info, 4:debug", cmd_loglevel, NULL},
    {"history_n", "set keep_historical_folder_numbers", cmd_history_n, NULL},
    {"speed", "max speed since ylog start", cmd_speed, NULL},
    {"time", "show the time info", cmd_time, NULL},
    {"ylog", "list all existing ylog, also can start or stop it, ex.\n"\
            "              ylog_cli ylog                    - show each ylog short description\n"\
            "              ylog_cli ylog kenrel             - show ylog kernel detailed description\n"\
            "              ylog_cli ylog all                - show each ylog detailed description\n"\
            "              ylog_cli ylog all stop           - turn off all running ylog\n"\
            "              ylog_cli ylog all start          - turn on the previous all running ylog\n"\
            "              ylog_cli ylog kernel stop        - turn off the kernel ylog\n"\
            "              ylog_cli ylog kernel start       - turn on the kernel ylog\n"\
            "              ylog_cli ylog kernel get started - get the running status of kernel ylog\n"\
            "              ylog_cli ylog kernel timestamp 1 - 1 with timestamp, 0 without\n"\
            "              ylog_cli ylog kernel bypass    1 - 1 just read, not store to disk or cache, 0 store\n"\
            "              ylog_cli ylog kernel ydst max_segment 5       - ajust ydst segments to 5\n"\
            "              ylog_cli ylog kernel ydst max_segment_size 20 - ajust ydst each segment size to 20M\n"\
            "              ylog_cli ylog kernel ydst segment_size 5 20   - ajust ydst segments to 5, size to 20M\n"\
            "              ylog_cli ylog kernel ydst quota 60            - ajust ydst quota to occupy 60\% percent\n"\
            "              ylog_cli ylog kernel cache bypass 1           - data in the cache, 1 droped, 0 save to disk\n"\
            "              ylog_cli ylog kernel cache timeout 500        - cacheline timeout to 500ms\n"\
            "              ylog_cli ylog kernel cache debuglevel 0x03    - bit0: INFO, bit1: CRITICAL, bit2: WRAP, bit7: DATA"
            , cmd_ylog, NULL},
    {"cpath", "change log path, named 'ylog' will be created under it, ex. ylog_cli cpath /sdcard/", cmd_cpath, NULL},
    {"quota", "give a new quota for the ylog (unit is 'M') 500M ex. ylog_cli quota 500", cmd_quota, NULL},
    {"rylog", "last_ylog, remove the last_ylog folder", cmd_clear_ylog, (void*)(YLOG_CLEAR_LAST_YLOG)},
    {"ryloga", "all ylog, remove the last_ylog folder and also all the current saved ylog",
                                    cmd_clear_ylog, (void*)(YLOG_CLEAR_LAST_YLOG | YLOG_CLEAR_CURRENT_RUNNING)},
    {"rylogr", "all ylog and restart, remove last_ylog and ylog folder, then restart ylog service",
                                    cmd_clear_ylog, (void*)(YLOG_CLEAR_ALL_QUIT)},
    {"space", "check ylog root folder and last_ylog the size of taking up", cmd_space, NULL},
    {"freespace", "check ylog root folder free size left now", cmd_freespace, NULL},
    {"isignal", "1:ignore signal, 0:process signal(default)", cmd_isignal, NULL},
    {"benchmark", "while (1) write data to ylog/socket/open/ without timestamp", cmd_benchmark, NULL},
    {"benchmarkt", "while (1) write data to ylog/socket/open/ with timestamp", cmd_benchmark, "2"},
    {"exit", "quit all ylog threads, and kill ylog itself to protect sdcard", cmd_exit, NULL},
    {"snapshot", "snapshot the ecosystem, ex. ylog_cli snapshot", cmd_snapshot, NULL},
    {NULL, NULL, NULL, NULL}
};
