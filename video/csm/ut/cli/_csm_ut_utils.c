/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2012 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 18551 $ $Date: 2012-10-23 01:55:53 -0700 (Tue, 23 Oct 2012) $
 */

#include <osal.h>
#include "_csm_ut_utils.h"

/* ======== _CSM_UT_getLine ========
 * get user input by char with echo
 */
int _CSM_UT_getLine(
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
                *(buf + index) = 0;
                index--;
            }
            continue;
        }
        if (('\n' == c) || ('\r' == c)) { 
            break;
        }
        if (index >= max) { 
            continue;
        }
        *(buf + index) = c;
        index++;
    }
    *(buf + index) = 0;
    return (index);
}

/*
 * ======== CSM_UT_getBuffer() ========
 *
 * This function is used to a multilines string to the buffer
 *
 * Return Values:
 * size of the read chars
 */
int CSM_UT_getBuffer(
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
                *(buf + index) = 0;
                index--;
            }
            continue;
        }
        if (index >= max) {
            break;
        }
        *(buf + index) = c;
        index++;
    }
    *(buf + index) = 0;
    return (index);
}

