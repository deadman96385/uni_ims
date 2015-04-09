/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2005 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 988 $ $Date: 2006-11-02 18:47:08 -0500 (Thu, 02 Nov 2006) $
 */

#include <osal.h>
#include "vhw.h"

typedef struct {
    vint     obufi[VHW_PCM_BUF_SZ];
    vint     ibufi[VHW_PCM_BUF_SZ];
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
    OSAL_taskDelay(10);
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
    /*
     * This will cause the audio driver and vTSP to sleep.
     */
    _VHW_sleep = 1; 
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
