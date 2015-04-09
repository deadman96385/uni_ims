/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 7573 $ $Date: 2008-09-09 12:58:53 -0400 (Tue, 09 Sep 2008) $
 *
 */

#include <osal_mem.h>

/* 
 * ======== OSAL_memSet() ========
 * This function is a C library memset implementation.
 *
 * Returns:
 *  Number of chars printed.
 */
void OSAL_memSet(
    void    *mem_ptr, 
    int32    value,   
    int32    size)
{
    memset(mem_ptr,
            value,
            size);
}

/* 
 * ======== OSAL_memCpy() ========
 * This function is a C library memcpy implementation.
 *
 * Returns:
 *  Number of chars printed.
 */
void OSAL_memCpy(
    void        *dest_ptr, 
    const void  *src_ptr,
    int32        size)
{
    memcpy(dest_ptr,
            src_ptr,
            size);
}

/* 
 * ======== OSAL_memCmp() ========
 * This function is a C library memcmp implementation.
 *
 * Returns:
 *  Number of chars printed.
 */
int OSAL_memCmp(
    const void *mem1_ptr,
    const void *mem2_ptr,
    int32       size)
{
    return (memcmp(
            mem1_ptr,
            mem2_ptr,
            size));
}

/*
 * ======== OSAL_memAlloc() ========
 *
 * Allocates memory
 *
 * Returns
 *  void * - ptr to a block or NULL on failure
 */
void * OSAL_memAlloc(
    int32 numBytes,
    int32 memArg)
{
    return((void *)kmalloc(numBytes,
            GFP_KERNEL));
}

/*
 * ======== OSAL_memCalloc() ========
 *
 * Allocates memory and zeros it
 *
 * Returns
 *  void * - ptr to a block or NULL on failure
 */
void * OSAL_memCalloc(
    int32 numElements,
    int32 elementSize,
    int32 memArg)
{
    void *ret_ptr;
    int   sz;
    
    sz = numElements * elementSize;

    ret_ptr = OSAL_memAlloc(sz,
            0);
    if (NULL != ret_ptr) {
        memset(ret_ptr,
                0,
                sz);
    }
    return (ret_ptr);
}

/*
 * ======== OSAL_memFree() ========
 *
 * Frees memory
 *
 * Returns
 *  OSAL_SUCCESS or OSAL_FAIL
 */
OSAL_Status OSAL_memFree(
    void  *mem_ptr,
    int32  memArg)
{
    if(NULL == mem_ptr) {
        return(OSAL_FAIL);
    }

    kfree(mem_ptr);

    return(OSAL_SUCCESS);
}



EXPORT_SYMBOL(OSAL_memCpy);
EXPORT_SYMBOL(OSAL_memSet);
EXPORT_SYMBOL(OSAL_memCmp);
EXPORT_SYMBOL(OSAL_memAlloc);
EXPORT_SYMBOL(OSAL_memCalloc);
EXPORT_SYMBOL(OSAL_memFree);
