/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 11156 $ $Date: 2010-01-14 17:42:56 -0800 (Thu, 14 Jan 2010) $
 */

#include "vhw.h"
#include "_vhw.h"

static int _VHW_init = 0;

/*
 * Driver sleep/wake flag
 */
int _VHW_sleep = 1;
int _VHW_sleepOld = 1;

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
    int retval;

    /*
     * Init the VHW private object, and get the IRQ
     */
    if (_VHW_init == 0) {
        if ((retval = _VHW_allocHw()) == 0) {
            _VHW_init = 1;
        }
        return (retval);
    }
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
void VHW_shutdown(
    void)
{
    /*
     * Free resources
     */
    _VHW_shutdown();

    /* Clear Init flag */
    _VHW_init = 0;
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
    /*
     * Start
     */
    _VHW_start();
}

/*
 * ======== VHW_exchange() ========
 * Application calls this function to get data from the PCM highway.
 * Two pointers are returned.
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
    vint **tx_ptr,
    vint **rx_ptr)
{
    /*
     * Blocking request for data
     */
    _VHW_sleepForPcmData(tx_ptr, rx_ptr);
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
