/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 27760 $ $Date: 2014-07-29 16:52:43 +0800 (Tue, 29 Jul 2014) $
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
    return((void *)malloc(numBytes));
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
    return((void *)calloc(
            (size_t) numElements,
            (size_t) elementSize));
}

/*
 * ======== OSAL_memAlloc() ========
 *
 * Allocates memory
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

    free(mem_ptr);

    return(OSAL_SUCCESS);
}
/*
 * ======== OSAL_memMove() ========
 *
 * Move memory
 *
 * Returns
 *  
 */
void OSAL_memMove(
    void        *dest_ptr,
    const void  *src_ptr,
    int32        size)
{
    memmove(dest_ptr, src_ptr, size);
}

/*
 * ======== OSAL_memReAlloc() ========
 *
 * memory re-allocate
 *
 * Returns
 *  
 */
void * OSAL_memReAlloc(
    void  *mem_ptr,
    int32  numBytes,
    int32  memArg)
{
    return realloc(mem_ptr, numBytes);
}

