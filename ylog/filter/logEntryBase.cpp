/**
 * Copyright (C) 2016 Spreadtrum Communications Inc.
 */

#include "logEntryBase.h"
#include <log/logprint.h>
#include <string.h>

namespace android {

logEntryBase::logEntryBase() {

}

logEntryBase::~logEntryBase() {

}

void logEntryBase::logPraser(char *buf, int count, struct logEntry_t logLine) {

    char subStr = ' ';

    while (1) {
        char *s = strstr(buf,&subStr);
        printf("%s\n", s);
        
    }

    if(0) {
       buf = buf;
       count = count;
       logLine = logLine;
    }
}

} //namespace android
