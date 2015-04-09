/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 12262 $ $Date: 2010-06-10 18:55:56 -0400 (Thu, 10 Jun 2010) $
 */

#include <vcd.h>
#include <vdd.h>
#include <video.h>
#include <ve.h>

#include <osal.h>

#include <video_net.h>
#include <vier.h>

static int _width = 320;
static int _height = 240;
static int _oldwidth = 320;
static int _oldheight = 240;

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
    int argc,
    char *argv[])
{
    /*
     * Initialize VIER.
     */
    if (OSAL_FAIL == VIER_init()) {
        OSAL_logMsg("%s:%d Init vier failed.\n", __FUNCTION__, __LINE__);
        return (0);
    }

    /*
     * VE
     */
    VE_init();

    _sem = OSAL_semMutexCreate();

    /*
     * Camera view
     */
    if (0 != VDD_init(VIDEO_CALLID_CAMERA, 0)) {
        VE_shutdown();
        OSAL_logMsg("%s %d\n", __FILE__, __LINE__);
        return (-1);
    }

    /*
     * Stream 0 view
     */
    if (0 != VDD_init(0, 0)) {
        VE_shutdown();
        VDD_shutdown(0);
        OSAL_logMsg("%s %d\n", __FILE__, __LINE__);
        return (-1);
    }
    
    /*
     * Stream 1 view
     */
    if (0 != VDD_init(1, 0)) {
        VE_shutdown();
        VDD_shutdown(0);
        OSAL_logMsg("%s %d\n", __FILE__, __LINE__);
        return (-1);
    }

    /*
     * Camera
     */
    if (0 != VCD_init(_width, _height, 0, 0)) {
        VE_shutdown();
        VDD_shutdown(0);
        VDD_shutdown(1);
        OSAL_logMsg("%s %d\n", __FILE__, __LINE__);
        return (-1);
    }

    while (1) {
        
        OSAL_semAcquire(_sem, OSAL_WAIT_FOREVER);
        if ((_oldwidth != _width) || (_oldheight != _height)) {
            VCD_shutdown();
            VCD_init(_width, _height, 0, 0);
            _oldwidth = _width;
            _oldheight = _height;
        }
        OSAL_semGive(_sem);
        
        OSAL_taskDelay(1000);

    }

    VE_shutdown();
    VDD_shutdown(0);
    VDD_shutdown(VIDEO_CALLID_CAMERA);
    VCD_shutdown();

    /* Shutdown VIER */
    VIER_shutdown();

    OSAL_semDelete(_sem);

    return (0);
}
