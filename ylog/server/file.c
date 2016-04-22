/**
 * Copyright (C) 2016 Spreadtrum Communications Inc.
 */

#include "analyzer_bottom_half_template.h"

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t mutex_journal_log = PTHREAD_MUTEX_INITIALIZER;
static int fd_journal_file = -1;
static long journal_last_pos = 0;

static void reset_journal_file(void) {
    pthread_mutex_lock(&mutex_journal_log);
    journal_last_pos = 0;
    pthread_mutex_unlock(&mutex_journal_log);
}

static int get_journal_file(char *buf, int buf_size) {
    int len;
    int journal_file_size;
    pthread_mutex_lock(&mutex_journal_log);
    journal_file_size = global_context->journal_file_size;
    if (journal_last_pos > journal_file_size)
        journal_last_pos = 0; /* wrap happen */
    if (journal_last_pos != journal_file_size) {
        /* 1. seek to last reading place */
        LSEEK(fd_journal_file, journal_last_pos, SEEK_SET);
        len = journal_file_size - journal_last_pos;
        if (len > buf_size)
            len = buf_size;
        /* 2. read len */
        len = read(fd_journal_file, buf, len);
        journal_last_pos += len;
        /* 3. restore to the last place for print2journal_file writing */
        LSEEK(fd_journal_file, journal_file_size, SEEK_SET);
    } else
        len = 0;
    pthread_mutex_unlock(&mutex_journal_log);
    return len;
}

static long get_journal_file_size(void) {
    if (fd_journal_file > 0) {
        struct stat st;
        if (fstat(fd_journal_file, &st) == 0)
            return st.st_size;
    }
    return 0;
}

static int print2journal_file(const char *fmt, ...) {
    static char buf[4096+2]; /* when short of memory, it can also works well */
    char timeBuf[32];
    int len;
    va_list ap;
    static long fsize = 0; /* fsize = &global_context->journal_file_size; */
    pthread_mutex_lock(&mutex_journal_log);
    if (fd_journal_file < 0) {
        struct context *c = global_context;
        char *file = c->journal_file;
        if (file) {
            char unit;
            float fsize1;
            mkdirs_with_file(file);
            fd_journal_file = open(file, O_RDWR | O_CREAT | O_APPEND, 0666); /* append mode */
            fsize = get_journal_file_size();
            fsize1 = ylog_get_unit_size_float(fsize, &unit);
            ylog_info("open %s, size is %.2f%c\n", file, fsize1, unit);
        }
        if (fd_journal_file < 0) {
            if (file)
                ylog_error("open %s failed: %s\n", file, strerror(errno));
            fd_journal_file = STDOUT_FILENO;
        }
    }
    len = ylog_get_format_time(buf);
    va_start(ap, fmt);
    len += vsnprintf(buf + len, sizeof(buf) - len - 2, fmt, ap);
    if (buf[len-1] != '\n') { /* append \n if the source does not contain */
        buf[len++] = '\n';
        buf[len] = 0;
    }
    va_end(ap);
    if ((fsize + len) > 10 * 1024 * 1024) {
        if (fd_journal_file != STDOUT_FILENO) {
            struct context *c = global_context;
            char *file = c->journal_file;
            CLOSE(fd_journal_file);
            if (file) {
                mkdirs_with_file(file);
                fd_journal_file = open(file, O_RDWR | O_CREAT | O_TRUNC, 0666); /* trunc mode */
                if (fd_journal_file > 0) {
                    ylog_info("re-open %s, size is %s\n", len);
                }
            }
            if (fd_journal_file < 0) {
                if (file)
                    ylog_error("open %s failed: %s\n", file, strerror(errno));
                fd_journal_file = STDOUT_FILENO;
            }
        }
        fsize = 0;
    }
#if 1
    fsize = get_journal_file_size();
    LSEEK(fd_journal_file, fsize, SEEK_SET);
#endif
    len = write(fd_journal_file, buf, len);
    fsize += len;
    global_context->journal_file_size = fsize;
    pthread_mutex_unlock(&mutex_journal_log);
    return 0;
}

static struct ylog *ylog_get_by_name(char *name) {
    struct ylog *y;
    int i;
    for_each_ylog(i, y, NULL) {
        if (y->name == NULL)
            continue;
        if (strcmp(y->name, name) == 0)
            return y;
    }
    return NULL;
}

static int insert_new_speed(unsigned long delta_size, int delta_millisecond,
        struct timeval *prev_tv, struct timeval *cur_tv, struct timespec *cur_ts, struct speed *speed, int max) {
    unsigned long cur_speed = ylog_get_speed(delta_size, delta_millisecond);
    int i;
    if (cur_speed <= speed[max-1].max_speed)
        return 1;
    for (i = 0; i < max; i++) {
        if (cur_speed > speed[i].max_speed) { /* insert it to the right place */
            int j;
            for (j = max - 1; j > i; j--)
                speed[j] = speed[j-1];
            speed[i].max_speed = cur_speed;
            speed[i].max_speed_size = delta_size;
            speed[i].max_speed_millisecond = delta_millisecond;
            speed[i].max_speed_tv_start = *prev_tv;
            speed[i].max_speed_tv_end = *cur_tv;
            speed[i].second_since_start = cur_ts->tv_sec - global_context->ts.tv_sec;
            break;
        }
    }
    return 0;
}

static int send_speed(int fd, char *buf, int buf_size, struct speed *speed, int max, char *prefix, char *suffix) {
    int i;
    float vspeed;
    char unit;
    struct tm delta_tm;
    char time_start[32], time_end[32];
    if (prefix)
        SEND(fd, buf, snprintf(buf, buf_size, "%s", prefix), MSG_NOSIGNAL);
    for (i = 0; i < max; i++) {
        ylog_tv2format_time(time_start, &speed[i].max_speed_tv_start);
        ylog_tv2format_time(time_end, &speed[i].max_speed_tv_end);
        vspeed = ylog_get_unit_size_float_with_speed(speed[i].max_speed_size,
                &unit, speed[i].max_speed_millisecond);
        gmtime_r(&speed[i].second_since_start, &delta_tm); /* UTC, don't support keep running more than 1 year by luther */
        SEND(fd, buf, snprintf(buf, buf_size, "%02d. %s~ %s%02d day %02d:%02d:%02d ago %.2f%c/s\n",
                    i + 1,time_start, time_end,
                    delta_tm.tm_yday, delta_tm.tm_hour,
                    delta_tm.tm_min, delta_tm.tm_sec,
                    vspeed, unit), MSG_NOSIGNAL);
    }
    if (suffix)
        SEND(fd, buf, snprintf(buf, buf_size, "%s", suffix), MSG_NOSIGNAL);
    return 0;
}

static int fcntl_read_nonblock(int fd, char *desc) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0) {
        ylog_critical("xxxxxxxxxxxxxxxxxxxxxx ylog fcntl_nonblock %s is failed, %s\n",
                desc, strerror(errno));
        return 1;
    }
    flags |= O_NONBLOCK;
    if (fcntl(fd, F_SETFL, flags) < 0) {
        ylog_critical("xxxxxxxxxxxxxxxxxxxxxx ylog fcntl_nonblock %s is failed, %s\n",
                desc, strerror(errno));
        return 1;
    }
    return 0;
}

static int fcntl_read_block(int fd, char *desc) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0) {
        ylog_critical("xxxxxxxxxxxxxxxxxxxxxx ylog fcntl_block %s is failed, %s\n",
                desc, strerror(errno));
        return 1;
    }
    flags &= ~O_NONBLOCK;
    if (fcntl(fd, F_SETFL, flags) < 0) {
        ylog_critical("xxxxxxxxxxxxxxxxxxxxxx ylog fcntl_block %s is failed, %s\n",
                desc, strerror(errno));
        return 1;
    }
    return 0;
}

static void pcmd(char *cmd, int *cnt, ylog_write_handler w, struct ylog *y, char *prefix) {
    char buf[4096];
    int count = sizeof buf;
    char *p = buf;
    char *pmax = p + sizeof(buf);
    int ret, timeout = 0;
    FILE *wfp = popen2(cmd, "r");
    char timeBuf[32];
    if (wfp) {
        int fd = fileno(wfp);
        struct pollfd pfd[1];
        if (w) {
            ylog_get_format_time(timeBuf);
            if (prefix == NULL)
                prefix = "pcmd";
            if (cnt) {
                w(p, snprintf(p, pmax - p, "\n%s %03d [ %s ] %s\n", prefix, *cnt, cmd, timeBuf), y);
                *cnt = *cnt + 1;
            } else
                w(p, snprintf(p, pmax - p, "\n%s [ %s ]\n", prefix, cmd), y);
        }
        pfd[0].fd = fd;
        pfd[0].events = POLLIN;
        do {
            ret = poll(pfd, 1, 500);
            if (ret <= 0) {
                ylog_critical("xxxxxxxxxxxxxxxxxxxxxx ylog cmd %s is failed, %s\n",
                        cmd, ret == 0 ? "timeout":strerror(errno));
                timeout = 1;
                break;
            }
            if (fcntl_read_nonblock(fd, cmd) == 0) {
                ret = read(fd, p, count);
                if (ret > 0)
                    fcntl_read_block(fd, cmd);
            } else
                ret = 0;
            if (ret > 0 && w)
                w(p, ret, y);
        } while (ret > 0);
        if (w) {
            if (timeout)
                ret = snprintf(buf, count, "xxxxxxxxxxxxxxxxxxxxxx ylog cmd %s is failed, %s\n",
                        cmd, ret == 0 ? "timeout":strerror(errno));
            else
                ret = snprintf(buf, count, "\n");
            w(buf, ret, y);
        }
        /**
         * For ylog_cli space command
         * maybe more than 500ms there is no log send back to ylog_cli space command
         * so poll will timeout
         * if we use popen / pclose(wfp); the pipe is closed by here,
         * but ylog_cli is still running, when the cmd_space du -sh return value,
         * ylog service will send(fd,...) to ylog_cli, ylog_cli poll will get data,
         * ylog_cli will call write(STDOUT_FILENO, buf, ret);
         * but the the remote end of the pipe has been closed by pclose(wfp)
         * then ylog_cli will get signal 13 (SIGPIPE), tombstones will be generated in Android
         * following are the tombstones log trace
         * so we have to use popen2 / pclose2(wfp) to let pclose2 kill the ylog_cli first [2016.02.29]
         *
         * ylog_tombstones 001 [ cat /data/tombstones/tombstone_00 ] [01-01 11:54:04.363]
         * *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** ***
         * Native Crash TIME: 12064470
         * *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** ***
         * Build fingerprint: 'SPRD/sp9832a_2h11_4mvoltesea/sp9832a_2h11_volte:6.0/MRA58K/W16.09.6N18:userdebug/test-keys'
         * Revision: '0'
         * ABI: 'arm'
         * pid: 24598, tid: 24598, name: ylog_cli  >>> /system/bin/ylog_cli <<<
         * signal 13 (SIGPIPE), code 0 (SI_USER), fault addr --------
         *     r0 ffffffe0  r1 bebfaa34  r2 00000054  r3 00000000
         *     r4 bebfaa34  r5 00000054  r6 bebfaa24  r7 00000004
         *     r8 b6f056d8  r9 b6f08000  sl b6f056c7  fp bebfba34
         *     ip 00000001  sp bebfaa18  lr b6f03a3f  pc b6de54ac  cpsr 20070010
         *     d0  0000000000000000  d1  3038343120353470
         *     d2  3236373637322061  d3  2037373836312063
         *     d4  3431203436373332  d5  3038343120383038
         *     d6  3637303135392038  d7  3933323436323120
         *     d8  0000000000000000  d9  0000000000000000
         *     d10 0000000000000000  d11 0000000000000000
         *     d12 0000000000000000  d13 0000000000000000
         *     d14 0000000000000000  d15 0000000000000000
         *     d16 0000000000000000  d17 0000000000000000
         *     d18 0000000000000000  d19 0000000000000000
         *     d20 0000000000000000  d21 0000000000000000
         *     d22 0000000000000000  d23 0000000000000000
         *     d24 0000000000000000  d25 0000000000000000
         *     d26 0000000000000000  d27 0000000000000000
         *     d28 0000000000000000  d29 0000000000000000
         *     d30 0000000000000000  d31 0000000000000000
         *     scr 00000000

         * backtrace:
         *     #00 pc 000424ac  /system/lib/libc.so (write+12)
         *     #01 pc 00000a3b  /system/bin/ylog_cli
         *     #02 pc 0001735d  /system/lib/libc.so (__libc_init+44)
         *     #03 pc 00000b40  /system/bin/ylog_cli
         */
        pclose2(wfp);
    } else {
        ylog_error("popen2 %s failed: %s\n", cmd, strerror(errno));
    }
}

static void pcmds(char *cmds[], int *cnt, ylog_write_handler w, struct ylog *y, char *prefix) {
    char **cmd;
    for (cmd = cmds; *cmd; cmd++)
        pcmd(*cmd, cnt, w, y, prefix);
}

static unsigned long long ydst_sum_all_storage_space(struct ydst *ydst) {
    int i;
    struct ydst *yd;
    float max_size_float;
    char max_unit;
    struct ydst_root *root = global_ydst_root;
    unsigned long long size = 0;
    unsigned long long quota_now = root->quota_now ? root->quota_now : root->max_size;

    for_each_ydst(i, yd, ydst) {
        if (yd->file == NULL || yd->refs <= 0)
            continue;
        size += yd->max_size_now;
    }
    if (size == 0)
        size = 1;
    for_each_ydst(i, yd, ydst) {
        if (yd->file == NULL || yd->refs <= 0)
            continue;
        max_size_float = ylog_get_unit_size_float(yd->max_size_now, &max_unit);
        ylog_info("%5.2f%% ydst %s size %.2f%c\n", (100 * yd->max_size_now) / (float)quota_now,
                yd->file, max_size_float, max_unit);
    }

    return size;
}

static unsigned long long ydst_all_transfered_size(void) {
    int i;
    struct ydst *yd;
    unsigned long long size = 0;

    for_each_ydst(i, yd, NULL) {
        if (yd->file == NULL || yd->refs <= 0)
            continue;
        size += yd->size;
    }

    return size;
}

static void show_ydst_storage_info(struct ydst_root *root) {
    float max_size_float, quota;
    char max_unit, unit;
    if (root == NULL)
        root = global_ydst_root;
    max_size_float = ylog_get_unit_size_float(ydst_sum_all_storage_space(NULL), &max_unit);
    quota = ylog_get_unit_size_float(root->quota_now ? root->quota_now : root->max_size, &unit);
    ylog_info("All ydst total size %.2f%c, quota now %.2f%c\n", max_size_float, max_unit, quota, unit);
}

static int ydst_size_new(unsigned long long max_segment_size_new, int max_segment_new, struct ydst *ydst) {
    if (max_segment_size_new == 0)
        max_segment_size_new = ydst->max_segment_size_now;
    if (max_segment_new == 0)
        max_segment_new = ydst->max_segment_now;
    ydst->max_segment_size_new = max_segment_size_new;
    ydst->max_segment_new = max_segment_new;
    ydst->max_size_new = max_segment_size_new * max_segment_new;
    return 0;
}

static int ydst_quota(unsigned long long max_segment_size_new, int max_segment_new,
        struct ylog *y, struct ydst_root *root) {
    int waiting;
    struct ydst *ydst;

    if (y == NULL)
        return -1;

    ydst = y->ydst;
    pthread_mutex_lock(&mutex);
    if (root == NULL)
        root = global_ydst_root;
    pthread_mutex_lock(&root->mutex);

    ydst_size_new(max_segment_size_new, max_segment_new, ydst);
    root->refs_cur_resize_segment = root->refs;
    root->ydst_change_seq_resize_segment++;
    ylog_critical("Notify ylog '%s' to resize\n", y->name);
    y->thread_resize_segment(y, 0);
    pthread_mutex_unlock(&root->mutex);
    /* Wait until all ydst attached onto this root resize done by luther */
    ylog_critical("Waiting for ydst '%s' resizing\n", ydst->file);
    do {
        waiting = 0;
            if (ydst->refs &&
                (ydst->max_segment_size_new != ydst->max_segment_size_now ||
                 ydst->max_segment_new != ydst->max_segment_now
                 )) {
                float fsize1, fsize2;
                char unit1, unit2;
                fsize1 = ylog_get_unit_size_float(ydst->max_segment_size_now, &unit1);
                fsize2 = ylog_get_unit_size_float(ydst->max_segment_size_new, &unit2);
                ylog_warn("%s is resizing... max_segment %d -> %d, max_segment_size %.2f%c -> %.2f%c\n",
                        ydst->file, ydst->max_segment_now, ydst->max_segment_new,
                        fsize1, unit1, fsize2, unit2);
                waiting = 1;
            }
        if (waiting)
            usleep(100*1000);
    } while (ydst->ydst_change_seq_resize_segment != root->ydst_change_seq_resize_segment);

    ylog_critical("ydst '%s' resize done\n", ydst->file);
    pthread_mutex_unlock(&mutex);

    return 0;
}

static int ydst_root_quota(struct ydst_root *root, unsigned long long quota) {
    struct ylog *y;
    int i;
    struct ydst *yd, *yd_max = NULL, *yd_min = NULL;
    int waiting;
    unsigned long long max_size_now = 0, max_size = 0;
    float scale;
    int max_segment_new;
    unsigned long long max_segment_size_new;
    float fsize1, fsize2, fsize3;
    char unit1, unit2, unit3;

    pthread_mutex_lock(&mutex);
    if (root == NULL)
        root = global_ydst_root;
    pthread_mutex_lock(&root->mutex);

    root->quota_new = quota;

    for_each_ydst(i, yd, NULL) {
        if (yd->file == NULL || yd->refs <= 0)
            continue;
        if (yd_max == NULL)
            yd_max = yd;
        else if (yd_max->max_size < yd->max_size)
            yd_max = yd;
        if (yd_min == NULL)
            yd_min = yd;
        else if (yd_min->max_size > yd->max_size)
            yd_min = yd;
        max_size_now += yd->max_size_now;
    }

    max_size = root->max_size;
    scale = (float)quota / max_size;

    fsize1 = ylog_get_unit_size_float(max_size_now, &unit1);
    fsize2 = ylog_get_unit_size_float(max_size, &unit2);
    fsize3 = ylog_get_unit_size_float(quota, &unit3);
    ylog_critical("max_size_now=%.2f%c, max_size=%.2f%c, quota=%.2f%c, scale=%.4f quota/max_size\n",
            fsize1, unit1, fsize2, unit2, fsize3, unit3, scale);

    max_size = 0;
    for_each_ydst(i, yd, NULL) {
        if (yd->file == NULL || yd->refs <= 0)
            continue;
        if (yd == yd_max)
            continue;
        if (yd->max_segment > 1) {
            max_segment_new = yd->max_segment * scale;
            if (max_segment_new == 0)
                max_segment_new = 1;
            max_segment_size_new = yd->max_segment_size;
        } else {
            max_segment_new = yd->max_segment;
            max_segment_size_new = yd->max_segment_size * scale;
            if (max_segment_size_new < 64*1024)
                max_segment_size_new = 64*1024;
        }
        ydst_size_new(max_segment_size_new, max_segment_new, yd);
        max_size += max_segment_size_new * max_segment_new;
    }

    fsize1 = ylog_get_unit_size_float(yd_max->max_segment_size * yd_max->max_segment, &unit1);
    fsize2 = ylog_get_unit_size_float(max_size, &unit2);
    ylog_critical("space used except maximum ydst <%s> %.2f%c/%.2f%c, quota is %.2f%c\n",
            yd_max->file, fsize2, unit2, fsize1, unit1, fsize3, unit3);

    if (quota < max_size) {
        max_size = 1024*1024;
    } else {
        max_size = quota - max_size;
    }

    max_segment_new = max_size / yd_max->max_segment_size_new;
    if (max_segment_new == 0)
        max_segment_new = 1;
    ydst_size_new(yd_max->max_segment_size_new, max_segment_new, yd_max);

    show_ydst_storage_info(root);

    root->refs_cur_resize_segment = root->refs;
    root->ydst_change_seq_resize_segment++;

    ylog_critical("Notify all ylog to resize\n");
    for_each_ylog(i, y, NULL) {
        if (y->name == NULL)
            continue;
        y->thread_resize_segment(y, 0); /* Because some threads maybe there is no log any more by luher*/
    }
    pthread_mutex_unlock(&root->mutex);

    /* Wait until all ydst attached onto this root resize done by luther */
    ylog_critical("Waiting for all ydst resizing\n");
    do {
        waiting = 0;
        for_each_ydst(i, yd, NULL) {
            if (yd->refs &&
                (yd->max_segment_size_new != yd->max_segment_size_now ||
                 yd->max_segment_new != yd->max_segment_now ||
                 yd->ydst_change_seq_resize_segment != root->ydst_change_seq_resize_segment)) {
                float fsize1, fsize2;
                char unit1, unit2;
                fsize1 = ylog_get_unit_size_float(yd->max_segment_size_now, &unit1);
                fsize2 = ylog_get_unit_size_float(yd->max_segment_size_new, &unit2);
                ylog_warn("%s is resizing... max_segment %d -> %d, max_segment_size %.2f%c -> %.2f%c\n",
                        yd->file, yd->max_segment_now, yd->max_segment_new,
                        fsize1, unit1, fsize2, unit2);
                waiting = 1;
            }
        }
        if (waiting)
            usleep(100*1000);
    } while (root->refs_cur_resize_segment);

    ylog_critical("All ydst resize done\n");
    show_ydst_storage_info(root);
    pthread_mutex_unlock(&mutex);

    return 0;
}

static int ydst_root_new(struct ydst_root *root, char *root_new) {
    struct ylog *y;
    int i;
    struct ydst *yd;
    int waiting;

    pthread_mutex_lock(&mutex);

    if (root == NULL)
        root = global_ydst_root;

    if (root->root && strcmp(root->root, root_new) == 0) {
        ylog_critical("The root path is already changed to %s\n", root->root);
        pthread_mutex_unlock(&mutex);
        return 1;
    }

    pthread_mutex_lock(&root->mutex);
    /* Send YLOG_MOVE_ROOT state */
    root->root_new = strdup(root_new);
    root->refs_cur_move_root = root->refs;
    root->ydst_change_seq_move_root++;
    ylog_critical("Notify all ylog to change root from %s to %s\n", root->root, root_new);
    for_each_ylog(i, y, NULL) {
        if (y->name == NULL)
            continue;
        y->thread_move_root(y, 0); /* Because some threads maybe there is no log any more by luher*/
    }
    pthread_mutex_unlock(&root->mutex);
    /* Wait until all ydst attached onto this root move finish by luther */
    ylog_critical("Waiting for all ydst root move to %s\n", root_new);
    do {
        int num = 0;
        waiting = 0;
        for_each_ydst(i, yd, NULL) {
            if (yd->refs && yd->ydst_change_seq_move_root != root->ydst_change_seq_move_root) {
                ylog_warn("<index %d> %s is moving... %d,%d\n",
                        num++, yd->file, yd->ydst_change_seq_move_root, root->ydst_change_seq_move_root);
                waiting = 1;
            }
        }
        if (waiting)
            usleep(100*1000);
    } while (root->refs_cur_move_root);
    ylog_critical("All ydst finished root move to %s\n", root_new);

    pthread_mutex_unlock(&mutex);

    return 0;
}

static int yds_new_segment_file_name(char *path, int len, int segment, struct ydst *ydst) {
    int multi = 1;
    char *file = ydst->file;
    if (ydst->max_segment_now > 1 || ydst->max_segment > 1)
        snprintf(path, len, "%s/%s%03d", ydst->root_folder, file, segment);
    else {
        multi = 0;
        if (file[strlen(file) - 1] != '/')
            snprintf(path, len, "%s/%s", ydst->root_folder, file);
        else
            snprintf(path, len, "%s/%s/noname", ydst->root_folder, file);
    }
    return multi;
}

static int ydst_shrink_file(int segment_from, int segment_to, struct ydst *ydst) {
    char file_to[PATH_MAX];
    int i;
    /* Do i need to shrink or inflate myself by luther */
    for (i = segment_from; i < segment_to; i++) {
        yds_new_segment_file_name(file_to, sizeof file_to, i, ydst);
        if (access(file_to, F_OK) == 0) {
            rm_all(file_to);
        }
    }
    return 0;
}

static int yds_rename_segment_sequnce_file_and_left_segment0(struct ydst *ydst) {
    char file_from[PATH_MAX];
    char file_to[PATH_MAX];
    int i;

    for (i = ydst->max_segment_now - 1; i; i--) {
        yds_new_segment_file_name(file_from, sizeof file_from, i - 1, ydst);
        if (access(file_from, F_OK)) {
            // ylog_debug("%s does not exist.\n", file_from);
        } else {
            yds_new_segment_file_name(file_to, sizeof file_to, i, ydst);
            mv(file_from, file_to);
        }
    }

    ydst_shrink_file(ydst->max_segment_now, ydst->max_segment, ydst);

    return 0;
}

static void create_outline(struct ydst *ydst) {
    char file_from[PATH_MAX];
    char file_to[PATH_MAX];
    char buf[512];
    char timestamp[32] = {0};
    int multi;
    int i, cur_seg;
    char *b = buf, *bmax = buf + sizeof(buf);
    int from;
    int fd_outline = -1;

    for (i = ydst->max_segment_now - 1; i >= 0;) {
        cur_seg = i;
        multi = yds_new_segment_file_name(file_to, sizeof file_to, i, ydst);
        if (multi == 0)
            goto _exit;
        i--;
        ylog_debug("xxxxxxxxxxxxxxxxxxx=%s\n", file_to);
        if (access(file_to, F_OK))
            continue;
        ylog_debug("yyyyyyyyyyyyyyyyyyy=%s\n", file_to);
        from = 0;
        for (; i >= 0; i--) {
            yds_new_segment_file_name(file_from, sizeof file_from, i, ydst);
            if (access(file_from, F_OK))
                continue;
            from = 1;
            break;
        }
        /**
         * 017 - 2016.02.15 10:00:00 ~ 2016.02.15 18:19:08
         * 016 - 2016.02.15 18:19:08 ~
         *
         * "U1[ylog_segment=0/10,5.00M] 2016.02.15 18:19:08 -00d00:00:00/4ms 0.00B/50.00M 0.00B/s"
         *
         * 0, 1, 2, 3, 4, 5, 6, 7, 8
         * from = 7
         * to = 8
         */
        {
            char *p, *last;
            int ret;
            int len = 100;
            int fd_f = -1;
            int fd_t = open(file_to, O_RDONLY);
            if (from)
                fd_f = open(file_from, O_RDONLY);
            if (fd_f >= 0 || fd_t >= 0) {
                if (fd_outline < 0) {
                    dirname2(file_to);
                    strcpy(file_to + strlen(file_to), "/outline");
                    fd_outline = open(file_to, O_RDWR | O_CREAT | O_TRUNC, 0644);
                }
                if (fd_t >= 0)
                    ret = read(fd_t, file_to, len);
                else
                    ret = 0;
                if (ret < 0) {
                    ret = 0;
                    ylog_error("create_outline read file_to failed: %s\n", strerror(errno));
                }
                file_to[ret] = 0;
                p = file_to;
                strtok_r(p, "]", &last);
                p = strtok_r(NULL, "-", &last);
                buf[0] = 0;
                if (timestamp[0] == 0) /* ylog_write_header_default */
                    snprintf(timestamp, sizeof timestamp, " 20%02d.%02d.%02d %02d:%02d:%02d ",
                            ydst->tm.tm_year % 100,
                            ydst->tm.tm_mon + 1,
                            ydst->tm.tm_mday,
                            ydst->tm.tm_hour,
                            ydst->tm.tm_min,
                            ydst->tm.tm_sec);
                if (p == NULL) /* ydst cache maybe does not flush back now luther */
                    p = timestamp;
                b = buf;
                b += snprintf(b, bmax - b, "%03d -%s~", cur_seg, p);
                if (from) {
                    extern char *strptime(const char *s, const char *format, struct tm *tm);
                    struct tm tm_before, tm_after, delta_tm;
                    time_t time_t_before, time_t_after, delta_seconds;
                    memset(&tm_before, 0, sizeof(struct tm));
                    strptime(p, " %Y.%m.%d %H:%M:%S ", &tm_before);
                    if (fd_f >= 0)
                        ret = read(fd_f, file_from, len);
                    else
                        ret = 0;
                    if (ret < 0) {
                        ret = 0;
                        ylog_error("create_outline read file_from failed: %s\n", strerror(errno));
                    }
                    file_from[ret] = 0;
                    p = file_from;
                    strtok_r(p, "]", &last);
                    p = strtok_r(NULL, "-", &last);
                    if (p == NULL) { /* ydst cache maybe does not flush back now luther */
                        p = file_from;
                        strcpy(p, timestamp);
                    }
                    memset(&tm_after, 0, sizeof(struct tm));
                    strptime(p, " %Y.%m.%d %H:%M:%S ", &tm_after);
                    time_t_before = mktime(&tm_before);
                    time_t_after = mktime(&tm_after);
                    if (time_t_before != -1 && time_t_after != -1) {
                        delta_seconds = time_t_after - time_t_before;
                        gmtime_r(&delta_seconds, &delta_tm);
                        b += snprintf(b, bmax - b, "%s[%02d %02d:%02d:%02d]\n",
                                p,
                                delta_tm.tm_yday,
                                delta_tm.tm_hour,
                                delta_tm.tm_min,
                                delta_tm.tm_sec);
                    } else {
                        p[strlen(p) - 1] = '\n';
                        b += snprintf(b, bmax - b, "%s", p);
                    }
                } else {
                    strcat(buf, "\n");
                }
                if (fd_outline >= 0)
                    write(fd_outline, buf, strlen(buf));
            }
            if (fd_f >= 0)
                CLOSE(fd_f);
            if (fd_t >= 0)
                CLOSE(fd_t);
        }
    }
_exit:
    if (fd_outline >= 0)
        CLOSE(fd_outline);
}

static int ydst_pre_fill_zero_to_possession_storage_spaces(struct ydst *ydst,
        int excluded_segment, struct context *c) {
    if (c == NULL)
        c = global_context;

    if (c->pre_fill_zero_to_possession_storage_spaces) {
        int ret, size;
        int segment;
        char path[PATH_MAX];
        char buf[512];
        int buf_size = sizeof buf;
        int fd;

        memset(buf, 0, buf_size);
        buf[buf_size - 1] = '\n';

        for (segment = 0; segment < ydst->max_segment_now; segment++) {
            yds_new_segment_file_name(path, sizeof path, segment, ydst);
            if (excluded_segment == segment) {
                ylog_critical("excluded_segment %d to %s is ignored\n", excluded_segment, path);
                continue;
            }
            ylog_critical("Fill 0 to %s\n", path);
            if (mkdirs_with_file(path) == 0) {
                fd = open(path, O_RDWR | O_CREAT | O_TRUNC, 0664);
                if (fd < 0) {
                    ylog_critical("Failed to open %s\n", strerror(errno));
                    continue;
                }
                size = ydst->max_segment_size_now;
                do {
                    if (buf_size > size) {
                        buf_size = size;
                        buf[size - 1] = '\n';
                    }
                    ret = write(fd, buf, buf_size);
                    if (ret != buf_size) {
                        ylog_critical("Failed to %d write size %d, real size %d, %s\n", fd, size, ret, strerror(errno));
                    }
                    size -= ret;
                } while (size);
                CLOSE(fd);
            } else {
                ylog_critical("Failed to create dir %s\n", path);
            }
        }

        /* Do i need to shrink or inflate myself by luther */
        for (segment = ydst->max_segment_now; segment < ydst->max_segment; segment++) {
            yds_new_segment_file_name(path, sizeof path, segment, ydst);
            if (access(path, F_OK) == 0) {
                rm_all(path);
            }
        }
    }

    return 0;
}

static int ylog_historical_folder_do(char *root, char *historical_folder_root,
        int start_number, int keep_historical_folder_numbers, int only_rm) {
    int i;
    char tmp[PATH_MAX];
    char tmp_to[PATH_MAX];
    if (keep_historical_folder_numbers == 0) {
        ylog_critical("No need to keep historical ylog folder...\n");
        rm_sub_all(root);
        if (rmdir(root) == 0)
            ylog_warn("rmdir %s remove empty folder\n", root);
    } else {
        /**
         * if keep_historical_folder_numbers == 0
         * --> ylog/ylog
         * if keep_historical_folder_numbers == 1
         * --> ylog/ylog, ylog/ylog1
         * if keep_historical_folder_numbers == 2
         * --> ylog/ylog, ylog/ylog1, ylog/ylog2
         * by luther
         */
        char basename_root[PATH_MAX];
        char *pbasename_root;
        strcpy(basename_root, root);
        pbasename_root = basename(basename_root);
        ylog_critical("Check historical ylog folder...\n");
        for (i = keep_historical_folder_numbers; i >= start_number; i--) {
            if (historical_folder_root)
                snprintf(tmp, sizeof(tmp), "%s/%s%d", historical_folder_root, pbasename_root, i);
            else
                snprintf(tmp, sizeof(tmp), "%s%d", root, i);
            if (access(tmp, F_OK)) {
                if (only_rm == 0)
                    ylog_critical("%s does not exist.\n", tmp);
            } else {
                if (only_rm == 0) {
                    if (i == keep_historical_folder_numbers)
                        rm_all(tmp);
                    else {
                        if (historical_folder_root)
                            snprintf(tmp_to, sizeof(tmp_to), "%s/%s%d", historical_folder_root, pbasename_root, i+1);
                        else
                            snprintf(tmp_to, sizeof(tmp_to), "%s%d", root, i+1);
                        if (access(tmp_to, F_OK) == 0)
                            rm_all(tmp_to);
                        mv(tmp, tmp_to);
                    }
                } else {
                    rm_all(tmp);
                }
            }
            if (only_rm == 0) {
                if (rmdir(root) == 0)
                    ylog_warn("rmdir %s remove empty folder\n", root);
                if (i == 1 && access(root, F_OK) == 0) {
                    mkdirs_with_file(tmp);
                    mv(root, tmp);
                }
            }
        }
        if (historical_folder_root)
            rmdir(historical_folder_root);
    }
    return 0;
}

static int ylog_root_folder_delete(char *root, char *historical_folder_root, int start_number, int end_number) {
    struct ydst_root *yroot = global_ydst_root;
    int ret;
    pthread_mutex_lock(&yroot->mutex);
    if (end_number < 0) {
        start_number = 1;
        end_number = 30;
    }
    if (start_number < 0)
        start_number = 1;
    ret = ylog_historical_folder_do(root, historical_folder_root, start_number, end_number, 1);
    pthread_mutex_unlock(&yroot->mutex);
    return ret;
}

static int ylog_historical_folder(char *root, struct context *c) {
    return ylog_historical_folder_do(root, c->historical_folder_root, 1, c->keep_historical_folder_numbers, 0);
}

static void ydst_move_other_files(char *root_old, char *root_new) {
    UNUSED(root_old);
    UNUSED(root_new);
    return;
}

static int ydst_move_root(struct ydst_root *root, struct ydst *ydst,
            struct ylog *y, int ydst_change_seq_move_root, int excluded_segment) {
    struct context *c = global_context;
    int moved = 0;
    if (root == NULL)
        root = global_ydst_root;
    if (ydst->ydst_change_seq_move_root != ydst_change_seq_move_root) {
        char tmp[PATH_MAX];
        char tmp_to[PATH_MAX];
        int refs_cur_move_root = root->refs_cur_move_root;
        ylog_critical("ydst <%s> %s moves from %s to %s, refs_cur_move_root=%d\n",
                ydst->file, y->name, ydst->root_folder, root->root_new, refs_cur_move_root);
        /**
         * only move yourself, don't need to care others
         */
        if (refs_cur_move_root == root->refs) {
            ylog_historical_folder(root->root_new, c);
        }
        if (mkdirs(root->root_new) == 0) {
            snprintf(tmp, sizeof(tmp), "%s/%s", ydst->root_folder, ydst->file);
            dirname2(tmp);
            if (strcmp(tmp, ydst->root_folder) == 0)
                snprintf(tmp, sizeof(tmp), "%s/%s", ydst->root_folder, ydst->file);
            snprintf(tmp_to, sizeof(tmp_to), "%s/%s", root->root_new, ydst->file);
            dirname2(tmp_to);
            if (strcmp(tmp_to, root->root_new))
                dirname2(tmp_to);
            ydst_move_other_files(tmp, tmp_to);
            if (access(tmp, F_OK) == 0) {
                mkdirs(tmp_to);
                mv(tmp, tmp_to);
                while (strcmp(tmp, ydst->root_folder)) {
                    dirname2(tmp);
                    rmdir(tmp);
                    ylog_critical("rmdir %s\n", tmp);
                }
            } else
                ylog_warn("File %s does not exist\n", tmp);
        }
        ydst->root_folder = root->root_new;
        ydst_pre_fill_zero_to_possession_storage_spaces(ydst, excluded_segment, NULL);
        if (refs_cur_move_root && --refs_cur_move_root == 0) {
            /* All sub folder under root have been moved out to the root_new by ltuher */
            ylog_critical("All sub folder under %s have been moved out to the %s\n", root->root, root->root_new);
            if (0 && rmdir(root->root)) {
                ylog_error("rmdir %s failed: %s\n", root->root, strerror(errno));
            }
            free(root->root); /* root->root must be with strdup or malloc */
            root->root = root->root_new;
        } else {
            ylog_warn("%d sub folder is left, they are moving...\n", refs_cur_move_root);
        }
        ydst->ydst_change_seq_move_root = ydst_change_seq_move_root;
        root->refs_cur_move_root = refs_cur_move_root;
        moved = 1;
    }
    return moved;
}

static void ydst_cache_lock(struct ydst *ydst, struct cacheline *cl) {
    if (ydst->cache_locked == 0) {
        ylog_debug("try to get cacheline %s mutex\n", cl->name);
        pthread_mutex_lock(&cl->mutex);
        ydst->cache_locked = 1;
        while (cl->writing) {
            ylog_info("cacheline %s is writing, wait 10ms\n", cl->name);
            usleep(10*1000);
        }
    } else
        ylog_debug("cacheline %s is locked\n", cl->name);
}

static void ydst_cache_unlock(struct ydst *ydst, struct cacheline *cl) {
    if (ydst->cache_locked) {
        ylog_debug("cacheline %s mutex is released\n", cl->name);
        ydst->cache_locked = 0;
        pthread_mutex_unlock(&cl->mutex);
    } else
        ylog_critical("cacheline %s is not locked\n", cl->name);
}

static int ydst_nowrap_segment(struct ydst *ydst) {
    return ydst->nowrap && (ydst->segments >= ydst->max_segment_now);
}

static int ydst_new_segment_default(struct ylog *y, int ymode) {
    int written_count = 0;
    struct ydst *ydst = y->ydst;
    struct ydst_root *root = ydst->root;
    char *file = ydst->file;
    char *mode = ydst->mode;
    char path[PATH_MAX];
    int need_reopen = 0;
    int segment;
    struct cacheline *cl = ydst->cache;
    int ydst_change_seq_move_root;
    int ydst_change_seq_resize_segment;
    int moved = 0;
    int nowrap;
    int generate_analyzer = 0;

    if (cl) {
        if (ydst->write_data2cache_first == 0)
            ydst_cache_lock(ydst, cl);
        else
            cl = NULL;
    }

    pthread_mutex_lock(&root->mutex);

    ydst_change_seq_move_root = root->ydst_change_seq_move_root;
    ydst_change_seq_resize_segment = root->ydst_change_seq_resize_segment;
    nowrap = ydst_nowrap_segment(ydst);

    if (ymode == YDST_SEGMENT_MODE_NEW ||
        ymode == YDST_SEGMENT_MODE_RESET ||
        ydst->ydst_change_seq_move_root != ydst_change_seq_move_root) {
        /* need_reopen == 1 ydst has been opened, so we need to re-open it as well */
        need_reopen = ydst->close(ydst); /* if ydst has cache, flush on cached data back to disk */
    }

    if (ymode == YDST_SEGMENT_MODE_RESET) {
        y->size = 0;
        ydst->segments = 0;
        ydst->segment = 0;
        ydst->prev_size = ydst->size = 0;
        ydst->segment_size = 0;
        ydst_shrink_file(0, ydst->max_segment_now, ydst);
        pthread_mutex_unlock(&root->mutex);
        if (cl)
            ydst_cache_unlock(ydst, cl);
        return 0;
    }

    if (ydst->ydst_change_seq_resize_segment != ydst_change_seq_resize_segment) {
        if (ydst->ydst_change_seq_resize_segment != ydst_change_seq_resize_segment) {
            float fsize1, fsize2, fsize3, fsize4, fsize5, fsize6, fsize7, fsize8;
            char unit1, unit2, unit3, unit4, unit5, unit6, unit7, unit8;
            int shrinked_segments = 0;
            int refs_cur_resize_segment = root->refs_cur_resize_segment;
            if (ydst->max_size_new < ydst->max_size_now) {
                /* shrink more */
                shrinked_segments += \
                    (ydst->max_size_now - ydst->max_size_new + ydst->max_segment_size_now - 1) / ydst->max_segment_size_now;
                if (shrinked_segments >= ydst->max_segment_now)
                    shrinked_segments--;
            }
            if (ydst->max_segment_now > ydst->max_segment_new) {
                switch (ydst->segment_mode) {
                case YDST_SEGMENT_SEQUNCE:
                    segment = 0;
                    break;
                case YDST_SEGMENT_CIRCLE:
                    if (ydst->segment >= ydst->max_segment_new) {
                        segment = 0;
                        /* need_reopen == 1 ydst has been opened, so we need to re-open it as well */
                        need_reopen = ydst->close(ydst); /* if ydst has cache, flush on cached data back to disk */
                    }
                    break;
                }
            }
            fsize1 = ylog_get_unit_size_float(ydst->max_segment_size_now, &unit1);
            fsize2 = ylog_get_unit_size_float(ydst->max_segment_size_new, &unit2);
            fsize3 = ylog_get_unit_size_float(ydst->max_size_now, &unit3);
            fsize4 = ylog_get_unit_size_float(ydst->max_size_new, &unit4);
            fsize5 = ylog_get_unit_size_float(root->quota_now, &unit5);
            fsize6 = ylog_get_unit_size_float(root->quota_new, &unit6);
            fsize7 = ylog_get_unit_size_float(ydst->max_segment_size, &unit7);
            fsize8 = ylog_get_unit_size_float(ydst->max_size, &unit8);
            ylog_debug("\nydst <%s> resize_segment from:\n" \
                    "quota %.2f%c -> %.2f%c\n"\
                    "max_segment_size %.2f%c -> %.2f%c (%.2f%c)\n"\
                    "max_segment %d -> %d (%d)\n"\
                    "max_size %.2f%c -> %.2f%c (%.2f%c)\n"\
                    "shrinked_segments=%d/%d\n\n",
                    ydst->file,
                    fsize5, unit5, fsize6, unit6,
                    fsize1, unit1, fsize2, unit2, fsize7, unit7,
                    ydst->max_segment_now, ydst->max_segment_new, ydst->max_segment,
                    fsize3, unit3, fsize4, unit4, fsize8, unit8,
                    shrinked_segments, ydst->max_segment_now);
            /* shrink segment */
            if (shrinked_segments)
                ydst_shrink_file(ydst->max_segment_now - shrinked_segments, ydst->max_segment_now, ydst);
            ydst->max_segment_size_now = ydst->max_segment_size_new;
            ydst->max_segment_now = ydst->max_segment_new;
            ydst->max_size_now = ydst->max_size_new;
            if (refs_cur_resize_segment && --refs_cur_resize_segment == 0) {
                /* All sub ydst under root have finished resizing by ltuher */
                root->quota_now = root->quota_new;
                ylog_critical("All ydst has finished resize: quota %.2f%c -> %.2f%c\n",
                        fsize5, unit5, fsize6, unit6);
            } else {
                ylog_warn("%d sub ydst is left, they are resizing...\n", refs_cur_resize_segment);
            }
            ydst->ydst_change_seq_resize_segment = ydst_change_seq_resize_segment;
            root->refs_cur_resize_segment = refs_cur_resize_segment;
        }
    }

    if (ydst->ydst_change_seq_move_root != ydst_change_seq_move_root) {
        int excluded_segment;
        if (ymode == YDST_SEGMENT_MODE_UPDATE) {
            mode = "a+";
            switch (ydst->segment_mode) {
            case YDST_SEGMENT_SEQUNCE:
                segment = 0;
                break;
            case YDST_SEGMENT_CIRCLE:
                if (ydst->segment)
                    segment = ydst->segment - 1;
                else {
                    if (need_reopen)
                        segment = ydst->max_segment_now - 1;
                    else
                        segment = 0; /* First start this ylog */
                }
                break;
            }
            excluded_segment = segment;
        } else {
            excluded_segment = -1;
        }
        moved = ydst_move_root(root, ydst, y, ydst_change_seq_move_root, excluded_segment);
    }

    pthread_mutex_unlock(&root->mutex);

    if (ymode == YDST_SEGMENT_MODE_NEW) {
        switch (ydst->segment_mode) {
        case YDST_SEGMENT_SEQUNCE:
            segment = 0;
            yds_rename_segment_sequnce_file_and_left_segment0(ydst);
            break;
        case YDST_SEGMENT_CIRCLE:
            segment = ydst->segment;
            break;
        }
    }

    path[0] = 0;
    if (ymode == YDST_SEGMENT_MODE_NEW || need_reopen) {
        char *ydst_file = NULL;
        if (file) {
            yds_new_segment_file_name(path, sizeof path, segment, ydst);
            if (mkdirs_with_file(path) == 0) {
                ydst_file = path;
            }
        }
        if (nowrap == 0) {
            ydst->open(ydst_file, mode, ydst); /* if ydst has cache, restart cache line */
            ylog_debug("new_segment: fopen %s mode=%s\n", path, mode);
        } else {
            ylog_critical("ylog %s is forced stop, because ydst %s has reached max_segment %d/%ld\n",
                    y->name, ydst->file, ydst->max_segment_now, ydst->segments);
            print2journal_file("ylog %s is forced stop, because ydst %s has reached max_segment %d/%ld\n",
                    y->name, ydst->file, ydst->max_segment_now, ydst->segments);
            y->thread_stop(y, 0);
        }
    }

    if (cl)
        ydst_cache_unlock(ydst, cl);

    if (ymode == YDST_SEGMENT_MODE_NEW && nowrap == 0) {
        ylog_debug("new_segment: new %d\n", segment);
        if (y->raw_data == 0)
            written_count += y->write_header(y);
        ydst->size += written_count;
        y->size += written_count;
        ydst->segment_size = written_count;
        ydst->segment = (segment + 1) % ydst->max_segment_now;
        ydst->segments++;
        if (y->raw_data == 0)
            create_outline(ydst);
    }

    // if ((ydst->segments == 1 || moved) && (ydst->max_segment_now > 1 || ydst->max_segment > 1)) {
    if ((ydst->segments == 1) && (ydst->max_segment_now > 1 || ydst->max_segment > 1))
        generate_analyzer = 1;

    if (generate_analyzer && ydst->ytag) {
        struct ytag_header ytag_header;
        memset(&ytag_header, 0, sizeof(struct ytag_header));
        ytag_header.tag = YTAG_MAGIC;
        ytag_header.len = sizeof(struct ytag_header);
        ytag_header.version = YTAG_VERSION;
        /* append ytag in the first segment file */
        written_count = ydst->write(y->id_token, y->id_token_len, (char*)&ytag_header, sizeof(struct ytag_header), ydst);
        ydst->size += written_count;
        y->size += written_count;
        ydst->segment_size += written_count;
    }

    if (generate_analyzer) {
        int fd;
        /* First time to generate this ydst folder */
        /**
         * run.sh
         * name="luther"
         * >${name}
         * ls | tr -s ' ' | sed '/[0-9]$/!d;s/\ /\n/g' | sort -r | xargs -I file bash -c "cat file >>$name"
         */
#if 0
        char *run_script = \
">$name\n" \
"ls | tr -s ' ' | sed '/[0-9]$/!d;s/\\ /\\n/g' | sort -r | xargs -I file bash -c \"cat file >>$name\"\n";
#endif
        // sed 's/\\/\\\\/g;s/\(.*\)/"\1\\n" \\/' analyzer.py
        char *template_python1 = \
"#!/usr/bin/env python\n" \
"# -*- coding:utf-8 -*-\n" \
"'''\n" \
"Copyright (C) 2016 Spreadtrum\n" \
"Created on Jan 18, 2016\n" \
"2016.03.11 -- add ytag parser\n" \
"'''\n" \
"\n";

#if 0
"YTAG = 1\n" \
"YTAG_MAGIC = 0xf00e789a\n" \
"YTAG_VERSION = 1\n" \
"YTAG_TAG_PROPERTY = 0x05\n" \
"YTAG_TAG_NEWFILE_BEGIN = 0x10\n" \
"YTAG_TAG_NEWFILE_END = 0x11\n" \
"YTAG_TAG_RAWDATA = 0x12\n" \
"YTAG_STRUCT_SIZE = 0x08\n" \
"ytag_folder = 'ytag'\n" \
"merged = 'all_log'\n"
"logpath = ''\n"
"id_token_len = 1\n"
"logdict = {\n"
"'A':'main.log',\n"
"'B':'event.log'\n"
"}\n"
#endif

        yds_new_segment_file_name(path, sizeof path, 0, ydst);
        dirname2(path);
        strcpy(path + strlen(path), "/analyzer.py");
        fd = open(path, O_RDWR | O_CREAT | O_TRUNC, 0755);
        ylog_info("create %s\n", path);
        if (fd >= 0) {
            int i;
            char *p;
            write(fd, template_python1, strlen(template_python1));
            write(fd, path, snprintf(path, sizeof(path), "YTAG = %d\n", ydst->ytag));
            write(fd, path, snprintf(path, sizeof(path), "YTAG_MAGIC = 0x%08x\n", YTAG_MAGIC));
            write(fd, path, snprintf(path, sizeof(path), "YTAG_VERSION = 0x%08x\n", YTAG_VERSION));
            write(fd, path, snprintf(path, sizeof(path), "YTAG_TAG_PROPERTY = 0x%02x\n", YTAG_TAG_PROPERTY));
            write(fd, path, snprintf(path, sizeof(path), "YTAG_TAG_NEWFILE_BEGIN = 0x%02x\n", YTAG_TAG_NEWFILE_BEGIN));
            write(fd, path, snprintf(path, sizeof(path), "YTAG_TAG_NEWFILE_END = 0x%02x\n", YTAG_TAG_NEWFILE_END));
            write(fd, path, snprintf(path, sizeof(path), "YTAG_TAG_RAWDATA = 0x%02x\n", YTAG_TAG_RAWDATA));
            write(fd, path, snprintf(path, sizeof(path), "YTAG_STRUCT_SIZE = 0x%02x\n", (unsigned int)sizeof(struct ytag)));
            write(fd, path, snprintf(path, sizeof(path), "ytag_folder = 'ytag'\n"));
            write(fd, path, snprintf(path, sizeof(path), "merged = '%s'\n", ydst->file_name));
            write(fd, path, snprintf(path, sizeof(path), "logpath = ''\n"));
            write(fd, path, snprintf(path, sizeof(path), "id_token_len = %d\n", y->id_token_len));
            write(fd, path, snprintf(path, sizeof(path), "logdict = {\n"));
            for_each_ylog(i, y, NULL) {
                if (y->name == NULL)
                    continue;
                if (y->ydst == ydst && y->id_token_len) {
                    char *pmax = path + sizeof(path);
                    p = path;
                    *p++ = '\'';
                    memcpy(p, y->id_token, y->id_token_len);
                    p += y->id_token_len;
                    *p++ = '\'';
                    p += snprintf(p, pmax - p, ":'%s',\n", y->id_token_filename);
                    write(fd, path, p - path);
                    //write(fd, path, snprintf(path, sizeof(path), "'%s':'%s',\n", y->id_token, y->id_token_filename));
                }
            }
            write(fd, path, snprintf(path, sizeof(path), "}\n\n"));
            write(fd, analyzer_bottom_half_template, strlen(analyzer_bottom_half_template));
            CLOSE(fd);
        } else {
            ylog_critical("Failed: create %s\n", path);
        }
    }

    return 0;
}

static int ylog_open_default(char *file, char *mode, struct ylog *y) {
    FILE *f;
    int fd = -1;
    struct ylog_poll *yp = &y->yp;

    switch (y->type) {
    case FILE_NORMAL:
        f = fopen(file, mode);
        if (f == NULL) {
            ylog_error("open %s failed: %s\n", file, strerror(errno));
            return -1;
        }
        yp_set(f, -1, YLOG_POLL_INDEX_DATA, yp, NULL);
        break;
    case FILE_SOCKET_LOCAL_SERVER:
        if (create_socket_local_server(&fd, file) < 0) {
            ylog_error("open %s failed: %s\n", file, strerror(errno));
            if (fd >= 0)
                CLOSE(fd);
            return -1;
        }
        yp_set(NULL, fd, YLOG_POLL_INDEX_SERVER_SOCK, yp, mode);
        break;
    case FILE_SOCKET_LOCAL_CLIENT:
        if (connect_socket_local_server(&fd, file) < 0) {
            ylog_error("open %s failed: %s\n", file, strerror(errno));
            if (fd >= 0)
                CLOSE(fd);
            return -1;
        }
        yp_set(NULL, fd, YLOG_POLL_INDEX_DATA, yp, mode);
        break;
    case FILE_POPEN:
        if (y->mode & YLOG_READ_MODE_BLOCK) {
            ylog_critical("popen2 %s, because its mode & YLOG_READ_MODE_BLOCK = 1\n", y->file);
            f = popen2(file, mode);
        } else {
            f = popen2(file, mode);
        }
        if (f == NULL) {
            ylog_error("open failed: %s\n", strerror(errno));
            return -1;
        }
        yp_set(f, -1, YLOG_POLL_INDEX_DATA, yp, NULL);
        break;
    case FILE_INOTIFY:
        if (create_inotify(&fd, &y->yinotify) == 0)
            yp_set(NULL, fd, YLOG_POLL_INDEX_INOTIFY, yp, mode);
        break;
    }

    return 0;
}

static int ylog_close_default(FILE *fp, struct ylog *y) {
    if (y->type == FILE_POPEN) {
        if (y->mode & YLOG_READ_MODE_BLOCK) {
            ylog_critical("pclose2 %s, because its mode & YLOG_READ_MODE_BLOCK = 1\n", y->file);
            return pclose2(fp);
        } else {
            //ylog_info("pclose %s\n", y->file);
            return pclose2(fp);
        }
    } else
        return fclose(fp);
}

static void *ylog_exit_default(struct ylog *y) {
    /* Free resources */
    int i;
    struct ylog_poll *yp = &y->yp;

    y->status &= ~YLOG_STARTED;
    for (i = 0; i < YLOG_POLL_INDEX_MAX; i++)
        yp_invalid(i, yp, y);

    free(y->buf);

    return y;
}

static int ylog_read_default_line(char *buf, int count, FILE *fp, int fd, struct ylog *y) {
    UNUSED(fd);
    UNUSED(y);
    /**
     * man fgets
     *
     * fgets()  reads  in  at  most one less than size characters from stream and
     * stores them into the buffer pointed to by s.
     * Reading stops after an EOF or a newline.  If a newline is read, it is stored into the buffer.
     * A terminating null  byte ('\0') is stored after the last character in the buffer.
     */
    if (fgets(buf, count, fp)) {
        return strlen(buf);
    } else {
        if (ferror(fp)) {
            return -1;
        } else {
            return 0;
        }
    }
}

static int ylog_read_default_binary(char *buf, int count, FILE *fp, int fd, struct ylog *y) {
    UNUSED(y);
    UNUSED(fp);
    return read(fd, buf, count);
}

static int ydst_open_default(char *file, char *mode, struct ydst *ydst) {
    if (file) {
        ydst->fp = fopen(file, mode);
        if (ydst->fp == NULL) {
            /* we need to output the log to stdout if the ydst file can't open properly */
            ydst->fp = stdout; // fopen("/dev/null", mode);
            ylog_error("critical errror -> ydst open %s failed: %s\n", file, strerror(errno));
        }
    } else {
        ydst->fp = stdout;
    }
    ydst->fd = fileno(ydst->fp);
    return 0;
}

static int ydst_close_default__write_data2cache_first(struct ydst *ydst) {
    if (ydst->fp) {
        /**
         * As for write_data2cache_first == 1
         * the data left in the current cacheline will be saved in the next segment
         * cacheline_thread_handler_default thread may be calling
         * cl->ydst->write_handler(), so ydst_close_default may be called
         * so ydst->cache->flush(ydst->cache) will never have chance to return
         * because cacheline_thread_handler_default thread is calling
         * cl->ydst->write_handler() now by luther 2016.03.16
         */
        if (ydst->fp != stdout)
            fclose(ydst->fp);
        ydst->fd = -1;
        ydst->fp = NULL;
        return 1;
    }
    return 0;
}

static int ydst_close_default(struct ydst *ydst) {
    if (ydst->fp) {
        struct cacheline *cl = ydst->cache;
        if (cl) {
            if (ydst->cache_locked) {
                ylog_debug("cacheline %s mutex is released for flush\n", cl->name);
                ydst->cache_locked = 0;
                pthread_mutex_unlock(&cl->mutex);
            }
            ydst->cache->flush(ydst->cache);
            ylog_debug("try to get cacheline %s mutex after flush command\n", cl->name);
            pthread_mutex_lock(&cl->mutex);
            ylog_debug("get the cacheline %s mutex after flush command\n", cl->name);
            ydst->cache_locked = 1;
            while (cl->writing) {
                ylog_info("cacheline %s is writing after flush command, wait 10ms\n", cl->name);
                usleep(10*1000);
            }
        }
        if (ydst->fp != stdout)
            fclose(ydst->fp);
        ydst->fd = -1;
        ydst->fp = NULL;
        return 1;
    }
    return 0;
}

static int ydst_flush_default__write_data2cache_first(struct ydst *ydst) {
    /**
     * when ydst->write_data2cache_first is set
     * we can't use any ydst related mutex lock in ylog thread
     * 1. pthread_mutex_lock(&ydst->mutex);
     * 2. pthread_mutex_lock(&cl->mutex);
     * it maybe cause deadlock in here
     * because in cacheline thread, the mutex lock sequence is
     * 1. pthread_mutex_lock(&cl->mutex);
     * 2. pthread_mutex_lock(&ydst->mutex); -> cl->ydst->write_handler()
     */
    return ydst->cache->flush(ydst->cache);
}

static int ydst_flush_default(struct ydst *ydst) {
    int ret = 0;
    int locked = 0;

    /* ydst needs mutex protect? */
    if (ydst->refs == 1) {
        /* only one ylog is using this ydst now */
        locked = 0;
    } else if (ydst->refs > 1) {
        /* more ylogs are using this ydst now */
        pthread_mutex_lock(&ydst->mutex);
        locked = 1;
    }

    if (ydst->cache) {
        ret = ydst->cache->flush(ydst->cache);
    } else if (ydst->fd > 0) {
        ret = fsync(ydst->fd);
        if (ret)
            ylog_error("fsync %s failed: %s\n", ydst->file, strerror(errno));
    }

    if (locked)
        pthread_mutex_unlock(&ydst->mutex);

    return ret;
}

static int ydst_write_default__write_data2cache_first(char *buf, int count, struct ydst *ydst) {
    return ydst->fwrite(buf, count, ydst->fd, ydst->file_name);
}

static int ydst_write_default(char *buf, int count, struct ydst *ydst) {
    if (ydst->cache)
        return ydst->cache->write(buf, count, ydst->cache);
    else
        return ydst->fwrite(buf, count, ydst->fd, ydst->file_name);
}

static int ydst_write_default_with_token__write_data2cache_first(char *id_token, int id_token_len,
        char *buf, int count, struct ydst *ydst) {
    UNUSED(id_token);
    UNUSED(id_token_len);
    return ydst_write_default__write_data2cache_first(buf, count, ydst);
}

static int ydst_write_default_with_token(char *id_token, int id_token_len,
        char *buf, int count, struct ydst *ydst) {
    int ret = 0;
    if (id_token_len)
        ret += ydst_write_default(id_token, id_token_len, ydst);
    if (count)
        ret += ydst_write_default(buf, count, ydst);
    return ret;
}

static int ylog_write_timestamp_default(struct ylog *y) {
    char timeBuf[32];
    struct ydst *ydst = y->ydst;
    return ydst->write(y->id_token, y->id_token_len, timeBuf, ylog_get_format_time(timeBuf), ydst);
}

static int ylog_write_header_default(struct ylog *y) {
    char buf[512];
    struct timespec ts;
    struct tm delta_tm;
    int count;
    struct ydst *ydst = y->ydst;
    long delta_speed_size;
    time_t delta_t, delta_speed_millisecond;
    char max_unit, segment_unit, unit, delta_speed_unit;
    float transfered_size_float, delta_speed_float, max_size_float, segment_size_float;

    gettimeofday(&ydst->tv, NULL);
    localtime_r(&ydst->tv.tv_sec, &ydst->tm);
    get_boottime(&ts);

    delta_speed_millisecond = diff_ts_millisecond(&ydst->ts, &ts);
    ydst->ts = ts;
    delta_speed_size = ydst->size - ydst->prev_size;
    ydst->prev_size = ydst->size;
    if (delta_speed_millisecond == 0)
        delta_speed_millisecond = 1000;
    delta_speed_float = ylog_get_unit_size_float_with_speed(delta_speed_size,
            &delta_speed_unit, delta_speed_millisecond);

    delta_t = ydst->ts.tv_sec - global_context->ts.tv_sec;
    gmtime_r(&delta_t, &delta_tm); /* UTC, don't support keep running more than 1 year by luther */

    transfered_size_float = ylog_get_unit_size_float(ydst->size, &unit);
    max_size_float = ylog_get_unit_size_float(ydst->max_size_now, &max_unit);
    segment_size_float = ylog_get_unit_size_float(ydst->max_segment_size_now, &segment_unit);

    count = snprintf(buf, sizeof(buf), "[ylog_segment=%ld/%d,%.2f%c] 20%02d.%02d.%02d %02d:%02d:%02d -%02dd%02d:%02d:%02d/%ldms %.2f%c/%.2f%c %.2f%c/s\n",
                ydst->segments,
                ydst->max_segment_now,
                segment_size_float,
                segment_unit,
                ydst->tm.tm_year % 100,
                ydst->tm.tm_mon + 1,
                ydst->tm.tm_mday,
                ydst->tm.tm_hour,
                ydst->tm.tm_min,
                ydst->tm.tm_sec,
                delta_tm.tm_yday,
                delta_tm.tm_hour,
                delta_tm.tm_min,
                delta_tm.tm_sec,
                delta_speed_millisecond,
                transfered_size_float,
                unit,
                max_size_float,
                max_unit,
                delta_speed_float,
                delta_speed_unit);
    return ydst->write(y->id_token, y->id_token_len, buf, count, ydst);
}

static int ylog_write_handler_default(char *buf, int count, struct ylog *y) {
    int written_count = 0;
    int locked = 0;
    struct filter_pattern *p = y->fp_array;
    struct ydst *ydst = y->ydst;
    /**
     * we don't support filter for ydst->write_data2cache_first 1
     */
    if (ydst->write_data2cache_first == 0 && buf != NULL) {
        int ignore = 0;
        if (y->bypass)
            return count;
        /**
         * Process filter, if pattern can be matched,
         * the relevant filter action will be called
         * buf buf should be got from fgets as one line
         */
        while (p) {
            if ((p->offset <= (count - p->key_string_len)) && \
                memcmp(buf + p->offset, p->key_string, p->key_string_len) == 0)
                if (p->filter(buf, p))
                    ignore = 1;
            p++;
        }
        if (y->fplugin_filter_log.func != NULL)
            ignore |= y->fplugin_filter_log.func(buf, count, NORMAL);
        if (ignore)
            return count;
    }
    /* ydst needs mutex protect? */
    if (ydst->refs == 1) {
        /* only one ylog is using this ydst now */
        locked = 0;
    } else if (ydst->refs > 1) {
        /* more ylogs are using this ydst now */
        pthread_mutex_lock(&ydst->mutex);
        locked = 1;
    } else {
        ylog_critical("BUG: %s %s refs %d is wrong\n", y->name, ydst->file, ydst->refs);
    }
    /* YLOG_MOVE_ROOT or YLOG_RESIZE_SEGMENT or YLOG_RESET */
    if (buf == NULL) {
        int mode = count;
        ydst->new_segment(y, mode);
        if (locked)
            pthread_mutex_unlock(&ydst->mutex);
        return count;
    }
    /* create new segment? */
    if (ydst->fd < 0 && ydst->new_segment(y, YDST_SEGMENT_MODE_NEW)) {
        ylog_critical("%s new_segment %s failed\n", y->name, ydst->file);
        if (locked)
            pthread_mutex_unlock(&ydst->mutex);
        return count;
    }
    /* append timestamp before the buf line */
    if (y->timestamp) {
        written_count += y->write_timestamp(y);
        written_count += ydst->write(NULL, 0, buf, count, ydst);
    } else {
        written_count += ydst->write(y->id_token, y->id_token_len, buf, count, ydst);
    }
    /**
     * Check the size, segment, and do something
     */
    if (written_count) {
        ydst->size += written_count;
        y->size += written_count;
        ydst->segment_size += written_count;
        if (ydst->segment_size >= ydst->max_segment_size_now) {
            /**
             * one segment is full, we need to generate a new one
             * and y->ydst->fd will be replaced with the new segment
             */
            if (ydst->new_segment(y, YDST_SEGMENT_MODE_NEW)) {
                ylog_critical("%s new_segment %s failed\n", y->name, ydst->file);
            }
        }
    } else {
        ylog_critical("%s write failed\n", y->name);
    }
    /* need to release mutex? */
    if (locked)
        pthread_mutex_unlock(&ydst->mutex);
    return written_count;
}

static int ylog_write_handler_default__write_data2cache_first(char *buf, int count, struct ylog *y) {
    struct ydst *ydst = y->ydst;

    if (buf == NULL)
        return y->ydst->write_handler(buf, count, y);
    /**
     * For cache first, there will be no timestamp, no id_token
     * and cache has pthread_mutex_lock(&cl->mutex)
     * to protect this ydst->cache->write action being atomic
     */
    return ydst->cache->write(buf, count, ydst->cache);
}

static int ylog_thread_new_state(enum ylog_thread_state state, struct ylog *y, int block) {
    unsigned long count;
    int retries = 1000;
    char *state_str;

    if (y->state == YLOG_EXIT) {
        ylog_critical("%s thread has exited\n", y->name);
        return -1;
    }

    switch (state) {
    case YLOG_RUN: state_str = "A\n"; break;
    case YLOG_SUSPEND: state_str = "B\n"; break;
    case YLOG_RESUME: state_str = "C\n"; break;
    case YLOG_STOP: state_str = "D\n"; break;
    case YLOG_EXIT: state_str = "E\n"; break;
    case YLOG_RESTART: state_str = "F\n"; break;
    case YLOG_MOVE_ROOT: state_str = "G\n"; break;
    case YLOG_RESIZE_SEGMENT: state_str = "H\n"; break;
    case YLOG_FLUSH: state_str = "I\n"; break;
    case YLOG_RESET: state_str = "J\n"; break;
    default: state_str = "Z\n"; break;
    }

    count = y->state_pipe_count;
    if (write(y->state_pipe[1], state_str, 2) != 2) {
        ylog_critical("write state %d failed\n", state);
        return -1;
    }

    if (block) {
        for (;count == y->state_pipe_count && retries--;)
            usleep(1000);

        if (retries == 0)
            ylog_critical("%s failed: state %d communicate timeout\n", y->name, state);
    }

    return retries == 0;
}

static int ylog_thread_run_default(struct ylog *y, int block) {
    return ylog_thread_new_state(YLOG_RUN, y, block);
}

static int ylog_thread_suspend_default(struct ylog *y, int block) {
    return ylog_thread_new_state(YLOG_SUSPEND, y, block);
}

static int ylog_thread_resume_default(struct ylog *y, int block) {
    return ylog_thread_new_state(YLOG_RESUME, y, block);
}

static int ylog_thread_stop_default(struct ylog *y, int block) {
    return ylog_thread_new_state(YLOG_STOP, y, block);
}

static int ylog_thread_exit_default(struct ylog *y, int block) {
    return ylog_thread_new_state(YLOG_EXIT, y, block);
}

static int ylog_thread_restart_default(struct ylog *y, int block) {
    return ylog_thread_new_state(YLOG_RESTART, y, block);
}

static int ylog_thread_move_root_default(struct ylog *y, int block) {
    return ylog_thread_new_state(YLOG_MOVE_ROOT, y, block);
}

static int ylog_thread_resize_segment_default(struct ylog *y, int block) {
    return ylog_thread_new_state(YLOG_RESIZE_SEGMENT, y, block);
}

static int ylog_thread_nop_default(struct ylog *y, int block) {
    return ylog_thread_new_state(YLOG_NOP, y, block);
}

static int ylog_thread_flush_default(struct ylog *y, int block) {
    return ylog_thread_new_state(YLOG_FLUSH, y, block);
}

static int ylog_thread_reset_default(struct ylog *y, int block) {
    return ylog_thread_new_state(YLOG_RESET, y, block);
}

static void ylog_trigger(struct ylog *ylog) {
    if (ylog->status & YLOG_DISABLED_MASK)
        return;
    ylog->thread_run(ylog, 1);
}

void ylog_trigger_and_wait_for_finish(struct ylog *ylog) {
    char *name = ylog->name;
    ylog_trigger(ylog);
    ylog_debug("ylog <%s> waiting for finish\n", name);
    while ((ylog->status & YLOG_DISABLED) != YLOG_DISABLED) {
        usleep(200 * 1000);
        ylog_debug("ylog <%s> waiting for finish\n", name);
    }
    ylog_debug("ylog <%s> is finished\n", name);
}

void ylog_trigger_all(struct ylog *ylog) {
    struct ylog *y;
    int i;
    for_each_ylog(i, y, ylog) {
        if (y->name == NULL)
            continue;
        ylog_trigger(y);
    }
    show_ydst_storage_info(NULL);
}

/**
 * Multi thread calling is not supported,
 * you must call and use it one by one, step by step
 */
static struct ylog *ylog_get_empty_slot(void) {
    struct ylog *ys;
    int i;
    for_each_ylog(i, ys, NULL) {
        if (ys->name == NULL) {
            ylog_debug("Success: Get one empty ylog %d\n", i);
            return ys;
        }
    }
    ylog_critical("Failed: Get ylog there is no space to store %d\n", i);
    return NULL;
}

static int ylog_insert(struct ylog *y) {
    struct ylog *ys;
    int i;
    if (y == NULL)
        return 0;
    for_each_ylog(i, ys, NULL) {
        if (ys->name == NULL) {
            *ys = *y;
            ylog_debug("Success: insert ylog %s\n", y->name);
            return i;
        }
    }
    ylog_critical("Failed: insert ylog %s, there is no space to store\n", y->name);
    return -1;
}

static int ylog_insert_all(struct ylog *y) {
    for (;y && y->name; y++)
        ylog_insert(y);
    return 0;
}

static int ydst_insert(struct ydst *ydi) {
    struct ydst *yd;
    int i;
    if (ydi == NULL)
        return 0;
    for_each_ydst(i, yd, NULL) {
        if (yd->file == NULL) {
            *yd = *ydi;
            ylog_debug("Success: insert ydst %s\n", yd->file);
            return i;
        }
    }
    ylog_critical("Failed: insert ydst %s, there is no space to store\n", ydi->file);
    return -1;
}

/**
 * Multi thread calling is not supported,
 * you must call and use it one by one, step by step
 */
static struct ydst *ydst_get_empty_slot(void) {
    struct ydst *yd;
    int i;
    for_each_ydst(i, yd, NULL) {
        if (yd->file == NULL) {
            ylog_debug("Success: Get one empty ydst, index is %d\n", i);
            return yd;
        }
    }
    ylog_critical("Failed: Get ydst there is no space to store %d\n", i);
    return NULL;
}

static int ydst_insert_all(struct ydst *yd) {
    for (;yd && yd->file; yd++)
        ydst_insert(yd);
    return 0;
}

static int ynode_insert(struct ynode *ynode) {
    struct ydst *ydst;
    struct ylog *ylog;
    int i, ret = 1;
    pthread_mutex_lock(&mutex);
    ydst = ydst_get_empty_slot();
    if (ydst) {
        if (ynode->ydst.file) {
            struct cacheline *cache = (ynode->cache.size && ynode->cache.num) ? calloc(sizeof(struct cacheline), 1) : NULL;
            if (cache)
                *cache = ynode->cache;
            *ydst = ynode->ydst;
            ylog_debug("Install ydst : %s\n", ydst->file);
            ydst->cache = cache;
        } else {
            ydst = NULL;
            ylog_debug("Empty ydst in this ynode\n");
        }
        ret = 0;
        for (i = 0; ynode->ylog[i].file; i++) {
            ylog = ylog_get_empty_slot();
            if (ylog == NULL) {
                ret |= 1;
                break;
            }
            *ylog = ynode->ylog[i];
            ylog->ydst = ydst;
            ylog_debug("Install ylog : %s\n", ylog->name);
        }
    }
    pthread_mutex_unlock(&mutex);
    return ret;
}

static int ynode_insert_all(struct ynode *ynode, int num) {
    int ret = 0;
    int i;
    for (i = 0; i < num; i++)
        ret |= ynode_insert(&ynode[i]);
    return ret;
}

static void ydst_refs_inc(struct ylog *y) {
    struct ydst *ydst = y->ydst;
    pthread_mutex_lock(&mutex);
    pthread_mutex_lock(&ydst->root->mutex);
    if (ydst->refs++ == 0) {
        if (y->ydst->write_data2cache_first) {
            /**
             * For write_data2cache_first, because cacheline thread and ylog_thread_handler_default
             * can use ydst at the same time, so we need to let cacheline as another vritual ylog by luther
             * cahceline is calling
             * 1. cl->ydst->write_handler(pcache, wpos, cl->ydst->ylog);
             * ylog_thread_handler_default here is processing pipe state change
             * 2. y->write_handler(NULL, XXXX, y);
             */
            ydst->refs++;
        }
        ydst->root->refs_cur = ++ydst->root->refs;
        ydst->root->max_size += ydst->max_size;
        ydst_pre_fill_zero_to_possession_storage_spaces(ydst, -1, NULL);
    }
    pthread_mutex_unlock(&ydst->root->mutex);
    pthread_mutex_unlock(&mutex);
}

static void ydst_refs_dec(struct ylog *y) {
    struct ydst *ydst = y->ydst;
    int locked = 1;
    pthread_mutex_lock(&mutex);
    pthread_mutex_lock(&ydst->root->mutex);
    if (y->ydst->write_data2cache_first) {
        /**
         * For write_data2cache_first, because cacheline thread and ylog_thread_handler_default
         * can use ydst at the same time, so we need to let cacheline as another vritual ylog by luther
         * cahceline is calling
         * 1. cl->ydst->write_handler(pcache, wpos, cl->ydst->ylog);
         * ylog_thread_handler_default here is processing pipe state change
         * 2. y->write_handler(NULL, XXXX, y);
         */
        if (ydst->refs)
            --ydst->refs;
    }
    if (ydst->refs && --ydst->refs == 0) {
        ydst->root->refs_cur = --ydst->root->refs;
        ydst->root->max_size -= ydst->max_size;
        if (ydst->cache) {
            if (ydst->write_data2cache_first) {
                /**
                 * when ydst->write_data2cache_first is set
                 * we can't use any ydst related mutex lock in ylog thread
                 * 1. pthread_mutex_lock(&ydst->mutex);
                 * 2. pthread_mutex_lock(&cl->mutex);
                 * it maybe cause deadlock in here
                 * because in cacheline thread, the mutex lock sequence is
                 * 1. pthread_mutex_lock(&cl->mutex);
                 * 2. pthread_mutex_lock(&ydst->mutex); -> cl->ydst->write_handler()
                 */
                locked = 0;
                pthread_mutex_unlock(&ydst->root->mutex);
                pthread_mutex_unlock(&mutex);
            }
            ydst->cache->exit(ydst->cache);
        }
    }
    if (locked) {
        pthread_mutex_unlock(&ydst->root->mutex);
        pthread_mutex_unlock(&mutex);
    }
}

static void *ylog_event_thread_handler_default(void *arg);
struct ylog_event_thread *yevent_thread_lists[YLOG_EVENT_THREAD_TYPE_MAX];

#define for_each_event_thread_start(e) { \
    int t; int i; \
    pthread_mutex_lock(&mutex); \
    for (t = 0; t < YLOG_EVENT_THREAD_TYPE_MAX; t++) { \
        for (e = yevent_thread_lists[t]; e; e = e->next) {

#define for_each_event_thread_end() \
        } \
    } \
    pthread_mutex_unlock(&mutex); \
}

static void ylog_event_thread_insert(enum ylog_event_thread_type type, struct ylog_event_thread *yevent_thread) {
    pthread_mutex_lock(&mutex);
    yevent_thread->next = yevent_thread_lists[type];
    yevent_thread_lists[type] = yevent_thread;
    pthread_mutex_unlock(&mutex);
}

static void ylog_event_thread_del(struct ylog_event_thread *yevent_thread) {
    struct ylog_event_thread *cur, *last;
    enum ylog_event_thread_type type = yevent_thread->type;
    pthread_mutex_lock(&mutex);
    for (last = NULL, cur = yevent_thread_lists[type]; cur; last = cur, cur = cur->next)
        if (cur == yevent_thread) {
            /* Remove the entry from the linked list. */
            if (last == NULL)
                yevent_thread_lists[type] = cur->next;
            else
                last->next = cur->next;
            break;
        }
    pthread_mutex_unlock(&mutex);
}

static void ylog_event_thread_notify_stop(struct ylog_event_thread *yevent_thread) {
    struct ylog_event_cond_wait *yewait = &yevent_thread->yewait;
    pthread_mutex_lock(&yewait->mutex);
    ylog_info("ylog_event notify %s stop\n", yevent_thread->yewait.name);
    yevent_thread->state = YLOG_STOP;
    pthread_cond_broadcast(&yevent_thread->yewait.cond);
    pthread_mutex_unlock(&yewait->mutex);
}

void ylog_event_thread_notify_all_stop(struct ylog_event_thread *lists) {
    struct ylog_event_thread *yevent_thread;
    for (yevent_thread = lists; yevent_thread; yevent_thread = yevent_thread->next)
        ylog_event_thread_notify_stop(yevent_thread);
}

void ylog_event_thread_notify_all_stop_type(void) {
    int i;
    for (i = 0; i < YLOG_EVENT_THREAD_TYPE_MAX; i++)
        ylog_event_thread_notify_all_stop(yevent_thread_lists[i]);
}

static void ylog_event_thread_notify_run(struct ylog_event_thread *yevent_thread) {
    struct ylog_event_cond_wait *yewait = &yevent_thread->yewait;
    pthread_mutex_lock(&yewait->mutex);
    ylog_info("ylog_event notify %s run\n", yevent_thread->yewait.name);
    yevent_thread->state = YLOG_RUN;
    pthread_cond_broadcast(&yevent_thread->yewait.cond);
    pthread_mutex_unlock(&yewait->mutex);
}

void ylog_event_thread_notify_all_run(struct ylog_event_thread *lists) {
    struct ylog_event_thread *yevent_thread;
    for (yevent_thread = lists; yevent_thread; yevent_thread = yevent_thread->next)
        ylog_event_thread_notify_run(yevent_thread);
}

void ylog_event_thread_notify_all_run_type(void) {
    int i;
    for (i = 0; i < YLOG_EVENT_THREAD_TYPE_MAX; i++)
        ylog_event_thread_notify_all_run(yevent_thread_lists[i]);
}

static int ylog_event_cond_wait_init(char *name, int period, struct ylog_event_cond_wait *yewait) {
    pthread_mutex_init(&yewait->mutex, NULL);
    yewait->name = name;
    yewait->period = period;
    pthread_cond_timedwait_monotonic_init(&yewait->cond, &yewait->condattr);
    return 0;
}

static int ylog_event_handler_create(char *name, int period,
        ylog_event_timer_handler event_timer_handler, void *arg, enum ylog_event_thread_type type) {
    struct ylog_event_thread *yevent_thread;

    yevent_thread = calloc(1, sizeof(*yevent_thread));
    if (yevent_thread == NULL) {
        ylog_error("Failed to calloc memory %s\n", strerror(errno));
        return -1;
    }

    yevent_thread->event_handler = event_timer_handler;
    yevent_thread->arg = arg;
    yevent_thread->type = type;

    ylog_event_cond_wait_init(name, period, &yevent_thread->yewait);
    ylog_event_thread_insert(type, yevent_thread);

    if (pthread_create(&yevent_thread->ptid, NULL, ylog_event_thread_handler_default, yevent_thread)) {
        ylog_error("Failed to pthread_create %s\n", strerror(errno));
        ylog_event_thread_del(yevent_thread);
        free(yevent_thread);
        return -1;
    }

    return 0;
}

static void *ylog_event_thread_handler_default(void *arg) {
    struct ylog_event_thread *yevent_thread = arg;
    struct ylog_event_cond_wait *yewait = &yevent_thread->yewait;
    long tick = 0;
    int ret;
    enum ylog_thread_state *state = &yevent_thread->state;
    int wait_period = yewait->period;
    yevent_thread->pid = getpid();
    yevent_thread->tid = gettid();
    pthread_mutex_lock(&yewait->mutex);
    ylog_info("event %s --> %dms is started, pid=%d, tid=%d\n",
            yewait->name, wait_period, yevent_thread->pid, yevent_thread->tid);
    for (;;) {
        if (*state == YLOG_RUN) {
            ret = pthread_cond_timedwait_monotonic2(wait_period, &yewait->cond, &yewait->mutex, &yewait->ts);
            if (ret == ETIMEDOUT) {
                tick++;
                wait_period = yewait->period;
            } else {
                struct timespec ts;
                clock_gettime(CLOCK_MONOTONIC, &ts);
                wait_period = diff_ts_millisecond(&ts, &yewait->ts);
                ylog_debug("ylog_event_cond_wait %s get singal, left time %dms\n", yewait->name, wait_period);
                if (wait_period <= 0) {
                    tick++;
                    wait_period = yewait->period;
                }
                /* yewait->handler(yevent_thread); */
                continue;
            }
            pthread_mutex_unlock(&yewait->mutex);
            // ylog_debug("ylog_event_handler %s event_handler enter\n", yewait->name);
            if (yevent_thread->event_handler(yevent_thread->arg, tick, yewait))
                break;
            // ylog_debug("ylog_event_handler %s event_handler exit\n", yewait->name);
            pthread_mutex_lock(&yewait->mutex);
        } else {
            ylog_debug("ylog_event_handler %s is not YLOG_RUN, pending\n", yewait->name);
            pthread_cond_wait(&yewait->cond, &yewait->mutex);
        }
    }
    ylog_critical("%s event_handler exit\n", yewait->name);
    ylog_event_thread_del(yevent_thread);
    free(yevent_thread);
    return NULL;
}

int ylog_os_event_timer_create(char *name, int period, ylog_event_timer_handler event_timer_handler, void *arg) {
    return ylog_event_handler_create(name, period,
                event_timer_handler, arg, YLOG_EVENT_THREAD_TYPE_OS_TIMER);
}

static void ylog_load_filter_plugin_func(char *path, char *fun, struct sfilter_plugin *sfp) {
    ylog_filter_plugin_func func = NULL;
    void *handle;
    const char *error;
    if (path == NULL)
        return;
    handle = dlopen(path, RTLD_LAZY);
    if (!handle) {
        ylog_debug("ylog open %s failed: %s\n", path, dlerror());
        return;
    }
    /* clear error before */
    dlerror();
    func = dlsym(handle, fun);
    if ((error = dlerror()) != NULL) {
        ylog_error("%s \n",error);
        dlclose(handle);
        return;
    }
    sfp->handle = handle;
    sfp->func = func;
}

static void ylog_load_filter_plugin(char *in_name, char *fun, char *prefix, struct sfilter_plugin *sfp) {
    char out_libfilter[PATH_MAX];
    if (in_name == NULL || fun == NULL)
        return;
    sprintf(out_libfilter, "%slibfilter_%s.so", prefix, in_name);
    ylog_load_filter_plugin_func(out_libfilter, fun, sfp);
}

static void *ylog_thread_handler_default(void *arg) {
    struct ylog *y = arg;
    char *file = y->file;
    struct timespec ts;
    int timeout = -1;
    enum ylog_thread_state state_timeout = YLOG_NOP;
    unsigned long long ms_prev, ms_curr;
    unsigned long gap;
    char *mode = "r";
    int count;
    enum ylog_thread_state state_curr;
    struct ylog_poll *yp = &y->yp;
    char *name = y->name;
    int nowrap;
    ydst_refs_inc(y);
    y->pid = getpid();
    y->tid = gettid();
    get_boottime(&y->ts);
    ms_prev = currentTimeMillis();
    y->status |= YLOG_STARTED;
    ylog_warn("Start ylog thread <%s, %s, file type is %d> --> %s %d started, pid=%d, tid=%d\n",
            name, y->file, y->type, y->ydst->file, y->ydst->refs, y->pid, y->tid);
    for (;;) {
        /* Step 1: waiting for control, client, inotify or data by luther */
        if (poll(yp->pfd, YLOG_POLL_INDEX_MAX, timeout) < 0) {
            ylog_error("%s poll failed: %d %s\n", name, timeout, strerror(errno));
            /* continue; */
            y->state = state_curr = YLOG_RESTART;
            sleep(1);
            goto __state_control;
        }
        /* Step 2: control channel has data? */
        if (yp_isset(YLOG_POLL_INDEX_PIPE, yp)) {
#if 0
            char buf[3];
            count = y->fread(buf, sizeof(buf), yp_fp(YLOG_POLL_INDEX_PIPE, yp), yp_fd(YLOG_POLL_INDEX_PIPE, yp), y);
#else
            char buf[2];
            count = y->read(buf, sizeof(buf), yp_fp(YLOG_POLL_INDEX_PIPE, yp), yp_fd(YLOG_POLL_INDEX_PIPE, yp), y);
#endif
            ylog_debug("%s control state read %d, %s\n", name, count, buf);
            switch (buf[0]) {
            case 'A': state_curr = YLOG_RUN; break;
            case 'B': state_curr = YLOG_SUSPEND; break;
            case 'C': state_curr = YLOG_RESUME; break;
            case 'D': state_curr = YLOG_STOP; break;
            case 'E': state_curr = YLOG_EXIT; break;
            case 'F': state_curr = YLOG_RESTART; break;
            case 'G': state_curr = YLOG_MOVE_ROOT; break;
            case 'H': state_curr = YLOG_RESIZE_SEGMENT; break;
            case 'I': state_curr = YLOG_FLUSH; break;
            case 'J': state_curr = YLOG_RESET; break;
            default: state_curr = YLOG_NOP; break;
            }
            y->state = state_curr;
            ylog_debug("%s control state to %d, count from %d to %d\n",
                    name, state_curr, y->state_pipe_count, y->state_pipe_count+1);
__state_control:
            switch (state_curr) {
            case YLOG_RUN:
                nowrap = ydst_nowrap_segment(y->ydst);
                if (yp_isclosed(YLOG_POLL_INDEX_DATA, yp) && nowrap == 0) {
                    if (y->open(file, mode, y)) {
                        ylog_critical("open %s failed\n", file);
                        y->state = YLOG_STOP;
                        pthread_mutex_lock(&mutex);
                        y->status |= YLOG_DISABLED;
                        pthread_mutex_unlock(&mutex);
                        if (os_hooks.ylog_status_hook)
                            os_hooks.ylog_status_hook(YLOG_STOP, y);
                        if (y->mode & YLOG_READ_MODE_BLOCK_RESTART_ALWAYS) {
                            y->state = YLOG_RESTART;
                            if (y->restart_period) {
                                timeout = y->restart_period;
                            } else {
                                timeout = 1*1000; /* try to recovery 1 time */
                            }
                            ms_prev = currentTimeMillis();
                            state_timeout = YLOG_RESTART;
                            //ylog_info("%s block will restart in %dms\n", file, timeout);
                        }
                    } else {
                        pthread_mutex_lock(&mutex);
                        y->status &= ~YLOG_DISABLED;
                        pthread_mutex_unlock(&mutex);
                        if (os_hooks.ylog_status_hook)
                            os_hooks.ylog_status_hook(YLOG_RUN, y);
                    }
                } else {
                    if (nowrap == 0)
                        ylog_critical("%s %s is opened, no need to re-open again\n", name, file);
                    else
                        ylog_critical("%s %s is forbidden open, ydst has reached max size\n", name, file);
                }
                y->state_pipe_count++;
                continue;
            case YLOG_SUSPEND:
                yp_clr(YLOG_POLL_INDEX_DATA, yp);
                y->state_pipe_count++;
                continue;
            case YLOG_RESUME:
                yp_reassign(YLOG_POLL_INDEX_DATA, yp);
                y->state = YLOG_RUN;
                y->state_pipe_count++;
                continue;
            case YLOG_STOP:
            case YLOG_RESTART:
                if (os_hooks.ylog_status_hook)
                    os_hooks.ylog_status_hook(YLOG_STOP, y);
                yp_invalid(YLOG_POLL_INDEX_DATA, yp, y);
                pthread_mutex_lock(&mutex);
                timeout = -1;
                if (y->status & YLOG_DISABLED_FORCED) {
                    if ((y->status & YLOG_DISABLED) == 0)
                        y->status |= YLOG_DISABLED_FORCED_RUNNING;
                }
                if (state_curr == YLOG_RESTART) {
                    if ((y->status & YLOG_DISABLED_FORCED) == 0) {
                        y->state = state_curr = YLOG_RUN;
                        pthread_mutex_unlock(&mutex);
                        goto __state_control;
                    } else {
                        ylog_info("ylog <%s> is disabled forcely by ylog_cli\n", name);
                    }
                }
                y->status |= YLOG_DISABLED;
                pthread_mutex_unlock(&mutex);
                y->state_pipe_count++;
                continue;
            case YLOG_MOVE_ROOT:
                y->write_handler(NULL, YDST_SEGMENT_MODE_UPDATE, y);
                y->state_pipe_count++;
                continue;
            case YLOG_RESIZE_SEGMENT:
                y->write_handler(NULL, YDST_SEGMENT_MODE_UPDATE, y);
                y->state_pipe_count++;
                continue;
            case YLOG_FLUSH:
                y->ydst->flush(y->ydst);
                y->state_pipe_count++;
                continue;
            case YLOG_RESET:
                y->write_handler(NULL, YDST_SEGMENT_MODE_RESET, y);
                y->state_pipe_count++;
                continue;
            case YLOG_EXIT:
                if (os_hooks.ylog_status_hook)
                    os_hooks.ylog_status_hook(YLOG_STOP, y);
                ylog_critical("%s %s exit\n", name, file);
                y->state_pipe_count++;
                goto __exit;
            default:
                y->state_pipe_count++;
                ylog_debug("%s %s state NOP\n", name, file);
                continue;
            }
        }
        /* Step 3: client try to connect server sock? */
        if (yp_isset(YLOG_POLL_INDEX_SERVER_SOCK, yp)) {
            int fd_client;
            if ((fd_client = accept_client(yp_fd(YLOG_POLL_INDEX_SERVER_SOCK, yp))) < 0) {
                ylog_error("%s %s server sock accept failed: %s\n", name, file, strerror(errno));
            } else {
                if (yp_fd(YLOG_POLL_INDEX_DATA, yp) < 0) {
                    ylog_debug("%s %s accept this client\n", name, file);
                    yp_set(NULL, fd_client, YLOG_POLL_INDEX_DATA, yp, mode);
                } else {
                    ylog_warn("%s %s server had accepted one client, forbid you\n", name, file);
                    CLOSE(fd_client);
                }
            }
        }
        /* Step 4: inotify detects action? */
        if (yp_isset(YLOG_POLL_INDEX_INOTIFY, yp)) {
            int tmp;
            tmp = ylog_inotfiy_file_handler(y);
            if (tmp >= 0 && ((timeout < 0) || tmp < timeout)) {
                state_timeout = YLOG_NOP;
                ms_prev = currentTimeMillis();
                timeout = tmp;
            }
        }
        /* Step 5: check timeout */
        if (timeout >= 0) {
            ms_curr = currentTimeMillis();
            gap = get_gap(ms_prev, ms_curr);
            if ((int)gap < timeout) {
                ms_prev = ms_curr;
                timeout -= gap;
                if (y->type == FILE_INOTIFY) {
                    int tmp = ylog_inotfiy_file_handler_timeout(y);
                    if (tmp > 0 && tmp < timeout)
                        timeout = tmp;
                }
                continue;
            } else {
                if (y->type == FILE_INOTIFY)
                    timeout = ylog_inotfiy_file_handler_timeout(y);
                else
                    timeout = -1;
                if (timeout >= 0) {
                    /* we still have pending node */
                    state_timeout = YLOG_NOP;
                    ms_prev = currentTimeMillis();
                } else {
                    y->state = state_curr = state_timeout;
                    state_timeout = YLOG_NOP;
                    goto __state_control;
                }
            }
        }
        /* Step 6: read data and save them */
        if (yp_isset(YLOG_POLL_INDEX_DATA, yp)) {
            count = y->fread(y->buf, y->buf_size, yp_fp(YLOG_POLL_INDEX_DATA, yp), yp_fd(YLOG_POLL_INDEX_DATA, yp), y);
            if (count > 0) {
                if (count != INT_MAX) /* INT_MAX to tell here, y->fread has called y->write_handler in itself */
                    y->write_handler(y->buf, count, y);
            } else {
                if (y->mode & YLOG_READ_MODE_BLOCK) {
                    int go_to_stop = 1;
                    yp_clr(YLOG_POLL_INDEX_DATA, yp);
                    if (count < 0) {
                        ylog_error("%s block read %s failed: %s\n", name, file, strerror(errno));
                        // if (y->mode & YLOG_READ_MODE_BINARY == 0) clearerr(yp_fp(YLOG_POLL_INDEX_DATA, yp));
                        if (y->type != FILE_SOCKET_LOCAL_SERVER) {
                            go_to_stop = 0;
                            if (y->restart_period) {
                                timeout = y->restart_period;
                            } else {
                                timeout = 1*1000; /* try to recovery 1 time */
                            }
                            ms_prev = currentTimeMillis();
                            state_timeout = YLOG_RESTART;
                            //ylog_info("%s block will restart in %dms\n", file, timeout);
                        }
                    }
                    if (go_to_stop) {
                        if (y->mode & YLOG_READ_LEN_MIGHT_ZERO) {
                            ylog_critical("%s block read %s return 0, read_len_zero_count=%d\n",
                                    name, file, ++y->read_len_zero_count);
                            if (y->restart_period) {
                                timeout = y->restart_period;
                            } else {
                                timeout = 1*1000; /* try to recovery 1 time */
                            }
                            ms_prev = currentTimeMillis();
                            state_timeout = YLOG_RESTART;
                            #if 0
                            y->state = state_curr = YLOG_RESTART;
                            goto __state_control;
                            #endif
                        } else {
                            ylog_critical("%s block read %s return 0, thread_stop\n", name, file);
                            /* y->thread_stop(s, 0); may cause pipe dead lock by luther */
                            y->state = state_curr = YLOG_STOP;
                            goto __state_control;
                        }
                    }
                } else {
                    if (y->restart_period) {
                        timeout = y->restart_period;
                        ms_prev = currentTimeMillis();
                        state_timeout = YLOG_RESTART;
                        yp_clr(YLOG_POLL_INDEX_DATA, yp);
                        //ylog_info("%s will restart in %dms\n", file, timeout);
                    } else {
                        if (count < 0) {
                            ylog_error("%s read %s failed: %s\n", name, file, strerror(errno));
                             // if (y->mode & YLOG_READ_MODE_BINARY == 0) clearerr(yp_fp(YLOG_POLL_INDEX_DATA, yp));
                        } else {
                            ylog_debug("%s read %s return %d, thread_stop\n", name, file, count);
                        }
                        /* y->thread_stop(s, 0); may cause pip dead lock by luther */
                        y->state = state_curr = YLOG_STOP;
                        goto __state_control;
                    }
                }
            }
        }
    }
__exit:
    ydst_refs_dec(y);
    return y->exit(y);
}

static void ylog_init(struct ydst_root *root, struct context *c) {
    struct ylog *y;
    struct ylog_poll *yp;
    struct ydst *yd;
    int i;
    int ydst_change_seq_init = 0;
    int all_thread_started;

    if (root == NULL)
        root = global_ydst_root;
    if (root->root == NULL)
        root->root = strdup("ylog");
    root->ydst_change_seq = root->ydst_change_seq_move_root = \
    root->ydst_change_seq_resize_segment = ydst_change_seq_init;
    pthread_mutex_init(&root->mutex, NULL);
    if (c == NULL)
        c = global_context;
    gettimeofday(&c->tv, NULL);
    localtime_r(&c->tv.tv_sec, &c->tm);
    get_boottime(&c->ts);
    ylog_get_format_time(c->timeBuf);
    ylog_historical_folder(root->root, c);

    for_each_ydst(i, yd, NULL) {
        if (yd->file == NULL)
            continue;
        if (yd->file_name == NULL)
            yd->file_name = "ylog_all";
        ylog_info("ydst <%s> is initialized\n", yd->file);
        pthread_mutex_init(&yd->mutex, NULL);
        yd->refs = 0;
        yd->fp = NULL;
        yd->fd = -1;
        yd->ydst_change_seq_move_root = yd->ydst_change_seq_resize_segment = ydst_change_seq_init;
        gettimeofday(&yd->tv, NULL);
        localtime_r(&yd->tv.tv_sec, &yd->tm);
        get_boottime(&yd->ts);
        yd->prev_size = yd->size = 0;
        if (yd->root == NULL)
            yd->root = root;
        if (yd->max_segment == 0)
            yd->max_segment = 1;
        if (yd->new_segment == NULL)
            yd->new_segment = ydst_new_segment_default;
        if (yd->mode == NULL)
            yd->mode = "w+";
        if (yd->write == NULL)
            yd->write = yd->write_data2cache_first ? \
                        ydst_write_default_with_token__write_data2cache_first:ydst_write_default_with_token;
        if (yd->fwrite == NULL)
            yd->fwrite = fd_write;
        if (yd->flush == NULL)
            yd->flush = yd->write_data2cache_first ? \
                        ydst_flush_default__write_data2cache_first:ydst_flush_default;
        if (yd->open == NULL)
            yd->open = ydst_open_default;
        if (yd->close == NULL)
            yd->close = yd->write_data2cache_first ? \
                        ydst_close_default__write_data2cache_first:ydst_close_default;
        if (yd->cache) {
            struct cacheline *cache = yd->cache;
            cache->ydst = yd;
            if (cache->name == NULL)
                cache->name = yd->file;
            cache->wclidx = cache->rclidx = 0;
            cache->wpos = 0;
            cache->status = CACHELINE_RUN;
            if (cache->size == 0)
                cache->size = 512 * 1024;
            if (cache->num == 0)
                cache->num = 4;
            if (cache->write == NULL)
                cache->write = cacheline_write_default;
            if (cache->update_timeout == NULL)
                cache->update_timeout = cacheline_update_timeout_default;
            if (cache->flush == NULL)
                cache->flush = cacheline_flush_default;
            if (cache->exit == NULL)
                cache->exit = cacheline_exit_default;
            if (cache->handler == NULL)
                cache->handler = cacheline_thread_handler_default;
            pthread_mutex_init(&cache->mutex, NULL);
            pthread_cond_timedwait_monotonic_init(&cache->cond, &cache->condattr);
            cache->cache = malloc(cache->size * (cache->num + 1));
            if (cache->cache == NULL) {
                ylog_error("ydst malloc %ld failed: %s\n", cache->size * cache->num, strerror(errno));
                exit(0);
            }
            pthread_mutex_lock(&cache->mutex);
            pthread_create(&cache->ptid, NULL, cache->handler, cache);
        }

        yd->max_size = yd->max_segment_size * yd->max_segment;
        yd->root_folder = yd->root->root;

        yd->max_size_now = yd->max_size;
        yd->max_segment_size_new = yd->max_segment_size_now = yd->max_segment_size;
        yd->max_segment_new = yd->max_segment_now = yd->max_segment;
    }

    for_each_ylog(i, y, NULL) {
        if (y->name == NULL)
            continue;
        ylog_info("ylog <%s> is initialized\n", y->name);
        yp = &y->yp;
        if (y->ydst == NULL)
            y->ydst = &global_ydst[YDST_TYPE_DEFAULT];
        if (y->write_handler == NULL) {
            if (y->ydst->write_data2cache_first && y->ydst->cache == NULL) {
                ylog_critical("Fatal: ylog -> cache -> ydst<%s>, cache is null, forced it write_data2cache_first to 0\n",
                        y->ydst->file);
                y->ydst->write_data2cache_first = 0;
            }
            if (y->ydst->write_data2cache_first == 0)
                y->write_handler = ylog_write_handler_default;
            else {
                if (y->ydst->write_handler || y->ydst->ylog) {
                    ylog_critical("Fatal: ylog -> cache -> ydst<%s> should only be used by one ylog %s\n",
                            y->ydst->file, y->name);
                }
                y->write_handler = ylog_write_handler_default__write_data2cache_first;
                y->ydst->write_handler = ylog_write_handler_default;
                y->ydst->ylog = y;
            }
        }
        if (y->open == NULL)
            y->open = ylog_open_default;
        if (y->fread == NULL) {
            if (y->mode & YLOG_READ_MODE_BINARY) {
                y->fread = ylog_read_default_binary;
            } else {
                y->fread = ylog_read_default_line;
            }
        }
        if (y->read == NULL)
            y->read = ylog_read_default_binary;
        if (y->close == NULL)
            y->close = ylog_close_default;
        if (y->exit == NULL)
            y->exit = ylog_exit_default;
        if (y->write_header == NULL)
            y->write_header = ylog_write_header_default;
        if (y->write_timestamp == NULL)
            y->write_timestamp = ylog_write_timestamp_default;
        if (y->thread_handler == NULL)
            y->thread_handler = ylog_thread_handler_default;
        if (y->thread_run == NULL)
            y->thread_run = ylog_thread_run_default;
        if (y->thread_suspend == NULL)
            y->thread_suspend = ylog_thread_suspend_default;
        if (y->thread_resume == NULL)
            y->thread_resume = ylog_thread_resume_default;
        if (y->thread_stop == NULL)
            y->thread_stop = ylog_thread_stop_default;
        if (y->thread_exit == NULL)
            y->thread_exit = ylog_thread_exit_default;
        if (y->thread_restart == NULL)
            y->thread_restart = ylog_thread_restart_default;
        if (y->thread_move_root == NULL)
            y->thread_move_root = ylog_thread_move_root_default;
        if (y->thread_resize_segment == NULL)
            y->thread_resize_segment = ylog_thread_resize_segment_default;
        if (y->thread_flush == NULL)
            y->thread_flush = ylog_thread_flush_default;
        if (y->thread_reset == NULL)
            y->thread_reset = ylog_thread_reset_default;
        if (y->thread_nop == NULL)
            y->thread_nop = ylog_thread_nop_default;
        if (y->fplugin_filter_log.func == NULL)
            ylog_load_filter_plugin(y->name, "filter_log", c->filter_plugin_path, &y->fplugin_filter_log);

        y->state = YLOG_STOP;
        yp_invalid(YLOG_POLL_INDEX_INOTIFY, yp, y);
        yp_invalid(YLOG_POLL_INDEX_SERVER_SOCK, yp, y);
        yp_invalid(YLOG_POLL_INDEX_DATA, yp, y);

        if (y->id_token && y->id_token_filename == NULL)
            y->id_token_filename = y->name;

        y->size = 0;
        if (y->buf_size == 0)
            y->buf_size = 4096;
        y->buf = malloc(y->buf_size);
        if (y->buf == NULL) {
            ylog_error("malloc %ld failed: %s\n", y->buf_size, strerror(errno));
            exit(0);
        }

        if (pipe(y->state_pipe))
            ylog_error("create pipe %s failed: %s\n", y->name, strerror(errno));
        yp_set(NULL, y->state_pipe[0], YLOG_POLL_INDEX_PIPE, yp, "r");

        if (pthread_create(&y->ptid, NULL, y->thread_handler, y))
            ylog_error("create thread %s failed: %s\n", y->name, strerror(errno));
    }

    /* waiting for all created thread start */
    do {
        ylog_info("waiting for all ylog thread ready ...\n");
        all_thread_started = 1;
        for_each_ylog(i, y, NULL) {
            if (y->name == NULL)
                continue;
            if ((y->status & YLOG_STARTED) == 0) {
                all_thread_started = 0;
                usleep(1*1000);
                break;
            }
        }
    } while (!all_thread_started);
    root->quota_now = root->max_size;
    ylog_info("all ylog thread are ready!\n");
}
