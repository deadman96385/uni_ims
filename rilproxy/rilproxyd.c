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
#include <sqlite3.h>


static int intcallback = 0;
static int str2intcallback(void* param,int argc,char** argv,char** cname)
{
    int *sqlresult = (int *)param;
    if (argc == 1) {
        intcallback = 1;
        ALOGD(" value=%s\n", argv[0]);
        *sqlresult = atoi(argv[0]);
    }
    return 0;
}

static int query_int_form_db(const char* dbname, const char *table, char* name)
{
    sqlite3 *db=NULL;
    char *errmsg=NULL;
    char sqlbuf[128];
    int rc, ret=0;
    int value;

    rc = sqlite3_open(dbname, &db);
    if(rc != 0) {
        ALOGE("open %s fail [%d:%s]", dbname, sqlite3_errcode(db), sqlite3_errmsg(db));
        ret = 0;
        goto out;
    } else {
        ALOGD("open %s success.", dbname);
    }

    memset(sqlbuf, 0, sizeof(sqlbuf));
    sprintf(sqlbuf, "SELECT value FROM %s WHERE name='%s'", table, name);
    ALOGD("sql query = %s", sqlbuf);

    intcallback = 0;
    rc=sqlite3_exec(db,sqlbuf,&str2intcallback,&value,&errmsg);
    if (rc != 0) {
        ALOGE(" select table fail, errmsg=%s [%d:%s]", errmsg, sqlite3_errcode(db), sqlite3_errmsg(db));
        ret = 0;
        goto out;
    }

    if (intcallback == 1) {
        ret = value;
    }

    ALOGD("select table global success, ret=0x%x", ret);

out:
    sqlite3_close(db);
    return ret;
}


static int get_airplane_mode(void) {

    const char *dbname="/data/data/com.android.providers.settings/databases/settings.db";

    return query_int_form_db(dbname, "global", "airplane_mode_on");
}

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
    int airplanemode;

    airplanemode = get_airplane_mode();
    set_lte_radio_on(airplanemode == 0);

    switchUser();  
    rilproxy_init();

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

    rilproxy_server();

    return 0;
}

