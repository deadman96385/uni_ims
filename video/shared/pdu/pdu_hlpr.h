/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 22547 $ $Date: 2013-10-23 11:17:36 +0800 (Wed, 23 Oct 2013) $
 */

#ifndef _PDU_HLPR_H_
#define _PDU_HLPR_H_

typedef enum {
    PDU_ERR = 1,
    PDU_OK = 0,
} PDU_Return;

int PDU_pduHexStringToBytes(
    char          *in_ptr,
    unsigned char *out_ptr);

int PDU_pduBytesToHexString(
    unsigned char *in_ptr,
    int            len,
    char          *out_ptr);

#endif
