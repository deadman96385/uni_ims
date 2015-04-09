/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004-2012 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 19918 $ $Date: 2013-02-26 15:29:12 +0800 (Tue, 26 Feb 2013) $
 *
 */

#include <osal.h>
#include "isi_xdr.h"
#include "isi.h"
#include "isi_rpc.h"

/* 
 * ======== _ISI_xdrWriteBytes() ========
 *
 * This function is to write bytes to ISI_Xdr structure.
 *
 * Returns:
 *  OSAL_SUCCESS: Success
 *  OSAL_FAIL:    Failure.
 */
static OSAL_Status _ISI_xdrWriteBytes(
    ISI_Xdr    *xdr_ptr,
    const void *buf_ptr,
    vint        size)
{
    if (NULL == xdr_ptr) {
        return (OSAL_FAIL);
    }

    /* Check if there is available space */
    if (ISI_DATA_SIZE < (xdr_ptr->length + size)) {
        return (OSAL_FAIL);
    }

    /* Copy data */
    OSAL_memCpy(xdr_ptr->cur_ptr, buf_ptr, size);
    /* Move current point and increase current length */
    xdr_ptr->cur_ptr += size;
    xdr_ptr->length += size;

    return (OSAL_SUCCESS);
}

/* 
 * ======== _ISI_xdrReadBytes() ========
 *
 * This function is to read bytes to ISI_Xdr structure.
 *
 * Returns:
 *  OSAL_SUCCESS: Success
 *  OSAL_FAIL:    Failure.
 */
static OSAL_Status _ISI_xdrReadBytes(
    ISI_Xdr *xdr_ptr,
    void    *buf_ptr,
    vint     size)
{
    if (NULL == xdr_ptr) {
        return (OSAL_FAIL);
    }

    /* Check if there is enough data */
    if (xdr_ptr->length < size) {
        return (OSAL_FAIL);
    }

    /* Copy data */
    OSAL_memCpy(buf_ptr, xdr_ptr->cur_ptr, size);
    /* Move current point and decrease current length */
    xdr_ptr->cur_ptr += size;
    xdr_ptr->length -= size;

    return (OSAL_SUCCESS);
}

/* 
 * ======== ISI_xdrEncodeInit() ========
 *
 * This function is to initialize ISI_Xdr for encode.
 *
 * Returns:
 *  OSAL_SUCCESS: Success
 *  OSAL_FAIL:    Failure.
 */
OSAL_Status ISI_xdrEncodeInit(
    ISI_Xdr *xdr_ptr)
{
    if (NULL == xdr_ptr) {
        return (OSAL_FAIL);
    }

    /* Clear memory */
    OSAL_memSet(xdr_ptr, 0, sizeof(ISI_Xdr));

    /* Point cur_ptr to start of data */
    xdr_ptr->cur_ptr = &xdr_ptr->data[0];
    return (OSAL_SUCCESS);
}

/* 
 * ======== ISI_xdrEncodeInit() ========
 *
 * This function is to initialize ISI_Xdr for decode
 *
 * Returns:
 *  OSAL_SUCCESS: Success
 *  OSAL_FAIL:    Failure.
 */
OSAL_Status ISI_xdrDecodeInit(
    ISI_Xdr *xdr_ptr)
{
    if (NULL == xdr_ptr) {
        return (OSAL_FAIL);
    }

    ISI_xdrEncodeInit(xdr_ptr);

    /* Set length to total data size */
    xdr_ptr->length = ISI_DATA_SIZE;
    return (OSAL_SUCCESS);
}

/* 
 * ======== ISI_xdrPutInteger() ========
 *
 * This function is to write 32-bit integer to ISI_Xdr structure.
 *
 * Returns:
 *  OSAL_SUCCESS: Success
 *  OSAL_FAIL:    Failure.
 */
OSAL_Status ISI_xdrPutInteger(
    ISI_Xdr   *xdr_ptr,
    const vint i)
{
    int nbInt; /* network byte ordered integer */

    /* Convert to network byte order */
    nbInt = OSAL_netHtonl(i);
    /* Write to 4 bytes to ISI_Xdr */
    return (_ISI_xdrWriteBytes(xdr_ptr, &nbInt, sizeof(nbInt)));
}

/* 
 * ======== ISI_xdrGetInteger() ========
 *
 * This function is to get 32-bit integer from ISI_Xdr structure.
 *
 * Returns:
 *  OSAL_SUCCESS: Success
 *  OSAL_FAIL:    Failure.
 */
OSAL_Status ISI_xdrGetInteger(
    ISI_Xdr *xdr_ptr,
    vint    *i_ptr)
{
    vint nbInt = 0; /* network byte ordered integer */

    _ISI_xdrReadBytes(xdr_ptr, &nbInt, sizeof(nbInt));

    /* Convert to host byte order */
    *i_ptr = OSAL_netNtohl(nbInt);
    return (OSAL_SUCCESS);
}

/* 
 * ======== ISI_xdrPutString() ========
 *
 * This function is to put a string to ISI_Xdr structure.
 *
 * Returns:
 *  OSAL_SUCCESS: Success
 *  OSAL_FAIL:    Failure.
 */
OSAL_Status ISI_xdrPutString(
    ISI_Xdr    *xdr_ptr,
    const char *c_ptr,
    vint        length)
{
    /* Put string length */
    if (OSAL_FAIL == ISI_xdrPutInteger(xdr_ptr, length)) {
        return (OSAL_FAIL);
    }
    /* Write string content */
    return (_ISI_xdrWriteBytes(xdr_ptr, c_ptr, length));
}

/* 
 * ======== ISI_xdrGetString() ========
 *
 * This function is to get a string from ISI_Xdr structure.
 *
 * Returns:
 *  OSAL_SUCCESS: Success
 *  OSAL_FAIL:    Failure.
 */
OSAL_Status ISI_xdrGetString(
    ISI_Xdr *xdr_ptr,
    char    *c_ptr,
    vint    *length_ptr)
{
    /* Get string length */
    if (OSAL_FAIL == ISI_xdrGetInteger(xdr_ptr, length_ptr)) {
        return (OSAL_FAIL);
    }
    /* Check string length */
    if (0 > *length_ptr) {
        return (OSAL_FAIL);
    }

    /* Read string content */
    return (_ISI_xdrReadBytes(xdr_ptr, c_ptr, *length_ptr));
}

