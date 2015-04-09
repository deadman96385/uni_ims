/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004-2007 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: $ $Date: $
 *
 */


#ifndef EXCLUDE_FFMPEG_LIBRARY
#include <libavcodec/avcodec.h>
#endif
#include <osal.h>
#include "vid.h"

OSAL_SemId VID_dspLock = NULL;

/*
 * ======== VID_dspInit() ========
 * Inits the video DSP.
 *
 * Returns:
 * 0 : Success
 * -1 : Failed
 */
int VID_dspInit(
    void)
{
    if (NULL != VID_dspLock) {
        return (-1);
    }
    VID_dspLock = OSAL_semMutexCreate();

    if (NULL == VID_dspLock) {
        return (-1);
    }

    if (OSAL_FAIL == OSAL_semAcquire(VID_dspLock, OSAL_WAIT_FOREVER)) {
        return (-1);
    }

#ifndef EXCLUDE_FFMPEG_LIBRARY
    avcodec_register_all();
#endif

    if (OSAL_FAIL == OSAL_semGive(VID_dspLock)) {
        return (-1);
    }

    return (0);
}

/*
 * ======== VID_dspShutdown() ========
 * Shuts the video DSP.
 *
 * Returns:
 * 0 : Success
 * -1 : Failed
 */
int VID_dspShutdown(
    void)
{
    if (NULL == VID_dspLock) {
        return (-1);
    }
    OSAL_semDelete(VID_dspLock);
    VID_dspLock = NULL;

    return(0);
}
