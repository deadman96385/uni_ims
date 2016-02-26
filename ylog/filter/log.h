/**
 * Copyright (C) 2016 Spreadtrum Communications Inc.
 */
#ifndef LOG_H_
#define LOG_H_

#include <cutils/log.h>
#undef LOG_TAG
#define LOG_TAG "filterLog"

struct timeLog {
    //unsigned int year;
    unsigned int month;
    unsigned int day;
    unsigned int hour;
    unsigned int min;
    unsigned int second;
};
struct logEntry_t {
    struct LogEntry_t *Next;
    struct timeLog time;
    unsigned int pid;
    unsigned int tid;
    const char* priority;
    const char* tag;
    const char* msg;
};

#endif