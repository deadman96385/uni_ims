#ifndef _COMMON_H_
#define _COMMON_H_

#define UNUSED(x) (void)(x) /* avoid compiler warning */

#define PRINT_HEAD 0x01
#define PRINT_BODY 0x02
#define PRINT_END  0x03

static bool tag_on = false;
static bool no_header = false;

struct log_output {
    int (*print_log)(int flag, void *private, char *p);
    void *private;
    char *p;
};

static int msleep(int ms) {
    if (ms > 1000) {
        sleep(ms / 1000);
        ms %= 1000;
    }
    usleep(ms * 1000);
    return 0;
}

static int logs_print(char *cur_time, int flag, struct log_output *log_out) {
    int i;

    if (flag & PRINT_BODY) {
        if (tag_on)
            printf("\n%s %s,", (char*)log_out[0].private, cur_time);
        else
            printf("%s,", cur_time);
    }

    for (i = 0; log_out[i].print_log; i++) {
        if (i) printf(",");
        log_out[i].print_log(flag, log_out[i].private, log_out[i].p);
    }

    printf("\n");
    return 0;
}

#endif /* _COMMON_H_ */

