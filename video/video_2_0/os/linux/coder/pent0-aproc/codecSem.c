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
#include "codecSem.h"

OSAL_SemId Codec_SemLock = NULL;

/*
 * ======== Codec_SemInit() ========
 * Initialize the semaphore for avcodec open/close.
 * avcodec_register_all is necessary for avcodec.
 *
 * Returns:
 * 0 : Success
 * -1 : Failed
 */
int Codec_SemInit(
    void)
{
    if (NULL != Codec_SemLock) {
        return (-1);
    }
    Codec_SemLock = OSAL_semMutexCreate();

    if (NULL == Codec_SemLock) {
        return (-1);
    }

    if (OSAL_FAIL == OSAL_semAcquire(Codec_SemLock, OSAL_WAIT_FOREVER)) {
        return (-1);
    }

#ifndef EXCLUDE_FFMPEG_LIBRARY
    avcodec_register_all();
#endif

    if (OSAL_FAIL == OSAL_semGive(Codec_SemLock)) {
        return (-1);
    }

    return (0);
}

/*
 * ======== Codec_SemShutdown() ========
 * Shuts the semaphore.
 *
 * Returns:
 * 0 : Success
 * -1 : Failed
 */
int Codec_SemShutdown(
    void)
{
    if (NULL == Codec_SemLock) {
        return (-1);
    }
    OSAL_semDelete(Codec_SemLock);
    Codec_SemLock = NULL;

    return(0);
}
