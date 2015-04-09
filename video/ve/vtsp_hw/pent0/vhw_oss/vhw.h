/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2005 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 988 $ $Date: 2006-11-02 18:47:08 -0500 (Thu, 02 Nov 2006) $
 */

#ifndef _VHW_H_
#define _VHW_H_

#include <osal_types.h>
#include <osal.h>

/*
 * This file describes interfaces to the low level hardware of the target
 * processor.
 */

#define VHW_DEVNAME             "/dev/vtsp-hw"

/*
 * Define the channel buffer size in samples per processing period
 */
#if defined(VTSP_ENABLE_STREAM_8K)
#define _VHW_SAMPLE_RATE         (8000)
#else
#define _VHW_SAMPLE_RATE         (16000)
#endif

#define _VHW_FRAMES_PER_SEC      (100)

#define VHW_PCM_BUF_SZ          (_VHW_SAMPLE_RATE / _VHW_FRAMES_PER_SEC)


/*
 * Number of PCM channels supported.
 */ 
#define VHW_NUM_PCM_CH          (1)

/*
 * Number of SPI channels supported
 */
#define VHW_NUM_SPI_CH          (0)
 
/*
 * VHW Error codes
 */
#define VHW_ERROR_IRQ_ALLOC     (0x03000000)
#define VHW_ERROR_MEM_ALLOC     (0x03000001)
#define VHW_ERROR_ALREADY_INIT  (0x03000002)

/*
 * Set physical interfaces audio characteristics
 */
#define VHW_AUDIO_SHIFT         (2)
#undef  VHW_AUDIO_MULAW

/*
 * This macro returns start address of buffer for a channel 'ch' given data_ptr 
 * points to the start address of buffer for ch0.
 * Returns NULL if buffer is not available.
 */
#define VHW_GETBUF(data_ptr, ch)      \
        (ch > (VHW_NUM_PCM_CH - 1)) ? \
         NULL : (((vint *)data_ptr) + (VHW_PCM_BUF_SZ * ch))

/* 
 * Function prototypes
 */

#define VHW_AMP_DISABLE
#define VHW_AMP_ENABLE

int VHW_init(
    void);

void VHW_start(
    void);

void VHW_shutdown(
    void);

void VHW_exchange(
    int **tx_ptr,
    int **rx_ptr);

void VHW_attach(
    void);

void VHW_detach(
    void);

int VHW_isSleeping(
    void);

#endif
