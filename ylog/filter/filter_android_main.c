/**
 * Copyright (C) 2016 Spreadtrum Communications Inc.
 */
#include <string.h>
#include <stdio.h>

enum filter_state_t {
    NORMAL = 0,
    START,
};

int filter_log(char *line, int count, enum filter_state_t state) {
    // printf("ylog<debug> ""I catch you: %s\n",line);
    return 0;
    if(0) {
        line = line;
        count = count;
        state = state;
    }
}
