#define LOG_TAG    "RILProxy"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <cutils/properties.h>
#include <utils/Log.h>
#include <cutils/sockets.h>
#include <linux/capability.h>
#include <linux/prctl.h>
#include "rilproxy.h"
#include <private/android_filesystem_config.h>

/*
 * switchUser - Switches UID to radio, preserving CAP_NET_ADMIN capabilities.
 * Our group, cache, was set by init.
 */
void switchUser() {
    prctl(PR_SET_KEEPCAPS, 1, 0, 0, 0);
    setuid(AID_RADIO);

    struct __user_cap_header_struct header;
    struct __user_cap_data_struct cap;
    header.version = _LINUX_CAPABILITY_VERSION;
    header.pid = 0;
    cap.effective = cap.permitted = (1 << CAP_NET_ADMIN) | (1 << CAP_NET_RAW);
    cap.inheritable = 0;
    capset(&header, &cap);
}


int main(int argc, char *argv[])
{
    pthread_t tid, lte_tid, tdg_tid, lte_server_tid;

    switchUser();  
    rilproxy_init();
#if 0
    if (pthread_create(&tid, NULL, rilproxy_client, NULL) < 0) {
         ALOGE("Create rilproxy client failure, exit");
         exit(0);
    }
#else
    if (is_svlte()) {
        if (pthread_create(&lte_tid, NULL, rilproxy_client, LTE_RILD_SOCKET_NAME) < 0) {
            ALOGE("Failded to Create rilproxy client LTE thread, exit");
            exit(0);
        }
    }
    if (pthread_create(&tdg_tid, NULL, rilproxy_client, TDG_RILD_SOCKET_NAME) < 0) {
         ALOGE("Failded to Create rilproxy client TD/G thread, exit");
         exit(0);
    }
#endif
#if 0
    /***************************/
    /* Add for dual signal bar */
    /***************************/
    if (pthread_create(&lte_server_tid, NULL, (void*)rilproxy_lte_server, NULL) < 0) {
         ALOGE("Failded to Create rilproxy lte server thread, exit");
         exit(0);
    }
#endif
    rilproxy_server();
    return 0;
}

