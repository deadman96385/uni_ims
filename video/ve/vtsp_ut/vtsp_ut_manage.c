/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 7558 $ $Date: 2008-09-08 14:21:17 -0400 (Mon, 08 Sep 2008) $
 *
 */

/* Test management functions
 * Assumes the VTSP has already been initialized
 */

#include "osal.h"
#include "vtsp.h"
#include "vtsp_ut.h"


UT_Return UT_shutdown(
    vint arg)
{
    VTSP_Context  *vtsp_ptr;

    /*
     * Sleep the VHW audio driver 
     */
    if (VTSP_OK != VTSP_infcControlIO(0, VTSP_CONTROL_INFC_IO_AUDIO_ATTACH,
            VTSP_CONTROL_INFC_IO_AUDIO_DRIVER_ENABLE)) {
        OSAL_logMsg("%s: FAILED to 'attach' the vtsp\n", __FUNCTION__);
        return (UT_FAIL);
    }

    /* attempt re-init and expect fail */
    UT_EXPECT3(VTSP_E_SHUTDOWN, VTSP_init, &vtsp_ptr, NULL, NULL);

    /* shutdown and expect success */
    UT_EXPECT0(VTSP_OK, VTSP_shutdown);

    /* Shutdown again, expect failure */

    UT_EXPECT0(VTSP_E_INIT, VTSP_shutdown);

    /* Assume Pass - set shutdown state 
     * If shutdown was a failure, this will lose current context.
     */
    UT_vtsp_ptr = NULL;

    return (UT_PASS);
}

UT_Return UT_init(
    vint arg)
{
    VTSP_Context    *vtsp_ptr;

    UT_EXPECT3(VTSP_E_SHUTDOWN, VTSP_init, &vtsp_ptr, NULL, NULL);

    return (UT_PASS);
}
