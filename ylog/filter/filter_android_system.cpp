/**
 * Copyright (C) 2016 Spreadtrum Communications Inc.
 */

#include <string.h>
#include <stdio.h>
#include <cutils/log.h>
#include "logEntryBase.h"
#include "log.h"
#include <vector>

#define LOG_LEN_MAX 1024

using android::logEntryBase;
enum filter_state_t {
    NORMAL = 0,
    START,
};

struct logTagTable{
    const char* logTag;
    unsigned int hashId;
    unsigned int count;
};

// BKDR Hash Function
static unsigned int BKDRHash(const char *str)
{
    unsigned int seed = 131; // 31 131 1313 13131 131313 etc..
    unsigned int hash = 0;

    while (*str)
    {
        hash = hash * seed + (*str++);
    }

    return (hash & 0x7FFFFFFF);
}


extern "C" int filter_log(char *line, int count, enum filter_state_t state) {
    logEntryBase log;
    struct logEntry_t logLine;
    unsigned int hashId = 0;
    if (state == START) {
        ALOGI("filter start\n");
    }else if (state == NORMAL) {
        log.logPraser(line, count, &logLine);
        hashId = BKDRHash(logLine.tag);
        ALOGI("hashId of %s is %u \n", logLine.tag, hashId);
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
    char buf[LOG_LEN_MAX] = "01-02 08:16:23.053 11986 11986 W YLOG    : [01-02 08:16:23.052] ylog<info> Success: insert ydst android/";
    int count = strlen(buf);
    filter_log(NULL, 0, START);
    filter_log(buf, count, NORMAL);
    printf("finished\n");
    return 0;
}
