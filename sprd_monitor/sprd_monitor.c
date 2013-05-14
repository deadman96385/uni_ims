#define LOG_TAG "sprd_monitor"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <dirent.h>
#include <ctype.h>
#include <cutils/properties.h>
#include <utils/Log.h>

#define DEBUG 1

#ifdef DEBUG
#define MONITOR_LOGD(x...) ALOGD( x )
#define MONITOR_LOGE(x...) ALOGE( x )
#else
#define MONITOR_LOGD(x...) do {} while(0)
#define MONITOR_LOGD(x...) do {} while(0)
#endif

#define PROP_PHONE_COUNT  "persist.msms.phone_count"
#define PHONE_APP   "com.android.phone"
#define PROP_TTYDEV  "persist.ttydev"
#define PHONE_APP_PROP  "sys.phone.app"

static char ttydev[12];
static int ttydev_fd;

static int s_multiSimMode = 0;

/* helper function to get pid from process name */
static int get_task_pid(char *name)
{
    DIR *d;
    struct dirent *de;
    char cmdline[1024];

    d = opendir("/proc");
    if (d == 0) return -1;

    while ((de = readdir(d)) != 0) {
        if(isdigit(de->d_name[0])) {
            int pid = atoi(de->d_name);
            int fd, ret;
            sprintf(cmdline, "/proc/%d/cmdline", pid);
            fd = open(cmdline, O_RDONLY);
            if (fd < 0) continue;
            ret = read(fd, cmdline, 1023);
            close(fd);
            if (ret < 0) ret = 0;
            cmdline[ret] = 0;
            if (strcmp(name, cmdline) == 0) {
                closedir(d);
                return pid;
            }
        }
    }
    closedir(d);
    return -1;
}


static int kill_nvitemd(void)
{
    property_set("ctl.stop", "nvitemd");

    return 0;
}

static int start_nvitemd(void)
{
    property_set("ctl.start", "nvitemd");

    return 0;
}

static int kill_engservice(void)
{
    property_set("ctl.stop", "engservicew");
    property_set("ctl.stop", "engmodemclientw");
    property_set("ctl.stop", "engpcclientw");

    return 0;
}

static int start_engservice(void)
{
    property_set("ctl.start", "engservicew");
    property_set("ctl.start", "engmodemclientw");
    property_set("ctl.start", "engpcclientw");

    return 0;
}

static int start_phser()
{
    property_set("ctl.start", "phoneserver_w");

    return 0;
}

static int kill_phser()
{
    property_set("ctl.stop", "phoneserver_w");

    return 0;
}

static int stop_rild(void)
{
	/* stop rild */
    if(s_multiSimMode == 3) {
        property_set("ctl.stop", "wril-daemon");
        property_set("ctl.stop", "wril-daemon1");
        property_set("ctl.stop", "wril-daemon2");
    } else if(s_multiSimMode == 2) {
        property_set("ctl.stop", "wril-daemon");
        property_set("ctl.stop", "wril-daemon1");
    } else {
        property_set("ctl.stop", "wril-daemon");
    }

    return 0;
}

static int start_rild(void)
{
    /* start rild */
    if(s_multiSimMode == 3) {
        property_set("ctl.start", "wril-daemon");
        property_set("ctl.start", "wril-daemon1");
        property_set("ctl.start", "wril-daemon2");
    } else if(s_multiSimMode == 2) {
        property_set("ctl.start", "wril-daemon");
        property_set("ctl.start", "wril-daemon1");
    } else {
        property_set("ctl.start", "wril-daemon");
    }

    return 0;
}

void assert_handler(int sig_num)
{
    pid_t pid;
    char pid_str[32] = {0};

    kill_nvitemd();

    kill_engservice();

    /*kill phoneserver*/
    MONITOR_LOGD("kill phoneserver!");
    kill_phser();

    /* close ttydev */
    if (ttydev_fd >= 0)
        close(ttydev_fd);

    MONITOR_LOGD("stop rild!\n");
    stop_rild();

    /*kill com.android.phone*/
    pid = get_task_pid(PHONE_APP);
    MONITOR_LOGD("restart %s (%d)!\n", PHONE_APP, pid);
    if (pid > 0) {
        snprintf(pid_str, sizeof(pid_str), "%d", pid);
        property_set(PHONE_APP_PROP, pid_str);
        property_set("ctl.start", "kill_phone");
    }
}

void reset_handler(int sig_num)
{
    char mux_2sim_mode[]="echo 1 > /proc/mux_mode";
    char mux_3sim_mode[]="echo 2 > /proc/mux_mode";
    char path[32];

    /* open ttydev */
    sprintf(path, "/dev/%s", ttydev);
    ttydev_fd = open(path, O_RDWR);
    if (ttydev_fd < 0)
        MONITOR_LOGE("Failed to open %s!\n", path);

    if(s_multiSimMode == 3) {
        MONITOR_LOGD("enter %d sim card mode!\n", s_multiSimMode);
        system(mux_3sim_mode);
    } else if(s_multiSimMode == 2) {
        MONITOR_LOGD("enter %d sim card mode!\n", s_multiSimMode);
        system(mux_2sim_mode);
    }

    /*make sure the condition mux needed is OK*/
    sleep(1);

    /*start phoneserver*/
    MONITOR_LOGD("restart phoneserver!\n");
    start_phser();

    sleep(1);

    start_engservice();

    MONITOR_LOGD("restart rild!\n");
    start_rild();

    /* restart mediaserver */
    MONITOR_LOGD("restart mediaserver");
    property_set("ctl.restart", "media");

    sleep(2);

    start_nvitemd();
}

int main(int argc, char **argv)
{
    struct sigaction act;
    struct sigaction reset_act;
    int ret;
    int str;
    char mux_2sim_mode[]="echo 1 > /proc/mux_mode";
    char mux_3sim_mode[]="echo 2 > /proc/mux_mode";
    char phoneCount[5];
    char path[32];

    if(0 == property_get(PROP_PHONE_COUNT, phoneCount, "1")) {
        s_multiSimMode = 1;
    } else {
        if(!strcmp(phoneCount, "3"))
            s_multiSimMode = 3;
        else if(!strcmp(phoneCount, "2"))
            s_multiSimMode = 2;
        else
            s_multiSimMode = 1;
    }

    property_get(PROP_TTYDEV, ttydev, "ttyNK3");

    memset (&act, 0x00, sizeof(act));
    act.sa_handler = &assert_handler;
    act.sa_flags = SA_NODEFER;
    sigfillset(&act.sa_mask);	//block all signals when handler is running.
    ret = sigaction (SIGUSR1, &act, NULL);
    if (ret < 0) {
        perror("sigaction() failed!\n");
        exit(1);
    }

    memset (&reset_act, 0x00, sizeof(reset_act));
    reset_act.sa_handler = &reset_handler;
    reset_act.sa_flags = SA_NODEFER;
    sigfillset(&reset_act.sa_mask);	//block all signals when handler is running.
    ret = sigaction (SIGUSR2, &reset_act, NULL);
    if (ret < 0) {
        perror("sigaction() failed!\n");
        exit(1);
    }

    sprintf(path, "/dev/%s", ttydev);
    ttydev_fd = open(path, O_RDWR);
    if (ttydev_fd < 0)
        MONITOR_LOGE("Failed to open %s!\n", path);

    if(s_multiSimMode == 3) {
        MONITOR_LOGD("enter %d sim card mode!\n", s_multiSimMode);
        system(mux_3sim_mode);
    } else if(s_multiSimMode == 2) {
        MONITOR_LOGD("enter %d sim card mode!\n", s_multiSimMode);
        system(mux_2sim_mode);
    }

    /*make sure the condition mux needed is OK*/
    sleep(1);

    MONITOR_LOGD("start phoneserver!\n");
    /*start phoneserver*/
    start_phser();

    MONITOR_LOGD("start rild!\n");
    start_rild();

    start_engservice();

    sleep(1);

    start_nvitemd();

    while(1) pause();
}
