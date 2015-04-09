/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 22547 $ $Date: 2013-10-23 11:17:36 +0800 (Wed, 23 Oct 2013) $
 */

#include <osal.h>
#include <pduconv.h>
#include <utf8_to_utf16.h>
#include "pdu_hlpr.h"
#include "_pdu_hlpr.h"

/* 
 * ======== PDU_pduHexStringToBytes() ========
 * 
 * Private function to convert hex string to bytes
 *
 * Returns: 
 *    Converted bytes size.
 */
int PDU_pduHexStringToBytes(
    char          *in_ptr,
    unsigned char *out_ptr) {
    int sz, i;

    char a,b;
    
    sz = OSAL_strlen(in_ptr);

    for (i = 0 ; i < sz ; i += 2) {
        a = _PDU_pduHexCharToInt(in_ptr[i]) << 4;
        b = _PDU_pduHexCharToInt(in_ptr[i + 1]);
        out_ptr[i / 2] = a | b;
    }
    return (i / 2);
}

/*
 * ======== _PDU_pduAdvance() ========
 * 
 * Public function to convert bytes to hex string
 *
 * Returns: 
 *    Converted length of the hex string
 */
int PDU_pduBytesToHexString(
    unsigned char *in_ptr,
    int            len,
    char          *out_ptr)
{
    static char digits[] = "0123456789ABCDEF";
    int b, i;
    char *cur_ptr;
    
    cur_ptr = out_ptr;
        
    for (i = 0 ; i < len ; i++) {
        
        b = 0x0f & (in_ptr[i] >> 4);
        *cur_ptr = digits[b];
        cur_ptr++;

        b = 0x0f & in_ptr[i];
        *cur_ptr = digits[b];
        cur_ptr++;
    }
    // NULL terminate
    *cur_ptr = 0;
    // return the length
    return (cur_ptr - out_ptr);
}

