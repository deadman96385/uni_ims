/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 12262 $ $Date: 2010-06-10 18:55:56 -0400 (Thu, 10 Jun 2010) $
 */

#include <osal.h>
#include <vier.h>
#include <vce.h>

static int _width = 320;
static int _height = 240;

static OSAL_SemId _sem;

/*
 * ======== OS_changeCameraResolutionRequest() ========
 * App must implement this.
 * Returns:
 *  0: Success.
 *  !0: Failed.
 */
int OS_changeCameraResolutionRequest(
    int width,
    int height)
{
    OSAL_semAcquire(_sem, OSAL_WAIT_FOREVER);
    _width = width;
    _height = height;
    OSAL_semGive(_sem);
    return (0);
}

/*
 * ======== main() ========
 * Returns:
 *  0: Success.
 *  !0: Failed.
 */
int main(
    int   argc,
    char *argv[])
{
    signal(SIGPIPE, SIG_IGN);
    _sem = OSAL_semMutexCreate();

   VCE_init(_width, _height);

   while (1) {
        
        OSAL_semAcquire(_sem, OSAL_WAIT_FOREVER);

        OSAL_semGive(_sem);
        
        OSAL_taskDelay(1000);
    }

    VCE_shutdown();

    OSAL_semDelete(_sem);

    return (0);
}
