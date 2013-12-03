/*************************************************************************
** File Name:      ps_tables.h                                           *
** Author:         Reed zhang                                            *
** Date:           21/04/2006                                            *
** Copyright:      2001 Spreatrum, Incoporated. All Rights Reserved.     *
** Description:    This file defines the common table for PS tool        *
**                        Edit History                                   *
** ----------------------------------------------------------------------*
** DATE           NAME             DESCRIPTION                           *
** 21/04/2006     Reed zhang       Create.                               *
**************************************************************************/

#ifndef __PS_TABLES_H__
#define __PS_TABLES_H__

#ifdef __cplusplus
extern "C" {
#endif
#include "aac_common.h"


typedef sh_complex_t sh_complex_t3[3];
typedef aac_complex    complex_t5[5];
#define FILTER8_BIT 11
#define NEGATE_IPD_MASK (0x1000)


#define PS_COS_SIN_TAB_BIT 11
#define DECAY_FILT_BIT 10
#define DECAY_SLOPE    51   // Q0.10
#define PS_DECORR_BIT_TAB 9

#ifdef __cplusplus
#endif
#endif
