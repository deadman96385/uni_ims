/**
 * Copyright (C) 2016 Spreadtrum Communications Inc.
 */

#include "logEntryBase.h"
#include <log/logprint.h>
#include <string.h>

namespace android {

std::vector<string_t> logEntryBase::split(string_t str, string_t pattern, string_t::size_type count)
{
    string_t::size_type pos = 0, pos_new, size, i;
    string_t s;
    std::vector<string_t> result;
    str += pattern;
    size = str.size();
    for (i = 0; i < size; i++)
    {
        pos_new = str.find(pattern, i);
        if (pos_new < size) {
            if (pos_new - pos == pattern.size()) {
                pos_new += pattern.size();
            } else {
                if (result.size() > count) {
                    s = str.substr(i, str.size() - i);
                    result.push_back(s);
                    break;
                }
                s = str.substr(i, pos_new - i);
                result.push_back(s);
            }
            i = pos_new + pattern.size() - 1;
            pos = pos_new;
        }
    }
    return result;
}

int logEntryBase::decomposeLog(std::vector<string_t> logVec, struct logEntry_t *logLine){
    if (logVec.size() > 7 || logVec[0].find('-') == string_t::npos ||
        logVec[4].size() > 1) {
        return -1;
    }
    logLine->time.month = std::stoi(logVec[0].substr(0,2));
    logLine->time.day = std::stoi(logVec[0].substr(3,2));
    logLine->time.hour = std::stoi(logVec[1].substr(0,2));
    logLine->time.min = std::stoi(logVec[1].substr(3,2));
    logLine->time.second =std::stoi(logVec[1].substr(6,2));
    logLine->pid = std::stoi(logVec[2]);
    logLine->tid = std::stoi(logVec[3]);
    logLine->priority = logVec[4].c_str();
    logLine->tag = logVec[5].c_str();
    logLine->msg = logVec[6].c_str();
    return 0;
    if(0) {
       logVec = logVec;
       logLine = logLine;
    }

}
logEntryBase::logEntryBase() {

}

logEntryBase::~logEntryBase() {

}

void logEntryBase::logPraser(char *buf, int count, struct logEntry_t *logLine) {
    string_t pattern = " ";
    std::vector<string_t> result = split(buf, pattern,5);
    decomposeLog(result,logLine);
    if(0) {
       buf = buf;
       count = count;
       logLine = logLine;
    }
}

} //namespace android
