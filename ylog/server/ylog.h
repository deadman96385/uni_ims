/**
 * Copyright (C) 2016 Spreadtrum Communications Inc.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <time.h>
#include <pthread.h>
#include <sys/poll.h>
#include <sys/stat.h>
#include <stdarg.h>
#include <sys/vfs.h>
#include <dlfcn.h>

#define NEW_SEGMENT_FAST_METHOD
#define NEW_SEGMENT_FAST_METHOD_NUM_WIDTH 4
#define NEW_SEGMENT_MAX_NUM 50

#ifndef PATH_MAX
#define PATH_MAX 1024
#endif

#define UNUSED(x) (void)(x) /* avoid compiler warning */

extern struct context *global_context;
extern struct ydst *global_ydst;
extern struct ylog *global_ylog;
extern struct ydst_root *global_ydst_root;
extern struct os_hooks os_hooks;
static int ylog_printf_format(struct context *c, int level, const char *fmt, ...);
//static char *basename(char *s);

#define for_each_ylog(i, y, inity)    \
    for (y = inity ? inity : global_ylog, i = 0; i < YLOG_MAX; i++, y++)

#define for_each_ydst(i, yd, inityd) \
    for (yd = inityd ? inityd : global_ydst, i = 0; i < YDST_MAX; i++, yd++)

#define LSEEK(fd, offset, whence) ({\
    int ll_ret = lseek(fd, offset, whence); \
    if (ll_ret < 0) \
        ylog_error("lseek %s %s\n", __func__, strerror(errno)); \
    ll_ret; \
})

#define SEND(sockfd, buf, len, flags) ({ \
    int ll_ret = send(sockfd, buf, len, flags); \
    if (ll_ret > 0) \
        ylog_debug("ylog_cli ack : %s", buf); \
    else \
        ylog_error("send %s %s\n", __func__, strerror(errno)); \
    ll_ret; \
})

#define CLOSE(fd) ({\
    int ll_ret = -1; \
    if (fd < 0) { \
        ylog_critical("BUG close fd is %d %s\n", fd, __func__); \
    } else {\
        if (fd == 0) \
            ylog_critical("BUG: close fd is %d %s\n", fd, __func__); \
        ll_ret = close(fd); \
        if (ll_ret < 0) \
            ylog_error("close %d %s %s\n", fd, __func__, strerror(errno)); \
    } \
    ll_ret; \
})

#define RENAME(from, to) ({\
    int ll_ret; \
    ll_ret = rename(from, to); \
    if (ll_ret < 0) \
        ylog_error("rename %s -> %s %s %s\n", from, to, __func__, strerror(errno)); \
    ll_ret; \
})

/* it is used for pending cpath hotplug */
#define PCMDS_YLOG_CALL_LOCK(y) \
    struct ydst *ydst = y->ydst; \
    struct ydst_root *root; \
    /**
     * In ylog_write_handler_default() the mutex sequence is
     * 1. lock ydst->mutex in ylog_write_handler_default
     * 2. lock root->mutex in ydst_new_segment_default
     * so here we need to keep the same sequence to avoid dead lock in some race condition
     */ \
    pthread_mutex_lock(&ydst->mutex); \
    root = ydst->root; \
    pthread_mutex_lock(&root->mutex); /* To avoid root folder change by ydst_move_root */

#define PCMDS_YLOG_CALL_UNLOCK() \
    pthread_mutex_unlock(&root->mutex); \
    pthread_mutex_unlock(&ydst->mutex);

enum loglevel {
    LOG_ERROR,
    LOG_CRITICAL,
    LOG_WARN,
    LOG_INFO,
    LOG_DEBUG,
    LOG_LEVEL_MAX,
};

#ifdef ANDROID
#define LOG_TAG "YLOG"
#include "cutils/log.h"
#include <cutils/sockets.h>
#define ___ylog_printf___ SLOGW
#else
#define ___ylog_printf___ printf
#endif
#define YLOG_PRINT_TIME
#define ARRAY_LEN(A) (sizeof(A)/sizeof((A)[0]))
#ifdef YLOG_PRINT_TIME
#define ylog_printf(c, level, fmt...) ylog_printf_format(c, level, fmt)
#else
#define ylog_printf(c, level, fmt...) if (c && c->loglevel >= level) ___ylog_printf___(fmt)
#endif
#define ylog_debug(msg...) ylog_printf(global_context, LOG_DEBUG, "ylog<debug> "msg)
#define ylog_info(msg...) ylog_printf(global_context, LOG_INFO, "ylog<info> "msg)
#define ylog_warn(msg...) ylog_printf(global_context, LOG_WARN, "ylog<warn> "msg)
#define ylog_critical(msg...) ylog_printf(global_context, LOG_CRITICAL, "ylog<critical> "msg)
#define ylog_error(msg...) ylog_printf(global_context, LOG_ERROR, "ylog<error> "msg)

#define YTAG_TAG_PROPERTY       0x05
#define YTAG_TAG_NEWFILE_BEGIN  0x10
#define YTAG_TAG_NEWFILE_END    0x11
#define YTAG_TAG_RAWDATA        0x12
struct ytag {
    unsigned int tag; /* u32bits le32_to_cpu; All of them are Little Endian stored */
    unsigned int len;
    /* unsigned int namelen; */
    char data[0];
};

#define YTAG_MAGIC      0xf00e789a
#define YTAG_VERSION    0x00000001
struct ytag_header {
    unsigned int tag; /* u32bits le32_to_cpu; All of them are Little Endian stored */
    unsigned int len;
    unsigned int version;
    unsigned int reserved[65];
};

#define ytag_rawdata(buf, count) do { \
    ytag.tag = YTAG_TAG_RAWDATA; \
    ytag.len = sizeof(ytag) + count; \
    write(STDOUT_FILENO, &ytag, sizeof(ytag)); \
    if (count) \
        write(STDOUT_FILENO, buf, count); \
} while (0);

#define ytag_newfile(_YTAG, _NAME) do { \
    int ynlen; char *ylname = _NAME; \
    ytag.tag = _YTAG; \
    ytag.len = sizeof(ytag); \
    if (ylname) { \
        ynlen = strlen(ylname); \
        ytag.len += ynlen; \
    } \
    write(STDOUT_FILENO, &ytag, sizeof(ytag)); \
    if (ylname && ynlen) \
        write(STDOUT_FILENO, ylname, ynlen); \
} while (0);

#define ytag_newfile_begin(_NAME) ytag_newfile(YTAG_TAG_NEWFILE_BEGIN, _NAME)
#define ytag_newfile_end(_NAME) ytag_newfile(YTAG_TAG_NEWFILE_END, _NAME)

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

struct ylog_snapshot_list_s {
    char *name;
    char *usage;
    void (*f)(struct ylog_snapshot_args *args);
};

struct ylog_keyword {
    const char *key;
    void (*handler)(struct ylog_keyword *kw, int nargs, char **args);
};

enum contextual_model {
    C_FULL_LOG = 0,
    C_MINI_LOG,
    C_NO_LOG,
};

enum mode_types {
    M_USER = 0,
    M_USER_DEBUG,
    M_MODE_NUM,
};

struct context {
    char *config_file;
    char *journal_file;
    char *ylog_config_file;
    char *filter_plugin_path;
    int journal_file_size;
    enum contextual_model model;
    enum loglevel loglevel;
    char *historical_folder_root;
    int keep_historical_folder_numbers;
    int keep_historical_folder_numbers_default;
    int pre_fill_zero_to_possession_storage_spaces;
    struct timeval tv;
    struct tm tm;
    struct timespec ts;
    char timeBuf[32];
    int ignore_signal_process;
    int command_loop_ready;
    struct ylog_keyword *ylog_keyword;
    struct ylog_snapshot_list_s *ylog_snapshot_list;
};

enum file_type {
    FILE_NORMAL = 0,
    FILE_SOCKET_LOCAL_SERVER,
    FILE_SOCKET_LOCAL_CLIENT,
    FILE_POPEN,
    FILE_INOTIFY,
};

enum ylog_thread_state {
    YLOG_RUN = 0, /* A */
    YLOG_SUSPEND, /* B */
    YLOG_RESUME, /* C */
    YLOG_STOP, /* D */
    YLOG_EXIT, /* E */
    YLOG_RESTART, /* F */
    YLOG_MOVE_ROOT, /* G */
    YLOG_RESIZE_SEGMENT, /* H */
    YLOG_FLUSH, /* I */
    YLOG_RESET, /* J */
    YLOG_NOP, /* Z */
};

enum ylog_event_thread_type {
    YLOG_EVENT_THREAD_TYPE_OS_TIMER,
    YLOG_EVENT_THREAD_TYPE_MAX
};

enum filter_status_t {
    NORMAL = 0,
    START,
};

struct ylog;
struct cacheline;
struct filter_pattern;
struct ylog_event_cond_wait;
typedef int (*ydst_new_segment)(struct ylog *y, int mode);
typedef int (*ylog_filter)(char *line, struct filter_pattern *p);
typedef int (*ylog_write_handler)(char *buf, int count, struct ylog *y, void *private);
typedef int (*ylog_open)(char *file, char *mode, struct ylog *y);
typedef int (*ylog_read)(char *buf, int count, FILE *fp, int fd, struct ylog *y);
typedef int (*ylog_close)(FILE *fp, struct ylog *y);
typedef void*(*ylog_exit)(struct ylog *y);
typedef int (*ylog_write_header)(struct ylog *y);
typedef int (*ylog_write_timestamp)(struct ylog *y);
typedef void*(*ylog_thread_handler)(void *arg);
typedef int (*ylog_thread_run)(struct ylog *y, int block);
typedef int (*ylog_thread_suspend)(struct ylog *y, int block);
typedef int (*ylog_thread_resume)(struct ylog *y, int block);
typedef int (*ylog_thread_stop)(struct ylog *y, int block);
typedef int (*ylog_thread_exit)(struct ylog *y, int block);
typedef int (*ylog_thread_restart)(struct ylog *y, int block);
typedef int (*ylog_thread_move_root)(struct ylog *y, int block);
typedef int (*ylog_thread_resize_segment)(struct ylog *y, int block);
typedef int (*ylog_thread_flush)(struct ylog *y, int block);
typedef int (*ylog_thread_reset)(struct ylog *y, int block);
typedef int (*ylog_thread_nop)(struct ylog *y, int block);
typedef int (*ylog_event_timer_handler)(void *, long tick, struct ylog_event_cond_wait *yewait);
typedef int (*cacheline_update_timeout)(long millisecond, struct cacheline *cl);
typedef int (*cacheline_flush)(struct cacheline *cl);
typedef int (*cacheline_write)(char *buf, int count, struct cacheline *cl);
typedef int (*cacheline_exit)(struct cacheline *cl);
typedef void*(*cacheline_thread_handler)(void *arg);
typedef int (*ydst_write)(char *id_token, int id_token_len, char *buf, int count, struct ydst *ydst);
typedef int (*ydst_fwrite)(char *buf, int count, int fd, char *desc);
typedef int (*ydst_flush)(struct ydst *ydst);
typedef int (*ydst_open)(char *file, char *mode, struct ydst *ydst);
typedef int (*ydst_close)(struct ydst *ydst);
typedef void*(*ylog_pthread_handler)(void *arg);

struct ylog_event_cond_wait {
    char *name;
    struct timespec ts;
    int period; /* millisecond */
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    pthread_condattr_t condattr;
};

struct ylog_event_thread {
    pthread_t ptid;
    pid_t pid;
    pid_t tid;
    ylog_event_timer_handler event_handler;
    void *arg;
    enum ylog_thread_state state;
    struct ylog_event_cond_wait yewait;
    struct ylog_event_thread *next;
    enum ylog_event_thread_type type;
};

struct filter_pattern {
    int offset;
    char *key_string;
    int key_string_len;
    ylog_filter filter;
};

#define YLOG_POLL_INDEX_PIPE 0
#define YLOG_POLL_INDEX_DATA 1
#define YLOG_POLL_INDEX_SERVER_SOCK 2
#define YLOG_POLL_INDEX_INOTIFY 3
#define YLOG_POLL_INDEX_MAX 32
#define YLOG_POLL_FLAG_FREE_LATER         (1 << 0)
#define YLOG_POLL_FLAG_COMMAND_ONLINE     (1 << 1)
#define YLOG_POLL_FLAG_THREAD            (1 << 2)
#define YLOG_POLL_FLAG_THREAD_STOP        (1 << 3)
#define YLOG_POLL_FLAG_CAN_BE_FREE_NOW     (1 << 8)
struct ylog_poll {
    FILE *fp[YLOG_POLL_INDEX_MAX];
    struct pollfd pfd[YLOG_POLL_INDEX_MAX];
    int flags[YLOG_POLL_INDEX_MAX];
    void *args[YLOG_POLL_INDEX_MAX];
};

struct ylog_argument {
    struct ylog_poll *yp;
    int index;
    void *args;
    int flags;
};

struct command {
    char *name;
    char *help;
    int (*handler)(struct command *cmd, char *buf, int buf_size, int fd, int index, struct ylog_poll *yp);
    char *args;
};

/**
 * external/blktrace/rbtree.h by luther
 */
#undef offsetof
#ifdef __compiler_offsetof
#define offsetof(TYPE,MEMBER) __compiler_offsetof(TYPE,MEMBER)
#else
#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)
#endif

#define container_of(ptr, type, member) ({                      \
    const typeof( ((type *)0)->member ) *__mptr = (ptr);    \
    (type *)( (char *)__mptr - offsetof(type,member) );})

struct speed {
    unsigned long max_speed;
    unsigned long max_speed_size;
    int max_speed_millisecond;
    struct timeval max_speed_tv_start, max_speed_tv_end;
    time_t second_since_start;
};

struct ydst_root {
    char *root;
    int ydst_change_seq;
    int ydst_change_seq_move_root; /* sequnce number */
    int ydst_change_seq_resize_segment; /* sequnce number */
    char *root_new;
    int refs_cur;
    int refs_cur_move_root;
    int refs_cur_resize_segment;
    int refs; /* how many ydst is using this root by luther */
    unsigned long long max_size; /* the maximum size of this root */
    unsigned long long quota_now; /* the quota for this root now in use */
    unsigned long long quota_new; /* the quota for this root will be used */
#define YDST_ROOT_SPEED_NUM 10
    struct speed speed[YDST_ROOT_SPEED_NUM];
    pthread_mutex_t mutex;
};

#define YDST_TYPE_SOCKET_DEFAULT_SIZE (1 * 1024 * 1024)
enum ydst_type {
    YDST_TYPE_DEFAULT = 0,
    YDST_TYPE_SOCKET,
    YDST_TYPE_YLOG_DEBUG,
#ifdef HAVE_YLOG_INFO
    YDST_TYPE_INFO,
#endif
#ifdef HAVE_YLOG_JOURNAL
    YDST_TYPE_JOURNAL,
#endif
    OS_YDST_TYPE_BASE,
};

enum ydst_segment_mode {
    YDST_SEGMENT_SEQUNCE = 0,
    YDST_SEGMENT_CIRCLE,
};

enum cacheline_status {
    CACHELINE_RUN,
    CACHELINE_UPDATE_TIMEOUT,
    CACHELINE_FLUSH,
    CACHELINE_EXIT,
};

#define CACHELINE_DEBUG_INFO     0x01
#define CACHELINE_DEBUG_CRITICAL 0x02
#define CACHELINE_DEBUG_WCLIDX_WRAP 0x04
#define CACHELINE_DEBUG_DATA     0x80
struct cacheline {
    char *name;
    pthread_t ptid;
    pid_t pid;
    pid_t tid;
    struct timespec ts;
    int writing; /* to tell new_segment waiting */
    unsigned long writes; /* how many disk write times we have saved when this cacheline timeout*/
    unsigned long writes_cachelines; /* how many cachelines has been written back to disk when cache timeout*/
    int debuglevel; /* for debug */
    long size; /* cacheline size */
    int num; /* cacheline numbers */
    int wclidx_max; /* cache line index ever reached max number */
    int wclidx; /* current cache line index being used for writing now */
    int rclidx; /* read cache line index */
    long wpos; /* write pos in current wclidx*/
    int timeout; /* millisecond, if timeout happen, wclidx cache line will be flushed back */
    volatile int status;
    int bypass; /* don't save data to disk, just return */
    struct ydst *ydst;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    pthread_condattr_t condattr;
    cacheline_write write;
    cacheline_update_timeout update_timeout;
    cacheline_flush flush;
    cacheline_exit exit;
    cacheline_thread_handler handler;
    char *cache;
};

#define YLOG_CLEAR_LAST_YLOG (1 << 0)
#define YLOG_CLEAR_CURRENT_RUNNING (1 << 1)
#define YLOG_CLEAR_ALL_QUIT (1 << 2)

#define YDST_SEGMENT_MODE_UPDATE    0
#define YDST_SEGMENT_MODE_NEW        1
#define YDST_SEGMENT_MODE_RESET        2

#define YDST_MAX 80
struct ydst {
    char *file_name;
    char *file;
    char *mode;
    FILE *fp;
    int fd;
    int cache_locked; /* 1 cache mutex has been gotten */
    struct cacheline *cache; /* cache is used for disk and input data process efficiency by luther */

    ydst_open open;
    ydst_close close;
    ydst_write write;
    ydst_flush flush;
    ydst_fwrite fwrite;
    int ydst_change_seq_move_root; /* sequnce number */
    int ydst_change_seq_resize_segment; /* sequnce number */
    char *root_folder;
    struct ydst_root *root; /* root folder */
    int refs; /* how many ylog is using this dst, if more than 1, it will be a shared dst by luther */
    pthread_mutex_t mutex;

    #define YDST_SPEED_NUM 5
    struct speed speed[YDST_SPEED_NUM];
    struct timeval tv;
    struct tm tm;
    struct timespec ts;
    unsigned long long size; /* current size of this dst log (you should sum all existed segment in init process) */
    unsigned long long prev_size; /* last time's size of this dst log (you should sum all existed segment in init process) */
    unsigned long long max_size; /* the maximum size of this dst log */
    unsigned long long segment_size; /* the current size of the current segment */
    unsigned long long max_segment_size; /* the max size of each segment */
    int segment; /* current segment numbers (you should assign right value in init process) */
    long segments; /* all segment numbers generated till now */
    int max_segment; /* how many segment can be reached */
    int nowrap; /* when the log size reaches the max, stop it */
    // begin of write_data2cache_first {
    int write_data2cache_first; /* write data to cache first, then cache will call ylog_write_handler_default */
    ylog_write_handler write_handler; /* used when write_data2cache_first is none 0 */
    struct ylog *ylog; /* used when write_data2cache_first is none 0 */
    // } end of write_data2cache_first
    int ytag; /* 1: use ytag process in analyzer.py; 0: no ytag process in analyzer.py */
    enum ydst_segment_mode segment_mode;

    unsigned long long max_segment_size_now; /* the max size of each segment now in use */
    unsigned long long max_segment_size_new; /* the max size of each segment new will be used */
    int max_segment_now; /* how many segment can be reached now in use */
    int max_segment_new; /* how many segment can be reached new will be used */
    unsigned long long max_size_now; /* the maximum size of this dst log now in use */
    unsigned long long max_size_new; /* the maximum size of this dst log will be used */

    ydst_new_segment new_segment;
};

#if 0
/* kernel/include/uapi/linux/inotify.h */
/* the following are legal, implemented events that user-space can watch for */
#define IN_ACCESS        0x00000001    /* File was accessed */
#define IN_MODIFY        0x00000002    /* File was modified */
#define IN_ATTRIB        0x00000004    /* Metadata changed */
#define IN_CLOSE_WRITE        0x00000008    /* Writtable file was closed */
#define IN_CLOSE_NOWRITE    0x00000010    /* Unwrittable file closed */
#define IN_OPEN            0x00000020    /* File was opened */
#define IN_MOVED_FROM        0x00000040    /* File was moved from X */
#define IN_MOVED_TO        0x00000080    /* File was moved to Y */
#define IN_CREATE        0x00000100    /* Subfile was created */
#define IN_DELETE        0x00000200    /* Subfile was deleted */
#define IN_DELETE_SELF        0x00000400    /* Self was deleted */
#define IN_MOVE_SELF        0x00000800    /* Self was moved */

/* the following are legal events.  they are sent as needed to any watch */
#define IN_UNMOUNT        0x00002000    /* Backing fs was unmounted */
#define IN_Q_OVERFLOW        0x00004000    /* Event queued overflowed */
#define IN_IGNORED        0x00008000    /* File was ignored */

/* helper events */
#define IN_CLOSE        (IN_CLOSE_WRITE | IN_CLOSE_NOWRITE) /* close */
#define IN_MOVE            (IN_MOVED_FROM | IN_MOVED_TO) /* moves */

/* special flags */
#define IN_ONLYDIR        0x01000000    /* only watch the path if it is a directory */
#define IN_DONT_FOLLOW        0x02000000    /* don't follow a sym link */
#define IN_EXCL_UNLINK        0x04000000    /* exclude events on unlinked objects */
#define IN_MASK_ADD        0x20000000    /* add to the mask of an already existing watch */
#define IN_ISDIR        0x40000000    /* event occurred against dir */
#define IN_ONESHOT        0x80000000    /* only send event once */

/*
 * All of the events - we build the list by hand so that we can add flags in
 * the future and not break backward compatibility.  Apps will get only the
 * events that they originally wanted.  Be sure to add new events here!
 */
#define IN_ALL_EVENTS    (IN_ACCESS | IN_MODIFY | IN_ATTRIB | IN_CLOSE_WRITE | \
             IN_CLOSE_NOWRITE | IN_OPEN | IN_MOVED_FROM | \
             IN_MOVED_TO | IN_DELETE | IN_CREATE | IN_DELETE_SELF | \
             IN_MOVE_SELF)
#endif

struct ylog_inotify_files {
    int num;
    int len;
    char *files_array; /* two array char files[num][len] */
};

#define YLOG_INOTIFY_CELL_TYPE_STORE_FILES    (1 << 0)
struct ylog_inotify_cell_args {
    int type;
    char *name;
    char *prefix;
    char *suffix;
    struct ylog_inotify_files file;
};

#define YLOG_INOTIFY_MAX 5
struct ylog_inotify_cell {
    char *pathname; /* if pathname != NULL, YLOG_INOTIFY_TYPE_WATCH_FILE_FOLDER will be set*/
    char *filename;
    unsigned long mask;
    unsigned long event_mask;
#define YLOG_INOTIFY_TYPE_WATCH_FOLDER 0x01 /* only watch the folder */
#define YLOG_INOTIFY_TYPE_WATCH_FILE_FOLDER 0x02 /* watch the file under this folder */
#define YLOG_INOTIFY_TYPE_MASK_EQUAL 0x04 /* must be equal */
#define YLOG_INOTIFY_TYPE_MASK_SUBSET_BIT 0x08 /* if all of the bits are set, maybe other left bits are also set */
    int type;
#define YLOG_INOTIFY_WAITING_TIMEOUT 0x01
    int status;
    int wd; /* watch descriptor - unique id */
    long timeout; /* unit is millisecond */
    int step; /* to mark process step */
    struct timeval tvf; /* first active time */
    struct timeval tv; /* last active time */
    struct timespec ts; /* last active time */
    int (*handler)(struct ylog_inotify_cell *cell, int timeout, struct ylog *y); /* return timeout value, unit is millisecond, -1 means pending wait */
    int (*handler_prev)(struct ylog_inotify_cell *cell, int timeout, struct ylog *y);
    void *args; /* for extension */
    int cache_full; /* cache is full */
};

struct ylog_inotify {
    struct ylog_inotify_cell cells[YLOG_INOTIFY_MAX];
};

typedef int (*ylog_filter_plugin_func)(char *buf, int count, enum filter_status_t status);
struct sfilter_plugin {
    void *handle;
    ylog_filter_plugin_func func;
};

#define YLOG_MAX 100
struct ylog {
    pid_t pid;
    pid_t tid;
    char *buf;
    int buf_size;
    char *name;

#define YLOG_DISABLED                    0x01
#define YLOG_DISABLED_FORCED             0x40
#define YLOG_DISABLED_FORCED_RUNNING    0x80
#define YLOG_STARTED                    0x800
#define YLOG_DISABLED_MASK        (YLOG_DISABLED | YLOG_DISABLED_FORCED | YLOG_DISABLED_FORCED_RUNNING)
    int status;
    int bypass;
    int raw_data;
    int timestamp;
    char *id_token_filename; /* splited filename for this token in python script */
    char *id_token; /* like bio, as a identifier used in mux and demux */
    int id_token_len;
    int restart_period; /* ms */
    struct filter_pattern *fp_array;
    struct ylog_poll yp;
    struct timespec ts;
    #define YLOG_SPEED_NUM 5
    struct speed speed[YLOG_SPEED_NUM];

    enum file_type type;
    char *file;
#define YLOG_READ_MODE_BINARY        0x01
#define YLOG_READ_MODE_BLOCK        0x02
#define YLOG_READ_MODE_BLOCK_RESTART_ALWAYS    0x40
#define YLOG_READ_LEN_MIGHT_ZERO    0x80 /**
                                          * why /proc/kmsg sometime return 0 when we read it?
                                          * lsof | grep kmsg
                                          * we can see logd also read /proc/kmsg after android 6.0,
                                          * code is here: system/core/logd/main.cpp
                                          * so we need to clearly tell logd not read it
                                          * using the following property:
                                          *
                                          * setprop logd.klogd false
                                          *
                                          */
#define YLOG_READ_LEN_MIGHT_ZERO_WAIT_MAX_COUNT 0x100
#define YLOG_GROUP_MODEM            0x8000
#define YLOG_READ_ROOT_USER         0x40000000
    int mode;
    int read_len_zero_count;
    int block_read; /* 1: the file will be a blocked file type, 0: others by luther */
    struct ydst *ydst; /* it will be shared by multi thread */
    unsigned long long size; /* current size of this ylog generated */
    struct ylog_inotify yinotify;

    ylog_write_handler write_handler;
    ylog_read fread;
    ylog_read read;
    ylog_close close;
    ylog_exit exit;
    ylog_open open;
    ylog_write_header write_header;
    ylog_write_timestamp write_timestamp;

    volatile enum ylog_thread_state state;
    pthread_t ptid;
    int state_pipe[2];
    volatile unsigned int state_pipe_count;
    ylog_thread_handler thread_handler;
    ylog_thread_run thread_run;
    ylog_thread_suspend thread_suspend;
    ylog_thread_resume thread_resume;
    ylog_thread_stop thread_stop;
    ylog_thread_exit thread_exit;
    ylog_thread_restart thread_restart;
    ylog_thread_move_root thread_move_root;
    ylog_thread_resize_segment thread_resize_segment;
    ylog_thread_flush thread_flush;
    ylog_thread_reset thread_reset;
    ylog_thread_nop thread_nop;

    struct sfilter_plugin fplugin_filter_log;
    void *privates;
};

struct ynode {
    struct ylog ylog[10];
    struct ydst ydst;
    struct cacheline cache;
};

struct os_hooks {
    ylog_read ylog_read_ylog_debug_hook;
    ylog_read ylog_read_info_hook;
    int (*process_command_hook)(char *buf, int buf_size, int fd, int index, struct ylog_poll *yp);
    void (*cmd_ylog_hook)(int nargs, char **args);
    void (*ylog_status_hook)(enum ylog_thread_state state, struct ylog *y);
    void (*pthread_create_hook)(struct ylog *y, void *args, const char *fmt, ...);
    void (*ready_go)(void);
};

struct pcmds_print2file_args {
    char **cmds;
    char *cmd;
    char *file;
    int flags;
    int *cnt;
#define PCMDS_PRINT2FILE_FD_DUP (9999999)
    int fd_dup_flag;
    int fd;
    ylog_write_handler w;
    struct ylog *y;
    char *prefix;
    int millisecond;
    int max_size;
#define PCMDS_PRINT2FILE_LOCKED 0xffff
    int locked;
    int malloced;
#define PCMDS_PRINT2FD  0xffff
    int print2fd;
};

static inline void yp_clr(int index, struct ylog_poll *yp) {
    yp->pfd[index].fd = -1;
    yp->pfd[index].events = 0;
}

static inline FILE *yp_fp(int index, struct ylog_poll *yp) {
    return yp->fp[index];
}

static inline int yp_fd(int index, struct ylog_poll *yp) {
    return yp->pfd[index].fd;
}

static inline void yp_invalid(int index, struct ylog_poll *yp, struct ylog *y) {
    FILE *fp = yp->fp[index];
    int fd = yp_fd(index, yp);
    if (fp) {
        y->close(fp, y);
    } else if (fd > 0) {
        CLOSE(fd);
    }
    yp->fp[index] = NULL;
    yp_clr(index, yp);
}

static inline void yp_reassign(int index, struct ylog_poll *yp) {
    FILE *fp = yp->fp[index];
    yp->pfd[index].fd = fileno(fp);
    yp->pfd[index].events = POLLIN;
}

static inline int yp_isset(int index, struct ylog_poll *yp) {
    return yp->pfd[index].revents;
}

static inline int yp_isclosed(int index, struct ylog_poll *yp) {
    return yp->fp[index] == NULL;
}

static inline void yp_free(int index, struct ylog_poll *yp) {
    FILE *fp = yp_fp(index, yp);
    int fd = yp_fd(index, yp);
    if (fp) {
        fclose(fp);
    } else if (fd > 0) {
        CLOSE(fd);
    }
    yp->fp[index] = NULL;
    yp_clr(index, yp);
}

static inline void yp_set(FILE *fp, int fd, int index, struct ylog_poll *yp, char *mode) {
    if (fp == NULL) {
        fp = fdopen(fd, mode);
        if (fp == NULL) {
            ylog_error("fdopen failed: %s\n", strerror(errno));
            CLOSE(fd);
            return;
        }
    }
    if (fd < 0)
        fd = fileno(fp);
    yp->fp[index] = fp;
    yp->pfd[index].fd = fd;
    yp->pfd[index].events = POLLIN;
}

static inline int yp_insert(FILE *fp, int fd, int max, struct ylog_poll *yp, char *mode) {
    int i;
    for (i = 0; i < max; i++) {
        if (yp_isclosed(i, yp)) {
            yp_set(fp, fd, i, yp, mode);
            return i;
        }
    }
    return -1;
}
