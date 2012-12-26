#ifndef _SRIL_C
#define _SRIL_C

/* ************** INCLUDES ************** */
#include <telephony/sprd_ril.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdbool.h>
#include <time.h>

#include <utils/Log.h>
#include "sprd_ril_api.h"
#include "sril.h"
#include "sril_gprs.h"
#include "fsr_if.h"

/* ************** DEFINITIONS ************** */

/* ************** VARIABLES AND DATATYPES ************** */

/* ************** FUNCTIONS ************** */

/* ************** CODE ************** */
void sril_GetDGSUniqueNumber(int request, void *data, size_t datalen, RIL_Token t)
{
    int i, fd;
    unsigned char uniqueNumber[21] = {0,};
    char temp[5], data_UN[50], cmd_buf[200];
    PAGEINFO_T    stPageinfo;

    ALOGD("%s: GetDGSUniqueNumber\n", __FUNCTION__);

    memset(data_UN, 0, sizeof(data_UN));

    fd = open("/dev/bml15", 0); //open device
    if(fd < 0) {
        ALOGD("%s: device open error\n", __FUNCTION__);
        RIL_onRequestComplete(t, RIL_E_REQUEST_NOT_SUPPORTED, NULL, 0);
        return;
}

    stPageinfo.offset = 5;
    if(ioctl(fd, LLD_DGS_READ, &stPageinfo) != 0) {
        ALOGD("%s: DGS read error\n",__FUNCTION__);
        RIL_onRequestComplete(t, RIL_E_REQUEST_NOT_SUPPORTED, NULL, 0);
        close(fd);
        return;
}

    for(i = 149; i < 169; i++) {
        sprintf(temp, "%02X", stPageinfo.mbuf[i]);
        strcat(data_UN, temp);
}
    memcpy(uniqueNumber, stPageinfo.mbuf + 149, 20);

    close(fd);

    ALOGD("data_UN=%s", data_UN);

    sprintf(cmd_buf, "broadcast -a com.android.samsungtest.DGS_UniqueNumber -e UniqueNumberKey %s", data_UN);
    RIL_onUnsolicitedResponse(RIL_UNSOL_AM, (void*)cmd_buf, strlen(cmd_buf));

    RIL_onRequestComplete(t, RIL_E_SUCCESS, (void*)uniqueNumber, sizeof(uniqueNumber));
    return;
}

void sril_OemHookRaw(int request, void *data, size_t datalen, RIL_Token t) {
    if(request == RIL_REQUEST_OEM_HOOK_RAW) {
        OemReqMsgHdr * req = (OemReqMsgHdr *) data;
        ALOGD("%s: RIL_REQUEST_OEM_HOOK_RAW [%d]\n", __FUNCTION__, req->funcId);
        switch(req->funcId) {
            case OEM_FUNCTION_ID_IMEI:
                sril_IMEIHandler(request, data, datalen, t);
                break;

            case OEM_FUNCTION_ID_SVC_MODE :
                sril_SvcModeHander(request, data, datalen, t);
                break;

            case OEM_FUNCTION_ID_FACTORY :
                //sril_FactoryTestHander(request, data, datalen, t);
                break;

            case OEM_FUNCTION_ID_SYSDUMP :
                sril_SysDumpHander(request, data, datalen, t);
                break;

            case OEM_FUNCTION_ID_PERSONALIZATION:
                //sril_PersoHandler(request, data, datalen, t);
                break;

            case OEM_FUNCTION_ID_GPRS :
                sril_GprsHander(request, data, datalen, t);
                break;

            case OEM_FUNCTION_ID_CONFIGURATION: {
                ALOGD("%s: Configuration[%x]\n", __FUNCTION__, req->subfId);

                switch(req->subfId) {
                    case OEM_CFG_GET_DGS_UNIQUENUMBER:
                        sril_GetDGSUniqueNumber(request, data, datalen, t);
                        break;

                    default :
                        ALOGD("%s: Not supported Configuration [%x]\n", __FUNCTION__, req->subfId);
                        RIL_onRequestComplete(t, RIL_E_REQUEST_NOT_SUPPORTED, NULL, 0);
                        break;
                }
                break;
            }

            case OEM_FUNCTION_ID_CALL: {
                ALOGD("%s: CallCommand[%x]\n", __FUNCTION__, req->subfId);

                switch(req->subfId) {
                    case OEM_CALL_GET_LIFETIMECALL:
                    case OEM_CALL_SET_LIFETIMECALL:
                        sril_TotalCallTimeHandler(request, data, datalen, t);
                        break;

                    default :
                        ALOGD("%s: Not supported Call Cmd [%x]\n", __FUNCTION__, req->subfId);
                        break;
                }
                break;
            }

            default :
                ALOGD("%s: Not supported Cmd [%x]\n", __FUNCTION__, req->subfId);
                break;
        }
    }
}

#endif
