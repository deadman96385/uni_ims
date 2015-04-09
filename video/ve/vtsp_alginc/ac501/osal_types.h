/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004-2007 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * +D2Tech+ Rev: 3639  Date: 2009-06-16 15:08:48 -0700 (Tue, 16 Jun 2009) 
 * +D2Tech+ Release Version: d2fs1-comm_1_6
 */

#define _D2TYPES_H_
#ifndef _D2TYPES_H_
/* In case d2types.h is also included later, this define prevents conflict. */
#define _D2TYPES_H_       
#endif

#ifndef _OSAL_TYPES_
#define _OSAL_TYPES_

/* 
 * For WinCE, don't use inline
 */
#ifdef OSAL_WINCE
#define OSAL_INLINE
#else
#define OSAL_INLINE inline
#endif

/*
 * NULL pointer
 */
#ifndef NULL
#define NULL (void *)0
#endif

/* 
 * Commonly used types
 */
typedef unsigned                uvint;
typedef int                     vint;
typedef char                    int8;
typedef unsigned char           uint8;
typedef short                   int16;
typedef unsigned short          uint16;
typedef int                     int32;
typedef unsigned int            uint32;
typedef long long               int64;
typedef unsigned long long      uint64;

/*
 * Special type for passing arguments. This argument is large enough to take a
 * pointer.
 *
 * Since pointer and non-pointer types may be different sizes, this type allows
 * programmers to select how to use the task argument.
 */
#ifdef OSAL_64
typedef union {
    uint32  uValue;
    int32   iValue;
    void   *val_ptr;
} OSAL_TaskArg;

typedef int                     OSAL_TaskReturn;
#define OSAL_TASK_RETURN(x)     return (x);
#else
typedef int                     OSAL_TaskArg;
typedef int                     OSAL_TaskReturn;
#define OSAL_TASK_RETURN(x)     return (x);
#endif


/*
 * Maximum and Minimum values of the commonly used types
 */
#define MAX_INT8  (int8)(0x7F)
#define MIN_INT8  (int8)(0x80)
#define MAX_INT16 (int16)(0x7FFF)
#define MIN_INT16 (int16)(0x8000)
#define MAX_INT32 (int32)(0x7FFFFFFF)
#define MIN_INT32 (int32)(0x80000000)
#define MAX_INT64 (int64)(0x7FFFFFFFFFFFFFFF)
#define MIN_INT64 (int64)(0x8000000000000000)


typedef enum OSAL_Status {
    OSAL_FAIL = 0,
    OSAL_SUCCESS = 1
} OSAL_Status;

typedef enum OSAL_Boolean {
    OSAL_FALSE = 0,
    OSAL_TRUE = 1
} OSAL_Boolean;


#endif /* _OSAL_TYPES_ */

