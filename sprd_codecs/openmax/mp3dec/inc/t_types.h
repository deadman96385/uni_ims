/******************************************************************************
 ** File Name:      t_types.h                                                 *
 ** Author:         Lin.liu                                                   *
 ** DATE:           12/16/2002                                                *
 ** Copyright:      2002 Spreatrum, Incoporated. All Rights Reserved.         *
 ** Description:    Basic Types & Macro Define For Test Template              *
 ******************************************************************************

 ******************************************************************************
 **                        Edit History                                       *
 ** ------------------------------------------------------------------------- *
 ** DATE           NAME             DESCRIPTION                               *
 ** 12/16/2002     Lin.liu          Create.                                   *
 ******************************************************************************/
#ifndef T_TYPES_H
#define T_TYPES_H

#include "stdio.h"  /* printf prototype */
 
/**---------------------------------------------------------------------------*
 **                         Compiler Flag                                     *
 **---------------------------------------------------------------------------*/
#ifdef __cplusplus
    extern   "C"
    {
#endif

/*
	System Clock.
*/
#define CPU_CLOCK       12998000    // 12.998 MHz

/*
	Basic Types Define
*/
typedef		int				 Flag;

typedef  unsigned int    u_32;
typedef  unsigned short   u_16;
typedef  unsigned char    u_8;
typedef  unsigned int    uint32;
typedef  unsigned short   uint16;
typedef  unsigned char    uint8;

typedef  unsigned char    bool;

typedef  signed  int    int32;
typedef  signed  short   int16;
typedef  signed char    int8;

#define  VOLATILE         volatile
#define  CONST            const

#define  LOCAL            static
#define  PUBLIC  


#ifndef  NULL
#define  NULL             (void*)(0)
#endif

#define  S_SUCCESS        0
#define  S_FAIL           1

#undef HANDLE
typedef void * HANDLE;

/*
	Enable a C/C++ function to be used as an interrupt routine
	called by the IRQ or FIQ vectors.
*/
#define IRQ_FUNC   __irq


/*
	Output the formatted string.
*/
#define TRACE(PARAM)         //printf PARAM

/**---------------------------------------------------------------------------*
 **                         Compiler Flag                                     *
 **---------------------------------------------------------------------------*/
#ifdef __cplusplus
    }
#endif

#endif // T_TYPES_H

/* End of File */
