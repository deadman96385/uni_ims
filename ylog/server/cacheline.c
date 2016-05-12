/**
 * Copyright (C) 2016 Spreadtrum Communications Inc.
 */

static int cacheline_update_status(enum cacheline_status status, struct cacheline *cl) {
    pthread_mutex_lock(&cl->mutex);
    cl->status = status;
    pthread_cond_broadcast(&cl->cond);
    pthread_mutex_unlock(&cl->mutex);

    do {
        pthread_mutex_lock(&cl->mutex);
        /* Step 2 may be on going */
        pthread_mutex_unlock(&cl->mutex);
        if (cl->status != CACHELINE_RUN)
            usleep(1000);
    } while (cl->status != CACHELINE_RUN);

    return 0;
}

static int cacheline_update_timeout_default(long millisecond, struct cacheline *cl) {
    cl->timeout = millisecond;
    return cacheline_update_status(CACHELINE_UPDATE_TIMEOUT, cl);
}

static int cacheline_flush_default(struct cacheline *cl) {
    return cacheline_update_status(CACHELINE_FLUSH, cl);
}

static int cacheline_exit_default(struct cacheline *cl) {
    return cacheline_update_status(CACHELINE_EXIT, cl);
}

inline static char *cacheline_line_pointer(int idx, struct cacheline *cl) {
    return cl->cache + idx * cl->size; /* if idx << 14 is faster by luther */
}

static void cacheline_drain(struct cacheline *cl, int reason) {
    char *pcache;
    int ret;
    if (cl->wpos) {
        long wpos = cl->wpos;
        cl->wpos = 0;
        if (cl->debuglevel & CACHELINE_DEBUG_INFO) {
            ylog_info("%s cacheline drain, reason %d, write back %d to disk, collect %ld write actions, cachelines %ld\n",
                    cl->name, reason, wpos, cl->writes, cl->writes_cachelines);
            cl->writes = 0;
            cl->writes_cachelines = 0;
        }
        pcache = cacheline_line_pointer(cl->num, cl);
        memcpy(pcache, cacheline_line_pointer(cl->rclidx, cl), wpos);
        cl->writing = 1;
        pthread_mutex_unlock(&cl->mutex); /* give ydst more chance to use other free cachelines */
        if (cl->bypass == 0) {
            if (cl->ydst->write_data2cache_first == 0)
                ret = cl->ydst->fwrite(pcache, wpos, cl->ydst->fd, cl->ydst->file_name);
            else {
                ret = cl->ydst->write_handler(pcache, wpos, cl->ydst->ylog);
                if (ret > wpos)
                    ret = wpos;
            }
        } else
            ret = wpos;
        if (ret != wpos)
            ylog_critical("%s cacheline write wrong %d -> %d\n", cl->name, wpos, ret);
        cl->writing = 0;
        pthread_mutex_lock(&cl->mutex);
    }
}

static int grab_one_new_cacheline(struct cacheline *cl) {
    int wclidx;
    do {
        wclidx = (cl->wclidx + 1) % cl->num;
        if (wclidx == cl->rclidx) {
            /**
             * all cacheline are full
             */
            pthread_mutex_unlock(&cl->mutex);
            if (cl->debuglevel & CACHELINE_DEBUG_DATA)
                ylog_debug("cacheline %s are full\n", cl->name);
            usleep(5000); /* 5ms, if cl->size is 512k, (512/1024.0)/0.005 = 100M/s disk write speed */
            pthread_mutex_lock(&cl->mutex);
        } else {
            cl->wclidx = wclidx;
            cl->wpos = 0;
            return wclidx;
        }
    } while (1);
}

static int cacheline_write_default(char *buf, int count, struct cacheline *cl) {
    char *pcache;
    int size;
    int cachefull = 0;
    int count_saved = count;

    pthread_mutex_lock(&cl->mutex);
    if (cl->debuglevel & CACHELINE_DEBUG_INFO)
        cl->writes++;
    do {
        size = cl->size - cl->wpos;
        if (size) {
            /**
             * this cacheline has more space
             */
            if (cl->debuglevel & CACHELINE_DEBUG_DATA)
                ylog_debug("xxxxxxxxxxxxxxx %d/%d, size=%d, cl->wclidx=%d, cl->wpos=%d, cl->size=%d, \n",
                    count, count_saved, size, cl->wclidx, cl->wpos, cl->size);
            if (size >= count)
                size = count;
            pcache = cacheline_line_pointer(cl->wclidx, cl);
            memcpy(pcache + cl->wpos, buf, size);
            count -= size;
            cl->wpos += size;
            buf += size;
            if (cl->size <= cl->wpos) {
                /**
                 * this cacheline is filled full
                 */
                cachefull = 1;
                if (count) {
                    /* we still have data to save, otherwise no need to wait by luther*/
                    grab_one_new_cacheline(cl);
                }
            }
        } else {
            /**
             * this cacheline has full, we need to get a new one
             */
            if (cl->debuglevel & CACHELINE_DEBUG_DATA)
                ylog_debug("yyyyyyyyyyyyyyy %d/%d, cl->wclidx=%d, cl->wpos=%d, cl->size=%d, \n",
                    count, count_saved, cl->wclidx, cl->wpos, cl->size);
            grab_one_new_cacheline(cl);
        }
    } while (count);

    if (cachefull)
        pthread_cond_broadcast(&cl->cond);
    pthread_mutex_unlock(&cl->mutex);

    return count_saved;
}

static void *cacheline_thread_handler_default(void *arg) {
    struct cacheline *cl = arg;
    struct timespec *ts = &cl->ts;
    int *millisecond = &cl->timeout;
    pthread_mutex_t *mutex = &cl->mutex;
    pthread_cond_t *cond = &cl->cond;
    char *pcache;
    int ret;

    os_hooks.pthread_create_hook(NULL, "cacheline %s", cl->name);

    cl->pid = getpid();
    cl->tid = gettid();

    ylog_info("cacheline %s --> %s is started, pid=%d, tid=%d\n", cl->name, cl->ydst->file, cl->pid, cl->tid);

    for (;;) {
        /* Step 1. wait for cacheline data full */
        if ((cl->status == CACHELINE_RUN) && \
            (cl->wclidx == cl->rclidx)) {
            if (*millisecond) {
                if (pthread_cond_timedwait_monotonic2(*millisecond, cond, mutex, ts) == ETIMEDOUT) {
                    if (cl->wclidx == cl->rclidx) {
                        /**
                         * every timeout happens, we will try to flush data back
                         * to disk ASAP to avoid more log missing if unknow os reboot time happen
                         */
                        cacheline_drain(cl, 0);
                        continue;
                    }
                }
            } else
                pthread_cond_wait(cond, mutex);
        }
        /* Step 2. write back cacheline */
        while (cl->wclidx != cl->rclidx) {
            pcache = cacheline_line_pointer(cl->rclidx, cl);
            if (cl->debuglevel & CACHELINE_DEBUG_INFO)
                cl->writes_cachelines++;
            cl->writing = 1;
            pthread_mutex_unlock(&cl->mutex); /* give ydst more chance to use other free cachelines */
            if (cl->bypass == 0) {
                if (cl->ydst->write_data2cache_first == 0)
                    ret = cl->ydst->fwrite(pcache, cl->size, cl->ydst->fd, cl->ydst->file_name);
                else {
                    ret = cl->ydst->write_handler(pcache, cl->size, cl->ydst->ylog);
                    if (ret > cl->size)
                        ret = cl->size;
                }
            } else
                ret = cl->size;
            if (ret != cl->size)
                ylog_critical("%s cacheline write cacheline wrong %d -> %d\n", cl->name, cl->size, ret);
            cl->writing = 0;
            pthread_mutex_lock(&cl->mutex);
            cl->rclidx = (cl->rclidx + 1) % cl->num; /* free this cacheline to let ydst use */
            if (cl->wclidx_max < cl->wclidx)
                cl->wclidx_max = cl->wclidx;
        }
        /* Step X. wrap back the wclidx pointer to let malloc avoid doing more physical memory page missing request */
        if (cl->wclidx == cl->rclidx) {
            if (cl->wclidx != 0) { /* wclidx will be wrapped back to index 0 slot */
                long wpos = cl->wpos;
                if (wpos) {
                    char *pcache = cacheline_line_pointer(0, cl);
                    memcpy(pcache, cacheline_line_pointer(cl->wclidx, cl), wpos);
                }
                if (cl->debuglevel & CACHELINE_DEBUG_WCLIDX_WRAP)
                    ylog_info("%s cacheline wrap back to 0 from %d %dbytes\n", cl->name, cl->wclidx, wpos);
                cl->wclidx = cl->rclidx = 0;
            }
        } else {
            if (cl->status == CACHELINE_RUN)
                ylog_critical("%s cacheline fatal error, wclidx=%d, rclidx=%d, wpos=%d\n",
                        cl->name, cl->wclidx, cl->rclidx, cl->wpos);
        }
        /* Step 3. flush or exit */
        if (cl->status != CACHELINE_RUN) {
            if (cl->status == CACHELINE_FLUSH || \
                cl->status == CACHELINE_EXIT)
                cacheline_drain(cl, 1);
            if (cl->status == CACHELINE_EXIT)
                break;
            cl->status = CACHELINE_RUN;
        }
    }

    cl->status = CACHELINE_RUN;
    free(cl->cache);

    ylog_info("cacheline %s --> %s exited\n", cl->name, cl->ydst->file);

    return NULL;
}
