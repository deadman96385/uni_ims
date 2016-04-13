/**
 * Copyright (C) 2016 Spreadtrum Communications Inc.
 */

#include <sys/param.h>
#include <sys/wait.h>
#include <sys/socket.h>

#include <signal.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <paths.h>

extern char **environ;

static pthread_mutex_t popen2_pidlist_mutex = PTHREAD_MUTEX_INITIALIZER;

static struct pid {
    struct pid *next;
    FILE *fp;
    pid_t pid;
} *pidlist;

FILE *popen2(char *command, char *type) {
    struct pid *cur;
    FILE *iop;
    int pdes[2], pid;
    int volatile twoway;
    struct pid *p;
    const char * volatile xtype = type;

    if (strchr(xtype, '+')) {
        twoway = 1;
        xtype = "r+";
        if (socketpair(AF_UNIX, SOCK_STREAM, 0, pdes) < 0)
            return (NULL);
    } else {
        twoway = 0;
        if ((xtype[0] != 'r' && xtype[0] != 'w') || xtype[1])
            return (NULL);
    }
    if (pipe(pdes) < 0)
        return (NULL);

    if ((cur = malloc(sizeof(struct pid))) == NULL) {
        (void)CLOSE(pdes[0]);
        (void)CLOSE(pdes[1]);
        errno = ENOMEM;
        return (NULL);
    }

    switch (pid = vfork()) {
    case -1:            /* Error. */
        (void)CLOSE(pdes[0]);
        (void)CLOSE(pdes[1]);
        free(cur);
        return (NULL);
        /* NOTREACHED */
    case 0:                /* Child. */
        if (xtype[0] == 'r') {
            /*
             * The _dup2() to STDIN_FILENO is repeated to avoid
             * writing to pdes[1], which might corrupt the
             * parent's copy.  This isn't good enough in
             * general, since the _exit() is no return, so
             * the compiler is free to corrupt all the local
             * variables.
             */
            (void)CLOSE(pdes[0]);
            if (pdes[1] != STDOUT_FILENO) {
                (void)dup2(pdes[1], STDOUT_FILENO);
                (void)CLOSE(pdes[1]);
                if (twoway)
                    (void)dup2(STDOUT_FILENO, STDIN_FILENO);
            } else if (twoway && (pdes[1] != STDIN_FILENO)) {
                (void)dup2(pdes[1], STDIN_FILENO);
            }
        } else {
            if (pdes[0] != STDIN_FILENO) {
                (void)dup2(pdes[0], STDIN_FILENO);
                (void)CLOSE(pdes[0]);
            }
            (void)CLOSE(pdes[1]);
        }
        for (p = pidlist; p; p = p->next) {
            (void)CLOSE(fileno(p->fp));
        }
        //execl(_PATH_BSHELL, "sh", "-c", command, NULL);
        /**
         * if we use sh -c, the pclose2 can't kill all
         * the child created by sh -c,
         * will became zombie process attached to init by luther
         */
        // execl(command, "111111111111111", NULL);
#if 0
        {
            char **env = environ;
                while (*env){
                    printf("environ %s\n", *env);
                    env++;
                }
        }
#endif
        // execle(command, basename(command), NULL, environ);
        {
            char tmp[1024];
            char *cmd;
            char *last;
            char *argv[100];
            int i;
            memset(argv, 0, sizeof argv);
            strcpy(tmp, command);
            cmd=strtok_r(tmp, " ", &last);
            argv[0] = cmd;
            for (i = 1; i < 100; i++) {
                argv[i] = strtok_r(NULL, " ", &last);
                if (argv[i] == NULL)
                    break;
            }
            execvp(cmd, argv);
        }
        _exit(127);
        /* NOTREACHED */
    }

    /* Parent; assume fdopen can't fail. */
    if (xtype[0] == 'r') {
        iop = fdopen(pdes[0], xtype);
        (void)CLOSE(pdes[1]);
    } else {
        iop = fdopen(pdes[1], xtype);
        (void)CLOSE(pdes[0]);
    }

    /* Link into list of file descriptors. */
    cur->fp = iop;
    cur->pid =  pid;
    pthread_mutex_lock(&popen2_pidlist_mutex);
    cur->next = pidlist;
    pidlist = cur;
    pthread_mutex_unlock(&popen2_pidlist_mutex);

    return (iop);
}

/**
 *    pclose2 returns -1 if stream is not associated with a `popened' command,
 *    if already `pclosed', or waitpid returns an error.
 */
int pclose2(FILE *iop) {
    register struct pid *cur, *last;
    int pstat;
    pid_t pid;
    int fd;

    pthread_mutex_lock(&popen2_pidlist_mutex);
    /* Find the appropriate file pointer. */
    for (last = NULL, cur = pidlist; cur; last = cur, cur = cur->next)
        if (cur->fp == iop)
            break;
    if (cur == NULL) {
        pthread_mutex_unlock(&popen2_pidlist_mutex);
        return (-1);
    }
    /* Remove the entry from the linked list. */
    if (last == NULL)
        pidlist = cur->next;
    else
        last->next = cur->next;
    pthread_mutex_unlock(&popen2_pidlist_mutex);

    fd = cur->pid;

    kill(fd, SIGKILL);
    //kill(fd, SIGINT);
    //kill(fd, SIGTERM);

    (void)fclose(iop);

    do {
        pid = waitpid(cur->pid, &pstat, 0);
    } while (pid == -1 && errno == EINTR);

    free(cur);

    return (pid == -1 ? -1 : pstat);
}
