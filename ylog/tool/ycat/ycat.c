#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include "ytag.h"

int main(int argc, char *argv[]) {
    int fd;
    int ret;
    char buf[4096];
    char *test;
    struct ytag ytag;

    fd = open(argv[1], O_RDONLY);
    if (fd < 0)
        return 1;

    ytag_newfile_begin(NULL);
    while ((ret = read(fd, buf, 1024)) > 0)
        ytag_rawdata(buf, ret);

    test = "ytag test embeded_file1\n";
    ytag_newfile_begin("embeded_file1");
    ytag_rawdata(test, strlen(test));
    test = "ytag test embeded_file3\n";
    ytag_newfile_begin("embeded_file3");
    ytag_rawdata(test, strlen(test));
    test = "ytag test embeded_file4\n";
    ytag_newfile_begin("embeded_file4");
    ytag_rawdata(test, strlen(test));
    ytag_newfile_end("embeded_file4");
    test = "ytag test embeded_file4.2\n";
    ytag_newfile_begin("embeded_file4");
    ytag_rawdata(test, strlen(test));
    ytag_newfile_end("embeded_file4");
    test = "ytag test embeded_file3.2\n";
    ytag_newfile_begin("embeded_file3");
    ytag_rawdata(test, strlen(test));
    ytag_newfile_end("embeded_file3");
    ytag_newfile_end("embeded_file3");
    ytag_newfile_end("embeded_file1");

    test = "ytag test embeded_file2\n";
    ytag_newfile_begin("embeded_file2");
    ytag_rawdata(test, strlen(test));
    ytag_newfile_end("embeded_file2");

    test = "ytag test embeded_file5中文\n";
    ytag_newfile_begin("embeded_file5中文");
    ytag_rawdata(test, strlen(test));
    ytag_newfile_end("embeded_file5中文");

    test = "All embeded done! -- append this test string to the parent file content\n";
    ytag_rawdata(test, strlen(test));

    ytag_newfile_end(NULL);

    close(fd);
    return 0;

    argc = argc;
}
