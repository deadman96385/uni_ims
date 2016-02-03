/**
 * Copyright (C) 2016 Spreadtrum Communications Inc.
 */

#include <string.h>
#include <stdio.h>
#include <cutils/log.h>
#include "logEntryBase.h"

enum filter_state_t {
    NORMAL = 0,
    START,
};
#undef LOG_TAG
#define LOG_TAG "filteLog"

extern "C" int filter_log(char *line, int count, enum filter_state_t state) {
    if (state == START) {
        ALOGI("filter start");
    }else if (state == NORMAL) {
        ALOGI("filter new data");
    }
    return 0;
    if(0) {
       line = line;
       count = count;
       state = state;
    }
}
/*for test*/
int main(int /*argc*/, char** /*argv*/) {
    char buf[1024] = "01-05 19:40:51.740   245  1397 E audio_hw_primary: vbc_ctrl_voip_thread_routine open fail, pipe_name:/dev/spipe_lte4, 19.\n";
    int count = strlen(buf);
    filter_log(NULL, 0, START);
    filter_log(buf, count, NORMAL);
    return 0;
}
