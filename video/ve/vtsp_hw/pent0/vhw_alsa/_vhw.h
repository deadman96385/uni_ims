/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2005-2009 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 10825 $ $Date: 2009-11-18 16:03:22 -0800 (Wed, 18 Nov 2009) $
 */

#ifndef __VHW_H_
#define __VHW_H_

#include <vhw.h>
#include <osal.h>
#include <osal_types.h>

#include <alsa/asoundlib.h>

/*
 * This file describes interfaces to the low level hardware of the target
 * processor.
 */
#define _VHW_BYTES_PER_SAMPLE  (sizeof(int16))
#define _VHW_BUF_SZ_BYTES_10MS (_VHW_PCM_BUF_SZ * _VHW_BYTES_PER_SAMPLE)

#define _VHW_RD_BUF_SZ         (640)  /* 64ms @ 16kHz */
#define _VHW_WR_BUF_SZ         (640)  /* 30ms @ 16kHz */

#define _VHW_RD_BUF_HW_NUM     (10)     /* 640 bytes divides 10ms */
#define _VHW_WR_BUF_HW_NUM     (10)     /* 640 bytes divides 10ms */

#define _VHW_RD_BUF_APP_NUM    ((_VHW_RD_BUF_HW_NUM * _VHW_RD_BUF_SZ) / _VHW_BUF_SZ_BYTES_10MS)
#define _VHW_WR_BUF_APP_NUM    ((_VHW_WR_BUF_HW_NUM * _VHW_WR_BUF_SZ) / _VHW_BUF_SZ_BYTES_10MS)

#define _VHW_RD_BUF_SZ_SAMP    (_VHW_RD_BUF_SZ / _VHW_BYTES_PER_SAMPLE)
#define _VHW_WR_BUF_SZ_SAMP    (_VHW_WR_BUF_SZ / _VHW_BYTES_PER_SAMPLE)

#define _VHW_TASK_STACKSIZE    (8000)
#define _VHW_TASK_PRIORITY     (99)

#define _VHW_RD_HEADSTART      (5)
#define _VHW_WR_HEADSTART      (2)

#define _VHW_RD_SLEEP_TIME     (9700)
#define _VHW_WR_SLEEP_TIME     (8500)

typedef struct {
    vint             pcmXmit_ary[_VHW_PCM_BUF_SZ];
    vint             pcmRecv_ary[_VHW_PCM_BUF_SZ];
    int16            pcmIn[_VHW_RD_BUF_HW_NUM * _VHW_RD_BUF_SZ_SAMP];
    int16            pcmOut[_VHW_WR_BUF_HW_NUM * _VHW_WR_BUF_SZ_SAMP];
    struct timespec  abst;
    sem_t            sema;
    int16           *curIn_ptr;
    int16           *curOut_ptr;
    vint             inBytes;
    vint             outBytes;
    uint32           count10ms;
    uvint            hwWrIdx;
    uvint            appWrIdx;
    uvint            hwRdIdx;
    uvint            appRdIdx;
    vint             writeFlag;
    vint             readFlag;
} _VHW_Obj;

/*
 * Private prototypes.
 */

void _VHW_pcmIsr(
    void);

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

void _VHW_setVolume(
    _VHW_Obj *_VHW_ptr,
    uvint     device,
    uvint     volume);

void _VHW_printPcmConfig(
    void *config);

void _VHW_dumpBuffer(
    int16 *buf,
    vint   bytesRead);

void _VHW_10msWait(
    void);

void _VHW_dumpSndPcmParams(
    snd_pcm_hw_params_t *params);
#endif
