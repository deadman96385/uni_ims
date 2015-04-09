/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 516 $ $Date: 2006-09-20 20:28:39 -0400 (Wed, 20 Sep 2006) $
 *
 */

/* Test stream functions
 * VTSP must be initialized and started prior to running these functions.
 */

#include "osal.h"

#include "vtsp.h"

#include "vtsp_ut.h"

        
/* 
 * This is a stream test designed to run for VQT only.
 * For VQT there must be:
 *  - no osal msgs
 *  - no coder changes
 *  - no cid type 2
 *
 */

UT_Return UT_testStreamConfVQT(
        vint         infc,
        VTSP_Stream *stream1_ptr,
        VTSP_Stream *stream2_ptr)
{
    uvint          streamId;

    UT_run = 1;

    streamId = stream1_ptr->streamId;

    /* Turn off event printing
     */
    OSAL_logMsg("%s: Event printing OFF.\n", __FILE__);
    UT_eventTaskDisplay = 0;

    OSAL_logMsg("%s: Starting streamId %d..\n", __FILE__,
            streamId);
    OSAL_logMsg("%s: Encoder = %d.\n", __FILE__,
            stream1_ptr->encoder);

    UT_EXPECT2(VTSP_OK, VTSP_streamStart, infc, stream1_ptr);

    streamId = stream2_ptr->streamId;

    OSAL_logMsg("%s: Starting streamId %d..\n", __FILE__,
            streamId);
    OSAL_logMsg("%s: Encoder = %d.\n", __FILE__,
            stream2_ptr->encoder);

    UT_EXPECT2(VTSP_OK, VTSP_streamStart, infc, stream2_ptr);

    while (UT_run) { 
        OSAL_taskDelay(1000);
        if (UT_testTimeSec != 0) { 
            UT_testTimeSec -= 1;
            if (UT_testTimeSec <= 0) { 
                break;
            }
        }
    }

    UT_EXPECT2(VTSP_OK, VTSP_streamEnd, infc, stream1_ptr->streamId);
    UT_EXPECT2(VTSP_OK, VTSP_streamEnd, infc, stream2_ptr->streamId);


    return (UT_PASS);
}


