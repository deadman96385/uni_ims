/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2005 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 988 $ $Date: 2006-11-02 18:47:08 -0500 (Thu, 02 Nov 2006) $
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/select.h>
#include <sys/soundcard.h>
#include <sys/ioctl.h>
#include <pthread.h>
#include "vhw.h"

typedef struct {
    int      fd;
    int      fdOut;
    int      fdIn;
    vint     obufi[VHW_PCM_BUF_SZ];
    vint     ibufi[VHW_PCM_BUF_SZ];
    int16    obuf[VHW_PCM_BUF_SZ];
    int16    ibuf[VHW_PCM_BUF_SZ];
    int      init;
} _VHW_Obj;

static _VHW_Obj _VHW_obj;

/*
 * Driver sleep/wake flag
 */
int _VHW_sleep = 1; 

/*
 * ======== VHW_exchange() ========
 * Application calls this function to get data from the PCM highway.
 * Four pointers are returned.
 * 1. Data transmit pointer. This is the place where the application can put
 *    data to be transmitted in the next block.
 * 2. Data receive pointer. This is the pointer where the application can
 *    read just arrived block of data.
 *
 * If data is not immediately available, this function will sleep till 
 * the data is not avaialable.
 *
 * Return Values:
 * None.
 */
void VHW_exchange(
    int **tx_ptr,
    int **rx_ptr)
{
    int   sample;
    int   tmp;
    int   devcaps;

    if (!_VHW_obj.init) {
        _VHW_obj.init = 1;
    
        /*
         * Open the device file. The one device full duplex scheme requires 
         * that the device file is opened with O_RDWR. See the description of
         * the open system call for more info.
         */
        if ((_VHW_obj.fd = open(VHW_DEVNAME, O_RDWR, 0)) == -1) {
            printf("%s:%d - VHW ERROR\n", __FILE__, __LINE__);
            return;
        }

        /*
         * Check that the device supports full duplex.
         * Otherwise there is no point in continuing.
         */
        if (ioctl (_VHW_obj.fd, SNDCTL_DSP_GETCAPS, &devcaps) == -1) {
            close(_VHW_obj.fd);
            printf("%s:%d - VHW ERROR\n", __FILE__, __LINE__);
            return;
        }
        if (!(devcaps & DSP_CAP_DUPLEX)) {
            close(_VHW_obj.fd);
            printf("%s:%d - VHW ERROR\n", __FILE__, __LINE__);
            return;
        }

        /*
         * Turn on the full duplex mode.
         */
        if (ioctl (_VHW_obj.fd, SNDCTL_DSP_SETDUPLEX, NULL) == -1) {
            close(_VHW_obj.fd);
            printf("%s:%d - VHW ERROR\n", __FILE__, __LINE__);
            return;
        }

        /*
         * Try to set the fragment size to suitable level.
         */
        tmp = 0x7FFF0004;
        if (ioctl (_VHW_obj.fd, SNDCTL_DSP_SETFRAGMENT, &tmp) == -1) {
            close(_VHW_obj.fd);
            printf("%s:%d - VHW ERROR\n", __FILE__, __LINE__);
            return;
        }

        /*
         * Set up num ch.
         */
        tmp = 1;
        if (ioctl (_VHW_obj.fd, SNDCTL_DSP_CHANNELS, &tmp) == -1) {
            close(_VHW_obj.fd);
            printf("%s:%d - VHW ERROR\n", __FILE__, __LINE__);
            return;
        }

        /*
         * Request sample format.
         */
        tmp = AFMT_S16_LE;
        if (ioctl (_VHW_obj.fd, SNDCTL_DSP_SETFMT, &tmp) == -1) {
            close(_VHW_obj.fd);
            printf("%s:%d - VHW ERROR\n", __FILE__, __LINE__);
            return;
        }

        /*
         * Set sample rate.
         */
        tmp = _VHW_SAMPLE_RATE;
        if (ioctl (_VHW_obj.fd, SNDCTL_DSP_SPEED, &tmp) == -1) {
            close(_VHW_obj.fd);
            printf("%s:%d - VHW ERROR\n", __FILE__, __LINE__);
            return;
        }

        _VHW_obj.fdIn = _VHW_obj.fdOut = _VHW_obj.fd;

        write(_VHW_obj.fdOut, _VHW_obj.obuf, sizeof(_VHW_obj.obuf));
        write(_VHW_obj.fdOut, _VHW_obj.obuf, sizeof(_VHW_obj.obuf));
    }

    read(_VHW_obj.fdIn, _VHW_obj.ibuf, sizeof(_VHW_obj.ibuf));
    for (sample = 0; sample < VHW_PCM_BUF_SZ; sample++) {
        _VHW_obj.ibufi[sample] =
                ((vint)_VHW_obj.ibuf[sample]);
        _VHW_obj.obuf[sample] =
                ((int16)_VHW_obj.obufi[sample]);
    }
    write(_VHW_obj.fdOut, _VHW_obj.obuf, sizeof(_VHW_obj.obuf));

    /*
     * Conversion buffers
     */
    *rx_ptr = _VHW_obj.ibufi;
    *tx_ptr = _VHW_obj.obufi;
}

/*
 * ======== VHW_init() ========
 * This function inits the interface to the low level hardware of 
 * the host processor.
 *
 * Return Values:
 * 0 : Success.
 * !0: Error (see error codes).
 */
int VHW_init(
    void)
{
    return (0);
}

/*
 * ======== VHW_start() ========
 * Start the voice hardware by taking CPLD out of reset and 
 * enabling interrupts from it.
 *
 * Return Values:
 * None.
 */
void VHW_start(
    void)
{
}

/*
 * ======== VHW_shutdown() ========
 * This function closes the interface to the low level hardware of 
 * the host processor.
 *
 * Return Values:
 * None.
 */
void VHW_shutdown(
    void)
{
}
/*
 * ======== _VHW_detach() ========
 * Puts VHW in low power mode by detaching audio.
 *
 * Return Values:
 */
void VHW_detach(
    void)
{
    int sample;
    /*
     * This will cause the audio driver and vTSP to sleep.
     */
    _VHW_sleep = 1;
    for (sample = 0; sample < VHW_PCM_BUF_SZ; sample++) {
        _VHW_obj.obuf[sample] = 0;
    }
}

/*
 * ======== _VHW_attach() ========
 * Puts VHW in full power mode by attaching audio.
 *
 * Return Values:
 */
void VHW_attach(
    void)
{
    /*
     * This will cause the audio driver and vTSP to come out of sleep.
     */
    _VHW_sleep = 0; 
}

/*
 * ======== VHW_isSleeping() ========
 * Return VHW sleeping st.
 *
 * Return Values:
 */
int VHW_isSleeping(
    void)
{
    /*
     * This will cause the audio driver and vTSP to come out of sleep.
     */
    return(_VHW_sleep); 
}
