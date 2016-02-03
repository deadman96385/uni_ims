/**
 * Copyright (C) 2016 Spreadtrum Communications Inc.
 */

#include <sys/inotify.h>

static int create_inotify(int *fd, struct ylog_inotify *yi) {
    int i;
    struct ylog_inotify_cell *pcell;

    *fd = inotify_init();
    if (*fd < 0) {
        ylog_error("inotify_init failed, %s\n", strerror(errno));
        return -1;
    }

    for (i = 0; i < YLOG_INOTIFY_MAX; i++) {
        pcell = &yi->cells[i];
        if (pcell->pathname) {
            if (pcell->filename)
                pcell->type |= YLOG_INOTIFY_TYPE_WATCH_FILE_FOLDER;
            else
                pcell->type |= YLOG_INOTIFY_TYPE_WATCH_FOLDER;
        }
        if (pcell->type & (YLOG_INOTIFY_TYPE_WATCH_FILE_FOLDER | YLOG_INOTIFY_TYPE_WATCH_FOLDER))
            pcell->wd = inotify_add_watch(*fd, pcell->pathname, pcell->mask); /* watch the folder */
        else
            pcell->wd = inotify_add_watch(*fd, pcell->filename, pcell->mask); /* watch the file */
        get_boottime(&pcell->ts);
    }

    return 0;
}

static int ylog_inotfiy_file_handler_timeout(struct ylog *y) {
    int timeout = -1;
    long elapsed, left;
    int i;
    struct ylog_inotify_cell *pcell;
    struct ylog_inotify *yi = &y->yinotify;

    for (i = 0; i < YLOG_INOTIFY_MAX; i++) {
        pcell = &yi->cells[i];
        if (pcell->status & YLOG_INOTIFY_WAITING_TIMEOUT) {
            struct timespec ts;
            get_boottime(&ts);
            elapsed = diff_ts_millisecond(&pcell->ts, &ts);
            if (elapsed >= pcell->timeout) {
                pcell->status &= ~YLOG_INOTIFY_WAITING_TIMEOUT;
                ylog_info("ylog inotify '%s %s' %dms timeout now\n",
                        pcell->pathname ? pcell->pathname:"", pcell->filename ? pcell->filename:"", pcell->timeout);
                if (pcell->handler)
                    pcell->handler(pcell, 1, y); /* 1 for timeout */
                left = elapsed = -1;
            } else {
                left = pcell->timeout - elapsed;
            }
            if (left > 0) { /* generate next timeout value */
                if (timeout < 0 || timeout > left)
                    timeout = left;
            }
        }
    }

    return timeout;
}

static int ylog_inotfiy_file_handler(struct ylog *y) {
    struct inotify_event *event;
    char *event_buf;
    int count;
    int event_pos = 0;
    int timeout = -1;
    int ret;
    int i;
    struct ylog_inotify_cell *pcell;
    struct ylog_inotify *yi = &y->yinotify;

    count = read(yp_fd(YLOG_POLL_INDEX_INOTIFY, &y->yp), y->buf, y->buf_size);

    if (count < (int)sizeof(*event)) {
        if(errno == EINTR)
            return timeout;
        ylog_error("ylog inotify could not get event, %s\n", strerror(errno));
        return timeout;
    }

    event_buf = y->buf;

    while (count >= (int)sizeof(*event)) {
        int event_size;
        event = (struct inotify_event *)(event_buf + event_pos);

        ylog_debug("ylog inotify event : 0x%08x %s\n", event->mask, event->len ? event->name : "");

        for (i = 0; i < YLOG_INOTIFY_MAX; i++) {
            pcell = &yi->cells[i];
            if (pcell->wd == event->wd) {
                int found = 0;
                int match = 0;
                if (pcell->type & YLOG_INOTIFY_TYPE_WATCH_FILE_FOLDER) {
                    if (event->len && !strcmp(pcell->filename, event->name))
                        match = 1;
                } else if (pcell->type & YLOG_INOTIFY_TYPE_WATCH_FOLDER) {
                    match = 1;
                } else {
                    match = 1;
                }
                if (match) {
                    if ((pcell->type & YLOG_INOTIFY_TYPE_MASK_EQUAL) &&
                        (pcell->mask == event->mask))
                        found = 1;
                    if ((pcell->type & YLOG_INOTIFY_TYPE_MASK_SUBSET_BIT) &&
                        ((pcell->mask & event->mask) == pcell->mask))
                        found = 1;
                }
                if (found) {
                    if (pcell->type & YLOG_INOTIFY_TYPE_WATCH_FOLDER) {
                        if (event->len) {
                            if (pcell->args) {
                                struct ylog_inotify_cell_args *pcella = pcell->args;
                                if (pcella->type & YLOG_INOTIFY_CELL_TYPE_STORE_FILES) {
                                    int i;
                                    char *empty = NULL;
                                    char *fempty = NULL;
                                    int found = 0;
                                    struct ylog_inotify_files *file = &pcella->file;
                                    char *a;
                                    char *prefix = pcella->prefix;
                                    char *suffix = pcella->suffix;
                                    int prefix_len = prefix ? strlen(prefix):0;
                                    for (i = 0; i < file->num; i++) {
                                        a = file->files_array + i * file->len;
                                        if (a[0] && strncmp(&a[prefix_len], event->name, event->len) == 0) {
                                            found = 1;
                                            fempty = a;
                                        }
                                        if (empty == NULL && a[0] == 0)
                                            empty = a;
                                    }
                                    if (empty) {
                                        if (found == 0) {
                                            snprintf(empty, file->len, "%s%s%s",
                                                    prefix ? prefix:"", event->name, suffix ? suffix:"");
                                            ylog_debug("%s insert %s\n", pcella->name, empty);
                                        } else {
                                            ylog_debug("%s already has %s\n", pcella->name, fempty);
                                        }
                                    } else {
                                        ylog_info("ylog_inotify_cell_args %s is full\n", pcella->name);
                                        for (i = 0; i < file->num; i++) {
                                            a = file->files_array + i * file->len;
                                            ylog_info("%s -> %s\n", pcella->name, a);
                                        }
                                    }
                                }
                            }
                        }
                    }
                    if (pcell->timeout > 0) {
                        get_boottime(&pcell->ts);
                        ret = pcell->timeout; /* remove dithering, wait for dither timeout happen */
                        pcell->status |= YLOG_INOTIFY_WAITING_TIMEOUT;
                        ylog_debug("ylog inotify '%s %s'' waiting %dms timeout\n",
                                pcell->pathname ? pcell->pathname:"", pcell->filename ? pcell->filename:"", ret);
                    } else {
                        if (pcell->handler)
                            ret = pcell->handler(pcell, 0, y); /* 0 for not timeout */
                        else
                            ret = -1;
                    }
                    if (ret > 0) {
                        if (timeout < 0 || timeout > ret)
                            timeout = ret;
                    }
                    break; /* for (i = 0; i < YLOG_INOTIFY_MAX; i++) */
                }
            }
        }

        event_size = sizeof(*event) + event->len;
        count -= event_size;
        event_pos += event_size;
    }

    return timeout;
}
