/**
 * Copyright (C) 2016 Spreadtrum Communications Inc.
 */
#ifndef LOGENTRYBASE_H_
#define LOGENTRYBASE_H_

#include "log.h"
#include <string.h>
#include <vector>
#include <string>

namespace android {
typedef std::string string_t;
class logEntryBase {
    public:
        logEntryBase();
        ~logEntryBase();
        void logPraser(char *buf, int count, struct logEntry_t *logLine);
    protected:
        const char* logInfo;
    private:
        std::vector<std::string> split(string_t str, string_t pattern, string_t::size_type count);
        int decomposeLog(std::vector<string_t> logVec, struct logEntry_t *logLine);
        const char* buf;
};
}; //namespace android
#endif
