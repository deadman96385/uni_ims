#include <osal.h>

/* ======== D2_getLine ========
 * get user input by char with echo
 */
int D2_getLine(
    char *buf, 
    unsigned int max) 
{ 
    int           c;
    uvint         index;
    
    index = 0;
    max--;
    while (EOF != (c = fgetc(stdin))) { 
        if (((0x7f == c) || ('\b' == c))) { 
            if (index) { 
                OSAL_logMsg("\b \b");
                *(buf + index) = 0;
                index--;
            }
            continue;
        }
        if (('\n' == c) || ('\r' == c)) { 
            OSAL_logMsg("\n");
            break;
        }
        if (index >= max) { 
            continue;
        }
        OSAL_logMsg("%c", c);
        *(buf + index) = c;
        index++;
    }
    *(buf + index) = 0;
    return (index);
}
