/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004-2009 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 10497 $ $Date: 2009-10-06 17:59:33 +0800 (Tue, 06 Oct 2009) $
 *
 */

#include "csm_event.h"
#include "csmsdi_input.h"
#include "csmsdi_output.h"
#include <stdio.h>

int main(int argc, char **argv)
{
    /* input checking */
    if (sizeof(CSMSDI_InputEvent) != sizeof(CSM_InputEvent)) {
        printf("error on struct size at line %d\n", __LINE__);
    }
    if (sizeof(CSMSDI_InputCall) != sizeof(CSM_InputCall)) {
        printf("error on struct size at line %d\n", __LINE__);
    }
    if (sizeof(CSMSDI_InputSms) != sizeof(CSM_InputSms)) {
        printf("error on struct size at line %d\n", __LINE__);
    }
    if (sizeof(CSMSDI_InputService) != sizeof(CSM_InputService)) {
        printf("error on struct size at line %d\n", __LINE__);
    }
    if (sizeof(CSMSDI_InputRadio) != sizeof(CSM_InputRadio)) {
        printf("error on struct size at line %d\n", __LINE__);
    }
    if (sizeof(CSMSDI_InputSupSrv) != sizeof(CSM_InputSupSrv)) {
        printf("error on struct size at line %d\n", __LINE__);
    }
    if (sizeof(CSMSDI_InputUssd) != sizeof(CSM_InputUssd)) {
        printf("error on struct size at line %d\n", __LINE__);
    }

    /* output checking */
    if (sizeof(CSMSDI_OutputEvent) != sizeof(CSM_OutputEvent)) {
        printf("error on struct size at line %d\n", __LINE__);
    }
    if (sizeof(CSMSDI_OutputCall) != sizeof(CSM_OutputCall)) {
        printf("error on struct size at line %d\n", __LINE__);
    }
    if (sizeof(CSMSDI_OutputSms) != sizeof(CSM_OutputSms)) {
        printf("error on struct size at line %d\n", __LINE__);
    }
    if (sizeof(CSMSDI_OutputService) != sizeof(CSM_OutputService)) {
        printf("error on struct size at line %d\n", __LINE__);
    }
    if (sizeof(CSMSDI_OutputSupSrv) != sizeof(CSM_OutputSupSrv)) {
        printf("error on struct size at line %d\n", __LINE__);
    }
    if (sizeof(CSMSDI_OutputUssd) != sizeof(CSM_OutputUssd)) {
        printf("error on struct size at line %d\n", __LINE__);
    }

    printf("done all sizeof testing\n");
}
