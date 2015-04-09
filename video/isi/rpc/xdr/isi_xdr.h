/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004-2012 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 19918 $ $Date: 2013-02-26 15:29:12 +0800 (Tue, 26 Feb 2013) $
 *
 */

#ifndef _ISI_XDR_H_
#define _ISI_XDR_H_

#ifdef __cplusplus
extern "C" {
#endif

#define ISI_DATA_SIZE (2047) /* XXX Need to know thw max data size */

typedef struct {
    unsigned char  data[ISI_DATA_SIZE];
    vint           length;
    unsigned char *cur_ptr;
} ISI_Xdr;

OSAL_Status ISI_xdrPutInteger(
    ISI_Xdr    *xdr_ptr,
    const vint  i);

OSAL_Status ISI_xdrGetInteger(
    ISI_Xdr *xdr_ptr,
    vint    *i_ptr);

OSAL_Status ISI_xdrPutString(
    ISI_Xdr    *xdr_ptr,
    const char *c_ptr,
    vint        length);

OSAL_Status ISI_xdrGetString(
    ISI_Xdr *xdr_ptr,
    char    *c_ptr,
    vint    *length_ptr);

OSAL_Status ISI_xdrDecodeInit(
    ISI_Xdr *xdr_ptr);

OSAL_Status ISI_xdrEncodeInit(
    ISI_Xdr *xdr_ptr);

#ifdef __cplusplus
}
#endif

#endif /* _ISI_XDR_H_ */
