#ifndef _SRIL_SVCMODE_VERSION_H
#define _SRIL_SVCMODE_VERSION_H

#include <telephony/sprd_ril.h>

#ifdef GLOBAL
#undef GLOBAL
#endif
#ifdef _SRIL_SVCMODE_VERSION_C
#define GLOBAL
#else
#define GLOBAL extern
#endif

GLOBAL void sril_ReadIMEI(int request, void *data, size_t datalen, RIL_Token t);
GLOBAL void sril_ReadIMSI(int request, void *data, size_t datalen, RIL_Token t);
GLOBAL void sril_ReadSWVersion(int request, void *data, size_t datalen, RIL_Token t);
GLOBAL void sril_ReadFtaSWVersion(int request, void *data, size_t datalen, RIL_Token t);
GLOBAL void sril_ReadAllSWVersion(int request, void *data, size_t datalen, RIL_Token t);
GLOBAL void sril_ReadHWVersion(int request, void *data, size_t datalen, RIL_Token t);
GLOBAL void sril_WriteHWVersion(int request, void *data, size_t datalen, RIL_Token t);
GLOBAL void sril_ReadFtaHWVersion(int request, void *data, size_t datalen, RIL_Token t);
GLOBAL void sril_ReadCalDate(int request, void *data, size_t datalen, RIL_Token t);
GLOBAL void sril_WriteCalDate(int request, void *data, size_t datalen, RIL_Token t);
GLOBAL int  sril_GetVersionValid(int request, void *data, size_t datalen, RIL_Token t);

typedef enum {
	ID_SVCMENU_VERSION_INFO= 0x20000000,
		ID_SVCMENU_INFO_SW_VERSION	= 0x21000000,
			ID_SVCMENU_SW_VERSION		= 0x21100000,
			ID_SVCMENU_READ_IMEI		= 0x21200000,
			ID_SVCMENU_READ_IMSI		= 0x21300000,
			ID_SVCMENU_READ_SW_VER		= 0x21400000,
			ID_SVCMENU_READ_FTA_SW_VER	= 0x21500000,
			ID_SVCMENU_READ_ALL_SW_VER	= 0x21600000,
		ID_SVCMENU_INFO_HW_VERSION	= 0x22000000,
			ID_SVCMENU_HW_VERSION		= 0x22100000,
			ID_SVCMENU_READ_HW_VERSION	= 0x22200000,
			ID_SVCMENU_WRITE_HW_VERSION	= 0x22300000,
			ID_SVCMENU_READ_FTA_HW_VERSION= 0x22400000,
			ID_SVCMENU_READ_CAL_DATE	= 0x22500000,
			ID_SVCMENU_WRITE_CAL_DATE	= 0x22600000,
}t_SVCMENU_ID_VERSION;

#ifdef _SRIL_SVCMODE_VERSION_C

#define HW_VERSION			"Unknown"
#define FTA_SW_VERSION		"Unknown"
#define FTA_HW_VERSION		"Unknown"
#define RF_CAL_DATE			"Unknown"

#else
	//////////////////////////////////////////////////
	///* 2. VERSION INFORMATION
	/////////////////////////////////////////////////
t_SvcMenuStruct aSvcSWVersionMenu[] = {
	{ID_SVCMENU_SW_VERSION, 		"SW VERSION", 			NULL, 	/*uSril_GetVersionValid*/NULL, 0 },
	{ID_SVCMENU_READ_IMEI, 			"[1] READ IMEI", 		NULL, 	/*uSril_ReadIMEI*/NULL, 0},
	{ID_SVCMENU_READ_IMSI, 			"[2] READ IMSI", 		NULL, 	/*uSril_ReadIMSI*/NULL, 0 },
	{ID_SVCMENU_READ_SW_VER, 		"[3] READ SW VER", 		NULL, 	/*uSril_ReadSWVersion*/NULL, 0 },
	{ID_SVCMENU_READ_FTA_SW_VER,	"[4] READ FTA SW VER", 	NULL, 	/*uSril_ReadFtaSWVersion*/NULL, 0 },
	{ID_SVCMENU_READ_ALL_SW_VER, 	"[5] READ ALL SW VER", 	NULL, 	/*uSril_ReadAllSWVersion*/NULL, 0 },
	{ID_SVCMENU_END,				NULL,					NULL,	NULL, 0 },
};

t_SvcMenuStruct aSvcHWVersionMenu[] = {
	{ID_SVCMENU_HW_VERSION, 		"HW VERSION", 				NULL, 	NULL, 0 },
	{ID_SVCMENU_READ_HW_VERSION, 	"[1] READ HW VERSION", 		NULL, 	/*uSril_ReadHWVersion*/NULL, 0 },
	{ID_SVCMENU_WRITE_HW_VERSION, 	"[2] WRITE HW VERSION", 	NULL, 	/*uSril_WriteHWVersion*/NULL, 0 },
	{ID_SVCMENU_READ_FTA_HW_VERSION,"[3] READ FTA HW VERSION", 	NULL, 	/*uSril_ReadFtaHWVersion*/NULL, 0 },
	{ID_SVCMENU_READ_CAL_DATE,		"[4] READ CAL DATE", 		NULL, 	/*uSril_ReadCalDate*/NULL, 0 },
	{ID_SVCMENU_WRITE_CAL_DATE, 	"[5] WRITE CAL DATE", 		NULL, 	/*uSril_WriteCalDate*/NULL, 0 },
	{ID_SVCMENU_END,				NULL,						NULL,	NULL, 0 },
};

t_SvcMenuStruct aSvcVersionMenu[] = {
	{ID_SVCMENU_VERSION_INFO, 		"VERSION INFO", 	NULL, 				NULL, 0 },
	{ID_SVCMENU_INFO_SW_VERSION, 	"[1] SW VERSION", 	aSvcSWVersionMenu, 	NULL, 0 },
	{ID_SVCMENU_INFO_HW_VERSION, 	"[2] HW VERSION", 	aSvcHWVersionMenu, 	NULL, 0 },
	{ID_SVCMENU_END,				NULL,				NULL,				NULL, 0 },
};

#endif

#endif // _SRIL_SVCMODE_VERSION_H
