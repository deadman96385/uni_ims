/**
 * Copyright (C) 2016 Spreadtrum Communications Inc.
 */
#ifndef LOGENTRYBASE_H_
#define LOGENTRYBASE_H_

#include <log/logprint.h>
#include <string.h>

#define LOG_LEN_MAX 1024
namespace android {
struct logEntry_t {
    AndroidLogEntry log;
    struct LogEntry_t *Next;
};

class logEntryBase {
    public:
       logEntryBase();
       ~logEntryBase();
       void logPraser(char *buf, int count, struct logEntry_t logLine);
    protected:
       const char* logInfo;
    private:
       const char* buf;
};
}; //namespace android
#endif
