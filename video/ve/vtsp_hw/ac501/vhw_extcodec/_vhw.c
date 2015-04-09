/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2005-2009 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 13638 $ $Date: 2010-12-07 12:12:38 -0800 (Tue, 07 Dec 2010) $
 */

#include "_vhw.h"
#include <dsp.h>

extern int _VHW_sleep;
extern int _VHW_sleepOld;

/*
 *
 * Voice hardware object
 */
static _VHW_Obj *_VHW_ptr = NULL;

/*
 * ======== _VHW_sleepForPcmData() ========
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
void _VHW_sleepForPcmData(
    int **tx_ptr,
    int **rx_ptr)
{
    if (_VHW_sleep) {
        if (_VHW_sleepOld != _VHW_sleep) {
            _VHW_sleepOld = _VHW_sleep;
            _VHW_stopVoice();
        }
        OSAL_taskDelay(10);
    }
    else {
        if (_VHW_sleepOld != _VHW_sleep) {
            _VHW_sleepOld = _VHW_sleep;
            /* Start/Re-start voice */
            _VHW_startVoice();
        }
        /* Send and Receive audio by external codec */

        if (0 == _VHW_ptr->app10msFlag) {
            _VHW_ptr->app10msFlag = 1;
            OSAL_taskDelay(10);
        }
        else {
            _VHW_ptr->app10msFlag = 0;
        }
    }
    /*
     * Conversion buffers
     */
    *rx_ptr = _VHW_ptr->pcmRecv_ary;
    *tx_ptr = _VHW_ptr->pcmXmit_ary;
}

/*
 * ======== _VHW_allocHw() ========
 * This function inits the interface to the low level hardware of
 * the host processor.
 *
 * Return Values:
 * 0 : Success.
 * !0: Error (see error codes).
 */
int _VHW_allocHw(
    void)
{
     /*
      * Allocate _VHW_OBj
      */
    _VHW_ptr = (_VHW_Obj *) OSAL_memAlloc(sizeof(_VHW_Obj),
            OSAL_MEM_ARG_STATIC_ALLOC);

    /*
     * Zero TX/RX buffers
     */
    OSAL_memSet(_VHW_ptr->pcmRecv_ary, 0, sizeof(_VHW_ptr->pcmRecv_ary));
    OSAL_memSet(_VHW_ptr->pcmXmit_ary, 0, sizeof(_VHW_ptr->pcmXmit_ary));

    /*
     * Start in sleeping state
     */
    _VHW_sleep = 1;
    _VHW_sleepOld = 1;

    return (0);
}

void _VHW_stopVoice(
    void)
{
    OSAL_logMsg("_VHW_stopVoice ENTER\n");

    /*
     * Zero hw and app bufs
     */
    OSAL_memSet(_VHW_ptr->pcmRecv_ary, 0, sizeof(_VHW_ptr->pcmRecv_ary));
    OSAL_memSet(_VHW_ptr->pcmXmit_ary, 0, sizeof(_VHW_ptr->pcmXmit_ary));

    OSAL_logMsg("_VHW_stopVoice EXIT\n");
}

void _VHW_startVoice(
    void)
{
    OSAL_logMsg("%s:%d - _VHW_startVoice()\n", __FILE__, __LINE__);

    _VHW_ptr->app10msFlag = 0;

    return;
}

/*
 * ======== VHW_start() ========
 * Start the voice read and write tasks
 *
 * Return Values:
 * None.
 */
int _VHW_start(
    void)
{
    return (0);
}

/*
 * ======== VHW_shutdown() ========
 * This function closes the interface to the low level hardware of
 * the host processor.
 *
 * Return Values:
 * None.
 */
void _VHW_shutdown(
    void)
{
    OSAL_memFree(_VHW_ptr, 0);
}

/*
 * ======== _VHW_detach() ========
 * Puts VHW in low power mode by detaching audio.
 *
 * Return Values:
 */
void _VHW_detach(
    void)
{
    /*
     * This will cause the audio driver and vTSP to sleep.
     */
    _VHW_sleep = 1;
    DSP_codecStop();
}

/*
 * ======== _VHW_attach() ========
 * Puts VHW in full power mode by attaching audio.
 *
 * Return Values:
 */
void _VHW_attach(
    void)
{
    /*
     * This will cause the audio driver and vTSP to come out of sleep.
     */
    _VHW_sleep = 0;
}

