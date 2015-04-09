/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004-2009 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 7048 $ $Date: 2008-07-18 10:35:53 -0700 (Fri, 18 Jul 2008) $
 *
 */

#include <osal_mem.h>
#include <osal_log.h>
#include "sdi_cfg_entity.h"
#include "sdi_mem.h"

/*
  * _OSAL_MEM_C_ is defined in
  * /MS_Code/PS/build/obj/gen/debug/ims_stack_file_code.h 
  */
#define _FILE_CODE_ _OSAL_MEM_C_

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
    osa_mem_set(mem_ptr,
            (uint8)value,
            (uint32)size);
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
    osa_mem_cpy(dest_ptr,
            src_ptr,
            (uint32)size);
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
    return (int)(osa_mem_cmp(
            mem1_ptr,
            mem2_ptr,
            (uint32)size));
}

/*
 * ======== OSAL_byteMemAlloc() ========
 *
 * Allocates memory from byte memory pool
 *
 * Returns
 *  void * - ptr to a block or NULL on failure
 */
void * OSAL_byteMemAlloc(
    int32 numBytes,
    int32 memArg)
{
    return((void *)sdi_mem_int_alloc_((uint32)numBytes,
            ENTITY_IMS_STACK,
            _FILE_CODE_, 
            __LINE__));
}

/*
 * ======== OSAL_blkMemAlloc() ========
 *
 * Allocates memory from block memory pool
 *
 * Returns
 *  void * - ptr to a block or NULL on failure
 */
void * OSAL_blkMemAlloc(
    int32 numBytes,
    int32 memArg)
{
    return((void *)sdi_blk_mem_alloc_((uint32)numBytes,
            ENTITY_IMS_STACK,
            _FILE_CODE_, 
            __LINE__));
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
    void *ptr;
    if (OSAL_MEM_ARG_STATIC_ALLOC == memArg) {
        ptr = OSAL_byteMemAlloc(numBytes, memArg);
        if (NULL == ptr) {
            OSAL_logMsg("%s %d alloc static Memory failed !!! \n");
        }
        return (ptr);
    }
    else if (OSAL_MEM_ARG_DYNAMIC_ALLOC == memArg) {
        ptr = OSAL_blkMemAlloc(numBytes, memArg);
        if (NULL == ptr) {
            OSAL_logMsg("%s %d alloc dynamic Memory failed !!! \n");
        }
        return (ptr);
    }
    return (NULL);
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

    ret_ptr = OSAL_memAlloc(sz, memArg);
    if (NULL != ret_ptr) {
        OSAL_memSet(ret_ptr,
                0,
                sz);
    }
    return (ret_ptr);
}

/*
 * ======== OSAL_memFree() ========
 *
 * Frees static memory
 *
 * Returns
 *  OSAL_SUCCESS or OSAL_FAIL
 */
OSAL_Status OSAL_byteMemFree(
    void  *mem_ptr,
    int32  memArg)
{
    if(NULL == mem_ptr) {
        return (OSAL_FAIL);
    }

    sdi_mem_int_free(mem_ptr,
         ENTITY_IMS_STACK,
         _FILE_CODE_, 
         __LINE__);

    return(OSAL_SUCCESS);
}

/*
 * ======== OSAL_blkMemFree() ========
 *
 * Frees dynamic memory
 *
 * Returns
 *  OSAL_SUCCESS or OSAL_FAIL
 */
OSAL_Status OSAL_blkMemFree(
    void  *mem_ptr,
    int32  memArg)
{
    if(NULL == mem_ptr) {
        return(OSAL_FAIL);
    }

    sdi_blk_mem_free_(mem_ptr,
         ENTITY_IMS_STACK,
         _FILE_CODE_, 
         __LINE__);

    return (OSAL_SUCCESS);
}
/*
 * ======== OSAL_blkMemFree() ========
 *
 * Frees  memory
 *
 * Returns
 *  OSAL_SUCCESS or OSAL_FAIL
 */
OSAL_Status OSAL_memFree(
    void  *mem_ptr,
    int32  memArg)
{
    if (OSAL_MEM_ARG_STATIC_ALLOC == memArg) {
        return OSAL_byteMemFree(mem_ptr, memArg);
    }
    else if (OSAL_MEM_ARG_DYNAMIC_ALLOC == memArg) {
        return OSAL_blkMemFree(mem_ptr, memArg);
    }
    OSAL_logMsg("Invalid mem arg for memFree:%d!!!\n", memArg);
    return (OSAL_FAIL);
}

/*
 * ======== OSAL_memReAlloc() ========
 *
 * memory re-allocate
 *
 * Returns
 *  
 */
void* OSAL_memReAlloc(
    void  *mem_ptr,
    int32  numBytes,
    int32  memArg)
{
    /* The platform does not support dynamic memory allocate */
    return (NULL);
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

