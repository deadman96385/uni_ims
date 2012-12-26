#ifndef _SRIL_SVCMODE_VERSION_C
#define _SRIL_SVCMODE_VERSION_C


#include <telephony/sprd_ril.h>
#include <utils/Log.h>
#include <cutils/properties.h>

#include <sys/stat.h>
#include <stdbool.h>

#include "sprd_ril_api.h"
#include "sril_svcmode.h"
#include "sril_svcmode_version.h"
#include "samsung_nv_flash.h"

extern bool IsUpdate;

void sril_ReadFtaSWVersion(int request, void *data, size_t datalen, RIL_Token t) {
	OemSvcModeRsp * pSvcRsp = NULL;
	OemSvcModeRsp * pTmpRsp = NULL;

	ALOGD("%s\n", __FUNCTION__);

	pSvcRsp =  malloc(sizeof(OemSvcModeRsp) * MAX_SVCMENU_LINE);
	if (!pSvcRsp) {
		ALOGE("%s : Memory allocation failed\n", __FUNCTION__);
	    return;
	}
	memset(pSvcRsp, 0, sizeof(OemSvcModeRsp) * MAX_SVCMENU_LINE);

	pTmpRsp = pSvcRsp;	// Line[0]
	pTmpRsp->line = 0;
	pTmpRsp->reverse = 0;
	sprintf(pTmpRsp->string, "READ FTA SW VERSION");

	pTmpRsp++;
	pTmpRsp->line = 1;
	pTmpRsp->reverse = 0;
	sprintf(pTmpRsp->string, "FTA SW VERSION : %s", FTA_SW_VERSION);

	RIL_onRequestComplete(t, RIL_E_SUCCESS, (void*)pSvcRsp, sizeof(OemSvcModeRsp) * MAX_SVCMENU_LINE);
}

void sril_ReadFtaHWVersion(int request, void *data, size_t datalen, RIL_Token t) {
	OemSvcModeRsp * pSvcRsp = NULL;
	OemSvcModeRsp * pTmpRsp = NULL;

	ALOGD("%s\n", __FUNCTION__);

	pSvcRsp =  malloc(sizeof(OemSvcModeRsp) * MAX_SVCMENU_LINE);
	if (!pSvcRsp) {
		ALOGE("%s : Memory allocation failed\n", __FUNCTION__);
	    return;
	}
	memset(pSvcRsp, 0, sizeof(OemSvcModeRsp) * MAX_SVCMENU_LINE);

	pTmpRsp = pSvcRsp;	// Line[0]
	pTmpRsp->line = 0;
	pTmpRsp->reverse = 0;
	sprintf(pTmpRsp->string, "READ FTA HW VERSION");

	pTmpRsp++;
	pTmpRsp->line = 1;
	pTmpRsp->reverse = 0;
	sprintf(pTmpRsp->string, "FTA HW VERSION : %s", FTA_HW_VERSION);

	RIL_onRequestComplete(t, RIL_E_SUCCESS, (void*)pSvcRsp, sizeof(OemSvcModeRsp) * MAX_SVCMENU_LINE);
}

#define SUCCESS 1
#define FAIL -1

void sril_ReadSWVersion(int request, void *data, size_t datalen, RIL_Token t) {
	OemSvcModeRsp * pSvcRsp = NULL;
	OemSvcModeRsp * pTmpRsp = NULL;
	int lenVersion = 0;
	char sVersion[100] = {0x0,};

	ALOGD("%s\n", __FUNCTION__);

/* This is example test code for update svcmode window */
	IsUpdate = true;  //need update this menu.

	pSvcRsp =  malloc(sizeof(OemSvcModeRsp) * MAX_SVCMENU_LINE);
	if (!pSvcRsp) {
		ALOGE("%s : Memory allocation failed\n",__FUNCTION__);
	    return;
	}
	memset(pSvcRsp, 0, sizeof(OemSvcModeRsp) * MAX_SVCMENU_LINE);

	pTmpRsp = pSvcRsp;	// Line[0]
	lenVersion = property_get("ro.build.PDA", sVersion, "Unknown");
	pTmpRsp->line = 0;
	pTmpRsp->reverse = 0;
	sprintf(pTmpRsp->string, "%s", sVersion);

	pTmpRsp++;	// Line[1]
	pTmpRsp->line = 1;
	pTmpRsp->reverse = 0;
	sprintf(pTmpRsp->string, "Compile Date: %s", __DATE__);

	pTmpRsp++;	// Line[2]
	pTmpRsp->line = 2;
	pTmpRsp->reverse = 0;
	sprintf(pTmpRsp->string, "Compile Time: %s", __TIME__);

    RIL_onRequestComplete(t, RIL_E_SUCCESS, (void*)pSvcRsp, sizeof(OemSvcModeRsp) * MAX_SVCMENU_LINE);
}

void sril_ReadAllSWVersion(int request, void *data, size_t datalen, RIL_Token t) {
	OemSvcModeRsp * pSvcRsp = NULL;
	OemSvcModeRsp * pTmpRsp = NULL;
	int lenVersion = 0;
	char sVersion[100] = {0x0,};
	unsigned char hw_version[HW_VERSION_LENGTH];
	unsigned char calibration_date[CALIBRATION_DATE_LENGTH];

	ALOGD("%s\n", __FUNCTION__);

	pSvcRsp =  malloc(sizeof(OemSvcModeRsp) * MAX_SVCMENU_LINE);

	if (!pSvcRsp) {
		ALOGE("%s : Memory allocation failed\n",__FUNCTION__);
	    return;
	}
	memset(pSvcRsp, 0, sizeof(OemSvcModeRsp) * MAX_SVCMENU_LINE);

	pTmpRsp = pSvcRsp;	// Line[0]
	pTmpRsp->line = 0;
	pTmpRsp->reverse = 0;
	sprintf(pTmpRsp->string, "READ ALL SW VERSION");

	pTmpRsp++;	// Line[0]
	lenVersion = property_get("ro.build.PDA", sVersion, "Unknown");
	pTmpRsp->line = 1;
	pTmpRsp->reverse = 0;
	sprintf(pTmpRsp->string, "CP SW VERSION : %s", sVersion);

	pTmpRsp++;	// Line[0]
	pTmpRsp->line = 2;
	pTmpRsp->reverse = 0;
	if(Flash_Read_NV_Data(hw_version, NV_HWVERSION) == SUCCESS) {
		ALOGD("ParseBasebandVersion: hw_version:[%s]", hw_version);
		sprintf(pTmpRsp->string, "HW VERSION : %s", hw_version);
	} else {
		ALOGD("ParseBasebandVersion: hw_version:[%s]", hw_version);
        sprintf(pTmpRsp->string, "HW VERSION : %s", HW_VERSION);
	}

	pTmpRsp++;	// Line[0]
	pTmpRsp->line = 3;
	pTmpRsp->reverse = 0;
	sprintf(pTmpRsp->string, "FTA SW VERSION : %s", FTA_SW_VERSION);

	pTmpRsp++;	// Line[0]
	pTmpRsp->line = 4;
	pTmpRsp->reverse = 0;
	sprintf(pTmpRsp->string, "FTA HW VERSION : %s", FTA_HW_VERSION);

	pTmpRsp++;	// Line[0]
	pTmpRsp->line = 5;
	pTmpRsp->reverse = 0;
	if(Flash_Read_NV_Data(calibration_date, NV_PRDINFO) == SUCCESS) {
		ALOGD("ParseBasebandVersion: calibration_date:[%s]", calibration_date);
		sprintf(pTmpRsp->string, "RF Cal Date : %s", calibration_date);
	} else {
		ALOGD("ParseBasebandVersion: calibration_date:[%s]", calibration_date);
        sprintf(pTmpRsp->string, "RF Cal Date : %s", RF_CAL_DATE);
	}

    RIL_onRequestComplete(t, RIL_E_SUCCESS, (void*)pSvcRsp, sizeof(OemSvcModeRsp) * MAX_SVCMENU_LINE);
}

#endif //_SRIL_SVCMODE_VERSION_C
