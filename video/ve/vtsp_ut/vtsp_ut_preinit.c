/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 5573 $ $Date: 2008-03-25 17:10:21 -0400 (Tue, 25 Mar 2008) $
 *
 */

/* Test all functions pre-init case
 * Assumes the VTSP has NOT been initialized
 *
 * When VTSP has not been initialized, all public
 * functions other than 'init' must fail.
 */

#include "osal.h"

#include "vtsp.h"

#include "vtsp_ut.h"


UT_Return UT_preInit(
    vint arg)
{
    VTSP_Version version_ptr;

    if (NULL == UT_vtsp_ptr) {
        /* control funcs */
        UT_EXPECT4(VTSP_E_INIT, VTSP_toneLocal, 0, 0, 0, 0);
        UT_EXPECT1(VTSP_E_INIT, VTSP_toneLocalStop, 0);
        UT_EXPECT3(VTSP_E_INIT, VTSP_infcControlGain, 0, 0, 0);

        /* management funcs */
        UT_EXPECT0((int)NULL, (int)VTSP_query);
        /* VTSP_init is skipped here */
        UT_EXPECT3(VTSP_E_INIT, VTSP_config, 0, 0, NULL);
        UT_EXPECT2(VTSP_E_INIT, VTSP_configTone, 0, NULL);
        UT_EXPECT0(VTSP_E_INIT, VTSP_start);
        UT_EXPECT0(VTSP_E_INIT, VTSP_shutdown);
        UT_EXPECT1(VTSP_OK, VTSP_getVersion, &version_ptr);

        /* event funcs */
        UT_EXPECT1(VTSP_E_INIT, VTSP_infcLineStatus, 0);
        UT_EXPECT3(VTSP_E_INIT, VTSP_getEvent, 0, NULL, 0);

        /* stream funcs */
        UT_EXPECT2(VTSP_E_INIT, VTSP_streamStart, 0, NULL);
        UT_EXPECT2(VTSP_E_INIT, VTSP_streamEnd, 0, 0);
        UT_EXPECT2(VTSP_E_INIT, VTSP_streamModify, 0, NULL);
        UT_EXPECT3(VTSP_E_INIT, VTSP_streamModifyDir, 0, 0, 0);
        UT_EXPECT3(VTSP_E_INIT, VTSP_streamModifyEncoder, 0, 0, 0);
        UT_EXPECT3(VTSP_E_INIT, VTSP_streamModifyConf, 0, 0, 0);
    }
    else {
        OSAL_logMsg("%s:%d - VTSP already initialized can't do preinit test" ,
                __FILE__, __LINE__);
    }

    return (UT_PASS);
}

