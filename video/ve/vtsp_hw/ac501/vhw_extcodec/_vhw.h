/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2005-2009 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 12729 $ $Date: 2010-08-09 17:35:41 -0700 (Mon, 09 Aug 2010) $
 */

#ifndef __VHW_H_
#define __VHW_H_

#include <vhw.h>

typedef struct {
    vint    pcmXmit_ary[_VHW_PCM_BUF_SZ]; /* App bufs */
    vint    pcmRecv_ary[_VHW_PCM_BUF_SZ];
    uvint   app10msFlag;
} _VHW_Obj;

/*
 * Private prototypes.
 */
void _VHW_sleepForPcmData(
    int **tx_ptr,
    int **rx_ptr);

int _VHW_allocHw(
    void);

int _VHW_start(
    void);

void _VHW_shutdown(
    void);

void _VHW_startVoice(
    void);

void _VHW_stopVoice(
    void);

#endif
