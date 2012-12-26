#ifndef _SRIL_GPRS_C
#define _SRIL_GPRS_C

/* ************** INCLUDES ************** */

#include <telephony/sprd_ril.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdbool.h>
#include <time.h>

#include <cutils/properties.h>

#include <utils/Log.h>
#include "sprd_ril_api.h"
#include "sril.h"
#include "sril_gprs.h"

/* ************** CODE ************** */

void sril_GprsHander(int request, void *data, size_t datalen, RIL_Token t) {
    OemReqMsgHdr * pGprsCmd = (OemReqMsgHdr *)data;

    ALOGD("%s: subfId[%x]\n", __FUNCTION__, pGprsCmd->subfId);

    switch(pGprsCmd->subfId) {
        case OEM_GPRS_SET_DORMANCY:
            sril_OemSetDormancy(request, data, datalen, t);
            break;

        case OEM_GPRS_EXEC_DUN_PIN_CTRL:
        case OEM_GPRS_DISCONNECT_DUN:
        default: {
            ALOGD("%s: Not supported GprsCmd [%x]\n",__FUNCTION__, pGprsCmd->subfId);
            RIL_onRequestComplete(t, RIL_E_REQUEST_NOT_SUPPORTED, NULL, 0);
            break;
        }
    }
}


void sril_OemSetDormancy(int request, void *data, size_t datalen, RIL_Token t) {
    int err = RIL_E_SUCCESS;

    ALOGD("%s : Enter\n", __FUNCTION__);

    /* Implementation Here */
    if (err < 0) {
        RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);        
    } else {
        RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);        
    }
}


#endif //_SRIL_GPRS_C
