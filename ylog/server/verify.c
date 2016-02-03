/**
 * Copyright (C) 2016 Spreadtrum Communications Inc.
 */
static void ylog_terminate_signal_handler(int sig);

static void ylog_verify(void) {
    char buf[4096];
    int count;
    struct ylog *y;

    if (0) {
        sleep(10);
        ylog_terminate_signal_handler(2);
    }

    if (0) {
        int numbers = 0;
        int times = 5000;
        y = ylog_get_by_name("test");
        if (y) {
            while (times--) {
                count = snprintf(buf, sizeof(buf), "%d\n", numbers++);
                y->write_handler(buf, count, y);
            }
        }
    }

    if (0) {
        static int changed = 0;
        if (changed == 0) {
            changed = 1;
            if (1) for (;;)
            {
                static int count = 0;
                kernel_notify(buf, snprintf(buf, sizeof(buf), "%d\n", count++));
                ydst_root_quota(NULL, 512*1024*1024);
                ydst_root_new(NULL, "sdcard/Tylog");
                sleep(7);
                ydst_root_quota(NULL, 50*1024*1024);
                ydst_root_new(NULL, "sdcard/Tylog0");
                sleep(10);
                ydst_root_quota(NULL, 100*1024*1024);
                ydst_root_new(NULL, "sdcard/Tylog1");
                sleep(11);
                ydst_root_quota(NULL, 200*1024*1024);
                ydst_root_new(NULL, "sdcard/Tylog2");
                sleep(12);
                ydst_root_quota(NULL, 30*1024*1024);
                ydst_root_new(NULL, "sdcard/Tylog3");
                sleep(13);
                ydst_root_quota(NULL, 330*1024*1024);
                ydst_root_new(NULL, "sdcard/Tylog4");
                sleep(14);
                ydst_root_quota(NULL, 1024*1024*1024);
                ydst_root_new(NULL, "sdcard/Tylog5");
                sleep(15);
            }
        }
    }
}
