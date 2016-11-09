/**
 * Copyright (C) 2016 Spreadtrum Communications Inc.
 */

static int fd_write(char *buf, int count, int fd, char *desc) {
    int c;
    int retries = 5;
    int written = 0;

    if (fd < 0) {
        ylog_critical("write fd does not open %s\n", desc);
        return 0;
    }

    do {
        c = write(fd, buf, count);
        if (c > 0) {
            written += c;
            if (count == c) {
                break;
            } else {
                buf += c;
                count -= c;
            }
        } else {
            ylog_error("%s write failed %d: %d %s\n", desc, c, fd, strerror(errno));
        }
        usleep(10*1000);
    } while (--retries);

    if (retries == 0) {
        ylog_critical("write failed: retries all %s\n", desc);
    }

    return written;
}

static int copy_file(int fd_to, int fd_from, int max_size_limit, char *desc) {
    char buf[64 * 1024];
    int ret, cnt = 0;
    do {
        ret = read(fd_from, buf, sizeof buf);
        if (ret > 0) {
            cnt += ret;
            if (ret != fd_write(buf, ret, fd_to, desc)) {
                ylog_info("write %s fail.%s", desc, strerror(errno));
                ret = -1;
                break;
            }
        } else {
            if (ret < 0)
                ylog_info("read %s fail.%s", desc, strerror(errno));
            break;
        }
    } while (cnt < max_size_limit);

    return ret < 0 ? -1:0;
}

pid_t gettid(void) {
    /**
     * man gettid
     *
     * Note: There is no glibc wrapper for this system call; see NOTES.
     *
     * gettid()  returns the caller's thread ID (TID).  In a single-threaded process,
     * the thread ID is equal to the process ID (PID, as returned by getpid(2)).
     * In a multithreaded process, all threads have the same PID, but each one has a  unique
     * TID.  For further details, see the discussion of CLONE_THREAD in clone(2).
     *
     * On success, returns the thread ID of the calling process.
     */
    return syscall(SYS_gettid);
}

static int ylog_pthread_create(ylog_pthread_handler handler, void *arg) {
    pthread_t ptid;
    if (pthread_create(&ptid, NULL, handler, arg)) {
        ylog_error("Failed to pthread_create %s\n", strerror(errno));
        return -1;
    }
    return 0;
}

static unsigned long long calculate_path_disk_available(char *path) {
    struct statfs diskInfo;

    memset(&diskInfo, 0, sizeof diskInfo);

    if (statfs(path, &diskInfo)) {
        ylog_error("statfs failed %s %s\n", path, strerror(errno));
        return 0;
    }

    return diskInfo.f_bavail * diskInfo.f_bsize;
}

static void pthread_cond_timedwait_monotonic_init(pthread_cond_t *cond, pthread_condattr_t *condattr) {
    pthread_condattr_init(condattr);
    pthread_condattr_setclock(condattr, CLOCK_MONOTONIC);
    pthread_cond_init(cond, condattr);
}

static int pthread_cond_timedwait_monotonic2(int millisecond, pthread_cond_t *cond,
                    pthread_mutex_t *mutex, struct timespec *ts) {
    clock_gettime(CLOCK_MONOTONIC, ts);
    ts->tv_sec += millisecond / 1000;
    ts->tv_nsec += 1000000 * (millisecond % 1000);
    if (ts->tv_nsec >= 1000*1000*1000) {
        ts->tv_sec++;
        ts->tv_nsec -= 1000*1000*1000;
    }
    return pthread_cond_timedwait(cond, mutex, ts);
}

static int ylog_tv2format_time_year(char *timeBuf, struct timeval *tv) {
    struct tm tm;
    struct tm* ptm;
    time_t t;
    int len;

    t = tv->tv_sec; //time(NULL);
//#if defined(HAVE_LOCALTIME_R)
    ptm = localtime_r(&t, &tm);
//#else
//    ptm = localtime(&t);
//#endif
    /* strftime(timeBuf, sizeof(timeBuf), "%Y-%m-%d %H:%M:%S", ptm); */
    len = strftime(timeBuf, 32, "%Y%m%d-%H%M%S", ptm);
    if (1/* usec_time_output */) {
        /* 20160501-221436.000 */
        len += snprintf(timeBuf + len, 32 - len,
                ".%03ld", tv->tv_usec / 1000);
    } else {
        /* 20160501-221436.000 */
        len += snprintf(timeBuf + len, 32 - len,
                 ".%03ld", tv->tv_usec / 1000000);
    }
    return len;
}

static int ylog_get_format_time_year(char *buf) {
    char *timeBuf = buf;
    struct timeval tv;
    //char timeBuf[32];/* good margin, 23+nul for msec, 26+nul for usec */
    /* From system/core/liblog/logprint.c */
    /*
     * Get the current date/time in pretty form
     *
     * It's often useful when examining a log with "less" to jump to
     * a specific point in the file by searching for the date/time stamp.
     * For this reason it's very annoying to have regexp meta characters
     * in the time stamp.  Don't use forward slashes, parenthesis,
     * brackets, asterisks, or other special chars here.
     */
    gettimeofday(&tv, NULL);
    return ylog_tv2format_time_year(timeBuf, &tv);
}

static int ylog_tv2format_time(char *timeBuf, struct timeval *tv) {
    struct tm tm;
    struct tm* ptm;
    time_t t;
    int len;

    t = tv->tv_sec; //time(NULL);
//#if defined(HAVE_LOCALTIME_R)
    ptm = localtime_r(&t, &tm);
//#else
//    ptm = localtime(&t);
//#endif
    /* strftime(timeBuf, sizeof(timeBuf), "%Y-%m-%d %H:%M:%S", ptm); */
    len = strftime(timeBuf, 32, "[%m-%d %H:%M:%S", ptm);
    if (1/* usec_time_output */) {
        /* 01-01 22:14:36.000   826   826 D SignalClusterView_dual: */
        len += snprintf(timeBuf + len, 32 - len,
                ".%03ld] ", tv->tv_usec / 1000);
    } else {
        /* 01-01 22:14:36.000   826   826 D SignalClusterView_dual: */
        len += snprintf(timeBuf + len, 32 - len,
                 ".%03ld] ", tv->tv_usec / 1000000);
    }
    return len;
}

static int ylog_ts2format_time(char *timeBuf, struct timespec *ts) {
    struct tm tm;
    struct tm* ptm;
    time_t t;
    int len;

    t = ts->tv_sec; //time(NULL);
//#if defined(HAVE_LOCALTIME_R)
    ptm = localtime_r(&t, &tm);
//#else
//    ptm = localtime(&t);
//#endif
    /* strftime(timeBuf, sizeof(timeBuf), "%Y-%m-%d %H:%M:%S", ptm); */
    len = strftime(timeBuf, 32, "[%m-%d %H:%M:%S", ptm);
#if 1
    /* usec_time_output */
    /* 01-01 22:14:36.000   826   826 D SignalClusterView_dual: */
    len += snprintf(timeBuf + len, 32 - len,
            ".%03ld] ", ts->tv_nsec / 1000000);
#else
    /* 01-01 22:14:36.000   826   826 D SignalClusterView_dual: */
    len += snprintf(timeBuf + len, 32 - len,
             ".%03ld] ", ts->tv_nsec / 1000000);
#endif
    return len;
}

static int ylog_get_format_time(char *buf) {
    char *timeBuf = buf;
    struct timeval tv;
    //char timeBuf[32];/* good margin, 23+nul for msec, 26+nul for usec */
    /* From system/core/liblog/logprint.c */
    /*
     * Get the current date/time in pretty form
     *
     * It's often useful when examining a log with "less" to jump to
     * a specific point in the file by searching for the date/time stamp.
     * For this reason it's very annoying to have regexp meta characters
     * in the time stamp.  Don't use forward slashes, parenthesis,
     * brackets, asterisks, or other special chars here.
     */
    gettimeofday(&tv, NULL);
    return ylog_tv2format_time(timeBuf, &tv);
}

#ifdef YLOG_PRINT_TIME
static int ylog_printf_format(struct context *c, int level, const char *fmt, ...) {
    if (c && (int)c->loglevel >= level) {
        int len;
        va_list ap;
        char buf[4096];
        len = ylog_get_format_time(buf);
        va_start(ap, fmt);
        vsnprintf(buf + len, sizeof(buf) - len, fmt, ap);
        va_end(ap);
        ___ylog_printf___("%s", buf);
    }
    return 0;
}
#endif

static unsigned long get_gap(unsigned long long before, unsigned long long curr) {
    return (unsigned long)(curr - before);
}

/*
static int ylog_get_unit_size(unsigned long long size, char *unit) {
    if (size >= 1024*1024*1024) {
        *unit = 'G';
        return size / (1024*1024*1024);
    } else if (size >= 1024*1024) {
        *unit = 'M';
        return size / (1024*1024);
    } else if (size >= 1024) {
        *unit = 'K';
        return size / (1024);
    } else {
        *unit = 'B';
        return size;
    }
}
*/

static float ylog_get_unit_size_float(unsigned long long size, char *unit) {
    if (size >= 1024*1024*1024) {
        *unit = 'G';
        return (float)size / (1024*1024*1024);
    } else if (size >= 1024*1024) {
        *unit = 'M';
        return (float)size / (1024*1024);
    } else if (size >= 1024) {
        *unit = 'K';
        return (float)size / (1024);
    } else {
        *unit = 'B';
        return size;
    }
}

static unsigned long ylog_get_speed(unsigned long long size, time_t millisecond) {
    if (millisecond == 0)
        return 0;
    return (1000 * size) / millisecond;
}

static float ylog_get_unit_size_float_with_speed(unsigned long long size, char *unit, time_t millisecond) {
    float ret;
    if (millisecond == 0) {
        *unit = 'B';
        return 0;
    }
    /* GB/s */
    if (size >= 1024*1024*1024) {
        ret = (1000 * (float)size / (1024*1024*1024)) / millisecond;
        if (ret > 1) {
            *unit = 'G';
            return ret;
        }
    }
    /* MB/s */
    if (size >= 1024*1024) {
        ret = (1000 * (float)size / (1024*1024)) / millisecond;
        if (ret > 1) {
            *unit = 'M';
            return ret;
        }
    }
    /* KB/s */
    if (size >= 1024) {
        ret = (1000 * (float)size / (1024)) / millisecond;
        if (ret > 1) {
            *unit = 'K';
            return ret;
        }
    }
    /* B/s */
    *unit = 'B';
    return (1000 * (float)size) / millisecond;
}

static int get_monotime(struct timespec *ts) {
    if (clock_gettime(CLOCK_MONOTONIC, ts) == -1) {
        ylog_error("Could not get monotonic time: %s\n", strerror(errno));
        return -1;
    }
    return 0;
}

static int get_boottime(struct timespec *ts) {
    struct timespec elapsed_timespec;
    if (clock_gettime(CLOCK_BOOTTIME, &elapsed_timespec) == -1) {
        ylog_error("Could not get boot time: %s\n", strerror(errno));
        return -1;
    }
    if (ts)
        *ts = elapsed_timespec;
    return 0;
}

static unsigned long long currentTimeMillis(void) {
#if 0
    struct timeval tv;
    gettimeofday(&tv, (struct timezone *) NULL);
    return (unsigned long long)(tv.tv_sec * 1000 + tv.tv_usec / 1000);
#else
    struct timespec ts;
    get_monotime(&ts);
    return (unsigned long long)ts.tv_sec * 1000L + ts.tv_nsec / 1000000;
#endif
}

#if 0
static time_t diff_tv_millisecond(struct timeval *b, struct timeval *a) {
    /**
     * b -- before
     * a -- after
     */
    return (a->tv_sec - b->tv_sec) * 1000 + (a->tv_usec - b->tv_usec) / 1000;
}
#endif

static time_t diff_ts_millisecond(struct timespec *b, struct timespec *a) {
    /**
     * b -- before
     * a -- after
     */
    return (a->tv_sec - b->tv_sec) * 1000 + (a->tv_nsec - b->tv_nsec) / 1000000;
}

static char *basename(char *s) {
    /**
     * musl/src/misc/basename.c
     */
    size_t i;
    if (!s || !*s) return ".";
    i = strlen(s)-1;
    for (; i&&s[i]=='/'; i--) s[i] = 0;
    for (; i&&s[i-1]!='/'; i--);
    return s+i;
}

static char *dirname(char *s) {
    /**
     * musl/src/misc/dirname.c
     */
    int i;
    if (!s || !*s) return "./";
    i = strlen(s)-1;
    for (; s[i]=='/'; i--) if (!i) return "/";
    for (; s[i]!='/'; i--) if (!i) return "./";
    for (; s[i]=='/'; i--) if (!i) return "/";
    s[i+1] = 0;
    return s;
}

static char *dirname2(char *s) {
    /**
     * musl/src/misc/dirname.c
     */
    int i;
    if (!s || !*s) return ".";
    i = strlen(s)-1;
    // a/b/c/ --> a/b/c
    // a/b/c -- > a/b
    // for (; s[i]=='/'; i--) if (!i) return "/";
    for (; s[i]!='/'; i--) if (!i) return ".";
    for (; s[i]=='/'; i--) if (!i) return "/";
    s[i+1] = 0;
    return s;
}

static int mkdirs(char *path) {
    /**
     * "a/b/c/d
     * system/core/toolbox/mkdir.c
     */
    int ret;
    char tmp[PATH_MAX];
    char currpath[PATH_MAX], *pathpiece;
    struct stat st;
    char *last;
    /* reset path */
    strcpy(tmp, path);
    strcpy(currpath, "");
    /* create the pieces of the path along the way */
    // pathpiece = strtok(tmp, "/"); // when multi thread strtok has bug by luther
    pathpiece = strtok_r(tmp, "/", &last);
    if (tmp[0] == '/') {
        /* prepend / if needed */
        strcat(currpath, "/");
    }
    while (pathpiece != NULL) {
        if (strlen(currpath) + strlen(pathpiece) + 2/*NUL and slash*/ > PATH_MAX) {
            ylog_critical("Invalid path %s specified: too long\n", path);
            return -1;
        }
        strcat(currpath, pathpiece);
        strcat(currpath, "/");
        if (stat(currpath, &st) != 0) {
            ret = mkdir(currpath, 0777);
            if (ret < 0) {
                ylog_error("mkdir failed for %s, %s\n", currpath, strerror(errno));
                return ret;
            }
        }
        pathpiece = strtok_r(NULL, "/", &last);
    }
    return 0;
}

static int mkdirs_with_file(char *file) {
    char tmp[PATH_MAX];
    strcpy(tmp, file);
    return mkdirs(dirname(tmp));
}

static int do_cmd(char *buf, int count, int print_cmd, const char *fmt, ...) {
    int ret;
    va_list ap;
    FILE *f;
    int fd;
    char cmd[4096];

    va_start(ap, fmt);
    vsnprintf(cmd, sizeof(cmd), fmt, ap);
    va_end(ap);

    f = popen(cmd, "r");

    if (f) {
        /**
         * man 2 pclose
         * The pclose() function waits for the associated process to terminate and
         * returns the  exit status of the command as returned by wait4(2).
         * The  pclose()  function  returns  -1 if wait4(2) returns an error,
         * or some other error is detected.  In the event of an error, these functions
         * set errno to indicate the cause of the error.
         */
        fd = fileno(f);
        if (buf)
            ret = read(fd, buf, count);
        else
            ret = 0;
        if (pclose(f) < 0) {
            ylog_error("do_cmd %s failed: %s\n", cmd, strerror(errno));
            return -1;
        } else {
            if (print_cmd)
                ylog_debug("do_cmd %s success, ret=%d\n", cmd, ret);
        }
        return ret;
    } else {
        ylog_critical("do_cmd %s failed\n", cmd);
    }

    return -1;
}

#if 0
static int rm(char *path) {
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "rm -f %s", path);
    if (do_cmd(cmd) == 0) {
        ylog_critical("Success: %s\n", cmd);
    } else {
        ylog_critical("Failed: %s\n", cmd);
    }
}
#endif

static int rm_all(char *path) {
    return do_cmd(NULL, 0, 1, "rm -rf %s", path);
}

static int rm_sub_all(char *path) {
    return do_cmd(NULL, 0, 1, "rm -rf %s/*", path);
}

static int mv(char *from, char *to) {
    return do_cmd(NULL, 0, 1, "mv -f %s %s", from, to);
}

static int uptime(char *buf, int count) {
    return do_cmd(buf, count, 1, "uptime");
}
