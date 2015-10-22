/* //vendor/sprd/proprietories-source/ril/sprd_rild/sprd_rild.c
**
** Copyright 2006, The Android Open Source Project
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/
#define LOG_TAG "RILD"
#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <utils/Log.h>
#include <cutils/properties.h>
#include <cutils/sockets.h>
#include <linux/capability.h>
#include <linux/prctl.h>
#include <telephony/sprd_ril.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <private/android_filesystem_config.h>

#define LIB_PATH_PROPERTY   "rild.libpath"
#define LIB_ARGS_PROPERTY   "rild.libargs"
#define MAX_LIB_ARGS        16
#define MAX_CAP_NUM         (CAP_TO_INDEX(CAP_LAST_CAP) + 1)
#define RIL_AT_TEST_PROPERTY  "persist.sys.sprd.attest"

static int modem;
#define RILLOGI(fmt, args...) ALOGI("[%c] " fmt, modem,  ## args)
#define RILLOGD(fmt, args...) ALOGD("[%c] " fmt, modem,  ## args)
#define RILLOGV(fmt, args...) ALOGV("[%c] " fmt, modem,  ## args)
#define RILLOGW(fmt, args...) ALOGW("[%c] " fmt, modem,  ## args)
#define RILLOGE(fmt, args...) ALOGE("[%c] " fmt, modem,  ## args)

static void usage(const char *argv0)
{
    fprintf(stderr, "Usage: %s -l <ril impl library> [-- <args for impl library>]\n", argv0);
    exit(EXIT_FAILURE);
}

extern void RIL_register (const RIL_RadioFunctions *callbacks, int argc, char ** argv);

extern void RIL_register_ATCIServer (RIL_RadioFunctions *(*Init)
        (const struct RIL_Env *, int, char **), RIL_SOCKET_TYPE socketType, int argc, char **argv);

extern void RIL_onRequestComplete(RIL_Token t, RIL_Errno e,
                           void *response, size_t responselen);

#if defined(ANDROID_MULTI_SIM)
extern void RIL_onUnsolicitedResponse(int unsolResponse, void *data,
                                size_t datalen, RIL_SOCKET_ID socket_id);
#else
extern void RIL_onUnsolicitedResponse(int unsolResponse, void *data,
                                size_t datalen);
#endif

extern void RIL_requestTimedCallback (RIL_TimedCallback callback,
                               void *param, const struct timeval *relativeTime);

extern void RIL_removeTimedCallback(void *callbackInfo);

static struct RIL_Env s_rilEnv = {
    RIL_onRequestComplete,
    RIL_onUnsolicitedResponse,
    RIL_requestTimedCallback,
    RIL_removeTimedCallback
};

extern void RIL_startEventLoop();

static int make_argv(char * args, char ** argv)
{
    // Note: reserve argv[0]
    int count = 1;
    char * tok;
    char * s = args;

    while ((tok = strtok(s, " \0"))) {
        argv[count] = tok;
        s = NULL;
        count++;
    }
    return count;
}

/*
 * switchUser - Switches UID to radio, preserving CAP_NET_ADMIN capabilities.
 * Our group, cache, was set by init.
 */
void switchUser() {
    char debuggable[PROP_VALUE_MAX];

    prctl(PR_SET_KEEPCAPS, 1, 0, 0, 0);
    setuid(AID_RADIO);

    struct __user_cap_header_struct header;
    memset(&header, 0, sizeof(header));
    header.version = _LINUX_CAPABILITY_VERSION_3;
    header.pid = 0;

    struct __user_cap_data_struct data[MAX_CAP_NUM];
    memset(&data, 0, sizeof(data));

    data[CAP_TO_INDEX(CAP_NET_ADMIN)].effective |= CAP_TO_MASK(CAP_NET_ADMIN);
    data[CAP_TO_INDEX(CAP_NET_ADMIN)].permitted |= CAP_TO_MASK(CAP_NET_ADMIN);

    data[CAP_TO_INDEX(CAP_NET_RAW)].effective |= CAP_TO_MASK(CAP_NET_RAW);
    data[CAP_TO_INDEX(CAP_NET_RAW)].permitted |= CAP_TO_MASK(CAP_NET_RAW);

    data[CAP_TO_INDEX(CAP_BLOCK_SUSPEND)].effective |= CAP_TO_MASK(CAP_BLOCK_SUSPEND);
    data[CAP_TO_INDEX(CAP_BLOCK_SUSPEND)].permitted |= CAP_TO_MASK(CAP_BLOCK_SUSPEND);

    if (capset(&header, &data[0]) == -1) {
        RLOGE("capset failed: %s", strerror(errno));
        exit(EXIT_FAILURE);
    }

    /*
     * Debuggable build only:
     * Set DUMPABLE that was cleared by setuid() to have tombstone on RIL crash
     */
    property_get("ro.debuggable", debuggable, "0");
    if (strcmp(debuggable, "1") == 0) {
        prctl(PR_SET_DUMPABLE, 1, 0, 0, 0);
    }
}

int main(int argc, char **argv)
{
    const char * rilLibPath = NULL;
    const char * rilModem = NULL;
    const char * rilCurrentSim = NULL;
    char **rilArgv;
    void *dlHandle;
    const RIL_RadioFunctions *(*rilInit)(const struct RIL_Env *, int, char **);
    const RIL_RadioFunctions *(*rilATCIInit)(const struct RIL_Env *, int, char **);
    char *err_str = NULL;
    const RIL_RadioFunctions *funcs;
    unsigned char hasLibArgs = 0;
    char cmdline[1024];
    int i, rc, fd;
    int califlag = 0;
    char prop[PROPERTY_VALUE_MAX];

    for (i = 1; i < argc ;) {
        if (0 == strcmp(argv[i], "-l") && (argc - i > 1)) {
            rilLibPath = argv[i + 1];
            i += 2;
        } else if (0 == strcmp(argv[i], "-m") && (argc - i > 1)) {
            rilModem = argv[i + 1];
            i += 2;
        } else if (0 == strcmp(argv[i], "-n") && (argc - i > 1)) {
            rilCurrentSim = argv[i + 1];
            modem = *argv[i + 1];
            break;
        } else {
            usage(argv[0]);
        }
    }

    rilArgv = argv + 2;
    argc = argc - 2;

    if (rilModem == NULL) {
        ALOGD("rilModem == NULL  exit");
        exit(EXIT_FAILURE);
    }

    //modem = *rilModem;

    memset(cmdline, 0, 1024);
    fd = open("/proc/cmdline", O_RDONLY);
    if(fd > 0) {
        rc = read(fd, cmdline, sizeof(cmdline));
        if(rc > 0) {
            if(strstr(cmdline, "calibration") != NULL)
            califlag = 1;
        }
        close(fd);
    }

    if(califlag == 1) {
        RILLOGD("RIL: Calibration mode,RIL goto sleep!\n");
        goto done;
    }

    property_get(RIL_AT_TEST_PROPERTY, prop, "0");
    if(!strcmp(prop, "1")) {
        RILLOGD("RIL: AT test mode,RIL goto sleep!\n");
        goto done;
    }

OpenLib:
    switchUser();

    dlHandle = dlopen(rilLibPath, RTLD_NOW);

    if (dlHandle == NULL) {
        fprintf(stderr, "dlopen failed: %s\n", dlerror());
        exit(EXIT_FAILURE);
    }

    RIL_startEventLoop();

    rilInit = (const RIL_RadioFunctions *(*)(const struct RIL_Env *, int, char **))dlsym(dlHandle, "RIL_Init");

    if (rilInit == NULL) {
        fprintf(stderr, "RIL_Init not defined or exported in %s\n", rilLibPath);
        exit(EXIT_FAILURE);
    }

    dlerror(); // Clear any previous dlerror
    rilATCIInit =
        (const RIL_RadioFunctions *(*)(const struct RIL_Env *, int, char **))
        dlsym(dlHandle, "RIL_ATCI_Init");

    err_str = dlerror();
    if (err_str) {
        RLOGW("RIL_ATCI_Init not defined or exported in %s: %s\n", rilLibPath, err_str);
    } else if (!rilATCIInit) {
        RLOGW("RIL_ATCI_Init defined as null in %s. AT Channel Not usable\n", rilLibPath);
    }

    RILLOGD("Rild: rilArgv[1]=%s,rilArgv[2]=%s,rilArgv[3]=%s,rilArgv[4]=%s",rilArgv[1],rilArgv[2],rilArgv[3],rilArgv[4]);
    funcs = rilInit(&s_rilEnv, argc, rilArgv);
    RIL_register(funcs, argc, rilArgv);

    if (rilATCIInit) {
        RLOGD("RIL_register_ATCIServer started");
        RIL_register_ATCIServer(rilATCIInit, RIL_ATCI_SOCKET, argc, rilArgv);
    }
    RLOGD("RIL_register_socket completed");

done:

    RLOGD("RIL_Init starting sleep loop");
    while (true) {
        sleep(UINT32_MAX);
    }
}

