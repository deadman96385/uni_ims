/**
 * Copyright (C) 2016 Spreadtrum Communications Inc.
 */

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>

static pthread_mutex_t update_config_mutex = PTHREAD_MUTEX_INITIALIZER;

#define T_EOF 0
#define T_TEXT 1
#define T_NEWLINE 2

#define INIT_PARSER_MAXARGS 64

struct parse_state {
    char *ptr;
    char *text;
    int line;
    int nexttoken;
    const char *filename;
};

static int next_token(struct parse_state *state) {
    char *x = state->ptr;
    char *s;

    if (state->nexttoken) {
        int t = state->nexttoken;
        state->nexttoken = 0;
        return t;
    }

    for (;;) {
        switch (*x) {
            case 0:
                state->ptr = x;
                return T_EOF;
            case '\n':
                x++;
                state->ptr = x;
                return T_NEWLINE;
            case ' ':
            case '\t':
            case '\r':
                x++;
                continue;
            case '#':
                while (*x && (*x != '\n')) x++;
                if (*x == '\n') {
                    state->ptr = x+1;
                    return T_NEWLINE;
                } else {
                    state->ptr = x;
                    return T_EOF;
                }
            default:
                goto text;
        }
    }

textdone:
    state->ptr = x;
    *s = 0;
    return T_TEXT;
text:
    state->text = s = x;
textresume:
    for (;;) {
        switch (*x) {
            case 0:
                goto textdone;
            case ' ':
            case '\t':
            case '\r':
                x++;
                goto textdone;
            case '\n':
                state->nexttoken = T_NEWLINE;
                x++;
                goto textdone;
            case '"':
                x++;
                for (;;) {
                    switch (*x) {
                        case 0:
                            /* unterminated quoted thing */
                            state->ptr = x;
                            return T_EOF;
                        case '"':
                            x++;
                            goto textresume;
                        default:
                            *s++ = *x++;
                    }
                }
                break;
            case '\\':
                x++;
                switch (*x) {
                    case 0:
                        goto textdone;
                    case 'n':
                        *s++ = '\n';
                        break;
                    case 'r':
                        *s++ = '\r';
                        break;
                    case 't':
                        *s++ = '\t';
                        break;
                    case '\\':
                        *s++ = '\\';
                        break;
                    case '\r':
                        /* \ <cr> <lf> -> line continuation */
                        if (x[1] != '\n') {
                            x++;
                            continue;
                        }
                    case '\n':
                        /* \ <lf> -> line continuation */
                        state->line++;
                        x++;
                        /* eat any extra whitespace */
                        while((*x == ' ') || (*x == '\t')) x++;
                        continue;
                    default:
                        /* unknown escape -- just copy */
                        *s++ = *x++;
                }
                continue;
            default:
                *s++ = *x++;
        }
    }
    return T_EOF;
}

static char *ylog_load_config(const char *path) {
    char *data;
    struct stat sb;
    int fd = open(path, O_RDONLY | O_NOFOLLOW | O_CLOEXEC);

    if (fd == -1) {
        return NULL;
    }

    if (fstat(fd, &sb) == -1) {
        ylog_error("fstat failed for '%s': %s\n", path, strerror(errno));
        CLOSE(fd);
        return NULL;
    }

#if 0
    if ((sb.st_mode & (S_IWGRP | S_IWOTH)) != 0) {
        ylog_error("skipping insecure file '%s'\n", path);
        return NULL;
    }
#endif

    data = malloc(sb.st_size + 64);
    if (data) {
        int ret;
        char *p = data;
        char *pmax = p + sb.st_size;
        do {
            ret = read(fd, p, pmax - p);
            if (ret > 0)
                p += ret;
            else
                break;
        } while (p < pmax);
    }
    CLOSE(fd);

    return data;
}

static void ylog_update_config(const char *fn, int u_nargs, char **u_args, int un) {
    char *args[INIT_PARSER_MAXARGS];
    char tempPath[PATH_MAX];
    struct parse_state state;
    int nargs = 0;
    int wfd = 0;
    int found = 0;
    char *mptr;
    char *desc = "ylog_update_config";

    pthread_mutex_lock(&update_config_mutex);
    if (access(fn, F_OK)) {
        wfd = open(fn, O_RDWR | O_CREAT | O_TRUNC, 0644);
        /* ftruncate(wfd, 0); */
        if (wfd >= 0)
            CLOSE(wfd);
    }

    state.filename = fn;
    state.line = 0;
    state.ptr = mptr = ylog_load_config(fn);
    state.nexttoken = 0;

    if (state.ptr == NULL) {
        ylog_critical("%s can't malloc memory\n", __func__);
        pthread_mutex_unlock(&update_config_mutex);
        return;
    }

    snprintf(tempPath, sizeof(tempPath), "%s.temp.XXXXXX", fn);
    wfd = mkstemp(tempPath);
    if (wfd < 0) {
        free(mptr);
        ylog_error("Unable to write new ylog config to temp file %s errno: %d\n", tempPath, errno);
        pthread_mutex_unlock(&update_config_mutex);
        return;
    }

    for (;;) {
        switch (next_token(&state)) {
            case T_EOF:
                // process_keyword(&state, nargs, args);
                goto parser_done;
            case T_NEWLINE:
                state.line++;
                if (nargs) {
                    char **s_args;
                    int s_nargs;
                    int i = -1;

                    if (un <= nargs)
                        for (i = 0; i < un && strcmp(args[i], u_args[i]) == 0; i++);

                    if (i == un) {
                        s_args = u_args;
                        s_nargs = u_nargs;
                        found = 1;
                    } else {
                        s_args = args;
                        s_nargs = nargs;
                    }

                    for (i = 0; i < s_nargs; i++) {
                        fd_write(s_args[i], strlen(s_args[i]), wfd, desc);
                        if (i != (s_nargs - 1))
                            fd_write(" ", 1, wfd, desc);
                    }
                    fd_write("\n", 1, wfd, desc);

                    nargs = 0;
                }
                break;
            case T_TEXT:
                if (nargs < INIT_PARSER_MAXARGS) {
                    args[nargs++] = state.text;
                }
                break;
        }
    }

parser_done:
    if (found == 0) {
        int i;
        LSEEK(wfd, 0, SEEK_END);
        for (i = 0; i < u_nargs; i++) {
            fd_write(u_args[i], strlen(u_args[i]), wfd, desc);
            fd_write(" ", 1, wfd, desc);
        }
        fd_write("\n", 1, wfd, desc);
    }
    free(mptr);
    if (fchmod(wfd, 0644))
        ylog_error("Unable to chmod ylog config file %s to 0644\n", fn);
    fsync(wfd);
    CLOSE(wfd);
    if (rename(tempPath, fn)) {
        unlink(tempPath);
        ylog_error("Unable to rename ylog config file %s to %s\n", tempPath, fn);
    }
    pthread_mutex_unlock(&update_config_mutex);
}

static void ylog_update_config2(char *key, char *value) {
    char *argv[2];
    argv[0] = key;
    argv[1] = value;
    ylog_update_config(global_context->ylog_config_file, 2, argv, 1);
}

void process_keyword(struct parse_state *state, int nargs, char **args);
static void parse_config(const char *fn) {
    char *args[INIT_PARSER_MAXARGS];
    struct parse_state state;
    int nargs = 0;
    char *mptr;

    if (access(fn, F_OK)) {
        ylog_info("%s does not exist\n", fn);
        return;
    }

    state.filename = fn;
    state.line = 0;
    state.ptr = mptr = ylog_load_config(fn);
    state.nexttoken = 0;

    if (state.ptr == NULL) {
        ylog_critical("%s can't malloc memory\n", __func__);
        return;
    }

    for (;;) {
        switch (next_token(&state)) {
            case T_EOF:
                // process_keyword(&state, nargs, args);
                goto parser_done;
            case T_NEWLINE:
                state.line++;
                if (nargs) {
                    process_keyword(&state, nargs, args);
                    nargs = 0;
                }
                break;
            case T_TEXT:
                if (nargs < INIT_PARSER_MAXARGS) {
                    args[nargs++] = state.text;
                }
                break;
        }
    }

parser_done:
    free(mptr);
}

void process_keyword(struct parse_state *state, int nargs, char **args) {
    UNUSED(state);
    struct ylog_keyword *kw;
    for (kw = global_context->ylog_keyword; kw->key; kw++) {
        if (strcmp(args[0], kw->key) == 0) {
            if (kw->handler)
                kw->handler(kw, nargs, args);
            break;
        }
    }
}
