#ifndef _SRIL_SVCMODE_C
#define _SRIL_SVCMODE_C

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
#include "sril_svcmode.h"
#include "sril_svcmode_menu.h"
#include "samsung_nv_flash.h"

/* ************** DEFINITIONS ************** */

#ifdef LOG_TAG
#undef LOG_TAG
#endif /* LOG_TAG */
#define LOG_TAG "SRIL_SVC"

#define SUCCESS 1
#define FAIL -1

/* ************** VARIABLES AND DATATYPES ************** */

bool IsUpdate = false;
unsigned long int vUpdateMenuId ;

static int vCurrMenuId, vPrevMenuId;
static int vCurrDepth;
static t_SvcMenuStruct* aSvcMenuStack[MAX_SVCMENU_DEPTH];
static t_SvcMenuStruct* pCurrMenu;
static bool    bNaviMenu;

//++ JSHAN_GPRS_Attach_Mode
static bool bGprsAttach = false;
//++ JSHAN Attach for next power on
#define NETWORK_GPRS_ATTACH_MODE "persist.radio.gprsattachmode"
//-- JSHAN Attach for next power on
//-- JSHAN_GPRS_Attach_Mode

/* ************** FUNCTIONS ************** */

void sril_SvcEnterModeMessage(int request, void *data, size_t datalen, RIL_Token t);
void sril_SvcProcessKeyMessage(int request, void *data, size_t datalen, RIL_Token t);
void sril_SvcEndModeMessage(int request, void *data, size_t datalen, RIL_Token t);

void                sril_SvcMenuInit(void);
unsigned int        sril_ProcessSvcMenu(t_SvcMenuStruct * pMenu, OemSvcModeRsp ** pMenuStr);
void                sril_PushSvcMenu(t_SvcMenuStruct * pMenu);
t_SvcMenuStruct*    sril_PopSvcMenu(void);
int                 sril_GetMenuIdByKeycode (unsigned char keyCode);

void sril_ToggleAutoAnswer(int request, void *data, size_t datalen, RIL_Token t); // CWYoon 110526::Factory test, Auto answering

/* ************** CODE ************** */

void sril_SvcModeHander(int request, void *data, size_t datalen, RIL_Token t) {
    OemReqMsgHdr * pSvcCmd = (OemReqMsgHdr *)data;
    ALOGD("%s: svcMode[%x]\n", __FUNCTION__, pSvcCmd->subfId);

    switch(pSvcCmd->subfId) {
        case OEM_SVC_ENTER_MODE_MESSAGE:
            sril_SvcEnterModeMessage(request, data, datalen, t);
            break;

        case OEM_SVC_PROCESS_KEY_MESSAGE:
            sril_SvcProcessKeyMessage(request, data, datalen, t);
            break;

        case OEM_SVC_END_MODE_MESSAGE:
            sril_SvcEndModeMessage(request, data, datalen, t);
            break;

        case OEM_SVC_GET_DISPLAY_DATA_MESSAGE:
        case OEM_SVC_QUERY_DISPLAY_DATA_MESSAGE:
        case OEM_SVC_DEBUG_DUMP_MESSAGE:
        case OEM_SVC_DEBUG_STRING_MESSAGE:
        default:
            ALOGD("%s: Not supported svcMode [%x]\n",__FUNCTION__, pSvcCmd->subfId);
            RIL_onRequestComplete(t, RIL_E_REQUEST_NOT_SUPPORTED, NULL, 0);
            break;
    }
}

void sril_SvcEnterModeMessage(int request, void *data, size_t datalen, RIL_Token t) {
    OemSvcEnterModeMsg * pSvcCmd = (OemSvcEnterModeMsg *)data;

    ALOGD("sril_SvcEnterModeMessage = %s: modeType[%x]\n",__FUNCTION__, pSvcCmd->modeType);
    ALOGD("sril_SvcEnterModeMessage = %s: subType[%x]\n",__FUNCTION__, pSvcCmd->subType);

    ALOGD("%s: IsUpdate [%d]\n", __FUNCTION__, IsUpdate);

    if(IsUpdate) { //if svc window need to update
        ALOGD("sril_SvcEnterModeMessage IsUpdate = true \n");
        pSvcCmd->modeType = SVC_MODE_MONITOR;
    } else {
        sril_SvcMenuInit();

        switch(pSvcCmd->modeType) {
            case SVC_MODE_TEST_MANUAL: {
                switch(pSvcCmd->subType) {
                    case TST_TESTMODE_ENTER: {
                        OemSvcModeRsp * pSvcRsp;
                        unsigned int size;

                        ALOGD("%s: Entering menu [%x]\n",__FUNCTION__, pSvcCmd->subType);

                        vCurrMenuId = ID_SVCMENU_MAIN;
                        pCurrMenu = aSvcMainMenu;

                        size = sril_ProcessSvcMenu(pCurrMenu, &pSvcRsp);

                        RIL_onRequestComplete(t, RIL_E_SUCCESS, (void*)pSvcRsp, size);
                        break;
                    }
                    case TST_SW_VERSION_ENTER:
                        ALOGD("%s: Entering TST_SW_VERSION_ENTER]\n", __FUNCTION__);
                        sril_ReadSWVersion(request, data, datalen, t);
                        break;

                    case TST_FTA_SW_VERSION_ENTER:    //*#1111# : OEM_SM_TYPE_SUB_TST_FTA_SW_VERSION_ENTER
                        ALOGD("%s: Entering TST_FTA_SW_VERSION_ENTER]\n", __FUNCTION__);
                        sril_ReadFtaSWVersion(request, data, datalen, t);
                        break;

                    case TST_FTA_HW_VERSION_ENTER:    //*#2222# : OEM_SM_TYPE_SUB_TST_FTA_HW_VERSION_ENTER
                        ALOGD("%s: Entering TST_FTA_HW_VERSION_ENTER]\n", __FUNCTION__);
                        sril_ReadFtaHWVersion(request, data, datalen, t);
                        break;

                    case TST_ALL_VERSION_ENTER:
                        ALOGD("%s: Entering TST_ALL_VERSION_ENTER]\n", __FUNCTION__);
                        sril_ReadAllSWVersion(request, data, datalen, t);
                        break;

                    //++ JSHAN_GPRS_Attach_Mode
                    case TST_GPRS_ATTACH_MODE_ENTER:
                           bGprsAttach = 1;
                           // FIXME: check usage of CP here
                        break;
                    //-- JSHAN_GPRS_Attach_Mode

                    case TST_AUTO_ANSWER_ENTER: // CWYoon 110526::Factory test, Auto answering
                        ALOGD("%s: Entering TST_AUTO_ANSWER_ENTER]\n", __FUNCTION__);
                        sril_ToggleAutoAnswer(request, data, datalen, t);
                        break;

                    case TST_SELLOUT_SMS_ENABLE_ENTER:
                        sril_SetSelloutSmsOn(request, data, datalen, t);
                        break;

                    case TST_SELLOUT_SMS_DISABLE_ENTER:
                        sril_SetSelloutSmsOff(request, data, datalen, t);
                        break;

                    case TST_SELLOUT_SMS_TEST_MODE_ON:
                        sril_SetSelloutSmsTestMode(request, data, datalen, t);
                        break;

                    case TST_SELLOUT_SMS_PRODUCT_MODE_ON:
                        sril_SetSelloutSmsProductMode(request, data, datalen, t);
                        break;

                    case TST_GET_SELLOUT_SMS_INFO_ENTER:
                        sril_GetSelloutSmsInfo(request, data, datalen, t);
                        break;

                    case TST_RRC_VERSION_ENTER: {
                        OemSvcModeRsp * pSvcRsp;
                        unsigned int size;

                        ALOGD("Entering TST_RRC_VERSION_ENTER \n");

                        vCurrMenuId = ID_SVCMENU_RRC_HSPA_CONTROL;
                        pCurrMenu = aSvcRRCHspaControlMenu;

                        size = sril_ProcessSvcMenu(pCurrMenu, &pSvcRsp);

                        RIL_onRequestComplete(t, RIL_E_SUCCESS, (void*)pSvcRsp, size);
                        break;
                    }
                    default:
                        // FIXME: implement rest of commands here
                        break;
                }
                break;
            }

            case SVC_MODE_TEST_AUTO:                /* 0x02 : Auto test mode */
                break;

            case SVC_MODE_MONITOR: {               /* 0x04 : Monitor screen mode */
                IsUpdate = true;    //for update info. on svc screen periodically
                ALOGD("%s: SVC_MODE_MONITOR [%d]\n",__FUNCTION__, IsUpdate);
                break;
            }

            case SVC_MODE_NAM:                      /* 0x03 : Full NAM edit mode */
            case SVC_MODE_PHONE_TEST:               /* 0x05 : Phone test mode ( just for the debugging ) */
            case SVC_MODE_OPERATOR_SPECIFIC_TEST:   /*0x06: Specific test mode required by operator*/
            default:
                ALOGD("%s: Not supported modeType [%x]\n", __FUNCTION__, pSvcCmd->modeType);
                RIL_onRequestComplete(t, RIL_E_REQUEST_NOT_SUPPORTED, NULL, 0);
                break;
        }
    }
}

void sril_SvcProcessKeyMessage(int request, void *data, size_t datalen, RIL_Token t) {
    OemSvcProcKeyMsg * pSvcCmd = (OemSvcProcKeyMsg *)data;
    t_SvcMenuStruct * pNextMenu;
    t_SvcMenuStruct * pSelectedMenu;

    unsigned char index;

    ALOGD("%s: keyCode[%x]\n", __FUNCTION__, pSvcCmd->keyCode);
    ALOGD("%s: IsUpdate[%d]\n", __FUNCTION__, IsUpdate);

    //++ JSHAN_GPRS_Attach_Mode
    if(bGprsAttach) {
        //++ JSHAN Attach for next power on
        int attach_mode = 0;
        char tmpStrGPRSattch[5];

        if (pSvcCmd->keyCode == 49) {
            attach_mode = 1;
            sprintf(&tmpStrGPRSattch[0], "%d", attach_mode);
            property_set(NETWORK_GPRS_ATTACH_MODE, tmpStrGPRSattch);
        } else if (pSvcCmd->keyCode == 50) {
            attach_mode = 2;
            sprintf(&tmpStrGPRSattch[0], "%d", attach_mode);
            property_set(NETWORK_GPRS_ATTACH_MODE, tmpStrGPRSattch);
        }
        //-- JSHAN Attach for next power on

        return;
    }
    //-- JSHAN_GPRS_Attach_Mode

    if(vCurrMenuId == ID_SVCMENU_INIT) {
        ALOGD("%s: ID_SVCMENU_INIT\n",__FUNCTION__);
        ALOGD("%s: Not supported keyCode [%x]\n",__FUNCTION__, pSvcCmd->keyCode);
        IsUpdate = false;
        RIL_onRequestComplete(t, RIL_E_REQUEST_NOT_SUPPORTED, NULL, 0);
        return;
    }

    if(pSvcCmd->keyCode == '0') {
        RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
    } else if(pSvcCmd->keyCode > '0' && pSvcCmd->keyCode <= '9' && bNaviMenu == 1) {
        OemSvcModeRsp * pSvcRsp;
        unsigned int size;

        index = pSvcCmd->keyCode - '0';
        pSelectedMenu = pCurrMenu + index;

        sril_PushSvcMenu(pCurrMenu);

        if(pSelectedMenu->customFunc) {
            bNaviMenu = 0;
            ALOGD("pSelectedMenu->customFunc \n");

            pSelectedMenu->customFunc(request, data, datalen, t);
            return;
        }

        vCurrMenuId = sril_GetMenuIdByKeycode(pSvcCmd->keyCode);

        pNextMenu = (t_SvcMenuStruct*) pSelectedMenu->childNode;
        size = sril_ProcessSvcMenu(pNextMenu, &pSvcRsp);

        if(pNextMenu)
            pCurrMenu = pNextMenu;

        ALOGD("%s: Current Menu Id [%x]\n",__FUNCTION__, vCurrMenuId);

        RIL_onRequestComplete(t, RIL_E_SUCCESS, (void*)pSvcRsp, size);
        return;
    } else if(pSvcCmd->keyCode == 0x5C)    { // BACK Key
        OemSvcModeRsp * pSvcRsp;
        unsigned int size;

        IsUpdate = false;

        pNextMenu = sril_PopSvcMenu();
        if(pNextMenu) {
            size = sril_ProcessSvcMenu(pNextMenu, &pSvcRsp);
            pCurrMenu = pNextMenu;
            vCurrMenuId = pCurrMenu->gId;

            RIL_onRequestComplete(t, RIL_E_SUCCESS, (void*)pSvcRsp, size);
        } else {
            ALOGE("%s: No previous menu\n",__FUNCTION__);
            RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
        }

        ALOGD("%s: Current Menu Id [%x]\n",__FUNCTION__, vCurrMenuId);

        if(!bNaviMenu)
            bNaviMenu = 1;

        return;
    } else {
        RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
    }
}

void sril_SvcEndModeMessage(int request, void *data, size_t datalen, RIL_Token t) {
    OemSvcEndModeMsg * pSvcCmd = (OemSvcEndModeMsg *)data;

    ALOGD("%s: modeType[%x]\n",__FUNCTION__, pSvcCmd->modeType);

    IsUpdate = false;
    //++ JSHAN_GPRS_Attach_Mode
    bGprsAttach = false;
    //-- JSHAN_GPRS_Attach_Mode
    sril_SvcMenuInit();
    RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
}

void sril_SvcMenuInit(void) {
    ALOGD("sril_SvcMenuInit \n");

    vCurrMenuId = vPrevMenuId = ID_SVCMENU_INIT;
    vCurrDepth = 0;
    pCurrMenu = NULL;
    bNaviMenu = true;

    memset(aSvcMenuStack, 0x0, sizeof(aSvcMenuStack));
}

void sril_PushSvcMenu(t_SvcMenuStruct * pMenu) {
    int i = 0;

    if(vCurrDepth >= MAX_SVCMENU_DEPTH) {
        ALOGE("%s: Menu Stack Full [%d]",__FUNCTION__, vCurrDepth);
        vCurrDepth = MAX_SVCMENU_DEPTH - 1;
        return;
    }

    aSvcMenuStack[vCurrDepth] = pMenu;
    vCurrDepth++;

    ALOGD("%s: vCurrDepth [%d]\n",__FUNCTION__, vCurrDepth);
    for(i = 0; i < vCurrDepth; i++) {
        ALOGD("<<%d>> %s\n", i, (aSvcMenuStack[i])->menuString);
    }
}

t_SvcMenuStruct *  sril_PopSvcMenu(void) {
    t_SvcMenuStruct * pMenu = NULL;
    int i = 0;

    vCurrDepth--;

    if(vCurrDepth < 0) {
        ALOGE("%s: Menu Stack Empty [%d]",__FUNCTION__, vCurrDepth);
        vCurrDepth = 0;
        return NULL;
    }

    pMenu = aSvcMenuStack[vCurrDepth];
    aSvcMenuStack[vCurrDepth] = NULL;

    ALOGD("%s: vCurrDepth [%d]\n",__FUNCTION__, vCurrDepth);

    for(i = 0; i < vCurrDepth; i++) {
        ALOGD("[%d] %s\n", i, (aSvcMenuStack[i])->menuString);
    }

    return pMenu;
}

int sril_GetMenuIdByKeycode (unsigned char keyCode) {
    unsigned int vMenuId;
    char vMaxMenuDepth = 8;

    if(keyCode >= '0' && keyCode < '9') {
        vMenuId = (keyCode - '0') & 0x0000000f;
        vMenuId = vCurrMenuId | ( vMenuId << (vMaxMenuDepth-vCurrDepth)*4);
    } else if(keyCode == 0x5C)    // BACK Key
        vMenuId = vPrevMenuId;
    else
        vMenuId = ID_SVCMENU_END;

    return vMenuId;
}

unsigned int sril_ProcessSvcMenu(t_SvcMenuStruct * pMenu, OemSvcModeRsp ** pMenuStr) {
    OemSvcModeRsp * lpMenuStr = NULL;
    OemSvcModeRsp * pTmpStr = NULL;
    t_SvcMenuStruct * pTmpMenu = NULL;

    unsigned int size;

    lpMenuStr = (OemSvcModeRsp *) malloc(sizeof(OemSvcModeRsp) * MAX_SVCMENU_LINE);

    if(!lpMenuStr) {
        ALOGE("%s: malloc failed\n", __FUNCTION__);
        return 0;
    }

    memset(lpMenuStr, 0x0, sizeof(OemSvcModeRsp) * MAX_SVCMENU_LINE);
    pTmpStr = lpMenuStr;

    if (pMenu == NULL) {
        pTmpStr->line = 0;
        pTmpStr->reverse = 0;

        sprintf(pTmpStr->string, "FUCTION NOT IMPLEMENTED !!!");
    } else {
        int i;

        for(i = 0, pTmpMenu = pMenu; pTmpMenu->gId != ID_SVCMENU_END; pTmpMenu++) {
            pTmpStr->line = i;
            pTmpStr->reverse = 0;
            sprintf(pTmpStr->string, "%s", pTmpMenu->menuString);

            if(i == 0 && pTmpMenu->customFunc) {
                char * infoStr = (char *)malloc(MAX_SVCSTR_PER_LINE);
                int strLen = pTmpMenu->customFunc(0, (void *)infoStr, MAX_SVCSTR_PER_LINE, 0);

                strncat(pTmpStr->string, (char*)infoStr, MAX_SVCSTR_PER_LINE - strlen(pTmpStr->string) - 1);
                // don't we need to free() infoStr here?
            }

            ALOGD("%s\n", pTmpStr->string);

            pTmpStr++;
            i++;
        }
    }

    *pMenuStr = lpMenuStr;
    //size = sizeof(OemSvcModeRsp) * i;
    size = sizeof(OemSvcModeRsp) * MAX_SVCMENU_LINE;    //for full update
    return size;
}

/* CWYoon 110527::Factory test, Auto answering ++ */
int sril_GetAutoAnswerValue(int request, void *data, size_t datalen, RIL_Token t) {
    char *    AUTO_ANS = "ril.AUTO_ANS";
    char value[PROPERTY_VALUE_MAX];

    ALOGD("%s\n", __FUNCTION__);

    property_get(AUTO_ANS, value, NULL);

    if (!strncmp(value, "on", 2))
        strcpy((char*) data, " : ON");
    else
        strcpy((char*) data, " : OFF");

    return strlen((char*) data);
}

int sril_SetOnAutoAnswerValue(int request, void *data, size_t datalen, RIL_Token t) {
    char *    AUTO_ANS = "ril.AUTO_ANS";

    OemSvcModeRsp * pSvcRsp = NULL;
    ALOGD("%s\n", __FUNCTION__);

    pSvcRsp =  malloc(sizeof(OemSvcModeRsp) * MAX_SVCMENU_LINE);
    if (!pSvcRsp) {
        ALOGE("%s : Memory allocation failed\n",__FUNCTION__);
        return(0);
    }
    memset(pSvcRsp, 0, sizeof(OemSvcModeRsp) * MAX_SVCMENU_LINE);

    property_set(AUTO_ANS, "on"); // CWYoon 110523::king debugging

    pSvcRsp->line = 0;
    pSvcRsp->reverse = 0;
    sprintf(pSvcRsp->string, "AUTO ANSWERING ON");

    RIL_onRequestComplete(t, RIL_E_SUCCESS, (void*)pSvcRsp, sizeof(OemSvcModeRsp) * MAX_SVCMENU_LINE);
  return(0);
}

int sril_SetOffAutoAnswerValue(int request, void *data, size_t datalen, RIL_Token t) {
    char *    AUTO_ANS = "ril.AUTO_ANS";

    OemSvcModeRsp * pSvcRsp = NULL;
    ALOGD("%s\n", __FUNCTION__);

    pSvcRsp =  malloc(sizeof(OemSvcModeRsp) * MAX_SVCMENU_LINE);
    if (!pSvcRsp) {
        ALOGE("%s : Memory allocation failed\n",__FUNCTION__);
        return(0);
    }
    memset(pSvcRsp, 0, sizeof(OemSvcModeRsp) * MAX_SVCMENU_LINE);

    property_set(AUTO_ANS, "off"); // CWYoon 110523::king debugging

    pSvcRsp->line = 0;
    pSvcRsp->reverse = 0;
    sprintf(pSvcRsp->string, "AUTO ANSWERING OFF");

    RIL_onRequestComplete(t, RIL_E_SUCCESS, (void*)pSvcRsp, sizeof(OemSvcModeRsp) * MAX_SVCMENU_LINE);
  return(0);
}

void sril_ToggleAutoAnswer(int request, void *data, size_t datalen, RIL_Token t) {// CWYoon 110527::king debugging
    OemSvcModeRsp * pSvcRsp = NULL;
    OemSvcModeRsp * pTmpRsp = NULL;
    char *    AUTO_ANS = "ril.AUTO_ANS";
    char value[PROPERTY_VALUE_MAX];

    ALOGD("%s\n", __FUNCTION__);

    pSvcRsp =  malloc(sizeof(OemSvcModeRsp) * MAX_SVCMENU_LINE);
    if (!pSvcRsp) {
        ALOGE("%s : Memory allocation failed\n",__FUNCTION__);
        return;
    }
    memset(pSvcRsp, 0, sizeof(OemSvcModeRsp) * MAX_SVCMENU_LINE);

    pTmpRsp = pSvcRsp;    // Line[0]
    pTmpRsp->line = 0;
    pTmpRsp->reverse = 0;

    property_get(AUTO_ANS, value, "off");

    if (!strncmp(value, "on", 2)) {
        property_set(AUTO_ANS, "off");
        sprintf(pTmpRsp->string, "AUTO ANSWER : OFF");
    } else {
        property_set(AUTO_ANS, "on");
        sprintf(pTmpRsp->string, "AUTO ANSWER : ON");
    }

    RIL_onRequestComplete(t, RIL_E_SUCCESS, (void*)pSvcRsp, sizeof(OemSvcModeRsp) * MAX_SVCMENU_LINE);
}
/* -- CWYoon 110526 */

/* Factory test, Sellout SMS : Junhee_110710 */
int sril_SelloutSms(int request, void *data, size_t datalen, RIL_Token t) {
    unsigned int     sellout_sms_mode = false;
    unsigned int     sellout_opr_mode = false;
    unsigned short    sellout_product_info = true;

    ALOGD("%s\n", __FUNCTION__);

    /* 1. Sellout SMS : ON/OFF (toggle) */
    if( Flash_Read_NV_Data(&sellout_sms_mode, NV_SELLOUT_SMS_MODE_I) != SUCCESS )
        sellout_sms_mode = false;
    ALOGD("sellout_sms_mode : [%d]\n", sellout_sms_mode);

    if( sellout_sms_mode == true )
        strcpy((char*) data, " : ON");
    else
        strcpy((char*) data, " : OFF");

    /* 2. Mode : Prod/Test (toggle) */
    if( Flash_Read_NV_Data(&sellout_opr_mode, NV_SELLOUT_OPR_MODE_I) != SUCCESS )
        sellout_opr_mode = false;
    ALOGD("sellout_opr_mode : [%d]\n", sellout_opr_mode);

    if( sellout_opr_mode == true ) {
        strcat((char*) data, " / Test mode");
    } else {
        if( Flash_Read_NV_Data(&sellout_product_info, NV_SELLOUT_PRODUCT_INFO_I) != SUCCESS )
            sellout_product_info = true;    //defult (1) : test
        ALOGD("sellout_product_info : [%d]\n", sellout_product_info);

        if( sellout_product_info == true )    // 1 : test
            strcat((char*) data, " / Test mode");
        else        // 0 : Product
            strcat((char*) data, " / Product mode");
    }

    return strlen((char*) data);
}

int sril_SetSelloutSmsOn(int request, void *data, size_t datalen, RIL_Token t) {
    OemSvcModeRsp * pSvcRsp = NULL;
    OemSvcModeRsp * pTmpRsp = NULL;
    unsigned int sellout_sms_mode = true;    //On

    ALOGD("%s\n",__FUNCTION__);

    pSvcRsp =  malloc(sizeof(OemSvcModeRsp) * MAX_SVCMENU_LINE);
    if (!pSvcRsp) {
        ALOGE("%s : Memory allocation failed\n",__FUNCTION__);
        return(0);
    }
    memset(pSvcRsp, 0, sizeof(OemSvcModeRsp) * MAX_SVCMENU_LINE);

    pTmpRsp = pSvcRsp;    // Line[0]
    pTmpRsp->line = 0;
    pTmpRsp->reverse = 0;

    Flash_Write_NV_Data(&sellout_sms_mode, NV_SELLOUT_SMS_MODE_I);

    Flash_Read_NV_Data(&sellout_sms_mode, NV_SELLOUT_SMS_MODE_I);
    ALOGD("sellout_sms_mode : [%d]\n", sellout_sms_mode);

    sprintf(pTmpRsp->string, "SMS to be sent");

    RIL_onRequestComplete(t, RIL_E_SUCCESS, (void*)pSvcRsp, sizeof(OemSvcModeRsp) * MAX_SVCMENU_LINE);
  return(0);
}

int sril_SetSelloutSmsOff(int request, void *data, size_t datalen, RIL_Token t) {
    OemSvcModeRsp * pSvcRsp = NULL;
    OemSvcModeRsp * pTmpRsp = NULL;
    unsigned int sellout_sms_mode = false;    //Off

    ALOGD("%s\n",__FUNCTION__);

    pSvcRsp =  malloc(sizeof(OemSvcModeRsp) * MAX_SVCMENU_LINE);
    if (!pSvcRsp) {
        ALOGE("%s : Memory allocation failed\n", __FUNCTION__);
        return(0);
    }
    memset(pSvcRsp, 0, sizeof(OemSvcModeRsp) * MAX_SVCMENU_LINE);

    pTmpRsp = pSvcRsp;    // Line[0]
    pTmpRsp->line = 0;
    pTmpRsp->reverse = 0;

    Flash_Write_NV_Data(&sellout_sms_mode, NV_SELLOUT_SMS_MODE_I);

    Flash_Read_NV_Data(&sellout_sms_mode, NV_SELLOUT_SMS_MODE_I);
    ALOGD("sellout_sms_mode : [%d]\n", sellout_sms_mode);

    sprintf(pTmpRsp->string, "SMS not to be sent");

    RIL_onRequestComplete(t, RIL_E_SUCCESS, (void*)pSvcRsp, sizeof(OemSvcModeRsp) * MAX_SVCMENU_LINE);
  return(0);
}

int sril_SetSelloutSmsTestMode(int request, void *data, size_t datalen, RIL_Token t) {
    OemSvcModeRsp * pSvcRsp = NULL;
    OemSvcModeRsp * pTmpRsp = NULL;
    unsigned int sellout_opr_mode = true;    //Test mode

    ALOGD("%s\n", __FUNCTION__);

    pSvcRsp =  malloc(sizeof(OemSvcModeRsp) * MAX_SVCMENU_LINE);

    if (!pSvcRsp) {
        ALOGE("%s : Memory allocation failed\n",__FUNCTION__);
        return(0);
    }
    memset(pSvcRsp, 0, sizeof(OemSvcModeRsp) * MAX_SVCMENU_LINE);

    pTmpRsp = pSvcRsp;    // Line[0]
    pTmpRsp->line = 0;
    pTmpRsp->reverse = 0;

    Flash_Write_NV_Data(&sellout_opr_mode, NV_SELLOUT_OPR_MODE_I);

    Flash_Read_NV_Data(&sellout_opr_mode, NV_SELLOUT_OPR_MODE_I);
    ALOGD("sellout_opr_mode : [%d]\n", sellout_opr_mode);

    sprintf(pTmpRsp->string, "SMS Set Test Mode");

    RIL_onRequestComplete(t, RIL_E_SUCCESS, (void*)pSvcRsp, sizeof(OemSvcModeRsp) * MAX_SVCMENU_LINE);
  return(0);
}

int sril_SetSelloutSmsProductMode(int request, void *data, size_t datalen, RIL_Token t) {
    OemSvcModeRsp * pSvcRsp = NULL;
    OemSvcModeRsp * pTmpRsp = NULL;
    unsigned int sellout_opr_mode = false;    //Product mode

    ALOGD("%s\n", __FUNCTION__);

    pSvcRsp =  malloc(sizeof(OemSvcModeRsp) * MAX_SVCMENU_LINE);
    if (!pSvcRsp) {
        ALOGE("%s : Memory allocation failed\n",__FUNCTION__);
        return(0);
    }
    memset(pSvcRsp, 0, sizeof(OemSvcModeRsp) * MAX_SVCMENU_LINE);

    pTmpRsp = pSvcRsp;    // Line[0]
    pTmpRsp->line = 0;
    pTmpRsp->reverse = 0;

    Flash_Write_NV_Data(&sellout_opr_mode, NV_SELLOUT_OPR_MODE_I);

    Flash_Read_NV_Data(&sellout_opr_mode, NV_SELLOUT_OPR_MODE_I);
    ALOGD("sellout_opr_mode : [%d]\n", sellout_opr_mode);

    sprintf(pTmpRsp->string, "SMS Set Product Mode");

    RIL_onRequestComplete(t, RIL_E_SUCCESS, (void*)pSvcRsp, sizeof(OemSvcModeRsp) * MAX_SVCMENU_LINE);
  return(0);
}

int sril_GetSelloutSmsInfo(int request, void *data, size_t datalen, RIL_Token t) {
    OemSvcModeRsp * pSvcRsp = NULL;
    OemSvcModeRsp * pTmpRsp = NULL;
    unsigned int     sellout_sms_mode = false;
    unsigned int     sellout_opr_mode = false;
    char    sellout_product_code[20] = {'\0'};
    unsigned short    sellout_product_info = true;

    ALOGD("%s\n", __FUNCTION__);

    pSvcRsp =  malloc(sizeof(OemSvcModeRsp) * MAX_SVCMENU_LINE);
    if (!pSvcRsp) {
        ALOGE("%s : Memory allocation failed\n",__FUNCTION__);
        return(0);
    }
    memset(pSvcRsp, 0, sizeof(OemSvcModeRsp) * MAX_SVCMENU_LINE);

    /* 1. Sellout SMS : ON/OFF */
    pTmpRsp = pSvcRsp;    // Line[0]
    pTmpRsp->line = 0;
    pTmpRsp->reverse = 0;

    if( Flash_Read_NV_Data(&sellout_sms_mode, NV_SELLOUT_SMS_MODE_I) != SUCCESS )
        sellout_sms_mode = false;
    ALOGD("sellout_sms_mode : [%d]\n", sellout_sms_mode);

    if( sellout_sms_mode == true )
        sprintf(pTmpRsp->string, " [1] Sellout SMS : ON");
    else
        sprintf(pTmpRsp->string, " [1] Sellout SMS : OFF");

    /* 2. PCode : NV/Default Just Display */
    pTmpRsp++;    // Line[1]
    pTmpRsp->line = 1;
    pTmpRsp->reverse = 0;

    if( Flash_Read_NV_Data(sellout_product_code, NV_SELLOUT_PRODUCT_CODE_I) != SUCCESS ) {
        /*Set Default Model Code*/
        strncpy(sellout_product_code, "GT-ixxxxXITEST", sizeof(sellout_product_code));
    }

    ALOGD("sellout_product_code : [%s]\n", sellout_product_code);
    sprintf(pTmpRsp->string, " [2] PCode : %s", sellout_product_code);

    /* 3. Mode : Prod/Test(toggle)*/
    pTmpRsp++;    // Line[2]
    pTmpRsp->line = 2;
    pTmpRsp->reverse = 0;

    if( Flash_Read_NV_Data(&sellout_opr_mode, NV_SELLOUT_OPR_MODE_I) != SUCCESS )
        sellout_opr_mode = false;
    ALOGD("sellout_opr_mode : [%d]\n", sellout_opr_mode);

    if( sellout_opr_mode == true ) {
        sprintf(pTmpRsp->string, " [3] Mode : Test");
    } else {
        if( Flash_Read_NV_Data(&sellout_product_info, NV_SELLOUT_PRODUCT_INFO_I) != SUCCESS )
            sellout_product_info = true;    //defult (1) : test
        ALOGD("sellout_product_info : [%d]\n", sellout_product_info);

        if( sellout_product_info == true )    // 1 : test
            sprintf(pTmpRsp->string, " [3] Mode : Test");
        else        // 0 : Product
            sprintf(pTmpRsp->string, " [3] Mode : Prod");
    }

    RIL_onRequestComplete(t, RIL_E_SUCCESS, (void*)pSvcRsp, sizeof(OemSvcModeRsp) * MAX_SVCMENU_LINE);
  return(0);
}

void sril_svc_update_screen(RIL_Token tid, RIL_Errno err, int nb_line, char** p_line)
{
  OemSvcModeRsp * pSvcRsp = NULL;
    OemSvcModeRsp * pTmpRsp = NULL;
    int j;
  int size = sizeof(OemSvcModeRsp) * nb_line;

    pSvcRsp =  malloc(size);

    if (!pSvcRsp)
    {
      RIL_onRequestComplete(tid, RIL_E_REQUEST_NOT_SUPPORTED, NULL, 0);
      return;
    }

    memset(pSvcRsp, 0, size);

    for(j=0; j<nb_line; j++)
  {
        if(j==0)
            pTmpRsp = pSvcRsp;    // Line[0]
        else
            pTmpRsp++;
        pTmpRsp->line = j;
        pTmpRsp->reverse = 0;
    if(p_line[j])
    {
      strncpy(pTmpRsp->string, p_line[j], MAX_SVCSTR_PER_LINE);
    }
  }

  RIL_onRequestComplete(tid, err, (void*)pSvcRsp, size);
}

void sril_svc_wbamr_response(RIL_Token tid, RIL_Errno err, void *response, size_t responselen)
{
  RIL_onRequestComplete(tid, err, response, responselen);
}

void sril_svc_band_select_response(RIL_Token tid, RIL_Errno err, void *response, size_t responselen)
{
  RIL_onRequestComplete(tid, err, response, responselen);
}

#endif    //    _SRIL_SVCMODE_C
