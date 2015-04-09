/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2005-2008 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 11816 $ $Date: 2010-04-16 06:36:12 +0800 (五, 16  4月 2010) $
 *
 */

#include <stdio.h>
#include <pthread.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "vport_revinfo.c"
#include "system_start.h"

extern int  VAPP_main(int arg, char *argv);
extern int  VAPP_mainControl(void *context_ptr);

/* ======== D2_getLine ========
 * get user input by char with echo
 */
int D2_getLine(
    char *buf, 
    unsigned int max) 
{ 
    int           c;
    int           index;

    index = 0;
    max--;
    while (EOF != (c = fgetc(stdin))) { 
        if (((0x7f == c) || ('\b' == c))) { 
            if (index) { 
                printf("\b \b");
                *(buf + index) = 0;
                index--;
            }
            continue;
        }
        if (('\n' == c) || ('\r' == c)) { 
            printf("\n");
            break;
        }
        if (index >= max) { 
            continue;
        }
        printf("%c", c);
        *(buf + index) = c;
        index++;
    }
    *(buf + index) = 0;
    return (index);
}


/* 
 * ======== D2_writeToNvMem ===================
 * This function writes board configuration to the NV memory / file.
 *
 * Returns:
 *  0: Success
 *  -1: Fail
 */   
vint D2_writeToNvMem(
    void *data_ptr)
{

    return (0);
}


/* 
 * ======== D2_readFromNvMem ===================
 * This function reads from NV memory / file, the board configuration data.
 *
 * Returns:
 *  0: Success
 *  -1: Fail
 */   
vint D2_readFromNvMem(
    void *data_ptr,
    char *cFile_ptr)
{
    return (0);
}

/*
 * ======== daemonize ========
 *
 * Unix standard start process for creating a daemon
 */
void daemonize(void)
{
    int i,lfp;
    char str[10];

#if 0 /* XXX BUG1029 */
    if((i=getppid())==1) return; /* already a daemon */

    i=fork();
    if (i<0) exit(1); /* fork error */
    if (i>0) exit(0); /* parent exits */
    printf("child pid=%d\n",getpid());
    /* child (daemon) continues */
    setsid(); /* obtain a new process group */
    i=fork(); /* fork again as to never control a terminal */
    if (i<0) exit(1); /* fork error */
    if (i>0) exit(0); /* parent exits */
#endif
    chdir("/usr/d2"); /* change running directory */
    umask(0600); /* set newly created file permissions */
#ifndef VAPP_MGMT_PRINT_TO_CONSOLE
    for (i=getdtablesize();i>=0;--i) OSAL_netCloseSocket(&i); /* close all descriptors */
    i=open("/dev/null",O_RDONLY); /* stdin */
    if (-1 == i) exit(3);   /* can not open */
    i=open("/tmp/vapp_log.txt",O_WRONLY|O_CREAT, 0644);  /* stdout */
    if (-1 == i) exit(4);   /* can not open */
    dup(i); /* stderr */
#endif
    i=open("/tmp/vapp_vers.txt",O_RDWR|O_CREAT,0644); /* version data */
    str[0] = '\n';
    str[1] = 0;
    write(i, str, 1);
    write(i, D2_Release_VPORT, sizeof(D2_Release_VPORT));
    write(i, str, 1);
    OSAL_netCloseSocket(&i);
    lfp=open("/tmp/vapp_cfg.pid",O_RDWR|O_CREAT,0644);
    if (lfp<0) exit(1); /* can not open */
    if (lockf(lfp,F_TLOCK,0)<0) exit(5); /* can not lock */
    /* first instance continues */
    sprintf(str,"%d\n",getpid());
    write(lfp,str,OSAL_strlen(str)); /* record pid to lockfile */
    signal(SIGCHLD,SIG_IGN); /* ignore child */
    signal(SIGTSTP,SIG_IGN); /* ignore tty signals */
    signal(SIGTTOU,SIG_IGN);
    signal(SIGTTIN,SIG_IGN);
    signal(SIGUSR1,SIG_IGN);
    signal(SIGUSR2,SIG_IGN);
}

int VAPP_mainControl(
    void *context_ptr) 
{
#ifdef VAPP_MGMT
    FILE            *fin;
    char             buf[25];
    int              port;
    vint             size;
    OSAL_NetSockId   sockFd;
    OSAL_NetAddress  addr;

    OSAL_netSocket(&sockFd, OSAL_NET_SOCK_TCP);
    if (sockFd < 0) { 
        return (-2);
    }
    if (NULL == (fin = fopen(VAPP_CFG_PORT_PATHNAME, "r"))) { 
        fprintf(stderr, "%s: vapp_cfg.port file error\n", __FUNCTION__);
        return (-1);
    }
    OSAL_memSet(buf, 0, sizeof(buf));
    fgets(buf, sizeof(buf) - 1, fin);
    fclose(fin);
    buf[sizeof(buf)-1] = 0;
    port = atoi(buf);
    OSAL_netStringToAddress("127.0.0.1", &addr);
    addr.port        = OSAL_netHtons(port);
    if (OSAL_SUCCESS != OSAL_netConnectSocket(&sockFd, &addr)) { 
        fprintf(stderr, "%s: Connect error\n", __FUNCTION__);
        return (-3);
    }
    fprintf(stderr, "%s: Connected to %x:%d\n", __FUNCTION__, 
            OSAL_netNtohl(addr.ipv4), port);
    OSAL_memSet(buf, 0, sizeof(buf));
    OSAL_strcpy(buf, (char *)context_ptr);
    size = sizeof(buf);
    OSAL_taskDelay(3000);
    if (OSAL_SUCCESS != OSAL_netSocketSend(&sockFd, buf, &size)) { 
        fprintf(stderr, "%s: write error\n", __FUNCTION__);
        OSAL_netCloseSocket(&sockFd);
        return (-4);
    }
    fprintf(stderr, "%s: sent: %s on port:%d\n", __FUNCTION__, buf, port);
    OSAL_netCloseSocket(&sockFd);
#endif
    return (0);
}

/*
 * ======== main ========
 * Unix main
 */
int main(
    int   argc, 
    char *argv[])
{

    int console;
    int argn;
#ifdef VAPP_MAIN
    char  buf[25];
#endif
   
    console = 1;
    argn = 1;

#ifdef VAPP_MAIN
    if (argc > 1) { 
        /* -r = reinit */
        if (('-' == argv[argn][0]) && ('r' == argv[argn][1])) {
            argn++;
            OSAL_strcpy(buf, "reinit ");
            OSAL_strcpy(&buf[7], argv[argn]);
            return (VAPP_mainControl(buf));
        }
        /* -S = shutdown */
        else if (('-' == argv[argn][0]) && ('S' == argv[argn][1])) { 
            return (VAPP_mainControl("shutdown"));
        }
        else if (('-' == argv[argn][0]) && ('i' == argv[argn][1])) { 
            /* interactive mode */
            console = 1;
            argn++;
        }
        else if (('-' == argv[argn][0]) && ('b' == argv[argn][1])) { 
            /* background mode */
            console = 0;
            argn++;
        }
        else {
            printf("%s: [-rSib]\n", argv[0]);
            printf("%s: r=restart, S=shutdown\n"
                    " i=interactive, b=background\n",
                    argv[0]);
            return (-1);
        }
    }
#endif

    fprintf(stderr,
"\n\n========\nvPort Release %s\n========\n"
"\n"
"               D2 Technologies\n"
"      _   _  __                           \n"
"     / | '/  /_  _  /_ _  _  /_  _  ._   _\n"
"    /_.'/_  //_'/_ / // //_///_//_///_'_\\ \n"
"                                _/        \n"
"\n"
"       Unified Communication Products\n"
"\n"
"                 www.d2tech.com\n"
"\n", D2_Release_VPORT);


    if (!console) {
        daemonize();
    }
#ifdef VAPP_MAIN
    VAPP_main(0, argv[argn]);
#else
#warning XXX Not compiling with VAPP_main()
#endif

    {
        pthread_mutex_t shutdown_mutex; 
        pthread_cond_t shutdown_cond; 
        pthread_mutex_init(&shutdown_mutex, NULL);
        pthread_cond_init(&shutdown_cond, NULL);
        pthread_mutex_lock(&shutdown_mutex);
        pthread_cond_wait(&shutdown_cond, &shutdown_mutex);
    }

    return (0);
}
