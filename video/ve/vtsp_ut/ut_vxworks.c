#include <osal.h>
#include <stdio.h>
#include <string.h>

/* ======== D2_getLine ========
 * get user input by char with echo
 */
int D2_getLine(char *buf, unsigned int max) {
    int           c;
    int           index;

    index = 0;
    max--;
    /* Note, spec defines stdin to be fd 0 */
    while (EOF != (c = fgetc(stdin))) {
        if (((0x7f == c) || ('\b' == c))) {
            if (index) {
                printf("\b \b");
                *(buf + index) = 0;
                index--;
            }
            continue;
        }
        if (('\n' == c) || ('\r' == c)) {
            printf("\n");
            break;
        }
        if (index >= max) {
            continue;
        }
        printf("%c", c);
        *(buf + index) = c;
        index++;
    }
    *(buf + index) = 0;
    return (index);
}

