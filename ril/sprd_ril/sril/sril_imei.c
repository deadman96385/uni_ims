#ifndef _SRIL_IMEI_C
#define _SRIL_IMEI_C

/* ************** INCLUDES ************** */
#include <telephony/sprd_ril.h>
#include <cutils/sockets.h>
#include <cutils/properties.h>

#include <sys/stat.h>
#include <stdbool.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <math.h>

#include <netinet/in.h>
#include <arpa/inet.h>

#include <utils/Log.h>
#include "sprd_ril_api.h"
#include "sril.h"
#include "sril_imei.h"
#include "sril_personalization.h"
#include "sril_svcmode.h"
#include "samsung_nv_flash.h"
//#include <SecNativeFeature.h>        // for CSC feature

/* ************** DEFINITIONS ************** */

#define LOG_TAG "SRIL_IMEI"

// network type
#define NETWORK_TYPE_PATH "persist.radio.networktype"
// network band
#define NETWORK_BAND_PATH "persist.radio.networkband"
/*- sh0515.lee - Band Selection for CSC update -*/

//++ JSHAN Attach for CSC update
#define NETWORK_GPRS_ATTACH_MODE "persist.radio.gprsattachmode"
//-- JSHAN Attach for CSC update

#define MAX_MSG_LENGTH 8192

// LTN_SW1_SYSTEM kwanguk.kim 2012.05.22 - TFG IMEI issue
#define MAX_TFG_CSC_NUM        12
#define MAX_CSC_CODE_LEN    3
// end - LTN_SW1_SYSTEM kwanguk.kim 2012.05.22 - TFG IMEI issue

#define ADDR "127.0.0.1"
#define PORT 9000

/* ************** VARIABLES AND DATATYPES ************** */

static int CscCompareResult = 0;

typedef struct {
    int len;
    char data[MAX_MSG_LENGTH];
} AtResultMsg;

// LTN_SW1_SYSTEM kwanguk.kim 2012.05.22 - TFG IMEI issue
static const char TFG_SalesCode_Table[MAX_TFG_CSC_NUM][MAX_CSC_CODE_LEN] = {
    "UFN", "UFU", "SAL", "EBE", "NBS", "PBS",
    "TGU", "TMM", "COB", "CHT", "SAM", "VMT"
};
// end - LTN_SW1_SYSTEM kwanguk.kim 2012.05.22 - TFG IMEI issue

/* ************** FUNCTIONS ************** */

void sril_IMEIHandler(int request, void *data, size_t datalen, RIL_Token t);

void sril_SetUpdateItemMessage(int request, void *data, size_t datalen, RIL_Token t);
void sril_CfrmUpdateItemMessage(int request, void *data, size_t datalen, RIL_Token t);
void sril_EventVerifyCompareMessage(int request, void *data, size_t datalen, RIL_Token t);
void sril_GetTotalCallTime(int request, void *data, size_t datalen, RIL_Token t);
void sril_SetTotalCallTime(int request, void *data, size_t datalen, RIL_Token t);

/* ************** CODE ************** */

// LTN_SW1_SYSTEM kwanguk.kim 2012.05.22 - TFG IMEI issue
int IsTFGSalesCode (char* sales_code) {
    int i = 0;

    for (i = 0; i < MAX_TFG_CSC_NUM; i++) {
        if (!(strncmp(sales_code, TFG_SalesCode_Table[i], MAX_CSC_CODE_LEN))) {
            ALOGD("[LTN] IsTFGSalesCode: {%s} is the kind of TFG salse code", sales_code);
            return 1;
        }
    }

    ALOGD("[LTN] IsTFGSalesCode: {%s} is not the kind of TFG salse code", sales_code);
    return 0;
}
// end - LTN_SW1_SYSTEM kwanguk.kim 2012.05.22 - TFG IMEI issue

int ConvertStrToNum(char *string) {
    int str_len = strlen(string);
    int i, digit, out_number;

    out_number = 0;

    for(i = 0; i < str_len; i++) {
        digit = (int)(string[i] - '0');
        out_number += (int)(digit * pow(10,(str_len-i-1)));
    }
    return out_number;
}

void put_factory_string(const char *s, unsigned int count) {
    int fp;

#if CONFIG_FTA_VERSION_ZANIN    // hjh temp
    fp = open( "/dev/ttyS1", O_WRONLY|O_TRUNC);
#else
    fp = open( "/dev/ttyS0", O_WRONLY|O_TRUNC);
#endif

    if (fp < 0) {
        //printk( "put_factory_string: filp_open failed.\n" ) ;
    } else {
        write(fp, s, count);
        close(fp);
    }
}

int sendReponseToATC(void * data, size_t len) {
    int fd = -1;

    struct sockaddr_in addr;

    bzero(&addr, sizeof(addr));

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(ADDR);
    addr.sin_port = htons(PORT);

    if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        return -1;

    int count = 50;
    while (count-- > 0) {
        ALOGD("U_sril_imei   send Response to AT : connect(%d) ...\n", count);
        if (connect(fd, &addr, (socklen_t) sizeof(addr)) < 0)
            usleep(100000);
        else
            break;
    }

    if (send(fd, data, len, 0) < (ssize_t) len){
        close(fd);
        return -1;
    }

    if (close(fd) < 0)
        return -1;
    return 0;
}

void compareModemLockInfo(char* data, int dataNum) {
    ALOGE("#### compareModemLockInfo ########\n");

    perso_me_data_type_t perso_data;

    FILE *fp = NULL;

    char* item = NULL;
    char* element = NULL;

    char result[40] = {0};
    char pad[17]  = "XXXXXXXXXXXXXXXX";

    int compFail = 0;             // 1 is failed, 0 is successful
    int itemNum = 0;
    int itemCnt = 0;
    int itemLen = 0;
    int elementCnt = 0;
    int elementLen = 0;

    item = (char*)data;
    itemNum = dataNum;

    int i =0;
    int nwcode = 0;
    int nscode = 0;
    int spcode = 0;
    int cpcode = 0;
    char lockcode[100] = {0x0,};
    char nvlockcode[100] = {0x0,};
    char nslockcode[100] = {0x0,};
    char nvnslockcode[100] = {0x0,};
    char splockcode[100] = {0x0,};
    char nvsplockcode[100] = {0x0,};
    char cplockcode[100] = {0x0,};
    char nvcplockcode[100] = {0x0,};

/*+ sh0515.lee - Band Selection for CSC update +*/
    char sType[PROPERTY_VALUE_MAX] = {0x0,};
    int iType = 0;
    char sBand[PROPERTY_VALUE_MAX] = {0x0,};
    int iBand = 0;
/*- sh0515.lee - Band Selection for CSC update -*/

//++ JSHAN Attach for CSC update
    int attach_mode = 0;
    char tmpStrGPRSattch[PROPERTY_VALUE_MAX];
//-- JSHAN Attach for CSC update
    //wannatea
    int amrwb = 0;
    int temp_wb_amr_mode = 0;

    memset(&perso_data, 0, sizeof(perso_data));
    Flash_Read_Secure_Data (&perso_data, SEC_LOCK );

    ALOGE("#### compareModemLockInfo  >> itemNum= [%x]\n", itemNum);

    for(itemCnt = 0; itemCnt < (int)itemNum; itemCnt++) {
        if(item[0] == 0x87) {        //IPC_IMEI_ITEM_NETWORK_MODE
            ALOGE("#### compareModemLockInfo : IPC_IMEI_ITEM_NETWORK_MODE >> item Len = [%x]\n", item[4]);
            ALOGE("#### compareModemLockInfo : IPC_IMEI_ITEM_NETWORK_MODE >> item value = [%x]\n", item[6]);

            itemLen = (int)item[4];

            // read network mode info and compare with item value
            /*+ sh0515.lee - Band Selection for CSC update +*/

            property_get(NETWORK_TYPE_PATH, sType, "0");
            property_get(NETWORK_BAND_PATH, sBand, "0");

            iType = atoi(sType);
            iBand = atoi(sBand);

            // FIXME: this should be enabled
/*            if ((item[6] == 0x01) && (iType == ReConvertNetworkType(DUAL_MODE_UMTS_PREF)) &&
                (iBand == ReConvertBandMode(BAND_AUTO))) {
                   compFail = 0;
               } else if ((item[6] == 0x02) && (iType == ReConvertNetworkType(UMTS_ONLY)) &&
                (iBand == ReConvertBandMode(BAND_ALL_UMTS))) {
                   compFail = 0;
               } else if ((item[6] == 0x03) && (iType == ReConvertNetworkType(GSM_ONLY)) &&
                (iBand == ReConvertBandMode(BAND_ALL_GSM))) {
                   compFail = 0;
               } else if ((item[6] == 0x04) && (iType == ReConvertNetworkType(GSM_ONLY)) &&
                (iBand == ReConvertBandMode(BAND_GSM900_ONLY))) {
                   compFail = 0;
               } else if ((item[6] == 0x05) && (iType == ReConvertNetworkType(GSM_ONLY)) &&
                (iBand == ReConvertBandMode(BAND_DCS1800_ONLY))) {
                   compFail = 0;
               } else if ((item[6] == 0x06) && (iType == ReConvertNetworkType(GSM_ONLY)) &&
                (iBand == ReConvertBandMode(BAND_PCS1900_ONLY))) {
                   compFail = 0;
               } else {            // if two values are not the same
                compFail = 1;
                strcpy(result, PRECONF_SET_FAIL);
            }
*/
            ALOGE("#### compareModemLockInfo : IPC_IMEI_ITEM_NETWORK_MODE >> result = [%d]\n", compFail);
/*- sh0515.lee - Band Selection for CSC update -*/
        } else if(item[0] == 0x88) {        //IPC_IMEI_ITEM_SECURITY_CODE
            ALOGE("#### compareModemLockInfo : IPC_IMEI_ITEM_SECURITY_CODE\n");
            // read security code info and compare with item value
            // if two values are not the same
            compFail = 1;
            strcpy(result, PRECONF_SET_FAIL);
        } else if(item[0] == 0x89) {        //IPC_IMEI_ITEM_GPRS_MODE
            ALOGE("#### compareModemLockInfo : IPC_IMEI_ITEM_GPRS_MODE >> item Len = [%x]\n", item[4]);
            ALOGE("#### compareModemLockInfo : IPC_IMEI_ITEM_GPRS_MODE >> item value = [%x]\n", item[6]);

            itemLen=(int)item[4];

            // read GPRS auto attach mode info and compare with item value
            //++ JSHAN Attach for CSC update
            property_get(NETWORK_GPRS_ATTACH_MODE, tmpStrGPRSattch, "1"); // 1 for gprs auto attach by default
            attach_mode = ConvertStrToNum(tmpStrGPRSattch);

            if ( (item[6] == 0x01) && (attach_mode == 1) ) {
                   compFail = 0;
               } else if ( (item[6] == 0x00) && (attach_mode == 2) ) {
                   compFail = 0;
               } else    {
                //-- JSHAN Attach for CSC update
                // if two values are not the same
                compFail = 1;
                strcpy(result, PRECONF_SET_FAIL);
            }
            ALOGE("#### compareModemLockInfo : IPC_IMEI_ITEM_GPRS_MODE >> result = [%d]\n", compFail);
        } else if(item[0] == 0x8B) {        // IPC_IMEI_ITEM_GSM_AMR_CALL_CONFIG;
            ALOGE("#### compareModemLockInfo : IPC_IMEI_ITEM_GSM_AMR_CALL_CONFIG >> item Len = [%x]\n", item[4]);
            ALOGE("#### compareModemLockInfo : IPC_IMEI_ITEM_GSM_AMR_CALL_CONFIG >> element cnt = [%x]\n", item[6]);

            itemLen=(int)item[4];
            /* To do..
                    compare item[6](WbAmr value from XML) with stored WbAmr value
                    if the two values are the same, set compFail=0;
                    if not the same, set  compFail=1;
            */
            /*wannatea*/
#if 0       /* this is not for wbamr */
                     Flash_Read_NV_Data(&amrwb, NV_AMR_CODEC);

                    if ((item[6] == 0x01)&&(amrwb==1))        //WbAmr enable
                    {
                        compFail=0;
                    }
                    else if ((item[6] == 0x00)&&(amrwb==1))        //WbAmr disable
                    {
                        compFail=0;
                    }
                    else
                    {           // if two values are not the same
                        compFail=1;
                        strcpy(result, PRECONF_SET_FAIL);
                    }
#endif
            ALOGE("#### compareModemLockInfo : IPC_IMEI_ITEM_GSM_AMR_CALL_CONFIG >> result = [%d]\n", compFail);
        } else if(item[0] == 0x8D) {        //IPC_GSM_IMEI_ITEM_SELLOUT_SMS; // item id for the 1st item
            ALOGE("#### compareModemLockInfo : IPC_GSM_IMEI_ITEM_SELLOUT_SMS >> item Len = [%x]\n", item[4]);    // item length (0x18 == 24)
            ALOGE("#### compareModemLockInfo : IPC_GSM_IMEI_ITEM_SELLOUT_SMS >> element cnt = [%x]\n", item[6]);    // Element Count

            itemLen = (int)item[4];

            element = item + 7;
            for(elementCnt = 0; elementCnt < (int)item[6]; elementCnt++) {
                ALOGE("#### compareModemLockInfo : IPC_GSM_IMEI_ITEM_SELLOUT_SMS >> elelment ID = [%x]\n", element[0]);    // IEMI_TOOL_TRIGGER_AP // Sellout SMS Mode
                ALOGE("#### compareModemLockInfo : IPC_GSM_IMEI_ITEM_SELLOUT_SMS >> elelment len = [%d]\n", (int)element[2]);    // length
                ALOGE("#### compareModemLockInfo : IPC_GSM_IMEI_ITEM_SELLOUT_SMS >> elelment value = [%x]\n", element[4]);    // value

                elementLen=(int)element[2];
                if (element[0] ==0x01) {        // IEMI_TOOL_TRIGGER_AP // Sellout SMS Mode
                    unsigned int sellout_sms_mode = 0x00;    //0 : Off, 1 : On
                    ALOGE("#### compareModemLockInfo : IPC_GSM_IMEI_ITEM_SELLOUT_SMS >> elelment ID = [%x]\n", element[0]);

                    // read sellout info and compare with item value
                    Flash_Read_NV_Data(&sellout_sms_mode, NV_SELLOUT_SMS_MODE_I);

                    if( sellout_sms_mode == element[4] ) {
                           compFail = 0;
                    } else { // if two values are not the same
                        compFail = 1;
                        strcpy(result, PRECONF_SET_FAIL);
                    }
                }
#if 0
                else if (element[0] ==0x02){        // IEMI_TOOL_TRIGGER_PC // Sellout Product Info
                    unsigned short    sellout_product_info = 0x01;    //0 : Product, 1 : test
                    ALOGE("#### compareModemLockInfo : IPC_GSM_IMEI_ITEM_SELLOUT_SMS >> elelment ID = [%x]\n", element[0]);

                    // read sellout info and compare with item value
                    Flash_Read_NV_Data(&sellout_product_info, NV_SELLOUT_PRODUCT_INFO_I);

                    if( sellout_product_info == element[4] )
                    {
                           compFail=0;
                    }
                    else  // if two values are not the same
                    {
                        compFail=0;
                        //strcpy(result, PRECONF_SET_FAIL);
                    }
                }
#endif
                element = element+(4+elementLen);
            }
        } else if(item[0] == 0x94) {        // IPC_IMEI_ITEM_WB_AMR_MODE;
            ALOGE("#### compareModemLockInfo : IPC_IMEI_ITEM_WB_AMR_MODE >> item Len = [%x]\n", item[4]);
            ALOGE("#### compareModemLockInfo : IPC_IMEI_ITEM_WB_AMR_MODE >> element cnt = [%x]\n", item[6]);

            itemLen=(int)item[4];
            Flash_Read_NV_Data(&temp_wb_amr_mode, NV_AMR_CODEC);

            if ((item[6] == 0x01)&&(temp_wb_amr_mode==1)) {        //WbAmr enable
                compFail = 0;
            } else if ((item[6] == 0x00)&&(temp_wb_amr_mode==1)) {       //WbAmr disable
                compFail = 0;
            } else {
                // if two values are not the same
                compFail = 1;
                strcpy(result, PRECONF_SET_FAIL);
            }
            ALOGE("#### compareModemLockInfo : IPC_IMEI_ITEM_WB_AMR_MODE >> current wbamr mode = [%d]\n", temp_wb_amr_mode);
            ALOGE("#### compareModemLockInfo : IPC_IMEI_ITEM_WB_AMR_MODE >> result = [%d]\n", compFail);
        }

        if(item[0] == 0x81) {            // IPC_IMEI_ITEM_GENERAL_LOCK
            ALOGE("#### compareModemLockInfo : IPC_IMEI_ITEM_GENERAL_LOCK\n");
        } else if(item[0] == 0x82) {            //IPC_IMEI_ITEM_NP_LOCK
            ALOGE("#### compareModemLockInfo : IPC_IMEI_ITEM_NP_LOCK\n");
            ALOGE("#### compareModemLockInfo : IPC_IMEI_ITEM_NP_LOCK >> unlock cnt = [%x]\n", item[16]);
            ALOGE("#### compareModemLockInfo : IPC_IMEI_ITEM_NP_LOCK >> nwcode = [%x]\n", item[19]);

            nwcode =  item[19];
            for(i = 0; i < nwcode; i++) {
                lockcode[i] = item[21 + i];
            }
            lockcode[i + 1] =  '\0';
            /*    nwcode -> 20601#20602# */

            ALOGD("%s: nwcode[%s]\n",__FUNCTION__, lockcode);

            strncpy(nvlockcode, (char*)(perso_data.nw_lock_codes), 8);
            ALOGD("%s: NVnwcode[%s]\n",__FUNCTION__, nvlockcode);

            if (strcmp(lockcode, nvlockcode) == 0) {
                   compFail = 0;
                ALOGE("#### nwcode =  NVnwcode is correct  ####\n");
            } else {
                   compFail = 1;
                strcpy(result, NWLOCK_FAIL);
                ALOGE("#### Mismatch Networkcode ####\n");
             }
#if 0
                if (strcmp(perso_data.cks_max_attempts, (unsigned char)item[16]) == 0)
                {
                   compFail=1;
                ALOGE("#### cks_max_attempts =  Nvattempt is correct  ####\n");
                }
             else
             {
                   compFail=0;
                strcpy(result, PRECONF_SET_FAIL);
                ALOGE("#### Mismatch cks_max_attempts ####\n");
             }
#endif
        } else if(item[0] == 0x83) {            //IPC_IMEI_ITEM_NP_LOCK
            ALOGE("#### compareModemLockInfo : IPC_IMEI_ITEM_NSP_LOCK\n");
            ALOGE("#### compareModemLockInfo : IPC_IMEI_ITEM_NSP_LOCK >> unlock cnt = [%x]\n", item[16]);
            ALOGE("#### compareModemLockInfo : IPC_IMEI_ITEM_NSP_LOCK >> nscode = [%x]\n", item[19]);

            nscode =  item[19];
            for(i = 0; i < nscode; i++) {
                nslockcode[i] =  item[21 + i];
            }
            nslockcode[i + 1] =  '\0';
            /*    nwcode -> 20601#20602# */

            ALOGD("%s: nslockcode[%s]\n",__FUNCTION__, nslockcode);
            strncpy(nvnslockcode, (char*)(perso_data.ns_lock_codes), 8);
            ALOGD("%s: nvnslockcode[%s]\n",__FUNCTION__, nvnslockcode);

            if (strcmp(nslockcode, nvnslockcode) == 0) {
                   compFail = 0;
                ALOGE("#### nslockcode =  nvnslockcode is correct  ####\n");
            } else {
                   compFail = 1;
                strcpy(result, SUBSETLOCK_FAIL );
                ALOGE("#### Mismatch Networkcode ####\n");
             }
        } else if(item[0] == 0x84) {            //IPC_IMEI_ITEM_SP_LOCK
            ALOGE("#### compareModemLockInfo : IPC_IMEI_ITEM_SP_LOCK\n");
            spcode =  item[19];

            for(i = 0; i < spcode; i++) {
                splockcode[i] =  item[21 + i];
            }
            splockcode[i+1] =  '\0';
            /*    nwcode -> 20601#20602# */

            ALOGD("%s: spcode[%s]\n",__FUNCTION__, splockcode);

            strncpy(nvsplockcode, (char*)(perso_data.sp_lock_codes), 8);
            ALOGD("%s: NVspcode[%s]\n",__FUNCTION__, nvsplockcode);

            if (strcmp(splockcode, nvsplockcode) == 0) {
                   compFail = 0;
                ALOGE("#### spcode =  NVspcode is correct  ####\n");
             } else {
                   compFail = 1;
                strcpy(result, SPLOCK_FAIL );
                ALOGE("#### Mismatch spcode ####\n");
            }
        } else if(item[0] == 0x85) {            //IPC_IMEI_ITEM_CP_LOCK
            ALOGE("#### compareModemLockInfo : IPC_IMEI_ITEM_CP_LOCK\n");
            cpcode = item[19];

            for(i = 0; i < cpcode; i++) {
                cplockcode[i] =  item[21 + i];
             }
            cplockcode[i+1] =  '\0';
            ALOGD("%s: cpcode[%s]\n",__FUNCTION__, cplockcode);

            strncpy(nvcplockcode, (char*)(perso_data.cp_lock_codes), 8);
            ALOGD("%s: NVspcode[%s]\n",__FUNCTION__, nvcplockcode);

            if (strcmp(cplockcode, nvcplockcode) == 0) {
                   compFail = 0;
                ALOGE("#### cpcode =  NVcpcode is correct  ####\n");
            } else {
                   compFail = 1;
                strcpy(result, CPLOCK_FAIL);
                ALOGE("#### Mismatch spcode ####\n");
            }
        }
        item = item + (6 + itemLen);
    }

    if(CscCompareResult == 0) {
        if (!compFail)
            requestGetPreConfiguration(result);

        AtResultMsg* ATresult = (AtResultMsg*) malloc(sizeof(AtResultMsg));
        strcpy(ATresult->data, result);
        ATresult->len = strlen(ATresult->data) +1;
        if(sendReponseToATC(ATresult, sizeof(AtResultMsg)) < 0 )
            ALOGD("AT result sent ERROR : %s ( %d bytes ) ", ATresult->data, ATresult->len);
        else
            ALOGD("AT result sent : %s ( %d bytes ) ", ATresult->data, ATresult->len);
        free(ATresult);
    }
}

void sril_IMEIHandler(int request, void *data, size_t datalen, RIL_Token t) {

    OemReqMsgHdr * pSvcCmd = (OemReqMsgHdr *)data;

    unsigned char Simcheck[5] = {0x0,};    // gearn dismiss network lock
    property_get("ril.ICC_TYPE", Simcheck, "3"); // FIXME: check for SIM_ID in Renesas
    ALOGD("sril_IMEIHandler:  SimState : %s  \n", Simcheck);

    ALOGD("%s: IMEI[%x]\n",__FUNCTION__, pSvcCmd->subfId);
    ALOGE("########## SRIL  Request Imei Handler   ############\n");

    switch(pSvcCmd->subfId) {
        case OEM_IMEI_EVENT_START_IMEI: {
            ALOGE("########## SRIL  BOOT COMPLETE   ############\n");

            // FIXME: not sure it is right. test it
            char *rawdata = (char*)data;
            int resetMode = (int)rawdata[3];
            if(resetMode == 5) {
                if((int)rawdata[4] == 1) {
                    ALOGD("########## RESET_COMPLETE ############\n");
                    put_factory_string("RESET COMPLETED\n",17);
                }
            }

            RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
            break;
        }
        case OEM_IMEI_SET_PRECONFIGURAION: {
            FILE *fp = NULL;
            char* rawdata = NULL;
            char result[40]={0};
            rawdata = (char*)data;

            ALOGE("sril_SetPreConfigurationMessage = %s: modeType[%x]\n", __FUNCTION__, rawdata[0]);
            ALOGE("sril_SetPreConfigurationMessage = %s: subType[%x]\n", __FUNCTION__, rawdata[1]);

            char sales_code[40] = {rawdata[6], rawdata[7], rawdata[8], 0};

            requestSetPreConfiguration(result, sales_code);

            if(!strcmp(result, PRECONF_OK))
                RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
            else
                RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);

            break;
        }
#if defined(SEC_MULTI_CSC)
        case OEM_IMEI_GET_PRECONFIGURAION: {
            ALOGE("########## SRIL  Get Preconfiguration   ############\n");
            ALOGE(" ---=== Not supported! ===--- \n");
            break;
        }
#endif
        case OEM_IMEI_SET_WRITE_ITEM: {
            ALOGE("########## SRIL  Set Write Item  ############\n");
            ALOGE(" ---=== Not supported! ===--- \n");
            break;
        }
        case OEM_IMEI_GET_WRITE_ITEM: {
            ALOGE("########## SRIL  Get Write Item  ############\n");
            ALOGE(" ---=== Not supported! ===--- \n");
            break;
        }
        case OEM_IMEI_SET_UPDATE_ITEM:
            sril_SetUpdateItemMessage(request, data, datalen, t);
            break;

        case OEM_IMEI_CFRM_UPDATE_ITEM:
            sril_CfrmUpdateItemMessage(request, data, datalen, t);
            break;

        case OEM_IMEI_EVENT_VERIFY_COMPARE:
            sril_EventVerifyCompareMessage(request, data, datalen, t);
            break;

        case OEM_IMEI_RESP_FILE_NUM:
            ALOGE("########## SRIL  Resp file Num  ############\n");
            ALOGE(" ---=== Not supported! ===--- \n");
            break;

        case OEM_IMEI_SIM_OUT:
            ALOGD("%s: OEM_IMEI_SIM_OUT #### \n",__FUNCTION__);
            // FIXME: check usage of CP here
            break;

        default:
            ALOGD("%s: Not supported svcMode [%x]\n", __FUNCTION__, pSvcCmd->subfId);
            RIL_onRequestComplete(t, RIL_E_REQUEST_NOT_SUPPORTED, NULL, 0);
            break;
    }
}

void sril_EventVerifyCompareMessage(int request, void *data, size_t datalen, RIL_Token t) {
    ALOGE("########## SRIL  Event Verify Compare  ############\n");

    char *result = (char*)data;
    char cmd_buf[640] = {0};
    char response[17]  = {0};
    FILE *fp = NULL;

    int AppCmpResult = (int)result[4];    // 0 is failed, 1 is successful

    ALOGE("#### sril_VerifyCompare = %s: modeType[%x]\n",__FUNCTION__, result[0]);
    ALOGE("#### sril_VerifyCompare = %s: subType[%x]\n",__FUNCTION__, result[1]);
    ALOGE("#### sril_VerifyCompare = %s: dataLen[%d]\n",__FUNCTION__, (int)result[3]);
    ALOGE("#### sril_VerifyCompare = %s: result[%x]\n",__FUNCTION__, result[4]);
    ALOGE("#### sril_VerifyCompare = %s: failcount[%x]\n",__FUNCTION__, result[5]);

    CscCompareResult = 0;
    if(AppCmpResult == 0) {             // csc compare is failed in applications. return result to ATC
        ALOGD("CSC app compare result failed : %s ", result + 6);
        if(strstr(result + 6, "APPCOUNT") != NULL)
            sprintf(response, APPCOUNT_FAIL);
        else
            sprintf(response, PRECONF_SET_FAIL);

        AtResultMsg* ATresult = (AtResultMsg*) malloc(sizeof(AtResultMsg));
        strcpy(ATresult->data, response);
        ATresult->len = strlen(ATresult->data) + 1;

        if(sendReponseToATC(ATresult, sizeof(AtResultMsg)) < 0 )
            ALOGD("AT result sent ERROR : %s ( %d bytes ) ", ATresult->data, ATresult->len);
        else
            ALOGD("AT result sent : %s ( %d bytes ) ", ATresult->data, ATresult->len);

        free(ATresult);
        CscCompareResult = 1 ;
    }
    /* send intent for verifying Modem info and Lock info */
    sprintf(cmd_buf, "am broadcast -a android.intent.action.CSC_MODEM_SETTING --es MODE COMPARE_VERIFY_MODEM");
    ALOGE("cmd_buf :%s", cmd_buf);

    if (system(cmd_buf) < 0) {
        ALOGE("cmd_buf error : %s", cmd_buf);    /* DBGMSG("%s(): system(%s) error", __FUNCTION__, cmd_buf); */
    }
    RIL_onRequestComplete(t, RIL_E_SUCCESS, data, datalen);
}

void sril_SetUpdateItemMessage(int request, void *data, size_t datalen, RIL_Token t) {

    ALOGE("########## SRIL  Set Update Item  ############\n");

    FILE *fp = NULL;

    char* rawdata = NULL;
    char* item = NULL;
    char* element = NULL;

    char fsbuild_check[40] = {0};
    char cmd_buf[640] = {0};

    int mode = 0;
    int byteLen = 0;
    int itemNum = 0;
    int itemCnt = 0;
    int itemLen = 0;
    int elementCnt = 0;
    int elementLen = 0;

    rawdata = (char*)data;

    ALOGE("#### sril_SetUpdateItem = %s: modeType[%x]\n", __FUNCTION__, rawdata[0]);
    ALOGE("#### sril_SetUpdateItem = %s: subType[%x]\n", __FUNCTION__, rawdata[1]);

    byteLen = ((int)rawdata[3])-6;
    mode = (int)rawdata[4];            // write(0) or compare(1)
    itemNum = (int)rawdata[5];

/*+ sh0515.lee - Band Selection for CSC update +*/
    OemReqMsgHdr * pSvcCmd = (OemReqMsgHdr *)data;
    OemSvcEnterModeMsg * pTempCmd;
    char tmpStrRat[5];
    char tmpStrBand[5];
    pTempCmd = (OemSvcEnterModeMsg *) malloc(sizeof(OemSvcEnterModeMsg));
    memset(pTempCmd, 0x0, sizeof(OemSvcEnterModeMsg));
/*- sh0515.lee - Band Selection for CSC update -*/

//++ JSHAN Attach for CSC update
    int attach_mode = 0;
    char tmpStrGPRSattch[PROPERTY_VALUE_MAX];
//-- JSHAN Attach for CSC update

    //wannatea
    int amrwb = 0;
    int umts_wb_amr_mode = 0;
    //memcpy(&byteLen, rawdata+2, 2);
    //memcpy(&itemNum, rawdata+4, 2);

    ALOGE("#### sril_SetUpdateItem = %s: MODE [%d]\n", __FUNCTION__, mode);
    ALOGE("#### sril_SetUpdateItem = %s: byte Len[%d]\n", __FUNCTION__, (int)byteLen);
    ALOGE("#### sril_SetUpdateItem = %s: item Num[%d]\n", __FUNCTION__, (int)itemNum);

    item = rawdata + 6;

    if(mode==0) {                         // write modem info

        /* set fsbuild check flag to COMPLETE */
        if((fp = fopen(FSBUILD_CHECK_PATH, "w+")) == NULL) {
            ALOGD("fsbuild check file open err");
            RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
        } else {
            ALOGD("fsbuild check file open OK");
            fprintf(fp, "%s", FSBUILD_COMPLETE);
            fclose(fp);
        }

        if((fp = fopen(FSBUILD_CHECK_PATH, "r")) == NULL) {
            ALOGD("fsbuild check file open err");
            RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
        } else {
            ALOGD("fsbuild check file open OK");
            fread(fsbuild_check, 1, 15, fp);
            ALOGD("fsbuild check flag in file ==> %s", fsbuild_check);
            fclose(fp);
        }

        for(itemCnt=0;itemCnt<(int)itemNum;itemCnt++) {

            if(item[0] == 0x87) {        //IPC_IMEI_ITEM_NETWORK_MODE
                ALOGE("#### sril_SetUpdateItem : IPC_IMEI_ITEM_NETWORK_MODE >> item Len = [%x]\n", item[4]);
                ALOGE("#### sril_SetUpdateItem : IPC_IMEI_ITEM_NETWORK_MODE >> item value = [%x]\n", item[6]);

                itemLen=(int)item[4];

/*+ sh0515.lee - Band Selection for CSC update +*/

                pTempCmd->hdr.funcId = pSvcCmd->funcId;
                pTempCmd->hdr.subfId = OEM_SVC_ENTER_MODE_MESSAGE;
                pTempCmd->hdr.len= pSvcCmd->len;
                pTempCmd->modeType = SVC_MODE_TEST_MANUAL ;
                pTempCmd->query= 0x00;

                // FIXME: this should be enabled
/*                if (item[6] == 0x01) { // Auto
                    pTempCmd->subType= TST_SET_BAND_AUTO;
                    sprintf(&tmpStrRat[0], "%d", ReConvertNetworkType(DUAL_MODE_UMTS_PREF));
                    sprintf(&tmpStrBand[0], "%d", ReConvertBandMode(BAND_AUTO));
                    property_set(NETWORK_TYPE_PATH, &tmpStrRat);
                    property_set(NETWORK_BAND_PATH,&tmpStrBand);
                } else if (item[6] == 0x02) { // UMTS
                    pTempCmd->subType= TST_SET_BAND_ALL_UMTS;
                    sprintf(&tmpStrRat[0], "%d", ReConvertNetworkType(UMTS_ONLY));
                    sprintf(&tmpStrBand[0], "%d", ReConvertBandMode(BAND_ALL_UMTS));
                    property_set(NETWORK_TYPE_PATH, &tmpStrRat);
                    property_set(NETWORK_BAND_PATH,&tmpStrBand);
                } else if (item[6] == 0x03) { // GSM
                    pTempCmd->subType= TST_SET_BAND_ALL_GSM;
                    sprintf(&tmpStrRat[0], "%d", ReConvertNetworkType(GSM_ONLY));
                    sprintf(&tmpStrBand[0], "%d", ReConvertBandMode(BAND_ALL_GSM));
                    property_set(NETWORK_TYPE_PATH, &tmpStrRat);
                    property_set(NETWORK_BAND_PATH,&tmpStrBand);
                } else if (item[6] == 0x04) { // 900
                    pTempCmd->subType= TST_SET_BAND_GSM_900;
                    sprintf(&tmpStrRat[0], "%d", ReConvertNetworkType(GSM_ONLY));
                    sprintf(&tmpStrBand[0], "%d", ReConvertBandMode(BAND_GSM900_ONLY));
                    property_set(NETWORK_TYPE_PATH, &tmpStrRat);
                    property_set(NETWORK_BAND_PATH,&tmpStrBand);
                } else if (item[6] == 0x05) { // 1800
                    pTempCmd->subType= TST_SET_BAND_DCS_1800;
                    sprintf(&tmpStrRat[0], "%d", ReConvertNetworkType(GSM_ONLY));
                    sprintf(&tmpStrBand[0], "%d", ReConvertBandMode(BAND_DCS1800_ONLY));
                    property_set(NETWORK_TYPE_PATH, &tmpStrRat);
                    property_set(NETWORK_BAND_PATH,&tmpStrBand);
                } else if (item[6] == 0x06) { // 1900
                    pTempCmd->subType= TST_SET_BAND_PCS_1900;
                    sprintf(&tmpStrRat[0], "%d", ReConvertNetworkType(GSM_ONLY));
                    sprintf(&tmpStrBand[0], "%d", ReConvertBandMode(BAND_PCS1900_ONLY));
                    property_set(NETWORK_TYPE_PATH, &tmpStrRat);
                    property_set(NETWORK_BAND_PATH,&tmpStrBand);
                } else { // Default
                    pTempCmd->subType= TST_SET_BAND_AUTO;
                    sprintf(&tmpStrRat[0], "%d", ReConvertNetworkType(DUAL_MODE_UMTS_PREF));
                    sprintf(&tmpStrBand[0], "%d", ReConvertBandMode(BAND_AUTO));
                    property_set(NETWORK_TYPE_PATH, &tmpStrRat);
                    property_set(NETWORK_BAND_PATH,&tmpStrBand);
                }
*/
/*- sh0515.lee - Band Selection for CSC update -*/
            } else if(item[0] == 0x88) {        //IPC_IMEI_ITEM_SECURITY_CODE
                ALOGE("#### sril_SetUpdateItem : IPC_IMEI_ITEM_SECURITY_CODE\n");
            } else if(item[0] == 0x89) {        //IPC_IMEI_ITEM_GPRS_MODE
                ALOGE("#### sril_SetUpdateItem : IPC_IMEI_ITEM_GPRS_MODE >> item Len = [%x]\n", item[4]);
                ALOGE("#### sril_SetUpdateItem : IPC_IMEI_ITEM_GPRS_MODE >> item value = [%x]\n", item[6]);

                itemLen=(int)item[4];

                //++ JSHAN Attach for CSC update
                if (item[6] == 0x01) {
                    attach_mode = 1;
                    sprintf(&tmpStrGPRSattch[0], "%d", attach_mode);
                    property_set(NETWORK_GPRS_ATTACH_MODE, tmpStrGPRSattch);
                } else if (item[6] == 0x00) {
                    attach_mode = 2;
                    sprintf(&tmpStrGPRSattch[0], "%d", attach_mode);
                    property_set(NETWORK_GPRS_ATTACH_MODE, tmpStrGPRSattch);
                }
            //-- JSHAN Attach for CSC update
            } else if(item[0] == 0x8B) {        // IPC_IMEI_ITEM_GSM_AMR_CALL_CONFIG;
                ALOGE("#### sril_SetUpdateItem : IPC_IMEI_ITEM_GSM_AMR_CALL_CONFIG >> item Len = [%x]\n", item[4]);
                ALOGE("#### sril_SetUpdateItem : IPC_IMEI_ITEM_GSM_AMR_CALL_CONFIG >> item value = [%x]\n", item[6]);

                itemLen=(int)item[4];

            /* To do.. set WbAmr value with item[6] */
            /*wannatea*/
#if 0            /* this is not for wbamr*/
                    if (item[6] == 0x01)        //WbAmr enable
                    {
                        amrwb = 1;
                        Flash_Write_NV_Data(&amrwb, NV_AMR_CODEC);
                    }
                    else if (item[6] == 0x00)        //WbAmr disable
                    {
                        amrwb = 0;
                        Flash_Write_NV_Data(&amrwb, NV_AMR_CODEC);
                    }
#endif
                ALOGE("#### sril_SetUpdateItem : IPC_IMEI_ITEM_GSM_AMR_CALL_CONFIG >> amrwb = [%x]\n", amrwb);
            } else if(item[0] == 0x8D) {        //IPC_GSM_IMEI_ITEM_SELLOUT_SMS; // item id for the 1st item
                ALOGE("#### sril_SetUpdateItem : IPC_GSM_IMEI_ITEM_SELLOUT_SMS >> item Len = [%x]\n", item[4]);    // item length (0x18 == 24)
                ALOGE("#### sril_SetUpdateItem : IPC_GSM_IMEI_ITEM_SELLOUT_SMS >> element cnt = [%x]\n", item[6]);    // Element Count

                itemLen=(int)item[4];

                element = item + 7;

                for(elementCnt = 0; elementCnt < (int)item[6]; elementCnt++) {
                    ALOGE("#### sril_SetUpdateItem : IPC_GSM_IMEI_ITEM_SELLOUT_SMS >> elelment ID = [%x]\n", element[0]);    // IEMI_TOOL_TRIGGER_AP // Sellout SMS Mode
                    ALOGE("#### sril_SetUpdateItem : IPC_GSM_IMEI_ITEM_SELLOUT_SMS >> elelment len = [%d]\n", (int)element[2]);    // length
                    ALOGE("#### sril_SetUpdateItem : IPC_GSM_IMEI_ITEM_SELLOUT_SMS >> elelment value = [%x]\n", element[4]);    // value

                    elementLen=(int)element[2];

                    if (element[0] ==0x01) {        // IEMI_TOOL_TRIGGER_AP // Sellout SMS Mode
                        unsigned int sellout_sms_mode = 0x00;    //0 : Off, 1 : On
                        ALOGE("#### sril_SetUpdateItem : IPC_GSM_IMEI_ITEM_SELLOUT_SMS >> elelment ID = [%x]\n", element[0]);
                        if( element[4] == 0x01 )
                            sellout_sms_mode = 0x01;
                        else
                            sellout_sms_mode = 0x00;

                        Flash_Write_NV_Data(&sellout_sms_mode, NV_SELLOUT_SMS_MODE_I);
                    }
#if 0
                else if (element[0] ==0x02){        // IEMI_TOOL_TRIGGER_PC // Sellout Product Info
                    unsigned short    sellout_product_info = 0x01;    //0 : Product, 1 : test
                    ALOGE("#### sril_SetUpdateItem : IPC_GSM_IMEI_ITEM_SELLOUT_SMS >> elelment ID = [%x]\n", element[0]);
                    if( element[4] == 0x01 )
                        sellout_product_info = 0x01;
                    else
                        sellout_product_info = 0x00;

                    Flash_Write_NV_Data(&sellout_product_info, NV_SELLOUT_PRODUCT_INFO_I);
                }
#endif
                    element = element+(4+elementLen);
                }
            } else if(item[0] == 0x94) {        // IPC_IMEI_ITEM_WB_AMR_MODE;
                int umts_wb_amr_mode = 0;
                ALOGE("#### sril_SetUpdateItem : IPC_IMEI_ITEM_WB_AMR_MODE >> item Len = [%x]\n", item[4]);
                ALOGE("#### sril_SetUpdateItem : IPC_IMEI_ITEM_WB_AMR_MODE >> item value = [%x]\n", item[6]);

                itemLen=(int)item[4];

                /* set WbAmr value with item[6] */
                if (item[6] == 0x01) {        //WbAmr enable
                    umts_wb_amr_mode = 1;
                    Flash_Write_NV_Data(&umts_wb_amr_mode, NV_AMR_CODEC);
                } else if (item[6] == 0x00) {        //WbAmr disable
                    umts_wb_amr_mode = 0;
                    Flash_Write_NV_Data(&umts_wb_amr_mode, NV_AMR_CODEC);
                }
                ALOGE("#### sril_SetUpdateItem : IPC_IMEI_ITEM_WB_AMR_MODE >> umts_wb_amr_mode = [%x]\n", umts_wb_amr_mode);
            }
            item = item + (6+itemLen);
        }

    /* to implement
        send response to CSC appl */

    /* write lock info, network mode
    sprintf(cmd_buf, "am broadcast -a android.intent.action.CSC_MODEM_SETTING --es MODE SET_LOCK_INFO");
    ALOGE("cmd_buf :%s", cmd_buf);
    if (system(cmd_buf) < 0)
    {
        ALOGE("cmd_buf error : ", cmd_buf);
    }
    */
    } else {
        ALOGE("#### jump to compare \n");
        compareModemLockInfo(item, itemNum);
    }
    RIL_onRequestComplete(t, RIL_E_SUCCESS, data, datalen);
    free(pTempCmd);
}

void sril_CfrmUpdateItemMessage(int request, void *data, size_t datalen, RIL_Token t) {
    perso_me_data_type_t perso_data;
    perso_me_data_type_t temp_data;
    char* rawdata = NULL;

    char* item = NULL;

    int i = 0;
    int nwcode = 0;
    int nscode = 0;
    int spcode = 0;
    int cpcode = 0;

    char* SECLOCK_UPDATE = "ril.SECLOCK";

    rawdata = (char*)data;

    ALOGE("########## SRIL  Confirm Update Item  ############\n");
    ALOGE("#### sril_CfrmUpdateItem = %s: modeType[%x]\n", __FUNCTION__, rawdata[0]);
    ALOGE("#### sril_CfrmUpdateItem = %s: subType[%x]\n", __FUNCTION__, rawdata[1]);

    item = rawdata + 5;

    memset(&perso_data, 0, sizeof(perso_data));
    memset(&temp_data, 0, sizeof(temp_data));

    if(item[11] == 0) {    // IPC_IMEI_ITEM_GENERAL_LOCK
        ALOGE("## No lock on CSC -> Reset lock info ##\n");
        Flash_Read_Secure_Data (&temp_data, SEC_LOCK );

        memset(&perso_data, 0, sizeof(perso_data));

        memcpy(perso_data.ms_cks, temp_data.ms_cks,PERSO_CKS_ITEM_LENGTH);
        memcpy(perso_data.nw_cks, temp_data.nw_cks,PERSO_CKS_ITEM_LENGTH);
        memcpy(perso_data.ns_cks, temp_data.ns_cks, PERSO_CKS_ITEM_LENGTH);
        memcpy(perso_data.sp_cks, temp_data.sp_cks,PERSO_CKS_ITEM_LENGTH);
        memcpy(perso_data.cp_cks, temp_data.cp_cks,PERSO_CKS_ITEM_LENGTH);
        memcpy(perso_data.sim_cks, temp_data.sim_cks,PERSO_CKS_ITEM_LENGTH);
    } else {
        Flash_Read_Secure_Data (&temp_data, SEC_LOCK );

        memcpy(perso_data.ms_cks, temp_data.ms_cks,PERSO_CKS_ITEM_LENGTH);
        memcpy(perso_data.nw_cks, temp_data.nw_cks,PERSO_CKS_ITEM_LENGTH);
        memcpy(perso_data.ns_cks, temp_data.ns_cks, PERSO_CKS_ITEM_LENGTH);
        memcpy(perso_data.sp_cks, temp_data.sp_cks,PERSO_CKS_ITEM_LENGTH);
        memcpy(perso_data.cp_cks, temp_data.cp_cks,PERSO_CKS_ITEM_LENGTH);
        memcpy(perso_data.sim_cks, temp_data.sim_cks,PERSO_CKS_ITEM_LENGTH);

        if(item[0] == 0x81)    { // IPC_IMEI_ITEM_GENERAL_LOCK
            ALOGE("#### sril_CfrmUpdateItem : IPC_IMEI_ITEM_GENERAL_LOCK >> element value[11] = [%x]\n", item[11]);
            perso_data.state_inds = (unsigned char)item[11] ;

            if(perso_data.state_inds == 1) {
                perso_data.nw_tag = 1;
            } else if(perso_data.state_inds == 3) {
                perso_data.nw_tag = 1;
                perso_data.ns_tag = 1;
            } else if(perso_data.state_inds == 5) {
                perso_data.nw_tag = 1;
                perso_data.sp_tag = 1;
                perso_data.state_inds = 9;
                ALOGE("## perso_data.state_inds = 9 ##\n");
            } else if(perso_data.state_inds == 9) {
                perso_data.nw_tag = 1;
                perso_data.cp_tag = 1;
                perso_data.state_inds = 17;
            }

            ALOGE("#### item[12] = [%x]\n", item[12]);
            if(item[12] == 0x82) { //IPC_IMEI_ITEM_NP_LOCK
                ALOGE("#### sril_SetUpdateItem : IPC_IMEI_ITEM_NP_LOCK >> cks_max_attempts cnt = [%x]\n", item[29]);
                perso_data.cks_max_attempts = (unsigned char)item[29];

                ALOGE("#### lengthoflockcode = [%x]\n", item[31]);
                nwcode =  item[31];
                perso_data.nw_len = nwcode;

                for(i = 0; i < nwcode; i++) {
                    perso_data.nw_lock_codes[i] =  item[33 + i];
                }

                /*    nwcode -> 20601#20602# */
                perso_data.nw_lock_codes[i+1] = " ";
                ALOGD("%s: nw_lock_codes[%s]\n",__FUNCTION__, perso_data.nw_lock_codes);
            }

            ALOGE("#### item[33+nwcode] = [%x]\n", item[33 + nwcode]);
            if(item[33+nwcode] == 0x83)    { //IPC_IMEI_ITEM_NSP_LOCK
                ALOGE("#### sril_CfrmUpdateItem : IPC_IMEI_ITEM_NSP_LOCK\n");

                ALOGE("#### length of nscode = [%x]\n", item[33 + nwcode + 19]);
                nscode =  item[33 + nwcode + 19];
                perso_data.ns_len = nscode;

                for(i = 0; i < nscode; i++) {
                        perso_data.ns_lock_codes[i] =  item[33 + nwcode + 21 + i];
                }

                /*    nscode -> 555# -> (mccmnc length : 5, SP code : 55, #)        */
                perso_data.ns_lock_codes[i + 1] = " ";
                ALOGD("%s: ns_lock_codes[%s]\n",__FUNCTION__, perso_data.ns_lock_codes);
            }

            if(item[33 + nwcode] == 0x84) {    //IPC_IMEI_ITEM_SP_LOCK
                ALOGE("#### sril_CfrmUpdateItem : IPC_IMEI_ITEM_SP_LOCK\n");

                ALOGE("#### length of spcode = [%x]\n", item[33 + nwcode + 19]);
                spcode =  item[33 + nwcode + 19];
                perso_data.sp_len = spcode;

                for(i = 0; i < spcode; i++) {
                    perso_data.sp_lock_codes[i] =  item[33 + nwcode + 21 + i];
                }

                /*    spcode -> 555# -> (mccmnc length : 5, SP code : 55, #)        */
                perso_data.sp_lock_codes[i + 1] = " ";
                ALOGD("%s: sp_lock_codes[%s]\n",__FUNCTION__, perso_data.sp_lock_codes);
            }

            if(item[33 + nwcode] == 0x85) {    //IPC_IMEI_ITEM_CP_LOCK
                ALOGE("#### sril_CfrmUpdateItem : IPC_IMEI_ITEM_CP_LOCK\n");

                ALOGE("#### length of cpcode = [%x]\n", item[33 + nwcode + 19]);
                cpcode =  item[33 + nwcode + 19];
                perso_data.cp_len = cpcode;

                for(i = 0; i < cpcode; i++) {
                    perso_data.cp_lock_codes[i] =  item[33 + nwcode + 21 + i];
                }

                /*    spcode -> 555# -> (mccmnc length : 5, SP code : 55, #)        */
                perso_data.cp_lock_codes[i+1] = " ";
                ALOGD("%s: cp_lock_codes[%s]\n",__FUNCTION__, perso_data.cp_lock_codes);
            }
        }
    }

    Flash_Write_Secure_Data(&perso_data, SEC_LOCK );
#if 0
    if(SecNativeFeature_getEnableStatus(TAG_CSCFEATURE_RIL_ENABLE_TOTALCALLTIME)) {
        unsigned long call_time = 0;
        if(!Flash_Write_NV_Data(&call_time, NV_TOTAL_CALLTIME)) {
            ALOGE("sril_ReSetTotalCallTime\n");
        } else {
            ALOGE("sril_ReSetTotalCallTime Fail \n");
        }
    }
#endif
    property_set(SECLOCK_UPDATE,"1");
    ALOGD("%s: jseseo CSC MEP info is updated to NV [%s]\n", __FUNCTION__, SECLOCK_UPDATE);

    RIL_onRequestComplete(t, RIL_E_SUCCESS, data, datalen);
}

void requestSetPreConfiguration(char *ATCresult, char *data) {
    FILE *fp = NULL;
    FILE *fp_tmp = NULL;
    char *result = ATCresult;

    DIR *cscDir;
    struct dirent *ent;

    int param_check = 0;

    ALOGE("### requestSetPreConfiguration = %s: [%s]\n", __FUNCTION__, data);

    char sales_code[40] = {data[0], data[1], data[2], 0};
    char stored_sales_code[40] = {0};

    strcpy(result, PRECONF_OK);
#if 0
// LTN_SW1_SYSTEM kwanguk.kim 2012.05.22 - TFG IMEI issue
    if(!strncmp(SecNativeFeature_getString(TAG_CSCFEATURE_RIL_REPLACE_IMEI_SALESCODE_AS), "TFG", 3)) {
        if (IsTFGSalesCode(sales_code) == 1) {
            ALOGD("[LTN] PRECONFIG_WRITE: Set as TFG");
            strncpy(sales_code, "TFG", 3);
        }
    }
#endif
// end - LTN_SW1_SYSTEM kwanguk.kim 2012.05.22 - TFG IMEI issue

    // check if the input data is available
    if((fp = fopen("/system/csc/sales_code.dat", "r")) == NULL) {
        ALOGD("sales_code file open err");
        strcpy(result, PRECONF_SET_FAIL);
        return;
    } else {
        ALOGD("sales_code file open OK");
        fread(stored_sales_code, 1, 3, fp);
        ALOGD("sales code in sales_code file ==> %s", stored_sales_code);
        fclose(fp);
    }

    if(!strncmp((char*)sales_code, (char*)stored_sales_code, 3)) {
        param_check = 1;
    } else {
        cscDir = opendir (CSC_DIR_PATH);
        if (cscDir != NULL) {
            while ((ent = readdir (cscDir)) != NULL) {
                ALOGD ("CSC DIR ==> %s", ent->d_name);
                if(!(strncmp(ent->d_name, sales_code, 3))) {
                    param_check = 1;
                    break;
                }
            }
            closedir (cscDir);
        } else {
            strcpy(result, PRECONF_SET_FAIL);
            ALOGD("CSC Directory Open ERR!!!!");
            return;
        }
    }

    if(!param_check) {                    //    parameter error
        strcpy(result, PRECONF_PARAM_ERR);
        return;
    } else {            // set sales code
        param_check    = 0;
        property_set("ril.sales_code", (char *)(sales_code));
        ALOGD("sales code setting ==> %s\n", sales_code);

        /* store sales code in file */
        ALOGD("sales code file path : %s", SALES_CODE_PATH);

        if((fp = fopen(SALES_CODE_PATH, "w+")) == NULL) {
            ALOGD("sales code file open err");
            strcpy(result, PRECONF_SET_FAIL);
            return;
        } else {
            ALOGD("sales code file open OK");
            fprintf(fp, "%s", sales_code);
            fclose(fp);
        }

        /* store sales code in tmp file */
        if((fp_tmp = fopen(SALES_CODE_PATH_TMP, "w+")) == NULL) {
            ALOGD("sales code tmp file open err");
            strcpy(result, PRECONF_SET_FAIL);
            return;
        } else {
            ALOGD("sales code tmp file open OK");
            fprintf(fp_tmp, "%s", sales_code);
            fclose(fp_tmp);
        }

        // check if the sales code saved well
        if((fp = fopen(SALES_CODE_PATH, "r")) == NULL) {
            ALOGD("sales code file open err");
            strcpy(result, PRECONF_SET_FAIL);
            return;
        } else {
            ALOGD("sales code file open OK");
            fread(stored_sales_code, 1, 15, fp);
            ALOGD("sales code in file ==> %s", stored_sales_code);
            fclose(fp);
        }

        chmod(SALES_CODE_PATH, 0664);
        if(strncmp((char*)sales_code, (char*)stored_sales_code, 3)) {
            strcpy(result, PRECONF_SET_FAIL);
            return;
        }

        // check if the sales code in tmp file saved well
        if((fp_tmp = fopen(SALES_CODE_PATH_TMP, "r")) == NULL) {
            ALOGD("sales code tmp file open err");
            strcpy(result, PRECONF_SET_FAIL);
            return;
        } else {
            ALOGD("sales code file tmp open OK");
            fread(stored_sales_code, 1, 15, fp_tmp);
            ALOGD("sales code in file ==> %s", stored_sales_code);
            fclose(fp_tmp);
        }

        chmod(SALES_CODE_PATH_TMP, 0664);
        if(strncmp((char*)sales_code, (char*)stored_sales_code, 3)) {
            strcpy(result, PRECONF_SET_FAIL);
            return;
        }
    }
}

void requestGetPreConfiguration(char *ATCresult) {
    FILE *fp = NULL;
    char *result = ATCresult;

    if((fp = fopen(SALES_CODE_PATH, "r")) == NULL) {
        ALOGD("sales code file open err");
        strcpy(result, PRECONF_GET_FAIL);
    } else {
        ALOGD("sales code file open OK");
        fread(result, 1, 15, fp);
        result[3]=0;
        ALOGD("sales code in file ==> %s", result);
        fclose(fp);
#if 0
// LTN_SW1_SYSTEM kwanguk.kim 2012.05.22 - TFG IMEI issue
        if(!strncmp(SecNativeFeature_getString(TAG_CSCFEATURE_RIL_REPLACE_IMEI_SALESCODE_AS), "TFG", 3)) {
            if (!(strncmp(result, "TFG", 3))) {
                char product_code[PROPERTY_VALUE_MAX] = {0,};
                ALOGD("[LTN] PRECONFIG_READ: TFG sales_code should be changed");
                Flash_Read_NV_Data(product_code, NV_SELLOUT_PRODUCT_CODE_I);
                if (strlen(product_code) > 2) {
                    ALOGD("[LTN] product_code : %s", product_code);
                    strncpy(result, product_code + (strlen(product_code) - 3), 3);
                }
            }
        }
#endif
// end - LTN_SW1_SYSTEM kwanguk.kim 2012.05.22 - TFG IMEI issue
    }
}

void sril_TotalCallTimeHandler(int request, void *data, size_t datalen, RIL_Token t) {
    OemReqMsgHdr * pSvcCmd = (OemReqMsgHdr *)data;

    ALOGD("%s: IMEI[%x]\n",__FUNCTION__, pSvcCmd->subfId);
    ALOGE("########## URIL  Request Imei Handler   ############\n");

    switch(pSvcCmd->subfId) {
        case OEM_CALL_GET_LIFETIMECALL:
            sril_GetTotalCallTime(request, data, datalen, t);
            break;

        case OEM_CALL_SET_LIFETIMECALL:
            sril_SetTotalCallTime(request, data, datalen, t);
            break;
        default:
            ALOGD("%s: Not supported Command [%x]\n",__FUNCTION__, pSvcCmd->subfId);
            RIL_onRequestComplete(t, RIL_E_REQUEST_NOT_SUPPORTED, NULL, 0);
            break;
    }
}

void sril_GetTotalCallTime(int request, void *data, size_t datalen, RIL_Token t) {
    unsigned long call_time_struct[7];
    unsigned long call_time = 0;
    int resp_size = sizeof(call_time_struct);

    ALOGE("uSril_GetTotalCallTime ");
    memset(&call_time_struct, 0, sizeof(call_time_struct));

    if(Flash_Read_NV_Data(&call_time, NV_TOTAL_CALLTIME)) {
        ALOGD("uSril_GetTotalCallTime = NV stored call time  >>> %x\n", call_time);
        call_time_struct[3] = call_time;
        RIL_onRequestComplete(t, RIL_E_SUCCESS, (void*)&call_time_struct, resp_size);
    } else {
        ALOGD("uSril_GetTotalCallTime = Read call time ERR >>> \n");
        RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
    }
}

void sril_SetTotalCallTime(int request, void *data, size_t datalen, RIL_Token t) {
    unsigned long call_time;

    if (data != NULL)
        memcpy(&call_time, (char*)(data+4), sizeof(call_time));

    if(!Flash_Write_NV_Data(&call_time, NV_TOTAL_CALLTIME)) {
        ALOGD("uSril_SetTotalCallTime = Write call time  >>> %x \n", call_time);
        RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
    }
    else{
        ALOGD("uSril_SetTotalCallTime = Write call time ERR >>> %x \n", call_time);
        RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
    }
}

#endif    //    _SRIL_IMEI_C

